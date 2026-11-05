// main/eat_what_store.c —— NVS：种子池首次写入、用户池、轮盘范围。
#include "eat_what_store.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

static const char *TAG = "eat_store";
static const char *const NVS_NAMESPACE = "eatwhat";
static const char *const KEY_SEED = "seed";
static const char *const KEY_SEED_VERSION = "seed_ver";
static const char *const KEY_USER = "user";
static const char *const KEY_SCOPE = "scope";

static nvs_handle_t s_handle;
static bool s_open;

// blob 上限约 1KB：放在静态区，避免占用 app_main 默认任务栈。
static uint8_t s_blob[EAT_BLOB_MAX];

int eat_store_open(void) {
    if (s_open) return 0;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS 分区需要重建: %s", esp_err_to_name(err));
        esp_err_t erase_err = nvs_flash_erase();
        if (erase_err != ESP_OK) {
            ESP_LOGE(TAG, "NVS 擦除失败: %s", esp_err_to_name(erase_err));
            return -1;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 初始化失败: %s", esp_err_to_name(err));
        return -1;
    }
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS 命名空间打开失败: %s", esp_err_to_name(err));
        return -1;
    }
    s_open = true;
    return 0;
}

bool eat_store_seed_if_needed(eat_name_list_t *seed_names) {
    if (!s_open || seed_names == NULL) return false;

    eat_name_list_t compiled;
    eat_seed_fill_list(&compiled);
    if (compiled.count == 0) {
        ESP_LOGE(TAG, "编译期种子池为空，无法播种");
        return false;
    }

    uint16_t version = 0;
    if (nvs_get_u16(s_handle, KEY_SEED_VERSION, &version) == ESP_OK &&
        version == EAT_SEED_VERSION) {
        size_t length = sizeof(s_blob);
        if (nvs_get_blob(s_handle, KEY_SEED, s_blob, &length) == ESP_OK &&
            eat_pool_deserialize(seed_names, s_blob, length) && seed_names->count > 0) {
            ESP_LOGI(TAG, "种子池已在 NVS（v%u，%u 项）", (unsigned)version,
                     (unsigned)seed_names->count);
            return false;
        }
        ESP_LOGW(TAG, "种子快照缺失或损坏，按编译期表重写");
    }

    const size_t length = eat_pool_serialize(&compiled, s_blob, sizeof(s_blob));
    if (length == 0) {
        ESP_LOGE(TAG, "种子池序列化失败");
        return false;
    }
    esp_err_t err = nvs_set_blob(s_handle, KEY_SEED, s_blob, length);
    if (err == ESP_OK) err = nvs_set_u16(s_handle, KEY_SEED_VERSION, EAT_SEED_VERSION);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "种子池写入 NVS 失败: %s", esp_err_to_name(err));
        return false;
    }
    // 首次播种是全新数据基线：用户池从空开始。
    nvs_erase_key(s_handle, KEY_USER);
    if (nvs_commit(s_handle) != ESP_OK) {
        ESP_LOGE(TAG, "种子池提交失败");
        return false;
    }
    *seed_names = compiled;
    ESP_LOGI(TAG, "首次启动：%u 项种子已写入 NVS", (unsigned)seed_names->count);
    return true;
}

bool eat_store_load_user(eat_name_list_t *user) {
    if (user == NULL) return false;
    eat_name_list_reset(user);
    if (!s_open) return false;
    size_t length = sizeof(s_blob);
    if (nvs_get_blob(s_handle, KEY_USER, s_blob, &length) != ESP_OK) return false;
    return eat_pool_deserialize(user, s_blob, length);
}

bool eat_store_save_user(const eat_name_list_t *user) {
    if (user == NULL || !s_open) return false;
    const size_t length = eat_pool_serialize(user, s_blob, sizeof(s_blob));
    if (length == 0) {
        ESP_LOGE(TAG, "用户池序列化失败");
        return false;
    }
    esp_err_t err = nvs_set_blob(s_handle, KEY_USER, s_blob, length);
    if (err == ESP_OK) err = nvs_commit(s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "用户池写入 NVS 失败: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool eat_store_load_scope(eat_scope_t *scope) {
    if (scope == NULL || !s_open) return false;
    uint8_t stored = 0;
    if (nvs_get_u8(s_handle, KEY_SCOPE, &stored) != ESP_OK) return false;
    if (stored >= (uint8_t)EAT_SCOPE_COUNT) return false;
    *scope = (eat_scope_t)stored;
    return true;
}

bool eat_store_save_scope(eat_scope_t scope) {
    if (!s_open || (unsigned)scope >= EAT_SCOPE_COUNT) return false;
    esp_err_t err = nvs_set_u8(s_handle, KEY_SCOPE, (uint8_t)scope);
    if (err == ESP_OK) err = nvs_commit(s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "轮盘范围写入 NVS 失败: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}
