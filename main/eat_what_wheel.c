// main/eat_what_wheel.c —— 轮盘位置推进与停止吸附。
#include "eat_what_wheel.h"

#include <stddef.h>

static int32_t normalize_pos(int32_t pos_milli, uint16_t count) {
    if (count == 0) return 0;
    const int32_t span = (int32_t)count * EAT_WHEEL_MILLI;
    int32_t pos = pos_milli % span;
    if (pos < 0) pos += span;
    return pos;
}

// 吸附到最近的条目：四舍五入后取模。这就是“停止后的索引计算”的唯一实现，
// 手动步进、立即停止、减速自然停三种路径共用它，避免出现两套取整规则。
static void snap(eat_wheel_t *wheel) {
    if (wheel->count == 0) {
        wheel->pos_milli = 0;
        return;
    }
    const int32_t index = (wheel->pos_milli + EAT_WHEEL_MILLI / 2) / EAT_WHEEL_MILLI;
    wheel->pos_milli = (int32_t)(index % (int32_t)wheel->count) * EAT_WHEEL_MILLI;
    if (wheel->pos_milli < 0) wheel->pos_milli += (int32_t)wheel->count * EAT_WHEEL_MILLI;
}

void eat_wheel_init(eat_wheel_t *wheel, uint16_t count) {
    if (wheel == NULL) return;
    wheel->count = count;
    wheel->pos_milli = 0;
    wheel->speed_milli = 0;
    wheel->spinning = false;
    wheel->decelerating = false;
}

void eat_wheel_set_count(eat_wheel_t *wheel, uint16_t count) {
    if (wheel == NULL) return;
    wheel->count = count;
    snap(wheel);
    if (count == 0) {
        wheel->spinning = false;
        wheel->decelerating = false;
        wheel->speed_milli = 0;
    }
}

void eat_wheel_start(eat_wheel_t *wheel) {
    if (wheel == NULL || wheel->count == 0) return;
    snap(wheel);
    wheel->speed_milli = EAT_WHEEL_SPIN_SPEED;
    wheel->spinning = true;
    wheel->decelerating = false;
}

void eat_wheel_release(eat_wheel_t *wheel) {
    if (wheel == NULL || !wheel->spinning) return;
    wheel->decelerating = true;
}

bool eat_wheel_tick(eat_wheel_t *wheel, uint32_t dt_ms) {
    if (wheel == NULL || !wheel->spinning || wheel->count == 0) return false;
    if (dt_ms == 0) return false;

    const uint16_t before = eat_wheel_index(wheel);

    if (wheel->decelerating) {
        // 每毫秒按时间常数衰减：v -= v * dt / TC，整数运算保证可复现。
        int32_t drop = (int32_t)(((int64_t)wheel->speed_milli * (int64_t)dt_ms) /
                                 EAT_WHEEL_DECAY_TC_MS);
        if (drop < 1) drop = 1;  // 保证速度单调下降，不会卡在最小值上方
        wheel->speed_milli -= drop;
        if (wheel->speed_milli < EAT_WHEEL_MIN_SPEED) {
            wheel->speed_milli = 0;
            wheel->spinning = false;
            wheel->decelerating = false;
            snap(wheel);
            return eat_wheel_index(wheel) != before;
        }
    }

    const int64_t advance = ((int64_t)wheel->speed_milli * (int64_t)dt_ms) / 1000;
    wheel->pos_milli = normalize_pos(wheel->pos_milli + (int32_t)advance, wheel->count);
    return eat_wheel_index(wheel) != before;
}

void eat_wheel_stop_now(eat_wheel_t *wheel) {
    if (wheel == NULL) return;
    wheel->speed_milli = 0;
    wheel->spinning = false;
    wheel->decelerating = false;
    snap(wheel);
}

void eat_wheel_step(eat_wheel_t *wheel, int delta) {
    if (wheel == NULL || wheel->count == 0) return;
    snap(wheel);
    const int32_t span = (int32_t)wheel->count * EAT_WHEEL_MILLI;
    wheel->pos_milli = normalize_pos(wheel->pos_milli + delta * EAT_WHEEL_MILLI, wheel->count);
    if (wheel->pos_milli >= span) wheel->pos_milli -= span;
}

void eat_wheel_reset(eat_wheel_t *wheel) {
    if (wheel == NULL) return;
    wheel->pos_milli = 0;
    wheel->speed_milli = 0;
    wheel->spinning = false;
    wheel->decelerating = false;
}

uint16_t eat_wheel_index(const eat_wheel_t *wheel) {
    if (wheel == NULL || wheel->count == 0) return 0;
    int32_t index = (wheel->pos_milli + EAT_WHEEL_MILLI / 2) / EAT_WHEEL_MILLI;
    index %= (int32_t)wheel->count;
    if (index < 0) index += (int32_t)wheel->count;
    return (uint16_t)index;
}

bool eat_wheel_is_spinning(const eat_wheel_t *wheel) {
    return wheel != NULL && wheel->spinning;
}
