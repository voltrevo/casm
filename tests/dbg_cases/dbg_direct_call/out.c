#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t get_five(void);
int32_t main(void);

int32_t get_five(void) {
    return 5;
}

int32_t main(void) {
    printf("test.csm:7:4: get_five() = %d\n", get_five());
    return 0;
}
