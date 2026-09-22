#include "MaxBookActionsActivity.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {
const char* LABELS[5] = {
    "Ulubione",
    "Status",
    "Seria",
    "Kolekcja",
    "Tom",
};

bool digitsOnly(const std::string& value) {
  if (value.empty()) return true;
  return std::all_of(value.begin(), value.end(),
                     [](const unsigned char c) { return std::isdigit(c) != 0; });
}
}

bool MaxBookActionsActivity::reload() {
  return MaxLibraryStore::getBook(bookPath, book);
}

void MaxBookActionsActivity::onEnter() {
  UiListActivity::onEnter();
  if (!reload()) {
    finish();
    return;
  }
  rebuildRows();
}

void MaxBookActionsActivity::rebuildRows() {
  values[0] = book.favorite ? "TAK" : "NIE";
  values[1] = MaxLibraryStore::statusName(book.status);
  values[2] = book.series.empty() ? "(brak)" : book.series;
  values[3] = book.collection.empty() ? "(brak)" : book.collection;
  values[4] = book.volume == 0 ? "(brak)" : std::to_string(book.volume);

  for (size_t i = 0; i < rows.size(); ++i) {
    rows[i] = {};
    rows[i].label = LABELS[i];
    rows[i].value = values[i].c_str();
    rows[i].actionValue = static_cast<int16_t>(i);
  }
}

void MaxBookActionsActivity::editSeries() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Seria", book.series, 80, InputType::Text),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          const auto& kb = std::get<KeyboardResult>(result.data);
          if (MaxLibraryStore::setSeries(bookPath, kb.text)) {
            reload();
            rebuildRows();
          }
        }
        requestUpdate();
      });
}

void MaxBookActionsActivity::editCollection() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Kolekcja", book.collection, 80, InputType::Text),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          const auto& kb = std::get<KeyboardResult>(result.data);
          if (MaxLibraryStore::setCollection(bookPath, kb.text)) {
            reload();
            rebuildRows();
          }
        }
        requestUpdate();
      });
}

void MaxBookActionsActivity::editVolume() {
  const std::string current = book.volume == 0 ? "" : std::to_string(book.volume);
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Tom (0-9999)", current, 4, InputType::Text),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          const auto& kb = std::get<KeyboardResult>(result.data);
          if (digitsOnly(kb.text)) {
            const unsigned long raw = kb.text.empty() ? 0UL : std::strtoul(kb.text.c_str(), nullptr, 10);
            if (raw <= 9999UL &&
                MaxLibraryStore::setVolume(bookPath, static_cast<uint16_t>(raw))) {
              reload();
              rebuildRows();
            }
          }
        }
        requestUpdate();
      });
}

void MaxBookActionsActivity::activateIndex(const int index) {
  switch (index) {
    case 0: {
      bool value = false;
      if (MaxLibraryStore::toggleFavorite(bookPath, &value)) {
        book.favorite = value;
        rebuildRows();
        requestUpdate();
      }
      break;
    }
    case 1: {
      MaxReadingStatus value;
      if (MaxLibraryStore::cycleStatus(bookPath, &value)) {
        book.status = value;
        rebuildRows();
        requestUpdate();
      }
      break;
    }
    case 2:
      editSeries();
      break;
    case 3:
      editCollection();
      break;
    case 4:
      editVolume();
      break;
    default:
      break;
  }
}

void MaxBookActionsActivity::buildScreen(UiScreen& screen) {
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
