#pragma once

#include <stdint.h>

typedef void (*flipper_hidh_pairing_code_cb_t)(uint32_t code);
typedef void (*flipper_hidh_pairing_done_cb_t)(void);

/* Called from the NimBLE HID host's numeric-comparison event. */
void flipper_hidh_set_pairing_code_cb(flipper_hidh_pairing_code_cb_t cb);
void flipper_hidh_set_pairing_done_cb(flipper_hidh_pairing_done_cb_t cb);
