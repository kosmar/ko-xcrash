/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: DJ combo masks + invert — never TACET/TUTTI during intervention. */
#include "fm_dj.h"
#include "fm_dj_seq.h"
#include "fm_seq.h"
#include "perf_toast_policy.h"
#include <stdio.h>

static int fail;

static const uint8_t ENS_MUTE[FM_SEQ_NENSEMBLES] = {
    0x3fu, 0x3eu, 0x17u, 0x07u, 0x15u, 0x11u, 0x26u, 0x2cu, 0x0eu,
    0x31u, 0x13u, 0x19u, 0x2eu, 0x2au, 0x38u, 0x28u, 0x01u, 0x00u
};

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_combo_mask_guard: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    uint8_t ens;
    fm_dj_t dj;
    fm_dj_seq_ens_ov_t ov;

    fail = 0;

#ifdef GENRE_HOUSE
    check(perf_toast_policy_same_msg(FM_SEQ_KNOB_P4[2], "CREW"), "house crew label");
#endif

    for (ens = 0u; ens < FM_SEQ_NENSEMBLES; ens++) {
        uint8_t m = fm_dj_seq_mask_low6(ENS_MUTE[ens]);
        uint8_t inv = fm_dj_seq_invert_mask(m);

        if (fm_dj_seq_ens_is_combo(ens)) {
            check(fm_dj_seq_ens_combo_mask_ok(ENS_MUTE, ens), "combo preset mask ok");
            check(fm_dj_seq_mask_dj_intervention_ok(inv), "combo invert mask ok");
        }
        if (ens == FM_SEQ_ENS_MUTE)
            check(fm_dj_seq_mask_all_muted(m), "tacet is all-muted");
        if (ens == FM_SEQ_ENS_ALL)
            check(fm_dj_seq_mask_all_unmuted(m), "tutti is all-unmuted");
    }

    fm_dj_enable(&dj, 0x600du);
    fm_dj_seq_ens_ov_reset(&ov);
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_RIDE, FM_DJ_TGT_ENSEMBLE, 127, 3u, 1u, 4u);
    fm_dj_tick(&dj, 0u, 16u);
    (void)fm_dj_seq_ens_ov_step(&dj, &ov, 0u, 0u, 16u, 1u, FM_SEQ_ENS_MUTE, ENS_MUTE[FM_SEQ_ENS_MUTE], 127,
                                ENS_MUTE, 0u);
    check(ov.active, "overlay arms from manual tacet base");
    check(ov.toast_ens != FM_SEQ_ENS_MUTE, "dj toast not tacet");
    check(fm_dj_seq_ens_combo_mask_ok(ENS_MUTE, ov.toast_ens), "dj toast combo mask ok");

    {
        uint8_t st;

        for (st = 0u; st < 16u; st++) {
            uint8_t m = fm_dj_seq_ens_ov_step(&dj, &ov, 0u, st, 16u, 1u, FM_SEQ_ENS_MUTE,
                                              ENS_MUTE[FM_SEQ_ENS_MUTE], 127, ENS_MUTE, 0u);

            if (ov.active && ov.seg != FM_DJ_SEQ_ENS_OV_SEG_RES) {
                check(m != 0xffu, "mask applied during intervention");
                check(fm_dj_seq_mask_dj_intervention_ok(m), "applied mask never tacet/tutti");
                if (ov.invert_accent) {
                    uint8_t inv = fm_dj_seq_invert_mask(m);

                    check(fm_dj_seq_mask_dj_intervention_ok(inv), "invert preview safe at combo");
                }
            }
        }
    }

    return fail ? 1 : 0;
}
