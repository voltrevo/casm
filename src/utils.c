#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Memory management */
void* xmalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Fatal: malloc failed\n");
        exit(1);
    }
    return ptr;
}

void* xrealloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Fatal: realloc failed\n");
        exit(1);
    }
    return new_ptr;
}

void xfree(void* ptr) {
    free(ptr);
}

/* String utilities */
char* xstrdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* dup = xmalloc(len + 1);
    strcpy(dup, str);
    return dup;
}

char* xstrndup(const char* str, size_t n) {
    if (!str) return NULL;
    char* dup = xmalloc(n + 1);
    strncpy(dup, str, n);
    dup[n] = '\0';
    return dup;
}
