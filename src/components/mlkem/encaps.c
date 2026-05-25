#include <string.h>
#include "mlkem.h"

/* ML-KEM.Encaps — FIPS 203 Algorithm 20
 *
 * Implementation outline:
 *  1. Sample 32-byte random message m.
 *  2. (K, r) = G(m || H(ek))    where G is SHA3-512.
 *  3. ct = K-PKE.Encrypt(ek, m, r):
 *       a. Decode t_hat and rho from ek.
 *       b. Regenerate A_hat from rho.
 *       c. Sample r_vec, e1, e2 from CBD_{eta1/eta2}(r).
 *       d. u = InvNTT(A_hat^T * NTT(r_vec)) + e1.
 *       e. v = InvNTT(t_hat^T * NTT(r_vec)) + e2 + Decompress_1(m).
 *       f. ct = Compress_{du}(u) || Compress_{dv}(v).
 *  4. ss = K.
 */
int mlkem512_encaps(
    uint8_t ct[MLKEM512_CTBYTES], uint8_t ss[MLKEM512_SSBYTES], const uint8_t ek[MLKEM512_EKBYTES]
) {
    /* TODO: implement per FIPS 203 Algorithm 20 */
    (void)ct;
    (void)ss;
    (void)ek;
    return 0;
}
