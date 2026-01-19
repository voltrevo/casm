#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 10;
    int32_t y = 5;
    _Bool b1 = (x > y);
    _Bool b2 = (x < y);
    _Bool b3 = (x == y);
    _Bool b4 = (x != y);
    _Bool b5 = (x >= y);
    _Bool b6 = (x <= y);
    printf("test.csm:10:4: b1 = %s\n", (b1) ? "true" : "false");
    printf("test.csm:11:4: b2 = %s\n", (b2) ? "true" : "false");
    printf("test.csm:12:4: b3 = %s\n", (b3) ? "true" : "false");
    printf("test.csm:13:4: b4 = %s\n", (b4) ? "true" : "false");
    printf("test.csm:14:4: b5 = %s\n", (b5) ? "true" : "false");
    printf("test.csm:15:4: b6 = %s\n", (b6) ? "true" : "false");
    return 0;
}
