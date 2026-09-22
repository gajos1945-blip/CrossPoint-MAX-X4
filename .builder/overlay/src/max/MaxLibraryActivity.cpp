#include "MaxLibraryActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"

namespace fui = freeink::ui;

namespace {
constexpr unsigned long LONG_PRESS_MS = 1000;
constexpr unsigned long VERY_LONG_PRESS_MS = 2200;

const char* viewName(const MaxLibraryActivity::View view) {
  switch (view) {
    case MaxLibraryActivity::View::All:
      return "Wszystkie";
    case MaxLibraryActivity::View::InProgress:
      return "W trakcie";
    case MaxLibraryActivity::View::Unread:
      return "Nieprzeczytane";
    case MaxLibraryActivity::View::Read:
      return "Przeczytane";
    case MaxLibraryActivity::View::Favorites:
      return "Ulubione";
    case MaxLibraryActivity::View::Authors:
      return "Autorzy";
    case MaxLibraryActivity::View::Series:
      return "Serie";
    case MaxLibraryActivity::View::Collections:
      return "Kolekcje";
    case MaxLibraryActivity::View::Count:
      break;
  }
  return "Biblioteka";
}

bool ciLess(const std::string& a, const std::string& b) {
  return FsHelpers::naturalLess(a, b);
}
}  // namespace

void MaxLibraryActivity::onEnter() {
  UiListActivity::onEnter();
  reloadIndex();
}

void MaxLibraryActivity::onExit() {
  Activity::onExit();
  rowItems.clear();
  rowLabels.clear();
  rowSubtitles.clear();
  visible.clear();
  books.clear();
}

void MaxLibraryActivity::reloadIndex() {
  books.clear();
  if (!MaxLibraryStore::load(books)) {
    MaxLibraryStore::rebuild(books);
  }
  indexLimitReached = books.size() >= MaxLibraryStore::MAX_BOOKS;
  rebuildVisible();
}

void MaxLibraryActivity::rebuildVisible() {
  visible.clear();
  visible.reserve(books.size());

  for (int i = 0; i < static_cast<int>(books.size()); ++i) {
    const auto& book = books[i];
    bool include = true;
    switch (view) {
      case View::All:
      case View::Authors:
        break;
      case View::InProgress:
        include = book.status == MaxReadingStatus::InProgress;
        break;
      case View::Unread:
        include = book.status == MaxReadingStatus::Unread;
        break;
      case View::Read:
        include = book.status == MaxReadingStatus::Read;
        break;
      case View::Favorites:
        include = book.favorite;
        break;
      case View::Series:
        include = !book.series.empty();
        break;
      case View::Collections:
        include = !book.collection.empty();
        break;
      case View::Count:
        include = false;
        break;
    }
    if (include) visible.push_back(i);
  }

  std::sort(visible.begin(), visible.end(), [&](const int ai, const int bi) {
    const auto& a = books[ai];
    const auto& b = books[bi];
    switch (view) {
      case View::Authors:
        if (a.author != b.author) return ciLess(a.author, b.author);
        return ciLess(a.title, b.title);
      case View::Series:
        if (a.series != b.series) return ciLess(a.series, b.series);
        if (a.volume != b.volume) {
          if (a.volume == 0) return false;
          if (b.volume == 0) return true;
          return a.volume < b.volume;
        }
        return ciLess(a.title, b.title);
      case View::Collections:
        if (a.collection != b.collection) return ciLess(a.collection, b.collection);
        return ciLess(a.title, b.title);
      default:
        return ciLess(a.title, b.title);
    }
  });

  if (nav.selected >= static_cast<int>(visible.size())) {
    nav.selected = visible.empty() ? 0 : static_cast<int>(visible.size()) - 1;
  }
  nav.top = 0;
  nav.follow(listCount());

  rebuildRows();
}

void MaxLibraryActivity::rebuildRows() {
  rowLabels.clear();
  rowSubtitles.clear();
  rowItems.clear();

  rowLabels.reserve(visible.size());
  rowSubtitles.reserve(visible.size());
  rowItems.reserve(visible.size());

  for (int idx : visible) {
    const auto& book = books[idx];
    rowLabels.push_back(book.title.empty() ? book.path : book.title);

    std::string subtitle;
    if (view == View::Series) {
      subtitle = book.series;
      if (book.volume != 0) subtitle += " #" + std::to_string(book.volume);
    } else if (view == View::Collections) {
      subtitle = book.collection;
    } else {
      subtitle = book.author;
    }

    if (book.favorite) {
      if (!subtitle.empty()) subtitle += " | ";
      subtitle += "*";
    }
    if (!subtitle.empty()) subtitle += " | ";
    subtitle += MaxLibraryStore::statusName(book.status);
    rowSubtitles.push_back(std::move(subtitle));
  }

  for (size_t i = 0; i < visible.size(); ++i) {
    fui::ListItem item;
    item.label = rowLabels[i].c_str();
    item.subtitle = rowSubtitles[i].c_str();
    item.icon = listIconFor(UITheme::getFileIcon(books[visible[i]].path), 32);
    item.actionValue = static_cast<int16_t>(i);
    rowItems.push_back(item);
  }

  header = "MAX ";
  header += viewName(view);
  header += " (";
  header += std::to_string(visible.size());
  header += ")";
  if (indexLimitReached) header += " !";
}

const MaxLibraryBook* MaxLibraryActivity::selectedBook() const {
  if (nav.selected < 0 || nav.selected >= static_cast<int>(visible.size())) return nullptr;
  const int idx = visible[nav.selected];
  return idx >= 0 && idx < static_cast<int>(books.size()) ? &books[idx] : nullptr;
}

MaxLibraryBook* MaxLibraryActivity::selectedBook() {
  if (nav.selected < 0 || nav.selected >= static_cast<int>(visible.size())) return nullptr;
  const int idx = visible[nav.selected];
  return idx >= 0 && idx < static_cast<int>(books.size()) ? &books[idx] : nullptr;
}

void MaxLibraryActivity::cycleView() {
  const int next = (static_cast<int>(view) + 1) % static_cast<int>(View::Count);
  view = static_cast<View>(next);
  nav.selected = 0;
  rebuildVisible();
  requestUpdate();
}

void MaxLibraryActivity::activateIndex(const int index) {
  if (index < 0 || index >= listCount()) return;
  const auto& book = books[visible[index]];
  app.clearTapFlash();
  activityManager.goToReader(book.path);
}

bool MaxLibraryActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (visible.empty()) return true;
    MaxLibraryBook* book = selectedBook();
    if (!book) return true;

    const unsigned long held = mappedInput.getHeldTime();
    if (held >= VERY_LONG_PRESS_MS) {
      MaxReadingStatus newStatus;
      if (MaxLibraryStore::cycleStatus(book->path, &newStatus)) {
        book->status = newStatus;
        rebuildVisible();
        requestUpdate();
      }
    } else if (held >= LONG_PRESS_MS) {
      bool favorite = false;
      if (MaxLibraryStore::toggleFavorite(book->path, &favorite)) {
        book->favorite = favorite;
        rebuildVisible();
        requestUpdate();
      }
    } else {
      activateIndex(nav.selected);
    }
    return true;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (mappedInput.getHeldTime() >= LONG_PRESS_MS) {
      cycleView();
    } else {
      onGoHome();
    }
    return true;
  }

  return false;
}

void MaxLibraryActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(
      fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                  static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  if (visible.empty()) {
    screen.centeredText("Brak ksiazek w tym widoku", screen.theme().bodyText);
    return;
  }

  fui::ListProps props;
  props.items = rowItems.data();
  props.count = static_cast<uint16_t>(rowItems.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;

  fui::TextStyle label = screen.theme().smallText;
  label.bold = true;
  props.labelText = label;
  syncListViewport(screen, props, true);
  screen.list(props);
}

void MaxLibraryActivity::drawFooter() {
  const bool empty = visible.empty();
  const auto labels = mappedInput.mapLabels(
      "Back/Hold=View", empty ? "" : "Open/Hold=Fav/2s=Status",
      empty ? "" : tr(STR_DIR_UP), empty ? "" : tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
