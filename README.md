# 💬 MultiServer - Real-Time Chat & Web Server

A high-performance C-based server that combines **HTTP web serving** and **real-time chat** in a single application. Built with advanced networking concepts including event-driven architecture, connection pooling, and intelligent protocol detection.

## 🚀 Quick Start

**Just want to try it? Run this:**
```bash
./run.sh
```

**Want to see the chat demo? Run this:**
```bash
./demo.sh
```

That's it! 🎉

## ✨ What Can It Do?

### 🌐 Web Server (HTTP)
- **Static file serving** - Host websites, images, CSS, JavaScript
- **Smart routing** - Automatic index.html serving  
- **File browsing** - Directory listings when enabled
- **Multi-format support** - HTML, CSS, JS, images, text files

### 💬 Real-Time Chat Server  
- **Persistent chat sessions** - Stay connected, no reconnection needed
- **Multiple chat rooms** - Create and join different rooms
- **User management** - Nicknames, user lists, join/leave notifications
- **Rich commands** - `/help`, `/join`, `/nick`, `/list`, `/quit` and more
- **Live messaging** - Real-time chat with other users

### ⚡ Advanced Features
- **Dual-protocol** - HTTP (port 8080) + Chat (port 8081) simultaneously  
- **Event-driven** - Handles 1000+ concurrent connections efficiently
- **Smart detection** - Automatically routes HTTP vs Chat traffic
- **Configurable** - Easy INI-style configuration
- **Production logging** - Multi-level logs with colors

## 🎮 Try It Out

### Option 1: Web Server
```bash
./run.sh
# Then visit: http://localhost:8080
```

### Option 2: Chat Server  
```bash
./run.sh
# In another terminal:
./demo.sh
# Or manually: telnet localhost 8081
```

### Option 3: Multi-User Chat
```bash
# Terminal 1:
./run.sh

# Terminal 2:
telnet localhost 8081
/join lobby
/nick Alice
Hello everyone!

# Terminal 3:  
telnet localhost 8081
/join lobby
/nick Bob
Hi Alice!
```

## 💻 Chat Commands

| Command | Description | Example |
|---------|-------------|---------|
| `/help` | Show all commands | `/help` |
| `/join <room>` | Join or create a room | `/join lobby` |
| `/nick <name>` | Change your nickname | `/nick Alice` |
| `/list users` | See who's in your room | `/list users` |
| `/list rooms` | See available rooms | `/list rooms` |
| `/leave` | Leave current room | `/leave` |
| `/quit` | Exit chat | `/quit` |
| `<message>` | Send chat message | `Hello world!` |

## 🛠️ Installation

### Prerequisites
- **Linux/WSL** (Ubuntu, Debian, etc.)
- **GCC compiler** (`sudo apt install build-essential`)
- **Make** (usually included with build-essential)

### Build & Run
```bash
# Clone or download the project
cd multiserver

# Build (first time only)
make

# Run the server
./run.sh

# Stop the server (if needed)
./stop.sh

# Or run manually
./multiserver -c config/test.conf
```

### Troubleshooting
- **Port already in use?** - The run script automatically stops existing servers
- **Permission denied?** - Make scripts executable: `chmod +x *.sh`
- **Build errors?** - Install build tools: `sudo apt install build-essential`

## 📁 Project Structure

```
multiserver/
├── run.sh              # Quick start script  
├── demo.sh             # Interactive chat demo
├── stop.sh             # Stop server script
├── multiserver         # Main executable (after build)
├── Makefile           # Build configuration
├── config/            # Server configuration
│   └── test.conf      # Default config file
├── src/               # Source code
│   ├── main.c         # Program entry point
│   ├── server.c       # Core server engine
│   ├── connection.c   # Connection management
│   ├── enhanced_chat.c # Chat system
│   └── logging.c      # Logging system
├── include/           # Header files
├── www/               # Web content directory
│   └── index.html     # Default web page
└── tools/             # Demo and testing tools
```

## ⚙️ Configuration

Edit `config/test.conf` to customize:

```ini
[server]
http_port = 8080        # Web server port
chat_port = 8081        # Chat server port  
max_connections = 1000  # Concurrent connection limit

[logging]
level = INFO           # DEBUG, INFO, WARN, ERROR
console = true         # Show logs in terminal
to_file = true         # Save logs to file

[chat]
max_rooms = 100        # Maximum chat rooms
max_users_per_room = 50 # Users per room limit
```

## 🔧 Architecture Highlights

- **Event-Driven**: Uses `select()` for non-blocking I/O
- **Protocol Detection**: Automatically distinguishes HTTP vs Chat traffic
- **Connection Pooling**: Efficient memory and resource management  
- **Persistent Sessions**: Chat connections stay alive for real-time messaging
- **Modular Design**: Clean separation between HTTP, Chat, and Core systems

## 🎯 Technical Features

### Performance
- **1000+ concurrent connections** supported
- **Non-blocking I/O** for maximum efficiency
- **Memory-safe** connection management
- **Signal handling** for graceful shutdown

### Networking
- **Dual-port operation** (HTTP + Chat simultaneously)
- **Protocol multiplexing** on single server
- **Keep-alive connections** for chat persistence
- **Automatic cleanup** of idle connections

### Development
- **Clean C code** with comprehensive error handling
- **Configurable logging** with multiple levels
- **Easy build system** with Make
- **Cross-platform** (Linux/Unix/WSL)

## 🚀 What's Next?

This server demonstrates core networking concepts and is ready for extension:

- **Authentication system** - User login/registration
- **File upload handling** - POST request processing  
- **Database integration** - Persistent chat history
- **WebSocket support** - Browser-based chat client
- **Load balancing** - Multiple server instances
- **SSL/TLS encryption** - Secure connections

## 📊 Current Status

✅ **Fully Functional** - Both HTTP and Chat working  
✅ **Production Ready** - Handles multiple users simultaneously  
✅ **Well Documented** - Clear code and extensive comments  
✅ **Easy to Use** - Simple scripts for immediate testing  
✅ **Extensible** - Clean architecture for future features  

**Ready to showcase networking skills!** 🎉
