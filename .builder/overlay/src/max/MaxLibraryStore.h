#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class MaxReadingStatus : uint8_t {
  Unread = 0,
  InProgress = 1,
  Read = 2,
};

struct MaxLibraryBook {
  std::string path;
  std::string title;
  std::string author;
  std::string language;
  std::string series;
  uint16_t volume = 0;
  std::string collection;
  bool favorite = false;
  MaxReadingStatus status = MaxReadingStatus::Unread;
};

namespace MaxLibraryStore {
constexpr size_t MAX_BOOKS = 600;
constexpr const char* ROOT = "/.crosspoint-max/library";
constexpr const char* INDEX_PATH = "/.crosspoint-max/library/index.jsonl";

const char* statusName(MaxReadingStatus status);

bool load(std::vector<MaxLibraryBook>& books);
bool save(const std::vector<MaxLibraryBook>& books);

// Re-scan supported books from /Books when it exists, otherwise from /.
// Cached EPUB metadata is reused; missing metadata falls back to filename.
// Existing favorite/status/series/collection values are preserved by path.
bool rebuild(std::vector<MaxLibraryBook>& books);

// Called by the reader. These update the persistent index when it exists.
// markOpened never downgrades Read -> InProgress.
bool markOpened(const std::string& path, const std::string& title,
                const std::string& author, const std::string& language);
bool markRead(const std::string& path);
bool toggleFavorite(const std::string& path, bool* newValue = nullptr);
bool cycleStatus(const std::string& path, MaxReadingStatus* newStatus = nullptr);
}
