#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int8_t a = 10;
    int8_t b = 20;
    int8_t c = (a + b);
    printf("test.csm:5:4: c = %d\n", c);
    return 0;
}
