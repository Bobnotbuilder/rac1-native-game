import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
def load(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools/ios'/(name+'.py'))
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
cache=load('verify_dependency_cache');core=load('compile_native_core')
class IosIntegrationContracts(unittest.TestCase):
    def test_exact_pinned_header(self):
        original=b'// synthetic public header\n'
        self.assertEqual(cache.checked_bytes(original,len(original),cache.blob_hash(original)),original)
    def test_import_lf_repair_requires_original_hash(self):
        original=b'// fixture\n'
        self.assertEqual(cache.checked_bytes(original+b'\n',len(original),cache.blob_hash(original),True),original)
        with self.assertRaises(ValueError):cache.checked_bytes(original+b'\n',len(original),'0'*40,True)
    def test_mutated_public_dependency_never_used(self):
        data=b'original'
        with self.assertRaises(ValueError):cache.checked_bytes(b'replaced',len(data),cache.blob_hash(data),True)
    def test_extra_lf_rejected_without_explicit_import_fix(self):
        data=b'fixture'
        with self.assertRaises(ValueError):cache.checked_bytes(data+b'\n',len(data),cache.blob_hash(data))
    def test_hpp_change_invalidates_native_object_context(self):
        with tempfile.TemporaryDirectory(prefix='rac1-hpp-cache-') as folder:
            source=Path(folder)/'source.cpp';header=Path(folder)/'clock.hpp'
            source.write_text('#include "clock.hpp"\n');header.write_text('first')
            first=core.direct_hpp_context(source);header.write_text('second')
            self.assertNotEqual(first,core.direct_hpp_context(source))
    def test_inventory_preserves_original_game_and_no_new_interpreter_fallback(self):
        paths=core.inventory()
        self.assertIn('src/game/boot/front_end.cpp',paths)
        self.assertIn('src/game/ovl/level_change.cpp',paths)
        self.assertIn('runtime/iop/snd989_iop.cpp',paths)
        self.assertNotIn('runtime/ee/ee_interpreter.cpp',paths)
        self.assertEqual(len(paths),len(set(paths)))
if __name__=='__main__':unittest.main()
