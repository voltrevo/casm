#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "utils.h"

static char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        exit(1);
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = xmalloc(size + 1);
    if (fread(content, 1, size, file) != (size_t)size) {
        fprintf(stderr, "Error: Could not read file '%s'\n", filename);
        fclose(file);
        exit(1);
    }
    fclose(file);
    
    content[size] = '\0';
    return content;
}

static void print_ast_program(ASTProgram* program);

static void print_ast_expression(ASTExpression* expr, int indent) {
    if (!expr) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (expr->type) {
        case EXPR_LITERAL:
            if (expr->as.literal.type == LITERAL_INT) {
                printf("Literal(%ld)\n", expr->as.literal.value.int_value);
            } else {
                printf("Literal(%s)\n", expr->as.literal.value.bool_value ? "true" : "false");
            }
            break;
        case EXPR_VARIABLE:
            printf("Variable(%s)\n", expr->as.variable.name);
            break;
        case EXPR_FUNCTION_CALL:
            printf("FunctionCall(%s, %d args)\n", expr->as.function_call.function_name, expr->as.function_call.argument_count);
            for (int i = 0; i < expr->as.function_call.argument_count; i++) {
                print_ast_expression(&expr->as.function_call.arguments[i], indent + 1);
            }
            break;
        case EXPR_BINARY_OP:
            printf("BinaryOp(%d)\n", expr->as.binary_op.op);
            print_ast_expression(expr->as.binary_op.left, indent + 1);
            print_ast_expression(expr->as.binary_op.right, indent + 1);
            break;
        case EXPR_UNARY_OP:
            printf("UnaryOp(%d)\n", expr->as.unary_op.op);
            print_ast_expression(expr->as.unary_op.operand, indent + 1);
            break;
    }
}

static void print_ast_statement(ASTStatement* stmt, int indent);

static void print_ast_statement(ASTStatement* stmt, int indent) {
    if (!stmt) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (stmt->type) {
        case STMT_RETURN:
            printf("Return\n");
            if (stmt->as.return_stmt.value) {
                print_ast_expression(stmt->as.return_stmt.value, indent + 1);
            }
            break;
        case STMT_VAR_DECL:
            printf("VarDecl(%s : %s)\n", stmt->as.var_decl_stmt.var_decl.name,
                   type_to_string(stmt->as.var_decl_stmt.var_decl.type.type));
            if (stmt->as.var_decl_stmt.var_decl.initializer) {
                print_ast_expression(stmt->as.var_decl_stmt.var_decl.initializer, indent + 1);
            }
            break;
        case STMT_EXPR:
            printf("ExprStmt\n");
            print_ast_expression(stmt->as.expr_stmt.expr, indent + 1);
            break;
    }
}

static void print_ast_function(ASTFunctionDef* func) {
    printf("Function(%s : %s, %d params)\n", func->name, type_to_string(func->return_type.type), func->parameter_count);
    
    for (int i = 0; i < func->parameter_count; i++) {
        printf("  Param(%s : %s)\n", func->parameters[i].name, type_to_string(func->parameters[i].type.type));
    }
    
    printf("  Body:\n");
    for (int i = 0; i < func->body.statement_count; i++) {
        print_ast_statement(&func->body.statements[i], 2);
    }
}

static void print_ast_program(ASTProgram* program) {
    printf("Program (%d functions)\n", program->function_count);
    for (int i = 0; i < program->function_count; i++) {
        print_ast_function(&program->functions[i]);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.csm>\n", argv[0]);
        return 1;
    }
    
    char* source = read_file(argv[1]);
    Parser* parser = parser_create(source);
    
    ASTProgram* program = parser_parse(parser);
    
    if (parser->errors->error_count > 0) {
        error_list_print(parser->errors, argv[1]);
        parser_free(parser);
        xfree(source);
        return 1;
    }
    
    print_ast_program(program);
    
    ast_program_free(program);
    parser_free(parser);
    xfree(source);
    return 0;
}
