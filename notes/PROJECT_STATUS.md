# Casm Compiler Project Status

## Project Overview
Building a C-like compiler (Casm) that targets both C and WebAssembly. The compiler is written in C and uses a traditional pipeline: Lexer → Parser → Semantic Analysis → Code Generation.

## Current Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     LEXER (src/lexer.c)                         │
│  Tokenizes source code with source location tracking (line:col) │
│  106 unit tests passing, 0 failures                             │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PARSER (src/parser.c)                        │
│  Recursive descent parser building AST                          │
│  Supports: functions, variables, expressions, return statements │
│  Error accumulation: reports all errors at once                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│              SEMANTIC ANALYZER [TO BE IMPLEMENTED]              │
│  - Build symbol table (functions and variables)                 │
│  - Validate type compatibility                                  │
│  - Report semantic errors (undefined vars, type mismatches)     │
│  - Annotate AST with resolved types                             │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│              CODE GENERATORS [TO BE IMPLEMENTED]                │
│  - C Code Generator (primary target)                            │
│  - WAT Code Generator (WebAssembly, secondary)                  │
└─────────────────────────────────────────────────────────────────┘
```

## Compiler Features Implemented

### Language Features
- ✅ **Types:** i8, i16, i32, i64, u8, u16, u32, u64, bool, void
- ✅ **Functions:** Definitions with typed parameters and return types
- ✅ **Variables:** Declaration with optional initializers
- ✅ **Expressions:**
  - Binary operators: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`
  - Unary operators: `-` (negation), `!` (logical not)
  - Function calls with arguments
  - Integer and boolean literals
  - Variable references
- ✅ **Comments:** Single-line (`//`) and multi-line (`/* */`)
- ❌ Control flow: if/else, while, for loops (not yet implemented)
- ❌ Module system: `#import` statements (parsed but not implemented)

### Compiler Infrastructure
- ✅ **Source tracking:** Every token and AST node has line:column:offset location
- ✅ **Error handling:** 
  - Lexer errors reported with location
  - Parser accumulates all errors before reporting
  - Error format: `filename:line:column: message`
- ✅ **Memory management:**
  - All allocations tracked with xmalloc/xfree
  - AST cleanup implemented
  - No memory leaks detected by AddressSanitizer
- ✅ **Compiler flags:**
  - Debug build: AddressSanitizer + UBSanitizer enabled
  - Release build: -O3 optimization
- ✅ **Testing:**
  - 106 unit tests (lexer)
  - 4 example files that parse successfully
  - All tests run in <600ms with aggressive timeouts
  - Build system: clean separation of build/test targets

## Files and Line Counts

| File | Lines | Purpose |
|------|-------|---------|
| src/lexer.c | ~350 | Tokenization with source location |
| src/lexer.h | ~40 | Lexer API |
| src/parser.c | ~740 | Recursive descent parser |
| src/parser.h | ~40 | Parser API and error list |
| src/ast.c | ~180 | AST node management |
| src/ast.h | ~220 | AST structure definitions |
| src/utils.c | ~50 | Memory allocation helpers |
| src/utils.h | ~20 | Utils API |
| src/main.c | ~140 | CLI and AST printing |
| tests/test_lexer.c | ~450 | Lexer unit tests |
| Makefile | ~55 | Build system |
| run_tests.sh | ~75 | Test runner with timeouts |
| **Total** | **~2,300** | **Functional compiler frontend** |

## Completed Tasks (Commits)

1. **Initial setup** - Project structure, Makefile, basic lexer
2. **Lexer implementation** - Full tokenization with 106 tests passing
3. **Parser implementation** - Recursive descent parser with AST generation
4. **Error output improvement** - Changed to `filename:line:column: message` format
5. **Memory management** - Fixed double-free issues in AST cleanup
6. **Sanitizer setup** - AddressSanitizer + UBSanitizer for debug builds
7. **Test infrastructure** - Aggressive timeouts, fast execution (~600ms)
8. **Code cleanup** - Removed unused functions, zero compiler warnings

## Remaining High-Priority Tasks

### 1. Symbol Table & Type System (IN PROGRESS)
**Status:** Design complete, ready for implementation
**Files to create:** 
- src/types.h/c (symbol table structures)
- src/semantics.h/c (semantic analyzer)
**Files to modify:**
- src/ast.h (add resolved_type to expressions)
- src/main.c (integrate semantic analysis)
- Makefile (add new source files)
**Scope:** ~600-800 lines of code
**Test coverage:** 30-40 semantic analysis test cases
**Documentation:** See notes/SYMBOL_TABLE_IMPLEMENTATION.md

### 2. C Code Generator
**Status:** Not started
**Purpose:** Walk type-checked AST and generate valid C code
**Key requirements:**
- Map Casm types to C types (i32 → int32_t, etc.)
- Generate function definitions and prototypes
- Generate variable declarations
- Generate expression code
- Generate statements (return, etc.)
**Output:** Valid C code that can be compiled with gcc/clang
**Scope:** ~1000 lines of code
**Testing:** Compare generated C to expected output

### 3. WAT Code Generator
**Status:** Not started
**Purpose:** Generate WebAssembly text format
**Key requirements:**
- Similar to C codegen but generates WAT syntax
- Handle stack-based architecture of WASM
- Function calls and control flow
**Output:** Valid .wat files that can be assembled with wasm-as
**Scope:** ~1000-1200 lines of code
**Priority:** Secondary (after C codegen)

## Known Limitations and TODOs

1. **No control flow statements yet**
   - Parser doesn't handle if/else, while, for
   - Symbol table implementation should support block scoping for future support

2. **No module system**
   - Parser recognizes `#import` but doesn't load modules
   - Would need module path resolution and multi-file compilation

3. **No assignments**
   - Variable assignments not implemented
   - Assignments after initialization only via operators
   - TODO: Implement assignment operator

4. **No string literals**
   - Only integer and boolean literals supported
   - Adding strings would require constant pool management

5. **No type inference**
   - All types must be explicit
   - Design decision: keep it simple for now

6. **No function overloading**
   - Duplicate function names are errors
   - Design decision: keep name resolution simple

## Test Coverage

### Unit Tests (tests/test_lexer.c)
- 106 test cases covering all lexer features
- Tests: integers, identifiers, keywords, operators, comments, location tracking
- All passing with <600ms execution time

### Integration Tests (run_tests.sh)
- Tests 4 example files that use only implemented features:
  - simple_add.csm (basic arithmetic)
  - variables.csm (variable declarations and usage)
  - function_call.csm (function definitions and calls)
  - mixed_types.csm (multiple type usage)
- All passing

### Example Files (examples/)
- 9 total example files
- 4 with supported features (above)
- 5 with unsupported features (if/while/for) - these currently timeout/fail

## Build System Details

### Makefile Targets
```
make build           # Builds debug version (default)
make build-debug     # Explicit debug build with sanitizers
make build-release   # Release build with -O3, no sanitizers
make test            # Builds and runs all tests
make unit-test       # Builds and runs only unit tests
make clean           # Removes built binaries
make help            # Shows help
```

### Compiler Flags
**Debug (-Wall -Wextra -pedantic -std=c99 -g -fsanitize=address -fsanitize=undefined)**
- Aggressive warnings
- Full debug symbols
- Memory error detection (ASan)
- Undefined behavior detection (UBSan)

**Release (-Wall -Wextra -pedantic -std=c99 -O3)**
- Aggressive optimizations
- No debug symbols
- No runtime checks

### Test Execution
```bash
make test              # Compiles compiler + tests, runs all tests
./run_tests.sh         # Assumes binaries built, runs tests only
timeout 600ms make test  # Enforce 600ms timeout on full test suite
```

## Design Decisions (Final)

### Parser Design
- **2-pass approach:** Tokenize entire source first, then parse linearly
- **Error accumulation:** Collect all parse errors, report at end
- **No forward declarations:** Future 2-pass compilation will allow forward refs
- **Recursive descent:** Simple, understandable, easy to extend

### Type System
- **Explicit types:** No type inference
- **No overloading:** Functions must have unique names
- **No implicit conversions:** Types must be explicitly compatible
- **No generics:** Keep simple for now

### Memory Management
- **Value semantics for embedded structs:** AST statements/expressions embedded in arrays
- **Pointer semantics for referenced nodes:** Binary ops hold pointers to sub-expressions
- **Clear lifetime ownership:** Parser owns created nodes, semantic analyzer reads them, codegen consumes them
- **Sanitizers enabled by default:** Catch memory errors immediately

### Error Handling
- **Accumulate all errors:** Don't stop on first error
- **Consistent formatting:** All errors in `file:line:col: message` format
- **Location tracking:** Every error has source location for IDE integration
- **Separation of concerns:** Lexer errors, parser errors, semantic errors separate

## Getting Started for New Contributors

1. **Understand the codebase:**
   - Read src/lexer.h/c (input tokenization)
   - Read src/parser.h/c (syntax analysis)
   - Read src/ast.h/c (tree representation)
   - Read src/main.c (pipeline integration)

2. **For Symbol Table implementation:**
   - Read notes/SYMBOL_TABLE_IMPLEMENTATION.md (detailed guide)
   - Understand current AST structure (src/ast.h)
   - Implement types.h/c and semantics.h/c following the guide
   - Add tests in tests/test_semantics.c
   - Integrate into main.c pipeline

3. **For Code Generation:**
   - Understand the validated AST from semantic analysis
   - Implement codegen_c.h/c (for C output)
   - Implement codegen_wat.h/c (for WebAssembly output)
   - Test with example files

4. **Build and test:**
   - `make build` to compile
   - `make test` to run tests
   - `./casm examples/simple_add.csm` to try compiler
   - Look at Makefile and run_tests.sh for build details

## Performance Baseline

- **Lexer:** <50ms for typical file
- **Parser:** <50ms for typical file
- **Full test suite:** <600ms including compilation
- **Memory:** <5MB for typical program

## Future Enhancements (Out of Scope)

- Incremental compilation / build system
- Optimization passes
- Code coverage analysis
- Debugging information (DWARF)
- LSP (Language Server Protocol) support
- IDE plugins
- Package manager
- Standard library
