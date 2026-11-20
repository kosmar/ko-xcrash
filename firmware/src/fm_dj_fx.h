/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ FX scenes: coherent echo/reverb/iso moves (additive on morph base, no cfg writes). */
#ifndef FM_DJ_FX_H
#define FM_DJ_FX_H

#ifndef FM_DJ_FX_RIDE_RARE
#define FM_DJ_FX_RIDE_RARE 19u
#endif
#ifndef FM_DJ_FX_BREAK_RARE
#define FM_DJ_FX_BREAK_RARE 4u
#endif

#include "dub_types.h"
#include "fm_dj.h"
#include "fm_dj_drop.h"
#include "fm_dj_time.h"

enum {
    FM_DJ_FX_SCENE_NONE = 0,
    FM_DJ_FX_SCENE_DUB_ECHO,
    FM_DJ_FX_SCENE_TAPE_ECHO,
    FM_DJ_FX_SCENE_PONG_THROW,
    FM_DJ_FX_SCENE_REV_WASH,
    FM_DJ_FX_SCENE_FREEZE_HOLD,
    FM_DJ_FX_SCENE_ISO_FILTER,
    FM_DJ_FX_SCENE_N
};

typedef struct {
    uint8_t active;
    uint8_t scene;
    uint8_t dj_tgt;
    uint8_t last_time; /* beat-quantized dly_time anchor */
} fm_dj_fx_slot_ov_t;

typedef struct {
    fm_dj_fx_slot_ov_t per[FM_DJ_SLOT_N];
} fm_dj_fx_ov_t;

static inline void fm_dj_fx_ov_reset(fm_dj_fx_ov_t *ov)
{
    uint8_t i;

    for (i = 0u; i < FM_DJ_SLOT_N; i++)
        ov->per[i].active = 0u;
}

static inline uint8_t fm_dj_fx_blocked(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                       uint8_t steps_per_bar)
{
    if (!dj->on && !dj->slew_off)
        return 1u;
    if (dj->fill_blocked)
        return 1u;
    if (fm_dj_trans_is_drop_path(dj) && playing &&
        fm_dj_pre_drop_silence(dj, 1u, bar_step, steps_per_bar) &&
        dj->drop_sil_keep >= FM_DJ_DROP_SIL_LN_N)
        return 1u;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_DROP))
        return 1u;
    if (dj->trans_pending != 0u)
        return 1u;
    return 0u;
}

static inline uint8_t fm_dj_fx_pick_scene(fm_dj_t *dj, uint8_t role, uint8_t tgt, uint8_t r)
{
    if (role == FM_DJ_ROLE_THROW) {
        if (tgt == FM_DJ_TGT_DUB || tgt == FM_DJ_TGT_SPACE)
            return (r & 1u) ? FM_DJ_FX_SCENE_DUB_ECHO : FM_DJ_FX_SCENE_TAPE_ECHO;
        if (tgt == FM_DJ_TGT_FB)
            return FM_DJ_FX_SCENE_PONG_THROW;
        return FM_DJ_FX_SCENE_TAPE_ECHO;
    }
    if (role == FM_DJ_ROLE_FILTER)
        return FM_DJ_FX_SCENE_ISO_FILTER;
    if (role == FM_DJ_ROLE_RIDE) {
        if ((r % FM_DJ_FX_RIDE_RARE) != 0u)
            return FM_DJ_FX_SCENE_NONE;
        /* 1/5 of rare ride FX → hog (steals from REV WASH / ECHO scenes). */
        if ((fm_dj_rng_u8(dj) % 5u) == 0u) {
            dj->ride_hog = 2u;
            return FM_DJ_FX_SCENE_FREEZE_HOLD;
        }
        if (tgt == FM_DJ_TGT_SPACE || tgt == FM_DJ_TGT_ISO)
            return FM_DJ_FX_SCENE_REV_WASH;
        return FM_DJ_FX_SCENE_DUB_ECHO;
    }
    if (role == FM_DJ_ROLE_BREAK) {
        if ((r % FM_DJ_FX_BREAK_RARE) != 0u)
            return FM_DJ_FX_SCENE_NONE;
        return FM_DJ_FX_SCENE_REV_WASH;
    }
    return FM_DJ_FX_SCENE_NONE;
}

static inline void fm_dj_fx_ov_track(fm_dj_fx_ov_t *ov, fm_dj_t *dj, uint8_t blocked)
{
    uint8_t si;

    if (blocked) {
        fm_dj_fx_ov_reset(ov);
        return;
    }
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_slot_t *s = &dj->slot[si];
        fm_dj_fx_slot_ov_t *o = &ov->per[si];
        int32_t live;
        uint8_t lt;

        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0) {
            o->active = 0u;
            continue;
        }
        live = (int32_t)s->delta;
        if (live < 0)
            live = -live;
        if (live < FM_DJ_TOAST_AUDIBLE && s->phase == FM_DJ_SLOT_OFF) {
            o->active = 0u;
            continue;
        }
        lt = s->target;
        if (!o->active || o->dj_tgt != lt) {
            uint8_t r = (uint8_t)fm_dj_rng_u8(dj);

            o->active = 1u;
            o->dj_tgt = lt;
            o->scene = fm_dj_fx_pick_scene(dj, s->role, lt, r);
            o->last_time = 0xffu;
        }
    }
}

static inline int32_t fm_dj_fx_strength(const fm_dj_slot_t *s)
{
    int32_t d = (int32_t)s->delta;

    if (d < 0)
        d = -d;
    return d;
}

static inline int fm_dj_fx_echo_time_scene(uint8_t scene)
{
    return scene == FM_DJ_FX_SCENE_DUB_ECHO || scene == FM_DJ_FX_SCENE_TAPE_ECHO;
}

static inline int fm_dj_fx_echo_time_live(const fm_dj_t *dj, const fm_dj_fx_ov_t *ov)
{
    uint8_t si;

    if (fm_dj_build_moves_echo_time(dj))
        return 1;
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        if (ov->per[si].active && fm_dj_fx_echo_time_scene(ov->per[si].scene))
            return 1;
    }
    return 0;
}

static inline void fm_dj_fx_apply_scene(dub_params_t *vp, uint8_t scene, int32_t str, uint8_t bar_step,
                                        fm_dj_fx_slot_ov_t *o)
{
    int32_t k;
    uint32_t bpm;

    if (scene == FM_DJ_FX_SCENE_NONE || str < FM_DJ_TOAST_AUDIBLE)
        return;
    k = str;
    bpm = vp->bpm ? vp->bpm : 120u;
    switch (scene) {
    case FM_DJ_FX_SCENE_DUB_ECHO:
        if (bar_step == 0u || bar_step == (uint8_t)(DUB_STEPS >> 1)) {
            int32_t steps = 1 + (k * 2) / 127;

            if (o->last_time == 0xffu)
                o->last_time = fm_dj_time_quantize_cc(vp->dly_time, bpm);
            vp->dly_time = fm_dj_time_step_div(o->last_time, bpm, steps);
            o->last_time = vp->dly_time;
        } else if (o->last_time != 0xffu)
            vp->dly_time = o->last_time;
        vp->loop_filt = fm_dj_clamp_u8((int32_t)vp->loop_filt + (k * 28) / 127, 0, 127);
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (k * 36) / 127, 0, 127);
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (k * 22) / 127, 0, 127);
        break;
    case FM_DJ_FX_SCENE_TAPE_ECHO:
        if (bar_step == 0u) {
            int32_t steps = 1 + k / 127;

            if (o->last_time == 0xffu)
                o->last_time = fm_dj_time_quantize_cc(vp->dly_time, bpm);
            vp->dly_time = fm_dj_time_step_div(o->last_time, bpm, steps);
            o->last_time = vp->dly_time;
        } else if (o->last_time != 0xffu)
            vp->dly_time = o->last_time;
        vp->loop_filt = fm_dj_clamp_u8((int32_t)vp->loop_filt + (k * 34) / 127, 0, 127);
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (k * 24) / 127, 0, 127);
        vp->wow = fm_dj_clamp_u8((int32_t)vp->wow + (k * 12) / 127, 0, 127);
        break;
    case FM_DJ_FX_SCENE_PONG_THROW:
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (k * 40) / 127, 0, 127);
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (k * 26) / 127, 0, 127);
        vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev + (k * 18) / 127, 0, 127);
        vp->drm_pan[1] = fm_dj_clamp_u8((int32_t)vp->drm_pan[1] + (k * 16) / 127, 1, 127);
        vp->drm_pan[2] = fm_dj_clamp_u8((int32_t)vp->drm_pan[2] - (k * 16) / 127, 1, 127);
        break;
    case FM_DJ_FX_SCENE_REV_WASH:
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + (k * 38) / 127, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail + (k * 32) / 127, 0, 127);
        vp->rev_damp = fm_dj_clamp_u8((int32_t)vp->rev_damp + (k * 14) / 127, 0, 127);
        vp->tank = fm_dj_clamp_u8((int32_t)vp->tank + (k * 24) / 127, 0, 127);
        vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev + (k * 20) / 127, 0, 127);
        break;
    case FM_DJ_FX_SCENE_FREEZE_HOLD:
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (k * 18) / 127, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail + (k * 22) / 127, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + (k * 16) / 127, 0, 127);
        break;
    case FM_DJ_FX_SCENE_ISO_FILTER:
        vp->isolator = fm_dj_clamp_u8((int32_t)vp->isolator + (k * 30) / 127, 0, 127);
        vp->loop_filt = fm_dj_clamp_u8((int32_t)vp->loop_filt + (k * 22) / 127, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + (k * 18) / 127, 0, 127);
        break;
    default:
        break;
    }
}

static inline void fm_dj_fx_apply(dub_params_t *vp, const fm_dj_t *dj, fm_dj_fx_ov_t *ov, uint8_t bar_step)
{
    uint8_t si;
    uint8_t best_si = 0xffu;
    int32_t best_str = FM_DJ_TOAST_AUDIBLE - 1;

    if (!dj->on && !dj->slew_off)
        return;
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_slot_t *s = &dj->slot[si];
        const fm_dj_fx_slot_ov_t *o = &ov->per[si];
        int32_t str;

        if (!o->active || o->scene == FM_DJ_FX_SCENE_NONE)
            continue;
        str = fm_dj_fx_strength(s);
        if (str > best_str) {
            best_str = str;
            best_si = si;
        }
    }
    if (best_si == 0xffu)
        return;
    fm_dj_fx_apply_scene(vp, ov->per[best_si].scene, best_str, bar_step, &ov->per[best_si]);
}

static inline int32_t fm_dj_fx_u8_delta(uint8_t eff, uint8_t manual)
{
    int32_t d = (int32_t)eff - (int32_t)manual;

    if (d < 0)
        d = -d;
    return d;
}

/* Effective vs manual morph base: toast/UI only when param actually moved. */
static inline int fm_dj_fx_scene_param_audible(uint8_t scene, const dub_params_t *eff,
                                               const dub_params_t *manual)
{
    if (!eff || !manual)
        return 1;
    switch (scene) {
    case FM_DJ_FX_SCENE_DUB_ECHO:
        return fm_dj_fx_u8_delta(eff->dly_lvl, manual->dly_lvl) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->feedback, manual->feedback) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->loop_filt, manual->loop_filt) >= FM_DJ_TOAST_AUDIBLE;
    case FM_DJ_FX_SCENE_TAPE_ECHO:
        return fm_dj_fx_u8_delta(eff->dly_lvl, manual->dly_lvl) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->loop_filt, manual->loop_filt) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->wow, manual->wow) >= FM_DJ_TOAST_AUDIBLE;
    case FM_DJ_FX_SCENE_PONG_THROW:
        return fm_dj_fx_u8_delta(eff->dly_lvl, manual->dly_lvl) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->feedback, manual->feedback) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->dly_rev, manual->dly_rev) >= FM_DJ_TOAST_AUDIBLE;
    case FM_DJ_FX_SCENE_REV_WASH:
        return fm_dj_fx_u8_delta(eff->rev_mix, manual->rev_mix) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->rev_tail, manual->rev_tail) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->tank, manual->tank) >= FM_DJ_TOAST_AUDIBLE;
    case FM_DJ_FX_SCENE_FREEZE_HOLD:
        return fm_dj_fx_u8_delta(eff->feedback, manual->feedback) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->rev_tail, manual->rev_tail) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->rev_mix, manual->rev_mix) >= FM_DJ_TOAST_AUDIBLE;
    case FM_DJ_FX_SCENE_ISO_FILTER:
        return fm_dj_fx_u8_delta(eff->isolator, manual->isolator) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->loop_filt, manual->loop_filt) >= FM_DJ_TOAST_AUDIBLE ||
               fm_dj_fx_u8_delta(eff->cutoff_c, manual->cutoff_c) >= FM_DJ_TOAST_AUDIBLE;
    default:
        return 0;
    }
}

/* FX bank knob overlay highlight (TIME stays note-only via echo_time_live elsewhere). */
static inline int fm_dj_fx_knob_overlay_live(const fm_dj_fx_ov_t *ov, uint8_t fx_pg, uint8_t slot)
{
    uint8_t si;

    if (!ov)
        return 0;
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_fx_slot_ov_t *o = &ov->per[si];

        if (!o->active)
            continue;
        switch (o->scene) {
        case FM_DJ_FX_SCENE_DUB_ECHO:
            if (fx_pg == 0u && (slot == 0u || slot == 2u || slot == 3u))
                return 1;
            break;
        case FM_DJ_FX_SCENE_TAPE_ECHO:
            if (fx_pg == 0u && (slot == 0u || slot == 3u))
                return 1;
            if (fx_pg == 1u && slot == 0u)
                return 1;
            break;
        case FM_DJ_FX_SCENE_PONG_THROW:
            if (fx_pg == 0u && (slot == 0u || slot == 2u))
                return 1;
            if (fx_pg == 2u && slot == 3u)
                return 1;
            break;
        case FM_DJ_FX_SCENE_REV_WASH:
            if (fx_pg == 1u && slot == 2u)
                return 1;
            if (fx_pg == 2u && slot <= 2u)
                return 1;
            break;
        case FM_DJ_FX_SCENE_FREEZE_HOLD:
            if (fx_pg == 0u && slot == 2u)
                return 1;
            if (fx_pg == 2u && slot <= 2u)
                return 1;
            break;
        case FM_DJ_FX_SCENE_ISO_FILTER:
            if (fx_pg == 1u && slot == 3u)
                return 1;
            if (fx_pg == 0u && slot == 3u)
                return 1;
            break;
        default:
            break;
        }
    }
    return 0;
}

static inline const char *fm_dj_fx_scene_toast(uint8_t scene)
{
    switch (scene) {
    case FM_DJ_FX_SCENE_DUB_ECHO:
        return "ECHO TIME";
    case FM_DJ_FX_SCENE_TAPE_ECHO:
        return "ECHO TONE";
    case FM_DJ_FX_SCENE_PONG_THROW:
        return "ECHO MIX";
    case FM_DJ_FX_SCENE_REV_WASH:
        return "REVERB TAIL";
    case FM_DJ_FX_SCENE_FREEZE_HOLD:
        return "ECHO FEEDBACK";
    case FM_DJ_FX_SCENE_ISO_FILTER:
        return "ISOLATOR";
    default:
        return 0;
    }
}

static inline const char *fm_dj_fx_toast_label_vp(const fm_dj_t *dj, const fm_dj_fx_ov_t *ov,
                                                  const dub_params_t *eff, const dub_params_t *manual)
{
    uint8_t si;
    int32_t best = FM_DJ_TOAST_AUDIBLE;
    const char *msg = 0;
    uint8_t echo_scene = 0u;
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_slot_t *s = &dj->slot[si];
        const fm_dj_fx_slot_ov_t *o = &ov->per[si];
        int32_t ad;

        if (!o->active)
            continue;
        ad = fm_dj_fx_strength(s);
        if (ad > best) {
            const char *m = fm_dj_fx_scene_toast(o->scene);

            if (m && fm_dj_fx_scene_param_audible(o->scene, eff, manual)) {
                best = ad;
                msg = m;
                echo_scene = fm_dj_fx_echo_time_scene(o->scene);
            }
        }
    }
    if (msg && echo_scene && eff && eff->dly_time != 0u) {
        static char buf[24];
        const char *nd = fm_time_note_div(eff->dly_time, eff->bpm ? eff->bpm : 120u);
        uint32_t i = 0u;

        if (nd) {
            while (*msg && i + 2u < sizeof buf)
                buf[i++] = *msg++;
            buf[i++] = ' ';
            while (*nd && i + 1u < sizeof buf)
                buf[i++] = *nd++;
            buf[i] = 0;
            return buf;
        }
    }
    return msg;
}

static inline const char *fm_dj_fx_toast_label(const fm_dj_t *dj, const fm_dj_fx_ov_t *ov)
{
    return fm_dj_fx_toast_label_vp(dj, ov, 0, 0);
}

#define FM_DJ_FX_WIRED 1

#endif /* FM_DJ_FX_H */
