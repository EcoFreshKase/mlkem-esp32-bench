#include <string.h>
#include "esp_random.h"
#include "fips202.h"
#include "mlkem.h"
#include "params.h"
#include "poly.h"

/* ML-KEM.KeyGen_internal — FIPS 203 Algorithm 16 (wrapping K-PKE.KeyGen,
 * Algorithm 13). Seeds d and z are caller-supplied for KAT determinism. */
int mlkem512_keygen_internal(
    uint8_t ek[MLKEM512_EKBYTES],
    uint8_t dk[MLKEM512_DKBYTES],
    const uint8_t d[32],
    const uint8_t z[32]
) {
    /* G = SHA3-512(d ‖ k): the single domain-separation byte k = K is
     * appended to the 32-byte seed (final FIPS 203, not IPD). */
    uint8_t g_in[MLKEM512_SYMBYTES + 1];
    uint8_t g_out[64];
    memcpy(g_in, d, MLKEM512_SYMBYTES);
    g_in[MLKEM512_SYMBYTES] = MLKEM512_K;
    sha3_512(g_out, g_in, sizeof(g_in));

    const uint8_t *rho = g_out;
    const uint8_t *sigma = g_out + MLKEM512_SYMBYTES;

    /* Â[i][j] = SampleNTT(rho ‖ j ‖ i): the column byte j comes first. */
    polyvec_t a[MLKEM512_K];
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        for (unsigned int j = 0; j < MLKEM512_K; j++) {
            poly_sample_ntt(&a[i].vec[j], rho, (uint8_t)j, (uint8_t)i);
        }
    }

    polyvec_t s, e, t;
    uint8_t nonce = 0;
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_getnoise_eta1(&s.vec[i], sigma, nonce++);
    }
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        poly_getnoise_eta1(&e.vec[i], sigma, nonce++);
    }

    polyvec_ntt(&s);
    polyvec_ntt(&e);

    /* t̂ = Â · ŝ + ê (matrix-vector product in the NTT domain). */
    for (unsigned int i = 0; i < MLKEM512_K; i++) {
        polyvec_basemul_acc(&t.vec[i], &a[i], &s);
    }
    polyvec_add(&t, &t, &e);
    polyvec_reduce(&t);

    /* ek = ByteEncode_12(t̂) ‖ rho */
    polyvec_tobytes(ek, &t);
    memcpy(ek + MLKEM512_K * MLKEM_POLYBYTES, rho, MLKEM512_SYMBYTES);

    /* dk = ByteEncode_12(ŝ) ‖ ek ‖ H(ek) ‖ z */
    polyvec_tobytes(dk, &s);
    uint8_t *p = dk + MLKEM512_K * MLKEM_POLYBYTES;
    memcpy(p, ek, MLKEM512_EKBYTES);
    p += MLKEM512_EKBYTES;
    sha3_256(p, ek, MLKEM512_EKBYTES);
    p += MLKEM512_SYMBYTES;
    memcpy(p, z, MLKEM512_SYMBYTES);

    return 0;
}

int mlkem512_keygen(uint8_t ek[MLKEM512_EKBYTES], uint8_t dk[MLKEM512_DKBYTES]) {
    uint8_t d[32];
    uint8_t z[32];
    esp_fill_random(d, sizeof(d));
    esp_fill_random(z, sizeof(z));
    return mlkem512_keygen_internal(ek, dk, d, z);
}
