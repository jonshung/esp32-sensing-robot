#ifndef HY_SRF05_DEFINE
#define HY_SRF05_DEFINE

#if __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "driver/gpio.h"

#include <esp_err.h>
#include <freertos/FreeRTOS.h>

#define HY_SRF05_TAG "HY-SRF05"

// --------------------- internal working constants
#define __HY_SRF05_TRIG_LOW_DELAY 2
#define __HY_SRF05_TRIG_HIGH_DELAY 10
#define __HY_SRF05_WAIT_TIMEOUT 100
#define __HY_SRF05_START_TIMEOUT 6000
#define __HY_SRF05_MEASURE_TIMEOUT 6000
#define __HY_SRF05_VELO_IDEAL (double)(0.034) // ideal sound travelling, in cm/us (DEFAULT FALLBACK IF NO ENVIRONMENT CORRECTION IS PROVIDED)

//   VELOCITY CORRECTION CONSTANTS, ASSUMING WE ONLY OPERATE IN AIR-FILLED ENVIRONMENT
#define __HY_SRF05_AIR_HEAT_RATIO 1.4f
#define __HY_SRF05_GAS_CONSTANT 286.9f // J/kg K

#define HY_SRF05_ERR_BUSY (esp_err_t)(0x300)
#define HY_SRF05_ERR_START_TIMEOUT (esp_err_t)(0x301)
#define HY_SRF05_ERR_MEASURE_TIMEOUT (esp_err_t)(0x302)

typedef struct {
    portMUX_TYPE hy_srf05_mux;
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;
} hy_srf05_dev_t;

esp_err_t hy_srf05_init(hy_srf05_dev_t *dev);
esp_err_t hy_srf05_measure_us(hy_srf05_dev_t *dev, const uint32_t *max_time_us, uint32_t *time_us);
// t_corr in Kelvin
esp_err_t hy_srf05_measure_cm(hy_srf05_dev_t *dev, const uint32_t *max_time_us, const float *t_corr, float *distance);

#if __cplusplus
}
#endif

#endif // #define HY_SRF05_DEFINE