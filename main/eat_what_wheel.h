// main/eat_what_wheel.h —— 轮盘旋转/减速/停止的位置与索引计算（纯逻辑，可主机测试）。
//
// 位置用 milli-index（1 个条目 = 1000）表示，速度单位是 milli-index/毫秒，全程整数运算，
// 因此“松开后停在哪一项”在同一组 dt 序列下完全可复现，主机测试可以逐 tick 断言。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define EAT_WHEEL_MILLI 1000

// 起转速度：40 项/秒。要的是“刷刷刷”的滚动手感。
#define EAT_WHEEL_SPIN_SPEED 40000
// 减速时间常数（ms）：松开后约 0.6~0.8 秒停稳。
#define EAT_WHEEL_DECAY_TC_MS 210
// 低于该速度（不足 1 项/秒）直接吸附停止，避免长时间爬行。
#define EAT_WHEEL_MIN_SPEED 1100

typedef struct {
    uint16_t count;        // 条目数；0 表示空池
    int32_t pos_milli;     // 已归一化到 [0, count * EAT_WHEEL_MILLI)
    int32_t speed_milli;   // 每毫秒推进的 milli-index
    bool spinning;         // 正在旋转（含减速阶段）
    bool decelerating;     // 已松开上键，正在减速
} eat_wheel_t;

void eat_wheel_init(eat_wheel_t *wheel, uint16_t count);
// 切换轮盘范围后条目数变化：保持当前索引不越界，速度与旋转状态不变。
void eat_wheel_set_count(eat_wheel_t *wheel, uint16_t count);

// 把当前位置随机落到某一项（开机首次显示、起转瞬间用）。
// 随机数由调用方给，纯逻辑里不依赖硬件 RNG，便于主机测试。
void eat_wheel_randomize(eat_wheel_t *wheel, uint32_t random_value);

// 上键长按：起转（从对齐位置开始，清掉残留减速状态）。
void eat_wheel_start(eat_wheel_t *wheel);
// 上键松开：进入减速；未在旋转时无副作用。
void eat_wheel_release(eat_wheel_t *wheel);
// 推进 dt 毫秒。返回 true 表示当前条目索引发生了变化（UI 需要重绘）。
bool eat_wheel_tick(eat_wheel_t *wheel, uint32_t dt_ms);
// 下键短按：立即吸附到最近条目并停止。
void eat_wheel_stop_now(eat_wheel_t *wheel);
// 手动移动 delta 个条目（±1），会先吸附到整项。
void eat_wheel_step(eat_wheel_t *wheel, int delta);
// 回到第一个条目（种子池开头）。
void eat_wheel_reset(eat_wheel_t *wheel);

// 当前条目索引；count 为 0 时返回 0。
uint16_t eat_wheel_index(const eat_wheel_t *wheel);
bool eat_wheel_is_spinning(const eat_wheel_t *wheel);
