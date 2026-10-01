idf_component_register(
    SRCS "main.c"
         "assets/img_solar.c"
         "assets/img_battery.c"
         "assets/img_grid.c"
         "assets/img_home.c"
         "assets/img_inverter.c"
         "assets/img_temp.c"
         "assets/font_bold_10.c"
         "assets/font_bold_12.c"
         "assets/font_bold_14.c"
         "assets/font_bold_16.c"
         "assets/font_bold_20.c"
    INCLUDE_DIRS "." "assets"
    REQUIRES nvs_flash esp_timer esp_driver_uart esp_driver_gpio esp_driver_spi esp_lcd lvgl spi_flash
)
