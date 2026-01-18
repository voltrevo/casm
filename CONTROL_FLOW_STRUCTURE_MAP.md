# CASM Control Flow - Detailed Structure Mapping

## Current Compiler Pipeline

```
Source Code (.csm)
    ↓
Lexer (src/lexer.c)
    ├─ Tokenizes keywords, operators, literals
    ├─ Tracks source location (line, column)
    └─ Produces: Token stream
         ✅ TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR exist
    ↓
Parser (src/parser.c)
    ├─ Recursive descent parser
    ├─ Builds AST
    └─ Produces: ASTProgram
         🔴 Lines 508-535: REJECTS control flow
    ↓
Semantic Analysis (src/semantics.c)
    ├─ 2-pass analysis
    ├─ Pass 1: Collect function signatures
    ├─ Pass 2: Validate bodies and types
    └─ Produces: Symbol table, error list
         🟡 No handlers for STMT_IF/WHILE/FOR
    ↓
Code Generation (src/codegen.c)
    ├─ AST → C code
    └─ Produces: C source file
         🟡 emit_statement() has no control flow cases
    ↓
C Code (.c)
```

## Summary of Changes by File

| File | Changes | Lines Added | Complexity |
|------|---------|-------------|-----------|
| ast.h | Add 3 enums, 3 structs, 3 union members | ~40 | EASY |
| ast.c | Add cleanup for 3 statement types | ~45 | EASY |
| parser.c | Add 4 functions, replace 1 block | ~230 | MEDIUM |
| semantics.c | Add 3 cases in switch statement | ~50 | EASY |
| codegen.c | Add 3 cases in switch statement | ~110 | EASY |
| **TOTAL** | | **~475 lines** | **MEDIUM** |

This document contains the complete, detailed code mappings and implementation patterns needed for adding control flow support. See CONTROL_FLOW_ANALYSIS.md for the comprehensive analysis.

For specific implementation code, see the sections below:

### Implementation Sections in This Document:
1. **src/ast.h** - New structures and types needed
2. **src/ast.c** - Memory management updates
3. **src/parser.c** - Parsing functions (4 new functions)
4. **src/semantics.c** - Semantic analysis handlers
5. **src/codegen.c** - Code generation handlers

### Testing Checklist

- [ ] Parse if_statement.csm without errors
- [ ] Parse while_loop.csm without errors
- [ ] Parse for_loop.csm without errors
- [ ] Generate valid C code for if_statement.csm
- [ ] Generate valid C code for while_loop.csm
- [ ] Generate valid C code for for_loop.csm
- [ ] Semantic error for non-bool condition
- [ ] Nested if/while/for statements parse correctly
- [ ] Scoping works correctly (variables accessible in nested blocks)
- [ ] No memory leaks (valgrind clean)
