#include "indicator.h"
#include "esp_log.h"

static const char *TAG = "FD_STATUS";
static indicator_state_t current = -1;

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
    default:               return "ERROR";
    }
}

void indicator_init(void)
{
    current = -1;
    indicator_set(IND_BOOT);
}

void indicator_set(indicator_state_t state)
{
    if (state == current) return;
    current = state;
    ESP_LOGI(TAG, "=== %s ===", state_name(state));
}
