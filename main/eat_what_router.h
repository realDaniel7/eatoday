// main/eat_what_router.h —— 应用层按键路由（App Router）。
//
// 全局按键语义只在这里定义：页面不各自处理按键，只实现“动作”。路由输出 eat_action_t，
// 由 main.c 执行（起转/减速/换范围/开热点…）。旋转变由轮盘（eat_wheel）持有，
// 路由通过参数读取，避免出现两份“是否在转”的状态。
//
// 交互契约：
//   主界面静止：上长按=起转  下短按=下一个  下长按=切换列表（默认池→我的1..3→默认池）
//               确定短按=接受  确定长按=打开手机设置后台
//   主界面旋转：上松开=减速  下短按=立即停止
//   后台页：    下长按=退出（其余手势忽略）
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    EAT_KEY_UP = 0,
    EAT_KEY_DOWN,
    EAT_KEY_OK,
} eat_key_t;

typedef enum {
    EAT_KEY_PRESS = 0,
    EAT_KEY_CLICK,
    EAT_KEY_LONG,
    EAT_KEY_RELEASE,
} eat_key_ev_t;

typedef enum {
    EAT_PAGE_MAIN = 0,
    EAT_PAGE_WEB,
} eat_page_t;

typedef enum {
    EAT_ACT_NONE = 0,
    EAT_ACT_SPIN_START,
    EAT_ACT_SPIN_RELEASE,
    EAT_ACT_SPIN_STOP,
    EAT_ACT_SELECT_NEXT,
    EAT_ACT_SCOPE_TOGGLE,
    EAT_ACT_ACCEPT,
    EAT_ACT_OPEN_WEB,
    EAT_ACT_CLOSE_WEB,
} eat_action_t;

typedef struct {
    eat_page_t page;
} eat_router_t;

void eat_router_init(eat_router_t *router);
void eat_router_enter_web(eat_router_t *router);
void eat_router_leave_web(eat_router_t *router);

// 分发一次按键事件。spinning 为当前轮盘是否仍在旋转（含减速）。
eat_action_t eat_router_key(eat_router_t *router, eat_key_t key, eat_key_ev_t event,
                            bool spinning);
