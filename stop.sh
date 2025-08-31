#!/bin/bash

# MultiServer Stop Script
# Safely stops all MultiServer instances

echo "🛑 Stopping MultiServer..."

# Kill all multiserver processes
if pkill -f multiserver 2>/dev/null; then
    echo "✅ MultiServer processes stopped"
    
    # Wait a moment for clean shutdown
    sleep 1
    
    # Check if any are still running
    if pgrep -f multiserver >/dev/null; then
        echo "⚠️  Force killing remaining processes..."
        pkill -9 -f multiserver 2>/dev/null || true
    fi
else
    echo "ℹ️  No MultiServer processes were running"
fi

echo "🏁 All MultiServer instances stopped"
