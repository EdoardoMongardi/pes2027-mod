"""Convert legacy named Playstyle/PlayerSkill tables to iOS 11.1.1 records.

Only these tables change; every other CPK byte is preserved. Layout evidence:
pesdb::Manager record-size table at 0x105784d38, entries 42 and 43 = 4;
LinkPesdb::ImportPlayStyleData at 0x104ba1048 advances the source by 4.
Input and output are local user-owned assets, never repository fixtures.
"""
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path
from cpk_toc import utf_table


def unpack_wesys(data):
    if data[3:8] != b"WESYS":
        raise ValueError("not WESYS")
    compressed, size = struct.unpack_from("<II", data, 8)
    payload = bytearray(data[16:16 + compressed])
    if len(payload) != compressed:
        raise ValueError("truncated WESYS")
    if data[2] & 2:
        if data[1] & 15 != 2:
            raise ValueError("unsupported WESYS crypt version")
        # sys::File::PostProcessMainFunc, iOS 11.1.1 0x104b7b4e4..588.
        x, y, z = 0xed5b2960, 0x4a523b4e, 0xf3a31bad
        w = (compressed | size << 16) & 0xffffffff
        for p in range(0, compressed // 4 * 4, 4):
            t = (x ^ x << 11) & 0xffffffff
            x, y, z = y, z, w
            w = (w ^ w >> 19 ^ t ^ t >> 8) & 0xffffffff
            struct.pack_into("<I", payload, p, struct.unpack_from("<I", payload, p)[0] ^ w)
    raw = zlib.decompress(payload) if data[2] & 1 else bytes(payload)
    if len(raw) != size:
        raise ValueError("WESYS size mismatch")
    return raw


def compact_table(raw):
    # Legacy records contain a 4-byte bitfield followed by localized names.
    # Require consecutive IDs and name/padding bytes to avoid guessing a stride.
    if len(raw) % 168 or not raw:
        return raw
    count = len(raw) // 168
    if not 30 <= count <= 100:
        return raw
    records = [raw[i:i + 168] for i in range(0, len(raw), 168)]
    ids = [struct.unpack_from("<I", r)[0] & 63 for r in records]
    # Skill IDs can be wider than the six-bit playstyle ID.
    ids8 = [struct.unpack_from("<I", r)[0] & 255 for r in records]
    if ids != list(range(count)) and ids8 != list(range(count)):
        raise ValueError("unrecognized legacy table IDs")
    return b"".join(r[:4] for r in records)


def repair_cpk_bytes(data):
    result = bytearray(data)
    header = utf_table(data[16:0x800])[0]
    toc = header["TocOffset"]
    length = struct.unpack_from("<Q", data, toc + 8)[0]
    rows = utf_table(data[toc + 16:toc + 16 + length])
    changes = []
    for row in rows:
        if row.get("FileName") not in ("Playstyle.bin", "PlayerSkill.bin"):
            continue
        offset = min(toc, header["ContentOffset"]) + row["FileOffset"]
        size = row["FileSize"]
        if size != row["ExtractSize"]:
            raise ValueError("CPK-compressed table unsupported")
        old = unpack_wesys(data[offset:offset + size])
        new = compact_table(old)
        if new == old:
            continue
        payload = zlib.compress(new, 9)
        # Native WESYS explicitly supports compressed, unencrypted payloads.
        packed = b"\x00\x01\x01WESYS" + struct.pack("<II", len(payload), len(new)) + payload
        if len(packed) > size:
            raise ValueError("replacement exceeds table allocation")
        result[offset:offset + size] = packed.ljust(size, b"\0")
        changes.append({"table": row["FileName"], "before": len(old), "after": len(new), "records": len(new) // 4})
    return bytes(result), changes


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()
    original = args.input.read_bytes()
    fixed, changes = repair_cpk_bytes(original)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.resolve() == args.input.resolve():
        raise SystemExit("use a separate output path")
    args.output.write_bytes(fixed)
    print(json.dumps({"changes": changes, "sha256": hashlib.sha256(fixed).hexdigest()}, indent=2))
