/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: DROP pre-silence full mute vs single-lane survivor (fm_dj_drop.h). */
#include "fm_dj_drop.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_drop_sil_lane: %s\n", msg);
        fail = 1;
    }
}

static uint8_t pop_count(uint8_t m)
{
    uint8_t n = 0u;
    uint8_t i;

    for (i = 0u; i < FM_DJ_DROP_SIL_LN_N; i++) {
        if (m & (uint8_t)(1u << i))
            n++;
    }
    return n;
}

int main(void)
{
    fm_dj_t dj;
    uint8_t on[FM_DJ_DROP_SIL_LN_N];
    uint8_t i;

    fail = 0;
    memset(on, 1, sizeof(on));

    fm_dj_enable(&dj, 0xabcdu);
    dj.trans_type = FM_DJ_TRANS_DROP;
    dj.drop_sil_full = 1u;
    dj.drop_sil_keep = FM_DJ_DROP_SIL_KEEP_ALL;
    dj.phrase_bar = dj.drop_arm;
    fm_dj_drop_sil_gate_lanes(&dj, 1u, 0u, 16u, on);
    check(pop_count(fm_dj_drop_sil_lane_mask(&dj, 1u, 0u, 16u)) == 0u, "full silence mask empty");
    for (i = 0u; i < FM_DJ_DROP_SIL_LN_N; i++)
        check(on[i] == 0u, "full silence gates all lanes");

    memset(on, 1, sizeof(on));
    dj.drop_sil_lane_pend = 1u;
    dj.drop_sil_keep = FM_DJ_DROP_SIL_KEEP_PEND;
    fm_dj_drop_sil_bind_audible(&dj, 0x04u); /* hat only */
    check(dj.drop_sil_lane_pend == 0u, "bind clears pending");
    check(dj.drop_sil_keep == 2u, "hat lane from audible mask");
    fm_dj_drop_sil_gate_lanes(&dj, 1u, 0u, 16u, on);
    check(pop_count(fm_dj_drop_sil_lane_mask(&dj, 1u, 0u, 16u)) == 1u, "exactly one lane bit");
    for (i = 0u; i < FM_DJ_DROP_SIL_LN_N; i++) {
        if (i == 2u)
            check(on[i] == 1u, "survivor lane stays on");
        else
            check(on[i] == 0u, "other lanes gated");
    }

    dj.trans_type = FM_DJ_TRANS_CROSSFADE;
    memset(on, 1, sizeof(on));
    fm_dj_drop_sil_gate_lanes(&dj, 1u, 0u, 16u, on);
    check(on[0] == 1u && on[5] == 1u, "CROSSFADE does not gate");

    fm_dj_disable(&dj);
    check(dj.drop_sil_keep == FM_DJ_DROP_SIL_KEEP_ALL, "disable resets keep");
    check(dj.drop_sil_lane_pend == 0u, "disable clears pending");

    fm_dj_enable(&dj, 0x777u);
    for (i = 0u; i < 64u; i++) {
        uint8_t m;

        dj.drop_sil_lane_pend = 1u;
        dj.drop_sil_keep = FM_DJ_DROP_SIL_KEEP_PEND;
        fm_dj_drop_sil_bind_audible(&dj, 0x3eu);
        m = (uint8_t)(1u << dj.drop_sil_keep);
        check(pop_count(m) == 1u, "bind picks one lane");
        check((m & 1u) == 0u, "kick excluded by default");
    }

    return fail ? 1 : 0;
}
