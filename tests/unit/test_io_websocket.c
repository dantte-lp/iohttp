/**
 * @file test_io_websocket.c
 * @brief Unit tests for WebSocket protocol (RFC 6455).
 */

#include "ws/io_websocket.h"

#include <errno.h>
#include <string.h>

#include <unity.h>

void setUp(void)
{
}
void tearDown(void)
{
}

/* ---- Handshake ---- */

void test_ws_compute_accept(void)
{
    /* RFC 6455 §4.2.2 test vector */
    const char *key = "dGhlIHNhbXBsZSBub25jZQ==";
    char accept[IO_WS_ACCEPT_KEY_LEN + 1];

    int rc = io_ws_compute_accept(key, accept);
    TEST_ASSERT_EQUAL_INT(0, rc);
    TEST_ASSERT_EQUAL_STRING("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", accept);
}

void test_ws_validate_upgrade_valid(void)
{
    int rc =
        io_ws_validate_upgrade("GET", "websocket", "Upgrade", "dGhlIHNhbXBsZSBub25jZQ==", "13");
    TEST_ASSERT_EQUAL_INT(0, rc);
}

void test_ws_validate_upgrade_missing_key(void)
{
    int rc = io_ws_validate_upgrade("GET", "websocket", "Upgrade", "", "13");
    TEST_ASSERT_EQUAL_INT(-EINVAL, rc);
}

/* ---- Frame encoding ---- */

void test_ws_frame_encode_text(void)
{
    uint8_t buf[128];
    const char *msg = "Hello";

    int rc = io_ws_frame_encode(buf, sizeof(buf), IO_WS_OP_TEXT, true, (const uint8_t *)msg,
                                strlen(msg));
    TEST_ASSERT_EQUAL_INT(7, rc);         /* 2 header + 5 payload */
    TEST_ASSERT_EQUAL_HEX8(0x81, buf[0]); /* FIN + TEXT */
    TEST_ASSERT_EQUAL_HEX8(0x05, buf[1]); /* length 5, no mask */
    TEST_ASSERT_EQUAL_INT(0, memcmp(buf + 2, "Hello", 5));
}

void test_ws_frame_encode_binary(void)
{
    uint8_t buf[256];
    uint8_t payload[200];
    memset(payload, 0xAB, sizeof(payload));

    int rc = io_ws_frame_encode(buf, sizeof(buf), IO_WS_OP_BINARY, true, payload, sizeof(payload));
    /* 2 header + 2 extended length + 200 payload = 204 */
    TEST_ASSERT_EQUAL_INT(204, rc);
    TEST_ASSERT_EQUAL_HEX8(0x82, buf[0]); /* FIN + BINARY */
    TEST_ASSERT_EQUAL_HEX8(126, buf[1]);  /* 16-bit length indicator */
    /* Big-endian 200 = 0x00C8 */
    TEST_ASSERT_EQUAL_HEX8(0x00, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xC8, buf[3]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, buf[4]); /* first payload byte */
}

void test_ws_frame_encode_ping(void)
{
    uint8_t buf[32];

    int rc = io_ws_frame_encode(buf, sizeof(buf), IO_WS_OP_PING, true, nullptr, 0);
    TEST_ASSERT_EQUAL_INT(2, rc);         /* 2 header, no payload */
    TEST_ASSERT_EQUAL_HEX8(0x89, buf[0]); /* FIN + PING */
    TEST_ASSERT_EQUAL_HEX8(0x00, buf[1]); /* zero length */
}

void test_ws_frame_encode_pong(void)
{
    uint8_t buf[32];
    const char *data = "ping-data";

    int rc = io_ws_frame_encode(buf, sizeof(buf), IO_WS_OP_PONG, true, (const uint8_t *)data,
                                strlen(data));
    TEST_ASSERT_EQUAL_INT(11, rc);        /* 2 + 9 */
    TEST_ASSERT_EQUAL_HEX8(0x8A, buf[0]); /* FIN + PONG */
    TEST_ASSERT_EQUAL_HEX8(0x09, buf[1]); /* length 9 */
    TEST_ASSERT_EQUAL_INT(0, memcmp(buf + 2, "ping-data", 9));
}

void test_ws_frame_encode_close(void)
{
    uint8_t buf[32];
    /* Close frame: 2-byte status code + reason */
    uint8_t payload[7];
    /* Status 1000 = 0x03E8 */
    payload[0] = 0x03;
    payload[1] = 0xE8;
    memcpy(payload + 2, "Bye!", 4);
    payload[6] = '\0'; /* not needed but for clarity */

    int rc = io_ws_frame_encode(buf, sizeof(buf), IO_WS_OP_CLOSE, true, payload, 6);
    TEST_ASSERT_EQUAL_INT(8, rc);         /* 2 header + 6 payload */
    TEST_ASSERT_EQUAL_HEX8(0x88, buf[0]); /* FIN + CLOSE */
    TEST_ASSERT_EQUAL_HEX8(0x06, buf[1]); /* length 6 */
    TEST_ASSERT_EQUAL_HEX8(0x03, buf[2]); /* close code high */
    TEST_ASSERT_EQUAL_HEX8(0xE8, buf[3]); /* close code low */
    TEST_ASSERT_EQUAL_INT(0, memcmp(buf + 4, "Bye!", 4));
}

/* ---- Frame decoding ---- */

void test_ws_frame_decode_text(void)
{
    /* Unmasked text frame: "Hello" */
    uint8_t raw[] = {0x81, 0x05, 'H', 'e', 'l', 'l', 'o'};
    io_ws_frame_t frame;

    int rc = io_ws_frame_decode(raw, sizeof(raw), &frame);
    TEST_ASSERT_EQUAL_INT(7, rc);
    TEST_ASSERT_TRUE(frame.fin);
    TEST_ASSERT_EQUAL_UINT8(IO_WS_OP_TEXT, frame.opcode);
    TEST_ASSERT_FALSE(frame.masked);
    TEST_ASSERT_EQUAL_UINT64(5, frame.payload_len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(frame.payload, "Hello", 5));
}

void test_ws_frame_decode_masked(void)
{
    /* Masked text frame: "Hello" with mask key 0x37 0xFA 0x21 0x3D */
    uint8_t raw[] = {
        0x81, 0x85,                  /* FIN + TEXT, MASK + len=5 */
        0x37, 0xFA, 0x21, 0x3D,      /* mask key */
        0x7F, 0x9F, 0x4D, 0x51, 0x58 /* masked "Hello" */
    };
    io_ws_frame_t frame;

    int rc = io_ws_frame_decode(raw, sizeof(raw), &frame);
    TEST_ASSERT_EQUAL_INT(11, rc);
    TEST_ASSERT_TRUE(frame.fin);
    TEST_ASSERT_EQUAL_UINT8(IO_WS_OP_TEXT, frame.opcode);
    TEST_ASSERT_TRUE(frame.masked);
    TEST_ASSERT_EQUAL_UINT64(5, frame.payload_len);

    /* Unmask in-place to verify */
    uint8_t unmasked[5];
    memcpy(unmasked, frame.payload, 5);
    io_ws_apply_mask(unmasked, 5, frame.mask_key);
    TEST_ASSERT_EQUAL_INT(0, memcmp(unmasked, "Hello", 5));
}

void test_ws_frame_decode_fragmented(void)
{
    /* First fragment: continuation with FIN=0 */
    uint8_t frag1[] = {0x01, 0x03, 'H', 'e', 'l'}; /* TEXT, not FIN */
    io_ws_frame_t frame1;

    int rc1 = io_ws_frame_decode(frag1, sizeof(frag1), &frame1);
    TEST_ASSERT_EQUAL_INT(5, rc1);
    TEST_ASSERT_FALSE(frame1.fin);
    TEST_ASSERT_EQUAL_UINT8(IO_WS_OP_TEXT, frame1.opcode);
    TEST_ASSERT_EQUAL_UINT64(3, frame1.payload_len);

    /* Final fragment: continuation with FIN=1 */
    uint8_t frag2[] = {0x80, 0x02, 'l', 'o'}; /* CONTINUATION, FIN */
    io_ws_frame_t frame2;

    int rc2 = io_ws_frame_decode(frag2, sizeof(frag2), &frame2);
    TEST_ASSERT_EQUAL_INT(4, rc2);
    TEST_ASSERT_TRUE(frame2.fin);
    TEST_ASSERT_EQUAL_UINT8(IO_WS_OP_CONTINUATION, frame2.opcode);
    TEST_ASSERT_EQUAL_UINT64(2, frame2.payload_len);

    /* Reassemble */
    char reassembled[6];
    memcpy(reassembled, frame1.payload, 3);
    memcpy(reassembled + 3, frame2.payload, 2);
    reassembled[5] = '\0';
    TEST_ASSERT_EQUAL_STRING("Hello", reassembled);
}

void test_ws_frame_decode_oversized(void)
{
    /* Frame claiming 64-bit length > buffer */
    uint8_t raw[] = {
        0x81, 0x7F,                                    /* TEXT, 64-bit length */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00 /* 1 MiB */
    };
    io_ws_frame_t frame;

    /* Only 10 bytes in buffer, but payload claims 1 MiB → -EAGAIN */
    int rc = io_ws_frame_decode(raw, sizeof(raw), &frame);
    TEST_ASSERT_EQUAL_INT(-EAGAIN, rc);
}

/* ---- Test runner ---- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ws_compute_accept);
    RUN_TEST(test_ws_validate_upgrade_valid);
    RUN_TEST(test_ws_validate_upgrade_missing_key);
    RUN_TEST(test_ws_frame_encode_text);
    RUN_TEST(test_ws_frame_encode_binary);
    RUN_TEST(test_ws_frame_decode_text);
    RUN_TEST(test_ws_frame_decode_masked);
    RUN_TEST(test_ws_frame_encode_ping);
    RUN_TEST(test_ws_frame_encode_pong);
    RUN_TEST(test_ws_frame_encode_close);
    RUN_TEST(test_ws_frame_decode_fragmented);
    RUN_TEST(test_ws_frame_decode_oversized);

    return UNITY_END();
}
