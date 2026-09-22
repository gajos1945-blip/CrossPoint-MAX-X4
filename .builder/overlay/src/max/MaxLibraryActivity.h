#pragma once

#include <string>
#include <vector>

#include "MaxLibraryStore.h"
#include "activities/UiListActivity.h"

class MaxLibraryActivity final : public UiListActivity {
 public:
  enum class View : uint8_t {
    All,
    InProgress,
    Unread,
    Read,
    Favorites,
    Authors,
    Series,
    Collections,
    Count
  };

 private:
  std::vector<MaxLibraryBook> books;
  std::vector<int> visible;
  std::vector<std::string> rowLabels;
  std::vector<std::string> rowSubtitles;
  std::vector<freeink::ui::ListItem> rowItems;

  View view = View::All;
  std::string header;
  bool indexLimitReached = false;

  void rebuildVisible();
  void rebuildRows();
  void cycleView();
  void reloadIndex();
  const MaxLibraryBook* selectedBook() const;
  MaxLibraryBook* selectedBook();

 protected:
  int listCount() const override { return static_cast<int>(visible.size()); }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  bool handleButtons() override;
  const char* headerTitle() const override { return header.c_str(); }
  void drawFooter() override;

 public:
  explicit MaxLibraryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : UiListActivity("MaxLibrary", renderer, mappedInput, false) {}

  void onEnter() override;
  void onExit() override;
};
