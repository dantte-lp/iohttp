# iohttp Backlog

## Container Updates

### BL-1: Update library versions in Containerfile

**Priority:** High
**Status:** Done (Containerfile updated, image rebuild pending)

Containerfile ARG versions updated in commit `9be21ee`:

| Library | Old | New | Status |
|---------|-----|-----|--------|
| wolfSSL | 5.8.2 | **5.8.4** | Updated |
| nghttp2 | 1.65.0 | **1.68.0** | Updated |
| ngtcp2 | 1.12.0 | **1.21.0** | Updated |
| nghttp3 | 1.9.0 | **1.15.0** | Updated |
| cppcheck | 2.18.3 | **2.20.0** | Updated |

**Remaining:**
1. Rebuild: `podman build -t localhost/iohttp-dev:latest -f deploy/podman/Containerfile .`
2. Recreate container: `podman rm iohttp-dev && podman run -d --name iohttp-dev ...`

---

### BL-2: Review glibc release notes

**Priority:** Medium
**Status:** Done (reviewed 2026-03-08)

Container runs glibc 2.39 (OL10.1). Reviewed 2.40–2.43 release notes.

#### glibc 2.40 (2024-07)

- `_ISOC23_SOURCE` macro added (we use `_GNU_SOURCE` which covers it)
- `<stdbit.h>` type-generic macros improved for GCC 14.1+
- **FORTIFY_SOURCE enhanced for Clang** — relevant for our Clang 22 builds
- 5 CVEs fixed (iconv, nscd)
- No io_uring changes, no `<stdckdint.h>` changes

#### glibc 2.41 (2025-01)

- C23 trig functions (acospi, sinpi, etc.) — not needed for iohttp
- `_ISOC2Y_SOURCE` macro for draft C2Y
- **`sched_setattr`/`sched_getattr`** — useful for worker thread SCHED_DEADLINE (future)
- **`dlopen` no longer makes stack executable** — security improvement
- **`abort()` now async-signal-safe** — good for signal handlers
- DNS stub resolver `strict-error` option
- rseq ABI extension (kernel 6.3+)
- 1 CVE fixed (assert buffer overflow)

#### glibc 2.42 (2025-07)

- **`pthread_gettid_np`** — cleaner than `syscall(SYS_gettid)`, consider adopting
- **Arbitrary baud rate** via redefined `speed_t` — not relevant
- **malloc tcache caches large blocks** — perf improvement for free (we use mimalloc though)
- **Stack guard pages** via `MADV_GUARD_INSTALL` in `pthread_create` — free security
- **`termio.h` removed** — verify we don't use it (we don't)
- TX lock elision deprecated — we don't use it
- 4 CVEs fixed

#### glibc 2.43 (2026-01)

- **C23 const-preserving macros** — `strchr()` et al. propagate const; may break code assuming mutable returns. **Test with our codebase before upgrading.**
- NPTL trylock optimization for contended mutexes
- No io_uring changes

#### Upgrade decision

**No upgrade needed now.** Current glibc 2.39 is sufficient for iohttp.

Useful features if we upgrade later:
- `pthread_gettid_np` (2.42) — cleaner tid retrieval
- `sched_setattr` (2.41) — SCHED_DEADLINE for workers
- Stack guard pages (2.42) — free security
- FORTIFY_SOURCE Clang improvements (2.40)

**Risk:** C23 const-preserving macros in 2.43 may require code changes. Test before upgrading.

**Path:** OL10 dnf updates may ship newer glibc; check `dnf info glibc` periodically.

---

## Future Improvements

### BL-3: Pin mold version in Containerfile

**Status:** Done (pinned to v2.40.4 in commit `9be21ee`)

### BL-4: Add sfparse version tracking

sfparse (RFC 9651 structured fields) has no GitHub releases, only tags. Current: v0.3.0. Monitor for updates.
