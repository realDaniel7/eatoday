// main/eat_what_router.h —— 应用层按键路由（App Router）。
//
// 全局按键语义只在这里定义：页面不各自处理按键，只实现“动作”。路由输出 eat_action_t，
// 由 main.c 执行（起转/减速/换池/保存…）。旋转变由轮盘（eat_wheel）持有，
// 路由通过参数读取，避免出现两份“是否在转”的状态。
//
// 交互契约：
//   主界面静止：上长按=起转  下短按=下一个  下长按=切换范围  确定短按=接受  确定长按=添加页
//   主界面旋转：上松开=减速  下短按=立即停止
//   添加页：    上/下短按=切候选  上长按=取消返回  确定短按=选中  确定长按=保存
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    EAT_PAGE_ADD,
} eat_page_t;

typedef enum {
    EAT_ACT_NONE = 0,
    EAT_ACT_SPIN_START,
    EAT_ACT_SPIN_RELEASE,
    EAT_ACT_SPIN_STOP,
    EAT_ACT_SELECT_NEXT,
    EAT_ACT_SCOPE_TOGGLE,
    EAT_ACT_ACCEPT,
    EAT_ACT_OPEN_ADD,
    EAT_ACT_ADD_MOVE_PREV,
    EAT_ACT_ADD_MOVE_NEXT,
    EAT_ACT_ADD_CHOOSE,
    EAT_ACT_ADD_SAVE,
    EAT_ACT_ADD_CANCEL,
} eat_action_t;

// 添加页的候选词表（“高频关键词选择”），由应用层提供常量表。
typedef struct {
    const char *const *words;
    size_t count;
} eat_candidates_t;

typedef struct {
    eat_page_t page;
    const eat_candidates_t *candidates;
    uint8_t cursor;         // 候选词光标
    bool chosen;            // 是否已短按选中品类
    const char *category;   // 已选品类词
    const char *suffix;     // 可选的追加短名称
} eat_router_t;

void eat_router_init(eat_router_t *router, const eat_candidates_t *candidates);
// 进入/离开添加页时复位输入状态；page 由路由内部切换。
void eat_router_enter_add(eat_router_t *router);
void eat_router_leave_add(eat_router_t *router);

// 分发一次按键事件。spinning 为当前轮盘是否仍在旋转（含减速）。
eat_action_t eat_router_key(eat_router_t *router, eat_key_t key, eat_key_ev_t event,
                            bool spinning);

// 保存时使用的品类词：未选中时取当前光标词，候选表为空返回 NULL。
const char *eat_router_selected_word(const eat_router_t *router);
const char *eat_router_suffix(const eat_router_t *router);
