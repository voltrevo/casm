#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t base_add(int32_t a, int32_t b);
int32_t level1_add_one(int32_t x);
int32_t level2_add_two(int32_t x);
int32_t main(void);

int32_t base_add(int32_t a, int32_t b) {
    return (a + b);
}

int32_t level1_add_one(int32_t x) {
    return base_add(x, 1);
}

int32_t level2_add_two(int32_t x) {
    return level1_add_one(level1_add_one(x));
}

int32_t main(void) {
    int32_t result = level2_add_two(5);
    printf("test.csm:6:4: result = %d\n", result);
    return 0;
}
