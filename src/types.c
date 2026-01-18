#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "utils.h"

/* Symbol table creation and destruction */
SymbolTable* symbol_table_create(void) {
    SymbolTable* table = xmalloc(sizeof(SymbolTable));
    table->functions = xmalloc(10 * sizeof(FunctionSymbol));
    table->function_count = 0;
    table->function_capacity = 10;
    
    /* Create global scope */
    table->current_scope = xmalloc(sizeof(Scope));
    table->current_scope->variables = xmalloc(20 * sizeof(VariableSymbol));
    table->current_scope->variable_count = 0;
    table->current_scope->variable_capacity = 20;
    table->current_scope->parent = NULL;
    
    return table;
}

static void scope_free(Scope* scope) {
    if (!scope) return;
    if (scope->parent) {
        scope_free(scope->parent);
    }
    for (int i = 0; i < scope->variable_count; i++) {
        xfree(scope->variables[i].name);
    }
    xfree(scope->variables);
    xfree(scope);
}

void symbol_table_free(SymbolTable* table) {
    if (!table) return;
    
    for (int i = 0; i < table->function_count; i++) {
        xfree(table->functions[i].name);
        xfree(table->functions[i].module_name);
        if (table->functions[i].param_types) {
            xfree(table->functions[i].param_types);
        }
    }
    xfree(table->functions);
    
    if (table->current_scope) {
        scope_free(table->current_scope);
    }
    
    xfree(table);
}

/* Add a function to the symbol table */
int symbol_table_add_function(SymbolTable* table, const char* name, CasmType return_type,
                               CasmType* param_types, int param_count, SourceLocation location) {
    /* Check for duplicates */
    for (int i = 0; i < table->function_count; i++) {
        if (strcmp(table->functions[i].name, name) == 0) {
            return 0;  /* Duplicate function */
        }
    }
    
    /* Expand if needed */
    if (table->function_count >= table->function_capacity) {
        table->function_capacity *= 2;
        table->functions = xrealloc(table->functions, table->function_capacity * sizeof(FunctionSymbol));
    }
    
    FunctionSymbol* func = &table->functions[table->function_count];
    func->name = xmalloc(strlen(name) + 1);
    strcpy(func->name, name);
    func->module_name = NULL;  /* No module for locally defined functions */
    func->return_type = return_type;
    func->param_count = param_count;
    func->location = location;
    
    if (param_count > 0) {
        func->param_types = xmalloc(param_count * sizeof(CasmType));
        memcpy(func->param_types, param_types, param_count * sizeof(CasmType));
    } else {
        func->param_types = NULL;
    }
    
    table->function_count++;
    return 1;  /* Success */
}

/* Look up a function */
FunctionSymbol* symbol_table_lookup_function(SymbolTable* table, const char* name) {
    for (int i = 0; i < table->function_count; i++) {
        if (strcmp(table->functions[i].name, name) == 0) {
            return &table->functions[i];
        }
    }
    return NULL;
}

/* Add a variable to the current scope */
int symbol_table_add_variable(SymbolTable* table, const char* name, CasmType type, SourceLocation location) {
    Scope* scope = table->current_scope;
    
    /* Check for duplicates in current scope only */
    for (int i = 0; i < scope->variable_count; i++) {
        if (strcmp(scope->variables[i].name, name) == 0) {
            return 0;  /* Duplicate variable in same scope */
        }
    }
    
    /* Expand if needed */
    if (scope->variable_count >= scope->variable_capacity) {
        scope->variable_capacity *= 2;
        scope->variables = xrealloc(scope->variables, scope->variable_capacity * sizeof(VariableSymbol));
    }
    
    VariableSymbol* var = &scope->variables[scope->variable_count];
    var->name = xmalloc(strlen(name) + 1);
    strcpy(var->name, name);
    var->type = type;
    var->location = location;
    var->initialized = 0;  /* Initially uninitialized */
    
    scope->variable_count++;
    return 1;  /* Success */
}

/* Look up a variable (searches up the scope chain) */
VariableSymbol* symbol_table_lookup_variable(SymbolTable* table, const char* name) {
    Scope* scope = table->current_scope;
    
    while (scope) {
        for (int i = 0; i < scope->variable_count; i++) {
            if (strcmp(scope->variables[i].name, name) == 0) {
                return &scope->variables[i];
            }
        }
        scope = scope->parent;
    }
    
    return NULL;
}

/* Mark a variable as initialized */
int symbol_table_mark_initialized(SymbolTable* table, const char* name) {
    VariableSymbol* var = symbol_table_lookup_variable(table, name);
    if (!var) {
        return 0;  /* Variable not found */
    }
    var->initialized = 1;
    return 1;
}

/* Check if a variable is initialized */
int symbol_table_is_initialized(SymbolTable* table, const char* name) {
    VariableSymbol* var = symbol_table_lookup_variable(table, name);
    if (!var) {
        return 0;  /* Variable not found */
    }
    return var->initialized;
}

/* Push a new scope */
void symbol_table_push_scope(SymbolTable* table) {
    Scope* new_scope = xmalloc(sizeof(Scope));
    new_scope->variables = xmalloc(20 * sizeof(VariableSymbol));
    new_scope->variable_count = 0;
    new_scope->variable_capacity = 20;
    new_scope->parent = table->current_scope;
    
    table->current_scope = new_scope;
}

/* Pop the current scope */
void symbol_table_pop_scope(SymbolTable* table) {
    if (!table->current_scope || !table->current_scope->parent) {
        return;  /* Can't pop the global scope */
    }
    
    Scope* old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    
    /* Free variables in old scope */
    for (int i = 0; i < old_scope->variable_count; i++) {
        xfree(old_scope->variables[i].name);
    }
    xfree(old_scope->variables);
    xfree(old_scope);
}

/* Check if two types are compatible */
/* Enforce stricter type checking to prevent most narrowing conversions.
   Allow exact match, widening conversions, and i64/u64 narrowing (as these
   are used as default types for integer literals).
   
   This improves type safety while being pragmatic about literal defaults.
   
   Arguments: left = source type, right = target type
*/
int types_compatible(CasmType left, CasmType right) {
    if (left == right) {
        return 1;  /* Exact match */
    }
    
    /* Get size of each type */
    int source_size = get_type_size_bits(left);
    int target_size = get_type_size_bits(right);
    
    /* If either is not numeric, they must be exact match */
    if (source_size < 0 || target_size < 0) {
        return 0;
    }
    
    /* Both must be same signedness (both signed or both unsigned) */
    int source_signed = (left >= TYPE_I8 && left <= TYPE_I64);
    int target_signed = (right >= TYPE_I8 && right <= TYPE_I64);
    
    if (source_signed != target_signed) {
        return 0;  /* Can't mix signed and unsigned */
    }
    
    /* Allow widening: source type must be <= target type */
    if (source_size <= target_size) {
        return 1;
    }
    
    /* Allow i64/u64 to narrow to smaller integers
       (i64/u64 are default types for integer literals) */
    if ((left == TYPE_I64 && target_signed) || (left == TYPE_U64 && !target_signed)) {
        return 1;
    }
    
    return 0;  /* Prevent other narrowing conversions */
}

/* Get the bit width of a type */
int get_type_size_bits(CasmType type) {
    switch (type) {
        case TYPE_I8:
        case TYPE_U8:
            return 8;
        case TYPE_I16:
        case TYPE_U16:
            return 16;
        case TYPE_I32:
        case TYPE_U32:
            return 32;
        case TYPE_I64:
        case TYPE_U64:
            return 64;
        default:
            return -1;  /* Not a numeric type */
    }
}

/* Get result type for binary operations */
CasmType get_binary_op_result_type(CasmType left, BinaryOpType op, CasmType right) {
    switch (op) {
        case BINOP_ADD:
        case BINOP_SUB:
        case BINOP_MUL:
        case BINOP_DIV:
        case BINOP_MOD:
            /* Arithmetic: result is the wider type */
            if (left == TYPE_I64 || right == TYPE_I64) return TYPE_I64;
            if (left == TYPE_I32 || right == TYPE_I32) return TYPE_I32;
            if (left == TYPE_I16 || right == TYPE_I16) return TYPE_I16;
            if (left == TYPE_I8 || right == TYPE_I8) return TYPE_I8;
            if (left == TYPE_U64 || right == TYPE_U64) return TYPE_U64;
            if (left == TYPE_U32 || right == TYPE_U32) return TYPE_U32;
            if (left == TYPE_U16 || right == TYPE_U16) return TYPE_U16;
            return TYPE_U8;
        
        case BINOP_EQ:
        case BINOP_NE:
        case BINOP_LT:
        case BINOP_GT:
        case BINOP_LE:
        case BINOP_GE:
            /* Comparisons return bool */
            return TYPE_BOOL;
        
        case BINOP_AND:
        case BINOP_OR:
            /* Logical ops return bool */
            return TYPE_BOOL;
        
        case BINOP_ASSIGN:
            /* Assignment returns the type of the right operand (should be handled in semantics) */
            return right;
    }
    
    return TYPE_VOID;  /* Should not reach */
}

/* Get result type for unary operations */
CasmType get_unary_op_result_type(UnaryOpType op, CasmType operand) {
    switch (op) {
        case UNOP_NEG:
            /* Negation preserves type */
            return operand;
        case UNOP_NOT:
            /* Logical not returns bool */
            return TYPE_BOOL;
    }
    return TYPE_VOID;  /* Should not reach */
}

/* Check if type is numeric */
int is_numeric_type(CasmType type) {
    return type >= TYPE_I8 && type <= TYPE_U64;
}

/* Parse a qualified name like "math:add" into module and function names
 * If no colon, module_name is NULL
 */
void parse_qualified_name(const char* qualified_name, char** out_module, char** out_function) {
    const char* colon = strchr(qualified_name, ':');
    
    if (!colon) {
        /* No colon - it's just a function name */
        *out_module = NULL;
        *out_function = xstrdup(qualified_name);
    } else {
        /* Split at colon */
        int module_len = colon - qualified_name;
        *out_module = xstrndup(qualified_name, module_len);
        *out_function = xstrdup(colon + 1);  /* Skip the colon */
    }
}
