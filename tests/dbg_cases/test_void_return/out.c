#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t func_void(void);
int32_t main(void);

int32_t func_void(void) {
    return 0;
}

int32_t main(void) {
    func_void();
    return 0;
}
