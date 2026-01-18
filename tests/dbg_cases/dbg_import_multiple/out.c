#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t add(int32_t a, int32_t b);
int32_t multiply(int32_t a, int32_t b);
int32_t subtract(int32_t a, int32_t b);
int32_t main(void);

int32_t add(int32_t a, int32_t b) {
    return (a + b);
}

int32_t multiply(int32_t a, int32_t b) {
    return (a * b);
}

int32_t subtract(int32_t a, int32_t b) {
    return (a - b);
}

int32_t main(void) {
    int32_t sum = add(10, 5);
    int32_t product = multiply(3, 4);
    printf("test.csm:7:4: sum = %d\n", sum);
    printf("test.csm:8:4: product = %d\n", product);
    return 0;
}
