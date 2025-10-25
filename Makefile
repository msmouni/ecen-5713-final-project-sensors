# Compiler
CC = aarch64-linux-gnu-gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -I. -Ii2c -Ihtu21d -Ibmp280 -Idb
LDFLAGS := -lsqlite3

# Directories
OBJ_DIR = build
BIN_DIR = build/bin

# Source Files
SRCS = main.c i2c/i2c.c htu21d/htu21d.c bmp280/bmp280.c db/db.c
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Output Executable
TARGET = $(BIN_DIR)/rpi_sensors

# Default rule: Compile everything
all: directories $(TARGET)

# Ensure build/ and bin/ directories exist
directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR) $(OBJ_DIR)/i2c $(OBJ_DIR)/htu21d $(OBJ_DIR)/bmp280 $(OBJ_DIR)/db

# Linking step
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile .c files into .o in the build directory
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build and bin directories
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
