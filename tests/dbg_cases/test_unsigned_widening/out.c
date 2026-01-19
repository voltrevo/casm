#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    uint32_t a = 100;
    uint32_t b = a;
    uint32_t c = a;
    uint32_t d = a;
    printf("test.csm:6:4: a = %u\n", (unsigned int)(a));
    printf("test.csm:7:4: b = %u\n", (unsigned int)(b));
    printf("test.csm:8:4: c = %u\n", (unsigned int)(c));
    printf("test.csm:9:4: d = %u\n", (unsigned int)(d));
    return 0;
}
