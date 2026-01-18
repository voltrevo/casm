# Casm (C-like to WebAssembly) Compiler - Implementation Plan

## Project Overview

A compiler written in C, compiling Casm (a C-like language) to WebAssembly (WAT) and C. Casm is inspired by C but deviates significantly:
- **No forward declarations needed** (2-pass parser)
- **Explicit type system**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `bool`, `void`
- **Module imports** instead of `#include`: `#import "./module.csm"`
- **File extension**: `.csm`

### Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Implementation Language | C (C99) | Bootstrapping potential, portable, widely supported |
| Parser Strategy | 2-pass | Avoids forward declaration requirement |
| AST Representation | Explicit structs per node type | Type-safe, efficient |
| Memory Management | Manual malloc/free | C99 compatible, explicit control |
| Default Output | WAT text | Simple, readable, debuggable |
| Secondary Output | C code | For testing, validation, and alternative compilation path |
| Error Handling | Exit with error messages + line/column info | Practical for CLI tool with debugging info |
| Testing Strategy | Simple test harness + program output testing | Direct validation of functionality |
| Build System | Makefile | Simple, portable, standard |

## Casm Language Features

### Type System
- **Integer types**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`
- **Boolean type**: `bool` with literals `true`, `false`
- **Void type**: `void` for functions with no return value
- **Explicit type declarations**: All variables must have explicit types

### Phase 1: Tokenization ✅ COMPLETE
- Keywords: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `bool`, `void`, `true`, `false`, `if`, `else`, `while`, `for`, `return`, `#import`
- Operators: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`
- Literals: decimal integers, identifiers, boolean literals
- Comments: `//` and `/* */`
- Whitespace handling

### Phase 2: Parsing (AST) - IN PROGRESS
- Function definitions with parameters and return types
- Variable declarations with explicit types
- Binary/unary operations
- Variable assignment and usage
- Function calls
- Module imports: `#import "./module.csm"`

### Phase 3: Symbol Table & Type System
- Track variables with types
- Scope management for local vs global
- Function signatures
- Type checking and validation

### Phase 4: Code Generation - C Backend (FIRST)
- AST → C code output
- Proper variable declarations with types
- Function definitions and calls
- Basic type conversions where needed

### Phase 5: Code Generation - WAT Backend
- AST → WAT text output
- Type-appropriate WebAssembly instructions
- Variable storage via local/global stack
- Module structure

### Phase 6: Control Flow
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
│   ├── codegen_c.c/h    # C code generation
│   ├── codegen_wat.c/h  # WAT code generation
│   ├── types.c/h        # Type system & symbol table
│   ├── ast.c/h          # AST node definitions
│   └── utils.c/h        # Memory, string handling, etc.
├── tests/
│   ├── test_harness.h   # Simple test runner
│   ├── test_lexer.c     # Lexer tests
│   ├── test_parser.c    # Parser tests
│   ├── test_codegen_c.c # C codegen tests
│   └── test_codegen_wat.c # WAT codegen tests
├── examples/
│   ├── simple_add.csm       # Simple arithmetic
│   ├── function_call.csm    # Function definitions and calls
│   ├── if_statement.csm     # Conditional logic
│   ├── while_loop.csm       # Loop control
│   ├── for_loop.csm         # Loop variants
│   └── all_operators.csm    # Operator testing
├── Makefile
├── README.md
└── PLAN.md (this file)
```

## Implementation Milestones

### Milestone 1: Tokenization ✅ DONE
- [x] Set up project structure
- [x] Implement lexer with source location tracking
- [x] Create simple test harness
- [x] Write test programs for tokenization
- [x] 94 unit tests passing

### Milestone 2: Update Lexer for New Type System
- [ ] Update keywords: remove `int`, add `i8`-`i64`, `u8`-`u64`, `bool`, `true`, `false`
- [ ] Update example files to use `.csm` extension
- [ ] Update tokenization tests for new keywords
- [ ] Test that old `int` keyword is rejected

### Milestone 3: Parsing
- [ ] Implement recursive descent parser
- [ ] Build AST node structures
- [ ] Implement 2-pass approach (signature collection + body parsing)
- [ ] Write parser tests
- [ ] Support module imports parsing

**Definition of Done**: Can parse valid Casm programs and produce correct AST; rejects invalid syntax.

### Milestone 4: Type System & Symbol Table
- [ ] Implement type representation
- [ ] Implement symbol table with type tracking
- [ ] Track variable scopes
- [ ] Implement type checking
- [ ] Write symbol table tests

**Definition of Done**: Can validate variable declarations, types, and usage.

### Milestone 5: C Code Generation
- [ ] Implement C code output generation
- [ ] Generate function definitions
- [ ] Generate variable declarations with types
- [ ] Generate arithmetic operations
- [ ] Generate function calls
- [ ] Write C codegen tests

**Definition of Done**: Can compile simple Casm programs to valid C code.

### Milestone 6: WAT Code Generation
- [ ] Implement WAT module generation
- [ ] Generate function definitions with proper types
- [ ] Generate variable allocation and access
- [ ] Generate type-specific arithmetic operations
- [ ] Write WAT codegen tests

**Definition of Done**: Can compile simple Casm programs to valid WAT.

### Milestone 7: Control Flow
- [ ] Implement if/else statements
- [ ] Implement while loops
- [ ] Implement for loops
- [ ] Write control flow tests

**Definition of Done**: Can handle conditionals and loops.

## Testing Strategy

### Test Approach

1. **Unit Tests**: Test individual components (lexer, parser, codegen)
2. **Program Tests**: Write small Casm programs, compile to C and WAT, validate
3. **Integration Tests**: Compile and validate full programs
4. **Cross-validation**: Generate C, compile with gcc, run and compare output

### Example Casm Program

**Input (add.csm)**
```c
i32 add(i32 a, i32 b) {
    return a + b;
}

i32 main() {
    i32 x = add(5, 3);
    return x;
}
```

**Expected C Output**
```c
#include <stdint.h>

int32_t add(int32_t a, int32_t b) {
    return a + b;
}

int32_t main() {
    int32_t x = add(5, 3);
    return x;
}
```

**Expected WAT Output**
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
- **Updated for Casm**: Recognize `i8`-`i64`, `u8`-`u64`, `bool`, `true`, `false`, `#import`

### Parser
- **Pass 1**: Scan for function signatures and global variable declarations
- **Pass 2**: Parse function bodies and expressions
- Use recursive descent for expression parsing
- Operator precedence: standard C precedence
- Parse module imports

### Code Generator (C)
- Walk AST, emit C code
- Map Casm types to C types (i32 → int32_t, etc.)
- Maintain proper scoping for variables
- Generate function prototypes and definitions

### Code Generator (WAT)
- Walk AST, emit WAT instructions
- Map Casm types to WAT types (i32 → i32, bool → i32, etc.)
- Maintain local variable stack frame
- Track function signatures for call validation

### Type System
- Represent types as enum (I32, I64, U32, U64, BOOL, VOID, etc.)
- Map types to C equivalents and WAT equivalents
- Validate type compatibility in operations

### Symbol Table
- Map variable names to types and storage locations
- Support nested scopes (function parameters, local variables)
- Function signatures stored globally with parameter and return types

## Known Limitations (MVP)

- No structs
- No pointers/dereferencing
- No arrays/indexing
- No string literals
- No floating point
- No bitwise operations
- No preprocessor
- No optimization
- Single-file compilation only (no separate compilation units yet)

## Future Enhancements

1. Additional types: `f32`, `f64`, `char`, `string`
2. Arrays and indexing
3. Pointers and dereferencing
4. Structs and methods
5. Module system with incremental compilation
6. Optimization passes
7. Better error recovery
8. Inline assembly
9. Built-in functions/intrinsics
