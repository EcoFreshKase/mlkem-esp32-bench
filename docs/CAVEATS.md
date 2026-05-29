# Implementation caveats

This is a **naive, from-scratch ML-KEM-512** (FIPS 203) implementation written for
cycle-count benchmarking on the ESP32-WROOM-32D. It passes the NIST ACVP KAT vectors
(keyGen, encapsulation, decapsulation) but is deliberately *not* a hardened production
library. The deviations below are intentional and scoped; they are documented here so the
thesis can state them explicitly and so any later optimized variant has a clear baseline to
compare against.

## 1. No constant-time / side-channel resistance (out of scope)

Side-channel resistance is explicitly **not** a goal of this work.

- Modular reduction (`reduce_mod_q`, `fqmul` in `ntt.c`; `freeze` in `poly.c`) uses the C
  `%` operator on data-dependent values. Division timing and branch behaviour can depend on
  the operands, so execution time is **not** input-independent.
- `poly_sample_ntt` (SampleNTT) is rejection sampling: its running time depends on the XOF
  output, as in every ML-KEM implementation.
- Decaps uses a best-effort constant-time `ct_neq`/`ct_cmov` pair for the implicit-rejection
  selection, but this is **not** verified against timing or power analysis and should not be
  relied upon for security. It is present only because the Fujisaki–Okamoto transform needs
  a selection step, not as a hardening claim.

## 2. Naive modular arithmetic (the point of the baseline)

No Montgomery or Barrett reduction, no SIMD, no assembly. Every modular multiply goes
through a 32-bit multiply followed by a `%` (hardware/software division). This is correct
and readable but slow — it is the baseline the thesis intends to measure. An optimized
implementation would replace these with Montgomery/Barrett reduction and is expected to be
substantially faster; the `%`-based path is the obvious first target for such a comparison.

## 3. Single parameter set

Only **ML-KEM-512** is implemented (`K=2`, `eta1=3`, `eta2=2`, `du=10`, `dv=4`). The other
FIPS 203 parameter sets (ML-KEM-768, ML-KEM-1024) are not supported; several sizes are
hardcoded via `params.h`.

## 4. Stack-only, large frames

No heap and no `static` locals in the crypto path (reentrant). Intermediate `polyvec_t`
values are ~1 KB each and the sampled matrix is `K*K` polynomials (~2 KB), so keygen/encaps/
decaps frames consume several KB of stack — including the re-encryption inside decaps, which
nests a full encrypt frame. This relies on the configured 32 KB main-task stack
(`CONFIG_ESP_MAIN_TASK_STACK_SIZE=32768`). Running on a smaller stack would overflow.

## 5. Randomness

`mlkem512_keygen` / `mlkem512_encaps` draw `d`, `z`, and `m` from `esp_fill_random()`
(ESP-IDF CSPRNG). Its entropy quality depends on the hardware RNG being properly seeded
(RF subsystem active, or bootloader entropy). For deterministic KAT testing the `_internal`
variants take caller-supplied seeds, bypassing the RNG entirely.

## 6. Input validation / key checks

- The FIPS 203 §7.2/§7.3 key checks **are** implemented: `mlkem512_check_ek` enforces the
  encapsulation-key length (800 B) and the modulus check (every decoded 12-bit `t̂`
  coefficient `< q`); `mlkem512_check_dk` enforces the decapsulation-key length (1632 B) and
  the embedded `H(ek)` hash check. They run at the top of `mlkem512_encaps` / `mlkem512_decaps`
  and are exercised by the ACVP `encapsulationKeyCheck` / `decapsulationKeyCheck` groups
  (tgId 7/8) and the Wycheproof negative cases (see §9).
- `poly_frombytes` (ByteDecode_12) masks each coefficient to 12 bits but does **not** itself
  reject values `>= q`, matching the pq-crystals reference behaviour — the modulus check lives
  in `mlkem512_check_ek`, not in the decoder.
- **Ciphertext length is not validated.** The decaps/encaps API takes fixed-size buffers
  (`uint8_t[MLKEM512_CTBYTES]`), so a wrong-length ciphertext cannot even be expressed at the
  call boundary; length enforcement is delegated to the caller (see §9).

## 7. NTT structure

The NTT is the standard Kyber incomplete transform down to degree-2 factors (7 layers), with
the final inverse scaled by `128^{-1} mod q = 3303`. The `zetas` table is stored in **plain
form** (`17^BitRev7(i) mod q`), not Montgomery form. Results are bit-identical to a Montgomery
NTT; only the arithmetic representation (and therefore performance) differs.

## 8. Benchmark methodology

- Timing uses `esp_cpu_get_cycle_count()`, a **32-bit** counter that wraps roughly every
  17.9 s at 240 MHz. A single ML-KEM operation is far shorter than one wrap, and the
  `t1 - t0` unsigned subtraction is correct across at most one wrap — but a measurement
  window must never span two wraps.
- Code executes from flash through the instruction cache. `BENCH_WARMUP_ITERS` warm-up
  iterations prime the I-cache and branch predictors before timing, but first-touch flash
  fetch effects are only partially controlled.
- The measured window covers only the operation call; `printf` of the CSV row is outside it.
- CPU frequency is assumed to be 240 MHz (`CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ_240`); the
  reported `BENCH_CPU_FREQ_MHZ` must match the actual `sdkconfig` setting for cycles→time
  conversion to be valid.

## 9. Wycheproof coverage (519 of 561 ML-KEM-512 cases wired)

On top of the NIST ACVP KATs, the Google/C2SP **Wycheproof** ML-KEM-512 vectors are embedded
and run on-device (`mlkem_wycheproof_vectors.{c,h}`, generated by
`scripts/convert_wycheproof_vector_to_source_code.py`). They add adversarial / edge-case
coverage — malformed keys, modulus overflow, corrupted `H(ek)`, implicit-rejection edges —
beyond ACVP's happy-path KATs. Mapping:

| Wycheproof source | Wired as | Cases |
|---|---|---|
| `mlkem_512_keygen_seed_test.json` (valid) | `wp keygen` KAT | 100 |
| `mlkem_512_encaps_test.json` (valid) | `wp encaps` KAT | 133 |
| `mlkem_512_encaps_test.json` (invalid) | `wp ekcheck` (reject) | 128 |
| `mlkem_512_test.json` (valid) | `wp decaps` (seed→keygen→decaps) | 153 |
| `mlkem_512_semi_expanded_decaps_test.json` | `wp dkcheck` (accept/reject) | 5 |

**42 cases are deliberately skipped** because our fixed-buffer API cannot represent a
wrong-length input at the call boundary (the buffer type fixes the length):

- **40** from `mlkem_512_test.json` (invalid) — malformed seed / ciphertext *lengths*
  ("Private key too short", etc.). `mlkem512_keygen_internal` / `mlkem512_decaps` take
  fixed-size arrays, so a wrong-length seed or ciphertext is unrepresentable.
- **2** from `mlkem_512_semi_expanded_decaps_test.json` — `IncorrectCiphertextLength`
  (767 / 769 B). The naive fixed-buffer decaps has no ciphertext-length validator.

This mirrors **mlkem-native**, which delegates length checks to its harness's `decode_hex`
rather than the crypto core. Wrong-*key*-length cases are still covered: `check_ek` / `check_dk`
take an explicit `len` argument, so the `wp ekcheck` / `wp dkcheck` groups do exercise
length-based rejection.
