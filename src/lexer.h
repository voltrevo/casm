#ifndef LEXER_H
#define LEXER_H

#include "utils.h"

/* Token types */
typedef enum {
    /* Literals and identifiers */
    TOK_INT_LITERAL,
    TOK_IDENTIFIER,
    
    /* Keywords - Types */
    TOK_I8,
    TOK_I16,
    TOK_I32,
    TOK_I64,
    TOK_U8,
    TOK_U16,
    TOK_U32,
    TOK_U64,
    TOK_BOOL,
    TOK_VOID,
    
    /* Keywords - Control Flow */
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    
    /* Keywords - Literals */
    TOK_TRUE,
    TOK_FALSE,
    
    /* Keywords - Module */
    TOK_IMPORT,
    
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
