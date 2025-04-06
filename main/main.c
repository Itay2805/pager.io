#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <math.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/cache.h>

#include "lcd_panel.h"

#define COLOR_LUT_SIZE 256
static uint16_t rainbow_lut[COLOR_LUT_SIZE];

// Convert HSV hue [0,1] to RGB565 (used once for LUT generation)
static uint16_t hsv_to_rgb565(float h) {
    float r = 0, g = 0, b = 0;
    float s = 1.0f, v = 1.0f;
    int i = (int)(h * 6.0f);
    float f = (h * 6.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    uint8_t R = (uint8_t)(r * 255.0f);
    uint8_t G = (uint8_t)(g * 255.0f);
    uint8_t B = (uint8_t)(b * 255.0f);
    return ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
}

// Call once before rendering
void init_rainbow_lut() {
    for (int i = 0; i < COLOR_LUT_SIZE; i++) {
        float h = (float)i / COLOR_LUT_SIZE;
        rainbow_lut[i] = hsv_to_rgb565(h);
    }
}

// Fast diagonal rainbow using LUT
void draw_diagonal_rainbow_lut(uint16_t* framebuffer, int width, int height, int frame, int speed) {
    int max_sum = width + height;
    int shift = (frame * speed) % COLOR_LUT_SIZE;

    for (int y = 0; y < height; y++) {
        uint16_t* row = framebuffer + y * width;
        for (int x = 0; x < width; x++) {
            int sum = x + y;
            int index = (sum * COLOR_LUT_SIZE / max_sum + shift) % COLOR_LUT_SIZE;
            row[x] = rainbow_lut[index];
        }
    }
}

bool on_frame_buf_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(user_ctx, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken;
}

static void render_task_main(void* arg) {
    ESP_UNUSED(arg);

    printf("Render task started!\n");

    // the panel init must be done in the task to ensure that the
    // interrupt registration happens on the correct core
    lcd_panel_init_panel();

    // register to get a notification when the framebuffer transmission finishes so
    // we stay in sync and not tear the screen
    esp_lcd_rgb_panel_event_callbacks_t callbacks = {
        .on_frame_buf_complete = on_frame_buf_complete
    };
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_rgb_panel_register_event_callbacks(
        g_lcd_panel_handle,
        &callbacks,
        xTaskGetCurrentTaskHandle()
    ));

    // get the framebuffer of the panel
    void* framebuffer;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_rgb_panel_get_frame_buffer(g_lcd_panel_handle, 1, &framebuffer));

    // precalculate the lut
    init_rainbow_lut();

    // and start the render loop
    int frame = 0;
    for (;;) {
        // wait for the last framebuffer transmission to finish
        // before we are trying to draw the next one
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // and now we can draw
        draw_diagonal_rainbow_lut(framebuffer, 480, 480, frame, 3);
        frame++;
    }
}


void app_main(void) {
    // Create a max priority task on the second core, task
    // will be in charge of rendering to allow the main core
    // to run an event loop
    TaskHandle_t render_task;
    if (xTaskCreatePinnedToCore(
        render_task_main,
        "render_task",
        3584,
        NULL,
        configMAX_PRIORITIES - 1,
        &render_task,
        1
    ) != pdPASS) {
        printf("Failed to create render task!\n");
        return;
    }

    //
    // Handle input
    //

    lcd_panel_init_touch();

    for (;;) {
        // poll for touch events and handle them in here
        esp_lcd_touch_read_data(g_lcd_touch_handle);
        uint16_t touch_x[5];
        uint16_t touch_y[5];
        uint16_t touch_strength[5];
        uint8_t touch_cnt = 0;
        bool touchpad_pressed = esp_lcd_touch_get_coordinates(g_lcd_touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, 5);
        if (touchpad_pressed) {
            printf("Touched!\n");
            for (int i = 0; i < touch_cnt; i++) {
                printf("\t%d) %d,%d - %d\n", i, touch_x[i], touch_y[i], touch_strength[i]);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
