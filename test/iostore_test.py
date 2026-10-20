import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from make_extra_pak import build_store
from normalize_extra_pak import source_entry


class StoreTest(unittest.TestCase):
    def test_extended_texture_store_uses_export_counts_not_byte_size(self):
        rows = [struct.pack('<QIIIIII', 23151, 1, 1, 1938, 0, 0, 0),
                struct.pack('<QIIIIII', 6783, 1, 1, 19, 0, 0, 0)]
        header = {'pids': [11, 12], 'store': b''.join(rows)}
        entry, stride = source_entry(header, 1)
        self.assertEqual(stride, 32)
        self.assertEqual(entry, (struct.pack('<II', 1, 1), []))

    def test_native_store_preserves_package_dependencies(self):
        entries = [(struct.pack('<II', 2, 1), [0x76a4a2b737fe3821, 0x3951f821a7cbbdb1]),
                   (struct.pack('<II', 1, 1), [])]
        header = {'pids': [11, 12], 'store': build_store(entries)}
        entry, stride = source_entry(header, 0)
        self.assertEqual(stride, 16)
        self.assertEqual(entry, entries[0])

    def test_bad_export_bundle_counts_are_rejected(self):
        with self.assertRaises(ValueError):
            source_entry({'pids': [11], 'store': struct.pack('<IIII', 23151, 0, 0, 0)}, 0)


if __name__ == '__main__':
    unittest.main()
