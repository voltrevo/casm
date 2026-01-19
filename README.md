# Casm

Casm is a C-like language compiler written in C. It compiles `.csm` sources to wasm (wat format), and C.

## Requirements

- **Build:** `gcc` with C99 support
- **Testing:** `wasmtime` (required for WAT code validation)
- **Coverage reports:** `lcov` (provides `lcov` + `genhtml`)

### Installing wasmtime

wasmtime is required to run the full test suite. Install it using:

```bash
curl https://wasmtime.dev/install.sh -sSf | bash
```

Then add it to your PATH (the installer typically does this automatically):

```bash
export PATH="$HOME/.wasmtime/bin:$PATH"
```

For more details, see: https://docs.wasmtime.dev/cli-installing.html

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
