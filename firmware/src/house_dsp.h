/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HOUSE_DSP_H
#define HOUSE_DSP_H

#include "house_data.h"

static const int8_t house_kit_k_note[DKIT_N]  = { 31, 36, 30, 38, 40, 42, 35, 37 };
static const uint8_t house_kit_k_sweep[DKIT_N] = { 10, 32, 22, 18, 22, 12, 14, 18 };
static const uint8_t house_kit_k_fm[DKIT_N]   = {  2, 24,  0,  6, 18,  4,  6, 10 };
static const uint8_t house_kit_k_drv[DKIT_N]  = { 18, 22, 10, 14, 20, 10, 14, 18 };
static const int8_t house_kit_r_note[DKIT_N]  = { 76, 60, 68, 71, 63, 79, 58, 74 };
static const uint8_t house_kit_r_wire[DKIT_N] = { 92, 78, 86, 88, 96, 72, 98, 84 };
static inline int32_t house_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                        int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;

    (void)lfo_mod_b;
    switch (model) {
    case BASS_ORGAN: {
        uint32_t det = inc >> 4;
        int32_t fund, oct, fifth, sub, click;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc + (inc >> 1);
        sub = dub_sine_q64(s->sub_ph);
        fund = dub_square(s->sub_ph) >> 1;
        oct = dub_square(s->sub_ph >> 1) >> 2;
        fifth = dub_square(s->sub_ph3) >> 2;
        click = s->sub_env > 24000 ? (int32_t)(s->sub_env >> 9) : 0;
        raw = sub + fund + oct + fifth + click;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_OCTAVE: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 1)
            + (dub_square(s->sub_ph >> 1) >> 3);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_MOOG: {
        int32_t saw_w, fund;
        s->sub_ph += inc;
        fund = dub_sine_q64(s->sub_ph);
        saw_w = dub_saw(s->sub_ph);
        dub_svf_mode(saw_w, fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 20u > 127u ? 127u : p->reso_mode + 20u), &raw);
        dub_svf_mode(raw, fc, &s->sub_svf_b1, &s->sub_svf_b2, (uint8_t)(p->reso_mode >> 1), &raw);
        raw = (fund >> 1) + raw;
        break;
    }
    case BASS_PLUCK: {
        int32_t env_mod, plk_fc;
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph);
        env_mod = ((int32_t)(s->sub_env >> 3) * (48 + (int32_t)p->wobble)) >> 6;
        plk_fc = fc + env_mod;
        if (plk_fc < 260)
            plk_fc = 260;
        else if (plk_fc > 26000)
            plk_fc = 26000;
        dub_svf_mode(raw, plk_fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)s->sub_env) >> 15;
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
        raw = ((s1 + s2 + s3) / 3) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_808: {
        int32_t fund, harm;
        s->sub_ph += inc;
        fund = dub_sine_q64(s->sub_ph);
        harm = dub_sine_q64(s->sub_ph << 1);
        raw = fund + (harm >> 2);
        dub_svf_mode(raw, 1200 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_303: {
        int32_t saw_w, sq_w, acid_fc, fold_amt;
        uint8_t reso_hi;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        sq_w = dub_square(s->sub_ph + ((uint32_t)wob_all << 8));
        raw = saw_w + (sq_w >> 1);
        acid_fc = fc + (((s->sub_env >> 3) * (int32_t)p->wobble) >> 6);
        if (acid_fc < 220)
            acid_fc = 220;
        else if (acid_fc > 22000)
            acid_fc = 22000;
        reso_hi = (uint8_t)(p->reso_mode + 22u > 127u ? 127u : p->reso_mode + 22u);
        dub_svf_mode(raw, acid_fc, &s->sub_svf_l1, &s->sub_svf_l2, reso_hi, &raw);
        raw = (raw * 7) >> 3;
        if (p->reso_mode > 76u) {
            fold_amt = 256 + (int32_t)p->wobble * 2 + (int32_t)p->reso_mode;
            raw = (raw * fold_amt) >> 8;
            while (raw > 32767 || raw < -32767) {
                if (raw > 32767)
                    raw = 65534 - raw;
                else
                    raw = -65534 - raw;
            }
        }
        break;
    }
    case BASS_FM: {
        int32_t mod_ix, mod3_ix, fb_amt, fund;

        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        s->sub_ph3 += inc * 3u;
        mod_ix = 24 + (int32_t)p->cutoff_b + (s->sub_env >> 9);
        fb_amt = 40 + (int32_t)p->wobble;
        mod3_ix = 12 + (int32_t)p->reso_mode + ((int32_t)p->wobble >> 1);
        fund = dub_sine_q64(s->sub_ph);
        raw = dub_fm_3op(s->sub_ph, s->sub_ph2, s->sub_ph3, &s->sub_fm_fb, mod_ix, fb_amt, mod3_ix);
        raw = (fund >> 1) + raw;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_PWM: {
        uint32_t duty;
        int32_t pulse;
        s->sub_ph += inc;
        s->sub_ph2 += 20000u + (uint32_t)p->wobble * 1200u;
        duty = 0x20000000u + ((uint32_t)p->cutoff_b << 23);
        pulse = (s->sub_ph < duty) ? 18000 : -18000;
        raw = pulse + (dub_sine_q64(s->sub_ph) >> 1) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    default:
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 1);
        dub_svf_mode(raw, 400 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    return raw;
}

/* STAB SNAP (p->siren_rate 0..127): four zones with ±4-tick crossfade at 32 / 64 / 96.
 * 0..31   M1 house piano (+1 oct CHRD); SNAP = hammer bite & FM brightness.
 * 32..63  Call/response Rhodes IV (+5) or bVII (+10 at SNAP 48..63) vs CHRD + root ghost.
 * 64..95  Garage organ / fifth-bell on [0,7,12,19]; SNAP = drawbar & FM bell index.
 * 96..127 French/disco brass upper triad; classic pitch fall when env decays below 22k.
 * Base: ch_root+12 clamped MIDI 60..72 + kt_s (drm_tune[3]) + s->siren_st_tune; stab_ph[0..7].
 */
/* 2^(semi_q8 / (12 * 256)) in Q16; semi_q8 in 1/256 semitone, |semi| ≤ 48 st. */
static inline uint32_t house_semi_ratio_q16(int32_t semi_q8)
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

static inline uint32_t house_stab_bent_inc(const dub_synth_t *s, uint32_t inc)
{
    uint32_t n = s->siren_note_age;
    uint32_t b_smp = (DUB_FS * 75u) / 1000u;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0 && n < b_smp) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        int32_t depth = (int32_t)s->siren_hp_bend;
        int32_t off_q8 = (semi * 256 * depth) / 127;

        off_q8 = (int32_t)(((int64_t)off_q8 * (int64_t)(b_smp - n)) / (int64_t)b_smp);
        if (off_q8 != 0)
            inc = (uint32_t)(((uint64_t)inc * house_semi_ratio_q16(-off_q8)) >> 16);
    }
    return inc;
}

static inline int32_t house_stab_tone(dub_synth_t *s, uint8_t ph_ix, uint8_t note, int32_t env,
                                      int32_t detune_q16, uint8_t level_shr, int32_t snap_expr)
{
    uint32_t inc = dub_note_inc(note);
    uint32_t ph;
    int32_t fm_idx, tone;

    if (detune_q16 != 0)
        inc = (uint32_t)(((uint64_t)inc * (uint64_t)(65536 + detune_q16)) >> 16);
    inc = house_stab_bent_inc(s, inc);
    ph = s->stab_ph[ph_ix];
    s->stab_ph[ph_ix] += inc;

    /* dub_fm_ep_tine: 1 index unit ≈ 0.0123 rad → 320 ≈ 3.9 rad peak */
    fm_idx = (env * env) >> 15;
    fm_idx = (fm_idx * (48 + (snap_expr >> 1))) >> 12;
    if (fm_idx < 4)
        fm_idx = 4;
    else if (fm_idx > 320)
        fm_idx = 320;
    tone = dub_fm_ep_tine(ph, ph, fm_idx);
    return tone >> level_shr;
}

static inline int32_t house_stab_pitch_inc(const dub_synth_t *s, uint8_t note, int32_t detune_q16,
                                           int32_t fall_q16)
{
    uint32_t inc = dub_note_inc(note);
    int32_t bend = detune_q16 + fall_q16;

    if (bend != 0)
        inc = (uint32_t)(((uint64_t)inc * (uint64_t)(65536 + bend)) >> 16);
    inc = house_stab_bent_inc(s, inc);
    return (int32_t)inc;
}

static inline void house_stab_snap_weights(int32_t snap, int32_t w[4])
{
    w[0] = w[1] = w[2] = w[3] = 0;
    if (snap <= 27) {
        w[0] = 256;
    } else if (snap < 36) {
        int32_t t = snap - 27;

        w[0] = 256 - t * 32;
        w[1] = t * 32;
    } else if (snap <= 59) {
        w[1] = 256;
    } else if (snap < 68) {
        int32_t t = snap - 59;

        w[1] = 256 - t * 32;
        w[2] = t * 32;
    } else if (snap <= 91) {
        w[2] = 256;
    } else if (snap < 100) {
        int32_t t = snap - 91;

        w[2] = 256 - t * 32;
        w[3] = t * 32;
    } else {
        w[3] = 256;
    }
}

static inline int32_t house_stab_zone0(dub_synth_t *s, const dub_params_t *p, int32_t env,
                                       int32_t root_m, int32_t snap_r, int32_t pk)
{
    static const int16_t stab_det[4] = { -90, 110, -140, 85 };
    int32_t raw = 0, hammer, snap_z;
    uint8_t vi;
    uint32_t hash;

    snap_z = snap_r & 31;
    for (vi = 0; vi < 4u; vi++) {
        uint8_t note = dub_clamp_midi(root_m + (int32_t)dub_chord_interval(p->voicing, vi));

        raw += house_stab_tone(s, vi, note, env, (int32_t)stab_det[vi], vi == 0u ? 1u : 2u, snap_z);
    }
    (void)pk;
    if (s->siren_note_age < 256u) {
        /* hammer transient: 5.8 ms linear decay from note-on */
        int32_t atk = 256 - (int32_t)s->siren_note_age;

        hash = (uint32_t)env ^ s->stab_ph[0];
        hash *= 69069u;
        hash ^= hash >> 16;
        hammer = (dub_square(s->stab_ph[0]) >> 3) + (((int32_t)(hash >> 16) - 32768) >> 2);
        hammer = (hammer * atk * (32 + (snap_z >> 1))) >> 14;
        raw += hammer;
    }
    return raw;
}

static inline int32_t house_stab_zone1(dub_synth_t *s, const dub_params_t *p, int32_t env,
                                       int32_t root_m, int32_t snap_r)
{
    static const int16_t stab_det[4] = { -70, 95, -120, 70 };
    int32_t tr, ctr_m, raw = 0, snap_z;
    uint8_t vi;

    snap_z = snap_r & 31;
    tr = snap_r >= 48 ? 10 : 5;
    ctr_m = root_m + tr;
    for (vi = 0; vi < 3u; vi++) {
        uint8_t note = dub_clamp_midi(ctr_m + (int32_t)dub_chord_interval(p->voicing, vi));

        raw += house_stab_tone(s, (uint8_t)(4u + vi), note, env, (int32_t)stab_det[vi], 2u, snap_z);
    }
    {
        uint8_t ghost_n = dub_clamp_midi(root_m);

        raw += house_stab_tone(s, 7u, ghost_n, env, -30, 5u, snap_z);
    }
    return raw;
}

static inline int32_t house_stab_organ_voice(dub_synth_t *s, uint8_t ph_ix, int32_t inc, int32_t env,
                                            int32_t bright)
{
    uint32_t ph;
    int32_t fund, h2, h3, h4, draw, bell_idx, mod, bell;

    ph = s->stab_ph[ph_ix];
    s->stab_ph[ph_ix] += (uint32_t)inc;
    fund = dub_square(ph) >> 2;
    h2 = dub_square(ph << 1) >> 3;
    h3 = dub_square(ph + (ph << 1)) >> 3;
    h4 = dub_square(ph << 2) >> 4;
    draw = fund + ((h2 + h3) * bright >> 7) + (h4 * bright >> 8);
    bell_idx = (bright * (env >> 7)) >> 4;
    if (bell_idx < 6)
        bell_idx = 6;
    mod = dub_sine_q64(ph << 1);
    mod = (mod * bell_idx) >> 10;
    bell = dub_sine_q64(ph + ((uint32_t)mod << 13));
    return (draw >> 1) + (bell >> 2);
}

static inline int32_t house_stab_zone2(dub_synth_t *s, int32_t env, int32_t root_m, int32_t snap_r,
                                       int32_t pk)
{
    static const int8_t organ_deg[4] = { 0, 7, 12, 19 };
    int32_t raw = 0, bright, click, snap_z;
    uint8_t vi;

    snap_z = snap_r & 31;
    bright = 96 + snap_z * 4;
    for (vi = 0; vi < 4u; vi++) {
        uint8_t note = dub_clamp_midi(root_m + (int32_t)organ_deg[vi]);
        int32_t inc = house_stab_pitch_inc(s, note, 0, 0);

        raw += house_stab_organ_voice(s, vi, inc, env, bright);
    }
    (void)pk;
    if (s->siren_note_age < 128u) {
        /* key click: 2.9 ms linear decay from note-on */
        click = dub_square(s->stab_ph[0] + 0x20000000u) >> 2;
        raw += (click * (128 - (int32_t)s->siren_note_age)) >> 8;
    }
    return raw;
}

static inline int32_t house_stab_brass_voice(dub_synth_t *s, uint8_t ph_ix, int32_t inc, int32_t env,
                                             int32_t detune_q16, int32_t fm_amt)
{
    uint32_t ph;

    if (detune_q16 != 0)
        inc = (int32_t)(((uint64_t)(uint32_t)inc * (uint64_t)(65536 + detune_q16)) >> 16);
    ph = s->stab_ph[ph_ix];
    s->stab_ph[ph_ix] += (uint32_t)inc;
    return dub_fm_brass(ph, ph, fm_amt);
}

static inline int32_t house_stab_zone3(dub_synth_t *s, const dub_params_t *p, int32_t env,
                                       int32_t root_m, int32_t snap_r)
{
    static const int16_t brass_det[4] = { -60, 80, -110, 55 };
    int32_t raw = 0, fall_amt, fall_q16, snap_z, fm_amt;
    uint8_t vi;

    snap_z = snap_r & 31;
    fall_amt = 24 + snap_z;
    fm_amt = 40 + snap_z;
    if (env < 22000)
        fall_q16 = -(((28000 - env) * fall_amt) >> 11);
    else
        fall_q16 = 0;
    for (vi = 0; vi < 4u; vi++) {
        int32_t deg = (int32_t)dub_chord_interval(p->voicing, vi);

        if (vi == 0u)
            deg += 12;
        else if (vi == 1u)
            deg += 7;
        {
            uint8_t note = dub_clamp_midi(root_m + deg);
            int32_t inc = house_stab_pitch_inc(s, note, (int32_t)brass_det[vi], fall_q16);

            raw += house_stab_brass_voice(s, (uint8_t)(4u + vi), inc, env, (vi == 0u ? 140 : 0), fm_amt);
        }
    }
    return raw;
}

static inline int32_t house_stab_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, ch_base, root_m, snap_r, w[4], mix, pk;

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 24 / 64;
    ch_base = (int32_t)(s->ch_root ? s->ch_root : (p->root ? p->root : 48u));
    root_m = ch_base + 12 + kt_s + (int32_t)s->siren_st_tune;
    if (root_m < 56)
        root_m = 56;
    else if (root_m > 76)
        root_m = 76;
    snap_r = (int32_t)p->siren_rate;
    pk = s->siren_peak > 0 ? s->siren_peak : 28000;

    house_stab_snap_weights(snap_r, w);
    mix = 0;
    if (w[0] > 0)
        mix += house_stab_zone0(s, p, env, root_m, snap_r, pk) * w[0];
    if (w[1] > 0)
        mix += house_stab_zone1(s, p, env, root_m, snap_r) * w[1];
    if (w[2] > 0)
        mix += house_stab_zone2(s, env, root_m, snap_r, pk) * w[2];
    if (w[3] > 0)
        mix += house_stab_zone3(s, p, env, root_m, snap_r) * w[3];
    mix >>= 8;

    return (mix * env) >> 15;
}

static inline uint16_t house_siren_atk_samples(uint8_t era, uint8_t atk)
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

static inline void house_stab_note_gate(uint8_t trig_cnt, int8_t lane_gate, uint16_t *hold_out,
                                        uint16_t *rel_out, uint8_t len_plock_on, uint8_t len_ix,
                                        uint32_t samples_per_step)
{
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;

    (void)trig_cnt;
    if (len_plock_on != 0u && samples_per_step > 0u) {
        total = house_stab_gate_len_samples(samples_per_step, len_ix);
        if (total < 1u)
            total = 1u;
    } else {
        total = (DUB_FS * 26u) / 100u;
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

static inline void house_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    if ((s->siren_atk_left > 0u || s->siren_gate > 0u || s->siren_lead_rel > 0u || s->siren_env > 0)
        && s->siren_note_age < 65535u)
        s->siren_note_age++;

    if (s->siren_atk_left > 0u) {
        int32_t pk = s->siren_peak;
        int32_t diff = pk - s->siren_env;
        int32_t step = diff / (int32_t)s->siren_atk_left;

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
