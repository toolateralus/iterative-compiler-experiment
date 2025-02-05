PRJ_NAME := iterative
COMPILER := clang
COMPILER_FLAGS := -std=c23 -O3
BIN_DIR := bin
SRCS := $(wildcard *.c)

all: directories $(PRJ_NAME)

directories:
	mkdir -p $(BIN_DIR)

$(PRJ_NAME): $(SRCS)
	$(COMPILER) $(COMPILER_FLAGS) -o $(BIN_DIR)/$(PRJ_NAME) $(SRCS)

clean:
	rm -rf $(BIN_DIR)

run: all
	./$(BIN_DIR)/$(PRJ_NAME)
