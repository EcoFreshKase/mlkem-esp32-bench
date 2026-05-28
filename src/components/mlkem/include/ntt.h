#pragma once

#include "poly.h"

/* -----------------------------------------------------------------------
 * Number-Theoretic Transform over Z_q[x]/(x^256 + 1), q = 3329.
 *
 * Plain modular form (no Montgomery): the zetas table holds
 * 17^BitRev7(i) mod q directly. All three operations work in place / on
 * caller-provided storage and leave coefficients in [0, q).
 * ----------------------------------------------------------------------- */

/* Forward NTT (FIPS 203 Algorithm 9), in place. */
void poly_ntt(poly_t *r);

/* Inverse NTT (FIPS 203 Algorithm 10), in place. Includes the final
 * multiplication by 128^{-1} mod q = 3303. */
void poly_invntt(poly_t *r);

/* Base-case multiplication in the NTT domain (FIPS 203 Algorithms 11/12):
 * r = a * b as 128 products of degree-1 polynomials mod (x^2 - zeta). */
void poly_basemul(poly_t *r, const poly_t *a, const poly_t *b);
