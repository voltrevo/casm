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
    
    parser->source = source;  /* Store source for later reference */
    
    /* Tokenize the entire source */
    Lexer* lexer = lexer_create(source);
    int capacity = 100;
    parser->tokens = xmalloc(capacity * sizeof(Token));
    parser->token_count = 0;
    
    parser->current = 0;
    parser->errors = error_list_create();
    
    Token token;
    do {
        if (parser->token_count >= capacity) {
            capacity *= 2;
            parser->tokens = xrealloc(parser->tokens, capacity * sizeof(Token));
        }
        
        token = lexer_next_token(lexer);
        parser->tokens[parser->token_count++] = token;
        
        /* Report lexer errors immediately */
        if (token.type == TOK_ERROR) {
            error_list_add(parser->errors, "Integer overflow: value too large", token.location);
            /* Continue lexing to get EOF */
        }
    } while (token.type != TOK_EOF);
    
    lexer_free(lexer);
    
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
        
        /* Check for qualified name (module:name) */
        if (check(parser, TOK_COLON)) {
            advance(parser);  /* consume ':' */
            
            if (!check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected identifier after ':' in qualified name");
                xfree(name);
                return NULL;
            }
            
            char* qualified_part = xstrndup(current_token(parser).lexeme, current_token(parser).lexeme_len);
            advance(parser);
            
            /* Build qualified name: "module:name" */
            int total_len = strlen(name) + 1 + strlen(qualified_part);
            char* qualified_name = xmalloc(total_len + 1);
            snprintf(qualified_name, total_len + 1, "%s:%s", name, qualified_part);
            xfree(name);
            xfree(qualified_part);
            name = qualified_name;
        }
        
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
        ASTExpression* new_expr = ast_expression_create(EXPR_BINARY_OP, location);
        new_expr->as.binary_op.left = expr;
        new_expr->as.binary_op.right = value;
        new_expr->as.binary_op.op = BINOP_ASSIGN;
        
        return new_expr;
    }
    
    return expr;
}

/* Top-level expression parsing */
static ASTExpression* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

/* Parse a block: { statements } - returns heap-allocated ASTBlock */
static ASTBlock* parse_block_stmt(Parser* parser) {
    ASTBlock* block = ast_block_create();
    block->location = current_token(parser).location;
    
    if (!match(parser, TOK_LBRACE)) {
        parser_error(parser, "Expected '{' at start of block");
        return block;
    }
    
    /* Parse statements until we hit } or EOF */
    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        /* Stop if we hit a lexer error token */
        if (check(parser, TOK_ERROR)) {
            break;
        }
        
        ASTStatement* stmt = parse_statement(parser);
        if (stmt) {
            ast_block_add_statement(block, *stmt);
            xfree(stmt);  /* Free the heap-allocated statement since we copied it */
        } else {
            /* Statement failed to parse - skip one token for error recovery */
            advance(parser);
        }
    }
    
    if (!match(parser, TOK_RBRACE)) {
        parser_error(parser, "Expected '}' at end of block");
    }
    
    return block;
}

/* Parse an if statement with optional else-if chain and optional else */
static ASTStatement* parse_if_statement(Parser* parser) {
    SourceLocation location = current_token(parser).location;
    advance(parser);  /* consume 'if' */
    
    if (!match(parser, TOK_LPAREN)) {
        parser_error(parser, "Expected '(' after 'if'");
        return NULL;
    }
    
    ASTExpression* condition = parse_expression(parser);
    if (!condition) {
        parser_error(parser, "Expected expression in if condition");
        return NULL;
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser_error(parser, "Expected ')' after if condition");
        return NULL;
    }
    
    /* Require block body */
    if (!check(parser, TOK_LBRACE)) {
        parser_error(parser, "If statement body must be a block (use {...})");
        ast_expression_free(condition);
        return NULL;
    }
    
    ASTBlock* then_body = parse_block_stmt(parser);
    
    /* Parse optional else-if chain and final else block */
    ASTElseIfClause* else_if_chain = NULL;
    ASTElseIfClause** else_if_tail = &else_if_chain;
    ASTBlock* else_body = NULL;
    
    while (check(parser, TOK_ELSE)) {
        advance(parser);  /* consume 'else' */
        
        /* Check for else-if */
        if (check(parser, TOK_IF)) {
            advance(parser);  /* consume 'if' */
            
            if (!match(parser, TOK_LPAREN)) {
                parser_error(parser, "Expected '(' after 'else if'");
                return NULL;
            }
            
            ASTExpression* elif_cond = parse_expression(parser);
            if (!elif_cond) {
                parser_error(parser, "Expected expression in else-if condition");
                return NULL;
            }
            
            if (!match(parser, TOK_RPAREN)) {
                parser_error(parser, "Expected ')' after else-if condition");
                ast_expression_free(elif_cond);
                return NULL;
            }
            
            if (!check(parser, TOK_LBRACE)) {
                parser_error(parser, "Else-if statement body must be a block (use {...})");
                ast_expression_free(elif_cond);
                return NULL;
            }
            
            ASTBlock elif_body = *parse_block_stmt(parser);
            
            ASTElseIfClause* elif_clause = ast_else_if_create(elif_cond, elif_body, location);
            *else_if_tail = elif_clause;
            else_if_tail = &elif_clause->next;
            /* Continue loop to check for more else-if or final else */
        } else {
            /* This is the final else block (no condition) */
            if (!check(parser, TOK_LBRACE)) {
                parser_error(parser, "Else statement body must be a block (use {...})");
                return NULL;
            }
            
            else_body = parse_block_stmt(parser);
            /* Break out of loop - final else consumes no more clauses */
            break;
        }
    }
    
    ASTStatement* stmt = ast_statement_create(STMT_IF, location);
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_body = *then_body;
    xfree(then_body);
    stmt->as.if_stmt.else_if_chain = else_if_chain;
    stmt->as.if_stmt.else_body = else_body;
    
    return stmt;
}

/* Parse a while statement */
static ASTStatement* parse_while_statement(Parser* parser) {
    SourceLocation location = current_token(parser).location;
    advance(parser);  /* consume 'while' */
    
    if (!match(parser, TOK_LPAREN)) {
        parser_error(parser, "Expected '(' after 'while'");
        return NULL;
    }
    
    ASTExpression* condition = parse_expression(parser);
    if (!condition) {
        parser_error(parser, "Expected expression in while condition");
        return NULL;
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser_error(parser, "Expected ')' after while condition");
        return NULL;
    }
    
    /* Require block body */
    if (!check(parser, TOK_LBRACE)) {
        parser_error(parser, "While statement body must be a block (use {...})");
        ast_expression_free(condition);
        return NULL;
    }
    
    ASTBlock* body = parse_block_stmt(parser);
    
    ASTStatement* stmt = ast_statement_create(STMT_WHILE, location);
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = *body;
    xfree(body);
    
    return stmt;
}

/* Parse a for statement: for(init; condition; update) { body } */
static ASTStatement* parse_for_statement(Parser* parser) {
    SourceLocation location = current_token(parser).location;
    advance(parser);  /* consume 'for' */
    
    if (!match(parser, TOK_LPAREN)) {
        parser_error(parser, "Expected '(' after 'for'");
        return NULL;
    }
    
    /* Parse optional init (can be variable declaration or expression) */
    ASTStatement* init = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        /* Check if it's a variable declaration */
        Token token = current_token(parser);
        if (token.type == TOK_I8 || token.type == TOK_I16 || token.type == TOK_I32 ||
            token.type == TOK_I64 || token.type == TOK_U8 || token.type == TOK_U16 ||
            token.type == TOK_U32 || token.type == TOK_U64 || token.type == TOK_BOOL) {
            
            /* Variable declaration in for init */
            init = parse_statement(parser);
            /* Don't consume semicolon here - parse_statement for var decl already did */
        } else {
            /* Expression statement */
            ASTExpression* expr = parse_expression(parser);
            if (!expr) {
                parser_error(parser, "Expected expression in for init");
                return NULL;
            }
            
            init = ast_statement_create(STMT_EXPR, location);
            init->as.expr_stmt.expr = expr;
            
            if (!match(parser, TOK_SEMICOLON)) {
                parser_error(parser, "Expected ';' after for init");
                return NULL;
            }
        }
    } else {
        advance(parser);  /* consume the semicolon */
    }
    
    /* Parse optional condition */
    ASTExpression* condition = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        condition = parse_expression(parser);
        if (!condition) {
            parser_error(parser, "Expected expression in for condition");
            return NULL;
        }
    }
    
    if (!match(parser, TOK_SEMICOLON)) {
        parser_error(parser, "Expected ';' after for condition");
        return NULL;
    }
    
    /* Parse optional update */
    ASTExpression* update = NULL;
    if (!check(parser, TOK_RPAREN)) {
        update = parse_expression(parser);
        if (!update) {
            parser_error(parser, "Expected expression in for update");
            return NULL;
        }
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser_error(parser, "Expected ')' after for clauses");
        return NULL;
    }
    
    /* Require block body */
    if (!check(parser, TOK_LBRACE)) {
        parser_error(parser, "For statement body must be a block (use {...})");
        if (init) ast_statement_free(init);
        if (condition) ast_expression_free(condition);
        if (update) ast_expression_free(update);
        return NULL;
    }
    
    ASTBlock* body = parse_block_stmt(parser);
    
    ASTStatement* stmt = ast_statement_create(STMT_FOR, location);
    stmt->as.for_stmt.init = init;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.update = update;
    stmt->as.for_stmt.body = *body;
    xfree(body);
    
     return stmt;
}

/* Helper: Generate a descriptive name for an expression (for dbg output) */
static char* extract_expression_name(const ASTExpression* expr) {
    char buffer[256];
    
    if (!expr) {
        return xstrdup("expr");
    }
    
    switch (expr->type) {
        case EXPR_VARIABLE:
            return xstrdup(expr->as.variable.name);
        
        case EXPR_LITERAL: {
            if (expr->as.literal.type == LITERAL_INT) {
                snprintf(buffer, sizeof(buffer), "%ld", expr->as.literal.value.int_value);
            } else if (expr->as.literal.type == LITERAL_BOOL) {
                snprintf(buffer, sizeof(buffer), "%s", expr->as.literal.value.bool_value ? "true" : "false");
            } else {
                snprintf(buffer, sizeof(buffer), "literal");
            }
            return xstrdup(buffer);
        }
        
        case EXPR_BINARY_OP: {
            const ASTBinaryOp* binop = &expr->as.binary_op;
            const char* op_str = "?";
            
            switch (binop->op) {
                case BINOP_ADD: op_str = "+"; break;
                case BINOP_SUB: op_str = "-"; break;
                case BINOP_MUL: op_str = "*"; break;
                case BINOP_DIV: op_str = "/"; break;
                case BINOP_MOD: op_str = "%"; break;
                case BINOP_EQ: op_str = "=="; break;
                case BINOP_NE: op_str = "!="; break;
                case BINOP_LT: op_str = "<"; break;
                case BINOP_LE: op_str = "<="; break;
                case BINOP_GT: op_str = ">"; break;
                case BINOP_GE: op_str = ">="; break;
                case BINOP_AND: op_str = "&&"; break;
                case BINOP_OR: op_str = "||"; break;
                case BINOP_ASSIGN: op_str = "="; break;
                default: op_str = "?"; break;
            }
            
            snprintf(buffer, sizeof(buffer), "expr(%s)", op_str);
            return xstrdup(buffer);
        }
        
        case EXPR_UNARY_OP: {
            const ASTUnaryOp* unop = &expr->as.unary_op;
            const char* op_str = "?";
            
            switch (unop->op) {
                case UNOP_NEG: op_str = "-"; break;
                case UNOP_NOT: op_str = "!"; break;
                default: op_str = "?"; break;
            }
            
            snprintf(buffer, sizeof(buffer), "%sexpr", op_str);
            return xstrdup(buffer);
        }
        
        case EXPR_FUNCTION_CALL: {
            snprintf(buffer, sizeof(buffer), "%s()", expr->as.function_call.function_name);
            return xstrdup(buffer);
        }
        
        default:
            return xstrdup("expr");
    }
}

/* Parse a dbg statement: dbg(expr1, expr2, ...) */
static ASTStatement* parse_dbg_statement(Parser* parser) {
    SourceLocation location = current_token(parser).location;
    advance(parser);  /* consume 'dbg' */
    
    if (!match(parser, TOK_LPAREN)) {
        parser_error(parser, "Expected '(' after dbg");
        return NULL;
    }
    
    /* Parse arguments */
    char** arg_names = xmalloc(sizeof(char*) * 32);  /* Max 32 arguments */
    ASTExpression* arguments = xmalloc(sizeof(ASTExpression) * 32);
    int argument_count = 0;
    
    while (!check(parser, TOK_RPAREN) && argument_count < 32) {
        /* Parse expression */
        ASTExpression* expr = parse_expression(parser);
        if (!expr) {
            parser_error(parser, "Expected expression in dbg");
            xfree(arg_names);
            xfree(arguments);
            return NULL;
        }
        
        /* Extract argument name: if it's a simple variable, use the variable name
           For complex expressions, generate a descriptive name */
        char* arg_name = extract_expression_name(expr);
        arg_names[argument_count] = arg_name;
        arguments[argument_count] = *expr;
        xfree(expr);
        argument_count++;
        
        if (!check(parser, TOK_RPAREN)) {
            if (!match(parser, TOK_COMMA)) {
                parser_error(parser, "Expected ',' or ')' in dbg");
                xfree(arg_names);
                xfree(arguments);
                return NULL;
            }
        }
    }
    
    if (!match(parser, TOK_RPAREN)) {
        parser_error(parser, "Expected ')' after dbg arguments");
        xfree(arg_names);
        xfree(arguments);
        return NULL;
    }
    
    if (!match(parser, TOK_SEMICOLON)) {
        parser_error(parser, "Expected ';' after dbg statement");
        xfree(arg_names);
        xfree(arguments);
        return NULL;
    }
    
    ASTStatement* stmt = ast_statement_create(STMT_DBG, location);
    stmt->as.dbg_stmt.arg_names = arg_names;
    stmt->as.dbg_stmt.arguments = arguments;
    stmt->as.dbg_stmt.argument_count = argument_count;
    stmt->as.dbg_stmt.location = location;
    
    return stmt;
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
    
    /* Control flow statements */
    if (token.type == TOK_IF) {
        return parse_if_statement(parser);
    }
    
    if (token.type == TOK_WHILE) {
        return parse_while_statement(parser);
    }
    
    if (token.type == TOK_FOR) {
        return parse_for_statement(parser);
    }
    
    /* Debug statement */
    if (token.type == TOK_DBG) {
        return parse_dbg_statement(parser);
    }
    
    /* Bare block statement */
    if (token.type == TOK_LBRACE) {
        SourceLocation location = token.location;
        ASTBlock block;
        parse_block(parser, &block);
        
        ASTStatement* stmt = ast_statement_create(STMT_BLOCK, location);
        stmt->as.block_stmt.block = block;
        stmt->as.block_stmt.location = location;
        return stmt;
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
        /* Stop if we hit a lexer error token */
        if (check(parser, TOK_ERROR)) {
            break;
        }
        
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
             xfree(stmt);
        } else {
            /* Statement failed to parse - skip one token for error recovery */
            advance(parser);
        }
    }
    
    if (!match(parser, TOK_RBRACE)) {
        parser_error(parser, "Expected '}'");
    }
}

/* Extract the base name (without extension) from a file path.
 * For example: "./foo.csm" -> "foo", "./dir/bar.csm" -> "bar"
 * Returns newly allocated string that must be freed by caller. */
static char* extract_base_name(const char* file_path) {
    const char* slash = strrchr(file_path, '/');
    const char* base = slash ? slash + 1 : file_path;
    
    const char* dot = strchr(base, '.');
    int name_len = dot ? (int)(dot - base) : (int)strlen(base);
    
    return xstrndup(base, name_len);
}

/* Parse an import statement - fills in provided struct */
static int parse_import(Parser* parser, ASTImportStatement* out_import) {
    int error_count_before = parser->errors->error_count;
    
    /* Expect: # import name1, name2, ... from "path" ;
     * OR: # import "path" ; (shorthand, expands to #import basename from "path") */
    if (!match(parser, TOK_HASH)) {
        parser_error(parser, "Expected '#' for import statement");
        return 0;
    }
    
    if (!match(parser, TOK_IMPORT)) {
        parser_error(parser, "Expected 'import' keyword after '#'");
        return 0;
    }
    
    SourceLocation location = current_token(parser).location;
    char** imported_names = xmalloc(10 * sizeof(char*));
    int name_count = 0;
    int name_capacity = 10;
    char* file_path = NULL;
    
    /* Check for shorthand syntax: #import "path" */
    if (check(parser, TOK_STRING)) {
        Token path_token = current_token(parser);
        /* Extract string content (remove quotes) */
        const char* quoted_path = path_token.lexeme;
        int path_len = path_token.lexeme_len;
        if (path_len >= 2 && quoted_path[0] == '"' && quoted_path[path_len - 1] == '"') {
            path_len -= 2;
            quoted_path++;
        }
        file_path = xstrndup(quoted_path, path_len);
        advance(parser);
        
        /* Extract base name from file path and use as imported name */
        imported_names[0] = extract_base_name(file_path);
        name_count = 1;
    } else {
        /* Standard syntax: #import name1, name2, ... from "path" */
        while (1) {
            if (!check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected identifier in import list");
                for (int i = 0; i < name_count; i++) {
                    xfree(imported_names[i]);
                }
                xfree(imported_names);
                return 0;
            }
            
            Token name_token = current_token(parser);
            char* name = xstrndup(name_token.lexeme, name_token.lexeme_len);
            advance(parser);
            
            if (name_count >= name_capacity) {
                name_capacity *= 2;
                imported_names = xrealloc(imported_names, name_capacity * sizeof(char*));
            }
            imported_names[name_count++] = name;
            
            /* Check for comma or move to 'from' */
            if (!match(parser, TOK_COMMA)) {
                break;
            }
        }
        
        if (!match(parser, TOK_FROM)) {
            parser_error(parser, "Expected 'from' after import names");
            for (int i = 0; i < name_count; i++) {
                xfree(imported_names[i]);
            }
            xfree(imported_names);
            return 0;
        }
        
        if (!check(parser, TOK_STRING)) {
            parser_error(parser, "Expected string literal for file path");
            for (int i = 0; i < name_count; i++) {
                xfree(imported_names[i]);
            }
            xfree(imported_names);
            return 0;
        }
        
        Token path_token = current_token(parser);
        /* Extract string content (remove quotes) */
        const char* quoted_path = path_token.lexeme;
        int path_len = path_token.lexeme_len;
        if (path_len >= 2 && quoted_path[0] == '"' && quoted_path[path_len - 1] == '"') {
            path_len -= 2;
            quoted_path++;
        }
        file_path = xstrndup(quoted_path, path_len);
        advance(parser);
    }
    
    out_import->imported_names = imported_names;
    out_import->name_count = name_count;
    out_import->file_path = file_path;
    out_import->location = location;
    
    return parser->errors->error_count == error_count_before;
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
    
    /* First, parse all import statements */
    int import_capacity = 10;
    program->imports = xmalloc(import_capacity * sizeof(ASTImportStatement));
    
    while (check(parser, TOK_HASH)) {
        if (program->import_count >= import_capacity) {
            import_capacity *= 2;
            program->imports = xrealloc(program->imports, import_capacity * sizeof(ASTImportStatement));
        }
        
        ASTImportStatement temp_import = {0};
        if (parse_import(parser, &temp_import)) {
            program->imports[program->import_count] = temp_import;
            program->import_count++;
        } else {
            /* Import failed to parse - clean up and skip to next */
            ast_import_free_contents(&temp_import);
            advance(parser); /* Skip the problematic token */
        }
    }
    
    /* Then, parse function definitions */
    int func_capacity = 10;
    program->functions = xmalloc(func_capacity * sizeof(ASTFunctionDef));
    
    while (!check(parser, TOK_EOF)) {
        /* Stop if we encounter a lexer error */
        if (check(parser, TOK_ERROR)) {
            break;
        }
        
        if (program->function_count >= func_capacity) {
            func_capacity *= 2;
            program->functions = xrealloc(program->functions, func_capacity * sizeof(ASTFunctionDef));
        }
        
        ASTFunctionDef temp_func = {0};
        if (parse_function(parser, &temp_func)) {
            program->functions[program->function_count] = temp_func;
            program->function_count++;
        } else {
            /* Function failed to parse - clean up the partial function and skip to next */
            ast_function_free(&temp_func);
            advance(parser); /* Skip the problematic token */
        }
    }
    
    return program;
}
