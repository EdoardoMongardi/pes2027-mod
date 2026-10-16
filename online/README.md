# PES2027 online (mod 1.1) – Friend Match qua server AWS

Bản 1.1 là bản thử nghiệm cho phòng Friend Match giữa hai iPhone, chưa xác nhận trận P2P/TURN chạy hoàn chỉnh trên thiết bị. myClub tiếp tục chạy offline trên từng máy.
Chỉ các lệnh online đi lên server này:
- phòng: tạo, vào, thông tin phòng, sẵn sàng, chọn bên, rời phòng;
- phiên trận: game session, TURN, tiến trình trận, kết quả.

## Cách hoạt động
- **Dylib** (`ios/ZiruOffline.mm`) đọc `PESmobile.app/ziru/online.json`. Người chơi có thể ghi đè bằng `Documents/PES2027-online.json`:
  `{"server": "https://IP:8443", "name": "Tên hiển thị", "enabled": true, "token": "PRIVATE_DEPLOYMENT_TOKEN", "cert_sha256": "CERTIFICATE_SHA256"}`.
  Các lệnh online được chuyển tới `server + /pes22/gate/gate_<CMD>.php`, giữ nguyên mã hóa của game, kèm UID, tên, platform IOS và token riêng của deployment. Chứng chỉ tự ký phải khớp SHA-256 đã cấu hình; không chấp nhận mọi chứng chỉ của cùng host.
- **Định danh:** mọi máy đều dùng tài khoản Ziru (user_id 359042717). Server làm việc với id riêng của từng máy. Trong mọi phản hồi, server đổi id của chính người gọi về 359042717 để game nhận ra mình.
- **Trận đấu:** chạy P2P (`game_sycom = P2P`) bằng địa chỉ NTL mà hai máy gửi lên khi tạo, vào phòng hoặc khi bấm sẵn sàng. Nếu P2P không được, game dùng TURN (coturn trên cùng máy EC2), với thông tin đăng nhập tạm thời do server cấp qua `CMD_GET_TURN_SERVER_LIST`.
- **gRPC:** tắt (`connect_grpc_task.disable = true` trong server env của Ziru). Phòng được cập nhật bằng cách hỏi định kỳ `CMD_GET_ROOM_INFO` mỗi 5 giây.

## Dựng trên AWS (khoảng 15 phút)
1. Tạo EC2 Ubuntu 24.04, vùng **ap-southeast-1 (Singapore)**, loại t3.small, gắn Elastic IP.
2. Security group mở:

   | Giao thức | Cổng | Dùng cho |
   |---|---|---|
   | TCP | 8443 | server |
   | TCP + UDP | 3478 | TURN |
   | TCP | 5349 | TURN TLS |
   | UDP | 49160-49259 | relay TURN |

3. Chép thư mục `online/` lên máy, chạy: `sudo bash setup_ec2.sh <Elastic IP>`.
4. Lấy `/etc/pes2027-online/client.json` về PC qua SSH, giữ ngoài Git. Build IPA với `--online-config PATH_TO_PRIVATE_CLIENT_JSON --skip-data` và `--extra-pak` như bản đang dùng. Token không được ghi vào build-report hoặc log console.
5. Kiểm tra: mở `https://<IP>:8443/health` (trình duyệt cảnh báo chứng chỉ tự ký là bình thường).

## Thử lần đầu (thu log)
Hai iPhone cùng IPA 1.1, cùng có mạng:
1. Máy A vào chế độ Đấu với bạn / phòng riêng, tạo phòng, đọc mã phòng (6 số).
2. Máy B tìm phòng theo mã và vào.
3. Cả hai bấm sẵn sàng.

Gửi lại để chỉnh tiếp:
- `ziru.log` của cả hai máy: các dòng `[ONLINE]`;
- `/opt/pes2027-online/logs/online-*.jsonl` trên server: request/response đã giải mã của từng lệnh.

Phần đã dò từ binary iOS 11.1.1, chưa kiểm chứng trên máy: thứ tự lệnh khi vào trận, cách NTL lấy địa chỉ reflexive, và TURN có đúng chuẩn RFC hay không. Log lần thử đầu sẽ cho biết.

## Dùng chung máy GTA và kiểm tra trên PC

`setup_ec2.sh` tạo riêng user `pes2027`, dịch vụ `pes2027-online` và `pes2027-turn`, cấu hình trong `/etc/pes2027-online`, code/log trong `/opt/pes2027-online`. Không reboot hoặc cấu hình lại GTA. API giới hạn RAM 192 MB, TURN 160 MB; UDP relay có quota và giới hạn băng thông. Script dừng nếu đã có coturn khác đang chạy.

Admin đọc trạng thái bằng `Authorization: Bearer <PES2027_ADMIN_TOKEN>`; token nằm trong file root-only `server.env`, không dùng query URL. Cấu hình/khóa/chứng chỉ không commit lên Git. Token chung là quyền truy cập cho nhóm bạn thử nghiệm, chưa phải hệ thống tài khoản công khai có xác thực từng người.

Chạy kiểm tra mã hóa HTTP với hai client mô phỏng: `python -m unittest discover -s online -p test_online_server.py -v`. Các kiểm tra này bao gồm UID remap, phòng/mật khẩu, sẵn sàng hai phía, phiên trận, trao đổi TURN, kết quả và phân quyền phòng. Chúng chưa chứng minh game native bắt tay và chơi được trên hai iPhone.
