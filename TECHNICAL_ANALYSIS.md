# CASM Codebase Technical Analysis: Symbol ID System with Name Deduplication

## Executive Summary

This document provides a detailed technical analysis of the CASM compiler codebase across four key areas: AST structures, semantic analysis, code generation, and module loading. The analysis identifies current design patterns and suggests where changes are needed to support symbol IDs with intelligent name deduplication for multi-module compilation.

---

## 1. AST Structures for Functions and Symbols (`src/ast.h` and `src/ast.c`)

### Current Function Definition Structure

**File**: `src/ast.h` lines 246-254

```c
struct ASTFunctionDef {
    char* name;                      /* Simple string name only */
    TypeNode return_type;
    ASTParameter* parameters;
    int parameter_count;
    ASTBlock body;
    SourceLocation location;
};
```

**Key Observations**:

1. **No Unique Identifier**: Functions are identified by name alone. There's no ID field or way to distinguish between functions with the same name from different modules.

2. **Simple String Name**: The `name` field is a single `char*` with no qualified naming convention (e.g., no "module:name" format).

3. **No Module Origin Tracking**: The function definition carries no information about which module it originated from.

4. **Parameter Structure** (lines 55-59):
   ```c
   struct ASTParameter {
       char* name;
       TypeNode type;
       SourceLocation location;
   };
   ```
   Parameters only track name and type, no ID assignment.

### Function Call Structure

**File**: `src/ast.h` lines 212-217

```c
struct ASTFunctionCall {
    char* function_name;             /* Unqualified name only */
    ASTExpression* arguments;
    int argument_count;
    SourceLocation location;
};
```

**Issue**: Function calls reference functions by unqualified name only. When multiple modules define functions with the same name, there's no way to disambiguate which one should be called.

### Changes Needed

To support symbol IDs and deduplication:

1. **Add Symbol ID field** to `ASTFunctionDef`:
   ```c
   uint32_t symbol_id;              /* Unique identifier assigned during semantic analysis */
   char* original_name;             /* Name from source before deduplication */
   char* module_path;               /* Absolute path of the module this function comes from */
   ```

2. **Track in function calls**:
   ```c
   struct ASTFunctionCall {
       char* function_name;         /* Still used for initial lookup */
       uint32_t resolved_symbol_id; /* Set by semantic analyzer */
       /* ... rest of fields ... */
   };
   ```

3. **Add symbol metadata struct** to track deduplication decisions:
   ```c
   typedef struct {
       uint32_t symbol_id;
       char* original_name;         /* Name before mangling */
       char* mangled_name;          /* Name after deduplication */
       char* module_path;
       int is_deduplicated;         /* True if merged with another function */
   } SymbolInfo;
   ```

---

## 2. Semantic Analysis (`src/semantics.c` and `src/types.h`)

### Current Symbol Table Structure

**File**: `src/types.h` lines 13-44

```c
struct FunctionSymbol {
    char* name;                      /* Only field for identification */
    char* module_name;               /* Exists but not used effectively */
    CasmType return_type;
    CasmType* param_types;
    int param_count;
    SourceLocation location;
};

struct SymbolTable {
    FunctionSymbol* functions;
    int function_count;
    int function_capacity;
    Scope* current_scope;
};
```

### Symbol Table Operations

**File**: `src/types.c` lines 55-87

```c
int symbol_table_add_function(SymbolTable* table, const char* name, CasmType return_type,
                               CasmType* param_types, int param_count, SourceLocation location)
{
    /* Check for duplicates by name only */
    for (int i = 0; i < table->function_count; i++) {
        if (strcmp(table->functions[i].name, name) == 0) {
            return 0;  /* Duplicate function - ERROR */
        }
    }
    /* ... add function with name-based identification ... */
}
```

**Critical Issue**: Line 59 shows that duplicate detection is **name-based only**. If two modules define functions with the same name, the second one is rejected outright.

### Function Lookup

**File**: `src/types.c` lines 90-97

```c
FunctionSymbol* symbol_table_lookup_function(SymbolTable* table, const char* name)
{
    for (int i = 0; i < table->function_count; i++) {
        if (strcmp(table->functions[i].name, name) == 0) {
            return &table->functions[i];  /* Returns first match only */
        }
    }
    return NULL;
}
```

**Limitation**: Linear search by name, returns first match. No support for qualified names or symbol IDs.

### Semantic Analysis Flow

**File**: `src/semantics.c` lines 502-529

The 4-pass analysis currently:

1. **Pass 1**: `validate_import_collisions()` (lines 445-470) - Detects if the same function name is imported from multiple files
2. **Pass 2**: `validate_imports()` (lines 473-500) - Checks if imported names exist
3. **Pass 3**: `collect_functions()` (lines 392-417) - Adds all functions to symbol table by name
4. **Pass 4**: `validate_functions()` (lines 420-442) - Validates function bodies

**Key Insight**: Pass 1 explicitly prevents importing the same function name from multiple sources (line 459-465). This is the current deduplication blocking point.

### Changes Needed

1. **Extend FunctionSymbol** to include:
   ```c
   struct FunctionSymbol {
       char* name;
       char* module_name;
       uint32_t symbol_id;           /* NEW: Unique ID assigned during semantic analysis */
       uint32_t dedup_group_id;      /* NEW: Groups identical functions for deduplication */
       int is_canonical;             /* NEW: True if this is the selected implementation */
       CasmType return_type;
       CasmType* param_types;
       int param_count;
       SourceLocation location;
   };
   ```

2. **Add deduplication pass** between passes 2 and 3:
   - After validating imports exist, identify functions with identical signatures
   - Group them into deduplication candidates
   - Select canonical implementations (or require explicit disambiguation)
   - Assign symbol IDs based on deduplication decisions

3. **Modify lookup functions**:
   ```c
   FunctionSymbol* symbol_table_lookup_function_by_id(SymbolTable* table, uint32_t symbol_id);
   FunctionSymbol* symbol_table_lookup_function_qualified(SymbolTable* table, const char* module, const char* name);
   ```

4. **Track original vs. deduplicated names**:
   - Store original unqualified name in separate field
   - Store post-deduplication mangled name in `name` field for codegen

---

## 3. Code Generation (`src/codegen.c` and `src/codegen_wat.c`)

### Current Function Name Mangling

**File**: `src/codegen.c` lines 30-39

```c
static char* mangle_function_name(const char* qualified_name)
{
    char* mangled = xstrdup(qualified_name);
    for (int i = 0; mangled[i]; i++) {
        if (mangled[i] == ':') {
            mangled[i] = '_';  /* Replace ':' with '_' */
        }
    }
    return mangled;
}
```

**Current Behavior**: 
- Simple character replacement (`:` → `_`)
- Designed for "module:name" format but module information is never propagated
- In practice, this function receives unqualified names and does nothing

### Function Declaration Emission

**File**: `src/codegen.c` lines 408-433

```c
static void emit_function_declarations(FILE* out, ASTProgram* program)
{
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        char* mangled_name = mangle_function_name(func->name);  /* Uses func->name directly */
        
        fprintf(out, "%s %s(",
                casm_type_to_c_type(func->return_type.type),
                mangled_name);
        /* ... emit parameters and close declaration ... */
    }
}
```

**Issue**: Mangling is attempted on `func->name`, but since `func->name` is never set to a qualified name format, mangling has no effect.

### Function Call Emission

**File**: `src/codegen.c` lines 142-153

```c
case EXPR_FUNCTION_CALL: {
    char* mangled_name = mangle_function_name(expr->as.function_call.function_name);
    fprintf(out, "%s(", mangled_name);
    /* ... emit arguments ... */
    fprintf(out, ")");
    break;
}
```

**Problem**: References `function_name` from AST, which is unqualified. Mangling doesn't help without module information.

### WAT Code Generation

**File**: `src/codegen_wat.c` lines 24-33 and 162-177

Similar issues:
- `mangle_function_name()` replaces `:` with `_`
- Function calls use the mangled name
- No qualified names are ever created in the AST

### Changes Needed

1. **Store mangled names in AST** during semantic analysis:
   - Semantic analyzer determines final function names after deduplication
   - Writes mangled names back to AST function definitions and call sites
   - Codegen then uses these pre-computed names

2. **Enhanced mangling strategy**:
   ```c
   static char* mangle_function_name_with_id(uint32_t symbol_id, const char* original_name)
   {
       char buffer[256];
       snprintf(buffer, sizeof(buffer), "casm_%u_%s", symbol_id, original_name);
       return xstrdup(buffer);
   }
   ```
   This ensures uniqueness even for identically-named functions from different modules.

3. **Modify codegen to use pre-computed names**:
   - Add `char* mangled_name` field to `ASTFunctionDef` (set during semantic analysis)
   - Use this field directly in codegen instead of calling `mangle_function_name()`

4. **Codegen for function calls**:
   ```c
   case EXPR_FUNCTION_CALL: {
       /* During semantic analysis, resolved_symbol_id is set */
       /* Codegen uses pre-computed mangled name from AST or symbol table */
       fprintf(out, "%s(", expr->as.function_call.mangled_name);
       /* ... */
   }
   ```

5. **WAT codegen** needs same updates:
   - Replace module-qualified names (e.g., `math:add`) with mangled IDs
   - Emit `(func $casm_1234_add ...` format with ID prefix for disambiguation

---

## 4. Module Loading (`src/module_loader.c`)

### Current Module Cache Structure

**File**: `src/module_loader.h` lines 7-22

```c
typedef struct {
    char* absolute_path;             /* Resolved absolute path */
    char* source_code;               /* File contents */
    char* module_name;               /* Import alias (not used) */
    ASTProgram* ast;                 /* Parsed AST */
} LoadedModule;

typedef struct {
    LoadedModule* modules;
    int count;
    int capacity;
    char** import_chain;             /* For circular import detection */
    int chain_depth;
} ModuleCache;
```

**Observations**:
1. `module_name` field exists but is always set to `NULL` (line 269 of `module_loader.c`)
2. No tracking of which functions come from which module
3. No deduplication logic at the module loading level

### Module Loading Process

**File**: `src/module_loader.c` lines 173-273

The `module_cache_load_internal()` function:
1. Resolves paths relative to import directories
2. Checks for circular imports using an import chain stack
3. Caches already-loaded modules by path
4. Recursively loads all imports
5. Stores each module's AST

**Key Section** (lines 351-386):
```c
/* Copy all functions from all modules */
for (int i = 0; i < cache->count; i++) {
    if (cache->modules[i].ast && cache->modules[i].ast->function_count > 0) {
        for (int j = 0; j < cache->modules[i].ast->function_count; j++) {
            ASTFunctionDef* src_func = &cache->modules[i].ast->functions[j];
            ASTFunctionDef* dst_func = &complete->functions[complete->function_count++];
            
            /* Copy function with original name (no deduplication) */
            dst_func->name = xstrdup(src_func->name);
            /* ... copy parameters ... */
        }
    }
}
```

**Merge Process Issues**:
1. Functions from different modules are merged into a single array
2. Names are not qualified with module information
3. No deduplication happens - if two modules define `add()`, the merge fails silently or causes duplicates
4. The AST returned has no metadata about function origins

### Changes Needed

1. **Extend LoadedModule** to track function origins:
   ```c
   typedef struct {
       char* absolute_path;
       char* source_code;
       char* module_name;
       ASTProgram* ast;
       
       /* NEW: Track function origins */
       typedef struct {
           uint32_t function_index;  /* Index in ast->functions */
           uint32_t symbol_id;       /* To be assigned by semantic analyzer */
       } FunctionOrigin;
       
       FunctionOrigin* function_origins;
       int function_origin_count;
   } LoadedModule;
   ```

2. **Build function origin map** during module merge:
   ```c
   /* After loading all modules, build a map of:
      module_path -> function_name -> function_index */
   typedef struct {
       char* module_path;
       char* function_name;
       uint32_t module_index;
       uint32_t function_index;
       ASTFunctionDef* function_ptr;
   } FunctionOriginMap;
   ```

3. **Deduplication at merge time**:
   - Before merging functions into single array:
     - Group functions by (signature, name)
     - Detect duplicates across modules
     - Create deduplication candidates
   - Pass this information to semantic analyzer

4. **Enhanced AST metadata**:
   ```c
   typedef struct {
       FunctionOriginMap* function_origins;
       int origin_count;
   } ASTMetadata;
   
   struct ASTProgram {
       /* ... existing fields ... */
       ASTMetadata* metadata;  /* NEW: Module origin information */
   };
   ```

5. **Return full context from build_complete_ast()**:
   - Instead of just returning merged AST
   - Return structure containing:
     - Merged AST
     - Module cache (for origin lookup)
     - Function origin mappings
     - Deduplication candidates identified

---

## Current Collision Detection

**File**: `src/semantics.c` lines 444-470

```c
static void validate_import_collisions(ASTProgram* program, SemanticErrorList* errors)
{
    for (int i = 0; i < program->import_count; i++) {
        ASTImportStatement* import1 = &program->imports[i];
        
        for (int j = i + 1; j < program->import_count; j++) {
            ASTImportStatement* import2 = &program->imports[j];
            
            /* Check if same name imported from different files */
            for (int a = 0; a < import1->name_count; a++) {
                for (int b = 0; b < import2->name_count; b++) {
                    if (strcmp(import1->imported_names[a], 
                              import2->imported_names[b]) == 0) {
                        /* Error: Function imported from both sources */
                    }
                }
            }
        }
    }
}
```

**This is the critical blocking point for multi-module support**. It explicitly rejects having the same function name available from multiple import sources.

---

## Summary of Current Limitations

| Area | Limitation | Impact |
|------|-----------|--------|
| **AST** | No unique IDs, no module origin tracking | Can't distinguish identical function names |
| **Semantic Analysis** | Name-only duplicate detection, rejected outright | Prevents valid multi-module compilation |
| **Symbol Table** | `module_name` field unused, linear name-based lookup | No support for qualified names or IDs |
| **Codegen** | Mangling expects qualified names that never exist | Function names never actually mangled |
| **Module Loading** | No function origin tracking, naive merge | Loses information about source modules |

---

## Proposed Architecture for Symbol IDs and Deduplication

### Phase 1: Extend Data Structures
1. Add `symbol_id` to `ASTFunctionDef` and `FunctionSymbol`
2. Add `module_path` to track origin
3. Create `SymbolDeduplicationInfo` structure
4. Extend `ASTMetadata` to store origin and deduplication info

### Phase 2: Deduplication Logic
1. After module loading, before semantic analysis
2. Build function signature map (return type + parameter types)
3. Identify candidate duplicates (same name, different modules)
4. For each candidate group:
   - If signatures match exactly: mark as deduplicable
   - If signatures differ: error (true conflict)
   - If only one implementation: use it
   - If multiple implementations: require explicit selection or use first

### Phase 3: Symbol ID Assignment
1. After deduplication decisions, assign symbol IDs
2. Update all references in AST
3. Compute mangled names (e.g., `casm_1234_add`)

### Phase 4: Codegen Integration
1. Semantic analyzer pre-computes mangled names
2. Codegen uses pre-computed names
3. Both C and WAT backends emit unique function names

### Phase 5: Runtime Linking
1. When linking compiled modules:
   - Deduplicated functions are linked once
   - Symbol IDs ensure correct function resolution
   - No name collisions at link time

---

## Implementation Entry Points

1. **Semantic Analysis** (`src/semantics.c`):
   - Add new pass between current passes 2 and 3
   - Call new `dedup_and_assign_symbol_ids()` function
   - Modify `symbol_table_lookup_function()` to support symbol IDs

2. **Module Loader** (`src/module_loader.c`):
   - Return additional metadata from `build_complete_ast()`
   - Build function origin mappings during merge

3. **Codegen** (`src/codegen.c` and `src/codegen_wat.c`):
   - Use pre-computed mangled names from AST
   - Remove dynamic mangling

4. **Types** (`src/types.h` and `src/types.c`):
   - Extend `FunctionSymbol` struct
   - Add new lookup functions by symbol ID
   - Implement deduplication logic

---

## Backward Compatibility

These changes should be backward compatible with single-module compilation:
- Single-module programs will have one symbol ID per function
- Deduplication phase will find no duplicates
- Mangled names will be simple (no ID prefix needed if only one function)
- Semantic analysis will pass all functions through normally

---

## Testing Strategy

1. **Unit tests** for deduplication logic
2. **Integration tests** for multi-module compilation
3. **Regression tests** for single-module programs
4. **Edge cases**:
   - Identical functions from multiple modules
   - Functions with same name but different signatures
   - Circular imports with function duplication
   - Deep module hierarchies

