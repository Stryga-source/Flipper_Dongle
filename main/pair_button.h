#pragma once
#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    PAIR_BUTTON_NONE = 0,
    PAIR_BUTTON_SHORT,
    PAIR_BUTTON_LONG,
} pair_button_event_t;

esp_err_t pair_button_init(void);
pair_button_event_t pair_button_poll(void);
