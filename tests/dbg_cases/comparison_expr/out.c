#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 10;
    int32_t y = 20;
    printf("test.csm:5:4: expr(==) = %s, expr(<) = %s, expr(>) = %s, expr(!=) = %s\n", ((x == y)) ? "true" : "false", ((x < y)) ? "true" : "false", ((x > y)) ? "true" : "false", ((x != y)) ? "true" : "false");
    return 0;
}
