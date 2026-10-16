#!/usr/bin/env python3
"""Build PES2027.ipa (code + bootstrap) and PES2027-data-<id>.zip (heavy data) from local inputs.

Nothing Konami/Ziru-owned is downloaded or uploaded: every input comes from your own disk.

Inputs
  --app        Payload/PESmobile.app of eFootball iOS 11.1.1 (decrypted, build 11.1.101)
  --dylib      ZiruOffline.dylib built by CI (GitHub Actions)
  --apk        Efootball_Offline_VH.apk (Ziru) – source of assets/responses and assets/livedata
  --out        output directory

Outputs
  PES2027.ipa                   app without pak/ cpk/, with dylib + Ziru responses + patches
  PES2027-data-<data_id>.zip    pak/ cpk/ livedata/ + data-manifest.json (stored, not recompressed)
  build-report.json             hashes of inputs/outputs and every patch applied

Usage (Windows PowerShell):
  python build_ipa.py --app D:\\Telegram\\Payload\\PESmobile.app --dylib ZiruOffline.dylib `
      --apk "D:\\Efootball 2027 Offline Full Epic - Bigtime by Ziru\\Efootball_Offline_VH.apk" `
      --commentary D:\\Telegram\\dt510_eng_mobile_all.cpk --out D:\\PES2027\\out
"""
import argparse, hashlib, json, os, plistlib, shutil, struct, sys, time, zipfile

EXPECTED_EXE_SHA256 = "0cf9bacd9a9f049587f78a5fb8646b4ed5b87abe40a761d1f8f6337cafa9a0de"
EXPECTED_VERSION = ("11.1.1", "11.1.101")
DYLIB_INSTALL_NAME = b"@executable_path/Frameworks/ZiruOffline.dylib"
MOD_VERSION = "1.0.0"

# basic::PathGet(PathType): types 6 (cpk) and 7 (pak) are built from GetAppBundlePath() (x20).
# Switch them to GetDocumentsPath() (x21) so the game reads Documents/cpk and Documents/pak,
# which the dylib links to Library/Application Support/PES2027/GameData/<data_id>/.
TEXT_VMADDR = 0x100000000
PATCHES = [
    # (vmaddr, expected little-endian u32, new u32, description)
    (0x1000155BC, 0xF9000FF4, 0xF9000FF5, "PathGet cpk: str x20,[sp,#0x18] -> str x21"),
    (0x1000155C0, 0xAA1403E0, 0xAA1503E0, "PathGet cpk: mov x0,x20 -> mov x0,x21"),
    (0x10001561C, 0xF9000FF4, 0xF9000FF5, "PathGet pak: str x20,[sp,#0x18] -> str x21"),
    (0x100015620, 0xAA1403E0, 0xAA1503E0, "PathGet pak: mov x0,x20 -> mov x0,x21"),
]


def sha256_file(path, chunk=1 << 20):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def log(*a):
    print(time.strftime("%H:%M:%S"), *a, flush=True)


# ------------------------------------------------------------------ Mach-O patching
def patch_executable(data: bytearray, report):
    magic, cputype, _, _, ncmds, sizeofcmds, flags, _ = struct.unpack_from("<IiiIIIII", data, 0)
    if magic != 0xFEEDFACF or cputype != 0x0100000C:
        raise SystemExit("PESmobile is not a thin arm64 Mach-O")
    # locate __TEXT fileoff to map vmaddr->file offset, and check for existing load
    off = 32
    text_fileoff = None
    first_sect = 1 << 62
    for _ in range(ncmds):
        cmd, size = struct.unpack_from("<II", data, off)
        if cmd == 0x19:
            seg = data[off + 8:off + 24].rstrip(b"\0")
            vmaddr, vmsize, fileoff = struct.unpack_from("<QQQ", data, off + 24)
            if seg == b"__TEXT":
                text_fileoff = fileoff
            nsects = struct.unpack_from("<I", data, off + 64)[0]
            for s in range(nsects):
                so = off + 72 + s * 80
                o = struct.unpack_from("<I", data, so + 48)[0]
                if o:
                    first_sect = min(first_sect, o)
        if cmd in (0xC, 0x18, 0x8000001F):
            name = data[off + struct.unpack_from("<I", data, off + 8)[0]:off + size].split(b"\0")[0]
            if name == DYLIB_INSTALL_NAME:
                raise SystemExit("Executable already contains ZiruOffline load command (patched twice?)")
        off += size
    for vm, old, new, desc in PATCHES:
        fo = vm - TEXT_VMADDR + text_fileoff
        cur = struct.unpack_from("<I", data, fo)[0]
        if cur != old:
            raise SystemExit(f"Unexpected bytes at {hex(vm)}: {cur:#010x} (wanted {old:#010x}) – wrong executable?")
        struct.pack_into("<I", data, fo, new)
        report["patches"].append({"vmaddr": hex(vm), "old": f"{old:#010x}", "new": f"{new:#010x}", "desc": desc})
    # append LC_LOAD_DYLIB
    name = DYLIB_INSTALL_NAME + b"\0"
    cmdsize = (24 + len(name) + 7) & ~7
    end = 32 + sizeofcmds
    if end + cmdsize > first_sect:
        raise SystemExit("No room for an extra load command")
    lc = struct.pack("<IIIIII", 0xC, cmdsize, 24, 2, 0x00010000, 0x00010000) + name
    lc += b"\0" * (cmdsize - len(lc))
    if any(data[end:end + cmdsize]):
        raise SystemExit("Load-command padding is not empty")
    data[end:end + cmdsize] = lc
    struct.pack_into("<II", data, 16, ncmds + 1, sizeofcmds + cmdsize)
    report["patches"].append({"load_dylib": DYLIB_INSTALL_NAME.decode(), "cmdsize": cmdsize})


# ------------------------------------------------------------------ IPA
def build_ipa(app, dylib, apk, out, report, bundle_id, display_name):
    exe = os.path.join(app, "PESmobile")
    info_path = os.path.join(app, "Info.plist")
    with open(info_path, "rb") as f:
        info = plistlib.load(f)
    ver = (info.get("CFBundleShortVersionString"), info.get("CFBundleVersion"))
    if ver != EXPECTED_VERSION:
        raise SystemExit(f"App version {ver} != expected {EXPECTED_VERSION}")
    exe_sha = sha256_file(exe)
    report["inputs"]["PESmobile_sha256"] = exe_sha
    if exe_sha != EXPECTED_EXE_SHA256:
        raise SystemExit(f"PESmobile SHA-256 {exe_sha} does not match the analysed binary {EXPECTED_EXE_SHA256}")
    data = bytearray(open(exe, "rb").read())
    patch_executable(data, report)

    info["UIFileSharingEnabled"] = True
    info["LSSupportsOpeningDocumentsInPlace"] = True
    if bundle_id:
        info["CFBundleIdentifier"] = bundle_id
    if display_name:
        info["CFBundleDisplayName"] = display_name
        info["CFBundleName"] = display_name

    ipa = os.path.join(out, "PES2027.ipa")
    tmp = ipa + ".tmp"
    skip_top = {"pak", "cpk", "_CodeSignature", "SC_Info", "embedded.mobileprovision"}
    n = 0
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
        root = "Payload/PESmobile.app/"
        for dirpath, dirnames, filenames in os.walk(app):
            rel = os.path.relpath(dirpath, app).replace("\\", "/")
            top = rel.split("/")[0]
            if top in skip_top:
                dirnames[:] = []
                continue
            for fn in filenames:
                p = os.path.join(dirpath, fn)
                arc = root + (fn if rel == "." else f"{rel}/{fn}")
                if arc == root + "PESmobile":
                    zi = zipfile.ZipInfo(arc, time.localtime()[:6])
                    zi.external_attr = 0o100755 << 16
                    zi.compress_type = zipfile.ZIP_DEFLATED
                    z.writestr(zi, bytes(data))
                elif arc == root + "Info.plist":
                    z.writestr(arc, plistlib.dumps(info, fmt=plistlib.FMT_BINARY))
                else:
                    z.write(p, arc)
                n += 1
        zi = zipfile.ZipInfo(root + "Frameworks/ZiruOffline.dylib", time.localtime()[:6])
        zi.external_attr = 0o100755 << 16
        zi.compress_type = zipfile.ZIP_DEFLATED
        z.writestr(zi, open(dylib, "rb").read())
        # Ziru responses (JSON templates) from the APK
        with zipfile.ZipFile(apk) as a:
            for e in a.infolist():
                if e.filename.startswith("assets/responses/") and not e.is_dir():
                    z.writestr(root + "ziru/responses/" + e.filename[len("assets/responses/"):], a.read(e))
                    n += 1
    os.replace(tmp, ipa)
    report["outputs"]["ipa"] = {"path": ipa, "size": os.path.getsize(ipa), "sha256": sha256_file(ipa), "files": n}
    log(f"IPA written: {ipa} ({os.path.getsize(ipa) / 1e6:.1f} MB)")


# ------------------------------------------------------------------ data pack
def build_data(app, apk, out, report, include_livedata=True, ziru_cpk=("dt500",), commentary=None):
    files = []  # (arcname, source spec)
    apk_names = set(zipfile.ZipFile(apk).namelist())
    for sub in ("pak", "cpk"):
        d = os.path.join(app, sub)
        for fn in sorted(os.listdir(d)):
            stem = fn.split("_mobile_")[0]
            src = ("fs", os.path.join(d, fn))
            if sub == "cpk" and stem in ziru_cpk and fn.endswith("_mobile_all.cpk") and f"assets/{fn}" in apk_names:
                # Ziru's CPK (platform-independent "mobile_all"): dt500 carries 604 extra uniform files
                src = ("apk", f"assets/{fn}")
                report.setdefault("cpk_from_ziru", []).append(fn)
            files.append((f"{sub}/{fn}", src))
    if commentary:
        files.append(("download/" + os.path.basename(commentary), ("fs", commentary)))
        report["commentary"] = os.path.abspath(commentary)
    if include_livedata:
        with zipfile.ZipFile(apk) as a:
            for e in a.infolist():
                if e.filename.startswith("assets/livedata/") and not e.is_dir():
                    files.append(("livedata/" + e.filename[len("assets/livedata/"):], ("apk", e.filename)))
    log(f"hashing {len(files)} data files…")
    recs = []
    apkz = zipfile.ZipFile(apk)
    for arc, (kind, src) in files:
        if kind == "fs":
            size, sha = os.path.getsize(src), sha256_file(src)
        else:
            h = hashlib.sha256(); size = 0
            with apkz.open(src) as fi:
                for b in iter(lambda: fi.read(1 << 20), b""):
                    h.update(b); size += len(b)
            sha = h.hexdigest()
        recs.append({"path": arc, "size": size, "sha256": sha})
    digest = hashlib.sha256(json.dumps(recs, sort_keys=True).encode()).hexdigest()[:12]
    data_id = f"pes2027-{EXPECTED_VERSION[1]}-{digest}"
    manifest = {"format": 1, "data_id": data_id, "client_version": EXPECTED_VERSION[0], "client_build": EXPECTED_VERSION[1],
                "mod_version": MOD_VERSION, "created": time.strftime("%Y-%m-%dT%H:%M:%S%z"), "files": recs}
    path = os.path.join(out, f"PES2027-data-{data_id}.zip")
    tmp = path + ".tmp"
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_STORED, allowZip64=True) as z:
        z.writestr("data-manifest.json", json.dumps(manifest, indent=1))
        for arc, (kind, src) in files:
            log("  +", arc)
            if kind == "fs":
                z.write(src, arc)
            else:
                zi = zipfile.ZipInfo(arc, time.localtime()[:6])
                with apkz.open(src) as fi, z.open(zi, "w", force_zip64=True) as fo:
                    shutil.copyfileobj(fi, fo, 1 << 20)
    os.replace(tmp, path)
    with open(os.path.join(out, "data-manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
    total = sum(r["size"] for r in recs)
    report["outputs"]["data"] = {"path": path, "data_id": data_id, "files": len(recs), "payload_bytes": total, "zip_size": os.path.getsize(path)}
    log(f"data pack written: {path} ({os.path.getsize(path) / 1e9:.2f} GB, {len(recs)} files)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--app", required=True)
    ap.add_argument("--dylib", required=True)
    ap.add_argument("--apk", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--bundle-id", default="", help="optional new CFBundleIdentifier (keep empty to let your signer decide)")
    ap.add_argument("--display-name", default="PES 2027")
    ap.add_argument("--skip-data", action="store_true", help="only build the IPA")
    ap.add_argument("--no-livedata", action="store_true", help="do not ship Ziru's Android livedata in the data pack")
    ap.add_argument("--ziru-cpk", default="dt500", help="comma list of CPK stems taken from the Ziru APK instead of the IPA (default dt500; '' = none)")
    ap.add_argument("--commentary", default="", help="commentary CPK to install into the game's Download folder, e.g. D:\\Telegram\\dt510_eng_mobile_all.cpk (Vietnamese)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    report = {"tool": "build_ipa.py", "mod_version": MOD_VERSION, "time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
              "inputs": {"app": os.path.abspath(a.app), "dylib": os.path.abspath(a.dylib), "dylib_sha256": sha256_file(a.dylib),
                         "apk": os.path.abspath(a.apk)},
              "patches": [], "outputs": {}}
    build_ipa(a.app, a.dylib, a.apk, a.out, report, a.bundle_id, a.display_name)
    if not a.skip_data:
        build_data(a.app, a.apk, a.out, report, include_livedata=not a.no_livedata,
                   ziru_cpk=tuple(x for x in a.ziru_cpk.split(",") if x), commentary=a.commentary or None)
    with open(os.path.join(a.out, "build-report.json"), "w") as f:
        json.dump(report, f, indent=1)
    log("done. Sign PES2027.ipa with your usual tool (Sideloadly/ESign/…), install, then import the data zip in the app.")


if __name__ == "__main__":
    main()
