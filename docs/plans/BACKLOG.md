# iohttp Backlog

## Container Updates

### BL-1: Update library versions in Containerfile

**Priority:** High
**Status:** Pending

Update `deploy/podman/Containerfile` ARG versions and rebuild container image.

| Library | Current | Target | Notes |
|---------|---------|--------|-------|
| wolfSSL | 5.8.2 | **5.8.4** | [v5.8.4-stable](https://github.com/wolfSSL/wolfssl/releases/tag/v5.8.4-stable) — security fixes, check RN |
| nghttp2 | 1.65.0 | **1.68.0** | Already manually installed in running container |
| ngtcp2 | 1.12.0 | **1.21.0** | Major version jump — check API compat with wolfSSL crypto |
| nghttp3 | 1.9.0 | **1.15.0** | Major version jump — check API changes |
| cppcheck | 2.18.3 | **2.20.0** | New checks, possible new warnings |

Libraries already at latest (no action needed):
- liburing 2.14, wslay 1.1.1, yyjson 0.12.0
- Clang 22.1.0, CMake 4.2.3, Unity 2.6.1, Doxygen 1.16.1

**Action items:**
1. Update ARG versions in Containerfile
2. Check wolfSSL 5.8.4 release notes for breaking changes (esp. `--disable-sp-asm` workaround)
3. Check ngtcp2 1.21.0 API compatibility with `ngtcp2_crypto_wolfssl`
4. Rebuild: `podman build -t localhost/iohttp-dev:latest -f deploy/podman/Containerfile .`
5. Recreate container: `podman rm iohttp-dev && podman run -d --name iohttp-dev ...`

---

### BL-2: Review glibc release notes

**Priority:** Medium
**Status:** Pending

Container runs glibc 2.39 (OL10.1). Review release notes for relevant features/fixes:

- [glibc 2.40 (2024-07)](https://sourceware.org/pipermail/libc-alpha/2024-July/158467.html)
- [glibc 2.41 (2025-01)](https://sourceware.org/pipermail/libc-alpha/2025-January/164378.html)
- [glibc 2.42 (2025-07)](https://sourceware.org/pipermail/libc-alpha/2025-July/168994.html)
- [glibc 2.43 (2026-01)](https://sourceware.org/pipermail/libc-alpha/2026-January/174374.html)

**What to check:**
- New C23 support (stdckdint.h, constexpr, typeof improvements)
- io_uring-related fixes (io_uring_setup, mmap, etc.)
- Security fixes (FORTIFY_SOURCE, stack protector improvements)
- New POSIX functions we could use
- Deprecations or removals affecting our code

**If upgrade needed:**
- Source archive: https://ftp.gnu.org/gnu/glibc/
- Would require rebuilding container base image or custom glibc install
- OL10 may get glibc updates via dnf — check first before building from source

---

## Future Improvements

### BL-3: Pin mold version in Containerfile

Currently builds mold from `--depth 1` (latest main). Should pin to `v2.40.4` for reproducibility.

### BL-4: Add sfparse version tracking

sfparse (RFC 9651 structured fields) has no GitHub releases, only tags. Current: v0.3.0. Monitor for updates.
