#pragma once
#include <string>

struct MaxChapterCheckpoint {
  int spine = -1;
  int nextPage = 0;
  int totalPages = 0;
  std::string source;
  std::string target;
  std::string layoutKey;
  std::string status;
};

namespace MaxChapterState {
std::string statePath(const std::string& bookPath, int spine, const std::string& target);
bool load(const std::string& bookPath, int spine, const std::string& target, MaxChapterCheckpoint& state);
bool save(const std::string& bookPath, const MaxChapterCheckpoint& state);
}
