/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_DSP_H
#define BASS_DSP_H

#include "bass_data.h"


static const int8_t techno_kit_k_note[DKIT_N]  = { 31, 36, 33, 38, 40, 42, 35, 37 };
static const uint8_t techno_kit_k_sweep[DKIT_N] = {  8, 28, 12, 16, 20, 10, 12, 16 };
static const uint8_t techno_kit_k_fm[DKIT_N]   = {  2, 20,  6,  5, 14,  3,  5,  8 };
static const uint8_t techno_kit_k_drv[DKIT_N]  = { 14, 18, 12, 10, 16,  8, 12, 14 };
static const int8_t techno_kit_r_note[DKIT_N]  = { 74, 58, 66, 69, 61, 77, 56, 72 };
static const uint8_t techno_kit_r_wire[DKIT_N] = { 72, 62, 68, 70, 78, 58, 82, 66 };

enum {
    TECHNO_HAT_GLITCH_SWING = 0, TECHNO_HAT_GLITCH_BELL, TECHNO_HAT_GLITCH_WASH, TECHNO_HAT_GLITCH_ACID_HAT
};

static const int8_t techno_hat_glitch_note[DKIT_N] = {
    88, 90, 86, 92, 84, 80, 94, 96
};
static const uint8_t techno_hat_glitch_fm[DKIT_N] = {
    18, 24, 14, 20, 12,  8, 48, 62
};
static const uint8_t techno_hat_glitch_noise[DKIT_N] = {
    22, 28, 18, 24, 20, 16, 60, 52
};
static const uint8_t techno_hat_glitch_metal[DKIT_N] = {
    92, 88, 96, 90, 86, 78, 68, 72
};
static const uint8_t techno_hat_glitch_decay[DKIT_N] = {
    14, 16, 12, 15, 13, 18,  4,  5
};
static const uint8_t techno_hat_glitch_open[DKIT_N] = {
    10, 12,  8, 11,  9, 14,  2,  3
};
static const uint8_t techno_hat_glitch_style[DKIT_N] = {
    TECHNO_HAT_GLITCH_SWING, TECHNO_HAT_GLITCH_BELL, TECHNO_HAT_GLITCH_WASH, TECHNO_HAT_GLITCH_SWING,
    TECHNO_HAT_GLITCH_BELL, TECHNO_HAT_GLITCH_WASH, TECHNO_HAT_GLITCH_ACID_HAT, TECHNO_HAT_GLITCH_ACID_HAT
};
static const uint8_t techno_hat_glitch_cut[DKIT_N] = {
    48, 52, 44, 50, 46, 40, 46, 47
};

static const uint8_t techno_hat_decay[DKIT_N] = { 8, 18, 6, 3, 5, 4, 10, 2 };
static const uint8_t techno_hat_open[DKIT_N] = { 6, 22, 4, 2, 3, 2, 8, 1 };

static inline int techno_hat_glitch_is_acid_kit(uint8_t kit)
{
    return techno_hat_glitch_style[kit % DKIT_N] == (uint8_t)TECHNO_HAT_GLITCH_ACID_HAT;
}

static inline int32_t techno_drm_kick_shape(int32_t kick_sig, uint16_t kick_snap, int32_t k_drv_tot,
                                          uint32_t *noise)
{
    /* Tiny transient only — shared PUNCH path owns beater; avoid double click. */
    if (kick_snap > 150u && noise)
        kick_sig += (dub_noise(noise) * (4 + (k_drv_tot >> 3))) >> 9;
    return dub_softclip_q15((kick_sig * (272 + k_drv_tot * 3)) >> 8);
}

static inline int32_t techno_drm_rim_mix(int32_t shell, int32_t wire, uint8_t kit_wire, uint8_t rim_drv)
{
    int32_t mix_r = ((shell * (40 - (kit_wire >> 4))) >> 7) + ((wire * 13) >> 3);

    return dub_softclip_q15((mix_r * (272 + (int32_t)rim_drv * 2)) >> 8);
}

static inline int32_t techno_drm_bus(int32_t drum_sig, uint8_t kit_drive)
{
    int32_t gain = 256 + ((int32_t)kit_drive * 2);

    drum_sig = (drum_sig * gain) >> 8;
    return dub_softclip_q15(drum_sig);
}

static inline int32_t techno_drm_room_send(int32_t kick_sig, int32_t rim_sig, int32_t hat_glitch_sig)
{
    (void)kick_sig;
    return ((rim_sig * 10) >> 7) + ((hat_glitch_sig * 8) >> 7);
}

static inline int32_t bass_bass_house_sub(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t raw;

    s->sub_ph += inc;
    raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
    dub_svf_mode(raw, 600 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t bass_hat_glitch_is_acid_kit(uint8_t kit)
{
    return kit == (uint8_t)DKIT_ACID;
}

static inline int32_t bass_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;
    int32_t walk = lfo_mod_b;
    uint32_t pwm_ph;
    int32_t fold_in;

    switch (model) {
    case BASS_RUMBLE: {
        s->sub_ph += inc >> 1;
        s->sub_ph2 += inc >> 2;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph2) >> 2);
        dub_svf_mode(raw, 280 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)(128u - (p->pump >> 1))) >> 7;
        break;
    }
    case BASS_SUB909: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, 1100 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_EBM16: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_square(s->sub_ph) >> 2);
        dub_svf_mode(raw, fc + (walk >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 12u), &raw);
        /* amp env is applied downstream (bass_dahdsr); fixed drive gain only */
        raw = (raw * 3) >> 1;
        break;
    }
    case BASS_HOOVER: {
        uint32_t det = inc >> 4;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc - det;
        raw = dub_saw(s->sub_ph) + dub_saw(s->sub_ph2) + dub_saw(s->sub_ph3);
        dub_svf_mode(raw, fc + (wob_all >> 9), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 18u), &raw);
        break;
    }
    case BASS_ACID303: {
        int32_t saw_w, acid_fc;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        raw = saw_w + (dub_square(s->sub_ph) >> 1);
        acid_fc = fc + (((s->sub_env >> 3) * (int32_t)p->env_filt) >> 5);
        dub_svf_mode(raw, acid_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 12u > 127u ? 127u : p->reso_mode + 12u), &raw);
        break;
    }
    case BASS_DETROIT: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc + (inc >> 2);
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + ((uint32_t)mod2 << 12));
        dub_svf_mode(raw, fc + (walk >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * 3) >> 2;
        break;
    }
    case BASS_SCHRANZ: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_sine_q64(s->sub_ph) >> 2);
        dub_svf_mode(raw, 500 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 8u), &raw);
        raw = (raw * (int32_t)(96u + (p->drive >> 1))) >> 7;
        break;
    }
    case BASS_REESE: {
        uint32_t det = (inc >> 5) + (uint32_t)(wob_all >> 11);
        int32_t s1, s2, s3;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc - det;
        s1 = dub_saw(s->sub_ph);
        s2 = dub_saw(s->sub_ph2);
        s3 = dub_saw(s->sub_ph3);
        raw = (s1 + s2 + s3) / 3;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_MODFM: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + ((uint32_t)mod2 << 13));
        dub_svf_mode(raw, fc + ((int32_t)p->wobble * 6), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_DRONE: {
        /* −1 oct; WOBBLE detunes the second sine up to +27 cents (slow beating) */
        s->sub_ph += inc >> 1;
        s->sub_ph2 += (inc >> 1) + (((inc >> 10) * (uint32_t)p->wobble) >> 4);
        raw = dub_sine_q64(s->sub_ph) >> 1;
        raw += dub_sine_q64(s->sub_ph2) >> 2;
        dub_svf_mode(raw, fc >> 1, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode >> 1, &raw);
        break;
    }
    case BASS_FOLD: {
        s->sub_ph += inc;
        fold_in = dub_saw(s->sub_ph);
        fold_in = (fold_in * (int32_t)(96u + p->drive)) >> 7;
        raw = dub_softclip_q15(fold_in);
        raw += dub_softclip_q15((fold_in * 3) >> 2);
        dub_svf_mode(raw, fc + (walk >> 3), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_SYNC: {
        /* hard sync: slave (1×…3×, WOBBLE) restarts on each master wrap */
        uint32_t sync_inc = inc + (uint32_t)(((uint64_t)inc * p->wobble) >> 6);
        uint32_t m_prev = s->sub_ph2;

        s->sub_ph2 += inc;
        if (s->sub_ph2 < m_prev && inc > 0u)
            s->sub_ph = (uint32_t)(((uint64_t)s->sub_ph2 * sync_inc) / inc);
        else
            s->sub_ph += sync_inc;
        raw = dub_saw(s->sub_ph) - (dub_saw(s->sub_ph2) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 6u), &raw);
        break;
    }
    case BASS_PWM: {
        /* saw − shifted saw = pulse; duty 12.5…62 % via WOBBLE */
        pwm_ph = s->sub_ph + ((uint32_t)(32u + p->wobble) << 24);
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) - dub_saw(pwm_ph);
        dub_svf_mode(raw, fc + (wob_all >> 10), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_WAVEFOLD: {
        int32_t x;
        s->sub_ph += inc;
        x = dub_sine_q64(s->sub_ph) + (dub_saw(s->sub_ph) >> 1);
        x = (x * (int32_t)(80u + p->drive)) >> 6;
        while (x > 28000)
            x -= 56000;
        while (x < -28000)
            x += 56000;
        raw = dub_softclip_q15(x);
        dub_svf_mode(raw, fc >> 1, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    default:
        return bass_bass_house_sub(s, inc, fc, p->reso_mode);
    }
    (void)p;
    return raw;
}

#if defined(GENRE_BASS)
#define idm_kit_k_note techno_kit_k_note
#define idm_kit_k_sweep techno_kit_k_sweep
#define idm_kit_k_fm techno_kit_k_fm
#define idm_kit_k_drv techno_kit_k_drv
#define idm_kit_r_note techno_kit_r_note
#define idm_kit_r_wire techno_kit_r_wire
#define idm_hat_glitch_note techno_hat_glitch_note
#define idm_hat_glitch_fm techno_hat_glitch_fm
#define idm_hat_glitch_noise techno_hat_glitch_noise
#define idm_hat_glitch_metal techno_hat_glitch_metal
#define idm_hat_glitch_style techno_hat_glitch_style
#define idm_hat_glitch_cut techno_hat_glitch_cut
#define idm_hat_glitch_decay techno_hat_glitch_decay
#define idm_hat_glitch_open techno_hat_glitch_open
#define idm_drm_kick_shape techno_drm_kick_shape
#define idm_drm_rim_mix techno_drm_rim_mix
#define idm_drm_bus techno_drm_bus
#define idm_drm_room_send techno_drm_room_send
#define idm_hat_glitch_is_acid_kit techno_hat_glitch_is_acid_kit
#define IDM_HAT_GLITCH_WASH TECHNO_HAT_GLITCH_WASH
#define IDM_HAT_GLITCH_BELL TECHNO_HAT_GLITCH_BELL
#define IDM_HAT_GLITCH_SWING TECHNO_HAT_GLITCH_SWING
#define IDM_HAT_GLITCH_ACID_HAT TECHNO_HAT_GLITCH_ACID_HAT
static inline uint8_t idm_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = (uint8_t)(8u + ((uint32_t)decay * 5u) / 127u);
    (void)era;
    return body;
}
static inline uint8_t idm_env_body_gate(uint8_t decay, int8_t lane_gate, uint8_t era)
{
    int32_t g = (int32_t)lane_gate;
    uint8_t body = idm_env_body(decay, era);
    if (g < 0)
        g = -g;
    body = (uint8_t)(body + (g >> 5));
    return body > 127u ? 127u : body;
}
static inline uint16_t idm_siren_atk_samples(uint8_t era, uint8_t atk)
{
    uint16_t a = atk ? (uint16_t)(22u + ((uint32_t)atk * 28u) / 127u) : 44u;
    (void)era;
    return a > 110u ? 110u : a;
}
static inline void idm_lead_note_gate_piece(uint8_t era, uint8_t piece, uint8_t trig_cnt, int8_t lane_gate,
                                            uint16_t *hold_out, uint16_t *rel_out,
                                            uint8_t len_plock_on, uint8_t len_ix,
                                            uint32_t samples_per_step)
{
    uint32_t total = samples_per_step ? samples_per_step * 2u : (DUB_FS / 8u);
    *hold_out = (uint16_t)((total * 3u) / 4u);
    *rel_out = (uint16_t)(total - *hold_out);
    (void)era;
    (void)piece;
    (void)trig_cnt;
    (void)lane_gate;
    (void)len_plock_on;
    (void)len_ix;
}
static inline void idm_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    if (s->siren_atk_left > 0u) {
        s->siren_env = s->siren_peak;
        s->siren_atk_left--;
        return;
    }
    if (s->siren_gate > 0u)
        s->siren_gate--;
    else if (s->siren_lead_rel > 0u) {
        s->siren_env -= (s->siren_env >> 9) + 2;
        if (s->siren_env < 0)
            s->siren_env = 0;
        s->siren_lead_rel--;
    } else if (s->siren_env > 0) {
        s->siren_env -= (s->siren_env >> 9) + 3;
        if (s->siren_env < 0)
            s->siren_env = 0;
    }
    (void)p;
}
static inline int32_t idm_lead_dly_boost(const dub_params_t *p)
{
    return (int32_t)(p->dly_c >> 2);
}
#define idm_piece_lead_step_bit bass_line_lead_step_bit
#define idm_piece_lead_pitch bass_line_lead_pitch
#define idm_piece_lead_len_ix bass_line_lead_len_ix
#define idm_piece_lead_bend_u7 bass_line_lead_bend_u7
#define IDM_PIECE BASS_LINE
#define IDM_PIECE_KIT BASS_LINE_KIT
#define idm_era_first_piece bass_era_first_line
#define IDM_ERA_MACROS BASS_ERA_MACROS
#define IDM_LEAD_LEN_N 15u
static const uint8_t BASS_LEAD_LEN_16THS[15] = {
    1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64
};
#define IDM_LEAD_LEN_16THS BASS_LEAD_LEN_16THS
#define idm_lead_len_ix(v) bass_lead_len_ix(v)
static inline uint8_t bass_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;
    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)IDM_LEAD_LEN_N)
        ix = (int32_t)IDM_LEAD_LEN_N - 1;
    return (uint8_t)ix;
}
static inline int32_t idm_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t fc)
{
    (void)s;
    (void)p;
    (void)fc;
    return 0;
}
static inline void idm_stutter_process_buffer(dub_synth_t *s, int16_t *lr, uint32_t n)
{
    (void)s;
    (void)lr;
    (void)n;
}
#endif

#endif