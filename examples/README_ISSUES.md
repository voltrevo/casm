# Code Review Issue Test Programs

This directory contains test programs designed to trigger the substantial issues found in the CASM compiler code review.

## Issue Summary

### Critical Issues

1. **issue_1_unary_null_deref.csm** - NULL pointer in unary parsing
   - Status: Hard to trigger - error recovery prevents crash
   - The parser catches these cases and reports "Expected expression"

2. **issue_2_binary_null_deref.csm** - NULL pointer in binary parsing  
   - Status: Hard to trigger - error recovery prevents crash
   - Similar to issue #1 - error messages catch the problem

3. **issue_3_infinite_loop_bare_blocks.csm** - CONFIRMED INFINITE LOOP ✓
   - Status: **CONFIRMED** - The compiler hangs indefinitely
   - Bare blocks `{ }` nested in functions cause infinite loop
   - **Command to test:** `timeout 2 ./bin/casm --target=c examples/issue_3_infinite_loop_bare_blocks.csm`
   - **Expected behavior:** Times out after 2 seconds (infinite loop)

### High Severity Issues

4. **issue_4_integer_overflow.csm** - Silent integer overflow
   - Status: **CONFIRMED** - No error reported
   - The number 9999999999999999999 exceeds LLONG_MAX and is silently truncated
   - No warning issued to the user
   - Compiles to C code successfully with corrupt value

5. **issue_5_uninitialized_var.csm** - Uninitialized variable use
   - Status: **CONFIRMED** - Compiles without error
   - Variable declared without initializer can be used
   - Allows undefined behavior (reading garbage memory)
   - The semantic analyzer doesn't track initialization state

6. **issue_6_type_compatibility.csm** - Type system too permissive
   - Status: **CONFIRMED** - Compiles without error
   - i64 value assigned to i8 silently overflows
   - All signed ints compatible with all other signed ints
   - Weakens type safety - should error or at least warn

### Medium Severity Issues

7. **issue_7_inconsistent_free.csm** - Inconsistent free() usage
   - Status: Code quality issue (not runtime)
   - One place uses bare `free(stmt)` instead of `xfree(stmt)` (parser.c:854)
   - Removed from lexer.h/c during cleanup

8. **issue_8_missing_bare_blocks.csm** - No bare block statement type
   - Status: Same as issue #3 (infinite loop)
   - Architectural issue - STMT_BLOCK never defined
   - Related to bare blocks causing infinite loop

### Low Severity Issues

9. **issue_9_removed_dead_code.csm** - Broken lexer_peek_token()
   - Status: **REMOVED** as dead code
   - Function was broken and never called
   - Has been deleted from lexer.h and lexer.c

## Testing

Run with C target to see errors (WAT target not yet implemented):

```bash
# Test issue #3 (will timeout - shows infinite loop)
timeout 2 ./bin/casm --target=c examples/issue_3_infinite_loop_bare_blocks.csm

# Test issue #4 (shows silent overflow - no error)
./bin/casm --target=c examples/issue_4_integer_overflow.csm
cat out.c  # Shows the corrupted value

# Test issue #5 (shows uninitialized var use - no error)
./bin/casm --target=c examples/issue_5_uninitialized_var.csm

# Test issue #6 (shows unsafe type conversion - no error)
./bin/casm --target=c examples/issue_6_type_compatibility.csm
```

## Confirmed Issues

- ✓ Issue #3: Infinite loop with bare blocks
- ✓ Issue #4: Silent integer overflow
- ✓ Issue #5: Uninitialized variable use
- ✓ Issue #6: Overly permissive type compatibility
- ✓ Issue #7: Inconsistent memory deallocation style
- ✓ Issue #9: Dead code removed
