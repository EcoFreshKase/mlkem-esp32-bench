#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "mlkem.h"
#include "mlkem_kat_vectors.h"
#include "ntt.h"
#include "params.h"
#include "poly.h"

static const char *TAG = "mlkem_test";

/* InvNTT ∘ NTT must be the identity on coefficients in [0, q). */
static void test_ntt_roundtrip(void) {
    poly_t p, orig;
    for (int i = 0; i < MLKEM_N; i++) {
        p.coeffs[i] = (int16_t)(i % MLKEM_Q);
    }
    orig = p;

    poly_ntt(&p);
    poly_invntt(&p);

    if (memcmp(p.coeffs, orig.coeffs, sizeof(orig.coeffs)) == 0)
        ESP_LOGI(TAG, "NTT round-trip PASS");
    else
        ESP_LOGE(TAG, "NTT round-trip FAIL");
}

/* Multiply two polynomials via NTT·basemul·InvNTT and compare against the
 * expected negacyclic product in Z_q[x]/(x^256 + 1).
 *   x · x        = x^2          (no wrap)
 *   x^255 · x    = x^256 = -1   (wraps to q-1 at coefficient 0) */
static void test_ntt_convolution(void) {
    poly_t a = {0}, b = {0}, r;

    a.coeffs[1] = 1; /* a(x) = x     */
    b.coeffs[1] = 1; /* b(x) = x     */
    poly_ntt(&a);
    poly_ntt(&b);
    poly_basemul(&r, &a, &b);
    poly_invntt(&r);

    int ok = (r.coeffs[2] == 1);
    for (int i = 0; i < MLKEM_N && ok; i++) {
        if (i != 2 && r.coeffs[i] != 0)
            ok = 0;
    }

    poly_t c = {0}, d = {0}, s;
    c.coeffs[255] = 1; /* c(x) = x^255 */
    d.coeffs[1] = 1;   /* d(x) = x     */
    poly_ntt(&c);
    poly_ntt(&d);
    poly_basemul(&s, &c, &d);
    poly_invntt(&s);

    int ok_wrap = (s.coeffs[0] == MLKEM_Q - 1);
    for (int i = 1; i < MLKEM_N && ok_wrap; i++) {
        if (s.coeffs[i] != 0)
            ok_wrap = 0;
    }

    if (ok && ok_wrap)
        ESP_LOGI(TAG, "NTT convolution PASS");
    else
        ESP_LOGE(TAG, "NTT convolution FAIL (mul=%d wrap=%d)", ok, ok_wrap);
}

/* ByteEncode_12 ∘ ByteDecode_12 must be the identity for coeffs in [0, q),
 * at both the poly and polyvec level. */
static void test_poly_encode_roundtrip(void) {
    poly_t p, q;
    uint8_t bytes[MLKEM_POLYBYTES];
    for (int i = 0; i < MLKEM_N; i++) {
        p.coeffs[i] = (int16_t)(i % MLKEM_Q);
    }
    poly_tobytes(bytes, &p);
    poly_frombytes(&q, bytes);

    int ok = (memcmp(p.coeffs, q.coeffs, sizeof(p.coeffs)) == 0);

    polyvec_t pv, qv;
    uint8_t vbytes[MLKEM512_K * MLKEM_POLYBYTES];
    for (int v = 0; v < MLKEM512_K; v++) {
        for (int i = 0; i < MLKEM_N; i++) {
            pv.vec[v].coeffs[i] = (int16_t)((i * (v + 1)) % MLKEM_Q);
        }
    }
    polyvec_tobytes(vbytes, &pv);
    polyvec_frombytes(&qv, vbytes);
    for (int v = 0; v < MLKEM512_K && ok; v++) {
        if (memcmp(pv.vec[v].coeffs, qv.vec[v].coeffs, sizeof(pv.vec[v].coeffs)) != 0)
            ok = 0;
    }

    if (ok)
        ESP_LOGI(TAG, "poly encode round-trip PASS");
    else
        ESP_LOGE(TAG, "poly encode round-trip FAIL");
}

/* CBD on an all-zero PRF stream must produce the all-zero polynomial
 * (every (a - b) pair is 0 - 0). */
static void test_poly_cbd_zero(void) {
    uint8_t buf1[MLKEM512_ETA1 * MLKEM_N / 4] = {0};
    uint8_t buf2[MLKEM512_ETA2 * MLKEM_N / 4] = {0};
    poly_t p1, p2;
    poly_cbd_eta1(&p1, buf1);
    poly_cbd_eta2(&p2, buf2);

    int ok = 1;
    for (int i = 0; i < MLKEM_N && ok; i++) {
        if (p1.coeffs[i] != 0 || p2.coeffs[i] != 0)
            ok = 0;
    }

    if (ok)
        ESP_LOGI(TAG, "poly CBD zero PASS");
    else
        ESP_LOGE(TAG, "poly CBD zero FAIL");
}

/* Decompress(Compress(x, d)) must stay within the FIPS 203 error bound
 * round(q / 2^(d+1)): 2 for du = 10, 104 for dv = 4. */
static void test_poly_compress_bound(void) {
    polyvec_t u, u2;
    poly_t v, v2;
    uint8_t ubytes[MLKEM512_DU * MLKEM512_K * MLKEM_N / 8];
    uint8_t vbytes[MLKEM512_DV * MLKEM_N / 8];

    for (int vec = 0; vec < MLKEM512_K; vec++) {
        for (int i = 0; i < MLKEM_N; i++) {
            u.vec[vec].coeffs[i] = (int16_t)((i * 13 + vec) % MLKEM_Q);
        }
    }
    for (int i = 0; i < MLKEM_N; i++) {
        v.coeffs[i] = (int16_t)((i * 7) % MLKEM_Q);
    }

    polyvec_compress_du(ubytes, &u);
    polyvec_decompress_du(&u2, ubytes);
    poly_compress_dv(vbytes, &v);
    poly_decompress_dv(&v2, vbytes);

    int ok = 1;
    for (int vec = 0; vec < MLKEM512_K && ok; vec++) {
        for (int i = 0; i < MLKEM_N && ok; i++) {
            int diff = u2.vec[vec].coeffs[i] - u.vec[vec].coeffs[i];
            if (diff > MLKEM_Q / 2)
                diff -= MLKEM_Q;
            if (diff < -MLKEM_Q / 2)
                diff += MLKEM_Q;
            if (diff < 0)
                diff = -diff;
            if (diff > 2)
                ok = 0;
        }
    }
    for (int i = 0; i < MLKEM_N && ok; i++) {
        int diff = v2.coeffs[i] - v.coeffs[i];
        if (diff > MLKEM_Q / 2)
            diff -= MLKEM_Q;
        if (diff < -MLKEM_Q / 2)
            diff += MLKEM_Q;
        if (diff < 0)
            diff = -diff;
        if (diff > 104)
            ok = 0;
    }

    if (ok)
        ESP_LOGI(TAG, "poly compress bound PASS");
    else
        ESP_LOGE(TAG, "poly compress bound FAIL");
}

/* poly_to_msg ∘ poly_from_msg must round-trip a 32-byte message exactly. */
static void test_poly_msg_roundtrip(void) {
    uint8_t msg[MLKEM512_SYMBYTES];
    uint8_t out[MLKEM512_SYMBYTES];
    poly_t p;
    for (int i = 0; i < MLKEM512_SYMBYTES; i++) {
        msg[i] = (uint8_t)(0xA5 ^ (i * 31));
    }
    poly_from_msg(&p, msg);
    poly_to_msg(out, &p);

    if (memcmp(msg, out, MLKEM512_SYMBYTES) == 0)
        ESP_LOGI(TAG, "poly msg round-trip PASS");
    else
        ESP_LOGE(TAG, "poly msg round-trip FAIL");
}

/* All NIST ACVP ML-KEM-512 keyGen vectors (AFT, 25 vectors). Each test feeds
 * (d, z) to mlkem512_keygen_internal and compares (ek, dk) against expected. */
static void test_mlkem512_keygen_vectors(void) {
    uint8_t ek[MLKEM512_EKBYTES];
    uint8_t dk[MLKEM512_DKBYTES];
    int pass = 0;

    for (size_t i = 0; i < KAT_KEYGEN_COUNT; i++) {
        mlkem512_keygen_internal(ek, dk, kat_keygen_d[i], kat_keygen_z[i]);
        int ek_ok = (memcmp(ek, kat_keygen_ek[i], MLKEM512_EKBYTES) == 0);
        int dk_ok = (memcmp(dk, kat_keygen_dk[i], MLKEM512_DKBYTES) == 0);
        if (ek_ok && dk_ok)
            pass++;
        else
            ESP_LOGE(TAG, "ML-KEM-512 keygen tcId %d FAIL (ek=%d dk=%d)",
                     kat_keygen_tc_id[i], ek_ok, dk_ok);
    }
    ESP_LOGI(TAG, "ML-KEM-512 keygen: %d/%d PASS", pass, KAT_KEYGEN_COUNT);
}

/* All NIST ACVP ML-KEM-512 encapDecap tgId 1 vectors (AFT encaps, 25 vectors).
 * Each test feeds (ek, m) to mlkem512_encaps_internal and compares (c, k). */
static void test_mlkem512_encaps_vectors(void) {
    uint8_t ct[MLKEM512_CTBYTES];
    uint8_t ss[MLKEM512_SSBYTES];
    int pass = 0;

    for (size_t i = 0; i < KAT_ENCAPS_COUNT; i++) {
        mlkem512_encaps_internal(ct, ss, kat_encaps_ek[i], kat_encaps_m[i]);
        int ct_ok = (memcmp(ct, kat_encaps_c[i], MLKEM512_CTBYTES) == 0);
        int ss_ok = (memcmp(ss, kat_encaps_k[i], MLKEM512_SSBYTES) == 0);
        if (ct_ok && ss_ok)
            pass++;
        else
            ESP_LOGE(TAG, "ML-KEM-512 encaps tcId %d FAIL (ct=%d ss=%d)",
                     kat_encaps_tc_id[i], ct_ok, ss_ok);
    }
    ESP_LOGI(TAG, "ML-KEM-512 encaps: %d/%d PASS", pass, KAT_ENCAPS_COUNT);
}

/* All NIST ACVP ML-KEM-512 encapDecap tgId 4 vectors (VAL decaps, 10 vectors).
 * Each test feeds (dk, c) to mlkem512_decaps and compares k. */
static void test_mlkem512_decaps_vectors(void) {
    uint8_t ss[MLKEM512_SSBYTES];
    int pass = 0;

    for (size_t i = 0; i < KAT_DECAPS_COUNT; i++) {
        mlkem512_decaps(ss, kat_decaps_c[i], kat_decaps_dk[i]);
        if (memcmp(ss, kat_decaps_k[i], MLKEM512_SSBYTES) == 0)
            pass++;
        else
            ESP_LOGE(TAG, "ML-KEM-512 decaps tcId %d FAIL",
                     kat_decaps_tc_id[i]);
    }
    ESP_LOGI(TAG, "ML-KEM-512 decaps: %d/%d PASS", pass, KAT_DECAPS_COUNT);
}

/* Full round-trip: keygen → encaps → decaps; asserts both shared secrets match. */
static void test_mlkem512_roundtrip(void) {
    static uint8_t ek[MLKEM512_EKBYTES];
    static uint8_t dk[MLKEM512_DKBYTES];
    static uint8_t ct[MLKEM512_CTBYTES];
    static uint8_t ss_enc[MLKEM512_SSBYTES];
    static uint8_t ss_dec[MLKEM512_SSBYTES];

    mlkem512_keygen(ek, dk);
    mlkem512_encaps(ct, ss_enc, ek);
    mlkem512_decaps(ss_dec, ct, dk);

    if (memcmp(ss_enc, ss_dec, MLKEM512_SSBYTES) == 0)
        ESP_LOGI(TAG, "ML-KEM-512 round-trip PASS");
    else
        ESP_LOGE(TAG, "ML-KEM-512 round-trip FAIL");
}

void run_mlkem512_tests(void) {
    ESP_LOGI(TAG, "=== ML-KEM-512 tests ===");
    test_ntt_roundtrip();
    test_ntt_convolution();
    test_poly_encode_roundtrip();
    test_poly_cbd_zero();
    test_poly_compress_bound();
    test_poly_msg_roundtrip();
    test_mlkem512_keygen_vectors();
    test_mlkem512_encaps_vectors();
    test_mlkem512_decaps_vectors();
    test_mlkem512_roundtrip();
    ESP_LOGI(TAG, "=== ML-KEM-512 tests done ===");
}
