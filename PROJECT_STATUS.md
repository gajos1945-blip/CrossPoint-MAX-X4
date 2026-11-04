# PROJECT_STATUS — CrossPoint MAX X4

Ostatnia aktualizacja: 2026-11-04

## Stan
- Wersja: `1.0.0-rc1`, status `RELEASE_CANDIDATE_HARDWARE_UNVERIFIED`. Zakres software zamknięty (`FINAL_SCOPE.md`).
- Funkcje: tłumaczenie strony / rozdziału / całej książki z checkpointami i resume, Study Mode, cache na microSD,
  MAX Library (indeks, wyszukiwanie, statusy, ulubione, serie, kolekcje, ręczne metadane), MAX Settings, Translation Gateway.
- Testy (2026-11-04): aktualne 12 passed; 9 failed w `test_v12/13/14/15_package.py`.

## Decyzje
- Overlay + patcher zamiast forka CrossPoint.
- Fail-closed CI z weryfikacją partycji, obrazu ESP i markerów funkcji w BIN.
- Gateway HTTP w LAN; klucze providerów wyłącznie po stronie komputera.

## Otwarte
- Testy v12–v15 asercją sprawdzają stare wersje manifestu/BIN — do usunięcia lub przepisania na rc1.
- CI nie uruchamia pytest (w przeciwieństwie do X4 MultiHub) i brak `pytest.ini`.
- Fizyczna walidacja na X4 (boot, e-ink/przyciski, Wi-Fi, gateway, długie tłumaczenie, resume po zaniku zasilania) — NOT RUN.
- Builder i gateway w dużej części zdublowane z `X4-MultiHub-X4`.

## Następne kroki
1. Usunąć/zaktualizować nieaktualne testy v12–v15, dodać `pytest.ini` i krok `pytest` w CI.
2. Test na fizycznym X4.
3. Rozważyć wspólny builder/gateway z X4 MultiHub.
