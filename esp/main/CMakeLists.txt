idf_component_register(
    SRCS
        "web_socket.c"
        "flash.c"
        "stm_flash.c"
        "stm_pro_mode.c"
        "logger.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server esp_wifi nvs_flash esp_driver_uart driver esp_driver_gpio spiffs esp_event freertos
)