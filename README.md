# pes2027-mod — eFootball Ziru Offline trên IPA iOS 11.1.1

Mod trực tiếp IPA eFootball iOS **11.1.1 (build 11.1.101)** để chạy offline bằng logic server của bản Android
“Efootball 2027 Offline Full Epic – Bigtime by Ziru”. Repo này **chỉ chứa mã nguồn**: không có IPA, APK,
pak/cpk, responses hay livedata. Mọi dữ liệu game lấy từ máy của bạn khi ghép.

> Trạng thái: build và test trên PC. **Chưa thử trên iPhone.** Chỉ ghi “chơi được” sau khi đã chạy trên thiết bị.

## Cách hoạt động

| Thành phần | Nội dung |
| --- | --- |
| `core/` | Bản port C++ của `com.offline.server.OfflineServer` (Ziru, `classes5.dex`): 180 phương thức, SQLite `ziru_save.db`, SharedPreferences `almanac_state`, codec `AES-256-CBC + zlib/gzip + msgpack`. |
| `ios/ZiruOffline.mm` | Dylib nạp vào `PESmobile`: chuyển URL của `HttpImplAsyncRequest/HttpImplRequest/HttpImpl2Request` sang `https://127.0.0.1:8085` (giống bản vá `HttpImpl.SendRequest` của Ziru trên Android) và trả lời qua `NSURLProtocol` ngay trong tiến trình. Có màn hình nhập gói data trước khi UE4 khởi động. |
| `tools/build_ipa.py` | Ghép `PES2027.ipa` + `PES2027-data-<data_id>.zip` từ IPA đã decrypt, APK Ziru và dylib CI. |
| `test/` | Test đối chiếu C++ với **bytecode gốc** của Ziru chạy trên JVM (oracle) và test importer. |

### Giao thức
Client iOS 11.1.1 giải khóa AES trong `onlinesystem::ServerDef::GetAESKey()` (`BECStringConcreteData<32>` tại
VA `0x106a62340`, `key[i] = d[i] ^ A[i%9] ^ B[i%12] ^ 0x75`) — kết quả trùng `KEY_HEX` của server Ziru.
Request: `IV ‖ AES(zlib(msgpack))`. Response: `IV ‖ AES(gzip(msgpack))`.

### Tách data khỏi IPA
`basic::PathGet()` dựng `<bundle>/cpk/` (type 6) và `<bundle>/pak/` (type 7). `build_ipa.py` vá 4 lệnh để hai type này
dùng `GetDocumentsPath()`; dylib tạo symlink `Documents/pak`, `Documents/cpk` →
`Library/Application Support/PES2027/GameData/<data_id>/{pak,cpk}`. Save nằm riêng ở
`Library/Application Support/PES2027/Save/` và không bị đụng khi cập nhật data/IPA.

| Vị trí patch | Cũ | Mới |
| --- | --- | --- |
| `0x1000155BC` / `0x1000155C0` (cpk) | `str x20` / `mov x0,x20` | `str x21` / `mov x0,x21` |
| `0x10001561C` / `0x100015620` (pak) | `str x20` / `mov x0,x20` | `str x21` / `mov x0,x21` |
| Load commands | — | `LC_LOAD_DYLIB @executable_path/Frameworks/ZiruOffline.dylib` |

Script dừng nếu SHA-256 của `PESmobile` khác `0cf9bacd…a0de` hoặc byte tại vị trí patch không như mong đợi.

## Ghép IPA trên Windows

1. Tải `ZiruOffline.dylib` từ tab **Actions** (artifact `ZiruOffline-dylib`) hoặc **Releases**.
2. Chạy (Python 3.9+ có sẵn trong máy):

```powershell
python tools\build_ipa.py --app D:\Telegram\Payload\PESmobile.app --dylib ZiruOffline.dylib `
  --apk "D:\Efootball 2027 Offline Full Epic - Bigtime by Ziru\Efootball_Offline_VH.apk" --out D:\PES2027\out
```

3. Ký `PES2027.ipa` bằng công cụ quen dùng (Sideloadly, ESign…), cài lên máy.
4. Chép `PES2027-data-<data_id>.zip` vào app Tệp (Trên iPhone › PES 2027) hoặc để ở chỗ bất kỳ trong Tệp.
5. Mở app → màn hình **Nhập dữ liệu game** → chọn gói → app giải nén, kiểm tra SHA-256 từng file → vào game.

Log để gửi khi lỗi: app Tệp › PES 2027 › `PES2027-logs/ziru.log` và `offline_server.log`.

## Test trên PC

```bash
make build/runcpp build/packtest
# oracle: dex2jar classes5.dex -> classes5.jar, rồi test/oracle/Patch.java cố định Random/đồng hồ
python3 test/difftest.py          # cần assets/responses của APK (biến môi trường ZASSETS)
```

Kết quả hiện tại: 7 kịch bản / 183 request (login, chiêu mộ, huấn luyện, careerplan, đội hình,
Division, MyLeague) khớp từng giá trị (kể cả kiểu int/float và thứ tự key) với bytecode Ziru.

## Việc còn mở
- Chưa chạy trên iPhone: chưa xác nhận UE4 mount pak từ symlink, chưa biết luồng offline có gọi gRPC không.
- Livedata của Ziru là bản cook Android (`_mobile_and`); chưa biết client iOS có dùng được không
  (`--no-livedata` để bỏ ra khỏi gói data).
- Chưa đối chiếu các CPK `_mobile_all` mà Ziru sửa (ví dụ `dt500`) với bản iOS.
