// main/eat_what_router.c —— App Router 状态机实现（不依赖 ESP-IDF / LVGL）。
#include "eat_what_router.h"

void eat_router_init(eat_router_t *router) {
    if (router == NULL) return;
    router->page = EAT_PAGE_MAIN;
}

void eat_router_enter_web(eat_router_t *router) {
    if (router == NULL) return;
    router->page = EAT_PAGE_WEB;
}

void eat_router_leave_web(eat_router_t *router) {
    if (router == NULL) return;
    router->page = EAT_PAGE_MAIN;
}

static eat_action_t route_main(eat_key_t key, eat_key_ev_t event, bool spinning) {
    if (spinning) {
        // 旋转中只认两个手势：上键松开减速、下键短按立即停止。
        if (key == EAT_KEY_UP && event == EAT_KEY_RELEASE) return EAT_ACT_SPIN_RELEASE;
        if (key == EAT_KEY_DOWN && event == EAT_KEY_CLICK) return EAT_ACT_SPIN_STOP;
        return EAT_ACT_NONE;
    }

    switch (key) {
        case EAT_KEY_UP:
            if (event == EAT_KEY_LONG) return EAT_ACT_SPIN_START;
            return EAT_ACT_NONE;
        case EAT_KEY_DOWN:
            if (event == EAT_KEY_CLICK) return EAT_ACT_SELECT_NEXT;
            if (event == EAT_KEY_LONG) return EAT_ACT_SCOPE_TOGGLE;
            return EAT_ACT_NONE;
        case EAT_KEY_OK:
            if (event == EAT_KEY_CLICK) return EAT_ACT_ACCEPT;
            // 机器上不再做选词输入，长按确定直接开热点，用手机编辑列表。
            if (event == EAT_KEY_LONG) return EAT_ACT_OPEN_WEB;
            return EAT_ACT_NONE;
        default:
            return EAT_ACT_NONE;
    }
}

static eat_action_t route_web(eat_key_t key, eat_key_ev_t event) {
    // 后台页只认一个手势：长按下键退出（避免误触改列表）。
    if (key == EAT_KEY_DOWN && event == EAT_KEY_LONG) return EAT_ACT_CLOSE_WEB;
    return EAT_ACT_NONE;
}

eat_action_t eat_router_key(eat_router_t *router, eat_key_t key, eat_key_ev_t event,
                            bool spinning) {
    if (router == NULL) return EAT_ACT_NONE;
    if (router->page == EAT_PAGE_WEB) return route_web(key, event);
    return route_main(key, event, spinning);
}
