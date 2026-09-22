from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[2]

def test_manifest_v12():
    data = json.loads((ROOT / ".builder/manifest.json").read_text(encoding="utf-8"))
    assert data["version"] == "1.2-dev"
    assert "chapter translation page-by-page" in data["implemented"]

def test_workflow_auto_push_and_expected_bin():
    text = (ROOT / ".github/workflows/build-x4-bin.yml").read_text(encoding="utf-8")
    assert "push:" in text
    assert "branches: [main]" in text
    assert "CrossPoint_MAX_X4_v1.2-dev.bin" in text

def test_chapter_sources_present():
    base = ROOT / ".builder/overlay/src/max"
    assert (base / "MaxChapterState.cpp").exists()
    assert (base / "MaxChapterTranslationActivity.cpp").exists()
