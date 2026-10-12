#!/usr/bin/env python3
# usage: vu2cpp.py <micromem.bin> <first> <last> <name> [--ranges a-b,c-d] > out.inc
#        vu2cpp.py <micromem.bin> <first> <last> <name> --fast --entries 0x000,.. --exact <step fn> [--stop-backward] > out.inc
#
# Ports a VU micro program instruction by instruction: a C++ step function (runtime/vu/
# vu_native.h's building blocks) with one case per micro address in [first, last], running the
# instruction at pipe.pc exactly as the VU interpreter does (the upper and lower ops of a bundle
# both read the registers as they were before it; the upper's result keeps the lanes the lower
# changed; Q, MAC flags, branch delay, E bit through the pipeline helpers). The generated
# program is resumable at any address (a VU0 program waiting on the EE), and is proved against
# the interpreter running the uploaded original. Instructions it cannot port are emitted as
# `#error` lines.
import re
import struct
import sys

LANES = 'xyzw'


# the micro memory's address wrap for branch targets: 0xffff (a target outside the program stops it) by default; the world's VU1 programs
# (tools/codegen/gen_world_vu.py) use 0x7ff: a branch / JR target is a 11-bit address and the code relies on it (`b` at 0x1c with a target of -0x1ae
# lands on 0x652 in tfrag program A)
TGT_MASK = 0xffff

# the integer branch hazard (a conditional branch right after an ALU op / MTIR of the register it tests sees the old
# value) is modelled by default; HAZARD = False generates as before, the moby programs' committed ports
# (tools/codegen/gen_moby_vu_fast.py: regenerated with the hazard, the moby VU1 program no longer matches the original: an open question)
HAZARD = True
# a JR / JALR target masked to the micro memory; JR_MASK = False: the register's value as is, as before
JR_MASK = True


def _jr(r):
    return f'static_cast<uint32_t>(s.vi[{r}]) & 0x{TGT_MASK:x}u' if JR_MASK else f'static_cast<uint32_t>(s.vi[{r}])'


def mask_of(dest):  # dest field bits: x = 8, y = 4, z = 2, w = 1 -> lanes 0..3
    return [l for l in range(4) if (dest >> (3 - l)) & 1]


class Ins:
    def __init__(self, pc, lo, up):
        self.pc, self.lo, self.up = pc, lo, up


def vf(r, pre):
    return f'pre{r}' if r in pre else f's.vf[{r}]'


def upper(u, pre, ldest):
    """(code, writes_vf_reg or None, is_nop) for the upper instruction (COP2 encoding, bits 0..24)."""
    dest, ft, fs, fd, fn = (u >> 21) & 15, (u >> 16) & 31, (u >> 11) & 31, (u >> 6) & 31, u & 63
    lanes = mask_of(dest)
    S, T = (lambda l: f'cop({vf(fs, pre)}.u[{l}])'), (lambda l: f'cop({vf(ft, pre)}.u[{l}])')
    ACC = lambda l: 'cop(s.acc.u[%d])' % l
    out = []

    def fmac(dst_reg, expr, to_acc=False):
        # results into a temp, then the lanes (the lower's changed lanes kept), MAC flags, status
        tgt = 's.acc' if to_acc else f's.vf[{dst_reg}]'
        lines = []
        for l in range(4):
            if l in lanes:
                lines.append(f'const uint32_t r{l} = macw(s, {l}, {expr(l)});')
            else:
                lines.append(f'macclr(s, {l});')
        for l in lanes if to_acc or dst_reg != 0 else []:  # (vf0 is constant: only the flags)
            if not to_acc and dst_reg == ldest:
                lines.append(f'if ({tgt}.u[{l}] == pre{dst_reg}.u[{l}]) {tgt}.u[{l}] = r{l};')
            else:
                lines.append(f'{tgt}.u[{l}] = r{l};')
        lines.append('status(s);')
        return lines

    def raw(dst_reg, expr):  # no MAC flags (MAX / MINI / ITOF / FTOI / ABS)
        if dst_reg == 0: return []
        tgt = f's.vf[{dst_reg}]'
        lines = [f'const uint32_t r{l} = {expr(l)};' for l in lanes]
        for l in lanes:
            if dst_reg == ldest:
                lines.append(f'if ({tgt}.u[{l}] == pre{dst_reg}.u[{l}]) {tgt}.u[{l}] = r{l};')
            else:
                lines.append(f'{tgt}.u[{l}] = r{l};')
        return lines

    Q, I = 'copf(s.q)', 'copf(s.i)'
    if fn < 0x3c:
        bc = fn & 3
        B = lambda l: f'cop({vf(ft, pre)}.u[{bc}])'
        grp = fn >> 2
        if fn < 0x1c:
            if grp == 0: return fmac(fd, lambda l: f'{S(l)} + {B(l)}'), fd, False
            if grp == 1: return fmac(fd, lambda l: f'{S(l)} - {B(l)}'), fd, False
            if grp == 2: return fmac(fd, lambda l: f'{ACC(l)} + {S(l)} * {B(l)}'), fd, False
            if grp == 3: return fmac(fd, lambda l: f'{ACC(l)} - {S(l)} * {B(l)}'), fd, False
            if grp == 4: return raw(fd, lambda l: f'fmax_u({vf(fs, pre)}.u[{l}], {vf(ft, pre)}.u[{bc}])'), fd, False
            if grp == 5: return raw(fd, lambda l: f'fmin_u({vf(fs, pre)}.u[{l}], {vf(ft, pre)}.u[{bc}])'), fd, False
            if grp == 6: return fmac(fd, lambda l: f'{S(l)} * {B(l)}'), fd, False
        t = {0x1c: ('*', Q), 0x1e: ('*', I), 0x20: ('+', Q), 0x22: ('+', I), 0x24: ('-', Q), 0x26: ('-', I)}
        if fn in t: return fmac(fd, lambda l: f'{S(l)} {t[fn][0]} {t[fn][1]}'), fd, False
        t = {0x21: ('+', Q), 0x23: ('+', I), 0x25: ('-', Q), 0x27: ('-', I)}
        if fn in t: return fmac(fd, lambda l: f'{ACC(l)} {t[fn][0]} {S(l)} * {t[fn][1]}'), fd, False
        if fn == 0x1d: return raw(fd, lambda l: f'fmax_u({vf(fs, pre)}.u[{l}], u_of(s.i))'), fd, False
        if fn == 0x1f: return raw(fd, lambda l: f'fmin_u({vf(fs, pre)}.u[{l}], u_of(s.i))'), fd, False
        v = {0x28: lambda l: f'{S(l)} + {T(l)}', 0x2a: lambda l: f'{S(l)} * {T(l)}', 0x2c: lambda l: f'{S(l)} - {T(l)}',
             0x29: lambda l: f'{ACC(l)} + {S(l)} * {T(l)}', 0x2d: lambda l: f'{ACC(l)} - {S(l)} * {T(l)}'}
        if fn in v: return fmac(fd, v[fn]), fd, False
        if fn == 0x2e:  # OPMSUB (x / y / z; w's flags cleared; fd 0: only the status)
            if fd == 0: return ['status(s);'], None, False
            sub = {0: (1, 2), 1: (2, 0), 2: (0, 1)}
            lines = []
            for l in range(3):
                a, b = sub[l]
                if l in lanes:
                    lines.append(f'const uint32_t r{l} = macw(s, {l}, {ACC(l)} - cop({vf(fs, pre)}.u[{a}]) * cop({vf(ft, pre)}.u[{b}]));')
                else:
                    lines.append(f'macclr(s, {l});')
            lines.append('macclr(s, 3);')
            for l in lanes:
                if l == 3: continue
                if fd == ldest: lines.append(f'if (s.vf[{fd}].u[{l}] == pre{fd}.u[{l}]) s.vf[{fd}].u[{l}] = r{l};')
                else: lines.append(f's.vf[{fd}].u[{l}] = r{l};')
            return lines + ['status(s);'], fd, False
        if fn == 0x2b: return raw(fd, lambda l: f'fmax_u({vf(fs, pre)}.u[{l}], {vf(ft, pre)}.u[{l}])'), fd, False
        if fn == 0x2f: return raw(fd, lambda l: f'fmin_u({vf(fs, pre)}.u[{l}], {vf(ft, pre)}.u[{l}])'), fd, False
        return [f'#error upper {u:08x}'], None, False
    op2 = (fd << 2) | (fn & 3)
    bc = op2 & 3
    B = lambda l: f'cop({vf(ft, pre)}.u[{bc}])'
    if op2 < 0x10:
        e = [lambda l: f'{S(l)} + {B(l)}', lambda l: f'{S(l)} - {B(l)}', lambda l: f'{ACC(l)} + {S(l)} * {B(l)}',
             lambda l: f'{ACC(l)} - {S(l)} * {B(l)}'][op2 >> 2]
        return fmac(None, e, True), None, False
    if 0x10 <= op2 < 0x14:
        sc = ['1.0f', '(1.0f / 16.0f)', '(1.0f / 4096.0f)', '(1.0f / 32768.0f)'][op2 & 3]
        return raw(ft, lambda l: f'u_of(static_cast<float>(static_cast<int32_t>({vf(fs, pre)}.u[{l}])) * {sc})'), ft, False
    if 0x14 <= op2 < 0x18:
        sc = ['1.0f', '16.0f', '4096.0f', '32768.0f'][op2 & 3]
        return raw(ft, lambda l: f'ftoi({vf(fs, pre)}.u[{l}], {sc})'), ft, False
    if 0x18 <= op2 < 0x1c: return fmac(None, lambda l: f'{S(l)} * {B(l)}', True), None, False
    t = {0x1c: f'{{S}} * {Q}', 0x1e: f'{{S}} * {I}', 0x20: f'{{S}} + {Q}', 0x22: f'{{S}} + {I}', 0x24: f'{{S}} - {Q}', 0x26: f'{{S}} - {I}',
         0x21: f'{{A}} + {{S}} * {Q}', 0x23: f'{{A}} + {{S}} * {I}', 0x25: f'{{A}} - {{S}} * {Q}', 0x27: f'{{A}} - {{S}} * {I}'}
    if op2 in t: return fmac(None, lambda l: t[op2].replace('{S}', S(l)).replace('{A}', ACC(l)), True), None, False
    if op2 == 0x1d: return raw(ft, lambda l: f'({vf(fs, pre)}.u[{l}] & 0x7fffffffu)'), ft, False
    v = {0x28: lambda l: f'{S(l)} + {T(l)}', 0x2a: lambda l: f'{S(l)} * {T(l)}', 0x2c: lambda l: f'{S(l)} - {T(l)}',
         0x29: lambda l: f'{ACC(l)} + {S(l)} * {T(l)}', 0x2d: lambda l: f'{ACC(l)} - {S(l)} * {T(l)}'}
    if op2 in v: return fmac(None, v[op2], True), None, False
    if op2 == 0x2f: return [], None, True  # NOP
    if op2 == 0x1f:  # CLIPw.xyz
        a, b = vf(fs, pre), vf(ft, pre)
        return [f'clip(s, {a}.u[0], {a}.u[1], {a}.u[2], {b}.u[3]);'], None, False
    if op2 == 0x2e:  # OPMULA (w's flags kept)
        a, b = vf(fs, pre), vf(ft, pre)
        return [f's.acc.u[0] = macw(s, 0, cop({a}.u[1]) * cop({b}.u[2]));', f's.acc.u[1] = macw(s, 1, cop({a}.u[2]) * cop({b}.u[0]));',
                f's.acc.u[2] = macw(s, 2, cop({a}.u[0]) * cop({b}.u[1]));', 'status(s);'], None, False
    return [f'#error upper special2 {u:08x}'], None, False


def lower_info(l):
    """(kind, dest vf reg or None) used to plan the bundle: what vf register the lower writes."""
    if l & 0x80000000:
        fn = l & 63
        if (fn & 0x3c) == 0x3c:
            op2 = (l & 3) | ((l >> 4) & 0x7c)
            ft, fs = (l >> 16) & 31, (l >> 11) & 31
            if op2 in (0x30, 0x31): return op2, (ft if ft else None)
            if op2 == 0x3d: return op2, (ft if ft else None)  # MFIR
            if op2 in (0x34, 0x36): return op2, (ft if ft else None)  # LQI / LQD
            return op2, None
        return ('s1', fn), None
    op = (l >> 25) & 0x7f
    if op == 0x00:
        rt = (l >> 16) & 31
        return ('lq',), (rt if rt else None)
    return ('l2', op), None


def hz_writer(l):
    """the integer register a lower instruction writes through the 1-cycle integer unit, whose new value a conditional branch right after it does not
    see (the integer branch hazard, runtime/vu/vu_interpreter.cpp): the ALU ops and MTIR; 0 for any other instruction. """
    if l & 0x80000000:
        fn = l & 63
        if (fn & 0x3c) == 0x3c:
            op2 = (l & 3) | ((l >> 4) & 0x7c)
            return ((l >> 16) & 15) if op2 == 0x3c else 0
        if fn in (0x30, 0x31, 0x34, 0x35): return (l >> 6) & 15
        if fn == 0x32: return (l >> 16) & 15
        return 0
    op = (l >> 25) & 0x7f
    return ((l >> 16) & 15) if op in (0x08, 0x09) else 0


def lower(pc, l, pre):
    """(code, efu op or None, branch?) for the lower instruction."""
    if l & 0x80000000:
        fn = l & 63
        ft, fs, fd = (l >> 16) & 31, (l >> 11) & 31, (l >> 6) & 31
        it, is_, id_ = ft & 15, fs & 15, fd & 15
        dest = (l >> 21) & 15
        lanes = mask_of(dest)
        if (fn & 0x3c) == 0x3c:
            op2 = (l & 3) | ((l >> 4) & 0x7c)
            fsf, ftf = (l >> 21) & 3, (l >> 23) & 3
            if op2 == 0x30:  # MOVE
                if ft == 0: return [], None, False
                return [f's.vf[{ft}].u[{x}] = s.vf[{fs}].u[{x}];' for x in lanes] if fs != ft else [], None, False
            if op2 == 0x31:  # MR32
                if ft == 0: return [], None, False
                src = 'mr' if fs == ft else f's.vf[{fs}]'
                c = [f'const VUVector mr = s.vf[{fs}];'] if fs == ft else []
                return c + [f's.vf[{ft}].u[{x}] = {src}.u[{(x + 1) & 3}];' for x in lanes], None, False
            if op2 == 0x3c:  # MTIR
                return ([f's.vi[{it}] = static_cast<int16_t>(s.vf[{fs}].u[{fsf}] & 0xffff);'] if it else []), None, False
            if op2 == 0x3d:  # MFIR
                if ft == 0: return [], None, False
                return [f's.vf[{ft}].u[{x}] = static_cast<uint32_t>(static_cast<int32_t>(s.vi[{is_}]));' for x in lanes], None, False
            if op2 == 0x3e:  # ILWR (the interpreter's: one lane's low half; with several lanes the last of x y z w wins)
                return ([f's.vi[{it}] = static_cast<int16_t>(qw(s, s.vi[{is_}])[{lanes[-1]}] & 0xffff);'] if it and lanes else []), None, False
            if op2 == 0x3f:  # ISWR (a lane's low half = vi[it], its high half 0)
                return [f'qw(s, s.vi[{is_}])[{x}] = static_cast<uint16_t>(s.vi[{it}]);' for x in lanes], None, False
            if op2 == 0x34:  # LQI
                c = ([f'{{ const uint32_t* m = qw(s, s.vi[{is_}]); ' + ' '.join(f's.vf[{ft}].u[{x}] = m[{x}];' for x in lanes) + ' }'] if ft else [])
                return c + ([f's.vi[{is_}] = static_cast<int16_t>(s.vi[{is_}] + 1);'] if fs and is_ else []), None, False
            if op2 == 0x35:  # SQI
                c = [f'{{ uint32_t* m = qw(s, s.vi[{it}]); ' + ' '.join(f'm[{x}] = s.vf[{fs}].u[{x}];' for x in lanes) + ' }']
                return c + ([f's.vi[{it}] = static_cast<int16_t>(s.vi[{it}] + 1);'] if ft and it else []), None, False
            if op2 == 0x36:  # LQD
                c = [f's.vi[{is_}] = static_cast<int16_t>(s.vi[{is_}] - 1);'] if is_ else []
                return c + ([f'{{ const uint32_t* m = qw(s, s.vi[{is_}]); ' + ' '.join(f's.vf[{ft}].u[{x}] = m[{x}];' for x in lanes) + ' }'] if ft else []), None, False
            if op2 == 0x37:  # SQD
                c = [f's.vi[{it}] = static_cast<int16_t>(s.vi[{it}] - 1);'] if it else []
                return c + [f'{{ uint32_t* m = qw(s, s.vi[{it}]); ' + ' '.join(f'm[{x}] = s.vf[{fs}].u[{x}];' for x in lanes) + ' }'], None, False
            if op2 == 0x38: return [f'vdiv(s, s.vf[{fs}].u[{fsf}], s.vf[{ft}].u[{ftf}]);'], 0x38, False
            if op2 == 0x39: return [f'vsqrt(s, s.vf[{ft}].u[{ftf}]);'], 0x39, False
            if op2 == 0x3a: return [f'vrsqrt(s, s.vf[{fs}].u[{fsf}], s.vf[{ft}].u[{ftf}]);'], 0x3a, False
            if op2 == 0x3b: return [], 0x3b, False
            if op2 == 0x6c: return [f'xgkick(s, p, static_cast<uint16_t>(s.vi[{is_}]));'], None, False
            if op2 == 0x68: return ([f's.vi[{it}] = static_cast<int16_t>(s.xtop & 0x3ff);'] if it else []), None, False
            return [f'#error lower special2 op {op2:#x} {l:08x}'], None, False
        if fn == 0x30: return ([f's.vi[{id_}] = static_cast<int16_t>(s.vi[{is_}] + s.vi[{it}]);'] if id_ else []), None, False
        if fn == 0x31: return ([f's.vi[{id_}] = static_cast<int16_t>(s.vi[{is_}] - s.vi[{it}]);'] if id_ else []), None, False
        if fn == 0x32:
            imm5 = (l >> 6) & 31
            if imm5 & 0x10: imm5 -= 32
            return ([f's.vi[{it}] = static_cast<int16_t>(s.vi[{is_}] + ({imm5}));'] if it else []), None, False
        if fn == 0x34: return ([f's.vi[{id_}] = static_cast<int16_t>(s.vi[{is_}] & s.vi[{it}]);'] if id_ else []), None, False
        if fn == 0x35: return ([f's.vi[{id_}] = static_cast<int16_t>(s.vi[{is_}] | s.vi[{it}]);'] if id_ else []), None, False
        return [f'#error lower special1 fn {fn:#x} {l:08x}'], None, False
    op = (l >> 25) & 0x7f
    it, is_ = (l >> 16) & 15, (l >> 11) & 15
    rt, fs = (l >> 16) & 31, (l >> 11) & 31
    lanes = mask_of((l >> 21) & 15)
    imm11 = l & 0x7ff
    if imm11 & 0x400: imm11 -= 0x800
    tgt = pc + 1 + imm11
    if op == 0x00:  # LQ
        if rt == 0: return [], None, False
        return [f'{{ const uint32_t* m = qw(s, s.vi[{is_}] + ({imm11})); ' + ' '.join(f's.vf[{rt}].u[{x}] = m[{x}];' for x in lanes) + ' }'], None, False
    if op == 0x01:  # SQ
        return [f'{{ uint32_t* m = qw(s, s.vi[{it}] + ({imm11})); ' + ' '.join(f'm[{x}] = s.vf[{fs}].u[{x}];' for x in lanes) + ' }'], None, False
    if op == 0x04:  # ILW
        if it == 0: return [], None, False
        x = lanes[0] if lanes else 0
        return [f's.vi[{it}] = static_cast<int16_t>(qw(s, s.vi[{is_}] + ({imm11}))[{x}] & 0xffff);'], None, False
    if op == 0x05:  # ISW
        return [f'{{ uint32_t* m = qw(s, s.vi[{is_}] + ({imm11})); ' + ' '.join(f'm[{x}] = static_cast<uint16_t>(s.vi[{it}]);' for x in lanes) + ' }'], None, False
    if op in (0x08, 0x09):  # IADDIU / ISUBIU (unsigned imm15)
        imm15 = (l & 0x7ff) | (((l >> 21) & 15) << 11)
        o = '+' if op == 0x08 else '-'
        return ([f's.vi[{it}] = static_cast<int16_t>(s.vi[{is_}] {o} {imm15});'] if it else []), None, False
    if op == 0x18:
        return ([f's.vi[{it}] = (static_cast<uint32_t>(static_cast<uint16_t>(s.vi[{is_}])) == (s.mac_visible & 0xffff)) ? 1 : 0;'] if it else []), None, False
    if op == 0x1a:
        return ([f's.vi[{it}] = static_cast<int16_t>(static_cast<uint16_t>(s.vi[{is_}]) & (s.mac_visible & 0xffff));'] if it else []), None, False
    if op == 0x1b:
        return ([f's.vi[{it}] = static_cast<int16_t>(static_cast<uint16_t>(s.vi[{is_}]) | (s.mac_visible & 0xffff));'] if it else []), None, False
    imm12 = ((l >> 10) & 0x800) | (l & 0x7ff)
    if op == 0x10: return [f's.vi[1] = (s.clip_visible & 0xffffff) == 0x{l & 0xffffff:x}u ? 1 : 0;'], None, False
    if op == 0x11: return [f's.clipflag = 0x{l & 0xffffff:x}u;'], None, False
    if op == 0x12: return [f's.vi[1] = (s.clip_visible & 0x{l & 0xffffff:x}u) != 0 ? 1 : 0;'], None, False
    if op == 0x13: return [f's.vi[1] = ((s.clip_visible | 0x{l & 0xffffff:x}u) & 0xffffff) == 0xffffff ? 1 : 0;'], None, False
    if op == 0x14: return ([f's.vi[{it}] = (s.status_visible & 0xfff) == 0x{imm12:x}u ? 1 : 0;'] if it else []), None, False
    if op == 0x15: return [f's.statusflag = (s.statusflag & 0x3f) | 0x{imm12 & 0xfc0:x}u;'], None, False
    if op == 0x16: return ([f's.vi[{it}] = static_cast<int16_t>(s.status_visible & 0x{imm12:x}u);'] if it else []), None, False
    if op == 0x17: return ([f's.vi[{it}] = static_cast<int16_t>((s.status_visible & 0xfff) | 0x{imm12:x}u);'] if it else []), None, False
    if op == 0x1c: return ([f's.vi[{it}] = static_cast<int16_t>(s.clip_visible & 0xfff);'] if it else []), None, False
    if op == 0x20: return [f'taken = true; target = 0x{tgt & TGT_MASK:x};'], None, True
    if op == 0x21: return ([f's.vi[{it}] = 0x{pc + 2:x};'] if it else []) + [f'taken = true; target = 0x{tgt & TGT_MASK:x};'], None, True
    if op == 0x24: return [f'taken = true; target = {_jr(is_)};'], None, True
    if op == 0x25:
        return [f'const uint32_t jt = {_jr(is_)};'] + ([f's.vi[{it}] = 0x{pc + 2:x};'] if it else []) + ['taken = true; target = jt;'], None, True
    # (a conditional branch right after an ALU op / MTIR sees the register's value from before it: hzv, the VuPipe's backup)
    conds = {0x28: f'hzv(p, s, {is_}) == hzv(p, s, {it})', 0x29: f'hzv(p, s, {is_}) != hzv(p, s, {it})', 0x2c: f'hzv(p, s, {is_}) < 0', 0x2d: f'hzv(p, s, {is_}) > 0',
             0x2e: f'hzv(p, s, {is_}) <= 0', 0x2f: f'hzv(p, s, {is_}) >= 0'}
    if not HAZARD:
        conds = {k: re.sub(r'hzv\(p, s, (\d+)\)', r's.vi[\1]', v) for k, v in conds.items()}
    if op in conds: return [f'if ({conds[op]}) {{ taken = true; target = 0x{tgt & TGT_MASK:x}; }}'], None, True
    return [f'#error lower2 op {op:#x} {l:08x}'], None, False


def upper_sources(u):
    dest, ft, fs, fd, fn = (u >> 21) & 15, (u >> 16) & 31, (u >> 11) & 31, (u >> 6) & 31, u & 63
    return {fs, ft}


def gen(mem, first, last, name):
    out = [f'// orig: (vu) micro {first:#05x}..{last:#05x} -- generated by tools/codegen/vu2cpp.py, see the header',
           f'bool {name}(VUState& s, VuPipe& p) {{', '    using namespace vun;', '    bool taken = false;', '    uint32_t target = 0;',
           '    switch (p.pc) {']
    for pc in range(first, last + 1):
        lo, up = struct.unpack_from('<II', mem, pc * 8)
        out.append(f'    case 0x{pc:03x}: {{')
        loi = (up & 0x80000000) != 0
        efu_op = None
        body = []
        if not loi:
            kind, ldest = lower_info(lo)
            is_efu = (lo & 0x80000000) != 0 and (lo & 0x3c) == 0x3c and ((lo & 3) | ((lo >> 4) & 0x7c)) in (0x38, 0x39, 0x3a, 0x3b)
        else:
            ldest, is_efu = None, False
        srcs = upper_sources(up & 0x1ffffff)
        # registers the upper reads that the lower writes: read before the lower
        pre = set()
        if ldest is not None:
            pre.add(ldest)  # (its old value also decides which lanes the lower changed)
        body.append(f'        begin(s, p, {"true" if is_efu else "false"});')
        if up & 0x40000000: body.append('        ebit(p);')
        if is_efu: body.append('        const float q_before = s.q;')
        uc, _, nop = upper(up & 0x1ffffff, pre, ldest)
        for r in sorted(pre):
            if any(f'pre{r}' in x for x in uc): body.append(f'        const VUVector pre{r} = s.vf[{r}];')
        if not loi:
            wreg = hz_writer(lo) if HAZARD else 0
            if wreg: body.append(f'        const int16_t hzo = s.vi[{wreg}];')
            lc, efu_op, _ = lower(pc, lo, pre)
            body += ['        ' + x for x in lc]
            if HAZARD: body.append(f'        p.vi_hz_reg = {wreg if wreg else -1};' + (' p.vi_hz_old = hzo;' if wreg else ''))
            if efu_op is not None: body.append(f'        efu_issued(s, p, q_before, 0x{efu_op:x});')
        elif HAZARD:
            body.append('        p.vi_hz_reg = -1;')
        body += ['        ' + x for x in uc]
        if loi: body.append(f'        s.i = f_of(0x{lo:08x}u);')
        body.append('        return end(s, p, taken, target);')
        out += body
        out.append('    }')
    out += ['    default: break;', '    }', '    return false;', '}']
    return '\n'.join(out)


if __name__ == '__main__':
    # exact mode (the default): one case per micro address, one instruction per call, the pipeline
    # bookkeeping on every instruction (what the VU interpreter does);
    # --fast: basic blocks, see vu2cpp_fast.py:
    #   vu2cpp.py <micromem.bin> <first> <last> <name> --fast --entries 0x000,0x111 --exact <exact step fn> [--stop-backward]
    mem = open(sys.argv[1], 'rb').read()
    if '--fast' in sys.argv:
        import vu2cpp_fast
        arg = lambda k, d=None: sys.argv[sys.argv.index(k) + 1] if k in sys.argv else d
        entries = [int(x, 0) for x in arg('--entries').split(',')]
        print(vu2cpp_fast.gen_fast(mem, int(sys.argv[2], 0), int(sys.argv[3], 0), sys.argv[4], entries, arg('--exact'), '--stop-backward' in sys.argv,
                                   int(arg('--size', '512'), 0), '--debug-stop' in sys.argv))
    else:
        print(gen(mem, int(sys.argv[2], 0), int(sys.argv[3], 0), sys.argv[4]))
