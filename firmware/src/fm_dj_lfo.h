/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ temporary LFO plans on macro/param targets (overlay — never writes user slots). */
#ifndef FM_DJ_LFO_H
#define FM_DJ_LFO_H

#include "dub_types.h"
#include "fm_dj.h"
#include "fm_lfo.h"
#include <stdint.h>

typedef struct {
    uint8_t active;
    uint8_t lfo_tgt;
    uint8_t dj_tgt;
    fm_lfo_slot_t saved;
    fm_lfo_slot_t plan;
} fm_dj_lfo_slot_ov_t;

typedef struct {
    fm_dj_lfo_slot_ov_t per[FM_DJ_SLOT_N];
} fm_dj_lfo_ov_t;

static inline void fm_dj_lfo_ov_reset(fm_dj_lfo_ov_t *ov)
{
    uint8_t i;

    for (i = 0u; i < FM_DJ_SLOT_N; i++)
        ov->per[i].active = 0u;
}

static inline uint8_t fm_dj_lfo_map_tgt(uint8_t dj_tgt)
{
    switch (dj_tgt) {
    case FM_DJ_TGT_MOTION:
        return DUB_LFO_M_MOTION;
    case FM_DJ_TGT_FLUX:
        return DUB_LFO_M_FLUX;
    case FM_DJ_TGT_WOB:
        return DUB_LFO_M_WOB;
    case FM_DJ_TGT_ISO:
        return DUB_LFO_M_ISO;
    case FM_DJ_TGT_SPACE:
        return DUB_LFO_ESEND;
    case FM_DJ_TGT_WEIGHT:
        return DUB_LFO_M_WEIGHT;
    case FM_DJ_TGT_DUB:
        return DUB_LFO_EMIX;
    case FM_DJ_TGT_CUTOFF:
        return DUB_LFO_LOOP;
    case FM_DJ_TGT_FB:
        return DUB_LFO_FB;
    case FM_DJ_TGT_DENS_H:
        return DUB_LFO_HAT;
    case FM_DJ_TGT_DENS_K:
        return DUB_LFO_DENS_K;
    case FM_DJ_TGT_SEQ_VEL:
        return DUB_LFO_SEQ_VEL;
    case FM_DJ_TGT_SEQ_TRIG:
        return DUB_LFO_SEQ_TRIG;
    case FM_DJ_TGT_SEQ_GATE:
        return DUB_LFO_SEQ_GATE;
    case FM_DJ_TGT_SEQ_PITCH:
        return DUB_LFO_SEQ_PITCH;
    default:
        return 0xffu;
    }
}

static inline uint8_t fm_dj_lfo_shape_from_slot(const fm_dj_slot_t *s)
{
    if (s->hold_bars >= 4u || s->clock_div >= 8u)
        return 0u; /* long sweep */
    if (s->hold_bars <= 1u && s->clock_div <= 2u)
        return 2u; /* burst */
    return 1u;       /* rhythmic */
}

static inline void fm_dj_lfo_fill_plan(fm_lfo_slot_t *p, uint8_t lfo_tgt, uint8_t role, uint8_t shape,
                                       uint8_t peak_pct, fm_dj_t *dj)
{
    uint8_t r = (uint8_t)fm_dj_rng_u8(dj);
    uint8_t depth;
    uint8_t rate;
    uint8_t wave;

    fm_lfo_slot_reset(p);
    depth = (uint8_t)(24u + (peak_pct >> 1) + (r % 14u));
    if (depth > 58u)
        depth = 58u;
    if (depth < 22u)
        depth = 22u;

    if (shape == 0u) {
        rate = (uint8_t)(3u + (r % 4u));
        wave = (r & 1u) ? DUB_LFO_TRI : DUB_LFO_SINE;
        if (depth > 38u)
            depth = 38u;
    } else if (shape == 2u) {
        rate = (uint8_t)(8u + (r % 6u));
        wave = (r % 3u) == 0u ? 15u : DUB_LFO_TRI;
        p->phase = (uint8_t)((r & 0x3fu));
        if (depth > 32u)
            depth = 32u;
    } else {
        rate = (uint8_t)(5u + (r % 5u));
        wave = (r & 2u) ? DUB_LFO_TRI : DUB_LFO_SINE;
        if ((r % 5u) == 0u)
            wave = DUB_LFO_SH;
    }

    if (lfo_tgt == DUB_LFO_FB || lfo_tgt == DUB_LFO_M_FEED) {
        if (wave == DUB_LFO_SQR || wave == DUB_LFO_STEP)
            wave = DUB_LFO_TRI;
        if (depth > 28u)
            depth = 28u;
    }
    if (lfo_tgt == DUB_LFO_M_WOB || lfo_tgt == DUB_LFO_WOB)
        wave = DUB_LFO_TRI;
    if (role == FM_DJ_ROLE_FILTER && shape != 2u) {
        rate = (uint8_t)(rate < 4u ? 4u : rate);
        wave = DUB_LFO_SINE;
    }
    if (role == FM_DJ_ROLE_THROW && shape == 2u)
        p->phase = 0u;

    p->depth = depth;
    p->rate = rate;
    p->wave = wave;
    p->phase = (uint8_t)((r * 17u) & 0x7fu);
    p->smooth = shape == 0u ? 12u : (shape == 2u ? 6u : 18u);
    p->skew = FM_LFO_SKEW_DEFAULT;
    p->algo = FM_LFO_ALG_CLASSIC;
    if (wave == DUB_LFO_SH && shape == 1u)
        p->algo = FM_LFO_ALG_SH_FM;
    if (shape == 2u)
        p->phase = 0u;
}

static inline uint8_t fm_dj_lfo_blocked(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                        uint8_t steps_per_bar)
{
    if (!dj->on && !dj->slew_off)
        return 1u;
    if (dj->fill_blocked)
        return 1u;
    if (playing && fm_dj_pre_drop_silence(dj, 1u, bar_step, steps_per_bar))
        return 1u;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_DROP))
        return 1u;
    return 0u;
}

static inline void fm_dj_lfo_ov_track(fm_dj_lfo_ov_t *ov, fm_dj_t *dj, const fm_lfo_slot_t *user_slots,
                                      uint8_t blocked)
{
    uint8_t si;

    if (blocked) {
        fm_dj_lfo_ov_reset(ov);
        return;
    }
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_slot_t *s = &dj->slot[si];
        fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        uint8_t lt;
        int32_t live;

        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0) {
            o->active = 0u;
            continue;
        }
        lt = fm_dj_lfo_map_tgt(s->target);
        if (lt >= DUB_LFO_TGT_N) {
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
            o->saved = user_slots[lt];
            fm_dj_lfo_fill_plan(&o->plan, lt, s->role, fm_dj_lfo_shape_from_slot(s),
                                fm_dj_clamp_u8((int32_t)(s->peak < 0 ? -s->peak : s->peak), 1, 127), dj);
        }
    }
}

static inline int32_t fm_dj_lfo_slot_drive(const fm_dj_slot_t *s)
{
    int32_t ad = (int32_t)s->delta;

    if (ad < 0)
        ad = -ad;
    if (s->phase == FM_DJ_SLOT_ATTACK || s->phase == FM_DJ_SLOT_HOLD) {
        int32_t pk = (int32_t)s->peak;

        if (pk < 0)
            pk = -pk;
        if (pk > ad)
            ad = (int32_t)((ad + pk + 1) / 2);
    }
    return ad;
}

static inline uint8_t fm_dj_lfo_depth_scaled(uint8_t plan_depth, const fm_dj_slot_t *s)
{
    int32_t sc = fm_dj_lfo_slot_drive(s);
    int32_t d;

    d = ((int32_t)plan_depth * sc) / 127;
    if (d < 0)
        d = 0;
    if (d > 127)
        return 127u;
    if (d > 0 && d < 8 && sc > FM_DJ_TOAST_AUDIBLE)
        return 8u;
    if (d == 0 && s->phase != FM_DJ_SLOT_OFF && sc > 0)
        return (uint8_t)(plan_depth / 5u + 6u);
    return (uint8_t)d;
}

static inline void fm_dj_lfo_eff_build(const fm_dj_lfo_ov_t *ov, const fm_dj_t *dj, const fm_lfo_slot_t *user,
                                       fm_lfo_slot_t *eff, uint32_t ndests)
{
    uint32_t t;
    uint8_t si;

    for (t = 0u; t < ndests; t++)
        eff[t] = user[t];

    if (!dj->on && !dj->slew_off)
        return;

    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        const fm_dj_slot_t *s;
        fm_lfo_slot_t *e;
        uint8_t dj_d;

        if (!o->active || o->lfo_tgt >= ndests)
            continue;
        s = &dj->slot[si];
        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0)
            continue;
        dj_d = fm_dj_lfo_depth_scaled(o->plan.depth, s);
        if (dj_d == 0u)
            continue;
        e = &eff[o->lfo_tgt];
        e->depth = o->saved.depth > 0u ? fm_dj_clamp_u8((int32_t)o->saved.depth + (int32_t)dj_d, 0, 127) : dj_d;
        e->rate = o->plan.rate;
        e->wave = o->plan.wave;
        e->phase = o->plan.phase;
        e->smooth = o->plan.smooth;
        e->skew = o->plan.skew;
        e->algo = o->plan.algo;
        e->ratio = o->plan.ratio;
        e->index = o->plan.index;
        e->fb = o->plan.fb;
        e->fold = o->plan.fold;
    }
}

static inline int32_t fm_dj_lfo_slot_delta_sum(const fm_lfo_slot_t *eff, const fm_lfo_rt_t *rt, uint8_t t)
{
    return fm_lfo_slot_delta(eff, rt, t);
}

static inline void fm_dj_lfo_eff_tgt(const fm_dj_lfo_ov_t *ov, const fm_dj_t *dj, const fm_lfo_slot_t *user,
                                     fm_lfo_slot_t *e, uint8_t lt)
{
    uint8_t si;

    if (lt >= DUB_LFO_TGT_N) {
        *e = user[0];
        return;
    }
    *e = user[lt];
    if (!dj->on && !dj->slew_off)
        return;
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        const fm_dj_slot_t *s;
        uint8_t dj_d;

        if (!o->active || o->lfo_tgt != lt)
            continue;
        s = &dj->slot[si];
        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0)
            continue;
        dj_d = fm_dj_lfo_depth_scaled(o->plan.depth, s);
        if (dj_d == 0u)
            continue;
        e->depth = o->saved.depth > 0u ? fm_dj_clamp_u8((int32_t)o->saved.depth + (int32_t)dj_d, 0, 127) : dj_d;
        e->rate = o->plan.rate;
        e->wave = o->plan.wave;
        e->phase = o->plan.phase;
        e->smooth = o->plan.smooth;
        e->skew = o->plan.skew;
        e->algo = o->plan.algo;
        e->ratio = o->plan.ratio;
        e->index = o->plan.index;
        e->fb = o->plan.fb;
        e->fold = o->plan.fold;
    }
}

#ifndef FM_DJ_LFO_TOAST_MIN_DELTA
#define FM_DJ_LFO_TOAST_MIN_DELTA 4
#endif

static inline int fm_dj_lfo_toast_audible(const fm_lfo_slot_t *eff, const fm_lfo_rt_t *rt, uint8_t lfo_tgt)
{
    int32_t d;

    if (lfo_tgt >= DUB_LFO_TGT_N || !eff[lfo_tgt].depth)
        return 0;
    d = fm_lfo_slot_delta(eff, rt, lfo_tgt);
    if (d < 0)
        d = -d;
    return d >= FM_DJ_LFO_TOAST_MIN_DELTA;
}

static inline uint8_t fm_dj_lfo_toast_pick_tgt(const fm_dj_lfo_ov_t *ov, const fm_dj_t *dj, const fm_lfo_slot_t *eff,
                                               const fm_lfo_rt_t *rt)
{
    uint8_t si;
    uint8_t best_lt = 0xffu;
    int32_t best_mag = FM_DJ_LFO_TOAST_MIN_DELTA - 1;

    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        const fm_dj_slot_t *s = &dj->slot[si];
        int32_t d;

        if (!o->active || o->lfo_tgt >= DUB_LFO_TGT_N)
            continue;
        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0)
            continue;
        if (fm_dj_lfo_depth_scaled(o->plan.depth, s) == 0u)
            continue;
        if (!fm_dj_lfo_toast_audible(eff, rt, o->lfo_tgt))
            continue;
        d = fm_lfo_slot_delta(eff, rt, o->lfo_tgt);
        if (d < 0)
            d = -d;
        if (d > best_mag) {
            best_mag = d;
            best_lt = o->lfo_tgt;
        }
    }
    return best_lt;
}

#ifndef FM_DJ_LFO_TOAST_SWITCH_MARGIN
#define FM_DJ_LFO_TOAST_SWITCH_MARGIN 3
#endif
#ifndef FM_DJ_LFO_TOAST_LOSS
#define FM_DJ_LFO_TOAST_LOSS 8u
#endif

typedef struct {
    uint8_t lfo_tgt;
    uint16_t below_ticks;
} fm_dj_lfo_toast_hold_t;

static inline void fm_dj_lfo_toast_hold_reset(fm_dj_lfo_toast_hold_t *h)
{
    h->lfo_tgt = 0xffu;
    h->below_ticks = 0u;
}

static inline uint8_t fm_dj_lfo_toast_pick_tgt_stable(const fm_dj_lfo_ov_t *ov, const fm_dj_t *dj,
                                                      const fm_lfo_slot_t *eff, const fm_lfo_rt_t *rt,
                                                      fm_dj_lfo_toast_hold_t *h)
{
    uint8_t pick;
    int32_t pick_mag;
    int32_t held_mag;

    if (!dj->on) {
        fm_dj_lfo_toast_hold_reset(h);
        return 0xffu;
    }
    pick = fm_dj_lfo_toast_pick_tgt(ov, dj, eff, rt);
    if (h->lfo_tgt >= DUB_LFO_TGT_N) {
        if (pick < DUB_LFO_TGT_N)
            h->lfo_tgt = pick;
        return h->lfo_tgt;
    }
    held_mag = fm_dj_lfo_toast_audible(eff, rt, h->lfo_tgt) ?
                   fm_lfo_slot_delta(eff, rt, h->lfo_tgt) :
                   0;
    if (held_mag < 0)
        held_mag = -held_mag;
    if (held_mag < FM_DJ_LFO_TOAST_MIN_DELTA) {
        if (h->below_ticks < FM_DJ_LFO_TOAST_LOSS)
            h->below_ticks++;
        else {
            h->lfo_tgt = pick;
            h->below_ticks = 0u;
        }
    } else
        h->below_ticks = 0u;
    if (pick < DUB_LFO_TGT_N && pick != h->lfo_tgt) {
        pick_mag = fm_lfo_slot_delta(eff, rt, pick);
        if (pick_mag < 0)
            pick_mag = -pick_mag;
        if (pick_mag > held_mag + FM_DJ_LFO_TOAST_SWITCH_MARGIN) {
            h->lfo_tgt = pick;
            h->below_ticks = 0u;
        }
    }
    if (h->lfo_tgt < DUB_LFO_TGT_N && !fm_dj_lfo_toast_audible(eff, rt, h->lfo_tgt))
        return 0xffu;
    return h->lfo_tgt;
}

static inline int fm_dj_lfo_macro_static_bypass(const fm_dj_t *dj, const fm_dj_lfo_ov_t *ov, uint8_t macro_idx)
{
    uint8_t dj_tgt;
    uint8_t si;

    switch (macro_idx) {
    case 8u:
        dj_tgt = FM_DJ_TGT_MOTION;
        break;
    case 9u:
        dj_tgt = FM_DJ_TGT_FLUX;
        break;
    case 2u:
        dj_tgt = FM_DJ_TGT_WOB;
        break;
    case 3u:
        dj_tgt = FM_DJ_TGT_ISO;
        break;
    case 4u:
        dj_tgt = FM_DJ_TGT_SPACE;
        break;
    case 5u:
        dj_tgt = FM_DJ_TGT_WEIGHT;
        break;
    case 7u:
        dj_tgt = FM_DJ_TGT_DUB;
        break;
    default:
        return 0;
    }
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        const fm_dj_slot_t *s = &dj->slot[si];

        if (!o->active || o->dj_tgt != dj_tgt)
            continue;
        if (s->phase != FM_DJ_SLOT_OFF || s->delta != 0) {
            if (fm_dj_lfo_depth_scaled(o->plan.depth, s) > 0u)
                return 1;
        }
    }
    return 0;
}

static inline int fm_dj_lfo_tgt_overlay_live(const fm_dj_lfo_ov_t *ov, const fm_dj_t *dj, uint8_t lt)
{
    uint8_t si;

    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_lfo_slot_ov_t *o = &ov->per[si];
        const fm_dj_slot_t *s = &dj->slot[si];

        if (!o->active || o->lfo_tgt != lt)
            continue;
        if (s->phase != FM_DJ_SLOT_OFF || s->delta != 0) {
            if (fm_dj_lfo_depth_scaled(o->plan.depth, s) > 0u)
                return 1;
        }
    }
    return 0;
}

#define FM_DJ_LFO_WIRED 1

#endif /* FM_DJ_LFO_H */
