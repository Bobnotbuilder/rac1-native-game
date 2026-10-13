#!/usr/bin/env python3
# usage: vu_micro.py <elf> moby_vu0 | moby_vu1 | world <name> | light [out.bin]
#
# The VU micro programs the game uploads, read from the boot ELF (assets/dumps/SCUS_971.99, setup.sh): the VIF
# packets that MPG them into micro memory are data of the ELF, so the code generators of the ported VU programs
# (gen_moby_vu_fast.py, gen_moby_vu.py, gen_world_vu.py, gen_light_vu0.py) need no capture of the running game:
#   moby_vu0   VU0 micro memory (4 KB): MPG 0x10008c (0x000..0x0ff), 0x100894 (0x100..0x147), 0x10e4cc (0x190..0x1ed)
#   moby_vu1   VU1 micro memory (16 KB): the VIF1 packet of the moby pass's chain, 0x10faa0 (its qword count at 0x10fa90): 0x000..0x507
#   world      one of the world passes' sets (tools/codegen/gen_world_vu.py SETS: the address of its first MPG; the set from micro 0)
#   light      the static lighting's VU0 set (MPG 0x100aec: 0x000..0x0af)
# Each image is checked against the fnv1a64 the ports register (the uploaded set's hash), so a wrong ELF fails here.
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'overlay_match'))
import region_view  # noqa: E402

# the disc's own boot ELF (the VU programs and their packets are below the boot module: the same bytes in every region's)
ELF_DEFAULT = region_view.boot_elf_path(os.environ.get('RAC1_ASSETS') or os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'assets'))


class Elf:
    def __init__(self, path):
        self.d = open(path, 'rb').read()
        d = self.d
        phoff, = struct.unpack_from('<I', d, 0x1c)
        phnum, = struct.unpack_from('<H', d, 0x2c)
        self.segs = []
        for i in range(phnum):
            t, off, va, pa, fs, ms = struct.unpack_from('<6I', d, phoff + 32 * i)
            if t == 1:
                self.segs.append((va, off, fs))

    def read(self, va, n):
        for sva, off, fs in self.segs:
            if sva <= va and va + n <= sva + fs:
                return self.d[off + va - sva:off + va - sva + n]
        raise ValueError('0x%x..0x%x not in the ELF' % (va, va + n))

    def word(self, va):
        return struct.unpack('<I', self.read(va, 4))[0]


def fnv(data):
    h = 0xcbf29ce484222325
    for b in data:
        h ^= b
        h = (h * 0x100000001b3) & 0xffffffffffffffff
    return h


def apply_mpg(elf, mem, code_va):
    """The MPG VIF code at code_va (cmd 0x4a, num, load address): its instructions into mem. Returns the address after them."""
    w = elf.word(code_va)
    assert (w >> 24) & 0x7f == 0x4a, 'no MPG at 0x%x (0x%08x)' % (code_va, w)
    num = ((w >> 16) & 0xff) or 256
    at = (w & 0xffff) * 8
    mem[at:at + num * 8] = elf.read(code_va + 4, num * 8)
    return code_va + 4 + num * 8


def vif_mpgs(elf, mem, va, end):
    """Walks a VIF packet from va to end, applying its MPGs (the other codes are skipped by their sizes)."""
    while va < end:
        w = elf.word(va)
        cmd, num, imm = (w >> 24) & 0x7f, (w >> 16) & 0xff, w & 0xffff
        if cmd == 0x4a:
            va = apply_mpg(elf, mem, va)
            continue
        va += 4
        if cmd == 0x20:  # STMASK
            va += 4
        elif cmd in (0x30, 0x31):  # STROW / STCOL
            va += 16
        elif cmd in (0x50, 0x51):  # DIRECT / DIRECTHL
            va = ((va + 15) & ~15) + 16 * (imm or 0x10000)
        elif cmd & 0x60 == 0x60:  # UNPACK
            vn, vl = (cmd >> 2) & 3, cmd & 3
            n = num or 256
            bits = (32 >> vl) * (vn + 1)
            va += ((bits * n + 31) // 32) * 4
        elif cmd not in (0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x13, 0x14, 0x15, 0x17):
            raise ValueError('VIF code 0x%02x at 0x%x' % (cmd, va - 4))
    return mem


# the registered fnv1a64 of each set (src/game/ovl/moby_vu0_host.cpp, moby_vu1_host.cpp, gen_world_vu.py SETS, world_light_vu0.cpp)
MOBY_VU0 = [(0x000, 0x148, 0x007ee420f0ef0448), (0x190, 0x05e, 0x115121753a51f441)]
MOBY_VU1 = (0x000, 0x508)


def moby_vu0(elf):
    mem = bytearray(4096)
    for va in (0x10008c, 0x100894, 0x10e4cc):
        apply_mpg(elf, mem, va)
    for first, n, h in MOBY_VU0:
        got = fnv(mem[first * 8:(first + n) * 8])
        assert got == h, 'moby VU0 set 0x%x: hash %016x, expected %016x' % (first, got, h)
    return bytes(mem)


def moby_vu1(elf):
    mem = bytearray(16384)
    qwc = struct.unpack('<H', elf.read(0x10fa90, 2))[0]
    vif_mpgs(elf, mem, 0x10faa0, 0x10faa0 + 16 * qwc)
    return bytes(mem)


def world(elf, first_mpg, n, h):
    """A world pass's set: the MPGs from first_mpg (consecutive packets of the ELF's data) until n instructions are loaded."""
    mem = bytearray(16384)
    va = first_mpg
    loaded = 0
    while loaded < n:
        w = elf.word(va)
        if (w >> 24) & 0x7f != 0x4a:
            va += 4  # (a VIF NOP / the DMA tag's words between the packets)
            continue
        num = ((w >> 16) & 0xff) or 256
        va = apply_mpg(elf, mem, va)
        loaded += num
    got = fnv(mem[:n * 8])
    assert loaded == n and got == h, 'world set at 0x%x: %d instructions, hash %016x, expected %d / %016x' % (first_mpg, loaded, got, n, h)
    return bytes(mem)


def main():
    a = sys.argv[1:]
    elf = Elf(a[0] if a else ELF_DEFAULT)
    what = a[1]
    if what == 'moby_vu0':
        out, rest = moby_vu0(elf), a[2:]
    elif what == 'moby_vu1':
        out, rest = moby_vu1(elf), a[2:]
    else:
        sys.exit('usage: vu_micro.py <elf> moby_vu0 | moby_vu1 [out.bin]')
    if rest:
        open(rest[0], 'wb').write(out)
    else:
        print('%d bytes, fnv1a64 %016x' % (len(out), fnv(out)))


if __name__ == '__main__':
    main()
