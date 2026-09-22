#pragma once

#include <array>
#include <string>

#include "activities/UiListActivity.h"
#include "MaxTranslationConfig.h"

class MaxSettingsActivity final : public UiListActivity {
 public:
  static constexpr int ROWS = 6;

 private:

  MaxTranslationConfig config;
  std::array<std::string, ROWS> values;
  std::array<freeink::ui::ListItem, ROWS> rows{};
  std::string header = "MAX Settings";

  void rebuildRows();
  void editGateway();
  void editSource();
  void editTarget();
  void rebuildLibrary();

 protected:
  int listCount() const override { return ROWS; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override { return header.c_str(); }

 public:
  MaxSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : UiListActivity("MaxSettings", renderer, mappedInput) {}

  void onEnter() override;
};
