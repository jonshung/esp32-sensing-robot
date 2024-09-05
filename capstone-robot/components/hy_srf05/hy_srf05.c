#include <math.h>
#include <float.h>

#include "hy_srf05.h"
#include "helper.h"

#include <freertos/task.h>

esp_err_t hy_srf05_init(hy_srf05_dev_t *dev) {
    NULL_ARG_CHECK(dev);
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << dev->echo_pin),
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    ERR_CHECK(gpio_config(&io_cfg));
    io_cfg.pin_bit_mask = (1ULL << dev->trig_pin);
    io_cfg.mode = GPIO_MODE_OUTPUT;
    ERR_CHECK(gpio_config(&io_cfg));
    portMUX_INITIALIZE(&dev->hy_srf05_mux);

    return gpio_set_level(dev->trig_pin, 0);
}

esp_err_t hy_srf05_measure_us(hy_srf05_dev_t *dev, const uint32_t *max_time_us, uint32_t *time_us) {
    NULL_ARG_CHECK(dev);
    NULL_ARG_CHECK(max_time_us);
    NULL_ARG_CHECK(time_us);

    taskENTER_CRITICAL(&dev->hy_srf05_mux);
    int64_t t_ms = esp_timer_get_time();
    ERR_CHECK_CRITICAL(gpio_set_level(dev->trig_pin, 0), &dev->hy_srf05_mux);
    // good enough
    WAIT_FOR_US(t_ms, __HY_SRF05_TRIG_LOW_DELAY);
    t_ms = esp_timer_get_time();
    ERR_CHECK_CRITICAL(gpio_set_level(dev->trig_pin, 1), &dev->hy_srf05_mux);
    WAIT_FOR_US(t_ms, __HY_SRF05_TRIG_HIGH_DELAY);
    ERR_CHECK_CRITICAL(gpio_set_level(dev->trig_pin, 0), &dev->hy_srf05_mux);

    t_ms = esp_timer_get_time();
    while(gpio_get_level(dev->echo_pin)) { // wait for previous reading
        if(!TIMEOUT_CHECK_US(t_ms, __HY_SRF05_WAIT_TIMEOUT)) {
            taskEXIT_CRITICAL(&dev->hy_srf05_mux);
            return HY_SRF05_ERR_BUSY;
        }
    }

    t_ms = esp_timer_get_time();
    while(!gpio_get_level(dev->echo_pin)) {
        if(TIMEOUT_CHECK_US(t_ms, __HY_SRF05_START_TIMEOUT)) {
            taskEXIT_CRITICAL(&dev->hy_srf05_mux);
            return HY_SRF05_ERR_START_TIMEOUT;
        }
    }

    // started
    uint32_t start = (uint32_t) esp_timer_get_time();
    while(gpio_get_level(dev->echo_pin)) {
        if(TIMEOUT_CHECK_US(start, *max_time_us)) {
            taskEXIT_CRITICAL(&dev->hy_srf05_mux);
            return HY_SRF05_ERR_MEASURE_TIMEOUT;
        }
    }
    uint32_t end = (uint32_t) esp_timer_get_time();
    taskEXIT_CRITICAL(&dev->hy_srf05_mux);
    *time_us = end-start;   
    return ESP_OK;
}

// t_corr in Kelvin
esp_err_t hy_srf05_measure_cm(hy_srf05_dev_t *dev, const uint32_t *max_time_us, const float *t_corr, float *distance) {
    NULL_ARG_CHECK(dev);
    NULL_ARG_CHECK(max_time_us);
    NULL_ARG_CHECK(distance);

    uint32_t time_us;
    ERR_CHECK(hy_srf05_measure_us(dev, max_time_us, &time_us));
    // speed correction
    float sound_velo = __HY_SRF05_VELO_IDEAL;
    *distance = (sound_velo * time_us) / 2;
    return ESP_OK;
}