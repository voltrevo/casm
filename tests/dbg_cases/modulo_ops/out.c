#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t a = 17;
    int32_t b = 5;
    printf("test.csm:5:4: expr(%%) = %d, expr(%%) = %d, expr(%%) = %d\n", (a % b), (b % a), (0 % b));
    return 0;
}
