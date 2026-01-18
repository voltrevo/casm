#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/semantics.h"
#include "../src/types.h"
#include "../src/utils.h"

/* Test utilities */
typedef struct {
    const char* name;
    int passed;
    int failed;
} TestSuite;

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        printf("  FAIL: %s (expected %d, got %d)\n", msg, b, a); \
        suite->failed++; \
        return 0; \
    } \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { \
        printf("  FAIL: %s\n", msg); \
        suite->failed++; \
        return 0; \
    } \
} while(0)

#define TEST_START(name) \
    printf("  Test: %s... ", name); \
    fflush(stdout)

#define TEST_PASS suite->passed++; printf("OK\n"); return 1

/* Helper: Parse and analyze a program */
static int parse_and_analyze(const char* source, SymbolTable** out_table, SemanticErrorList** out_errors) {
    Parser* parser = parser_create(source);
    ASTProgram* program = parser_parse(parser);
    
    if (parser->errors->error_count > 0) {
        *out_table = NULL;
        *out_errors = NULL;
        parser_free(parser);
        ast_program_free(program);
        return 0;
    }
    
    *out_table = symbol_table_create();
    *out_errors = semantic_error_list_create();
    
    int success = analyze_program(program, *out_table, *out_errors);
    
    parser_free(parser);
    ast_program_free(program);
    
    return success;
}

/* Test: Symbol table insert/lookup */
static int test_symbol_table_basic(TestSuite* suite) {
    TEST_START("Symbol table insert/lookup");
    
    SymbolTable* table = symbol_table_create();
    
    /* Add a function */
    CasmType params[] = {TYPE_I32};
    int result = symbol_table_add_function(table, "foo", TYPE_I32, params, 1, (SourceLocation){1, 1, 0});
    ASSERT_EQ(result, 1, "Function add should succeed");
    
    /* Lookup the function */
    FunctionSymbol* func = symbol_table_lookup_function(table, "foo");
    ASSERT_TRUE(func != NULL, "Function lookup should succeed");
    ASSERT_EQ(func->return_type, TYPE_I32, "Return type should match");
    ASSERT_EQ(func->param_count, 1, "Param count should match");
    
    /* Lookup non-existent function */
    func = symbol_table_lookup_function(table, "bar");
    ASSERT_TRUE(func == NULL, "Non-existent function should return NULL");
    
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Symbol table duplicate detection */
static int test_symbol_table_duplicates(TestSuite* suite) {
    TEST_START("Symbol table duplicate detection");
    
    SymbolTable* table = symbol_table_create();
    
    CasmType params[] = {TYPE_I32};
    int result = symbol_table_add_function(table, "foo", TYPE_I32, params, 1, (SourceLocation){1, 1, 0});
    ASSERT_EQ(result, 1, "First add should succeed");
    
    result = symbol_table_add_function(table, "foo", TYPE_I32, params, 1, (SourceLocation){2, 1, 0});
    ASSERT_EQ(result, 0, "Duplicate add should fail");
    
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Variable scope operations */
static int test_variable_scopes(TestSuite* suite) {
    TEST_START("Variable scopes");
    
    SymbolTable* table = symbol_table_create();
    
    /* Add variable in global scope */
    int result = symbol_table_add_variable(table, "x", TYPE_I32, (SourceLocation){1, 1, 0});
    ASSERT_EQ(result, 1, "Add to global scope should succeed");
    
    /* Push new scope */
    symbol_table_push_scope(table);
    
    /* Add variable with same name in inner scope (should succeed) */
    result = symbol_table_add_variable(table, "x", TYPE_I64, (SourceLocation){2, 1, 0});
    ASSERT_EQ(result, 1, "Add with same name to inner scope should succeed");
    
    /* Lookup should find inner scope variable */
    VariableSymbol* var = symbol_table_lookup_variable(table, "x");
    ASSERT_TRUE(var != NULL, "Variable lookup should succeed");
    ASSERT_EQ(var->type, TYPE_I64, "Should find inner scope variable");
    
    /* Pop scope */
    symbol_table_pop_scope(table);
    
    /* Now lookup should find outer scope variable */
    var = symbol_table_lookup_variable(table, "x");
    ASSERT_TRUE(var != NULL, "Variable lookup should succeed");
    ASSERT_EQ(var->type, TYPE_I32, "Should find outer scope variable");
    
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Type compatibility */
static int test_type_compatibility(TestSuite* suite) {
    TEST_START("Type compatibility");
    
    /* Same types are compatible */
    ASSERT_EQ(types_compatible(TYPE_I32, TYPE_I32), 1, "Same types should be compatible");
    
    /* Signed integers are compatible */
    ASSERT_EQ(types_compatible(TYPE_I8, TYPE_I32), 1, "Signed ints should be compatible");
    ASSERT_EQ(types_compatible(TYPE_I16, TYPE_I64), 1, "Signed ints should be compatible");
    
    /* Unsigned integers are compatible */
    ASSERT_EQ(types_compatible(TYPE_U8, TYPE_U32), 1, "Unsigned ints should be compatible");
    ASSERT_EQ(types_compatible(TYPE_U16, TYPE_U64), 1, "Unsigned ints should be compatible");
    
    /* Signed and unsigned NOT compatible */
    ASSERT_EQ(types_compatible(TYPE_I32, TYPE_U32), 0, "Signed/unsigned not compatible");
    
    /* Bool only compatible with bool */
    ASSERT_EQ(types_compatible(TYPE_BOOL, TYPE_BOOL), 1, "Bool compatible with bool");
    ASSERT_EQ(types_compatible(TYPE_BOOL, TYPE_I32), 0, "Bool not compatible with int");
    
    TEST_PASS;
}

/* Test: Simple valid program */
static int test_valid_program(TestSuite* suite) {
    TEST_START("Valid program");
    
    const char* source = "i32 main() { return 42; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 1, "Program should be valid");
    ASSERT_EQ(errors->error_count, 0, "Should have no errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Undefined function call */
static int test_undefined_function(TestSuite* suite) {
    TEST_START("Undefined function call");
    
    const char* source = "i32 main() { return foo(); }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Undefined variable */
static int test_undefined_variable(TestSuite* suite) {
    TEST_START("Undefined variable");
    
    const char* source = "i32 main() { return x; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Type mismatch in return */
static int test_return_type_mismatch(TestSuite* suite) {
    TEST_START("Return type mismatch");
    
    const char* source = "i32 main() { return true; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Function with parameters */
static int test_function_with_params(TestSuite* suite) {
    TEST_START("Function with parameters");
    
    const char* source = 
        "i32 add(i32 a, i32 b) { return a + b; }\n"
        "i32 main() { return add(1, 2); }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 1, "Program should be valid");
    ASSERT_EQ(errors->error_count, 0, "Should have no errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Function call with wrong argument count */
static int test_wrong_arg_count(TestSuite* suite) {
    TEST_START("Wrong argument count");
    
    const char* source = 
        "i32 add(i32 a, i32 b) { return a + b; }\n"
        "i32 main() { return add(1); }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Variable declaration with initializer */
static int test_var_with_initializer(TestSuite* suite) {
    TEST_START("Variable with initializer");
    
    const char* source = 
        "i32 main() { i32 x = 42; return x; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 1, "Program should be valid");
    ASSERT_EQ(errors->error_count, 0, "Should have no errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Duplicate function definition */
static int test_duplicate_function(TestSuite* suite) {
    TEST_START("Duplicate function definition");
    
    const char* source = 
        "i32 foo() { return 1; }\n"
        "i32 foo() { return 2; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Duplicate variable in same scope */
static int test_duplicate_variable(TestSuite* suite) {
    TEST_START("Duplicate variable in same scope");
    
    const char* source = 
        "i32 main() { i32 x = 1; i32 x = 2; return x; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Binary operator type checking */
static int test_binary_op_types(TestSuite* suite) {
    TEST_START("Binary operator type checking");
    
    const char* source = 
        "i32 main() { return 1 + true; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Logical operators require bool */
static int test_logical_op_types(TestSuite* suite) {
    TEST_START("Logical operator type checking");
    
    const char* source = 
        "bool main() { return 1 && 2; }";
    
    SymbolTable* table;
    SemanticErrorList* errors;
    int result = parse_and_analyze(source, &table, &errors);
    
    ASSERT_EQ(result, 0, "Program should be invalid");
    ASSERT_TRUE(errors->error_count > 0, "Should have errors");
    
    semantic_error_list_free(errors);
    symbol_table_free(table);
    TEST_PASS;
}

/* Test: Assignment expression type is LHS */
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
              "Assignment expr type should be LHS type (i32)");

    semantic_error_list_free(errors);
    symbol_table_free(table);
    parser_free(parser);
    ast_program_free(program);

    TEST_PASS;
}

/* Test: Nested blocks with same variable names */
static int test_nested_blocks_same_var_name(TestSuite* suite) {
    TEST_START("Nested blocks with same variable names");

    const char* source =
        "i32 main() {\n"
        "  { i32 x = 1; }\n"
        "  { i32 x = 2; }\n"
        "  return 0;\n"
        "}\n";

    Parser* parser = parser_create(source);
    ASTProgram* program = parser_parse(parser);
    ASSERT_EQ(parser->errors->error_count, 0, "Parsing should succeed");

    SymbolTable* table = symbol_table_create();
    SemanticErrorList* errors = semantic_error_list_create();

    int ok = analyze_program(program, table, errors);
    ASSERT_EQ(ok, 1, "Program should be semantically valid");
    ASSERT_EQ(errors->error_count, 0, "Should have no semantic errors - blocks create separate scopes");

    semantic_error_list_free(errors);
    symbol_table_free(table);
    parser_free(parser);
    ast_program_free(program);

    TEST_PASS;
}

/* Run all tests */
int main(void) {
    TestSuite suite = {
        .name = "Semantics Tests",
        .passed = 0,
        .failed = 0
    };
    
    printf("Running %s...\n\n", suite.name);
    
    /* Symbol table tests */
    test_symbol_table_basic(&suite);
    test_symbol_table_duplicates(&suite);
    test_variable_scopes(&suite);
    
    /* Type system tests */
    test_type_compatibility(&suite);
    
    /* Semantic analysis tests */
    test_valid_program(&suite);
    test_undefined_function(&suite);
    test_undefined_variable(&suite);
    test_return_type_mismatch(&suite);
    test_function_with_params(&suite);
    test_wrong_arg_count(&suite);
    test_var_with_initializer(&suite);
    test_duplicate_function(&suite);
    test_duplicate_variable(&suite);
    test_binary_op_types(&suite);
    test_logical_op_types(&suite);
    test_assignment_expression_type_is_lhs(&suite);
    test_nested_blocks_same_var_name(&suite);
    
    printf("\n");
    printf("Passed: %d\n", suite.passed);
    printf("Failed: %d\n", suite.failed);
    
    return suite.failed == 0 ? 0 : 1;
}
