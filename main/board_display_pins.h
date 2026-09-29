#pragma once

#include "driver/gpio.h"

// Pocket pins were tested on the operator's board. LILYGO pins come from its
// published T-Dongle-S3 display setup and still require hardware validation.
#if defined(POCKET_DONGLE)
#define LCD_SCLK GPIO_NUM_10
#define LCD_MOSI GPIO_NUM_11
#define LCD_CS   GPIO_NUM_12
#define LCD_DC   GPIO_NUM_13
#define LCD_RST  GPIO_NUM_14
#elif defined(LILYGO_T_DONGLE_S3)
#define LCD_SCLK GPIO_NUM_5
#define LCD_MOSI GPIO_NUM_3
#define LCD_CS   GPIO_NUM_4
#define LCD_DC   GPIO_NUM_2
#define LCD_RST  GPIO_NUM_1
#else
#error "Scene display requires a known board profile"
#endif
