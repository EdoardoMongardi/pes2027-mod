# Báo Cáo Bàn Giao Kỹ Thuật: Tính Năng Online Friend Match (Mod v1.1.0)
**Thời gian lập:** 17/10/2026 00:25 (+07)  
**Tác giả bàn giao:** Claude (nghiên cứu & phát triển) → Gemini (tổng hợp bàn giao)  
**Người tiếp nhận:** Codex / Kỹ sư phụ trách tiếp theo  
**Vị trí mã nguồn:** `D:\PES2027\pes2027-mod`  

---

## 1. Tóm Tắt Trạng Thái Hiện Tại (Executive Summary)

Claude đã hoàn thành **100% việc viết mã nguồn và thiết kế kiến trúc** cho tính năng **Online Friend Match (Đá giao hữu qua mạng giữa 2 iPhone)** trước khi chạm giới hạn lượt tương tác (rate limit).
Toàn bộ mã nguồn đã nằm sẵn ở thư mục làm việc cục bộ `D:\PES2027\pes2027-mod` (chưa commit lên git), bao gồm:
1. **Server trung gian (`online/pes_online_server.py`)**: Server Python xử lý mã hóa Konami, quản lý phòng đấu, đồng bộ P2P / TURN relay và remap UID người chơi.
2. **Script triển khai 1-click lên AWS EC2 (`online/setup_ec2.sh`)**: Tự động cài đặt coturn (TURN server), service systemd và SSL tự ký.
3. **Mã nguồn Client Dylib (`ios/ZiruOffline.mm`)**: Bộ định tuyến forward các lệnh phòng/trận lên server online, kèm SSL delegate tự động chấp nhận chứng chỉ tự ký.
4. **Công cụ đóng gói IPA (`tools/build_ipa.py`)**: Bổ sung cờ `--online-server` và nâng phiên bản lên `1.1.0`.
5. **Bản build dylib thử nghiệm cục bộ**: Đã được biên dịch tại `D:\PES2027\claude-build\ZiruOffline.dylib` (932.352 bytes).

---

## 2. Kiến Trúc & Nguyên Lý Hoạt Động

### 2.1. Phân Tách Luồng Offline và Online (Hybrid Architecture)
* **myClub, Rút thẻ (Agent Box), Sự kiện offline, Dữ liệu cầu thủ:** Tiếp tục chạy 100% offline trên server local (localhost) bên trong `ZiruOffline.dylib` như bản v1.0.1.
* **Friend Match (Tạo/vào phòng riêng, tìm trận bạn bè, đồng bộ thi đấu):** Dylib phát hiện các lệnh thuộc danh sách `isOnlineCommand` và chuyển tiếp trực tiếp (forward) lên Server AWS qua giao thức HTTPS (`/pes22/gate/gate_<CMD>.php`).

### 2.2. Xử Lý Vấn Đề Trùng Tài Khoản Ziru (UID Remapping - "Cốt Lõi Sáng Tạo")
* **Vấn đề:** Tất cả các máy cài bản mod Ziru đều dùng chung một profile tài khoản mặc định của Ziru (`user_id: 359042717`). Nếu để 2 máy cùng online, server thông thường sẽ xem 2 máy là cùng 1 người chơi.
* **Giải pháp của Claude:**
  * Client Dylib tự động sinh một `ZiruOnlineUid` ngẫu nhiên lưu trong `NSUserDefaults` của iPhone (ví dụ `1xxxxxxxxx`) và gửi kèm qua HTTP header `X-Ziru-Uid` cùng tên hiển thị `X-Ziru-Name`.
  * Server Python quản lý các thành viên trong phòng bằng `X-Ziru-Uid` riêng biệt của từng máy.
  * **Remap ngược:** Khi trả response về cho máy nào, server sẽ tự động duyệt cây dữ liệu (dict/list) và đổi ID của máy đó về lại `359042717` (`remap(resp, caller_uid, LOCAL_UID)`). Nhờ đó, ứng dụng eFootball trên mỗi iPhone đều nhận diện chính mình là tài khoản chủ, không hề bị lỗi xung đột ID hay mismatch dữ liệu!

### 2.3. Cơ Chế Kết Nối Trận Đấu (P2P + Fallback TURN Relay)
* Trận đấu sử dụng chế độ P2P của game (`game_sycom = P2P`).
* Hai máy trao đổi địa chỉ NTL (IP và Port NAT) thông qua các lệnh `CMD_SEND_TURN_ADDRESS_DATA` và `CMD_GET_TURN_ADDRESS_DATA`.
* Trong trường hợp cả 2 máy kết nối qua mạng 4G/LTE bị Symmetric NAT chặn kết nối P2P trực tiếp, server sẽ cung cấp credentials truy cập vào **TURN Server (coturn)** trên cùng máy chủ EC2 (được ký hạn giờ bằng HMAC-SHA1 secret thông qua lệnh `CMD_GET_TURN_SERVER_LIST`) để làm relay trung chuyển dữ liệu trận đấu.

---

## 3. Danh Mục Tệp & Thay Đổi Chi Tiết

| Đường Dẫn Tệp | Trạng Thái | Mô Tả Chức Năng |
|---|---|---|
| `online/pes_online_server.py` | Tạo mới (618 dòng) | Server Python hoàn chỉnh: giải mã AES-256-CBC, msgpack, zlib/gzip; quản lý User, Room (mã 6 số), Member, GameSession, TURN credentials, đồng bộ tỷ số/phút đấu. |
| `online/setup_ec2.sh` | Tạo mới (72 dòng) | Script triển khai 1 dòng lệnh trên Ubuntu 22.04/24.04: cài đặt coturn, venv python, service systemd `pes2027-online`, cấp chứng chỉ SSL tự ký. |
| `online/README.md` | Tạo mới (42 dòng) | Hướng dẫn vận hành server và cấu hình cổng AWS Security Group. |
| `ios/ZiruOffline.mm` | Sửa đổi (+137 dòng) | Thêm `loadOnlineConfig()`, hàm kiểm tra `isOnlineCommand()`, hàm `forwardOnline()`, và delegate `ZiruOnlineDelegate` bỏ qua xác thực SSL tự ký. |
| `tools/build_ipa.py` | Sửa đổi (+14 dòng) | Thêm tham số `--online-server`, ghi file `ziru/online.json` vào IPA, nâng `MOD_VERSION = "1.1.0"`. |
| `D:\PES2027\claude-build\ZiruOffline.dylib` | Đã build thử | File binary dylib đã được Claude biên dịch thử nghiệm cục bộ để xác minh cú pháp Objective-C++. |

---

## 4. Hướng Dẫn Các Bước Tiếp Theo Dành Cho Codex

### Bước 1: Quản Lý Git & Nhánh Phát Hành
Mã nguồn hiện tại nằm ở working tree nhánh `main` của `D:\PES2027\pes2027-mod`. Codex có thể tạo nhánh mới `feature/online-1.1` hoặc commit thẳng:
```powershell
cd D:\PES2027\pes2027-mod
git checkout -b feature/online-1.1
git add online/ ios/ZiruOffline.mm tools/build_ipa.py
git commit -m "feat(online): implement Friend Match server, coturn setup and dylib forwarding (mod 1.1)"
git push origin feature/online-1.1
```

### Bước 2: Dựng Máy Chủ AWS EC2
1. Tạo một máy chủ EC2 **Ubuntu 24.04** (Khuyến nghị vùng **Singapore `ap-southeast-1`**, cấu hình `t3.small` hoặc `t4g.small`, gắn Elastic IP cố định).
2. Cấu hình **Security Group** mở các cổng:
   * **TCP 8443**: Cổng REST API của Server game (`pes_online_server.py`).
   * **TCP + UDP 3478**: Cổng TURN điều khiển.
   * **TCP 5349**: Cổng TURN TLS.
   * **UDP 49152 - 65535**: Dãy cổng truyền dữ liệu Relay TURN.
3. Chép toàn bộ thư mục `online/` lên EC2 và chạy lệnh cài đặt:
   ```bash
   sudo bash setup_ec2.sh <ELASTIC_IP_CUA_BAN>
   ```
4. Kiểm tra sức khỏe server:
   ```bash
   curl -sk "https://<ELASTIC_IP>:8443/health"
   # Kết quả trả về: {"status": "ok", "version": "1.1.0", ...}
   ```

### Bước 3: Đóng Gói IPA Bản 1.1.0
Sau khi GitHub Actions build xong `ZiruOffline.dylib` từ commit mới:
```powershell
& "C:\Users\vuiga\AppData\Local\Python\bin\python.exe" D:\PES2027\pes2027-mod\tools\build_ipa.py `
    --app D:\Telegram\Payload\PESmobile.app `
    --dylib D:\PES2027\ZiruOffline.dylib `
    --apk "D:\Efootball 2027 Offline Full Epic - Bigtime by Ziru\Efootball_Offline_VH.apk" `
    --extra-pak D:\PES2027\claude-build\pak_extra `
    --online-server https://<ELASTIC_IP>:8443 `
    --out D:\PES2027\out --skip-data
```
*(Nếu muốn đổi server test mà không cần build lại IPA: người chơi có thể tạo file `Documents/PES2027-online.json` trên iPhone với nội dung: `{"server": "https://<IP>:8443", "name": "TenNguoiChoi", "enabled": true}`).*

### Bước 4: Kiểm Thử Thực Tế & Thu Thập Log (Testing Checklist)
Cài đặt bản IPA 1.1.0 lên 2 máy iPhone:
1. **Kiểm tra kết nối server:** Mở game, kiểm tra `ziru.log` xem có dòng:
   `[ONLINE] server https://<IP>:8443 uid <ID_RIENG> name PlayerXXXX`
2. **Tạo và vào phòng:**
   * Máy A vào mục *Trận đấu bạn bè (Friend Match)* ➡️ *Tạo phòng* ➡️ Lấy mã phòng gồm 6 chữ số.
   * Máy B vào mục *Tìm phòng* ➡️ Nhập mã 6 số ➡️ Vào phòng thành công.
3. **Sẵn sàng & Bắt đầu trận:**
   * Cả 2 chọn Home/Away, nhấn *Sẵn sàng*.
   * Quan sát tiến trình nạp trận đấu.
4. **Các điểm kỹ thuật cần theo dõi qua log:**
   * Trên iPhone: Xem `ziru.log` các dòng bắt đầu bằng `[ONLINE]`.
   * Trên Server EC2: Theo dõi file log `/opt/pes2027-online/logs/online-*.jsonl` (ghi lại toàn bộ request/response đã giải mã).
   * **Lưu ý kiểm chứng:** Thứ tự bắt tay P2P thực tế giữa 2 máy iOS, kiểm tra xem NTL có thể đục lỗ NAT thành công hay game sẽ tự động switch sang TURN relay IP trên cổng 3478.
