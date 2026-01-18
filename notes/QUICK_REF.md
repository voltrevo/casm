# Quick Reference

## Build & Test

```bash
make build              # Debug build with sanitizers
make test               # Build and run all 128 tests
make clean              # Remove binaries
```

## Running Examples

```bash
./bin/casm examples/simple_add.csm       # Compile and show AST
./bin/casm examples/if_statement.csm     # Try a control flow example
timeout 2 ./bin/casm examples/foo.csm    # With timeout
```

## Testing

- **Unit tests:** 106 lexer + 15 semantics tests
- **Integration:** 7 example programs
- **Runtime:** ~500ms total
- **Quality:** Zero warnings, zero leaks, ASAN/UBSAN enabled

## File Organization

| File | Purpose | Size |
|------|---------|------|
| `src/lexer.c` | Tokenization | 350 lines |
| `src/parser.c` | Syntax analysis | 1000+ lines |
| `src/semantics.c` | Type checking & scopes | 380 lines |
| `src/codegen.c` | C code generation | 280 lines |
| `src/types.c` | Symbol table & types | 230 lines |
| `src/ast.c` | AST structures | 230 lines |
| `src/main.c` | CLI pipeline | 150 lines |
| `tests/test_lexer.c` | Lexer tests | 315 lines |
| `tests/test_semantics.c` | Semantic tests | 408 lines |

## Type System

- **Integer types:** `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- **Boolean:** `bool` (literals: `true`, `false`)
- **Void:** `void` (for functions with no return)

**Type Compatibility:**
- Signed integers compatible with each other
- Unsigned integers compatible with each other
- Signed and unsigned NOT compatible
- Bool/void only compatible with themselves

## Error Format

All errors use standard format: `filename:line:column: message`

## Debug Checklist

```bash
# Verify no warnings or leaks
make clean && make test 2>&1 | grep -iE "(warning|leak|error)"

# Memory safety check
valgrind ./bin/casm examples/simple_add.csm
```

## Features Status

| Feature | Status | Notes |
|---------|--------|-------|
| Lexer | ✅ Complete | All tokens, source tracking |
| Parser | ✅ Complete | 2-pass, recursive descent |
| Functions | ✅ Complete | Parameters, returns, calls |
| Variables | ✅ Complete | Declarations, initialization |
| Expressions | ✅ Complete | Binary/unary ops, function calls |
| Type Checking | ✅ Complete | Full validation + error reporting |
| Control Flow | ✅ Complete | if/else, while, for loops |
| C Code Gen | ✅ Complete | Generates valid C code |
| WAT Code Gen | ❌ Next | WebAssembly text format |
| Memory Safety | ✅ Complete | Zero leaks, sanitizers enabled |

## Adding Tests

1. Add test function to `tests/test_lexer.c` or `tests/test_semantics.c`
2. Add to `tests` array in main
3. Run `make test`

## Common Tasks

**Debug a specific file:**
```bash
./bin/casm path/to/file.csm
```

**Count lines of code:**
```bash
find src tests -name "*.c" | xargs wc -l
```

**Check for compiler warnings:**
```bash
make clean && make build 2>&1 | grep warning
```
