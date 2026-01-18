#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 5;
    _Bool b = true;
    printf("test.csm:5:4: -expr = %d, !expr = %s\n", (-x), ((!b)) ? "true" : "false");
    return 0;
}
