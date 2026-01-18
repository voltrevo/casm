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
            fprintf(out, "%s(", expr->as.function_call.function_name);
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
            for (int i = 0; i < dbg->argument_count; i++) {
                print_indent(out, indent);
                fprintf(out, "_casm_dbg_");
                
                /* Use the type of the expression to determine the format specifier */
                CasmType arg_type = dbg->arguments[i].resolved_type;
                switch (arg_type) {
                    case TYPE_I8:
                    case TYPE_I16:
                    case TYPE_I32: fprintf(out, "i32(\""); break;
                    case TYPE_I64: fprintf(out, "i64(\""); break;
                    case TYPE_U8:
                    case TYPE_U16:
                    case TYPE_U32: fprintf(out, "u32(\""); break;
                    case TYPE_U64: fprintf(out, "u64(\""); break;
                    case TYPE_BOOL: fprintf(out, "bool(\""); break;
                    default: fprintf(out, "i32(\""); break;
                }
                
                /* Emit location info: file:line:col: argname = */
                fprintf(out, "%s:%d:%d: ", g_source_filename, dbg->location.line, dbg->location.column);
                if (dbg->arg_names[i] && strlen(dbg->arg_names[i]) > 0) {
                    fprintf(out, "%s = ", dbg->arg_names[i]);
                } else {
                    /* Fallback: just use generic label */
                    fprintf(out, "arg%d = ", i);
                }
                fprintf(out, "\", ");
                
                emit_expression(out, &dbg->arguments[i]);
                fprintf(out, ");\n");
            }
            break;
        }
    }
}

/* Emit function forward declarations */
static void emit_function_declarations(FILE* out, ASTProgram* program) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        fprintf(out, "%s %s(",
                casm_type_to_c_type(func->return_type.type),
                func->name);
        
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
    }
    fprintf(out, "\n");
}

/* Emit function definitions */
static void emit_function_definitions(FILE* out, ASTProgram* program) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        fprintf(out, "%s %s(",
                casm_type_to_c_type(func->return_type.type),
                func->name);
        
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
    
    /* Emit debug helper functions */
    fprintf(output, "/* Debug helper functions */\n");
    fprintf(output, "static void __attribute__((unused)) _casm_dbg_i32(const char* label, int32_t value) {\n");
    fprintf(output, "    printf(\"%%s%%d\\n\", label, value);\n");
    fprintf(output, "}\n");
    fprintf(output, "static void __attribute__((unused)) _casm_dbg_i64(const char* label, int64_t value) {\n");
    fprintf(output, "    printf(\"%%s%%lld\\n\", label, (long long)value);\n");
    fprintf(output, "}\n");
    fprintf(output, "static void __attribute__((unused)) _casm_dbg_u32(const char* label, uint32_t value) {\n");
    fprintf(output, "    printf(\"%%s%%u\\n\", label, value);\n");
    fprintf(output, "}\n");
    fprintf(output, "static void __attribute__((unused)) _casm_dbg_u64(const char* label, uint64_t value) {\n");
    fprintf(output, "    printf(\"%%s%%llu\\n\", label, (unsigned long long)value);\n");
    fprintf(output, "}\n");
    fprintf(output, "static void __attribute__((unused)) _casm_dbg_bool(const char* label, _Bool value) {\n");
    fprintf(output, "    printf(\"%%s%%s\\n\", label, value ? \"true\" : \"false\");\n");
    fprintf(output, "}\n");
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
