#!/usr/bin/env python3
"""Convert NIST ACVP ML-KEM-512 KAT vectors into C source arrays.

Two modes:

  print KIND    Print N test vectors to stdout (the original ad-hoc paste mode).
  gen-tables    Write src/components/tests/mlkem_kat_vectors.{c,h} containing
                ALL keygen, encaps (tgId 1) and decaps (tgId 4) vectors as
                `[N][len]` byte arrays + COUNT macros. Run once whenever the
                ACVP data on disk changes; the generated files are checked in.

Examples
--------
    # Generate the full KAT table source (overwrites mlkem_kat_vectors.{c,h}):
    python3 scripts/convert_acvp_vector_to_source_code.py gen-tables

    # Print the first 2 encapsulation vectors to stdout:
    python3 scripts/convert_acvp_vector_to_source_code.py print encaps --count 2

Vector layout (ML-KEM-512, all JSON values are hex strings):
    keygen : keyGen group        prompt {d,z}    -> expected {ek,dk}    (AFT, 25)
    encaps : encapDecap tgId 1   prompt {ek,m}   -> expected {c,k}      (AFT, 25)
    decaps : encapDecap tgId 4   prompt {dk,c}   -> expected {k}        (VAL, 10)
    ekcheck: encapDecap tgId 8   prompt {ek}     -> expected {testPassed} (VAL, 10)
    dkcheck: encapDecap tgId 7   prompt {dk}     -> expected {testPassed} (VAL, 10)

ekcheck/dkcheck inputs are variable-length (malformed vectors carry wrong
lengths), so they are emitted as zero-padded [COUNT][MAXLEN] arrays plus a
per-test length array and a boolean expected-verdict array.
"""

import argparse
import glob
import json
import os
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Byte lengths per field, used to size the C arrays.
FIELD_BYTES = {
    "d": 32,
    "z": 32,
    "m": 32,
    "k": 32,
    "ek": 800,
    "dk": 1632,
    "c": 768,
}

# kind -> (json subdirectory, tgId or None, prompt fields, expected fields)
KINDS = {
    "keygen": ("ML-KEM-keyGen-FIPS203", None, ["d", "z"], ["ek", "dk"]),
    "encaps": ("ML-KEM-encapDecap-FIPS203", 1, ["ek", "m"], ["c", "k"]),
    "decaps": ("ML-KEM-encapDecap-FIPS203", 4, ["dk", "c"], ["k"]),
}

# keyCheck groups: a single (variable-length) input key per test and a boolean
# `testPassed` verdict. Unlike KINDS the inputs are NOT fixed-length — malformed
# vectors carry deliberately wrong lengths — so these get their own tables with
# a per-test length and a zero-padded [COUNT][MAXLEN] data array.
# name -> (tgId, input field)
KEYCHECKS = {
    "ekcheck": (8, "ek"),  # encapsulationKeyCheck — FIPS 203 §7.2
    "dkcheck": (7, "dk"),  # decapsulationKeyCheck — FIPS 203 §7.3
}


def find_files_dir():
    """Locate acvp-data/<version>/files, tolerating a changing version dir and
    a changing parent directory."""
    matches = glob.glob(
        os.path.join(REPO_ROOT, "**", "acvp-data", "*", "files"), recursive=True
    )
    matches = [m for m in matches if "build" + os.sep not in m]
    if not matches:
        sys.exit("error: could not find **/acvp-data/*/files under the repo root")
    return sorted(matches)[-1]


def pick_group(doc, tg_id):
    groups = doc["testGroups"]
    if tg_id is None:
        return groups[0]
    for g in groups:
        if g["tgId"] == tg_id:
            return g
    sys.exit(f"error: tgId {tg_id} not found in {[g['tgId'] for g in groups]}")


def load_kind(kind):
    """Return (tc_ids, {field: [hex_str_per_test]}) for the given KIND."""
    subdir, tg_id, prompt_fields, expected_fields = KINDS[kind]
    base = os.path.join(find_files_dir(), subdir)
    prompt = json.load(open(os.path.join(base, "prompt.json")))
    expected = json.load(open(os.path.join(base, "expectedResults.json")))
    pg = pick_group(prompt, tg_id)
    eg = pick_group(expected, tg_id)
    emap = {t["tcId"]: t for t in eg["tests"]}

    tc_ids = []
    rows = {f: [] for f in prompt_fields + expected_fields}
    for t in pg["tests"]:
        tc_ids.append(t["tcId"])
        for f in prompt_fields:
            rows[f].append(t[f])
        et = emap[t["tcId"]]
        for f in expected_fields:
            rows[f].append(et[f])

    # Sanity check: every hex string must match the documented byte length.
    for f, hex_list in rows.items():
        for i, h in enumerate(hex_list):
            if len(h) != 2 * FIELD_BYTES[f]:
                sys.exit(
                    f"error: {kind}.{f} test {tc_ids[i]} is "
                    f"{len(h)//2} B, expected {FIELD_BYTES[f]} B"
                )
    return tc_ids, rows


def c_array_body(hex_str, indent="        ", per_line=15):
    """Render a hex string as comma-separated 0xNN bytes wrapped to per_line."""
    data = bytes.fromhex(hex_str)
    tokens = [f"0x{b:02X}" for b in data]
    rows = [tokens[i : i + per_line] for i in range(0, len(tokens), per_line)]
    return "\n".join(indent + ", ".join(row) + "," for row in rows)


def emit_array(name, count, length, hex_rows, scope="static const"):
    out = [f"{scope} uint8_t {name}[{count}][{length}] = {{"]
    for hex_str in hex_rows:
        out.append("    {")
        out.append(c_array_body(hex_str, indent="        "))
        out.append("    },")
    out.append("};")
    return "\n".join(out)


def load_keycheck(tg_id, field):
    """Return (tc_ids, [hex_str], [bool_passed]) for a keyCheck test group."""
    base = os.path.join(find_files_dir(), "ML-KEM-encapDecap-FIPS203")
    prompt = json.load(open(os.path.join(base, "prompt.json")))
    expected = json.load(open(os.path.join(base, "expectedResults.json")))
    pg = pick_group(prompt, tg_id)
    eg = pick_group(expected, tg_id)
    emap = {t["tcId"]: t for t in eg["tests"]}

    tc_ids, hexes, passed = [], [], []
    for t in pg["tests"]:
        tc_ids.append(t["tcId"])
        hexes.append(t[field])
        passed.append(bool(emap[t["tcId"]]["testPassed"]))
    return tc_ids, hexes, passed


def _int_array(name, dim, values):
    out = [f"{name}[{dim}] = {{"]
    for i in range(0, len(values), 10):
        out.append("    " + ", ".join(str(v) for v in values[i : i + 10]) + ",")
    out.append("};")
    return "\n".join(out)


def emit_keycheck_header(name, tg_id, field, count, maxlen):
    up = name.upper()
    return "\n".join([
        f"/* {name}: {count} vector(s), tgId {tg_id}; variable-length inputs. */",
        f"#define KAT_{up}_COUNT {count}",
        f"#define KAT_{up}_MAXLEN {maxlen}",
        f"extern const uint8_t kat_{name}_{field}[KAT_{up}_COUNT][KAT_{up}_MAXLEN];",
        f"extern const size_t kat_{name}_{field}_len[KAT_{up}_COUNT];",
        f"extern const int kat_{name}_expected[KAT_{up}_COUNT];",
        f"extern const int kat_{name}_tc_id[KAT_{up}_COUNT];",
        "",
    ])


def emit_keycheck_source(name, tg_id, field, tc_ids, hexes, passed):
    up = name.upper()
    lens = [len(h) // 2 for h in hexes]
    out = [f"/* --- {name} ({len(tc_ids)} vectors, tgId {tg_id}) --- */", ""]
    out.append(_int_array(f"const int kat_{name}_tc_id", f"KAT_{up}_COUNT", tc_ids))
    out.append("")
    out.append(_int_array(
        f"const int kat_{name}_expected", f"KAT_{up}_COUNT",
        [1 if p else 0 for p in passed],
    ))
    out.append("")
    out.append(_int_array(
        f"const size_t kat_{name}_{field}_len", f"KAT_{up}_COUNT", lens,
    ))
    out.append("")
    # Shorter rows are zero-padded out to MAXLEN by C; the _len array carries
    # the true length each test must be checked against.
    out.append(emit_array(
        f"kat_{name}_{field}", f"KAT_{up}_COUNT", f"KAT_{up}_MAXLEN", hexes,
        scope="const",
    ))
    out.append("")
    return "\n".join(out)


# --- print mode (unchanged behaviour, scoped to function-local arrays) -----

def cmd_print(args):
    tc_ids, rows = load_kind(args.kind)
    tc_ids = tc_ids[args.start : args.start + args.count]
    if not tc_ids:
        sys.exit("error: no tests in the requested range")
    rows = {f: rows[f][args.start : args.start + args.count] for f in rows}
    n = len(tc_ids)

    subdir, tg_id, prompt_fields, expected_fields = KINDS[args.kind]
    print(
        f"/* ACVP ML-KEM-512 {subdir} "
        f"{'tgId ' + str(tg_id) + ' ' if tg_id else ''}"
        f"{n} vector(s) starting at index {args.start} (tcIds {tc_ids}). */"
    )
    for f in prompt_fields + expected_fields:
        # Indent for function-local placement, matching the original output.
        body = emit_array(f"vec_{f}", n, FIELD_BYTES[f], rows[f])
        print("    " + body.replace("\n", "\n    "))


# --- gen-tables mode -------------------------------------------------------

HEADER_TOP = """\
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
"""

SOURCE_TOP = """\
/* AUTO-GENERATED — do not edit by hand.
 *
 * Regenerate with:
 *   python scripts/convert_acvp_vector_to_source_code.py gen-tables
 */

#include "mlkem_kat_vectors.h"
"""

# How each field maps onto a C array dimension: either a params.h macro
# (clearer at the call site) or a literal byte count for the 32-byte seeds.
FIELD_DIM = {
    "d": "32",
    "z": "32",
    "m": "32",
    "k": "MLKEM512_SSBYTES",
    "ek": "MLKEM512_EKBYTES",
    "dk": "MLKEM512_DKBYTES",
    "c": "MLKEM512_CTBYTES",
}


def cmd_gen_tables(args):
    out_dir = os.path.join(REPO_ROOT, args.out_dir)
    if not os.path.isdir(out_dir):
        sys.exit(f"error: output directory does not exist: {out_dir}")

    kinds_order = ["keygen", "encaps", "decaps"]
    bundle = {}
    for kind in kinds_order:
        tc_ids, rows = load_kind(kind)
        bundle[kind] = (tc_ids, rows)

    keycheck_order = ["ekcheck", "dkcheck"]
    keycheck_bundle = {}
    for name in keycheck_order:
        tg_id, field = KEYCHECKS[name]
        tc_ids, hexes, passed = load_keycheck(tg_id, field)
        maxlen = max(len(h) // 2 for h in hexes)
        keycheck_bundle[name] = (tg_id, field, tc_ids, hexes, passed, maxlen)

    # --- header --------------------------------------------------------------
    h_lines = [HEADER_TOP]
    for kind in kinds_order:
        tc_ids, rows = bundle[kind]
        _, tg_id, prompt_fields, expected_fields = KINDS[kind]
        h_lines.append(
            f"/* {kind}: {len(tc_ids)} vector(s)"
            f"{', tgId ' + str(tg_id) if tg_id else ''}. */"
        )
        h_lines.append(f"#define KAT_{kind.upper()}_COUNT {len(tc_ids)}")
        for f in prompt_fields + expected_fields:
            h_lines.append(
                f"extern const uint8_t kat_{kind}_{f}"
                f"[KAT_{kind.upper()}_COUNT][{FIELD_DIM[f]}];"
            )
        h_lines.append(
            f"extern const int kat_{kind}_tc_id[KAT_{kind.upper()}_COUNT];"
        )
        h_lines.append("")
    for name in keycheck_order:
        tg_id, field, tc_ids, _, _, maxlen = keycheck_bundle[name]
        h_lines.append(
            emit_keycheck_header(name, tg_id, field, len(tc_ids), maxlen)
        )
    h_text = "\n".join(h_lines)

    # --- source --------------------------------------------------------------
    s_lines = [SOURCE_TOP]
    for kind in kinds_order:
        tc_ids, rows = bundle[kind]
        _, _, prompt_fields, expected_fields = KINDS[kind]
        s_lines.append(f"/* --- {kind} ({len(tc_ids)} vectors) --- */")
        s_lines.append("")
        s_lines.append(
            f"const int kat_{kind}_tc_id[KAT_{kind.upper()}_COUNT] = {{"
        )
        # 10 tcIds per line.
        for i in range(0, len(tc_ids), 10):
            chunk = ", ".join(str(x) for x in tc_ids[i : i + 10])
            s_lines.append(f"    {chunk},")
        s_lines.append("};")
        s_lines.append("")
        for f in prompt_fields + expected_fields:
            name = f"kat_{kind}_{f}"
            dim = FIELD_DIM[f]
            s_lines.append(
                emit_array(name, f"KAT_{kind.upper()}_COUNT", dim, rows[f],
                           scope="const")
            )
            s_lines.append("")
    for name in keycheck_order:
        tg_id, field, tc_ids, hexes, passed, _ = keycheck_bundle[name]
        s_lines.append(
            emit_keycheck_source(name, tg_id, field, tc_ids, hexes, passed)
        )
    s_text = "\n".join(s_lines)

    h_path = os.path.join(out_dir, "mlkem_kat_vectors.h")
    c_path = os.path.join(out_dir, "mlkem_kat_vectors.c")
    with open(h_path, "w") as fh:
        fh.write(h_text)
    with open(c_path, "w") as fh:
        fh.write(s_text)
    total_bytes = sum(
        len(bundle[k][0]) * sum(FIELD_BYTES[f] for f in
                                KINDS[k][2] + KINDS[k][3])
        for k in kinds_order
    )
    print(f"wrote {h_path}")
    print(f"wrote {c_path}  ({total_bytes} bytes of KAT data)")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = ap.add_subparsers(dest="mode", required=True)

    pp = sub.add_parser("print", help="print N vectors to stdout")
    pp.add_argument("kind", choices=KINDS.keys())
    pp.add_argument("--count", type=int, default=2,
                    help="number of vectors (default 2)")
    pp.add_argument("--start", type=int, default=0,
                    help="first test index (default 0)")
    pp.set_defaults(func=cmd_print)

    pg = sub.add_parser(
        "gen-tables",
        help="generate mlkem_kat_vectors.{c,h} under src/components/tests/",
    )
    pg.add_argument(
        "--out-dir",
        default="src/components/tests",
        help="output directory, relative to repo root (default: %(default)s)",
    )
    pg.set_defaults(func=cmd_gen_tables)

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
