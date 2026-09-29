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

set(BOARD_DISPLAY_SOURCE "")
set(BOARD_EMBED_FILES "")
if(POCKET_DONGLE OR LILYGO_T_DONGLE_S3)
    set(BOARD_DISPLAY_SOURCE "pocket_display.c")
    set(BOARD_EMBED_FILES "pocket_scenes.bin")
endif()

idf_component_register(
    SRCS
        "main.c"
        "usb_hid_bridge.c"
        "pair_button.c"
        "indicator.c"
        ${BOARD_DISPLAY_SOURCE}
        "${PATCHED_HID_GAP}"
    INCLUDE_DIRS
        "."
        "${HID_EXAMPLE_DIR}"
    REQUIRES
        esp_tinyusb
        esp_hid
        bt
        nvs_flash
    EMBED_FILES
        ${BOARD_EMBED_FILES}
)

if(POCKET_DONGLE OR LILYGO_T_DONGLE_S3)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BOARD_SCENE_DISPLAY=1)
    if(POCKET_DONGLE)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE POCKET_DONGLE=1)
    else()
        target_compile_definitions(${COMPONENT_LIB} PRIVATE LILYGO_T_DONGLE_S3=1)
    endif()
    idf_component_get_property(GPIO_LIB esp_driver_gpio COMPONENT_LIB)
    idf_component_get_property(SPI_LIB esp_driver_spi COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PRIVATE ${GPIO_LIB} ${SPI_LIB})
endif()
