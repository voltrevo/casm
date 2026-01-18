# Quick Reference for Development

## Common Development Tasks

### Building and Testing
```bash
make build          # Debug build with sanitizers
make build-release  # Optimized release build
make test           # Full test suite
make unit-test      # Just unit tests
make clean          # Remove binaries
```

### Running Examples
```bash
./casm examples/simple_add.csm      # Parse and print AST
timeout 2 ./casm examples/foo.csm   # With timeout protection
```

### Adding a New Test
1. **Lexer test:** Add to tests/test_lexer.c, add function name to run_all_tests()
2. **Semantic test:** Will go in tests/test_semantics.c (to be created)
3. **Example test:** Add .csm file to examples/, update run_tests.sh if needed

### Memory Debugging
```bash
# Build debug version (default)
make build-debug

# Run with AddressSanitizer (automatic with debug build)
./casm examples/simple_add.csm
```

### Checking for Warnings
```bash
make clean && make build-debug 2>&1 | grep -i warning
# Should produce no output (clean build)
```

## Code Organization

### src/ Directory
- **lexer.c/h** - Tokenization
- **parser.c/h** - Syntax analysis + error list
- **ast.c/h** - AST node management
- **types.c/h** - Symbol table (TO BE CREATED)
- **semantics.c/h** - Semantic analysis (TO BE CREATED)
- **codegen_c.c/h** - C code generation (TO BE CREATED)
- **codegen_wat.c/h** - WebAssembly generation (TO BE CREATED)
- **utils.c/h** - Memory allocation helpers
- **main.c** - CLI and pipeline orchestration

### tests/ Directory
- **test_lexer.c** - Lexer unit tests (106 tests)
- **test_semantics.c** - Semantic analysis tests (TO BE CREATED)
- **test_codegen_c.c** - C codegen tests (TO BE CREATED)

### examples/ Directory
- **simple_add.csm** - Basic arithmetic
- **variables.csm** - Variable declarations
- **function_call.csm** - Function definitions and calls
- **mixed_types.csm** - Multiple type usage
- **all_operators.csm** - All operators (unsupported control flow)
- **bool_type.csm** - Boolean type (unsupported if/else)
- etc.

## Key Data Structures

### Token (lexer.h)
```c
typedef struct {
    TokenType type;
    char* lexeme;
    int lexeme_len;
    SourceLocation location;
    long numeric_value;  // for integer literals
} Token;
```

### AST Node Example (ast.h)
```c
struct ASTExpression {
    ExpressionType type;
    SourceLocation location;
    CasmType resolved_type;  // Set by semantic analyzer
    union {
        ASTBinaryOp binary_op;
        ASTUnaryOp unary_op;
        ASTFunctionCall function_call;
        ASTLiteral literal;
        ASTVariable variable;
    } as;
};
```

### Symbol Table (types.h - TO BE CREATED)
```c
typedef struct {
    FunctionSymbol* functions;
    int function_count;
    Scope* global_scope;
    Scope* current_scope;
} SymbolTable;
```

## Error Message Examples

### Lexer Error
```
example.csm:5:10: Unexpected character '@'
```

### Parser Error
```
example.csm:12:3: Expected ')' after parameters
```

### Semantic Error (TO BE IMPLEMENTED)
```
example.csm:8:5: Undefined variable 'x'
example.csm:15:10: Type mismatch: cannot add i32 and bool
example.csm:20:1: Function 'foo' already defined at line 10
```

## Testing Checklist

When adding a feature:

- [ ] Add lexer tests if new tokens needed
- [ ] Add parser tests if new grammar needed
- [ ] Add semantic tests for type checking
- [ ] Add codegen tests for output correctness
- [ ] Update example files if applicable
- [ ] Run `make test` - all tests should pass
- [ ] Run with timeout - should complete in <600ms
- [ ] Check for memory leaks - no ASAN errors
- [ ] Check for warnings - `make clean && make build-debug 2>&1 | grep warning` should be empty

## Type System Compatibility

### Signed Integers
- i8, i16, i32, i64 are mutually compatible
- Can be used in arithmetic operations
- Result type is the widest input type

### Unsigned Integers
- u8, u16, u32, u64 are mutually compatible
- Can be used in arithmetic operations
- Result type is the widest input type

### Boolean
- bool is only compatible with bool
- Used in logical operations (&&, ||, !)
- Comparison operators produce bool

### Void
- void is only for function return types
- Cannot be used in expressions

### Incompatibilities
- Signed and unsigned integers cannot mix
- Numbers and bool cannot mix
- Examples of type errors:
  - `i32 x = true` - ERROR: cannot assign bool to i32
  - `bool b = 5 + 3` - ERROR: cannot assign i32 to bool
  - `i32 y = x + true` - ERROR: cannot add i32 and bool

## Adding New Token Type

1. Add to TokenType enum in src/lexer.h
2. Add keyword to keyword_map array in src/lexer.c
3. Add case to lexer_next_token() if special handling needed
4. Add test in tests/test_lexer.c
5. Run `make test` to verify

## Adding New AST Node Type

1. Add to ASTNode type enum in src/ast.h
2. Define struct for node data
3. Create constructor/destructor in src/ast.c
4. Add case to freeing functions
5. Update parser to create node in src/parser.c
6. Add test for parsing

## Adding New Binary Operator

1. Add to BinaryOpType enum in src/ast.h
2. Add tokenization in src/lexer.c (if new token type)
3. Add parsing in appropriate precedence level in src/parser.c
4. Add type checking in src/semantics.c (to be created)
5. Add code generation in src/codegen_c.c or codegen_wat.c
6. Add test case

## Debugging Tips

### AST Printing
The main.c program prints the AST in human-readable format. This is useful for:
- Understanding what the parser generated
- Checking node nesting and structure
- Verifying type annotations

### Memory Debugging
```bash
# Build with debug symbols and sanitizers (default)
make build-debug

# Run a specific example
./casm examples/simple_add.csm

# ASAN will report any memory errors with stack trace
```

### Parser Debugging
Add temporary error messages in parser.c:
```c
fprintf(stderr, "DEBUG: current token = %s\n", token_type_name(current_token(parser).type));
```

### Lexer Debugging
Check lexeme output:
```c
fprintf(stderr, "DEBUG: lexeme '%.*s' length %d\n", token.lexeme_len, token.lexeme, token.lexeme_len);
```

## Performance Profiling

```bash
# Time the entire build + test
time make test

# Time just parsing
time ./casm examples/mixed_types.csm >/dev/null

# Profile with debug build (has overhead)
perf record ./casm examples/mixed_types.csm
perf report
```

## Common Mistakes

1. **Forgetting to free allocated memory** - Use xfree() for all xmalloc() calls
2. **Modifying global state in tests** - Use setup/teardown functions
3. **Type mismatches in comparisons** - Keep signed/unsigned separate
4. **String length off-by-one** - Remember strings aren't null-terminated in tokens
5. **Scope issues in symbol table** - Push before entering scope, pop when exiting
