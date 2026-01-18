# Code Review Issues

## Identified Issues

### Critical
- **#3 - Infinite Loop (Bare Blocks)**: Parser hangs on `{ }` in function bodies. In `parse_block()`, when `parse_statement()` returns NULL, token position isn't advanced, causing infinite loop. Fix: Add `STMT_BLOCK` type to AST enum, implement bare block parsing.

### High Severity
- **#4 - Silent Integer Overflow**: `sscanf(token.lexeme, "%ld", &int_value)` silently clamps to LLONG_MAX. Fix: Use `strtoll()` with overflow detection.
- **#5 - Uninitialized Variables**: Variables usable before initialization. Fix: Track initialization state in semantic analyzer.
- **#6 - Type Compatibility Too Permissive**: All signed ints compatible with each other (i8↔i64). Allows silent overflow on assignments. Fix: Stricter type rules or explicit casts.

### Medium
- **#7 - Inconsistent free()**: ✅ FIXED - Changed `free(stmt)` to `xfree(stmt)` in parser.c:854
- **#8 - Missing Bare Block Type**: Related to #3, no `STMT_BLOCK` enum value

### Low
- **#1-2 - NULL in Expression Parsing**: Not real bugs, error recovery prevents crashes
- **#9 - Dead Code**: ✅ FIXED - Removed `lexer_peek_token()` from lexer.h/c

## Test Programs

Example programs triggering each issue in `examples/issue_*.csm`:
- #3: `issue_3_infinite_loop_bare_blocks.csm` - timeout confirms hang
- #4: `issue_4_integer_overflow.csm` - silent overflow to LLONG_MAX
- #5: `issue_5_uninitialized_var.csm` - garbage memory values allowed
- #6: `issue_6_type_compatibility.csm` - i64→i8 overflow allowed

## Priority Fixes

1. Issue #3 - Infinite loop (blocks valid/invalid programs)
2. Issue #4 - Integer overflow (data corruption)
3. Issue #5 - Uninitialized use (undefined behavior)
4. Issue #6 - Type safety (silent overflows)
