#include <stdio.h>
#include <stdint.h>
#include "esp_cpu.h"
#include "esp_log.h"
#include "benchmark.h"
#include "mlkem.h"

static const char *TAG = "benchmark";

/* -----------------------------------------------------------------------
 * Per-operation benchmarks — each iteration emits one CSV row.
 * ----------------------------------------------------------------------- */

void bench_run_keygen(uint32_t iterations) {
    static uint8_t ek[MLKEM512_EKBYTES];
    static uint8_t dk[MLKEM512_DKBYTES];

    ESP_LOGI(TAG, "Warming up keygen (%d iters)...", BENCH_WARMUP_ITERS);
    for (int i = 0; i < BENCH_WARMUP_ITERS; i++) {
        mlkem512_keygen(ek, dk);
    }

    ESP_LOGI(TAG, "Measuring keygen (%lu iters)...", (unsigned long)iterations);
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t t0 = esp_cpu_get_cycle_count();
        mlkem512_keygen(ek, dk);
        uint32_t t1 = esp_cpu_get_cycle_count();
        printf(
            "keygen,%lu,%lu,%lu,%lu\n",
            (unsigned long)i,
            (unsigned long)t0,
            (unsigned long)t1,
            (unsigned long)(t1 - t0)
        );
    }
}

void bench_run_encaps(uint32_t iterations) {
    static uint8_t ek[MLKEM512_EKBYTES];
    static uint8_t dk[MLKEM512_DKBYTES];
    static uint8_t ct[MLKEM512_CTBYTES];
    static uint8_t ss[MLKEM512_SSBYTES];

    mlkem512_keygen(ek, dk);

    ESP_LOGI(TAG, "Warming up encaps (%d iters)...", BENCH_WARMUP_ITERS);
    for (int i = 0; i < BENCH_WARMUP_ITERS; i++) {
        mlkem512_encaps(ct, ss, ek);
    }

    ESP_LOGI(TAG, "Measuring encaps (%lu iters)...", (unsigned long)iterations);
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t t0 = esp_cpu_get_cycle_count();
        mlkem512_encaps(ct, ss, ek);
        uint32_t t1 = esp_cpu_get_cycle_count();
        printf(
            "encaps,%lu,%lu,%lu,%lu\n",
            (unsigned long)i,
            (unsigned long)t0,
            (unsigned long)t1,
            (unsigned long)(t1 - t0)
        );
    }
}

void bench_run_decaps(uint32_t iterations) {
    static uint8_t ek[MLKEM512_EKBYTES];
    static uint8_t dk[MLKEM512_DKBYTES];
    static uint8_t ct[MLKEM512_CTBYTES];
    static uint8_t ss[MLKEM512_SSBYTES];

    mlkem512_keygen(ek, dk);
    mlkem512_encaps(ct, ss, ek);

    ESP_LOGI(TAG, "Warming up decaps (%d iters)...", BENCH_WARMUP_ITERS);
    for (int i = 0; i < BENCH_WARMUP_ITERS; i++) {
        mlkem512_decaps(ss, ct, dk);
    }

    ESP_LOGI(TAG, "Measuring decaps (%lu iters)...", (unsigned long)iterations);
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t t0 = esp_cpu_get_cycle_count();
        mlkem512_decaps(ss, ct, dk);
        uint32_t t1 = esp_cpu_get_cycle_count();
        printf(
            "decaps,%lu,%lu,%lu,%lu\n",
            (unsigned long)i,
            (unsigned long)t0,
            (unsigned long)t1,
            (unsigned long)(t1 - t0)
        );
    }
}

/* -----------------------------------------------------------------------
 * CSV output
 * ----------------------------------------------------------------------- */

void bench_print_header(uint32_t cpu_mhz, uint32_t iterations) {
    printf("# ML-KEM-512 Benchmark - ESP32-WROOM-32D\n");
    printf("# CPU frequency: %lu MHz\n", (unsigned long)cpu_mhz);
    printf("# Warmup iterations: %d\n", BENCH_WARMUP_ITERS);
    printf("# Measurement iterations: %lu\n", (unsigned long)iterations);
    printf("operation,iteration,start_cycle,end_cycle,cycles\n");
}

void bench_run_all(uint32_t iterations) {
    bench_print_header(BENCH_CPU_FREQ_MHZ, iterations);
    bench_run_keygen(iterations);
    bench_run_encaps(iterations);
    bench_run_decaps(iterations);
}
