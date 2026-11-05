// tests/test_eat_what_router.c —— App Router：旋转中/静止/添加页三条分支。
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "eat_what_router.h"
#include "eat_what_wheel.h"

static const char *const WORDS[] = {"拉面", "盖饭", "饺子", "烤鸭", "火锅"};
static const eat_candidates_t CANDIDATES = {
    .words = WORDS,
    .count = sizeof(WORDS) / sizeof(WORDS[0]),
};

static eat_router_t make_router(void) {
    eat_router_t router;
    eat_router_init(&router, &CANDIDATES);
    return router;
}

static void test_main_page_idle_branches(void) {
    eat_router_t router = make_router();
    assert(router.page == EAT_PAGE_MAIN);

    // 上键：短按无动作，长按起转。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, false) == EAT_ACT_SPIN_START);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_PRESS, false) == EAT_ACT_NONE);

    // 下键：短按下一个，长按切换范围。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_SELECT_NEXT);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_LONG, false) == EAT_ACT_SCOPE_TOGGLE);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_RELEASE, false) == EAT_ACT_NONE);

    // 确定键：短按接受，长按进添加页。
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_ACCEPT);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, false) == EAT_ACT_OPEN_ADD);
    // Router 只发动作，页面切换由应用层完成。
    assert(router.page == EAT_PAGE_MAIN);
}

static void test_main_page_spinning_branches(void) {
    eat_router_t router = make_router();

    // 旋转中：上键松开触发减速。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_RELEASE, true) == EAT_ACT_SPIN_RELEASE);
    // 旋转中：下键短按立即停止。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, true) == EAT_ACT_SPIN_STOP);
    // 旋转中其它手势一律忽略，避免误触改范围/接受结果/进添加页。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_LONG, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, true) == EAT_ACT_NONE);
}

static void test_add_page_branches(void) {
    eat_router_t router = make_router();
    eat_router_enter_add(&router);
    assert(router.page == EAT_PAGE_ADD);
    assert(router.cursor == 0);
    assert(eat_router_selected_word(&router) == WORDS[0]);

    // 上下键移动光标并对齐到目标词。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_NEXT);
    assert(router.cursor == 1);
    assert(strcmp(eat_router_selected_word(&router), "盖饭") == 0);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_PREV);
    assert(router.cursor == 0);
    // 越界回绕
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_PREV);
    assert(router.cursor == CANDIDATES.count - 1);

    // 短按确定选中当前词作为品类。
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_ADD_CHOOSE);
    assert(router.chosen);
    assert(strcmp(eat_router_selected_word(&router), "火锅") == 0);
    assert(eat_router_suffix(&router) == NULL);

    // 再选另一个词作为追加短名称。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_PREV);
    assert(strcmp(WORDS[router.cursor], "烤鸭") == 0);                // 光标落在烤鸭
    assert(strcmp(eat_router_selected_word(&router), "火锅") == 0);   // 品类仍是火锅
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_ADD_CHOOSE);
    assert(strcmp(eat_router_selected_word(&router), "火锅") == 0);   // 品类不变
    assert(strcmp(eat_router_suffix(&router), "烤鸭") == 0);

    // 光标回到“火锅”（品类本身）再短按：不重复追加，追加项清空。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_NEXT);
    assert(strcmp(WORDS[router.cursor], "火锅") == 0);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_ADD_CHOOSE);
    assert(eat_router_suffix(&router) == NULL);

    // 长按确定保存；此时状态仍在（应用层取完名字再离开添加页）。
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, false) == EAT_ACT_ADD_SAVE);
    assert(strcmp(eat_router_selected_word(&router), "火锅") == 0);

    eat_router_leave_add(&router);
    assert(router.page == EAT_PAGE_MAIN);
    assert(router.category == NULL);
    assert(router.suffix == NULL);
}

static void test_add_page_save_without_choosing(void) {
    eat_router_t router = make_router();
    eat_router_enter_add(&router);
    // 没短按选中过就直接长按保存：保存当前光标词，只存品类标签。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_NEXT);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, false) == EAT_ACT_ADD_SAVE);
    assert(strcmp(eat_router_selected_word(&router), "盖饭") == 0);
    assert(eat_router_suffix(&router) == NULL);
}

static void test_add_page_cancel_and_ignored_keys(void) {
    eat_router_t router = make_router();
    eat_router_enter_add(&router);
    // 添加页不响应轮盘手势。
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_PRESS, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_LONG, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_RELEASE, false) == EAT_ACT_NONE);
    // 上键长按退出添加页。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, false) == EAT_ACT_ADD_CANCEL);
    eat_router_leave_add(&router);
    assert(router.page == EAT_PAGE_MAIN);
}

static void test_empty_candidate_table(void) {
    const eat_candidates_t empty = { .words = NULL, .count = 0 };
    eat_router_t router;
    eat_router_init(&router, &empty);
    eat_router_enter_add(&router);
    assert(router.cursor == 0);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_ADD_MOVE_NEXT);
    assert(router.cursor == 0);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, false) == EAT_ACT_NONE);
    assert(eat_router_selected_word(&router) == NULL);
}

static void test_router_matches_wheel_state(void) {
    // 真实链路：轮盘转动时 Router 必须走“旋转中”分支，停稳后立刻回到静止分支。
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 10);
    eat_router_t router = make_router();

    eat_wheel_start(&wheel);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK,
                          eat_wheel_is_spinning(&wheel)) == EAT_ACT_SPIN_STOP);
    eat_wheel_stop_now(&wheel);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK,
                          eat_wheel_is_spinning(&wheel)) == EAT_ACT_SELECT_NEXT);
}

int main(void) {
    test_main_page_idle_branches();
    test_main_page_spinning_branches();
    test_add_page_branches();
    test_add_page_save_without_choosing();
    test_add_page_cancel_and_ignored_keys();
    test_empty_candidate_table();
    test_router_matches_wheel_state();
    printf("test_eat_what_router: PASS\n");
    return 0;
}
