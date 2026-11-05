// tests/test_eat_what_wheel.c —— 轮盘停止位置/索引计算（纯逻辑，主机可跑）。
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "eat_what_wheel.h"

static uint16_t spin_and_stop_index(uint16_t count, uint32_t dt_ms) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, count);
    eat_wheel_start(&wheel);
    for (int i = 0; i < 10; i++) {
        (void)eat_wheel_tick(&wheel, dt_ms);
    }
    eat_wheel_release(&wheel);
    int guard = 0;
    while (eat_wheel_is_spinning(&wheel) && guard++ < 5000) {
        (void)eat_wheel_tick(&wheel, dt_ms);
    }
    assert(!eat_wheel_is_spinning(&wheel));
    assert(wheel.speed_milli == 0);
    // 停稳后位置必须对齐到整项，索引才可能稳定。
    assert(wheel.pos_milli % EAT_WHEEL_MILLI == 0);
    return eat_wheel_index(&wheel);
}

static void test_stop_index_rounds_to_nearest(void) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 10);

    wheel.pos_milli = 0;
    assert(eat_wheel_index(&wheel) == 0);
    wheel.pos_milli = 499;
    assert(eat_wheel_index(&wheel) == 0);
    wheel.pos_milli = 500;
    assert(eat_wheel_index(&wheel) == 1);
    wheel.pos_milli = 1400;
    assert(eat_wheel_index(&wheel) == 1);
    wheel.pos_milli = 1600;
    assert(eat_wheel_index(&wheel) == 2);
    // 取模：9500 → round = 10 → 回绕到 0
    wheel.pos_milli = 9500;
    assert(eat_wheel_index(&wheel) == 0);
}

static void test_stop_now_snaps_to_nearest_item(void) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 7);
    eat_wheel_start(&wheel);
    wheel.pos_milli = 2600;
    eat_wheel_stop_now(&wheel);
    assert(!eat_wheel_is_spinning(&wheel));
    assert(wheel.pos_milli == 3000);
    assert(eat_wheel_index(&wheel) == 3);

    // 已经在减速中按下“下键短按”：同样立即吸附停止。
    eat_wheel_start(&wheel);
    eat_wheel_release(&wheel);
    wheel.pos_milli = 5400;
    eat_wheel_stop_now(&wheel);
    assert(!eat_wheel_is_spinning(&wheel));
    assert(eat_wheel_index(&wheel) == 5);
}

static void test_deceleration_is_deterministic_and_in_range(void) {
    const uint16_t first = spin_and_stop_index(30, 20);
    const uint16_t second = spin_and_stop_index(30, 20);
    assert(first == second);   // 同一组 dt 必须停在同一条
    assert(first < 30);

    // 不同的 dt 序列走出的距离不同，但结果仍然落在有效范围内。
    const uint16_t third = spin_and_stop_index(30, 7);
    assert(third < 30);
}

static void test_release_only_matters_while_spinning(void) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 5);
    eat_wheel_release(&wheel);   // 静止时松开：无副作用
    assert(!eat_wheel_is_spinning(&wheel));
    assert(!eat_wheel_tick(&wheel, 20));

    eat_wheel_start(&wheel);
    assert(eat_wheel_is_spinning(&wheel));
    eat_wheel_release(&wheel);
    assert(wheel.decelerating);
    // 减速期间位置继续前进（返回值只表示“索引是否变了”，20ms 内未必跨过一项）。
    int32_t before = wheel.pos_milli;
    int32_t speed_before = wheel.speed_milli;
    (void)eat_wheel_tick(&wheel, 20);
    assert(wheel.pos_milli != before);
    assert(wheel.speed_milli < speed_before);
}

static void test_manual_step_and_reset(void) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 4);
    eat_wheel_step(&wheel, 1);
    assert(eat_wheel_index(&wheel) == 1);
    eat_wheel_step(&wheel, 1);
    assert(eat_wheel_index(&wheel) == 2);
    eat_wheel_step(&wheel, -1);
    assert(eat_wheel_index(&wheel) == 1);
    // 回绕
    eat_wheel_step(&wheel, -1);
    eat_wheel_step(&wheel, -1);
    assert(eat_wheel_index(&wheel) == 3);
    eat_wheel_reset(&wheel);
    assert(eat_wheel_index(&wheel) == 0);
    assert(!eat_wheel_is_spinning(&wheel));
}

static void test_count_change_keeps_index_valid(void) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 30);
    eat_wheel_step(&wheel, 25);
    assert(eat_wheel_index(&wheel) == 25);
    // 切到“仅种子池”后条目变少：索引按取模落回范围内。
    eat_wheel_set_count(&wheel, 10);
    assert(eat_wheel_index(&wheel) == 5);
    assert(wheel.pos_milli == 5 * EAT_WHEEL_MILLI);
}

static void test_empty_wheel_is_safe(void) {
    eat_wheel_t wheel;
    eat_wheel_init(&wheel, 0);
    eat_wheel_start(&wheel);
    assert(!eat_wheel_is_spinning(&wheel));
    assert(!eat_wheel_tick(&wheel, 20));
    eat_wheel_step(&wheel, 1);
    eat_wheel_stop_now(&wheel);
    assert(eat_wheel_index(&wheel) == 0);
    assert(wheel.pos_milli == 0);
}

int main(void) {
    test_stop_index_rounds_to_nearest();
    test_stop_now_snaps_to_nearest_item();
    test_deceleration_is_deterministic_and_in_range();
    test_release_only_matters_while_spinning();
    test_manual_step_and_reset();
    test_count_change_keeps_index_valid();
    test_empty_wheel_is_safe();
    printf("test_eat_what_wheel: PASS\n");
    return 0;
}
