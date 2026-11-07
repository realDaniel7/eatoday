// main/eat_wifi.h —— 手机设置后台用的 Wi-Fi 热点（SoftAP）。
//
// 只在“手机后台”页面里开启：手机连上热点后浏览器打开 http://192.168.4.1 即可编辑列表。
// 离开页面就关掉，不常驻耗电。
#pragma once

#include "esp_err.h"

#include <stdbool.h>

#define EAT_WIFI_SSID_MAX 32
#define EAT_WIFI_PASSWORD "eatoday88"
#define EAT_WIFI_URL "http://192.168.4.1"

typedef struct {
    char ssid[EAT_WIFI_SSID_MAX];
    const char *password;
    const char *url;
} eat_wifi_info_t;

// 启动热点（可重复调用）。成功后 info 填好 SSID 与访问地址。
esp_err_t eat_wifi_start(eat_wifi_info_t *info);
// 关闭热点并释放协议栈资源（可重复调用）。
esp_err_t eat_wifi_stop(void);
bool eat_wifi_is_running(void);
