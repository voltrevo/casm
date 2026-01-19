#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "utils.h"

/* Global variable to track source filename for dbg output */
static const char* g_source_filename = "unknown.csm";

/* Global counter for unique dbg temporary variables */
static int g_dbg_tmp_counter = 0;

/* Global reference to the program being compiled (for function call resolution) */
static ASTProgram* g_current_program = NULL;

/* Global reference to the current function being compiled (for module context in calls) */
static ASTFunctionDef* g_current_function = NULL;

/* Helper: Map CASM type to C type string */
static const char* casm_type_to_c_type(CasmType type) {
    switch (type) {
        case TYPE_I8:    return "int8_t";
        case TYPE_I16:   return "int16_t";
        case TYPE_I32:   return "int32_t";
        case TYPE_I64:   return "int64_t";
        case TYPE_U8:    return "uint8_t";
        case TYPE_U16:   return "uint16_t";
        case TYPE_U32:   return "uint32_t";
        case TYPE_U64:   return "uint64_t";
        case TYPE_BOOL:  return "_Bool";
        case TYPE_VOID:  return "void";
        default:         return "void";
    }
}

/* Helper: Convert qualified name to mangled name (module:name -> module_name) */
static char* mangle_function_name(const char* qualified_name) {
    char* mangled = xstrdup(qualified_name);
    for (int i = 0; mangled[i]; i++) {
        if (mangled[i] == ':') {
            mangled[i] = '_';
        }
    }
    return mangled;
}

/* Helper: Look up the allocated name for a function call */
static const char* get_call_target_name(const char* call_name) {
    if (!g_current_program || !call_name) {
        return call_name;
    }
    
    /* If we have module context, prefer functions from the same module */
    if (g_current_function && g_current_function->module_path) {
        for (int i = 0; i < g_current_program->function_count; i++) {
            ASTFunctionDef* func = &g_current_program->functions[i];
            if (func->allocated_name && 
                strcmp(func->name, call_name) == 0 &&
                strcmp(func->module_path, g_current_function->module_path) == 0) {
                return func->allocated_name;
            }
        }
    }
    
    /* Fallback: Try to find any function with this name that has an allocated_name */
    for (int i = 0; i < g_current_program->function_count; i++) {
        ASTFunctionDef* func = &g_current_program->functions[i];
        if (func->allocated_name && strcmp(func->name, call_name) == 0) {
            return func->allocated_name;
        }
    }
    
    /* Not found - use the original name */
    return call_name;
}

/* Helper: Check if expression is a function call */
static int is_function_call(ASTExpression* expr) {
    return expr && expr->type == EXPR_FUNCTION_CALL;
}

/* Helper: Emit expression to file */
static void emit_expression(FILE* out, ASTExpression* expr);

/* Helper: Emit expression that may need parentheses if it contains assignment */
static void emit_expression_in_context(FILE* out, ASTExpression* expr, int needs_parens_for_assign) {
    if (!expr) return;
    
    /* If this is an assignment and we're in a context that needs parentheses, wrap it */
    if (needs_parens_for_assign && expr->type == EXPR_BINARY_OP && expr->as.binary_op.op == BINOP_ASSIGN) {
        fprintf(out, "(");
        emit_expression(out, expr);
        fprintf(out, ")");
    } else {
        emit_expression(out, expr);
    }
}

/* Helper: Print indent */
static void print_indent(FILE* out, int indent) {
    for (int i = 0; i < indent; i++) {
        fprintf(out, "    ");
    }
}

/* Helper: Emit binary operator */
static const char* binop_to_string(BinaryOpType op) {
    switch (op) {
        case BINOP_ADD: return "+";
        case BINOP_SUB: return "-";
        case BINOP_MUL: return "*";
        case BINOP_DIV: return "/";
        case BINOP_MOD: return "%";
        case BINOP_EQ:  return "==";
        case BINOP_NE:  return "!=";
        case BINOP_LT:  return "<";
        case BINOP_GT:  return ">";
        case BINOP_LE:  return "<=";
        case BINOP_GE:  return ">=";
        case BINOP_AND: return "&&";
        case BINOP_OR:  return "||";
        case BINOP_ASSIGN: return "=";
        default:        return "?";
    }
}

/* Helper: Emit unary operator */
static const char* unop_to_string(UnaryOpType op) {
    switch (op) {
        case UNOP_NEG: return "-";
        case UNOP_NOT: return "!";
        default:       return "?";
    }
}

/* Emit a single expression */
static void emit_expression(FILE* out, ASTExpression* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_LITERAL:
            if (expr->as.literal.type == LITERAL_INT) {
                fprintf(out, "%ld", expr->as.literal.value.int_value);
            } else {
                fprintf(out, "%s", expr->as.literal.value.bool_value ? "true" : "false");
            }
            break;
            
        case EXPR_VARIABLE:
            fprintf(out, "%s", expr->as.variable.name);
            break;
            
        case EXPR_BINARY_OP: {
            /* Assignment doesn't need parentheses and has different spacing */
            if (expr->as.binary_op.op == BINOP_ASSIGN) {
                emit_expression(out, expr->as.binary_op.left);
                fprintf(out, " = ");
                emit_expression(out, expr->as.binary_op.right);
            } else {
                fprintf(out, "(");
                /* Parenthesize assignment sub-expressions to preserve precedence */
                emit_expression_in_context(out, expr->as.binary_op.left, 1);
                fprintf(out, " %s ", binop_to_string(expr->as.binary_op.op));
                emit_expression_in_context(out, expr->as.binary_op.right, 1);
                fprintf(out, ")");
            }
            break;
        }
        
        case EXPR_UNARY_OP: {
            fprintf(out, "(%s", unop_to_string(expr->as.unary_op.op));
            /* Parenthesize assignment sub-expressions */
            emit_expression_in_context(out, expr->as.unary_op.operand, 1);
            fprintf(out, ")");
            break;
        }
        
        case EXPR_FUNCTION_CALL: {
            /* Look up the actual function name (handles allocated names with mangling) */
            const char* call_target = get_call_target_name(expr->as.function_call.function_name);
            char* mangled_name = mangle_function_name(call_target);
            fprintf(out, "%s(", mangled_name);
            xfree(mangled_name);
            for (int i = 0; i < expr->as.function_call.argument_count; i++) {
                if (i > 0) fprintf(out, ", ");
                /* Parenthesize assignment sub-expressions in function arguments */
                emit_expression_in_context(out, &expr->as.function_call.arguments[i], 1);
            }
            fprintf(out, ")");
            break;
        }
    }
}

/* Forward declaration for emit_statement */
static void emit_statement(FILE* out, ASTStatement* stmt, int indent);

/* Emit a block of statements */
static void emit_block(FILE* out, ASTBlock* block, int indent) {
    for (int i = 0; i < block->statement_count; i++) {
        emit_statement(out, &block->statements[i], indent);
    }
}

/* Emit a statement */
static void emit_statement(FILE* out, ASTStatement* stmt, int indent) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_VAR_DECL: {
            ASTVarDecl* var = &stmt->as.var_decl_stmt.var_decl;
            print_indent(out, indent);
            fprintf(out, "%s %s", casm_type_to_c_type(var->type.type), var->name);
            if (var->initializer) {
                fprintf(out, " = ");
                emit_expression(out, var->initializer);
            }
            fprintf(out, ";\n");
            break;
        }
        
        case STMT_EXPR: {
            print_indent(out, indent);
            emit_expression(out, stmt->as.expr_stmt.expr);
            fprintf(out, ";\n");
            break;
        }
        
        case STMT_RETURN: {
            print_indent(out, indent);
            fprintf(out, "return");
            if (stmt->as.return_stmt.value) {
                fprintf(out, " ");
                emit_expression(out, stmt->as.return_stmt.value);
            }
            fprintf(out, ";\n");
            break;
        }
        
        case STMT_IF: {
            ASTIfStmt* if_stmt = &stmt->as.if_stmt;
            
            print_indent(out, indent);
            fprintf(out, "if (");
            emit_expression(out, if_stmt->condition);
            fprintf(out, ") {\n");
            emit_block(out, &if_stmt->then_body, indent + 1);
            print_indent(out, indent);
            fprintf(out, "}");
            
            /* Emit else-if chain */
            for (ASTElseIfClause* elif = if_stmt->else_if_chain; elif; elif = elif->next) {
                fprintf(out, " else if (");
                emit_expression(out, elif->condition);
                fprintf(out, ") {\n");
                emit_block(out, &elif->body, indent + 1);
                print_indent(out, indent);
                fprintf(out, "}");
            }
            
            /* Emit else block if present */
            if (if_stmt->else_body) {
                fprintf(out, " else {\n");
                emit_block(out, if_stmt->else_body, indent + 1);
                print_indent(out, indent);
                fprintf(out, "}\n");
            } else {
                fprintf(out, "\n");
            }
            break;
        }
        
        case STMT_WHILE: {
            ASTWhileStmt* while_stmt = &stmt->as.while_stmt;
            
            print_indent(out, indent);
            fprintf(out, "while (");
            emit_expression(out, while_stmt->condition);
            fprintf(out, ") {\n");
            emit_block(out, &while_stmt->body, indent + 1);
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        
        case STMT_FOR: {
            ASTForStmt* for_stmt = &stmt->as.for_stmt;
            
            print_indent(out, indent);
            fprintf(out, "for (");
            
            /* Emit init */
            if (for_stmt->init) {
                if (for_stmt->init->type == STMT_VAR_DECL) {
                    /* Variable declaration in for init */
                    ASTVarDecl* var = &for_stmt->init->as.var_decl_stmt.var_decl;
                    fprintf(out, "%s %s", casm_type_to_c_type(var->type.type), var->name);
                    if (var->initializer) {
                        fprintf(out, " = ");
                        emit_expression(out, var->initializer);
                    }
                } else if (for_stmt->init->type == STMT_EXPR) {
                    /* Expression statement in for init */
                    emit_expression(out, for_stmt->init->as.expr_stmt.expr);
                }
            }
            fprintf(out, "; ");
            
            /* Emit condition */
            if (for_stmt->condition) {
                emit_expression(out, for_stmt->condition);
            }
            fprintf(out, "; ");
            
            /* Emit update */
            if (for_stmt->update) {
                emit_expression(out, for_stmt->update);
            }
            fprintf(out, ") {\n");
            
            emit_block(out, &for_stmt->body, indent + 1);
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        
        case STMT_BLOCK: {
            /* Emit nested block with braces to preserve scoping */
            print_indent(out, indent);
            fprintf(out, "{\n");
            emit_block(out, &stmt->as.block_stmt.block, indent + 1);
            print_indent(out, indent);
            fprintf(out, "}\n");
            break;
        }
        
        case STMT_DBG: {
            ASTDbgStmt* dbg = &stmt->as.dbg_stmt;
            
            /* Store temporary variable names for each argument */
            char** tmp_names = xmalloc(dbg->argument_count * sizeof(char*));
            
            /* First, check if any arguments are function calls */
            /* If so, we need to store their values in temp variables */
            for (int i = 0; i < dbg->argument_count; i++) {
                if (is_function_call(&dbg->arguments[i])) {
                    /* Create unique temporary variable name */
                    char tmp_name_buf[64];
                    snprintf(tmp_name_buf, sizeof(tmp_name_buf), "__dbg_tmp_%d", g_dbg_tmp_counter++);
                    tmp_names[i] = xstrdup(tmp_name_buf);
                    
                    /* Emit temporary variable assignment for this function call */
                    print_indent(out, indent);
                    fprintf(out, "%s %s = ",
                            casm_type_to_c_type(dbg->arguments[i].resolved_type),
                            tmp_names[i]);
                    emit_expression(out, &dbg->arguments[i]);
                    fprintf(out, ";\n");
                } else {
                    tmp_names[i] = NULL;
                }
            }
            
            print_indent(out, indent);
            fprintf(out, "printf(\"");
            
            /* Location info only once */
            fprintf(out, "%s:%d:%d: ", g_source_filename, dbg->location.line, dbg->location.column);
            
             /* Build the format string with all arguments */
             for (int i = 0; i < dbg->argument_count; i++) {
                 if (i > 0) fprintf(out, ", ");  /* Comma between arguments */
                 
                 if (dbg->arg_names[i] && strlen(dbg->arg_names[i]) > 0) {
                     /* Escape % characters in arg_names for the C printf format string
                        We need double-escaping because fprintf will turn %% into %,
                        and then the C compiler will see that % in the source code */
                     const char* name = dbg->arg_names[i];
                     for (int j = 0; name[j]; j++) {
                         if (name[j] == '%') {
                             fprintf(out, "%%%%");  /* %%%% -> %% (in source) -> % (at runtime) */
                         } else {
                             fprintf(out, "%c", name[j]);
                         }
                     }
                     fprintf(out, " = ");
                 } else {
                     fprintf(out, "arg%d = ", i);
                 }
                 
                 /* Add format specifier based on type */
                 CasmType arg_type = dbg->arguments[i].resolved_type;
                 switch (arg_type) {
                     case TYPE_I8:
                     case TYPE_I16:
                     case TYPE_I32: fprintf(out, "%%d"); break;
                     case TYPE_I64: fprintf(out, "%%lld"); break;
                     case TYPE_U8:
                     case TYPE_U16:
                     case TYPE_U32: fprintf(out, "%%u"); break;
                     case TYPE_U64: fprintf(out, "%%llu"); break;
                     case TYPE_BOOL: fprintf(out, "%%s"); break;
                     default: fprintf(out, "%%d"); break;
                 }
             }
            fprintf(out, "\\n\"");
            
            /* Add arguments to printf */
            for (int i = 0; i < dbg->argument_count; i++) {
                fprintf(out, ", ");
                CasmType arg_type = dbg->arguments[i].resolved_type;
                
                /* Use temp variable if this was a function call, otherwise emit expression */
                if (tmp_names[i] != NULL) {
                    if (arg_type == TYPE_BOOL) {
                        fprintf(out, "%s ? \"true\" : \"false\"", tmp_names[i]);
                    } else if (arg_type == TYPE_I64 || arg_type == TYPE_U64) {
                        fprintf(out, "(long long)%s", tmp_names[i]);
                    } else if (arg_type == TYPE_U32 || arg_type == TYPE_U8 || arg_type == TYPE_U16) {
                        fprintf(out, "(unsigned int)%s", tmp_names[i]);
                    } else {
                        fprintf(out, "%s", tmp_names[i]);
                    }
                } else {
                    /* For non-function-call expressions, emit them directly as before */
                    if (arg_type == TYPE_BOOL) {
                        fprintf(out, "(");
                        emit_expression(out, &dbg->arguments[i]);
                        fprintf(out, ") ? \"true\" : \"false\"");
                    } else if (arg_type == TYPE_I64 || arg_type == TYPE_U64) {
                        fprintf(out, "(long long)(");
                        emit_expression(out, &dbg->arguments[i]);
                        fprintf(out, ")");
                    } else if (arg_type == TYPE_U32 || arg_type == TYPE_U8 || arg_type == TYPE_U16) {
                        fprintf(out, "(unsigned int)(");
                        emit_expression(out, &dbg->arguments[i]);
                        fprintf(out, ")");
                    } else {
                        emit_expression(out, &dbg->arguments[i]);
                    }
                }
            }
            fprintf(out, ");\n");
            
            /* Free temporary names */
            for (int i = 0; i < dbg->argument_count; i++) {
                if (tmp_names[i] != NULL) {
                    xfree(tmp_names[i]);
                }
            }
            xfree(tmp_names);
            break;
        }
    }
}

/* Emit function forward declarations */
static void emit_function_declarations(FILE* out, ASTProgram* program) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        if (program->import_count > 0 && !func->allocated_name) {
            continue;
        }
        /* Use allocated name if available (for dead code elimination in multi-module),
         * otherwise use the function's original name (for single-file programs) */
        const char* func_name = func->allocated_name ? func->allocated_name : func->name;
        char* mangled_name = mangle_function_name(func_name);
        
        fprintf(out, "%s %s(",
                casm_type_to_c_type(func->return_type.type),
                mangled_name);
        
        if (func->parameter_count == 0) {
            fprintf(out, "void");
        } else {
            for (int j = 0; j < func->parameter_count; j++) {
                if (j > 0) fprintf(out, ", ");
                fprintf(out, "%s %s",
                        casm_type_to_c_type(func->parameters[j].type.type),
                        func->parameters[j].name);
            }
        }
        
        fprintf(out, ");\n");
        xfree(mangled_name);
    }
    fprintf(out, "\n");
}

/* Emit function definitions */
static void emit_function_definitions(FILE* out, ASTProgram* program) {
    int emit_total = 0;
    for (int i = 0; i < program->function_count; i++) {
        if (program->import_count > 0 && !program->functions[i].allocated_name) {
            continue;
        }
        emit_total++;
    }

    int emit_count = 0;
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        if (program->import_count > 0 && !func->allocated_name) {
            continue;
        }
         
         /* Use allocated name if available (for dead code elimination in multi-module),
          * otherwise use the function's original name (for single-file programs) */
         const char* func_name = func->allocated_name ? func->allocated_name : func->name;
         
         /* Set context for call resolution */
         g_current_function = func;
         
         /* Use allocated/original name for code generation */
         char* mangled_name = mangle_function_name(func_name);
        
        fprintf(out, "%s %s(",
                casm_type_to_c_type(func->return_type.type),
                mangled_name);
        
        if (func->parameter_count == 0) {
            fprintf(out, "void");
        } else {
            for (int j = 0; j < func->parameter_count; j++) {
                if (j > 0) fprintf(out, ", ");
                fprintf(out, "%s %s",
                        casm_type_to_c_type(func->parameters[j].type.type),
                        func->parameters[j].name);
            }
        }
        
        fprintf(out, ") {\n");
        emit_block(out, &func->body, 1);
        fprintf(out, "}\n");

        emit_count++;
        if (emit_count < emit_total) {
            fprintf(out, "\n");
        }
        
        xfree(mangled_name);
        
        /* Clear context */
        g_current_function = NULL;
    }
}

/* Main code generation function */
CodegenResult codegen_program(ASTProgram* program, FILE* output, const char* source_filename) {
    if (!program || !output) {
        CodegenResult result;
        result.success = 0;
        result.error_msg = "Invalid input to codegen_program";
        return result;
    }
    
    /* Store references for use in code emission */
    g_source_filename = source_filename ? source_filename : "unknown.csm";
    g_current_program = program;
    
    /* Emit includes */
    fprintf(output, "#include <stdint.h>\n");
    fprintf(output, "#include <stdbool.h>\n");
    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "\n");
    
    /* Emit function declarations */
    emit_function_declarations(output, program);
    
    /* Emit function definitions */
    emit_function_definitions(output, program);
    
    /* Clear global reference */
    g_current_program = NULL;
    
    CodegenResult result;
    result.success = 1;
    result.error_msg = NULL;
    return result;
}
