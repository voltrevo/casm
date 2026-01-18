#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t a = 100;
    int32_t b = 50;
    _Bool c = true;
    printf("test.csm:6:4: a = %d, b = %d, c = %s\n", a, b, (c) ? "true" : "false");
    return 0;
}
