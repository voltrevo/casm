#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t neg = (-100);
    int32_t pos = 100;
    printf("test.csm:5:4: neg = %d, pos = %d, expr(+) = %d\n", neg, pos, (neg + pos));
    return 0;
}
