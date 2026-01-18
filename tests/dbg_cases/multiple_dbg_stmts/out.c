#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 10;
    printf("test.csm:4:4: x = %d\n", x);
    x = (x + 5);
    printf("test.csm:6:4: x = %d\n", x);
    x = (x * 2);
    printf("test.csm:8:4: x = %d\n", x);
    return 0;
}
