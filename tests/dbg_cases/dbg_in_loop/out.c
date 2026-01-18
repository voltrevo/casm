#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t i = 0;
    while ((i < 3)) {
        printf("test.csm:5:8: i = %d\n", i);
        i = (i + 1);
    }
    return 0;
}
