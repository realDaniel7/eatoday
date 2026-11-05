// tests/test_eat_what_seed.c —— 种子池数据完整性（30 项、分品类、无重复）。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "eat_what_pool.h"
#include "eat_what_seed.h"

#define EXPECTED_SEED_COUNT 30

static void test_count_and_categories(void) {
    assert(EAT_SEED_POOL_COUNT == EXPECTED_SEED_COUNT);

    bool seen[EAT_CAT_COUNT] = {false};
    for (size_t i = 0; i < EAT_SEED_POOL_COUNT; i++) {
        const eat_seed_t *seed = &EAT_SEED_POOL[i];
        assert(seed->name != NULL);
        assert(seed->name[0] != '\0');
        // 种子名要能塞进定长记录（NVS 快照与用户池共用同一布局）。
        assert(strlen(seed->name) < EAT_NAME_MAX_BYTES);
        assert((int)seed->category >= 0 && seed->category < EAT_CAT_COUNT);
        seen[seed->category] = true;
    }
    // 七个品类都必须有内容，避免某一类名存实亡。
    for (int category = 0; category < EAT_CAT_COUNT; category++) {
        assert(seen[category]);
        assert(eat_category_label((eat_category_t)category)[0] != '\0');
    }
    assert(strcmp(eat_category_label((eat_category_t)EAT_CAT_COUNT), "") == 0);
}

static void test_no_duplicates(void) {
    for (size_t i = 0; i < EAT_SEED_POOL_COUNT; i++) {
        for (size_t j = i + 1; j < EAT_SEED_POOL_COUNT; j++) {
            assert(strcmp(EAT_SEED_POOL[i].name, EAT_SEED_POOL[j].name) != 0);
        }
    }
}

static void test_named_entries_are_present(void) {
    // 需求里点名要求的条目必须在池子里。
    static const char *const REQUIRED[] = {
        "四季民福", "便宜坊", "大鸭梨", "紫光园",
        "庆丰包子铺", "方砖厂69号炸酱面", "嘉和一品", "西部马华",
        "东来顺", "呷哺呷哺",
        "胡大饭馆", "龙人居水煮鱼", "江边城外",
        "小吊梨汤", "局气", "旺顺阁",
        "刘妈妈肉汁饺子", "明园餐厅", "湘彬萱",
        "麦当劳", "肯德基", "汉堡王", "吉野家", "和合谷", "永和大王",
    };
    for (size_t r = 0; r < sizeof(REQUIRED) / sizeof(REQUIRED[0]); r++) {
        bool found = false;
        for (size_t i = 0; i < EAT_SEED_POOL_COUNT; i++) {
            if (strcmp(EAT_SEED_POOL[i].name, REQUIRED[r]) == 0) {
                found = true;
                break;
            }
        }
        assert(found);
    }
}

static void test_fill_list_matches_table(void) {
    eat_name_list_t list;
    eat_seed_fill_list(&list);
    assert(list.count == EAT_SEED_POOL_COUNT);
    for (size_t i = 0; i < EAT_SEED_POOL_COUNT; i++) {
        assert(strcmp(list.names[i], EAT_SEED_POOL[i].name) == 0);
    }
}

int main(void) {
    test_count_and_categories();
    test_no_duplicates();
    test_named_entries_are_present();
    test_fill_list_matches_table();
    printf("test_eat_what_seed: PASS\n");
    return 0;
}
