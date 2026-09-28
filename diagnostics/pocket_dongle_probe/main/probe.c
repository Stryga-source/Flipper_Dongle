#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pocket_probe";

static bool test_psram(void)
{
    enum { WORD_COUNT = (64 * 1024) / sizeof(uint32_t) };
    volatile uint32_t *words = heap_caps_malloc(WORD_COUNT * sizeof(uint32_t),
                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (words == NULL) {
        ESP_LOGE(TAG, "PSRAM allocation of 64 KiB failed");
        return false;
    }

    for (size_t i = 0; i < WORD_COUNT; ++i) {
        words[i] = 0xa5a50000u ^ (uint32_t)(i * 2654435761u);
    }
    for (size_t i = 0; i < WORD_COUNT; ++i) {
        const uint32_t expected = 0xa5a50000u ^ (uint32_t)(i * 2654435761u);
        if (words[i] != expected) {
            ESP_LOGE(TAG, "PSRAM mismatch at word %u: got 0x%08" PRIx32,
                     (unsigned)i, words[i]);
            free((void *)words);
            return false;
        }
    }

    free((void *)words);
    ESP_LOGI(TAG, "PSRAM 64 KiB write/read test: PASS");
    return true;
}

void app_main(void)
{
    esp_chip_info_t chip = {0};
    uint32_t flash_bytes = 0;
    bool psram_pass = false;
    esp_chip_info(&chip);

    ESP_LOGI(TAG, "Pocket-Dongle diagnostic build; no display, SD, LED, or button GPIO is driven");
    ESP_LOGI(TAG, "Chip model=%d revision=%d cores=%d", chip.model, chip.revision, chip.cores);

    const esp_err_t flash_result = esp_flash_get_size(esp_flash_default_chip, &flash_bytes);
    if (flash_result == ESP_OK) {
        ESP_LOGI(TAG, "Flash size: %" PRIu32 " bytes", flash_bytes);
    } else {
        ESP_LOGE(TAG, "Flash size query failed: %s", esp_err_to_name(flash_result));
    }

    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM size: %u bytes", (unsigned)esp_psram_get_size());
        ESP_LOGI(TAG, "PSRAM heap: %u bytes", (unsigned)heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
        psram_pass = test_psram();
    } else {
        ESP_LOGE(TAG, "PSRAM not initialized by ESP-IDF");
    }

    ESP_LOGI(TAG, "Diagnostic complete; USB HID and peripheral pinout remain untested");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Heartbeat: flash=%" PRIu32 " bytes, PSRAM 64 KiB test=%s",
                 flash_bytes, psram_pass ? "PASS" : "FAIL");
    }
}
