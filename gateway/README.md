# CrossPoint MAX Translation Gateway

The X4 firmware deliberately stores no translation-service API keys. It sends
only `source`, `target` and page text to a gateway on your LAN:

`POST /translate` -> `{"translation":"..."}`

Default port: `8787`.

## LibreTranslate

Run your LibreTranslate server separately, then on Windows:

```bat
set MAX_TRANSLATION_PROVIDER=libretranslate
set LIBRETRANSLATE_URL=http://127.0.0.1:5000
set LIBRETRANSLATE_API_KEY=
START_GATEWAY_WINDOWS.cmd
```

## DeepL

```bat
set MAX_TRANSLATION_PROVIDER=deepl
set DEEPL_API_KEY=YOUR_KEY
START_GATEWAY_WINDOWS.cmd
```

The key stays on the PC running the gateway, not in X4 firmware.

## Argos Translate

Install the Python package and the required language model first:

```bat
python -m pip install argostranslate
set MAX_TRANSLATION_PROVIDER=argos
START_GATEWAY_WINDOWS.cmd
```

Argos mode requires an explicit source language in MAX Settings; `auto` is not
guessed.

## X4 configuration

Find the LAN IPv4 address of the gateway PC, for example `192.168.1.20`, then
on X4 open **MAX Settings -> Translation Gateway** and enter:

`http://192.168.1.20:8787`

The X4 and gateway computer must be reachable on the same network.

Health check from another computer/browser:

`http://PC_IP:8787/health`
