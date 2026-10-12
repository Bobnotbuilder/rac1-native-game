#!/usr/bin/env python3
# Regenerates the moby pass's ported VU programs: src/game/ovl/moby_vu0.cpp / moby_vu1.cpp (the exact step functions,
# tools/codegen/vu2cpp.py) and moby_vu0_fast.cpp / moby_vu1_fast.cpp (their fast form, tools/codegen/vu2cpp_fast.py):
# gen_moby_vu_fast.py [<vu0_micro.bin> <vu1_micro.bin>] [--elf ELF] [--out DIR] [--mutate latency|qready] [--debug-file ]
# The micro memories: by default read from the boot ELF (assets/dumps/SCUS_971.99: the VIF packets that MPG them, tools/codegen/vu_micro.py,
#), or given as files (vu0_micro.bin: the VU0 micro memory, 4 KB, dumped by the
# development repository's oracle; vu1_micro.bin: the VU1 micro memory, 16 KB, the tail of an assets/dumps/vu1_moby/*/*.bin capture).
# The run and registration code around the ports is hand-written: moby_vu0_reg.cpp, moby_vu1_run.cpp.
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vu2cpp
import vu2cpp_fast
import vu_micro

HEAD = '''// Ported {what} micro code, fast form (tools/codegen/gen_moby_vu_fast.py: tools/codegen/vu2cpp.py --fast): the same
// programs as {exact}, as basic blocks in one function, the flags kept only where a later
// instruction (or the caller) can read them, the float ops on four lanes at once (runtime/vu/
// vu_native_fast.h). Observably identical to the exact port and to the VU interpreter (
// the state compared where it can be seen -- block exits, kicks, the program's end); the exact one
// stays for debugging (RAC1_VU_EXACT=1).
#include <game/ovl/{hdr}.h>

#include <runtime/vu/vu_native.h>
#include <runtime/vu/vu_native_fast.h>

namespace rac1 {{

'''


HEAD_EXACT0 = '''// Ported VU0 micro code (tools/codegen/vu2cpp.py, instruction by instruction): the programs the
// moby pass runs beside the EE -- the skinning set (micro 0x000..0x147: the vertex program 0x000
// of 0x1ee858, the per-vertex entries 0x111 / 0x121 / 0x12d of its second half) and the set the
// overlay's main sends (0x190..0x1ed: the drop shadows' 0x190 / 0x192). Each runs the instruction
// at the pipeline's pc exactly as the VU interpreter does (runtime/vu/vu_native.h), resumable at
// any address; proved against the interpreter on the uploaded original.
// (tools/codegen/gen_moby_vu_fast.py; the registration: moby_vu0_reg.cpp)
#include <game/ovl/moby_vu0.h>

#include <runtime/vu/vu_native.h>

namespace rac1 {

'''

HEAD_EXACT1 = '''// Ported VU1 micro code (tools/codegen/vu2cpp.py, instruction by instruction): the moby programs the
// moby pass's chain starts (MPG at 0x10faa0, lhu 0x10fa90 quadwords: micro 0x000..0x507, the
// vertex transform / lighting / clipping / GIF packet build of each moby submesh). Each step runs
// the instruction at the pipeline's pc exactly as the VU interpreter does (runtime/vu/
// vu_native.h); proved against the interpreter on the uploaded original.
// (tools/codegen/gen_moby_vu_fast.py; the run and registration: moby_vu1_run.cpp)
#include <game/ovl/moby_vu1.h>

#include <runtime/vu/vu_native.h>

namespace rac1 {

'''


def write(path, text):
    import gen_common
    gen_common.write_if_changed(path, text)


def main():
    vu2cpp.HAZARD = vu2cpp.JR_MASK = False  # (the moby ports predate the integer branch hazard / JR mask: see vu2cpp.HAZARD)
    pos = [a for a in sys.argv[1:] if not a.startswith('--')]
    for k in ('--elf', '--out', '--mutate', '--debug-file'):
        if k in sys.argv:
            pos.remove(sys.argv[sys.argv.index(k) + 1])
    if len(pos) >= 2:
        vu0 = open(pos[0], 'rb').read()
        vu1 = open(pos[1], 'rb').read()
    else:
        elf = vu_micro.Elf(sys.argv[sys.argv.index('--elf') + 1] if '--elf' in sys.argv else vu_micro.ELF_DEFAULT)
        vu0, vu1 = vu_micro.moby_vu0(elf), vu_micro.moby_vu1(elf)
    out = sys.argv[sys.argv.index('--out') + 1] if '--out' in sys.argv else pos[2] if len(pos) > 2 else 'src/game/ovl'
    e0 = HEAD_EXACT0 + '// orig: 0x00100090 (micro 0x000..0x0ff, MPG 0x10008c; 0x100..0x147 from MPG 0x100894)\n'
    e0 += vu2cpp.gen(vu0, 0x000, 0x147, 'moby_vu0_skin_step') + '\n\n\n'
    e0 += '// orig: 0x0010e4d0 (micro 0x190..0x1ed, MPG 0x10e4cc)\n'
    e0 += vu2cpp.gen(vu0, 0x190, 0x1ed, 'moby_vu0_shadow_step') + '\n\n}  // namespace rac1\n'
    write(os.path.join(out, 'moby_vu0.cpp'), e0)
    e1 = HEAD_EXACT1 + '// orig: 0x0010faa8 (micro 0x000..0x507, the MPGs of the chain at 0x10fa90)\n'
    e1 += vu2cpp.gen(vu1, 0x000, 0x507, 'moby_vu1_step') + '\n\n}  // namespace rac1\n'
    write(os.path.join(out, 'moby_vu1.cpp'), e1)
    mutate = sys.argv[sys.argv.index('--mutate') + 1] if '--mutate' in sys.argv else None  # (testing the proof: see vu2cpp_fast.py)
    f0 = HEAD.format(what='VU0', exact='moby_vu0.cpp', hdr='moby_vu0')
    f0 += '// orig: 0x00100090 (micro 0x000..0x147: the skinning set)\n'
    f0 += vu2cpp_fast.gen_fast(vu0, 0x000, 0x147, 'moby_vu0_skin_fast', [0x000, 0x111, 0x121, 0x12d], 'moby_vu0_skin_step', True, 512, False, mutate) + '\n\n'
    f0 += '// orig: 0x0010e4d0 (micro 0x190..0x1ed: the drop shadows)\n'
    f0 += vu2cpp_fast.gen_fast(vu0, 0x190, 0x1ed, 'moby_vu0_shadow_fast', [0x190, 0x192], 'moby_vu0_shadow_step', True, 512, False, mutate) + '\n\n}  // namespace rac1\n'
    write(os.path.join(out, 'moby_vu0_fast.cpp'), f0)
    f1 = HEAD.format(what='VU1', exact='moby_vu1.cpp', hdr='moby_vu1')
    f1 += '// orig: 0x0010faa8 (micro 0x000..0x507: the moby programs)\n'
    f1 += vu2cpp_fast.gen_fast(vu1, 0x000, 0x507, 'moby_vu1_fast', [0x000, 0x00a, 0x00c, 0x00e, 0x014], 'moby_vu1_step', False, 2048, False, mutate) + '\n\n}  // namespace rac1\n'
    write(os.path.join(out, 'moby_vu1_fast.cpp'), f1)
    if '--debug-file' in sys.argv:
        # debugging the proof (not committed): the VU1 port with an exit at every block boundary, so that the oracle
        # (ORACLE_VU1_DBG=1) compares the state there and names the first block that goes wrong
        path = sys.argv[sys.argv.index('--debug-file') + 1]
        d = '#include <game/ovl/moby_vu1.h>\n#include <runtime/vu/vu_native.h>\n#include <runtime/vu/vu_native_fast.h>\nnamespace rac1 {\n'
        d += vu2cpp_fast.gen_fast(vu1, 0x000, 0x507, 'moby_vu1_fast_dbg', [0x000, 0x00a, 0x00c, 0x00e, 0x014], 'moby_vu1_step', False, 2048, True) + '\n}\n'
        open(path, 'w').write(d)


main()
