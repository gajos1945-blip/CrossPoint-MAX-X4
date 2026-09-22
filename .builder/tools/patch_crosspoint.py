from __future__ import annotations
from pathlib import Path
import argparse
import shutil
import sys

class PatchError(RuntimeError):
    pass

def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise PatchError(f"{label}: expected exactly one anchor in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")

def ensure_absent(path: Path, marker: str) -> None:
    if marker in path.read_text(encoding="utf-8"):
        raise PatchError(f"Patch marker already present in {path}; refusing double patch")

def apply(repo: Path, overlay: Path) -> None:
    menu_h = repo / "src/activities/reader/EpubReaderMenuActivity.h"
    menu_cpp = repo / "src/activities/reader/EpubReaderMenuActivity.cpp"
    reader_h = repo / "src/activities/reader/EpubReaderActivity.h"
    reader_cpp = repo / "src/activities/reader/EpubReaderActivity.cpp"

    for p in (menu_h, menu_cpp, reader_h, reader_cpp):
        if not p.is_file():
            raise PatchError(f"Required upstream file missing: {p}")

    ensure_absent(menu_h, "TRANSLATE_PAGE")
    ensure_absent(reader_cpp, "openMaxTranslateBook()")

    replace_once(
        menu_h,
        "    DELETE_CACHE,\n    DICTIONARY\n",
        "    DELETE_CACHE,\n    DICTIONARY,\n    TRANSLATE_PAGE,\n"
        "    TRANSLATE_CHAPTER,\n    TRANSLATE_BOOK\n",
        "menu enum",
    )
    replace_once(
        menu_h,
        "    StrId labelId;\n",
        "    StrId labelId;\n    const char* customLabel = nullptr;\n",
        "menu custom label",
    )
    replace_once(
        menu_h,
        "  static constexpr size_t MAX_MENU_ITEMS = 16;\n",
        "  static constexpr size_t MAX_MENU_ITEMS = 19;\n",
        "menu capacity",
    )

    replace_once(
        menu_cpp,
        "    item.label = I18N.get(menuItems[i].labelId);\n",
        "    item.label = menuItems[i].customLabel ? menuItems[i].customLabel : "
        "I18N.get(menuItems[i].labelId);\n",
        "menu label rendering",
    )
    replace_once(
        menu_cpp,
        "  items.push_back({MenuAction::DICTIONARY, StrId::STR_LOOKUP});\n",
        "  items.push_back({MenuAction::DICTIONARY, StrId::STR_LOOKUP});\n"
        "  items.push_back({MenuAction::TRANSLATE_PAGE, StrId::STR_LOOKUP, "
        "\"Translate Page\"});\n"
        "  items.push_back({MenuAction::TRANSLATE_CHAPTER, StrId::STR_LOOKUP, "
        "\"Translate Chapter\"});\n"
        "  items.push_back({MenuAction::TRANSLATE_BOOK, StrId::STR_LOOKUP, "
        "\"Translate Book\"});\n",
        "translation menu insertion",
    )

    replace_once(
        reader_cpp,
        '#include "MappedInputManager.h"\n',
        '#include "MappedInputManager.h"\n'
        '#include "max/MaxChapterTranslationActivity.h"\n'
        '#include "max/MaxLibraryStore.h"\n'
        '#include "max/MaxPageText.h"\n'
        '#include "max/MaxTranslateActivity.h"\n'
        '#include "max/MaxWholeBookTranslationActivity.h"\n',
        "reader MAX includes",
    )

    replace_once(
        reader_h,
        "  void openDictionaryWordSelect();\n",
        "  void openDictionaryWordSelect();\n"
        "  void openMaxTranslatePage();\n"
        "  void openMaxTranslateChapter();\n"
        "  void openMaxTranslateBook();\n",
        "reader MAX method declarations",
    )

    anchor = "void EpubReaderActivity::openDictionaryWordSelect() {\n"
    impl = '''void EpubReaderActivity::openMaxTranslatePage() {
  if (!section || !epub) {
    requestUpdate();
    return;
  }

  const int sourcePage = section->currentPage;
  const int sourceSpine = currentSpineIndex;
  auto page = section->loadPage(sourcePage);
  if (!page) {
    requestUpdate();
    return;
  }

  std::string text = MaxPageText::extract(*page);
  if (text.empty()) {
    requestUpdate();
    return;
  }

  startActivityForResult(
      std::make_unique<MaxTranslateActivity>(renderer, mappedInput, epub->getPath(),
                                             sourceSpine, sourcePage, std::move(text)),
      [this](const ActivityResult& result) {
        if (!result.isCancelled) {
          const auto* pageResult = std::get_if<PageResult>(&result.data);
          if (pageResult && pageResult->page == 1) {
            pageTurn(true);
          }
        }
        requestUpdate();
      });
}

void EpubReaderActivity::openMaxTranslateChapter() {
  if (!section || !epub) {
    requestUpdate();
    return;
  }

  startActivityForResult(
      std::make_unique<MaxChapterTranslationActivity>(
          renderer, mappedInput, epub->getPath(), currentSpineIndex, section.get(),
          SETTINGS.readerRenderSpec(buildViewportWidth, buildViewportHeight)),
      [this](const ActivityResult&) { requestUpdate(); });
}

void EpubReaderActivity::openMaxTranslateBook() {
  if (!epub || buildViewportWidth == 0 || buildViewportHeight == 0) {
    requestUpdate();
    return;
  }

  const ReaderRenderSpec spec =
      SETTINGS.readerRenderSpec(buildViewportWidth, buildViewportHeight);
  if (section) {
    nextPageNumber = section->currentPage;
    section.reset();
  }
  startActivityForResult(
      std::make_unique<MaxWholeBookTranslationActivity>(
          renderer, mappedInput, epub, spec),
      [this](const ActivityResult&) { requestUpdate(); });
}

'''
    replace_once(reader_cpp, anchor, impl + anchor, "reader MAX methods")

    replace_once(
        reader_cpp,
        "    case EpubReaderMenuActivity::MenuAction::DICTIONARY: {\n"
        "      openDictionaryWordSelect();\n"
        "      break;\n"
        "    }\n",
        "    case EpubReaderMenuActivity::MenuAction::DICTIONARY: {\n"
        "      openDictionaryWordSelect();\n"
        "      break;\n"
        "    }\n"
        "    case EpubReaderMenuActivity::MenuAction::TRANSLATE_PAGE: {\n"
        "      openMaxTranslatePage();\n"
        "      break;\n"
        "    }\n"
        "    case EpubReaderMenuActivity::MenuAction::TRANSLATE_CHAPTER: {\n"
        "      openMaxTranslateChapter();\n"
        "      break;\n"
        "    }\n"
        "    case EpubReaderMenuActivity::MenuAction::TRANSLATE_BOOK: {\n"
        "      openMaxTranslateBook();\n"
        "      break;\n"
        "    }\n",
        "reader translation switch",
    )

    replace_once(
        reader_cpp,
        '  return row >= 0 && row < static_cast<int>(moreItems.size()) ? '
        'I18N.get(moreItems[row].labelId) : "";\n',
        '  if (row < 0 || row >= static_cast<int>(moreItems.size())) return "";\n'
        '  return moreItems[row].customLabel ? moreItems[row].customLabel : '
        'I18N.get(moreItems[row].labelId);\n',
        "toolbar More custom label",
    )

    # MAX Library lifecycle: one persistent status update when a book is opened,
    # and one when the reader reaches the end.
    replace_once(
        reader_h,
        "  bool recentsEntryRemoved = false;\n",
        "  bool recentsEntryRemoved = false;\n"
        "  bool maxLibraryReadMarked = false;\n",
        "reader MAX library read flag",
    )
    replace_once(
        reader_cpp,
        "  epub = std::move(loadedEpub);\n",
        "  epub = std::move(loadedEpub);\n"
        "  MaxLibraryStore::markOpened(epub->getPath(), epub->getTitle(), "
        "epub->getAuthor(), epub->getLanguage());\n",
        "reader MAX library opened state",
    )
    replace_once(
        reader_cpp,
        "  const bool atEndOfBook = currentSpineIndex > 0 && "
        "currentSpineIndex >= epub->getSpineItemsCount();\n",
        "  const bool atEndOfBook = currentSpineIndex > 0 && "
        "currentSpineIndex >= epub->getSpineItemsCount();\n"
        "  if (atEndOfBook && !maxLibraryReadMarked) {\n"
        "    MaxLibraryStore::markRead(epub->getPath());\n"
        "    maxLibraryReadMarked = true;\n"
        "  } else if (!atEndOfBook) {\n"
        "    maxLibraryReadMarked = false;\n"
        "  }\n",
        "reader MAX library read state",
    )

    # MAX Library activity manager integration.
    am_h = repo / "src/activities/ActivityManager.h"
    am_cpp = repo / "src/activities/ActivityManager.cpp"
    home_h = repo / "src/activities/home/HomeActivity.h"
    home_cpp = repo / "src/activities/home/HomeActivity.cpp"
    for p in (am_h, am_cpp, home_h, home_cpp):
        if not p.is_file():
            raise PatchError(f"Required MAX Library upstream file missing: {p}")

    ensure_absent(am_h, "MAX_LIBRARY")
    ensure_absent(am_cpp, "goToMaxLibrary()")

    replace_once(
        am_h,
        "enum class HomeMenuItem { NONE, FILE_BROWSER, RECENTS, OPDS_BROWSER, FILE_TRANSFER, SETTINGS_MENU };\n",
        "enum class HomeMenuItem { NONE, MAX_LIBRARY, MAX_SETTINGS, FILE_BROWSER, RECENTS, "
        "OPDS_BROWSER, FILE_TRANSFER, SETTINGS_MENU };\n",
        "ActivityManager HomeMenuItem",
    )
    replace_once(
        am_h,
        "  void goToFileBrowser(std::string path = {});\n",
        "  void goToFileBrowser(std::string path = {});\n"
        "  void goToMaxLibrary();\n"
        "  void goToMaxSettings();\n",
        "ActivityManager goToMaxLibrary declaration",
    )
    replace_once(
        am_cpp,
        '#include "home/RecentBooksActivity.h"\n',
        '#include "home/RecentBooksActivity.h"\n'
        '#include "max/MaxLibraryActivity.h"\n'
        '#include "max/MaxSettingsActivity.h"\n',
        "ActivityManager MAX Library include",
    )
    replace_once(
        am_cpp,
        "void ActivityManager::goToRecentBooks() {\n",
        "void ActivityManager::goToMaxLibrary() {\n"
        "  replaceActivity(std::make_unique<MaxLibraryActivity>(renderer, mappedInput));\n"
        "}\n"
        "void ActivityManager::goToMaxSettings() {\n"
        "  replaceActivity(std::make_unique<MaxSettingsActivity>(renderer, mappedInput));\n"
        "}\n"
        "void ActivityManager::goToRecentBooks() {\n",
        "ActivityManager goToMaxLibrary implementation",
    )
    replace_once(
        am_cpp,
        '    if (activityName == "FileBrowser") {\n',
        '    if (activityName == "MaxLibrary") {\n'
        '      initialMenuItem = HomeMenuItem::MAX_LIBRARY;\n'
        '    } else if (activityName == "MaxSettings") {\n'
        '      initialMenuItem = HomeMenuItem::MAX_SETTINGS;\n'
        '    } else if (activityName == "FileBrowser") {\n',
        "ActivityManager home return mapping",
    )

    # Home menu: put MAX Library first, while preserving existing recent covers
    # and all stock navigation.
    replace_once(
        home_h,
        "    if (item == HomeMenuItem::FILE_BROWSER) return i;\n",
        "    if (item == HomeMenuItem::MAX_LIBRARY) return i;\n"
        "    ++i;\n"
        "    if (item == HomeMenuItem::MAX_SETTINGS) return i;\n"
        "    ++i;\n"
        "    if (item == HomeMenuItem::FILE_BROWSER) return i;\n",
        "Home menu item mapping forward",
    )
    replace_once(
        home_h,
        "    if (idx == i++) return HomeMenuItem::FILE_BROWSER;\n",
        "    if (idx == i++) return HomeMenuItem::MAX_LIBRARY;\n"
        "    if (idx == i++) return HomeMenuItem::MAX_SETTINGS;\n"
        "    if (idx == i++) return HomeMenuItem::FILE_BROWSER;\n",
        "Home menu item mapping reverse",
    )
    replace_once(
        home_h,
        "  void onFileBrowserOpen();\n",
        "  void onFileBrowserOpen();\n"
        "  void onMaxLibraryOpen();\n"
        "  void onMaxSettingsOpen();\n",
        "Home MAX Library declaration",
    )
    replace_once(
        home_cpp,
        "  int count = 4;  // File Browser, Recents, File transfer, Settings\n",
        "  int count = 6;  // MAX Library, MAX Settings, File Browser, Recents, File transfer, Settings\n",
        "Home base menu count",
    )
    replace_once(
        home_cpp,
        "      case HomeMenuItem::FILE_BROWSER:\n"
        "        onFileBrowserOpen();\n"
        "        break;\n",
        "      case HomeMenuItem::MAX_LIBRARY:\n"
        "        onMaxLibraryOpen();\n"
        "        break;\n"
        "      case HomeMenuItem::FILE_BROWSER:\n"
        "        onFileBrowserOpen();\n"
        "        break;\n",
        "Home MAX Library action",
    )
    # Patch the two unique source lines separately.  The pinned upstream wraps
    # menuItems across two physical lines; matching the whole block made the
    # patch unnecessarily whitespace-sensitive.
    replace_once(
        home_cpp,
        "  std::vector<const char*> menuItems = {tr(STR_BROWSE_FILES), tr(STR_MENU_RECENT_BOOKS), tr(STR_FILE_TRANSFER),\n",
        "  std::vector<const char*> menuItems = {\"MAX Library\", \"MAX Settings\", tr(STR_BROWSE_FILES), "
        "tr(STR_MENU_RECENT_BOOKS), tr(STR_FILE_TRANSFER),\n",
        "Home MAX Library menuItems line",
    )
    replace_once(
        home_cpp,
        "  std::vector<UIIcon> menuIcons = {Folder, Recent, Transfer, Settings};\n",
        "  std::vector<UIIcon> menuIcons = {Library, Settings, Folder, Recent, Transfer, Settings};\n",
        "Home MAX Library menuIcons line",
    )
    replace_once(
        home_cpp,
        "    menuItems.insert(menuItems.begin() + 2, tr(STR_OPDS_BROWSER));\n"
        "    menuIcons.insert(menuIcons.begin() + 2, Library);\n",
        "    menuItems.insert(menuItems.begin() + 4, tr(STR_OPDS_BROWSER));\n"
        "    menuIcons.insert(menuIcons.begin() + 4, Library);\n",
        "Home OPDS insertion offset",
    )
    replace_once(
        home_cpp,
        "metrics.homeContinueReadingInMenu && !recentBooks.empty() ? "
        "recentBooks[0].title.c_str() : nullptr);",
        "metrics.homeContinueReadingInMenu && !recentBooks.empty() ? "
        "recentBooks[0].title.c_str() : \"CrossPoint MAX\");",
        "Home MAX branding",
    )
    replace_once(
        home_cpp,
        "void HomeActivity::onFileBrowserOpen() { activityManager.goToFileBrowser(); }\n",
        "void HomeActivity::onFileBrowserOpen() { activityManager.goToFileBrowser(); }\n"
        "void HomeActivity::onMaxLibraryOpen() { activityManager.goToMaxLibrary(); }\n"
        "void HomeActivity::onMaxSettingsOpen() { activityManager.goToMaxSettings(); }\n",
        "Home MAX Library implementation",
    )


    dest = repo / "src/max"
    if dest.exists():
        raise PatchError(f"{dest} already exists; refusing to overwrite")
    shutil.copytree(overlay / "src/max", dest)
    print("CrossPoint MAX v1.5-dev patch applied safely.")

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("repo")
    ap.add_argument("--overlay",
                    default=str(Path(__file__).resolve().parents[1] / "overlay"))
    ns = ap.parse_args()
    apply(Path(ns.repo).resolve(), Path(ns.overlay).resolve())
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except PatchError as exc:
        print(f"PATCH ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
