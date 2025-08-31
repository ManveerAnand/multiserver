#!/usr/bin/env python3
import socket
import time

def test_clear_command():
    # Connect to the chat server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5.0)
    try:
        sock.connect(('localhost', 8081))
        
        # Read welcome message
        try:
            welcome = sock.recv(1024).decode()
            print("CONNECTED:", repr(welcome[:50]) + "...")
        except socket.timeout:
            print("TIMEOUT waiting for welcome message")
        
        # Send /help command to see new clear command
        print("\nTesting /help command...")
        sock.send(b"/help\n")
        time.sleep(0.1)
        try:
            response = sock.recv(2048).decode()
            print("HELP RESPONSE:")
            print(response)
        except socket.timeout:
            print("TIMEOUT waiting for help response")
        
        # Test /clear command
        print("\nTesting /clear command...")
        sock.send(b"/clear\n")
        time.sleep(0.1)
        try:
            response = sock.recv(1024)
            print("CLEAR RESPONSE (raw bytes):", repr(response))
            print("CLEAR RESPONSE (decoded):", repr(response.decode()))
        except socket.timeout:
            print("TIMEOUT waiting for clear response")
        
    except Exception as e:
        print(f"ERROR: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    test_clear_command()
