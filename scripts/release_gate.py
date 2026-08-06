#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = []
# ///
"""Run the iohttp release quality gate with explicit subprocess arguments."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    commands = (
        [sys.executable, str(root / "scripts" / "quality.py")],
        ["cmake", "--build", "--preset", "clang-debug", "--target", "docs"],
        [sys.executable, str(root / "scripts" / "lint-docs.py")],
    )
    for command in commands:
        subprocess.run(command, cwd=root, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
