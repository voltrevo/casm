#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "utils.h"

/* Error collection */
typedef struct {
    char* message;
    SourceLocation location;
} ParseError;

typedef struct {
    ParseError* errors;
    int error_count;
    int error_capacity;
} ErrorList;

ErrorList* error_list_create(void);
void error_list_free(ErrorList* errors);
void error_list_add(ErrorList* errors, const char* message, SourceLocation location);
void error_list_print(ErrorList* errors, const char* filename);

/* Parser state */
typedef struct {
    Token* tokens;
    int token_count;
    int current;
    ErrorList* errors;
} Parser;

/* Parser API */
Parser* parser_create(const char* source);
void parser_free(Parser* parser);

ASTProgram* parser_parse(Parser* parser);

/* Helper for returning statements without heap allocation */
void ast_statement_init(ASTStatement* stmt, StatementType type, SourceLocation location);

#endif /* PARSER_H */
