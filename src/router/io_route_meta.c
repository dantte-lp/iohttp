/**
 * @file io_route_meta.c
 * @brief Route metadata utility functions.
 */

#include "router/io_route_meta.h"

const char *io_param_in_name(io_param_in_t in)
{
    switch (in) {
    case IO_PARAM_PATH:
        return "path";
    case IO_PARAM_QUERY:
        return "query";
    case IO_PARAM_HEADER:
        return "header";
    case IO_PARAM_COOKIE:
        return "cookie";
    default:
        return "unknown";
    }
}
