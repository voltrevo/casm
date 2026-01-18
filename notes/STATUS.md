# Project Status

## Current State
- **Lexer:** Complete (106 tests)
- **Parser:** Complete (functions, variables, expressions, returns)
- **Error handling:** Complete (accumulated errors, file:line:col format)
- **Memory:** Safe (sanitizers enabled, no leaks)
- **Tests:** All passing (<600ms)

## Completed
1. Lexer with source location tracking
2. Recursive descent parser with AST
3. Error accumulation + reporting
4. Memory management fixes
5. AddressSanitizer + UBSanitizer enabled
6. Test infrastructure with aggressive timeouts

## Next (In Order)
1. **Symbol table & type system** (types.c/h, semantics.c/h)
   - 2-pass semantic analysis
   - Type compatibility checking
   - Annotate AST with resolved types
   - See SYMBOL_TABLE.md

2. **C code generator** (codegen_c.c/h)
   - Walk type-checked AST
   - Emit valid C code

3. **WAT code generator** (codegen_wat.c/h)
   - Generate WebAssembly text format

## File Counts
| File | Lines |
|------|-------|
| src/lexer.c | 350 |
| src/parser.c | 740 |
| src/ast.c | 180 |
| src/main.c | 140 |
| src/utils.c | 50 |
| tests/test_lexer.c | 450 |
| Makefile | 55 |
| run_tests.sh | 75 |
| **Total** | **2,040** |

## Build Commands
```bash
make build          # Debug with sanitizers
make build-release  # Optimized
make test           # All tests
make clean          # Remove binaries
```

## Design Decisions
- Explicit types (no inference)
- Full block scoping
- Accumulate all errors
- AST annotation with resolved_type
- Warn on shadowing, error on duplicates
- No function overloading
