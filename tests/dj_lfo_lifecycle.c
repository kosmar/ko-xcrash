/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj.h"
#include "fm_dj_lfo.h"
#include "fm_lfo.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_lfo_lifecycle: %s\n", msg);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    fm_lfo_slot_t user[DUB_LFO_TGT_N];
    fm_lfo_slot_t eff[DUB_LFO_TGT_N];
    fm_lfo_rt_t rt[DUB_LFO_TGT_N];
    fm_dj_lfo_ov_t ov;
    uint8_t lt;

    fail = 0;
    fm_lfo_slots_init(user, rt, DUB_LFO_TGT_N);
    user[DUB_LFO_M_MOTION].depth = 0u;
    user[DUB_LFO_M_MOTION].rate = 4u;
    user[DUB_LFO_M_MOTION].wave = 0u;
    fm_dj_enable(&dj, 0x1234u);
    fm_dj_lfo_ov_reset(&ov);
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_MOTION, 90, 3u, 4u, 8u);
    fm_dj_tick(&dj, 0u, 16u);
    fm_dj_lfo_ov_track(&ov, &dj, user, 0u);
    lt = fm_dj_lfo_map_tgt(FM_DJ_TGT_MOTION);
    check(lt == DUB_LFO_M_MOTION, "motion maps to macro LFO");
    check(ov.per[0].active, "overlay armed");
    check(ov.per[0].plan.depth >= 22u, "plan depth nonzero");
    check(ov.per[0].plan.rate > 0u, "plan rate set");
    fm_dj_lfo_eff_build(&ov, &dj, user, eff, DUB_LFO_TGT_N);
    check(eff[lt].depth > user[lt].depth, "effective depth active");
    check(eff[lt].wave == ov.per[0].plan.wave, "effective wave from plan");
    check(eff[lt].rate == ov.per[0].plan.rate, "effective rate from plan");
    check(eff[lt].phase == ov.per[0].plan.phase, "effective phase from plan");
    check(ov.per[0].plan.smooth > 0u, "plan smooth set");
    check(user[lt].depth == 0u, "user slot untouched");
    check(fm_dj_lfo_map_tgt(FM_DJ_TGT_DUB) == DUB_LFO_EMIX, "DUB maps echo mix LFO");
    check(fm_dj_lfo_map_tgt(FM_DJ_TGT_FB) == DUB_LFO_FB, "FB maps feedback LFO");
    fm_dj_disable(&dj);
    while (dj.slew_off)
        fm_dj_tick(&dj, 0u, 16u);
    fm_dj_lfo_ov_reset(&ov);
    fm_dj_lfo_eff_build(&ov, &dj, user, eff, DUB_LFO_TGT_N);
    check(eff[lt].depth == user[lt].depth, "restore base depth after DJ off");
    return fail ? 1 : 0;
}
