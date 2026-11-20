/* SPDX-License-Identifier: GPL-3.0-only */
/* Host: DJ isolator effective path (slot delta, FX scene, ISO+TANK climb, toast, restore). */
#include "fm_dj.h"
#include "fm_dj_climax.h"
#include "fm_dj_fx.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "dj_iso_lifecycle: %s\n", msg);
        fail = 1;
    }
}

static void apply_fx_base_iso(dub_params_t *vp, const dub_params_t *manual, const fm_dj_t *dj)
{
    vp->isolator = fm_dj_mod_u8(manual->isolator, dj, FM_DJ_TGT_ISO, 0, 127);
}

int main(void)
{
    fm_dj_t dj;
    dub_params_t manual;
    dub_params_t vp;
    dub_params_t pre;
    fm_dj_fx_ov_t ov;
    const char *toast;

    fail = 0;
    memset(&manual, 0, sizeof(manual));
    manual.isolator = 64u;
    manual.cutoff_c = 60u;
    manual.loop_filt = 64u;
    manual.tank = 40u;
    manual.rev_mix = 30u;
    manual.bpm = 120u;

    fm_dj_enable(&dj, 0x5151u);
    fm_dj_fx_ov_reset(&ov);

    /* Slot FM_DJ_TGT_ISO: base → effective isolator (DSP/UI parity). */
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_FILTER, FM_DJ_TGT_ISO, 100, 2u, 4u, 8u);
    {
        uint8_t k;

        for (k = 0u; k < 20u; k++)
            fm_dj_tick(&dj, 0u, 16u);
    }
    vp = manual;
    apply_fx_base_iso(&vp, &manual, &dj);
    check(fm_dj_fx_u8_delta(vp.isolator, manual.isolator) >= FM_DJ_TOAST_AUDIBLE, "ISO slot moves effective isolator");
    check(fm_dj_toast_delta_audible(fm_dj_delta_for_tgt(&dj, FM_DJ_TGT_ISO)), "ISO slot toast-audible");

    /* FX ISO_FILTER scene stacks after base (fm_app order). */
    fm_dj_disable(&dj);
    while (dj.slew_off)
        fm_dj_tick(&dj, 0u, 16u);
    fm_dj_enable(&dj, 0x6161u);
    fm_dj_fx_ov_reset(&ov);
    vp = manual;
    apply_fx_base_iso(&vp, &manual, &dj);
    pre = vp;
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_FILTER, FM_DJ_TGT_ISO, 110, 2u, 4u, 8u);
    for (uint8_t k = 0u; k < 24u; k++)
        fm_dj_tick(&dj, 0u, 16u);
    apply_fx_base_iso(&vp, &manual, &dj);
    fm_dj_fx_ov_track(&ov, &dj, 0u);
    check(ov.per[0].active && ov.per[0].scene == FM_DJ_FX_SCENE_ISO_FILTER, "iso filter scene armed");
    fm_dj_fx_apply(&vp, &dj, &ov, 0u);
    check(vp.isolator > pre.isolator || vp.loop_filt > pre.loop_filt || vp.cutoff_c > pre.cutoff_c,
          "ISO_FILTER scene after base");
    toast = fm_dj_fx_toast_label_vp(&dj, &ov, &vp, &manual);
    check(toast != 0, "ISOLATOR fx toast when effective delta audible");
    check(fm_dj_fx_scene_param_audible(FM_DJ_FX_SCENE_ISO_FILTER, &vp, &manual), "iso scene param predicate");

    /* ISO+TANK buildup sweep on isolator (phrase climax after FX base). */
    vp = manual;
    apply_fx_base_iso(&vp, &manual, &dj);
    dj.build_arc = FM_DJ_BUILD_ISO_TANK;
    dj.trans_type = FM_DJ_TRANS_DROP;
    dj.phrase_bar = dj.build_end;
    fm_dj_apply_phrase_climax(&vp, &dj, 0u, 0u);
    check(fm_dj_fx_u8_delta(vp.isolator, manual.isolator) >= FM_DJ_TOAST_AUDIBLE ||
              fm_dj_fx_u8_delta(vp.tank, manual.tank) >= FM_DJ_TOAST_AUDIBLE,
          "ISO+TANK climb moves iso or tank");
    check(fm_dj_build_toast_label(&dj) != 0, "ISO+TANK build toast label");

    /* Cleanup: DJ off restores manual isolator in voice path. */
    fm_dj_disable(&dj);
    while (dj.slew_off)
        fm_dj_tick(&dj, 0u, 16u);
    vp = manual;
    apply_fx_base_iso(&vp, &manual, &dj);
    check(vp.isolator == manual.isolator, "manual isolator restored after DJ off");

    return fail ? 1 : 0;
}
