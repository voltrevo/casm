# Symbol Table & Type System Implementation Guide

## Overview
This task implements semantic analysis for the Casm compiler. After the parser builds an AST, this phase:
1. Builds a symbol table tracking all functions and variables
2. Validates type compatibility across operations
3. Reports semantic errors (undefined variables, type mismatches, duplicate functions)
4. Annotates AST nodes with resolved type information

## Architecture Decisions (FINAL)
- **Scope handling:** Full block scoping (nested scopes for future control flow)
- **Error handling:** Accumulate ALL errors (don't stop on first)
- **AST annotation:** Modify AST nodes to include `CasmType resolved_type` field
- **Type inference:** None - all types explicit in source
- **Variable shadowing:** Warn when inner scope shadows outer scope variable
- **Function overloading:** Not supported - duplicate function names are errors

## Current Pipeline (Post-Implementation)

```
source code
    ↓
Lexer (src/lexer.c)
    ↓
Parser (src/parser.c) → AST
    ↓
Semantic Analyzer (src/semantics.c) → Symbol Table + Type-annotated AST
    ↓
[Code Generator] (not yet implemented)
```

## Files to Create

### 1. src/types.h
**Purpose:** Data structures for symbol table and type system

**Key structures:**
```c
typedef struct {
    char* name;
    CasmType type;
    SourceLocation location;
} VariableSymbol;

typedef struct {
    char* name;
    CasmType return_type;
    CasmType* parameter_types;
    int parameter_count;
    SourceLocation location;
} FunctionSymbol;

typedef struct Scope {
    VariableSymbol* variables;
    int variable_count;
    struct Scope* parent;  // NULL if global scope
} Scope;

typedef struct {
    FunctionSymbol* functions;
    int function_count;
    Scope* global_scope;
    Scope* current_scope;
} SymbolTable;
```

**Public API functions:**
- `SymbolTable* symbol_table_create(void)`
- `void symbol_table_free(SymbolTable* table)`
- `int symbol_table_add_function(SymbolTable* table, const char* name, CasmType return_type, CasmType* param_types, int param_count, SourceLocation location)`
  - Returns: 1 on success, 0 if duplicate function
- `FunctionSymbol* symbol_table_lookup_function(SymbolTable* table, const char* name)`
  - Returns: function symbol or NULL if not found
- `int symbol_table_add_variable(SymbolTable* table, const char* name, CasmType type, SourceLocation location)`
  - Returns: 1 on success, 0 if duplicate in same scope
- `VariableSymbol* symbol_table_lookup_variable(SymbolTable* table, const char* name)`
  - Returns: variable symbol or NULL if not found
  - Searches from current scope up to global
- `void symbol_table_push_scope(SymbolTable* table)`
  - Creates new child scope
- `void symbol_table_pop_scope(SymbolTable* table)`
  - Exits current scope, returns to parent

**Type checking functions:**
- `int types_compatible(CasmType left, CasmType right)`
  - Returns: 1 if types can be used together, 0 otherwise
  - Rule: i8-i64 are compatible with each other, u8-u64 are compatible with each other, bool/void separate
- `CasmType get_binary_op_result_type(CasmType left, BinaryOpType op, CasmType right)`
  - Returns: result type for operation, or TYPE_VOID if incompatible
  - Handle all binary ops defined in ast.h
- `CasmType get_unary_op_result_type(UnaryOpType op, CasmType operand)`
  - Returns: result type for operation, or TYPE_VOID if incompatible

### 2. src/semantics.h
**Purpose:** Semantic analyzer API

**Error structure:**
```c
typedef struct {
    char* message;
    SourceLocation location;
} SemanticError;

typedef struct {
    SemanticError* errors;
    int error_count;
    int error_capacity;
} SemanticErrorList;
```

**Public API functions:**
- `SemanticErrorList* semantic_error_list_create(void)`
- `void semantic_error_list_free(SemanticErrorList* errors)`
- `void semantic_error_list_add(SemanticErrorList* errors, const char* message, SourceLocation location)`
- `void semantic_error_list_print(SemanticErrorList* errors, const char* filename)`
  - Same format as parser errors: `filename:line:column: message`
- `int analyze_program(ASTProgram* program, SymbolTable* table, SemanticErrorList* errors)`
  - Returns: 1 if no errors, 0 if errors found
  - Main entry point for semantic analysis

## Files to Modify

### src/ast.h
Add one field to each AST node structure that has a type:

```c
struct ASTExpression {
    ExpressionType type;
    SourceLocation location;
    CasmType resolved_type;  // NEW: set by semantic analyzer
    union { ... } as;
};

// Similar for ASTStatement (STMT_VAR_DECL has a type)
struct ASTVarDecl {
    char* name;
    TypeNode type;
    ASTExpression* initializer;
    SourceLocation location;
    // Note: type field already exists, no need to add resolved_type
};
```

Actually, for statements and variable decls, the type is already explicit in the source, so no need to add `resolved_type` - just use the existing `type` field. **Only add `resolved_type` to ASTExpression and ASTStatement structs where types aren't already explicit.**

### src/main.c
Update compilation pipeline:

```c
// After parsing:
if (parser->errors->error_count > 0) {
    error_list_print(parser->errors, argv[1]);
    // cleanup...
    return 1;
}

// NEW: Semantic analysis
SymbolTable* table = symbol_table_create();
SemanticErrorList* sem_errors = semantic_error_list_create();

if (!analyze_program(program, table, sem_errors)) {
    semantic_error_list_print(sem_errors, argv[1]);
    // cleanup...
    return 1;
}

// If we get here, semantic analysis passed
print_ast_program(program);

// cleanup including symbol table
symbol_table_free(table);
semantic_error_list_free(sem_errors);
ast_program_free(program);
parser_free(parser);
xfree(source);
return 0;
```

### Makefile
Add types.c and semantics.c to SOURCES:
```makefile
SOURCES = src/main.c src/lexer.c src/parser.c src/ast.c src/utils.c src/types.c src/semantics.c
```

## Implementation Details

### Phase 1: First Pass - Collect Function Definitions
Walk the AST once to collect all top-level function definitions into the symbol table:
- For each function in program->functions:
  - Call `symbol_table_add_function()` with return type and parameter types
  - If duplicate, add error "Function 'foo' already defined at line X"

### Phase 2: Second Pass - Validate Expressions and Variables
Walk the AST again to validate expressions and variable usage:
- For each variable reference (EXPR_VARIABLE):
  - Look up in symbol table
  - If not found, add error "Undefined variable 'x'"
  - Set `expr->resolved_type` to the variable's type
- For each variable declaration (STMT_VAR_DECL):
  - If initializer exists, validate initializer expression
  - Check if initializer type matches declared type
  - Add variable to current scope
  - Warn if shadowing outer scope variable: "Variable 'x' shadows declaration at line Y"
- For each function call (EXPR_FUNCTION_CALL):
  - Look up function in symbol table
  - If not found, add error "Undefined function 'foo'"
  - Validate argument count matches parameter count
  - Validate each argument type matches parameter type
  - Set `expr->resolved_type` to function's return type
- For each binary operation (EXPR_BINARY_OP):
  - Validate left and right operands recursively
  - Check if left.type and right.type are compatible
  - If not compatible, add error describing mismatch
  - Set `expr->resolved_type` using `get_binary_op_result_type()`
- For each unary operation (EXPR_UNARY_OP):
  - Validate operand recursively
  - Check if operand type is compatible with operator
  - If not, add error
  - Set `expr->resolved_type` using `get_unary_op_result_type()`
- For return statements (STMT_RETURN):
  - If return has value, validate expression
  - Check if expression type matches function return type
  - If mismatch, add error "Return type mismatch: expected TYPE_X, got TYPE_Y"

### Scope Management During Phase 2
- When entering a function body (parse_block for function):
  - Call `symbol_table_push_scope()`
  - Add all function parameters as variables to the scope
- When exiting a function body:
  - Call `symbol_table_pop_scope()`
- Same for any block `{ ... }` (though we don't parse those yet)

### Type Compatibility Rules
Implement in `types_compatible()`:
- Signed integers: i8, i16, i32, i64 are compatible with each other
- Unsigned integers: u8, u16, u32, u64 are compatible with each other
- Signed and unsigned are NOT compatible
- bool is only compatible with bool
- void is only compatible with void (for functions)
- No implicit conversions

Example:
```c
int types_compatible(CasmType left, CasmType right) {
    // Same type is always compatible
    if (left == right) return 1;
    
    // Signed integer compatibility
    if ((left >= TYPE_I8 && left <= TYPE_I64) &&
        (right >= TYPE_I8 && right <= TYPE_I64)) {
        return 1;
    }
    
    // Unsigned integer compatibility
    if ((left >= TYPE_U8 && left <= TYPE_U64) &&
        (right >= TYPE_U8 && right <= TYPE_U64)) {
        return 1;
    }
    
    return 0;
}
```

### Binary Operation Type Rules
Implement in `get_binary_op_result_type()`:
- Arithmetic ops (+, -, *, /, %): both operands must be numeric (i* or u*), result is widest type
- Comparison ops (<, >, <=, >=): both must be numeric, result is bool
- Equality ops (==, !=): both must be compatible, result is bool
- Logical ops (&&, ||): both must be bool, result is bool

### Unary Operation Type Rules
Implement in `get_unary_op_result_type()`:
- Negation (-): operand must be numeric, result is same type
- Logical not (!): operand must be bool, result is bool

## Testing Strategy

Create tests/test_semantics.c with test cases for:

1. **Symbol table basic operations:**
   - Add function, lookup function
   - Add variable, lookup variable
   - Scope push/pop
   
2. **Type compatibility:**
   - Same types compatible
   - Different signed integer types compatible
   - Different unsigned integer types compatible
   - Signed vs unsigned incompatible
   - bool only compatible with bool

3. **Binary operations:**
   - Valid arithmetic operations on integers
   - Invalid arithmetic on bool
   - Comparisons result in bool
   - Logical ops require bool operands

4. **Unary operations:**
   - Negation on numeric types
   - Not on bool

5. **Semantic errors:**
   - Undefined variable usage
   - Undefined function call
   - Function parameter type mismatch
   - Return type mismatch
   - Duplicate function definition
   - Variable shadowing (should warn, not error)

6. **Valid programs:**
   - All existing example files should pass analysis
   - Create new test cases for different features

## Integration with Makefile and Tests

After implementation:
- `make build-debug` will compile types.c and semantics.c
- `make test` will run both lexer tests AND semantic analysis tests
- Update run_tests.sh to include semantic tests if needed
- All existing example files should still parse and pass semantic analysis

## Error Messages Format

All errors should follow the established format:
```
filename:line:column: error message
```

Examples:
```
main.c:5:10: Undefined variable 'x'
main.c:12:5: Function 'foo' already defined at line 8
main.c:20:15: Type mismatch: cannot add i32 and bool
main.c:7:5: Variable 'x' shadows declaration at line 3
```

## Performance Considerations

- Symbol table is small (functions + variables in scope), no optimization needed
- Two-pass analysis is simple: first collect functions, second validate
- No need for caching or memoization at this stage

## Future Extensions (Not in Scope)

- Type inference
- Function overloading
- Implicit type conversions
- Generic types
- Module system
- Symbol table persistence across files
