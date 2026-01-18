#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

int32_t main(void);

int32_t main(void) {
    int32_t very_long_variable_name_that_goes_on_and_on = 42;
    printf("test.csm:4:4: very_long_variable_name_that_goes_on_and_on = %d\n", very_long_variable_name_that_goes_on_and_on);
    return 0;
}
