from pathlib import Path
import json
import importlib.util

ROOT = Path(__file__).resolve().parents[2]

def test_final_manifest():
    data = json.loads((ROOT / ".builder/manifest.json").read_text(encoding="utf-8"))
    assert data["version"] == "1.0.0-rc1"
    assert data["firmware_scope_complete"] is True
    assert data["physical_device_verified"] is False
    assert data["automatic_flash"] is False
    assert data["automatic_erase"] is False

def test_final_workflow_artifact():
    text = (ROOT / ".github/workflows/build-x4-bin.yml").read_text(encoding="utf-8")
    assert "CrossPoint_MAX_X4_v1.0.0-rc1.bin" in text
    assert "CrossPoint_MAX_X4_v1_0_0_RC1_READY_TO_FLASH" in text
    assert "push:" in text

def test_runtime_has_no_stale_v11_messages():
    base = ROOT / ".builder/overlay/src/max"
    texts = "\n".join(
        p.read_text(encoding="utf-8")
        for p in base.glob("*.cpp")
    )
    assert "NOT IMPLEMENTED w v1.1-dev" not in texts
    assert "v1.1-dev: gateway" not in texts

def test_release_guard_markers_cover_final_features():
    guard_path = ROOT / ".builder/tools/release_guard.py"
    spec = importlib.util.spec_from_file_location("release_guard", guard_path)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    expected = {
        b"MAX Library",
        b"MAX Settings",
        b"MAX Book",
        b"Szukaj",
        b"Translate Page",
        b"Translate Chapter",
        b"Translate Book",
    }
    assert expected.issubset(set(mod.REQUIRED_MARKERS))

def test_ci_calls_release_guard_and_partition_guard():
    text = (ROOT / ".builder/build_ci.py").read_text(encoding="utf-8")
    assert "verify_required_markers(fw)" in text
    assert "Application partition size UNKNOWN" in text
    assert 'info["size"] > int(app_partition["size"])' in text
    assert "physical_device_verified" in text

def test_gateway_present_and_no_embedded_api_key():
    server = (ROOT / "gateway/server.py").read_text(encoding="utf-8")
    assert "LibreTranslate" in server
    assert "DeepL" in server
    assert "argostranslate" in server
    assert 'os.environ.get("DEEPL_API_KEY", "")' in server
    assert "YOUR_KEY" not in server

def test_default_sd_config_has_no_invented_gateway_ip():
    cfg = json.loads((ROOT / "sdcard/.crosspoint-max/config.json").read_text(encoding="utf-8"))
    assert cfg["gateway"] == ""
    assert cfg["source"] == "auto"
    assert cfg["target"] == "pl"

def test_core_final_sources_exist():
    base = ROOT / ".builder/overlay/src/max"
    required = (
        "MaxTranslateActivity.cpp",
        "MaxChapterTranslationActivity.cpp",
        "MaxWholeBookTranslationActivity.cpp",
        "MaxLibraryActivity.cpp",
        "MaxLibraryStore.cpp",
        "MaxBookActionsActivity.cpp",
        "MaxSettingsActivity.cpp",
        "MaxTranslationCache.cpp",
    )
    for name in required:
        assert (base / name).exists()
