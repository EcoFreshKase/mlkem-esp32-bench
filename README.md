# mlkem-esp32-bench

Cycle-accurate benchmark for **ML-KEM-512** (FIPS 203) on the **ESP32-WROOM-32D**,
built with ESP-IDF v5.x.

The three ML-KEM operations—KeyGen, Encaps, and Decaps—are timed over 1 000 runs each.
Results are printed to UART0 as CSV with comment lines prefixed by `#`.

---

## Prerequisites

| Tool | Version |
|------|---------|
| ESP-IDF | v5.2 or later |
| xtensa-esp32-elf toolchain | bundled with ESP-IDF |
| Python | ≥ 3.8 |

Source and activate the ESP-IDF environment before any build step:

```bash
. $IDF_PATH/export.sh
```

---

## Directory structure

```
mlkem-esp32-bench/
├── CMakeLists.txt                  # Top-level ESP-IDF project file
├── sdkconfig.defaults              # Default Kconfig overrides (stack size, 240 MHz, WDT)
└── src/                            # All source code lives here
    ├── main/
    │   ├── CMakeLists.txt
    │   └── main.c                  # app_main: initialises and dispatches benchmarks
    └── components/
        ├── mlkem/                  # ML-KEM-512 implementation (FIPS 203)
        │   ├── CMakeLists.txt      # globs *.c; set -O2 or -O3 here
        │   ├── include/
        │   │   ├── params.h        # compile-time constants (n, q, k, sizes)
        │   │   ├── mlkem.h         # public API: keygen / encaps / decaps
        │   │   ├── poly.h          # poly_t, polyvec_t, polynomial operations
        │   │   └── ntt.h           # NTT, inverse NTT, base multiplication
        │   ├── params.c            # (reserved for runtime parameter validation)
        │   ├── keygen.c            # ML-KEM.KeyGen  – FIPS 203 Algorithm 19
        │   ├── encaps.c            # ML-KEM.Encaps  – FIPS 203 Algorithm 20
        │   ├── decaps.c            # ML-KEM.Decaps  – FIPS 203 Algorithm 21
        │   ├── ntt.c               # NTT over Z_q[x]/(x^256+1)
        │   └── poly.c              # polynomial arithmetic mod q
        └── benchmark/
            ├── CMakeLists.txt      # globs *.c; set -O2 or -O3 here
            ├── include/
            │   └── benchmark.h    # bench_result_t, BENCH_* constants, public API
            └── benchmark.c        # Welford online mean/stddev, CSV printer
```

---

## Build

```bash
# Configure (uses sdkconfig.defaults automatically)
idf.py set-target esp32

# Build
idf.py build

# Flash and open monitor (adjust port as needed)
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## Expected serial output

Progress lines are emitted via `ESP_LOGI` (format `I (ms) tag: …`).  
The CSV block is written with `printf` immediately after all measurements complete.

```
I (312) main: === ML-KEM-512 Benchmark ===
I (312) main: ESP32-WROOM-32D  |  iterations: 1000
I (312) benchmark: Warming up keygen (10 iters)...
I (350) benchmark: Measuring keygen (1000 iters)...
...
# ML-KEM-512 Benchmark - ESP32-WROOM-32D
# CPU frequency: 240 MHz
# Warmup iterations: 10
# Measurement iterations: 1000
operation,iterations,mean_cycles,stddev_cycles,min_cycles,max_cycles,mean_us,stddev_us,min_us,max_us
keygen,1000,…
encaps,1000,…
decaps,1000,…
```

Extract the CSV block from a capture file:

```bash
grep -v '^I ' capture.txt | grep -v '^W ' | grep -v '^E '
```

---

## Changing optimisation flags

Each component's `CMakeLists.txt` ends with a `target_compile_options` line:

```cmake
# components/mlkem/CMakeLists.txt
target_compile_options(${COMPONENT_LIB} PRIVATE -O2)   # change to -O3 here
```

```cmake
# components/benchmark/CMakeLists.txt
target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
```

Change `-O2` to `-O3` (or add `-Os`, `-funroll-loops`, etc.) and rebuild:

```bash
idf.py build
```

The components are compiled independently, so you can mix optimisation levels
(e.g. `-O3` for `mlkem`, `-O2` for `benchmark`) to isolate the effect.

---

## Implementing ML-KEM

The source files in `components/mlkem/` are stubs with correct FIPS 203 signatures.
Fill them in following the FIPS 203 specification:

| File | Algorithm |
|------|-----------|
| `keygen.c` | Algorithm 19 – ML-KEM.KeyGen |
| `encaps.c` | Algorithm 20 – ML-KEM.Encaps |
| `decaps.c` | Algorithm 21 – ML-KEM.Decaps |
| `ntt.c`    | NTT / InvNTT over Z_{3329}[x]/(x^{256}+1) |
| `poly.c`   | Polynomial arithmetic, CBD sampling, compression |

You will also need a SHA-3 / SHAKE provider.  ESP-IDF ships `mbedtls` which
includes SHA-3 from v3.4; alternatively link an external provider (e.g. `tiny_sha3`)
and add it as a component.

---

## Third-party code and credits

| Component | Files | Origin | Authors / Licence |
|-----------|-------|--------|-------------------|
| FIPS 202 (SHA-3 / SHAKE) | `src/components/fips202/fips202.{c,h}` | [pq-crystals/kyber](https://github.com/pq-crystals/kyber) reference implementation | Taken directly from the ML-KEM (Kyber) reference implementation by the pq-crystals team. The implementation is itself derived from two earlier public-domain works: the `crypto_hash/keccakc512/simple/` entry from the [SUPERCOP](https://bench.cr.yp.to/supercop.html) benchmark suite by **Ronny Van Keer**, and the [TweetFips202](https://twitter.com/tweetfips202) implementation by **Gilles Van Assche**, **Daniel J. Bernstein**, and **Peter Schwabe**. All three sources are released into the public domain. |

---

## Stack sizing

`polyvec_t` (k=2 polynomials × 256 × 2 bytes) is 1 024 bytes.  KeyGen and Encaps
each keep several polyvecs live simultaneously; the `CONFIG_ESP_MAIN_TASK_STACK_SIZE`
in `sdkconfig.defaults` is set to 32 768 bytes to provide sufficient headroom.
Increase it if you observe stack overflow panics after implementing the full algorithm.
