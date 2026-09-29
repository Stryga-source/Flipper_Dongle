#include "indicator.h"
#include <stdatomic.h>
#include "esp_log.h"
#ifdef BOARD_SCENE_DISPLAY
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "pocket_display.h"
#endif

static const char *TAG = "FD_STATUS";
static indicator_state_t current = -1;
#ifdef BOARD_SCENE_DISPLAY
static QueueHandle_t display_queue;
static indicator_input_t current_input;
static bool badusb_source;
static atomic_bool pairing_code_active;

typedef struct {
    indicator_state_t state;
    indicator_input_t input;
    uint32_t code;
} display_message_t;

static void display_task(void *arg)
{
    (void)arg;
    const esp_err_t err = pocket_display_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pocket display init failed: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }
    display_message_t message = {.state = IND_BOOT};
    unsigned scan_frame = 0;
    while (1) {
        const TickType_t wait = message.state == IND_SCANNING
                                    ? pdMS_TO_TICKS(350) : portMAX_DELAY;
        display_message_t next;
        if (xQueueReceive(display_queue, &next, wait) == pdTRUE) {
            message = next;
            scan_frame = 0;
        } else if (message.state == IND_SCANNING) {
            scan_frame = (scan_frame + 1) % 3;
        }
        if (message.state == IND_CONNECTED) {
            pocket_scene_t scene = POCKET_SCENE_FOUND;
            if (message.input == IND_INPUT_KEYBOARD) scene = POCKET_SCENE_KEYBOARD;
            else if (message.input == IND_INPUT_MOUSE) scene = POCKET_SCENE_MOUSE;
            else if (message.input == IND_INPUT_OTHER) scene = POCKET_SCENE_OTHER;
            else if (message.input == IND_INPUT_BADUSB) scene = POCKET_SCENE_BADUSB;
            pocket_display_show_scene(scene);
            continue;
        }
        switch (message.state) {
        case IND_BOOT:
        case IND_IDLE:
        case IND_DISCONNECTED:
        case IND_PAIR_RESET:
            pocket_display_show_scene(POCKET_SCENE_IDLE); break;
        case IND_SCANNING:
            pocket_display_show_scene(POCKET_SCENE_SCAN_1 + scan_frame); break;
        case IND_FOUND:
        case IND_CONNECTING:
            pocket_display_show_scene(POCKET_SCENE_FOUND); break;
        case IND_PAIRING_CODE:
            pocket_display_show_pairing_code(message.code);
            /* Later status messages stay queued until BLE pairing resolves. */
            while (atomic_load(&pairing_code_active)) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            break;
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
    case IND_PAIRING_CODE: return "PAIRING CODE";
    case IND_ERROR:        return "ERROR";
    default:               return "ERROR";
    }
}

void indicator_init(void)
{
    current = -1;
#ifdef BOARD_SCENE_DISPLAY
    current_input = IND_INPUT_NONE;
    badusb_source = false;
    atomic_store(&pairing_code_active, false);
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
#ifdef BOARD_SCENE_DISPLAY
    if (state != IND_CONNECTED) current_input = IND_INPUT_NONE;
    if (display_queue) {
        display_message_t message = {.state = state, .input = current_input};
        (void)xQueueOverwrite(display_queue, &message);
    }
#endif
}

void indicator_input(indicator_input_t input)
{
#ifdef BOARD_SCENE_DISPLAY
    if (badusb_source) input = IND_INPUT_BADUSB;
    if (current != IND_CONNECTED || input == IND_INPUT_NONE ||
        input == current_input || !display_queue) return;
    current_input = input;
    display_message_t message = {.state = IND_CONNECTED, .input = input};
    (void)xQueueOverwrite(display_queue, &message);
#else
    (void)input;
#endif
}

void indicator_source_badusb(bool enabled)
{
#ifdef BOARD_SCENE_DISPLAY
    badusb_source = enabled;
#else
    (void)enabled;
#endif
}

void indicator_pairing_code(uint32_t code)
{
#ifdef BOARD_SCENE_DISPLAY
    current = IND_PAIRING_CODE;
    atomic_store(&pairing_code_active, true);
    current_input = IND_INPUT_NONE;
    ESP_LOGI(TAG, "=== PAIRING CODE ===");
    if (display_queue) {
        display_message_t message = {
            .state = IND_PAIRING_CODE, .input = IND_INPUT_NONE, .code = code,
        };
        (void)xQueueOverwrite(display_queue, &message);
    }
#else
    (void)code;
#endif
}

void indicator_pairing_done(void)
{
#ifdef BOARD_SCENE_DISPLAY
    if (atomic_exchange(&pairing_code_active, false)) {
        ESP_LOGI(TAG, "Pairing code display complete");
        if (current == IND_PAIRING_CODE) indicator_set(IND_CONNECTING);
    }
#endif
}
