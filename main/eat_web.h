// main/eat_web.h —— 手机设置后台的 HTTP 服务。
//
// 路由：
//   GET  /            手机端单页（内嵌 HTML，支持增删改、切套、保存）
//   GET  /api/state   5 套列表 + 当前套（JSON）
//   POST /api/save    全量保存（JSON），成功写入 NVS 并触发回调
//
// 只在热点开启期间提供服务；回调运行在 HTTP 任务里，不要在其中操作 LVGL。
#pragma once

#include "eat_sets.h"
#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>

typedef void (*eat_web_saved_cb_t)(void *context);

esp_err_t eat_web_start(eat_sets_t *sets, size_t seed_count, eat_web_saved_cb_t on_saved,
                        void *context);
esp_err_t eat_web_stop(void);
bool eat_web_is_running(void);
