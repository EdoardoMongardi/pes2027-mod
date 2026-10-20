"""Build a separate repair candidate from an explicitly identified existing IPA.
Never overwrite inputs. The original executable and unrelated bundle files are
preserved; signing and device visual validation are separate steps.
"""
import argparse
import hashlib
import json
import zipfile
from pathlib import Path

ROOT = 'Payload/PESmobile.app/'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def build(args):
    if args.out.resolve() in (args.ipa.resolve(), args.dylib.resolve()):
        raise ValueError('output must differ from inputs')
    if args.out.exists():
        raise ValueError('output already exists; choose a new candidate name')
    dylib = args.dylib.read_bytes()
    checksum = args.dylib.with_suffix('.dylib.sha256').read_text().split()[0]
    if digest(dylib) != checksum:
        raise ValueError('CI artifact checksum mismatch')
    with zipfile.ZipFile(args.ipa) as source:
        names = source.namelist()
        if len(names) != len(set(names)):
            raise ValueError('input IPA has duplicate paths')
        executable = source.read(ROOT + 'PESmobile')
        if digest(executable) != args.exe_sha256:
            raise ValueError('unrecognized executable; do not patch offsets for another version')
        replacements = {ROOT + 'Frameworks/ZiruOffline.dylib': dylib}
        repairs = {'pak_extra': {}}
        previous_dirs = [Path(p) for p in args.previous_extra]
        for path in args.extra.iterdir():
            if path.suffix not in ('.pak', '.utoc', '.ucas'):
                continue
            data = path.read_bytes()
            destination = ROOT + 'ziru/pak_extra/' + path.name
            originals = {digest(source.read(destination))} if destination in names else set()
            for directory in previous_dirs:
                previous = directory / path.name
                if previous.is_file():
                    originals.add(digest(previous.read_bytes()))
            repairs['pak_extra'][path.name] = {'sha256': digest(data), 'original_sha256': sorted(originals)}
            replacements[destination] = data
        existing_extra = {Path(n).name for n in names if '/pak_extra/' in n and n.endswith(('.pak', '.ucas', '.utoc'))}
        for name in existing_extra - set(repairs['pak_extra']):
            destination = ROOT + 'ziru/pak_extra/' + name
            data = source.read(destination)
            repairs['pak_extra'][name] = {'sha256': digest(data)}
        replacements[ROOT + 'ziru/resource-repairs.json'] = json.dumps(repairs, indent=2).encode()
        cpk = args.cpk.read_bytes()
        previous_hashes = {digest(Path(p).read_bytes()) for p in args.previous_cpk}
        for n in names:
            if '/livedata/dt870' in n and n.endswith('.cpk'):
                previous_hashes.add(digest(source.read(n)))
        tables = {}
        for name in ('dt870_mobile_and.cpk', 'dt870_mobile_ios.cpk'):
            tables[name] = {'sha256': digest(cpk), 'original_sha256': sorted(previous_hashes),
                            'source': 'baseline-plus-legends-union', 'merge_audit': args.cpk.with_suffix('.audit.json').name}
            replacements[ROOT + 'ziru/livedata/' + name] = cpk
        replacements[ROOT + 'ziru/table-fixes.json'] = json.dumps(tables, indent=2).encode()
        build_info = {'source_commit': args.commit, 'ci_run': args.ci_run,
                      'source_ipa_sha256': digest(args.ipa.read_bytes()),
                      'executable_sha256': digest(executable), 'dylib_sha256': digest(dylib),
                      'status': 'repair candidate; device rendering unverified'}
        replacements[ROOT + 'ziru/repair-build.json'] = json.dumps(build_info, indent=2).encode()
        args.out.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(args.out, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as target:
            for info in source.infolist():
                if '/_CodeSignature/' in info.filename or info.filename.endswith('/embedded.mobileprovision'):
                    continue
                target.writestr(info, replacements.pop(info.filename, source.read(info.filename)))
            for name, data in replacements.items():
                info = zipfile.ZipInfo(name)
                info.external_attr = 0o100644 << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                target.writestr(info, data)
    with zipfile.ZipFile(args.ipa) as original, zipfile.ZipFile(args.out) as result:
        if result.testzip() is not None or len(result.namelist()) != len(set(result.namelist())):
            raise ValueError('invalid output archive')
        if result.read(ROOT + 'PESmobile') != executable:
            raise ValueError('executable unexpectedly changed')
        changed = []
        for name in original.namelist():
            if name in result.namelist() and result.read(name) != original.read(name):
                changed.append(name)
                if not (name.startswith(ROOT + 'ziru/') or name == ROOT + 'Frameworks/ZiruOffline.dylib'):
                    raise ValueError('unrelated bundle file changed: ' + name)
        build_info.update(output=str(args.out), output_size=args.out.stat().st_size,
                          output_sha256=digest(args.out.read_bytes()), changed_existing_entries=changed,
                          zip_crc_verified=True, executable_unchanged=True,
                          removed_signing_metadata=True,
                          limitations=['Requires signing before installation.', 'Native base packs must still be audited/restored on device.',
                                       'Directory aliases with incorrect package IDs are not new assets.'])
        args.out.with_suffix('.json').write_text(json.dumps(build_info, indent=2), encoding='utf-8')
    print(json.dumps(build_info, indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ipa', type=Path, required=True)
    p.add_argument('--dylib', type=Path, required=True)
    p.add_argument('--extra', type=Path, required=True)
    p.add_argument('--cpk', type=Path, required=True)
    p.add_argument('--previous-extra', action='append', default=[])
    p.add_argument('--previous-cpk', action='append', default=[])
    p.add_argument('--exe-sha256', required=True)
    p.add_argument('--commit', required=True)
    p.add_argument('--ci-run', required=True)
    p.add_argument('--out', type=Path, required=True)
    build(p.parse_args())
