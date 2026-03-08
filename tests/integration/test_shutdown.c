/**
 * @file test_shutdown.c
 * @brief Integration tests for graceful shutdown with connection drain.
 */

#include "core/io_ctx.h"
#include "core/io_server.h"
#include "http/io_request.h"

#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <unity.h>

void setUp(void)
{
}
void tearDown(void)
{
}

/* ---- Helpers ---- */

static uint16_t get_bound_port(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(fd, (struct sockaddr *)&addr, &len);
    return ntohs(addr.sin_port);
}

static int connect_to(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return -errno;
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
    };
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -errno;
    }
    return fd;
}

/* ---- Handler ---- */

static int handler(io_ctx_t *c)
{
    return io_ctx_text(c, 200, "OK");
}

static int on_request_cb(io_ctx_t *c, void *user_data)
{
    (void)user_data;
    return handler(c);
}

/* ---- Test helpers ---- */

static uint16_t next_port = 19080;

static io_server_t *make_server(void)
{
    io_server_config_t cfg;
    io_server_config_init(&cfg);
    cfg.listen_port = next_port++;
    cfg.max_connections = 16;
    cfg.queue_depth = 32;
    cfg.keepalive_timeout_ms = 500; /* short timeout for drain tests */
    return io_server_create(&cfg);
}

/* ---- Test 1: Immediate shutdown closes all connections ---- */

void test_shutdown_immediate_closes_all(void)
{
    io_server_t *srv = make_server();
    TEST_ASSERT_NOT_NULL(srv);
    TEST_ASSERT_EQUAL_INT(0, io_server_set_on_request(srv, on_request_cb, nullptr));

    int listen_fd = io_server_listen(srv);
    TEST_ASSERT_GREATER_THAN(0, listen_fd);
    uint16_t port = get_bound_port(listen_fd);

    /* Connect a client */
    int client = connect_to(port);
    TEST_ASSERT_TRUE(client >= 0);

    /* Let server accept the connection */
    for (int i = 0; i < 3; i++) {
        (void)io_server_run_once(srv, 100);
    }
    TEST_ASSERT_EQUAL_UINT32(1, io_conn_pool_active(io_server_pool(srv)));

    /* Immediate shutdown */
    TEST_ASSERT_EQUAL_INT(0, io_server_shutdown(srv, IO_SHUTDOWN_IMMEDIATE));
    TEST_ASSERT_EQUAL_UINT32(0, io_conn_pool_active(io_server_pool(srv)));

    close(client);
    io_server_destroy(srv);
}

/* ---- Test 2: Drain shutdown closes idle connections ---- */

void test_shutdown_drain(void)
{
    io_server_t *srv = make_server();
    TEST_ASSERT_NOT_NULL(srv);
    TEST_ASSERT_EQUAL_INT(0, io_server_set_on_request(srv, on_request_cb, nullptr));

    int listen_fd = io_server_listen(srv);
    TEST_ASSERT_GREATER_THAN(0, listen_fd);
    uint16_t port = get_bound_port(listen_fd);

    /* Connect a client */
    int client = connect_to(port);
    TEST_ASSERT_TRUE(client >= 0);

    /* Let server accept the connection */
    for (int i = 0; i < 3; i++) {
        (void)io_server_run_once(srv, 100);
    }
    TEST_ASSERT_EQUAL_UINT32(1, io_conn_pool_active(io_server_pool(srv)));

    /* Close client side so server sees EOF during drain */
    close(client);

    /* Drain shutdown — should close connections after drain timeout */
    TEST_ASSERT_EQUAL_INT(0, io_server_shutdown(srv, IO_SHUTDOWN_DRAIN));
    TEST_ASSERT_EQUAL_UINT32(0, io_conn_pool_active(io_server_pool(srv)));

    io_server_destroy(srv);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_shutdown_immediate_closes_all);
    RUN_TEST(test_shutdown_drain);
    return UNITY_END();
}
