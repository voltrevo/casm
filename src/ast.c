#include "ast.h"
#include <stdlib.h>
#include <string.h>

/* Forward declaration */
static void ast_statement_free_contents(ASTStatement* stmt);
static void ast_expression_free_contents(ASTExpression* expr);

const char* type_to_string(CasmType type) {
    switch (type) {
        case TYPE_I8: return "i8";
        case TYPE_I16: return "i16";
        case TYPE_I32: return "i32";
        case TYPE_I64: return "i64";
        case TYPE_U8: return "u8";
        case TYPE_U16: return "u16";
        case TYPE_U32: return "u32";
        case TYPE_U64: return "u64";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
    }
    return "unknown";
}

CasmType token_type_to_casm_type(TokenType tok_type) {
    switch (tok_type) {
        case TOK_I8: return TYPE_I8;
        case TOK_I16: return TYPE_I16;
        case TOK_I32: return TYPE_I32;
        case TOK_I64: return TYPE_I64;
        case TOK_U8: return TYPE_U8;
        case TOK_U16: return TYPE_U16;
        case TOK_U32: return TYPE_U32;
        case TOK_U64: return TYPE_U64;
        case TOK_BOOL: return TYPE_BOOL;
        case TOK_VOID: return TYPE_VOID;
        default: return TYPE_I32;  /* default fallback */
    }
}

ASTProgram* ast_program_create(void) {
    ASTProgram* program = xmalloc(sizeof(ASTProgram));
    program->functions = NULL;
    program->function_count = 0;
    return program;
}

void ast_program_free(ASTProgram* program) {
    if (!program) return;
    for (int i = 0; i < program->function_count; i++) {
        ast_function_free(&program->functions[i]);
    }
    xfree(program->functions);
    xfree(program);
}

ASTFunctionDef* ast_function_create(const char* name, TypeNode return_type, SourceLocation location) {
    ASTFunctionDef* func = xmalloc(sizeof(ASTFunctionDef));
    func->name = xstrdup(name);
    func->return_type = return_type;
    func->parameters = NULL;
    func->parameter_count = 0;
    func->body.statements = NULL;
    func->body.statement_count = 0;
    func->body.location = location;
    func->location = location;
    return func;
}

void ast_function_free(ASTFunctionDef* func) {
    if (!func) return;
    xfree(func->name);
    for (int i = 0; i < func->parameter_count; i++) {
        ast_parameter_free(&func->parameters[i]);
    }
    xfree(func->parameters);
    ast_block_free(&func->body);
}

ASTBlock* ast_block_create(void) {
    ASTBlock* block = xmalloc(sizeof(ASTBlock));
    block->statements = NULL;
    block->statement_count = 0;
    return block;
}

void ast_block_free(ASTBlock* block) {
    if (!block) return;
    for (int i = 0; i < block->statement_count; i++) {
        /* Free statement contents only, not the statement itself (it's embedded in the array) */
        ast_statement_free_contents(&block->statements[i]);
    }
    xfree(block->statements);
}

void ast_block_add_statement(ASTBlock* block, ASTStatement stmt) {
    block->statements = xrealloc(block->statements, 
                                  (block->statement_count + 1) * sizeof(ASTStatement));
    block->statements[block->statement_count++] = stmt;
}

ASTStatement* ast_statement_create(StatementType type, SourceLocation location) {
    ASTStatement* stmt = xmalloc(sizeof(ASTStatement));
    stmt->type = type;
    stmt->location = location;
    return stmt;
}

void ast_statement_free(ASTStatement* stmt) {
    if (!stmt) return;
    ast_statement_free_contents(stmt);
    xfree(stmt);
}

/* Free statement contents only (for embedded statements in arrays) */
static void ast_statement_free_contents(ASTStatement* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_RETURN:
            if (stmt->as.return_stmt.value) {
                ast_expression_free(stmt->as.return_stmt.value);
            }
            break;
        case STMT_EXPR:
            if (stmt->as.expr_stmt.expr) {
                ast_expression_free(stmt->as.expr_stmt.expr);
            }
            break;
        case STMT_VAR_DECL:
            if (stmt->as.var_decl_stmt.var_decl.initializer) {
                ast_expression_free(stmt->as.var_decl_stmt.var_decl.initializer);
            }
            xfree(stmt->as.var_decl_stmt.var_decl.name);
            break;
        case STMT_IF:
            ast_expression_free(stmt->as.if_stmt.condition);
            ast_block_free(&stmt->as.if_stmt.then_body);
            ast_else_if_free(stmt->as.if_stmt.else_if_chain);
            if (stmt->as.if_stmt.else_body) {
                ast_block_free(stmt->as.if_stmt.else_body);
                xfree(stmt->as.if_stmt.else_body);
            }
            break;
        case STMT_WHILE:
            ast_expression_free(stmt->as.while_stmt.condition);
            ast_block_free(&stmt->as.while_stmt.body);
            break;
        case STMT_FOR:
            if (stmt->as.for_stmt.init) {
                ast_statement_free(stmt->as.for_stmt.init);
            }
            if (stmt->as.for_stmt.condition) {
                ast_expression_free(stmt->as.for_stmt.condition);
            }
            if (stmt->as.for_stmt.update) {
                ast_expression_free(stmt->as.for_stmt.update);
            }
            ast_block_free(&stmt->as.for_stmt.body);
            break;
    }
}

ASTExpression* ast_expression_create(ExpressionType type, SourceLocation location) {
    ASTExpression* expr = xmalloc(sizeof(ASTExpression));
    expr->type = type;
    expr->location = location;
    return expr;
}

void ast_expression_free(ASTExpression* expr) {
    if (!expr) return;
    ast_expression_free_contents(expr);
    xfree(expr);
}

/* Free expression contents only (for embedded expressions in arrays) */
static void ast_expression_free_contents(ASTExpression* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_BINARY_OP:
            ast_expression_free(expr->as.binary_op.left);
            ast_expression_free(expr->as.binary_op.right);
            break;
        case EXPR_UNARY_OP:
            ast_expression_free(expr->as.unary_op.operand);
            break;
        case EXPR_FUNCTION_CALL:
            xfree(expr->as.function_call.function_name);
            for (int i = 0; i < expr->as.function_call.argument_count; i++) {
                /* Arguments are embedded in array, so free contents only */
                ast_expression_free_contents(&expr->as.function_call.arguments[i]);
            }
            xfree(expr->as.function_call.arguments);
            break;
        case EXPR_VARIABLE:
            xfree(expr->as.variable.name);
            break;
        case EXPR_LITERAL:
            /* Literals have no sub-allocations */
            break;
    }
}

ASTParameter* ast_parameter_create(const char* name, TypeNode type, SourceLocation location) {
    ASTParameter* param = xmalloc(sizeof(ASTParameter));
    param->name = xstrdup(name);
    param->type = type;
    param->location = location;
    return param;
}

void ast_parameter_free(ASTParameter* param) {
    if (!param) return;
    xfree(param->name);
}

ASTElseIfClause* ast_else_if_create(ASTExpression* cond, ASTBlock body, SourceLocation location) {
    (void)location;  /* Suppress unused parameter warning */
    ASTElseIfClause* clause = xmalloc(sizeof(ASTElseIfClause));
    clause->condition = cond;
    clause->body = body;
    clause->next = NULL;
    return clause;
}

void ast_else_if_free(ASTElseIfClause* clause) {
    if (!clause) return;
    ast_expression_free(clause->condition);
    ast_block_free(&clause->body);
    ast_else_if_free(clause->next);  /* Recursive free of chain */
    xfree(clause);
}

