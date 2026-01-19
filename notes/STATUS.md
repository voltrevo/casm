# Casm Compiler - Current Status

## Project Overview

A C-like to WebAssembly compiler written in C. Compiles `.csm` files to C and WAT output.

**Language Features:**
- Explicit type system: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `bool`, `void`
- Functions with parameters and return types
- Variables with explicit type declarations
- Control flow: `if`/`else`, `while`, `for`
- Binary and unary operators with full type checking
- Block-scoped variables with proper scoping rules

## Current Implementation Status

| Component | Status | Tests | Details |
|-----------|--------|-------|---------|
| **Lexer** | ✅ Complete | 106 | Tokenization, source location tracking |
| **Parser** | ✅ Complete | implicit | 2-pass recursive descent, AST generation |
| **Semantic Analysis** | ✅ Complete | 15 | Type checking, symbol table, error accumulation |
| **C Code Generator** | ✅ Complete | 7 examples | Generates valid C code |
| **Module System** | ✅ Complete | 30 DBG | Import statements, module loading, merged AST |
| **Memory Safety** | ✅ Complete | — | Zero leaks in module loading, ASAN/UBSAN enabled |
| **WAT Code Generator** | ❌ Not started | — | Next phase |

**Total Tests:** 106 lexer + 15 semantics + 30 DBG module + 7 examples = **158+ tests**

**Code:** ~4,100 lines of C (src/ + tests/)

**Test Runtime:** ~500ms total (excluding DBG tests due to ASAN overhead)

## Completed Features

1. ✅ Full lexer with line/column tracking
2. ✅ Recursive descent parser with 2-pass approach
3. ✅ Error accumulation and proper error reporting
4. ✅ Symbol table with scoping (global + nested blocks)
5. ✅ Type system with 10 types
6. ✅ 2-pass semantic analysis (signatures + body validation)
7. ✅ Type checking for all operations
8. ✅ Uninitialized variable detection
9. ✅ Integer overflow detection
10. ✅ Strict type compatibility rules
11. ✅ C code generation (fully functional)
12. ✅ Control flow statements (`if`/`else`, `while`, `for`)
13. ✅ Module system with circular import detection
14. ✅ Recursive module loading with merged AST
15. ✅ Memory safety (zero leaks in module loading with ASAN)
16. ✅ DBG statement parsing for debugging output

## Known Limitations

1. **Type Narrowing**: i64 literals can implicitly narrow to smaller integer types due to default literal typing
2. **No Explicit Casts**: Language lacks explicit cast operator for intentional narrowing
3. **Module System Limitations**: Basic import/export, no visibility control
4. **Call Graph Leak**: Small leak (~40 bytes) in `call_graph.c` during callees array tracking (marked TODO)
5. **No Advanced Features**: No structs, arrays, pointers, strings, floats, or bitwise ops

## Next Steps

### Immediate (In Progress)
- Fix remaining memory leak in `call_graph.c` (40 bytes in callees array)
- Investigate DBG test failures (compile error output mismatch)
- Complete WAT code generation

### Phase 1: WAT Code Generator
- Implement `src/codegen_wat.c/h`
- Generate WebAssembly text format output
- Map Casm types to WAT types
- Test with existing example programs

### Phase 2: Enhanced Error Recovery
- Better error messages with context
- Suggestion system for common mistakes
- Recovery strategies for parser

### Phase 3: Standard Library
- Built-in functions (print, math, etc.)
- Module system for code reuse

## File Organization

**Source Code:**
- `src/lexer.c/h` — Tokenization (350 lines)
- `src/parser.c/h` — Syntax analysis & AST (1400+ lines)
- `src/semantics.c/h` — Type checking & symbol table (600+ lines)
- `src/codegen.c/h` — C code generation (280 lines)
- `src/ast.c/h` — AST data structures & freeing (290 lines)
- `src/types.c/h` — Type system (230 lines)
- `src/module_loader.c/h` — Module loading & AST merging (440 lines)
- `src/call_graph.c/h` — Call graph building (250 lines)
- `src/name_allocator.c/h` — Symbol deduplication (170 lines)
- `src/utils.c/h` — Utilities & memory management
- `src/main.c` — CLI entry point (190 lines)

**Tests:**
- `tests/test_lexer.c` — 106 unit tests
- `tests/test_semantics.c` — 15 semantic tests
- `tests/test_memory_leaks.c` — 3 memory leak tests (NEW)
- `tests/dbg_cases/` — 30+ DBG integration tests

**Examples:** 13 example programs in `examples/` (7 tested, 6 issue-specific)

## Build & Test Commands

```bash
make build              # Debug with sanitizers
make test               # Run all tests
make clean              # Remove binaries
```

## Example Programs

All examples compile to valid C and run successfully:
- `simple_add.csm` — Basic arithmetic
- `variables.csm` — Variable declarations
- `function_call.csm` — Functions with parameters
- `mixed_types.csm` — Multiple integer types
- `if_statement.csm` — Conditional logic
- `while_loop.csm` — Loop control
- `for_loop.csm` — Loop variants

(Plus 6 examples for specific issue testing)

## Design Principles

- **Explicit types everywhere** — No type inference
- **Block scoping** — Variables scoped to their block
- **Accumulate errors** — Report all errors before exiting
- **Safe memory** — Manual malloc/free with zero leaks
- **Error recovery** — Graceful handling of invalid input
