#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"
#include "types.h"
#include "utils.h"

/* Forward declarations */
typedef struct SemanticError SemanticError;
typedef struct SemanticErrorList SemanticErrorList;

/* Semantic error - like ErrorNode but for semantic phase */
struct SemanticError {
    char* message;
    SourceLocation location;
};

/* List of semantic errors */
struct SemanticErrorList {
    SemanticError* errors;
    int error_count;
    int error_capacity;
};

/* Error list creation and management */
SemanticErrorList* semantic_error_list_create(void);
void semantic_error_list_free(SemanticErrorList* list);
void semantic_error_list_add(SemanticErrorList* list, const char* message, SourceLocation location);
void semantic_error_list_print(SemanticErrorList* list, const char* filename);

/* Main semantic analysis function - 2-pass analysis */
int analyze_program(ASTProgram* program, SymbolTable* table, SemanticErrorList* errors);

#endif /* SEMANTICS_H */
