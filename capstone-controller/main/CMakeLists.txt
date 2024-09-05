idf_component_register(SRCS "main.c" "display.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                    esp_lcd
                    nvs_flash
                    esp_wifi
                    mqtt
                    json
                    wifi_provisioning

                    espressif__mpu6050
                    espressif__esp_lvgl_port
                    espressif__button
                    espressif__qrcode
)
