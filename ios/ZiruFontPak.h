// Vietnamese UI font for eFootball iOS 11.1.1.
//
// The iOS fonts (PesMobile/Content/Platforms/_IOS/Assets/ui/Font/eFootballSans-*.ufont, eFootballStencil-Regular.ufont
// in pc0000_mobile_ios.pak) have no glyphs for U+1EA0–U+1EF9, so "Tiếng Việt" renders as "Ting Vit". Ziru's Android
// mod fixes this with ZFont_Orginal.pak (same four .ufont files with full Vietnamese coverage). build_ipa.py turns
// that pak into ziru/ZiruFont_IOS_P.pak (mount point _IOS) inside the IPA; this module mounts it with a higher
// pak order right after the game mounts its own first pak, by replacing the UEBridgeSystem::MountPakFile vtable
// slot. Nothing in the imported game data is changed.
#pragma once

namespace zfont {
// pakPath: absolute path of ZiruFont_IOS_P.pak in the app bundle. log: line sink (may be null).
void install(const char* pakPath, void (*log)(const char* line));
}  // namespace zfont
