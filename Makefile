# Compiler and flags
CC = gcc
CFLAGS = -O2 -pthread -Wall -Wextra -g
DBGFLAG = -DDBG

# Directories
SRC_DIR = src
BUILD_DIR = build
BUILD_DIR_DBG = build_dbg
OUT_DIR = out

# Output binaries
TARGET = $(OUT_DIR)/word_count
TARGET_DBG = $(OUT_DIR)/word_count_dbg

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
OBJS_DBG = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR_DBG)/%.o, $(SRCS))

# Default target
all: release

# Create output directories if needed
$(BUILD_DIR) $(BUILD_DIR_DBG) $(OUT_DIR):
	mkdir -p $@

# Rule to build release .o files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to build debug .o files
$(BUILD_DIR_DBG)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR_DBG)
	$(CC) $(CFLAGS) $(DBGFLAG) -c $< -o $@

# Release target
release: $(OUT_DIR) $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

# Debug target
debug: $(OUT_DIR) $(OBJS_DBG)
	$(CC) $(OBJS_DBG) -o $(TARGET_DBG) $(CFLAGS) $(DBGFLAG)

# Clean everything
clean:
	rm -rf $(BUILD_DIR) $(BUILD_DIR_DBG) $(OUT_DIR)

.PHONY: all release debug clean
