from pathlib import Path
import sys

src = Path(sys.argv[1])
dst = Path(sys.argv[2])
text = src.read_text(encoding="utf-8")

text = text.replace(
    "disc_params.filter_duplicates = 1;",
    "disc_params.filter_duplicates = 0; /* keep ADV + scan response */",
    1,
)

fn_sig = "handle_ble_device_result(const struct ble_gap_disc_desc *disc)"
fn_start = text.find(fn_sig)
if fn_start < 0:
    raise SystemExit("ERROR: NimBLE scanner function not found")

fn_end = text.find("\n}\n#endif", fn_start)
if fn_end < 0:
    raise SystemExit("ERROR: scanner function end not found")

fn = text[fn_start:fn_end+3]

needle = '''    if (fields.appearance_is_present) {
        MODLOG_DFLT(DEBUG, "    appearance=0x%04x\\n", fields.appearance);
        appearance = fields.appearance;
    }
'''
if needle not in fn:
    raise SystemExit("ERROR: appearance block not found")

diag = needle + '''
    ESP_LOGI(TAG,
             "BLE ADV %02X:%02X:%02X:%02X:%02X:%02X RSSI=%d name='%s' appearance=0x%04x uuids16=%d",
             disc->addr.val[5], disc->addr.val[4], disc->addr.val[3],
             disc->addr.val[2], disc->addr.val[1], disc->addr.val[0],
             disc->rssi,
             adv_name_len ? (char *)adv_name : "(none)",
             appearance,
             fields.num_uuids16);

    for (int u = 0; u < fields.num_uuids16; u++) {
        ESP_LOGI(TAG, "  UUID16[%d]=0x%04x", u, ble_uuid_u16(&fields.uuids16[u].u));
    }
'''
fn = fn.replace(needle, diag, 1)

start_marker = "    for (int i = 0; i < fields.num_uuids16; i++) {"
start = fn.find(start_marker)
if start < 0:
    raise SystemExit("ERROR: original HID filter loop not found")

pos = start
depth = 0
seen = False
end = None
while pos < len(fn):
    c = fn[pos]
    if c == "{":
        depth += 1
        seen = True
    elif c == "}":
        depth -= 1
        if seen and depth == 0:
            end = pos + 1
            break
    pos += 1

if end is None:
    raise SystemExit("ERROR: original HID filter loop end not found")

replacement = '''    bool hid_uuid = false;
    for (int i = 0; i < fields.num_uuids16; i++) {
        if (ble_uuid_u16(&fields.uuids16[i].u) == BLE_HID_SVC_UUID) {
            hid_uuid = true;
            break;
        }
    }

    const bool control_name =
        (adv_name_len >= 7 && memcmp(adv_name, "Control", 7) == 0);

    if (hid_uuid || control_name) {
        ESP_LOGW(TAG,
                 "REAL HID CANDIDATE: hid=%d control=%d name='%s' RSSI=%d",
                 hid_uuid, control_name,
                 adv_name_len ? (char *)adv_name : "(none)", disc->rssi);
        add_ble_scan_result(disc->addr.val, disc->addr.type, appearance,
                            adv_name, adv_name_len, disc->rssi);
    } else if (adv_name_len >= 7 && memcmp(adv_name, "Flipper", 7) == 0) {
        ESP_LOGI(TAG, "Ignoring normal Flipper profile (not HID): name='%s'",
                 (char *)adv_name);
    }'''

fn = fn[:start] + replacement + fn[end:]
text = text[:fn_start] + fn + text[fn_end+3:]

lines = text.splitlines(True)
for i, line in enumerate(lines):
    if "pkey.numcmp_accept" in line and "=" in line:
        indent = line[:len(line) - len(line.lstrip())]
        lines[i] = indent + "pkey.numcmp_accept = 1; /* Flipper Dongle */\n"
text = "".join(lines)

dst.parent.mkdir(parents=True, exist_ok=True)
dst.write_text(text, encoding="utf-8")
print("Patched v0.4.9 strict Control/HID scanner successfully")
