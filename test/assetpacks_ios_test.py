import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'tools'))
from port_assetpacks_ios import native_header
from make_extra_pak import parse_header, store_entry


class NativeStoreTests(unittest.TestCase):
    def fixture(self):
        return {'cid': 19, 'pids': [13, 17], 'names': b'', 'hashes': b'\0' * 8,
                'store': struct.pack('<Q6I', 700, 1, 1, 8, 0, 0, 0) + struct.pack('<Q6I', 800, 1, 1, 4, 0, 0, 0)}

    def test_native_layout_preserves_package_identity(self):
        result = parse_header(native_header(self.fixture(), {13: 700, 17: 800}))
        self.assertEqual(result['cid'], 19)
        self.assertEqual(result['pids'], [13, 17])
        self.assertEqual(len(result['store']), 32)
        for i in range(2):
            fixed, imports = store_entry(result['store'], i)
            self.assertEqual(struct.unpack('<II', fixed), (1, 1))
            self.assertEqual(imports, [])

    def test_size_mismatch_does_not_silently_reinterpret_another_layout(self):
        with self.assertRaises(ValueError):
            native_header(self.fixture(), {13: 701, 17: 800})

    def test_dependencies_require_explicit_conversion(self):
        h = self.fixture()
        h['store'] = struct.pack('<Q6I', 700, 1, 1, 8, 0, 1, 20) + h['store'][32:]
        with self.assertRaises(ValueError):
            native_header(h, {13: 700, 17: 800})

    def test_duplicate_order_rejected(self):
        h = self.fixture()
        h['store'] = h['store'][:32] + struct.pack('<Q6I', 800, 1, 1, 8, 0, 0, 0)
        with self.assertRaises(ValueError):
            native_header(h, {13: 700, 17: 800})


if __name__ == '__main__':
    unittest.main()
