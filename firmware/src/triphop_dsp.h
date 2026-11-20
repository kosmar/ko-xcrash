/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRIPHOP_DSP_H
#define TRIPHOP_DSP_H

#include "triphop_data.h"
#include "jazz_pluck.h"
/* IDM-shared DSP paths use semantic era / bass aliases */
#define ERA_OBLIQ ERA_SMOKE
#define ERA_WARM ERA_DUST
#define ERA_GLITCH ERA_VINYL
#define ERA_MELO ERA_STRINGS
#define ERA_DRILL ERA_DARK
#define ERA_BREAK ERA_CINEMA
#define BASS_ROUND BASS_SUBDEEP
#define BASS_THROB BASS_VINYL


/* --- Drum kit tables (kick/rim; lane 2 = hat_glitch — see triphop_hat_glitch_*). --- */
static const int8_t triphop_kit_k_note[DKIT_N]  = { 31, 36, 33, 38, 40, 42, 35, 37 };
static const uint8_t triphop_kit_k_sweep[DKIT_N] = {  8, 28, 12, 16, 20, 10, 12, 16 };
static const uint8_t triphop_kit_k_fm[DKIT_N]   = {  2, 20,  6,  5, 14,  3,  5,  8 };
static const uint8_t triphop_kit_k_drv[DKIT_N]  = { 14, 18, 12, 10, 16,  8,  8, 10 };
static const int8_t triphop_kit_r_note[DKIT_N]  = { 74, 58, 66, 69, 61, 77, 60, 64 };
static const uint8_t triphop_kit_r_wire[DKIT_N] = { 72, 62, 68, 70, 78, 58, 46, 50 };

enum {
    TRIPHOP_HAT_GLITCH_SWING = 0, TRIPHOP_HAT_GLITCH_BELL, TRIPHOP_HAT_GLITCH_WASH, TRIPHOP_HAT_GLITCH_ACID_HAT
};

static const int8_t triphop_hat_glitch_note[DKIT_N] = {
    88, 90, 86, 92, 84, 80, 70, 76
};
static const uint8_t triphop_hat_glitch_fm[DKIT_N] = {
    18, 24, 14, 20, 12,  8,  6, 14
};
static const uint8_t triphop_hat_glitch_noise[DKIT_N] = {
    22, 28, 18, 24, 20, 16, 36, 22
};
static const uint8_t triphop_hat_glitch_metal[DKIT_N] = {
    92, 88, 96, 90, 86, 78, 50, 62
};
static const uint8_t triphop_hat_glitch_decay[DKIT_N] = {
    24, 24, 24, 25, 17, 24, 16, 15
};
static const uint8_t triphop_hat_glitch_open[DKIT_N] = {
     7, 10,  7,  9,  7, 13, 12, 11
};
static const uint8_t triphop_hat_glitch_style[DKIT_N] = {
    TRIPHOP_HAT_GLITCH_SWING, TRIPHOP_HAT_GLITCH_BELL, TRIPHOP_HAT_GLITCH_WASH, TRIPHOP_HAT_GLITCH_SWING,
    TRIPHOP_HAT_GLITCH_BELL, TRIPHOP_HAT_GLITCH_WASH, TRIPHOP_HAT_GLITCH_WASH, TRIPHOP_HAT_GLITCH_BELL
};
static const uint8_t triphop_hat_glitch_cut[DKIT_N] = {
    48, 52, 44, 50, 46, 40, 30, 42
};

static inline int triphop_hat_glitch_is_acid_kit(uint8_t kit)
{
    return triphop_hat_glitch_style[kit % DKIT_N] == (uint8_t)TRIPHOP_HAT_GLITCH_ACID_HAT;
}

static inline int32_t triphop_drm_kick_shape(int32_t kick_sig, uint16_t kick_snap, int32_t k_drv_tot,
                                          uint32_t *noise)
{
    /* Tiny transient only — shared PUNCH path owns beater; avoid double click. */
    if (kick_snap > 150u && noise)
        kick_sig += (dub_noise(noise) * (4 + (k_drv_tot >> 3))) >> 9;
    return dub_softclip_q15((kick_sig * (272 + k_drv_tot * 3)) >> 8);
}

static inline int32_t triphop_drm_rim_mix(int32_t shell, int32_t wire, uint8_t kit_wire, uint8_t rim_drv)
{
    int32_t mix_r = ((shell * (40 - (kit_wire >> 4))) >> 7) + ((wire * 5) >> 4);

    return dub_softclip_q15((mix_r * (272 + (int32_t)rim_drv * 2)) >> 8);
}

static inline int32_t triphop_drm_bus(int32_t drum_sig, uint8_t kit_drive)
{
    int32_t gain = 256 + ((int32_t)kit_drive * 2);

    drum_sig = (drum_sig * gain) >> 8;
    return dub_softclip_q15(drum_sig);
}

static inline int32_t triphop_drm_room_send(int32_t kick_sig, int32_t rim_sig, int32_t hat_glitch_sig)
{
    (void)kick_sig;
    return ((rim_sig * 10) >> 7) + ((hat_glitch_sig * 8) >> 7);
}

static inline int32_t triphop_bass_house_sub(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t raw;

    s->sub_ph += inc;
    s->sub_ph2 += inc >> 1;
    raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph2) >> 2);
    dub_svf_mode(raw, 600 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t triphop_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;
    int32_t walk = lfo_mod_b;

    switch (model) {
    case BASS_SUBDEEP: {
        s->sub_ph += inc;
        s->sub_ph2 += inc >> 1;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph2) >> 2);
        dub_svf_mode(raw, 280 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)(128u - (p->pump >> 1))) >> 7;
        break;
    }
    case BASS_VINYL: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, 1100 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_SMOKE: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_square(s->sub_ph) >> 2);
        dub_svf_mode(raw, fc + (walk >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 12u), &raw);
        raw = (int32_t)(((int64_t)raw * s->sub_env) >> 14);
        if (raw > 32767)
            raw = 32767;
        else if (raw < -32767)
            raw = -32767;
        break;
    }
    case BASS_RHODES: {
        uint32_t det = inc >> 4;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc - det;
        raw = dub_saw(s->sub_ph) + dub_saw(s->sub_ph2) + dub_saw(s->sub_ph3);
        dub_svf_mode(raw, fc + (wob_all >> 9), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 18u), &raw);
        break;
    }
    case BASS_WARM: {
        int32_t saw_w, acid_fc;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        raw = saw_w + (dub_square(s->sub_ph) >> 1);
        acid_fc = fc + (((s->sub_env >> 3) * (int32_t)p->wobble) >> 5);
        dub_svf_mode(raw, acid_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 28u > 127u ? 127u : p->reso_mode + 28u), &raw);
        break;
    }
    case BASS_DARK: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc + (inc >> 2);
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + ((uint32_t)mod2 << 12));
        dub_svf_mode(raw, fc + (walk >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (int32_t)(((int64_t)raw * s->sub_env) >> 15);
        if (raw > 32767)
            raw = 32767;
        else if (raw < -32767)
            raw = -32767;
        break;
    }
    case BASS_STRING: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_sine_q64(s->sub_ph) >> 2);
        dub_svf_mode(raw, 500 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 8u), &raw);
        raw = (raw * (int32_t)(96u + (p->drive >> 1))) >> 7;
        break;
    }
    case BASS_DOOM: {
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
    case BASS_TAPE: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + ((uint32_t)mod2 << 13));
        dub_svf_mode(raw, fc + ((int32_t)p->wobble * 6), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_CINEMA: {
        s->sub_ph += inc >> 1;
        s->sub_ph2 += (inc >> 1) + (uint32_t)(((uint64_t)(inc >> 1) * p->wobble) >> 12);
        raw = dub_sine_q64(s->sub_ph) >> 1;
        raw += dub_sine_q64(s->sub_ph2) >> 2;
        dub_svf_mode(raw, fc >> 1, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode >> 1, &raw);
        break;
    }
    default:
        return triphop_bass_house_sub(s, inc, fc, p->reso_mode);
    }
    (void)p;
    return raw;
}


/* --- Lane-3 lead env / gate / len / bend (HP-shaped, no pluck path). --- */
typedef struct {
    uint8_t hold_pct;
    uint8_t env_floor_e8;
    uint8_t rel_decay_sh;
    uint8_t gate_cs;
} triphop_lead_sustain_t;

static const triphop_lead_sustain_t TRIPHOP_LEAD_SUSTAIN[TRIPHOP_ERA_N] = {
    { 62, 4, 10, 30 }, /* DETROIT */
    { 68, 5, 11, 34 }, /* BELGIUM */
    { 42, 2,  8, 14 }, /* SCHRANZ */
    { 52, 4, 10, 22 }, /* HYPNO */
    { 58, 5, 12, 32 }, /* BERLIN */
    { 48, 3,  9, 18 }, /* EBM */
};

static inline const triphop_lead_sustain_t *triphop_lead_sustain(uint8_t era)
{
    return &TRIPHOP_LEAD_SUSTAIN[triphop_clamp_era(era)];
}

enum {
    TRIPHOP_LEAD_FLUTE = 0, TRIPHOP_LEAD_SAX, TRIPHOP_LEAD_MUTE_TRP,
    TRIPHOP_LEAD_CLAV, TRIPHOP_LEAD_HORN, TRIPHOP_LEAD_EP, TRIPHOP_LEAD_TIMBRE_N
};

static inline uint8_t triphop_lead_timbre(uint8_t era)
{
    static const uint8_t timbre[TRIPHOP_ERA_N] = {
        TRIPHOP_LEAD_EP, TRIPHOP_LEAD_SAX, TRIPHOP_LEAD_MUTE_TRP,
        TRIPHOP_LEAD_CLAV, TRIPHOP_LEAD_HORN, TRIPHOP_LEAD_FLUTE
    };

    return timbre[triphop_clamp_era(era)];
}

static inline uint8_t triphop_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = (uint8_t)(8u + ((uint32_t)decay * 5u) / 127u);

    era = triphop_clamp_era(era);
    if (era == (uint8_t)ERA_OBLIQ)
        body = (uint8_t)(body + 2u);
    else if (era == (uint8_t)ERA_WARM)
        body = (uint8_t)(body + 1u);
    return body;
}

static inline uint8_t triphop_env_body_gate(uint8_t decay, int8_t lane_gate, uint8_t era)
{
    int32_t g = (int32_t)lane_gate;
    uint8_t body;

    body = triphop_env_body(decay, era);
    if (g < 0)
        g = -g;
    body = (uint8_t)(body + (g >> 5));
    if (body > 127u)
        body = 127u;
    return body;
}

enum { TRIPHOP_LEAD_LEN_N = 15u };

/* Same GATE note-value labels as HP lead. */
static const char *const TRIPHOP_LEAD_LEN_NAMES[TRIPHOP_LEAD_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t TRIPHOP_LEAD_LEN_NUM[TRIPHOP_LEAD_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t TRIPHOP_LEAD_LEN_DEN[TRIPHOP_LEAD_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
/* TRIPHOP_LEAD_LEN_16THS lives in triphop_data.h */

static inline uint8_t triphop_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)TRIPHOP_LEAD_LEN_N)
        ix = (int32_t)TRIPHOP_LEAD_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t triphop_lead_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = triphop_lead_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)TRIPHOP_LEAD_LEN_16THS[ix];
}

static inline uint8_t triphop_lead_bend_semi(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

static inline uint8_t triphop_lead_bend_plock(uint8_t bend_semi)
{
    if (bend_semi < 3u)
        return 0u;
    return (uint8_t)(32u + ((uint32_t)bend_semi * 95u) / 12u);
}

#define TRIPHOP_LEAD_BEND_SAMPLES ((DUB_FS * 55u) / 1000u)

/* 2^(semi_q8 / (12 * 256)) in Q16; semi_q8 in 1/256 semitone, |semi| ≤ 48 st. */
static inline uint32_t triphop_semi_ratio_q16(int32_t semi_q8)
{
    static const uint32_t st_q16[13] = {
        65536u, 69433u, 73562u, 77936u, 82570u, 87480u, 92682u,
        98193u, 104032u, 110218u, 116772u, 123715u, 131072u
    };
    int32_t oct = 0;
    uint32_t st, fr, r;

    while (semi_q8 < 0) {
        semi_q8 += 12 * 256;
        oct--;
    }
    while (semi_q8 >= 12 * 256) {
        semi_q8 -= 12 * 256;
        oct++;
    }
    st = (uint32_t)semi_q8 >> 8;
    fr = (uint32_t)semi_q8 & 255u;
    r = st_q16[st] + (((st_q16[st + 1u] - st_q16[st]) * fr) >> 8);
    return oct >= 0 ? r << oct : r >> -oct;
}

static inline uint32_t triphop_lead_bent_inc(const dub_synth_t *s, uint32_t inc)
{
    uint32_t n = s->siren_note_age;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0 && n < TRIPHOP_LEAD_BEND_SAMPLES) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        int32_t depth = (int32_t)s->siren_hp_bend;
        int32_t off_q8 = (semi * 256 * depth) / 127;

        off_q8 = (int32_t)(((int64_t)off_q8 * (int64_t)(TRIPHOP_LEAD_BEND_SAMPLES - n)) / (int64_t)TRIPHOP_LEAD_BEND_SAMPLES);
        if (off_q8 != 0)
            inc = (uint32_t)(((uint64_t)inc * triphop_semi_ratio_q16(-off_q8)) >> 16);
    }
    return inc;
}

static inline uint16_t triphop_siren_atk_samples(uint8_t era, uint8_t atk)
{
    uint16_t a;
    uint16_t cap = 110u;

    era = triphop_clamp_era(era);
    if (era == (uint8_t)ERA_WARM || era == (uint8_t)ERA_OBLIQ)
        cap = 120u;
    else if (era == (uint8_t)ERA_DRILL || era == (uint8_t)ERA_GLITCH)
        cap = 72u;
    else if (era == (uint8_t)ERA_MELO || era == (uint8_t)ERA_BREAK)
        cap = 88u;
    if (atk)
        a = (uint16_t)(22u + ((uint32_t)atk * 28u) / 127u);
    else
        a = 44u;
    if (era == (uint8_t)ERA_DRILL)
        a = (uint16_t)((a * 55u) / 100u);
    else if (era == (uint8_t)ERA_GLITCH)
        a = (uint16_t)((a * 70u) / 100u);
    else if (era == (uint8_t)ERA_WARM)
        a = (uint16_t)((a * 115u) / 100u);
    if (a > cap)
        a = cap;
    return a;
}

static inline void triphop_lead_note_gate_piece(uint8_t era, uint8_t piece, uint8_t trig_cnt, int8_t lane_gate,
                                              uint16_t *hold_out, uint16_t *rel_out,
                                              uint8_t len_plock_on, uint8_t len_ix,
                                              uint32_t samples_per_step)
{
    static const uint16_t role_scale_q8[4] = { 280u, 220u, 320u, 200u };
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;
    uint8_t role = (uint8_t)((trig_cnt - 1u) & 3u);
    const triphop_lead_sustain_t *su = triphop_lead_sustain(era);
    uint8_t gate_cs = triphop_loop_lead_gate_cs(piece);

    if (len_plock_on != 0u && samples_per_step > 0u) {
        total = triphop_lead_len_samples(samples_per_step, len_ix);
        if (total < 1u)
            total = 1u;
    } else {
        total = (DUB_FS * (uint32_t)(gate_cs ? gate_cs : su->gate_cs)) / 100u;
        total = (total * (uint32_t)role_scale_q8[role]) >> 8;
        if (g < 0)
            g = -g;
        total += (uint32_t)g * (DUB_FS / 700u);
    }
    hold = (total * (uint32_t)su->hold_pct) / 100u;
    if (hold > 65500u)
        hold = 65500u;
    if (total > hold + 65535u)
        total = hold + 65535u;
    *hold_out = (uint16_t)hold;
    *rel_out = (uint16_t)(total - hold);
}

static inline int32_t triphop_lead_release_decay_shift(uint8_t era)
{
    return (int32_t)triphop_lead_sustain(era)->rel_decay_sh;
}

static inline void triphop_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    if ((s->siren_atk_left > 0u || s->siren_gate > 0u || s->siren_lead_rel > 0u || s->siren_env > 0)
        && s->siren_note_age < 65535u) {
        if (s->siren_note_age == 0u)
            s->siren_pluck_arm = 1u;
        s->siren_note_age++;
    }

    if (s->siren_atk_left > 0u) {
        int32_t pk = s->siren_peak;
        int32_t diff = pk - s->siren_env;
        int32_t step = diff >> 2;

        if (step < pk >> 6)
            step = pk >> 6;
        if (step < 1)
            step = 1;
        if (step > diff)
            step = diff;
        s->siren_env += step;
        s->siren_atk_left--;
        if (s->siren_atk_left == 0u || s->siren_env >= pk)
            s->siren_env = pk;
        return;
    }
    if (s->siren_gate > 0u) {
        const triphop_lead_sustain_t *su = triphop_lead_sustain(s->voice_era);
        int32_t sustain_floor = (s->siren_peak * (int32_t)su->env_floor_e8) >> 3;

        if (s->siren_env > sustain_floor)
            s->siren_env -= ((s->siren_env - sustain_floor) >> 12) + 1;
        else
            s->siren_env = sustain_floor;
        s->siren_gate--;
    } else if (s->siren_lead_rel > 0u) {
        int32_t sh = triphop_lead_release_decay_shift(s->voice_era);

        s->siren_env -= (s->siren_env >> sh) + 2;
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

static inline int32_t triphop_siren_decay_shift(uint8_t era, uint8_t drm_dec)
{
    int32_t sh = 12 + (((int32_t)drm_dec - 64) >> 5);

    era = triphop_clamp_era(era);
    if (era == (uint8_t)ERA_OBLIQ)
        sh += 1;
    else if (era == (uint8_t)ERA_GLITCH)
        sh += 2;
    if (sh < 11)
        sh = 11;
    else if (sh > 15)
        sh = 15;
    return sh;
}

static inline int32_t triphop_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}

static inline int triphop_lead_uses_karplus(uint8_t timbre)
{
    return timbre == (uint8_t)TRIPHOP_LEAD_SAX;
}

static inline void triphop_lead_pluck_trig(dub_synth_t *s, const dub_params_t *p, uint32_t inc)
{
    uint8_t dec = p->drm_dec[3] ? p->drm_dec[3] : 64u;
    uint8_t bright = 52u + ((uint32_t)p->siren_rate * 44u) / 127u;

    jazz_pluck_trig(inc, dec, bright, &s->noise, 1u);
}

/* --- House-light lead: 1–2 oscillators + SVF, era timbre switch. --- */
static inline int32_t triphop_lead_voice_osc(dub_synth_t *s, uint32_t inc, int32_t env, uint8_t timbre,
                                          int32_t bright)
{
    uint32_t ph = s->stab_ph[0];

    s->stab_ph[0] += inc;
    switch (timbre) {
    case TRIPHOP_LEAD_FLUTE: {
        int32_t air = (dub_noise(&s->noise) * (bright >> 2)) >> 10;
        int32_t tone = dub_sine_q64(ph) + (dub_sine_q64(ph << 1) >> 5);

        return tone + air;
    }
    case TRIPHOP_LEAD_SAX: {
        int32_t pk = jazz_pluck_step(inc);
        int32_t sine = dub_sine_q64(ph);

        return pk + (sine >> 2);
    }
    case TRIPHOP_LEAD_MUTE_TRP: {
        int32_t mod = dub_sine_q64(ph + (ph >> 1));
        int32_t tone = dub_sine_q64(ph + (uint32_t)((mod * (18 + (bright >> 2))) >> 9));
        int32_t body = (tone * 7) >> 3;
        int32_t bits = 11 - (((bright - 48) * 6) / 56); /* bright 48..104 → 11..5 bit */
        int32_t step;

        if (bits < 5)
            bits = 5;
        else if (bits > 11)
            bits = 11;
        step = (int32_t)1 << (15 - bits);
        if ((ph >> 19) & 3u)
            body >>= 1;
        body = (body + (step >> 1)) & ~(step - 1);
        return body;
    }
    case TRIPHOP_LEAD_CLAV: {
        int32_t mod1 = dub_sine_q64(ph << 1);
        int32_t mod2 = dub_sine_q64(ph + (ph << 1));
        int32_t idx = ((mod1 + (mod2 >> 1)) * (20 + (bright >> 1))) >> 10;

        return dub_sine_q64(ph + ((uint32_t)idx << 13));
    }
    case TRIPHOP_LEAD_HORN: {
        int32_t mod = dub_sine_q64(ph << 1) + (dub_sine_q64(ph + ph) >> 1);
        int32_t sawv = dub_saw(ph) >> 1;
        int32_t tone = dub_sine_q64(ph + (uint32_t)((mod * (44 + bright + (bright >> 1))) >> 9)) >> 1;

        return sawv + tone;
    }
    case TRIPHOP_LEAD_EP:
    default: {
        int32_t mod = dub_sine_q64(ph << 1) + (dub_sine_q64(ph + (ph << 1)) >> 1);
        int32_t warp = (bright * bright) >> 7;
        int32_t idx = (mod * (16 + warp + (bright >> 2))) >> 10;
        int32_t tone = dub_sine_q64(ph + ((uint32_t)idx << 13));

        return tone + (dub_sine_q64(ph << 1) >> 3) + (dub_sine_q64(ph + ((uint32_t)idx << 12)) >> 4);
    }
    }
    (void)env;
}

static inline int32_t triphop_lead_tone(dub_synth_t *s, int32_t raw, uint8_t timbre, int32_t env, int32_t fc,
                                     uint8_t reso)
{
    int32_t out = raw;

    dub_svf_mode(out, fc, &s->lead_cab_lp, &s->lead_cab_bp, reso, &out);
    if (timbre == TRIPHOP_LEAD_MUTE_TRP) {
        dub_svf_mode(out, fc + (fc >> 2), &s->lead_amp_treb_lp, &s->lead_amp_treb_bp,
                     (uint8_t)(reso >> 1), &out);
        out = (out * 9) >> 4;
    } else if (timbre == TRIPHOP_LEAD_SAX) {
        dub_svf_mode(out, fc + (fc >> 2), &s->lead_amp_treb_lp, &s->lead_amp_treb_bp, reso, &out);
        out += s->lead_amp_treb_bp >> 2;
    } else if (timbre == TRIPHOP_LEAD_HORN) {
        dub_svf_mode(out, fc + (fc >> 1), &s->lead_amp_mid_lp, &s->lead_amp_mid_bp, reso, &out);
        out += s->lead_amp_mid_bp >> 1;
    } else if (timbre == TRIPHOP_LEAD_FLUTE) {
        dub_svf_mode(out, fc + (fc >> 2), &s->lead_amp_hp_lp, &s->lead_amp_hp_bp,
                     (uint8_t)(reso >> 2), &out);
    }
    return (int32_t)(((int64_t)out * env) >> 15);
}

static inline int32_t triphop_lead_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, root_m, bright, fc;
    uint32_t inc;
    uint8_t era = triphop_clamp_era(p->era);
    uint8_t timbre = triphop_lead_timbre(era);

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 20 / 64;
    root_m = (int32_t)(p->root ? p->root : 60u) + kt_s + (int32_t)s->siren_st_tune + 12;
    if (root_m < 55)
        root_m = 55;
    else if (root_m > 84)
        root_m = 84;

    inc = triphop_lead_bent_inc(s, dub_note_inc(dub_clamp_midi((uint8_t)root_m)));
    if (s->siren_pluck_arm) {
        s->siren_pluck_arm = 0u;
        if (triphop_lead_uses_karplus(timbre))
            triphop_lead_pluck_trig(s, p, inc);
    }
    bright = 48 + ((int32_t)p->siren_rate * 56) / 127;
    fc = 900 + ((int32_t)p->cutoff_c * 120) + (bright * 8);
    if (fc > 14000)
        fc = 14000;

    {
        int32_t v1 = triphop_lead_voice_osc(s, inc, env, timbre, bright);
        int32_t v2 = 0;

        if (timbre == TRIPHOP_LEAD_EP || timbre == TRIPHOP_LEAD_CLAV) {
            s->stab_ph[1] += inc;
            v2 = dub_sine_q64(s->stab_ph[1] + (inc >> 2)) >> 2;
        }
        return triphop_lead_tone(s, v1 + v2, timbre, env, fc, p->reso_mode);
    }
}

/* Same lesson as Jazz: 38 Q12 → near inaudible vs drums. */
#define TRIPHOP_LEAD_OUT_GAIN_Q12 1600u

static inline int32_t triphop_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t lfo_mod_c)
{
    int32_t sig, lvl;
    uint32_t n = s->siren_note_age;
    uint8_t timbre = triphop_lead_timbre(triphop_clamp_era(p->era));

    (void)lfo_mod_c;
    sig = triphop_lead_sample(s, p, s->siren_env);
    if (n < 120u) {
        int32_t p_env = (int32_t)(120u - n);
        int32_t atk = (dub_noise(&s->noise) >> 3);

        if (timbre == TRIPHOP_LEAD_FLUTE)
            atk >>= 1;
        atk = (atk * p_env) >> 9;
        sig += (atk * s->siren_env) >> 16;
    }
    /* LEAD LEVEL = siren_pitch (same as Jazz / Dub SIREN). */
    sig = (sig * (int32_t)(p->siren_pitch ? p->siren_pitch : 96u)) >> 7;
    lvl = p->lvl_c ? p->lvl_c : 96u;
    sig = (sig * (int32_t)lvl) / 92;
    sig = dub_softclip_q15((sig * 420) >> 8);
    sig = (sig * (int32_t)TRIPHOP_LEAD_OUT_GAIN_Q12) >> 12;
    return dub_softclip_q15(sig);
}

/* --- Stutter buffer + bit-degrade (era-scaled; wired from fm_app Task 3). --- */
enum { TRIPHOP_STUTTER_TAP_N = 2048u };

enum { TRIPHOP_STUT_HACK = 0, TRIPHOP_STUT_SCREW = 1, TRIPHOP_STUT_GEST = 2 };

typedef struct {
    int16_t ring[TRIPHOP_STUTTER_TAP_N];
    uint16_t wpos;
    uint16_t rpos;
    uint16_t slice_len;
    uint16_t slice_left;
    uint8_t active;
    uint8_t wet;
    uint8_t reverse;
    uint8_t mode; /* TRIPHOP_STUT_* */
    int32_t f_lp; /* 1-pole for iso colour */
} triphop_stutter_t;

static inline uint8_t triphop_era_degrade_base(uint8_t era)
{
    /* WARM/MELO quiet; GLITCH→OBLIQ light grit (chop via stutter, not noise floor) */
    static const uint8_t base[TRIPHOP_ERA_N] = { 8, 10, 18, 22, 16, 12 };

    return base[triphop_clamp_era(era)];
}

static inline uint8_t triphop_degrade_amount(uint8_t era, uint8_t macro_grit, uint8_t edit_density)
{
    uint32_t amt = (uint32_t)triphop_era_degrade_base(era) + (((uint32_t)macro_grit * 3u) >> 2) +
                   ((uint32_t)edit_density >> 2);

    if (amt > 127u)
        amt = 127u;
    return (uint8_t)amt;
}

/* amt = triphop_degrade_amount() >> 3 (fm_app caller), i.e. 0..15. */
static inline int32_t triphop_degrade_sample(int32_t x, uint32_t *noise, uint8_t amt)
{
    int32_t nz;

    if (amt == 0u || !noise)
        return x;
    if (amt > 15u)
        amt = 15u;
    nz = dub_noise(noise) >> (11u - (amt >> 2));
    if (amt >= 4u) {
        int32_t bits = 12 - (((int32_t)amt - 4) * 6) / 11; /* 12..6 bit */
        int32_t step = (int32_t)1 << (15 - bits);

        x = (x + (step >> 1)) & ~(step - 1);
    }
    if (amt >= 8u)
        x = (x * (256 - ((int32_t)amt << 2))) >> 8;
    if (amt >= 10u && (((uint32_t)dub_noise(noise) >> 3) & 31u) == 0u)
        x = 0;
    x += nz;
    if (x > 32767)
        x = 32767;
    else if (x < -32768)
        x = -32768;
    return x;
}

static inline void triphop_stutter_push(triphop_stutter_t *st, int32_t sample)
{
    if (!st)
        return;
    if (sample > 32767)
        sample = 32767;
    else if (sample < -32768)
        sample = -32768;
    st->ring[st->wpos % TRIPHOP_STUTTER_TAP_N] = (int16_t)sample;
    st->wpos++;
}

static inline int32_t triphop_stutter_mix(triphop_stutter_t *st, int32_t dry, uint8_t wet_u7)
{
    int32_t tap, out, env_w;
    uint16_t idx;
    uint16_t pos;
    uint16_t len;

    if (!st || !st->active || wet_u7 == 0u)
        return dry;
    if (st->slice_len == 0u)
        st->slice_len = 64u;
    if (st->slice_left == 0u) {
        st->rpos = (uint16_t)(st->wpos - st->slice_len);
        st->slice_left = st->slice_len;
    }
    len = st->slice_len;
    pos = (uint16_t)(len - st->slice_left); /* 0 at burst start */
    idx = st->reverse ? (uint16_t)(st->rpos + st->slice_left - 1u) : (uint16_t)(st->rpos + pos);
    tap = (int32_t)st->ring[idx % TRIPHOP_STUTTER_TAP_N];
    st->slice_left--;
    if (st->slice_left == 0u)
        st->rpos = (uint16_t)((st->rpos + 1u) % TRIPHOP_STUTTER_TAP_N);

    if (st->mode == (uint8_t)TRIPHOP_STUT_SCREW) {
        /* Telephone freeze: LP iso + soft attack / long sustain; crossfade dry */
        uint16_t atk = (uint16_t)(len / 6u);
        uint16_t rel = (uint16_t)(len / 5u);
        int32_t e;

        if (atk < 8u)
            atk = 8u;
        if (rel < 8u)
            rel = 8u;
        if (pos < atk)
            e = ((int32_t)pos * 128) / (int32_t)atk;
        else if (pos + rel >= len)
            e = (((int32_t)len - (int32_t)pos) * 128) / (int32_t)rel;
        else
            e = 128;
        if (e < 0)
            e = 0;
        if (e > 128)
            e = 128;
        st->f_lp += (tap - st->f_lp) >> 3; /* slow LP = mid scoop / phone */
        tap = (st->f_lp * 3 + tap) >> 2;
        env_w = ((int32_t)wet_u7 * e) >> 7;
        if (env_w > 96)
            env_w = 96;
        out = ((dry * (128 - env_w)) + (tap * env_w)) >> 7;
    } else if (st->mode == (uint8_t)TRIPHOP_STUT_HACK) {
        /* Percussive HP click: fast attack, hard decay; additive blip */
        uint16_t atk = (uint16_t)(len / 16u);
        int32_t e;
        int32_t hp;

        if (atk < 2u)
            atk = 2u;
        if (pos < atk)
            e = ((int32_t)pos * 128) / (int32_t)atk;
        else
            e = 128 - (((int32_t)(pos - atk) * 128) / (int32_t)(len - atk + 1u));
        if (e < 0)
            e = 0;
        if (e > 128)
            e = 128;
        st->f_lp += (tap - st->f_lp) >> 2;
        hp = tap - st->f_lp; /* thin transient */
        tap = hp + (tap >> 3);
        env_w = ((int32_t)wet_u7 * e) >> 7;
        out = dry + ((tap * env_w) >> 7);
    } else {
        /* GEST / FILL: classic additive */
        out = dry + ((tap * (int32_t)wet_u7) >> 7);
    }
    return dub_softclip_q15(out);
}

static inline uint16_t triphop_stutter_slice_from_vel(uint8_t vel, uint8_t era)
{
    uint16_t base = (uint16_t)(28u + ((uint32_t)vel * 88u) / 127u);
    uint8_t e = triphop_clamp_era(era);

    if (e == (uint8_t)ERA_SMOKE)
        base = (uint16_t)((base * 5u) / 8u);
    else if (e == (uint8_t)ERA_VINYL)
        base = (uint16_t)((base * 5u) / 4u);
    else if (e == (uint8_t)ERA_DARK)
        base = (uint16_t)((base * 3u) / 4u);
    else if (e == (uint8_t)ERA_CINEMA)
        base = (uint16_t)((base * 5u) / 6u);
    if (base < 6u)
        base = 6u;
    if (base > 512u)
        base = 512u;
    return base;
}


#if defined(GENRE_TRIPHOP)
#define idm_env_body_gate triphop_env_body_gate
#define idm_env_body triphop_env_body
#define idm_lead_timbre triphop_lead_timbre
#define idm_kit_k_note triphop_kit_k_note
#define idm_kit_k_sweep triphop_kit_k_sweep
#define idm_kit_k_fm triphop_kit_k_fm
#define idm_kit_k_drv triphop_kit_k_drv
#define idm_kit_r_note triphop_kit_r_note
#define idm_kit_r_wire triphop_kit_r_wire
#define idm_hat_glitch_note triphop_hat_glitch_note
#define idm_hat_glitch_fm triphop_hat_glitch_fm
#define idm_hat_glitch_noise triphop_hat_glitch_noise
#define idm_hat_glitch_metal triphop_hat_glitch_metal
#define idm_hat_glitch_style triphop_hat_glitch_style
#define idm_hat_glitch_cut triphop_hat_glitch_cut
#define idm_drm_kick_shape triphop_drm_kick_shape
#define idm_drm_rim_mix triphop_drm_rim_mix
#define idm_drm_bus triphop_drm_bus
#define idm_drm_room_send triphop_drm_room_send
#define IDM_ERA_MACROS TRIPHOP_ERA_MACROS
#define IDM_LEAD_LEN_N TRIPHOP_LEAD_LEN_N
#define IDM_LEAD_LEN_16THS TRIPHOP_LEAD_LEN_16THS
#define IDM_HAT_GLITCH_WASH TRIPHOP_HAT_GLITCH_WASH
#define IDM_HAT_GLITCH_BELL TRIPHOP_HAT_GLITCH_BELL
#define IDM_HAT_GLITCH_SWING TRIPHOP_HAT_GLITCH_SWING
#define IDM_HAT_GLITCH_ACID_HAT TRIPHOP_HAT_GLITCH_ACID_HAT
#define idm_lead_dly_boost triphop_lead_dly_boost
#define idm_stutter_t triphop_stutter_t
#define idm_piece_lead_step_bit triphop_loop_lead_step_bit
#define idm_piece_lead_pitch triphop_loop_lead_pitch
#define idm_piece_lead_len_ix triphop_loop_lead_len_ix
#define idm_piece_lead_bend_u7 triphop_loop_lead_bend_u7
#define idm_lead_bend_plock triphop_lead_bend_plock
#define idm_lead_bend_semi triphop_lead_bend_semi
#define idm_lead_len_ix triphop_lead_len_ix
#define idm_fill_morph_steps triphop_fill_morph_steps
#define idm_stutter_edit_live triphop_stutter_edit_live
#define idm_u8_blend triphop_u8_blend
#define idm_fill_build_amt triphop_fill_build_amt
#define idm_stutter_edit_targets triphop_stutter_edit_targets
#define idm_fill_drop_targets triphop_fill_drop_targets
#define idm_apply_fill_break_voice triphop_apply_fill_break_voice
#define idm_stutter_edit_restore triphop_stutter_edit_restore
#define idm_stutter_start triphop_stutter_start
#define idm_stutter_release triphop_stutter_release
#define idm_stutter_bar triphop_stutter_bar
#define idm_edit_latch_begin triphop_edit_latch_begin
#define idm_stutter_latch_block triphop_stutter_latch_block
#define idm_rev_stutter_flip triphop_rev_stutter_flip
#define idm_glitch_burst_hit triphop_glitch_burst_hit
#define idm_stutter_sync_runtime triphop_stutter_sync_runtime
#define idm_edit_apply_drums triphop_edit_apply_drums
#define idm_stutter_process_buffer triphop_stutter_process_buffer
#define idm_loop_macro_sync triphop_loop_macro_sync
#define idm_lead_note_gate_piece triphop_lead_note_gate_piece
#define idm_siren_atk_samples triphop_siren_atk_samples
#define idm_siren_env_step triphop_siren_env_step
#define idm_lead_render triphop_lead_render
#define idm_lead_bent_inc triphop_lead_bent_inc
#define idm_lead_len_samples triphop_lead_len_samples
#define idm_stutter_push triphop_stutter_push
#define idm_stutter_mix triphop_stutter_mix
#define idm_stutter_slice_from_vel triphop_stutter_slice_from_vel
#define idm_degrade_amount triphop_degrade_amount
#define idm_degrade_sample triphop_degrade_sample
#define idm_hat_glitch_is_acid_kit triphop_hat_glitch_is_acid_kit
#define idm_hat_glitch_decay triphop_hat_glitch_decay
#define idm_hat_glitch_open triphop_hat_glitch_open
#define IDM_PIECE TRIPHOP_LOOP
#define IDM_PIECE_KIT TRIPHOP_LOOP_KIT
#define idm_era_first_piece triphop_era_first_loop
#define IDM_LEAD_LEN_NAMES TRIPHOP_LEAD_LEN_NAMES
#define IDM_STUT_HACK TRIPHOP_STUT_HACK
#define IDM_STUT_SCREW TRIPHOP_STUT_SCREW
/* IDM_FILL_MORPH_STEPS / IDM_STUTTER_MORPH_STEPS: defined in fm_app.c */
#endif

#endif
