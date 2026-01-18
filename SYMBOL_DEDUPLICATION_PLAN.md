# Symbol ID-Based Linking with Intelligent Name Deduplication

## Overview

Implement a system where each symbol has a unique ID, tracks original names, and uses intelligent name deduplication during code generation. The key innovation is a **two-phase approach**:

1. **Name Allocation Phase** - Traverse dependency graph starting from entry point, allocate names only to reachable symbols
2. **Code Generation Phase** - Generate code using pre-allocated names

This naturally implements dead code elimination (unreachable symbols are not generated).

---

## Phase 1: Extend Data Structures

### 1.1 AST Function Definition (src/ast.h)

Add to `ASTFunctionDef` structure:

```c
struct ASTFunctionDef {
    char* name;                    // Current name (pre-dedup)
    uint32_t symbol_id;            // NEW: Unique identifier (assigned during merge)
    char* original_name;           // NEW: Name before any deduplication
    char* module_path;             // NEW: Source file path (e.g., "module_a.csm")
    char* allocated_name;          // NEW: Final resolved name (NULL if not allocated)
    // ... existing fields ...
};
```

### 1.2 Function Symbol (src/types.h)

Add to `FunctionSymbol` structure:

```c
struct FunctionSymbol {
    char* name;
    uint32_t symbol_id;            // NEW: Unique identifier for this function
    char* original_name;           // NEW: Original function name
    char* module_path;             // NEW: Where it came from
    char* allocated_name;          // NEW: Final name in generated code
    // ... existing fields ...
};
```

### 1.3 Global State (new in semantics.c or codegen.c)

```c
// Name allocation tracking
static uint32_t g_next_symbol_id = 1000;

typedef struct {
    char* allocated_name;          // What we allocated
    uint32_t symbol_id;            // Which symbol it belongs to
} AllocatedName;

// Global tracking during allocation
static HashSet* g_used_names = NULL;  // Set of all allocated names
static AllocatedName* g_allocations = NULL;
static int g_allocation_count = 0;
static int g_allocation_capacity = 10;
```

---

## Phase 2: Assign Symbol IDs During Module Merge

**File:** `src/module_loader.c`

### 2.1 Initialize Global Counter

At start of `build_complete_ast()`:
```c
// Reset global symbol ID counter for each compilation
g_next_symbol_id = 1000;
```

### 2.2 Assign IDs During Merge

In the function merge loop (around line 351-386):

```c
// When copying a function from a module to the merged AST:
for (int i = 0; i < src_program->function_count; i++) {
    ASTFunctionDef* src_func = &src_program->functions[i];
    ASTFunctionDef* dst_func = &merged_program->functions[merged_program->function_count];
    
    // Copy basic function data
    *dst_func = *src_func;
    
    // NEW: Assign unique symbol ID
    dst_func->symbol_id = g_next_symbol_id++;
    
    // NEW: Store original name (before any collision renaming)
    dst_func->original_name = xstrdup(src_func->name);
    
    // NEW: Store module path
    dst_func->module_path = xstrdup(module_path);
    
    // NEW: No allocation yet
    dst_func->allocated_name = NULL;
    
    merged_program->function_count++;
}
```

### 2.3 Track Module Origins (Optional but Helpful)

Create a mapping structure:

```c
typedef struct {
    uint32_t symbol_id;
    char* original_name;
    char* module_path;
    ASTFunctionDef* function_ptr;
} SymbolOriginInfo;
```

This can be stored and returned from `build_complete_ast()` for debugging/error messages.

---

## Phase 3: Remove Collision Rejection in Semantics

**File:** `src/semantics.c`

### 3.1 Delete Hard Rejection Code

Remove the code at lines 459-465 that rejects functions with same name from different modules:

```c
// DELETE THIS SECTION:
if (strcmp(import1->imported_names[a], import2->imported_names[b]) == 0) {
    snprintf(msg, sizeof(msg), 
             "Function '%s' imported from both '%s' and '%s'",
             import1->imported_names[a], 
             import1->file_path, 
             import2->file_path);
    semantic_error_list_add(errors, msg, import2->location);
}
```

This hard rejection is no longer needed because deduplication happens during name allocation.

---

## Phase 4: Add Entry Point Analysis

**New function in src/codegen.c or src/semantics.c**

This analyzes which symbols are actually used, starting from the entry point.

### 4.1 Identify Entry Point

```c
// In codegen or semantics, find the main function
ASTFunctionDef* find_entry_point(ASTProgram* program) {
    for (int i = 0; i < program->function_count; i++) {
        if (strcmp(program->functions[i].name, "main") == 0) {
            return &program->functions[i];
        }
    }
    return NULL;
}
```

### 4.2 Collect Function Dependencies

Build a call graph by analyzing function bodies:

```c
// Pseudo-code structure
typedef struct {
    uint32_t* callee_ids;          // Symbol IDs of functions this calls
    int callee_count;
} FunctionCallGraph;

// Call graph[symbol_id] = functions called by symbol_id
FunctionCallGraph* g_call_graph = NULL;

void build_call_graph(ASTProgram* program) {
    g_call_graph = xmalloc(sizeof(FunctionCallGraph) * program->function_count);
    
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        // Analyze func->body to find all function calls
        // For each call expression, find the called function's symbol_id
        // Store in g_call_graph[i].callee_ids[]
        
        g_call_graph[i].callee_ids = xmalloc(sizeof(uint32_t) * 100);  // estimate
        g_call_graph[i].callee_count = 0;
        
        analyze_function_calls(&func->body, &g_call_graph[i]);
    }
}

// Helper to extract all function calls from an expression
void analyze_function_calls(ASTExpression* expr, FunctionCallGraph* graph) {
    if (!expr) return;
    
    if (expr->type == EXPR_CALL) {
        // Find the callee's symbol_id
        uint32_t callee_id = find_function_symbol_id(expr->as.call.callee_name);
        if (callee_id != 0) {
            graph->callee_ids[graph->callee_count++] = callee_id;
        }
    }
    
    // Recursively analyze sub-expressions
    // ... handle all expression types ...
}
```

---

## Phase 5: Name Allocation Phase (The Core Algorithm)

**New function: `allocate_symbol_names()`**

This is the critical new algorithm that implements intelligent deduplication.

### 5.1 Algorithm Overview

```
ALGORITHM: allocate_symbol_names(program, entry_point)

INPUT:
  - program: merged AST with symbol IDs assigned
  - entry_point: main function
  - call_graph: dependency information

OUTPUT:
  - Each reachable symbol has allocated_name set
  - Unreachable symbols have allocated_name = NULL

PROCESS:

1. Initialize:
   - used_names = empty set (tracks all allocated names)
   - to_allocate = empty queue
   - allocated = empty set (tracks allocated symbol_ids)
   - queue entry_point's symbol_id

2. While to_allocate is not empty:
   a. Dequeue symbol_id
   
   b. If already allocated, continue
   
   c. Allocate name for this symbol:
      - preferred = symbol.original_name
      - 
      - if preferred not in used_names:
          allocated_name = preferred
      - else:
          basename = filename without extension
          qualified = "{basename}_{preferred}"
          
          if qualified not in used_names:
              allocated_name = qualified
          else:
              n = 2
              while true:
                  numbered = "{basename}_{preferred}_{n}"
                  if numbered not in used_names:
                      allocated_name = numbered
                      break
                  n++
      
      - symbol.allocated_name = allocated_name
      - used_names.add(allocated_name)
      - allocated.add(symbol_id)
   
   d. For each function called by this symbol:
      - If not in allocated set:
          - Queue it for allocation
   
   e. Mark in symbol table:
      - Set symbol_entry.allocated_name = allocated_name

3. After traversal:
   - Symbols with allocated_name = NULL are dead code
   - Only generate code for symbols with allocated_name set
```

### 5.2 Pseudocode Implementation

```c
void allocate_symbol_names(ASTProgram* program, SymbolTable* table) {
    // Initialize
    HashSet* used_names = hashset_create();
    Queue* to_allocate = queue_create();
    HashSet* allocated_symbols = hashset_create();
    
    // Find entry point and start allocation
    ASTFunctionDef* main_func = find_entry_point(program);
    if (!main_func) {
        // Error: no main found
        return;
    }
    
    queue_enqueue(to_allocate, (void*)(uintptr_t)main_func->symbol_id);
    
    // Process queue
    while (!queue_empty(to_allocate)) {
        uint32_t symbol_id = (uint32_t)(uintptr_t)queue_dequeue(to_allocate);
        
        // Skip if already allocated
        if (hashset_contains(allocated_symbols, (void*)(uintptr_t)symbol_id)) {
            continue;
        }
        
        // Find the function with this symbol_id
        ASTFunctionDef* func = find_function_by_symbol_id(program, symbol_id);
        if (!func) continue;
        
        // Allocate name
        char* allocated_name = allocate_name_for_symbol(
            func->original_name,
            func->module_path,
            used_names
        );
        
        func->allocated_name = allocated_name;
        hashset_add(used_names, allocated_name);
        hashset_add(allocated_symbols, (void*)(uintptr_t)symbol_id);
        
        // Queue callees
        if (g_call_graph[symbol_id].callee_count > 0) {
            for (int i = 0; i < g_call_graph[symbol_id].callee_count; i++) {
                uint32_t callee_id = g_call_graph[symbol_id].callee_ids[i];
                if (!hashset_contains(allocated_symbols, (void*)(uintptr_t)callee_id)) {
                    queue_enqueue(to_allocate, (void*)(uintptr_t)callee_id);
                }
            }
        }
    }
    
    // Update symbol table
    for (int i = 0; i < table->function_count; i++) {
        FunctionSymbol* sym = &table->functions[i];
        ASTFunctionDef* func = find_function_by_symbol_id(program, sym->symbol_id);
        if (func) {
            sym->allocated_name = func->allocated_name;
        }
    }
}

// Helper: allocate a unique name following the priority rules
char* allocate_name_for_symbol(
    const char* original_name,
    const char* module_path,
    HashSet* used_names
) {
    char buffer[256];
    
    // Priority 1: Original name
    if (!hashset_contains(used_names, (void*)original_name)) {
        return xstrdup(original_name);
    }
    
    // Priority 2: {basename}_{original_name}
    char* basename = get_filename_without_extension(module_path);
    snprintf(buffer, sizeof(buffer), "%s_%s", basename, original_name);
    if (!hashset_contains(used_names, (void*)buffer)) {
        xfree(basename);
        return xstrdup(buffer);
    }
    
    // Priority 3: {basename}_{original_name}_{N} where N >= 2
    int n = 2;
    while (n < 10000) {  // safety limit
        snprintf(buffer, sizeof(buffer), "%s_%s_%d", basename, original_name, n);
        if (!hashset_contains(used_names, (void*)buffer)) {
            xfree(basename);
            return xstrdup(buffer);
        }
        n++;
    }
    
    xfree(basename);
    return NULL;  // Should never reach here
}

// Helper: extract filename without path or extension
char* get_filename_without_extension(const char* filepath) {
    // Example: "/path/to/module_a.csm" → "module_a"
    const char* filename_start = strrchr(filepath, '/');
    if (!filename_start) {
        filename_start = filepath;
    } else {
        filename_start++;  // Skip the '/'
    }
    
    // Find extension
    const char* ext_start = strrchr(filename_start, '.');
    if (!ext_start) {
        ext_start = filename_start + strlen(filename_start);
    }
    
    // Copy without extension
    int len = ext_start - filename_start;
    char* result = xmalloc(len + 1);
    strncpy(result, filename_start, len);
    result[len] = '\0';
    
    return result;
}

ASTFunctionDef* find_function_by_symbol_id(ASTProgram* program, uint32_t symbol_id) {
    for (int i = 0; i < program->function_count; i++) {
        if (program->functions[i].symbol_id == symbol_id) {
            return &program->functions[i];
        }
    }
    return NULL;
}
```

---

## Phase 6: Update Function Call Resolution

**File:** `src/semantics.c` or `src/types.c`

When resolving function calls, use the allocated name:

```c
// When looking up a function for a call
FunctionSymbol* lookup_function_for_call(SymbolTable* table, const char* name) {
    // Find function with matching original name
    for (int i = 0; i < table->function_count; i++) {
        if (strcmp(table->functions[i].name, name) == 0 ||
            strcmp(table->functions[i].original_name, name) == 0) {
            return &table->functions[i];
        }
    }
    return NULL;
}
```

---

## Phase 7: Update Code Generation

**Files:** `src/codegen.c` and `src/codegen_wat.c`

### 7.1 Function Declaration

Replace:
```c
fprintf(out, "%s %s(", casm_type_to_c_type(func->return_type.type), func->name);
```

With:
```c
// Use allocated name if available (should always be available for generated code)
const char* func_name = func->allocated_name ? func->allocated_name : func->name;
fprintf(out, "%s %s(", casm_type_to_c_type(func->return_type.type), func_name);
```

### 7.2 Function Calls

When emitting a call to a function, look up its symbol ID and use allocated name:

```c
// In emit_expression when handling EXPR_CALL
case EXPR_CALL: {
    ASTCallExpr* call = &expr->as.call;
    
    // Look up the function
    FunctionSymbol* callee = lookup_function_for_call(current_symbol_table, call->callee_name);
    if (!callee) {
        // Error
        return;
    }
    
    // Use allocated name
    const char* callee_name = callee->allocated_name ? callee->allocated_name : callee->name;
    fprintf(out, "%s(", callee_name);
    
    // ... emit arguments ...
    fprintf(out, ")");
    break;
}
```

### 7.3 Dead Code Elimination

Before code generation, filter functions:

```c
void codegen_program(ASTProgram* program, FILE* out) {
    // Generate only functions with allocated_name set
    // Skip functions with allocated_name == NULL (dead code)
    
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        if (func->allocated_name == NULL) {
            // Dead code - skip
            continue;
        }
        
        // Generate code for this function
        codegen_function(func, out);
    }
}
```

---

## Phase 8: Update WAT Code Generation

**File:** `src/codegen_wat.c`

Apply the same changes as Phase 7 (use allocated_name everywhere).

---

## Implementation Order

### Step 1: Data Structure Changes
- Modify `ASTFunctionDef` in ast.h
- Modify `FunctionSymbol` in types.h
- Add new global state variables

### Step 2: Symbol ID Assignment
- Add ID assignment in module_loader.c
- Test: IDs are unique, original_name is stored

### Step 3: Remove Collision Rejection
- Delete hard rejection code in semantics.c
- Test: Code compiles without premature rejection

### Step 4: Build Call Graph
- Implement call graph analysis
- Test: Call graph correctly identifies dependencies

### Step 5: Name Allocation
- Implement allocate_symbol_names()
- Implement helper functions
- Test: Names are allocated following priority rules

### Step 6: Function Resolution
- Update lookup functions to use allocated names
- Update symbol table operations

### Step 7: Code Generation
- Update codegen.c to use allocated_name
- Update codegen_wat.c to use allocated_name
- Add dead code elimination filter

### Step 8: Testing & Validation
- Test dbg_import_same_name_no_collision passes
- Verify all existing tests still pass
- Test dead code elimination
- Test transitive imports

---

## Expected Test Results

### Before Implementation
```
Testing dbg_import_same_name_no_collision... ✓ (expected compile error)
Error: Function 'helper' already defined
```

### After Implementation
```
Testing dbg_import_same_name_no_collision... ✓
Output:
  test.csm:10:4: result_a = 13
  test.csm:11:4: result_b = 15
```

---

## Key Benefits of This Approach

1. **Dead Code Elimination** - Unreachable functions are automatically excluded
2. **Intelligent Naming** - Prefers original names, only adds qualifiers when needed
3. **Minimal Generated Code** - Only includes what's actually used
4. **Clear Error Messages** - Still use original names in error reporting
5. **Future-Proof** - Symbol IDs enable future module system features
6. **Non-Breaking** - No changes to import syntax or function calls

---

## Potential Gotchas & Mitigations

### Issue: Call Graph May Miss Dynamic Calls
**Mitigation:** For now, assume all calls are static (this is true for CASM). Document this limitation.

### Issue: Function Pointers
**Mitigation:** CASM doesn't have function pointers yet. If added in future, need to conservatively include all functions that could be referenced.

### Issue: HashSet Implementation
**Mitigation:** May need to implement a simple string-based HashSet if one doesn't exist. Or use linear search for now (less efficient but works).

### Issue: Compiler Crashes on Circular Dependencies
**Mitigation:** The call graph builds after semantic analysis rejects actual circular imports. Static analysis only - no infinite loops possible.

---

## Success Criteria

- [ ] All 43 existing DBG tests pass
- [ ] dbg_import_same_name_no_collision transitions from error to passing test
- [ ] Generated C code has no symbol collisions
- [ ] Generated WAT code has no symbol collisions
- [ ] Dead code elimination works (unreachable functions excluded)
- [ ] Error messages still show original function names
- [ ] No performance regression in compilation
