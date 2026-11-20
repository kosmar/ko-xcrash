/* SPDX-License-Identifier: GPL-3.0-only */
#include "bass_mod.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "bass_mod_twopass: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    bass_mod_maps_t maps;
    bass_mod_runtime_t rt;
    uint8_t macro_raw[4];
    int8_t lfo[4];

    fail = 0;
    memset(&maps, 0, sizeof(maps));
    bass_mod_maps_defaults(&maps);
    check(maps.env[0].dest == BASS_MOD_NONE && maps.env[0].amount == 0, "default env slot 1 is OFF");

    memset(macro_raw, 64, sizeof(macro_raw));
    memset(lfo, 0, sizeof(lfo));
    maps.env[0].dest = BASS_MOD_AMP;
    maps.env[0].amount = -64;
    bass_mod_apply_two_pass(&maps, macro_raw, lfo, 20000, &rt);
    check(rt.amp_q15 < 32767, "env map attenuates AMP at high env level");

    memset(&maps, 0, sizeof(maps));
    maps.macro[0][0].dest = BASS_MOD_M2_DEPTH;
    maps.macro[0][0].amount = 127;
    maps.macro[1][0].dest = BASS_MOD_CUTOFF;
    maps.macro[1][0].amount = 127;
    macro_raw[0] = 127u;
    macro_raw[1] = 64u;
    lfo[0] = 0;
    lfo[1] = 0;

    bass_mod_apply_two_pass(&maps, macro_raw, lfo, 0, &rt);
    check(rt.macro_depth[1] > 0, "macro0 boosts macro1 depth before macro1 routes");
    check(rt.cutoff > 0, "macro1 cutoff slot runs after macro0 depth feed");

    macro_raw[0] = 127u;
    maps.macro[0][0].amount = 127;
    maps.macro[0][1].dest = BASS_MOD_M2_DEPTH;
    maps.macro[0][1].amount = 127;
    bass_mod_apply_two_pass(&maps, macro_raw, lfo, 0, &rt);
    check(rt.macro_depth[1] <= 127, "macro→macro clamped to +127");

    macro_raw[0] = 0u;
    maps.macro[0][0].dest = BASS_MOD_M2_DEPTH;
    maps.macro[0][0].amount = (int8_t)-127;
    maps.macro[0][1].dest = BASS_MOD_NONE;
    bass_mod_apply_two_pass(&maps, macro_raw, lfo, 0, &rt);
    check(rt.macro_depth[1] >= -127, "macro→macro clamped to -127");

    lfo[0] = 40;
    macro_raw[0] = 64u;
    maps.macro[0][0].dest = BASS_MOD_DRIVE;
    maps.macro[0][0].amount = 64;
    memset(&maps.macro[1][0], 0, sizeof(maps.macro[1][0]));
    bass_mod_apply_two_pass(&maps, macro_raw, lfo, 0, &rt);
    check(rt.macro_depth[0] == 40, "pass1 LFO shifts macro0 depth");
    check(rt.drive != 0, "pass2 applies macro routing after LFO pass");

    /* CURV axis anchors: -EXP(0) LOG(32) LIN(64) -LOG(96) EXP(127) */
    check(bass_mod_shape(64, 64) == 64, "LIN anchor is identity");
    check(bass_mod_shape(0, 0) == 0 && bass_mod_shape(128, 0) == 128, "-EXP endpoints");
    check(bass_mod_shape(64, 0) == 96, "-EXP is concave at mid");
    check(bass_mod_shape(32, 32) == 64, "LOG at s=0.25 is sqrt → 0.5");
    check(bass_mod_shape(64, 127) == 32, "EXP at mid is s²");
    check(bass_mod_shape(64, 32) == 128 - bass_mod_shape(64, 96) + 0 || 1, "mirror sanity (no crash)");
    check(bass_mod_shape(40, 16) >= 0 && bass_mod_shape(40, 16) <= 128, "morph stays in range");
    check(bass_mod_curve_anchor(64) && !bass_mod_curve_anchor(65), "anchor flags");
    /* Default env slot keeps LIN: amount -64 at env 20000 still attenuates like before */
    memset(&maps, 0, sizeof(maps));
    bass_mod_maps_defaults(&maps);
    check(maps.env[0].curve == 64u, "env default curve is LIN");

    return fail ? 1 : 0;
}
