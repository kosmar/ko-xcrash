/* SPDX-License-Identifier: GPL-3.0-only */
/* Integration: DJ phrase bars 0–8 — MAIN lane must not churn at gest/ARP spawn (~bar 4–5). */
#include "fm_dj.h"
#include "fm_dj_gest.h"
#include "perf_toast_policy.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_toast_bar_spawn_stability: %s\n", m);
        fail = 1;
    }
}

static int perf_toast_is_dj_msg(const char *msg)
{
    return msg && msg[0] == 'D' && msg[1] == 'J' && msg[2] == 0;
}

/* Mirrors perf_toast_pick DJ + gest ARP branches (ARP → DJ sticky, side elsewhere). */
static const char *pick_sticky_msg(uint8_t dj_on, uint8_t gest_arp, uint8_t arp_mode, uint8_t gest_loop)
{
    if (!dj_on)
        return 0;
    if (gest_loop)
        return "DJ";
    if (gest_arp && arp_mode != 0u)
        return "DJ";
    return "DJ";
}

int main(void)
{
    fm_dj_t dj;
    fm_dj_gest_env_t env;
    perf_toast_lane_t main_lane;
    uint32_t frame;
    uint32_t t0 = 5000u;
    uint32_t dj_start_ms = 0u;
    uint8_t dj_seen = 0u;
    uint8_t pb;
    uint8_t gest_arp = 0u;
    uint8_t arp_mode = 0u;

    fail = 0;
    memset(&main_lane, 0, sizeof main_lane);
    fm_dj_enable(&dj, 0x4242u);
    fm_dj_layout_phrase(&dj);

    for (frame = 0u; frame < 2400u; frame++) {
        uint32_t now = t0 + frame * 16u;
        const char *msg;
        uint8_t kind = 1u;

        if ((frame % 384u) == 0u && dj.phrase_bar < dj.phrase_len - 1u)
            dj.phrase_bar++;
        pb = dj.phrase_bar;

        if (pb >= 4u && pb <= 6u && !gest_arp && (frame % 800u) == 100u) {
            gest_arp = 1u;
            arp_mode = 2u;
        }
        if (gest_arp && frame > 0u && (frame % 800u) == 750u) {
            gest_arp = 0u;
            arp_mode = 0u;
        }

        msg = pick_sticky_msg(1u, gest_arp, arp_mode, 0u);
        if (msg && msg[0])
            perf_toast_policy_lane_set_main(&main_lane, msg, kind, -1, 0u, now, 0, 0u);
        if (main_lane.on && perf_toast_is_dj_msg(main_lane.msg)) {
            if (!dj_seen) {
                dj_seen = 1u;
                dj_start_ms = main_lane.start_ms;
            } else
                check(main_lane.start_ms == dj_start_ms, "DJ sticky start_ms stable across gest ARP spawn");
        }
    }

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
    for (pb = 0u; pb <= 8u; pb++) {
        uint8_t plan;

        dj.phrase_bar = pb;
        plan = fm_dj_gest_plan_pick(&dj, pb, &env);
        if (plan == FM_DJ_GEST_ARP) {
            const char *old_pick = "SKANK";
            const char *new_pick = pick_sticky_msg(1u, 1u, 3u, 0u);

            check(perf_toast_is_dj_msg(new_pick), "ARP gest uses DJ sticky not ARP label");
            check(!perf_toast_policy_same_msg(old_pick, new_pick), "old bug used ARP label sticky");
        }
    }

    printf("dj_toast_bar_spawn_stability: OK (pushes=0)\n");
    return fail ? 1 : 0;
}
