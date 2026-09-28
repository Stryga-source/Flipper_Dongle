#pragma once

#include "esp_err.h"
#include <stdbool.h>

// Pocket-Dongle-S3-0.96 display pins were verified by readable diagnostic text
// on the operator's board; the controller package marking remains unknown.
esp_err_t pocket_display_init(void);
void pocket_display_show(const char *line1, const char *line2, const char *line3);
void pocket_display_show_input(bool keyboard);
