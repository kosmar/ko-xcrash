/* SPDX-License-Identifier: GPL-3.0-only */
/* Host unit: DJ REVERB UP phrase predicates (fm_dj.h). */
#include <stdio.h>
#include "fm_dj.h"

static int fail(const char *msg)
{
    fprintf(stderr, "dj_wash_predicate: %s\n", msg);
    return 1;
}

int main(void)
{
    fm_dj_t dj;
    uint8_t pb;

    fm_dj_enable(&dj, 0x4242u);
    dj.build_arc = FM_DJ_BUILD_WASH;
    dj.drop_sil_full = 0u;
    dj.phrase_bar = dj.build_end;

    if (fm_dj_pre_drop_silence(&dj, 1u, 12u, 16u))
        return fail("climb bar must not pre-drop silence");
    if (!fm_dj_is_wash_climb_bar(&dj))
        return fail("wash climb during buildup");

    dj.phrase_bar = dj.break_arm;
    if (fm_dj_pre_drop_silence(&dj, 1u, 0u, 16u))
        return fail("break_arm must not pre-drop at step 0");
    if (fm_dj_is_wash_climb_bar(&dj))
        return fail("break_arm not in wash climb window");

    dj.phrase_bar = dj.drop_arm;
    if (!fm_dj_is_break_bar(&dj))
        return fail("drop_arm is break playback bar");
    if (!fm_dj_pre_drop_silence(&dj, 1u, 8u, 16u))
        return fail("break bar half silence step 8");
    if (fm_dj_pre_drop_silence(&dj, 1u, 4u, 16u))
        return fail("break bar half silence step 4 inactive");
    if (fm_dj_is_wash_climb_bar(&dj))
        return fail("break playback not wash climb");

    dj.drop_sil_full = 1u;
    if (!fm_dj_pre_drop_silence(&dj, 1u, 0u, 16u))
        return fail("full break bar silence");

    for (pb = dj.build_start; pb <= dj.build_end; pb++) {
        dj.phrase_bar = pb;
        dj.build_arc = FM_DJ_BUILD_WASH;
        dj.drop_sil_full = 0u;
        if (fm_dj_pre_drop_silence(&dj, 1u, 15u, 16u))
            return fail("wash climb must never pre-drop");
        if (!fm_dj_is_wash_climb_bar(&dj))
            return fail("wash climb bar missing");
    }

    dj.phrase_bar = (uint8_t)(dj.build_start + 1u);
    dj.build_arc = FM_DJ_BUILD_THIN;
    if (fm_dj_is_wash_climb_bar(&dj))
        return fail("THIN not wash climb");

    printf("dj_wash_predicate: OK\n");
    return 0;
}
