# Quick Reference

## Build
```bash
make build          # Debug + sanitizers
make test           # Build + run all tests
make clean          # Remove binaries
make semantics-test # Run semantics tests only
```

## Test an Example
```bash
./bin/casm examples/simple_add.csm      # Parse + print AST
timeout 2 ./bin/casm examples/foo.csm   # With timeout
./bin/casm examples/if_statement.csm    # Unsupported features show error
```

## Add a Test
Create function in `tests/test_lexer.c` or `tests/test_semantics.c`, add to test array.

## Code Organization
- `src/lexer.c` - Tokenization (350 lines)
- `src/parser.c` - Syntax analysis (770 lines)
- `src/ast.c` - AST management (180 lines)
- `src/types.c` - Symbol table (200 lines)
- `src/semantics.c` - Semantic analysis (300 lines)
- `src/main.c` - CLI + pipeline (157 lines)
- `tests/test_lexer.c` - Lexer tests (450 lines)
- `tests/test_semantics.c` - Semantics tests (350 lines)
- `examples/` - .csm test files (9 files)
- `bin/` - Compiled binaries (gitignored)

## Type Compatibility
- Signed ints (i8-i64) compatible with each other
- Unsigned ints (u8-u64) compatible with each other
- Signed and unsigned NOT compatible
- bool only compatible with bool
- No implicit conversions

## Debugging
```bash
make build-debug           # Default - has sanitizers
./bin/casm file.csm        # ASAN reports memory errors
valgrind ./bin/casm file   # Full leak detection
```

## Error Format
All errors: `filename:line:column: message`

## Verify Quality
```bash
make clean && make test 2>&1 | grep -iE "(warning|leak|error)" # Should be empty
```

## Features Status
| Feature | Status | Notes |
|---------|--------|-------|
| Functions | ✓ Complete | Parameters, returns, calls |
| Variables | ✓ Complete | Declarations, initializers |
| Expressions | ✓ Complete | Binary/unary ops, calls, literals |
| Types | ✓ Complete | i8-i64, u8-u64, bool, void |
| Scoping | ✓ Complete | Block scoping, upward lookup |
| Type checking | ✓ Complete | Full validation + error reporting |
| Memory safety | ✓ Complete | Zero leaks, ASAN/UBSAN enabled |
| if/while/for | ✗ Planned | Parser gracefully reports "not yet implemented" |
| Code generation | ✗ Next | C and WAT targets |

