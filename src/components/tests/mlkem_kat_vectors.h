/* AUTO-GENERATED — do not edit by hand.
 *
 * Regenerate with:
 *   python scripts/convert_acvp_vector_to_source_code.py gen-tables
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

/* ekcheck: 10 vector(s), tgId 8; variable-length inputs. */
#define KAT_EKCHECK_COUNT 10
#define KAT_EKCHECK_MAXLEN 1216
extern const uint8_t kat_ekcheck_ek[KAT_EKCHECK_COUNT][KAT_EKCHECK_MAXLEN];
extern const size_t kat_ekcheck_ek_len[KAT_EKCHECK_COUNT];
extern const int kat_ekcheck_expected[KAT_EKCHECK_COUNT];
extern const int kat_ekcheck_tc_id[KAT_EKCHECK_COUNT];

/* dkcheck: 10 vector(s), tgId 7; variable-length inputs. */
#define KAT_DKCHECK_COUNT 10
#define KAT_DKCHECK_MAXLEN 1632
extern const uint8_t kat_dkcheck_dk[KAT_DKCHECK_COUNT][KAT_DKCHECK_MAXLEN];
extern const size_t kat_dkcheck_dk_len[KAT_DKCHECK_COUNT];
extern const int kat_dkcheck_expected[KAT_DKCHECK_COUNT];
extern const int kat_dkcheck_tc_id[KAT_DKCHECK_COUNT];
