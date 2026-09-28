idf_component_register(
    SRCS "probe.c" "../../../main/pocket_display.c"
    INCLUDE_DIRS "." "../../../main"
    REQUIRES esp_psram spi_flash esp_hw_support esp_driver_gpio esp_driver_spi
)
