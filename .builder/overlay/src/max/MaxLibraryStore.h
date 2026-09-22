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

  // Preserve deliberate user overrides across index rebuilds, including
  // deliberately-cleared values.
  bool seriesManual = false;
  bool volumeManual = false;
  bool collectionManual = false;
};

namespace MaxLibraryStore {
constexpr size_t MAX_BOOKS = 600;
constexpr const char* ROOT = "/.crosspoint-max/library";
constexpr const char* INDEX_PATH = "/.crosspoint-max/library/index.jsonl";

const char* statusName(MaxReadingStatus status);

bool load(std::vector<MaxLibraryBook>& books);
bool save(const std::vector<MaxLibraryBook>& books);
bool rebuild(std::vector<MaxLibraryBook>& books);
bool getBook(const std::string& path, MaxLibraryBook& book);

bool markOpened(const std::string& path, const std::string& title,
                const std::string& author, const std::string& language);
bool markRead(const std::string& path);
bool toggleFavorite(const std::string& path, bool* newValue = nullptr);
bool cycleStatus(const std::string& path, MaxReadingStatus* newStatus = nullptr);

bool setSeries(const std::string& path, const std::string& value);
bool setCollection(const std::string& path, const std::string& value);
bool setVolume(const std::string& path, uint16_t value);
}
