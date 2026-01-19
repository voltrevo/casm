#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    _Bool b1 = true;
    _Bool b2 = false;
    _Bool b3 = (b1 && b2);
    _Bool b4 = (b1 || b2);
    _Bool b5 = (!b1);
    _Bool b6 = (!b2);
    printf("test.csm:8:4: b3 = %s\n", (b3) ? "true" : "false");
    printf("test.csm:9:4: b4 = %s\n", (b4) ? "true" : "false");
    printf("test.csm:10:4: b5 = %s\n", (b5) ? "true" : "false");
    printf("test.csm:11:4: b6 = %s\n", (b6) ? "true" : "false");
    return 0;
}
