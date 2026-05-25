#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "benchmark.h"
#include "tests.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "=== ML-KEM-512 Benchmark ===");
    ESP_LOGI(TAG, "ESP32-WROOM-32D  |  iterations: %d", BENCH_MEASURE_ITERS);

    /* Brief pause so a serial monitor can connect before output begins. */
    vTaskDelay(pdMS_TO_TICKS(200));

    bench_run_all(BENCH_MEASURE_ITERS);

    run_all_tests();

    ESP_LOGI(TAG, "Benchmark complete.");
}
