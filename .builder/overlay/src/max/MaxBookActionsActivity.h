#pragma once

#include <array>
#include <string>

#include "MaxLibraryStore.h"
#include "activities/UiListActivity.h"

class MaxBookActionsActivity final : public UiListActivity {
  std::string bookPath;
  MaxLibraryBook book;
  std::array<std::string, 5> values;
  std::array<freeink::ui::ListItem, 5> rows{};

  bool reload();
  void rebuildRows();
  void editSeries();
  void editCollection();
  void editVolume();

 protected:
  int listCount() const override { return 5; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override { return "MAX Book"; }

 public:
  MaxBookActionsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                         std::string bookPath)
      : UiListActivity("MaxBookActions", renderer, mappedInput),
        bookPath(std::move(bookPath)) {}

  void onEnter() override;
};
