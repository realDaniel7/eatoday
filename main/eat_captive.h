// main/eat_captive.h —— 连上热点后自动弹出设置页（Captive Portal）。
//
// 两个动作配合：
//   1) DNS 劫持：所有域名都解析到 192.168.4.1，手机探测联网状态时会被引到本机；
//   2) HTTP 404 → 302 跳转到设置页（注册在 eat_web.c 里）。
// 只在设置后台打开期间运行。
#pragma once

#include "esp_err.h"

esp_err_t eat_captive_start(void);
esp_err_t eat_captive_stop(void);
