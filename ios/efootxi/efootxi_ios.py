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
_country_names = json.loads(Path(__file__).with_name("country_names.json").read_text(encoding="utf-8"))
_banner_names = frozenset(("efootxi-home-players.jpg", "efootxi-home-coaches.jpg"))
_banner_hashes = {}
_agent_count = None


def _resource_path(path):
    """Accept native CDN-relative banners and the old malformed CDN + URL form."""
    path = unquote(urlsplit(path).path)
    if path.startswith("/livedata/"):
        return path
    name = path.rsplit("/", 1)[-1]
    if name in _banner_hashes and path.startswith(("/prd/efootxi/", "/efootxi/")):
        return "/livedata/" + _banner_hashes[name]
    if name in _banner_names and (path.startswith("/prd/efootxi/") or
            path.startswith("/efootxi/") or
            path.startswith("/prdhttp://127.0.0.1:8085/livedata/")):
        return "/livedata/" + name
    return None


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
        if isinstance(result.get("country_list"), list):
            # The Android release supplies CJK labels; the installed iOS font
            # replacement only covers Latin/Vietnamese. Preserve codes/policy.
            for country in result["country_list"]:
                if isinstance(country, dict):
                    code = country.get("country_code")
                    country["country_name"] = _country_names.get(code, code or country.get("country_name", ""))
        banner = result.get("banner_file_name")
        if isinstance(banner, str):
            name = urlsplit(banner).path.rsplit("/", 1)[-1]
            if name in _banner_names:
                # iOS prepends its CDN root and /prd. Absolute URLs here are
                # concatenated literally, not resolved as a URL.
                digest = next((h for h, n in _banner_hashes.items() if n == name), None)
                # Native image caches treat the last URL component as a file
                # identity. Match the CDN's hash-only names, without ?rev text.
                result["banner_file_name"] = "/efootxi/" + (digest or name)
                if name == 'efootxi-home-players.jpg':
                    result.update(title_text='Cầu thủ đặc biệt', description_text='Các pack và cầu thủ mới nhất')
                else:
                    result.update(title_text='Huấn luyện viên', description_text='Các huấn luyện viên mới nhất')
                result['note_text'] = 'efootxi · https://t.me/efootxi'
        from efootxi_catalogue import LABELS
        if 'agent_num' in result and _agent_count is not None:
            result['agent_num'] = _agent_count
        if result.get('agent_id') in LABELS and 'title' in result:
            result['title'] = LABELS[result['agent_id']]
            if 'description' in result:
                result['description'] = 'Cầu thủ trong pack theo giải đấu và vị trí.'
        elif 'agent_id' in result and isinstance(result.get('title'), str):
            result['title'] = result['title'].replace('（', ' (').replace('人）', ' players)')
        if isinstance(result.get("livedata_list"), list) and "version_file" in result:
            version = json.loads(result["version_file"])
            # Bumped when the bundled livedata set changes (pc9290-9292 supplemental
            # containers) so the client re-reads the dt880 list and mounts them.
            version["version"] = max(int(version.get("version", 0)), 2026102202)
            # Native iOS IsPassAppVer rejects the Android release's 6.1.0 target.
            version.pop("targetAppVersion", None)
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
    global _bundle, _session, _gateway, _native_live, _banner_hashes, _agent_count
    _bundle, _session = Path(bundle), Path(session)
    _session.mkdir(parents=True, exist_ok=True)
    (_session / "requests").mkdir(exist_ok=True)
    # Unbuffered stderr/stdout are also captured at the native file-descriptor level.
    if sys.stdout is not None:
        try:
            sys.stdout.reconfigure(line_buffering=True)
        except Exception:
            pass
    if sys.stderr is not None:
        try:
            sys.stderr.reconfigure(line_buffering=True)
        except Exception:
            pass
    try:
        import faulthandler
        faulthandler.enable(all_threads=True)
    except BaseException:
        pass
    event("python.jsonl", event="initializing", python=sys.version, bundle=bundle)
    target = Path(support) / "release-1.3.2-r1303" / "gateway-release"
    if not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        staging = target.with_name("gateway-release.staging")
        # Resume an interrupted first copy, then atomically publish it.
        shutil.copytree(_bundle / "gateway-release", staging, dirs_exist_ok=True)
        os.replace(staging, target)
    bundle_tools = _bundle / "gateway-release" / "work" / "tools"
    if bundle_tools.is_dir():
        sys.path.insert(0, str(bundle_tools))
    sys.path.insert(1, str(target / "work" / "tools"))
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
    from efootxi_catalogue import install, active_count
    restored = install(target.resolve())
    _agent_count = active_count(target.resolve())
    event('python.jsonl', event='catalogue_restored', agent_ids=restored)
    try:
        from efootxi_catalogue import fix_coach_linkups
        event('python.jsonl', event='coach_linkups_fixed', coaches=fix_coach_linkups(target.resolve()))
    except Exception as error:
        event('python.jsonl', event='coach_linkup_fix_error', error=repr(error))
    try:
        from efootxi_catalogue import trim_factory_gifts
        event('python.jsonl', event='factory_gifts_trimmed', result=trim_factory_gifts(target.resolve()))
        from efootxi_catalogue import hide_artless_cards
        event('python.jsonl', event='artless_cards_hidden', result=hide_artless_cards(target.resolve()))
        _agent_count = active_count(target.resolve())
    except Exception as error:
        event('python.jsonl', event='factory_gift_trim_error', error=repr(error))
    _banner_hashes = {}
    for name in _banner_names:
        image = target / 'livedata' / name
        if image.is_file():
            _banner_hashes[hashlib.sha256(image.read_bytes()).hexdigest()] = name
    original = _gateway._local_urls
    _gateway._local_urls = lambda value: _manifest(original(value))
    event("python.jsonl", event="ready", status=status, save=str(target))
    return status


_data_root = None
_STADIUM_VERSION = 2026081300
_STADIUM_CACHE = ".efootxi-stadium-hashes.json"


def _stadium_name(name):
    """Native iOS stadium container parts only; pc3001/pc3011 belong to the base."""
    import re
    m = re.fullmatch(r"(pc30\d\d)_mobile_ios\.(pak|ucas|utoc)", name)
    return m and m.group(1) not in ("pc3001", "pc3011")


def _stage_stadiums(root, original_documents, emit):
    """Collect user-supplied iOS stadium packs into <root>/Download and hash them once.

    Android stadium packs are never accepted: their materials break the Metal
    spectator shader (white stands). Files dropped in the app's Documents
    folder (Files app) are moved; the international data's own Download files
    are linked, never moved.
    """
    download = Path(root) / "Download"
    download.mkdir(parents=True, exist_ok=True)
    docs = Path(original_documents)
    for source, move in ((docs, True), (Path(root), True), (docs / "Download", False)):
        if not source.is_dir() or source.resolve() == download.resolve():
            continue
        for item in source.iterdir():
            if not item.is_file() or not _stadium_name(item.name):
                continue
            target = download / item.name
            if target.exists() or target.is_symlink():
                if move and target.is_file() and target.stat().st_size == item.stat().st_size:
                    continue
                os.replace(target, target.with_name(target.name + ".previous-" + str(time.time_ns())))
            if move:
                os.replace(item, target)
            else:
                os.symlink(item, target)
            emit("stadium_imported", file=item.name, moved=move)
    cache_path = download / _STADIUM_CACHE
    try:
        cache = json.loads(cache_path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        cache = {}
    fresh = {}
    for item in sorted(download.iterdir()):
        if not _stadium_name(item.name) or not item.is_file():
            continue
        st = item.stat()
        old = cache.get(item.name) or {}
        if old.get("size") == st.st_size and old.get("mtime_ns") == st.st_mtime_ns and len(old.get("sha256", "")) == 64:
            fresh[item.name] = old
            continue
        with item.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        fresh[item.name] = {"size": st.st_size, "mtime_ns": st.st_mtime_ns, "sha256": digest}
    temporary = cache_path.with_name(_STADIUM_CACHE + ".tmp")
    temporary.write_text(json.dumps(fresh, indent=2), encoding="utf-8")
    os.replace(temporary, cache_path)
    stems = sorted({n.split("_mobile_ios.")[0] for n in fresh})
    complete = [s for s in stems if all(s + "_mobile_ios." + e in fresh for e in ("pak", "ucas", "utoc"))]
    emit("stadiums_ready", stadiums=complete)


def _stadium_list():
    """CMD_GET_STADIUM_DATA: exactly the complete, hashed iOS stadium triples installed."""
    if _data_root is None:
        return []
    download = Path(_data_root) / "Download"
    try:
        cache = json.loads((download / _STADIUM_CACHE).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return []
    out = []
    for stem in sorted({n.split("_mobile_ios.")[0] for n in cache}):
        parts = [stem + "_mobile_ios." + e for e in ("pak", "ucas", "utoc")]
        rows = []
        for name in parts:
            record, path = cache.get(name), download / name
            if not record or not path.is_file() or path.stat().st_size != record["size"]:
                rows = []
                break
            rows.append({"name": name, "size": record["size"], "hash": record["sha256"],
                         "version": _STADIUM_VERSION, "changelist_number": 0})
        out += rows
    return out


def prepare_data(mode, root, original_documents):
    global _data_root
    _data_root = root
    from efootxi_resources import install, prepare_global, install_native_live
    def emit(kind, **value):
        event('resource-install.jsonl', event=kind, **value)
        print('EFOOTXI_DATA', kind, json.dumps(value, ensure_ascii=False), flush=True)
        try:
            import _efootxi_native
            if kind == 'native_base_restart_required':
                _efootxi_native.restart_required()
            total = value.get('total') or value.get('count') or 0
            done = value.get('downloaded') or value.get('index') or 0
            descriptions = {'download_begin': 'Đang kết nối tải data China', 'download_progress': 'Đang tải data China',
                'download_verified': 'Đã kiểm tra gói tải', 'extract_verified': 'Đang cài tài nguyên',
                'checking_base': 'Đang kiểm tra dữ liệu có sẵn', 'global_copy': 'Đang chuẩn bị data quốc tế',
                'native_base_copy': 'Đang chuẩn bị đồ họa iOS gốc', 'native_base_ready': 'Đồ họa iOS đã sẵn sàng',
                'native_base_unavailable': 'Chưa tìm thấy bộ đồ họa iOS khớp với phiên bản này',
                'native_base_restart_required': 'Đồ họa iOS đã chuẩn bị xong; mở lại game để áp dụng',
                'installed': 'Đã cài data China; đang thử khả năng tương thích iOS', 'already_installed': 'Data đã sẵn sàng'}
            detail = descriptions.get(kind, kind)
            if value.get('file'):
                detail += '\n' + value['file']
            if total and kind.startswith('download'):
                detail += '\n%.1f / %.1f MB' % (done / 1e6, total / 1e6)
            _efootxi_native.progress(detail, done / total if total else 0.0)
        except ImportError:
            pass
    try:
        if mode == 'china-data':
            result = install(_bundle, root, emit)
        else:
            result = prepare_global(original_documents, root, emit)
        install_native_live(_bundle, Path(root), emit)
        if mode == 'china-data':
            from efootxi_native_base import install as install_native_base
            install_native_base(_bundle, original_documents, root, emit)
        try:
            _stage_stadiums(root, original_documents, emit)
        except Exception as error:
            event("python.jsonl", event="stadium_stage_error", error=repr(error))
        event('resource-install.jsonl', event='ready', mode=mode, result=result)
        return True
    except BaseException:
        details = traceback.format_exc()
        event('resource-install.jsonl', event='failed', mode=mode, traceback=details)
        print(details, file=sys.stderr, flush=True)
        raise


def _resource(path):
    relative = path.removeprefix("/livedata/")
    root = (_bundle / "livedata").resolve()
    p = (root / relative).resolve()
    if not p.is_relative_to(root):
        return 403, "text/plain", b"Invalid resource path"
    if not p.is_file():
        # The release also serves generated banners from its outputs folder.
        for directory in ("livedata", "outputs/efootball", "outputs/reference"):
            candidate = (Path(_gateway._root) / directory / relative).resolve()
            if candidate.is_relative_to((Path(_gateway._root) / directory).resolve()) and candidate.is_file():
                p = candidate
                break
    if not p.is_file():
        event("livedata.jsonl", event="missing", path=path)
        return 404, "text/plain", ("Missing resource: " + relative).encode()
    data = p.read_bytes()
    event("livedata.jsonl", event="served", path=path, actual=str(p), size=len(data), sha256=hashlib.sha256(data).hexdigest())
    mime = "image/jpeg" if p.suffix.lower() in (".jpg", ".jpeg") else "application/octet-stream"
    return 200, mime, data


_VIETNAMESE_EVENTS = [
    {
        "event_id": 90002,
        "event_name": "UEFA Champions League - Cúp C1 Châu Âu",
        "event_list_disp_name": "UEFA Champions League - Cúp C1 Châu Âu",
        "major_category": "SQUAD_CHALLENGE",
        "is_vs_user": "NO",
        "is_custom_team": "YES",
        "event_start_time": 0,
        "event_end_time": 2147483647,
        "event_panel_open_time": 0,
        "event_panel_close_time": 0,
        "match_entry": [],
        "is_nonsupport": "NO",
        "description_text": "Tranh cúp vô địch các câu lạc bộ châu Âu đỉnh cao.",
        "rule_detail_text": "",
        "is_championship_pro": "NO",
        "is_official_competition": "NO",
        "esports_mode_kind": "NONE",
        "is_in_esports_menu": "NO",
        "link_title_list": [],
        "link_url_list": [],
        "achievement_name_list": ["European Club Championship"],
        "is_local_gamerelay": "NO",
        "has_ranking": "NO",
        "fame_id_list": [],
        "collabo_kind": "NONE"
    },
    {
        "event_id": 90003,
        "event_name": "FIFA World Cup - Cúp Vô Địch Thế Giới",
        "event_list_disp_name": "FIFA World Cup - Cúp Vô Địch Thế Giới",
        "major_category": "SQUAD_CHALLENGE",
        "is_vs_user": "NO",
        "is_custom_team": "YES",
        "event_start_time": 0,
        "event_end_time": 2147483647,
        "event_panel_open_time": 0,
        "event_panel_close_time": 0,
        "match_entry": [],
        "is_nonsupport": "NO",
        "description_text": "Đại hội bóng đá thế giới - Cúp vô địch các đội tuyển quốc gia.",
        "rule_detail_text": "",
        "is_championship_pro": "NO",
        "is_official_competition": "NO",
        "esports_mode_kind": "NONE",
        "is_in_esports_menu": "NO",
        "link_title_list": [],
        "link_url_list": [],
        "achievement_name_list": ["World Cup Championship"],
        "is_local_gamerelay": "NO",
        "has_ranking": "NO",
        "fame_id_list": [],
        "collabo_kind": "NONE"
    },
    {
        "event_id": 90004,
        "event_name": "Legend All-Stars Challenge - Thách Đấu Huyền Thoại",
        "event_list_disp_name": "Legend All-Stars Challenge - Thách Đấu Huyền Thoại",
        "major_category": "SQUAD_CHALLENGE",
        "is_vs_user": "NO",
        "is_custom_team": "YES",
        "event_start_time": 0,
        "event_end_time": 2147483647,
        "event_panel_open_time": 0,
        "event_panel_close_time": 0,
        "match_entry": [],
        "is_nonsupport": "NO",
        "description_text": "Thử thách đỉnh cao cùng đội hình huyền thoại bóng đá thế giới.",
        "rule_detail_text": "",
        "is_championship_pro": "NO",
        "is_official_competition": "NO",
        "esports_mode_kind": "NONE",
        "is_in_esports_menu": "NO",
        "link_title_list": [],
        "link_url_list": [],
        "achievement_name_list": ["Legend Challenge"],
        "is_local_gamerelay": "NO",
        "has_ranking": "NO",
        "fame_id_list": [],
        "collabo_kind": "NONE"
    },
    {
        "event_id": 90005,
        "event_name": "European Super Derbies - Siêu Kinh Điển El Clásico",
        "event_list_disp_name": "European Super Derbies - Siêu Kinh Điển El Clásico",
        "major_category": "SQUAD_CHALLENGE",
        "is_vs_user": "NO",
        "is_custom_team": "YES",
        "event_start_time": 0,
        "event_end_time": 2147483647,
        "event_panel_open_time": 0,
        "event_panel_close_time": 0,
        "match_entry": [],
        "is_nonsupport": "NO",
        "description_text": "Các trận cầu siêu kinh điển rực lửa của bóng đá châu Âu.",
        "rule_detail_text": "",
        "is_championship_pro": "NO",
        "is_official_competition": "NO",
        "esports_mode_kind": "NONE",
        "is_in_esports_menu": "NO",
        "link_title_list": [],
        "link_url_list": [],
        "achievement_name_list": ["Super Derbies"],
        "is_local_gamerelay": "NO",
        "has_ranking": "NO",
        "fame_id_list": [],
        "collabo_kind": "NONE"
    },
    {
        "event_id": 100004,
        "event_name": "My League Sprint - Đua Tốc Độ & Thử Thách",
        "event_list_disp_name": "My League Sprint - Đua Tốc Độ & Thử Thách",
        "major_category": "SQUAD_CHALLENGE",
        "is_vs_user": "NO",
        "is_custom_team": "YES",
        "event_start_time": 0,
        "event_end_time": 2147483647,
        "event_panel_open_time": 0,
        "event_panel_close_time": 0,
        "match_entry": [],
        "is_nonsupport": "NO",
        "description_text": "Sự kiện đồng hành cùng mùa giải My League.",
        "rule_detail_text": "",
        "is_championship_pro": "NO",
        "is_official_competition": "NO",
        "esports_mode_kind": "NONE",
        "is_in_esports_menu": "NO",
        "link_title_list": [],
        "link_url_list": [],
        "achievement_name_list": ["My League Sprint"],
        "is_local_gamerelay": "NO",
        "has_ranking": "NO",
        "fame_id_list": [],
        "collabo_kind": "NONE"
    }
]


def _second_style(position, first):
    """Secondary (defensive) playing style when the card data leaves it 0.

    Same position rule as the original PES2027 iOS server (z27GetDefensiveStyle),
    which matches the captured official account (e.g. Messi RWF [14, 23]).
    """
    if position == 0:
        return 33 if first in (16, 17) else 16
    if position == 1 or first in (16, 17):
        return 33
    by_position = {2: 30, 3: 28, 4: 28, 5: 25, 6: 25, 7: 27, 8: 27, 9: 23, 10: 23, 11: 23, 12: 26, 13: 26}
    if position in by_position:
        return by_position[position]
    if first in (1, 2, 3, 13, 22):
        return 26
    if first in (4, 6, 10, 14, 18, 19):
        return 23
    if first in (7, 8, 9, 20):
        return 25
    if first in (11, 12, 15, 21):
        return 28
    return 27


def _fill_second_styles(value):
    """Display-only: fill play_style[1] where it is 0; parameters stay untouched."""
    if isinstance(value, dict):
        style = value.get("play_style")
        if (isinstance(style, list) and len(style) == 2 and type(style[0]) is int and style[0] > 0
                and style[1] == 0):
            params = value.get("parameters")
            position = params[0] if isinstance(params, list) and params and type(params[0]) is int else -1
            value["play_style"] = [style[0], _second_style(position, style[0])]
        for item in value.values():
            if isinstance(item, (dict, list)):
                _fill_second_styles(item)
    elif isinstance(value, list):
        for item in value:
            if isinstance(item, (dict, list)):
                _fill_second_styles(item)
    return value


def _adapt_ios_response(request, resp):
    if not isinstance(resp, dict):
        return resp
    _fill_second_styles(resp)
    mid = resp.get("msgid") or (request.get("msgid") if isinstance(request, dict) else None)

    if mid in ("CMD_GET_MAINMENU_INFO", "CMD_GET_MYCLUB_MAINMENU_INFO"):
        banners = resp.get("banner_list")
        if isinstance(banners, list) and banners:
            for idx, b in enumerate(banners):
                if not isinstance(b, dict):
                    continue
                name = urlsplit(b.get("banner_file_name", "")).path.rsplit("/", 1)[-1]
                if "players" in name or idx == 0:
                    b.update(announcement_type="AGENT", entity_id=91000,
                             title_text="Cầu thủ đặc biệt", description_text="Các pack và cầu thủ mới nhất",
                             note_text="efootxi · https://t.me/efootxi", is_popup="NO")
                else:
                    b.update(announcement_type="AGENT", entity_id=91001,
                             title_text="Huấn luyện viên & Siêu sao", description_text="Các huấn luyện viên và thẻ Big Time mới nhất",
                             note_text="efootxi · https://t.me/efootxi", is_popup="NO")
        try:
            import club_store as store
            import managed_card_pool
            import sqlite3
            root = getattr(_gateway, '_root', None)
            if root:
                db_path = Path(root) / "outputs/efootball/local-club.sqlite3"
                if db_path.is_file():
                    with sqlite3.connect(db_path.as_uri() + "?mode=ro", uri=True) as conn:
                        agents = store.agents(conn)
                        by_id = {a.get("agent_id"): a for a in agents}
                        banner_aids = [91000, 91001]
                        banner_agents = [by_id[aid] for aid in banner_aids if aid in by_id]
                        if banner_agents:
                            decorated = managed_card_pool.decorate_agents(conn, banner_agents)
                            resp["agent_list"] = _manifest(decorated)
                        prices_str = store.get_meta(conn, "agent_price_list", "[]")
                        if prices_str:
                            all_prices = json.loads(prices_str)
                            resp["agent_price_list"] = [p for p in all_prices if p.get("agent_id") in banner_aids]
                        read_str = store.get_meta(conn, "efootxi_news_read_v1", "[]")
                        seen = set(json.loads(read_str)) if read_str else set()
                        if 140001 not in seen:
                            resp["is_news_popup"] = "YES"
                            try:
                                with sqlite3.connect(db_path) as wconn:
                                    seen.add(140001)
                                    store.set_meta(wconn, "efootxi_news_read_v1", json.dumps(sorted(seen)))
                                    wconn.commit()
                            except Exception:
                                pass
                        else:
                            resp["is_news_popup"] = "NO"
        except Exception as e:
            event("python.jsonl", event="banner_adapt_error", error=str(e))

    elif mid == "CMD_GET_MAILBOX_INFO":
        try:
            news_list = resp.get("news_list")
            if isinstance(news_list, list):
                if not any(n.get("news_id") == 140001 for n in news_list if isinstance(n, dict)):
                    import club_store as store
                    import sqlite3
                    seen = set()
                    root = getattr(_gateway, '_root', None)
                    if root:
                        db_path = Path(root) / "outputs/efootball/local-club.sqlite3"
                        if db_path.is_file():
                            with sqlite3.connect(db_path.as_uri() + "?mode=ro", uri=True) as conn:
                                read_str = store.get_meta(conn, "efootxi_news_read_v1", "[]")
                                seen = set(json.loads(read_str)) if read_str else set()
                    notice_140001 = {
                        "news_id": 140001,
                        "title": "Thông báo bản quyền & Chuyển đổi iOS",
                        "date": 1792634400,
                        "body": "Bản quyền thuộc về Efootxi và iOSVN chỉ là người biên dịch nó sang iOS theo yêu cầu của member.",
                        "readFlag": "YES" if 140001 in seen else "NO",
                        "type": "NEWS_TYPE_GLOBAL",
                        "priority": 0,
                        "frequency_type": "ONCE_POPUP_ONLY",
                        "category": "NEWS_CATEGORY_IMPORTANT",
                        "banner_name": "",
                        "link_list": [],
                        "surveyFlag": "NO"
                    }
                    news_list.insert(0, notice_140001)
                    resp["result"] = "NOERR"
        except Exception as e:
            event("python.jsonl", event="mailbox_adapt_error", error=str(e))

    elif mid == "CMD_SET_NEWSLIST":
        try:
            supplied = request.get("news_list") if isinstance(request, dict) else None
            if isinstance(supplied, list) and 140001 in supplied:
                import club_store as store
                import sqlite3
                root = getattr(_gateway, '_root', None)
                if root:
                    db_path = Path(root) / "outputs/efootball/local-club.sqlite3"
                    if db_path.is_file():
                        with sqlite3.connect(db_path) as conn:
                            read_str = store.get_meta(conn, "efootxi_news_read_v1", "[]")
                            seen = set(json.loads(read_str)) if read_str else set()
                            seen.add(140001)
                            store.set_meta(conn, "efootxi_news_read_v1", json.dumps(sorted(seen)))
                            conn.commit()
            resp["result"] = "NOERR"
        except Exception as e:
            event("python.jsonl", event="news_ack_error", error=str(e))

    elif mid == "CMD_GET_STADIUM_DATA":
        resp["data_list"] = _stadium_list()
        resp["result"] = "NOERR"

    elif mid == "CMD_GET_MAINMENU_MATCH_CREATIVE_INFO":
        try:
            creative_template_path = Path(__file__).with_name("old_ipa_creative_info.json")
            if creative_template_path.is_file():
                import copy
                c_data = json.loads(creative_template_path.read_text(encoding="utf-8"))
                for k in ("event_info_list", "league_division_info", "league_info", "analyst_main_info",
                          "ml_event_info", "sheet_list", "empty_slot_info_list", "season_root_info"):
                    if k in c_data:
                        resp[k] = copy.deepcopy(c_data[k])
                resp["result"] = "NOERR"
            else:
                far_future = 2147483647
                if isinstance(resp.get("event_info_list"), list):
                    for item in resp["event_info_list"]:
                        if isinstance(item, dict):
                            item["event_end_time"] = far_future
                            if "event_start_time" in item:
                                item["event_start_time"] = 0
                if isinstance(resp.get("ml_event_info"), dict):
                    resp["ml_event_info"]["event_end_time"] = far_future
                    resp["ml_event_info"]["event_panel_close_time"] = far_future
                if isinstance(resp.get("league_info"), dict):
                    resp["league_info"]["league_phase_end_time"] = far_future
        except Exception as e:
            event("python.jsonl", event="creative_adapt_error", error=str(e))

    elif mid in ("CMD_GET_EVENT_LIST_AND_LOG", "CMD_GET_EVENT_LIST_AND_INFO"):
        try:
            event_template_path = Path(__file__).with_name("old_ipa_event_list.json")
            if event_template_path.is_file():
                import copy
                t_data = json.loads(event_template_path.read_text(encoding="utf-8"))
                resp["event_list"] = copy.deepcopy(t_data["event_list"])
                resp["event_log_list"] = copy.deepcopy(t_data["event_log_list"])
                resp["event_num"] = len(resp["event_list"])
                for k in ("sp_rate_list", "cpu_level_rate_list", "result_rate_list", "default_member",
                          "event_recommend_kind", "event_recommend_str", "event_recommend_info_type"):
                    if k in t_data:
                        resp[k] = copy.deepcopy(t_data[k])
                resp["result"] = "NOERR"
            else:
                far_future = 2147483647
                event_list = resp.get("event_list")
                if isinstance(event_list, list) and event_list:
                    for ev in event_list:
                        if isinstance(ev, dict):
                            ev["event_start_time"] = 0
                            ev["event_end_time"] = far_future
                            eid = ev.get("event_id")
                            matched = next((ve for ve in _VIETNAMESE_EVENTS if ve["event_id"] == eid), None)
                            if matched:
                                ev["event_name"] = matched["event_name"]
                                ev["event_list_disp_name"] = matched["event_list_disp_name"]
                                ev["description_text"] = matched["description_text"]
                else:
                    import copy
                    resp["event_list"] = copy.deepcopy(_VIETNAMESE_EVENTS)
                    resp["event_num"] = len(_VIETNAMESE_EVENTS)
                resp["result"] = "NOERR"
        except Exception as e:
            event("python.jsonl", event="event_adapt_error", error=str(e))

    return resp


def serve(method, path, body):
    global _serial
    with _lock:
        _serial += 1
        prefix = _session / "requests" / ("%06d" % _serial)
        prefix.with_suffix(".request.bin").write_bytes(body)
        start = time.monotonic()
        event("network.jsonl", event="begin", id=_serial, method=method, path=path, bytes=len(body))
        try:
            resource = _resource_path(path)
            if resource is not None:
                status, mime, response = _resource(resource)
            elif method.upper() in ("GET", "HEAD") and "GateInfo" not in path:
                status, mime, response = 404, "text/plain", b"Unknown local resource"
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
                    response_dict = _adapt_ios_response(request, response_dict)
                    response = pes_msg.encode_response(response_dict)
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
