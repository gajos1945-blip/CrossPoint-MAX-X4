@echo off
setlocal
cd /d "%~dp0"
echo.
echo CrossPoint MAX Translation Gateway
echo =================================
echo Provider: %MAX_TRANSLATION_PROVIDER%
if "%MAX_TRANSLATION_PROVIDER%"=="" echo Provider not set - default is libretranslate
echo.
python server.py
pause
