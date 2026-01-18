#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Error list implementation */
ErrorList* error_list_create(void) {
    ErrorList* errors = xmalloc(sizeof(ErrorList));
    errors->errors = NULL;
    errors->error_count = 0;
    errors->error_capacity = 0;
    return errors;
}

void error_list_free(ErrorList* errors) {
    if (!errors) return;
    for (int i = 0; i < errors->error_count; i++) {
        xfree(errors->errors[i].message);
    }
    xfree(errors->errors);
    xfree(errors);
}

void error_list_add(ErrorList* errors, const char* message, SourceLocation location) {
    if (errors->error_count >= errors->error_capacity) {
        errors->error_capacity = errors->error_capacity == 0 ? 10 : errors->error_capacity * 2;
        errors->errors = xrealloc(errors->errors, errors->error_capacity * sizeof(ParseError));
    }
    
    errors->errors[errors->error_count].message = xstrdup(message);
    errors->errors[errors->error_count].location = location;
    errors->error_count++;
}

void error_list_print(ErrorList* errors, const char* filename) {
    for (int i = 0; i < errors->error_count; i++) {
        ParseError* err = &errors->errors[i];
        fprintf(stderr, "%s:%d:%d: %s\n",
                filename, err->location.line, err->location.column, err->message);
    }
}

/* Parser implementation */
Parser* parser_create(const char* source) {
    Parser* parser = xmalloc(sizeof(Parser));
    
    /* Tokenize the entire source */
    Lexer* lexer = lexer_create(source);
    int capacity = 100;
    parser->tokens = xmalloc(capacity * sizeof(Token));
    parser->token_count = 0;
    
    Token token;
    do {
        if (parser->token_count >= capacity) {
            capacity *= 2;
            parser->tokens = xrealloc(parser->tokens, capacity * sizeof(Token));
        }
        
        token = lexer_next_token(lexer);
        parser->tokens[parser->token_count++] = token;
    } while (token.type != TOK_EOF && token.type != TOK_ERROR);
    
    lexer_free(lexer);
    
    parser->current = 0;
    parser->errors = error_list_create();
    
    return parser;
}

void parser_free(Parser* parser) {
    if (!parser) return;
    xfree(parser->tokens);
    error_list_free(parser->errors);
    xfree(parser);
}

/* Helper functions */
static Token current_token(Parser* parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current];
    }
    /* Return EOF token if we're past the end */
    Token eof = {TOK_EOF, "", 0, {0, 0, 0}, 0};
    return eof;
}

static Token advance(Parser* parser) {
    Token token = current_token(parser);
    if (parser->current < parser->token_count) {
        parser->current++;
    }
    return token;
}

static int match(Parser* parser, TokenType type) {
    if (current_token(parser).type == type) {
        advance(parser);
        return 1;
    }
    return 0;
}

static int check(Parser* parser, TokenType type) {
    return current_token(parser).type == type;
}

static void parser_error(Parser* parser, const char* message) {
    Token token = current_token(parser);
    error_list_add(parser->errors, message, token.location);
}

/* Forward declarations */
static ASTExpression* parse_expression(Parser* parser);
static ASTStatement* parse_statement(Parser* parser);
static void parse_block(Parser* parser, ASTBlock* out_block);

/* Parse primary expression: literals, variables, parenthesized expressions, function calls */
static ASTExpression* parse_primary(Parser* parser) {
    Token token = current_token(parser);
    
    /* Integer literal */
    if (token.type == TOK_INT_LITERAL) {
        ASTExpression* expr = ast_expression_create(EXPR_LITERAL, token.location);
        expr->as.literal.type = LITERAL_INT;
        expr->as.literal.value.int_value = token.int_value;
        advance(parser);
        return expr;
    }
    
    /* Boolean literals */
    if (token.type == TOK_TRUE) {
        ASTExpression* expr = ast_expression_create(EXPR_LITERAL, token.location);
        expr->as.literal.type = LITERAL_BOOL;
        expr->as.literal.value.bool_value = 1;
        advance(parser);
        return expr;
    }
    
    if (token.type == TOK_FALSE) {
        ASTExpression* expr = ast_expression_create(EXPR_LITERAL, token.location);
        expr->as.literal.type = LITERAL_BOOL;
        expr->as.literal.value.bool_value = 0;
        advance(parser);
        return expr;
    }
    
    /* Identifier - could be variable or function call */
    if (token.type == TOK_IDENTIFIER) {
        char* name = xstrndup(token.lexeme, token.lexeme_len);
        SourceLocation location = token.location;
        advance(parser);
        
        /* Check for function call */
        if (check(parser, TOK_LPAREN)) {
            advance(parser);  /* consume '(' */
            
            ASTExpression* expr = ast_expression_create(EXPR_FUNCTION_CALL, location);
            expr->as.function_call.function_name = name;
            expr->as.function_call.arguments = NULL;
            expr->as.function_call.argument_count = 0;
            
            /* Parse arguments */
            if (!check(parser, TOK_RPAREN)) {
                int arg_capacity = 10;
                expr->as.function_call.arguments = xmalloc(arg_capacity * sizeof(ASTExpression));
                
                while (1) {
                    if (expr->as.function_call.argument_count >= arg_capacity) {
                        arg_capacity *= 2;
                        expr->as.function_call.arguments = xrealloc(
                            expr->as.function_call.arguments,
                            arg_capacity * sizeof(ASTExpression)
                        );
                    }
                    
                    ASTExpression* arg = parse_expression(parser);
                    if (!arg) {
                        parser_error(parser, "Expected expression in function call");
                        return expr;
                    }
                    
                    expr->as.function_call.arguments[expr->as.function_call.argument_count++] = *arg;
                    xfree(arg);
                    
                    if (!match(parser, TOK_COMMA)) {
                        break;
                    }
                }
            }
            
            if (!match(parser, TOK_RPAREN)) {
                parser_error(parser, "Expected ')' after function arguments");
            }
            
            return expr;
        } else {
            /* Just a variable reference */
            ASTExpression* expr = ast_expression_create(EXPR_VARIABLE, location);
            expr->as.variable.name = name;
            return expr;
        }
    }
    
    /* Parenthesized expression */
    if (token.type == TOK_LPAREN) {
        advance(parser);
        ASTExpression* expr = parse_expression(parser);
        if (!match(parser, TOK_RPAREN)) {
            parser_error(parser, "Expected ')' after expression");
        }
        return expr;
    }
    
    parser_error(parser, "Expected expression");
    return NULL;
}

/* Parse unary expressions: -x, !x */
static ASTExpression* parse_unary(Parser* parser) {
    Token token = current_token(parser);
    
    if (token.type == TOK_MINUS) {
        SourceLocation location = token.location;
        advance(parser);
        ASTExpression* expr = ast_expression_create(EXPR_UNARY_OP, location);
        expr->as.unary_op.op = UNOP_NEG;
        expr->as.unary_op.operand = parse_unary(parser);
        return expr;
    }
    
    if (token.type == TOK_NOT) {
        SourceLocation location = token.location;
        advance(parser);
        ASTExpression* expr = ast_expression_create(EXPR_UNARY_OP, location);
        expr->as.unary_op.op = UNOP_NOT;
        expr->as.unary_op.operand = parse_unary(parser);
        return expr;
    }
    
    return parse_primary(parser);
}

/* Parse multiplicative expressions: *, /, % */
static ASTExpression* parse_multiplicative(Parser* parser) {
    ASTExpression* expr = parse_unary(parser);
    
    while (1) {
        Token token = current_token(parser);
        BinaryOpType op;
        
        if (token.type == TOK_STAR) {
            op = BINOP_MUL;
        } else if (token.type == TOK_SLASH) {
            op = BINOP_DIV;
        } else if (token.type == TOK_PERCENT) {
            op = BINOP_MOD;
        } else {
            break;
        }
        
        SourceLocation location = token.location;
        advance(parser);
        
        ASTExpression* right = parse_unary(parser);
        if (!right) {
            parser_error(parser, "Expected expression after operator");
            return expr;
        }
        
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = right;
        new_expr->as.binary_op.op = op;
        
        expr = new_expr;
    }
    
    return expr;
}

/* Parse additive expressions: +, - */
static ASTExpression* parse_additive(Parser* parser) {
    ASTExpression* expr = parse_multiplicative(parser);
    
    while (1) {
        Token token = current_token(parser);
        BinaryOpType op;
        
        if (token.type == TOK_PLUS) {
            op = BINOP_ADD;
        } else if (token.type == TOK_MINUS) {
            op = BINOP_SUB;
        } else {
            break;
        }
        
        SourceLocation location = token.location;
        advance(parser);
        
        ASTExpression* right = parse_multiplicative(parser);
        if (!right) {
            parser_error(parser, "Expected expression after operator");
            return expr;
        }
        
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = right;
        new_expr->as.binary_op.op = op;
        
        expr = new_expr;
    }
    
    return expr;
}

/* Parse relational expressions: <, >, <=, >= */
static ASTExpression* parse_relational(Parser* parser) {
    ASTExpression* expr = parse_additive(parser);
    
    while (1) {
        Token token = current_token(parser);
        BinaryOpType op;
        
        if (token.type == TOK_LT) {
            op = BINOP_LT;
        } else if (token.type == TOK_GT) {
            op = BINOP_GT;
        } else if (token.type == TOK_LE) {
            op = BINOP_LE;
        } else if (token.type == TOK_GE) {
            op = BINOP_GE;
        } else {
            break;
        }
        
        SourceLocation location = token.location;
        advance(parser);
        
        ASTExpression* right = parse_additive(parser);
        if (!right) {
            parser_error(parser, "Expected expression after operator");
            return expr;
        }
        
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = right;
        new_expr->as.binary_op.op = op;
        
        expr = new_expr;
    }
    
    return expr;
}

/* Parse equality expressions: ==, != */
static ASTExpression* parse_equality(Parser* parser) {
    ASTExpression* expr = parse_relational(parser);
    
    while (1) {
        Token token = current_token(parser);
        BinaryOpType op;
        
        if (token.type == TOK_EQ) {
            op = BINOP_EQ;
        } else if (token.type == TOK_NE) {
            op = BINOP_NE;
        } else {
            break;
        }
        
        SourceLocation location = token.location;
        advance(parser);
        
        ASTExpression* right = parse_relational(parser);
        if (!right) {
            parser_error(parser, "Expected expression after operator");
            return expr;
        }
        
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = right;
        new_expr->as.binary_op.op = op;
        
        expr = new_expr;
    }
    
    return expr;
}

/* Parse logical AND: && */
static ASTExpression* parse_logical_and(Parser* parser) {
    ASTExpression* expr = parse_equality(parser);
    
    while (check(parser, TOK_AND)) {
        SourceLocation location = current_token(parser).location;
        advance(parser);
        
        ASTExpression* right = parse_equality(parser);
        if (!right) {
            parser_error(parser, "Expected expression after &&");
            return expr;
        }
        
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = right;
        new_expr->as.binary_op.op = BINOP_AND;
        
        expr = new_expr;
    }
    
    return expr;
}

/* Parse logical OR: || */
static ASTExpression* parse_logical_or(Parser* parser) {
    ASTExpression* expr = parse_logical_and(parser);
    
    while (check(parser, TOK_OR)) {
        SourceLocation location = current_token(parser).location;
        advance(parser);
        
        ASTExpression* right = parse_logical_and(parser);
        if (!right) {
            parser_error(parser, "Expected expression after ||");
            return expr;
        }
        
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = right;
        new_expr->as.binary_op.op = BINOP_OR;
        
        expr = new_expr;
    }
    
    return expr;
}

/* Parse assignment: var = expr */
static ASTExpression* parse_assignment(Parser* parser) {
    ASTExpression* expr = parse_logical_or(parser);
    
    if (check(parser, TOK_ASSIGN)) {
        SourceLocation location = current_token(parser).location;
        advance(parser);
        
        /* For now, only support variable assignment */
        if (expr->type != EXPR_VARIABLE) {
            parser_error(parser, "Can only assign to variables");
            return expr;
        }
        
        ASTExpression* value = parse_assignment(parser);  /* Right-associative */
        if (!value) {
            parser_error(parser, "Expected expression after =");
            return expr;
        }
        
        /* Convert to a binary operation (assignment) */
        /* For now we'll use a function call as a workaround - we'll handle this differently later */
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = value;
        new_expr->as.binary_op.op = BINOP_ADD;  /* Placeholder - we'll fix this later */
        
        return new_expr;
    }
    
    return expr;
}

/* Top-level expression parsing */
static ASTExpression* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

/* Parse a statement - caller must free returned statement */
static ASTStatement* parse_statement(Parser* parser) {
    Token token = current_token(parser);
    
    /* Return statement */
    if (token.type == TOK_RETURN) {
        SourceLocation location = token.location;
        advance(parser);
        
        ASTStatement* stmt = ast_statement_create(STMT_RETURN, location);
        
        /* Check if there's a return value (not just ';') */
        if (!check(parser, TOK_SEMICOLON)) {
            stmt->as.return_stmt.value = parse_expression(parser);
        } else {
            stmt->as.return_stmt.value = NULL;
        }
        
        if (!match(parser, TOK_SEMICOLON)) {
            parser_error(parser, "Expected ';' after return statement");
        }
        
        return stmt;
    }
    
    /* Unsupported control flow - report error and skip it */
    if (token.type == TOK_IF || token.type == TOK_WHILE || token.type == TOK_FOR) {
        const char* keyword = (token.type == TOK_IF) ? "if" : 
                              (token.type == TOK_WHILE) ? "while" : "for";
        char msg[100];
        snprintf(msg, sizeof(msg), "Control flow '%s' not yet implemented", keyword);
        parser_error(parser, msg);
        
        /* MUST advance to avoid infinite loop in block parser */
        advance(parser);
        
        /* Skip to closing brace or semicolon - simple recovery */
        int depth = 0;
        while (!check(parser, TOK_EOF)) {
            if (check(parser, TOK_LBRACE)) {
                depth++;
            } else if (check(parser, TOK_RBRACE)) {
                if (depth == 0) {
                    /* Don't consume the closing brace - it belongs to parent block */
                    break;
                }
                depth--;
            }
            advance(parser);
        }
        
        return NULL;  /* Return NULL to skip this statement */
    }
    
    /* Variable declaration */
    if (token.type == TOK_I8 || token.type == TOK_I16 || token.type == TOK_I32 ||
        token.type == TOK_I64 || token.type == TOK_U8 || token.type == TOK_U16 ||
        token.type == TOK_U32 || token.type == TOK_U64 || token.type == TOK_BOOL ||
        token.type == TOK_VOID) {
        
        SourceLocation location = token.location;
        TypeNode type;
        type.location = location;
        type.type = token_type_to_casm_type(token.type);
        advance(parser);
        
        if (!check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected identifier after type");
            return NULL;
        }
        
        char* name = xstrndup(current_token(parser).lexeme, current_token(parser).lexeme_len);
        advance(parser);
        
        ASTStatement* stmt = ast_statement_create(STMT_VAR_DECL, location);
        stmt->as.var_decl_stmt.var_decl.name = name;
        stmt->as.var_decl_stmt.var_decl.type = type;
        stmt->as.var_decl_stmt.var_decl.location = location;
        stmt->as.var_decl_stmt.var_decl.initializer = NULL;
        
        /* Check for initializer */
        if (match(parser, TOK_ASSIGN)) {
            stmt->as.var_decl_stmt.var_decl.initializer = parse_expression(parser);
            if (!stmt->as.var_decl_stmt.var_decl.initializer) {
                parser_error(parser, "Expected expression after =");
            }
        }
        
        if (!match(parser, TOK_SEMICOLON)) {
            parser_error(parser, "Expected ';' after variable declaration");
        }
        
        return stmt;
    }
    
    /* Expression statement */
    ASTExpression* expr = parse_expression(parser);
    if (!expr) {
        parser_error(parser, "Expected statement");
        return NULL;
    }
    
    ASTStatement* stmt = ast_statement_create(STMT_EXPR, token.location);
    stmt->as.expr_stmt.expr = expr;
    
    if (!match(parser, TOK_SEMICOLON)) {
        parser_error(parser, "Expected ';' after expression");
    }
    
    return stmt;
}

/* Parse a block of statements */
static void parse_block(Parser* parser, ASTBlock* out_block) {
    out_block->location = current_token(parser).location;
    out_block->statements = NULL;
    out_block->statement_count = 0;
    
    if (!match(parser, TOK_LBRACE)) {
        parser_error(parser, "Expected '{'");
        return;
    }
    
    int stmt_capacity = 10;
    out_block->statements = xmalloc(stmt_capacity * sizeof(ASTStatement));
    
    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        ASTStatement* stmt = parse_statement(parser);
        if (stmt) {
            if (out_block->statement_count >= stmt_capacity) {
                stmt_capacity *= 2;
                out_block->statements = xrealloc(out_block->statements, stmt_capacity * sizeof(ASTStatement));
            }
            /* Copy statement and steal its allocations */
            out_block->statements[out_block->statement_count] = *stmt;
            out_block->statement_count++;
            /* Free the wrapper struct but NOT its contents (they're now owned by block) */
            free(stmt);
        }
    }
    
    if (!match(parser, TOK_RBRACE)) {
        parser_error(parser, "Expected '}'");
    }
}

/* Parse a function definition - fills in provided struct */
static int parse_function(Parser* parser, ASTFunctionDef* out_func) {
    int error_count_before = parser->errors->error_count;
    Token token = current_token(parser);
    
    /* Parse return type */
    if (!(token.type == TOK_I8 || token.type == TOK_I16 || token.type == TOK_I32 ||
          token.type == TOK_I64 || token.type == TOK_U8 || token.type == TOK_U16 ||
          token.type == TOK_U32 || token.type == TOK_U64 || token.type == TOK_BOOL ||
          token.type == TOK_VOID)) {
        parser_error(parser, "Expected type for function return");
        return 0;
    }
    
    TypeNode return_type;
    return_type.type = token_type_to_casm_type(token.type);
    return_type.location = token.location;
    advance(parser);
    
    if (!check(parser, TOK_IDENTIFIER)) {
        parser_error(parser, "Expected function name");
        return 0;
    }
    
    char* name = xstrndup(current_token(parser).lexeme, current_token(parser).lexeme_len);
    SourceLocation location = current_token(parser).location;
    advance(parser);
    
    out_func->name = name;
    out_func->return_type = return_type;
    out_func->location = location;
    out_func->parameters = NULL;
    out_func->parameter_count = 0;
    out_func->body.statements = NULL;
    out_func->body.statement_count = 0;
    out_func->body.location = location;
    
    /* Parse parameters */
    if (!match(parser, TOK_LPAREN)) {
        parser_error(parser, "Expected '(' after function name");
        return 0;
    }
    
    if (!check(parser, TOK_RPAREN)) {
        int param_capacity = 10;
        out_func->parameters = xmalloc(param_capacity * sizeof(ASTParameter));
        
        while (1) {
            Token param_token = current_token(parser);
            if (!(param_token.type == TOK_I8 || param_token.type == TOK_I16 ||
                  param_token.type == TOK_I32 || param_token.type == TOK_I64 ||
                  param_token.type == TOK_U8 || param_token.type == TOK_U16 ||
                  param_token.type == TOK_U32 || param_token.type == TOK_U64 ||
                  param_token.type == TOK_BOOL || param_token.type == TOK_VOID)) {
                parser_error(parser, "Expected type in parameter list");
                /* Skip to next comma or close paren */
                while (!check(parser, TOK_COMMA) && !check(parser, TOK_RPAREN) && !check(parser, TOK_EOF)) {
                    advance(parser);
                }
                if (check(parser, TOK_COMMA)) {
                    advance(parser);
                    continue;
                } else {
                    break;
                }
            }
            
            TypeNode param_type;
            param_type.type = token_type_to_casm_type(param_token.type);
            param_type.location = param_token.location;
            advance(parser);
            
            if (!check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected parameter name");
                break;
            }
            
            char* param_name = xstrndup(current_token(parser).lexeme, current_token(parser).lexeme_len);
            SourceLocation param_location = current_token(parser).location;
            advance(parser);
            
            if (out_func->parameter_count >= param_capacity) {
                param_capacity *= 2;
                out_func->parameters = xrealloc(out_func->parameters, param_capacity * sizeof(ASTParameter));
            }
            
            out_func->parameters[out_func->parameter_count].name = param_name;
            out_func->parameters[out_func->parameter_count].type = param_type;
            out_func->parameters[out_func->parameter_count].location = param_location;
            out_func->parameter_count++;
            
            if (!match(parser, TOK_COMMA)) {
                break;
            }
        }
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser_error(parser, "Expected ')' after parameters");
    }
    
    /* Parse function body */
    parse_block(parser, &out_func->body);
    
    /* Return success only if no errors were added during this function */
    return parser->errors->error_count == error_count_before;
}

/* Main parsing function */
ASTProgram* parser_parse(Parser* parser) {
    ASTProgram* program = ast_program_create();
    
    int func_capacity = 10;
    program->functions = xmalloc(func_capacity * sizeof(ASTFunctionDef));
    
    while (!check(parser, TOK_EOF)) {
        if (program->function_count >= func_capacity) {
            func_capacity *= 2;
            program->functions = xrealloc(program->functions, func_capacity * sizeof(ASTFunctionDef));
        }
        
        if (parse_function(parser, &program->functions[program->function_count])) {
            program->function_count++;
        }
    }
    
    return program;
}
