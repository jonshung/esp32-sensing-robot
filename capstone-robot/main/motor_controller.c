#include "motor_controller.h"

#include "bdc_motor.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

bdc_motor_handle_t main_motor1_dev;
bdc_motor_handle_t main_motor2_dev;
bdc_motor_handle_t main_motor3_dev;

esp_err_t motor_controller_init() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1 << MOTOR1_GPIO1) | (1 << MOTOR1_GPIO2) | (1 << MOTOR2_GPIO1) | (1 << MOTOR2_GPIO2) | (1 << MOTOR3_GPIO1) | (1 << MOTOR3_GPIO2)
    };
    gpio_config(&cfg);
    

    return ESP_OK;
}

void motor_task(void* pvParams) {
    motor_controller_init();
    TickType_t current_time = xTaskGetTickCount();
    // start all motors
    while(1) {
        gpio_set_level(MOTOR1_GPIO1, 1);
        gpio_set_level(MOTOR2_GPIO1, 1);
        vTaskDelayUntil(&current_time, pdMS_TO_TICKS(2500));
        gpio_set_level(MOTOR1_GPIO1, 0);
        gpio_set_level(MOTOR2_GPIO2, 0);
        vTaskDelayUntil(&current_time, pdMS_TO_TICKS(2500));

        gpio_set_level(MOTOR2_GPIO1, 1);
        gpio_set_level(MOTOR3_GPIO1, 1);
        vTaskDelayUntil(&current_time, pdMS_TO_TICKS(2500));
        gpio_set_level(MOTOR2_GPIO1, 0);
        gpio_set_level(MOTOR3_GPIO2, 0);
        vTaskDelayUntil(&current_time, pdMS_TO_TICKS(2500));

        gpio_set_level(MOTOR1_GPIO1, 1);
        gpio_set_level(MOTOR3_GPIO1, 1);
        vTaskDelayUntil(&current_time, pdMS_TO_TICKS(2500));
        gpio_set_level(MOTOR1_GPIO1, 0);
        gpio_set_level(MOTOR3_GPIO2, 0);
        vTaskDelayUntil(&current_time, pdMS_TO_TICKS(2500));
    }
}

esp_err_t motor_controller_sleep() {
    if(main_motor1_dev) {
        bdc_motor_disable(main_motor1_dev);
    }
    if(main_motor2_dev) {
        bdc_motor_disable(main_motor2_dev);
    }
    if(main_motor3_dev) {
        bdc_motor_disable(main_motor3_dev);
    }
    return ESP_OK;
}