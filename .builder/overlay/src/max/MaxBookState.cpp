#include "MaxBookState.h"

#include <ArduinoJson.h>
#include <HalStorage.h>

#include "MaxTranslationCache.h"

namespace {
constexpr const char* ROOT = "/.crosspoint-max";
constexpr const char* TRANS = "/.crosspoint-max/translations";

bool safeLanguage(const std::string& lang) {
  if (lang.empty() || lang.size() > 16) return false;
  for (const char c : lang) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_';
    if (!ok) return false;
  }
  return true;
}
}

namespace MaxBookState {
std::string statePath(const std::string& bookPath, const std::string& target) {
  if (!safeLanguage(target)) return {};
  return std::string(TRANS) + "/" + MaxTranslationCache::stableBookHash(bookPath) +
         "/book_" + target + "_state.json";
}

bool load(const std::string& bookPath, const std::string& target,
          MaxBookCheckpoint& state) {
  const std::string path = statePath(bookPath, target);
  if (path.empty() || !Storage.exists(path.c_str())) return false;

  const String raw = Storage.readFile(path.c_str());
  if (raw.length() == 0) return false;

  JsonDocument doc;
  if (deserializeJson(doc, raw)) return false;

  state.nextSpine = doc["next_spine"] | 0;
  state.nextPage = doc["next_page"] | 0;
  state.totalSpines = doc["total_spines"] | 0;
  state.currentSpinePages = doc["current_spine_pages"] | 0;
  if (const char* s = doc["source"] | nullptr) state.source = s;
  if (const char* t = doc["target"] | nullptr) state.target = t;
  if (const char* k = doc["layout_key"] | nullptr) state.layoutKey = k;
  if (const char* st = doc["status"] | nullptr) state.status = st;
  return state.target == target;
}

bool save(const std::string& bookPath, const MaxBookCheckpoint& state) {
  const std::string hash = MaxTranslationCache::stableBookHash(bookPath);
  const std::string dir = std::string(TRANS) + "/" + hash;
  const std::string path = statePath(bookPath, state.target);
  if (path.empty()) return false;

  if (!Storage.ensureDirectoryExists(ROOT)) return false;
  if (!Storage.ensureDirectoryExists(TRANS)) return false;
  if (!Storage.ensureDirectoryExists(dir.c_str())) return false;

  JsonDocument doc;
  doc["next_spine"] = state.nextSpine;
  doc["next_page"] = state.nextPage;
  doc["total_spines"] = state.totalSpines;
  doc["current_spine_pages"] = state.currentSpinePages;
  doc["source"] = state.source;
  doc["target"] = state.target;
  doc["layout_key"] = state.layoutKey;
  doc["status"] = state.status;

  String out;
  serializeJsonPretty(doc, out);
  return Storage.writeFile(path.c_str(), out);
}
}
