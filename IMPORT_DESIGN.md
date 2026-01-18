# Casm Import System - Comprehensive Design Plan

## Overview
Implement `#import foo from "./other.csm"` syntax to allow modular code organization across multiple files.

## Syntax
```casm
#import math from "./math.csm"
#import utils from "./lib/utils.csm"
#import helpers from "../helpers.csm"

i32 main() {
    i32 result = math:add(10, 5);
    return 0;
}
```

Module names are referenced with `:` separator (e.g., `math:add`).

## Architecture Overview

### 1. Lexer Changes
- Add `TOK_HASH` token for `#`
- Add `TOK_IMPORT` keyword for `import`
- Add `TOK_FROM` keyword for `from`
- String literals already supported for file paths

### 2. Parser Changes
- Add `ASTImportStatement` structure:
  ```c
  typedef struct {
      char* module_name;        // "math"
      char* file_path;          // "./math.csm"
      SourceLocation location;
  } ASTImportStatement;
  ```
- Parse `#import foo from "./file.csm"` at program level (before functions)
- Import statements must come before all function/variable definitions
- Store imports in `ASTProgram.imports[]` array

### 3. File Loading System
- New module: `src/module_loader.h/c`
- Responsibilities:
  - Resolve file paths (relative to current file, not working directory)
  - Load and cache file contents
  - Handle file not found errors
  - Track loaded modules to prevent infinite loops

```c
typedef struct {
    char* absolute_path;     // Resolved absolute path
    char* source_code;       // File contents
    char* module_name;       // Import alias
} LoadedModule;

typedef struct {
    LoadedModule* modules;
    int count;
    int capacity;
    char** loaded_paths;     // For cycle detection
    int loaded_count;
} ModuleCache;
```

### 4. Multi-File AST Construction
- New function: `build_complete_ast(const char* main_file, ModuleCache* cache)`
- Recursively load and parse all imports
- Merge all ASTs into a single unified AST
- Return complete `ASTProgram`

Flow:
1. Parse main file → get imports
2. For each import:
   - Resolve file path
   - Check if already loaded (in cache)
   - Load file, parse it, track in cache
   - Recursively process ITS imports
3. Merge all ASTs

### 5. Namespace and Symbol Management

#### Symbol Naming Convention
All imported symbols prefixed with module name:
- `math:add` refers to function `add` from module `math`
- Stored internally with full name: `math_add`
- Display errors with qualified names: `Module math, function add`

#### Symbol Table Architecture
Extend `SymbolTable` to support scoped functions:
```c
typedef struct {
    char* full_name;         // "math_add"
    char* display_name;      // "add" (for user visibility)
    char* module_name;       // "math"
    // ... rest of function symbol data
} FunctionSymbol;
```

#### Import Scope Rules
- Imports create a new "module namespace"
- Functions from imported modules not directly accessible
- Must use qualified name: `math:add(x, y)`
- Local functions accessible without prefix

### 6. Collision Detection

#### Scenarios to Handle
1. **Function name collision in same file** (existing check)
   ```casm
   i32 add(i32 a, i32 b) { return a + b; }
   i32 add(i32 a, i32 b, i32 c) { return a + b + c; }  // ERROR: duplicate
   ```

2. **Same-named function in different imports** (ALLOWED)
   ```casm
   #import math from "./math.csm"      // has add()
   #import mylib from "./mylib.csm"    // has add()
   math:add(5, 3)   // OK - calls math version
   mylib:add(5, 3)  // OK - calls mylib version
   ```

3. **Module name collision** (ERROR)
   ```casm
   #import utils from "./utils1.csm"
   #import utils from "./utils2.csm"   // ERROR: duplicate module name
   ```

4. **File being imported from itself** (ERROR)
   ```casm
   // file1.csm
   #import a from "./file1.csm"         // ERROR: file importing itself
   ```

5. **Direct circular import** (ERROR)
   ```casm
   // a.csm
   #import b from "./b.csm"
   
   // b.csm
   #import a from "./a.csm"             // ERROR: circular
   ```

6. **Indirect circular import** (ERROR)
   ```casm
   // a.csm
   #import b from "./b.csm"
   
   // b.csm
   #import c from "./c.csm"
   
   // c.csm
   #import a from "./a.csm"             // ERROR: circular
   ```

#### Collision Detection Implementation
- Track import chain during recursive loading:
  ```c
  char** import_chain;      // Stack of file paths
  int chain_depth;
  
  // Before loading a file, check if it's already in the chain
  if (is_in_import_chain(file_path, import_chain, chain_depth)) {
      error("Circular import detected: a.csm -> b.csm -> a.csm");
  }
  ```

### 7. Semantic Analysis for Imports

#### Validation Steps
1. All imports exist and are valid Casm files
2. No module name collisions
3. No circular imports
4. All qualified function calls reference existing functions
5. Qualified names follow `module:name` pattern
6. Unqualified calls only match local functions

#### Error Messages
```
error: module "math" not found (searched: ./math.csm, ../math.csm)
error: duplicate module name "utils" (previously imported at line 5)
error: circular import detected: a.csm -> b.csm -> a.csm
error: undefined module "math" in expression "math:add"
error: module "math" has no function "square"
```

### 8. Code Generation

#### Strategy: Flatten to Single C File
- Merge all imported modules into one C output
- Prepend all imported code before main file
- Use mangled names internally: `math_add`, `utils_sort`, etc.
- Preserve original names in comments for readability

Example output:
```c
// Generated from: math.csm
int32_t math_add(int32_t a, int32_t b) {
    return (a + b);
}

// Generated from: utils.csm
int32_t utils_max(int32_t a, int32_t b) {
    if ((a > b)) { return a; } else { return b; }
}

// Generated from: main.csm
int32_t main(void) {
    int32_t x = math_add(10, 5);
    return 0;
}
```

#### Function Lookup During Code Generation
- When encountering `math:add(5, 3)`:
  - Look up `math_add` in symbol table
  - Emit call to mangled name
  - Codegen produces: `math_add(5, 3)`

#### Handling Duplicate Function Names Across Files
- Mangling automatically handles this
- `math.csm` with `add()` → `math_add()`
- `lib.csm` with `add()` → `lib_add()`
- No collision even if multiple files have `add()`

### 9. Import Path Resolution

#### Rules
- Paths are relative to the importing file's directory
- Support `../` for parent directory traversal
- Support `./` for explicit current directory
- Resolve to absolute paths internally for caching

Example:
```
Project structure:
  src/
    main.csm           (imports: "./math/add.csm")
    math/
      add.csm          (imports: "../utils.csm")
    utils.csm

Resolutions:
  main.csm + "./math/add.csm" → {project}/src/math/add.csm
  add.csm + "../utils.csm" → {project}/src/utils.csm
```

### 10. Testing Strategy

#### Test Files Structure
```
tests/imports/
  test_simple_import/
    main.csm              (imports utils)
    utils.csm
    expected_output.txt
    expected_out.c
  test_collision_detect/
    main.csm              (has duplicate module name import)
    utils1.csm
    utils2.csm
    expected_error.txt
  test_circular_import/
    a.csm                 (imports b)
    b.csm                 (imports a)
    expected_error.txt
  test_qualified_names/
    main.csm
    math.csm              (has add, multiply)
    utils.csm             (has add)
    expected_output.txt
  ...more test cases
```

#### Test Cases to Implement
1. ✅ Simple single import
2. ✅ Multiple imports
3. ✅ Nested imports (a imports b, b imports c)
4. ✅ Same function names in different modules
5. ✅ Path traversal (../files)
6. ✅ Calling imported functions
7. ✅ Import not found error
8. ✅ Module name collision error
9. ✅ Circular import detection
10. ✅ Undefined module in function call
11. ✅ Undefined function in qualified call
12. ✅ C code generation with mangled names
13. ✅ WAT code generation with namespacing

### 11. Implementation Order

**Phase 1: Foundation**
1. Add lexer tokens for import statement
2. Create ASTImportStatement structure
3. Parse import statements
4. Basic file loading system

**Phase 2: Multi-File Support**
1. Implement recursive AST building
2. Add symbol table support for modules
3. Semantic analysis for imports
4. Error reporting

**Phase 3: Safety & Correctness**
1. Circular import detection
2. Name collision detection
3. Path resolution and caching
4. Module name uniqueness

**Phase 4: Code Generation**
1. Name mangling
2. C code generation for imports
3. WAT code generation for imports
4. Merging of multiple ASTs

**Phase 5: Testing & Polish**
1. Comprehensive test suite
2. Error message refinement
3. Documentation
4. Edge case handling

### 12. Potential Issues & Solutions

#### Issue: Diamond Import Problem
```
     a.csm
    /      \
   b.csm    c.csm
    \      /
     d.csm
```
**Solution**: Cache modules by absolute path; load each file only once globally

#### Issue: Variable Scope Across Imports
- Only functions are imported in this design
- Variables can't be imported (keep them local to each file)
- Prevents state sharing issues

#### Issue: Different Import Styles
```casm
#import "./math.csm" as math          // Alternative syntax
#import math                           // Auto-discover math.csm
import math from "./math.csm"         // JavaScript-style
```
**Decision**: Use explicit `#import foo from "./file.csm"` format for clarity

#### Issue: Module Initialization
- No module initialization code currently
- Keep it simple: just function definitions per file
- Functions execute when called, not on import

#### Issue: Export Declarations
- Don't use explicit exports
- All functions in a file are automatically "exported"
- Allows simpler implementation
- Users import whole modules, not individual functions

## Summary

The import system will:
- ✅ Support `#import module from "./path.csm"` syntax
- ✅ Recursively load and parse imported files
- ✅ Detect circular imports and report errors
- ✅ Prevent module name collisions
- ✅ Handle same function names in different modules via qualified names
- ✅ Generate flat C output with name mangling
- ✅ Maintain full type safety across file boundaries
- ✅ Provide clear error messages for all failure modes

## Files to Create/Modify

**New Files:**
- `src/module_loader.h` - File loading interface
- `src/module_loader.c` - File loading implementation
- `tests/imports/` - Test directory with test cases

**Modified Files:**
- `src/lexer.h/c` - Add TOK_IMPORT, TOK_FROM, TOK_HASH
- `src/ast.h/c` - Add ASTImportStatement
- `src/parser.h/c` - Parse import statements
- `src/semantics.c` - Validate imports, detect collisions
- `src/codegen.c/codegen_wat.c` - Generate for imports
- `src/main.c` - Use new multi-file AST building
