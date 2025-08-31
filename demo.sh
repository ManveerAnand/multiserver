#!/bin/bash

# MultiServer Chat Demo
# Shows the real-time chat capabilities

echo "💬 MultiServer Chat Demo"
echo "========================"
echo ""
echo "This demo shows the persistent chat system."
echo "The server supports:"
echo "  ✅ Real-time messaging"
echo "  ✅ Multiple chat rooms" 
echo "  ✅ User management"
echo "  ✅ Persistent connections"
echo ""

# Check if server is running
if ! netstat -ln 2>/dev/null | grep -q ":8081 "; then
    echo "⚠️  Starting MultiServer first..."
    echo "   (You can also run './run.sh' in another terminal)"
    echo ""
    
    # Stop any existing processes first
    pkill -f multiserver 2>/dev/null || true
    sleep 1
    
    # Start server in background for demo
    ./multiserver -c config/test.conf &
    SERVER_PID=$!
    sleep 2
    
    echo "✅ Server started (PID: $SERVER_PID)"
    STARTED_SERVER=true
fi

echo ""
echo "🔌 Connecting to chat server..."
echo "💡 Try these commands:"
echo "   /help          - Show all commands"
echo "   /join lobby    - Join the main chat room"
echo "   /list users    - See who's online"
echo "   /nick YourName - Change your nickname"
echo "   hello world    - Send a chat message"
echo "   /quit          - Exit the chat"
echo ""
echo "🚀 Starting interactive chat session..."
echo "================================================"

# Connect to chat
telnet localhost 8081

# Cleanup if we started the server
if [ "$STARTED_SERVER" = true ]; then
    echo ""
    echo "🛑 Stopping demo server..."
    kill $SERVER_PID 2>/dev/null
fi

echo ""
echo "✅ Demo complete!"
echo "💡 To run your own server: ./run.sh"
