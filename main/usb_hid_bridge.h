#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

esp_err_t usb_hid_bridge_init(void);
bool usb_hid_bridge_ready(void);
bool usb_hid_bridge_keyboard(const uint8_t *data, uint16_t len);
bool usb_hid_bridge_mouse(const uint8_t *data, uint16_t len);
bool usb_hid_bridge_release_all(void);
