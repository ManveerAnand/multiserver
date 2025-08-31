#include "server.h"
#include "logging.h"

// Global variables for signal handling
volatile sig_atomic_t running = 1;
volatile sig_atomic_t reload_config = 0;

// Enhanced HTTP handler
static int simple_http_handler(Connection *conn)
{
    if (!conn || conn->read_buffer_used == 0)
        return 0;

    // Try to serve the index.html file
    char filepath[PATH_MAX];
    snprintf(filepath, sizeof(filepath), "./www/index.html");

    FILE *file = fopen(filepath, "r");
    if (file)
    {
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Read file content
        char *file_content = malloc(file_size + 1);
        if (file_content)
        {
            size_t bytes_read = fread(file_content, 1, file_size, file);
            file_content[bytes_read] = '\0';

            // Build HTTP response
            char response[BUFFER_SIZE * 4];
            snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html; charset=utf-8\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "Server: MultiServer/1.0.0\r\n"
                     "\r\n"
                     "%s",
                     file_size, file_content);

            connection_prepare_response(conn, response, strlen(response));
            free(file_content);
            fclose(file);
        }
        else
        {
            fclose(file);
            goto fallback;
        }
    }
    else
    {
    fallback:
        // Fallback response
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 51\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body><h1>MultiServer Working!</h1></body></html>";

        connection_prepare_response(conn, response, strlen(response));
    }

    conn->state = CONN_STATE_WRITING;
    log_info("Served HTTP request from %s:%d", conn->ip, conn->port);
    return 1;
}

// Enhanced chat handler
static int simple_chat_handler(Connection *conn)
{
    if (!conn || conn->read_buffer_used == 0)
        return 0;

    // Clean up the input (remove newlines)
    char *message = conn->read_buffer;
    char *newline = strchr(message, '\n');
    if (newline)
        *newline = '\0';

    char response[BUFFER_SIZE];

    // Handle different chat commands
    if (strncmp(message, "HELP", 4) == 0)
    {
        snprintf(response, sizeof(response),
                 "=== MultiServer Chat Commands ===\n"
                 "HELP     - Show this help\n"
                 "TIME     - Show current time\n"
                 "STATUS   - Show server status\n"
                 "ECHO <msg> - Echo your message\n"
                 "QUIT     - Disconnect\n"
                 "================================\n");
    }
    else if (strncmp(message, "TIME", 4) == 0)
    {
        time_t now = time(NULL);
        snprintf(response, sizeof(response), "Server time: %s", ctime(&now));
    }
    else if (strncmp(message, "STATUS", 6) == 0)
    {
        snprintf(response, sizeof(response),
                 "MultiServer Status:\n"
                 "- HTTP Port: 8080\n"
                 "- Chat Port: 8081\n"
                 "- Your IP: %s:%d\n"
                 "- Connected at: %s",
                 conn->ip, conn->port, ctime(&conn->connected_at));
    }
    else if (strncmp(message, "QUIT", 4) == 0)
    {
        snprintf(response, sizeof(response), "Goodbye! Disconnecting...\n");
        connection_prepare_response(conn, response, strlen(response));
        conn->state = CONN_STATE_CLOSING;
        return 1;
    }
    else if (strncmp(message, "ECHO ", 5) == 0)
    {
        snprintf(response, sizeof(response), "ECHO: %s\n", message + 5);
    }
    else
    {
        snprintf(response, sizeof(response),
                 "Unknown command: '%s'\nType 'HELP' for available commands.\n", message);
    }

    connection_prepare_response(conn, response, strlen(response));
    conn->state = CONN_STATE_WRITING;

    log_info("Handled chat command '%s' from %s:%d", message, conn->ip, conn->port);
    return 1;
}

Server *server_create(ServerConfig *config)
{
    Server *server = malloc(sizeof(Server));
    if (!server)
    {
        log_error("Failed to allocate server");
        return NULL;
    }

    memset(server, 0, sizeof(Server));
    server->config = config;

    // Create connection pool
    server->conn_pool = connection_pool_create(config->max_connections);
    if (!server->conn_pool)
    {
        free(server);
        return NULL;
    }

    // Initialize statistics
    server->stats.start_time = time(NULL);

    // Set protocol handlers
    server->http_handler = simple_http_handler;
    server->chat_handler = simple_chat_handler;

    server->http_socket = -1;
    server->chat_socket = -1;

    log_info("Server created successfully");
    return server;
}

void server_destroy(Server *server)
{
    if (!server)
        return;

    log_info("Destroying server");

    if (server->http_socket >= 0)
    {
        close(server->http_socket);
    }

    if (server->chat_socket >= 0)
    {
        close(server->chat_socket);
    }

    connection_pool_destroy(server->conn_pool);
    free(server);
}

static int create_server_socket(int port, const char *name)
{
    int sockfd;
    struct sockaddr_in server_addr;
    int opt = 1;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        log_error("Failed to create %s socket: %s", name, strerror(errno));
        return -1;
    }

    // Set socket options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        log_error("Failed to set SO_REUSEADDR for %s socket: %s", name, strerror(errno));
        close(sockfd);
        return -1;
    }

    // Make socket non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        log_error("Failed to set non-blocking mode for %s socket: %s", name, strerror(errno));
        close(sockfd);
        return -1;
    }

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        log_error("Failed to bind %s socket to port %d: %s", name, port, strerror(errno));
        close(sockfd);
        return -1;
    }

    // Listen for connections
    if (listen(sockfd, 128) < 0)
    {
        log_error("Failed to listen on %s socket: %s", name, strerror(errno));
        close(sockfd);
        return -1;
    }

    log_info("%s server listening on port %d", name, port);
    return sockfd;
}

int server_init_sockets(Server *server)
{
    if (!server)
        return -1;

    // Create HTTP socket
    server->http_socket = create_server_socket(server->config->http_port, "HTTP");
    if (server->http_socket < 0)
    {
        return -1;
    }

    // Create chat socket
    server->chat_socket = create_server_socket(server->config->chat_port, "Chat");
    if (server->chat_socket < 0)
    {
        close(server->http_socket);
        server->http_socket = -1;
        return -1;
    }

    return 0;
}

ProtocolType server_detect_protocol(const char *data, size_t length)
{
    if (length < 3)
        return PROTOCOL_UNKNOWN;

    // Check for HTTP methods
    if (strncmp(data, "GET ", 4) == 0 ||
        strncmp(data, "POST ", 5) == 0 ||
        strncmp(data, "HEAD ", 5) == 0 ||
        strncmp(data, "PUT ", 4) == 0 ||
        strncmp(data, "DELETE ", 7) == 0 ||
        strncmp(data, "OPTIONS ", 8) == 0)
    {
        return PROTOCOL_HTTP;
    }

    // Check for chat protocol (simple text-based)
    if (strncmp(data, "CHAT ", 5) == 0 ||
        strncmp(data, "JOIN ", 5) == 0 ||
        strncmp(data, "MSG ", 4) == 0)
    {
        return PROTOCOL_CHAT;
    }

    // Default to HTTP for unknown protocols
    return PROTOCOL_HTTP;
}

int server_handle_new_connection(Server *server, int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            log_error("Accept failed: %s", strerror(errno));
        }
        return -1;
    }

    // Make client socket non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        log_error("Failed to set non-blocking mode for client socket: %s", strerror(errno));
        close(client_fd);
        return -1;
    }

    // Create connection
    Connection *conn = connection_create(client_fd, &client_addr);
    if (!conn)
    {
        close(client_fd);
        return -1;
    }

    // Set protocol based on which server socket accepted the connection
    if (server_fd == server->http_socket)
    {
        conn->protocol = PROTOCOL_HTTP;
        log_debug("Connection from %s:%d assigned to HTTP protocol", conn->ip, conn->port);
    }
    else if (server_fd == server->chat_socket)
    {
        conn->protocol = PROTOCOL_CHAT;
        log_debug("Connection from %s:%d assigned to CHAT protocol", conn->ip, conn->port);
    }

    // Add to connection pool
    if (connection_pool_add(server->conn_pool, conn) < 0)
    {
        connection_destroy(conn);
        return -1;
    }

    server->stats.total_connections++;

    log_info("New connection from %s:%d (fd=%d, protocol=%s)",
             conn->ip, conn->port, client_fd,
             conn->protocol == PROTOCOL_HTTP ? "HTTP" : "CHAT");
    return 0;
}

int server_handle_connection_read(Server *server, Connection *conn)
{
    int bytes_read = connection_read(conn);
    if (bytes_read < 0)
    {
        // Error reading
        return -1;
    }
    else if (bytes_read == 0)
    {
        // Connection closed or no data
        return 0;
    }

    // If protocol not detected yet, try to detect it
    if (conn->protocol == PROTOCOL_UNKNOWN)
    {
        conn->protocol = server_detect_protocol(conn->read_buffer, conn->read_buffer_used);
        log_debug("Detected protocol %d for connection %s:%d",
                  conn->protocol, conn->ip, conn->port);
    }

    // Handle based on protocol
    switch (conn->protocol)
    {
    case PROTOCOL_HTTP:
        if (server->http_handler)
        {
            return server->http_handler(conn);
        }
        break;

    case PROTOCOL_CHAT:
        // Use enhanced chat system
        log_debug("Processing chat data: '%.*s'", (int)conn->read_buffer_used, conn->read_buffer);
        return enhanced_chat_handler(chat_get_server(), conn);

    default:
        log_warn("Unknown protocol for connection %s:%d", conn->ip, conn->port);
        return -1;
    }

    return 0;
}

int server_handle_connection_write(Server *server, Connection *conn)
{
    (void)server; // Unused parameter

    int bytes_sent = connection_write(conn);
    if (bytes_sent < 0)
    {
        return -1;
    }

    // If all data sent and not keep-alive, mark for closing
    if (!conn->has_data_to_send && !conn->keep_alive)
    {
        conn->state = CONN_STATE_CLOSING;
    }

    return bytes_sent;
}

int server_run(Server *server)
{
    if (!server)
        return -1;

    log_info("Starting server main loop");

    fd_set read_fds, write_fds;
    int max_fd;
    struct timeval timeout;

    while (running)
    {
        // Handle config reload signal
        if (reload_config)
        {
            log_info("Config reload requested (not implemented yet)");
            reload_config = 0;
        }

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        // Add server sockets to read set
        FD_SET(server->http_socket, &read_fds);
        FD_SET(server->chat_socket, &read_fds);
        max_fd = (server->http_socket > server->chat_socket) ? server->http_socket : server->chat_socket;

        // Add client connections to appropriate sets
        for (int i = 0; i < server->conn_pool->max_connections; i++)
        {
            Connection *conn = server->conn_pool->connections[i];
            if (conn)
            {
                if (conn->state != CONN_STATE_CLOSING)
                {
                    FD_SET(conn->fd, &read_fds);
                }
                if (conn->has_data_to_send)
                {
                    FD_SET(conn->fd, &write_fds);
                }
                if (conn->fd > max_fd)
                {
                    max_fd = conn->fd;
                }
            }
        }

        // Set timeout for periodic cleanup
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
            {
                continue; // Interrupted by signal
            }
            log_error("Select error: %s", strerror(errno));
            break;
        }

        // Handle new connections
        if (FD_ISSET(server->http_socket, &read_fds))
        {
            server_handle_new_connection(server, server->http_socket);
        }

        if (FD_ISSET(server->chat_socket, &read_fds))
        {
            server_handle_new_connection(server, server->chat_socket);
        }

        // Handle existing connections
        for (int i = 0; i < server->conn_pool->max_connections; i++)
        {
            Connection *conn = server->conn_pool->connections[i];
            if (!conn)
                continue;

            bool should_close = false;

            // Handle read events
            if (FD_ISSET(conn->fd, &read_fds))
            {
                if (server_handle_connection_read(server, conn) < 0)
                {
                    should_close = true;
                }
            }

            // Handle write events
            if (!should_close && FD_ISSET(conn->fd, &write_fds))
            {
                if (server_handle_connection_write(server, conn) < 0)
                {
                    should_close = true;
                }
            }

            // Close connection if needed
            if (should_close || conn->state == CONN_STATE_CLOSING)
            {
                connection_pool_remove(server->conn_pool, conn);
            }
        }

        // Periodic cleanup of idle connections
        static time_t last_cleanup = 0;
        time_t now = time(NULL);
        if (now - last_cleanup > 60)
        { // Cleanup every minute
            connection_pool_cleanup_idle(server->conn_pool, server->config->idle_timeout);
            last_cleanup = now;
        }
    }

    log_info("Server main loop terminated");
    return 0;
}

void server_shutdown(Server *server)
{
    if (!server)
        return;

    log_info("Shutting down server");
    running = 0;
}

void server_print_stats(const Server *server)
{
    if (!server)
        return;

    time_t uptime = time(NULL) - server->stats.start_time;

    log_info("=== Server Statistics ===");
    log_info("Uptime: %ld seconds", uptime);
    log_info("Total connections: %d", server->stats.total_connections);
    log_info("Active connections: %d", server->conn_pool->active_connections);
    log_info("HTTP requests: %d", server->stats.http_requests);
    log_info("Chat messages: %d", server->stats.chat_messages);
    log_info("Bytes sent: %lu", server->stats.bytes_sent);
    log_info("Bytes received: %lu", server->stats.bytes_received);
    log_info("========================");
}

// Signal handlers
void signal_handler(int signum)
{
    switch (signum)
    {
    case SIGTERM:
    case SIGINT:
        log_info("Received shutdown signal");
        running = 0;
        break;
    case SIGHUP:
        log_info("Received config reload signal");
        reload_config = 1;
        break;
    default:
        break;
    }
}

void setup_signal_handlers(void)
{
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    // Ignore SIGPIPE (broken pipe)
    signal(SIGPIPE, SIG_IGN);
}
