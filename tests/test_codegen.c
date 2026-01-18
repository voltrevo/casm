#include "test_harness.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "codegen.h"
#include "ast.h"

/* Generate C code into a heap string. Caller must free(). */
static char* generate_c_from_source(const char* src) {
    Parser* p = parser_create(src);
    ASTProgram* prog = parser_parse(p);

    if (p->errors->error_count != 0) {
        parser_free(p);
        ast_program_free(prog);
        return NULL;
    }

    FILE* f = tmpfile();
    if (f == NULL) {
        parser_free(p);
        ast_program_free(prog);
        return NULL;
    }

    CodegenResult r = codegen_program(prog, f);
    
    if (!r.success) {
        fclose(f);
        if (r.error_msg) free(r.error_msg);
        parser_free(p);
        ast_program_free(prog);
        return NULL;
    }

    fflush(f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        if (r.error_msg) free(r.error_msg);
        parser_free(p);
        ast_program_free(prog);
        return NULL;
    }

    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (buf == NULL) {
        fclose(f);
        if (r.error_msg) free(r.error_msg);
        parser_free(p);
        ast_program_free(prog);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';

    fclose(f);
    if (r.error_msg) free(r.error_msg);

    ast_program_free(prog);
    parser_free(p);

    return buf;
}

static int contains(const char* haystack, const char* needle) {
    return haystack != NULL && strstr(haystack, needle) != NULL;
}

static void test_assignment_as_add_operand_is_parenthesized(void) {
    const char* src =
        "i32 main() {\n"
        "    i32 x;\n"
        "    return (x = 1) + (x = 2);\n"
        "}\n";

    char* c = generate_c_from_source(src);

    ASSERT_TRUE(c != NULL);
    ASSERT_FALSE(contains(c, "x = 1 + x = 2"));
    ASSERT_TRUE(contains(c, "(x = 1)"));
    ASSERT_TRUE(contains(c, "(x = 2)"));

    free(c);
}

static void test_assignment_under_unary_is_parenthesized(void) {
    const char* src =
        "i32 main() {\n"
        "    i32 x;\n"
        "    return !(x = 0);\n"
        "}\n";

    char* c = generate_c_from_source(src);

    ASSERT_TRUE(c != NULL);
    ASSERT_FALSE(contains(c, "!x = 0"));
    ASSERT_TRUE(contains(c, "!(x = 0)"));

    free(c);
}

static void test_nested_block_emits_braces(void) {
    const char* src =
        "i32 main() {\n"
        "  { i32 x = 1; }\n"
        "  { i32 x = 2; }\n"
        "  return 0;\n"
        "}\n";

    char* c = generate_c_from_source(src);

    ASSERT_TRUE(c != NULL);
    
    /* If braces are missing, both declarations would be in same scope */
    ASSERT_FALSE(contains(c, "int32_t x = 1;\n    int32_t x = 2;"));

    /* Positive signal: braces should separate the declarations */
    ASSERT_TRUE(contains(c, "int32_t x = 1;"));
    ASSERT_TRUE(contains(c, "int32_t x = 2;"));

    free(c);
}

int main(void) {
    RUN_TEST(test_assignment_as_add_operand_is_parenthesized);
    RUN_TEST(test_assignment_under_unary_is_parenthesized);
    RUN_TEST(test_nested_block_emits_braces);
    PRINT_SUMMARY();
}
