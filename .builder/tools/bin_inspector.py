from pathlib import Path
import hashlib, struct

def inspect_bin(path: Path):
    data = path.read_bytes()
    out = {
        "path": str(path),
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "esp_image": False,
        "segments": None,
        "entry_point": None,
    }
    if len(data) >= 24 and data[0] == 0xE9:
        out["esp_image"] = True
        out["segments"] = data[1]
        out["entry_point"] = f"0x{struct.unpack_from('<I', data, 4)[0]:08X}"
    return out
