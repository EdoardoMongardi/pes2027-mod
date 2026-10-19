"use strict";
/* Yanıt Paneli – responses/*.json dosyalarını düzenler, değişenleri ZIP olarak dışa aktarır */
(function () {
  const R = window.RESPONSES || {};
  const MANAGED = new Set(["CmdGetMyclubAgentlist.json", "get_myclub_agentlist.json", "get_myclub_mainmenu_info.json", "ZIRU_PACKS_CONFIG.json"]);
  const PRESETS = {
    "CmdLogin.json": [
      ["Kullanıcı adı", "pes_user_info.user_name"], ["GP", "gp"], ["Coin", "coin_info.coin"],
      ["Oyuncu limiti", "entry_info.gameplayer_list_max"], ["Mevcut oyuncu limiti", "entry_info.current_gameplayer_list_max"],
      ["Ana kadro ID", "entry_info.main_squad_id"], ["Ülke", "user_eula_info.eula_select_country_a3"],
      ["Depo limiti", "entry_info.warehouse_player_max"], ["Tutorial bitti", "entry_info.tutorial_end"]
    ]
  };
  PRESETS["CmdGetMyclubEntryInfo.json"] = PRESETS["CmdGetEntryInfo.json"] = [
    ["Oyuncu limiti", "gameplayer_list_max"], ["Mevcut oyuncu limiti", "current_gameplayer_list_max"], ["Ana kadro ID", "main_squad_id"]
  ];
  const INFO = window.RESP_INFO || { files: {}, keys: {} };
  const INFO_EN = window.RESP_INFO_EN || { files: {}, keys: {} };
  const isEn = () => typeof state !== "undefined" && state && state.language === "en";
  const VIEW = window.RESPONSES_VIEW || {}, VDESC = window.RESPONSES_VIEW_DESC || {};
  const ALLNAMES = () => Object.keys(R).concat(Object.keys(VIEW));
  const descF = (n) => (isEn() && INFO_EN.files[n]) || INFO.files[n] || VDESC[n] || (n === "CmdLoginbackup.json" ? "CmdLogin yedeği (düzenlenebilir)." : "");
  const descK = (k) => (isEn() && INFO_EN.keys[k]) || INFO.keys[k];
  const EN = {
    "Kaydedilemedi (alan dolu)": "Could not save (storage full)", "Sıfırlandı": "Reset", "Değişiklik yok": "No changes",
    "Yanıtlar dışa aktarıldı": "Responses exported", "Dosya hatası": "File error", "Geçersiz JSON": "Invalid JSON",
    "Sil": "Delete", "Anahtar adı:": "Key name:", "Değer (JSON, ör. 5, \"YES\", true):": "Value (JSON, e.g. 5, \"YES\", true):"
  };
  const T = (s) => (isEn() && EN[s]) || s;
  const HIDE = new Set(["result", "rqid", "msgid"]);
  const hid = (path, k) => path.length === 0 && HIDE.has(k);
  const LS = "resp_panel_edits_v1";
  let edits = {};
  try { edits = JSON.parse(localStorage.getItem(LS) || "{}"); } catch (e) { edits = {}; }
  const work = {};            // name -> çalışma kopyası
  let cur = null, mode = "quick", filter = "", expanded = false;
  const $ = (id) => document.getElementById(id);
  const clone = (o) => JSON.parse(JSON.stringify(o));
  const esc = (s) => String(s).replace(/[&<>"]/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));
  const toast = (m) => (typeof showToast === "function" ? showToast(m) : alert(m));
  function data(name) {
    if (!work[name]) work[name] = edits[name] ? clone(edits[name]) : clone(R[name]);
    return work[name];
  }
  function save(name) {
    const same = JSON.stringify(work[name]) === JSON.stringify(R[name]);
    if (same) delete edits[name]; else edits[name] = work[name];
    try { localStorage.setItem(LS, JSON.stringify(edits)); } catch (e) { toast(T("Kaydedilemedi (alan dolu)")); }
    renderList(); updateCount();
  }
  const getP = (o, p) => p.split(".").reduce((a, k) => (a == null ? undefined : a[k]), o);
  function setP(o, path, v) { const ks = path.slice(); const last = ks.pop(); ks.reduce((a, k) => a[k], o)[last] = v; }
  function castLike(old, str) {
    if (typeof old === "number") { const n = Number(str); return Number.isFinite(n) ? n : old; }
    if (typeof old === "boolean") return str === "true";
    return str;
  }
  function updateCount() { const c = Object.keys(edits).length; const b = $("resp-edit-count"); if (b) b.textContent = c; }

  /* ---------- yerleşim ---------- */
  function build() {
    const host = $("view-resp");
    host.innerHTML = `
      <div class="rp-head">
        <div><span class="pill">Yanıt Paneli</span>
        <p>Response dosyalarındaki değerleri düzenle. Değişenler cihazda saklanır.</p></div>
        <button class="button button-primary" id="resp-export" type="button"><span aria-hidden="true">↓</span> <span>Değişenleri aktar (<b id="resp-edit-count">0</b>)</span></button>
      </div>
      <div class="rp-grid">
        <aside class="rp-side">
          <div class="sec-head"><span class="pill">Dosyalar</span><span class="seeall" id="resp-total">0 ›</span></div>
          <label class="search-box"><span aria-hidden="true">⌕</span><input id="resp-file-search" placeholder="Dosya ara..." autocomplete="off"></label>
          <select id="resp-mobile-select" class="mobile-package-select" style="display:none"></select>
          <div id="resp-list" class="rp-list"></div>
        </aside>
        <section class="rp-main" id="resp-main"><div class="empty-state">Soldan bir dosya seçin</div></section>
      </div>`;
    $("resp-file-search").addEventListener("input", e => { filter = e.target.value.toLowerCase(); renderList(); });
    $("resp-mobile-select").addEventListener("change", e => openFile(e.target.value));
    $("resp-export").addEventListener("click", exportEdits);
    $("resp-total").textContent = ALLNAMES().length + " ›";
    renderList(); updateCount();
  }
  function renderList() {
    const all = ALLNAMES().filter(n => (n + " " + (descF(n))).toLowerCase().includes(filter));
    const LIM = 8; const full = expanded || filter || all.length <= LIM;
    let names = full ? all : all.slice(0, LIM);
    if (!full && cur && all.includes(cur) && !names.includes(cur)) names = names.concat(cur);
    $("resp-list").innerHTML = names.map(n => `<button type="button" class="rp-file${n === cur ? " active" : ""}" data-n="${esc(n)}">
      <small>${MANAGED.has(n) ? "PAKET" : VIEW[n] ? "GÖRÜNÜM" : n.startsWith("ZIRU") ? "ZIRU" : /^cmd/i.test(n) ? "CMD" : "DATA"}${edits[n] ? " · ●" : ""}</small>
      <span><b>${esc(n.replace(/\.json$/, ""))}</b><i>${esc(descF(n))}</i></span></button>`).join("") || '<div class="rp-none">Sonuç yok</div>';
    $("resp-list").querySelectorAll(".rp-file").forEach(b => b.addEventListener("click", () => openFile(b.dataset.n)));
    if (!filter && all.length > LIM) {
      const mb = document.createElement("button"); mb.type = "button"; mb.className = "rp-more";
      mb.textContent = expanded ? "Daralt ▲" : "Tümünü göster (" + all.length + ") ▼";
      mb.addEventListener("click", () => { expanded = !expanded; renderList(); });
      $("resp-list").appendChild(mb);
    }
    $("resp-mobile-select").innerHTML = '<option value="">Dosya seç…</option>' + names.map(n => `<option value="${esc(n)}"${n === cur ? " selected" : ""}>${esc(n)}${edits[n] ? " ●" : ""}</option>`).join("");
  }
  function openFile(n) { if (!n) return; cur = n; if (VIEW[n]) { renderList(); renderView(n); return; } mode = PRESETS[n] ? "quick" : "tree"; renderList(); renderMain(); setTimeout(function () { var m = $("resp-main"); if (m) m.scrollIntoView({ behavior: "smooth", block: "start" }); }, 60); }

  function renderView(n) {
    const v = VIEW[n]; const m = $("resp-main");
    m.innerHTML = `<div class="rp-title"><div><span class="pill">Görünüm</span><div class="eyebrow" style="margin-top:8px">${(v.text.length / 1024).toFixed(1)} KB · SALT OKUNUR</div><h2>${esc(n)}</h2></div></div><p class="rp-desc">${esc(descF(n))}</p><div id="resp-body"></div>`;
    const body = $("resp-body");
    if (v.kind === "html") {
      const f = document.createElement("iframe"); f.setAttribute("sandbox", ""); f.srcdoc = v.text;
      f.style.cssText = "width:100%;max-width:calc(100vw - 44px);box-sizing:border-box;height:70vh;border:0;border-radius:12px;background:#fff"; body.appendChild(f);
    } else {
      const pre = document.createElement("pre"); pre.textContent = v.text;
      pre.style.cssText = "max-width:calc(100vw - 44px);white-space:pre-wrap;word-break:break-word;font-size:12px;max-height:70vh;overflow:auto"; body.appendChild(pre);
    }
    setTimeout(function () { m.scrollIntoView({ behavior: "smooth", block: "start" }); }, 50);
  }

  function renderMain() {
    const m = $("resp-main"); const n = cur; const d = data(n);
    const kb = (JSON.stringify(d).length / 1024).toFixed(1);
    m.innerHTML = `<div class="rp-title"><div><span class="pill">Düzenleyici</span><div class="eyebrow" style="margin-top:8px">${kb} KB${edits[n] ? " · DEĞİŞTİ" : ""}</div><h2>${esc(n)}</h2></div>
      <button class="button button-danger" id="resp-reset" type="button">Sıfırla</button></div>
      <p class="rp-desc">${esc(descF(n))}</p>
      ${MANAGED.has(n) ? '<div class="rp-note">Bu dosya Paketler sekmesi tarafından da üretiliyor. Paket dışa aktarımıyla çakışırsa sonra aktardığınız geçerli olur.</div>' : ""}
      <div class="rp-tabs">${[["quick", "Hızlı"], ["tree", "Ağaç"], ["find", "Ara"], ["json", "JSON"]].map(([k, l]) => `<button type="button" data-m="${k}" class="${mode === k ? "on" : ""}">${l}</button>`).join("")}</div>
      <div id="resp-body"></div>`;
    m.querySelectorAll(".rp-tabs button").forEach(b => b.addEventListener("click", () => { mode = b.dataset.m; renderMain(); }));
    $("resp-reset").addEventListener("click", () => { delete edits[n]; delete work[n]; try { localStorage.setItem(LS, JSON.stringify(edits)); } catch (e) {} renderList(); updateCount(); renderMain(); toast(T("Sıfırlandı")); });
    ({ quick: renderQuick, tree: renderTree, find: renderFind, json: renderJson })[mode]($("resp-body"));
  }

  /* ---------- tek alan girişi ---------- */
  function field(path, value, label) {
    const id = "f" + Math.random().toString(36).slice(2, 9);
    let ctl;
    if (typeof value === "boolean") ctl = `<select id="${id}"><option value="true"${value ? " selected" : ""}>true</option><option value="false"${!value ? " selected" : ""}>false</option></select>`;
    else if (value === "YES" || value === "NO") ctl = `<select id="${id}"><option${value === "YES" ? " selected" : ""}>YES</option><option${value === "NO" ? " selected" : ""}>NO</option></select>`;
    else ctl = `<input id="${id}" type="${typeof value === "number" ? "number" : "text"}" ${typeof value === "number" ? 'step="any" inputmode="decimal"' : ""} value="${esc(value ?? "")}">`;
    const wrap = document.createElement("label");
    wrap.className = "rp-field";
    const hint = descK(path[path.length - 1]);
    wrap.innerHTML = `<span>${esc(label)}</span>${hint ? `<em class="hint">${esc(hint)}</em>` : ""}${ctl}`;
    wrap.querySelector("input,select").addEventListener("change", e => {
      const nv = castLike(value, e.target.value); setP(data(cur), path, nv); value = nv; save(cur);
      const ey = $("resp-main").querySelector(".rp-title .eyebrow"); if (ey && !/DEĞİŞTİ/.test(ey.textContent)) ey.textContent += edits[cur] ? " · DEĞİŞTİ" : "";
    });
    return wrap;
  }

  function renderQuick(body) {
    const d = data(cur); const box = document.createElement("div"); box.className = "rp-form";
    const pre = PRESETS[cur];
    if (pre) {
      const t = document.createElement("div"); t.className = "rp-sub"; t.textContent = "ÖNEMLİ ALANLAR"; box.appendChild(t);
      pre.forEach(([label, p]) => { const v = getP(d, p); if (v !== undefined && typeof v !== "object") box.appendChild(field(p.split("."), v, label)); });
    }
    const t2 = document.createElement("div"); t2.className = "rp-sub"; t2.textContent = "ÜST DÜZEY DEĞERLER"; box.appendChild(t2);
    let c = 0;
    const walk = (o, pa, depth) => {
      if (o && typeof o === "object" && !Array.isArray(o)) for (const k of Object.keys(o)) {
        if (hid(pa, k)) continue;
        const v = o[k];
        if (v !== null && typeof v === "object") { if (depth < 1 && !Array.isArray(v)) walk(v, [...pa, k], depth + 1); }
        else if (c < 80 && v !== null) { c++; box.appendChild(field([...pa, k], v, [...pa, k].join(" › "))); }
      }
    };
    walk(d, [], 0);
    if (!c) { const e = document.createElement("div"); e.className = "rp-none"; e.textContent = "Üst düzeyde değer yok – Ağaç sekmesini kullanın."; box.appendChild(e); }
    body.appendChild(box);
  }

  function renderTree(body) {
    const d = data(cur); const root = document.createElement("div"); root.className = "rp-tree"; body.appendChild(root);
    const draw = (container, obj, path) => {
      const keys = (Array.isArray(obj) ? obj.map((_, i) => i) : Object.keys(obj)).filter(k => !hid(path, k));
      const LIM = 40; let shown = 0;
      const more = () => {
        keys.slice(shown, shown + LIM).forEach(k => container.insertBefore(row(obj, k, path), btn));
        shown = Math.min(keys.length, shown + LIM); if (shown >= keys.length) btn.remove(); else btn.textContent = `Daha fazla (${keys.length - shown})`;
      };
      const btn = document.createElement("button"); btn.type = "button"; btn.className = "text-button"; container.appendChild(btn);
      more();
    };
    const row = (obj, k, path) => {
      const v = obj[k]; const p = [...path, k];
      if (v !== null && typeof v === "object") {
        const det = document.createElement("details"); const sm = document.createElement("summary");
        sm.innerHTML = `<b>${esc(k)}</b><small>${Array.isArray(v) ? "[" + v.length + "]" : "{" + Object.keys(v).length + "}"}</small>`;
        const del = document.createElement("button"); del.type = "button"; del.className = "rp-x"; del.textContent = "×"; del.title = T("Sil");
        del.addEventListener("click", e => { e.preventDefault(); if (!confirm(isEn() ? `Delete "${k}"?` : `"${k}" silinsin mi?`)) return; Array.isArray(obj) ? obj.splice(k, 1) : delete obj[k]; save(cur); renderMain(); });
        sm.appendChild(del); det.appendChild(sm);
        const inner = document.createElement("div"); inner.className = "rp-inner"; det.appendChild(inner);
        det.addEventListener("toggle", () => { if (det.open && !inner.hasChildNodes()) { draw(inner, v, p); if (!Array.isArray(v)) addKey(inner, v); } });
        return det;
      }
      return field(p, v, String(k));
    };
    const addKey = (inner, o) => {
      const b = document.createElement("button"); b.type = "button"; b.className = "text-button"; b.textContent = "+ anahtar ekle";
      b.addEventListener("click", () => { const k = prompt(T("Anahtar adı:")); if (!k) return; const raw = prompt(T("Değer (JSON, ör. 5, \"YES\", true):"), '""'); try { o[k] = JSON.parse(raw); save(cur); renderMain(); } catch (e) { toast(T("Geçersiz JSON")); } });
      inner.appendChild(b);
    };
    draw(root, d, []);
  }

  function renderFind(body) {
    body.innerHTML = `<label class="search-box"><span>⌕</span><input id="resp-q" placeholder="Anahtar veya değer ara (ör. gp, coin, YES)…" autocomplete="off"></label><div id="resp-hits" class="rp-form"></div>`;
    const run = () => {
      const q = $("resp-q").value.toLowerCase(); const out = $("resp-hits"); out.innerHTML = "";
      if (q.length < 2) { out.innerHTML = '<div class="rp-none">En az 2 karakter yazın</div>'; return; }
      let n = 0; const stack = [[data(cur), []]];
      while (stack.length && n < 120) {
        const [o, p] = stack.pop();
        for (const k of (Array.isArray(o) ? o.map((_, i) => i).reverse() : Object.keys(o).reverse())) {
          if (hid(p, k)) continue;
          const v = o[k]; const pp = [...p, k];
          if (v !== null && typeof v === "object") stack.push([v, pp]);
          else if (v !== null && (String(k).toLowerCase().includes(q) || String(v).toLowerCase().includes(q))) { if (n++ < 120) out.appendChild(field(pp, v, pp.join(" › "))); }
        }
      }
      if (!n) out.innerHTML = '<div class="rp-none">Eşleşme yok</div>'; else if (n >= 120) { const m = document.createElement("div"); m.className = "rp-none"; m.textContent = "İlk 120 sonuç gösteriliyor – aramayı daraltın."; out.appendChild(m); }
    };
    let tm; $("resp-q").addEventListener("input", () => { clearTimeout(tm); tm = setTimeout(run, 250); });
  }

  function renderJson(body) {
    const full = data(cur); const stash = {};
    const view = Array.isArray(full) ? full : Object.assign({}, full);
    if (!Array.isArray(full)) HIDE.forEach(k => { if (k in view) { stash[k] = view[k]; delete view[k]; } });
    const text = JSON.stringify(view, null, 2);
    body.innerHTML = `<textarea id="resp-raw" class="rp-raw" spellcheck="false"></textarea><div class="rp-actions"><button class="button button-primary" id="resp-apply" type="button">Uygula</button><span id="resp-raw-msg"></span></div>`;
    $("resp-raw").value = text.length > 600000 ? text.slice(0, 600000) + "\n…" : text;
    if (text.length > 600000) { $("resp-raw").readOnly = true; $("resp-apply").disabled = true; $("resp-raw-msg").textContent = "Dosya büyük: Ham düzenleme kapalı, Ağaç/Ara kullanın."; }
    $("resp-apply").addEventListener("click", () => {
      try { const nv = JSON.parse($("resp-raw").value); if (nv && !Array.isArray(nv)) Object.assign(nv, stash); work[cur] = nv; save(cur); $("resp-raw-msg").textContent = "✓ Uygulandı"; }
      catch (e) { $("resp-raw-msg").textContent = "Hata: " + e.message; }
    });
  }

  /* ---------- dışa aktarım ---------- */
  function exportEdits() {
    const names = Object.keys(edits);
    if (!names.length) { toast(T("Değişiklik yok")); return; }
    const files = {}; names.forEach(n => files[n] = JSON.stringify(edits[n], null, 2) + "\n");
    if (typeof Android !== "undefined" && Android.exportZip) Android.exportZip(JSON.stringify(files), T("Yanıtlar dışa aktarıldı"), T("Dosya hatası"));
    else { const a = document.createElement("a"); a.href = "data:application/json;charset=utf-8," + encodeURIComponent(JSON.stringify(files)); a.download = "responses-edited.json"; a.click(); }
  }

  /* ---------- sekmeler ---------- */
  function tabs() {
    const set = (v) => {
      $("view-packs").hidden = v !== "packs"; $("view-resp").hidden = v !== "resp"; if ($("view-players")) $("view-players").hidden = v !== "players"; if ($("view-squad")) $("view-squad").hidden = v !== "squad"; document.dispatchEvent(new CustomEvent("opm-tab", { detail: v }));
      document.querySelectorAll(".nav-tab").forEach(b => b.classList.toggle("on", b.dataset.v === v));
      if (v === "resp" && !$("view-resp").hasChildNodes()) build();
      window.scrollTo(0, 0);
    };
    document.querySelectorAll(".nav-tab").forEach(b => b.addEventListener("click", () => set(b.dataset.v)));
  }
  tabs();
  document.addEventListener("opm-lang", () => {
    if ($("view-resp").hasChildNodes()) { renderList(); if (cur) renderMain(); }
  });
})();
