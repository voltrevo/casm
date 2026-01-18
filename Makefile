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

# Output directory
BIN_DIR = bin

# Source files
SOURCES = src/main.c src/lexer.c src/parser.c src/ast.c src/utils.c src/types.c src/semantics.c src/codegen.c
TEST_SOURCES = tests/test_lexer.c src/lexer.c src/utils.c
SEMANTICS_TEST_SOURCES = tests/test_semantics.c src/lexer.c src/parser.c src/ast.c src/utils.c src/types.c src/semantics.c
CODEGEN_TEST_SOURCES = tests/test_codegen.c src/lexer.c src/parser.c src/ast.c src/utils.c src/types.c src/semantics.c src/codegen.c

# Output
MAIN_BINARY = $(BIN_DIR)/casm
TEST_BINARY = $(BIN_DIR)/test_casm
SEMANTICS_TEST_BINARY = $(BIN_DIR)/test_semantics
CODEGEN_TEST_BINARY = $(BIN_DIR)/test_codegen

all: build

build: build-debug

build-debug: $(BIN_DIR) $(SOURCES)
	$(CC) $(CFLAGS_DEBUG) -o $(MAIN_BINARY) $(SOURCES) $(LDFLAGS)

build-release: $(BIN_DIR) $(SOURCES)
	$(CC) $(CFLAGS_RELEASE) -o $(MAIN_BINARY) $(SOURCES) $(LDFLAGS)

test: build-debug $(TEST_BINARY) $(SEMANTICS_TEST_BINARY) $(CODEGEN_TEST_BINARY)
	./run_tests.sh

unit-test: $(TEST_BINARY)
	./$(TEST_BINARY)

semantics-test: $(SEMANTICS_TEST_BINARY)
	./$(SEMANTICS_TEST_BINARY)

codegen-test: $(CODEGEN_TEST_BINARY)
	./$(CODEGEN_TEST_BINARY)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TEST_BINARY): $(BIN_DIR) $(TEST_SOURCES)
	$(CC) $(CFLAGS_DEBUG) -o $(TEST_BINARY) $(TEST_SOURCES) $(LDFLAGS)

$(SEMANTICS_TEST_BINARY): $(BIN_DIR) $(SEMANTICS_TEST_SOURCES)
	$(CC) $(CFLAGS_DEBUG) -o $(SEMANTICS_TEST_BINARY) $(SEMANTICS_TEST_SOURCES) $(LDFLAGS)

$(CODEGEN_TEST_BINARY): $(BIN_DIR) $(CODEGEN_TEST_SOURCES)
	$(CC) $(CFLAGS_DEBUG) -o $(CODEGEN_TEST_BINARY) $(CODEGEN_TEST_SOURCES) $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)

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
