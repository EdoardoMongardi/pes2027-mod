"""Resumable, hash-checked installer for the supplied China data experiment.

Preserves the original Android files and records iOS mount-name aliases as
unverified compatibility attempts. It does not convert cooked platform assets.
"""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path
import shutil
import time
import urllib.request
import zipfile
import re
import ssl

ALLOWED = {'.pak', '.ucas', '.utoc', '.cpk'}


def digest(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def safe_name(name):
    p = Path(name)
    if p.is_absolute() or '..' in p.parts or '\\' in name or ':' in name:
        raise ValueError('Unsafe archive path: ' + name)
    return p.name


def download(url, destination, expected, size, emit, ca_file=None):
    if destination.exists() and destination.stat().st_size == size and digest(destination) == expected:
        return
    partial = destination.with_suffix(destination.suffix + '.part')
    offset = partial.stat().st_size if partial.exists() else 0
    headers = {'User-Agent': 'Efootxi-iOS-data-diagnostics/1'}
    if offset:
        headers['Range'] = 'bytes=%d-' % offset
    emit('download_begin', url=url, downloaded=offset, total=size)
    context = ssl.create_default_context(cafile=str(ca_file)) if ca_file else None
    with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=60, context=context) as response:
        append = bool(offset and response.status == 206)
        if append and not response.headers.get('Content-Range', '').startswith('bytes %d-' % offset):
            raise ValueError('Unexpected resumed Content-Range')
        if not append:
            offset = 0
        last = 0
        with partial.open('ab' if append else 'wb') as out:
            while chunk := response.read(1024 * 1024):
                out.write(chunk)
                offset += len(chunk)
                if offset > size:
                    raise ValueError('Download exceeds pinned size')
                if time.monotonic() - last > 2:
                    last = time.monotonic()
                    emit('download_progress', downloaded=offset, total=size, file=destination.name)
    if offset != size or digest(partial) != expected:
        bad = partial.with_name(partial.name + '.bad-' + str(time.time_ns()))
        os.replace(partial, bad)
        raise ValueError('Downloaded resource size/SHA-256 mismatch: ' + destination.name)
    os.replace(partial, destination)
    emit('download_verified', file=destination.name, bytes=size, sha256=expected)


def extract(archive, entries, staging, emit):
    names = set()
    selected = [entry for entry in entries if Path(entry['path']).suffix in ALLOWED]
    with zipfile.ZipFile(archive) as z:
        for index, entry in enumerate(selected):
            name = safe_name(entry['path'])
            group = 'cpk' if name.endswith('.cpk') else 'pak'
            relative = group + '/' + name
            if relative in names:
                raise ValueError('Duplicate resource name: ' + name)
            names.add(relative)
            target = staging / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists() and target.stat().st_size == entry['bytes'] and digest(target) == entry['sha256']:
                continue
            temporary = target.with_name(target.name + '.extracting')
            h = hashlib.sha256()
            count = 0
            with z.open(entry['path']) as source, temporary.open('wb') as out:
                while chunk := source.read(1024 * 1024):
                    out.write(chunk)
                    h.update(chunk)
                    count += len(chunk)
                    if count > entry['bytes']:
                        raise ValueError('Expanded size exceeds pinned manifest')
            if count != entry['bytes'] or h.hexdigest() != entry['sha256']:
                raise ValueError('Extracted resource integrity failed: ' + name)
            os.replace(temporary, target)
            emit('extract_verified', file=name, index=index+1, count=len(selected), bytes=count, sha256=h.hexdigest())


def install(bundle, root, emit):
    bundle, root = Path(bundle), Path(root)
    config = json.loads((bundle / 'china/installer.json').read_text(encoding='utf-8'))
    fingerprint = digest(bundle / 'china/installer.json')
    marker = root / 'china-data-installed.json'
    if marker.is_file():
        prior = json.loads(marker.read_text())
        if prior.get('installer_sha256') == fingerprint and all(
                (root / r['path']).is_file() and (root / r['path']).stat().st_size == r['bytes'] for r in prior['files']):
            emit('already_installed', files=len(prior['files']))
            return prior
    root.mkdir(parents=True, exist_ok=True)
    staging = root / '.china-staging'
    staging.mkdir(exist_ok=True)
    cache = root / '.downloads'
    cache.mkdir(exist_ok=True)
    required = sum(item.get('expandedBytes', 0) + item.get('bytes', 0) for item in config['remote']['files'])
    if shutil.disk_usage(root).free < required:
        raise OSError('Insufficient free space to stage and verify China resources; need %d bytes' % required)
    base = bundle / 'china/base.zip'
    emit('checking_base', bytes=base.stat().st_size)
    if digest(base) != config['base']['sha256']:
        raise ValueError('Bundled China base archive SHA-256 mismatch')
    extract(base, config['base']['entries'], staging, emit)
    for item in config['remote']['files']:
        destination = cache / safe_name(item['name'])
        download(config['base_url'] + item['name'], destination, item['sha256'], item['bytes'], emit,
                 ca_file=bundle / 'bridge/cacert.pem')
        extract(destination, item['entries'], staging, emit)
    aliases = []
    for group in ('pak', 'cpk'):
        directory = staging / group
        directory.mkdir(exist_ok=True)
        for source in list(directory.iterdir()):
            if source.suffix not in ALLOWED or '_mobile_and' not in source.name:
                continue
            alias = source.with_name(source.name.replace('_mobile_and', '_mobile_ios'))
            if not alias.exists():
                os.link(source, alias)
            aliases.append(dict(source=group+'/'+source.name, alias=group+'/'+alias.name,
                                bytes=source.stat().st_size, platform_compatibility='unverified; unchanged Android payload'))
    # All supplied files verify before any active directory is published.
    for group in ('pak', 'cpk'):
        current = root / group
        if current.exists():
            os.replace(current, root / (group + '.previous-' + str(time.time_ns())))
        os.replace(staging / group, current)
    files = [dict(path=str(p.relative_to(root)).replace('\\','/'), bytes=p.stat().st_size)
             for group in ('pak','cpk') for p in sorted((root/group).iterdir()) if p.is_file()]
    result = dict(installer_sha256=fingerprint, files=files, aliases=aliases,
                  mode='China data experiment on international iOS executable',
                  native_china_client=False, cooked_assets_converted=False)
    temp = marker.with_suffix('.tmp')
    temp.write_text(json.dumps(result, indent=2), encoding='utf-8')
    os.replace(temp, marker)
    emit('installed', files=len(files), aliases=aliases, compatibility='requires on-device testing')
    return result


def prepare_global(original_documents, root, emit):
    original_documents, root = Path(original_documents), Path(root)
    root.mkdir(parents=True, exist_ok=True)
    marker = root / 'global-data-installed.json'
    if marker.is_file():
        result = json.loads(marker.read_text())
        if all((root / f['path']).is_file() and (root / f['path']).stat().st_size == f['bytes'] for f in result['files']):
            return result
    files = []
    for group in ('pak', 'cpk'):
        directory = original_documents / group
        if not directory.is_dir():
            continue
        for source in sorted(directory.iterdir()):
            if source.suffix not in ALLOWED or not source.is_file():
                continue
            # Keep the carrier's base assets; its old Live Update belongs to another mod.
            if re.match(r'pc9[0-9]+_', source.name) or source.name.startswith('dt870_'):
                continue
            relative = group + '/' + source.name
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists() or target.stat().st_size != source.stat().st_size:
                temporary = target.with_name(target.name + '.copying')
                shutil.copy2(source, temporary, follow_symlinks=True)
                os.replace(temporary, target)
            files.append(dict(path=relative, bytes=source.stat().st_size, source=str(source)))
            emit('global_copy', file=source.name, bytes=source.stat().st_size)
    if not files or not any(f['path'].startswith('pak/pc0000_') for f in files):
        raise RuntimeError('No global iOS base data found in Documents/pak and Documents/cpk. Install over the existing app with its data retained.')
    result = dict(mode='international iOS base', files=files, old_mod_livedata_imported=False)
    marker.write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result


def install_native_live(bundle, root, emit):
    native = Path(bundle) / 'livedata'
    records = json.loads((native / 'native-manifest.json').read_text())
    for record in records:
        source = native / record['name']
        target = root / 'livedata' / source.name
        target.parent.mkdir(parents=True, exist_ok=True)
        # Only our immutable shipped manifest controls this folder's native files.
        if not target.is_file() or digest(target) != record['sha256']:
            temporary = target.with_name(target.name + '.copying')
            shutil.copyfile(source, temporary)
            if digest(temporary) != record['sha256']:
                raise ValueError('Native iOS livedata copy hash mismatch')
            os.replace(temporary, target)
        group = 'cpk' if target.suffix == '.cpk' else 'pak'
        for folder in (group, 'Download'):
            link = root / folder / source.name
            link.parent.mkdir(parents=True, exist_ok=True)
            if link.is_symlink():
                link.unlink()
            elif link.exists():
                # Preserve differing files from the China data rather than overwrite them.
                if digest(link) == record['sha256']:
                    continue
                os.replace(link, link.with_name(link.name + '.previous-' + str(time.time_ns())))
            try:
                os.symlink('../livedata/' + source.name, link)
            except OSError:
                shutil.copy2(target, link)
    emit('native_livedata_ready', files=len(records))
