#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 0;
    int32_t y = 0;
    printf("test.csm:5:4: expr(=) = %d, expr(=) = %d\n", x = 5, y = 10);
    printf("test.csm:6:4: x = %d, y = %d\n", x, y);
    return 0;
}
