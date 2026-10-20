#!/usr/bin/env python3
"""Local-only Veldin overlay input for a bounded original-code math oracle. No code generation."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso",type=Path,required=True)
    args=parser.parse_args()
    report=json.loads((ROOT/"build/private-setup-audit.json").read_text())
    if str(args.iso.resolve()) != report["iso"]["path"]: raise ValueError("Use the previously audited ISO only")
    sys.path.insert(0,str(ROOT/"tools/ios"))
    from audit_private_setup import digest
    if digest(args.iso)!=report["iso"]["sha256"]: raise ValueError("ISO changed since audit")
    spec=importlib.util.spec_from_file_location("original_overlay_reader",ROOT/"tools/overlay_match/overlay_match.py")
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with args.iso.open("rb") as image: sections,entry=module.level_overlay(image,0)
    loaded=[(address,content) for address,kind,content in sections if kind==1 and content]
    if not loaded or any(address<1 or address+len(content)>0x04000000 for address,content in loaded):
        raise ValueError("Original overlay address range invalid")
    output=ROOT/"assets/dumps/ios-fp-oracle-overlay.bin"
    part=output.with_suffix(".bin.part")
    with part.open("wb") as stream:
        stream.write(b"R1FPORC1"+struct.pack("<I",len(loaded)))
        for address,content in loaded: stream.write(struct.pack("<II",address,len(content))+content)
    part.replace(output)
    receipt={"iso_sha256":report["iso"]["sha256"],"serial":"SCUS-97199","level":0,
             "overlay_sha256":digest(output),"sections":len(loaded),"entry":entry,"privacy":"LOCAL_ONLY"}
    output.with_suffix(".json").write_text(json.dumps(receipt,indent=2)+"\n")
    print("PRIVATE_ORIGINAL_MATH_ORACLE_INPUT=READY (no game bytes exported to public services)")
if __name__=="__main__": main()
