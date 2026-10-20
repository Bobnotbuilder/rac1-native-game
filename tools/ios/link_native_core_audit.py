#!/usr/bin/env python3
"""Combine verified LOCAL game objects, inventory actual remaining dependencies.

Output remains MH_OBJECT, not an executable. Never allows undefined symbols in an
application or reports this partial link as successful full-game/IPA delivery.
"""
import json
from pathlib import Path
import subprocess
from compile_generated_audit import ROOT
from audit_private_setup import digest
from checkpoint_progress import atomic_json
def undefined(path):
    return set(subprocess.check_output(['xcrun','nm','-u',str(path)],text=True).splitlines())
def main():
    generated=json.loads((ROOT/'build-private-arm64-O0-probes/compilation-receipt.json').read_text())
    core=json.loads((ROOT/'build-private-arm64-core/compilation-receipt.json').read_text())
    if generated['covered_units']!=generated['expected_units'] or core['compiled_units']!=core['expected_units']:
        raise RuntimeError('All actual manifested objects are required')
    entries=list(generated['covered'].values())+[core['units'][name] for name in core['source_inventory']]
    objects=[]
    for item in entries:
        path=Path(item['object'])
        if not path.is_file() or digest(path)!=item['object_sha256']:
            raise RuntimeError('Object missing/changed; no link claim')
        objects.append(str(path))
    output=ROOT/'build/private-game-core-relocatable.o'
    command=['xcrun','ld','-r','-arch','arm64','-platform_version','ios','15.0','15.2',*objects,'-o',str(output)]
    with (ROOT/'build/private-game-core-relocatable-link.log').open('w') as log:
        result=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    report={'generated_objects':generated['expected_units'],'native_core_objects':core['expected_units'],
        'exit_code':result.returncode,'kind':'MH_OBJECT_RELOCATABLE_NOT_EXECUTABLE',
        'full_game_linked':False,'ipa_created':False,'privacy':'LOCAL_ONLY',
        'production_fidelity_verified':False,'command':command}
    if result.returncode==0:
        old=undefined(ROOT/'build/private-generated-game-relocatable.o')
        remaining=undefined(output)
        demangled=subprocess.check_output(['c++filt'],input='\n'.join(sorted(remaining)),text=True)
        (ROOT/'build/private-core-unresolved-symbols.txt').write_text(demangled)
        report.update(object_sha256=digest(output),unresolved_symbols=sorted(remaining),
            previous_unresolved_symbols=len(old),previous_dependency_names_now_resolved=len(old-remaining),
            previous_dependency_names_still_unresolved=len(old&remaining),
            remaining_symbol_names=len(remaining),measurement_scope='LINK_SYMBOLS_NOT_ENGINE_CAPABILITY_OR_GAMEPLAY_COVERAGE')
    atomic_json(ROOT/'build/private-game-core-relocatable-link.json',report)
    print(json.dumps({k:v for k,v in report.items() if k not in ('command','unresolved_symbols')}))
    raise SystemExit(result.returncode)
if __name__=='__main__':main()
