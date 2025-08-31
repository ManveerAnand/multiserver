#ifndef ENHANCED_CHAT_H
#define ENHANCED_CHAT_H

#include "common.h"
#include "connection.h"

#define MAX_NICKNAME_LENGTH 32
#define MAX_ROOM_NAME_LENGTH 32
#define MAX_MESSAGE_LENGTH 512
#define MAX_USERS_PER_ROOM 50
#define MAX_ROOMS 100

// Chat user structure
typedef struct ChatUser
{
    char nickname[MAX_NICKNAME_LENGTH];
    Connection *connection;
    struct ChatRoom *current_room;
    time_t join_time;
    time_t last_activity;
    bool authenticated;
    bool is_admin;
} ChatUser;

// Chat room structure
typedef struct ChatRoom
{
    char name[MAX_ROOM_NAME_LENGTH];
    ChatUser *users[MAX_USERS_PER_ROOM];
    int user_count;
    time_t created_at;
    char topic[256];
    bool password_protected;
    char password[64];
    bool private_room;
} ChatRoom;

// Chat server structure
typedef struct ChatServer
{
    ChatRoom *rooms[MAX_ROOMS];
    int room_count;
    ChatUser *users[MAX_CONNECTIONS];
    int user_count;
    time_t start_time;

    // Statistics
    int total_messages;
    int total_users_served;
    int peak_concurrent_users;
} ChatServer;

// Function prototypes
ChatServer *chat_server_create(void);
void chat_server_destroy(ChatServer *chat_server);

// User management
ChatUser *chat_user_create(Connection *conn);
void chat_user_destroy(ChatUser *user);
ChatUser *chat_find_user_by_nickname(ChatServer *server, const char *nickname);
ChatUser *chat_find_user_by_connection(ChatServer *server, Connection *conn);
int chat_authenticate_user(ChatUser *user, const char *nickname);

// Room management
ChatRoom *chat_room_create(const char *name);
void chat_room_destroy(ChatRoom *room);
ChatRoom *chat_find_room(ChatServer *server, const char *name);
int chat_join_room(ChatServer *server, ChatUser *user, const char *room_name, const char *password);
int chat_leave_room(ChatUser *user);
void chat_list_rooms(ChatServer *server, ChatUser *user);
void chat_list_users_in_room(ChatRoom *room, ChatUser *requesting_user);

// Message handling
void chat_broadcast_to_room(ChatRoom *room, const char *message, ChatUser *sender);
void chat_send_private_message(ChatUser *sender, ChatUser *recipient, const char *message);
void chat_send_system_message(ChatUser *user, const char *message);
void chat_announce_to_room(ChatRoom *room, const char *message);

// Command processing
int chat_process_command(ChatServer *server, ChatUser *user, const char *input);
void chat_handle_join_command(ChatServer *server, ChatUser *user, const char *args);
void chat_handle_leave_command(ChatUser *user);
void chat_handle_msg_command(ChatServer *server, ChatUser *user, const char *args);
void chat_handle_nick_command(ChatServer *server, ChatUser *user, const char *args);
void chat_handle_help_command(ChatUser *user);
void chat_handle_list_command(ChatServer *server, ChatUser *user, const char *args);
void chat_handle_stats_command(ChatServer *server, ChatUser *user);

// Enhanced chat handler
int enhanced_chat_handler(ChatServer *server, Connection *conn);

// Global chat system functions
int chat_system_init(void);
ChatServer *chat_get_server(void);
void chat_system_cleanup(void);

#endif // ENHANCED_CHAT_H
