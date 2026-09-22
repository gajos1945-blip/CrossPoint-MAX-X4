CROSSPOINT MAX X4 — UPDATE v1.4-dev
===================================

NOWE W v1.4
-----------
1. MAX Library na ekranie Home.
2. Indeks biblioteki na microSD:
      /.crosspoint-max/library/index.jsonl
3. Do 600 ksiazek w indeksie tego etapu.
4. Widoki:
      Wszystkie
      W trakcie
      Nieprzeczytane
      Przeczytane
      Ulubione
      Autorzy
      Serie
      Kolekcje
5. Tytul/autor/jezyk sa pobierane z juz istniejacego cache EPUB, kiedy jest dostepny.
   Nie wymuszamy parsowania setek EPUB-ow naraz.
6. Status "W trakcie" zapisuje sie automatycznie po otwarciu EPUB.
7. Status "Przeczytane" zapisuje sie automatycznie po dojsciu do konca EPUB.
8. Home zachowuje standardowe CrossPoint "Continue Reading"/okladki, ale dostaje
   wejscie MAX Library i branding CrossPoint MAX.

STEROWANIE W MAX LIBRARY
------------------------
Krotki Confirm:
  otworz ksiazke

Przytrzymaj Confirm ok. 1 sekundy:
  dodaj/usun Ulubione

Przytrzymaj Confirm ponad 2.2 sekundy:
  recznie przelacz status:
  NEW -> READING -> READ -> NEW

Krotki Back:
  Home

Przytrzymaj Back ok. 1 sekundy:
  nastepny widok biblioteki

Up / Down:
  nawigacja listy

SERIE I KOLEKCJE
----------------
W v1.4-dev uzywamy bezpiecznej, deterministycznej konwencji folderow:

  /Books/<Kolekcja>/<Seria>/<plik.epub>

Przyklad:
  /Books/Fantasy/Wiedzmin/01 Ostatnie zyczenie.epub

Da:
  Kolekcja = Fantasy
  Seria     = Wiedzmin

Jesli plik lezy:
  /Books/Fantasy/Hobbit.epub

Da:
  Kolekcja = Fantasy
  Seria = pusta

Nie zgadujemy serii z tytulu ani metadanych, ktorych pinned CrossPoint 1.6.0
nie udostepnia przez publiczny Epub API.

NIE MA JESZCZE W v1.4
---------------------
- tekstowego wyszukiwania wpisywanego na urzadzeniu,
- edytora nazw serii/kolekcji na X4,
- automatycznego numeru tomu z metadanych EPUB.

Te elementy zostaja do etapu integracyjnego v1.5.

JAK ZBUDOWAC
------------
1. Rozpakuj ZIP.
2. Skopiuj CALA zawartosc do lokalnego repo CrossPoint-MAX-X4 i nadpisz pliki.
3. GitHub Desktop:
      Summary: CrossPoint MAX v1.4-dev MAX Library and Home
      Commit to main
      Push origin
4. Build uruchomi sie automatycznie.
5. GitHub -> Actions -> BUILD READY BIN FOR X4.
6. Po zielonym Success pobierz CrossPoint_MAX_X4_READY_TO_FLASH.

W artifact powinny byc:
  CrossPoint_MAX_X4_v1.4-dev.bin
  CrossPoint_MAX_X4_v1.4-dev.bin.sha256.txt
  build_manifest.json
  source_report.json

Jezeli build jest czerwony: NIE FLASHUJ. Wyslij screenshot/log bledu.
