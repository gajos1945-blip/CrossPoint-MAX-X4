from __future__ import annotations
from pathlib import Path
import csv, json, re

class VerificationError(RuntimeError):
    pass

def parse_int(value: str):
    v = value.strip()
    if not v:
        return None
    mult = 1
    if v[-1:].upper() == "K":
        mult, v = 1024, v[:-1]
    elif v[-1:].upper() == "M":
        mult, v = 1024 * 1024, v[:-1]
    return int(v, 0) * mult

def parse_partitions(path: Path):
    rows = []
    for raw in csv.reader(path.read_text(encoding="utf-8-sig").splitlines()):
        if not raw or not raw[0].strip() or raw[0].lstrip().startswith("#"):
            continue
        raw += [""] * (6-len(raw))
        rows.append({
            "name": raw[0].strip(),
            "type": raw[1].strip(),
            "subtype": raw[2].strip(),
            "offset": parse_int(raw[3]),
            "size": parse_int(raw[4]),
            "flags": raw[5].strip(),
        })
    return rows

def verify_no_overlap(parts):
    known = sorted((p for p in parts if p["offset"] is not None and p["size"] is not None),
                   key=lambda p: p["offset"])
    for a, b in zip(known, known[1:]):
        if a["offset"] + a["size"] > b["offset"]:
            raise VerificationError(f"Partition overlap: {a['name']} vs {b['name']}")

def inspect(root: Path):
    ini = root/"platformio.ini"
    pfile = root/"partitions.csv"
    if not ini.is_file() or not pfile.is_file():
        raise VerificationError("platformio.ini or partitions.csv missing")

    text = ini.read_text(encoding="utf-8")
    def setting(name):
        m = re.search(rf"(?m)^\s*{re.escape(name)}\s*=\s*([^\r\n;]+)", text)
        return m.group(1).strip() if m else "UNKNOWN"

    parts = parse_partitions(pfile)
    verify_no_overlap(parts)
    return {
        "board": setting("board"),
        "framework": setting("framework"),
        "flash_size": setting("board_upload.flash_size"),
        "maximum_size": setting("board_upload.maximum_size"),
        "application_offset": setting("board_upload.offset_address"),
        "partition_file": setting("board_build.partitions"),
        "partitions": parts,
        "sources": {
            "flash_size": "platformio.ini" if setting("board_upload.flash_size") != "UNKNOWN" else "UNKNOWN",
            "application_offset": "platformio.ini" if setting("board_upload.offset_address") != "UNKNOWN" else "UNKNOWN",
            "partitions": "partitions.csv",
        }
    }

def main():
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("repo")
    ap.add_argument("--json")
    ns = ap.parse_args()
    report = inspect(Path(ns.repo))
    out = json.dumps(report, indent=2)
    if ns.json:
        Path(ns.json).write_text(out+"\n", encoding="utf-8")
    print(out)

if __name__ == "__main__":
    main()
