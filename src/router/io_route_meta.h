/**
 * @file io_route_meta.h
 * @brief Route metadata types for introspection and documentation.
 *
 * Provides structured per-route metadata (summary, tags, params, deprecated)
 * independent of liboas. Stored via io_route_opts_t.meta pointer.
 */

#ifndef IOHTTP_ROUTER_ROUTE_META_H
#define IOHTTP_ROUTER_ROUTE_META_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Parameter location in HTTP request */
typedef enum : uint8_t {
    IO_PARAM_PATH,   /* /users/:id */
    IO_PARAM_QUERY,  /* ?page=1 */
    IO_PARAM_HEADER, /* X-Request-Id */
    IO_PARAM_COOKIE, /* session_id */
} io_param_in_t;

/* Per-parameter metadata (name, location, description) */
typedef struct {
    const char *name;        /* parameter name, e.g. "id" */
    const char *description; /* human-readable, e.g. "User UUID" */
    io_param_in_t in;        /* where in the request */
    bool required;           /* always true for path params */
} io_param_meta_t;

/* Per-route metadata for introspection and documentation */
typedef struct {
    const char *summary;            /* short description, e.g. "Get user by ID" */
    const char *description;        /* longer text (nullable) */
    const char *const *tags;        /* nullptr-terminated array, e.g. {"users", nullptr} */
    bool deprecated;                /* route is deprecated */
    const io_param_meta_t *params;  /* array of param descriptors */
    uint32_t param_count;           /* number of params */
} io_route_meta_t;

/**
 * @brief Get string name for a parameter location.
 * @param in Parameter location enum value.
 * @return Static string: "path", "query", "header", "cookie", or "unknown".
 */
const char *io_param_in_name(io_param_in_t in);

#endif /* IOHTTP_ROUTER_ROUTE_META_H */
