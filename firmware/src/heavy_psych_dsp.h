/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HEAVY_PSYCH_DSP_H
#define HEAVY_PSYCH_DSP_H

#include "heavy_psych_data.h"
#include "heavy_psych_pluck.h"

/* Kick: DOOM kit was outlier (high note + max FM/sweep) → metallic at moderate PUNCH. */
static const int8_t heavy_psych_kit_k_note[DKIT_N]  = { 30, 31, 32, 38, 39, 42, 34, 36 };
static const uint8_t heavy_psych_kit_k_sweep[DKIT_N] = { 20, 26, 24, 24, 28, 16, 18, 20 };
static const uint8_t heavy_psych_kit_k_fm[DKIT_N]   = { 14, 18, 18, 16, 24, 12, 10, 12 };
static const uint8_t heavy_psych_kit_k_drv[DKIT_N]  = { 34, 36, 30, 26, 32, 20, 24, 26 };
static const int8_t heavy_psych_kit_r_note[DKIT_N]  = { 72, 58, 66, 71, 62, 77, 56, 72 };
static const uint8_t heavy_psych_kit_r_wire[DKIT_N] = { 98, 82, 96, 100, 104, 84, 108, 94 };

static inline int32_t heavy_psych_drm_kick_shape(int32_t kick_sig, uint16_t kick_snap, int32_t k_drv_tot,
                                                 uint32_t *noise)
{
    (void)kick_snap;
    (void)noise;
    return dub_softclip_q15((kick_sig * (312 + k_drv_tot * 5)) >> 8);
}

static inline int32_t heavy_psych_drm_rim_mix(int32_t shell, int32_t wire, uint8_t kit_wire, uint8_t rim_drv)
{
    int32_t mix_r = ((shell * (54 - (kit_wire >> 4))) >> 7) + ((wire * 7) >> 4);

    mix_r = dub_softclip_q15((mix_r * (304 + (int32_t)rim_drv * 4)) >> 8);
    return mix_r;
}

static inline int32_t heavy_psych_drm_bus(int32_t drum_sig, uint8_t kit_drive)
{
    int32_t gain = 272 + ((int32_t)kit_drive * 3);

    drum_sig = (drum_sig * gain) >> 8;
    return dub_softclip_q15(drum_sig);
}

/* Heavy Psych drums: kick dry (0), snare/rim and hat short room send only (no dub echo). */
static inline int32_t heavy_psych_drm_room_send(int32_t kick_sig, int32_t rim_sig, int32_t hat_sig)
{
    (void)kick_sig;
    return ((rim_sig * 18) >> 7) + ((hat_sig * 8) >> 7);
}
static inline int32_t heavy_psych_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                        int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;

    if (p->era == ERA_DOOM || p->era == ERA_DRONE) {
        /* −7 semitones */
        inc = (uint32_t)(((uint64_t)inc * 43690u) >> 16);
    } else if (p->era == ERA_STONER || p->era == ERA_DESERT) {
        /* −5 semitones */
        inc = (uint32_t)(((uint64_t)inc * 49115u) >> 16);
    } else if (p->era == ERA_GAZE) {
        /* −3 semitones */
        inc = (uint32_t)(((uint64_t)inc * 52017u) >> 16);
    }
    (void)lfo_mod_b;
    if (p->era == ERA_DOOM || p->era == ERA_STONER)
        fc -= 580;
    else if (p->era == ERA_DESERT || p->era == ERA_DRONE)
        fc -= 420;
    else if (p->era == ERA_GAZE)
        fc -= 320;
    else if (p->era == ERA_SPACE)
        fc -= 260;
    if (fc < 260)
        fc = 260;
    switch (model) {
    case BASS_FUZZ: {
        uint32_t det = inc >> 3;
        int32_t fund, oct, fifth, click;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc + (inc >> 1);
        fund = dub_square(s->sub_ph) >> 1;
        oct = dub_square(s->sub_ph >> 1) >> 3;
        fifth = dub_square(s->sub_ph3) >> 3;
        click = s->sub_env > 28000 ? (int32_t)(s->sub_env >> 10) : 0;
        raw = fund + oct + fifth + click;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_OCT: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_square(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_MOOG: {
        int32_t saw_w;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        dub_svf_mode(saw_w, fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 28u > 127u ? 127u : p->reso_mode + 28u), &raw);
        dub_svf_mode(raw, fc, &s->sub_svf_b1, &s->sub_svf_b2, (uint8_t)(p->reso_mode >> 1), &raw);
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
    case BASS_MUFF: {
        /* Big Muff style: two detuned squares, hard clip, mid-scooped by the post filter */
        int32_t clip;
        s->sub_ph += inc;
        clip = dub_square(s->sub_ph) + (dub_square(s->sub_ph + (inc >> 3)) >> 1);
        raw = (clip * (96 + 2 * (int32_t)p->wobble)) >> 7;
        if (raw > 24000)
            raw = 24000;
        else if (raw < -24000)
            raw = -24000;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_DRONE: {
        /* Sustained sine root plus sub octave, low-passed; no sweep */
        s->sub_ph += inc;
        s->sub_ph2 += inc >> 1;
        s->sub_ph3 += inc + (inc >> 6);
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph2) >> 1);
        raw += dub_sine_q64(s->sub_ph3) >> 2;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_DOOM: {
        int32_t fund, harm;
        s->sub_ph += inc;
        fund = dub_sine_q64(s->sub_ph);
        harm = dub_sine_q64(s->sub_ph << 1);
        raw = fund + (harm >> 2);
        dub_svf_mode(raw, 1200 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_WAH: {
        /* Wah: saw through a resonant LP whose cutoff sweeps with the envelope (WOBBLE = sweep depth) */
        int32_t saw_w, wah_fc;
        uint8_t reso_w;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        wah_fc = fc + (((s->sub_env >> 2) * ((int32_t)p->wobble + 8)) >> 5);
        if (wah_fc < 260)
            wah_fc = 260;
        else if (wah_fc > 26000)
            wah_fc = 26000;
        reso_w = (uint8_t)(p->reso_mode + 24u > 127u ? 127u : p->reso_mode + 24u);
        dub_svf_mode(saw_w, wah_fc, &s->sub_svf_l1, &s->sub_svf_l2, reso_w, &raw);
        break;
    }
    case BASS_FM: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2 + ((uint32_t)s->sub_fm_fb << 15));
        s->sub_fm_fb = (mod2 * (40 + (int32_t)p->wobble)) >> 8;
        mod2 = (mod2 * (16 + ((int32_t)p->cutoff_b >> 1))) >> 7;
        raw = dub_sine_q64(s->sub_ph + ((uint32_t)mod2 << 15));
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
        raw = pulse + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    default:
        s->sub_ph += inc;
        s->sub_ph2 += inc + (inc >> 5);
        s->sub_ph3 += inc - (inc >> 5);
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 1);
        raw += dub_sine_q64(s->sub_ph << 1) >> 3;
        raw += (dub_sine_q64(s->sub_ph2) >> 2) + (dub_sine_q64(s->sub_ph3) >> 2);
        dub_svf_mode(raw, 780 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    {
        int32_t drv = (int32_t)p->drive + (int32_t)p->wobble;
        int32_t era_gain = 256;

        if (model == BASS_MUFF || model == BASS_FUZZ || model == BASS_WAH || model == BASS_DOOM)
            drv += 24;
        if (p->era == ERA_DOOM || p->era == ERA_STONER)
            era_gain = 304;
        else if (p->era == ERA_DESERT || p->era == ERA_DRONE)
            era_gain = 292;
        else if (p->era == ERA_SPACE)
            era_gain = 276;
        else if (p->era == ERA_GAZE)
            era_gain = 284;
        raw = (raw * era_gain) >> 8;
        if (s->sub_env > 26000 && model != BASS_FUZZ && model != BASS_FM)
            raw += raw >> 3;
        if (drv > 8)
            raw = dub_softclip_q15((raw * (256 + drv * 2)) >> 8);
    }
    return raw;
}

/* STAB SING expr (p->siren_rate 0..127): four zones with ±4-tick crossfade at 32 / 64 / 96.
 * 0..31   M1 house piano (+1 oct CHRD); SING = hammer bite & FM brightness.
 * 32..63  Call/response Rhodes IV (+5) or bVII (+10 at SING 48..63) vs CHRD + root ghost.
 * 64..95  Garage organ / fifth-bell on [0,7,12,19]; SING = drawbar & FM bell index.
 * 96..127 French/disco brass upper triad; classic pitch fall when env decays below 22k.
 * Base: ch_root+12 clamped MIDI 60..72 + kt_s (drm_tune[3]) + s->siren_st_tune; stab_ph[0..7].
 */
static inline int32_t heavy_psych_stab_tone(dub_synth_t *s, uint8_t ph_ix, uint8_t note, int32_t env,
                                      int32_t detune_q16, uint8_t level_shr, int32_t snap_expr)
{
    uint32_t inc = dub_note_inc(note);
    uint32_t ph;
    int32_t fm_idx, mod1, mod23, mod, tone;

    if (detune_q16 != 0)
        inc = (uint32_t)(((uint64_t)inc * (uint64_t)(65536 + detune_q16)) >> 16);
    ph = s->stab_ph[ph_ix];
    s->stab_ph[ph_ix] += inc;

    fm_idx = (env * env) >> 15;
    fm_idx = (fm_idx * (48 + (snap_expr >> 1))) >> 6;
    if (fm_idx < 4)
        fm_idx = 4;
    /* int32 headroom: mod * fm_idx and mod << 13 must not wrap (wrap = metallic bell) */
    if (fm_idx > 2048)
        fm_idx = 2048;
    mod1 = dub_sine_q64(ph);
    mod23 = dub_sine_q64(ph << 1) + (dub_sine_q64(ph + (ph << 1)) >> 1);
    mod = mod1 + (mod23 >> 1);
    mod = (mod * fm_idx) >> 10;
    tone = dub_sine_q64(ph + (uint32_t)(mod << 13));
    return tone >> level_shr;
}

static inline int32_t heavy_psych_stab_pitch_inc(uint8_t note, int32_t detune_q16, int32_t fall_q16)
{
    uint32_t inc = dub_note_inc(note);
    int32_t bend = detune_q16 + fall_q16;

    if (bend != 0)
        inc = (uint32_t)(((uint64_t)inc * (uint64_t)(65536 + bend)) >> 16);
    return (int32_t)inc;
}

static inline void heavy_psych_stab_snap_weights(int32_t snap, int32_t w[4])
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

static inline int32_t heavy_psych_stab_zone0(dub_synth_t *s, const dub_params_t *p, int32_t env,
                                       int32_t root_m, int32_t snap_r, int32_t pk)
{
    static const int16_t stab_det[4] = { -90, 110, -140, 85 };
    int32_t raw = 0, hammer, snap_z;
    uint8_t vi;
    uint32_t hash;

    snap_z = snap_r & 31;
    for (vi = 0; vi < 4u; vi++) {
        uint8_t note = dub_clamp_midi(root_m + (int32_t)dub_chord_interval(p->voicing, vi));

        raw += heavy_psych_stab_tone(s, vi, note, env, (int32_t)stab_det[vi], vi == 0u ? 1u : 2u, snap_z);
    }
    if (env > pk - (pk >> 4)) {
        int32_t atk = env - (pk - (pk >> 4));

        hash = (uint32_t)env ^ s->stab_ph[0];
        hash *= 69069u;
        hash ^= hash >> 16;
        hammer = (dub_square(s->stab_ph[0]) >> 3) + ((int32_t)hash >> 4);
        hammer = (hammer * atk * (32 + (snap_z >> 1))) >> 22;
        raw += hammer;
    }
    return raw;
}

static inline int32_t heavy_psych_stab_zone1(dub_synth_t *s, const dub_params_t *p, int32_t env,
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

        raw += heavy_psych_stab_tone(s, (uint8_t)(4u + vi), note, env, (int32_t)stab_det[vi], 2u, snap_z);
    }
    {
        uint8_t ghost_n = dub_clamp_midi(root_m);

        raw += heavy_psych_stab_tone(s, 7u, ghost_n, env, -30, 5u, snap_z);
    }
    return raw;
}

static inline int32_t heavy_psych_stab_organ_voice(dub_synth_t *s, uint8_t ph_ix, int32_t inc, int32_t env,
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
    if (bell_idx > 1024)
        bell_idx = 1024;
    mod = dub_sine_q64(ph << 1);
    mod = (mod * bell_idx) >> 10;
    bell = dub_sine_q64(ph + (uint32_t)(mod << 13));
    return (draw >> 1) + (bell >> 2);
}

static inline int32_t heavy_psych_stab_zone2(dub_synth_t *s, int32_t env, int32_t root_m, int32_t snap_r,
                                       int32_t pk)
{
    static const int8_t organ_deg[4] = { 0, 7, 12, 19 };
    int32_t raw = 0, bright, click, snap_z;
    uint8_t vi;

    snap_z = snap_r & 31;
    bright = 96 + snap_z * 4;
    for (vi = 0; vi < 4u; vi++) {
        uint8_t note = dub_clamp_midi(root_m + (int32_t)organ_deg[vi]);
        int32_t inc = heavy_psych_stab_pitch_inc(note, 0, 0);

        raw += heavy_psych_stab_organ_voice(s, vi, inc, env, bright);
    }
    if (env > pk - (pk >> 3)) {
        int32_t atk = env - (pk - (pk >> 3));

        click = dub_square(s->stab_ph[0] + 0x20000000u) >> 2;
        raw += (click * atk) >> 13;
    }
    return raw;
}

static inline int32_t heavy_psych_stab_brass_voice(dub_synth_t *s, uint8_t ph_ix, int32_t inc, int32_t env,
                                             int32_t detune_q16, int32_t fm_amt)
{
    uint32_t ph;
    int32_t mod, sawv, tone;

    if (detune_q16 != 0)
        inc = (int32_t)(((uint64_t)(uint32_t)inc * (uint64_t)(65536 + detune_q16)) >> 16);
    ph = s->stab_ph[ph_ix];
    s->stab_ph[ph_ix] += (uint32_t)inc;
    mod = dub_sine_q64(ph << 1);
    mod = (mod * fm_amt) >> 9;
    sawv = dub_saw(ph) >> 2;
    tone = dub_sine_q64(ph + (uint32_t)(mod << 13)) >> 2;
    return sawv + tone;
}

static inline int32_t heavy_psych_stab_zone3(dub_synth_t *s, const dub_params_t *p, int32_t env,
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
            int32_t inc = heavy_psych_stab_pitch_inc(note, (int32_t)brass_det[vi], fall_q16);

            raw += heavy_psych_stab_brass_voice(s, (uint8_t)(4u + vi), inc, env, (vi == 0u ? 140 : 0), fm_amt);
        }
    }
    return raw;
}

/*
 * Chord fuzz, RAT drive mode (drive type 2) of the 303 in m-vave/fm1_open.
 * Source: third_party/synth/bass303.c (M-VAVE, FM1 additions, GPLv3 with SDK linking exception),
 * built on Open303 (Robin Schmidt, MIT). Adapted to fixed point; state lives in dub_synth_t.
 * Chain: pre HP (720 Hz) -> add high boost -> gain -> tanh -> post LP (3300..1900 Hz) -> DC block.
 */
/* Chamberlin SVF, g and damp in Q8. Returns lp and bp via state. */
static inline void heavy_psych_svf(int32_t in, int32_t g, int32_t damp, int32_t *lp, int32_t *bp,
                                   int32_t *hp_out)
{
    int32_t hp = in - *lp - ((*bp * damp) >> 8);

    *bp += (hp * g) >> 8;
    *lp += (*bp * g) >> 8;
    *hp_out = hp;
}

static inline uint8_t heavy_psych_chord_fuzz_grit(uint8_t era, uint8_t drive)
{
    uint32_t g;

    era = heavy_psych_clamp_era(era);
    switch (era) {
    case ERA_DRONE:
        g = 4u;
        break;
    case ERA_SPACE:
        g = 10u;
        break;
    case ERA_GAZE:
        g = 14u;
        break;
    case ERA_DESERT:
        g = 22u;
        break;
    case ERA_STONER:
        g = 32u;
        break;
    case ERA_DOOM:
        g = 42u;
        break;
    default:
        g = 22u;
        break;
    }
    g += ((uint32_t)drive * 12u) / 127u;
    if (g > 64u)
        g = 64u;
    return (uint8_t)g;
}

/* Parallel fuzz wet (Q8): STONER/DOOM keep a dark bed; wash eras stay clean. */
static inline uint8_t heavy_psych_chord_fuzz_wet_q8(uint8_t era)
{
    era = heavy_psych_clamp_era(era);
    switch (era) {
    case ERA_DOOM:
        return 100u;
    case ERA_STONER:
        return 72u;
    case ERA_DESERT:
        return 48u;
    case ERA_GAZE:
        return 32u;
    case ERA_SPACE:
        return 24u;
    default:
        return 16u;
    }
}

static inline uint8_t heavy_psych_chord_rev_send(uint8_t era, uint8_t rev_c_param)
{
    uint32_t send;

    era = heavy_psych_clamp_era(era);
    send = (uint32_t)rev_c_param;
    switch (era) {
    case ERA_GAZE:
        send += 36u;
        break;
    case ERA_SPACE:
        send += 28u;
        break;
    case ERA_DRONE:
        send += 24u;
        break;
    case ERA_DESERT:
        send += 14u;
        break;
    case ERA_STONER:
        send += 8u;
        break;
    default:
        send += 4u;
        break;
    }
    if (send > 127u)
        send = 127u;
    return (uint8_t)send;
}

static inline uint8_t heavy_psych_chord_level_div(uint8_t era)
{
    era = heavy_psych_clamp_era(era);
    if (era == ERA_GAZE || era == ERA_SPACE || era == ERA_DRONE)
        return 68u;
    if (era == ERA_DESERT)
        return 76u;
    return 84u;
}

/* Pad sits under drums/lead — less sidechain duck than stab skank. */
static inline int32_t heavy_psych_chord_pump_duck(int32_t duck, uint8_t era)
{
    era = heavy_psych_clamp_era(era);
    if (era == ERA_GAZE || era == ERA_SPACE || era == ERA_DRONE)
        return (duck * 28) >> 6;
    if (era == ERA_DESERT)
        return (duck * 36) >> 6;
    return (duck * 44) >> 6;
}

static inline int32_t heavy_psych_chord_fc_bias(uint8_t era)
{
    era = heavy_psych_clamp_era(era);
    if (era == ERA_DOOM || era == ERA_DRONE)
        return -1200;
    if (era == ERA_STONER)
        return -800;
    if (era == ERA_DESERT)
        return -500;
    if (era == ERA_GAZE)
        return -350;
    return -200;
}

static inline int32_t heavy_psych_chord_render(dub_synth_t *s, const dub_params_t *p, int32_t ch_root_live,
                                               int32_t glide_sh, int32_t tmb, int32_t lfo_mod_c, int32_t ch_env)
{
    static const int16_t pad_det[4] = { -220, 190, -280, 240 };
    uint8_t vo = p->voicing;
    uint8_t era = heavy_psych_clamp_era(p->era);
    int32_t mix = 0;
    int32_t wash = lfo_mod_c + (dub_sine_q64(s->ch_ph[0] + (s->ch_ph[0] >> 3)) >> 4);
    uint32_t vi;

    (void)ch_env;
    for (vi = 0; vi < DUB_CHORD_N; vi++) {
        uint8_t n = dub_clamp_midi(ch_root_live + (int32_t)dub_chord_interval(vo, (uint8_t)vi));
        uint32_t tgt_inc = dub_note_inc(n);
        uint32_t inc;
        uint32_t ph;
        int32_t soft;
        int32_t det = (int32_t)pad_det[vi];
        int32_t det2 = det + (wash >> (vi & 1u ? 5 : 4));

        if (p->glide_c == 0u) {
            s->ch_inc[vi] = tgt_inc;
        } else {
            uint32_t cur_inc = s->ch_inc[vi];

            s->ch_tgt_inc[vi] = tgt_inc;
            if (cur_inc == 0u)
                cur_inc = tgt_inc;
            if (cur_inc < tgt_inc) {
                uint32_t step = ((tgt_inc - cur_inc) >> glide_sh) + 1u;

                cur_inc += step;
                if (cur_inc > tgt_inc)
                    cur_inc = tgt_inc;
            } else if (cur_inc > tgt_inc) {
                uint32_t step = ((cur_inc - tgt_inc) >> glide_sh) + 1u;

                if (cur_inc > step)
                    cur_inc -= step;
                else
                    cur_inc = tgt_inc;
                if (cur_inc < tgt_inc)
                    cur_inc = tgt_inc;
            }
            s->ch_inc[vi] = cur_inc;
        }
        inc = s->ch_inc[vi];
        if (det2 != 0)
            inc = (uint32_t)(((uint64_t)inc * (uint64_t)(65536 + det2)) >> 16);
        s->ch_ph[vi] += inc;
        ph = s->ch_ph[vi];

        /* Metal pad: saw/square stack + detune wash — no FM index / bell partials */
        {
            int32_t saw1, saw2, sq, body, stack;
            uint32_t ph_w;

            saw1 = dub_saw(ph) >> 2;
            sq = dub_square(ph) >> 3;
            ph_w = ph + (uint32_t)(wash >> 2) + (uint32_t)(det >> 3);
            saw2 = dub_saw(ph_w) >> 3;
            body = dub_sine_q64(ph) >> 4;
            stack = saw1 + sq + saw2;
            if (tmb > 48)
                stack += dub_square(ph << 1) >> (5 + ((127 - tmb) >> 4));
            if (tmb > 80)
                stack += dub_saw(ph + (inc >> (4u + vi))) >> 4;

            if (era == ERA_STONER || era == ERA_DOOM) {
                soft = stack + body + (dub_saw(ph << 1) >> 4);
                soft += dub_square(ph >> 1) >> 2;
            } else if (era == ERA_GAZE || era == ERA_SPACE) {
                soft = saw1 + saw2 + (sq >> 2) + body;
            } else if (era == ERA_DRONE) {
                soft = (saw1 >> 1) + (sq >> 1) + body;
                soft += dub_square(ph >> 1) >> 2;
            } else {
                soft = stack + body;
            }

            if (era == ERA_DRONE || era == ERA_DOOM)
                soft += dub_square(ph >> 1) >> 3;
        }

        mix += soft >> 2;
    }
    return mix;
}

static inline int32_t heavy_psych_fuzz(dub_synth_t *s, int32_t x, uint8_t grit, uint8_t ch)
{
    /* drive 0..1 in Q7 (grit/127), 32-bit fixed-point (no 64-bit division in ISR) */
    int32_t d_q7 = (int32_t)grit;
    int32_t pre_hp, high, post_g, dc_out;
    int32_t d2_q7, term, gain_q8, pre, v, v2, num, den, t;

    /* pre HP 720 Hz, Q 0.707 -> g = 720*0.0365 = 26 */
    heavy_psych_svf(x, 26, 362, &s->fz_pre_lp[ch], &s->fz_pre_bp[ch], &pre_hp);
    /* drive_high = 1.6 - 0.8*d ; Q8 */
    high = ((410 - ((205 * d_q7) >> 7)) * pre_hp) >> 8;
    /* drive_gain = 6 + d^2*(1500 - 800*d), d = grit/127 ; Q8 */
    d2_q7 = (d_q7 * d_q7) >> 7;
    term = 1500 - ((800 * d_q7) >> 7);
    gain_q8 = 1536 + ((d2_q7 * term) << 1);
    pre = (int32_t)(((int64_t)(x + high) * gain_q8) >> 8);
    /* tanh via Pade [3/2] on v in [-1024, 1024] representing [-3, 3] */
    v = (int32_t)(((int64_t)pre * 1024) >> 15);
    if (v > 1024)
        v = 1024;
    else if (v < -1024)
        v = -1024;
    v2 = (v * v) >> 10;
    num = v * (3072 + v2);
    den = 3072 + 9 * v2;
    /* full scale at v=1024: num/den = 341 -> scale to +-6000 */
    t = (num * 18) / den;
    x = t;
    /* post LP: 3300 - 1400*d Hz, g = f*0.0365 */
    post_g = ((3300 - ((1400 * d_q7) >> 7)) * 73) / 2000;
    heavy_psych_svf(x, post_g, 362, &s->fz_post_lp[ch], &s->fz_post_bp[ch], &pre_hp);
    x = s->fz_post_lp[ch];
    /* DC block ~3 Hz */
    s->fz_dc[ch] += (x - s->fz_dc[ch]) >> 11;
    dc_out = x - s->fz_dc[ch];
    return dc_out;
}

static inline uint8_t heavy_psych_lead_grit(const dub_params_t *p)
{
    uint8_t kit = p->drum_kit % DKIT_N;
    uint8_t era = heavy_psych_clamp_era(p->era);
    uint32_t g;

    switch (era) {
    case ERA_STONER:
        g = 76u;
        break;
    case ERA_DOOM:
        g = 92u;
        break;
    case ERA_DRONE:
        g = 18u;
        break;
    case ERA_DESERT:
        g = 42u;
        break;
    case ERA_SPACE:
        g = 38u;
        break;
    default:
        g = 34u;
        break;
    }
    g += ((uint32_t)heavy_psych_kit_k_drv[kit] * 5u) / 32u;
    g += ((uint32_t)p->drive * 18u) / 127u;
    if (g > 127u)
        g = 127u;
    return (uint8_t)g;
}

static inline uint8_t heavy_psych_lead_lp_cut(const dub_params_t *p)
{
    uint8_t kit = p->drum_kit % DKIT_N;
    uint8_t era = heavy_psych_clamp_era(p->era);
    uint8_t lc = p->drm_cut[3] ? p->drm_cut[3] : 64u;
    int32_t lp = ((int32_t)lc * 3) / 4;

    lp += ((int32_t)heavy_psych_kit_k_fm[kit] - 16) >> 1;
    if (era == ERA_STONER || era == ERA_SPACE)
        lp += 14;
    else if (era == ERA_DOOM)
        lp -= 22;
    else if (era == ERA_DRONE)
        lp -= 4;
    else if (era == ERA_DESERT || era == ERA_GAZE)
        lp += 8;
    if (era == ERA_DOOM) {
        if (lp < 36)
            lp = 36;
        else if (lp > 78)
            lp = 78;
    } else {
        if (lp < 52)
            lp = 52;
        else if (lp > 120)
            lp = 120;
    }
    return (uint8_t)lp;
}

static inline uint8_t heavy_psych_lead_lp_sweep(const dub_params_t *p, int32_t env, int32_t peak)
{
    uint8_t base = heavy_psych_lead_lp_cut(p);
    uint8_t era = heavy_psych_clamp_era(p->era);
    int32_t depth;
    int32_t env_q;
    int32_t cut;

    if (peak <= 0)
        return base;
    env_q = (env << 7) / peak;
    switch (era) {
    case ERA_STONER:
        depth = 22;
        break;
    case ERA_DOOM:
        depth = 16;
        break;
    case ERA_DRONE:
        depth = 34;
        break;
    case ERA_DESERT:
        depth = 20;
        break;
    case ERA_SPACE:
        depth = 18;
        break;
    default:
        depth = 24;
        break;
    }
    if (era == ERA_DOOM)
        depth = 8;
    cut = (int32_t)base + ((depth * env_q) >> 7);
    if (era == ERA_DOOM) {
        if (cut < 32)
            cut = 32;
        else if (cut > 80)
            cut = 80;
    } else {
        if (cut < 48)
            cut = 48;
        else if (cut > 120)
            cut = 120;
    }
    return (uint8_t)cut;
}

static inline uint8_t heavy_psych_lead_fuzz_on(const dub_params_t *p)
{
    uint8_t era = heavy_psych_clamp_era(p->era);

    return (era != ERA_DRONE && era != ERA_DOOM) ? 1u : 0u;
}

static inline uint8_t heavy_psych_lead_dly_boost(const dub_params_t *p)
{
    uint8_t kit = p->drum_kit % DKIT_N;
    uint8_t era = heavy_psych_clamp_era(p->era);

    if (era != ERA_SPACE)
        return 0u;
    return (uint8_t)(28u + (heavy_psych_kit_k_sweep[kit] >> 2));
}

static inline int32_t heavy_psych_lead_tanh_pade(int32_t x)
{
    int32_t v = (int32_t)(((int64_t)x * 1024) >> 15);
    int32_t v2, num, den;

    if (v > 1024)
        v = 1024;
    else if (v < -1024)
        v = -1024;
    v2 = (v * v) >> 10;
    num = v * (3072 + v2);
    den = 3072 + 9 * v2;
    return (num * 18) / den;
}

static inline int32_t heavy_psych_lead_amp_gain_q8(const dub_params_t *p)
{
    uint8_t kit = p->drum_kit % DKIT_N;
    uint8_t era = heavy_psych_clamp_era(p->era);
    int32_t g;

    switch (era) {
    case ERA_STONER:
        g = 340;
        break;
    case ERA_DOOM:
        g = 360;
        break;
    case ERA_DESERT:
        g = 272;
        break;
    case ERA_SPACE:
        g = 232;
        break;
    case ERA_DRONE:
        g = 200;
        break;
    default:
        g = 256;
        break;
    }
    g += ((int32_t)heavy_psych_kit_k_drv[kit] * 9) / 32;
    if (g > 420)
        g = 420;
    return g;
}

static inline int32_t heavy_psych_lead_amp_cab(dub_synth_t *s, int32_t x, const dub_params_t *p)
{
    int32_t hp, pre, mid, treb, body, out;
    int32_t gain = heavy_psych_lead_amp_gain_q8(p);

    heavy_psych_svf(x, 4, 362, &s->lead_amp_hp_lp, &s->lead_amp_hp_bp, &hp);
    pre = (int32_t)(((int64_t)hp * gain) >> 8);
    pre += 520;
    pre = heavy_psych_lead_tanh_pade(pre);
    pre = heavy_psych_lead_tanh_pade((pre * 5) >> 2);
    heavy_psych_svf(pre, 36, 280, &s->lead_amp_mid_lp, &s->lead_amp_mid_bp, &hp);
    mid = pre + (s->lead_amp_mid_bp >> 1);
    heavy_psych_svf(mid, 152, 362, &s->lead_amp_treb_lp, &s->lead_amp_treb_bp, &hp);
    treb = s->lead_amp_treb_lp >> 1;
    heavy_psych_svf(treb, 118, 362, &s->lead_cab_lp, &s->lead_cab_bp, &hp);
    heavy_psych_svf(s->lead_cab_lp, 4, 420, &s->lead_cab_body_lp, &s->lead_cab_body_bp, &hp);
    body = s->lead_cab_body_lp;
    out = s->lead_cab_lp + (body >> 2);
    return dub_softclip_q15(out >> 1);
}

/* Lead pitch: ≤5 cents fixed detune (Q16); DESERT-only vibrato ≤6 Hz / ≤8 cents. */
#define HP_LEAD_DETUNE_Q16     47   /* ~5 cents between dual saws */
#define HP_LEAD_DESERT_VIB_Q16 303  /* ~8 cents peak */
#define HP_LEAD_DESERT_LFO_INC 2268u /* ~6 Hz at DUB_FS */
#define HP_LEAD_SAW_BODY_Q7    48u  /* detuned saw for even-harmonic body */
#define HP_LEAD_SAW_OCT_Q7     56u  /* 2nd-harmonic octave saw */
#define HP_LEAD_DOOM_SAW_BODY_Q7 22u
#define HP_LEAD_DOOM_SAW_OCT_Q7  8u
#define HP_LEAD_BEND_CENTS_Q16 94   /* ~10 ct subtle attack bend over 60 ms */
#define HP_LEAD_BEND_SAMPLES   ((DUB_FS * 6u) / 100u)
#define HP_LEAD_DRIVE_RAMP_SAMPLES ((DUB_FS * 25u) / 100u)

static inline uint32_t heavy_psych_lead_drive_curve_q8(uint16_t note_age, uint8_t curve)
{
    uint32_t n = note_age > HP_LEAD_DRIVE_RAMP_SAMPLES ? HP_LEAD_DRIVE_RAMP_SAMPLES : note_age;
    uint32_t t = (n * 256u) / HP_LEAD_DRIVE_RAMP_SAMPLES;

    if (curve == 1u) {
        /* Convex: fast initial bite, early saturation */
        uint32_t inv = 256u - t;

        return 256u - ((inv * inv) >> 8);
    }
    if (curve == 2u) {
        /* Concave: clean pick attack, late screaming bloom */
        return (t * t) >> 8;
    }
    return t;
}

static inline uint32_t heavy_psych_lead_bent_inc_trax(const dub_synth_t *s, uint32_t inc, uint8_t trax)
{
    uint32_t n = s->siren_note_age;
    (void)trax;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        uint32_t b_smp = (DUB_FS * 75u) / 1000u;
        uint32_t depth = (uint32_t)s->siren_hp_bend;
        uint32_t abs_semi = (uint32_t)(semi >= 0 ? semi : -semi);
        uint32_t bend_q16 = abs_semi * 3800u * depth / 127u;

        if (bend_q16 > 0u && n < b_smp) {
            if (semi > 0) {
                inc = (uint32_t)(((uint64_t)inc * (65536u - bend_q16 + (bend_q16 * n) / b_smp)) >> 16);
            } else {
                inc = (uint32_t)(((uint64_t)inc * (65536u + bend_q16 - (bend_q16 * n) / b_smp)) >> 16);
            }
        }
    }
    return inc;
}

static inline uint8_t heavy_psych_lead_drive_grit_trax(const dub_params_t *p, uint16_t note_age,
                                                       int32_t swell_q8, uint8_t trig_cnt)
{
    const heavy_psych_lead_voice_t *lv = heavy_psych_trax_lead_voice(p->plate);
    uint8_t role = (uint8_t)((trig_cnt - 1u) & 3u);
    uint32_t gs = lv->grit_start;
    uint32_t ge = lv->grit_end + (((uint32_t)p->drive * 16u) / 127u) + (role == 2u ? 18u : 0u);
    uint32_t t_q8 = heavy_psych_lead_drive_curve_q8(note_age, lv->grit_curve);
    uint32_t g;

    if (ge > 127u)
        ge = 127u;
    if (gs > ge)
        gs = ge;
    g = gs + (((ge - gs) * t_q8) >> 8);
    g = (g * (uint32_t)swell_q8) >> 8;
    if (g > 127u)
        g = 127u;
    return (uint8_t)g;
}

static inline int32_t heavy_psych_lead_saw_voice(uint32_t ph, uint32_t inc, uint8_t era)
{
    int32_t core = dub_saw(ph);
    int32_t body = dub_saw(ph + (inc >> 3));
    int32_t oct  = dub_saw(ph << 1);
    uint8_t body_q7 = HP_LEAD_SAW_BODY_Q7;
    uint8_t oct_q7 = HP_LEAD_SAW_OCT_Q7;

    if (heavy_psych_clamp_era(era) == ERA_DOOM) {
        body_q7 = HP_LEAD_DOOM_SAW_BODY_Q7;
        oct_q7 = HP_LEAD_DOOM_SAW_OCT_Q7;
    }
    return core + ((body * (int32_t)body_q7) >> 7)
                + ((oct * (int32_t)oct_q7) >> 7);
}

#if defined(DINGHY_HOST)
#include <stdio.h>
#include <stdlib.h>

static struct {
    uint64_t pluck_sq;
    uint32_t pluck_n;
    uint64_t out_sq[6];
    uint32_t out_n[6];
} hp_host_lead_met;

static inline uint32_t hp_host_isqrt64(uint64_t x)
{
    uint64_t op = x;
    uint64_t res = 0;
    uint64_t one = 1ull << 62;

    while (one > op)
        one >>= 2;
    while (one) {
        if (op >= res + one) {
            op -= res + one;
            res = (res >> 1) + one;
        } else
            res >>= 1;
        one >>= 2;
    }
    return (uint32_t)res;
}

static inline void heavy_psych_host_lead_metrics_print(void)
{
    const char *e = getenv("FM1_HVYPSY_LEAD_METRICS");
    uint32_t rms;

    if (!e || !e[0])
        return;
    if (hp_host_lead_met.pluck_n > 0u) {
        rms = hp_host_isqrt64(hp_host_lead_met.pluck_sq / (uint64_t)hp_host_lead_met.pluck_n);
        printf("hvypsy_lead_pluck_rms=%u n=%u\n", (unsigned)rms, (unsigned)hp_host_lead_met.pluck_n);
    }
    {
        static const char *era_tag[6] = { "STONER", "DESERT", "SPACE", "DRONE", "GAZE", "DOOM" };
        uint8_t er;

        for (er = 0; er < 6u; er++) {
            if (hp_host_lead_met.out_n[er] == 0u)
                continue;
            rms = hp_host_isqrt64(hp_host_lead_met.out_sq[er] / (uint64_t)hp_host_lead_met.out_n[er]);
            printf("hvypsy_lead_out_rms_%s=%u n=%u\n", era_tag[er], (unsigned)rms,
                   (unsigned)hp_host_lead_met.out_n[er]);
        }
    }
}

static struct {
    uint64_t ch_sq;
    uint32_t ch_n;
    int32_t ch_env_max;
    int32_t ch_sig_peak;
} hp_host_chord_met;

static inline void heavy_psych_host_chord_sample(int32_t ch_sig, int32_t ch_env)
{
    int64_t s = (int64_t)ch_sig;

    hp_host_chord_met.ch_sq += (uint64_t)(s * s);
    hp_host_chord_met.ch_n++;
    if (ch_env > hp_host_chord_met.ch_env_max)
        hp_host_chord_met.ch_env_max = ch_env;
    if (ch_sig < 0)
        ch_sig = -ch_sig;
    if (ch_sig > hp_host_chord_met.ch_sig_peak)
        hp_host_chord_met.ch_sig_peak = ch_sig;
}

static inline void heavy_psych_host_chord_metrics_print(void)
{
    const char *e = getenv("FM1_HVYPSY_CHORD_METRICS");
    uint32_t rms;

    if (!e || !e[0] || hp_host_chord_met.ch_n == 0u)
        return;
    rms = hp_host_isqrt64(hp_host_chord_met.ch_sq / (uint64_t)hp_host_chord_met.ch_n);
    printf("hvypsy_chord_rms=%u peak=%d env_max=%d n=%u\n", (unsigned)rms,
           (int)hp_host_chord_met.ch_sig_peak, (int)hp_host_chord_met.ch_env_max,
           (unsigned)hp_host_chord_met.ch_n);
}
#else
static inline void heavy_psych_host_lead_metrics_print(void) {}
static inline void heavy_psych_host_chord_sample(int32_t ch_sig, int32_t ch_env)
{
    (void)ch_sig;
    (void)ch_env;
}
static inline void heavy_psych_host_chord_metrics_print(void) {}
#endif

/* Saw-algo smoothing LP and DOOM second body band; kept apart from hp_lead_pluck / lead_cab_body_*. */
static int32_t hp_lead_saw_lp;
static int32_t hp_doom_body2_lp, hp_doom_body2_bp;

/* Central lead sound-generator dispatch (add new algos here only). */
static inline int32_t heavy_psych_lead_algo_voice(dub_synth_t *s, const dub_params_t *p, int32_t env,
                                                  uint32_t inc, uint8_t era, int32_t root_m)
{
    int32_t mix;
    uint8_t algo = heavy_psych_lead_algo(era);

    switch (algo) {
    case HP_LEAD_ALGO_PLUCK:
        if (s->siren_pluck_arm) {
            const heavy_psych_lead_voice_t *lv = heavy_psych_trax_lead_voice(p->plate);
            uint8_t dec_u7 = p->drm_dec[3] ? p->drm_dec[3] : 96u;
            uint8_t bright = lv->lp_cut;

            if (bright > 52u)
                bright = (uint8_t)(24u + ((bright - 52u) * 28u) / 75u);
            heavy_psych_pluck_trig(dub_note_inc(dub_clamp_midi(root_m)), dec_u7, bright, &s->noise, 0u);
            s->siren_pluck_arm = 0u;
        }
        s->stab_ph[0] += inc;
        mix = heavy_psych_pluck_step(inc);
#if defined(DINGHY_HOST)
        if (getenv("FM1_HVYPSY_LEAD_METRICS")) {
            hp_host_lead_met.pluck_sq += (uint64_t)(int64_t)mix * (int64_t)mix;
            hp_host_lead_met.pluck_n++;
        }
#endif
        return (mix * env) >> 15;
    case HP_LEAD_ALGO_SINE:
        s->stab_ph[0] += inc;
        mix = dub_sine_q64(s->stab_ph[0]);
        return (mix * env) >> 15;
    case HP_LEAD_ALGO_SAW:
    default:
        s->stab_ph[0] += inc;
        mix = heavy_psych_lead_saw_voice(s->stab_ph[0], inc, era);
        hp_lead_saw_lp += ((mix - hp_lead_saw_lp) * 56) >> 8;
        mix = hp_lead_saw_lp;
        return (mix * env) >> 15;
    }
}

/* Monophonic lead (SING lane): era algo from HEAVY_PSYCH_LEAD_SUSTAIN; fixed pitch. */
static inline int32_t heavy_psych_lead_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, ch_base, root_m, oct;
    uint32_t inc;
    uint8_t era = heavy_psych_clamp_era(p->era);
    uint8_t kit = p->drum_kit % DKIT_N;
    (void)heavy_psych_kit_k_fm[kit];

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 24 / 64;
    ch_base = (int32_t)(p->root ? p->root : 48u);
    if (era == ERA_DOOM)
        oct = -14;
    else if (era == ERA_STONER || era == ERA_DESERT || era == ERA_GAZE)
        oct = -12;
    else if (era == ERA_DRONE)
        oct = -5;
    else
        oct = 7;
    root_m = ch_base + oct + kt_s + (int32_t)s->siren_st_tune;
    if (heavy_psych_lead_uses_pluck(era)) {
        if (root_m < 36)
            root_m = 36;
        else if (root_m > 58)
            root_m = 58;
    } else if (era == ERA_DOOM) {
        if (root_m < 32)
            root_m = 32;
        else if (root_m > 54)
            root_m = 54;
    } else if (era == ERA_STONER || era == ERA_DESERT) {
        if (root_m < 38)
            root_m = 38;
        else if (root_m > 67)
            root_m = 67;
    } else if (era == ERA_DRONE) {
        if (root_m < 34)
            root_m = 34;
        else if (root_m > 62)
            root_m = 62;
    } else {
        if (root_m < 42)
            root_m = 42;
        else if (root_m > 76)
            root_m = 76;
    }
    {
        uint32_t tgt_inc = dub_note_inc(dub_clamp_midi(root_m));
        uint32_t cur_inc = s->siren_lead_inc;

        /* Legato portamento only while GLIDE is up. Knob 1 ≈ 25 ms, 127 ≈ 0.7 s per time constant.
         * A fresh attack zeros siren_lead_inc and snaps. */
        if (cur_inc == 0u || p->glide_c == 0u || !s->siren_lead_glide_on)
            cur_inc = tgt_inc;
        else {
            int32_t glide_sh = 10 + (int32_t)(((uint32_t)p->glide_c * 5u) / 127u);

            if (cur_inc < tgt_inc) {
                uint32_t step = ((tgt_inc - cur_inc) >> (uint32_t)glide_sh) + 1u;

                cur_inc += step;
                if (cur_inc > tgt_inc)
                    cur_inc = tgt_inc;
            } else if (cur_inc > tgt_inc) {
                uint32_t step = ((cur_inc - tgt_inc) >> (uint32_t)glide_sh) + 1u;

                if (cur_inc > step)
                    cur_inc -= step;
                else
                    cur_inc = tgt_inc;
                if (cur_inc < tgt_inc)
                    cur_inc = tgt_inc;
            }
        }
        s->siren_lead_inc = cur_inc;
        s->siren_lead_tgt_inc = tgt_inc;
        if (cur_inc == tgt_inc)
            s->siren_lead_glide_on = 0u;
        inc = heavy_psych_lead_bent_inc_trax(s, cur_inc, p->plate);
    }
    return heavy_psych_lead_algo_voice(s, p, env, inc, era, root_m);
}

static inline int32_t heavy_psych_lead_asym_clip(int32_t x)
{
    int32_t y;

    if (x >= 0)
        y = heavy_psych_lead_tanh_pade(x << 2);
    else
        y = heavy_psych_lead_tanh_pade(x * 2) >> 1;
    /* Scale post-clip stage so (sig * HP_LEAD_OUT_GAIN_Q12) >> 12 sits ~-2 dB vs chord */
    return y * 76;
}

/* Wide tone: scoop ~500 Hz, mid body ~750 Hz, wah/presence sweep (40% range), per-trax cab LP. */
static inline int32_t heavy_psych_lead_guitar_tone(dub_synth_t *s, int32_t x, uint8_t trax, int32_t wah_q15,
                                                   uint8_t pluck_era, uint8_t era)
{
    int32_t hp;
    const heavy_psych_lead_voice_t *lv = heavy_psych_trax_lead_voice(trax);
    uint8_t doom_saw = (!pluck_era && heavy_psych_clamp_era(era) == ERA_DOOM) ? 1u : 0u;
    /* Base cutoff ±25% per trax, plus ±40% wah filter sweep on sustained notes */
    int32_t cab_base = 88 + (int32_t)lv->lp_cut;
    int32_t cab_sweep = ((cab_base * 2) / 5 * wah_q15) >> 15;
    int32_t cab_g = cab_base + cab_sweep;
    int32_t treb_base = 68 + ((int32_t)lv->lp_cut >> 1);
    int32_t treb_sweep = ((treb_base * 2) / 5 * wah_q15) >> 15;
    int32_t treb_g = treb_base + treb_sweep;

    if (cab_g < 100)
        cab_g = 100;
    else if (cab_g > 235)
        cab_g = 235;
    if (treb_g < 48)
        treb_g = 48;
    else if (treb_g > 165)
        treb_g = 165;
    if (pluck_era) {
        wah_q15 >>= 2;
        if (cab_g > 175)
            cab_g = 175;
        if (treb_g > 95)
            treb_g = 95;
    } else if (doom_saw) {
        wah_q15 >>= 3;
        cab_base = 62 + ((int32_t)lv->lp_cut >> 2);
        cab_g = cab_base + (((cab_base * 3) / 20 * wah_q15) >> 15);
        treb_base = 40 + ((int32_t)lv->lp_cut >> 3);
        treb_g = treb_base + (((treb_base * 3) / 20 * wah_q15) >> 15);
        if (cab_g > 108)
            cab_g = 108;
        if (treb_g > 58)
            treb_g = 58;
    }
    heavy_psych_svf(x, 12, 362, &s->lead_amp_hp_lp, &s->lead_amp_hp_bp, &hp);
    x = hp;
    heavy_psych_svf(x, pluck_era ? 22 : (doom_saw ? 24 : 18), 200, &s->lead_amp_mid_lp, &s->lead_amp_mid_bp, &hp);
    x = x - (s->lead_amp_mid_bp >> (pluck_era ? 0 : 1));
    if (pluck_era) {
        heavy_psych_svf(x, 24, 320, &s->lead_cab_body_lp, &s->lead_cab_body_bp, &hp);
        x = x - (s->lead_cab_body_bp >> 2);
    } else if (doom_saw) {
        heavy_psych_svf(x, 32, 300, &s->lead_cab_body_lp, &s->lead_cab_body_bp, &hp);
        x = x - (s->lead_cab_body_bp >> 1);
        heavy_psych_svf(x, 36, 360, &hp_doom_body2_lp, &hp_doom_body2_bp, &hp);
        x = x - (hp_doom_body2_bp >> 1);
    } else {
        heavy_psych_svf(x, 28, 260, &s->lead_cab_body_lp, &s->lead_cab_body_bp, &hp);
        x = x + (s->lead_cab_body_bp >> 2);
    }
    heavy_psych_svf(x, treb_g, 150, &s->lead_amp_treb_lp, &s->lead_amp_treb_bp, &hp);
    if (pluck_era)
        x = x + (s->lead_amp_treb_lp >> 2);
    else if (doom_saw)
        x = s->lead_amp_treb_lp >> 1;
    else
        x = x + (s->lead_amp_treb_bp * 2) + (s->lead_amp_treb_lp >> 1);
    heavy_psych_svf(x, cab_g, 340, &s->lead_cab_lp, &s->lead_cab_bp, &hp);
    return s->lead_cab_lp;
}

/* Lead output: ~+2.5 dB vs anti-squeak cut (33); still ~1 dB below pre-cut 63. */
#define HP_LEAD_OUT_GAIN_Q12 47u

/* Lead: lvl → pick transient → chord fuzz → even-harmonic body → guitar EQ → drive ramp → asym clip. */
static inline int32_t heavy_psych_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t lfo_mod_c)
{
    int32_t sig, oct_body, rect_fuzz;
    int32_t sing;
    uint8_t lvl;
    uint32_t n = s->siren_note_age;
    const heavy_psych_lead_voice_t *lv = heavy_psych_trax_lead_voice(p->plate);
    const heavy_psych_lead_sustain_t *su = heavy_psych_lead_sustain(p->era);
    uint32_t t_curve = heavy_psych_lead_drive_curve_q8(s->siren_note_age, lv->grit_curve);
    int32_t sustain_mod_q8;
    int32_t wah_q15;
    int32_t swell_q8;

    (void)lfo_mod_c;
    sing = (int32_t)p->siren_rate;
    /* Advance slow sustain filter LFO (0.5..2.0 Hz) and grit swell LFO (1.0..3.0 Hz) */
    s->siren_ph += (uint32_t)lv->sweep_rate_q6 * 1522u;
    s->stab_ph[2] += 64928u + (uint32_t)lv->sweep_rate_q6 * 1522u;
    sustain_mod_q8 = heavy_psych_lead_sustain_mod_q8((uint16_t)n, p->era);
    wah_q15 = 0;
    if (su->wah_swell)
        wah_q15 = (dub_sine_q64(s->siren_ph) * sustain_mod_q8) >> 8;
    /* SING (siren_rate): static cab/presence bias; siren_pitch stays level only */
    if (!heavy_psych_lead_uses_pluck(p->era)) {
        if (p->era == ERA_DOOM)
            wah_q15 += (sing - 64) * 32;
        else
            wah_q15 += (sing - 64) * 256;
    } else
        wah_q15 += (sing - 64) * 64;
    swell_q8 = 256;
    if (su->grit_swell)
        swell_q8 = 256 + (((dub_sine_q64(s->stab_ph[2]) * 38) >> 15) * sustain_mod_q8 >> 8);

    sig = heavy_psych_lead_sample(s, p, s->siren_env);
    /* Pick transient: soft noise-only on Karplus eras; saw+noise on synth-algo eras */
    if (heavy_psych_lead_uses_pluck(p->era)) {
        if (n < 88u) {
            int32_t p_env = (int32_t)(88u - n);
            int32_t pick = dub_noise(&s->noise) >> 3;

            pick = (pick * p_env) >> 8;
            sig += (pick * s->siren_env) >> 16;
        }
    } else if (p->era != ERA_DOOM && n < 264u) {
        int32_t p_env = (int32_t)(264u - n);
        int32_t pick = ((dub_noise(&s->noise) >> 2) + (dub_saw(s->stab_ph[0] << 2) >> 1));

        pick = (pick * p_env) >> 8;
        sig += (pick * s->siren_env) >> 15;
    }
    sig = (sig * (int32_t)p->siren_pitch) >> 7;
    lvl = p->lvl_c ? p->lvl_c : 96u;
    sig = (sig * (int32_t)lvl) / 92;
    {
        uint8_t grit = heavy_psych_lead_drive_grit_trax(p, s->siren_note_age, swell_q8, s->siren_trig_count);
        int32_t pre_drv = ((int32_t)(102u + ((230u * t_curve) >> 8)) * swell_q8) >> 8;
        int32_t grit_add = (sing * 44) >> 7;

        if (grit_add > 127 - (int32_t)grit)
            grit_add = 127 - (int32_t)grit;
        grit = (uint8_t)((int32_t)grit + grit_add);
        if (heavy_psych_lead_uses_pluck(p->era)) {
            pre_drv = (pre_drv * 124) >> 8;
            grit = (uint8_t)((grit * 54) >> 7);
        } else if (p->era == ERA_DOOM) {
            pre_drv = (pre_drv * 120) >> 8;
            grit = (uint8_t)((grit * 5) >> 3);
        } else
            pre_drv = (pre_drv * (168 + ((sing * 88) >> 7))) >> 8;

        /* Pre-fuzz drive follows per-trax curve (linear/convex/concave) + 15% sustain swell */
        sig = (sig * pre_drv) >> 8;
        if (heavy_psych_lead_uses_pluck(p->era))
            sig = dub_softclip_q15((sig * (256 + (int32_t)grit * 2)) >> 8);
        else if (heavy_psych_lead_fuzz_on(p))
            sig = heavy_psych_fuzz(s, sig, grit, 1u);
        else
            sig = dub_softclip_q15((sig * (256 + (int32_t)grit * 3)) >> 8);
    }
    if (heavy_psych_lead_uses_pluck(p->era)) {
        oct_body = ((dub_saw(s->stab_ph[0] << 1) * s->siren_env) >> 15);
        oct_body = (oct_body * 900) >> 12;
        sig = sig + oct_body;
    } else if (p->era != ERA_DOOM) {
        oct_body = ((dub_saw(s->stab_ph[0] << 1) * s->siren_env) >> 15);
        oct_body = (oct_body * 1800) >> 12;
        rect_fuzz = (sig < 0 ? -sig : sig) >> 1;
        sig = sig + oct_body + rect_fuzz;
    }
    sig = heavy_psych_lead_guitar_tone(s, sig, p->plate, wah_q15,
                                       heavy_psych_lead_uses_pluck(p->era), p->era);
    sig = (sig * (int32_t)(192u + ((192u * t_curve) >> 8))) >> 8;
    sig = heavy_psych_lead_asym_clip(sig);
    /* Post-clip per-note expression: ±4 dB step velocity spread (vel 58..102) and +5 dB accent (vel 120+) */
    {
        int32_t v = (int32_t)(s->siren_vel ? s->siren_vel : 80u);
        int32_t expr_q8;

        if (v <= 80)
            expr_q8 = 152 + ((v - 58) * 60) / 22;
        else
            expr_q8 = 212 + ((v - 80) * 164) / 42;
        if (expr_q8 < 148)
            expr_q8 = 148;
        else if (expr_q8 > 376)
            expr_q8 = 376;
        if (p->era == ERA_DOOM)
            expr_q8 = (expr_q8 * 10) >> 3;
        sig = (int32_t)(((int64_t)sig * expr_q8) >> 8);
    }
    if (heavy_psych_lead_uses_pluck(p->era))
        sig = heavy_psych_pluck_scale_out(sig);
    else if (p->era == ERA_DRONE)
        sig = (sig * 224) >> 8;
    sig = (sig * (int32_t)HP_LEAD_OUT_GAIN_Q12) >> 12;
#if defined(DINGHY_HOST)
    if (getenv("FM1_HVYPSY_LEAD_METRICS")) {
        uint8_t er = heavy_psych_clamp_era(p->era);
        int64_t t = (int64_t)sig;

        hp_host_lead_met.out_sq[er] += (uint64_t)(t * t);
        hp_host_lead_met.out_n[er]++;
    }
#endif
    return dub_softclip_q15(sig);
}

static inline int32_t heavy_psych_stab_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, ch_base, root_m, snap_r, w[4], mix, pk;

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 24 / 64;
    ch_base = (int32_t)(s->ch_root ? s->ch_root : (p->root ? p->root : 48u));
    root_m = ch_base + 12 + kt_s + (int32_t)s->siren_st_tune;
    if (root_m < 60)
        root_m = 60;
    else if (root_m > 72)
        root_m = 72;
    snap_r = (int32_t)p->siren_rate;
    pk = s->siren_peak > 0 ? s->siren_peak : 28000;

    if (p->era == ERA_STONER) {
        /* Power chord: root + fifth + octave, saw/square, no FM (no bell) */
        int32_t mix;
        uint32_t i0 = dub_note_inc(dub_clamp_midi(root_m));
        uint32_t i1 = dub_note_inc(dub_clamp_midi(root_m + 7));
        uint32_t i2 = dub_note_inc(dub_clamp_midi(root_m + 12));

        s->stab_ph[0] += i0;
        s->stab_ph[1] += i1;
        s->stab_ph[2] += i2;
        mix = dub_saw(s->stab_ph[0]) + (dub_square(s->stab_ph[1]) >> 1) + (dub_saw(s->stab_ph[2]) >> 1);
        return (int32_t)(((int64_t)mix * env) >> 15);
    }

    if (p->era == ERA_SPACE) {
        int32_t vib = dub_sine_q64(s->siren_lfo_ph + ((uint32_t)p->siren_rate << 17));
        uint32_t inc = dub_note_inc(dub_clamp_midi(root_m));

        inc = (uint32_t)(((uint64_t)inc * (uint64_t)(65536 + (vib >> 8))) >> 16);
        s->siren_lfo_ph += 600u + (uint32_t)p->siren_rate * 48u;
        s->stab_ph[0] += inc;
        mix = dub_saw(s->stab_ph[0]) + (dub_square(s->stab_ph[0]) >> 1);
        mix = (mix + (dub_sine_q64(s->stab_ph[0] + (s->siren_lfo_ph >> 2)) >> 2));
        return (mix * env) >> 15;
    }

    heavy_psych_stab_snap_weights(snap_r, w);
    mix = 0;
    if (w[0] > 0)
        mix += heavy_psych_stab_zone0(s, p, env, root_m, snap_r, pk) * w[0];
    if (w[1] > 0)
        mix += heavy_psych_stab_zone1(s, p, env, root_m, snap_r) * w[1];
    if (w[2] > 0)
        mix += heavy_psych_stab_zone2(s, env, root_m, snap_r, pk) * w[2];
    if (w[3] > 0)
        mix += heavy_psych_stab_zone3(s, p, env, root_m, snap_r) * w[3];
    mix >>= 8;

    return (mix * env) >> 15;
}

#endif
