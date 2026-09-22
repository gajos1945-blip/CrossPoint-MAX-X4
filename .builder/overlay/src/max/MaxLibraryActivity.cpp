#include "MaxLibraryActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cctype>

#include "MappedInputManager.h"
#include "MaxBookActionsActivity.h"
#include "activities/ActivityManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"

namespace fui = freeink::ui;

namespace {
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

std::string asciiFold(const std::string& value) {
  std::string out = value;
  for (char& c : out) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (u < 128) c = static_cast<char>(std::tolower(u));
  }
  return out;
}

bool containsQuery(const MaxLibraryBook& book, const std::string& foldedQuery) {
  if (foldedQuery.empty()) return true;
  return asciiFold(book.title).find(foldedQuery) != std::string::npos ||
         asciiFold(book.author).find(foldedQuery) != std::string::npos ||
         asciiFold(book.series).find(foldedQuery) != std::string::npos ||
         asciiFold(book.collection).find(foldedQuery) != std::string::npos ||
         asciiFold(book.path).find(foldedQuery) != std::string::npos;
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
  const std::string query = asciiFold(searchQuery);

  for (int i = 0; i < static_cast<int>(books.size()); ++i) {
    const auto& book = books[i];
    bool include = containsQuery(book, query);
    if (!include) continue;

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

  if (nav.selected >= listCount()) nav.selected = listCount() - 1;
  if (nav.selected < 0) nav.selected = 0;
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
  rowItems.reserve(visible.size() + CONTROL_ROWS);

  fui::ListItem search;
  search.label = "Szukaj";
  search.value = searchQuery.empty() ? "(wszystkie)" : searchQuery.c_str();
  search.actionValue = 0;
  rowItems.push_back(search);

  fui::ListItem viewRow;
  viewRow.label = "Widok";
  viewRow.value = viewName(view);
  viewRow.actionValue = 1;
  rowItems.push_back(viewRow);

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
    item.actionValue = static_cast<int16_t>(i + CONTROL_ROWS);
    rowItems.push_back(item);
  }

  header = "MAX ";
  header += viewName(view);
  header += " (";
  header += std::to_string(visible.size());
  header += ")";
  if (!searchQuery.empty()) header += " Q";
  if (indexLimitReached) header += " !";
}

const MaxLibraryBook* MaxLibraryActivity::selectedBook() const {
  const int index = nav.selected - CONTROL_ROWS;
  if (index < 0 || index >= static_cast<int>(visible.size())) return nullptr;
  const int bookIndex = visible[index];
  return bookIndex >= 0 && bookIndex < static_cast<int>(books.size()) ? &books[bookIndex] : nullptr;
}

void MaxLibraryActivity::cycleView() {
  const int next = (static_cast<int>(view) + 1) % static_cast<int>(View::Count);
  view = static_cast<View>(next);
  nav.selected = 1;
  rebuildVisible();
  requestUpdate();
}

void MaxLibraryActivity::openSearch() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(
          renderer, mappedInput, "Szukaj: tytul/autor/seria/kolekcja",
          searchQuery, 80, InputType::Text),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          searchQuery = std::get<KeyboardResult>(result.data).text;
          nav.selected = 0;
          rebuildVisible();
        }
        requestUpdate();
      });
}

void MaxLibraryActivity::openBookActions(const int visibleIndex) {
  if (visibleIndex < 0 || visibleIndex >= static_cast<int>(visible.size())) return;
  const std::string path = books[visible[visibleIndex]].path;
  startActivityForResult(
      std::make_unique<MaxBookActionsActivity>(renderer, mappedInput, path),
      [this](const ActivityResult&) {
        reloadIndex();
        requestUpdate();
      });
}

void MaxLibraryActivity::activateIndex(const int index) {
  if (index == 0) {
    openSearch();
    return;
  }
  if (index == 1) {
    cycleView();
    return;
  }

  const int visibleIndex = index - CONTROL_ROWS;
  if (visibleIndex < 0 || visibleIndex >= static_cast<int>(visible.size())) return;
  const auto& book = books[visible[visibleIndex]];
  app.clearTapFlash();
  activityManager.goToReader(book.path);
}

void MaxLibraryActivity::onRowLongPress(const int index) {
  if (index < CONTROL_ROWS) return;
  openBookActions(index - CONTROL_ROWS);
}

bool MaxLibraryActivity::handleButtons() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    const int selected = nav.selected;
    if (selected >= CONTROL_ROWS && mappedInput.getHeldTime() >= ACTION_HOLD_MS) {
      openBookActions(selected - CONTROL_ROWS);
    } else if (selected >= 0 && selected < listCount()) {
      activateIndex(selected);
    }
    return true;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    if (!searchQuery.empty()) {
      searchQuery.clear();
      nav.selected = 0;
      rebuildVisible();
      requestUpdate();
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

  fui::ListProps props;
  props.items = rowItems.data();
  props.count = static_cast<uint16_t>(rowItems.size());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch | fui::InputLongPress;
  fui::TextStyle label = screen.theme().smallText;
  label.bold = true;
  props.labelText = label;
  props.valueInset = 8;
  syncListViewport(screen, props, true);
  screen.list(props);
}

void MaxLibraryActivity::drawFooter() {
  const auto labels = mappedInput.mapLabels(
      searchQuery.empty() ? "Home" : "Wyczysc",
      nav.selected >= CONTROL_ROWS ? "Open / Hold: Akcje" : "Select",
      tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
