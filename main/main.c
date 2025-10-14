#include <esp_err.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>

#include "drivers/lcd_panel.h"
#include "gui/renderer.h"
#include "protocol/ble.h"

/// The current scroll position
float scroll_pos = 150.f;

static void input_task() {
    // TODO: should we move to our own init code? need to check if the touch
    //       controller has any special features we might wanna use other than
    //       the raw input
    esp_lcd_touch_handle_t touch = lcd_panel_init_touch();

    // Last state, for delta computation
    bool last_pressed = false;
    uint16_t last_y = 0;

    // Velocities. Using the integer version while the finger is down for performance
    int16_t velocity = 0;
    float fvelocity = 0;

    for (;;) {
        // poll the touch controller for inputs and get them
        esp_lcd_touch_read_data(touch);
        uint16_t touch_x[1];
        uint16_t touch_y[1];
        uint16_t touch_strength[1];
        uint8_t touch_cnt = 0;
        // Track only one finger, to simplify UI
        bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch, touch_x, touch_y, touch_strength, &touch_cnt, 1);

        // TODO: implement rubberbanding
        if (touchpad_pressed && last_pressed) {
            // Finger is held, track it
            scroll_pos += (float) (touch_y[0] - last_y);
        } else if (!touchpad_pressed) {
            if (last_pressed) {
                // Finger was just lifted, switch to float velocity
                fvelocity = (float) (int32_t) velocity;
            } else if (fvelocity != 0) {
                // Apply drag to velocity. Eventually stop to save compute.
                fvelocity *= 0.92f;
                if (fvelocity < 0.2 && fvelocity > -0.2) {
                    fvelocity = 0;
                }
            }
            // Scroll according to current velocity
            scroll_pos += fvelocity;

            // Restrict scroll area with rubber-banding
            if (scroll_pos < 0) {
                scroll_pos -= scroll_pos * 0.25f;
                if (scroll_pos >= -0.2) {
                    scroll_pos = 0;
                }
                if (fvelocity < 0) {
                    fvelocity *= 0.8f; // Apply drag faster when out of bounds
                }
            }
            float scroll_end = LCD_HEIGHT - 169;
            if (scroll_pos > scroll_end) {
                scroll_pos -= (scroll_pos - scroll_end) * 0.25f;
                if (scroll_pos <= scroll_end + 0.2) {
                    scroll_pos = scroll_end;
                }
                if (fvelocity > 0) {
                    fvelocity *= 0.8f; // Apply drag faster when out of bounds
                }
            }
        }

        last_pressed = touchpad_pressed;
        // This assumes the FPS is mostly constant. Seems logical IMO
        velocity = (int16_t)((int16_t) touch_y[0] - (int16_t)last_y);
        last_y = touch_y[0];

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
