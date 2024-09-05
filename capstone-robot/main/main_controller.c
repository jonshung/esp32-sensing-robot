#include "main_controller.h"

// internal
#include "sensors_controller.h"
#include "wifi_controller.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mqtt_client.h>
#include <string.h>
#include <cJSON.h>

#include "neopixel.h"

bool mqtt_connected = false;
static esp_mqtt_client_handle_t mqtt_client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        break;
    default:
        break;
    }
}

void start_mqtt() {
    esp_mqtt_client_config_t mqtt_cfg = { };
    mqtt_cfg.broker.address.uri = "mqtt://mqtt.eclipseprojects.io";

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void main_controller_task(void* pvParamater) {
    // setup
    main_controller_init();
}

esp_err_t main_controller_init(void) {
    // wifi, esp-now, communication to sensors module, ...
    start_led();
    start_mqtt();
    return ESP_OK;
}


tNeopixel pixel_on[] =
{
    { 0, NP_RGB(255, 255,  255) },
    { 1, NP_RGB(255, 255,  255) },
    { 2, NP_RGB(255, 255,  255) },
    { 3, NP_RGB(255, 255,  255) }
};
tNeopixel pixel_off[] =
{
    { 0, NP_RGB(0, 0,  0) },
    { 1, NP_RGB(0, 0,  0) },
    { 2, NP_RGB(0, 0,  0) },
    { 3, NP_RGB(0, 0,  0) }
};

tNeopixelContext neopixel;
esp_err_t start_led(void) {
    neopixel = neopixel_Init(4, 4);
    return ESP_OK;
}

static bool tester = false;
void main_controller_measure_task(void *pvParams) {
    TickType_t current_time = xTaskGetTickCount();
    bme680_data_t bme680_dat;
    //uint16_t distance = 0;
    float dist_1 = 0.0f;
    float lux;
    while(1) {
        retrieve_bme680_data(&bme680_dat);
        int score = bme680_get_humidity_score(&bme680_dat.humidity) + bme680_get_gas_score(&bme680_dat.gas_resistance);

        retrieve_bh1750(&lux);
        retrieve_hy_srf05_data(&dist_1);
        printf("BME680 data: %.3f temp, %.3f humidity, %.3f pressure, %.3f gas resistance\n", bme680_dat.temperature, bme680_dat.humidity, bme680_dat.pressure, bme680_dat.gas_resistance);
        printf("Brightness: %.3f lux\n", lux);
        printf("Distance: %.3fcm\n", dist_1);
        if(lux < 35) {
            for(int i = 0; i < 4; ++i) {
                neopixel_SetPixel(neopixel, &pixel_on[i], 1);
            }
        } else {

            for(int i = 0; i < 4; ++i) {
                neopixel_SetPixel(neopixel, &pixel_off[i], 1);
            }
        }
        if(mqtt_connected) {
            cJSON *monitor = cJSON_CreateObject();
            float buf[6] = {bme680_dat.temperature, bme680_dat.humidity, bme680_dat.pressure, score, lux, dist_1};
            cJSON *temp = cJSON_CreateFloatArray(buf, 6);
            cJSON_AddItemToObject(monitor, "data", temp);
            char* string = cJSON_Print(monitor);

            size_t len = strlen(string);
            esp_mqtt_client_publish(mqtt_client, "jonshung", string, len, 1, 0);
            cJSON_free(monitor);
            cJSON_free(temp);

            if(score >= 200) {
                cJSON *danger = cJSON_CreateObject();
                float buf[4] = {bme680_dat.temperature, bme680_dat.humidity, bme680_dat.pressure, score};
                temp = cJSON_CreateFloatArray(buf, 4);
                cJSON_AddItemToObject(danger, "data", temp);
                cJSON *scorer = cJSON_CreateNumber(score);
                cJSON_AddItemToObject(danger, "score", scorer);
                cJSON *stat_string = cJSON_CreateString(bme680_IAQ_status(score));
                cJSON_AddItemToObject(danger, "status", stat_string);
                tester = true;

                char* json_ret = cJSON_Print(danger);
                esp_mqtt_client_publish(mqtt_client, "jonshung_danger_topic", json_ret, strlen(json_ret), 1, 0);
                cJSON_free(danger);
                cJSON_free(scorer);
                cJSON_free(stat_string);
            }
        }
        xTaskDelayUntil(&current_time, pdMS_TO_TICKS(500));
    }
}