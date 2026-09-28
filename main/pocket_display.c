#include "pocket_display.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_SCLK GPIO_NUM_10
#define LCD_MOSI GPIO_NUM_11
#define LCD_CS   GPIO_NUM_12
#define LCD_DC   GPIO_NUM_13
#define LCD_RST  GPIO_NUM_14
#define LCD_WIDTH 160
#define LCD_HEIGHT 80

static const char *TAG = "pocket_lcd";
static spi_device_handle_t lcd;
static uint16_t frame[LCD_WIDTH * LCD_HEIGHT];

// Five columns per glyph, bit zero at the top. Only characters used by the
// status screen are included; unsupported characters render as spaces.
static const uint8_t font[27][5] = {
    {0,0,0,0,0}, {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36},
    {0x3e,0x41,0x41,0x41,0x22}, {0x7f,0x41,0x41,0x22,0x1c},
    {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f},
    {0x41,0x41,0x7f,0x41,0x41}, {0x20,0x40,0x41,0x3f,0x01},
    {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f},
    {0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x09,0x09,0x09,0x06},
    {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7f,0x01,0x01},
    {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f},
    {0x3f,0x40,0x38,0x40,0x3f}, {0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43},
};

static esp_err_t send(bool data, const void *bytes, size_t count)
{
    gpio_set_level(LCD_DC, data);
    spi_transaction_t tr = {.length = count * 8, .tx_buffer = bytes};
    return spi_device_polling_transmit(lcd, &tr);
}

static esp_err_t command(uint8_t cmd, const uint8_t *args, size_t count)
{
    esp_err_t err = send(false, &cmd, 1);
    if (err == ESP_OK && count) err = send(true, args, count);
    return err;
}

static void pixel(int x, int y, uint16_t color)
{
    if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
        frame[y * LCD_WIDTH + x] = __builtin_bswap16(color);
    }
}

static void line(const char *s, int x, int y, uint16_t color, int scale)
{
    if (!s) return;
    for (; *s && x + 6 * scale < LCD_WIDTH; ++s, x += 6 * scale) {
        char c = *s;
        const uint8_t *glyph = font[(c >= 'A' && c <= 'Z') ? c - 'A' + 1 : 0];
        for (int col = 0; col < 5; ++col) {
            for (int row = 0; row < 7; ++row) {
                if (!(glyph[col] & (1u << row))) continue;
                for (int dy = 0; dy < scale; ++dy)
                    for (int dx = 0; dx < scale; ++dx)
                        pixel(x + col * scale + dx, y + row * scale + dy, color);
            }
        }
    }
}

static void fill_rect(int x0, int y0, int x1, int y1, uint16_t color)
{
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            pixel(x, y, color);
}

static void fill_ellipse(int cx, int cy, int rx, int ry, uint16_t color)
{
    for (int y = -ry; y <= ry; ++y)
        for (int x = -rx; x <= rx; ++x)
            if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry)
                pixel(cx + x, cy + y, color);
}

static int edge(int ax, int ay, int bx, int by, int px, int py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static void fill_triangle(int ax, int ay, int bx, int by, int cx, int cy, uint16_t color)
{
    int min_x = ax < bx ? (ax < cx ? ax : cx) : (bx < cx ? bx : cx);
    int max_x = ax > bx ? (ax > cx ? ax : cx) : (bx > cx ? bx : cx);
    int min_y = ay < by ? (ay < cy ? ay : cy) : (by < cy ? by : cy);
    int max_y = ay > by ? (ay > cy ? ay : cy) : (by > cy ? by : cy);
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            int e0 = edge(ax, ay, bx, by, x, y);
            int e1 = edge(bx, by, cx, cy, x, y);
            int e2 = edge(cx, cy, ax, ay, x, y);
            if ((e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (e0 <= 0 && e1 <= 0 && e2 <= 0)) pixel(x, y, color);
        }
    }
}

static void draw_dolphin(void)
{
    const uint16_t blue = 0x047f;
    const uint16_t pale = 0xbfff;
    fill_triangle(24, 43, 2, 28, 9, 44, blue);
    fill_triangle(24, 43, 2, 59, 10, 44, blue);
    fill_triangle(40, 35, 49, 17, 55, 35, blue);
    fill_ellipse(46, 43, 31, 13, blue);
    fill_triangle(69, 39, 88, 40, 78, 47, blue);
    fill_triangle(47, 51, 60, 68, 56, 50, blue);
    fill_ellipse(50, 49, 24, 6, pale);
    fill_rect(62, 36, 64, 38, 0x0000);
    fill_rect(77, 47, 84, 47, 0x0000);
    fill_rect(16, 70, 33, 71, 0x047f);
    fill_rect(42, 73, 62, 74, 0x047f);
}

static void draw_keyboard(void)
{
    const uint16_t rim = 0xffe0;
    fill_rect(92, 40, 155, 69, rim);
    fill_rect(95, 43, 152, 66, 0x18c3);
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 7; ++col)
            fill_rect(98 + col * 8, 46 + row * 8,
                      103 + col * 8, 51 + row * 8, 0xffff);
    fill_rect(110, 62, 139, 64, 0xffff);
}

static void draw_mouse(void)
{
    const uint16_t rim = 0xffe0;
    fill_ellipse(124, 51, 17, 22, rim);
    fill_ellipse(124, 51, 14, 19, 0x18c3);
    fill_rect(123, 32, 125, 46, rim);
    fill_rect(121, 39, 127, 42, 0xffff);
    fill_rect(112, 72, 136, 73, 0x07e0);
}

static void flush_frame(void)
{
    const uint8_t columns[] = {0,1,0,160};
    const uint8_t rows[] = {0,26,0,105};
    if (command(0x2a, columns, sizeof(columns)) != ESP_OK ||
        command(0x2b, rows, sizeof(rows)) != ESP_OK) return;
    const uint8_t write_ram = 0x2c;
    if (send(false, &write_ram, 1) != ESP_OK) return;
    if (send(true, frame, sizeof(frame)) != ESP_OK)
        ESP_LOGE(TAG, "LCD frame transfer failed");
}

esp_err_t pocket_display_init(void)
{
    gpio_config_t outputs = {
        .pin_bit_mask = (1ULL << LCD_DC) | (1ULL << LCD_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    esp_err_t err = gpio_config(&outputs);
    if (err != ESP_OK) return err;
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    spi_bus_config_t bus = {
        .mosi_io_num = LCD_MOSI, .miso_io_num = -1, .sclk_io_num = LCD_SCLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = sizeof(frame),
    };
    err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) return err;
    spi_device_interface_config_t dev = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0, .spics_io_num = LCD_CS, .queue_size = 1,
    };
    err = spi_bus_add_device(SPI2_HOST, &dev, &lcd);
    if (err != ESP_OK) return err;

    // ST7735R sequence used by the reference green-tab 160x80 setup.
    const uint8_t f1[] = {0x01,0x2c,0x2d};
    const uint8_t f3[] = {0x01,0x2c,0x2d,0x01,0x2c,0x2d};
    const uint8_t p1[] = {0xa2,0x02,0x84};
    const uint8_t p3[] = {0x0a,0x00};
    const uint8_t p4[] = {0x8a,0x2a};
    const uint8_t p5[] = {0x8a,0xee};
    const uint8_t gamma1[] = {0x02,0x1c,0x07,0x12,0x37,0x32,0x29,0x2d,0x29,0x25,0x2b,0x39,0x00,0x01,0x03,0x10};
    const uint8_t gamma2[] = {0x03,0x1d,0x07,0x06,0x2e,0x2c,0x29,0x2d,0x2e,0x2e,0x37,0x3f,0x00,0x00,0x02,0x10};
#define CMD(c, a) do { err = command(c, a, sizeof(a)); if (err != ESP_OK) return err; } while (0)
#define CMD1(c, v) do { const uint8_t arg = (v); err = command(c, &arg, 1); if (err != ESP_OK) return err; } while (0)
    err = command(0x01, NULL, 0); if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(150));
    err = command(0x11, NULL, 0); if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(500));
    CMD(0xb1, f1); CMD(0xb2, f1); CMD(0xb3, f3);
    const uint8_t inv = 0x07, pw2 = 0xc5, vcom = 0x0e, depth = 0x05;
    CMD1(0xb4, inv); CMD(0xc0, p1); CMD1(0xc1, pw2);
    CMD(0xc2, p3); CMD(0xc3, p4); CMD(0xc4, p5);
    CMD1(0xc5, vcom); CMD1(0x3a, depth);
    CMD(0xe0, gamma1); CMD(0xe1, gamma2);
    // Rotation 1: 160x80, BGR; reference offsets become x=1, y=26.
    const uint8_t madctl = 0x28;
    CMD1(0x36, madctl);
    err = command(0x21, NULL, 0); if (err != ESP_OK) return err;
    err = command(0x13, NULL, 0); if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(10));
    err = command(0x29, NULL, 0);
    ESP_LOGI(TAG, "ST7735 candidate display init: %s", esp_err_to_name(err));
    return err;
}

void pocket_display_show(const char *line1, const char *line2, const char *line3)
{
    if (!lcd) return;
    memset(frame, 0, sizeof(frame));
    line("FLIPPER DONGLE", 4, 3, 0x07ff, 1);
    for (int x = 0; x < LCD_WIDTH; ++x) pixel(x, 15, 0x07ff);
    line(line1, 4, 21, 0xffff, 2);
    line(line2, 4, 43, 0xffe0, 1);
    line(line3, 4, 58, 0x07e0, 1);
    flush_frame();
}

void pocket_display_show_input(bool keyboard)
{
    if (!lcd) return;
    memset(frame, 0, sizeof(frame));
    line("CONNECTED", 4, 3, 0x07e0, 1);
    line(keyboard ? "KEYBOARD" : "MOUSE", 101, 3, 0xffe0, 1);
    for (int x = 0; x < LCD_WIDTH; ++x) pixel(x, 14, 0x047f);
    draw_dolphin();
    if (keyboard) draw_keyboard();
    else draw_mouse();
    flush_frame();
}
