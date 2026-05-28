#include "util.h"

uint32_t mod_power_of_two(uint32_t a, int d) {
    return a & ((1u << d) - 1);
}
