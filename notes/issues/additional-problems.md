# Additional Suspected Bugs & Regression Tests

This document captures **two additional likely bugs** identified after the initial assignment‑expression issue, along with concrete tests to detect them.

---

## 1. Codegen Bug: Nested Blocks Lose Braces (Scope Break)

### Description

When emitting statements, nested `{ ... }` blocks are flattened instead of being emitted with braces. This breaks lexical scoping and can produce **invalid C** due to variable redeclarations.

### Why This Matters

CASM allows multiple inner scopes with the same variable name:

```casm
i32 main() {
  { i32 x = 1; }
  { i32 x = 2; }
  return 0;
}
```

If braces are omitted during codegen, the generated C effectively becomes:

```c
int32_t x = 1;
int32_t x = 2; // redeclaration error
```

This is both incorrect and hard to diagnose downstream.

---

### Regression Test

Add to `tests/test_codegen.c`:

```c
static void test_nested_block_emits_braces(void) {
    const char* src =
        "i32 main() {\n"
        "  { i32 x = 1; }\n"
        "  { i32 x = 2; }\n"
        "  return 0;\n"
        "}\n";

    char* c = generate_c_from_source(src);

    /* If braces are missing, both declarations land in one scope */
    ASSERT_FALSE(contains(c, "int32_t x = 1;\n    int32_t x = 2;"));

    /* Positive signal: braces should separate the declarations */
    ASSERT_TRUE(contains(c, "int32_t x = 1;"));
    ASSERT_TRUE(contains(c, "{"));
    ASSERT_TRUE(contains(c, "}"));
    ASSERT_TRUE(contains(c, "int32_t x = 2;"));

    free(c);
}
```

---

## 2. Semantics Bug: Assignment Expression Resolves to RHS Type

### Description

In semantic analysis, assignment expressions currently resolve to the **right‑hand side type**. In C‑like languages, assignment expressions evaluate to the **left‑hand side type** (after conversion).

This affects type propagation in larger expressions and can silently introduce type mismatches.

---

### Example

```casm
i64 main() {
  i32 x;
  i64 y;
  y = 123;
  return (x = y);
}
```

Correct behavior:

* `(x = y)` resolves to `i32`

Current behavior:

* `(x = y)` resolves to `i64`

---

### Regression Test

Add to `tests/test_semantics.c`:

```c
static int test_assignment_expression_type_is_lhs(TestSuite* suite) {
    TEST_START("Assignment expression type is LHS");

    const char* source =
        "i64 main() {\n"
        "  i32 x;\n"
        "  i64 y;\n"
        "  y = 123;\n"
        "  return (x = y);\n"
        "}\n";

    Parser* parser = parser_create(source);
    ASTProgram* program = parser_parse(parser);
    ASSERT_EQ(parser->errors->error_count, 0, "Parsing should succeed");

    SymbolTable* table = symbol_table_create();
    SemanticErrorList* errors = semantic_error_list_create();

    int ok = analyze_program(program, table, errors);
    ASSERT_EQ(ok, 1, "Program should be semantically valid");
    ASSERT_EQ(errors->error_count, 0, "Should have no semantic errors");

    ASTFunctionDef* f = &program->functions[0];
    ASTStatement* last = &f->body.statements[f->body.statement_count - 1];

    ASSERT_EQ(last->type, STMT_RETURN, "Last statement should be return");
    ASSERT_EQ(last->as.return_stmt.value->as.binary_op.op, BINOP_ASSIGN,
              "Return value should be assignment");

    /* Assignment expression should resolve to LHS type (x: i32) */
    ASSERT_EQ(last->as.return_stmt.value->resolved_type, TYPE_I32,
              "Assignment expr type should be LHS type");

    semantic_error_list_free(errors);
    symbol_table_free(table);
    parser_free(parser);
    ast_program_free(program);

    TEST_PASS;
}
```

---

## Expected Outcome

* Both tests **fail on current code**.
* They pass once:

  * Codegen emits braces for nested blocks
  * Semantic analysis assigns the LHS type to assignment expressions

---

## Notes

These are both classic, high‑impact bugs that:

* Rarely show up in simple examples
* Break real programs in non‑obvious ways
* Are cheap to lock down with focused regression tests

Fixing them early meaningfully increases compiler robustness.
