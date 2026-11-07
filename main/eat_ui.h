// main/eat_ui.h —— “今天吃什么”的两屏界面（全部从零绘制，不复用 demo 测试菜单外壳）。
//
//   主界面：标题 / 中央大字号卡片（待机时是启动画面）/ 统计行 / 范围行 / 按键提示
//   后台页：手机设置后台的热点名、密码、地址与提示
//
// 机器上不再提供选词添加页：长按确定直接开热点，用手机编辑列表。
// 所有函数都必须在 LVGL 锁内调用（应用任务持有 bsp_lvgl_lock()）。
#pragma once

#include "eat_what_pool.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

typedef struct {
    lv_obj_t *main_scr;
    lv_obj_t *title_label;
    lv_obj_t *battery_label;
    lv_obj_t *name_card;
    lv_obj_t *name_label;
    lv_obj_t *state_label;
    lv_obj_t *stats_label;
    lv_obj_t *range_label;
    lv_obj_t *hint_label;

    lv_obj_t *web_scr;
    lv_obj_t *web_title;
    lv_obj_t *web_lines[4];
    lv_obj_t *web_status;
    lv_obj_t *web_hint;

    lv_obj_t *divider;          // 标题下的细分割线

    lv_obj_t *toast;
    lv_obj_t *toast_label;
} eat_ui_t;

typedef struct {
    bool welcome;               // 开机后还没开始转：显示启动画面而不是某一家店
    const char *name;           // 当前选项（不含 ★ 前缀）
    bool is_user;               // 自定义条目：显示 ★，并用运行时字体
    bool spinning;
    bool accepted;              // 刚按过确定，短暂高亮
    size_t seed_count;          // 默认池条目数
    size_t set_count;           // 当前列表条目数
    const char *range_name;     // 当前列表名字；NULL 表示只用默认池
    bool any_set_filled;        // 是否已经建过任何自定义列表
    int battery_percent;        // -1 表示不可用
} eat_main_view_t;

typedef struct {
    const char *ssid;
    const char *password;
    const char *url;
    bool ok;                    // 热点是否启动成功
    const char *status;         // 额外状态（启动失败原因 / 已保存提示）
} eat_web_view_t;

void eat_ui_build(eat_ui_t *ui);
void eat_ui_show_main(eat_ui_t *ui);
void eat_ui_show_web(eat_ui_t *ui);

void eat_ui_update_main(eat_ui_t *ui, const eat_main_view_t *view);
void eat_ui_update_web(eat_ui_t *ui, const eat_web_view_t *view);

void eat_ui_show_toast(eat_ui_t *ui, const char *text);
void eat_ui_hide_toast(eat_ui_t *ui);
