#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
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
        fprintf(stderr, "Usage: %s <source.c>\n", argv[0]);
        return 1;
    }
    
    char* source = read_file(argv[1]);
    Lexer* lexer = lexer_create(source);
    
    /* Tokenize and print tokens */
    Token token;
    do {
        token = lexer_next_token(lexer);
        printf("[%d:%d] %s", token.location.line, token.location.column, token_type_name(token.type));
        
        if (token.type == TOK_INT_LITERAL) {
            printf(" (%ld)", token.int_value);
        } else if (token.type == TOK_IDENTIFIER) {
            printf(" (%.*s)", token.lexeme_len, token.lexeme);
        }
        printf("\n");
    } while (token.type != TOK_EOF && token.type != TOK_ERROR);
    
    if (token.type == TOK_ERROR) {
        fprintf(stderr, "Tokenization failed at line %d, column %d\n",
                token.location.line, token.location.column);
        lexer_free(lexer);
        xfree(source);
        return 1;
    }
    
    lexer_free(lexer);
    xfree(source);
    return 0;
}
