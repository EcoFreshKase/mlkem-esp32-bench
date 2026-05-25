#pragma once

/* -----------------------------------------------------------------------
 * ML-KEM-512 compile-time parameters — FIPS 203, Section 8
 * ----------------------------------------------------------------------- */

/* Polynomial ring Z_q[x]/(x^n + 1) */
#define MLKEM_N    256
#define MLKEM_Q    3329
#define MLKEM_QINV 62209 /* q^{-1} mod 2^16, used in Montgomery reduction */

/* ML-KEM-512 rank and noise parameters */
#define MLKEM512_K    2 /* module rank */
#define MLKEM512_ETA1 3 /* CBD noise for key generation (s, e) */
#define MLKEM512_ETA2 2 /* CBD noise for encryption (r, e1, e2) */

/* Ciphertext compression parameters */
#define MLKEM512_DU 10 /* bits per coefficient for u vector */
#define MLKEM512_DV 4  /* bits per coefficient for v polynomial */

/* -----------------------------------------------------------------------
 * Byte-length constants (derived from FIPS 203 Table 2)
 *
 *  ek  = 12·k·n/8 + 32  = 12·2·256/8 + 32  =  800 bytes
 *  dk  = 384·k + 800 + 64               = 1632 bytes
 *  ct  = du·k·n/8 + dv·n/8             =  768 bytes
 *  ss  = 32 bytes (fixed for all parameter sets)
 * ----------------------------------------------------------------------- */
#define MLKEM512_EKBYTES  800
#define MLKEM512_DKBYTES  1632
#define MLKEM512_CTBYTES  768
#define MLKEM512_SSBYTES  32
#define MLKEM512_SYMBYTES 32 /* seed / hash input length */

/* Encoded polynomial size: 12 bits × 256 coefficients / 8 = 384 bytes */
#define MLKEM_POLYBYTES 384
