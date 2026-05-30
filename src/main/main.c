#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#if defined(CONFIG_MLKEM_BUILD_MODE_BENCH)
#include "benchmark.h"
#elif defined(CONFIG_MLKEM_BUILD_MODE_TEST)
#include "tests.h"
#endif

static const char *TAG = "main";

void app_main(void) {
    /* Brief pause so a serial monitor can connect before output begins. */
    vTaskDelay(pdMS_TO_TICKS(200));

#if defined(CONFIG_MLKEM_BUILD_MODE_BENCH)
    ESP_LOGI(TAG, "=== ML-KEM-512 Benchmark ===");
    ESP_LOGI(TAG, "ESP32-WROOM-32D  |  iterations: %d", BENCH_MEASURE_ITERS);
    bench_run_all(BENCH_MEASURE_ITERS);
    ESP_LOGI(TAG, "Benchmark complete.");
#elif defined(CONFIG_MLKEM_BUILD_MODE_TEST)
    ESP_LOGI(TAG, "=== ML-KEM-512 Tests ===");
    run_all_tests();
    ESP_LOGI(TAG, "Tests complete.");
#else
#error "Select CONFIG_MLKEM_BUILD_MODE_TEST or CONFIG_MLKEM_BUILD_MODE_BENCH in menuconfig."
#endif
}
