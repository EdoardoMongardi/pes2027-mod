"""Small data-pack ZIPs for test/datapacktest.cpp.

usage: python3 test/datapack_fixtures.py DIR

  root.zip    data-manifest.json, pak/, cpk/ at the top of the ZIP (the documented layout)
  folder.zip  everything inside one folder, the way PES2027-data.zip is made by compressing the
              PES2027-data folder: folder entries, the manifest stored last
  finder.zip  like folder.zip plus the __MACOSX copies macOS Finder adds
  two.zip     two folders, each with a manifest (ambiguous: refused)
  deep.zip    manifest two folders down (refused)
  none.zip    no manifest at all (refused)
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


def inside(folder, data_id, finder=False):
    out = [(folder + "/", b"")]
    for sub in ("cpk", "livedata", "pak"):
        out.append((f"{folder}/{sub}/", b""))
        out += [(f"{folder}/{p}", d) for p, d in FILES.items() if p.startswith(sub + "/")]
    out.append((f"{folder}/data-manifest.json", manifest(data_id)))
    if finder:
        out += [("__MACOSX/", b""), (f"__MACOSX/{folder}/", b""),
                (f"__MACOSX/{folder}/._data-manifest.json", b"\0\5\26\7AppleDouble"),
                (f"__MACOSX/._{folder}", b"\0\5\26\7AppleDouble")]
    return out


write("root.zip", [("data-manifest.json", manifest("ci-root"))] + list(FILES.items()))
write("folder.zip", inside("PES2027-data", "ci-folder"))
write("finder.zip", inside("PES2027-data", "ci-finder", finder=True))
write("two.zip", inside("A", "ci-a") + inside("B", "ci-b"))
write("deep.zip", [("x/y/data-manifest.json", manifest("ci-deep"))] + [("x/y/" + p, d) for p, d in FILES.items()])
write("none.zip", list(FILES.items()))
