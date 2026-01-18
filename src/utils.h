#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <string.h>

/* Memory management utilities */
void* xmalloc(size_t size);
void* xrealloc(void* ptr, size_t size);
void xfree(void* ptr);

/* String utilities */
char* xstrdup(const char* str);
char* xstrndup(const char* str, size_t n);

/* Error reporting with location info */
typedef struct {
    int line;
    int column;
    int offset;
} SourceLocation;

void error(const char* message);
void error_at(SourceLocation loc, const char* message);
void warning(const char* message);
void warning_at(SourceLocation loc, const char* message);

#endif /* UTILS_H */
