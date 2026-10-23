"""Small data-pack ZIPs for test/datapacktest.cpp.

usage: python3 test/datapack_fixtures.py DIR

  root.zip    data-manifest.json, pak/, cpk/ at the top of the ZIP (the documented layout)
"""
import hashlib
import json
import pathlib
import sys
import zipfile

root = pathlib.Path(sys.argv[1])
root.mkdir(parents=True, exist_ok=True)

FILES = {
    "pak/global_mobile_ios.utoc": b"utoc" * 100,
    "pak/global_mobile_ios.ucas": b"ucas" * 300,
    "pak/pc0000_mobile_ios.ucas": b"base" * 500,
    "cpk/dt200_mobile_all.cpk": b"cpk!" * 400,
    "livedata/dt870_mobile_and.cpk": b"live" * 50,
}


def manifest(data_id):
    files = [{"path": p, "size": len(d), "sha256": hashlib.sha256(d).hexdigest()} for p, d in FILES.items()]
    return json.dumps({"format": 1, "data_id": data_id, "client_version": "11.1.1", "client_build": "11.1.101",
                       "mod_version": "1.0.0", "files": files}).encode()


def write(name, entries):
    with zipfile.ZipFile(root / name, "w", compression=zipfile.ZIP_DEFLATED) as z:
        for path, data in entries:
            if path.endswith("/"):
                z.writestr(zipfile.ZipInfo(path), b"")
            else:
                z.writestr(path, data)


write("root.zip", [("data-manifest.json", manifest("ci-root"))] + list(FILES.items()))
