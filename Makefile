PRJ_NAME := iterative
COMPILER := clang
COMPILER_FLAGS := -std=c23 -g `llvm-config --cflags`
LINKER_FLAGS := `llvm-config --libs --system-libs`
BIN_DIR := bin
OBJ_DIR := obj
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)

all: directories $(BIN_DIR)/$(PRJ_NAME)

directories:
	mkdir -p $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR)/$(PRJ_NAME): $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) -o $@ $^ $(LINKER_FLAGS)

$(OBJ_DIR)/%.o: %.c
	$(COMPILER) $(COMPILER_FLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

run: all
	./$(BIN_DIR)/$(PRJ_NAME)