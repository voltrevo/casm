# Quick Reference

## Build & Test

```bash
make build              # Debug build with sanitizers
make test               # Build and run all tests
make clean              # Remove binaries
```

## Running Examples

```bash
./bin/casm examples/simple_add.csm       # Compile and show AST
./bin/casm examples/if_statement.csm     # Try a control flow example
timeout 2 ./bin/casm examples/foo.csm    # With timeout
```

## Debug Checklist

```bash
# Verify no warnings or leaks
make clean && make test 2>&1 | grep -iE "(warning|leak|error)"

# Memory safety check
valgrind ./bin/casm examples/simple_add.csm
```

## Adding Tests

1. Add test function to `tests/test_lexer.c` or `tests/test_semantics.c`
2. Add to `tests` array in main
3. Run `make test`
