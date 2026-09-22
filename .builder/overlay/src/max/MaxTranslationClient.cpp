#include "MaxTranslationClient.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

namespace {
std::string endpointFor(const std::string& gateway) {
  std::string base = gateway;
  while (!base.empty() && base.back() == '/') base.pop_back();
  return base + "/translate";
}
}

MaxTranslationResponse MaxTranslationClient::translate(const std::string& gateway,
                                                       const std::string& source,
                                                       const std::string& target,
                                                       const std::string& text) const {
  MaxTranslationResponse result;
  if (WiFi.status() != WL_CONNECTED) {
    result.error = "Wi-Fi nie jest polaczone";
    return result;
  }
  if (gateway.rfind("http://", 0) != 0) {
    result.error = "HTTPS na X4: NOT IMPLEMENTED w v1.1-dev";
    return result;
  }
  if (text.empty()) {
    result.error = "Brak tekstu na tej stronie";
    return result;
  }

  JsonDocument req;
  req["source"] = source;
  req["target"] = target;
  req["text"] = text;
  String payload;
  serializeJson(req, payload);

  HTTPClient http;
  http.setConnectTimeout(7000);
  http.setTimeout(20000);
  const std::string url = endpointFor(gateway);
  if (!http.begin(url.c_str())) {
    result.error = "Nie mozna otworzyc Translation Gateway";
    return result;
  }
  http.addHeader("Content-Type", "application/json");
  const int status = http.POST(payload);
  if (status != 200) {
    result.error = "Gateway HTTP " + std::to_string(status);
    http.end();
    return result;
  }

  const String body = http.getString();
  http.end();

  JsonDocument resp;
  const DeserializationError err = deserializeJson(resp, body);
  if (err) {
    result.error = "Nieprawidlowa odpowiedz JSON";
    return result;
  }
  const char* translation = resp["translation"] | nullptr;
  if (!translation || *translation == '\0') {
    result.error = "Brak pola translation";
    return result;
  }
  result.ok = true;
  result.translation = translation;
  return result;
}
