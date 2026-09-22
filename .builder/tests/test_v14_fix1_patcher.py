from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def test_home_menu_patch_uses_pinned_unique_lines():
    patch = (ROOT / ".builder/tools/patch_crosspoint.py").read_text(encoding="utf-8")
    assert 'std::vector<const char*> menuItems = {tr(STR_BROWSE_FILES), tr(STR_MENU_RECENT_BOOKS), tr(STR_FILE_TRANSFER),' in patch
    assert 'std::vector<UIIcon> menuIcons = {Folder, Recent, Transfer, Settings};' in patch
    assert 'Home MAX Library menuItems line' in patch
    assert 'Home MAX Library menuIcons line' in patch

def test_old_whitespace_sensitive_block_is_gone():
    patch = (ROOT / ".builder/tools/patch_crosspoint.py").read_text(encoding="utf-8")
    assert 'Home MAX Library render row' not in patch
