file(TO_CMAKE_PATH "$ENV{IDF_PATH}" IDF_PATH_CMAKE)
set(IDF_ESP_HID_DIR "${IDF_PATH_CMAKE}/components/esp_hid")
set(PATCHED_NIMBLE_HIDH "${CMAKE_CURRENT_BINARY_DIR}/nimble_hidh_fixed.c")

execute_process(
    COMMAND "${PYTHON}"
            "${CMAKE_CURRENT_LIST_DIR}/patch_nimble_hidh.py"
            "${IDF_ESP_HID_DIR}/src/nimble_hidh.c"
            "${PATCHED_NIMBLE_HIDH}"
    RESULT_VARIABLE PATCH_RESULT
    OUTPUT_VARIABLE PATCH_OUTPUT
    ERROR_VARIABLE PATCH_ERROR
)

if(NOT PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to patch ESP-IDF NimBLE HID Host: ${PATCH_ERROR}")
endif()

message(STATUS "${PATCH_OUTPUT}")

set(srcs
    "${IDF_ESP_HID_DIR}/src/esp_hidd.c"
    "${IDF_ESP_HID_DIR}/src/esp_hidh.c"
    "${IDF_ESP_HID_DIR}/src/esp_hid_common.c"
)

if(CONFIG_BT_ENABLED)
    if(CONFIG_BT_BLUEDROID_ENABLED)
        list(APPEND srcs
            "${IDF_ESP_HID_DIR}/src/ble_hidd.c"
            "${IDF_ESP_HID_DIR}/src/ble_hidh.c"
            "${IDF_ESP_HID_DIR}/src/bt_hidh.c"
            "${IDF_ESP_HID_DIR}/src/bt_hidd.c"
        )
    endif()

    if(CONFIG_BT_NIMBLE_ENABLED)
        list(APPEND srcs
            "${IDF_ESP_HID_DIR}/src/nimble_hidd.c"
            "${PATCHED_NIMBLE_HIDH}"
        )
    endif()
endif()

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${IDF_ESP_HID_DIR}/include"
    PRIV_INCLUDE_DIRS "${IDF_ESP_HID_DIR}/private"
    REQUIRES esp_event bt
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
