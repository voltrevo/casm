#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t max_val = 2147483647;
    int32_t min_val = (-2147483648);
    int32_t zero = 0;
    printf("test.csm:6:4: max_val = %d, min_val = %d, zero = %d\n", max_val, min_val, zero);
    return 0;
}
