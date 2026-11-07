// tests/test_eat_what_router.c —— App Router：旋转中/静止/后台页三条分支。
#include <assert.h>
#include <stdio.h>

#include "eat_what_router.h"
#include "eat_what_wheel.h"

static void test_main_page_idle_branches(void) {
    eat_router_t router;
    eat_router_init(&router);
    assert(router.page == EAT_PAGE_MAIN);

    // 上键：短按无动作，长按起转。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, false) == EAT_ACT_SPIN_START);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_PRESS, false) == EAT_ACT_NONE);

    // 下键：短按下一个，长按切换列表。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_SELECT_NEXT);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_LONG, false) == EAT_ACT_SCOPE_TOGGLE);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_RELEASE, false) == EAT_ACT_NONE);

    // 确定键：短按接受，长按直接打开手机后台（机器上不再有选词页）。
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_ACCEPT);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, false) == EAT_ACT_OPEN_WEB);
    // Router 只发动作，页面切换由应用层完成。
    assert(router.page == EAT_PAGE_MAIN);
}

static void test_main_page_spinning_branches(void) {
    eat_router_t router;
    eat_router_init(&router);

    // 旋转中：上键松开触发减速。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_RELEASE, true) == EAT_ACT_SPIN_RELEASE);
    // 旋转中：下键短按立即停止。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, true) == EAT_ACT_SPIN_STOP);
    // 旋转中其它手势一律忽略，避免误触改列表/接受结果/开热点。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_CLICK, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_LONG, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, true) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, true) == EAT_ACT_NONE);
}

static void test_web_page_branches(void) {
    eat_router_t router;
    eat_router_init(&router);
    eat_router_enter_web(&router);
    assert(router.page == EAT_PAGE_WEB);
    // 后台页只认“长按下键退出”，其余手势忽略，避免误改列表。
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_LONG, false) == EAT_ACT_CLOSE_WEB);
    assert(eat_router_key(&router, EAT_KEY_DOWN, EAT_KEY_CLICK, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_RELEASE, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_CLICK, false) == EAT_ACT_NONE);
    assert(eat_router_key(&router, EAT_KEY_OK, EAT_KEY_LONG, false) == EAT_ACT_NONE);
    eat_router_leave_web(&router);
    assert(router.page == EAT_PAGE_MAIN);
    // 回到主界面后手势恢复。
    assert(eat_router_key(&router, EAT_KEY_UP, EAT_KEY_LONG, false) == EAT_ACT_SPIN_START);
}

static void test_router_matches_wheel_state(void) {
    // 真实链路：轮盘转动时 Router 必须走“旋转中”分支，停稳后立刻回到静止分支。
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 10);
    eat_router_t router;
    eat_router_init(&router);

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
    test_web_page_branches();
    test_router_matches_wheel_state();
    printf("test_eat_what_router: PASS\n");
    return 0;
}
