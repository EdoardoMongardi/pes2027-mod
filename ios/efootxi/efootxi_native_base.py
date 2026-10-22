"""Keep China account/gameplay data while rendering with verified iOS assets.

The manifest identifies the original iOS base already installed on the device.
No Android textures, shaders, movies or package stores are renamed as iOS here.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import time


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def install(bundle, original_documents, root, emit, records=None):
    bundle, original_documents, root = map(Path, (bundle, original_documents, root))
    if records is None:
        records = json.loads(Path(__file__).with_name('native_base_manifest.json').read_text())
    fingerprint = hashlib.sha256(json.dumps(records, sort_keys=True).encode()).hexdigest()
    marker_path = root / 'china-data-installed.json'
    before = marker_path.read_bytes()
    marker = json.loads(before)
    current = root / 'pak'
    pending_path = root / 'native-render-pending.json'
    if pending_path.exists():
        pending = json.loads(pending_path.read_text())
        if pending.get('manifest_sha256') == fingerprint:
            emit('native_base_restart_required', files=len(records))
            return True
        raise RuntimeError('A different native graphics update is pending')
    if marker.get('native_render_overlay', {}).get('manifest_sha256') == fingerprint and all(
            (current / row['name']).is_file() and (current / row['name']).stat().st_size == row['bytes'] for row in records):
        emit('native_base_ready', files=len(records), already_installed=True)
        return True
    # Pin native pak/index identities. Large UCAS payloads may be identified by
    # their paired pinned index and exact length when no full capture exists;
    # every copied payload is still hashed against its original source.
    sources = original_documents / 'pak'
    source_digests = {}
    for row in records:
        name = row['name']
        if Path(name).name != name or '/' in name or '\\' in name or not name.endswith(('.pak', '.utoc', '.ucas')):
            raise ValueError('Invalid native base filename')
        source = sources / name
        if not source.is_file() or source.stat().st_size != row['bytes']:
            emit('native_base_unavailable', file=name, reason='original iOS file missing or wrong size; active data preserved')
            return False
        actual_digest = digest(source)
        expected_digest = row.get('sha256')
        if not expected_digest:
            index_name = name.removesuffix('.ucas') + '.utoc'
            index = next((item for item in records if item['name'] == index_name), None)
            if not name.endswith('.ucas') or not index or not index.get('sha256') or row.get('index_sha256') != index['sha256']:
                raise ValueError('Native payload requires a pinned companion index')
            expected_digest = actual_digest if digest(sources / index_name) == index['sha256'] else None
        if actual_digest != expected_digest:
            emit('native_base_unavailable', file=name, reason='original iOS SHA-256 does not match; active data preserved')
            return False
        source_digests[name] = actual_digest
    live = json.loads((bundle / 'livedata/native-manifest.json').read_text())
    live = [row for row in live if row['name'].endswith(('.pak', '.utoc', '.ucas'))]
    for row in live:
        if digest(root / 'livedata' / row['name']) != row['sha256']:
            raise ValueError('Native livedata integrity failed: ' + row['name'])
    stamp = str(time.time_ns())
    staging = root / ('pak.native-staging-' + stamp)
    backup = root / ('pak.before-native-' + stamp)
    marker_backup = root / ('china-data-installed.before-native-' + stamp + '.json')
    required = sum(row['bytes'] for row in records)
    if shutil.disk_usage(root).free < required + 64 * 1024 * 1024:
        raise OSError('Insufficient space for an independent native graphics copy')
    staging.mkdir()
    for index, row in enumerate(records):
        destination = staging / row['name']
        shutil.copyfile(sources / row['name'], destination)
        if digest(destination) != source_digests[row['name']]:
            raise ValueError('Native base copy integrity failed: ' + row['name'])
        emit('native_base_copy', file=row['name'], index=index + 1, count=len(records))
    for row in live:
        destination = staging / row['name']
        try:
            destination.symlink_to('../livedata/' + row['name'])
        except OSError:
            shutil.copyfile(root / 'livedata' / row['name'], destination)
    # Retain the original account and CPK state, and make the installer validate
    # the actual native files on subsequent launches instead of restoring aliases.
    marker['files'] = [row for row in marker['files'] if row['path'].startswith('cpk/')]
    marker['files'] += [dict(path='pak/' + row['name'], bytes=row['bytes']) for row in records]
    marker['files'] += [dict(path='pak/' + row['name'], bytes=row['size']) for row in live]
    marker.update(aliases=[], mode='China backend with verified original iOS rendering data',
                  native_render_overlay=dict(revision=1, manifest_sha256=fingerprint, previous_pak=backup.name,
                                             backup_marker=marker_backup.name))
    temporary_marker = root / ('china-data-installed.native-staging-' + stamp + '.json')
    temporary_marker.write_text(json.dumps(marker, indent=2), encoding='utf-8')
    marker_backup.write_bytes(before)
    # Unreal starts on the main thread while preparation runs in the background.
    # Publish only a pending descriptor here. The native launch hook performs
    # the small directory swap on the next launch, before Unreal mounts assets.
    pending = dict(pak_staging=staging.name, pak_backup=backup.name,
                   marker_staging=temporary_marker.name, marker_backup=marker_backup.name,
                   manifest_sha256=fingerprint)
    temporary_pending = root / 'native-render-pending.tmp'
    temporary_pending.write_text(json.dumps(pending, indent=2), encoding='utf-8')
    os.replace(temporary_pending, pending_path)
    emit('native_base_restart_required', files=len(records), account_state_changed=False)
    return True
