import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

MODULE = Path(__file__).resolve().parents[1] / 'ios/efootxi/efootxi_ios.py'


class BridgeTests(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location('bridge', MODULE)
        self.bridge = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.bridge)
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.bridge._bundle = root / 'bundle'
        self.bridge._session = root / 'logs'
        (root / 'logs/requests').mkdir(parents=True)
        (root / 'bundle/livedata').mkdir(parents=True)

    def test_real_resource_bytes_and_missing_file_logged(self):
        expected = bytes(range(256)) * 100
        (self.bridge._bundle / 'livedata/test.utoc').write_bytes(expected)
        code, _, actual = self.bridge.serve('GET', '/livedata/test.utoc', b'')
        self.assertEqual(code, 200)
        self.assertEqual(actual, expected)
        self.assertEqual((self.bridge._session / 'requests/000001.response.bin').read_bytes(), expected)
        # No gateway needed for a missing path; the missing resource must be logged.
        self.bridge._gateway = type('Gateway', (), {'_root': self.bridge._bundle})()
        self.assertEqual(self.bridge.serve('GET', '/livedata/missing.utoc', b'')[0], 404)
        self.assertIn('missing.utoc', (self.bridge._session / 'livedata.jsonl').read_text())

    def test_traversal_cannot_expose_saved_account(self):
        (self.bridge._bundle / 'secret').write_bytes(b'private')
        for path in ('/livedata/../secret', '/livedata/%2e%2e/secret', '/livedata//etc/passwd'):
            self.assertEqual(self.bridge.serve('GET', path, b'')[0], 403)

    def test_exception_preserves_wire_and_traceback(self):
        # Importing the absent private codec fails, but the complete body survives.
        body = b'\x00\xff' * 100000
        with self.assertRaises(Exception):
            self.bridge.serve('POST', '/gate/bad.php', body)
        folder = self.bridge._session / 'requests'
        self.assertEqual((folder / '000001.request.bin').read_bytes(), body)
        self.assertIn('Traceback', (folder / '000001.exception.txt').read_text())

    def test_manifest_uses_native_hashes_sizes_and_all_container_parts(self):
        records = {}
        for i, ext in enumerate(('pak', 'ucas', 'utoc')):
            records['pc9257_mobile_ios.' + ext] = dict(size=100 + i, sha256=str(i) * 64)
        records['dt870_mobile_ios.cpk'] = dict(size=999, sha256='a' * 64)
        self.bridge._native_live = records
        result = self.bridge._manifest({'login': {'livedata_list': [{'name': 'android.pak'}],
                                                'version_file': json.dumps({'version': 1, 'targetAppVersion': '6.1.0'})}})['login']
        version = json.loads(result['version_file'])
        self.assertEqual(len(result['livedata_list']), 4)
        self.assertEqual(version['dt880'][0]['ucas'], '1' * 64)
        self.assertEqual(version['dt880'][0]['utocFileSize'], 102)
        self.assertEqual(version['dt870FileSize'], 999)
        self.assertEqual(version['targetAppVersion'], '6.1.0')
        self.assertTrue(all('_ios.' in v['name'] for v in result['livedata_list']))

    def test_binary_diagnostic_json_is_lossless(self):
        self.bridge.event('binary.jsonl', payload=b'\x00\xff')
        import base64
        value = json.loads((self.bridge._session / 'binary.jsonl').read_text())
        self.assertEqual(base64.b64decode(value['payload']['$bytes_base64']), b'\x00\xff')


if __name__ == '__main__':
    unittest.main()
