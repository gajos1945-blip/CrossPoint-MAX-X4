#include "MaxTranslationCache.h"
#include <HalStorage.h>
#include <cstdio>
#include <cstdint>

namespace {
constexpr const char* ROOT = "/.crosspoint-max";
constexpr const char* TRANS = "/.crosspoint-max/translations";

uint64_t fnv1a64(const std::string& value) {
  uint64_t hash = 14695981039346656037ULL;
  for (const unsigned char c : value) {
    hash ^= static_cast<uint64_t>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}

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

namespace MaxTranslationCache {
std::string stableBookHash(const std::string& bookPath) {
  char buf[17]{};
  std::snprintf(buf, sizeof(buf), "%016llx",
                static_cast<unsigned long long>(fnv1a64(bookPath)));
  return std::string(buf);
}

std::string pagePath(const std::string& bookPath, const int spine, const int page,
                     const std::string& target) {
  if (!safeLanguage(target) || spine < 0 || page < 0) return {};
  return std::string(TRANS) + "/" + stableBookHash(bookPath) + "/p_" +
         std::to_string(spine) + "_" + std::to_string(page) + "_" + target + ".txt";
}

bool load(const std::string& bookPath, const int spine, const int page,
          const std::string& target, std::string& translated) {
  const std::string path = pagePath(bookPath, spine, page, target);
  if (path.empty() || !Storage.exists(path.c_str())) return false;
  const String data = Storage.readFile(path.c_str());
  if (data.length() == 0) return false;
  translated.assign(data.c_str(), data.length());
  return true;
}

bool store(const std::string& bookPath, const int spine, const int page,
           const std::string& target, const std::string& translated) {
  if (translated.empty()) return false;
  const std::string hash = stableBookHash(bookPath);
  const std::string dir = std::string(TRANS) + "/" + hash;
  const std::string path = pagePath(bookPath, spine, page, target);
  if (path.empty()) return false;
  if (!Storage.ensureDirectoryExists(ROOT)) return false;
  if (!Storage.ensureDirectoryExists(TRANS)) return false;
  if (!Storage.ensureDirectoryExists(dir.c_str())) return false;
  return Storage.writeFile(path.c_str(), String(translated.c_str()));
}
}
