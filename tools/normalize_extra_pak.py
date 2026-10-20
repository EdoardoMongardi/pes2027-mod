"""Repair directory paths in a supplemental UI/legend container without changing
its cooked assets. This is not Android-to-iOS shader or mesh conversion.
"""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path
from make_extra_pak import Toc, dir_index

MOUNT = '../../../PesMobile/Content/Assets/'


def relative_asset(path):
    path = path.replace('\\', '/')
    if 'PesMobile/Content/Assets/' in path:
        path = path.split('PesMobile/Content/Assets/', 1)[1]
    while path.startswith('Assets/'):
        path = path[len('Assets/'):]
    if not path.startswith(('ui/', 'character/')) or '..' in path.split('/'):
        raise ValueError('not a supplemental UI/character asset: ' + path)
    return path


def normalize(source, aliases, output):
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
    output.write_bytes(result)
    for ext in ('.pak', '.ucas'):
        shutil.copy2(source.with_suffix(ext), output.with_suffix(ext))
    verified = Toc(str(output))
    if verified.ids != toc.ids or verified.blocks != toc.blocks or verified.ol != toc.ol:
        raise ValueError('payload mapping unexpectedly changed')
    if verified.files != paths or verified.mount != MOUNT:
        raise ValueError('directory did not round-trip')
    # Every payload is read back, not just its directory listing.
    for i in range(len(toc.ids)):
        if verified.read(i) != toc.read(i):
            raise ValueError('payload mismatch at chunk ' + str(i))
    report = {'source': str(source), 'output': str(output), 'mount': MOUNT,
              'source_files': len(toc.files), 'result_files': len(paths),
              'payload_chunks_preserved': len(toc.ids),
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
    a = p.parse_args()
    r = normalize(a.input, a.aliases, a.out)
    print(json.dumps({k: v for k, v in r.items() if k != 'aliases_with_different_package_ids'}, indent=2))
    print('Aliases needing separate package IDs:', len(r.get('aliases_with_different_package_ids', [])))
