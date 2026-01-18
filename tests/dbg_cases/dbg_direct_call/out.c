#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t get_five(void);
int32_t main(void);

int32_t get_five(void) {
    return 5;
}

int32_t main(void) {
    int32_t __dbg_tmp_0 = get_five();
    printf("test.csm:7:4: get_five() = %d\n", __dbg_tmp_0);
    return 0;
}
