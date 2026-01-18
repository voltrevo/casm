#ifndef LEXER_H
#define LEXER_H

#include "utils.h"

/* Token types */
typedef enum {
    /* Literals and identifiers */
    TOK_INT_LITERAL,
    TOK_IDENTIFIER,
    
    /* Keywords */
    TOK_INT,
    TOK_VOID,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    
    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,        /* * */
    TOK_SLASH,       /* / */
    TOK_PERCENT,     /* % */
    TOK_ASSIGN,      /* = */
    TOK_EQ,          /* == */
    TOK_NE,          /* != */
    TOK_LT,          /* < */
    TOK_GT,          /* > */
    TOK_LE,          /* <= */
    TOK_GE,          /* >= */
    TOK_AND,         /* && */
    TOK_OR,          /* || */
    TOK_NOT,         /* ! */
    
    /* Delimiters */
    TOK_LPAREN,      /* ( */
    TOK_RPAREN,      /* ) */
    TOK_LBRACE,      /* { */
    TOK_RBRACE,      /* } */
    TOK_SEMICOLON,   /* ; */
    TOK_COMMA,       /* , */
    
    /* Special */
    TOK_EOF,
    TOK_ERROR,
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    const char* lexeme;     /* Points to source string */
    int lexeme_len;
    SourceLocation location;
    /* For literals */
    long int_value;
} Token;

/* Lexer state */
typedef struct {
    const char* source;
    int source_len;
    int current;            /* Current position in source */
    int line;
    int column;
    int line_start;         /* Offset of start of current line */
    Token current_token;
} Lexer;

/* Lexer functions */
Lexer* lexer_create(const char* source);
void lexer_free(Lexer* lexer);

Token lexer_next_token(Lexer* lexer);
Token lexer_peek_token(Lexer* lexer);

const char* token_type_name(TokenType type);

#endif /* LEXER_H */
