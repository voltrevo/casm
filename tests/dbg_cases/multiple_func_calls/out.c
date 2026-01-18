#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t add(int32_t a, int32_t b);
int32_t multiply(int32_t a, int32_t b);
int32_t main(void);

int32_t add(int32_t a, int32_t b) {
    return (a + b);
}

int32_t multiply(int32_t a, int32_t b) {
    return (a * b);
}

int32_t main(void) {
    int32_t __dbg_tmp_0 = add(2, 3);
    int32_t __dbg_tmp_1 = multiply(4, 5);
    printf("test.csm:11:4: add() = %d, multiply() = %d\n", __dbg_tmp_0, __dbg_tmp_1);
    return 0;
}
