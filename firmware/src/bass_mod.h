/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_MOD_H
#define BASS_MOD_H

#include <stdint.h>

enum {
    BASS_MOD_NONE = 0,
    BASS_MOD_AMP,
    BASS_MOD_CUTOFF,
    BASS_MOD_RESO,
    BASS_MOD_DRIVE,
    BASS_MOD_DELAY,
    BASS_MOD_REV,
    BASS_MOD_GLIDE,
    BASS_MOD_MODEL,
    BASS_MOD_ENV_DELAY,
    BASS_MOD_ENV_ATTACK,
    BASS_MOD_ENV_HOLD,
    BASS_MOD_ENV_DECAY,
    BASS_MOD_ENV_SUSTAIN,
    BASS_MOD_ENV_RELEASE,
    BASS_MOD_ENV_RETRIG,
    BASS_MOD_ENV_INVERT,
    BASS_MOD_ENV_ACCENT,
    BASS_MOD_SEQ_GATE,
    BASS_MOD_SEQ_RATCHET,
    BASS_MOD_SEQ_ROTATE,
    BASS_MOD_SEQ_DENSITY,
    BASS_MOD_M1_DEPTH,
    BASS_MOD_M2_DEPTH,
    BASS_MOD_M3_DEPTH,
    BASS_MOD_M4_DEPTH,
    BASS_MOD_N
};

#define BASS_MOD_SLOTS_PER_MAP 8u
#define BASS_MOD_MACRO_N       4u

typedef struct {
    uint8_t dest;
    int8_t amount;
    uint8_t curve;
} bass_mod_slot_t;

typedef struct {
    bass_mod_slot_t macro[BASS_MOD_MACRO_N][BASS_MOD_SLOTS_PER_MAP];
    bass_mod_slot_t env[BASS_MOD_SLOTS_PER_MAP];
} bass_mod_maps_t;

typedef struct {
    int32_t amp_q15;
    int8_t cutoff;
    int8_t reso;
    int8_t drive;
    int8_t delay;
    int8_t rev;
    int8_t glide;
    int8_t model;
    int8_t env_delay;
    int8_t env_attack;
    int8_t env_hold;
    int8_t env_decay;
    int8_t env_sustain;
    int8_t env_release;
    int8_t env_retrig;
    int8_t env_invert;
    int8_t env_accent;
    int8_t seq_gate;
    int8_t seq_ratchet;
    int8_t seq_rotate;
    int8_t seq_density;
    int8_t macro_depth[BASS_MOD_MACRO_N];
} bass_mod_runtime_t;

static inline int8_t bass_mod_clamp_i8(int32_t v)
{
    if (v < -127)
        return -127;
    if (v > 127)
        return 127;
    return (int8_t)v;
}

/* Curve axis 0..127: -EXP (0) · LOG (32) · LIN (64) · -LOG (96) · EXP (127), morphs between anchors.
 * s = source magnitude 0..128, output 0..128 (LIN = s). */
static inline int32_t bass_mod_isqrt(int32_t v)
{
    int32_t r = 0;
    int32_t bit = 1 << 14;

    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

static inline int32_t bass_mod_shape_fn(uint8_t k, int32_t s)
{
    switch (k) {
    case 0u:
        return 128 - ((128 - s) * (128 - s)) / 128; /* -EXP */
    case 1u:
        return bass_mod_isqrt(s * 128); /* LOG */
    case 2u:
        return s; /* LIN */
    case 3u:
        return 128 - bass_mod_isqrt((128 - s) * 128); /* -LOG */
    default:
        return (s * s) / 128; /* EXP */
    }
}

static inline int32_t bass_mod_shape(int32_t s, uint8_t c)
{
    uint8_t k;
    int32_t t;
    int32_t fa;
    int32_t fb;

    if (s < 0)
        s = 0;
    if (s > 128)
        s = 128;
    if (c > 127u)
        c = 127u;
    k = (uint8_t)(c / 32u);
    t = ((int32_t)(c & 31u) * 128) / 31;
    fa = bass_mod_shape_fn(k, s);
    fb = bass_mod_shape_fn((uint8_t)(k + 1u), s);
    return fa + ((fb - fa) * t) / 128;
}

static inline int bass_mod_curve_anchor(uint8_t c)
{
    return c == 0u || c == 32u || c == 64u || c == 96u || c == 127u;
}

static inline int8_t bass_mod_curve_apply(int8_t amt, uint8_t curve, int32_t src_q15)
{
    int32_t s = (src_q15 < 0 ? -src_q15 : src_q15) >> 8;
    int32_t y;

    if (s > 128)
        s = 128;
    y = bass_mod_shape(s, curve);
    if (src_q15 < 0)
        y = -y;
    return bass_mod_clamp_i8(((int32_t)amt * y) >> 7);
}

/* HOME MAC 1..3 + AUX (macro row 4: no knob, only reachable via M4 DEP chaining). */
enum { BASS_MAC_ACID = 0, BASS_MAC_GRIT, BASS_MAC_SPACE, BASS_MAC_AUX };

static inline const char *bass_mod_macro_name(uint8_t mi)
{
    static const char *const N[BASS_MOD_MACRO_N] = {"ACID", "GRIT", "SPACE", "AUX"};

    return N[mi % BASS_MOD_MACRO_N];
}

static inline const char *bass_mod_dest_name(uint8_t dest)
{
    static const char *const N[BASS_MOD_N] = {
        "OFF",    "AMP",    "CUTOFF", "RESO",   "DRIVE",  "DELAY",  "REV",    "GLIDE",  "MODEL",
        "E.DLY",  "E.ATK",  "E.HOLD", "E.DEC",  "E.SUS",  "E.REL",  "RETRIG", "INVERT", "ACCENT",
        "GATE",   "RATCH",  "SHIFT",  "DENS",   "M1 DEP", "M2 DEP", "M3 DEP", "M4 DEP",
    };

    return dest < BASS_MOD_N ? N[dest] : "?";
}

static inline void bass_mod_slot_set(bass_mod_slot_t *sl, uint8_t dest, int8_t amount)
{
    sl->dest = dest;
    sl->amount = amount;
    sl->curve = 64u;
}

/* Macro raw 64 = no change; full turn ≈ ±amount/2 on the dest. */
static inline void bass_mod_macro_rows_defaults(bass_mod_maps_t *m)
{
    uint8_t mi;
    uint8_t si;

    for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++)
        for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++)
            bass_mod_slot_set(&m->macro[mi][si], BASS_MOD_NONE, 0);
    bass_mod_slot_set(&m->macro[BASS_MAC_ACID][0], BASS_MOD_CUTOFF, 60);
    bass_mod_slot_set(&m->macro[BASS_MAC_ACID][1], BASS_MOD_RESO, 40);
    bass_mod_slot_set(&m->macro[BASS_MAC_ACID][2], BASS_MOD_ENV_DECAY, 60);
    bass_mod_slot_set(&m->macro[BASS_MAC_ACID][3], BASS_MOD_ENV_ACCENT, 40);
    bass_mod_slot_set(&m->macro[BASS_MAC_GRIT][0], BASS_MOD_DRIVE, 110);
    bass_mod_slot_set(&m->macro[BASS_MAC_GRIT][1], BASS_MOD_RESO, 40);
    bass_mod_slot_set(&m->macro[BASS_MAC_GRIT][2], BASS_MOD_ENV_SUSTAIN, 40);
    bass_mod_slot_set(&m->macro[BASS_MAC_SPACE][0], BASS_MOD_DELAY, 100);
    bass_mod_slot_set(&m->macro[BASS_MAC_SPACE][1], BASS_MOD_REV, 90);
    bass_mod_slot_set(&m->macro[BASS_MAC_SPACE][2], BASS_MOD_ENV_RELEASE, 50);
}

static inline uint8_t bass_mod_macro_rows_empty(const bass_mod_maps_t *m)
{
    uint8_t mi;
    uint8_t si;

    for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++)
        for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++)
            if (m->macro[mi][si].dest != BASS_MOD_NONE && m->macro[mi][si].amount != 0)
                return 0u;
    return 1u;
}

static inline void bass_mod_maps_defaults(bass_mod_maps_t *m)
{
    uint8_t si;

    if (!m)
        return;
    bass_mod_macro_rows_defaults(m);
    for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
        m->env[si].dest = BASS_MOD_NONE;
        m->env[si].amount = 0;
        m->env[si].curve = 64u;
    }
    /* Env slot 1 ships empty: AMP was only ever a ducking hook, so +100 looked like routing but did nothing. */
    m->env[0].dest = BASS_MOD_NONE;
    m->env[0].amount = 0;
    m->env[0].curve = 64u;
}

static inline void bass_mod_runtime_clear(bass_mod_runtime_t *rt)
{
    uint8_t i;

    if (!rt)
        return;
    for (i = 0u; i < BASS_MOD_MACRO_N; i++)
        rt->macro_depth[i] = 0;
    rt->amp_q15 = 32767;
    rt->cutoff = rt->reso = rt->drive = rt->delay = rt->rev = rt->glide = rt->model = 0;
    rt->env_delay = rt->env_attack = rt->env_hold = rt->env_decay = 0;
    rt->env_sustain = rt->env_release = rt->env_retrig = rt->env_invert = rt->env_accent = 0;
    rt->seq_gate = rt->seq_ratchet = rt->seq_rotate = rt->seq_density = 0;
}

static inline int8_t bass_mod_macro_depth_dest(uint8_t dest)
{
    if (dest == BASS_MOD_M1_DEPTH)
        return 0;
    if (dest == BASS_MOD_M2_DEPTH)
        return 1;
    if (dest == BASS_MOD_M3_DEPTH)
        return 2;
    if (dest == BASS_MOD_M4_DEPTH)
        return 3;
    return -1;
}

static inline void bass_mod_apply_dest(bass_mod_runtime_t *rt, uint8_t dest, int8_t delta)
{
    int8_t md;

    if (!rt || dest == BASS_MOD_NONE || dest >= BASS_MOD_N)
        return;
    md = bass_mod_macro_depth_dest(dest);
    if (md >= 0) {
        rt->macro_depth[md] = bass_mod_clamp_i8((int32_t)rt->macro_depth[md] + delta);
        return;
    }
    switch (dest) {
    case BASS_MOD_AMP:
        rt->amp_q15 += (int32_t)delta * 258;
        if (rt->amp_q15 < 0)
            rt->amp_q15 = 0;
        if (rt->amp_q15 > 32767)
            rt->amp_q15 = 32767;
        break;
    case BASS_MOD_CUTOFF:
        rt->cutoff = bass_mod_clamp_i8((int32_t)rt->cutoff + delta);
        break;
    case BASS_MOD_RESO:
        rt->reso = bass_mod_clamp_i8((int32_t)rt->reso + delta);
        break;
    case BASS_MOD_DRIVE:
        rt->drive = bass_mod_clamp_i8((int32_t)rt->drive + delta);
        break;
    case BASS_MOD_DELAY:
        rt->delay = bass_mod_clamp_i8((int32_t)rt->delay + delta);
        break;
    case BASS_MOD_REV:
        rt->rev = bass_mod_clamp_i8((int32_t)rt->rev + delta);
        break;
    case BASS_MOD_GLIDE:
        rt->glide = bass_mod_clamp_i8((int32_t)rt->glide + delta);
        break;
    case BASS_MOD_MODEL:
        rt->model = bass_mod_clamp_i8((int32_t)rt->model + delta);
        break;
    case BASS_MOD_ENV_DELAY:
        rt->env_delay = bass_mod_clamp_i8((int32_t)rt->env_delay + delta);
        break;
    case BASS_MOD_ENV_ATTACK:
        rt->env_attack = bass_mod_clamp_i8((int32_t)rt->env_attack + delta);
        break;
    case BASS_MOD_ENV_HOLD:
        rt->env_hold = bass_mod_clamp_i8((int32_t)rt->env_hold + delta);
        break;
    case BASS_MOD_ENV_DECAY:
        rt->env_decay = bass_mod_clamp_i8((int32_t)rt->env_decay + delta);
        break;
    case BASS_MOD_ENV_SUSTAIN:
        rt->env_sustain = bass_mod_clamp_i8((int32_t)rt->env_sustain + delta);
        break;
    case BASS_MOD_ENV_RELEASE:
        rt->env_release = bass_mod_clamp_i8((int32_t)rt->env_release + delta);
        break;
    case BASS_MOD_ENV_RETRIG:
        rt->env_retrig = bass_mod_clamp_i8((int32_t)rt->env_retrig + delta);
        break;
    case BASS_MOD_ENV_INVERT:
        rt->env_invert = bass_mod_clamp_i8((int32_t)rt->env_invert + delta);
        break;
    case BASS_MOD_ENV_ACCENT:
        rt->env_accent = bass_mod_clamp_i8((int32_t)rt->env_accent + delta);
        break;
    case BASS_MOD_SEQ_GATE:
        rt->seq_gate = bass_mod_clamp_i8((int32_t)rt->seq_gate + delta);
        break;
    case BASS_MOD_SEQ_RATCHET:
        rt->seq_ratchet = bass_mod_clamp_i8((int32_t)rt->seq_ratchet + delta);
        break;
    case BASS_MOD_SEQ_ROTATE:
        rt->seq_rotate = bass_mod_clamp_i8((int32_t)rt->seq_rotate + delta);
        break;
    case BASS_MOD_SEQ_DENSITY:
        rt->seq_density = bass_mod_clamp_i8((int32_t)rt->seq_density + delta);
        break;
    default:
        break;
    }
}

static inline int8_t bass_mod_slot_delta(const bass_mod_slot_t *sl, int32_t src_q15)
{
    if (!sl || sl->dest == BASS_MOD_NONE || sl->amount == 0)
        return 0;
    return bass_mod_curve_apply(sl->amount, sl->curve, src_q15);
}

/* Bit d set = dest d is fed by a macro row in macro_on (bit mi); M*-DEP chains reach later rows only,
 * as in bass_mod_apply_two_pass. */
static inline uint32_t bass_mod_dests_live(const bass_mod_maps_t *maps, uint8_t macro_on)
{
    uint32_t live = 0u;
    uint8_t mi;
    uint8_t si;

    if (!maps)
        return 0u;
    for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++) {
        if ((macro_on & (1u << mi)) == 0u)
            continue;
        for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
            const bass_mod_slot_t *sl = &maps->macro[mi][si];
            int8_t md;

            if (sl->dest == BASS_MOD_NONE || sl->dest >= BASS_MOD_N || sl->amount == 0)
                continue;
            live |= 1u << sl->dest;
            md = bass_mod_macro_depth_dest(sl->dest);
            if (md >= 0)
                macro_on = (uint8_t)(macro_on | (1u << md));
        }
    }
    return live;
}

static inline void bass_mod_apply_two_pass(const bass_mod_maps_t *maps, const uint8_t macro_raw[BASS_MOD_MACRO_N],
                                           const int8_t lfo_macro_delta[BASS_MOD_MACRO_N], int32_t env_mod_q15,
                                           bass_mod_runtime_t *rt)
{
    int8_t macro_val[BASS_MOD_MACRO_N];
    uint8_t mi;
    uint8_t si;

    if (!maps || !rt)
        return;
    bass_mod_runtime_clear(rt);

    /* Pass 1: LFO deltas → macro depths (centered around 0) */
    for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++) {
        int32_t d = (int32_t)macro_raw[mi] - 64;

        if (lfo_macro_delta)
            d += lfo_macro_delta[mi];
        macro_val[mi] = bass_mod_clamp_i8(d);
        rt->macro_depth[mi] = macro_val[mi];
    }

    /* Pass 2: Macros 1–4 → destinations (macro→macro clamped before next macro row) */
    for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++) {
        int32_t src = (int32_t)macro_val[mi] * 258;

        for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
            const bass_mod_slot_t *sl = &maps->macro[mi][si];
            int8_t md;
            int8_t delta;

            if (sl->dest == BASS_MOD_NONE || sl->amount == 0)
                continue;
            delta = bass_mod_slot_delta(sl, src);
            bass_mod_apply_dest(rt, sl->dest, delta);
            md = bass_mod_macro_depth_dest(sl->dest);
            if (md >= 0)
                macro_val[md] = rt->macro_depth[md];
        }
    }

    /* Env dest map (same enum) */
    for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
        const bass_mod_slot_t *sl = &maps->env[si];
        int8_t delta;

        if (sl->dest == BASS_MOD_NONE || sl->amount == 0)
            continue;
        delta = bass_mod_slot_delta(sl, env_mod_q15);
        bass_mod_apply_dest(rt, sl->dest, delta);
    }
}

#endif
