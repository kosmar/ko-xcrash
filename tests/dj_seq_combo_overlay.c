/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: DJ ensemble combo overlay — never TUTTI/TACET as intervention target. */
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
        fprintf(stderr, "dj_seq_combo_overlay: %s\n", msg);
        fail = 1;
    }
}

static void sim_bars(fm_dj_t *dj, fm_dj_seq_ens_ov_t *ov, uint8_t base_ens, int32_t ed, uint8_t bars)
{
    uint8_t b;
    uint8_t st;

    for (b = 0u; b < bars; b++) {
        for (st = 0u; st < 16u; st++) {
            uint8_t m;

            m = fm_dj_seq_ens_ov_step(dj, ov, b, st, 16u, 1u, base_ens, ENS_MUTE[FM_SEQ_ENS_ALL], ed, ENS_MUTE,
                                      0u);
            if (m != 0xffu) {
                check(ov->toast_ens != FM_SEQ_ENS_MUTE, "never tacet toast");
                check(ov->toast_ens != FM_SEQ_ENS_ALL || ov->seg == FM_DJ_SEQ_ENS_OV_SEG_RES,
                      "tutti only on resolve segment");
                check(fm_dj_seq_ens_is_combo(ov->combo_cur) || ov->seg == FM_DJ_SEQ_ENS_OV_SEG_RES ||
                          ov->combo_cur == FM_SEQ_ENS_ALL,
                      "active combo is valid preset");
            }
            if (st == 0u && b > 0u)
                check(st == 0u, "bar boundary at step 0");
        }
    }
}

int main(void)
{
    fm_dj_t dj;
    fm_dj_seq_ens_ov_t ov;
    uint8_t m;
    uint8_t st;

    fail = 0;
    fm_dj_enable(&dj, 0xbeefu);
    fm_dj_seq_ens_ov_reset(&ov);

    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_ENSEMBLE, 127, 4u, 1u, 4u);
    fm_dj_tick(&dj, 0u, 16u);
    check(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_ENSEMBLE) > 16, "ensemble slot live");

    m = fm_dj_seq_ens_ov_step(&dj, &ov, 0u, 0u, 16u, 1u, FM_SEQ_ENS_ALL, ENS_MUTE[FM_SEQ_ENS_ALL], 127, ENS_MUTE,
                              0u);
    check(m != 0xffu, "mask applied on arm");
    check(ov.active, "overlay armed");
    check(fm_dj_seq_ens_is_combo(ov.combo_a), "combo_a is real preset");
    check(ov.combo_a != FM_SEQ_ENS_MUTE && ov.combo_a != FM_SEQ_ENS_ALL, "combo_a not home/silence");

    sim_bars(&dj, &ov, FM_SEQ_ENS_ALL, 127, 6u);

    check(ov.cycle_done, "tutti resolve completed before slot release");
    check(!ov.active, "overlay cleared after resolve");
    m = fm_dj_seq_ens_ov_step(&dj, &ov, 6u, 0u, 16u, 1u, FM_SEQ_ENS_ALL, ENS_MUTE[FM_SEQ_ENS_ALL], 0, ENS_MUTE, 0u);
    check(m == 0xffu, "slot end uses base path when cycle done");

    fm_dj_seq_ens_ov_reset(&ov);
    fm_dj_slot_begin(&dj, 1u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_ENSEMBLE, 127, 5u, 1u, 2u);
    fm_dj_tick(&dj, 0u, 16u);
    (void)fm_dj_seq_ens_ov_step(&dj, &ov, 0u, 0u, 16u, 1u, FM_SEQ_ENS_DRUMS, ENS_MUTE[FM_SEQ_ENS_DRUMS], 127,
                                ENS_MUTE, 0u);
    check(ov.base_ens == FM_SEQ_ENS_DRUMS, "base captured");
    {
        uint8_t bar;
        uint8_t saw_res = 0u;
        uint8_t res_tutti = 0u;

        for (bar = 0u; bar < 8u && !ov.cycle_done; bar++) {
            for (st = 0u; st < 16u; st++) {
                m = fm_dj_seq_ens_ov_step(&dj, &ov, bar, st, 16u, 1u, FM_SEQ_ENS_DRUMS,
                                          ENS_MUTE[FM_SEQ_ENS_DRUMS], 127, ENS_MUTE, 0u);
                if (ov.active && ov.seg == FM_DJ_SEQ_ENS_OV_SEG_RES) {
                    saw_res = 1u;
                    if (m == (ENS_MUTE[FM_SEQ_ENS_ALL] & 0x3fu))
                        res_tutti = 1u;
                }
            }
        }
        check(saw_res, "non-tutti base still gets resolve segment");
        check(res_tutti, "resolve segment is tutti mask");
        check(ov.cycle_done, "combo cycle completes");
        m = fm_dj_seq_ens_ov_step(&dj, &ov, bar, 0u, 16u, 1u, FM_SEQ_ENS_DRUMS, ENS_MUTE[FM_SEQ_ENS_DRUMS], 127,
                                  ENS_MUTE, 0u);
        check(m == 0xffu, "after cycle caller owns base mask");
    }
    m = fm_dj_seq_ens_ov_step(&dj, &ov, 9u, 0u, 16u, 1u, FM_SEQ_ENS_DRUMS, ENS_MUTE[FM_SEQ_ENS_DRUMS], 0, ENS_MUTE,
                              0u);
    check(m == 0xffu, "slot end uses base path after tutti resolve");

    fm_dj_seq_ens_ov_reset(&ov);
    fm_dj_slot_begin(&dj, 2u, FM_DJ_ROLE_THROW, FM_DJ_TGT_ENSEMBLE, 127, 1u, 1u, 1u);
    (void)fm_dj_seq_ens_ov_step(&dj, &ov, 0u, 8u, 16u, 1u, FM_SEQ_ENS_ALL, ENS_MUTE[FM_SEQ_ENS_ALL], 127, ENS_MUTE,
                                0u);
    check(ov.invert_left <= 2u, "invert accent bounded");

    fm_dj_disable(&dj);
    check(fm_dj_seq_ens_bar_overlay(&dj, &ov, ENS_MUTE, 64u) == 64u, "ui bar follows base when dj off");

    return fail ? 1 : 0;
}
