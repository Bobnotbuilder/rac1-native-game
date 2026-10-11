#!/usr/bin/env python3
"""Match the boot module (the code SCUS_971.99 runs before the first level: the boot, the
intro movie, the title and its main menu; the disc's globals/frontbin.elf is the same image)
against Veldin's overlay, both ways, as tools/overlay_match does for the levels (task 0301).

The boot module occupies SCUS_971.99's sections .lit / .bss / .data / lvl.* / .text
(0x15ef00..0x23d360, entry 0x1e9658): a build of the same engine as the levels' overlays, linked
at its own addresses. This tool reports which of its functions are Veldin's code (relocations
masked) and maps its data addresses to Veldin's (e.g. its game mode 0x15f604 is Veldin's
0x15f5c4), the evidence the port uses to run the boot module's shared code on Veldin's build
(src/game/boot/front_end.cpp, docs/game-logic.md, "The boot").

Output (gitignored, generated from your ELF dump and ISO; tools/setup_from_iso.py):
  <out>/front.txt         "fn <boot> <veldin|-------->  <size>" for every boot module function,
                          "data <boot> <veldin> <uses>" and a summary
  <out>/front_reloc.txt   the other way round, in the levels' map format (overlay_match.py's
                          levelN.txt: "fn <veldin> <boot> <size>", "data <veldin> <boot> <uses>"):
                          the translation the ports use when they run on the boot module's own
                          image (task 0308, ADR 0008: src/game/ovl/ovl_reloc.cpp, OVL_FRONT)

Usage (repo root):
  tools/overlay_match/front_match.py [--elf assets/dumps/SCUS_971.99] [--iso assets/iso/rac1-ntsc-u.iso]
                                     [--out assets/generated/overlay_map]
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import overlay_match as om  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SECTIONS = [".lit", ".bss", ".data", "lvl.vtbl", "lvl.camvtbl", "lvl.sndvtbl", ".text"]


def boot_module(elf):
    """The boot module's sections [(dest, type, bytes)] in the overlay layout (code last)."""
    shoff = struct.unpack_from("<I", elf, 0x20)[0]
    shnum = struct.unpack_from("<H", elf, 0x30)[0]
    shstrndx = struct.unpack_from("<H", elf, 0x32)[0]
    secs = [struct.unpack_from("<IIIIII", elf, shoff + i * 40) for i in range(shnum)]
    stro = secs[shstrndx][4]

    def name(n):
        return elf[stro + n:elf.index(b"\0", stro + n)].decode()
    out = []
    for want in SECTIONS:
        for n, typ, _flags, addr, off, size in secs:
            if name(n) == want:
                out.append((addr, 1 if typ == 1 else 8, elf[off:off + size] if typ == 1 else b""))
    if len(out) != len(SECTIONS):
        raise SystemExit("[front_match] the boot module's sections not found in the ELF")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", default=str(ROOT / "assets/dumps/SCUS_971.99"))
    ap.add_argument("--iso", default=str(ROOT / "assets/iso/rac1-ntsc-u.iso"))
    ap.add_argument("--out", default=str(ROOT / "assets/generated/overlay_map"))
    a = ap.parse_args()
    F = om.Overlay(boot_module(open(a.elf, "rb").read()), 0x1E9658)
    with open(a.iso, "rb") as iso:
        V = om.Overlay(*om.level_overlay(iso, om.VELDIN))
    starts = F.function_starts()
    fmap, exact, _amb, missing, sizes = om.match(F, V, starts)
    pairs, jal_bad = om.data_pairs(F, V, fmap, sizes)
    vmap, _e, _a, _m, _s = om.match(V, F, V.function_starts())
    total = sum(sizes.values())
    only = [f for f in starts if f not in fmap]
    lines = [
        "# the boot module (SCUS_971.99 %08x..%08x, entry 1e9658) matched in Veldin's overlay (tools/overlay_match/front_match.py, task 0301)"
        % (F.code_base, F.code_end),
        "# boot module functions %d (%d bytes): found in Veldin %d (%d bytes), byte-identical %d, only in the boot module %d (%d bytes)"
        % (len(starts), total, len(fmap), sum(sizes[f] for f in fmap), exact, len(only), sum(sizes[f] for f in only)),
        "# Veldin's functions found in the boot module: %d; jal targets inconsistent with the map: %d" % (len(vmap), jal_bad),
    ]
    for name, lo, hi in om.REGIONS:
        sel = [(x, c.most_common(1)[0][0]) for x, c in pairs.items() if lo <= x < hi]
        lines.append("# data %s: %d addresses, unchanged %d" % (name, len(sel), sum(1 for x, y in sel if x == y)))
    for f in starts:
        lines.append("fn %08x %s %d" % (f, ("%08x" % fmap[f]) if f in fmap else "--------", sizes[f]))
    for x in sorted(pairs):
        y, _c = pairs[x].most_common(1)[0]
        lines.append("data %08x %08x %d" % (x, y, sum(pairs[x].values())))
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    (out / "front.txt").write_text("\n".join(lines) + "\n")
    print("\n".join(l for l in lines if l.startswith("#")))
    # Veldin -> the boot module, as for a level (task 0308)
    om.compare(V, F, "front", out, None)
    (out / "levelfront.txt").replace(out / "front_reloc.txt")


if __name__ == "__main__":
    main()
