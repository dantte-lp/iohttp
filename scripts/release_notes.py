#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14,<3.15"
# dependencies = []
# ///
"""Render deterministic iohttp release notes."""

from __future__ import annotations

import os
import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    tag = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("GITHUB_REF_NAME", "dev")
    dist = root / "dist"
    dist.mkdir(parents=True, exist_ok=True)
    (dist / "RELEASE_NOTES.md").write_text(
        f"""# iohttp {tag}

## Scope

- Embedded HTTP server library for C23 with io_uring and wolfSSL
- HTTP/1.1 + HTTP/2 + HTTP/3 support
- TLS 1.3 via wolfSSL, QUIC via ngtcp2

## Verification

- Release gate: `scripts/release_gate.py`
- Quality pipeline: `python scripts/quality.py`

## Published Assets

- Source tarball
- Source zip archive
- Generated API reference archive (if available)
- SHA256 checksums

## References

- Architecture: `docs/en/01-architecture.md`
- Comparison: `docs/en/04-framework-comparison.md`
""",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
