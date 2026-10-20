#!/usr/bin/env python3
"""Incremental PRIVATE ARM64 translation-unit investigation; never an app/link/fidelity pass.

Clang 13's experimental constrained FP path is opt-in and remains NOT RELEASE VALIDATED.
This script never uploads sources/objects and never permits unresolved-symbol fake linking.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/ios"))
from audit_private_setup import digest
from checkpoint_progress import atomic_json

def architecture_command(path):
    # Apple's older lipo consumes trailing arguments after -verify_arch as architectures.
    return ["xcrun","lipo",str(path),"-verify_arch","arm64"]

def source_context_fingerprint(setup):
    header_hashes={str(p.relative_to(ROOT)):digest(p) for folder in ("src","runtime","ios")
                   for p in (ROOT/folder).rglob("*.h")}
    return setup.code_sources_fingerprint()+hashlib.sha256(json.dumps(header_hashes,sort_keys=True).encode()).hexdigest()

def unit_cache_key(source, fingerprint, version, command_prefix):
    return hashlib.sha256((digest(source)+fingerprint+version+json.dumps(command_prefix)).encode()).hexdigest()

def run_compiler(command, cwd, stream, timeout):
    # A compiler driver may spawn cc1. A bounded timeout must not leave that child running.
    # Only signal the fresh process group CREATED HERE, never a discovered/pre-existing PID.
    child=subprocess.Popen(command,cwd=cwd,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
    try:
        return child.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        try: os.killpg(child.pid,signal.SIGTERM)
        except ProcessLookupError: pass
        try: child.wait(timeout=5)
        except subprocess.TimeoutExpired:
            try: os.killpg(child.pid,signal.SIGKILL)
            except ProcessLookupError: pass
            child.wait(timeout=5)
        return 124

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--experimental-clang13-investigation",action="store_true")
    p.add_argument("--max-seconds",type=int,default=900)
    p.add_argument("--unit-timeout-seconds",type=int,default=300)
    args=p.parse_args()
    if not 1<=args.max_seconds<=3600 or not 1<=args.unit_timeout_seconds<=900:
        p.error("Use a bounded batch (1..3600 seconds) and unit timeout (1..900 seconds)")
    spec=importlib.util.spec_from_file_location("private_preparation",ROOT/"tools/ios/prepare_private_sources.py")
    prep=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prep)
    setup=prep.load_setup()
    files=prep.verify_generated_state(setup)
    version=subprocess.check_output(["xcrun","--sdk","iphoneos","clang++","--version"],text=True)
    base=["xcrun","--sdk","iphoneos","clang++","-target","arm64-apple-ios15.0","-std=c++20",
          "-O1","-g","-frounding-math","-ffp-contract=off","-fno-strict-aliasing",
          "-Werror=unsupported-floating-point-opt","-I"+str(ROOT/"ios/simd"),"-I"+str(ROOT),"-I"+str(ROOT/"src")]
    if args.experimental_clang13_investigation:
        base += ["-ffp-exception-behavior=strict","-Xclang","-fexperimental-strict-floating-point"]
    directory=ROOT/"build-private-arm64-generated"
    directory.mkdir(exist_ok=True)
    receipt=directory/"compilation-receipt.json"
    report=json.loads(receipt.read_text()) if receipt.exists() else {"units":{}}
    report.update(compiler=version,command_prefix=base,expected_units=len(files),
                  experimental_investigation=args.experimental_clang13_investigation,
                  production_fidelity_verified=False,full_game_linked=False,ipa_created=False,
                  privacy="LOCAL_ONLY_DO_NOT_UPLOAD",source_commit_baseline="78ffc48b63a082bcd6a57f929a367fb1699eb2b1")
    fingerprint=source_context_fingerprint(setup)
    started=time.monotonic()
    for i,name in enumerate(files):
        source=setup.CODE_ROOT/name
        key=unit_cache_key(source,fingerprint,version,base)
        output=directory/(Path(name).name+".o")
        old=report["units"].get(name,{})
        if old.get("key")==key and output.is_file() and digest(output)==old.get("object_sha256"):
            continue
        if time.monotonic()-started>args.max_seconds:
            report["status"]="BOUNDED_BATCH_COMPLETE_MORE_UNITS_REMAIN"
            atomic_json(receipt,report)
            print("PRIVATE_ARM64_COMPILE_BATCH=BOUNDED_RESUME_REQUIRED",flush=True)
            return
        log=directory/(Path(name).name+".log")
        part=output.with_suffix(".o.part")
        cmd=base+["-c",str(source),"-o",str(part)]
        before=time.monotonic()
        print(f"PRIVATE_ARM64_UNIT={i+1}/{len(files)} SOURCE={Path(name).name}",flush=True)
        with log.open("w") as stream:
            rc=run_compiler(cmd,ROOT,stream,args.unit_timeout_seconds)
        report["units"][name]={"key":key,"exit_code":rc,"elapsed_seconds":time.monotonic()-before,"log":str(log)}
        if rc:
            report.update(status="BOUNDED_UNIT_COMPILE_TIMEOUT" if rc==124 else "FIRST_SOURCE_COMPILATION_FAILURE",
                          first_failed_source=name)
            atomic_json(receipt,report)
            print(f"PRIVATE_ARM64_UNIT_FAILED={Path(name).name} EXIT={rc} (see local log)",flush=True)
            sys.exit(1)
        report["units"][name]["pending_object_sha256"]=digest(part)
        report["status"]="ARCHITECTURE_VERIFICATION_PENDING"
        atomic_json(receipt,report)
        subprocess.run(architecture_command(part),check=True)
        part.replace(output)
        report["units"][name]["object_sha256"]=digest(output)
        report["compiled_units"]=sum(u.get("exit_code")==0 for u in report["units"].values())
        report["status"]="STILL_COMPILING_NOT_LINKED"
        atomic_json(receipt,report)
    report.update(status="ALL_GENERATED_UNITS_COMPILED_NOT_LINKED",compiled_units=len(files))
    atomic_json(receipt,report)
    print(f"PRIVATE_ARM64_GENERATED_COMPILE={len(files)}/{len(files)} FULL_GAME_LINK=NO PRODUCTION_FIDELITY=UNVERIFIED",flush=True)

if __name__=="__main__": main()
