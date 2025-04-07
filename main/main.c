#include <esp_heap_caps.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <math.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <rom/cache.h>

#include "lcd_panel.h"

#define MIN(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#define ABS(a) ({ __typeof__(a) _a = (a); _a < 0 ? -_a : _a; })

// darkening is 0-63, 64 is no change, 65-255 is lightening
uint16_t mult_color(uint16_t color, uint8_t multiplier) {
    uint16_t r = (color & 0xF800) >> 11;
    uint16_t g = (color & 0x07E0) >> 5;
    uint16_t b = (color & 0x001F);
    r = MIN(r * ((uint16_t)multiplier / 2) / 32, 31);
    g = MIN(g * (uint16_t)multiplier / 64, 63);
    b = MIN(b * ((uint16_t)multiplier / 2) / 32, 31);
    return ((r & 0xF8) << 11) | ((g & 0xFC) << 5) | (b & 0x1F);
}

struct component_t {
    // render fn
    void (*render)(struct component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1);
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;
    union {
        struct {
            uint16_t color;
        } rect;
    } data;
};

#define MAX_COMPONENTS 64
#define FB_WIDTH 480
#define FB_HEIGHT 480
static struct component_t component_storage[MAX_COMPONENTS];
// static struct component_t* y_sorted_components[MAX_COMPONENTS];

void render_rect(struct component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1) {
    uint16_t color = component->data.rect.color;
    uint32_t color_pair = (color << 16) | color;
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

uint32_t font_buffer[16/4] = {
    0x10000000, 0xc6c66c38, 0xc6c6c6fe, 0x000000c6
};

void render_text(struct component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1) {
    int16_t font_y = (y - component->top) / 2;
    if (font_y < 0 || font_y >= 16) {
        return;
    }
    uint8_t font_row = *((uint8_t*)font_buffer + font_y);
    for (int x = x0; x < x1; x++) {
        uint8_t font_col = ((x - x0) / 2) % 8;
        uint8_t font_bit = (font_row >> font_col) & 1;
        if (font_bit) {
            ((uint16_t*)framebuffer_row)[x] = component->data.rect.color;
        }
    }
}

// Fast diagonal rainbow using LUT
void draw_everything(uint32_t* framebuffer, int frame, int speed) {
    component_storage[0].render = render_rect;
    // component_storage[0].data.rect.color = rainbow_lut[frame % COLOR_LUT_SIZE];
    component_storage[0].data.rect.color = 0x0000;
    component_storage[0].left = 0;
    component_storage[0].top = 0;
    component_storage[0].right = FB_WIDTH;
    component_storage[0].bottom = FB_HEIGHT;
    // uint32_t scroll = ABS((frame * 4) % 1000 - 500);
    uint32_t scroll = sinf(frame * 0.02f) * 500.0f + 500.0f;
    // uint32_t scroll = 0;
    for (int i = 0; i < 16; i++) {
        int32_t top_offset = i * (120 + 6) - scroll;
        int32_t adj_top_offset = top_offset < 0 ? top_offset / 2 : top_offset;
        int32_t shrinkage = top_offset < 0 ? top_offset / 8 : 0;
        component_storage[i*2+1].render = render_rect;
        component_storage[i*2+1].data.rect.color = top_offset < 0 ? mult_color(0xaa22, MAX(0, 64+top_offset/6)) : 0xaa22;
        component_storage[i*2+1].left = 16 - shrinkage;
        component_storage[i*2+1].top = 32 + adj_top_offset - shrinkage;
        component_storage[i*2+1].right = FB_WIDTH - 16 + shrinkage;
        component_storage[i*2+1].bottom = 120 + 32 + adj_top_offset + shrinkage;

        component_storage[i*2+2].render = render_text;
        component_storage[i*2+2].data.rect.color = top_offset < 0 ? mult_color(0xffff, MAX(0, 64+top_offset/4)) : 0xffff;
        component_storage[i*2+2].left = 32 - shrinkage;
        component_storage[i*2+2].top = 64 + adj_top_offset - shrinkage;
        component_storage[i*2+2].right = FB_WIDTH - 32 + shrinkage;
        component_storage[i*2+2].bottom = 120 + 32 + adj_top_offset + shrinkage;
    }

    // for (int i = 1; i < MAX_COMPONENTS; i++) {
    //     int16_t off = ABS(((frame * 4 + i * 3) % 200) - 100);
    //     component_storage[i].render = render_rect;
    //     component_storage[i].data.rect.color = i << 8 | i;
    //     component_storage[i].left = 16 + off;
    //     component_storage[i].top = 16 + i * 2;
    //     component_storage[i].right = 48 + off;
    //     component_storage[i].bottom = 48 + i * 2;
    // }

    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int c = 0; c < MAX_COMPONENTS; c++) {
            struct component_t* component = &component_storage[c];
            if (component->render == NULL) {
                continue;
            }
            if (y >= component->top && y < component->bottom) {
                uint32_t* framebuffer_row = framebuffer + y * (FB_WIDTH/2);
                component->render(component, framebuffer_row, y, MAX(component->left, 0), MIN(component->right, FB_WIDTH));
            }
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

    // ensure we have the DMA at a fixed point right now
    esp_lcd_rgb_panel_restart(g_lcd_panel_handle);

    // and start the render loop
    int frame = 0;
    for (;;) {
        // wait for the last framebuffer transmission to finish
        // before we are trying to draw the next one
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        draw_everything(framebuffer, frame, 3);
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
