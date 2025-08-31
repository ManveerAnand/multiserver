#ifndef COMMON_H
#define COMMON_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>

// Constants
#define MAX_CONNECTIONS 1000
#define BUFFER_SIZE 8192
#define PATH_MAX 4096
#define MAX_CONFIG_LINE 256
#define MAX_ROOMS 100
#define MAX_USERS_PER_ROOM 50

// Protocol types
typedef enum
{
    PROTOCOL_UNKNOWN = 0,
    PROTOCOL_HTTP,
    PROTOCOL_CHAT,
    PROTOCOL_HTTPS
} ProtocolType;

// Log levels
typedef enum
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

// Global variables
extern volatile sig_atomic_t running;
extern volatile sig_atomic_t reload_config;

#endif // COMMON_H
