# Casm Compiler Documentation Index

Welcome! This directory contains complete documentation for the Casm compiler project. Whether you're a new contributor, continuing development, or reviewing the codebase, start here.

## Quick Start (5 minutes)

1. **New to the project?** Start with [PROJECT_STATUS.md](PROJECT_STATUS.md)
   - Understand what's been built and what's remaining
   - See the architecture diagram
   - Learn about design decisions

2. **Ready to implement something?** Read [NEXT_AGENT_TASKS.md](NEXT_AGENT_TASKS.md)
   - See exactly what needs to be done
   - Get a 5-step implementation guide
   - Learn success criteria

3. **Need to look something up?** Use [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
   - Common commands and tasks
   - Code organization
   - Debugging tips
   - Type system reference

## Documentation Files

### [SYMBOL_TABLE_IMPLEMENTATION.md](SYMBOL_TABLE_IMPLEMENTATION.md)
**Complete technical specification for the symbol table and type system**

Use this when:
- You're implementing symbol table (types.c/h)
- You're implementing semantic analyzer (semantics.c/h)
- You need to understand architecture decisions
- You want type compatibility rules

Contains:
- Architecture decisions (scope handling, error recovery, AST annotation strategy)
- Data structure definitions with exact field names
- Public API specification
- Implementation algorithm (2-pass analysis)
- Type compatibility rules
- Binary/unary operation type rules
- Scope management details
- Error message format

**~350 lines of detailed technical specification**

### [PROJECT_STATUS.md](PROJECT_STATUS.md)
**Comprehensive project overview and roadmap**

Use this when:
- You're new to the project
- You want to understand what's been completed
- You need the big picture architecture
- You want to see remaining work
- You need to understand design decisions
- You want file organization and line counts

Contains:
- Project overview and architecture diagram
- Current compiler features (✅ implemented, ❌ not yet)
- File organization with line counts
- Completed tasks and commits
- Remaining high-priority tasks
- Known limitations and TODOs
- Test coverage overview
- Build system details
- Design decisions explained
- Performance baselines
- Future enhancements (out of scope)

**~290 lines covering full project state**

### [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
**Quick lookup reference for common tasks**

Use this when:
- You need to build/test the compiler
- You're adding a new feature
- You need debugging help
- You want code organization overview
- You need type system compatibility matrix
- You're checking for common mistakes

Contains:
- Build/test commands
- Running examples
- Adding tests
- Memory debugging
- Code organization (src/, tests/, examples/)
- Key data structures
- Error message examples
- Type compatibility matrix
- Adding new tokens/AST nodes/operators
- Testing checklist
- Performance profiling
- Common mistakes

**~245 lines of practical reference**

### [NEXT_AGENT_TASKS.md](NEXT_AGENT_TASKS.md)
**Detailed guide for the next implementer**

Use this if you're implementing the symbol table. Contains:
- What you need to accomplish (overview)
- 5-step implementation plan
- Exact functions to implement
- Testing strategy with success criteria
- Key implementation details with code examples
- Two-pass analysis algorithm walkthrough
- Scope management pattern
- Type checking examples
- Common pitfalls to avoid
- Files to read first
- Getting help resources

**~300 lines of hands-on guidance**

## How to Use These Docs

### If you're joining the project:
1. Read PROJECT_STATUS.md (10 min) - understand what exists
2. Read QUICK_REFERENCE.md (5 min) - learn build commands
3. Skim SYMBOL_TABLE_IMPLEMENTATION.md (10 min) - see what's next
4. Run `make test` - verify everything works
5. Read existing code (src/lexer.c, src/parser.c) to understand patterns

### If you're implementing symbol table:
1. Read NEXT_AGENT_TASKS.md completely (15 min)
2. Reference SYMBOL_TABLE_IMPLEMENTATION.md as you code
3. Use QUICK_REFERENCE.md for common tasks
4. Follow the 5-step plan exactly
5. Verify success criteria are met

### If you're implementing code generation:
1. Read PROJECT_STATUS.md (understand what semantic analysis does)
2. Read QUICK_REFERENCE.md (understand code organization)
3. Study type-annotated AST from semantic analysis
4. Write C/WAT codegen following established patterns
5. Use run_tests.sh for testing

### If you're debugging something:
1. Check QUICK_REFERENCE.md section "Debugging Tips"
2. Run compiler under AddressSanitizer (automatic with `make build`)
3. Look at existing code in src/ for patterns
4. Check test files (tests/) for examples

## Key Information at a Glance

### Current State
- **Completed:** Lexer (106 tests), Parser (AST generation), Error handling, Memory management
- **In Progress:** Symbol table & type system (design complete, ready to implement)
- **Not Started:** C code generation, WAT code generation, Control flow statements

### Next Immediate Task
Implement symbol table and semantic analysis (types.c/h, semantics.c/h) following NEXT_AGENT_TASKS.md

### Build Commands
```bash
make build              # Debug build with sanitizers (default)
make test               # Full test suite (<600ms)
make build-release      # Optimized release build
make clean              # Remove binaries
```

### Testing
```bash
make test               # Builds and runs all tests
./casm examples/simple_add.csm  # Parse an example
timeout 2 ./casm examples/foo.csm  # With timeout
```

### Code Quality
- All code compiles with zero warnings
- All tests pass (106 unit tests + 4 integration tests)
- All memory safe (AddressSanitizer clean)
- Full source location tracking (line:column:offset)

## Design Philosophy

The Casm compiler follows these principles:

1. **Explicit is better than implicit**
   - No type inference
   - No implicit conversions
   - Explicit error messages

2. **Error recovery**
   - Accumulate all errors, not just first
   - Report with precise source locations
   - Enable user to fix multiple issues at once

3. **Memory safety**
   - Sanitizers enabled by default
   - Careful lifetime management
   - All allocations tracked

4. **Simple architecture**
   - Traditional pipeline: Lex → Parse → Analyze → Generate
   - Recursive descent parser
   - No complex optimizations (yet)

5. **Testability**
   - Fast execution (<600ms for full suite)
   - Aggressive timeouts to catch hangs
   - Unit tests for low-level components
   - Integration tests for features

## File Structure

```
/home/andrew/casm/
├── src/                    # Source code
│   ├── lexer.c/h          # Tokenization (350 lines)
│   ├── parser.c/h         # Syntax analysis (740 lines)
│   ├── ast.c/h            # AST management (180 lines)
│   ├── types.c/h          # Symbol table (TO BE IMPLEMENTED)
│   ├── semantics.c/h      # Semantic analysis (TO BE IMPLEMENTED)
│   ├── codegen_c.c/h      # C generation (TODO)
│   ├── codegen_wat.c/h    # WAT generation (TODO)
│   ├── utils.c/h          # Helpers (50 lines)
│   └── main.c             # CLI & pipeline (140 lines)
├── tests/
│   ├── test_lexer.c       # Lexer tests (450 lines, 106 tests)
│   ├── test_semantics.c   # Semantic tests (TODO)
│   └── test_codegen.c     # Codegen tests (TODO)
├── examples/              # Test programs (9 .csm files)
├── notes/                 # Documentation (this directory)
├── Makefile               # Build system
├── run_tests.sh           # Test runner
├── .gitignore             # Git configuration
├── AGENTS.md              # Agent instructions
├── PLAN.md                # Project plan
└── README.md              # (if you create one)
```

## Getting Help

### Understanding the codebase:
- Read notes/PROJECT_STATUS.md for overview
- Read src/lexer.c for simple tokenization example
- Read src/parser.c for recursive descent parser pattern
- Read src/ast.c for memory management pattern

### Implementing a feature:
- Read NEXT_AGENT_TASKS.md for detailed steps
- Use SYMBOL_TABLE_IMPLEMENTATION.md as reference
- Copy patterns from existing code (src/)
- Check tests/ for examples

### Debugging:
- Use AddressSanitizer (automatic with debug build)
- Add debug fprintf statements
- Check QUICK_REFERENCE.md debugging section
- Run with timeout to catch infinite loops

### Code standards:
- C99 standard (-std=c99)
- Follow existing code style in src/
- No warnings (-Wall -Wextra -pedantic)
- Test your code (add tests to tests/)
- Memory safe (no leaks, no use-after-free)

## Contributing Guidelines

When implementing a new feature:

1. **Design:** Read relevant documentation first
2. **Implement:** Follow patterns from existing code
3. **Test:** Add unit tests and integration tests
4. **Verify:** `make clean && make test` passes
5. **Quality:** No warnings, no memory leaks, <600ms tests
6. **Document:** Update these notes if needed
7. **Commit:** Write clear commit message explaining why

## Version Information

- **Language:** C99
- **Compiler:** gcc/clang with -Wall -Wextra -pedantic
- **Build System:** Make
- **Testing:** Custom test harness with aggressive timeouts
- **Memory Checking:** AddressSanitizer + UBSanitizer
- **Git:** Standard workflow with meaningful commits

## What's Next?

The immediate next task is implementing the symbol table and type system. See [NEXT_AGENT_TASKS.md](NEXT_AGENT_TASKS.md) for the detailed 5-step plan.

After that:
1. C code generator
2. WAT code generator
3. Control flow statements (if/while/for)
4. Module system

---

**Last Updated:** January 18, 2026  
**Project Status:** Functional compiler frontend, semantic analysis ready for implementation  
**Next Milestone:** Symbol table and type system working with all tests passing
