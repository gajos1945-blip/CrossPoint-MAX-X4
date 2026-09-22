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
    ensure_absent(reader_cpp, "openMaxTranslateChapter()")

    replace_once(
        menu_h,
        "    DELETE_CACHE,\n    DICTIONARY\n",
        "    DELETE_CACHE,\n    DICTIONARY,\n    TRANSLATE_PAGE,\n    TRANSLATE_CHAPTER\n",
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
        "  static constexpr size_t MAX_MENU_ITEMS = 18;\n",
        "menu capacity",
    )

    replace_once(
        menu_cpp,
        "    item.label = I18N.get(menuItems[i].labelId);\n",
        "    item.label = menuItems[i].customLabel ? menuItems[i].customLabel : I18N.get(menuItems[i].labelId);\n",
        "menu label rendering",
    )
    replace_once(
        menu_cpp,
        "  items.push_back({MenuAction::DICTIONARY, StrId::STR_LOOKUP});\n",
        "  items.push_back({MenuAction::DICTIONARY, StrId::STR_LOOKUP});\n"
        "  items.push_back({MenuAction::TRANSLATE_PAGE, StrId::STR_LOOKUP, \"Translate Page\"});\n"
        "  items.push_back({MenuAction::TRANSLATE_CHAPTER, StrId::STR_LOOKUP, \"Translate Chapter\"});\n",
        "translation menu insertion",
    )

    replace_once(
        reader_cpp,
        '#include "MappedInputManager.h"\n',
        '#include "MappedInputManager.h"\n'
        '#include "max/MaxChapterTranslationActivity.h"\n'
        '#include "max/MaxPageText.h"\n'
        '#include "max/MaxTranslateActivity.h"\n',
        "reader MAX includes",
    )

    replace_once(
        reader_h,
        "  void openDictionaryWordSelect();\n",
        "  void openDictionaryWordSelect();\n"
        "  void openMaxTranslatePage();\n"
        "  void openMaxTranslateChapter();\n",
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
          renderer, mappedInput, epub->getPath(), currentSpineIndex, section.get()),
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
        "    }\n",
        "reader translation switch",
    )

    replace_once(
        reader_cpp,
        '  return row >= 0 && row < static_cast<int>(moreItems.size()) ? I18N.get(moreItems[row].labelId) : "";\n',
        '  if (row < 0 || row >= static_cast<int>(moreItems.size())) return "";\n'
        '  return moreItems[row].customLabel ? moreItems[row].customLabel : I18N.get(moreItems[row].labelId);\n',
        "toolbar More custom label",
    )

    dest = repo / "src/max"
    if dest.exists():
        raise PatchError(f"{dest} already exists; refusing to overwrite")
    shutil.copytree(overlay / "src/max", dest)
    print("CrossPoint MAX v1.2-dev patch applied safely.")

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("repo")
    ap.add_argument("--overlay", default=str(Path(__file__).resolve().parents[1] / "overlay"))
    ns = ap.parse_args()
    apply(Path(ns.repo).resolve(), Path(ns.overlay).resolve())
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except PatchError as exc:
        print(f"PATCH ERROR: {exc}", file=sys.stderr)
        raise SystemExit(2)
