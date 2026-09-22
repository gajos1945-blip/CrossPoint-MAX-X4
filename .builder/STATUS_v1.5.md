# CrossPoint MAX v1.5-dev

Integration stage before release candidate.

Implemented:
- Page / Chapter / Whole Book translation.
- Persistent translation checkpoints.
- SD-first translation cache.
- MAX Library.
- Free-text library search using the stock CrossPoint KeyboardEntryActivity.
- Book metadata action screen (favorite/status/series/collection/volume).
- Explicit manual-override flags in index.jsonl.
- MAX Settings on Home.
- On-device Translation Gateway/source/target editing.
- Study Mode/cache toggles.
- Library rebuild.

Verified design constraints:
- No API keys embedded in firmware.
- No erase command.
- No flash command.
- No merged/full-flash image.
- CI still blocks output outside the confirmed application partition.
- Search is an in-memory filter over the bounded MAX Library index.
- Manual metadata edits are persisted to microSD.

Still required before release:
- successful GitHub PlatformIO build,
- BIN validation,
- physical X4 smoke test,
- translation gateway connectivity test.
