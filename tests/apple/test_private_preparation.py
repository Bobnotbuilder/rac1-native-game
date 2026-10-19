import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("private_prep", Path(__file__).resolve().parents[2] / "tools/ios/prepare_private_sources.py")
prep = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prep)


class PrivatePreparationContracts(unittest.TestCase):
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
