#ifndef MAIN_CONTROLLER_DEFINE
#define MAIN_CONTROLLER_DEFINE

#include <esp_err.h>
#include "sdkconfig.h"

#define HY_SRF05_MEASURING_INTERVAL CONFIG_HY_SRF05_MEASURING_INTERVAL // ms
#define BME680_MEASURING_INTERVAL CONFIG_BME680_MEASURING_INTERVAL // ms
#define VL53L0X_MEASURE_INTERVAL CONFIG_VL53L0X_MEASURING_INTERVAL // ms

#define MAIN_CONTROLLER_LED_PIN CONFIG_MAIN_LED_DATA_GPIO
#define MAIN_CONTROLLER_LED_NUM CONFIG_MAIN_LED_NUM

void main_controller_task(void* pvParamater);
esp_err_t main_controller_init(void);

esp_err_t start_led();

void main_controller_measure_task(void *pvParams);

#endif // #ifndef MAIN_CONTROLLER_DEFINE