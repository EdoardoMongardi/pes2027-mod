"""Repair directory paths in a supplemental UI/legend container without changing
its cooked assets. This is not Android-to-iOS shader or mesh conversion.
"""
import argparse
import hashlib
import json
import shutil
import struct
import zlib
from pathlib import Path
from make_extra_pak import Toc, dir_index, parse_header, store_entry, build_store

MOUNT = '../../../PesMobile/Content/Assets/'


def source_entry(header, index):
    """Recognize the extended 32-byte no-import layout before reading a row.
    It stores export-bundle byte size (u64), export/bundle counts, load order,
    padding, and an empty imported-packages view. iOS uses 16-byte entries.
    """
    store, count = header['store'], len(header['pids'])
    extended = len(store) == count * 32 and all(
        struct.unpack_from('<II', store, n * 32 + 24) == (0, 0)
        and struct.unpack_from('<I', store, n * 32 + 20)[0] == 0
        and 0 < struct.unpack_from('<I', store, n * 32 + 8)[0] < 100000
        and 0 < struct.unpack_from('<I', store, n * 32 + 12)[0] < 100000
        for n in range(count))
    if extended:
        return (store[index * 32 + 8:index * 32 + 16], []), 32
    fixed, imports = store_entry(store, index)
    exports, bundles = struct.unpack('<II', fixed)
    if not (0 < exports < 100000 and 0 < bundles < 100000):
        raise ValueError('invalid source package-store counts')
    return (fixed, imports), 16


def repaired_header(toc, source_paths):
    hi = next(i for i, chunk in enumerate(toc.ids) if chunk[11] == 10)
    original = parse_header(toc.read(hi))
    wanted = set(original['pids'])
    candidates = {}
    for path in source_paths:
        source = Toc(str(path))
        header = parse_header(source.read(next(i for i, c in enumerate(source.ids) if c[11] == 10)))
        chunks = {struct.unpack_from('<Q', c)[0]: i for i, c in enumerate(source.ids) if c[11] == 2}
        for position, pid in enumerate(header['pids']):
            if pid in wanted and pid in chunks:
                candidates.setdefault(pid, []).append((source, header, position, chunks[pid]))
    export_chunks = {struct.unpack_from('<Q', c)[0]: i for i, c in enumerate(toc.ids) if c[11] == 2}
    entries, converted, origins = [], [], []
    for pid in original['pids']:
        current = toc.read(export_chunks[pid])
        found = None
        for source, header, position, chunk in candidates.get(pid, []):
            if source.read(chunk) == current:
                entry, stride = source_entry(header, position)
                found = entry
                origins.append({'package_id': f'{pid:016x}', 'source': source.path, 'source_stride': stride})
                if stride == 32:
                    converted.append(f'{pid:016x}')
                break
        if found is None:
            raise ValueError('no byte-identical source for package ' + f'{pid:016x}')
        entries.append(found)
    out = bytearray(struct.pack('<QI', original['cid'], original['cnt']))
    for data in (original['names'], original['hashes']):
        out += struct.pack('<i', len(data)) + data
    out += struct.pack('<i', len(original['pids'])) + struct.pack('<%dQ' % len(original['pids']), *original['pids'])
    store = build_store(entries)
    out += struct.pack('<i', len(store)) + store + original['rest']
    return hi, bytes(out), converted, origins


def write_payload(toc, header_index, header_data, paths, output):
    """Repack offsets/compression while preserving every cooked asset byte."""
    block_size = 65536
    blocks, offsets, metadata, payloads = [], [], [], []
    virtual_offset = 0
    with output.with_suffix('.ucas').open('wb') as f:
        for i, cid in enumerate(toc.ids):
            data = header_data if i == header_index else toc.read(i)
            offsets.append((virtual_offset, len(data)))
            metadata.append(hashlib.sha1(data).digest() + b'\0' * 13 if i == header_index else toc.metas[i])
            payloads.append(data)
            for start in range(0, len(data), block_size):
                raw = data[start:start + block_size]
                compressed = zlib.compress(raw, 9)
                content, method = (compressed, 1) if len(compressed) < len(raw) else (raw, 0)
                blocks.append((f.tell(), len(content), len(raw), method))
                f.write(content)
            virtual_offset += (len(data) + block_size - 1) // block_size * block_size
    index = dir_index(MOUNT, paths)
    head = bytearray(toc.raw[:toc.hs])
    struct.pack_into('<IIIIIIIII', head, 20, toc.hs, len(toc.ids), len(blocks), 12, 1, 32, block_size, len(index), 0)
    head[64:80] = b'\0' * 16
    head[80] = 9
    out = head + b''.join(toc.ids)
    out += b''.join(o.to_bytes(5, 'big') + n.to_bytes(5, 'big') for o, n in offsets)
    out += b''.join(o.to_bytes(5, 'little') + c.to_bytes(3, 'little') + n.to_bytes(3, 'little') + bytes([m]) for o, c, n, m in blocks)
    out += b'Zlib'.ljust(32, b'\0') + index + b''.join(metadata)
    output.write_bytes(out)
    shutil.copy2(Path(toc.path).with_suffix('.pak'), output.with_suffix('.pak'))
    verified = Toc(str(output))
    for i, expected in enumerate(payloads):
        if verified.read(i) != expected:
            raise ValueError('repacked payload mismatch')
    return verified


def relative_asset(path):
    path = path.replace('\\', '/')
    if 'PesMobile/Content/Assets/' in path:
        path = path.split('PesMobile/Content/Assets/', 1)[1]
    while path.startswith('Assets/'):
        path = path[len('Assets/'):]
    if not path.startswith(('ui/', 'character/')) or '..' in path.split('/'):
        raise ValueError('not a supplemental UI/character asset: ' + path)
    return path


def normalize(source, aliases, output, package_sources=None):
    source, output = Path(source), Path(output)
    if source.resolve() == output.resolve():
        raise ValueError('output must differ from input')
    toc = Toc(str(source))
    if toc.flags & (2 | 4):
        raise ValueError('encrypted/signed indices require a separate writer')
    ids = {chunk: i for i, chunk in enumerate(toc.ids)}
    paths = {}
    for other in [toc] + [Toc(str(p)) for p in aliases]:
        for path, index in other.files.items():
            chunk = other.ids[index]
            if chunk not in ids:
                raise ValueError('alias source contains a payload chunk absent from primary container')
            canonical = relative_asset(path)
            previous = paths.get(canonical)
            if previous is not None and previous != ids[chunk]:
                raise ValueError('two payloads claim ' + canonical)
            paths[canonical] = ids[chunk]
    hs, ec, bc, bs, nc, nl = struct.unpack_from('<IIIIII', toc.raw, 20)
    directory_size = struct.unpack_from('<I', toc.raw, 48)[0]
    start = hs + ec * 22 + bc * bs + nc * nl
    new_index = dir_index(MOUNT, paths)
    result = bytearray(toc.raw[:start] + new_index + toc.raw[start + directory_size:])
    struct.pack_into('<I', result, 48, len(new_index))
    output.parent.mkdir(parents=True, exist_ok=True)
    converted, origins, header_index = [], [], None
    if package_sources:
        header_index, header_data, converted, origins = repaired_header(toc, package_sources)
        verified = write_payload(toc, header_index, header_data, paths, output)
    else:
        output.write_bytes(result)
        for ext in ('.pak', '.ucas'):
            shutil.copy2(source.with_suffix(ext), output.with_suffix(ext))
        verified = Toc(str(output))
    if verified.ids != toc.ids:
        raise ValueError('payload mapping unexpectedly changed')
    if verified.files != paths or verified.mount != MOUNT:
        raise ValueError('directory did not round-trip')
    # Every payload is read back, not just its directory listing.
    for i in range(len(toc.ids)):
        if i != header_index and verified.read(i) != toc.read(i):
            raise ValueError('payload mismatch at chunk ' + str(i))
    report = {'source': str(source), 'output': str(output), 'mount': MOUNT,
              'source_files': len(toc.files), 'result_files': len(paths),
              'cooked_asset_chunks_preserved': len(toc.ids) - (1 if package_sources else 0),
              'container_header_chunks_rebuilt': 1 if package_sources else 0,
              'extended_store_entries_converted': len(converted),
              'package_store_sources': origins,
              'ucas_sha256': hashlib.sha256(output.with_suffix('.ucas').read_bytes()).hexdigest(),
              'limitations': ['Cooked asset platform compatibility remains unverified.',
                              'A directory alias alone does not create a new package ID or export name.']}
    try:
        from cityhash import CityHash64
        mismatches = []
        for path, i in paths.items():
            if not path.endswith('.uasset'):
                continue
            package = '/Game/Assets/' + path[:-7]
            wanted = CityHash64(package.lower().encode('utf-16-le'))
            actual = struct.unpack_from('<Q', verified.ids[i])[0]
            if wanted != actual:
                mismatches.append({'path': path, 'expected': f'{wanted:016x}', 'actual': f'{actual:016x}'})
        report['aliases_with_different_package_ids'] = mismatches
    except ImportError:
        report['package_id_validation'] = 'cityhash not installed'
    output.with_suffix('.audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', required=True)
    p.add_argument('--aliases', action='append', default=[])
    p.add_argument('--out', required=True)
    p.add_argument('--package-source-root', action='append', default=[])
    a = p.parse_args()
    sources = [path for root in a.package_source_root for path in Path(root).rglob('*.utoc')]
    r = normalize(a.input, a.aliases, a.out, sources)
    print(json.dumps({k: v for k, v in r.items() if k not in ('aliases_with_different_package_ids', 'package_store_sources')}, indent=2))
    if 'aliases_with_different_package_ids' in r:
        print('Aliases needing separate package IDs:', len(r['aliases_with_different_package_ids']))
    else:
        print('Package ID validation unavailable:', r.get('package_id_validation'))
