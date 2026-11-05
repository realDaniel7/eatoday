// main/eat_what_router.c —— App Router 状态机实现（不依赖 ESP-IDF / LVGL）。
#include "eat_what_router.h"

static void reset_add_state(eat_router_t *router) {
    router->cursor = 0;
    router->chosen = false;
    router->category = NULL;
    router->suffix = NULL;
}

static const char *candidate_at(const eat_router_t *router, uint8_t index) {
    if (router->candidates == NULL || router->candidates->count == 0) return NULL;
    return router->candidates->words[index % router->candidates->count];
}

static uint8_t move_cursor(const eat_router_t *router, int delta) {
    const size_t count = (router->candidates == NULL) ? 0 : router->candidates->count;
    if (count == 0) return 0;
    int next = (int)router->cursor + delta;
    const int span = (int)count;
    next %= span;
    if (next < 0) next += span;
    return (uint8_t)next;
}

void eat_router_init(eat_router_t *router, const eat_candidates_t *candidates) {
    if (router == NULL) return;
    router->page = EAT_PAGE_MAIN;
    router->candidates = candidates;
    reset_add_state(router);
}

void eat_router_enter_add(eat_router_t *router) {
    if (router == NULL) return;
    router->page = EAT_PAGE_ADD;
    reset_add_state(router);
}

void eat_router_leave_add(eat_router_t *router) {
    if (router == NULL) return;
    router->page = EAT_PAGE_MAIN;
    reset_add_state(router);
}

const char *eat_router_selected_word(const eat_router_t *router) {
    if (router == NULL) return NULL;
    if (router->category != NULL) return router->category;
    return candidate_at(router, router->cursor);
}

const char *eat_router_suffix(const eat_router_t *router) {
    return router == NULL ? NULL : router->suffix;
}

static eat_action_t route_main(eat_router_t *router, eat_key_t key, eat_key_ev_t event,
                               bool spinning) {
    // 主界面不需要改路由内部状态：页面切换由应用层在动作返回后完成。
    (void)router;
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
            if (event == EAT_KEY_LONG) return EAT_ACT_OPEN_ADD;
            return EAT_ACT_NONE;
        default:
            return EAT_ACT_NONE;
    }
}

static eat_action_t route_add(eat_router_t *router, eat_key_t key, eat_key_ev_t event) {
    switch (key) {
        case EAT_KEY_UP:
            if (event == EAT_KEY_CLICK) {
                router->cursor = move_cursor(router, -1);
                return EAT_ACT_ADD_MOVE_PREV;
            }
            if (event == EAT_KEY_LONG) return EAT_ACT_ADD_CANCEL;
            return EAT_ACT_NONE;
        case EAT_KEY_DOWN:
            if (event == EAT_KEY_CLICK) {
                router->cursor = move_cursor(router, 1);
                return EAT_ACT_ADD_MOVE_NEXT;
            }
            return EAT_ACT_NONE;
        case EAT_KEY_OK: {
            if (event == EAT_KEY_LONG) {
                // 长按保存：没选中过就直接用当前光标词，只存品类标签。
                if (eat_router_selected_word(router) == NULL) return EAT_ACT_NONE;
                return EAT_ACT_ADD_SAVE;
            }
            if (event == EAT_KEY_CLICK) {
                const char *word = candidate_at(router, router->cursor);
                if (word == NULL) return EAT_ACT_NONE;
                if (!router->chosen) {
                    router->chosen = true;
                    router->category = word;
                } else if (router->category != NULL && router->category == word) {
                    // 同一个词不重复追加。
                    router->suffix = NULL;
                } else {
                    router->suffix = word;
                }
                return EAT_ACT_ADD_CHOOSE;
            }
            return EAT_ACT_NONE;
        }
        default:
            return EAT_ACT_NONE;
    }
}

eat_action_t eat_router_key(eat_router_t *router, eat_key_t key, eat_key_ev_t event,
                            bool spinning) {
    if (router == NULL) return EAT_ACT_NONE;
    if (router->page == EAT_PAGE_ADD) return route_add(router, key, event);
    return route_main(router, key, event, spinning);
}
