#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 5;
    int32_t result = (-x);
    printf("test.csm:4:4: result = %d\n", result);
    return 0;
}
