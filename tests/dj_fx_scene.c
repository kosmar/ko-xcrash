/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_dj_fx.h"
#include "fm_dj_time.h"
#include <stdio.h>
#include <string.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_fx_scene: %s\n", m);
        fail = 1;
    }
}

static void scene_moves(uint8_t scene, const dub_params_t *base, const dub_params_t *after, const char *tag)
{
    switch (scene) {
    case FM_DJ_FX_SCENE_DUB_ECHO:
        check(after->dly_lvl > base->dly_lvl && after->loop_filt > base->loop_filt && after->feedback >= base->feedback,
              tag);
        check(fm_dj_time_is_locked_cc(after->dly_time, 120u), tag);
        break;
    case FM_DJ_FX_SCENE_TAPE_ECHO:
        check(after->dly_lvl >= base->dly_lvl && after->loop_filt > base->loop_filt && after->wow > base->wow, tag);
        break;
    case FM_DJ_FX_SCENE_PONG_THROW:
        check(after->dly_lvl > base->dly_lvl && after->feedback >= base->feedback && after->dly_rev > base->dly_rev,
              tag);
        break;
    case FM_DJ_FX_SCENE_REV_WASH:
        check(after->rev_mix > base->rev_mix && after->rev_tail > base->rev_tail && after->tank > base->tank, tag);
        break;
    case FM_DJ_FX_SCENE_FREEZE_HOLD:
        check(after->feedback >= base->feedback && after->rev_tail > base->rev_tail && after->rev_mix > base->rev_mix,
              tag);
        break;
    case FM_DJ_FX_SCENE_ISO_FILTER:
        check(after->isolator > base->isolator && after->loop_filt > base->loop_filt && after->cutoff_c > base->cutoff_c,
              tag);
        break;
    default:
        check(0, "unknown scene in inventory");
        break;
    }
}

int main(void)
{
    fm_dj_t dj;
    dub_params_t vp;
    dub_params_t pristine;
    dub_params_t base;
    fm_dj_fx_ov_t ov;
    fm_dj_fx_slot_ov_t o;
    uint8_t sc;

    fail = 0;
    memset(&vp, 0, sizeof(vp));
    vp.dly_time = 40u;
    vp.dly_lvl = 30u;
    vp.feedback = 20u;
    vp.loop_filt = 25u;
    vp.wow = 10u;
    vp.rev_mix = 15u;
    vp.rev_tail = 18u;
    vp.tank = 12u;
    vp.isolator = 8u;
    vp.cutoff_c = 50u;
    pristine = vp;
    fm_dj_enable(&dj, 0xbeefu);
    fm_dj_fx_ov_reset(&ov);
    fm_dj_slot_begin(&dj, 0u, FM_DJ_ROLE_THROW, FM_DJ_TGT_DUB, 100, 2u, 1u, 4u);
    {
        uint8_t k;

        for (k = 0u; k < 12u; k++)
            fm_dj_tick(&dj, 0u, 16u);
    }
    fm_dj_fx_ov_track(&ov, &dj, 0u);
    check(ov.per[0].active, "fx overlay armed");
    check(ov.per[0].scene == FM_DJ_FX_SCENE_DUB_ECHO || ov.per[0].scene == FM_DJ_FX_SCENE_TAPE_ECHO,
          "throw+dub picks echo scene");
    base = vp;
    fm_dj_fx_apply(&vp, &dj, &ov, 0u);
    scene_moves(ov.per[0].scene, &base, &vp, "tracked throw scene");
    check(fm_dj_time_is_locked_cc(vp.dly_time, 120u), "echo scene TIME is note division on bar");
    {
        uint8_t hold = vp.dly_time;

        fm_dj_fx_apply(&vp, &dj, &ov, 4u);
        check(vp.dly_time == hold, "TIME held between quant steps");
    }
    check(fm_dj_fx_toast_label(&dj, &ov) != 0, "fx toast when audible");

    {
        uint32_t i;
        uint8_t saw_hog = 0u;

        for (i = 0u; i < 500u; i++) {
            fm_dj_enable(&dj, 0x1000u + i * 0x85ebca6bu);
            dj.ride_hog = 0u;
            if (fm_dj_fx_pick_scene(&dj, FM_DJ_ROLE_RIDE, FM_DJ_TGT_SPACE, 0u) ==
                FM_DJ_FX_SCENE_FREEZE_HOLD) {
                check(dj.ride_hog == 2u, "ride hog flagged");
                saw_hog = 1u;
                break;
            }
        }
        check(saw_hog, "ride hog reachable via rare steal");
    }
    for (sc = FM_DJ_FX_SCENE_DUB_ECHO; sc < FM_DJ_FX_SCENE_N; sc++) {
        memset(&o, 0, sizeof(o));
        o.active = 1u;
        o.scene = sc;
        vp = pristine;
        base = vp;
        fm_dj_fx_apply_scene(&vp, sc, 110, 0u, &o);
        scene_moves(sc, &base, &vp, "inventory scene");
    }
    dj.on = 0u;
    dj.slew_off = 0u;
    base = vp;
    fm_dj_fx_apply(&vp, &dj, &ov, 0u);
    check(memcmp(&vp, &base, sizeof(vp)) == 0, "DJ off skips fx overlay (base unchanged)");
    dj.on = 1u;
    dj.fill_blocked = 1u;
    fm_dj_fx_ov_track(&ov, &dj, fm_dj_fx_blocked(&dj, 1u, 0u, 16u));
    check(ov.per[0].active == 0u, "fill-block clears fx overlay");
    fm_dj_disable(&dj);
    fm_dj_fx_ov_reset(&ov);
    return fail ? 1 : 0;
}
