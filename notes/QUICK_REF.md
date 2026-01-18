# Quick Reference

## Build
```bash
make build          # Debug + sanitizers
make test           # Build + run tests
make clean          # Remove binaries
```

## Test an Example
```bash
./casm examples/simple_add.csm      # Parse + print AST
timeout 2 ./casm examples/foo.csm   # With timeout
```

## Add a Test
Create function in `tests/test_lexer.c` or `tests/test_semantics.c`, add to test array.

## Code Organization
- `src/lexer.c` - Tokenization
- `src/parser.c` - Syntax analysis
- `src/ast.c` - AST management
- `src/types.c` - Symbol table (TO CREATE)
- `src/semantics.c` - Semantic analysis (TO CREATE)
- `src/main.c` - CLI + pipeline
- `tests/` - Unit tests
- `examples/` - .csm test files

## Type Compatibility
- Signed ints (i8-i64) compatible with each other
- Unsigned ints (u8-u64) compatible with each other
- Signed and unsigned NOT compatible
- bool only compatible with bool
- No implicit conversions

## Debugging
```bash
make build-debug    # Default - has sanitizers
./casm file.csm     # ASAN reports memory errors
```

## Error Format
All errors: `filename:line:column: message`

## Verify Quality
```bash
make clean && make test 2>&1 | grep -i warning  # Should be empty
```
