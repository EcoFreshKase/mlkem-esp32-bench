#include "ntt.h"
#include "params.h"

/* -----------------------------------------------------------------------
 * Plain-form zetas: zetas[i] = 17^BitRev7(i) mod 3329, reduced to [0, q).
 * NOT the pq-crystals Montgomery table. Regenerate with:
 *   python3 -c "q=3329;br=lambda i:int(f'{i:07b}'[::-1],2);
 *               print([pow(17,br(i),q) for i in range(128)])"
 * ----------------------------------------------------------------------- */
static const int16_t zetas[128] = {
       1, 1729, 2580, 3289, 2642,  630, 1897,  848,
    1062, 1919,  193,  797, 2786, 3260,  569, 1746,
     296, 2447, 1339, 1476, 3046,   56, 2240, 1333,
    1426, 2094,  535, 2882, 2393, 2879, 1974,  821,
     289,  331, 3253, 1756, 1197, 2304, 2277, 2055,
     650, 1977, 2513,  632, 2865,   33, 1320, 1915,
    2319, 1435,  807,  452, 1438, 2868, 1534, 2402,
    2647, 2617, 1481,  648, 2474, 3110, 1227,  910,
      17, 2761,  583, 2649, 1637,  723, 2288, 1100,
    1409, 2662, 3281,  233,  756, 2156, 3015, 3050,
    1703, 1651, 2789, 1789, 1847,  952, 1461, 2687,
     939, 2308, 2437, 2388,  733, 2337,  268,  641,
    1584, 2298, 2037, 3220,  375, 2549, 2090, 1645,
    1063,  319, 2773,  757, 2099,  561, 2466, 2594,
    2804, 1092,  403, 1026, 1143, 2150, 2775,  886,
    1722, 1212, 1874, 1029, 2110, 2935,  885, 2154,
};

/* Reduce an integer to [0, q). Naive (no Barrett magic) per project scope. */
static int16_t reduce_mod_q(int32_t a) {
    int32_t r = a % MLKEM_Q;
    if (r < 0) {
        r += MLKEM_Q;
    }
    return (int16_t)r;
}

/* Modular multiplication, result in [0, q). Handles negative operands so
 * basemul can pass -zeta directly. */
static int16_t fqmul(int16_t a, int16_t b) {
    return reduce_mod_q((int32_t)a * b);
}

void poly_ntt(poly_t *p) {
    int16_t *r = p->coeffs;
    unsigned int len, start, j, k = 1;

    for (len = 128; len >= 2; len >>= 1) {
        for (start = 0; start < MLKEM_N; start = j + len) {
            int16_t zeta = zetas[k++];
            for (j = start; j < start + len; j++) {
                int16_t t = fqmul(zeta, r[j + len]);
                r[j + len] = reduce_mod_q((int32_t)r[j] - t);
                r[j] = reduce_mod_q((int32_t)r[j] + t);
            }
        }
    }
}

void poly_invntt(poly_t *p) {
    int16_t *r = p->coeffs;
    unsigned int len, start, j, k = 127;
    const int16_t n_inv = 3303; /* 128^{-1} mod q */

    for (len = 2; len <= 128; len <<= 1) {
        for (start = 0; start < MLKEM_N; start = j + len) {
            int16_t zeta = zetas[k--];
            for (j = start; j < start + len; j++) {
                int16_t t = r[j];
                r[j] = reduce_mod_q((int32_t)t + r[j + len]);
                r[j + len] = reduce_mod_q((int32_t)r[j + len] - t);
                r[j + len] = fqmul(zeta, r[j + len]);
            }
        }
    }

    for (j = 0; j < MLKEM_N; j++) {
        r[j] = fqmul(r[j], n_inv);
    }
}

/* One base-case product: (a0 + a1*x)(b0 + b1*x) mod (x^2 - zeta). */
static void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta) {
    r[0] = fqmul(fqmul(a[1], b[1]), zeta);
    r[0] = reduce_mod_q((int32_t)r[0] + fqmul(a[0], b[0]));
    r[1] = reduce_mod_q((int32_t)fqmul(a[0], b[1]) + fqmul(a[1], b[0]));
}

void poly_basemul(poly_t *r, const poly_t *a, const poly_t *b) {
    for (unsigned int i = 0; i < MLKEM_N / 4; i++) {
        basemul(&r->coeffs[4 * i], &a->coeffs[4 * i], &b->coeffs[4 * i], zetas[64 + i]);
        basemul(
            &r->coeffs[4 * i + 2],
            &a->coeffs[4 * i + 2],
            &b->coeffs[4 * i + 2],
            (int16_t)(-zetas[64 + i])
        );
    }
}
