#!/usr/bin/env python3
"""Attest successful unchanged upstream exports/readback; cache only completed DATA stages."""
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tools/ios"))
from audit_private_setup import digest
from checkpoint_progress import atomic_json

def completed_export_log(stage, text):
    """Require the actual final controller line, not an arbitrary partial 'done'."""
    marker=f"UPSTREAM_EXPORT_STAGE={stage} RESULT=PASS"
    if marker in text.splitlines(): return True
    if stage=="world":
        levels=" ".join(str(i) for i in range(19))+" title"
        expected=f"[setup] [3/5] Converting the world geometry and textures, level... {levels} done"
    elif stage=="flight":
        expected="[setup] [4/5] Converting the flight between planets (the loading screen)... done"
    else:
        return False
    return expected in text.splitlines()

def main():
    audit=json.loads((ROOT/"build/private-setup-audit.json").read_text())
    iso=Path(audit["iso"]["path"])
    executable=ROOT/"build-private-preparation/rac1-private-asset-prep"
    result=subprocess.run([str(executable),"--verify-assets"],cwd=ROOT,capture_output=True,text=True,check=True)
    match=re.search(r"ASSET_READBACK=PASS CLASSES=(\d+) TEXTURE_FILES=(\d+) WORLDS=21 REQUIRED_GROUPS=21",result.stdout)
    if not match: raise RuntimeError("Actual upstream-reader verification missing")
    for stage in ("moby","world","flight"):
        log=(ROOT/"build"/f"mrp-{stage}-conversion.log").read_text()
        # These are the actual bounded exporter/controller runs, not fixture or packaging tests.
        if not completed_export_log(stage,log):
            raise RuntimeError(f"No successful real export evidence for {stage}")
    if digest(iso)!=audit["iso"]["sha256"]: raise RuntimeError("ISO changed during preparation")
    spec=importlib.util.spec_from_file_location("upstream_setup",ROOT/"tools/setup.py")
    setup=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(setup)
    state=setup.load_state()
    source_hashes={}
    patterns={}
    for name,description,sources in setup.STEPS:
        patterns[name]=setup.sources_fingerprint(sources)
        source_hashes.update({str(p.relative_to(ROOT)):digest(p) for p in setup.sources_files(sources)})
    files={str(p.relative_to(ROOT/"assets/generated/mrp")):digest(p) for p in sorted((ROOT/"assets/generated/mrp").rglob("*")) if p.is_file()}
    previous=ROOT/"assets/setup-state.json"
    backup=previous.with_name("setup-state.before-mrp."+digest(previous)[:16]+".json")
    if not backup.exists(): shutil.copy2(previous,backup)
    for name in ("moby","world","flight"):
        state[name]=setup.disc_fingerprint(iso)+":"+patterns[name]
    # ELF/overlay/code keys are deliberately not fabricated or overwritten.
    setup.save_state(state)
    receipt={"schema_version":1,"status":"VERIFIED_STRUCTURAL_CONVERSION_ONLY", "iso_sha256":audit["iso"]["sha256"],
        "source_commit_baseline":"78ffc48b63a082bcd6a57f929a367fb1699eb2b1", "local_source_changes":True,
        "exporter_sha256":digest(executable),"source_hashes":source_hashes,
        "data_stage_source_fingerprints":patterns,"readback":result.stdout.strip(),"file_hashes":files,
        "files_verified":len(files),"original_setup_controller_used":True,
        "game_rendered":False,"private_upload":False,"privacy":"LOCAL_ONLY"}
    atomic_json(ROOT/"assets/ios-runtime-assets.json",receipt)
    print(result.stdout.strip())
    print(f"PRIVATE_RUNTIME_ASSET_FILES_HASHED={len(files)} DATA_STAGES_CACHED=moby,world,flight")

if __name__=="__main__": main()
