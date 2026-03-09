/**
 * @file io_log.h
 * @brief Structured logging with level filtering and custom sinks.
 */

#ifndef IOHTTP_CORE_LOG_H
#define IOHTTP_CORE_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum : uint8_t {
    IO_LOG_ERROR = 0,
    IO_LOG_WARN = 1,
    IO_LOG_INFO = 2,
    IO_LOG_DEBUG = 3,
} io_log_level_t;

/**
 * @brief Custom log sink callback.
 * @param level   Log level of the message.
 * @param module  Module name (e.g. "server", "tls").
 * @param message Formatted message string.
 * @param user_data Opaque pointer from io_log_set_sink().
 */
typedef void (*io_log_sink_fn)(io_log_level_t level, const char *module, const char *message,
                               void *user_data);

/**
 * @brief Set the minimum log level. Messages below this level are filtered.
 * @param level Minimum level to emit (default IO_LOG_INFO).
 */
void io_log_set_level(io_log_level_t level);

/**
 * @brief Get the current minimum log level.
 * @return Current minimum level.
 */
io_log_level_t io_log_get_level(void);

/**
 * @brief Set a custom log sink. Pass nullptr to revert to default stderr output.
 * @param sink      Callback function.
 * @param user_data Opaque pointer passed to the callback.
 */
void io_log_set_sink(io_log_sink_fn sink, void *user_data);

/**
 * @brief Emit a log message at the given level.
 * @param level  Log level.
 * @param module Module name (e.g. "server").
 * @param fmt    printf-style format string.
 */
void io_log(io_log_level_t level, const char *module, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * @brief Return the human-readable name for a log level.
 * @param level Log level.
 * @return Static string: "ERROR", "WARN", "INFO", or "DEBUG".
 */
const char *io_log_level_name(io_log_level_t level);

/* ---- Convenience macros ---- */

#define IO_LOG_ERROR(mod, ...) io_log(IO_LOG_ERROR, (mod), __VA_ARGS__)
#define IO_LOG_WARN(mod, ...)  io_log(IO_LOG_WARN, (mod), __VA_ARGS__)
#define IO_LOG_INFO(mod, ...)  io_log(IO_LOG_INFO, (mod), __VA_ARGS__)
#define IO_LOG_DEBUG(mod, ...) io_log(IO_LOG_DEBUG, (mod), __VA_ARGS__)

#endif /* IOHTTP_CORE_LOG_H */
