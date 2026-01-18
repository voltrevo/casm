#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t a = 5;
    _Bool b = true;
    _Bool c = false;
    printf("test.csm:6:4: a = %d\n", a);
    printf("test.csm:7:4: b = %s\n", (b) ? "true" : "false");
    printf("test.csm:8:4: c = %s\n", (c) ? "true" : "false");
    return 0;
}
