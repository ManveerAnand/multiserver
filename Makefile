CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -O2 -g -D_GNU_SOURCE
LDFLAGS = -lpthread
INCLUDE = -Iinclude
SRC_DIR = src
BUILD_DIR = build
TARGET = multiserver

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/http/*.c) \
       $(wildcard $(SRC_DIR)/chat/*.c) \
       $(wildcard $(SRC_DIR)/utils/*.c)

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Install the executable
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	mkdir -p /etc/multiserver
	cp config/multiserver.conf /etc/multiserver/
	mkdir -p /var/log/multiserver
	mkdir -p /var/www/multiserver
	cp -r www/* /var/www/multiserver/

# Run the server
run: $(TARGET)
	./$(TARGET) -c config/multiserver.conf

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all clean install run debug
