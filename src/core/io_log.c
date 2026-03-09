/**
 * @file io_log.c
 * @brief Structured logging implementation with level filtering and custom sinks.
 */

#include "core/io_log.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* ---- Global state ---- */

static io_log_level_t g_min_level = IO_LOG_INFO;
static io_log_sink_fn g_sink = nullptr;
static void *g_sink_data = nullptr;

/* ---- Level names ---- */

static const char *const LEVEL_NAMES[] = {
    [IO_LOG_ERROR] = "ERROR",
    [IO_LOG_WARN] = "WARN",
    [IO_LOG_INFO] = "INFO",
    [IO_LOG_DEBUG] = "DEBUG",
};

const char *io_log_level_name(io_log_level_t level)
{
    if (level > IO_LOG_DEBUG) {
        return "UNKNOWN";
    }
    return LEVEL_NAMES[level];
}

/* ---- Default stderr sink ---- */

static void default_sink(io_log_level_t level, const char *module, const char *message,
                         [[maybe_unused]] void *user_data)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm;
    gmtime_r(&ts.tv_sec, &tm);

    long ms = ts.tv_nsec / 1000000L;

    fprintf(stderr, "[%-5s] %04d-%02d-%02dT%02d:%02d:%02d.%03ldZ %s: %s\n",
            io_log_level_name(level), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
            tm.tm_min, tm.tm_sec, ms, module != nullptr ? module : "unknown", message);
}

/* ---- Public API ---- */

void io_log_set_level(io_log_level_t level)
{
    g_min_level = level;
}

io_log_level_t io_log_get_level(void)
{
    return g_min_level;
}

void io_log_set_sink(io_log_sink_fn sink, void *user_data)
{
    g_sink = sink;
    g_sink_data = user_data;
}

void io_log(io_log_level_t level, const char *module, const char *fmt, ...)
{
    if (level > g_min_level) {
        return;
    }

    char buf[1024];
    buf[0] = '\0';

    if (fmt != nullptr) {
        va_list ap;
        va_start(ap, fmt);
        (void)vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
    }

    if (g_sink != nullptr) {
        g_sink(level, module != nullptr ? module : "unknown", buf, g_sink_data);
    } else {
        default_sink(level, module, buf, nullptr);
    }
}
