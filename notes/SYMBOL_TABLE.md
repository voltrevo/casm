# Symbol Table & Type System

## What to Build

Create `src/types.h` and `src/types.c` with:
- `SymbolTable` structure (functions array, scopes)
- `Scope` structure (variables array, parent pointer for nesting)
- `FunctionSymbol` and `VariableSymbol` structures

Create `src/semantics.h` and `src/semantics.c` with:
- Semantic analyzer that does 2 passes:
  1. Collect all function definitions
  2. Validate expressions/variables and assign `resolved_type`

## Key Functions

**Symbol Table:**
- `symbol_table_create/free()`
- `add_function(name, return_type, param_types, param_count)` - return 0 on duplicate
- `lookup_function(name)` - return NULL if not found
- `add_variable(name, type)` - return 0 on duplicate in same scope
- `lookup_variable(name)` - search current scope upward
- `push_scope()` / `pop_scope()` - for nested blocks

**Type Operations:**
- `types_compatible(left, right)` - signed ints compatible with each other, unsigned with each other, bool/void separate
- `get_binary_op_result_type(left, op, right)` - result type for operations
- `get_unary_op_result_type(op, operand)` - result type for unary ops

## Changes to Existing Files

**src/ast.h:** Add to `ASTExpression`:
```c
CasmType resolved_type;  // filled by semantic analyzer
```

**src/main.c:** After parser succeeds:
```c
SymbolTable* table = symbol_table_create();
SemanticErrorList* errors = semantic_error_list_create();
if (!analyze_program(program, table, errors)) {
    semantic_error_list_print(errors, argv[1]);
    return 1;
}
// AST is now validated and type-annotated
```

**Makefile:** Add `src/types.c src/semantics.c` to SOURCES

## Type Rules

- Arithmetic ops (+,-,*,/,%): numeric types, result = widest input
- Comparisons (<,>,<=,>=): compatible types, result = bool
- Logical ops (&&,||): bool only, result = bool
- Unary -: numeric types
- Unary !: bool only
- Function calls: args must match parameter types exactly
- Returns: expression type must match function return type

## Errors to Detect

1. Undefined variable/function (lookup fails)
2. Type mismatch in binary/unary/function args/returns
3. Duplicate function definition
4. Variable shadowing (warn, don't error)

## Testing

Write `tests/test_semantics.c` covering:
- Symbol table insert/lookup/scope operations
- Type compatibility all cases
- All error types
- Valid programs pass

Run `make test` - should complete in <600ms with all tests passing.
