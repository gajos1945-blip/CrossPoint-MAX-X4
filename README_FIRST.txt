CROSSPOINT MAX X4 — FINAL SOFTWARE PACKAGE v1.0.0-rc1
=====================================================

STATUS
------
Kod i zakres funkcjonalny projektu sa zakonczone po stronie software.

Ta wersja jest oznaczona:
  RELEASE_CANDIDATE_HARDWARE_UNVERIFIED

Nie oznaczam jej jako fizycznie zweryfikowanej wersji produkcyjnej, poniewaz
XTEINK X4 nie jest obecnie dostepny do testu uruchomienia. To jest jedyna
pozostala bramka przed nazwaniem tego samego kodu wersja v1.0.0.

FUNKCJE
-------
- CrossPoint 1.6.0 jako przypieta, stabilna baza.
- Home z MAX Library i MAX Settings.
- Biblioteka:
  Wszystkie / W trakcie / Nieprzeczytane / Przeczytane /
  Ulubione / Autorzy / Serie / Kolekcje.
- Szukanie po tytule, autorze, serii, kolekcji i sciezce.
- Edycja Ulubione / Status / Seria / Kolekcja / Tom.
- Translate Page.
- Translate Chapter z progress/cancel/checkpoint/resume.
- Translate Book spine-po-spine z progress/cancel/checkpoint/resume.
- Tryb nauki.
- Cache tlumaczen na microSD.
- MAX Settings:
  gateway, source language, target language, Study Mode, cache, rebuild Library.
- Translation Gateway dla LibreTranslate / DeepL / opcjonalnego Argos.
- Stockowe funkcje CrossPoint pozostaja baza: reader, Wi-Fi, file manager,
  recent books, ustawienia czytania itd.

BEZPIECZENSTWO BUILDU
---------------------
GitHub Actions publikuje BIN tylko gdy:
- flash size jest potwierdzony z przypietego projektu,
- application offset jest potwierdzony,
- tabela partycji zostala odczytana,
- firmware jest obrazem ESP,
- firmware miesci sie w calej pamieci flash,
- firmware miesci sie w potwierdzonej partycji aplikacji,
- wymagane markery funkcji MAX sa faktycznie obecne w gotowym BIN.

Workflow NIE wykonuje erase_flash i NIE flashuje urzadzenia.

JAK ZBUDOWAC RC1
----------------
1. Rozpakuj ten ZIP.
2. Skopiuj CALA zawartosc do Twojego lokalnego repo:
      CrossPoint-MAX-X4
3. Potwierdz nadpisanie.
4. GitHub Desktop:
      Summary: CrossPoint MAX v1.0.0-rc1 final software package
      Commit to main
      Push origin
5. Build uruchomi sie automatycznie.
6. GitHub -> Actions -> BUILD READY BIN FOR X4.
7. Po Success pobierz artifact:
      CrossPoint_MAX_X4_v1_0_0_RC1_READY_TO_FLASH

W artifact:
  CrossPoint_MAX_X4_v1.0.0-rc1.bin
  CrossPoint_MAX_X4_v1.0.0-rc1.bin.sha256.txt
  build_manifest.json
  source_report.json

TRANSLATION GATEWAY
-------------------
Folder:
  gateway/

Nie zapisuj klucza DeepL/API w firmware ani na publicznym GitHubie.
Klucz ustawiaj tylko jako zmienna srodowiskowa na komputerze uruchamiajacym
gateway.

UWAGA O "FINAL"
---------------
Bez fizycznego X4 mozemy zakonczyc implementacje, kompilacje, statyczne testy
i kontrole obrazu BIN. Nie mozemy uczciwie potwierdzic bootu, przyciskow,
e-ink refresh, Wi-Fi ani dlugiego tlumaczenia na konkretnym egzemplarzu.
Dlatego artefakt jest RC1, a nie falszywie oznaczonym "hardware verified".
