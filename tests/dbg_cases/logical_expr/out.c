#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    _Bool a = true;
    _Bool b = false;
    printf("test.csm:5:4: expr(&&) = %s, expr(||) = %s\n", ((a && b)) ? "true" : "false", ((a || b)) ? "true" : "false");
    return 0;
}
