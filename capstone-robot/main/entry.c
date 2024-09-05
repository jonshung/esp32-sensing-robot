#include "main_controller.h"
#include "wifi_controller.h"
#include "sensors_controller.h"
#include "motor_controller.h"

#include <freertos/FreeRTOS.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <stdio.h>

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    
    ret = sensors_controller_init();
    wifi_controller_init();
    main_controller_init();
    // setting up controllers and storages
    xTaskCreate(main_controller_measure_task, "Sensor measuring task", (uint32_t) 4096, NULL, portPRIVILEGE_BIT | 3, NULL);
    xTaskCreate(motor_task, "Motor task", (uint32_t) 4096*3, NULL, portPRIVILEGE_BIT | 3, NULL);
    return;
}

