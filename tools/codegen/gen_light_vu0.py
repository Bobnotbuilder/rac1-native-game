#!/usr/bin/env python3
# Regenerates src/game/ovl/world_light_vu0.cpp (exact step function, tools/codegen/vu2cpp.py) and world_light_vu0_fast.cpp (the fast
# form, tools/codegen/vu2cpp_fast.py) for the VU0 micro programs of the world's static lighting: the set render_frame
# (0x1f85b8) and the level load (gameplay_load) send by VIF0 from the chain at 0x100ae0 (one MPG of 0xb0 instructions at micro
# address 0, its words at 0x100af0 in the ELF), started by the tie lighting 0x297728 (vcallms 0x58 / 0x84) and the shrub lighting
# 0x288908 (vcallms 0x000 / 0x02c).
#   gen_light_vu0.py [<input>] [outdir] [--mutate latency|qready]
# <input>: the set as rac1-native dumps it (RAC1_VU0_DUMP_SETS=<dir>: vu0set_000_0b0_<hash>.bin) or the ELF (default
# assets/dumps/SCUS_971.99, read at 0x100af0). The generated code is translated from the original microcode, as moby_vu0.cpp /
# world_vu1_*.cpp are (ADR 0005 / 0007); the input stays in assets/ (gitignored).
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vu2cpp
import vu2cpp_fast
import gen_common
import vu_micro

N = 0xb0
HASH = None  # printed by the run, written into the registration (world_light_vu0.cpp's tail)
SRC = 0x00100af0
ENTRIES = [0x000, 0x02c, 0x058, 0x084]


def fnv(words):
    h = 0xcbf29ce484222325
    for w in words:
        for b in range(8):
            h ^= (w >> (8 * b)) & 0xff
            h = (h * 0x100000001b3) & 0xffffffffffffffff
    return h


def from_elf(path):
    d = open(path, 'rb').read()
    phoff, = struct.unpack_from('<I', d, 0x1c)
    phnum, = struct.unpack_from('<H', d, 0x2c)
    for i in range(phnum):
        t, off, va, pa, fs, ms = struct.unpack_from('<6I', d, phoff + 32 * i)
        if t == 1 and va <= SRC and SRC + N * 8 <= va + fs:
            # the chain's tag (0x100ae0): END, qwc 0x58, VIF NOP + MPG 0xb0 at 0
            tag, = struct.unpack_from('<Q', d, off + 0x100ae0 - va)
            vif, = struct.unpack_from('<I', d, off + 0x100aec - va)
            assert tag & 0xffff == 0x58 and (tag >> 28) & 7 == 7 and vif == 0x4ab00000, 'unexpected VIF0 chain at 0x100ae0'
            return d[off + SRC - va:off + SRC - va + N * 8]
    sys.exit('0x100af0 not in the ELF\'s segments')


HEAD_EXACT = '''// Ported VU0 micro code (tools/codegen/gen_light_vu0.py: tools/codegen/vu2cpp.py, instruction by instruction): the world's static lighting
// set -- 0xb0 instructions MPG'd from the ELF's data at 0x00100af0 (VIF0 chain 0x100ae0, sent by render_frame and the level load)
// into micro memory 0x000..0x0af. Started by the tie lighting 0x297728 (0x058, 0x084) and the shrub lighting 0x288908
// (0x000, 0x02c). Each step runs the instruction at the pipeline's pc exactly as the VU interpreter does (runtime/vu/vu_native.h),
// resumable at any address; proved against the interpreter on the uploaded original.
#include <game/ovl/world_light_vu0.h>

#include <runtime/vu/vu0.h>
#include <runtime/vu/vu_native.h>

#include <cstdlib>

namespace rac1 {

// orig: 0x00100af0 (micro 0x000..0x0af: the tie / shrub static lighting)
'''

HEAD_FAST = '''// Ported VU0 micro code, fast form (tools/codegen/gen_light_vu0.py: tools/codegen/vu2cpp.py --fast): the same program as
// world_light_vu0.cpp, as basic blocks in one function, the flags kept only where a later instruction (or the caller) can read
// them, the float ops on four lanes at once (runtime/vu/vu_native_fast.h). Observably identical to the exact port and to the VU
// interpreter (the state compared where it can be seen -- block exits, the program's end); the exact one stays for
// debugging (RAC1_VU_EXACT=1).
#include <game/ovl/world_light_vu0.h>

#include <runtime/vu/vu_native.h>
#include <runtime/vu/vu_native_fast.h>

namespace rac1 {

// orig: 0x00100af0 (micro 0x000..0x0af: the tie / shrub static lighting)
'''

TAIL_EXACT = '''

// orig: (none) the port's registration with the runtime VU0 (the code hash of the uploaded set)
void world_light_vu0_register() {
    if (std::getenv("RAC1_VU0_LIGHT_INTERP") != nullptr) return;  // testing: the set on the VU interpreter, as before
    const bool exact = std::getenv("RAC1_VU_EXACT") != nullptr || std::getenv("RAC1_VU0_EXACT") != nullptr;  // testing: the exact step-by-step port
    vu0::register_program(0x000, 0x0b0, 0x%016xull, exact ? &world_light_vu0_step : &world_light_vu0_fast, "world static lighting");
}

}  // namespace rac1
'''


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    mutate = sys.argv[sys.argv.index('--mutate') + 1] if '--mutate' in sys.argv else None  # (testing the proof: see vu2cpp_fast.py)
    if mutate in args:
        args.remove(mutate)
    src = args[0] if args else vu_micro.ELF_DEFAULT
    out = args[1] if len(args) > 1 else 'src/game/ovl'
    raw = open(src, 'rb').read()
    if len(raw) != N * 8:
        raw = from_elf(src)
    words = list(struct.unpack('<%dQ' % N, raw))
    h = fnv(words)
    sys.stderr.write('[gen_light_vu0] 0x%x instructions, hash %016x\n' % (N, h))
    mem = raw + b'\0' * (4096 - len(raw))
    exact = HEAD_EXACT + vu2cpp.gen(mem, 0, N - 1, 'world_light_vu0_step') + (TAIL_EXACT % h)
    gen_common.write_if_changed(os.path.join(out, 'world_light_vu0.cpp'), exact)
    fast = HEAD_FAST + vu2cpp_fast.gen_fast(mem, 0, N - 1, 'world_light_vu0_fast', ENTRIES, 'world_light_vu0_step', True, 512, False, mutate) + \
        '\n\n}  // namespace rac1\n'
    gen_common.write_if_changed(os.path.join(out, 'world_light_vu0_fast.cpp'), fast)


main()
