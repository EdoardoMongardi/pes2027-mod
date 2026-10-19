"use strict";
(function () {
  var KEY = "efootbase_theme_v1", t0 = Date.now(), root = document.documentElement;
  var PRESETS = [[192, 100, 50], [215, 100, 56], [16, 100, 62], [340, 100, 62], [275, 90, 64], [235, 90, 68], [200, 90, 56], [168, 80, 50], [95, 70, 52], [46, 100, 58]];
  var st = { h: 192, s: 100, l: 50 };
  try { var sv = JSON.parse(localStorage.getItem(KEY)); if (sv && sv.h != null) st = sv; } catch (e) {}
  function hsl2rgb(h, s, l) {
    s /= 100; l /= 100; var k = function (n) { return (n + h / 30) % 12; }, a = s * Math.min(l, 1 - l);
    var f = function (n) { return Math.round(255 * (l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1))))); };
    return [f(0), f(8), f(4)];
  }
  function apply() {
    var a = hsl2rgb(st.h, st.s, st.l), b = hsl2rgb((st.h + 30) % 360, st.s, Math.min(st.l + 12, 78));
    var hex = function (c) { return "#" + c.map(function (v) { return ("0" + v.toString(16)).slice(-2); }).join(""); };
    var lum = 0.299 * a[0] + 0.587 * a[1] + 0.114 * a[2];
    var s = root.style;
    s.setProperty("--ember", hex(a)); s.setProperty("--gold", hex(b));
    s.setProperty("--e", a.join(",")); s.setProperty("--g", b.join(","));
    s.setProperty("--on", lum > 150 ? "#14100a" : "#ffffff");
    var m = document.querySelector('meta[name="theme-color"]'); if (m) m.setAttribute("content", "#07070b");
  }
  function save() { try { localStorage.setItem(KEY, JSON.stringify(st)); } catch (e) {} }
  apply();

  function ready() {
    var hdr = document.querySelector(".header-actions");
    var btn = document.createElement("button"); btn.type = "button"; btn.className = "theme-btn"; btn.setAttribute("aria-label", "Tema"); btn.innerHTML = "<i></i>";
    if (hdr) hdr.insertBefore(btn, hdr.firstChild);
    var bg = document.createElement("div"); bg.className = "sheet-bg";
    var sh = document.createElement("div"); sh.className = "sheet";
    sh.innerHTML = '<div class="grab"></div><h3>Tema rengi</h3><p>Uygulamanın vurgu rengini seç. Anında uygulanır ve hatırlanır.</p><div class="swatches"></div>' +
      '<div class="sl"><label><span>TON</span></label><input type="range" id="th-h" min="0" max="359"></div>' +
      '<div class="sl"><label><span>DOYGUNLUK</span></label><input type="range" id="th-s" min="20" max="100"></div>' +
      '<div class="sl"><label><span>PARLAKLIK</span></label><input type="range" id="th-l" min="35" max="75"></div>' +
      '<div class="sheet-actions"><button class="button button-secondary" id="th-reset" type="button">Varsayılan</button><button class="button button-primary" id="th-ok" type="button">Tamam</button></div>';
    document.body.appendChild(bg); document.body.appendChild(sh);
    var sw = sh.querySelector(".swatches"), H = sh.querySelector("#th-h"), S = sh.querySelector("#th-s"), L = sh.querySelector("#th-l");
    function paintBars() {
      var hs = []; for (var i = 0; i <= 12; i++) hs.push("hsl(" + i * 30 + ",100%,60%)");
      H.style.setProperty("--bar", "linear-gradient(90deg," + hs.join(",") + ")");
      S.style.setProperty("--bar", "linear-gradient(90deg,hsl(" + st.h + ",20%," + st.l + "%),hsl(" + st.h + ",100%," + st.l + "%))");
      L.style.setProperty("--bar", "linear-gradient(90deg,hsl(" + st.h + "," + st.s + "%,35%),hsl(" + st.h + "," + st.s + "%,75%))");
    }
    function sync() {
      H.value = st.h; S.value = st.s; L.value = st.l; paintBars();
      [].forEach.call(sw.children, function (e, i) { var p = PRESETS[i]; e.classList.toggle("on", p[0] === st.h && p[1] === st.s && p[2] === st.l); });
    }
    PRESETS.forEach(function (p) {
      var e = document.createElement("button"); e.type = "button"; e.className = "swatch"; e.style.background = "hsl(" + p[0] + "," + p[1] + "%," + p[2] + "%)";
      e.addEventListener("click", function () { st = { h: p[0], s: p[1], l: p[2] }; apply(); save(); sync(); });
      sw.appendChild(e);
    });
    [[H, "h"], [S, "s"], [L, "l"]].forEach(function (x) { x[0].addEventListener("input", function () { st[x[1]] = +x[0].value; apply(); save(); sync(); }); });
    sh.querySelector("#th-reset").addEventListener("click", function () { st = { h: 16, s: 100, l: 62 }; apply(); save(); sync(); });
    var open = function (v) { bg.classList.toggle("open", v); sh.classList.toggle("open", v); if (v) sync(); };
    btn.addEventListener("click", function () { open(true); });
    bg.addEventListener("click", function () { open(false); }); sh.querySelector("#th-ok").addEventListener("click", function () { open(false); });
    document.addEventListener("contextmenu", function (e) { if (!/INPUT|TEXTAREA/.test(e.target.tagName)) e.preventDefault(); });
  }
  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", ready); else ready();

  
  function dismissIntro() {
    var sp = document.getElementById("splash");
    if (!sp || sp.classList.contains("out")) return;
    sp.classList.add("out");
    setTimeout(function () { if (sp.parentNode) sp.parentNode.removeChild(sp); }, 650);
  }

  function initIntroVideo() {
    var vid = document.getElementById("intro-video");
    var skip = document.getElementById("skip-intro");
    if (vid) {
      vid.addEventListener("ended", dismissIntro);
      vid.addEventListener("error", dismissIntro);
      vid.addEventListener("click", dismissIntro);
    }
    if (skip) {
      skip.addEventListener("click", dismissIntro);
    }
    setTimeout(dismissIntro, 6500); // 6.5s maximum fallback
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initIntroVideo);
  } else {
    initIntroVideo();
  }

})();