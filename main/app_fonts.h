// main/app_fonts.h —— 界面字体：固定文案用位图子集，自定义条目用运行时 TTF。
//
// 位图子集（assets/fonts/app_font_*.c）覆盖界面固定文案与默认池店名，
// 由 tools/gen_eat_what_fonts.sh 生成，覆盖情况由 python 测试逐字校验。
//
// 自定义条目是用户在手机上自由输入的，任何静态子集都可能缺字，所以用
// LVGL 的 Tiny TTF 在运行时栅格化一份常用汉字 TTF（assets/fonts/noto-sc-gb2312.ttf，
// GB2312 一级 3755 字 + ASCII）。
#pragma once

#include "lvgl.h"

#include <stdbool.h>
#include <stddef.h>

LV_FONT_DECLARE(app_font_16);
LV_FONT_DECLARE(app_font_20);
LV_FONT_DECLARE(app_font_24);
LV_FONT_DECLARE(app_font_32);
LV_FONT_DECLARE(app_font_44);

// 固定文案按字符数选档（位图字体，渲染快）。
const lv_font_t *eat_font_for_name(size_t chars);

// 16px 界面文字：位图为主，缺字时自动回落到运行时 TTF。
// 套名、列表名来自用户输入，位图子集不可能覆盖全部汉字（例如“我的1”里的“我”），
// 所以这一档必须带 fallback，否则会出现方框。
const lv_font_t *eat_font_ui_16(void);

// 自定义条目用的运行时字体。首次调用时从内嵌 TTF 创建；
// 失败返回 NULL，调用方应回退到 eat_font_for_name()。
const lv_font_t *eat_font_user_large(void);   // 短名字（≈40px）
const lv_font_t *eat_font_user_medium(void);  // 长名字（≈28px）

// 运行时字体是否可用（后台页/测试用来提示）。
bool eat_fonts_runtime_ready(void);

// 释放运行时字体（关机前或测试用）。
void eat_fonts_deinit(void);
