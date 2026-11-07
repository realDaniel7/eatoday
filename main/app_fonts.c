// main/app_fonts.c —— 位图字号选档 + 自定义条目的运行时 TTF 字体。
#include "app_fonts.h"

#include "esp_log.h"

// 由 CMake 的 target_add_binary_data 嵌入（assets/fonts/noto-sc-gb2312.ttf）。
extern const uint8_t noto_sc_ttf_start[] asm("_binary_noto_sc_gb2312_ttf_start");
extern const uint8_t noto_sc_ttf_end[] asm("_binary_noto_sc_gb2312_ttf_end");

static const char *TAG = "eat_fonts";

static lv_font_t *s_user;
static lv_font_t *s_user_spin;
static lv_font_t s_ui16;
static lv_font_t *s_runtime_16;
static bool s_runtime_failed;
static bool s_ui16_ready;
static bool s_ui16_failed;

const lv_font_t *eat_font_for_name(size_t chars) {
    // 可用宽度约 200px：44px 放 4 字，32px 放 6 字，再长交给 24px 并允许换行。
    if (chars <= 4) return &app_font_44;
    if (chars <= 6) return &app_font_32;
    return &app_font_24;
}

#if LV_USE_TINY_TTF
static lv_font_t *create_runtime_font(int32_t size) {
    const size_t bytes = (size_t)(noto_sc_ttf_end - noto_sc_ttf_start);
    if (bytes < 4096) {
        ESP_LOGE(TAG, "内嵌 TTF 数据异常（%u 字节）", (unsigned)bytes);
        s_runtime_failed = true;
        return NULL;
    }
    lv_font_t *font = lv_tiny_ttf_create_data(noto_sc_ttf_start, bytes, size);
    if (font == NULL) {
        ESP_LOGE(TAG, "运行时字体创建失败（%d px）", (int)size);
        s_runtime_failed = true;
    } else {
        ESP_LOGI(TAG, "运行时字体就绪（%d px，%u 字节 TTF）", (int)size, (unsigned)bytes);
    }
    return font;
}
#endif

const lv_font_t *eat_font_ui_16(void) {
#if LV_USE_TINY_TTF
    if (!s_ui16_ready && !s_ui16_failed) {
        if (s_runtime_16 == NULL && !s_runtime_failed) s_runtime_16 = create_runtime_font(16);
        if (s_runtime_16 != NULL) {
            // 位图字体优先，缺字时由运行时字体补上（字号一致，行高不跳）。
            s_ui16 = app_font_16;
            s_ui16.fallback = s_runtime_16;
            s_ui16_ready = true;
        } else {
            s_ui16_failed = true;
        }
    }
    if (s_ui16_ready) return &s_ui16;
#endif
    return &app_font_16;
}

const lv_font_t *eat_font_user(void) {
#if LV_USE_TINY_TTF
    if (s_user == NULL && !s_runtime_failed) s_user = create_runtime_font(32);
    return s_user;
#else
    return NULL;
#endif
}

const lv_font_t *eat_font_user_spin(void) {
#if LV_USE_TINY_TTF
    if (s_user_spin == NULL && !s_runtime_failed) s_user_spin = create_runtime_font(24);
    return s_user_spin;
#else
    return NULL;
#endif
}

bool eat_fonts_runtime_ready(void) {
#if LV_USE_TINY_TTF
    return eat_font_user() != NULL;
#else
    return false;
#endif
}

void eat_fonts_deinit(void) {
#if LV_USE_TINY_TTF
    if (s_runtime_16 != NULL) {
        lv_tiny_ttf_destroy(s_runtime_16);
        s_runtime_16 = NULL;
        s_ui16_ready = false;
    }
    if (s_user != NULL) {
        lv_tiny_ttf_destroy(s_user);
        s_user = NULL;
    }
    if (s_user_spin != NULL) {
        lv_tiny_ttf_destroy(s_user_spin);
        s_user_spin = NULL;
    }
#endif
}
