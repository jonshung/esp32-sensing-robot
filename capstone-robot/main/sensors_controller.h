#ifndef SENSORS_CONTROLLER_DEFINE
#define SENSORS_CONTROLLER_DEFINE
// sensor componenets
#include "hy_srf05.h"
#include "bme680.h"
//#include "vl53l0x.h"
#include "bh1750.h"
#include "sdkconfig.h"

#include <esp_check.h>

#include <stdint.h>

#define SEN_CTRL_TAG "SEN_CTRL"

#define SENSORS_NUM 4

typedef struct {
    float hy_srf05_distance;
    bme680_data_t bme680_data;
    uint16_t vl53l0x_data;
} sensors_data_collection_t;

typedef struct {
    bh1750_handle_t hal;
    TickType_t last_retrieval;
} bh1750_dev_t;

// ---------------------------- hardware connections and operating value ---------------------------------
#define I2C_MAIN_SDA_PIN CONFIG_MAIN_I2C_SDA_GPIO
#define I2C_MAIN_SCL_PIN CONFIG_MAIN_I2C_SCL_GPIO

#define HY_SRF05_TRIG_PIN_TEST CONFIG_HY_SRF05_TRIG_GPIO
#define HY_SRF05_ECHO_PIN_TEST CONFIG_HY_SRF05_ECHO_GPIO

#define HY_SRF05_MEASURING_MAX_TIMEOUT_US (uint32_t) (500 / __HY_SRF05_VELO_IDEAL) // 5 meter max in ideal gas ~ 14 ms

#define BH1750_MEASURE_INTERVAL CONFIG_BH1750_MEASURING_INTERVAL // ms

esp_err_t start_hy_srf05(void);
esp_err_t retrieve_hy_srf05_data(float *distance);

esp_err_t start_i2c_bus(void);

esp_err_t start_bme680(void);
esp_err_t retrieve_bme680_data(bme680_data_t* data);

/*
esp_err_t start_vl53l0x(void);
esp_err_t retrieve_vl53l0x_data(uint16_t *distance);
*/
esp_err_t start_bh1750(void);
esp_err_t retrieve_bh1750(float* lx);

esp_err_t sensors_controller_init(void);

#endif // #ifndef SENSORS_CONTROLLER_DEFINE