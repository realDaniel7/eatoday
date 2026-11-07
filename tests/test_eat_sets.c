// tests/test_eat_sets.c —— 3 套自定义列表：默认值、meta 往返、范围循环、空套跳过。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "eat_sets.h"

static void test_reset_defaults(void) {
    eat_sets_t sets;
    eat_sets_reset(&sets);
    assert(sets.active == -1);
    for (int i = 0; i < EAT_SET_COUNT; i++) {
        assert(sets.sets[i].items.count == 0);
        assert(strncmp(sets.sets[i].name, "我的", strlen("我的")) == 0);
    }
    assert(eat_sets_active(&sets) == NULL);
    assert(eat_sets_active_name(&sets) == NULL);
    assert(!eat_sets_any_filled(&sets));
}

static void test_factory_defaults(void) {
    eat_sets_t sets;
    eat_sets_reset(&sets);
    assert(!eat_sets_any_filled(&sets));       // reset 本身不预置内容

    eat_sets_fill_defaults(&sets);
    assert(sets.sets[0].items.count >= 10);    // 出厂内容都进第 1 套
    assert(strcmp(sets.sets[0].items.names[0], "1976烧烤吧") == 0);
    assert(eat_sets_any_filled(&sets));
    assert(sets.sets[1].items.count == 0);     // 第 2、3 套保持空白

    // 重复调用不会重复追加（同名会被池内去重挡掉）。
    const uint16_t before = sets.sets[0].items.count;
    eat_sets_fill_defaults(&sets);
    assert(sets.sets[0].items.count == before);
}

static void test_next_active_cycles(void) {
    assert(eat_sets_next_active(-1) == 0);
    assert(eat_sets_next_active(0) == 1);
    assert(eat_sets_next_active(1) == 2);
    assert(eat_sets_next_active(2) == -1);
    // 非法输入收敛到“仅默认池”，不会越界。
    assert(eat_sets_next_active(9) == -1);
    assert(eat_sets_next_active(-5) == -1);
}

static void test_next_visible_skips_empty_sets(void) {
    eat_sets_t sets;
    eat_sets_reset(&sets);
    // 全空：任何一次切换都回到默认池。
    assert(eat_sets_next_visible(&sets, -1) == -1);

    // 只有第 2 套有内容：从默认池直接跳到它，再往前就回默认池。
    assert(eat_sets_append(&sets, 1, "拉面"));
    assert(eat_sets_next_visible(&sets, -1) == 1);
    assert(eat_sets_next_visible(&sets, 1) == -1);
    assert(eat_sets_any_filled(&sets));

    // 第 1、3 套也有内容后按顺序循环。
    assert(eat_sets_append(&sets, 0, "盖饭"));
    assert(eat_sets_append(&sets, 2, "火锅"));
    assert(eat_sets_next_visible(&sets, -1) == 0);
    assert(eat_sets_next_visible(&sets, 0) == 1);
    assert(eat_sets_next_visible(&sets, 1) == 2);
    assert(eat_sets_next_visible(&sets, 2) == -1);
}

static void test_meta_round_trip(void) {
    eat_sets_t source;
    eat_sets_reset(&source);
    eat_sets_set_name(&source, 0, "公司周边");
    assert(eat_sets_append(&source, 0, "拉面"));
    eat_sets_set_name(&source, 2, "周末");
    source.active = 2;

    uint8_t blob[EAT_SETS_META_SIZE];
    assert(eat_sets_serialize_meta(&source, blob, sizeof(blob)) == EAT_SETS_META_SIZE);

    eat_sets_t restored;
    eat_sets_reset(&restored);
    assert(eat_sets_deserialize_meta(&restored, blob, sizeof(blob)));
    assert(restored.active == 2);
    assert(strcmp(restored.sets[0].name, "公司周边") == 0);
    assert(strcmp(restored.sets[2].name, "周末") == 0);
    // meta 不含条目，条目由各自的 blob 负责。
    assert(restored.sets[0].items.count == 0);

    // 版本与长度不符都要拒绝。
    blob[0] ^= 0xFF;
    assert(!eat_sets_deserialize_meta(&restored, blob, sizeof(blob)));
    blob[0] ^= 0xFF;
    assert(!eat_sets_deserialize_meta(&restored, blob, sizeof(blob) - 1));
}

static void test_append_limits(void) {
    eat_sets_t sets;
    eat_sets_reset(&sets);
    assert(!eat_sets_append(&sets, -1, "拉面"));
    assert(!eat_sets_append(&sets, EAT_SET_COUNT, "拉面"));
    assert(eat_sets_append(&sets, 2, "麻辣烫"));
    assert(!eat_sets_append(&sets, 2, "麻辣烫"));   // 同套去重

    for (int i = 0; (int)sets.sets[2].items.count < EAT_SET_ITEMS_MAX; i++) {
        char buffer[12];
        snprintf(buffer, sizeof(buffer), "第%d家", i);
        assert(eat_sets_append(&sets, 2, buffer));
    }
    assert(sets.sets[2].items.count == EAT_SET_ITEMS_MAX);
    assert(!eat_sets_append(&sets, 2, "再来一家"));
    assert(sets.sets[1].items.count == 0);
}

static void test_set_name_truncation(void) {
    eat_sets_t sets;
    eat_sets_reset(&sets);
    eat_sets_set_name(&sets, 0, "海淀黄庄附近的店");
    assert(eat_utf8_chars(sets.sets[0].name) == 7);   // 最多 7 个字符

    eat_sets_set_name(&sets, 0, "");
    assert(strcmp(sets.sets[0].name, "我的1") == 0);  // 空名回到默认
}

static void test_view_follows_active_set(void) {
    eat_sets_t sets;
    eat_sets_reset(&sets);
    const char *seed_ptrs[3] = {"A", "B", "C"};
    eat_wheel_view_t view;

    // 无套：只有默认池。
    eat_sets_fill_view(&sets, seed_ptrs, 3, &view);
    assert(view.include_seed);
    assert(view.extra == NULL);
    assert(eat_wheel_view_count(&view) == 3);
    assert(eat_sets_wheel_count(&sets, 3) == 3);

    // 有内容的套：只转这一套，不再叠加默认池。
    sets.active = 0;
    assert(eat_sets_append(&sets, 0, "拉面"));
    eat_sets_fill_view(&sets, seed_ptrs, 3, &view);
    assert(!view.include_seed);
    assert(view.extra != NULL);
    assert(eat_wheel_view_count(&view) == 1);
    assert(strcmp(eat_wheel_view_name(&view, 0), "拉面") == 0);
    assert(eat_wheel_view_is_user(&view, 0));

    // 空的自定义套：回退到默认池，轮盘不会空转。
    eat_sets_reset(&sets);
    sets.active = 1;
    eat_sets_fill_view(&sets, seed_ptrs, 3, &view);
    assert(view.include_seed);
    assert(view.extra == NULL);
    assert(eat_wheel_view_count(&view) == 3);
}

int main(void) {
    test_reset_defaults();
    test_factory_defaults();
    test_next_active_cycles();
    test_next_visible_skips_empty_sets();
    test_meta_round_trip();
    test_append_limits();
    test_set_name_truncation();
    test_view_follows_active_set();
    printf("test_eat_sets: PASS\n");
    return 0;
}
