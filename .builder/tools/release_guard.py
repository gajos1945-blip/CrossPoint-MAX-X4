from __future__ import annotations
from pathlib import Path

REQUIRED_MARKERS = (
    b"MAX Library",
    b"MAX Settings",
    b"MAX Book",
    b"Szukaj",
    b"Translation Gateway",
    b"Translate Page",
    b"Translate Chapter",
    b"Translate Book",
    b"CrossPoint MAX - Cala ksiazka",
    b"/.crosspoint-max/library/index.jsonl",
    b"Seria",
    b"Kolekcja",
    b"Tom (0-9999)",
)

FORBIDDEN_MARKERS = (
    b"NOT IMPLEMENTED w v1.1-dev",
    b"v1.1-dev: gateway",
)

def verify_required_markers(path: Path) -> list[str]:
    data = path.read_bytes()
    missing = [m.decode("utf-8", errors="replace") for m in REQUIRED_MARKERS if m not in data]
    forbidden = [m.decode("utf-8", errors="replace") for m in FORBIDDEN_MARKERS if m in data]
    errors: list[str] = []
    if missing:
        errors.append("Missing required firmware markers: " + ", ".join(missing))
    if forbidden:
        errors.append("Forbidden stale development markers found: " + ", ".join(forbidden))
    return errors
