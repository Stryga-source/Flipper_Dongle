cmake_minimum_required(VERSION 3.16)

# Default to ESP32-S3 for this hardware if target wasn't selected explicitly.
if(NOT DEFINED IDF_TARGET)
    set(IDF_TARGET "esp32s3")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(flipper_dongle_v041)
