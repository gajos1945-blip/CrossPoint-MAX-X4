# CrossPoint MAX X4 v1.0.0-rc1

## Completed software scope

### Reading
The firmware stays based on pinned CrossPoint 1.6.0 and therefore preserves
the upstream reader, file browser, recent-book flow, Wi-Fi and reader settings.

### Translation
- Page translation.
- Study Mode.
- Chapter translation.
- Whole-book translation.
- Progress screens.
- Cancel.
- Per-page and per-book persistent checkpoints.
- Resume after cancellation or provider/network failure.
- microSD cache bound to page content/layout.

### MAX Library
- SD-backed bounded index.
- Search.
- Statuses.
- Favorites.
- Authors.
- Series.
- Collections.
- Manual series / collection / volume editing.
- Persistent user metadata overrides.
- Home integration.

### MAX Settings
- Translation gateway address.
- Source language.
- Target language.
- Study Mode toggle.
- Translation cache toggle.
- Library rebuild.

### Translation Gateway
Included Windows/Python gateway supports:
- LibreTranslate,
- DeepL,
- optional local Argos Translate.

No provider secret is compiled into firmware.

## Release gates implemented in CI
- exact CrossPoint upstream commit,
- project flash-size discovery,
- application-offset discovery,
- partition-table parsing,
- ESP image validation,
- flash-range validation,
- application-partition-size validation,
- SHA-256 output,
- required-feature-marker scan.

## Only remaining verification that cannot be done without hardware
- physical X4 boot,
- display/controls smoke test,
- Wi-Fi connectivity on the device,
- translation gateway reachability from the device,
- long-run chapter/book translation test,
- power-cycle resume test on the physical device.

This is why the package is `v1.0.0-rc1`,
`RELEASE_CANDIDATE_HARDWARE_UNVERIFIED`.
