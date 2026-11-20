/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_ENV_H
#define BASS_ENV_H

#include <stdint.h>
#include "fm_env_plot.h"

enum {
    BASS_ENV_DELAY = 0,
    BASS_ENV_ATTACK,
    BASS_ENV_HOLD,
    BASS_ENV_DECAY,
    BASS_ENV_SUSTAIN,
    BASS_ENV_RELEASE,
    BASS_ENV_IDLE
};

typedef struct {
    uint8_t delay;
    uint8_t attack;
    uint8_t hold;
    uint8_t decay;
    uint8_t sustain;
    uint8_t release;
} bass_dahdsr_times_t;

typedef struct {
    uint8_t retrigger;
    uint8_t invert;
    uint8_t accent_scale;
} bass_dahdsr_policy_t;

typedef struct {
    bass_dahdsr_times_t t;
    bass_dahdsr_policy_t pol;
} bass_dahdsr_cfg_t;

typedef struct {
    uint8_t stage;
    uint32_t stage_left; /* DECAY/RELEASE: nominal samples left (playhead only) */
    int32_t level_q15;
    uint8_t gate_open;
    uint8_t hard_pass;
    int32_t peak_q15;
    int32_t lvl_q31;  /* DECAY/RELEASE accumulator; resynced when level_q15 is written externally */
    uint32_t k_q31;   /* per-sample one-pole coefficient, cached for (k_stage, k_v) */
    uint8_t k_stage;
    uint8_t k_v;
    int32_t rel_q15; /* level at RELEASE entry: CURVE progress reference */
} bass_dahdsr_rt_t;

/* KIT CURVE (fm_env_plot.h laws): 64 = the plain laws below (linear attack, one-pole tails). */
#define BASS_DAHDSR_CURVE_LIN 64u

/* Persisted CURVE byte: bit 7 = set, bits 0..6 = curve; 0 (blob saved without CURVE) = LIN. */
static inline uint8_t bass_env_shape_curve(uint8_t shape)
{
    return (shape & 0x80u) ? (uint8_t)(shape & 0x7Fu) : (uint8_t)BASS_DAHDSR_CURVE_LIN;
}

static inline uint8_t bass_env_shape_pack(uint8_t curve)
{
    return (uint8_t)(0x80u | (curve > 127u ? 127u : curve));
}

/* Q15 floor: −60 dBFS. DECAY → SUSTAIN within this distance, RELEASE → IDLE below it. */
#define BASS_DAHDSR_TOL_Q15 33

/* 2^(e/256) in Q16 for e < 3072: 1 + 0.6565·f + 0.3435·f², exact at both ends */
static inline uint32_t bass_dahdsr_pow2_q16(uint32_t e_q8)
{
    uint32_t fr = e_q8 & 255u;

    return (65536u + ((fr * (43025u + ((fr * 22511u) >> 8))) >> 8)) << (e_q8 >> 8);
}

/* Delay/attack/hold in samples, exponential: 44100·(2^(10·v/127) − 1)/1023.
 * v 1 ≈ 2 smp, 16 ≈ 1.4 ms, 64 ≈ 31 ms, 96 ≈ 0.18 s, 127 = 1 s; 0 = none. */
static inline uint16_t bass_dahdsr_1s_samples(uint8_t v)
{
    uint32_t p, n;

    if (v == 0u)
        return 0u;
    if (v > 127u)
        v = 127u;
    p = bass_dahdsr_pow2_q16(((uint32_t)v * 2560u) / 127u);
    n = (uint32_t)(((uint64_t)(p - 65536u) * 44100u) / (1023u * 65536u));
    return (uint16_t)(n ? n : 1u);
}

static inline uint16_t bass_dahdsr_atk_samples(uint8_t v)
{
    return bass_dahdsr_1s_samples(v);
}

/* Decay/release time to −60 dB in samples: 2 ms · ratio^(v/127), log2(ratio) given in Q8.
 * Decay ratio 2000 (2 ms … 4 s), release ratio 3000 (2 ms … 6 s). */
#define BASS_DAHDSR_DEC_L2_Q8 2807u
#define BASS_DAHDSR_REL_L2_Q8 2957u

static inline uint32_t bass_dahdsr_tail_samples(uint8_t v, uint32_t l2_q8)
{
    if (v > 127u)
        v = 127u;
    return (uint32_t)(((uint64_t)bass_dahdsr_pow2_q16(((uint32_t)v * l2_q8) / 127u) * 441u) / (5u * 65536u));
}

static inline uint32_t bass_dahdsr_decay_samples(uint8_t v)
{
    return bass_dahdsr_tail_samples(v, BASS_DAHDSR_DEC_L2_Q8);
}

static inline uint32_t bass_dahdsr_release_samples(uint8_t v)
{
    return bass_dahdsr_tail_samples(v, BASS_DAHDSR_REL_L2_Q8);
}

/* Nominal length of a stage in samples (UI playhead; SUSTAIN/IDLE = 0). */
static inline uint32_t bass_dahdsr_stage_samples(uint8_t stage, const bass_dahdsr_cfg_t *cfg)
{
    switch (stage) {
    case BASS_ENV_DELAY:
        return bass_dahdsr_1s_samples(cfg->t.delay);
    case BASS_ENV_ATTACK:
        return bass_dahdsr_atk_samples(cfg->t.attack);
    case BASS_ENV_HOLD:
        return bass_dahdsr_1s_samples(cfg->t.hold);
    case BASS_ENV_DECAY:
        return bass_dahdsr_decay_samples(cfg->t.decay);
    case BASS_ENV_RELEASE:
        return bass_dahdsr_release_samples(cfg->t.release);
    default:
        return 0u;
    }
}

/* One-pole step toward tgt so that a full-scale distance shrinks by 60 dB in the stage time:
 * k = 1 − e^(−ln 1000 / N) ≈ x − x²/2, x = ln 1000 / N (N ≥ 88 → x ≤ 0.079). */
static inline uint32_t bass_dahdsr_tail_k(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg)
{
    uint8_t v = rt->stage == BASS_ENV_DECAY ? cfg->t.decay : cfg->t.release;
    uint64_t x;

    if (rt->k_stage != rt->stage || rt->k_v != v) {
        x = 14834291506ull / bass_dahdsr_stage_samples(rt->stage, cfg);
        rt->k_q31 = (uint32_t)(x - ((x * x) >> 32));
        rt->k_stage = rt->stage;
        rt->k_v = v;
    }
    return rt->k_q31;
}

/* CURVE ≠ LIN: one-pole k scaled by fm_env_decay_step_mul at the stage progress (KIT deck_env_decay law). */
static inline uint32_t bass_dahdsr_tail_k_shaped(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, int32_t tgt_q15,
                                                 uint8_t curve)
{
    uint32_t k = bass_dahdsr_tail_k(rt, cfg);
    uint8_t rel = rt->stage == BASS_ENV_RELEASE ? 1u : 0u;
    int32_t from = rel ? (rt->rel_q15 > 0 ? rt->rel_q15 : rt->peak_q15) : rt->peak_q15;
    int32_t span = from - tgt_q15;
    int32_t prog;
    uint64_t ks;

    if (curve == BASS_DAHDSR_CURVE_LIN)
        return k;
    if (span < 1)
        span = 1;
    prog = 128 - ((rt->level_q15 - tgt_q15) * 128) / span;
    if (prog < 0)
        prog = 0;
    if (prog > 128)
        prog = 128;
    ks = ((uint64_t)k * (uint32_t)fm_env_decay_step_mul((uint8_t)prog, curve, rel)) >> 7;
    return ks > 0x7FFFFFFFull ? 0x7FFFFFFFu : (uint32_t)ks;
}

/* Advance lvl_q31 toward tgt_q15; returns 1 once within BASS_DAHDSR_TOL_Q15. */
static inline int bass_dahdsr_tail_step(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, int32_t tgt_q15,
                                        uint8_t curve)
{
    int32_t tgt = tgt_q15 << 16;
    int32_t d;

    if ((rt->lvl_q31 >> 16) != rt->level_q15)
        rt->lvl_q31 = rt->level_q15 << 16;
    d = tgt - rt->lvl_q31;
    if (d <= (BASS_DAHDSR_TOL_Q15 << 16) && d >= -(BASS_DAHDSR_TOL_Q15 << 16))
        return 1;
    /* floor of the negative product still moves ≥ 1 LSB down → release can't stall */
    rt->lvl_q31 += (int32_t)(((int64_t)d * (int64_t)bass_dahdsr_tail_k_shaped(rt, cfg, tgt_q15, curve)) >> 31);
    rt->level_q15 = rt->lvl_q31 >> 16;
    return 0;
}

static inline int32_t bass_dahdsr_peak(uint8_t vel, uint8_t accent, const bass_dahdsr_policy_t *pol)
{
    int32_t pk = (int32_t)vel * 258u;
    int32_t asc = pol ? (int32_t)pol->accent_scale : 64;

    if (accent) /* 127 = full accent; lower values scale the boost (step ACCENT 33/66) */
        pk += (((pk * asc) >> 7) * (accent > 127u ? 127 : (int32_t)accent)) / 127;
    if (pk > 32767)
        pk = 32767;
    if (pk < 1)
        pk = 1;
    return pk;
}

static inline void bass_dahdsr_init(bass_dahdsr_rt_t *rt)
{
    if (!rt)
        return;
    rt->stage = BASS_ENV_IDLE;
    rt->stage_left = 0u;
    rt->level_q15 = 0;
    rt->gate_open = 0u;
    rt->hard_pass = 0u;
    rt->peak_q15 = 0;
    rt->lvl_q31 = 0;
    rt->k_q31 = 0u;
    rt->k_stage = 0xFFu;
    rt->k_v = 0u;
    rt->rel_q15 = 0;
}

static inline void bass_dahdsr_trig(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, uint8_t vel,
                                  uint8_t accent)
{
    if (!rt || !cfg)
        return;
    if (rt->gate_open && !cfg->pol.retrigger && rt->stage != BASS_ENV_IDLE && rt->stage != BASS_ENV_RELEASE)
        return;
    rt->gate_open = 1u;
    rt->hard_pass = 1u;
    rt->peak_q15 = bass_dahdsr_peak(vel, accent, &cfg->pol);
    rt->level_q15 = 0;
    if (cfg->t.delay > 0u) {
        rt->stage = BASS_ENV_DELAY;
        rt->stage_left = bass_dahdsr_1s_samples(cfg->t.delay);
    } else if (cfg->t.attack > 0u) {
        rt->stage = BASS_ENV_ATTACK;
        rt->stage_left = bass_dahdsr_atk_samples(cfg->t.attack);
    } else {
        rt->level_q15 = rt->peak_q15;
        rt->stage = BASS_ENV_HOLD;
        rt->stage_left = bass_dahdsr_1s_samples(cfg->t.hold);
    }
}

/* open=1 retriggers gate hold; open=0 enters release */
static inline void bass_dahdsr_gate(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, uint8_t open)
{
    if (!rt || !cfg)
        return;
    if (open) {
        rt->gate_open = 1u;
        if (rt->stage == BASS_ENV_IDLE)
            rt->hard_pass = 1u;
        return;
    }
    rt->gate_open = 0u;
    if (rt->stage == BASS_ENV_IDLE) {
        rt->hard_pass = 0u;
        return;
    }
    if (rt->stage != BASS_ENV_RELEASE) {
        rt->stage = BASS_ENV_RELEASE;
        rt->stage_left = bass_dahdsr_release_samples(cfg->t.release);
        rt->rel_q15 = rt->level_q15;
    }
}

static inline int32_t bass_dahdsr_mod_q15(const bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg)
{
    if (!rt || !cfg)
        return 0;
    if (cfg->pol.invert)
        return 32767 - rt->level_q15;
    return rt->level_q15;
}

/* Attack level at stage progress for CURVE ≠ LIN (fm_env_attack_level, Q8-interpolated). */
static inline int32_t bass_dahdsr_attack_shaped(const bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg,
                                                uint8_t curve)
{
    uint32_t total = bass_dahdsr_atk_samples(cfg->t.attack);
    uint32_t done;
    uint32_t p8;
    uint8_t pi;
    int32_t lo;
    int32_t hi;

    if (total < 1u)
        total = 1u;
    done = rt->stage_left < total ? total - rt->stage_left : 0u;
    p8 = (done << 15) / total;
    if (p8 > (128u << 8))
        p8 = 128u << 8;
    pi = (uint8_t)(p8 >> 8);
    lo = fm_env_attack_level(0, rt->peak_q15, pi, curve);
    hi = pi < 128u ? fm_env_attack_level(0, rt->peak_q15, (uint8_t)(pi + 1u), curve) : lo;
    return lo + (((hi - lo) * (int32_t)(p8 & 255u)) >> 8);
}

static inline int32_t bass_dahdsr_step_shaped(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, uint8_t curve)
{
    int32_t target;
    int32_t step;

    if (!rt || !cfg)
        return 0;
    if (rt->stage == BASS_ENV_IDLE) {
        rt->level_q15 = 0;
        rt->hard_pass = 0u;
        return 0;
    }
    if (rt->stage_left > 0u)
        rt->stage_left--;
    switch (rt->stage) {
    case BASS_ENV_DELAY:
        rt->level_q15 = 0;
        if (rt->stage_left == 0u) {
            if (cfg->t.attack > 0u) {
                rt->stage = BASS_ENV_ATTACK;
                rt->stage_left = bass_dahdsr_atk_samples(cfg->t.attack);
            } else {
                rt->level_q15 = rt->peak_q15;
                rt->stage = BASS_ENV_HOLD;
                rt->stage_left = bass_dahdsr_1s_samples(cfg->t.hold);
            }
        }
        break;
    case BASS_ENV_ATTACK:
        /* linear ramp: reach peak when the attack time runs out */
        target = rt->peak_q15;
        if (curve != BASS_DAHDSR_CURVE_LIN) {
            if (rt->stage_left == 0u) {
                rt->level_q15 = target;
                rt->stage = BASS_ENV_HOLD;
                rt->stage_left = bass_dahdsr_1s_samples(cfg->t.hold);
            } else
                rt->level_q15 = bass_dahdsr_attack_shaped(rt, cfg, curve);
            break;
        }
        step = (target - rt->level_q15) / ((int32_t)rt->stage_left + 1);
        if (step < 1)
            step = 1;
        if (rt->stage_left == 0u || rt->level_q15 + step >= target) {
            rt->level_q15 = target;
            rt->stage = BASS_ENV_HOLD;
            rt->stage_left = bass_dahdsr_1s_samples(cfg->t.hold);
        } else
            rt->level_q15 += step;
        break;
    case BASS_ENV_HOLD:
        rt->level_q15 = rt->peak_q15;
        if (rt->stage_left == 0u) {
            rt->stage = BASS_ENV_DECAY;
            rt->stage_left = bass_dahdsr_decay_samples(cfg->t.decay);
        }
        break;
    case BASS_ENV_DECAY:
        target = ((int32_t)cfg->t.sustain * rt->peak_q15) >> 7;
        if (bass_dahdsr_tail_step(rt, cfg, target, curve)) {
            rt->level_q15 = target;
            rt->stage = BASS_ENV_SUSTAIN;
        }
        break;
    case BASS_ENV_SUSTAIN:
        rt->level_q15 = ((int32_t)cfg->t.sustain * rt->peak_q15) >> 7;
        if (!rt->gate_open) {
            rt->stage = BASS_ENV_RELEASE;
            rt->stage_left = bass_dahdsr_release_samples(cfg->t.release);
            rt->rel_q15 = rt->level_q15;
        }
        break;
    case BASS_ENV_RELEASE:
        if (bass_dahdsr_tail_step(rt, cfg, 0, curve)) {
            rt->level_q15 = 0;
            rt->stage = BASS_ENV_IDLE;
            rt->stage_left = 0u;
            rt->hard_pass = 0u;
        }
        break;
    default:
        rt->stage = BASS_ENV_IDLE;
        rt->level_q15 = 0;
        rt->hard_pass = 0u;
        break;
    }
    return bass_dahdsr_mod_q15(rt, cfg);
}

static inline int32_t bass_dahdsr_step(bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg)
{
    return bass_dahdsr_step_shaped(rt, cfg, BASS_DAHDSR_CURVE_LIN);
}

static inline int32_t bass_hard_vca(int32_t sig, const bass_dahdsr_rt_t *rt)
{
    if (!rt || !rt->hard_pass)
        return 0;
    return sig;
}

static inline void bass_dahdsr_defaults(bass_dahdsr_cfg_t *cfg)
{
    if (!cfg)
        return;
    cfg->t.delay = 0u;
    cfg->t.attack = 4u;
    cfg->t.hold = 0u;
    cfg->t.decay = 80u;   /* ≈ 240 ms to sustain */
    cfg->t.sustain = 96u;
    cfg->t.release = 40u; /* ≈ 25 ms: tight acid gate, no click */
    cfg->pol.retrigger = 1u;
    cfg->pol.invert = 0u;
    cfg->pol.accent_scale = 64u;
}

#endif
