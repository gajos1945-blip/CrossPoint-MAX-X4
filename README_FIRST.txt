CROSSPOINT MAX X4 — v1.5-dev FIX1
=================================

Ta paczka naprawia pierwszy blad kompilacji v1.5-dev.

GitHub Actions zatrzymal sie w MaxLibraryActivity.cpp przy:
  listIconFor(UITheme::getFileIcon(...), 32)

Przyczyna:
  listIconFor() jest deklarowane w components/UiAppHelpers.h,
  a v1.5-dev uzywalo tej funkcji bez dolaczenia tego naglowka.

FIX1 dodaje brakujacy include. Nie zmienia logiki flash ani partycji.


TEN ETAP DOMYKA GLOWNE FUNKCJE MAX PRZED RELEASE CANDIDATE.

NOWE W v1.5
-----------
MAX LIBRARY:
- jawny wiersz "Szukaj" na gorze biblioteki,
- klawiatura CrossPoint do wpisywania zapytania,
- szukanie po:
    tytule,
    autorze,
    serii,
    kolekcji,
    sciezce pliku,
- jawny wiersz "Widok" do zmiany:
    Wszystkie / W trakcie / Nieprzeczytane / Przeczytane /
    Ulubione / Autorzy / Serie / Kolekcje,
- przytrzymanie Confirm na ksiazce otwiera "MAX Book":
    Ulubione,
    Status,
    Seria,
    Kolekcja,
    Tom,
- recznie wpisane Serie/Kolekcje/Tom przetrwaja przebudowe indeksu,
  rowniez gdy Seria/Kolekcja zostanie celowo wyczyszczona.

MAX SETTINGS:
- nowa pozycja "MAX Settings" na Home,
- edycja Translation Gateway na X4,
- edycja jezyka zrodlowego,
- edycja jezyka docelowego,
- wlacz/wylacz Tryb nauki,
- wlacz/wylacz cache tlumaczen,
- przebuduj MAX Library.

ZOSTAJE Z POPRZEDNICH WERSJI
----------------------------
- Translate Page,
- Study Mode,
- Translate Chapter + checkpoint/resume,
- Translate Book + spine-by-spine + checkpoint/resume,
- cache na microSD zwiazany z trescia strony i layoutem,
- MAX Library,
- statusy czytania,
- ulubione,
- Home MAX.

STEROWANIE BIBLIOTEKI v1.5
--------------------------
Confirm na "Szukaj":
  otwiera klawiature

Confirm na "Widok":
  zmienia widok

Confirm na ksiazce:
  otwiera ksiazke

Przytrzymaj Confirm ok. 0.9 s na ksiazce:
  otwiera MAX Book / akcje i metadane

Back, gdy wyszukiwanie jest aktywne:
  czysci wyszukiwanie

Back bez wyszukiwania:
  wraca do Home

JAK ZBUDOWAC
------------
1. Rozpakuj ZIP.
2. Skopiuj CALA zawartosc do lokalnego repo:
      CrossPoint-MAX-X4
   i potwierdz nadpisanie.
3. GitHub Desktop:
      Summary: CrossPoint MAX v1.5-dev integration
      Commit to main
      Push origin
4. Build wystartuje automatycznie.
5. GitHub -> Actions -> BUILD READY BIN FOR X4.
6. Po zielonym Success pobierz:
      CrossPoint_MAX_X4_READY_TO_FLASH

W artifact powinny byc:
  CrossPoint_MAX_X4_v1.5-dev.bin
  CrossPoint_MAX_X4_v1.5-dev.bin.sha256.txt
  build_manifest.json
  source_report.json

Jezeli build jest czerwony:
  NIE FLASHUJ.
  Przeslij screenshot/log czerwonego kroku.

Po prawidlowym v1.5 bedziemy mieli baze do RELEASE CANDIDATE,
ale RC dopiero po analizie BIN i testach na fizycznym X4.