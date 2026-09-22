#pragma once
#include <string>
namespace MaxTranslationCache {
std::string stableBookHash(const std::string& bookPath);
std::string pagePath(const std::string& bookPath, int spine, int page, const std::string& target);
bool load(const std::string& bookPath, int spine, int page, const std::string& target, std::string& translated);
bool store(const std::string& bookPath, int spine, int page, const std::string& target, const std::string& translated);
}
