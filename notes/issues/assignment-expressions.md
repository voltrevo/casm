# Suspected Bug: Assignment Expressions in Codegen

## Summary

There is a likely **code generation bug** when **assignment expressions** are used as **sub-expressions** (e.g., operands of binary/unary operators). The codegen path appears to special-case assignment without ensuring proper parenthesization, which can produce **invalid or semantically incorrect C code**.

---

## Why This Is a Bug

In C, assignment has **lower precedence** than most operators. When an assignment is nested inside another expression, it **must be parenthesized** to preserve meaning.

### Examples

| Source              | Incorrect Output | Correct Output      |
| ------------------- | ---------------- | ------------------- |
| `(x = 1) + (x = 2)` | `x = 1 + x = 2`  | `(x = 1) + (x = 2)` |
| `!(x = 0)`          | `!x = 0`         | `!(x = 0)`          |

If codegen emits assignment expressions without parentheses when nested, the generated C becomes invalid or changes semantics.

---

## Tests That Catch the Bug

Add the following test file to ensure assignment expressions are correctly parenthesized in nested contexts.

### `tests/test_codegen.c`

```c
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

    ASSERT_EQ(p->errors->error_count, 0);

    FILE* f = tmpfile();
    ASSERT_TRUE(f != NULL);

    CodegenResult r = codegen_program(prog, f);
    ASSERT_TRUE(r.success);

    fflush(f);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    ASSERT_TRUE(sz >= 0);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc((size_t)sz + 1);
    ASSERT_TRUE(buf != NULL);

    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';

    fclose(f);
    if (r.error_msg) free(r.error_msg);

    ast_program_free(prog);
    parser_free(p);

    return buf;
}

static int contains(const char* haystack, const char* needle) {
    return strstr(haystack, needle) != NULL;
}

static void test_assignment_as_add_operand_is_parenthesized(void) {
    const char* src =
        "i32 main() {\n"
        "    i32 x;\n"
        "    return (x = 1) + (x = 2);\n"
        "}\n";

    char* c = generate_c_from_source(src);

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

    ASSERT_FALSE(contains(c, "!x = 0"));
    ASSERT_TRUE(contains(c, "!(x = 0)"));

    free(c);
}

int main(void) {
    RUN_TEST(test_assignment_as_add_operand_is_parenthesized);
    RUN_TEST(test_assignment_under_unary_is_parenthesized);
    PRINT_SUMMARY();
}
```

---

## Build Integration

### Makefile additions

```make
CODEGEN_TEST_SOURCES = tests/test_codegen.c src/lexer.c src/parser.c src/ast.c src/utils.c src/types.c src/semantics.c src/codegen.c
CODEGEN_TEST_BINARY  = $(BIN_DIR)/test_codegen

test: build-debug $(TEST_BINARY) $(SEMANTICS_TEST_BINARY) $(CODEGEN_TEST_BINARY)
	./run_tests.sh

$(CODEGEN_TEST_BINARY): $(BIN_DIR) $(CODEGEN_TEST_SOURCES)
	$(CC) $(CFLAGS_DEBUG) -o $(CODEGEN_TEST_BINARY) $(CODEGEN_TEST_SOURCES) $(LDFLAGS)
```

### `run_tests.sh`

Ensure `./bin/test_codegen` is executed alongside existing test binaries.

---

## Expected Outcome

* These tests **fail on current code**.
* They pass once codegen **wraps assignment expressions in parentheses** whenever the assignment node is used as a sub-expression.

---

## Minimal Fix (Conceptual)

In `emit_expression()`:

* Treat assignment as **lowest-precedence**.
* If the parent context expects an expression (binary/unary operand), **emit parentheses around the assignment**.

This can be implemented without a full precedence table by tracking whether the caller requires parentheses.

---

## Rationale

This is a classic codegen pitfall and exactly the kind of issue that:

* Survives basic testing
* Breaks real programs
* Is cheap to lock down with regression tests

Catching it now is the right move.
