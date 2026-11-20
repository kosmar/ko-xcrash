/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared ADHSR-style envelope plot geometry (BASS draw_view_env + DECK drum env). */
#ifndef FM_ENV_PLOT_H
#define FM_ENV_PLOT_H

#include <stdint.h>

#ifndef FM_ENV_PLOT_SCALE
#define FM_ENV_PLOT_SCALE color_scale_rgb
#endif

/* KO KIT ENV bank page count (layout: fm_env_kit.h). */
#define FM_ENV_KIT_PAGES 3u

/*
 * CURVE axis 0..127: -EXP (0) . LOG (32) . LIN (64) . -LOG (96) . EXP (127).
 * Between anchors: linear morph of the two neighbouring shape values (fm_env_shape_pick5).
 * Progress laws, p 0..128, output 0..128: nexp, log, lin, nlog, exp (same as bass_mod_shape_fn).
 */
static inline int32_t fm_env_isqrt(int32_t v)
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

static inline const char *fm_env_shape_name(uint8_t curve)
{
    if (curve == 0u)
        return "-EXP";
    if (curve == 32u)
        return "LOG";
    if (curve == 64u)
        return "LIN";
    if (curve == 96u)
        return "-LOG";
    if (curve == 127u)
        return "EXP";
    return "BLND";
}

/*
 * Five anchor values (same units), order by anchor: nexp (0), log (32), lin (64), nlog (96), exp (127).
 * Segment k = curve / 32 (0..3), t = ((curve & 31) * 128) / 31 (0..128); curve 127 yields v_exp exactly.
 */
static inline int32_t fm_env_shape_pick5(int32_t v_nexp, int32_t v_log, int32_t v_lin, int32_t v_nlog,
                                         int32_t v_exp, uint8_t curve)
{
    int32_t f[5];
    uint32_t k;
    int32_t t;

    f[0] = v_nexp;
    f[1] = v_log;
    f[2] = v_lin;
    f[3] = v_nlog;
    f[4] = v_exp;
    if (curve > 127u)
        curve = 127u;
    k = curve / 32u;
    t = ((int32_t)(curve & 31u) * 128) / 31;
    return f[k] + ((f[k + 1u] - f[k]) * t) / 128;
}

/* Segment time → samples (DECK delay/attack/decay/release plot weights). */
static inline uint16_t fm_env_time_samples(uint8_t seg, uint32_t fs_hz)
{
    uint32_t s;

    if (seg == 0u)
        return 0u;
    s = ((uint32_t)seg * fs_hz) / 2000u;
    if (s > 8000u)
        s = 8000u;
    if (s < 1u)
        s = 1u;
    return (uint16_t)s;
}

/*
 * Attack level: prog 0 = floor, 128 = peak (same law as fm_env_curve_y A segment).
 * Curve 0 = nexp (1-(1-p)^2, legacy 2*lin - exp rounding), 127 = exp (p^2), 64 = lin.
 */
static inline int32_t fm_env_attack_level(int32_t y_bot, int32_t y_peak, uint8_t prog, uint8_t curve)
{
    int32_t p = (int32_t)prog;
    int32_t span = y_peak - y_bot;
    int32_t y_lin = y_bot + (span * p) / 128;
    int32_t y_exp = y_bot + (span * p * p) / (128 * 128);
    int32_t y_nexp = y_lin + y_lin - y_exp;
    int32_t y_log = y_bot + (span * fm_env_isqrt(p * 128)) / 128;
    int32_t y_nlog = y_bot + (span * (128 - fm_env_isqrt((128 - p) * 128))) / 128;

    return fm_env_shape_pick5(y_nexp, y_log, y_lin, y_nlog, y_exp, curve);
}

/*
 * Decay/release step scale vs linear (128). prog 0 = segment start, 128 = end.
 * Step laws are indexed reversed (exp at 0 ... nexp at 127). The nexp slot keeps the legacy bow
 * (decay p*(128-p), release tr*tr) so 0 / 64 / 127 stay identical; 32 / 96 use isqrt-based laws.
 * Range 24..280 by construction; clamped below as a safety bound (callers also clamp the level).
 */
static inline int32_t fm_env_decay_step_mul(uint8_t prog, uint8_t curve, uint8_t release_seg)
{
    int32_t t = (int32_t)prog;
    int32_t tr = 128 - t;
    int32_t m_exp = 24 + (t * t >> 6);
    int32_t m_log = 24 + 2 * fm_env_isqrt(t * 128);
    int32_t m_nlog = 24 + 2 * (128 - fm_env_isqrt(tr * 128));
    int32_t m_nexp = release_seg ? 24 + (tr * tr >> 6) : 24 + ((128 - t) * t >> 6);
    int32_t m = fm_env_shape_pick5(m_exp, m_nlog, 128, m_log, m_nexp, curve);

    if (m < 24)
        m = 24;
    if (m > 280)
        m = 280;
    return m;
}

/*
 * Falling segment (decay / release), prog 0 = top, 128 = bot.
 * Reversed law index: exp at curve 0 (p^2), nexp at curve 127 (tr^2 from bot), lin at 64.
 */
static inline int32_t fm_env_fall_level(int32_t top, int32_t bot, uint8_t prog, uint8_t curve)
{
    int32_t p = (int32_t)prog;
    int32_t tr = 128 - p;
    int32_t span = top - bot;
    int32_t y_lin = top - (span * p) / 128;
    int32_t y_exp = top - (span * p * p) / (128 * 128);
    int32_t y_nexp = bot + (span * tr * tr) / (128 * 128);
    int32_t y_log = top - (span * fm_env_isqrt(p * 128)) / 128;
    int32_t y_nlog = bot + (span * fm_env_isqrt(tr * 128)) / 128;

    return fm_env_shape_pick5(y_exp, y_nlog, y_lin, y_log, y_nexp, curve);
}

/*
 * Decay amplitude at segment progress p (0 = peak, 128 = sustain floor).
 * Curve 0 = exp (p^2 law), 127 = nexp (tr^2 law); not p*(128-p), which is 0 at both ends.
 */
static inline int32_t fm_env_decay_level(int32_t peak, int32_t floor, uint8_t prog, uint8_t curve)
{
    return fm_env_fall_level(peak, floor, prog, curve);
}

/* Release progress p: 0 = sustain, 128 = floor (e.g. amp env zero). */
static inline int32_t fm_env_release_level(int32_t sus, int32_t bot, uint8_t prog, uint8_t curve)
{
    return fm_env_fall_level(sus, bot, prog, curve);
}

/* Legacy shift tweak — kept for callers that only adjust sh; DECK uses step_mul in deck_env_decay. */
static inline uint32_t fm_env_decay_sh(uint32_t base_sh, uint8_t curve, int32_t v, int32_t peak)
{
    int32_t norm;
    int32_t bend;
    int32_t extra;
    uint32_t sh;

    if (peak < 1)
        return base_sh > 20u ? 20u : (base_sh < 1u ? 1u : base_sh);
    norm = (v << 7) / peak;
    if (norm < 0)
        norm = 0;
    if (norm > 128)
        norm = 128;
    bend = (int32_t)curve - 64;
    extra = (bend * (64 - norm)) >> 4;
    sh = (uint32_t)((int32_t)base_sh + extra);
    if (sh < 1u)
        sh = 1u;
    if (sh > 22u)
        sh = 22u;
    return sh;
}

/* INVERT = flip Y only (peak − env visually); time axis (X) unchanged. */
static inline int32_t fm_env_plot_map_y(int32_t y, int32_t y_top, int32_t y_bot, uint8_t invert)
{
    if (!invert)
        return y;
    return y_top + y_bot - y;
}

static inline void fm_env_plot_bounds(const uint32_t w[5], int32_t x0, int32_t plot_w, int32_t px[7])
{
    uint32_t acc4;
    uint32_t tot_ar;
    int32_t sus_w;
    int32_t rel_w;
    int32_t prefix_w;

    acc4 = w[0] + w[1] + w[2] + w[3];
    if (acc4 < 1u)
        acc4 = 1u;
    tot_ar = acc4 + w[4];
    if (tot_ar < 1u)
        tot_ar = 1u;
    sus_w = plot_w / 4;
    if (sus_w < 6)
        sus_w = 6;
    rel_w = (int32_t)((uint32_t)(plot_w - sus_w) * w[4] / tot_ar);
    if (w[4] > 0u) {
        int32_t rel_min = plot_w / 10;

        if (rel_min < 8)
            rel_min = 8;
        if (rel_w < rel_min)
            rel_w = rel_min;
    }
    if (rel_w > plot_w - sus_w - 8)
        rel_w = plot_w - sus_w - 8;
    if (rel_w < 0)
        rel_w = 0;
    {
        int32_t rel_floor = plot_w / 12;

        if (rel_floor < 10)
            rel_floor = 10;
        if (rel_w < rel_floor && plot_w - sus_w > rel_floor + 8)
            rel_w = rel_floor;
    }
    prefix_w = plot_w - sus_w - rel_w;
    if (prefix_w < 8) {
        prefix_w = 8;
        rel_w = plot_w - sus_w - prefix_w;
        if (rel_w < 0)
            rel_w = 0;
    }
    px[0] = x0;
    px[1] = x0 + (int32_t)((w[0] * (uint32_t)prefix_w) / acc4);
    px[2] = x0 + (int32_t)(((w[0] + w[1]) * (uint32_t)prefix_w) / acc4);
    px[3] = x0 + (int32_t)(((w[0] + w[1] + w[2]) * (uint32_t)prefix_w) / acc4);
    px[4] = x0 + prefix_w;
    px[5] = px[4] + sus_w;
    px[6] = x0 + plot_w;
}

static inline int32_t fm_env_curve_y(int32_t x, const int32_t *px, int32_t y_bot, int32_t y_peak, int32_t y_sus,
                                     uint8_t curve)
{
    int32_t den;
    int32_t t;

    if (x <= px[1])
        return y_bot;
    if (x <= px[2]) {
        den = px[2] - px[1];
        if (den < 1)
            den = 1;
        t = x - px[1];
        return fm_env_attack_level(y_bot, y_peak, (uint8_t)((t * 128) / den), curve);
    }
    if (x <= px[3])
        return y_peak;
    if (x <= px[4]) {
        den = px[4] - px[3];
        if (den < 1)
            den = 1;
        t = x - px[3];
        if (t >= den)
            return y_sus;
        return fm_env_decay_level(y_peak, y_sus, (uint8_t)((t * 128) / den), curve);
    }
    if (x <= px[5])
        return y_sus;
    if (x <= px[6]) {
        den = px[6] - px[5];
        if (den < 1)
            den = 1;
        t = x - px[5];
        if (t >= den)
            return y_bot;
        return fm_env_release_level(y_sus, y_bot, (uint8_t)((t * 128) / den), curve);
    }
    return y_bot;
}

#endif /* FM_ENV_PLOT_H */
