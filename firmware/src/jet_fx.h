/* SPDX-License-Identifier: GPL-3.0-only */
/* KO JET master FX: KO BASS dub bus (ko_dubfx.h) + JET extras — LOCUT on the reverb send, SPIN (rotating
 * reverb image), WIDTH (M/S). */
#ifndef JET_FX_H
#define JET_FX_H

#include <stdint.h>
#include "jet_patch.h"
#include "ko_dubfx.h"

#ifndef JET_POOL_SECTION
#define JET_POOL_SECTION KO_POOL_SECTION
#endif

typedef struct {
    ko_dfx_params_t dfx;
    int32_t locut_a;   /* reverb-send HP one-pole, Q8; 0 = off */
    int32_t spin_q15;  /* reverb rotation depth */
    uint32_t spin_inc;
    int32_t width_q15; /* side gain, 0 = untouched */
} jet_fx_coef_t;

typedef struct {
    ko_dfx_t dfx;
    int32_t locut_lp;
    uint32_t spin_ph;
} jet_fx_t;

static inline void jet_fx_coef_build(jet_fx_coef_t *c, const jet_patch_t *p, uint16_t bpm)
{
    ko_dfx_params_t *d;

    if (!c || !p)
        return;
    d = &c->dfx;
    d->dly_lvl = p->dly_lvl;
    d->dly_time = p->dly_time;
    d->feedback = p->dly_fb;
    d->tone = p->dly_tone;
    d->wow = p->wow;
    d->tap = p->tap;
    d->tank = p->tank ? 127u : 0u;
    d->iso = p->iso;
    d->rev_mix = p->rev_mix;
    d->rev_damp = p->rev_hi;
    d->rev_tail = p->rev_size;
    d->bleed = p->dly_rev;
    d->bpm = (uint8_t)(bpm > 255u ? 255u : bpm);
    d->tape_oct = 0;
    d->freeze = 0u;
    c->locut_a = p->rev_lo ? 1 + ((int32_t)p->rev_lo * (int32_t)p->rev_lo) / 500 : 0;
    c->spin_q15 = ((int32_t)p->rev_spin * 32767) >> 7;
    c->spin_inc = 6000u + (uint32_t)p->rev_spin * 900u;
    c->width_q15 = ((int32_t)p->width - 64) * 512;
}

static inline void jet_fx_init(jet_fx_t *fx)
{
    if (!fx)
        return;
    ko_dfx_init(&fx->dfx);
    fx->locut_lp = 0;
    fx->spin_ph = 0u;
}

static inline void jet_fx_render(jet_fx_t *fx, const jet_fx_coef_t *c, int32_t *dry_l, int32_t *dry_r,
                                 uint32_t n)
{
    uint32_t i;

    if (!fx || !c || !dry_l || !dry_r || n == 0u)
        return;
    ko_dfx_block_begin(&fx->dfx, &c->dfx);
    for (i = 0u; i < n; i++) {
        int32_t mid = (dry_l[i] + dry_r[i]) >> 1;
        int32_t side = (dry_l[i] - dry_r[i]) >> 1;
        int32_t rev_send = mid >> 1;
        int32_t rev = 0;
        int32_t out_l, out_r;

        if (c->locut_a) {
            fx->locut_lp += ((rev_send - fx->locut_lp) * c->locut_a) >> 8;
            rev_send -= fx->locut_lp;
        }
        side += (side * c->width_q15) >> 15;
        out_l = mid + side;
        out_r = mid - side;
        ko_dfx_frame(&fx->dfx, &c->dfx, mid, rev_send, &out_l, &out_r, &rev);
        if (c->spin_q15 > 0) {
            int32_t s = (fm_lfo_sine_q15(fx->spin_ph) * c->spin_q15) >> 15;

            fx->spin_ph += c->spin_inc;
            out_l += rev + ((rev * s) >> 15);
            out_r += rev - ((rev * s) >> 15);
        } else {
            out_l += rev;
            out_r += rev;
        }
        ko_dfx_master(&fx->dfx, c->dfx.iso, &out_l, &out_r);
        dry_l[i] = out_l;
        dry_r[i] = out_r;
    }
}

#endif /* JET_FX_H */
