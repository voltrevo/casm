#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 10;
    int32_t y = 5;
    int32_t z = 2;
    printf("test.csm:6:4: expr(+) = %d, expr(*) = %d, expr(/) = %d\n", (x + (y * z)), ((x - y) * (y + z)), ((x * y) / z));
    return 0;
}
