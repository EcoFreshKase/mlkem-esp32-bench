#include <string.h>
#include "esp_random.h"
#include "fips202.h"
#include "kpke.h"
#include "mlkem.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"

#define POLYVEC_BYTES   (MLKEM512_K * MLKEM_POLYBYTES) /* 768 */
#define COMPRESSED_U    (MLKEM512_DU * MLKEM512_K * MLKEM_N / 8) /* 640 */

/* K-PKE.Encrypt — FIPS 203 Algorithm 14. Writes the 768-byte ciphertext
 * c = Compress_du(u) ‖ Compress_dv(v). */
void k_pke_encrypt(
    uint8_t c[MLKEM512_CTBYTES],
    const uint8_t ek[MLKEM512_EKBYTES],
    const uint8_t m[MLKEM512_SYMBYTES],
    const uint8_t r[MLKEM512_SYMBYTES]
) {
    polyvec_t t;
    poly_frombytes(&t.vec[0], ek);
    poly_frombytes(&t.vec[1], ek + MLKEM_POLYBYTES);
    const uint8_t *rho = ek + POLYVEC_BYTES;

    /* Encryption uses the transpose: Âᵀ[i][j] = SampleNTT(rho ‖ i ‖ j). */
    polyvec_t at[MLKEM512_K];
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        for (unsigned int j = 0; j < MLKEM512_K; j++) {
            poly_sample_ntt(&at[i].vec[j], rho, (uint8_t)i, (uint8_t)j);
        }
    }

    polyvec_t rvec, e1;
    poly_t e2;
    uint8_t nonce = 0;
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_getnoise_eta1(&rvec.vec[i], r, nonce++);
    }
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_getnoise_eta2(&e1.vec[i], r, nonce++);
    }
    poly_getnoise_eta2(&e2, r, nonce++);

    polyvec_ntt(&rvec);

    /* u = InvNTT(Âᵀ ∘ r̂) + e1 */
    polyvec_t u;
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        polyvec_basemul_acc(&u.vec[i], &at[i], &rvec);
    }
    polyvec_invntt(&u);
    polyvec_add(&u, &u, &e1);
    polyvec_reduce(&u);

    /* v = InvNTT(t̂ ∘ r̂) + e2 + Decompress_1(m) */
    poly_t v, mu;
    polyvec_basemul_acc(&v, &t, &rvec);
    poly_invntt(&v);
    poly_from_msg(&mu, m);
    poly_add(&v, &v, &e2);
    poly_add(&v, &v, &mu);
    poly_reduce(&v);

    polyvec_compress_du(c, &u);
    poly_compress_dv(c + COMPRESSED_U, &v);
}

int mlkem512_encaps_internal(
    uint8_t ct[MLKEM512_CTBYTES],
    uint8_t ss[MLKEM512_SSBYTES],
    const uint8_t ek[MLKEM512_EKBYTES],
    const uint8_t m[MLKEM512_SYMBYTES]
) {
    /* (K, r) = G(m ‖ H(ek)): K is the shared secret, r drives encryption.
     * Final FIPS 203 — no separate hash of m. */
    uint8_t g_in[2 * MLKEM512_SYMBYTES];
    uint8_t g_out[64];
    memcpy(g_in, m, MLKEM512_SYMBYTES);
    sha3_256(g_in + MLKEM512_SYMBYTES, ek, MLKEM512_EKBYTES);
    sha3_512(g_out, g_in, sizeof(g_in));

    k_pke_encrypt(ct, ek, m, g_out + MLKEM512_SYMBYTES);
    memcpy(ss, g_out, MLKEM512_SSBYTES);
    return 0;
}

int mlkem512_encaps(
    uint8_t ct[MLKEM512_CTBYTES], uint8_t ss[MLKEM512_SSBYTES], const uint8_t ek[MLKEM512_EKBYTES]
) {
    uint8_t m[MLKEM512_SYMBYTES];
    esp_fill_random(m, sizeof(m));
    return mlkem512_encaps_internal(ct, ss, ek, m);
}
