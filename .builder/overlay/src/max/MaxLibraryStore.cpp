#include "MaxLibraryStore.h"

#include <ArduinoJson.h>
#include <Epub.h>
#include <FsHelpers.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Xtc.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <utility>

#include "RecentBooksStore.h"

namespace {
constexpr const char* MOD = "MAXLIB";
constexpr size_t NAME_BUFFER_SIZE = 384;
constexpr size_t MAX_LINE_BYTES = 2048;

bool supportedBook(const std::string& path) {
  return FsHelpers::hasEpubExtension(path) || FsHelpers::hasXtcExtension(path) ||
         FsHelpers::hasTxtExtension(path) || FsHelpers::hasMarkdownExtension(path);
}

std::string filenameOf(const std::string& path) {
  const size_t slash = path.find_last_of('/');
  return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string stemOf(const std::string& path) {
  std::string name = filenameOf(path);
  const size_t dot = name.find_last_of('.');
  if (dot != std::string::npos) name.resize(dot);
  return name;
}

bool recentPath(const std::string& path) {
  for (const auto& recent : RECENT_BOOKS.getBooks()) {
    if (recent.path == path) return true;
  }
  return false;
}

void deriveFolderMetadata(const std::string& path, std::string& collection,
                          std::string& series) {
  std::string rel = path;
  constexpr const char* booksPrefix = "/Books/";
  if (rel.rfind(booksPrefix, 0) == 0) {
    rel.erase(0, std::strlen(booksPrefix));
  } else if (!rel.empty() && rel.front() == '/') {
    rel.erase(0, 1);
  }

  std::vector<std::string> parts;
  size_t pos = 0;
  while (pos < rel.size()) {
    const size_t slash = rel.find('/', pos);
    if (slash == std::string::npos) break;
    if (slash > pos) parts.push_back(rel.substr(pos, slash - pos));
    pos = slash + 1;
  }

  if (!parts.empty()) collection = parts[0];
  if (parts.size() >= 2) series = parts[1];
}

bool readLine(HalFile& file, std::string& line) {
  line.clear();
  while (file.available()) {
    const int ch = file.read();
    if (ch < 0) break;
    if (ch == '\n') return true;
    if (ch == '\r') continue;
    if (line.size() >= MAX_LINE_BYTES) {
      while (file.available()) {
        const int c = file.read();
        if (c < 0 || c == '\n') break;
      }
      line.clear();
      return true;
    }
    line.push_back(static_cast<char>(ch));
  }
  return !line.empty();
}

bool parseBookLine(const std::string& line, MaxLibraryBook& book) {
  if (line.empty()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, line)) return false;

  const char* path = doc["path"] | nullptr;
  if (!path || !*path) return false;

  book.path = path;
  book.title = doc["title"] | "";
  book.author = doc["author"] | "";
  book.language = doc["language"] | "";
  book.series = doc["series"] | "";
  book.volume = doc["volume"] | 0;
  book.collection = doc["collection"] | "";
  book.favorite = doc["favorite"] | false;

  const int rawStatus = doc["status"] | 0;
  book.status = rawStatus == 2 ? MaxReadingStatus::Read
                              : rawStatus == 1 ? MaxReadingStatus::InProgress
                                               : MaxReadingStatus::Unread;

  book.seriesManual = doc["series_manual"] | false;
  book.volumeManual = doc["volume_manual"] | false;
  book.collectionManual = doc["collection_manual"] | false;
  return true;
}

bool writeBookLine(HalFile& file, const MaxLibraryBook& book) {
  JsonDocument doc;
  doc["path"] = book.path;
  doc["title"] = book.title;
  doc["author"] = book.author;
  doc["language"] = book.language;
  doc["series"] = book.series;
  doc["volume"] = book.volume;
  doc["collection"] = book.collection;
  doc["favorite"] = book.favorite;
  doc["status"] = static_cast<uint8_t>(book.status);
  doc["series_manual"] = book.seriesManual;
  doc["volume_manual"] = book.volumeManual;
  doc["collection_manual"] = book.collectionManual;

  String out;
  serializeJson(doc, out);
  if (file.write(reinterpret_cast<const uint8_t*>(out.c_str()), out.length()) != out.length()) {
    return false;
  }
  return file.write(static_cast<uint8_t>('\n')) == 1;
}

const MaxLibraryBook* findOld(const std::vector<MaxLibraryBook>& oldBooks,
                              const std::string& path) {
  const auto it = std::find_if(oldBooks.begin(), oldBooks.end(),
                               [&](const MaxLibraryBook& b) { return b.path == path; });
  return it == oldBooks.end() ? nullptr : &*it;
}

void fillMetadata(MaxLibraryBook& book) {
  book.title = stemOf(book.path);

  if (FsHelpers::hasEpubExtension(book.path)) {
    Epub epub(book.path, "/.crosspoint");
    // Never force metadata generation for the full library scan.
    if (epub.load(false, true)) {
      if (!epub.getTitle().empty()) book.title = epub.getTitle();
      book.author = epub.getAuthor();
      book.language = epub.getLanguage();
    }
  } else if (FsHelpers::hasXtcExtension(book.path)) {
    Xtc xtc(book.path, "/.crosspoint");
    if (xtc.load()) {
      if (!xtc.getTitle().empty()) book.title = xtc.getTitle();
      book.author = xtc.getAuthor();
    }
  }

  deriveFolderMetadata(book.path, book.collection, book.series);
  if (recentPath(book.path)) book.status = MaxReadingStatus::InProgress;
}

bool mutateByPath(const std::string& path,
                  const std::function<bool(MaxLibraryBook&)>& mutate) {
  std::vector<MaxLibraryBook> books;
  if (!MaxLibraryStore::load(books)) return false;

  bool changed = false;
  for (auto& book : books) {
    if (book.path != path) continue;
    changed = mutate(book);
    break;
  }
  if (!changed) return false;
  return MaxLibraryStore::save(books);
}
}  // namespace

namespace MaxLibraryStore {

const char* statusName(const MaxReadingStatus status) {
  switch (status) {
    case MaxReadingStatus::Unread:
      return "NEW";
    case MaxReadingStatus::InProgress:
      return "READING";
    case MaxReadingStatus::Read:
      return "READ";
  }
  return "";
}

bool load(std::vector<MaxLibraryBook>& books) {
  books.clear();
  HalFile file;
  if (!Storage.openFileForRead(MOD, INDEX_PATH, file)) return false;

  std::string line;
  line.reserve(512);
  while (books.size() < MAX_BOOKS && readLine(file, line)) {
    MaxLibraryBook book;
    if (!parseBookLine(line, book)) continue;
    if (!Storage.exists(book.path.c_str())) continue;
    books.push_back(std::move(book));
  }
  file.close();
  return true;
}

bool save(const std::vector<MaxLibraryBook>& books) {
  if (!Storage.ensureDirectoryExists("/.crosspoint-max")) return false;
  if (!Storage.ensureDirectoryExists(ROOT)) return false;

  const std::string temp = std::string(INDEX_PATH) + ".tmp";
  HalFile file;
  if (!Storage.openFileForWrite(MOD, temp, file)) return false;

  const size_t count = std::min(books.size(), MAX_BOOKS);
  for (size_t i = 0; i < count; ++i) {
    if (!writeBookLine(file, books[i])) {
      file.close();
      Storage.remove(temp.c_str());
      return false;
    }
  }
  file.flush();
  file.close();

  Storage.remove(INDEX_PATH);
  if (!Storage.rename(temp.c_str(), INDEX_PATH)) {
    Storage.remove(temp.c_str());
    return false;
  }
  return true;
}

bool rebuild(std::vector<MaxLibraryBook>& books) {
  std::vector<MaxLibraryBook> oldBooks;
  load(oldBooks);

  books.clear();
  books.reserve(std::min<size_t>(MAX_BOOKS, 128));

  const std::string rootPath = Storage.exists("/Books") ? "/Books" : "/";
  std::vector<std::string> pendingDirs;
  pendingDirs.reserve(24);
  pendingDirs.push_back(rootPath);

  char nameBuffer[NAME_BUFFER_SIZE]{};

  while (!pendingDirs.empty() && books.size() < MAX_BOOKS) {
    std::string dirPath = std::move(pendingDirs.back());
    pendingDirs.pop_back();

    auto dir = Storage.open(dirPath.c_str());
    if (!dir || !dir.isDirectory()) continue;
    dir.rewindDirectory();

    for (auto entry = dir.openNextFile(); entry && books.size() < MAX_BOOKS;
         entry = dir.openNextFile()) {
      entry.getName(nameBuffer, sizeof(nameBuffer));
      if (nameBuffer[0] == '\0' || std::strcmp(nameBuffer, ".") == 0 ||
          std::strcmp(nameBuffer, "..") == 0 ||
          std::strcmp(nameBuffer, "System Volume Information") == 0) {
        continue;
      }

      const bool isDir = entry.isDirectory();
      entry.close();

      if (isDir && (std::strcmp(nameBuffer, ".crosspoint") == 0 ||
                    std::strcmp(nameBuffer, ".crosspoint-max") == 0)) {
        continue;
      }

      std::string full = dirPath;
      if (full.empty() || full.back() != '/') full += '/';
      full += nameBuffer;

      if (isDir) {
        pendingDirs.push_back(std::move(full));
        continue;
      }
      if (!supportedBook(full)) continue;

      MaxLibraryBook book;
      book.path = std::move(full);
      fillMetadata(book);

      if (const MaxLibraryBook* old = findOld(oldBooks, book.path)) {
        book.favorite = old->favorite;
        book.status = old->status;

        book.seriesManual = old->seriesManual;
        book.volumeManual = old->volumeManual;
        book.collectionManual = old->collectionManual;

        if (old->seriesManual) {
          book.series = old->series;
        } else if (!old->series.empty()) {
          book.series = old->series;
        }
        if (old->volumeManual) {
          book.volume = old->volume;
        } else if (old->volume != 0) {
          book.volume = old->volume;
        }
        if (old->collectionManual) {
          book.collection = old->collection;
        } else if (!old->collection.empty()) {
          book.collection = old->collection;
        }

        if (!old->title.empty() && book.title.empty()) book.title = old->title;
        if (!old->author.empty() && book.author.empty()) book.author = old->author;
        if (!old->language.empty() && book.language.empty()) book.language = old->language;
      }

      if (book.status != MaxReadingStatus::Read && recentPath(book.path)) {
        book.status = MaxReadingStatus::InProgress;
      }
      books.push_back(std::move(book));
    }
    dir.close();
  }

  std::sort(books.begin(), books.end(),
            [](const MaxLibraryBook& a, const MaxLibraryBook& b) {
              return FsHelpers::naturalLess(a.path, b.path);
            });

  return save(books);
}

bool getBook(const std::string& path, MaxLibraryBook& book) {
  std::vector<MaxLibraryBook> books;
  if (!load(books)) return false;
  const auto it = std::find_if(books.begin(), books.end(),
                               [&](const MaxLibraryBook& b) { return b.path == path; });
  if (it == books.end()) return false;
  book = *it;
  return true;
}

bool markOpened(const std::string& path, const std::string& title,
                const std::string& author, const std::string& language) {
  std::vector<MaxLibraryBook> books;
  if (!load(books)) return false;

  auto it = std::find_if(books.begin(), books.end(),
                         [&](const MaxLibraryBook& b) { return b.path == path; });
  if (it == books.end()) {
    if (books.size() >= MAX_BOOKS) return false;
    MaxLibraryBook book;
    book.path = path;
    book.title = title.empty() ? stemOf(path) : title;
    book.author = author;
    book.language = language;
    deriveFolderMetadata(path, book.collection, book.series);
    book.status = MaxReadingStatus::InProgress;
    books.push_back(std::move(book));
  } else {
    if (!title.empty()) it->title = title;
    if (!author.empty()) it->author = author;
    if (!language.empty()) it->language = language;
    if (it->status != MaxReadingStatus::Read) it->status = MaxReadingStatus::InProgress;
  }
  return save(books);
}

bool markRead(const std::string& path) {
  return mutateByPath(path, [](MaxLibraryBook& book) {
    if (book.status == MaxReadingStatus::Read) return false;
    book.status = MaxReadingStatus::Read;
    return true;
  });
}

bool toggleFavorite(const std::string& path, bool* newValue) {
  bool value = false;
  const bool ok = mutateByPath(path, [&](MaxLibraryBook& book) {
    book.favorite = !book.favorite;
    value = book.favorite;
    return true;
  });
  if (ok && newValue) *newValue = value;
  return ok;
}

bool cycleStatus(const std::string& path, MaxReadingStatus* newStatus) {
  MaxReadingStatus value = MaxReadingStatus::Unread;
  const bool ok = mutateByPath(path, [&](MaxLibraryBook& book) {
    switch (book.status) {
      case MaxReadingStatus::Unread:
        book.status = MaxReadingStatus::InProgress;
        break;
      case MaxReadingStatus::InProgress:
        book.status = MaxReadingStatus::Read;
        break;
      case MaxReadingStatus::Read:
        book.status = MaxReadingStatus::Unread;
        break;
    }
    value = book.status;
    return true;
  });
  if (ok && newStatus) *newStatus = value;
  return ok;
}

bool setSeries(const std::string& path, const std::string& value) {
  return mutateByPath(path, [&](MaxLibraryBook& book) {
    book.series = value;
    book.seriesManual = true;
    return true;
  });
}

bool setCollection(const std::string& path, const std::string& value) {
  return mutateByPath(path, [&](MaxLibraryBook& book) {
    book.collection = value;
    book.collectionManual = true;
    return true;
  });
}

bool setVolume(const std::string& path, const uint16_t value) {
  return mutateByPath(path, [&](MaxLibraryBook& book) {
    book.volume = value;
    book.volumeManual = true;
    return true;
  });
}

}  // namespace MaxLibraryStore
