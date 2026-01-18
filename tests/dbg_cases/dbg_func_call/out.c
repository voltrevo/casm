#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t add(int32_t a, int32_t b);
int32_t main(void);

int32_t add(int32_t a, int32_t b) {
    return (a + b);
}

int32_t main(void) {
    int32_t result = add(5, 3);
    printf("test.csm:8:4: result = %d\n", result);
    return 0;
}
