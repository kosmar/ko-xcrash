/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: DJ SEQ horsemen (VEL/TRIG/GATE/PITCH) per-lane delta + spawn binding. */
#include "fm_dj.h"
#include "fm_dj_lfo.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_seq_param_lane: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    uint8_t tgt;
    uint8_t lane;
    uint8_t role;
    uint8_t k;

    fail = 0;
    fm_dj_enable(&dj, 0x5151u);

    fm_dj_slot_begin_lane(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_SEQ_VEL, 90, 2u, 4u, 8u, 2u);
    for (k = 0u; k < 24u; k++)
        fm_dj_tick(&dj, 0u, 16u);
    check(fm_dj_delta_for_tgt_lane(&dj, FM_DJ_TGT_SEQ_VEL, 2u) != 0, "vel delta on bound lane");
    check(fm_dj_delta_for_tgt_lane(&dj, FM_DJ_TGT_SEQ_VEL, 1u) == 0, "vel delta off other lane");
    check(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_SEQ_VEL) != 0, "vel global sum non-zero");

    fm_dj_disable(&dj);
    while (dj.slew_off)
        fm_dj_tick(&dj, 0u, 16u);
    fm_dj_enable(&dj, 0x6161u);

    role = FM_DJ_ROLE_RIDE;
    tgt = FM_DJ_TGT_MOTION;
    lane = FM_DJ_SLOT_LANE_GLOBAL;
    {
        uint32_t t;
        uint8_t got = 0u;

        for (t = 0u; t < 256u; t++) {
            role = FM_DJ_ROLE_RIDE;
            tgt = FM_DJ_TGT_MOTION;
            if (fm_dj_seq_param_try(&dj, 4u, 2u, &role, &tgt, &lane)) {
                got = 1u;
                break;
            }
        }
        check(got, "param try accepts burst bar");
    }
    check(fm_dj_tgt_is_seq_param(tgt), "param try picks horseman");
    check(lane < 6u, "param try binds lane");
    check(fm_dj_lfo_map_tgt(tgt) >= DUB_LFO_SEQ_VEL && fm_dj_lfo_map_tgt(tgt) <= DUB_LFO_SEQ_PITCH,
          "lfo map for seq param tgt");

    role = FM_DJ_ROLE_RIDE;
    tgt = FM_DJ_TGT_MOTION;
    check(!fm_dj_seq_param_try(&dj, dj.build_end + 1u, 2u, &role, &tgt, &lane), "param try skips late phrase");

    printf("dj_seq_param_lane: OK\n");
    return fail ? 1 : 0;
}
