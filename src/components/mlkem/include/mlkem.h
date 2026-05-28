#pragma once

#include <stdint.h>
#include "params.h"

/* -----------------------------------------------------------------------
 * ML-KEM-512 public API — FIPS 203, Section 7
 *
 * All functions return 0 on success, negative on error.
 * Array-dimension annotations mirror the FIPS 203 spec notation.
 * ----------------------------------------------------------------------- */

/**
 * ML-KEM.KeyGen — Algorithm 19
 *
 * Generates an encapsulation key (ek) and a decapsulation key (dk).
 * Internally samples random seeds d and z.
 *
 * @param[out] ek   Encapsulation key, MLKEM512_EKBYTES (800) bytes.
 * @param[out] dk   Decapsulation key, MLKEM512_DKBYTES (1632) bytes.
 * @return 0 on success.
 */
int mlkem512_keygen(uint8_t ek[MLKEM512_EKBYTES], uint8_t dk[MLKEM512_DKBYTES]);

/**
 * ML-KEM.KeyGen internal — deterministic form of Algorithm 19
 *
 * Same as mlkem512_keygen but uses caller-supplied seeds d and z instead of
 * sampling them randomly.  Use this for KAT tests.
 *
 * @param[out] ek   Encapsulation key, MLKEM512_EKBYTES (800) bytes.
 * @param[out] dk   Decapsulation key, MLKEM512_DKBYTES (1632) bytes.
 * @param[in]  d    32-byte seed d.
 * @param[in]  z    32-byte seed z.
 * @return 0 on success.
 */
int mlkem512_keygen_internal(
    uint8_t ek[MLKEM512_EKBYTES],
    uint8_t dk[MLKEM512_DKBYTES],
    const uint8_t d[32],
    const uint8_t z[32]
);

/**
 * ML-KEM.Encaps — Algorithm 20
 *
 * Produces a ciphertext (ct) and shared secret (ss) from an encapsulation key.
 *
 * @param[out] ct   Ciphertext, MLKEM512_CTBYTES (768) bytes.
 * @param[out] ss   Shared secret, MLKEM512_SSBYTES (32) bytes.
 * @param[in]  ek   Encapsulation key, MLKEM512_EKBYTES (800) bytes.
 * @return 0 on success.
 */
int mlkem512_encaps(
    uint8_t ct[MLKEM512_CTBYTES], uint8_t ss[MLKEM512_SSBYTES], const uint8_t ek[MLKEM512_EKBYTES]
);

/**
 * ML-KEM.Encaps internal — deterministic form of Algorithm 20
 *
 * Same as mlkem512_encaps but uses the caller-supplied message m instead of
 * sampling it randomly.  Use this for KAT tests.
 *
 * @param[out] ct   Ciphertext, MLKEM512_CTBYTES (768) bytes.
 * @param[out] ss   Shared secret, MLKEM512_SSBYTES (32) bytes.
 * @param[in]  ek   Encapsulation key, MLKEM512_EKBYTES (800) bytes.
 * @param[in]  m    32-byte message m.
 * @return 0 on success.
 */
int mlkem512_encaps_internal(
    uint8_t ct[MLKEM512_CTBYTES],
    uint8_t ss[MLKEM512_SSBYTES],
    const uint8_t ek[MLKEM512_EKBYTES],
    const uint8_t m[MLKEM512_SYMBYTES]
);

/**
 * ML-KEM.Decaps — Algorithm 21
 *
 * Recovers the shared secret from a ciphertext using the decapsulation key.
 * Always writes MLKEM512_SSBYTES bytes to ss (implicit rejection on failure).
 *
 * @param[out] ss   Shared secret, MLKEM512_SSBYTES (32) bytes.
 * @param[in]  ct   Ciphertext, MLKEM512_CTBYTES (768) bytes.
 * @param[in]  dk   Decapsulation key, MLKEM512_DKBYTES (1632) bytes.
 * @return 0 on success.
 */
int mlkem512_decaps(
    uint8_t ss[MLKEM512_SSBYTES],
    const uint8_t ct[MLKEM512_CTBYTES],
    const uint8_t dk[MLKEM512_DKBYTES]
);
