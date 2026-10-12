#!/usr/bin/env python3
# usage: mkall_l10.py <out.cpp> "<title>" <addr hex>:<cpp name> [...]
#
# tools/codegen/mkall.py + mips2cpp.py for Orxon (level 10): the level-1 generator with the level's own names, applied
# as textual substitutions to temporary copies (so the shared tools stay as they are): the overlay map
# `level10.txt` (and the same-code rows of src/game/ovl/variants_l10.cpp as Veldin equivalents), `L10:` annotations,
# `L10_ADDR` literals, helpers `orx_<addr>_l10`, the base ELF's bound 0x15ef00 (Orxon's overlay code starts at
# 0x1de000, not 0x1eaa00: addresses below 0x1eaa00 are not all the ELF's, the ELF ends at 0x15ef00).
# The level's overlay is read straight from the ISO (MKALL_ISO=1, tools/codegen/ovlist.py: no Ghidra project, as for Kerwan).
# Environment: MKALL_COMMON (glue for callees the ports have no declaration for: the base ELF's rand / memset, ...),
# MKALL_TMP (the listings' cache), MKALL_INCLUDE as in mkall.py.
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
W = os.path.abspath(os.path.join(HERE, '..', '..'))

class _Patched(str):
    """A source text whose `replace` says when a patch found nothing to change (the shared tools move on: a patch that no longer applies is either obsolete or a bug)."""
    def replace(self, old, new, *a):
        if old not in self:
            print('[mkall_l10] patch not applied: %r' % (old[:70],), file=sys.stderr)
        return _Patched(str.replace(self, old, new, *a))


mk = _Patched(open(os.path.join(HERE, 'mkall.py')).read())
tr = _Patched(open(os.path.join(HERE, 'mips2cpp.py')).read())
d = tempfile.mkdtemp(prefix='mkall_l10_')
shutil.copy(os.path.join(HERE, 'asm2cpp.py'), d)

tr = tr.replace('(?:OVF?|L1_ADDR)', '(?:OVF?|L10_ADDR)')
open(os.path.join(d, 'mips2cpp.py'), 'w').write(tr)

mk = mk.replace("W = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))", "W = %r" % W)
mk = mk.replace("TR = os.path.dirname(os.path.abspath(__file__)) + '/mips2cpp.py'", "TR = %r" % (d + '/mips2cpp.py'))
mk = mk.replace("nov_%06x_l1", "orx_%06x_l10")
mk = mk.replace("'lv' + LEVEL + '_<addr>_l' + LEVEL}", "os.environ.get('MKALL_HELPER_PREFIX', 'orx_') + '<addr>_l10'}")
mk = mk.replace("(t if (t < 0x1eaa00 or VELDIN) else None)", "(t if (t < 0x15ef00 or VELDIN) else None)").replace("if t < 0x1eaa00:  # a base ELF function", "if t < 0x15ef00:  # a base ELF function")
mk = mk.replace("# ---- the ports' declarations by Veldin address", """import re as _re
for _l in open(W + '/src/game/ovl/variants_l10.cpp'):
    _m = _re.match(r'\\s*\\{0x([0-9a-f]+), 0x([0-9a-f]+), 0x([0-9a-f]+)\\}', _l)
    if _m:
        fnmap[int(_m.group(2), 16)] = int(_m.group(1), 16)

# ---- the ports' declarations by Veldin address""", 1)
# the headers of the ports a glue line calls (the fixed include list of the generated head does not name every one)
mk = mk.replace("            if g:\n                glue[t] = g[1]\n                continue", "            if g:\n                glue[t] = g[1]\n                USED_HDR.add(g[2].rsplit('(', 1)[1].rstrip(')'))\n                continue")
mk = mk.replace("# ---- the ports' declarations by Veldin address", """USED_HDR = set()


def _used_includes():
    out = ''
    for h in sorted(USED_HDR):
        for dd in ('src/game/ovl', 'src/game/math', 'src/game/lib', 'src/game'):
            if os.path.exists(W + '/' + dd + '/' + h):
                out += '#include <%s/%s>\\n' % (dd[4:], h)
                break
    return out


# ---- the ports' declarations by Veldin address""", 1)
mk = mk.replace("{EXTRA_INC}#include <game/math/vecmath.h>", "{EXTRA_INC}{_used_includes()}#include <game/math/vecmath.h>")
# a declaration line with two `orig:` annotations (ovl.h has a few): the first one is the function's
mk = mk.replace(r"\)\s*;\s*//.*orig: 0x0*([0-9a-f]+)", r"\)\s*;\s*//.*?orig: 0x0*([0-9a-f]+)")
# an entry named `..._u` keeps the uniform ABI (TrRet, every argument register): a camera type's accept function ($a0 camera, $a1 other)
mk = mk.replace("        if ad in entry_names:\n            cmd +=", "        if ad in entry_names and not entry_names[ad].endswith('_u'):\n            cmd +=")
mk = mk.replace("        if ad not in entry_names:\n            glue[ad]", "        if ad not in entry_names or entry_names[ad].endswith('_u'):\n            glue[ad]")
mk = mk.replace("        if ad in entry_names:\n            rt, ar = ENTRY_SIG[ad]", "        if ad in entry_names and not entry_names[ad].endswith('_u'):\n            rt, ar = ENTRY_SIG[ad]")
# the unported-callee stubs=1 (the hero variants): a callee that is a Veldin function without a port stays a TODO_PORT stub, as in the Veldin
# ports (the per-level oracle's batches no-op those callees on the original's side too); without it such callees are transliterated
mk = mk.replace("            visit(t, stack + [ad])", """            if os.environ.get('MKALL_STUB_UNPORTED') and v and t >= 0x15ef00 and t not in FORCE:
                glue[t] = '{ TODO_PORT(0x%08x, "Veldin function 0x%x without a port")(); set32(v0, 0); }' % (t, v)
                continue
            visit(t, stack + [ad])""", 1)
# MKALL_HELPER_PREFIX: the helpers' name prefix (the hero run and the class run have their own name spaces)
mk = mk.replace("'lv' + LEVEL + '_%06x_l' + LEVEL", "os.environ.get('MKALL_HELPER_PREFIX', 'orx_') + '%06x_l10'")
# the entries in the uniform ABI (`..._u`) call each other (the hero functions): their glue is known before they are generated
mk = mk.replace("    entry_names = dict(entries)\n", """    entry_names = dict(entries)
    for _ad, _nm in entries:
        if _nm.endswith('_u'):
            glue[_ad] = '{ const TrRet r = %s(a0.w(), a1.w(), a2.w(), a3.w(), t0.w(), t1.w(), t2.w(), t3.w(), %s); set32(v0, r.v0); fr[0] = r.f0; }' % (_nm, FARGS)
""", 1)
open(os.path.join(d, 'mkall_l10_run.py'), 'w').write(mk)
env = dict(os.environ)
env['PYTHONPATH'] = HERE + os.pathsep + env.get('PYTHONPATH', '')  # (the copy of mkall.py in a temp dir imports tools/codegen/ovlist.py)
env.setdefault('MKALL_ISO', '1')  # the level's overlay straight from the ISO (tools/codegen/ovlist.py), no Ghidra project
env.setdefault('MKALL_LEVEL', '10')
r = subprocess.run([sys.executable, os.path.join(d, 'mkall_l10_run.py')] + sys.argv[1:], env=env)
shutil.rmtree(d, ignore_errors=True)
sys.exit(r.returncode)
