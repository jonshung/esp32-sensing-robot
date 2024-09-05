#ifndef BME680_DEFINE
#define BME680_DEFINE

#include <stdint.h>

#include "bme68x.h"

#include <esp_err.h>
#include <driver/i2c_types.h>

#if __cplusplus
extern "C" {
#endif

#define BME680_TAG "BME680"

// --------------------- internal working constants
#define __BME680_I2C_READ_TIMEOUT (int)(1e3) // 1e3 ms => 1 second
#define __BME680_I2C_CL_FREQUENCY 100000 // 100 kHz

typedef struct bme68x_dev bme680_dev_t;
typedef struct bme68x_data bme680_data_t;

esp_err_t bme680_init(bme680_dev_t* const dev, const i2c_master_bus_handle_t *i2c_bus);
esp_err_t bme680_read(bme680_dev_t* const dev, bme680_data_t* const data, uint8_t sample_count);
int bme680_get_humidity_score(const float *current_hum);
int bme680_get_gas_score(const float * gas_ref);
const char* bme680_IAQ_status(int score);

// --------------- private interfaces for sensor API
BME68X_INTF_RET_TYPE bme680_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
BME68X_INTF_RET_TYPE bme680_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void bme680_delay_us(uint32_t period, void *intf_ptr);

#if __cplusplus
}
#endif

#endif // #ifndef BME680_DEFINE