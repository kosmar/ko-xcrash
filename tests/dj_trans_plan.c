/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: DROP / CROSSFADE / CUEING transition planner (fm_dj.h). */
#include "fm_dj.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_trans_plan: %s\n", msg);
        fail = 1;
    }
}

static void sim_drop_arm_bar(fm_dj_t *dj)
{
    uint8_t i;

    for (i = 0u; i < dj->drop_arm; i++)
        fm_dj_arm_bar(dj, 0u);
    check(dj->phrase_bar == dj->drop_arm, "walk to drop_arm");
}

static void fire_drop_arm(fm_dj_t *dj, uint8_t *tt_out)
{
    *tt_out = dj->trans_type;
    fm_dj_arm_bar(dj, 0u);
}

int main(void)
{
    fm_dj_t dj;
    uint32_t seed;
    uint32_t hist[FM_DJ_TRANS_N];
    uint8_t prev;
    int n;

    memset(hist, 0, sizeof(hist));
    fail = 0;

    fm_dj_enable(&dj, 0xcafebabeu);
    check(dj.trans_type < FM_DJ_TRANS_N, "initial trans_type valid");
    check(dj.trans_type < FM_DJ_TRANS_N, "initial trans in enum");

    fm_dj_enable(&dj, 0x12345678u);
    prev = 0xffu;
    for (n = 0; n < 80; n++) {
        uint8_t t = fm_dj_sim_next_phrase_plan(&dj);

        check(t < FM_DJ_TRANS_N, "trans_type in range");
        hist[t]++;
        if (prev != 0xffu)
            check(t != prev, "no immediate repeat of transition type");
        prev = t;
    }
    check(hist[FM_DJ_TRANS_DROP] > 10, "DROP appears in sequence");
    check(hist[FM_DJ_TRANS_CROSSFADE] > 10, "CROSSFADE appears in sequence");
    check(hist[FM_DJ_TRANS_CUEING] > 10, "CUEING appears in sequence");
    check(hist[FM_DJ_TRANS_DROP] < hist[FM_DJ_TRANS_CROSSFADE] + hist[FM_DJ_TRANS_CUEING] + 20,
          "DROP not majority-biased in sample");
    check(hist[FM_DJ_TRANS_ECHO_OUT] > 0, "ECHO OUT appears in sequence");

    {
        uint32_t seed_m;
        uint8_t saw_motion = 0u;
        uint8_t saw_dub_echo = 0u;

        for (seed_m = 1u; seed_m <= 200u; seed_m++) {
            fm_dj_enable(&dj, seed_m * 0x6c078965u);
            if (fm_dj_trans_is_drop_path(&dj) && dj.build_arc == FM_DJ_BUILD_MOTION_RISE)
                saw_motion = 1u;
            if (fm_dj_trans_is_drop_path(&dj) && dj.build_arc == FM_DJ_BUILD_DUB_ECHO)
                saw_dub_echo = 1u;
        }
        check(saw_motion, "MOTION RISE build reachable on DROP path");
        if (fm_dj_build_dub_echo_supported())
            check(saw_dub_echo, "DUB ECHO build reachable on dub host");
        else
            check(!saw_dub_echo, "DUB ECHO absent when unsupported");
    }

    for (seed = 3u; seed <= 20u; seed++) {
        fm_dj_enable(&dj, seed * 0x9e3779b9u);
        if (fm_dj_trans_is_drop_path(&dj)) {
            check(dj.drop_sil_full == 0u || dj.drop_sil_full == 1u, "drop_sil only on DROP plan");
        } else
            check(dj.drop_sil_full == 0u, "no drop_sil_full on non-DROP plan");
        if (dj.trans_type == FM_DJ_TRANS_CUEING)
            check(dj.build_arc == FM_DJ_BUILD_THIN, "CUEING uses thin build");

        uint8_t tt;

        sim_drop_arm_bar(&dj);
        fire_drop_arm(&dj, &tt);
        check(fm_dj_trans_pending_decode(&dj) == tt, "trans_pending encodes planned type");
        if (tt == FM_DJ_TRANS_DROP || tt == FM_DJ_TRANS_FILTER_SLAM) {
            check(dj.last_role == FM_DJ_ROLE_DROP, "DROP arms DROP slot");
            dj.trans_type = tt;
            dj.phrase_bar = dj.drop_arm;
            check(fm_dj_pre_drop_silence(&dj, 1u, 8u, 16u) == 1u || dj.drop_sil_full == 1u,
                  "DROP gates silence on break playback bar");
        } else {
            check(dj.last_role != FM_DJ_ROLE_DROP, "non-DROP must not arm DROP slot");
            check(fm_dj_pre_drop_silence(&dj, 1u, 8u, 16u) == 0u, "only DROP has pre-drop silence");
        }
    }

    fm_dj_enable(&dj, 77u);
    dj.trans_type = FM_DJ_TRANS_CROSSFADE;
    dj.phrase_bar = dj.break_arm;
    fm_dj_arm_bar(&dj, 0u);
    check(dj.last_role != FM_DJ_ROLE_BREAK, "CROSSFADE skips BREAK bar arm");

    fm_dj_enable(&dj, 88u);
    dj.trans_type = FM_DJ_TRANS_DROP;
    dj.phrase_bar = dj.break_arm;
    fm_dj_arm_bar(&dj, 0u);
    check(dj.last_role == FM_DJ_ROLE_BREAK, "DROP still arms BREAK");

    check(fm_dj_trans_plate_dir(FM_DJ_TRANS_CROSSFADE) == -1, "XF plate dir left");
    check(fm_dj_trans_plate_dir(FM_DJ_TRANS_DROP) == 0, "DROP has no plate dir");
    check(!fm_dj_trans_era_step_picked(FM_DJ_TRANS_DROP, 7u), "DROP never era-step pick");
    check(!fm_dj_trans_era_step_picked(FM_DJ_TRANS_CUEING, 7u), "CUEING never era-step pick");
    check(!fm_dj_trans_era_step_picked(FM_DJ_TRANS_CROSSFADE, 0u), "XF plate-only on seq 0");
    check(fm_dj_trans_era_step_picked(FM_DJ_TRANS_CROSSFADE, 7u), "XF era-step on seq 7");
    check(fm_dj_trans_era_step_picked(FM_DJ_TRANS_ECHO_OUT, 15u), "ECHO era-step on seq 15");
    check(fm_dj_trans_era_after(0, -1, 4u) == 3u, "era wrap prev at 0");
    check(fm_dj_trans_era_after(3, 1, 4u) == 0u, "era wrap next at last");
    check(fm_dj_trans_era_step_next(3u, 1, 4u, 4u) == 0u, "DJ +1 from last genre not artist");
    check(fm_dj_trans_era_step_next(0u, -1, 4u, 4u) == 3u, "DJ -1 from 0 wraps to last genre");
    check(fm_dj_trans_era_step_next(4u, -1, 4u, 4u) == 3u, "DJ leave artist backward");
    check(fm_dj_trans_era_step_next(4u, 1, 4u, 4u) == 0u, "DJ leave artist forward");
    check(fm_dj_trans_era_step_next(2u, 1, 4u, 4u) != 4u, "DJ mid step never artist");

#if defined(GENRE_TECHNO)
    {
        uint32_t i;
        uint8_t saw_slam = 0u;

        for (i = 1u; i <= 300u; i++) {
            fm_dj_enable(&dj, i * 0xdeadbeefu);
            if (dj.trans_type == FM_DJ_TRANS_FILTER_SLAM)
                saw_slam = 1u;
        }
        check(saw_slam, "techno FILTER SLAM reachable");
        check(fm_dj_trans_filter_slam_supported(), "techno reports filter slam");
    }
#elif defined(GENRE_POP)
    {
        uint32_t i;
        uint8_t saw_track = 0u;

        for (i = 1u; i <= 400u; i++) {
            fm_dj_enable(&dj, i * 0xcafebabeu);
            if (fm_dj_trans_is_drop_path(&dj) && dj.build_arc == FM_DJ_BUILD_TRACK_LIFT)
                saw_track = 1u;
        }
        check(saw_track, "pop TRACK LIFT reachable");
        check(fm_dj_build_track_lift_supported(), "pop reports track lift");
    }
#else
    check(!fm_dj_trans_filter_slam_supported(), "filter slam exclusive");
    check(!fm_dj_build_track_lift_supported(), "track lift exclusive");
#endif

    return fail ? 1 : 0;
}
