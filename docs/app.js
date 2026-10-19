"use strict";

const catalog = JSON.parse(Android.getCatalog());
const basePacks = cloneJson(catalog.packs);
const baseCommandTemplate = cloneJson(catalog.command_template);
const baseConfigTemplate = cloneJson(catalog.config_template);
const baseMainMenuTemplate = cloneJson(catalog.mainmenu_template);
const basePackIds = new Set(basePacks.map(pack => pack.agent_id));
const basePlayerIds = new Set(Object.keys(catalog.records));
const livePlayerIds = new Set(Object.keys(catalog.live_players || {}));
const baseRecords = cloneJson(catalog.records);
const basePlayers = cloneJson(catalog.players);
const emptyState = document.getElementById("empty-state");
const editorContent = document.getElementById("editor-content");
const packList = document.getElementById("pack-list");
const playerSearch = document.getElementById("player-search");
const translations = {
  vi: {
    subtitle: "Công cụ sửa Database & Gói thẻ eFootball (iOS)", offline: "OFFLINE", overview: "TỔNG QUAN",
    packages: "Gói Cầu Thủ", intro: "Chỉnh sửa gói mở thẻ và danh sách cầu thủ. Dữ liệu lưu trực tiếp trên thiết bị của bạn.",
    "package-kind": "GÓI THẺ",
    "package-banner": "BANNER GÓI", "banner-background": "Hình nền gói thẻ",
    "select-banner": "Chọn Banner", "clear-banner": "Xóa Banner",
    "banner-help": "Ảnh được chọn sẽ hiển thị làm hình nền của gói thẻ trong game.",
    "banner-error": "Không thể tải banner: {message}", "banner-removed": "Đã xóa banner gói thẻ.",
    "banner-loaded": "Đã thêm banner gói thẻ.", "banner-too-large": "Ảnh banner phải nhỏ hơn 8 MB.",
    export: "Xuất lệnh / Nạp vào game", "pack-count-label": "GÓI THẺ", "player-records": "CƠ SỞ DỮ LIỆU",
    "total-cards": "TỔNG CẦU THỦ", "package-list": "DANH SÁCH GÓI", "search-packages": "Tìm gói thẻ...",
    "new-package": "Tạo Gói Mới", loading: "Đang tải dữ liệu…", "package-settings": "CÀI ĐẶT GÓI THẺ",
    "delete-package": "Xóa Gói", "package-title": "TÊN GÓI THẺ", "package-title-placeholder": "Nhập tên gói thẻ",
    "featured-players": "CẦU THỦ TIÊU BIỂU", "first-three": "3 cầu thủ nổi bật hàng đầu",
    "player-ids": "ID CẦU THỦ", squad: "ĐỘI HÌNH", "player-list": "Danh Sách Cầu Thủ",
    "search-players": "Tìm theo tên hoặc ID cầu thủ...", "auto-save": "Tự động lưu thay đổi",
    "manual-player-id": "Nhập ID Cầu thủ", "add-by-id": "Thêm ID",
    undo: "Hoàn tác", redo: "Làm lại", duplicate: "Nhân bản", backup: "Sao lưu",
    import: "Nhập file", changes: "Thay đổi", validate: "Kiểm tra",
    "all-positions": "Tất cả vị trí", "filter-age": "Tuổi", "bulk-add": "Thêm Hàng Loạt",
    "bulk-placeholder": "Nhập các ID cách nhau bằng dấu phẩy, khoảng trắng hoặc xuống dòng", "add-all": "Thêm Tất Cả",
    "featured-select": "Chọn Tiêu Biểu", "edit-player": "Sửa Chỉ Số Cầu Thủ",
    "move-left": "Sang Trái", "move-right": "Sang Phải",
    "edit-featured": "Sửa Tiêu Biểu", "player-editor": "Chỉ Số Cầu Thủ",
    "restore-original": "Khôi Phục Gốc",
    "player-record-missing": "Không tìm thấy dữ liệu cho ID: {id}",
    "player-original-restored": "Đã khôi phục chỉ số gốc của {name}.",
    "level-label": "Cấp độ", "height-label": "Chiều cao (cm)", "weight-label": "Cân nặng (kg)",
    "stats-label": "Chỉ Số Kỹ Thuật", save: "Lưu Lại", cancel: "Hủy",
    "player-updated": "Đã cập nhật cầu thủ {name}.",
    "offensive-awareness": "Nhận thức tấn công", "ball-control": "Kiểm soát bóng",
    dribbling: "Rê bóng", "tight-possession": "Giữ bóng trong chân", "low-pass": "Chuyền sệt",
    "lofted-pass": "Chuyền bổng", finishing: "Dứt điểm", heading: "Đánh đầu",
    "set-piece-taking": "Sút phạt", curl: "Độ xoáy", speed: "Tốc độ", acceleration: "Tăng tốc",
    "kicking-power": "Lực sút", jumping: "Bật nhảy", "physical-contact": "Tì đè thể chất",
    balance: "Thăng bằng", stamina: "Thể lực", "defensive-awareness": "Nhận thức phòng ngự",
    "defensive-engagement": "Tham gia phòng ngự",
    "ball-winning": "Đoạt bóng", aggression: "Quyết liệt", "gk-awareness": "Nhận thức thủ môn",
    "gk-catching": "Bắt bóng", "gk-clearing": "Phá bóng",
    "gk-reflexes": "Phản xạ thủ môn", "gk-reach": "Tầm với thủ môn",
    "featured-limit": "Tối đa 3 cầu thủ tiêu biểu.",
    "duplicate-title": "{title} (Bản sao)", "backup-title": "Sao Lưu Gói Thẻ",
    "import-title": "Nhập Tệp Cấu Hình", "import-success": "Đã nhập thành công {count} gói thẻ.",
    "import-error": "Nhập thất bại: {message}", "no-import-packs": "Không có gói thẻ hợp lệ.",
    "changes-title": "Danh Sách Thay Đổi", "no-changes": "Chưa có thay đổi nào.",
    "added-pack": "Gói Đã Thêm", "removed-pack": "Gói Đã Xóa", "changed-pack": "Gói Đã Sửa",
    "added-player-label": "Cầu Thủ Đã Thêm", "removed-player-label": "Cầu Thủ Đã Bỏ",
    "title-changed": "Tên đã đổi", "validate-title": "Kiểm Tra Trước Khi Xuất",
    "validation-ok": "Gói thẻ đã chuẩn xác, sẵn sàng xuất!", "validation-errors": "Cần sửa các mục sau trước khi xuất:",
    "no-backup-file": "Tệp sao lưu trống.", "package-copy": "Đã nhân bản gói thẻ.",
    "bulk-result": "Đã thêm {added} cầu thủ; bỏ qua {skipped} ID trùng lặp.",
    "delete-player-confirm": "Bạn có chắc muốn xóa cầu thủ này khỏi gói?",
    "backup-saved": "Đã lưu bản sao lưu.", "backup-error": "Sao lưu thất bại: {message}",
    "invalid-package-title": "Mỗi gói thẻ phải có một tên gọi.",
    "invalid-package-id": "ID gói hoặc cầu thủ không hợp lệ.",
    "invalid-featured": "Cầu thủ tiêu biểu phải nằm trong danh sách gói (tối đa 3 người).",
    "invalid-banner": "Ảnh banner không hợp lệ.",
    "invalid-player-record": "Dữ liệu không khớp với ID: {id}",
    "confirm-remove": "Bỏ cầu thủ này khỏi gói thẻ?", "import-backup": "Đã khôi phục bản sao lưu.",
    "import-packages": "Đã nhập {count} gói thẻ.", "import-config": "Đã nhập cấu hình.",
    "saved-locally": "Đã lưu cục bộ trên máy", "local-only": "OFFLINE HOÀN TOÀN",
    "no-server": "KHÔNG CẦN INTERNET", "featured-number": "TIÊU BIỂU",
    "no-featured": "Chưa có cầu thủ tiêu biểu", "add-player-hint": "Thêm cầu thủ vào gói để hiển thị ở đây.",
    "show-less": "THU GỌN", "show-all": "XEM TẤT CẢ", "unknown-age": "chưa rõ tuổi",
    "age-label": "tuổi", add: "Thêm Vào Gói", remove: "Bỏ Khỏi Gói",
    "no-results": "Không tìm thấy kết quả", "no-results-hint": "Thử tìm bằng tên khác hoặc nhập ID.",
    "invalid-player-id-input": "Vui lòng nhập số ID hợp lệ.",
    "already-in-package": "Cầu thủ này đã có trong gói thẻ rồi.",
    "added-player": "Đã thêm {name} vào gói thẻ.", "removed-player": "Đã bỏ {name} khỏi gói thẻ.",
    "invalid-saved-state": "Lỗi dữ liệu đã lưu, đã tải lại cấu hình mặc định.",
    "custom-pack-error": "Không thể tải gói thẻ: {id}",
    "template-error": "Không tìm thấy template mẫu: {id}",
    "invalid-player-id": "ID cầu thủ không hợp lệ: {id}",
    "list-error": "Không tìm thấy dữ liệu: {key} ({id})",
    "new-pack-title": "Gói {category} Mới", "new-pack-created": "Đã tạo gói mới. Hãy đặt tên và chọn cầu thủ.",
    "minimum-pack": "Phải giữ lại ít nhất một gói thẻ.", "pack-deleted": "Đã xóa gói thẻ.",
    "export-error": "Xuất file thất bại: {message}", "export-success": "Đã xuất cấu hình thành công!",
    "file-error": "Lỗi ghi tệp.", "data-error": "Lỗi đọc dữ liệu gói thẻ."
  },
  id: {
    subtitle: "Editor Database & Paket eFootball", offline: "OFFLINE", overview: "RINGKASAN",
    packages: "Paket", intro: "Kelola paket dan data pemain. Perubahan disimpan di perangkat Anda.",
    "package-kind": "PAKET",
    "package-banner": "BANNER PAKET", "banner-background": "Latar belakang paket",
    "select-banner": "Pilih Banner", "clear-banner": "Hapus Banner",
    "banner-help": "Gambar yang dipilih akan ditampilkan sebagai latar belakang paket.",
    "banner-error": "Gagal memuat banner: {message}", "banner-removed": "Banner paket dihapus.",
    "banner-loaded": "Banner paket ditambahkan.", "banner-too-large": "Banner harus lebih kecil dari 8 MB.",
    export: "Ekspor Komando", "pack-count-label": "PAKET", "player-records": "DATABASE PEMAIN",
    "total-cards": "TOTAL KARTU", "package-list": "DAFTAR PAKET", "search-packages": "Cari paket...",
    "new-package": "Paket Baru", loading: "Memuat paket…", "package-settings": "PENGATURAN PAKET",
    "delete-package": "Hapus Paket", "package-title": "NAMA PAKET", "package-title-placeholder": "Nama paket",
    "featured-players": "PEMAIN UNGGULAN", "first-three": "Tiga pemain pertama",
    "player-ids": "ID PEMAIN", squad: "SKUAD", "player-list": "Daftar Pemain",
    "search-players": "Cari pemain dengan nama atau ID...", "auto-save": "Perubahan otomatis disimpan",
    "manual-player-id": "Masukkan ID Pemain", "add-by-id": "Tambah ID",
    undo: "Batal", redo: "Ulangi", duplicate: "Duplikat", backup: "Cadangkan",
    import: "Impor", changes: "Perubahan", validate: "Validasi",
    "all-positions": "Semua Posisi", "filter-age": "Usia", "bulk-add": "Tambah Massal",
    "bulk-placeholder": "Pisahkan ID dengan koma, spasi, atau baris baru", "add-all": "Tambah Semua",
    "featured-select": "Pilih Unggulan", "edit-player": "Edit Atribut Pemain",
    "move-left": "Geser Kiri", "move-right": "Geser Kanan",
    "edit-featured": "Edit Unggulan", "player-editor": "Atribut Pemain",
    "restore-original": "Kembalikan Asli",
    "player-record-missing": "Tidak ada data pemain untuk ID: {id}",
    "player-original-restored": "Atribut asli {name} telah dipulihkan.",
    "level-label": "Level", "height-label": "Tinggi (cm)", "weight-label": "Berat (kg)",
    "stats-label": "Statistik Pemain", save: "Simpan", cancel: "Batal",
    "player-updated": "{name} diperbarui.",
    "offensive-awareness": "Kesadaran Menyerang", "ball-control": "Kontrol Bola",
    dribbling: "Dribel", "tight-possession": "Penguasaan Bola Rapat", "low-pass": "Umpan Rendah",
    "lofted-pass": "Umpan Lambung", finishing: "Penyelesaian", heading: "Sundulan",
    "set-piece-taking": "Bola Mati", curl: "Lengkungan", speed: "Kecepatan", acceleration: "Akselerasi",
    "kicking-power": "Kekuatan Tendangan", jumping: "Lompatan", "physical-contact": "Kontak Fisik",
    balance: "Keseimbangan", stamina: "Stamina", "defensive-awareness": "Kesadaran Bertahan",
    "defensive-engagement": "Keterlibatan Bertahan",
    "ball-winning": "Perebutan Bola", aggression: "Agresivitas", "gk-awareness": "Kesadaran Kiper",
    "gk-catching": "Tangkapan Kiper", "gk-clearing": "Tepis Kiper",
    "gk-reflexes": "Refleks Kiper", "gk-reach": "Jangkauan Kiper",
    "featured-limit": "Maksimal 3 pemain unggulan.",
    "duplicate-title": "{title} (Salinan)", "backup-title": "Cadangan Paket",
    "import-title": "Impor Berkas", "import-success": "{count} paket berhasil diimpor.",
    "import-error": "Gagal mengimpor: {message}", "no-import-packs": "Tidak ada paket valid.",
    "changes-title": "Perubahan Paket", "no-changes": "Belum ada perubahan.",
    "added-pack": "Paket Ditambahkan", "removed-pack": "Paket Dihapus", "changed-pack": "Paket Diubah",
    "added-player-label": "Pemain Ditambahkan", "removed-player-label": "Pemain Dikeluarkan",
    "title-changed": "Nama diubah", "validate-title": "Validasi Ekspor",
    "validation-ok": "Paket siap diekspor.", "validation-errors": "Perbaiki sebelum ekspor:",
    "no-backup-file": "Berkas cadangan kosong.", "package-copy": "Paket diduplikat.",
    "bulk-result": "Ditambahkan {added} ID; dilewati {skipped} ID yang sudah ada/duplikat.",
    "delete-player-confirm": "Hapus pemain ini?",
    "backup-saved": "Cadangan disimpan.", "backup-error": "Cadangan gagal: {message}",
    "invalid-package-title": "Setiap paket harus memiliki nama.",
    "invalid-package-id": "ID paket atau pemain tidak valid atau duplikat.",
    "invalid-featured": "Pemain unggulan harus ada di dalam paket (maks 3).",
    "invalid-banner": "Gambar banner tidak valid.",
    "invalid-player-record": "Data pemain tidak cocok dengan ID: {id}",
    "confirm-remove": "Keluarkan pemain ini dari paket?", "import-backup": "Cadangan dipulihkan.",
    "import-packages": "{count} paket diimpor.", "import-config": "Konfigurasi paket diimpor.",
    "saved-locally": "Tersimpan secara lokal", "local-only": "LOKAL OFFLINE",
    "no-server": "TANPA KONEKSI SERVER", "featured-number": "UNGGULAN",
    "no-featured": "Belum ada pemain", "add-player-hint": "Tambahkan pemain untuk melihat di sini.",
    "show-less": "SEDIKIT", "show-all": "SEMUA", "unknown-age": "usia tidak diketahui",
    "age-label": "tahun", add: "Tambah Pemain", remove: "Hapus Pemain",
    "no-results": "Tidak ditemukan", "no-results-hint": "Cari dengan nama atau ID pemain.",
    "invalid-player-id-input": "Masukkan angka ID yang valid.",
    "already-in-package": "Pemain ini sudah ada di dalam paket.",
    "added-player": "{name} ditambahkan ke paket.", "removed-player": "{name} dikeluarkan dari paket.",
    "invalid-saved-state": "Gagal membaca data tersimpan; paket bawaan dimuat.",
    "custom-pack-error": "Gagal memuat paket: {id}",
    "template-error": "Template komando tidak ditemukan: {id}",
    "invalid-player-id": "ID Pemain tidak valid: {id}",
    "list-error": "Data komando tidak ditemukan: {key} ({id})",
    "new-pack-title": "Paket {category} Baru", "new-pack-created": "Paket dibuat. Masukkan judul dan tambahkan pemain.",
    "minimum-pack": "Minimal harus ada satu paket.", "pack-deleted": "Paket dihapus.",
    "export-error": "Ekspor gagal: {message}", "export-success": "Komando paket disimpan sebagai file ZIP.",
    "file-error": "Gagal menulis berkas.", "data-error": "Gagal membaca data paket."
  },
  tr: {
    subtitle: "Yerel paket ve oyuncu düzenleyici", offline: "ÇEVRİMDIŞI", overview: "GENEL BAKIŞ",
    packages: "Paketler", intro: "Paketleri ve oyuncu listelerini düzenleyin. Değişiklikler bu cihazda saklanır.",
    "package-kind": "PAKET",
    "package-banner": "PAKET BANNER’I", "banner-background": "Paket arka planı",
    "select-banner": "Banner seç", "clear-banner": "Banner’ı kaldır",
    "banner-help": "Seçtiğiniz görsel paket arka planı olarak önizlenir ve komut ZIP’ine eklenir.",
    "banner-error": "Banner yüklenemedi: {message}", "banner-removed": "Paket banner’ı kaldırıldı.",
    "banner-loaded": "Paket banner’ı eklendi.", "banner-too-large": "Banner 8 MB’tan küçük olmalıdır.",
    export: "Komutları dışa aktar", "pack-count-label": "PAKET", "player-records": "OYUNCU KAYDI",
    "total-cards": "TOPLAM KART", "package-list": "PAKET LİSTESİ", "search-packages": "Paket ara...",
    "new-package": "Yeni paket", loading: "Paketler yükleniyor…", "package-settings": "PAKET AYARLARI",
    "delete-package": "Paketi sil", "package-title": "PAKET BAŞLIĞI", "package-title-placeholder": "Paket adı",
    "featured-players": "ÖNE ÇIKAN OYUNCULAR", "first-three": "İlk üç oyuncu",
    "player-ids": "OYUNCU ID’LERİ", squad: "KADRO", "player-list": "Oyuncu listesi",
    "search-players": "Oyuncu adı veya ID ile ara...", "auto-save": "Değişiklikler otomatik kaydedilir",
    "manual-player-id": "Oyuncu ID’si gir", "add-by-id": "ID ekle",
    undo: "Geri al", redo: "Yinele", duplicate: "Paketi kopyala", backup: "Yedekle",
    import: "İçe aktar", changes: "Değişiklikler", validate: "Doğrula",
    "all-positions": "Tüm pozisyonlar", "filter-age": "Yaş", "bulk-add": "Toplu ID",
    "bulk-placeholder": "ID’leri virgül, boşluk veya satırla ayırın", "add-all": "Tümünü ekle",
    "featured-select": "Öne çıkan olarak ayarla", "edit-player": "Oyuncu özelliklerini düzenle",
    "move-left": "Sola taşı", "move-right": "Sağa taşı",
    "edit-featured": "Öne çıkanları düzenle", "player-editor": "Oyuncu özellikleri",
    "restore-original": "Orijinal özellikleri geri yükle",
    "player-record-missing": "Bu ID için gerçek oyuncu kaydı yok: {id}",
    "player-original-restored": "{name} oyuncusunun orijinal özellikleri geri yüklendi.",
    "level-label": "Seviye", "height-label": "Boy (cm)", "weight-label": "Kilo (kg)",
    "stats-label": "Oyuncu istatistikleri", save: "Kaydet", cancel: "İptal",
    "player-updated": "{name} güncellendi.",
    "offensive-awareness": "Hücum farkındalığı", "ball-control": "Top kontrolü",
    dribbling: "Dribbling", "tight-possession": "Dar alan kontrolü", "low-pass": "Yerden pas",
    "lofted-pass": "Havadan pas", finishing: "Bitiricilik", heading: "Kafa vuruşu",
    "set-piece-taking": "Duran top",     curl: "Falso", speed: "Hız", acceleration: "Hızlanma",
    "kicking-power": "Şut gücü", jumping: "Sıçrama", "physical-contact": "Fiziksel temas",
    balance: "Denge", stamina: "Dayanıklılık", "defensive-awareness": "Savunma farkındalığı",
    "ball-winning": "Top kapma", aggression: "Agresiflik", "gk-awareness": "Kaleci farkındalığı",
    "gk-catching": "Kaleci tutuş", "gk-clearing": "Kaleci uzaklaştırma",
    "gk-reflexes": "Kaleci refleksleri", "gk-reach": "Kaleci uzanma",
    "featured-limit": "En fazla üç öne çıkan oyuncu seçilebilir.",
    "duplicate-title": "{title} (Kopya)", "backup-title": "Paket yedeği",
    "import-title": "Dosya içe aktar", "import-success": "{count} paket içe aktarıldı.",
    "import-error": "Dosya içe aktarılamadı: {message}", "no-import-packs": "Dosyada kullanılabilir paket bulunamadı.",
    "changes-title": "Paket değişiklikleri", "no-changes": "Henüz paket değişikliği yok.",
    "added-pack": "Eklenen paket", "removed-pack": "Silinen paket", "changed-pack": "Değiştirilen paket",
    "added-player-label": "Eklenen oyuncular", "removed-player-label": "Çıkarılan oyuncular",
    "title-changed": "Başlık değiştirildi", "validate-title": "Dışa aktarma doğrulaması",
    "validation-ok": "Paketler dışa aktarmaya hazır.", "validation-errors": "Dışa aktarma öncesi düzeltilmesi gerekenler:",
    "no-backup-file": "Yedek dosyası boş.", "package-copy": "Paket kopyalandı.",
    "bulk-result": "{added} ID eklendi, {skipped} mevcut/tekrarlı ID atlandı.",
    "delete-player-confirm": "Oyuncu çıkarılsın mı?",
    "backup-saved": "Yedek dosyası kaydedildi.", "backup-error": "Yedekleme başarısız: {message}",
    "invalid-package-title": "Her paketin bir başlığı olmalıdır.",
    "invalid-package-id": "Paket veya oyuncu ID’leri geçersiz ya da yinelenmiş.",
    "invalid-featured": "Öne çıkan oyuncular pakette bulunmalı ve en fazla üç kişi olmalıdır.",
    "invalid-banner": "Paket banner görseli geçersiz.",
    "invalid-player-record": "Oyuncu kaydı geçersiz veya ID ile eşleşmiyor: {id}",
    "confirm-remove": "Bu oyuncu paketten çıkarılsın mı?", "import-backup": "Yedek geri yüklendi.",
    "import-packages": "{count} paket içe aktarıldı.", "import-config": "Paket yapılandırması içe aktarıldı.",
    "saved-locally": "Yerel olarak kaydedildi", "local-only": "YALNIZCA YEREL",
    "no-server": "SUNUCU BAĞLANTISI YOK", "featured-number": "ÖNE ÇIKAN",
    "no-featured": "Henüz oyuncu yok", "add-player-hint": "Bir oyuncu ekleyince burada görünür.",
    "show-less": "DAHA AZ GÖSTER", "show-all": "TÜMÜNÜ GÖR", "unknown-age": "yaş bilgisi yok",
    "age-label": "yaş", add: "Oyuncuyu ekle", remove: "Oyuncuyu çıkar",
    "no-results": "Eşleşme bulunamadı", "no-results-hint": "Oyuncu adı veya ID ile arayın.",
    "invalid-player-id-input": "1 ile 9007199254740991 arasında geçerli bir sayısal ID girin.",
    "already-in-package": "Bu oyuncu zaten pakette.",
    "added-player": "{name} pakete eklendi.", "removed-player": "{name} paketten çıkarıldı.",
    "invalid-saved-state": "Kayıtlı düzenleme okunamadı; varsayılan paketler açıldı.",
    "custom-pack-error": "Kayıtlı özel paket açılamadı: {id}",
    "template-error": "Paket komut şablonu bulunamadı: {id}",
    "invalid-player-id": "Geçersiz oyuncu ID’si: {id}",
    "list-error": "Komut verisi bulunamadı: {key} ({id})",
    "new-pack-title": "Yeni {category} paketi", "new-pack-created": "Yeni paket oluşturuldu. Başlığı belirleyin ve oyuncu ekleyin.",
    "minimum-pack": "En az bir paket kalmalıdır.", "pack-deleted": "Paket silindi.",
    "export-error": "Dışa aktarma başarısız: {message}", "export-success": "Paket komutları ZIP olarak kaydedildi.",
    "file-error": "Dosya yazma akışı açılamadı. ", "data-error": "Paket verisi okunamadı."
  },
  en: {
    subtitle: "Local package and player editor", offline: "OFFLINE", overview: "OVERVIEW",
    packages: "Packages", intro: "Edit packages and player lists. Changes are saved on this device.",
    "package-kind": "PACKAGE",
    "package-banner": "PACKAGE BANNER", "banner-background": "Package background",
    "select-banner": "Choose banner", "clear-banner": "Remove banner",
    "banner-help": "The selected image is previewed as the package background and included in the command ZIP.",
    "banner-error": "Could not load banner: {message}", "banner-removed": "Package banner removed.",
    "banner-loaded": "Package banner added.", "banner-too-large": "Banner must be smaller than 8 MB.",
    export: "Export commands", "pack-count-label": "PACKAGES", "player-records": "PLAYER RECORDS",
    "total-cards": "TOTAL CARDS", "package-list": "PACKAGES", "search-packages": "Search packages...",
    "new-package": "New package", loading: "Loading packages…", "package-settings": "PACKAGE SETTINGS",
    "delete-package": "Delete package", "package-title": "PACKAGE TITLE", "package-title-placeholder": "Package name",
    "featured-players": "FEATURED PLAYERS", "first-three": "First three players",
    "player-ids": "PLAYER IDS", squad: "SQUAD", "player-list": "Player list",
    "search-players": "Search players by name or ID...", "auto-save": "Changes are saved automatically",
    "manual-player-id": "Enter a player ID", "add-by-id": "Add ID",
    undo: "Undo", redo: "Redo", duplicate: "Duplicate", backup: "Back up",
    import: "Import", changes: "Changes", validate: "Validate",
    "all-positions": "All positions", "filter-age": "Age", "bulk-add": "Bulk IDs",
    "bulk-placeholder": "Separate IDs with commas, spaces, or new lines", "add-all": "Add all",
    "featured-select": "Set as featured", "edit-player": "Edit player attributes",
    "move-left": "Move left", "move-right": "Move right",
    "edit-featured": "Edit featured players", "player-editor": "Player attributes",
    "restore-original": "Restore original attributes",
    "player-record-missing": "No real player record exists for this ID: {id}",
    "player-original-restored": "Original attributes restored for {name}.",
    "level-label": "Level", "height-label": "Height (cm)", "weight-label": "Weight (kg)",
    "stats-label": "Player stats", save: "Save", cancel: "Cancel",
    "player-updated": "{name} updated.",
    "offensive-awareness": "Offensive awareness", "ball-control": "Ball control",
    dribbling: "Dribbling", "tight-possession": "Tight possession", "low-pass": "Low pass",
    "lofted-pass": "Lofted pass", finishing: "Finishing", heading: "Heading",
    "set-piece-taking": "Set-piece taking", curl: "Curl", speed: "Speed", acceleration: "Acceleration",
    "kicking-power": "Kicking power", jumping: "Jumping", "physical-contact": "Physical contact",
    balance: "Balance", stamina: "Stamina", "defensive-awareness": "Defensive awareness",
"defensive-engagement": "Defensive engagement",
"ball-winning": "Ball winning", aggression: "Aggression", "gk-awareness": "GK awareness",
    "gk-catching": "GK catching", "gk-clearing": "GK clearing",
    "gk-reflexes": "GK reflexes", "gk-reach": "GK reach",
    "featured-limit": "Select up to three featured players.",
    "duplicate-title": "{title} (Copy)", "backup-title": "Package backup",
    "import-title": "Import file", "import-success": "Imported {count} packages.",
    "import-error": "Could not import file: {message}", "no-import-packs": "No usable packages found in the file.",
    "changes-title": "Package changes", "no-changes": "There are no package changes yet.",
    "added-pack": "Added package", "removed-pack": "Deleted package", "changed-pack": "Changed package",
    "added-player-label": "Added players", "removed-player-label": "Removed players",
    "title-changed": "Title changed", "validate-title": "Export validation",
    "validation-ok": "Packages are ready to export.", "validation-errors": "Fix these issues before exporting:",
    "no-backup-file": "The backup file is empty.", "package-copy": "Package duplicated.",
    "bulk-result": "Added {added} IDs; skipped {skipped} existing or duplicate IDs.",
    "delete-player-confirm": "Remove player?",
    "backup-saved": "Backup file saved.", "backup-error": "Backup failed: {message}",
    "invalid-package-title": "Every package must have a title.",
    "invalid-package-id": "Package or player IDs are invalid or duplicated.",
    "invalid-featured": "Featured players must be in the package and limited to three.",
    "invalid-banner": "Package banner image is invalid.",
    "invalid-player-record": "Player record is invalid or does not match its ID: {id}",
    "confirm-remove": "Remove this player from the package?", "import-backup": "Backup restored.",
    "import-packages": "Imported {count} packages.", "import-config": "Package configuration imported.",
    "saved-locally": "Saved locally", "local-only": "LOCAL ONLY",
    "no-server": "NO SERVER CONNECTION", "featured-number": "FEATURED",
    "no-featured": "No players yet", "add-player-hint": "Add a player to see them here.",
    "show-less": "SHOW LESS", "show-all": "SHOW ALL", "unknown-age": "age unavailable",
    "age-label": "years", add: "Add player", remove: "Remove player",
    "no-results": "No matches found", "no-results-hint": "Search by player name or ID.",
    "invalid-player-id-input": "Enter a numeric ID between 1 and 9007199254740991.",
    "already-in-package": "This player is already in the package.",
    "added-player": "{name} added to the package.", "removed-player": "{name} removed from the package.",
    "invalid-saved-state": "Saved changes could not be read; default packages were loaded.",
    "custom-pack-error": "Could not load saved package: {id}",
    "template-error": "Package command template not found: {id}",
    "invalid-player-id": "Invalid player ID: {id}",
    "list-error": "Command data not found: {key} ({id})",
    "new-pack-title": "New {category} package", "new-pack-created": "Package created. Set a title and add players.",
    "minimum-pack": "At least one package must remain.", "pack-deleted": "Package deleted.",
    "export-error": "Export failed: {message}", "export-success": "Package commands saved as a ZIP file.",
    "file-error": "Could not write export: ", "data-error": "Could not read package data."
  }
};
let state;
let selectedAgentId = null;
let showAllPlayers = false;
let toastTimer;
let history = [];
let historyIndex = -1;
let restoringHistory = false;
let searchTimer;

function t(key, values = {}) {
  let text = translations[state?.language || "vi"][key] || translations.tr[key] || key;
  Object.entries(values).forEach(([name, value]) => {
    text = text.replaceAll(`{${name}}`, String(value));
  });
  return text;
}

function applyLanguage() {
  const language = state?.language || "vi";
  document.documentElement.lang = language;
  document.title = language === "vi" ? "eFootbase - Trình Quản Lý & Chỉnh Sửa eFootball" : (language === "en" ? "eFootbase - Ziru Database Editor" : "Offline Pack Manager");
  document.querySelectorAll("[data-i18n]").forEach(element => {
    element.textContent = t(element.dataset.i18n);
  });
  document.querySelectorAll("[data-i18n-placeholder]").forEach(element => {
    element.placeholder = t(element.dataset.i18nPlaceholder);
  });
  const toggleBtn = document.getElementById("language-toggle");
  if (toggleBtn) toggleBtn.textContent = (state?.language || "vi").toUpperCase();
  const clearSearch = document.getElementById("clear-player-search");
  if (clearSearch) clearSearch.setAttribute("aria-label", language === "vi" ? "Xóa tìm kiếm" : (language === "tr" ? "Aramayı temizle" : "Clear search"));
  const mobSelect = document.getElementById("mobile-package-select");
  if (mobSelect) mobSelect.setAttribute("aria-label", language === "vi" ? "Chọn gói thẻ" : (language === "tr" ? "Paket seç" : "Select package"));
  const posFilter = document.getElementById("position-filter");
  if (posFilter) posFilter.setAttribute("aria-label", t("all-positions"));
  const ageFilter = document.getElementById("age-filter");
  if (ageFilter) ageFilter.setAttribute("aria-label", t("filter-age"));
  renderPackList();
  renderRoster();
  renderFeatured();
}

function freshState() {
  return {
    language: "vi",
    custom_records: {},
    player_overrides: {},
    player_originals: {},
    live_player_overrides: {},
    custom_players: {},
    packs: catalog.packs.map(pack => ({
      agent_id: pack.agent_id,
      title: pack.title,
      player_ids: [...pack.player_ids],
      featured_ids: [...(pack.featured_ids || [])],
      legacy_title: pack.legacy_title,
      legacy_featured_ids: [...(pack.legacy_featured_ids || [])],
      category: pack.category,
      pack_id: pack.pack_id,
      custom: Boolean(pack.custom),
      source_agent_id: pack.source_agent_id || pack.agent_id
    }))
  };
}

function loadState(savedOverride = null) {
  const saved = savedOverride ?? Android.getSavedState();
  if (!saved) return freshState();
  try {
    const parsed = JSON.parse(saved);
    Object.entries(parsed.player_overrides || {}).forEach(([id, record]) => {
      if (isValidPlayerId(Number(id)) && record?.gameplayer_id?.id === Number(id)) {
        catalog.records[id] = record;
      }
    });
    Object.entries(parsed.custom_players || {}).forEach(([id, player]) => {
      if (isValidPlayerId(Number(id)) && player && typeof player.name === "string") {
        catalog.players[id] = player;
      }
    });
    Object.entries(parsed.custom_records || {}).forEach(([id, record]) => {
      if (isValidPlayerId(Number(id)) && record?.gameplayer_id?.id === Number(id) && !basePlayerIds.has(id)) {
        catalog.records[id] = record;
        catalog.players[id] = parsed.custom_players?.[id] || { name: `ID ${id}`, position: "—", age: "—" };
      }
    });
    const originalIds = new Set(catalog.packs.map(pack => pack.agent_id));
    (parsed.packs || []).filter(pack => pack.custom && !originalIds.has(Number(pack.agent_id))).forEach(pack => {
      const agentId = Number(pack.agent_id);
      const sourceId = Number(pack.source_agent_id);
      const sourcePack = catalog.packs.find(item => item.agent_id === sourceId) || catalog.packs[0];
      const sourceCommand = catalog.command_template.agent_list.find(agent => agent.agent_id === sourceId) ||
        catalog.command_template.agent_list.find(agent => agent.agent_id === sourcePack.agent_id);
      const sourceConfig = catalog.config_template.packs.find(item => item.agent_id === sourcePack.agent_id);
      if (!sourcePack || !sourceCommand || !sourceConfig || !Number.isFinite(agentId)) {
        throw new Error(t("custom-pack-error", { id: pack.agent_id }));
      }
      const customPack = {
        agent_id: agentId,
        pack_id: String(pack.pack_id || `custom_${agentId}`),
        title: String(pack.title || "YENİ PAKET").slice(0, 72),
        category: String(pack.category || sourcePack.category),
        base_count: 0,
        player_ids: [],
        featured_ids: [],
        custom: true,
        source_agent_id: sourcePack.source_agent_id || sourcePack.agent_id
      };
      catalog.packs.push(customPack);
      catalog.command_template.agent_list.push({
        ...cloneJson(sourceCommand),
        agent_id: agentId,
        title: customPack.title,
        pickup_list: [],
        pickup_cardtype_list: [],
        base_player_count: [0, 0, 0, 0, 0],
        remaining_player_count: [0, 0, 0, 0, 0],
        player_list_total: 0,
        probability: "0"
      });
      catalog.config_template.packs.push({
        ...cloneJson(sourceConfig),
        agent_id: agentId,
        pack_id: customPack.pack_id,
        title: customPack.title,
        base_count: 0,
        player_ids: []
      });
    });
    const defaults = freshState();
    const byId = new Map((parsed.packs || []).map(pack => [Number(pack.agent_id), pack]));
    const playerOriginals = {};
    const livePlayerOverrides = Object.fromEntries(Object.entries(parsed.live_player_overrides || {}).filter(([id, record]) =>
      Array.isArray(catalog.live_players?.[id]) && Array.isArray(record) && record.length === 5 &&
      typeof record[0] === "string" && record[0].trim() &&
      record.slice(1).every(Number.isInteger) &&
      record[1] >= 0 && record[1] <= 255 &&
      record[2] >= 0 && record[2] <= 255 &&
      record[3] >= 0 && record[3] <= 1023 &&
      record[4] >= 0 && record[4] < 13
    ).map(([id, record]) => [id, cloneJson(record)]));
    Object.entries(parsed.custom_records || {}).forEach(([id, record]) => {
      if (!basePlayerIds.has(id) && isValidPlayerId(Number(id)) &&
          record?.gameplayer_id?.id === Number(id) && Array.isArray(record.parameters) && record.level_info) {
        playerOriginals[id] = cloneJson(record);
      }
    });
    Object.entries(parsed.player_originals || {}).forEach(([id, record]) => {
      if (!basePlayerIds.has(id) && isValidPlayerId(Number(id)) &&
          record?.gameplayer_id?.id === Number(id) && Array.isArray(record.parameters) && record.level_info) {
        playerOriginals[id] = cloneJson(record);
      }
    });
    return {
      language: (parsed.language === "en" || parsed.language === "id" || parsed.language === "tr") ? parsed.language : "vi",
      custom_records: Object.fromEntries(Object.entries(parsed.custom_records || {}).filter(([id]) => !basePlayerIds.has(id))),
      player_overrides: Object.fromEntries(Object.entries(parsed.player_overrides || {}).filter(([id]) => basePlayerIds.has(id))),
      player_originals: playerOriginals,
      live_player_overrides: livePlayerOverrides,
      custom_players: Object.fromEntries(Object.entries(parsed.custom_players || {}).filter(([id]) =>
        !basePlayerIds.has(id)
      )),
      packs: defaults.packs.map(pack => {
        const stored = byId.get(pack.agent_id);
        const ids = Array.isArray(stored?.player_ids)
          ? [...new Set(stored.player_ids.map(Number).filter(isValidPlayerId))]
          : [...pack.player_ids];
        if (pack.agent_id === 1348 || pack.agent_id === 1368) {
          if (!ids.includes(88040000002137)) ids.unshift(88040000002137);
          if (!ids.includes(88040000038240)) ids.splice(1, 0, 88040000038240);
        }
        ids.forEach(ensurePlayerRecord);
        return {
          ...pack,
          title: typeof stored?.title === "string" && stored.title !== pack.legacy_title
            ? stored.title.slice(0, 72)
            : pack.title,
          player_ids: ids,
          featured_ids: Array.isArray(stored?.featured_ids)
            ? (JSON.stringify(stored.featured_ids.map(Number)) === JSON.stringify(pack.legacy_featured_ids)
              ? [...pack.featured_ids]
              : [...new Set(stored.featured_ids.map(Number).filter(id => ids.includes(id)))].slice(0, 3))
            : [...pack.featured_ids],
          banner_data: typeof stored?.banner_data === "string" && /^data:image\/(?:png|jpeg|webp);base64,[A-Za-z0-9+/=]+$/.test(stored.banner_data)
            ? stored.banner_data : "",
          custom: Boolean(pack.custom),
          source_agent_id: pack.source_agent_id || pack.agent_id
        };
      })
    };
  } catch (error) {
    showToast(t("invalid-saved-state"));
    return freshState();
  }
}

function persist() {
  if (!state) return;
  const serialized = JSON.stringify(state);
  if (!restoringHistory && serialized !== history[historyIndex]) {
    history = history.slice(0, historyIndex + 1);
    history.push(serialized);
    if (history.length > 80) history.shift();
    historyIndex = history.length - 1;
    renderHistoryButtons();
  }
  Android.saveState(serialized);
}

function renderHistoryButtons() {
  const undo = document.getElementById("undo-button");
  const redo = document.getElementById("redo-button");
  if (undo) undo.disabled = historyIndex <= 0;
  if (redo) redo.disabled = historyIndex < 0 || historyIndex >= history.length - 1;
}

function resetCatalogToBase() {
  catalog.packs = cloneJson(basePacks);
  catalog.command_template = cloneJson(baseCommandTemplate);
  catalog.config_template = cloneJson(baseConfigTemplate);
  catalog.mainmenu_template = cloneJson(baseMainMenuTemplate);
  catalog.records = cloneJson(baseRecords);
  catalog.players = cloneJson(basePlayers);
}

function syncCatalogToState() {
  resetCatalogToBase();
  Object.entries(state.custom_records || {}).forEach(([id, record]) => {
    catalog.records[id] = cloneJson(record);
    catalog.players[id] = { name: `ID ${id}`, position: "—", age: "—" };
  });
  Object.entries(state.player_overrides || {}).forEach(([id, record]) => {
    catalog.records[id] = cloneJson(record);
  });
  Object.entries(state.custom_players || {}).forEach(([id, player]) => {
    catalog.players[id] = cloneJson(player);
  });
  state.packs.forEach(pack => {
    if (!basePackIds.has(pack.agent_id)) {
      const sourceId = pack.source_agent_id || basePacks[0].agent_id;
      const sourcePack = basePacks.find(item => item.agent_id === sourceId) || basePacks[0];
      const sourceCommand = baseCommandTemplate.agent_list.find(item => item.agent_id === sourcePack.agent_id);
      const sourceConfig = baseConfigTemplate.packs.find(item => item.agent_id === sourcePack.agent_id);
      catalog.packs.push({ agent_id: pack.agent_id, pack_id: pack.pack_id || `custom_${pack.agent_id}`, title: pack.title, category: pack.category || sourcePack.category, base_count: 0, player_ids: [...pack.player_ids], featured_ids: [...pack.featured_ids], custom: true, source_agent_id: sourcePack.agent_id });
      catalog.command_template.agent_list.push({ ...cloneJson(sourceCommand), agent_id: pack.agent_id, title: pack.title, pickup_list: [], pickup_cardtype_list: [], base_player_count: [0, 0, 0, 0, 0], remaining_player_count: [0, 0, 0, 0, 0], player_list_total: 0, probability: "0" });
      catalog.config_template.packs.push({ ...cloneJson(sourceConfig), agent_id: pack.agent_id, pack_id: pack.pack_id || `custom_${pack.agent_id}`, title: pack.title, base_count: 0, player_ids: [] });
    }
    pack.player_ids.forEach(ensurePlayerRecord);
  });
}

function currentPack() {
  return state.packs.find(pack => pack.agent_id === selectedAgentId);
}

function originalPack(agentId) {
  return catalog.packs.find(pack => pack.agent_id === agentId);
}

function playerInfo(id) {
  const key = String(id);
  const metadata = { ...(catalog.players[key] || { name: `ID ${id}`, position: "—", age: "—" }) };
  const record = catalog.records[key];
  if (basePlayers[key]) metadata.name = basePlayers[key].name;
  if (Array.isArray(record?.parameters)) {
    metadata.position = POSITION_NAMES[Number(record.parameters[0])] || metadata.position;
    metadata.age = Number.isInteger(Number(record.parameters[46]))
      ? Number(record.parameters[46])
      : metadata.age;
  }
  return metadata;
}

function isValidPlayerId(id) {
  return Number.isSafeInteger(id) && id > 0;
}

function ensurePlayerRecord(id) {
  const key = String(id);
  if (!catalog.records[key]) {
    throw new Error(t("player-record-missing", { id }));
  }
  if (state.player_overrides[key]) catalog.records[key] = cloneJson(state.player_overrides[key]);
}

function safeText(value) {
  return String(value).replace(/[&<>"']/g, character => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[character]);
}

function packIcon() {
  return "✧";
}

function renderStats() {
  document.getElementById("stat-packs").textContent = state.packs.length;
  document.getElementById("list-count").textContent = state.packs.length;
  document.getElementById("stat-players").textContent = Object.keys(catalog.players).length.toLocaleString(state.language);
  document.getElementById("stat-total").textContent = state.packs.reduce((sum, pack) => sum + pack.player_ids.length, 0).toLocaleString(state.language);
}

function renderPackList() {
  const query = document.getElementById("pack-search").value.trim().toLocaleLowerCase(state.language);
  const visible = state.packs.filter(pack => pack.title.toLocaleLowerCase(state.language).includes(query));
  const packageSelect = document.getElementById("mobile-package-select");
  packageSelect.innerHTML = state.packs.map(pack =>
    `<option value="${pack.agent_id}">${safeText(pack.title)}</option>`
  ).join("");
  packageSelect.value = String(selectedAgentId ?? state.packs[0]?.agent_id ?? "");
  packList.innerHTML = visible.map(pack => {
    return `<button class="pack-item${pack.agent_id === selectedAgentId ? " active" : ""}" data-agent="${pack.agent_id}">
      <span class="pack-gem">${packIcon()}</span>
      <span class="pack-item-text"><strong>${safeText(pack.title)}</strong><small>AGENT ID · ${pack.agent_id}</small></span>
      <span class="pack-count">${pack.player_ids.length}</span>
    </button>`;
  }).join("");
  packList.querySelectorAll("[data-agent]").forEach(button => {
    button.addEventListener("click", () => selectPack(Number(button.dataset.agent)));
    const pack = state.packs.find(item => item.agent_id === Number(button.dataset.agent));
    if (pack?.banner_data) {
      button.classList.add("has-banner");
      button.style.backgroundImage = `linear-gradient(90deg, #111315ee, #11131566), url("${pack.banner_data}")`;
    }
  });
}

function filteredPlayerIds() {
  const pack = currentPack();
  if (!pack) return [];
  const query = playerSearch.value.trim().toLocaleLowerCase(state.language);
  const position = document.getElementById("position-filter").value;
  const ageValue = document.getElementById("age-filter").value.trim();
  const age = ageValue === "" ? null : Number(ageValue);
  return pack.player_ids.filter(id => {
    const player = playerInfo(id);
    return (!query || String(id).includes(query) || player.name.toLocaleLowerCase(state.language).includes(query)) &&
      (!position || player.position === position) &&
      (age === null || Number(player.age) === age);
  });
}

function renderPositionFilter() {
  const select = document.getElementById("position-filter");
  const selected = select.value;
  const positions = [...new Set(Object.values(catalog.players).map(player => player.position).filter(position => position && position !== "—"))].sort();
  select.innerHTML = `<option value="">${t("all-positions")}</option>${positions.map(position => `<option value="${safeText(position)}">${safeText(position)}</option>`).join("")}`;
  select.value = positions.includes(selected) ? selected : "";
}

function renderRoster() {
  const pack = currentPack();
  if (!pack) return;
  document.getElementById("roster-count").textContent = `· ${pack.player_ids.length}`;
  const filteredIds = filteredPlayerIds();
  const visibleIds = showAllPlayers ? filteredIds : filteredIds.slice(0, 6);
  document.getElementById("show-roster").textContent = showAllPlayers ? t("show-less") : t("show-all");
  document.getElementById("show-roster").hidden = filteredIds.length <= 6;
  document.getElementById("roster-list").innerHTML = visibleIds.map((id, index) => {
    const player = playerInfo(id);
    const featuredIndex = pack.featured_ids.indexOf(id);
    return `<div class="roster-row">
      <span class="roster-index">${String(index + 1).padStart(2, "0")}</span>
      <div class="roster-info"><strong>${safeText(player.name)}</strong><small>ID ${id} · ${safeText(player.position)} · ${player.age === "—" ? t("unknown-age") : `${player.age} ${t("age-label")}`}</small></div>
      <button class="feature-player${featuredIndex >= 0 ? " selected" : ""}" data-feature="${id}" aria-label="${t("featured-select")}">${featuredIndex >= 0 ? featuredIndex + 1 : "☆"}</button>
      ${featuredIndex > 0 ? `<button class="reorder-player" data-move="${id}" data-direction="-1" aria-label="${t("move-left")}">↑</button>` : ""}
      ${featuredIndex >= 0 && featuredIndex < pack.featured_ids.length - 1 ? `<button class="reorder-player" data-move="${id}" data-direction="1" aria-label="${t("move-right")}">↓</button>` : ""}
      <button class="remove-player" data-remove="${id}" aria-label="${t("remove")}">−</button>
    </div>`;
  }).join("");
  document.getElementById("roster-list").querySelectorAll("[data-remove]").forEach(button => {
    button.addEventListener("click", () => removePlayer(Number(button.dataset.remove)));
  });
  document.getElementById("roster-list").querySelectorAll("[data-feature]").forEach(button => {
    button.addEventListener("click", () => toggleFeatured(Number(button.dataset.feature)));
  });
  document.getElementById("roster-list").querySelectorAll("[data-move]").forEach(button => {
    button.addEventListener("click", () => moveFeatured(Number(button.dataset.move), Number(button.dataset.direction)));
  });
  renderPositionFilter();
    renderFeaturedPicker();
}

function renderFeatured() {
  const pack = currentPack();
  if (!pack) return;
  const featured = pack.featured_ids.filter(id => pack.player_ids.includes(id)).slice(0, 3);
  document.getElementById("featured-list").innerHTML = featured.length
    ? featured.map((id, index) => {
      const player = playerInfo(id);
      return `<article class="player-card"><div class="player-card-top"><span>0${index + 1} · ${t("featured-number")}</span><b>${safeText(player.position)}</b></div><strong>${safeText(player.name)}</strong><small>ID ${id}${player.age === "—" ? "" : ` · ${player.age} ${t("age-label").toLocaleUpperCase(state.language)}`}</small><div class="featured-order"><button data-feature-move="${id}" data-direction="-1" ${index === 0 ? "disabled" : ""}>↑</button><button data-feature-move="${id}" data-direction="1" ${index === featured.length - 1 ? "disabled" : ""}>↓</button></div></article>`;
    }).join("")
    : `<article class="player-card"><div class="player-card-top"><span>${t("featured-number")}</span><b>—</b></div><strong>${t("no-featured")}</strong><small>${t("add-player-hint")}</small></article>`;
  document.getElementById("featured-list").querySelectorAll("[data-feature-move]").forEach(button => {
    button.addEventListener("click", () => moveFeatured(Number(button.dataset.featureMove), Number(button.dataset.direction)));
  });
}

function renderFeaturedPicker() {
  const pack = currentPack();
  const picker = document.getElementById("featured-picker");
  if (!pack || picker.hidden) return;
  picker.innerHTML = pack.player_ids.map(id => {
    const player = playerInfo(id);
    const checked = pack.featured_ids.includes(id) ? " checked" : "";
    return `<label class="featured-option"><input type="checkbox" data-feature-choice="${id}"${checked}><span>${safeText(player.name)} <small>ID ${id}</small></span></label>`;
  }).join("");
  picker.querySelectorAll("[data-feature-choice]").forEach(input => {
    input.addEventListener("change", () => {
      const id = Number(input.dataset.featureChoice);
      if (input.checked && pack.featured_ids.length >= 3) {
        input.checked = false;
        showToast(t("featured-limit"));
        return;
      }
      pack.featured_ids = input.checked
        ? [...pack.featured_ids, id]
        : pack.featured_ids.filter(featuredId => featuredId !== id);
      persist();
      renderFeatured();
      renderRoster();
      renderFeaturedPicker();
    });
  });
}

function toggleFeatured(id) {
  const pack = currentPack();
  const index = pack.featured_ids.indexOf(id);
  if (index >= 0) {
    pack.featured_ids.splice(index, 1);
  } else if (pack.featured_ids.length < 3 && pack.player_ids.includes(id)) {
    pack.featured_ids.push(id);
  } else {
    showToast(t("featured-limit"));
    return;
  }
  persist();
  renderFeatured();
  renderRoster();
}

function moveFeatured(id, direction) {
  const pack = currentPack();
  const index = pack.featured_ids.indexOf(id);
  const target = index + direction;
  if (index < 0 || target < 0 || target >= pack.featured_ids.length) return;
  [pack.featured_ids[index], pack.featured_ids[target]] = [pack.featured_ids[target], pack.featured_ids[index]];
  persist();
  renderFeatured();
  renderRoster();
}

const POSITION_NAMES = ["GK", "CB", "LB", "RB", "DMF", "CMF", "LMF", "RMF", "AMF", "LWF", "RWF", "SS", "CF"];
const playerStats = [
  ["offensive-awareness", 14], ["ball-control", 18], ["dribbling", 17],
  ["tight-possession", 19], ["low-pass", 21], ["lofted-pass", 22],
  ["finishing", 20], ["heading", 23], ["set-piece-taking", 32], ["curl", 25],
  ["speed", 27], ["acceleration", 31], ["kicking-power", 34], ["jumping", 33],
  ["physical-contact", 28], ["balance", 29], ["stamina", 30],
  ["defensive-awareness", 15], ["defensive-engagement", 26],
  ["ball-winning", 24], ["aggression", 35],
  ["gk-awareness", 36], ["gk-catching", 37], ["gk-clearing", 38],
  ["gk-reflexes", 39], ["gk-reach", 16]
];

function closeModal() {
  document.getElementById("modal-backdrop").hidden = true;
  document.getElementById("modal-content").innerHTML = "";
  document.getElementById("modal-actions").innerHTML = "";
}

function selectPack(agentId) {
  selectedAgentId = agentId;
  showAllPlayers = false;
  document.getElementById("player-search").value = "";
  document.getElementById("player-results").hidden = true;
  emptyState.hidden = true;
  editorContent.hidden = false;
  const pack = currentPack();
  document.getElementById("editor-kicker").textContent = t("package-kind");
  document.getElementById("pack-agent").textContent = `AGENT ID ${pack.agent_id}`;
  document.getElementById("pack-category").textContent = t("package-kind");
  document.getElementById("pack-title").value = pack.title;
  updateBannerPreview();
  renderPackList();
  renderFeatured();
  renderRoster();
  document.getElementById("stat-total").textContent = state.packs.reduce((sum, item) => sum + item.player_ids.length, 0).toLocaleString(state.language);
}

function updateBannerPreview() {
  const pack = currentPack();
  if (!pack) return;
  const preview = document.getElementById("pack-banner-preview");
  document.getElementById("banner-preview-title").textContent = pack.title || t("package-kind");
  document.getElementById("banner-preview-id").textContent = `AGENT ID ${pack.agent_id}`;
  preview.style.backgroundImage = pack.banner_data
    ? `linear-gradient(90deg, #111315dd, #11131530), url("${pack.banner_data}")`
    : "linear-gradient(115deg, #263a31, #20272a 55%, #302b25)";
  preview.classList.toggle("has-image", Boolean(pack.banner_data));
}

window.handleImportedBanner = rawData => {
  const data = String(rawData || "");
  if (!/^data:image\/(?:png|jpeg|webp);base64,[A-Za-z0-9+/=]+$/.test(data)) {
    showToast(t("banner-error", { message: "Unsupported image format." }));
    return;
  }
  if (data.length > 11_200_000) {
    showToast(t("banner-too-large"));
    return;
  }
  const pack = currentPack();
  if (!pack) return;
  pack.banner_data = data;
  persist();
  updateBannerPreview();
  renderPackList();
  showToast(t("banner-loaded"));
};

function searchPlayers(query) {
  const results = document.getElementById("player-results");
  const normalized = query.trim().toLocaleLowerCase(state.language);
  if (!normalized && !document.getElementById("position-filter").value && !document.getElementById("age-filter").value) {
    results.hidden = true;
    results.innerHTML = "";
    return;
  }
  const pack = currentPack();
  const position = document.getElementById("position-filter").value;
  const ageValue = document.getElementById("age-filter").value.trim();
  const age = ageValue === "" ? null : Number(ageValue);
  const matches = Object.keys(catalog.players)
    .filter(id => !livePlayerIds.has(id))
    .map(id => [id, playerInfo(Number(id))])
    .filter(([id, player]) =>
      (!normalized || id.includes(normalized) || player.name.toLocaleLowerCase(state.language).includes(normalized)) &&
      (!position || player.position === position) &&
      (age === null || Number(player.age) === age))
    .slice(0, 30);
  results.hidden = false;
  results.innerHTML = matches.length ? matches.map(([id, player]) => {
    const alreadyAdded = pack.player_ids.includes(Number(id));
    return `<div class="result-row"><div class="result-info"><strong>${safeText(player.name)}</strong><small>ID ${id} · ${safeText(player.position)} · ${player.age === "—" ? t("unknown-age") : `${player.age} ${t("age-label")}`}</small></div>${alreadyAdded ? `<button class="remove-player" data-remove="${id}" aria-label="${t("remove")}">−</button>` : `<button class="add-player" data-add="${id}" aria-label="${t("add")}">+</button>`}</div>`;
  }).join("") : `<div class="result-row"><div class="result-info"><strong>${t("no-results")}</strong><small>${t("no-results-hint")}</small></div></div>`;
  results.querySelectorAll("[data-add]").forEach(button => {
    button.addEventListener("click", () => addPlayer(Number(button.dataset.add)));
  });
  results.querySelectorAll("[data-remove]").forEach(button => {
    button.addEventListener("click", () => removePlayer(Number(button.dataset.remove)));
  });
}

function addPlayer(id) {
  const pack = currentPack();
  if (!pack || !isValidPlayerId(id)) return false;
  if (pack.player_ids.includes(id)) {
    showToast(t("already-in-package"));
    return false;
  }
  if (!catalog.records[String(id)]) {
    showToast(t("player-record-missing", { id }));
    return false;
  }
  ensurePlayerRecord(id);
  pack.player_ids.push(id);
  if (pack.featured_ids.length < 3) pack.featured_ids.push(id);
  persist();
  renderPackList();
  renderFeatured();
  renderRoster();
  renderStats();
  searchPlayers(playerSearch.value);
  showToast(t("added-player", { name: playerInfo(id).name }));
  return true;
}

function removePlayer(id) {
  const pack = currentPack();
  if (!pack) return;
  pack.player_ids = pack.player_ids.filter(playerId => playerId !== id);
  pack.featured_ids = pack.featured_ids.filter(playerId => playerId !== id);
  pack.player_ids.forEach(playerId => {
    if (pack.featured_ids.length < 3 && !pack.featured_ids.includes(playerId)) pack.featured_ids.push(playerId);
  });
  persist();
  renderPackList();
  renderFeatured();
  renderRoster();
  renderStats();
  searchPlayers(playerSearch.value);
  showToast(t("removed-player", { name: playerInfo(id).name }));
}

function cloneJson(value) {
  return JSON.parse(JSON.stringify(value));
}

function backupDocument() {
  return {
    app: "offline-pack-manager",
    version: 1,
    state: cloneJson(state)
  };
}

function validatePackages() {
  const errors = [];
  const agentIds = new Set();
  if (!Array.isArray(state.packs) || state.packs.length === 0) return [t("minimum-pack")];
  state.packs.forEach(pack => {
    if (!Number.isSafeInteger(pack.agent_id) || pack.agent_id <= 0 || agentIds.has(pack.agent_id)) {
      errors.push(`${t("invalid-package-id")} (${pack.title || pack.agent_id})`);
    }
    agentIds.add(pack.agent_id);
    if (typeof pack.title !== "string" || !pack.title.trim()) errors.push(`${t("invalid-package-title")} (${pack.agent_id})`);
    if (!Array.isArray(pack.player_ids) || pack.player_ids.some(id => !isValidPlayerId(id)) ||
        new Set(pack.player_ids).size !== pack.player_ids.length ||
        pack.player_ids.some(id => !catalog.records[String(id)])) {
      errors.push(`${t("invalid-package-id")} (${pack.title || pack.agent_id})`);
    }
    const featured = pack.featured_ids;
    if (!Array.isArray(featured) || featured.length > 3 || new Set(featured).size !== featured.length ||
        featured.some(id => !pack.player_ids.includes(id))) {
      errors.push(`${t("invalid-featured")} (${pack.title || pack.agent_id})`);
    }
    if (pack.banner_data && (!/^data:image\/(?:png|jpeg|webp);base64,[A-Za-z0-9+/=]+$/.test(pack.banner_data) ||
        pack.banner_data.length > 11_200_000)) errors.push(`${t("invalid-banner")} (${pack.title || pack.agent_id})`);
  });
  return [...new Set(errors)];
}

function isValidBackupState(savedState) {
  if (!savedState || !Array.isArray(savedState.packs) || savedState.packs.length === 0) return false;
  const agentIds = new Set();
  return savedState.packs.every(pack => {
    if (!Number.isSafeInteger(pack.agent_id) || pack.agent_id <= 0 || agentIds.has(pack.agent_id) ||
        typeof pack.title !== "string" || !Array.isArray(pack.player_ids) || !Array.isArray(pack.featured_ids)) return false;
    agentIds.add(pack.agent_id);
    return pack.player_ids.every(isValidPlayerId) &&
      new Set(pack.player_ids).size === pack.player_ids.length &&
      pack.featured_ids.length <= 3 &&
      pack.featured_ids.every(id => isValidPlayerId(id) && pack.player_ids.includes(id)) &&
      new Set(pack.featured_ids).size === pack.featured_ids.length;
  });
}

function showInfoModal(title, lines) {
  document.getElementById("modal-title").textContent = title;
  document.getElementById("modal-content").innerHTML = lines.length
    ? lines.map(line => `<p class="modal-line">${line}</p>`).join("")
    : `<p class="modal-line">${safeText(t("no-changes"))}</p>`;
  document.getElementById("modal-actions").innerHTML = `<button class="button button-secondary" id="modal-close-action">${safeText(t("cancel"))}</button>`;
  document.getElementById("modal-backdrop").hidden = false;
  document.getElementById("modal-close-action").addEventListener("click", closeModal);
}

function showValidation() {
  const errors = validatePackages();
  showInfoModal(t("validate-title"), errors.length
    ? [safeText(t("validation-errors")), ...errors.map(safeText)]
    : [safeText(t("validation-ok"))]);
  return errors.length === 0;
}

function showChangeSummary() {
  const currentById = new Map(state.packs.map(pack => [pack.agent_id, pack]));
  const originalById = new Map(basePacks.map(pack => [pack.agent_id, pack]));
  const lines = [];
  state.packs.forEach(pack => {
    const original = originalById.get(pack.agent_id);
    if (!original) {
      lines.push(`<strong>${safeText(t("added-pack"))}:</strong> ${safeText(pack.title)} · ${pack.player_ids.length}`);
      return;
    }
    const changed = [];
    if (pack.title !== original.title) changed.push(safeText(t("title-changed")));
    const added = pack.player_ids.filter(id => !original.player_ids.includes(id));
    const removed = original.player_ids.filter(id => !pack.player_ids.includes(id));
    if (added.length) changed.push(`${safeText(t("added-player-label"))}: ${added.join(", ")}`);
    if (removed.length) changed.push(`${safeText(t("removed-player-label"))}: ${removed.join(", ")}`);
    if (changed.length) lines.push(`<strong>${safeText(t("changed-pack"))}:</strong> ${safeText(pack.title)} · ${changed.join(" · ")}`);
  });
  basePacks.forEach(pack => {
    if (!currentById.has(pack.agent_id)) lines.push(`<strong>${safeText(t("removed-pack"))}:</strong> ${safeText(pack.title)}`);
  });
  showInfoModal(t("changes-title"), lines);
}

function restoreHistory(index) {
  if (index < 0 || index >= history.length) return;
  historyIndex = index;
  restoringHistory = true;
  state = JSON.parse(history[historyIndex]);
  syncCatalogToState();
  if (!state.packs.some(pack => pack.agent_id === selectedAgentId)) selectedAgentId = state.packs[0]?.agent_id ?? null;
  Android.saveState(JSON.stringify(state));
  restoringHistory = false;
  applyLanguage();
  renderStats();
  if (selectedAgentId !== null) selectPack(selectedAgentId);
  renderHistoryButtons();
}

function nextCustomAgentId() {
  return Math.max(9000, ...state.packs.map(pack => Number(pack.agent_id) || 0)) + 1;
}

function appendCustomPack(sourcePack, title, playerIds = [], featuredIds = []) {
  const nextId = nextCustomAgentId();
  const sourceAgentId = sourcePack.source_agent_id || sourcePack.agent_id;
  const customPack = {
    agent_id: nextId, title: title.slice(0, 72), category: sourcePack.category,
    pack_id: `custom_${nextId}`, custom: true, source_agent_id: sourceAgentId,
    player_ids: [...new Set(playerIds)], featured_ids: [...new Set(featuredIds)].filter(id => playerIds.includes(id)).slice(0, 3),
    banner_data: sourcePack.banner_data || ""
  };
  state.packs.push(customPack);
  customPack.player_ids.forEach(ensurePlayerRecord);
  syncCatalogToState();
  return customPack;
}

function importConfig(config, command, importedFiles) {
  const importedPacks = Array.isArray(config?.packs) ? config.packs : [];
  if (!importedPacks.length) throw new Error(t("no-import-packs"));
  const commandById = new Map((command?.agent_list || []).map(agent => [Number(agent.agent_id), agent]));
  const importedState = cloneJson(state);
  const currentById = new Map(importedState.packs.map(pack => [pack.agent_id, pack]));
  const seenAgentIds = new Set();
  importedPacks.forEach(item => {
    const agentId = Number(item.agent_id);
    if (!isValidPlayerId(agentId) || !Array.isArray(item.player_ids)) throw new Error(t("invalid-package-id"));
    if (seenAgentIds.has(agentId)) throw new Error(t("invalid-package-id"));
    seenAgentIds.add(agentId);
    const playerIds = item.player_ids.map(Number);
    if (playerIds.some(id => !isValidPlayerId(id)) || new Set(playerIds).size !== playerIds.length) {
      throw new Error(t("invalid-package-id"));
    }
    const existing = currentById.get(agentId);
    const base = basePacks.find(pack => pack.agent_id === agentId);
    const source = base || basePacks[0];
    const commandItem = commandById.get(agentId);
    const bannerPath = commandItem?.banner_file_c_name || commandItem?.banner_file_d_name;
    const bannerData = bannerPath
      ? importedFiles[`/${bannerPath.replace(/^\/+/, "")}`] || importedFiles[bannerPath.replace(/^\/+/, "")]
      : "";
    const featuredFromCommand = (commandItem?.pickup_list || [])
      .map(player => Number(player?.gameplayer_id?.id))
      .filter(id => playerIds.includes(id)).slice(0, 3);
    const featuredIds = featuredFromCommand.length
      ? featuredFromCommand
      : (existing?.featured_ids || playerIds.slice(0, 3)).filter(id => playerIds.includes(id)).slice(0, 3);
    const updated = {
      ...(existing || {}),
      agent_id: agentId,
      title: String(item.title || commandItem?.title || `${state.language === "vi" ? "Gói Thẻ" : (state.language === "en" ? "Package" : "Paket")} ${agentId}`).slice(0, 72),
      player_ids: playerIds,
      featured_ids: featuredIds,
      banner_data: bannerData || existing?.banner_data || "",
      category: base?.category || source.category,
      pack_id: String(item.pack_id || base?.pack_id || `custom_${agentId}`),
      custom: !base,
      source_agent_id: base?.agent_id || source.agent_id
    };
    if (existing) Object.assign(existing, updated);
    else importedState.packs.push(updated);
  });
  if (importedState.packs.length === state.packs.length && !importedPacks.some(item => currentById.has(Number(item.agent_id)))) {
    throw new Error(t("no-import-packs"));
  }
  resetCatalogToBase();
  state = loadState(JSON.stringify(importedState));
  syncCatalogToState();
  state.packs.forEach(pack => pack.player_ids.forEach(ensurePlayerRecord));
  persist();
  return importedPacks.length;
}

function importPlayerRecords(importedRecords) {
  if (!importedRecords || typeof importedRecords !== "object" || Array.isArray(importedRecords)) return;
  Object.entries(importedRecords).forEach(([key, value]) => {
    const id = Number(key);
    const record = value?.gameplayer || value;
    if (!isValidPlayerId(id) || record?.gameplayer_id?.id !== id ||
        !Array.isArray(record.parameters) || !record.level_info) {
      throw new Error(t("invalid-player-record", { id: key }));
    }
    catalog.records[key] = cloneJson(record);
    if (basePlayerIds.has(key)) {
      state.player_overrides[key] = cloneJson(record);
      delete state.player_originals[key];
    } else {
      state.custom_records[key] = cloneJson(record);
      state.player_originals[key] = cloneJson(record);
      delete state.player_overrides[key];
    }
  });
  persist();
}

window.handleImportedFile = rawPayload => {
  const previousState = cloneJson(state);
  const previousSelection = selectedAgentId;
  try {
    const payload = JSON.parse(rawPayload);
    let backup = null;
    let config = null;
    let command = null;
    let playerRecords = null;
    const jsonFiles = payload.files || {};
    for (const [filename, text] of Object.entries(jsonFiles)) {
      const name = filename.split("/").pop().toLowerCase();
      let parsed;
      try {
        parsed = JSON.parse(text);
      } catch {
        continue;
      }
      if (parsed.app === "offline-pack-manager" && parsed.state) backup = parsed;
      if (name === "z_iru_packs_config.json" || name === "ziru_packs_config.json" || Array.isArray(parsed.packs)) config = parsed;
      if (Array.isArray(parsed.agent_list)) command = parsed;
      if (name === "ziru_player_records.json") playerRecords = parsed;
    }
    if (payload.json) {
      const parsed = JSON.parse(payload.json);
      if (parsed.app === "offline-pack-manager" && parsed.state) backup = parsed;
      else if (Array.isArray(parsed.packs)) config = parsed;
      else if (Array.isArray(parsed.agent_list)) command = parsed;
      else if (parsed && typeof parsed === "object") playerRecords = parsed;
    }
    if (backup) {
      if (backup.version !== 1 || !isValidBackupState(backup.state)) throw new Error(t("no-import-packs"));
      resetCatalogToBase();
      state = loadState(JSON.stringify(backup.state));
      syncCatalogToState();
      const errors = validatePackages();
      if (errors.length) throw new Error(errors.join("; "));
      selectedAgentId = state.packs[0]?.agent_id ?? null;
      persist();
      renderStats();
      if (selectedAgentId !== null) selectPack(selectedAgentId);
      showToast(t("import-backup"));
      return;
    }
    if (!config) throw new Error(t("no-import-packs"));
    importPlayerRecords(playerRecords);
    const count = importConfig(config, command, jsonFiles);
    renderStats();
    if (selectedAgentId === null || !state.packs.some(pack => pack.agent_id === selectedAgentId)) selectedAgentId = state.packs[0].agent_id;
    selectPack(selectedAgentId);
    showToast(t("import-packages", { count }));
  } catch (error) {
    resetCatalogToBase();
    state = loadState(JSON.stringify(previousState));
    syncCatalogToState();
    selectedAgentId = state.packs.some(pack => pack.agent_id === previousSelection)
      ? previousSelection : state.packs[0]?.agent_id ?? null;
    Android.saveState(JSON.stringify(state));
    renderStats();
    if (selectedAgentId !== null) selectPack(selectedAgentId);
    showToast(t("import-error", { message: error.message }));
  }
};

function exportFiles() {
  const errors = validatePackages();
  if (errors.length) throw new Error(errors.join("; "));
  const agentList = cloneJson(catalog.command_template);
  const config = cloneJson(catalog.config_template);
  const mainMenu = cloneJson(catalog.mainmenu_template);
  const agentById = new Map(agentList.agent_list.map(agent => [agent.agent_id, agent]));
  const configById = new Map(config.packs.map(pack => [pack.agent_id, pack]));
  const templateById = new Map(catalog.command_template.agent_list.map(agent => [agent.agent_id, agent]));
  const previewLines = [];
  const filesToExport = {};
  agentList.agent_list = state.packs.map(pack => {
    const existing = agentById.get(pack.agent_id);
    const source = existing || templateById.get(pack.source_agent_id);
    if (!source) throw new Error(t("template-error", { id: pack.agent_id }));
    return existing || { ...cloneJson(source), agent_id: pack.agent_id, title: pack.title };
  });
  ["agent_price_list", "agent_bonus_list"].forEach(key => {
    const templates = catalog.command_template[key];
    if (!Array.isArray(templates) || templates.length === 0) return;
    const byId = new Map(templates.map(item => [item.agent_id, item]));
    agentList[key] = state.packs.map(pack => {
      const source = byId.get(pack.agent_id) || byId.get(pack.source_agent_id);
      if (!source) throw new Error(t("list-error", { key, id: pack.agent_id }));
      return pack.agent_id === source.agent_id ? cloneJson(source) : { ...cloneJson(source), agent_id: pack.agent_id };
    });
  });

  state.packs.forEach(pack => {
    const commandPack = agentById.get(pack.agent_id);
    const configPack = configById.get(pack.agent_id);
    if (!commandPack || !configPack) throw new Error(t("template-error", { id: pack.agent_id }));
    const ids = [...new Set(pack.player_ids.map(Number))];
    if (ids.some(id => !catalog.records[String(id)])) throw new Error(t("invalid-player-id", { id: pack.agent_id }));
    commandPack.title = pack.title.trim();
    if (pack.banner_data) {
      const extension = pack.banner_data.startsWith("data:image/png;") ? "png"
        : pack.banner_data.startsWith("data:image/webp;") ? "webp" : "jpg";
      const imagePath = `/banner/agent/${String(pack.agent_id).padStart(6, "0")}/offline_pack_banner.${extension}`;
      commandPack.banner_file_c_name = imagePath;
      commandPack.banner_file_d_name = imagePath;
      const imageKey = imagePath.slice(1);
      filesToExport[imageKey] = pack.banner_data;
    }
    const featuredIds = pack.featured_ids.filter(id => ids.includes(id)).slice(0, 3);
    const featuredPlayers = featuredIds.map(id => {
      const source = catalog.records[String(id)];
      const record = cloneJson(source.gameplayer || source);
      record.gameplayer_id.id = id;
      record.gameplayer_id.serial = 0;
      return record;
    });
    commandPack.pickup_list = cloneJson(featuredPlayers);
    commandPack.banner_a_pickup_list = cloneJson(featuredPlayers);
    commandPack.banner_c_pickup_list = cloneJson(featuredPlayers);
    commandPack.pickup_cardtype_list = featuredIds.map(id => ({ id, serial: 0 }));
    commandPack.base_player_count = [0, 0, ids.length, 0, 0];
    commandPack.remaining_player_count = [0, 0, ids.length, 0, 0];
    commandPack.player_list_total = ids.length;
    commandPack.probability = String(ids.length);
    configPack.title = commandPack.title;
    configPack.base_count = ids.length;
    configPack.player_ids = ids;
    previewLines.push(`${pack.agent_id} | ${commandPack.title} | ${ids.length} players`);
    ids.forEach(id => {
      const player = playerInfo(id);
      previewLines.push(`  ${id} | ${player.name} | ${player.position} | age ${player.age}`);
    });
    previewLines.push("");
  });

  const commandJson = `${JSON.stringify(agentList, null, 2)}\n`;
  const playerRecords = Object.fromEntries(Object.entries(catalog.records).map(([id, record]) => [
    id,
    record.gameplayer
      ? cloneJson(record)
      : { gameplayer: cloneJson(record), is_dummy: "NO" }
  ]));
  {
    const bannerTemplates = new Map((catalog.mainmenu_template.banner_list || []).filter(b => b.announcement_type === "AGENT").map(b => [b.entity_id, b]));
    const fallbackBanner = [...bannerTemplates.values()][0];
    const packById = new Map(state.packs.map(pack => [pack.agent_id, pack]));
    const bannerPath = pack => {
      if (!pack.banner_data) return null;
      const ext = pack.banner_data.startsWith("data:image/png;") ? "png" : pack.banner_data.startsWith("data:image/webp;") ? "webp" : "jpg";
      return `/banner/agent/${String(pack.agent_id).padStart(6, "0")}/offline_pack_banner.${ext}`;
    };
    const kept = (catalog.mainmenu_template.banner_list || [])
      .filter(b => b.announcement_type !== "AGENT" || packById.has(b.entity_id))
      .map(b => {
        const copy = cloneJson(b);
        const pack = packById.get(b.entity_id);
        if (copy.announcement_type === "AGENT" && pack) {
          copy.title_text = pack.title;
          if (bannerPath(pack)) copy.banner_file_name = bannerPath(pack);
        }
        return copy;
      });
    const shown = new Set(kept.filter(b => b.announcement_type === "AGENT").map(b => b.entity_id));
    state.packs.filter(pack => pack.custom && !shown.has(pack.agent_id)).forEach(pack => {
      const base = bannerTemplates.get(pack.source_agent_id) || fallbackBanner;
      if (!base) return;
      const copy = cloneJson(base);
      copy.entity_id = pack.agent_id;
      copy.title_text = pack.title;
      if (bannerPath(pack)) copy.banner_file_name = bannerPath(pack);
      kept.push(copy);
    });
    mainMenu.banner_list = kept;
    mainMenu.unread_information_count = Math.max(kept.length, 1);
  }
  mainMenu.agent_list = cloneJson(agentList.agent_list);
  mainMenu.agent_price_list = cloneJson(agentList.agent_price_list);
  mainMenu.agent_bonus_list = cloneJson(agentList.agent_bonus_list);
  return {
    "CmdGetMyclubAgentlist.json": commandJson,
    "get_myclub_agentlist.json": commandJson,
    "get_myclub_mainmenu_info.json": `${JSON.stringify(mainMenu, null, 2)}\n`,
    "ZIRU_PACKS_CONFIG.json": `${JSON.stringify(config, null, 2)}\n`,
    "ZIRU_PLAYER_RECORDS.json": `${JSON.stringify(playerRecords, null, 2)}\n`,
    "ZIRU_PACK_PLAYER_PREVIEW_V3.txt": previewLines.join("\n"),
    "offline-pack-manager-backup.json": `${JSON.stringify(backupDocument(), null, 2)}\n`,
    ...exampleFiles(),
    ...filesToExport
  };
}

function exampleFiles() {
  const note = "ORNEK / EXAMPLE ONLY - this file is NOT loaded by the game. Copy values by hand.";
  const mainMenuBanner = {
    _note: note,
    banner_list: [{
      announcement_type: "AGENT", entity_id: 9999, title_text: "MY PACK",
      banner_file_name: "/banner/agent/009999/offline_pack_banner.png",
      description_text: "Example description", note_text: "Example note",
      is_popup: "NO", start_time: 1700000000, end_time: 2000000000, reward: []
    }]
  };
  const agentEntry = {
    _note: note,
    agent_list: [{
      agent_id: 9999, title: "MY PACK", banner_file_c_name: "/banner/agent/009999/offline_pack_banner.png",
      banner_file_d_name: "/banner/agent/009999/offline_pack_banner.png",
      base_player_count: [0, 0, 3, 0, 0], remaining_player_count: [0, 0, 3, 0, 0],
      pickup_cardtype_list: [{ id: 88029649706260, serial: 0 }]
    }]
  };
  const lookup = {
    _note: note,
    player_id: 88029649706260,
    main_position_index: 9, foot_index_43: 0, level_info: { level: 1, level_limit: 34 },
    initial_base_team_id: 108, initial_base_national_team_id: 7
  };
  const readme = [
    note, "",
    "Server: OfflineServer http://127.0.0.1:8085 (inside the game).",
    "Real files (put into assets/responses): CmdGetMyclubAgentlist.json, get_myclub_agentlist.json, get_myclub_mainmenu_info.json, ZIRU_PACKS_CONFIG.json.",
    "get_myclub_mainmenu_info.json banner_list is rebuilt from the package list: delete a package -> its banner disappears; add a package -> a banner is added.",
    "Banner images: /banner/agent/<agent_id padded to 6>/offline_pack_banner.<ext>", ""
  ].join("\n");
  return {
    "examples/EXAMPLE_mainmenu_banner.json": `${JSON.stringify(mainMenuBanner, null, 2)}\n`,
    "examples/EXAMPLE_agent_pack_entry.json": `${JSON.stringify(agentEntry, null, 2)}\n`,
    "examples/EXAMPLE_player_fields.json": `${JSON.stringify(lookup, null, 2)}\n`,
    "examples/README_EXAMPLES.txt": readme
  };
}

function showToast(message) {
  const toast = document.getElementById("toast");
  toast.textContent = message;
  toast.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove("show"), 2600);
}

document.getElementById("language-toggle").addEventListener("click", () => {
  if (state.language === "vi") state.language = "en"; else if (state.language === "en") state.language = "id"; else state.language = "vi";
  persist();
  applyLanguage();
  renderStats();
  if (selectedAgentId !== null) selectPack(selectedAgentId);
});

document.getElementById("pack-title").addEventListener("input", event => {
  const pack = currentPack();
  if (!pack) return;
  pack.title = event.target.value.slice(0, 72);
  persist();
  renderPackList();
  updateBannerPreview();
});
document.getElementById("select-banner").addEventListener("click", () => {
  Android.selectBanner(t("banner-error", { message: "" }));
});
document.getElementById("clear-banner").addEventListener("click", () => {
  const pack = currentPack();
  if (!pack?.banner_data) return;
  pack.banner_data = "";
  persist();
  updateBannerPreview();
  renderPackList();
  showToast(t("banner-removed"));
});
document.getElementById("pack-search").addEventListener("input", renderPackList);
document.getElementById("mobile-package-select").addEventListener("change", event => {
  selectPack(Number(event.target.value));
});
playerSearch.addEventListener("input", () => {
  document.getElementById("clear-player-search").hidden = !playerSearch.value;
  renderRoster();
  searchPlayers(playerSearch.value);
});
document.getElementById("add-player-id").addEventListener("click", () => {
  const input = document.getElementById("manual-player-id");
  const value = input.value.trim();
  if (!/^\d{1,16}$/.test(value) || !isValidPlayerId(Number(value))) {
    showToast(t("invalid-player-id-input"));
    return;
  }
  if (addPlayer(Number(value))) {
    input.value = "";
    input.focus();
  }
});
document.getElementById("manual-player-id").addEventListener("keydown", event => {
  if (event.key === "Enter") {
    event.preventDefault();
    document.getElementById("add-player-id").click();
  }
});
document.getElementById("clear-player-search").addEventListener("click", () => {
  playerSearch.value = "";
  document.getElementById("clear-player-search").hidden = true;
  renderRoster();
  searchPlayers("");
  playerSearch.focus();
});
document.getElementById("age-filter").addEventListener("input", () => {
  renderRoster();
  searchPlayers(playerSearch.value);
});
document.getElementById("position-filter").addEventListener("change", () => {
  renderRoster();
  searchPlayers(playerSearch.value);
});
document.getElementById("toggle-bulk-add").addEventListener("click", () => {
  const panel = document.getElementById("bulk-add-panel");
  panel.hidden = !panel.hidden;
  if (!panel.hidden) document.getElementById("bulk-player-ids").focus();
});
document.getElementById("add-bulk-ids").addEventListener("click", () => {
  const input = document.getElementById("bulk-player-ids");
  const ids = [...new Set(input.value.split(/[,\s;]+/).map(value => Number(value.trim())).filter(isValidPlayerId))];
  let added = 0;
  ids.forEach(id => { if (addPlayer(id)) added += 1; });
  showToast(t("bulk-result", { added, skipped: input.value.trim() ? Math.max(0, input.value.split(/[,\s;]+/).filter(Boolean).length - added) : 0 }));
  input.value = "";
});
document.getElementById("toggle-featured-picker").addEventListener("click", event => {
  const picker = document.getElementById("featured-picker");
  picker.hidden = !picker.hidden;
  event.currentTarget.setAttribute("aria-expanded", String(!picker.hidden));
  renderFeaturedPicker();
});
document.getElementById("undo-button").addEventListener("click", () => restoreHistory(historyIndex - 1));
document.getElementById("redo-button").addEventListener("click", () => restoreHistory(historyIndex + 1));
document.getElementById("duplicate-pack").addEventListener("click", () => {
  const source = currentPack();
  if (!source) return;
  const copy = appendCustomPack(source, t("duplicate-title", { title: source.title }), source.player_ids, source.featured_ids);
  persist();
  selectedAgentId = copy.agent_id;
  renderStats();
  selectPack(copy.agent_id);
  showToast(t("package-copy"));
});
document.getElementById("backup-export").addEventListener("click", () => {
  const content = JSON.stringify(backupDocument(), null, 2);
  Android.exportBackup(content, "offline-pack-manager-backup.json", t("backup-saved"), t("backup-error", { message: "" }));
});
document.getElementById("backup-import").addEventListener("click", () => {
  Android.importFile(t("import-error", { message: "" }));
});
document.getElementById("change-summary").addEventListener("click", showChangeSummary);
document.getElementById("validate-button").addEventListener("click", showValidation);
document.getElementById("modal-close").addEventListener("click", closeModal);
document.getElementById("modal-backdrop").addEventListener("click", event => {
  if (event.target.id === "modal-backdrop") closeModal();
});
document.addEventListener("contextmenu", event => event.preventDefault());
document.addEventListener("copy", event => event.preventDefault());
document.addEventListener("cut", event => event.preventDefault());
document.addEventListener("dragstart", event => event.preventDefault());
document.addEventListener("selectstart", event => {
  if (!event.target.matches("input, textarea")) event.preventDefault();
});
document.addEventListener("keydown", event => {
  if ((event.ctrlKey || event.metaKey) && ["a", "c", "x"].includes(event.key.toLowerCase())) {
    event.preventDefault();
  }
});
document.getElementById("show-roster").addEventListener("click", () => {
  showAllPlayers = !showAllPlayers;
  renderRoster();
});
document.getElementById("new-pack").addEventListener("click", () => {
  const source = originalPack(selectedAgentId) || catalog.packs[0];
  const customPack = appendCustomPack(source, state.language === "vi" ? "Gói Mới" : (state.language === "en" ? "New package" : "Yeni paket"));
  persist();
  renderStats();
  selectedAgentId = customPack.agent_id;
  selectPack(customPack.agent_id);
  renderPackList();
  document.getElementById("pack-title").focus();
  showToast(t("new-pack-created"));
});
document.getElementById("delete-pack").addEventListener("click", () => {
  if (state.packs.length <= 1) {
    showToast(t("minimum-pack"));
    return;
  }
  const deletedId = selectedAgentId;
  state.packs = state.packs.filter(pack => pack.agent_id !== deletedId);
  catalog.packs = catalog.packs.filter(pack => pack.agent_id !== deletedId);
  catalog.command_template.agent_list = catalog.command_template.agent_list.filter(agent => agent.agent_id !== deletedId);
  catalog.config_template.packs = catalog.config_template.packs.filter(pack => pack.agent_id !== deletedId);
  selectedAgentId = state.packs[0].agent_id;
  persist();
  renderStats();
  selectPack(selectedAgentId);
  showToast(t("pack-deleted"));
});
document.getElementById("export-button").addEventListener("click", () => {
  try {
    const files = exportFiles();
    Android.exportZip(JSON.stringify(files), t("export-success"), t("file-error"));
  } catch (error) {
    showToast(t("export-error", { message: error.message }));
  }
});

state = loadState();
syncCatalogToState();
history = [JSON.stringify(state)];
historyIndex = 0;
renderHistoryButtons();
applyLanguage();
if (!catalog.packs.length || !catalog.players || !catalog.records) {
  emptyState.textContent = catalog.error || t("data-error");
} else {
  renderStats();
  selectPack(state.packs[0]?.agent_id ?? catalog.packs[0].agent_id);
}
