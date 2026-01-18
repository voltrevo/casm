#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "types.h"

/* Result of code generation */
typedef struct {
    int success;        /* 1 if generation succeeded, 0 if failed */
    char* error_msg;    /* Error message if failed (NULL if success) */
} CodegenResult;

/* Generate C code from AST and write to file */
CodegenResult codegen_program(ASTProgram* program, FILE* output);

#endif /* CODEGEN_H */
