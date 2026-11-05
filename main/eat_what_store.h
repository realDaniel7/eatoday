// main/eat_what_store.h —— 唯一持久层：种子池首次写入 + 用户池 + 轮盘范围（NVS）。
//
// 这一层只做“读写 NVS”，不含轮盘与按键逻辑；序列化格式在 eat_what_pool.c 里，
// 因此存储布局可以在主机上测试，不需要芯片。
#pragma once

#include "eat_what_pool.h"

#include <stdbool.h>
#include <stddef.h>

// 打开 NVS 命名空间（内部会先 nvs_flash_init，必要时擦除重建）。
// 返回 ESP_OK 表示之后可以读写；失败时调用方应让应用以只读的编译期数据继续运行。
int eat_store_open(void);

// 首次启动（或种子版本升级）时把 30 项种子写入 NVS，并清空用户池。
// 返回 true 表示本次确实写入了 NVS。
bool eat_store_seed_if_needed(eat_name_list_t *seed_names);

// 读取用户池；NVS 中没有记录时返回空池并返回 false。
bool eat_store_load_user(eat_name_list_t *user);
bool eat_store_save_user(const eat_name_list_t *user);

// 轮盘范围。NVS 中没有记录时保持传入值不变并返回 false。
bool eat_store_load_scope(eat_scope_t *scope);
bool eat_store_save_scope(eat_scope_t scope);
