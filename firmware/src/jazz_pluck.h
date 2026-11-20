/* SPDX-License-Identifier: MIT */
/*
 * Karplus-Strong pluck, algorithm after DaisySP pluck (MIT, Copyright (c) 2020 Electrosmith).
 * Re-implemented in fixed point for FM1 Jazz horn lane.
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
#ifndef JAZZ_PLUCK_H
#define JAZZ_PLUCK_H

#include <stdint.h>

#define JAZZ_PLUCK_MAX_LEN 512u
#define JAZZ_PLUCK_MASK    (JAZZ_PLUCK_MAX_LEN - 1u)

typedef struct {
    int16_t buf[JAZZ_PLUCK_MAX_LEN];
    uint32_t ph_q16;
    uint32_t period_q16;
    uint16_t excite_len;
    int32_t damp_state;
    int32_t out_lp;
    int16_t fb_q15;
    uint8_t damp_a_q8;
} jazz_pluck_t;

static jazz_pluck_t jazz_horn_pluck;

#define JAZZ_PLUCK_OUT_GAIN_Q8 186

static inline int32_t jazz_pluck_scale_out(int32_t sig)
{
    return (sig * JAZZ_PLUCK_OUT_GAIN_Q8) >> 8;
}

static inline int16_t jazz_pluck_calc_fb(uint32_t inc, uint8_t decay_u7)
{
    uint32_t hz_q4 = (uint32_t)(((uint64_t)inc * 705600u) >> 32);
    uint32_t tau_ms = 50u + ((uint32_t)decay_u7 * 7950u) / 127u;
    uint32_t denom_q4 = (hz_q4 * tau_ms) / 1000u;
    uint32_t x_q15;
    int32_t fb;

    if (denom_q4 < 16u)
        denom_q4 = 16u;
    x_q15 = 3621250u / denom_q4;
    if (x_q15 > 490u)
        x_q15 = 490u;
    fb = 32767 - (int32_t)x_q15 + (int32_t)((x_q15 * x_q15) >> 16);
    if (fb < 32100)
        fb = 32100;
    else if (fb > 32756)
        fb = 32756;
    return (int16_t)fb;
}

static inline uint32_t jazz_pluck_period_q16(uint32_t inc)
{
    uint32_t per_q16;

    if (inc < (1u << 23))
        inc = (1u << 23);
    per_q16 = (0x80000000u / (inc >> 10)) << 7;
    if (per_q16 < (8u << 16))
        per_q16 = (8u << 16);
    else if (per_q16 > ((JAZZ_PLUCK_MAX_LEN - 2u) << 16))
        per_q16 = (JAZZ_PLUCK_MAX_LEN - 2u) << 16;
    return per_q16;
}

static inline void jazz_pluck_trig(uint32_t inc, uint8_t decay_u7, uint8_t bright_u7, uint32_t *rng,
                                 uint8_t stoner_pick)
{
    uint32_t per_q16 = jazz_pluck_period_q16(inc);
    uint16_t len = (uint16_t)(per_q16 >> 16);
    int32_t a_q8 = stoner_pick ? (14 + (((int32_t)bright_u7 * 18) / 127))
                               : (6 + (((int32_t)bright_u7 * 12) / 127));
    int32_t lp = 0;
    int32_t sum = 0;
    int32_t mean;
    uint16_t half;
    uint16_t i;

    if (len < 8u)
        len = 8u;
    else if (len > JAZZ_PLUCK_MAX_LEN)
        len = JAZZ_PLUCK_MAX_LEN;
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
        if (i < half)
            tri = -16384 + ((int32_t)i * 32768) / (int32_t)half;
        else
            tri = 16384 - ((int32_t)(i - half) * 32768) / (int32_t)(len - half ? len - half : 1u);
        exc = lp + (stoner_pick ? ((tri * 3) >> 2) : (tri >> 1));
        jazz_horn_pluck.buf[i] = (int16_t)exc;
        sum += exc;
    }
    mean = sum / (int32_t)len;
    for (i = 0u; i < len; i++) {
        int32_t v = (int32_t)jazz_horn_pluck.buf[i] - mean;

        if (v > 32767)
            v = 32767;
        else if (v < -32767)
            v = -32767;
        jazz_horn_pluck.buf[i] = (int16_t)v;
    }
    for (; i < JAZZ_PLUCK_MAX_LEN; i++)
        jazz_horn_pluck.buf[i] = 0;

    jazz_horn_pluck.ph_q16 = 0u;
    jazz_horn_pluck.period_q16 = per_q16;
    jazz_horn_pluck.excite_len = len;
    jazz_horn_pluck.damp_state = 0;
    jazz_horn_pluck.out_lp = 0;
    jazz_horn_pluck.fb_q15 = jazz_pluck_calc_fb(inc, decay_u7);
    jazz_horn_pluck.damp_a_q8 = stoner_pick
        ? (uint8_t)(52u + (((uint32_t)bright_u7 * 72u) / 127u))
        : (uint8_t)(40u + (((uint32_t)bright_u7 * 56u) / 127u));
}

static inline int32_t jazz_pluck_step(uint32_t inc)
{
    uint32_t per_q16 = jazz_pluck_period_q16(inc);
    uint32_t len_int = per_q16 >> 16;
    uint32_t w_idx, r_idx0, r_idx1, frac;
    int32_t s0, s1, rd, avg, damped, wr;

    if (len_int < 8u)
        len_int = 8u;
    else if (len_int > JAZZ_PLUCK_MAX_LEN - 2u)
        len_int = JAZZ_PLUCK_MAX_LEN - 2u;

    w_idx = (jazz_horn_pluck.ph_q16 >> 16) & JAZZ_PLUCK_MASK;
    r_idx0 = (w_idx + 1u) % len_int;
    r_idx1 = (r_idx0 + 1u) % len_int;
    frac = (jazz_horn_pluck.ph_q16 >> 8) & 255u;

    s0 = jazz_horn_pluck.buf[r_idx0];
    s1 = jazz_horn_pluck.buf[r_idx1];
    rd = s0 + (((s1 - s0) * (int32_t)frac) >> 8);

    avg = (s0 + s1) >> 1;
    jazz_horn_pluck.damp_state += ((avg - jazz_horn_pluck.damp_state) * (int32_t)jazz_horn_pluck.damp_a_q8) >> 8;
    damped = (avg + jazz_horn_pluck.damp_state) >> 1;

    wr = (damped * (int32_t)jazz_horn_pluck.fb_q15) >> 15;
    if (wr > 32767)
        wr = 32767;
    else if (wr < -32767)
        wr = -32767;
    jazz_horn_pluck.buf[w_idx] = (int16_t)wr;

    jazz_horn_pluck.ph_q16 += 65536u;
    if ((jazz_horn_pluck.ph_q16 >> 16) >= len_int)
        jazz_horn_pluck.ph_q16 &= 0xFFFFu;

    jazz_horn_pluck.out_lp += ((rd - jazz_horn_pluck.out_lp) * 88) >> 8;
    return (jazz_horn_pluck.out_lp * 9) >> 3;
}

#endif /* JAZZ_PLUCK_H */
