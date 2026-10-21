import hashlib
import http.server
import importlib.util
import json
from pathlib import Path
import tempfile
import threading
import unittest
import zipfile

MODULE = Path(__file__).resolve().parents[1] / 'ios/efootxi/efootxi_resources.py'
spec = importlib.util.spec_from_file_location('resources', MODULE)
resources = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resources)


class ResourcesTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.events = []
        self.emit = lambda kind, **v: self.events.append((kind, v))

    def test_resume_range_and_reject_corrupt_download(self):
        data = bytes(range(256)) * 1000
        class Handler(http.server.BaseHTTPRequestHandler):
            def log_message(self, *args): pass
            def do_GET(self):
                start = int(self.headers.get('Range', 'bytes=0-')[6:-1])
                self.send_response(206 if start else 200)
                self.send_header('Content-Length', str(len(data)-start))
                self.send_header('Content-Range', 'bytes %d-%d/%d' % (start, len(data)-1, len(data)))
                self.end_headers()
                self.wfile.write(data[start:])
        server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        path = self.root / 'resource.zip'
        path.with_suffix('.zip.part').write_bytes(data[:12345])
        url = 'http://127.0.0.1:%d/resource.zip' % server.server_port
        resources.download(url, path, hashlib.sha256(data).hexdigest(), len(data), self.emit)
        self.assertEqual(path.read_bytes(), data)
        self.assertEqual(self.events[0][1]['downloaded'], 12345)
        other = self.root / 'corrupt.zip'
        with self.assertRaisesRegex(ValueError, 'mismatch'):
            resources.download(url, other, '0'*64, len(data), self.emit)
        self.assertFalse(other.exists())
        self.assertTrue(list(self.root.glob('corrupt.zip.part.bad-*')))

    def fixture(self, bad=False):
        bundle = self.root / 'bundle'
        (bundle / 'china').mkdir(parents=True)
        data = b'Android cooked data, unchanged; not proof of iOS compatibility'
        archive = bundle / 'china/base.zip'
        with zipfile.ZipFile(archive, 'w') as z:
            z.writestr('assets/test_mobile_and.pak', data)
        entries = [dict(path='assets/test_mobile_and.pak', bytes=len(data), sha256='0'*64 if bad else hashlib.sha256(data).hexdigest())]
        config = dict(base_url='https://invalid.example/', base=dict(sha256=resources.digest(archive), entries=entries), remote=dict(files=[]))
        (bundle / 'china/installer.json').write_text(json.dumps(config))
        return bundle, data

    def test_unverified_mount_aliases_are_explicit_and_original_is_preserved(self):
        bundle, data = self.fixture()
        root = self.root / 'game'
        result = resources.install(bundle, root, self.emit)
        self.assertEqual((root / 'pak/test_mobile_and.pak').read_bytes(), data)
        self.assertEqual((root / 'pak/test_mobile_ios.pak').read_bytes(), data)
        self.assertFalse(result['cooked_assets_converted'])
        self.assertFalse(result['native_china_client'])
        self.assertIn('unverified', result['aliases'][0]['platform_compatibility'])
        self.assertEqual(resources.install(bundle, root, self.emit), result)

    def test_hash_failure_keeps_active_resources_and_no_ready_marker(self):
        bundle, data = self.fixture(bad=True)
        root = self.root / 'game'
        (root / 'pak').mkdir(parents=True)
        original = root / 'pak/old.pak'
        original.write_bytes(b'existing user resources')
        with self.assertRaisesRegex(ValueError, 'integrity'):
            resources.install(bundle, root, self.emit)
        self.assertEqual(original.read_bytes(), b'existing user resources')
        self.assertFalse((root / 'china-data-installed.json').exists())

    def test_global_data_is_copied_without_old_liveupdate_and_is_writable_independently(self):
        source = self.root / 'old-documents'
        (source / 'pak').mkdir(parents=True)
        (source / 'cpk').mkdir()
        (source / 'pak/pc0000_mobile_ios.pak').write_bytes(b'global base')
        (source / 'pak/pc9257_mobile_ios.pak').write_bytes(b'old Live Update')
        (source / 'cpk/dt500_mobile_all.cpk').write_bytes(b'base tables')
        target = self.root / 'new-documents'
        resources.prepare_global(source, target, self.emit)
        self.assertFalse((target / 'pak/pc9257_mobile_ios.pak').exists())
        (target / 'pak/pc0000_mobile_ios.pak').write_bytes(b'new changes')
        self.assertEqual((source / 'pak/pc0000_mobile_ios.pak').read_bytes(), b'global base')

    def test_archive_traversal_rejected(self):
        with self.assertRaises(ValueError):
            resources.safe_name('assetpacks/../../outside.pak')


if __name__ == '__main__':
    unittest.main()
