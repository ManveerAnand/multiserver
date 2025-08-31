#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "config.h"
#include "connection.h"
#include "enhanced_chat.h"

// Server statistics
typedef struct
{
    int total_connections;
    int active_connections;
    int http_requests;
    int chat_messages;
    time_t start_time;
    unsigned long bytes_sent;
    unsigned long bytes_received;
} ServerStats;

// Server structure
typedef struct
{
    ServerConfig *config;
    ConnectionPool *conn_pool;
    ServerStats stats;

    // Sockets
    int http_socket;
    int chat_socket;

    // Protocol handlers
    int (*http_handler)(Connection *conn);
    int (*chat_handler)(Connection *conn);
} Server;

// Function prototypes
Server *server_create(ServerConfig *config);
void server_destroy(Server *server);
int server_init_sockets(Server *server);
int server_run(Server *server);
void server_shutdown(Server *server);
ProtocolType server_detect_protocol(const char *data, size_t length);
int server_handle_new_connection(Server *server, int server_fd);
int server_handle_connection_read(Server *server, Connection *conn);
int server_handle_connection_write(Server *server, Connection *conn);
void server_print_stats(const Server *server);

// Signal handlers
void signal_handler(int signum);
void setup_signal_handlers(void);

#endif // SERVER_H
