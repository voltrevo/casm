#include "test_harness.h"
#include "../src/lexer.h"
#include "../src/utils.h"

/* Helper to tokenize a string and return all tokens */
typedef struct {
    Token* tokens;
    int count;
} TokenList;

static TokenList tokenize(const char* source) {
    Lexer* lexer = lexer_create(source);
    TokenList list = {0};
    list.tokens = xmalloc(sizeof(Token) * 100);
    list.count = 0;
    
    Token token;
    do {
        token = lexer_next_token(lexer);
        if (list.count < 100) {
            list.tokens[list.count++] = token;
        }
    } while (token.type != TOK_EOF);
    
    lexer_free(lexer);
    return list;
}

static void free_token_list(TokenList list) {
    xfree(list.tokens);
}

/* Tests */

void test_single_integer() {
    TokenList list = tokenize("42");
    ASSERT_EQ(list.tokens[0].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[0].int_value, 42);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_multiple_integers() {
    TokenList list = tokenize("1 2 3");
    ASSERT_EQ(list.tokens[0].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[0].int_value, 1);
    ASSERT_EQ(list.tokens[1].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[1].int_value, 2);
    ASSERT_EQ(list.tokens[2].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[2].int_value, 3);
    ASSERT_EQ(list.tokens[3].type, TOK_EOF);
    free_token_list(list);
}

void test_simple_identifier() {
    TokenList list = tokenize("x");
    ASSERT_EQ(list.tokens[0].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[0].lexeme_len, 1);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_multiple_identifiers() {
    TokenList list = tokenize("foo bar _baz");
    ASSERT_EQ(list.tokens[0].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[1].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[2].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[3].type, TOK_EOF);
    free_token_list(list);
}

void test_keyword_int() {
    TokenList list = tokenize("int");
    ASSERT_EQ(list.tokens[0].type, TOK_INT);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_keyword_void() {
    TokenList list = tokenize("void");
    ASSERT_EQ(list.tokens[0].type, TOK_VOID);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_keyword_if() {
    TokenList list = tokenize("if");
    ASSERT_EQ(list.tokens[0].type, TOK_IF);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_keyword_while() {
    TokenList list = tokenize("while");
    ASSERT_EQ(list.tokens[0].type, TOK_WHILE);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_keyword_for() {
    TokenList list = tokenize("for");
    ASSERT_EQ(list.tokens[0].type, TOK_FOR);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_keyword_return() {
    TokenList list = tokenize("return");
    ASSERT_EQ(list.tokens[0].type, TOK_RETURN);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_single_char_operators() {
    TokenList list = tokenize("+ - * / % ; , ( )");
    ASSERT_EQ(list.tokens[0].type, TOK_PLUS);
    ASSERT_EQ(list.tokens[1].type, TOK_MINUS);
    ASSERT_EQ(list.tokens[2].type, TOK_STAR);
    ASSERT_EQ(list.tokens[3].type, TOK_SLASH);
    ASSERT_EQ(list.tokens[4].type, TOK_PERCENT);
    ASSERT_EQ(list.tokens[5].type, TOK_SEMICOLON);
    ASSERT_EQ(list.tokens[6].type, TOK_COMMA);
    ASSERT_EQ(list.tokens[7].type, TOK_LPAREN);
    ASSERT_EQ(list.tokens[8].type, TOK_RPAREN);
    ASSERT_EQ(list.tokens[9].type, TOK_EOF);
    free_token_list(list);
}

void test_multi_char_operators() {
    TokenList list = tokenize("== != <= >= && ||");
    ASSERT_EQ(list.tokens[0].type, TOK_EQ);
    ASSERT_EQ(list.tokens[1].type, TOK_NE);
    ASSERT_EQ(list.tokens[2].type, TOK_LE);
    ASSERT_EQ(list.tokens[3].type, TOK_GE);
    ASSERT_EQ(list.tokens[4].type, TOK_AND);
    ASSERT_EQ(list.tokens[5].type, TOK_OR);
    ASSERT_EQ(list.tokens[6].type, TOK_EOF);
    free_token_list(list);
}

void test_comparison_operators() {
    TokenList list = tokenize("< > = ! ");
    ASSERT_EQ(list.tokens[0].type, TOK_LT);
    ASSERT_EQ(list.tokens[1].type, TOK_GT);
    ASSERT_EQ(list.tokens[2].type, TOK_ASSIGN);
    ASSERT_EQ(list.tokens[3].type, TOK_NOT);
    ASSERT_EQ(list.tokens[4].type, TOK_EOF);
    free_token_list(list);
}

void test_braces() {
    TokenList list = tokenize("{ }");
    ASSERT_EQ(list.tokens[0].type, TOK_LBRACE);
    ASSERT_EQ(list.tokens[1].type, TOK_RBRACE);
    ASSERT_EQ(list.tokens[2].type, TOK_EOF);
    free_token_list(list);
}

void test_simple_function() {
    TokenList list = tokenize("int add(int a, int b) { return a + b; }");
    ASSERT_EQ(list.tokens[0].type, TOK_INT);
    ASSERT_EQ(list.tokens[1].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[2].type, TOK_LPAREN);
    ASSERT_EQ(list.tokens[3].type, TOK_INT);
    ASSERT_EQ(list.tokens[4].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[5].type, TOK_COMMA);
    ASSERT_EQ(list.tokens[6].type, TOK_INT);
    ASSERT_EQ(list.tokens[7].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[8].type, TOK_RPAREN);
    ASSERT_EQ(list.tokens[9].type, TOK_LBRACE);
    ASSERT_EQ(list.tokens[10].type, TOK_RETURN);
    ASSERT_EQ(list.tokens[11].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[12].type, TOK_PLUS);
    ASSERT_EQ(list.tokens[13].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[14].type, TOK_SEMICOLON);
    ASSERT_EQ(list.tokens[15].type, TOK_RBRACE);
    ASSERT_EQ(list.tokens[16].type, TOK_EOF);
    free_token_list(list);
}

void test_single_line_comment() {
    TokenList list = tokenize("42 // comment\n 43");
    ASSERT_EQ(list.tokens[0].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[0].int_value, 42);
    ASSERT_EQ(list.tokens[1].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[1].int_value, 43);
    ASSERT_EQ(list.tokens[2].type, TOK_EOF);
    free_token_list(list);
}

void test_multi_line_comment() {
    TokenList list = tokenize("42 /* comment */ 43");
    ASSERT_EQ(list.tokens[0].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[0].int_value, 42);
    ASSERT_EQ(list.tokens[1].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[1].int_value, 43);
    ASSERT_EQ(list.tokens[2].type, TOK_EOF);
    free_token_list(list);
}

void test_line_column_tracking() {
    TokenList list = tokenize("int x\nint y");
    ASSERT_EQ(list.tokens[0].location.line, 1);
    ASSERT_EQ(list.tokens[1].location.line, 1);
    ASSERT_EQ(list.tokens[2].location.line, 2);
    ASSERT_EQ(list.tokens[3].location.line, 2);
    free_token_list(list);
}

void test_large_number() {
    TokenList list = tokenize("999999999");
    ASSERT_EQ(list.tokens[0].type, TOK_INT_LITERAL);
    ASSERT_EQ(list.tokens[0].int_value, 999999999);
    ASSERT_EQ(list.tokens[1].type, TOK_EOF);
    free_token_list(list);
}

void test_identifier_with_numbers() {
    TokenList list = tokenize("var123 x_456_y");
    ASSERT_EQ(list.tokens[0].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[0].lexeme_len, 6);
    ASSERT_EQ(list.tokens[1].type, TOK_IDENTIFIER);
    ASSERT_EQ(list.tokens[1].lexeme_len, 7);
    ASSERT_EQ(list.tokens[2].type, TOK_EOF);
    free_token_list(list);
}

void test_mixed_code() {
    TokenList list = tokenize("int main() { int x = 10; return x; }");
    /* Just verify it tokenizes without error */
    int found_error = 0;
    for (int i = 0; i < list.count; i++) {
        if (list.tokens[i].type == TOK_ERROR) {
            found_error = 1;
            break;
        }
    }
    ASSERT_FALSE(found_error);
    free_token_list(list);
}

int main() {
    RUN_TEST(test_single_integer);
    RUN_TEST(test_multiple_integers);
    RUN_TEST(test_simple_identifier);
    RUN_TEST(test_multiple_identifiers);
    RUN_TEST(test_keyword_int);
    RUN_TEST(test_keyword_void);
    RUN_TEST(test_keyword_if);
    RUN_TEST(test_keyword_while);
    RUN_TEST(test_keyword_for);
    RUN_TEST(test_keyword_return);
    RUN_TEST(test_single_char_operators);
    RUN_TEST(test_multi_char_operators);
    RUN_TEST(test_comparison_operators);
    RUN_TEST(test_braces);
    RUN_TEST(test_simple_function);
    RUN_TEST(test_single_line_comment);
    RUN_TEST(test_multi_line_comment);
    RUN_TEST(test_line_column_tracking);
    RUN_TEST(test_large_number);
    RUN_TEST(test_identifier_with_numbers);
    RUN_TEST(test_mixed_code);
    
    PRINT_SUMMARY();
}
