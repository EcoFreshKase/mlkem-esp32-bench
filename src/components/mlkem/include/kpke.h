#pragma once

#include <stdint.h>
#include "params.h"

/* -----------------------------------------------------------------------
 * Internal K-PKE primitives shared between encaps and decaps. Not part of
 * the public ML-KEM API (see mlkem.h).
 * ----------------------------------------------------------------------- */

/* K-PKE.Encrypt (FIPS 203 Algorithm 14). ek is the 800-byte encapsulation
 * key (t̂ ‖ rho); m is the 32-byte message; r is the 32-byte encryption
 * randomness. Writes the 768-byte ciphertext to c. */
void k_pke_encrypt(
    uint8_t c[MLKEM512_CTBYTES],
    const uint8_t ek[MLKEM512_EKBYTES],
    const uint8_t m[MLKEM512_SYMBYTES],
    const uint8_t r[MLKEM512_SYMBYTES]
);
