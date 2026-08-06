#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = []
# ///
"""Build reproducible iohttp release archives and checksums."""

from __future__ import annotations

import hashlib
import os
import subprocess
import sys
from pathlib import Path


def run(command: list[str], *, root: Path, check: bool = True) -> None:
    subprocess.run(command, cwd=root, check=check)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    dist = root / "dist"
    tag = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("GITHUB_REF_NAME", "dev")
    prefix = f"iohttp-{tag}"
    dist.mkdir(parents=True, exist_ok=True)
    for name in (
        f"{prefix}.tar.gz",
        f"{prefix}.zip",
        f"{prefix}-docs.tar.gz",
        f"{prefix}.sha256",
        "RELEASE_NOTES.md",
    ):
        (dist / name).unlink(missing_ok=True)

    run(
        [
            "git",
            "archive",
            "--format=tar.gz",
            f"--prefix={prefix}/",
            "-o",
            str(dist / f"{prefix}.tar.gz"),
            "HEAD",
        ],
        root=root,
    )
    run(
        [
            "git",
            "archive",
            "--format=zip",
            f"--prefix={prefix}/",
            "-o",
            str(dist / f"{prefix}.zip"),
            "HEAD",
        ],
        root=root,
    )
    run(["cmake", "--preset", "clang-debug"], root=root, check=False)
    run(
        ["cmake", "--build", "--preset", "clang-debug", "--target", "docs"],
        root=root,
        check=False,
    )
    docs = root / "docs" / "api" / "html"
    if docs.is_dir():
        run(
            ["tar", "-C", str(docs), "-czf", str(dist / f"{prefix}-docs.tar.gz"), "."],
            root=root,
        )
    run(
        ["uv", "run", "--script", str(root / "scripts" / "release_notes.py"), tag],
        root=root,
    )
    checksums = []
    for path in sorted(
        path
        for path in dist.iterdir()
        if path.is_file()
        and path.name.startswith(prefix)
        and not path.name.endswith(".sha256")
    ):
        if path.name.endswith(".sha256"):
            continue
        checksums.append(
            f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}"
        )
    (dist / f"{prefix}.sha256").write_text(
        "\n".join(checksums) + "\n", encoding="utf-8"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
