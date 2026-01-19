# Quick Reference

## Build & Test

```bash
make build              # Debug build with sanitizers
make test               # Build and run all tests (includes coverage report)
make clean              # Remove binaries
```

## Code Coverage

Coverage is collected automatically during `make test` when DBG tests run. The branch coverage report is printed at the end of testing.

View the detailed HTML report:
```bash
firefox coverage_report/index.html
```

The report shows:
- **Branch Coverage %**: Primary metric - % of code branches executed
- **Line Coverage %**: % of lines executed  
- **Function Coverage %**: % of functions called
- Per-file statistics with line-by-line coverage detail

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
