// main/eat_ui.c —— 主界面与手机后台页的 LVGL 实现（深色高对比，全部自绘）。
//
// 布局（240×320 竖屏）：
//   主界面  标题行 / 中央大字号卡片 / 统计行 / 范围行 / 按键提示
//   后台页  热点名 / 密码 / 打开地址 / 提示
//
// 待机态（开机后还没开始转）中央显示“今天吃什么”，不显示任何具体条目，
// 免得第一屏看起来像把某一家店当封面。
#include "eat_ui.h"

#include "app_fonts.h"
#include "eat_what_text.h"

#include <stdio.h>
#include <string.h>

#define EAT_COLOR_BG        0x0B0F14
#define EAT_COLOR_CARD      0x151C26
#define EAT_COLOR_CARD_HOT  0x1C2836
#define EAT_COLOR_LINE      0x24303E
#define EAT_COLOR_TEXT      0xF2F6FA
#define EAT_COLOR_MUTED     0x8095AA
#define EAT_COLOR_DIM       0x4E6076
#define EAT_COLOR_ACCENT    0xFFB454

static lv_obj_t *make_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(EAT_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
}

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

static lv_obj_t *make_box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color,
                          int radius) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_remove_style_all(box);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_bg_color(box, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, radius, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static void build_main(eat_ui_t *ui) {
    ui->main_scr = make_screen();

    ui->title_label = make_label(ui->main_scr, eat_font_ui_16(), EAT_COLOR_MUTED);
    lv_obj_set_width(ui->title_label, 156);
    lv_obj_set_pos(ui->title_label, 14, 12);
    lv_obj_set_style_text_align(ui->title_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(ui->title_label, EAT_TEXT_TITLE);

    ui->battery_label = make_label(ui->main_scr, eat_font_ui_16(), EAT_COLOR_DIM);
    lv_obj_set_width(ui->battery_label, 56);
    lv_obj_set_pos(ui->battery_label, 168, 12);
    lv_obj_set_style_text_align(ui->battery_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(ui->battery_label, EAT_TEXT_BATTERY_UNKNOWN);

    // 中央卡片：唯一的视觉焦点，旋转时边框点亮为强调色。
    ui->name_card = make_box(ui->main_scr, 20, 60, 200, 152, EAT_COLOR_CARD, 18);
    lv_obj_set_style_border_width(ui->name_card, 2, 0);
    lv_obj_set_style_border_color(ui->name_card, lv_color_hex(EAT_COLOR_LINE), 0);

    ui->name_label = make_label(ui->name_card, &app_font_44, EAT_COLOR_TEXT);
    lv_obj_set_width(ui->name_label, 176);
    lv_obj_set_style_text_align(ui->name_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(ui->name_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->name_label, LV_ALIGN_CENTER, 0, -12);
    lv_label_set_text(ui->name_label, "");

    ui->state_label = make_label(ui->name_card, eat_font_ui_16(), EAT_COLOR_ACCENT);
    lv_obj_set_width(ui->state_label, 172);
    lv_obj_align(ui->state_label, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_label_set_text(ui->state_label, "");

    ui->stats_label = make_label(ui->main_scr, eat_font_ui_16(), EAT_COLOR_MUTED);
    lv_obj_set_width(ui->stats_label, 208);
    lv_obj_set_pos(ui->stats_label, 16, 224);
    lv_label_set_text(ui->stats_label, "");

    ui->range_label = make_label(ui->main_scr, eat_font_ui_16(), EAT_COLOR_ACCENT);
    lv_obj_set_width(ui->range_label, 208);
    lv_obj_set_pos(ui->range_label, 16, 248);
    lv_label_set_text(ui->range_label, "");

    ui->hint_label = make_label(ui->main_scr, eat_font_ui_16(), EAT_COLOR_DIM);
    lv_obj_set_width(ui->hint_label, 212);
    lv_obj_set_pos(ui->hint_label, 14, 276);
    lv_label_set_text(ui->hint_label, EAT_TEXT_HINT_MAIN);
}

static void build_web(eat_ui_t *ui) {
    ui->web_scr = make_screen();

    ui->web_title = make_label(ui->web_scr, &app_font_20, EAT_COLOR_TEXT);
    lv_obj_set_width(ui->web_title, 212);
    lv_obj_set_pos(ui->web_title, 14, 16);
    lv_label_set_text(ui->web_title, EAT_TEXT_WEB_TITLE);

    for (int i = 0; i < 4; i++) {
        ui->web_lines[i] = make_label(ui->web_scr, eat_font_ui_16(), EAT_COLOR_MUTED);
        lv_obj_set_width(ui->web_lines[i], 212);
        lv_obj_set_pos(ui->web_lines[i], 14, 60 + i * 26);
        lv_label_set_text(ui->web_lines[i], "");
    }

    ui->web_status = make_label(ui->web_scr, eat_font_ui_16(), EAT_COLOR_ACCENT);
    lv_obj_set_width(ui->web_status, 212);
    lv_obj_set_pos(ui->web_status, 14, 180);
    lv_label_set_long_mode(ui->web_status, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ui->web_status, "");

    ui->web_hint = make_label(ui->web_scr, eat_font_ui_16(), EAT_COLOR_DIM);
    lv_obj_set_width(ui->web_hint, 212);
    lv_obj_set_pos(ui->web_hint, 14, 282);
    lv_label_set_text(ui->web_hint, EAT_TEXT_WEB_TIP);
}

static void build_toast_overlay(eat_ui_t *ui) {
    // toast 挂在主界面之上，保存成功后 1 秒自动消失（时长由 main.c 控制）。
    ui->toast = make_box(ui->main_scr, 44, 148, 152, 44, EAT_COLOR_ACCENT, 22);
    ui->toast_label = make_label(ui->toast, eat_font_ui_16(), EAT_COLOR_BG);
    lv_obj_set_width(ui->toast_label, 136);
    lv_obj_align(ui->toast_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(ui->toast_label, "");
    lv_obj_add_flag(ui->toast, LV_OBJ_FLAG_HIDDEN);
}

void eat_ui_build(eat_ui_t *ui) {
    if (ui == NULL) return;
    memset(ui, 0, sizeof(*ui));
    build_main(ui);
    build_toast_overlay(ui);
    build_web(ui);
    eat_ui_show_main(ui);
}

void eat_ui_show_main(eat_ui_t *ui) {
    if (ui == NULL || ui->main_scr == NULL) return;
    lv_screen_load(ui->main_scr);
}

void eat_ui_show_web(eat_ui_t *ui) {
    if (ui == NULL || ui->web_scr == NULL) return;
    lv_screen_load(ui->web_scr);
}

static const lv_font_t *name_font(const eat_main_view_t *view, size_t chars) {
    if (view->is_user) {
        // 自定义条目来自手机输入，用运行时 TTF，避免静态子集缺字。
        const lv_font_t *runtime = chars <= 6 ? eat_font_user_large() : eat_font_user_medium();
        if (runtime != NULL) return runtime;
    }
    return eat_font_for_name(chars);
}

void eat_ui_update_main(eat_ui_t *ui, const eat_main_view_t *view) {
    if (ui == NULL || view == NULL || ui->name_label == NULL) return;

    if (view->welcome) {
        // 启动画面：还没开始转，别把某一条默认池当成“封面”。
        lv_obj_set_style_text_font(ui->name_label, eat_font_for_name(5), 0);
        lv_label_set_text(ui->name_label, EAT_TEXT_WELCOME_TITLE);
        lv_label_set_text(ui->state_label,
                          view->any_set_filled ? EAT_TEXT_WELCOME_HINT
                                               : EAT_TEXT_WELCOME_HINT_NO_SET);
        lv_label_set_text(ui->stats_label, "");
        lv_label_set_text(ui->range_label, "");
        lv_obj_set_style_border_color(ui->name_card, lv_color_hex(EAT_COLOR_LINE), 0);
        lv_obj_set_style_bg_color(ui->name_card, lv_color_hex(EAT_COLOR_CARD), 0);
    } else {
        const size_t chars = eat_utf8_chars(view->name) + (view->is_user ? 2u : 0u);
        lv_obj_set_style_text_font(ui->name_label, name_font(view, chars), 0);
        if (view->is_user) {
            lv_label_set_text_fmt(ui->name_label, "★ %s", view->name == NULL ? "" : view->name);
        } else {
            lv_label_set_text(ui->name_label, view->name == NULL ? "" : view->name);
        }

        lv_obj_set_style_border_color(
            ui->name_card, lv_color_hex(view->spinning ? EAT_COLOR_ACCENT : EAT_COLOR_LINE), 0);
        lv_obj_set_style_bg_color(
            ui->name_card, lv_color_hex(view->spinning ? EAT_COLOR_CARD_HOT : EAT_COLOR_CARD), 0);

        const char *state = "";
        if (view->spinning) {
            state = EAT_TEXT_SPINNING;
        } else if (view->accepted) {
            state = EAT_TEXT_ACCEPTED;
        }
        lv_label_set_text(ui->state_label, state);

        lv_label_set_text_fmt(ui->stats_label, EAT_TEXT_STATS_FMT, (unsigned)view->seed_count,
                              view->range_name == NULL ? EAT_TEXT_RANGE_DEFAULT_NAME
                                                       : view->range_name,
                              (unsigned)view->set_count);

        if (view->range_name == NULL) {
            lv_label_set_text(ui->range_label, EAT_TEXT_RANGE_DEFAULT);
        } else {
            lv_label_set_text_fmt(ui->range_label, EAT_TEXT_RANGE_SET_FMT, view->range_name);
        }
    }

    if (view->battery_percent < 0) {
        lv_label_set_text(ui->battery_label, EAT_TEXT_BATTERY_UNKNOWN);
        lv_obj_set_style_text_color(ui->battery_label, lv_color_hex(EAT_COLOR_DIM), 0);
    } else {
        lv_label_set_text_fmt(ui->battery_label, EAT_TEXT_BATTERY_FMT, view->battery_percent);
        lv_obj_set_style_text_color(ui->battery_label,
                                    lv_color_hex(view->battery_percent <= 20 ? 0xE4695A
                                                                             : EAT_COLOR_DIM),
                                    0);
    }
}

void eat_ui_update_web(eat_ui_t *ui, const eat_web_view_t *view) {
    if (ui == NULL || view == NULL || ui->web_lines[0] == NULL) return;

    if (view->ok && view->ssid != NULL) {
        lv_label_set_text_fmt(ui->web_lines[0], EAT_TEXT_WEB_WIFI_FMT, view->ssid);
        lv_label_set_text_fmt(ui->web_lines[1], EAT_TEXT_WEB_PASS_FMT, view->password);
        lv_label_set_text_fmt(ui->web_lines[2], EAT_TEXT_WEB_URL_FMT, view->url);
        lv_obj_set_style_text_color(ui->web_lines[0], lv_color_hex(EAT_COLOR_TEXT), 0);
    } else {
        lv_label_set_text(ui->web_lines[0], EAT_TEXT_WEB_FAIL);
        lv_label_set_text(ui->web_lines[1], "");
        lv_label_set_text(ui->web_lines[2], "");
        lv_obj_set_style_text_color(ui->web_lines[0], lv_color_hex(0xE4695A), 0);
    }
    lv_label_set_text(ui->web_lines[3], EAT_TEXT_WEB_HELP);
    lv_label_set_text(ui->web_status, view->status == NULL ? "" : view->status);
}

void eat_ui_show_toast(eat_ui_t *ui, const char *text) {
    if (ui == NULL || ui->toast == NULL) return;
    lv_label_set_text(ui->toast_label, text == NULL ? "" : text);
    lv_obj_clear_flag(ui->toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui->toast);
}

void eat_ui_hide_toast(eat_ui_t *ui) {
    if (ui == NULL || ui->toast == NULL) return;
    lv_obj_add_flag(ui->toast, LV_OBJ_FLAG_HIDDEN);
}
