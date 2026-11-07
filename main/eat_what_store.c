// main/eat_what_store.c —— NVS：默认池快照 + 5 套自定义列表。
#include "eat_what_store.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "eat_store";
static const char *const NVS_NAMESPACE = "eatwhat";
static const char *const KEY_SEED = "seed";
static const char *const KEY_SEED_VERSION = "seed_ver";
static const char *const KEY_META = "set_meta";
static const char *const KEY_LEGACY_USER = "user";
static const char *const KEY_LEGACY_SCOPE = "scope";

static nvs_handle_t s_handle;
static bool s_open;

// blob 上限约 1.6KB：放在静态区，避免占用任务栈。
static uint8_t s_blob[EAT_BLOB_MAX];
static uint8_t s_meta[EAT_SETS_META_SIZE];

static void set_key(int index, char *out, size_t capacity) {
    snprintf(out, capacity, "set%d", index);
}

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
            ESP_LOGI(TAG, "默认池已在 NVS（v%u，%u 项）", (unsigned)version,
                     (unsigned)seed_names->count);
            return false;
        }
        ESP_LOGW(TAG, "默认池快照缺失或损坏，按编译期表重写");
    }

    const size_t length = eat_pool_serialize(&compiled, s_blob, sizeof(s_blob));
    if (length == 0) {
        ESP_LOGE(TAG, "默认池序列化失败");
        return false;
    }
    esp_err_t err = nvs_set_blob(s_handle, KEY_SEED, s_blob, length);
    if (err == ESP_OK) err = nvs_set_u16(s_handle, KEY_SEED_VERSION, EAT_SEED_VERSION);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "默认池写入 NVS 失败: %s", esp_err_to_name(err));
        return false;
    }
    if (nvs_commit(s_handle) != ESP_OK) {
        ESP_LOGE(TAG, "默认池提交失败");
        return false;
    }
    *seed_names = compiled;
    ESP_LOGI(TAG, "首次启动：%u 项默认池已写入 NVS", (unsigned)seed_names->count);
    return true;
}

bool eat_store_save_set(int index, const eat_name_list_t *items) {
    if (!s_open || items == NULL || index < 0 || index >= EAT_SET_COUNT) return false;
    const size_t length = eat_pool_serialize(items, s_blob, sizeof(s_blob));
    if (length == 0) {
        ESP_LOGE(TAG, "套 %d 序列化失败", index);
        return false;
    }
    char key[8];
    set_key(index, key, sizeof(key));
    esp_err_t err = nvs_set_blob(s_handle, key, s_blob, length);
    if (err == ESP_OK) err = nvs_commit(s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "套 %d 写入失败: %s", index, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool eat_store_save_sets(const eat_sets_t *sets) {
    if (!s_open || sets == NULL) return false;
    if (eat_sets_serialize_meta(sets, s_meta, sizeof(s_meta)) == 0) return false;
    esp_err_t err = nvs_set_blob(s_handle, KEY_META, s_meta, EAT_SETS_META_SIZE);
    if (err == ESP_OK) err = nvs_commit(s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "套信息写入失败: %s", esp_err_to_name(err));
        return false;
    }
    for (int i = 0; i < EAT_SET_COUNT; i++) {
        if (!eat_store_save_set(i, &sets->sets[i].items)) return false;
    }
    return true;
}

bool eat_store_save_meta_only(const eat_sets_t *sets) {
    if (!s_open || sets == NULL) return false;
    if (eat_sets_serialize_meta(sets, s_meta, sizeof(s_meta)) == 0) return false;
    esp_err_t err = nvs_set_blob(s_handle, KEY_META, s_meta, EAT_SETS_META_SIZE);
    if (err == ESP_OK) err = nvs_commit(s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "套信息写入失败: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool eat_store_load_sets(eat_sets_t *sets) {
    if (sets == NULL) return false;
    eat_sets_reset(sets);
    if (!s_open) return false;

    bool meta_ok = false;
    size_t meta_length = sizeof(s_meta);
    if (nvs_get_blob(s_handle, KEY_META, s_meta, &meta_length) == ESP_OK &&
        eat_sets_deserialize_meta(sets, s_meta, meta_length)) {
        meta_ok = true;
    } else {
        eat_sets_reset(sets);
    }

    for (int i = 0; i < EAT_SET_COUNT; i++) {
        char key[8];
        set_key(i, key, sizeof(key));
        size_t length = sizeof(s_blob);
        if (nvs_get_blob(s_handle, key, s_blob, &length) != ESP_OK) continue;
        if (!eat_pool_deserialize(&sets->sets[i].items, s_blob, length)) {
            ESP_LOGW(TAG, "套 %d 条目损坏，已清空", i);
            eat_name_list_reset(&sets->sets[i].items);
        }
    }
    ESP_LOGI(TAG, "套信息 %s，当前套 %d", meta_ok ? "已读取" : "缺失（默认空套）",
             (int)sets->active);
    return meta_ok;
}

bool eat_store_migrate_legacy_user(eat_sets_t *sets) {
    if (!s_open || sets == NULL) return false;

    size_t length = sizeof(s_blob);
    const bool found = nvs_get_blob(s_handle, KEY_LEGACY_USER, s_blob, &length) == ESP_OK;
    if (!found) return false;

    eat_name_list_t legacy;
    if (!eat_pool_deserialize(&legacy, s_blob, length) || legacy.count == 0) {
        nvs_erase_key(s_handle, KEY_LEGACY_USER);
        nvs_commit(s_handle);
        return false;
    }

    if (sets->sets[0].items.count == 0) {
        sets->sets[0].items = legacy;
        eat_utf8_truncate(sets->sets[0].name, EAT_SET_NAME_MAX, "我的", 7);
        sets->active = 0;
        (void)eat_store_save_sets(sets);
        ESP_LOGI(TAG, "已把 v0.1 的 %u 条“我的”迁移到第 1 套", (unsigned)legacy.count);
    }

    nvs_erase_key(s_handle, KEY_LEGACY_USER);
    nvs_erase_key(s_handle, KEY_LEGACY_SCOPE);
    nvs_commit(s_handle);
    return true;
}
