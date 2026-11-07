// main/eat_what_pool.c —— 名称池、blob 序列化与 UTF-8 帮助函数。
#include "eat_what_pool.h"

#include "eat_what_text.h"

#include <string.h>

// 名称连接符与界面文案共用同一份定义（见 eat_what_text.h）。
static const char NAME_JOINER[] = EAT_TEXT_JOINER;

static uint16_t read_u16(const uint8_t *in) {
    return (uint16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
}

static void write_u16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)(value >> 8);
}

void eat_name_list_reset(eat_name_list_t *list) {
    if (list == NULL) return;
    memset(list, 0, sizeof(*list));
}

void eat_seed_fill_list(eat_name_list_t *list) {
    if (list == NULL) return;
    eat_name_list_reset(list);
    for (size_t i = 0; i < EAT_SEED_POOL_COUNT && list->count < EAT_USER_MAX; i++) {
        eat_name_list_append(list, EAT_SEED_POOL[i].name);
    }
}

bool eat_name_list_contains(const eat_name_list_t *list, const char *name) {
    if (list == NULL || name == NULL || name[0] == '\0') return false;
    const uint16_t count = list->count > EAT_USER_MAX ? EAT_USER_MAX : list->count;
    for (uint16_t i = 0; i < count; i++) {
        if (strcmp(list->names[i], name) == 0) return true;
    }
    return false;
}

bool eat_name_list_append(eat_name_list_t *list, const char *name) {
    if (list == NULL || name == NULL) return false;
    const size_t length = strlen(name);
    if (length == 0 || length >= EAT_NAME_MAX_BYTES) return false;
    if (list->count >= EAT_USER_MAX) return false;
    if (eat_name_list_contains(list, name)) return false;
    memcpy(list->names[list->count], name, length + 1);
    list->count++;
    return true;
}

size_t eat_pool_blob_size(const eat_name_list_t *list) {
    const size_t count = (list == NULL || list->count > EAT_USER_MAX) ? 0 : list->count;
    return EAT_BLOB_HEADER + count * EAT_NAME_MAX_BYTES;
}

size_t eat_pool_serialize(const eat_name_list_t *list, uint8_t *out, size_t capacity) {
    if (list == NULL || out == NULL || list->count > EAT_USER_MAX) return 0;
    const size_t total = eat_pool_blob_size(list);
    if (capacity < total) return 0;
    write_u16(out, EAT_BLOB_VERSION);
    write_u16(out + 2, list->count);
    for (uint16_t i = 0; i < list->count; i++) {
        uint8_t *record = out + EAT_BLOB_HEADER + (size_t)i * EAT_NAME_MAX_BYTES;
        memset(record, 0, EAT_NAME_MAX_BYTES);
        const size_t length = strlen(list->names[i]);
        if (length == 0 || length >= EAT_NAME_MAX_BYTES) return 0;
        memcpy(record, list->names[i], length);
    }
    return total;
}

bool eat_pool_deserialize(eat_name_list_t *list, const uint8_t *in, size_t length) {
    if (list == NULL) return false;
    eat_name_list_reset(list);
    if (in == NULL || length < EAT_BLOB_HEADER) return false;
    if (read_u16(in) != EAT_BLOB_VERSION) return false;
    const uint16_t count = read_u16(in + 2);
    if (count > EAT_USER_MAX) return false;
    if (length != EAT_BLOB_HEADER + (size_t)count * EAT_NAME_MAX_BYTES) return false;

    for (uint16_t i = 0; i < count; i++) {
        const uint8_t *record = in + EAT_BLOB_HEADER + (size_t)i * EAT_NAME_MAX_BYTES;
        // 记录必须以 NUL 结尾且非空，否则视为损坏，整块作废。
        const uint8_t *terminator = memchr(record, '\0', EAT_NAME_MAX_BYTES);
        if (terminator == NULL || terminator == record) {
            eat_name_list_reset(list);
            return false;
        }
        memcpy(list->names[i], record, (size_t)(terminator - record) + 1);
    }
    list->count = count;
    return true;
}

size_t eat_utf8_chars(const char *text) {
    if (text == NULL) return 0;
    size_t chars = 0;
    const uint8_t *cursor = (const uint8_t *)text;
    while (*cursor != '\0') {
        const uint8_t byte = *cursor;
        size_t step = 1;
        if ((byte & 0x80u) == 0) {
            step = 1;
        } else if ((byte & 0xE0u) == 0xC0u) {
            step = 2;
        } else if ((byte & 0xF0u) == 0xE0u) {
            step = 3;
        } else if ((byte & 0xF8u) == 0xF0u) {
            step = 4;
        }
        for (size_t i = 1; i < step; i++) {
            if (cursor[i] == '\0') {  // 截断的多字节序列，按一个字符收尾
                return chars + 1;
            }
        }
        cursor += step;
        chars++;
    }
    return chars;
}

size_t eat_utf8_truncate(char *dst, size_t capacity, const char *src, size_t max_chars) {
    if (dst == NULL || capacity == 0) return 0;
    dst[0] = '\0';
    if (src == NULL || max_chars == 0) return 0;

    size_t written = 0;
    size_t chars = 0;
    const uint8_t *cursor = (const uint8_t *)src;
    while (*cursor != '\0' && chars < max_chars) {
        const uint8_t byte = *cursor;
        size_t step = 1;
        if ((byte & 0x80u) == 0) {
            step = 1;
        } else if ((byte & 0xE0u) == 0xC0u) {
            step = 2;
        } else if ((byte & 0xF0u) == 0xE0u) {
            step = 3;
        } else if ((byte & 0xF8u) == 0xF0u) {
            step = 4;
        }
        if (written + step + 1 > capacity) break;  // 留 NUL，不留半个字符
        bool complete = true;
        for (size_t i = 1; i < step; i++) {
            if (cursor[i] == '\0') {
                complete = false;
                break;
            }
        }
        if (!complete) break;
        memcpy(dst + written, cursor, step);
        written += step;
        cursor += step;
        chars++;
    }
    dst[written] = '\0';
    return written;
}

size_t eat_compose_name(char *dst, size_t capacity, const char *first, const char *second) {
    if (dst == NULL || capacity == 0) return 0;
    dst[0] = '\0';

    const char *head = (first != NULL && first[0] != '\0') ? first : NULL;
    const char *tail = (second != NULL && second[0] != '\0') ? second : NULL;
    if (head == NULL && tail == NULL) return 0;
    if (head == NULL) head = tail;  // 只有第二个词时就当唯一名称用

    const size_t head_length = strlen(head);
    if (head_length + 1 > capacity) return 0;
    memcpy(dst, head, head_length);
    size_t written = head_length;

    if (tail != NULL && strcmp(head, tail) != 0) {
        const size_t joiner = sizeof(NAME_JOINER) - 1;
        const size_t tail_length = strlen(tail);
        if (written + joiner + tail_length + 1 > capacity) {
            dst[0] = '\0';
            return 0;
        }
        memcpy(dst + written, NAME_JOINER, joiner);
        written += joiner;
        memcpy(dst + written, tail, tail_length);
        written += tail_length;
    }

    dst[written] = '\0';
    return written;
}

size_t eat_name_list_pointers(const eat_name_list_t *list, const char **dst, size_t capacity) {
    if (list == NULL || dst == NULL) return 0;
    const size_t count = list->count > EAT_USER_MAX ? EAT_USER_MAX : list->count;
    size_t written = 0;
    for (size_t i = 0; i < count && written < capacity; i++) {
        dst[written++] = list->names[i];
    }
    return written;
}

size_t eat_wheel_view_count(const eat_wheel_view_t *view) {
    if (view == NULL || view->seed_names == NULL) return 0;
    size_t total = view->include_seed ? view->seed_count : 0;
    if (view->extra != NULL) total += view->extra->count;
    if (total == 0) total = view->seed_count;  // 空套兜底：仍然转默认池
    return total;
}

const char *eat_wheel_view_name(const eat_wheel_view_t *view, size_t index) {
    const size_t count = eat_wheel_view_count(view);
    if (count == 0) return NULL;
    const size_t extra_count = (view->extra == NULL) ? 0 : view->extra->count;
    size_t slot = index % count;
    if (view->include_seed) {
        if (slot < view->seed_count) return view->seed_names[slot];
        slot -= view->seed_count;
    }
    if (slot < extra_count) return view->extra->names[slot];
    return view->seed_count > 0 ? view->seed_names[slot % view->seed_count] : NULL;
}

bool eat_wheel_view_is_user(const eat_wheel_view_t *view, size_t index) {
    const size_t count = eat_wheel_view_count(view);
    if (count == 0) return false;
    const size_t extra_count = (view->extra == NULL) ? 0 : view->extra->count;
    if (extra_count == 0) return false;
    const size_t slot = index % count;
    if (view->include_seed && slot < view->seed_count) return false;
    return true;
}
