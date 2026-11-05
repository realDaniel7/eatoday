// tests/test_eat_what_pool.c —— 用户池追加、blob 往返、轮盘视图（纯逻辑）。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "eat_what_pool.h"
#include "eat_what_seed.h"

static void test_append_rules(void) {
    eat_name_list_t list;
    eat_name_list_reset(&list);
    assert(list.count == 0);

    assert(eat_name_list_append(&list, "火锅"));
    assert(list.count == 1);
    assert(strcmp(list.names[0], "火锅") == 0);

    // 池内同名不重复追加。
    assert(!eat_name_list_append(&list, "火锅"));
    assert(list.count == 1);

    // 空名与超长名都拒收（记录里连 NUL 一起算，所以最长 31 字节）。
    assert(!eat_name_list_append(&list, ""));
    char fits[EAT_NAME_MAX_BYTES];
    memset(fits, 'a', sizeof(fits) - 1);
    fits[sizeof(fits) - 1] = '\0';
    assert(eat_name_list_append(&list, fits));
    char too_long[EAT_NAME_MAX_BYTES + 1];
    memset(too_long, 'b', sizeof(too_long) - 1);
    too_long[sizeof(too_long) - 1] = '\0';
    assert(!eat_name_list_append(&list, too_long));
    assert(list.count == 2);

    // 满额后不再增长。
    for (int i = 0; list.count < EAT_USER_MAX; i++) {
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "w%d", i);
        assert(eat_name_list_append(&list, buffer));
    }
    assert(list.count == EAT_USER_MAX);
    assert(!eat_name_list_append(&list, "再来一个"));
    assert(list.count == EAT_USER_MAX);
    assert(eat_name_list_contains(&list, "火锅"));
    assert(!eat_name_list_contains(&list, "拉面"));
}

static void test_utf8_helpers(void) {
    assert(eat_utf8_chars("火锅") == 2);
    assert(eat_utf8_chars("abc") == 3);
    assert(eat_utf8_chars("") == 0);
    assert(eat_utf8_chars("火锅·拉面") == 5);

    char buffer[EAT_NAME_MAX_BYTES];
    assert(eat_utf8_truncate(buffer, sizeof(buffer), "方砖厂69号炸酱面", 4) == strlen("方砖厂6"));
    assert(strcmp(buffer, "方砖厂6") == 0);
    assert(eat_utf8_truncate(buffer, sizeof(buffer), "火锅", 6) == strlen("火锅"));
    assert(strcmp(buffer, "火锅") == 0);

    // 容量不足时宁可不写，也不留下半个汉字（"火锅" = 3 + 3 字节 + NUL）。
    char small[4];
    eat_utf8_truncate(small, sizeof(small), "火锅", 6);
    assert(strcmp(small, "火") == 0);
}

static void test_compose_name(void) {
    char buffer[EAT_NAME_MAX_BYTES];
    assert(eat_compose_name(buffer, sizeof(buffer), "火锅", "东来顺") ==
           strlen("火锅·东来顺"));
    assert(strcmp(buffer, "火锅·东来顺") == 0);

    // 同词不重复拼接，缺项时退化为单词。
    assert(eat_compose_name(buffer, sizeof(buffer), "火锅", "火锅") == strlen("火锅"));
    assert(strcmp(buffer, "火锅") == 0);
    assert(eat_compose_name(buffer, sizeof(buffer), "", "拉面") == strlen("拉面"));
    assert(strcmp(buffer, "拉面") == 0);
    assert(eat_compose_name(buffer, sizeof(buffer), "拉面", NULL) == strlen("拉面"));
    assert(eat_compose_name(buffer, sizeof(buffer), "", "") == 0);

    // 缓冲区放不下就失败，且不写出半个字符。
    char tiny[6];
    assert(eat_compose_name(tiny, sizeof(tiny), "火锅", "东来顺") == 0);
    assert(tiny[0] == '\0');
}

static void test_blob_round_trip(void) {
    eat_name_list_t source;
    eat_name_list_t restored;
    eat_name_list_reset(&source);
    eat_name_list_reset(&restored);
    assert(eat_name_list_append(&source, "火锅"));
    assert(eat_name_list_append(&source, "★饺子"));

    uint8_t blob[EAT_BLOB_MAX];
    const size_t length = eat_pool_serialize(&source, blob, sizeof(blob));
    assert(length == EAT_BLOB_HEADER + 2u * EAT_NAME_MAX_BYTES);
    assert(eat_pool_blob_size(&source) == length);

    assert(eat_pool_deserialize(&restored, blob, length));
    assert(restored.count == 2);
    assert(strcmp(restored.names[0], "火锅") == 0);
    assert(strcmp(restored.names[1], "★饺子") == 0);

    // 容量不足时不写入。
    assert(eat_pool_serialize(&source, blob, length - 1) == 0);

    // 版本不符与长度不符都被拒绝，并清空目标池。
    blob[0] ^= 0xFFu;
    assert(!eat_pool_deserialize(&restored, blob, length));
    assert(restored.count == 0);
    blob[0] ^= 0xFFu;
    assert(!eat_pool_deserialize(&restored, blob, length - 1));
    assert(restored.count == 0);

    // 空池也能往返。
    eat_name_list_t empty;
    eat_name_list_reset(&empty);
    const size_t empty_length = eat_pool_serialize(&empty, blob, sizeof(blob));
    assert(empty_length == EAT_BLOB_HEADER);
    assert(eat_pool_deserialize(&restored, blob, empty_length));
    assert(restored.count == 0);
}

static void test_wheel_view_scopes(void) {
    eat_name_list_t seed;
    eat_seed_fill_list(&seed);
    assert(seed.count == EAT_SEED_POOL_COUNT);

    const char *seed_ptrs[EAT_USER_MAX];
    assert(eat_name_list_pointers(&seed, seed_ptrs, EAT_USER_MAX) == seed.count);

    eat_name_list_t user;
    eat_name_list_reset(&user);
    assert(eat_name_list_append(&user, "★火锅"));

    eat_wheel_view_t view = {
        .seed_names = seed_ptrs,
        .seed_count = seed.count,
        .user = &user,
        .scope = EAT_SCOPE_ALL,
    };
    assert(eat_wheel_view_count(&view) == seed.count + 1);
    assert(strcmp(eat_wheel_view_name(&view, seed.count), "★火锅") == 0);
    assert(eat_wheel_view_is_user(&view, seed.count));
    assert(!eat_wheel_view_is_user(&view, 0));
    // 越界索引按取模处理，和轮盘保持一致。
    assert(strcmp(eat_wheel_view_name(&view, seed.count + 1), seed_ptrs[0]) == 0);

    view.scope = EAT_SCOPE_SEED_ONLY;
    assert(eat_wheel_view_count(&view) == seed.count);
    assert(!eat_wheel_view_is_user(&view, seed.count));
    assert(strcmp(eat_wheel_view_name(&view, seed.count), seed_ptrs[0]) == 0);

    // 空种子池不应崩溃。
    eat_wheel_view_t empty = { .seed_names = NULL, .seed_count = 0, .user = &user,
                               .scope = EAT_SCOPE_ALL };
    assert(eat_wheel_view_count(&empty) == 0);
    assert(eat_wheel_view_name(&empty, 0) == NULL);
    assert(!eat_wheel_view_is_user(&empty, 0));
}

int main(void) {
    test_append_rules();
    test_utf8_helpers();
    test_compose_name();
    test_blob_round_trip();
    test_wheel_view_scopes();
    printf("test_eat_what_pool: PASS\n");
    return 0;
}
