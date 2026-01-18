#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    _Bool t = true;
    _Bool f = false;
    printf("test.csm:5:4: t = %s\n", (t) ? "true" : "false");
    printf("test.csm:6:4: f = %s\n", (f) ? "true" : "false");
    return 0;
}
