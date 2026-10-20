#!/usr/bin/env python3
"""LOCAL full UIKit game link attempt, using all proven generated/core objects.

No dynamic_lookup/undefined-symbol bypass, fabricated implementations, no-generated build
or automatic IPA packaging. Failed linker diagnostics are the actual remaining blockers.
Mixed old-Clang investigation inputs can NEVER satisfy the production-fidelity gate.
"""
import argparse
import json
from pathlib import Path
import subprocess
from compile_generated_audit import ROOT
from audit_private_setup import digest
from checkpoint_progress import atomic_json
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--experimental-local-investigation',action='store_true',required=True)
    p.parse_args()
    receipt=json.loads((ROOT/'build/private-game-core-relocatable-link.json').read_text())
    core=ROOT/'build/private-game-core-relocatable.o'
    if receipt['exit_code']!=0 or digest(core)!=receipt['object_sha256'] or receipt['generated_objects']!=227:
        raise RuntimeError('Complete actual generated/native core attestation required')
    names=['uikit-main-arm64.o','game-session-arm64.o','game-data-arm64.o',
        'gs-scene-arm64.o','gs-scene-metal-arm64.o','platform-apple-device.o',
        'game-controller-device.o','coroutine-apple-device.o','arm64-context-device.o',
        'host-input-device.o','host-language-device.o','iso-extract-device.o']
    objects=[core]+[ROOT/'build'/name for name in names]
    for obj in objects:
        subprocess.run(['xcrun','lipo',str(obj),'-verify_arch','arm64'],check=True)
    output=ROOT/'build/RAC1Native-investigation'
    log=ROOT/'build/private-application-link.log'
    command=['xcrun','--sdk','iphoneos','clang++','-target','arm64-apple-ios15.0',*map(str,objects),
        '-framework','UIKit','-framework','MetalKit','-framework','Metal',
        '-framework','Foundation','-framework','CoreGraphics','-framework','GameController','-framework','AVFAudio',
        '-framework','AVFoundation','-framework','AudioToolbox','-framework','CoreAudio',
        '-Wl,-dead_strip','-o',str(output)]
    with log.open('w') as stream:result=subprocess.run(command,cwd=ROOT,stdout=stream,stderr=subprocess.STDOUT)
    report={'kind':'FULL_UIKIT_APPLICATION_LINK_ATTEMPT','exit_code':result.returncode,
        'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        'source_worktree_changed':True,'generated_units':227,'native_core_units':receipt['native_core_objects'],
        'inputs':{str(f.relative_to(ROOT)):digest(f) for f in objects},'command':command,'log':str(log),
        'application_linked':result.returncode==0,'production_fidelity_verified':False,
        'ipa_created':False,'privacy':'LOCAL_ONLY_NO_PRIVATE_UPLOAD'}
    if result.returncode==0:
        report.update(binary_sha256=digest(output),binary_kind=subprocess.check_output(['file',str(output)],text=True).strip())
    atomic_json(ROOT/'build/private-application-link.json',report)
    print(json.dumps({k:report[k] for k in ('kind','exit_code','generated_units','native_core_units','application_linked','ipa_created')}))
    # A failed full application link is reported as failure, never swallowed by a final successful command.
    raise SystemExit(result.returncode)
if __name__=='__main__':main()
