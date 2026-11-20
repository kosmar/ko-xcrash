/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: DJ ENSEMBLE/FLIP targets and flip gate (fm_dj.h + fm_dj_seq.h). */
#include "fm_dj.h"
#include "fm_dj_seq.h"
#include <stdio.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_seq_fx_predicate: %s\n", msg);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    uint8_t role;
    uint8_t tgt;
    static const uint8_t ens_mute[FM_SEQ_NENSEMBLES] = {
        0x3fu, 0x3eu, 0x17u, 0x07u, 0x15u, 0x11u, 0x26u, 0x2cu, 0x0eu,
        0x31u, 0x13u, 0x19u, 0x2eu, 0x2au, 0x38u, 0x28u, 0x01u, 0x00u
    };

    fail = 0;
    fm_dj_enable(&dj, 0x1234u);
    dj.phrase_bar = (uint8_t)(dj.build_start + 2u);
    dj.build_arc = FM_DJ_BUILD_WASH;
    role = FM_DJ_ROLE_RIDE;
    tgt = FM_DJ_TGT_MOTION;
    check(fm_dj_seq_fx_supported() == 1u, "seq fx supported in host build");
    check(fm_dj_seq_fx_try(&dj, dj.break_arm, 0u, &role, &tgt) == 0u, "no ensemble spawn on break bar index");
    check(fm_dj_seq_fx_try(&dj, dj.build_end, 1u, &role, &tgt) == 0u, "no flip spawn at build_end+");

    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_THROW, FM_DJ_TGT_FLIP, 127, 1u, 1u, 2u);
    check(dj.slot[0].target == FM_DJ_TGT_FLIP, "flip slot mapped");
    fm_dj_tick(&dj, 0u, 16u);
    check(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_FLIP) != 0, "flip slot live after tick");
    dj.phrase_bar = dj.drop_arm;
    check(fm_dj_seq_flip_blocked(&dj, 1u, 8u, 16u) == 1u, "flip blocked on break bar");
    check(fm_dj_seq_flip_want(&dj, 1u, 8u, 16u, 127) == 0u, "flip want off when blocked");

    dj.phrase_bar = dj.build_start;
    check(fm_dj_seq_ens_index_from_bar(127u) == FM_SEQ_ENS_ALL, "bar 127 maps to tutti index");
    check(fm_dj_seq_blend_mute_toward(0x3fu, ens_mute, FM_SEQ_ENS_MUTE, 127) == ens_mute[FM_SEQ_ENS_MUTE],
          "ensemble sweep toward tacet");
    check(fm_dj_seq_blend_mute_mask(0x3fu, ens_mute, FM_SEQ_ENS_ALL, 127) == ens_mute[FM_SEQ_ENS_ALL],
          "ensemble widen full");
    {
        fm_dj_seq_ens_ov_t ov;

        fm_dj_seq_ens_ov_reset(&ov);
        check(fm_dj_seq_ens_bar_overlay(&dj, &ov, ens_mute, 64u) == 64u, "ens bar ui follows manual base");
        fm_dj_disable(&dj);
        check(fm_dj_seq_ens_bar_overlay(&dj, &ov, ens_mute, 40u) == 40u, "ui bar off when dj disabled");
    }

    return fail ? 1 : 0;
}
