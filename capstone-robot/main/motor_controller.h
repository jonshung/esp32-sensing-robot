#ifndef MOTOR_CONTROLLER_DEFINE
#define MOTOR_CONTROLLER_DEFINE

#include <esp_err.h>
#include <driver/gpio.h>

#define MOTOR1_GPIO1 GPIO_NUM_18
#define MOTOR1_GPIO2 GPIO_NUM_19

#define MOTOR2_GPIO1 GPIO_NUM_25
#define MOTOR2_GPIO2 GPIO_NUM_26

#define MOTOR3_GPIO1 GPIO_NUM_32
#define MOTOR3_GPIO2 GPIO_NUM_33

#define BDC_MCPWM_TIMER_RESOLUTION_HZ 10000000 // 10MHz, 1 tick = 0.1us
#define BDC_MCPWM_FREQ_HZ             25000    // 25KHz PWM
#define BDC_MCPWM_DUTY_TICK_MAX       (BDC_MCPWM_TIMER_RESOLUTION_HZ / BDC_MCPWM_FREQ_HZ) // maximum value we can set for the duty cycle, in ticks

esp_err_t motor_controller_init();
void motor_task(void* pvParams);

#endif