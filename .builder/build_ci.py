from __future__ import annotations
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / ".builder" / "tools"
sys.path.insert(0, str(TOOLS))

from patch_crosspoint import apply as apply_patch, PatchError
from verify_project import inspect as inspect_project, VerificationError
from bin_inspector import inspect_bin

UPSTREAM = "https://github.com/crosspoint-reader/crosspoint-reader.git"
COMMIT = "54337e6d73fc628f4ba523ddc89a743ca8c6e4c5"
ENV = "gh_release"
RELEASE_NAME = "CrossPoint_MAX_X4_v1.4-dev.bin"

class BuildError(RuntimeError):
    pass

def run(args: list[str], cwd: Path | None = None) -> str:
    p = subprocess.run(
        args,
        cwd=str(cwd) if cwd else None,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        shell=False,
    )
    print(p.stdout, end="")
    if p.returncode != 0:
        raise BuildError(f"Command failed ({p.returncode}): {args!r}")
    return p.stdout

def require(name: str) -> str:
    p = shutil.which(name)
    if not p:
        raise BuildError(f"Required tool not found: {name}")
    return p

def parse_flash_bytes(value: str) -> int:
    s = value.strip().upper()
    for suffix, mult in (("MB", 1024*1024), ("KB", 1024), ("M", 1024*1024), ("K", 1024)):
        if s.endswith(suffix):
            return int(s[:-len(suffix)]) * mult
    return int(s, 0)

def main() -> int:
    git = require("git")
    pio = require("pio")
    work = ROOT / "_work" / "crosspoint-reader"
    dist = ROOT / "dist"
    if work.exists():
        shutil.rmtree(work)
    if dist.exists():
        shutil.rmtree(dist)
    work.parent.mkdir(parents=True, exist_ok=True)
    dist.mkdir(parents=True, exist_ok=True)

    print("=== 1/7 Clone exact CrossPoint 1.6.0 ===")
    run([git, "clone", "--recursive", UPSTREAM, str(work)])
    run([git, "checkout", "--detach", COMMIT], cwd=work)
    run([git, "submodule", "sync", "--recursive"], cwd=work)
    run([git, "submodule", "update", "--init", "--recursive"], cwd=work)
    actual = run([git, "rev-parse", "HEAD"], cwd=work).strip()
    if actual.lower() != COMMIT.lower():
        raise BuildError(f"Wrong commit: {actual}")

    print("=== 2/7 Verify project facts ===")
    report = inspect_project(work)
    if report["flash_size"] == "UNKNOWN":
        raise BuildError("Flash size UNKNOWN")
    if report["application_offset"] == "UNKNOWN":
        raise BuildError("Application offset UNKNOWN")
    (dist / "source_report.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )

    print("=== 3/7 Apply CrossPoint MAX layer ===")
    apply_patch(work, ROOT / ".builder" / "overlay")

    print("=== 4/7 Build official X4/X3 C3 release environment ===")
    started = time.time()
    run([pio, "run", "-e", ENV, "-j1"], cwd=work)
    elapsed = round(time.time() - started, 3)

    print("=== 5/7 Validate application image ===")
    fw = work / ".pio" / "build" / ENV / "firmware.bin"
    if not fw.is_file():
        raise BuildError("Build succeeded but firmware.bin is missing")
    info = inspect_bin(fw)
    if not info["esp_image"]:
        raise BuildError("Output is not an ESP application image")

    flash_bytes = parse_flash_bytes(report["flash_size"])
    app_offset = int(report["application_offset"], 0)
    if app_offset + info["size"] > flash_bytes:
        raise BuildError("Application image exceeds confirmed flash range")

    app_partition = None
    for part in report.get("partitions", []):
        if part.get("type") == "app" and part.get("offset") == app_offset:
            app_partition = part
            break
    if not app_partition or app_partition.get("size") is None:
        raise BuildError("Application partition size UNKNOWN; release blocked")
    if info["size"] > int(app_partition["size"]):
        raise BuildError(
            f"Application image ({info['size']}) exceeds confirmed "
            f"{app_partition['name']} partition ({app_partition['size']})"
        )

    print("=== 6/7 Create release files ===")
    final_bin = dist / RELEASE_NAME
    shutil.copy2(fw, final_bin)
    sha = hashlib.sha256(final_bin.read_bytes()).hexdigest()
    (dist / (RELEASE_NAME + ".sha256.txt")).write_text(
        f"{sha}  {RELEASE_NAME}\n", encoding="ascii"
    )

    manifest = {
        "project": "CrossPoint MAX X4",
        "version": "1.4-dev",
        "base_release": "CrossPoint 1.6.0",
        "upstream_commit": COMMIT,
        "platformio_environment": ENV,
        "target_family": "ESP32-C3 / X4-X3 release environment",
        "build_status": "SUCCESS",
        "build_seconds": elapsed,
        "artifact": {**info, "filename": RELEASE_NAME, "sha256": sha},
        "source_facts": report,
        "application_partition": app_partition,
        "physical_device_verified": False,
        "flash_performed": False,
        "merged_full_flash": False,
    }
    (dist / "build_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )

    print("=== 7/7 DONE ===")
    print("BIN:", final_bin)
    print("SHA-256:", sha)
    print("This workflow does NOT flash or erase any device.")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (BuildError, PatchError, VerificationError, ValueError) as exc:
        print("BUILD BLOCKED:", exc, file=sys.stderr)
        raise SystemExit(2)
