/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ phrase climax (variable buildup / break / DROP indices). */
#ifndef FM_DJ_CLIMAX_H
#define FM_DJ_CLIMAX_H

#include "dub_types.h"
#include "fm_dj.h"
#include "fm_dj_time.h"

#if !defined(GENRE_JAZZ)

static inline void fm_dj_apply_drop_emphasis(dub_params_t *vp, uint8_t lvl)
{
    int32_t kboost;
    int32_t sboost;

    if (lvl == 0u)
        return;
    kboost = ((int32_t)(127 - vp->drm_dens[0]) * (int32_t)lvl) / 127;
    vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] + kboost, 0, 127);
    sboost = ((int32_t)(127 - vp->drm_dens[1]) * (int32_t)lvl * 11) / (127 * 10);
    vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + sboost, 0, 127);
    vp->kick_boom = fm_dj_clamp_u8((int32_t)vp->kick_boom + ((int32_t)lvl * 72) / 127, 0, 127);
    vp->rim_tone = fm_dj_clamp_u8((int32_t)vp->rim_tone + ((int32_t)lvl * 48) / 127, 0, 127);
    vp->density = fm_dj_clamp_u8((int32_t)vp->density + ((int32_t)lvl * 42) / 127, 0, 127);
}

static inline void fm_dj_drop_cut_build_fx(dub_params_t *vp, uint8_t build_arc)
{
    vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback - 110, 0, 127);
    vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev - 90, 0, 127);
    if (build_arc == FM_DJ_BUILD_WASH) {
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix - 95, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail - 70, 0, 127);
    } else if (build_arc == FM_DJ_BUILD_ISO_TANK) {
        vp->tank = fm_dj_clamp_u8((int32_t)vp->tank - 105, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail - 80, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix - 90, 0, 127);
        vp->isolator = fm_dj_clamp_u8(64 + ((int32_t)vp->isolator - 64) / 4, 0, 127);
        vp->rev_damp = fm_dj_clamp_u8((int32_t)vp->rev_damp + 28, 0, 127);
    } else {
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix - 100, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail - 85, 0, 127);
    }
}

static inline void fm_dj_apply_build_climb(dub_params_t *vp, const fm_dj_t *dj, int32_t climb)
{
    uint8_t arc;
    uint8_t pb;
    uint8_t i;

    if (climb <= 0)
        return;
    arc = dj->build_arc;
    if (arc >= FM_DJ_BUILD_N)
        arc = FM_DJ_BUILD_THIN;
    pb = dj->phrase_bar;

    switch (arc) {
    case FM_DJ_BUILD_WASH:
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + (climb * 72) / 127, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail + (climb * 58) / 127, 0, 127);
        vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev + (climb * 48) / 127, 0, 127);
        vp->tank = fm_dj_clamp_u8((int32_t)vp->tank + (climb * 36) / 127, 0, 127);
        vp->dly_c = fm_dj_clamp_u8((int32_t)vp->dly_c + (climb * 28) / 127, 0, 127);
        for (i = 0u; i < 4u; i++)
            vp->drm_rev[i] = fm_dj_clamp_u8((int32_t)vp->drm_rev[i] + (climb * 22) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - (climb * 18) / 127, 0, 127);
        if (pb >= dj->break_arm)
            vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + 24, 0, 127);
        break;
    case FM_DJ_BUILD_ISO_TANK: {
        int32_t iso_push;
        uint8_t sweep;

        /* Broad → band emphasis; alternate LP/HP sweep by phrase bar (additive on morph base). */
        sweep = (uint8_t)((pb - dj->build_start) & 3u);
        iso_push = (climb * 40) / 127;
        if (sweep < 2u)
            vp->isolator = fm_dj_clamp_u8((int32_t)vp->isolator - iso_push, 18, 127);
        else
            vp->isolator = fm_dj_clamp_u8((int32_t)vp->isolator + iso_push, 0, 112);
        vp->tank = fm_dj_clamp_u8((int32_t)vp->tank + (climb * 50) / 127, 0, 112);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail + (climb * 52) / 127, 0, 110);
        vp->rev_damp = fm_dj_clamp_u8((int32_t)vp->rev_damp + (climb * 34) / 127, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + (climb * 22) / 127, 0, 90);
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (climb * 18) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - (climb * 14) / 127, 0, 127);
        vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] - (climb * 20) / 127, 0, 127);
        if (pb >= dj->break_arm - 1u)
            vp->tank = fm_dj_clamp_u8((int32_t)vp->tank + 12, 0, 112);
        break;
    }
    case FM_DJ_BUILD_MOTION_RISE:
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + (climb * 44) / 127, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + (climb * 36) / 127, 0, 127);
        vp->pump = fm_dj_clamp_u8((int32_t)vp->pump + (climb * 38) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + (climb * 24) / 127, 0, 127);
        vp->drm_dens[2] = fm_dj_clamp_u8((int32_t)vp->drm_dens[2] + (climb * 28) / 127, 0, 127);
        vp->drm_dens[3] = fm_dj_clamp_u8((int32_t)vp->drm_dens[3] + (climb * 22) / 127, 0, 127);
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (climb * 14) / 127, 0, 127);
        break;
    case FM_DJ_BUILD_DUB_ECHO:
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (climb * 58) / 127, 0, 127);
        vp->dly_time = fm_dj_time_climb_div(vp->dly_time, vp->bpm ? vp->bpm : 120u, climb);
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (climb * 42) / 127, 0, 127);
        vp->loop_filt = fm_dj_clamp_u8((int32_t)vp->loop_filt + (climb * 30) / 127, 0, 127);
        vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev + (climb * 22) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - (climb * 10) / 127, 0, 127);
        break;
    case FM_DJ_BUILD_TRACK_LIFT:
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + (climb * 46) / 127, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + (climb * 44) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + (climb * 32) / 127, 0, 127);
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + (climb * 28) / 127, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + (climb * 20) / 127, 0, 127);
        vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + (climb * 26) / 127, 0, 127);
        break;
    case FM_DJ_BUILD_FB_PONG:
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (climb * 62) / 127, 0, 127);
        if (climb >= 100)
            vp->feedback = 127u; /* maps to capped regen in dub_dsp (106=unity, top=tapered) */
        vp->dly_time = fm_dj_time_climb_div(vp->dly_time, vp->bpm ? vp->bpm : 120u, climb);
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (climb * 34) / 127, 0, 127);
        vp->loop_filt = fm_dj_clamp_u8((int32_t)vp->loop_filt + (climb * 24) / 127, 0, 127);
        vp->drm_pan[1] = fm_dj_clamp_u8((int32_t)vp->drm_pan[1] + (climb * 20) / 127, 1, 127);
        vp->drm_pan[2] = fm_dj_clamp_u8((int32_t)vp->drm_pan[2] - (climb * 20) / 127, 1, 127);
        vp->isolator = fm_dj_clamp_u8((int32_t)vp->isolator + (climb * 30) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - (climb * 12) / 127, 0, 127);
        break;
    default: /* FM_DJ_BUILD_THIN */
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + (climb * 62) / 127, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + (climb * 40) / 127, 0, 127);
        vp->pump = fm_dj_clamp_u8((int32_t)vp->pump + (climb * 32) / 127, 0, 127);
        vp->hat_lvl = fm_dj_clamp_u8((int32_t)vp->hat_lvl + (climb * 28) / 127, 0, 127);
        vp->rim_tone = fm_dj_clamp_u8((int32_t)vp->rim_tone + (climb * 22) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + (climb * 20) / 127, 0, 127);
        vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] - (climb * 48) / 127, 0, 127);
        vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + (climb * 26) / 127, 0, 127);
        vp->drm_dens[2] = fm_dj_clamp_u8((int32_t)vp->drm_dens[2] + (climb * 30) / 127, 0, 127);
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (climb * 12) / 127, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + (climb * 18) / 127, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + (climb * 16) / 127, 0, 127);
        break;
    }
}

static inline void fm_dj_apply_phrase_climax(dub_params_t *vp, const fm_dj_t *dj,
                                              uint8_t drop_emph_rem, uint8_t drop_emph_span)
{
    int32_t climb;
    uint8_t i;
    uint8_t emph;

    if (!dj->on)
        return;

    climb = (int32_t)fm_dj_phrase_climb(dj);
    if (climb > 0)
        fm_dj_apply_build_climb(vp, dj, climb);
    else if (dj->phrase_bar == dj->break_arm) {
        int32_t tail = 55; /* ~1 bar pre-drop swell */

        fm_dj_apply_build_climb(vp, dj, tail);
    }

    if (fm_dj_any_slot_role(dj, FM_DJ_ROLE_DROP)) {
        fm_dj_drop_cut_build_fx(vp, dj->build_arc);
        vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] +
                                         ((127 - (int32_t)vp->drm_dens[0]) * (int32_t)FM_DJ_DROP_EMPH_PEAK) / 127, 0, 127);
        vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + 85, 0, 127);
        vp->drm_dens[2] = fm_dj_clamp_u8((int32_t)vp->drm_dens[2] + 80, 0, 127);
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + 58, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + 42, 0, 127);
        vp->pump = fm_dj_clamp_u8((int32_t)vp->pump + 55, 0, 127);
        vp->kick_boom = fm_dj_clamp_u8((int32_t)vp->kick_boom + ((int32_t)FM_DJ_DROP_EMPH_PEAK * 72) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + 70, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + 45, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + 45, 0, 127);
        vp->hat_lvl = fm_dj_clamp_u8((int32_t)vp->hat_lvl + 35, 0, 127);
        vp->rim_tone = fm_dj_clamp_u8((int32_t)vp->rim_tone + 55, 0, 127);
    } else if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_BREAK) && !fm_dj_build_tail_hold(dj)) {
        vp->drm_dens[0] = 0u;
        for (i = 1u; i < 4u; i++)
            vp->drm_dens[i] = fm_dj_clamp_u8((int32_t)vp->drm_dens[i] - 95, 0, 127);
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b - 72, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c - 58, 0, 127);
        vp->pump = fm_dj_clamp_u8((int32_t)vp->pump - 70, 0, 127);
        vp->kick_boom = fm_dj_clamp_u8((int32_t)vp->kick_boom - 70, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + 48, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - 90, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b - 85, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c - 85, 0, 127);
        vp->hat_lvl = fm_dj_clamp_u8((int32_t)vp->hat_lvl - 60, 0, 127);
    } else {
        emph = fm_dj_drop_emph_level(drop_emph_rem, drop_emph_span);
        fm_dj_apply_drop_emphasis(vp, emph);
    }
}

#else /* GENRE_JAZZ */

static inline void fm_dj_apply_drop_emphasis(dub_params_t *vp, uint8_t lvl)
{
    int32_t kboost;
    int32_t rboost;
    int32_t rdboost;

    if (lvl == 0u)
        return;
    kboost = ((int32_t)(127 - vp->drm_dens[0]) * (int32_t)lvl) / 127;
    vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] + kboost, 0, 127);
    rboost = ((int32_t)(127 - vp->drm_dens[1]) * (int32_t)lvl * 9) / (127 * 10);
    vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + rboost, 0, 127);
    rdboost = ((int32_t)(127 - vp->drm_dens[2]) * (int32_t)lvl) / (127 * 2);
    vp->drm_dens[2] = fm_dj_clamp_u8((int32_t)vp->drm_dens[2] + rdboost, 0, 127);
    vp->density = fm_dj_clamp_u8((int32_t)vp->density + ((int32_t)lvl * 36) / 127, 0, 127);
    vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + ((int32_t)lvl * 28) / 127, 0, 127);
    vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + ((int32_t)lvl * 32) / 127, 0, 127);
}

static inline void fm_dj_drop_cut_build_fx(dub_params_t *vp, uint8_t build_arc)
{
    vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback - 90, 0, 127);
    vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev - 70, 0, 127);
    if (build_arc == FM_DJ_BUILD_WASH) {
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix - 80, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail - 55, 0, 127);
    } else {
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix - 85, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail - 65, 0, 127);
    }
}

static inline void fm_dj_apply_build_climb(dub_params_t *vp, const fm_dj_t *dj, int32_t climb)
{
    uint8_t arc;
    uint8_t pb;
    uint8_t i;

    if (climb <= 0)
        return;
    arc = dj->build_arc;
    if (arc >= FM_DJ_BUILD_N || arc == FM_DJ_BUILD_ISO_TANK)
        arc = FM_DJ_BUILD_WASH;
    pb = dj->phrase_bar;

    switch (arc) {
    case FM_DJ_BUILD_WASH:
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + (climb * 68) / 127, 0, 127);
        vp->rev_tail = fm_dj_clamp_u8((int32_t)vp->rev_tail + (climb * 52) / 127, 0, 127);
        vp->dly_rev = fm_dj_clamp_u8((int32_t)vp->dly_rev + (climb * 44) / 127, 0, 127);
        vp->dly_c = fm_dj_clamp_u8((int32_t)vp->dly_c + (climb * 30) / 127, 0, 127);
        for (i = 0u; i < 4u; i++)
            vp->drm_rev[i] = fm_dj_clamp_u8((int32_t)vp->drm_rev[i] + (climb * 18) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - (climb * 12) / 127, 0, 127);
        if (pb >= dj->break_arm)
            vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + 18, 0, 127);
        break;
    case FM_DJ_BUILD_FB_PONG:
        vp->feedback = fm_dj_clamp_u8((int32_t)vp->feedback + (climb * 48) / 127, 0, 127);
        vp->dly_time = fm_dj_time_climb_div(vp->dly_time, vp->bpm ? vp->bpm : 120u, climb);
        vp->dly_lvl = fm_dj_clamp_u8((int32_t)vp->dly_lvl + (climb * 28) / 127, 0, 127);
        vp->loop_filt = fm_dj_clamp_u8((int32_t)vp->loop_filt + (climb * 18) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - (climb * 10) / 127, 0, 127);
        break;
    case FM_DJ_BUILD_MOTION_RISE:
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + (climb * 40) / 127, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + (climb * 32) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + (climb * 20) / 127, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + (climb * 18) / 127, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + (climb * 16) / 127, 0, 127);
        break;
    default:
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + (climb * 48) / 127, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + (climb * 36) / 127, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + (climb * 16) / 127, 0, 127);
        vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] - (climb * 32) / 127, 0, 127);
        vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + (climb * 22) / 127, 0, 127);
        vp->drm_dens[2] = fm_dj_clamp_u8((int32_t)vp->drm_dens[2] + (climb * 26) / 127, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + (climb * 14) / 127, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + (climb * 12) / 127, 0, 127);
        break;
    }
}

static inline void fm_dj_apply_phrase_climax(dub_params_t *vp, const fm_dj_t *dj,
                                              uint8_t drop_emph_rem, uint8_t drop_emph_span)
{
    int32_t climb;
    uint8_t i;
    uint8_t emph;

    if (!dj->on)
        return;

    climb = (int32_t)fm_dj_phrase_climb(dj);
    if (climb > 0)
        fm_dj_apply_build_climb(vp, dj, climb);
    else if (dj->phrase_bar == dj->break_arm) {
        int32_t tail = 48;

        fm_dj_apply_build_climb(vp, dj, tail);
    }

    if (fm_dj_any_slot_role(dj, FM_DJ_ROLE_DROP)) {
        fm_dj_drop_cut_build_fx(vp, dj->build_arc);
        vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] +
                                         ((127 - (int32_t)vp->drm_dens[0]) * (int32_t)FM_DJ_DROP_EMPH_PEAK) / 127, 0, 127);
        vp->drm_dens[1] = fm_dj_clamp_u8((int32_t)vp->drm_dens[1] + 72, 0, 127);
        vp->drm_dens[2] = fm_dj_clamp_u8((int32_t)vp->drm_dens[2] + 68, 0, 127);
        vp->drm_dens[3] = fm_dj_clamp_u8((int32_t)vp->drm_dens[3] + 55, 0, 127);
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b + 42, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c + 38, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density + 55, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b + 40, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c + 42, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + 28, 0, 127);
    } else if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_BREAK) && !fm_dj_build_tail_hold(dj)) {
        vp->drm_dens[0] = fm_dj_clamp_u8((int32_t)vp->drm_dens[0] - 70, 0, 127);
        for (i = 1u; i < 4u; i++)
            vp->drm_dens[i] = fm_dj_clamp_u8((int32_t)vp->drm_dens[i] - 72, 0, 127);
        vp->cutoff_b = fm_dj_clamp_u8((int32_t)vp->cutoff_b - 48, 0, 127);
        vp->cutoff_c = fm_dj_clamp_u8((int32_t)vp->cutoff_c - 40, 0, 127);
        vp->rev_mix = fm_dj_clamp_u8((int32_t)vp->rev_mix + 55, 0, 127);
        vp->density = fm_dj_clamp_u8((int32_t)vp->density - 65, 0, 127);
        vp->lvl_b = fm_dj_clamp_u8((int32_t)vp->lvl_b - 55, 0, 127);
        vp->lvl_c = fm_dj_clamp_u8((int32_t)vp->lvl_c - 50, 0, 127);
    } else {
        emph = fm_dj_drop_emph_level(drop_emph_rem, drop_emph_span);
        fm_dj_apply_drop_emphasis(vp, emph);
    }
}

#endif

#endif /* FM_DJ_CLIMAX_H */
