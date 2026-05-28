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

/* -----------------------------------------------------------------------
 * Coefficient arithmetic. poly_add/poly_sub do raw signed arithmetic;
 * call poly_reduce afterwards to bring coefficients back into [0, q).
 * ----------------------------------------------------------------------- */
void poly_reduce(poly_t *r);
void poly_add(poly_t *r, const poly_t *a, const poly_t *b);
void poly_sub(poly_t *r, const poly_t *a, const poly_t *b);

/* Module-vector (length-K) wrappers. */
void polyvec_reduce(polyvec_t *r);
void polyvec_add(polyvec_t *r, const polyvec_t *a, const polyvec_t *b);
void polyvec_ntt(polyvec_t *r);
void polyvec_invntt(polyvec_t *r);

/* r = sum_i a.vec[i] * b.vec[i] in the NTT domain, reduced to [0, q). */
void polyvec_basemul_acc(poly_t *r, const polyvec_t *a, const polyvec_t *b);

/* -----------------------------------------------------------------------
 * ByteEncode_12 / ByteDecode_12 (FIPS 203 Alg 5/6). Input coefficients
 * must be in [0, q); poly_tobytes folds negatives back first.
 * ----------------------------------------------------------------------- */
void poly_tobytes(uint8_t r[MLKEM_POLYBYTES], const poly_t *a);
void poly_frombytes(poly_t *r, const uint8_t a[MLKEM_POLYBYTES]);
void polyvec_tobytes(uint8_t r[MLKEM512_K * MLKEM_POLYBYTES], const polyvec_t *a);
void polyvec_frombytes(polyvec_t *r, const uint8_t a[MLKEM512_K * MLKEM_POLYBYTES]);

/* -----------------------------------------------------------------------
 * Noise sampling. SamplePolyCBD (FIPS 203 Alg 8) over a caller-provided
 * PRF stream; poly_getnoise_* run the SHAKE256 PRF then the CBD.
 * cbd_eta1/eta2 buffers are 64*eta bytes (192 and 128 respectively).
 * ----------------------------------------------------------------------- */
void poly_cbd_eta1(poly_t *r, const uint8_t buf[MLKEM512_ETA1 * MLKEM_N / 4]);
void poly_cbd_eta2(poly_t *r, const uint8_t buf[MLKEM512_ETA2 * MLKEM_N / 4]);
void poly_getnoise_eta1(poly_t *r, const uint8_t seed[MLKEM512_SYMBYTES], uint8_t nonce);
void poly_getnoise_eta2(poly_t *r, const uint8_t seed[MLKEM512_SYMBYTES], uint8_t nonce);

/* SampleNTT (FIPS 203 Alg 7): rejection-sample Â[.][.] from SHAKE128 on
 * rho ‖ x ‖ y. Result is already in the NTT domain. */
void poly_sample_ntt(
    poly_t *r, const uint8_t rho[MLKEM512_SYMBYTES], uint8_t x, uint8_t y
);

/* -----------------------------------------------------------------------
 * Message <-> polynomial (FIPS 203 §). Each message bit maps to a
 * coefficient of (q+1)/2; poly_to_msg is per-coefficient Compress_1.
 * ----------------------------------------------------------------------- */
void poly_from_msg(poly_t *r, const uint8_t msg[MLKEM512_SYMBYTES]);
void poly_to_msg(uint8_t msg[MLKEM512_SYMBYTES], const poly_t *a);

/* -----------------------------------------------------------------------
 * Ciphertext compression wrappers (loop Compress/Decompress over coeffs).
 *   u-vector: d = du = 10, 320 bytes per poly (640 for the vector)
 *   v-poly:   d = dv = 4,  128 bytes
 * ----------------------------------------------------------------------- */
void polyvec_compress_du(uint8_t r[MLKEM512_DU * MLKEM512_K * MLKEM_N / 8], const polyvec_t *a);
void polyvec_decompress_du(polyvec_t *r, const uint8_t a[MLKEM512_DU * MLKEM512_K * MLKEM_N / 8]);
void poly_compress_dv(uint8_t r[MLKEM512_DV * MLKEM_N / 8], const poly_t *a);
void poly_decompress_dv(poly_t *r, const uint8_t a[MLKEM512_DV * MLKEM_N / 8]);
