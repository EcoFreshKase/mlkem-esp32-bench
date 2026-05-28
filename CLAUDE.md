# CLAUDE.md — mlkem-esp32-bench

Project context for Claude Code sessions in this repository.

## Project overview

Bachelor's thesis. **Naive, from-scratch, plain-C implementation of ML-KEM-512** (FIPS 203)
on **ESP32-WROOM-32D**, for cycle-accurate benchmarking. "Naive" means:

- No hardware acceleration (no SIMD, no peripheral offload)
- No Montgomery tricks — plain modular arithmetic
- No assembly
- Readable C, optimized for clarity

The implementation must pass NIST ACVP KAT vectors (vectors on disk; see below).

## Scope decisions (settled)

- **From scratch per FIPS 203 final standard** (not IPD). The spec PDF is at the repo
  root: `NIST-FIPS-203.pdf`. Reference pq-crystals/kyber only for cross-checking edge
  cases (e.g. zetas table values).
- **NTT is required** for KAT-compatible serialization — encoded `ek` and `dk` store
  `t̂` and `ŝ` in the NTT domain.
- **Plain modular NTT** — no Montgomery form, no Barrett tricks beyond a basic
  reduction helper.
- **RNG**: `esp_fill_random()` from `<esp_random.h>` (ESP-IDF CSPRNG).
- **Stack-only, no heap, no `static` locals in the crypto path** (reentrant).
  32 KB main task stack already configured.
- **Best-effort constant-time** in decaps (XOR-accumulate compare, masked cmov); not
  hardened against side-channels.

## Directory map

```
mlkem-esp32-bench/
├── CMakeLists.txt              # top-level ESP-IDF project
├── sdkconfig.defaults          # 32 KB stack, 240 MHz CPU, 60 s WDT
├── NIST-FIPS-203.pdf           # spec
├── acvp-data/v1.1.0.41/files/  # NIST ACVP KAT vectors
│   ├── ML-KEM-keyGen-FIPS203/{prompt,expectedResults}.json
│   └── ML-KEM-encapDecap-FIPS203/{prompt,expectedResults}.json
├── scripts/
│   └── convert_acvp_vector_to_source_code.py
├── src/
│   ├── main/main.c
│   ├── util/                   # mod_power_of_two helper
│   └── components/
│       ├── fips202/            # SHA3-256/512, SHAKE128/256 — DONE
│       ├── mlkem/              # ML-KEM-512 — partial (see below)
│       ├── benchmark/          # cycle counter + CSV
│       └── tests/              # unit tests + KAT harness
└── run_tests.sh
```

## What's done vs. stub vs. missing

**Done:**
- `components/fips202/` — full SHA3-256/512, SHAKE128/256 (incremental absorb/squeeze)
  via `fips202.h`. Namespaced through `FIPS202_NAMESPACE`. Three unit tests pass.
- `components/mlkem/compression_util.c` — `compress(uint16_t, d)` / `decompress(uint16_t, d)`
  per FIPS 203 §4.2.1. Single-coefficient; callers must reduce inputs to `[0, q)` first.
- `components/mlkem/include/params.h` — all constants.
- `components/mlkem/include/mlkem.h` — public API (signatures).
- `components/benchmark/` — raw cycle counter via `esp_cpu_get_cycle_count()`, CSV output.
  KeyGen bench line active; encaps/decaps lines commented at `benchmark.c:110-111`.
- `components/tests/mlkem_tests.c` — one hardcoded keygen KAT (ACVP keyGen tcId 1) at
  lines 11–204; encaps/decaps test fns are stubs; round-trip scaffold lines 217–232.

**Stubs (signatures present, bodies TODO):**
- `mlkem/keygen.c`, `mlkem/encaps.c`, `mlkem/decaps.c`.

**Missing entirely:**
- `mlkem/include/poly.h` + `mlkem/poly.c` — polynomial / polyvec types, arithmetic,
  CBD sampling, ByteEncode/ByteDecode (d=12), SampleNTT, msg↔poly, compress wrappers
  (d=10, d=4).
- `mlkem/include/ntt.h` + `mlkem/ntt.c` — `poly_ntt`, `poly_invntt`, `poly_basemul`,
  `zetas[128]` (plain form), Barrett reduce, fqmul.

## Build & flash

```bash
. $IDF_PATH/export.sh             # one-time per shell
idf.py set-target esp32           # one-time
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
# or:
./run_tests.sh
```

## FIPS 203 algorithm → file map

| FIPS 203 algorithm                | File                                      |
|-----------------------------------|-------------------------------------------|
| Alg 5/6: ByteEncode/Decode (d=12) | `poly.c`                                  |
| Alg 7: SampleNTT                  | `poly.c` (via SHAKE128)                   |
| Alg 8: SamplePolyCBD              | `poly.c` (via SHAKE256)                   |
| Alg 9: NTT                        | `ntt.c`                                   |
| Alg 10: NTT⁻¹                     | `ntt.c` (final ×3303)                     |
| Alg 11/12: BaseCaseMul / MultPoly | `ntt.c` (`poly_basemul`)                  |
| Alg 13: K-PKE.KeyGen              | `keygen.c`                                |
| Alg 14: K-PKE.Encrypt             | `encaps.c`                                |
| Alg 15: K-PKE.Decrypt             | `decaps.c`                                |
| Alg 16/19: ML-KEM.KeyGen          | `keygen.c` (`_internal` + public)         |
| Alg 17/20: ML-KEM.Encaps          | `encaps.c` (`_internal` + public)         |
| Alg 18/21: ML-KEM.Decaps          | `decaps.c`                                |

## Trap list (silent-failure bugs to avoid)

1. **zetas must be PLAIN form**, not Montgomery: `zetas[i] = 17^BitRev7(i) mod 3329`,
   reduced to `[0, q)`. Do NOT copy pq-crystals' Montgomery zetas table. Generate via:
   ```bash
   python3 -c "q=3329;br=lambda i:int(f'{i:07b}'[::-1],2);print([pow(17,br(i),q) for i in range(128)])"
   ```
2. **KeyGen domain separator**: `G = SHA3-512(d ‖ k)` with the single byte `k = MLKEM512_K = 2`
   appended to `d` (33-byte input). Final FIPS 203, not IPD.
3. **Â indexing**: KeyGen samples `Â[i][j] = SampleNTT(ρ ‖ j ‖ i)` (column byte first).
   Encrypt uses the **transpose**: `Âᵀ[i][j] = SampleNTT(ρ ‖ i ‖ j)`.
4. **Encaps (K, r) split**: `(K, r) = SHA3-512(m ‖ H(ek))`. K = bytes 0–31, r = 32–63.
   Final FIPS 203 has **no** separate `H(m)` step.
5. **InvNTT scale factor** = `128⁻¹ mod q = 3303` (incomplete 7-layer NTT to degree-2
   factors), not `256⁻¹`.
6. **Compress input domain**: `compress()` expects `[0, q)`; reduce u/v coefficients
   before packing (InvNTT outputs may be centered/negative after add).
7. **dk slicing in Decaps**: `dk_pke = dk[0:768]`, `ek = dk[768:1568]`,
   `h = dk[1568:1600]`, `z = dk[1600:1632]`.

## CMake notes

`components/mlkem/CMakeLists.txt` needs `PRIV_REQUIRES fips202 esp_hw_support` so it can
include `fips202.h` (SHA-3/SHAKE) and `esp_random.h` (`esp_fill_random`). The trailing
`*.c` glob auto-includes new source files, so adding `poly.c`/`ntt.c` requires no
CMake edit beyond the requires fix.

`components/tests/CMakeLists.txt` already `REQUIRES fips202 mlkem`.

## Testing approach

- **FIPS 202 tests**: 3 hardcoded vectors in `fips202_tests.c`, pass today.
- **NTT unit tests**: round-trip (`InvNTT∘NTT == id`) and small-input convolution check
  via base-multiplication.
- **Poly unit tests**: encode round-trip, CBD on zero input, compress error bound.
- **KeyGen KAT**: one ACVP vector already wired (`mlkem_tests.c:11-204`).
- **Encaps KAT**: ~3 vectors from ACVP `encapDecap` **tgId 1** (AFT). Requires
  `mlkem512_encaps_internal(ct, ss, ek, m)` so the message is caller-supplied. Add
  this signature to `mlkem.h`.
- **Decaps KAT**: ~3 vectors from ACVP `encapDecap` **tgId 4** (VAL). Each test has
  a per-test `dk`.
- **Round-trip**: scaffold exists; auto-passes once all three ops work.

## Code style

- Conforms to `.clang-format` at repo root (LLVM-based, 4-space indent, 100-col,
  attach braces, parameters/arguments unpacked one per line when wrapping).
- See user memory `feedback_code_style.md` for additional preferences (Prettier-like).

## Reference

- `NIST-FIPS-203.pdf` (this repo) — primary spec.
- pq-crystals/kyber reference implementation — cross-check only, not copied. FIPS 202
  is the one exception (it's verbatim ported under public domain).
