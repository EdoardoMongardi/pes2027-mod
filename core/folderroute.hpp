// Finds the folder that really holds the game data for Documents/<name> (pak, cpk, livedata)
// and points Documents/<name> at it.
//
// Why: the first launch before any data exists creates real folders Documents/pak (bundled
// extra paks) and Documents/livedata (bundled livedata). Those folders contain only links the
// mod made. When the player later copies PES2027-data/{pak,cpk,livedata} into Documents, the
// old search returned the mod-made Documents/pak itself, so the engine never saw the base paks.
//
// Rules:
//  * A folder with the base files (markers) wins over one without them.
//  * A mod-made folder (only symbolic links inside) is turned into a link to the real folder.
//    Removing a symbolic link never removes the file it points to.
//  * A folder with real files the player (or a mod install) put there is never deleted or
//    moved; the real folder's files are linked into it one by one instead.
//  * Links inside Documents are relative, so they survive the app container moving.
//  * Running it again changes nothing.
// Plain POSIX so it runs the same in the app and in the host test (test/folderroutetest.cpp).
#pragma once
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <functional>
#include <string>
#include <vector>

namespace zfr {

using LogFn = std::function<void(const std::string&)>;

struct Spec {
  std::string name;                  // folder name under Documents: "pak", "cpk", "livedata"
  std::vector<std::string> markers;  // lower-case file names that only the real game data has
  bool anyRealDataFile = false;      // livedata: any real .cpk/.pak/.ucas/.utoc file counts as a marker
  // Files never linked one by one into a folder that has to keep its real files (for pak: the
  // stadium paks, which the app keeps out of Documents/pak and removes from there on every launch).
  std::function<bool(const std::string&)> notLinkedSingly;
};

enum class Action { None, Linked, Relinked, Converted, Merged, KeptReal, KeptLink, NotFound };

inline const char* actionName(Action a) {
  switch (a) {
    case Action::None: return "none";
    case Action::Linked: return "linked";
    case Action::Relinked: return "relinked";
    case Action::Converted: return "converted";
    case Action::Merged: return "merged";
    case Action::KeptReal: return "kept-real-folder";
    case Action::KeptLink: return "kept-link";
    case Action::NotFound: return "not-found";
  }
  return "?";
}

struct Result {
  std::string folder;           // the folder Documents/<name> now shows (empty = nothing usable)
  bool hasBase = false;         // Documents/<name> now shows the markers
  bool otherDataFolder = false; // a folder other than Documents/<name> holds the markers
  std::string dataFolder;       // where the data comes from: Documents/<name> (or what it links to), or the folder merged into it
  Action action = Action::None;
  bool changed() const {
    return action == Action::Linked || action == Action::Relinked || action == Action::Converted || action == Action::Merged;
  }
};

inline std::string lower(std::string s) {
  for (auto& c : s) c = (char)((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
  return s;
}

inline std::vector<std::string> listDir(const std::string& dir) {
  std::vector<std::string> out;
  DIR* d = opendir(dir.c_str());
  if (!d) return out;
  while (dirent* e = readdir(d)) {
    std::string n = e->d_name;
    if (n == "." || n == "..") continue;
    out.push_back(n);
  }
  closedir(d);
  std::sort(out.begin(), out.end());  // readdir order differs between file systems
  return out;
}

inline bool lstatOk(const std::string& p, struct stat& st) { return lstat(p.c_str(), &st) == 0; }
inline bool statOk(const std::string& p, struct stat& st) { return stat(p.c_str(), &st) == 0; }
inline bool isLink(const std::string& p) { struct stat st; return lstatOk(p, st) && S_ISLNK(st.st_mode); }
inline bool isDirFollow(const std::string& p) { struct stat st; return statOk(p, st) && S_ISDIR(st.st_mode); }
inline bool sameFile(const std::string& a, const std::string& b) {
  struct stat x, y;
  return statOk(a, x) && statOk(b, y) && x.st_dev == y.st_dev && x.st_ino == y.st_ino;
}

inline bool isDataExt(const std::string& lowerName) {
  auto dot = lowerName.rfind('.');
  if (dot == std::string::npos) return false;
  std::string e = lowerName.substr(dot + 1);
  return e == "cpk" || e == "pak" || e == "ucas" || e == "utoc";
}

// realOnly: the marker must be a real file in this folder (not a link to one).
inline bool hasMarker(const std::string& dir, const Spec& spec, bool realOnly) {
  for (const auto& n : listDir(dir)) {
    std::string l = lower(n);
    bool want = spec.anyRealDataFile ? isDataExt(l)
                                     : std::find(spec.markers.begin(), spec.markers.end(), l) != spec.markers.end();
    if (!want) continue;
    struct stat st;
    std::string p = dir + "/" + n;
    if (spec.anyRealDataFile || realOnly) {
      if (lstatOk(p, st) && S_ISREG(st.st_mode) && st.st_size > 0) return true;
    } else if (statOk(p, st) && S_ISREG(st.st_mode) && st.st_size > 0) {
      return true;
    }
  }
  return false;
}

// "pak", "PAK", and the Files app's "Keep Both" copy "pak 2".
inline bool nameMatches(const std::string& entry, const std::string& name) {
  std::string e = lower(entry), n = lower(name);
  if (e == n) return true;
  if (e.size() > n.size() + 1 && e.compare(0, n.size(), n) == 0 && e[n.size()] == ' ') {
    return e.find_first_not_of("0123456789", n.size() + 1) == std::string::npos;
  }
  return false;
}

// Folders the mod itself owns; never searched for game data.
inline bool skippedTopLevel(const std::string& n) {
  return n == "PES2027-logs" || n == "PES2027-cache" || n == "PES2027-repair-backups" || n == "ModBackups";
}

struct Candidate {
  std::string path;
  int depth = 0;
  bool marked = false;
  bool near = false;  // sits next to the folder the other data came from (same parent folder)
};

// Real folders named <name> up to maxDepth levels below docs (same reach as the old search),
// best first: folders with the markers, then folders in the same parent folder as `near` (the
// data folder the pak folder came from, so a player's mod folder such as Documents/AMod/livedata
// does not win over PES2027-data/livedata), then shallower, then by path. Links are not followed,
// hidden entries are skipped, and `exclude` (usually Documents/<name>) is never returned.
inline std::vector<Candidate> candidates(const std::string& docs, const Spec& spec, int maxDepth, const std::string& exclude,
                                         const std::string& near = "") {
  std::vector<Candidate> out;
  std::vector<std::pair<std::string, int>> queue{{docs, 0}};
  for (size_t qi = 0; qi < queue.size(); qi++) {
    auto [dir, depth] = queue[qi];
    for (const auto& n : listDir(dir)) {
      if (n.empty() || n[0] == '.') continue;
      if (depth == 0 && skippedTopLevel(n)) continue;
      std::string full = dir + "/" + n;
      struct stat st;
      if (!lstatOk(full, st) || !S_ISDIR(st.st_mode)) continue;
      if (nameMatches(n, spec.name)) {
        if (full == exclude || sameFile(full, exclude)) continue;
        out.push_back({full, depth, hasMarker(full, spec, false), !near.empty() && sameFile(dir, near)});
        continue;
      }
      if (depth < maxDepth) queue.push_back({full, depth + 1});
    }
  }
  std::stable_sort(out.begin(), out.end(), [](const Candidate& a, const Candidate& b) {
    if (a.marked != b.marked) return a.marked;
    if (a.near != b.near) return a.near;
    if (a.depth != b.depth) return a.depth < b.depth;
    return a.path < b.path;
  });
  return out;
}

// The folder holding the data folder in use ("Documents/PES2027-data" when Documents/pak links to,
// or is filled from, PES2027-data/pak), or "" when the base data was not found. Passed as `near` so
// cpk, livedata and Download are taken from the same data folder. "<folder>/.." is resolved by the
// file system, through the link when <folder> is one.
inline std::string dataParent(const Result& r) { return r.hasBase && !r.dataFolder.empty() ? r.dataFolder + "/.." : ""; }

inline std::vector<std::string> splitPath(const std::string& p) {
  std::vector<std::string> out;
  size_t s = 0;
  while (s <= p.size()) {
    size_t e = p.find('/', s);
    if (e == std::string::npos) e = p.size();
    std::string c = p.substr(s, e - s);
    if (!c.empty() && c != ".") out.push_back(c);
    s = e + 1;
  }
  return out;
}

// Link text for a link placed in linkDir that points at target: relative when both are inside
// docs (survives the app container moving), absolute otherwise.
inline std::string linkText(const std::string& docs, const std::string& linkDir, const std::string& target) {
  std::string root = docs + "/";
  if ((linkDir + "/").compare(0, root.size(), root) != 0 || target.compare(0, root.size(), root) != 0) return target;
  auto a = splitPath(linkDir), b = splitPath(target);
  size_t common = 0;
  while (common < a.size() && common < b.size() && a[common] == b[common]) common++;
  std::string out;
  for (size_t i = common; i < a.size(); i++) out += "../";
  for (size_t i = common; i < b.size(); i++) out += b[i] + (i + 1 < b.size() ? "/" : "");
  return out.empty() ? "." : out;
}

// Symbolic links whose target is gone. Users cannot create links, so these are always the
// mod's own leftovers (for example links into the previous app bundle after a reinstall).
inline int pruneDanglingLinks(const std::string& dir, const LogFn& log = nullptr) {
  int n = 0;
  for (const auto& f : listDir(dir)) {
    std::string p = dir + "/" + f;
    struct stat st;
    if (isLink(p) && stat(p.c_str(), &st) != 0 && errno == ENOENT && unlink(p.c_str()) == 0) n++;
  }
  if (n && log) log("removed " + std::to_string(n) + " broken links in " + dir);
  return n;
}

inline bool onlyLinks(const std::string& dir) {
  for (const auto& n : listDir(dir)) {
    if (!isLink(dir + "/" + n)) return false;
  }
  return true;
}

// Links each file of `from` into `into` where nothing of that name exists yet. Never replaces a
// real file or a working link; repairs only links whose target is gone.
inline int mergeLinks(const std::string& docs, const std::string& into, const std::string& from,
                      const std::function<bool(const std::string&)>& skip = nullptr) {
  int made = 0;
  for (const auto& f : listDir(from)) {
    if (f.empty() || f[0] == '.' || (skip && skip(f))) continue;
    std::string src = from + "/" + f, dst = into + "/" + f;
    struct stat st;
    if (!statOk(src, st) || !S_ISREG(st.st_mode)) continue;
    struct stat lst;
    if (lstatOk(dst, lst)) {
      if (!S_ISLNK(lst.st_mode) || statOk(dst, st)) continue;  // real file or working link: keep
      unlink(dst.c_str());
    }
    if (symlink(linkText(docs, into, src).c_str(), dst.c_str()) == 0) made++;
  }
  return made;
}

// Removes the links of a moved-aside mod folder. Anything else found there is kept in a
// visible backup folder instead of being deleted.
inline void retireStaging(const std::string& docs, const std::string& staging, const std::string& name, const LogFn& log) {
  for (const auto& n : listDir(staging)) {
    std::string p = staging + "/" + n;
    if (isLink(p)) unlink(p.c_str());
  }
  if (rmdir(staging.c_str()) == 0) return;
  std::string backups = docs + "/PES2027-repair-backups";
  mkdir(backups.c_str(), 0755);
  const std::string base = backups + "/" + name + "-kept-" + std::to_string((long long)time(nullptr));
  std::string keep = base;
  struct stat st;
  for (int i = 2; lstatOk(keep, st) && i < 100; i++) keep = base + "-" + std::to_string(i);  // never onto an earlier backup
  if (rename(staging.c_str(), keep.c_str()) == 0) {
    if (log) log("kept unexpected files from the old " + name + " folder in " + keep);
  } else if (log) {
    log("could not tidy " + staging + " (left in place)");
  }
}

inline bool convertToLink(const std::string& docs, const std::string& link, const std::string& staging,
                          const std::string& target, const std::string& name, const LogFn& log) {
  if (rename(link.c_str(), staging.c_str()) != 0) return false;
  if (symlink(linkText(docs, docs, target).c_str(), link.c_str()) != 0) {
    rename(staging.c_str(), link.c_str());
    return false;
  }
  retireStaging(docs, staging, name, log);
  return true;
}

inline Result finish(const std::string& path, const Spec& spec, Action action, bool other) {
  Result r;
  r.action = action;
  r.otherDataFolder = other;
  if (isDirFollow(path)) {
    r.folder = r.dataFolder = path;
    r.hasBase = hasMarker(path, spec, false);
  }
  return r;
}

// Loose data files dropped at the Documents root may be linked into Documents/<name> only when that
// folder is missing or is the one this mod made (links only). Never into a folder holding the player's
// own files, and never through a link into their data folder.
inline bool mayLinkLooseFiles(const std::string& folder, const Result& r) {
  if (r.hasBase) return false;
  struct stat st;
  if (!lstatOk(folder, st)) return true;
  return S_ISDIR(st.st_mode) && onlyLinks(folder);
}

// near: see candidates() (pass dataParent(<pak result>) for cpk and livedata, "" for pak itself).
inline Result resolve(const std::string& docs, const Spec& spec, const LogFn& log = nullptr, const std::string& near = "",
                      int maxDepth = 3) {
  const std::string link = docs + "/" + spec.name;
  const std::string staging = docs + "/." + spec.name + ".ziru-convert";
  auto say = [&](const std::string& s) { if (log) log(spec.name + ": " + s); };

  // A conversion interrupted between its two steps: put the folder back, or finish tidying.
  struct stat st;
  if (lstatOk(staging, st)) {
    struct stat lst;
    if (!lstatOk(link, lst) && rename(staging.c_str(), link.c_str()) == 0) say("restored after an interrupted change");
    else retireStaging(docs, staging, spec.name, log);
  }

  if (!lstatOk(link, st)) {
    auto c = candidates(docs, spec, maxDepth, link, near);
    if (c.empty()) return finish(link, spec, Action::NotFound, false);
    if (symlink(linkText(docs, docs, c[0].path).c_str(), link.c_str()) != 0) {
      say("could not link to " + c[0].path);
      return finish(link, spec, Action::NotFound, c[0].marked);
    }
    say("linked to " + c[0].path);
    return finish(link, spec, Action::Linked, c[0].marked);
  }

  if (S_ISLNK(st.st_mode)) {
    bool alive = isDirFollow(link);
    if (alive && hasMarker(link, spec, false)) return finish(link, spec, Action::KeptLink, false);
    auto c = candidates(docs, spec, maxDepth, link, near);
    // Move a working link only to a folder that has the markers; move a broken one to anything.
    if (!c.empty() && (c[0].marked || !alive)) {
      if (unlink(link.c_str()) == 0 && symlink(linkText(docs, docs, c[0].path).c_str(), link.c_str()) == 0) {
        say("moved link to " + c[0].path);
        return finish(link, spec, Action::Relinked, c[0].marked);
      }
      say("could not move link to " + c[0].path);
    }
    // A broken link with nothing better: leave it for the ZIP importer's own repair.
    return finish(link, spec, alive ? Action::KeptLink : Action::NotFound, false);
  }

  if (!S_ISDIR(st.st_mode)) {
    say("is a file, not a folder; left alone");
    return finish(link, spec, Action::NotFound, false);
  }

  // A real folder holding real base files: the player's own data (files copied straight into
  // Documents/pak). Nothing to do.
  if (hasMarker(link, spec, true)) return finish(link, spec, Action::KeptReal, false);

  auto c = candidates(docs, spec, maxDepth, link, near);
  if (c.empty() || !c[0].marked) return finish(link, spec, Action::KeptReal, false);

  if (onlyLinks(link)) {
    if (convertToLink(docs, link, staging, c[0].path, spec.name, log)) {
      say("replaced the folder the mod made earlier with a link to " + c[0].path);
      return finish(link, spec, Action::Converted, true);
    }
    say("could not replace the folder the mod made earlier; linking files one by one");
  }
  int n = mergeLinks(docs, link, c[0].path, spec.notLinkedSingly);
  if (n) say("kept the real files already in " + link + " and linked " + std::to_string(n) + " files from " + c[0].path);
  Result r = finish(link, spec, n ? Action::Merged : Action::KeptReal, true);
  r.dataFolder = c[0].path;  // filled from there (now or on an earlier launch)
  return r;
}

}  // namespace zfr
