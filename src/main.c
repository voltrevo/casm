#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "codegen.h"
#include "codegen_wat.h"
#include "module_loader.h"
#include "name_allocator.h"
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

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--target=c|wat] <source.csm>\n", argv[0]);
        fprintf(stderr, "Default target: wat\n");
        return 1;
    }
    
    /* Parse command line arguments */
    const char* source_file = NULL;
    const char* target = "wat";  /* Default target */
    const char* output_file = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--target=", 9) == 0) {
            target = argv[i] + 9;
        } else if (strncmp(argv[i], "--output=", 9) == 0) {
            output_file = argv[i] + 9;
        } else if (argv[i][0] != '-') {
            source_file = argv[i];
        }
    }
    
    if (!source_file) {
        fprintf(stderr, "Error: No source file specified\n");
        return 1;
    }
    
    /* Validate target */
    if (strcmp(target, "c") != 0 && strcmp(target, "wat") != 0) {
        fprintf(stderr, "Error: Invalid target '%s'. Use 'c' or 'wat'.\n", target);
        return 1;
    }
    
    char* source = read_file(source_file);
    
    /* Try to load with module system first (handles imports) */
    char* error_msg = NULL;
    ASTProgram* program = build_complete_ast(source_file, &error_msg);
    
    if (!program) {
        if (error_msg) {
            fprintf(stderr, "Error: %s\n", error_msg);
            xfree(error_msg);
        } else {
            fprintf(stderr, "Error: Failed to build AST\n");
        }
        xfree(source);
        return 1;
    }
    
    /* If the program has no imports, we still need to do basic semantic checking */
    if (program->import_count == 0) {
        /* No imports - verify the local file parsed correctly */
        Parser* parser = parser_create(source);
        if (parser->errors->error_count > 0) {
            error_list_print(parser->errors, source_file);
            parser_free(parser);
            ast_program_free_merged(program);
            xfree(source);
            return 1;
        }
        parser_free(parser);
    }
    
    /* Semantic analysis */
    SymbolTable* table = symbol_table_create();
    SemanticErrorList* sem_errors = semantic_error_list_create();
    
    if (!analyze_program(program, table, sem_errors)) {
        semantic_error_list_print(sem_errors, source_file);
        semantic_error_list_free(sem_errors);
        symbol_table_free(table);
        ast_program_free_merged(program);
        xfree(source);
        return 1;
    }
    
    /* Allocate names to handle symbol deduplication (Phase 5) */
    NameAllocator* allocator = name_allocator_create(program);
    if (allocator) {
        name_allocator_apply(allocator, program);
    }
    
    /* Code generation */
    if (strcmp(target, "c") == 0) {
        /* Generate output filename if not specified */
        char output_buffer[512];
        if (!output_file) {
            /* Always output to out.c */
            snprintf(output_buffer, sizeof(output_buffer), "out.c");
            output_file = output_buffer;
        }
        
        FILE* out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Could not open output file '%s' for writing\n", output_file);
            semantic_error_list_free(sem_errors);
            symbol_table_free(table);
            ast_program_free_merged(program);
            xfree(source);
            return 1;
        }
        
        CodegenResult result = codegen_program(program, out, source_file);
        fclose(out);
        
        if (!result.success) {
            fprintf(stderr, "Error: Code generation failed: %s\n", result.error_msg);
            semantic_error_list_free(sem_errors);
            symbol_table_free(table);
            ast_program_free_merged(program);
            xfree(source);
            return 1;
        }
    } else if (strcmp(target, "wat") == 0) {
        /* Generate output filename if not specified */
        char output_buffer[512];
        if (!output_file) {
            /* Always output to out.wat */
            snprintf(output_buffer, sizeof(output_buffer), "out.wat");
            output_file = output_buffer;
        }
        
        FILE* out = fopen(output_file, "w");
        if (!out) {
            fprintf(stderr, "Error: Could not open output file '%s' for writing\n", output_file);
            semantic_error_list_free(sem_errors);
            symbol_table_free(table);
            ast_program_free_merged(program);
            xfree(source);
            return 1;
        }
        
        CodegenWatResult result = codegen_wat_program(program, out, source_file);
        fclose(out);
        
        if (!result.success) {
            fprintf(stderr, "Error: WAT code generation failed: %s\n", result.error_msg);
            semantic_error_list_free(sem_errors);
            symbol_table_free(table);
            ast_program_free_merged(program);
            xfree(source);
            return 1;
        }
        
        printf("Generated WAT code: %s\n", output_file);
    }
    
    semantic_error_list_free(sem_errors);
    symbol_table_free(table);
    if (allocator) {
        name_allocator_free(allocator);
    }
    ast_program_free_merged(program);
    xfree(source);
    return 0;
}
