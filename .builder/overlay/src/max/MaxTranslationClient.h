#pragma once
#include <string>

struct MaxTranslationResponse {
  bool ok = false;
  std::string translation;
  std::string error;
};

class MaxTranslationClient {
 public:
  MaxTranslationResponse translate(const std::string& gateway,
                                   const std::string& source,
                                   const std::string& target,
                                   const std::string& text) const;
};
