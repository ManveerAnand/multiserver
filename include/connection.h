#ifndef CONNECTION_H
#define CONNECTION_H

#include "common.h"

// Connection state
typedef enum
{
    CONN_STATE_NEW = 0,
    CONN_STATE_READING,
    CONN_STATE_PROCESSING,
    CONN_STATE_WRITING,
    CONN_STATE_CLOSING
} ConnectionState;

// Connection structure
typedef struct
{
    int fd;                    // Socket file descriptor
    char ip[INET6_ADDRSTRLEN]; // Client IP address
    int port;                  // Client port
    time_t connected_at;       // Connection timestamp
    time_t last_activity;      // Last activity timestamp
    ProtocolType protocol;     // Detected protocol
    ConnectionState state;     // Current connection state

    // Buffers
    char *read_buffer;       // Read buffer
    size_t read_buffer_size; // Read buffer size
    size_t read_buffer_used; // Bytes used in read buffer

    char *write_buffer;       // Write buffer
    size_t write_buffer_size; // Write buffer size
    size_t write_buffer_used; // Bytes used in write buffer
    size_t write_buffer_sent; // Bytes already sent

    // Protocol-specific data
    void *protocol_data;          // Protocol-specific data pointer
    void (*cleanup_func)(void *); // Cleanup function for protocol data

    // Flags
    bool keep_alive;       // Keep connection alive
    bool has_data_to_send; // Has data waiting to be sent
} Connection;

// Connection pool structure
typedef struct
{
    Connection **connections; // Array of connection pointers
    int max_connections;      // Maximum connections allowed
    int active_connections;   // Currently active connections
    int total_connections;    // Total connections served
} ConnectionPool;

// Function prototypes
ConnectionPool *connection_pool_create(int max_connections);
void connection_pool_destroy(ConnectionPool *pool);
Connection *connection_create(int fd, struct sockaddr_in *client_addr);
void connection_destroy(Connection *conn);
int connection_pool_add(ConnectionPool *pool, Connection *conn);
void connection_pool_remove(ConnectionPool *pool, Connection *conn);
Connection *connection_pool_find_by_fd(ConnectionPool *pool, int fd);
void connection_pool_cleanup_idle(ConnectionPool *pool, int timeout);
int connection_read(Connection *conn);
int connection_write(Connection *conn);
void connection_set_protocol_data(Connection *conn, void *data, void (*cleanup)(void *));
void connection_prepare_response(Connection *conn, const char *data, size_t length);

#endif // CONNECTION_H
