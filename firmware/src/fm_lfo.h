/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared LFO: 10-anchor morph + 8 FM operator algorithms (integer only, no float). */
#ifndef FM_LFO_H
#define FM_LFO_H

#include <stdint.h>

enum {
    FM_LFO_ALG_CLASSIC = 0, /* MORPH: 1-op 10-anchor waveform morph (SINE..NOISE) */
    FM_LFO_ALG_2OP     = 1, /* M -> C (2-Op phase mod + feedback) */
    FM_LFO_ALG_STACK   = 2, /* M2 -> M1 -> C (3-Op cascade) */
    FM_LFO_ALG_PARA    = 3, /* (M1 + M2) -> C (Dual parallel modulators) */
    FM_LFO_ALG_RING    = 4, /* C * M + FM (Ring mod AM + PM) */
    FM_LFO_ALG_FOLD    = 5, /* FOLD(M -> C) (Wavefolded 2-Op FM) */
    FM_LFO_ALG_SH_FM   = 6, /* S&H(M) -> C (Stepped S&H modulator -> FM carrier) */
    FM_LFO_ALG_CROSS   = 7, /* C <-> M (Cross-coupled dual feedback pendulum) */
    FM_LFO_ALG_PLUCK   = 8, /* EXP(M) -> C (Cycle-decaying FM pluck envelope) */
    FM_LFO_NALGOS      = 9
};

static const char *const FM_LFO_ALG_NAMES[FM_LFO_NALGOS] = {
    "MORPH", "FM 2-OP", "FM STACK", "FM PARA", "RING+FM", "FOLD FM", "SH->FM", "CROSS FB", "PLUCK FM"};
static const char *const FM_LFO_ALG_SHORT[FM_LFO_NALGOS] = {
    "1-OP", "M>C", "M2>M1>C", "M1+M2>C", "CxM+FM", "FLD(M>C)", "SH>FM", "C<>M", "ENV>FM"};

#define FM_LFO_NSHAPE_ANCHORS 10u

static const uint8_t FM_LFO_SHAPE_ANCHORS[FM_LFO_NSHAPE_ANCHORS] = {
    0u, 15u, 30u, 45u, 60u, 75u, 90u, 102u, 114u, 127u};

/* --- shape (0..127: 10-anchor classic waveform morph) --- */

static inline uint8_t fm_lfo_shape_snap(uint8_t v)
{
    uint32_t i;

    if (v > 127u)
        v = 127u;
    for (i = 0; i < FM_LFO_NSHAPE_ANCHORS; i++) {
        uint8_t a = FM_LFO_SHAPE_ANCHORS[i];
        uint32_t d = v > a ? (uint32_t)(v - a) : (uint32_t)(a - v);

        if (d <= 1u)
            return a;
    }
    return v;
}

static inline const char *fm_lfo_shape_name(uint8_t raw_v)
{
    uint8_t v = raw_v > 127u ? 127u : raw_v;

    /* Same edges as fm_lfo_classic_at / FM_LFO_SHAPE_ANCHORS. */
    if (v == 0u)
        return "SINE";
    if (v <= 14u)
        return "SINRMP";
    if (v == 15u)
        return "RAMP";
    if (v <= 21u)
        return "RMPRIS";
    if (v == 22u)
        return "RISE";
    if (v <= 29u)
        return "RISTRI";
    if (v == 30u)
        return "TRIANG";
    if (v <= 44u)
        return "TRISAW";
    if (v == 45u)
        return "SAW";
    if (v <= 59u)
        return "SAWFAL";
    if (v == 60u)
        return "FALL";
    if (v <= 74u)
        return "FALSQR";
    if (v == 75u)
        return "SQUARE";
    if (v <= 89u)
        return "SQRS&H";
    if (v == 90u)
        return "S&H";
    if (v <= 101u)
        return "S&HWLK";
    if (v == 102u)
        return "WALK";
    if (v <= 113u)
        return "WLKDRK";
    if (v == 114u)
        return "DRUNK";
    if (v <= 126u)
        return "DRKNSE";
    return "NOISE";
}

static inline uint8_t fm_lfo_step_shape(uint8_t cur, int32_t steps, uint8_t edit)
{
    if (edit) {
        uint32_t i, best = 0u, best_d = 999u;
        int32_t ni;

        if (cur > 127u)
            cur = 127u;
        for (i = 0; i < FM_LFO_NSHAPE_ANCHORS; i++) {
            uint8_t a = FM_LFO_SHAPE_ANCHORS[i];
            uint32_t d = cur > a ? (uint32_t)(cur - a) : (uint32_t)(a - cur);

            if (d < best_d) {
                best_d = d;
                best = i;
            }
        }
        ni = (int32_t)best + (steps > 0 ? 1 : -1);
        if (ni < 0)
            ni = 0;
        if (ni > (int32_t)(FM_LFO_NSHAPE_ANCHORS - 1u))
            ni = (int32_t)(FM_LFO_NSHAPE_ANCHORS - 1u);
        return FM_LFO_SHAPE_ANCHORS[(uint32_t)ni];
    }
    {
        int32_t v = (int32_t)cur + (steps > 0 ? 2 : -2);

        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
        return fm_lfo_shape_snap((uint8_t)v);
    }
}

/* Sentinel: runtime resolves to 1/1 at current BPM (fm_lfo_rate_resolved). */
#define FM_LFO_RATE_DEFAULT 22u
#define FM_LFO_TICKS_WHOLE 96u
#define FM_LFO_TICKS_4_1 384u
/* skew==FM_LFO_SKEW_DEFAULT: identity (no phase skew); UI shows (skew - default) as bipolar. */
#define FM_LFO_SKEW_DEFAULT 64u

static inline uint32_t fm_lfo_skew_phase(uint32_t ph, uint8_t skew)
{
    uint32_t u, split, out_u;

    if (skew == FM_LFO_SKEW_DEFAULT)
        return ph;
    u = ph >> 16;
    split = 8u + ((uint32_t)(127u - (skew > 127u ? 127u : skew)) * 240u) / 127u;
    if (split < 1u)
        split = 1u;
    if (split > 255u)
        split = 255u;
    if ((u >> 8) < split)
        out_u = (u * 128u) / split;
    else
        out_u = 32768u + ((u - (split << 8)) * 128u) / (256u - split);
    return (out_u << 16) | (ph & 0xFFFFu);
}

/* --- rate / tempo (no 64-bit divide) --- */

static inline uint32_t fm_lfo_inc(uint32_t rate)
{
    if (rate < 1u)
        rate = 1u;
    return 214748u + ((rate * rate) * 4100u);
}

static inline uint32_t fm_lfo_hz100(uint32_t rate)
{
    return (uint32_t)(((unsigned long long)fm_lfo_inc(rate) * 100000ull) >> 32);
}

static inline uint8_t fm_lfo_rate_anchor_ticks(uint32_t ticks, uint32_t bpm)
{
    uint32_t tgt, anchor = 0u, err = 0xffffffffu, r, e;

    if (bpm < 40u || bpm > 300u || !ticks)
        return (uint8_t)FM_LFO_RATE_DEFAULT;
    tgt = (bpm * 40u) / ticks;
    if (tgt < 4u || tgt > 1700u)
        return (uint8_t)FM_LFO_RATE_DEFAULT;
    for (r = 0u; r < 128u; r++) {
        uint32_t hz = fm_lfo_hz100(r);

        e = hz > tgt ? hz - tgt : tgt - hz;
        if (e < err) {
            err = e;
            anchor = r;
        }
    }
    return (uint8_t)anchor;
}

static inline uint8_t fm_lfo_rate_min(uint32_t bpm)
{
    if (bpm < 40u)
        bpm = 40u;
    else if (bpm > 300u)
        bpm = 300u;
    return fm_lfo_rate_anchor_ticks(FM_LFO_TICKS_4_1, bpm);
}

static inline uint8_t fm_lfo_rate_clamp(uint8_t rate, uint32_t bpm)
{
    uint8_t min = fm_lfo_rate_min(bpm);

    if (rate == (uint8_t)FM_LFO_RATE_DEFAULT)
        return rate;
    if (rate < min)
        return min;
    return rate;
}

static inline void fm_lfo_rate_store_clamp(uint8_t *rate, uint32_t bpm)
{
    uint8_t min = fm_lfo_rate_min(bpm);

    if (*rate == (uint8_t)FM_LFO_RATE_DEFAULT)
        return;
    if (*rate < min)
        *rate = min;
}

static inline void fm_lfo_rates_sanitize(uint8_t *rates, uint32_t n, uint32_t bpm)
{
    uint32_t i;

    for (i = 0; i < n; i++)
        fm_lfo_rate_store_clamp(&rates[i], bpm);
}

static inline uint8_t fm_lfo_rate_resolved(uint8_t rate, uint32_t bpm)
{
    if (rate == (uint8_t)FM_LFO_RATE_DEFAULT)
        return fm_lfo_rate_anchor_ticks(FM_LFO_TICKS_WHOLE, bpm);
    return rate;
}

enum {
    FM_LFO_DIV_4_1 = 1,
    FM_LFO_DIV_2_1,
    FM_LFO_DIV_1_1,
    FM_LFO_DIV_1_2,
    FM_LFO_DIV_1_4,
    FM_LFO_DIV_1_4T,
    FM_LFO_DIV_1_8,
    FM_LFO_DIV_1_8T,
    FM_LFO_DIV_1_16,
    FM_LFO_DIV_1_16T,
    FM_LFO_DIV_1_32
};

static inline uint32_t fm_lfo_div_ticks(uint8_t div)
{
    static const uint32_t ticks[11] = {384u, 192u, 96u, 48u, 24u, 16u, 12u, 8u, 6u, 4u, 3u};

    if (div < FM_LFO_DIV_4_1 || div > FM_LFO_DIV_1_32)
        return 0u;
    return ticks[div - 1u];
}

static inline uint8_t fm_lfo_rate_from_div(uint8_t div, uint32_t bpm)
{
    if (bpm < 40u)
        bpm = 40u;
    else if (bpm > 300u)
        bpm = 300u;
    if (div == FM_LFO_DIV_1_1)
        return (uint8_t)FM_LFO_RATE_DEFAULT;
    return fm_lfo_rate_anchor_ticks(fm_lfo_div_ticks(div), bpm);
}

static inline uint8_t fm_lfo_legacy_shape(uint8_t wave)
{
    switch (wave) {
    case 0u:
        return 0u;
    case 1u:
        return 30u;
    case 2u:
        return 75u;
    case 3u:
        return 90u;
    case 4u:
        return 45u;
    default:
        return wave;
    }
}

static inline uint32_t fm_lfo_lock_ticks(uint32_t rate, uint32_t bpm)
{
    static const uint32_t ticks[11] = {384u, 192u, 96u, 48u, 24u, 16u, 12u, 8u, 6u, 4u, 3u};
    uint32_t i, best = 11u, best_d = 99u;

    if (bpm < 40u || bpm > 300u || rate > 127u)
        return 0;
    rate = fm_lfo_rate_resolved((uint8_t)rate, bpm);
    for (i = 0; i < 11u; i++) {
        uint32_t tgt = (bpm * 40u) / ticks[i];
        uint32_t anchor = 0, err = 0xffffffffu, r, e, d;

        if (tgt < 4u || tgt > 1700u)
            continue;
        for (r = 0; r < 128u; r++) {
            uint32_t hz = fm_lfo_hz100(r);

            e = hz > tgt ? hz - tgt : tgt - hz;
            if (e < err) {
                err = e;
                anchor = r;
            }
        }
        if (err > tgt / 8u && err > 8u)
            continue;
        d = rate > anchor ? rate - anchor : anchor - rate;
        if (d <= 2u && d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best < 11u ? ticks[best] : 0u;
}

static inline const char *fm_lfo_note_div(uint32_t rate, uint32_t bpm)
{
    static const char *const name[11] = {
        "4/1", "2/1", "1/1", "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32"};
    static const uint32_t ticks[11] = {384u, 192u, 96u, 48u, 24u, 16u, 12u, 8u, 6u, 4u, 3u};
    uint32_t locked = fm_lfo_lock_ticks(rate, bpm);
    uint32_t i;

    if (!locked)
        return 0;
    for (i = 0; i < 11u; i++)
        if (ticks[i] == locked)
            return name[i];
    return 0;
}

/* --- Unified continuous Delay / Echo TIME continuum (0 = OFF, 1..127 = 20 ms .. 4000 ms with 16 musical fraction locks) --- */
#define FM_TIME_NDIVS 16u

static inline uint32_t fm_time_curve_ms(uint32_t cc)
{
    if (cc == 0u)
        return 0u;
    if (cc > 127u)
        cc = 127u;
    /* Smooth quadratic+cubic tape time curve: 1 -> ~20 ms, 64 -> ~620 ms, 127 -> 4000 ms */
    return 18u + cc * 2u + (cc * cc * 230u) / 1000u;
}

static inline uint32_t fm_time_div_mul24(uint32_t idx)
{
    static const uint16_t mul24[FM_TIME_NDIVS] = {
        3u, 4u, 6u, 8u, 9u, 12u, 16u, 18u, 24u, 32u, 36u, 48u, 96u, 192u, 288u, 384u
    };
    return mul24[idx < FM_TIME_NDIVS ? idx : (FM_TIME_NDIVS - 1u)];
}

static inline const char *fm_time_div_label(uint32_t idx)
{
    static const char *const names[FM_TIME_NDIVS] = {
        "1/32", "1/24", "1/16", "1/12", "3/32",
        "1/8",  "1/6",  "3/16", "1/4",  "1/3",  "3/8",
        "1/2",  "1/1",  "2/1",  "3/1",  "4/1"
    };
    return names[idx < FM_TIME_NDIVS ? idx : (FM_TIME_NDIVS - 1u)];
}

static inline uint8_t fm_time_anchor_cc(uint32_t idx, uint32_t bpm)
{
    uint32_t b = bpm < 40u ? 40u : (bpm > 240u ? 240u : bpm);
    uint32_t tgt_ms = (2500u * fm_time_div_mul24(idx)) / b;
    uint32_t best_cc = 1u, best_err = 0xffffffffu, c;

    for (c = 1u; c <= 127u; c++) {
        uint32_t ms = fm_time_curve_ms(c);
        uint32_t err = ms > tgt_ms ? ms - tgt_ms : tgt_ms - ms;
        if (err < best_err) {
            best_err = err;
            best_cc = c;
        }
    }
    return (uint8_t)best_cc;
}

static inline int fm_time_lock_idx(uint32_t cc, uint32_t bpm)
{
    uint32_t i;
    int best = -1;
    uint32_t best_d = 99u;

    if (cc == 0u || cc > 127u)
        return -1;
    for (i = 0u; i < FM_TIME_NDIVS; i++) {
        uint32_t a = fm_time_anchor_cc(i, bpm);
        uint32_t d = cc > a ? cc - a : a - cc;
        if (d <= 1u && d < best_d) {
            best_d = d;
            best = (int)i;
        }
    }
    return best;
}

static inline const char *fm_time_note_div(uint32_t cc, uint32_t bpm)
{
    int ix;
    if (cc == 0u)
        return "OFF";
    ix = fm_time_lock_idx(cc, bpm);
    if (ix >= 0)
        return fm_time_div_label((uint32_t)ix);
    return 0;
}

static inline uint32_t fm_time_ms(uint32_t cc, uint32_t bpm)
{
    int ix;
    uint32_t b = bpm < 40u ? 40u : (bpm > 240u ? 240u : bpm);

    if (cc == 0u)
        return 0u;
    ix = fm_time_lock_idx(cc, bpm);
    if (ix >= 0)
        return (2500u * fm_time_div_mul24((uint32_t)ix)) / b;
    return fm_time_curve_ms(cc);
}

static inline uint32_t fm_time_samples(uint32_t cc, uint32_t bpm, uint32_t fs)
{
    int ix;
    uint32_t b = bpm < 40u ? 40u : (bpm > 240u ? 240u : bpm);

    if (cc == 0u)
        return 0u;
    ix = fm_time_lock_idx(cc, bpm);
    if (ix >= 0) {
        uint32_t quarter = (60u * fs) / b;
        return (quarter * fm_time_div_mul24((uint32_t)ix)) / 24u;
    }
    return (fm_time_curve_ms(cc) * fs) / 1000u;
}

static inline uint8_t fm_time_step_snap(uint8_t cc, int32_t dir, uint32_t bpm)
{
    uint32_t i;
    if (dir > 0) {
        for (i = 0u; i < FM_TIME_NDIVS; i++) {
            uint8_t a = fm_time_anchor_cc(i, bpm);
            if (a > cc + 1u)
                return a;
        }
        return 127u;
    }
    if (dir < 0) {
        for (i = FM_TIME_NDIVS; i > 0u; i--) {
            uint8_t a = fm_time_anchor_cc(i - 1u, bpm);
            if (a + 1u < cc)
                return a;
        }
        return 0u;
    }
    return cc;
}

static inline void fm_time_fmt(char *s, uint8_t cc, uint32_t bpm, uint32_t now_ms)
{
    const char *nd = fm_time_note_div(cc, bpm);
    uint32_t ms, v;
    char *p = s;

    if (cc == 0u) {
        s[0] = 'O'; s[1] = 'F'; s[2] = 'F'; s[3] = 0;
        return;
    }
    /* When locked to a musical fraction, alternate between fraction ("3/16") and exact ms ("341MS") like LFO rate */
    if (nd && ((now_ms / 800u) & 1u) == 0u) {
        while (*nd)
            *p++ = *nd++;
        *p = 0;
        return;
    }
    ms = fm_time_ms(cc, bpm);
    if (ms > 9999u)
        ms = 9999u;
    v = ms;
    if (v >= 1000u) {
        *p++ = (char)('0' + v / 1000u);
        v %= 1000u;
        *p++ = (char)('0' + v / 100u);
        v %= 100u;
        *p++ = (char)('0' + v / 10u);
        *p++ = (char)('0' + v % 10u);
    } else if (v >= 100u) {
        *p++ = (char)('0' + v / 100u);
        v %= 100u;
        *p++ = (char)('0' + v / 10u);
        *p++ = (char)('0' + v % 10u);
    } else if (v >= 10u) {
        *p++ = (char)('0' + v / 10u);
        *p++ = (char)('0' + v % 10u);
    } else {
        *p++ = (char)('0' + v);
    }
    *p++ = 'm';
    *p++ = 's';
    *p = 0;
}

static inline uint32_t fm_lfo_u64_div_u32(uint32_t hi, uint32_t lo, uint32_t d)
{
    uint32_t q = 0u;
    uint32_t r = hi;
    int i;

    if (!d || r >= d)
        return 0xffffffffu;
    for (i = 0; i < 32; i++) {
        r = (r << 1) | (lo >> 31);
        lo <<= 1;
        q <<= 1;
        if (r >= d) {
            r -= d;
            q |= 1u;
        }
    }
    return q;
}

static inline uint32_t fm_lfo_run_inc(uint32_t rate, uint32_t bpm)
{
    uint32_t locked;

    rate = fm_lfo_rate_resolved((uint8_t)rate, bpm);
    rate = fm_lfo_rate_clamp((uint8_t)rate, bpm);
    locked = fm_lfo_lock_ticks(rate, bpm);

    if (!locked)
        return fm_lfo_inc(rate);
    return fm_lfo_u64_div_u32(bpm, 0u, 2500u * locked);
}

static inline uint32_t fm_lfo_slot_inc(uint32_t rate, uint32_t bpm, uint32_t slot)
{
    uint32_t inc = fm_lfo_run_inc(rate, bpm);
    int32_t spread = ((int32_t)((slot * 7u) & 15u) - 8);
    int32_t d = ((int32_t)(inc >> 9) * spread);

    return (uint32_t)((int32_t)inc + d);
}

static inline uint32_t fm_lfo_ph_step_px(void)
{
    return (uint32_t)(4294967296ull / 120u);
}

static inline void fm_lfo_ph64_rewind(uint64_t *ph64, uint32_t step_px, uint32_t steps)
{
    uint64_t sub = (uint64_t)step_px * (uint64_t)steps;

    if (*ph64 >= sub)
        *ph64 -= sub;
    else
        *ph64 = 0;
}

/* --- ratio --- */

static inline uint32_t fm_lfo_ratio_curve_q8(uint32_t v)
{
    if (v > 127u)
        v = 127u;
    return 85u + (v * 339u) / 127u + (v * v * 600u) / (127u * 127u);
}

static inline int fm_lfo_ratio_lock_idx(uint32_t v)
{
    static const uint8_t anchor[8] = {0u, 8u, 15u, 25u, 40u, 52u, 96u, 127u};
    uint32_t i;

    for (i = 0; i < 8u; i++) {
        uint32_t a = anchor[i];
        uint32_t d = v > a ? v - a : a - v;

        if (d <= 2u)
            return (int)i;
    }
    return -1;
}

static inline const char *fm_lfo_ratio_name(uint32_t v)
{
    static const char *const name[8] = {"3:1", "2:1", "3:2", "1:1", "2:3", "1:2", "1:3", "1:4"};
    int idx = fm_lfo_ratio_lock_idx(v);

    return idx >= 0 ? name[idx] : 0;
}

/* LFO modulator/carrier in Q8. CC 0 = 0.25, CC 64 = 1.00, CC 127 = 4.00.
 * Geometric, so the travel around 1.0 is fine and both ends stay reachable. */
static inline uint32_t fm_lfo_mod_ratio_q8(uint32_t cc)
{
    static const uint16_t tab[17] = {
        64, 76, 90, 108, 128, 152, 181, 215, 256, 304, 362, 431, 512, 609, 724, 861, 1024};
    uint32_t x, i, f, a, b;

    if (cc > 127u)
        cc = 127u;
    x = (cc * 64u) / 127u;
    if (x >= 64u)
        return 1024u;
    i = x >> 2;
    f = x & 3u;
    a = tab[i];
    b = tab[i + 1u];
    return a + ((b - a) * f) / 4u;
}

static inline void fm_lfo_fmt_ratio(char *d, uint32_t cc)
{
    uint32_t r100 = (fm_lfo_mod_ratio_q8(cc) * 100u + 128u) >> 8;
    uint32_t whole = r100 / 100u;
    uint32_t dec = r100 % 100u;

    if (whole > 9u)
        whole = 9u;
    d[0] = (char)('0' + whole);
    d[1] = '.';
    d[2] = (char)('0' + (dec / 10u));
    d[3] = (char)('0' + (dec % 10u));
    d[4] = 0;
}

/* Cubic so CC 64 is a small depth (~1.2) and 127 is still full (9.9). */
static inline uint8_t fm_lfo_depth_eff(int32_t cc)
{
    uint32_t c;

    if (cc < 0)
        c = 0u;
    else if (cc > 127)
        c = 127u;
    else
        c = (uint32_t)cc;
    return (uint8_t)((c * c * c) / (127u * 127u));
}

static inline void fm_lfo_fmt_depth(char *d, int32_t cc)
{
    uint32_t t = ((uint32_t)fm_lfo_depth_eff(cc) * 99u) / 127u;

    d[0] = (char)('0' + (t / 10u));
    d[1] = '.';
    d[2] = (char)('0' + (t % 10u));
    d[3] = 0;
}

static inline void fm_lfo_ratio_cm_q8(uint32_t v, uint32_t *cm_out, uint32_t *mm_q8_out)
{
    static const uint8_t c_mul[8] = {3, 2, 3, 1, 2, 1, 1, 1};
    static const uint8_t m_mul[8] = {1, 1, 2, 1, 3, 2, 3, 4};
    int idx = fm_lfo_ratio_lock_idx(v);

    if (idx >= 0) {
        *cm_out = c_mul[idx];
        *mm_q8_out = (uint32_t)m_mul[idx] << 8;
    } else {
        *cm_out = 1u;
        *mm_q8_out = fm_lfo_ratio_curve_q8(v);
    }
}

/* --- classic morph internals --- */

static inline uint32_t fm_lfo_hash(uint32_t a, uint32_t slot)
{
    uint32_t h = (a * 747796405u + slot * 2891336453u + 277803737u);

    h = ((h >> ((h >> 28) + 4u)) ^ h) * 277803737u;
    return h ^ (h >> 22);
}

static inline int32_t fm_lfo_walk_reflect(int32_t p)
{
    if (p > 58)
        p = 116 - p;
    if (p < -58)
        p = -116 - p;
    return p;
}

static inline int32_t fm_lfo_walk_step_delta(uint32_t seg, uint32_t slot)
{
    static const int8_t deltas[6] = {-22, -14, -7, 7, 14, 22};

    return (int32_t)deltas[fm_lfo_hash(seg, slot) % 6u];
}

static inline int32_t fm_lfo_walk_pos(uint32_t abs_seg, uint32_t slot)
{
    uint32_t base_seg = abs_seg & ~15u;
    uint32_t s;
    int32_t p = (int32_t)(fm_lfo_hash(base_seg, slot) % 41u) - 20;

    for (s = base_seg + 1u; s <= abs_seg; s++) {
        p += fm_lfo_walk_step_delta(s, slot);
        p = fm_lfo_walk_reflect(p);
    }
    return p;
}

static inline int32_t fm_lfo_sine_q15(uint32_t ph)
{
    int32_t x = (int32_t)(int16_t)(ph >> 16), ax = x < 0 ? -x : x, y, ay;

    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return y > 32767 ? 32767 : y < -32767 ? -32767 : y;
}

static inline int32_t fm_lfo_sine_scaled(uint32_t ph)
{
    return (fm_lfo_sine_q15(ph) * 63) >> 15;
}

/* SIN~RP edge on normalized 0..65535. The sine bend straightens as t rises. */
static inline int32_t fm_lfo_sine_ramp_edge(uint32_t u, uint32_t t, uint32_t falling)
{
    uint32_t curve_ph;
    int32_t curve, linear, s;

    if (u > 65535u)
        u = 65535u;
    curve_ph = (falling ? 0x40000000u : 0xc0000000u) + (u << 15);
    s = fm_lfo_sine_q15(curve_ph);
    curve = ((s + 32767) * 127) / 65534 - 64;
    if (falling)
        linear = 63 - (int32_t)((u * 127u) / 65535u);
    else
        linear = -64 + (int32_t)((u * 127u) / 65535u);
    return curve + ((linear - curve) * (int32_t)t) / 256;
}

/*
 * Peak travels 1/4 → 235/256, trough 3/4 → 255/256. The long rising edge
 * crosses the wrap; the shrinking return edge stays finite until the ramp
 * anchor, so every intermediate cycle remains closed and cliff-free.
 */
static inline int32_t fm_lfo_sine_ramp_skew(uint32_t ph, uint32_t t)
{
    uint32_t pos, peak, trough, span, dist, u;

    if (t == 0u)
        return fm_lfo_sine_scaled(ph);
    if (t >= 256u) {
        uint32_t p8 = (ph >> 24) & 255u;

        return p8 < 255u ? ((int32_t)p8 * 127) / 255 - 64 : 63;
    }

    pos = ph >> 16;
    peak = 16384u + (43776u * t) / 255u;
    trough = 49152u + (16128u * t) / 255u;
    if (pos >= peak && pos < trough) {
        span = trough - peak;
        dist = pos - peak;
        u = (dist * 65535u) / span;
        return fm_lfo_sine_ramp_edge(u, t, 1u);
    }

    span = 65536u - trough + peak;
    dist = pos >= trough ? pos - trough : 65536u - trough + pos;
    u = (dist * 65535u) / span;
    return fm_lfo_sine_ramp_edge(u, t, 0u);
}

static inline int32_t fm_lfo_tri_at(uint32_t ph, uint32_t split)
{
    uint32_t p8 = (ph >> 24) & 255u;
    int32_t y;

    if (split < 1u)
        split = 1u;
    if (split > 255u)
        split = 255u;
    if (p8 < split)
        y = ((int32_t)p8 * 127) / (int32_t)split - 64;
    else
        y = 63 - ((int32_t)(p8 - split) * 127) / (int32_t)(256u - split);
    if (y > 63)
        y = 63;
    if (y < -64)
        y = -64;
    return y;
}

static inline int32_t fm_lfo_saw_at(uint32_t ph)
{
    uint32_t p8 = (ph >> 24) & 255u;

    return 63 - ((int32_t)p8 * 127) / 255;
}

static inline int32_t fm_lfo_sqr_at(uint32_t ph, uint32_t hi_w)
{
    uint32_t p8 = (ph >> 24) & 255u;

    if (hi_w > 255u)
        hi_w = 255u;
    return p8 < hi_w ? 63 : -64;
}

static inline int32_t fm_lfo_exp_at(uint32_t ph)
{
    uint32_t u = (ph >> 24) & 255u;
    uint32_t inv = 255u - u;
    uint32_t e = (inv * inv) >> 8;

    e = (e * inv) >> 8;
    return (int32_t)(e >> 1) - 64;
}

static inline int32_t fm_lfo_exp_rise_at(uint32_t ph)
{
    uint32_t u = (ph >> 24) & 255u;

    return fm_lfo_exp_at(((255u - u) << 24) | (ph & 0x00FFFFFFu));
}

/*
 * RIS→TRI (CC 23..29): peak slides 255→128 p8 while the rise straightens
 * exp→linear; the growing return leg removes the end-of-cycle cliff.
 */
static inline int32_t fm_lfo_exp_rise_tri_skew(uint32_t ph, uint32_t t)
{
    uint32_t pos = (ph >> 24) & 255u;
    uint32_t peak_p8;
    int32_t y;

    if (t == 0u)
        return fm_lfo_exp_rise_at(ph);
    if (t >= 256u)
        return fm_lfo_tri_at(ph, 128u);

    peak_p8 = 255u - (127u * t) / 256u;
    if (peak_p8 < 1u)
        peak_p8 = 1u;

    if (pos <= peak_p8) {
        uint32_t u = (pos * 255u) / peak_p8;
        int32_t exp_y = fm_lfo_exp_rise_at((u << 24) | (ph & 0x00FFFFFFu));
        int32_t lin_y = -64 + ((int32_t)pos * 127) / (int32_t)peak_p8;

        y = exp_y + ((lin_y - exp_y) * (int32_t)t) / 256;
    } else {
        uint32_t span = 255u - peak_p8;

        if (span < 1u)
            return 63;
        y = 63 - ((int32_t)(pos - peak_p8) * 127) / (int32_t)span;
    }
    if (y > 63)
        y = 63;
    if (y < -64)
        y = -64;
    return y;
}

static inline int32_t fm_lfo_sh_level(uint32_t abs_seg, uint32_t slot)
{
    return (int32_t)(fm_lfo_hash(abs_seg, slot + 19u) % 127u) - 63;
}

static inline int32_t fm_lfo_sh_at(uint32_t ph, uint32_t cycle, uint32_t slot)
{
    uint32_t abs_seg = (cycle << 2) | (ph >> 30);

    return fm_lfo_sh_level(abs_seg, slot);
}

static inline int32_t fm_lfo_walk_ramp(uint32_t ph, uint32_t cycle, uint32_t slot, uint32_t u)
{
    uint32_t abs_seg = (cycle << 2) | (ph >> 30);
    int32_t y0 = fm_lfo_walk_pos(abs_seg, slot);
    int32_t y1 = fm_lfo_walk_pos(abs_seg + 1u, slot);

    return y0 + ((y1 - y0) * (int32_t)u) / 256;
}

static inline int32_t fm_lfo_sh_slew_walk(uint32_t ph, uint32_t cycle, uint32_t slot, uint32_t u,
                                          uint32_t morph_pct)
{
    uint32_t abs_seg = (cycle << 2) | (ph >> 30);
    int32_t sh = fm_lfo_sh_level(abs_seg, slot);
    int32_t sh1 = fm_lfo_sh_level(abs_seg + 1u, slot);
    int32_t slew;
    uint32_t hold_u;

    if (morph_pct >= 100u)
        return fm_lfo_walk_ramp(ph, cycle, slot, u);
    if (morph_pct == 0u)
        return sh;
    hold_u = (255u * (100u - morph_pct)) / 100u;
    if (u <= hold_u)
        slew = sh;
    else if (hold_u >= 255u)
        slew = sh1;
    else
        slew = sh + ((sh1 - sh) * (int32_t)(u - hold_u)) / (int32_t)(255u - hold_u);
    {
        int32_t walk = fm_lfo_walk_ramp(ph, cycle, slot, u);
        uint32_t t = (morph_pct * 256u) / 100u;

        return slew + ((walk - slew) * (int32_t)t) / 256;
    }
}

static inline int32_t fm_lfo_drunk_at(uint32_t ph, uint32_t cycle, uint32_t slot, uint32_t u)
{
    uint32_t abs_seg = (cycle << 3) | (ph >> 29);
    uint32_t h = fm_lfo_hash(abs_seg, slot + 99u);
    uint32_t mode = h % 100u;
    int32_t y0 = fm_lfo_walk_pos(abs_seg, slot);
    int32_t y1 = fm_lfo_walk_pos(abs_seg + 1u, slot);
    int32_t y;

    if (mode < 25u) {
        if (u < 128u)
            y = y0;
        else
            y = y0 + ((y1 - y0) * (int32_t)(u - 128u)) / 128;
    } else if (mode < 60u) {
        if (u < 96u)
            y = y0 - (y1 - y0) / 2 + ((y1 - y0) * (int32_t)u) / 192;
        else if (u < 160u)
            y = y1 + (y1 - y0) / 3;
        else
            y = y1 + ((y0 - y1) * (int32_t)(255u - u)) / 192;
    } else {
        uint32_t u_w;

        if ((h >> 8) & 1u)
            u_w = (u * u) >> 8;
        else
            u_w = 255u - (((255u - u) * (255u - u)) >> 8);
        if (u > 112u && u < 144u)
            u_w = u_w + ((u > 128u ? u - 128u : 128u - u) >> 2);
        y = y0 + ((y1 - y0) * (int32_t)u_w) / 256;
    }
    if (y > 63)
        y = 63;
    if (y < -64)
        y = -64;
    return y;
}

static inline int32_t fm_lfo_noise_at(uint32_t ph, uint32_t slot)
{
    uint32_t abs_seg = (ph >> 21) ^ (slot * 0x9e3779b9u);

    return (int32_t)(fm_lfo_hash(abs_seg, slot + 517u) % 121u) - 60;
}

static inline int32_t fm_lfo_blend(int32_t a, int32_t b, uint32_t t)
{
    return a + ((b - a) * (int32_t)t) / 256;
}

static inline int32_t fm_lfo_crackle_amp(uint32_t ph, uint32_t shift, uint32_t slot, uint32_t salt,
                                         uint32_t amp)
{
    uint32_t sub = ph >> shift;
    uint32_t span = amp * 2u + 1u;

    return (int32_t)(fm_lfo_hash(sub, slot + salt) % span) - (int32_t)amp;
}

static inline int32_t fm_lfo_drunk_noise_at(uint32_t ph, uint32_t cycle, uint32_t slot, uint32_t u_drunk,
                                            uint32_t cc)
{
    uint32_t t = ((cc - 114u) * 256u) / 13u;
    int32_t d = fm_lfo_drunk_at(ph, cycle, slot, u_drunk);
    int32_t y;

    if (cc <= 118u) {
        uint32_t step = cc - 115u;
        uint32_t amp = 6u + (step * 8u + 1u) / 3u;

        y = d + fm_lfo_crackle_amp(ph, 27, slot, 901u, amp);
    } else if (cc <= 122u) {
        uint32_t step = cc - 119u;
        uint32_t amp = 18u + (step * 18u + 1u) / 3u;
        int32_t carrier = (d * (int32_t)(256 - (t >> 1))) >> 8;
        int32_t crack = fm_lfo_crackle_amp(ph, 27, slot, 902u, amp / 2u)
                      + fm_lfo_crackle_amp(ph, 26, slot, 903u, amp)
                      + fm_lfo_crackle_amp(ph, 25, slot, 904u, amp / 2u);

        y = carrier + crack;
    } else {
        int32_t n = fm_lfo_noise_at(ph, slot);
        int32_t hf = (int32_t)(fm_lfo_hash(ph >> 25, slot + 905u) % 121u) - 60;
        uint32_t dom = ((cc - 123u) << 8) / 3u + 170u;

        if (dom > 255u)
            dom = 255u;
        int32_t ghost = (d * (int32_t)(256 - dom)) >> 8;

        y = ghost + ((hf * (int32_t)dom + n * (int32_t)dom) >> 8);
    }
    if (y > 63)
        y = 63;
    if (y < -64)
        y = -64;
    return y;
}

static inline int32_t fm_lfo_tri_skew_at(uint32_t ph, uint32_t split)
{
    if (split < 1u)
        return fm_lfo_saw_at(ph);
    if (split > 255u)
        split = 255u;
    return fm_lfo_tri_at(ph, split);
}

static inline int32_t fm_lfo_classic_at(uint32_t ph, uint32_t cycle, uint8_t shape_cc, uint32_t slot)
{
    uint32_t cc = shape_cc;
    uint32_t u_walk = (ph >> 22) & 255u;
    uint32_t u_drunk = (ph >> 21) & 255u;

    if (cc == 0u)
        return fm_lfo_sine_scaled(ph);
    if (cc <= 14u)
        return fm_lfo_sine_ramp_skew(ph, ((cc - 1u) << 8) / 13u);
    if (cc == 15u)
        return fm_lfo_tri_at(ph, 255u);
    if (cc <= 22u) {
        uint32_t t = ((cc - 16u) << 8) / 6u;

        if (t > 256u)
            t = 256u;
        return fm_lfo_blend(fm_lfo_tri_at(ph, 255u), fm_lfo_exp_rise_at(ph), t);
    }
    if (cc <= 29u) {
        /* CC 22 = open RIS; 23..29 slide peak with t = 1/7 .. 7/7 of morph span. */
        uint32_t t = ((cc - 22u) << 8) / 7u;

        if (t > 256u)
            t = 256u;
        return fm_lfo_exp_rise_tri_skew(ph, t);
    }
    if (cc == 30u)
        return fm_lfo_tri_at(ph, 128u);
    if (cc <= 44u) {
        uint32_t t = ((cc - 31u) << 8) / 13u;
        uint32_t split = 128u - (t * 127u) / 256u;

        return fm_lfo_tri_skew_at(ph, split);
    }
    if (cc == 45u)
        return fm_lfo_saw_at(ph);
    if (cc <= 59u) {
        uint32_t t = ((cc - 46u) << 8) / 13u;

        if (t > 256u)
            t = 256u;
        return fm_lfo_blend(fm_lfo_saw_at(ph), fm_lfo_exp_at(ph), t);
    }
    if (cc == 60u)
        return fm_lfo_exp_at(ph);
    if (cc <= 74u) {
        uint32_t t = ((cc - 61u) << 8) / 13u;

        if (t > 256u)
            t = 256u;
        return fm_lfo_blend(fm_lfo_exp_at(ph), fm_lfo_sqr_at(ph, 128u), t);
    }
    if (cc == 75u)
        return fm_lfo_sqr_at(ph, 128u);
    if (cc <= 89u) {
        uint32_t t = ((cc - 76u) << 8) / 13u;
        int32_t sh = fm_lfo_sh_at(ph, cycle, slot);

        if (t > 256u)
            t = 256u;
        return fm_lfo_blend(fm_lfo_sqr_at(ph, 128u), sh, t);
    }
    if (cc == 90u)
        return fm_lfo_sh_at(ph, cycle, slot);
    if (cc <= 101u) {
        uint32_t morph = ((cc - 91u) * 100u) / 10u;

        return fm_lfo_sh_slew_walk(ph, cycle, slot, u_walk, morph);
    }
    if (cc == 102u)
        return fm_lfo_walk_ramp(ph, cycle, slot, u_walk);
    if (cc <= 113u) {
        uint32_t t = ((cc - 103u) << 8) / 10u;
        int32_t walk = fm_lfo_walk_ramp(ph, cycle, slot, u_walk);
        int32_t drunk = fm_lfo_drunk_at(ph, cycle, slot, u_drunk);

        return fm_lfo_blend(walk, drunk, t);
    }
    if (cc == 114u)
        return fm_lfo_drunk_at(ph, cycle, slot, u_drunk);
    if (cc <= 126u)
        return fm_lfo_drunk_noise_at(ph, cycle, slot, u_drunk, cc);
    return fm_lfo_noise_at(ph, slot);
}

/* --- FM operator helpers --- */

static inline int32_t fm_lfo_clamp64(int32_t y)
{
    if (y > 63)
        return 63;
    if (y < -64)
        return -64;
    return y;
}

static inline int32_t fm_lfo_op_at(uint32_t ph, uint32_t cycle, uint32_t slot, uint8_t shape_cc,
                                   uint8_t skew_cc)
{
    ph = fm_lfo_skew_phase(ph, skew_cc);
    return fm_lfo_classic_at(ph, cycle, fm_lfo_shape_snap(shape_cc), slot);
}

static inline uint32_t fm_lfo_ph_m1(uint32_t ph, uint32_t mm_q8)
{
    return (uint32_t)(((uint64_t)ph * (uint64_t)mm_q8) >> 8);
}

static inline uint32_t fm_lfo_pmod_ph(int32_t mod64, int32_t index_cc)
{
    int32_t s = mod64 * (int32_t)fm_lfo_depth_eff(index_cc);

    /* eff 127 → about ±1/2 cycle. The cubic keeps that at the top of the knob. */
    return (uint32_t)(s << 17);
}

static inline int32_t fm_lfo_fold64(int32_t x, int32_t fold_gain)
{
    int32_t lim;
    int32_t guard = 8;

    /* fold_gain is already fm_lfo_depth_eff (0..127). 0 = no fold, 127 = tight. */
    if (fold_gain < 0)
        fold_gain = 0;
    if (fold_gain > 127)
        fold_gain = 127;
    lim = 64 - (fold_gain >> 1);
    if (lim < 4)
        lim = 4;
    while (guard-- > 0) {
        if (x <= lim && x >= -lim)
            break;
        if (x > lim)
            x = lim - (x - lim);
        else
            x = -lim + (-lim - x);
    }
    return fm_lfo_clamp64(x);
}

static inline int32_t fm_lfo_eval_full_at(uint32_t ph, uint32_t cycle, uint32_t slot, uint8_t shape_cc,
                                          uint8_t skew_cc, uint8_t algo, uint8_t ratio_cc,
                                          uint8_t index_cc, uint8_t fb_cc, uint8_t fold_cc)
{
    if (algo == FM_LFO_ALG_CLASSIC) {
        int32_t y = fm_lfo_op_at(ph, cycle, slot, shape_cc, skew_cc);

        if (fold_cc > 0u)
            y = fm_lfo_fold64(y, (int32_t)fm_lfo_depth_eff((int32_t)fold_cc));
        return y;
    }

    {
        uint32_t mm_q8;
        uint32_t ph_c, ph_m1, ph_m2, mm2_q8;
        int32_t idx, fb, mod, car, y;
        uint32_t abs_seg;

        mm_q8 = fm_lfo_mod_ratio_q8(ratio_cc);
        ph_c = ph;
        ph_m1 = fm_lfo_ph_m1(ph, mm_q8);
        idx = (int32_t)index_cc + 1;
        fb = (int32_t)fb_cc;

        switch (algo) {
        case FM_LFO_ALG_2OP:
            mod = fm_lfo_op_at(ph_m1 + fm_lfo_pmod_ph(fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc), fb),
                               cycle, slot, shape_cc, skew_cc);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            return fm_lfo_clamp64(car);

        case FM_LFO_ALG_STACK:
            mm2_q8 = mm_q8 << 1;
            if (mm2_q8 > 2048u)
                mm2_q8 = 2048u;
            ph_m2 = fm_lfo_ph_m1(ph, mm2_q8);
            mod = fm_lfo_op_at(ph_m2, cycle, slot, shape_cc, skew_cc);
            mod = fm_lfo_op_at(ph_m1 + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            return fm_lfo_clamp64(car);

        case FM_LFO_ALG_PARA:
            mm2_q8 = mm_q8 ? (256u * 256u) / mm_q8 : 256u;
            if (mm2_q8 < 64u)
                mm2_q8 = 64u;
            if (mm2_q8 > 1024u)
                mm2_q8 = 1024u;
            ph_m2 = fm_lfo_ph_m1(ph, mm2_q8);
            mod = fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc);
            mod += fm_lfo_op_at(ph_m2, cycle, slot, shape_cc, skew_cc);
            mod >>= 1;
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            return fm_lfo_clamp64(car);

        case FM_LFO_ALG_RING: {
            int32_t c0 = fm_lfo_op_at(ph_c, cycle, slot, shape_cc, skew_cc);
            int32_t m0 = fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc);
            int32_t ring = (c0 * m0) >> 6;

            mod = fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            y = (ring >> 1) + (car >> 1);
            return fm_lfo_clamp64(y);
        }

        case FM_LFO_ALG_FOLD:
            mod = fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            {
                int32_t fg = (int32_t)fm_lfo_depth_eff(fb) + (int32_t)fm_lfo_depth_eff(idx - 1);

                if (fg > 127)
                    fg = 127;
                car = fm_lfo_fold64(car, fg);
            }
            return fm_lfo_clamp64(car);

        case FM_LFO_ALG_SH_FM:
            abs_seg = (cycle << 2) | (ph >> 30);
            mod = fm_lfo_sh_level(abs_seg, slot + (uint32_t)fb);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(mod, idx - 1), cycle, slot, shape_cc, skew_cc);
            return fm_lfo_clamp64(car);

        case FM_LFO_ALG_CROSS: {
            int32_t c0 = fm_lfo_op_at(ph_c, cycle, slot, shape_cc, skew_cc);
            int32_t m0 = fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc);
            mod = fm_lfo_op_at(ph_m1 + fm_lfo_pmod_ph(c0, fb), cycle, slot, shape_cc, skew_cc);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph(m0, idx - 1), cycle, slot, shape_cc, skew_cc);
            y = (car + mod) >> 1;
            return fm_lfo_clamp64(y);
        }

        case FM_LFO_ALG_PLUCK: {
            int32_t env = fm_lfo_exp_at(ph) + 64;

            mod = fm_lfo_op_at(ph_m1, cycle, slot, shape_cc, skew_cc);
            car = fm_lfo_op_at(ph_c + fm_lfo_pmod_ph((mod * env) >> 7, idx - 1), cycle, slot, shape_cc,
                               skew_cc);
            return fm_lfo_clamp64(car);
        }

        default:
            return fm_lfo_op_at(ph, cycle, slot, shape_cc, skew_cc);
        }
    }
}

static inline int32_t fm_lfo_eval_full_q15(uint32_t ph, uint32_t cycle, uint32_t slot, uint8_t shape_cc,
                                           uint8_t skew_cc, uint8_t algo, uint8_t ratio_cc,
                                           uint8_t index_cc, uint8_t fb_cc, uint8_t fold_cc)
{
    int32_t v = fm_lfo_eval_full_at(ph, cycle, slot, shape_cc, skew_cc, algo, ratio_cc, index_cc, fb_cc,
                                    fold_cc);
    int32_t q = v * 512;

    if (q > 32767)
        return 32767;
    if (q < -32767)
        return -32767;
    return q;
}

/* Legacy entry (dinghy / midi_ctrl): skew defaults to center. */
static inline int32_t fm_lfo_eval_at(uint32_t ph, uint32_t cycle, uint32_t slot, uint8_t algo,
                                     uint8_t shape_cc, uint8_t ratio_cc, uint8_t index_cc, uint8_t fb_cc,
                                     uint8_t fold_cc)
{
    return fm_lfo_eval_full_at(ph, cycle, slot, shape_cc, FM_LFO_SKEW_DEFAULT, algo, ratio_cc, index_cc, fb_cc,
                               fold_cc);
}

static inline int32_t fm_lfo_slew_step(int32_t prev, int32_t target, uint8_t smooth)
{
    int32_t alpha;

    if (smooth == 0u)
        return target;
    alpha = 256 - ((int32_t)smooth * 244) / 127;
    if (alpha < 8)
        alpha = 8;
    return prev + (((target - prev) * alpha) >> 8);
}

/* --- Shared Multi-Destination LFO Bank (host passes dest_names[ndests]) --- */
typedef struct {
    uint8_t rate;
    uint8_t depth;
    uint8_t wave;
    uint8_t phase;
    uint8_t smooth;
    uint8_t skew;
    uint8_t algo;
    uint8_t ratio;
    uint8_t index;
    uint8_t fb;
    uint8_t fold;
} fm_lfo_slot_t;

typedef struct {
    uint32_t ph;
    uint32_t cycle;
    int32_t sm;
} fm_lfo_rt_t;

static inline void fm_lfo_slot_reset(fm_lfo_slot_t *s)
{
    s->rate = FM_LFO_RATE_DEFAULT;
    s->depth = 0u;
    s->wave = 0u;
    s->phase = 0u;
    s->smooth = 0u;
    s->skew = FM_LFO_SKEW_DEFAULT;
    s->algo = FM_LFO_ALG_CLASSIC;
    s->ratio = 52u;
    s->index = 64u;
    s->fb = 0u;
    s->fold = 0u;
}

static inline void fm_lfo_slots_init(fm_lfo_slot_t *slots, fm_lfo_rt_t *rt, uint32_t ndests)
{
    uint32_t i;
    for (i = 0; i < ndests; i++) {
        fm_lfo_slot_reset(&slots[i]);
        if (rt) {
            rt[i].ph = 0u;
            rt[i].cycle = 0u;
            rt[i].sm = 0;
        }
    }
}

/* Persist/plate paths may leave rate at 0; remap untouched legacy default (32 = old 1/2 lock). */
static inline void fm_lfo_slots_fixup_empty_rates(fm_lfo_slot_t *slots, uint32_t ndests)
{
    uint32_t i;

    for (i = 0; i < ndests; i++) {
        if (slots[i].rate == 0u)
            slots[i].rate = fm_lfo_rate_min(120u);
        else if (slots[i].rate == 32u && slots[i].depth == 0u)
            slots[i].rate = FM_LFO_RATE_DEFAULT;
        else
            fm_lfo_rate_store_clamp(&slots[i].rate, 120u);
    }
}

static inline void fm_lfo_slots_sanitize_rates(fm_lfo_slot_t *slots, uint32_t ndests, uint32_t bpm)
{
    uint32_t i;

    for (i = 0; i < ndests; i++)
        fm_lfo_rate_store_clamp(&slots[i].rate, bpm);
}

/* Tempo-synced slot = its rate locks onto a note division at bpm (same test as fm_lfo_run_inc). */
static inline int fm_lfo_slot_is_sync(uint8_t rate, uint32_t bpm)
{
    rate = fm_lfo_rate_clamp(fm_lfo_rate_resolved(rate, bpm), bpm);
    return fm_lfo_lock_ticks(rate, bpm) != 0u;
}

/* Transport start (Play / MIDI FA): tempo-synced slots restart at their slot phase. Free slots keep running. */
static inline void fm_lfo_slots_sync_restart(const fm_lfo_slot_t *slots, fm_lfo_rt_t *rt, uint32_t ndests,
                                             uint32_t bpm)
{
    uint32_t t;

    for (t = 0; t < ndests; t++) {
        if (slots[t].depth != 0u && !fm_lfo_slot_is_sync(slots[t].rate, bpm))
            continue;
        rt[t].ph = 0u;
        rt[t].cycle = 0u;
    }
}

/* n_samples at sample_rate: fm_lfo_inc is phase/ms (same as midi_lfo_tick).
 * running = 0 (transport stopped): tempo-synced slots hold phase and output; free slots keep running. */
static inline void fm_lfo_slots_step_tr(const fm_lfo_slot_t *slots, fm_lfo_rt_t *rt, uint32_t ndests,
                                        uint8_t sel_target, uint32_t bpm, uint32_t n_samples,
                                        uint32_t sample_rate, uint8_t running)
{
    uint32_t t;

    uint32_t mul_q12;
    if (!sample_rate)
        sample_rate = 44100u;
    mul_q12 = ((n_samples * 1000u) << 12) / sample_rate;
    for (t = 0; t < ndests; t++) {
        const fm_lfo_slot_t *s = &slots[t];
        uint32_t inc;
        uint32_t old_ph;
        uint32_t ph0;
        uint32_t dph;
        int32_t q;

        if (s->depth == 0u && t != (uint32_t)sel_target)
            continue;
        if (!running && fm_lfo_slot_is_sync(s->rate, bpm))
            continue;
        inc = fm_lfo_slot_inc(s->rate, bpm, t);
        dph = (uint32_t)(((uint64_t)inc * (uint64_t)mul_q12) >> 12);
        old_ph = rt[t].ph;
        rt[t].ph += dph;
        if (rt[t].ph < old_ph)
            rt[t].cycle++;
        ph0 = rt[t].ph + ((uint32_t)s->phase << 25);
        q = fm_lfo_eval_full_q15(ph0, rt[t].cycle, t, s->wave, s->skew, s->algo, s->ratio, s->index, s->fb,
                                 s->fold);
        rt[t].sm = fm_lfo_slew_step(rt[t].sm, q, s->smooth);
    }
}

static inline void fm_lfo_slots_step(const fm_lfo_slot_t *slots, fm_lfo_rt_t *rt, uint32_t ndests,
                                     uint8_t sel_target, uint32_t bpm, uint32_t n_samples,
                                     uint32_t sample_rate)
{
    fm_lfo_slots_step_tr(slots, rt, ndests, sel_target, bpm, n_samples, sample_rate, 1u);
}

static inline int32_t fm_lfo_slot_delta(const fm_lfo_slot_t *slots, const fm_lfo_rt_t *rt, uint8_t t)
{
    int32_t d = (int32_t)slots[t].depth;
    if (d == 0)
        return 0;
    return (rt[t].sm * d) >> 15;
}

static inline uint8_t fm_lfo_clamp_u8(int32_t v, uint8_t lo, uint8_t hi)
{
    if (v < (int32_t)lo)
        return lo;
    if (v > (int32_t)hi)
        return hi;
    return (uint8_t)v;
}

/* --- Knob readout range: base ± depth while an LFO is fast ---
 * Bars stay live. Value text shows lo~hi = base − depth ~ base + depth (raw 0..127, clamped),
 * i.e. the LFO's full excursion around the parameter's own value. Shown while the LFO is fast
 * (rate >= FM_LFO_READOUT_FAST_HZ100, or the effective value moved > 1 step since the last frame).
 * Tunable: FM_LFO_READOUT_FAST_HZ100 (2.00 Hz). */
#ifndef FM_LFO_READOUT_FAST_HZ100
#define FM_LFO_READOUT_FAST_HZ100 200u
#endif
#ifndef FM_LFO_READOUT_N
#define FM_LFO_READOUT_N 4u
#endif

typedef struct {
    uint8_t prev_eff;
    uint8_t have_prev;
} fm_lfo_readout_t;

static fm_lfo_readout_t fm_lfo_readout_slots[FM_LFO_READOUT_N];

/* Effective modulation rate in Hz×100: tempo-locked divisions use tick period, else free Hz. */
static inline uint32_t fm_lfo_slot_hz100(uint8_t rate, uint32_t bpm)
{
    uint32_t locked = fm_lfo_lock_ticks(rate, bpm);

    if (locked)
        return (bpm * 40u) / locked;
    return fm_lfo_hz100(fm_lfo_rate_resolved(rate, bpm));
}

static inline uint8_t fm_lfo_readout_is_fast(uint32_t hz100, uint8_t eff, uint8_t prev, uint8_t have_prev)
{
    int32_t d;

    if (hz100 >= FM_LFO_READOUT_FAST_HZ100)
        return 1u;
    if (!have_prev)
        return 0u;
    d = (int32_t)eff - (int32_t)prev;
    if (d < 0)
        d = -d;
    return d > 1 ? 1u : 0u;
}

/* Readout pair for the value text: base ± depth, clamped to the knob's raw 0..127.
 * Bipolar params use the same raw scale (centre 64 = 0 on the ±50 display). Integer only. */
static inline void fm_lfo_readout_range(uint8_t base, uint8_t depth, uint8_t *out_lo, uint8_t *out_hi)
{
    *out_lo = fm_lfo_clamp_u8((int32_t)base - (int32_t)depth, 0u, 127u);
    *out_hi = fm_lfo_clamp_u8((int32_t)base + (int32_t)depth, 0u, 127u);
}

/* Feed one knob column with the current effective (heard) u8 and the parameter's base u8.
 * Returns 1 when the value text should show base ± depth (*out_lo / *out_hi); 0 = show live value. */
static inline uint8_t fm_lfo_readout_update(uint8_t col, uint8_t eff, uint8_t base, uint8_t depth,
                                           uint8_t rate, uint32_t bpm, uint8_t *out_lo, uint8_t *out_hi)
{
    (void)eff;
    (void)rate;
    (void)bpm;
    if (col >= FM_LFO_READOUT_N)
        return 0u;
    if (depth == 0u)
        return 0u;
    fm_lfo_readout_range(base, depth, out_lo, out_hi);
    return 1u;
}

/* --- Destination list: every bank knob with a target is a dest; touching it selects it --- */
typedef struct {
    uint8_t *order;
    uint8_t *pos;
    uint8_t n;
    uint8_t cap;
} fm_lfo_dests_t;

static inline void fm_lfo_dests_clear(fm_lfo_dests_t *d)
{
    uint32_t i;

    d->n = 0u;
    for (i = 0; i < d->cap; i++)
        d->pos[i] = 255u;
}

static inline void fm_lfo_dests_init(fm_lfo_dests_t *d, uint8_t *order, uint8_t *pos, uint8_t cap)
{
    d->order = order;
    d->pos = pos;
    d->cap = cap;
    fm_lfo_dests_clear(d);
}

static inline int fm_lfo_dests_has(const fm_lfo_dests_t *d, uint8_t t)
{
    return t < d->cap && d->pos[t] < d->n;
}

static inline int fm_lfo_dests_add(fm_lfo_dests_t *d, uint8_t t)
{
    if (t >= d->cap)
        return 0;
    if (!fm_lfo_dests_has(d, t)) {
        d->pos[t] = d->n;
        d->order[d->n++] = t;
    }
    return 1;
}

/* Knob touch: registers t (no knob target can fall off the list) and makes it the selected dest. */
static inline int fm_lfo_dests_touch(fm_lfo_dests_t *d, uint8_t *sel, uint8_t t)
{
    if (!fm_lfo_dests_add(d, t))
        return 0;
    *sel = t;
    return 1;
}

static inline uint8_t fm_lfo_dests_first(const fm_lfo_dests_t *d, uint8_t fallback)
{
    return d->n ? d->order[0] : fallback;
}

static inline uint8_t fm_lfo_dests_last(const fm_lfo_dests_t *d, uint8_t fallback)
{
    return d->n ? d->order[d->n - 1u] : fallback;
}

static inline uint8_t fm_lfo_dests_unpack(const fm_lfo_dests_t *d, uint8_t raw, uint8_t fallback)
{
    uint8_t t = d->cap ? (uint8_t)(raw % d->cap) : raw;

    return fm_lfo_dests_has(d, t) ? t : fm_lfo_dests_first(d, fallback);
}

static inline uint8_t fm_lfo_dests_step(const fm_lfo_dests_t *d, uint8_t cur, int32_t delta)
{
    int32_t ix;

    if (d->n == 0u)
        return d->cap ? (uint8_t)(cur % d->cap) : cur;
    cur = (uint8_t)(cur % d->cap);
    if (!fm_lfo_dests_has(d, cur))
        return delta < 0 ? d->order[d->n - 1u] : d->order[0];
    ix = ((int32_t)d->pos[cur] + delta) % (int32_t)d->n;
    if (ix < 0)
        ix += d->n;
    return d->order[ix];
}

/* 0..127 knob position of t along the list. */
static inline uint8_t fm_lfo_dests_knob_val(const fm_lfo_dests_t *d, uint8_t t)
{
    uint32_t ix;

    if (d->n <= 1u)
        return 0u;
    ix = fm_lfo_dests_has(d, t) ? d->pos[t] : 0u;
    return (uint8_t)((ix * 127u) / (d->n - 1u));
}

#ifdef C_WHITE
typedef int32_t (*fm_lfo_sample_fn)(void *ctx, uint32_t ph, uint32_t cycle);

/* Left edge is now. Pixels to the right are the upcoming phase. Returns the now Y. */
static inline int32_t fm_lfo_draw_trace(void *ctx, fm_lfo_sample_fn sample, uint64_t now64, uint8_t smooth,
                                        int32_t depth, int32_t x0, int32_t npx, int32_t mid, int32_t amp,
                                        int32_t y_lo, int32_t y_hi, uint16_t col)
{
    uint64_t step = 4294967296ull / 112ull;
    uint64_t ph64 = now64;
    uint64_t pre64 = ph64 >= step * 32ull ? ph64 - step * 32ull : 0ull;
    int32_t prev;
    int32_t py = -1;
    int32_t y_now = mid;
    uint32_t i;
    int32_t x;

    prev = sample(ctx, (uint32_t)pre64, (uint32_t)(pre64 >> 32));
    for (i = 0; i < 32u; i++) {
        int32_t raw = sample(ctx, (uint32_t)pre64, (uint32_t)(pre64 >> 32));

        pre64 += step;
        prev = fm_lfo_slew_step(prev, raw, smooth);
    }
    for (x = 0; x < npx; x++) {
        int32_t raw = sample(ctx, (uint32_t)ph64, (uint32_t)(ph64 >> 32));
        int32_t w = fm_lfo_slew_step(prev, raw, smooth);
        int32_t y;
        int32_t xp = x0 + x;

        ph64 += step;
        prev = w;
        if (depth == 0)
            y = mid;
        else
            y = mid - ((w * depth) / 127 * amp) / 32767;
        if (y < y_lo)
            y = y_lo;
        if (y > y_hi)
            y = y_hi;
        if (x == 0)
            y_now = y;
        if (py >= 0 && xp - 1 != x0)
            cv_line(xp - 1, py, xp, y, col);
        py = y;
    }
    return y_now;
}

static inline void fm_lfo_draw_now_tick(int32_t x, int32_t y_now, int32_t y_lo, int32_t y_hi, uint16_t col)
{
    int32_t y0 = y_now - 8;
    int32_t y1 = y_now + 8;
    int32_t y;

    if (y0 < y_lo)
        y0 = y_lo;
    if (y1 > y_hi)
        y1 = y_hi;
    for (y = y0; y <= y1; y++)
        cv_pset(x, y, col);
}

static inline const char *fm_lfo_shape_pure_name(uint8_t v)
{
    if (v > 127u)
        v = 127u;
    switch (v) {
    case 0u:
        return "SINE";
    case 15u:
        return "RAMP";
    case 22u:
        return "RISE";
    case 30u:
        return "TRIANG";
    case 45u:
        return "SAW";
    case 60u:
        return "FALL";
    case 75u:
        return "SQUARE";
    case 90u:
        return "S&H";
    case 102u:
        return "WALK";
    case 114u:
        return "DRUNK";
    case 127u:
        return "NOISE";
    default:
        return 0;
    }
}

/* Morph halves: 3-char tokens; t is 0..255 along the span (nearer left at t=0). */
static inline int fm_lfo_shape_morph(uint8_t raw_v, const char **left, const char **right, uint32_t *t)
{
    uint8_t v = raw_v > 127u ? 127u : raw_v;

    if (fm_lfo_shape_pure_name(v))
        return 0;
    if (v <= 14u) {
        *left = "SIN";
        *right = "RMP";
        *t = ((uint32_t)(v - 1u) << 8) / 13u;
        return 1;
    }
    if (v <= 21u) {
        *left = "RMP";
        *right = "RIS";
        *t = ((uint32_t)(v - 16u) << 8) / 6u;
        return 1;
    }
    if (v <= 29u) {
        *left = "RIS";
        *right = "TRI";
        *t = ((uint32_t)(v - 23u) << 8) / 7u;
        return 1;
    }
    if (v <= 44u) {
        *left = "TRI";
        *right = "SAW";
        *t = ((uint32_t)(v - 31u) << 8) / 13u;
        return 1;
    }
    if (v <= 59u) {
        *left = "SAW";
        *right = "FAL";
        *t = ((uint32_t)(v - 46u) << 8) / 13u;
        return 1;
    }
    if (v <= 74u) {
        *left = "FAL";
        *right = "SQR";
        *t = ((uint32_t)(v - 61u) << 8) / 13u;
        return 1;
    }
    if (v <= 89u) {
        *left = "SQR";
        *right = "S&H";
        *t = ((uint32_t)(v - 76u) << 8) / 13u;
        return 1;
    }
    if (v <= 101u) {
        *left = "S&H";
        *right = "WLK";
        *t = ((uint32_t)(v - 91u) << 8) / 10u;
        return 1;
    }
    if (v <= 113u) {
        *left = "WLK";
        *right = "DRK";
        *t = ((uint32_t)(v - 103u) << 8) / 10u;
        return 1;
    }
    if (v <= 126u) {
        *left = "DRK";
        *right = "NSE";
        *t = ((uint32_t)(v - 115u) << 8) / 11u;
        return 1;
    }
    return 0;
}

static inline uint16_t fm_lfo_rgb565_lerp(uint16_t c0, uint16_t c1, uint32_t t)
{
    uint32_t r0 = (uint32_t)(c0 >> 11) & 31u;
    uint32_t g0 = (uint32_t)(c0 >> 5) & 63u;
    uint32_t b0 = (uint32_t)c0 & 31u;
    uint32_t r1 = (uint32_t)(c1 >> 11) & 31u;
    uint32_t g1 = (uint32_t)(c1 >> 5) & 63u;
    uint32_t b1 = (uint32_t)c1 & 31u;
    uint32_t inv = 255u - t;

    return (uint16_t)(((r0 * inv + r1 * t) / 255u) << 11 | ((g0 * inv + g1 * t) / 255u) << 5 |
                      ((b0 * inv + b1 * t) / 255u));
}

/* Knob column: center two morph halves in 48 px; lerp col_hi/col_lo by span t (0..255). */
static inline int32_t fm_dual_morph_label_draw(int32_t col_x, int32_t y, const char *left,
                                               const char *right, uint32_t t, uint16_t col_hi,
                                               uint16_t col_lo)
{
    const int32_t bar_l = col_x + 6;
    const int32_t bar_w = 48;
    int32_t tw = text_w(&FONT_S, left) + text_w(&FONT_S, right);
    int32_t tx = bar_l + (bar_w - tw) / 2;
    uint16_t lc = fm_lfo_rgb565_lerp(col_hi, col_lo, t);
    uint16_t rc = fm_lfo_rgb565_lerp(col_lo, col_hi, t);
    int32_t x1 = cv_text(tx, y, &FONT_S, left, lc);

    return cv_text(x1, y, &FONT_S, right, rc);
}

/* Knob column: center label in 48 px; morph halves lerp col_hi/col_lo by span t (0..255). */
static inline int32_t fm_lfo_shape_draw(int32_t col_x, int32_t y, uint8_t cc, uint16_t col_hi,
                                        uint16_t col_lo)
{
    const char *pure = fm_lfo_shape_pure_name(cc);
    const int32_t bar_l = col_x + 6;
    const int32_t bar_w = 48;

    if (pure) {
        int32_t tw = text_w(&FONT_S, pure);
        int32_t tx = bar_l + (bar_w - tw) / 2;

        cv_text(tx, y, &FONT_S, pure, col_hi);
        return tx + tw;
    }
    {
        const char *left, *right;
        uint32_t t;
        int32_t tw, tx;

        if (!fm_lfo_shape_morph(cc, &left, &right, &t)) {
            const char *fallback = fm_lfo_shape_name(cc);

            tw = text_w(&FONT_S, fallback);
            tx = bar_l + (bar_w - tw) / 2;
            cv_text(tx, y, &FONT_S, fallback, col_hi);
            return tx + tw;
        }
        return fm_dual_morph_label_draw(col_x, y, left, right, t, col_hi, col_lo);
    }
}

static inline void fm_lfo_draw_algo_badge(int32_t x, int32_t y, uint8_t algo, uint16_t col_op,
                                          uint16_t col_car, uint16_t col_line)
{
    int32_t bx = x, by = y + 3;

    switch (algo) {
    case FM_LFO_ALG_CLASSIC:
        cv_rect(bx + 14, by, 8, 8, col_car);
        break;

    case FM_LFO_ALG_2OP:
        cv_rect(bx + 4, by, 8, 8, col_op);
        cv_rect(bx + 22, by, 8, 8, col_car);
        cv_line(bx + 12, by + 4, bx + 22, by + 4, col_line);
        break;

    case FM_LFO_ALG_STACK:
        cv_rect(bx, by, 7, 7, col_op);
        cv_rect(bx + 12, by, 7, 7, col_op);
        cv_rect(bx + 26, by, 8, 8, col_car);
        cv_line(bx + 7, by + 3, bx + 12, by + 3, col_line);
        cv_line(bx + 19, by + 3, bx + 26, by + 3, col_line);
        break;

    case FM_LFO_ALG_PARA:
        cv_rect(bx + 2, by - 2, 7, 6, col_op);
        cv_rect(bx + 2, by + 4, 7, 6, col_op);
        cv_rect(bx + 22, by + 1, 8, 8, col_car);
        cv_line(bx + 9, by + 1, bx + 22, by + 5, col_line);
        cv_line(bx + 9, by + 7, bx + 22, by + 5, col_line);
        break;

    case FM_LFO_ALG_RING:
        cv_rect(bx + 6, by, 8, 8, col_car);
        cv_rect(bx + 20, by, 8, 8, col_op);
        cv_line(bx + 10, by + 2, bx + 24, by + 6, col_line);
        cv_line(bx + 10, by + 6, bx + 24, by + 2, col_line);
        break;

    case FM_LFO_ALG_FOLD:
        cv_rect(bx + 4, by, 7, 8, col_op);
        cv_rect(bx + 22, by, 8, 8, col_car);
        cv_line(bx + 11, by + 4, bx + 22, by + 4, col_line);
        cv_line(bx + 14, by + 1, bx + 17, by + 4, col_line);
        cv_line(bx + 17, by + 4, bx + 14, by + 7, col_line);
        break;

    case FM_LFO_ALG_SH_FM:
        cv_rect(bx + 4, by, 8, 8, col_op);
        cv_rect(bx + 22, by, 8, 8, col_car);
        cv_line(bx + 6, by + 2, bx + 10, by + 2, col_line);
        cv_line(bx + 6, by + 5, bx + 10, by + 5, col_line);
        cv_line(bx + 12, by + 4, bx + 22, by + 4, col_line);
        break;

    case FM_LFO_ALG_CROSS:
        cv_rect(bx + 4, by, 8, 8, col_car);
        cv_rect(bx + 22, by, 8, 8, col_op);
        cv_line(bx + 12, by + 2, bx + 22, by + 6, col_line);
        cv_line(bx + 12, by + 6, bx + 22, by + 2, col_line);
        cv_line(bx + 4, by + 6, bx + 12, by + 2, col_line);
        cv_line(bx + 4, by + 2, bx + 12, by + 6, col_line);
        break;

    case FM_LFO_ALG_PLUCK:
        cv_line(bx + 6, by, bx + 14, by + 8, col_line);
        cv_line(bx + 14, by + 8, bx + 18, by + 8, col_line);
        cv_rect(bx + 22, by, 8, 8, col_car);
        cv_line(bx + 18, by + 4, bx + 22, by + 4, col_line);
        break;

    default:
        cv_rect(bx + 14, by, 8, 8, col_car);
        break;
    }
}

typedef struct {
    const fm_lfo_slot_t *slots;
    uint8_t slot;
} fm_lfo_slot_eval_ctx_t;

static inline int32_t fm_lfo_slot_trace_cb(void *ctx, uint32_t ph, uint32_t cycle)
{
    const fm_lfo_slot_eval_ctx_t *c = (const fm_lfo_slot_eval_ctx_t *)ctx;
    const fm_lfo_slot_t *s = &c->slots[c->slot];
    return fm_lfo_eval_full_q15(ph + ((uint32_t)s->phase << 25), cycle, c->slot,
                                s->wave, s->skew, s->algo, s->ratio, s->index, s->fb, s->fold);
}

static inline void fm_lfo_slots_draw_view(const fm_lfo_slot_t *slots, const fm_lfo_rt_t *rt,
                                          uint8_t sel_target, uint8_t ndests,
                                          const char *const *dest_names, uint16_t col_acc)
{
    uint8_t sel = sel_target % ndests;
    uint8_t t;
    fm_lfo_slot_eval_ctx_t ctx;
    const fm_lfo_slot_t *ss = &slots[sel];

    ctx.slots = slots;
    cv_line(8, 66, 231, 66, C_LINE);
    for (t = 0; t < ndests; t++) {
        uint64_t now64;
        if (t == sel || slots[t].depth == 0u)
            continue;
        ctx.slot = t;
        now64 = ((uint64_t)rt[t].cycle << 32) | (uint64_t)rt[t].ph;
        fm_lfo_draw_trace(&ctx, fm_lfo_slot_trace_cb, now64, slots[t].smooth,
                          (int32_t)slots[t].depth, 8, 224, 66, 48, 8, 124, C_DIM);
    }
    {
        uint64_t now64 = ((uint64_t)rt[sel].cycle << 32) | (uint64_t)rt[sel].ph;
        int32_t y_now;

        ctx.slot = sel;
        y_now = fm_lfo_draw_trace(&ctx, fm_lfo_slot_trace_cb, now64, ss->smooth,
                                  (int32_t)ss->depth, 8, 224, 66, 48, 8, 124,
                                  ss->depth ? C_WHITE : C_DIM);
        fm_lfo_draw_now_tick(8, y_now, 8, 124, col_acc);
    }
    if (dest_names && dest_names[sel])
        cv_text(4, 4, &FONT_S, dest_names[sel], C_WHITE);
    if (ss->algo != FM_LFO_ALG_CLASSIC)
        fm_lfo_draw_algo_badge(136, 6, ss->algo, col_acc, C_WHITE, C_GRAY);
}
#endif /* C_WHITE */

#endif /* FM_LFO_H */
