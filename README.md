# Casm

Casm is a C-like language compiler written in C. It compiles `.csm` sources to wasm (wat format), and C.

## Quick Start

```bash
make build
make test
```

## Repository Pointers

- Current project status and next steps: `notes/STATUS.md`
- Build/test commands and workflows: `notes/QUICK_REF.md`
- Notes index: `notes/README.md`

## Capabilities (Current)

- Explicit integer types (`i8`–`i64`, `u8`–`u64`), `bool`, and `void`
- Functions, variables, and block scoping
- Control flow: `if`/`else`, `while`, `for`
- Full type checking with error accumulation
- C code generation (WAT pending)
