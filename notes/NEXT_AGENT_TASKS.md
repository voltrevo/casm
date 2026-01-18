# Next Steps: Symbol Table & Type System Implementation

## What You Need to Accomplish

You are implementing **semantic analysis** for the Casm compiler. After the parser builds an AST, your code will:

1. Build a symbol table tracking all functions and variables
2. Validate that types are used correctly throughout the program
3. Report any semantic errors (undefined variables, type mismatches, etc.)
4. Annotate AST nodes with resolved type information for later code generation

This is a prerequisite for code generation - the C code generator needs to know the type of every expression.

## Your Task in 5 Steps

### Step 1: Implement src/types.h and src/types.c

**Create src/types.h** with:
- Data structures for VariableSymbol, FunctionSymbol, Scope, SymbolTable
- Function declarations for symbol table operations
- Function declarations for type checking operations

**Create src/types.c** with:
- All symbol table operations (create, free, add, lookup, push/pop scope)
- Type compatibility checking
- Type result type calculation for operations

**Key functions to implement:**
```c
// Lifecycle
SymbolTable* symbol_table_create(void)
void symbol_table_free(SymbolTable* table)

// Function management
int symbol_table_add_function(SymbolTable* table, const char* name, ...)
FunctionSymbol* symbol_table_lookup_function(SymbolTable* table, const char* name)

// Variable management
int symbol_table_add_variable(SymbolTable* table, const char* name, ...)
VariableSymbol* symbol_table_lookup_variable(SymbolTable* table, const char* name)

// Scope management
void symbol_table_push_scope(SymbolTable* table)
void symbol_table_pop_scope(SymbolTable* table)

// Type operations
int types_compatible(CasmType left, CasmType right)
CasmType get_binary_op_result_type(CasmType left, BinaryOpType op, CasmType right)
CasmType get_unary_op_result_type(UnaryOpType op, CasmType operand)
```

**Reference:** See notes/SYMBOL_TABLE_IMPLEMENTATION.md section "Files to Create" for detailed specs.

### Step 2: Implement src/semantics.h and src/semantics.c

**Create src/semantics.h** with:
- SemanticError and SemanticErrorList structures
- Function declarations for semantic analysis

**Create src/semantics.c** with:
- Two-phase analyzer:
  - Phase 1: Collect all function definitions
  - Phase 2: Validate all expressions, variables, and type usage
- Functions to add/print semantic errors
- Main entry point: `int analyze_program(ASTProgram*, SymbolTable*, SemanticErrorList*)`

**Key functions to implement:**
```c
SemanticErrorList* semantic_error_list_create(void)
void semantic_error_list_free(SemanticErrorList* errors)
void semantic_error_list_add(SemanticErrorList* errors, const char* msg, SourceLocation loc)
void semantic_error_list_print(SemanticErrorList* errors, const char* filename)
int analyze_program(ASTProgram* program, SymbolTable* table, SemanticErrorList* errors)
```

**Reference:** See notes/SYMBOL_TABLE_IMPLEMENTATION.md section "Implementation Details" for detailed algorithm.

### Step 3: Modify src/ast.h

Add one field to ASTExpression:
```c
struct ASTExpression {
    ExpressionType type;
    SourceLocation location;
    CasmType resolved_type;  // NEW - set by semantic analyzer
    union { ... } as;
};
```

This allows the code generator to quickly find the type of any expression without re-analyzing it.

### Step 4: Modify src/main.c

Update the compilation pipeline after parser succeeds:
```c
// After parsing successfully...

// NEW: Semantic analysis
SymbolTable* table = symbol_table_create();
SemanticErrorList* sem_errors = semantic_error_list_create();

if (!analyze_program(program, table, sem_errors)) {
    semantic_error_list_print(sem_errors, argv[1]);
    // cleanup...
    return 1;
}

// AST is now validated and type-annotated
print_ast_program(program);

// cleanup
symbol_table_free(table);
semantic_error_list_free(sem_errors);
ast_program_free(program);
parser_free(parser);
xfree(source);
return 0;
```

### Step 5: Update Makefile

Add types.c and semantics.c to SOURCES:
```makefile
SOURCES = src/main.c src/lexer.c src/parser.c src/ast.c src/utils.c src/types.c src/semantics.c
```

## Testing Your Implementation

### Unit Tests (Create tests/test_semantics.c)

Write test cases covering:
1. Symbol table: insert, lookup, scope push/pop
2. Type compatibility: all valid and invalid combinations
3. Semantic errors: all error types
4. Valid programs: ensure existing examples pass

Example test structure:
```c
void test_symbol_table_lookup(void) {
    SymbolTable* table = symbol_table_create();
    CasmType param_types[] = {TYPE_I32};
    
    symbol_table_add_function(table, "add", TYPE_I32, param_types, 1, (SourceLocation){0,0,0});
    FunctionSymbol* found = symbol_table_lookup_function(table, "add");
    
    assert(found != NULL);
    assert(found->return_type == TYPE_I32);
    
    symbol_table_free(table);
}
```

### Integration Tests

Run existing examples:
```bash
make test
```

All 4 supported examples should pass semantic analysis:
- examples/simple_add.csm ✓
- examples/variables.csm ✓
- examples/function_call.csm ✓
- examples/mixed_types.csm ✓

## Success Criteria

Your implementation is complete when:

1. ✅ All 4 example files parse AND pass semantic analysis
2. ✅ No compiler warnings or errors in new code
3. ✅ All unit tests pass (106 lexer + your new semantic tests)
4. ✅ Full test suite runs in <600ms
5. ✅ No memory leaks (AddressSanitizer clean)
6. ✅ AST nodes have `resolved_type` filled in
7. ✅ Semantic errors reported in `file:line:col: message` format
8. ✅ Variable shadowing produces warnings (not errors)
9. ✅ Duplicate functions produce errors
10. ✅ Undefined variable/function references produce errors

## Key Implementation Details

### Two-Pass Analysis

**First pass:**
- Walk program->functions array
- Call symbol_table_add_function() for each
- If duplicate, add error immediately
- This ensures all functions are known before validating expressions

**Second pass:**
- Walk statements and expressions
- Validate variable references
- Validate function calls
- Validate type compatibility
- Set resolved_type on expressions

### Scope Management

When analyzing a function:
1. Push new scope: `symbol_table_push_scope(table)`
2. Add all parameters as variables to new scope
3. Walk function body statements
4. Pop scope when done: `symbol_table_pop_scope(table)`

For blocks (will be needed for if/while/for in future):
1. Push scope on entering `{`
2. Add any variables declared in block
3. Walk block statements
4. Pop scope on exiting `}`

### Type Checking for Expressions

For each expression type, recursively validate sub-expressions:

**Variables (EXPR_VARIABLE):**
```c
// Look up in symbol table
VariableSymbol* var = symbol_table_lookup_variable(table, name);
if (!var) {
    add_error("Undefined variable '%s'", name);
    expr->resolved_type = TYPE_I32;  // default type
} else {
    expr->resolved_type = var->type;
}
```

**Function Calls (EXPR_FUNCTION_CALL):**
```c
// Look up function
FunctionSymbol* func = symbol_table_lookup_function(table, func_name);
if (!func) {
    add_error("Undefined function '%s'", func_name);
} else {
    // Check argument count
    if (arg_count != func->parameter_count) {
        add_error("Function expects %d args, got %d", 
                  func->parameter_count, arg_count);
    }
    // Check each argument type
    for (int i = 0; i < arg_count; i++) {
        if (!types_compatible(args[i]->resolved_type, func->parameter_types[i])) {
            add_error("Argument %d: type mismatch", i);
        }
    }
    expr->resolved_type = func->return_type;
}
```

**Binary Operations (EXPR_BINARY_OP):**
```c
// Recursively validate operands
validate_expression(left);
validate_expression(right);

// Check type compatibility
if (!types_compatible(left->resolved_type, right->resolved_type)) {
    add_error("Cannot perform %s on types %s and %s",
              op_name, type_name(left->resolved_type), 
              type_name(right->resolved_type));
}

// Get result type
expr->resolved_type = get_binary_op_result_type(
    left->resolved_type, op, right->resolved_type);
```

## Common Pitfalls to Avoid

1. **Scope lookup direction:** Look from current scope UP to global, not just current
2. **Shadowing warnings:** Only warn if inner scope variable matches outer scope, not duplicates in same scope
3. **Type result calculation:** Remember result type depends on operation AND operand types
4. **Function parameters:** Add all parameters to scope before validating body
5. **Error recovery:** Continue validating after errors to find more problems
6. **Memory cleanup:** Free symbol table and error list properly in main.c

## Files You Need to Read First

1. **src/ast.h** - Understand AST node structures
2. **src/types.h** (after creating it) - Type enum definitions
3. **notes/SYMBOL_TABLE_IMPLEMENTATION.md** - Complete specification
4. **src/parser.h** - Error handling pattern to follow

## Getting Help

- **Lexer/Parser behavior:** Look at src/lexer.c and src/parser.c for patterns
- **Memory management:** Look at src/utils.h for xmalloc/xfree and src/ast.c for example cleanup
- **Type system:** See notes/SYMBOL_TABLE_IMPLEMENTATION.md section "Type Compatibility Rules"
- **Error messages:** See PROJECT_STATUS.md section "Error Messages Format"

## When You're Done

1. Commit with message: "Implement symbol table and semantic analysis"
2. Verify: `make clean && make test` passes with <600ms execution
3. Verify: `./casm examples/simple_add.csm` shows AST with resolved types
4. Next task: Someone implements C code generator

Good luck! You're implementing a critical piece of the compiler pipeline.
