/* SPDX-License-Identifier: GPL-3.0-only */
#if defined(GENRE_JAZZ)
#include "fm_dj_conf.h"
#endif
#include "fm_dj.h"
#include "fm_dj_gest.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_gest_lifecycle: %s\n", m);
        fail = 1;
    }
}

static uint8_t pick_until(fm_dj_t *dj, uint8_t want, fm_dj_gest_env_t *env, uint8_t force_role)
{
    uint32_t seed;

    for (seed = 1u; seed < 8192u; seed++) {
        uint8_t pb;

        fm_dj_enable(dj, seed * 0x9e3779b9u);
        if (force_role < FM_DJ_ROLE_N)
            dj->role = force_role;
        for (pb = 2u; pb < dj->break_arm - 1u; pb++) {
            uint8_t i;
            uint8_t g;

            dj->phrase_bar = pb;
            for (i = 0u; i < 64u; i++) {
                g = fm_dj_gest_plan_pick(dj, pb, env);
                if (g == want)
                    return g;
            }
        }
    }
    return FM_DJ_GEST_NONE;
}

int main(void)
{
    fm_dj_t dj;
    fm_dj_gest_env_t env;
    uint8_t g;

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
    fm_dj_enable(&dj, 0x4242u);
#if defined(GENRE_JAZZ)
    check((env.caps & 8u) == 0u, "jazz: no fill cap");
    check((env.caps & 16u) == 0u, "jazz: no loop cap");
#else
    check((env.caps & 8u) != 0u || (env.caps & 16u) != 0u, "non-jazz may have fill/loop");
#endif
    g = pick_until(&dj, FM_DJ_GEST_FLAM, &env, 0xffu);
    check(g == FM_DJ_GEST_FLAM, "flam pickable");
    check(fm_dj_gest_plan_bars(FM_DJ_GEST_FLAM, 1u) == 0u, "flam uses steps not bars");
    g = pick_until(&dj, FM_DJ_GEST_SOLO_BASS, &env, 0xffu);
    check(g == FM_DJ_GEST_SOLO_BASS, "solo bass pickable");
    check(fm_dj_gest_plan_bars(FM_DJ_GEST_SOLO_BASS, 3u) >= 1u && fm_dj_gest_plan_bars(FM_DJ_GEST_SOLO_BASS, 3u) <= 4u,
          "solo 1-4 bars");
    g = pick_until(&dj, FM_DJ_GEST_SOLO_DRUM, &env, 0xffu);
    check(g == FM_DJ_GEST_SOLO_DRUM, "solo drum pickable");
    g = pick_until(&dj, FM_DJ_GEST_BREAK_MICRO, &env, 0xffu);
    check(g == FM_DJ_GEST_BREAK_MICRO, "micro-break pickable");
    check(fm_dj_gest_plan_bars(FM_DJ_GEST_BREAK_MICRO, 0u) == 1u, "break one bar");
#if !defined(GENRE_JAZZ)
    env.caps = (uint8_t)(env.caps | 8u | 16u | 32u);
    g = pick_until(&dj, FM_DJ_GEST_FILL, &env, 0xffu);
    check(g == FM_DJ_GEST_FILL, "fill pickable when capped");
    env.loop_active = 0u;
    g = pick_until(&dj, FM_DJ_GEST_LOOP, &env, 0xffu);
    check(g == FM_DJ_GEST_LOOP, "looper pickable when capped");
    env.manual_arp_mode = 0u;
    g = pick_until(&dj, FM_DJ_GEST_ARP, &env, FM_DJ_ROLE_RIDE);
    check(g == FM_DJ_GEST_ARP, "alternate ARP pickable on ride role");
    check(fm_dj_gest_plan_bars(FM_DJ_GEST_ARP, 4u) >= 2u && fm_dj_gest_plan_bars(FM_DJ_GEST_ARP, 4u) <= 4u,
          "arp overlay 2-4 bars");
#endif
    dj.trans_type = FM_DJ_TRANS_DROP;
    dj.drop_sil_full = 1u;
    dj.drop_sil_keep = 0xffu;
    dj.phrase_bar = dj.drop_arm;
    check(fm_dj_gest_env_blocked(&dj, &env), "full pre-drop blocks gestures");
    dj.drop_sil_keep = 2u;
    dj.phrase_bar = (uint8_t)(dj.drop_arm - 3u);
    check(!fm_dj_gest_env_blocked(&dj, &env), "survivor lane plan allows mid-phrase gestures");
    env.manual_gesture_latch = 2u;
    check(fm_dj_gest_plan_pick(&dj, 6u, &env) == FM_DJ_GEST_NONE, "manual latch blocks");
    return fail ? 1 : 0;
}
