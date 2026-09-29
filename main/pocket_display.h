#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// Pocket-Dongle-S3-0.96 display pins were verified by readable diagnostic text
// on the operator's board; the controller package marking remains unknown.
esp_err_t pocket_display_init(void);
void pocket_display_show(const char *line1, const char *line2, const char *line3);
typedef enum {
    POCKET_SCENE_IDLE = 0,
    POCKET_SCENE_KEYBOARD,
    POCKET_SCENE_OTHER,
    POCKET_SCENE_MOUSE,
    POCKET_SCENE_BADUSB,
    POCKET_SCENE_FOUND,
    POCKET_SCENE_PAIRING,
    POCKET_SCENE_SCAN_1,
    POCKET_SCENE_SCAN_2,
    POCKET_SCENE_SCAN_3,
    POCKET_SCENE_COUNT,
} pocket_scene_t;

void pocket_display_show_scene(pocket_scene_t scene);
void pocket_display_show_pairing_code(uint32_t code);
