#pragma once

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
} indicator_state_t;

typedef enum {
    IND_INPUT_NONE = 0,
    IND_INPUT_KEYBOARD,
    IND_INPUT_MOUSE,
} indicator_input_t;

void indicator_init(void);
void indicator_set(indicator_state_t state);
void indicator_input(indicator_input_t input);
