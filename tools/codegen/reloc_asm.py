#!/usr/bin/env python3
"""Relocate the overlay addresses of transliterated assembly (tools/codegen/asm2cpp.py output, ADR 0008).

asm2cpp writes an address as the original builds it: `set32(r, 0x1b0000u);  // lui` then
`set32(r, r.w() + -0x3a00);  // addiu` or a load / store at `r.w() + off`; $gp-relative accesses as
`gp.w() + k`. tools/codegen/reloc_rewrite.py cannot translate those halves. This pass follows each
register loaded by a `lui` with an overlay high half (0x15..0x2f) through the straight-line code
after it (until the register is written again or a label) and rewrites:
  - `set32(r, r.w() + lo)` (addiu)        -> `set32(r, OV(0x...))`  (r then holds a full address)
  - `xx_(r.w() + off)` / `sx_(r.w() + off, ...)`  -> `xx_(OV(0x...))` (a load / store at it)
  - `gp.w() + k` while gp holds the overlay's $gp (set to 0x166c00 and not written since: the
    transliterations also use gp as a scratch register) -> `OV(0x00166c00u + k)`
Code addresses (0x1eaa00..) get OVF. Idempotent. Usage: tools/codegen/reloc_asm.py [--check] files...
"""
import re
import sys
from pathlib import Path

DATA = (0x15EF00, 0x1EAA00)
CODE = (0x1EAA00, 0x2F0CD0)
GP = 0x166C00

LUI = re.compile(r"set32\((\w+), (0x[0-9a-fA-F]+)u?\);\s*// lui ")
LABEL = re.compile(r"^\s*L_\w+:")


def wrap(a):
    if DATA[0] <= a < DATA[1]:
        return f"OV(0x{a:08x}u)"
    if CODE[0] <= a < CODE[1]:
        return f"OVF(0x{a:08x}u)"
    return None


def const_high_regs(text):
    """Registers whose every write in the text is a `lui` of the same overlay high half (a transliterated function's
    constants, set before its loops): they hold it across labels too. Only for the text of ONE function (asm2cpp)."""
    writes = {}
    for line in text.split("\n"):
        code = line.partition("//")[0]
        dst = re.match(r"\s*(?:set32\((\w+),|(\w+)(?:\.lo)? = )", code)
        if not dst:
            continue
        r = dst.group(1) or dst.group(2)
        m = LUI.search(line)
        hi = int(m.group(2), 16) if m and m.group(1) == r and (int(m.group(2), 16) >> 16) in range(0x15, 0x30) else None
        writes.setdefault(r, []).append(hi)
    return {r: w[0] for r, w in writes.items() if w[0] is not None and all(x == w[0] for x in w)}


def relocate(text, one_function=False):
    out, n = [], 0
    consts = const_high_regs(text) if one_function else {}
    held = {}  # register -> the high half a lui put in it (an overlay address's)
    gp_valid = False  # gp holds the overlay's $gp (set32(gp, 0x00166c00) seen, not written since)
    for line in text.split("\n"):
        code, sep, comment = line.partition("//")
        if re.match(r"^\s*(\}|[a-z_0-9:<>]+\s+\w+\(.*\)\s*\{)\s*$", code):
            held.clear()  # a function's start / end (or a block's end: the function's constants stay)
            held.update(consts)
            gp_valid = False
        elif LABEL.match(line):
            held.clear()
            held.update(consts)
        # $gp-relative
        def gpk(m):
            nonlocal n
            k = int(m.group(1), 16) * (-1 if m.group(0).find("+ -") >= 0 else 1)
            w = wrap(GP + k)
            if w is None:
                return m.group(0)
            n += 1
            return w
        if gp_valid:
            code = re.sub(r"\bgp\.w\(\) \+ (?:-)?(0x[0-9a-fA-F]+)", gpk, code)
        # uses of a held high half
        for r, hi in list(held.items()):
            def use(m):
                nonlocal n
                k = int(m.group(2), 16) * (-1 if m.group(1) else 1)
                w = wrap((hi + k) & 0xFFFFFFFF)
                if w is None:
                    return m.group(0)
                n += 1
                return w
            code = re.sub(rf"\b{r}\.w\(\) \+ (-?)(0x[0-9a-fA-F]+)", use, code)
        # what the line writes
        m = LUI.search(line)
        dst = re.match(r"\s*(?:set32\((\w+),|(\w+)(?:\.lo)? = )", code)
        if dst:
            r = dst.group(1) or dst.group(2)
            held.pop(r, None)
            if r == "gp":
                gp_valid = re.match(r"\s*set32\(gp, 0x0*166c00u?\);", code) is not None
            # an addiu just folded into a full address: the register now holds it (no longer a half)
        if m:
            hi = int(m.group(2), 16)
            if (hi >> 16) in range(0x15, 0x30):
                held[m.group(1)] = hi
        out.append(code + sep + comment if sep else code)
    return "\n".join(out), n


def main():
    check = "--check" in sys.argv
    total = 0
    for a in [x for x in sys.argv[1:] if not x.startswith("--")]:
        p = Path(a)
        text = p.read_text()
        new, n = relocate(text)
        if new != text:
            total += n
            print(f"{a}: {n}")
            if not check:
                if "game/ovl/ovl_reloc.h" not in new:
                    m = list(re.finditer(r'^#include [<"][^>"]+[>"]\s*$', new, re.M))
                    new = new[:m[0].end()] + "\n#include <game/ovl/ovl_reloc.h>" + new[m[0].end():]
                p.write_text(new)
    print(f"total {total}")
    return 1 if check and total else 0


if __name__ == "__main__":
    sys.exit(main())
