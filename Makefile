# Compiler
CC = aarch64-linux-gnu-gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -I. -Ihtu21d

# Directories
OBJ_DIR = build
BIN_DIR = build/bin

# Source Files
SRCS = main.c htu21d/htu21d.c
OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)

# Output Executable
TARGET = $(BIN_DIR)/htu21d_app

# Default rule: Compile everything
all: directories $(TARGET)

# Ensure build/ and bin/ directories exist
directories:
	mkdir -p $(OBJ_DIR) $(BIN_DIR) $(OBJ_DIR)/htu21d

# Linking step
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compile .c files into .o in the build directory
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build and bin directories
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
