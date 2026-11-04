# CrossPoint MAX X4 — instrukcje dla Claude

Firmware dla XTEINK X4 (ESP32-C3, e-ink) zbudowany jako **overlay na przypiętym CrossPoint Reader 1.6.0**
(commit `54337e6d73fc628f4ba523ddc89a743ca8c6e4c5`, env `gh_release`): tłumaczenie książek + MAX Library,
plus Pythonowy **Translation Gateway** w LAN (LibreTranslate / DeepL / opcjonalnie Argos).
Status: `1.0.0-rc1`, `RELEASE_CANDIDATE_HARDWARE_UNVERIFIED`. Bieżący stan: `PROJECT_STATUS.md`, zakres: `FINAL_SCOPE.md`.

## Struktura
- `.builder/overlay/src/max/` — cały kod C++ MAX (Translate*/Chapter*/WholeBook* Activity, TranslationCache/Client, Library*, Settings).
- `.builder/tools/patch_crosspoint.py` — wstrzykuje overlay do sklonowanego upstreamu (`replace_once` / `ensure_absent`). Zmiany w upstreamie TYLKO tędy.
- `.builder/build_ci.py` — klon upstreamu → patch → `pio run` → weryfikacja BIN (`verify_project`, `bin_inspector`, `release_guard`).
- `.builder/manifest.json` — wersja, status, `implemented`, `known_constraints`.
- `.builder/tests/` — statyczne testy buildera/paczki (brak `pytest.ini`; CI ich obecnie NIE uruchamia).
- `gateway/server.py` — translation gateway. `sdcard/.crosspoint-max/` — konfiguracja i indeks biblioteki na karcie.

## Komendy
- Aktualne testy: `python -m pytest -q .builder/tests --ignore=.builder/tests/test_v12_package.py --ignore=.builder/tests/test_v13_package.py --ignore=.builder/tests/test_v14_package.py --ignore=.builder/tests/test_v15_package.py`
  (testy v12–v15 są nieaktualne — sprawdzają stare wersje manifestu; patrz `PROJECT_STATUS.md`).
- Build firmware: tylko GitHub Actions (`.github/workflows/build-x4-bin.yml`, push na `main`).
- Gateway: `python gateway/server.py`; klucz DeepL wyłącznie jako zmienna środowiskowa.

## Zasady nienaruszalne
- NIGDY nie wpisuj na sztywno flash size, offsetów ani partycji — CI odczytuje je z upstreamu (fail-closed).
- Workflow nie może wykonywać `erase_flash`, flashowania ani tworzyć merged/full-flash image.
- Żadnych kluczy API w firmware ani w repo (repo jest publiczne).
- Nie modyfikuj formatów stanu CrossPoint (np. `progress.bin`) bez wyraźnej prośby.
- Nie zmieniaj przypiętego commitu CrossPoint bez wyraźnej prośby.
- Nie oznaczaj niczego jako „hardware verified” — fizyczny test X4 nie został wykonany.
- Nowa funkcja = wpis w `manifest.json` + marker wymagany przez `release_guard` + test w `.builder/tests/`.
- Cache tłumaczeń musi pozostać związany z treścią i layoutem strony (`MaxRenderFingerprint`).

## Praca
- Po edycji `.py` hook (`.claude/hooks/post_edit_check.py`) uruchamia ruff (F) i aktualne testy; błąd wraca do Claude.
- Dokumentacja bibliotek (PlatformIO, ESP-IDF/Arduino-ESP32, DeepL/LibreTranslate API): używaj Context7.
- Do nawigacji po C++ preferuj Serena (symbole) zamiast czytania całych plików.
- Po istotnym etapie zaktualizuj `PROJECT_STATUS.md` i `README_FIRST.txt`.
