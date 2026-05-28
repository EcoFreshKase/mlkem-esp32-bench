#!/usr/bin/env python3
"""Convert NIST ACVP ML-KEM-512 KAT vectors into C source arrays.

Reads the on-disk ACVP prompt/expectedResults JSON and prints `static const
uint8_t` arrays (plus a memcmp loop) ready to paste into
`src/components/tests/mlkem_tests.c`.

Examples
--------
    # First 2 encapsulation vectors (ek, m -> c, k)
    python3 scripts/convert_acvp_vector_to_source_code.py encaps --count 2

    # First 3 decapsulation vectors (dk, c -> k)
    python3 scripts/convert_acvp_vector_to_source_code.py decaps --count 3

    # The keygen vector at index 0 (d, z -> ek, dk)
    python3 scripts/convert_acvp_vector_to_source_code.py keygen --count 1

Vector layout (ML-KEM-512, all JSON values are hex strings):
    keygen : keyGen group        prompt {d,z}    -> expected {ek,dk}
    encaps : encapDecap tgId 1    prompt {ek,m}   -> expected {c,k}    (AFT)
    decaps : encapDecap tgId 4    prompt {dk,c}   -> expected {k}      (VAL)
"""

import argparse
import glob
import json
import os
import sys
import textwrap

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


def find_files_dir():
    """Locate acvp-data/<version>/files, tolerating a changing version dir."""
    matches = glob.glob(os.path.join(REPO_ROOT, "acvp-data", "*", "files"))
    if not matches:
        sys.exit("error: could not find acvp-data/*/files under the repo root")
    return sorted(matches)[-1]


def pick_group(doc, tg_id):
    groups = doc["testGroups"]
    if tg_id is None:
        return groups[0]
    for g in groups:
        if g["tgId"] == tg_id:
            return g
    sys.exit(f"error: tgId {tg_id} not found in {[g['tgId'] for g in groups]}")


def c_array_body(hex_str, indent="        ", per_line=15):
    """Render a hex string as comma-separated 0xNN bytes wrapped to per_line."""
    data = bytes.fromhex(hex_str)
    tokens = [f"0x{b:02X}" for b in data]
    rows = [tokens[i:i + per_line] for i in range(0, len(tokens), per_line)]
    return "\n".join(indent + ", ".join(row) + "," for row in rows)


def emit_array(name, count, length, hex_rows):
    out = [f"    static const uint8_t {name}[{count}][{length}] = {{"]
    for hex_str in hex_rows:
        out.append("        {")
        out.append(c_array_body(hex_str))
        out.append("        },")
    out.append("    };")
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("kind", choices=KINDS.keys())
    ap.add_argument("--count", type=int, default=2, help="number of vectors (default 2)")
    ap.add_argument("--start", type=int, default=0, help="first test index (default 0)")
    args = ap.parse_args()

    subdir, tg_id, prompt_fields, expected_fields = KINDS[args.kind]
    base = os.path.join(find_files_dir(), subdir)
    prompt = json.load(open(os.path.join(base, "prompt.json")))
    expected = json.load(open(os.path.join(base, "expectedResults.json")))

    pg = pick_group(prompt, tg_id)
    eg = pick_group(expected, tg_id)
    emap = {t["tcId"]: t for t in eg["tests"]}

    tests = pg["tests"][args.start:args.start + args.count]
    if not tests:
        sys.exit("error: no tests in the requested range")
    n = len(tests)

    # Collect the hex rows per field.
    rows = {f: [] for f in prompt_fields + expected_fields}
    for t in tests:
        for f in prompt_fields:
            rows[f].append(t[f])
        et = emap[t["tcId"]]
        for f in expected_fields:
            rows[f].append(et[f])

    print(f"/* ACVP ML-KEM-512 {subdir} "
          f"{'tgId ' + str(tg_id) + ' ' if tg_id else ''}"
          f"({pg.get('function', 'keyGen')}, {pg.get('testType', '')}), "
          f"{n} vector(s) from index {args.start}. */")
    for f in prompt_fields + expected_fields:
        print(emit_array(f"vec_{f}", n, FIELD_BYTES[f], rows[f]))


if __name__ == "__main__":
    main()
