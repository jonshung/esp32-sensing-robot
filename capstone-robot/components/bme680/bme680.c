#include <string.h>

#include "bme680.h"
#include "helper.h"

#include <driver/i2c_master.h>
#include <driver/i2c_types.h>

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

// we primarily use i2c here
esp_err_t bme680_init(bme680_dev_t* const dev_info, const i2c_master_bus_handle_t *i2c_bus) {
    NULL_ARG_CHECK(dev_info);
    // intialize i2c device handle, no need for high speed
    i2c_device_config_t i2c_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME68X_I2C_ADDR_HIGH,
        .scl_speed_hz = 100000, // 200 kHz
    };
    i2c_master_dev_handle_t local_handle;
    ERR_CHECK(i2c_master_bus_add_device(*i2c_bus, &i2c_dev_cfg, &local_handle));
    dev_info->intf_ptr = (void*) local_handle;
    
    dev_info->intf = BME68X_I2C_INTF;
    dev_info->delay_us = bme680_delay_us;
    dev_info->read = bme680_i2c_read;
    dev_info->write = bme680_i2c_write;
    dev_info->amb_temp = 25;

    struct bme68x_conf dev_conf;
    struct bme68x_heatr_conf heatr_conf;
    ERR_CHECK(bme68x_init(dev_info));
    ERR_CHECK(bme68x_get_conf(&dev_conf, dev_info));

    dev_conf.filter = BME68X_FILTER_SIZE_3;
    dev_conf.odr = BME68X_ODR_NONE;
    dev_conf.os_hum = BME68X_OS_16X;
    dev_conf.os_pres = BME68X_OS_1X;
    dev_conf.os_temp = BME68X_OS_2X;
    ERR_CHECK(bme68x_set_conf(&dev_conf, dev_info));

    heatr_conf.enable = BME68X_ENABLE;
    heatr_conf.heatr_temp = 300; // 300 deg C
    heatr_conf.heatr_dur = 100; // 100 ms
    ERR_CHECK(bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heatr_conf, dev_info));
    return ESP_OK;
}

esp_err_t bme680_read(bme680_dev_t* const dev, bme680_data_t* const data, uint8_t sample_count) {
    uint8_t n_fields = 0;
    // waiting for heater duration to worn off
    struct bme68x_conf dev_conf;
    ERR_CHECK(bme68x_set_op_mode(BME68X_FORCED_MODE, dev));
    ERR_CHECK(bme68x_get_conf(&dev_conf, dev));
    uint32_t del_period = bme68x_get_meas_dur(BME68X_FORCED_MODE, &dev_conf, dev) + (100 * 1000);
    dev->delay_us(del_period, dev->intf_ptr);
    ERR_CHECK(bme68x_get_data(BME68X_FORCED_MODE, data, &n_fields, dev));

    if(!n_fields) return ESP_FAIL; // todo: handle error
    return ESP_OK;
}

int bme680_get_humidity_score(const float *current_humidity) {  //Calculate humidity contribution to IAQ index
    NULL_ARG_CHECK(current_humidity);
    int humidity_score = 0;
    if (*current_humidity >= 38 && *current_humidity <= 42) // Humidity +/-5% around optimum
        humidity_score = 0.25 * 100;
    else
    { // Humidity is sub-optimal
        if (*current_humidity < 38)
        humidity_score = 0.25 / 40 * *current_humidity * 100;
        else
        {
        humidity_score = ((-0.25 / (100 - 40) * *current_humidity) + 0.416666) * 100;
        }
    }
    return humidity_score;
}


const char* bme680_IAQ_status(int score) {
    score = (100 - score) * 5;
    if      (score >= 301)                  return "Hazardous";
    else if (score >= 201 && score <= 300 ) return "Very Unhealthy";
    else if (score >= 176 && score <= 200 ) return "Unhealthy";
    else if (score >= 151 && score <= 175 ) return "Unhealthy for Sensitive Groups";
    else if (score >=  51 && score <= 150 ) return "Moderate";
    else if (score >=  00 && score <=  50 ) return "Good";
    return "Normal";
}

int   gas_lower_limit = 10000;  // Bad air quality limit
int   gas_upper_limit = 300000; // Good air quality limit
int bme680_get_gas_score(const float * gas_ref) {
  //Calculate gas contribution to IAQ index
  int gas_score = 0;
  gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * (*gas_ref) - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00;
  if (gas_score > 75) gas_score = 75; // Sometimes gas readings can go outside of expected scale maximum
  if (gas_score <  0) gas_score = 0;  // Sometimes gas readings can go outside of expected scale minimum
  return gas_score;
}

// ---------------- private interfaces for sensor API
BME68X_INTF_RET_TYPE bme680_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    i2c_master_dev_handle_t device_handle = (i2c_master_dev_handle_t) intf_ptr;
    return i2c_master_transmit_receive(device_handle, &reg_addr, 1, reg_data, len, __BME680_I2C_READ_TIMEOUT);
}

BME68X_INTF_RET_TYPE bme680_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    i2c_master_dev_handle_t device_handle = (i2c_master_dev_handle_t) intf_ptr;
    uint8_t *buffer = calloc(sizeof(uint8_t), 1+len);
    buffer[0] = reg_addr;
    memcpy(buffer+1, reg_data, len*sizeof(uint8_t));
    esp_err_t ret = i2c_master_transmit(device_handle, buffer, len+1, __BME680_I2C_READ_TIMEOUT);
    free(buffer);
    return ret;
}

void bme680_delay_us(uint32_t period, void *intf_ptr) {
    (void)intf_ptr;
    int64_t start = esp_timer_get_time();
    WAIT_FOR_US(start, period);
}