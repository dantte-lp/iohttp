/**
 * @file io_health.c
 * @brief Health check endpoint framework implementation.
 */

#include "core/io_health.h"

#include "core/io_ctx.h"
#include "core/io_server.h"
#include "router/io_router.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef IOHTTP_HAVE_YYJSON
#include <yyjson.h>
#endif

/* ---- File-static state ---- */

static const io_health_config_t *s_health_cfg;
static io_server_t *s_health_srv;

/* ---- Config ---- */

void io_health_config_init(io_health_config_t *cfg)
{
    if (cfg == nullptr) {
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->enabled = true;
    cfg->health_path = "/health";
    cfg->ready_path = "/ready";
    cfg->live_path = "/live";
    cfg->checker_count = 0;
}

int io_health_add_checker(io_health_config_t *cfg, const char *name,
                          io_health_check_fn check, void *user_data)
{
    if (cfg == nullptr || name == nullptr || check == nullptr) {
        return -EINVAL;
    }
    if (cfg->checker_count >= IO_HEALTH_MAX_CHECKS) {
        return -ENOSPC;
    }

    cfg->checkers[cfg->checker_count].name = name;
    cfg->checkers[cfg->checker_count].check = check;
    cfg->checkers[cfg->checker_count].user_data = user_data;
    cfg->checker_count++;

    return 0;
}

/* ---- Handlers ---- */

static int health_handler(io_ctx_t *c)
{
    return io_ctx_json(c, 200, "{\"status\":\"ok\"}");
}

static int ready_handler(io_ctx_t *c)
{
    if (io_server_is_draining(s_health_srv)) {
        return io_ctx_json(c, 503, "{\"status\":\"unavailable\"}");
    }
    return io_ctx_json(c, 200, "{\"status\":\"ready\"}");
}

static int live_handler(io_ctx_t *c)
{
    if (s_health_cfg == nullptr || s_health_cfg->checker_count == 0) {
        return io_ctx_json(c, 200, "{\"status\":\"ok\"}");
    }

    bool all_healthy = true;

#ifdef IOHTTP_HAVE_YYJSON
    yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
    if (doc == nullptr) {
        return io_ctx_json(c, 500, "{\"status\":\"error\",\"message\":\"OOM\"}");
    }

    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_val *checks_obj = yyjson_mut_obj(doc);

    for (uint32_t i = 0; i < s_health_cfg->checker_count; i++) {
        const io_health_checker_t *chk = &s_health_cfg->checkers[i];
        const char *msg = nullptr;
        int rc = chk->check(&msg, chk->user_data);

        yyjson_mut_val *entry = yyjson_mut_obj(doc);
        if (rc == 0) {
            yyjson_mut_obj_add_str(doc, entry, "status", "ok");
        } else {
            yyjson_mut_obj_add_str(doc, entry, "status", "fail");
            all_healthy = false;
        }
        if (msg != nullptr) {
            yyjson_mut_obj_add_str(doc, entry, "message", msg);
        }
        yyjson_mut_obj_add_val(doc, checks_obj, chk->name, entry);
    }

    yyjson_mut_obj_add_str(doc, root, "status", all_healthy ? "ok" : "degraded");
    yyjson_mut_obj_add_val(doc, root, "checks", checks_obj);

    const char *json = yyjson_mut_write(doc, 0, nullptr);
    int ret;
    if (json != nullptr) {
        ret = io_ctx_json(c, all_healthy ? 200 : 503, json);
        free((void *)json);
    } else {
        ret = io_ctx_json(c, 500, "{\"status\":\"error\"}");
    }

    yyjson_mut_doc_free(doc);
    return ret;

#else
    /* Simple string concat fallback without yyjson */
    char buf[1024];
    int off = snprintf(buf, sizeof(buf), "{\"status\":");

    /* First pass: run checks to determine overall status */
    for (uint32_t i = 0; i < s_health_cfg->checker_count; i++) {
        const char *msg = nullptr;
        int rc = s_health_cfg->checkers[i].check(&msg, s_health_cfg->checkers[i].user_data);
        if (rc != 0) {
            all_healthy = false;
            break;
        }
    }

    off += snprintf(buf + off, sizeof(buf) - (size_t)off,
                    "\"%s\",\"checks\":{", all_healthy ? "ok" : "degraded");

    for (uint32_t i = 0; i < s_health_cfg->checker_count; i++) {
        const io_health_checker_t *chk = &s_health_cfg->checkers[i];
        const char *msg = nullptr;
        int rc = chk->check(&msg, chk->user_data);

        if (i > 0) {
            off += snprintf(buf + off, sizeof(buf) - (size_t)off, ",");
        }

        off += snprintf(buf + off, sizeof(buf) - (size_t)off,
                        "\"%s\":{\"status\":\"%s\"", chk->name, rc == 0 ? "ok" : "fail");
        if (msg != nullptr) {
            off += snprintf(buf + off, sizeof(buf) - (size_t)off, ",\"message\":\"%s\"", msg);
        }
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "}");
    }

    off += snprintf(buf + off, sizeof(buf) - (size_t)off, "}}");

    if (off < 0 || (size_t)off >= sizeof(buf)) {
        return io_ctx_json(c, 500, "{\"status\":\"error\",\"message\":\"response too large\"}");
    }

    return io_ctx_json(c, all_healthy ? 200 : 503, buf);
#endif
}

/* ---- Registration ---- */

int io_health_register(io_router_t *r, io_server_t *srv, const io_health_config_t *cfg)
{
    if (r == nullptr || cfg == nullptr) {
        return -EINVAL;
    }

    if (!cfg->enabled) {
        return 0;
    }

    s_health_cfg = cfg;
    s_health_srv = srv;

    int rc = io_router_get(r, cfg->health_path, health_handler);
    if (rc < 0) {
        return rc;
    }

    rc = io_router_get(r, cfg->ready_path, ready_handler);
    if (rc < 0) {
        return rc;
    }

    rc = io_router_get(r, cfg->live_path, live_handler);
    if (rc < 0) {
        return rc;
    }

    return 0;
}
