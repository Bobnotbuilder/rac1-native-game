#!/usr/bin/env python3
"""usage: asm2fn_batch.py <spec.json>

Generates a C++ file of transliterated functions (tools/codegen/asm2fn.py) from a spec:
  {"out": "src/game/ovl/foo_asm.cpp", "head": "// file comment lines...", "includes": ["game/ovl/weapons.h", ...],
   "listings": "$RE_TMP (the directory of lst_<addr>.s, the translator / lst_level.py)", "level": 1 (a level's own functions),
   "functions": [{"addr": "2c8078", "name": "doom_agent_splash", "ret": "void", "params": "uint32_t m",
                  "jr": ["2c80d8"], "jalr": [], "stub": [], "doc": "one comment line"}],  ("hand": true: ported by hand, only declared)
   "register_fn": "foo_fns_register", "register": [["2c89e8", "doom_agent_update"]]}
The listing of each function is read from <listings>/lst_<addr>.s (run the translator first); the headers of the callees it
calls through an adapter are included automatically. the translator makes the spec of a weapon / class from its root.
"""
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

DEFAULT_INCLUDES = ['game/math/vecmath.h', 'game/lib/libc.h', 'game/ovl/ovl_reloc.h', 'game/ovl/ee_regs.h', 'game/ovl/ovl.h',
                    'game/ovl/ovl_common.h', 'game/ovl/enemy_749.h', 'game/ovl/quad_draw.h', 'game/ovl/novalis_hooks.h', 'game/port_stubs.h']


def main():
    spec = json.load(open(sys.argv[1]))
    extra_inc = set()
    body = []
    for f in spec['functions']:
        if f.get('hand'):  # ported by hand (its declaration in the spec makes it a known callee of the others)
            continue
        lst = os.path.join(os.path.expandvars(spec['listings']), f'lst_{f["addr"]}.s')
        cmd = [sys.executable, os.path.join(HERE, 'asm2fn.py'), lst, '--name', f['name'], '--ret', f.get('ret', 'void'),
               '--params', f.get('params', ''), '--spec', sys.argv[1]]
        if f.get('jr'):
            cmd += ['--jr', ','.join(f['jr'])]
        if f.get('jalr'):
            cmd += ['--jalr', ','.join(f['jalr'])]
        if f.get('level', spec.get('level')):  # (a function's own "level" overrides the spec's)
            cmd += ['--level', str(f.get('level', spec.get('level')))]
        if spec.get('jalr_level_first'):  # the level's own class hooks asked first at every jalr
            cmd += ['--jalr-level-first']
        if f.get('stub'):
            cmd += ['--stub', ','.join(f['stub'])]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            sys.stderr.write(r.stderr)
            sys.exit(1)
        if r.stderr.strip():
            sys.stderr.write(f'{f["name"]}: {r.stderr}')
        lines = []
        for ln in r.stdout.rstrip('\n').split('\n'):
            m_ = re.match(r'^// asm2fn-include: <(.+)>$', ln)
            if m_:
                extra_inc.add(m_.group(1))
            else:
                lines.append(ln)
        if f.get('doc'):  # the doc comment goes above the `// orig:` line
            body.append(f'// {f["doc"]}')
        body.extend(lines)
        body.append('')
    if spec.get('register_fn'):
        body.append('// orig: (none) -- the ports for the engine\'s indirect calls (a moby class\'s update, a draw callback)')
        body.append(f'void {spec["register_fn"]}() {{')
        for addr, fn in spec.get('register', []):
            body.append(f'    ovl_register_moby_fn(0x{int(addr, 16):08x}, {fn});')
        body.append('}')
        body.append('')
    out = [spec.get('head', '').rstrip(),
           '// (the overlay addresses of the lui halves are relocated by tools/codegen/hi_flow.py, a pass over the control flow)']
    seen = []
    for inc in DEFAULT_INCLUDES + spec.get('includes', []) + sorted(extra_inc):
        if inc not in seen:
            seen.append(inc)
            out.append(f'#include <{inc}>')
    vu0 = any('vf()[' in l for l in body)
    if vu0:
        out.append('#include <game/ovl/vu0_regs.h>')
    out += ['', 'namespace rac1 {', '', 'using namespace eer;'] + (['using namespace vu0m;'] if vu0 else []) + ['']
    out += body
    out.append('}  // namespace rac1')
    path = os.path.join(ROOT, spec['out']) if not os.path.isabs(spec['out']) else spec['out']
    open(path, 'w').write('\n'.join(out) + '\n')
    text = '\n'.join(out)
    errs = [l for l in text.split('\n') if '#error' in l]
    print(f'{path}: {len(spec["functions"])} functions, {len(errs)} #error')
    for l in errs:
        print('  ', l.strip())


if __name__ == '__main__':
    main()
