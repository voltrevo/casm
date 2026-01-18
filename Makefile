.PHONY: all clean test build build-release build-debug test help run-example

CC = gcc

# Debug flags: sanitizers enabled
CFLAGS_DEBUG = -Wall -Wextra -pedantic -std=c99 -g \
	-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer \
	-I./src

# Release flags: optimized, no sanitizers
CFLAGS_RELEASE = -Wall -Wextra -pedantic -std=c99 -O3 -I./src

# Default to debug
CFLAGS = $(CFLAGS_DEBUG)

LDFLAGS = -lm

# Source files
SOURCES = src/main.c src/lexer.c src/parser.c src/ast.c src/utils.c
TEST_SOURCES = tests/test_lexer.c src/lexer.c src/utils.c

# Output
MAIN_BINARY = casm
TEST_BINARY = test_casm

all: build

build: build-debug

build-debug: $(SOURCES)
	$(CC) $(CFLAGS_DEBUG) -o $(MAIN_BINARY) $^ $(LDFLAGS)

build-release: $(SOURCES)
	$(CC) $(CFLAGS_RELEASE) -o $(MAIN_BINARY) $^ $(LDFLAGS)

test: 
	$(CC) $(CFLAGS_DEBUG) -o $(TEST_BINARY) $(TEST_SOURCES) $(LDFLAGS)
	./$(TEST_BINARY)

clean:
	rm -f $(MAIN_BINARY) $(TEST_BINARY)

run-example: build
	./$(MAIN_BINARY) examples/simple_add.csm

help:
	@echo "Available targets:"
	@echo "  build           - Build the compiler (debug with sanitizers)"
	@echo "  build-debug     - Build debug version with sanitizers"
	@echo "  build-release   - Build optimized release version"
	@echo "  test            - Run unit tests"
	@echo "  clean           - Remove built files"
	@echo "  run-example     - Compile and run an example"
	@echo "  help            - Show this help message"
