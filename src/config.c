#include "config.h"
#include "logging.h"

void config_set_defaults(ServerConfig *config)
{
    if (!config)
        return;

    // Server settings
    config->http_port = 8080;
    config->chat_port = 8081;
    config->max_connections = 1000;
    strncpy(config->document_root, "./www", sizeof(config->document_root) - 1);

    // Logging settings
    config->log_level = LOG_INFO;
    strncpy(config->log_file, "./logs/multiserver.log", sizeof(config->log_file) - 1);
    config->log_to_console = true;
    config->log_to_file = true;

    // HTTP settings
    strncpy(config->default_page, "index.html", sizeof(config->default_page) - 1);
    config->directory_listing = false;
    config->gzip_compression = false;

    // Chat settings
    config->max_rooms = 100;
    config->max_users_per_room = 50;
    config->idle_timeout = 300; // 5 minutes

    // Security settings
    config->rate_limit_requests = 100;
    config->rate_limit_window = 60; // 1 minute
    config->enable_access_control = false;
}

static LogLevel parse_log_level(const char *level_str)
{
    if (strcasecmp(level_str, "DEBUG") == 0)
        return LOG_DEBUG;
    if (strcasecmp(level_str, "INFO") == 0)
        return LOG_INFO;
    if (strcasecmp(level_str, "WARN") == 0 || strcasecmp(level_str, "WARNING") == 0)
        return LOG_WARN;
    if (strcasecmp(level_str, "ERROR") == 0)
        return LOG_ERROR;
    if (strcasecmp(level_str, "FATAL") == 0)
        return LOG_FATAL;
    return LOG_INFO; // Default
}

static bool parse_bool(const char *value)
{
    if (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "1") == 0 || strcasecmp(value, "on") == 0)
    {
        return true;
    }
    return false;
}

static void trim_whitespace(char *str)
{
    char *start = str;
    char *end;

    // Trim leading whitespace
    while (*start == ' ' || *start == '\t')
        start++;

    // If all spaces
    if (*start == 0)
    {
        *str = 0;
        return;
    }

    // Trim trailing whitespace
    end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
    {
        end--;
    }
    end[1] = '\0';

    // Move trimmed string to beginning
    memmove(str, start, strlen(start) + 1);
}

int config_load(const char *filename, ServerConfig *config)
{
    FILE *file;
    char line[MAX_CONFIG_LINE];
    char section[64] = "";
    int line_number = 0;

    if (!config || !filename)
    {
        return -1;
    }

    // Set defaults first
    config_set_defaults(config);

    file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Warning: Could not open config file '%s', using defaults\n", filename);
        return 0; // Not fatal, we have defaults
    }

    while (fgets(line, sizeof(line), file))
    {
        line_number++;
        trim_whitespace(line);

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        // Check for section header
        if (line[0] == '[')
        {
            char *end = strchr(line, ']');
            if (end)
            {
                *end = '\0';
                strncpy(section, line + 1, sizeof(section) - 1);
                trim_whitespace(section);
                continue;
            }
            else
            {
                fprintf(stderr, "Config error at line %d: Invalid section header\n", line_number);
                continue;
            }
        }

        // Parse key=value pairs
        char *equals = strchr(line, '=');
        if (!equals)
        {
            fprintf(stderr, "Config error at line %d: Invalid key=value format\n", line_number);
            continue;
        }

        *equals = '\0';
        char *key = line;
        char *value = equals + 1;

        trim_whitespace(key);
        trim_whitespace(value);

        // Parse based on section
        if (strcmp(section, "server") == 0)
        {
            if (strcmp(key, "http_port") == 0)
            {
                config->http_port = atoi(value);
            }
            else if (strcmp(key, "chat_port") == 0)
            {
                config->chat_port = atoi(value);
            }
            else if (strcmp(key, "max_connections") == 0)
            {
                config->max_connections = atoi(value);
            }
            else if (strcmp(key, "document_root") == 0)
            {
                strncpy(config->document_root, value, sizeof(config->document_root) - 1);
            }
        }
        else if (strcmp(section, "logging") == 0)
        {
            if (strcmp(key, "level") == 0)
            {
                config->log_level = parse_log_level(value);
            }
            else if (strcmp(key, "file") == 0)
            {
                strncpy(config->log_file, value, sizeof(config->log_file) - 1);
            }
            else if (strcmp(key, "console") == 0)
            {
                config->log_to_console = parse_bool(value);
            }
            else if (strcmp(key, "to_file") == 0)
            {
                config->log_to_file = parse_bool(value);
            }
        }
        else if (strcmp(section, "http") == 0)
        {
            if (strcmp(key, "default_page") == 0)
            {
                strncpy(config->default_page, value, sizeof(config->default_page) - 1);
            }
            else if (strcmp(key, "directory_listing") == 0)
            {
                config->directory_listing = parse_bool(value);
            }
            else if (strcmp(key, "gzip_compression") == 0)
            {
                config->gzip_compression = parse_bool(value);
            }
        }
        else if (strcmp(section, "chat") == 0)
        {
            if (strcmp(key, "max_rooms") == 0)
            {
                config->max_rooms = atoi(value);
            }
            else if (strcmp(key, "max_users_per_room") == 0)
            {
                config->max_users_per_room = atoi(value);
            }
            else if (strcmp(key, "idle_timeout") == 0)
            {
                config->idle_timeout = atoi(value);
            }
        }
        else if (strcmp(section, "security") == 0)
        {
            if (strcmp(key, "rate_limit_requests") == 0)
            {
                config->rate_limit_requests = atoi(value);
            }
            else if (strcmp(key, "rate_limit_window") == 0)
            {
                config->rate_limit_window = atoi(value);
            }
            else if (strcmp(key, "enable_access_control") == 0)
            {
                config->enable_access_control = parse_bool(value);
            }
        }
    }

    fclose(file);
    return 0;
}

int config_validate(const ServerConfig *config)
{
    if (!config)
        return -1;

    // Validate port numbers
    if (config->http_port < 1 || config->http_port > 65535)
    {
        fprintf(stderr, "Invalid HTTP port: %d\n", config->http_port);
        return -1;
    }

    if (config->chat_port < 1 || config->chat_port > 65535)
    {
        fprintf(stderr, "Invalid chat port: %d\n", config->chat_port);
        return -1;
    }

    if (config->http_port == config->chat_port)
    {
        fprintf(stderr, "HTTP and chat ports cannot be the same\n");
        return -1;
    }

    // Validate connection limits
    if (config->max_connections < 1 || config->max_connections > 10000)
    {
        fprintf(stderr, "Invalid max connections: %d\n", config->max_connections);
        return -1;
    }

    // Validate document root
    struct stat st;
    if (stat(config->document_root, &st) != 0 || !S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "Document root is not a valid directory: %s\n", config->document_root);
        return -1;
    }

    return 0;
}

void config_print(const ServerConfig *config)
{
    if (!config)
        return;

    printf("=== Server Configuration ===\n");
    printf("HTTP Port: %d\n", config->http_port);
    printf("Chat Port: %d\n", config->chat_port);
    printf("Max Connections: %d\n", config->max_connections);
    printf("Document Root: %s\n", config->document_root);
    printf("Log Level: %d\n", config->log_level);
    printf("Log File: %s\n", config->log_file);
    printf("Log to Console: %s\n", config->log_to_console ? "yes" : "no");
    printf("Log to File: %s\n", config->log_to_file ? "yes" : "no");
    printf("Default Page: %s\n", config->default_page);
    printf("Directory Listing: %s\n", config->directory_listing ? "yes" : "no");
    printf("Max Rooms: %d\n", config->max_rooms);
    printf("Max Users per Room: %d\n", config->max_users_per_room);
    printf("Idle Timeout: %d seconds\n", config->idle_timeout);
    printf("=============================\n");
}

void config_free(ServerConfig *config)
{
    // Currently no dynamic allocations in config, but keep for future use
    (void)config;
}
