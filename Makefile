CC = gcc
# Include the local header directory
CFLAGS = -Wall -Wextra -O2 -I./include

# Define build directories and target
BUILD_DIR = build
SRC_DIR = src
TARGET = $(BUILD_DIR)/lcdtool

# Automatically find all .c files in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Generate corresponding .o file paths in build/
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
# The | $(BUILD_DIR) ensures the directory exists before compiling
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean up
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
