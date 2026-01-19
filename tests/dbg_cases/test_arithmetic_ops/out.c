#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 5;
    int32_t y = 3;
    int32_t a = (x + y);
    int32_t b = (x - y);
    int32_t c = (x * y);
    int32_t d = (x / y);
    int32_t e = (x % y);
    printf("test.csm:9:4: a = %d\n", a);
    printf("test.csm:10:4: b = %d\n", b);
    printf("test.csm:11:4: c = %d\n", c);
    printf("test.csm:12:4: d = %d\n", d);
    printf("test.csm:13:4: e = %d\n", e);
    return 0;
}
