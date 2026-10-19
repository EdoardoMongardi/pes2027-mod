// ios_bridge.js - iOS Safari & Web Bridge for eFootbase Editor
(function() {
  console.log("[eFootbase] Initializing iOS / Web Bridge...");

  let bannerInput = document.createElement("input");
  bannerInput.type = "file";
  bannerInput.id = "hidden-banner-input";
  bannerInput.accept = "image/png,image/jpeg,image/webp";
  bannerInput.style.display = "none";

  let importInput = document.createElement("input");
  importInput.type = "file";
  importInput.id = "hidden-import-input";
  importInput.accept = ".json,.zip";
  importInput.style.display = "none";

  function ensureInputs() {
    if (!document.getElementById("hidden-banner-input") && document.body) {
      document.body.appendChild(bannerInput);
    }
    if (!document.getElementById("hidden-import-input") && document.body) {
      document.body.appendChild(importInput);
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", ensureInputs);
  } else {
    ensureInputs();
  }

  bannerInput.addEventListener("change", function(e) {
    const file = e.target.files && e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = function(evt) {
      if (typeof window.handleImportedBanner === "function") {
        window.handleImportedBanner(evt.target.result);
      }
    };
    reader.readAsDataURL(file);
    bannerInput.value = "";
  });

  importInput.addEventListener("change", function(e) {
    const file = e.target.files && e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = function(evt) {
      if (typeof window.handleImportedFile === "function") {
        window.handleImportedFile(evt.target.result);
      }
    };
    reader.readAsText(file);
    importInput.value = "";
  });

  function downloadBlob(content, filename, mimeType) {
    const blob = new Blob([content], { type: mimeType || "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    setTimeout(function() {
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }, 200);
  }

  window.Android = {
    getCatalog: function() {
      if (window.__CATALOG_RAW__) {
        return JSON.stringify(window.__CATALOG_RAW__);
      }
      return "{}";
    },

    getSavedState: function() {
      try {
        return localStorage.getItem("opm_saved_state") || "";
      } catch (e) {
        return "";
      }
    },

    saveState: function(stateStr) {
      try {
        localStorage.setItem("opm_saved_state", stateStr);
      } catch (e) {
        console.error("Save state failed:", e);
      }
    },

    selectBanner: function(errorMsg) {
      ensureInputs();
      bannerInput.click();
    },

    importFile: function(errorMsg) {
      ensureInputs();
      importInput.click();
    },

    exportBackup: function(content, filename, successMsg, errorMsg) {
      try {
        downloadBlob(content, filename || "offline-pack-manager-backup.json", "application/json");
        if (typeof showToast === "function") showToast(successMsg || "Đã lưu bản sao lưu");
      } catch (e) {
        if (typeof showToast === "function") showToast(errorMsg || "Lỗi lưu sao lưu");
      }
    },

    exportZip: function(jsonFilesStr, successMsg, errorMsg) {
      console.log("[eFootbase] exportZip called");
      // Try sending to local Python server for direct iPhone USB push & IPA build
      fetch("/api/export", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: jsonFilesStr
      })
      .then(function(res) { return res.json(); })
      .then(function(data) {
        if (data && data.ok) {
          let msg = "✓ Đã lưu gói mod!";
          if (data.pushed_to_device) {
            msg += " Đã nạp trực tiếp vào iPhone qua USB!";
          }
          if (typeof showToast === "function") showToast(msg);
        } else {
          downloadBlob(jsonFilesStr, "pes_export_files.json", "application/json");
          showExportGuideModal();
        }
      })
      .catch(function(err) {
        downloadBlob(jsonFilesStr, "pes_export_files.json", "application/json");
        showExportGuideModal();
      });
    }
  };

  function showExportGuideModal() {
    let oldModal = document.getElementById("export-guide-modal");
    if (oldModal) oldModal.remove();

    let modal = document.createElement("div");
    modal.id = "export-guide-modal";
    modal.style.cssText = "position:fixed;inset:0;z-index:999999;background:rgba(0,0,0,0.8);backdrop-filter:blur(8px);display:flex;align-items:center;justify-content:center;padding:16px;";
    modal.innerHTML = `
      <div style="background:#0c1324;border:1.5px solid rgba(0,229,255,0.4);border-radius:20px;max-width:440px;width:100%;padding:24px;box-shadow:0 12px 40px rgba(0,0,0,0.8);color:#fff;font-family:sans-serif;">
        <div style="font-size:18px;font-weight:800;color:#00e5ff;margin-bottom:12px;display:flex;align-items:center;gap:8px;">
          <span>✓</span> Đã xuất cấu hình thành công!
        </div>
        <p style="font-size:13px;line-height:1.5;color:#94a3b8;margin-bottom:16px;">
          File cấu hình đã được tải xuống máy của bạn. Vì chính sách bảo mật Sandbox của iOS, trình duyệt Safari không thể tự ý ghi đè vào ứng dụng game khác.
        </p>
        <div style="background:rgba(15,23,42,0.8);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:12px;margin-bottom:12px;">
          <div style="font-size:13px;font-weight:700;color:#10b981;margin-bottom:4px;">Cách 1: Nạp tự động 1-click qua PC (Khuyên dùng)</div>
          <div style="font-size:12px;color:#cbd5e1;line-height:1.4;">
            Cắm cáp iPhone vào PC, nháy đúp chạy file <strong>Nap_RoBeo_Zidane.bat</strong> trên máy tính. Dữ liệu sẽ tự động nạp thẳng vào game trong 3 giây!
          </div>
        </div>
        <div style="background:rgba(15,23,42,0.8);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:12px;margin-bottom:16px;">
          <div style="font-size:13px;font-weight:700;color:#38bdf8;margin-bottom:4px;">Cách 2: Nạp trên iPhone (Filza / TrollStore)</div>
          <div style="font-size:12px;color:#cbd5e1;line-height:1.4;">
            Sao chép file <code>pes_export_files.json</code> vừa tải vào thư mục <strong>Documents</strong> của game eFootball.
          </div>
        </div>
        <button id="close-export-guide" type="button" style="width:100%;height:46px;background:linear-gradient(135deg,#00e5ff,#3b82f6);color:#041019;border:none;border-radius:12px;font-weight:800;font-size:14px;cursor:pointer;">
          Đã hiểu ›
        </button>
      </div>
    `;
    document.body.appendChild(modal);
    document.getElementById("close-export-guide").addEventListener("click", function() {
      modal.remove();
    });
  }

  console.log("[eFootbase] iOS / Web Bridge ready!");
})();
