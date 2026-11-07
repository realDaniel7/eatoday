// main/eat_what_store.h —— 唯一持久层：种子池首次写入 + 5 套自定义列表（NVS）。
//
// 存储布局（namespace “eatwhat”）：
//   seed / seed_ver     默认池快照与版本（首次启动写入）
//   set_meta            套名、是否叠加默认池、当前套（eat_sets_serialize_meta）
//   set0 … set4         每套的条目（eat_pool_serialize，最多 50 项 ≈ 1.6KB）
//   user / scope        v0.1 的旧键，只在迁移时读取一次
#pragma once

#include "eat_what_pool.h"
#include "eat_sets.h"

#include <stdbool.h>
#include <stddef.h>

// 打开 NVS 命名空间（内部先 nvs_flash_init，必要时擦除重建）。返回 0 表示可用。
int eat_store_open(void);

// 首次启动（或种子版本升级）时把 30 项种子写入 NVS。返回 true 表示本次确实写入了。
bool eat_store_seed_if_needed(eat_name_list_t *seed_names);

// 读/写全部套（meta + 5 个 blob）。读取失败时 sets 被重置为空套。
bool eat_store_load_sets(eat_sets_t *sets);
bool eat_store_save_sets(const eat_sets_t *sets);
// 只写套信息（名字/叠加开关/当前套），切套时用，避免重写 5 份条目。
bool eat_store_save_meta_only(const eat_sets_t *sets);
// 只保存某一套的条目（手机后台保存时逐套写入，避免整包失败）。
bool eat_store_save_set(int index, const eat_name_list_t *items);

// v0.1 的“我的”列表迁移到第 1 套；没有旧数据时返回 false。
bool eat_store_migrate_legacy_user(eat_sets_t *sets);
