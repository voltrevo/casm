#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    printf("test.csm:3:4: true = %s, false = %s\n", (true) ? "true" : "false", (false) ? "true" : "false");
    return 0;
}
