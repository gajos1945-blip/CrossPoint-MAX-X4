#pragma once
#include <string>

namespace MaxTranslationCache {
std::string stableBookHash(const std::string& bookPath);
std::string sourceFingerprint(const std::string& sourceText);
std::string pagePath(const std::string& bookPath, int spine, int page,
                     const std::string& target, const std::string& sourceText);
bool load(const std::string& bookPath, int spine, int page, const std::string& target,
          const std::string& sourceText, std::string& translated);
bool store(const std::string& bookPath, int spine, int page, const std::string& target,
           const std::string& sourceText, const std::string& translated);
}
