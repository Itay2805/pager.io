#include <esp_err.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>

#include "drivers/lcd_panel.h"
#include "gui/renderer.h"
#include "protocol/ble.h"

static void input_task() {
    // TODO: should we move to our own init code? need to check if the touch
    //       controller has any special features we might wanna use other than
    //       the raw input
    esp_lcd_touch_handle_t touch = lcd_panel_init_touch();

    for (;;) {
        // poll the touch controller for inputs and get them
        esp_lcd_touch_read_data(touch);
        uint16_t touch_x[5];
        uint16_t touch_y[5];
        uint16_t touch_strength[5];
        uint8_t touch_cnt = 0;
        bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch, touch_x, touch_y, touch_strength, &touch_cnt, 5);
        // TODO: something with this

        // no need to poll faster than we can render, so
        // this should be good enough
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void app_main(void) {
    // Setup the NVS, BLE needs it
    ESP_ERROR_CHECK(nvs_flash_init());
    init_ble();
    init_renderer();

    // run the input task on top of the main task, for fun and profit
    input_task();
}
