# Bàn giao Gemini: booster, save, D1, giật hình, áo và logo

Ngày 17/10/2026, giờ Việt Nam. Source: nhánh `codex/member-feedback-20261017` trong `D:\PES2027\pes2027-mod`. Commit mã sửa cuối: `8540a14da4688217c4301f3a42877386c14b8ab8` (gồm bổ sung lỗi số 6: skill).

Người dùng yêu cầu sửa source và báo lại để Gemini build IPA. **Chưa build/cài/phát hành IPA mới. Chưa kiểm chứng chơi trên iPhone.** Không thay đổi data trên thiết bị. Các log là feedback từ nhiều member; không gộp thành một save hoặc một tài khoản.

## Đầu vào và bằng chứng

- `D:\Telegram\PES2027-logs.zip`: 3 log, gồm crash ngày 17/10 ở `0x1047decec`, qua tính overall tự động và dựng banner trang chủ.
- `D:\Telegram\offline_server.log`, `ziru.log`, `crash.log`: có nhiều phiên, kết quả trận và save squad. Crash camera trong file này xảy ra ngày 16/10; thời điểm chép file không phải thời điểm crash.
- `D:\Telegram\PES2027-logs (2).zip`: 3 log của member khác; crash cũng tại `0x1047decec` khi dựng banner.
- Bản sao/trích xuất và phân tích: `D:\PES2027\review\2026-10-17-user-logs`.
- Không có DB/save hoặc frame trace từ các member. Log không đủ chứng minh file save đã bị xóa, xác định mọi nguyên nhân khựng hình, hay chứng minh Android texture chạy đúng trên iOS.

## Các sửa đổi đã thực hiện

### Booster có sẵn và booster tự thêm

Native iOS 11.1.1 nhận tối đa 147 phần tử parameters. `PlayerBase::SetData` tại `0x104bc4064` có bảng nhảy giới hạn index `0x92`. Index 142 và 143 ghi hai bit skill; 144–146 ghi ba ID booster, mỗi ID 10 bit. `ModeUtility::SetCommandGamePlayerToMyClubGamePlayer` tại `0x104703a64` giới hạn 147 phần tử; đường `GameplayerDispInfo` đặt booster ở `0x90`, `0x91`, `0x92`.

Server Ziru ghi booster ở 142–144, với template 145 hoặc 149 phần tử. Vì vậy ID booster có thể bị ghi thành bit skill, và client đọc booster trống/sai. `core/ios_compat.hpp` chuyển định dạng tại biên request/response; bật bằng `Config::iosPlayerLayout` trong adapter iOS. DB và template Android giữ định dạng cũ. Response được clone trước khi đổi, tránh sửa cache hoặc save qua shared pointer.

Sửa thêm `original_parameters` của màn hình cầu thủ: sao chép booster theo index 142 cố định của định dạng nội bộ, thay vì `length - 7`, vốn sai khi trộn template 149 với save 145. Khôi phục booster gốc khi slot trống, giữ booster đã huấn luyện. Hai skill bit mới của iOS hiện được điền 0 khi xuất dữ liệu Ziru; việc port các skill mới riêng của 11.1.1 vẫn chưa được xác minh.

### Skill hiển thị sai so với skill thực tế — lỗi số 6

Đã tự đối chiếu toàn bộ 1.679 record và cả hai bảng native, không cần member cung cấp từng ví dụ. Có đủ 1.679/1.679 ID trong các pack; không có ID phải dùng record cầu thủ khác. Tất cả bit skill gốc trong nguồn đều là 0 hoặc 1; bốn phần tử dư ở cuối mỗi record 149 đều bằng 0.

Bảng Android trong `libUE4.so` của APK ở file offset `0xcc12ec` có 72 skill. Bảng iOS `Player::GetDataLabel(SkillCard)` ở VA `0x10578bc00` có 74 skill. Đối chiếu theo parameter label cho thấy iOS **chèn** skill mới ở ID 25 và 68:

| ID Android | ID iOS tương ứng |
| --- | --- |
| 0–24 | giữ nguyên |
| 25–66 | cộng 1 |
| 67–71 | cộng 2 |

Vì server trước đây giữ nguyên ID, skill tự thêm và kết quả training có thể hiện thành tên skill khác. Nay chuyển đồng nhất cả `additional_skill_id` (array và scalar trong kết quả training), `skillcard_id_list` và chiều request ngược về định dạng save. DB vẫn giữ ID Android, nên không migrate lệch hoặc cộng offset hai lần. Hai skill riêng của iOS không được quảng cáo là có trong pool Ziru.

Sửa thêm hai lỗi: skill ID **0 là hợp lệ** nhưng trước đây bị loại khi nạp/lưu; pool training chỉ loại skill đã thêm mà chưa loại skill gốc cầu thủ đang có. Nay giữ ID 0 qua restart và loại cả skill gốc khi chọn skill bổ sung.

Bỏ hẳn thao tác ghi preset ngoại hình vào parameters 49–64: bảng native cho thấy vùng này chứa SecretAbility (49/50), MotionType và COM skills, không phải một block skin/face 16 phần tử như mã cũ giả định. Không dùng sửa màu da để thay đổi các thuộc tính này.

Bằng chứng kiểm kê: `D:\PES2027\review\2026-10-17-user-logs\skill-audit-result.json`. Test so đủ 72 ánh xạ với bảng iOS độc lập, roundtrip list/result, training không trùng skill gốc và giữ ID 0 sau restart. Chưa kiểm chứng tên/icon/hiệu ứng skill khi chơi trên iPhone; nếu giao diện còn sai thì phải đối chiếu cả tài nguyên UI của member.

### Lưu và khôi phục dữ liệu

`CmdCreateUser.json` không có `entry_info`. Luồng này nay bổ sung dữ liệu login rồi áp dụng cùng quy trình khôi phục squad, cầu thủ đã tuyển, progression, skill và livedata. CREATE_USER lặp lại dùng cùng save offline.

Body request được đặt trước các handler chạy sớm, tránh lấy rqid hoặc dữ liệu của lệnh trước. Preferences flush/fsync trước đổi tên. Lỗi mở/đọc/ghi SQLite và preferences được báo ra thay vì coi là đã lưu; lỗi khôi phục roster không âm thầm trả đội mặc định. Login tránh ghi lại cầu thủ nếu nội dung không đổi. Chỉ GameData bị loại khỏi backup; Save được phép backup.

Đây là các lỗi source đã xử lý, **chưa xác định chắc nguyên nhân mọi báo cáo mất data**. Các log có nhiều LOGIN là hành vi có thể bình thường; không đủ để kết luận yêu cầu đăng nhập là mất xác thực. Không đổi bundle ID, không gỡ app để cài lại nếu muốn giữ sandbox/save của member.

### D1 và độ khó

Trước đây snapshot đối thủ D10/EASY có thể còn sau khi state đã nâng D1. Sửa cả save chưa migrate và save đã có cờ `z27_div1_set`: trận chưa hoàn tất lấy division từ state, CPU level tính lại theo division. D1 trả `LEGEND`. Log chọn đối thủ/bắt đầu trận thêm division và CPU level.

Kiểm tra này xác nhận dữ liệu server gửi, chưa chứng minh AI trên iPhone khó đúng ý người chơi. Max level có sẵn trong code không tự chứng minh đã phân bổ tối đa chỉ số AI. Không tăng chỉ số tùy tiện trong bản sửa này.

### Khựng/giật, kể cả chọn 60 FPS

Dylib cũ ép cả ba đường CADisplayLink lên 120 FPS, bất kể giá trị game yêu cầu; còn ép dynamic max FPS, native resolution, contact shadows, shadow distance, anisotropy và streaming pool. Builder cũ sửa bảng FPS 60/30/20/15 thành 120/90/60/30.

Đã bỏ toàn bộ ép FPS và Ultra CVars mặc định. Bảng native giữ 60/30/20/15; game tự dùng thiết lập FPS/chất lượng. Bộ đếm drawable dùng thời gian thực giữa hai mẫu, không chia cố định 10 giây khi timer bị trễ. Đây vẫn là drawable rate trung bình, không phải frame-time trace hay bằng chứng hết giật.

Nếu có `Documents/ue4commandline.txt` của bản cũ, nó vẫn là override do người dùng cung cấp và có thể tiếp tục ép đồ họa. Khi thử baseline, sao lưu rồi đổi tên riêng file này; không xóa data/save.

Sửa bổ sung guard camera trong builder: CBZ phải kiểm tra **x19 64 bit**, không phải w19. Chưa sửa crash overall/banner `0x1047decec`; phải theo dõi riêng, không coi crash cũ đã được giải quyết bởi thay đổi FPS.

### Áo, face và logo

- Hàm symlink cũ có thể xóa file thường đang có trước khi thay bằng link, kể cả link quay về chính file qua `Download/download`. Nay giữ file thường, so inode để tránh tự trỏ, chỉ cập nhật link khi an toàn.
- Bỏ ép mọi đội thành LICENSED và bỏ đổi ID đội fallback thành một ID không có data. Giữ license/ID của đội thực tế trong template.
- Hoàn lại bảng 539 đội `ZIRU_MATCH_TEAMS.json` từ đúng APK `Ziru_27_BUGFIX.apk`; bỏ các màu được tăng độ rực/phỏng đoán trong commit trước. RGB trong bảng này chủ yếu phục vụ chọn áo tránh trùng, **không tạo texture áo/logo**.
- Bỏ ghi các preset skin/face phỏng đoán vào parameters 49–64, vì đó là vùng thuộc tính/COM skills khác theo bảng native.

**Chưa thể xác nhận áo/face/logo đã đúng đầy đủ.** Đổi tên `_mobile_and` thành `_mobile_ios` không chuyển texture/shader Android thành iOS. File iOS đã bị ghi đè trước đó cần đối chiếu/khôi phục từ nguồn iOS phù hợp; source mới chỉ ngăn ghi đè tiếp. Không xóa toàn bộ data của member. Đối chiếu từng bộ áo, emblem, face và container trước khi thay tài nguyên. Pak UI bổ sung cũng cần kiểm chứng hiển thị trên thiết bị.

## Kiểm tra

- Lượt `6dc6ebb`: https://github.com/iOSVNNews/pes2027-mod/actions/runs/39001279268 — Linux và dylib iOS thành công.
- Lượt `90a36e2`: https://github.com/iOSVNNews/pes2027-mod/actions/runs/39001444764 — Linux và dylib iOS thành công.
- Lượt `b44e567`: https://github.com/iOSVNNews/pes2027-mod/actions/runs/39001598033 — Linux và dylib iOS thành công.
- Lượt commit mã cuối `8540a14`: https://github.com/iOSVNNews/pes2027-mod/actions/runs/39001888726 — **Linux tests và dylib arm64 iOS đều thành công**, gồm test đủ 72 skill/roundtrip, skill ID 0 sống qua restart và training không trùng skill gốc. Log: `D:\PES2027\review\2026-10-17-user-logs\ci-39001888726.log`.
- Gameplay regression gồm: chuyển slot booster 145/149 → 147; request iOS → save → response; không đổi định dạng DB; CREATE_USER khôi phục hai cầu thủ/squad sau restart; snapshot D10/EASY khi state D1 với cả hai trạng thái migration; từ chối ghi DB read-only; SQLite và preferences sống qua `_exit` không chạy destructor.
- CI còn chạy 4 test online hiện có và importer self-test. Không dùng các test này để tuyên bố LAN hay hình ảnh đã được kiểm chứng trên iPhone.
- `tools/build_ipa.py` qua kiểm tra cú pháp. Dry-run patch binary thật thành công, bảng FPS native còn nguyên và guard camera kiểm tra x19. Bằng chứng: `D:\PES2027\review\2026-10-17-user-logs\builder-check.json`.

## Đầu vào build đã chuẩn bị cho Gemini

Đường dẫn app gốc cũ trong README không còn trên máy. Đã phục hồi riêng bootstrap app từ IPA hiện có, đảo đúng 52 patch và bỏ load command Ziru. SHA-256 executable sau phục hồi khớp **chính xác** binary gốc đã phân tích: `0cf9bacd9a9f049587f78a5fb8646b4ed5b87abe40a761d1f8f6337cafa9a0de`.

- App input: `D:\PES2027\review\2026-10-17-user-logs\recovered-input\PESmobile.app`.
- APK: `D:\PES2027\Ziru_27_BUGFIX.apk`.
- UI pak hiện có: `D:\PES2027\claude-build\pak_extra` (3 file, nội dung Android chuyển container; chưa chứng minh mọi ảnh hiển thị đúng).
- Python chạy được: `C:\Users\vuiga\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe`. Python mặc định ở AppData/Local/Programs hiện thiếu standard library.
- Dùng dylib của **commit 8540a14**, không lấy lại dylib cũ ở thư mục gốc. Các dylib b44e567 trở về trước chưa có sửa enum skill số 6.
- Đã tải artifact đúng commit về `D:\PES2027\artifacts\member-feedback-20261017\build\ZiruOffline.dylib`, kèm checksum CI. Chưa ghép IPA.

App input phục hồi không có pak/cpk nặng, chỉ dành build IPA với `--skip-data`. Không dùng để tái tạo gói data hoàn chỉnh. Không lấy PESmobile đã patch trong IPA cũ làm đầu vào rồi bỏ kiểm tra SHA; làm vậy giữ lại bản vá FPS cũ.

Lệnh build IPA cho Gemini (chưa chạy):

```powershell
Set-Location -LiteralPath 'D:\PES2027\pes2027-mod'
& 'C:\Users\vuiga\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' tools\build_ipa.py `
  --app 'D:\PES2027\review\2026-10-17-user-logs\recovered-input\PESmobile.app' `
  --dylib 'D:\PES2027\artifacts\member-feedback-20261017\build\ZiruOffline.dylib' `
  --apk 'D:\PES2027\Ziru_27_BUGFIX.apk' `
  --extra-pak 'D:\PES2027\claude-build\pak_extra' `
  --out 'D:\PES2027\out-member-feedback-20261017' --skip-data
```

Giữ bundle ID và cách ký phù hợp bản member đang dùng khi cài đè. Việc ký/cài và đối chiếu tài nguyên thực tế chưa được thực hiện trong lượt này.

Gemini build ra thư mục mới, giữ IPA cũ để so sánh. Trước khi báo hoàn thành cho member: kiểm tra booster có sẵn/tự thêm và ảnh hưởng chỉ số; tắt tiến trình rồi mở lại và so cầu thủ/squad; xác nhận log D1/LEGEND; thử 60 FPS với thiết lập đồ họa gốc; chụp áo/logo/face của đội cụ thể. Những lỗi hình ảnh hoặc xác thực còn lại phải ghi là chưa hoàn tất, không chỉ dựa vào CI.
