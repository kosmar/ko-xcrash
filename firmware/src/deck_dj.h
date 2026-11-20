/* SPDX-License-Identifier: GPL-3.0-only */
/* KO KIT DJ mode glue (pattern of fm_dj_app.h). Include from deck.c after the DECK_LFO enum, before deck_app_t.
 * Planner core is reused as-is. Dub-specific parts are replaced by a deck-side adapter:
 *   - targets:  FM_DJ_TGT_* -> deck_master_t / deck_perf_t fields (deck_dj_apply_targets), + DJ LFO overlay
 *   - LFO:      fm_dj_lfo.h overlay on DECK_LFO_* dests, DJ-only depth (additive on top of effective LFO)
 *   - gestures: SOLO_BASS / SOLO_DRUM -> solo mask, BREAK_MICRO -> kick duck, ARP -> arp rate overlay
 *   - seq:      ENSEMBLE / FLIP lane mutes, SEQ_VEL / SEQ_TRIG / SEQ_GATE / SEQ_PITCH -> deck_seq_djo
 *   - climax/FX: dub_params_t shadow, loaded from and stored back to the mapped deck fields only
 *   - archetypes: era-mapped (deck_dj_arc_for_era), see table below
 *   - transitions: plate/era steps (deck_dj_trans_act), executed by deck.c callback
 * FLAM: no grace note on deck; approximated as 2x Ratchet on BD/SN/CH hits (deck_seq_djo.flam, see deck_dj_seq_sync).
 * Dropped (no deck equivalent): GEST_LOOP, GEST_FILL (off on all genres), SEQ_ROT / SEQ_CYC / SEQ_REP.
 */
#ifndef DECK_DJ_H
#define DECK_DJ_H

#include <stdint.h>
#include "fm_dj_conf.h" /* no GENRE_* defined for KO KIT -> dub default caps */
#include "dub_types.h"  /* dub_params_t shadow: already included by deck_dsp.h */
#include "fm_dj_app.h"  /* fm_dj.h, FX longpress + phrase step helpers */
#include "fm_dj_climax.h"
#include "fm_dj_fx.h"
#include "fm_dj_gest.h"
#include "fm_dj_lfo.h"
#include "fm_dj_seq.h"

#define DECK_DJ_BAR_STEPS 16u /* FM_SEQ_PAGE_STEPS: one bar on the DECK sequencer */

/* Gesture caps: SOLO | FLAM | BREAK | ARP. LOOP has no deck equivalent; FILL is off on all genres. */
#define DECK_DJ_GEST_CAPS 39u
/* Voices for SOLO_BASS (kick, low toms, low conga) and SOLO_DRUM (snare, clap, hats). */
#define DECK_DJ_SOLO_BASS ((1u << DECK_BD) | (1u << DECK_TM) | (1u << DECK_TL) | (1u << DECK_CNL))
#define DECK_DJ_SOLO_DRUM ((1u << DECK_SN) | (1u << DECK_CP) | (1u << DECK_CH) | (1u << DECK_OH))
/* FLAM approximation voices (fm_app: dub voices 0/1/2 = kick, rim, hat). */
#define DECK_DJ_FLAM_VOICES ((1u << DECK_BD) | (1u << DECK_SN) | (1u << DECK_CH))

/* Voice -> DJ lane group (0..5), same role as the 6 dub lanes: 0 low, 1 snare/clap, 2 perc, 3 CH, 4 OH/cym, 5 unused. */
static const uint8_t DECK_DJ_VOICE_LANE[DECK_NVOICES] = {
    0u, 1u, 2u, 3u, 4u, 1u, 2u, 2u, 2u, 0u, 0u, 0u, 2u, 0u, 4u, 4u
};

/* No per-dest LFO state: used as "user slots" so fm_dj_lfo_eff_build yields DJ-only depth. */
static const fm_lfo_slot_t DECK_DJ_LFO_ZERO[DECK_LFO_TGT_N];

/*
 * Era-mapped build archetypes (KO KIT eras, bit = DECK_ERA_*: STAX=0 DEAD=1 GATE=2 CRATE=3 GRID=4 TRAP=5).
 * Allowed mask per FM_DJ_BUILD_*; a planned arc outside its mask falls back to DECK_DJ_ARC_FALLBACK[era].
 * TRACK_LIFT has no KO KIT equivalent (never allowed).
 */
enum {
    DECK_DJ_ERA_STAX = 0,
    DECK_DJ_ERA_DEAD,
    DECK_DJ_ERA_GATE,
    DECK_DJ_ERA_CRATE,
    DECK_DJ_ERA_GRID,
    DECK_DJ_ERA_TRAP,
    DECK_DJ_ERA_N
};

static const uint8_t DECK_DJ_ARC_MASK[FM_DJ_BUILD_N] = {
    0x3fu, /* THIN        : all eras   */
    0x1bu, /* WASH        : STAX DEAD CRATE GRID (reverb climb) */
    0x3au, /* FB_PONG     : DEAD CRATE GRID TRAP (feedback ping-pong) */
    0x2cu, /* ISO_TANK    : GATE CRATE TRAP (isolator climb) */
    0x34u, /* MOTION_RISE : GATE GRID TRAP (filter/motion riser) */
    0x0bu, /* DUB_ECHO    : STAX DEAD CRATE (delay climb) */
    0x00u, /* TRACK_LIFT  : none */
};

static const uint8_t DECK_DJ_ARC_FALLBACK[DECK_DJ_ERA_N] = {
    FM_DJ_BUILD_WASH,        /* STAX  */
    FM_DJ_BUILD_DUB_ECHO,    /* DEAD  */
    FM_DJ_BUILD_ISO_TANK,    /* GATE  */
    FM_DJ_BUILD_FB_PONG,     /* CRATE */
    FM_DJ_BUILD_MOTION_RISE, /* GRID  */
    FM_DJ_BUILD_FB_PONG,     /* TRAP  */
};

typedef struct {
    fm_dj_t dj;
    fm_dj_fx_ov_t fx_ov;
    dub_params_t vp; /* shadow: only mapped fields are loaded / stored, others stay neutral */
    uint32_t fx_press_ms;
    uint8_t fx_long_used;
    uint8_t fx_long_cancel;
    uint8_t kick_slam_bars;
    uint8_t drop_emph_span;
    uint8_t drop_do_plate; /* DROP alternates plate step / era step */
    uint8_t xf_seq;
    uint8_t last_bar_step;
    uint8_t plan_done;
    uint32_t ui_dirty; /* set by planner, consumed by deck.c (mark_dirty) */

    /* LFO overlay: DJ-only depth on DECK_LFO_* dests, stepped per audio block */
    fm_dj_lfo_ov_t lfo_ov;
    fm_lfo_slot_t lfo_eff[DECK_LFO_TGT_N];
    fm_lfo_rt_t lfo_rt[DECK_LFO_TGT_N];

    /* Seq overlay: ENSEMBLE / FLIP lane mutes (ens_m: 6-bit mask, valid while ens_on) */
    fm_dj_seq_ens_ov_t ens_ov;
    uint8_t ens_on;
    uint8_t ens_m;
    uint8_t flip;

    /* Gesture planner state (fm_app dj_gest_* equivalent, no loop) */
    uint8_t gest_kind;     /* FM_DJ_GEST_SOLO_BASS / SOLO_DRUM / BREAK_MICRO, 0 = none */
    uint8_t gest_bars;
    uint8_t gest_ov;       /* FM_DJ_GEST_OV_ARP while the arp rate overlay runs */
    uint8_t gest_arp_rate; /* DECK arp rate index used under OV_ARP */
    uint8_t gest_arp_cd;
    uint8_t gest_toast;    /* plan id set on arm, consumed by deck.c (deck_toast) */
    uint8_t gest_flam_steps; /* FLAM window in 16th steps (2..5); deck_seq_djo.flam while > 0 */
    uint8_t gest_flam_cd;    /* bars until FLAM may be planned again */
    uint8_t flam_last_st;    /* master_step & 0xff at the last step edge */

    /* Overlay toasts (deck.c queue): last announced LFO target (lt+1) / FX scene (scene+1), 0 = none */
    fm_dj_lfo_toast_hold_t lfo_hold;
    uint8_t toast_lfo_last;
    uint8_t toast_fx_last;
    /* Sticky DJ side label (fm_app dj_toast_param): stable target hold + current label, NULL = none */
    fm_dj_toast_tgt_hold_t tgt_hold;
    const char *toast_side;
} deck_dj_t;

typedef struct {
    int8_t plate; /* 0 none, else plate step */
    int8_t era;   /* 0 none, else era step */
} deck_dj_act_t;

static inline uint8_t deck_dj_arc_ok(uint8_t era, uint8_t arc)
{
    if (era >= DECK_DJ_ERA_N || arc >= FM_DJ_BUILD_N)
        return 0u;
    return (uint8_t)((DECK_DJ_ARC_MASK[arc] >> era) & 1u);
}

static inline uint8_t deck_dj_arc_for_era(uint8_t era, uint8_t arc)
{
    if (era >= DECK_DJ_ERA_N)
        return FM_DJ_BUILD_THIN;
    if (deck_dj_arc_ok(era, arc))
        return arc;
    return DECK_DJ_ARC_FALLBACK[era];
}

static inline fm_dj_app_fx_t deck_dj_fx_io(deck_dj_t *d)
{
    fm_dj_app_fx_t io;

    io.press_ms = &d->fx_press_ms;
    io.long_used = &d->fx_long_used;
    io.long_cancel = &d->fx_long_cancel;
    io.dj = &d->dj;
    io.ui_dirty = &d->ui_dirty;
    return io;
}

/* Planner adapter: planner only rolls FILTER_SLAM on techno/triphop builds; KO KIT adds it here (~14% of DROP). */
static inline void deck_dj_post_plan(deck_dj_t *d, uint8_t era)
{
    fm_dj_t *dj = &d->dj;

    if (dj->trans_type == FM_DJ_TRANS_DROP && (fm_dj_rng_u8(dj) % 100u) < 14u) {
        dj->trans_type = FM_DJ_TRANS_FILTER_SLAM;
        dj->build_arc = ((fm_dj_rng_u8(dj) % 12u) < 6u) ? FM_DJ_BUILD_MOTION_RISE : FM_DJ_BUILD_THIN;
    }
    dj->build_arc = deck_dj_arc_for_era(era, dj->build_arc);
}

/* Transition -> deck action (executed by deck.c with cycle_plate / cycle_era). Mirrors fm_dj_on_drop / on_trans. */
static inline deck_dj_act_t deck_dj_trans_act(deck_dj_t *d, uint8_t trans, uint8_t rng4, uint8_t *kick_slam_bars)
{
    deck_dj_act_t a = {0, 0};

    if (trans == FM_DJ_TRANS_DROP || trans == FM_DJ_TRANS_FILTER_SLAM) {
        uint8_t span = fm_dj_drop_emph_span_phrase(rng4);

        *kick_slam_bars = span;
        d->drop_emph_span = span;
        if (d->drop_do_plate)
            a.plate = -1;
        else
            a.era = (int8_t)fm_dj_trans_era_dir(rng4);
        d->drop_do_plate ^= 1u;
        return a;
    }
    *kick_slam_bars = 0u;
    d->drop_emph_span = 0u;
    if (trans == FM_DJ_TRANS_CROSSFADE || trans == FM_DJ_TRANS_ECHO_OUT) {
        if (fm_dj_trans_era_step_picked(trans, d->xf_seq))
            a.era = (int8_t)fm_dj_trans_era_dir(rng4);
        else
            a.plate = (int8_t)fm_dj_trans_plate_dir(trans);
        d->xf_seq++;
    } else if (trans == FM_DJ_TRANS_CUEING) {
        a.plate = 1;
    }
    return a;
}

/* ---- Gesture planner (fm_app dub_dj_gest_* equivalent) ---- */

static inline uint8_t deck_dj_gest_active(const deck_dj_t *d)
{
    return (uint8_t)(d->gest_kind != 0u || d->gest_bars != 0u || d->gest_ov != FM_DJ_GEST_OV_NONE ||
                     d->gest_flam_steps != 0u);
}

static inline void deck_dj_gest_restore(deck_dj_t *d)
{
    d->gest_kind = 0u;
    d->gest_bars = 0u;
    d->gest_ov = FM_DJ_GEST_OV_NONE;
    d->gest_flam_steps = 0u;
    d->ui_dirty = 1u;
}

static inline void deck_dj_gest_arm(deck_dj_t *d, uint8_t plan)
{
    static const uint8_t ARP_RATE[2] = {1u, 3u}; /* DECK_ARP_RATE: 1/16, 1/16T */
    uint8_t rng;

    if (plan == FM_DJ_GEST_NONE)
        return;
    if (deck_dj_gest_active(d))
        deck_dj_gest_restore(d);
    rng = (uint8_t)fm_dj_rng_u8(&d->dj);
    switch (plan) {
    case FM_DJ_GEST_SOLO_BASS:
    case FM_DJ_GEST_SOLO_DRUM:
    case FM_DJ_GEST_BREAK_MICRO:
        d->gest_kind = plan;
        d->gest_bars = fm_dj_gest_plan_bars(plan, rng);
        break;
    case FM_DJ_GEST_ARP:
        d->gest_ov = FM_DJ_GEST_OV_ARP;
        d->gest_arp_rate = ARP_RATE[(rng >> 1) & 1u];
        d->gest_bars = fm_dj_gest_plan_bars(plan, rng);
        d->gest_arp_cd = FM_DJ_GEST_ARP_CD_BARS;
        break;
    case FM_DJ_GEST_FLAM: /* same length and cooldown as fm_app dub_dj_gest_arm */
        d->gest_flam_steps = (uint8_t)(2u + (rng & 3u));
        d->gest_flam_cd = FM_DJ_GEST_FLAM_CD_BARS;
        break;
    default: /* LOOP / FILL: no KO KIT equivalent */
        return;
    }
    d->gest_toast = plan;
    d->ui_dirty = 1u;
}

/* Bar edge: abort/countdown, then plan a new gesture. */
static inline void deck_dj_gest_on_bar(deck_dj_t *d, uint8_t playing)
{
    fm_dj_gest_env_t env;
    uint8_t plan;

    if (!d->dj.on) {
        if (deck_dj_gest_active(d))
            deck_dj_gest_restore(d);
        return;
    }
    if (deck_dj_gest_active(d) && fm_dj_gest_st_abort(&d->dj))
        deck_dj_gest_restore(d);
    if (d->gest_bars > 0u) {
        d->gest_bars--;
        if (d->gest_bars == 0u)
            deck_dj_gest_restore(d);
    }
    if (d->gest_arp_cd > 0u)
        d->gest_arp_cd--;
    if (d->gest_flam_cd > 0u)
        d->gest_flam_cd--;
    if (deck_dj_gest_active(d))
        return;
    env.caps = DECK_DJ_GEST_CAPS;
    env.manual_gesture_latch = 0u;
    env.manual_arp_mode = 0u;
    env.loop_active = 0u;
    env.loop_cd_bars = 0u;
    env.arp_cd_bars = d->gest_arp_cd;
    env.gest_active = 0u;
    env.fill_build_live = 0u;
    env.playing = playing ? 1u : 0u;
    env.flam_cd_bars = d->gest_flam_cd;
    plan = fm_dj_gest_plan_pick(&d->dj, d->dj.phrase_bar, &env);
    deck_dj_gest_arm(d, plan);
}

static inline const char *deck_dj_gest_label(uint8_t plan)
{
    switch (plan) {
    case FM_DJ_GEST_SOLO_BASS:
        return "SOLO BASS";
    case FM_DJ_GEST_SOLO_DRUM:
        return "SOLO DRUM";
    case FM_DJ_GEST_BREAK_MICRO:
        return "BREAK";
    case FM_DJ_GEST_ARP:
        return "ARP";
    case FM_DJ_GEST_FLAM:
        return "FLAM!";
    default:
        return "";
    }
}

/* FX overlay label for the toast queue: strongest live FX slot with a scene label, as scene+1 (0 = none).
 * Same pick as fm_dj_fx_toast_label_vp, without its audibility check (no eff/manual pair on deck). */
static inline uint8_t deck_dj_fx_toast_scene(const deck_dj_t *d)
{
    uint8_t si;
    int32_t best = FM_DJ_TOAST_AUDIBLE;
    uint8_t pick = 0u;

    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_fx_slot_ov_t *o = &d->fx_ov.per[si];
        int32_t ad;

        if (!o->active || !fm_dj_fx_scene_toast(o->scene))
            continue;
        ad = fm_dj_fx_strength(&d->dj.slot[si]);
        if (ad > best) {
            best = ad;
            pick = (uint8_t)(o->scene + 1u);
        }
    }
    return pick;
}

/* Solo mask override while a solo gesture runs; 0 = keep the user solo. */
static inline uint16_t deck_dj_solo(const deck_dj_t *d, uint16_t base)
{
    if (d->gest_kind == FM_DJ_GEST_SOLO_BASS)
        return DECK_DJ_SOLO_BASS;
    if (d->gest_kind == FM_DJ_GEST_SOLO_DRUM)
        return DECK_DJ_SOLO_DRUM;
    return base;
}

static inline uint8_t deck_dj_arp_rate(const deck_dj_t *d, uint8_t base)
{
    return d->gest_ov == FM_DJ_GEST_OV_ARP ? d->gest_arp_rate : base;
}

/* ---- LFO overlay (fm_app dub_dj_lfo_* equivalent, DECK_LFO_* dests) ---- */

/* DJ target -> DECK_LFO dest (global FX/perf only; SEQ_* are handled by deck_seq_djo). DECK_LFO_TGT_N = none. */
static inline uint8_t deck_dj_lfo_lt(uint8_t dj_tgt)
{
    switch (dj_tgt) {
    case FM_DJ_TGT_MOTION:
        return DECK_LFO_WOW;
    case FM_DJ_TGT_FLUX:
        return DECK_LFO_WIDTH;
    case FM_DJ_TGT_WOB:
        return DECK_LFO_TAPE;
    case FM_DJ_TGT_ISO:
        return DECK_LFO_ISO;
    case FM_DJ_TGT_SPACE:
        return DECK_LFO_REV;
    case FM_DJ_TGT_WEIGHT:
        return DECK_LFO_COMP;
    case FM_DJ_TGT_DUB:
        return DECK_LFO_EMIX;
    case FM_DJ_TGT_CUTOFF:
        return DECK_LFO_FXFILTER;
    case FM_DJ_TGT_FB:
        return DECK_LFO_EFB;
    case FM_DJ_TGT_DENS_H:
    case FM_DJ_TGT_DENS_K:
        return DECK_LFO_DENS;
    default:
        return DECK_LFO_TGT_N;
    }
}

/* Shape hints in fm_dj_lfo_fill_plan use DUB codes; only FB and WOB are special there. */
static inline uint8_t deck_dj_lfo_shape_tgt(uint8_t dj_tgt)
{
    if (dj_tgt == FM_DJ_TGT_FB)
        return DUB_LFO_FB;
    if (dj_tgt == FM_DJ_TGT_WOB)
        return DUB_LFO_M_WOB;
    return 0xffu;
}

static inline void deck_dj_lfo_track(deck_dj_t *d, uint8_t blocked)
{
    fm_dj_lfo_ov_t *ov = &d->lfo_ov;
    uint8_t si;

    if (blocked) {
        fm_dj_lfo_ov_reset(ov);
        return;
    }
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_slot_t *s = &d->dj.slot[si];
        fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        uint8_t lt;
        int32_t live;

        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0) {
            o->active = 0u;
            continue;
        }
        lt = deck_dj_lfo_lt(s->target);
        if (lt >= DECK_LFO_TGT_N) {
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
        if (!o->active || o->dj_tgt != s->target || o->lfo_tgt != lt) {
            o->active = 1u;
            o->dj_tgt = s->target;
            o->lfo_tgt = lt;
            o->saved.depth = 0u; /* DJ-only: user LFO is applied earlier (deck_lfo_apply_eff) */
            fm_dj_lfo_fill_plan(&o->plan, deck_dj_lfo_shape_tgt(s->target), s->role, fm_dj_lfo_shape_from_slot(s),
                                fm_dj_clamp_u8((int32_t)(s->peak < 0 ? -s->peak : s->peak), 1, 127), &d->dj);
        }
    }
}

/* DJ-only LFO delta on a dest, 0 if no DJ overlay is live on it. */
static inline int32_t deck_dj_lfo_delta(deck_dj_t *d, uint8_t dj_tgt)
{
    uint8_t lt = deck_dj_lfo_lt(dj_tgt);

    if (lt >= DECK_LFO_TGT_N || !fm_dj_lfo_tgt_overlay_live(&d->lfo_ov, &d->dj, lt))
        return 0;
    return fm_lfo_slot_delta(d->lfo_eff, d->lfo_rt, lt);
}

/* True if a knob mapped to LFO dest lt is touched by DJ (overlay live or a DJ target delta on it). */
static inline uint8_t deck_dj_lt_live(const deck_dj_t *d, uint8_t lt)
{
    uint8_t t;

    if (lt >= DECK_LFO_TGT_N || (!d->dj.on && !d->dj.slew_off))
        return 0u;
    if (fm_dj_lfo_tgt_overlay_live(&d->lfo_ov, &d->dj, lt))
        return 1u;
    for (t = 0u; t < FM_DJ_TGT_N; t++) {
        if (deck_dj_lfo_lt(t) == lt && fm_dj_delta_for_tgt(&d->dj, t) != 0)
            return 1u;
    }
    return 0u;
}

static inline uint16_t deck_dj_role_col(const deck_dj_t *d)
{
    switch (d->dj.role) {
    case FM_DJ_ROLE_RIDE:
        return RGB(80, 220, 255);
    case FM_DJ_ROLE_THROW:
        return RGB(255, 180, 60);
    case FM_DJ_ROLE_FILTER:
        return RGB(255, 100, 220);
    case FM_DJ_ROLE_BREAK:
        return RGB(255, 70, 70);
    case FM_DJ_ROLE_DROP:
        return C_WHITE;
    default:
        return C_ACC;
    }
}

/* ---- Seq overlay (fm_app dub_apply_dj_seq_overlay / seq lane targets equivalent) ---- */

static inline int8_t deck_dj_scale(int32_t d, int32_t mul, int32_t lim)
{
    int32_t r = d * mul / 127;

    if (r < -lim)
        r = -lim;
    if (r > lim)
        r = lim;
    return (int8_t)r;
}

/* Per-voice lane deltas -> deck_seq_djo (vel ±50, gate ±50, pitch ±24 semis, trig ±100 raw). */
static inline void deck_dj_seq_sync(deck_dj_t *d)
{
    uint8_t v;
    uint8_t live = (d->dj.on || d->dj.slew_off) ? 1u : 0u;
    uint8_t emph = 0u;

    if (live)
        emph = fm_dj_any_slot_role(&d->dj, FM_DJ_ROLE_DROP) ? FM_DJ_DROP_EMPH_PEAK : fm_dj_drop_emph_level(d->kick_slam_bars, d->drop_emph_span);
    deck_seq_djo.flam = (live && d->gest_flam_steps) ? DECK_DJ_FLAM_VOICES : 0u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        uint8_t ln = DECK_DJ_VOICE_LANE[v];

        if (!live) {
            deck_seq_djo.vel[v] = 0;
            deck_seq_djo.gate[v] = 0;
            deck_seq_djo.pitch[v] = 0;
            deck_seq_djo.trig[v] = 0;
            continue;
        }
        deck_seq_djo.vel[v] = deck_dj_scale(fm_dj_delta_for_tgt_lane(&d->dj, FM_DJ_TGT_SEQ_VEL, ln), 50, 50);
        if (emph) { /* drop emphasis lift (fm_app formula, base vel 110 assumed: deck steps carry no absolute vel here) */
            int32_t vd = deck_seq_djo.vel[v];

            if (ln == 0u)
                vd += ((127 - 110) * emph) / 127;
            else if (ln == 1u)
                vd += ((127 - 110) * emph * 9) / (127 * 10);
            else if (ln == 2u && emph >= 96u)
                vd += (40 * emph) / 127;
            if (vd > 50)
                vd = 50;
            deck_seq_djo.vel[v] = (int8_t)vd;
        }
        deck_seq_djo.gate[v] = deck_dj_scale(fm_dj_delta_for_tgt_lane(&d->dj, FM_DJ_TGT_SEQ_GATE, ln), 50, 50);
        deck_seq_djo.pitch[v] = deck_dj_scale(fm_dj_delta_for_tgt_lane(&d->dj, FM_DJ_TGT_SEQ_PITCH, ln), 24, 24);
        deck_seq_djo.trig[v] = deck_dj_scale(fm_dj_delta_for_tgt_lane(&d->dj, FM_DJ_TGT_SEQ_TRIG, ln), 100, 100);
    }
}

/* WASH break thinning (fm_app: on[0]=0, on[2]/on[4]/on[5] by sb). Voice mute, lanes 1 and 3 untouched. */
static inline uint16_t deck_dj_break_thin_mute(const deck_dj_t *d)
{
    uint16_t mute = 0u;
    uint8_t sb = (uint8_t)(d->last_bar_step & (DECK_DJ_BAR_STEPS - 1u));
    uint8_t v;

    if (!d->dj.on || d->gest_kind != 0u || !fm_dj_break_seq_thin(&d->dj))
        return 0u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        uint8_t ln = DECK_DJ_VOICE_LANE[v];
        uint8_t keep = 1u;

        if (ln == 0u)
            keep = 0u;
        else if (ln == 2u)
            keep = (uint8_t)((sb & 3u) == 2u);
        else if (ln == 4u)
            keep = (uint8_t)(sb == 0u);
        else if (ln == 5u)
            keep = (uint8_t)(sb == 2u);
        if (!keep)
            mute |= (uint16_t)(1u << v);
    }
    return mute;
}

/* Render-time lane mute: ENSEMBLE overlay replaces the lane mask, FLIP inverts it, break thinning ORs on top. */
static inline uint16_t deck_dj_lane_mute(const deck_dj_t *d, uint16_t base)
{
    uint16_t m = base;

    if (!d->dj.on && !d->dj.slew_off)
        return base;
    if (d->ens_on)
        m = deck_ens_mute_expand(d->ens_m);
    if (d->flip)
        m = (uint16_t)~m;
    return (uint16_t)(m | deck_dj_break_thin_mute(d));
}

/* Targets -> deck fields. DJ-only delta on top of the effective (LFO-applied) value. */
static inline uint8_t deck_dj_mod(uint8_t base, deck_dj_t *d, uint8_t tgt)
{
    int32_t dd = fm_dj_delta_for_tgt(&d->dj, tgt);

    if (tgt != FM_DJ_TGT_DENS_K) /* DENS_H already carries the DENS LFO overlay */
        dd += deck_dj_lfo_delta(d, tgt);
    if (!dd)
        return base;
    return fm_dj_clamp_u8((int32_t)base + dd, 0, 127);
}

static inline void deck_dj_apply_targets(deck_dj_t *d, deck_master_t *m, deck_perf_t *p)
{
    m->wow = deck_dj_mod(m->wow, d, FM_DJ_TGT_MOTION);
    m->width = deck_dj_mod(m->width, d, FM_DJ_TGT_FLUX);
    m->tape = deck_dj_mod(m->tape, d, FM_DJ_TGT_WOB);
    m->isolator = deck_dj_mod(m->isolator, d, FM_DJ_TGT_ISO);
    m->rev_mix = deck_dj_mod(m->rev_mix, d, FM_DJ_TGT_SPACE);
    m->comp = deck_dj_mod(m->comp, d, FM_DJ_TGT_WEIGHT);
    m->dly_lvl = deck_dj_mod(m->dly_lvl, d, FM_DJ_TGT_DUB);
    m->filter = deck_dj_mod(m->filter, d, FM_DJ_TGT_CUTOFF);
    m->feedback = deck_dj_mod(m->feedback, d, FM_DJ_TGT_FB);
    p->density = deck_dj_mod(p->density, d, FM_DJ_TGT_DENS_H);
    p->density = deck_dj_mod(p->density, d, FM_DJ_TGT_DENS_K);
}

/* Shadow mapping. Climax/FX use cutoff_c as the filter sweep: deck filter (64 = off) is shared with it. */
static inline void deck_dj_load_vp(dub_params_t *vp, const deck_master_t *m, const deck_perf_t *p, uint32_t bpm)
{
    vp->rev_mix = m->rev_mix;
    vp->rev_tail = m->rev_tail;
    vp->rev_damp = m->rev_damp;
    vp->dly_rev = m->dly_rev;
    vp->dly_lvl = m->dly_lvl;
    vp->dly_time = m->dly_time;
    vp->feedback = m->feedback;
    vp->loop_filt = m->loop_filt;
    vp->isolator = m->isolator;
    vp->tank = m->tank;
    vp->wow = m->wow;
    vp->cutoff_c = m->filter;
    vp->pump = m->comp;
    vp->density = p->density;
    vp->bpm = (uint8_t)bpm;
}

static inline void deck_dj_store_vp(const dub_params_t *vp, deck_master_t *m, deck_perf_t *p)
{
    m->rev_mix = vp->rev_mix;
    m->rev_tail = vp->rev_tail;
    m->rev_damp = vp->rev_damp;
    m->dly_rev = vp->dly_rev;
    m->dly_lvl = vp->dly_lvl;
    m->dly_time = vp->dly_time;
    m->feedback = vp->feedback;
    m->loop_filt = vp->loop_filt;
    m->isolator = vp->isolator;
    m->tank = vp->tank;
    m->wow = vp->wow;
    m->filter = vp->cutoff_c;
    m->comp = vp->pump;
    p->density = vp->density;
}

/* ---- Main-loop step ---- */

/* Main-loop step: bar edge, phrase planner, era-mapped plan, FX/LFO/gesture/seq overlays. Call once per app_step. */
static inline void deck_dj_app_step(deck_dj_t *d, const deck_runtime_t *r, uint8_t era, fm_dj_app_trans_fn on_trans)
{
    uint32_t master_step = r->master_step;
    uint8_t playing = r->playing ? 1u : 0u;
    uint8_t bar_step = (uint8_t)(master_step & (DECK_DJ_BAR_STEPS - 1u));
    uint8_t bar_edge = (uint8_t)(bar_step == 0u && bar_step != d->last_bar_step);
    uint8_t blocked;
    uint8_t ret;
    uint8_t base_ens;
    int32_t fd;

    if ((uint8_t)master_step != d->flam_last_st) {
        d->flam_last_st = (uint8_t)master_step;
        if (d->gest_flam_steps > 0u) {
            d->gest_flam_steps--;
            if (d->gest_flam_steps == 0u)
                d->ui_dirty = 1u;
        }
    }
    if (bar_step != d->last_bar_step) {
        if (bar_step == 0u) {
            if (d->dj.on)
                d->dj.bar_pending = 1u;
            if (d->kick_slam_bars > 0u) {
                d->kick_slam_bars--;
                if (d->kick_slam_bars == 0u)
                    d->drop_emph_span = 0u;
            }
        }
        d->last_bar_step = bar_step;
    }
    fm_dj_app_phrase_step(&d->dj, bar_step, DECK_DJ_BAR_STEPS, 0u, &d->ui_dirty, on_trans, &d->kick_slam_bars);
    if (bar_edge)
        deck_dj_gest_on_bar(d, playing);
    if (d->dj.on && d->dj.phrase_bar == 0u) {
        if (!d->plan_done) {
            deck_dj_post_plan(d, era);
            d->plan_done = 1u;
        }
    } else {
        d->plan_done = 0u;
    }
    fm_dj_fx_ov_track(&d->fx_ov, &d->dj, fm_dj_fx_blocked(&d->dj, playing, bar_step, DECK_DJ_BAR_STEPS));

    deck_dj_lfo_track(d, fm_dj_lfo_blocked(&d->dj, playing, bar_step, DECK_DJ_BAR_STEPS));
    fm_dj_lfo_eff_build(&d->lfo_ov, &d->dj, DECK_DJ_LFO_ZERO, d->lfo_eff, DECK_LFO_TGT_N);

    base_ens = deck_detect_ensemble(r);
    blocked = fm_dj_seq_ens_blocked(&d->dj, playing, bar_step, DECK_DJ_BAR_STEPS);
    ret = fm_dj_seq_ens_ov_step(&d->dj, &d->ens_ov, (uint8_t)(master_step / DECK_DJ_BAR_STEPS), bar_step,
                                DECK_DJ_BAR_STEPS, playing, base_ens, (uint8_t)(deck_lane_mute_mask16(r) & 0x3fu),
                                fm_dj_delta_for_tgt(&d->dj, FM_DJ_TGT_ENSEMBLE), DECK_ENS_MUTE6, blocked);
    /* Restore value on overlay exit is not applied: the user lane mute was never changed. */
    d->ens_on = d->ens_ov.active ? 1u : 0u;
    d->ens_m = ret;
    if (d->ens_on && !fm_dj_seq_mask_dj_intervention_ok(d->ens_m)) {
        uint8_t safe = fm_dj_seq_ens_safe_combo(DECK_ENS_MUTE6);

        d->ens_ov.combo_cur = safe;
        d->ens_ov.toast_ens = safe;
        d->ens_ov.invert_accent = 0u;
        d->ens_ov.invert_left = 0u;
        d->ens_m = fm_dj_seq_mask_low6(DECK_ENS_MUTE6[safe]);
    }
    fd = fm_dj_delta_for_tgt(&d->dj, FM_DJ_TGT_FLIP);
    d->flip = fm_dj_seq_flip_want(&d->dj, playing, bar_step, DECK_DJ_BAR_STEPS, fd);
    if (d->flip && d->ens_on && !fm_dj_seq_mask_dj_intervention_ok((uint8_t)~d->ens_m))
        d->flip = 0u;

    deck_dj_seq_sync(d);
}

/* Audio block: call on the effective copy (deck_lfo_apply_eff pattern); caller restores master/perf/cfg after render. */
static inline void deck_dj_apply_eff(deck_dj_t *d, deck_master_t *m, deck_perf_t *p, deck_voice_cfg_t *cfg,
                                     uint32_t bpm, uint32_t n, uint8_t playing, uint8_t sel)
{
    if (!d->dj.on && !d->dj.slew_off)
        return;
    fm_lfo_slots_sanitize_rates(d->lfo_eff, DECK_LFO_TGT_N, bpm);
    fm_lfo_slots_step_tr(d->lfo_eff, d->lfo_rt, DECK_LFO_TGT_N, sel, bpm, n, DECK_FS, playing);
    if (d->gest_kind == FM_DJ_GEST_BREAK_MICRO)
        cfg[DECK_BD].level = (uint8_t)(cfg[DECK_BD].level >> 2);
    deck_dj_apply_targets(d, m, p);
    deck_dj_load_vp(&d->vp, m, p, bpm);
    fm_dj_apply_phrase_climax(&d->vp, &d->dj, d->kick_slam_bars, d->drop_emph_span);
    fm_dj_fx_apply(&d->vp, &d->dj, &d->fx_ov, d->last_bar_step);
    deck_dj_store_vp(&d->vp, m, p);
}

/* Drop silence: BREAK bar before DROP mutes every voice (dub: 6 lanes, KO KIT: no lane pick). */
static inline uint16_t deck_dj_silence_mask(const deck_dj_t *d, uint8_t playing)
{
    return fm_dj_pre_drop_silence(&d->dj, playing ? 1u : 0u, d->last_bar_step, DECK_DJ_BAR_STEPS) ? 0xffffu : 0u;
}

#endif /* DECK_DJ_H */
