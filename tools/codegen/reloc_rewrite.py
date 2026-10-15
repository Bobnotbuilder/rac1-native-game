#!/usr/bin/env python3
"""Make the ports' overlay literals relocatable (ADR 0008): every hex literal of src/game in
Veldin's overlay data (0x15ef00..0x1eaa00: sections 0..5) becomes OV(literal), in its code
(0x1eaa00..0x2f0cd0) OVF(literal); a namespace-scope `constexpr uint32_t NAME = literal;`
becomes `constexpr OvConst<literal> NAME{};`. Comments, strings, `case` labels, TODO_PORT's
address, the registries' keys (ovl_register_moby_fn / snd_register_callback: Veldin's addresses,
the dispatchers translate) and literals already wrapped are left alone. Idempotent.

Usage (repo root): tools/codegen/reloc_rewrite.py [--check] [files...]   (default: src/game/**/*.{cpp,h})
  --check: list what would change, write nothing (exit 1 if anything would).
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = (0x15EF00, 0x1EAA00)
GP_VALUE = 0x166C00  # $gp: the same on every overlay (gp-relative addresses are translated whole)
CODE = (0x1EAA00, 0x2F0CD0)
SKIP_FILES = {"src/game/ovl/ovl_reloc.h", "src/game/ovl/ovl_reloc.cpp",
              # the per-level variant table (Veldin function -> the level's own): both sides are literal addresses
              "src/game/ovl/variants_l1.cpp", "src/game/ovl/variants_l2.cpp", "src/game/ovl/variants_l3.cpp", "src/game/ovl/variants_l4.cpp", "src/game/ovl/variants_l5.cpp", "src/game/ovl/variants_l6.cpp", "src/game/ovl/variants_l7.cpp", "src/game/ovl/variants_l8.cpp", "src/game/ovl/variants_l9.cpp", "src/game/ovl/variants_l10.cpp", "src/game/ovl/variants_l11.cpp", "src/game/ovl/variants_l12.cpp", "src/game/ovl/variants_sky.cpp",
              # the boot module's code (its own image in SCUS_971.99, its own addresses; the host
              # runs its loader with no overlay translation)
              "src/game/boot/front_end.cpp", "src/game/boot/front_end.h",
              "src/game/boot/level_read.cpp", "src/game/boot/level_read.h", "src/game/boot/front_image.cpp",
              "src/game/boot/boot_start.cpp", "src/game/boot/title_load.cpp", "src/game/boot/title.cpp",
              "src/game/boot/title_menu.cpp", "src/game/boot/title.h", "src/game/boot/title_menu.h", "src/game/boot/boot_start.h",
              "src/game/boot/title_sky.cpp", "src/game/boot/title_sky.h"}

TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\b0[xX][0-9a-fA-F]+[uUlL]*\b', re.S)
CONSTEXPR = re.compile(r'^(\s*)(?:static\s+)?constexpr\s+uint32_t\s+(\w+)\s*=\s*(0[xX][0-9a-fA-F]+)[uU]?\s*;', re.M)


def kind(v):
    if DATA[0] <= v < DATA[1]:
        return "OV"
    if CODE[0] <= v < CODE[1]:
        return "OVF"
    return None


def rewrite(text):
    changes = []

    def cx(m):
        v = int(m.group(3), 16)
        if kind(v) != "OV" or v == GP_VALUE:
            return m.group(0)
        changes.append(m.group(0).strip())
        return f"{m.group(1)}constexpr OvConst<0x{v:08x}u> {m.group(2)}{{}};"

    text = CONSTEXPR.sub(cx, text)
    out, pos = [], 0
    for m in TOKEN.finditer(text):
        tok = m.group(0)
        if not tok[0] == "0":
            continue  # comment / string
        v = int(re.match(r"0[xX][0-9a-fA-F]+", tok).group(0), 16)
        k = kind(v)
        if k is None or v == GP_VALUE:
            continue
        before = text[max(0, m.start() - 48):m.start()]
        if re.search(r"\b(OV|OVF|OvConst<|ov_site<|ovf_site<|NOT_ADDR|L1_ADDR|L2_ADDR|L3_ADDR|L4_ADDR|L5_ADDR|L6_ADDR|L7_ADDR|L8_ADDR|L9_ADDR|L10_ADDR|L11_ADDR|L12_ADDR)\s*\(?\s*$", before) or before.rstrip().endswith("OvConst<"):
            continue
        line_end = text.find("\n", m.end())
        if "// lui " in text[m.end():line_end if line_end >= 0 else len(text)]:
            continue  # a lui's high half (transliterated assembly): tools/codegen/reloc_asm.py folds it
        if re.search(r"\bcase\s+$", before) or re.search(r"(TODO_PORT|ovl_register_moby_fn|snd_register_callback)\(\s*$", before):
            continue
        out.append(text[pos:m.start()])
        out.append(f"{k}({tok})")
        pos = m.end()
        changes.append(tok)
    out.append(text[pos:])
    return fix_constexpr("".join(out), changes), changes


def fix_constexpr(text, changes):
    """A constexpr declaration whose value now goes through OV: at namespace scope (column 0),
    each `NAME = OV(lit)` becomes an OvConst; inside a function, the declaration becomes const
    (evaluated at each call: the level's address)."""
    lines = text.split("\n")
    for i, l in enumerate(lines):
        m = re.match(r"^(\s*)(static\s+)?constexpr\s+uint32_t\s+(.*);(.*)$", l)
        if not m:
            if re.match(r"^\s*(static\s+)?constexpr\s+uint32_t\s", l) and "OV(" in l:  # (continued on the next line)
                raise SystemExit(f"reloc_rewrite: a constexpr declaration over several lines to fix by hand: {l.strip()}")
            continue
        ind, body, tail = m.group(1), m.group(3), m.group(4)
        # (inside a function: also a constant computed from a named overlay constant)
        if "OV(" not in l and (ind == "" or not re.search(r"\b[A-Z_][A-Z0-9_]+\s*[-+]", body)):
            continue
        decls = [d.strip() for d in re.split(r",(?![^()]*\))", body)]
        if ind == "":
            out, plain = [], []
            for d in decls:
                k = re.fullmatch(r"(\w+)\s*=\s*OV\((0x[0-9a-fA-F]+)u?\)", d)
                if k:
                    out.append(f"constexpr OvConst<{k.group(2)}u> {k.group(1)}{{}};")
                elif "OV(" in d:
                    raise SystemExit(f"reloc_rewrite: a namespace-scope constant to fix by hand: {l.strip()}")
                else:
                    plain.append(d)
            if plain:
                out.insert(0, "constexpr uint32_t " + ", ".join(plain) + ";")
            lines[i] = "\n".join(out) + tail
        else:
            lines[i] = l.replace("static constexpr uint32_t", "const uint32_t", 1).replace("constexpr uint32_t", "const uint32_t", 1)
        changes.append(l.strip())
    return "\n".join(lines)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    check = "--check" in sys.argv
    files = [Path(a) for a in args] or sorted(p for p in (ROOT / "src/game").rglob("*") if p.suffix in (".cpp", ".h"))
    total = 0
    for p in files:
        rel = str(p.resolve().relative_to(ROOT))
        if rel in SKIP_FILES:
            continue
        text = p.read_text()
        new, changes = rewrite(text)
        if not changes:
            continue
        total += len(changes)
        if check:
            print(f"{rel}: {len(changes)}")
            continue
        if "game/ovl/ovl_reloc.h" not in new:
            m = list(re.finditer(r'^#include [<"][^>"]+[>"]\s*$', new, re.M))
            at = m[0].end() if m else 0
            new = new[:at] + "\n#include <game/ovl/ovl_reloc.h>" + new[at:]
        p.write_text(new)
        print(f"{rel}: {len(changes)}")
    print(f"total {total}")
    return 1 if check and total else 0


if __name__ == "__main__":
    sys.exit(main())
