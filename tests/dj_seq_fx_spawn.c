/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: DJ ENSEMBLE/FLIP targets spawn via phrase arm + mapping lifecycle. */
#include "fm_dj.h"
#include "fm_dj_seq.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_seq_fx_spawn: %s\n", msg);
        fail = 1;
    }
}

static uint8_t slot_has_tgt(const fm_dj_t *dj, uint8_t tgt)
{
    uint8_t i;

    for (i = 0u; i < FM_DJ_SLOT_N; i++) {
        const fm_dj_slot_t *s = &dj->slot[i];

        if (s->phase != FM_DJ_SLOT_OFF && s->target == tgt)
            return 1u;
    }
    return 0u;
}

static void sim_phrases(fm_dj_t *dj, uint8_t phrases, uint8_t *ens_hit, uint8_t *flip_hit)
{
    uint8_t p;
    uint8_t b;

    *ens_hit = 0u;
    *flip_hit = 0u;
    for (p = 0u; p < phrases; p++) {
        for (b = 0u; b < dj->phrase_len; b++) {
            uint8_t st;

            fm_dj_arm_bar(dj, 0u);
            if (slot_has_tgt(dj, FM_DJ_TGT_ENSEMBLE))
                *ens_hit = 1u;
            if (slot_has_tgt(dj, FM_DJ_TGT_FLIP))
                *flip_hit = 1u;
            for (st = 0u; st < 16u; st++)
                fm_dj_tick(dj, st, 16u);
            if (fm_dj_delta_for_tgt(dj, FM_DJ_TGT_ENSEMBLE) != 0)
                *ens_hit = 1u;
            if (fm_dj_delta_for_tgt(dj, FM_DJ_TGT_FLIP) != 0)
                *flip_hit = 1u;
        }
    }
}

int main(void)
{
    fm_dj_t dj;
    uint32_t seed;
    uint8_t ens_any = 0u;
    uint8_t flip_any = 0u;
    uint8_t ens;
    uint8_t flip;
    uint8_t role;
    uint8_t tgt;

    fail = 0;
    check(fm_dj_seq_fx_supported() == 1u, "seq fx supported in default host build");

    {
        uint32_t s;
        uint8_t got_ens = 0u;

        for (s = 1u; s < 512u; s++) {
            fm_dj_enable(&dj, s);
            dj.phrase_bar = (uint8_t)(dj.build_start + 1u);
            if (dj.phrase_bar >= dj.break_arm)
                dj.phrase_bar = dj.build_start;
            role = FM_DJ_ROLE_RIDE;
            tgt = FM_DJ_TGT_MOTION;
            if (fm_dj_seq_fx_try(&dj, dj.phrase_bar, 0u, &role, &tgt) && tgt == FM_DJ_TGT_ENSEMBLE) {
                got_ens = 1u;
                break;
            }
        }
        check(got_ens, "shape0 can pick ensemble");
    }
    fm_dj_slot_begin(&dj, 0u, role, tgt, 127, 2u, 4u, 8u);
    fm_dj_tick(&dj, 0u, 16u);
    check(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_ENSEMBLE) != 0, "ensemble slot produces delta");

    {
        uint32_t s;
        uint8_t got_flip = 0u;

        for (s = 1u; s < 256u; s++) {
            fm_dj_enable(&dj, s);
            dj.phrase_bar = dj.build_start;
            role = FM_DJ_ROLE_THROW;
            tgt = FM_DJ_TGT_MOTION;
            if (fm_dj_seq_fx_try(&dj, dj.phrase_bar, 1u, &role, &tgt) && tgt == FM_DJ_TGT_FLIP) {
                got_flip = 1u;
                break;
            }
        }
        check(got_flip, "shape1 can pick flip on build");
        fm_dj_slot_begin(&dj, 1u, role, tgt, 127, 1u, 1u, 2u);
        fm_dj_tick(&dj, 0u, 16u);
        check(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_FLIP) > 16, "flip burst reaches audibility threshold");
    }

    for (seed = 1u; seed <= 48u; seed++) {
        fm_dj_enable(&dj, seed * 0x9e3779b9u);
        sim_phrases(&dj, 2u, &ens, &flip);
        if (ens)
            ens_any = 1u;
        if (flip)
            flip_any = 1u;
        if (ens && flip)
            break;
    }
    check(ens_any, "ensemble spawns within 2 phrases (seed sweep)");
    check(flip_any, "invert/flip spawns within 2 phrases (seed sweep)");

    fm_dj_enable(&dj, 0xc0ffeeu);
    check(strcmp(fm_dj_seq_ens_toast_label(), FM_SEQ_KNOB_P4[2]) == 0, "ens toast uses knob table");
    check(strcmp(fm_dj_seq_flip_toast_label(), FM_SEQ_KNOB_P4[3]) == 0, "flip toast uses knob table");

    return fail ? 1 : 0;
}
