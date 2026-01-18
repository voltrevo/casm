#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 10;
    {
        int32_t y = 20;
        printf("test.csm:6:8: x = %d, y = %d\n", x, y);
    }
    printf("test.csm:8:4: x = %d\n", x);
    return 0;
}
