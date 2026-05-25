#include "tests.h"

void run_fips202_tests(void);
void run_mlkem512_tests(void);

void run_all_tests(void) {
    run_fips202_tests();
    run_mlkem512_tests();
}
