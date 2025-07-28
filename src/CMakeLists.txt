idf_component_register(SRCS "main.cpp"
                    PRIV_INCLUDE_DIRS .
                    PRIV_REQUIRES nvs_flash
                    REQUIRES "fatfs" "esp_event" "esp_timer" "driver")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")