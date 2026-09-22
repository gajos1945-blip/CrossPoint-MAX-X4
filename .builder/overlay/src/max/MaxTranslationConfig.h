#pragma once
#include <string>

struct MaxTranslationConfig {
  std::string gateway;
  std::string source = "auto";
  std::string target = "pl";
  bool studyMode = true;
  bool cache = true;

  static constexpr const char* CONFIG_DIR = "/.crosspoint-max";
  static constexpr const char* CONFIG_PATH = "/.crosspoint-max/config.json";

  static MaxTranslationConfig load();
  static bool save(const MaxTranslationConfig& config);
  bool valid(std::string& error) const;
};
