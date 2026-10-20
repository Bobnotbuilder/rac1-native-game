#!/usr/bin/env python3
"""PRIVATE mixed-optimization ARM64 source-compatibility investigation, NEVER a release build.

Preserves the O1 timeout and every validated O1 object. Separately probes only remaining
translation units at O0. No app link, source generation, input upload or fidelity acceptance.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import time
from compile_generated_audit import ROOT, architecture_command, run_compiler, source_context_fingerprint, unit_cache_key
from audit_private_setup import digest
from checkpoint_progress import atomic_json

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--experimental-clang13-investigation",action="store_true",required=True)
    p.add_argument("--max-seconds",type=int,default=900)
    args=p.parse_args()
    if not 1<=args.max_seconds<=1800: p.error("Use a bounded 1..1800 second investigation")
    spec=importlib.util.spec_from_file_location("private_preparation",ROOT/"tools/ios/prepare_private_sources.py")
    prep=importlib.util.module_from_spec(spec); spec.loader.exec_module(prep)
    setup=prep.load_setup(); files=prep.verify_generated_state(setup)
    parent=json.loads((ROOT/"build-private-arm64-generated/compilation-receipt.json").read_text())
    version=subprocess.check_output(["xcrun","--sdk","iphoneos","clang++","--version"],text=True)
    if version!=parent["compiler"] or not parent["experimental_investigation"]:
        raise RuntimeError("Compiler/investigation context changed; do not claim reuse")
    optimized=parent["command_prefix"]
    if "-O1" not in optimized: raise RuntimeError("Expected the recorded O1 investigation")
    base=["-O0" if flag=="-O1" else flag for flag in optimized]
    fingerprint=source_context_fingerprint(setup)
    directory=ROOT/"build-private-arm64-O0-probes"; directory.mkdir(exist_ok=True)
    receipt=directory/"compilation-receipt.json"
    report=json.loads(receipt.read_text()) if receipt.exists() else {"units":{}}
    report.update(expected_units=len(files),compiler=version,probe_command_prefix=base,
        scope="MIXED_O1_AND_O0_GENERATED_SOURCE_COMPATIBILITY_ONLY",privacy="LOCAL_ONLY_DO_NOT_UPLOAD",
        production_fidelity_verified=False,full_game_linked=False,ipa_created=False)
    started=time.monotonic(); covered={}
    for name in files:
        source=setup.CODE_ROOT/name
        prior=parent["units"].get(name,{})
        old_key=unit_cache_key(source,fingerprint,version,optimized)
        old_object=ROOT/"build-private-arm64-generated"/(Path(name).name+".o")
        if prior.get("key")==old_key and prior.get("exit_code")==0 and old_object.is_file() and digest(old_object)==prior.get("object_sha256"):
            covered[name]={"optimization":"O1","object":str(old_object),"object_sha256":digest(old_object)}
            continue
        key=unit_cache_key(source,fingerprint,version,base)
        output=directory/(Path(name).name+".o")
        existing=report["units"].get(name,{})
        if not (existing.get("key")==key and output.is_file() and digest(output)==existing.get("object_sha256")):
            # Reuse the immediately preceding single-unit probe only when its original O1
            # context key and exact O0 command still match. Never overwrite its evidence.
            single=ROOT/"build/world-tfrag-a-fast-O0-probe.json"
            if name=="src/game/ovl/world_vu1_tfrag_a_fast.cpp" and prior.get("key")==old_key and single.exists():
                data=json.loads(single.read_text())
                candidate=ROOT/"build/world-tfrag-a-fast-arm64-O0-investigation.o"
                if data.get("exit_code")==0 and data.get("source_sha256")==digest(source) and candidate.is_file() and data.get("command")==base+["-c",str(source),"-o",str(candidate)]:
                    subprocess.run(architecture_command(candidate),check=True)
                    covered[name]={"optimization":"O0_SINGLE_PROBE","object":str(candidate),"object_sha256":digest(candidate)}
                    continue
            if time.monotonic()-started>args.max_seconds:
                report.update(status="BOUNDED_RESUME_REQUIRED",covered=covered,covered_units=len(covered))
                atomic_json(receipt,report); return
            log=directory/(Path(name).name+".log"); before=time.monotonic()
            print("PRIVATE_O0_PROBE="+Path(name).name,flush=True)
            with log.open("w") as stream: rc=run_compiler(base+["-c",str(source),"-o",str(output)],ROOT,stream,180)
            report["units"][name]={"key":key,"exit_code":rc,"elapsed_seconds":time.monotonic()-before,"log":str(log)}
            if rc:
                report.update(status="O0_PROBE_FAILED_OR_TIMED_OUT",first_failed_source=name,covered=covered,covered_units=len(covered))
                atomic_json(receipt,report); raise SystemExit(rc)
            subprocess.run(architecture_command(output),check=True)
            report["units"][name]["object_sha256"]=digest(output)
        covered[name]={"optimization":"O0_PROBE","object":str(output),"object_sha256":digest(output)}
        report.update(status="STILL_PROBING_NOT_LINKED",covered=covered,covered_units=len(covered))
        atomic_json(receipt,report)
    report.update(status="ALL_GENERATED_UNITS_HAVE_ARM64_OBJECTS_NOT_LINKED",covered=covered,covered_units=len(covered))
    atomic_json(receipt,report)
    print(f"ARM64_SOURCE_COMPATIBILITY={len(covered)}/{len(files)} MIXED_OPTIMIZATION=YES FULL_GAME_LINK=NO PRODUCTION_FIDELITY=UNVERIFIED",flush=True)

if __name__=="__main__": main()
