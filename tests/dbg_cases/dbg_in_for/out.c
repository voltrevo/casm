#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t i = 0;
    for (i = 0; (i < 3); i = (i + 1)) {
        int32_t squared = (i * i);
        printf("test.csm:6:8: i = %d, squared = %d\n", i, squared);
    }
    return 0;
}
