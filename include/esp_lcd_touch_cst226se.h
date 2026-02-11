#pragma once
#include <esp_err.h>
#include "esp_lcd_touch.h"
#include <driver/i2c_types.h>

#ifdef __cplusplus
extern "C" {
#endif
esp_err_t esp_lcd_touch_new_i2c_cst226se(i2c_master_bus_handle_t bus, const esp_lcd_touch_config_t* config,
                                         esp_lcd_touch_handle_t* touch_handle);
#ifdef __cplusplus
}

#include "TouchDrvCST226.h"
TouchDrvCST226* cst226se_get_inner_driver(const esp_lcd_touch_t* tp);
#endif
