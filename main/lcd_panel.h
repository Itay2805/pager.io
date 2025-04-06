#pragma once

#include <esp_lcd_touch.h>
#include <esp_lcd_types.h>

extern esp_lcd_panel_handle_t g_lcd_panel_handle;
extern esp_lcd_touch_handle_t g_lcd_touch_handle;

void lcd_panel_init_panel(void);
void lcd_panel_init_touch(void);
