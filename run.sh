#!/bin/bash

# MultiServer Quick Start Script
# This script makes it easy to build and run the server

echo "🚀 MultiServer Quick Start"
echo "=========================="

# Stop any existing multiserver processes
echo "🛑 Stopping any existing MultiServer processes..."
pkill -f multiserver 2>/dev/null || true
sleep 1

# Build the project
echo "📦 Building MultiServer..."
make clean
make

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo "✅ Build successful!"
echo ""
echo "🌐 Starting MultiServer..."
echo "   - HTTP Server: http://localhost:8080"
echo "   - Chat Server: telnet localhost 8081"
echo ""
echo "💡 Quick Demo:"
echo "   - Run: ./demo.sh to see chat demo"
echo "   - Visit: http://localhost:8080 for web interface"
echo ""
echo "🛑 Press Ctrl+C to stop the server"
echo ""

# Start the server
./multiserver -c config/test.conf
