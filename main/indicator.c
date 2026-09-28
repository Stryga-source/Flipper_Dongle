#include "indicator.h"
#include "esp_log.h"
#ifdef POCKET_DONGLE
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "pocket_display.h"
#endif

static const char *TAG = "FD_STATUS";
static indicator_state_t current = -1;
#ifdef POCKET_DONGLE
static QueueHandle_t display_queue;
static indicator_input_t current_input;

typedef struct {
    indicator_state_t state;
    indicator_input_t input;
} display_message_t;

static void display_task(void *arg)
{
    (void)arg;
    const esp_err_t err = pocket_display_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pocket display init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }
    display_message_t message;
    while (xQueueReceive(display_queue, &message, portMAX_DELAY) == pdTRUE) {
        if (message.state == IND_CONNECTED && message.input != IND_INPUT_NONE) {
            pocket_display_show_input(message.input == IND_INPUT_KEYBOARD);
            continue;
        }
        switch (message.state) {
        case IND_BOOT:         pocket_display_show("BOOT", "STARTING BLE AND USB", "WAIT"); break;
        case IND_IDLE:         pocket_display_show("IDLE", "PRESS BOOT TO PAIR", "WAITING"); break;
        case IND_SCANNING:     pocket_display_show("SCANNING", "SEARCHING BLE HID", "WAIT"); break;
        case IND_FOUND:        pocket_display_show("FOUND", "FLIPPER HID SEEN", "OPENING"); break;
        case IND_CONNECTING:   pocket_display_show("CONNECTING", "BLE SESSION OPEN", "WAIT INPUT"); break;
        case IND_CONNECTED:    pocket_display_show("CONNECTED", "HID REPORT RECEIVED", "USB QUEUED"); break;
        case IND_DISCONNECTED: pocket_display_show("NO HID", "NO BLE CONNECTION", "PRESS BOOT"); break;
        case IND_PAIR_RESET:   pocket_display_show("RESET", "BLE BONDS CLEARED", "PRESS BOOT"); break;
        default:               pocket_display_show("ERROR", "CHECK SERIAL LOG", "RETRY"); break;
        }
    }
}
#endif

static const char *state_name(indicator_state_t s)
{
    switch (s) {
    case IND_BOOT:         return "BOOT";
    case IND_IDLE:         return "IDLE";
    case IND_SCANNING:     return "PAIRING / SCANNING";
    case IND_FOUND:        return "FLIPPER FOUND";
    case IND_CONNECTING:   return "CONNECTING";
    case IND_CONNECTED:    return "CONNECTED";
    case IND_DISCONNECTED: return "DISCONNECTED";
    case IND_PAIR_RESET:   return "PAIR RESET";
    case IND_ERROR:        return "ERROR";
    default:               return "ERROR";
    }
}

void indicator_init(void)
{
    current = -1;
#ifdef POCKET_DONGLE
    current_input = IND_INPUT_NONE;
    display_queue = xQueueCreate(1, sizeof(display_message_t));
    if (display_queue) {
        if (xTaskCreate(display_task, "pocket_display", 4096, NULL, 2, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Pocket display task creation failed");
            vQueueDelete(display_queue);
            display_queue = NULL;
        }
    } else {
        ESP_LOGE(TAG, "Pocket display queue allocation failed");
    }
#endif
    indicator_set(IND_BOOT);
}

void indicator_set(indicator_state_t state)
{
    if (state == current) return;
    current = state;
    ESP_LOGI(TAG, "=== %s ===", state_name(state));
#ifdef POCKET_DONGLE
    if (state != IND_CONNECTED) current_input = IND_INPUT_NONE;
    if (display_queue) {
        display_message_t message = {.state = state, .input = current_input};
        (void)xQueueOverwrite(display_queue, &message);
    }
#endif
}

void indicator_input(indicator_input_t input)
{
#ifdef POCKET_DONGLE
    if (current != IND_CONNECTED || input == IND_INPUT_NONE ||
        input == current_input || !display_queue) return;
    current_input = input;
    display_message_t message = {.state = IND_CONNECTED, .input = input};
    (void)xQueueOverwrite(display_queue, &message);
#else
    (void)input;
#endif
}
