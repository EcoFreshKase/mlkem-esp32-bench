#include <string.h>
#include "mlkem.h"

/* ML-KEM.Decaps — FIPS 203 Algorithm 21
 *
 * Implementation outline:
 *  1. Parse dk into: dk_pke (s_hat bytes), ek, h = H(ek), z.
 *  2. m' = K-PKE.Decrypt(dk_pke, ct):
 *       a. u = Decompress_{du}(ct[0..du*k*n/8]).
 *       b. v = Decompress_{dv}(ct[du*k*n/8..]).
 *       c. w = v - InvNTT(s_hat^T * NTT(u)).
 *       d. m' = Compress_1(w).
 *  3. (K', r') = G(m' || h).
 *  4. K_bar = J(z || ct)   where J is SHAKE-256.
 *  5. ct' = K-PKE.Encrypt(ek, m', r').
 *  6. ss = K'  if ct == ct' (constant-time),  else ss = K_bar.
 */
int mlkem512_decaps(uint8_t ss[MLKEM512_SSBYTES],
                    const uint8_t ct[MLKEM512_CTBYTES],
                    const uint8_t dk[MLKEM512_DKBYTES])
{
    /* TODO: implement per FIPS 203 Algorithm 21 */
    (void)ss;
    (void)ct;
    (void)dk;
    return 0;
}
