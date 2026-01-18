# C to WebAssembly Compiler - Implementation Plan

## Project Overview

A toy C to WebAssembly (WAT) compiler written in C, targeting a useful subset of the C language.

### Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Implementation Language | C (C99) | Bootstrapping potential, portable, widely supported |
| Parser Strategy | 2-pass | Avoids forward declaration requirement |
| AST Representation | Explicit structs per node type | Type-safe, efficient |
| Memory Management | Manual malloc/free | C99 compatible, explicit control |
| WAT Output | Direct text generation | Simple, readable, debuggable |
| Integer Size | i32 (WebAssembly native) | Matches WebAssembly's type system |
| Error Handling | Exit with error messages + line/column info | Practical for CLI tool with debugging info |
| Testing Strategy | Simple test harness + program output testing | Direct validation of functionality |
| Build System | Makefile | Simple, portable, standard |

## Target Language Features (MVP)

### Phase 1: Tokenization (CURRENT)
- Keywords: `int`, `if`, `else`, `while`, `for`, `return`, `void`, `{`, `}`, `;`, `(`, `)`
- Operators: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`
- Literals: decimal integers, identifiers
- Comments: `//` and `/* */`
- Whitespace handling

### Phase 2: Parsing (AST)
- Global and local int variable declarations
- Function definitions with parameters and return types
- Binary/unary operations
- Variable assignment and usage
- Function calls
- (Control flow and statements come later)

### Phase 3: Symbol Table & Type System
- Track int variables, function parameters, return types
- Scope management for local vs global
- Basic type checking

### Phase 4: Code Generation
- AST → WAT text output
- Integer arithmetic operations
- Function calls and definitions
- Variable storage via local/global i32 stack

### Phase 5: Control Flow
- if/else statements
- while loops
- for loops

## Project Structure

```
casm/
├── src/
│   ├── main.c           # CLI entry point
│   ├── lexer.c/h        # Tokenization with location tracking
│   ├── parser.c/h       # AST building (recursive descent, 2-pass)
│   ├── codegen.c/h      # WAT generation
│   ├── types.c/h        # Type system & symbol table
│   ├── ast.c/h          # AST node definitions
│   └── utils.c/h        # Memory, string handling, etc.
├── tests/
│   ├── test_harness.c   # Simple test runner
│   ├── test_lexer.c     # Lexer tests
│   ├── test_parser.c    # Parser tests
│   └── test_codegen.c   # Code gen tests
├── examples/
│   ├── add.c            # Simple arithmetic
│   ├── factorial.c      # Recursion
│   ├── fibonacci.c      # Loops
│   └── loop.c           # Control flow
├── Makefile
├── README.md
└── PLAN.md (this file)
```

## Implementation Milestones

### Milestone 1: Tokenization ✓ In Progress
- [x] Set up project structure (Makefile, directories)
- [ ] Implement lexer with source location tracking (line, column)
- [ ] Create simple test harness
- [ ] Write test programs for tokenization
- [ ] Test invalid token sequences

**Definition of Done**: Can tokenize valid C programs and reject invalid ones with clear error messages.

### Milestone 2: Parsing
- [ ] Implement recursive descent parser
- [ ] Build AST node structures
- [ ] Implement 2-pass approach (signature collection + body parsing)
- [ ] Write parser tests

**Definition of Done**: Can parse valid C programs and produce correct AST; rejects invalid syntax.

### Milestone 3: Type System
- [ ] Implement symbol table
- [ ] Track variable scopes
- [ ] Implement basic type checking
- [ ] Write symbol table tests

**Definition of Done**: Can validate variable declarations and usage.

### Milestone 4: Code Generation (Simple)
- [ ] Implement WAT module generation
- [ ] Generate function definitions and calls
- [ ] Generate variable allocation and access
- [ ] Generate arithmetic operations
- [ ] Write codegen tests

**Definition of Done**: Can compile simple int programs to WAT and execute them.

### Milestone 5: Control Flow
- [ ] Implement if/else statements
- [ ] Implement while loops
- [ ] Implement for loops
- [ ] Write control flow tests

**Definition of Done**: Can handle conditionals and loops.

## Testing Strategy

### Test Harness Structure
```c
// Simple assertion-based testing
#define TEST(name) void test_##name(void)
#define ASSERT_EQ(actual, expected) ...
#define ASSERT_STR_EQ(actual, expected) ...
#define ASSERT_TRUE(condition) ...
#define ASSERT_FALSE(condition) ...

// Test runner
int main() {
    test_lexer_simple_int();
    test_lexer_identifiers();
    test_lexer_operators();
    // ... etc
}
```

### Test Approach

1. **Unit Tests**: Test individual components (lexer, parser, codegen)
2. **Program Tests**: Write small C programs, compile them, check WAT output
3. **Integration Tests**: Compile and validate full programs

### Example Test Programs

**tokenization tests:**
```c
// valid_add.c
int main() {
    return 5 + 3;
}

// valid_vars.c
int main() {
    int x = 5;
    int y = 3;
    return x + y;
}

// invalid_unclosed_paren.c
int main(
    return 5;
}
```

## Example Compilation

### Input (hello_add.c)
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int x = add(5, 3);
    return x;
}
```

### Expected WAT Output
```wasm
(module
  (func $add (param $a i32) (param $b i32) (result i32)
    (i32.add (local.get $a) (local.get $b))
  )
  (func $main (result i32)
    (local $x i32)
    (local.set $x (call $add (i32.const 5) (i32.const 3)))
    (local.get $x)
  )
  (export "main" (func $main))
)
```

## Key Implementation Details

### Lexer
- Track current position (line, column, offset in source)
- Each token stores its source location
- Handle single-line (`//`) and multi-line (`/* */`) comments
- Distinguish keywords from identifiers

### Parser
- **Pass 1**: Scan for function signatures and global variable declarations
- **Pass 2**: Parse function bodies and expressions
- Use recursive descent for expression parsing
- Operator precedence: standard C precedence

### Code Generator
- Walk AST, emit WAT instructions
- Maintain local variable stack frame
- Track function signatures for call validation

### Symbol Table
- Map variable names to types and storage locations
- Support nested scopes (function parameters, local variables)
- Function signatures stored globally

## Known Limitations

- No structs (yet)
- No pointers (yet)
- No arrays (yet)
- No strings (yet)
- Only `int` type (yet)
- No floating point
- No bitwise operations (yet)
- No preprocessor
- No optimization

## Next Steps After MVP

1. Add more types: `float`, `char`, `void`
2. Add arrays and indexing
3. Add pointers and dereferencing
4. Add structs
5. Add string literals
6. Add more operators
7. Optimization passes
8. Better error recovery
