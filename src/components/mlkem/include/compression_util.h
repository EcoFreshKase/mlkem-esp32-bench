
#pragma once

#include <stdint.h>

uint16_t compress(uint16_t input, uint8_t compression_strength);

uint16_t decompress(uint16_t input, uint8_t compression_strength);