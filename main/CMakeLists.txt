file(GLOB_RECURSE SOURCES *.h *.c)

idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES esp_lcd_st7701
                    PRIV_REQUIRES esp_lcd_touch_gt911
                    PRIV_REQUIRES esp_lcd_panel_io_additions
                    PRIV_REQUIRES bt
                    PRIV_REQUIRES nvs_flash
                    INCLUDE_DIRS ".")
