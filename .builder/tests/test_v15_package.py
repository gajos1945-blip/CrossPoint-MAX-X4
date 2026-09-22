from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]

def test_manifest_v15():
    m = json.loads((ROOT / ".builder/manifest.json").read_text(encoding="utf-8"))
    assert m["version"] == "1.5-dev"
    assert "free-text library search via upstream KeyboardEntryActivity" in m["implemented"]
    assert "MAX Settings home entry" in m["implemented"]
    assert m["automatic_flash"] is False

def test_expected_bin_and_auto_push():
    wf = (ROOT / ".github/workflows/build-x4-bin.yml").read_text(encoding="utf-8")
    assert "push:" in wf
    assert "branches: [main]" in wf
    assert "CrossPoint_MAX_X4_v1.5-dev.bin" in wf

def test_search_uses_stock_keyboard():
    cpp = (ROOT / ".builder/overlay/src/max/MaxLibraryActivity.cpp").read_text(encoding="utf-8")
    assert "KeyboardEntryActivity" in cpp
    assert "Szukaj: tytul/autor/seria/kolekcja" in cpp
    assert "asciiFold(book.title)" in cpp
    assert "asciiFold(book.author)" in cpp
    assert "asciiFold(book.series)" in cpp
    assert "asciiFold(book.collection)" in cpp

def test_book_actions_and_manual_metadata():
    base = ROOT / ".builder/overlay/src/max"
    assert (base / "MaxBookActionsActivity.cpp").exists()
    store_h = (base / "MaxLibraryStore.h").read_text(encoding="utf-8")
    store_cpp = (base / "MaxLibraryStore.cpp").read_text(encoding="utf-8")
    for token in ("seriesManual", "collectionManual", "volumeManual"):
        assert token in store_h
    for token in ("setSeries", "setCollection", "setVolume"):
        assert token in store_cpp

def test_max_settings_uses_stock_keyboard_and_config():
    cpp = (ROOT / ".builder/overlay/src/max/MaxSettingsActivity.cpp").read_text(encoding="utf-8")
    assert "KeyboardEntryActivity" in cpp
    assert "MaxTranslationConfig::save" in cpp
    assert "Translation Gateway" in cpp
    assert "Przebuduj MAX Library" in cpp

def test_home_has_library_and_settings_integration():
    patch = (ROOT / ".builder/tools/patch_crosspoint.py").read_text(encoding="utf-8")
    assert "MAX_LIBRARY" in patch
    assert "MAX_SETTINGS" in patch
    assert "goToMaxLibrary" in patch
    assert "goToMaxSettings" in patch
    assert '\\"MAX Library\\"' in patch
    assert '\\"MAX Settings\\"' in patch

def test_previous_translation_stages_present():
    base = ROOT / ".builder/overlay/src/max"
    for name in (
        "MaxTranslateActivity.cpp",
        "MaxChapterTranslationActivity.cpp",
        "MaxWholeBookTranslationActivity.cpp",
        "MaxTranslationCache.cpp",
    ):
        assert (base / name).exists()

def test_ci_application_partition_guard_still_present():
    build = (ROOT / ".builder/build_ci.py").read_text(encoding="utf-8")
    assert "Application partition size UNKNOWN" in build
    assert 'info["size"] > int(app_partition["size"])' in build
