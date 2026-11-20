/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj.h"
#include "fm_dj_gest.h"
#include <stdio.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_gest_scheduler: %s\n", msg);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    fm_dj_gest_env_t env;
    uint8_t got = 0u;
    uint8_t pb;
    uint8_t k;

    fail = 0;
    env.caps = fm_dj_gest_caps_default();
    env.manual_gesture_latch = 0u;
    env.manual_arp_mode = 0u;
    env.loop_active = 0u;
    env.loop_cd_bars = 0u;
    env.arp_cd_bars = 0u;
    env.gest_active = 0u;
    env.fill_build_live = 0u;
    env.playing = 1u;
    env.flam_cd_bars = 0u;
    for (k = 1u; k < 128u && !got; k++) {
        fm_dj_enable(&dj, k * 0x9e3779b9u);
        for (pb = 2u; pb < dj.break_arm - 1u; pb++) {
            uint8_t g;

            dj.phrase_bar = pb;
            g = fm_dj_gest_plan_pick(&dj, pb, &env);
            if (g == FM_DJ_GEST_NONE)
                continue;
            got = g;
            check(fm_dj_gest_plan_bars(g, 2u) > 0u || g == FM_DJ_GEST_FLAM, "bounded duration");
            if (g == FM_DJ_GEST_SOLO_BASS || g == FM_DJ_GEST_SOLO_DRUM)
                check(fm_dj_gest_plan_bars(g, 1u) <= 4u, "solo max 4 bars");
            if (g == FM_DJ_GEST_ARP)
                check(fm_dj_gest_plan_arp_mode(0u) >= 1u, "arp mode valid");
            break;
        }
    }
    check(got != 0u, "scheduler yields a gesture in phrase");
    fm_dj_enable(&dj, 0xfaceu);
    env.manual_gesture_latch = 3u;
    check(fm_dj_gest_plan_pick(&dj, 8u, &env) == FM_DJ_GEST_NONE, "manual latch blocks DJ");
    env.manual_gesture_latch = 0u;
    dj.phrase_bar = dj.break_arm;
    check(fm_dj_gest_env_blocked(&dj, &env), "break bar blocked");
    for (k = 0u; k < 32u; k++) {
        uint8_t a = fm_dj_gest_plan_arp_mode(k);
        check(a >= 1u && a <= 5u, "arp mode in range");
    }
    return fail ? 1 : 0;
}
