#ifndef TYPES_H
#define TYPES_H

#include "ast.h"

/* Forward declarations */
typedef struct SymbolTable SymbolTable;
typedef struct Scope Scope;
typedef struct FunctionSymbol FunctionSymbol;
typedef struct VariableSymbol VariableSymbol;

/* Function symbol - stores function metadata */
struct FunctionSymbol {
    char* name;
    CasmType return_type;
    CasmType* param_types;
    int param_count;
    SourceLocation location;
};

/* Variable symbol - stores variable metadata */
struct VariableSymbol {
    char* name;
    CasmType type;
    SourceLocation location;
    int initialized;  /* Whether the variable has been assigned a value */
};

/* Scope - manages variables in a block */
struct Scope {
    VariableSymbol* variables;
    int variable_count;
    int variable_capacity;
    Scope* parent;  /* Parent scope for nested lookups */
};

/* Symbol table - manages all functions and scopes */
struct SymbolTable {
    FunctionSymbol* functions;
    int function_count;
    int function_capacity;
    Scope* current_scope;  /* Current scope during analysis */
};

/* Symbol table operations */
SymbolTable* symbol_table_create(void);
void symbol_table_free(SymbolTable* table);

/* Function operations */
int symbol_table_add_function(SymbolTable* table, const char* name, CasmType return_type,
                               CasmType* param_types, int param_count, SourceLocation location);
FunctionSymbol* symbol_table_lookup_function(SymbolTable* table, const char* name);

/* Variable operations */
int symbol_table_add_variable(SymbolTable* table, const char* name, CasmType type, SourceLocation location);
VariableSymbol* symbol_table_lookup_variable(SymbolTable* table, const char* name);
int symbol_table_mark_initialized(SymbolTable* table, const char* name);
int symbol_table_is_initialized(SymbolTable* table, const char* name);

/* Scope operations */
void symbol_table_push_scope(SymbolTable* table);
void symbol_table_pop_scope(SymbolTable* table);

/* Type operations */
int types_compatible(CasmType left, CasmType right);
int get_type_size_bits(CasmType type);  /* Returns bit width of type, -1 for non-numeric */
CasmType get_binary_op_result_type(CasmType left, BinaryOpType op, CasmType right);
CasmType get_unary_op_result_type(UnaryOpType op, CasmType operand);

/* Helper to check if type is numeric */
int is_numeric_type(CasmType type);

#endif /* TYPES_H */
