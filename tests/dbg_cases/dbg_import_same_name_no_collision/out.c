#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t module_a_helper(int32_t x);
int32_t process_a(int32_t n);
int32_t module_b_helper(int32_t x);
int32_t process_b(int32_t n);
int32_t main(void);

int32_t module_a_helper(int32_t x) {
    return (x + 10);
}

int32_t process_a(int32_t n) {
    return module_a_helper(n);
}

int32_t module_b_helper(int32_t x) {
    return (x * 5);
}

int32_t process_b(int32_t n) {
    return module_b_helper(n);
}

int32_t main(void) {
    int32_t result_a = process_a(3);
    int32_t result_b = process_b(3);
    printf("test.csm:12:4: result_a = %d\n", result_a);
    printf("test.csm:13:4: result_b = %d\n", result_b);
    return 0;
}
