#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t add(int32_t a, int32_t b);
int32_t main(void);

int32_t add(int32_t a, int32_t b) {
    return (a + b);
}

int32_t main(void) {
    int32_t __dbg_tmp_0 = add(add(1, 2), 3);
    printf("test.csm:7:4: add() = %d\n", __dbg_tmp_0);
    return 0;
}
