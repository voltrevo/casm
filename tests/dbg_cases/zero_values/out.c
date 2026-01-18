#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t z = 0;
    int32_t x = 10;
    printf("test.csm:5:4: z = %d, expr(*) = %d, expr(+) = %d\n", z, (x * z), (z + x));
    return 0;
}
