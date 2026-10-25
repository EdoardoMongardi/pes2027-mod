# pes2027-mod — eFootball iOS 11.1.1 (Offline & Online Mod)

Dự án mod và port logic máy chủ offline cho eFootball iOS phiên bản **11.1.1 (build 11.1.101)** dựa trên kiến trúc bản Android *(Efootball 2027 Offline Full Epic – Bigtime by Ziru)*, bổ sung tính năng thi đấu Online Friend Match (1v1) qua máy chủ Relay độc lập.

Kho lưu trữ này **chỉ chứa mã nguồn** (C++ core, Objective-C++ dylib, công cụ nhị phân Mach-O, server signaling Python). **Không chứa tài nguyên có bản quyền** (IPA gốc của Konami, APK Android, mô hình 3D, khuôn mặt hay livedata). Toàn bộ dữ liệu được tổng hợp và đóng gói cục bộ thông qua script đi kèm.

---

## Tính năng nổi bật

- **Offline Server In-Process (C++17)**: Tái tạo toàn bộ logic của `com.offline.server.OfflineServer` (Ziru) bằng mã C++ hiệu năng cao, chạy trực tiếp trong tiến trình game với SQLite3 (`ziru_save.db`) và SharedPreferences giả lập (`almanac_state`).
- **Tách dữ liệu game (Data Decoupling)**: Vá hàm `basic::PathGet()` trong binary Mach-O để chuyển hướng đọc `pak/` và `cpk/` ra thư mục `Documents`. File cài đặt `PES2027.ipa` chỉ nặng ~180 MB, dữ liệu lớn (~4.6 GB) được nhập độc lập qua app Tệp (Files), không làm phình dung lượng app hay mất save khi nâng cấp.
- **Hiển thị tên cầu thủ trên lưng áo (Jersey Names)**: Vá trực tiếp chuỗi hàm render (`PlayerModelConfig::SetHasUniformName`, `SetupUniformConfig`, `APlayerActorMobile::SetUniformName`, `APlayerActor::SetUniformName`), ép Unreal Engine hiển thị đầy đủ tên cầu thủ phía trên số áo trong cả trận đấu và các màn ăn mừng.
- **Mở khóa toàn bộ sân vận động**: Cho phép tự do lựa chọn các sân vận động bản quyền (Santiago Bernabéu, Spotify Camp Nou, San Siro, Emirates, Stamford Bridge, v.v.) trong mọi chế độ chơi (Giao hữu, Tour Event, Authentic Match).
- **Việt hóa giao diện & Bình luận**: Nạp trực tiếp bộ font OpenType (`eFootballSans`, `eFootballStencil`) vào hệ thống Slate UI của Unreal Engine, hiển thị tiếng Việt có dấu chuẩn xác; hỗ trợ tích hợp gói bình luận tiếng Việt (dt510 CPK).
- **Hệ thống thi đấu AI & myClub đầy đủ**:
  - Thi đấu Rank Division từ 10 lên 1 với cơ chế thăng hạng ngay khi đủ mốc điểm.
  - Tự động đồng bộ điểm kỹ năng (Progression Points), bảo toàn trọn vẹn kỹ năng đặc biệt (Showtime, Fortress, Momentum Dribbling...) và kỹ năng gốc qua các lần khởi động lại.
  - Tinh chỉnh chuẩn xác chỉ số, độ thành thạo vị trí và khuôn mặt thật của các cầu thủ cập nhật (Çalhanoğlu, Mkhitaryan, Cristiano Ronaldo, v.v.).
- **Chế độ Online Friend Match (Mod v1.1)**: Tích hợp máy chủ Signaling API (HTTPS) và STUN/TURN Coturn relay, cho phép 2 thiết bị iOS tạo phòng bằng mã 6 số và thi đấu giao hữu qua mạng Internet với độ trễ thấp (P2P NAT Punching / UDP Relay).

---

## Cấu trúc mã nguồn

| Thư mục / Tệp | Vai trò |
| --- | --- |
| `core/` | Nhân server C++: 180+ endpoint xử lý myClub, Division, MyLeague, chiêu mộ, huấn luyện, giao dịch SQLite và bộ mã hóa giải mã `AES-256-CBC + zlib/gzip + msgpack`. |
| `ios/` | Mã nguồn `ZiruOffline.dylib`: Đánh chặn mạng qua `NSURLProtocol` (chuyển hướng request nội bộ sang loopback `127.0.0.1:8085`), nạp font chữ tiếng Việt và giao diện nhập gói data trước khi Unreal Engine 4 khởi động. |
| `tools/build_ipa.py` | Công cụ tự động hóa: vá binary Mach-O (ARM64), tiêm `LC_LOAD_DYLIB`, tích hợp livedata, font chữ và đóng gói file `PES2027.ipa` hoàn chỉnh. |
| `online/` | Mã nguồn máy chủ giao hữu: Signaling API (`pes_online_server.py`), kịch bản triển khai tự động lên VPS (`setup_ec2.sh`) và bộ kiểm thử giao thức mã hóa hai client. |
| `ziru-extra/` | Các phản hồi JSON bổ sung cho client iOS 11.1.1 (xử lý EULA, sự kiện, gói sân vận động). |
| `test/` | Bộ kiểm thử hồi quy: đối chiếu dữ liệu C++ với JVM bytecode gốc của Ziru, kiểm tra bảng dữ liệu native iOS và tính toàn vẹn gói data. |

---

## Chi tiết kỹ thuật & Binary Patches

### Cơ chế giao thức mã hóa
Client eFootball iOS 11.1.1 giải khóa AES trong hàm `onlinesystem::ServerDef::GetAESKey()` (tại địa chỉ ảo `0x106A62340`):
$$\text{key}[i] = d[i] \oplus A[i \pmod 9] \oplus B[i \pmod{12}] \oplus \text{0x75}$$
Khóa giải mã hoàn toàn trùng khớp với `KEY_HEX` của máy chủ offline Ziru:
- **Request từ game:** `IV (16 bytes) ‖ AES-256-CBC(zlib(msgpack))`
- **Response trả về:** `IV (16 bytes) ‖ AES-256-CBC(gzip(msgpack))`

### Bảng các điểm vá nhị phân (Mach-O ARM64)
Toàn bộ các bản vá được thực thi tự động bởi [`tools/build_ipa.py`](tools/build_ipa.py) trên file thực thi `PESmobile`:

| Địa chỉ ảo (VA) | Lệnh gốc (Disassembly) | Lệnh sau vá (Patched) | Mục đích |
| --- | --- | --- | --- |
| `0x1000155BC` | `str x20, [sp, #0x18]` | `str x21, [sp, #0x18]` | Chuyển hướng đường dẫn CPK sang Documents |
| `0x10001561C` | `str x20, [sp, #0x18]` | `str x21, [sp, #0x18]` | Chuyển hướng đường dẫn PAK sang Documents |
| `0x1046FE0B0` | `cbz x0, 0x1046fe0b4` | `b 0x1046fe0b4` | Tránh crash `AddStadiumDlTask::Bind` khi kích thước rỗng |
| `0x1008F2FA8` | `cmp w1, #0` | `mov w8, #2` | Ép `PlayerModelConfig::SetHasUniformName` luôn bật |
| `0x10162331C` | `cset w1, ne` | `mov w1, #1` | Kích hoạt `SetupUniformConfig` mang tên lưng áo |
| `0x10089F4A4` | `ldr w8, [x19, #0xe38]` | `mov w8, #2` | Kích hoạt render font tên áo trong `BackNumberStep` |
| `0x100895F44` | `ldrb w8, [x19, #0xf10]` | `mov w8, #1` | Bỏ qua cờ ẩn tên áo trong `APlayerActorMobile` |
| `0x100896064` | `b.eq 0x100895f54` | `b 0x100895f54` | Ép vẽ tên cầu thủ ngay từ frame khởi tạo đầu |
| `0x100911418` | `ldrb w10, [x19, #0xf10]`| `mov w10, #1` | Kích hoạt cờ tên áo trong `APlayerActor` |
| Mach-O Header | — | `LC_LOAD_DYLIB` | Nạp `Frameworks/ZiruOffline.dylib` khi game mở |

---

## Hướng dẫn đóng gói IPA (Build Instructions)

### 1. Chuẩn bị nguyên liệu
1. **File ứng dụng iOS gốc đã giải mã (Decrypted IPA / `PESmobile.app`)**: Phiên bản eFootball 11.1.1 (build 11.1.101), trích xuất bằng `frida-ios-dump`, `IAPDump` hoặc tải từ các nguồn decrypt uy tín.
2. **File APK Android của Ziru**: Dùng để trích xuất kho dữ liệu `assets/` (database cầu thủ, sự kiện, mô hình đội bóng).
3. **Thư viện `ZiruOffline.dylib`**: Biên dịch trực tiếp qua macOS hoặc tải file nhị phân mới nhất từ tab **Actions** / **Releases** của repository.
4. **Python 3.9+** trên máy tính (Windows / macOS / Linux).

### 2. Thực hiện đóng gói
Chạy lệnh đóng gói thông qua công cụ `tools/build_ipa.py`:

```bash
# Ví dụ trên Windows PowerShell:
python tools/build_ipa.py `
  --app "path/to/Payload/PESmobile.app" `
  --dylib "path/to/ZiruOffline.dylib" `
  --apk "path/to/Ziru_Offline.apk" `
  --extra-pak "path/to/pak_extra" `
  --native-livedata "path/to/native-livedata" `
  --out "D:/PES2027" `
  --skip-data
```

*Các tùy chọn hữu ích:*
- `--skip-data`: Chỉ đóng gói file `PES2027.ipa` (dành cho các bản cập nhật mã nguồn/dylib mà không cần nén lại 4.6 GB data).
- `--commentary "path/to/dt510.cpk"`: Tự động đưa gói bình luận tiếng Việt vào game.
- `--online-config "path/to/client.json"`: Nhúng cấu hình máy chủ thi đấu Online Friend Match vào app.

---

## Hướng dẫn cài đặt lên thiết bị iOS

1. **Cài đặt file IPA**:
   - **TrollStore (Khuyên dùng)**: Mở trực tiếp file `PES2027.ipa` và chọn Install. App chạy với đầy đủ quyền hạn, không bị thu hồi chứng chỉ và không giới hạn 7 ngày.
   - **ESign / Feather / Sideloadly / AltStore**: Ký file IPA bằng chứng chỉ cá nhân hoặc chứng chỉ doanh nghiệp và cài đặt lên máy.
2. **Nhập gói dữ liệu (Data Pack)**:
   - Chép file dữ liệu `PES2027-data-<data_id>.zip` vào ứng dụng **Tệp (Files)** trên iPhone (thư mục *Trên iPhone › PES 2027* hoặc vị trí bất kỳ).
   - Mở game lần đầu → Màn hình **Nhập dữ liệu game** của mod xuất hiện → Nhấn chọn gói ZIP vừa chép.
   - Gói ZIP có thể có `data-manifest.json` ngay ở gốc, hoặc nằm trong đúng một thư mục (ví dụ nén cả thư mục `PES2027-data` thành `PES2027-data.zip`).
   - Ứng dụng sẽ tự động giải nén, kiểm tra tính toàn vẹn (SHA-256) của từng file và khởi tạo môi trường offline.
   - Cách khác: dùng 3uTools hoặc ứng dụng Tệp chép cả thư mục `PES2027-data` (gồm `pak`, `cpk`, `livedata`) vào thư mục của app. Mod tự trỏ `Documents/pak`, `cpk`, `livedata` tới thư mục này, kể cả khi app đã từng được mở trước khi có dữ liệu.
   - Nếu dữ liệu được chép hoặc nhập trong lúc màn hình nhập dữ liệu đang mở, hãy tắt hẳn ứng dụng rồi mở lại: game chỉ đọc dữ liệu khi khởi động.
3. **Xem file nhật ký khi gặp sự cố**:
   - Toàn bộ log vận hành được ghi tại: *Tệp › Trên iPhone › PES 2027 › `PES2027-logs/ziru.log`* và `offline_server.log`.
4. **Save cũ (bản build 138) khi cập nhật lên bản này**:
   - Mỗi cầu thủ khởi đầu giờ có số serial riêng như dữ liệu gốc của Konami (trước đây 70 thẻ dùng chung 11 serial), nên huấn luyện kỹ năng, booster, vị trí và điểm phát triển chỉ áp dụng cho đúng thẻ được chọn.
   - Lần mở đầu tiên, save cũ được chuyển một lần: thẻ đang giữ tiến trình ở mỗi serial cũ được chuyển sang serial mới của chính nó cùng kỹ năng đã học, booster, vị trí, cấp độ và điểm phát triển; các thẻ khởi đầu khác bắt đầu với chỉ số gốc; đội hình và đội hình MyLeague trỏ sang serial mới; thẻ từ gói không bị động tới.
   - Bản sao dữ liệu cũ được giữ trong save (`meta` khóa `z27_starter_serials_backup`); mỗi bước được ghi vào `offline_server.log` với nhãn `Z27_SERIAL_MIGRATION`.
   - Yêu cầu huấn luyện cho một thẻ không nằm ở serial đó sẽ được trả lời là không thành công và không thay đổi gì (`Z27_TRAINING_REFUSED` trong log).
5. **Những thay đổi trong bản fix3**:
   - **Vé huấn luyện vị trí (chọn)**: thẻ học đúng vị trí đã chọn. Mỗi ô vị trí đã học nâng độ thành thạo ở vị trí đó thêm một bậc so với mức vốn có của thẻ (tối đa 2), như game gốc của Konami. Khi đăng nhập, các vị trí đã học trong save cũ được chỉnh lại theo quy tắc này, nên một số vị trí có thể giảm từ mức 2 xuống mức 1, đúng như game gốc.
   - **Vé huấn luyện kỹ năng (chọn)**: mod gửi cho game danh sách kỹ năng thẻ có thể học (40 kỹ năng thường; thủ môn có thêm 4 kỹ năng thủ môn), nên các nhóm kỹ năng không còn bị khóa với nhãn "(learned)" và thẻ học đúng kỹ năng đã chọn.
   - Với cả hai vé, lựa chọn mà game không cho chọn (ví dụ một kỹ năng thẻ đã có hoặc đã học) được trả lời là không thành công: thẻ giữ nguyên và không mất vé (`ZIRU_SKILL_SELECT_REFUSED` / `ZIRU_POSITION_REFUSED` trong `offline_server.log`; mỗi lần dùng vé được ghi với nhãn `ZIRU_ITEM`).
   - **Xóa kỹ năng đã học**: kỹ năng bị xóa trong màn hình huấn luyện kỹ năng giờ bị xóa thật. Trước đây nó quay lại ở lần đăng nhập sau, và lần huấn luyện tiếp theo có thể ghi đè nhầm một kỹ năng khác. Kỹ năng đã xóa có thể học lại bằng vé chọn (`ZIRU_SKILL_DELETE` trong `offline_server.log`).
   - **Kỹ năng thủ môn**: game gốc chỉ cho thủ môn học GK Low Punt, GK High Punt, GK Long Throw và GK Penalty Saver, nhưng huấn luyện kỹ năng ngẫu nhiên ở các bản trước đã dạy cả cho cầu thủ không phải thủ môn. Khi đăng nhập, các kỹ năng này được gỡ khỏi những thẻ đó, các kỹ năng còn lại được dồn lên (`Z27_GK_SKILL_REMOVED` trong `offline_server.log`).
   - **Booster**: sau mỗi lần dùng vé booster, kể cả khi không thành công, thẻ vẫn giữ đúng giá bán (trước đây có lúc hiện 0).
   - **Thẻ đội tuyển quốc gia** hiện cờ quốc gia trên mặt thẻ, còn thẻ câu lạc bộ có kỹ năng Low Screamer không còn hiện nhầm cờ. Phần chi tiết thẻ ghi câu lạc bộ của cầu thủ ở giai đoạn mà thẻ thể hiện (ví dụ thẻ "Italy 2000" ghi câu lạc bộ của cầu thủ năm 2000) (theo dữ liệu chính thức khi có, nếu không thì là ước tính) hoặc không ghi câu lạc bộ, thay vì tên quốc gia.
   - **Xem trước gói thẻ**: các kỹ năng nổi bật là đúng kỹ năng của từng thẻ (trước đây bị lệch và ô trống hiện "Fighting Spirit"); kỹ năng đặc biệt ở màn hình thay người trong trận cũng đúng.
   - Cờ quốc gia, câu lạc bộ của thẻ đội tuyển quốc gia và kỹ năng nổi bật (cùng kỹ năng ở màn hình thay người) đến từ các file dữ liệu đi kèm IPA fix3 (file `dt870` mới và dữ liệu câu lạc bộ của thẻ), không phải từ mã nguồn. Một bản tự build chỉ từ mã nguồn trong kho này vẫn dùng file dữ liệu cũ nên chưa có ba sửa đổi đó.
   - Khi khởi động, mod tự thay file `dt870` cũ trong thư mục dữ liệu (bản gốc của Konami hoặc bản mà các phiên bản mod trước đã cài) bằng bản mới đi kèm; file khác do người chơi tự đặt vào được giữ nguyên, kể cả file quá lớn hoặc không đọc được (`IOS_TABLE_FIX` trong `ziru.log`). Thư mục `PES2027-repair-backups` không còn nhận thêm file sau mỗi lần mở app.
   - Mod chép file vào thư mục `Download` dưới một tên tạm và chỉ đổi sang tên thật khi đã chép xong, nên tắt app giữa chừng không còn để lại file chép dở. Nếu trong `Download` có một bản `dt870` đi kèm bị chép dở (chỉ có phần đầu của file), mod chuyển nó vào `PES2027-repair-backups` và chép lại bản đầy đủ.
6. **Quay lại bản fix2**: trước khi cài lại IPA fix2, mở app **Tệp (Files)**, vào *Trên iPhone › PES 2027 › Download* và xóa hai file `dt870_mobile_and.cpk` và `dt870_mobile_ios.cpk`. Nếu không xóa, fix2 coi file `dt870` mới là file riêng của người chơi và giữ nguyên nó; khi đã xóa, lần mở đầu tiên của fix2 tự chép lại file của chính nó.
7. **Bản fix4: các gói thẻ mới**:
   - Các gói thẻ cũ được thay bằng 29 gói, xếp theo loại thẻ và vị trí đăng ký của thẻ:
     - **ZIRU BIG TIME** và **ZIRU LEGENDARY**: mọi thẻ Big Time và mọi thẻ Legendary có hình trên iOS. Thẻ không có hình, thẻ trùng (bản Limited/Selection của một thẻ khác) và thẻ giữ chỗ của Konami không có trong gói nào; riêng một thẻ Big Time chỉ có hình dành cho Android nằm trong **ZIRU ART TEST**.
     - **ZIRU EPIC** và **ZIRU SHOW TIME**: mỗi loại 12 gói, mỗi gói một vị trí: GK, CB, LB, RB, DMF, CMF, LMF, RMF, AMF / SS (thẻ SS nằm chung gói với AMF), LWF, RWF, CF.
     - **ZIRU HIGHLIGHT** (các thẻ Highlight) và **ZIRU OTHERS** (các thẻ khởi đầu không thuộc gói nào ở trên).
     - **ZIRU ART TEST**: các thẻ có hình chỉ nằm trong IPA hoặc chỉ có bản dành cho Android, nên hình có thể không hiện trên iPhone/iPad. Gói này dùng để thử xem hình của các thẻ đó có hiện hay không; các thẻ này không có trong gói nào khác.
   - Có tất cả 1.987 thẻ. Mỗi thẻ chỉ nằm trong một gói, và mọi thẻ của các gói cũ đều có trong các gói mới. Gói nhỏ nhất có 15 thẻ, nên gói nào cũng mở được 10 thẻ một lần khi còn đủ 10 thẻ.
   - Một gói chỉ đưa ra những thẻ mà câu lạc bộ chưa có. Sau khi bấm **Reset**, gói có lại đủ thẻ, kể cả những thẻ đã có.
   - Các gói mới và dữ liệu cầu thủ của các thẻ trong gói đến từ các file dữ liệu đi kèm IPA fix4, không phải từ mã nguồn. Một bản tự build chỉ từ mã nguồn trong kho này vẫn dùng các gói cũ.
8. **Bản fix4: save mới bắt đầu không có cầu thủ**:
   - Chỉ một save hoàn toàn mới bắt đầu với câu lạc bộ trống, không có 70 thẻ khởi đầu. Save đang dùng vẫn giữ 70 thẻ khởi đầu (mục 9).
   - Cách có save mới: xóa app (save hiện tại và dữ liệu đã nhập cũng bị xóa theo), cài lại IPA fix4, rồi nhập lại gói dữ liệu hoặc chép lại thư mục `PES2027-data` như ở mục 2.
   - Mở thẻ từ các gói, rồi vào **Game Plan** để xếp đội hình: kéo một thẻ từ danh sách **RES** (dự bị) thả vào ô trống, hoặc bấm **Auto Placement** để game tự xếp. Chạm vào ô trống không có tác dụng gì.
   - Phải có đủ 23 cầu thủ khác nhau và một huấn luyện viên trong đội hình thì mới thi đấu được.
   - Đội hình MyLeague là đội hình riêng và cũng bắt đầu trống: xếp nó trong Game Plan của MyLeague theo cách trên.
9. **Bản fix4: save đang dùng**:
   - Save giữ nguyên 70 thẻ khởi đầu, mọi thẻ đã mở và đội hình.
   - Vì gói không đưa ra thẻ mà câu lạc bộ đã có, các gói gồm nhiều thẻ khởi đầu gần như hết ngay: **ZIRU OTHERS** có thể hiện còn 0 thẻ và **ZIRU HIGHLIGHT** còn 1 thẻ. Bấm **Reset** trong gói đó để có lại đủ thẻ (cần làm trước khi mở 10 thẻ một lần ở hai gói này).
   - Thẻ nổi bật trên banner của một gói mà câu lạc bộ đã có thì chỉ ra sau khi Reset gói đó.
10. **Quay lại bản fix3**:
    - Chỉ cần cài lại IPA fix3. fix4 dùng đúng file `dt870` của fix3, nên không cần xóa hai file `dt870` trong `Download` (việc đó chỉ cần khi quay lại fix2, mục 6).
    - Nếu sau đó lại cài fix4 (fix4 → fix3 → fix4):
      - trên save mới không có cầu thủ, fix3 thêm 70 thẻ khởi đầu vào save, và chúng ở lại luôn khi quay lại fix4;
      - các gói của fix4 có thể đưa ra lại những thẻ đã mở trong lúc dùng fix3;
      - thẻ Neymar Standard hoặc De Bruyne Standard đã mở từ gói bị fix3 đổi hẳn thành Neymar Big Time hoặc De Bruyne Show Time, và vẫn như vậy khi quay lại fix4;
      - trên save đó, đặt lại điểm phát triển của một thẻ khởi đầu khi dùng fix4 đưa thẻ về chỉ số gốc chưa luyện, như thẻ đó khi mở từ gói (thẻ Standard lấy chỉ số hiện tại của Konami), và trả lại đủ điểm phát triển. Trên fix3, và trên save đang dùng khi lên fix4, đặt lại đưa thẻ khởi đầu về chỉ số trong dữ liệu khởi đầu của Konami (nhiều thẻ trong đó đã được luyện sẵn), cũng kèm đủ điểm.

---

## Kiểm thử & Phát triển (Testing)

Dự án tích hợp sẵn bộ test đầy đủ để xác minh tính tương thích trước khi phát hành:

```bash
# 1. Biên dịch bộ công cụ test trên Linux / macOS:
make -j2 build/runcpp build/packtest build/gameplaytest build/modtest build/folderroutetest build/datapacktest

# 2. Chạy hồi quy kiểm thử logic gameplay (division, career, progression, skills):
./build/gameplaytest

# 3. Kiểm thử chuyển đổi bảng dữ liệu nhị phân iOS (đặt DT870_ORIGINAL=<dt870_mobile_and.cpk gốc của Konami>
#    và DT200_ORIGINAL=<cpk/dt200_mobile_all.cpk của gói dữ liệu> để so thêm với dữ liệu của Konami):
python3 -m unittest discover -s test -p 'ios_tables_test.py' -v

# 4. Kiểm thử giao thức mã hóa máy chủ Online (Signaling + TURN):
python3 -m unittest discover -s online -p 'test_online_server.py' -v

# 5. Kiểm thử liên kết thư mục dữ liệu (Documents/pak, cpk, livedata) và nhập gói data ZIP:
./build/folderroutetest
python3 test/datapack_fixtures.py /tmp/datapack-test && ./build/datapacktest /tmp/datapack-test

# 6. Kiểm tra dữ liệu cầu thủ trong ziru-extra/responses (mỗi phía tối đa một phong cách chơi,
#    play_style khớp tham số 61/62, mọi bản sao của một thẻ khớp với ZIRU_PLAYER_RECORDS.json,
#    mỗi thẻ khởi đầu có serial riêng của Konami và bảng chuyển save core/starter_serials.inc khớp dữ liệu):
python3 -m unittest discover -s test -p 'player_data_test.py' -v
```

---

## Giấy phép & Lời cảm ơn (Credits & Disclaimer)

- **Ziru**: Tác giả nguyên bản của bản mod Android *Efootball 2027 Offline Full Epic – Bigtime*, người đã đảo ngược và xây dựng bộ API offline server gốc trên nền tảng Java.
- **Dự án mã nguồn mở**: Sử dụng các thư viện `sqlite3`, `zlib`, `msgpack-c`, và `coturn`.
- **Miễn trừ trách nhiệm**: Dự án này được phát triển hoàn toàn vì mục đích học tập, nghiên cứu kỹ thuật đảo ngược (reverse engineering) và bảo tồn trò chơi. Tất cả nhãn hiệu, bản quyền hình ảnh, âm thanh và thương hiệu eFootball thuộc về **Konami Digital Entertainment**. Vui lòng không sử dụng mã nguồn này vào mục đích thương mại.
