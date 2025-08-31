#include "logging.h"
#include <stdarg.h>
#include <pthread.h>

// Global logging state
static FILE *log_file_handle = NULL;
static LogLevel current_log_level = LOG_INFO;
static bool log_to_console_enabled = true;
static bool log_to_file_enabled = false;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log level strings and colors
const char *log_level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

const char *log_level_colors[] = {
    COLOR_GRAY,   // DEBUG
    COLOR_GREEN,  // INFO
    COLOR_YELLOW, // WARN
    COLOR_RED,    // ERROR
    COLOR_MAGENTA // FATAL
};

int logging_init(const ServerConfig *config)
{
    if (!config)
        return -1;

    pthread_mutex_lock(&log_mutex);

    current_log_level = config->log_level;
    log_to_console_enabled = config->log_to_console;
    log_to_file_enabled = config->log_to_file;

    // Open log file if file logging is enabled
    if (log_to_file_enabled)
    {
        // Create logs directory if it doesn't exist
        char *last_slash = strrchr(config->log_file, '/');
        if (last_slash)
        {
            char dir_path[PATH_MAX];
            size_t dir_len = last_slash - config->log_file;
            strncpy(dir_path, config->log_file, dir_len);
            dir_path[dir_len] = '\0';

            struct stat st;
            if (stat(dir_path, &st) != 0)
            {
                if (mkdir(dir_path, 0755) != 0)
                {
                    fprintf(stderr, "Failed to create log directory: %s\n", dir_path);
                    pthread_mutex_unlock(&log_mutex);
                    return -1;
                }
            }
        }

        log_file_handle = fopen(config->log_file, "a");
        if (!log_file_handle)
        {
            fprintf(stderr, "Failed to open log file: %s\n", config->log_file);
            log_to_file_enabled = false;
            pthread_mutex_unlock(&log_mutex);
            return -1;
        }

        // Set line buffering for immediate flush
        setvbuf(log_file_handle, NULL, _IOLBF, 0);
    }

    pthread_mutex_unlock(&log_mutex);

    log_info("Logging system initialized");
    log_info("Log level: %s", log_level_strings[current_log_level]);
    log_info("Console logging: %s", log_to_console_enabled ? "enabled" : "disabled");
    log_info("File logging: %s", log_to_file_enabled ? "enabled" : "disabled");

    return 0;
}

void logging_cleanup(void)
{
    pthread_mutex_lock(&log_mutex);

    if (log_file_handle)
    {
        log_info("Shutting down logging system");
        fclose(log_file_handle);
        log_file_handle = NULL;
    }

    pthread_mutex_unlock(&log_mutex);
}

const char *get_timestamp(void)
{
    static char timestamp[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    return timestamp;
}

void log_message(LogLevel level, const char *format, ...)
{
    if (level < current_log_level)
        return;

    va_list args;
    char message[1024];

    // Format the message
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    pthread_mutex_lock(&log_mutex);

    const char *timestamp = get_timestamp();
    const char *level_str = log_level_strings[level];

    // Log to console
    if (log_to_console_enabled)
    {
        const char *color = log_level_colors[level];
        printf("%s[%s] %s%s%s %s\n",
               COLOR_GRAY, timestamp,
               color, level_str, COLOR_RESET,
               message);
        fflush(stdout);
    }

    // Log to file
    if (log_to_file_enabled && log_file_handle)
    {
        fprintf(log_file_handle, "[%s] %s %s\n", timestamp, level_str, message);
        fflush(log_file_handle);
    }

    pthread_mutex_unlock(&log_mutex);

    // Exit on fatal errors
    if (level == LOG_FATAL)
    {
        exit(EXIT_FAILURE);
    }
}

void log_raw(LogLevel level, const char *message)
{
    if (level < current_log_level)
        return;

    pthread_mutex_lock(&log_mutex);

    const char *timestamp = get_timestamp();
    const char *level_str = log_level_strings[level];

    // Log to console
    if (log_to_console_enabled)
    {
        const char *color = log_level_colors[level];
        printf("%s[%s] %s%s%s %s\n",
               COLOR_GRAY, timestamp,
               color, level_str, COLOR_RESET,
               message);
        fflush(stdout);
    }

    // Log to file
    if (log_to_file_enabled && log_file_handle)
    {
        fprintf(log_file_handle, "[%s] %s %s\n", timestamp, level_str, message);
        fflush(log_file_handle);
    }

    pthread_mutex_unlock(&log_mutex);

    // Exit on fatal errors
    if (level == LOG_FATAL)
    {
        exit(EXIT_FAILURE);
    }
}
