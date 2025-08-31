#include "enhanced_chat.h"
#include "logging.h"

// Global chat server instance
static ChatServer *global_chat_server = NULL;

ChatServer *chat_server_create(void)
{
    ChatServer *server = malloc(sizeof(ChatServer));
    if (!server)
    {
        log_error("Failed to allocate chat server");
        return NULL;
    }

    memset(server, 0, sizeof(ChatServer));
    server->start_time = time(NULL);

    // Create default lobby room
    ChatRoom *lobby = chat_room_create("lobby");
    if (lobby)
    {
        strcpy(lobby->topic, "Welcome to MultiServer Chat! Type /help for commands.");
        server->rooms[0] = lobby;
        server->room_count = 1;
    }

    log_info("Enhanced chat server created with lobby room");
    return server;
}

void chat_server_destroy(ChatServer *server)
{
    if (!server)
        return;

    // Destroy all rooms
    for (int i = 0; i < server->room_count; i++)
    {
        if (server->rooms[i])
        {
            chat_room_destroy(server->rooms[i]);
        }
    }

    // Destroy all users
    for (int i = 0; i < server->user_count; i++)
    {
        if (server->users[i])
        {
            chat_user_destroy(server->users[i]);
        }
    }

    free(server);
    log_info("Chat server destroyed");
}

ChatUser *chat_user_create(Connection *conn)
{
    ChatUser *user = malloc(sizeof(ChatUser));
    if (!user)
    {
        log_error("Failed to allocate chat user");
        return NULL;
    }

    memset(user, 0, sizeof(ChatUser));
    user->connection = conn;
    user->join_time = time(NULL);
    user->last_activity = user->join_time;
    user->authenticated = false;
    user->is_admin = false;
    user->current_room = NULL;

    // Generate default nickname
    snprintf(user->nickname, sizeof(user->nickname), "User%d", (int)(user->join_time % 10000));

    return user;
}

void chat_user_destroy(ChatUser *user)
{
    if (!user)
        return;

    // Leave current room if in one
    if (user->current_room)
    {
        chat_leave_room(user);
    }

    free(user);
}

ChatRoom *chat_room_create(const char *name)
{
    ChatRoom *room = malloc(sizeof(ChatRoom));
    if (!room)
    {
        log_error("Failed to allocate chat room");
        return NULL;
    }

    memset(room, 0, sizeof(ChatRoom));
    strncpy(room->name, name, sizeof(room->name) - 1);
    room->created_at = time(NULL);
    room->password_protected = false;
    room->private_room = false;

    log_info("Created chat room: %s", name);
    return room;
}

void chat_room_destroy(ChatRoom *room)
{
    if (!room)
        return;

    // Remove all users from room
    for (int i = 0; i < room->user_count; i++)
    {
        if (room->users[i])
        {
            room->users[i]->current_room = NULL;
        }
    }

    log_info("Destroyed chat room: %s", room->name);
    free(room);
}

ChatUser *chat_find_user_by_nickname(ChatServer *server, const char *nickname)
{
    for (int i = 0; i < server->user_count; i++)
    {
        if (server->users[i] && strcmp(server->users[i]->nickname, nickname) == 0)
        {
            return server->users[i];
        }
    }
    return NULL;
}

ChatUser *chat_find_user_by_connection(ChatServer *server, Connection *conn)
{
    for (int i = 0; i < server->user_count; i++)
    {
        if (server->users[i] && server->users[i]->connection == conn)
        {
            return server->users[i];
        }
    }
    return NULL;
}

ChatRoom *chat_find_room(ChatServer *server, const char *name)
{
    for (int i = 0; i < server->room_count; i++)
    {
        if (server->rooms[i] && strcmp(server->rooms[i]->name, name) == 0)
        {
            return server->rooms[i];
        }
    }
    return NULL;
}

int chat_join_room(ChatServer *server, ChatUser *user, const char *room_name, const char *password)
{
    ChatRoom *room = chat_find_room(server, room_name);

    // Create room if it doesn't exist
    if (!room)
    {
        if (server->room_count >= MAX_ROOMS)
        {
            chat_send_system_message(user, "Cannot create room: Maximum rooms reached");
            return -1;
        }

        room = chat_room_create(room_name);
        if (!room)
        {
            chat_send_system_message(user, "Failed to create room");
            return -1;
        }

        server->rooms[server->room_count++] = room;
    }

    // Check password if room is protected
    if (room->password_protected && password && strcmp(room->password, password) != 0)
    {
        chat_send_system_message(user, "Incorrect room password");
        return -1;
    }

    // Check room capacity
    if (room->user_count >= MAX_USERS_PER_ROOM)
    {
        chat_send_system_message(user, "Room is full");
        return -1;
    }

    // Leave current room if in one
    if (user->current_room)
    {
        chat_leave_room(user);
    }

    // Add user to room
    room->users[room->user_count++] = user;
    user->current_room = room;

    // Send welcome messages
    char message[512];
    snprintf(message, sizeof(message), "*** %s joined the room", user->nickname);
    chat_announce_to_room(room, message);

    snprintf(message, sizeof(message),
             "Welcome to #%s!\nTopic: %s\nUsers online: %d\nType /list users to see who's here.",
             room->name, room->topic, room->user_count);
    chat_send_system_message(user, message);

    log_info("User %s joined room %s", user->nickname, room->name);
    return 0;
}

int chat_leave_room(ChatUser *user)
{
    if (!user->current_room)
    {
        chat_send_system_message(user, "You are not in a room");
        return -1;
    }

    ChatRoom *room = user->current_room;

    // Remove user from room
    for (int i = 0; i < room->user_count; i++)
    {
        if (room->users[i] == user)
        {
            // Shift remaining users
            for (int j = i; j < room->user_count - 1; j++)
            {
                room->users[j] = room->users[j + 1];
            }
            room->user_count--;
            break;
        }
    }

    // Announce departure
    char message[256];
    snprintf(message, sizeof(message), "*** %s left the room", user->nickname);
    chat_announce_to_room(room, message);

    user->current_room = NULL;
    chat_send_system_message(user, "You left the room");

    log_info("User %s left room %s", user->nickname, room->name);
    return 0;
}

void chat_send_system_message(ChatUser *user, const char *message)
{
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "*** %s\n", message);
    connection_prepare_response(user->connection, response, strlen(response));
    connection_write(user->connection);
}

void chat_broadcast_to_room(ChatRoom *room, const char *message, ChatUser *sender)
{
    if (!room || !sender)
        return;

    char formatted_message[BUFFER_SIZE];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(formatted_message, sizeof(formatted_message), "[%H:%M:%S] ", tm_info);
    snprintf(formatted_message + strlen(formatted_message),
             sizeof(formatted_message) - strlen(formatted_message),
             "<%s> %s\n", sender->nickname, message);

    // Send to all users in room except sender
    for (int i = 0; i < room->user_count; i++)
    {
        if (room->users[i] && room->users[i] != sender)
        {
            connection_prepare_response(room->users[i]->connection,
                                        formatted_message, strlen(formatted_message));
        }
    }

    // Send confirmation to sender
    snprintf(formatted_message, sizeof(formatted_message), "Message sent to #%s\n", room->name);
    connection_prepare_response(sender->connection, formatted_message, strlen(formatted_message));
}

void chat_announce_to_room(ChatRoom *room, const char *message)
{
    if (!room)
        return;

    char formatted_message[BUFFER_SIZE];
    snprintf(formatted_message, sizeof(formatted_message), "%s\n", message);

    for (int i = 0; i < room->user_count; i++)
    {
        if (room->users[i])
        {
            connection_prepare_response(room->users[i]->connection,
                                        formatted_message, strlen(formatted_message));
        }
    }
}

void chat_handle_help_command(ChatUser *user)
{
    const char *help_text =
        "=== MultiServer Chat Commands ===\n"
        "/join <room> [password] - Join or create a room\n"
        "/leave                  - Leave current room\n"
        "/list rooms            - List available rooms\n"
        "/list users            - List users in current room\n"
        "/msg <user> <message>  - Send private message\n"
        "/nick <nickname>       - Change your nickname\n"
        "/stats                 - Show server statistics\n"
        "/time                  - Show current time\n"
        "/help                  - Show this help\n"
        "/quit                  - Disconnect\n"
        "\nTo chat, just type your message (must be in a room)\n"
        "================================\n";

    connection_prepare_response(user->connection, help_text, strlen(help_text));
    connection_write(user->connection);
}

int chat_process_command(ChatServer *server, ChatUser *user, const char *input)
{
    char *input_copy = strdup(input);
    char *command = strtok(input_copy, " ");
    char *args = strtok(NULL, "");

    if (!command)
    {
        free(input_copy);
        return 0;
    }

    user->last_activity = time(NULL);

    if (command[0] == '/')
    {
        // Handle commands
        if (strcmp(command, "/help") == 0)
        {
            chat_handle_help_command(user);
        }
        else if (strcmp(command, "/join") == 0)
        {
            chat_handle_join_command(server, user, args);
        }
        else if (strcmp(command, "/leave") == 0)
        {
            chat_leave_room(user);
        }
        else if (strcmp(command, "/nick") == 0)
        {
            chat_handle_nick_command(server, user, args);
        }
        else if (strcmp(command, "/list") == 0)
        {
            chat_handle_list_command(server, user, args);
        }
        else if (strcmp(command, "/stats") == 0)
        {
            chat_handle_stats_command(server, user);
        }
        else if (strcmp(command, "/time") == 0)
        {
            time_t now = time(NULL);
            char time_msg[128];
            snprintf(time_msg, sizeof(time_msg), "Server time: %s", ctime(&now));
            chat_send_system_message(user, time_msg);
        }
        else if (strcmp(command, "/quit") == 0)
        {
            chat_send_system_message(user, "Goodbye!");
            free(input_copy);
            return -1; // Signal to close connection
        }
        else
        {
            chat_send_system_message(user, "Unknown command. Type /help for available commands.");
        }
    }
    else
    {
        // Regular chat message
        if (user->current_room)
        {
            chat_broadcast_to_room(user->current_room, input, user);
            server->total_messages++;
        }
        else
        {
            chat_send_system_message(user, "You must join a room to chat. Type /join lobby");
        }
    }

    free(input_copy);
    return 0;
}

void chat_handle_join_command(ChatServer *server, ChatUser *user, const char *args)
{
    if (!args)
    {
        chat_send_system_message(user, "Usage: /join <room> [password]");
        return;
    }

    char *args_copy = strdup(args);
    char *room_name = strtok(args_copy, " ");
    char *password = strtok(NULL, " ");

    if (room_name)
    {
        chat_join_room(server, user, room_name, password);
    }
    else
    {
        chat_send_system_message(user, "Usage: /join <room> [password]");
    }

    free(args_copy);
}

void chat_handle_nick_command(ChatServer *server, ChatUser *user, const char *args)
{
    if (!args || strlen(args) == 0)
    {
        chat_send_system_message(user, "Usage: /nick <new_nickname>");
        return;
    }

    if (strlen(args) >= MAX_NICKNAME_LENGTH)
    {
        chat_send_system_message(user, "Nickname too long");
        return;
    }

    // Check if nickname is already taken
    if (chat_find_user_by_nickname(server, args))
    {
        chat_send_system_message(user, "Nickname already taken");
        return;
    }

    char old_nick[MAX_NICKNAME_LENGTH];
    strcpy(old_nick, user->nickname);
    strcpy(user->nickname, args);

    char message[256];
    snprintf(message, sizeof(message), "Your nickname changed from %s to %s", old_nick, user->nickname);
    chat_send_system_message(user, message);

    // Announce to current room
    if (user->current_room)
    {
        snprintf(message, sizeof(message), "*** %s is now known as %s", old_nick, user->nickname);
        chat_announce_to_room(user->current_room, message);
    }
}

void chat_handle_list_command(ChatServer *server, ChatUser *user, const char *args)
{
    if (!args || strcmp(args, "rooms") == 0)
    {
        // List rooms
        char response[BUFFER_SIZE] = "=== Available Rooms ===\n";
        for (int i = 0; i < server->room_count; i++)
        {
            if (server->rooms[i])
            {
                char room_info[512]; // Increased buffer size
                snprintf(room_info, sizeof(room_info), "#%s (%d users) - %.100s\n",
                         server->rooms[i]->name,
                         server->rooms[i]->user_count,
                         server->rooms[i]->topic);
                if (strlen(response) + strlen(room_info) < BUFFER_SIZE - 1)
                {
                    strcat(response, room_info);
                }
            }
        }
        strcat(response, "=======================\n");
        connection_prepare_response(user->connection, response, strlen(response));
    }
    else if (strcmp(args, "users") == 0)
    {
        // List users in current room
        if (!user->current_room)
        {
            chat_send_system_message(user, "You are not in a room");
            return;
        }

        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), "=== Users in #%s ===\n", user->current_room->name);

        for (int i = 0; i < user->current_room->user_count; i++)
        {
            if (user->current_room->users[i])
            {
                char user_info[128];
                snprintf(user_info, sizeof(user_info), "%s\n", user->current_room->users[i]->nickname);
                strcat(response, user_info);
            }
        }
        strcat(response, "===================\n");
        connection_prepare_response(user->connection, response, strlen(response));
    }
    else
    {
        chat_send_system_message(user, "Usage: /list [rooms|users]");
    }
}

void chat_handle_stats_command(ChatServer *server, ChatUser *user)
{
    time_t uptime = time(NULL) - server->start_time;
    char response[BUFFER_SIZE];

    snprintf(response, sizeof(response),
             "=== Server Statistics ===\n"
             "Uptime: %ld seconds\n"
             "Total rooms: %d\n"
             "Active users: %d\n"
             "Total messages: %d\n"
             "Total users served: %d\n"
             "Peak concurrent users: %d\n"
             "========================\n",
             uptime, server->room_count, server->user_count,
             server->total_messages, server->total_users_served,
             server->peak_concurrent_users);

    connection_prepare_response(user->connection, response, strlen(response));
}

int enhanced_chat_handler(ChatServer *server, Connection *conn)
{
    if (!server || !conn)
        return 0;

    log_info("Chat handler called, buffer_used: %zu, buffer: '%.*s'",
             conn->read_buffer_used, (int)conn->read_buffer_used, conn->read_buffer);

    if (conn->read_buffer_used == 0)
        return 0;

    // Find or create user for this connection
    ChatUser *user = chat_find_user_by_connection(server, conn);
    if (!user)
    {
        // New user
        if (server->user_count >= MAX_CONNECTIONS)
        {
            const char *full_msg = "Server full. Try again later.\n";
            connection_prepare_response(conn, full_msg, strlen(full_msg));
            return -1;
        }

        user = chat_user_create(conn);
        if (!user)
            return -1;

        // Set connection to keep-alive for persistent chat sessions
        conn->keep_alive = true;

        server->users[server->user_count++] = user;
        server->total_users_served++;

        if (server->user_count > server->peak_concurrent_users)
        {
            server->peak_concurrent_users = server->user_count;
        }

        // Welcome new user
        const char *welcome =
            "Welcome to MultiServer Chat!\n"
            "You are now connected in a persistent session.\n"
            "Type /help for commands, /join lobby to start chatting, or /quit to disconnect.\n"
            ">>> ";
        connection_prepare_response(conn, welcome, strlen(welcome));
        connection_write(conn);

        log_info("New persistent chat user %s connected", user->nickname);

        // DON'T clear buffer yet - continue processing any additional commands
        // that came with the initial connection
    }

    // Process user input - handle multiple lines if present
    char *buffer = conn->read_buffer;
    size_t remaining = conn->read_buffer_used;

    while (remaining > 0)
    {
        char *newline = memchr(buffer, '\n', remaining);
        if (!newline)
        {
            // No complete line yet, wait for more data
            log_info("No complete line found, waiting for more data");
            break;
        }

        // Process this line
        *newline = '\0';
        size_t line_length = newline - buffer;

        log_info("Processing line: '%s' (length: %zu)", buffer, line_length);

        if (line_length > 0)
        {
            // Check for quit command first
            if (strcmp(buffer, "/quit") == 0 || strcmp(buffer, "quit") == 0 || strcmp(buffer, "QUIT") == 0) {
                const char *goodbye = "Goodbye! Thanks for using MultiServer Chat.\n";
                connection_prepare_response(conn, goodbye, strlen(goodbye));
                connection_write(conn);
                conn->read_buffer_used = 0;
                return -1; // Signal to close connection
            }
            
            int result = chat_process_command(server, user, buffer);
            if (result < 0)
            {
                conn->read_buffer_used = 0;
                return result;
            }
            
            // Add prompt after each command for interactive feel
            const char *prompt = ">>> ";
            connection_prepare_response(conn, prompt, strlen(prompt));
            connection_write(conn);
        }

        // Move to next line
        buffer = newline + 1;
        remaining -= (line_length + 1);
    }

    // Clear the processed data from buffer
    conn->read_buffer_used = 0;
    return 1;
}

// Initialize global chat server
int chat_system_init(void)
{
    global_chat_server = chat_server_create();
    return global_chat_server ? 0 : -1;
}

// Get global chat server
ChatServer *chat_get_server(void)
{
    return global_chat_server;
}

// Cleanup global chat server
void chat_system_cleanup(void)
{
    if (global_chat_server)
    {
        chat_server_destroy(global_chat_server);
        global_chat_server = NULL;
    }
}
