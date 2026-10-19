#!/usr/bin/env python3
"""Private, disc-only code generation using UPSTREAM generators; no desktop GL/compiler required.

Never upload assets/, generated game C++, this receipt, or the ISO to the public fork/CI.
This does not export MRP assets, link the game or claim a playable iOS build.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
BOOT = "SCUS_971.99"
DISC_SIZE = 4214095872


def validate_disc_identity(boot, size):
    if boot != BOOT:
        raise ValueError("This first iOS validation requires NTSC-U SCUS-97199; no region substitution")
    if size != DISC_SIZE:
        raise ValueError("Disc size differs from the upstream NTSC-U reference; full validation required")


def validate_revision(revision):
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("Provide the exact 40-character source commit, not a branch or invented revision")
    return revision


def private_output_ignored(text):
    # Upstream deliberately uses /assets (without trailing slash) to ignore symlinks too.
    return any(line.strip() in {"/assets", "/assets/", "assets", "assets/"} for line in text.splitlines())


def load_setup():
    spec = importlib.util.spec_from_file_location("rac1_private_upstream_setup", ROOT / "tools/setup.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def verify_generated_state(setup):
    files = setup.generated_files()
    if not files or len(files) != len(set(files)):
        raise RuntimeError("Empty or duplicate upstream generated-source inventory")
    for name in files:
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts:
            raise RuntimeError("Invalid upstream generated-source path")
        if not (setup.CODE_ROOT / relative).is_file():
            raise RuntimeError("Incomplete generated-source inventory; do not compile a partial game")
    if not setup.code_current():
        raise RuntimeError("Upstream generated-source fingerprint verification failed")
    return files


def prepare_or_verify(setup, iso, jobs, verify_existing):
    # A completed run can be attested at a later integration revision without restarting codegen.
    # Never rewrite the upstream manifest/fingerprint to make an out-of-date generation pass.
    if not verify_existing:
        setup.prepare_code(iso, set(), jobs)
    return verify_generated_state(setup)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--verify-existing", action="store_true",
                        help="Verify the complete current upstream generation; NEVER rerun generators")
    # Four-GB Intel host: deliberately one compiler/generator job unless the caller opts in.
    parser.add_argument("--jobs", type=int, default=1, choices=range(1, 5))
    args = parser.parse_args()
    revision = validate_revision(args.source_revision)
    iso = args.iso.expanduser().resolve(strict=True)
    setup = load_setup()
    validate_disc_identity(setup.looks_like_ps2_disc(iso), iso.stat().st_size)
    if not private_output_ignored((ROOT / ".gitignore").read_text()):
        raise ValueError("Missing upstream assets exclusion; refusing private preparation")
    digest = hashlib.sha256()
    with iso.open("rb") as stream:
        for block in iter(lambda: stream.read(8 << 20), b""):
            digest.update(block)
    # Reuse upstream extraction, map matching, generators and input fingerprint; no new decompiler.
    os.environ["RAC1_ISO"] = str(iso)
    generated = prepare_or_verify(setup, iso, args.jobs, args.verify_existing)
    hashes = {}
    for name in generated:
        content_hash = hashlib.sha256()
        with (setup.CODE_ROOT / name).open("rb") as stream:
            for block in iter(lambda: stream.read(1 << 20), b""):
                content_hash.update(block)
        hashes[name] = content_hash.hexdigest()
    receipt = {
        "schema_version": 1, "source_revision": revision, "disc_serial": "SCUS-97199",
        "preparation_script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "disc_size": iso.stat().st_size, "disc_sha256": digest.hexdigest(),
        "codegen_fingerprint": setup.code_sources_fingerprint(),
        "generated_files": len(generated), "jobs": args.jobs,
        "verification_only": args.verify_existing,
        "generated_source_hashes": hashes,
        "generated_sources_current": True,
        "game_linked": False, "oracle_run": False, "ios_playable": False,
        "privacy": "LOCAL_ONLY_DO_NOT_UPLOAD",
    }
    output = ROOT / "assets/ios-private-preparation.json"
    if output.exists():
        previous = hashlib.sha256(output.read_bytes()).hexdigest()[:16]
        backup = output.with_name(f"ios-private-preparation.{previous}.json")
        if not backup.exists():
            shutil.copy2(output, backup)
    temporary = output.with_suffix(".json.part")
    temporary.write_text(json.dumps(receipt, indent=2) + "\n")
    temporary.replace(output)
    print("PRIVATE_PREPARATION=PASS (generated sources only; not a game build)")
    print("LOCAL_RECEIPT=assets/ios-private-preparation.json (do not publish)")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError) as error:
        print(f"PRIVATE_PREPARATION=FAIL: {error}", file=sys.stderr)
        sys.exit(1)
