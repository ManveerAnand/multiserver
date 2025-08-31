#include "connection.h"
#include "logging.h"

ConnectionPool *connection_pool_create(int max_connections)
{
    ConnectionPool *pool = malloc(sizeof(ConnectionPool));
    if (!pool)
    {
        log_error("Failed to allocate connection pool");
        return NULL;
    }

    pool->connections = calloc(max_connections, sizeof(Connection *));
    if (!pool->connections)
    {
        log_error("Failed to allocate connection array");
        free(pool);
        return NULL;
    }

    pool->max_connections = max_connections;
    pool->active_connections = 0;
    pool->total_connections = 0;

    log_info("Connection pool created with max %d connections", max_connections);
    return pool;
}

void connection_pool_destroy(ConnectionPool *pool)
{
    if (!pool)
        return;

    // Close all active connections
    for (int i = 0; i < pool->max_connections; i++)
    {
        if (pool->connections[i])
        {
            connection_destroy(pool->connections[i]);
        }
    }

    free(pool->connections);
    free(pool);
    log_info("Connection pool destroyed");
}

Connection *connection_create(int fd, struct sockaddr_in *client_addr)
{
    Connection *conn = malloc(sizeof(Connection));
    if (!conn)
    {
        log_error("Failed to allocate connection");
        return NULL;
    }

    memset(conn, 0, sizeof(Connection));

    conn->fd = fd;
    conn->connected_at = time(NULL);
    conn->last_activity = conn->connected_at;
    conn->protocol = PROTOCOL_UNKNOWN;
    conn->state = CONN_STATE_NEW;

    // Convert client address to string
    inet_ntop(AF_INET, &client_addr->sin_addr, conn->ip, sizeof(conn->ip));
    conn->port = ntohs(client_addr->sin_port);

    // Allocate buffers
    conn->read_buffer_size = BUFFER_SIZE;
    conn->read_buffer = malloc(conn->read_buffer_size);
    if (!conn->read_buffer)
    {
        log_error("Failed to allocate read buffer");
        free(conn);
        return NULL;
    }

    conn->write_buffer_size = BUFFER_SIZE;
    conn->write_buffer = malloc(conn->write_buffer_size);
    if (!conn->write_buffer)
    {
        log_error("Failed to allocate write buffer");
        free(conn->read_buffer);
        free(conn);
        return NULL;
    }

    conn->keep_alive = false;
    conn->has_data_to_send = false;

    log_debug("Connection created for %s:%d (fd=%d)", conn->ip, conn->port, conn->fd);
    return conn;
}

void connection_destroy(Connection *conn)
{
    if (!conn)
        return;

    log_debug("Destroying connection %s:%d (fd=%d)", conn->ip, conn->port, conn->fd);

    // Close socket
    if (conn->fd >= 0)
    {
        close(conn->fd);
    }

    // Clean up protocol-specific data
    if (conn->protocol_data && conn->cleanup_func)
    {
        conn->cleanup_func(conn->protocol_data);
    }

    // Free buffers
    free(conn->read_buffer);
    free(conn->write_buffer);

    free(conn);
}

int connection_pool_add(ConnectionPool *pool, Connection *conn)
{
    if (!pool || !conn)
        return -1;

    if (pool->active_connections >= pool->max_connections)
    {
        log_warn("Connection pool full, rejecting connection from %s:%d",
                 conn->ip, conn->port);
        return -1;
    }

    // Find empty slot
    for (int i = 0; i < pool->max_connections; i++)
    {
        if (pool->connections[i] == NULL)
        {
            pool->connections[i] = conn;
            pool->active_connections++;
            pool->total_connections++;

            log_debug("Connection added to pool at slot %d (%s:%d)",
                      i, conn->ip, conn->port);
            return i;
        }
    }

    log_error("No empty slot found in connection pool");
    return -1;
}

void connection_pool_remove(ConnectionPool *pool, Connection *conn)
{
    if (!pool || !conn)
        return;

    for (int i = 0; i < pool->max_connections; i++)
    {
        if (pool->connections[i] == conn)
        {
            pool->connections[i] = NULL;
            pool->active_connections--;

            log_debug("Connection removed from pool slot %d (%s:%d)",
                      i, conn->ip, conn->port);

            connection_destroy(conn);
            return;
        }
    }

    log_warn("Connection not found in pool for removal");
}

Connection *connection_pool_find_by_fd(ConnectionPool *pool, int fd)
{
    if (!pool)
        return NULL;

    for (int i = 0; i < pool->max_connections; i++)
    {
        if (pool->connections[i] && pool->connections[i]->fd == fd)
        {
            return pool->connections[i];
        }
    }

    return NULL;
}

void connection_pool_cleanup_idle(ConnectionPool *pool, int timeout)
{
    if (!pool)
        return;

    time_t now = time(NULL);
    int cleaned = 0;

    for (int i = 0; i < pool->max_connections; i++)
    {
        Connection *conn = pool->connections[i];
        if (conn && (now - conn->last_activity) > timeout)
        {
            log_debug("Cleaning up idle connection %s:%d", conn->ip, conn->port);
            connection_pool_remove(pool, conn);
            cleaned++;
        }
    }

    if (cleaned > 0)
    {
        log_info("Cleaned up %d idle connections", cleaned);
    }
}

int connection_read(Connection *conn)
{
    if (!conn)
        return -1;

    // Ensure we have space in the buffer
    if (conn->read_buffer_used >= conn->read_buffer_size - 1)
    {
        // Buffer is full, we might need to expand it or this is an error
        log_warn("Read buffer full for connection %s:%d", conn->ip, conn->port);
        return -1;
    }

    ssize_t bytes_read = recv(conn->fd,
                              conn->read_buffer + conn->read_buffer_used,
                              conn->read_buffer_size - conn->read_buffer_used - 1,
                              0);

    if (bytes_read > 0)
    {
        conn->read_buffer_used += bytes_read;
        conn->read_buffer[conn->read_buffer_used] = '\0'; // Null terminate
        conn->last_activity = time(NULL);

        log_debug("Read %zd bytes from %s:%d", bytes_read, conn->ip, conn->port);
        return bytes_read;
    }
    else if (bytes_read == 0)
    {
        // Connection closed by client
        log_debug("Connection closed by client %s:%d", conn->ip, conn->port);
        return 0;
    }
    else
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // No data available right now
            return 0;
        }
        else
        {
            log_error("Read error from %s:%d: %s", conn->ip, conn->port, strerror(errno));
            return -1;
        }
    }
}

int connection_write(Connection *conn)
{
    if (!conn || !conn->has_data_to_send)
        return 0;

    size_t remaining = conn->write_buffer_used - conn->write_buffer_sent;
    if (remaining == 0)
    {
        conn->has_data_to_send = false;
        return 0;
    }

    ssize_t bytes_sent = send(conn->fd,
                              conn->write_buffer + conn->write_buffer_sent,
                              remaining,
                              MSG_NOSIGNAL);

    if (bytes_sent > 0)
    {
        conn->write_buffer_sent += bytes_sent;
        conn->last_activity = time(NULL);

        log_debug("Sent %zd bytes to %s:%d", bytes_sent, conn->ip, conn->port);

        // Check if all data has been sent
        if (conn->write_buffer_sent >= conn->write_buffer_used)
        {
            conn->has_data_to_send = false;
            conn->write_buffer_used = 0;
            conn->write_buffer_sent = 0;
        }

        return bytes_sent;
    }
    else if (bytes_sent == 0)
    {
        // No data sent
        return 0;
    }
    else
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Socket buffer full, try again later
            return 0;
        }
        else
        {
            log_error("Write error to %s:%d: %s", conn->ip, conn->port, strerror(errno));
            return -1;
        }
    }
}

void connection_set_protocol_data(Connection *conn, void *data, void (*cleanup)(void *))
{
    if (!conn)
        return;

    // Clean up existing data
    if (conn->protocol_data && conn->cleanup_func)
    {
        conn->cleanup_func(conn->protocol_data);
    }

    conn->protocol_data = data;
    conn->cleanup_func = cleanup;
}

void connection_prepare_response(Connection *conn, const char *data, size_t length)
{
    if (!conn || !data || length == 0)
        return;

    // Ensure we have enough space in the write buffer
    if (length > conn->write_buffer_size)
    {
        // Expand buffer if needed
        char *new_buffer = realloc(conn->write_buffer, length);
        if (!new_buffer)
        {
            log_error("Failed to expand write buffer for %s:%d", conn->ip, conn->port);
            return;
        }
        conn->write_buffer = new_buffer;
        conn->write_buffer_size = length;
    }

    // Reset buffer state
    conn->write_buffer_used = 0;
    conn->write_buffer_sent = 0;

    // Copy data to buffer
    memcpy(conn->write_buffer, data, length);
    conn->write_buffer_used = length;
    conn->has_data_to_send = true;

    log_debug("Prepared %zu bytes for sending to %s:%d", length, conn->ip, conn->port);
}
