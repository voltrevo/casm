# Test: Same Name, No Collision

## What This Tests

This test documents a **current limitation** of the import system that needs to be fixed.

### The Problem

When two modules define functions with the same name, and both modules are imported and used in the same program, the current compiler rejects it with:
```
test.csm:3:4: Function 'helper' already defined
```

### The Scenario

- **module_a.csm**: Defines `helper(x) = x + 10` and `process_a(n)` that calls it
- **module_b.csm**: Defines `helper(x) = x * 5` and `process_b(n)` that calls it  
- **test.csm**: Imports and uses both `process_a` and `process_b`

### Why Both Symbols Need to Exist

In the final compiled program (C or WAT), we need:
- `process_a()` function (imported from module_a) - **MUST BE COMPILED IN**
- `process_b()` function (imported from module_b) - **MUST BE COMPILED IN**
- Module A's `helper()` function - **MUST BE COMPILED IN** (called by process_a)
- Module B's `helper()` function - **MUST BE COMPILED IN** (called by process_b)

But they can't both be named `helper` in the final code - they would collide at link time.

### The Solution (When Implemented)

The compiler should **namespace/mangle** the internal functions based on their module:
- Module A's helper → `module_a_helper`
- Module B's helper → `module_b_helper`
- Update calls within each module to use the mangled names

### Expected Behavior (When Fixed)

```casm
process_a(3):
  calls module_a_helper(3)
  = 3 + 10 = 13

process_b(3):
  calls module_b_helper(3)
  = 3 * 5 = 15
```

### Current Status

**Status:** ❌ **FAILS** - Compiler correctly detects the conflict but rejects the compilation entirely instead of namespace-mangling the symbols.

### How to Fix

This requires changes to the code generator to:
1. Track which module each function comes from
2. Apply module-based name mangling to all functions (not just imported ones)
3. Update all internal function calls to use mangled names
4. Ensure WAT generation also applies the same mangling

### Related Tests

- `dbg_import_error_collision` - Tests explicit collision detection when same function imported from both modules
- `dbg_import_multiple` - Tests multiple imports from same module (no collision because same namespace)
