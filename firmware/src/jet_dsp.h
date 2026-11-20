/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef JET_DSP_H
#define JET_DSP_H

#include <stdint.h>
#include "jet_patch.h"
#include "jet_tables.h"
#include "bass_env.h"
#include "jet_fx.h"

#define JET_FS        44100u
#define JET_NVOICE    4u
#define JET_PIPE_LEN  2048u
#define JET_PIPE_MASK (JET_PIPE_LEN - 1u)
#define JET_SUB       32u

typedef struct {
    int16_t bend_cents;
    uint8_t breath;
    uint8_t mw;
    uint8_t at;
    uint8_t sustain;
    int16_t lfo_pitch;
} jet_expr_t;

typedef struct {
    bass_dahdsr_cfg_t env_cfg;
    uint8_t env_curve;
    uint8_t vel_sens;
    int16_t blow_q15;
    int16_t noise_mix_q15;
    int16_t push_q15;
    uint32_t inv_t_on_q24;
    uint32_t inv_t_off_q24;
    int16_t sat_makeup_q15;
    int16_t out_gain_q10;
    int16_t sat_drive_q15;
    int16_t sat_bias_q15;
    int16_t sat_shape_q15;
    int16_t harm_a_q15;
    int16_t lp_a_q15;
    int16_t hp_a_q15;
    int16_t noise_a_q15;
    uint16_t split_q15;
    int16_t pol_sign_q15;
    int16_t level_q15;
    int16_t x_breath_q15;
    int16_t x_mw_lpf_q15;
    int16_t x_at_push_q15;
    uint8_t polarity;
    uint8_t tcomp;
    uint32_t period_sub_q16;
    jet_fx_coef_t fx;
} jet_coef_t;

typedef struct {
    int16_t *pipe;
    uint32_t w;
    uint32_t period_q16;
    uint32_t cent_q16;
    uint32_t cent_tgt_q16;
    uint16_t l_int;
    uint8_t lp_open;
    uint16_t thiran_idx;
    int16_t eta_q15;
    int32_t thiran_ap;
    int32_t disp_ap0;
    int32_t disp_ap1;
    int32_t lp_z;
    int32_t hp_z;
    int32_t noise_lp;
    uint32_t rng;
    bass_dahdsr_rt_t env;
    uint8_t note;
    uint8_t gate;
    uint8_t active;
    uint8_t sus_held; /* note-off arrived while the sustain pedal was down */
    uint8_t pan_q15;
    int16_t detune_cents;
    uint16_t quiet_cnt;
    uint16_t fade;
    int16_t peak_y;
    uint32_t age;
} jet_voice_t;

typedef struct {
    jet_voice_t vo[JET_NVOICE];
    jet_fx_t fx;
    jet_patch_t patch;
    jet_coef_t coef[2];
    uint8_t coef_ix;
    jet_expr_t expr;
    uint8_t sustain_cc;
    uint8_t mode;
    int16_t detune_spread;
    uint8_t glide_coef;
    int8_t stack_note[8];
    int8_t stack_vel[8];
    uint8_t stack_n;
    uint32_t voice_tick;
} jet_engine_t;

static int16_t jet_pipe_pool[JET_NVOICE][JET_PIPE_LEN] JET_POOL_SECTION;

static inline int32_t jet_xorshift(uint32_t *st)
{
    uint32_t x = *st;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *st = x;
    return (int32_t)(x >> 16) - 32768;
}

static inline int32_t jet_exp2_neg_q15(uint32_t e_q8)
{
    uint32_t i = e_q8 >> 8;
    uint32_t f = e_q8 & 255u;
    int32_t whole;
    int32_t frac;

    if (i >= 16u)
        return 0;
    whole = 32768 >> i;
    frac = JET_EXP2_FRAC_Q15[f];
    return (int32_t)(((int64_t)whole * frac) >> 15);
}

static inline int16_t jet_fbk_q15(uint32_t period_q16, uint32_t inv_t_q24)
{
    uint32_t p = period_q16 >> 16;
    uint64_t prod;
    uint32_t e_q8;

    if (p < 8u)
        p = 8u;
    prod = (uint64_t)p * (uint64_t)inv_t_q24;
    e_q8 = (uint32_t)((prod * 9966ull) >> (24 + 2));
    if (e_q8 > (32u << 8))
        return 0;
    return (int16_t)jet_exp2_neg_q15(e_q8);
}

static inline int32_t jet_lp1(int32_t x, int32_t a, int32_t *z)
{
    *z += ((x - *z) * a) >> 15;
    return *z;
}

static inline int32_t jet_hp1(int32_t x, int32_t a, int32_t *z)
{
    *z += ((x - *z) * a) >> 15;
    return x - *z;
}

static inline int32_t jet_ap1(int32_t x, int32_t a, int32_t *s)
{
    if (a <= 0)
        return x;
    {
        int32_t y = ((int32_t)a * x >> 15) + *s;
        *s = x - ((int32_t)a * y >> 15);
        return y;
    }
}

static inline int32_t jet_thiran_read(const int16_t *pipe, uint32_t w, uint16_t l_int, int16_t eta,
                                      int32_t *ap)
{
    uint32_t r = (w - (uint32_t)l_int) & JET_PIPE_MASK;
    int32_t x0 = pipe[r];
    int32_t y = ((int32_t)eta * x0 >> 15) + *ap;
    *ap = x0 - ((int32_t)eta * y >> 15);
    return y;
}

static inline int32_t jet_tanh_mix(int32_t x, int16_t shape_q15)
{
    int32_t u = x + (4 << 15);
    int32_t idx;
    int32_t lo;
    int32_t hi;
    int32_t f;
    int32_t th;
    int32_t cl;

    if (u <= 0)
        th = -32767;
    else if (u >= (8 << 15))
        th = 32767;
    else {
        idx = u >> 8;
        if (idx > 1023)
            idx = 1023;
        f = u & 255;
        lo = JET_TANH_Q15[idx];
        hi = JET_TANH_Q15[idx + 1];
        th = lo + (((hi - lo) * f) >> 8);
    }
    cl = x;
    if (cl > 32767)
        cl = 32767;
    else if (cl < -32767)
        cl = -32767;
    return th + (((cl - th) * (int32_t)shape_q15) >> 15);
}

static inline int32_t jet_sat(int32_t x, const jet_coef_t *c)
{
    int32_t xp = ((x * (int32_t)c->sat_drive_q15) >> 12) + c->sat_bias_q15;
    int32_t y = jet_tanh_mix(xp, c->sat_shape_q15);
    int32_t bias_sh = jet_tanh_mix(c->sat_bias_q15, c->sat_shape_q15);
    return ((y - bias_sh) * (int32_t)c->sat_makeup_q15) >> 15;
}

static inline uint32_t jet_cents_from_note(uint8_t note, int8_t tune, int8_t fine)
{
    int32_t c = (int32_t)(note - 12) * 100 + (int32_t)tune * 100 + (int32_t)fine;
    if (c < 0)
        c = 0;
    if (c > 12000)
        c = 12000;
    return (uint32_t)c << 16;
}

static inline uint32_t jet_coef_period_sub_q16(int16_t lp_a, int16_t harm_a, int16_t hp_a, uint8_t tcomp)
{
    int32_t lp_d = 0;
    int32_t hp_d = 0;
    int32_t ap_d = 0;
    int32_t comp;

    if (lp_a > 0)
        lp_d = (((32767 - lp_a) << 15) / lp_a) >> 16;
    if (hp_a > 0)
        hp_d = -((32767 << 15) / (hp_a + 256)) >> 16;
    if (harm_a > 0) {
        int32_t ap1 = (((32767 - harm_a) << 15) / (32767 + harm_a)) >> 16;

        ap_d = ap1 << 1;
    }
    comp = lp_d + hp_d + ap_d;
    comp = (comp * (int32_t)tcomp) / 127;
    if (comp <= 0)
        return 0u;
    return (uint32_t)comp << 16;
}

static inline uint32_t jet_period_from_cents(uint32_t cent_q16, uint8_t polarity, uint32_t period_sub_q16)
{
    uint32_t cents = cent_q16 >> 16;
    uint32_t oct = 0u;
    uint32_t p;

    while (cents >= 1200u) {
        cents -= 1200u;
        oct++;
    }
    p = JET_LEN_OCT_Q16[cents];
    if (oct > 0u) {
        if (oct >= 16u)
            p = 8u << 16;
        else
            p >>= oct;
    }
    if (p < (8u << 16))
        p = 8u << 16;
    if (p > ((JET_PIPE_LEN - 4u) << 16))
        p = (JET_PIPE_LEN - 4u) << 16;
    if (polarity)
        p >>= 1;
    if (period_sub_q16 > 0u && period_sub_q16 < p)
        p -= period_sub_q16;
    return p;
}

static inline void jet_voice_pitch(jet_voice_t *v, const jet_coef_t *c, const jet_expr_t *ex, int16_t extra_cent)
{
    int32_t cent;
    uint32_t p;

    cent = (int32_t)(v->cent_q16 >> 16) + (int32_t)ex->lfo_pitch + extra_cent + v->detune_cents;
    if (cent < 0)
        cent = 0;
    if (cent > 12000)
        cent = 12000;
    p = jet_period_from_cents((uint32_t)cent << 16, c->polarity, c->period_sub_q16);
    v->period_q16 = p;
    if (cent > 4000 && p > (4u << 16))
        v->period_q16 -= c->polarity ? 29000u : 92446u + 26000u;
    p = v->period_q16;
    {
        uint32_t li = (p - 32768u) >> 16;

        v->lp_open = li < 48u ? (uint8_t)((48u - (li < 16u ? 16u : li)) << 3) : 0u;
        if (v->lp_open) {
            uint32_t a0 = c->lp_a_q15 > 64 ? (uint32_t)c->lp_a_q15 : 64u;
            uint32_t a1 = a0 + (((32767u - a0) * v->lp_open) >> 8);
            uint32_t d0 = ((32767u - a0) << 16) / a0;
            uint32_t d1 = ((32767u - a1) << 16) / a1;

            p += (((d0 - d1) >> 2) * 3u * (uint32_t)c->tcomp) >> 7;
            v->period_q16 = p;
        }
    }
    v->l_int = (uint16_t)((p - 32768u) >> 16);
    if (v->l_int < 8u)
        v->l_int = 8u;
    if (v->l_int > JET_PIPE_LEN - 4u)
        v->l_int = JET_PIPE_LEN - 4u;
    {
        uint32_t d_q16 = p - ((uint32_t)v->l_int << 16);
        int32_t idx = (int32_t)(d_q16 >> 8) - 128;
        if (idx < 0)
            idx = 0;
        if (idx > 256)
            idx = 256;
        v->thiran_idx = (uint16_t)idx;
        v->eta_q15 = JET_THIRAN_Q15[idx];
    }
}

static inline void jet_coef_build(jet_coef_t *c, const jet_patch_t *p, uint16_t bpm)
{
    if (!c || !p)
        return;
    if (bpm < 40u)
        bpm = 40u;
    if (bpm > 300u)
        bpm = 300u;
    c->env_cfg.t.delay = p->s_dly;
    c->env_cfg.t.attack = p->s_atk;
    c->env_cfg.t.hold = p->s_hold;
    c->env_cfg.t.decay = p->s_dec;
    c->env_cfg.t.sustain = p->s_sus;
    c->env_cfg.t.release = p->s_rel;
    c->env_cfg.pol.retrigger = 1u;
    c->env_cfg.pol.invert = 0u;
    c->env_cfg.pol.accent_scale = 64u;
    c->env_curve = p->s_curve;
    c->vel_sens = p->vel_sens;
    c->blow_q15 = (int16_t)(((int32_t)p->blow * 32767) >> 7);
    c->noise_mix_q15 = (int16_t)(((int32_t)p->dc_noise * 32767) >> 7);
    c->push_q15 = (int16_t)((int32_t)p->push * 512);
    c->inv_t_on_q24 = JET_T60_INV_Q24[p->fb_dec > 127u ? 127u : p->fb_dec];
    c->inv_t_off_q24 = JET_T60_INV_Q24[p->fb_rel > 127u ? 127u : p->fb_rel];
    c->sat_drive_q15 = (int16_t)(4096 + (((int32_t)p->sat * 12288) >> 7));
    c->sat_makeup_q15 = (int16_t)((32767 * 4096) / c->sat_drive_q15);
    c->sat_bias_q15 = (int16_t)((((int32_t)p->sat_sym - 64) * 12000) >> 6);
    c->sat_shape_q15 = (int16_t)(((int32_t)p->sat_shape * 32767) >> 7);
    c->harm_a_q15 = (int16_t)(((int32_t)p->harm * 28000) >> 7);
    c->lp_a_q15 = JET_OP_A_Q15[p->lpf > 127u ? 127u : p->lpf];
    c->hp_a_q15 = JET_OP_A_Q15[p->hpf > 127u ? 127u : p->hpf];
    c->noise_a_q15 = JET_OP_A_Q15[p->noise_lpf > 127u ? 127u : p->noise_lpf];
    c->split_q15 = p->split ? (uint16_t)(((int32_t)p->split * 32767) >> 7) : 0u;
    c->pol_sign_q15 = p->polarity ? -32767 : 32767;
    c->level_q15 = (int16_t)(((int32_t)p->level * 32767) >> 7);
    {
        int32_t pu = p->push < 0 ? 0 : (p->push > 16 ? 16 : p->push);

        c->out_gain_q10 = (int16_t)(1536 + ((16 - pu) * (16384 - 1536)) / 16);
    }
    c->x_breath_q15 = (int16_t)(((int32_t)p->x_breath_blow - 64) * 512);
    c->x_mw_lpf_q15 = (int16_t)(((int32_t)p->x_mw_lpf - 64) * 512);
    c->x_at_push_q15 = (int16_t)(((int32_t)p->x_at_push - 64) * 512);
    c->polarity = p->polarity;
    c->tcomp = p->tcomp;
    c->period_sub_q16 = jet_coef_period_sub_q16(c->lp_a_q15, c->harm_a_q15, c->hp_a_q15, c->tcomp);
    jet_fx_coef_build(&c->fx, p, bpm);
}

static inline void jet_memzero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    uint32_t i;

    for (i = 0u; i < n; i++)
        b[i] = 0u;
}

static inline void jet_engine_init(jet_engine_t *e)
{
    uint8_t i;

    if (!e)
        return;
    jet_memzero(e, (uint32_t)sizeof(*e));
    for (i = 0u; i < JET_NVOICE; i++) {
        jet_voice_t *v = &e->vo[i];
        uint32_t k;

        for (k = 0u; k < JET_PIPE_LEN; k++)
            jet_pipe_pool[i][k] = 0;
        v->pipe = jet_pipe_pool[i];
        v->w = 0u;
        v->period_q16 = 220u << 16;
        v->cent_q16 = jet_cents_from_note(60u, 0, 0);
        v->cent_tgt_q16 = v->cent_q16;
        v->l_int = 200u;
        v->thiran_idx = 128u;
        v->eta_q15 = 0;
        v->thiran_ap = 0;
        v->disp_ap0 = 0;
        v->disp_ap1 = 0;
        v->lp_z = 0;
        v->hp_z = 0;
        v->noise_lp = 0;
        v->rng = 0x12345678u + i;
        bass_dahdsr_init(&v->env);
        v->note = 0u;
        v->gate = 0u;
        v->active = 0u;
        v->pan_q15 = 0;
        v->detune_cents = 0;
        v->quiet_cnt = 0u;
        v->fade = 0u;
        v->peak_y = 0;
        v->age = 0u;
    }
    jet_fx_init(&e->fx);
    jet_patch_defaults(&e->patch);
    jet_coef_build(&e->coef[0], &e->patch, 120u);
    e->mode = e->patch.mode;
    e->glide_coef = e->patch.glide;
    e->detune_spread = e->patch.detune;
    e->coef[1] = e->coef[0];
    e->coef_ix = 0u;
    e->expr.bend_cents = 0;
    e->expr.breath = 0u;
    e->expr.mw = 0u;
    e->expr.at = 0u;
    e->expr.sustain = 127u;
    e->expr.lfo_pitch = 0;
    e->sustain_cc = 127u;
    e->mode = JET_MODE_POLY;
    e->detune_spread = 6;
    e->glide_coef = 0u;
    e->stack_n = 0u;
    e->voice_tick = 0u;
}

static inline void jet_engine_publish(jet_engine_t *e, const jet_coef_t *c)
{
    uint8_t nix;

    if (!e || !c)
        return;
    nix = e->coef_ix ^ 1u;
    e->coef[nix] = *c;
    e->coef_ix = nix;
}

static inline int32_t jet_vel_peak(uint8_t vel, uint8_t vel_sens)
{
    static const bass_dahdsr_policy_t pol = { 1u, 0u, 64u };
    int32_t full = bass_dahdsr_peak(127, 0, &pol);

    if (vel_sens == 0u)
        return full;
    return (full * (int32_t)vel) >> 7;
}

static inline void jet_voice_clear(jet_voice_t *v)
{
    uint32_t k;
    for (k = 0u; k < JET_PIPE_LEN; k++)
        v->pipe[k] = 0;
    v->thiran_ap = 0;
    v->disp_ap0 = 0;
    v->disp_ap1 = 0;
    v->lp_z = 0;
    v->hp_z = 0;
    v->noise_lp = 0;
    v->w = 0u;
}

static inline jet_voice_t *jet_alloc_voice(jet_engine_t *e)
{
    uint8_t i;
    jet_voice_t *free_v = 0;
    jet_voice_t *old = 0;

    for (i = 0u; i < JET_NVOICE; i++) {
        jet_voice_t *v = &e->vo[i];
        if (!v->active && v->env.stage == BASS_ENV_IDLE)
            free_v = v;
        if (!old || v->age > old->age)
            old = v;
    }
    if (free_v)
        return free_v;
    if (old) {
        old->fade = 64u;
        return old;
    }
    return &e->vo[0];
}

static inline void jet_voice_trigger(jet_engine_t *e, jet_voice_t *v, uint8_t note, uint8_t vel, uint8_t legato)
{
    const jet_coef_t *c = &e->coef[e->coef_ix];

    if (v->fade == 0u && !legato)
        jet_voice_clear(v);
    v->note = note;
    v->gate = 1u;
    v->active = 1u;
    v->sus_held = 0u;
    v->quiet_cnt = 0u;
    v->cent_tgt_q16 = jet_cents_from_note(note, e->patch.tune, e->patch.fine);
    if (!legato || e->glide_coef == 0u)
        v->cent_q16 = v->cent_tgt_q16;
    jet_voice_pitch(v, c, &e->expr, e->expr.bend_cents);
    if (!legato) {
        bass_dahdsr_trig(&v->env, &c->env_cfg, vel, 0u);
        v->env.peak_q15 = jet_vel_peak(vel, c->vel_sens);
    } else
        bass_dahdsr_gate(&v->env, &c->env_cfg, 1u);
    v->age = e->voice_tick++;
}

static inline void jet_engine_note_on(jet_engine_t *e, uint8_t note, uint8_t vel, uint8_t legato)
{
    uint8_t i;
    jet_voice_t *v;

    if (!e || vel == 0u)
        return;
    if (e->mode == JET_MODE_UNISON) {
        static const int8_t det[4] = { -3, -1, 1, 3 };
        int32_t spread = e->detune_spread;
        for (i = 0u; i < JET_NVOICE; i++) {
            v = &e->vo[i];
            v->detune_cents = (int16_t)((det[i] * spread) >> 2);
            v->pan_q15 = (int16_t)((i - 1) * 12000);
            jet_voice_trigger(e, v, note, vel, legato);
        }
        return;
    }
    if (e->mode == JET_MODE_MONO || e->mode == JET_MODE_LEGATO) {
        if (e->stack_n < 8u) {
            e->stack_note[e->stack_n] = (int8_t)note;
            e->stack_vel[e->stack_n] = (int8_t)vel;
            e->stack_n++;
        }
        v = jet_alloc_voice(e);
        v->detune_cents = 0;
        v->pan_q15 = 0;
        jet_voice_trigger(e, v, note, vel, legato && e->mode == JET_MODE_LEGATO);
        return;
    }
    v = jet_alloc_voice(e);
    v->detune_cents = 0;
    v->pan_q15 = 0;
    jet_voice_trigger(e, v, note, vel, 0u);
}

static inline void jet_engine_note_off(jet_engine_t *e, uint8_t note)
{
    uint8_t i;

    if (!e)
        return;
    for (i = 0u; i < JET_NVOICE; i++) {
        jet_voice_t *v = &e->vo[i];

        if (!v->active || v->note != note)
            continue;
        if (e->sustain_cc)
            v->sus_held = 1u;
        else
            bass_dahdsr_gate(&v->env, &e->coef[e->coef_ix].env_cfg, 0u);
    }
}

static inline void jet_engine_all_off(jet_engine_t *e, uint8_t hard)
{
    uint8_t i;

    if (!e)
        return;
    e->stack_n = 0u;
    for (i = 0u; i < JET_NVOICE; i++) {
        jet_voice_t *v = &e->vo[i];
        if (hard) {
            bass_dahdsr_init(&v->env);
            v->active = 0u;
            jet_voice_clear(v);
        } else
            bass_dahdsr_gate(&v->env, &e->coef[e->coef_ix].env_cfg, 0u);
    }
}

static inline void jet_engine_expr(jet_engine_t *e, const jet_expr_t *x)
{
    if (!e || !x)
        return;
    e->expr = *x;
    if (e->sustain_cc && !x->sustain) {
        uint8_t i;

        for (i = 0u; i < JET_NVOICE; i++) {
            jet_voice_t *v = &e->vo[i];

            if (v->sus_held) {
                v->sus_held = 0u;
                if (v->active)
                    bass_dahdsr_gate(&v->env, &e->coef[e->coef_ix].env_cfg, 0u);
            }
        }
    }
    e->sustain_cc = x->sustain;
}

static inline int32_t jet_voice_sample(jet_voice_t *v, const jet_coef_t *c, const jet_expr_t *ex, int16_t fbk_q15,
                                       int16_t push_eff, int16_t blow_eff)
{
    int32_t g;
    int32_t nz;
    int32_t src;
    int32_t exc;
    int32_t y;
    int32_t vloop;
    int32_t out;
    int16_t pol = c->pol_sign_q15;

    if (v->fade > 0u) {
        v->fade--;
        if (v->fade == 0u)
            jet_voice_clear(v);
        return 0;
    }
    if (v->env.stage == BASS_ENV_IDLE) {
        if (v->active) {
            jet_voice_clear(v);
            v->active = 0u;
        }
        return 0;
    }
    g = bass_dahdsr_step_shaped(&v->env, &c->env_cfg, c->env_curve);
    g = (int32_t)(((int64_t)g * (int64_t)blow_eff) >> 7);
    nz = jet_xorshift(&v->rng);
    nz = jet_lp1(nz, c->noise_a_q15, &v->noise_lp);
    src = ((32767 - c->noise_mix_q15) * 8192 >> 15) + ((nz * c->noise_mix_q15) >> 15);
    exc = (int32_t)(((int64_t)g * src) >> 15);
    y = jet_thiran_read(v->pipe, v->w, v->l_int, v->eta_q15, &v->thiran_ap);
    if (y > 32767)
        y = 32767;
    else if (y < -32767)
        y = -32767;
    if (c->split_q15 > 0) {
        uint32_t l2 = ((uint32_t)v->l_int * c->split_q15) >> 15;
        if (l2 < 4u)
            l2 = 4u;
        y = (y + v->pipe[(v->w - l2) & JET_PIPE_MASK]) >> 1;
    }
    exc += (int32_t)(((int64_t)g * jet_tanh_mix((int32_t)(((int64_t)y * push_eff) >> 15), 0)) >> 15);
    vloop = exc + (int32_t)(((int64_t)y * fbk_q15) >> 15);
    vloop = (vloop * pol) >> 15;
    vloop = jet_ap1(jet_ap1(vloop, c->harm_a_q15, &v->disp_ap0), c->harm_a_q15, &v->disp_ap1);
    vloop = jet_lp1(vloop, c->lp_a_q15 + (((32767 - c->lp_a_q15) * (int32_t)v->lp_open) >> 8), &v->lp_z);
    vloop = jet_hp1(vloop, c->hp_a_q15, &v->hp_z);
    vloop = jet_sat(vloop, c);
    if (vloop > 32767)
        vloop = 32767;
    else if (vloop < -32767)
        vloop = -32767;
    v->pipe[v->w] = (int16_t)vloop;
    v->w = (v->w + 1u) & JET_PIPE_MASK;
    out = y;
    {
        int32_t ay = out >= 0 ? out : -out;
        if (ay > v->peak_y)
            v->peak_y = (int16_t)(ay > 32767 ? 32767 : ay);
    }
    if (v->env.stage == BASS_ENV_IDLE) {
        v->gate = 0u;
        if (v->peak_y < 8) {
            if (v->quiet_cnt < 2048u)
                v->quiet_cnt++;
        } else {
            v->quiet_cnt = 0u;
            v->peak_y = (int16_t)(v->peak_y - (v->peak_y >> 8) - 1);
        }
    }
    return (int32_t)(((((int64_t)out * c->level_q15) >> 15) * c->out_gain_q10) >> 10);
}

static inline void jet_glide_cent(jet_voice_t *v, uint8_t glide)
{
    if (glide == 0u) {
        v->cent_q16 = v->cent_tgt_q16;
        return;
    }
    {
        int32_t d = (int32_t)(v->cent_tgt_q16 >> 16) - (int32_t)(v->cent_q16 >> 16);
        int32_t step = d >> (4u + (glide >> 4));
        if (step == 0 && d != 0)
            step = d > 0 ? 1 : -1;
        v->cent_q16 = (uint32_t)((int32_t)(v->cent_q16 >> 16) + step) << 16;
    }
}

static inline void jet_engine_render(jet_engine_t *e, int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i;
    uint32_t sub;
    const jet_coef_t *c;

    if (!e || !l || !r || n == 0u)
        return;
    c = &e->coef[e->coef_ix];

    for (sub = 0u; sub < n; sub += JET_SUB) {
        uint32_t block = n - sub;
        int32_t dry_l[JET_SUB];
        int32_t dry_r[JET_SUB];
        uint8_t vi;

        if (block > JET_SUB)
            block = JET_SUB;

        for (i = 0u; i < block; i++) {
            dry_l[i] = 0;
            dry_r[i] = 0;
        }

        for (vi = 0u; vi < JET_NVOICE; vi++) {
            jet_voice_t *v = &e->vo[vi];
            int16_t fbk;
            int16_t push_eff;
            int16_t blow_eff;
            int32_t blow_u7;
            int32_t push_u7;

            if (!v->active)
                continue;
            jet_glide_cent(v, e->glide_coef);
            jet_voice_pitch(v, c, &e->expr, e->expr.bend_cents);
            fbk = jet_fbk_q15(v->period_q16, v->env.gate_open ? c->inv_t_on_q24 : c->inv_t_off_q24);
            push_u7 = (int32_t)e->patch.push +
                      (((int32_t)e->expr.at * ((int32_t)e->patch.x_at_push - 64)) >> 7);
            if (push_u7 > 63)
                push_u7 = 63;
            else if (push_u7 < -64)
                push_u7 = -64;
            push_eff = (int16_t)(push_u7 * 512);
            blow_u7 = (int32_t)e->patch.blow +
                      (((int32_t)e->expr.breath * ((int32_t)e->patch.x_breath_blow - 64)) >> 6);
            if (blow_u7 > 127)
                blow_u7 = 127;
            else if (blow_u7 < 0)
                blow_u7 = 0;
            blow_eff = (int16_t)blow_u7;
            for (i = 0u; i < block; i++) {
                int32_t s = jet_voice_sample(v, c, &e->expr, fbk, push_eff, blow_eff);
                int32_t pan = v->pan_q15;
                int64_t acc_l;
                int64_t acc_r;

                acc_l = (int64_t)dry_l[i] + (int64_t)s - (((int64_t)s * pan) >> 15);
                acc_r = (int64_t)dry_r[i] + (int64_t)s + (((int64_t)s * pan) >> 15);
                if (acc_l > 32767)
                    acc_l = 32767;
                else if (acc_l < -32767)
                    acc_l = -32767;
                if (acc_r > 32767)
                    acc_r = 32767;
                else if (acc_r < -32767)
                    acc_r = -32767;
                dry_l[i] = (int32_t)acc_l;
                dry_r[i] = (int32_t)acc_r;
            }
        }

        for (i = 0u; i < block; i++) {
            if (dry_l[i] > 32767)
                dry_l[i] = 32767;
            else if (dry_l[i] < -32767)
                dry_l[i] = -32767;
            if (dry_r[i] > 32767)
                dry_r[i] = 32767;
            else if (dry_r[i] < -32767)
                dry_r[i] = -32767;
        }

        jet_fx_render(&e->fx, &c->fx, dry_l, dry_r, block);

        for (i = 0u; i < block; i++) {
            l[sub + i] = dry_l[i];
            r[sub + i] = dry_r[i];
        }
    }
}

#endif /* JET_DSP_H */
