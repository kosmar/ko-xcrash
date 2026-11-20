/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ mode: additive delta layer after morph + user LFO (header-only). */
#ifndef FM_DJ_H
#define FM_DJ_H

#include <stdint.h>

enum {
    FM_DJ_ROLE_RIDE = 0,
    FM_DJ_ROLE_THROW,
    FM_DJ_ROLE_FILTER,
    FM_DJ_ROLE_BREAK,
    FM_DJ_ROLE_DROP,
    FM_DJ_ROLE_N
};

/* Phrase-long buildup shape — one archetype per drop cycle (variable phrase_len). */
enum {
    FM_DJ_BUILD_THIN = 0,
    FM_DJ_BUILD_WASH,
    FM_DJ_BUILD_FB_PONG,
    FM_DJ_BUILD_ISO_TANK,
    FM_DJ_BUILD_MOTION_RISE, /* shared: MOTION + FLUX climb */
    FM_DJ_BUILD_DUB_ECHO,    /* dub ROM: DUB + FB echo climb */
    FM_DJ_BUILD_TRACK_LIFT,  /* pop: TRACK stack climb */
    FM_DJ_BUILD_N
};

/* End-of-phrase transition (stable for whole phrase_len). */
enum {
    FM_DJ_TRANS_DROP = 0,
    FM_DJ_TRANS_CROSSFADE = 1,
    FM_DJ_TRANS_CUEING = 2,
    FM_DJ_TRANS_ECHO_OUT,   /* shared: SPACE → DUB throw, then XF */
    FM_DJ_TRANS_FILTER_SLAM, /* techno: FILTER → DROP */
    FM_DJ_TRANS_N = 5
};

enum {
    FM_DJ_TGT_MOTION = 0,
    FM_DJ_TGT_FLUX,
    FM_DJ_TGT_WOB,
    FM_DJ_TGT_ISO,
    FM_DJ_TGT_SPACE,
    FM_DJ_TGT_WEIGHT,
    FM_DJ_TGT_DUB,
    FM_DJ_TGT_CUTOFF,
    FM_DJ_TGT_FB,
    FM_DJ_TGT_DENS_H,
    FM_DJ_TGT_SEQ_ROT,
    FM_DJ_TGT_SEQ_VEL,
    FM_DJ_TGT_SEQ_TRIG,
    FM_DJ_TGT_SEQ_GATE,
    FM_DJ_TGT_SEQ_PITCH,
    FM_DJ_TGT_DENS_K,
    FM_DJ_TGT_ENSEMBLE,
    FM_DJ_TGT_FLIP,
    FM_DJ_TGT_SEQ_CYC,
    FM_DJ_TGT_SEQ_REP,
    FM_DJ_TGT_N
};

#define FM_DJ_SLOT_LANE_GLOBAL 0xffu

/* Concurrent gestures — each slot has its own attack / hold / release. */
#ifndef FM_DJ_SLOT_N
#define FM_DJ_SLOT_N 4u
#endif

enum {
    FM_DJ_SLOT_OFF = 0,
    FM_DJ_SLOT_ATTACK,
    FM_DJ_SLOT_HOLD,
    FM_DJ_SLOT_RELEASE
};

typedef struct {
    uint8_t phase;
    uint8_t target;
    uint8_t role;
    uint8_t clock_div;   /* attack slew */
    uint8_t release_div; /* release slew (typically slower) */
    uint8_t hold_bars;   /* bars left at peak (current bar included at spawn) */
    int8_t peak;
    int8_t delta;
    uint8_t lane; /* FM_DJ_SLOT_LANE_GLOBAL or 0..5 for SEQ param targets */
} fm_dj_slot_t;

typedef struct {
    uint8_t on;
    uint8_t phrase_bar; /* 0..phrase_len-1 */
    uint8_t phrase_len; /* bars per drop cycle (16|20|24|32) */
    uint8_t build_start;
    uint8_t build_end;   /* last bar with audible climb */
    uint8_t break_arm;   /* bar index: arm BREAK / tail */
    uint8_t drop_arm;    /* bar index: arm DROP (phrase_len-1) */
    uint8_t last_drop_span; /* post-drop emph bars from previous DROP */
    uint8_t role;       /* UI: latest gesture */
    uint8_t target;
    uint8_t energy;
    uint8_t bar_pending;
    uint8_t trans_pending; /* 0=none; else FM_DJ_TRANS_*+1 at phrase end */
    uint8_t trans_type;    /* planned transition for current phrase */
    uint8_t last_trans_type;
    uint8_t drop_armed; /* 1 until ENC_PRESETS clears auto-drop for phrase */
    uint8_t fill_blocked;
    uint8_t build_arc; /* FM_DJ_BUILD_* — stable for current phrase */
    uint8_t ride_hog; /* 0=off; 1=phrase plans hog; 2=ride FX hog consumed */
    uint8_t drop_sil_full; /* 1 = full BREAK bar silent before DROP; 0 = last half only */
    uint8_t drop_sil_keep; /* 0xff=all muted; 0..5 single dry lane; 0xfe=unbound */
    uint8_t drop_sil_lane_pend; /* 1 until app binds audible lanes at break bar */
    uint8_t slew_off;
    uint8_t last_role;
    uint8_t prev_role;
    uint8_t lead_slot; /* index of newest gesture for UI */
    int8_t hold;       /* UI mirror of lead peak */
    int8_t delta;      /* UI mirror of lead slot delta */
    fm_dj_slot_t slot[FM_DJ_SLOT_N];
    uint32_t rng;
    uint32_t phase;
} fm_dj_t;

static inline uint32_t fm_dj_rng_u8(fm_dj_t *dj)
{
    dj->rng = dj->rng * 1664525u + 1013904223u;
    return (dj->rng >> 16) & 0xffu;
}

static inline uint8_t fm_dj_clamp_u8(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo)
        return (uint8_t)lo;
    if (v > hi)
        return (uint8_t)hi;
    return (uint8_t)v;
}

/* Post-DROP kick/rim tail: starts at FM_DJ_DROP_EMPH_PEAK on the drop and fades over the whole span. */
#define FM_DJ_DROP_EMPH_PEAK 96u
/* Phrase spacing still counts the old 6..9 bar tail; the longer fade may run into the next rest bars. */
#define FM_DJ_DROP_EMPH_LAYOUT_MAX 9u

static inline uint8_t fm_dj_drop_emph_span_phrase(uint8_t rng4)
{
    return (uint8_t)(10u + (rng4 & 3u)); /* 10..13 bars */
}

static inline uint8_t fm_dj_phrase_wrap(const fm_dj_t *dj, uint8_t pb)
{
    uint8_t len = dj->phrase_len ? dj->phrase_len : 16u;

    pb++;
    if (pb >= len)
        return 0u;
    return pb;
}

static inline uint8_t fm_dj_phrase_is_build(const fm_dj_t *dj, uint8_t pb)
{
    return (uint8_t)(pb >= dj->build_start && pb <= dj->build_end);
}

/* Map intro bar into 0..14 for spawn/role heuristics (stable per phrase). */
static inline uint8_t fm_dj_phrase_intro_map(const fm_dj_t *dj, uint8_t pb)
{
    uint8_t lim = dj->build_start ? dj->build_start : 8u;

    if (pb >= lim)
        return 14u;
    if (lim <= 1u)
        return pb;
    return (uint8_t)((pb * 14u) / (lim - 1u));
}

static inline uint8_t fm_dj_pick_phrase_len(fm_dj_t *dj, uint8_t min_len)
{
    static const uint8_t lens[4] = {16u, 20u, 24u, 32u};
    uint8_t r = (uint8_t)(fm_dj_rng_u8(dj) % 100u);
    uint8_t pick;
    uint8_t i;

    if (r < 15u)
        pick = 0u;
    else if (r < 35u)
        pick = 1u;
    else if (r < 80u)
        pick = 2u;
    else
        pick = 3u;
    if (lens[pick] < min_len) {
        for (i = 0u; i < 4u; i++) {
            if (lens[i] >= min_len)
                return lens[i];
        }
        return 32u;
    }
    return lens[pick];
}

static inline void fm_dj_layout_phrase(fm_dj_t *dj)
{
    uint8_t len;
    uint8_t drop_arm;
    uint8_t break_arm;
    uint8_t build_bars;
    uint8_t build_end;
    uint8_t build_start;
    uint8_t min_rest;
    uint8_t floor;

    min_rest = (uint8_t)(3u + (fm_dj_rng_u8(dj) % 4u)); /* 3..6 neutral bars after emph */
    floor = dj->last_drop_span + min_rest;
    len = fm_dj_pick_phrase_len(dj, (uint8_t)(floor + 8u)); /* drop spacing only — not buildup length */
    drop_arm = (uint8_t)(len - 1u);
    break_arm = (uint8_t)(len - 2u);
    build_end = (uint8_t)(break_arm - 1u);
    /* Pre-drop FX climb: fixed 2..4 bars before break (not scaled by phrase_len). */
    build_bars = (uint8_t)(2u + (fm_dj_rng_u8(dj) % 3u));
    build_start = (uint8_t)(build_end + 1u - build_bars);
    if (build_start > build_end)
        build_start = (build_end >= 2u) ? (uint8_t)(build_end - 1u) : 0u;

    dj->phrase_len = len;
    dj->drop_arm = drop_arm;
    dj->break_arm = break_arm;
    dj->build_start = build_start;
    dj->build_end = build_end;
}

static inline uint8_t fm_dj_drop_emph_level(uint8_t rem, uint8_t span)
{
    int32_t r;
    int32_t x;

    if (rem == 0u || span == 0u)
        return 0u;
    if (rem > span)
        rem = span;
    r = ((int32_t)rem * 127) / (int32_t)span;
    x = (r + (r * r) / 127) / 2; /* between linear and quadratic: long soft tail */
    x = (x * (int32_t)FM_DJ_DROP_EMPH_PEAK) / 127;
    return fm_dj_clamp_u8(x < 4 ? 4 : x, 0, 127);
}

static inline uint8_t fm_dj_energy_for_bar(const fm_dj_t *dj, uint8_t pb)
{
    uint8_t span;
    uint8_t t;

    if (pb < dj->build_start)
        return 70u;
    if (pb <= dj->build_end) {
        span = (uint8_t)(dj->build_end - dj->build_start + 1u);
        t = (uint8_t)(pb - dj->build_start);
        if (span <= 1u)
            return 127u;
        return (uint8_t)(70u + (t * 57u) / (span - 1u)); /* → 127 */
    }
    if (pb == dj->break_arm)
        return 35u;
    return 127u;
}

/* Phrase pre-drop tension during buildup: 0 or 70..127 — audible climb in fm_dj_climax.h. */
static inline uint8_t fm_dj_phrase_climb(const fm_dj_t *dj)
{
    uint8_t pb;

    if (!dj->on)
        return 0u;
    pb = dj->phrase_bar;
    if (!fm_dj_phrase_is_build(dj, pb))
        return 0u;
    return fm_dj_energy_for_bar(dj, pb);
}

#if defined(GENRE_JAZZ)
#define FM_DJ_BUILD_ISO_TANK_SUPPORTED 0
#else
#define FM_DJ_BUILD_ISO_TANK_SUPPORTED 1
#endif

static inline uint8_t fm_dj_build_iso_tank_supported(void)
{
    return FM_DJ_BUILD_ISO_TANK_SUPPORTED;
}

#if defined(GENRE_POP)
#define FM_DJ_BUILD_TRACK_LIFT_SUPPORTED 1
#else
#define FM_DJ_BUILD_TRACK_LIFT_SUPPORTED 0
#endif

#if defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP)
#define FM_DJ_TRANS_FILTER_SLAM_SUPPORTED 1
#else
#define FM_DJ_TRANS_FILTER_SLAM_SUPPORTED 0
#endif

#if !defined(GENRE_JAZZ) && !defined(GENRE_POP) && !defined(GENRE_HOUSE) && !defined(GENRE_TECHNO) \
    && !defined(GENRE_TRIPHOP) \
    && !defined(GENRE_FUNK) && !defined(GENRE_HIPHOP) && !defined(GENRE_IDM) \
    && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_SOUL)
#define FM_DJ_BUILD_DUB_ECHO_SUPPORTED 1
#else
#define FM_DJ_BUILD_DUB_ECHO_SUPPORTED 0
#endif

static inline uint8_t fm_dj_build_track_lift_supported(void)
{
    return FM_DJ_BUILD_TRACK_LIFT_SUPPORTED;
}

static inline uint8_t fm_dj_build_dub_echo_supported(void)
{
    return FM_DJ_BUILD_DUB_ECHO_SUPPORTED;
}

static inline uint8_t fm_dj_trans_filter_slam_supported(void)
{
    return FM_DJ_TRANS_FILTER_SLAM_SUPPORTED;
}

static inline uint8_t fm_dj_trans_echo_out_supported(void)
{
    return 1u;
}

static inline uint8_t fm_dj_build_motion_rise_supported(void)
{
    return 1u;
}

static inline uint8_t fm_dj_trans_is_drop_path(const fm_dj_t *dj)
{
    if (!dj->on)
        return 0u;
    return (uint8_t)(dj->trans_type == FM_DJ_TRANS_DROP || dj->trans_type == FM_DJ_TRANS_FILTER_SLAM);
}

/* Plate-only XF/ECHO turns match manual left preset (era stays unless era-step fires). */
static inline int32_t fm_dj_trans_plate_dir(uint8_t trans)
{
    if (trans == FM_DJ_TRANS_CROSSFADE || trans == FM_DJ_TRANS_ECHO_OUT)
        return -1;
    return 0;
}

/* Era direction when a DJ transition steps era (same sign mix as DROP cycle_era). */
static inline int32_t fm_dj_trans_era_dir(uint8_t rng4)
{
    return (rng4 == 2u) ? 1 : -1;
}

/* Wrap era index for genre era count (cycle_era / ENC_ALGORITHM). */
static inline uint8_t fm_dj_trans_era_after(uint8_t era, int32_t dir, uint8_t era_n)
{
    int32_t v;

    if (!era_n)
        return era;
    v = (int32_t)era + dir;
    while (v < 0)
        v += (int32_t)era_n;
    return (uint8_t)((uint32_t)v % era_n);
}

/* DJ transition era target: genre eras only (artist_ix == genre_era_n, never returned). */
static inline uint8_t fm_dj_trans_era_step_next(uint8_t era, int32_t dir, uint8_t genre_era_n,
                                                uint8_t artist_era_ix)
{
    uint8_t base;

    if (!genre_era_n)
        return era;
    if (era == artist_era_ix)
        base = (dir > 0) ? (uint8_t)(genre_era_n - 1u) : 0u;
    else
        base = era;
    return fm_dj_trans_era_after(base, dir, genre_era_n);
}

/* 1 = CROSSFADE / ECHO OUT land on era step instead of plate-only (~1/8). */
static inline uint8_t fm_dj_trans_era_step_picked(uint8_t trans, uint8_t xf_seq)
{
    if (trans != FM_DJ_TRANS_CROSSFADE && trans != FM_DJ_TRANS_ECHO_OUT)
        return 0u;
    return (uint8_t)((xf_seq & 7u) == 7u);
}

/* REVERB UP / ISO+TANK: pre-drop dry gate + FX tail (not classic drum thin). */
static inline uint8_t fm_dj_build_tail_hold(const fm_dj_t *dj)
{
    if (!dj->on)
        return 0u;
    if (dj->build_arc == FM_DJ_BUILD_WASH || dj->build_arc == FM_DJ_BUILD_ISO_TANK)
        return 1u;
    return 0u;
}

#define FM_DJ_DROP_SIL_KEEP_ALL 0xffu
#define FM_DJ_DROP_SIL_KEEP_PEND 0xfeu

static inline void fm_dj_drop_sil_reset(fm_dj_t *dj)
{
    dj->drop_sil_full = 0u;
    dj->drop_sil_keep = FM_DJ_DROP_SIL_KEEP_ALL;
    dj->drop_sil_lane_pend = 0u;
}

static inline void fm_dj_drop_sil_plan_pick(fm_dj_t *dj)
{
    uint8_t r = (uint8_t)(fm_dj_rng_u8(dj) % 20u);

    dj->drop_sil_keep = FM_DJ_DROP_SIL_KEEP_ALL;
    dj->drop_sil_lane_pend = 0u;
    if (r < 5u)
        dj->drop_sil_full = 1u;
    else if (r < 12u)
        dj->drop_sil_full = 0u;
    else {
        dj->drop_sil_full = (uint8_t)((r >> 1) & 1u);
        dj->drop_sil_keep = FM_DJ_DROP_SIL_KEEP_PEND;
        dj->drop_sil_lane_pend = 1u;
    }
}

static inline uint8_t fm_dj_pick_trans_type(fm_dj_t *dj)
{
    uint8_t r = (uint8_t)(fm_dj_rng_u8(dj) % 100u);
    uint8_t t;

    /* Stolen from CUEING tail (was 25%) — ECHO OUT is shared alternate. */
    if (r < 38u)
        t = FM_DJ_TRANS_DROP;
    else if (r < 70u)
        t = FM_DJ_TRANS_CROSSFADE;
    else if (r < 88u)
        t = FM_DJ_TRANS_CUEING;
    else if (fm_dj_trans_echo_out_supported())
        t = FM_DJ_TRANS_ECHO_OUT;
    else
        t = FM_DJ_TRANS_CUEING;
    if (t == FM_DJ_TRANS_DROP && fm_dj_trans_filter_slam_supported()) {
        uint8_t s = (uint8_t)(fm_dj_rng_u8(dj) % 100u);

        if (s < 14u)
            t = FM_DJ_TRANS_FILTER_SLAM;
    }
    if (t == dj->last_trans_type) {
        t = (uint8_t)((t + 1u) % FM_DJ_TRANS_N);
        if (t == FM_DJ_TRANS_FILTER_SLAM && !fm_dj_trans_filter_slam_supported())
            t = FM_DJ_TRANS_DROP;
        if (t == dj->last_trans_type)
            t = FM_DJ_TRANS_CROSSFADE;
    }
    dj->last_trans_type = t;
    return t;
}

static inline uint8_t fm_dj_pick_drop_build_arc(fm_dj_t *dj, uint8_t r)
{
    (void)dj;
#if defined(GENRE_JAZZ)
    if (r < 4u)
        return FM_DJ_BUILD_THIN;
    if (r < 8u)
        return FM_DJ_BUILD_WASH;
    if (r < 10u && fm_dj_build_motion_rise_supported())
        return FM_DJ_BUILD_MOTION_RISE;
    return FM_DJ_BUILD_FB_PONG;
#else
    if (r < 3u)
        return FM_DJ_BUILD_THIN;
    if (r < 5u && fm_dj_build_motion_rise_supported())
        return FM_DJ_BUILD_MOTION_RISE;
    if (r < 8u)
        return FM_DJ_BUILD_WASH;
    if (r < 10u)
        return FM_DJ_BUILD_FB_PONG;
    if (fm_dj_build_dub_echo_supported() && r == 10u)
        return FM_DJ_BUILD_DUB_ECHO;
    if (fm_dj_build_track_lift_supported() && r == 11u)
        return FM_DJ_BUILD_TRACK_LIFT;
    if (fm_dj_build_iso_tank_supported())
        return FM_DJ_BUILD_ISO_TANK;
    return FM_DJ_BUILD_FB_PONG;
#endif
}

static inline void fm_dj_pick_phrase_plan(fm_dj_t *dj)
{
    uint8_t r;

    fm_dj_layout_phrase(dj);
    dj->ride_hog = 0u;
    dj->trans_type = fm_dj_pick_trans_type(dj);
    if (!fm_dj_trans_is_drop_path(dj))
        fm_dj_drop_sil_reset(dj);
    if (dj->trans_type == FM_DJ_TRANS_CUEING)
        dj->build_arc = FM_DJ_BUILD_THIN;
    r = (uint8_t)(fm_dj_rng_u8(dj) % 12u);
    if (dj->trans_type == FM_DJ_TRANS_CROSSFADE) {
        if (r >= 8u)
            dj->build_arc = FM_DJ_BUILD_WASH;
        else if (r >= 4u)
            dj->build_arc = FM_DJ_BUILD_THIN;
        else
            dj->build_arc = FM_DJ_BUILD_FB_PONG;
    } else if (dj->trans_type == FM_DJ_TRANS_ECHO_OUT) {
        if (r >= 6u)
            dj->build_arc = FM_DJ_BUILD_WASH;
        else
            dj->build_arc = FM_DJ_BUILD_FB_PONG;
    } else if (dj->trans_type == FM_DJ_TRANS_FILTER_SLAM) {
        dj->build_arc = (r < 6u) ? FM_DJ_BUILD_MOTION_RISE : FM_DJ_BUILD_THIN;
        fm_dj_drop_sil_plan_pick(dj);
    } else if (fm_dj_trans_is_drop_path(dj)) {
        dj->build_arc = fm_dj_pick_drop_build_arc(dj, r);
        fm_dj_drop_sil_plan_pick(dj);
    }
}

/* REVERB UP: audible wash climb only in the short pre-break window. */
static inline uint8_t fm_dj_is_wash_climb_bar(const fm_dj_t *dj)
{
    uint8_t pb;

    if (!dj->on || dj->build_arc != FM_DJ_BUILD_WASH)
        return 0u;
    pb = dj->phrase_bar;
    return (uint8_t)(pb >= dj->build_start && pb <= dj->build_end);
}

/* ISO+TANK: same pre-break window (break bar tail via fm_dj_climax.h). */
static inline uint8_t fm_dj_is_iso_tank_climb_bar(const fm_dj_t *dj)
{
    uint8_t pb;

    if (!dj->on || dj->build_arc != FM_DJ_BUILD_ISO_TANK)
        return 0u;
    pb = dj->phrase_bar;
    return (uint8_t)(pb >= dj->build_start && pb <= dj->build_end);
}

/* BREAK bar audio (phrase index after break arm): pre-drop silence window only. */
static inline uint8_t fm_dj_is_break_bar(const fm_dj_t *dj)
{
    return (uint8_t)(dj->on && dj->phrase_bar == dj->drop_arm);
}

/* BREAK bar (phrase 15): half- or full-bar dry gate before DROP on next downbeat. */
static inline uint8_t fm_dj_pre_drop_silence(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                             uint8_t steps_per_bar)
{
    uint8_t half;

    if (!playing || !dj->on || steps_per_bar < 2u)
        return 0u;
    if (!fm_dj_trans_is_drop_path(dj))
        return 0u;
    if (!fm_dj_is_break_bar(dj))
        return 0u;
    if (dj->drop_sil_full)
        return 1u;
    half = steps_per_bar >> 1;
    return (uint8_t)(bar_step >= half);
}

static inline uint8_t fm_dj_role_gain(uint8_t role)
{
    switch (role) {
    case FM_DJ_ROLE_RIDE:
        return 110u;
    case FM_DJ_ROLE_THROW:
        return 127u;
    case FM_DJ_ROLE_FILTER:
        return 120u;
    case FM_DJ_ROLE_BREAK:
        return 100u;
    case FM_DJ_ROLE_DROP:
        return 127u;
    default:
        return 0u;
    }
}

static const uint8_t FM_DJ_RIDE_TGT[4] = {
    FM_DJ_TGT_MOTION, FM_DJ_TGT_FLUX, FM_DJ_TGT_DENS_H, FM_DJ_TGT_SEQ_ROT};
static const uint8_t FM_DJ_THROW_TGT[4] = {
    FM_DJ_TGT_SPACE, FM_DJ_TGT_DUB, FM_DJ_TGT_FB, FM_DJ_TGT_ISO};
static const uint8_t FM_DJ_FILTER_TGT[3] = {
    FM_DJ_TGT_CUTOFF, FM_DJ_TGT_WOB, FM_DJ_TGT_WEIGHT};

static inline uint8_t fm_dj_pick_target(fm_dj_t *dj, uint8_t role)
{
    const uint8_t *list;
    uint8_t n;
    uint8_t i;
    uint8_t pick;

    switch (role) {
    case FM_DJ_ROLE_RIDE:
        list = FM_DJ_RIDE_TGT;
        n = 4u;
        break;
    case FM_DJ_ROLE_THROW:
        list = FM_DJ_THROW_TGT;
        n = 4u;
        break;
    case FM_DJ_ROLE_FILTER:
        list = FM_DJ_FILTER_TGT;
        n = 3u;
        break;
    case FM_DJ_ROLE_BREAK:
        return FM_DJ_TGT_DENS_K;
    default:
        return FM_DJ_TGT_N;
    }
    pick = (uint8_t)(fm_dj_rng_u8(dj) % n);
    for (i = 0; i < n; i++) {
        uint8_t t = list[(pick + i) % n];
        if (role != dj->last_role || t != dj->target)
            return t;
    }
    return list[pick];
}

static inline uint8_t fm_dj_pick_role(fm_dj_t *dj, uint8_t phrase_bar)
{
    uint8_t r;
    uint8_t tries;
    uint8_t pb = fm_dj_phrase_intro_map(dj, phrase_bar);

    if (pb <= 5u) {
        r = (uint8_t)(fm_dj_rng_u8(dj) % 3u);
        if (r == 2u)
            r = FM_DJ_ROLE_RIDE;
        else if (r == 1u)
            r = FM_DJ_ROLE_FILTER;
        else
            r = FM_DJ_ROLE_RIDE;
    } else if (pb <= 11u) {
        r = (uint8_t)(fm_dj_rng_u8(dj) % 4u);
        if (r == 3u)
            r = FM_DJ_ROLE_THROW;
        else if (r == 2u)
            r = FM_DJ_ROLE_FILTER;
        else if (r == 1u)
            r = FM_DJ_ROLE_RIDE;
        else
            r = FM_DJ_ROLE_THROW;
    } else {
        r = (uint8_t)(fm_dj_rng_u8(dj) % 3u);
        r = (r == 0u) ? FM_DJ_ROLE_THROW : (r == 1u ? FM_DJ_ROLE_FILTER : FM_DJ_ROLE_RIDE);
    }
    for (tries = 0; tries < 4u; tries++) {
        if (r != dj->last_role)
            break;
        r = (uint8_t)((r + 1u) % 3u);
    }
    return r;
}

static inline uint8_t fm_dj_pick_clock_div(fm_dj_t *dj)
{
    static const uint8_t divs[5] = {1u, 2u, 4u, 8u, 16u};
    return divs[fm_dj_rng_u8(dj) % 5u];
}

static inline void fm_dj_slot_clear(fm_dj_slot_t *s)
{
    s->phase = FM_DJ_SLOT_OFF;
    s->target = FM_DJ_TGT_N;
    s->role = FM_DJ_ROLE_N;
    s->clock_div = 4u;
    s->release_div = 8u;
    s->hold_bars = 0u;
    s->peak = 0;
    s->delta = 0;
    s->lane = FM_DJ_SLOT_LANE_GLOBAL;
}

static inline void fm_dj_refresh_lead(fm_dj_t *dj)
{
    uint8_t i;
    fm_dj_slot_t *s;

    if (dj->lead_slot < FM_DJ_SLOT_N) {
        s = &dj->slot[dj->lead_slot];
        if (s->phase != FM_DJ_SLOT_OFF) {
            dj->role = s->role;
            dj->target = s->target;
            dj->hold = s->peak;
            dj->delta = s->delta;
            return;
        }
    }
    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        s = &dj->slot[i];
        if (s->phase != FM_DJ_SLOT_OFF) {
            dj->lead_slot = i;
            dj->role = s->role;
            dj->target = s->target;
            dj->hold = s->peak;
            dj->delta = s->delta;
            return;
        }
    }
    dj->role = FM_DJ_ROLE_N;
    dj->target = FM_DJ_TGT_N;
    dj->hold = 0;
    dj->delta = 0;
}

static inline void fm_dj_clear_delta(fm_dj_t *dj)
{
    uint8_t i;

    for (i = 0; i < FM_DJ_SLOT_N; i++)
        fm_dj_slot_clear(&dj->slot[i]);
    dj->lead_slot = 0u;
    dj->hold = 0;
    dj->delta = 0;
    dj->target = FM_DJ_TGT_N;
    dj->role = FM_DJ_ROLE_N;
}

static inline int8_t fm_dj_pick_hold(fm_dj_t *dj, uint8_t role)
{
    int32_t amp;
    uint8_t gain = fm_dj_role_gain(role);

    amp = (int32_t)dj->energy * (int32_t)gain / 100;
    if (amp < 1)
        amp = 1;
    if (amp > 127)
        amp = 127;
    if (role == FM_DJ_ROLE_THROW)
        return (int8_t)amp;
    if (role == FM_DJ_ROLE_BREAK)
        return (int8_t)(-amp);
    if ((fm_dj_rng_u8(dj) % 5u) == 0u)
        return (int8_t)(-amp);
    return (int8_t)amp;
}

static inline uint8_t fm_dj_release_div(uint8_t attack_div)
{
    uint8_t d = attack_div ? attack_div : 4u;

    d = (uint8_t)(d * 2u);
    if (d > 16u)
        d = 16u;
    return d;
}

/* Spawn pacing (bars): lower intro max → denser early gestures; raise skip denominators → fewer random skips.
 * GAP_BIAS is added to every scheduled gap (0 = legacy). GAP_CAP clamps the result. */
#ifndef FM_DJ_FX_RIDE_RARE
#define FM_DJ_FX_RIDE_RARE 19u
#endif

#ifndef FM_DJ_SPAWN_INTRO_CD_MAX
#define FM_DJ_SPAWN_INTRO_CD_MAX 4u /* first phrase quarter: gap upper bound after roll */
#endif
#ifndef FM_DJ_SPAWN_SKIP_NUM
#define FM_DJ_SPAWN_SKIP_NUM 11u /* ~1/N bars skipped when cd==0 (early phrase) */
#endif
#ifndef FM_DJ_SPAWN_GAP_BIAS
#define FM_DJ_SPAWN_GAP_BIAS 0u
#endif
#ifndef FM_DJ_SPAWN_GAP_CAP
#define FM_DJ_SPAWN_GAP_CAP 8u
#endif

/* dj->phase: low byte = bars until next spawn window; bit 8 = double-hit next bar. */
static inline uint8_t fm_dj_spawn_cd(const fm_dj_t *dj)
{
    return (uint8_t)(dj->phase & 0xffu);
}

static inline uint8_t fm_dj_spawn_burst(const fm_dj_t *dj)
{
    return (uint8_t)((dj->phase >> 8) & 1u);
}

static inline void fm_dj_spawn_set_cd(fm_dj_t *dj, uint8_t cd)
{
    dj->phase = (dj->phase & 0xffffff00u) | (uint32_t)cd;
}

static inline void fm_dj_spawn_set_burst(fm_dj_t *dj, uint8_t on)
{
    if (on)
        dj->phase |= 0x100u;
    else
        dj->phase &= ~0x100u;
}

static inline uint8_t fm_dj_spawn_take_burst_env(fm_dj_t *dj)
{
    if (dj->phase & 0x200u) {
        dj->phase &= ~0x200u;
        return 1u;
    }
    return 0u;
}

static inline uint8_t fm_dj_slot_load_count(const fm_dj_t *dj)
{
    uint8_t i;
    uint8_t n = 0u;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        if (dj->slot[i].phase != FM_DJ_SLOT_OFF)
            n++;
    }
    return n;
}

static inline uint8_t fm_dj_schedule_spawn_gap(fm_dj_t *dj, uint8_t phrase_bar)
{
    uint8_t r = (uint8_t)fm_dj_rng_u8(dj);
    uint8_t load = fm_dj_slot_load_count(dj);
    uint8_t cd;
    uint8_t burst = 0u;
    uint8_t pb = fm_dj_phrase_intro_map(dj, phrase_bar);

    if (pb <= 3u) {
        cd = (uint8_t)(1u + (r % FM_DJ_SPAWN_INTRO_CD_MAX)); /* 1..FM_DJ_SPAWN_INTRO_CD_MAX */
        if ((r & 0x30u) == 0u)
            cd += 1u;
    } else if (pb <= 7u) {
        cd = (uint8_t)(1u + (r % 5u)); /* 1..5 */
    } else if (pb <= 11u) {
        switch (r % 13u) {
        case 0u:
            burst = 1u;
            cd = (uint8_t)(1u + (r % 2u));
            break;
        case 1u:
        case 2u:
            cd = 0u;
            break;
        case 3u:
        case 4u:
        case 5u:
            cd = 1u;
            break;
        case 6u:
        case 7u:
            cd = 2u;
            break;
        default:
            cd = (uint8_t)(1u + (r % 4u)); /* 1..4 */
            break;
        }
        if ((pb & 3u) == 3u && (r & 3u) != 0u)
            cd++;
    } else if (fm_dj_phrase_is_build(dj, phrase_bar)) {
        cd = (uint8_t)((r % 4u) ? (1u + (r % 3u)) : 2u);
    } else {
        cd = (uint8_t)(1u + (r % 3u));
    }

    if (load >= 3u)
        cd += (uint8_t)(1u + (r & 1u));
    else if (load >= 2u && (r % 6u) == 0u)
        cd += 2u;

    cd = (uint8_t)(cd + FM_DJ_SPAWN_GAP_BIAS);
    if (cd > FM_DJ_SPAWN_GAP_CAP)
        cd = FM_DJ_SPAWN_GAP_CAP;
    fm_dj_spawn_set_cd(dj, cd);
    fm_dj_spawn_set_burst(dj, burst);
    return cd;
}

/* 1 = spawn this bar, 0 = wait (countdown advanced). */
static inline uint8_t fm_dj_spawn_gate(fm_dj_t *dj, uint8_t phrase_bar)
{
    uint8_t r;
    uint8_t pb = fm_dj_phrase_intro_map(dj, phrase_bar);

    if (fm_dj_spawn_burst(dj)) {
        fm_dj_spawn_set_burst(dj, 0u);
        dj->phase |= 0x200u; /* stacked hit → short burst envelope */
        return 1u;
    }
    if (fm_dj_spawn_cd(dj) > 0u) {
        fm_dj_spawn_set_cd(dj, (uint8_t)(fm_dj_spawn_cd(dj) - 1u));
        if (fm_dj_spawn_cd(dj) > 0u)
            return 0u;
        /* countdown just reached 0 — fall through and try spawn this bar */
    }

    r = (uint8_t)fm_dj_rng_u8(dj);
    if (pb <= 5u) {
        if ((pb & 3u) == 2u && (r % 5u) == 0u)
            return 0u;
        if ((r % FM_DJ_SPAWN_SKIP_NUM) == 0u)
            return 0u;
    } else if (pb <= 11u) {
        if ((pb & 1u) == 0u && (r % 13u) == 0u)
            return 0u;
    }
    return 1u;
}

/* 0 = long sweep / nudge, 1 = short burst. */
static inline uint8_t fm_dj_pick_shape(fm_dj_t *dj, uint8_t phrase_bar, uint8_t force_burst)
{
    uint8_t r = (uint8_t)fm_dj_rng_u8(dj);
    uint8_t pb = fm_dj_phrase_intro_map(dj, phrase_bar);

    if (force_burst)
        return 1u;
    if (fm_dj_phrase_is_build(dj, phrase_bar))
        return (r < 179u) ? 1u : 0u; /* ~70% burst OK pre-break */
    if (pb <= 5u)
        return (r < 191u) ? 0u : 1u; /* ~25% burst — audible LFO sooner */
    if (pb <= 11u)
        return (r < 204u) ? 0u : 1u; /* ~20% burst */
    return (r < 200u) ? 0u : 1u;
}

static inline int8_t fm_dj_scale_peak(int8_t peak, uint8_t pct)
{
    int32_t v = (int32_t)peak * (int32_t)pct / 100;

    if (v > 127)
        v = 127;
    if (v < -127)
        v = -127;
    if (v == 0 && peak != 0)
        v = (peak > 0) ? 1 : -1;
    return (int8_t)v;
}

static inline void fm_dj_plan_envelope(fm_dj_t *dj, uint8_t phrase_bar, uint8_t role, uint8_t shape,
                                       uint8_t *clock_div, uint8_t *release_div, uint8_t *hold_bars,
                                       uint8_t *peak_pct)
{
    uint8_t r = (uint8_t)fm_dj_rng_u8(dj);
    uint8_t pb = fm_dj_phrase_intro_map(dj, phrase_bar);
    static const uint8_t sweep_atk[5] = {4u, 6u, 8u, 12u, 4u};

    if (shape == 0u) {
        *clock_div = sweep_atk[r % 5u];
        *release_div = 16u;
        if (fm_dj_phrase_is_build(dj, phrase_bar))
            *hold_bars = (uint8_t)(2u + (r % 3u)); /* 2..4 */
        else if (pb <= 5u)
            *hold_bars = (uint8_t)(4u + (r % 4u)); /* 4..7 */
        else if (pb <= 11u)
            *hold_bars = (uint8_t)(3u + (r % 5u)); /* 3..7 */
        else
            *hold_bars = (uint8_t)(2u + (r % 3u)); /* 2..4 */
        *peak_pct = (uint8_t)(55u + (r % 35u));    /* 55..89 — audible sweeps */
    } else {
        *clock_div = (uint8_t)((r % 3u) == 0u ? 1u : 2u);
        *release_div = (uint8_t)(1u + (r % 3u)); /* 1..3 */
        *hold_bars = (uint8_t)((r % 5u) == 0u ? 0u : 1u);
        *peak_pct = (uint8_t)(95u + (r % 33u));  /* 95..127 */
    }
    if (role == FM_DJ_ROLE_THROW) {
        if (shape == 0u) {
            *clock_div = 4u;
            *hold_bars = (uint8_t)(2u + (r % 2u));
            *peak_pct = (uint8_t)(55u + (r % 30u));
        } else {
            *clock_div = 1u;
            *release_div = 2u;
            *hold_bars = (uint8_t)((r % 3u) == 0u ? 0u : 1u);
        }
    }
    if (role == FM_DJ_ROLE_FILTER && shape == 0u)
        *clock_div = (uint8_t)(*clock_div < 8u ? 8u : *clock_div);
}

static inline uint8_t fm_dj_pick_slot(fm_dj_t *dj)
{
    uint8_t i;
    uint8_t pick = 0xffu;
    uint8_t rel = 0xffu;
    int32_t weakest = 127;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        if (dj->slot[i].phase == FM_DJ_SLOT_OFF)
            return i;
        if (dj->slot[i].phase == FM_DJ_SLOT_RELEASE && rel == 0xffu)
            rel = i;
        {
            int32_t a = (int32_t)dj->slot[i].delta;
            if (a < 0)
                a = -a;
            if (a < weakest) {
                weakest = a;
                pick = i;
            }
        }
    }
    if (rel != 0xffu)
        return rel;
    if (pick != 0xffu)
        return pick;
    return 0u;
}

static inline uint8_t fm_dj_tgt_is_seq_param(uint8_t tgt)
{
    return (uint8_t)(tgt >= FM_DJ_TGT_SEQ_VEL && tgt <= FM_DJ_TGT_SEQ_PITCH);
}

static inline void fm_dj_slot_begin_lane(fm_dj_t *dj, uint8_t si, uint8_t role, uint8_t tgt, int8_t peak,
                                       uint8_t hold_bars, uint8_t clock_div, uint8_t release_div, uint8_t lane)
{
    fm_dj_slot_t *s = &dj->slot[si];

    s->phase = FM_DJ_SLOT_ATTACK;
    s->role = role;
    s->target = tgt;
    s->lane = fm_dj_tgt_is_seq_param(tgt) ? (uint8_t)(lane % 6u) : FM_DJ_SLOT_LANE_GLOBAL;
    s->peak = peak;
    s->hold_bars = hold_bars;
    s->clock_div = clock_div ? clock_div : 4u;
    s->release_div = release_div ? release_div : fm_dj_release_div(s->clock_div);
    if (role == FM_DJ_ROLE_BREAK || role == FM_DJ_ROLE_DROP) {
        s->delta = peak;
        s->phase = (hold_bars > 0u) ? FM_DJ_SLOT_HOLD : FM_DJ_SLOT_RELEASE;
    }
    dj->lead_slot = si;
    dj->last_role = role;
    dj->role = role;
    dj->target = tgt;
    dj->hold = peak;
    dj->delta = s->delta;
}

static inline void fm_dj_slot_begin(fm_dj_t *dj, uint8_t si, uint8_t role, uint8_t tgt, int8_t peak,
                                    uint8_t hold_bars, uint8_t clock_div, uint8_t release_div)
{
    fm_dj_slot_begin_lane(dj, si, role, tgt, peak, hold_bars, clock_div, release_div, FM_DJ_SLOT_LANE_GLOBAL);
}

static inline void fm_dj_slots_age_bars(fm_dj_t *dj)
{
    uint8_t i;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        fm_dj_slot_t *s = &dj->slot[i];

        if (s->phase == FM_DJ_SLOT_HOLD && s->hold_bars > 0u) {
            s->hold_bars--;
            if (s->hold_bars == 0u)
                s->phase = FM_DJ_SLOT_RELEASE;
        }
    }
}

static inline void fm_dj_slots_phrase_flush(fm_dj_t *dj)
{
    uint8_t i;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        fm_dj_slot_t *s = &dj->slot[i];

        if (s->phase != FM_DJ_SLOT_OFF)
            s->phase = FM_DJ_SLOT_RELEASE;
    }
}

static inline uint8_t fm_dj_seq_fx_try(fm_dj_t *dj, uint8_t phrase_bar, uint8_t shape, uint8_t *role,
                                       uint8_t *tgt);

#ifndef FM_DJ_SEQ_FX_SUPPORTED
#define FM_DJ_SEQ_FX_SUPPORTED 1
#endif
#ifndef FM_DJ_SEQ_PARAM_RARE_NUM
#define FM_DJ_SEQ_PARAM_RARE_NUM 31u
#endif

/* SEQ page-1 horsemen on one lane (burst shapes only; sparse). */
static inline uint8_t fm_dj_seq_param_try(fm_dj_t *dj, uint8_t phrase_bar, uint8_t shape, uint8_t *role,
                                          uint8_t *tgt, uint8_t *lane)
{
    uint8_t r;
    uint8_t pick;

    if (!FM_DJ_SEQ_FX_SUPPORTED)
        return 0u;
    if (phrase_bar >= dj->break_arm)
        return 0u;
    if (shape == 0u)
        return 0u;
    if (phrase_bar >= dj->build_end)
        return 0u;
    r = (uint8_t)fm_dj_rng_u8(dj);
    if ((r % FM_DJ_SEQ_PARAM_RARE_NUM) != 0u)
        return 0u;
    *lane = (uint8_t)(fm_dj_rng_u8(dj) % 6u);
    pick = (uint8_t)(fm_dj_rng_u8(dj) % 4u);
    switch (pick) {
    case 0u:
        *tgt = FM_DJ_TGT_SEQ_VEL;
        break;
    case 1u:
        *tgt = FM_DJ_TGT_SEQ_TRIG;
        break;
    case 2u:
        *tgt = FM_DJ_TGT_SEQ_GATE;
        break;
    default:
        *tgt = FM_DJ_TGT_SEQ_PITCH;
        break;
    }
    if (*role == FM_DJ_ROLE_BREAK || *role == FM_DJ_ROLE_DROP)
        *role = FM_DJ_ROLE_RIDE;
    return 1u;
}

/* Phrase-end hand-sequence (does not add spawn gates). */
static inline void fm_dj_trans_prelude_bars(fm_dj_t *dj, uint8_t pb)
{
    uint8_t si;

    if (pb >= dj->break_arm)
        return;
    if (dj->trans_type == FM_DJ_TRANS_ECHO_OUT && dj->break_arm >= 2u) {
        /* Both bars are before break_arm; the function returns once pb reaches it. */
        if (pb == (uint8_t)(dj->break_arm - 2u)) {
            si = fm_dj_pick_slot(dj);
            fm_dj_slot_begin(dj, si, FM_DJ_ROLE_THROW, FM_DJ_TGT_SPACE, 72, 1u, 2u, 6u);
        } else if (pb == (uint8_t)(dj->break_arm - 1u)) {
            si = fm_dj_pick_slot(dj);
            fm_dj_slot_begin(dj, si, FM_DJ_ROLE_THROW, FM_DJ_TGT_DUB, 88, 1u, 1u, 4u);
        }
        return;
    }
    if (dj->trans_type == FM_DJ_TRANS_FILTER_SLAM) {
        if (pb == dj->build_end) {
            si = fm_dj_pick_slot(dj);
            fm_dj_slot_begin(dj, si, FM_DJ_ROLE_FILTER, FM_DJ_TGT_WOB, 64, 1u, 4u, 8u);
        } else if (pb == (uint8_t)(dj->break_arm - 1u)) {
            si = fm_dj_pick_slot(dj);
            fm_dj_slot_begin(dj, si, FM_DJ_ROLE_FILTER, FM_DJ_TGT_CUTOFF, 96, 1u, 2u, 6u);
        }
    }
}

static inline void fm_dj_spawn_gesture(fm_dj_t *dj, uint8_t phrase_bar)
{
    uint8_t role;
    uint8_t tgt;
    uint8_t atk_div;
    uint8_t rel_div;
    uint8_t hb;
    uint8_t si;
    uint8_t shape;
    uint8_t peak_pct;
    int8_t peak;
    uint8_t lane = FM_DJ_SLOT_LANE_GLOBAL;

    if (phrase_bar >= dj->break_arm)
        return;
    if (!fm_dj_spawn_gate(dj, phrase_bar)) {
        if (fm_dj_spawn_cd(dj) == 0u)
            fm_dj_schedule_spawn_gap(dj, phrase_bar);
        return;
    }

    role = fm_dj_pick_role(dj, phrase_bar);
    shape = fm_dj_pick_shape(dj, phrase_bar, fm_dj_spawn_take_burst_env(dj));
    fm_dj_plan_envelope(dj, phrase_bar, role, shape, &atk_div, &rel_div, &hb, &peak_pct);
    if (fm_dj_seq_fx_try(dj, phrase_bar, shape, &role, &tgt)) {
        int8_t hold = fm_dj_pick_hold(dj, role);

        if (tgt == FM_DJ_TGT_FLIP)
            hold = hold > 0 ? hold : (int8_t)-hold;
        peak = fm_dj_scale_peak(hold, peak_pct);
    } else if (fm_dj_seq_param_try(dj, phrase_bar, shape, &role, &tgt, &lane)) {
        peak = fm_dj_scale_peak(fm_dj_pick_hold(dj, role), peak_pct);
    } else {
        tgt = fm_dj_pick_target(dj, role);
        peak = fm_dj_scale_peak(fm_dj_pick_hold(dj, role), peak_pct);
    }
    if (tgt == FM_DJ_TGT_ENSEMBLE && hb < 5u)
        hb = 5u; /* combo A(2)+B(1)+TUTTI resolve(1) */
    if (dj->ride_hog == 2u && role == FM_DJ_ROLE_RIDE) {
        tgt = (uint8_t)((fm_dj_rng_u8(dj) & 1u) ? FM_DJ_TGT_FB : FM_DJ_TGT_SPACE);
        if (hb < 6u)
            hb = (uint8_t)(6u + (fm_dj_rng_u8(dj) % 3u));
        rel_div = 12u;
    }
    si = fm_dj_pick_slot(dj);
    if (fm_dj_tgt_is_seq_param(tgt))
        fm_dj_slot_begin_lane(dj, si, role, tgt, peak, hb, atk_div, rel_div, lane);
    else
        fm_dj_slot_begin(dj, si, role, tgt, peak, hb, atk_div, rel_div);
    fm_dj_schedule_spawn_gap(dj, phrase_bar);
}

static inline void fm_dj_arm_bar(fm_dj_t *dj, uint8_t fill_blocked)
{
    uint8_t pb;

    dj->fill_blocked = fill_blocked;
    if (!dj->on)
        return;

    pb = dj->phrase_bar;
    dj->energy = fm_dj_energy_for_bar(dj, pb);

    if (fill_blocked) {
        fm_dj_clear_delta(dj);
        dj->phrase_bar = fm_dj_phrase_wrap(dj, pb);
        if (dj->phrase_bar == 0u) {
            fm_dj_pick_phrase_plan(dj);
            dj->drop_armed = 1u;
        }
        return;
    }

    fm_dj_slots_age_bars(dj);

    if (pb == dj->break_arm)
        fm_dj_slots_phrase_flush(dj);

    dj->prev_role = dj->role;

    fm_dj_trans_prelude_bars(dj, pb);
    if (pb < dj->break_arm)
        fm_dj_spawn_gesture(dj, pb);
    else if (pb == dj->break_arm && fm_dj_trans_is_drop_path(dj)) {
        uint8_t si = fm_dj_pick_slot(dj);
        int8_t br_peak = fm_dj_build_tail_hold(dj) ? 0 : -127;

        fm_dj_slot_begin(dj, si, FM_DJ_ROLE_BREAK, FM_DJ_TGT_DENS_K, br_peak, 2u, 1u, 6u);
        dj->last_role = FM_DJ_ROLE_BREAK;
    } else if (pb == dj->drop_arm) {
        uint8_t si = fm_dj_pick_slot(dj);

        if (fm_dj_trans_is_drop_path(dj)) {
            fm_dj_slot_begin(dj, si, FM_DJ_ROLE_DROP, FM_DJ_TGT_DENS_K, 127, 2u, 1u, 4u);
            dj->last_role = FM_DJ_ROLE_DROP;
        } else {
            fm_dj_slot_begin(dj, si, FM_DJ_ROLE_RIDE, FM_DJ_TGT_SPACE, 48, 1u, 4u, 8u);
            dj->last_role = FM_DJ_ROLE_RIDE;
        }
        if (dj->drop_armed)
            dj->trans_pending = (uint8_t)(dj->trans_type + 1u);
    }

    fm_dj_refresh_lead(dj);
    dj->phrase_bar = fm_dj_phrase_wrap(dj, pb);
    if (dj->phrase_bar == 0u) {
        fm_dj_pick_phrase_plan(dj);
        dj->drop_armed = 1u;
    }
}

static inline void fm_dj_enable(fm_dj_t *dj, uint32_t seed)
{
    uint8_t i;

    dj->on = 1u;
    dj->slew_off = 0u;
    dj->fill_blocked = 0u;
    fm_dj_drop_sil_reset(dj);
    dj->ride_hog = 0u;
    dj->rng = seed ? seed : 1u;
    dj->phrase_bar = 0u;
    dj->last_drop_span = 0u;
    dj->drop_armed = 1u;
    dj->bar_pending = 0u;
    dj->trans_pending = 0u;
    dj->last_trans_type = FM_DJ_TRANS_N;
    dj->last_role = FM_DJ_ROLE_N;
    dj->prev_role = FM_DJ_ROLE_N;
    dj->lead_slot = 0u;
    dj->phase = 0u; /* cd==0 → spawn on first bar arm after enable */
    for (i = 0; i < FM_DJ_SLOT_N; i++)
        fm_dj_slot_clear(&dj->slot[i]);
    fm_dj_clear_delta(dj);
    fm_dj_pick_phrase_plan(dj);
}

static inline void fm_dj_disable(fm_dj_t *dj)
{
    dj->on = 0u;
    dj->slew_off = 1u;
    dj->bar_pending = 0u;
    dj->trans_pending = 0u;
    dj->last_trans_type = FM_DJ_TRANS_N;
    fm_dj_drop_sil_reset(dj);
}

static inline void fm_dj_toggle(fm_dj_t *dj, uint32_t seed)
{
    if (dj->on)
        fm_dj_disable(dj);
    else
        fm_dj_enable(dj, seed);
}

static inline int32_t fm_dj_slew_toward(int8_t cur, int8_t goal, uint8_t div, uint8_t attack)
{
    int32_t step;
    int32_t d = (int32_t)cur;
    int32_t g = (int32_t)goal;
    int32_t err = g - d;

    div = div ? div : 4u;
    step = (int32_t)(2u + (16u / div));
    if (attack) {
        /* Fast attack — larger steps when far from goal (mild curve). */
        int32_t ad = err < 0 ? -err : err;
        if (ad > 48)
            step += 4;
        else if (ad > 24)
            step += 2;
    } else {
        /* Slower, softer release. */
        step = (step * 5) / 8;
        if (step < 1)
            step = 1;
    }
    if (err > 0) {
        d += step;
        if (d > g)
            d = g;
    } else if (err < 0) {
        d -= step;
        if (d < g)
            d = g;
    }
    if (d > 127)
        d = 127;
    if (d < -127)
        d = -127;
    return d;
}

static inline uint8_t fm_dj_any_slot_live(const fm_dj_t *dj)
{
    uint8_t i;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        if (dj->slot[i].phase != FM_DJ_SLOT_OFF)
            return 1u;
        if (dj->slot[i].delta != 0)
            return 1u;
    }
    return 0u;
}

static inline void fm_dj_slot_tick(fm_dj_slot_t *s, uint8_t force_release)
{
    int32_t d;
    int8_t goal;

    if (s->phase == FM_DJ_SLOT_OFF) {
        if (s->delta != 0) {
            d = fm_dj_slew_toward(s->delta, 0, s->release_div, 0);
            s->delta = (int8_t)d;
        }
        return;
    }
    if (force_release)
        s->phase = FM_DJ_SLOT_RELEASE;

    switch (s->phase) {
    case FM_DJ_SLOT_ATTACK:
        goal = s->peak;
        d = fm_dj_slew_toward(s->delta, goal, s->clock_div, 1u);
        s->delta = (int8_t)d;
        if (s->delta == goal)
            s->phase = (s->hold_bars == 0u) ? FM_DJ_SLOT_RELEASE : FM_DJ_SLOT_HOLD;
        break;
    case FM_DJ_SLOT_HOLD:
        goal = s->peak;
        d = fm_dj_slew_toward(s->delta, goal, s->clock_div, 1u);
        s->delta = (int8_t)d;
        break;
    case FM_DJ_SLOT_RELEASE:
        d = fm_dj_slew_toward(s->delta, 0, s->release_div, 0);
        s->delta = (int8_t)d;
        if (s->delta == 0)
            fm_dj_slot_clear(s);
        break;
    default:
        break;
    }
}

static inline void fm_dj_tick(fm_dj_t *dj, uint8_t bar_step, uint8_t steps_per_bar)
{
    uint8_t i;
    uint8_t force_rel = 0u;

    (void)bar_step;
    (void)steps_per_bar;

    if (dj->slew_off)
        force_rel = 1u;

    if (!dj->on && !dj->slew_off)
        return;

    for (i = 0; i < FM_DJ_SLOT_N; i++)
        fm_dj_slot_tick(&dj->slot[i], force_rel);

    fm_dj_refresh_lead(dj);

    if (dj->slew_off && !fm_dj_any_slot_live(dj)) {
        dj->slew_off = 0u;
        dj->target = FM_DJ_TGT_N;
        dj->hold = 0;
        dj->delta = 0;
    }
}

static inline int32_t fm_dj_delta_for_tgt(const fm_dj_t *dj, uint8_t tgt)
{
    uint8_t i;
    int32_t sum = 0;

    if (!dj->on && !dj->slew_off)
        return 0;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        const fm_dj_slot_t *s = &dj->slot[i];

        if (s->target != tgt)
            continue;
        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0)
            continue;
        sum += (int32_t)s->delta;
    }
    if (sum > 127)
        return 127;
    if (sum < -127)
        return -127;
    return sum;
}

/* Per-lane SEQ param deltas (horsemen): only slots bound to ln or global lane. */
static inline int32_t fm_dj_delta_for_tgt_lane(const fm_dj_t *dj, uint8_t tgt, uint8_t ln)
{
    uint8_t i;
    int32_t sum = 0;

    if (!dj->on && !dj->slew_off)
        return 0;
    ln %= 6u;
    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        const fm_dj_slot_t *s = &dj->slot[i];

        if (s->target != tgt)
            continue;
        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0)
            continue;
        if (s->lane != FM_DJ_SLOT_LANE_GLOBAL && s->lane != ln)
            continue;
        sum += (int32_t)s->delta;
    }
    if (sum > 127)
        return 127;
    if (sum < -127)
        return -127;
    return sum;
}

#ifndef FM_DJ_SEQ_FX_SUPPORTED
#define FM_DJ_SEQ_FX_SUPPORTED 1
#endif

#ifndef FM_DJ_SEQ_FX_ENS_INTRO_NUM
#define FM_DJ_SEQ_FX_ENS_INTRO_NUM 5u
#endif
#ifndef FM_DJ_SEQ_FX_ENS_MID_NUM
#define FM_DJ_SEQ_FX_ENS_MID_NUM 4u
#endif
#ifndef FM_DJ_SEQ_FX_BUILD_TAIL_NUM
#define FM_DJ_SEQ_FX_BUILD_TAIL_NUM 3u
#endif
#ifndef FM_DJ_SEQ_FX_BUILD_NUM
#define FM_DJ_SEQ_FX_BUILD_NUM 4u
#endif
#ifndef FM_DJ_SEQ_FX_FLIP_NUM
#define FM_DJ_SEQ_FX_FLIP_NUM 4u
#endif

static inline uint8_t fm_dj_seq_fx_supported(void)
{
    return FM_DJ_SEQ_FX_SUPPORTED;
}

/* Long sweep → ENSEMBLE; short burst → FLIP (sparse, phrase-aware). */
static inline uint8_t fm_dj_seq_fx_try(fm_dj_t *dj, uint8_t phrase_bar, uint8_t shape, uint8_t *role,
                                       uint8_t *tgt)
{
    uint8_t r;
    int32_t sp;
    int32_t db;

    if (!fm_dj_seq_fx_supported())
        return 0u;
    if (phrase_bar >= dj->break_arm)
        return 0u;

    r = (uint8_t)fm_dj_rng_u8(dj);
    if (shape == 0u) {
        uint8_t imap = fm_dj_phrase_intro_map(dj, phrase_bar);

        if (fm_dj_phrase_is_build(dj, phrase_bar) && phrase_bar >= dj->build_end - 1u &&
            !fm_dj_build_tail_hold(dj) && (r % 3u) != 0u)
            return 0u;
        if (!fm_dj_phrase_is_build(dj, phrase_bar)) {
            if (imap <= 5u) {
                if ((r % FM_DJ_SEQ_FX_ENS_INTRO_NUM) != 0u)
                    return 0u;
            } else if ((r % FM_DJ_SEQ_FX_ENS_MID_NUM) != 0u)
                return 0u;
        } else {
            if (fm_dj_build_tail_hold(dj)) {
                /* Pass 1/N. A larger denom is rarer; the old `== 1` test got denser as N grew. */
                if ((r % FM_DJ_SEQ_FX_BUILD_TAIL_NUM) != 0u)
                    return 0u;
            } else if ((r % FM_DJ_SEQ_FX_BUILD_NUM) != 0u)
                return 0u;
        }
        sp = fm_dj_delta_for_tgt(dj, FM_DJ_TGT_SPACE);
        db = fm_dj_delta_for_tgt(dj, FM_DJ_TGT_DUB);
        if (sp > 52 && db > 40)
            return 0u;
        *tgt = FM_DJ_TGT_ENSEMBLE;
        if (*role == FM_DJ_ROLE_BREAK || *role == FM_DJ_ROLE_DROP)
            *role = FM_DJ_ROLE_RIDE;
        return 1u;
    }
    if (phrase_bar >= dj->build_end)
        return 0u;
    if ((r % FM_DJ_SEQ_FX_FLIP_NUM) != 0u)
        return 0u;
    *tgt = FM_DJ_TGT_FLIP;
    if (*role == FM_DJ_ROLE_BREAK || *role == FM_DJ_ROLE_DROP)
        *role = FM_DJ_ROLE_THROW;
    return 1u;
}

/* Live gesture layers (attack/hold/release), for toast stack hint. */
static inline uint8_t fm_dj_live_slot_count(const fm_dj_t *dj)
{
    uint8_t i;
    uint8_t n = 0u;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        if (dj->slot[i].phase != FM_DJ_SLOT_OFF)
            n++;
    }
    return n;
}

static inline uint8_t fm_dj_any_slot_role(const fm_dj_t *dj, uint8_t role)
{
    uint8_t i;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        const fm_dj_slot_t *s = &dj->slot[i];

        if (s->phase != FM_DJ_SLOT_OFF && s->role == role)
            return 1u;
    }
    return 0u;
}

/* Attack/hold only — release must not re-duck under DROP. */
static inline uint8_t fm_dj_slot_role_peaking(const fm_dj_t *dj, uint8_t role)
{
    uint8_t i;

    for (i = 0; i < FM_DJ_SLOT_N; i++) {
        const fm_dj_slot_t *s = &dj->slot[i];

        if (s->role == role &&
            (s->phase == FM_DJ_SLOT_ATTACK || s->phase == FM_DJ_SLOT_HOLD))
            return 1u;
    }
    return 0u;
}

/* THIN / FB+PONG: classic break-bar drum thin-out (not REVERB UP wash). */
static inline uint8_t fm_dj_break_seq_thin(const fm_dj_t *dj)
{
    if (!fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_BREAK))
        return 0u;
    if (fm_dj_build_tail_hold(dj))
        return 0u;
    return 1u;
}

static inline const char *fm_dj_build_toast_label(const fm_dj_t *dj)
{
    uint8_t pb;

    if (!dj->on)
        return 0;
    pb = dj->phrase_bar;
    if (pb >= dj->build_start && pb <= dj->build_end) {
        if (dj->build_arc == FM_DJ_BUILD_WASH)
            return "REVERB UP";
        if (dj->build_arc == FM_DJ_BUILD_ISO_TANK)
            return "ISO + TANK";
        if (dj->build_arc == FM_DJ_BUILD_FB_PONG)
            return (pb & 1u) ? "FEEDBACK" : "PONG BUILD";
        if (dj->build_arc == FM_DJ_BUILD_MOTION_RISE)
            return "MOTION + FLUX";
        if (dj->build_arc == FM_DJ_BUILD_DUB_ECHO)
            return "ECHO TIME";
        if (dj->build_arc == FM_DJ_BUILD_TRACK_LIFT)
            return "TRACK LIFT";
        return "BUILD THIN";
    }
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_BREAK)) {
        if (dj->build_arc == FM_DJ_BUILD_WASH)
            return "REVERB UP";
        if (dj->build_arc == FM_DJ_BUILD_ISO_TANK)
            return "ISO + TANK";
        if (dj->build_arc == FM_DJ_BUILD_FB_PONG)
            return "PONG BUILD";
        if (dj->build_arc == FM_DJ_BUILD_MOTION_RISE)
            return "MOTION + FLUX";
        if (dj->build_arc == FM_DJ_BUILD_DUB_ECHO)
            return "ECHO TIME";
        if (dj->build_arc == FM_DJ_BUILD_TRACK_LIFT)
            return "TRACK LIFT";
        return "BUILD THIN";
    }
    return 0;
}

static inline const char *fm_dj_trans_toast_label(const fm_dj_t *dj)
{
    if (!dj->on)
        return 0;
    switch (dj->trans_type) {
    case FM_DJ_TRANS_ECHO_OUT:
        return "ECHO OUT";
    case FM_DJ_TRANS_FILTER_SLAM:
        return "FILTER SLAM";
    default:
        return 0;
    }
}

#ifndef FM_DJ_TOAST_AUDIBLE
#define FM_DJ_TOAST_AUDIBLE 16
#endif

static inline uint8_t fm_dj_toast_delta_audible(int32_t d)
{
    int32_t ad = d < 0 ? -d : d;

    return ad > FM_DJ_TOAST_AUDIBLE ? 1u : 0u;
}

/* Toast param: target with largest |combined delta| above audibility floor only. */
static inline uint8_t fm_dj_toast_target(const fm_dj_t *dj)
{
    uint8_t t;
    uint8_t best_tgt = FM_DJ_TGT_N;
    int32_t best = FM_DJ_TOAST_AUDIBLE;

    for (t = 0u; t < FM_DJ_TGT_N; t++) {
        int32_t d = fm_dj_delta_for_tgt(dj, t);
        int32_t ad = d < 0 ? -d : d;

        if (ad > best) {
            best = ad;
            best_tgt = t;
        }
    }
    if (best_tgt < FM_DJ_TGT_N)
        return best_tgt;
    return FM_DJ_TGT_N;
}

#ifndef FM_DJ_TOAST_TGT_MARGIN
#define FM_DJ_TOAST_TGT_MARGIN 8
#endif
#ifndef FM_DJ_TOAST_TGT_LOSS
#define FM_DJ_TOAST_TGT_LOSS 10u
#endif

typedef struct {
    uint8_t tgt;
    uint16_t below_ticks;
} fm_dj_toast_tgt_hold_t;

static inline void fm_dj_toast_tgt_hold_reset(fm_dj_toast_tgt_hold_t *h)
{
    h->tgt = FM_DJ_TGT_N;
    h->below_ticks = 0u;
}

static inline int32_t fm_dj_toast_tgt_mag(const fm_dj_t *dj, uint8_t tgt)
{
    int32_t d;

    if (tgt >= FM_DJ_TGT_N)
        return 0;
    d = fm_dj_delta_for_tgt(dj, tgt);
    return d < 0 ? -d : d;
}

/* Hysteresis: hold target until challenger clearly wins or held fades out. */
static inline uint8_t fm_dj_toast_target_stable(const fm_dj_t *dj, fm_dj_toast_tgt_hold_t *h)
{
    uint8_t pick;
    int32_t pick_mag;
    int32_t held_mag;

    if (!dj->on) {
        fm_dj_toast_tgt_hold_reset(h);
        return FM_DJ_TGT_N;
    }
    pick = fm_dj_toast_target(dj);
    if (h->tgt >= FM_DJ_TGT_N) {
        if (pick < FM_DJ_TGT_N)
            h->tgt = pick;
        return h->tgt;
    }
    held_mag = fm_dj_toast_tgt_mag(dj, h->tgt);
    if (!fm_dj_toast_delta_audible(held_mag)) {
        if (h->below_ticks < FM_DJ_TOAST_TGT_LOSS)
            h->below_ticks++;
        else {
            h->tgt = pick;
            h->below_ticks = 0u;
        }
    } else
        h->below_ticks = 0u;
    if (pick < FM_DJ_TGT_N && pick != h->tgt) {
        pick_mag = fm_dj_toast_tgt_mag(dj, pick);
        if (pick_mag > held_mag + FM_DJ_TOAST_TGT_MARGIN) {
            h->tgt = pick;
            h->below_ticks = 0u;
        }
    }
    if (h->tgt < FM_DJ_TGT_N && !fm_dj_toast_delta_audible(fm_dj_toast_tgt_mag(dj, h->tgt)))
        return FM_DJ_TGT_N;
    return h->tgt;
}

static inline uint8_t fm_dj_mod_u8(uint8_t base, const fm_dj_t *dj, uint8_t tgt, int32_t lo, int32_t hi)
{
    int32_t v = (int32_t)base + fm_dj_delta_for_tgt(dj, tgt);
    return fm_dj_clamp_u8(v, lo, hi);
}

static inline uint8_t fm_dj_mod_macro(uint8_t base, const fm_dj_t *dj, uint8_t macro_idx)
{
    uint8_t tgt;

    switch (macro_idx) {
    case 8u:
        tgt = FM_DJ_TGT_MOTION;
        break;
    case 9u:
        tgt = FM_DJ_TGT_FLUX;
        break;
    case 2u:
        tgt = FM_DJ_TGT_WOB;
        break;
    case 3u:
        tgt = FM_DJ_TGT_ISO;
        break;
    case 4u:
        tgt = FM_DJ_TGT_SPACE;
        break;
    case 5u:
        tgt = FM_DJ_TGT_WEIGHT;
        break;
    case 7u:
        tgt = FM_DJ_TGT_DUB;
        break;
    default:
        return base;
    }
    return fm_dj_mod_u8(base, dj, tgt, 0, 127);
}

/* Host tests: advance phrase plan without running full bar sim. */
static inline uint8_t fm_dj_sim_next_phrase_plan(fm_dj_t *dj)
{
    fm_dj_pick_phrase_plan(dj);
    return dj->trans_type;
}

static inline uint8_t fm_dj_trans_pending_decode(const fm_dj_t *dj)
{
    if (dj->trans_pending == 0u)
        return 0xffu;
    return (uint8_t)(dj->trans_pending - 1u);
}

#endif /* FM_DJ_H */
