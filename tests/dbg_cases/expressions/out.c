#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 5;
    int32_t y = 3;
    printf("test.csm:5:4: expr(+) = %d\n", (x + y));
    printf("test.csm:6:4: expr(*) = %d\n", (x * y));
    return 0;
}
