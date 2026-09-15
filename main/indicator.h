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

void indicator_init(void);
void indicator_set(indicator_state_t state);
