#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "semantics.h"

/* Semantic error list operations */
SemanticErrorList* semantic_error_list_create(void) {
    SemanticErrorList* list = xmalloc(sizeof(SemanticErrorList));
    list->errors = xmalloc(10 * sizeof(SemanticError));
    list->error_count = 0;
    list->error_capacity = 10;
    return list;
}

void semantic_error_list_free(SemanticErrorList* list) {
    if (!list) return;
    
    for (int i = 0; i < list->error_count; i++) {
        xfree(list->errors[i].message);
    }
    xfree(list->errors);
    xfree(list);
}

void semantic_error_list_add(SemanticErrorList* list, const char* message, SourceLocation location) {
    if (list->error_count >= list->error_capacity) {
        list->error_capacity *= 2;
        list->errors = xrealloc(list->errors, list->error_capacity * sizeof(SemanticError));
    }
    
    SemanticError* error = &list->errors[list->error_count];
    error->message = xmalloc(strlen(message) + 1);
    strcpy(error->message, message);
    error->location = location;
    
    list->error_count++;
}

void semantic_error_list_print(SemanticErrorList* list, const char* filename) {
    for (int i = 0; i < list->error_count; i++) {
        SemanticError* error = &list->errors[i];
        fprintf(stderr, "%s:%d:%d: %s\n", filename, error->location.line, error->location.column, error->message);
    }
}

/* Forward declarations */
static CasmType analyze_expression(ASTExpression* expr, SymbolTable* table, SemanticErrorList* errors);
static void analyze_statement(ASTStatement* stmt, SymbolTable* table, CasmType return_type, SemanticErrorList* errors);
static void analyze_block(ASTBlock* block, SymbolTable* table, CasmType return_type, SemanticErrorList* errors);

/* Analyze an expression and return its type */
static CasmType analyze_expression(ASTExpression* expr, SymbolTable* table, SemanticErrorList* errors) {
    if (!expr) {
        return TYPE_VOID;
    }
    
    switch (expr->type) {
        case EXPR_LITERAL: {
            if (expr->as.literal.type == LITERAL_INT) {
                expr->resolved_type = TYPE_I64;  /* Default int type */
            } else {
                expr->resolved_type = TYPE_BOOL;
            }
            return expr->resolved_type;
        }
        
        case EXPR_VARIABLE: {
            VariableSymbol* var = symbol_table_lookup_variable(table, expr->as.variable.name);
            if (!var) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Undefined variable '%s'", expr->as.variable.name);
                semantic_error_list_add(errors, msg, expr->location);
                expr->resolved_type = TYPE_VOID;
                return TYPE_VOID;
            }
            
            /* Check if variable is initialized before use */
            if (!var->initialized) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Variable '%s' used before initialization", expr->as.variable.name);
                semantic_error_list_add(errors, msg, expr->location);
            }
            
            expr->resolved_type = var->type;
            return var->type;
        }
        
        case EXPR_BINARY_OP: {
            BinaryOpType op = expr->as.binary_op.op;
            
            CasmType left_type;
            CasmType right_type;
            
            /* For assignment, don't check initialization on left side */
            if (op == BINOP_ASSIGN) {
                /* Get type of left side without checking initialization */
                if (expr->as.binary_op.left->type == EXPR_VARIABLE) {
                    VariableSymbol* var = symbol_table_lookup_variable(table, expr->as.binary_op.left->as.variable.name);
                    if (!var) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Undefined variable '%s'", expr->as.binary_op.left->as.variable.name);
                        semantic_error_list_add(errors, msg, expr->as.binary_op.left->location);
                        left_type = TYPE_VOID;
                    } else {
                        left_type = var->type;
                        expr->as.binary_op.left->resolved_type = var->type;
                    }
                } else {
                    semantic_error_list_add(errors, "Can only assign to variables", expr->location);
                    left_type = TYPE_VOID;
                }
                
                right_type = analyze_expression(expr->as.binary_op.right, table, errors);
                
                if (left_type != TYPE_VOID && !types_compatible(left_type, right_type)) {
                    semantic_error_list_add(errors, "Assignment type mismatch", expr->location);
                }
                
                /* Mark the variable as initialized */
                if (expr->as.binary_op.left->type == EXPR_VARIABLE) {
                    symbol_table_mark_initialized(table, expr->as.binary_op.left->as.variable.name);
                }
                
                /* Assignment expression has the type of the left-hand side */
                expr->resolved_type = left_type;
                return left_type;
            }
            
            /* For other operators, analyze both sides normally */
            left_type = analyze_expression(expr->as.binary_op.left, table, errors);
            right_type = analyze_expression(expr->as.binary_op.right, table, errors);
            
            /* Check type compatibility */
            if (op >= BINOP_ADD && op <= BINOP_MOD) {
                /* Arithmetic operators - both must be numeric and compatible */
                if (!is_numeric_type(left_type) || !is_numeric_type(right_type)) {
                    semantic_error_list_add(errors, "Arithmetic operators require numeric operands", expr->location);
                    expr->resolved_type = TYPE_VOID;
                    return TYPE_VOID;
                }
                if (!types_compatible(left_type, right_type)) {
                    semantic_error_list_add(errors, "Operands must have compatible types", expr->location);
                    expr->resolved_type = TYPE_VOID;
                    return TYPE_VOID;
                }
            } else if (op >= BINOP_LT && op <= BINOP_GE) {
                /* Comparison operators - both must be numeric and compatible */
                if (!is_numeric_type(left_type) || !is_numeric_type(right_type)) {
                    semantic_error_list_add(errors, "Comparison operators require numeric operands", expr->location);
                    expr->resolved_type = TYPE_BOOL;
                    return TYPE_BOOL;
                }
                if (!types_compatible(left_type, right_type)) {
                    semantic_error_list_add(errors, "Operands must have compatible types", expr->location);
                    expr->resolved_type = TYPE_BOOL;
                    return TYPE_BOOL;
                }
            } else if (op == BINOP_AND || op == BINOP_OR) {
                /* Logical operators - both must be bool */
                if (left_type != TYPE_BOOL) {
                    semantic_error_list_add(errors, "Logical AND/OR require boolean operands", expr->location);
                }
                if (right_type != TYPE_BOOL) {
                    semantic_error_list_add(errors, "Logical AND/OR require boolean operands", expr->location);
                }
            }
            
            expr->resolved_type = get_binary_op_result_type(left_type, op, right_type);
            return expr->resolved_type;
        }
        
        case EXPR_UNARY_OP: {
            CasmType operand_type = analyze_expression(expr->as.unary_op.operand, table, errors);
            
            UnaryOpType op = expr->as.unary_op.op;
            
            if (op == UNOP_NEG) {
                if (!is_numeric_type(operand_type)) {
                    semantic_error_list_add(errors, "Unary negation requires numeric operand", expr->location);
                }
            } else if (op == UNOP_NOT) {
                if (operand_type != TYPE_BOOL) {
                    semantic_error_list_add(errors, "Logical NOT requires boolean operand", expr->location);
                }
            }
            
            expr->resolved_type = get_unary_op_result_type(op, operand_type);
            return expr->resolved_type;
        }
        
        case EXPR_FUNCTION_CALL: {
            FunctionSymbol* func = symbol_table_lookup_function(table, expr->as.function_call.function_name);
            
            if (!func) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Undefined function '%s'", expr->as.function_call.function_name);
                semantic_error_list_add(errors, msg, expr->location);
                expr->resolved_type = TYPE_VOID;
                return TYPE_VOID;
            }
            
            /* Check argument count */
            if (expr->as.function_call.argument_count != func->param_count) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Function '%s' expects %d arguments, got %d",
                         expr->as.function_call.function_name, func->param_count,
                         expr->as.function_call.argument_count);
                semantic_error_list_add(errors, msg, expr->location);
            }
            
            /* Check argument types */
            for (int i = 0; i < expr->as.function_call.argument_count && i < func->param_count; i++) {
                CasmType arg_type = analyze_expression(&expr->as.function_call.arguments[i], table, errors);
                if (!types_compatible(arg_type, func->param_types[i])) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Argument %d type mismatch", i + 1);
                    semantic_error_list_add(errors, msg, expr->location);
                }
            }
            
            expr->resolved_type = func->return_type;
            return func->return_type;
        }
    }
    
    expr->resolved_type = TYPE_VOID;
    return TYPE_VOID;
}

/* Analyze a statement */
static void analyze_statement(ASTStatement* stmt, SymbolTable* table, CasmType return_type, SemanticErrorList* errors) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_RETURN: {
            if (stmt->as.return_stmt.value) {
                CasmType expr_type = analyze_expression(stmt->as.return_stmt.value, table, errors);
                if (!types_compatible(expr_type, return_type)) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Return type mismatch: expected %s", type_to_string(return_type));
                    semantic_error_list_add(errors, msg, stmt->location);
                }
            } else {
                if (return_type != TYPE_VOID) {
                    semantic_error_list_add(errors, "Function must return a value", stmt->location);
                }
            }
            break;
        }
        
        case STMT_VAR_DECL: {
            ASTVarDecl* var_decl = &stmt->as.var_decl_stmt.var_decl;
            
            /* Add variable to symbol table */
            if (!symbol_table_add_variable(table, var_decl->name, var_decl->type.type, var_decl->location)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Variable '%s' already declared in this scope", var_decl->name);
                semantic_error_list_add(errors, msg, var_decl->location);
            }
            
            /* Analyze initializer if present */
            if (var_decl->initializer) {
                CasmType init_type = analyze_expression(var_decl->initializer, table, errors);
                if (!types_compatible(init_type, var_decl->type.type)) {
                    semantic_error_list_add(errors, "Initializer type mismatch", var_decl->location);
                }
                /* Mark variable as initialized */
                symbol_table_mark_initialized(table, var_decl->name);
            }
            break;
        }
        
        case STMT_EXPR: {
            analyze_expression(stmt->as.expr_stmt.expr, table, errors);
            break;
        }
        
        case STMT_IF: {
            ASTIfStmt* if_stmt = &stmt->as.if_stmt;
            
            /* Analyze condition - must be bool type */
            CasmType cond_type = analyze_expression(if_stmt->condition, table, errors);
            if (cond_type != TYPE_BOOL) {
                semantic_error_list_add(errors, "If condition must have bool type", stmt->location);
            }
            
            /* Analyze then block */
            analyze_block(&if_stmt->then_body, table, return_type, errors);
            
            /* Analyze else-if chain */
            for (ASTElseIfClause* elif = if_stmt->else_if_chain; elif; elif = elif->next) {
                CasmType elif_type = analyze_expression(elif->condition, table, errors);
                if (elif_type != TYPE_BOOL) {
                    semantic_error_list_add(errors, "Else-if condition must have bool type", elif->condition->location);
                }
                analyze_block(&elif->body, table, return_type, errors);
            }
            
            /* Analyze else block if present */
            if (if_stmt->else_body) {
                analyze_block(if_stmt->else_body, table, return_type, errors);
            }
            break;
        }
        
        case STMT_WHILE: {
            ASTWhileStmt* while_stmt = &stmt->as.while_stmt;
            
            /* Analyze condition - must be bool type */
            CasmType cond_type = analyze_expression(while_stmt->condition, table, errors);
            if (cond_type != TYPE_BOOL) {
                semantic_error_list_add(errors, "While condition must have bool type", stmt->location);
            }
            
            /* Analyze body block */
            analyze_block(&while_stmt->body, table, return_type, errors);
            break;
        }
        
        case STMT_FOR: {
            ASTForStmt* for_stmt = &stmt->as.for_stmt;
            
            /* Create new scope for loop variable */
            symbol_table_push_scope(table);
            
            /* Analyze init statement if present */
            if (for_stmt->init) {
                analyze_statement(for_stmt->init, table, return_type, errors);
            }
            
            /* Analyze condition - must be bool type if present */
            if (for_stmt->condition) {
                CasmType cond_type = analyze_expression(for_stmt->condition, table, errors);
                if (cond_type != TYPE_BOOL) {
                    semantic_error_list_add(errors, "For loop condition must have bool type", for_stmt->condition->location);
                }
            }
            
            /* Analyze update expression if present */
            if (for_stmt->update) {
                analyze_expression(for_stmt->update, table, errors);
            }
            
            /* Analyze body block */
            analyze_block(&for_stmt->body, table, return_type, errors);
            
            /* Pop scope */
            symbol_table_pop_scope(table);
            break;
        }
        
        case STMT_BLOCK: {
            /* Analyze nested block statements */
            analyze_block(&stmt->as.block_stmt.block, table, return_type, errors);
            break;
        }
    }
}

/* Analyze a block of statements */
static void analyze_block(ASTBlock* block, SymbolTable* table, CasmType return_type, SemanticErrorList* errors) {
    if (!block) return;
    
    /* Push scope for the block */
    symbol_table_push_scope(table);
    
    for (int i = 0; i < block->statement_count; i++) {
        analyze_statement(&block->statements[i], table, return_type, errors);
    }
    
    /* Pop scope */
    symbol_table_pop_scope(table);
}

/* Pass 1: Collect all function definitions */
static void collect_functions(ASTProgram* program, SymbolTable* table, SemanticErrorList* errors) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        /* Convert parameter types */
        CasmType* param_types = NULL;
        if (func->parameter_count > 0) {
            param_types = xmalloc(func->parameter_count * sizeof(CasmType));
            for (int j = 0; j < func->parameter_count; j++) {
                param_types[j] = func->parameters[j].type.type;
            }
        }
        
        /* Add function to symbol table */
        if (!symbol_table_add_function(table, func->name, func->return_type.type, 
                                       param_types, func->parameter_count, func->location)) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Function '%s' already defined", func->name);
            semantic_error_list_add(errors, msg, func->location);
        }
        
        if (param_types) {
            xfree(param_types);
        }
    }
}

/* Pass 2: Validate function bodies */
static void validate_functions(ASTProgram* program, SymbolTable* table, SemanticErrorList* errors) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        /* Push scope for function parameters */
        symbol_table_push_scope(table);
        
        /* Add parameters to scope */
        for (int j = 0; j < func->parameter_count; j++) {
            symbol_table_add_variable(table, func->parameters[j].name, 
                                      func->parameters[j].type.type, 
                                      func->parameters[j].location);
            /* Function parameters are initialized by the call */
            symbol_table_mark_initialized(table, func->parameters[j].name);
        }
        
        /* Analyze function body */
        analyze_block(&func->body, table, func->return_type.type, errors);
        
        /* Pop function scope */
        symbol_table_pop_scope(table);
    }
}

/* Main semantic analysis - 2-pass */
int analyze_program(ASTProgram* program, SymbolTable* table, SemanticErrorList* errors) {
    /* Pass 1: Collect all function definitions */
    collect_functions(program, table, errors);
    
    if (errors->error_count > 0) {
        return 0;  /* Stop if there are errors */
    }
    
    /* Pass 2: Validate function bodies and expressions */
    validate_functions(program, table, errors);
    
    return errors->error_count == 0;
}
