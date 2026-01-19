#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    {
        int32_t x = 1;
        printf("test.csm:4:8: x = %d\n", x);
    }
    {
        int32_t x = 2;
        printf("test.csm:8:8: x = %d\n", x);
    }
    return 0;
}
