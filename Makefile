.PHONY: all clean test build

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -I./src
LDFLAGS = -lm

# Source files
SOURCES = src/main.c src/lexer.c src/utils.c
TEST_SOURCES = tests/test_lexer.c src/lexer.c src/utils.c

# Output
MAIN_BINARY = casm
TEST_BINARY = test_casm

all: build

build: $(MAIN_BINARY)

$(MAIN_BINARY): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_BINARY)
	./$(TEST_BINARY)

$(TEST_BINARY): $(TEST_SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(MAIN_BINARY) $(TEST_BINARY)

run-example: build
	./$(MAIN_BINARY) examples/simple.c

help:
	@echo "Available targets:"
	@echo "  build       - Build the compiler"
	@echo "  test        - Run tests"
	@echo "  clean       - Remove built files"
	@echo "  run-example - Compile and run an example"
