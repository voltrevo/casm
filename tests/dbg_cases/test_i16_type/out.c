#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t add_i16(int16_t a, int16_t b);
int32_t main(void);

int32_t add_i16(int16_t a, int16_t b) {
    int32_t result = (a + b);
    return result;
}

int32_t main(void) {
    int16_t x = 10;
    int16_t y = 20;
    int32_t result = add_i16(x, y);
    printf("test.csm:10:4: result = %d\n", result);
    return 0;
}
