#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen_wat.h"
#include "utils.h"

/* Global reference to the program being compiled (for function call resolution) */
static ASTProgram* g_current_program = NULL;

/* Global reference to the source filename (for debug output) */
static const char* g_source_filename = NULL;

/* Global reference to the current function being compiled (for module context in calls) */
static ASTFunctionDef* g_current_function = NULL;

/* Debug format string storage for WAT data section */
typedef struct {
    char* format_string;  /* The format string with % placeholders */
    int offset;           /* Offset in data section */
    int length;           /* Length of format string */
    CasmType* arg_types;  /* Array of types for each % placeholder */
    int arg_count;        /* Number of arguments */
} DebugFormatString;

/* Global array to collect debug format strings during code generation */
static DebugFormatString* g_debug_formats = NULL;
static int g_debug_format_count = 0;
static int g_debug_format_capacity = 0;

/* Track current data offset for packing format strings */
static int g_data_offset = 0;

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

/* Helper: Get the debug value function name for a type */
static const char* get_debug_value_func_name(CasmType type) {
    switch (type) {
        case TYPE_I8:
        case TYPE_I16:
        case TYPE_I32:   return "$debug_value_i32";
        case TYPE_I64:   return "$debug_value_i64";
        case TYPE_U8:
        case TYPE_U16:
        case TYPE_U32:   return "$debug_value_u32";
        case TYPE_U64:   return "$debug_value_u64";
        case TYPE_BOOL:  return "$debug_value_bool";
        default:         return NULL;  /* Unsupported type */
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

/* Helper: Register a debug format string and return its offset */
static int register_debug_format(ASTDbgStmt* dbg) {
    /* Ensure we have capacity */
    if (g_debug_format_count >= g_debug_format_capacity) {
        g_debug_format_capacity = g_debug_format_capacity == 0 ? 10 : g_debug_format_capacity * 2;
        g_debug_formats = xrealloc(g_debug_formats, g_debug_format_capacity * sizeof(DebugFormatString));
    }
    
     /* Build format string: "file:line:col: arg1 = %, arg2 = %, ..." */
     char format_buf[1024];
     int len = snprintf(format_buf, sizeof(format_buf), "%s:%d:%d: ",
                        g_source_filename ? g_source_filename : "unknown",
                        dbg->location.line, dbg->location.column);
    
     /* Add argument names with % placeholders
         Note: We need to escape any % in arg_name by doubling it (%% in WAT format string)
         to distinguish them from the % placeholder */
      for (int i = 0; i < dbg->argument_count; i++) {
          if (i > 0) len += snprintf(format_buf + len, sizeof(format_buf) - len, ", ");
          
          const char* arg_name = dbg->arg_names[i] && strlen(dbg->arg_names[i]) > 0 
                               ? dbg->arg_names[i] 
                               : "arg";
          
          /* Copy arg_name to format_buf, escaping any % characters */
          for (const char* p = arg_name; *p; p++) {
              if (*p == '%') {
                  format_buf[len++] = '%';
                  format_buf[len++] = '%';
              } else {
                  format_buf[len++] = *p;
              }
          }
          
          /* Add the " = %%" suffix (where %% is the placeholder) */
          len += snprintf(format_buf + len, sizeof(format_buf) - len, " = %%");
      }
     
     /* Calculate actual string length (snprintf with %% counts as 2 but produces 1 in output) */
     len = strlen(format_buf);

    
    /* Store the format string */
    DebugFormatString* fmt = &g_debug_formats[g_debug_format_count];
    fmt->format_string = xstrdup(format_buf);
    fmt->length = len;
    fmt->offset = g_data_offset;
    fmt->arg_types = xmalloc(dbg->argument_count * sizeof(CasmType));
    fmt->arg_count = dbg->argument_count;
    
    /* Copy argument types */
    for (int i = 0; i < dbg->argument_count; i++) {
        fmt->arg_types[i] = dbg->arguments[i].resolved_type;
    }
    
    int result_offset = g_data_offset;
    g_data_offset += len;
    g_debug_format_count++;
    
    return result_offset;
}

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
                /* Assignment: evaluate RHS, store to LHS, and leave value on stack
                   Use local.tee instead of local.set so the assigned value remains
                   on the stack for use in expressions like dbg(x = 5) */
                emit_expression(out, binop->right, indent);
                fprintf(out, "\n");
                print_indent(out, indent);
                fprintf(out, "local.tee $%s", binop->left->as.variable.name);
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
            
            if (unop->op == UNOP_NEG) {
                /* Negation: compute 0 - operand
                   Push 0 first, then operand, then subtract */
                print_indent(out, indent);
                fprintf(out, "i32.const 0\n");
                emit_expression(out, unop->operand, indent);
                fprintf(out, "\n");
                print_indent(out, indent);
                fprintf(out, "i32.sub");
            } else if (unop->op == UNOP_NOT) {
                /* Logical NOT */
                emit_expression(out, unop->operand, indent);
                fprintf(out, "\n");
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
            /* Look up the actual function name (handles allocated names with mangling) */
            const char* call_target = get_call_target_name(call->function_name);
            char* mangled_name = mangle_function_name(call_target);
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
            /* WAT dbg support - emit begin/value/end pattern with format strings */
            ASTDbgStmt* dbg = &stmt->as.dbg_stmt;
            
            /* Validate all argument types are supported */
            for (int i = 0; i < dbg->argument_count; i++) {
                if (!get_debug_value_func_name(dbg->arguments[i].resolved_type)) {
                    /* Type not supported - codegen will fail */
                    fprintf(stderr, "Error: unsupported type in dbg() statement\n");
                    exit(1);
                }
            }
            
            /* Register format string and get its offset */
            int format_offset = register_debug_format(dbg);
            
            /* Get format string length */
            int format_len = g_debug_formats[g_debug_format_count - 1].length;
            
            /* Emit: debug_begin(format_ptr, format_len) */
            print_indent(out, indent);
            fprintf(out, "i32.const %d\n", format_offset);
            print_indent(out, indent);
            fprintf(out, "i32.const %d\n", format_len);
            print_indent(out, indent);
            fprintf(out, "call $debug_begin\n");
            
            /* Emit each argument with type-specific function */
            for (int i = 0; i < dbg->argument_count; i++) {
                CasmType arg_type = dbg->arguments[i].resolved_type;
                const char* func_name = get_debug_value_func_name(arg_type);
                
                /* Emit the expression value */
                emit_expression(out, &dbg->arguments[i], indent);
                fprintf(out, "\n");
                
                /* Call the type-specific debug_value function */
                print_indent(out, indent);
                fprintf(out, "call %s\n", func_name);
            }
            
            /* Emit: debug_end() */
            print_indent(out, indent);
            fprintf(out, "call $debug_end\n");
            break;
        }
    }
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

        emit_count++;
        if (emit_count < emit_total) {
            fprintf(out, "\n");
        }
        
        xfree(mangled_name);
        
        /* Clear context */
        g_current_function = NULL;
    }
}

/* Recursively check if a statement or any nested statement contains a dbg() call */
static int statement_contains_dbg(ASTStatement* stmt) {
    if (!stmt) return 0;
    
    switch (stmt->type) {
        case STMT_DBG:
            return 1;
        
        case STMT_IF: {
            ASTIfStmt* if_stmt = &stmt->as.if_stmt;
            
            /* Check then body */
            for (int i = 0; i < if_stmt->then_body.statement_count; i++) {
                if (statement_contains_dbg(&if_stmt->then_body.statements[i])) {
                    return 1;
                }
            }
            
            /* Check else-if chain */
            for (ASTElseIfClause* elif = if_stmt->else_if_chain; elif; elif = elif->next) {
                for (int i = 0; i < elif->body.statement_count; i++) {
                    if (statement_contains_dbg(&elif->body.statements[i])) {
                        return 1;
                    }
                }
            }
            
            /* Check else body */
            if (if_stmt->else_body) {
                for (int i = 0; i < if_stmt->else_body->statement_count; i++) {
                    if (statement_contains_dbg(&if_stmt->else_body->statements[i])) {
                        return 1;
                    }
                }
            }
            break;
        }
        
        case STMT_WHILE: {
            ASTWhileStmt* while_stmt = &stmt->as.while_stmt;
            for (int i = 0; i < while_stmt->body.statement_count; i++) {
                if (statement_contains_dbg(&while_stmt->body.statements[i])) {
                    return 1;
                }
            }
            break;
        }
        
        case STMT_FOR: {
            ASTForStmt* for_stmt = &stmt->as.for_stmt;
            
            /* Check init statement */
            if (for_stmt->init && statement_contains_dbg(for_stmt->init)) {
                return 1;
            }
            
            /* Check loop body */
            for (int i = 0; i < for_stmt->body.statement_count; i++) {
                if (statement_contains_dbg(&for_stmt->body.statements[i])) {
                    return 1;
                }
            }
            break;
        }
        
        case STMT_BLOCK: {
            ASTBlockStmt* block_stmt = &stmt->as.block_stmt;
            for (int i = 0; i < block_stmt->block.statement_count; i++) {
                if (statement_contains_dbg(&block_stmt->block.statements[i])) {
                    return 1;
                }
            }
            break;
        }
        
        case STMT_RETURN:
        case STMT_EXPR:
        case STMT_VAR_DECL:
            /* These statements don't contain nested statements */
            break;
    }
    
    return 0;
}

/* Main WAT code generation function */
CodegenWatResult codegen_wat_program(ASTProgram* program, FILE* output, const char* source_filename) {
    if (!program || !output) {
        CodegenWatResult result;
        result.success = 0;
        result.error_msg = "Invalid input to codegen_wat_program";
        return result;
    }
    
    /* Store program and source filename references for use in code emission */
    g_current_program = program;
    g_source_filename = source_filename;
    
    /* Initialize debug format collection */
    g_data_offset = 0;
    g_debug_format_count = 0;
    
    /* Emit module header */
    fprintf(output, "(module\n");
    
    /* Check if there are any dbg statements that need debug support */
    int has_dbg = 0;
    for (int i = 0; i < program->function_count; i++) {
        if (program->import_count > 0 && !program->functions[i].allocated_name) {
            continue;
        }
        /* Check all statements in function body, including nested ones */
        for (int j = 0; j < program->functions[i].body.statement_count; j++) {
            if (statement_contains_dbg(&program->functions[i].body.statements[j])) {
                has_dbg = 1;
                break;
            }
        }
        if (has_dbg) break;
    }
    
    /* If there are dbg statements, emit host imports and memory */
    if (has_dbg) {
        fprintf(output, "  (import \"host\" \"debug_begin\" (func $debug_begin (param i32 i32)))\n");
        fprintf(output, "  (import \"host\" \"debug_value_i32\" (func $debug_value_i32 (param i32)))\n");
        fprintf(output, "  (import \"host\" \"debug_value_i64\" (func $debug_value_i64 (param i64)))\n");
        fprintf(output, "  (import \"host\" \"debug_value_u32\" (func $debug_value_u32 (param i32)))\n");
        fprintf(output, "  (import \"host\" \"debug_value_u64\" (func $debug_value_u64 (param i64)))\n");
        fprintf(output, "  (import \"host\" \"debug_value_bool\" (func $debug_value_bool (param i32)))\n");
        fprintf(output, "  (import \"host\" \"debug_end\" (func $debug_end))\n");
        
        fprintf(output, "  (memory 1)\n");
    }
    
    /* Emit function definitions (this will register debug formats as they're encountered) */
    emit_function_definitions(output, program);
    
     /* Now emit data section with all collected format strings */
     if (has_dbg && g_debug_format_count > 0) {
         fprintf(output, "  (data (i32.const 0)");
         for (int i = 0; i < g_debug_format_count; i++) {
             /* Use fputs for the string literal to avoid fprintf interpreting % chars */
             fprintf(output, " \"");
             fputs(g_debug_formats[i].format_string, output);
             fprintf(output, "\"");
         }
         fprintf(output, ")\n");
        
        /* Export memory so host can access debug strings */
        fprintf(output, "  (export \"memory\" (memory 0))\n");
    }
    
    /* Export the main function if it exists */
    for (int i = 0; i < program->function_count; i++) {
        if (program->import_count > 0 && !program->functions[i].allocated_name) {
            continue;
        }
        if (strcmp(program->functions[i].name, "main") == 0) {
            const char* func_name = program->functions[i].allocated_name ? 
                                   program->functions[i].allocated_name : 
                                   program->functions[i].name;
            char* mangled_name = mangle_function_name(func_name);
            fprintf(output, "  (export \"main\" (func $%s))\n", mangled_name);
            xfree(mangled_name);
            break;
        }
    }
    
    /* Close module */
    fprintf(output, ")\n");
    
    /* Clean up debug format strings */
    for (int i = 0; i < g_debug_format_count; i++) {
        xfree(g_debug_formats[i].format_string);
        xfree(g_debug_formats[i].arg_types);
    }
    xfree(g_debug_formats);
    g_debug_formats = NULL;
    g_debug_format_count = 0;
    g_debug_format_capacity = 0;
    
    /* Clear global reference */
    g_current_program = NULL;
    
    CodegenWatResult result;
    result.success = 1;
    result.error_msg = NULL;
    return result;
}
