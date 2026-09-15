#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_hidh.h"
#include "esp_hid_gap.h"
#include "host/ble_hs.h"
#include "host/ble_sm.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "usb_hid_bridge.h"
#include "pair_button.h"
#include "indicator.h"
#include "host/ble_store.h"

static const char *TAG = "FD_V04";
static volatile bool s_connected = false;

/*
 * Pairing policy:
 * - no saved bond: idle until short BOOT requests one 5 s pairing scan
 * - saved bond: auto-reconnect only to that bonded HID peer
 * - long BOOT: clear bond and return to idle; it must NOT auto-pair
 */
static volatile bool s_pair_scan_requested = false;
static volatile bool s_auto_reconnect = false;
static volatile bool s_have_bond = false;

static ble_addr_t s_bonded_peers[CONFIG_BT_NIMBLE_MAX_BONDS];
static int s_bond_count = 0;

static esp_hidh_dev_t *s_current_dev = NULL;

/*
 * Pair/reset synchronization.
 *
 * esp_hidh_dev_close() is asynchronous and esp_hid_scan() is synchronous.
 * These flags + generation counter prevent a short BOOT request from being
 * erased by a late CLOSE event and prevent an old scan result from opening a
 * device after bonds were cleared.
 */
static volatile bool s_pair_reset_in_progress = false;
static volatile bool s_pair_after_close = false;
static volatile uint32_t s_scan_generation = 1;

typedef enum {
    BRIDGE_REPORT_KEYBOARD = 1,
    BRIDGE_REPORT_MOUSE = 2,
} bridge_report_kind_t;

typedef struct {
    bridge_report_kind_t kind;
    uint16_t report_id;
    uint16_t len;
    uint8_t data[16];
} bridge_report_t;

static QueueHandle_t s_hid_queue = NULL;

static bool keyboard_report_is_release(const uint8_t *data, uint16_t len)
{
    if (!data || len != 8) return false;
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] != 0) return false;
    }
    return true;
}

static bool keyboard_report_is_active(const uint8_t *data, uint16_t len)
{
    return data && len == 8 && !keyboard_report_is_release(data, len);
}

static void queue_bridge_report(const bridge_report_t *report, bool priority)
{
    (void)priority;

    if (!s_hid_queue || !report) return;

    if (xQueueSend(s_hid_queue, report, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "USB HID queue full: kind=%d id=%u len=%u",
                 report->kind, report->report_id, report->len);
    }
}

static void queue_release_all(void)
{
    bridge_report_t kb = {
        .kind = BRIDGE_REPORT_KEYBOARD,
        .report_id = 1,
        .len = 8,
        .data = {0},
    };
    bridge_report_t mouse = {
        .kind = BRIDGE_REPORT_MOUSE,
        .report_id = 2,
        .len = 4,
        .data = {0},
    };

    queue_bridge_report(&kb, true);
    queue_bridge_report(&mouse, true);
}

void ble_store_config_init(void);

static void log_ble_addr(const char *prefix, const ble_addr_t *addr)
{
    if (!addr) return;
    ESP_LOGI(TAG, "%s %02X:%02X:%02X:%02X:%02X:%02X type=%u",
             prefix,
             addr->val[5], addr->val[4], addr->val[3],
             addr->val[2], addr->val[1], addr->val[0],
             addr->type);
}

static bool refresh_bond_state(void)
{
    int num_peers = 0;

    memset(s_bonded_peers, 0, sizeof(s_bonded_peers));

    int rc = ble_store_util_bonded_peers(
        s_bonded_peers,
        &num_peers,
        CONFIG_BT_NIMBLE_MAX_BONDS);

    if (rc != 0) {
        ESP_LOGW(TAG, "ble_store_util_bonded_peers rc=%d", rc);
        return s_have_bond;
    }

    s_bond_count = num_peers;
    s_have_bond = (s_bond_count > 0);

    ESP_LOGI(TAG, "Saved BLE HID bonds: %d/%d",
             s_bond_count, CONFIG_BT_NIMBLE_MAX_BONDS);

    for (int i = 0; i < s_bond_count; i++) {
        char label[32];
        snprintf(label, sizeof(label), "  bond[%d]:", i);
        log_ble_addr(label, &s_bonded_peers[i]);
    }

    return s_have_bond;
}

static int scan_result_bond_index(const esp_hid_scan_result_t *r)
{
    if (!r || !s_have_bond || r->transport != ESP_HID_TRANSPORT_BLE) {
        return -1;
    }

    for (int i = 0; i < s_bond_count; i++) {
        if (memcmp(r->bda, s_bonded_peers[i].val, sizeof(r->bda)) == 0) {
            return i;
        }
    }

    return -1;
}

static bool scan_result_matches_bond(const esp_hid_scan_result_t *r)
{
    return scan_result_bond_index(r) >= 0;
}

static void hidh_callback(void *handler_args, esp_event_base_t base,
                          int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *p = (esp_hidh_event_data_t *)event_data;

    switch (event) {
    case ESP_HIDH_OPEN_EVENT:
        if (p->open.status == ESP_OK) {
            s_current_dev = p->open.dev;
            s_connected = true;
            s_pair_scan_requested = false;

            (void)refresh_bond_state();
            s_auto_reconnect = true;

            indicator_set(IND_CONNECTING);
            ESP_LOGI(TAG, "BLE HID session opened: %s (waiting for input reports)",
                     esp_hidh_dev_name_get(p->open.dev));
        } else {
            s_current_dev = NULL;
            s_connected = false;
            ESP_LOGW(TAG, "BLE HID open failed");
            indicator_set(IND_ERROR);
        }
        break;

    case ESP_HIDH_INPUT_EVENT: {
        const uint8_t *hid_data =
            ((const uint8_t *)event_data) + sizeof(esp_hidh_event_data_t);
        const uint16_t hid_len = p->input.length;

        ESP_LOGI(TAG, "BLE HID INPUT: usage=%d id=%u len=%u",
                 p->input.usage, p->input.report_id, hid_len);

        if (hid_len) {
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, hid_data, hid_len, ESP_LOG_INFO);
        }

        bool is_keyboard =
            (p->input.report_id == 1) ||
            (p->input.usage == ESP_HID_USAGE_KEYBOARD) ||
            (hid_len == 8);

        bool is_mouse =
            (p->input.report_id == 2) ||
            (p->input.usage == ESP_HID_USAGE_MOUSE) ||
            (hid_len == 4 && p->input.report_id != 1);

        bridge_report_t r = {0};

        if (is_keyboard) {
            r.kind = BRIDGE_REPORT_KEYBOARD;
            r.report_id = p->input.report_id;
            r.len = hid_len > sizeof(r.data) ? sizeof(r.data) : hid_len;
            memcpy(r.data, hid_data, r.len);

            const bool release = keyboard_report_is_release(r.data, r.len);
            ESP_LOGI(TAG, "Keyboard %s", release ? "RELEASE" : "PRESS/STATE");
            queue_bridge_report(&r, release);
        } else if (is_mouse) {
            r.kind = BRIDGE_REPORT_MOUSE;
            r.report_id = p->input.report_id;
            r.len = hid_len > sizeof(r.data) ? sizeof(r.data) : hid_len;
            memcpy(r.data, hid_data, r.len);
            queue_bridge_report(&r, false);
        } else {
            ESP_LOGI(TAG, "Ignoring HID report: usage=%d id=%u len=%u",
                     p->input.usage, p->input.report_id, hid_len);
        }
        break;
    }

    case ESP_HIDH_CLOSE_EVENT: {
        queue_release_all();
        s_current_dev = NULL;
        s_connected = false;

        const bool reset_close = s_pair_reset_in_progress;
        const bool deferred_pair = s_pair_after_close;

        s_pair_reset_in_progress = false;
        s_pair_after_close = false;

        (void)refresh_bond_state();

        if (reset_close) {
            s_auto_reconnect = false;

            if (deferred_pair) {
                s_pair_scan_requested = true;
                ESP_LOGI(TAG,
                         "Deferred BOOT pairing request released after CLOSE");
                indicator_set(IND_SCANNING);
            } else {
                s_pair_scan_requested = false;
                indicator_set(IND_DISCONNECTED);
            }
        } else {
            s_auto_reconnect = s_have_bond;
            if (!s_pair_scan_requested) {
                indicator_set(IND_DISCONNECTED);
            }
        }

        ESP_LOGI(TAG,
                 "CLOSE: reason=%d reset_close=%d deferred_pair=%d "
                 "auto_reconnect=%d pair_requested=%d gen=%lu",
                 p->close.reason,
                 reset_close,
                 deferred_pair,
                 s_auto_reconnect,
                 s_pair_scan_requested,
                 (unsigned long)s_scan_generation);

        esp_hidh_dev_free(p->close.dev);
        break;
    }

    default:
        break;
    }
}

static bool send_bridge_report(const bridge_report_t *r)
{
    const uint8_t *data = r->data;
    uint16_t len = r->len;

    if (r->kind == BRIDGE_REPORT_KEYBOARD && len == 9 && data[0] == 1) {
        data++;
        len--;
    } else if (r->kind == BRIDGE_REPORT_MOUSE && len == 5 && data[0] == 2) {
        data++;
        len--;
    }

    bool queued = false;

    if (r->kind == BRIDGE_REPORT_KEYBOARD) {
        queued = usb_hid_bridge_keyboard(data, len);
    } else if (r->kind == BRIDGE_REPORT_MOUSE) {
        queued = usb_hid_bridge_mouse(data, len);
    }

    if (queued) {
        ESP_LOGI(TAG, "USB HID queued: kind=%d id=%u len=%u",
                 r->kind, r->report_id, len);
        indicator_set(IND_CONNECTED);
    } else {
        ESP_LOGE(TAG, "USB HID queue-to-endpoint FAILED: kind=%d id=%u len=%u",
                 r->kind, r->report_id, len);
    }

    return queued;
}

static void usb_hid_worker_task(void *arg)
{
    (void)arg;
    bridge_report_t r;

    while (1) {
        if (xQueueReceive(s_hid_queue, &r, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        (void)send_bridge_report(&r);
    }
}

static void nimble_host_task(void *param)
{
    (void)param;
    ESP_LOGI(TAG, "NimBLE host started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void scan_and_connect_task(void *arg)
{
    (void)arg;

    while (1) {
        const bool pair_now = s_pair_scan_requested;
        const bool reconnect_now = s_auto_reconnect && s_have_bond;

        if (!s_connected &&
            !s_pair_reset_in_progress &&
            (pair_now || reconnect_now)) {

            const uint32_t my_generation = s_scan_generation;

            if (pair_now) {
                s_pair_scan_requested = false;
            }

            size_t n = 0;
            esp_hid_scan_result_t *results = NULL;

            indicator_set(IND_SCANNING);
            ESP_LOGI(TAG,
                     "%s scan for BLE HID... gen=%lu",
                     pair_now ? "PAIRING" : "AUTO-RECONNECT",
                     (unsigned long)my_generation);

            esp_hid_scan(5, &n, &results);

            if (my_generation != s_scan_generation ||
                s_pair_reset_in_progress ||
                s_connected) {
                ESP_LOGW(TAG,
                         "Discarding stale BLE scan: scan_gen=%lu current_gen=%lu reset=%d connected=%d",
                         (unsigned long)my_generation,
                         (unsigned long)s_scan_generation,
                         s_pair_reset_in_progress,
                         s_connected);
                esp_hid_scan_results_free(results);
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            esp_hid_scan_result_t *best = NULL;

            for (esp_hid_scan_result_t *r = results; r; r = r->next) {
                if (r->transport != ESP_HID_TRANSPORT_BLE) continue;

                const int bond_index = scan_result_bond_index(r);
                const bool bonded_match = (bond_index >= 0);

                ESP_LOGI(TAG,
                         "BLE HID candidate: %s RSSI=%d appearance=0x%04x bonded=%d slot=%d",
                         r->name ? r->name : "(no name)",
                         r->rssi,
                         r->ble.appearance,
                         bonded_match,
                         bond_index);

                if (!pair_now && !bonded_match) {
                    continue;
                }

                if (!best || r->rssi > best->rssi) {
                    best = r;
                }
            }

            if (my_generation != s_scan_generation ||
                s_pair_reset_in_progress ||
                s_connected) {
                ESP_LOGW(TAG, "Scan invalidated before open; ignoring result");
                esp_hid_scan_results_free(results);
                continue;
            }

            if (best) {
                indicator_set(IND_FOUND);
                ESP_LOGI(TAG,
                         "Opening BLE HID: %s (%s) gen=%lu",
                         best->name ? best->name : "(no name)",
                         pair_now ? "pairing" : "bonded reconnect",
                         (unsigned long)my_generation);
                indicator_set(IND_CONNECTING);
                esp_hidh_dev_open(best->bda, best->transport, best->ble.addr_type);
            } else if (pair_now) {
                indicator_set(IND_DISCONNECTED);
                ESP_LOGW(TAG,
                         "Pairing scan finished: no HID found. "
                         "Open Bluetooth Remote/BadUSB and short-press BOOT again.");
            } else {
                indicator_set(IND_DISCONNECTED);
            }

            esp_hid_scan_results_free(results);
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void pair_button_task(void *arg)
{
    (void)arg;

    while (1) {
        pair_button_event_t ev = pair_button_poll();

        if (ev == PAIR_BUTTON_SHORT) {
            if (s_pair_reset_in_progress) {
                s_pair_after_close = true;
                s_scan_generation++;
                ESP_LOGI(TAG,
                         "BOOT short: pairing deferred until BLE close completes gen=%lu",
                         (unsigned long)s_scan_generation);
                indicator_set(IND_SCANNING);
            } else if (s_connected) {
                ESP_LOGI(TAG, "BOOT short ignored: HID already connected");
            } else {
                s_scan_generation++;
                s_auto_reconnect = false;
                s_pair_scan_requested = true;

                ESP_LOGI(TAG,
                         "BOOT short: one pairing scan requested gen=%lu",
                         (unsigned long)s_scan_generation);
                indicator_set(IND_SCANNING);
            }

        } else if (ev == PAIR_BUTTON_LONG) {
            ESP_LOGW(TAG, "BOOT long: clearing ALL BLE HID bonds");
            indicator_set(IND_PAIR_RESET);

            s_scan_generation++;

            s_pair_scan_requested = false;
            s_pair_after_close = false;
            s_auto_reconnect = false;
            s_have_bond = false;
            s_bond_count = 0;
            memset(s_bonded_peers, 0, sizeof(s_bonded_peers));

            queue_release_all();
            ble_store_clear();

            if (s_current_dev && esp_hidh_dev_exists(s_current_dev)) {
                s_pair_reset_in_progress = true;
                ESP_LOGI(TAG,
                         "Closing current BLE HID connection; reset pending gen=%lu",
                         (unsigned long)s_scan_generation);
                (void)esp_hidh_dev_close(s_current_dev);
            } else {
                s_pair_reset_in_progress = false;
                s_connected = false;
                s_current_dev = NULL;
                ESP_LOGI(TAG, "No active BLE HID connection; reset completed immediately");
            }

            ESP_LOGW(TAG,
                     "All bonds cleared. Dongle stays IDLE until short BOOT.");
            indicator_set(IND_DISCONNECTED);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(ret);
    }

    ESP_LOGI(TAG, "Flipper Dongle v0.5.6: pairing race fix");
    indicator_init();
    ESP_ERROR_CHECK(pair_button_init());
    ESP_ERROR_CHECK(usb_hid_bridge_init());

    ESP_ERROR_CHECK(esp_hid_gap_init(HIDH_BLE_MODE));

    esp_hidh_config_t cfg = {
        .callback = hidh_callback,
        .event_stack_size = 6144,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK(esp_hidh_init(&cfg));

    ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_YESNO;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_sc_only = 0;
    ble_hs_cfg.sm_mitm = 1;

    ble_hs_cfg.sm_our_key_dist =
        BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist =
        BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    ESP_LOGI(TAG,
             "SMP cfg: io=%u bond=%u mitm=%u sc=%u sc_only=%u our_keys=0x%02x their_keys=0x%02x",
             ble_hs_cfg.sm_io_cap,
             ble_hs_cfg.sm_bonding,
             ble_hs_cfg.sm_mitm,
             ble_hs_cfg.sm_sc,
             ble_hs_cfg.sm_sc_only,
             ble_hs_cfg.sm_our_key_dist,
             ble_hs_cfg.sm_their_key_dist);

    s_hid_queue = xQueueCreate(128, sizeof(bridge_report_t));
    ESP_ERROR_CHECK(s_hid_queue ? ESP_OK : ESP_ERR_NO_MEM);
    xTaskCreate(usb_hid_worker_task, "usb_hid_worker", 4096, NULL, 4, NULL);

    ble_store_config_init();
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    (void)refresh_bond_state();
    s_auto_reconnect = s_have_bond;
    s_pair_scan_requested = false;

    ESP_LOGI(TAG,
             "Startup pairing state: have_bond=%d auto_reconnect=%d",
             s_have_bond, s_auto_reconnect);

    nimble_port_freertos_init(nimble_host_task);

    vTaskDelay(pdMS_TO_TICKS(300));
    xTaskCreate(scan_and_connect_task, "flipper_scan", 6144, NULL, 2, NULL);
    xTaskCreate(pair_button_task, "pair_button", 3072, NULL, 3, NULL);
}
