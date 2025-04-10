#include "renderer.h"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_types.h>
#include <freertos/FreeRTOS.h>

#include <drivers/lcd_panel.h>

/**
 * Called wehenever the frame buffer DMA is finished, this lets us know we can now start working on
 * processing the next frame
 */
static bool on_frame_buf_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(user_ctx, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken;
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