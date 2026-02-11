#include "esp_lcd_touch_cst226se.h"

#include "TouchDrvCST226.h"
#include <vector>
#include <ranges>

constexpr auto* TAG = "touch.cst226se";

class ExtendedTouchDrvCST226 : public TouchDrvCST226 {
public:
    using TouchDrvCST226::TouchDrvCST226;
    const TouchData& get_report() { return report; }
};

// I'm doing two unusual things here:
// 1. Including an esp_lcd_touch_t base member (and using __containerof) instead of subclassing it.
// (and using a static_cast).
// I'm doing this to keep with the common pattern in other esp_lcd drivers
// and maintain C interop.
//
// 2. Having a TouchDrvCST226* member that needs to be explicitly created and destroyed,
// instead of a plain non-pointer TouchDrvCST226 member.
// This is to avoid warnings and possibly undefined behavior
// when I invoke the __containerof magic working.
struct CST226SE {
    esp_lcd_touch_t base{};
    ExtendedTouchDrvCST226* touch = new ExtendedTouchDrvCST226();

    ~CST226SE() {
        delete touch;
    }

    CST226SE() = default;
    CST226SE(const CST226SE& other) = delete;
    CST226SE(CST226SE&& other) noexcept = delete;
    CST226SE& operator=(const CST226SE& other) = delete;
    CST226SE& operator=(CST226SE&& other) noexcept = delete;
};

class ScopedCriticalSection {
public:
    explicit ScopedCriticalSection(portMUX_TYPE* mux) : lock(mux) {
        portENTER_CRITICAL(lock);
    }

    ~ScopedCriticalSection() {
        portEXIT_CRITICAL(lock);
    }

    portMUX_TYPE* lock;

    ScopedCriticalSection(const ScopedCriticalSection& other) = delete;
    ScopedCriticalSection(ScopedCriticalSection&& other) noexcept = delete;
    ScopedCriticalSection& operator=(const ScopedCriticalSection& other) = delete;
    ScopedCriticalSection& operator=(ScopedCriticalSection&& other) noexcept = delete;
};

namespace {
    CST226SE* touch_cast(const esp_lcd_touch_t* tp) {
        return __containerof(tp, CST226SE, base);
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    esp_err_t enter_sleep(esp_lcd_touch_t* tp) {
        const auto cst226 = touch_cast(tp);

        cst226->touch->sleep();
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    esp_err_t exit_sleep(esp_lcd_touch_t* tp) {
        const auto cst226 = touch_cast(tp);

        cst226->touch->wakeup();
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    esp_err_t read_data(esp_lcd_touch_t* tp) {
        const auto cst226 = touch_cast(tp);

        const uint8_t max_point_num = cst226->touch->getSupportTouchPoint();
        std::vector<int16_t> x(max_point_num), y(max_point_num);

        const uint8_t num_points = cst226->touch->getPoint(x.data(), y.data(), x.size());

        ScopedCriticalSection const lock(&tp->data.lock);

        tp->data.points = std::min(num_points, static_cast<uint8_t>(CONFIG_ESP_LCD_TOUCH_MAX_POINTS));

        const auto report = cst226->touch->get_report();

        for (auto&& [r_x, r_y, r_pressure, coords] : std::ranges::views::zip(
                 std::span(report.x).first(tp->data.points),
                 std::span(report.y).first(tp->data.points),
                 std::span(report.pressure).first(tp->data.points),
                 std::span(tp->data.coords).first(tp->data.points))) {
            coords.x = r_x;
            coords.y = r_y;
            coords.strength = r_pressure;
        }

        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    esp_err_t del(esp_lcd_touch_t* tp) {
        std::unique_ptr<CST226SE> const owner(touch_cast(tp));
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    bool get_xy(esp_lcd_touch_t* tp, uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* point_num,
                uint8_t max_point_num) {
        ScopedCriticalSection const lock(&tp->data.lock);

        *point_num = std::min(tp->data.points, max_point_num);

        for (auto&& [out_x, out_y, out_strength, in_coords] : std::ranges::views::zip(
                 std::span(x, *point_num),
                 std::span(y, *point_num),
                 std::span(strength, *point_num),
                 std::span(tp->data.coords).first(*point_num))) {
            out_x = in_coords.x;
            out_y = in_coords.y;
            out_strength = in_coords.strength;
        }

        return *point_num > 0;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    esp_err_t set_swap_xy(esp_lcd_touch_t* tp, bool swap) {
        const auto cst226 = touch_cast(tp);
        cst226->base.config.flags.swap_xy = swap;

        cst226->touch->setSwapXY(swap);
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void update_mirroring(esp_lcd_touch_t* tp) {
        const auto cst226 = touch_cast(tp);
        cst226->touch->setMirrorXY(cst226->base.config.flags.mirror_x,
                                   cst226->base.config.flags.mirror_y);
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    esp_err_t set_mirror_y(esp_lcd_touch_t* tp, bool mirror) {
        const auto cst226 = touch_cast(tp);

        cst226->base.config.flags.mirror_y = mirror;

        update_mirroring(tp);
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    esp_err_t set_mirror_x(esp_lcd_touch_t* tp, bool mirror) {
        const auto cst226 = touch_cast(tp);

        cst226->base.config.flags.mirror_y = mirror;

        update_mirroring(tp);
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    esp_err_t get_swap_xy(esp_lcd_touch_t* tp, bool* swap) {
        const auto cst226 = touch_cast(tp);

        *swap = cst226->base.config.flags.swap_xy;
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    esp_err_t get_mirror_y(esp_lcd_touch_t* tp, bool* mirror) {
        const auto cst226 = touch_cast(tp);

        *mirror = cst226->base.config.flags.mirror_y;
        return ESP_OK;
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    // ReSharper disable once CppParameterMayBeConst
    esp_err_t get_mirror_x(esp_lcd_touch_t* tp, bool* mirror) {
        const auto cst226 = touch_cast(tp);

        *mirror = cst226->base.config.flags.mirror_x;
        return ESP_OK;
    }
}

esp_err_t esp_lcd_touch_new_i2c_cst226se(const i2c_master_bus_handle_t bus, const esp_lcd_touch_config_t* config, // NOLINT(*-misplaced-const)
                                         esp_lcd_touch_handle_t* touch_handle) {
    auto cst226 = std::make_unique<CST226SE>();
    cst226->base = {
        .enter_sleep = enter_sleep,
        .exit_sleep = exit_sleep,
        .read_data = read_data,
        .get_xy = get_xy,
        .get_track_id = nullptr,
        .set_swap_xy = set_swap_xy,
        .get_swap_xy = get_swap_xy,
        .set_mirror_x = set_mirror_x,
        .get_mirror_x = get_mirror_x,
        .set_mirror_y = set_mirror_y,
        .get_mirror_y = get_mirror_y,
        .del = del,
    };

    cst226->base.data.lock.owner = portMUX_FREE_VAL;

    memcpy(&cst226->base.config, config, sizeof(esp_lcd_touch_config_t));

    if (!cst226->touch->begin(bus, CST226SE_SLAVE_ADDRESS)) {
        ESP_LOGE(TAG, "Touch init failed!"); // NOLINT
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Touch driver initialized"); // NOLINT

    cst226->touch->setMaxCoordinates(config->x_max, config->y_max);
    cst226->touch->setSwapXY(config->flags.swap_xy);
    update_mirroring(&cst226->base);

    if (cst226->base.config.interrupt_callback) {
        esp_lcd_touch_register_interrupt_callback(&cst226->base, cst226->base.config.interrupt_callback);
    }

    // The caller now owns cst226. Release it.
    *touch_handle = &cst226.release()->base;
    return ESP_OK;
}

TouchDrvCST226* cst226se_get_inner_driver(const esp_lcd_touch_t* tp) {
    const auto cst226 = touch_cast(tp);

    return cst226->touch;
}
