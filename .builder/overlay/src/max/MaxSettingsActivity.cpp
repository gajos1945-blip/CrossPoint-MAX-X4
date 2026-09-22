#include "MaxSettingsActivity.h"

#include <GfxRenderer.h>

#include <vector>

#include "MaxLibraryStore.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
const char* LABELS[MaxSettingsActivity::ROWS] = {
    "Translation Gateway",
    "Jezyk zrodlowy",
    "Jezyk docelowy",
    "Tryb nauki",
    "Cache tlumaczen",
    "Przebuduj MAX Library",
};
}

void MaxSettingsActivity::onEnter() {
  UiListActivity::onEnter();
  config = MaxTranslationConfig::load();
  rebuildRows();
}

void MaxSettingsActivity::rebuildRows() {
  values[0] = config.gateway.empty() ? "(nie ustawiono)" : config.gateway;
  values[1] = config.source;
  values[2] = config.target;
  values[3] = config.studyMode ? "ON" : "OFF";
  values[4] = config.cache ? "ON" : "OFF";

  std::vector<MaxLibraryBook> books;
  if (MaxLibraryStore::load(books)) {
    values[5] = std::to_string(books.size()) + " ksiazek";
  } else {
    values[5] = "brak indeksu";
  }

  for (int i = 0; i < ROWS; ++i) {
    rows[i] = {};
    rows[i].label = LABELS[i];
    rows[i].value = values[i].c_str();
    rows[i].actionValue = static_cast<int16_t>(i);
  }
}

void MaxSettingsActivity::editGateway() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Translation Gateway", config.gateway, 160, InputType::Url),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          config.gateway = std::get<KeyboardResult>(result.data).text;
          MaxTranslationConfig::save(config);
          rebuildRows();
        }
        requestUpdate();
      });
}

void MaxSettingsActivity::editSource() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Jezyk zrodlowy (np. auto/en/de)", config.source, 16, InputType::Text),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          const std::string value = std::get<KeyboardResult>(result.data).text;
          if (!value.empty()) {
            config.source = value;
            MaxTranslationConfig::save(config);
            rebuildRows();
          }
        }
        requestUpdate();
      });
}

void MaxSettingsActivity::editTarget() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Jezyk docelowy (np. pl/en/de)", config.target, 16, InputType::Text),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          const std::string value = std::get<KeyboardResult>(result.data).text;
          if (!value.empty()) {
            config.target = value;
            MaxTranslationConfig::save(config);
            rebuildRows();
          }
        }
        requestUpdate();
      });
}

void MaxSettingsActivity::rebuildLibrary() {
  std::vector<MaxLibraryBook> books;
  if (MaxLibraryStore::rebuild(books)) {
    header = "MAX Settings - Library OK";
  } else {
    header = "MAX Settings - Library ERROR";
  }
  rebuildRows();
  requestUpdate();
}

void MaxSettingsActivity::activateIndex(const int index) {
  switch (index) {
    case 0:
      editGateway();
      break;
    case 1:
      editSource();
      break;
    case 2:
      editTarget();
      break;
    case 3:
      config.studyMode = !config.studyMode;
      MaxTranslationConfig::save(config);
      rebuildRows();
      requestUpdate();
      break;
    case 4:
      config.cache = !config.cache;
      MaxTranslationConfig::save(config);
      rebuildRows();
      requestUpdate();
      break;
    case 5:
      rebuildLibrary();
      break;
    default:
      break;
  }
}

void MaxSettingsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  fui::ListProps props;
  props.items = rows.data();
  props.count = static_cast<uint16_t>(rows.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  props.labelText = screen.theme().smallText;
  props.labelText.maxLines = 2;
  syncListViewport(screen, props);
  screen.list(props);
}
