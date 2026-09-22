#include "MaxTranslationConfig.h"
#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

namespace { constexpr const char* MOD = "MAXCfg"; }

MaxTranslationConfig MaxTranslationConfig::load() {
  MaxTranslationConfig cfg;
  Storage.ensureDirectoryExists(CONFIG_DIR);
  if (!Storage.exists(CONFIG_PATH)) {
    save(cfg);
    return cfg;
  }
  const String raw = Storage.readFile(CONFIG_PATH);
  if (raw.length() == 0) return cfg;

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, raw);
  if (err) {
    LOG_ERR(MOD, "Invalid MAX config JSON: %s", err.c_str());
    return cfg;
  }
  if (const char* value = doc["gateway"] | nullptr) cfg.gateway = value;
  if (const char* value = doc["source"] | nullptr) cfg.source = value;
  if (const char* value = doc["target"] | nullptr) cfg.target = value;
  cfg.studyMode = doc["study_mode"] | true;
  cfg.cache = doc["cache"] | true;
  return cfg;
}

bool MaxTranslationConfig::save(const MaxTranslationConfig& cfg) {
  if (!Storage.ensureDirectoryExists(CONFIG_DIR)) return false;
  JsonDocument doc;
  doc["gateway"] = cfg.gateway;
  doc["source"] = cfg.source;
  doc["target"] = cfg.target;
  doc["study_mode"] = cfg.studyMode;
  doc["cache"] = cfg.cache;
  String out;
  serializeJsonPretty(doc, out);
  return Storage.writeFile(CONFIG_PATH, out);
}

bool MaxTranslationConfig::valid(std::string& error) const {
  if (gateway.empty()) {
    error = "Brak gateway. Ustaw /.crosspoint-max/config.json";
    return false;
  }
  if (gateway.rfind("http://", 0) != 0) {
    error = "v1.1-dev: gateway musi zaczynac sie od http://";
    return false;
  }
  if (target.empty() || target.size() > 16) {
    error = "Nieprawidlowy jezyk docelowy";
    return false;
  }
  if (source.empty() || source.size() > 16) {
    error = "Nieprawidlowy jezyk zrodlowy";
    return false;
  }
  return true;
}
