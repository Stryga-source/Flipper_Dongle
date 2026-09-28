#pragma once

#include "esp_err.h"

// Experimental Pocket-Dongle-S3-0.96 display. Pins are from a public board
// reference and still require visual confirmation on this exact board.
esp_err_t pocket_display_init(void);
void pocket_display_show(const char *line1, const char *line2, const char *line3);
