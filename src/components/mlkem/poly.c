#include "poly.h"
#include <string.h>
#include "compression_util.h"
#include "fips202.h"
#include "ntt.h"
#include "params.h"

/* CBD PRF stream lengths: 64*eta bytes per FIPS 203 Alg 8. */
#define CBD_ETA1_BYTES (MLKEM512_ETA1 * MLKEM_N / 4) /* 192 */
#define CBD_ETA2_BYTES (MLKEM512_ETA2 * MLKEM_N / 4) /* 128 */

/* Bring one coefficient into [0, q) from any int32 representative. */
static int16_t freeze(int32_t a) {
    int32_t r = a % MLKEM_Q;
    if (r < 0) {
        r += MLKEM_Q;
    }
    return (int16_t)r;
}

void poly_reduce(poly_t *r) {
    for (unsigned int i = 0; i < MLKEM_N; i++) {
        r->coeffs[i] = freeze(r->coeffs[i]);
    }
}

void poly_add(poly_t *r, const poly_t *a, const poly_t *b) {
    for (unsigned int i = 0; i < MLKEM_N; i++) {
        r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
    }
}

void poly_sub(poly_t *r, const poly_t *a, const poly_t *b) {
    for (unsigned int i = 0; i < MLKEM_N; i++) {
        r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
    }
}

void polyvec_reduce(polyvec_t *r) {
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_reduce(&r->vec[i]);
    }
}

void polyvec_add(polyvec_t *r, const polyvec_t *a, const polyvec_t *b) {
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
    }
}

void polyvec_ntt(polyvec_t *r) {
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_ntt(&r->vec[i]);
    }
}

void polyvec_invntt(polyvec_t *r) {
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_invntt(&r->vec[i]);
    }
}

void polyvec_basemul_acc(poly_t *r, const polyvec_t *a, const polyvec_t *b) {
    poly_t t;
    poly_basemul(r, &a->vec[0], &b->vec[0]);
    for (unsigned int i = 1; i < MLKEM512_K; i++) {
        poly_basemul(&t, &a->vec[i], &b->vec[i]);
        poly_add(r, r, &t);
    }
    poly_reduce(r);
}

/* -----------------------------------------------------------------------
 * ByteEncode_12 / ByteDecode_12 — pack 2 coefficients into 3 bytes.
 * ----------------------------------------------------------------------- */
void poly_tobytes(uint8_t r[MLKEM_POLYBYTES], const poly_t *a) {
    for (unsigned int i = 0; i < MLKEM_N / 2; i++) {
        int16_t t0 = a->coeffs[2 * i];
        int16_t t1 = a->coeffs[2 * i + 1];
        t0 += (t0 >> 15) & MLKEM_Q; /* fold a possible negative into [0, q) */
        t1 += (t1 >> 15) & MLKEM_Q;
        r[3 * i + 0] = (uint8_t)(t0 & 0xFF);
        r[3 * i + 1] = (uint8_t)((t0 >> 8) | (t1 << 4));
        r[3 * i + 2] = (uint8_t)(t1 >> 4);
    }
}

void poly_frombytes(poly_t *r, const uint8_t a[MLKEM_POLYBYTES]) {
    for (unsigned int i = 0; i < MLKEM_N / 2; i++) {
        r->coeffs[2 * i] = (int16_t)(((a[3 * i + 0] >> 0) | ((uint16_t)a[3 * i + 1] << 8)) & 0xFFF);
        r->coeffs[2 * i + 1] =
            (int16_t)(((a[3 * i + 1] >> 4) | ((uint16_t)a[3 * i + 2] << 4)) & 0xFFF);
    }
}

void polyvec_tobytes(uint8_t r[MLKEM512_K * MLKEM_POLYBYTES], const polyvec_t *a) {
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_tobytes(r + i * MLKEM_POLYBYTES, &a->vec[i]);
    }
}

void polyvec_frombytes(polyvec_t *r, const uint8_t a[MLKEM512_K * MLKEM_POLYBYTES]) {
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_frombytes(&r->vec[i], a + i * MLKEM_POLYBYTES);
    }
}

/* -----------------------------------------------------------------------
 * Centered binomial distribution (FIPS 203 Alg 8).
 * ----------------------------------------------------------------------- */
static uint32_t load24_le(const uint8_t x[3]) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) | ((uint32_t)x[2] << 16);
}

static uint32_t load32_le(const uint8_t x[4]) {
    return (uint32_t)x[0] | ((uint32_t)x[1] << 8) | ((uint32_t)x[2] << 16) | ((uint32_t)x[3] << 24);
}

void poly_cbd_eta1(poly_t *r, const uint8_t buf[CBD_ETA1_BYTES]) {
    /* eta1 = 3: 3 input bits per sample, 24 bits -> 4 coefficients. */
    for (unsigned int i = 0; i < MLKEM_N / 4; i++) {
        uint32_t t = load24_le(buf + 3 * i);
        uint32_t d = t & 0x00249249;
        d += (t >> 1) & 0x00249249;
        d += (t >> 2) & 0x00249249;
        for (unsigned int j = 0; j < 4; j++) {
            int16_t a = (d >> (6 * j)) & 0x7;
            int16_t b = (d >> (6 * j + 3)) & 0x7;
            r->coeffs[4 * i + j] = a - b;
        }
    }
}

void poly_cbd_eta2(poly_t *r, const uint8_t buf[CBD_ETA2_BYTES]) {
    /* eta2 = 2: 2 input bits per sample, 32 bits -> 8 coefficients. */
    for (unsigned int i = 0; i < MLKEM_N / 8; i++) {
        uint32_t t = load32_le(buf + 4 * i);
        uint32_t d = t & 0x55555555;
        d += (t >> 1) & 0x55555555;
        for (unsigned int j = 0; j < 8; j++) {
            int16_t a = (d >> (4 * j)) & 0x3;
            int16_t b = (d >> (4 * j + 2)) & 0x3;
            r->coeffs[8 * i + j] = a - b;
        }
    }
}

/* PRF (FIPS 203 §4.1): SHAKE256(seed ‖ nonce). */
static void prf(uint8_t *out, size_t outlen, const uint8_t seed[MLKEM512_SYMBYTES], uint8_t nonce) {
    uint8_t in[MLKEM512_SYMBYTES + 1];
    memcpy(in, seed, MLKEM512_SYMBYTES);
    in[MLKEM512_SYMBYTES] = nonce;
    shake256(out, outlen, in, sizeof(in));
}

void poly_getnoise_eta1(poly_t *r, const uint8_t seed[MLKEM512_SYMBYTES], uint8_t nonce) {
    uint8_t buf[CBD_ETA1_BYTES];
    prf(buf, sizeof(buf), seed, nonce);
    poly_cbd_eta1(r, buf);
}

void poly_getnoise_eta2(poly_t *r, const uint8_t seed[MLKEM512_SYMBYTES], uint8_t nonce) {
    uint8_t buf[CBD_ETA2_BYTES];
    prf(buf, sizeof(buf), seed, nonce);
    poly_cbd_eta2(r, buf);
}

/* -----------------------------------------------------------------------
 * SampleNTT (FIPS 203 Alg 7): 12-bit rejection sampling over SHAKE128.
 * SHAKE128_RATE (168) is divisible by 3, so each squeezed block yields a
 * whole number of 24-bit chunks with no carryover between blocks.
 * ----------------------------------------------------------------------- */
void poly_sample_ntt(poly_t *r, const uint8_t rho[MLKEM512_SYMBYTES], uint8_t x, uint8_t y) {
    keccak_state state;
    uint8_t seed[MLKEM512_SYMBYTES + 2];
    uint8_t buf[SHAKE128_RATE];

    memcpy(seed, rho, MLKEM512_SYMBYTES);
    seed[MLKEM512_SYMBYTES] = x;
    seed[MLKEM512_SYMBYTES + 1] = y;
    shake128_absorb_once(&state, seed, sizeof(seed));

    unsigned int ctr = 0;
    while (ctr < MLKEM_N) {
        shake128_squeezeblocks(buf, 1, &state);
        for (unsigned int k = 0; k + 3 <= SHAKE128_RATE && ctr < MLKEM_N; k += 3) {
            uint16_t d1 = ((uint16_t)buf[k] | ((uint16_t)(buf[k + 1] & 0xF) << 8));
            uint16_t d2 = ((uint16_t)(buf[k + 1] >> 4) | ((uint16_t)buf[k + 2] << 4));
            if (d1 < MLKEM_Q) {
                r->coeffs[ctr++] = (int16_t)d1;
            }
            if (d2 < MLKEM_Q && ctr < MLKEM_N) {
                r->coeffs[ctr++] = (int16_t)d2;
            }
        }
    }
}

/* -----------------------------------------------------------------------
 * Message encode/decode.
 * ----------------------------------------------------------------------- */
void poly_from_msg(poly_t *r, const uint8_t msg[MLKEM512_SYMBYTES]) {
    for (unsigned int i = 0; i < MLKEM_N / 8; i++) {
        for (unsigned int j = 0; j < 8; j++) {
            int16_t mask = -(int16_t)((msg[i] >> j) & 1);
            r->coeffs[8 * i + j] = mask & ((MLKEM_Q + 1) / 2);
        }
    }
}

void poly_to_msg(uint8_t msg[MLKEM512_SYMBYTES], const poly_t *a) {
    for (unsigned int i = 0; i < MLKEM_N / 8; i++) {
        msg[i] = 0;
        for (unsigned int j = 0; j < 8; j++) {
            uint16_t t = compress((uint16_t)freeze(a->coeffs[8 * i + j]), 1);
            msg[i] |= (uint8_t)(t << j);
        }
    }
}

/* -----------------------------------------------------------------------
 * Ciphertext compression wrappers.
 * ----------------------------------------------------------------------- */
void polyvec_compress_du(uint8_t r[MLKEM512_DU * MLKEM512_K * MLKEM_N / 8], const polyvec_t *a) {
    /* du = 10: pack 4 coefficients (10 bits each) into 5 bytes. */
    for (unsigned int v = 0; v < MLKEM512_K; v++) {
        const poly_t *p = &a->vec[v];
        for (unsigned int i = 0; i < MLKEM_N / 4; i++) {
            uint16_t t[4];
            for (unsigned int j = 0; j < 4; j++) {
                t[j] = compress((uint16_t)freeze(p->coeffs[4 * i + j]), 10);
            }
            r[0] = (uint8_t)(t[0]);
            r[1] = (uint8_t)((t[0] >> 8) | (t[1] << 2));
            r[2] = (uint8_t)((t[1] >> 6) | (t[2] << 4));
            r[3] = (uint8_t)((t[2] >> 4) | (t[3] << 6));
            r[4] = (uint8_t)(t[3] >> 2);
            r += 5;
        }
    }
}

void polyvec_decompress_du(polyvec_t *r, const uint8_t a[MLKEM512_DU * MLKEM512_K * MLKEM_N / 8]) {
    for (unsigned int v = 0; v < MLKEM512_K; v++) {
        poly_t *p = &r->vec[v];
        for (unsigned int i = 0; i < MLKEM_N / 4; i++) {
            uint16_t t[4];
            t[0] = (a[0] >> 0) | ((uint16_t)a[1] << 8);
            t[1] = (a[1] >> 2) | ((uint16_t)a[2] << 6);
            t[2] = (a[2] >> 4) | ((uint16_t)a[3] << 4);
            t[3] = (a[3] >> 6) | ((uint16_t)a[4] << 2);
            a += 5;
            for (unsigned int j = 0; j < 4; j++) {
                p->coeffs[4 * i + j] = (int16_t)decompress(t[j] & 0x3FF, 10);
            }
        }
    }
}

void poly_compress_dv(uint8_t r[MLKEM512_DV * MLKEM_N / 8], const poly_t *a) {
    /* dv = 4: pack 2 coefficients (4 bits each) per byte. */
    for (unsigned int i = 0; i < MLKEM_N / 2; i++) {
        uint16_t t0 = compress((uint16_t)freeze(a->coeffs[2 * i]), 4);
        uint16_t t1 = compress((uint16_t)freeze(a->coeffs[2 * i + 1]), 4);
        r[i] = (uint8_t)(t0 | (t1 << 4));
    }
}

void poly_decompress_dv(poly_t *r, const uint8_t a[MLKEM512_DV * MLKEM_N / 8]) {
    for (unsigned int i = 0; i < MLKEM_N / 2; i++) {
        r->coeffs[2 * i] = (int16_t)decompress(a[i] & 0xF, 4);
        r->coeffs[2 * i + 1] = (int16_t)decompress(a[i] >> 4, 4);
    }
}
