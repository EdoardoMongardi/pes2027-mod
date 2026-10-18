#!/usr/bin/env python3
"""Port the user's texture mod to the native eFootball 11.1.101 IoStore layout.

Android base containers are supplied by their native iOS equivalents, never
renamed. The pc7000 mod has BGRA textures but an extended 32-byte package-store
entry; iOS's unversioned FFilePackageStore expects 16-byte entries. Preserve
every package/bulk chunk and package ID, rebuild this header and the directory
index, then verify every output chunk by SHA-1 and byte comparison.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import shutil
import zlib

from make_extra_pak import Toc, build_store, dir_index, parse_header

BLOCK = 65536


def native_header(header, texture_sizes):
    pids, store = header['pids'], header['store']
    if len(store) != len(pids) * 32:
        raise ValueError('pc7000 does not have the analysed extended package-store layout')
    entries = []
    load_orders = set()
    for pos, pid in enumerate(pids):
        size, exports, bundles, order, reserved, count, offset = struct.unpack_from('<Q6I', store, pos * 32)
        if size != texture_sizes[pid] or exports != 1 or bundles != 1 or reserved or count or offset:
            raise ValueError(f'Unsupported package-store record at {pos}: dependencies require separate conversion')
        if order in load_orders:
            raise ValueError('Duplicate load-order index')
        load_orders.add(order)
        entries.append((struct.pack('<II', exports, bundles), []))
    output = bytearray(struct.pack('<QI', header['cid'], len(pids)))
    for key in ('names', 'hashes'):
        output += struct.pack('<i', len(header[key])) + header[key]
    output += struct.pack('<i', len(pids)) + struct.pack(f'<{len(pids)}Q', *pids)
    compact = build_store(entries)
    output += struct.pack('<i', len(compact)) + compact
    output += struct.pack('<ii', 0, 0)  # empty culture map and package redirects
    return bytes(output)


def export_mod(source, destination):
    toc = Toc(str(source))
    headers = [i for i, cid in enumerate(toc.ids) if cid[11] == 10]
    if len(headers) != 1:
        raise ValueError('Expected one container header')
    hi = headers[0]
    original = parse_header(toc.read(hi))
    if struct.unpack_from('<Q', toc.raw, 56)[0] != original['cid']:
        raise ValueError('TOC/container ID mismatch')
    sizes, formats = {}, Counter()
    import re
    for i, cid in enumerate(toc.ids):
        if cid[11] == 2:
            data = toc.read(i)
            pixel_formats = set(re.findall(rb'PF_[A-Za-z0-9_]+', data))
            if pixel_formats != {b'PF_B8G8R8A8'}:
                raise ValueError(f'Package {cid.hex()} is not a verified BGRA texture: {pixel_formats}')
            sizes[struct.unpack_from('<Q', cid)[0]] = len(data)
            formats['PF_B8G8R8A8'] += 1
        elif cid[11] not in (3, 10):
            raise ValueError(f'Unexpected chunk type {cid[11]}')
    if set(sizes) != set(original['pids']):
        raise ValueError('Package store does not match export chunks')
    header = native_header(original, sizes)
    destination.parent.mkdir(parents=True, exist_ok=True)
    data_path = destination.with_suffix('.ucas')
    if toc.flags & 2 or toc.methods != [b'Zlib'] or toc.cbs != BLOCK:
        raise ValueError('Expected the verified unencrypted Zlib pc7000 source')
    # Keep all compressed texture/bulk blocks byte-for-byte. The replacement
    # native header is appended at a new virtual block boundary; the old
    # extended header's blocks become unreferenced. No texture is recooked.
    shutil.copyfile(source.with_suffix('.ucas'), data_path)
    locations = list(toc.ol)
    blocks = list(toc.blocks)
    ids = list(toc.ids)
    metas = list(toc.metas)
    old_cid = original['cid']
    cid = struct.unpack('<Q', hashlib.sha1(destination.stem.encode()).digest()[:8])[0]
    header = bytearray(header)
    struct.pack_into('<Q', header, 0, cid)
    header = bytes(header)
    ids[hi] = struct.pack('<Q', cid) + ids[hi][8:]
    locations[hi] = (len(blocks) * BLOCK, len(header))
    metas[hi] = hashlib.sha1(header).digest() + b'\0' * 12 + b'\x01'
    with data_path.open('ab') as output:
        for pos in range(0, len(header), BLOCK):
            raw = header[pos:pos + BLOCK]
            compressed = zlib.compress(raw, 6)
            offset = output.tell()
            if len(compressed) < len(raw):
                output.write(compressed)
                blocks.append((offset, len(compressed), len(raw), 1))
            else:
                output.write(raw)
                blocks.append((offset, len(raw), len(raw), 0))
    # Normalize the mod editor's ORI_IDS27 project mount to the iOS project.
    index = dir_index('../../../PesMobile/Content/Assets/', toc.files)
    result = bytearray(toc.raw[:toc.hs])
    struct.pack_into('<IIIIIIIII', result, 20, toc.hs, len(ids), len(blocks), 12, 1, 32, BLOCK, len(index), 0)
    struct.pack_into('<Q', result, 56, cid)
    result[64:80] = b'\0' * 16
    result[80] = 1 | 8  # compressed, indexed, unencrypted
    result += b''.join(ids)
    result += b''.join(o.to_bytes(5, 'big') + n.to_bytes(5, 'big') for o, n in locations)
    result += b''.join(o.to_bytes(5, 'little') + c.to_bytes(3, 'little') + u.to_bytes(3, 'little') + bytes([m])
                       for o, c, u, m in blocks)
    result += b'Zlib'.ljust(32, b'\0') + index + b''.join(metas)
    destination.write_bytes(result)
    verify = Toc(str(destination))
    verified_header = parse_header(verify.read(hi))
    if len(verified_header['store']) != 16 * len(sizes) or verified_header['pids'] != original['pids']:
        raise ValueError('Converted package-store layout mismatch')
    if verify.files != toc.files or verify.ids != ids:
        raise ValueError('Lost directory/package identifiers during conversion')
    for i in range(len(ids)):
        if i % 1000 == 0:
            print(f'Verifying chunk {i}/{len(ids)}', flush=True)
        actual = verify.read(i)
        expected = header if i == hi else toc.read(i)
        if actual != expected or hashlib.sha1(actual).digest() != verify.metas[i][:20]:
            raise ValueError(f'Output chunk {i} failed verification')
    return {'source': str(source), 'output': str(destination), 'packages': len(sizes), 'chunks': len(ids),
            'formats': dict(formats), 'old_store_entry_bytes': 32, 'new_store_entry_bytes': 16,
            'all_texture_and_bulk_chunks_identical': True, 'all_chunk_hashes_verified': True,
            'mount': verify.mount, 'container_id': hex(cid), 'original_container_id': hex(old_cid),
            'compressed_resource_blocks_preserved': True, 'ucas_bytes': data_path.stat().st_size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assetpacks', type=Path, required=True)
    parser.add_argument('--ios-pak', type=Path, required=True, help='Native iOS pak folder, used for an empty companion pak')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    matches = list(args.assetpacks.glob('*/*/*/assets/pc7000_mobile_and.utoc'))
    if len(matches) != 1:
        raise SystemExit('Expected exactly one pc7000 mod in assetpacks')
    report = export_mod(matches[0], args.out / 'pc7998_mobile_ios_P.utoc')
    companion = args.ios_pak / 'pc0103_mobile_ios.pak'
    if companion.stat().st_size != 365:
        raise SystemExit('Expected the verified empty native iOS pc0103 companion pak')
    shutil.copyfile(companion, args.out / 'pc7998_mobile_ios_P.pak')
    (args.out / 'conversion.json').write_text(json.dumps(report, indent=2), encoding='utf8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
