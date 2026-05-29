/* AUTO-GENERATED — do not edit by hand.
 *
 * Regenerate with:
 *   python3 scripts/convert_acvp_vector_to_source_code.py gen-tables
 *
 * Source: NIST ACVP ML-KEM-512 KAT vectors under acvp-data/.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "params.h"

/* keygen: 25 vector(s). */
#define KAT_KEYGEN_COUNT 25
extern const uint8_t kat_keygen_d[KAT_KEYGEN_COUNT][32];
extern const uint8_t kat_keygen_z[KAT_KEYGEN_COUNT][32];
extern const uint8_t kat_keygen_ek[KAT_KEYGEN_COUNT][MLKEM512_EKBYTES];
extern const uint8_t kat_keygen_dk[KAT_KEYGEN_COUNT][MLKEM512_DKBYTES];
extern const int kat_keygen_tc_id[KAT_KEYGEN_COUNT];

/* encaps: 25 vector(s), tgId 1. */
#define KAT_ENCAPS_COUNT 25
extern const uint8_t kat_encaps_ek[KAT_ENCAPS_COUNT][MLKEM512_EKBYTES];
extern const uint8_t kat_encaps_m[KAT_ENCAPS_COUNT][32];
extern const uint8_t kat_encaps_c[KAT_ENCAPS_COUNT][MLKEM512_CTBYTES];
extern const uint8_t kat_encaps_k[KAT_ENCAPS_COUNT][MLKEM512_SSBYTES];
extern const int kat_encaps_tc_id[KAT_ENCAPS_COUNT];

/* decaps: 10 vector(s), tgId 4. */
#define KAT_DECAPS_COUNT 10
extern const uint8_t kat_decaps_dk[KAT_DECAPS_COUNT][MLKEM512_DKBYTES];
extern const uint8_t kat_decaps_c[KAT_DECAPS_COUNT][MLKEM512_CTBYTES];
extern const uint8_t kat_decaps_k[KAT_DECAPS_COUNT][MLKEM512_SSBYTES];
extern const int kat_decaps_tc_id[KAT_DECAPS_COUNT];
