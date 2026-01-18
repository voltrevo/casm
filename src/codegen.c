#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "utils.h"

/* Global variable to track source filename for dbg output */
static const char* g_source_filename = "unknown.csm";

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
            char* mangled_name = mangle_function_name(expr->as.function_call.function_name);
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
            
            /* First, check if any arguments are function calls */
            /* If so, we need to store their values in temp variables */
            for (int i = 0; i < dbg->argument_count; i++) {
                if (is_function_call(&dbg->arguments[i])) {
                    /* Emit temporary variable assignment for this function call */
                    print_indent(out, indent);
                    fprintf(out, "%s __dbg_tmp_%d = ",
                            casm_type_to_c_type(dbg->arguments[i].resolved_type), i);
                    emit_expression(out, &dbg->arguments[i]);
                    fprintf(out, ";\n");
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
                    fprintf(out, "%s = ", dbg->arg_names[i]);
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
                if (is_function_call(&dbg->arguments[i])) {
                    if (arg_type == TYPE_BOOL) {
                        fprintf(out, "__dbg_tmp_%d ? \"true\" : \"false\"", i);
                    } else if (arg_type == TYPE_I64 || arg_type == TYPE_U64) {
                        fprintf(out, "(long long)__dbg_tmp_%d", i);
                    } else if (arg_type == TYPE_U32 || arg_type == TYPE_U8 || arg_type == TYPE_U16) {
                        fprintf(out, "(unsigned int)__dbg_tmp_%d", i);
                    } else {
                        fprintf(out, "__dbg_tmp_%d", i);
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
            break;
        }
    }
}

/* Emit function forward declarations */
static void emit_function_declarations(FILE* out, ASTProgram* program) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        char* mangled_name = mangle_function_name(func->name);
        
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
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        char* mangled_name = mangle_function_name(func->name);
        
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
        
        if (i < program->function_count - 1) {
            fprintf(out, "\n");
        }
        
        xfree(mangled_name);
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
    
    /* Store filename for use in dbg output */
    g_source_filename = source_filename ? source_filename : "unknown.csm";
    
    /* Emit includes */
    fprintf(output, "#include <stdint.h>\n");
    fprintf(output, "#include <stdbool.h>\n");
    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "\n");
    
    /* Emit function declarations */
    emit_function_declarations(output, program);
    
    /* Emit function definitions */
    emit_function_definitions(output, program);
    
    CodegenResult result;
    result.success = 1;
    result.error_msg = NULL;
    return result;
}
