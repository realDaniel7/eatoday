// main/eat_web.c —— 设置后台的 HTTP 路由与 JSON 处理。
#include "eat_web.h"

#include "cJSON.h"
#include "eat_what_pool.h"
#include "eat_what_store.h"
#include "eat_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

extern const char EAT_WEB_PAGE[];

static const char *TAG = "eat_web";

#define EAT_WEB_BODY_MAX 8192
#define EAT_WEB_ITEM_MAX_CHARS 10   // 单个条目最多 10 个字符

static httpd_handle_t s_server;
static eat_sets_t *s_sets;
static size_t s_seed_count;
static eat_web_saved_cb_t s_on_saved;
static void *s_context;

// 请求体放静态区（HTTP 任务栈只有几 KB）。解析结果直接写进 mirror，
// 解析失败时由调用方从 NVS 重载，避免把半截数据留在内存里。
static char s_body[EAT_WEB_BODY_MAX];

static esp_err_t send_text(httpd_req_t *req, const char *message) {
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    char payload[192];
    snprintf(payload, sizeof(payload), "{\"ok\":false,\"error\":\"%s\"}", message);
    return httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
}

// 手机探测联网状态时会访问各种域名/路径：一律 302 到设置页，系统便会“自动弹出”登录页。
static esp_err_t handle_not_found(httpd_req_t *req, httpd_err_code_t error) {
    (void)error;
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", EAT_WIFI_URL "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t handle_index(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, EAT_WEB_PAGE, HTTPD_RESP_USE_STRLEN);
}

static void add_sets(cJSON *root, const eat_sets_t *sets) {
    cJSON *array = cJSON_AddArrayToObject(root, "sets");
    for (int i = 0; i < EAT_SET_COUNT; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", sets->sets[i].name);
        cJSON *items = cJSON_AddArrayToObject(item, "items");
        for (int j = 0; j < (int)sets->sets[i].items.count; j++) {
            cJSON_AddItemToArray(items, cJSON_CreateString(sets->sets[i].items.names[j]));
        }
        cJSON_AddItemToArray(array, item);
    }
}

static esp_err_t handle_state(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return send_text(req, "内存不足");
    cJSON_AddNumberToObject(root, "active", s_sets->active);
    cJSON_AddNumberToObject(root, "seedCount", (double)s_seed_count);
    add_sets(root, s_sets);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) return send_text(req, "序列化失败");
    httpd_resp_set_type(req, "application/json; charset=utf-8");
    const esp_err_t err = httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    cJSON_free(payload);
    return err;
}

static bool read_body(httpd_req_t *req, char *buffer, size_t capacity) {
    const size_t total = req->content_len;
    if (total == 0 || total >= capacity) return false;
    size_t got = 0;
    while (got < total) {
        const int chunk = httpd_req_recv(req, buffer + got, total - got);
        if (chunk == HTTPD_SOCK_ERR_TIMEOUT) continue;
        if (chunk <= 0) return false;
        got += (size_t)chunk;
    }
    buffer[got] = '\0';
    return true;
}

static bool parse_save(const char *json, eat_sets_t *out, char *error, size_t error_capacity) {
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        snprintf(error, error_capacity, "JSON 解析失败");
        return false;
    }

    const cJSON *sets = cJSON_GetObjectItem(root, "sets");
    if (!cJSON_IsArray(sets) || cJSON_GetArraySize(sets) != EAT_SET_COUNT) {
        snprintf(error, error_capacity, "需要 %d 套列表", EAT_SET_COUNT);
        cJSON_Delete(root);
        return false;
    }

    eat_sets_reset(out);
    int index = 0;
    const cJSON *set = NULL;
    cJSON_ArrayForEach(set, sets) {
        const cJSON *name = cJSON_GetObjectItem(set, "name");
        const cJSON *items = cJSON_GetObjectItem(set, "items");
        if (index >= EAT_SET_COUNT) break;

        if (cJSON_IsString(name)) eat_sets_set_name(out, index, name->valuestring);

        if (cJSON_IsArray(items)) {
            const cJSON *entry = NULL;
            cJSON_ArrayForEach(entry, items) {
                if (!cJSON_IsString(entry)) continue;
                char trimmed[EAT_NAME_MAX_BYTES];
                eat_utf8_truncate(trimmed, sizeof(trimmed), entry->valuestring,
                                  EAT_WEB_ITEM_MAX_CHARS);
                if (trimmed[0] == '\0') continue;
                if (!eat_name_list_append(&out->sets[index].items, trimmed) &&
                    out->sets[index].items.count >= EAT_SET_ITEMS_MAX) {
                    snprintf(error, error_capacity, "每套最多 %d 项", EAT_SET_ITEMS_MAX);
                    cJSON_Delete(root);
                    return false;
                }
            }
        }
        index++;
    }

    const cJSON *active = cJSON_GetObjectItem(root, "active");
    if (cJSON_IsNumber(active)) {
        const int value = active->valueint;
        out->active = (value >= 0 && value < EAT_SET_COUNT) ? (int8_t)value : -1;
    } else {
        out->active = -1;
    }

    cJSON_Delete(root);
    return true;
}

static esp_err_t handle_save(httpd_req_t *req) {
    if (!read_body(req, s_body, sizeof(s_body))) return send_text(req, "内容过大或读取失败");

    char error[64] = {0};
    if (!parse_save(s_body, s_sets, error, sizeof(error))) {
        (void)eat_store_load_sets(s_sets);  // 解析失败：恢复成存储里的数据
        return send_text(req, error);
    }
    if (!eat_store_save_sets(s_sets)) return send_text(req, "写入失败，请重试");

    ESP_LOGI(TAG, "手机后台已保存：当前套 %d", (int)s_sets->active);
    if (s_on_saved != NULL) s_on_saved(s_context);

    httpd_resp_set_type(req, "application/json; charset=utf-8");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}

esp_err_t eat_web_start(eat_sets_t *sets, size_t seed_count, eat_web_saved_cb_t on_saved,
                        void *context) {
    if (s_server != NULL) return ESP_OK;
    if (sets == NULL) return ESP_ERR_INVALID_ARG;

    s_sets = sets;
    s_seed_count = seed_count;
    s_on_saved = on_saved;
    s_context = context;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;
    config.stack_size = 6144;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    if (httpd_start(&s_server, &config) != ESP_OK) {
        s_server = NULL;
        ESP_LOGE(TAG, "HTTP 服务启动失败");
        return ESP_FAIL;
    }

    const httpd_uri_t index_uri = {
        .uri = "/", .method = HTTP_GET, .handler = handle_index, .user_ctx = NULL,
    };
    const httpd_uri_t state_uri = {
        .uri = "/api/state", .method = HTTP_GET, .handler = handle_state, .user_ctx = NULL,
    };
    const httpd_uri_t save_uri = {
        .uri = "/api/save", .method = HTTP_POST, .handler = handle_save, .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &index_uri);
    httpd_register_uri_handler(s_server, &state_uri);
    httpd_register_uri_handler(s_server, &save_uri);
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, handle_not_found);

    ESP_LOGI(TAG, "HTTP 服务已启动");
    return ESP_OK;
}

esp_err_t eat_web_stop(void) {
    if (s_server == NULL) return ESP_OK;
    httpd_stop(s_server);
    s_server = NULL;
    ESP_LOGI(TAG, "HTTP 服务已停止");
    return ESP_OK;
}

bool eat_web_is_running(void) {
    return s_server != NULL;
}
