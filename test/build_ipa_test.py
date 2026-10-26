"""Checks on the patch table of tools/build_ipa.py: the IPA builder leaves the game's own capitals for
player names alone."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import build_ipa  # noqa: E402

# menuutility::AllCapsUtility::ConvertAllCaps in PESmobile 11.1.101: its first instruction, and the
# first instruction of the function after it (from the executable's list of function starts).
CONVERT_ALL_CAPS = 0x1046D2E0C
CONVERT_ALL_CAPS_END = 0x1046D3154


class ConvertAllCapsTest(unittest.TestCase):
    """Menus that show a player's name in capitals keep Konami's own function."""

    def test_no_patch_inside_convert_all_caps(self):
        inside = [f"{vm:#x} {desc}" for vm, _old, _new, desc in build_ipa.PATCHES
                  if CONVERT_ALL_CAPS <= vm < CONVERT_ALL_CAPS_END]
        self.assertEqual(inside, [], "ConvertAllCaps must run as Konami wrote it (no Title Case rewrite)")


if __name__ == "__main__":
    unittest.main()
