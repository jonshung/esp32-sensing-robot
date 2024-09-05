#include "sensors_controller.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/i2c_types.h>

#include <memory.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>

#include "helper.h"

hy_srf05_dev_t main_hy_srf05_dev;
bool hy_srf05_initialized = 0;  

i2c_master_bus_handle_t main_i2c_bus_handle;

bme680_dev_t main_bme680_dev;
bool bme680_initialized = 0;

/*
vl53l0x_t * main_vl53l0x_dev;
bool vl53l0x_intialized = 0;
*/

bh1750_dev_t main_bh1750_dev;
bool bh1750_initalized = 0;

sensors_data_collection_t last_data = {
    .hy_srf05_distance = 0
};

esp_err_t start_hy_srf05(void) {
    main_hy_srf05_dev.trig_pin = HY_SRF05_TRIG_PIN_TEST;
    main_hy_srf05_dev.echo_pin = HY_SRF05_ECHO_PIN_TEST;
    esp_err_t ret;
    if((ret = hy_srf05_init(&main_hy_srf05_dev)) != ESP_OK) {
        ESP_LOGE(HY_SRF05_TAG, "Error initializing");
        return ret;
    }
    hy_srf05_initialized = 1;
    ESP_LOGI(HY_SRF05_TAG, "Initialized");
    return ESP_OK;
}

esp_err_t start_i2c_bus(void) {
    i2c_master_bus_config_t master_cfg = {
        .i2c_port = 0,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .flags.enable_internal_pullup = true // since we are mostly operating in not-high-frequency, its better to use internal pullup
    };
    ERR_CHECK(i2c_new_master_bus(&master_cfg, &main_i2c_bus_handle));
    printf("started i2c\n");
    return ESP_OK;
}

esp_err_t start_bme680(void) {
    if(!main_i2c_bus_handle) {
        return ESP_FAIL; // handle uninitialized fail
    }
    ERR_CHECK(bme680_init(&main_bme680_dev, &main_i2c_bus_handle));
    bme680_initialized = true;
    printf("Started bme680\n");
    return ESP_OK;
}

/*
esp_err_t start_vl53l0x(void) {
    // intialize i2c device handle, no need for high speed
    i2c_device_config_t i2c_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x29,
        .scl_speed_hz = 100000, // 200 kHz
    };
    i2c_master_dev_handle_t local_handle;
    ERR_CHECK(i2c_master_bus_add_device(main_i2c_bus_handle, &i2c_dev_cfg, &local_handle));
    main_vl53l0x_dev = vl53l0x_config(local_handle, GPIO_NUM_22, GPIO_NUM_21, -1, 0x29, 0);
    ERR_CHECK(vl53l0x_init(&main_vl53l0x_dev));
    return ESP_OK;
    // int16_t temp;
    // ERR_CHECK(vl53l0x_measure_continuous_mm(&main_vl53l0x_dev, false, VL53L0X_MEASURE_INTERVAL, &temp));

    printf("Started vl53l0x\n");
    return ESP_OK;
}
*/

esp_err_t start_bh1750(void) {
    main_bh1750_dev.hal = bh1750_create(&main_i2c_bus_handle, BH1750_I2C_ADDRESS_DEFAULT);
    if(!main_bh1750_dev.hal) {
        return ESP_FAIL;
    }
    ERR_CHECK(bh1750_power_on(main_bh1750_dev.hal));
    ERR_CHECK(bh1750_set_measure_mode(main_bh1750_dev.hal, BH1750_CONTINUE_1LX_RES));
    main_bh1750_dev.last_retrieval = xTaskGetTickCount();
    printf("Started bh1750\n");
    return ESP_OK;
}

esp_err_t sensors_controller_init(void) {
    // might add modularity to this later - jonshung 
    ERR_CHECK(start_hy_srf05());
    ERR_CHECK(start_i2c_bus());
    ERR_CHECK(start_bme680());
    //ERR_CHECK(start_vl53l0x());
    ERR_CHECK(start_bh1750());
    return ESP_OK;
}

esp_err_t retrieve_hy_srf05_data(float *distance) {
    NULL_ARG_CHECK(distance);
    if(!hy_srf05_initialized) {
        ESP_LOGE(HY_SRF05_TAG, "Device is not initialized");
        return ESP_FAIL;
    }
    uint32_t max_timeout_us = HY_SRF05_MEASURING_MAX_TIMEOUT_US;
    float temp;
    if((temp = last_data.bme680_data.temperature) <= FLT_EPSILON) {
        temp = 0.0f;
    }
    ERR_CHECK(hy_srf05_measure_cm(&main_hy_srf05_dev, &max_timeout_us, &temp, &last_data.hy_srf05_distance));
    *distance = last_data.hy_srf05_distance;
    return ESP_OK;
}

esp_err_t retrieve_bme680_data(bme680_data_t* data) {
    NULL_ARG_CHECK(data);
    if(!bme680_initialized) {
        ESP_LOGE(BME680_TAG, "Device is not initialized");
        return ESP_FAIL;
    }
    esp_err_t res = bme680_read(&main_bme680_dev, &last_data.bme680_data, 1);
    if(res != ESP_OK && res != BME68X_OK && res != BME68X_W_NO_NEW_DATA) {
        return res;
    }
    *data = last_data.bme680_data;
    return ESP_OK;
}

/*
esp_err_t retrieve_vl53l0x_data(uint16_t *distance) {
    NULL_ARG_CHECK(distance);
    last_data.vl53l0x_data = vl53l0x_readRangeSingleMillimeters(&main_vl53l0x_dev);
    *distance = last_data.vl53l0x_data;
    return ESP_OK;
}
*/

esp_err_t retrieve_bh1750(float* lx) {
    NULL_ARG_CHECK(lx);

    if((xTaskGetTickCount() - main_bh1750_dev.last_retrieval) >= BH1750_MEASURE_INTERVAL)
        bh1750_get_data(main_bh1750_dev.hal, lx);
    else *lx = -1.0f;
    return ESP_OK;
}