idf_component_register(SRCS "main.c" "lcd_panel.c" "notifications.c"
                    PRIV_REQUIRES esp_lcd_st7701
                    PRIV_REQUIRES esp_lcd_touch_gt911
                    PRIV_REQUIRES esp_lcd_panel_io_additions
                    PRIV_REQUIRES bt
                    PRIV_REQUIRES nvs_flash
                    INCLUDE_DIRS ".")
