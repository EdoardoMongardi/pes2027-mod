"""Read-only inventory of source containers and IPA bundles. Never infer iOS
compatibility from a renamed file, a valid directory index, or a pack number.
"""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from collections import Counter, defaultdict
from pathlib import Path

from make_extra_pak import Toc, parse_header


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(4 * 1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def inventory_container(path):
    record = {"path": str(path), "sha256": sha256(path)}
    record["companions"] = {
        ext: {"exists": path.with_suffix(ext).is_file(),
              "size": path.with_suffix(ext).stat().st_size if path.with_suffix(ext).is_file() else None}
        for ext in (".pak", ".utoc", ".ucas")}
    try:
        toc = Toc(str(path))
        paths = sorted(toc.files)
        record.update(container_id=f"{struct.unpack_from('<Q', toc.raw, 56)[0]:016x}",
                      file_count=len(paths), chunk_count=len(toc.ids), mount=toc.mount,
                      paths=paths,
                      legends=[p for p in paths if re.search(r"(?:37576|9901397|zidane|ronaldo)", p, re.I)],
                      player_photos=sum('/thumbnail/player/' in p.lower() for p in paths),
                      real_faces=sum('/real/' in p.lower() for p in paths),
                      emblems=sum('/symbol/emblem' in p.lower() for p in paths),
                      uniforms=sum('/uniform/' in p.lower() for p in paths))
        headers = [i for i, chunk in enumerate(toc.ids) if chunk[11] == 10]
        if headers and path.with_suffix('.ucas').is_file():
            header = parse_header(toc.read(headers[0]))
            record['package_count'] = header['cnt']
    except Exception as exc:
        record['error'] = f'{type(exc).__name__}: {exc}'
    return record


def inventory_ipa(path):
    record = {'path': str(path), 'size': path.stat().st_size, 'sha256': sha256(path)}
    with zipfile.ZipFile(path) as z:
        names = z.namelist()
        counts = Counter(names)
        record['duplicate_entries'] = [n for n, count in counts.items() if count > 1]
        record['commandlines'] = {n: z.read(n).decode('utf-8', errors='replace')
                                  for n in names if 'commandline.txt' in n.lower()}
        record['resources'] = [{'path': n, 'size': z.getinfo(n).file_size,
                                'sha256': hashlib.sha256(z.read(n)).hexdigest()}
                               for n in names if n.endswith(('.utoc', '.cpk', '/PESmobile', 'ZiruOffline.dylib', 'table-fixes.json'))]
        record['has_offline_dylib'] = any(n.endswith('ZiruOffline.dylib') for n in names)
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', action='append', default=[])
    parser.add_argument('--ipa', action='append', default=[])
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    paths = sorted({p.resolve() for root in args.root for p in Path(root).rglob('*.utoc')})
    containers = []
    for path in paths:
        result = inventory_container(path)
        containers.append(result)
        print(f"{path}: {result.get('file_count', '?')} entries; {result.get('error', 'index parsed; platform unverified')}", flush=True)
    by_hash = defaultdict(list)
    for container in containers:
        by_hash[container['sha256']].append(container['path'])
    report = {'containers': containers, 'byte_identical_indices': [v for v in by_hash.values() if len(v) > 1],
              'ipas': [inventory_ipa(Path(p)) for p in args.ipa],
              'limitations': ['Index parsing does not validate target-platform shaders, materials or mesh serialization.',
                              'Matching UTOC hashes prove identical indices; payload equality requires UCAS hashes.',
                              'No file on the device or in source folders was changed.']}
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f"Saved {len(containers)} containers and {len(report['ipas'])} IPA inventories to {out}")


if __name__ == '__main__':
    main()
