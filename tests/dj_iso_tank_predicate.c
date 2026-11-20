/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: DJ ISO+TANK phrase predicates + plan weights (fm_dj.h). */
#include <stdio.h>
#include "fm_dj.h"

static int fail(const char *msg)
{
    fprintf(stderr, "dj_iso_tank_predicate: %s\n", msg);
    return 1;
}

int main(void)
{
    fm_dj_t dj;
    uint8_t pb;
    uint32_t iso_n;
    uint32_t i;

    fm_dj_enable(&dj, 0x5151u);
    if (!fm_dj_build_iso_tank_supported())
        return fail("expected ISO+TANK supported in host build");

    dj.trans_type = FM_DJ_TRANS_DROP;
    dj.build_arc = FM_DJ_BUILD_ISO_TANK;
    dj.drop_sil_full = 0u;
    dj.phrase_bar = dj.build_end;

    if (fm_dj_pre_drop_silence(&dj, 1u, 12u, 16u))
        return fail("climb bar must not pre-drop silence");
    if (!fm_dj_is_iso_tank_climb_bar(&dj))
        return fail("iso+tank climb during buildup");
    if (fm_dj_is_wash_climb_bar(&dj))
        return fail("iso+tank not wash climb");
    if (!fm_dj_build_tail_hold(&dj))
        return fail("iso+tank is tail-hold build");

    dj.phrase_bar = dj.break_arm;
    if (fm_dj_pre_drop_silence(&dj, 1u, 0u, 16u))
        return fail("break_arm must not pre-drop at step 0");
    if (fm_dj_is_iso_tank_climb_bar(&dj))
        return fail("break_arm not in iso+tank climb window");

    dj.trans_type = FM_DJ_TRANS_DROP;
    dj.phrase_bar = dj.drop_arm;
    if (!fm_dj_is_break_bar(&dj))
        return fail("drop_arm is break playback bar");
    if (!fm_dj_pre_drop_silence(&dj, 1u, 8u, 16u))
        return fail("break bar half silence step 8");
    if (fm_dj_is_iso_tank_climb_bar(&dj))
        return fail("break playback not iso climb bar");

    dj.build_arc = FM_DJ_BUILD_THIN;
    if (fm_dj_build_tail_hold(&dj))
        return fail("THIN not tail-hold");

    dj.build_arc = FM_DJ_BUILD_WASH;
    if (!fm_dj_build_tail_hold(&dj))
        return fail("WASH is tail-hold");

    iso_n = 0u;
    for (i = 0u; i < 1200u; i++) {
        fm_dj_pick_phrase_plan(&dj);
        if (dj.trans_type == FM_DJ_TRANS_DROP && dj.build_arc == FM_DJ_BUILD_ISO_TANK)
            iso_n++;
    }
    if (iso_n < 25u || iso_n > 120u)
        return fail("ISO+TANK pick weight out of band (~17% of DROP)");

    for (pb = dj.build_start; pb <= dj.build_end; pb++) {
        dj.phrase_bar = pb;
        dj.build_arc = FM_DJ_BUILD_ISO_TANK;
        if (fm_dj_pre_drop_silence(&dj, 1u, 15u, 16u))
            return fail("iso climb must never pre-drop");
        if (!fm_dj_is_iso_tank_climb_bar(&dj))
            return fail("iso climb bar missing");
    }

    printf("dj_iso_tank_predicate: OK\n");
    return 0;
}
