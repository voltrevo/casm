#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 5;
    int32_t y = 10;
    printf("test.csm:5:4: expr(+) = %d, expr(*) = %d, expr(-) = %d\n", (x + y), (x * y), (y - x));
    return 0;
}
