// Host test for the data-pack ZIP import (core/datapack.cpp): linking Documents/pak and cpk to the
// installed pack never deletes a folder moved aside earlier.
//   python3 test/datapack_fixtures.py DIR && ./build/datapacktest DIR
#include "../core/datapack.hpp"

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <iterator>

static int gFails = 0, gOks = 0;
#define CHECK(cond, msg)                                                                  \
  do {                                                                                    \
    if (cond) { gOks++; std::cout << "  ok    " << msg << "\n"; }                         \
    else { gFails++; std::cout << "  FAIL  " << msg << " (line " << __LINE__ << ")\n"; } \
  } while (0)

static void writeFile(const std::string& p, const std::string& s) {
  zd::mkdirs(p.substr(0, p.rfind('/')));
  std::ofstream(p, std::ios::binary) << s;
}
static std::string readAll(const std::string& p) {
  std::ifstream i(p, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(i)), {});
}
static bool exists(const std::string& p) { struct stat st; return lstat(p.c_str(), &st) == 0; }
static bool isLink(const std::string& p) { struct stat st; return lstat(p.c_str(), &st) == 0 && S_ISLNK(st.st_mode); }
static std::string readlinkStr(const std::string& p) {
  char b[PATH_MAX];
  ssize_t r = readlink(p.c_str(), b, sizeof b - 1);
  return r > 0 ? std::string(b, (size_t)r) : "";
}

// A fresh app container: Documents plus Library/Application Support/PES2027/GameData.
static zd::Layout container(const std::string& base, const std::string& name) {
  std::string root = base + "/" + name;
  zd::removeTree(root);
  zd::Layout L{root + "/Library/Application Support/PES2027/GameData", root + "/Documents"};
  zd::mkdirs(L.documents);
  return L;
}
// What a launch before any data exists leaves behind: a real Documents/pak holding only links into the app.
static void modMadePak(const zd::Layout& L, const std::string& bundleFile) {
  writeFile(bundleFile, "bundled extra pak");
  zd::mkdirs(L.documents + "/pak");
  symlink(bundleFile.c_str(), (L.documents + "/pak/pc7999_mobile_ios.pak").c_str());
}
static bool install(const std::string& zip, const zd::Layout& L, zd::Manifest& m, std::string& err) {
  return zd::installPack(zip, L, 0, nullptr, m, err);
}

int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "usage: datapacktest FIXTURE_DIR (from test/datapack_fixtures.py)\n"; return 2; }
  const std::string dir = argv[1], work = dir + "/work";
  zd::removeTree(work);
  zd::mkdirs(work);

  std::cout << "[1] linking never deletes a folder moved aside earlier\n";
  {
    zd::Layout L = container(work, "aside");
    zd::Manifest m;
    std::string err;
    writeFile(L.documents + "/pak/my_own_mobile_ios.pak", "mine");
    writeFile(L.documents + "/pak.old/older_own_mobile_ios.pak", "older");
    CHECK(install(dir + "/root.zip", L, m, err), "first import " + err);
    CHECK(readAll(L.documents + "/pak.old/older_own_mobile_ios.pak") == "older", "existing pak.old kept as it was");
    CHECK(readAll(L.documents + "/pak.old2/my_own_mobile_ios.pak") == "mine", "player's Documents/pak moved to pak.old2");
    CHECK(isLink(L.documents + "/pak"), "Documents/pak is now the link");

    // The player copies a folder straight into Documents again, then imports a second time.
    unlink((L.documents + "/pak").c_str());
    writeFile(L.documents + "/pak/again_mobile_ios.pak", "again");
    writeFile(L.documents + "/pak/.DS_Store", "finder");
    CHECK(install(dir + "/root.zip", L, m, err), "second import " + err);
    CHECK(readAll(L.documents + "/pak.old/older_own_mobile_ios.pak") == "older" &&
              readAll(L.documents + "/pak.old2/my_own_mobile_ios.pak") == "mine",
          "both earlier folders still there");
    CHECK(readAll(L.documents + "/pak.old3/again_mobile_ios.pak") == "again" && exists(L.documents + "/pak.old3/.DS_Store"),
          "the new one goes to pak.old3, hidden files included");
    CHECK(readlinkStr(L.documents + "/pak") == zd::dataDir(L, "ci-root") + "/pak", "Documents/pak links to the pack again");

    // The quick check at launch repairs a replaced link the same way.
    unlink((L.documents + "/cpk").c_str());
    writeFile(L.documents + "/cpk/mine_mobile_all.cpk", "my cpk");
    std::string verr;
    CHECK(zd::quickVerify(L, "ci-root", "11.1.1", "11.1.101", "1.0.0", verr), "quick check relinks " + verr);
    CHECK(readAll(L.documents + "/cpk.old/mine_mobile_all.cpk") == "my cpk" && isLink(L.documents + "/cpk"),
          "a real Documents/cpk is moved to cpk.old, not deleted");
  }

  std::cout << "\n[2] a folder of links only is removed, whatever it is called\n";
  {
    zd::Layout L = container(work, "links");
    modMadePak(L, work + "/links/Bundle/pc7999_mobile_ios.pak");
    zd::mkdirs(L.documents + "/cpk");  // an empty folder counts as links only
    writeFile(L.documents + "/pak.old/older_own_mobile_ios.pak", "older");
    zd::Manifest m;
    std::string err;
    CHECK(install(dir + "/root.zip", L, m, err), "import " + err);
    CHECK(!exists(L.documents + "/pak.old2") && !exists(L.documents + "/cpk.old"), "no new .old folders for links-only or empty folders");
    CHECK(readAll(L.documents + "/pak.old/older_own_mobile_ios.pak") == "older", "existing pak.old untouched");
  }

  std::cout << "\n" << gOks << " passed, " << gFails << " failed\n";
  return gFails ? 1 : 0;
}
