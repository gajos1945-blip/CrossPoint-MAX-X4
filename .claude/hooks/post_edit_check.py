"""Claude Code PostToolUse hook: fast checks after Claude edits a file.

- *.py  -> ruff (pyflakes rules only, edited file) + full pytest suite (~4 s)
- other -> no-op

Exit code 2 sends the failure output back to Claude so it fixes it right away.
Missing tools (ruff/pytest) are skipped silently, never block work.
"""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

# v12-v15 package tests are stale (assert old manifest versions); see PROJECT_STATUS.md
PYTEST_ARGS = ["-q", "-x", "--no-header", ".builder/tests",
               "--ignore=.builder/tests/test_v12_package.py",
               "--ignore=.builder/tests/test_v13_package.py",
               "--ignore=.builder/tests/test_v14_package.py",
               "--ignore=.builder/tests/test_v15_package.py"]


def main() -> int:
    try:
        payload = json.load(sys.stdin)
    except Exception:
        return 0
    file_path = (payload.get("tool_input") or {}).get("file_path") or ""
    if not file_path.endswith(".py"):
        return 0

    root = Path(os.environ.get("CLAUDE_PROJECT_DIR") or Path(__file__).resolve().parents[2])
    problems: list[str] = []

    if shutil.which("ruff"):
        r = subprocess.run(["ruff", "check", "--select", "F", "--quiet", file_path],
                           cwd=root, capture_output=True, text=True)
        if r.returncode != 0:
            problems.append("ruff:\n" + (r.stdout + r.stderr).strip())

    r = subprocess.run([sys.executable, "-m", "pytest", *PYTEST_ARGS],
                       cwd=root, capture_output=True, text=True)
    if r.returncode not in (0, 5):  # 5 = no tests collected
        tail = "\n".join((r.stdout + r.stderr).strip().splitlines()[-30:])
        problems.append("pytest:\n" + tail)

    if problems:
        print("\n\n".join(problems), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
