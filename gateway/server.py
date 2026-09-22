from __future__ import annotations

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request

HOST = os.environ.get("MAX_GATEWAY_HOST", "0.0.0.0")
PORT = int(os.environ.get("MAX_GATEWAY_PORT", "8787"))
PROVIDER = os.environ.get("MAX_TRANSLATION_PROVIDER", "libretranslate").strip().lower()
LIBRE_URL = os.environ.get("LIBRETRANSLATE_URL", "http://127.0.0.1:5000").rstrip("/")
LIBRE_API_KEY = os.environ.get("LIBRETRANSLATE_API_KEY", "")
DEEPL_API_KEY = os.environ.get("DEEPL_API_KEY", "")
DEEPL_URL = os.environ.get("DEEPL_API_URL", "https://api-free.deepl.com/v2/translate")
MAX_TEXT_BYTES = 64 * 1024

class GatewayError(RuntimeError):
    pass

def http_json(url: str, payload: dict, *, headers: dict[str, str] | None = None) -> dict:
    body = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": "application/json", **(headers or {})},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=45) as resp:
            raw = resp.read()
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise GatewayError(f"provider HTTP {exc.code}: {detail[:300]}") from exc
    except OSError as exc:
        raise GatewayError(f"provider connection error: {exc}") from exc
    try:
        return json.loads(raw.decode("utf-8"))
    except Exception as exc:
        raise GatewayError("provider returned invalid JSON") from exc

def translate_libre(source: str, target: str, text: str) -> str:
    payload = {"q": text, "source": source or "auto", "target": target, "format": "text"}
    if LIBRE_API_KEY:
        payload["api_key"] = LIBRE_API_KEY
    data = http_json(f"{LIBRE_URL}/translate", payload)
    value = data.get("translatedText")
    if not isinstance(value, str) or not value:
        raise GatewayError("LibreTranslate response has no translatedText")
    return value

def translate_deepl(source: str, target: str, text: str) -> str:
    if not DEEPL_API_KEY:
        raise GatewayError("DEEPL_API_KEY is not configured")
    form: dict[str, str] = {
        "text": text,
        "target_lang": target.upper(),
    }
    if source and source.lower() != "auto":
        form["source_lang"] = source.upper()
    body = urllib.parse.urlencode(form).encode("utf-8")
    req = urllib.request.Request(
        DEEPL_URL,
        data=body,
        headers={
            "Authorization": f"DeepL-Auth-Key {DEEPL_API_KEY}",
            "Content-Type": "application/x-www-form-urlencoded",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=45) as resp:
            data = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")
        raise GatewayError(f"DeepL HTTP {exc.code}: {detail[:300]}") from exc
    except OSError as exc:
        raise GatewayError(f"DeepL connection error: {exc}") from exc
    try:
        value = data["translations"][0]["text"]
    except Exception as exc:
        raise GatewayError("DeepL response has no translation") from exc
    if not isinstance(value, str) or not value:
        raise GatewayError("DeepL returned empty translation")
    return value

def translate_argos(source: str, target: str, text: str) -> str:
    if not source or source.lower() == "auto":
        raise GatewayError("Argos provider requires an explicit source language")
    try:
        import argostranslate.translate  # type: ignore
    except ImportError as exc:
        raise GatewayError("Argos Translate is not installed") from exc

    languages = argostranslate.translate.get_installed_languages()
    src = next((x for x in languages if x.code.lower() == source.lower()), None)
    dst = next((x for x in languages if x.code.lower() == target.lower()), None)
    if src is None or dst is None:
        raise GatewayError("required Argos language/model is not installed")
    translator = src.get_translation(dst)
    if translator is None:
        raise GatewayError("required Argos translation model is not installed")
    value = translator.translate(text)
    if not value:
        raise GatewayError("Argos returned empty translation")
    return value

def translate(source: str, target: str, text: str) -> str:
    if PROVIDER == "libretranslate":
        return translate_libre(source, target, text)
    if PROVIDER == "deepl":
        return translate_deepl(source, target, text)
    if PROVIDER == "argos":
        return translate_argos(source, target, text)
    raise GatewayError(f"unknown provider: {PROVIDER}")

class Handler(BaseHTTPRequestHandler):
    server_version = "CrossPointMAXGateway/1.0"

    def log_message(self, fmt: str, *args) -> None:
        sys.stdout.write("%s - %s\n" % (self.address_string(), fmt % args))

    def send_json(self, status: int, payload: dict) -> None:
        raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self) -> None:
        if self.path == "/health":
            self.send_json(200, {"ok": True, "provider": PROVIDER})
            return
        self.send_json(404, {"error": "not found"})

    def do_POST(self) -> None:
        if self.path != "/translate":
            self.send_json(404, {"error": "not found"})
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.send_json(400, {"error": "invalid Content-Length"})
            return
        if length <= 0 or length > MAX_TEXT_BYTES:
            self.send_json(413, {"error": "payload too large or empty"})
            return

        try:
            body = self.rfile.read(length)
            data = json.loads(body.decode("utf-8"))
            text = data.get("text")
            source = data.get("source", "auto")
            target = data.get("target")
            if not isinstance(text, str) or not text.strip():
                raise GatewayError("text is required")
            if not isinstance(source, str) or len(source) > 16:
                raise GatewayError("invalid source language")
            if not isinstance(target, str) or not target or len(target) > 16:
                raise GatewayError("invalid target language")
            result = translate(source, target, text)
        except GatewayError as exc:
            self.send_json(502, {"error": str(exc)})
            return
        except Exception:
            self.send_json(400, {"error": "invalid request"})
            return

        self.send_json(200, {"translation": result})

def main() -> None:
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"CrossPoint MAX Translation Gateway listening on http://{HOST}:{PORT}")
    print(f"Provider: {PROVIDER}")
    server.serve_forever()

if __name__ == "__main__":
    main()
