// main/eat_what_seed.h —— “今天吃什么”种子池数据。
//
// 纯数据 + 纯逻辑：不依赖 ESP-IDF / LVGL，主机测试可直接编译本文件。
// 首次启动时由 eat_what_store 把这份表写入 NVS，之后运行期以 NVS 快照为准；
// 编译期表仍是“出厂内容”的唯一事实来源，也能在 NVS 损坏时兜底。
#pragma once

#include <stddef.h>

// 种子条目的品类，只用于分组维护与文案，不参与轮盘顺序（轮盘按表内顺序走）。
typedef enum {
    EAT_CAT_ROAST_DUCK = 0,  // 京菜烤鸭
    EAT_CAT_SNACK,           // 小吃快餐
    EAT_CAT_HOTPOT,          // 火锅
    EAT_CAT_SICHUAN,         // 川味
    EAT_CAT_HOMESTYLE,       // 家常
    EAT_CAT_LOCAL,           // 亲民小馆
    EAT_CAT_FALLBACK,        // 轻决策兜底
    EAT_CAT_COUNT
} eat_category_t;

typedef struct {
    const char *name;         // UTF-8，显示用全名
    eat_category_t category;
} eat_seed_t;

// 种子池内容版本：改动 EAT_SEED_POOL 时递增，已存快照会在下次启动被重写。
#define EAT_SEED_VERSION 1u

// 表内顺序即种子轮盘的顺序。
extern const eat_seed_t EAT_SEED_POOL[];
extern const size_t EAT_SEED_POOL_COUNT;

// 品类中文标签；越界索引返回空串。
const char *eat_category_label(eat_category_t category);
