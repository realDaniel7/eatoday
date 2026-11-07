// main/eat_what_pool.h —— 名称池模型与 NVS blob 序列化（纯逻辑，可主机测试）。
//
// 用户池是“我的”条目：在主界面以 ★ 前缀标识，追加在种子池之后。
// blob 布局（小端，定长记录）：
//   u16 version | u16 count | count × char[EAT_NAME_MAX_BYTES]（UTF-8，NUL 结尾）
#pragma once

#include "eat_what_seed.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EAT_NAME_MAX_BYTES 32   // 6 个汉字 + 连接符 + 余量，含结尾 NUL
#define EAT_USER_MAX 50         // 单套自定义列表上限（也是 NVS blob 的上限来源）
#define EAT_BLOB_VERSION 1u
#define EAT_BLOB_HEADER 4u
#define EAT_BLOB_MAX (EAT_BLOB_HEADER + EAT_USER_MAX * EAT_NAME_MAX_BYTES)

typedef struct {
    char names[EAT_USER_MAX][EAT_NAME_MAX_BYTES];
    uint16_t count;
} eat_name_list_t;

// 轮盘条目视图：要么只转默认池（include_seed=true, extra=NULL），
// 要么只转某一套自定义列表（include_seed=false, extra=该套）。
// seed_names 由应用层在启动时从 EAT_SEED_POOL（或 NVS 快照）准备好。
typedef struct {
    const char *const *seed_names;
    size_t seed_count;
    const eat_name_list_t *extra;   // 当前套的条目，可为 NULL
    bool include_seed;              // true = 默认池在前，自定义条目在后
} eat_wheel_view_t;

// 把 list 中每条名称的地址写进 dst（用于构造上面的 seed_names）。返回写入条数。
size_t eat_name_list_pointers(const eat_name_list_t *list, const char **dst, size_t capacity);

void eat_name_list_reset(eat_name_list_t *list);

// 用编译期 EAT_SEED_POOL 填充 list（30 项出厂种子）。
void eat_seed_fill_list(eat_name_list_t *list);

// 追加一条名称：空串、超长、重复（池内同名）或已满都返回 false，且不修改池。
// 调用方负责先把名称截断到 EAT_NAME_MAX_BYTES 以内（见 eat_utf8_truncate）。
bool eat_name_list_append(eat_name_list_t *list, const char *name);
bool eat_name_list_contains(const eat_name_list_t *list, const char *name);

// 序列化长度（字节）。count 为 0 时仍返回 header 长度，便于写入“空池”。
size_t eat_pool_blob_size(const eat_name_list_t *list);
// 写入 blob，返回写入字节数；容量不足返回 0。
size_t eat_pool_serialize(const eat_name_list_t *list, uint8_t *out, size_t capacity);
// 解析 blob。版本不符、长度不符、名称未终止等任何异常都返回 false 并清空 list。
bool eat_pool_deserialize(eat_name_list_t *list, const uint8_t *in, size_t length);

// UTF-8 帮助函数：按“字符”计数与截断，绝不切出半个汉字。
size_t eat_utf8_chars(const char *text);
// 把 src 的前 max_chars 个字符写入 dst（含结尾 NUL）；返回写入字节数（不含 NUL）。
// capacity 不足以放下完整字符时提前停止，不留半个字符。
size_t eat_utf8_truncate(char *dst, size_t capacity, const char *src, size_t max_chars);

// 用 "·" 把两个词拼成“品类·短名称”。任一为空则只用另一个。
// 返回写入字节数（不含 NUL）；放不下时返回 0。
size_t eat_compose_name(char *dst, size_t capacity, const char *first, const char *second);

size_t eat_wheel_view_count(const eat_wheel_view_t *view);
// index 会先按 count 取模；count 为 0 时返回 NULL。
const char *eat_wheel_view_name(const eat_wheel_view_t *view, size_t index);
// 该下标是否是用户池条目（决定是否加 ★ 前缀）。越界返回 false。
bool eat_wheel_view_is_user(const eat_wheel_view_t *view, size_t index);
