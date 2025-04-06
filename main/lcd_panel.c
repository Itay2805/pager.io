// #include <esp_lcd_panel_rgb.h>
// #include <esp_log.h>
// #include <esp_psram.h>
// #include <lcd_panel.h>
// #include <math.h>
// #include <soc/gpio_struct.h>
//
// #include "esp_lcd_panel_io.h"
// #include "esp_lcd_panel_ops.h"
// #include "driver/gpio.h"
//
// esp_lcd_panel_handle_t g_lcd_panel_handle;
//
// static void lcd_panel_cs_high() { GPIO.out1_w1ts.val |= 1 << (39 & 31); }
// static void lcd_panel_cs_low() { GPIO.out1_w1tc.val |= 1 << (39 & 31); }
// static void lcd_panel_sda_high() { GPIO.out1_w1ts.val |= 1 << (47 & 31); }
// static void lcd_panel_sda_low() { GPIO.out1_w1tc.val |= 1 << (47 & 31); }
// static void lcd_panel_sck_high() { GPIO.out1_w1ts.val |= 1 << (48 & 31); }
// static void lcd_panel_sck_low() { GPIO.out1_w1tc.val |= 1 << (48 & 31); }
//
// static void lcd_panel_bl_high() { GPIO.out1_w1ts.val |= 1 << (38 & 31); }
// static void lcd_panel_bl_low() { GPIO.out1_w1tc.val |= 1 << (38 & 31); }
//
// static void lcd_panel_begin_write(void) {
//     lcd_panel_cs_low();
// }
//
// static void lcd_panel_end_write(void) {
//     lcd_panel_cs_high();
// }
//
// static void lcd_panel_write_command(uint8_t c) {
//     lcd_panel_sda_low();
//     lcd_panel_sck_high();
//     lcd_panel_sck_low();
//
//     uint8_t bit = 0x80;
//     while (bit) {
//         if (c & bit) {
//             lcd_panel_sda_high();
//         } else {
//             lcd_panel_sda_low();
//         }
//         lcd_panel_sck_high();
//         bit >>= 1;
//         lcd_panel_sck_low();
//     }
// }
//
// static void lcd_panel_write(uint8_t d) {
//     lcd_panel_sda_high();
//     lcd_panel_sck_high();
//     lcd_panel_sck_low();
//
//     uint8_t bit = 0x80;
//     while (bit) {
//         if (d & bit) {
//             lcd_panel_sda_high();
//         } else {
//             lcd_panel_sda_low();
//         }
//         lcd_panel_sck_high();
//         bit >>= 1;
//         lcd_panel_sck_low();
//     }
// }
//
// static void lcd_panel_send_command(uint8_t c) {
//     lcd_panel_begin_write();
//     lcd_panel_write_command(c);
//     lcd_panel_end_write();
// }
//
// void lcd_panel_init(void) {
//     // Setup the SPI pins
//     gpio_config_t cs_conf = {
//         .pin_bit_mask = (1ULL << 38) | (1ULL << 39) | (1ULL << 47) | (1ULL << 48),
//         .mode = GPIO_MODE_OUTPUT,
//         .pull_up_en = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE
//     };
//     gpio_config(&cs_conf);
//     lcd_panel_cs_high();
//     lcd_panel_sck_low();
//     lcd_panel_sda_low();
//
//     // reset the display
//     lcd_panel_send_command(0x01);
//     vTaskDelay(pdMS_TO_TICKS(120));
//
//     // initialize the display correctly
//     lcd_panel_begin_write();
//     lcd_panel_write_command(0xFF);
//     lcd_panel_write(0x77);
//     lcd_panel_write(0x01);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x13);
//     lcd_panel_write_command(0xEF);
//     lcd_panel_write(0x08);
//     lcd_panel_write_command(0xFF);
//     lcd_panel_write(0x77);
//     lcd_panel_write(0x01);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x10);
//     lcd_panel_write_command(0xC0);
//     lcd_panel_write(0x3B);
//     lcd_panel_write(0x00);
//     lcd_panel_write_command(0xC1);
//     lcd_panel_write(0x0D);
//     lcd_panel_write(0x02);
//     lcd_panel_write_command(0xC2);
//     lcd_panel_write(0x21);
//     lcd_panel_write(0x08);
//     lcd_panel_write_command(0xCD);
//     lcd_panel_write(0x00);//18-bit/pixel: MDT=0:D[21:16]=R,D[13:8]=G,D[5:0]=B(CDH=00) ;
//                                      // MDT=1:D[17:12]=R,D[11:6]=G,D[5:0]=B(CDH=08) ;
//
//
//     lcd_panel_write_command(0xB0);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x18);
//     lcd_panel_write(0x0E);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x06);
//     lcd_panel_write(0x07);
//     lcd_panel_write(0x08);
//     lcd_panel_write(0x07);
//     lcd_panel_write(0x22);
//     lcd_panel_write(0x04);
//     lcd_panel_write(0x12);
//     lcd_panel_write(0x0F);
//     lcd_panel_write(0xAA);
//     lcd_panel_write(0x31);
//     lcd_panel_write(0x18);
//     lcd_panel_write_command(0xB1);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x19);
//     lcd_panel_write(0x0E);
//     lcd_panel_write(0x12);
//     lcd_panel_write(0x07);
//     lcd_panel_write(0x08);
//     lcd_panel_write(0x08);
//     lcd_panel_write(0x08);
//     lcd_panel_write(0x22);
//     lcd_panel_write(0x04);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0xA9);
//     lcd_panel_write(0x32);
//     lcd_panel_write(0x18);
//     lcd_panel_write_command(0xFF);
//     lcd_panel_write(0x77);
//     lcd_panel_write(0x01);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x11);
//     lcd_panel_write_command(0xB0);
//     lcd_panel_write(0x60);
//     lcd_panel_write_command(0xB1);
//     lcd_panel_write(0x30);
//     lcd_panel_write_command(0xB2);
//     lcd_panel_write(0x87);
//     lcd_panel_write_command(0xB3);
//     lcd_panel_write(0x80);
//     lcd_panel_write_command(0xB5);
//     lcd_panel_write(0x49);
//     lcd_panel_write_command(0xB7);
//     lcd_panel_write(0x85);
//     lcd_panel_write_command(0xB8);
//     lcd_panel_write(0x21);
//     lcd_panel_write_command(0xC1);
//     lcd_panel_write(0x78);
//     lcd_panel_write_command(0xC2);
//     lcd_panel_write(0x78);
//     vTaskDelay(pdMS_TO_TICKS(20));
//     lcd_panel_write_command(0xE0);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x1B);
//     lcd_panel_write(0x02);
//     lcd_panel_write_command(0xE1);
//     lcd_panel_write(0x08);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x07);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x44);
//     lcd_panel_write(0x44);
//     lcd_panel_write_command(0xE2);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x44);
//     lcd_panel_write(0x44);
//     lcd_panel_write(0xED);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0xEC);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write_command(0xE3);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x11);
//     lcd_panel_write_command(0xE4);
//     lcd_panel_write(0x44);
//     lcd_panel_write(0x44);
//     lcd_panel_write_command(0xE5);
//     lcd_panel_write(0x0A);
//     lcd_panel_write(0xE9);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x0C);
//     lcd_panel_write(0xEB);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x0E);
//     lcd_panel_write(0xED);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x10);
//     lcd_panel_write(0xEF);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write_command(0xE6);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x11);
//     lcd_panel_write(0x11);
//     lcd_panel_write_command(0xE7);
//     lcd_panel_write(0x44);
//     lcd_panel_write(0x44);
//     lcd_panel_write_command(0xE8);
//     lcd_panel_write(0x09);
//     lcd_panel_write(0xE8);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x0B);
//     lcd_panel_write(0xEA);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x0D);
//     lcd_panel_write(0xEC);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write(0x0F);
//     lcd_panel_write(0xEE);
//     lcd_panel_write(0xD8);
//     lcd_panel_write(0xA0);
//     lcd_panel_write_command(0xEB);
//     lcd_panel_write(0x02);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0xE4);
//     lcd_panel_write(0xE4);
//     lcd_panel_write(0x88);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x40);
//     lcd_panel_write_command(0xEC);
//     lcd_panel_write(0x3C);
//     lcd_panel_write(0x00);
//     lcd_panel_write_command(0xED);
//     lcd_panel_write(0xAB);
//     lcd_panel_write(0x89);
//     lcd_panel_write(0x76);
//     lcd_panel_write(0x54);
//     lcd_panel_write(0x02);
//     lcd_panel_write(0xFF);
//     lcd_panel_write(0xFF);
//     lcd_panel_write(0xFF);
//     lcd_panel_write(0xFF);
//     lcd_panel_write(0xFF);
//     lcd_panel_write(0xFF);
//     lcd_panel_write(0x20);
//     lcd_panel_write(0x45);
//     lcd_panel_write(0x67);
//     lcd_panel_write(0x98);
//     lcd_panel_write(0xBA);
//     lcd_panel_write_command(0xFF);
//     lcd_panel_write(0x77);
//     lcd_panel_write(0x01);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//
//     lcd_panel_write_command(0x3A);
//     lcd_panel_write(0x66);//55/50=16bit(RGB565);66=18bit(RGB666);77��Ĭ�ϲ�д3AH��=24bit(RGB888)
//
//     lcd_panel_write_command(0x36);
//     lcd_panel_write(0x08);
//
//     lcd_panel_write_command(0x11);
//     vTaskDelay(pdMS_TO_TICKS(120));
//     lcd_panel_write_command(0x29);
//     vTaskDelay(pdMS_TO_TICKS(20));
//
//     // ensure its not inverted
//     lcd_panel_send_command(0x20);
//
//     // ensure the rotation is correct
//     lcd_panel_begin_write();
//     lcd_panel_write_command(0xFF);
//     lcd_panel_write(0x77);
//     lcd_panel_write(0x01);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x10);
//     lcd_panel_write(0xC7);
//     lcd_panel_write(0x00);
//     lcd_panel_write_command(0xFF);
//     lcd_panel_write(0x77);
//     lcd_panel_write(0x01);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x00);
//     lcd_panel_write(0x36);
//     lcd_panel_write(0x00);
//     lcd_panel_end_write();
//
//     // Setup the framebuffer
//     esp_lcd_rgb_panel_config_t panel_config = {
//         .clk_src = LCD_CLK_SRC_DEFAULT,
//         .timings = {
//             .pclk_hz = 16000000,
//             .h_res = 480,
//             .v_res = 480,
//             .hsync_front_porch = 20,
//             .hsync_pulse_width = 10,
//             .hsync_back_porch = 10,
//             .vsync_front_porch = 10,
//             .vsync_pulse_width = 10,
//             .vsync_back_porch = 10,
//             .flags = {
//                 .hsync_idle_low = 0,
//                 .vsync_idle_low = 0,
//                 .de_idle_high = 0,
//                 .pclk_idle_high = 0,
//                 .pclk_active_neg = 0
//             }
//         },
//         .data_width = 16,
//         .num_fbs = 1,
//         .bits_per_pixel = 16,
//         .sram_trans_align = 8,
//         .psram_trans_align = 64,
//         .hsync_gpio_num = 16,
//         .vsync_gpio_num = 17,
//         .de_gpio_num = 18,
//         .pclk_gpio_num = 21,
//         .disp_gpio_num = -1,
//         .data_gpio_nums = {
//              4,  5,  6,  7, 15, // B
//              8, 20,  3, 46,  9, 10, // G
//             11, 12, 13, 14,  0, // R
//         },
//         .flags = {
//             .disp_active_low = true,
//             .refresh_on_demand = false,
//             .fb_in_psram = true,
//             .double_fb = false,
//             .no_fb = false,
//             .bb_invalidate_cache = false
//         }
//     };
//     ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_rgb_panel(&panel_config, &g_lcd_panel_handle));
//     ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_reset(g_lcd_panel_handle));
//     ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_panel_init(g_lcd_panel_handle));
//
//     lcd_panel_bl_high();
// }


#include <esp_lcd_io_spi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_st7701.h>
#include <driver/gpio.h>

esp_lcd_panel_handle_t g_lcd_panel_handle;

bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

static const st7701_lcd_init_cmd_t m_st7701_type1_init_operations[] = {
    { 0xFF,(uint8_t[]){ 0x77,0x01,0x00,0x00,0x10 }, 0x5, 0 },
    { 0xC0,(uint8_t[]){ 0x3B,0x00 }, 0x2, 0 },
    { 0xC1,(uint8_t[]){ 0x0D,0x02 }, 0x2, 0 },
    { 0xC2,(uint8_t[]){ 0x31,0x05 }, 0x2, 0 },
    { 0xCD,(uint8_t[]){ 0x00 }, 0x1, 0 },
    { 0xB0,(uint8_t[]){ 0x00,0x11,0x18,0x0E,0x11,0x06,0x07,0x08,0x07,0x22,0x04,0x12,0x0F,0xAA,0x31,0x18 }, 0x10, 0 },
    { 0xB1,(uint8_t[]){ 0x00,0x11,0x19,0x0E,0x12,0x07,0x08,0x08,0x08,0x22,0x04,0x11,0x11,0xA9,0x32,0x18 }, 0x10, 0 },
    { 0xFF,(uint8_t[]){ 0x77,0x01,0x00,0x00,0x11 }, 0x5, 0 },
    { 0xB0,(uint8_t[]){ 0x60 }, 0x1, 0 },
    { 0xB1,(uint8_t[]){ 0x32 }, 0x1, 0 },
    { 0xB2,(uint8_t[]){ 0x07 }, 0x1, 0 },
    { 0xB3,(uint8_t[]){ 0x80 }, 0x1, 0 },
    { 0xB5,(uint8_t[]){ 0x49 }, 0x1, 0 },
    { 0xB7,(uint8_t[]){ 0x85 }, 0x1, 0 },
    { 0xB8,(uint8_t[]){ 0x21 }, 0x1, 0 },
    { 0xC1,(uint8_t[]){ 0x78 }, 0x1, 0 },
    { 0xC2,(uint8_t[]){ 0x78 }, 0x1, 0 },
    { 0xE0,(uint8_t[]){ 0x00,0x1B,0x02 }, 0x3, 0 },
    { 0xE1,(uint8_t[]){ 0x08,0xA0,0x00,0x00,0x07,0xA0,0x00,0x00,0x00,0x44,0x44 }, 0xB, 0 },
    { 0xE2,(uint8_t[]){ 0x11,0x11,0x44,0x44,0xED,0xA0,0x00,0x00,0xEC,0xA0,0x00,0x00 }, 0xC, 0 },
    { 0xE3,(uint8_t[]){ 0x00,0x00,0x11,0x11 }, 0x4, 0 },
    { 0xE4,(uint8_t[]){ 0x44,0x44 }, 0x2, 0 },
    { 0xE5,(uint8_t[]){ 0x0A,0xE9,0xD8,0xA0,0x0C,0xEB,0xD8,0xA0,0x0E,0xED,0xD8,0xA0,0x10,0xEF,0xD8,0xA0 }, 0x10, 0 },
    { 0xE6,(uint8_t[]){ 0x00,0x00,0x11,0x11 }, 0x4, 0 },
    { 0xE7,(uint8_t[]){ 0x44,0x44 }, 0x2, 0 },
    { 0xE8,(uint8_t[]){ 0x09,0xE8,0xD8,0xA0,0x0B,0xEA,0xD8,0xA0,0x0D,0xEC,0xD8,0xA0,0x0F,0xEE,0xD8,0xA0 }, 0x10, 0 },
    { 0xEB,(uint8_t[]){ 0x02,0x00,0xE4,0xE4,0x88,0x00,0x40 }, 0x7, 0 },
    { 0xEC,(uint8_t[]){ 0x3C,0x00 }, 0x2, 0 },
    { 0xED,(uint8_t[]){ 0xAB,0x89,0x76,0x54,0x02,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x20,0x45,0x67,0x98,0xBA }, 0x10, 0 },
    { 0xFF,(uint8_t[]){ 0x77,0x01,0x00,0x00,0x13 }, 0x5, 0 },
    { 0xE5,(uint8_t[]){ 0xE4 }, 0x1, 0 },
    { 0xFF,(uint8_t[]){ 0x77,0x01,0x00,0x00,0x00 }, 0x5, 0 },
    { 0x21,(uint8_t[]){ }, 0x0, 0 },
    { 0x3A,(uint8_t[]){ 0x60 }, 0x1, 0 },
    { 0x11,(uint8_t[]){ }, 0x0, 150 },
    { 0x29,(uint8_t[]){ }, 0x0, 150 },
    { 0x20,(uint8_t[]){ }, 0x0, 150 },
};

void lcd_panel_init(void) {
    esp_lcd_panel_io_handle_t io_handle = NULL;
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = 39,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = 48,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = 47,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = 16,
        .de_gpio_num = 18,
        .pclk_gpio_num = 21,
        .vsync_gpio_num = 17,
        .hsync_gpio_num = 16,
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            4, 5, 6, 7, 15,
            8, 20, 3, 46, 9, 10,
            11, 12, 13, 14, 0
        },
        .timings = {
            .pclk_hz = 16 * 1000 * 1000,
            .h_res = 480,
            .v_res = 480,
            .hsync_front_porch = 10,
            .hsync_pulse_width = 8,
            .hsync_back_porch = 50,
            .vsync_front_porch = 10,
            .vsync_pulse_width = 8,
            .vsync_back_porch = 20,
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .pclk_active_neg = 0,
            }
        },
        .bounce_buffer_size_px = 480 * 16,
        .flags.fb_in_psram = 1,
    };
    st7701_vendor_config_t vendor_config = {
        .init_cmds = m_st7701_type1_init_operations,
        .init_cmds_size = sizeof(m_st7701_type1_init_operations) / sizeof(m_st7701_type1_init_operations[0]),
        .rgb_config = &rgb_config,
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &g_lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(g_lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(g_lcd_panel_handle));

    // Turn on the backlight
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << 38
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(38, 1));
}
