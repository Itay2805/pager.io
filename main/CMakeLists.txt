idf_component_register(SRCS "main.c" "lcd_panel.c"
                    PRIV_REQUIRES esp_lcd_st7701
                    PRIV_REQUIRES esp_lcd_touch_gt911
                    PRIV_REQUIRES esp_lcd_panel_io_additions
                    INCLUDE_DIRS ".")
