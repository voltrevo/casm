#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../src/module_loader.h"
#include "../src/parser.h"
#include "../src/lexer.h"
#include "../src/utils.h"

/* Test: build_complete_ast should not leak abs_main_file */
static void test_build_complete_ast_no_leak(void) {
    printf("  Test: build_complete_ast frees abs_main_file... ");
    fflush(stdout);
    
    /* Create a simple test file */
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "/tmp/test_leak_%d.csm", getpid());
    
    FILE* out = fopen(temp_path, "w");
    if (!out) {
        printf("FAIL (couldn't create temp file)\n");
        return;
    }
    fprintf(out, "i32 main() { return 0; }\n");
    fclose(out);
    
    /* Call build_complete_ast */
    char* error_msg = NULL;
    ASTProgram* prog = build_complete_ast(temp_path, &error_msg);
    
    if (!prog) {
        printf("FAIL (build_complete_ast returned NULL: %s)\n", error_msg ? error_msg : "");
        if (error_msg) xfree(error_msg);
        unlink(temp_path);
        return;
    }
    
    if (error_msg) xfree(error_msg);
    
    ast_program_free_merged(prog);
    unlink(temp_path);
    
    printf("OK\n");
}

/* Test: parser_create and parser_parse should be properly freed */
static void test_parser_no_leak(void) {
    printf("  Test: parser_create/parse properly freed... ");
    fflush(stdout);
    
    const char* source = "i32 main() { return 42; }\n";
    
    Parser* parser = parser_create(source);
    if (!parser) {
        printf("FAIL (parser_create returned NULL)\n");
        return;
    }
    
    ASTProgram* prog = parser_parse(parser);
    if (!prog) {
        printf("FAIL (parser_parse returned NULL)\n");
        parser_free(parser);
        return;
    }
    
    parser_free(parser);
    ast_program_free(prog);
    
    printf("OK\n");
}

/* Test: parser_create without full parse should not leak */
static void test_parser_early_exit_no_leak(void) {
    printf("  Test: parser early exit no leak... ");
    fflush(stdout);
    
    const char* source = "i32 main() { return 0; }\n";
    
    Parser* parser = parser_create(source);
    if (!parser) {
        printf("FAIL (parser_create returned NULL)\n");
        return;
    }
    
    /* Free without parsing */
    parser_free(parser);
    
    printf("OK\n");
}

int main(void) {
    printf("Running Memory Leak Tests...\n\n");
    
    test_parser_no_leak();
    test_parser_early_exit_no_leak();
    test_build_complete_ast_no_leak();
    
    printf("\n");
    return 0;
}
