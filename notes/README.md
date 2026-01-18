# Documentation

## Files

- **STATUS.md** - Current project status, what's done, what's next
- **SYMBOL_TABLE.md** - Symbol table & type system (COMPLETE)
- **QUICK_REF.md** - Common commands and lookup info

## Start Here

New to project? Read STATUS.md first (2 min) for quick overview.

Already familiar? Check QUICK_REF.md for commands.

Want implementation details? See SYMBOL_TABLE.md.

## Key Info

- **Current:** Symbol table & type system complete
- **Status:** 3,382 LOC, 125 tests passing, zero memory leaks
- **Quality:** Zero warnings, zero leaks, <600ms tests, ASAN/UBSAN enabled
- **Next:** C and WAT code generators

## Test Coverage

| Component | Tests | Status |
|-----------|-------|--------|
| Lexer | 106 | ✓ Complete |
| Parser | implicit | ✓ Complete |
| Semantics | 15 | ✓ Complete |
| Examples | 4 | ✓ All passing |
| **Total** | **125** | **✓ All passing** |

## Architecture

```
Source Code (.csm)
    ↓
Lexer (tokenization)
    ↓
Parser (syntax analysis)
    ↓
AST (abstract syntax tree)
    ↓
Semantic Analyzer (type checking)
    ↓
Type-Annotated AST
    ↓
Code Generators (C / WAT) [TODO]
    ↓
Output
```

## Quick Start

```bash
# Build
make build

# Run tests
make test

# Try an example
./bin/casm examples/simple_add.csm

# Check for issues
make clean && make test 2>&1 | grep -iE "(warning|leak|error)"
```
