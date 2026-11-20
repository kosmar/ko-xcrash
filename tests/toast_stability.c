/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj.h"
#include "perf_toast_policy.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "toast_stability: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    perf_toast_lane_t main_lane;
    uint32_t frame;
    uint32_t t0 = 1000u;
    fm_dj_t dj;
    fm_dj_toast_tgt_hold_t hold;

    fail = 0;
    memset(&main_lane, 0, sizeof main_lane);

    for (frame = 0u; frame < 400u; frame++) {
        uint32_t now = t0 + frame * 16u;
        uint8_t kind = (frame & 1u) ? 2u : 1u;
        uint32_t start_before = main_lane.on ? main_lane.start_ms : 0u;

        perf_toast_policy_lane_set_main(&main_lane, "SKANK", kind, -1, 0u, now, 0, 0u);
        check(main_lane.on, "SKANK lane occupied");
        if (main_lane.on) {
            check(main_lane.start_ms == start_before || start_before == 0u, "SKANK start_ms stable");
            check(perf_toast_policy_same_msg(main_lane.msg, "SKANK"), "SKANK identity");
        }
    }

    fm_dj_enable(&dj, 0x5151u);
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_MOTION, 127, 2u, 8u, 8u);
    fm_dj_slot_begin(&dj, 1u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_FLUX, 127, 2u, 1u, 2u);
    {
        uint8_t i;

        for (i = 0u; i < 24u; i++)
            fm_dj_tick(&dj, (uint8_t)(i % 16u), 16u);
    }
    fm_dj_toast_tgt_hold_reset(&hold);
    for (frame = 0u; frame < 60u; frame++) {
        uint8_t tgt = fm_dj_toast_target_stable(&dj, &hold);

        if (frame > 5u)
            check(tgt == hold.tgt, "hysteresis holds DJ target under jitter");
    }

    printf("toast_stability: OK\n");
    return fail ? 1 : 0;
}
