#!/usr/bin/env python3
"""Package the actual LOCAL full-game investigation executable; NEVER upload it.

Requires explicit experimental opt-in. This is not production strict-FP validation,
an App Store signature, or physical-device acceptance. No game assets enter the IPA.
"""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
import tempfile
import zipfile

sys.path.insert(0,str(Path(__file__).resolve().parent))
from compile_generated_audit import ROOT
from audit_private_setup import digest
from checkpoint_progress import atomic_json

def validate_link(receipt, executable):
    if not receipt.get('application_linked') or receipt.get('exit_code') != 0:
        raise ValueError('A successful full UIKit executable link is required')
    if receipt.get('generated_units') != 227 or receipt.get('native_core_units',0) < 302:
        raise ValueError('Complete generated game and native dependency attestation required')
    if digest(executable) != receipt.get('binary_sha256'):
        raise ValueError('Executable changed since successful link')

def identity_record(audit, manifest):
    if not all(audit.get(k) for k in ('iso_validated','generated_verified','converted_assets_verified')):
        raise ValueError('Verified private preparation required')
    sha=audit['iso']['sha256']
    if manifest.get('iso_sha256') != sha or not manifest.get('files_verified'):
        raise ValueError('Runtime data must match the compiled disc')
    return {'disc_sha256':sha}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--experimental-local-investigation',action='store_true',required=True)
    parser.parse_args()
    executable=ROOT/'build/RAC1Native-investigation'
    receipt=json.loads((ROOT/'build/private-application-link.json').read_text())
    validate_link(receipt,executable)
    subprocess.run(['xcrun','lipo',str(executable),'-verify_arch','arm64'],check=True)
    header=subprocess.check_output(['xcrun','otool','-hv',str(executable)],text=True)
    if 'EXECUTE' not in header or 'NOUNDEFS' not in header:
        raise RuntimeError('Real fully linked MH_EXECUTE required')
    libraries=subprocess.check_output(['xcrun','otool','-L',str(executable)],text=True)
    if any(s in libraries.lower() for s in ('opengl','glfw','@rpath')):
        raise RuntimeError('Unexpected desktop or unbundled framework dependency')
    manifest_path=ROOT/'assets/ios-runtime-assets.json'
    identity=identity_record(json.loads((ROOT/'build/private-setup-audit.json').read_text()),
                             json.loads(manifest_path.read_text()))
    identity['runtime_manifest_sha256']=digest(manifest_path)
    output=ROOT/'build/private-ios-delivery';output.mkdir(exist_ok=True)
    symbols=output/'RAC1Native.app.dSYM'
    # Run before copying/stripping: original DWARF-bearing object paths must exist.
    subprocess.run(['xcrun','dsymutil',str(executable),'-o',str(symbols)],check=True,timeout=300)
    uuid=subprocess.check_output(['xcrun','dwarfdump','--uuid',str(executable)],text=True)
    symbol_uuid=subprocess.check_output(['xcrun','dwarfdump','--uuid',str(symbols)],text=True)
    if uuid.split()[1:3] != symbol_uuid.split()[1:3]:
        raise RuntimeError('Matching ARM64 debug symbols required')
    with tempfile.TemporaryDirectory(prefix='rac1-private-ipa-',dir=ROOT/'build') as staging:
        app=Path(staging)/'Payload/RAC1Native.app';app.mkdir(parents=True)
        shutil.copy2(executable,app/'RAC1Native')
        info=plistlib.loads((ROOT/'ios/app/Info.plist').read_bytes())
        info.update(CFBundleSupportedPlatforms=['iPhoneOS'],UIDeviceFamily=[1],
                    UIRequiredDeviceCapabilities=['arm64','metal'])
        (app/'Info.plist').write_bytes(plistlib.dumps(info))
        (app/'private-input-identity.json').write_text(json.dumps(identity,sort_keys=True)+'\n')
        shutil.copy2(ROOT/'build/gs_scene.metallib',app/'default.metallib')
        provenance={'source_commit':receipt['source_commit'],'source_worktree_changed':True,
            'build_kind':'EXPERIMENTAL_LOCAL_OLD_CLANG_STRICT_FP_NOT_PRODUCTION_VALIDATED',
            'physical_device_tested':False,'renderer_complete':False,
            'privacy':'PRIVATE_GENERATED_GAME_EXECUTABLE_NEVER_PUBLICLY_UPLOAD'}
        (app/'build-evidence.json').write_text(json.dumps(provenance,indent=2)+'\n')
        ipa=output/'RAC1Native-iOS-arm64-unsigned.ipa'
        with zipfile.ZipFile(ipa,'w',compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(app.iterdir()):archive.write(path,path.relative_to(staging))
        with zipfile.ZipFile(ipa) as archive:
            if archive.testzip() is not None:raise RuntimeError('IPA archive CRC failed')
            expected={'RAC1Native','Info.plist','private-input-identity.json','default.metallib','build-evidence.json'}
            if {Path(n).name for n in archive.namelist()} != expected:
                raise RuntimeError('Unexpected IPA contents')
    report={'ipa':str(ipa),'sha256':digest(ipa),'bytes':ipa.stat().st_size,
        'executable_sha256':digest(executable),'uuid':uuid.strip(),'symbols':str(symbols),
        'source_commit':receipt['source_commit'],'source_worktree_changed':True,
        'full_executable_linked':True,'unsigned_archive_structurally_valid':True,
        'signing_required':True,'installation_validated':False,'physical_device_tested':False,
        'production_fidelity_verified':False,'privacy':'LOCAL_ONLY_NO_UPLOAD',
        'bundled_game_assets':False,'generated_game_code_in_executable':True}
    atomic_json(output/'private-package-receipt.json',report)
    (output/(ipa.name+'.sha256')).write_text(report['sha256']+'  '+ipa.name+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
