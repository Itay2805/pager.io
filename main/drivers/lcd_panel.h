#pragma once

#include <esp_lcd_touch.h>
#include <esp_lcd_types.h>

esp_lcd_panel_handle_t lcd_panel_init_panel(void);
esp_lcd_touch_handle_t lcd_panel_init_touch(void);
