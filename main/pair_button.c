#include "pair_button.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define PAIR_BUTTON_GPIO GPIO_NUM_0
#define LONG_PRESS_US (5LL * 1000LL * 1000LL)
#define DEBOUNCE_US   (40LL * 1000LL)

static bool down;
static bool long_sent;
static int64_t down_at;
static int64_t last_edge;

esp_err_t pair_button_init(void)
{
    gpio_config_t c = {
        .pin_bit_mask = 1ULL << PAIR_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&c);
}

pair_button_event_t pair_button_poll(void)
{
    const int64_t now = esp_timer_get_time();
    const bool pressed = gpio_get_level(PAIR_BUTTON_GPIO) == 0;

    if (pressed != down && now - last_edge >= DEBOUNCE_US) {
        last_edge = now;
        down = pressed;
        if (pressed) {
            down_at = now;
            long_sent = false;
        } else if (!long_sent && now - down_at >= DEBOUNCE_US) {
            return PAIR_BUTTON_SHORT;
        }
    }

    if (down && !long_sent && now - down_at >= LONG_PRESS_US) {
        long_sent = true;
        return PAIR_BUTTON_LONG;
    }
    return PAIR_BUTTON_NONE;
}
