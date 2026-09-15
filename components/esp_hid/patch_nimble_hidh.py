from pathlib import Path
import sys

src = Path(sys.argv[1])
dst = Path(sys.argv[2])

text = src.read_text(encoding="utf-8")

needle = '''                cuuid = ble_uuid_u16(&char_result[c].uuid.u);
                chandle = char_result[c].val_handle;
'''
replacement = '''                cuuid = ble_uuid_u16(&char_result[c].uuid.u);
                chandle = char_result[c].val_handle;
                report = NULL;
'''
if needle not in text:
    raise SystemExit("ERROR: characteristic loop marker not found")
text = text.replace(needle, replacement, 1)

start_marker = '''                } else if (suuid == BLE_SVC_DIS_UUID16) {
'''
end_marker = '''                struct ble_gatt_dsc descr_result[HIDH_MAX_DSCS];
'''

start = text.find(start_marker)
if start < 0:
    raise SystemExit("ERROR: DIS branch start not found")

end = text.find(end_marker, start)
if end < 0:
    raise SystemExit("ERROR: descriptor discovery marker not found")

fixed = r'''                } else if (suuid == BLE_SVC_DIS_UUID16) {
                    if (char_result[c].properties & BLE_GATT_CHR_PROP_READ) {
                        if (cuuid == BLE_SVC_DIS_CHR_UUID16_PNP_ID) {
                            if (read_char(dev->ble.conn_id, chandle, &rdata, &rlen) == 0 && rlen == 7) {
                                dev->config.vendor_id = *((uint16_t *)&rdata[1]);
                                dev->config.product_id = *((uint16_t *)&rdata[3]);
                                dev->config.version = *((uint16_t *)&rdata[5]);
                            }
                        } else if (cuuid == BLE_SVC_DIS_CHR_UUID16_MANUFACTURER_NAME) {
                            if (read_char(dev->ble.conn_id, chandle, &rdata, &rlen) == 0 && rlen) {
                                char *mn = nimble_hidh_dup_cstr(rdata, rlen);
                                if (mn) {
                                    free((void *)dev->config.manufacturer_name);
                                    dev->config.manufacturer_name = mn;
                                }
                            }
                        } else if (cuuid == BLE_SVC_DIS_CHR_UUID16_SERIAL_NUMBER) {
                            if (read_char(dev->ble.conn_id, chandle, &rdata, &rlen) == 0 && rlen) {
                                char *sn = nimble_hidh_dup_cstr(rdata, rlen);
                                if (sn) {
                                    free((void *)dev->config.serial_number);
                                    dev->config.serial_number = sn;
                                }
                            }
                        }
                    }
                    continue;
                } else if (suuid == BLE_SVC_HID_UUID16) {
                    if (cuuid == BLE_SVC_HID_CHR_UUID16_PROTOCOL_MODE) {
                        if (char_result[c].properties & BLE_GATT_CHR_PROP_READ) {
                            if (read_char(dev->ble.conn_id, chandle, &rdata, &rlen) == 0 && rlen) {
                                dev->protocol_mode[hidindex] = *((uint8_t *)rdata);
                                free(rdata);
                                rdata = NULL;
                            }
                        }
                        continue;
                    }

                    if (cuuid == BLE_SVC_HID_CHR_UUID16_REPORT_MAP) {
                        if (char_result[c].properties & BLE_GATT_CHR_PROP_READ) {
                            if (read_char(dev->ble.conn_id, chandle, &rdata, &rlen) == 0 && rlen) {
                                uint8_t *copy = nimble_hidh_dup_bytes(rdata, rlen);
                                if (copy) {
                                    free((void *)dev->config.report_maps[hidindex].data);
                                    dev->config.report_maps[hidindex].data = copy;
                                    dev->config.report_maps[hidindex].len = rlen;
                                    ESP_LOGI(TAG, "FD FIX: HID Report Map read, len=%u", rlen);
                                }
                                free(rdata);
                                rdata = NULL;
                            }
                        }
                        continue;
                    }

                    if (cuuid == BLE_SVC_HID_CHR_UUID16_BOOT_KBD_INP ||
                            cuuid == BLE_SVC_HID_CHR_UUID16_BOOT_KBD_OUT ||
                            cuuid == BLE_SVC_HID_CHR_UUID16_BOOT_MOUSE_INP ||
                            cuuid == BLE_SVC_HID_CHR_UUID16_RPT) {
                        report = (esp_hidh_dev_report_t *)calloc(1, sizeof(esp_hidh_dev_report_t));
                        if (report == NULL) {
                            ESP_LOGE(TAG, "malloc esp_hidh_dev_report_t failed");
                            goto done;
                        }

                        report->permissions = char_result[c].properties;
                        report->handle = chandle;
                        report->ccc_handle = 0;
                        report->report_id = 0;
                        report->map_index = hidindex;

                        if (cuuid == BLE_SVC_HID_CHR_UUID16_BOOT_KBD_INP) {
                            report->protocol_mode = ESP_HID_PROTOCOL_MODE_BOOT;
                            report->report_type = ESP_HID_REPORT_TYPE_INPUT;
                            report->usage = ESP_HID_USAGE_KEYBOARD;
                            report->value_len = 8;
                        } else if (cuuid == BLE_SVC_HID_CHR_UUID16_BOOT_KBD_OUT) {
                            report->protocol_mode = ESP_HID_PROTOCOL_MODE_BOOT;
                            report->report_type = ESP_HID_REPORT_TYPE_OUTPUT;
                            report->usage = ESP_HID_USAGE_KEYBOARD;
                            report->value_len = 8;
                        } else if (cuuid == BLE_SVC_HID_CHR_UUID16_BOOT_MOUSE_INP) {
                            report->protocol_mode = ESP_HID_PROTOCOL_MODE_BOOT;
                            report->report_type = ESP_HID_REPORT_TYPE_INPUT;
                            report->usage = ESP_HID_USAGE_MOUSE;
                            report->value_len = 8;
                        } else {
                            report->protocol_mode = ESP_HID_PROTOCOL_MODE_REPORT;
                            report->report_type = 0;
                            report->usage = ESP_HID_USAGE_GENERIC;
                            report->value_len = 0;
                        }

                        ESP_LOGI(TAG,
                                 "FD FIX: HID report char handle=%u props=0x%02x map=%u",
                                 report->handle, report->permissions, report->map_index);
                    } else {
                        continue;
                    }
                }
'''

text = text[:start] + fixed + text[end:]

dst.parent.mkdir(parents=True, exist_ok=True)
dst.write_text(text, encoding="utf-8")
print(f"Flipper Dongle: patched ESP-IDF NimBLE HID Host -> {dst}")
