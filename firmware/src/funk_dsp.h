/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FUNK_DSP_H
#define FUNK_DSP_H

#include "funk_data.h"
#include "dub_fm.h"
#include "jazz_pluck.h"

static const int8_t funk_kit_k_note[DKIT_N]  = { 34, 36, 32, 38, 37, 30 };
static const uint8_t funk_kit_k_sweep[DKIT_N] = { 12, 28, 16, 24, 20, 10 };
static const uint8_t funk_kit_k_fm[DKIT_N]   = {  4, 18,  8, 22, 16,  2 };
static const uint8_t funk_kit_k_drv[DKIT_N]  = { 16, 20, 14, 18, 22, 12 };
static const int8_t funk_kit_r_note[DKIT_N]  = { 76, 62, 68, 71, 63, 58 };
static const uint8_t funk_kit_r_wire[DKIT_N] = { 88, 82, 86, 92, 90, 72 };
static inline int32_t funk_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                        int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;

    (void)lfo_mod_b;
    (void)wob_all;
    switch (model) {
    case BASS_FINGER: {
        int32_t body, click;
        s->sub_ph += inc;
        /* High env: saw + click. Fallen env: sine mute (chicken). Caller applies sub_env. */
        if (s->sub_env > 18000)
            body = dub_saw(s->sub_ph);
        else
            body = dub_sine_q64(s->sub_ph);
        click = (s->sub_env > 26000) ? (dub_square(s->sub_ph << 2) >> 3) : 0;
        raw = body + click;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_SLAP: {
        int32_t thumb, pop;
        s->sub_ph += inc;
        thumb = dub_sine_q64(s->sub_ph);
        pop = (s->sub_env > 22000) ? dub_sine_q64(s->sub_ph << 1) >> 1 : 0;
        raw = thumb + pop;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_OCT: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_saw(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_EP: {
        int32_t idx = 8 + (int32_t)p->cutoff_b / 8;
        if (idx > 20)
            idx = 20;
        s->sub_ph += inc;
        raw = dub_fm_ep_tine(s->sub_ph, s->sub_ph, idx);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    default:
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    return raw;
}

static inline uint32_t funk_stab_bent_inc(const dub_synth_t *s, uint32_t inc)
{
    uint32_t n = s->siren_note_age;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        uint32_t b_smp = (DUB_FS * 75u) / 1000u;
        uint32_t depth = (uint32_t)s->siren_hp_bend;
        uint32_t abs_semi = (uint32_t)(semi >= 0 ? semi : -semi);
        uint32_t bend_q16 = abs_semi * 3800u * depth / 127u;

        if (bend_q16 > 0u && n < b_smp) {
            if (semi > 0)
                inc = (uint32_t)(((uint64_t)inc * (65536u - bend_q16 + (bend_q16 * n) / b_smp)) >> 16);
            else
                inc = (uint32_t)(((uint64_t)inc * (65536u + bend_q16 - (bend_q16 * n) / b_smp)) >> 16);
        }
    }
    return inc;
}

/* Short gated stab — horn family (jazz_lead_voice_osc / dub_fm_brass), era-specific body. */
static inline void funk_stab_pluck_trig(dub_synth_t *s, const dub_params_t *p, uint32_t inc, uint8_t bright)
{
    uint8_t dec = p->drm_dec[3] ? p->drm_dec[3] : 64u;

    jazz_pluck_trig(inc, dec, bright, &s->noise, 1u);
}

static inline int32_t funk_stab_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, ch_base, root_m, snap_r, bright, raw;
    uint8_t era = p->era % GENRE_ERA_N;
    uint32_t inc, ph;

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 24 / 64;
    ch_base = (int32_t)(s->ch_root ? s->ch_root : (p->root ? p->root : 40u));
    root_m = ch_base + 12 + kt_s + (int32_t)s->siren_st_tune;
    if (root_m < 52)
        root_m = 52;
    else if (root_m > 76)
        root_m = 76;
    snap_r = (int32_t)p->siren_rate;
    bright = 34 + ((snap_r * 44) / 127);
    {
        int32_t l_timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;

        bright += l_timb >> 2;
    }
    if (bright < 14)
        bright = 14;
    else if (bright > 96)
        bright = 96;
    inc = dub_note_inc((uint8_t)dub_clamp_midi(root_m));
    inc = funk_stab_bent_inc(s, inc);
    ph = s->stab_ph[0];
    if (era == ERA_BROKEN) {
        if (s->siren_note_age == 0u)
            funk_stab_pluck_trig(s, p, inc, (uint8_t)bright);
        s->stab_ph[0] += inc;
        raw = jazz_pluck_step(inc) + (dub_sine_q64(s->stab_ph[0]) >> 3);
    } else {
        int32_t ix;

        s->stab_ph[0] += inc;
        ph = s->stab_ph[0];
        switch (era) {
        case ERA_POCKET:
            ix = 12 + (bright >> 2);
            raw = dub_fm_brass(ph, ph, ix);
            break;
        case ERA_CLAV:
            ix = 18 + (bright >> 1);
            raw = dub_fm_brass(ph, ph, ix) + (dub_saw(ph) >> 3);
            break;
        case ERA_SLAP: {
            int32_t sawv = dub_saw(ph) >> 1;

            raw = sawv + (dub_sine_q64(ph) >> 2);
            break;
        }
        case ERA_DISCO:
            ix = 24 + (bright >> 1);
            raw = dub_fm_brass(ph, ph, ix) + (dub_saw(ph) >> 4);
            break;
        case ERA_BOOGIE:
            ix = 26 + bright;
            if (ix > 52)
                ix = 52;
            raw = dub_fm_brass(ph, ph, ix);
            break;
        default:
            ix = 14 + (bright >> 2);
            raw = dub_fm_brass(ph, ph, ix);
            break;
        }
    }
    return (raw * env) >> 15;
}

static inline uint16_t funk_siren_atk_samples(uint8_t era, uint8_t atk)
{
    uint16_t a;

    (void)era;
    if (atk)
        a = (uint16_t)(22u + ((uint32_t)atk * 28u) / 127u);
    else
        a = 44u;
    if (a > 110u)
        a = 110u;
    return a;
}

static inline void funk_stab_note_gate(uint8_t trig_cnt, int8_t lane_gate, uint16_t *hold_out,
                                        uint16_t *rel_out, uint8_t len_plock_on, uint8_t len_ix,
                                        uint32_t samples_per_step)
{
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;

    (void)trig_cnt;
    if (len_plock_on != 0u && samples_per_step > 0u) {
        total = funk_stab_gate_len_samples(samples_per_step, len_ix);
        if (total < 1u)
            total = 1u;
    } else {
        total = (DUB_FS * 16u) / 100u;
        if (g < 0)
            g = -g;
        total += (uint32_t)g * (DUB_FS / 600u);
    }
    hold = (total * 35u) / 100u;
    if (hold > 65500u)
        hold = 65500u;
    if (total > hold + 65535u)
        total = hold + 65535u;
    *hold_out = (uint16_t)hold;
    *rel_out = (uint16_t)(total - hold);
}

/* Closed hat: noise-forward body; kit picks partials (not shared 4× square stack). */
static inline int32_t funk_hat_render(dub_synth_t *s, int32_t hat_base, uint8_t kit,
                                      int32_t nz_gain, int32_t met_w)
{
    static const int8_t funk_hat_offs[DKIT_N][4] = {
        {  0, 12, 19, 26 },
        {  0,  9, 17, 24 },
        {  0,  6, 11, 16 },
        {  0,  5, 10, 15 },
        {  0,  4,  9, 14 },
        {  0, -9,  4, 11 },
    };
    /* DRY (932 Hz) and DUST (349 Hz–1.1 kHz) partials sat in cowbell range */
    static const int8_t funk_hat_lift[DKIT_N] = { 12, 0, 0, 0, 0, 18 };
    int32_t hm = 0;
    uint8_t h;
    uint8_t h_style = funk_hat_style[kit];
    const int8_t *offs = funk_hat_offs[kit];

    for (h = 0; h < 4u; h++) {
        s->hat_ph[h] += dub_note_inc(dub_clamp_midi(hat_base + funk_hat_lift[kit] + offs[h]));
        {
            uint32_t ph = s->hat_ph[h];

            switch (kit) {
            case DKIT_DRY:
                if (h == 0u)
                    hm += (dub_sine_q64(ph) >> 1) + (dub_square(ph << 3) >> 8);
                else
                    hm += dub_sine_q64(ph) >> (1 + h);
                break;
            case DKIT_CLAV: {
                int32_t partial = dub_sine_q64(ph) >> 1;

                if (h == 0u)
                    partial += (dub_sine_q64(ph << 1) * (int32_t)funk_hat_fm[kit]) >> 13;
                partial += dub_square(ph >> 1) >> (4 + h);
                hm += partial;
                break;
            }
            case DKIT_SLAP:
                if (h < 2u)
                    hm += dub_sine_q64(ph) >> 1;
                else
                    hm += (dub_sine_q64(ph) * (int32_t)funk_hat_fm[kit]) >> 14;
                break;
            case DKIT_DISCO: {
                int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * (int32_t)funk_hat_fm[kit];

                hm += dub_sine_q64(ph + ((uint32_t)hfm << 11));
                if (h == 0u)
                    hm += dub_sine_q64(ph << 1) >> 3;
                break;
            }
            case DKIT_BOOG:
                hm += dub_sine_q64(ph) >> 2;
                if (funk_hat_fm[kit] > 16u) {
                    int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * ((int32_t)funk_hat_fm[kit] >> 1);

                    hm += dub_sine_q64(ph + ((uint32_t)hfm << 10)) >> 1;
                }
                break;
            case DKIT_DUST:
                hm += dub_sine_q64(ph) >> 1;
                if (h & 1u)
                    hm += dub_sine_q64(ph + (ph >> 3)) >> 3;
                break;
            default:
                hm += dub_sine_q64(ph) >> 2;
                break;
            }
        }
    }
    hm >>= 2;
    if (h_style == HAT_STYLE_TR6)
        hm &= ~63;
    if (h_style == HAT_STYLE_HISS)
        hm >>= 1;
    {
        int32_t cut_b = ((int32_t)funk_hat_cut[kit] - 64) >> 1;
        int32_t tone_w = met_w + (cut_b >> 1);
        int32_t nz_sh = 9;
        int32_t h_nz;
        int32_t sq;

        if (tone_w < 16)
            tone_w = 16;
        else if (tone_w > 127)
            tone_w = 127;
        if (h_style == HAT_STYLE_TICK)
            nz_sh = 10;
        h_nz = (dub_noise(&s->noise) * nz_gain) >> nz_sh;
        sq = dub_square(s->hat_ph[0]) >> 8;
        h_nz = (h_nz * (sq + 112)) >> 7;
        if (s->hat_env > 0)
            h_nz = (h_nz * (88 + (s->hat_env >> 8))) >> 7;
        if (kit == DKIT_DUST) {
            int32_t ash = dub_sine_q64(s->hat_ph[0] + (s->hat_ph[2] >> 2)) >> 8;

            h_nz = (h_nz * (96 + ash)) >> 7;
        }
        hm = ((hm * tone_w) >> 7) + h_nz;
        /* DUST: dark cut + low noise/metal sat ~14 dB under the other kits */
        if (kit == DKIT_DUST)
            hm = (hm * 21) >> 2;
    }
    return hm;
}

static inline void funk_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    if ((s->siren_atk_left > 0u || s->siren_gate > 0u || s->siren_lead_rel > 0u || s->siren_env > 0)
        && s->siren_note_age < 65535u)
        s->siren_note_age++;

    if (s->siren_atk_left > 0u) {
        int32_t pk = s->siren_peak;
        int32_t diff = pk - s->siren_env;
        int32_t step = diff;

        if (step > pk >> 3)
            step = pk >> 3;
        if (step < 1)
            step = 1;
        s->siren_env += step;
        s->siren_atk_left--;
        if (s->siren_atk_left == 0u || s->siren_env >= pk)
            s->siren_env = pk;
        return;
    }
    if (s->siren_gate > 0u) {
        s->siren_gate--;
    } else if (s->siren_lead_rel > 0u) {
        uint8_t dec_k = p->drm_dec[3] ? p->drm_dec[3] : 64u;
        int32_t sh = 12 + (((int32_t)dec_k - 64) >> 5);

        if (sh < 11)
            sh = 11;
        else if (sh > 13)
            sh = 13;
        s->siren_env -= (s->siren_env >> sh) + (sh <= 11 ? 2 : 1);
        if (s->siren_env < 0)
            s->siren_env = 0;
        s->siren_lead_rel--;
    } else if (s->siren_env > 0) {
        s->siren_env -= (s->siren_env >> 10) + 1;
        if (s->siren_env < 0)
            s->siren_env = 0;
    }
}

#endif
