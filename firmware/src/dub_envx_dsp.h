/* SPDX-License-Identifier: GPL-3.0-only */
/* KO KIT ENV for the genre voices: run once per sample before the voices render (dub_synth_render).
 * The legacy step functions keep running; this layer adjusts the lane env / phase state around them:
 * DELAY parks the trigger, HOLD pins the peak, CURVE bends the decay step (fm_env_decay_step_mul, as KO KIT /
 * KO BASS), SUST is the decay→release breakpoint (KO KIT law), RETRIG off ignores triggers while sounding,
 * INVERT runs an own true level (peak − env), SWEEP/P.DEC add a decaying pitch offset on the lane phases. */
#ifndef DUB_ENVX_DSP_H
#define DUB_ENVX_DSP_H

#include "fm_env_plot.h"
#include "bass_env.h" /* DELAY/HOLD time law + pow2 shared with KO BASS */

typedef struct {
    int32_t *env;
    uint16_t *snap;
    int32_t *peak;
    uint16_t *atk;
    uint32_t *ph[DUB_CHORD_N];
    uint8_t nph;
} dub_envx_lane_t;

static inline void dub_envx_lane(dub_synth_t *s, uint8_t ln, dub_envx_lane_t *L)
{
    uint32_t i;

    L->nph = 1u;
    switch (ln) {
    case 0:
        L->env = &s->kick_env, L->snap = &s->kick_snap, L->peak = &s->kick_peak, L->atk = &s->kick_atk_left;
        L->ph[0] = &s->kick_ph;
        break;
    case 1:
        L->env = &s->rim_env, L->snap = &s->rim_snap, L->peak = &s->rim_peak, L->atk = &s->rim_atk_left;
        L->ph[0] = &s->rim_ph;
        break;
    case 2:
        L->env = &s->hat_env, L->snap = &s->hat_snap, L->peak = &s->hat_peak, L->atk = &s->hat_atk_left;
        for (i = 0u; i < 4u; i++)
            L->ph[i] = &s->hat_ph[i];
        L->nph = 4u;
        break;
    case 3:
        L->env = &s->siren_env, L->snap = 0, L->peak = &s->siren_peak, L->atk = &s->siren_atk_left;
        L->ph[0] = &s->siren_ph;
        break;
    case 4:
        L->env = &s->sub_env, L->snap = &s->sub_snap, L->peak = &s->sub_peak, L->atk = &s->sub_atk_left;
        L->ph[0] = &s->sub_ph;
        L->ph[1] = &s->sub_ph2;
        L->ph[2] = &s->sub_ph3;
        L->nph = 3u;
        break;
    default:
        L->env = &s->ch_env, L->snap = &s->ch_snap, L->peak = &s->ch_peak, L->atk = &s->ch_atk_left;
        for (i = 0u; i < DUB_CHORD_N; i++)
            L->ph[i] = &s->ch_ph[i];
        L->nph = (uint8_t)DUB_CHORD_N;
        break;
    }
}

/* INVERT true-level decay: the lane's legacy body law without the snap transient. */
static inline uint32_t dub_envx_inv_mul(const dub_synth_t *s, uint8_t ln)
{
    static const uint32_t frac_q16[4] = {65536u, 55109u, 46341u, 38968u};
    uint32_t q4;

    switch (ln) {
    case 0:
    case 1:
        q4 = ln == 0u ? s->kick_body : s->rim_body;
        return (uint32_t)(((uint64_t)589824u * frac_q16[q4 & 3u]) >> (16u + (q4 >> 2)));
    case 2:
        q4 = s->hat_body;
        return (uint32_t)((4947u * frac_q16[q4 & 3u]) >> (16u + (q4 >> 2)));
    case 4:
        return s->sub_body < 20u ? 589824u >> s->sub_body : 1u;
    case 5:
        return s->ch_body < 20u ? 589824u >> s->ch_body : 1u;
    default:
        return 589824u >> 10;
    }
}

static inline void dub_envx_ph_sync(dub_envx_rt_t *rt, const dub_envx_lane_t *L)
{
    uint32_t i;

    for (i = 0u; i < L->nph; i++)
        rt->ph_prev[i] = *L->ph[i];
}

/* 2^(q8/12/256 oct) − 1 in Q16 for semitones in Q8 (±24 st). */
static inline int32_t dub_envx_pitch_q16(int32_t st_q8)
{
    int32_t e = st_q8 / 12;
    uint32_t pw;

    if (e >= 0) {
        if (e > 2047)
            e = 2047;
        return (int32_t)bass_dahdsr_pow2_q16((uint32_t)e) - 65536;
    }
    e = -e;
    if (e > 2047)
        e = 2047;
    pw = bass_dahdsr_pow2_q16((uint32_t)e);
    return (int32_t)(0xFFFFFFFFu / pw) - 65536;
}

/* Decay/release step with CURVE + SUST breakpoint (KO KIT deck_env_decay law). */
static inline int32_t dub_envx_shape(const dub_envx_v_t *c, int32_t prev, int32_t d, int32_t pk)
{
    int32_t floor = c->sus ? (pk * (int32_t)c->sus) >> 7 : 0;
    int32_t nv;
    int32_t prog;
    int32_t span;

    if (c->sus && prev <= floor) {
        uint32_t body = 6u + ((uint32_t)c->rel * 6u) / 127u;
        int32_t st = dub_env_decr(prev, 589824u >> body);

        if (c->curve) {
            span = floor > 0 ? floor : 1;
            prog = ((floor - prev) * 128) / span;
            prog = prog < 0 ? 0 : prog > 128 ? 128 : prog;
            st = (st * fm_env_decay_step_mul((uint8_t)prog, dub_envx_u8(c->curve), 1u)) >> 7;
        }
        if (st < 1)
            st = 1;
        nv = prev - st;
    } else {
        if (c->curve) {
            span = pk - floor;
            if (span < 1)
                span = 1;
            prog = 128 - ((prev - floor) * 128) / span;
            prog = prog < 0 ? 0 : prog > 128 ? 128 : prog;
            d = (d * fm_env_decay_step_mul((uint8_t)prog, dub_envx_u8(c->curve), 0u)) >> 7;
            if (d < 1)
                d = 1;
        }
        nv = prev - d;
        if (c->sus && nv < floor)
            nv = floor;
    }
    return nv > 0 ? nv : 0;
}

static inline void dub_envx_lane_tick(dub_synth_t *s, uint8_t ln)
{
    const dub_envx_v_t *c = &s->envx[ln];
    dub_envx_rt_t *rt = &s->envx_rt[ln];
    dub_envx_lane_t L;
    int32_t cur;
    int32_t pk;
    uint16_t atk;
    uint16_t snap;
    int trig;
    uint32_t i;

    dub_envx_lane(s, ln, &L);
    cur = *L.env;
    atk = *L.atk;
    snap = L.snap ? *L.snap : 0u;
    if (rt->dly_left) {
        rt->dly_left--;
        if (rt->dly_left) {
            *L.env = 0;
            *L.atk = 0u;
            if (L.snap)
                *L.snap = 0u;
            rt->set = 0;
            rt->prev_atk = 0u;
            rt->prev_snap = 0u;
            return;
        }
        *L.env = cur = rt->pend_env;
        *L.atk = atk = rt->pend_atk;
        if (L.snap)
            *L.snap = snap = rt->pend_snap;
        rt->set = cur;
        rt->prev_atk = atk;
        rt->prev_snap = snap;
        rt->tru = rt->pk;
        return;
    }
    pk = *L.peak > 0 ? *L.peak : cur;
    trig = atk > rt->prev_atk || (L.snap && snap > rt->prev_snap + 1u) ||
           (rt->prev_atk == 0u && cur > rt->set + 256 && cur >= pk - (pk >> 3));
    if (dub_envx_neutral(c)) {
        rt->set = cur;
        rt->prev_atk = atk;
        rt->prev_snap = snap;
        rt->active = cur > 0;
        rt->pitch_q8 = 0;
        rt->dly_left = 0u;
        rt->hold_left = 0u;
        return;
    }
    if (trig) {
        if (c->noretrig && rt->active && (c->inv ? rt->tru : rt->set) > (rt->pk >> 4)) {
            *L.env = cur = rt->set;
            *L.atk = atk = 0u;
            if (L.snap)
                *L.snap = snap = 0u;
        } else {
            rt->active = 1u;
            rt->pk = pk > 0 ? pk : 1;
            rt->tru = rt->pk;
            rt->hold_left = bass_dahdsr_1s_samples(c->hold);
            rt->pitch_q8 = c->sweep ? ((int32_t)c->sweep * (24 * 256)) / 63 : 0;
            rt->pf_cnt = 0u;
            dub_envx_ph_sync(rt, &L);
            if (c->dly) {
                rt->dly_left = bass_dahdsr_1s_samples(c->dly);
                rt->pend_env = cur;
                rt->pend_atk = atk;
                rt->pend_snap = snap;
                *L.env = 0;
                *L.atk = 0u;
                if (L.snap)
                    *L.snap = 0u;
                rt->set = 0;
                rt->prev_atk = 0u;
                rt->prev_snap = 0u;
                return;
            }
        }
    }

    if (c->inv) {
        if (rt->active) {
            int32_t d;

            *L.atk = atk = 0u;
            if (rt->hold_left)
                rt->hold_left--;
            else {
                d = dub_env_decr(rt->tru, dub_envx_inv_mul(s, ln));
                rt->tru = dub_envx_shape(c, rt->tru, d > 0 ? d : 1, rt->pk);
            }
            if (rt->tru <= 64) {
                rt->active = 0u;
                rt->tru = 0;
                cur = 0;
            } else
                cur = rt->pk - rt->tru;
            *L.env = cur;
        }
    } else if (atk == 0u) {
        if (rt->hold_left) {
            rt->hold_left--;
            if (cur < rt->pk)
                cur = rt->pk;
        } else if (rt->set > cur && (c->curve || c->sus))
            cur = dub_envx_shape(c, rt->set, rt->set - cur, rt->pk);
        *L.env = cur;
        if (cur <= 0)
            rt->active = 0u;
    }

    if (rt->pitch_q8) {
        int32_t sh = 14 - ((int32_t)dub_envx_u8(c->pdec) * 9) / 127;

        if ((rt->pf_cnt++ & 15u) == 0u)
            rt->pf_q16 = dub_envx_pitch_q16(rt->pitch_q8);
        for (i = 0u; i < L.nph; i++) {
            uint32_t dph = *L.ph[i] - rt->ph_prev[i];

            if (dph < 0x40000000u)
                *L.ph[i] += (uint32_t)(int32_t)(((int64_t)(int32_t)dph * rt->pf_q16) >> 16);
            rt->ph_prev[i] = *L.ph[i];
        }
        rt->pitch_q8 -= rt->pitch_q8 >> (sh < 4 ? 4 : sh);
        if (rt->pitch_q8 > -4 && rt->pitch_q8 < 4)
            rt->pitch_q8 = 0;
    }
    rt->set = cur;
    rt->prev_atk = atk;
    rt->prev_snap = L.snap ? *L.snap : 0u;
}

static inline void dub_envx_tick(dub_synth_t *s)
{
    uint8_t ln;

    for (ln = 0u; ln < DUB_ENVX_LANES; ln++)
        dub_envx_lane_tick(s, ln);
}

#endif /* DUB_ENVX_DSP_H */
