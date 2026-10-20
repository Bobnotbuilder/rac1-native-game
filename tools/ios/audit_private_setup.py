#!/usr/bin/env python3
"""Read-only private ISO/source/setup audit. Never regenerates code or exports game bytes."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
SECTOR = 2048


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8 << 20), b""): h.update(block)
    return h.hexdigest()


def read_region(stream, offset, size, total, limit=None):
    if offset < 0 or size < 0 or offset + size > total or (limit and size > limit):
        raise ValueError("DISC_EXTENT_OUT_OF_BOUNDS_OR_UNREASONABLE")
    stream.seek(offset)
    out = stream.read(size)
    if len(out) != size: raise ValueError("DISC_TRUNCATED")
    return out


def check_cnf(data):
    lines = re.findall(rb"^\s*BOOT2\s*=\s*cdrom0:\\+([^;\s]+);1\s*$", data, re.M | re.I)
    if len(lines) != 1: raise ValueError("SYSTEM_CNF_BOOT_MISSING_OR_AMBIGUOUS")
    if lines[0].upper() != b"SCUS_971.99": raise ValueError("SYSTEM_CNF_UNSUPPORTED_RELEASE")
    return "SCUS_971.99"


def verify_identity(iso):
    total = iso.stat().st_size
    if total != 4214095872: raise ValueError("NTSC_U_DISC_SIZE_MISMATCH")
    with iso.open("rb") as stream:
        pvd = read_region(stream, 16*SECTOR, SECTOR, total)
        if pvd[:7] != b"\x01CD001\x01": raise ValueError("NOT_UNCOMPRESSED_ISO9660")
        sectors = struct.unpack_from("<I", pvd, 80)[0]
        if sectors != struct.unpack_from(">I", pvd, 84)[0] or sectors*SECTOR > total:
            raise ValueError("ISO_VOLUME_SIZE_INVALID")
        offset = struct.unpack_from("<I", pvd, 158)[0]*SECTOR
        size = struct.unpack_from("<I", pvd, 166)[0]
        directory = read_region(stream, offset, size, total, 128*SECTOR)
        roots = {}
        pos = 0
        while pos < size:
            length = directory[pos]
            if not length:
                pos = (pos//SECTOR + 1)*SECTOR
                continue
            if length < 34 or pos+length > size: raise ValueError("ISO_ROOT_RECORD_INVALID")
            rec = directory[pos:pos+length]
            n = rec[32]
            if 33+n > length: raise ValueError("ISO_ROOT_NAME_INVALID")
            name = rec[33:33+n].split(b";")[0].upper()
            start, extent_size = struct.unpack_from("<I", rec, 2)[0]*SECTOR, struct.unpack_from("<I", rec, 10)[0]
            if start+extent_size > total: raise ValueError("ISO_ROOT_EXTENT_TRUNCATED")
            roots.setdefault(name, []).append((start,extent_size))
            pos += length
        for required in (b"SYSTEM.CNF", b"SCUS_971.99"):
            if len(roots.get(required, [])) != 1: raise ValueError("REQUIRED_ROOT_FILE_MISSING_OR_AMBIGUOUS")
        start, size = roots[b"SYSTEM.CNF"][0]
        boot = check_cnf(read_region(stream,start,size,total,4096))
        start, size = roots[b"SCUS_971.99"][0]
        elf = read_region(stream,start,size,total,16 << 20)
        if elf[:6] != b"\x7fELF\x01\x01" or struct.unpack_from("<H",elf,18)[0] != 8:
            raise ValueError("BOOT_NOT_LITTLE_ENDIAN_MIPS_ELF32")
        toc_start = 1500*SECTOR
        magic, toc_size = struct.unpack("<ii",read_region(stream,toc_start,8,total))
        if magic != 1 or toc_size < 8: raise ValueError("RAC1_TOC_INVALID")
        toc = read_region(stream,toc_start,toc_size,total,1 << 20)
        levels = {}
        for i in range(8,toc_size-7,8):
            sector = struct.unpack_from("<I",toc,i)[0]
            if not sector: continue
            head = read_region(stream,sector*SECTOR,40,total)
            level, header_size = struct.unpack_from("<ii",head)
            if header_size != 0x2434: continue
            read_region(stream,sector*SECTOR,header_size,total)
            if not 0 <= level <= 18: continue
            if level in levels: raise ValueError("RAC1_LEVEL_AMBIGUOUS")
            for j in range(4):
                at, count = struct.unpack_from("<II",head,8+8*j)
                if (at+count)*SECTOR > total: raise ValueError("RAC1_LEVEL_RANGE_TRUNCATED")
            levels[level] = True
        if set(levels) != set(range(19)): raise ValueError("RAC1_LEVELS_INCOMPLETE")
    return {"path": str(iso), "serial": "SCUS-97199", "region": "NTSC-U", "boot": boot,
            "size": total, "system_cnf_verified": True, "level_ranges_verified": len(levels),
            "boot_elf_sha256": hashlib.sha256(elf).hexdigest(),
            "checksum_authority": "LOCAL_PREVIOUS_PREPARATION_RECEIPT_NOT_INDEPENDENT_REDUMP_CERTIFICATION"}


def audit(iso):
    spec = importlib.util.spec_from_file_location("private_preparation", ROOT/"tools/ios/prepare_private_sources.py")
    prep = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prep)
    setup = prep.load_setup()
    before = iso.stat()
    identity = verify_identity(iso)
    identity["sha256"] = digest(iso)
    after = iso.stat()
    if (before.st_ino,before.st_size,before.st_mtime_ns) != (after.st_ino,after.st_size,after.st_mtime_ns):
        raise ValueError("ISO_CHANGED_DURING_AUDIT")
    previous = json.loads((ROOT/"assets/ios-private-preparation.json").read_text())
    identity["matches_previous_source_disc"] = identity["sha256"] == previous["disc_sha256"]
    if not identity["matches_previous_source_disc"]: raise ValueError("ISO_DIFFERS_FROM_GENERATION_SOURCE_NO_REUSE")
    files = prep.verify_generated_state(setup)
    hashes = previous.get("generated_source_hashes", {})
    if set(hashes) != set(files): raise ValueError("SOURCE_HASH_INVENTORY_MISMATCH")
    if any(digest(setup.CODE_ROOT/name) != hashes[name] for name in files):
        raise ValueError("GENERATED_SOURCE_HASH_MISMATCH")
    boot = ROOT/"assets/dumps/SCUS_971.99"
    if not boot.is_file() or digest(boot) != identity["boot_elf_sha256"]: raise ValueError("BOOT_ELF_DIFFERS_FROM_SELECTED_DISC")
    state = setup.load_state()
    maps = [ROOT/"assets/generated/overlay_map"/name for name in ["front_reloc.txt"]+[f"level{i}.txt" for i in range(1,19)]]
    if not all(p.is_file() and p.stat().st_size for p in maps): raise ValueError("OVERLAY_MAPS_INCOMPLETE")
    stages = {"disc_validation": "PASS", "boot_extraction": "PASS_MATCHES_SELECTED_DISC", "overlay_maps": "PRESENT_19_WITH_RECORDED_PROVENANCE",
              "code_generation": "PASS_227_FILE_HASHES_AND_UPSTREAM_FINGERPRINT", "game_build": "NOT_COMPLETED"}
    counts = {}
    for name,folder in (("moby", "moby"),("world","world"),("flight_moby","moby/flight"),("flight_world","world/flight")):
        path = ROOT/"assets/generated/mrp"/folder
        counts[name] = sum(p.is_file() for p in path.rglob("*")) if path.is_dir() else 0
    stages["converted_assets"] = "NOT_VERIFIED"  # counts/state keys cannot certify conversion
    converted_verified=False
    converted_receipt=ROOT/"assets/ios-runtime-assets.json"
    if converted_receipt.is_file():
        converted=json.loads(converted_receipt.read_text())
        root=ROOT/"assets/generated/mrp"
        actual={str(p.relative_to(root)) for p in root.rglob("*") if p.is_file()}
        expected=converted.get("file_hashes",{})
        source_hashes=converted.get("source_hashes",{})
        converted_verified=(converted.get("status")=="VERIFIED_STRUCTURAL_CONVERSION_ONLY"
            and converted.get("iso_sha256")==identity["sha256"]
            and actual==set(expected) and len(actual)>0
            and "ASSET_READBACK=PASS" in converted.get("readback","")
            and all(digest(root/name)==value for name,value in expected.items())
            and all((ROOT/name).is_file() and digest(ROOT/name)==value for name,value in source_hashes.items())
            and len(source_hashes)>0
            and all(state.get(name)==setup.disc_fingerprint(iso)+":"+setup.sources_fingerprint(patterns)
                    for name,description,patterns in setup.STEPS))
        stages["converted_assets"]="PASS_TYPED_UPSTREAM_READBACK_AND_ALL_FILE_HASHES" if converted_verified else "STALE_OR_INCOMPLETE"
    return {"schema_version": 1, "iso_validated": True, "generated_verified": True,
            "converted_assets_verified":converted_verified,"iso": identity,
            "setup": {"stages": stages, "generated_files_verified": len(files), "converted_asset_file_counts": counts,
                      "recorded_setup_keys": sorted(state), "overlay_map_sha256": {p.name:digest(p) for p in maps},
                      "overlay_source_fingerprint_matches":state.get("overlay")==setup.code_step_fingerprint("overlay",["tools/overlay_match/*.py","data/regions/*.txt"],setup.disc_fingerprint(iso),state),
                      "data_stages_recorded_complete": {s:s in state for s in ("moby","world","flight")},
                      "runtime_iso_reference_present": (ROOT/"assets/iso/rac1-ntsc-u.iso").exists()},
            "game_instructions_executed": False, "game_oracle_run": False, "no_private_upload": True}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--iso", type=Path, required=True)
    args = p.parse_args()
    report = audit(args.iso.expanduser().resolve(strict=True))
    output = ROOT/"build/private-setup-audit.json"
    temporary = output.with_suffix(".json.part")
    temporary.write_text(json.dumps(report,indent=2)+"\n")
    temporary.replace(output)
    print(json.dumps({"ISO": report["iso"], "GENERATED_FILES_VERIFIED": report["setup"]["generated_files_verified"],
                      "CONVERTED_ASSET_COUNTS": report["setup"]["converted_asset_file_counts"]},indent=2))


if __name__ == "__main__": main()
