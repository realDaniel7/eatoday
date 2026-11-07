// main/main.c —— “今天吃什么”应用入口。
//
// 结构：按键回调只把事件塞进队列；唯一的应用任务串行地做四件事
//   1) 走 App Router 决定动作      2) 执行动作（起转/减速/换列表/开关热点）
//   3) 推进轮盘                    4) 持 LVGL 锁刷新界面
// 轮盘、列表与路由都不在多任务间共享；手机后台改写的是镜像副本（s_sets_mirror），
// 由本任务在安全点整体同步，因此界面与存储不会读到半截数据。
#include "app_fonts.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_pins.h"
#include "eat_captive.h"
#include "eat_sets.h"
#include "eat_ui.h"
#include "eat_web.h"
#include "eat_wifi.h"
#include "eat_what_pool.h"
#include "eat_what_router.h"
#include "eat_what_seed.h"
#include "eat_what_store.h"
#include "eat_what_text.h"
#include "eat_what_wheel.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "eat_what";

#define TICK_MS 20                    // 轮盘推进与按键响应的基本节拍
#define ACCEPT_HOLD_US (900 * 1000)   // “就它了”提示保持时间
#define TOAST_HOLD_US (1200 * 1000)   // 手机保存后的提示时长
#define BATTERY_PERIOD_US (30 * 1000 * 1000)
#define INPUT_QUEUE_DEPTH 8
#define EAT_APP_TASK_STACK 6144

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
} input_event_t;

static QueueHandle_t s_queue;
static eat_ui_t s_ui;
static eat_router_t s_router;
static eat_wheel_t s_wheel;
static eat_name_list_t s_seed_list;
static const char *s_seed_ptrs[EAT_USER_MAX];
static eat_wheel_view_t s_view;
static eat_sets_t s_sets;
static eat_sets_t s_sets_mirror;          // 手机后台改写的副本
static eat_page_t s_page = EAT_PAGE_MAIN;

static eat_wifi_info_t s_wifi_info;
static bool s_web_ok;
static volatile bool s_web_saved;
static char s_web_status[64];

static int64_t s_accept_until_us;
static int64_t s_toast_until_us;
static int64_t s_battery_next_us;
static int s_battery_percent = -1;

static bool s_started;          // 开机后是否已经开始转过（false 时显示启动画面）
static bool s_dirty_main;
static bool s_dirty_web;
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
    eat_sets_fill_view(&s_sets, s_seed_ptrs, s_seed_list.count, &s_view);
    eat_wheel_set_count(&s_wheel, (uint16_t)eat_wheel_view_count(&s_view));
}

static void render_main_locked(void) {
    const uint16_t index = eat_wheel_index(&s_wheel);
    const eat_set_t *active = eat_sets_active(&s_sets);
    const eat_main_view_t view = {
        .welcome = !s_started,
        .name = eat_wheel_view_name(&s_view, index),
        .is_user = eat_wheel_view_is_user(&s_view, index),
        .spinning = eat_wheel_is_spinning(&s_wheel),
        .accepted = s_accept_until_us != 0,
        .seed_count = s_seed_list.count,
        .set_count = active == NULL ? 0 : active->items.count,
        .range_name = active == NULL ? NULL : active->name,
        .any_set_filled = eat_sets_any_filled(&s_sets),
        .battery_percent = s_battery_percent,
    };
    eat_ui_update_main(&s_ui, &view);
}

static void render_web_locked(void) {
    const eat_web_view_t view = {
        .ssid = s_wifi_info.ssid,
        .password = s_wifi_info.password,
        .url = s_wifi_info.url,
        .ok = s_web_ok,
        .status = s_web_status,
    };
    eat_ui_update_web(&s_ui, &view);
}

static void cycle_set(void) {
    // 空列表在设备上直接跳过；从最后一站回到默认池。
    s_sets.active = eat_sets_next_visible(&s_sets, s_sets.active);
    s_sets_mirror.active = s_sets.active;
    (void)eat_store_save_meta_only(&s_sets);
    rebuild_view();
    const char *name = eat_sets_active_name(&s_sets);
    ESP_LOGI(TAG, "轮盘范围：%s（%u 项）", name == NULL ? "默认池" : name,
             (unsigned)eat_wheel_view_count(&s_view));
    s_dirty_main = true;
}

static void open_web_page(void) {
    // 字体不在这里释放：退出后台页时重建 1MB 的 TTF 会明显卡一下并闪屏。
    // 字形缓存只占十几 KB，改用系统堆后和 Wi-Fi 可以共存。
    s_web_status[0] = '\0';
    s_web_ok = eat_wifi_start(&s_wifi_info) == ESP_OK;
    if (s_web_ok) {
        s_sets_mirror = s_sets;
        if (eat_web_start(&s_sets_mirror, s_seed_list.count, NULL, NULL) != ESP_OK) {
            s_web_ok = false;
            (void)eat_wifi_stop();
        } else if (eat_captive_start() != ESP_OK) {
            ESP_LOGW(TAG, "DNS 劫持启动失败，手机需手动打开地址");
        }
    }
    if (!s_web_ok) {
        snprintf(s_web_status, sizeof(s_web_status), "%s", EAT_TEXT_WEB_FAIL);
        s_wifi_info.ssid[0] = '\0';
        s_wifi_info.password = EAT_WIFI_PASSWORD;
        s_wifi_info.url = EAT_WIFI_URL;
    }
    eat_router_enter_web(&s_router);
    s_page = EAT_PAGE_WEB;
    if (bsp_lvgl_lock(300)) {
        eat_ui_show_web(&s_ui);
        render_web_locked();
        bsp_lvgl_unlock();
    }
    s_dirty_web = true;
}

static void close_web_page(void) {
    (void)eat_captive_stop();
    (void)eat_web_stop();
    (void)eat_wifi_stop();
    eat_router_leave_web(&s_router);
    s_page = EAT_PAGE_MAIN;
    if (bsp_lvgl_lock(300)) {
        // 先把主界面内容画好再切屏，切换瞬间就是上一次停留的样子，不会闪白。
        render_main_locked();
        eat_ui_show_main(&s_ui);
        bsp_lvgl_unlock();
    }
    s_dirty_main = false;
}

static void apply_web_changes(void) {
    // 手机后台保存后：把镜像整体换成当前数据，并刷新主界面与后台页状态。
    s_sets = s_sets_mirror;
    rebuild_view();
    const eat_set_t *active = eat_sets_active(&s_sets);
    snprintf(s_web_status, sizeof(s_web_status), EAT_TEXT_WEB_STATS_FMT,
             (unsigned)(active == NULL ? 0 : active->items.count),
             active == NULL ? EAT_TEXT_RANGE_DEFAULT_NAME : active->name);
    s_toast_until_us = esp_timer_get_time() + TOAST_HOLD_US;
    s_dirty_main = true;
    s_dirty_web = true;
}

static void handle_action(eat_action_t action) {
    switch (action) {
        case EAT_ACT_SPIN_START:
            s_started = true;
            // 每次起转都从随机一项出发，避免“每次都是第一家”。
            eat_wheel_randomize(&s_wheel, esp_random());
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
            s_started = true;
            eat_wheel_step(&s_wheel, 1);
            s_dirty_main = true;
            break;
        case EAT_ACT_SCOPE_TOGGLE:
            // 只是换列表，不算“开始”，启动画面保持不动。
            cycle_set();
            break;
        case EAT_ACT_ACCEPT:
            s_started = true;
            s_accept_until_us = esp_timer_get_time() + ACCEPT_HOLD_US;
            s_dirty_main = true;
            break;
        case EAT_ACT_OPEN_WEB:
            s_started = true;
            open_web_page();
            break;
        case EAT_ACT_CLOSE_WEB:
            close_web_page();
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
    if (!s_dirty_main && !s_dirty_web && !s_toast_expiring) return;
    if (!bsp_lvgl_lock(50)) return;

    if (s_toast_expiring) {
        eat_ui_hide_toast(&s_ui);
        s_toast_expiring = false;
    }
    if (s_page == EAT_PAGE_WEB) {
        if (s_dirty_web) render_web_locked();
    } else if (s_dirty_main) {
        render_main_locked();
    }
    bsp_lvgl_unlock();

    s_dirty_main = false;
    s_dirty_web = false;
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

        if (s_web_saved) {
            s_web_saved = false;
            apply_web_changes();
        }
        if (eat_web_take_reset()) {
            // 手机上点了“回到封面”：退后台页、回默认池待机态。
            s_started = false;
            s_sets.active = -1;
            s_sets_mirror.active = -1;
            (void)eat_store_save_meta_only(&s_sets);
            rebuild_view();
            eat_wheel_randomize(&s_wheel, esp_random());
            close_web_page();
        }

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

// HTTP 任务保存成功后只置标志，由应用任务在安全点同步。
static void on_web_saved(void *context) {
    (void)context;
    s_web_saved = true;
}

void app_main(void) {
    ESP_LOGI(TAG, "今天吃什么 v1.1 启动");

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败（MOSI=%d SCLK=%d CS=%d DC=%d BL=%d）", BSP_LCD_MOSI,
                 BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    const bool battery_ok = bsp_battery_init() == ESP_OK;
    if (!battery_ok) ESP_LOGW(TAG, "电量计不可用，右上角显示 --");

    if (eat_store_open() != 0) {
        ESP_LOGW(TAG, "NVS 不可用，本次运行使用编译期默认池且不保存");
    }
    const bool seeded = eat_store_seed_if_needed(&s_seed_list);
    if (s_seed_list.count == 0) {
        eat_seed_fill_list(&s_seed_list);  // NVS 不可用时的兜底
    }
    const bool sets_loaded = eat_store_load_sets(&s_sets);
    if (!sets_loaded) {
        // 第一次启动（或旧格式不再兼容）：第 1 套预置一组出厂内容。
        eat_sets_fill_defaults(&s_sets);
        (void)eat_store_save_sets(&s_sets);
        ESP_LOGI(TAG, "第 1 套已填入 %u 项出厂内容",
                 (unsigned)s_sets.sets[0].items.count);
    }
    if (eat_store_migrate_legacy_user(&s_sets)) {
        ESP_LOGI(TAG, "已迁移旧版“我的”列表");
    }
    s_sets_mirror = s_sets;

    eat_name_list_pointers(&s_seed_list, s_seed_ptrs, EAT_USER_MAX);
    eat_router_init(&s_router);
    eat_wheel_init(&s_wheel, 0);
    rebuild_view();
    // 启动就随机落到一条，避免“封面”永远是同一家。
    eat_wheel_randomize(&s_wheel, esp_random());


    const char *active_name = eat_sets_active_name(&s_sets);
    ESP_LOGI(TAG, "默认池 %u 项（本次%s写入 NVS），当前范围 %s，轮盘 %u 项",
             (unsigned)s_seed_list.count, seeded ? "" : "未",
             active_name == NULL ? "默认池" : active_name,
             (unsigned)eat_wheel_view_count(&s_view));

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
