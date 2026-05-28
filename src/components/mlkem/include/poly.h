#pragma once

#include <stdint.h>
#include "params.h"

/* -----------------------------------------------------------------------
 * Polynomial and module-vector types for ML-KEM-512.
 *
 * A poly_t holds the 256 coefficients of an element of Z_q[x]/(x^256 + 1).
 * Coefficients are kept signed (int16_t) so the centered binomial
 * distribution and intermediate subtractions stay representable; callers
 * reduce into [0, q) before serialization.
 *
 * The matrix Â is a plain C array poly_t[K][K] at the call sites — it has
 * no dedicated typedef.
 * ----------------------------------------------------------------------- */

typedef struct {
    int16_t coeffs[MLKEM_N];
} poly_t; /* 512 bytes */

typedef struct {
    poly_t vec[MLKEM512_K];
} polyvec_t; /* 1024 bytes for K = 2 */
