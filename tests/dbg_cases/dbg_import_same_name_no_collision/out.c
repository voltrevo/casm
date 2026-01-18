#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t process(int32_t s);
int32_t main(void);

int32_t process(int32_t s) {
    return ((s * 2) + 1);
}

int32_t main(void) {
    int32_t result = process(5);
    printf("test.csm:9:4: result = %d\n", result);
    return 0;
}
