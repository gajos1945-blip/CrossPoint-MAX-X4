CROSSPOINT MAX X4 — UPDATE v1.2-dev
===================================

TEN ETAP DODAJE
---------------
- Translate Page (z v1.1),
- Translate Chapter,
- tłumaczenie rozdziału strona po stronie,
- ekran postępu,
- anulowanie przyciskiem BACK,
- checkpoint na microSD po każdej stronie,
- wznowienie po ponownym wejściu w Translate Chapter,
- pomijanie stron już obecnych w cache.

WAŻNE
-----
Translate Chapter startuje dopiero po zakończeniu indeksowania/paginacji bieżącego
rozdziału. Jeżeli CrossPoint nadal buduje rozdział częściowo, MAX pokaże komunikat
i nie będzie zgadywał brakujących stron.

NIE MA JESZCZE
--------------
- tłumaczenia całej książki,
- pełnej Biblioteki MAX,
- finalnego Home MAX.

JAK WGRAC v1.2 DO TEGO SAMEGO REPO
----------------------------------
1. Rozpakuj ten ZIP.
2. Otwórz lokalny folder CrossPoint-MAX-X4, który masz już w GitHub Desktop.
3. Skopiuj CAŁĄ zawartość tej paczki do repo i potwierdź nadpisanie.
4. Wróć do GitHub Desktop.
5. W Summary wpisz:
      CrossPoint MAX v1.2-dev chapter translation
6. Kliknij Commit to main.
7. Kliknij Push origin.

OD v1.2 BUILD STARTUJE AUTOMATYCZNIE PO PUSH.
Ręczny Run workflow nadal zostaje jako opcja zapasowa.

Po Push:
GitHub -> Actions -> BUILD READY BIN FOR X4

Po zielonym Success pobierz:
CrossPoint_MAX_X4_READY_TO_FLASH

W środku ma być:
CrossPoint_MAX_X4_v1.2-dev.bin
CrossPoint_MAX_X4_v1.2-dev.bin.sha256.txt
build_manifest.json
source_report.json

Jeśli workflow jest czerwony, NIE FLASHUJ. Podeślij screenshot/log czerwonego kroku.
