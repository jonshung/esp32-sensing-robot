idf_component_register( 
    SRCS 
    "sensors_controller.c" 
    "main_controller.c"
    "wifi_controller.c"
    "motor_controller.c"
    "entry.c"
    
    INCLUDE_DIRS 
    "."
    REQUIRES
    esp_driver_i2c
    esp_wifi
    esp_pm
    nvs_flash
    mqtt
    json

    bme680
    hy_srf05
    espressif__bh1750
    zorxx__neopixel
    generic_helper
)