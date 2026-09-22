from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]

def test_manifest_v14():
    m = json.loads((ROOT / ".builder/manifest.json").read_text(encoding="utf-8"))
    assert m["version"] == "1.4-dev"
    assert "MAX Library SD index up to 600 books" in m["implemented"]
    assert m["automatic_flash"] is False

def test_expected_bin_and_auto_push():
    wf = (ROOT / ".github/workflows/build-x4-bin.yml").read_text(encoding="utf-8")
    assert "push:" in wf
    assert "branches: [main]" in wf
    assert "CrossPoint_MAX_X4_v1.4-dev.bin" in wf

def test_library_sources_present():
    base = ROOT / ".builder/overlay/src/max"
    for name in (
        "MaxLibraryStore.h",
        "MaxLibraryStore.cpp",
        "MaxLibraryActivity.h",
        "MaxLibraryActivity.cpp",
    ):
        assert (base / name).exists()

def test_library_is_sd_first_and_bounded():
    h = (ROOT / ".builder/overlay/src/max/MaxLibraryStore.h").read_text(encoding="utf-8")
    cpp = (ROOT / ".builder/overlay/src/max/MaxLibraryStore.cpp").read_text(encoding="utf-8")
    assert "MAX_BOOKS = 600" in h
    assert "index.jsonl" in h
    assert 'Storage.exists("/Books") ? "/Books" : "/"' in cpp
    assert '".crosspoint-max"' in cpp
    assert "epub.load(false, true)" in cpp

def test_library_views_and_controls():
    cpp = (ROOT / ".builder/overlay/src/max/MaxLibraryActivity.cpp").read_text(encoding="utf-8")
    for text in ("Wszystkie", "W trakcie", "Nieprzeczytane",
                 "Przeczytane", "Ulubione", "Autorzy", "Serie", "Kolekcje"):
        assert text in cpp
    assert "toggleFavorite" in cpp
    assert "cycleStatus" in cpp
    assert "cycleView" in cpp

def test_home_and_reader_integration_patcher():
    patch = (ROOT / ".builder/tools/patch_crosspoint.py").read_text(encoding="utf-8")
    assert "HomeMenuItem::MAX_LIBRARY" in patch
    assert "goToMaxLibrary" in patch
    assert "MaxLibraryStore::markOpened" in patch
    assert "MaxLibraryStore::markRead" in patch
    assert 'CrossPoint MAX' in patch

def test_previous_translation_stages_still_present():
    base = ROOT / ".builder/overlay/src/max"
    required = (
        "MaxTranslateActivity.cpp",
        "MaxChapterTranslationActivity.cpp",
        "MaxWholeBookTranslationActivity.cpp",
        "MaxBookState.cpp",
        "MaxChapterState.cpp",
        "MaxTranslationCache.cpp",
    )
    for name in required:
        assert (base / name).exists()

def test_ci_checks_application_partition_size():
    build = (ROOT / ".builder/build_ci.py").read_text(encoding="utf-8")
    assert "Application partition size UNKNOWN" in build
    assert 'info["size"] > int(app_partition["size"])' in build
