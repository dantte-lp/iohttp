/**
 * @file io_route_group.c
 * @brief Route groups with prefix composition and per-group middleware slots.
 */

#include "router/io_route_group.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr uint32_t GROUP_INITIAL_SUBGROUP_CAP = 4;

struct io_group {
    io_router_t *router;
    io_group_t *parent;
    char prefix[IO_MAX_GROUP_PREFIX];
    io_middleware_fn middleware[IO_MAX_GROUP_MIDDLEWARE];
    uint32_t middleware_count;
    io_group_t **subgroups;
    uint32_t subgroup_count;
    uint32_t subgroup_capacity;
};

/* ---- Internal helpers ---- */

/**
 * Build a full path by concatenating the group prefix and the pattern.
 * Returns 0 on success, -EINVAL if the result would overflow.
 */
static int build_full_path(const io_group_t *g, const char *pattern,
                            char *out, size_t out_size)
{
    int written = snprintf(out, out_size, "%s%s", g->prefix, pattern);
    if (written < 0 || (size_t)written >= out_size) {
        return -EINVAL;
    }
    return 0;
}

static int group_add_subgroup(io_group_t *parent, io_group_t *child)
{
    if (parent->subgroup_count >= parent->subgroup_capacity) {
        uint32_t new_cap = parent->subgroup_capacity == 0
                               ? GROUP_INITIAL_SUBGROUP_CAP
                               : parent->subgroup_capacity * 2;
        io_group_t **new_arr = realloc(
            parent->subgroups, (size_t)new_cap * sizeof(*new_arr));
        if (!new_arr) {
            return -ENOMEM;
        }
        parent->subgroups = new_arr;
        parent->subgroup_capacity = new_cap;
    }

    parent->subgroups[parent->subgroup_count++] = child;
    return 0;
}

/* Destructor wrapper for router ownership (void* interface) */
static void group_destroy_wrapper(void *ptr)
{
    io_group_destroy((io_group_t *)ptr);
}

/* ---- Public API ---- */

io_group_t *io_router_group(io_router_t *r, const char *prefix)
{
    if (!r || !prefix) {
        return nullptr;
    }

    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0 || prefix_len >= IO_MAX_GROUP_PREFIX) {
        return nullptr;
    }

    io_group_t *g = calloc(1, sizeof(*g));
    if (!g) {
        return nullptr;
    }

    g->router = r;
    g->parent = nullptr;
    memcpy(g->prefix, prefix, prefix_len + 1);

    /* Register ownership with the router */
    int rc = io_router_own_group(r, g, group_destroy_wrapper);
    if (rc < 0) {
        free(g);
        return nullptr;
    }

    return g;
}

io_group_t *io_group_subgroup(io_group_t *g, const char *prefix)
{
    if (!g || !prefix) {
        return nullptr;
    }

    size_t parent_len = strlen(g->prefix);
    size_t prefix_len = strlen(prefix);

    if (prefix_len == 0 || parent_len + prefix_len >= IO_MAX_GROUP_PREFIX) {
        return nullptr;
    }

    io_group_t *sub = calloc(1, sizeof(*sub));
    if (!sub) {
        return nullptr;
    }

    sub->router = g->router;
    sub->parent = g;

    /* Compose full prefix: parent prefix + this prefix */
    memcpy(sub->prefix, g->prefix, parent_len);
    memcpy(sub->prefix + parent_len, prefix, prefix_len + 1);

    int rc = group_add_subgroup(g, sub);
    if (rc < 0) {
        free(sub);
        return nullptr;
    }

    return sub;
}

int io_group_get(io_group_t *g, const char *pattern, io_handler_fn h)
{
    if (!g || !pattern || !h) {
        return -EINVAL;
    }

    char full[IO_MAX_GROUP_PREFIX + IO_MAX_URI_SIZE];
    int rc = build_full_path(g, pattern, full, sizeof(full));
    if (rc < 0) {
        return rc;
    }

    return io_router_handle(g->router, IO_METHOD_GET, full, h);
}

int io_group_post(io_group_t *g, const char *pattern, io_handler_fn h)
{
    if (!g || !pattern || !h) {
        return -EINVAL;
    }

    char full[IO_MAX_GROUP_PREFIX + IO_MAX_URI_SIZE];
    int rc = build_full_path(g, pattern, full, sizeof(full));
    if (rc < 0) {
        return rc;
    }

    return io_router_handle(g->router, IO_METHOD_POST, full, h);
}

int io_group_put(io_group_t *g, const char *pattern, io_handler_fn h)
{
    if (!g || !pattern || !h) {
        return -EINVAL;
    }

    char full[IO_MAX_GROUP_PREFIX + IO_MAX_URI_SIZE];
    int rc = build_full_path(g, pattern, full, sizeof(full));
    if (rc < 0) {
        return rc;
    }

    return io_router_handle(g->router, IO_METHOD_PUT, full, h);
}

int io_group_delete(io_group_t *g, const char *pattern, io_handler_fn h)
{
    if (!g || !pattern || !h) {
        return -EINVAL;
    }

    char full[IO_MAX_GROUP_PREFIX + IO_MAX_URI_SIZE];
    int rc = build_full_path(g, pattern, full, sizeof(full));
    if (rc < 0) {
        return rc;
    }

    return io_router_handle(g->router, IO_METHOD_DELETE, full, h);
}

int io_group_patch(io_group_t *g, const char *pattern, io_handler_fn h)
{
    if (!g || !pattern || !h) {
        return -EINVAL;
    }

    char full[IO_MAX_GROUP_PREFIX + IO_MAX_URI_SIZE];
    int rc = build_full_path(g, pattern, full, sizeof(full));
    if (rc < 0) {
        return rc;
    }

    return io_router_handle(g->router, IO_METHOD_PATCH, full, h);
}

int io_group_use(io_group_t *g, io_middleware_fn mw)
{
    if (!g || !mw) {
        return -EINVAL;
    }

    if (g->middleware_count >= IO_MAX_GROUP_MIDDLEWARE) {
        return -ENOSPC;
    }

    g->middleware[g->middleware_count++] = mw;
    return 0;
}

const char *io_group_prefix(const io_group_t *g)
{
    if (!g) {
        return nullptr;
    }
    return g->prefix;
}

const io_group_t *io_group_parent(const io_group_t *g)
{
    if (!g) {
        return nullptr;
    }
    return g->parent;
}

uint32_t io_group_middleware_count(const io_group_t *g)
{
    if (!g) {
        return 0;
    }
    return g->middleware_count;
}

io_middleware_fn io_group_middleware_at(const io_group_t *g, uint32_t idx)
{
    if (!g || idx >= g->middleware_count) {
        return nullptr;
    }
    return g->middleware[idx];
}

int io_group_get_with(io_group_t *g, const char *pattern,
                       io_handler_fn h, const io_route_opts_t *opts)
{
    if (!g || !pattern || !h) {
        return -EINVAL;
    }

    char full[IO_MAX_GROUP_PREFIX + IO_MAX_URI_SIZE];
    int rc = build_full_path(g, pattern, full, sizeof(full));
    if (rc < 0) {
        return rc;
    }

    return io_router_handle_with(g->router, IO_METHOD_GET, full, h, opts);
}

void io_group_destroy(io_group_t *g)
{
    if (!g) {
        return;
    }

    /* Recursively destroy subgroups */
    for (uint32_t i = 0; i < g->subgroup_count; i++) {
        io_group_destroy(g->subgroups[i]);
    }
    free(g->subgroups);
    free(g);
}
