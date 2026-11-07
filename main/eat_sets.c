// main/eat_sets.c —— 自定义列表的增删改、范围循环与 meta 序列化（纯逻辑）。
#include "eat_sets.h"

#include "eat_what_text.h"

#include <string.h>

// 出厂预置在第 1 套里的店，覆盖烧烤、寿司、快餐、小炒等日常选择。
static const char *const FACTORY_SET_ITEMS[] = {
    "1976烧烤吧", "吉野家", "滨寿司", "肉串汪", "汉拿山", "肯德基",
    "麦当劳", "李先生牛肉面", "阿招鸡煲", "DQ", "烤匠", "寿司郎",
    "沙县小吃", "西十二街牛排", "费大厨小炒肉", "争先寿司", "夜市烤生蚝", "萨莉亚",
};

static void set_default_name(eat_set_t *set, int index) {
    memset(set->name, 0, sizeof(set->name));
    const char *name = EAT_DEFAULT_SET_NAMES[index % EAT_DEFAULT_SET_NAME_COUNT];
    const size_t length = strlen(name);
    memcpy(set->name, name, length < EAT_SET_NAME_MAX ? length : EAT_SET_NAME_MAX - 1);
}

void eat_sets_reset(eat_sets_t *sets) {
    if (sets == NULL) return;
    memset(sets, 0, sizeof(*sets));
    sets->active = -1;
    for (int i = 0; i < EAT_SET_COUNT; i++) {
        eat_name_list_reset(&sets->sets[i].items);
        set_default_name(&sets->sets[i], i);
    }
}

void eat_sets_fill_defaults(eat_sets_t *sets) {
    if (sets == NULL) return;
    const size_t count = sizeof(FACTORY_SET_ITEMS) / sizeof(FACTORY_SET_ITEMS[0]);
    for (size_t i = 0; i < count; i++) {
        (void)eat_name_list_append(&sets->sets[0].items, FACTORY_SET_ITEMS[i]);
    }
    eat_sets_set_name(sets, 0, "常吃");
}

size_t eat_sets_serialize_meta(const eat_sets_t *sets, uint8_t *out, size_t capacity) {
    if (sets == NULL || out == NULL || capacity < EAT_SETS_META_SIZE) return 0;
    out[0] = (uint8_t)(EAT_SETS_META_VERSION & 0xFFu);
    out[1] = (uint8_t)(EAT_SETS_META_VERSION >> 8);
    out[2] = (uint8_t)(int8_t)sets->active;
    out[3] = (uint8_t)EAT_SET_COUNT;
    for (int i = 0; i < EAT_SET_COUNT; i++) {
        uint8_t *slot = out + 4 + (size_t)i * EAT_SET_NAME_MAX;
        memset(slot, 0, EAT_SET_NAME_MAX);
        size_t length = strlen(sets->sets[i].name);
        if (length >= EAT_SET_NAME_MAX) length = EAT_SET_NAME_MAX - 1;
        memcpy(slot, sets->sets[i].name, length);
    }
    return EAT_SETS_META_SIZE;
}

bool eat_sets_deserialize_meta(eat_sets_t *sets, const uint8_t *in, size_t length) {
    if (sets == NULL) return false;
    if (in == NULL || length != EAT_SETS_META_SIZE) return false;
    const uint16_t version = (uint16_t)(in[0] | ((uint16_t)in[1] << 8));
    if (version != EAT_SETS_META_VERSION) return false;
    if (in[3] != (uint8_t)EAT_SET_COUNT) return false;

    const int8_t active = (int8_t)in[2];
    if (active < -1 || active >= EAT_SET_COUNT) return false;

    for (int i = 0; i < EAT_SET_COUNT; i++) {
        const uint8_t *name = in + 4 + (size_t)i * EAT_SET_NAME_MAX;
        if (memchr(name, '\0', EAT_SET_NAME_MAX) == NULL) return false;
        memset(sets->sets[i].name, 0, sizeof(sets->sets[i].name));
        const size_t name_length = strnlen((const char *)name, EAT_SET_NAME_MAX - 1);
        if (name_length == 0) {
            set_default_name(&sets->sets[i], i);
        } else {
            memcpy(sets->sets[i].name, name, name_length);
        }
    }
    sets->active = active;
    return true;
}

int8_t eat_sets_next_active(int8_t active) {
    if (active < -1 || active >= EAT_SET_COUNT) return -1;
    if (active == EAT_SET_COUNT - 1) return -1;
    return (int8_t)(active + 1);
}

int8_t eat_sets_next_visible(const eat_sets_t *sets, int8_t active) {
    if (sets == NULL) return -1;
    int8_t candidate = active;
    for (int step = 0; step < EAT_SET_COUNT + 1; step++) {
        candidate = eat_sets_next_active(candidate);
        if (candidate < 0) return -1;                       // 回到默认池
        if (sets->sets[candidate].items.count > 0) return candidate;
    }
    return -1;
}

const eat_set_t *eat_sets_active(const eat_sets_t *sets) {
    if (sets == NULL) return NULL;
    if (sets->active < 0 || sets->active >= EAT_SET_COUNT) return NULL;
    return &sets->sets[sets->active];
}

const char *eat_sets_active_name(const eat_sets_t *sets) {
    const eat_set_t *set = eat_sets_active(sets);
    return set == NULL ? NULL : set->name;
}

bool eat_sets_any_filled(const eat_sets_t *sets) {
    if (sets == NULL) return false;
    for (int i = 0; i < EAT_SET_COUNT; i++) {
        if (sets->sets[i].items.count > 0) return true;
    }
    return false;
}

size_t eat_sets_wheel_count(const eat_sets_t *sets, size_t seed_count) {
    const eat_set_t *set = eat_sets_active(sets);
    if (set == NULL || set->items.count == 0) return seed_count;
    return set->items.count;
}

void eat_sets_fill_view(const eat_sets_t *sets, const char *const *seed_names,
                        size_t seed_count, eat_wheel_view_t *view) {
    if (view == NULL) return;
    view->seed_names = seed_names;
    view->seed_count = seed_count;
    view->extra = NULL;
    view->include_seed = true;

    const eat_set_t *set = eat_sets_active(sets);
    if (set == NULL || set->items.count == 0) return;  // 无套或空套 → 默认池
    view->extra = &set->items;
    view->include_seed = false;                        // 自定义套不再叠加默认池
}

bool eat_sets_append(eat_sets_t *sets, int index, const char *name) {
    if (sets == NULL || index < 0 || index >= EAT_SET_COUNT) return false;
    return eat_name_list_append(&sets->sets[index].items, name);
}

void eat_sets_set_name(eat_sets_t *sets, int index, const char *name) {
    if (sets == NULL || index < 0 || index >= EAT_SET_COUNT) return;
    eat_set_t *set = &sets->sets[index];
    if (name == NULL || name[0] == '\0') {
        set_default_name(set, index);
        return;
    }
    const size_t written = eat_utf8_truncate(set->name, sizeof(set->name), name, 7);
    if (written == 0) set_default_name(set, index);
}
