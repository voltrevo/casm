#include "lexer.h"
#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

static int is_at_end(Lexer* lexer) {
    return lexer->current >= lexer->source_len;
}

static char peek(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current];
}

static char peek_next(Lexer* lexer) {
    if (lexer->current + 1 >= lexer->source_len) return '\0';
    return lexer->source[lexer->current + 1];
}

static char advance(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    char c = lexer->source[lexer->current];
    lexer->current++;
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 0;
        lexer->line_start = lexer->current;
    } else {
        lexer->column++;
    }
    
    return c;
}

static void skip_whitespace(Lexer* lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance(lexer);
        } else if (c == '/' && peek_next(lexer) == '/') {
            /* Single-line comment */
            advance(lexer);
            advance(lexer);
            while (!is_at_end(lexer) && peek(lexer) != '\n') {
                advance(lexer);
            }
        } else if (c == '/' && peek_next(lexer) == '*') {
            /* Multi-line comment */
            advance(lexer);
            advance(lexer);
            while (!is_at_end(lexer)) {
                if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                    advance(lexer);
                    advance(lexer);
                    break;
                }
                advance(lexer);
            }
        } else {
            break;
        }
    }
}

static int is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

static int is_identifier_cont(char c) {
    return isalnum(c) || c == '_';
}

static Token make_token(Lexer* lexer, TokenType type, int start_offset, int length) {
    Token token;
    token.type = type;
    token.lexeme = lexer->source + start_offset;
    token.lexeme_len = length;
    token.location.offset = start_offset;
    token.location.line = lexer->line;
    token.location.column = start_offset - lexer->line_start;
    token.int_value = 0;
    return token;
}

static Token scan_number(Lexer* lexer) {
    int start = lexer->current;
    int start_col = lexer->column;
    int line = lexer->line;
    
    while (!is_at_end(lexer) && isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    int length = lexer->current - start;
    Token token = make_token(lexer, TOK_INT_LITERAL, start, length);
    token.location.line = line;
    token.location.column = start_col;
    
    /* Parse the integer value */
    sscanf(token.lexeme, "%ld", &token.int_value);
    
    return token;
}

static Token scan_identifier(Lexer* lexer) {
    int start = lexer->current;
    int start_col = lexer->column;
    int line = lexer->line;
    
    while (!is_at_end(lexer) && is_identifier_cont(peek(lexer))) {
        advance(lexer);
    }
    
    int length = lexer->current - start;
    const char* text = lexer->source + start;
    
    /* Check for keywords */
    TokenType type = TOK_IDENTIFIER;
    if (length == 3 && strncmp(text, "int", 3) == 0) type = TOK_INT;
    else if (length == 4 && strncmp(text, "void", 4) == 0) type = TOK_VOID;
    else if (length == 2 && strncmp(text, "if", 2) == 0) type = TOK_IF;
    else if (length == 4 && strncmp(text, "else", 4) == 0) type = TOK_ELSE;
    else if (length == 5 && strncmp(text, "while", 5) == 0) type = TOK_WHILE;
    else if (length == 3 && strncmp(text, "for", 3) == 0) type = TOK_FOR;
    else if (length == 6 && strncmp(text, "return", 6) == 0) type = TOK_RETURN;
    
    Token token = make_token(lexer, type, start, length);
    token.location.line = line;
    token.location.column = start_col;
    
    return token;
}

static Token next_token_impl(Lexer* lexer) {
    skip_whitespace(lexer);
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOK_EOF, lexer->current, 0);
    }
    
    int start = lexer->current;
    int start_col = lexer->column;
    int line = lexer->line;
    
    char c = advance(lexer);
    
    /* Single character tokens */
    switch (c) {
        case '(': return make_token(lexer, TOK_LPAREN, start, 1);
        case ')': return make_token(lexer, TOK_RPAREN, start, 1);
        case '{': return make_token(lexer, TOK_LBRACE, start, 1);
        case '}': return make_token(lexer, TOK_RBRACE, start, 1);
        case ';': return make_token(lexer, TOK_SEMICOLON, start, 1);
        case ',': return make_token(lexer, TOK_COMMA, start, 1);
        case '+': return make_token(lexer, TOK_PLUS, start, 1);
        case '-': return make_token(lexer, TOK_MINUS, start, 1);
        case '*': return make_token(lexer, TOK_STAR, start, 1);
        case '/': return make_token(lexer, TOK_SLASH, start, 1);
        case '%': return make_token(lexer, TOK_PERCENT, start, 1);
    }
    
    /* Multi-character operators */
    if (c == '=') {
        if (peek(lexer) == '=') {
            advance(lexer);
            return make_token(lexer, TOK_EQ, start, 2);
        }
        return make_token(lexer, TOK_ASSIGN, start, 1);
    }
    
    if (c == '!') {
        if (peek(lexer) == '=') {
            advance(lexer);
            return make_token(lexer, TOK_NE, start, 2);
        }
        return make_token(lexer, TOK_NOT, start, 1);
    }
    
    if (c == '<') {
        if (peek(lexer) == '=') {
            advance(lexer);
            return make_token(lexer, TOK_LE, start, 2);
        }
        return make_token(lexer, TOK_LT, start, 1);
    }
    
    if (c == '>') {
        if (peek(lexer) == '=') {
            advance(lexer);
            return make_token(lexer, TOK_GE, start, 2);
        }
        return make_token(lexer, TOK_GT, start, 1);
    }
    
    if (c == '&') {
        if (peek(lexer) == '&') {
            advance(lexer);
            Token token = make_token(lexer, TOK_AND, start, 2);
            token.location.line = line;
            token.location.column = start_col;
            return token;
        }
        Token token = make_token(lexer, TOK_ERROR, start, 1);
        token.location.line = line;
        token.location.column = start_col;
        return token;
    }
    
    if (c == '|') {
        if (peek(lexer) == '|') {
            advance(lexer);
            Token token = make_token(lexer, TOK_OR, start, 2);
            token.location.line = line;
            token.location.column = start_col;
            return token;
        }
        Token token = make_token(lexer, TOK_ERROR, start, 1);
        token.location.line = line;
        token.location.column = start_col;
        return token;
    }
    
    /* Numbers */
    if (isdigit(c)) {
        lexer->current--;  /* Back up to re-scan the number */
        lexer->column--;
        return scan_number(lexer);
    }
    
    /* Identifiers and keywords */
    if (is_identifier_start(c)) {
        lexer->current--;  /* Back up to re-scan the identifier */
        lexer->column--;
        return scan_identifier(lexer);
    }
    
    /* Unknown character */
    Token token = make_token(lexer, TOK_ERROR, start, 1);
    token.location.line = line;
    token.location.column = start_col;
    return token;
}

Lexer* lexer_create(const char* source) {
    Lexer* lexer = xmalloc(sizeof(Lexer));
    lexer->source = source;
    lexer->source_len = strlen(source);
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 0;
    lexer->line_start = 0;
    lexer->current_token.type = TOK_EOF;
    lexer->current_token.lexeme = "";
    lexer->current_token.lexeme_len = 0;
    return lexer;
}

void lexer_free(Lexer* lexer) {
    xfree(lexer);
}

Token lexer_next_token(Lexer* lexer) {
    lexer->current_token = next_token_impl(lexer);
    return lexer->current_token;
}

Token lexer_peek_token(Lexer* lexer) {
    /* This is a simple peek that doesn't handle multiple peeks properly */
    /* For now, we'll just return the current token */
    return lexer->current_token;
}

const char* token_type_name(TokenType type) {
    switch (type) {
        case TOK_INT_LITERAL: return "INT_LITERAL";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_INT: return "INT";
        case TOK_VOID: return "VOID";
        case TOK_IF: return "IF";
        case TOK_ELSE: return "ELSE";
        case TOK_WHILE: return "WHILE";
        case TOK_FOR: return "FOR";
        case TOK_RETURN: return "RETURN";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_STAR: return "STAR";
        case TOK_SLASH: return "SLASH";
        case TOK_PERCENT: return "PERCENT";
        case TOK_ASSIGN: return "ASSIGN";
        case TOK_EQ: return "EQ";
        case TOK_NE: return "NE";
        case TOK_LT: return "LT";
        case TOK_GT: return "GT";
        case TOK_LE: return "LE";
        case TOK_GE: return "GE";
        case TOK_AND: return "AND";
        case TOK_OR: return "OR";
        case TOK_NOT: return "NOT";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_SEMICOLON: return "SEMICOLON";
        case TOK_COMMA: return "COMMA";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
    }
    return "UNKNOWN";
}
