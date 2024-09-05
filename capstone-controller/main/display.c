#include "display.h"

#include "esp_lvgl_port.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include <driver/gpio.h>
#include "driver/spi_master.h"
#include "esp_err.h"

#include <esp_lcd_panel_st7789.h>
#include <esp_log.h>

#define LCD_HOST  SPI3_HOST

typedef struct {
    const char* label;
    lv_obj_t * handle;
} sensors_label;

static sensors_label sensors_label_handler[] = {
    { .handle = NULL, .label = "Temp" },
    { .handle = NULL, .label = "Hum" },
    { .handle = NULL, .label = "Pr" },
    { .handle = NULL, .label = "IAQ" },
    { .handle = NULL, .label = "Brightness" }
};

static void display_init(int32_t w, int32_t h);
static void sensor_data_tab_init(lv_obj_t * parent);
static lv_obj_t * create_scale_box(lv_obj_t * parent, sensors_label *handler, uint16_t handler_size);
static lv_obj_t * tab_create(lv_obj_t * tabview, const char * tab_name);
static void load_main_screen();
static void load_dpp_screen();

static void btn_event_cb(void *arg,void *usr_data);

lv_obj_t *tv;
lv_obj_t * main_screen;
static lv_display_t * main_display;
static char * dpp_key = NULL;

static float data_[5] = {
    0, 0, 0, 0, 0
};

// 0 = main, 1  = dpp
uint8_t current_screen = 0;

static lv_obj_t * create_scale_box(lv_obj_t * parent, sensors_label *handler, uint16_t handler_size) {
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(cont, 1);
    lv_obj_set_style_pad_all(cont, 3, 0);
    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_opa(cont, 0, 0);

    lv_obj_t * scale = lv_scale_create(cont);
    lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_OUTER);
    lv_scale_set_post_draw(scale, true);
    lv_obj_set_width(scale, LV_PCT(50));
    lv_obj_set_style_bg_opa(scale, 0, 0);
    lv_scale_set_label_show(scale, false);

    for(int i = 0; i < handler_size; ++i) {
        handler[i].handle = lv_label_create(cont);
        lv_label_set_text(handler[i].handle, handler[i].label);
    }

    static int32_t grid_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    int32_t * grid_row_dsc = calloc(sizeof(int32_t), (handler_size == 0 ? 1 : handler_size) + 1);
    for(int i = 0; i < handler_size; ++i) {
        grid_row_dsc[i] = LV_GRID_CONTENT;
    }
    grid_row_dsc[handler_size] = LV_GRID_TEMPLATE_LAST;

    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(scale, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, handler_size);
    for(int i = 0; i < handler_size; ++i) {
        lv_obj_set_grid_cell(handler[i].handle, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, i, 1);
    }
    return scale;
}

static void anim_test(void * var, int32_t v)
{
    lv_arc_set_value(var, v);
}

static void btn_event_cb(void *arg,void *usr_data)
{
    if(!main_display || !main_screen) return;
    if(current_screen == 0) {
        lvgl_port_lock(0);
        lv_obj_clean(main_screen);
        load_dpp_screen();
        current_screen = 1;
        lvgl_port_unlock();
    } else if(current_screen == 1) {
        lvgl_port_lock(0);
        lv_obj_clean(main_screen);
        load_main_screen();
        current_screen = 0;
        lvgl_port_unlock();
    }
}

void display_task(void *pvParams) {
    (void) pvParams;
    /*Initialize LVGL*/
    display_init(135, 240);
   
    lvgl_port_lock(0);
    
    main_screen = lv_disp_get_scr_act(main_display);
    load_main_screen();
    lvgl_port_unlock();

    while(1) {
        for(int i = 0; i < 5; i++) {
            if(sensors_label_handler[i].handle) {
                int v = (int) data_[i];
                lv_label_set_text_fmt(sensors_label_handler[i].handle, "%s: %d", sensors_label_handler[i].label, v);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelete(NULL);
}


static void load_main_screen() {
    tv = lv_tabview_create(main_screen);
    lv_tabview_set_tab_bar_size(tv, 30);
    lv_obj_set_style_text_font(main_screen, LV_FONT_DEFAULT, 0);

    lv_obj_t * sensor_tab = tab_create(tv, "Sensors data");

    sensor_data_tab_init(sensor_tab);
}

static void load_dpp_screen() {
    // DPP EasyConnect WiFi
    lv_obj_t * qr = lv_qrcode_create(main_screen);
    lv_qrcode_set_size(qr, 135 * 0.95);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_qrcode_update(qr, dpp_key, strlen(dpp_key));
    lv_obj_center(qr);
}


static lv_obj_t * tab_create(lv_obj_t * tabview, const char * tab_name) {
    lv_obj_t * t = lv_tabview_add_tab(tabview, tab_name);
    lv_obj_set_width(t, lv_pct(100));
    lv_obj_set_height(t, lv_pct(100));
    lv_obj_set_style_bg_opa(t, 0, 0);
    lv_obj_set_style_pad_top(t, 2, 0);
    lv_obj_set_style_pad_left(t, 5, 0);
    lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_style(t, NULL, LV_PART_SCROLLBAR);

    return t;
}

static void sensor_data_tab_init(lv_obj_t * parent) {
    lv_obj_t *scale_area = create_scale_box(parent, sensors_label_handler, sizeof(sensors_label_handler) / sizeof(sensors_label_handler[0]));
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_add_flag(lv_obj_get_parent(scale_area), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    lv_scale_set_mode(scale_area, LV_SCALE_MODE_ROUND_OUTER);
    lv_obj_set_style_pad_all(scale_area, 20, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_values(&a, 20, 100);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    lv_obj_t * arc;
    arc = lv_arc_create(scale_area);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_style(arc, NULL, LV_PART_MAIN);
    lv_obj_set_size(arc, lv_pct(100), lv_pct(100));
    lv_obj_set_style_arc_opa(arc, 0, 0);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    lv_anim_set_exec_cb(&a, anim_test);
    lv_anim_set_var(&a, arc);
    lv_anim_set_duration(&a, 4100);
    lv_anim_set_playback_duration(&a, 2700);
    lv_anim_start(&a);

    arc = lv_arc_create(scale_area);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_size(arc, lv_pct(100), lv_pct(100));
    lv_obj_set_style_margin_all(arc, 6, 0);
    lv_obj_set_style_arc_opa(arc, 0, 0);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc);

    lv_anim_set_exec_cb(&a, anim_test);
    lv_anim_set_var(&a, arc);
    lv_anim_set_duration(&a, 2600);
    lv_anim_set_playback_duration(&a, 3200);
    lv_anim_start(&a);

    arc = lv_arc_create(scale_area);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_size(arc, lv_pct(100), lv_pct(100));
    lv_obj_set_style_margin_all(arc, 12, 0);
    lv_obj_set_style_arc_opa(arc, 0, 0);
    lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(arc);

    lv_anim_set_exec_cb(&a, anim_test);
    lv_anim_set_var(&a, arc);
    lv_anim_set_duration(&a, 2800);
    lv_anim_set_playback_duration(&a, 1800);
    lv_anim_start(&a);
}

static void display_init(int32_t w, int32_t h) {

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_handle_t panel_handle = NULL;

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const spi_bus_config_t buscfg = {
        .sclk_io_num = GPIO_NUM_18,
        .mosi_io_num = GPIO_NUM_23,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = w * (h/10) * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = GPIO_NUM_16,
        .cs_gpio_num = GPIO_NUM_5,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_17,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Create LCD panel handle for ST7789, with the SPI IO device handle
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);
    esp_lcd_panel_invert_color(panel_handle, true);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = w * (h/10),
        .double_buffer = 0,
        .hres = w,
        .vres = h,
        .monochrome = false,
        .flags = {
            .swap_bytes = true,
            .sw_rotate = false
        }
    };
    main_display = lvgl_port_add_disp(&disp_cfg);
    lv_disp_set_rotation(main_display, LV_DISPLAY_ROTATION_90);


    const button_config_t button_config[] = {
        {
            .type = BUTTON_TYPE_GPIO,
            .gpio_button_config.gpio_num = 15,
            .gpio_button_config.active_level = 1,
            .long_press_time = 2000,
            .short_press_time = 100
        }
    };

    iot_button_register_cb(iot_button_create(&button_config[0]), BUTTON_SINGLE_CLICK, btn_event_cb, NULL);
}

void set_data(float data_to[5]) {
    memcpy(data_, data_to, sizeof(float)*5);
}

void set_dpp_key(const char * k, unsigned int len) {
    dpp_key = calloc(len+1, sizeof(char));
    strcpy(dpp_key, k);
}