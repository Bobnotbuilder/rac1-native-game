import importlib.util
from pathlib import Path
import io
import os
import subprocess
import sys
import tempfile
import time
import unittest

ROOT=Path(__file__).resolve().parents[2]
def module(name):
    spec=importlib.util.spec_from_file_location(name,ROOT/"tools/ios"/(name+".py"))
    result=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result
audit=module("audit_private_setup")
progress=module("checkpoint_progress")
compiler=module("compile_generated_audit")
finalize=module("finalize_private_assets")

class AuditContracts(unittest.TestCase):
    def test_exact_system_cnf(self):
        self.assertEqual(audit.check_cnf(b"BOOT2 = cdrom0:\\SCUS_971.99;1\r\nVER = 1.00\r\n"),"SCUS_971.99")
    def test_other_region_rejected(self):
        for serial in (b"SCES_509.16", b"SCPS_150.55", b"SCUS_000.00"):
            with self.assertRaises(ValueError): audit.check_cnf(b"BOOT2 = cdrom0:\\"+serial+b";1\n")
    def test_ambiguous_boot_rejected(self):
        with self.assertRaises(ValueError): audit.check_cnf(b"BOOT2 = cdrom0:\\SCUS_971.99;1\n"*2)
    def test_missing_boot_rejected(self):
        with self.assertRaises(ValueError): audit.check_cnf(b"SCUS_971.99 is only a filename")
    def test_out_of_bounds_rejected(self):
        for offset,size,total in ((-1,1,1),(0,2,1),(1,-1,1)):
            with self.assertRaises(ValueError): audit.read_region(io.BytesIO(b"x"),offset,size,total)
    def test_truncated_region_rejected(self):
        with self.assertRaises(ValueError): audit.read_region(io.BytesIO(b"x"),0,2,2)
    def test_fixed_delivery_score_denominator(self):
        self.assertEqual(sum(progress.GATES.values()),100)
        self.assertEqual(progress.delivery_score({}),0)
        self.assertEqual(progress.delivery_score({k:{"status":"PASS"} for k in progress.GATES}),100)
    def test_partial_compile_does_not_score_full_game_link(self):
        gates={"correct_iso_validated":{"status":"PASS"},"generated_game_code_verified":{"status":"PASS"},
               "full_game_arm64_compiled_and_linked":{"status":"COMPILED_COMPONENTS_ONLY"}}
        self.assertEqual(progress.delivery_score(gates),20)
    def test_old_apple_lipo_order(self):
        self.assertEqual(compiler.architecture_command(Path("real.o")),["xcrun","lipo","real.o","-verify_arch","arm64"])
    def test_compile_cache_binds_optimization_and_header_context(self):
        with tempfile.TemporaryDirectory(prefix="rac1-cache-contract-") as folder:
            source=Path(folder)/"synthetic.cpp"; source.write_text("int synthetic;\n")
            key=compiler.unit_cache_key(source,"headers-v1","clang-version",["-O1"])
            self.assertNotEqual(key,compiler.unit_cache_key(source,"headers-v1","clang-version",["-O0"]))
            self.assertNotEqual(key,compiler.unit_cache_key(source,"headers-v2","clang-version",["-O1"]))
            source.write_text("int changed_synthetic;\n")
            self.assertNotEqual(key,compiler.unit_cache_key(source,"headers-v1","clang-version",["-O1"]))
    def test_partial_export_does_not_attest_completion(self):
        self.assertFalse(finalize.completed_export_log("world","Converting the world geometry and textures\nlevel 0 done\n"))
        self.assertFalse(finalize.completed_export_log("flight","Converting the flight between planets\nother job done\n"))
    def test_exact_controller_completion_required(self):
        levels=" ".join(str(i) for i in range(19))+" title"
        self.assertTrue(finalize.completed_export_log("world",f"[setup] [3/5] Converting the world geometry and textures, level... {levels} done\n"))
        self.assertTrue(finalize.completed_export_log("flight","[setup] [4/5] Converting the flight between planets (the loading screen)... done\n"))
    def test_export_marker_cannot_be_a_substring(self):
        self.assertTrue(finalize.completed_export_log("moby","UPSTREAM_EXPORT_STAGE=moby RESULT=PASS\n"))
        self.assertFalse(finalize.completed_export_log("moby","not: UPSTREAM_EXPORT_STAGE=moby RESULT=PASS\n"))
    @unittest.skipUnless(hasattr(os,"killpg"),"POSIX compiler process groups only")
    def test_bounded_compiler_cancels_only_its_own_job(self):
        with open(os.devnull,"w") as stream:
            self.assertEqual(compiler.run_compiler([sys.executable,"-c","pass"],ROOT,stream,5),0)
            self.assertEqual(compiler.run_compiler([sys.executable,"-c","import time; time.sleep(5)"],ROOT,stream,.05),124)
    @unittest.skipUnless(hasattr(os,"killpg"),"POSIX compiler process groups only")
    def test_timeout_cancels_driver_descendant_too(self):
        with tempfile.TemporaryDirectory(prefix="rac1-compiler-contract-") as folder:
            marker=Path(folder)/"orphan-was-left-running"
            ready=Path(folder)/"descendant-started"
            worker="import time; from pathlib import Path; time.sleep(1.5); Path("+repr(str(marker))+").touch()"
            driver="import subprocess,sys,time; from pathlib import Path; subprocess.Popen([sys.executable,'-c',"+repr(worker)+"]); Path("+repr(str(ready))+").touch(); time.sleep(5)"
            with open(os.devnull,"w") as stream:
                self.assertEqual(compiler.run_compiler([sys.executable,"-c",driver],ROOT,stream,1),124)
            self.assertTrue(ready.exists(),"Fixture must actually create its driver descendant before cancellation")
            time.sleep(1.6)
            self.assertFalse(marker.exists(),"Cancelling the driver must not leave cc1-equivalent descendants")

if __name__=="__main__": unittest.main()
