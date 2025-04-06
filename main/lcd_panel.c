#include <esp_lcd_io_spi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_st7701.h>
#include <esp_lcd_touch_gt911.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/i2c_master.h>

esp_lcd_panel_handle_t g_lcd_panel_handle;
esp_lcd_touch_handle_t g_lcd_touch_handle;

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

void lcd_panel_init_panel(void) {
    //
    // Setup the io bus, we are going to bit-bang the SPI bus
    //
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
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    //
    // Configure the RGB interface, should allow for nice
    // smooth 60fps redraw of the framebuffer
    //
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

    //
    // Turn on the backlight
    //
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << 38
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(38, 1));
}

void lcd_panel_init_touch(void) {
    //
    // Setup the i2c bus for the touch controller
    //
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .scl_io_num = 45,
        .sda_io_num = 19,
        .master = {
            .clk_speed = 100 * 1000
        }
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    //
    // Create an IO handle for the i2c bus
    //
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(I2C_NUM_0, &io_config, &io_handle));

    //
    // Configure the GT911 touch controller
    //
    esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
        .dev_addr = io_config.dev_addr,
    };
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = 480,
        .y_max = 480,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .driver_data = &tp_gt911_config,
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(io_handle, &tp_cfg, &g_lcd_touch_handle));
}
