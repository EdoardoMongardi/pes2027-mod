"use strict";
/* Eklenti: açılışta dil seçimi, Oyuncular sekmesi (ID ile oyuncu bilgisi + düzenleme), Yanıtlar sekmesi İngilizce çevirisi */
(function () {
  const $ = (id) => document.getElementById(id);
  const lang = () => (typeof state !== "undefined" && state && state.language) || "vi";
  const VI_MAP = {
    "Paketler": "Gói Thẻ", "Packages": "Gói Thẻ",
    "Oyuncular": "Dữ Liệu Thẻ", "Players": "Dữ Liệu Thẻ",
    "Kadro": "Đội Hình", "Squad": "Đội Hình",
    "Yanıtlar": "Lệnh Ziru", "Responses": "Lệnh Ziru",
    "berwtproxy tarafından yapıldı": "Phát triển bởi eFootbase · Phiên bản Tiếng Việt",
    "Made by berwtproxy": "Phát triển bởi eFootbase · Phiên bản Tiếng Việt",
    "Oyuncu": "Cầu thủ", "Player": "Cầu thủ",
    "Oyuncu kartı": "Thẻ cầu thủ", "Player card": "Thẻ cầu thủ",
    "Geçerli bir oyuncu ID’si girin.": "Vui lòng nhập ID cầu thủ hợp lệ.",
    "Enter a valid player ID.": "Vui lòng nhập ID cầu thủ hợp lệ.",
    "Bu oyuncu kadroda zaten var.": "Cầu thủ này đã có trong đội hình.",
    "This player is already in the squad.": "Cầu thủ này đã có trong đội hình.",
    "Kadro en fazla 23 oyuncu içerebilir.": "Đội hình chỉ chứa tối đa 23 cầu thủ.",
    "A squad can contain at most 23 players.": "Đội hình chỉ chứa tối đa 23 cầu thủ.",
    "Oyuncu ve menajer seçin veya ID ile ekleyin.": "Chọn cầu thủ và huấn luyện viên hoặc thêm theo ID.",
    "Select players and managers or add them by ID.": "Chọn cầu thủ và huấn luyện viên hoặc thêm theo ID.",
    "Kadro adı": "Tên đội hình", "Squad name": "Tên đội hình",
    "Menajer": "Huấn luyện viên", "Manager": "Huấn luyện viên",
    "Menajer ID’si": "ID Huấn luyện viên", "Manager ID": "ID Huấn luyện viên",
    "ID ile menajer seç": "Chọn HLV theo ID", "Select manager by ID": "Chọn HLV theo ID",
    "ID ekle": "Thêm ID", "Add ID": "Thêm ID",
    "Oyunculardan seç": "Chọn từ danh sách", "Choose from Players": "Chọn từ danh sách",
    "Kadro oyuncuları": "Cầu thủ trong đội hình", "Squad players": "Cầu thủ trong đội hình",
    "Kadro oyuncularını Paketler’e ekle": "Thêm đội hình này vào Gói thẻ",
    "Add squad players to Packages": "Thêm đội hình này vào Gói thẻ",
    "Pakete ekle": "Thêm vào gói", "Add to package": "Thêm vào gói",
    "Kadro değişikliklerini oyuna aktar": "Nạp thay đổi đội hình vào game",
    "Export squad changes for the game": "Nạp thay đổi đội hình vào game",
    "Dışa aktarma CmdLogin yanıtındaki ana kadroyu günceller. Bilinmeyen ID’ler için oyuncu özellikleri oluşturulmaz.": "Lệnh xuất sẽ cập nhật đội hình chính trong phản hồi CmdLogin trên thiết bị.",
    "Export updates the main squad in the CmdLogin response. Player data is not fabricated for unknown IDs.": "Lệnh xuất sẽ cập nhật đội hình chính trong phản hồi CmdLogin trên thiết bị.",
    "Geçerli bir menajer ID’si girin.": "Vui lòng nhập ID HLV hợp lệ.", "Enter a valid manager ID.": "Vui lòng nhập ID HLV hợp lệ.",
    "Bu menajer ID’si response verisinde bulunamadı.": "Không tìm thấy HLV này trong dữ liệu.",
    "This manager ID was not found in the response data.": "Không tìm thấy HLV này trong dữ liệu.",
    "Menajer değerleri 0-99 arasında tam sayı olmalıdır.": "Chỉ số HLV phải là số nguyên từ 0 đến 99.",
    "Manager values must be whole numbers from 0 to 99.": "Chỉ số HLV phải là số nguyên từ 0 đến 99.",
    "Menajer özellikleri kaydedilemedi.": "Không thể lưu thông tin HLV.",
    "Manager settings could not be saved.": "Không thể lưu thông tin HLV.",
    "Menajer özellikleri kaydedildi.": "Đã lưu thông tin HLV thành công.",
    "Manager settings saved.": "Đã lưu thông tin HLV thành công.",
    "Seçilen menajer response dosyasında yok.": "HLV đã chọn không có trong phản hồi dữ liệu.",
    "The selected manager is not present in the response.": "HLV đã chọn không có trong phản hồi dữ liệu.",
    "Menajer özellikleri dışa aktarılamadı: menajer kayıt listesinde bulunamadı.": "Không thể xuất HLV: thiếu HLV trong danh sách.",
    "Manager attributes could not be exported: the manager is missing from the coach list.": "Không thể xuất HLV: thiếu HLV trong danh sách.",
    "Menajer özellikleri geçersiz. Tekrar düzenleyip kaydedin.": "Chỉ số HLV không hợp lệ.",
    "Manager attributes are invalid. Edit and save them again.": "Chỉ số HLV không hợp lệ.",
    "Yeni Kadro": "Đội Hình Mới", "New squad": "Đội Hình Mới",
    "Pozisyona göre filtrele": "Lọc theo vị trí", "Filter by position": "Lọc theo vị trí",
    "Tüm pozisyonlar": "Tất cả vị trí", "All positions": "Tất cả vị trí",
    "Oyuncu veritabanı": "Cơ sở dữ liệu cầu thủ", "Player database": "Cơ sở dữ liệu cầu thủ",
    "Veritabanındaki oyuncuları kartlarıyla görüntüleyin veya arayın. Seçili veritabanındaki tüm oyuncular listelenir.": "Xem và tìm kiếm dữ liệu cầu thủ chi tiết theo thẻ. Hiển thị toàn bộ cầu thủ trong cơ sở dữ liệu.",
    "Browse and search database players by card. All players of the selected database are listed.": "Xem và tìm kiếm dữ liệu cầu thủ chi tiết theo thẻ. Hiển thị toàn bộ cầu thủ trong cơ sở dữ liệu.",
    "Göster": "Hiển thị", "Show": "Hiển thị",
    "Oyuncu ID’si": "ID Cầu thủ", "Player ID": "ID Cầu thủ",
    "Oyuncu adı veya ID ara...": "Tìm theo tên hoặc ID cầu thủ...",
    "Search player name or ID...": "Tìm theo tên hoặc ID cầu thủ...",
    "Ana pozisyon": "Vị trí chính", "Main position": "Vị trí chính",
    "Ülke ID’si": "Mã quốc gia", "Country ID": "Mã quốc gia",
    "Boy (kaynak değeri)": "Chiều cao (cm)", "Height (source value)": "Chiều cao (cm)",
    "Kilo (kaynak değeri)": "Cân nặng (kg)", "Weight (source value)": "Cân nặng (kg)",
    "Oyuncu bilgisi": "Thông tin cầu thủ", "Player data": "Thông tin cầu thủ",
    "Varsayılan canlı veriden temel profil": "Hồ sơ cơ bản từ dữ liệu trực tuyến",
    "Basic profile from default live data": "Hồ sơ cơ bản từ dữ liệu trực tuyến",
    "Seviye": "Cấp độ", "Level": "Cấp độ",
    "Seviye sınırı": "Giới hạn cấp", "Level limit": "Giới hạn cấp",
    "Ülke": "Quốc gia", "Country": "Quốc gia",
    "Takım": "Câu lạc bộ", "Team": "Câu lạc bộ",
    "Güçlü ayak": "Chân thuận", "Strong foot": "Chân thuận",
    "Zayıf ayak": "Chân không thuận", "Weak foot": "Chân không thuận",
    "Sağ (zayıf: sol)": "Phải (chân nghịch: trái)", "Right (weak: left)": "Phải (chân nghịch: trái)",
    "Sol (zayıf: sağ)": "Trái (chân nghịch: phải)", "Left (weak: right)": "Trái (chân nghịch: phải)",
    "Oyuncu adı": "Tên cầu thủ", "Player name": "Tên cầu thủ",
    "Ülke (ID veya ad)": "Quốc gia (ID hoặc tên)", "Country (ID or name)": "Quốc gia (ID hoặc tên)",
    "Takım (ID veya ad)": "Câu lạc bộ (ID hoặc tên)", "Team (ID or name)": "Câu lạc bộ (ID hoặc tên)",
    "Bilinmeyen oyun tarzı": "Phong cách thi đấu chưa xác định", "Unknown playing style": "Phong cách thi đấu chưa xác định",
    "Yok / serbest": "Tự do / Không có", "None / free agent": "Tự do / Không có"
  };
  const L = (tr, en, idn, vi) => {
    const cur = lang();
    if (cur === "vi") return vi || VI_MAP[tr] || VI_MAP[en] || en || tr;
    if (cur === "id") return idn || en || tr;
    if (cur === "en") return en || tr;
    return tr;
  };
  const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const GATE_KEY = "opm_lang_chosen_v1";

  /* ---------- 1) açılışta dil seçimi ---------- */
  function setLanguage(l) {
    state.language = l;
    persist();
    try { localStorage.setItem(GATE_KEY, l); } catch (e) {}
    applyLanguage();
  }
  function initGate() {
    const gate = $("lang-gate");
    let chosen = null;
    try { chosen = localStorage.getItem(GATE_KEY); } catch (e) {}
    gate.querySelectorAll(".lg-btn").forEach((b) => b.addEventListener("click", () => {
      setLanguage(b.dataset.lang);
      gate.hidden = true;
    }));
    if (!chosen) gate.hidden = false;
  }

  /* ---------- 2) oyuncu bilgisi verileri ---------- */
  const SLOT_POS = { 1: "CF", 2: "SS", 3: "LWF", 4: "RWF", 5: "AMF", 6: "DMF", 7: "CMF", 8: "LMF", 9: "RMF", 10: "CB", 11: "LB", 12: "RB", 13: "GK" };
  const POS_NAMES_TR = { GK: "Kaleci", CB: "Stoper", LB: "Sol bek", RB: "Sağ bek", DMF: "Defansif orta saha", CMF: "Merkez orta saha", LMF: "Sol orta saha", RMF: "Sağ orta saha", AMF: "Ofansif orta saha", LWF: "Sol kanat forvet", RWF: "Sağ kanat forvet", SS: "Gölge forvet", CF: "Santrfor" };
  const POS_NAMES_EN = { GK: "Goalkeeper", CB: "Centre-back", LB: "Left-back", RB: "Right-back", DMF: "Defensive midfielder", CMF: "Central midfielder", LMF: "Left midfielder", RMF: "Right midfielder", AMF: "Attacking midfielder", LWF: "Left winger", RWF: "Right winger", SS: "Second striker", CF: "Centre-forward" };
  const POS_NAMES_VI = { GK: "Thủ môn", CB: "Trung vệ", LB: "Hậu vệ trái", RB: "Hậu vệ phải", DMF: "Tiền vệ phòng ngự", CMF: "Tiền vệ trung tâm", LMF: "Tiền vệ cánh trái", RMF: "Tiền vệ cánh phải", AMF: "Tiền vệ tấn công", LWF: "Tiền đạo cánh trái", RWF: "Tiền đạo cánh phải", SS: "Hộ công", CF: "Tiền đạo cắm" };
  const MAIN_POS = ["GK", "CB", "LB", "RB", "DMF", "CMF", "LMF", "RMF", "AMF", "LWF", "RWF", "SS", "CF"];
  
  // PLAYSTYLE ATK (0 - 22)
  const PLAY_STYLES_ATK = [
    { id: 0, tr: "Tidak Ada (Unactivated)", en: "None" },
    { id: 1, tr: "Goal Poacher", en: "Goal Poacher" },
    { id: 2, tr: "Dummy Runner", en: "Dummy Runner" },
    { id: 3, tr: "Fox in the Box", en: "Fox in the Box" },
    { id: 4, tr: "Prolific Winger", en: "Prolific Winger" },
    { id: 5, tr: "Classic No. 10", en: "Classic No. 10" },
    { id: 6, tr: "Hole Player", en: "Hole Player" },
    { id: 7, tr: "Box-to-Box", en: "Box-to-Box" },
    { id: 8, tr: "Anchor Man", en: "Anchor Man" },
    { id: 9, tr: "Destroyer", en: "Destroyer" },
    { id: 10, tr: "Extra Attacker", en: "Extra Attacker" },
    { id: 11, tr: "Attacking Full-back", en: "Attacking Full-back" },
    { id: 12, tr: "Defensive Full-back", en: "Defensive Full-back" },
    { id: 13, tr: "Deep-Lying Forward", en: "Deep-Lying Forward" },
    { id: 14, tr: "Creative Playmaker", en: "Creative Playmaker" },
    { id: 15, tr: "Build Up", en: "Build Up" },
    { id: 16, tr: "Offensive Goalkeeper", en: "Offensive Goalkeeper" },
    { id: 17, tr: "Defensive Goalkeeper", en: "Defensive Goalkeeper" },
    { id: 18, tr: "Roaming Flank", en: "Roaming Flank" },
    { id: 19, tr: "Cross Specialist", en: "Cross Specialist" },
    { id: 20, tr: "Orchestrator", en: "Orchestrator" },
    { id: 21, tr: "Full-back Finisher", en: "Full-back Finisher" },
    { id: 22, tr: "Target Man", en: "Target Man" }
  ];

  // PLAYSTYLE DEF (23 - 33)
  const PLAY_STYLES_DEF = [
    { id: 0, tr: "Tidak Ada (Unactivated)", en: "None" },
    { id: 23, tr: "Press Back", en: "Press Back" },
    { id: 24, tr: "Front Line Pressure", en: "Front Line Pressure" },
    { id: 25, tr: "Front Line Poacher", en: "Front Line Poacher" },
    { id: 26, tr: "Attacking Outlet", en: "Attacking Outlet" },
    { id: 27, tr: "All-action Defender", en: "All-action Defender" },
    { id: 28, tr: "Pass Disruptor", en: "Pass Disruptor" },
    { id: 29, tr: "Covering Role", en: "Covering Role" },
    { id: 30, tr: "High Line Master", en: "High Line Master" },
    { id: 31, tr: "Tough Maker", en: "Tough Maker" },
    { id: 32, tr: "Deep Defender", en: "Deep Defender" },
    { id: 33, tr: "Sweeper GK", en: "Sweeper GK" }
  ];

  // ============================================================================
  // COM Playing Styles (Parameters 63 - 69) - Dribbling & AI Moves
  // ============================================================================
  const COM_PLAYING_STYLES = [
    { parameter: 63, en: "Trickster", id: "Trickster", tr: "Trickster" },
    { parameter: 64, en: "Mazing Run", id: "Mazing Run", tr: "Mazing Run" },
    { parameter: 65, en: "Speeding Bullet", id: "Speeding Bullet", tr: "Speeding Bullet" },
    { parameter: 66, en: "Incisive Run", id: "Incisive Run", tr: "Incisive Run" },
    { parameter: 67, en: "Long Ball Expert", id: "Long Ball Expert", tr: "Long Ball Expert" },
    { parameter: 68, en: "Early Cross", id: "Early Cross", tr: "Early Cross" },
    { parameter: 69, en: "Long Ranger", id: "Long Ranger", tr: "Long Ranger" }
  ];

  // ============================================================================
  // Official eFootball / eFootbase Player Skills (Parameters 70 - 137)
  // ============================================================================
  const PLAYER_SKILLS = [
    { parameter: 70, en: "Scissors Feint", id: "Sentuhan Gunting", tr: "Scissors Feint" },
    { parameter: 71, en: "Flip Flap", id: "Flip Flap", tr: "Flip Flap" },
    { parameter: 72, en: "Marseille Turn", id: "Putaran Marseille", tr: "Marseille Turn" },
    { parameter: 73, en: "Sombrero", id: "Sombrero", tr: "Sombrero" },
    { parameter: 74, en: "Cut Behind & Turn", id: "Potong Belakang & Balik", tr: "Cut Behind & Turn" },
    { parameter: 75, en: "Scotch Move", id: "Gerakan Scotch", tr: "Scotch Move" },
    { parameter: 76, en: "Heading", id: "Sundulan", tr: "Heading" },
    { parameter: 77, en: "Long-range Curler", id: "Lengkung Jarak Jauh", tr: "Long-range Curler" },
    { parameter: 78, en: "Knuckle Shot", id: "Tembakan Knuckle", tr: "Knuckle Shot" },
    { parameter: 79, en: "Acrobatic Finishing", id: "Penyelesaian Akrobatik", tr: "Acrobatic Finishing" },
    { parameter: 80, en: "Heel Trick", id: "Trik Tumit", tr: "Heel Trick" },
    { parameter: 81, en: "First-time Shot", id: "Tembakan Satu Sentuhan", tr: "First-time Shot" },
    { parameter: 82, en: "One-touch Pass", id: "Umpan Satu Sentuhan", tr: "One-touch Pass" },
    { parameter: 83, en: "Weighted Pass", id: "Umpan Berbobot", tr: "Weighted Pass" },
    { parameter: 84, en: "Pinpoint Crossing", id: "Umpan Silang Akurat", tr: "Pinpoint Crossing" },
    { parameter: 85, en: "Outside Curler", id: "Lengkung Luar", tr: "Outside Curler" },
    { parameter: 86, en: "Rabona", id: "Rabona", tr: "Rabona" },
    { parameter: 87, en: "Low Lofted Pass", id: "Umpan Lambung Rendah", tr: "Low Lofted Pass" },
    { parameter: 88, en: "GK Low Punt", id: "Tendangan Voli Rendah Kiper", tr: "GK Low Punt" },
    { parameter: 89, en: "Long Throw", id: "Lemparan Jauh", tr: "Long Throw" },
    { parameter: 90, en: "GK Long Throw", id: "Lemparan Jauh Kiper", tr: "GK Long Throw" },
    { parameter: 91, en: "Gamesmanship", id: "Keahlian Pelanggaran", tr: "Gamesmanship" },
    { parameter: 92, en: "Man Marking", id: "Pengawalan Ketat", tr: "Man Marking" },
    { parameter: 93, en: "Track Back", id: "Bantu Bertahan", tr: "Track Back" },
    { parameter: 94, en: "Acrobatic Clearance", id: "Sapuan Akrobatik", tr: "Acrobatic Clearance" },
    { parameter: 95, en: "Captaincy", id: "Kapten", tr: "Captaincy" },
    { parameter: 96, en: "Super-sub", id: "Pemain Pengganti Super", tr: "Super-sub" },
    { parameter: 97, en: "Fighting Spirit", id: "Semangat Juang", tr: "Fighting Spirit" },
    { parameter: 98, en: "No Look Pass", id: "Umpan Tanpa Melihat", tr: "No Look Pass" },
    { parameter: 99, en: "Penalty Specialist", id: "Spesialis Penalti", tr: "Penalty Specialist" },
    { parameter: 100, en: "GK Penalty Saver", id: "Kiper Penahan Penalti", tr: "GK Penalty Saver" },
    { parameter: 101, en: "Chip Shot Control", id: "Kontrol Tembakan Cip", tr: "Chip Shot Control" },
    { parameter: 102, en: "Sole Control", id: "Kontrol Telapak Kaki", tr: "Sole Control" },
    { parameter: 103, en: "Dipping Shot", id: "Tembakan Menukik", tr: "Dipping Shot" },
    { parameter: 104, en: "Rising Shot", id: "Tembakan Roket", tr: "Rising Shot" },
    { parameter: 105, en: "GK High Punt", id: "Tendangan Voli Tinggi Kiper", tr: "GK High Punt" },
    { parameter: 106, en: "Interception", id: "Intersepsi", tr: "Interception" },
    { parameter: 107, en: "Double Touch", id: "Sentuhan Ganda", tr: "Double Touch" },
    { parameter: 108, en: "Chop Turn", id: "Tipuan Potong Sisi", tr: "Chop Turn" },
    { parameter: 109, en: "Long-range Shooting", id: "Tembakan Jarak Jauh", tr: "Long-range Shooting" },
    { parameter: 110, en: "Through Passing", id: "Umpan Terobosan", tr: "Through Passing" },
    { parameter: 111, en: "Blocker", id: "Pemblokir", tr: "Blocker" },
    { parameter: 112, en: "Aerial Superiority", id: "Superioritas Udara", tr: "Aerial Superiority" },
    { parameter: 113, en: "Sliding Tackle", id: "Tekel Meluncur", tr: "Sliding Tackle" },
    { parameter: 114, en: "Phenomenal Finishing", id: "Penyelesaian Fenomenal", tr: "Phenomenal Finishing" },
    { parameter: 115, en: "Fortress", id: "Benteng Pertahanan", tr: "Fortress" },
    { parameter: 116, en: "Momentum Dribbling", id: "Dribel Kilat", tr: "Momentum Dribbling" },
    { parameter: 117, en: "Game-changing Pass", id: "Umpan Penentu Kemenangan", tr: "Game-changing Pass" },
    { parameter: 118, en: "Visionary Pass", id: "Umpan Visioner", tr: "Visionary Pass" },
    { parameter: 119, en: "Edged Crossing", id: "Umpan Silang Melengkung", tr: "Edged Crossing" },
    { parameter: 120, en: "Blitz Curler", id: "Tembakan Lengkung Kilat", tr: "Blitz Curler" },
    { parameter: 121, en: "Bullet Header", id: "Sundulan Peluru", tr: "Bullet Header" },
    { parameter: 122, en: "Phenomenal Pass", id: "Umpan Fenomenal", tr: "Phenomenal Pass" },
    { parameter: 123, en: "Aerial Fort", id: "Benteng Udara", tr: "Aerial Fort" },
    { parameter: 124, en: "Acceleration Burst", id: "Akselerasi Kilat", tr: "Acceleration Burst" },
    { parameter: 125, en: "Long Reach Tackle", id: "Tekel Jangkauan Panjang", tr: "Long Reach Tackle" },
    { parameter: 126, en: "GK Directing Defence", id: "Kiper Pengarah Pertahanan", tr: "GK Directing Defence" },
    { parameter: 127, en: "Low Screamer", id: "Tembakan Rendah Menggelegar", tr: "Low Screamer" },
    { parameter: 128, en: "Willpower", id: "Tekad Baja", tr: "Willpower" },
    { parameter: 129, en: "GK Spirit Roar", id: "Pekik Tempur Kiper", tr: "GK Spirit Roar" },
    { parameter: 130, en: "Magnetic Feet", id: "Kaki Magnet", tr: "Magnetic Feet" },
    { parameter: 131, en: "Attack Trigger", id: "Pemicu Serangan", tr: "Attack Trigger" },
    { parameter: 132, en: "Shadow Hunt", id: "Penutup Celah", tr: "Shadow Hunt" },
    { parameter: 133, en: "Attacking Surge", id: "Lonjakan Serangan", tr: "Attacking Surge" },
    { parameter: 134, en: "Snap Strike", id: "Tendangan Sentak", tr: "Snap Strike" },
    { parameter: 135, en: "Tap Trick", id: "Trik Sentuhan Cepat", tr: "Tap Trick" },
    { parameter: 136, en: "Sharp Cut", id: "Tebasan Tajam", tr: "Sharp Cut" },
    { parameter: 137, en: "Power Tackle", id: "Tekel Bertenaga", tr: "Power Tackle" }
  ];

  const playStyleLabel = (id, isDef = false) => {
    const list = isDef ? PLAY_STYLES_DEF : PLAY_STYLES_ATK;
    const style = list.find((item) => item.id === Number(id));
    return style ? L(style.tr, style.en, style.id) : `${L("Bilinmeyen oyun tarzı", "Unknown playing style", "Gaya Bermain Tidak Diketahui")} (${id})`;
  };
  const playerSkillLabels = (parameters) => PLAYER_SKILLS
    .filter((skill) => Number(parameters[skill.parameter]) === 1 && skill.en && skill.en.trim() !== "")
    .map((skill) => L(skill.tr, skill.en, skill.id));

  const comPlayingStyleLabels = (parameters) => COM_PLAYING_STYLES
    .filter((com) => Number(parameters[com.parameter]) === 1)
    .map((com) => L(com.tr, com.en, com.id));
  const teamNames = () => {
    const r = window.RESPONSES && window.RESPONSES["ZIRU_2027_TEAM_NAMES.json"];
    return r || {};
  };
  const teamName = (id) => {
    if (!id) return L("Yok / serbest", "None / free agent");
    const t = teamNames()[String(id)];
    return t ? `${t.name} (${t.short})` : `${L("Takım", "Team")} #${id}`;
  };
  const posName = (p) => (lang() === "en" ? POS_NAMES_EN : (lang() === "vi" ? POS_NAMES_VI : POS_NAMES_TR))[p] || p;

  function packsOf(id) {
    return state.packs.filter((p) => p.player_ids.includes(Number(id))).map((p) => p.title);
  }

  const SQUAD_RESPONSE_KEY = "CmdLogin.json";
  const maxSquadPlayers = 23;
  const MANAGER_TACTIC_STATS = [
    { key: "possession", tr: "Topa sahip olma", en: "Possession" },
    { key: "short_counter", tr: "Hızlı kontra", en: "Quick counter" },
    { key: "long_ball", tr: "Uzun top", en: "Long ball" },
    { key: "long_counter", tr: "Uzun kontra", en: "Long counter" },
    { key: "side_attack", tr: "Kanat hücumu", en: "Side attack" },
    { key: "overload", tr: "Yığılma", en: "Overload" }
  ];
  function loginSquadContext(login) {
    const entry = login?.entry_info;
    const squads = entry?.squad_list;
    if (!entry || !Array.isArray(squads) || !squads.length) return null;
    const squad = squads.find((item) => Number(item.squad_id) === Number(entry.main_squad_id)) || squads[0];
    if (!squad?.squad_data || !Array.isArray(squad.squad_data.player_list)) return null;
    return { entry, squad, data: squad.squad_data };
  }

  function squadBuilderState() {
    const login = window.RESPONSES?.[SQUAD_RESPONSE_KEY];
    const context = loginSquadContext(login);
    state.squad_builder ||= {};
    if (!Array.isArray(state.squad_builder.player_ids)) {
      state.squad_builder.player_ids = context ? context.data.player_list.map((item) => String(item.gameplayer_id.id)) : [];
    }
    if (state.squad_builder.manager_id === undefined && context) {
      state.squad_builder.manager_id = Number(context.data.coach?.[0]?.coach_id?.id ?? 0);
    }
    if (state.squad_builder.name === undefined && context) {
      state.squad_builder.name = String(context.squad.name || L("Yeni Kadro", "New squad")).trim();
    }
    return state.squad_builder;
  }

  function squadPlayerInfo(id) {
    const info = catalog.players[String(id)];
    if (info?.name && !/^Oyuncu\s+\d+$/.test(info.name)) return { name: String(info.name), position: playerInfo(Number(id)).position };
    const live = livePlayer(String(id));
    if (Array.isArray(live)) return { name: String(live[0] || `ID ${id}`), position: MAIN_POS[Number(live[4])] || "—" };
    return { name: L("Oyuncu", "Player"), position: "—" };
  }

  function playerCardImage(id, size = "small") {
    const safeId = String(id);
    if (!/^\d{1,20}$/.test(safeId)) return "";
    return `<span class="player-card-thumb player-card-thumb-${size}"><span class="player-card-fallback">⚽</span><img data-player-card="${esc(safeId)}" alt="${L("Oyuncu kartı", "Player card")}" loading="lazy" decoding="async" referrerpolicy="no-referrer"></span>`;
  }

  function loadPlayerCardImages(root) {
    root.querySelectorAll("[data-player-card]").forEach((image) => {
      const id = image.dataset.playerCard;
      image.addEventListener("load", () => {
        const fallback = image.parentElement.querySelector(".player-card-fallback");
        if (fallback) fallback.hidden = true;
      }, { once: true });
      image.addEventListener("error", () => { image.hidden = true; }, { once: true });
      image.src = `https://efimg.com/efootballhub22/images/player_cards/${id}_l.png`;
    });
  }

  function addPlayerToSquad(rawId) {
    const id = String(rawId).trim();
    if (!/^\d{1,20}$/.test(id)) {
      showToast(L("Geçerli bir oyuncu ID’si girin.", "Enter a valid player ID."));
      return false;
    }
    const builder = squadBuilderState();
    if (builder.player_ids.includes(id)) {
      showToast(L("Bu oyuncu kadroda zaten var.", "This player is already in the squad."));
      return false;
    }
    if (builder.player_ids.length >= maxSquadPlayers) {
      showToast(L("Kadro en fazla 23 oyuncu içerebilir.", "A squad can contain at most 23 players."));
      return false;
    }
    ziruCommit(id);
    builder.player_ids.push(id);
    persist();
    renderSquad();
    showToast(L(`${squadPlayerInfo(id).name} kadroya eklendi.`, `${squadPlayerInfo(id).name} added to the squad.`));
    return true;
  }

  function buildSquad() {
    const host = $("view-squad");
    host.innerHTML = `
      <div class="rp-head"><div><span class="pill">${L("Kadro", "Squad")}</span><p>${L("Oyuncu ve menajer seçin veya ID ile ekleyin.", "Select players and managers or add them by ID.")}</p></div></div>
      <section class="pl-card squad-builder">
        <div class="squad-controls">
          <label>${L("Kadro adı", "Squad name")}<input id="squad-name" type="text" maxlength="40"></label>
          <label>${L("Menajer", "Manager")}<select id="squad-manager"></select></label>
        </div>
        <div class="squad-add-row squad-manager-add">
          <label class="search-box"><span aria-hidden="true">#</span><input id="squad-manager-id" type="text" inputmode="numeric" maxlength="20" autocomplete="off" placeholder="${L("Menajer ID’si", "Manager ID")}"></label>
          <button class="button button-secondary" id="squad-add-manager-id" type="button">${L("ID ile menajer seç", "Select manager by ID")}</button>
        </div>
        <div id="squad-manager-info" class="squad-manager-info" aria-live="polite"></div>
        <div class="squad-add-row">
          <label class="search-box"><span aria-hidden="true">#</span><input id="squad-player-id" type="text" inputmode="numeric" maxlength="20" autocomplete="off"></label>
          <button class="button button-primary" id="squad-add-id" type="button">${L("ID ekle", "Add ID")}</button>
          <button class="button button-secondary" id="squad-browse-players" type="button">${L("Oyunculardan seç", "Choose from Players")}</button>
        </div>
        <div class="squad-roster-heading"><h2>${L("Kadro oyuncuları", "Squad players")} <small id="squad-count"></small></h2></div>
        <div id="squad-roster" class="squad-roster"></div>
        <div class="squad-package-row">
          <label>${L("Kadro oyuncularını Paketler’e ekle", "Add squad players to Packages")}<select id="squad-package"></select></label>
          <button class="button button-secondary" id="squad-add-package" type="button">${L("Pakete ekle", "Add to package")}</button>
        </div>
        <div class="pl-actions squad-export-actions"><button class="button button-primary" id="squad-export" type="button">${L("Kadro değişikliklerini oyuna aktar", "Export squad changes for the game")}</button></div>
        <p class="squad-export-note">${L("Dışa aktarma CmdLogin yanıtındaki ana kadroyu günceller. Bilinmeyen ID’ler için oyuncu özellikleri oluşturulmaz.", "Export updates the main squad in the CmdLogin response. Player data is not fabricated for unknown IDs.")}</p>
      </section>`;
    $("squad-add-id").addEventListener("click", () => {
      if (addPlayerToSquad($("squad-player-id").value)) $("squad-player-id").value = "";
    });
    $("squad-player-id").addEventListener("keydown", (event) => {
      if (event.key === "Enter" && addPlayerToSquad($("squad-player-id").value)) $("squad-player-id").value = "";
    });
    $("squad-browse-players").addEventListener("click", () => document.querySelector('.nav-tab[data-v="players"]')?.click());
    $("squad-name").addEventListener("input", () => {
      const value = $("squad-name").value.trim();
      if (!value) return;
      squadBuilderState().name = value;
      persist();
    });
    $("squad-manager").addEventListener("change", () => {
      squadBuilderState().manager_id = Number($("squad-manager").value);
      persist();
      renderSquad();
    });
    const selectManagerById = () => {
      const rawId = $("squad-manager-id").value.trim();
      if (!/^\d{1,20}$/.test(rawId) || !Number.isSafeInteger(Number(rawId)) || Number(rawId) <= 0) {
        showToast(L("Geçerli bir menajer ID’si girin.", "Enter a valid manager ID."));
        return;
      }
      const login = window.RESPONSES?.[SQUAD_RESPONSE_KEY];
      const context = loginSquadContext(login);
      const coaches = context?.entry.coach_list || [];
      const currentCoaches = context?.data.coach || [];
      const exists = [...coaches, ...currentCoaches].some((coach) =>
        Number(coach.coach_id?.id ?? coach.coach_id ?? 0) === Number(rawId)
      );
      if (!exists) {
        showToast(L("Bu menajer ID’si response verisinde bulunamadı.", "This manager ID was not found in the response data."));
        return;
      }
      squadBuilderState().manager_id = Number(rawId);
      persist();
      renderSquad();
      $("squad-manager-id").value = "";
    };
    $("squad-add-manager-id").addEventListener("click", selectManagerById);
    $("squad-manager-id").addEventListener("keydown", (event) => {
      if (event.key === "Enter") selectManagerById();
    });
    $("squad-roster").addEventListener("click", (event) => {
      const button = event.target.closest("[data-squad-remove]");
      if (!button) return;
      const builder = squadBuilderState();
      builder.player_ids = builder.player_ids.filter((id) => id !== button.dataset.squadRemove);
      persist();
      renderSquad();
    });
    $("squad-manager-info").addEventListener("click", (event) => {
      if (!event.target.closest("[data-save-manager-stats]")) return;
      const inputs = [...$("squad-manager-info").querySelectorAll("[data-manager-stat]")];
      const values = {};
      for (const input of inputs) {
        const value = Number(input.value);
        if (!Number.isInteger(value) || value < 0 || value > 99) {
          input.focus();
          showToast(L("Menajer değerleri 0-99 arasında tam sayı olmalıdır.", "Manager values must be whole numbers from 0 to 99."));
          return;
        }
        values[input.dataset.managerStat] = value;
      }
      const managerId = String(squadBuilderState().manager_id || "");
      if (!managerId || inputs.length !== MANAGER_TACTIC_STATS.length) {
        showToast(L("Menajer özellikleri kaydedilemedi.", "Manager settings could not be saved."));
        return;
      }
      squadBuilderState().manager_overrides ||= {};
      squadBuilderState().manager_overrides[managerId] = values;
      persist();
      renderSquad();
      showToast(L("Menajer özellikleri kaydedildi.", "Manager settings saved."));
    });
    $("squad-add-package").addEventListener("click", addSquadToPackage);
    $("squad-export").addEventListener("click", exportSquad);
    renderSquad();
  }

  function renderSquad() {
    const host = $("squad-roster");
    if (!host) return;
    const builder = squadBuilderState();
    const login = window.RESPONSES?.[SQUAD_RESPONSE_KEY];
    const context = loginSquadContext(login);
    const coaches = context?.entry.coach_list || [];
    const currentCoaches = context?.data.coach || [];
    const managerSelect = $("squad-manager");
    const oldManager = Number(builder.manager_id || 0);
    const coachIdOf = (coach) => Number(coach.coach_id?.id ?? coach.coach_id ?? 0);
    managerSelect.innerHTML = coaches.map((coach) => {
      const coachId = coachIdOf(coach);
      const club = coach.initial_base_team_id ? teamName(coach.initial_base_team_id) : L("Menajer", "Manager");
      const selected = coachId === oldManager ? " selected" : "";
      return `<option value="${coachId}"${selected}>${esc(club)} · ID ${coachId} · ${L("Topa sahip olma", "Possession")} ${Number(coach.possession) || 0} / ${L("Hızlı kontra", "Quick counter")} ${Number(coach.short_counter) || 0}</option>`;
    }).join("");
    if (oldManager && !coaches.some((coach) => coachIdOf(coach) === oldManager)) {
      managerSelect.insertAdjacentHTML("beforeend", `<option value="${oldManager}" selected>${L("Mevcut menajer", "Current manager")} · ID ${oldManager}</option>`);
    }
    const selectedManagerId = Number(managerSelect.value);
    const selectedCoach = coaches.find((coach) => coachIdOf(coach) === selectedManagerId);
    const currentCoach = currentCoaches.find((coach) => coachIdOf(coach) === selectedManagerId);
    const editableCoach = selectedCoach || currentCoach;
    const canEditManagerStats = editableCoach && MANAGER_TACTIC_STATS.every(({ key }) =>
      Number.isInteger(Number(editableCoach[key])) && Number(editableCoach[key]) >= 0 && Number(editableCoach[key]) <= 99
    );
    const savedStats = builder.manager_overrides?.[String(selectedManagerId)];
    const managerStats = canEditManagerStats
      ? MANAGER_TACTIC_STATS.map(({ key, tr, en }) => {
        const overrideValue = Number(savedStats?.[key]);
        const value = Number.isInteger(overrideValue) && overrideValue >= 0 && overrideValue <= 99
          ? overrideValue
          : Number(editableCoach[key]);
        return `<label>${L(tr, en)}<input type="number" min="0" max="99" step="1" data-manager-stat="${key}" value="${Number(value)}"></label>`;
      }).join("")
      : "";
    $("squad-manager-info").innerHTML = canEditManagerStats
      ? `<div class="squad-manager-summary">${esc(teamName(editableCoach.initial_base_team_id))} · ID ${selectedManagerId}</div>
        <h3 class="stats-heading">${L("Menajer taktik özellikleri", "Manager tactical attributes")}</h3>
        <div class="manager-stats-grid">${managerStats}</div>
        <button class="button button-secondary" data-save-manager-stats type="button">${L("Menajer özelliklerini uygula", "Apply manager attributes")}</button>`
      : `<p>${L("Seçilen menajer için düzenlenebilir taktik özellik kaydı response verisinde bulunamadı.", "No editable tactical attributes were found for this manager in the response.")}</p>`;
    $("squad-name").value = builder.name || "";
    $("squad-count").textContent = `${builder.player_ids.length} / ${maxSquadPlayers}`;
    host.innerHTML = builder.player_ids.length
      ? builder.player_ids.map((id, index) => {
        const info = squadPlayerInfo(id);
        return `<div class="squad-player">${playerCardImage(id, "squad")}<span class="squad-player-number">${index + 1}</span><span class="squad-player-main"><strong>${esc(info.name)}</strong><small>${esc(info.position)} · ID ${esc(id)}</small></span><button class="button button-secondary squad-remove" type="button" data-squad-remove="${esc(id)}">${L("Çıkar", "Remove")}</button></div>`;
      }).join("")
      : `<div class="squad-empty">${L("Kadro boş. Oyuncular sekmesinden ekleyin veya ID girin.", "Squad is empty. Add players from Players or enter an ID.")}</div>`;
    loadPlayerCardImages(host);
    const packageSelect = $("squad-package");
    packageSelect.innerHTML = state.packs.map((pack) => `<option value="${pack.agent_id}">${esc(pack.title)}</option>`).join("");
    $("squad-add-package").disabled = state.packs.length === 0 || builder.player_ids.length === 0;
  }

  function addSquadToPackage() {
    const pack = state.packs.find((item) => String(item.agent_id) === $("squad-package").value);
    if (!pack) {
      showToast(L("Eklenecek bir paket bulunamadı.", "No package is available."));
      return;
    }
    const allIds = squadBuilderState().player_ids.map(Number);
    const ids = allIds.filter((id) => Array.isArray(catalog.records[String(id)]?.parameters));
    if (!ids.length) {
      showToast(L("Bu kadroda paket dışa aktarımına uygun oyuncu kaydı yok.", "This squad has no full player records that can be exported in a package."));
      return;
    }
    const existing = new Set(pack.player_ids);
    const added = ids.filter((id) => !existing.has(id));
    pack.player_ids.push(...added);
    const catalogPack = catalog.packs.find((item) => item.agent_id === pack.agent_id);
    if (catalogPack) catalogPack.player_ids = [...pack.player_ids];
    persist();
    if(window.renderPackList) window.renderPackList();
    if(window.renderRoster) window.renderRoster();
    if(window.renderFeatured) window.renderFeatured();
    renderSquad();
    const skipped = allIds.length - ids.length;
    showToast(skipped
      ? L(`${added.length} oyuncu eklendi; ${skipped} ID’nin tam kaydı olmadığı için atlandı.`, `${added.length} players added; ${skipped} IDs were skipped because full records are missing.`)
      : L(`${added.length} oyuncu pakete eklendi.`, `${added.length} players added to the package.`));
  }

  function exportSquad() {
    const source = window.RESPONSES?.[SQUAD_RESPONSE_KEY];
    const context = loginSquadContext(source);
    if (!source || !context) {
      showToast(L("CmdLogin response içinde düzenlenebilir kadro bulunamadı.", "No editable squad was found in the CmdLogin response."));
      return;
    }
    const builder = squadBuilderState();
    if (!builder.player_ids.length || builder.player_ids.length > maxSquadPlayers) {
      showToast(L("Kadro 1-23 oyuncu arasında olmalıdır.", "The squad must contain 1-23 players."));
      return;
    }
    const login = cloneJson(source);
    const editable = loginSquadContext(login);
    const entry = editable.entry;
    const squad = editable.squad;
    const data = editable.data;
    const recordById = new Map((entry.gameplayer_list || []).map((record) => [
      String(record.gameplayer_id?.id ?? ""),
      record
    ]));
    const previousPlayerList = data.player_list;
    const originalSlots = new Map(previousPlayerList.map((player) => [
      String(player.gameplayer_id?.id ?? ""),
      player
    ]));
    const newIndexById = new Map(builder.player_ids.map((id, index) => [id, index]));
    const usedUniformNumbers = new Set();
    const roster = builder.player_ids.map((id, index) => {
      const original = originalSlots.get(id);
      const owned = recordById.get(id);
      const databaseRecord = catalog.records[id];
      const sourceId = original?.gameplayer_id || owned?.gameplayer_id || databaseRecord?.gameplayer_id;
      let uniformNumber = Number(original?.uniform_number);
      if (!Number.isInteger(uniformNumber) || uniformNumber < 1 || usedUniformNumbers.has(uniformNumber)) {
        uniformNumber = 1;
        while (usedUniformNumbers.has(uniformNumber)) uniformNumber++;
      }
      usedUniformNumbers.add(uniformNumber);
      return {
        gameplayer_id: { id: Number(id), serial: Number(sourceId?.serial) || 0 },
        uniform_number: uniformNumber
      };
    });
    const manager = (entry.coach_list || []).find((coach) =>
      Number(coach.coach_id?.id ?? coach.coach_id ?? 0) === Number(builder.manager_id)
    );
    const currentManager = data.coach?.find((coach) =>
      Number(coach.coach_id?.id ?? coach.coach_id ?? 0) === Number(builder.manager_id)
    );
    const managerId = manager?.coach_id || currentManager?.coach_id;
    if (!managerId || !Number(managerId.id ?? managerId)) {
      showToast(L("Seçilen menajer response dosyasında yok.", "The selected manager is not present in the response."));
      return;
    }
    const managerOverride = builder.manager_overrides?.[String(builder.manager_id)];
    if (managerOverride) {
      if (!manager) {
        showToast(L("Menajer özellikleri dışa aktarılamadı: menajer kayıt listesinde bulunamadı.", "Manager attributes could not be exported: the manager is missing from the coach list."));
        return;
      }
      for (const { key } of MANAGER_TACTIC_STATS) {
        const value = Number(managerOverride[key]);
        if (!Number.isInteger(value) || value < 0 || value > 99) {
          showToast(L("Menajer özellikleri geçersiz. Tekrar düzenleyip kaydedin.", "Manager attributes are invalid. Edit and save them again."));
          return;
        }
        manager[key] = value;
      }
    }
    data.player_list = roster;
    const order = data.squad_gameplan?.match_plan_settings?.order;
    if (Array.isArray(order)) {
      data.squad_gameplan.match_plan_settings.order = order.map((oldIndex) => {
        if (oldIndex === 255) return 255;
        const oldId = previousPlayerList[oldIndex]?.gameplayer_id?.id;
        return oldId === undefined ? 255 : (newIndexById.get(String(oldId)) ?? 255);
      });
    }
    data.coach = [{ coach_id: cloneJson(typeof managerId === "object" ? managerId : { id: Number(managerId), serial: 0 }) }];
    squad.name = String(builder.name || "").trim() || L("Yeni Kadro", "New squad");
    entry.gameplayer_list ||= [];
    
    const LOGIN_PARAM_LEN = (entry.gameplayer_list[0]?.parameters || []).length || 145;
    let appliedEdits = 0;
    [...recordById.keys()].forEach((id) => {
      const owned = recordById.get(id);
      const base = baseRecords[id];
      const edited = state.player_overrides?.[id];
      if (!owned || !base || !edited) return;
      let touched = false;
      (edited.parameters || []).forEach((v, i) => {
        if (i < owned.parameters.length && v !== base.parameters[i]) { owned.parameters[i] = v; touched = true; }
      });
      ["level", "level_limit"].forEach((k) => {
        if (edited.level_info?.[k] !== base.level_info?.[k]) { owned.level_info[k] = edited.level_info[k]; touched = true; }
      });
      ["initial_base_team_id", "initial_base_national_team_id", "default_rarity"].forEach((k) => {
        if (edited[k] !== base[k]) { owned[k] = edited[k]; touched = true; }
      });
      if (JSON.stringify(edited.play_style) !== JSON.stringify(base.play_style)) { owned.play_style = cloneJson(edited.play_style); touched = true; }
      if (touched) appliedEdits++;
    });
    const unresolvedIds = [];
    builder.player_ids.forEach((id) => {
      if (recordById.has(id)) return;
      if (!catalog.records[id]) {
        unresolvedIds.push(id);
        return;
      }
      const newRecord = cloneJson(catalog.records[id]);
      newRecord.gameplayer_id ||= { id: Number(id), serial: 0 };
      newRecord.gameplayer_id.id = Number(id);
      newRecord.gameplayer_id.serial = 0;
      if (Array.isArray(newRecord.parameters)) newRecord.parameters = newRecord.parameters.slice(0, LOGIN_PARAM_LEN);
      if (entry.gameplayer_list[0] && !("additional_position" in entry.gameplayer_list[0])) { delete newRecord.additional_position; delete newRecord.additional_skill_id; }
      entry.gameplayer_list.push(newRecord);
      recordById.set(id, newRecord);
    });
    if (typeof Android === "undefined" || !Android.exportZip) {
      showToast(L("Bu cihazda ZIP dışa aktarma kullanılamıyor.", "ZIP export is unavailable on this device."));
      return;
    }
    const files = {
      [SQUAD_RESPONSE_KEY]: `${JSON.stringify(login, null, 2)}\n`,
      "SQUAD_IMPORT_README.txt": [
        L("Kadroyu oyuna aktarma", "Importing the squad into the game"),
        "",
        L("Bu paketteki CmdLogin.json dosyasını oyunun kullandığı responses klasörüne kopyalayın. Mevcut dosyanın yedeğini alın.", "Copy CmdLogin.json from this package into the responses folder used by the game. Back up the existing file first."),
        L("Kadro verisi response içindeki mevcut ana kadro yapısı korunarak güncellendi.", "The squad was updated while preserving the response's existing main-squad structure."),
        unresolvedIds.length
          ? `${L("Oyuncu verisi response/katalogda bulunmayan ID’ler (yalnızca kadro referansı yazıldı):", "IDs without player data in the response/catalog (squad references only):")} ${unresolvedIds.join(", ")}`
          : L("Eklenen oyuncuların tam kayıtları response içindeki gameplayer_list alanına eklendi.", "Full records for newly added catalog players were added to gameplayer_list."),
        ""
      ].join("\n")
    };
    if (unresolvedIds.length) {
      showToast(L(`Kadro aktarıldı; ${unresolvedIds.length} ID için oyuncu kaydı bulunmadığından yalnızca ID eklendi.`, `Squad exported; ${unresolvedIds.length} IDs had no player record, so only their IDs were added.`));
    }
    Android.exportZip(JSON.stringify(files), L("Kadro response dosyası dışa aktarıldı.", "Squad response exported."), L("Kadro dışa aktarılamadı.", "Squad export failed."));
  }

  /* ---------- 3) Oyuncular sekmesi ---------- */
  let currentId = null;
  const livePlayers = catalog.live_players || {};
  const livePlayerIds = Object.keys(livePlayers).sort();
  /* ---------- veritabanı seçici: 2027 / Ziru ---------- */
  let dbName = "2027";
  const ziru = { db: null, bytes: null, index: null, extra: [], metaIds: [] };
  let databasePlayerIds = [];
  let databaseIdsById = [];
  let idOnlyLivePlayerIds = [];
  const readDbChoice = () => { try { return localStorage.getItem("opm_db") === "ziru" ? "ziru" : "2027"; } catch (e) { return "2027"; } };
  const saveDbChoice = () => { try { localStorage.setItem("opm_db", dbName); } catch (e) {} };
  function ziruLoadScript(done) {
    if (window.ZIRU_DB) { done(true); return; }
    const el = document.createElement("script");
    el.src = "ziru_db.js";
    el.onload = () => done(Boolean(window.ZIRU_DB));
    el.onerror = () => done(false);
    document.head.appendChild(el);
  }
  function ziruPrepare() {
    if (ziru.bytes) return true;
    if (!window.ZIRU_DB) return false;
    ziru.db = window.ZIRU_DB;
    const bin = atob(ziru.db.rows);
    const bytes = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
    ziru.bytes = bytes;
    ziru.index = new Map(ziru.db.ids.map((id, i) => [String(id), i]));
    return true;
  }
  /* Player.bin'den doğrulanan alanlar: ana pozisyon, boy, kilo, 26 istatistik, GK slotu, booster ID'leri.
     Yaş, ayak, oyun tarzı, beceriler ve diğer pozisyon yeterlilikleri Player.bin içinde bulunmaz; varsayılanla başlar. */
  function ziruBuild(key) {
    const i = ziru.index && ziru.index.get(key);
    if (i === undefined) return null;
    const rl = ziru.db.rowLen, o = i * rl, r = ziru.bytes, stats = ziru.db.statIdx;
    const record = cloneJson(baseRecords[Object.keys(baseRecords)[0]]);
    const p = record.parameters.map(() => 0);
    record.gameplayer_id = { id: Number(key), serial: 0 };
    record.level_info = { level: 1, level_limit: 30, exp_total: 0, exp_current_level: 0, exp_to_next_level: 0, customize_point: 0 };
    record.initial_base_team_id = 0;
    record.initial_base_national_team_id = 0;
    record.default_rarity = 4;
    record.play_style = [0, 0];
    record.additional_skill_id = [];
    record.additional_position = [];
    const pos = Math.min(Math.max(r[o], 0), MAIN_POS.length - 1);
    p[0] = pos;
    p[13] = r[o + 3];
    p[POSITION_SLOT_BY_NAME[MAIN_POS[pos]]] = 2;
    if (!ziru.wf && ziru.db.wf) { const wb = atob(ziru.db.wf); ziru.wf = new Uint8Array(wb.length); for (let q = 0; q < wb.length; q++) ziru.wf[q] = wb.charCodeAt(q); }
    { const wv = ziru.wf ? ziru.wf[i] : 0; p[33] = wv & 3; p[45] = (wv >> 2) & 3; }
    p[46] = 25;
    p[47] = r[o + 1] + 100;
    p[48] = r[o + 2] + 30;
    stats.forEach((idx, t) => { p[idx] = 40 + r[o + 4 + t]; });
    const b = 4 + stats.length;
    p[142] = r[o + b];
    p[143] = r[o + b + 1] | (r[o + b + 2] << 8);
    record.initial_booster_id_list = [p[142], p[143], 0];
    record.parameters = p;
    return record;
  }
  function ziruEnsure(key) {
    key = String(key);
    if (dbName !== "ziru" || !ziru.bytes || baseRecords[key] || catalog.records[key]) return;
    const record = ziruBuild(key);
    if (!record) return;
    catalog.records[key] = record;
    ziru.extra.push(key);
  }
  function ziruCommit(key) {
    key = String(key);
    if (dbName !== "ziru" || baseRecords[key] || state.custom_records[key]) return;
    ziruEnsure(key);
    const record = catalog.records[key];
    if (!record) return;
    const i = ziru.index.get(key);
    const name = String(catalog.players[key]?.name || ziru.db.names[i] || `ID ${key}`);
    state.custom_records[key] = record;
    state.player_originals[key] = state.player_originals[key] || cloneJson(record);
    state.custom_players ||= {};
    state.custom_players[key] = { name, position: MAIN_POS[Number(record.parameters[0])] || "—", age: Number(record.parameters[46]) };
    catalog.players[key] = state.custom_players[key];
  }
  function ziruApplyMeta() {
    if (!ziru.bytes) return;
    const rl = ziru.db.rowLen;
    ziru.db.ids.forEach((id, i) => {
      const key = String(id);
      if (catalog.players[key]) return;
      catalog.players[key] = { name: String(ziru.db.names[i] || `ID ${key}`), position: MAIN_POS[Math.min(ziru.bytes[i * rl], MAIN_POS.length - 1)] || "—", age: "—" };
      ziru.metaIds.push(key);
    });
  }
  function ziruRemoveMeta() {
    ziru.metaIds.forEach((key) => { if (!state.custom_players?.[key]) delete catalog.players[key]; });
    ziru.extra.forEach((key) => { if (!state.custom_records?.[key]) delete catalog.records[key]; });
    ziru.metaIds = [];
    ziru.extra = [];
  }
  function recomputeDbLists() {
    const hasParams = (id) => Array.isArray(catalog.records[id]?.parameters);
    if (dbName === "ziru" && ziru.bytes) {
      const set = new Set(ziru.db.ids.map(String));
      Object.keys(catalog.records).forEach((id) => { if (hasParams(id)) set.add(id); });
      const keyed = Array.from(set, (id) => [String(catalog.players[id]?.name || "").toLowerCase(), id]);
      keyed.sort((x, y) => (x[0] < y[0] ? -1 : x[0] > y[0] ? 1 : 0));
      databasePlayerIds = keyed.map((x) => x[1]);
      databaseIdsById = databasePlayerIds.slice().sort();
      idOnlyLivePlayerIds = livePlayerIds.filter((id) => !set.has(id));
      return;
    }
    databasePlayerIds = Object.keys(catalog.records)
      .filter(hasParams)
      .sort((left, right) => {
        const leftName = /^Oyuncu\s+\d+$/.test(String(catalog.players[left]?.name || "")) ? "" : String(catalog.players[left]?.name || "");
        const rightName = /^Oyuncu\s+\d+$/.test(String(catalog.players[right]?.name || "")) ? "" : String(catalog.players[right]?.name || "");
        if (!leftName && rightName) return 1;
        if (leftName && !rightName) return -1;
        return leftName.localeCompare(rightName);
      });
    if (livePlayerIds.length) {
      const have = new Set(databasePlayerIds);
      const nm = (id) => String(catalog.players[id]?.name || (Array.isArray(livePlayers[id]) ? livePlayers[id][0] : "") || "").toLowerCase();
      const extra = livePlayerIds.filter((id) => !have.has(id));
      databasePlayerIds = databasePlayerIds.concat(extra.sort((a, b) => { const x = nm(a), y = nm(b); return x < y ? -1 : x > y ? 1 : 0; }));
    }
    databaseIdsById = databasePlayerIds.slice().sort();
    idOnlyLivePlayerIds = [];
  }
  const dbInfo = (id) => {
    const key = String(id);
    const base = playerInfo(Number(key));
    const live = livePlayers[key];
    if (Array.isArray(live) && (!catalog.players[key] || /^Oyuncu\s+\d+$/.test(String(catalog.players[key].name || "")) || base.position === "—")) {
      return { ...base, name: String(live[0] || base.name || `ID ${key}`), position: MAIN_POS[Number(live[4])] || base.position || "—" };
    }
    return base;
  };
  const idLenOk = (id) => dbName !== "ziru" || String(id).length >= 9;
  recomputeDbLists();
  let databaseVisibleCount = 48;
  const livePlayer = (id) => state.live_player_overrides?.[id] || livePlayers[id];
  function build() {
    const host = $("view-players");
    host.innerHTML = `
      <div class="rp-head"><div><span class="pill" data-x="pl-title"></span><p data-x="pl-intro"></p></div></div>
      <div class="pl-db-bar"><span class="pl-db-bar-label" id="pl-db-label"></span><button class="pl-db-switch" id="pl-db-switch" type="button" aria-haspopup="true"></button><div class="pl-db-menu" id="pl-db-menu" hidden></div></div>
      <div class="pl-search">
        <label class="search-box"><span aria-hidden="true">#</span><input id="pl-id" type="text" inputmode="numeric" maxlength="20" autocomplete="off"></label>
        <button class="button button-primary" id="pl-go" type="button" data-x="pl-show"></button>
      </div>
      <label class="search-box pl-name-search"><span aria-hidden="true">⌕</span><input id="pl-q" autocomplete="off"></label>
      <label class="search-box pl-position-filter"><span aria-hidden="true">⌖</span><select id="pl-position-filter" aria-label="${L("Pozisyona göre filtrele", "Filter by position")}"><option value="">${L("Tüm pozisyonlar", "All positions")}</option>${MAIN_POS.map((position) => `<option value="${position}">${position} · ${esc(posName(position))}</option>`).join("")}</select></label>
      <div id="pl-results" class="player-results"></div>
      <section id="pl-database" class="pl-database">
        <div class="pl-database-head"><h2>${L("Oyuncu veritabanı", "Player database")} <small id="pl-db-count"></small></h2></div>
        <div id="pl-db-grid" class="pl-db-grid"></div>
        <button class="button button-secondary pl-db-more" id="pl-db-more" type="button"></button>
      </section>
      <div id="pl-card"></div>`;
    $("pl-go").addEventListener("click", () => show($("pl-id").value));
    $("pl-id").addEventListener("keydown", (e) => { if (e.key === "Enter") show($("pl-id").value); });
    let tm;
    $("pl-q").addEventListener("input", () => { databaseVisibleCount = 48; clearTimeout(tm); tm = setTimeout(search, 120); });
    $("pl-position-filter").addEventListener("change", () => { databaseVisibleCount = 48; search(); });
    $("pl-db-more").addEventListener("click", () => { databaseVisibleCount += 48; renderDatabaseGallery(); });
    $("pl-db-switch").addEventListener("click", (event) => {
      event.stopPropagation();
      const menu = $("pl-db-menu");
      menu.hidden = !menu.hidden;
    });
    $("pl-db-menu").addEventListener("click", (event) => {
      const button = event.target.closest("[data-db-name]");
      if (!button) return;
      $("pl-db-menu").hidden = true;
      setDatabase(button.dataset.dbName);
    });
    document.addEventListener("click", () => { const menu = $("pl-db-menu"); if (menu) menu.hidden = true; });
    injectDbStyle();
    labels();
    if (readDbChoice() === "ziru" && dbName !== "ziru") setTimeout(() => setDatabase("ziru"), 0);
  }
  function injectDbStyle() {
    if (document.getElementById("pl-db-style")) return;
    const style = document.createElement("style");
    style.id = "pl-db-style";
    // Tweak to ensure horizontal scroll is disabled globally in this view
    style.textContent = `
      .pl-db-bar{position:relative;display:flex;align-items:center;gap:10px;margin:10px 0}
      .pl-db-bar-label{opacity:.7;font-size:.9rem}
      .pl-db-switch{font:inherit;color:inherit;padding:8px 14px;border-radius:12px;border:1px solid rgba(128,128,128,.4);background:rgba(128,128,128,.14);font-weight:700;cursor:pointer}
      .pl-db-menu{position:absolute;top:100%;left:0;z-index:30;margin-top:6px;min-width:140px;padding:6px;border-radius:12px;border:1px solid rgba(128,128,128,.4);background:#14141f;box-shadow:0 8px 24px rgba(0,0,0,.45)}
      .pl-db-menu button{display:block;width:100%;text-align:left;font:inherit;color:#fff;padding:10px 12px;border:0;border-radius:8px;background:transparent;cursor:pointer;font-weight:700}
      .pl-db-menu button:hover,.pl-db-menu button:active{background:rgba(128,128,128,.25)}
      html, body { overflow-x: hidden !important; width: 100vw !important; max-width: 100% !important; margin: 0; padding: 0; position:relative; }
      * { max-width: 100%; box-sizing: border-box !important; }
    `;
    document.head.appendChild(style);
  }
  function renderDbSwitch() {
    const sw = $("pl-db-switch");
    if (!sw) return;
    $("pl-db-label").textContent = L("Veritabanı", "Database");
    sw.textContent = (dbName === "ziru" ? "Ziru" : "2027") + " ▾";
    const other = dbName === "ziru" ? ["2027", "2027"] : ["ziru", "Ziru"];
    $("pl-db-menu").innerHTML = `<button type="button" data-db-name="${other[0]}">${other[1]}</button>`;
  }
  function afterDbSwitch() {
    databaseVisibleCount = 48;
    currentId = null;
    const card = $("pl-card");
    if (card) card.innerHTML = "";
    renderDbSwitch();
    if ($("pl-q")) search();
  }
  function setDatabase(name) {
    if (name === "ziru") {
      const apply = () => {
        if (!ziruPrepare()) { showToast(L("Ziru veritabanı yüklenemedi.", "Could not load the Ziru database.")); return; }
        dbName = "ziru";
        ziruApplyMeta();
        recomputeDbLists();
        saveDbChoice();
        afterDbSwitch();
        { const zn = ziru.db.ids.filter((x) => String(x).length >= 9).length; showToast(L(`Ziru veritabanı: ${zn} oyuncu (ID 9+ hane)`, `Ziru database: ${zn} players (ID 9+ digits)`)); }
      };
      if (window.ZIRU_DB) { apply(); return; }
      showToast(L("Ziru veritabanı yükleniyor…", "Loading Ziru database…"));
      ziruLoadScript((ok) => { if (ok) apply(); else showToast(L("Ziru veritabanı dosyası bulunamadı.", "Ziru database file was not found.")); });
      return;
    }
    dbName = "2027";
    ziruRemoveMeta();
    recomputeDbLists();
    saveDbChoice();
    afterDbSwitch();
  }
  function labels() {
    const host = $("view-players");
    if (!host || !host.firstChild) return;
    const map = {
      "pl-title": L("Oyuncular", "Players"),
      "pl-intro": L("Veritabanındaki oyuncuları kartlarıyla görüntüleyin veya arayın. Seçili veritabanındaki tüm oyuncular listelenir.", "Browse and search database players by card. All players of the selected database are listed."),
      "pl-show": L("Göster", "Show")
    };
    host.querySelectorAll("[data-x]").forEach((el) => { el.textContent = map[el.dataset.x]; });
    renderDbSwitch();
    $("pl-id").placeholder = L("Oyuncu ID’si", "Player ID");
    $("pl-q").placeholder = L("Oyuncu adı veya ID ara...", "Search player name or ID...");
    const positionFilter = $("pl-position-filter");
    const selectedPosition = positionFilter.value;
    positionFilter.innerHTML = `<option value="">${L("Tüm pozisyonlar", "All positions")}</option>${MAIN_POS.map((position) => `<option value="${position}">${position} · ${esc(posName(position))}</option>`).join("")}`;
    positionFilter.value = selectedPosition;
    search();
    if (currentId) show(currentId, true);
  }
  function search() {
    const q = $("pl-q").value.trim();
    const selectedPosition = $("pl-position-filter").value;
    const matchesPosition = (position) => !selectedPosition || position === selectedPosition;
    const out = $("pl-results");
    const database = $("pl-database");
    if (!q) {
      out.hidden = true;
      out.innerHTML = "";
      database.hidden = false;
      renderDatabaseGallery();
      return;
    }
    database.hidden = true;
    const rows = [];
    let available = 0;
    if (/^\d+$/.test(q)) {
      const addIdMatches = (ids, getInfo) => {
        let first = 0, last = ids.length;
        while (first < last) {
          const mid = (first + last) >>> 1;
          if (ids[mid] < q) first = mid + 1;
          else last = mid;
        }
        for (let index = first; index < ids.length && ids[index].startsWith(q); index++) {
          const id = ids[index];
          const info = getInfo(id);
          if (!matchesPosition(info.position) || !idLenOk(id)) continue;
          available++;
          if (rows.length < 25) rows.push([id, info]);
        }
      };
      addIdMatches(databaseIdsById, (id) => dbInfo(id));
      addIdMatches(idOnlyLivePlayerIds, (id) => {
        const data = livePlayer(id);
        return { name: String(data[0] || `ID ${id}`), position: MAIN_POS[Number(data[4])] || "—" };
      });
    } else {
      const query = normalizeSearch(q);
      databasePlayerIds.forEach((id) => {
        const info = dbInfo(id);
        const name = String(info.name || "");
        if (!/^Oyuncu\s+\d+$/.test(name) && normalizeSearch(name).includes(query) && matchesPosition(info.position) && idLenOk(id)) {
          available++;
          if (rows.length < 25) rows.push([id, info]);
        }
      });
    }
    rows.sort(([left], [right]) => left.localeCompare(right));
    out.hidden = false;
    const heading = L(`Arama sonuçları · ${available}`, `Search results · ${available}`);
    out.innerHTML = `<div class="result-row"><div class="result-info"><strong>${heading}</strong><small>${L("Seçili veritabanındaki tüm oyuncular", "All players of the selected database")}</small></div></div>` +
      (rows.length
        ? rows.map(([id, p]) => `<div class="result-row">${playerCardImage(id, "small")}<div class="result-info"><strong>${esc(p.name)}</strong><small>ID ${id} · ${esc(p.position)}</small></div><button class="button button-secondary" data-pick="${id}" type="button">${L("Oyuncuyu aç", "Open player")}</button></div>`).join("")
        : `<div class="result-row"><div class="result-info"><strong>${L("Oyuncu bulunamadı", "No player found")}</strong></div></div>`);
    loadPlayerCardImages(out);
    out.querySelectorAll("[data-pick]").forEach((b) => b.addEventListener("click", () => { $("pl-id").value = b.dataset.pick; openPlayerInfo(b.dataset.pick); }));
  }

  function normalizeSearch(value) {
    return String(value).normalize("NFD").replace(/[\u0300-\u036f]/g, "").toLocaleLowerCase();
  }

  function renderDatabaseGallery() {
    const grid = $("pl-db-grid");
    if (!grid) return;
    const selectedPosition = $("pl-position-filter").value;
    const filteredIds = databasePlayerIds.filter((id) => (!selectedPosition || dbInfo(id).position === selectedPosition) && idLenOk(id));
    const visibleIds = filteredIds.slice(0, databaseVisibleCount);
    $("pl-db-count").textContent = L(`${filteredIds.length} oyuncu`, `${filteredIds.length} players`);
    grid.innerHTML = visibleIds.map((id) => {
      const info = dbInfo(id);
      const name = /^Oyuncu\s+\d+$/.test(String(info.name || "")) ? L("Oyuncu", "Player") : info.name;
      const pos = info.position || "—";
      const rec = catalog.records[id];
      let ovrBadge = "";
      if (rec && typeof eFHub !== "undefined") {
        const stats = {};
        for (const [k, idx] of Object.entries(eFHub.STAT_MAP)) {
          stats[k] = rec.parameters?.[idx] || 40;
        }
        const ovr = eFHub.calculateOvr(stats, pos);
        if (ovr > 0) ovrBadge = `<span class="pl-db-ovr-badge">${ovr}</span>`;
      }
      return `<button class="pl-db-item" type="button" data-db-pick="${id}" style="position:relative">${playerCardImage(id, "gallery")}${ovrBadge}<span class="pl-db-info"><strong>${esc(name)}</strong><small>${esc(pos)} · ID ${id}</small></span></button>`;
    }).join("");
    const more = $("pl-db-more");
    more.hidden = visibleIds.length >= filteredIds.length;
    more.textContent = L("Daha fazla oyuncu göster", "Show more players");
    loadPlayerCardImages(grid);
    grid.querySelectorAll("[data-db-pick]").forEach((button) => button.addEventListener("click", () => {
      $("pl-id").value = button.dataset.dbPick;
      openPlayerInfo(button.dataset.dbPick);
    }));
  }


  // ============================================================================
  // eFootbase Embedded Engine: 51 Boosters & 65 Managers & Calibrated OVR
  // ============================================================================
  const ALL_BOOSTERS = {"basic_0": {"id": 0, "type": "basic", "name": "No Booster", "bonus": {}}, "basic_1": {"id": 1, "type": "basic", "name": "Accuracy +1", "bonus": {"low-pass": 1, "lofted-pass": 1, "finishing": 1, "kicking-power": 1}}, "basic_2": {"id": 2, "type": "basic", "name": "Aerial +1", "bonus": {"finishing": 1, "heading": 1, "physical-contact": 1, "jumping": 1}}, "basic_3": {"id": 3, "type": "basic", "name": "Aerial Block +1", "bonus": {"heading": 1, "defensive-awareness": 1, "physical-contact": 1, "jumping": 1}}, "basic_4": {"id": 4, "type": "basic", "name": "Agility +1", "bonus": {"speed": 1, "acceleration": 1, "balance": 1, "stamina": 1}}, "basic_5": {"id": 5, "type": "basic", "name": "Balancer +1", "bonus": {"offensive-awareness": 1, "defensive-awareness": 1, "acceleration": 1, "stamina": 1}}, "basic_6": {"id": 6, "type": "basic", "name": "Ball Protection +1", "bonus": {"ball-control": 1, "tight-possession": 1, "balance": 1, "physical-contact": 1}}, "basic_7": {"id": 7, "type": "basic", "name": "Ball-carrying +1", "bonus": {"tight-possession": 1, "dribbling": 1, "speed": 1, "balance": 1}}, "basic_8": {"id": 8, "type": "basic", "name": "Breakthrough +1", "bonus": {"dribbling": 1, "kicking-power": 1, "speed": 1, "physical-contact": 1}}, "basic_9": {"id": 9, "type": "basic", "name": "Counter +1", "bonus": {"low-pass": 1, "defensive-engagement": 1, "ball-winning": 1, "physical-contact": 1}}, "basic_10": {"id": 10, "type": "basic", "name": "Crossing +1", "bonus": {"lofted-pass": 1, "curl": 1, "speed": 1, "stamina": 1}}, "basic_11": {"id": 11, "type": "basic", "name": "Defending +1", "bonus": {"defensive-awareness": 1, "ball-winning": 1, "acceleration": 1, "jumping": 1}}, "basic_12": {"id": 12, "type": "basic", "name": "Duelling +1", "bonus": {"defensive-awareness": 1, "ball-winning": 1, "speed": 1, "stamina": 1}}, "basic_13": {"id": 13, "type": "basic", "name": "Fantasista +1", "bonus": {"ball-control": 1, "dribbling": 1, "finishing": 1, "balance": 1}}, "basic_14": {"id": 14, "type": "basic", "name": "Free-kick Taking +1", "bonus": {"finishing": 1, "set-piece-taking": 1, "curl": 1, "kicking-power": 1}}, "basic_15": {"id": 15, "type": "basic", "name": "Goalkeeping +1", "bonus": {"gk-awareness": 1, "gk-catching": 1, "gk-clearing": 1, "gk-reflexes": 1}}, "basic_16": {"id": 16, "type": "basic", "name": "Hard Worker +1", "bonus": {"aggression": 1, "acceleration": 1, "physical-contact": 1, "stamina": 1}}, "basic_17": {"id": 17, "type": "basic", "name": "Off the Ball +1", "bonus": {"offensive-awareness": 1, "speed": 1, "acceleration": 1, "stamina": 1}}, "basic_18": {"id": 18, "type": "basic", "name": "Offence Creator +1", "bonus": {"offensive-awareness": 1, "ball-control": 1, "low-pass": 1, "kicking-power": 1}}, "basic_19": {"id": 19, "type": "basic", "name": "Passing +1", "bonus": {"low-pass": 1, "lofted-pass": 1, "curl": 1, "kicking-power": 1}}, "basic_20": {"id": 20, "type": "basic", "name": "Physicality +1", "bonus": {"balance": 1, "physical-contact": 1, "jumping": 1, "stamina": 1}}, "basic_21": {"id": 21, "type": "basic", "name": "Rebuilding +1", "bonus": {"low-pass": 1, "defensive-awareness": 1, "defensive-engagement": 1, "aggression": 1}}, "basic_22": {"id": 22, "type": "basic", "name": "Regista +1", "bonus": {"tight-possession": 1, "low-pass": 1, "defensive-awareness": 1, "ball-winning": 1}}, "basic_23": {"id": 23, "type": "basic", "name": "Saving +1", "bonus": {"gk-awareness": 1, "gk-clearing": 1, "gk-reflexes": 1, "gk-reach": 1}}, "basic_24": {"id": 24, "type": "basic", "name": "Shooting +1", "bonus": {"ball-control": 1, "finishing": 1, "kicking-power": 1, "physical-contact": 1}}, "basic_25": {"id": 25, "type": "basic", "name": "Shutdown +1", "bonus": {"defensive-awareness": 1, "defensive-engagement": 1, "ball-winning": 1, "speed": 1}}, "basic_26": {"id": 26, "type": "basic", "name": "Stealing +1", "bonus": {"ball-winning": 1, "aggression": 1, "acceleration": 1, "physical-contact": 1}}, "basic_27": {"id": 27, "type": "basic", "name": "Strength +1", "bonus": {"kicking-power": 1, "speed": 1, "physical-contact": 1, "jumping": 1}}, "basic_28": {"id": 28, "type": "basic", "name": "Striker's Instinct +1", "bonus": {"offensive-awareness": 1, "ball-control": 1, "finishing": 1, "acceleration": 1}}, "basic_29": {"id": 29, "type": "basic", "name": "Technique +1", "bonus": {"ball-control": 1, "tight-possession": 1, "dribbling": 1, "low-pass": 1}}, "std_0": {"id": 0, "type": "special", "name": "No Booster", "bonus": {}}, "std_16": {"id": 16, "type": "special", "name": "Bearer of Fate +4", "bonus": {"finishing": 4, "heading": 4, "speed": 4, "acceleration": 4}}, "std_17": {"id": 17, "type": "special", "name": "BLUE LOCK Agility +3", "bonus": {"speed": 3, "acceleration": 3, "balance": 3, "stamina": 3}}, "std_18": {"id": 18, "type": "special", "name": "BLUE LOCK Ball-carrying +3", "bonus": {"tight-possession": 3, "dribbling": 3, "speed": 3, "balance": 3}}, "std_19": {"id": 19, "type": "special", "name": "BLUE LOCK Fantasista +3", "bonus": {"ball-control": 3, "dribbling": 3, "finishing": 3, "balance": 3}}, "std_20": {"id": 20, "type": "special", "name": "BLUE LOCK Hard Worker +3", "bonus": {"aggression": 3, "acceleration": 3, "physical-contact": 3, "stamina": 3}}, "std_21": {"id": 21, "type": "special", "name": "BLUE LOCK Passing +3", "bonus": {"low-pass": 3, "lofted-pass": 3, "curl": 3, "kicking-power": 3}}, "std_22": {"id": 22, "type": "special", "name": "BLUE LOCK Physicality +3", "bonus": {"finishing": 3, "heading": 3, "physical-contact": 3, "jumping": 3}}, "std_23": {"id": 23, "type": "special", "name": "BLUE LOCK Physicality +3", "bonus": {"balance": 3, "physical-contact": 3, "jumping": 3, "stamina": 3}}, "std_24": {"id": 24, "type": "special", "name": "BLUE LOCK Shooting +3", "bonus": {"ball-control": 3, "finishing": 3, "kicking-power": 3, "physical-contact": 3}}, "std_25": {"id": 25, "type": "special", "name": "BLUE LOCK Striker's Instinct +3", "bonus": {"offensive-awareness": 3, "ball-control": 3, "finishing": 3, "acceleration": 3}}, "std_26": {"id": 26, "type": "special", "name": "BLUE LOCK Technique +3", "bonus": {"ball-control": 3, "tight-possession": 3, "dribbling": 3, "low-pass": 3}}, "std_45": {"id": 45, "type": "special", "name": "King of Football +4", "bonus": {"tight-possession": 4, "dribbling": 4, "balance": 4, "physical-contact": 4}}, "std_46": {"id": 46, "type": "special", "name": "Le Petit Prince +3", "bonus": {"offensive-awareness": 3, "low-pass": 3, "finishing": 3, "kicking-power": 3}}, "std_47": {"id": 47, "type": "special", "name": "Magical +4", "bonus": {"offensive-awareness": 4, "tight-possession": 4, "dribbling": 4, "acceleration": 4}}, "std_48": {"id": 48, "type": "special", "name": "Natural Born +4", "bonus": {"offensive-awareness": 4, "ball-control": 4, "dribbling": 4, "finishing": 4}}, "std_59": {"id": 59, "type": "special", "name": "Regista +2", "bonus": {"dribbling": 2, "low-pass": 2, "defensive-awareness": 2, "ball-winning": 2}}, "std_69": {"id": 69, "type": "special", "name": "Son of God +4", "bonus": {"dribbling": 4, "low-pass": 4, "finishing": 4, "kicking-power": 4}}, "std_77": {"id": 77, "type": "special", "name": "Striking +4", "bonus": {"offensive-awareness": 4, "kicking-power": 4, "acceleration": 4, "physical-contact": 4}}, "std_80": {"id": 80, "type": "special", "name": "The Undisputed +3", "bonus": {"offensive-awareness": 3, "ball-control": 3, "dribbling": 3, "physical-contact": 3}}, "std_81": {"id": 81, "type": "special", "name": "Total Package +1", "bonus": {"offensive-awareness": 1, "ball-control": 1, "tight-possession": 1, "dribbling": 1, "low-pass": 1, "lofted-pass": 1, "finishing": 1, "set-piece-taking": 1, "curl": 1, "heading": 1, "defensive-awareness": 1, "defensive-engagement": 1, "ball-winning": 1, "aggression": 1, "kicking-power": 1, "speed": 1, "acceleration": 1, "balance": 1, "physical-contact": 1, "jumping": 1, "gk-awareness": 1, "gk-catching": 1, "gk-clearing": 1, "gk-reflexes": 1, "gk-reach": 1, "stamina": 1}}};
  const ALL_MANAGERS = {"Cesc F\u00e0bregas_89": {"name": "Cesc F\u00e0bregas", "rating": 89, "booster_1": "lofted-pass", "booster_2": "defensive-engagement"}, "Cristian Chivu_89": {"name": "Cristian Chivu", "rating": 89, "booster_1": "speed", "booster_2": "stamina"}, "Didier Deschamps_88": {"name": "Didier Deschamps", "rating": 88, "booster_1": "speed", "booster_2": null}, "Dragan Stojkovic_88": {"name": "Dragan Stojkovic", "rating": 88, "booster_1": "kicking-power", "booster_2": null}, "Erik ten Hag_85": {"name": "Erik ten Hag", "rating": 85, "booster_1": "speed", "booster_2": null}, "Franz Beckenbauer_89": {"name": "Franz Beckenbauer", "rating": 89, "booster_1": "finishing", "booster_2": "kicking-power"}, "Fabio Capello_89": {"name": "Fabio Capello", "rating": 89, "booster_1": "finishing", "booster_2": "defensive-awareness"}, "Frank Lampard_88": {"name": "Frank Lampard", "rating": 88, "booster_1": "balance", "booster_2": null}, "Frank Rijkaard_88": {"name": "Frank Rijkaard", "rating": 88, "booster_1": "low-pass", "booster_2": null}, "Frank Rijkaard_89": {"name": "Frank Rijkaard", "rating": 89, "booster_1": "tight-possession", "booster_2": "balance"}, "Gian Piero Gasperini_88": {"name": "Gian Piero Gasperini", "rating": 88, "booster_1": "physical-contact", "booster_2": null}, "Gareth Southgate_87": {"name": "Gareth Southgate", "rating": 87, "booster_1": "acceleration", "booster_2": null}, "Gennaro Gattuso_89": {"name": "Gennaro Gattuso", "rating": 89, "booster_1": "ball-winning", "booster_2": "lofted-pass"}, "Pep Guardiola_88": {"name": "Pep Guardiola", "rating": 88, "booster_1": "tight-possession", "booster_2": null}, "Pep Guardiola_89": {"name": "Pep Guardiola", "rating": 89, "booster_1": "kicking-power", "booster_2": "aggression"}, "Hansi Flick_88": {"name": "Hansi Flick", "rating": 88, "booster_1": "low-pass", "booster_2": "speed"}, "Hansi Flick_89": {"name": "Hansi Flick", "rating": 89, "booster_1": "low-pass", "booster_2": "dribbling"}, "Johan Cruyff_89": {"name": "Johan Cruyff", "rating": 89, "booster_1": "tight-possession", "booster_2": "jumping"}, "Jos\u00e9 Mourinho_88": {"name": "Jos\u00e9 Mourinho", "rating": 88, "booster_1": "ball-winning", "booster_2": null}, "Jos\u00e9 Mourinho_89": {"name": "Jos\u00e9 Mourinho", "rating": 89, "booster_1": "physical-contact", "booster_2": "stamina"}, "J\u00fcrgen Klopp_89": {"name": "J\u00fcrgen Klopp", "rating": 89, "booster_1": "speed", "booster_2": "aggression"}, "Luis de la Fuente_88": {"name": "Luis de la Fuente", "rating": 88, "booster_1": "acceleration", "booster_2": null}, "Lionel Scaloni_87": {"name": "Lionel Scaloni", "rating": 87, "booster_1": "low-pass", "booster_2": null}, "Luciano Spalletti_89": {"name": "Luciano Spalletti", "rating": 89, "booster_1": "defensive-awareness", "booster_2": "stamina"}, "Massimiliano Allegri_89": {"name": "Massimiliano Allegri", "rating": 89, "booster_1": "offensive-awareness", "booster_2": "ball-control"}, "Mikel Arteta_85": {"name": "Mikel Arteta", "rating": 85, "booster_1": "low-pass", "booster_2": null}, "Mikel Arteta_88": {"name": "Mikel Arteta", "rating": 88, "booster_1": "acceleration", "booster_2": "tight-possession"}, "Mikel Arteta_89": {"name": "Mikel Arteta", "rating": 89, "booster_1": "acceleration", "booster_2": "set-piece-taking"}, "Niko Kova\u010d_89": {"name": "Niko Kova\u010d", "rating": 89, "booster_1": "kicking-power", "booster_2": "tight-possession"}, "Olimpio Henriques_87": {"name": "Olimpio Henriques", "rating": 87, "booster_1": "kicking-power", "booster_2": null}, "Okan Buruk_89": {"name": "Okan Buruk", "rating": 89, "booster_1": "balance", "booster_2": "lofted-pass"}, "Patrick Kluivert_89": {"name": "Patrick Kluivert", "rating": 89, "booster_1": "low-pass", "booster_2": "defensive-engagement"}, "Patrick Vieira_88": {"name": "Patrick Vieira", "rating": 88, "booster_1": "ball-winning", "booster_2": null}, "Paulo Fonseca_88": {"name": "Paulo Fonseca", "rating": 88, "booster_1": "physical-contact", "booster_2": "kicking-power"}, "Roberto Mart\u00ednez_88": {"name": "Roberto Mart\u00ednez", "rating": 88, "booster_1": "physical-contact", "booster_2": null}, "Roberto Mart\u00ednez_89": {"name": "Roberto Mart\u00ednez", "rating": 89, "booster_1": "ball-control", "booster_2": "aggression"}, "Ronald Koeman_88": {"name": "Ronald Koeman", "rating": 88, "booster_1": "ball-control", "booster_2": "stamina"}, "Ronald Koeman_89": {"name": "Ronald Koeman", "rating": 89, "booster_1": "low-pass", "booster_2": "heading"}, "R\u00faben Amorim_88": {"name": "R\u00faben Amorim", "rating": 88, "booster_1": "speed", "booster_2": "offensive-awareness"}, "R\u00faben Amorim_89": {"name": "R\u00faben Amorim", "rating": 89, "booster_1": "physical-contact", "booster_2": "lofted-pass"}, "Rudi Garcia_89": {"name": "Rudi Garcia", "rating": 89, "booster_1": "ball-control", "booster_2": "offensive-awareness"}, "Simone Inzaghi_85": {"name": "Simone Inzaghi", "rating": 85, "booster_1": "stamina", "booster_2": null}, "Simone Inzaghi_88": {"name": "Simone Inzaghi", "rating": 88, "booster_1": "speed", "booster_2": "finishing"}, "St\u00e5le Solbakken_89": {"name": "St\u00e5le Solbakken", "rating": 89, "booster_1": "heading", "booster_2": "physical-contact"}, "Stefano Pioli_85": {"name": "Stefano Pioli", "rating": 85, "booster_1": "ball-winning", "booster_2": null}, "Steven Gerrard_88": {"name": "Steven Gerrard", "rating": 88, "booster_1": "dribbling", "booster_2": null}, "Thomas Tuchel_85": {"name": "Thomas Tuchel", "rating": 85, "booster_1": "physical-contact", "booster_2": null}, "Vincenzo Montella_88": {"name": "Vincenzo Montella", "rating": 88, "booster_1": "stamina", "booster_2": null}, "Vincent Kompany_88": {"name": "Vincent Kompany", "rating": 88, "booster_1": "acceleration", "booster_2": "kicking-power"}, "Xabi Alonso_88": {"name": "Xabi Alonso", "rating": 88, "booster_1": "acceleration", "booster_2": null}, "Xabi Alonso_89": {"name": "Xabi Alonso", "rating": 89, "booster_1": "ball-control", "booster_2": "finishing"}, "Xavi_85": {"name": "Xavi", "rating": 85, "booster_1": "ball-control", "booster_2": null}, "Zico_83": {"name": "Zico", "rating": 83, "booster_1": "dribbling", "booster_2": null}, "Roberto Mart\u00ednez_90": {"name": "Roberto Mart\u00ednez", "rating": 90, "booster_1": "finishing", "booster_2": "offensive-awareness"}, "Thomas Tuchel_90": {"name": "Thomas Tuchel", "rating": 90, "booster_1": "low-pass", "booster_2": "stamina"}, "Didier Deschamps_89": {"name": "Didier Deschamps", "rating": 89, "booster_1": "speed", "booster_2": "ball-control"}, "Vincenzo Montella_89": {"name": "Vincenzo Montella", "rating": 89, "booster_1": "acceleration", "booster_2": "tight-possession"}, "Julian Nagelsmann_89": {"name": "Julian Nagelsmann", "rating": 89, "booster_1": "dribbling", "booster_2": "physical-contact"}, "Antonio Conte_90": {"name": "Antonio Conte", "rating": 90, "booster_1": "defensive-awareness", "booster_2": "kicking-power"}, "Frank Lampard_89": {"name": "Frank Lampard", "rating": 89, "booster_1": "low-pass", "booster_2": "defensive-engagement"}, "Jos\u00e9 Mourinho_90": {"name": "Jos\u00e9 Mourinho", "rating": 90, "booster_1": "finishing", "booster_2": "aggression"}, "Makino Tomoaki_88": {"name": "Makino Tomoaki", "rating": 88, "booster_1": "ball-winning", "booster_2": "offensive-awareness"}, "R. Rodr\u00edguez_88": {"name": "R. Rodr\u00edguez", "rating": 88, "booster_1": "tight-possession", "booster_2": "balance"}, "Cristian Chivu_90": {"name": "Cristian Chivu", "rating": 90, "booster_1": "low-pass", "booster_2": "lofted-pass"}, "J\u00fcrgen Klopp_90": {"name": "J\u00fcrgen Klopp", "rating": 90, "booster_1": "defensive-awareness", "booster_2": "stamina"}};

  const OFFLINE_PLAYER_DB = {
    // Jude Bellingham (Big Time / Show Time / Standard / Alias 4)
    "89138556700485": {
      name: "Jude Bellingham", position: "AMF", level_limit: 32,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },
    "106732621793093": {
      name: "Jude Bellingham", position: "AMF", level_limit: 35,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },
    "106752486016837": {
      name: "Jude Bellingham", position: "AMF", level_limit: 36,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },
    "106782550787909": {
      name: "Jude Bellingham", position: "AMF", level_limit: 35,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },
    "132933": {
      name: "Jude Bellingham", position: "AMF", level_limit: 32,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },
    "52853062371141": {
      name: "Jude Bellingham", position: "AMF", level_limit: 32,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },
    "4": {
      name: "Jude Bellingham", position: "AMF", level_limit: 32,
      play_style: [6, 27],
      stats: { "offensive-awareness": 90, "ball-control": 93, "dribbling": 93, "tight-possession": 94, "low-pass": 92, "lofted-pass": 87, "finishing": 89, "heading": 66, "set-piece-taking": 68, "curl": 73, "speed": 93, "acceleration": 90, "kicking-power": 92, "jumping": 71, "physical-contact": 84, "balance": 86, "stamina": 95, "defensive-awareness": 75, "defensive-engagement": 80, "ball-winning": 73, "aggression": 80, "gk-awareness": 41, "gk-catching": 41, "gk-clearing": 41, "gk-reflexes": 41, "gk-reach": 41 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 97: 1, 107: 1, 109: 1, 110: 1, 130: 1, 133: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },

    // A. Costacurta (AC Milan Epic)
    "88032334055527": {
      name: "A. Costacurta", position: "LB", level_limit: 37,
      play_style: [12, 29],
      stats: { "offensive-awareness": 62, "ball-control": 76, "dribbling": 72, "tight-possession": 71, "low-pass": 78, "lofted-pass": 78, "finishing": 56, "heading": 80, "set-piece-taking": 58, "curl": 66, "speed": 73, "acceleration": 70, "kicking-power": 73, "jumping": 73, "physical-contact": 80, "balance": 70, "stamina": 83, "defensive-awareness": 78, "defensive-engagement": 86, "ball-winning": 76, "aggression": 73, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 76: 1, 92: 1, 94: 1, 97: 1, 106: 1, 111: 1, 112: 1, 113: 1, 115: 1, 125: 1 },
      com_styles: {}
    },
    "88036360587367": {
      name: "A. Costacurta", position: "RB", level_limit: 35,
      play_style: [12, 29],
      stats: { "offensive-awareness": 62, "ball-control": 76, "dribbling": 72, "tight-possession": 71, "low-pass": 78, "lofted-pass": 78, "finishing": 56, "heading": 80, "set-piece-taking": 58, "curl": 66, "speed": 74, "acceleration": 72, "kicking-power": 73, "jumping": 73, "physical-contact": 80, "balance": 70, "stamina": 84, "defensive-awareness": 78, "defensive-engagement": 86, "ball-winning": 76, "aggression": 73, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 76: 1, 92: 1, 94: 1, 97: 1, 106: 1, 111: 1, 112: 1, 113: 1, 115: 1, 125: 1 },
      com_styles: {}
    },

    // Lionel Messi
    "88032602496343": {
      name: "Lionel Messi", position: "SS", level_limit: 35,
      play_style: [14, 26],
      stats: { "offensive-awareness": 93, "ball-control": 95, "dribbling": 96, "tight-possession": 96, "low-pass": 89, "lofted-pass": 86, "finishing": 92, "heading": 64, "set-piece-taking": 92, "curl": 94, "speed": 85, "acceleration": 91, "kicking-power": 88, "jumping": 66, "physical-contact": 72, "balance": 96, "stamina": 80, "defensive-awareness": 48, "defensive-engagement": 55, "ball-winning": 45, "aggression": 52, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 101: 1, 107: 1, 110: 1, 116: 1, 120: 1, 130: 1 },
      com_styles: { 63: 1, 64: 1, 65: 1, 66: 1, 68: 1 }
    },
    "89062052470103": {
      name: "Lionel Messi", position: "SS", level_limit: 35,
      play_style: [14, 26],
      stats: { "offensive-awareness": 93, "ball-control": 95, "dribbling": 96, "tight-possession": 96, "low-pass": 89, "lofted-pass": 86, "finishing": 92, "heading": 64, "set-piece-taking": 92, "curl": 94, "speed": 85, "acceleration": 91, "kicking-power": 88, "jumping": 66, "physical-contact": 72, "balance": 96, "stamina": 80, "defensive-awareness": 48, "defensive-engagement": 55, "ball-winning": 45, "aggression": 52, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 101: 1, 107: 1, 110: 1, 116: 1, 120: 1, 130: 1 },
      com_styles: { 63: 1, 64: 1, 65: 1, 66: 1, 68: 1 }
    },
    "89138556575063": {
      name: "Lionel Messi", position: "SS", level_limit: 35,
      play_style: [14, 26],
      stats: { "offensive-awareness": 93, "ball-control": 95, "dribbling": 96, "tight-possession": 96, "low-pass": 89, "lofted-pass": 86, "finishing": 92, "heading": 64, "set-piece-taking": 92, "curl": 94, "speed": 85, "acceleration": 91, "kicking-power": 88, "jumping": 66, "physical-contact": 72, "balance": 96, "stamina": 80, "defensive-awareness": 48, "defensive-engagement": 55, "ball-winning": 45, "aggression": 52, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 77: 1, 81: 1, 82: 1, 85: 1, 101: 1, 107: 1, 110: 1, 116: 1, 120: 1, 130: 1 },
      com_styles: { 63: 1, 64: 1, 65: 1, 66: 1, 68: 1 }
    },

    // Kylian Mbappé
    "89068226588798": {
      name: "Kylian Mbappé", position: "CF", level_limit: 34,
      play_style: [1, 26],
      stats: { "offensive-awareness": 91, "ball-control": 88, "dribbling": 91, "tight-possession": 88, "low-pass": 80, "lofted-pass": 72, "finishing": 90, "heading": 72, "set-piece-taking": 70, "curl": 80, "speed": 98, "acceleration": 97, "kicking-power": 89, "jumping": 76, "physical-contact": 76, "balance": 88, "stamina": 85, "defensive-awareness": 45, "defensive-engagement": 52, "ball-winning": 46, "aggression": 55, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 70: 1, 77: 1, 81: 1, 82: 1, 85: 1, 107: 1, 109: 1, 120: 1, 124: 1, 135: 1 },
      com_styles: { 63: 1, 64: 1, 65: 1, 66: 1 }
    },
    "89138556678270": {
      name: "Kylian Mbappé", position: "CF", level_limit: 34,
      play_style: [1, 26],
      stats: { "offensive-awareness": 91, "ball-control": 88, "dribbling": 91, "tight-possession": 88, "low-pass": 80, "lofted-pass": 72, "finishing": 90, "heading": 72, "set-piece-taking": 70, "curl": 80, "speed": 98, "acceleration": 97, "kicking-power": 89, "jumping": 76, "physical-contact": 76, "balance": 88, "stamina": 85, "defensive-awareness": 45, "defensive-engagement": 52, "ball-winning": 46, "aggression": 55, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 70: 1, 77: 1, 81: 1, 82: 1, 85: 1, 107: 1, 109: 1, 120: 1, 124: 1, 135: 1 },
      com_styles: { 63: 1, 64: 1, 65: 1, 66: 1 }
    },

    // Erling Haaland
    "106733158664615": {
      name: "Erling Haaland", position: "CF", level_limit: 32,
      play_style: [1, 26],
      stats: { "offensive-awareness": 94, "ball-control": 81, "dribbling": 80, "tight-possession": 78, "low-pass": 70, "lofted-pass": 65, "finishing": 95, "heading": 87, "set-piece-taking": 68, "curl": 72, "speed": 91, "acceleration": 87, "kicking-power": 96, "jumping": 88, "physical-contact": 93, "balance": 76, "stamina": 83, "defensive-awareness": 48, "defensive-engagement": 60, "ball-winning": 52, "aggression": 65, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 76: 1, 79: 1, 81: 1, 85: 1, 97: 1, 103: 1, 104: 1, 112: 1, 114: 1, 121: 1 },
      com_styles: { 64: 1, 65: 1, 69: 1 }
    },
    "89138556701095": {
      name: "Erling Haaland", position: "CF", level_limit: 32,
      play_style: [1, 26],
      stats: { "offensive-awareness": 94, "ball-control": 81, "dribbling": 80, "tight-possession": 78, "low-pass": 70, "lofted-pass": 65, "finishing": 95, "heading": 87, "set-piece-taking": 68, "curl": 72, "speed": 91, "acceleration": 87, "kicking-power": 96, "jumping": 88, "physical-contact": 93, "balance": 76, "stamina": 83, "defensive-awareness": 48, "defensive-engagement": 60, "ball-winning": 52, "aggression": 65, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 76: 1, 79: 1, 81: 1, 85: 1, 97: 1, 103: 1, 104: 1, 112: 1, 114: 1, 121: 1 },
      com_styles: { 64: 1, 65: 1, 69: 1 }
    },

    // Neymar Jr
    "89071179308448": {
      name: "Neymar Jr", position: "LWF", level_limit: 35,
      play_style: [14, 26],
      stats: { "offensive-awareness": 88, "ball-control": 95, "dribbling": 96, "tight-possession": 95, "low-pass": 84, "lofted-pass": 80, "finishing": 86, "heading": 62, "set-piece-taking": 87, "curl": 88, "speed": 86, "acceleration": 90, "kicking-power": 82, "jumping": 68, "physical-contact": 64, "balance": 92, "stamina": 77, "defensive-awareness": 42, "defensive-engagement": 48, "ball-winning": 44, "aggression": 52, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 70: 1, 71: 1, 77: 1, 81: 1, 85: 1, 86: 1, 102: 1, 107: 1, 116: 1, 130: 1 },
      com_styles: { 63: 1, 64: 1, 65: 1, 66: 1, 67: 1 }
    },

    // Gianluigi Buffon
    "88040387118039": {
      name: "Gianluigi Buffon", position: "GK", level_limit: 35,
      play_style: [17, 32],
      stats: { "offensive-awareness": 43, "ball-control": 47, "dribbling": 47, "tight-possession": 45, "low-pass": 57, "lofted-pass": 59, "finishing": 43, "heading": 50, "set-piece-taking": 46, "curl": 49, "speed": 51, "acceleration": 51, "kicking-power": 68, "jumping": 70, "physical-contact": 64, "balance": 51, "stamina": 52, "defensive-awareness": 46, "defensive-engagement": 47, "ball-winning": 45, "aggression": 47, "gk-awareness": 82, "gk-catching": 80, "gk-clearing": 80, "gk-reflexes": 81, "gk-reach": 84 },
      skills: { 90: 1, 95: 1, 100: 1, 105: 1, 126: 1, 129: 1 },
      com_styles: {}
    },

    // David Villa
    "88029649706260": {
      name: "David Villa", position: "LWF", level_limit: 34,
      play_style: [1, 26],
      stats: { "offensive-awareness": 83, "ball-control": 77, "dribbling": 82, "tight-possession": 73, "low-pass": 70, "lofted-pass": 67, "finishing": 85, "heading": 70, "set-piece-taking": 72, "curl": 73, "speed": 75, "acceleration": 74, "kicking-power": 84, "jumping": 73, "physical-contact": 66, "balance": 72, "stamina": 75, "defensive-awareness": 45, "defensive-engagement": 62, "ball-winning": 47, "aggression": 58, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 77: 1, 78: 1, 79: 1, 81: 1, 82: 1, 85: 1, 99: 1, 101: 1, 109: 1, 114: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 69: 1 }
    },

    // Marcelo
    "88041997764700": {
      name: "Marcelo", position: "LB", level_limit: 34,
      play_style: [11, 27],
      stats: { "offensive-awareness": 72, "ball-control": 81, "dribbling": 77, "tight-possession": 80, "low-pass": 78, "lofted-pass": 83, "finishing": 68, "heading": 63, "set-piece-taking": 67, "curl": 77, "speed": 77, "acceleration": 81, "kicking-power": 78, "jumping": 70, "physical-contact": 69, "balance": 80, "stamina": 81, "defensive-awareness": 72, "defensive-engagement": 70, "ball-winning": 75, "aggression": 76, "gk-awareness": 40, "gk-catching": 40, "gk-clearing": 40, "gk-reflexes": 40, "gk-reach": 40 },
      skills: { 70: 1, 72: 1, 73: 1, 82: 1, 84: 1, 85: 1, 86: 1, 102: 1, 107: 1, 110: 1 },
      com_styles: { 63: 1, 64: 1, 66: 1, 68: 1 }
    }
  };

  const STAT_MAP = {
    'offensive-awareness': 14, 'ball-control': 17, 'dribbling': 18, 'tight-possession': 19,
    'low-pass': 21, 'lofted-pass': 22, 'finishing': 20, 'heading': 23, 'set-piece-taking': 27,
    'curl': 28, 'speed': 34, 'acceleration': 38, 'kicking-power': 37, 'jumping': 39,
    'physical-contact': 35, 'balance': 36, 'stamina': 40, 'defensive-awareness': 15,
    'defensive-engagement': 26, 'ball-winning': 24, 'aggression': 25, 'gk-awareness': 16,
    'gk-catching': 29, 'gk-clearing': 30, 'gk-reflexes': 31, 'gk-reach': 32
  };

  const TRAINING_CATEGORIES = {
    outfield: {
      shooting: ['finishing', 'set-piece-taking', 'curl'],
      passing: ['low-pass', 'lofted-pass'],
      dribbling: ['ball-control', 'dribbling', 'tight-possession'],
      dexterity: ['offensive-awareness', 'acceleration', 'balance'],
      lower_body: ['speed', 'kicking-power', 'stamina'],
      aerial: ['heading', 'jumping', 'physical-contact'],
      defending: ['defensive-awareness', 'ball-winning', 'defensive-engagement', 'aggression']
    },
    gk: {
      gk1: ['gk-awareness', 'jumping'],
      gk2: ['gk-catching', 'gk-reach'],
      gk3: ['gk-clearing', 'gk-reflexes'],
      aerial: ['heading', 'jumping', 'physical-contact']
    }
  };

  const POSITION_CALIBRATION = {
    'CF':  { weights: { 'fin': 18, 'oa': 16, 'acc': 14, 'spd': 10, 'kp': 8, 'hea': 7, 'phy': 7, 'bal': 6, 'bc': 5, 'drb': 5, 'jmp': 4 }, bonus: 7.0 },
    'SS':  { weights: { 'oa': 16, 'bc': 14, 'drb': 14, 'fin': 14, 'acc': 12, 'tp': 8, 'lp': 8, 'spd': 7, 'bal': 7 }, bonus: 7.0 },
    'LWF': { weights: { 'spd': 16, 'acc': 14, 'drb': 15, 'bc': 13, 'tp': 10, 'fin': 10, 'oa': 8, 'bal': 6, 'lp': 4, 'cur': 4 }, bonus: 7.5 },
    'RWF': { weights: { 'spd': 16, 'acc': 14, 'drb': 15, 'bc': 13, 'tp': 10, 'fin': 10, 'oa': 8, 'bal': 6, 'lp': 4, 'cur': 4 }, bonus: 7.5 },
    'AMF': { weights: { 'lp': 15, 'bc': 14, 'drb': 12, 'tp': 12, 'oa': 12, 'acc': 9, 'spd': 8, 'fin': 8, 'lop': 5, 'sta': 5 }, bonus: 7.0 },
    'CMF': { weights: { 'lp': 15, 'lop': 12, 'bc': 12, 'sta': 12, 'tp': 9, 'da': 9, 'bw': 9, 'kp': 7, 'spd': 7, 'acc': 7 }, bonus: 6.5 },
    'DMF': { weights: { 'bw': 17, 'da': 17, 'sta': 13, 'phy': 12, 'lp': 11, 'de': 10, 'agg': 8, 'spd': 6, 'acc': 6 }, bonus: 6.5 },
    'CB':  { weights: { 'da': 20, 'bw': 19, 'phy': 14, 'jmp': 12, 'hea': 10, 'de': 8, 'agg': 7, 'spd': 5, 'acc': 5 }, bonus: 6.0 },
    'LB':  { weights: { 'spd': 14, 'acc': 12, 'sta': 12, 'da': 11, 'bw': 11, 'lop': 10, 'lp': 8, 'de': 8, 'bc': 5, 'drb': 5, 'bal': 4 }, bonus: 7.5 },
    'RB':  { weights: { 'spd': 14, 'acc': 12, 'sta': 12, 'da': 11, 'bw': 11, 'lop': 10, 'lp': 8, 'de': 8, 'bc': 5, 'drb': 5, 'bal': 4 }, bonus: 7.5 },
    'LMF': { weights: { 'spd': 16, 'acc': 14, 'drb': 13, 'bc': 12, 'sta': 11, 'lop': 10, 'lp': 9, 'cur': 6, 'oa': 5, 'fin': 4 }, bonus: 7.0 },
    'RMF': { weights: { 'spd': 16, 'acc': 14, 'drb': 13, 'bc': 12, 'sta': 11, 'lop': 10, 'lp': 9, 'cur': 6, 'oa': 5, 'fin': 4 }, bonus: 7.0 },
    'GK':  { weights: { 'gkre': 22, 'gkr': 21, 'gkc': 20, 'gka': 18, 'gkcl': 11, 'jmp': 5, 'phy': 3 }, bonus: 5.5 }
  };

  const STAT_SHORT = {
    'offensive-awareness': 'oa', 'ball-control': 'bc', 'dribbling': 'drb', 'tight-possession': 'tp',
    'low-pass': 'lp', 'lofted-pass': 'lop', 'finishing': 'fin', 'heading': 'hea',
    'set-piece-taking': 'spt', 'curl': 'cur', 'speed': 'spd', 'acceleration': 'acc',
    'kicking-power': 'kp', 'jumping': 'jmp', 'physical-contact': 'phy', 'balance': 'bal',
    'stamina': 'sta', 'defensive-awareness': 'da', 'defensive-engagement': 'de',
    'ball-winning': 'bw', 'aggression': 'agg', 'gk-awareness': 'gka', 'gk-catching': 'gkc',
    'gk-clearing': 'gkcl', 'gk-reflexes': 'gkre', 'gk-reach': 'gkr'
  };

  const eFHub = {
    STAT_MAP,
    TRAINING_CATEGORIES,
    POSITION_CALIBRATION,
    ALL_BOOSTERS,
    ALL_MANAGERS,

    getPositionGroup(pos) {
      if (['CF', 'SS', 'LWF', 'RWF'].includes(pos)) return 'fw';
      if (['AMF', 'CMF', 'DMF', 'LMF', 'RMF'].includes(pos)) return 'mf';
      if (['CB', 'LB', 'RB'].includes(pos)) return 'df';
      if (pos === 'GK') return 'gk';
      return 'mf';
    },

    getCategoryPointCost(level) {
      let cost = 0;
      for (let i = 0; i < level; i++) cost += Math.floor(i / 4) + 1;
      return cost;
    },

    getMaxProgressionPoints(levelLimit) {
      return Math.max(0, (Number(levelLimit || 1) - 1) * 2);
    },

    getManagerBoost(rawStat, managerRating) {
      const r = Number(managerRating || 88);
      if (r <= 0) return 0;
      if (r >= 88) {
        if (rawStat >= 85) return 3;
        if (rawStat >= 59) return 2;
        return 1;
      } else if (r >= 85) {
        if (rawStat >= 93) return 3;
        if (rawStat >= 62) return 2;
        return 1;
      }
      return rawStat >= 70 ? 2 : 1;
    },

    calculateContinuousOvr(stats, position) {
      const pos = position || 'CF';
      const cfg = POSITION_CALIBRATION[pos] || POSITION_CALIBRATION['CF'];
      let totalW = 0, sum = 0;
      for (const [shortKey, weight] of Object.entries(cfg.weights)) {
        for (const [fullK, sK] of Object.entries(STAT_SHORT)) {
          if (sK === shortKey) {
            const val = Number(stats[fullK]) || 40;
            sum += val * weight;
            totalW += weight;
            break;
          }
        }
      }
      if (totalW === 0) return 80;
      return (sum / totalW) + cfg.bonus;
    },

    calculateOvr(stats, position) {
      return Math.round(this.calculateContinuousOvr(stats, position));
    },

    computeEffectiveStats(record, trainingAllocations, boosterKey, managerKey) {
      const p = record.parameters || [];
      const isGK = Number(p[0]) === 0; // In MAIN_POS, 0 is GK!
      const categories = isGK ? TRAINING_CATEGORIES.gk : TRAINING_CATEGORIES.outfield;
      const allocs = trainingAllocations || {};

      const booster = ALL_BOOSTERS[boosterKey] || { bonus: {} };
      const boosterBonus = booster.bonus || {};

      const manager = ALL_MANAGERS[managerKey] || { rating: 88 };
      const mgrRating = manager.rating || 88;
      const mgrB1 = manager.booster_1;
      const mgrB2 = manager.booster_2;

      const stats = {};
      const breakdowns = {};

      for (const [key, idx] of Object.entries(STAT_MAP)) {
        const base = Number(p[idx]) || 40;
        let prog = 0;
        for (const [catName, affected] of Object.entries(categories)) {
          if (affected.includes(key)) {
            prog += Number(allocs[catName] || 0);
          }
        }
        const bst = Number(boosterBonus[key] || 0);
        let mgrExtra = 0;
        if (mgrB1 === key) mgrExtra += 1;
        if (mgrB2 === key) mgrExtra += 1;

        const preMgr = base + prog + bst;
        const mgrProf = this.getManagerBoost(preMgr, mgrRating);
        const finalVal = preMgr + mgrProf + mgrExtra;

        stats[key] = finalVal;
        breakdowns[key] = {
          base,
          progressionAdd: prog,
          boosterAdd: bst,
          managerAdd: mgrProf + mgrExtra,
          finalStat: finalVal
        };
      }

      return { stats, breakdowns };
    },

    autoAllocate(record, position) {
      const maxPts = this.getMaxProgressionPoints(record.level_info?.level_limit || 1);
      const isGK = Number(record.parameters?.[0]) === 0;
      const categories = Object.keys(isGK ? TRAINING_CATEGORIES.gk : TRAINING_CATEGORIES.outfield);
      const allocs = {};
      categories.forEach(c => allocs[c] = 0);

      let pointsSpent = 0;

      while (true) {
        let bestCat = null;
        let bestScore = -1;

        for (const cat of categories) {
          const cur = allocs[cat];
          const nextCost = Math.floor(cur / 4) + 1;
          if (pointsSpent + nextCost <= maxPts && cur < 16) {
            allocs[cat] = cur + 1;
            const res = this.computeEffectiveStats(record, allocs, 'none', 'Pep Guardiola_88');
            const score = this.calculateContinuousOvr(res.stats, position);
            if (score > bestScore) {
              bestScore = score;
              bestCat = cat;
            }
            allocs[cat] = cur;
          }
        }

        if (!bestCat) break;
        allocs[bestCat]++;
        pointsSpent += Math.floor((allocs[bestCat] - 1) / 4) + 1;
      }

      return allocs;
    }
  };

  window.eFHub = eFHub;

  let activeBuilds = {};

  function renderCardProfile(id, targetContainer) {
    ziruEnsure(id);
    const record = catalog.records[id];
    if (!record || !Array.isArray(record.parameters)) {
      const liveRecord = livePlayer(id);
      if (Array.isArray(liveRecord)) {
        currentId = id;
        const [name, height, weight, nationality, positionId] = liveRecord;
        const position = MAIN_POS[Number(positionId)] || "—";
        targetContainer.innerHTML = `
          <article class="pl-card">
            <div class="pl-top">${playerCardImage(id, "large")}<div class="pl-top-info"><div class="eyebrow">ID ${id}</div><h2>${esc(name)}</h2></div><b class="pl-pos">${esc(position)}</b></div>
            <div class="pl-grid">
              <div><span>${L("Ana pozisyon", "Main position")}</span><strong>${esc(position)} · ${esc(posName(position))}</strong></div>
              <div><span>${L("Ülke ID’si", "Country ID")}</span><strong>${nationality}</strong></div>
              <div><span>${L("Boy (kaynak değeri)", "Height (source value)")}</span><strong>${height}</strong></div>
              <div><span>${L("Kilo (kaynak değeri)", "Weight (source value)")}</span><strong>${weight}</strong></div>
            </div>
            <div class="pl-row"><span>${L("Oyuncu bilgisi", "Player data")}</span><div class="pl-packs">${L("Varsayılan canlı veriden temel profil", "Basic profile from default live data")}</div></div>
            <div class="pl-actions pl-player-actions">
              <button class="button button-secondary" id="pl-add-live-squad" type="button">＋ ${L("Kadroya ekle", "Add to squad")}</button>
              <button class="button button-primary" id="pl-edit-live" type="button">✎ ${L("Oyuncuyu düzenle", "Edit player")}</button>
            </div>
          </article>`;
        loadPlayerCardImages(targetContainer);
        targetContainer.querySelector("#pl-add-live-squad")?.addEventListener("click", () => addPlayerToSquad(id));
        targetContainer.querySelector("#pl-edit-live")?.addEventListener("click", () => window.openPlayerEditor(Number(id)));
        return;
      }
      currentId = null;
      targetContainer.innerHTML = `<div style="padding:20px;text-align:center;color:var(--muted)">${L("Bu ID için oyuncu verisi bulunamadı.", "No player data was found for this ID.")}</div>`;
      return;
    }

    currentId = id;
    const meta = playerInfo(Number(id));
    const p = record.parameters;
    const mainIdx = Number(p[0]) || 0;
    const mainPos = MAIN_POS[mainIdx] || meta.position || "CF";
    const posGroup = eFHub.getPositionGroup(mainPos);
    const lv = record.level_info || {};
    const maxLvl = Math.max(1, Number(lv.level_limit || 1));
    const totalMaxPts = eFHub.getMaxProgressionPoints(maxLvl);

    if (!activeBuilds[id]) {
      const initB = (record.initial_booster_id_list && record.initial_booster_id_list[0]) || 0;
      let initKey = 'none';
      if (initB > 0) {
        initKey = eFHub.ALL_BOOSTERS[`basic_${initB}`] ? `basic_${initB}` : (eFHub.ALL_BOOSTERS[`std_${initB}`] ? `std_${initB}` : 'none');
      }
      activeBuilds[id] = {
        level: maxLvl,
        boosterKey: initKey,
        managerKey: 'Pep Guardiola_88',
        allocations: {}
      };
    }
    const bld = activeBuilds[id];

    const { stats, breakdowns } = eFHub.computeEffectiveStats(record, bld.allocations, bld.boosterKey, bld.managerKey);
    const currentOvr = eFHub.calculateOvr(stats, mainPos);

    let usedPts = 0;
    for (const pts of Object.values(bld.allocations)) {
      usedPts += eFHub.getCategoryPointCost(pts);
    }
    const remPts = Math.max(0, totalMaxPts - usedPts);

    const A = [], B = [];
    Object.entries(SLOT_POS).forEach(([slot, pos]) => { if (p[slot] === 2) A.push(pos); else if (p[slot] === 1) B.push(pos); });
    const left = Number(p[43]) === 1;
    const strong = left ? L("Sol", "Left") : L("Sağ", "Right");
    const weak = left ? L("Sağ", "Right") : L("Sol", "Left");
    const packs = packsOf(id);
    const playStyleIdAtk = Number((record.play_style || [])[0] ?? p[61] ?? 0);
    const playStyleIdDef = Number((record.play_style || [])[1] ?? 0);
    const skills = playerSkillLabels(p);
    const comStyles = comPlayingStyleLabels(p);

    const playerStatsList = [
      ["offensive-awareness", 14], ["ball-control", 17],
      ["dribbling", 18], ["tight-possession", 19],
      ["low-pass", 21], ["lofted-pass", 22],
      ["finishing", 20], ["heading", 23],
      ["set-piece-taking", 27], ["curl", 28],
      ["speed", 34], ["acceleration", 38],
      ["kicking-power", 37], ["jumping", 39],
      ["physical-contact", 35], ["balance", 36],
      ["stamina", 40], ["defensive-awareness", 15],
      ["defensive-engagement", 26], ["ball-winning", 24],
      ["aggression", 25], ["gk-awareness", 16],
      ["gk-catching", 29], ["gk-clearing", 30],
      ["gk-reflexes", 31], ["gk-reach", 32]
    ];

    const statsHtml = playerStatsList.map(([key, idx]) => {
      const b = breakdowns[key] || { base: Number(p[idx]) || 0, finalStat: Number(p[idx]) || 0, progressionAdd: 0, boosterAdd: 0, managerAdd: 0 };
      const v = b.finalStat;
      const barClass = v >= 90 ? "pl-stat-90" : (v >= 80 ? "pl-stat-80" : (v >= 70 ? "pl-stat-70" : "pl-stat-low"));
      return `
        <div class="pl-stat ${barClass}">
          <span>${esc(t(key))}</span>
          <div class="pl-stat-vals">
            <b>${v}</b>
            ${b.progressionAdd > 0 ? `<small class="pl-stat-prog">+${b.progressionAdd}</small>` : ""}
            ${b.boosterAdd > 0 ? `<small class="pl-stat-boost">+${b.boosterAdd}</small>` : ""}
            ${b.managerAdd > 0 ? `<small class="pl-stat-mgr">+${b.managerAdd}</small>` : ""}
          </div>
          <i style="width:${Math.min(v, 105)}%"></i>
        </div>`;
    }).join("");

    const chips = (arr) => arr.length ? arr.map((x) => `<em title="${esc(posName(x))}">${x}</em>`).join("") : "—";

    let bstOptions = `<option value="none" ${bld.boosterKey === 'none' ? 'selected' : ''}>Tanpa Booster (0)</option>`;
    bstOptions += `<optgroup label="Special & Epic Boosters (+2 / +3 / +4)">`;
    for (const [k, b] of Object.entries(ALL_BOOSTERS)) {
      if (b.type === 'special' && b.id > 0) {
        bstOptions += `<option value="${k}" ${bld.boosterKey === k ? 'selected' : ''}>${esc(b.name)}</option>`;
      }
    }
    bstOptions += `</optgroup><optgroup label="Basic Craftable Boosters (+1)">`;
    for (const [k, b] of Object.entries(ALL_BOOSTERS)) {
      if (b.type === 'basic' && b.id > 0) {
        bstOptions += `<option value="${k}" ${bld.boosterKey === k ? 'selected' : ''}>${esc(b.name)}</option>`;
      }
    }
    bstOptions += `</optgroup>`;

    let mgrOptions = `<option value="none" ${bld.managerKey === 'none' ? 'selected' : ''}>Off (Tanpa Boost)</option>`;
    mgrOptions += `<optgroup label="Booster Managers (Rating 89)">`;
    for (const [k, m] of Object.entries(ALL_MANAGERS)) {
      if (m.rating === 89) {
        const extra = m.booster_1 ? ` [+1 ${m.booster_1}]` : '';
        mgrOptions += `<option value="${k}" ${bld.managerKey === k ? 'selected' : ''}>${esc(m.name)} (89)${extra}</option>`;
      }
    }
    mgrOptions += `</optgroup><optgroup label="Top Managers (Rating 88)">`;
    for (const [k, m] of Object.entries(ALL_MANAGERS)) {
      if (m.rating === 88) {
        mgrOptions += `<option value="${k}" ${bld.managerKey === k ? 'selected' : ''}>${esc(m.name)} (88)</option>`;
      }
    }
    mgrOptions += `</optgroup><optgroup label="Rating 87 / 85 Managers">`;
    for (const [k, m] of Object.entries(ALL_MANAGERS)) {
      if (m.rating <= 87) {
        mgrOptions += `<option value="${k}" ${bld.managerKey === k ? 'selected' : ''}>${esc(m.name)} (${m.rating})</option>`;
      }
    }
    mgrOptions += `</optgroup>`;

    targetContainer.innerHTML = `
      <article class="pl-card efhub-card-theme">
        <div class="ef-card-header">
          <div class="ef-ovr-box">
            <span class="ef-pos-badge ef-pos-${posGroup}">${esc(mainPos)}</span>
            <span class="ef-ovr-val">${currentOvr}</span>
          </div>
          <div class="pl-top-info" style="text-align:right">
            <div class="eyebrow">ID ${id}</div>
            <h2 style="margin:0">${esc(meta.name)}</h2>
          </div>
        </div>

        <div style="display:flex;justify-content:center;padding:12px 0">
          ${playerCardImage(id, "large")}
        </div>

        <!-- eFootbase Live Controls -->
        <div class="ef-controls-panel">
          <div class="ef-slider-row">
            <div class="ef-slider-head">
              <span>Level ${bld.level} / ${maxLvl}</span>
              <span>Poin Sisa: <b>${remPts}</b> / ${totalMaxPts}</span>
            </div>
            <input type="range" id="ef-slider-lvl-${id}" min="1" max="${maxLvl}" value="${bld.level}">
          </div>

          <div class="ef-select-grid">
            <label>
              <span>Booster (51 Pilihan)</span>
              <select id="ef-sel-booster-${id}">${bstOptions}</select>
            </label>
            <label>
              <span>Manager (65 Pilihan)</span>
              <select id="ef-sel-mgr-${id}">${mgrOptions}</select>
            </label>
          </div>

          <div style="display:flex;gap:8px;margin-top:12px;flex-wrap:wrap">
            <button class="button button-primary ef-btn-auto" id="ef-btn-auto-${id}" type="button" style="flex:1;min-height:36px;font-size:12px">⚡ Auto-Allocate Max OVR</button>
            <button class="button button-secondary" id="ef-btn-rst-${id}" type="button" style="min-height:36px;font-size:12px">↺ Reset Poin</button>
            <button class="button button-secondary" id="ef-btn-sync-${id}" type="button" style="min-height:36px;font-size:12px;background:rgba(0,229,255,0.15);border-color:#00e5ff;color:#00e5ff">🌐 Salin dari Web (PESDB/eFHUB)</button>
          </div>
        </div>

        <div class="pl-grid">
          <div><span>${L("Ana pozisyon", "Main position")}</span><strong>${esc(mainPos)} · ${esc(posName(mainPos))}</strong></div>
          <div><span>${L("Seviye", "Level")}</span><strong>${bld.level} / ${maxLvl}</strong></div>
          <div><span>${L("Ülke", "Country")}</span><strong>${esc(teamName(record.initial_base_national_team_id))}</strong></div>
          <div><span>${L("Takım", "Team")}</span><strong>${esc(teamName(record.initial_base_team_id))}</strong></div>
          <div><span>${L("Güçlü ayak", "Strong foot")}</span><strong>${strong}</strong></div>
          <div><span>${L("Zayıf ayak", "Weak foot")}</span><strong>${weak}</strong></div>
          <div><span>${L("Zayıf ayak kullanımı / isabeti (ham)", "Weak-foot usage / accuracy (raw)")}</span><strong>${p[45]} / ${p[33]}</strong></div>
          <div><span>${L("Yaş · Boy · Kilo", "Age · Height · Weight")}</span><strong>${p[46]} · ${p[47]} cm · ${p[48]} kg</strong></div>
          <div><span>${L("Gaya Bermain (Serang)", "Attacking Playstyle")}</span><strong>${esc(playStyleLabel(playStyleIdAtk, false))} (${playStyleIdAtk})</strong></div>
          <div><span>${L("Gaya Bermain (Tahan)", "Defending Playstyle")}</span><strong>${esc(playStyleLabel(playStyleIdDef, true))} (${playStyleIdDef})</strong></div>
          <div><span>${L("Nadirlik (ham)", "Rarity (raw)")}</span><strong>${record.default_rarity ?? "—"}</strong></div>
        </div>
        <div class="pl-row"><span>${L("Pozisyonlar (A)", "Positions (A)")}</span><div class="pl-chips">${chips(A)}</div></div>
        <div class="pl-row"><span>${L("Pozisyonlar (B)", "Positions (B)")}</span><div class="pl-chips">${chips(B)}</div></div>
        <div class="pl-row"><span>${L("Paketler", "Packs")}</span><div class="pl-packs">${packs.length ? packs.map(esc).join(" · ") : "—"}</div></div>
        <div class="pl-row"><span>${L("Oyuncu becerileri", "Player skills", "Keahlian Pemain (Skills)")}</span><div class="pl-skill-tags">${skills.length ? skills.map((skill) => `<em class="skill-pill-efb">${esc(skill)}</em>`).join("") : "—"}</div></div>
        <div class="pl-row"><span>${L("COM Oynama Tarzı", "COM Styles", "Gaya Bermain COM (Dribel & AI)")}</span><div class="pl-skill-tags">${comStyles.length ? comStyles.map((cs) => `<em class="com-pill-efb" style="border-color:#38bdf8;color:#38bdf8">${esc(cs)}</em>`).join("") : "—"}</div></div>
        <h3 class="stats-heading">${L("Oyuncu istatistikleri", "Player stats")}</h3>
        <div class="pl-stats">${statsHtml}</div>
        <div class="pl-actions pl-player-actions">
          <button class="button button-secondary" id="pl-add-squad" type="button">＋ ${L("Kadroya ekle", "Add to squad")}</button>
          <button class="button button-primary" id="pl-edit" type="button">✎ ${L("Oyuncuyu düzenle", "Edit player")}</button>
        </div>
      </article>`;

    loadPlayerCardImages(targetContainer);

    const slider = targetContainer.querySelector(`#ef-slider-lvl-${id}`);
    if (slider) {
      slider.addEventListener("input", (e) => {
        bld.level = Number(e.target.value);
        renderCardProfile(id, targetContainer);
      });
    }

    const selBooster = targetContainer.querySelector(`#ef-sel-booster-${id}`);
    if (selBooster) {
      selBooster.addEventListener("change", (e) => {
        bld.boosterKey = e.target.value;
        renderCardProfile(id, targetContainer);
      });
    }

    const selMgr = targetContainer.querySelector(`#ef-sel-mgr-${id}`);
    if (selMgr) {
      selMgr.addEventListener("change", (e) => {
        bld.managerKey = e.target.value;
        renderCardProfile(id, targetContainer);
      });
    }

    const btnAuto = targetContainer.querySelector(`#ef-btn-auto-${id}`);
    if (btnAuto) {
      btnAuto.addEventListener("click", () => {
        bld.allocations = eFHub.autoAllocate(record, mainPos);
        renderCardProfile(id, targetContainer);
      });
    }

    const btnRst = targetContainer.querySelector(`#ef-btn-rst-${id}`);
    if (btnRst) {
      btnRst.addEventListener("click", () => {
        bld.allocations = {};
        renderCardProfile(id, targetContainer);
      });
    }

    const btnSync = targetContainer.querySelector(`#ef-btn-sync-${id}`);
    if (btnSync) {
      btnSync.addEventListener("click", () => {
        openWebImporter(id, targetContainer);
      });
    }

    const btnSquad = targetContainer.querySelector("#pl-add-squad");
    if (btnSquad) btnSquad.addEventListener("click", () => addPlayerToSquad(id));

    const btnEdit = targetContainer.querySelector("#pl-edit");
    if (btnEdit) btnEdit.addEventListener("click", () => openPlayerEditor(Number(id)));
  }

  function openPlayerInfo(rawId) {
    const id = String(rawId).replace(/\D/g, "");
    const modalContent = $("modal-content");
    if (!modalContent) return;
    renderCardProfile(id, modalContent);
    const player = playerInfo(Number(id));
    $("modal-title").textContent = player.name || L("Oyuncu bilgileri", "Player details");
    $("modal-actions").innerHTML = "";
    $("modal-backdrop").hidden = false;
  }

  function show(rawId, silent) {
    const id = String(rawId).replace(/\D/g, "");
    const card = $("pl-card");
    if (!card) return;
    renderCardProfile(id, card);
  }

  // ============================================================================
  // Web Importer (PESDB / eFHUB Parser & Instant Offline Database)
  // ============================================================================
  
  const SKILL_MAP_EN = {
    // 1. Dribbling Moves / Tricks
    "scissors feint": 70, "tipuan silang": 70, "sentuhan gunting": 70,
    "flip flap": 71,
    "marseille turn": 72, "putaran marseille": 72,
    "sombrero": 73,
    "cut behind & turn": 74, "cut behind and turn": 74, "potong belakang & balik": 74, "cruyff turn": 74,
    "scotch move": 75, "gerakan scotch": 75,
    "chop turn": 108, "tipuan potong sisi": 108, "tipuan bola pindah sisi": 108,
    "double touch": 107, "sentuhan ganda": 107,
    "sole control": 102, "kontrol telapak": 102, "kontrol telapak kaki": 102,

    // 2. Shooting Skills
    "heading": 76, "sundulan": 76,
    "long-range curler": 77, "long range curler": 77, "lengkung jarak jauh": 77, "lengkung jauh": 77,
    "knuckle shot": 78, "tembakan knuckle": 78,
    "acrobatic finishing": 79, "penyelesaian akrobatik": 79, "solusi akrobatik": 79,
    "heel trick": 80, "trik tumit": 80,
    "first-time shot": 81, "first time shot": 81, "tembakan satu sentuhan": 81,
    "chip shot control": 101, "kontrol tembakan cip": 101,
    "dipping shot": 103, "tembakan menukik": 103,
    "rising shot": 104, "tembakan roket": 104,
    "long-range shooting": 109, "long range shooting": 109, "tembakan jarak jauh": 109,

    // 3. Passing Skills
    "one-touch pass": 82, "one touch pass": 82, "umpan satu sentuhan": 82,
    "through passing": 110, "through pass": 110, "umpan terobosan": 110,
    "weighted pass": 83, "umpan berbobot": 83,
    "pinpoint crossing": 84, "umpan silang akurat": 84,
    "outside curler": 85, "lengkung luar": 85,
    "rabona": 86,
    "no look pass": 98, "no-look pass": 98, "umpan tanpa melihat": 98,
    "low lofted pass": 87, "umpan lambung rendah": 87,

    // 4. Defending Skills
    "man marking": 92, "marking": 92, "pengawalan ketat": 92,
    "track back": 93, "bantu bertahan": 93,
    "interception": 106, "intersepsi": 106,
    "blocker": 111, "pemblokir": 111,
    "acrobatic clearance": 94, "sapuan akrobatik": 94,
    "aerial superiority": 112, "superioritas udara": 112,
    "sliding tackle": 113, "tekel meluncur": 113,

    // 5. Goalkeeping Skills
    "gk low punt": 88, "tendangan gk rendah": 88,
    "gk high punt": 105, "tendangan gk tinggi": 105,
    "gk long throw": 90, "gk long throws": 90, "lemparan jauh gk": 90,
    "gk penalty saver": 100, "kiper penahan penalti": 100, "gk penahan penalti": 100,

    // 6. General / Specialty Skills
    "long throw": 89, "long throws": 89, "lemparan jauh": 89,
    "penalty specialist": 99, "spesialis penalti": 99,
    "gamesmanship": 91, "keahlian bermain game": 91, "keahlian pelanggaran": 91,
    "captaincy": 95, "kapten": 95,
    "super-sub": 96, "super sub": 96, "pengganti super": 96,
    "fighting spirit": 97, "semangat juang": 97,

    // 7. Show Time / Functional Skills
    "phenomenal finishing": 114, "penyelesaian fenomenal": 114,
    "fortress": 115, "benteng": 115, "benteng pertahanan": 115,
    "momentum dribbling": 116, "dribel kilat": 116,
    "game-changing pass": 117, "game changing pass": 117, "decisive pass": 117, "umpan kritis": 117, "umpan penentu kemenangan": 117,
    "visionary pass": 118, "umpan visioner": 118, "umpan mata elang": 118,
    "edged crossing": 119, "umpan silang melengkung": 119, "umpan silang menukik": 119,
    "blitz curler": 120, "tembakan lengkung kilat": 120,
    "bullet header": 121, "sundulan peluru": 121,
    "phenomenal pass": 122, "umpan fenomenal": 122,
    "aerial fort": 123, "benteng udara": 123, "gk aerial claim": 123,
    "acceleration burst": 124, "akselerasi kilat": 124,
    "long reach tackle": 125, "tekel jangkauan panjang": 125,
    "gk directing defence": 126, "gk directing defense": 126, "gk rush out": 126, "kiper pengarah pertahanan": 126,
    "low screamer": 127, "roket rendah": 127, "tembakan rendah menggelegar": 127,
    "willpower": 128, "tekad baja": 128,
    "gk spirit roar": 129, "pekik tempur kiper": 129,
    "magnetic feet": 130, "kaki magnet": 130,
    "attack trigger": 131, "pemicu serangan": 131,
    "shadow hunt": 132, "penutup celah": 132,
    "attacking surge": 133, "lonjakan serangan": 133,
    "snap strike": 134, "tendangan sentak": 134,
    "tap trick": 135, "trik sentuhan cepat": 135, "trik sentuhan": 135,
    "sharp cut": 136, "tebasan tajam": 136, "beast drive": 136,
    "power tackle": 137, "tekel bertenaga": 137
  };

  const PLAYSTYLE_MAP_EN = {
    "goal poacher": 1, "dummy runner": 2, "fox in the box": 3, "prolific winger": 4,
    "classic no. 10": 5, "hole player": 6, "box-to-box": 7, "anchor man": 8,
    "the destroyer": 9, "destroyer": 9, "extra frontman": 10, "extra attacker": 10,
    "attacking full-back": 11, "attacking fullback": 11, "defensive full-back": 12, "defensive fullback": 12,
    "deep-lying forward": 13, "creative playmaker": 14, "build up": 15,
    "offensive goalkeeper": 16, "defensive goalkeeper": 17, "roaming flank": 18,
    "cross specialist": 19, "orchestrator": 20, "full-back finisher": 21, "target man": 22,
    "press back": 23, "front line pressure": 24, "front line poacher": 25,
    "attacking outlet": 26, "all-action defender": 27, "pass disruptor": 28,
    "covering role": 29, "high line master": 30, "tough maker": 31,
    "deep defender": 32, "sweeper gk": 33
  };
function openWebImporter(id, parentContainer) {
    const record = catalog.records[id];
    const player = playerInfo(Number(id));
    const modalTitle = $("modal-title");
    const modalContent = $("modal-content");
    const modalActions = $("modal-actions");

    modalTitle.textContent = "🌐 Salin Database (Stat, Skill, Playstyle)";
    modalContent.innerHTML = `
      <div style="padding:6px 0;display:flex;flex-direction:column;gap:10px">
        <p style="font-size:12px;color:var(--muted);margin:0;line-height:1.4">
          Sinkronkan <b>26 Atribut</b>, <b>Batas Level</b>, <b>Semua Skills</b>, serta <b>Gaya Bermain Menyerang & Bertahan</b> secara akurat dari eFootballDB / database.
        </p>

        <div style="display:flex;flex-direction:column;gap:4px">
          <label style="font-size:11px;font-weight:700;color:var(--muted)">SUMBER & ID PEMAIN:</label>
          <div style="display:flex;gap:6px;align-items:center">
            <select id="web-source-select" style="font-size:12px;padding:6px 8px;border-radius:8px;background:#1e293b;color:#00e5ff;font-weight:700">
              <option value="efootballdb" selected>eFootballDB</option>
              <option value="pesdb">PESDB</option>
            </select>
            <input id="web-import-id" type="text" placeholder="ID Pemain (misal: ${id})" value="${id}" style="flex:1;font-size:12px;padding:6px 10px;border-radius:8px">
            <button class="button button-primary" id="web-btn-fetch" type="button" style="min-height:36px;font-size:12px;padding:0 14px;background:#00e5ff;color:#041019;font-weight:800">🌐 Fetch / Sync</button>
          </div>
        </div>

        <div style="display:flex;justify-content:space-between;align-items:center">
          <a id="web-link-external" href="https://efootballdb.com/players/profile?id=${id}" target="_blank" style="font-size:11px;color:#00e5ff;text-decoration:underline">↗ Buka Halaman Pemain di eFootballDB</a>
        </div>

        <div style="display:flex;flex-direction:column;gap:4px">
          <label style="font-size:11px;font-weight:700;color:var(--muted)">ATAU TEMPEL TEKS / TABEL DARI EFOOTBALLDB / PESDB:</label>
          <textarea id="web-import-paste" rows="5" placeholder="Buka efootballdb.com di browser, copy teks atau tabel statistiknya (termasuk skills & playstyle), lalu paste di sini..." style="font-size:11.5px;font-family:monospace;padding:8px;border-radius:8px"></textarea>
        </div>

        <div id="web-import-status" style="font-size:11.5px;color:#10b981;font-weight:700;min-height:16px;line-height:1.4"></div>
      </div>
    `;

    modalActions.innerHTML = `
      <button class="button button-secondary" id="web-btn-back" type="button" style="min-height:36px;font-size:12px">Kembali ke Kartu</button>
      <button class="button button-primary" id="web-btn-apply" type="button" style="background:#00e5ff;color:#041019;font-weight:800;min-height:36px;font-size:12px">✓ Terapkan ke Database</button>
    `;

    const statusEl = modalContent.querySelector("#web-import-status");
    const idInput = modalContent.querySelector("#web-import-id");
    const linkExternal = modalContent.querySelector("#web-link-external");
    const sourceSelect = modalContent.querySelector("#web-source-select");

    const updateLink = () => {
      const targetId = idInput.value.trim().match(/\d{5,}/)?.[0] || id;
      const src = sourceSelect.value;
      if (src === "efootballdb") {
        linkExternal.href = `https://efootballdb.com/players/profile?id=${targetId}`;
        linkExternal.textContent = `↗ Buka efootballdb.com/players/profile?id=${targetId}`;
      } else {
        linkExternal.href = `https://pesdb.net/efootball/?id=${targetId}`;
        linkExternal.textContent = `↗ Buka pesdb.net/?id=${targetId}`;
      }
    };
    idInput.addEventListener("input", updateLink);
    sourceSelect.addEventListener("change", updateLink);

    modalContent.querySelector("#web-btn-fetch")?.addEventListener("click", async () => {
      const rawInput = idInput.value.trim();
      const targetId = rawInput.match(/\d{5,}/)?.[0] || id;
      const source = sourceSelect.value || "efootballdb";

      // 1. Instant check in verified OFFLINE_PLAYER_DB
      if (OFFLINE_PLAYER_DB[targetId]) {
        const offData = OFFLINE_PLAYER_DB[targetId];
        applyParsedStats(id, offData);
        statusEl.innerHTML = `<span style="color:#10b981">✓ Berhasil memuat data resmi ${offData.name} (${offData.position}, Level ${offData.level_limit})!</span>`;
        setTimeout(() => renderCardProfile(id, modalContent), 700);
        return;
      }

      // 2. Instant check across ALL 44,000+ players in catalog & ziru_db!
      ziruEnsure(targetId);
      if (catalog.records[targetId]) {
        const rec = catalog.records[targetId];
        const pl = catalog.players[targetId] || {};
        const p = rec.parameters || [];
        const mainPos = MAIN_POS[Number(p[0])] || pl.position || "—";
        const stats = {};
        for (const [statKey, sIdx] of Object.entries(STAT_MAP)) {
          stats[statKey] = Number(p[sIdx]) || 40;
        }

        // Synchronize all skills!
        const skills = {};
        for (let i = 63; i <= 137; i++) {
          skills[i] = Number(p[i]) || 0;
        }

        // Synchronize both Attacking & Defending Playstyles!
        const play_style = Array.isArray(rec.play_style) ? [...rec.play_style] : [Number(p[61]) || 0, 0];

        const parsed = {
          name: pl.name || `ID ${targetId}`,
          position: mainPos,
          level_limit: rec.level_info?.level_limit || 30,
          stats: stats,
          skills: skills,
          play_style: play_style
        };
        applyParsedStats(id, parsed);

        const skillCount = Object.values(skills).filter(v => v === 1).length;
        const atkName = playStyleLabel(play_style[0], false);
        const defName = playStyleLabel(play_style[1], true);

        statusEl.innerHTML = `<span style="color:#10b981">✓ Berhasil memuat ${parsed.name} (${parsed.position}, Level ${parsed.level_limit}, ${skillCount} Skills, Gaya: ${atkName} / ${defName})!</span>`;
        setTimeout(() => renderCardProfile(id, modalContent), 700);
        return;
      }

      // 3. Fallback to online fetch if not in local database
      statusEl.textContent = `Menghubungi server ${source === 'efootballdb' ? 'eFootballDB' : 'PESDB'} untuk ID ${targetId}...`;
      const targetUrl = source === 'efootballdb' 
        ? `https://efootballdb.com/players/profile?id=${targetId}`
        : `https://pesdb.net/efootball/?id=${targetId}`;

      const proxies = [
        `https://api.allorigins.win/raw?url=${encodeURIComponent(targetUrl)}`,
        `https://api.codetabs.com/v1/proxy?quest=${encodeURIComponent(targetUrl)}`,
        `https://corsproxy.io/?${encodeURIComponent(targetUrl)}`
      ];

      let success = false;
      for (const pUrl of proxies) {
        try {
          const ctrl = new AbortController();
          const tid = setTimeout(() => ctrl.abort(), 6000);
          const resp = await fetch(pUrl, { signal: ctrl.signal });
          clearTimeout(tid);
          if (resp.ok) {
            const html = await resp.text();
            const parsed = parseWebStats(html);
            if (parsed && Object.keys(parsed.stats).length >= 5) {
              applyParsedStats(id, parsed);
              const skillCount = Object.keys(parsed.skills || {}).length;
              statusEl.innerHTML = `<span style="color:#10b981">✓ Berhasil mengambil & menerapkan data ${parsed.name || targetId} (${parsed.position || '—'}, ${skillCount} Skills, Playstyle disinkronkan)!</span>`;
              setTimeout(() => renderCardProfile(id, modalContent), 700);
              success = true;
              break;
            }
          }
        } catch (e) {}
      }

      if (!success) {
        statusEl.innerHTML = `<span style="color:#f59e0b">Koneksi dibatasi operator. Silakan ketuk tautan di atas untuk membuka halaman pemain, copy tabel statistiknya, lalu paste di kotak di bawah dan klik "Terapkan".</span>`;
      }
    });

    // Auto-parse on paste
    const pasteBox = modalContent.querySelector("#web-import-paste");
    pasteBox.addEventListener("input", () => {
      const text = pasteBox.value;
      if (text.length > 50) {
        const parsed = parseWebStats(text);
        if (parsed && Object.keys(parsed.stats).length >= 5) {
          const sc = Object.keys(parsed.skills || {}).length;
          const ps = parsed.play_style ? `${playStyleLabel(parsed.play_style[0], false)} / ${playStyleLabel(parsed.play_style[1], true)}` : '—';
          statusEl.innerHTML = `<span style="color:#00e5ff">Data terdeteksi: ${parsed.name || 'Pemain'} (${parsed.position || '—'}, Level ${parsed.level_limit || '—'}, ${sc} Skills, Gaya: ${ps}). Klik "Terapkan"!</span>`;
        }
      }
    });

    modalActions.querySelector("#web-btn-apply")?.addEventListener("click", () => {
      const text = pasteBox.value;
      if (!text.trim()) {
        statusEl.textContent = "Tempelkan teks statistik terlebih dahulu!";
        return;
      }
      const parsed = parseWebStats(text);
      if (parsed) {
        applyParsedStats(id, parsed);
        const sc = Object.keys(parsed.skills || {}).length;
        statusEl.innerHTML = `<span style="color:#10b981">✓ Berhasil mengekstrak ${parsed.name || 'Pemain'} (${parsed.position || '—'}, ${sc} Skills & Playstyle tersimpan)!</span>`;
        setTimeout(() => renderCardProfile(id, modalContent), 800);
      } else {
        statusEl.innerHTML = `<span style="color:#ef4444">Format tidak dikenali. Pastikan menyalin tabel atribut dari eFootballDB/PESDB.</span>`;
      }
    });

    modalActions.querySelector("#web-btn-back")?.addEventListener("click", () => {
      renderCardProfile(id, modalContent);
    });
  }

  function parseWebStats(rawText) {
    const res = { stats: {}, skills: {}, com_styles: {}, play_style: [0, 0] };

    // 1. Detect Name
    const nameM = rawText.match(/<h1[^>]*>([^<]+)<\/h1>/i) ||
                  rawText.match(/Overall Rating\s*\d+\s*([^\n\r·]+)/i) ||
                  rawText.match(/^([A-Z][a-zA-Z\s.]{2,30})/m);
    if (nameM) res.name = nameM[1].trim();

    // 2. Detect Level Limit
    const lvlM = rawText.match(/(?:Max Level|Level Cap|Maks Level|Level)[^\d]*(\d+)/i);
    if (lvlM) res.level_limit = Number(lvlM[1]);

    // 3. Detect Main Position strictly
    let posMatch = rawText.match(/(?:Registered Position|Main Position|Position|Posisi)[\s:·]*\b(CF|SS|LWF|RWF|AMF|CMF|DMF|LMF|RMF|CB|LB|RB|GK)\b/i);
    if (!posMatch) posMatch = rawText.match(/^[\s]*\b(CF|SS|LWF|RWF|AMF|CMF|DMF|LMF|RMF|CB|LB|RB)\b\s*\d+/im);
    if (!posMatch) posMatch = rawText.match(/[·|\-]\s*\b(CF|SS|LWF|RWF|AMF|CMF|DMF|LMF|RMF|CB|LB|RB)\b/i);
    if (!posMatch && (/Goalkeeper/i.test(rawText) || /\bGK\s*·/i.test(rawText))) posMatch = [null, "GK"];
    if (posMatch) res.position = posMatch[1].toUpperCase();

    // 4. Detect Attacking & Defending Playstyles!
    for (const [styleName, styleId] of Object.entries(PLAYSTYLE_MAP_EN)) {
      const re = new RegExp(`\\b${styleName}\\b`, "i");
      if (re.test(rawText)) {
        if (styleId <= 22) res.play_style[0] = styleId;
        else res.play_style[1] = styleId;
      }
    }

    // 5. Detect COM Playing Styles (63-69)
    for (const com of COM_PLAYING_STYLES) {
      const re = new RegExp(`\\b${com.en}\\b|\\b${com.id}\\b`, "i");
      if (re.test(rawText)) {
        res.com_styles[com.parameter] = 1;
      }
    }

    // 6. Detect Player Skills (70-137)
    for (const [skillName, paramIdx] of Object.entries(SKILL_MAP_EN)) {
      const escaped = skillName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
      const re = new RegExp(`\\b${escaped}\\b`, "i");
      if (re.test(rawText)) {
        res.skills[paramIdx] = 1;
      }
    }

    // 7. Map Stats
    const patterns = [
      [/Offensive Awareness[^\d]*(\d+)/i, "offensive-awareness"],
      [/Ball Control[^\d]*(\d+)/i, "ball-control"],
      [/Dribbling[^\d]*(\d+)/i, "dribbling"],
      [/Tight Possession[^\d]*(\d+)/i, "tight-possession"],
      [/Low Pass[^\d]*(\d+)/i, "low-pass"],
      [/Lofted Pass[^\d]*(\d+)/i, "lofted-pass"],
      [/Finishing[^\d]*(\d+)/i, "finishing"],
      [/Heading[^\d]*(\d+)/i, "heading"],
      [/Set Piece Taking[^\d]*(\d+)/i, "set-piece-taking"],
      [/Curl[^\d]*(\d+)/i, "curl"],
      [/Speed[^\d]*(\d+)/i, "speed"],
      [/Acceleration[^\d]*(\d+)/i, "acceleration"],
      [/Kicking Power[^\d]*(\d+)/i, "kicking-power"],
      [/Jumping[^\d]*(\d+)/i, "jumping"],
      [/Physical Contact[^\d]*(\d+)/i, "physical-contact"],
      [/Balance[^\d]*(\d+)/i, "balance"],
      [/Stamina[^\d]*(\d+)/i, "stamina"],
      [/Defensive Awareness[^\d]*(\d+)/i, "defensive-awareness"],
      [/Defensive Engagement[^\d]*(\d+)/i, "defensive-engagement"],
      [/(?:Tackling|Ball Winning)[^\d]*(\d+)/i, "ball-winning"],
      [/Aggression[^\d]*(\d+)/i, "aggression"],
      [/GK Awareness[^\d]*(\d+)/i, "gk-awareness"],
      [/GK Catching[^\d]*(\d+)/i, "gk-catching"],
      [/(?:GK Parrying|GK Clearing)[^\d]*(\d+)/i, "gk-clearing"],
      [/GK Reflexes[^\d]*(\d+)/i, "gk-reflexes"],
      [/GK Reach[^\d]*(\d+)/i, "gk-reach"]
    ];

    for (const [rePattern, key] of patterns) {
      const m = rawText.match(rePattern);
      if (m) res.stats[key] = Number(m[1]);
    }

    if (Object.keys(res.stats).length >= 5 || Object.keys(res.skills).length >= 1) return res;
    return null;
  }

  function applyParsedStats(id, parsed) {
    const key = String(id);
    ziruEnsure(key);
    const rec = catalog.records[key];
    if (!rec || !Array.isArray(rec.parameters)) return;

    if (parsed.name && catalog.players[key]) {
      catalog.players[key].name = parsed.name;
    }
    if (parsed.position) {
      const idx = MAIN_POS.indexOf(parsed.position);
      if (idx >= 0) rec.parameters[0] = idx;
      if (catalog.players[key]) catalog.players[key].position = parsed.position;
    }
    if (parsed.level_limit) {
      rec.level_info = rec.level_info || {};
      rec.level_info.level_limit = parsed.level_limit;
      rec.level_info.level = parsed.level_limit;
    }

    // Apply 26 Stats
    if (parsed.stats) {
      for (const [statKey, val] of Object.entries(parsed.stats)) {
        const paramIdx = STAT_MAP[statKey];
        if (paramIdx != null) {
          rec.parameters[paramIdx] = val;
        }
      }
    }

    // Apply Playstyles (Attacking & Defending)
    if (Array.isArray(parsed.play_style)) {
      rec.play_style = [...parsed.play_style];
      if (parsed.play_style[0] != null) {
        rec.parameters[61] = parsed.play_style[0];
      }
    }

    // Apply Skills: WIPE old skills (70-137) completely so 100% accurate!
    if (parsed.skills && Object.keys(parsed.skills).length > 0) {
      for (let i = 70; i <= 137; i++) {
        rec.parameters[i] = 0;
      }
      for (const [sIdx, val] of Object.entries(parsed.skills)) {
        const idx = Number(sIdx);
        if (idx >= 70 && idx <= 137) {
          rec.parameters[idx] = Number(val) ? 1 : 0;
        }
      }
    }

    // Apply COM styles: WIPE old COM styles (63-69)
    if (parsed.com_styles && Object.keys(parsed.com_styles).length > 0) {
      for (let i = 63; i <= 69; i++) {
        rec.parameters[i] = 0;
      }
      for (const [cIdx, val] of Object.entries(parsed.com_styles)) {
        const idx = Number(cIdx);
        if (idx >= 63 && idx <= 69) {
          rec.parameters[idx] = Number(val) ? 1 : 0;
        }
      }
    }

    state.custom_records ||= {};
    state.custom_records[key] = rec;
    persist();
  }



  function openLivePlayerEditor(id) {
    const original = livePlayers[id];
    const current = livePlayer(id);
    if (!Array.isArray(original) || !Array.isArray(current)) {
      showToast(L("Canlı oyuncu kaydı bulunamadı.", "Live player record was not found."));
      return;
    }
    const [name, height, weight, nationality, positionId] = current;
    const disabledNumber = (label, min = 0, max = 99) => `<label>${label}<input type="number" min="${min}" max="${max}" value="" placeholder="—" disabled></label>`;
    const disabledText = (label) => `<label>${label}<input type="text" value="" placeholder="—" disabled></label>`;
    const disabledSelect = (options) => `<select disabled><option selected>—</option>${options}</select>`;
    const slotOptions = `<option value="0">—</option><option value="1">B</option><option value="2">A</option>`;
    $("modal-title").textContent = L("Oyuncu özellikleri", "Player properties");
    $("modal-content").innerHTML = `
      <p class="modal-line">${L("Canlı kayıtta bulunmayan alanlar düzenlenemez; bu alanlara değer uydurulmaz.", "Fields missing from the live record are unavailable and are not fabricated.")}</p>
      <div class="player-edit-grid">
        <label>${L("Oyuncu adı", "Player name")}<input id="live-edit-name" type="text" maxlength="80" value="${esc(name)}"></label>
        <label>${L("Ana pozisyon", "Main position")}<select id="live-edit-position">${MAIN_POS.map((pos, index) => `<option value="${index}"${index === Number(positionId) ? " selected" : ""}>${pos} ·${esc(posName(pos))}</option>`).join("")}</select></label>
        ${disabledNumber(L("Seviye", "Level"), 1)}
        ${disabledNumber(L("Seviye sınırı", "Level limit"), 1)}
        <label>${L("Ülke ID’si", "Country ID")}<input id="live-edit-country" type="number" min="0" max="1023" value="${Number(nationality)}"></label>
        ${disabledText(L("Takım (ID veya ad)", "Team (ID or name)"))}
        <label>${L("Güçlü ayak", "Strong foot")}${disabledSelect(`<option>${L("Sağ (zayıf: sol)", "Right (weak: left)")}</option><option>${L("Sol (zayıf: sağ)", "Left (weak: right)")}</option>`)}</label>
        ${disabledNumber(L("Zayıf ayak kullanımı (0-3)", "Weak-foot usage (0-3)"), 0, 3)}
        ${disabledNumber(L("Zayıf ayak isabeti (0-3)", "Weak-foot accuracy (0-3)"), 0, 3)}
        ${disabledNumber(L("Yaş", "Age"), 1, 99)}
        <label>${L("Boy (kaynak değeri)", "Height (source value)")}<input id="live-edit-height" type="number" min="0" max="255" value="${Number(height)}"></label>
        <label>${L("Kilo (kaynak değeri)", "Weight (source value)")}<input id="live-edit-weight" type="number" min="0" max="255" value="${Number(weight)}"></label>
        ${disabledSelect("")}
        ${disabledNumber(L("Nadirlik (ham)", "Rarity (raw)"), 0, 20)}
      </div>`;
    const playerStats = [
  ["offensive-awareness", 14], ["ball-control", 17],
  ["dribbling", 18], ["tight-possession", 19],
  ["low-pass", 21], ["lofted-pass", 22],
  ["finishing", 20], ["heading", 23],
  ["set-piece-taking", 27], ["curl", 28],
  ["speed", 34], ["acceleration", 38],
  ["kicking-power", 37], ["jumping", 39],
  ["physical-contact", 35], ["balance", 36],
  ["stamina", 40], ["defensive-awareness", 15],
  ["defensive-engagement", 26], ["ball-winning", 24],
  ["aggression", 25], ["gk-awareness", 16],
  ["gk-catching", 29], ["gk-clearing", 30],
  ["gk-reflexes", 31], ["gk-reach", 32]
];
    $("modal-content").insertAdjacentHTML("beforeend", `
      <h3 class="stats-heading">${L("Pozisyon yeterlilikleri", "Position ratings")}</h3>
      <div class="pe-slots">${Object.entries(SLOT_POS).map(([slot, pos]) => `<label><span>${pos}</span>${disabledSelect(slotOptions)}</label>`).join("")}</div>
      <h3 class="stats-heading">${L("Oyuncu istatistikleri", "Player stats")}</h3>
      <div class="stats-edit-grid">${playerStats.map(([key]) => disabledNumber(esc(t(key)))).join("")}</div>
      <h3 class="stats-heading">${L("Oyuncu becerileri", "Player skills")}</h3>
      <div class="player-skill-grid">${PLAYER_SKILLS.filter((skill) => skill.en && skill.en.trim() !== "").map((skill) => `<label class="player-skill-check"><input type="checkbox" disabled><span>${esc(L(skill.tr, skill.en))}</span></label>`).join("")}</div>
      <h3 class="stats-heading">${L("Paketler", "Packs")}</h3>
      <p class="modal-line">${L("Canlı ID için oyuncu kaydı bulunmadığından Paketler’e oyuncu verisi eklenemez.", "This live ID has no player record to add to a package.")}</p>
      <div class="pe-packs">${state.packs.map((pack) => `<label><input type="checkbox" disabled><span>${esc(pack.title)}</span></label>`).join("")}</div>`);
    const hasOverride = Boolean(state.live_player_overrides?.[id]);
    $("modal-actions").innerHTML = `${hasOverride ? `<button class="button button-secondary" id="restore-live-player" type="button">${L("Orijinali geri yükle", "Restore original")}</button>` : ""}<button class="button button-secondary" id="cancel-live-player" type="button">${t("cancel")}</button><button class="button button-primary" id="save-live-player" type="button">${t("save")}</button>`;
    $("modal-backdrop").hidden = false;
    $("cancel-live-player").addEventListener("click", closeModal);
    $("restore-live-player")?.addEventListener("click", () => {
      delete state.live_player_overrides[id];
      persist();
      closeModal();
      search();
      show(id, true);
      showToast(L("Canlı oyuncu bilgileri geri yüklendi.", "Live player data restored."));
    });
    $("save-live-player").addEventListener("click", () => {
      const values = [
        $("live-edit-height"), $("live-edit-weight"), $("live-edit-country"), $("live-edit-position")
      ];
      const limits = [[0, 255], [0, 255], [0, 1023], [0, MAIN_POS.length - 1]];
      for (let index = 0; index < values.length; index++) {
        const value = Number(values[index].value);
        const [min, max] = limits[index];
        if (!Number.isInteger(value) || value < min || value > max) {
          values[index].focus();
          showToast(`${values[index].parentElement.textContent.trim()}: ${min}-${max}`);
          return;
        }
      }
      const updatedName = $("live-edit-name").value.trim();
      if (!updatedName) {
        $("live-edit-name").focus();
        showToast(L("Oyuncu adı boş olamaz.", "Player name cannot be empty."));
        return;
      }
      state.live_player_overrides ||= {};
      state.live_player_overrides[id] = [
        updatedName,
        Number($("live-edit-height").value),
        Number($("live-edit-weight").value),
        Number($("live-edit-country").value),
        Number($("live-edit-position").value)
      ];
      persist();
      closeModal();
      search();
      show(id, true);
      showToast(L(`${updatedName} güncellendi.`, `${updatedName} updated.`));
    });
  }

  /* ---------- tam oyuncu düzenleyici (kartta görünen her alan) ---------- */
  const idOf = (raw) => { const m = String(raw).match(/^\s*(\d+)/); return m ? Number(m[1]) : NaN; };
  const teamLabel = (id) => { const t = teamNames()[String(id)]; return t ? `${id} – ${t.name}` : String(id); };

  /* ---------- canlı oyuncular için tam kayıt taslağı ---------- */
  const liveDrafts = {};
  function buildLiveDraft(id) {
    const key = String(id);
    if (liveDrafts[key]) return liveDrafts[key];
    const live = livePlayer(key);
    if (!Array.isArray(live)) return null;
    const templateId = Object.keys(baseRecords)[0];
    const record = cloneJson(baseRecords[templateId]);
    const p = record.parameters.map(() => 0);
    record.gameplayer_id = { id: Number(key), serial: 0 };
    record.level_info = { level: 1, level_limit: 30, exp_total: 0, exp_current_level: 0, exp_to_next_level: 0, customize_point: 0 };
    record.initial_base_team_id = 0;
    record.initial_base_national_team_id = 0;
    record.default_rarity = 4;
    record.play_style = [0, 0];
    record.initial_booster_id_list = [0, 0, 0];
    record.additional_skill_id = [];
    record.additional_position = [];
    const playerStats = [
  ["offensive-awareness", 14], ["ball-control", 17],
  ["dribbling", 18], ["tight-possession", 19],
  ["low-pass", 21], ["lofted-pass", 22],
  ["finishing", 20], ["heading", 23],
  ["set-piece-taking", 27], ["curl", 28],
  ["speed", 34], ["acceleration", 38],
  ["kicking-power", 37], ["jumping", 39],
  ["physical-contact", 35], ["balance", 36],
  ["stamina", 40], ["defensive-awareness", 15],
  ["defensive-engagement", 26], ["ball-winning", 24],
  ["aggression", 25], ["gk-awareness", 16],
  ["gk-catching", 29], ["gk-clearing", 30],
  ["gk-reflexes", 31], ["gk-reach", 32]
];
    playerStats.forEach(([, idx]) => { p[idx] = 40; });
    const pos = Math.min(Math.max(Number(live[4]) || 0, 0), MAIN_POS.length - 1);
    p[0] = pos;
    p[POSITION_SLOT_BY_NAME[MAIN_POS[pos]]] = 2;
    p[45] = 1; p[33] = 1;
    p[46] = 25;
    p[47] = Math.min(250, Math.max(100, Number(live[1]) + 100));
    p[48] = Math.min(200, Math.max(35, Number(live[2]) + 30));
    p[61] = 0;
    record.parameters = p;
    liveDrafts[key] = record;
    return record;
  }

  window.openPlayerEditor = function (rawId) {
    const id = String(rawId);
    ziruEnsure(id);
    let record = catalog.records[id];
    if (!record || !Array.isArray(record.parameters) || !record.level_info) {
      const draft = buildLiveDraft(id);
      if (!draft) { showToast(t("invalid-player-record", { id })); return; }
      record = draft;
    }
    const p = record.parameters;
    const mainIdx = Number(p[0]) || 0;
    
    // TWO PLAYSTYLES
    const playStyleIdAtk = Number((record.play_style || [])[0] ?? p[61] ?? 0);
    const playStyleIdDef = Number((record.play_style || [])[1] ?? 0);
    
    const original = baseRecords[id] || state.player_originals[id] || state.custom_records[id];
    const hasChanges = original && JSON.stringify(record) !== JSON.stringify(original);
    const slotOpts = (v) => `<option value="0"${v === 0 ? " selected" : ""}>—</option><option value="1"${v === 1 ? " selected" : ""}>B</option><option value="2"${v === 2 ? " selected" : ""}>A</option>`;
    const teamOptions = Object.entries(teamNames()).map(([k, v]) => `<option value="${esc(k + " – " + v.name)}"></option>`).join("");
    
    const playStyleAtkOptions = PLAY_STYLES_ATK.map((style) => `<option value="${style.id}"${style.id === playStyleIdAtk ? " selected" : ""}>${esc(L(style.tr, style.en))}</option>`).join("") +
      (PLAY_STYLES_ATK.some((style) => style.id === playStyleIdAtk) ? "" : `<option value="${playStyleIdAtk}" selected>${esc(playStyleLabel(playStyleIdAtk, false))}</option>`);
      
    const playStyleDefOptions = PLAY_STYLES_DEF.map((style) => `<option value="${style.id}"${style.id === playStyleIdDef ? " selected" : ""}>${esc(L(style.tr, style.en))}</option>`).join("") +
      (PLAY_STYLES_DEF.some((style) => style.id === playStyleIdDef) ? "" : `<option value="${playStyleIdDef}" selected>${esc(playStyleLabel(playStyleIdDef, true))}</option>`);

    const f = (label, inner) => `<label>${label}${inner}</label>`;
    const nIn = (elId, v, min, max) => `<input id="${elId}" type="number" min="${min}" max="${max}" value="${Number(v) || 0}">`;
    const playerStats = [
  ["offensive-awareness", 14], ["ball-control", 17],
  ["dribbling", 18], ["tight-possession", 19],
  ["low-pass", 21], ["lofted-pass", 22],
  ["finishing", 20], ["heading", 23],
  ["set-piece-taking", 27], ["curl", 28],
  ["speed", 34], ["acceleration", 38],
  ["kicking-power", 37], ["jumping", 39],
  ["physical-contact", 35], ["balance", 36],
  ["stamina", 40], ["defensive-awareness", 15],
  ["defensive-engagement", 26], ["ball-winning", 24],
  ["aggression", 25], ["gk-awareness", 16],
  ["gk-catching", 29], ["gk-clearing", 30],
  ["gk-reflexes", 31], ["gk-reach", 32]
];

    $("modal-title").textContent = L("Oyuncu özellikleri", "Player properties");
    $("modal-content").innerHTML = `
      <div class="player-edit-grid">
        ${f(L("Ana pozisyon", "Main position"), `<select id="pe-pos">${MAIN_POS.map((x, i) => `<option value="${i}"${i === mainIdx ? " selected" : ""}>${x} · ${esc(posName(x))}</option>`).join("")}</select>`)}
        ${f(L("Seviye", "Level"), nIn("pe-level", record.level_info.level || 1, 1, 99))}
        ${f(L("Seviye sınırı", "Level limit"), nIn("pe-limit", record.level_info.level_limit || 1, 1, 99))}
        ${f(L("Ülke (ID veya ad)", "Country (ID or name)"), `<input id="pe-nat" list="pe-teams" value="${esc(teamLabel(record.initial_base_national_team_id))}">`)}
        ${f(L("Takım (ID veya ad)", "Team (ID or name)"), `<input id="pe-team" list="pe-teams" value="${esc(teamLabel(record.initial_base_team_id))}">`)}
        <datalist id="pe-teams">${teamOptions}</datalist>
        ${f(L("Güçlü ayak", "Strong foot"), `<select id="pe-foot"><option value="0"${Number(p[43]) !== 1 ? " selected" : ""}>${L("Sağ (zayıf: sol)", "Right (weak: left)")}</option><option value="1"${Number(p[43]) === 1 ? " selected" : ""}>${L("Sol (zayıf: sağ)", "Left (weak: right)")}</option></select>`)}
        ${f(L("Zayıf ayak kullanımı (0-3)", "Weak-foot usage (0-3)"), nIn("pe-wf-use", p[45], 0, 3))}
        ${f(L("Zayıf ayak isabeti (0-3)", "Weak-foot accuracy (0-3)"), nIn("pe-wf-acc", p[33], 0, 3))}
        ${f(L("Yaş", "Age"), nIn("pe-age", p[46], 1, 99))}
        ${f(L("Boy (cm)", "Height (cm)"), nIn("pe-height", p[47], 100, 250))}
        ${f(L("Kilo (kg)", "Weight (kg)"), nIn("pe-weight", p[48], 35, 200))}
        ${f(L("Gaya Bermain Menyerang", "Attacking Playstyle"), `<select id="pe-style-atk">${playStyleAtkOptions}</select>`)}
        ${f(L("Gaya Bermain Bertahan", "Defending Playstyle"), `<select id="pe-style-def">${playStyleDefOptions}</select>`)}
        ${f(L("Nadirlik (ham)", "Rarity (raw)"), nIn("pe-rarity", record.default_rarity, 0, 20))}
      </div>
      <h3 class="stats-heading">${L("Pozisyon yeterlilikleri", "Position ratings")}</h3>
      <div class="pe-slots">${Object.entries(SLOT_POS).map(([slot, pos]) => `<label><span>${pos}</span><select data-slot="${slot}">${slotOpts(Number(p[slot]) || 0)}</select></label>`).join("")}</div>
      <h3 class="stats-heading">${L("Oyuncu istatistikleri", "Player stats")}</h3>
      <div class="stats-edit-grid">${playerStats.map(([key, idx]) => `<label>${esc(t(key))}<input data-stat-index="${idx}" type="number" min="0" max="99" value="${Number(p[idx]) || 0}"></label>`).join("")}</div>
      <h3 class="stats-heading">${L("Oyuncu becerileri", "Player skills", "Keahlian Pemain (Skills)")}</h3>
      <div class="player-skill-grid">${PLAYER_SKILLS.filter((skill) => skill.en && skill.en.trim() !== "").map((skill) => `<label class="player-skill-check"><input type="checkbox" data-player-skill="${skill.parameter}"${Number(p[skill.parameter]) === 1 ? " checked" : ""}><span>${esc(L(skill.tr, skill.en, skill.id))}</span></label>`).join("")}</div>
      <h3 class="stats-heading">${L("COM Oynama Tarzı", "COM Playing Styles", "Gaya Bermain COM (Dribel & AI)")}</h3>
      <div class="player-skill-grid">${COM_PLAYING_STYLES.map((c) => `<label class="player-skill-check"><input type="checkbox" data-com-style="${c.parameter}"${Number(p[c.parameter]) === 1 ? " checked" : ""}><span>${esc(L(c.tr, c.en, c.id))}</span></label>`).join("")}</div>
      <h3 class="stats-heading">${L("Paketler", "Packs")}</h3>
      <div class="pe-packs">${state.packs.map((pk) => `<label><input type="checkbox" data-pack="${pk.agent_id}"${pk.player_ids.includes(Number(id)) ? " checked" : ""}><span>${esc(pk.title)}</span></label>`).join("")}</div>`;
    $("modal-actions").innerHTML = `${hasChanges ? `<button class="button button-secondary" id="restore-player-original" type="button">${t("restore-original")}</button>` : ""}<button class="button button-secondary" id="modal-cancel" type="button">${t("cancel")}</button><button class="button button-primary" id="save-player-editor" type="button">${t("save")}</button>`;
    $("modal-backdrop").hidden = false;
    $("modal-cancel").addEventListener("click", closeModal);
    $("restore-player-original")?.addEventListener("click", () => window.restorePlayerEditor(id));
    $("save-player-editor").addEventListener("click", () => window.savePlayerEditor(id));
  };

  window.savePlayerEditor = function (id) {
    const num = (elId, min, max) => {
      const el = $(elId); const v = Number(el.value);
      if (el.value === "" || !Number.isInteger(v) || v < min || v > max) { el.focus(); throw new Error(`${el.parentElement.textContent.trim()}: ${min}-${max}`); }
      return v;
    };
    const teamId = (elId) => {
      const el = $(elId); const v = idOf(el.value);
      if (!Number.isInteger(v) || v < 0 || v > 99999) { el.focus(); throw new Error(`${el.parentElement.textContent.trim()}: ID`); }
      return v;
    };
    try {
      const key = String(id);
      const record = cloneJson(catalog.records[key] || liveDrafts[key]);
      if (!record || !Array.isArray(record.parameters) || !record.level_info) {
        throw new Error(t("invalid-player-record", { id }));
      }
      const p = record.parameters;
      const level = num("pe-level", 1, 99), limit = num("pe-limit", 1, 99);
      if (level > limit) { $("pe-level").focus(); throw new Error(L("Seviye, sınırdan büyük olamaz.", "Level cannot exceed the level limit.")); }
      if (!baseRecords[key] && !state.player_originals[key]) {
        const original = state.custom_records[key] || record;
        state.player_originals[key] = cloneJson(original);
      }
      record.level_info.level = level; record.level_info.level_limit = limit;
      const mainIdx = Number($("pe-pos").value); p[0] = mainIdx;
      document.querySelectorAll("[data-slot]").forEach((el) => { p[Number(el.dataset.slot)] = Number(el.value); });
      p[POSITION_SLOT_BY_NAME[MAIN_POS[mainIdx]]] = 2;
      record.initial_base_national_team_id = teamId("pe-nat");
      record.initial_base_team_id = teamId("pe-team");
      p[43] = Number($("pe-foot").value);
      p[45] = num("pe-wf-use", 0, 3); p[33] = num("pe-wf-acc", 0, 3);
      const age = num("pe-age", 1, 99);
      p[46] = age; p[47] = num("pe-height", 100, 250); p[48] = num("pe-weight", 35, 200);
      
      record.play_style = Array.isArray(record.play_style) ? record.play_style : [0, 0];
      const origStyleAtk = Number(record.play_style[0]);
      const playStyleAtk = Number($("pe-style-atk").value) === origStyleAtk ? origStyleAtk : num("pe-style-atk", 0, 99);
      const origStyleDef = Number(record.play_style[1] || 0);
      const playStyleDef = Number($("pe-style-def").value) === origStyleDef ? origStyleDef : num("pe-style-def", 0, 99);
      
      record.play_style[0] = playStyleAtk;
      record.play_style[1] = playStyleDef;
      p[61] = playStyleAtk; // Backup ke parameter asli
      
      record.default_rarity = num("pe-rarity", 0, 20);
      document.querySelectorAll("[data-stat-index]").forEach((el) => {
        const v = Number(el.value);
        if (el.value === "" || !Number.isInteger(v) || v < 0 || v > 99) { el.focus(); throw new Error(`${el.parentElement.textContent.trim()}: 0-99`); }
        p[Number(el.dataset.statIndex)] = v;
      });
      document.querySelectorAll("[data-player-skill]").forEach((el) => {
        p[Number(el.dataset.playerSkill)] = el.checked ? 1 : 0;
      });
      document.querySelectorAll("[data-com-style]").forEach((el) => {
        p[Number(el.dataset.comStyle)] = el.checked ? 1 : 0;
      });
      if (!baseRecords[key]) {
        const live = livePlayer(key);
        if (Array.isArray(live) && !state.custom_records[key]) {
          state.custom_records[key] = record;
          state.custom_players ||= {};
          state.custom_players[key] = { name: String(live[0] || `ID ${key}`), position: MAIN_POS[Number(record.parameters[0])] || "—", age: Number(record.parameters[46]) };
          catalog.players[key] = state.custom_players[key];
          delete liveDrafts[key];
        }
      }
      if (baseRecords[key]) state.player_overrides[key] = record;
      if (state.custom_records[key]) state.custom_records[key] = record;
      catalog.records[key] = record;
      /* paket üyeliği */
      document.querySelectorAll("[data-pack]").forEach((el) => {
        const pk = state.packs.find((x) => String(x.agent_id) === el.dataset.pack);
        if (!pk) return;
        const has = pk.player_ids.includes(Number(id));
        if (el.checked && !has) pk.player_ids.push(Number(id));
        if (!el.checked && has) { pk.player_ids = pk.player_ids.filter((x) => x !== Number(id)); pk.featured_ids = pk.featured_ids.filter((x) => x !== Number(id)); }
      });
      persist();
      try { renderStats(); renderPackList(); renderRoster(); renderFeatured(); searchPlayers(playerSearch.value); } catch (e) {}
      closeModal();
      showToast(t("player-updated", { name: playerInfo(Number(id)).name }));
      if (currentId) show(currentId, true);
    } catch (error) { showToast(error.message); }
  };
  window.restorePlayerEditor = function (id) {
    const key = String(id);
    const original = baseRecords[key] || state.player_originals[key] || state.custom_records[key];
    if (!original) {
      showToast(t("invalid-player-record", { id }));
      return;
    }
    catalog.records[key] = cloneJson(original);
    if (baseRecords[key]) {
      delete state.player_overrides[key];
    } else {
      state.custom_records[key] = cloneJson(original);
      delete state.player_overrides[key];
    }
    persist();
    renderRoster();
    renderFeatured();
    searchPlayers(playerSearch.value);
    closeModal();
    showToast(t("player-original-restored", { name: playerInfo(Number(id)).name }));
    if (currentId) show(currentId, true);
  };
  const POSITION_SLOT_BY_NAME = { CF: 1, SS: 2, LWF: 3, RWF: 4, AMF: 5, DMF: 6, CMF: 7, LMF: 8, RMF: 9, CB: 10, LB: 11, RB: 12, GK: 13 };

  /* ---------- 4) Yanıtlar sekmesi çevirisi + nav etiketleri ---------- */
  const EXACT = [
    ["Yanıt Paneli", "Response Panel"], ["Response dosyalarındaki değerleri düzenle. Değişenler cihazda saklanır.", "Edit values in the response files. Changes are stored on this device."],
    ["Dosyalar", "Files"], ["Dosya ara...", "Search files..."], ["Soldan bir dosya seçin", "Select a file on the left"],
    ["Düzenleyici", "Editor"], ["Sıfırla", "Reset"], ["Hızlı", "Quick"], ["Ağaç", "Tree"], ["Ara", "Find"], ["Uygula", "Apply"], ["Sonuç yok", "No results"],
    ["ÖNEMLİ ALANLAR", "KEY FIELDS"], ["ÜST DÜZEY DEĞERLER", "TOP-LEVEL VALUES"],
    ["Bu dosya Paketler sekmesi tarafından da üretiliyor. Paket dışa aktarımıyla çakışırsa sonra aktardığınız geçerli olur.", "This file is also generated by the Packages tab. If the two exports collide, the one you apply later wins."],
    ["+ anahtar ekle", "+ add key"], ["Dosya seç…", "Select file…"], ["Eşleşme yok", "No match"], ["En az 2 karakter yazın", "Type at least 2 characters"],
    ["Üst düzeyde değer yok – Ağaç sekmesini kullanın.", "No top-level values – use the Tree tab."], ["Kullanıcı adı", "User name"], ["Oyuncu limiti", "Player limit"],
    ["Mevcut oyuncu limiti", "Current player limit"], ["Ana kadro ID", "Main squad ID"], ["Ülke", "Country"], ["Depo limiti", "Storage limit"], ["Tutorial bitti", "Tutorial done"],
    ["Anahtar veya değer ara (ör. gp, coin, YES)…", "Search key or value (e.g. gp, coin, YES)…"],
    ["Dosya büyük: Ham düzenleme kapalı, Ağaç/Ara kullanın.", "Large file: raw editing is disabled, use Tree/Find."],
    ["İlk 120 sonuç gösteriliyor – aramayı daraltın.", "Showing the first 120 results – narrow your search."],
    ["YEREL · HIZLI · GİZLİ", "LOCAL · FAST · PRIVATE"], ["Tema rengi", "Theme color"],
    ["Uygulamanın vurgu rengini seç. Anında uygulanır ve hatırlanır.", "Pick the app accent color. Applied instantly and remembered."],
    ["TON", "HUE"], ["DOYGUNLUK", "SATURATION"], ["PARLAKLIK", "BRIGHTNESS"], ["Varsayılan", "Default"], ["Tamam", "OK"]
  ];
  const PHRASE = [
    ["Değişenleri aktar", "Export changes"], ["Daha fazla", "More"], ["Tümünü göster", "Show all"], ["Daralt", "Collapse"],
    ["DEĞİŞTİ", "CHANGED"], ["Hata: ", "Error: "], ["Uygulandı", "Applied"], ["PAKET", "PACK"]
  ];
  const ARIA = [["Bölümler", "Sections"], ["Tema", "Theme"]];
  const orig = new WeakMap();
  function translateNode(root) {
    if (!root) return;
    const en = lang() === "en";
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT);
    const nodes = [];
    while (walker.nextNode()) nodes.push(walker.currentNode);
    nodes.forEach((n) => {
      const cur = n.nodeValue;
      if (en) {
        const trimmed = cur.trim();
        let out = cur;
        const hit = EXACT.find(([a]) => a === trimmed);
        if (hit) out = cur.replace(trimmed, hit[1]);
        else PHRASE.forEach(([a, b]) => { if (out.includes(a)) out = out.split(a).join(b); });
        if (out !== cur) { if (!orig.has(n)) orig.set(n, cur); n.nodeValue = out; }
      } else if (orig.has(n)) { n.nodeValue = orig.get(n); orig.delete(n); }
    });
    root.querySelectorAll("[placeholder]").forEach((el) => {
      const hit = EXACT.find(([a, b]) => el.placeholder === (en ? a : b));
      if (hit) el.placeholder = en ? hit[1] : hit[0];
    });
  }
  function relabel() {
    const names = {
      packs: lang() === "vi" ? "Gói Thẻ" : L("Paketler", "Packages"),
      players: lang() === "vi" ? "Dữ Liệu Thẻ" : L("Oyuncular", "Players"),
      squad: lang() === "vi" ? "Đội Hình" : L("Kadro", "Squad"),
      resp: lang() === "vi" ? "Lệnh Ziru" : L("Yanıtlar", "Responses")
    };
    document.querySelectorAll(".nav-tab").forEach((b) => { const s = b.querySelector("span"); if (s && names[b.dataset.v]) s.textContent = names[b.dataset.v]; });
    document.querySelectorAll(".credit-text").forEach((el) => { el.textContent = lang() === "vi" ? "Phát triển bởi eFootbase Team · Tiếng Việt" : L("berwtproxy tarafından yapıldı", "Made by berwtproxy"); });
    document.querySelectorAll("[aria-label]").forEach((el) => {
      const hit = ARIA.find(([a, b]) => el.getAttribute("aria-label") === (lang() === "en" ? a : b));
      if (hit) el.setAttribute("aria-label", lang() === "en" ? hit[1] : (lang() === "vi" ? (VI_MAP[b] || hit[1]) : hit[0]));
    });
    const g = $("lang-gate"); if (g) g.querySelector(".lg-title").textContent = lang() === "vi" ? "Chọn ngôn ngữ · Select language" : "Dil seçin · Choose language";
    labels();
    document.dispatchEvent(new CustomEvent("opm-lang"));
    if (lang() === "en") {
      [$("view-resp"), $("splash"), document.querySelector(".sheet")].forEach(translateNode);
    }
  }

  const baseApply = window.applyLanguage;
  window.applyLanguage = function () { if(baseApply) baseApply(); relabel(); };

  const cfg = { childList: true, subtree: true };
  const obs = new MutationObserver(() => { obs.disconnect(); translateNode($("view-resp")); obs.observe($("view-resp"), cfg); });
  if ($("view-resp")) obs.observe($("view-resp"), cfg);

  build();
  buildSquad();
  initGate();
  relabel();
  document.addEventListener("opm-lang", buildSquad);
})();