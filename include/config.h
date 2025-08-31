#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"

// Server configuration structure
typedef struct
{
    // Server settings
    int http_port;
    int chat_port;
    int max_connections;
    char document_root[PATH_MAX];

    // Logging settings
    int log_level;
    char log_file[PATH_MAX];
    bool log_to_console;
    bool log_to_file;

    // HTTP settings
    char default_page[256];
    bool directory_listing;
    bool gzip_compression;

    // Chat settings
    int max_rooms;
    int max_users_per_room;
    int idle_timeout;

    // Security settings
    int rate_limit_requests;
    int rate_limit_window;
    bool enable_access_control;
} ServerConfig;

// Function prototypes
int config_load(const char *filename, ServerConfig *config);
void config_set_defaults(ServerConfig *config);
int config_validate(const ServerConfig *config);
void config_print(const ServerConfig *config);
void config_free(ServerConfig *config);

#endif // CONFIG_H
