#include "common.h"
#include "config.h"
#include "logging.h"
#include "server.h"
#include "enhanced_chat.h"
#include <getopt.h>

void print_usage(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -c, --config FILE    Configuration file path\n");
    printf("  -d, --daemon         Run as daemon\n");
    printf("  -h, --help           Show this help message\n");
    printf("  -v, --version        Show version information\n");
    printf("  -t, --test-config    Test configuration and exit\n");
    printf("  -s, --stats          Show statistics and exit\n");
}

void print_version(void)
{
    printf("MultiServer v1.0.0\n");
    printf("Advanced Multi-Protocol Networking Server\n");
    printf("Built on %s at %s\n", __DATE__, __TIME__);
}

int main(int argc, char *argv[])
{
    ServerConfig config;
    Server *server = NULL;
    char *config_file = NULL;
    bool daemon_mode = false;
    bool test_config = false;
    bool show_stats = false;
    int opt;

    // Parse command line options
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"daemon", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"test-config", no_argument, 0, 't'},
        {"stats", no_argument, 0, 's'},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "c:dhvts", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'c':
            config_file = optarg;
            break;
        case 'd':
            daemon_mode = true;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        case 'v':
            print_version();
            exit(EXIT_SUCCESS);
        case 't':
            test_config = true;
            break;
        case 's':
            show_stats = true;
            break;
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Load configuration
    if (config_load(config_file ? config_file : "config/multiserver.conf", &config) < 0)
    {
        fprintf(stderr, "Failed to load configuration\n");
        exit(EXIT_FAILURE);
    }

    // Validate configuration
    if (config_validate(&config) < 0)
    {
        fprintf(stderr, "Configuration validation failed\n");
        exit(EXIT_FAILURE);
    }

    // Test config mode
    if (test_config)
    {
        printf("Configuration test successful\n");
        config_print(&config);
        exit(EXIT_SUCCESS);
    }

    // Initialize logging
    if (logging_init(&config) < 0)
    {
        fprintf(stderr, "Failed to initialize logging\n");
        exit(EXIT_FAILURE);
    }

    // Print startup banner
    log_info("=================================");
    log_info("  MultiServer v1.0.0 Starting");
    log_info("=================================");

    if (config_file)
    {
        log_info("Using config file: %s", config_file);
    }
    else
    {
        log_info("Using default configuration");
    }

    // Setup signal handlers
    setup_signal_handlers();

    // Initialize chat system
    if (chat_system_init() < 0)
    {
        log_fatal("Failed to initialize chat system");
        exit(EXIT_FAILURE);
    }
    log_info("Enhanced chat system initialized");

    // Create server
    server = server_create(&config);
    if (!server)
    {
        log_fatal("Failed to create server");
        exit(EXIT_FAILURE);
    }

    // Initialize server sockets
    if (server_init_sockets(server) < 0)
    {
        log_fatal("Failed to initialize server sockets");
        server_destroy(server);
        exit(EXIT_FAILURE);
    }

    // Show stats mode
    if (show_stats)
    {
        server_print_stats(server);
        server_destroy(server);
        exit(EXIT_SUCCESS);
    }

    // Daemon mode
    if (daemon_mode)
    {
        log_info("Starting in daemon mode");
        pid_t pid = fork();
        if (pid < 0)
        {
            log_fatal("Failed to fork daemon process");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            // Parent process - exit
            log_info("Daemon started with PID %d", pid);
            exit(EXIT_SUCCESS);
        }

        // Child process - become daemon
        if (setsid() < 0)
        {
            log_fatal("Failed to create new session");
            exit(EXIT_FAILURE);
        }

        // Change working directory to root
        if (chdir("/") < 0)
        {
            log_warn("Failed to change working directory to /");
        }

        // Close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    log_info("Server initialization complete");
    log_info("HTTP server listening on port %d", config.http_port);
    log_info("Chat server listening on port %d", config.chat_port);
    log_info("Maximum connections: %d", config.max_connections);
    log_info("Document root: %s", config.document_root);

    // Run server
    int result = server_run(server);

    // Cleanup
    log_info("Server shutting down");
    server_print_stats(server);
    server_destroy(server);
    logging_cleanup();

    log_info("Server shutdown complete");
    return result;
}
