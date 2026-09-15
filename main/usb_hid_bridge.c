#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_cdc_acm.h"
#include "tinyusb_console.h"
#include "class/hid/hid_device.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "usb_hid_bridge.h"

static const char *TAG = "USB_BRIDGE";

static SemaphoreHandle_t s_hid_tx_available = NULL;
static volatile uint32_t s_hid_tx_complete_count = 0;
static volatile uint32_t s_hid_tx_failed_count = 0;

enum {
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE = 2,
};

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
};

#if CFG_TUD_HID_EP_BUFSIZE < 9
#error "CFG_TUD_HID_EP_BUFSIZE must be at least 9 bytes for keyboard report ID + payload"
#endif

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL
};

#define EPNUM_HID       0x83
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t composite_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE,
        sizeof(hid_report_descriptor), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 0, EPNUM_CDC_NOTIF, 8,
        EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

esp_err_t usb_hid_bridge_init(void)
{
    if (!s_hid_tx_available) {
        s_hid_tx_available = xSemaphoreCreateBinary();
        if (!s_hid_tx_available) return ESP_ERR_NO_MEM;
        xSemaphoreGive(s_hid_tx_available);
    }

    tinyusb_config_t cfg = TINYUSB_DEFAULT_CONFIG();
    cfg.descriptor.full_speed_config = composite_configuration_descriptor;

    esp_err_t err = tinyusb_driver_install(&cfg);
    if (err != ESP_OK) return err;

    tinyusb_config_cdcacm_t acm_cfg = {0};
    acm_cfg.cdc_port = TINYUSB_CDC_ACM_0;

    err = tinyusb_cdcacm_init(&acm_cfg);
    if (err != ESP_OK) return err;

    err = tinyusb_console_init(TINYUSB_CDC_ACM_0);
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "USB composite ready: HID + CDC debug console");
    return ESP_OK;
}

bool usb_hid_bridge_ready(void)
{
    return tud_mounted() && tud_hid_ready();
}

static bool hid_tx_acquire(TickType_t timeout)
{
    if (!s_hid_tx_available) return false;
    if (xSemaphoreTake(s_hid_tx_available, timeout) == pdTRUE) return true;
    if (usb_hid_bridge_ready()) return true;
    return false;
}

static void hid_tx_cancel(void)
{
    if (s_hid_tx_available) xSemaphoreGive(s_hid_tx_available);
}

bool usb_hid_bridge_keyboard(const uint8_t *data, uint16_t len)
{
    if (!data || len != 8 || !tud_mounted()) return false;

    if (!hid_tx_acquire(pdMS_TO_TICKS(100))) {
        ESP_LOGW(TAG, "HID TX timeout before keyboard report");
        return false;
    }

    const bool queued = tud_hid_keyboard_report(REPORT_ID_KEYBOARD, data[0], &data[2]);
    if (!queued) {
        hid_tx_cancel();
        ESP_LOGW(TAG, "TinyUSB rejected keyboard report");
        return false;
    }
    return true;
}

bool usb_hid_bridge_mouse(const uint8_t *data, uint16_t len)
{
    if (!data || len < 3 || len > 5 || !tud_mounted()) return false;

    if (!hid_tx_acquire(pdMS_TO_TICKS(100))) {
        ESP_LOGW(TAG, "HID TX timeout before mouse report");
        return false;
    }

    const uint8_t buttons = data[0];
    const int8_t x = (int8_t)data[1];
    const int8_t y = (int8_t)data[2];
    const int8_t wheel = (len >= 4) ? (int8_t)data[3] : 0;
    const int8_t pan = (len >= 5) ? (int8_t)data[4] : 0;

    const bool queued = tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, pan);
    if (!queued) {
        hid_tx_cancel();
        ESP_LOGW(TAG, "TinyUSB rejected mouse report");
        return false;
    }
    return true;
}

bool usb_hid_bridge_release_all(void)
{
    uint8_t release[8] = {0};
    return usb_hid_bridge_keyboard(release, sizeof(release));
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    s_hid_tx_complete_count++;

    if (report && len > 0) {
        ESP_LOGD(TAG, "HID TX complete: report_id=%u len=%u total=%lu",
                 report[0], len, (unsigned long)s_hid_tx_complete_count);
    }

    if (s_hid_tx_available) xSemaphoreGive(s_hid_tx_available);
}

void tud_hid_report_failed_cb(uint8_t instance, hid_report_type_t report_type,
                              uint8_t const *report, uint16_t xferred_bytes)
{
    (void)instance;
    (void)report_type;
    s_hid_tx_failed_count++;

    ESP_LOGW(TAG, "HID TX FAILED callback: id=%u xferred=%u failures=%lu",
             (report && xferred_bytes) ? report[0] : 0,
             xferred_bytes,
             (unsigned long)s_hid_tx_failed_count);

    if (s_hid_tx_available) xSemaphoreGive(s_hid_tx_available);
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}
