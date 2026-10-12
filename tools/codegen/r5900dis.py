#!/usr/bin/env python3
# usage: r5900dis.py <hex word> [<hex address>]
#
# The text of one R5900 (EE) instruction in the style of Ghidra's listing (the style the transliterations' comments were
# first written in: `li v1,0xff`, `mul.S f0,f0,f23`, `beq s0,zero,0x002f86d4`, `lq v1,0x0(a1)`, `vmulaw.xyzw ACC,vf2,vf1`):
# the EE core with its 64-bit and MMI ops, the FPU (COP1) and the VU0 in macro mode (COP2). Ours, no table from Ghidra or
# capstone: the generators' comments (tools/codegen/ovlist.py's listings) no longer depend on an outside disassembler or its
# version, and the R5900-only ops (lq / sq / MMI / COP2) print right (capstone decoded them as MIPS MSA / DSP ops).
# The text is only ever a comment: no generator reads its code from it.
import sys

GPR = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
       's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7', 't8', 't9', 'k0', 'k1', 'gp', 'sp', 's8', 'ra']
FIELD = 'xyzw'


def _h(v):
    """A signed immediate as Ghidra prints it: 0x.. / -0x.."""
    return '-0x%x' % -v if v < 0 else '0x%x' % v


def _sx16(w):
    v = w & 0xffff
    return v - 0x10000 if v & 0x8000 else v


def _dest(w):
    d = (w >> 21) & 15
    return ''.join(c for k, c in enumerate(FIELD) if d & (8 >> k))


def _mem(w):
    return '%s,%s(%s)' % (GPR[(w >> 16) & 31], _h(_sx16(w)), GPR[(w >> 21) & 31])


def _special(w, a):
    rs, rt, rd, sa, fn = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
    R = GPR
    if w == 0:
        return 'nop', ''
    shifts = {0x00: 'sll', 0x02: 'srl', 0x03: 'sra', 0x38: 'dsll', 0x3a: 'dsrl', 0x3b: 'dsra', 0x3c: 'dsll32', 0x3e: 'dsrl32', 0x3f: 'dsra32'}
    if fn in shifts and rs == 0:
        return shifts[fn], '%s,%s,0x%x' % (R[rd], R[rt], sa)
    vshifts = {0x04: 'sllv', 0x06: 'srlv', 0x07: 'srav', 0x14: 'dsllv', 0x16: 'dsrlv', 0x17: 'dsrav'}
    if fn in vshifts and sa == 0:
        return vshifts[fn], '%s,%s,%s' % (R[rd], R[rt], R[rs])
    if fn == 0x08:
        return 'jr', R[rs]
    if fn == 0x09:
        return 'jalr', R[rs] if rd == 31 else '%s,%s' % (R[rd], R[rs])
    if fn in (0x0a, 0x0b):
        return ('movz', 'movn')[fn - 0x0a], '%s,%s,%s' % (R[rd], R[rs], R[rt])
    if fn == 0x0c:
        return 'syscall', '' if (w >> 6) & 0xfffff == 0 else '0x%x' % ((w >> 6) & 0xfffff)
    if fn == 0x0d:
        return 'break', '0x%x' % ((w >> 6) & 0xfffff)
    if fn == 0x0f:
        return 'sync' + ('.p' if sa & 0x10 else ''), ''
    if fn in (0x10, 0x12):
        return ('mfhi', 'mflo')[fn == 0x12], R[rd]
    if fn in (0x11, 0x13):
        return ('mthi', 'mtlo')[fn == 0x13], R[rs]
    if fn == 0x28:
        return 'mfsa', R[rd]
    if fn == 0x29:
        return 'mtsa', R[rs]
    if fn in (0x18, 0x19):
        mn = ('mult', 'multu')[fn - 0x18]
        return mn, ('%s,%s,%s' % (R[rd], R[rs], R[rt])) if rd else '%s,%s' % (R[rs], R[rt])
    if fn in (0x1a, 0x1b):
        return ('div', 'divu')[fn - 0x1a], '%s,%s' % (R[rs], R[rt])
    three = {0x20: 'add', 0x21: 'addu', 0x22: 'sub', 0x23: 'subu', 0x24: 'and', 0x25: 'or', 0x26: 'xor', 0x27: 'nor',
             0x2a: 'slt', 0x2b: 'sltu', 0x2c: 'dadd', 0x2d: 'daddu', 0x2e: 'dsub', 0x2f: 'dsubu'}
    if fn in three:
        mn = three[fn]
        if mn in ('addu', 'daddu', 'or') and rt == 0:
            return 'move', '%s,%s' % (R[rd], R[rs])
        if mn == 'subu' and rs == 0 and False:
            return 'negu', '%s,%s' % (R[rd], R[rt])
        return mn, '%s,%s,%s' % (R[rd], R[rs], R[rt])
    return None


def _regimm(w, a):
    rs, rt = (w >> 21) & 31, (w >> 16) & 31
    t = (a + 4 + 4 * _sx16(w)) & 0xffffffff
    br = {0x00: 'bltz', 0x01: 'bgez', 0x02: 'bltzl', 0x03: 'bgezl', 0x10: 'bltzal', 0x11: 'bgezal', 0x12: 'bltzall', 0x13: 'bgezall'}
    if rt in br:
        if rt == 0x11 and rs == 0:
            return 'bal', '0x%08x' % t
        return br[rt], '%s,0x%08x' % (GPR[rs], t)
    tr = {0x08: 'tgei', 0x09: 'tgeiu', 0x0a: 'tlti', 0x0b: 'tltiu', 0x0c: 'teqi', 0x0e: 'tnei'}
    if rt in tr:
        return tr[rt], '%s,%s' % (GPR[rs], _h(_sx16(w)))
    if rt == 0x18:
        return 'mtsab', '%s,%s' % (GPR[rs], _h(_sx16(w)))
    if rt == 0x19:
        return 'mtsah', '%s,%s' % (GPR[rs], _h(_sx16(w)))
    return None


MMI0 = ['paddw', 'psubw', 'pcgtw', 'pmaxw', 'paddh', 'psubh', 'pcgth', 'pmaxh', 'paddb', 'psubb', 'pcgtb', None, None, None, None, None,
        'paddsw', 'psubsw', 'pextlw', 'ppacw', 'paddsh', 'psubsh', 'pextlh', 'ppach', 'paddsb', 'psubsb', 'pextlb', 'ppacb', None, None, 'pext5', 'ppac5']
MMI1 = [None, 'pabsw', 'pceqw', 'pminw', 'padsbh', 'pabsh', 'pceqh', 'pminh', None, None, 'pceqb', None, None, None, None, None,
        'padduw', 'psubuw', 'pextuw', None, 'padduh', 'psubuh', 'pextuh', None, 'paddub', 'psubub', 'pextub', 'qfsrv', None, None, None, None]
MMI2 = ['pmaddw', None, 'psllvw', 'psrlvw', 'pmsubw', None, None, None, 'pmfhi', 'pmflo', 'pinth', None, 'pmultw', 'pdivw', 'pcpyld', None,
        'pmaddh', 'phmadh', 'pand', 'pxor', 'pmsubh', 'phmsbh', None, None, None, None, 'pexeh', 'prevh', 'pmulth', 'pdivbw', 'pexew', 'prot3w']
MMI3 = ['pmadduw', None, None, 'psravw', None, None, None, None, 'pmthi', 'pmtlo', 'pinteh', None, 'pmultuw', 'pdivuw', 'pcpyud', None,
        None, None, 'por', 'pnor', None, None, None, None, None, None, 'pexch', 'pcpyh', None, None, 'pexcw', None]


def _mmi(w, a):
    rs, rt, rd, sa, fn = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
    R = GPR
    if fn in (0x00, 0x01, 0x20, 0x21):  # madd / maddu / madd1 / maddu1
        mn = {0x00: 'madd', 0x01: 'maddu', 0x20: 'madd1', 0x21: 'maddu1'}[fn]
        return mn, ('%s,%s,%s' % (R[rd], R[rs], R[rt])) if rd else '%s,%s' % (R[rs], R[rt])
    if fn == 0x04:
        return 'plzcw', '%s,%s' % (R[rd], R[rs])
    if fn in (0x10, 0x12, 0x30, 0x32):
        return {0x10: 'mfhi1', 0x12: 'mflo1', 0x30: 'pmfhl', 0x32: 'pmthl'}[fn] + ('.%s' % ['lw', 'uw', 'slw', 'lh', 'sh'][sa] if fn in (0x30, 0x32) and sa < 5 else ''), R[rd] if fn != 0x32 else R[rs]
    if fn in (0x11, 0x13):
        return ('mthi1', 'mtlo1')[fn == 0x13], R[rs]
    if fn in (0x18, 0x19):
        mn = ('mult1', 'multu1')[fn - 0x18]
        return mn, ('%s,%s,%s' % (R[rd], R[rs], R[rt])) if rd else '%s,%s' % (R[rs], R[rt])
    if fn in (0x1a, 0x1b):
        return ('div1', 'divu1')[fn - 0x1a], '%s,%s' % (R[rs], R[rt])
    if fn in (0x34, 0x36, 0x37, 0x3c, 0x3e, 0x3f):
        return {0x34: 'psllh', 0x36: 'psrlh', 0x37: 'psrah', 0x3c: 'psllw', 0x3e: 'psrlw', 0x3f: 'psraw'}[fn], '%s,%s,0x%x' % (R[rd], R[rt], sa)
    tab = {0x08: MMI0, 0x28: MMI1, 0x09: MMI2, 0x29: MMI3}.get(fn)
    if tab is None or tab[sa] is None:
        return None
    mn = tab[sa]
    if mn == 'por' and rs == 0 and rt == 0:
        return 'clear', R[rd]
    if mn in ('pmfhi', 'pmflo'):
        return mn, R[rd]
    if mn in ('pmthi', 'pmtlo'):
        return mn, R[rs]
    if mn in ('pabsw', 'pabsh', 'pext5', 'ppac5', 'prevh', 'pexeh', 'pexew', 'prot3w', 'pexch', 'pexcw', 'pcpyh'):
        return mn, '%s,%s' % (R[rd], R[rt])
    if mn in ('pdivw', 'pdivuw', 'pdivbw'):
        return mn, '%s,%s' % (R[rs], R[rt])
    if mn in ('psllvw', 'psrlvw', 'psravw'):
        return mn, '%s,%s,%s' % (R[rd], R[rt], R[rs])
    return mn, '%s,%s,%s' % (R[rd], R[rs], R[rt])


def _cop1(w, a):
    rs, ft, fs, fd, fn = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, w & 63
    rt = ft
    if rs == 0:
        return 'mfc1', '%s,f%d' % (GPR[rt], fs)
    if rs == 2:
        return 'cfc1', '%s,fcr%d' % (GPR[rt], fs)
    if rs == 4:
        return ('clear', 'f%d' % fs) if rt == 0 else ('mtc1', '%s,f%d' % (GPR[rt], fs))
    if rs == 6:
        return 'ctc1', '%s,fcr%d' % (GPR[rt], fs)
    if rs == 8:
        t = (a + 4 + 4 * _sx16(w)) & 0xffffffff
        return ['bc1f', 'bc1t', 'bc1fl', 'bc1tl'][rt & 3], '0x%08x' % t
    if rs == 0x14 and fn == 0x20:
        return 'cvt.s.W', 'f%d,f%d' % (fd, fs)
    if rs != 0x10:
        return None
    S = '.S'
    three = {0x00: 'add', 0x01: 'sub', 0x02: 'mul', 0x03: 'div', 0x28: 'max', 0x29: 'min'}
    if fn in three:
        return three[fn] + S, 'f%d,f%d,f%d' % (fd, fs, ft)
    two = {0x04: 'sqrt', 0x05: 'abs', 0x06: 'mov', 0x07: 'neg'}
    if fn in two:
        return two[fn] + S, 'f%d,f%d' % (fd, ft if fn == 0x04 else fs)
    if fn == 0x16:
        return 'rsqrt' + S, 'f%d,f%d,f%d' % (fd, fs, ft)
    acc = {0x18: 'adda', 0x19: 'suba', 0x1a: 'mula', 0x1e: 'madda', 0x1f: 'msuba'}
    if fn in acc:
        return acc[fn] + S, 'f%d,f%d' % (fs, ft)
    if fn in (0x1c, 0x1d):
        return ('madd', 'msub')[fn - 0x1c] + S, 'f%d,f%d,f%d' % (fd, fs, ft)
    if fn == 0x24:
        return 'cvt.w.S', 'f%d,f%d' % (fd, fs)
    cmp_ = {0x30: 'c.f', 0x32: 'c.eq', 0x34: 'c.lt', 0x36: 'c.le'}
    if fn in cmp_:
        return cmp_[fn] + S, 'f%d,f%d' % (fs, ft)
    return None


def _vu_ctrl(r):
    return 'vi%d' % r if r < 16 else 'vc%d' % (r - 16)


# VU0 macro ops (COP2, bit 25): the lower 6 bits (special1) and, for 0x3c..0x3f, the 11-bit special2 opcode
V1 = {0x28: 'vadd', 0x29: 'vmadd', 0x2a: 'vmul', 0x2b: 'vmax', 0x2c: 'vsub', 0x2d: 'vmsub', 0x2e: 'vopmsub', 0x2f: 'vmini',
      0x20: 'vaddq', 0x21: 'vmaddq', 0x22: 'vaddi', 0x23: 'vmaddi', 0x24: 'vsubq', 0x25: 'vmsubq', 0x26: 'vsubi', 0x27: 'vmsubi',
      0x1c: 'vmulq', 0x1d: 'vmaxi', 0x1e: 'vmuli', 0x1f: 'vminii'}
V1_INT = {0x30: 'viadd', 0x31: 'visub', 0x32: 'viaddi', 0x34: 'viand', 0x35: 'vior'}
V2 = {0x28: 'vadda', 0x29: 'vmadda', 0x2a: 'vmula', 0x2c: 'vsuba', 0x2d: 'vmsuba', 0x2e: 'vopmula', 0x2f: 'vnop',
      0x20: 'vaddaq', 0x21: 'vmaddaq', 0x22: 'vaddai', 0x23: 'vmaddai', 0x24: 'vsubaq', 0x25: 'vmsubaq', 0x26: 'vsubai', 0x27: 'vmsubai',
      0x1c: 'vmulaq', 0x1d: 'vabs', 0x1e: 'vmulai', 0x1f: 'vclipw',
      0x10: 'vitof0', 0x11: 'vitof4', 0x12: 'vitof12', 0x13: 'vitof15', 0x14: 'vftoi0', 0x15: 'vftoi4', 0x16: 'vftoi12', 0x17: 'vftoi15',
      0x30: 'vmove', 0x31: 'vmr32', 0x34: 'vlqi', 0x35: 'vsqi', 0x36: 'vlqd', 0x37: 'vsqd',
      0x38: 'vdiv', 0x39: 'vsqrt', 0x3a: 'vrsqrt', 0x3b: 'vwaitq', 0x3c: 'vmtir', 0x3d: 'vmfir', 0x3e: 'vilwr', 0x3f: 'viswr',
      0x40: 'vrnext', 0x41: 'vrget', 0x42: 'vrinit', 0x43: 'vrxor'}


def _vu(w):
    dest = _dest(w)
    ft, fs, fd = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31
    fn = w & 63
    bc = FIELD[w & 3]
    if fn < 0x3c:
        if fn < 0x1c:  # the broadcast forms: vadd<bc>, vsub<bc>, vmadd<bc>, vmsub<bc>, vmax<bc>, vmini<bc>, vmul<bc>
            base = ['vadd', 'vsub', 'vmadd', 'vmsub', 'vmax', 'vmini', 'vmul'][fn >> 2]
            return '%s%s.%s' % (base, bc, dest), 'vf%d,vf%d,vf%d' % (fd, fs, ft)
        if fn in V1_INT:
            if fn == 0x32:
                imm = (w >> 6) & 31
                imm = imm - 32 if imm & 16 else imm
                return 'viaddi', 'vi%d,vi%d,%s' % (ft, fs, _h(imm))
            return V1_INT[fn], 'vi%d,vi%d,vi%d' % (fd, fs, ft)
        if fn == 0x38:
            return 'vcallms', '0x%x' % ((w >> 6) & 0x7fff)
        if fn == 0x39:
            return 'vcallmsr', 'vi27'
        if fn in V1:
            mn = V1[fn]
            last = 'Q' if mn.endswith('q') else 'I' if mn.endswith('i') and mn != 'vmini' else 'vf%d' % ft
            if mn == 'vminii':
                mn, last = 'vminii', 'I'
            return '%s.%s' % (mn, dest), 'vf%d,vf%d,%s' % (fd, fs, last)
        return None
    op2 = ((w >> 4) & 0x7c) | (w & 3)
    if op2 < 0x10:  # the broadcast accumulator forms: vadda<bc>, vsuba<bc>, vmadda<bc>, vmsuba<bc>, vitof/vftoi (0x10..), vmula<bc>, ...
        base = ['vadda', 'vsuba', 'vmadda', 'vmsuba'][op2 >> 2]
        return '%s%s.%s' % (base, bc, dest), 'ACC,vf%d,vf%d' % (fs, ft)
    if 0x18 <= op2 < 0x1c:
        return 'vmula%s.%s' % (bc, dest), 'ACC,vf%d,vf%d' % (fs, ft)
    if op2 not in V2:
        return None
    mn = V2[op2]
    if mn in ('vitof0', 'vitof4', 'vitof12', 'vitof15', 'vftoi0', 'vftoi4', 'vftoi12', 'vftoi15', 'vmove', 'vmr32', 'vabs'):
        return '%s.%s' % (mn, dest), 'vf%d,vf%d' % (ft, fs)
    if mn == 'vclipw':
        return 'vclipw.%s' % dest, 'vf%d,vf%d' % (fs, ft)
    if mn == 'vnop':
        return 'vnop', ''
    if mn == 'vwaitq':
        return 'vwaitq', ''
    if mn in ('vaddaq', 'vmaddaq', 'vsubaq', 'vmsubaq', 'vmulaq'):
        return '%s.%s' % (mn, dest), 'ACC,vf%d,Q' % fs
    if mn in ('vaddai', 'vmaddai', 'vsubai', 'vmsubai', 'vmulai'):
        return '%s.%s' % (mn, dest), 'ACC,vf%d,I' % fs
    if mn in ('vadda', 'vmadda', 'vmula', 'vsuba', 'vmsuba', 'vopmula'):
        return '%s.%s' % (mn, dest), 'ACC,vf%d,vf%d' % (fs, ft)
    fsf, ftf = FIELD[(w >> 21) & 3], FIELD[(w >> 23) & 3]
    if mn == 'vdiv':
        return 'vdiv', 'Q,vf%d%s,vf%d%s' % (fs, fsf, ft, ftf)
    if mn == 'vsqrt':
        return 'vsqrt', 'Q,vf%d%s' % (ft, ftf)
    if mn == 'vrsqrt':
        return 'vrsqrt', 'Q,vf%d,vf%d' % (fs, ft)
    if mn in ('vlqi', 'vlqd'):
        return '%s.%s' % (mn, dest), ('vf%d (vi%d++)' % (ft, fs)) if mn == 'vlqi' else 'vf%d (--vi%d)' % (ft, fs)
    if mn in ('vsqi', 'vsqd'):
        return '%s.%s' % (mn, dest), ('vf%d (vi%d++)' % (fs, ft)) if mn == 'vsqi' else 'vf%d (--vi%d)' % (fs, ft)
    if mn == 'vmtir':
        return 'vmtir', 'vi%d,vf%d%s' % (ft, fs, fsf)
    if mn == 'vmfir':
        return 'vmfir.%s' % dest, 'vf%d,vi%d' % (ft, fs)
    if mn in ('vilwr', 'viswr'):
        return '%s.%s' % (mn, dest), 'vi%d,(vi%d)' % (ft, fs)
    if mn in ('vrnext', 'vrget'):
        return '%s.%s' % (mn, dest), 'vf%d,R' % ft
    if mn in ('vrinit', 'vrxor'):
        return mn, 'R,vf%d%s' % (fs, fsf)
    return None


def _cop2(w, a):
    rs, rt, rd = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    if w & (1 << 25):
        return _vu(w)
    il = '.I' if w & 1 else ''
    if rs == 1:
        return 'qmfc2' + il, '%s,vf%d' % (GPR[rt], rd)
    if rs == 2:
        return 'cfc2' + il, '%s,%s' % (GPR[rt], _vu_ctrl(rd))
    if rs == 5:
        return 'qmtc2' + il, '%s,vf%d' % (GPR[rt], rd)
    if rs == 6:
        return 'ctc2' + il, '%s,%s' % (GPR[rt], _vu_ctrl(rd))
    if rs == 8:
        t = (a + 4 + 4 * _sx16(w)) & 0xffffffff
        return ['bc2f', 'bc2t', 'bc2fl', 'bc2tl'][rt & 3], '0x%08x' % t
    return None


def _cop0(w, a):
    rs, rt, rd = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    if rs == 0:
        return 'mfc0', '%s,cop0_%d' % (GPR[rt], rd)
    if rs == 4:
        return 'mtc0', '%s,cop0_%d' % (GPR[rt], rd)
    if rs == 0x10:
        return {0x18: ('eret', ''), 0x38: ('ei', ''), 0x39: ('di', ''), 0x02: ('tlbwi', ''), 0x06: ('tlbwr', ''), 0x08: ('tlbp', ''), 0x01: ('tlbr', '')}.get(w & 63)
    return None


def text(w, a=0):
    """(mnemonic, operands) of the instruction word w at address a; ('.word', '0x........') when it is not one."""
    op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
    R = GPR
    r = None
    if op == 0:
        r = _special(w, a)
    elif op == 1:
        r = _regimm(w, a)
    elif op in (2, 3):
        r = ('j', 'jal')[op - 2], '0x%08x' % (((a + 4) & 0xf0000000) | ((w & 0x3ffffff) << 2))
    elif op in (4, 5, 0x14, 0x15):
        t = (a + 4 + 4 * _sx16(w)) & 0xffffffff
        mn = {4: 'beq', 5: 'bne', 0x14: 'beql', 0x15: 'bnel'}[op]
        r = ('b', '0x%08x' % t) if op == 4 and rs == 0 and rt == 0 else (mn, '%s,%s,0x%08x' % (R[rs], R[rt], t))
    elif op in (6, 7, 0x16, 0x17) and rt == 0:
        t = (a + 4 + 4 * _sx16(w)) & 0xffffffff
        r = {6: 'blez', 7: 'bgtz', 0x16: 'blezl', 0x17: 'bgtzl'}[op], '%s,0x%08x' % (R[rs], t)
    elif op in (8, 9, 0x18, 0x19):
        mn = {8: 'addi', 9: 'addiu', 0x18: 'daddi', 0x19: 'daddiu'}[op]
        r = ('li', '%s,%s' % (R[rt], _h(_sx16(w)))) if op == 9 and rs == 0 else (mn, '%s,%s,%s' % (R[rt], R[rs], _h(_sx16(w))))
    elif op in (0xa, 0xb):
        r = ('slti', 'sltiu')[op - 0xa], '%s,%s,%s' % (R[rt], R[rs], _h(_sx16(w)))
    elif op in (0xc, 0xd, 0xe):
        r = ('andi', 'ori', 'xori')[op - 0xc], '%s,%s,0x%x' % (R[rt], R[rs], w & 0xffff)
    elif op == 0xf:
        r = 'lui', '%s,0x%x' % (R[rt], w & 0xffff)
    elif op == 0x10:
        r = _cop0(w, a)
    elif op == 0x11:
        r = _cop1(w, a)
    elif op == 0x12:
        r = _cop2(w, a)
    elif op == 0x1c:
        r = _mmi(w, a)
    else:
        mem = {0x1a: 'ldl', 0x1b: 'ldr', 0x1e: 'lq', 0x1f: 'sq', 0x20: 'lb', 0x21: 'lh', 0x22: 'lwl', 0x23: 'lw', 0x24: 'lbu', 0x25: 'lhu',
               0x26: 'lwr', 0x27: 'lwu', 0x28: 'sb', 0x29: 'sh', 0x2a: 'swl', 0x2b: 'sw', 0x2c: 'sdl', 0x2d: 'sdr', 0x2e: 'swr',
               0x37: 'ld', 0x3f: 'sd'}
        if op in mem:
            r = mem[op], _mem(w)
        elif op in (0x31, 0x39):
            r = ('lwc1', 'swc1')[op == 0x39], 'f%d,%s(%s)' % (rt, _h(_sx16(w)), R[rs])
        elif op in (0x36, 0x3e):
            r = ('lqc2', 'sqc2')[op == 0x3e], 'vf%d,%s(%s)' % (rt, _h(_sx16(w)), R[rs])
        elif op == 0x2f:
            r = 'cache', '0x%x,%s(%s)' % (rt, _h(_sx16(w)), R[rs])
        elif op == 0x33:
            r = 'pref', '0x%x,%s(%s)' % (rt, _h(_sx16(w)), R[rs])
    if r is None:
        return '.word', '0x%08x' % w
    return r


# the first part (before any `.`) of every mnemonic text() prints: tools/codegen/gen_common.py strips the comments that start with one
MNEMONICS = frozenset('''
abs add adda addi addiu addu and andi b bal bc1f bc1fl bc1t bc1tl bc2f bc2fl bc2t bc2tl beq beql bgez bgezal bgezall bgezl bgtz
bgtzl blez blezl bltz bltzal bltzall bltzl bne bnel break c cache cfc1 cfc2 clear ctc1 ctc2 cvt dadd daddi daddiu daddu di div
div1 divu divu1 dsll dsll32 dsllv dsra dsra32 dsrav dsrl dsrl32 dsrlv dsub dsubu ei eret f j jal jalr jr lb lbu ld ldl ldr lh lhu
li lq lqc2 lui lw lwc1 lwl lwr lwu madd madd1 madda maddu maddu1 max mfc0 mfc1 mfhi mfhi1 mflo mflo1 mfsa min mov move movn movz
msub msuba mtc0 mtc1 mthi mthi1 mtlo mtlo1 mtsa mtsab mtsah mul mula mult mult1 multu multu1 neg negu nop nor or ori pabsh pabsw
paddb paddh paddsb paddsh paddsw paddub padduh padduw paddw padsbh pand pceqb pceqh pceqw pcgtb pcgth pcgtw pcpyh pcpyld pcpyud
pdivbw pdivuw pdivw pexch pexcw pexeh pexew pext5 pextlb pextlh pextlw pextub pextuh pextuw phmadh phmsbh pinteh pinth plzcw
pmaddh pmadduw pmaddw pmaxh pmaxw pmfhi pmfhl pmflo pminh pminw pmsubh pmsubw pmthi pmthl pmtlo pmulth pmultuw pmultw pnor por
ppac5 ppacb ppach ppacw pref prevh prot3w psllh psllvw psllw psrah psravw psraw psrlh psrlvw psrlw psubb psubh psubsb psubsh
psubsw psubub psubuh psubuw psubw pxor qfsrv qmfc2 qmtc2 rsqrt s sb sd sdl sdr sh sll sllv slt slti sltiu sltu sq sqc2 sqrt sra
srav srl srlv sub suba subu sw swc1 swl swr sync syscall teqi tgei tgeiu tlbp tlbr tlbwi tlbwr tlti tltiu tnei vabs vadd vadda
vaddai vaddaq vaddi vaddq vc vcallms vcallmsr vclipw vdiv vf vftoi0 vftoi12 vftoi15 vftoi4 vi viadd viaddi viand vilwr vior visub
viswr vitof0 vitof12 vitof15 vitof4 vlqd vlqi vmadd vmadda vmaddai vmaddaq vmaddi vmaddq vmax vmaxi vmfir vmini vminii vmove vmr32
vmsub vmsuba vmsubai vmsubaq vmsubi vmsubq vmtir vmul vmula vmulai vmulaq vmuli vmulq vnop vopmsub vopmula vrget vrinit vrnext
vrsqrt vrxor vsqd vsqi vsqrt vsub vsuba vsubai vsubaq vsubi vsubq vwaitq xor xori
'''.split())


if __name__ == '__main__':
    w = int(sys.argv[1], 16)
    a = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
    print('%s %s' % text(w, a))
