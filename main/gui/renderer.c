#include "renderer.h"
#include "../heatshrink/heatshrink_decoder.h"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_types.h>
#include <freertos/FreeRTOS.h>

#include <drivers/lcd_panel.h>

#include "/home/david/code/experiments/heatshrink/img1.565.hs.h"
// #include "/home/david/code/experiments/heatshrink/img2.565.hs.h"

#include <esp_task_wdt.h>
#include <string.h>

#define MIN(A, B) ({__typeof__(A) _a = (A); __typeof__(B) _b = (B); _a < _b ? _a : _b; })
#define MAX(A, B) ({__typeof__(A) _a = (A); __typeof__(B) _b = (B); _a > _b ? _a : _b; })

// TODO: move to `scrolling.h`
extern float scroll_pos;

heatshrink_decoder decoder = {0};

#define RENDER_LOG(...)
// #define RENDER_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__);

/**
 * Called wehenever the frame buffer DMA is finished, this lets us know we can now start working on
 * processing the next frame
 */
static bool on_frame_buf_complete(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(user_ctx, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken;
}

void render_rect_row(uint32_t *framebuffer_row, uint16_t color, uint16_t x0,
                     uint16_t x1) {
  const uint32_t color_pair = (color << 16) | color;
  if (x0 % 2 == 1) {
    ((uint16_t *)framebuffer_row)[x0] = color;
  }
  for (int x = x0 / 2; x < x1 / 2; x++) {
    framebuffer_row[x] = color_pair;
  }
  if (x1 % 2 == 1) {
    ((uint16_t *)framebuffer_row)[x1 - 1] = color;
  }
}

static void feed_decoder(const uint8_t **cimg, size_t *cimg_len) {
    size_t inp_size = 0;
    RENDER_LOG("sink(%p, %zu)", *cimg, *cimg_len);
    // The only possible error is null param, we're not checking just for that :)
    heatshrink_decoder_sink(&decoder, *cimg, *cimg_len, &inp_size);
    RENDER_LOG("->%zu\n", inp_size);
    *cimg += inp_size;
    *cimg_len -= inp_size;
    if (*cimg_len == 0) {
        RENDER_LOG("finish\n");
        heatshrink_decoder_finish(&decoder);
    }
}

uint8_t discard_buf[32];

static void render_compressed(
    uint8_t* fb,
    const uint8_t* cimg, size_t cimg_len,
    int16_t x, int16_t y, uint16_t w, uint16_t h,
    uint16_t crop_left, uint16_t crop_top, uint16_t crop_right, uint16_t crop_bottom
) {
    heatshrink_decoder_reset(&decoder);
    const uint8_t* fb_top = fb + LCD_WIDTH * LCD_HEIGHT * 2;

    // Enforced crop when out of bounds
    if (x < 0) crop_left = MAX(crop_left, -x);
    if (y < 0) crop_top = MAX(crop_top, -y);
    if (x + w > LCD_WIDTH) crop_right = MAX(crop_right, x + w - LCD_WIDTH);
    if (y + h > LCD_HEIGHT) crop_bottom = MAX(crop_bottom, y + h - LCD_HEIGHT);
    // Culling
    if (crop_right + crop_left >= w) return;
    if (crop_top + crop_bottom >= h) return;

    RENDER_LOG("x:%d y:%d w:%u h:%u cl:%u ct:%u cr:%u cb:%u\n", x, y, w, h, crop_left, crop_top, crop_right, crop_bottom);

    // Seek framebuffer to first pixel we need to draw
    fb += ((y+crop_top) * LCD_WIDTH + (x+crop_left)) * 2;
    // Skip initial crop region
    size_t skip_bytes = (crop_top * w + crop_left) * 2;

    HSD_poll_res poll_res = HSDR_POLL_EMPTY;
    size_t out_size = 0;
    RENDER_LOG("imgc:%p sz:%zu fb:%p\n", cimg, cimg_len, fb);
    for (int i = 0; i < h - crop_top - crop_bottom; i++) {
        size_t copy_bytes = (w - crop_left - crop_right) * 2;
        uint8_t* initial_fb = fb;

        // Decompress bytes and discard them
        if (skip_bytes) {
            RENDER_LOG("skip_bytes:%zu\n", skip_bytes);
        }
        while (skip_bytes) {
            RENDER_LOG("poll_skip(%zu)", MIN(32, skip_bytes));
            poll_res = heatshrink_decoder_poll(&decoder, discard_buf, MIN(32, skip_bytes), &out_size);
            RENDER_LOG("->%zu{%d}\n", out_size, poll_res);
            if (poll_res < 0) return; // Error
            skip_bytes -= out_size;
            if (poll_res == HSDR_POLL_EMPTY) {
                if (cimg_len == 0) return;
                feed_decoder(&cimg, &cimg_len);
            }
        }

        // Decompress bytes into framebuffer
        while (copy_bytes) {
            RENDER_LOG("poll_cpy(%p, %zu)", fb, copy_bytes);
            if (fb + copy_bytes > fb_top) {
                RENDER_LOG("fb about to overflow\n");
                return; // Error
            }
            poll_res = heatshrink_decoder_poll(&decoder, fb, copy_bytes, &out_size);
            RENDER_LOG("->%zu{%d}\n", out_size, poll_res);
            if (poll_res < 0) return; // Error
            fb += out_size;
            copy_bytes -= out_size;
            if (poll_res == HSDR_POLL_EMPTY) {
                if (cimg_len == 0) return;
                feed_decoder(&cimg, &cimg_len);
            }
        }

        // Skip cropped region
        skip_bytes = (crop_left + crop_right) * 2;
        // Advance fb to next line
        fb = initial_fb + LCD_WIDTH * 2;
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

    // Clear the screen
    for (int row = 0; row < LCD_HEIGHT; ++row) {
        uint32_t* fb_row = (uint32_t*)framebuffer + row * (LCD_WIDTH / 2);
        render_rect_row(fb_row, 0x1082, 0, LCD_WIDTH);
    }

    // to ensure we are in sync with the hardware properly restart
    // the DMA process just in case
    esp_lcd_rgb_panel_restart(panel);
    bool which_img = true;
    int16_t prev_scroll_pos = scroll_pos;

    // and start the render loop
    for (;;) {
        vTaskDelay(1); // Feed watchdog
        // wait for the last framebuffer transmission to finish
        // before we are trying to draw the next one
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        which_img = !which_img;
        printf(which_img ? "1\n" : "0\n");
        int16_t iscroll_pos = (int16_t) scroll_pos;
        if (iscroll_pos > prev_scroll_pos) {
            for (int row = MAX(prev_scroll_pos, 0); row < MIN(iscroll_pos, LCD_HEIGHT); ++row) {
                uint32_t* fb_row = (uint32_t*)framebuffer + row * (LCD_WIDTH / 2);
                render_rect_row(fb_row, 0x1082, 55, LCD_WIDTH-55);
            }
        } else {
            for (int row = MAX(iscroll_pos + imgc1_height, 0); row < MIN(prev_scroll_pos + imgc1_height, LCD_HEIGHT); ++row) {
                uint32_t* fb_row = (uint32_t*)framebuffer + row * (LCD_WIDTH / 2);
                render_rect_row(fb_row, 0x1082, 55, LCD_WIDTH-55);
            }
        }
        render_compressed(
            framebuffer,
            imgc1,
            sizeof(imgc1),
            // which_img ? imgc2 : imgc1,
            // which_img ? sizeof(imgc2) : sizeof(imgc1),
            55, iscroll_pos, imgc1_width, imgc1_height,
            0, 0, 0, 0
        );

        prev_scroll_pos = iscroll_pos;

        // for (int y = 0; y < LCD_HEIGHT; y++) {
        //     uint32_t* fb_row = (uint32_t*)framebuffer + y * (LCD_WIDTH / 2);
        //
        //     // Display a repeating list of rectangles, reacting to the scroll_pos
        //     const float one_over_256 = 0.00390625f; // Mult much faster than div
        //     // Float remainder. Can maybe use `remainderf`
        //     int16_t scroll_y = (int16_t)(scroll_pos - (float)((int32_t)(scroll_pos * one_over_256) * 256));
        //     if (scroll_y < 0) scroll_y = (int16_t) (scroll_y + 256);
        //
        //     int adj_y = (y - scroll_y) % 256;
        //     if (adj_y < 0) adj_y = 256 + adj_y;
        //
        //     const uint16_t color = adj_y > 200 ? 0xfafa : 0x0;
        //     render_rect_row(fb_row, color, 128,  LCD_WIDTH - 128);
        // }

    }
}

void init_renderer(void) {
    //
    // Create a max priority task on the second core, task
    // will be in charge of rendering to allow the main core
    // to run an event loop
    //
    // memcpy(imgc1_, imgc1, sizeof(imgc1));
    // memcpy(imgc2_, imgc2, sizeof(imgc2));
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