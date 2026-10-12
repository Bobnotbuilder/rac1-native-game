#!/usr/bin/env python3
# The fast mode of vu2cpp.py (vu2cpp.py --fast ...): the same VU micro program as basic blocks in
# one C++ function, observably identical to the exact step-by-step port (and so to the VU
# interpreter) at the points where its state can be seen -- a block's boundary where the run
# stops (a taken backward jump for VU0: the EE looks at the registers there; the end of the
# program; a jump to code that is not a block), the XGKICK packets (address and contents, at the
# same instruction), the data memory, vf / vi / ACC / Q / I and the MAC / status / clip flags.
#
# What is not done per instruction any more:
#  - the dispatch (switch on pc) and the pipeline bookkeeping (rings, issued, branch / E / kick
#    countdowns): the control flow of a block is static; a jump is a goto;
#  - the flag work nobody can read: the flags live in locals (FlagRegs), the compiler drops what
#    nothing reads; the 4-instruction latency of the MAC / status / clip flags is a window of the
#    last four values per flag kind, kept only for the kinds a program reads (FMxxx / FSxxx / FCxxx);
#  - the lane-by-lane float rules: an FMAC op is done on four lanes with SSE2 (vu_native_fast.h);
#    an operand known to be clamped already (a result of an earlier op of the block) is not
#    clamped again.
# What stays exact: a bundle's reads before its writes, the lanes the lower op changed, Q's
# landing 7 / 13 instructions after an EFU op (a counter against the issue index), the branch delay
# slot, the E bit's one more instruction, XGKICK after the next instruction.
#
# Where it hands over to the exact step function (one instruction per call, the exact state in the
# VuPipe): a call that starts anywhere but at a block's start or with a branch / kick / halt in
# flight, a jump to a pc that is not a block (JR to a computed address), the end of the ported range.
import re
import struct
import sys

import vu2cpp as X

BRANCH_OPS = {0x20: 'b', 0x21: 'bal', 0x24: 'jr', 0x25: 'jalr', 0x28: 'bc', 0x29: 'bc', 0x2c: 'bc', 0x2d: 'bc', 0x2e: 'bc', 0x2f: 'bc'}


class Ins:
    pass


def decode(mem, pc):
    lo, up = struct.unpack_from('<II', mem, pc * 8)
    i = Ins()
    i.pc, i.lo, i.up = pc, lo, up
    i.loi = (up & 0x80000000) != 0
    i.ebit = (up & 0x40000000) != 0
    i.br = None
    i.xgkick = None
    i.efu = None
    if not i.loi:
        if (lo & 0x80000000) == 0:
            op = (lo >> 25) & 0x7f
            if op in BRANCH_OPS:
                kind = BRANCH_OPS[op]
                it, is_ = (lo >> 16) & 15, (lo >> 11) & 15
                imm11 = lo & 0x7ff
                if imm11 & 0x400:
                    imm11 -= 0x800
                tgt = (pc + 1 + imm11) & X.TGT_MASK
                i.br = dict(kind=kind, op=op, target=tgt, it=it, is_=is_)
                if kind in ('bal', 'jalr'):
                    i.br['link'] = pc + 2
        else:
            fn = lo & 63
            if (fn & 0x3c) == 0x3c:
                op2 = (lo & 3) | ((lo >> 4) & 0x7c)
                if op2 == 0x6c:
                    i.xgkick = (lo >> 11) & 15
                if op2 in (0x38, 0x39, 0x3a, 0x3b):
                    i.efu = op2
    return i


def reads_of(mem, first, last):
    """which flag kinds the program reads: FMxxx (mac), FSxxx (status), FCxxx (clip)."""
    r = {'m': False, 's': False, 'c': False}
    for pc in range(first, last + 1):
        lo, up = struct.unpack_from('<II', mem, pc * 8)
        if (up & 0x80000000) != 0 or (lo & 0x80000000) != 0:
            continue
        op = (lo >> 25) & 0x7f
        if op in (0x18, 0x1a, 0x1b): r['m'] = True
        if op in (0x14, 0x16, 0x17): r['s'] = True
        if op in (0x10, 0x12, 0x13, 0x1c): r['c'] = True
    return r


MUTATE = None  # (set by --mutate: a deliberately wrong generator, to show the proof catches it)
KIND = {'m': ('mac', 'macflag', 'mw', 'mac_visible', 'mac_ring'), 's': ('status', 'statusflag', 'sw', 'status_visible', 'status_ring'),
        'c': ('clip', 'clipflag', 'cw', 'clip_visible', 'clip_ring')}


def conv(line, B):
    """a line of the exact generator's text for the fast code: the flags / Q are in `f`."""
    for h in ('macw', 'macclr', 'status', 'clip', 'vdiv', 'vsqrt', 'vrsqrt'):
        line = line.replace(h + '(s, ', h + '(f, ').replace(h + '(s)', h + '(f)')
    line = line.replace('copf(s.q)', 'copf(f.q)')
    line = line.replace('s.statusflag', 'f.statusflag').replace('s.clipflag', 'f.clipflag').replace('s.macflag', 'f.macflag')
    for k in 'msc':
        nm = KIND[k][3]
        if 's.' + nm in line:
            line = line.replace('s.' + nm, '(' + B.win[k][B.j + (1 if MUTATE == 'latency' else 0)] + ')')
    return line


LQ_RE = re.compile(r'^\{ const uint32_t\* m = qw\(s, (.*)\); ((?:s\.vf\[\d+\]\.u\[\d\] = m\[\d\]; ?)+)\}$')
SQ_RE = re.compile(r'^\{ uint32_t\* m = qw\(s, (.*)\); ((?:m\[\d\] = s\.vf\[\d+\]\.u\[\d\]; ?)+)\}$')


def vec_mem(line):
    """LQ / SQ (and their increment / decrement forms) on the quadword at once."""
    m = LQ_RE.match(line)
    if m:
        asg = re.findall(r's\.vf\[(\d+)\]\.u\[(\d)\] = m\[(\d)\]', m.group(2))
        if all(a[1] == a[2] for a in asg) and len({a[0] for a in asg}) == 1:
            mask = 0
            for a in asg: mask |= 1 << int(a[1])
            return f'{{ st4m<{mask}>(s.vf[{asg[0][0]}], ldq4(qw(s, {m.group(1)}))); }}'
    m = SQ_RE.match(line)
    if m:
        asg = re.findall(r'm\[(\d)\] = s\.vf\[(\d+)\]\.u\[(\d)\]', m.group(2))
        if all(a[0] == a[2] for a in asg) and len({a[1] for a in asg}) == 1:
            mask = 0
            for a in asg: mask |= 1 << int(a[0])
            return f'{{ stq4m<{mask}>(qw(s, {m.group(1)}), ld4(s.vf[{asg[0][1]}])); }}'
    return line


class Block:
    pass


class Gen:
    def __init__(self, mem, first, last, name, entries, exact, stop_back, size):
        self.mem, self.first, self.last, self.name = mem, first, last, name
        self.entries, self.exact, self.stop_back, self.size = entries, exact, stop_back, size
        self.I = {pc: decode(mem, pc) for pc in range(first, last + 1)}
        self.reads = reads_of(mem, first, last)
        self.has_q = any(i.efu is not None for i in self.I.values())
        self.done, self.need = {}, list(entries)
        self.links = sorted({i.br['link'] for i in self.I.values() if i.br and 'link' in i.br and first <= i.br['link'] <= last})
        self.stats = dict(blocks=0, handoff_edges=0)
        self.mutate = None  # (testing the proof: a deliberately wrong generator: 'latency', 'qready', 'kick')
        self.debug_stop = False  # (debugging: every block boundary is an exit, so a test can compare there)

    # ---- window helpers
    def win_load(self):
        out = []
        for k in 'msc':
            if self.reads[k]:
                nm, _, w, _, ring = KIND[k]
                for a in range(4):
                    out.append(f'    [[maybe_unused]] uint32_t {w}{a} = p.{ring}[(iss + {a}) & 3];')
        return out

    def window_end(self, B, k, n):  # the window after n instructions: VV[n-4 .. n-1] = L[n .. n+3]
        return B.win[k][n:n + 4]

    def state_stores(self, B, n, pc_expr, halted, back):
        o = []
        o.append(f'{B.ind}s.macflag = f.macflag; s.statusflag = f.statusflag; s.clipflag = f.clipflag;')
        if self.has_q:
            o.append(f'{B.ind}if (qp && iss + {n} > qr) {{ f.q = qn; qp = false; }}')  # (the landing at the begin of the last instruction, iss + n - 1 >= qr)
            o.append(f'{B.ind}p.q_pending = qp; p.q_next = qn; p.q_ready = qr;')
        o.append(f'{B.ind}s.q = f.q;')
        o.append(f'{B.ind}p.issued = iss + {n}; p.pc = {pc_expr};')
        if X.HAZARD: o.append(f'{B.ind}p.vi_hz_reg = hzr; p.vi_hz_old = hzo;')
        for k in 'msc':
            if self.reads[k]:
                nm, _, w, _, ring = KIND[k]
                win = self.window_end(B, k, n)
                for a in range(4):
                    o.append(f'{B.ind}p.{ring}[(iss + {n - 4 + a + 8}) & 3] = {win[a]};')
        if B.kick is not None:
            o.append(f'{B.ind}p.kick_delay = 1; p.kick_addr = {B.kick};')
        if B.halt_in is not None and not halted:
            o.append(f'{B.ind}p.halt_pending = true; p.halt_delay = {B.halt_in};')
        if halted:
            o.append(f'{B.ind}p.halt_pending = true; p.halt_delay = 0; p.halted = true;')
        if back:
            o.append(f'{B.ind}p.back_jump = true;')
        return o

    def exit_code(self, B, n, pc_expr, halted=False, back=False):
        o = self.state_stores(B, n, pc_expr, halted, back)
        o.append(f'{B.ind}return false;' if halted else f'{B.ind}return p.pc < p.size;')
        return o

    def edge(self, B, target, n, ds_pc=None, taken=False):
        """the code that leaves the block after n instructions for pc `target`."""
        ind = B.ind
        backward = taken and self.stop_back and ds_pc is not None and target < ds_pc
        in_range = self.first <= target <= self.last
        if backward:
            return [f'{ind}{{'] + self.exit_code(B, n, f'0x{target:x}', back=True) + [f'{ind}}}']
        if self.debug_stop and in_range and target not in self.done and target not in self.need:
            self.need.append(target)
        if not in_range or B.kick is not None or B.halt_in is not None or self.debug_stop:
            self.stats['handoff_edges'] += 1
            return [f'{ind}{{'] + self.exit_code(B, n, f'0x{target:x}') + [f'{ind}}}']
        if target not in self.done and target not in self.need:
            self.need.append(target)
        o = [f'{ind}{{']
        wm = []
        for k in 'msc':
            if self.reads[k]:
                nm, _, w, _, ring = KIND[k]
                win = self.window_end(B, k, n)
                for a in range(4):
                    o.append(f'{ind}    const uint32_t t{k}{a} = {win[a]};')
                    wm.append(f'{w}{a} = t{k}{a};')
        o += [f'{ind}    ' + ' '.join(wm)] if wm else []
        o.append(f'{ind}    iss += {n}; goto B{target:03x};')
        o.append(f'{ind}}}')
        return o

    # ---- one instruction
    def instr(self, B, ins, j_lines):
        """emit the instruction (without its branch) into B.lines; B.j counts it."""
        L = B.lines
        ind = B.ind
        j = B.j
        pc = ins.pc
        lo, up = ins.lo, ins.up
        loi = ins.loi
        # kick pending from the previous instruction: runs at the end of this one
        flush_after = B.kick is not None
        kick_prev = B.kick
        if ins.xgkick is not None and B.kick is not None:
            L.append(f'{ind}kick_now(s, {B.kick});')
            flush_after = False
            B.kick = None
        if not loi:
            kind, ldest = X.lower_info(lo)
            is_efu = ins.efu is not None
        else:
            ldest, is_efu = None, False
        pre = set()
        if ldest is not None: pre.add(ldest)
        clean0 = dict(B.clean)
        acc0 = B.acc_clean
        L.append(f'{ind}// 0x{pc:03x}')
        if ins.br is not None:
            L.append(f'{ind}bool tk = false; uint32_t jt = 0; (void)tk; (void)jt;')
        body = []
        # upper
        uc, uw, nop = self.upper(up & 0x1ffffff, pre, ldest, clean0, acc0, B)
        lines_pre = []
        for r in sorted(pre):
            if any(f'pre{r}' in x for x in uc): lines_pre.append(f'const VUVector pre{r} = s.vf[{r}];')
        lc, efu_op = [], None
        if not loi:
            if ins.br is not None:
                lc = self.branch_lower(ins)
            else:
                lc, efu_op, _ = X.lower(pc, lo, pre)
                lc = [vec_mem(x) for x in lc]
        text_all = ' '.join(uc + lc)
        # Q: landing before the first reader / the forced landing of an EFU op
        if self.has_q:
            if is_efu: body.append('if (qp) { f.q = qn; qp = false; }')
            elif 'f.q' in text_all or 'copf(s.q)' in text_all: body.append(f'if (qp && iss + {j} >= qr) {{ f.q = qn; qp = false; }}')
        if is_efu: body.append('const float q_before = f.q;')
        body += lines_pre
        # the lower op
        wreg = 0 if loi or ins.br is not None or not X.HAZARD else X.hz_writer(lo)  # (a branch is no ALU / MTIR writer)
        if wreg: body.append(f'hzo = s.vi[{wreg}];')
        newkick = None
        for x in lc:
            m = re.match(r'xgkick\(s, p, (.*)\);', x)
            if m:
                L.append(f'{ind}uint32_t ka{j};')
                body.append(f'ka{j} = {m.group(1)};')
                newkick = f'ka{j}'
            else:
                body.append(x)
        if X.HAZARD: body.append(f'hzr = {wreg if wreg else -1};')
        if efu_op is not None:
            body.append(f'qn = f.q; f.q = q_before; qp = {"false" if efu_op == 0x3b else "true"}; qr = iss + {j} + {13 if efu_op == 0x3a else (6 if MUTATE == "qready" else 7)};')
        body += uc
        if loi: body.append(f's.i = f_of(0x{lo:08x}u);')
        L.append(ind + '{ // bundle')
        for x in body:
            L.append(ind + '    ' + conv(x, B))
        L.append(ind + '}')
        # the clean set
        self.update_clean(B, ins, uw, uc, ldest, up & 0x1ffffff, clean0)
        if flush_after and not (ins.xgkick is not None and kick_prev is not None):
            L.append(f'{ind}kick_now(s, {kick_prev});')
            B.kick = None
        if newkick is not None:
            B.kick = newkick
        # the flag windows
        txt = ' '.join(conv(x, B) for x in body)
        chg = {'m': bool(re.search(r'macw\(|macclr\(|fin<|f\.macflag =', txt)),
               's': bool(re.search(r'status\(f|status_efu|vdiv\(|vsqrt\(|vrsqrt\(|fin<|f\.statusflag =', txt)),
               'c': bool(re.search(r'clip\(f|f\.clipflag =', txt))}
        for k in 'msc':
            if not self.reads[k]:
                continue
            nm, reg, w, vis, ring = KIND[k]
            if chg[k] or B.win[k][-1] in (f'{w}0', f'{w}1', f'{w}2', f'{w}3'):
                L.append(f'{ind}[[maybe_unused]] const uint32_t {k}s{j} = f.{reg};')
                B.win[k].append(f'{k}s{j}')
            else:
                B.win[k].append(B.win[k][-1])
        B.j += 1
        # E bit
        if ins.ebit and B.halt_in is None:
            B.halt_in = 2
        halted_now = False
        if B.halt_in is not None:
            B.halt_in -= 1
            if B.halt_in == 0:
                halted_now = True
                B.halt_in = None
        return halted_now

    def branch_lower(self, ins):
        br = ins.br
        o = []
        k = br['kind']
        if k == 'bal':
            if br['it']: o.append(f's.vi[{br["it"]}] = 0x{br["link"]:x};')
        elif k == 'jalr':
            o.append(f'jt = {X._jr(br["is_"])};')
            if br['it']: o.append(f's.vi[{br["it"]}] = 0x{br["link"]:x};')
        elif k == 'jr':
            o.append(f'jt = {X._jr(br["is_"])};')
        elif k == 'bc':
            i, t = br['is_'], br['it']
            hv = (lambda r: f'(hzr == {r} ? hzo : s.vi[{r}])') if X.HAZARD else (lambda r: f's.vi[{r}]')  # (the integer branch hazard: see vu2cpp.hz_writer; hzr / hzo track the previous instruction's write)
            cond = {0x28: f'{hv(i)} == {hv(t)}', 0x29: f'{hv(i)} != {hv(t)}', 0x2c: f'{hv(i)} < 0', 0x2d: f'{hv(i)} > 0',
                    0x2e: f'{hv(i)} <= 0', 0x2f: f'{hv(i)} >= 0'}[br['op']]
            o.append(f'tk = {cond};')
        return o

    # ---- clean tracking
    def update_clean(self, B, ins, uw, uc, ldest, u, clean0):
        if ldest is not None:
            B.clean[ldest] = 0
        txt = ' '.join(uc)
        res = getattr(B, 'upper_res', None)
        if res is not None:
            kind, reg, m = res
            if kind == 'vf': B.clean[reg] = B.clean.get(reg, 0) | m
            else: B.acc_clean |= m
            B.upper_res = None
        else:
            if uw is not None: B.clean[uw] = 0
            if 's.acc.u[' in txt: B.acc_clean = 0
        B.clean[0] = 15

    # ---- the upper op: vector form when the bundle allows it, else the exact text
    def upper(self, u, pre, ldest, clean, acc_clean, B):
        B.upper_res = None
        dest, ft, fs, fd, fn = (u >> 21) & 15, (u >> 16) & 31, (u >> 11) & 31, (u >> 6) & 31, u & 63
        lanes = X.mask_of(dest)
        M = 0
        for l in lanes: M |= 1 << l
        vec = None  # (kind, a-spec, b-spec, to_acc, dst)
        def reg(r): return f'pre{r}' if r in pre else f's.vf[{r}]'
        def opd_vec(r, mask):
            e = f'ld4({reg(r)})'
            return e if (clean.get(r, 0) & mask) == mask else f'cop4({e})'
        def opd_bc(r, bc):
            e = f'VUN_BC(ld4({reg(r)}), {bc})'
            return e if (clean.get(r, 0) >> bc) & 1 else f'cop4({e})'
        def opd_acc(mask):
            e = 'ld4(s.acc)'
            return e if (acc_clean & mask) == mask else f'cop4({e})'
        def build(kindop, bspec, to_acc):
            if fd == ldest and not to_acc: return None
            a = opd_vec(fs, M)
            if bspec[0] == 'bc': b = opd_bc(ft, bspec[1])
            elif bspec[0] == 'vec': b = opd_vec(ft, M)
            elif bspec[0] == 'q': b = 'vq(f.q)'
            else: b = 'vq(s.i)'
            if kindop == 'add': ex = f'vadd(a_, b_)'
            elif kindop == 'sub': ex = f'vsub(a_, b_)'
            elif kindop == 'mul': ex = f'vmul(a_, b_)'
            elif kindop == 'madd': ex = f'vadd(c_, vmul(a_, b_))'
            else: ex = f'vsub(c_, vmul(a_, b_))'
            o = ['{ // four lanes']
            o.append(f'    const I4 a_ = {a}; const I4 b_ = {b};')
            if kindop in ('madd', 'msub'): o.append(f'    const I4 c_ = {opd_acc(M)};')
            o.append(f'    [[maybe_unused]] const I4 r_ = fin<{M}>(f, {ex});')
            if to_acc: o.append(f'    st4m<{M}>(s.acc, r_);'); B.upper_res = ('acc', None, M)
            elif fd != 0: o.append(f'    st4m<{M}>(s.vf[{fd}], r_);'); B.upper_res = ('vf', fd, M)
            o.append('}')
            return o
        out = None
        if fn < 0x3c:
            bc = fn & 3
            if fn < 0x1c:
                grp = fn >> 2
                kindop = {0: 'add', 1: 'sub', 2: 'madd', 3: 'msub', 6: 'mul'}.get(grp)
                if kindop: out = build(kindop, ('bc', bc), False)
            else:
                t = {0x1c: ('mul', 'q'), 0x1e: ('mul', 'i'), 0x20: ('add', 'q'), 0x22: ('add', 'i'), 0x24: ('sub', 'q'), 0x26: ('sub', 'i'),
                     0x21: ('madd', 'q'), 0x23: ('madd', 'i'), 0x25: ('msub', 'q'), 0x27: ('msub', 'i'),
                     0x28: ('add', 'vec'), 0x2a: ('mul', 'vec'), 0x2c: ('sub', 'vec'), 0x29: ('madd', 'vec'), 0x2d: ('msub', 'vec')}
                if fn in t: out = build(t[fn][0], (t[fn][1],), False)
        else:
            op2 = (fd << 2) | (fn & 3)
            bc = op2 & 3
            if op2 < 0x10:
                out = build(['add', 'sub', 'madd', 'msub'][op2 >> 2], ('bc', bc), True)
            elif 0x18 <= op2 < 0x1c:
                out = build('mul', ('bc', bc), True)
            else:
                t = {0x1c: ('mul', 'q'), 0x1e: ('mul', 'i'), 0x20: ('add', 'q'), 0x22: ('add', 'i'), 0x24: ('sub', 'q'), 0x26: ('sub', 'i'),
                     0x21: ('madd', 'q'), 0x23: ('madd', 'i'), 0x25: ('msub', 'q'), 0x27: ('msub', 'i'),
                     0x28: ('add', 'vec'), 0x2a: ('mul', 'vec'), 0x2c: ('sub', 'vec'), 0x29: ('madd', 'vec'), 0x2d: ('msub', 'vec')}
                if op2 in t: out = build(t[op2][0], (t[op2][1],), True)
        if out is not None:
            return out, None, False
        B.upper_res = None
        uc, uw, nop = X.upper(u, pre, ldest)
        # ITOF's results are floats of integers: clean
        if fn >= 0x3c and 0x10 <= ((fd << 2) | (fn & 3)) < 0x14 and ft != 0 and ft != ldest:
            B.upper_res = ('vf', ft, M)
        return uc, uw, nop

    # ---- a block
    def gen_block(self, L0):
        B = Block()
        B.pc0 = L0
        B.j = 0
        B.lines = []
        B.ind = '        '
        B.win = {k: [f'{KIND[k][2]}{a}' for a in range(4)] for k in 'msc'}
        B.clean = {0: 15}
        B.acc_clean = 0
        B.kick = None
        B.halt_in = None
        B.upper_res = None
        out = B.lines
        out.append(f'    B{L0:03x}: {{')
        out.append('        @LIMIT@')
        pc = L0
        while True:
            if pc not in self.I:
                out += self.exit_code(B, B.j, f'0x{pc:x}')
                break
            ins = self.I[pc]
            if ins.br is not None:
                br = ins.br
                bj = B.j
                halted = self.instr(B, ins, None)
                if halted:  # (the E bit one instruction before the branch: the program ends here, the delay slot never runs)
                    out += self.exit_code(B, B.j, f'0x{pc + 1:x}', halted=True)
                    break
                ds = self.I.get(pc + 1)
                assert ds is not None and ds.br is None, f'branch delay slot at {pc:#x}'
                dj = B.j
                halted2 = self.instr(B, ds, None)
                n = B.j
                ds_pc = pc + 1
                fall = pc + 2
                k = br['kind']
                if halted2:  # the E bit (on the branch itself): the program ends after the delay slot
                    # the program ends after the delay slot: pc as the branch leaves it
                    if k in ('b', 'bal'): pce = f'0x{br["target"]:x}'
                    elif k == 'bc': pce = f'(tk ? 0x{br["target"]:x}u : 0x{fall:x}u)'
                    else: pce = 'jt'
                    out += self.exit_code(B, n, pce, halted=True)
                    break
                if k in ('b', 'bal'):
                    out += self.edge(B, br['target'], n, ds_pc, True)
                elif k == 'bc':
                    out.append(f'        if (tk)')
                    out += self.edge(B, br['target'], n, ds_pc, True)
                    out += self.edge(B, fall, n)
                else:
                    ind = B.ind
                    if self.stop_back:
                        out.append(f'{ind}if (jt < 0x{ds_pc:x}u)')
                        out += [f'{ind}{{'] + self.exit_code(B, n, 'jt', back=True) + [f'{ind}}}']
                    out.append(f'{ind}switch (jt) {{')
                    for lk in self.links:
                        out.append(f'{ind}case 0x{lk:x}:')
                        out += self.edge(B, lk, n)
                    out.append(f'{ind}default:')
                    out += [f'{ind}{{'] + self.exit_code(B, n, 'jt') + [f'{ind}}}']
                    out.append(f'{ind}}}')
                break
            halted = self.instr(B, ins, None)
            pc += 1
            if halted:
                out += self.exit_code(B, B.j, f'0x{pc:x}', halted=True)
                break
            if B.halt_in is None and B.kick is None and pc in self.leaders:
                out += self.edge(B, pc, B.j)
                break
        out.append('    }')
        # the instruction budget (VuPipe::issue_limit): a block that would run past it is left to the exact step
        # function, which stops exactly where the caller does
        n_all = B.j
        chk = [f'if (iss + {n_all} > p.issue_limit) {{']
        B0 = Block()
        B0.ind, B0.kick, B0.halt_in = '            ', None, None
        B0.win = {k: [f'{KIND[k][2]}{a}' for a in range(4)] for k in 'msc'}
        ex = self.state_stores(B0, 0, f'0x{L0:03x}', False, False)  # (the state as the block found it, then one exact step)
        chk += ex + ['            if (iss >= p.issue_limit) return p.pc < p.size;', f'            return {self.exact}(s, p);', '        }']
        i = out.index('        @LIMIT@')
        out[i:i + 1] = ['        ' + chk[0]] + chk[1:]
        return out

    def compute_leaders(self):
        ld = set(self.entries)
        for pc, i in self.I.items():
            if i.br is not None:
                if self.first <= i.br['target'] <= self.last and i.br['kind'] in ('b', 'bal', 'bc'): ld.add(i.br['target'])
                if pc + 2 <= self.last: ld.add(pc + 2)
        self.leaders = ld

    def run(self):
        self.compute_leaders()
        blocks = {}
        while self.need:
            L0 = self.need.pop(0)
            if L0 in self.done: continue
            self.done[L0] = True
            blocks[L0] = self.gen_block(L0)
        self.stats['blocks'] = len(blocks)
        o = []
        o.append(f'// orig: (vu) micro {self.first:#05x}..{self.last:#05x}, fast form -- generated by tools/codegen/vu2cpp.py --fast, see tools/codegen/vu2cpp_fast.py')
        o.append(f'bool {self.name}(VUState& s, VuPipe& p) {{')
        o.append('    using namespace vun;')
        o.append('    p.back_jump = false;')
        o.append(f'    if (p.branch_pending || p.kick_delay != 0 || p.halt_pending) return {self.exact}(s, p);')
        o.append('    switch (p.pc) {')
        for L0 in sorted(blocks):
            o.append(f'    case 0x{L0:03x}: break;')
        o.append(f'    default: return {self.exact}(s, p);')
        o.append('    }')
        o.append('    FlagRegs f;')
        o.append('    f.macflag = s.macflag; f.statusflag = s.statusflag; f.clipflag = s.clipflag; f.q = s.q;')
        o.append('    uint64_t iss = p.issued;')
        if X.HAZARD: o.append('    int hzr = p.vi_hz_reg; int16_t hzo = p.vi_hz_old; (void)hzr; (void)hzo;')
        if self.has_q:
            o.append('    bool qp = p.q_pending; float qn = p.q_next; uint64_t qr = p.q_ready;')
        o += self.win_load()
        o.append('    switch (p.pc) {')
        for L0 in sorted(blocks):
            o.append(f'    case 0x{L0:03x}: goto B{L0:03x};')
        o.append('    default: break;')
        o.append('    }')
        o.append(f'    return {self.exact}(s, p);')
        for L0 in sorted(blocks):
            o += blocks[L0]
        o.append('}')
        return '\n'.join(o)


def gen_fast(mem, first, last, name, entries, exact, stop_back, size=512, debug_stop=False, mutate=None):
    global MUTATE
    MUTATE = mutate
    g = Gen(mem, first, last, name, entries, exact, stop_back, size)
    g.debug_stop = debug_stop
    code = g.run()
    sys.stderr.write(f'[vu2cpp --fast] {name}: {g.stats["blocks"]} blocks, {g.stats["handoff_edges"]} hand-off edges, reads {g.reads}\n')
    return code
