#!/usr/bin/env python3
"""Bounded LOCAL iPhone core dependency compilation; no stubs, no game link claim.

Reuses all existing generated objects separately. This audit compiles actual handwritten
upstream game/runtime sources, retaining each failed diagnostic and content-keyed object.
Old Clang's experimental strict FP requires explicit opt-in and is NEVER release acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parent))
from compile_generated_audit import ROOT, run_compiler, architecture_command
from audit_private_setup import digest
from checkpoint_progress import atomic_json

def direct_hpp_context(source):
    """The new native adapter uses .hpp, outside upstream's .h-only generation inventory."""
    included={}
    for name in re.findall(r'^\s*#\s*include\s*[<"]([^>"]+\.hpp)[>"]',source.read_text(),re.M):
        for base in (source.parent,ROOT/'src',ROOT):
            header=base/name
            if header.is_file():
                included[str(header.resolve())]=digest(header);break
    return hashlib.sha256(json.dumps(included,sort_keys=True).encode()).hexdigest() if included else ''

def inventory():
    text=(ROOT/'src/game/CMakeLists.txt').read_text()
    block=text.split('add_library(rac1_game STATIC',1)[1].split(')',1)[0]
    game=['src/game/'+s for s in re.findall(r'^\s+([^#\s]+\.cpp)\s*$',block,re.M)]
    # Renderer sources are integrated/compiled separately, never replaced by success stubs.
    selected=[s for s in game if '/render/' not in s]
    selected += ['src/game/render/render_toggles.cpp']
    runtime=[str(p.relative_to(ROOT)) for p in sorted((ROOT/'runtime').rglob('*.cpp'))
             if not any(x in str(p.relative_to(ROOT)) for x in
                        ('runtime/gs/gs_draw.cpp','runtime/gs/gs_renderer.cpp',
                         'runtime/gs/static_geom_cache.cpp','runtime/kernel/vu0.cpp',
                         'runtime/ee/ee_interpreter.cpp'))]
    # The original interpreter is an oracle reference, never a new app runtime fallback.
    selected += ['src/game/render/'+s+'.cpp' for s in
                 ('render_view','mrp_scene','mrp_world_assets','mrp_world_check',
                  'mrp_world_scene','mrp_world_static','mrp_lod','mrp_title_view','texture_decode')]
    formats=[str(p.relative_to(ROOT)) for p in sorted((ROOT/'src/formats').glob('*.cpp'))]
    sound=[str(p.relative_to(ROOT)) for folder in ('game/sound/989snd','game/sound/common')
           for p in sorted((ROOT/'third_party/989snd'/folder).glob('*.cpp'))]
    return selected+runtime+formats+sound

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--experimental-clang13-investigation',action='store_true',required=True)
    p.add_argument('--max-seconds',type=int,default=600)
    p.add_argument('--unit-timeout',type=int,default=120)
    args=p.parse_args()
    if not 1<=args.max_seconds<=1800 or not 1<=args.unit_timeout<=300:
        p.error('Bounded batches/units required')
    units=inventory()
    directory=ROOT/'build-private-arm64-core'; directory.mkdir(exist_ok=True)
    receipt=directory/'compilation-receipt.json'
    report=json.loads(receipt.read_text()) if receipt.exists() else {'units':{}}
    compiler=subprocess.check_output(['xcrun','--sdk','iphoneos','clang++','--version'],text=True)
    headers={str(p.relative_to(ROOT)):digest(p) for folder in ('src','runtime','ios')
             for p in (ROOT/folder).rglob('*.h')}
    context=hashlib.sha256(json.dumps(headers,sort_keys=True).encode()).hexdigest()
    command=['xcrun','--sdk','iphoneos','clang++','-target','arm64-apple-ios15.0','-std=c++20',
             '-O0','-g','-frounding-math','-ffp-exception-behavior=strict',
             '-Xclang','-fexperimental-strict-floating-point','-ffp-contract=off',
             '-fno-strict-aliasing','-Werror=unsupported-floating-point-opt',
             '-I'+str(ROOT/'ios/simd'),'-I'+str(ROOT),'-I'+str(ROOT/'src'),
             '-I'+str(ROOT/'third_party/989snd')]
    glm=ROOT/'build-public-dependencies/glm-1.0.1'
    if (glm/'provenance.json').exists():
        # Dependency cache must already have been verified against pinned Git blob hashes.
        from verify_dependency_cache import checked_bytes
        pinned=json.loads((glm/'provenance.json').read_text())
        for item in pinned['files']:
            checked_bytes((glm/item['path']).read_bytes(),item['size'],item['sha'])
        command += ['-isystem',str(glm)]
    report.update(expected_units=len(units),source_inventory=units,compiler=compiler,
        command_prefix=command,experimental_investigation=True,full_game_linked=False,
        ipa_created=False,privacy='LOCAL_ONLY_NOT_PUBLIC_CI',production_fidelity_verified=False)
    started=time.monotonic()
    for index,name in enumerate(units):
        if time.monotonic()-started>args.max_seconds: break
        source=ROOT/name
        key=hashlib.sha256((digest(source)+context+compiler+json.dumps(command)+direct_hpp_context(source)).encode()).hexdigest()
        obj=directory/(name.replace('/','__')+'.o')
        previous=report['units'].get(name,{})
        if previous.get('key')==key and previous.get('exit_code')==0 and obj.exists() and digest(obj)==previous.get('object_sha256'):
            continue
        log=obj.with_suffix('.log'); part=obj.with_suffix('.o.part')
        print(f'CORE_COMPILE={index+1}/{len(units)} SOURCE={name}',flush=True)
        before=time.monotonic()
        with log.open('w') as stream: rc=run_compiler(command+['-c',str(source),'-o',str(part)],ROOT,stream,args.unit_timeout)
        record={'key':key,'exit_code':rc,'log':str(log),'elapsed_seconds':time.monotonic()-before}
        if rc==0:
            subprocess.run(architecture_command(part),check=True)
            part.replace(obj)
            record.update(object=str(obj),object_sha256=digest(obj))
        report['units'][name]=record
        report['compiled_units']=sum(report['units'].get(n,{}).get('exit_code')==0 for n in units)
        report['failed_units']=[n for n in units if report['units'].get(n,{}).get('exit_code',0)!=0]
        report['status']='CORE_DEPENDENCY_AUDIT_IN_PROGRESS_NOT_LINKED'
        atomic_json(receipt,report)
    report['status']='CORE_COMPILE_COMPLETE_NOT_LINKED' if report.get('compiled_units')==len(units) else 'CORE_COMPILE_GAPS_RECORDED'
    atomic_json(receipt,report)
    print(json.dumps({k:report.get(k) for k in ('status','compiled_units','expected_units','failed_units')}),flush=True)

if __name__=='__main__': main()
