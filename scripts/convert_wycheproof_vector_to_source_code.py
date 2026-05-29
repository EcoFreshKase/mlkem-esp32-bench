#!/usr/bin/env python3
"""Convert Google/C2SP Wycheproof ML-KEM-512 test vectors into C source arrays.

Wycheproof targets common crypto attack/edge cases. This emits
src/components/tests/mlkem_wycheproof_vectors.{c,h} containing the ML-KEM-512
vectors as byte arrays + COUNT macros, mirroring the ACVP table layout produced
by convert_acvp_vector_to_source_code.py. Run once whenever the on-disk
Wycheproof data changes; the generated files are checked in.

Usage
-----
    python3 scripts/convert_wycheproof_vector_to_source_code.py
    python3 scripts/convert_wycheproof_vector_to_source_code.py --out-dir src/components/tests

Source files (under test-data/wycheproof/.wycheproof-data/):

  mlkem_512_keygen_seed_test.json      100 valid : seed(64)=d‖z -> ek, dk
  mlkem_512_encaps_test.json           valid     : ek, m        -> c, K
                                       invalid   : ek (modulus/length) — reject
  mlkem_512_test.json (combined)       valid     : seed -> keygen -> decaps(c)=K
  mlkem_512_semi_expanded_decaps_test  dk-validity: dk (length/hash) accept/reject

Our fixed-buffer ML-KEM API cannot represent wrong-length ciphertext/seed/m, so
those negative cases (combined-invalid; semi-expanded IncorrectCiphertextLength)
are skipped here and documented in docs/CAVEATS.md.

Emitted tables
--------------
  wp_keygen  (fixed)   : seed, ek, dk             — keygen KAT
  wp_encaps  (fixed)   : ek, m, c, k              — encaps KAT (valid only)
  wp_ekcheck (var-len) : ek, ek_len, expected     — check_ek reject (encaps invalid)
  wp_decaps  (fixed)   : seed, ek, c, k           — combined decaps KAT (valid only)
  wp_dkcheck (var-len) : dk, dk_len, expected     — check_dk accept/reject
"""

import argparse
import json
import os
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_DIR = os.path.join(REPO_ROOT, "test-data", "wycheproof", ".wycheproof-data")

# Fixed-length fields: JSON field name -> (C array suffix, byte length, C dim macro).
FIELD = {
    "seed": ("seed", 64, "64"),
    "ek": ("ek", 800, "MLKEM512_EKBYTES"),
    "dk": ("dk", 1632, "MLKEM512_DKBYTES"),
    "m": ("m", 32, "32"),
    "c": ("c", 768, "MLKEM512_CTBYTES"),
    "K": ("k", 32, "MLKEM512_SSBYTES"),
}


def load(name):
    path = os.path.join(DATA_DIR, name)
    if not os.path.isfile(path):
        sys.exit(f"error: missing Wycheproof data file: {path}")
    return json.load(open(path))


def all_tests(doc):
    return [t for tg in doc["testGroups"] for t in tg["tests"]]


# --- formatting helpers (match the ACVP generator's style) -----------------

def c_array_body(hex_str, indent="        ", per_line=15):
    data = bytes.fromhex(hex_str)
    tokens = [f"0x{b:02X}" for b in data]
    rows = [tokens[i : i + per_line] for i in range(0, len(tokens), per_line)]
    return "\n".join(indent + ", ".join(row) + "," for row in rows)


def emit_byte_array(name, count, dim, hex_rows):
    out = [f"const uint8_t {name}[{count}][{dim}] = {{"]
    for hex_str in hex_rows:
        out.append("    {")
        out.append(c_array_body(hex_str))
        out.append("    },")
    out.append("};")
    return "\n".join(out)


def emit_int_array(decl, dim, values):
    out = [f"{decl}[{dim}] = {{"]
    for i in range(0, len(values), 10):
        out.append("    " + ", ".join(str(v) for v in values[i : i + 10]) + ",")
    out.append("};")
    return "\n".join(out)


# --- table builders --------------------------------------------------------

def build_fixed(kind, tests, fields):
    """Validate byte lengths and collect tc_ids + per-field hex rows."""
    tc_ids = [t["tcId"] for t in tests]
    rows = {}
    for jf in fields:
        suffix, nbytes, _dim = FIELD[jf]
        col = []
        for t in tests:
            h = t[jf]
            if len(h) != 2 * nbytes:
                sys.exit(
                    f"error: wp_{kind}.{jf} tcId {t['tcId']} is "
                    f"{len(h)//2} B, expected {nbytes} B"
                )
            col.append(h)
        rows[jf] = col
    return tc_ids, rows


def build_keycheck(tests, field):
    """Variable-length inputs: collect hex, lengths, and accept/reject verdict."""
    tc_ids = [t["tcId"] for t in tests]
    hexes = [t[field] for t in tests]
    expected = [1 if t["result"] == "valid" else 0 for t in tests]
    return tc_ids, hexes, expected


HEADER_TOP = """\
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
"""

SOURCE_TOP = """\
/* AUTO-GENERATED — do not edit by hand.
 *
 * Regenerate with:
 *   python scripts/convert_wycheproof_vector_to_source_code.py
 */

#include "mlkem_wycheproof_vectors.h"
"""


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--out-dir",
        default="src/components/tests",
        help="output directory, relative to repo root (default: %(default)s)",
    )
    args = ap.parse_args()
    out_dir = os.path.join(REPO_ROOT, args.out_dir)
    if not os.path.isdir(out_dir):
        sys.exit(f"error: output directory does not exist: {out_dir}")

    keygen_doc = load("mlkem_512_keygen_seed_test.json")
    encaps_doc = load("mlkem_512_encaps_test.json")
    combined_doc = load("mlkem_512_test.json")
    semidecaps_doc = load("mlkem_512_semi_expanded_decaps_test.json")

    # wp_keygen — all keygen_seed cases are valid KATs.
    kg_tests = [t for t in all_tests(keygen_doc) if t["result"] == "valid"]
    # wp_encaps — valid encaps KATs.
    enc_valid = [t for t in all_tests(encaps_doc) if t["result"] == "valid"]
    # wp_ekcheck — invalid encaps cases are all ek-validity failures (modulus
    # overflow / not-reduced / wrong length); route through check_ek, expect reject.
    enc_invalid = [t for t in all_tests(encaps_doc) if t["result"] == "invalid"]
    # wp_decaps — combined valid cases: seed -> keygen -> decaps(c) == K.
    comb_valid = [t for t in all_tests(combined_doc) if t["result"] == "valid"]
    # wp_dkcheck — dk-validity: keep length + invalid-key cases, drop the
    # IncorrectCiphertextLength cases (valid dk, wrong ct length — unrepresentable).
    dk_tests = [
        t for t in all_tests(semidecaps_doc)
        if "IncorrectCiphertextLength" not in t.get("flags", [])
    ]

    # --- collect ------------------------------------------------------------
    kg_ids, kg_rows = build_fixed("keygen", kg_tests, ["seed", "ek", "dk"])
    enc_ids, enc_rows = build_fixed("encaps", enc_valid, ["ek", "m", "c", "K"])
    dec_ids, dec_rows = build_fixed("decaps", comb_valid, ["seed", "ek", "c", "K"])
    ekc_ids, ekc_hex, ekc_exp = build_keycheck(enc_invalid, "ek")
    dkc_ids, dkc_hex, dkc_exp = build_keycheck(dk_tests, "dk")
    ekc_maxlen = max(len(h) // 2 for h in ekc_hex)
    dkc_maxlen = max(len(h) // 2 for h in dkc_hex)

    fixed = [
        ("keygen", kg_ids, kg_rows, ["seed", "ek", "dk"]),
        ("encaps", enc_ids, enc_rows, ["ek", "m", "c", "K"]),
        ("decaps", dec_ids, dec_rows, ["seed", "ek", "c", "K"]),
    ]
    keycheck = [
        ("ekcheck", "ek", ekc_ids, ekc_hex, ekc_exp, ekc_maxlen),
        ("dkcheck", "dk", dkc_ids, dkc_hex, dkc_exp, dkc_maxlen),
    ]

    # --- header -------------------------------------------------------------
    h = [HEADER_TOP]
    for kind, ids, _rows, fields in fixed:
        up = kind.upper()
        h.append(f"/* wp_{kind}: {len(ids)} vector(s). */")
        h.append(f"#define KAT_WP_{up}_COUNT {len(ids)}")
        for jf in fields:
            suffix, _nb, dim = FIELD[jf]
            h.append(
                f"extern const uint8_t kat_wp_{kind}_{suffix}"
                f"[KAT_WP_{up}_COUNT][{dim}];"
            )
        h.append(f"extern const int kat_wp_{kind}_tc_id[KAT_WP_{up}_COUNT];")
        h.append("")
    for name, field, ids, _hex, _exp, maxlen in keycheck:
        up = name.upper()
        h.append(f"/* wp_{name}: {len(ids)} vector(s); variable-length inputs. */")
        h.append(f"#define KAT_WP_{up}_COUNT {len(ids)}")
        h.append(f"#define KAT_WP_{up}_MAXLEN {maxlen}")
        h.append(
            f"extern const uint8_t kat_wp_{name}_{field}"
            f"[KAT_WP_{up}_COUNT][KAT_WP_{up}_MAXLEN];"
        )
        h.append(f"extern const size_t kat_wp_{name}_{field}_len[KAT_WP_{up}_COUNT];")
        h.append(f"extern const int kat_wp_{name}_expected[KAT_WP_{up}_COUNT];")
        h.append(f"extern const int kat_wp_{name}_tc_id[KAT_WP_{up}_COUNT];")
        h.append("")

    # --- source -------------------------------------------------------------
    s = [SOURCE_TOP]
    for kind, ids, rows, fields in fixed:
        up = kind.upper()
        s.append(f"/* --- wp_{kind} ({len(ids)} vectors) --- */")
        s.append("")
        s.append(emit_int_array(f"const int kat_wp_{kind}_tc_id", f"KAT_WP_{up}_COUNT", ids))
        s.append("")
        for jf in fields:
            suffix, _nb, dim = FIELD[jf]
            s.append(
                emit_byte_array(f"kat_wp_{kind}_{suffix}", f"KAT_WP_{up}_COUNT", dim, rows[jf])
            )
            s.append("")
    for name, field, ids, hexes, exp, _maxlen in keycheck:
        up = name.upper()
        lens = [len(hx) // 2 for hx in hexes]
        s.append(f"/* --- wp_{name} ({len(ids)} vectors) --- */")
        s.append("")
        s.append(emit_int_array(f"const int kat_wp_{name}_tc_id", f"KAT_WP_{up}_COUNT", ids))
        s.append("")
        s.append(emit_int_array(f"const int kat_wp_{name}_expected", f"KAT_WP_{up}_COUNT", exp))
        s.append("")
        s.append(
            emit_int_array(f"const size_t kat_wp_{name}_{field}_len", f"KAT_WP_{up}_COUNT", lens)
        )
        s.append("")
        s.append(
            emit_byte_array(
                f"kat_wp_{name}_{field}", f"KAT_WP_{up}_COUNT", f"KAT_WP_{up}_MAXLEN", hexes
            )
        )
        s.append("")

    h_path = os.path.join(out_dir, "mlkem_wycheproof_vectors.h")
    c_path = os.path.join(out_dir, "mlkem_wycheproof_vectors.c")
    with open(h_path, "w") as fh:
        fh.write("\n".join(h))
    with open(c_path, "w") as fh:
        fh.write("\n".join(s))
    print(f"wrote {h_path}")
    print(f"wrote {c_path}")
    print(
        f"  wp_keygen={len(kg_ids)} wp_encaps={len(enc_ids)} "
        f"wp_ekcheck={len(ekc_ids)} wp_decaps={len(dec_ids)} wp_dkcheck={len(dkc_ids)}"
    )


if __name__ == "__main__":
    main()
