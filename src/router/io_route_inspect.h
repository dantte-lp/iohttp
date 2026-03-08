/**
 * @file io_route_inspect.h
 * @brief Route introspection API for walking, counting, and metadata binding.
 *
 * Provides route walking for documentation generation (e.g. OpenAPI),
 * route counting, and post-registration metadata attachment.
 */

#ifndef IOHTTP_ROUTER_ROUTE_INSPECT_H
#define IOHTTP_ROUTER_ROUTE_INSPECT_H

#include "router/io_route_meta.h"
#include "router/io_router.h"

/* Route info returned during walk */
typedef struct {
    io_method_t method;
    const char *pattern;            /* reconstructed pattern string from trie */
    io_handler_fn handler;
    const io_route_opts_t *opts;    /* full route options (may be nullptr) */
    const io_route_meta_t *meta;    /* route metadata (may be nullptr) */
} io_route_info_t;

/* Walk callback — return 0 to continue, non-zero to stop */
typedef int (*io_route_walk_fn)(const io_route_info_t *info, void *ctx);

/**
 * @brief Walk all registered routes in the router.
 * @param r  Router to walk.
 * @param fn Callback invoked for each route with a handler.
 * @param ctx User context passed to callback.
 * @return 0 on success (all routes visited), positive if callback stopped early,
 *         negative errno on error.
 */
[[nodiscard]] int io_router_walk(const io_router_t *r, io_route_walk_fn fn, void *ctx);

/**
 * @brief Count registered routes across all method trees.
 * @param r Router.
 * @return Number of routes with handlers.
 */
uint32_t io_router_route_count(const io_router_t *r);

/**
 * @brief Attach route metadata to an existing route.
 * @param r       Router.
 * @param method  HTTP method of the route.
 * @param pattern Original route pattern (e.g. "/users/:id").
 * @param meta    Route metadata to attach.
 * @return 0 on success, -EINVAL on bad args, -ENOENT if route not found.
 */
[[nodiscard]] int io_router_set_meta(io_router_t *r, io_method_t method, const char *pattern,
                                     const io_route_meta_t *meta);

#endif /* IOHTTP_ROUTER_ROUTE_INSPECT_H */
