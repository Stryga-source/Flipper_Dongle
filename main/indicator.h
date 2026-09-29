#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    IND_BOOT = 0,
    IND_IDLE,
    IND_SCANNING,
    IND_FOUND,
    IND_CONNECTING,
    IND_CONNECTED,
    IND_DISCONNECTED,
    IND_PAIR_RESET,
    IND_ERROR,
    IND_PAIRING_CODE,
} indicator_state_t;

typedef enum {
    IND_INPUT_NONE = 0,
    IND_INPUT_KEYBOARD,
    IND_INPUT_MOUSE,
    IND_INPUT_OTHER,
    IND_INPUT_BADUSB,
} indicator_input_t;

void indicator_init(void);
void indicator_set(indicator_state_t state);
void indicator_input(indicator_input_t input);
void indicator_source_badusb(bool enabled);
void indicator_pairing_code(uint32_t code);
