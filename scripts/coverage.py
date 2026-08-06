#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = []
# ///
"""Run the shared ioplane Clang coverage pipeline for iohttp."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    command = [
        "ioplane-build",
        "coverage",
        "--root",
        str(root),
        "--toolchain",
        "clang",
        "--clang-preset",
        os.environ.get("PRESET", "clang-coverage"),
        "--ignore-regex",
        r"(.*/tests/.*|.*/usr/local/src/unity/.*)",
    ]
    return subprocess.run(command, cwd=root, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
