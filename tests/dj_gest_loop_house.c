/* SPDX-License-Identifier: GPL-3.0-only */
#if defined(GENRE_HOUSE)
#include "fm_dj_conf.h"
#endif
#include "fm_dj.h"
#include "fm_dj_gest.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_gest_loop_house: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    fm_dj_gest_env_t env;
    uint32_t seed;
    uint32_t loop_hits = 0u;
    uint32_t bars = 0u;
    uint8_t got_loop = 0u;

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

#if defined(GENRE_HOUSE)
#define LOOP_HOUSE_SEED_MAX 8192u
#else
#define LOOP_HOUSE_SEED_MAX 2048u
#endif
    for (seed = 1u; seed < LOOP_HOUSE_SEED_MAX; seed++) {
        uint8_t pb;

        fm_dj_enable(&dj, seed * 0x600du);
        env.loop_cd_bars = 0u;
        for (pb = 12u; pb < dj.build_end - 2u && pb < dj.break_arm - 1u; pb++) {
            uint8_t g;

            dj.phrase_bar = pb;
            fm_dj_gest_env_tick(&env);
            g = fm_dj_gest_plan_pick(&dj, pb, &env);
            bars++;
            if (g == FM_DJ_GEST_LOOP) {
                loop_hits++;
                got_loop = 1u;
                env.loop_cd_bars = FM_DJ_GEST_LOOP_CD_BARS;
                env.gest_active = 1u;
                env.loop_active = 1u;
                check(env.loop_cd_bars >= 24u, "loop cooldown length");
            }
            if (env.gest_active) {
                env.gest_active = 0u;
                env.loop_active = 0u;
            }
        }
    }
    check(got_loop, "loop still reachable");
    check(bars > 1000u, "simulated enough bars");
#if defined(GENRE_HOUSE)
    check(loop_hits <= 12u, "house loop rare over long run");
#else
    check(loop_hits <= 24u, "loop bounded");
#endif
    env.gest_active = 1u;
    check(fm_dj_gest_env_blocked(&dj, &env), "active gest blocks planner");
    return fail ? 1 : 0;
}
