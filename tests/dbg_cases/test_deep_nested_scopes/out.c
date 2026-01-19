#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    {
        int32_t x = 1;
        {
            int32_t y = 2;
            {
                int32_t z = 3;
                printf("test.csm:8:16: x = %d\n", x);
                printf("test.csm:9:16: y = %d\n", y);
                printf("test.csm:10:16: z = %d\n", z);
            }
        }
    }
    return 0;
}
