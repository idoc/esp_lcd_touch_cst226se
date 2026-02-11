# ESP LCD Touch CST226SE Controller

[![Component Registry](https://components.espressif.com/components/espressif/esp_lcd_touch_cst226se/badge.svg)](https://components.espressif.com/components/idoc/esp_lcd_touch_cst226se)

A driver for the CST226SE touch controller, implementing the esp_lcd_touch interface.

This is just a thin driver around Lewis He's SensorLib. If it works for you, he deserves all the credit.

| Touch controller | Communication interface |     Component name     |                                                                                           Link to datasheet                                                                                           |
|:----------------:|:-----------------------:|:----------------------:|:-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|
|     CST226SE     |           I2C           | esp_lcd_touch_cst226se | [PDF (Chinese)](https://github.com/lewisxhe/SensorLib/blob/master/datasheet/%E6%B5%B7%E6%A0%8E%E5%88%9B%E8%A7%A6%E6%91%B8%E8%8A%AF%E7%89%87%E7%A7%BB%E6%A4%8D%E6%89%8B%E5%86%8C-v3.5-20220701(1).pdf) |

## Add to project

Packages from this repository are uploaded to [Espressif's component service](https://components.espressif.com/).
You can add them to your project via `idf.py add-dependancy`, e.g.
```
    idf.py add-dependency "idoc/esp_lcd_touch_cst226se^1.0.0"
```

Alternatively, you can create `idf_component.yml`. More is in [Espressif's documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

## Example use

I2C initialization of the touch component.

> [!NOTE]
Unlike (some) other esp_lcd_touch drivers, users of this driver skip the step of creating an
`esp_lcd_panel_io_i2c_config_t` or `esp_lcd_panel_io_handle_t`.
The I2C bus is passed directly to `esp_lcd_touch_new_i2c_cst226se`, which passes it along to SensorLib.  

``` c
    // Initialize the I2C bus
    const i2c_master_bus_config_t i2c_config = {
        // ...
    };
    
    i2c_master_bus_handle_t i2c_handle = NULL;
    i2c_new_master_bus(&i2c_config, &i2c_handle)

    // Initialize touch
    const esp_lcd_touch_config_t tp_config = {
        .x_max = 600,
        .y_max = 450,
        .rst_gpio_num = 17,
        .int_gpio_num = -1,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
        },
    };

    esp_lcd_touch_handle_t tp;
    esp_lcd_touch_new_i2c_cst226se(i2c_handle, &tp_config, &tp);
```

Read data from the touch controller and store it in memory.

This can be done regularly in timer loop, or you might be able to get the IRQ pin to work (I couldn't on my device, a LilyGo T4 S3. YMMV).

``` c
    esp_lcd_touch_read_data(tp);
```

Once data is read from the controller, you can get to it from your code:

``` c
    esp_lcd_touch_point_data_t touch_point_data[1];
    uint8_t touch_cnt = 0;

    ESP_ERROR_CHECK(esp_lcd_touch_get_data(tp, touch_point_data, &touch_cnt, 1));
```

## Multi-touch support

This driver includes multitouch support. It can report up to 5 touch points.
