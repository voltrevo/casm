#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t x = 10;
    if ((x > 5)) {
        printf("test.csm:5:8: x = %d\n", x);
    } else {
        printf("test.csm:7:8: 0 = %d\n", 0);
    }
    return 0;
}
