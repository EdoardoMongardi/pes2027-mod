"""Combine a complete baseline CPK with a mod overlay, preserving every file.
Duplicate paths take the overlay's bytes. No database records are guessed or
removed. Unsupported auxiliary indices and CRC modes are rejected.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from cpk_toc import decrypt_utf, utf_table
from repair_ios_tables import repair_cpk_bytes

FORMATS = {0: '>B', 1: '>b', 2: '>H', 3: '>h', 4: '>I', 5: '>i', 6: '>Q', 7: '>q', 8: '>f', 9: '>d'}


def utf_schema(packet):
    if packet[:4] != b'@UTF':
        packet = decrypt_utf(packet)
    t = packet[8:]
    strings = struct.unpack_from('>I', t, 4)[0]
    def text(offset):
        start = strings + offset
        return t[start:t.index(b'\0', start)].decode('utf-8')
    name = text(struct.unpack_from('>I', t, 12)[0])
    count = struct.unpack_from('>H', t, 16)[0]
    result, pos = [], 24
    for _ in range(count):
        flags = t[pos]
        column = text(struct.unpack_from('>I', t, pos + 1)[0])
        typ = flags & 15
        result.append((column, typ))
        pos += 5
        if flags & 0xf0 == 0x30:
            pos += struct.calcsize(FORMATS[typ]) if typ in FORMATS else (4 if typ == 10 else 8)
    return name, result


def encode_utf(name, columns, rows):
    strings, strings_index, binary = bytearray(), {}, bytearray()
    def string(value):
        value = value or ''
        if value not in strings_index:
            strings_index[value] = len(strings)
            strings.extend(value.encode('utf-8') + b'\0')
        return strings_index[value]
    name_offset = string(name)
    schema, values = bytearray(), bytearray()
    active = []
    for column, typ in columns:
        used = any(row.get(column) is not None for row in rows)
        schema += bytes([(0x50 if used else 0x10) | typ]) + struct.pack('>I', string(column))
        active.append((column, typ, used))
    for row in rows:
        for column, typ, used in active:
            if not used:
                continue
            value = row.get(column)
            if typ in FORMATS:
                values += struct.pack(FORMATS[typ], value or 0)
            elif typ == 10:
                values += struct.pack('>I', string(value))
            elif typ == 11:
                if isinstance(value, tuple):
                    if value[2] != 0:
                        raise ValueError('nonempty metadata blob requires explicit bytes')
                    value = b''
                value = value or b''
                values += struct.pack('>II', len(binary), len(value))
                binary += value
            else:
                raise ValueError('unsupported UTF type ' + str(typ))
    row_length = len(values) // len(rows) if rows else 0
    rows_offset = 24 + len(schema)
    strings_offset = rows_offset + len(values)
    data_offset = strings_offset + len(strings)
    body = struct.pack('>IIIIHHI', rows_offset, strings_offset, data_offset, name_offset,
                       len(columns), row_length, len(rows)) + schema + values + strings + binary
    return b'@UTF' + struct.pack('>I', len(body)) + body


def read_cpk(data):
    if data[:4] != b'CPK ':
        raise ValueError('not CPK')
    size = struct.unpack_from('<Q', data, 8)[0]
    header_packet = data[16:16 + size]
    header = utf_table(header_packet)[0]
    for key in ('HtocOffset', 'EtocOffset', 'ItocOffset', 'GtocOffset', 'HgtocOffset', 'EnableTocCrc', 'EnableFileCrc'):
        if header.get(key):
            raise ValueError('unsupported CPK feature ' + key)
    offset = header['TocOffset']
    size = struct.unpack_from('<Q', data, offset + 8)[0]
    packet = data[offset + 16:offset + 16 + size]
    files = {}
    for row in utf_table(packet):
        start = min(offset, header['ContentOffset']) + row['FileOffset']
        payload = data[start:start + row['FileSize']]
        if len(payload) != row['FileSize']:
            raise ValueError('truncated payload')
        key = (row['DirName'], row['FileName'])
        if key in files:
            raise ValueError('duplicate CPK path')
        files[key] = (row, payload)
    return header, utf_schema(header_packet), utf_schema(packet), files


def merge(baseline, overlay):
    header, header_schema, toc_schema, files = read_cpk(baseline)
    toc_prefix = baseline[header['TocOffset'] + 4:header['TocOffset'] + 8]
    baseline_ids = {key: row.get('ID') for key, (row, _) in files.items()}
    used_ids = {value for value in baseline_ids.values() if value is not None}
    if len(used_ids) != sum(value is not None for value in baseline_ids.values()):
        raise ValueError('baseline has duplicate file IDs')
    next_id = max(used_ids, default=-1) + 1
    _, _, overlay_schema, additions = read_cpk(overlay)
    if toc_schema != overlay_schema:
        raise ValueError('different TOC schemas')
    preserved = sorted(set(files) - set(additions))
    files.update(additions)
    alignment = header.get('Align') or 2048
    align = lambda n: (n + alignment - 1) // alignment * alignment
    rows, payload = [], bytearray()
    for key in sorted(files):
        row, data = files[key]
        row = dict(row)
        if key in baseline_ids:
            row['ID'] = baseline_ids[key]
        elif row.get('ID') is not None:
            if row['ID'] in used_ids:
                row['ID'] = next_id
                next_id += 1
            used_ids.add(row['ID'])
            next_id = max(next_id, row['ID'] + 1)
        row['FileOffset'] = len(payload)  # rebased once TOC size is known
        rows.append(row)
        payload += data
        payload += b'\0' * (align(len(payload)) - len(payload))
    toc_offset = 2048
    toc = encode_utf(*toc_schema, rows)
    content_offset = align(toc_offset + 16 + len(toc))
    for row in rows:
        row['FileOffset'] += content_offset - toc_offset
    toc = encode_utf(*toc_schema, rows)
    header = dict(header, TocOffset=toc_offset, TocSize=16 + len(toc),
                  ContentOffset=content_offset, ContentSize=len(payload), Files=len(rows),
                  EnabledPackedSize=sum(r['FileSize'] for r in rows),
                  EnabledDataSize=sum(r['ExtractSize'] for r in rows), Sorted=1)
    packet = encode_utf(*header_schema, [header])
    if len(packet) + 16 > toc_offset:
        raise ValueError('header exceeds CPK allocation')
    output = bytearray(b'CPK ' + baseline[4:8] + struct.pack('<Q', len(packet)) + packet)
    output += b'\0' * (toc_offset - len(output))
    output += b'TOC ' + toc_prefix + struct.pack('<Q', len(toc)) + toc
    output += b'\0' * (content_offset - len(output))
    output += payload
    fixed, repairs = repair_cpk_bytes(bytes(output))
    _, _, _, verified = read_cpk(fixed)
    changed_tables = {r['table'] for r in repairs}
    if set(verified) != set(files):
        raise ValueError('file list changed during round-trip')
    for key, (_, original) in files.items():
        if key[1] not in changed_tables and verified[key][1] != original:
            raise ValueError('payload changed: ' + repr(key))
    return fixed, {'files': len(files), 'baseline_only_files_preserved': preserved,
                   'table_repairs': repairs, 'sha256': hashlib.sha256(fixed).hexdigest()}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--overlay', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.out.resolve() in (args.baseline.resolve(), args.overlay.resolve()):
        raise SystemExit('use a separate output path')
    output, report = merge(args.baseline.read_bytes(), args.overlay.read_bytes())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(output)
    args.out.with_suffix('.audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Merged', report['files'], 'files; preserved', len(report['baseline_only_files_preserved']), 'baseline-only files; SHA256', report['sha256'])
