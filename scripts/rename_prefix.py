#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = []
# ///
"""Plan or apply the io_ to ioh_ project-prefix migration."""

from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path

LOWER_PATTERN = re.compile(rb"\bio_[a-z]")
UPPER_PATTERN = re.compile(rb"\bIO_[A-Z]")
IO_URING_PLACEHOLDER = b"__IOHTTP_RENAME_IO_URING__"
PATHSPEC = (
    "*.c",
    "*.h",
    "CMakeLists.txt",
    "cmake/*.cmake",
    "*.md",
    ".claude/skills/*",
    "examples/*",
    "scripts/*",
    ".clang-format",
    ".clang-tidy",
    "*.json",
    "*.yml",
    "*.yaml",
    "Containerfile",
    "deploy/*",
    ".gitignore",
    ".env.example",
)


def git(root: Path, *arguments: str, check: bool = True) -> str:
    result = subprocess.run(
        ["git", *arguments],
        cwd=root,
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return result.stdout.strip()


def tracked_paths(root: Path) -> list[Path]:
    output = git(root, "ls-files", "--", *PATHSPEC)
    paths = {Path(line) for line in output.splitlines() if line}
    if (root / "CMakePresets.json").is_file():
        paths.add(Path("CMakePresets.json"))
    return sorted(paths)


def rename_paths(root: Path) -> list[tuple[Path, Path]]:
    candidates = [
        path
        for path in (root / "src").rglob("*")
        if path.is_file()
        and path.suffix in {".c", ".h"}
        and path.name.startswith("io_")
    ]
    candidates.extend(
        path for path in (root / "tests" / "unit").glob("test_io_*.c") if path.is_file()
    )
    return [
        (path, path.with_name(path.name.replace("io_", "ioh_", 1)))
        for path in sorted(candidates)
    ]


def replace_prefix(data: bytes) -> bytes:
    protected = data.replace(b"io_uring", IO_URING_PLACEHOLDER)
    protected = re.sub(rb"\bio_", b"ioh_", protected)
    protected = re.sub(rb"\bIO_([A-Z])", rb"IOH_\1", protected)
    return protected.replace(IO_URING_PLACEHOLDER, b"io_uring")


def verification_paths(root: Path) -> list[Path]:
    paths = [
        path
        for path in (root / "src").rglob("*")
        if path.is_file() and path.suffix in {".c", ".h"}
    ]
    paths.extend(
        path
        for path in (root / "tests" / "unit").rglob("*")
        if path.is_file() and path.suffix == ".c"
    )
    paths.append(root / "CMakeLists.txt")
    return paths


def verify(root: Path) -> None:
    lower_misses: list[str] = []
    upper_misses: list[str] = []
    io_uring_count = 0
    for path in verification_paths(root):
        if path.is_dir() or not path.is_file():
            continue
        data = path.read_bytes()
        relative = str(path.relative_to(root))
        if "picohttpparser" not in relative:
            if LOWER_PATTERN.search(data.replace(b"io_uring", b"")):
                lower_misses.append(relative)
            if UPPER_PATTERN.search(data):
                upper_misses.append(relative)
        io_uring_count += data.count(b"io_uring")
    if lower_misses:
        print("WARNING: possible missed lowercase references:")
        print("\n".join(f"  {path}" for path in lower_misses[:30]))
    else:
        print("OK: no missed lowercase references in source")
    if upper_misses:
        print("WARNING: possible missed uppercase references:")
        print("\n".join(f"  {path}" for path in upper_misses[:30]))
    else:
        print("OK: no missed uppercase references in source")
    print(f"io_uring references in source: {io_uring_count}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply",
        action="store_true",
        help="apply git moves and content changes; requires a clean worktree",
    )
    return parser.parse_args()


def main() -> int:
    root = Path(git(Path.cwd(), "rev-parse", "--show-toplevel"))
    moves = rename_paths(root)
    files = tracked_paths(root)
    print(f"Repository: {root}")
    print(f"Files to rename: {len(moves)}")
    for source, destination in moves:
        print(f"  {source.relative_to(root)} -> {destination.relative_to(root)}")
    print(f"Files to scan: {len(files)}")
    if not parse_args().apply:
        print("Dry run only. Re-run with --apply on a clean worktree to mutate files.")
        return 0
    if git(root, "status", "--porcelain"):
        raise SystemExit("refusing --apply: working tree is not clean")
    for source, destination in moves:
        git(
            root,
            "mv",
            str(source.relative_to(root)),
            str(destination.relative_to(root)),
        )
    for relative in files:
        path = root / relative
        if not path.is_file() or path.as_posix().endswith("src/http/picohttpparser.c"):
            continue
        original = path.read_bytes()
        updated = replace_prefix(original)
        if updated != original:
            path.write_bytes(updated)
    verify(root)
    print("Changes are staged by git mv and unstaged for content review.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
