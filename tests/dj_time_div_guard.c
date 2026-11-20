/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj.h"
#include "fm_dj_climax.h"
#include "fm_dj_fx.h"
#include "fm_dj_time.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_time_div_guard: %s\n", m);
        fail = 1;
    }
}

static void check_locked(uint8_t cc, uint32_t bpm, const char *tag)
{
    check(fm_dj_time_is_locked_cc(cc, bpm), tag);
}

int main(void)
{
    fm_dj_t dj;
    dub_params_t vp;
    fm_dj_fx_ov_t ov;
    fm_dj_fx_slot_ov_t o;
    uint8_t st;

    fail = 0;
    memset(&vp, 0, sizeof(vp));
    vp.bpm = 128u;
    vp.dly_time = fm_dj_time_anchor_div(4u, vp.bpm);
    fm_dj_enable(&dj, 0x600du);
    fm_dj_fx_ov_reset(&ov);

    for (st = 0u; st < 16u; st++) {
        vp.dly_time = fm_dj_time_anchor_div(4u, vp.bpm);
        memset(&o, 0, sizeof(o));
        o.active = 1u;
        o.scene = FM_DJ_FX_SCENE_DUB_ECHO;
        fm_dj_fx_apply_scene(&vp, FM_DJ_FX_SCENE_DUB_ECHO, 110, st, &o);
        if (st == 0u || st == 8u)
            check_locked(vp.dly_time, vp.bpm, "fx echo TIME step on boundary");
        else if (o.last_time != 0xffu)
            check(vp.dly_time == o.last_time, "fx echo TIME held between boundaries");
    }

    dj.build_arc = FM_DJ_BUILD_FB_PONG;
    dj.build_start = 4u;
    dj.build_end = 12u;
    dj.phrase_bar = 8u;
    vp.dly_time = fm_dj_time_anchor_div(2u, vp.bpm);
    fm_dj_apply_build_climb(&vp, &dj, 90);
    check_locked(vp.dly_time, vp.bpm, "pong build climb uses division anchor");

    return fail ? 1 : 0;
}
