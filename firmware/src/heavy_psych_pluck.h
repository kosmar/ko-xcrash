/* SPDX-License-Identifier: MIT */
/*
 * Karplus-Strong pluck, algorithm after DaisySP pluck (MIT, Copyright (c) 2020 Electrosmith).
 * Re-implemented in fixed point for FM1.
 *
 * MIT License
 *
 * Copyright (c) 2020 Electrosmith, Corp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef HEAVY_PSYCH_PLUCK_H
#define HEAVY_PSYCH_PLUCK_H

#include <stdint.h>

#define HP_PLUCK_MAX_LEN 512u
#define HP_PLUCK_MASK    (HP_PLUCK_MAX_LEN - 1u)

/* Single-voice Karplus-Strong pluck state (~1.04 KiB total, well under 4 KiB budget). */
typedef struct {
    int16_t buf[HP_PLUCK_MAX_LEN];
    uint32_t ph_q16;
    uint32_t period_q16;
    uint16_t excite_len;
    int32_t damp_state;
    int32_t out_lp;
    int16_t fb_q15;
    uint8_t damp_a_q8;
} heavy_psych_pluck_t;

static heavy_psych_pluck_t hp_lead_pluck;

/* Output level scale for pluck core only (lands pluck lead at ~-4.5 dB under chord RMS). */
#define HP_PLUCK_OUT_GAIN_Q8 210

static inline int32_t heavy_psych_pluck_scale_out(int32_t sig)
{
    return (sig * HP_PLUCK_OUT_GAIN_Q8) >> 8;
}

/* Fixed-point feedback gain: exp(-6.907 / (hz * (0.05 + 7.95 * decay))) in Q15.
 * Uses 2nd-order Padé/Taylor exp(-x) ≈ 1 - x + x^2/2 for small x > 0. */
static inline int16_t heavy_psych_pluck_calc_fb(uint32_t inc, uint8_t decay_u7)
{
    /* hz ≈ inc * 44100 / 2^32. In Q4: hz_q4 = (inc * 705600) >> 32 ≈ (inc * 11) >> 16 */
    uint32_t hz_q4 = (uint32_t)(((uint64_t)inc * 705600u) >> 32);
    /* tau_ms = 50 + 7950 * (decay_u7 / 127) in ms */
    uint32_t tau_ms = 50u + ((uint32_t)decay_u7 * 7950u) / 127u;
    /* denom = hz * tau_s in Q4: (hz_q4 * tau_ms) / 1000 */
    uint32_t denom_q4 = (hz_q4 * tau_ms) / 1000u;
    uint32_t x_q15;
    int32_t fb;

    if (denom_q4 < 16u)
        denom_q4 = 16u;
    /* 6.907 in Q19 is 3621250; x_q15 = (6.907 * 2^19) / denom_q4 */
    x_q15 = 3621250u / denom_q4;
    /* Keep feedback musical for sustained psych lead notes (min ~0.985 = 32276 in Q15) */
    if (x_q15 > 490u)
        x_q15 = 490u;
    fb = 32767 - (int32_t)x_q15 + (int32_t)((x_q15 * x_q15) >> 16);
    if (fb < 32100)
        fb = 32100;
    else if (fb > 32756)
        fb = 32756;
    return (int16_t)fb;
}

/* Period in Q16 samples: DUB_FS / hz = 2^32 / inc (in Q16: 2^48 / inc), clamped to [8, 510] samples. */
static inline uint32_t heavy_psych_pluck_period_q16(uint32_t inc)
{
    uint32_t per_q16;

    if (inc < (1u << 23))
        inc = (1u << 23);
    /* Avoid 64-bit division in ISR: 2^48 / inc = (2^31 / (inc >> 10)) << 7 */
    per_q16 = (0x80000000u / (inc >> 10)) << 7;
    if (per_q16 < (8u << 16))
        per_q16 = (8u << 16);
    else if (per_q16 > ((HP_PLUCK_MAX_LEN - 2u) << 16))
        per_q16 = (HP_PLUCK_MAX_LEN - 2u) << 16;
    return per_q16;
}

/* Trigger / legato retrigger: fill delay line of length = period with brightness-filtered zero-mean noise burst. */
static inline void heavy_psych_pluck_trig(uint32_t inc, uint8_t decay_u7, uint8_t bright_u7, uint32_t *rng,
                                          uint8_t stoner_pick)
{
    uint32_t per_q16 = heavy_psych_pluck_period_q16(inc);
    uint16_t len = (uint16_t)(per_q16 >> 16);
    /* Reduced brightness one-pole (a_q8 = 6..18) so noise burst excites fewer 600-1500 Hz mid harmonics */
    int32_t a_q8 = stoner_pick ? (14 + (((int32_t)bright_u7 * 18) / 127))
                               : (6 + (((int32_t)bright_u7 * 12) / 127));
    int32_t lp = 0;
    int32_t sum = 0;
    int32_t mean;
    uint16_t half;
    uint16_t i;

    if (len < 8u)
        len = 8u;
    else if (len > HP_PLUCK_MAX_LEN)
        len = HP_PLUCK_MAX_LEN;
    if (a_q8 > 32)
        a_q8 = 32;
    half = (uint16_t)(len >> 1);
    if (half < 1u)
        half = 1u;

    for (i = 0u; i < len; i++) {
        int32_t n, tri, exc;

        *rng = (*rng * 1664525u) + 1013904223u;
        n = (int32_t)(int16_t)(*rng >> 16);
        lp += ((n - lp) * a_q8) >> 8;
        /* Fundamental-period triangle displacement (avoids 2nd-harmonic mid spike) + warm noise burst */
        if (i < half)
            tri = -16384 + ((int32_t)i * 32768) / (int32_t)half;
        else
            tri = 16384 - ((int32_t)(i - half) * 32768) / (int32_t)(len - half ? len - half : 1u);
        exc = lp + (stoner_pick ? ((tri * 3) >> 2) : (tri >> 1));
        hp_lead_pluck.buf[i] = (int16_t)exc;
        sum += exc;
    }
    mean = sum / (int32_t)len;
    for (i = 0u; i < len; i++) {
        int32_t v = (int32_t)hp_lead_pluck.buf[i] - mean;

        if (v > 32767)
            v = 32767;
        else if (v < -32767)
            v = -32767;
        hp_lead_pluck.buf[i] = (int16_t)v;
    }
    for (; i < HP_PLUCK_MAX_LEN; i++)
        hp_lead_pluck.buf[i] = 0;

    hp_lead_pluck.ph_q16 = 0u;
    hp_lead_pluck.period_q16 = per_q16;
    hp_lead_pluck.excite_len = len;
    hp_lead_pluck.damp_state = 0;
    hp_lead_pluck.out_lp = 0;
    hp_lead_pluck.fb_q15 = heavy_psych_pluck_calc_fb(inc, decay_u7);
    hp_lead_pluck.damp_a_q8 = stoner_pick
        ? (uint8_t)(52u + (((uint32_t)bright_u7 * 72u) / 127u))
        : (uint8_t)(40u + (((uint32_t)bright_u7 * 56u) / 127u));
}

/* Step Karplus-Strong delay line at current instantaneous pitch `inc` (retunes period for bends/vibrato). */
static inline int32_t heavy_psych_pluck_step(uint32_t inc)
{
    uint32_t per_q16 = heavy_psych_pluck_period_q16(inc);
    uint32_t len_int = per_q16 >> 16;
    uint32_t w_idx, r_idx0, r_idx1, frac;
    int32_t s0, s1, rd, avg, damped, wr;

    if (len_int < 8u)
        len_int = 8u;
    else if (len_int > HP_PLUCK_MAX_LEN - 2u)
        len_int = HP_PLUCK_MAX_LEN - 2u;

    w_idx = (hp_lead_pluck.ph_q16 >> 16) & HP_PLUCK_MASK;
    r_idx0 = (w_idx + 1u) % len_int;
    r_idx1 = (r_idx0 + 1u) % len_int;
    frac = (hp_lead_pluck.ph_q16 >> 8) & 255u;

    s0 = hp_lead_pluck.buf[r_idx0];
    s1 = hp_lead_pluck.buf[r_idx1];
    rd = s0 + (((s1 - s0) * (int32_t)frac) >> 8);

    /* Two-point KS averaging + brightness one-pole damping filter */
    avg = (s0 + s1) >> 1;
    hp_lead_pluck.damp_state += ((avg - hp_lead_pluck.damp_state) * (int32_t)hp_lead_pluck.damp_a_q8) >> 8;
    damped = (avg + hp_lead_pluck.damp_state) >> 1;

    wr = (damped * (int32_t)hp_lead_pluck.fb_q15) >> 15;
    if (wr > 32767)
        wr = 32767;
    else if (wr < -32767)
        wr = -32767;
    hp_lead_pluck.buf[w_idx] = (int16_t)wr;

    hp_lead_pluck.ph_q16 += 65536u;
    if ((hp_lead_pluck.ph_q16 >> 16) >= len_int)
        hp_lead_pluck.ph_q16 &= 0xFFFFu;

    /* Post-pluck low-pass filter (~4 kHz roll-off, g_q8 = 112) */
    hp_lead_pluck.out_lp += ((rd - hp_lead_pluck.out_lp) * 88) >> 8;
    return (hp_lead_pluck.out_lp * 9) >> 3;
}

#endif /* HEAVY_PSYCH_PLUCK_H */
