// main/eat_ui.h —— “今天吃什么”的两屏界面（全部从零绘制，不复用 demo 测试菜单外壳）。
//
// 所有函数都必须在 LVGL 锁内调用（应用任务持有 bsp_lvgl_lock()）。
// 界面只负责显示，事件由 eat_what_router 决定，main.c 执行。
#pragma once

#include "eat_what_pool.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

#define EAT_ADD_ROWS 3  // 添加页底部固定显示 3 行候选词

typedef struct {
    lv_obj_t *main_scr;
    lv_obj_t *title_label;
    lv_obj_t *battery_label;
    lv_obj_t *name_card;
    lv_obj_t *name_label;
    lv_obj_t *state_label;
    lv_obj_t *stats_label;
    lv_obj_t *scope_label;
    lv_obj_t *hint_label;

    lv_obj_t *add_scr;
    lv_obj_t *add_prompt;
    lv_obj_t *add_typed;
    lv_obj_t *add_hint;
    lv_obj_t *add_rows[EAT_ADD_ROWS];
    lv_obj_t *add_markers[EAT_ADD_ROWS];

    lv_obj_t *toast;
    lv_obj_t *toast_label;
} eat_ui_t;

typedef struct {
    const char *name;       // 当前选项（不含 ★ 前缀）
    bool is_user;           // 我的条目：显示 ★
    bool spinning;
    bool accepted;          // 刚按过确定，短暂高亮
    size_t seed_count;
    size_t user_count;
    eat_scope_t scope;
    int battery_percent;    // -1 表示不可用
} eat_main_view_t;

void eat_ui_build(eat_ui_t *ui);
void eat_ui_show_main(eat_ui_t *ui);
void eat_ui_show_add(eat_ui_t *ui);

void eat_ui_update_main(eat_ui_t *ui, const eat_main_view_t *view);

// typed：已输入文字（可能为空）；hint：中间下方的提示行；
// words/count/cursor：候选词表与当前高亮位置（窗口会自动滚动以保持 3 行可见）。
void eat_ui_update_add(eat_ui_t *ui, const char *typed, const char *hint,
                       const char *const *words, size_t count, size_t cursor);

void eat_ui_show_toast(eat_ui_t *ui, const char *text);
void eat_ui_hide_toast(eat_ui_t *ui);

// 候选窗口起点：让 cursor 始终落在 3 行窗口内（纯逻辑，便于主机测试）。
size_t eat_ui_window_start(size_t cursor, size_t count);
