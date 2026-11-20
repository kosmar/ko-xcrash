/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: DJ param toast only when delta/combo is audibly active. */
#include "fm_dj.h"
#include "fm_dj_seq.h"
#include <stdio.h>

static int fail;

static const uint8_t ENS_MUTE[FM_SEQ_NENSEMBLES] = {
    0x3fu, 0x3eu, 0x17u, 0x07u, 0x15u, 0x11u, 0x26u, 0x2cu, 0x0eu,
    0x31u, 0x13u, 0x19u, 0x2eu, 0x2au, 0x38u, 0x28u, 0x01u, 0x00u
};

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_toast_audible_predicate: %s\n", msg);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    fm_dj_seq_ens_ov_t ov;

    fail = 0;
    fm_dj_enable(&dj, 0x4242u);
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_MOTION, 127, 2u, 8u, 8u);
    check(fm_dj_toast_target(&dj) >= FM_DJ_TGT_N, "no toast target during attack delta 0");
    check(!fm_dj_toast_delta_audible(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_MOTION)), "motion inaudible at spawn");

    {
        uint8_t i;

        for (i = 0u; i < 32u; i++)
            fm_dj_tick(&dj, (uint8_t)(i % 16u), 16u);
    }
    check(fm_dj_toast_delta_audible(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_MOTION)), "motion audible after slew");
    check(fm_dj_toast_target(&dj) == FM_DJ_TGT_MOTION, "toast target follows audible motion");

    fm_dj_seq_ens_ov_reset(&ov);
    fm_dj_slot_begin(&dj, 1u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_ENSEMBLE, 127, 2u, 1u, 2u);
    fm_dj_tick(&dj, 0u, 16u);
    check(fm_dj_seq_ens_live(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_ENSEMBLE)), "ensemble slot audible");
    (void)fm_dj_seq_ens_ov_step(&dj, &ov, 0u, 0u, 16u, 1u, FM_SEQ_ENS_ALL, ENS_MUTE[FM_SEQ_ENS_ALL], 127, ENS_MUTE,
                                0u);
    check(fm_dj_seq_ens_ov_display_ens(&ov, 127, ENS_MUTE) != 0xffu, "combo toast when mask differs");
    ov.invert_accent = 0u;
    ov.invert_left = 0u;
    check(fm_dj_seq_ens_ov_display_ens(&ov, 127, ENS_MUTE) != 0xffu, "combo without invert pulse");
    ov.invert_accent = 1u;
    check(fm_dj_seq_ens_ov_display_ens(&ov, 127, ENS_MUTE) == 0xfeu, "invert sentinel only during pulse");
    ov.invert_accent = 0u;
    ov.active = 1u;
    ov.base_ens = FM_SEQ_ENS_DRUMS;
    ov.base_mask = ENS_MUTE[FM_SEQ_ENS_DRUMS] & 0x3fu;
    ov.seg = FM_DJ_SEQ_ENS_OV_SEG_RES;
    ov.combo_cur = FM_SEQ_ENS_ALL;
    check(fm_dj_seq_ens_ov_display_ens(&ov, 127, ENS_MUTE) == FM_SEQ_ENS_ALL,
          "tutti toast on resolve when mask differs from base");
    ov.base_ens = FM_SEQ_ENS_ALL;
    ov.base_mask = ENS_MUTE[FM_SEQ_ENS_ALL] & 0x3fu;
    check(fm_dj_seq_ens_ov_display_ens(&ov, 127, ENS_MUTE) == 0xffu,
          "no tutti toast on resolve when base already tutti");

    dj.phrase_bar = dj.build_start;
    check(fm_dj_build_toast_label(&dj) != 0, "build archetype label still on climb bars");

    return fail ? 1 : 0;
}
