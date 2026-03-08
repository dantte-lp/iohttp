/**
 * @file io_websocket.h
 * @brief WebSocket protocol (RFC 6455) frame encoding/decoding and handshake.
 */

#ifndef IOHTTP_WS_WEBSOCKET_H
#define IOHTTP_WS_WEBSOCKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* WebSocket opcodes (RFC 6455 §11.8) */
typedef enum : uint8_t {
    IO_WS_OP_CONTINUATION = 0x0,
    IO_WS_OP_TEXT         = 0x1,
    IO_WS_OP_BINARY       = 0x2,
    IO_WS_OP_CLOSE        = 0x8,
    IO_WS_OP_PING         = 0x9,
    IO_WS_OP_PONG         = 0xA,
} io_ws_opcode_t;

/* WebSocket close codes (RFC 6455 §7.4.1) */
constexpr uint16_t IO_WS_CLOSE_NORMAL     = 1000;
constexpr uint16_t IO_WS_CLOSE_GOING_AWAY = 1001;
constexpr uint16_t IO_WS_CLOSE_PROTOCOL   = 1002;
constexpr uint16_t IO_WS_CLOSE_INVALID    = 1003;
constexpr uint16_t IO_WS_CLOSE_TOO_BIG    = 1009;

/* Maximum sizes */
constexpr size_t IO_WS_MAX_FRAME_HEADER     = 14; /* 2 + 4(mask) + 8(64-bit len) */
constexpr size_t IO_WS_MAX_CONTROL_PAYLOAD  = 125;
constexpr size_t IO_WS_GUID_LEN            = 36;
constexpr size_t IO_WS_ACCEPT_KEY_LEN      = 28; /* base64(SHA-1) */
constexpr size_t IO_WS_DEFAULT_MAX_MSG      = 65536;

/* WebSocket frame (parsed) */
typedef struct {
    io_ws_opcode_t opcode;
    bool fin;
    bool masked;
    uint8_t mask_key[4];
    uint64_t payload_len;
    const uint8_t *payload;
} io_ws_frame_t;

/* WebSocket connection context for fragmented messages */
typedef struct {
    uint8_t *frag_buf;
    size_t frag_len;
    size_t frag_capacity;
    io_ws_opcode_t frag_opcode;
    size_t max_message_size;
    bool in_fragment;
} io_ws_conn_t;

/* ---- Upgrade handshake ---- */

/**
 * @brief Compute the Sec-WebSocket-Accept value from client key.
 * @param client_key  The Sec-WebSocket-Key header value (24 chars base64).
 * @param accept_out  Output buffer (at least IO_WS_ACCEPT_KEY_LEN + 1 bytes).
 * @return 0 on success, -EINVAL if client_key is nullptr.
 */
[[nodiscard]] int io_ws_compute_accept(const char *client_key, char *accept_out);

/**
 * @brief Validate an HTTP upgrade request for WebSocket.
 * @param method       HTTP method string.
 * @param upgrade_hdr  Upgrade header value.
 * @param conn_hdr     Connection header value.
 * @param ws_key       Sec-WebSocket-Key value.
 * @param ws_version   Sec-WebSocket-Version value.
 * @return 0 if valid, -EINVAL if invalid.
 */
[[nodiscard]] int io_ws_validate_upgrade(const char *method, const char *upgrade_hdr,
                                         const char *conn_hdr, const char *ws_key,
                                         const char *ws_version);

/* ---- Frame encoding/decoding ---- */

/**
 * @brief Encode a WebSocket frame into buffer.
 * Server frames are NOT masked (RFC 6455 §5.1).
 * @param buf       Output buffer.
 * @param buf_size  Buffer capacity.
 * @param opcode    Frame opcode.
 * @param fin       Final fragment flag.
 * @param payload   Payload data (may be nullptr if len==0).
 * @param len       Payload length.
 * @return Total frame size on success, -ENOSPC if buffer too small.
 */
[[nodiscard]] int io_ws_frame_encode(uint8_t *buf, size_t buf_size,
                                     io_ws_opcode_t opcode, bool fin,
                                     const uint8_t *payload, size_t len);

/**
 * @brief Decode a WebSocket frame from buffer.
 * @param buf      Input buffer.
 * @param buf_len  Input buffer length.
 * @param frame    Output parsed frame.
 * @return Total consumed bytes on success, -EAGAIN if incomplete,
 *         -EINVAL if malformed, -E2BIG if payload exceeds limit.
 */
[[nodiscard]] int io_ws_frame_decode(const uint8_t *buf, size_t buf_len,
                                     io_ws_frame_t *frame);

/**
 * @brief Apply/remove XOR mask on payload data (in-place).
 * @param data      Data to mask/unmask.
 * @param len       Data length.
 * @param mask_key  4-byte mask key.
 */
void io_ws_apply_mask(uint8_t *data, size_t len, const uint8_t mask_key[4]);

/* ---- Connection context ---- */

/**
 * @brief Initialize a WebSocket connection context.
 * @param ws               Connection context to initialize.
 * @param max_message_size Maximum reassembled message size (0 for default).
 * @return 0 on success, -EINVAL if ws is nullptr.
 */
[[nodiscard]] int io_ws_conn_init(io_ws_conn_t *ws, size_t max_message_size);

/**
 * @brief Reset connection context, freeing fragment buffer.
 * @param ws Connection context.
 */
void io_ws_conn_reset(io_ws_conn_t *ws);

/**
 * @brief Destroy connection context, freeing all resources.
 * @param ws Connection context.
 */
void io_ws_conn_destroy(io_ws_conn_t *ws);

#endif /* IOHTTP_WS_WEBSOCKET_H */
