// main/eat_sets.h —— 自定义列表（最多 3 套，每套最多 50 项）。
//
// 1.1 起，“我的”条目不再是扁平列表，而是 3 套可命名的独立列表：
//   套 0..2：在手机后台里自由增删改，只走自己这一套，不再叠加默认池
//   active = -1：轮盘只用默认池
//   空套在主界面切换时会被跳过（设备上看不到空列表）。
#pragma once

#include "eat_what_pool.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EAT_SET_COUNT 3
#define EAT_SET_ITEMS_MAX EAT_USER_MAX
#define EAT_SET_NAME_MAX 24   // UTF-8，最多 7 个汉字 + NUL

typedef struct {
    char name[EAT_SET_NAME_MAX];
    eat_name_list_t items;
} eat_set_t;

typedef struct {
    eat_set_t sets[EAT_SET_COUNT];
    int8_t active;              // -1 = 仅默认池；0..2 = 该套
} eat_sets_t;

// 全部套清空、名字恢复为“我的N”、active = -1。
void eat_sets_reset(eat_sets_t *sets);

// 首次启动（NVS 里还没有列表信息）时给第 1 套填一组出厂内容，
// 用户随后可以在手机后台随意增删。
void eat_sets_fill_defaults(eat_sets_t *sets);

// meta blob：u16 version | i8 active | u8 count | count × char[24]
#define EAT_SETS_META_VERSION 2u
#define EAT_SETS_META_SIZE (4u + EAT_SET_COUNT * EAT_SET_NAME_MAX)
size_t eat_sets_serialize_meta(const eat_sets_t *sets, uint8_t *out, size_t capacity);
bool eat_sets_deserialize_meta(eat_sets_t *sets, const uint8_t *in, size_t length);

// 下一套：-1 → 0 → 1 → 2 → -1。非法输入按 -1 处理。
int8_t eat_sets_next_active(int8_t active);

// 下一套“有内容”的范围：跳过空套；从最后一站回到 -1（默认池）。
int8_t eat_sets_next_visible(const eat_sets_t *sets, int8_t active);

// 当前套；active 越界或为 -1 时返回 NULL。
const eat_set_t *eat_sets_active(const eat_sets_t *sets);
// 当前套的名字；无套返回 NULL。
const char *eat_sets_active_name(const eat_sets_t *sets);
// 当前轮盘条目数（空套回退到默认池，结果恒 > 0）。
size_t eat_sets_wheel_count(const eat_sets_t *sets, size_t seed_count);

// 按当前套填好轮盘视图。seed_names 由调用方持有，生命周期需覆盖视图使用期。
void eat_sets_fill_view(const eat_sets_t *sets, const char *const *seed_names,
                        size_t seed_count, eat_wheel_view_t *view);

// 追加一条到指定套；套号越界、满额、重复都返回 false。
bool eat_sets_append(eat_sets_t *sets, int index, const char *name);

// 套名截断并写入（UTF-8 安全，最多 7 个字符）。
void eat_sets_set_name(eat_sets_t *sets, int index, const char *name);

// 是否有任何一套非空（决定主界面要不要提示去手机后台建列表）。
bool eat_sets_any_filled(const eat_sets_t *sets);
