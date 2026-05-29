/* AUTO-GENERATED — do not edit by hand.
 *
 * Regenerate with:
 *   python scripts/convert_wycheproof_vector_to_source_code.py
 *
 * Source: Google/C2SP Wycheproof ML-KEM-512 test vectors under test-data/wycheproof/.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "params.h"

/* wp_keygen: 100 vector(s). */
#define KAT_WP_KEYGEN_COUNT 100
extern const uint8_t kat_wp_keygen_seed[KAT_WP_KEYGEN_COUNT][64];
extern const uint8_t kat_wp_keygen_ek[KAT_WP_KEYGEN_COUNT][MLKEM512_EKBYTES];
extern const uint8_t kat_wp_keygen_dk[KAT_WP_KEYGEN_COUNT][MLKEM512_DKBYTES];
extern const int kat_wp_keygen_tc_id[KAT_WP_KEYGEN_COUNT];

/* wp_encaps: 133 vector(s). */
#define KAT_WP_ENCAPS_COUNT 133
extern const uint8_t kat_wp_encaps_ek[KAT_WP_ENCAPS_COUNT][MLKEM512_EKBYTES];
extern const uint8_t kat_wp_encaps_m[KAT_WP_ENCAPS_COUNT][32];
extern const uint8_t kat_wp_encaps_c[KAT_WP_ENCAPS_COUNT][MLKEM512_CTBYTES];
extern const uint8_t kat_wp_encaps_k[KAT_WP_ENCAPS_COUNT][MLKEM512_SSBYTES];
extern const int kat_wp_encaps_tc_id[KAT_WP_ENCAPS_COUNT];

/* wp_decaps: 153 vector(s). */
#define KAT_WP_DECAPS_COUNT 153
extern const uint8_t kat_wp_decaps_seed[KAT_WP_DECAPS_COUNT][64];
extern const uint8_t kat_wp_decaps_ek[KAT_WP_DECAPS_COUNT][MLKEM512_EKBYTES];
extern const uint8_t kat_wp_decaps_c[KAT_WP_DECAPS_COUNT][MLKEM512_CTBYTES];
extern const uint8_t kat_wp_decaps_k[KAT_WP_DECAPS_COUNT][MLKEM512_SSBYTES];
extern const int kat_wp_decaps_tc_id[KAT_WP_DECAPS_COUNT];

/* wp_ekcheck: 128 vector(s); variable-length inputs. */
#define KAT_WP_EKCHECK_COUNT 128
#define KAT_WP_EKCHECK_MAXLEN 829
extern const uint8_t kat_wp_ekcheck_ek[KAT_WP_EKCHECK_COUNT][KAT_WP_EKCHECK_MAXLEN];
extern const size_t kat_wp_ekcheck_ek_len[KAT_WP_EKCHECK_COUNT];
extern const int kat_wp_ekcheck_expected[KAT_WP_EKCHECK_COUNT];
extern const int kat_wp_ekcheck_tc_id[KAT_WP_EKCHECK_COUNT];

/* wp_dkcheck: 5 vector(s); variable-length inputs. */
#define KAT_WP_DKCHECK_COUNT 5
#define KAT_WP_DKCHECK_MAXLEN 1633
extern const uint8_t kat_wp_dkcheck_dk[KAT_WP_DKCHECK_COUNT][KAT_WP_DKCHECK_MAXLEN];
extern const size_t kat_wp_dkcheck_dk_len[KAT_WP_DKCHECK_COUNT];
extern const int kat_wp_dkcheck_expected[KAT_WP_DKCHECK_COUNT];
extern const int kat_wp_dkcheck_tc_id[KAT_WP_DKCHECK_COUNT];
