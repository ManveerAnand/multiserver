#!/usr/bin/env python3
import socket
import time

def test_help_command():
    # Connect to the chat server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8081))
    
    # Receive welcome message
    welcome = sock.recv(1024).decode()
    print("Welcome message:", repr(welcome))
    
    # Send /help command
    print("Sending /help command...")
    sock.send(b"/help\n")
    time.sleep(0.1)
    
    # Read response
    response = sock.recv(1024).decode()
    print("Response:", repr(response))
    
    # Send /quit command
    print("Sending /quit command...")
    sock.send(b"/quit\n")
    time.sleep(0.1)
    
    # Read response
    response = sock.recv(1024).decode()
    print("Quit response:", repr(response))
    
    sock.close()

if __name__ == "__main__":
    test_help_command()
