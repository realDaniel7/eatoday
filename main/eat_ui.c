// main/eat_ui.c —— 主界面与添加页的 LVGL 实现（深色高对比，全部自绘）。
//
// 布局（240×320 竖屏）：
//   主界面  标题行 / 中央大字号卡片 / 统计行 / 范围行 / 按键提示
//   添加页  顶部提问 / 中间已输入文字 / 底部 3 行候选词
#include "eat_ui.h"
#include "eat_what_text.h"

#include "app_fonts.h"

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
#define EAT_COLOR_ROW_SEL   0x212D3C

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

    ui->title_label = make_label(ui->main_scr, &app_font_16, EAT_COLOR_MUTED);
    lv_obj_set_width(ui->title_label, 156);
    lv_obj_set_pos(ui->title_label, 14, 12);
    lv_obj_set_style_text_align(ui->title_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(ui->title_label, EAT_TEXT_TITLE);

    ui->battery_label = make_label(ui->main_scr, &app_font_16, EAT_COLOR_DIM);
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

    ui->state_label = make_label(ui->name_card, &app_font_16, EAT_COLOR_ACCENT);
    lv_obj_set_width(ui->state_label, 172);
    lv_obj_align(ui->state_label, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_label_set_text(ui->state_label, "");

    ui->stats_label = make_label(ui->main_scr, &app_font_16, EAT_COLOR_MUTED);
    lv_obj_set_width(ui->stats_label, 208);
    lv_obj_set_pos(ui->stats_label, 16, 224);
    lv_label_set_text(ui->stats_label, "");

    ui->scope_label = make_label(ui->main_scr, &app_font_16, EAT_COLOR_ACCENT);
    lv_obj_set_width(ui->scope_label, 208);
    lv_obj_set_pos(ui->scope_label, 16, 248);
    lv_label_set_text(ui->scope_label, "");

    ui->hint_label = make_label(ui->main_scr, &app_font_16, EAT_COLOR_DIM);
    lv_obj_set_width(ui->hint_label, 212);
    lv_obj_set_pos(ui->hint_label, 14, 276);
    lv_label_set_text(ui->hint_label, EAT_TEXT_HINT_MAIN);
}

static void build_add(eat_ui_t *ui) {
    ui->add_scr = make_screen();

    ui->add_prompt = make_label(ui->add_scr, &app_font_20, EAT_COLOR_TEXT);
    lv_obj_set_width(ui->add_prompt, 212);
    lv_obj_set_pos(ui->add_prompt, 14, 14);
    lv_label_set_text(ui->add_prompt, EAT_TEXT_ADD_PROMPT);

    ui->add_typed = make_label(ui->add_scr, &app_font_32, EAT_COLOR_ACCENT);
    lv_obj_set_width(ui->add_typed, 212);
    lv_obj_set_pos(ui->add_typed, 14, 52);
    lv_label_set_long_mode(ui->add_typed, LV_LABEL_LONG_WRAP);
    lv_label_set_text(ui->add_typed, "");

    ui->add_hint = make_label(ui->add_scr, &app_font_16, EAT_COLOR_MUTED);
    lv_obj_set_width(ui->add_hint, 212);
    lv_obj_set_pos(ui->add_hint, 14, 116);
    lv_label_set_text(ui->add_hint, "");

    for (int i = 0; i < EAT_ADD_ROWS; i++) {
        lv_obj_t *row = make_box(ui->add_scr, 20, 150 + i * 48, 200, 44, EAT_COLOR_CARD, 10);
        lv_obj_t *marker = make_box(row, 0, 12, 4, 20, EAT_COLOR_ACCENT, 2);
        lv_obj_add_flag(marker, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *text = make_label(row, &app_font_20, EAT_COLOR_MUTED);
        lv_obj_set_width(text, 176);
        lv_obj_align(text, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(text, "");
        ui->add_rows[i] = row;
        ui->add_markers[i] = marker;
    }

    ui->toast = make_box(ui->add_scr, 40, 88, 160, 0, EAT_COLOR_BG, 0);
    lv_obj_delete(ui->toast);
    ui->toast = NULL;
    ui->toast_label = NULL;
}

static void build_toast_overlay(eat_ui_t *ui) {
    // toast 挂在主界面之上，保存成功后 1 秒自动消失（时长由 main.c 控制）。
    ui->toast = make_box(ui->main_scr, 44, 148, 152, 44, EAT_COLOR_ACCENT, 22);
    ui->toast_label = make_label(ui->toast, &app_font_16, EAT_COLOR_BG);
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
    build_add(ui);
    eat_ui_show_main(ui);
}

void eat_ui_show_main(eat_ui_t *ui) {
    if (ui == NULL || ui->main_scr == NULL) return;
    lv_screen_load(ui->main_scr);
}

void eat_ui_show_add(eat_ui_t *ui) {
    if (ui == NULL || ui->add_scr == NULL) return;
    lv_screen_load(ui->add_scr);
}

size_t eat_ui_window_start(size_t cursor, size_t count) {
    if (count <= EAT_ADD_ROWS) return 0;
    if (cursor <= 1) return 0;
    size_t start = cursor - 1;  // 光标尽量停在中间行
    if (start + EAT_ADD_ROWS > count) start = count - EAT_ADD_ROWS;
    return start;
}

void eat_ui_update_main(eat_ui_t *ui, const eat_main_view_t *view) {
    if (ui == NULL || view == NULL || ui->name_label == NULL) return;

    const size_t chars = eat_utf8_chars(view->name) + (view->is_user ? 1u : 0u);
    lv_obj_set_style_text_font(ui->name_label, eat_font_for_name(chars), 0);
    if (view->is_user) {
        lv_label_set_text_fmt(ui->name_label, "★ %s", view->name == NULL ? "" : view->name);
    } else {
        lv_label_set_text(ui->name_label, view->name == NULL ? "" : view->name);
    }

    lv_obj_set_style_border_color(ui->name_card,
                                  lv_color_hex(view->spinning ? EAT_COLOR_ACCENT : EAT_COLOR_LINE),
                                  0);
    lv_obj_set_style_bg_color(ui->name_card,
                              lv_color_hex(view->spinning ? EAT_COLOR_CARD_HOT : EAT_COLOR_CARD), 0);

    const char *state = "";
    if (view->spinning) {
        state = EAT_TEXT_SPINNING;
    } else if (view->accepted) {
        state = EAT_TEXT_ACCEPTED;
    }
    lv_label_set_text(ui->state_label, state);

    lv_label_set_text_fmt(ui->stats_label, EAT_TEXT_STATS_FMT, (unsigned)view->seed_count,
                          (unsigned)view->user_count);
    lv_label_set_text(ui->scope_label,
                      view->scope == EAT_SCOPE_SEED_ONLY ? EAT_TEXT_SCOPE_SEED_ONLY
                                                        : EAT_TEXT_SCOPE_ALL);

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

void eat_ui_update_add(eat_ui_t *ui, const char *typed, const char *hint,
                       const char *const *words, size_t count, size_t cursor) {
    if (ui == NULL || ui->add_typed == NULL) return;

    const bool has_text = typed != NULL && typed[0] != '\0';
    if (has_text) {
        lv_obj_set_style_text_font(ui->add_typed,
                                   eat_utf8_chars(typed) <= 4 ? &app_font_32 : &app_font_24, 0);
        lv_label_set_text(ui->add_typed, typed);
    } else {
        lv_obj_set_style_text_font(ui->add_typed, &app_font_20, 0);
        lv_obj_set_style_text_color(ui->add_typed, lv_color_hex(EAT_COLOR_DIM), 0);
        lv_label_set_text(ui->add_typed, EAT_TEXT_ADD_EMPTY);
    }
    if (has_text) lv_obj_set_style_text_color(ui->add_typed, lv_color_hex(EAT_COLOR_ACCENT), 0);

    lv_label_set_text(ui->add_hint, hint == NULL ? "" : hint);

    const size_t start = eat_ui_window_start(cursor, count);
    for (int i = 0; i < EAT_ADD_ROWS; i++) {
        const size_t index = start + (size_t)i;
        const bool visible = index < count && words != NULL;
        lv_obj_t *row = ui->add_rows[i];
        if (!visible) {
            lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lv_obj_get_child(row, 1), "");
            continue;
        }
        lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
        const bool selected = index == cursor;
        lv_obj_set_style_bg_color(row,
                                  lv_color_hex(selected ? EAT_COLOR_ROW_SEL : EAT_COLOR_CARD), 0);
        if (selected) {
            lv_obj_clear_flag(ui->add_markers[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui->add_markers[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_t *text = lv_obj_get_child(row, 1);
        lv_obj_set_style_text_font(text, selected ? &app_font_20 : &app_font_20, 0);
        lv_obj_set_style_text_color(text,
                                    lv_color_hex(selected ? EAT_COLOR_TEXT : EAT_COLOR_MUTED), 0);
        lv_label_set_text(text, words[index]);
    }
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
