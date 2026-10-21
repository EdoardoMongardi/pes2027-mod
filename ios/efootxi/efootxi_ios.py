"""Host the supplied Efootxi gateway without changing its save/gameplay logic.

The game source/assets are packaged locally, never committed to this repository.
Every request has complete wire and decoded payloads under the launch log folder.
"""
from __future__ import annotations
import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
import threading
import time
import traceback
from urllib.parse import urlsplit, unquote

_gateway = None
_bundle = None
_session = None
_serial = 0
_lock = threading.RLock()
_native_live = None


def _json(value):
    if isinstance(value, bytes):
        return {"$bytes_base64": base64.b64encode(value).decode()}
    if isinstance(value, dict):
        return {str(k): _json(v) for k, v in value.items()}
    if isinstance(value, (tuple, list)):
        return [_json(v) for v in value]
    if value is None or isinstance(value, (str, bool, int, float)):
        return value
    return {"$repr": repr(value)}


def event(name, **value):
    value["time"] = time.time()
    with (_session / name).open("a", encoding="utf-8") as f:
        f.write(json.dumps(_json(value), ensure_ascii=False) + "\n")


def write_json(path, value):
    with path.open("w", encoding="utf-8") as f:
        json.dump(_json(value), f, ensure_ascii=False)


def _manifest(value):
    """Use genuine iOS CDN containers and hashes of the bytes we actually serve."""
    if isinstance(value, dict):
        result = {k: _manifest(v) for k, v in value.items()}
        if isinstance(result.get("livedata_list"), list) and "version_file" in result:
            version = json.loads(result["version_file"])
            version["version"] = max(int(version.get("version", 0)), 2026102102)
            # Retain the resource compatibility target captured with this release.
            version["dt870Hash"] = _native_live["dt870_mobile_ios.cpk"]["sha256"]
            version["dt870FileSize"] = _native_live["dt870_mobile_ios.cpk"]["size"]
            version["dt880"] = []
            for base in sorted({n.split("_mobile_ios.")[0] for n in _native_live if n.endswith(".utoc")}):
                entry = {"name": base}
                for ext, field in (("pak", "hash"), ("ucas", "ucas"), ("utoc", "utoc")):
                    record = _native_live[base + "_mobile_ios." + ext]
                    entry[field] = record["sha256"]
                    entry[field + "FileSize"] = record["size"]
                version["dt880"].append(entry)
            result["version_file"] = json.dumps(version, separators=(",", ":"))
            result["livedata_list"] = [dict(name=n, size=r["size"], url="http://127.0.0.1:8085/livedata/" + n)
                                       for n, r in sorted(_native_live.items())]
            event("livedata.jsonl", event="manifest", version=version, files=result["livedata_list"])
        return result
    if isinstance(value, list):
        return [_manifest(v) for v in value]
    return value


def configure(bundle, support, session):
    global _bundle, _session, _gateway, _native_live
    _bundle, _session = Path(bundle), Path(session)
    _session.mkdir(parents=True, exist_ok=True)
    (_session / "requests").mkdir(exist_ok=True)
    # Unbuffered stderr/stdout are also captured at the native file-descriptor level.
    sys.stdout.reconfigure(line_buffering=True)
    sys.stderr.reconfigure(line_buffering=True)
    import faulthandler
    faulthandler.enable(all_threads=True)
    event("python.jsonl", event="initializing", python=sys.version, bundle=bundle)
    target = Path(support) / "release-1.3.2-r1303" / "gateway-release"
    if not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        staging = target.with_name("gateway-release.staging")
        # Resume an interrupted first copy, then atomically publish it.
        shutil.copytree(_bundle / "gateway-release", staging, dirs_exist_ok=True)
        os.replace(staging, target)
    sys.path.insert(0, str(target / "work" / "tools"))
    records = json.loads((_bundle / "livedata" / "native-manifest.json").read_text())
    _native_live = {}
    for record in records:
        p = _bundle / "livedata" / record["name"]
        if p.stat().st_size != record["size"] or hashlib.file_digest(p.open("rb"), "sha256").hexdigest() != record["sha256"]:
            raise RuntimeError("iOS resource integrity failed: " + p.name)
        _native_live[p.name] = record
    import gateway_runtime
    gateway_runtime._android_port = lambda: 8085
    import embedded_gateway
    _gateway = embedded_gateway
    status = _gateway.initialize(str(target))
    original = _gateway._local_urls
    _gateway._local_urls = lambda value: _manifest(original(value))
    event("python.jsonl", event="ready", status=status, save=str(target))
    return status


def _resource(path):
    relative = unquote(urlsplit(path).path).removeprefix("/livedata/")
    root = (_bundle / "livedata").resolve()
    p = (root / relative).resolve()
    if not p.is_relative_to(root):
        return 403, "text/plain", b"Invalid resource path"
    if not p.is_file():
        # The release also serves generated banners from its outputs folder.
        for directory in ("outputs/efootball", "outputs/reference"):
            candidate = (Path(_gateway._root) / directory / relative).resolve()
            if candidate.is_relative_to((Path(_gateway._root) / directory).resolve()) and candidate.is_file():
                p = candidate
                break
    if not p.is_file():
        event("livedata.jsonl", event="missing", path=path)
        return 404, "text/plain", ("Missing resource: " + relative).encode()
    data = p.read_bytes()
    event("livedata.jsonl", event="served", path=path, actual=str(p), size=len(data), sha256=hashlib.sha256(data).hexdigest())
    return 200, "application/octet-stream", data


def serve(method, path, body):
    global _serial
    with _lock:
        _serial += 1
        prefix = _session / "requests" / ("%06d" % _serial)
        prefix.with_suffix(".request.bin").write_bytes(body)
        start = time.monotonic()
        event("network.jsonl", event="begin", id=_serial, method=method, path=path, bytes=len(body))
        try:
            if path.startswith("/livedata/"):
                status, mime, response = _resource(path)
            else:
                import pes_msg
                if "GateInfo" in path:
                    response_dict = dict(result="NOERR", msgid="GATE_GET_INFO", server_time=time.strftime("%Y-%m-%d %H:%M:%S"),
                                         gate_url="https://127.0.0.1:8085/pes22/gate/", base_url="https://127.0.0.1:8085")
                    response = pes_msg.encode_response(response_dict)
                else:
                    request = pes_msg.decode_request(body)
                    write_json(prefix.with_suffix(".request.json"), request)
                    response = _gateway.handle(path, body)
                    response_dict = pes_msg.decode_response(response)
                write_json(prefix.with_suffix(".response.json"), response_dict)
                status, mime = 200, "application/x-msgpack"
            prefix.with_suffix(".response.bin").write_bytes(response)
            event("network.jsonl", event="end", id=_serial, status=status, bytes=len(response), seconds=time.monotonic()-start)
            return status, mime, response
        except BaseException:
            details = traceback.format_exc()
            prefix.with_suffix(".exception.txt").write_text(details, encoding="utf-8")
            print(details, file=sys.stderr, flush=True)
            event("network.jsonl", event="failed", id=_serial, seconds=time.monotonic()-start, traceback=details)
            raise
