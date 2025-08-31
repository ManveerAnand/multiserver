#ifndef LOGGING_H
#define LOGGING_H

#include "common.h"
#include "config.h"

// ANSI color codes for console output
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_GRAY "\033[90m"

// Log level strings
extern const char *log_level_strings[];
extern const char *log_level_colors[];

// Function prototypes
int logging_init(const ServerConfig *config);
void logging_cleanup(void);
void log_message(LogLevel level, const char *format, ...);
void log_raw(LogLevel level, const char *message);
const char *get_timestamp(void);

// Convenience macros
#define log_debug(fmt, ...) log_message(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  log_message(LOG_INFO, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  log_message(LOG_WARN, fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log_message(LOG_ERROR, fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) log_message(LOG_FATAL, fmt, ##__VA_ARGS__)

#endif // LOGGING_H
