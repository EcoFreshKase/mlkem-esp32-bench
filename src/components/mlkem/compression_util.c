#include <stdint.h>
#include "compression_util.h"
#include "params.h"
#include "util.h"

uint16_t compress(uint16_t input, uint8_t compression_strength) {
    uint32_t result = ((uint32_t)input * (1u << compression_strength) + MLKEM_Q / 2) / MLKEM_Q;
    return (uint16_t)mod_power_of_two(result, compression_strength);
}

uint16_t decompress(uint16_t input, uint8_t compression_strength) {
    uint32_t result = ((uint32_t)input * MLKEM_Q + (1u << compression_strength) / 2) /
                      (1u << compression_strength);
    return (uint16_t)result;
}
