#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "benchmark.h"
#include "fips202.h"

static const char *TAG = "main";

static void print_hex(const char *label, const uint8_t *buf, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++)
        printf("%02x", buf[i]);
    printf("\n");
}

static void fips202_smoke_test(void)
{
    const uint8_t msg[] = "abc";
    const size_t  msglen = 3;

    ESP_LOGI(TAG, "--- FIPS 202 smoke test (input: \"abc\") ---");

    uint8_t sha3_256_out[32];
    sha3_256(sha3_256_out, msg, msglen);
    print_hex("SHA3-256", sha3_256_out, sizeof(sha3_256_out));

    uint8_t sha3_512_out[64];
    sha3_512(sha3_512_out, msg, msglen);
    print_hex("SHA3-512", sha3_512_out, sizeof(sha3_512_out));

    uint8_t shake256_out[32];
    shake256(shake256_out, sizeof(shake256_out), msg, msglen);
    print_hex("SHAKE256", shake256_out, sizeof(shake256_out));

    ESP_LOGI(TAG, "--- FIPS 202 smoke test complete ---");
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== ML-KEM-512 Benchmark ===");
    ESP_LOGI(TAG, "ESP32-WROOM-32D  |  iterations: %d", BENCH_MEASURE_ITERS);

    /* Brief pause so a serial monitor can connect before output begins. */
    vTaskDelay(pdMS_TO_TICKS(200));

    bench_run_all(BENCH_MEASURE_ITERS);

    fips202_smoke_test();

    ESP_LOGI(TAG, "Benchmark complete.");
}
