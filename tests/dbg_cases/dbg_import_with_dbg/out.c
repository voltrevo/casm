#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t square(int32_t x);
int32_t cube(int32_t x);
int32_t main(void);

int32_t square(int32_t x) {
    return (x * x);
}

int32_t cube(int32_t x) {
    return ((x * x) * x);
}

int32_t main(void) {
    int32_t x = 3;
    int32_t __dbg_tmp_0 = square(x);
    printf("test.csm:6:4: square() = %d\n", __dbg_tmp_0);
    int32_t __dbg_tmp_1 = cube(x);
    printf("test.csm:7:4: cube() = %d\n", __dbg_tmp_1);
    int32_t __dbg_tmp_2 = square(cube(2));
    printf("test.csm:8:4: square() = %d\n", __dbg_tmp_2);
    return 0;
}
