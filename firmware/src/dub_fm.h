/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DUB_FM_H
#define DUB_FM_H

#include <stdint.h>

/* Integer DX-style FM helpers; dub_sine_q64 / dub_saw must be in scope (dub_dsp.h).
 * Callers advance ph / ph2 / ph3 (e.g. ph2 += inc<<1, ph3 += inc*3) before calling. */

static inline int32_t dub_fm_2op_fb(uint32_t ph, uint32_t ph2, int32_t *fb,
                                    int32_t index, int32_t fb_amt)
{
    int32_t mod2;

    mod2 = dub_sine_q64(ph2 + ((uint32_t)(*fb) << 15));
    *fb = (mod2 * fb_amt) >> 8;
    mod2 = (mod2 * index) >> 7;
    return dub_sine_q64(ph + ((uint32_t)mod2 << 15));
}

static inline int32_t dub_fm_3op(uint32_t ph, uint32_t ph2, uint32_t ph3, int32_t *fb,
                                 int32_t index, int32_t fb_amt, int32_t index3)
{
    int32_t mod2, mod3;

    mod2 = dub_sine_q64(ph2 + ((uint32_t)(*fb) << 15));
    *fb = (mod2 * fb_amt) >> 8;
    mod2 = (mod2 * index) >> 7;
    mod3 = (dub_sine_q64(ph3) * index3) >> 7;
    return dub_sine_q64(ph + ((uint32_t)(mod2 + mod3) << 15));
}

static inline int32_t dub_fm_ep_tine(uint32_t ph, uint32_t mod_ph, int32_t index)
{
    int32_t mod;

    mod = dub_sine_q64(mod_ph << 1);
    mod = (mod * index) >> 7;
    return dub_sine_q64(ph + ((uint32_t)mod << 15));
}

static inline int32_t dub_fm_brass(uint32_t ph, uint32_t mod_ph, int32_t index)
{
    int32_t mod, sawv, tone;

    mod = dub_sine_q64(mod_ph << 1);
    mod = (mod * index) >> 9;
    sawv = dub_saw(ph) >> 2;
    tone = dub_sine_q64(ph + ((uint32_t)mod << 13)) >> 2;
    return sawv + tone;
}

static inline int32_t dub_fm_4op_stack(uint32_t ph, uint32_t ph2, uint32_t ph3, uint32_t ph4,
                                      int32_t *fb, int32_t index, int32_t index3,
                                      int32_t index4, int32_t fb_amt)
{
    int32_t mod4, mod3, mod2;

    mod4 = dub_sine_q64(ph4 + ((uint32_t)(*fb) << 15));
    *fb = (mod4 * fb_amt) >> 8;
    mod4 = (mod4 * index4) >> 7;
    mod3 = dub_sine_q64(ph3 + ((uint32_t)mod4 << 15));
    mod3 = (mod3 * index3) >> 7;
    mod2 = dub_sine_q64(ph2 + ((uint32_t)mod3 << 15));
    mod2 = (mod2 * index) >> 7;
    return dub_sine_q64(ph + ((uint32_t)mod2 << 15));
}

#endif
