CROSSPOINT MAX X4 — UPDATE v1.3-dev
===================================

NOWE W v1.3
-----------
- Translate Book / tlumaczenie calej ksiazki,
- spine po spine (kolejne rozdzialy/sekcje EPUB),
- automatyczne dobudowanie brakujacego cache/paginacji sekcji,
- tylko kilka stron layoutu na jeden tick,
- tlumaczenie strona po stronie,
- checkpoint po kazdej stronie,
- BACK = bezpieczne anulowanie z zachowaniem checkpointu,
- ponowne Translate Book = wznowienie,
- blad Wi-Fi/providera nie kasuje postepu; po naprawie polaczenia mozna wznowic,
- strony juz przetlumaczone sa pomijane z cache,
- cache v2 jest zwiazany z tekstem konkretnej wyrenderowanej strony, wiec zmiana
  fontu/marginesow/paginacji nie powinna podmienic tlumaczenia ze starego ukladu.

ZOSTAJE Z v1.2
--------------
- Translate Page,
- Translate Chapter,
- Study Mode: Original / Next,
- cache microSD,
- chapter progress / cancel / checkpoint / resume.

JESZCZE NIE MA
--------------
- Biblioteki MAX: serie, kolekcje, statusy, rozbudowane sortowanie,
- finalnego Home MAX.

JAK ZBUDOWAC
------------
1. Rozpakuj ZIP.
2. Skopiuj CALA zawartosc do Twojego lokalnego folderu repo:
      CrossPoint-MAX-X4
   i potwierdz nadpisanie.
3. GitHub Desktop:
      Summary: CrossPoint MAX v1.3-dev whole book translation
      Commit to main
      Push origin
4. Build uruchomi sie automatycznie.
5. GitHub -> Actions -> BUILD READY BIN FOR X4.
6. Po zielonym Success pobierz artifact:
      CrossPoint_MAX_X4_READY_TO_FLASH

W srodku powinien byc:
      CrossPoint_MAX_X4_v1.3-dev.bin
      CrossPoint_MAX_X4_v1.3-dev.bin.sha256.txt
      build_manifest.json
      source_report.json

Jesli build jest czerwony: NIE FLASHUJ. Przeslij log/screenshot.

UWAGA
-----
Whole-book translation wymaga w config.json:
      "cache": true

v1.3 nadal jest DEV build. Po kompilacji najpierw sprawdzimy BIN i manifest.
