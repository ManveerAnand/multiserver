#!/usr/bin/env python3
import socket
import time
import select

def test_help_command():
    # Connect to the chat server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5.0)  # 5 second timeout
    try:
        sock.connect(('localhost', 8081))
        
        # Read welcome message with timeout
        try:
            welcome = sock.recv(1024).decode()
            print("WELCOME:", repr(welcome))
        except socket.timeout:
            print("TIMEOUT waiting for welcome message")
        
        # Send /help command
        print("Sending /help command...")
        sock.send(b"/help\n")
        
        # Wait a bit and read response with timeout
        time.sleep(0.1)
        try:
            response = sock.recv(1024).decode()
            print("RESPONSE:", repr(response))
        except socket.timeout:
            print("TIMEOUT waiting for help response")
        
        # Try /join lobby
        print("Sending /join lobby...")
        sock.send(b"/join lobby\n")
        time.sleep(0.1)
        try:
            response = sock.recv(1024).decode()
            print("JOIN RESPONSE:", repr(response))
        except socket.timeout:
            print("TIMEOUT waiting for join response")
        
        # Try /help again after joining
        print("Sending /help after join...")
        sock.send(b"/help\n")
        time.sleep(0.1)
        try:
            response = sock.recv(1024).decode()
            print("HELP AFTER JOIN:", repr(response))
        except socket.timeout:
            print("TIMEOUT waiting for help after join")
        
    except Exception as e:
        print(f"ERROR: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    test_help_command()
