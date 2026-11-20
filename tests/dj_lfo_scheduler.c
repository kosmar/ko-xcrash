/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj.h"
#include "fm_dj_lfo.h"
#include "fm_lfo.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_lfo_scheduler: %s\n", m);
        fail = 1;
    }
}

static uint8_t overlay_arm_events(fm_dj_lfo_ov_t *ov, const fm_dj_t *dj, const fm_lfo_slot_t *eff,
                                  uint8_t *prev_active)
{
    uint8_t si;
    uint8_t events = 0u;

    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        const fm_dj_slot_t *s = &dj->slot[si];
        uint8_t live = 0u;

        if (o->active && fm_dj_lfo_depth_scaled(o->plan.depth, s) > 0u && eff[o->lfo_tgt].depth > 0u)
            live = 1u;
        if (live && !prev_active[si])
            events++;
        prev_active[si] = live;
    }
    return events;
}

int main(void)
{
    fm_dj_t dj;
    fm_lfo_slot_t user[DUB_LFO_TGT_N];
    fm_lfo_slot_t eff[DUB_LFO_TGT_N];
    fm_lfo_rt_t rt[DUB_LFO_TGT_N];
    fm_dj_lfo_ov_t ov;
    uint32_t seed;
    uint32_t arm_events = 0u;
    uint32_t cycles = 0u;
    uint8_t prev_active[FM_DJ_SLOT_N];

    fail = 0;
    fm_lfo_slots_init(user, rt, DUB_LFO_TGT_N);

    for (seed = 1u; seed < 48u; seed++) {
        uint8_t pb;
        uint8_t cycle;
        uint8_t si;

        fm_dj_enable(&dj, seed * 0x9e3779b9u);
        fm_dj_lfo_ov_reset(&ov);
        for (si = 0u; si < FM_DJ_SLOT_N; si++)
            prev_active[si] = 0u;
        for (cycle = 0u; cycle < 3u; cycle++) {
            cycles++;
            for (pb = 0u; pb < dj.break_arm; pb++) {
                dj.phrase_bar = pb;
                fm_dj_arm_bar(&dj, 0u);
                {
                    uint8_t t;

                    for (t = 0u; t < 48u; t++)
                        fm_dj_tick(&dj, (uint8_t)(t % 16u), 16u);
                }
                fm_dj_lfo_ov_track(&ov, &dj, user, 0u);
                fm_dj_lfo_eff_build(&ov, &dj, user, eff, DUB_LFO_TGT_N);
                arm_events += overlay_arm_events(&ov, &dj, eff, prev_active);
            }
        }
    }
    check(arm_events > 0u, "DJ LFO overlay reaches audible depth");
    check(arm_events < cycles * 6u, "LFO arms not clustered every bar");
    check(arm_events * 2u >= cycles, "meaningful LFO events per transition cycles");

    fm_dj_enable(&dj, 0x4242u);
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_MOTION, 90, 3u, 4u, 8u);
    check(fm_dj_lfo_slot_drive(&dj.slot[0]) >= 45, "attack uses peak drive");
    fm_dj_lfo_ov_reset(&ov);
    fm_dj_lfo_ov_track(&ov, &dj, user, 0u);
    check(ov.per[0].plan.depth >= 22u, "plan depth floor");
    fm_dj_lfo_eff_build(&ov, &dj, user, eff, DUB_LFO_TGT_N);
    check(eff[DUB_LFO_M_MOTION].depth > 0u, "effective depth during attack");
    fm_dj_disable(&dj);
    while (dj.slew_off)
        fm_dj_tick(&dj, 0u, 16u);
    fm_dj_lfo_ov_reset(&ov);
    fm_dj_lfo_eff_build(&ov, &dj, user, eff, DUB_LFO_TGT_N);
    check(eff[DUB_LFO_M_MOTION].depth == user[DUB_LFO_M_MOTION].depth, "restore after DJ off");

    return fail ? 1 : 0;
}
