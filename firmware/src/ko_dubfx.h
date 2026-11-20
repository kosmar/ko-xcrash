/* SPDX-License-Identifier: GPL-3.0-only */
/* KO dub FX bus: KO BASS tape echo (6 head modes, wow, loop tilt, top-end saturation), spring/tank reverb
 * (damp, tail, bleed) and DJ isolator — ported from dub_dsp.h with private state and pools.
 * Header-only. Needs fm_lfo.h (fm_time_samples). */
#ifndef KO_DUBFX_H
#define KO_DUBFX_H

#include <stdint.h>
#include "fm_lfo.h"

#define KO_DFX_FS      44100u
#define KO_DFX_DLY_LEN 32768u
#define KO_DFX_REV_LEN 24576u

enum { KO_DFX_TAP_SINGLE = 0, KO_DFX_TAP_DUAL, KO_DFX_TAP_GALLOP, KO_DFX_TAP_TRIP, KO_DFX_TAP_SPREAD,
       KO_DFX_TAP_PING, KO_DFX_TAP_N };

static const char *const KO_DFX_TAP_NAME[KO_DFX_TAP_N] = {"SINGLE", "DUAL", "GALLOP", "TRIP", "SPREAD", "PING"};

#define KO_DFX_AP0_OFF 0u
#define KO_DFX_AP0_LEN 142u
#define KO_DFX_AP1_OFF (KO_DFX_AP0_OFF + KO_DFX_AP0_LEN)
#define KO_DFX_AP1_LEN 107u
#define KO_DFX_AP2_OFF (KO_DFX_AP1_OFF + KO_DFX_AP1_LEN)
#define KO_DFX_AP2_LEN 379u
#define KO_DFX_AP3_OFF (KO_DFX_AP2_OFF + KO_DFX_AP2_LEN)
#define KO_DFX_AP3_LEN 277u
#define KO_DFX_CH0_OFF (KO_DFX_AP3_OFF + KO_DFX_AP3_LEN)
#define KO_DFX_CH0_LEN 113u
#define KO_DFX_CH1_OFF (KO_DFX_CH0_OFF + KO_DFX_CH0_LEN)
#define KO_DFX_CH1_LEN 181u
#define KO_DFX_CH2_OFF (KO_DFX_CH1_OFF + KO_DFX_CH1_LEN)
#define KO_DFX_CH2_LEN 307u
#define KO_DFX_T0_OFF  (KO_DFX_CH2_OFF + KO_DFX_CH2_LEN)
#define KO_DFX_T0_LEN  3901u
#define KO_DFX_T1_OFF  (KO_DFX_T0_OFF + KO_DFX_T0_LEN)
#define KO_DFX_T1_LEN  4837u
#define KO_DFX_T2_OFF  (KO_DFX_T1_OFF + KO_DFX_T1_LEN)
#define KO_DFX_T2_LEN  3109u
#define KO_DFX_T3_OFF  (KO_DFX_T2_OFF + KO_DFX_T2_LEN)
#define KO_DFX_T3_LEN  (KO_DFX_REV_LEN - KO_DFX_T3_OFF)

#define KO_DFX_FB_DIST_START 108
#define KO_DFX_FB_DIST_SPAN  19
#define KO_DFX_FB_DIST_AMT   26214

#ifndef KO_POOL_SECTION
#if defined(DINGHY_HOST) || defined(DINGHY_WEB) || defined(__EMSCRIPTEN__) || defined(__APPLE__) || \
    defined(__linux__)
#define KO_POOL_SECTION
#else
#define KO_POOL_SECTION __attribute__((section(".pool")))
#endif
#endif

static int16_t ko_dfx_dly_pool[KO_DFX_DLY_LEN] KO_POOL_SECTION;
static int16_t ko_dfx_rev_pool[KO_DFX_REV_LEN] KO_POOL_SECTION;

typedef struct {
    uint8_t dly_lvl;  /* echo return */
    uint8_t dly_time; /* fm_time cc: synced divisions or free ms; 0 = echo off */
    uint8_t feedback; /* 106 = unity sustain, above = saturated top end */
    int8_t tone;      /* loop tilt: <0 darker repeats, >0 thinner repeats */
    uint8_t wow;
    uint8_t tap;      /* KO_DFX_TAP_* */
    uint8_t tank;     /* <64 spring, >=64 tank */
    int8_t iso;       /* -64 LP .. +63 HP */
    uint8_t rev_mix, rev_damp, rev_tail, bleed;
    uint8_t bpm;
    int8_t tape_oct;  /* -1, 0, +1 */
    uint8_t freeze;
} ko_dfx_params_t;

typedef struct {
    uint32_t dly_w;
    uint32_t dly_len_sm;
    uint32_t dly_base;
    int32_t tap_b_ofs;
    uint8_t dly_half_rate;
    uint8_t dly_half_toggle;
    int32_t dly_lf_sm;
    int32_t dly_loop_dc;
    int32_t dly_lp;
    int32_t dly_hp;
    uint32_t wow_ph;
    uint16_t rev_ap_pos[4];
    uint16_t rev_ch_pos[3];
    uint16_t rev_t_pos[4];
    int32_t rev_t_lp[4];
    int32_t iso_lp, iso_lp_r, iso_hp, iso_hp_r;
} ko_dfx_t;

static inline int32_t ko_dfx_softclip(int32_t x)
{
    int32_t x2, x3;

    if (x > 49151)
        x = 49151;
    else if (x < -49151)
        x = -49151;
    x2 = (int32_t)(((int64_t)x * x) >> 15);
    x3 = (int32_t)(((int64_t)x2 * x) >> 15);
    return x - ((x3 * 4855) >> 15);
}

static inline int32_t ko_dfx_echo_sat(int32_t x)
{
    int32_t x2, x3;

    if (x > 32767)
        x = 32767;
    else if (x < -32767)
        x = -32767;
    x2 = (x * x) >> 15;
    x3 = (x2 * x) >> 15;
    return x - (x3 * 10923 >> 15);
}

static inline int32_t ko_dfx_master_soft(int32_t x)
{
    x = ko_dfx_softclip(x);
    if (x > 28000)
        return 28000;
    if (x < -28000)
        return -28000;
    return x;
}

static inline void ko_dfx_init(ko_dfx_t *fx)
{
    uint32_t i;
    uint8_t *b = (uint8_t *)fx;

    for (i = 0u; i < (uint32_t)sizeof(*fx); i++)
        b[i] = 0u;
    for (i = 0u; i < KO_DFX_DLY_LEN; i++)
        ko_dfx_dly_pool[i] = 0;
    for (i = 0u; i < KO_DFX_REV_LEN; i++)
        ko_dfx_rev_pool[i] = 0;
    fx->dly_half_rate = 1u;
    fx->dly_lf_sm = 64;
}

/* Per block: resolve tempo-synced echo length and tape speed divider. */
static inline void ko_dfx_block_begin(ko_dfx_t *fx, const ko_dfx_params_t *p)
{
    uint32_t bpm = p->bpm < 40u ? 72u : (p->bpm > 180u ? 180u : (uint32_t)p->bpm);
    uint32_t d = fm_time_samples(p->dly_time, bpm, KO_DFX_FS);
    uint8_t rdiv = 1u;
    int32_t oct = (int32_t)p->tape_oct;

    if (d == 0u)
        d = (60u * KO_DFX_FS) / bpm;
    while (oct > 0) {
        d >>= 1;
        oct--;
    }
    while (oct < 0) {
        d <<= 1;
        oct++;
    }
    while (d > (KO_DFX_DLY_LEN - 128u) && rdiv < 16u) {
        d >>= 1;
        rdiv <<= 1;
    }
    if (d < 8u)
        d = 8u;
    if (d > (KO_DFX_DLY_LEN - 64u))
        d = KO_DFX_DLY_LEN - 64u;
    fx->dly_base = d;
    fx->dly_half_rate = rdiv;
    if (fx->dly_half_toggle >= rdiv)
        fx->dly_half_toggle = 0u;
    fx->tap_b_ofs = (int32_t)(KO_DFX_FS / (100u * (uint32_t)rdiv));
}

static inline int32_t ko_dfx_dly_read(uint32_t wpos, uint32_t delay, int32_t wow_smp)
{
    int32_t rp = (int32_t)wpos + (int32_t)KO_DFX_DLY_LEN - (int32_t)delay - wow_smp;

    while (rp < 0)
        rp += (int32_t)KO_DFX_DLY_LEN;
    while (rp >= (int32_t)KO_DFX_DLY_LEN)
        rp -= (int32_t)KO_DFX_DLY_LEN;
    return ko_dfx_dly_pool[rp];
}

static inline int32_t ko_dfx_ap(uint32_t off, uint32_t len, uint16_t *pos, int32_t in)
{
    int16_t *buf = &ko_dfx_rev_pool[off];
    uint32_t p = (uint32_t)*pos % len;
    int32_t ap = buf[p];
    int32_t v = in + ((ap * 180) >> 8);

    if (v > 32767)
        v = 32767;
    else if (v < -32767)
        v = -32767;
    buf[p] = (int16_t)(v >> 1);
    *pos = (uint16_t)((p + 1u) % len);
    return ap - v;
}

static inline int32_t ko_dfx_tank(uint32_t off, uint32_t len, uint16_t *pos, int32_t in, int32_t *lp, int32_t damp)
{
    int16_t *buf = &ko_dfx_rev_pool[off];
    uint32_t p = (uint32_t)*pos % len;
    int32_t d = buf[p];
    int32_t nw;

    *lp += ((d - *lp) * damp) >> 8;
    nw = in + (*lp >> 1);
    if (nw > 32767)
        nw = 32767;
    else if (nw < -32767)
        nw = -32767;
    buf[p] = (int16_t)nw;
    *pos = (uint16_t)((p + 1u) % len);
    return *lp;
}

/* One frame. dly_in / rev_send: mono sends (Q15). Adds echo + reverb returns to *wl / *wr; returns the
 * mono reverb return alone in *rev_out (for app-side extras) when non-NULL. */
static inline void ko_dfx_frame(ko_dfx_t *fx, const ko_dfx_params_t *p, int32_t dly_in, int32_t rev_send,
                                int32_t *wl, int32_t *wr, int32_t *rev_out)
{
    int32_t dly_l = 0;
    int32_t dly_r = 0;

    {
        uint32_t q = (fx->wow_ph >> 16) & 65535u;
        int32_t tri = (int32_t)(q < 32768u ? q : 65535u - q) - 16384;
        int32_t wow_smp = (tri * (int32_t)p->wow) >> 11;
        uint32_t dly_s;
        uint32_t dly_b;
        int32_t tap_a, tap_b, fb_gain, wet_l, wet_r;
        uint8_t head_mode = p->tap % KO_DFX_TAP_N;

        {
            int32_t dly_tgt = (int32_t)fx->dly_base;
            int32_t dly_cur = (int32_t)fx->dly_len_sm;

            if (dly_cur == 0)
                dly_cur = dly_tgt;
            else {
                int32_t dd = dly_tgt - dly_cur;

                if (dd > 12)
                    dd = 12;
                else if (dd < -12)
                    dd = -12;
                dly_cur += dd;
            }
            if (dly_cur < 8)
                dly_cur = 8;
            fx->dly_len_sm = (uint32_t)dly_cur;
            dly_s = (uint32_t)dly_cur;
        }
        fx->wow_ph += 900u + (uint32_t)p->wow * 20u;
        fx->dly_half_toggle = (uint8_t)((fx->dly_half_toggle + 1u) % fx->dly_half_rate);

        switch (head_mode) {
        case KO_DFX_TAP_DUAL:
            dly_b = dly_s >> 1;
            break;
        case KO_DFX_TAP_GALLOP:
            dly_b = (dly_s * 3u) >> 2;
            break;
        case KO_DFX_TAP_TRIP:
            dly_b = (dly_s * 2u) / 3u;
            break;
        case KO_DFX_TAP_SPREAD:
            dly_b = dly_s + (uint32_t)fx->tap_b_ofs;
            break;
        case KO_DFX_TAP_PING:
            dly_b = dly_s >> 1;
            break;
        default:
            dly_b = dly_s;
            break;
        }
        if (dly_b < 8u)
            dly_b = 8u;
        if (dly_b > (KO_DFX_DLY_LEN - 64u))
            dly_b = KO_DFX_DLY_LEN - 64u;

        tap_a = ko_dfx_dly_read(fx->dly_w, dly_s, wow_smp);
        tap_b = ko_dfx_dly_read(fx->dly_w, dly_b, -wow_smp);
        if (head_mode == KO_DFX_TAP_SINGLE) {
            wet_l = tap_a;
            wet_r = tap_a;
        } else if (head_mode == KO_DFX_TAP_SPREAD) {
            wet_l = tap_a;
            wet_r = tap_b;
        } else if (head_mode == KO_DFX_TAP_PING) {
            wet_l = tap_b;
            wet_r = tap_a;
        } else {
            wet_l = tap_a + (tap_b >> 1);
            wet_r = (tap_a >> 1) + tap_b;
        }

        {
            int32_t fb_eff = (int32_t)p->feedback;

            if (fb_eff > 127)
                fb_eff = 127;
            if (fb_eff <= 106)
                fb_gain = (fb_eff * 32767) / 106;
            else
                fb_gain = 32767 + ((fb_eff - 106) * (29490 - 32767)) / 21;
            if (p->freeze)
                fb_gain = 29490;
        }

        if (!p->freeze) {
            int32_t path = tap_a;
            int32_t lf_tgt = 64 + (int32_t)p->tone;
            int32_t fb_eff = (int32_t)p->feedback;
            int32_t lf = fx->dly_lf_sm;
            int32_t ld = lf_tgt - lf;
            int32_t tilt;

            if (ld > 2)
                ld = 2;
            else if (ld < -2)
                ld = -2;
            lf += ld;
            fx->dly_lf_sm = lf;
            tilt = lf - 64;

            fx->dly_loop_dc += (path - fx->dly_loop_dc) >> 9;
            path -= fx->dly_loop_dc;
            if (tilt < 0) {
                int32_t lp_a = 24 + ((64 + tilt) * 200) / 64;

                fx->dly_lp += ((path - fx->dly_lp) * lp_a) >> 8;
                path = fx->dly_lp;
                if (fb_eff > 96 && fb_eff <= 106)
                    path = (path * (256 + ((-tilt) * 72) / 64)) >> 8;
            } else if (tilt > 0) {
                int32_t hp_a = 8 + (tilt * 180) / 63;

                fx->dly_hp += ((path - fx->dly_hp) * hp_a) >> 8;
                path = path - fx->dly_hp;
                if (fb_eff > 96 && fb_eff <= 106)
                    path = (path * (256 + (tilt * 72) / 63)) >> 8;
            } else {
                fx->dly_lp += (path - fx->dly_lp) >> 1;
                path = fx->dly_lp;
            }
            if (path > 32767)
                path = 32767;
            else if (path < -32767)
                path = -32767;
            path = dly_in + ((path * fb_gain) >> 15);
            if (path > 65000)
                path = 65000;
            else if (path < -65000)
                path = -65000;
            {
                int32_t clean = ko_dfx_softclip(path);

                if (fb_eff >= KO_DFX_FB_DIST_START) {
                    int32_t t = (fb_eff > 127 ? 127 : fb_eff) - KO_DFX_FB_DIST_START;
                    int32_t u = KO_DFX_FB_DIST_SPAN - t;
                    int32_t span_sq = KO_DFX_FB_DIST_SPAN * KO_DFX_FB_DIST_SPAN;
                    int32_t a = (KO_DFX_FB_DIST_AMT * (span_sq - u * u)) / span_sq;

                    path = clean + (((ko_dfx_echo_sat(path) - clean) * a) >> 15);
                    if (path > 32767)
                        path = 32767;
                    else if (path < -32767)
                        path = -32767;
                } else {
                    path = clean;
                }
            }
            if (fx->dly_half_toggle == 0u) {
                ko_dfx_dly_pool[fx->dly_w] = (int16_t)path;
                fx->dly_w = (fx->dly_w + 1u) % KO_DFX_DLY_LEN;
            }
        } else {
            int32_t path = (ko_dfx_softclip(tap_a) * fb_gain) >> 15;

            if (fx->dly_half_toggle == 0u) {
                ko_dfx_dly_pool[fx->dly_w] = (int16_t)ko_dfx_softclip(path);
                fx->dly_w = (fx->dly_w + 1u) % KO_DFX_DLY_LEN;
            }
        }

        if (p->dly_time != 0u || p->freeze) {
            dly_l = (wet_l * (int32_t)p->dly_lvl) >> 7;
            dly_r = (wet_r * (int32_t)p->dly_lvl) >> 7;
            rev_send += (((dly_l + dly_r) >> 2) * (int32_t)p->bleed) >> 7;
        }
    }

    {
        int32_t rev_wet;
        int32_t rev_in = rev_send;
        int32_t damp = 248 - ((int32_t)p->rev_damp * 180) / 127;
        int32_t tail_g = 110 + ((int32_t)p->rev_tail * 130) / 127;

        if (p->tank < 64u) {
            rev_in = ko_dfx_ap(KO_DFX_CH0_OFF, KO_DFX_CH0_LEN, &fx->rev_ch_pos[0], rev_in);
            rev_in = ko_dfx_ap(KO_DFX_CH1_OFF, KO_DFX_CH1_LEN, &fx->rev_ch_pos[1], rev_in);
            rev_in = ko_dfx_ap(KO_DFX_CH2_OFF, KO_DFX_CH2_LEN, &fx->rev_ch_pos[2], rev_in);
            rev_wet = ko_dfx_tank(KO_DFX_T0_OFF, KO_DFX_T0_LEN, &fx->rev_t_pos[0], rev_in, &fx->rev_t_lp[0], damp);
        } else {
            int32_t t0, t1, t2, t3;

            rev_in = ko_dfx_ap(KO_DFX_AP0_OFF, KO_DFX_AP0_LEN, &fx->rev_ap_pos[0], rev_in);
            rev_in = ko_dfx_ap(KO_DFX_AP1_OFF, KO_DFX_AP1_LEN, &fx->rev_ap_pos[1], rev_in);
            rev_in = ko_dfx_ap(KO_DFX_AP2_OFF, KO_DFX_AP2_LEN, &fx->rev_ap_pos[2], rev_in);
            rev_in = ko_dfx_ap(KO_DFX_AP3_OFF, KO_DFX_AP3_LEN, &fx->rev_ap_pos[3], rev_in);
            t0 = ko_dfx_tank(KO_DFX_T0_OFF, KO_DFX_T0_LEN, &fx->rev_t_pos[0], rev_in, &fx->rev_t_lp[0], damp);
            t1 = ko_dfx_tank(KO_DFX_T1_OFF, KO_DFX_T1_LEN, &fx->rev_t_pos[1], rev_in + ((t0 * tail_g) >> 8),
                             &fx->rev_t_lp[1], damp);
            t2 = ko_dfx_tank(KO_DFX_T2_OFF, KO_DFX_T2_LEN, &fx->rev_t_pos[2], rev_in + ((t1 * tail_g) >> 8),
                             &fx->rev_t_lp[2], damp);
            t3 = ko_dfx_tank(KO_DFX_T3_OFF, KO_DFX_T3_LEN, &fx->rev_t_pos[3], rev_in + ((t2 * tail_g) >> 8),
                             &fx->rev_t_lp[3], damp);
            rev_wet = (t0 + t1 + t2 + t3) >> 2;
        }
        rev_wet = (rev_wet * (int32_t)p->rev_mix) >> 7;
        if (rev_out)
            *rev_out = rev_wet;
        else {
            dly_l += rev_wet;
            dly_r += rev_wet;
        }
    }
    *wl += dly_l;
    *wr += dly_r;
}

/* Master isolator + soft limiter, in place. */
static inline void ko_dfx_master(ko_dfx_t *fx, int8_t iso_in, int32_t *l, int32_t *r)
{
    int32_t iso = iso_in;
    int32_t sl = *l;
    int32_t sr = *r;

    if (iso < -64)
        iso = -64;
    else if (iso > 63)
        iso = 63;
    if (iso < 0) {
        int32_t c = 10 + ((64 + iso) * 54) / 64;

        fx->iso_lp += ((sl - fx->iso_lp) * c) >> 6;
        fx->iso_lp_r += ((sr - fx->iso_lp_r) * c) >> 6;
        sl = ((fx->iso_lp * 3) + sl) >> 2;
        sr = ((fx->iso_lp_r * 3) + sr) >> 2;
    } else if (iso > 0) {
        int32_t c = 1 + (iso * 17) / 63;

        fx->iso_hp += ((sl - fx->iso_hp) * c) >> 6;
        fx->iso_hp_r += ((sr - fx->iso_hp_r) * c) >> 6;
        sl = sl - ((fx->iso_hp * 3) >> 2);
        sr = sr - ((fx->iso_hp_r * 3) >> 2);
    }
    *l = ko_dfx_master_soft(sl);
    *r = ko_dfx_master_soft(sr);
}

#endif /* KO_DUBFX_H */
