#!/usr/bin/env python3
"""Verify a pinned public dependency cache against its exact Git blob inventory.

An optional bulk mechanical import fix removes ONLY an added final LF when doing so
recovers BOTH the original byte count and Git blob hash. No other changes are accepted.
Never executes downloaded scripts, installs packages or accesses game inputs.
"""
import argparse
import hashlib
import json
from pathlib import Path

def blob_hash(data):
    return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()

def checked_bytes(data,expected_size,expected_sha,repair_import=False):
    if len(data)==expected_size and blob_hash(data)==expected_sha: return data
    if repair_import and len(data)==expected_size+1 and data.endswith(b'\n'):
        original=data[:-1]
        if blob_hash(original)==expected_sha: return original
    raise ValueError('DEPENDENCY_CONTENT_DOES_NOT_MATCH_PINNED_GIT_BLOB')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('cache',type=Path)
    p.add_argument('--repair-import-final-lf',action='store_true')
    args=p.parse_args()
    root=args.cache.resolve(strict=True)
    manifest=json.loads((root/'provenance.json').read_text())
    pending=[]
    for item in manifest['files']:
        path=(root/item['path']).resolve(strict=True)
        if root not in path.parents or path.is_symlink(): raise ValueError('UNSAFE_CACHE_PATH')
        data=path.read_bytes()
        original=checked_bytes(data,item['size'],item['sha'],args.repair_import_final_lf)
        pending.append((path,data,original))
    # Validate every file BEFORE performing the strictly content-attested mechanical repair.
    for path,data,original in pending:
        if data!=original: path.write_bytes(original)
    print('PINNED_DEPENDENCY_BLOBS_VERIFIED='+str(len(pending)))

if __name__=='__main__': main()
