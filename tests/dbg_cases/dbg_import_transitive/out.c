#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t add(int32_t a, int32_t b);
int32_t increment(int32_t x);
int32_t double_value(int32_t x);
int32_t main(void);

int32_t add(int32_t a, int32_t b) {
    return (a + b);
}

int32_t increment(int32_t x) {
    return add(x, 1);
}

int32_t double_value(int32_t x) {
    return add(x, x);
}

int32_t main(void) {
    int32_t x = 5;
    int32_t inc_x = increment(x);
    int32_t double_x = double_value(x);
    printf("test.csm:8:4: inc_x = %d\n", inc_x);
    printf("test.csm:9:4: double_x = %d\n", double_x);
    return 0;
}
