/* SPDX-License-Identifier: GPL-3.0-only */
#include "perf_toast_policy.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "toast_lane_y: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    perf_toast_lane_t lanes[PERF_TOAST_LANE_N];
    int32_t by_center = 50;
    int32_t bh = 10;
    uint32_t t0 = 8000u;
    uint32_t frame;
    uint32_t dj_start = 0u;

    fail = 0;
    memset(lanes, 0, sizeof lanes);

    check(perf_toast_policy_lane_by(PERF_TOAST_LANE_MAIN, by_center, bh) == by_center, "MAIN Y at center");
    check(perf_toast_policy_lane_by(PERF_TOAST_LANE_PROC, by_center, bh) == by_center - bh, "PROC Y above MAIN");
    check(perf_toast_policy_lane_by(PERF_TOAST_LANE_EPH, by_center, bh) == by_center - 2 * bh, "EPH Y top row");

    for (frame = 0u; frame < 120u; frame++) {
        uint32_t now = t0 + frame * 16u;
        perf_toast_lane_t *main = &lanes[PERF_TOAST_LANE_MAIN];
        perf_toast_lane_t *proc = &lanes[PERF_TOAST_LANE_PROC];
        perf_toast_lane_t *eph = &lanes[PERF_TOAST_LANE_EPH];
        static const char *const AN[5] = {"SKANK", "CHOP", "DOT", "ROLL", "LOOP"};

        if ((frame % 40u) == 0u)
            perf_toast_policy_lane_set_proc(proc, "CROSSFADE", 1u, 0u, now);
        else if ((frame % 40u) == 20u)
            perf_toast_policy_lane_clear(proc);

        perf_toast_policy_lane_set_main(main, "DJ", 2u, -1, 0u, now, 0, 0u);
        if (frame >= 60u && (frame % 30u) == 0u)
            perf_toast_policy_lane_set_eph(eph, "FILL", 1u, 2, 0u, now);
        else if (frame >= 60u && (frame % 30u) == 15u)
            perf_toast_policy_lane_clear(eph);

        check(perf_toast_policy_lane_overlay_targets_eph(main->on, main->msg, AN, 5u), "DJ sticky -> overlay eph");

        if (main->on && perf_toast_policy_same_msg(main->msg, "DJ")) {
            if (dj_start == 0u)
                dj_start = main->start_ms;
            else
                check(main->start_ms == dj_start, "DJ start_ms stable across proc toggles");
        }
    }

    printf("toast_lane_y: OK\n");
    return fail ? 1 : 0;
}
