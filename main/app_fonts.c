// main/app_fonts.c —— 字号档位选择（纯逻辑，便于主机测试）。
#include "app_fonts.h"

const lv_font_t *eat_font_for_name(size_t chars) {
    // 可用宽度约 200px：44px 放 4 字，32px 放 6 字，再长交给 24px 并允许换行。
    if (chars <= 4) return &app_font_44;
    if (chars <= 6) return &app_font_32;
    return &app_font_24;
}
