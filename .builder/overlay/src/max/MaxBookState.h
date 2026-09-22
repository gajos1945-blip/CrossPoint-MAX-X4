#pragma once
#include <string>

struct MaxBookCheckpoint {
  int nextSpine = 0;
  int nextPage = 0;
  int totalSpines = 0;
  int currentSpinePages = 0;
  std::string source;
  std::string target;
  std::string layoutKey;
  std::string status;  // running / cancelled / error / complete
};

namespace MaxBookState {
std::string statePath(const std::string& bookPath, const std::string& target);
bool load(const std::string& bookPath, const std::string& target, MaxBookCheckpoint& state);
bool save(const std::string& bookPath, const MaxBookCheckpoint& state);
}
