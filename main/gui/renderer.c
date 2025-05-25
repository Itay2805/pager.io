#include "renderer.h"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_types.h>
#include <freertos/FreeRTOS.h>

#include <drivers/lcd_panel.h>

// TODO: move to `scrolling.h`
extern float scroll_pos;


/**
 * Called wehenever the frame buffer DMA is finished, this lets us know we can now start working on
 * processing the next frame
 */
static bool on_frame_buf_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(user_ctx, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken;
}

void render_rect_row(uint32_t* framebuffer_row, uint16_t color, uint16_t x0, uint16_t x1) {
    const uint32_t color_pair = (color << 16) | color;
    if (x0 % 2 == 1) {
        ((uint16_t*)framebuffer_row)[x0] = color;
    }
    for (int x = x0/2; x < x1/2; x++) {
        framebuffer_row[x] = color_pair;
    }
    if (x1 % 2 == 1) {
        ((uint16_t*)framebuffer_row)[x1-1] = color;
    }
}

static void renderer_task(void* arg) {
    // the panel init must be done in the task to ensure that the
    // interrupt registration happens on the correct core
    esp_lcd_panel_handle_t panel = lcd_panel_init_panel();

    // register to get a notification when the framebuffer transmission finishes so
    // we stay in sync and not tear the screen
    esp_lcd_rgb_panel_event_callbacks_t callbacks = {
        .on_frame_buf_complete = on_frame_buf_complete
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_rgb_panel_register_event_callbacks(
        panel,
        &callbacks,
        xTaskGetCurrentTaskHandle()
    ));

    // get the framebuffer of the panel
    void* framebuffer;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_rgb_panel_get_frame_buffer(panel, 1, &framebuffer));

    // to ensure we are in sync with the hardware properly restart
    // the DMA process just in case
    esp_lcd_rgb_panel_restart(panel);

    // and start the render loop
    for (;;) {
        // wait for the last framebuffer transmission to finish
        // before we are trying to draw the next one
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // TODO: perform whatever the renderer should do in here
        for (int y = 0; y < LCD_HEIGHT; y++) {
            uint32_t* fb_row = (uint32_t*)framebuffer + y * (LCD_WIDTH / 2);

            // Display a repeating list of rectangles, reacting to the scroll_pos
            const float one_over_256 = 0.00390625f; // Mult much faster than div
            // Float remainder. Can maybe use `remainderf`
            int16_t scroll_y = (int16_t)(scroll_pos - (float)((int32_t)(scroll_pos * one_over_256) * 256));
            if (scroll_y < 0) scroll_y = (int16_t) (scroll_y + 256);

            int adj_y = (y - scroll_y) % 256;
            if (adj_y < 0) adj_y = 256 + adj_y;

            const uint16_t color = adj_y > 200 ? 0xfafa : 0x0;
            render_rect_row(fb_row, color, 128,  LCD_WIDTH - 128);
        }
    }
}

void init_renderer(void) {
    //
    // Create a max priority task on the second core, task
    // will be in charge of rendering to allow the main core
    // to run an event loop
    //
    xTaskCreatePinnedToCore(
        renderer_task,
        "render_task",
        3584,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL,
        1
    );
}