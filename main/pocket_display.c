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
#define SCENE_BYTES (LCD_WIDTH * LCD_HEIGHT * 2)

static const char *TAG = "pocket_lcd";
static spi_device_handle_t lcd;
static uint16_t frame[LCD_WIDTH * LCD_HEIGHT];
extern const uint8_t pocket_scenes_bin_start[] asm("_binary_pocket_scenes_bin_start");
extern const uint8_t pocket_scenes_bin_end[] asm("_binary_pocket_scenes_bin_end");

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

static bool copy_scene(pocket_scene_t scene)
{
    if (!lcd || scene < 0 || scene >= POCKET_SCENE_COUNT) return false;
    if ((size_t)(pocket_scenes_bin_end - pocket_scenes_bin_start) <
        POCKET_SCENE_COUNT * SCENE_BYTES) {
        ESP_LOGE(TAG, "Pocket scene asset is incomplete");
        return false;
    }
    memcpy(frame, pocket_scenes_bin_start + scene * SCENE_BYTES, SCENE_BYTES);
    return true;
}

void pocket_display_show_scene(pocket_scene_t scene)
{
    if (copy_scene(scene)) flush_frame();
}

static void draw_digit(unsigned digit, int x, int y)
{
    static const uint8_t columns[10][5] = {
        {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31},
        {0x18,0x14,0x12,0x7f,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e},
    };
    for (int col = 0; col < 5; ++col) {
        for (int row = 0; row < 7; ++row) {
            if (!(columns[digit][col] & (1u << row))) continue;
            for (int dx = 0; dx < 2; ++dx)
                for (int dy = 0; dy < 2; ++dy)
                    pixel(x + col * 2 + dx, y + row * 2 + dy, 0x0000);
        }
    }
}

void pocket_display_show_pairing_code(uint32_t code)
{
    if (!copy_scene(POCKET_SCENE_PAIRING)) return;
    code %= 1000000;
    for (int i = 5; i >= 0; --i) {
        draw_digit(code % 10, 47 + i * 11, 56);
        code /= 10;
    }
    flush_frame();
}
