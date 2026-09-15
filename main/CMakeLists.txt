# Normalize IDF_PATH for CMake on Windows.
file(TO_CMAKE_PATH "$ENV{IDF_PATH}" IDF_PATH_CMAKE)
set(HID_EXAMPLE_DIR "${IDF_PATH_CMAKE}/examples/bluetooth/esp_hid_host/main")

# ESP-IDF 6.1's NimBLE HID host example filters scan results by Espressif demo
# device names. Patch the installed helper at configure time into the build
# directory so Flipper's standard HID-over-GATT advertisement (0x1812) is accepted.
set(PATCHED_HID_GAP "${CMAKE_CURRENT_BINARY_DIR}/esp_hid_gap_flipper.c")
execute_process(
    COMMAND "${PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/patch_esp_hid_gap.py"
            "${HID_EXAMPLE_DIR}/esp_hid_gap.c"
            "${PATCHED_HID_GAP}"
    RESULT_VARIABLE PATCH_RESULT
    OUTPUT_VARIABLE PATCH_OUTPUT
    ERROR_VARIABLE PATCH_ERROR
)
if(NOT PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to patch ESP-IDF esp_hid_gap.c: ${PATCH_ERROR}")
endif()
message(STATUS "${PATCH_OUTPUT}")

idf_component_register(
    SRCS
        "main.c"
        "usb_hid_bridge.c"
        "pair_button.c"
        "indicator.c"
        "${PATCHED_HID_GAP}"
    INCLUDE_DIRS
        "."
        "${HID_EXAMPLE_DIR}"
    REQUIRES
        esp_tinyusb
        esp_hid
        bt
        nvs_flash
)
