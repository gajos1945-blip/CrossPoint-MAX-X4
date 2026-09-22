CROSSPOINT MAX X4 — GITHUB ONE-CLICK BUILD
==========================================

CEL
---
Ta paczka ma zrobić jedną rzecz:
GitHub -> Actions -> Run workflow -> pobierz gotowy BIN -> użyj go jako Custom .bin.

NIE musisz instalować PlatformIO na swoim komputerze.

KROKI
-----
1. Rozpakuj ten ZIP.
2. Załóż puste repozytorium na GitHubie.
3. Wgraj CAŁĄ zawartość rozpakowanego folderu do repozytorium.
   Ważne: musi znaleźć się także folder ".github".
4. Otwórz zakładkę:
      Actions
5. Po lewej wybierz:
      BUILD READY BIN FOR X4
6. Kliknij:
      Run workflow
      Run workflow
7. Poczekaj aż zadanie będzie zielone (Success).
8. Na dole strony uruchomionego workflow pojawi się:
      Artifacts
      CrossPoint_MAX_X4_READY_TO_FLASH
9. Pobierz artifact ZIP i rozpakuj.
10. Interesuje Cię:
      CrossPoint_MAX_X4_v1.1-dev.bin

DOPIERO TEN PLIK jest wynikiem realnej kompilacji.

FLASH
-----
Na stronie:
https://crosspointreader.com/#flash-tools

wybierz X4 -> Custom .bin i wskaż:
CrossPoint_MAX_X4_v1.1-dev.bin

WAŻNE
-----
Przed pierwszym flashowaniem zachowaj działający oficjalny CrossPoint/update.bin.

Workflow:
- pobiera CrossPoint 1.6.0 z oficjalnego repo,
- przełącza na dokładny commit 54337e6d73fc628f4ba523ddc89a743ca8c6e4c5,
- pobiera submoduły,
- sprawdza platformio.ini i partitions.csv,
- nakłada MAX,
- buduje oficjalnym środowiskiem gh_release,
- sprawdza, czy wynik jest obrazem ESP,
- sprawdza zakres względem potwierdzonego flash size/offset,
- liczy SHA-256,
- NICZEGO nie flashuje.

Jeśli workflow jest czerwony (Failed):
NIE WGRYWAJ NICZEGO.
Prześlij do ChatGPT screenshot/log z czerwonego kroku "Build CrossPoint MAX X4".

STATUS FUNKCJI v1.1-dev
-----------------------
Jest:
- tłumaczenie aktualnej strony,
- cache microSD,
- Translation Gateway,
- prosty Study Mode: Original / Next.

Jeszcze nie ma:
- pełnej Biblioteki MAX,
- tłumaczenia całego rozdziału,
- tłumaczenia całej książki z resume.

To jest DEV build i wymaga testu na fizycznym X4.
