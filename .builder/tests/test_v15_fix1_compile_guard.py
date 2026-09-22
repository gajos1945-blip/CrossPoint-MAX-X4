from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def test_list_icon_helper_header_is_included():
    cpp = (ROOT / ".builder/overlay/src/max/MaxLibraryActivity.cpp").read_text(encoding="utf-8")
    assert '#include "components/UiAppHelpers.h"' in cpp
    assert "listIconFor(UITheme::getFileIcon" in cpp

def test_keyboard_api_matches_pinned_crosspoint_usage():
    settings = (ROOT / ".builder/overlay/src/max/MaxSettingsActivity.cpp").read_text(encoding="utf-8")
    actions = (ROOT / ".builder/overlay/src/max/MaxBookActionsActivity.cpp").read_text(encoding="utf-8")
    library = (ROOT / ".builder/overlay/src/max/MaxLibraryActivity.cpp").read_text(encoding="utf-8")
    assert "InputType::Url" in settings
    assert "KeyboardResult" in settings
    assert "KeyboardResult" in actions
    assert "KeyboardResult" in library
