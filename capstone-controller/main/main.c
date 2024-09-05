#include "mpu6050.h"
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_check.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_wifi.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>
#include "qrcode.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "display.h"
#include <mqtt_client.h>
#include <string.h>
#include <cJSON.h>

#define CONTROLLER_I2C_SDA 21
#define CONTROLLER_I2C_SCL 22

#define CONTROLLER_MPU6050_CALIBRATION_SAMPLE 1000
#define MPU6050_CALIBRATION_ACCE_ERROR_LIM 8
#define MPU6050_CALIBRATION_GYRO_ERROR_LIM 1

#define WIFI_MAX_RETRY_NUM 5

const char* tag = "CONTROLLER";
typedef struct {
    mpu6050_acce_value_t acce_calib;
    mpu6050_gyro_value_t gyro_calib;
} mpu6050_calibration_data;
static complimentary_angle_t complimentary_angle;

static esp_err_t init_i2c();
static esp_err_t mpu6050_init();
static esp_err_t retrieve_mpu6050(mpu6050_handle_t dev, mpu6050_acce_value_t *acce_data, mpu6050_gyro_value_t *gyro_data, const mpu6050_calibration_data *calib_data);

static mpu6050_handle_t main_mpu6050_dev;
static i2c_master_bus_handle_t main_i2c_bus;

static esp_err_t init_i2c() {
    esp_err_t ret;
    i2c_master_bus_config_t cfg = {
        .sda_io_num = CONTROLLER_I2C_SDA,
        .scl_io_num = CONTROLLER_I2C_SCL,
        .glitch_ignore_cnt = 7,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .flags.enable_internal_pullup = true
    };
    ret = i2c_new_master_bus(&cfg, &main_i2c_bus);
    ESP_RETURN_ON_ERROR(ret, tag, "Init i2c failed");
    return ESP_OK;
}

static esp_err_t mpu6050_init() {
    
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = MPU6050_I2C_ADDRESS,
        .scl_speed_hz = 100000, // 100 kHz
    };
    i2c_master_dev_handle_t dev;
    i2c_master_bus_add_device(main_i2c_bus, &cfg, &dev);
    main_mpu6050_dev = mpu6050_create(dev);
    mpu6050_config(main_mpu6050_dev, ACCE_FS_4G, GYRO_FS_500DPS);
    mpu6050_wake_up(main_mpu6050_dev);
    /*
    // if first time wake up, start calibration
    esp_err_t ret;
    nvs_handle_t storage_handle;
    ret = nvs_open("mpu6050_calib", NVS_READWRITE, &storage_handle);
    ESP_RETURN_ON_ERROR(ret, tag, "mpu6050 calibration storage error");
    // check if the calibration has been done before
    uint8_t calib_stats;
    ret = nvs_get_u8(storage_handle, "calib_status", &calib_stats);
    if(!calib_stats) { // perform calibration
        // accelerometer calibration
    } else {
        size_t data_len = (size_t)sizeof(mpu6050_calibration_data);
        ret = nvs_get_blob(storage_handle, "calib_data", &main_mpu6050_calib_data, &data_len);
    }
    nvs_close(storage_handle);
    */
    return ESP_OK;
}

// static void mpu6050_mean_data(mpu6050_handle_t dev, mpu6050_calibration_data *data) {
//     long i=0,buff_ax=0,buff_ay=0,buff_az=0,buff_gx=0,buff_gy=0,buff_gz=0;
//     mpu6050_acce_value_t a;
//     mpu6050_gyro_value_t g;
//     while (i < (CONTROLLER_MPU6050_CALIBRATION_SAMPLE+101)){
//         // read raw accel/gyro measurements from device
//         mpu6050_get_acce(dev, &a);
//         mpu6050_get_gyro(dev, &g);
        
//         if (i>100 && i<=(CONTROLLER_MPU6050_CALIBRATION_SAMPLE+100)){ //First 100 measures are discarded
//             buff_ax = buff_ax + a.acce_x;
//             buff_ay = buff_ay + a.acce_y;
//             buff_az = buff_az + a.acce_z;
//             buff_gx = buff_gx + g.gyro_x;
//             buff_gy = buff_gy + g.gyro_y;
//             buff_gz = buff_gz + g.gyro_z;
//         }
//         if (i==(CONTROLLER_MPU6050_CALIBRATION_SAMPLE+100)){
//             data->acce_calib.acce_x = buff_ax / CONTROLLER_MPU6050_CALIBRATION_SAMPLE;
//             data->acce_calib.acce_y = buff_ay / CONTROLLER_MPU6050_CALIBRATION_SAMPLE;
//             data->acce_calib.acce_z = buff_az / CONTROLLER_MPU6050_CALIBRATION_SAMPLE;
//             data->gyro_calib.gyro_x = buff_gx / CONTROLLER_MPU6050_CALIBRATION_SAMPLE;
//             data->gyro_calib.gyro_y = buff_gy / CONTROLLER_MPU6050_CALIBRATION_SAMPLE;
//             data->gyro_calib.gyro_z = buff_gz / CONTROLLER_MPU6050_CALIBRATION_SAMPLE;
//         }
//         i++;
//         TickType_t current = xTaskGetTickCount();
//         xTaskDelayUntil(&current, 2 / portTICK_PERIOD_MS);
//     }
// }

// static void mpu6050_calib(mpu6050_handle_t dev, mpu6050_calibration_data *data) {
//     float ax_offset,ay_offset,az_offset, gx_offset,gy_offset,gz_offset;

//     bool has_read_mean = false, has_calibration = false;
//     mpu6050_calibration_data mean_calib;
//     while(1) {
//         if(!has_read_mean) {
//             printf("Measuring first time mean\n");
//             mpu6050_mean_data(dev, &mean_calib);
//             has_read_mean = 1;
//             TickType_t current = xTaskGetTickCount();
//             xTaskDelayUntil(&current, 1000 / portTICK_PERIOD_MS);
//             continue;
//         }

//         if(!has_calibration) {
//             printf("Calibrating...\n");
//             ax_offset = -mean_calib.acce_calib.acce_x/8;
//             ay_offset = -mean_calib.acce_calib.acce_y/8;
//             az_offset=(16384-mean_calib.acce_calib.acce_z)/8;
//             gx_offset=-mean_calib.gyro_calib.gyro_x/4;
//             gy_offset=-mean_calib.gyro_calib.gyro_y/4;
//             gz_offset=-mean_calib.gyro_calib.gyro_z/4;

//             while(1) {
//                 int ready = 0;
//                 mpu6050_mean_data(dev, &mean_calib);
//                 printf("...");

//                 if (fabs(mean_calib.acce_calib.acce_x)<=MPU6050_CALIBRATION_ACCE_ERROR_LIM) ready++;
//                 else ax_offset=ax_offset-mean_calib.acce_calib.acce_x/MPU6050_CALIBRATION_ACCE_ERROR_LIM;

//                 if (fabs(mean_calib.acce_calib.acce_y)<=MPU6050_CALIBRATION_ACCE_ERROR_LIM) ready++;
//                 else ay_offset=ay_offset-mean_calib.acce_calib.acce_y/MPU6050_CALIBRATION_ACCE_ERROR_LIM;

//                 if (fabs(16384-mean_calib.acce_calib.acce_z)<=MPU6050_CALIBRATION_ACCE_ERROR_LIM) ready++;
//                 else az_offset=az_offset+(16384-mean_calib.acce_calib.acce_z)/MPU6050_CALIBRATION_ACCE_ERROR_LIM;

//                 if (fabs(mean_calib.gyro_calib.gyro_x)<= MPU6050_CALIBRATION_GYRO_ERROR_LIM) ready++;
//                 else gx_offset=gx_offset-mean_calib.gyro_calib.gyro_x/(MPU6050_CALIBRATION_GYRO_ERROR_LIM+1);

//                 if (fabs(mean_calib.gyro_calib.gyro_y)<= MPU6050_CALIBRATION_GYRO_ERROR_LIM) ready++;
//                 else gy_offset=gy_offset-mean_calib.gyro_calib.gyro_y/(MPU6050_CALIBRATION_GYRO_ERROR_LIM+1);

//                 if (fabs(mean_calib.gyro_calib.gyro_z)<= MPU6050_CALIBRATION_GYRO_ERROR_LIM) ready++;
//                 else gz_offset=gz_offset-mean_calib.gyro_calib.gyro_z/(MPU6050_CALIBRATION_GYRO_ERROR_LIM+1);

//                 if (ready==6) break;
//             }

//             has_calibration = true;
//             TickType_t current = xTaskGetTickCount();
//             xTaskDelayUntil(&current, 1000 / portTICK_PERIOD_MS);
//         }
//         break;
//     }
//     data->acce_calib = (mpu6050_acce_value_t){ .acce_x = ax_offset, .acce_y = ay_offset, .acce_z = az_offset };
//     data->gyro_calib = (mpu6050_gyro_value_t){ .gyro_x = gx_offset, .gyro_y = gy_offset, .gyro_z = gz_offset };
//     printf("Calibration done\n");
//     printf("Calibration offset: ax: %.3f, ay: %.3f, az: %.3f, gx: %.3f, gy: %.3f, gz: %.3f\n",
//                 ax_offset, ay_offset, az_offset, gx_offset, gy_offset, gz_offset);
// }

static esp_err_t retrieve_mpu6050(mpu6050_handle_t dev, mpu6050_acce_value_t *acce_data, mpu6050_gyro_value_t *gyro_data, const mpu6050_calibration_data *calib_data) {
    esp_err_t ret = ESP_OK;
    mpu6050_acce_value_t a;
    mpu6050_gyro_value_t g;
    ret = mpu6050_get_acce(dev, &a);
    if(acce_data) {
        *acce_data = a;
        ESP_RETURN_ON_ERROR(ret, tag, "Cannot get mpu6050 accelerometer data");
    }
    ret = mpu6050_get_gyro(dev, &g);
    if(gyro_data) {
        *gyro_data = g;
        ESP_RETURN_ON_ERROR(ret, tag, "Cannot get mpu6050 gyro data");
    }
    mpu6050_complimentory_filter(main_mpu6050_dev, &a, &g, &complimentary_angle);
    if(calib_data) {
        if(acce_data) {
            acce_data->acce_x += calib_data->acce_calib.acce_x;
            acce_data->acce_y += calib_data->acce_calib.acce_y;
            acce_data->acce_z += calib_data->acce_calib.acce_z;
        }
        if(gyro_data) {
            gyro_data->gyro_x += calib_data->gyro_calib.gyro_x;
            gyro_data->gyro_y += calib_data->gyro_calib.gyro_y;
            gyro_data->gyro_z += calib_data->gyro_calib.gyro_z;
        }
    }
    return ESP_OK;
}

static esp_mqtt_client_handle_t mqtt_client;
static bool mqtt_connected = false;
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        break;
    
    case MQTT_EVENT_DATA:
        char * buf = calloc(event->data_len, sizeof(uint8_t));
        memcpy(buf, event->data, sizeof(uint8_t)*event->data_len);
        cJSON* data__ = cJSON_Parse(buf);
        int n = cJSON_GetArraySize(data__);
        cJSON *elem;
        float data_send[5];
        for (int i = 0; i < (n >= 5 ? 5 : n); i++) {
            elem = cJSON_GetArrayItem(data__, i);
            data_send[i] = elem->valuedouble;
        }
        set_data(data_send);
        free(buf);
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

static const char* TAG = "Controller";
static EventGroupHandle_t wifi_event_group;
static void prov_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "SoftAP transport: Connected!");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "SoftAP transport: Disconnected!");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        start_mqtt();
        /* Signal main application to continue execution */
    } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
            case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
                ESP_LOGI(TAG, "Secured session established!");
                break;
            case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
                ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
                break;
            case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
                ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
                break;
            default:
                break;
        }
    }
}

void app_main() {
    
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();


    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &prov_event_handler, NULL));


    ESP_ERROR_CHECK( esp_netif_init() );
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };
    ESP_ERROR_CHECK( wifi_prov_mgr_init(config) );
    wifi_prov_mgr_reset_provisioning();
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

    char service_name[12];
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, 12, "%s%02X%02X%02X",
            ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    char payload[150] = {0};
    snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                ",\"transport\":\"%s\"}",
                "v1", service_name, "softap");
    set_dpp_key(payload, strlen(payload));
    const char *service_key = NULL;
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, service_name, service_key));
    

    err = init_i2c();
   
    err = mpu6050_init();

    xTaskCreatePinnedToCore(display_task, "gui", 4096, NULL, 0, NULL, 0);
    
    while(1) {
        mpu6050_acce_value_t acce;
        mpu6050_gyro_value_t gyro;
        retrieve_mpu6050(main_mpu6050_dev, &acce, &gyro, NULL);
        printf("Data:\nax: %.3f, ay: %.3f, az: %.3f\ngx: %.3f, gy: %.3f, gz: %.3f\n-----------------------\n",
                acce.acce_x, acce.acce_y, acce.acce_z, gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);
        TickType_t current = xTaskGetTickCount();
        xTaskDelayUntil(&current, 300 / portTICK_PERIOD_MS);
    }
    
}