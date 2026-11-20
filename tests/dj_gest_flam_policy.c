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
        fprintf(stderr, "dj_gest_flam_policy: %s\n", m);
        fail = 1;
    }
}

#if defined(GENRE_HOUSE)
/* Same as fm_dj_gest_plan_pick but FLAM branch uses flam_denom (full fall-through). */
static uint8_t test_gest_pick_flam_denom(fm_dj_t *dj, uint8_t phrase_bar, const fm_dj_gest_env_t *env, uint8_t flam_denom)
{
    uint8_t r;
    uint8_t role;
    uint8_t pick;
    uint8_t caps;

    if (fm_dj_gest_env_blocked(dj, env))
        return FM_DJ_GEST_NONE;
    caps = env->caps;
    if (!caps)
        return FM_DJ_GEST_NONE;
    r = (uint8_t)fm_dj_rng_u8(dj);
    if ((r % FM_DJ_GEST_PICK_DENOM) != 0u)
        return FM_DJ_GEST_NONE;
    role = dj->role;
    if (phrase_bar <= 3u && (r % 3u) == 0u)
        return FM_DJ_GEST_NONE;

    pick = (uint8_t)(r % 7u);
    if (pick == 0u && (caps & 1u) && phrase_bar >= 4u && phrase_bar < dj->build_end) {
        if ((r & 1u) == 0u)
            return FM_DJ_GEST_SOLO_BASS;
        return FM_DJ_GEST_SOLO_DRUM;
    }
    if (pick == 1u && (caps & 2u) && env->flam_cd_bars == 0u && phrase_bar >= 4u &&
        phrase_bar < dj->break_arm - 3u && ((uint8_t)fm_dj_rng_u8(dj) % flam_denom) == 0u)
        return FM_DJ_GEST_FLAM;
    if (pick == 2u && (caps & 4u) && phrase_bar >= 6u && phrase_bar < dj->break_arm - 2u)
        return FM_DJ_GEST_BREAK_MICRO;
    if (pick == 3u && (caps & 8u) && phrase_bar >= dj->build_end - 2u && phrase_bar < dj->break_arm - 1u)
        return FM_DJ_GEST_FILL;
    {
        uint8_t loop_hi = dj->build_end > 2u ? (uint8_t)(dj->build_end - 2u) : 0u;
        uint8_t loop_lo = 12u;

        if (loop_lo >= loop_hi && loop_hi > 8u)
            loop_lo = (uint8_t)(loop_hi - 2u);
        if (pick == 4u && (caps & 16u) && !env->loop_active && env->loop_cd_bars == 0u && phrase_bar >= loop_lo &&
            phrase_bar < loop_hi && ((uint8_t)fm_dj_rng_u8(dj) % FM_DJ_GEST_LOOP_RARE) == 0u)
            return FM_DJ_GEST_LOOP;
    }
    if (pick == 5u && (caps & 32u) && env->manual_arp_mode == 0u && env->arp_cd_bars == 0u && phrase_bar >= 10u &&
        role == FM_DJ_ROLE_RIDE && ((uint8_t)fm_dj_rng_u8(dj) % FM_DJ_GEST_ARP_RARE) == 0u)
        return FM_DJ_GEST_ARP;
    if ((caps & 1u) && phrase_bar >= 10u && (r % 11u) == 0u)
        return (r & 2u) ? FM_DJ_GEST_SOLO_DRUM : FM_DJ_GEST_SOLO_BASS;
    return FM_DJ_GEST_NONE;
}
#endif

int main(void)
{
    fm_dj_t dj;
    fm_dj_gest_env_t env;
    uint32_t seed;
    uint32_t flam_total = 0u;
    uint32_t bar_total = 0u;
    int got_flam = 0;

#if defined(GENRE_HOUSE)
    uint32_t flam_ref_denom5 = 0u;
#endif

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
#define FLAM_POLICY_SEED_MAX 512u
#else
#define FLAM_POLICY_SEED_MAX 96u
#endif
    for (seed = 1u; seed < FLAM_POLICY_SEED_MAX; seed++) {
        uint8_t pb;
        uint8_t flam_phrase = 0u;

        fm_dj_enable(&dj, seed * 0x9e3779b9u);
        env.flam_cd_bars = 0u;
        for (pb = 2u; pb < dj.break_arm - 1u; pb++) {
            uint8_t g;

            dj.phrase_bar = pb;
#if defined(GENRE_HOUSE)
            {
                fm_dj_t dj_ref;
                uint32_t rng_snap = dj.rng;

                g = fm_dj_gest_plan_pick(&dj, pb, &env);
                dj_ref = dj;
                dj_ref.rng = rng_snap;
                if (test_gest_pick_flam_denom(&dj_ref, pb, &env, 5u) == FM_DJ_GEST_FLAM)
                    flam_ref_denom5++;
            }
#else
            g = fm_dj_gest_plan_pick(&dj, pb, &env);
#endif
            bar_total++;
            if (g == FM_DJ_GEST_FLAM) {
                flam_total++;
                flam_phrase++;
                got_flam = 1;
                fm_dj_gest_flam_armed(&env);
            }
            fm_dj_gest_env_tick(&env);
        }
        check(flam_phrase <= 1u, "at most one FLAM per phrase pass");
    }
    check(got_flam, "FLAM still reachable");
#if defined(GENRE_HOUSE)
    check(FM_DJ_GEST_FLAM_RARE_DENOM > 5u, "house flam rarer denom than default");
    check(flam_ref_denom5 > flam_total, "house FLAM rarer than default denom-5 path");
    check(flam_total * 400u < bar_total, "house FLAM sparse vs eligible bars");
#else
    check(flam_total * 200u < bar_total, "FLAM sparse vs eligible bars");
#endif
    check(flam_total >= 2u, "FLAM occurs over seeded sweep");

    env.flam_cd_bars = 4u;
    fm_dj_enable(&dj, 0xfaceu);
    dj.phrase_bar = 8u;
    check(fm_dj_gest_plan_pick(&dj, 8u, &env) != FM_DJ_GEST_FLAM, "FLAM blocked during cooldown");

    return fail ? 1 : 0;
}
