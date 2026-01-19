#ifndef CODEGEN_WAT_H
#define CODEGEN_WAT_H

#include <stdio.h>
#include "ast.h"
#include "types.h"

/* Result of WAT code generation */
typedef struct {
    int success;        /* 1 if generation succeeded, 0 if failed */
    char* error_msg;    /* Error message if failed (NULL if success) */
} CodegenWatResult;

/* Generate WebAssembly text format from AST and write to file.
 * source_filename is used in debug output (typically the .csm filename). */
CodegenWatResult codegen_wat_program(ASTProgram* program, FILE* output, const char* source_filename);

#endif /* CODEGEN_WAT_H */
