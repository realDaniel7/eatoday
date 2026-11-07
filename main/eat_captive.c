// main/eat_captive.c —— 极简 DNS 劫持：任何域名都回 192.168.4.1。
//
// 手机连上热点后会去探测“是否联网”（iOS 的 captive.apple.com、Android 的
// connectivitycheck.gstatic.com 等）。把它们的域名都解析到本机，浏览器/系统
// 就会自动弹出设置页，不用手敲 192.168.4.1。
#include "eat_captive.h"

#include "eat_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

static const char *TAG = "eat_captive";

#define DNS_PORT 53
#define DNS_MAX_QUERY 512
#define DNS_TASK_STACK 3072

static volatile bool s_running;
static TaskHandle_t s_task;
static int s_socket = -1;

// 应答里固定回的地址（热点自己的 IP）。
static const uint8_t ANSWER_IP[4] = {192, 168, 4, 1};

static int build_answer(const uint8_t *query, int length, uint8_t *out, size_t capacity) {
    if (length < 12 || length > DNS_MAX_QUERY) return 0;
    // 只处理标准查询（QR=0），且至少有一个问题。
    if ((query[2] & 0x80) != 0) return 0;
    const int question_count = (query[4] << 8) | query[5];
    if (question_count < 1) return 0;
    const size_t needed = (size_t)length + 16;   // 头部 + 问题 + 一条 A 记录
    if (needed > capacity) return 0;

    memcpy(out, query, (size_t)length);
    out[2] = 0x81;   // 标准响应，递归可用
    out[3] = 0x80;
    out[6] = 0x00;   // answer count = 1
    out[7] = 0x01;
    out[8] = 0x00;   // authority = 0
    out[9] = 0x00;
    out[10] = 0x00;  // additional = 0
    out[11] = 0x00;

    uint8_t *cursor = out + length;
    *cursor++ = 0xC0;  // 指向问题里的同名域
    *cursor++ = 0x0C;
    *cursor++ = 0x00;  // type A
    *cursor++ = 0x01;
    *cursor++ = 0x00;  // class IN
    *cursor++ = 0x01;
    *cursor++ = 0x00;  // TTL 60s
    *cursor++ = 0x00;
    *cursor++ = 0x00;
    *cursor++ = 0x3C;
    *cursor++ = 0x00;  // rdlength = 4
    *cursor++ = 0x04;
    memcpy(cursor, ANSWER_IP, sizeof(ANSWER_IP));
    cursor += sizeof(ANSWER_IP);
    return (int)(cursor - out);
}

static void dns_task(void *arg) {
    (void)arg;
    uint8_t query[DNS_MAX_QUERY];
    uint8_t answer[DNS_MAX_QUERY + 32];

    while (s_running) {
        struct sockaddr_in from;
        socklen_t from_length = sizeof(from);
        const int received = recvfrom(s_socket, query, sizeof(query), 0,
                                      (struct sockaddr *)&from, &from_length);
        if (received <= 0) {
            if (!s_running) break;
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        const int length = build_answer(query, received, answer, sizeof(answer));
        if (length > 0) {
            (void)sendto(s_socket, answer, (size_t)length, 0, (struct sockaddr *)&from,
                         from_length);
        }
    }

    if (s_socket >= 0) {
        close(s_socket);
        s_socket = -1;
    }
    s_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t eat_captive_start(void) {
    if (s_running) return ESP_OK;

    s_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_socket < 0) {
        ESP_LOGE(TAG, "DNS socket 创建失败");
        return ESP_FAIL;
    }
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(DNS_PORT);
    if (bind(s_socket, (struct sockaddr *)&address, sizeof(address)) != 0) {
        ESP_LOGE(TAG, "DNS 端口绑定失败");
        close(s_socket);
        s_socket = -1;
        return ESP_FAIL;
    }

    s_running = true;
    if (xTaskCreate(dns_task, "eat_captive", DNS_TASK_STACK, NULL, 4, &s_task) != pdPASS) {
        s_running = false;
        close(s_socket);
        s_socket = -1;
        ESP_LOGE(TAG, "DNS 任务创建失败");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "DNS 劫持已开启：任意域名 → %s", EAT_WIFI_URL);
    return ESP_OK;
}

esp_err_t eat_captive_stop(void) {
    if (!s_running) return ESP_OK;
    s_running = false;
    if (s_socket >= 0) {
        // 先关 socket 让阻塞的 recvfrom 立刻返回，任务随之退出。
        shutdown(s_socket, SHUT_RDWR);
    }
    ESP_LOGI(TAG, "DNS 劫持已停止");
    return ESP_OK;
}
