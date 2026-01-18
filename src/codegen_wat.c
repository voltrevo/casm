#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen_wat.h"
#include "utils.h"

/* Helper: Map CASM type to WAT type string */
static const char* casm_type_to_wat_type(CasmType type) {
    switch (type) {
        case TYPE_I8:
        case TYPE_I16:
        case TYPE_I32:   return "i32";
        case TYPE_I64:   return "i64";
        case TYPE_U8:
        case TYPE_U16:
        case TYPE_U32:   return "i32";  /* WAT uses signed i32/i64, unsigned are same operations */
        case TYPE_U64:   return "i64";
        case TYPE_BOOL:  return "i32";  /* bool is i32 in WAT (0 or 1) */
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

/* Helper: Print indent (2 spaces per level) */
static void print_indent(FILE* out, int indent) {
    for (int i = 0; i < indent * 2; i++) {
        fprintf(out, " ");
    }
}

/* Helper: Emit binary operator as WAT instruction */
static void emit_binop_instruction(FILE* out, BinaryOpType op, CasmType type) {
    const char* type_prefix = (type == TYPE_I64 || type == TYPE_U64) ? "i64" : "i32";
    
    switch (op) {
        case BINOP_ADD:   fprintf(out, "%s.add", type_prefix); break;
        case BINOP_SUB:   fprintf(out, "%s.sub", type_prefix); break;
        case BINOP_MUL:   fprintf(out, "%s.mul", type_prefix); break;
        case BINOP_DIV:
            if (type == TYPE_I8 || type == TYPE_I16 || type == TYPE_I32 || type == TYPE_I64) {
                fprintf(out, "%s.div_s", type_prefix);
            } else {
                fprintf(out, "%s.div_u", type_prefix);
            }
            break;
        case BINOP_MOD:
            if (type == TYPE_I8 || type == TYPE_I16 || type == TYPE_I32 || type == TYPE_I64) {
                fprintf(out, "%s.rem_s", type_prefix);
            } else {
                fprintf(out, "%s.rem_u", type_prefix);
            }
            break;
        case BINOP_EQ:    fprintf(out, "%s.eq", type_prefix); break;
        case BINOP_NE:    fprintf(out, "%s.ne", type_prefix); break;
        case BINOP_LT:
            if (type == TYPE_I8 || type == TYPE_I16 || type == TYPE_I32 || type == TYPE_I64) {
                fprintf(out, "%s.lt_s", type_prefix);
            } else {
                fprintf(out, "%s.lt_u", type_prefix);
            }
            break;
        case BINOP_GT:
            if (type == TYPE_I8 || type == TYPE_I16 || type == TYPE_I32 || type == TYPE_I64) {
                fprintf(out, "%s.gt_s", type_prefix);
            } else {
                fprintf(out, "%s.gt_u", type_prefix);
            }
            break;
        case BINOP_LE:
            if (type == TYPE_I8 || type == TYPE_I16 || type == TYPE_I32 || type == TYPE_I64) {
                fprintf(out, "%s.le_s", type_prefix);
            } else {
                fprintf(out, "%s.le_u", type_prefix);
            }
            break;
        case BINOP_GE:
            if (type == TYPE_I8 || type == TYPE_I16 || type == TYPE_I32 || type == TYPE_I64) {
                fprintf(out, "%s.ge_s", type_prefix);
            } else {
                fprintf(out, "%s.ge_u", type_prefix);
            }
            break;
        case BINOP_AND:   fprintf(out, "i32.and"); break;
        case BINOP_OR:    fprintf(out, "i32.or"); break;
        case BINOP_ASSIGN: break;  /* Should not reach here */
    }
}

/* Forward declarations */
static void emit_expression(FILE* out, ASTExpression* expr, int indent);
static void emit_statement(FILE* out, ASTStatement* stmt, int indent);

/* Emit expression to stack - this should always result in value(s) on stack */
static void emit_expression(FILE* out, ASTExpression* expr, int indent) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_LITERAL:
            print_indent(out, indent);
            if (expr->as.literal.type == LITERAL_INT) {
                fprintf(out, "i32.const %ld", expr->as.literal.value.int_value);
            } else {
                fprintf(out, "i32.const %d", expr->as.literal.value.bool_value ? 1 : 0);
            }
            break;
            
        case EXPR_VARIABLE:
            print_indent(out, indent);
            fprintf(out, "local.get $%s", expr->as.variable.name);
            break;
            
        case EXPR_BINARY_OP: {
            ASTBinaryOp* binop = &expr->as.binary_op;
            
            if (binop->op == BINOP_ASSIGN) {
                /* Assignment: evaluate RHS, store to LHS */
                emit_expression(out, binop->right, indent);
                fprintf(out, "\n");
                print_indent(out, indent);
                fprintf(out, "local.set $%s", binop->left->as.variable.name);
            } else {
                /* Regular binary operation */
                emit_expression(out, binop->left, indent);
                fprintf(out, "\n");
                emit_expression(out, binop->right, indent);
                fprintf(out, "\n");
                print_indent(out, indent);
                emit_binop_instruction(out, binop->op, TYPE_I32);
            }
            break;
        }
        
        case EXPR_UNARY_OP: {
            ASTUnaryOp* unop = &expr->as.unary_op;
            
            emit_expression(out, unop->operand, indent);
            fprintf(out, "\n");
            
            if (unop->op == UNOP_NEG) {
                print_indent(out, indent);
                fprintf(out, "i32.const 0\n");
                print_indent(out, indent);
                fprintf(out, "i32.sub");
            } else if (unop->op == UNOP_NOT) {
                print_indent(out, indent);
                fprintf(out, "i32.eqz");
            }
            break;
        }
        
        case EXPR_FUNCTION_CALL: {
            ASTFunctionCall* call = &expr->as.function_call;
            
            /* Emit arguments in order */
            for (int i = 0; i < call->argument_count; i++) {
                if (i > 0) fprintf(out, "\n");
                emit_expression(out, &call->arguments[i], indent);
            }
            if (call->argument_count > 0) {
                fprintf(out, "\n");
            }
            print_indent(out, indent);
            char* mangled_name = mangle_function_name(call->function_name);
            fprintf(out, "call $%s", mangled_name);
            xfree(mangled_name);
            break;
        }
    }
}

/* Forward declaration for collecting locals */
static void collect_locals(ASTBlock* block, char** local_names, int* local_count);

/* Helper to collect local variables from a block */
static void collect_locals_from_stmt(ASTStatement* stmt, char** local_names, int* local_count) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_VAR_DECL: {
            ASTVarDecl* var = &stmt->as.var_decl_stmt.var_decl;
            /* Check if already added */
            int found = 0;
            for (int i = 0; i < *local_count; i++) {
                if (strcmp(local_names[i], var->name) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found && *local_count < 100) {  /* Reasonable limit */
                local_names[*local_count] = var->name;
                (*local_count)++;
            }
            break;
        }
        
        case STMT_IF: {
            ASTIfStmt* if_stmt = &stmt->as.if_stmt;
            collect_locals(&if_stmt->then_body, local_names, local_count);
            for (ASTElseIfClause* elif = if_stmt->else_if_chain; elif; elif = elif->next) {
                collect_locals(&elif->body, local_names, local_count);
            }
            if (if_stmt->else_body) {
                collect_locals(if_stmt->else_body, local_names, local_count);
            }
            break;
        }
        
        case STMT_WHILE: {
            ASTWhileStmt* while_stmt = &stmt->as.while_stmt;
            collect_locals(&while_stmt->body, local_names, local_count);
            break;
        }
        
        case STMT_FOR: {
            ASTForStmt* for_stmt = &stmt->as.for_stmt;
            if (for_stmt->init && for_stmt->init->type == STMT_VAR_DECL) {
                ASTVarDecl* var = &for_stmt->init->as.var_decl_stmt.var_decl;
                int found = 0;
                for (int i = 0; i < *local_count; i++) {
                    if (strcmp(local_names[i], var->name) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found && *local_count < 100) {
                    local_names[*local_count] = var->name;
                    (*local_count)++;
                }
            }
            collect_locals(&for_stmt->body, local_names, local_count);
            break;
        }
        
        case STMT_BLOCK: {
            collect_locals(&stmt->as.block_stmt.block, local_names, local_count);
            break;
        }
        
        default:
            break;
    }
}

/* Collect locals from a block */
static void collect_locals(ASTBlock* block, char** local_names, int* local_count) {
    for (int i = 0; i < block->statement_count; i++) {
        collect_locals_from_stmt(&block->statements[i], local_names, local_count);
    }
}

/* Emit a statement */
static void emit_statement(FILE* out, ASTStatement* stmt, int indent) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_VAR_DECL: {
            ASTVarDecl* var = &stmt->as.var_decl_stmt.var_decl;
            /* Local declarations are handled in function header */
            /* But if there's an initializer, emit the assignment */
            if (var->initializer) {
                emit_expression(out, var->initializer, indent);
                fprintf(out, "\n");
                print_indent(out, indent);
                fprintf(out, "local.set $%s\n", var->name);
            }
            break;
        }
        
        case STMT_EXPR: {
            emit_expression(out, stmt->as.expr_stmt.expr, indent);
            fprintf(out, "\n");
            break;
        }
        
        case STMT_RETURN: {
            if (stmt->as.return_stmt.value) {
                emit_expression(out, stmt->as.return_stmt.value, indent);
                fprintf(out, "\n");
            }
            print_indent(out, indent);
            fprintf(out, "return\n");
            break;
        }
        
        case STMT_IF: {
            ASTIfStmt* if_stmt = &stmt->as.if_stmt;
            
            emit_expression(out, if_stmt->condition, indent);
            fprintf(out, "\n");
            print_indent(out, indent);
            fprintf(out, "if\n");
            
            /* Then body */
            for (int i = 0; i < if_stmt->then_body.statement_count; i++) {
                emit_statement(out, &if_stmt->then_body.statements[i], indent + 1);
            }
            
            /* Else-if chain */
            for (ASTElseIfClause* elif = if_stmt->else_if_chain; elif; elif = elif->next) {
                print_indent(out, indent);
                fprintf(out, "else\n");
                emit_expression(out, elif->condition, indent + 1);
                fprintf(out, "\n");
                print_indent(out, indent + 1);
                fprintf(out, "if\n");
                
                for (int i = 0; i < elif->body.statement_count; i++) {
                    emit_statement(out, &elif->body.statements[i], indent + 2);
                }
            }
            
            /* Else block */
            if (if_stmt->else_body) {
                print_indent(out, indent);
                fprintf(out, "else\n");
                
                for (int i = 0; i < if_stmt->else_body->statement_count; i++) {
                    emit_statement(out, &if_stmt->else_body->statements[i], indent + 1);
                }
            }
            
            print_indent(out, indent);
            fprintf(out, "end\n");
            break;
        }
        
        case STMT_WHILE: {
            ASTWhileStmt* while_stmt = &stmt->as.while_stmt;
            
            print_indent(out, indent);
            fprintf(out, "block $break\n");
            print_indent(out, indent);
            fprintf(out, "loop $continue\n");
            
            /* Condition check */
            emit_expression(out, while_stmt->condition, indent + 1);
            fprintf(out, "\n");
            print_indent(out, indent + 1);
            fprintf(out, "i32.eqz\n");
            print_indent(out, indent + 1);
            fprintf(out, "br_if $break\n");
            
            /* Body */
            for (int i = 0; i < while_stmt->body.statement_count; i++) {
                emit_statement(out, &while_stmt->body.statements[i], indent + 1);
            }
            
            /* Jump back to loop */
            print_indent(out, indent + 1);
            fprintf(out, "br $continue\n");
            
            print_indent(out, indent);
            fprintf(out, "end\n");
            print_indent(out, indent);
            fprintf(out, "end\n");
            break;
        }
        
        case STMT_FOR: {
            ASTForStmt* for_stmt = &stmt->as.for_stmt;
            
            /* Emit init */
            if (for_stmt->init) {
                if (for_stmt->init->type == STMT_VAR_DECL) {
                    ASTVarDecl* var = &for_stmt->init->as.var_decl_stmt.var_decl;
                    if (var->initializer) {
                        emit_expression(out, var->initializer, indent);
                        fprintf(out, "\n");
                        print_indent(out, indent);
                        fprintf(out, "local.set $%s\n", var->name);
                    }
                } else if (for_stmt->init->type == STMT_EXPR) {
                    emit_expression(out, for_stmt->init->as.expr_stmt.expr, indent);
                    fprintf(out, "\n");
                }
            }
            
            print_indent(out, indent);
            fprintf(out, "block $break\n");
            print_indent(out, indent);
            fprintf(out, "loop $continue\n");
            
            /* Condition check */
            if (for_stmt->condition) {
                emit_expression(out, for_stmt->condition, indent + 1);
                fprintf(out, "\n");
                print_indent(out, indent + 1);
                fprintf(out, "i32.eqz\n");
                print_indent(out, indent + 1);
                fprintf(out, "br_if $break\n");
            }
            
            /* Body */
            for (int i = 0; i < for_stmt->body.statement_count; i++) {
                emit_statement(out, &for_stmt->body.statements[i], indent + 1);
            }
            
            /* Update */
            if (for_stmt->update) {
                emit_expression(out, for_stmt->update, indent + 1);
                fprintf(out, "\n");
            }
            
            /* Jump back to loop */
            print_indent(out, indent + 1);
            fprintf(out, "br $continue\n");
            
            print_indent(out, indent);
            fprintf(out, "end\n");
            print_indent(out, indent);
            fprintf(out, "end\n");
            break;
        }
        
        case STMT_BLOCK: {
            for (int i = 0; i < stmt->as.block_stmt.block.statement_count; i++) {
                emit_statement(out, &stmt->as.block_stmt.block.statements[i], indent);
            }
            break;
        }
        
        case STMT_DBG: {
            /* WAT dbg support - emit debug calls */
            ASTDbgStmt* dbg = &stmt->as.dbg_stmt;
            for (int i = 0; i < dbg->argument_count; i++) {
                /* Push label ID (we use the argument index) */
                print_indent(out, indent);
                fprintf(out, "i32.const %d\n", i);
                
                /* Emit the expression value */
                emit_expression(out, &dbg->arguments[i], indent);
                fprintf(out, "\n");
                
                /* Call the debug function with label_id and value */
                print_indent(out, indent);
                fprintf(out, "call $__casm_dbg_i32\n");
            }
            break;
        }
    }
}

/* Emit function definitions */
static void emit_function_definitions(FILE* out, ASTProgram* program) {
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        /* Skip dead code - functions without allocated names are unreachable */
        if (!func->allocated_name) {
            continue;
        }
        
        /* Use allocated name for dead code elimination */
        char* mangled_name = mangle_function_name(func->allocated_name);
        
        print_indent(out, 1);
        fprintf(out, "(func $%s", mangled_name);
        
        /* Emit parameters */
        for (int j = 0; j < func->parameter_count; j++) {
            fprintf(out, " (param $%s %s)",
                    func->parameters[j].name,
                    casm_type_to_wat_type(func->parameters[j].type.type));
        }
        
        /* Emit return type */
        if (func->return_type.type != TYPE_VOID) {
            fprintf(out, " (result %s)", casm_type_to_wat_type(func->return_type.type));
        }
        
        /* Collect local variables */
        char* local_names[100];
        int local_count = 0;
        collect_locals(&func->body, local_names, &local_count);
        
        /* Emit local variables declarations */
        for (int j = 0; j < local_count; j++) {
            fprintf(out, " (local $%s i32)", local_names[j]);  /* Assume i32 for now */
        }
        
        fprintf(out, "\n");
        
        /* Emit function body */
        for (int j = 0; j < func->body.statement_count; j++) {
            emit_statement(out, &func->body.statements[j], 2);
        }
        
        print_indent(out, 1);
        fprintf(out, ")\n");
        
        if (i < program->function_count - 1) {
            fprintf(out, "\n");
        }
        
        xfree(mangled_name);
    }
}

/* Main WAT code generation function */
CodegenWatResult codegen_wat_program(ASTProgram* program, FILE* output) {
    if (!program || !output) {
        CodegenWatResult result;
        result.success = 0;
        result.error_msg = "Invalid input to codegen_wat_program";
        return result;
    }
    
    /* Emit module header */
    fprintf(output, "(module\n");
    
    /* Check if there are any dbg statements that need debug support */
    int has_dbg = 0;
    for (int i = 0; i < program->function_count; i++) {
        for (int j = 0; j < program->functions[i].body.statement_count; j++) {
            if (program->functions[i].body.statements[j].type == STMT_DBG) {
                has_dbg = 1;
                break;
            }
        }
        if (has_dbg) break;
    }
    
    /* If there are dbg statements, define a debug function (stub for WAT execution) */
    if (has_dbg) {
        fprintf(output, "  (func $__casm_dbg_i32 (param i32 i32))\n");
    }
    
    /* Emit function definitions */
    emit_function_definitions(output, program);
    
    /* Close module */
    fprintf(output, ")\n");
    
    CodegenWatResult result;
    result.success = 1;
    result.error_msg = NULL;
    return result;
}
