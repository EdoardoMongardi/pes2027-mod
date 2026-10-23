// Host test for core/folderroute.hpp: builds fake app containers in a temp folder and runs the
// launch sequence of ios/ZiruOffline.mm checkDirectFolderData (top-level links, then the bundled
// livedata / pak_extra / livedata->pak link loops) with the old search and with zfr::resolve.
//   c++ -std=c++17 -Wall -o build/folderroutetest test/folderroutetest.cpp && ./build/folderroutetest
#include "../core/folderroute.hpp"

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>

static int gFails = 0, gOks = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (cond) { gOks++; std::cout << "  ok    " << msg << "\n"; }              \
    else { gFails++; std::cout << "  FAIL  " << msg << " (line " << __LINE__ << ")\n"; } \
  } while (0)

static void mkdirs(const std::string& p) {
  std::string cur;
  for (size_t i = 0; i <= p.size(); i++) {
    if (i == p.size() || (p[i] == '/' && i > 0)) mkdir(p.substr(0, i).c_str(), 0755);
  }
}
static void writeFile(const std::string& p, size_t n, char fill) {
  mkdirs(p.substr(0, p.rfind('/')));
  std::ofstream o(p, std::ios::binary);
  o << std::string(n, fill);
}
static std::string readAll(const std::string& p) {
  std::ifstream i(p, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(i)), {});
}
static void rmrf(const std::string& p) {
  struct stat st;
  if (lstat(p.c_str(), &st) != 0) return;
  if (S_ISDIR(st.st_mode)) {
    for (auto& n : zfr::listDir(p)) rmrf(p + "/" + n);
    rmdir(p.c_str());
  } else unlink(p.c_str());
}
static std::string readlinkStr(const std::string& p) {
  char b[PATH_MAX];
  ssize_t r = readlink(p.c_str(), b, sizeof b - 1);
  return r > 0 ? std::string(b, (size_t)r) : "";
}
static bool exists(const std::string& p) { struct stat st; return stat(p.c_str(), &st) == 0; }

// Every real (non-link) file under root: path -> content. Used to prove nothing real is lost.
static void realFiles(const std::string& root, std::map<std::string, std::string>& out, const std::string& rel = "") {
  for (auto& n : zfr::listDir(root + rel)) {
    std::string r = rel + "/" + n, p = root + r;
    struct stat st;
    if (lstat(p.c_str(), &st) != 0) continue;
    if (S_ISDIR(st.st_mode)) realFiles(root, out, r);
    else if (S_ISREG(st.st_mode)) out[r] = readAll(p);
  }
}
// Whole tree including links (path -> "L:<text>" or "F:<size>"), to prove a re-run changes nothing.
static void tree(const std::string& root, std::map<std::string, std::string>& out, const std::string& rel = "") {
  for (auto& n : zfr::listDir(root + rel)) {
    std::string r = rel + "/" + n, p = root + r;
    struct stat st;
    if (lstat(p.c_str(), &st) != 0) continue;
    if (S_ISLNK(st.st_mode)) out[r] = "L:" + readlinkStr(p);
    else if (S_ISDIR(st.st_mode)) { out[r] = "D"; tree(root, out, r); }
    else out[r] = "F:" + std::to_string((long long)st.st_size);
  }
}
static int danglingIn(const std::string& dir) {
  int n = 0;
  for (auto& f : zfr::listDir(dir)) {
    std::string p = dir + "/" + f;
    if (zfr::isLink(p) && !exists(p)) n++;
  }
  return n;
}

// Same rule as zm::stadiumName (core/modpack.cpp): pc30xx_ stadium paks, except pc3001 and pc3011.
static bool stadiumName(const std::string& n) {
  return n.size() > 6 && n.compare(0, 4, "pc30") == 0 && n.compare(0, 6, "pc3001") != 0 && n.compare(0, 6, "pc3011") != 0 &&
         n.find_first_not_of("0123456789", 3) == 6 && n[6] == '_';
}
// Same specs as ios/ZiruOffline.mm.
static zfr::Spec pakSpec() {
  zfr::Spec s{"pak", {"global_mobile_ios.utoc", "global_mobile_and.utoc", "pc0000_mobile_ios.ucas", "pc0000_mobile_and.ucas",
                      "pc0000_mobile_ios.pak", "pc0000_mobile_and.pak"}, false};
  s.notLinkedSingly = stadiumName;
  return s;
}
static zfr::Spec cpkSpec() { return {"cpk", {"dt200_mobile_all.cpk", "dt230_mobile_all.cpk"}, false}; }
static zfr::Spec livedataSpec() { return {"livedata", {}, true}; }

// ------------------------------------------------------------------ fake device
struct World {
  std::string root, docs, bundle;
};
static std::string gBase;
static World makeWorld(const std::string& name) {
  World w;
  w.root = gBase + "/" + name;
  rmrf(w.root);
  w.docs = w.root + "/Data/C1/Documents";
  w.bundle = w.root + "/Bundle/B1/PESmobile.app";
  mkdirs(w.docs);
  for (auto ext : {".pak", ".ucas", ".utoc"}) writeFile(w.bundle + "/ziru/pak_extra/pc7999_mobile_ios" + ext, 2000, 'x');
  writeFile(w.bundle + "/ziru/livedata/pc9242_mobile_and.pak", 365, 'p');
  writeFile(w.bundle + "/ziru/livedata/pc9242_mobile_and.ucas", 5000, 'u');
  writeFile(w.bundle + "/ziru/livedata/dt870_mobile_and.cpk", 9000, 'F');  // table-fixed copy
  return w;
}
static void addUserData(const std::string& docs, const std::string& folder, const std::string& pakName = "pak") {
  std::string d = docs + "/" + folder;
  writeFile(d + "/" + pakName + "/global_mobile_ios.utoc", 3000, 'g');
  writeFile(d + "/" + pakName + "/global_mobile_ios.ucas", 3000, 'g');
  writeFile(d + "/" + pakName + "/pc0000_mobile_ios.pak", 365, 'b');
  writeFile(d + "/" + pakName + "/pc0000_mobile_ios.ucas", 4000, 'b');
  writeFile(d + "/" + pakName + "/pc0000_mobile_ios.utoc", 4000, 'b');
  writeFile(d + "/cpk/dt200_mobile_all.cpk", 6000, 'c');
  writeFile(d + "/cpk/dt230_mobile_all.cpk", 6000, 'c');
  writeFile(d + "/livedata/pc9242_mobile_and.pak", 365, 'p');  // same bytes as the bundle copy
  writeFile(d + "/livedata/pc9242_mobile_and.ucas", 5000, 'u');
  writeFile(d + "/livedata/dt870_mobile_and.cpk", 9000, 'O');  // original (unfixed) table
}

// ------------------------------------------------------------------ 75288ee behaviour (model)
// findFolderNamed: depth-first, first match in listing order (APFS order is not alphabetical).
static std::string oldFind(const std::string& root, const std::string& name, int maxDepth, bool reverse) {
  if (maxDepth < 0) return "";
  auto items = zfr::listDir(root);
  if (reverse) std::reverse(items.begin(), items.end());
  for (auto& item : items) {
    std::string full = root + "/" + item;
    if (!zfr::isDirFollow(full)) continue;
    if (zfr::lower(item) == zfr::lower(name)) return full;
    std::string f = oldFind(full, name, maxDepth - 1, reverse);
    if (!f.empty()) return f;
  }
  return "";
}
// ensureSymlink (ZiruOffline.mm:1084-1136), same decisions, absolute link text.
static std::vector<std::string> gDeletedByEnsure;
static bool gHardened = false;
static void oldEnsure(const std::string& link, const std::string& target) {
  if (link == target || !exists(target)) return;
  if (zfr::sameFile(link, target) && !zfr::isLink(link)) return;
  std::string cur = readlinkStr(link);
  if (!cur.empty() && cur == target) return;
  struct stat st;
  if (stat(link.c_str(), &st) == 0) {
    bool isDir = S_ISDIR(st.st_mode);
    if (!isDir && cur.empty()) {
      if (st.st_size > 1024) return;  // DATA_PRESERVE
      if (gHardened && readAll(link) == readAll(target)) return;  // optional hardening: same bytes, keep the real file
      gDeletedByEnsure.push_back(link);
      unlink(link.c_str());
    }
    if (isDir && cur.empty()) {
      if (!zfr::listDir(link).empty()) return;  // "non-empty directory, will not overwrite"
      rmdir(link.c_str());
    }
    if (!cur.empty()) unlink(link.c_str());
  } else if (!cur.empty()) {
    unlink(link.c_str());
  }
  symlink(target.c_str(), link.c_str());
}
static void oldTopLevel(const std::string& docs, bool reverse) {
  for (auto n : {"pak", "cpk", "livedata"}) {
    std::string found = oldFind(docs, n, 3, reverse), link = docs + "/" + n;
    if (!found.empty() && found != link) oldEnsure(link, found);
  }
}
// The link loops that follow in checkDirectFolderData (bundled livedata, pak_extra, stale stadium links,
// livedata->pak).
static void modLoops(const World& w) {
  std::string live = w.docs + "/livedata", pak = w.docs + "/pak";
  if (!exists(live) && !zfr::isLink(live)) mkdir(live.c_str(), 0755);
  for (auto& f : zfr::listDir(w.bundle + "/ziru/livedata")) oldEnsure(live + "/" + f, w.bundle + "/ziru/livedata/" + f);
  if (!exists(pak) && !zfr::isLink(pak)) mkdir(pak.c_str(), 0755);
  for (auto& f : zfr::listDir(w.bundle + "/ziru/pak_extra")) oldEnsure(pak + "/" + f, w.bundle + "/ziru/pak_extra/" + f);
  for (auto& f : zfr::listDir(pak))
    if (stadiumName(f) && zfr::isLink(pak + "/" + f)) unlink((pak + "/" + f).c_str());
  for (auto& f : zfr::listDir(live)) {
    std::string l = zfr::lower(f);
    if (l.size() > 4 && (l.rfind(".pak") == l.size() - 4 || l.rfind(".ucas") == l.size() - 5 || l.rfind(".utoc") == l.size() - 5))
      oldEnsure(pak + "/" + f, live + "/" + f);
  }
}
static bool oldReady(const std::string& docs) {
  return zfr::isDirFollow(docs + "/pak") && zfr::isDirFollow(docs + "/cpk") && !zfr::listDir(docs + "/pak").empty() &&
         !zfr::listDir(docs + "/cpk").empty();
}
static bool engineBoots(const std::string& docs) {
  struct stat st;
  return stat((docs + "/pak/global_mobile_ios.utoc").c_str(), &st) == 0 && stat((docs + "/pak/pc0000_mobile_ios.ucas").c_str(), &st) == 0 &&
         stat((docs + "/cpk/dt200_mobile_all.cpk").c_str(), &st) == 0;
}

// ------------------------------------------------------------------ proposed behaviour
struct Launch {
  zfr::Result pak, cpk, live;
  bool ready = false;
};
static Launch newLaunch(const World& w, bool verbose = false) {
  zfr::LogFn log = [&](const std::string& s) { if (verbose) std::cout << "        [DATA_LINK] " << s << "\n"; };
  Launch L;
  // Same order as prepareDataFolderLinks: cpk and livedata prefer the data folder the pak folder came from.
  L.pak = zfr::resolve(w.docs, pakSpec(), log);
  std::string near = zfr::dataParent(L.pak);
  L.cpk = zfr::resolve(w.docs, cpkSpec(), log, near);
  L.live = zfr::resolve(w.docs, livedataSpec(), log, near);
  for (auto n : {"pak", "cpk", "livedata"}) zfr::pruneDanglingLinks(w.docs + "/" + n, log);
  modLoops(w);
  L.ready = oldReady(w.docs) && zfr::hasMarker(w.docs + "/pak", pakSpec(), false);
  return L;
}
static bool oldLaunch(const World& w, bool reverse) {
  oldTopLevel(w.docs, reverse);
  modLoops(w);
  return oldReady(w.docs);
}
static bool userFilesKept(const std::map<std::string, std::string>& before, const std::map<std::string, std::string>& after) {
  for (auto& [p, c] : before) {
    auto it = after.find(p);
    if (it == after.end() || it->second != c) { std::cout << "        lost/changed: " << p << "\n"; return false; }
  }
  return true;
}

int main(int argc, char** argv) {
  char tmpl[] = "/tmp/zfr-test-XXXXXX";
  gBase = argc > 1 ? argv[1] : mkdtemp(tmpl);
  mkdirs(gBase);
  std::cout << "work folder " << gBase << "\n";

  std::cout << "\n[1] 75288ee, launched once before data, then PES2027-data added (both listing orders)\n";
  for (bool reverse : {false, true}) {
    World w = makeWorld(std::string("old-C-") + (reverse ? "rev" : "fwd"));
    bool r1 = oldLaunch(w, reverse);
    addUserData(w.docs, "PES2027-data");
    bool r2 = oldLaunch(w, reverse);
    CHECK(!r1, std::string("old first launch not ready (") + (reverse ? "reverse" : "forward") + " order)");
    CHECK(r2 && !engineBoots(w.docs), "old second launch says ready but Documents/pak has no base paks (black screen reproduced)");
    CHECK(!zfr::isLink(w.docs + "/pak") && zfr::isLink(w.docs + "/cpk"), "old: cpk linked, pak still the mod-made folder");
  }

  std::cout << "\n[2] C: proposed code, launched once before data, then PES2027-data added\n";
  {
    World w = makeWorld("new-C");
    Launch a = newLaunch(w);
    CHECK(!a.ready, "first launch not ready");
    CHECK(zfr::isDirFollow(w.docs + "/pak") && !zfr::isLink(w.docs + "/pak") && !zfr::isLink(w.docs + "/livedata"),
          "first launch leaves mod-made real Documents/pak and Documents/livedata (the trigger)");
    addUserData(w.docs, "PES2027-data");
    std::map<std::string, std::string> before, mid;
    realFiles(w.root, before);
    zfr::Result p = zfr::resolve(w.docs, pakSpec(), [](const std::string& s) { std::cout << "        [DATA_LINK] " << s << "\n"; });
    zfr::Result l = zfr::resolve(w.docs, livedataSpec(), [](const std::string& s) { std::cout << "        [DATA_LINK] " << s << "\n"; });
    realFiles(w.root, mid);
    CHECK(p.action == zfr::Action::Converted && l.action == zfr::Action::Converted, "pak and livedata converted to links");
    CHECK(userFilesKept(before, mid) && before.size() == mid.size(), "conversion keeps every real file (bundle + user data)");
    CHECK(readlinkStr(w.docs + "/pak") == "PES2027-data/pak", "Documents/pak -> PES2027-data/pak (relative)");
    CHECK(!exists(w.docs + "/.pak.ziru-convert") && !zfr::isLink(w.docs + "/.pak.ziru-convert"), "staging folder removed");
    Launch b = newLaunch(w);
    CHECK(b.ready && engineBoots(w.docs), "second launch ready and the engine sees the base paks");
    CHECK(exists(w.docs + "/pak/pc7999_mobile_ios.pak"), "bundled extra pak still visible in Documents/pak");
    std::map<std::string, std::string> t1, t2;
    tree(w.root, t1);
    Launch c = newLaunch(w);
    tree(w.root, t2);
    CHECK(c.pak.action == zfr::Action::KeptLink && c.live.action == zfr::Action::KeptLink && c.cpk.action == zfr::Action::KeptLink,
          "third launch: nothing to do");
    CHECK(t1 == t2, "third launch leaves the whole tree identical (idempotent)");
  }

  std::cout << "\n[3] A: fresh install, data folder present on first launch\n";
  {
    World w = makeWorld("new-A");
    addUserData(w.docs, "PES2027-data");
    Launch a = newLaunch(w);
    CHECK(a.pak.action == zfr::Action::Linked && a.cpk.action == zfr::Action::Linked && a.live.action == zfr::Action::Linked, "all three linked");
    CHECK(a.ready && engineBoots(w.docs), "ready and boots");
    World o = makeWorld("old-A");
    addUserData(o.docs, "PES2027-data");
    CHECK(oldLaunch(o, false) && engineBoots(o.docs), "old code also handles A (no regression baseline)");
  }

  std::cout << "\n[4] B: files copied straight into Documents/pak and Documents/cpk\n";
  {
    World w = makeWorld("new-B");
    addUserData(w.docs, "tmp");
    rename((w.docs + "/tmp/pak").c_str(), (w.docs + "/pak").c_str());
    rename((w.docs + "/tmp/cpk").c_str(), (w.docs + "/cpk").c_str());
    rmrf(w.docs + "/tmp");
    std::map<std::string, std::string> before, after;
    realFiles(w.root, before);
    Launch a = newLaunch(w);
    realFiles(w.root, after);
    CHECK(a.pak.action == zfr::Action::KeptReal && a.cpk.action == zfr::Action::KeptReal, "real folders kept");
    CHECK(!zfr::isLink(w.docs + "/pak") && a.ready && engineBoots(w.docs), "no top-level link, ready, boots");
    CHECK(userFilesKept(before, after), "no real file touched");
  }

  std::cout << "\n[5] C + a user mod installed before data (real file inside the mod-made Documents/pak)\n";
  {
    World w = makeWorld("new-C-mod");
    newLaunch(w);
    writeFile(w.docs + "/pak/pc9000_mobile_ios.pak", 2000, 'm');  // zm::activatePending put a real file here
    addUserData(w.docs, "PES2027-data");
    std::map<std::string, std::string> before, after;
    realFiles(w.root, before);
    zfr::Result p = zfr::resolve(w.docs, pakSpec(), [](const std::string& s) { std::cout << "        [DATA_LINK] " << s << "\n"; });
    realFiles(w.root, after);
    CHECK(p.action == zfr::Action::Merged && !zfr::isLink(w.docs + "/pak"), "folder kept, data files linked into it");
    CHECK(userFilesKept(before, after) && before.size() == after.size(), "mod file and user data intact");
    CHECK(readAll(w.docs + "/pak/pc9000_mobile_ios.pak") == std::string(2000, 'm'), "mod pak still a real file at the same path");
    Launch a = newLaunch(w, true);
    CHECK(a.ready && engineBoots(w.docs), "ready and boots");
    std::map<std::string, std::string> t1, t2;
    tree(w.root, t1);
    Launch b = newLaunch(w);
    tree(w.root, t2);
    CHECK(!b.pak.changed() && t1 == t2, "re-run changes nothing");
  }

  std::cout << "\n[6] D: reinstall (bundle path changes) after the fix ran\n";
  {
    World w = makeWorld("new-D");
    newLaunch(w);
    addUserData(w.docs, "PES2027-data");
    newLaunch(w);
    std::string nb = w.root + "/Bundle/B2/PESmobile.app";
    mkdirs(w.root + "/Bundle/B2");
    rename(w.bundle.c_str(), nb.c_str());
    w.bundle = nb;
    CHECK(danglingIn(w.docs + "/pak") > 0, "old bundle links are broken before the launch");
    Launch a = newLaunch(w);
    CHECK(a.ready && engineBoots(w.docs) && danglingIn(w.docs + "/pak") == 0 && danglingIn(w.docs + "/livedata") == 0,
          "boots, no broken links left");
    CHECK(exists(w.docs + "/pak/pc7999_mobile_ios.pak") && readlinkStr(w.docs + "/pak/pc7999_mobile_ios.pak").find("/B2/") != std::string::npos,
          "extra pak relinked into the new bundle");
  }

  std::cout << "\n[7] D: app container path changes (Documents moves)\n";
  {
    World w = makeWorld("new-D2");
    newLaunch(w);
    addUserData(w.docs, "PES2027-data");
    newLaunch(w);
    mkdirs(w.root + "/Data/C2");
    rename((w.root + "/Data/C1/Documents").c_str(), (w.root + "/Data/C2/Documents").c_str());
    w.docs = w.root + "/Data/C2/Documents";
    CHECK(zfr::isDirFollow(w.docs + "/pak"), "relative Documents/pak link still works after the move");
    Launch a = newLaunch(w);
    CHECK(a.ready && engineBoots(w.docs) && danglingIn(w.docs + "/pak") == 0, "boots, broken absolute links repaired");
  }

  std::cout << "\n[8] D: stale absolute Documents/pak link from an old build + data present\n";
  {
    World w = makeWorld("new-D3");
    addUserData(w.docs, "PES2027-data");
    symlink("/var/mobile/Containers/Data/Application/OLD/Documents/PES2027-data/pak", (w.docs + "/pak").c_str());
    Launch a = newLaunch(w);
    CHECK(a.pak.action == zfr::Action::Relinked && readlinkStr(w.docs + "/pak") == "PES2027-data/pak", "broken link moved to the data folder");
    CHECK(a.ready && engineBoots(w.docs), "boots");
  }

  std::cout << "\n[9] ZIP route: Documents/pak -> GameData pack (outside Documents) + a stray data folder\n";
  {
    World w = makeWorld("new-zip");
    std::string gd = w.root + "/Data/C1/Library/Application Support/PES2027/GameData/id1";
    addUserData(gd.substr(0, gd.rfind('/')), "id1");
    symlink((gd + "/pak").c_str(), (w.docs + "/pak").c_str());
    symlink((gd + "/cpk").c_str(), (w.docs + "/cpk").c_str());
    addUserData(w.docs, "PES2027-data");
    Launch a = newLaunch(w);
    CHECK(a.pak.action == zfr::Action::KeptLink && readlinkStr(w.docs + "/pak") == gd + "/pak", "working pack link kept (no flip-flop)");
    CHECK(a.ready && engineBoots(w.docs), "boots");
  }

  std::cout << "\n[10] interrupted conversion\n";
  {
    World w = makeWorld("new-crash");
    newLaunch(w);
    addUserData(w.docs, "PES2027-data");
    rename((w.docs + "/pak").c_str(), (w.docs + "/.pak.ziru-convert").c_str());  // stopped after step 1
    Launch a = newLaunch(w, true);
    CHECK(zfr::isLink(w.docs + "/pak") && !zfr::isLink(w.docs + "/.pak.ziru-convert") && !exists(w.docs + "/.pak.ziru-convert"),
          "folder restored, then converted; staging gone");
    CHECK(a.ready && engineBoots(w.docs), "boots");
    mkdirs(w.docs + "/.pak.ziru-convert");
    writeFile(w.docs + "/.pak.ziru-convert/surprise.pak", 2000, 's');
    newLaunch(w, true);
    bool kept = false;
    for (auto& n : zfr::listDir(w.docs + "/PES2027-repair-backups"))
      if (exists(w.docs + "/PES2027-repair-backups/" + n + "/surprise.pak")) kept = true;
    CHECK(kept && !exists(w.docs + "/.pak.ziru-convert"), "a real file left in staging is moved to PES2027-repair-backups, not deleted");
  }

  std::cout << "\n[11] Files app 'Keep Both' copy named \"pak 2\"\n";
  {
    World w = makeWorld("new-keepboth");
    newLaunch(w);
    addUserData(w.docs, "tmp", "pak");
    rename((w.docs + "/tmp/pak").c_str(), (w.docs + "/pak 2").c_str());
    rename((w.docs + "/tmp/cpk").c_str(), (w.docs + "/cpk").c_str());
    Launch a = newLaunch(w);
    CHECK(a.pak.action == zfr::Action::Converted && readlinkStr(w.docs + "/pak") == "pak 2", "mod-made pak replaced by a link to \"pak 2\"");
    CHECK(a.ready && engineBoots(w.docs), "boots");
  }

  std::cout << "\n[12] mod backup folders are never taken as game data\n";
  {
    World w = makeWorld("new-backups");
    newLaunch(w);
    addUserData(w.docs + "/ModBackups/batch1", ".");
    Launch a = newLaunch(w);
    CHECK(!zfr::isLink(w.docs + "/pak") && !a.ready, "ModBackups/*/pak ignored, still not ready");
  }

  std::cout << "\n[13] cpk present but pak has no base paks\n";
  {
    World w = makeWorld("new-nobase");
    newLaunch(w);
    writeFile(w.docs + "/PES2027-data/cpk/dt200_mobile_all.cpk", 6000, 'c');
    Launch a = newLaunch(w);
    World o = makeWorld("old-nobase");
    oldLaunch(o, false);
    writeFile(o.docs + "/PES2027-data/cpk/dt200_mobile_all.cpk", 6000, 'c');
    bool oldR = oldLaunch(o, false);
    CHECK(!a.ready, "proposed: not ready (importer stays up with a message) instead of a black screen");
    CHECK(oldR && !engineBoots(o.docs), "old: reported ready without base paks");
  }

  std::cout << "\n[14] Download: nested data Download folder found even though Documents/Download exists\n";
  {
    World w = makeWorld("new-dl");
    mkdirs(w.docs + "/Download");
    writeFile(w.docs + "/PES2027-data/Download/dt510_eng_mobile_all.cpk", 100, 'v');
    zfr::Spec dl{"download", {}, false};
    auto c = zfr::candidates(w.docs, dl, 3, w.docs + "/Download");
    CHECK(c.size() == 1 && c[0].path == w.docs + "/PES2027-data/Download", "candidate is PES2027-data/Download");
    std::string oldDl = oldFind(w.docs, "Download", 3, false);
    CHECK(oldDl == w.docs + "/Download", "old search returns Documents/Download itself in this order (nested folder never linked)");
  }

  std::cout << "\n[15] layout A: user files the existing link loops replace (pre-existing behaviour, same in old fresh install)\n";
  {
    for (bool fresh : {true, false}) {
      gDeletedByEnsure.clear();
      World w = makeWorld(fresh ? "loops-A" : "loops-C");
      if (!fresh) newLaunch(w);
      addUserData(w.docs, "PES2027-data");
      gDeletedByEnsure.clear();
      newLaunch(w);
      std::set<std::string> names;
      for (auto& p : gDeletedByEnsure) names.insert(p.substr(p.rfind('/') + 1));
      std::cout << "        " << (fresh ? "A" : "C") << ": ensureSymlink replaced " << gDeletedByEnsure.size() << " small real file(s):";
      for (auto& n : names) std::cout << " " << n;
      std::cout << "\n";
      CHECK(names == std::set<std::string>{"pc9242_mobile_and.pak"}, std::string(fresh ? "A" : "C") + ": only the <=1 KB livedata stub is replaced");
    }
    gHardened = true;
    for (bool fresh : {true, false}) {
      World w = makeWorld(fresh ? "loops-A-h" : "loops-C-h");
      if (!fresh) newLaunch(w);
      addUserData(w.docs, "PES2027-data");
      std::map<std::string, std::string> before, after;
      realFiles(w.docs + "/PES2027-data", before);
      gDeletedByEnsure.clear();
      Launch a = newLaunch(w);
      realFiles(w.docs + "/PES2027-data", after);
      CHECK(gDeletedByEnsure.empty() && userFilesKept(before, after) && a.ready && engineBoots(w.docs),
            std::string(fresh ? "A" : "C") + " with the optional ensureSymlink hardening: no user file replaced, still boots");
    }
    gHardened = false;
  }

  std::cout << "\n[16] livedata and Download come from the same data folder as pak, not from a player's mod folder\n";
  {
    World w = makeWorld("new-near");
    addUserData(w.docs, "PES2027-data");
    writeFile(w.docs + "/AMod/livedata/dt870_mobile_and.cpk", 2000, 'm');  // sorts before PES2027-data by name
    writeFile(w.docs + "/AMod/Download/pc3045_mobile_ios.pak", 2000, 'm');
    writeFile(w.docs + "/PES2027-data/Download/dt510_eng_mobile_all.cpk", 2000, 'v');
    auto plain = zfr::candidates(w.docs, livedataSpec(), 3, w.docs + "/livedata");
    CHECK(plain.size() == 2 && plain[0].path == w.docs + "/AMod/livedata", "without the hint the mod folder would win (what the hint fixes)");
    Launch a = newLaunch(w);
    CHECK(readlinkStr(w.docs + "/livedata") == "PES2027-data/livedata", "Documents/livedata -> PES2027-data/livedata");
    CHECK(readlinkStr(w.docs + "/cpk") == "PES2027-data/cpk" && a.ready, "cpk from the same folder, ready");
    zfr::Spec dl{"download", {}, false};
    auto c = zfr::candidates(w.docs, dl, 3, w.docs + "/Download", zfr::dataParent(a.pak));
    CHECK(!c.empty() && c[0].path == w.docs + "/PES2027-data/Download", "Download candidate is PES2027-data/Download");
  }

  std::cout << "\n[16b] the same when pak is filled file by file (a mod file kept in Documents/pak)\n";
  {
    World w = makeWorld("new-near-merge");
    newLaunch(w);  // the mod makes Documents/pak and Documents/livedata (links only)
    writeFile(w.docs + "/pak/pc9000_mobile_ios.pak", 2000, 'm');  // a user mod installed before the data
    addUserData(w.docs, "PES2027-data");
    writeFile(w.docs + "/AMod/livedata/dt870_mobile_and.cpk", 2000, 'm');
    Launch a = newLaunch(w);
    CHECK(a.pak.action == zfr::Action::Merged && a.pak.dataFolder == w.docs + "/PES2027-data/pak", "pak filled from PES2027-data/pak");
    CHECK(readlinkStr(w.docs + "/livedata") == "PES2027-data/livedata" && readlinkStr(w.docs + "/cpk") == "PES2027-data/cpk",
          "livedata and cpk from the same data folder, not AMod/livedata");
    Launch b = newLaunch(w);
    CHECK(b.pak.action == zfr::Action::KeptReal && b.pak.dataFolder == w.docs + "/PES2027-data/pak",
          "next launch (nothing new to link) still knows where pak is filled from");
  }

  std::cout << "\n[17] merge mode with a stadium pak in the data's pak folder stays stable\n";
  {
    World w = makeWorld("new-merge-stadium");
    newLaunch(w);
    writeFile(w.docs + "/pak/pc9000_mobile_ios.pak", 2000, 'm');  // real file: forces merge mode
    addUserData(w.docs, "PES2027-data");
    writeFile(w.docs + "/PES2027-data/pak/pc3045_mobile_ios.pak", 3000, 's');
    Launch a = newLaunch(w);
    CHECK(a.pak.action == zfr::Action::Merged && a.ready, "merged and ready");
    CHECK(!zfr::isLink(w.docs + "/pak/pc3045_mobile_ios.pak") && !exists(w.docs + "/pak/pc3045_mobile_ios.pak"),
          "stadium pak not linked into Documents/pak");
    std::map<std::string, std::string> t1, t2;
    tree(w.root, t1);
    Launch b = newLaunch(w);
    tree(w.root, t2);
    CHECK(!b.pak.changed() && t1 == t2, "next launch: no change (no relink/unlink cycle)");
  }

  std::cout << "\n[18] loose files at the Documents root go only into a missing or mod-made folder\n";
  {
    World w = makeWorld("new-loose");
    zfr::Result none;
    CHECK(zfr::mayLinkLooseFiles(w.docs + "/pak", none), "Documents/pak missing: allowed");
    newLaunch(w);  // the mod makes Documents/pak with links only
    CHECK(zfr::mayLinkLooseFiles(w.docs + "/pak", none), "mod-made Documents/pak (links only): allowed");
    writeFile(w.docs + "/pak/my_mobile_ios.pak", 10, 'm');
    CHECK(!zfr::mayLinkLooseFiles(w.docs + "/pak", none), "Documents/pak holding a real file of the player: refused");
    writeFile(w.docs + "/PES2027-data/cpk/dt999_mobile_all.cpk", 10, 'c');  // a data folder without the markers
    symlink("PES2027-data/cpk", (w.docs + "/cpk").c_str());
    CHECK(!zfr::mayLinkLooseFiles(w.docs + "/cpk", none), "Documents/cpk linked into the player's data folder: refused");
    zfr::Result base;
    base.hasBase = true;
    CHECK(!zfr::mayLinkLooseFiles(w.docs + "/nothing", base), "base data already visible: nothing to do");
  }

  std::cout << "\n[19] a second leftover in the same second does not overwrite the first backup\n";
  {
    World w = makeWorld("new-backup-names");
    newLaunch(w);
    addUserData(w.docs, "PES2027-data");
    newLaunch(w);
    for (int i = 0; i < 2; i++) {
      writeFile(w.docs + "/.pak.ziru-convert/mine" + std::to_string(i) + ".pak", 2000, 's');
      zfr::resolve(w.docs, pakSpec());
    }
    int kept = 0;
    for (auto& n : zfr::listDir(w.docs + "/PES2027-repair-backups"))
      for (int i = 0; i < 2; i++) kept += exists(w.docs + "/PES2027-repair-backups/" + n + "/mine" + std::to_string(i) + ".pak");
    CHECK(kept == 2 && !exists(w.docs + "/.pak.ziru-convert"), "both leftovers kept in separate backup folders");
  }

  std::cout << "\nlinkText\n";
  CHECK(zfr::linkText("/d", "/d", "/d/PES2027-data/pak") == "PES2027-data/pak", "top-level link relative");
  CHECK(zfr::linkText("/d", "/d/pak", "/d/PES2027-data/pak/f") == "../PES2027-data/pak/f", "per-file link relative");
  CHECK(zfr::linkText("/d", "/d", "/lib/GameData/x/pak") == "/lib/GameData/x/pak", "outside Documents stays absolute");
  CHECK(zfr::nameMatches("pak 2", "pak") && zfr::nameMatches("PAK", "pak") && !zfr::nameMatches("pak.old", "pak") && !zfr::nameMatches("pak 2b", "pak"),
        "folder name matching");

  std::cout << "\n" << gOks << " passed, " << gFails << " failed\n";
  return gFails ? 1 : 0;
}
