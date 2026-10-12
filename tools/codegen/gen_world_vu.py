#!/usr/bin/env python3
# Regenerates src/game/ovl/world_vu1_<set>.cpp (exact step functions) and world_vu1_<set>_fast.cpp (the fast
# form, tools/codegen/vu2cpp_fast.py) for the world's VU1 micro programs: the tfrag, tie and shrub
# passes' sets, each uploaded by MPGs from the ELF's own data (0x103570 ...) into micro memory from address 0.
#   gen_world_vu.py [<setsdir> | --elf ELF] [outdir]
# The sets: by default read from the boot ELF (assets/dumps/SCUS_971.99: the MPGs from each set's address below, tools/codegen/vu_micro.py,
# checked against its hash), or from <setsdir>, the sets the game uploads, one file per distinct set (rac1-native with
# RAC1_VU1_DUMP_SETS=<dir>, a few hundred frames of Veldin's gameplay): set_<n instructions>_<fnv1a64>.bin.
# The generated code is translated from the original microcode, as moby_vu1.cpp / moby_vu1_fast.cpp are
# (ADR 0005 / 0007); the inputs stay in assets/ (gitignored).
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vu2cpp
import vu2cpp_fast
import vu_micro
import gen_common

# name, instructions, fnv1a64 of them, where the game's MPG chain takes them from, the starts (MSCAL addresses) seen in the running game
SETS = [
    ('tfrag_a', 0x74c, 0x7af1655d8332d13d, 0x00103570, [0x000, 0x006, 0x008, 0x00a, 0x00e, 0x010, 0x014]),
    ('tfrag_b', 0x3ba, 0x2a1b011291666a63, 0x00107020, [0x000, 0x002]),
    ('tie_a', 0x3d9, 0x52c8f765e4557bbe, 0x00108e20, [0x000, 0x004, 0x006, 0x008]),
    ('tie_b', 0x6ec, 0xdfa6c7d28920bedf, 0x0010ad20, [0x000, 0x004, 0x006, 0x008]),
    ('shrub_a', 0x16c, 0x617e427100e072d3, 0x00101760, [0x000, 0x011, 0x015, 0x063]),
    ('shrub_b', 0x24d, 0x202c265b05300479, 0x001022e0, [0x000, 0x011, 0x015, 0x063]),
]

HEAD_EXACT = '''// Ported VU1 micro code (tools/codegen/gen_world_vu.py: tools/codegen/vu2cpp.py, instruction by instruction): the world's
// {what} -- {n} instructions, MPG'd from the ELF's data at {src:#010x} into micro memory 0x000..{last:#05x} by the
// frame chain of the world pass. Each step runs the instruction at the pipeline's pc exactly as the VU
// interpreter does (runtime/vu/vu_native.h); proved against the interpreter on the uploaded original.
#include <game/ovl/world_vu1.h>

#include <runtime/vu/vu_native.h>

namespace rac1 {{

// orig: {src:#010x} (micro 0x000..{last:#05x}: {what})
'''

HEAD_FAST = '''// Ported VU1 micro code, fast form (tools/codegen/gen_world_vu.py: tools/codegen/vu2cpp.py --fast): the same program as
// world_vu1_{name}.cpp, as basic blocks in one function, the flags kept only where a later instruction (or the
// caller) can read them, the float ops on four lanes at once (runtime/vu/vu_native_fast.h). Observably identical to
// the exact port and to the VU interpreter (the state compared where it can be seen -- block exits,
// kicks, the program's end); the exact one stays for debugging (RAC1_VU_EXACT=1).
#include <game/ovl/world_vu1.h>

#include <runtime/vu/vu_native.h>
#include <runtime/vu/vu_native_fast.h>

namespace rac1 {{

// orig: {src:#010x} (micro 0x000..{last:#05x}: {what})
'''

WHAT = {'tfrag_a': 'the tfrag pass, program A', 'tfrag_b': 'the tfrag pass, program B', 'tie_a': 'the tie pass, program A', 'tie_b': 'the tie pass, program B',
        'shrub_a': 'the shrub pass, program A', 'shrub_b': 'the shrub pass, program B'}


def main():
    vu2cpp.TGT_MASK = 0x7ff  # (VU1's 2048-instruction micro memory: branch / JR targets wrap)
    if '--tgt-mask' in sys.argv:  # (testing the proof: no wrap)
        vu2cpp.TGT_MASK = int(sys.argv[sys.argv.index('--tgt-mask') + 1], 0)
    pos = [a for a in sys.argv[1:] if not a.startswith('--')]
    for k in ('--elf', '--mutate', '--only', '--tgt-mask'):
        if k in sys.argv:
            pos.remove(sys.argv[sys.argv.index(k) + 1])
    elf = None
    if pos and os.path.isdir(pos[0]) and glob.glob(os.path.join(pos[0], 'set_*.bin')):
        setsdir = pos.pop(0)
    else:
        setsdir = None
        elf = vu_micro.Elf(sys.argv[sys.argv.index('--elf') + 1] if '--elf' in sys.argv else vu_micro.ELF_DEFAULT)
    out = pos[0] if pos else 'src/game/ovl'
    mutate = sys.argv[sys.argv.index('--mutate') + 1] if '--mutate' in sys.argv else None  # (testing the proof: see vu2cpp_fast.py)
    only = sys.argv[sys.argv.index('--only') + 1] if '--only' in sys.argv else None
    for name, n, h, src, entries in SETS:
        if only is not None and name != only:
            continue
        if setsdir is None:
            mem = vu_micro.world(elf, src - 4, n, h)[:n * 8]  # (the MPG code right before the set's first instruction)
        else:
            files = glob.glob(os.path.join(setsdir, 'set_%04x_%016x.bin' % (n, h)))
            if not files:
                sys.exit('missing %s: run rac1-native with RAC1_VU1_DUMP_SETS' % ('set_%04x_%016x.bin' % (n, h)))
            mem = open(files[0], 'rb').read()
        assert len(mem) == n * 8
        mem += b'\0' * (16384 - len(mem))
        last = n - 1
        fmt = dict(name=name, n=n, last=last, src=src, what=WHAT[name])
        exact = HEAD_EXACT.format(**fmt) + vu2cpp.gen(mem, 0, last, 'world_%s_step' % name) + '\n\n}  // namespace rac1\n'
        gen_common.write_if_changed(os.path.join(out, 'world_vu1_%s.cpp' % name), exact)
        fast = HEAD_FAST.format(**fmt) + vu2cpp_fast.gen_fast(mem, 0, last, 'world_%s_fast' % name, entries, 'world_%s_step' % name, False, 2048, False, mutate) + '\n\n}  // namespace rac1\n'
        gen_common.write_if_changed(os.path.join(out, 'world_vu1_%s_fast.cpp' % name), fast)


main()
