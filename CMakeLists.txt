cmake_minimum_required(VERSION 3.16)

# Default to ESP32-S3 for this hardware if target wasn't selected explicitly.
if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET "esp32s3")
endif()

if(POCKET_DONGLE AND LILYGO_T_DONGLE_S3)
    message(FATAL_ERROR "Select only one dongle board profile")
endif()

# Board profiles use separate sdkconfig files; Waveshare remains the default.
if(POCKET_DONGLE)
    set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.pocket")
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.pocket.defaults")
elseif(LILYGO_T_DONGLE_S3)
    set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.lilygo")
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.lilygo.defaults")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(flipper_dongle_v041)
