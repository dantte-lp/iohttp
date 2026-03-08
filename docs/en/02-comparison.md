# Comparison with Existing Embedded HTTP Servers

## Executive Summary

iohttp fills a gap in the C embedded HTTP server landscape: **modern protocol support
(HTTP/1.1 + HTTP/2 + HTTP/3) with native io_uring and wolfSSL integration**. No existing
library combines all three. The closest competitor, H2O, supports all protocols but
requires its own picotls/quicly stack (incompatible with wolfSSL) and weighs 80-120K LOC.

---

## Feature Matrix

| Feature | **iohttp** | Mongoose | CivetWeb | libmicrohttpd | H2O | facil.io | Lwan |
|---------|-----------|----------|----------|---------------|-----|----------|------|
| **Language** | C23 | C99 | C (C++) | C99 | C11 | C | C11 |
| **License** | GPLv3 | GPLv2 | MIT | LGPL 2.1 | MIT | MIT | GPLv2 |
| **HTTP/1.1** | picohttpparser | custom | custom | custom | picohttpparser | custom | custom |
| **HTTP/2** | nghttp2 | -- | experimental | -- | custom | -- | -- |
| **HTTP/3** | ngtcp2+nghttp3 | -- | -- | -- | quicly | -- | -- |
| **WebSocket** | RFC 6455 | yes | yes | expmt. | yes | yes | -- |
| **SSE** | yes | manual | yes | -- | -- | -- | -- |
| **I/O model** | io_uring | select/poll/epoll | threads | select/poll/epoll | epoll | epoll | epoll |
| **TLS** | wolfSSL native | 5 backends | OpenSSL | GnuTLS | picotls | OpenSSL | mbedTLS |
| **wolfSSL** | native API | compat layer | -- | -- | -- | -- | -- |
| **QUIC/wolfSSL** | ngtcp2_crypto_wolfssl | -- | -- | -- | -- | -- | -- |
| **PROXY proto** | v1+v2 | -- | -- | -- | v2 | -- | -- |
| **SPA fallback** | yes | -- | -- | -- | via mruby | -- | -- |
| **Static files** | sendfile+#embed | yes | yes | -- | yes | yes | yes |
| **gzip/brotli** | streaming+precomp | precomp only | yes | -- | yes | -- | yes (gzip) |
| **Rate limiting** | token bucket | -- | -- | -- | -- | -- | -- |
| **CORS** | built-in | manual | manual | manual | manual | -- | -- |
| **JSON** | yyjson (~2.4 GB/s) | built-in | -- | -- | -- | -- | -- |
| **Metrics** | Prometheus | -- | -- | -- | -- | -- | -- |
| **Zero-copy** | io_uring SEND_ZC | -- | -- | -- | -- | -- | splice |
| **Linux only** | yes (6.7+) | cross-platform | cross-platform | cross-platform | Linux | Linux | Linux |
| **Own LOC** | ~12-20K | ~25-30K | ~30K | ~25K | ~80-120K | ~15-25K | ~20K |

---

## Detailed Comparison

### Mongoose (Cesanta, GPLv2)

**Strengths:** Single-file amalgamation, embedded-friendly (MCU, RTOS), extensive
driver support (Wi-Fi, Ethernet), working wolfSSL backend (`MG_TLS=MG_TLS_WOLFSSL`),
WebSocket with auto-fragmentation, precompressed gzip serving.

**Weaknesses:**
- **No HTTP/2** — Cesanta: *"We don't have any timeline"* (GitHub Discussion #1427)
- **No HTTP/3** — TCP-only architecture, no QUIC support
- **select()-based** — default FD_SETSIZE=1024 limit, epoll optional
- **No io_uring** — would require complete I/O core rewrite
- **No middleware** — no rate limiting, CORS, auth chains
- **GPLv2-only** — incompatible with GPLv3, commercial license pricing unpublished
- **No SPA fallback** — requires manual implementation
- **No SSE API** — possible via chunked encoding but no dedicated interface

**Verdict:** Excellent for embedded MCU HTTP/1.1 + WebSocket. Structurally unable to
support HTTP/2+ without architectural rewrite. wolfSSL integration exists but through
compatibility layer, not native API.

### CivetWeb (MIT fork of Mongoose)

**Strengths:** MIT license, multi-threaded model, experimental HTTP/2, CGI/Lua support,
~2900 GitHub stars, active community.

**Weaknesses:**
- **Thread-per-connection** — does not scale beyond hundreds of connections
- **HTTP/2 experimental** — not production-ready
- **No HTTP/3** — no QUIC support
- **No io_uring** — thread pool + poll/epoll
- **No wolfSSL** — OpenSSL/mbedTLS only
- **Codebase diverged** from Mongoose since 2013, ~30K LOC
- **No zero-copy** — traditional read/write

**Verdict:** Good MIT-licensed alternative to Mongoose for HTTP/1.1 with threads.
Not suitable when io_uring, wolfSSL, or HTTP/2+ is required.

### libmicrohttpd (GNU, LGPL 2.1)

**Strengths:** GNU project quality, LGPL license allows proprietary linking,
excellent RFC compliance, battle-tested in production systems, supports
internal thread pool and external select/poll/epoll integration.

**Weaknesses:**
- **HTTP/1.1 only** — no HTTP/2, no HTTP/3
- **No WebSocket** — only experimental support
- **No io_uring** — select/poll/epoll
- **GnuTLS only** — no wolfSSL support
- **No static file serving** — manual implementation required
- **No routing** — application handles all path matching
- **No middleware** — no built-in CORS, rate limiting, auth
- **Verbose API** — callback-heavy, complex configuration

**Verdict:** Solid HTTP/1.1 library with excellent standards compliance. Too minimal
for modern web serving — no protocol upgrades, no convenience features.

### H2O (DeNA, MIT)

**Strengths:** Full protocol suite (HTTP/1.1 + HTTP/2 + HTTP/3), excellent performance
(uses picohttpparser), production-tested at scale, mruby scripting, comprehensive
configuration, PROXY protocol v2.

**Weaknesses:**
- **No wolfSSL** — requires picotls (custom TLS) + BoringSSL/OpenSSL
- **No io_uring** — epoll-based event loop
- **Massive codebase** — 80-120K LOC including bundled dependencies
- **Complex build** — many dependencies, cmake + custom build scripts
- **QUIC via quicly** — custom QUIC implementation (not ngtcp2), BoringSSL-only
- **Not embeddable** — designed as standalone server, not library
- **Heavy** — too large for embedded / embedded-library use case

**Verdict:** Most feature-complete HTTP server in C. But not embeddable, no wolfSSL,
no io_uring. Architecture is standalone server, not library. Too heavy for iohttp's
use case.

### facil.io (MIT)

**Strengths:** MIT license, well-designed event-driven architecture, WebSocket support,
pub/sub system, Redis integration.

**Weaknesses:**
- **HTTP/1.1 only** — no HTTP/2 or HTTP/3
- **No wolfSSL** — OpenSSL only
- **No io_uring** — custom epoll event loop
- **15-25K LOC** — medium-sized codebase
- **Mostly abandoned** — infrequent updates in recent years

**Verdict:** Well-architected HTTP/1.1 library with good API design. Protocol
limitations and maintenance status make it unsuitable.

### Lwan (GPLv2)

**Strengths:** Extremely fast HTTP/1.1 (uses coroutines), splice-based zero-copy,
excellent benchmark results, coroutine-based request handling.

**Weaknesses:**
- **HTTP/1.1 only** — no HTTP/2 or HTTP/3
- **No TLS** — mbedTLS only, experimental
- **No wolfSSL** — not supported
- **No WebSocket** — not supported
- **GPLv2** — viral license
- **Linux-specific** — good for us, but no portability

**Verdict:** Impressive HTTP/1.1 performance through coroutines and splice. Too
limited in protocol support and TLS integration.

---

## Protocol Library Stack (iohttp's Approach)

All protocol libraries are MIT-licensed, I/O-agnostic (callback-based), and written
or maintained by the same community (Tatsuhiro Tsujikawa and contributors).

| Library | Function | LOC | License | wolfSSL Support |
|---------|----------|-----|---------|-----------------|
| picohttpparser | HTTP/1.1 parser | ~800 | MIT | N/A |
| nghttp2 | HTTP/2 frames + HPACK | ~18K | MIT | Via ALPN |
| ngtcp2 | QUIC transport | ~28K | MIT | **First-class** (ngtcp2_crypto_wolfssl) |
| nghttp3 | HTTP/3 + QPACK | ~12K | MIT | N/A |
| liburing | io_uring wrappers | ~3K | LGPL+MIT | N/A |
| yyjson | JSON (2.4 GB/s) | ~8K | MIT | N/A |
| wolfSSL | TLS 1.3, QUIC crypto | — | GPLv2+ | — |

**Key advantage:** ngtcp2 is the **only** QUIC library with native wolfSSL support,
via `libngtcp2_crypto_wolfssl` developed by wolfSSL's team. Other QUIC libraries
(quiche, msquic, lsquic, picoquic) require BoringSSL, OpenSSL, or Schannel.

---

## Performance Positioning

| Metric | iohttp (target) | Mongoose | H2O | nginx |
|--------|-----------------|----------|-----|-------|
| HTTP/1.1 parsing | ~2.5 GB/s (SSE4.2) | ~1 GB/s | ~2.5 GB/s | ~1.5 GB/s |
| I/O model | io_uring (zero-syscall) | select/epoll | epoll | epoll |
| JSON serialization | ~2.4 GB/s (yyjson) | ~200 MB/s | N/A | N/A |
| Zero-copy send | SEND_ZC (6.0+) | N/A | N/A | sendfile |
| Buffer management | Kernel-managed rings | Application | Application | Application |
| Connection scalability | 10K+ (io_uring) | ~1K (select) | 10K+ | 10K+ |

---

## Why iohttp?

1. **Only** embedded C HTTP library with HTTP/1.1 + HTTP/2 + HTTP/3 support
2. **Only** library with native io_uring I/O engine (not epoll shim)
3. **Only** library with native wolfSSL integration (not OpenSSL compat layer)
4. **Only** QUIC implementation compatible with wolfSSL (via ngtcp2)
5. **Smallest own code** for full protocol coverage (~12-20K vs 80-120K for H2O)
6. **Single TLS stack** — wolfSSL for HTTPS, QUIC, and mTLS
7. **Modern C23** — nullptr, constexpr, [[nodiscard]], type-safe enums
8. **Production features** — SPA, CORS, rate limiting, metrics, PROXY protocol
