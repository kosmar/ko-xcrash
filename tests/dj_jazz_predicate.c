/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: Jazz DJ enablement (compile with -DGENRE_JAZZ). */
#include "fm_dj_conf.h"
#include "fm_dj.h"
#include "fm_dj_seq.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_jazz_predicate: %s\n", msg);
        fail = 1;
    }
}

int main(void)
{
    fm_dj_t dj;
    uint32_t seed;
    uint8_t saw_iso = 0u;
    uint8_t saw_dub_echo = 0u;
    uint8_t saw_track = 0u;
    uint8_t saw_filter_slam = 0u;

    fail = 0;
    check(fm_dj_seq_fx_supported() == 1u, "jazz seq fx supported");
    check(strcmp(fm_dj_seq_ens_toast_label(), "COMBO") == 0, "jazz ens knob label COMBO");
    check(strcmp(fm_dj_seq_flip_toast_label(), "INVERT") == 0, "jazz invert label");
    check(fm_dj_build_iso_tank_supported() == 0u, "jazz excludes ISO+TANK archetype");

    for (seed = 1u; seed <= 64u; seed++) {
        fm_dj_enable(&dj, seed * 0x517cc1b7u);
        if (dj.build_arc == FM_DJ_BUILD_ISO_TANK)
            saw_iso = 1u;
        if (dj.build_arc == FM_DJ_BUILD_DUB_ECHO)
            saw_dub_echo = 1u;
        if (dj.build_arc == FM_DJ_BUILD_TRACK_LIFT)
            saw_track = 1u;
        if (dj.trans_type == FM_DJ_TRANS_FILTER_SLAM)
            saw_filter_slam = 1u;
        fm_dj_disable(&dj);
    }
    check(!saw_iso, "phrase plan never picks ISO+TANK on jazz");
    check(!saw_dub_echo, "jazz never picks DUB ECHO build");
    check(!saw_track, "jazz never picks TRACK LIFT build");
    check(!saw_filter_slam, "jazz never picks FILTER SLAM trans");

    fm_dj_enable(&dj, 0xabcdu);
    check(dj.phrase_len == 16u || dj.phrase_len == 20u || dj.phrase_len == 24u || dj.phrase_len == 32u,
          "jazz variable phrase lengths");
    fm_dj_disable(&dj);

    return fail ? 1 : 0;
}
