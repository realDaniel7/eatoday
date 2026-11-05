// main/main.c —— “今天吃什么”应用入口。
//
// 结构：按键回调只把事件塞进队列；唯一的应用任务串行地做四件事
//   1) 走 App Router 决定动作      2) 执行动作（起转/减速/换池/保存）
//   3) 推进轮盘                    4) 持 LVGL 锁刷新界面
// 轮盘与路由都不在多任务间共享，因此不需要额外互斥；界面访问一律经 bsp_lvgl_lock()。
#include "app_fonts.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "eat_ui.h"
#include "eat_what_pool.h"
#include "eat_what_router.h"
#include "eat_what_seed.h"
#include "eat_what_store.h"
#include "eat_what_text.h"
#include "eat_what_wheel.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <string.h>

static const char *TAG = "eat_what";

#define TICK_MS 20                    // 轮盘推进与按键响应的基本节拍
#define ACCEPT_HOLD_US (900 * 1000)   // “就它了”提示保持时间
#define TOAST_HOLD_US (1000 * 1000)   // “已加入轮盘 ★”1 秒后消失
#define BATTERY_PERIOD_US (30 * 1000 * 1000)
#define INPUT_QUEUE_DEPTH 8
#define EAT_USER_NAME_MAX_CHARS 6     // 添加页最多 6 字
#define EAT_APP_TASK_STACK 5120

// 添加页候选词表（内容在 eat_what_text.c，字体子集按同一张表生成）。
static eat_candidates_t s_candidates;

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
} input_event_t;

static QueueHandle_t s_queue;
static eat_ui_t s_ui;
static eat_router_t s_router;
static eat_wheel_t s_wheel;
static eat_name_list_t s_seed_list;
static eat_name_list_t s_user_list;
static const char *s_seed_ptrs[EAT_USER_MAX];
static eat_wheel_view_t s_view;
static eat_scope_t s_scope = EAT_SCOPE_ALL;
static eat_page_t s_page = EAT_PAGE_MAIN;

static int64_t s_accept_until_us;
static int64_t s_toast_until_us;
static int64_t s_battery_next_us;
static int s_battery_percent = -1;

static bool s_dirty_main;
static bool s_dirty_add;
static bool s_toast_expiring;

static eat_key_t map_key(bsp_btn_t btn) {
    switch (btn) {
        case BSP_BTN_UP: return EAT_KEY_UP;
        case BSP_BTN_DOWN: return EAT_KEY_DOWN;
        default: return EAT_KEY_OK;
    }
}

static eat_key_ev_t map_event(bsp_btn_ev_t ev) {
    switch (ev) {
        case BSP_BTN_PRESS: return EAT_KEY_PRESS;
        case BSP_BTN_LONG: return EAT_KEY_LONG;
        case BSP_BTN_RELEASE: return EAT_KEY_RELEASE;
        default: return EAT_KEY_CLICK;
    }
}

static void rebuild_view(void) {
    s_view.seed_names = s_seed_ptrs;
    s_view.seed_count = s_seed_list.count;
    s_view.user = &s_user_list;
    s_view.scope = s_scope;
    eat_wheel_set_count(&s_wheel, (uint16_t)eat_wheel_view_count(&s_view));
}

static void render_main_locked(void) {
    const uint16_t index = eat_wheel_index(&s_wheel);
    const eat_main_view_t view = {
        .name = eat_wheel_view_name(&s_view, index),
        .is_user = eat_wheel_view_is_user(&s_view, index),
        .spinning = eat_wheel_is_spinning(&s_wheel),
        .accepted = s_accept_until_us != 0,
        .seed_count = s_view.seed_count,
        .user_count = s_user_list.count,
        .scope = s_scope,
        .battery_percent = s_battery_percent,
    };
    eat_ui_update_main(&s_ui, &view);
}

static void render_add_locked(void) {
    char typed[EAT_NAME_MAX_BYTES];
    const char *category = eat_router_selected_word(&s_router);
    const char *suffix = eat_router_suffix(&s_router);
    eat_compose_name(typed, sizeof(typed), category, suffix);

    const char *hint;
    if (category == NULL) {
        hint = EAT_TEXT_ADD_HINT_CHOOSE;
    } else if (suffix != NULL) {
        hint = EAT_TEXT_ADD_HINT_SUFFIXED;
    } else {
        hint = EAT_TEXT_ADD_HINT_SAVE;
    }
    eat_ui_update_add(&s_ui, category == NULL ? "" : typed, hint, s_candidates.words,
                      s_candidates.count, s_router.cursor);
}

static void toggle_scope(void) {
    s_scope = (s_scope == EAT_SCOPE_ALL) ? EAT_SCOPE_SEED_ONLY : EAT_SCOPE_ALL;
    (void)eat_store_save_scope(s_scope);
    rebuild_view();
    ESP_LOGI(TAG, "轮盘范围：%s（%u 项）", s_scope == EAT_SCOPE_ALL ? "全部" : "仅种子池",
             (unsigned)eat_wheel_view_count(&s_view));
    s_dirty_main = true;
}

static void save_user_entry(void) {
    char composed[EAT_NAME_MAX_BYTES];
    char trimmed[EAT_NAME_MAX_BYTES];
    const char *category = eat_router_selected_word(&s_router);
    const char *suffix = eat_router_suffix(&s_router);
    const size_t length = eat_compose_name(composed, sizeof(composed), category, suffix);
    eat_utf8_truncate(trimmed, sizeof(trimmed), composed, EAT_USER_NAME_MAX_CHARS);

    bool added = false;
    if (length > 0 && trimmed[0] != '\0') {
        added = eat_name_list_append(&s_user_list, trimmed);
        if (added) (void)eat_store_save_user(&s_user_list);
    }
    ESP_LOGI(TAG, "%s：%s（我的 %u 项）", added ? "已加入轮盘" : "未加入（重复或已满）", trimmed,
             (unsigned)s_user_list.count);

    eat_router_leave_add(&s_router);
    s_page = EAT_PAGE_MAIN;
    rebuild_view();
    s_toast_until_us = esp_timer_get_time() + TOAST_HOLD_US;

    if (bsp_lvgl_lock(200)) {
        eat_ui_show_main(&s_ui);
        eat_ui_show_toast(&s_ui, added ? EAT_TEXT_TOAST_ADDED : EAT_TEXT_TOAST_REPEAT);
        bsp_lvgl_unlock();
    }
    s_dirty_main = true;
}

static void enter_add_page(void) {
    eat_router_enter_add(&s_router);
    s_page = EAT_PAGE_ADD;
    if (bsp_lvgl_lock(200)) {
        eat_ui_show_add(&s_ui);
        bsp_lvgl_unlock();
    }
    s_dirty_add = true;
}

static void leave_add_page(void) {
    eat_router_leave_add(&s_router);
    s_page = EAT_PAGE_MAIN;
    if (bsp_lvgl_lock(200)) {
        eat_ui_show_main(&s_ui);
        bsp_lvgl_unlock();
    }
    s_dirty_main = true;
}

static void handle_action(eat_action_t action) {
    switch (action) {
        case EAT_ACT_SPIN_START:
            eat_wheel_start(&s_wheel);
            s_dirty_main = true;
            break;
        case EAT_ACT_SPIN_RELEASE:
            eat_wheel_release(&s_wheel);
            s_dirty_main = true;
            break;
        case EAT_ACT_SPIN_STOP:
            eat_wheel_stop_now(&s_wheel);
            s_dirty_main = true;
            break;
        case EAT_ACT_SELECT_NEXT:
            eat_wheel_step(&s_wheel, 1);
            s_dirty_main = true;
            break;
        case EAT_ACT_SCOPE_TOGGLE:
            toggle_scope();
            break;
        case EAT_ACT_ACCEPT:
            s_accept_until_us = esp_timer_get_time() + ACCEPT_HOLD_US;
            s_dirty_main = true;
            break;
        case EAT_ACT_OPEN_ADD:
            enter_add_page();
            break;
        case EAT_ACT_ADD_MOVE_PREV:
        case EAT_ACT_ADD_MOVE_NEXT:
        case EAT_ACT_ADD_CHOOSE:
            s_dirty_add = true;
            break;
        case EAT_ACT_ADD_SAVE:
            save_user_entry();
            break;
        case EAT_ACT_ADD_CANCEL:
            leave_add_page();
            break;
        default:
            break;
    }
}

static void handle_input(const input_event_t *input) {
    const bool spinning = eat_wheel_is_spinning(&s_wheel);
    const eat_action_t action =
        eat_router_key(&s_router, map_key(input->btn), map_event(input->ev), spinning);
    handle_action(action);
}

static void poll_battery(void) {
    const int soc = bsp_battery_soc();
    if (soc != s_battery_percent) {
        s_battery_percent = soc;
        s_dirty_main = true;
    }
    s_battery_next_us = esp_timer_get_time() + BATTERY_PERIOD_US;
}

static void refresh_ui(void) {
    if (!s_dirty_main && !s_dirty_add && !s_toast_expiring) return;
    if (!bsp_lvgl_lock(50)) return;

    if (s_toast_expiring) {
        eat_ui_hide_toast(&s_ui);
        s_toast_expiring = false;
    }
    if (s_page == EAT_PAGE_ADD) {
        if (s_dirty_add) render_add_locked();
    } else if (s_dirty_main) {
        render_main_locked();
    }
    bsp_lvgl_unlock();

    s_dirty_main = false;
    s_dirty_add = false;
}

static void app_task(void *arg) {
    (void)arg;
    int64_t last_us = esp_timer_get_time();
    for (;;) {
        input_event_t input;
        if (xQueueReceive(s_queue, &input, pdMS_TO_TICKS(TICK_MS)) == pdTRUE) {
            handle_input(&input);
        }

        const int64_t now = esp_timer_get_time();
        const uint32_t dt_ms = (uint32_t)((now - last_us) / 1000);
        last_us = now;

        const bool was_spinning = eat_wheel_is_spinning(&s_wheel);
        if (dt_ms > 0 && eat_wheel_tick(&s_wheel, dt_ms)) s_dirty_main = true;
        if (was_spinning && !eat_wheel_is_spinning(&s_wheel)) s_dirty_main = true;

        if (s_accept_until_us != 0 && now >= s_accept_until_us) {
            s_accept_until_us = 0;
            s_dirty_main = true;
        }
        if (s_toast_until_us != 0 && now >= s_toast_until_us) {
            s_toast_until_us = 0;
            s_toast_expiring = true;
        }
        if (now >= s_battery_next_us) poll_battery();

        refresh_ui();
    }
}

// 按键回调运行在 button 组件的 esp_timer 任务里：只入队，不做任何重活。
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user) {
    (void)user;
    if (s_queue == NULL) return;
    const input_event_t input = { .btn = btn, .ev = ev };
    (void)xQueueSend(s_queue, &input, 0);
}

void app_main(void) {
    ESP_LOGI(TAG, "今天吃什么 v1 启动");

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败（MOSI=%d SCLK=%d CS=%d DC=%d BL=%d）", BSP_LCD_MOSI,
                 BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    const bool battery_ok = bsp_battery_init() == ESP_OK;
    if (!battery_ok) ESP_LOGW(TAG, "电量计不可用，右上角显示 --");

    if (eat_store_open() != 0) {
        ESP_LOGW(TAG, "NVS 不可用，本次运行使用编译期种子池且不保存");
    }
    eat_name_list_reset(&s_user_list);
    const bool seeded = eat_store_seed_if_needed(&s_seed_list);
    if (s_seed_list.count == 0) {
        eat_seed_fill_list(&s_seed_list);  // NVS 不可用时的兜底
    }
    if (!eat_store_load_user(&s_user_list)) {
        eat_name_list_reset(&s_user_list);
    }
    eat_scope_t stored_scope = EAT_SCOPE_ALL;
    if (eat_store_load_scope(&stored_scope)) s_scope = stored_scope;

    eat_name_list_pointers(&s_seed_list, s_seed_ptrs, EAT_USER_MAX);
    s_candidates.words = EAT_CANDIDATE_WORDS;
    s_candidates.count = EAT_CANDIDATE_COUNT;
    eat_router_init(&s_router, &s_candidates);
    eat_wheel_init(&s_wheel, 0);
    rebuild_view();
    eat_wheel_init(&s_wheel, (uint16_t)eat_wheel_view_count(&s_view));

    ESP_LOGI(TAG, "种子池 %u 项（本次%s写入 NVS），我的 %u 项，范围 %s",
             (unsigned)s_seed_list.count, seeded ? "" : "未", (unsigned)s_user_list.count,
             s_scope == EAT_SCOPE_ALL ? "全部" : "仅种子池");

    s_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (s_queue == NULL) {
        ESP_LOGE(TAG, "按键队列创建失败");
        return;
    }
    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败，应用只能显示不能操作");
    }

    if (bsp_lvgl_lock(1000)) {
        eat_ui_build(&s_ui);
        s_dirty_main = true;
        render_main_locked();
        bsp_lvgl_unlock();
        s_dirty_main = false;
    } else {
        ESP_LOGE(TAG, "LVGL 加锁失败，界面未创建");
    }

    if (xTaskCreate(app_task, "eat_what", EAT_APP_TASK_STACK, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "应用任务创建失败");
        return;
    }
    ESP_LOGI(TAG, "就绪");
}
