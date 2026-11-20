/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: variable DJ phrase lengths and drop spacing (fm_dj.h). */
#include "fm_dj.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_phrase_interval: %s\n", msg);
        fail = 1;
    }
}

static int len_allowed(uint8_t len)
{
    return len == 16u || len == 20u || len == 24u || len == 32u;
}

static void sim_phrase(fm_dj_t *dj, uint8_t *drop_at)
{
    uint8_t start = dj->phrase_bar;
    uint8_t len = dj->phrase_len;
    uint8_t i;

    *drop_at = 0xffu;
    for (i = 0u; i < len; i++) {
        uint8_t pb = dj->phrase_bar;

        if (pb == dj->drop_arm) {
            uint8_t tt = dj->trans_type;

            fm_dj_arm_bar(dj, 0u);
            if (tt == FM_DJ_TRANS_DROP || tt == FM_DJ_TRANS_FILTER_SLAM)
                check(dj->last_role == FM_DJ_ROLE_DROP, "DROP trans must arm DROP on drop_arm");
            else
                check(dj->last_role == FM_DJ_ROLE_RIDE, "non-DROP trans must not arm DROP role");
            *drop_at = i;
        } else
            fm_dj_arm_bar(dj, 0u);
    }
    check(dj->phrase_bar == start, "phrase must return to start index after full cycle");
    check(*drop_at == len - 1u, "transition only on final bar of phrase");
}

int main(void)
{
    fm_dj_t dj;
    uint32_t seed;
    uint8_t seen[4];
    uint8_t len_hist[33];
    uint8_t idx;
    int i;

    memset(seen, 0, sizeof(seen));
    memset(len_hist, 0, sizeof(len_hist));
    fail = 0;

    fm_dj_enable(&dj, 0xdeadbeefu);
    check(dj.phrase_len >= 16u, "phrase_len after enable");
    check(len_allowed(dj.phrase_len), "initial phrase_len allowed set");
    check(dj.build_end >= dj.build_start, "build window valid");
    check((uint8_t)(dj.build_end - dj.build_start + 1u) >= 2u, "buildup at least 2 bars");
    check((uint8_t)(dj.build_end - dj.build_start + 1u) <= 4u, "buildup at most 4 bars");
    check(dj.drop_arm == dj.phrase_len - 1u, "drop_arm at phrase end");
    check(dj.break_arm == dj.phrase_len - 2u, "break_arm before drop");

    for (idx = 0u; idx < dj.phrase_len; idx++) {
        if (dj.phrase_bar == dj.drop_arm)
            break;
        fm_dj_arm_bar(&dj, 0u);
    }
    check(dj.phrase_bar == dj.drop_arm, "first DROP only after full lead-in phrase");
    check(idx == dj.drop_arm, "no DROP before scheduled drop_arm");

    for (seed = 1u; seed <= 48u; seed++) {
        uint8_t drop_at;
        int p;

        fm_dj_enable(&dj, seed * 0x9e3779b9u);
        for (p = 0; p < 3; p++) {
            uint8_t len = dj.phrase_len;

            check(len_allowed(len), "phrase_len in {16,20,24,32}");
            check(len != 12u, "no 12-bar phrases");
            len_hist[len]++;
            if (len == 16u)
                seen[0] = 1u;
            else if (len == 20u)
                seen[1] = 1u;
            else if (len == 24u)
                seen[2] = 1u;
            else if (len == 32u)
                seen[3] = 1u;

            sim_phrase(&dj, &drop_at);
        }
    }

    check(seen[0] && seen[1] && seen[2] && seen[3], "all interval lengths appear across seeds");
    check(len_hist[24] >= len_hist[16], "24-bar phrases at least as common as 16");

    for (i = 1; i < 33; i++) {
        if (i != 16 && i != 20 && i != 24 && i != 32)
            check(len_hist[i] == 0u, "no off-grid phrase lengths");
    }

    return fail ? 1 : 0;
}
