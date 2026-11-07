// main/eat_wifi.c —— SoftAP 启停。只在本机热点内提供设置页面，不连接任何外部网络。
#include "eat_wifi.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "eat_wifi";

static bool s_netif_ready;
static bool s_stack_ready;
static bool s_running;
static esp_netif_t *s_ap_netif;

static void build_ssid(char *out, size_t capacity) {
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) != ESP_OK) {
        snprintf(out, capacity, "Eatoday");
        return;
    }
    snprintf(out, capacity, "Eatoday-%02X%02X", mac[4], mac[5]);
}

esp_err_t eat_wifi_start(eat_wifi_info_t *info) {
    if (s_running) {
        if (info != NULL) {
            build_ssid(info->ssid, sizeof(info->ssid));
            info->password = EAT_WIFI_PASSWORD;
            info->url = EAT_WIFI_URL;
        }
        return ESP_OK;
    }

    if (!s_netif_ready) {
        esp_err_t err = esp_netif_init();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "esp_netif_init 失败: %s", esp_err_to_name(err));
            return err;
        }
        err = esp_event_loop_create_default();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "事件循环创建失败: %s", esp_err_to_name(err));
            return err;
        }
        s_ap_netif = esp_netif_create_default_wifi_ap();
        if (s_ap_netif == NULL) {
            ESP_LOGE(TAG, "AP netif 创建失败");
            return ESP_FAIL;
        }
        s_netif_ready = true;
    }

    if (!s_stack_ready) {
        const wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err = esp_wifi_init(&init);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_wifi_init 失败: %s", esp_err_to_name(err));
            return err;
        }
        s_stack_ready = true;
    }

    char ssid[EAT_WIFI_SSID_MAX];
    build_ssid(ssid, sizeof(ssid));

    wifi_config_t config = {0};
    memcpy(config.ap.ssid, ssid, strlen(ssid));
    config.ap.ssid_len = (uint8_t)strlen(ssid);
    memcpy(config.ap.password, EAT_WIFI_PASSWORD, strlen(EAT_WIFI_PASSWORD));
    config.ap.channel = 1;
    config.ap.max_connection = 2;
    config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    config.ap.pmf_cfg.required = false;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err == ESP_OK) err = esp_wifi_set_config(WIFI_IF_AP, &config);
    if (err == ESP_OK) err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "热点启动失败: %s", esp_err_to_name(err));
        return err;
    }

    s_running = true;
    ESP_LOGI(TAG, "热点已开启 SSID=%s 密码=%s 地址=%s", ssid, EAT_WIFI_PASSWORD, EAT_WIFI_URL);
    if (info != NULL) {
        snprintf(info->ssid, sizeof(info->ssid), "%s", ssid);
        info->password = EAT_WIFI_PASSWORD;
        info->url = EAT_WIFI_URL;
    }
    return ESP_OK;
}

esp_err_t eat_wifi_stop(void) {
    if (!s_stack_ready) return ESP_OK;
    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT && err != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGW(TAG, "关闭热点失败: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "热点已关闭");
    }
    s_running = false;
    return ESP_OK;
}

bool eat_wifi_is_running(void) {
    return s_running;
}
