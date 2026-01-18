# Documentation

Quick overview of the Casm compiler project.

## Start Here

- **New to the project?** Read [STATUS.md](STATUS.md) for a 3-minute overview (current state, completed features, next steps)
- **Need to build/test?** See [QUICK_REF.md](QUICK_REF.md) for commands and code organization
- **Need a reminder?** Check [QUICK_REF.md](QUICK_REF.md#features-status) for feature status table

## Current State

- **Status:** 128 tests passing (106 lexer + 15 semantics + 7 examples)
- **Code:** 3,489 lines of C
- **Quality:** Zero warnings, zero leaks, ASAN/UBSAN enabled
- **Next:** WAT code generator

## Key Facts

- Compiles `.csm` files (C-like language) to C and WAT
- 10 explicit types: i8-i64, u8-u64, bool, void
- Full type checking and error accumulation
- Complete symbol table with block scoping
- C code generator working, WAT pending
