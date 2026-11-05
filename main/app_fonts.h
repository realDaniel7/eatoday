// main/app_fonts.h —— 本应用自带的中文子集字体（assets/fonts/ 由 tools/gen_eat_what_fonts.sh 生成）。
//
// 为什么不用 LVGL 内置的 Source Han Sans SC 16 CJK：那份子集只覆盖本应用所需汉字的
// 一部分（实测 118 个字符里缺 63 个），而且没有本界面需要的大字号。
// 这里的子集按 main/ 目录里实际出现的每个非 ASCII 字符生成，
// tests/test_eat_what_font_coverage.py 会逐字校验覆盖情况。
#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(app_font_16);
LV_FONT_DECLARE(app_font_20);
LV_FONT_DECLARE(app_font_24);
LV_FONT_DECLARE(app_font_32);
LV_FONT_DECLARE(app_font_44);

// 主界面中央大字号按字符数选档，保证最长条目也能完整放下（超长时由换行兜底）。
const lv_font_t *eat_font_for_name(size_t chars);
