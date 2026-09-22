from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]

def test_manifest_v13():
    m = json.loads((ROOT / ".builder/manifest.json").read_text(encoding="utf-8"))
    assert m["version"] == "1.3-dev"
    assert "whole-book translation spine-by-spine" in m["implemented"]
    assert m["automatic_flash"] is False

def test_expected_bin_and_auto_push():
    workflow = (ROOT / ".github/workflows/build-x4-bin.yml").read_text(encoding="utf-8")
    assert "push:" in workflow
    assert "branches: [main]" in workflow
    assert "CrossPoint_MAX_X4_v1.3-dev.bin" in workflow

def test_whole_book_sources_present():
    base = ROOT / ".builder/overlay/src/max"
    for name in (
        "MaxBookState.h",
        "MaxBookState.cpp",
        "MaxWholeBookTranslationActivity.h",
        "MaxWholeBookTranslationActivity.cpp",
        "MaxRenderFingerprint.h",
        "MaxRenderFingerprint.cpp",
    ):
        assert (base / name).exists()

def test_cache_is_content_aware():
    h = (ROOT / ".builder/overlay/src/max/MaxTranslationCache.h").read_text(encoding="utf-8")
    cpp = (ROOT / ".builder/overlay/src/max/MaxTranslationCache.cpp").read_text(encoding="utf-8")
    assert "sourceFingerprint" in h
    assert "sourceText" in h
    assert "sourceFingerprint(sourceText)" in cpp

def test_checkpoint_is_layout_bound():
    chapter = (ROOT / ".builder/overlay/src/max/MaxChapterState.cpp").read_text(encoding="utf-8")
    book = (ROOT / ".builder/overlay/src/max/MaxBookState.cpp").read_text(encoding="utf-8")
    whole = (ROOT / ".builder/overlay/src/max/MaxWholeBookTranslationActivity.cpp").read_text(encoding="utf-8")
    assert 'doc["layout_key"]' in chapter
    assert 'doc["layout_key"]' in book
    assert "saved.layoutKey == layoutKey" in whole

def test_whole_book_has_resume_and_incremental_build():
    cpp = (ROOT / ".builder/overlay/src/max/MaxWholeBookTranslationActivity.cpp").read_text(encoding="utf-8")
    assert "MaxBookState::load" in cpp
    assert "buildSomeMore(BUILD_PAGES_PER_TICK)" in cpp
    assert 'persistStatus("cancelled")' in cpp
    assert "checkpoint.nextSpine" in cpp
    assert "checkpoint.nextPage" in cpp

def test_ci_checks_application_partition_size():
    build = (ROOT / ".builder/build_ci.py").read_text(encoding="utf-8")
    assert "Application partition size UNKNOWN" in build
    assert 'info["size"] > int(app_partition["size"])' in build
