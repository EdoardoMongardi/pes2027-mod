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
          if (typeof showToast === "function") showToast(successMsg || "Đã xuất file cấu hình!");
        }
      })
      .catch(function(err) {
        downloadBlob(jsonFilesStr, "pes_export_files.json", "application/json");
        if (typeof showToast === "function") showToast(successMsg || "Đã xuất file cấu hình!");
      });
    }
  };

  console.log("[eFootbase] iOS / Web Bridge ready!");
})();
