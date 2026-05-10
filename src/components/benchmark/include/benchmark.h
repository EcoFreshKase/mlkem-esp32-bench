#pragma once

#include <stdint.h>

/* Number of warm-up iterations run before timing begins.
 * These fill I-cache and branch predictors without contributing to statistics. */
#define BENCH_WARMUP_ITERS   10

/* Number of timed iterations for each ML-KEM operation. */
#define BENCH_MEASURE_ITERS  1000

/* ESP32-WROOM-32D default CPU frequency (MHz).
 * Must match CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ_240 in sdkconfig.defaults.
 * Override at build time: -DBENCH_CPU_FREQ_MHZ=160 */
#ifndef BENCH_CPU_FREQ_MHZ
#define BENCH_CPU_FREQ_MHZ   240
#endif

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/* Run a single operation benchmark, printing one CSV row per iteration. */
void bench_run_keygen(uint32_t iterations);
void bench_run_encaps(uint32_t iterations);
void bench_run_decaps(uint32_t iterations);

/* Print the CSV header comment block and column row, then run all benchmarks. */
void bench_run_all(uint32_t iterations);

/* Print CSV header (comment block + column names). Called by bench_run_all. */
void bench_print_header(uint32_t cpu_mhz, uint32_t iterations);
