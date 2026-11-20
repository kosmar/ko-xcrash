/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HIPHOP_DSP_H
#define HIPHOP_DSP_H

#include "hiphop_data.h"

/* --- Drum kit tables (kick/rim; lane 2 = hat — hiphop_hat_* in hiphop_data.h). --- */
static const int8_t hiphop_kit_k_note[DKIT_N]  = { 28, 32, 28, 34, 36, 38, 31, 33 };
/* Soft sweep/FM — high FM+drive was reading as tin / metal, not boom. */
static const uint8_t hiphop_kit_k_sweep[DKIT_N] = { 10, 14, 18, 16, 20, 12, 10, 14 };
static const uint8_t hiphop_kit_k_fm[DKIT_N]   = {  2,  6,  0,  5,  8,  3,  3,  4 };
static const uint8_t hiphop_kit_k_drv[DKIT_N]  = { 14, 16,  8, 14, 16, 10, 10, 12 };
static const int8_t hiphop_kit_r_note[DKIT_N]  = { 72, 58, 66, 64, 56, 74, 54, 68 };
static const uint8_t hiphop_kit_r_wire[DKIT_N] = { 64, 52, 58, 56, 68, 48, 72, 60 };

static inline int32_t hiphop_drm_kick_shape(int32_t kick_sig, uint16_t kick_snap, int32_t k_drv_tot,
                                          uint32_t *noise)
{
    (void)kick_snap;
    (void)noise;
    /* Mild saturator — fat from boom/body, not hard clip harmonics. */
    return dub_softclip_q15((kick_sig * (256 + k_drv_tot * 2)) >> 8);
}

static inline int32_t hiphop_drm_rim_mix(int32_t shell, int32_t wire, uint8_t kit_wire, uint8_t rim_drv)
{
    /* Shell-led snare; wire was oversized and read metallic. */
    int32_t mix_r = ((shell * (56 - (kit_wire >> 4))) >> 7) + ((wire * 7) >> 3);

    return dub_softclip_q15((mix_r * (248 + (int32_t)rim_drv)) >> 8);
}

static inline int32_t hiphop_drm_bus(int32_t drum_sig, uint8_t kit_drive)
{
    int32_t gain = 256 + ((int32_t)kit_drive);

    drum_sig = (drum_sig * gain) >> 8;
    return dub_softclip_q15(drum_sig);
}

static inline int32_t hiphop_drm_room_send(int32_t kick_sig, int32_t rim_sig, int32_t hat_sig)
{
    (void)kick_sig;
    return ((rim_sig * 10) >> 7) + ((hat_sig * 8) >> 7);
}

/* SP1200-style bass stab: pitch bend + bit crush (no PCM). */
static inline uint8_t hiphop_bass_degrade_amt(uint8_t era)
{
    /* Dust, not bitcrush metal — high amt read as tinny digital. */
    static const uint8_t crush[HIPHOP_ERA_N] = { 28, 20, 12, 0, 0, 0 };

    return crush[hiphop_clamp_era(era)];
}

static inline uint32_t hiphop_bass_stab_bend_samples(uint8_t era)
{
    uint32_t ms;

    era = hiphop_clamp_era(era);
    if (era == (uint8_t)ERA_PARK)
        ms = 36u;
    else if (era == (uint8_t)ERA_GOLD)
        ms = 30u;
    else if (era == (uint8_t)ERA_BOOM)
        ms = 22u;
    else
        ms = 0u;
    return (DUB_FS * ms) / 1000u;
}

/* Bass note age in samples and sub-octave phase; single synth instance, reset when dub_trig_sub
 * zeroes sub_ph. */
static uint32_t hiphop_bass_age;
static uint32_t hiphop_sub_oct_ph;

static inline uint32_t hiphop_bass_bent_inc(const dub_synth_t *s, uint32_t tgt_inc, uint8_t era)
{
    uint32_t bend_samples, age, bend_q16;

    if (!s || tgt_inc == 0u || hiphop_clamp_era(era) > (uint8_t)ERA_BOOM)
        return tgt_inc;
    bend_samples = hiphop_bass_stab_bend_samples(era);
    if (bend_samples == 0u)
        return tgt_inc;
    age = hiphop_bass_age;
    if (age >= bend_samples)
        return tgt_inc;
    if (era == (uint8_t)ERA_PARK)
        bend_q16 = 4200u;
    else if (era == (uint8_t)ERA_GOLD)
        bend_q16 = 3000u;
    else
        bend_q16 = 2200u;
    return (uint32_t)(((uint64_t)tgt_inc * (65536u + bend_q16 - (bend_q16 * age) / bend_samples)) >> 16);
}

static inline uint32_t hiphop_bass_ph_inc(const dub_synth_t *s, uint32_t inc, uint8_t era, uint8_t model)
{
    (void)model;
    if (s->sub_ph == 0u) {
        hiphop_bass_age = 0u;
        hiphop_sub_oct_ph = 0u;
    } else if (hiphop_bass_age < 0xFFFFu)
        hiphop_bass_age++;
    if (hiphop_clamp_era(era) <= (uint8_t)ERA_BOOM)
        return hiphop_bass_bent_inc(s, inc, era);
    return inc;
}

static inline int32_t hiphop_degrade_sample(int32_t x, uint32_t *noise, uint8_t amt)
{
    int32_t nz;
    uint8_t sh;

    if (amt < 8u || !noise)
        return x;
    sh = (uint8_t)(6u + (amt >> 5));
    if (sh > 10u)
        sh = 10u;
    nz = dub_noise(noise) >> sh;
    if (amt >= 40u)
        x = (x >> 2) << 2;
    if (amt >= 64u)
        x = (x >> 3) << 3;
    if ((amt & 3u) == 0u)
        x = (x * (256 - (int32_t)(amt >> 1))) >> 8;
    if (amt >= 80u && ((dub_noise(noise) >> 9) & 31u) == 0u)
        x = 0;
    return x + nz;
}

static inline int32_t hiphop_bass_sample_finish(int32_t raw, dub_synth_t *s, uint8_t era)
{
    uint8_t damt;

    era = hiphop_clamp_era(era);
    if (era > (uint8_t)ERA_BOOM)
        return raw;
    damt = hiphop_bass_degrade_amt(era);
    raw = hiphop_degrade_sample(raw, &s->noise, damt);
    if (era <= (uint8_t)ERA_GOLD && s->sub_atk_left == 0u && s->sub_env > 0)
        raw = (raw * (int32_t)s->sub_env) >> 15;
    if (s->sub_atk_left > 0u)
        raw += (dub_noise(&s->noise) >> 10) * (int32_t)(2u + (s->sub_atk_left >> 3));
    return raw;
}

static inline int32_t hiphop_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                        int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;
    uint32_t ph_inc = hiphop_bass_ph_inc(s, inc, p->era, model);
    uint32_t so_ph = (hiphop_sub_oct_ph += ph_inc >> 1);

    (void)lfo_mod_b;
    switch (model) {
    case BASS_SUB: {
        /* Fat sub: fund + octave, no bright self-harm; dark LP */
        int32_t fund, subo;
        s->sub_ph += ph_inc;
        fund = dub_sine_q64(s->sub_ph);
        subo = dub_sine_q64(so_ph);
        raw = fund + ((subo * 3) >> 2);
        dub_svf_mode(raw, 260 + (fc >> 4), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 12u ? 12u : p->reso_mode), &raw);
        break;
    }
    case BASS_MOOG: {
        /* Warm amp: sine-led + soft saw, low reso (old +28 screamed metal) */
        int32_t saw_w, mix, dark_fc, reso;
        s->sub_ph += ph_inc;
        saw_w = dub_saw(s->sub_ph);
        mix = dub_sine_q64(s->sub_ph) + (saw_w >> 2) + (dub_sine_q64(so_ph) >> 1);
        dark_fc = 220 + (fc >> 2);
        reso = (int32_t)p->reso_mode >> 1;
        if (reso > 18)
            reso = 18;
        dub_svf_mode(mix, dark_fc, &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)reso, &raw);
        dub_svf_mode(raw, dark_fc, &s->sub_svf_b1, &s->sub_svf_b2, (uint8_t)(reso >> 1), &raw);
        break;
    }
    case BASS_PLUCK: {
        int32_t env_mod, plk_fc;
        s->sub_ph += ph_inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_saw(s->sub_ph) >> 3);
        env_mod = ((int32_t)(s->sub_env >> 3) * (24 + (int32_t)(p->wobble >> 1))) >> 6;
        plk_fc = 240 + (fc >> 2) + env_mod;
        if (plk_fc < 220)
            plk_fc = 220;
        dub_svf_mode(raw, plk_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 16u ? 16u : p->reso_mode), &raw);
        raw = (raw * (int32_t)s->sub_env) >> 15;
        break;
    }
    case BASS_REESE: {
        /* Detuned sines + hint of saw — triple-saw was the metal */
        uint32_t det = (ph_inc >> 5) + (uint32_t)(wob_all >> 11);
        int32_t s1, s2, s3;
        s->sub_ph += ph_inc;
        s->sub_ph2 += ph_inc + det;
        s->sub_ph3 += ph_inc - det;
        s1 = dub_sine_q64(s->sub_ph);
        s2 = dub_sine_q64(s->sub_ph2);
        s3 = dub_sine_q64(s->sub_ph3);
        raw = ((s1 + s2 + s3) / 3) + (dub_saw(s->sub_ph) >> 4) + (dub_sine_q64(so_ph) >> 1);
        dub_svf_mode(raw, 240 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 18u ? 18u : p->reso_mode), &raw);
        break;
    }
    case BASS_808: {
        int32_t fund, subo;
        s->sub_ph += ph_inc;
        fund = dub_sine_q64(s->sub_ph);
        subo = dub_sine_q64(so_ph);
        /* No odd-harmonic ping — fat 808 is sine+sub only */
        raw = fund + ((subo * 3) >> 2);
        dub_svf_mode(raw, 300 + (fc >> 4), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 16u ? 16u : p->reso_mode), &raw);
        break;
    }
    case BASS_FM: {
        /* Soft FM bell → dark rubber; old index tracked cutoff_b into clang */
        int32_t mod2;
        s->sub_ph += ph_inc;
        s->sub_ph2 += ph_inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2 + ((uint32_t)s->sub_fm_fb << 15));
        s->sub_fm_fb = (mod2 * (10 + (int32_t)(p->wobble >> 2))) >> 8;
        mod2 = (mod2 * (6 + (int32_t)(p->cutoff_b >> 3))) >> 7;
        raw = dub_sine_q64(s->sub_ph + ((uint32_t)mod2 << 14));
        raw += dub_sine_q64(so_ph) >> 1;
        dub_svf_mode(raw, 280 + (fc >> 3), &s->sub_svf_l1, &s->sub_svf_l2, 8u, &raw);
        break;
    }
    case BASS_PWM: {
        uint32_t duty;
        int32_t pulse;
        s->sub_ph += ph_inc;
        duty = 0x28000000u + ((uint32_t)(p->cutoff_b >> 1) << 22);
        pulse = (s->sub_ph < duty) ? 9000 : -9000;
        raw = (pulse >> 1) + dub_sine_q64(s->sub_ph) + (dub_sine_q64(so_ph) >> 1);
        dub_svf_mode(raw, 260 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 14u ? 14u : p->reso_mode), &raw);
        break;
    }
    case BASS_WOOZY: {
        uint32_t det = (ph_inc >> 4) + (uint32_t)(wob_all >> 10);
        s->sub_ph += ph_inc;
        s->sub_ph2 += ph_inc + det;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph2) >> 1);
        dub_svf_mode(raw, 280 + (fc >> 3) + (wob_all >> 13), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 14u ? 14u : p->reso_mode), &raw);
        break;
    }
    case BASS_SINE:
    default: {
        s->sub_ph += ph_inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(so_ph) >> 1);
        dub_svf_mode(raw, 300 + (fc >> 4), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode > 14u ? 14u : p->reso_mode), &raw);
        break;
    }
    }
    return hiphop_bass_sample_finish(raw, s, p->era);
}

/* --- Lane-3 lead env / gate / len / bend (HP-shaped, no pluck path). --- */
typedef struct {
    uint8_t hold_pct;
    uint8_t env_floor_e8;
    uint8_t rel_decay_sh;
    uint8_t gate_cs;
} hiphop_lead_sustain_t;

static const hiphop_lead_sustain_t HIPHOP_LEAD_SUSTAIN[HIPHOP_ERA_N] = {
    { 36, 3, 12, 20 }, /* PARK   — vinyl scratch / crossfader chop */
    { 40, 3, 11, 22 }, /* GOLD   — boom-bap scratch */
    { 46, 4, 10, 24 }, /* BOOM   — swung scratch gestures */
    { 56, 4,  9, 22 }, /* GRIME  — grimy hits */
    { 32, 2,  9, 12 }, /* TRAP   — short chirp scratches */
    { 70, 6, 12, 32 }, /* CLOUD  — soft melodic smear */
};

static inline const hiphop_lead_sustain_t *hiphop_lead_sustain(uint8_t era)
{
    return &HIPHOP_LEAD_SUSTAIN[hiphop_clamp_era(era)];
}

enum {
    HIPHOP_LEAD_FLUTE = 0, HIPHOP_LEAD_SAX, HIPHOP_LEAD_MUTE_TRP,
    HIPHOP_LEAD_CLAV, HIPHOP_LEAD_HORN, HIPHOP_LEAD_EP, HIPHOP_LEAD_TIMBRE_N
};

static inline uint8_t hiphop_lead_timbre(uint8_t era)
{
    static const uint8_t timbre[HIPHOP_ERA_N] = {
        HIPHOP_LEAD_FLUTE, HIPHOP_LEAD_SAX, HIPHOP_LEAD_MUTE_TRP,
        HIPHOP_LEAD_CLAV, HIPHOP_LEAD_HORN, HIPHOP_LEAD_EP
    };

    return timbre[hiphop_clamp_era(era)];
}

static inline uint8_t hiphop_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = (uint8_t)(12u + ((uint32_t)decay * 5u) / 127u);

    era = hiphop_clamp_era(era);
    if (era == (uint8_t)ERA_CLOUD)
        body = (uint8_t)(body + 3u);
    else if (era == (uint8_t)ERA_PARK || era == (uint8_t)ERA_BOOM)
        body = (uint8_t)(body + 2u);
    else
        body = (uint8_t)(body + 1u);
    return body;
}

static inline uint8_t hiphop_env_body_gate(uint8_t decay, int8_t lane_gate, uint8_t era)
{
    int32_t g = (int32_t)lane_gate;
    uint8_t body;

    body = hiphop_env_body(decay, era);
    if (g < 0)
        g = -g;
    body = (uint8_t)(body + (g >> 5));
    if (body > 127u)
        body = 127u;
    return body;
}

enum { HIPHOP_LEAD_LEN_N = 15u };

/* Same GATE note-value labels as HP lead. */
static const char *const HIPHOP_LEAD_LEN_NAMES[HIPHOP_LEAD_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t HIPHOP_LEAD_LEN_NUM[HIPHOP_LEAD_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t HIPHOP_LEAD_LEN_DEN[HIPHOP_LEAD_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
/* HIPHOP_LEAD_LEN_16THS lives in hiphop_data.h */

static inline uint8_t hiphop_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)HIPHOP_LEAD_LEN_N)
        ix = (int32_t)HIPHOP_LEAD_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t hiphop_lead_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = hiphop_lead_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)HIPHOP_LEAD_LEN_16THS[ix];
}

static inline uint8_t hiphop_lead_bend_semi(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

static inline uint8_t hiphop_lead_bend_plock(uint8_t bend_semi)
{
    if (bend_semi < 3u)
        return 0u;
    return (uint8_t)(32u + ((uint32_t)bend_semi * 95u) / 12u);
}

#define HIPHOP_LEAD_BEND_SAMPLES ((DUB_FS * 55u) / 1000u)

static inline int hiphop_lead_is_scratch_gesture(uint8_t era)
{
    return hiphop_clamp_era(era) <= (uint8_t)ERA_TRAP || hiphop_clamp_era(era) == (uint8_t)ERA_CLOUD;
}

static inline uint32_t hiphop_scratch_bend_samples(uint8_t era, uint8_t drag)
{
    uint32_t ms;

    era = hiphop_clamp_era(era);
    if (era == (uint8_t)ERA_PARK)
        ms = 32u;
    else if (era == (uint8_t)ERA_GOLD)
        ms = 26u;
    else if (era == (uint8_t)ERA_BOOM)
        ms = 20u;
    else if (era == (uint8_t)ERA_GRIME)
        ms = 18u;
    else if (era == (uint8_t)ERA_TRAP)
        ms = 14u;
    else if (era == (uint8_t)ERA_CLOUD)
        ms = 52u;
    else
        return 0u;
    {
        uint32_t base = (DUB_FS * ms) / 1000u;
        uint32_t snap = (DUB_FS * 22u) / 1000u;

        if (drag > 96u)
            base = snap + ((uint32_t)(127u - drag) * 8u);
        else if (drag > 64u)
            base = base - ((uint32_t)(drag - 64u) * (base / 96u));
        else
            base = base + ((uint32_t)(64u - drag) * (base >> 3));
        if (base < snap)
            base = snap;
        return base;
    }
}

static inline uint32_t hiphop_lead_bent_inc(const dub_synth_t *s, uint32_t inc, uint8_t era, uint8_t drag)
{
    uint32_t n = s->siren_note_age;

    era = hiphop_clamp_era(era);
    if (hiphop_lead_is_scratch_gesture(era)) {
        uint32_t bend_samples = hiphop_scratch_bend_samples(era, drag);
        uint32_t peak_q16;

        if (bend_samples > 0u && n < bend_samples) {
            uint32_t mid = bend_samples >> 1;

            if (era == (uint8_t)ERA_PARK)
                peak_q16 = 2700u;
            else if (era == (uint8_t)ERA_GOLD)
                peak_q16 = 2300u;
            else if (era == (uint8_t)ERA_BOOM)
                peak_q16 = 1900u;
            else if (era == (uint8_t)ERA_GRIME)
                peak_q16 = 1700u;
            else if (era == (uint8_t)ERA_CLOUD)
                peak_q16 = 1100u;
            else
                peak_q16 = 1500u;
            if (mid == 0u)
                mid = 1u;
            if (n <= mid)
                peak_q16 = (peak_q16 * n) / mid;
            else
                peak_q16 = peak_q16 - (peak_q16 * (n - mid)) / (bend_samples - mid);
            inc = (uint32_t)(((uint64_t)inc * (65536u + peak_q16)) >> 16);
        }
    }

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        uint32_t depth = (uint32_t)s->siren_hp_bend;
        uint32_t abs_semi = (uint32_t)(semi >= 0 ? semi : -semi);
        uint32_t bend_q16 = abs_semi * 3400u * depth / 127u;

        if (bend_q16 > 0u && n < HIPHOP_LEAD_BEND_SAMPLES) {
            if (semi > 0)
                inc = (uint32_t)(((uint64_t)inc * (65536u - bend_q16 + (bend_q16 * n) / HIPHOP_LEAD_BEND_SAMPLES)) >> 16);
            else
                inc = (uint32_t)(((uint64_t)inc * (65536u + bend_q16 - (bend_q16 * n) / HIPHOP_LEAD_BEND_SAMPLES)) >> 16);
        }
    }
    return inc;
}

static inline uint16_t hiphop_siren_atk_samples(uint8_t era, uint8_t atk)
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

static inline void hiphop_lead_note_gate_beat(uint8_t era, uint8_t beat, uint8_t trig_cnt, int8_t lane_gate,
                                              uint16_t *hold_out, uint16_t *rel_out,
                                              uint8_t len_plock_on, uint8_t len_ix,
                                              uint32_t samples_per_step)
{
    static const uint16_t role_scale_q8[4] = { 280u, 220u, 320u, 200u };
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;
    uint8_t role = (uint8_t)((trig_cnt - 1u) & 3u);
    const hiphop_lead_sustain_t *su = hiphop_lead_sustain(era);
    uint8_t gate_cs = hiphop_beat_lead_gate_cs(beat);

    if (len_plock_on != 0u && samples_per_step > 0u) {
        total = hiphop_lead_len_samples(samples_per_step, len_ix);
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

static inline int32_t hiphop_lead_release_decay_shift(uint8_t era)
{
    return (int32_t)hiphop_lead_sustain(era)->rel_decay_sh;
}

static inline void hiphop_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    if ((s->siren_atk_left > 0u || s->siren_gate > 0u || s->siren_lead_rel > 0u || s->siren_env > 0)
        && s->siren_note_age < 65535u)
        s->siren_note_age++;

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
        const hiphop_lead_sustain_t *su = hiphop_lead_sustain(s->voice_era);
        int32_t sustain_floor = (s->siren_peak * (int32_t)su->env_floor_e8) >> 3;

        if (s->siren_env > sustain_floor)
            s->siren_env -= ((s->siren_env - sustain_floor) >> 12) + 1;
        else
            s->siren_env = sustain_floor;
        s->siren_gate--;
    } else if (s->siren_lead_rel > 0u) {
        int32_t sh = hiphop_lead_release_decay_shift(s->voice_era);

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

static inline int32_t hiphop_siren_decay_shift(uint8_t era, uint8_t drm_dec)
{
    int32_t sh = 12 + (((int32_t)drm_dec - 64) >> 5);

    era = hiphop_clamp_era(era);
    if (era == (uint8_t)ERA_CLOUD)
        sh += 1;
    else if (era == (uint8_t)ERA_GRIME)
        sh += 2;
    if (sh < 11)
        sh = 11;
    else if (sh > 15)
        sh = 15;
    return sh;
}

static inline int32_t hiphop_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}

enum { HIPHOP_SCRATCH_LAYERS = 6u };

static inline uint8_t hiphop_scratch_crush_amt(uint8_t era)
{
    uint8_t damt = hiphop_bass_degrade_amt(era);

    if (era != (uint8_t)ERA_PARK)
        damt = (uint8_t)(damt >> 1);
    if (damt < 28u)
        damt = (uint8_t)(28u + (era & 3u) * 3u);
    if (damt > 40u)
        damt = 40u;
    return damt;
}

/* Soft overlapping windows only — no clicks (reverb-safe). */
static inline int32_t hiphop_scratch_layer_env(uint32_t age, uint32_t onset, uint32_t dur)
{
    int32_t a;

    if (age < onset || dur < 3u)
        return 0;
    a = (int32_t)(age - onset);
    if ((uint32_t)a >= dur)
        return 0;
    {
        /* 48000 · a(dur − a) / dur² in 32-bit (no libgcc __divdi3 on the target). */
        uint32_t ua = (uint32_t)a;
        uint32_t ud = dur;
        uint32_t f;
        uint32_t t;

        while (ud >= 65536u) {
            ud >>= 1;
            ua >>= 1;
        }
        f = (ua << 15) / ud;
        t = (f * (32768u - f)) >> 15;
        return (int32_t)((t * 48000u) >> 15);
    }
}

static inline int32_t hiphop_scratch_layer_sample(dub_synth_t *s, int32_t bright, uint32_t age,
                                                uint8_t li, uint32_t tick, int32_t nz_base)
{
    static const uint8_t onset_q[HIPHOP_SCRATCH_LAYERS] = { 0, 2, 4, 6, 9, 12 };
    static const uint8_t dur_q[HIPHOP_SCRATCH_LAYERS] = { 12, 11, 13, 10, 14, 12 };
    static const uint8_t lp_sh[HIPHOP_SCRATCH_LAYERS] = { 1, 2, 2, 3, 3, 4 };
    static const uint8_t gain_sh[HIPHOP_SCRATCH_LAYERS] = { 2, 2, 3, 3, 4, 4 };
    uint32_t scale = tick >> 2;
    int32_t env, nz, src;

    if (scale < 8u)
        scale = 8u;
    env = hiphop_scratch_layer_env(age, (uint32_t)onset_q[li] * scale / 4u,
                                   (uint32_t)dur_q[li] * scale / 3u);
    if (env <= 0)
        return 0;
    nz = nz_base + (dub_noise(&s->noise) >> (4 + (li & 1u)));
    src = (nz + (nz >> lp_sh[li]) + (nz >> (lp_sh[li] + 1))) >> 2;
    src += (dub_noise(&s->noise) >> (5 + gain_sh[li]));
    src = (src >> gain_sh[li]) + ((src * bright) >> 13);
    return (src * env) >> 18;
}

/* Vinyl scratch: staggered mid smear layers, dull noise body (no PCM, no SVF). */
static inline int32_t hiphop_scratch_voice(dub_synth_t *s, uint32_t inc, int32_t bright, uint8_t era)
{
    int32_t nz, smear, raw, layer_sum;
    uint8_t damt, li;
    uint32_t age = s->siren_note_age;
    uint32_t tick = hiphop_scratch_bend_samples(era, 64u);

    (void)inc;
    nz = dub_noise(&s->noise);
    smear = (nz + (nz >> 1) + (nz >> 2) + (nz >> 3)) >> 2;
    layer_sum = 0;
    for (li = 0; li < HIPHOP_SCRATCH_LAYERS; li++)
        layer_sum += hiphop_scratch_layer_sample(s, bright, age, li, tick, nz);
    raw = smear + layer_sum + ((smear * bright) >> 12);
    damt = hiphop_scratch_crush_amt(era);
    raw = hiphop_degrade_sample(raw, &s->noise, damt);
    return raw;
}

/* Bypass resonant SVF: warm 1-pole LP so scratch doesn’t ring the reverb tank. */
static inline int32_t hiphop_scratch_warm_out(dub_synth_t *s, int32_t raw, int32_t env, int32_t bright)
{
    int32_t warm;

    (void)bright;
    s->lead_cab_lp += (raw - s->lead_cab_lp) >> 3;
    warm = (raw + s->lead_cab_lp + (s->lead_cab_lp >> 1)) >> 2;
    return (warm * env) >> 15;
}

/* Dusty LP swoosh after chop — no HF chirplets (reverb-safe tail). */
static inline int32_t hiphop_scratch_dust_trail(dub_synth_t *s, const dub_params_t *p)
{
    int32_t nz, dust, env_q15, out;
    uint32_t n = s->siren_note_age;
    uint8_t era = hiphop_clamp_era(p->era);
    uint8_t drag = p->siren_rate ? p->siren_rate : 64u;
    uint32_t body = hiphop_scratch_bend_samples(era, drag);
    uint32_t xwin = body + body + (body >> 1);
    uint32_t tail = (DUB_FS * (10u + ((uint32_t)drag * 22u) / 127u)) / 1000u;

    if (n < (body >> 1) || n > xwin + tail)
        return 0;
    if (n <= xwin)
        env_q15 = (int32_t)(((n - (body >> 1)) * 12000u) / (xwin - (body >> 1) + 1u));
    else
        env_q15 = (int32_t)(28000 - ((n - xwin) * 28000u) / (tail + 1u));
    if (env_q15 < 0)
        env_q15 = 0;
    nz = dub_noise(&s->noise);
    /* lead_amp_hp_lp: slow LP dust state during scratch only (reset on note-on). */
    s->lead_amp_hp_lp += (nz - s->lead_amp_hp_lp) >> 4;
    dust = (s->lead_amp_hp_lp + (s->lead_amp_hp_lp >> 1) + (nz >> 3)) >> 2;
    out = (dust * env_q15) >> 15;
    out = (out * (72 + ((int32_t)drag * 24) / 127)) >> 7;
    return (out * s->siren_env) >> 16;
}

/* CLOUD / GRIME melodic lane: 1–2 oscillators + SVF, era timbre switch. */
static inline int32_t hiphop_lead_voice_osc(dub_synth_t *s, uint32_t inc, int32_t env, uint8_t timbre,
                                          int32_t bright)
{
    uint32_t ph = s->stab_ph[0];

    s->stab_ph[0] += inc;
    switch (timbre) {
    case HIPHOP_LEAD_FLUTE: {
        int32_t air = (dub_noise(&s->noise) * bright) >> 10;
        int32_t tone = dub_sine_q64(ph);

        return tone + air;
    }
    case HIPHOP_LEAD_SAX: {
        int32_t mod = dub_sine_q64(ph << 1);
        int32_t idx = (mod * (24 + (bright >> 2))) >> 10;

        return dub_sine_q64(ph + (uint32_t)(idx << 14));
    }
    case HIPHOP_LEAD_MUTE_TRP: {
        int32_t mod = dub_sine_q64(ph + (ph >> 1));
        int32_t tone = dub_sine_q64(ph + (uint32_t)((mod * (12 + (bright >> 3))) >> 9));

        return (tone * 7) >> 3;
    }
    case HIPHOP_LEAD_CLAV: {
        int32_t mod1 = dub_sine_q64(ph << 1);
        int32_t mod2 = dub_sine_q64(ph + (ph << 1));
        int32_t idx = ((mod1 + (mod2 >> 1)) * (20 + (bright >> 1))) >> 10;

        return dub_sine_q64(ph + (uint32_t)(idx << 13));
    }
    case HIPHOP_LEAD_HORN: {
        int32_t mod = dub_sine_q64(ph << 1);
        int32_t sawv = dub_saw(ph) >> 2;
        int32_t tone = dub_sine_q64(ph + (uint32_t)((mod * (32 + bright)) >> 9)) >> 2;

        return sawv + tone;
    }
    case HIPHOP_LEAD_EP:
    default: {
        int32_t mod = dub_sine_q64(ph << 1) + (dub_sine_q64(ph + (ph << 1)) >> 1);
        int32_t idx = (mod * (16 + (bright >> 1))) >> 10;
        int32_t tone = dub_sine_q64(ph + (uint32_t)(idx << 13));

        return tone + (dub_sine_q64(ph << 1) >> 3);
    }
    }
    (void)env;
}

static inline int32_t hiphop_lead_tone(dub_synth_t *s, int32_t raw, uint8_t timbre, int32_t env, int32_t fc,
                                     uint8_t reso)
{
    int32_t out = raw;

    dub_svf_mode(out, fc, &s->lead_cab_lp, &s->lead_cab_bp, reso, &out);
    if (timbre == HIPHOP_LEAD_MUTE_TRP) {
        dub_svf_mode(out, fc + (fc >> 2), &s->lead_amp_treb_lp, &s->lead_amp_treb_bp,
                     (uint8_t)(reso >> 1), &out);
    } else if (timbre == HIPHOP_LEAD_HORN) {
        dub_svf_mode(out, fc + (fc >> 1), &s->lead_amp_mid_lp, &s->lead_amp_mid_bp, reso, &out);
    } else if (timbre == HIPHOP_LEAD_FLUTE) {
        dub_svf_mode(out, fc + (fc >> 2), &s->lead_amp_hp_lp, &s->lead_amp_hp_bp,
                     (uint8_t)(reso >> 2), &out);
    }
    return (out * env) >> 15;
}

static inline int32_t hiphop_lead_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, root_m, bright, fc;
    uint32_t inc;
    uint8_t era = hiphop_clamp_era(p->era);
    uint8_t timbre = hiphop_lead_timbre(era);

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 20 / 64;
    root_m = (int32_t)(p->root ? p->root : 60u) + kt_s + (int32_t)s->siren_st_tune + 12;
    if (root_m < 55)
        root_m = 55;
    else if (root_m > 84)
        root_m = 84;

    {
        uint8_t drag = p->siren_rate ? p->siren_rate : 64u;

        inc = hiphop_lead_bent_inc(s, dub_note_inc(dub_clamp_midi((uint8_t)root_m)), era, drag);
        bright = 48 + ((int32_t)drag * 56) / 127;
        fc = 900 + ((int32_t)p->cutoff_c * 120) + (bright * 8);
        if (fc > 14000)
            fc = 14000;

        if (hiphop_lead_is_scratch_gesture(era)) {
            (void)timbre;
            (void)fc;
            return hiphop_scratch_warm_out(s, hiphop_scratch_voice(s, inc, bright, era), env, bright);
        }

        {
            int32_t v1 = hiphop_lead_voice_osc(s, inc, env, timbre, bright);
            int32_t v2 = 0;

            if (timbre == HIPHOP_LEAD_EP || timbre == HIPHOP_LEAD_CLAV) {
                s->stab_ph[1] += inc;
                v2 = dub_sine_q64(s->stab_ph[1] + (inc >> 2)) >> 2;
            }
            return hiphop_lead_tone(s, v1 + v2, timbre, env, fc, p->reso_mode);
        }
    }
}

/* Same lesson as Jazz: 38 Q12 → near inaudible vs drums. */
#define HIPHOP_LEAD_OUT_GAIN_Q12 1600u

static inline int32_t hiphop_scratch_xfade_q15(uint32_t n, uint8_t era, uint8_t drag)
{
    uint32_t win = hiphop_scratch_bend_samples(era, drag);

    if (win == 0u)
        return 32767;
    win += win + (win >> 1);
    if (n >= win)
        return 0;
    if (n < 18u)
        return (int32_t)((n * 32767u) / 18u);
    if (n < win / 2u)
        return (int32_t)(32767u - ((n - 18u) * 22000u) / (win / 2u - 18u + 1u));
    return (int32_t)(32767u - ((n - win / 2u) * 32767u) / (win - win / 2u + 1u));
}

static inline int32_t hiphop_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t lfo_mod_c)
{
    int32_t sig, lvl, xf;
    uint32_t n = s->siren_note_age;
    uint8_t era = hiphop_clamp_era(p->era);

    (void)lfo_mod_c;
    sig = hiphop_lead_sample(s, p, s->siren_env);
    if (n < 120u) {
        int32_t p_env = (int32_t)(120u - n);
        int32_t atk = dub_noise(&s->noise);

        if (hiphop_lead_is_scratch_gesture(era)) {
            atk = atk >> 4;
            atk = (atk * p_env * p_env) >> 14;
            sig += (atk * s->siren_env) >> 17;
        } else {
            atk = atk >> 3;
            atk = (atk * p_env) >> 9;
            sig += (atk * s->siren_env) >> 16;
        }
    }
    if (hiphop_lead_is_scratch_gesture(era)) {
        uint8_t drag = p->siren_rate ? p->siren_rate : 64u;
        int32_t trail;

        xf = hiphop_scratch_xfade_q15(n, era, drag);
        if (xf < 0)
            xf = 0;
        sig = (sig * xf) >> 15;
        trail = hiphop_scratch_dust_trail(s, p);
        sig += trail;
        s->lead_cab_body_lp += (sig - s->lead_cab_body_lp) >> 2;
        /* ×4 (+12 dB): zero-mean noise lost the old DC level */
        sig = (sig + s->lead_cab_body_lp) * 2;
    }
    /* SCRATCH LEVEL = siren_pitch (lane 3 desk level). */
    sig = (sig * (int32_t)(p->siren_pitch ? p->siren_pitch : 96u)) >> 7;
    lvl = p->lvl_c ? p->lvl_c : 96u;
    sig = (sig * (int32_t)lvl) / 92;
    sig = dub_softclip_q15((sig * 420) >> 8);
    sig = (sig * (int32_t)HIPHOP_LEAD_OUT_GAIN_Q12) >> 12;
    return dub_softclip_q15(sig);
}

/* --- Stutter buffer + bit-degrade (era-scaled; wired from fm_app Task 3). --- */
enum { HIPHOP_STUTTER_TAP_N = 2048u };

enum { HIPHOP_STUT_HACK = 0, HIPHOP_STUT_SCREW = 1, HIPHOP_STUT_GEST = 2 };

typedef struct {
    int16_t ring[HIPHOP_STUTTER_TAP_N];
    uint16_t wpos;
    uint16_t rpos;
    uint16_t slice_len;
    uint16_t slice_left;
    uint8_t active;
    uint8_t wet;
    uint8_t reverse;
    uint8_t mode; /* HIPHOP_STUT_* */
    int32_t f_lp; /* 1-pole for iso colour */
} hiphop_stutter_t;

static inline uint8_t hiphop_era_degrade_base(uint8_t era)
{
    /* WARM/MELO quiet; GLITCH→OBLIQ light grit (chop via stutter, not noise floor) */
    static const uint8_t base[HIPHOP_ERA_N] = { 6, 8, 20, 24, 22, 16 };

    return base[hiphop_clamp_era(era)];
}

static inline uint8_t hiphop_degrade_amount(uint8_t era, uint8_t macro_grit, uint8_t edit_density)
{
    uint32_t amt = (uint32_t)hiphop_era_degrade_base(era) + ((uint32_t)macro_grit >> 2) +
                   ((uint32_t)edit_density >> 3);

    if (amt > 127u)
        amt = 127u;
    return (uint8_t)amt;
}

static inline void hiphop_stutter_push(hiphop_stutter_t *st, int32_t sample)
{
    if (!st)
        return;
    st->ring[st->wpos % HIPHOP_STUTTER_TAP_N] = (int16_t)(sample >> 16);
    st->wpos++;
}

static inline int32_t hiphop_stutter_mix(hiphop_stutter_t *st, int32_t dry, uint8_t wet_u7)
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
    idx = st->reverse ? (uint16_t)(st->rpos + st->slice_left - 1u) : st->rpos;
    tap = (int32_t)st->ring[idx % HIPHOP_STUTTER_TAP_N] << 16;
    st->slice_left--;
    if (st->slice_left == 0u)
        st->rpos = (uint16_t)((st->rpos + 1u) % HIPHOP_STUTTER_TAP_N);

    if (st->mode == (uint8_t)HIPHOP_STUT_SCREW) {
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
    } else if (st->mode == (uint8_t)HIPHOP_STUT_HACK) {
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

static inline uint16_t hiphop_stutter_slice_from_vel(uint8_t vel, uint8_t era)
{
    uint16_t base = (uint16_t)(28u + ((uint32_t)vel * 88u) / 127u);
    uint8_t e = hiphop_clamp_era(era);

    if (e >= (uint8_t)ERA_GRIME)
        base = (uint16_t)((base * 3u) / 4u);
    if (e >= (uint8_t)ERA_BOOM)
        base = (uint16_t)((base * 5u) / 6u);
    if (e >= (uint8_t)ERA_TRAP)
        base = (uint16_t)(base >> 1);
    if (e == (uint8_t)ERA_CLOUD)
        base = (uint16_t)((base * 5u) / 4u); /* longer oblique freezes */
    if (base < 6u)
        base = 6u;
    if (base > 512u)
        base = 512u;
    return base;
}

#endif
