#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = []
# ///
"""Run the iohttp GCC analyzer build lane."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    preset = os.environ.get("PRESET", "gcc-debug")
    subprocess.run(["cmake", "--preset", preset, "--fresh"], cwd=root, check=True)
    subprocess.run(["cmake", "--build", "--preset", preset], cwd=root, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
