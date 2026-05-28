#include <string.h>
#include "fips202.h"
#include "kpke.h"
#include "mlkem.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"

#define POLYVEC_BYTES (MLKEM512_K * MLKEM_POLYBYTES)            /* 768 */
#define COMPRESSED_U  (MLKEM512_DU * MLKEM512_K * MLKEM_N / 8) /* 640 */

/* dk slicing (FIPS 203): dk_pke[0:768] ‖ ek[768:1568] ‖ h[1568:1600] ‖ z[1600:1632]. */
#define DK_EK_OFFSET POLYVEC_BYTES
#define DK_H_OFFSET  (DK_EK_OFFSET + MLKEM512_EKBYTES)
#define DK_Z_OFFSET  (DK_H_OFFSET + MLKEM512_SYMBYTES)

/* Constant-time inequality: returns 1 if the buffers differ, else 0. */
static uint8_t ct_neq(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t acc = 0;
    for (size_t i = 0; i < len; i++) {
        acc |= a[i] ^ b[i];
    }
    return (uint8_t)((-(uint32_t)acc) >> 31);
}

/* Constant-time conditional move: r = x for all bytes iff cond == 1. */
static void ct_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t cond) {
    uint8_t mask = (uint8_t)(-(int)cond);
    for (size_t i = 0; i < len; i++) {
        r[i] ^= mask & (r[i] ^ x[i]);
    }
}

/* K-PKE.Decrypt — FIPS 203 Algorithm 15. Recovers the 32-byte message. */
static void k_pke_decrypt(uint8_t m[MLKEM512_SYMBYTES], const uint8_t dk_pke[POLYVEC_BYTES],
                          const uint8_t ct[MLKEM512_CTBYTES]) {
    polyvec_t u, s;
    poly_t v, w;

    polyvec_decompress_du(&u, ct);
    poly_decompress_dv(&v, ct + COMPRESSED_U);
    polyvec_frombytes(&s, dk_pke);

    /* w = v - InvNTT(ŝ ∘ NTT(u)) */
    polyvec_ntt(&u);
    polyvec_basemul_acc(&w, &s, &u);
    poly_invntt(&w);
    poly_sub(&w, &v, &w);
    poly_reduce(&w);

    poly_to_msg(m, &w);
}

int mlkem512_decaps(
    uint8_t ss[MLKEM512_SSBYTES],
    const uint8_t ct[MLKEM512_CTBYTES],
    const uint8_t dk[MLKEM512_DKBYTES]
) {
    const uint8_t *dk_pke = dk;
    const uint8_t *ek = dk + DK_EK_OFFSET;
    const uint8_t *h = dk + DK_H_OFFSET;
    const uint8_t *z = dk + DK_Z_OFFSET;

    uint8_t mprime[MLKEM512_SYMBYTES];
    k_pke_decrypt(mprime, dk_pke, ct);

    /* (K', r') = G(m' ‖ h) */
    uint8_t g_in[2 * MLKEM512_SYMBYTES];
    uint8_t g_out[64];
    memcpy(g_in, mprime, MLKEM512_SYMBYTES);
    memcpy(g_in + MLKEM512_SYMBYTES, h, MLKEM512_SYMBYTES);
    sha3_512(g_out, g_in, sizeof(g_in));
    const uint8_t *kprime = g_out;
    const uint8_t *rprime = g_out + MLKEM512_SYMBYTES;

    /* Implicit-rejection secret K_bar = J(z ‖ ct) = SHAKE256(z ‖ ct, 32). */
    uint8_t kbar_in[MLKEM512_SYMBYTES + MLKEM512_CTBYTES];
    uint8_t kbar[MLKEM512_SSBYTES];
    memcpy(kbar_in, z, MLKEM512_SYMBYTES);
    memcpy(kbar_in + MLKEM512_SYMBYTES, ct, MLKEM512_CTBYTES);
    shake256(kbar, sizeof(kbar), kbar_in, sizeof(kbar_in));

    /* Re-encrypt and select K' on match, K_bar on mismatch (constant time). */
    uint8_t ct_cmp[MLKEM512_CTBYTES];
    k_pke_encrypt(ct_cmp, ek, mprime, rprime);

    memcpy(ss, kprime, MLKEM512_SSBYTES);
    uint8_t differ = ct_neq(ct, ct_cmp, MLKEM512_CTBYTES);
    ct_cmov(ss, kbar, MLKEM512_SSBYTES, differ);

    return 0;
}
