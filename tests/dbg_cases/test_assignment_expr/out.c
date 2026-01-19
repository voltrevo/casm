#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 42;
    int32_t y = x;
    int32_t z = y;
    printf("test.csm:5:4: x = %d\n", x);
    printf("test.csm:6:4: y = %d\n", y);
    printf("test.csm:7:4: z = %d\n", z);
    return 0;
}
