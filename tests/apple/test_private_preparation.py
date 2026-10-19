import importlib.util
from pathlib import Path
import unittest
import tempfile

spec = importlib.util.spec_from_file_location("private_prep", Path(__file__).resolve().parents[2] / "tools/ios/prepare_private_sources.py")
prep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prep)


class PrivatePreparationContracts(unittest.TestCase):
    class SetupFixture:
        def __init__(self, root):
            self.CODE_ROOT = root
            self.files = ["src/game/fixture.cpp"]
            self.current = True
            self.calls = 0
        def generated_files(self): return self.files
        def code_current(self): return self.current
        def prepare_code(self, *args): self.calls += 1

    def test_verify_existing_never_restarts_generation(self):
        with tempfile.TemporaryDirectory() as root:
            setup = self.SetupFixture(Path(root))
            path = setup.CODE_ROOT / setup.files[0]
            path.parent.mkdir(parents=True)
            path.write_text("// synthetic metadata fixture, not game code\n")
            self.assertEqual(prep.prepare_or_verify(setup, Path("unused.iso"), 1, True), setup.files)
            self.assertEqual(setup.calls, 0)
            setup.current = False
            with self.assertRaises(RuntimeError):
                prep.prepare_or_verify(setup, Path("unused.iso"), 1, True)
            self.assertEqual(setup.calls, 0)

    def test_matching_fingerprint_does_not_hide_missing_file(self):
        with tempfile.TemporaryDirectory() as root:
            setup = self.SetupFixture(Path(root))
            with self.assertRaises(RuntimeError): prep.verify_generated_state(setup)

    def test_invalid_generation_inventory_rejected(self):
        with tempfile.TemporaryDirectory() as root:
            setup = self.SetupFixture(Path(root))
            for files in ([], ["a", "a"], ["../a"], ["/a"]):
                setup.files = files
                with self.assertRaises(RuntimeError): prep.verify_generated_state(setup)

    def test_upstream_private_directory_exclusion(self):
        self.assertTrue(prep.private_output_ignored("# private\n/assets\n"))

    def test_comment_does_not_count_as_exclusion(self):
        self.assertFalse(prep.private_output_ignored("# assets/ must be private\n"))

    def test_ntsc_reference_metadata(self):
        prep.validate_disc_identity("SCUS_971.99", 4214095872)

    def test_no_pal_substitution(self):
        with self.assertRaises(ValueError):
            prep.validate_disc_identity("SCES_509.16", 4213997568)

    def test_truncated_image_rejected(self):
        with self.assertRaises(ValueError):
            prep.validate_disc_identity("SCUS_971.99", 4214095871)

    def test_wrong_game_rejected(self):
        with self.assertRaises(ValueError):
            prep.validate_disc_identity("SCUS_000.00", 4214095872)

    def test_immutable_source_identity_required(self):
        with self.assertRaises(ValueError):
            prep.validate_revision("main")
        self.assertEqual(prep.validate_revision("3fd14c12e4dff2fd4b6661a6e54187faea3f76c8"), "3fd14c12e4dff2fd4b6661a6e54187faea3f76c8")


if __name__ == "__main__":
    unittest.main()
