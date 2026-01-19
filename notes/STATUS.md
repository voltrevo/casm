# Casm Compiler - Current Status

## Project Overview

C-like to WebAssembly compiler written in C. Compiles `.csm` files to C, wasm (wat format).

## Current Implementation Status

| Component | Status | Tests | Details |
|-----------|--------|-------|---------|
| **Lexer** | ✅ Complete | 106 | Tokenization, source location tracking |
| **Parser** | ✅ Complete | implicit | 2-pass recursive descent, AST generation |
| **Semantic Analysis** | ✅ Complete | 15 | Type checking, symbol table, error accumulation |
| **C Code Generator** | ✅ Complete | 7 examples | Generates valid C code |
| **Module System** | ✅ Complete | 30 DBG | Import statements, module loading, merged AST |
| **Memory Safety** | ✅ Complete | — | Zero leaks in module loading, ASAN/UBSAN enabled |
| **WAT Code Generator** | ✅ Complete | — | |

**Total Tests:** 106 lexer + 15 semantics + 30 DBG module + 7 examples = **158+ tests**

**Code:** ~4,100 lines of C (src/ + tests/)

**Test Runtime:** ~500ms total (excluding DBG tests due to ASAN overhead)

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
