import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

MODULES = Path(__file__).resolve().parents[1] / 'ios/efootxi'
sys.path.insert(0, str(MODULES))
import efootxi_native_base as native
import efootxi_resources as resources


class NativeBaseTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        directory = Path(temporary.name)
        self.bundle, self.original, self.root = (directory / name for name in ('bundle', 'Documents', 'data'))
        for path in (self.bundle / 'livedata', self.bundle / 'china', self.original / 'pak', self.root / 'pak', self.root / 'cpk'):
            path.mkdir(parents=True)
        (self.bundle / 'livedata/native-manifest.json').write_text('[]')
        (self.bundle / 'china/installer.json').write_text('{}')
        self.records = []
        for name in ('pc0000_mobile_ios.pak', 'pc0000_mobile_ios.utoc', 'pc0000_mobile_ios.ucas',
                     'global_mobile_ios.utoc', 'global_mobile_ios.ucas'):
            data = ('native:' + name).encode()
            (self.original / 'pak' / name).write_bytes(data)
            self.records.append(dict(name=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        (self.root / 'pak/old_mobile_and.pak').write_bytes(b'old graphics')
        (self.root / 'cpk/dt500_mobile_all.cpk').write_bytes(b'cpk')
        (self.root / 'account.sqlite3').write_bytes(b'owned players and draw receipts')
        marker = dict(installer_sha256=resources.digest(self.bundle / 'china/installer.json'),
                      files=[dict(path='cpk/dt500_mobile_all.cpk', bytes=3)], aliases=['old alias'])
        self.marker = self.root / 'china-data-installed.json'
        self.marker.write_text(json.dumps(marker))
        self.events = []
        self.emit = lambda event, **values: self.events.append((event, values))

    def test_overlay_keeps_originals_account_and_backup_and_survives_installer(self):
        before = self.marker.read_bytes()
        self.assertTrue(native.install(self.bundle, self.original, self.root, self.emit, self.records))
        pending = json.loads((self.root / 'native-render-pending.json').read_text())
        self.assertEqual(self.marker.read_bytes(), before)
        self.assertEqual((self.root / 'pak/old_mobile_and.pak').read_bytes(), b'old graphics')
        self.assertEqual((self.root / 'account.sqlite3').read_bytes(), b'owned players and draw receipts')
        self.assertEqual((self.root / 'cpk/dt500_mobile_all.cpk').read_bytes(), b'cpk')
        staging = self.root / pending['pak_staging']
        for row in self.records:
            self.assertEqual(native.digest(self.original / 'pak' / row['name']), row['sha256'])
            self.assertEqual(native.digest(staging / row['name']), row['sha256'])
        self.assertTrue(native.install(self.bundle, self.original, self.root, self.emit, self.records))
        self.assertEqual(len(list(self.root.glob('pak.native-staging-*'))), 1)
        # Fixture for the completed native startup, whose actual filesystem
        # transaction is tested by display_repair_test.mm on macOS.
        (self.root / 'pak').rename(self.root / pending['pak_backup'])
        staging.rename(self.root / 'pak')
        (self.root / pending['marker_staging']).replace(self.marker)
        (self.root / 'native-render-pending.json').unlink()
        marker = json.loads(self.marker.read_text())
        self.assertEqual(resources.install(self.bundle, self.root, self.emit), marker)
        self.assertTrue(native.install(self.bundle, self.original, self.root, self.emit, self.records))
        self.assertEqual(len(list(self.root.glob('pak.before-native-*'))), 1)

    def test_wrong_source_hash_keeps_active_data_and_marker_untouched(self):
        before = self.marker.read_bytes()
        self.records[0]['sha256'] = '0' * 64
        self.assertFalse(native.install(self.bundle, self.original, self.root, self.emit, self.records))
        self.assertEqual(self.marker.read_bytes(), before)
        self.assertEqual((self.root / 'pak/old_mobile_and.pak').read_bytes(), b'old graphics')
        self.assertFalse(list(self.root.glob('pak.before-native-*')))

    def test_pending_publish_failure_leaves_active_assets_untouched(self):
        before = self.marker.read_bytes()
        original_replace = native.os.replace
        def fail_marker(source, target):
            if Path(target) == self.root / 'native-render-pending.json':
                raise OSError('simulated full disk')
            return original_replace(source, target)
        with patch.object(native.os, 'replace', side_effect=fail_marker):
            with self.assertRaisesRegex(OSError, 'full disk'):
                native.install(self.bundle, self.original, self.root, self.emit, self.records)
        self.assertEqual(self.marker.read_bytes(), before)
        self.assertEqual((self.root / 'pak/old_mobile_and.pak').read_bytes(), b'old graphics')


if __name__ == '__main__':
    unittest.main()
