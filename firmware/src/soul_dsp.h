/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SOUL_DSP_H
#define SOUL_DSP_H

#include "soul_data.h"
#include "dub_fm.h"
#include "jazz_pluck.h"

/* --- Drum kit tables (backbeat kick/snare; lane 2 = tamb/hat — soul_hat_*). --- */
static const int8_t soul_kit_k_note[DKIT_N]  = { 30, 34, 32, 36, 35, 33, 31, 34 };
static const uint8_t soul_kit_k_sweep[DKIT_N] = { 10, 22, 14, 18, 16, 20, 12, 16 };
static const uint8_t soul_kit_k_fm[DKIT_N]   = {  2, 12,  4,  8, 10,  6,  3, 14 };
static const uint8_t soul_kit_k_drv[DKIT_N]  = { 14, 18, 12, 16, 14, 20, 10, 16 };
static const int8_t soul_kit_r_note[DKIT_N]  = { 72, 60, 66, 70, 63, 58, 68, 62 };
static const uint8_t soul_kit_r_wire[DKIT_N] = { 82, 88, 76, 90, 86, 72, 84, 80 };

/* hat tables + HAT_STYLE_* live in soul_data.h */

static inline int soul_hat_is_tamb_kit(uint8_t kit)
{
    return soul_hat_style[kit % DKIT_N] == (uint8_t)HAT_STYLE_TAMB;
}

static inline int32_t soul_drm_kick_shape(int32_t kick_sig, uint16_t kick_snap, int32_t k_drv_tot,
                                          uint32_t *noise)
{
    (void)kick_snap;
    if (noise && kick_snap > 120u)
        kick_sig += (dub_noise(noise) * (6 + (k_drv_tot >> 2))) >> 10;
    return dub_softclip_q15((kick_sig * (280 + k_drv_tot * 2)) >> 8);
}

/* Snare shell + wire + clap stack (3 × ~10 ms bursts + tail). */
static inline int32_t soul_drm_snare_mix(int32_t shell, int32_t wire, uint8_t kit_wire, uint8_t rim_drv,
                                         uint16_t rim_age)
{
    int32_t sh = 30 - (kit_wire >> 4);
    int32_t mix_r, clap = 0;
    uint32_t burst_ms = (rim_age * 1000u) / (uint32_t)DUB_FS;

    if (sh < 8)
        sh = 8;
    mix_r = ((shell * sh) >> 7) + ((wire * 3) >> 2);
    if (burst_ms < 35u) {
        if (burst_ms < 12u || (burst_ms >= 18u && burst_ms < 22u) || (burst_ms >= 26u && burst_ms < 30u))
            clap = wire >> 1;
        else if (burst_ms < 50u)
            clap = wire >> 3;
    }
    mix_r += clap;
    return dub_softclip_q15((mix_r * (268 + (int32_t)rim_drv * 2)) >> 8);
}

static inline int32_t soul_drm_bus(int32_t drum_sig, uint8_t kit_drive)
{
    int32_t gain = 256 + ((int32_t)kit_drive * 2);

    drum_sig = (drum_sig * gain) >> 8;
    return dub_softclip_q15(drum_sig);
}

static inline int32_t soul_drm_room_send(int32_t kick_sig, int32_t rim_sig, int32_t hat_sig)
{
    (void)kick_sig;
    return ((rim_sig * 11) >> 7) + ((hat_sig * 9) >> 7);
}

/* --- Bass model helpers --- */
static inline int soul_bass_use_pedal(uint8_t model) { return model == (uint8_t)BASS_PEDAL; }
static inline int soul_bass_use_upright(uint8_t model) { return model == (uint8_t)BASS_UPRIGHT; }
static inline int soul_bass_use_finger(uint8_t model) { return model == (uint8_t)BASS_FINGER; }
static inline int soul_bass_use_flat(uint8_t model) { return model == (uint8_t)BASS_FLAT; }
static inline int soul_bass_use_thumb(uint8_t model) { return model == (uint8_t)BASS_THUMB; }
static inline int soul_bass_use_pick(uint8_t model) { return model == (uint8_t)BASS_PICK; }
static inline int soul_bass_use_octave(uint8_t model) { return model == (uint8_t)BASS_OCTAVE; }
static inline int soul_bass_use_synth(uint8_t model) { return model == (uint8_t)BASS_SYNTH; }
static inline int soul_bass_use_sub(uint8_t model) { return model == (uint8_t)BASS_SUB; }

static inline int32_t soul_bass_pedal_voice(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t fund, sub, raw;

    s->sub_ph += inc;
    s->sub_ph2 += inc >> 1;
    fund = dub_sine_q64(s->sub_ph);
    sub = dub_square(s->sub_ph) >> 2;
    raw = fund + sub + (dub_sine_q64(s->sub_ph2) >> 2);
    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t soul_bass_upright_voice(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso,
                                              int32_t walk_q)
{
    int32_t fund, thump, raw;
    int32_t fc_walk = fc + (walk_q >> 2);

    s->sub_ph += inc;
    fund = dub_sine_q64(s->sub_ph);
    thump = dub_sine_q64(s->sub_ph + (inc >> 1)) >> 2;
    if (s->sub_env > 22000)
        thump += dub_sine_q64(s->sub_ph << 2) >> 4;
    raw = fund + thump;
    dub_svf_mode(raw, fc_walk, &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t soul_bass_finger_voice(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t body, click, raw;

    s->sub_ph += inc;
    body = (s->sub_env > 16000) ? dub_saw(s->sub_ph) : dub_sine_q64(s->sub_ph);
    click = (s->sub_env > 26000) ? (dub_square(s->sub_ph << 2) >> 4) : 0;
    raw = body + click;
    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t soul_bass_pick_voice(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso,
                                           int32_t wob_all)
{
    int32_t saw_w, fund, raw;
    int32_t fc_dyn = fc + (wob_all >> 12);

    s->sub_ph += inc;
    fund = dub_sine_q64(s->sub_ph);
    saw_w = dub_saw(s->sub_ph);
    dub_svf_mode(saw_w, fc_dyn, &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    raw = (fund >> 1) + raw + (dub_saw(s->sub_ph << 1) >> 3);
    return raw;
}

static inline int32_t soul_bass_walk_mod(const dub_params_t *p, int32_t lfo_mod_b)
{
    (void)p;
    return lfo_mod_b;
}

static inline int32_t soul_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t walk = soul_bass_walk_mod(p, lfo_mod_b);
    uint8_t era = soul_clamp_era(p->era);
    int32_t fc_e = fc;

    if (era == (uint8_t)ERA_CHURCH)
        fc_e = (fc * 168) >> 8;
    else if (era == (uint8_t)ERA_MEMPHIS)
        fc_e = (fc * 152) >> 8;
    else if (era == (uint8_t)ERA_STREET)
        fc_e = (fc * 196) >> 8;
    else
        fc_e = (fc * 180) >> 8;

    switch (model) {
    case BASS_PEDAL:
        return soul_bass_pedal_voice(s, inc, fc_e, p->reso_mode);
    case BASS_UPRIGHT:
        return soul_bass_upright_voice(s, inc, fc_e, p->reso_mode, walk);
    case BASS_FINGER:
        return soul_bass_finger_voice(s, inc, fc_e, p->reso_mode);
    case BASS_FLAT:
    case BASS_THUMB: {
        int32_t raw;

        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 1);
        if (model == (uint8_t)BASS_THUMB && s->sub_env > 22000)
            raw += dub_sine_q64(s->sub_ph << 1) >> 2;
        dub_svf_mode(raw, fc_e, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        return raw;
    }
    case BASS_PICK:
        return soul_bass_pick_voice(s, inc, fc_e, p->reso_mode, wob_all);
    case BASS_OCTAVE: {
        int32_t raw;

        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_saw(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, fc_e, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        return raw;
    }
    case BASS_SYNTH: {
        int32_t raw, ix = 22 + (int32_t)p->cutoff_b + (s->sub_env >> 8);
        int32_t fb = 36 + (int32_t)p->wobble;

        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        raw = dub_fm_2op_fb(s->sub_ph, s->sub_ph2, &s->sub_fm_fb, ix, fb);
        dub_svf_mode(raw, fc_e, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        return raw;
    }
    case BASS_SUB:
    default: {
        int32_t raw;

        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, 420 + (fc_e >> 2), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode >> 1), &raw);
        return raw;
    }
    }
}

/* --- Keys timbres (chord lane) --- */
enum {
    SOUL_KEYS_ORGAN = 0, SOUL_KEYS_VIBES, SOUL_KEYS_PIANO, SOUL_KEYS_STRINGS, SOUL_KEYS_EP,
    SOUL_KEYS_TIMBRE_N
};

static inline uint8_t soul_keys_timbre(uint8_t era)
{
    static const uint8_t timbre[GENRE_ERA_N] = {
        SOUL_KEYS_ORGAN, SOUL_KEYS_VIBES, SOUL_KEYS_ORGAN, SOUL_KEYS_PIANO,
        SOUL_KEYS_STRINGS, SOUL_KEYS_EP
    };

    return timbre[soul_clamp_era(era)];
}

static inline int32_t soul_chord_voice_organ(uint32_t ph, int32_t ix, int32_t rotor)
{
    int32_t draw, click, mod;

    draw = (dub_sine_q64(ph) >> 1) + (dub_sine_q64(ph << 1) >> 2) + (dub_square(ph) >> 2);
    mod = (dub_sine_q64(ph * 2u) * rotor) >> 10;
    click = (dub_square(ph << 3) * ix) >> 14;
    return draw + click + ((draw * mod) >> 15);
}

static inline int32_t soul_chord_voice_vibes(uint32_t ph, uint32_t ph2, int32_t ix, int32_t trem)
{
    int32_t body, mod;

    mod = dub_sine_q64(ph2);
    body = dub_fm_ep_tine(ph, ph2, ix);
    return body + ((body * mod * trem) >> 17);
}

static inline int32_t soul_chord_voice_piano(uint32_t ph, int32_t env_q)
{
    int32_t blk = dub_square(ph) + (dub_saw(ph) >> 1);

    return (blk * env_q) >> 16;
}

static inline int32_t soul_chord_voice_strings(uint32_t ph, uint32_t ph2, int32_t det)
{
    int32_t s1, s2;

    s1 = dub_saw(ph);
    s2 = dub_saw(ph2 + ((uint32_t)det << 16));
    return (s1 + s2) >> 1;
}

static inline int32_t soul_chord_voice_ep(uint32_t ph, uint32_t ph2, int32_t ix, int32_t chorus)
{
    int32_t tine, det;

    tine = dub_fm_ep_tine(ph, ph2, ix);
    det = dub_sine_q64(ph + ((uint32_t)chorus << 14)) >> 3;
    return tine + det;
}

static inline int32_t soul_chord_render(dub_synth_t *s, const dub_params_t *p, int32_t ch_root_live,
                                        int32_t glide_sh, int32_t tmb, int32_t lfo_mod_c, int32_t ch_env)
{
    static uint16_t soul_ch_coeff_tag;
    static int32_t soul_ch_ix, soul_ch_fb, soul_ch_ix3, soul_ch_rot;
    uint8_t vo = p->voicing % VOICE_N;
    uint8_t era = soul_clamp_era(p->era);
    uint8_t keys = soul_keys_timbre(era);
    int32_t mix = 0;
    int32_t env_q = ch_env;
    uint32_t vi;
    uint16_t coeff_tag = (uint16_t)((s->ch_ph[0] >> 17) & 0xfffu) ^ ((uint16_t)era << 12) ^ (uint16_t)vo;

    if (env_q < 0)
        env_q = 0;
    else if (env_q > 32767)
        env_q = 32767;
    if (coeff_tag != soul_ch_coeff_tag) {
        soul_ch_coeff_tag = coeff_tag;
        soul_ch_ix = ((8 + ((int32_t)p->cutoff_c >> 2) + (lfo_mod_c >> 6)) * env_q) >> 15;
        soul_ch_fb = ((10 + ((int32_t)p->wobble >> 2)) * env_q) >> 15;
        soul_ch_ix3 = ((4 + ((int32_t)p->timbre >> 2) + (tmb >> 5)) * env_q) >> 15;
        soul_ch_rot = 24 + (lfo_mod_c >> 7) + ((int32_t)p->wobble >> 3);
    }

    for (vi = 0; vi < DUB_CHORD_N; vi++) {
        int32_t voice;
        uint8_t n = dub_clamp_midi(ch_root_live + (int32_t)dub_chord_interval(vo, (uint8_t)vi));
        uint32_t tgt_inc = dub_note_inc(n);
        uint32_t inc;

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
        s->ch_ph[vi] += inc;
        switch (keys) {
        case SOUL_KEYS_VIBES:
            voice = soul_chord_voice_vibes(s->ch_ph[vi], s->ch_ph[vi] + (inc >> 1),
                                           soul_ch_ix + 6, soul_ch_rot);
            break;
        case SOUL_KEYS_PIANO:
            voice = soul_chord_voice_piano(s->ch_ph[vi], env_q);
            break;
        case SOUL_KEYS_STRINGS:
            voice = soul_chord_voice_strings(s->ch_ph[vi], s->ch_ph[vi] + inc + (inc >> 3),
                                             soul_ch_ix3);
            break;
        case SOUL_KEYS_EP:
            voice = soul_chord_voice_ep(s->ch_ph[vi], s->ch_ph[vi] + (inc << 1),
                                        soul_ch_ix + 10, soul_ch_fb);
            break;
        case SOUL_KEYS_ORGAN:
        default:
            voice = soul_chord_voice_organ(s->ch_ph[vi], soul_ch_ix, soul_ch_rot);
            if (era == (uint8_t)ERA_MEMPHIS)
                voice = (voice * 5) >> 2;
            break;
        }
        mix += voice >> 2;
    }
    return mix;
}

/* --- Lane-3 horn env / gate / len / bend --- */
typedef struct {
    uint8_t hold_pct;
    uint8_t env_floor_e8;
    uint8_t rel_decay_sh;
    uint8_t gate_cs;
} soul_lead_sustain_t;

static const soul_lead_sustain_t SOUL_LEAD_SUSTAIN[GENRE_ERA_N] = {
    { 84, 6, 11, 48 }, /* CHURCH  — trombone / gospel call */
    { 72, 5, 10, 34 }, /* DETROIT — trumpet riff */
    { 58, 5, 10, 28 }, /* MEMPHIS — section stabs */
    { 76, 6, 11, 40 }, /* NORTHERN — brass fanfare */
    { 88, 7, 12, 52 }, /* PHILLY  — flugel legato */
    { 54, 4,  9, 24 }, /* STREET  — FM brass stab */
};

static inline const soul_lead_sustain_t *soul_lead_sustain(uint8_t era)
{
    return &SOUL_LEAD_SUSTAIN[soul_clamp_era(era)];
}

enum {
    SOUL_HORN_TROMBONE = 0, SOUL_HORN_TRUMPET, SOUL_HORN_SECTION, SOUL_HORN_BRASS,
    SOUL_HORN_FLUGEL, SOUL_HORN_FM_BRASS, SOUL_HORN_TIMBRE_N
};

static inline uint8_t soul_lead_timbre(uint8_t era)
{
    static const uint8_t timbre[GENRE_ERA_N] = {
        SOUL_HORN_TROMBONE, SOUL_HORN_TRUMPET, SOUL_HORN_SECTION, SOUL_HORN_BRASS,
        SOUL_HORN_FLUGEL, SOUL_HORN_FM_BRASS
    };

    return timbre[soul_clamp_era(era)];
}

static inline int soul_lead_section_sax(uint8_t timbre, uint8_t era)
{
    return timbre == (uint8_t)SOUL_HORN_SECTION && era == (uint8_t)ERA_MEMPHIS;
}

static inline void soul_lead_pluck_trig(dub_synth_t *s, const dub_params_t *p, uint32_t inc, uint8_t timbre)
{
    uint8_t dec = p->drm_dec[3] ? p->drm_dec[3] : 64u;
    uint8_t bright = 44u + ((uint32_t)p->siren_rate * 52u) / 127u;

    if (timbre == (uint8_t)SOUL_HORN_FLUGEL)
        bright = (uint8_t)(bright + 6u);
    jazz_pluck_trig(inc, dec, bright, &s->noise, soul_lead_section_sax(timbre, soul_clamp_era(p->era)));
}

static inline uint8_t soul_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = (uint8_t)(8u + ((uint32_t)decay * 5u) / 127u);

    era = soul_clamp_era(era);
    if (era == (uint8_t)ERA_PHILLY)
        body = (uint8_t)(body + 2u);
    else if (era == (uint8_t)ERA_CHURCH)
        body = (uint8_t)(body + 1u);
    return body;
}

static inline uint8_t soul_env_body_gate(uint8_t decay, int8_t lane_gate, uint8_t era)
{
    int32_t g = (int32_t)lane_gate;
    uint8_t body;

    body = soul_env_body(decay, era);
    if (g < 0)
        g = -g;
    body = (uint8_t)(body + (g >> 5));
    if (body > 127u)
        body = 127u;
    return body;
}

enum { SOUL_LEAD_LEN_N = 15u };

static const char *const SOUL_LEAD_LEN_NAMES[SOUL_LEAD_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t SOUL_LEAD_LEN_NUM[SOUL_LEAD_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t SOUL_LEAD_LEN_DEN[SOUL_LEAD_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
static const uint8_t SOUL_LEAD_LEN_16THS[SOUL_LEAD_LEN_N] = {
    1u, 2u, 4u, 5u, 8u, 11u, 12u, 16u, 20u, 24u, 28u, 32u, 40u, 48u, 64u
};

static inline uint8_t soul_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)SOUL_LEAD_LEN_N)
        ix = (int32_t)SOUL_LEAD_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t soul_lead_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = soul_lead_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)SOUL_LEAD_LEN_16THS[ix];
}

static inline uint8_t soul_lead_bend_semi(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

static inline uint8_t soul_lead_bend_plock(uint8_t bend_semi)
{
    if (bend_semi < 3u)
        return 0u;
    return (uint8_t)(32u + ((uint32_t)bend_semi * 95u) / 12u);
}

#define SOUL_LEAD_BEND_SAMPLES ((DUB_FS * 22u) / 1000u)
#define SOUL_VIB_DELAY_SAMPLES ((DUB_FS * 250u) / 1000u)
#define SOUL_BREATH_MAX_SAMPLES ((DUB_FS * 60u) / 1000u)
#define SOUL_BREATH_MIN_SAMPLES ((DUB_FS * 20u) / 1000u)

static inline uint32_t soul_lead_bent_inc(const dub_synth_t *s, uint32_t inc)
{
    uint32_t n = s->siren_note_age;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        uint32_t depth = (uint32_t)s->siren_hp_bend;
        uint32_t abs_semi = (uint32_t)(semi >= 0 ? semi : -semi);
        uint32_t bend_q16 = abs_semi * 2800u * depth / 127u;

        if (bend_q16 > 0u && n < SOUL_LEAD_BEND_SAMPLES) {
            if (semi > 0)
                inc = (uint32_t)(((uint64_t)inc * (65536u - bend_q16 + (bend_q16 * n) / SOUL_LEAD_BEND_SAMPLES)) >> 16);
            else
                inc = (uint32_t)(((uint64_t)inc * (65536u + bend_q16 - (bend_q16 * n) / SOUL_LEAD_BEND_SAMPLES)) >> 16);
        }
    }
    return inc;
}

static inline uint32_t soul_lead_vibrato_inc(const dub_synth_t *s, uint32_t inc, uint8_t timbre)
{
    uint32_t n = s->siren_note_age;
    int32_t depth;

    if (n < SOUL_VIB_DELAY_SAMPLES || timbre == (uint8_t)SOUL_HORN_FM_BRASS)
        return inc;
    depth = timbre == (uint8_t)SOUL_HORN_FLUGEL ? 140 : 90;
    {
        uint32_t vph = n * 42u;
        int32_t lfo = dub_sine_q64(vph) * depth;

        inc = (uint32_t)(((uint64_t)inc * (65536u + (uint32_t)(lfo >> 8))) >> 16);
    }
    return inc;
}

static inline uint16_t soul_siren_atk_samples(uint8_t era, uint8_t atk)
{
    uint16_t a, base;
    uint8_t timbre = soul_lead_timbre(era);

    (void)era;
    switch (timbre) {
    case SOUL_HORN_TRUMPET:
        base = atk ? (uint16_t)(16u + ((uint32_t)atk * 18u) / 127u) : 28u;
        break;
    case SOUL_HORN_FLUGEL:
        base = atk ? (uint16_t)(32u + ((uint32_t)atk * 36u) / 127u) : 52u;
        break;
    case SOUL_HORN_TROMBONE:
        base = atk ? (uint16_t)(28u + ((uint32_t)atk * 24u) / 127u) : 44u;
        break;
    case SOUL_HORN_FM_BRASS:
        base = atk ? (uint16_t)(12u + ((uint32_t)atk * 16u) / 127u) : 20u;
        break;
    default:
        base = atk ? (uint16_t)(22u + ((uint32_t)atk * 28u) / 127u) : 40u;
        break;
    }
    a = base;
    if (a > 140u)
        a = 140u;
    return a;
}

static inline void soul_lead_note_gate_side(uint8_t era, uint8_t side, uint8_t trig_cnt, int8_t lane_gate,
                                            uint32_t *hold_out, uint32_t *rel_out,
                                            uint8_t len_plock_on, uint8_t len_ix,
                                            uint32_t samples_per_step)
{
    static const uint16_t role_scale_q8[4] = { 280u, 220u, 320u, 200u };
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;
    uint8_t role = (uint8_t)((trig_cnt - 1u) & 3u);
    const soul_lead_sustain_t *su = soul_lead_sustain(era);
    uint8_t gate_cs = soul_side_lead_gate_cs(side);

    total = (DUB_FS * (uint32_t)(gate_cs ? gate_cs : su->gate_cs)) / 100u;
    total = (total * (uint32_t)role_scale_q8[role]) >> 8;
    if (g < 0)
        g = -g;
    total += (uint32_t)g * (DUB_FS / 700u);
    if (len_plock_on != 0u && samples_per_step > 0u) {
        uint32_t len_total = soul_lead_len_samples(samples_per_step, len_ix);

        if (len_total < 1u)
            len_total = 1u;
        if (len_total > total)
            total = len_total;
    }
    hold = (total * (uint32_t)su->hold_pct) / 100u;
    if (hold > total)
        hold = total;
    *hold_out = hold;
    *rel_out = total - hold;
}

static inline int32_t soul_lead_release_decay_shift(uint8_t era)
{
    return (int32_t)soul_lead_sustain(era)->rel_decay_sh;
}

static inline void soul_siren_env_step(dub_synth_t *s, const dub_params_t *p)
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
        const soul_lead_sustain_t *su = soul_lead_sustain(s->voice_era);
        int32_t sustain_floor = (s->siren_peak * (int32_t)su->env_floor_e8) >> 3;

        if (s->siren_env > sustain_floor)
            s->siren_env -= ((s->siren_env - sustain_floor) >> 12) + 1;
        else
            s->siren_env = sustain_floor;
        s->siren_gate--;
    } else if (s->siren_lead_rel > 0u) {
        int32_t sh = soul_lead_release_decay_shift(s->voice_era);

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

static inline int32_t soul_lead_clamp_root(uint8_t timbre, int32_t root_m)
{
    int32_t lo = 58, hi = 84;

    switch (timbre) {
    case SOUL_HORN_TROMBONE:
        lo = 45;
        hi = 74;
        break;
    case SOUL_HORN_TRUMPET:
        lo = 58;
        hi = 86;
        break;
    case SOUL_HORN_SECTION:
        lo = 55;
        hi = 80;
        break;
    case SOUL_HORN_BRASS:
        lo = 60;
        hi = 88;
        break;
    case SOUL_HORN_FLUGEL:
        lo = 52;
        hi = 79;
        break;
    case SOUL_HORN_FM_BRASS:
        lo = 50;
        hi = 78;
        break;
    default:
        break;
    }
    if (root_m < lo)
        root_m = lo;
    else if (root_m > hi)
        root_m = hi;
    return root_m;
}

static inline int32_t soul_lead_fm_ix(int32_t env, int32_t bright, uint8_t timbre)
{
    int32_t ix = 18 + bright + (env >> 9);

    if (timbre == (uint8_t)SOUL_HORN_FM_BRASS)
        ix = 28 + bright + (env >> 8);
    else if (timbre == (uint8_t)SOUL_HORN_TRUMPET)
        ix = 22 + bright + (env >> 10);
    if (ix < 4)
        ix = 4;
    if (ix > 56)
        ix = 56;
    return ix;
}

static inline int32_t soul_lead_voice_osc(dub_synth_t *s, uint32_t inc, int32_t env, uint8_t timbre,
                                          int32_t bright)
{
    uint32_t ph = s->stab_ph[0];
    int32_t ix, raw, harm = 0;

    inc = soul_lead_vibrato_inc(s, inc, timbre);
    ix = soul_lead_fm_ix(env, bright, timbre);

    s->stab_ph[0] += inc;
    ph = s->stab_ph[0];

    switch (timbre) {
    case SOUL_HORN_TROMBONE: {
        int32_t sawv = dub_saw(ph) >> 1;

        raw = sawv + (dub_sine_q64(ph) >> 2);
        break;
    }
    case SOUL_HORN_TRUMPET:
        raw = dub_fm_brass(ph, ph, ix) + (dub_saw(ph) >> 3);
        break;
    case SOUL_HORN_SECTION:
        raw = dub_fm_brass(ph, ph, ix);
        if (soul_lead_section_sax(timbre, s->voice_era)) {
            int32_t reed = jazz_pluck_step(inc) + (dub_saw(ph) >> 2);

            raw = (raw >> 1) + (reed >> 1);
        }
        s->stab_ph[1] += inc;
        harm = dub_fm_brass(s->stab_ph[1], s->stab_ph[1], ix - 4) >> 1;
        raw += harm;
        break;
    case SOUL_HORN_BRASS:
        raw = dub_fm_brass(ph, ph, ix);
        s->stab_ph[1] += inc - (inc >> 7);
        harm = dub_fm_brass(s->stab_ph[1] - (s->stab_ph[1] >> 8), s->stab_ph[1], ix - 6) >> 1;
        raw += harm;
        break;
    case SOUL_HORN_FLUGEL:
        raw = dub_fm_brass(ph, ph, ix - 6);
        break;
    case SOUL_HORN_FM_BRASS:
    default: {
        int32_t fb = 40 + (bright >> 1);
        uint32_t det = inc >> 9;

        s->stab_ph[1] += inc + det;
        raw = dub_fm_2op_fb(ph, s->stab_ph[1], &s->sub_fm_fb, ix, fb);
        break;
    }
    }
    (void)env;
    return raw;
}

static inline int32_t soul_lead_tone(dub_synth_t *s, int32_t raw, uint8_t timbre, int32_t env, int32_t fc,
                                     uint8_t reso)
{
    int32_t out = raw;
    int32_t treb_fc = 1500 + (fc >> 2);
    int32_t mid_fc = fc + (fc >> 1);

    if (timbre == (uint8_t)SOUL_HORN_TROMBONE)
        dub_svf_mode(out, fc - (fc >> 3), &s->lead_cab_lp, &s->lead_cab_bp, reso, &out);
    else
        dub_svf_mode(out, fc, &s->lead_cab_lp, &s->lead_cab_bp, reso, &out);
    if (timbre == (uint8_t)SOUL_HORN_TRUMPET || timbre == (uint8_t)SOUL_HORN_BRASS) {
        dub_svf_mode(out, mid_fc, &s->lead_amp_mid_lp, &s->lead_amp_mid_bp, reso, &out);
        out += s->lead_amp_mid_bp >> 1;
    } else if (timbre == (uint8_t)SOUL_HORN_FLUGEL) {
        dub_svf_mode(out, fc - (fc >> 2), &s->lead_amp_treb_lp, &s->lead_amp_treb_bp,
                     (uint8_t)(reso >> 1), &out);
    } else if (soul_lead_section_sax(timbre, s->voice_era)) {
        dub_svf_mode(out, treb_fc, &s->lead_amp_treb_lp, &s->lead_amp_treb_bp, reso, &out);
        out += s->lead_amp_treb_bp >> 2;
    }
    return (int32_t)(((int64_t)out * env) >> 15);
}

static inline int32_t soul_lead_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, root_m, bright, fc;
    uint32_t inc;
    uint8_t era = soul_clamp_era(p->era);
    uint8_t timbre = soul_lead_timbre(era);

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 20 / 64;
    root_m = (int32_t)(p->root ? p->root : 60u) + kt_s + (int32_t)s->siren_st_tune + 12;
    if (timbre == (uint8_t)SOUL_HORN_TROMBONE)
        root_m -= 5;
    root_m = soul_lead_clamp_root(timbre, root_m);

    {
        uint32_t tgt_inc = dub_note_inc(dub_clamp_midi((uint8_t)root_m));
        uint32_t cur_inc = s->siren_lead_inc;

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
        inc = soul_lead_bent_inc(s, cur_inc);
    }
    if (soul_lead_section_sax(timbre, era) && s->siren_note_age == 1u)
        soul_lead_pluck_trig(s, p, inc, timbre);
    bright = 46 + ((int32_t)p->siren_rate * 54) / 127;
    {
        int32_t l_timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;

        bright += (l_timb * 36) / 64;
        if (bright < 8)
            bright = 8;
        else if (bright > 120)
            bright = 120;
    }
    fc = 920 + ((int32_t)p->cutoff_c * 120) + (bright * 8);
    if (timbre == (uint8_t)SOUL_HORN_TROMBONE)
        fc -= fc >> 2;
    else if (timbre == (uint8_t)SOUL_HORN_FLUGEL)
        fc -= fc >> 3;
    if (fc > 14000)
        fc = 14000;

    return soul_lead_tone(s, soul_lead_voice_osc(s, inc, env, timbre, bright), timbre, env, fc,
                          p->reso_mode);
}

static inline int32_t soul_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}

#define SOUL_LEAD_OUT_GAIN_Q12 1650u

static inline int32_t soul_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t lfo_mod_c)
{
    int32_t sig, lvl, breath = 0;
    uint32_t n = s->siren_note_age;

    (void)lfo_mod_c;
    sig = soul_lead_sample(s, p, s->siren_env);
    if (n >= SOUL_BREATH_MIN_SAMPLES && n < SOUL_BREATH_MAX_SAMPLES) {
        int32_t nz = dub_noise(&s->noise);

        dub_svf_mode(nz, 1500, &s->lead_amp_treb_lp, &s->lead_amp_treb_bp, p->reso_mode, &breath);
        breath = (s->lead_amp_treb_bp * (int32_t)(SOUL_BREATH_MAX_SAMPLES - n)) >> 12;
        sig += (breath * s->siren_env) >> 16;
    } else if (n < SOUL_BREATH_MIN_SAMPLES) {
        int32_t atk = (dub_noise(&s->noise) >> 3);

        atk = (atk * (int32_t)(SOUL_BREATH_MIN_SAMPLES - n)) >> 9;
        sig += (atk * s->siren_env) >> 16;
    }
    sig = (sig * (int32_t)(p->siren_pitch ? p->siren_pitch : 96u)) >> 7;
    lvl = p->lvl_c ? p->lvl_c : 96u;
    sig = (sig * (int32_t)lvl) / 92;
    sig = dub_softclip_q15((sig * 430) >> 8);
    sig = (sig * (int32_t)SOUL_LEAD_OUT_GAIN_Q12) >> 12;
    return dub_softclip_q15(sig);
}

/* TAMB lane: dual bandpass noise (~6.5 / 9.5 kHz) + 3 micro-retriggers. */
static inline int32_t soul_hat_render(dub_synth_t *s, int32_t hat_base, uint8_t kit,
                                      int32_t nz_gain, int32_t met_w, int32_t hat_env)
{
    int32_t hm = 0;
    uint32_t r_inc = dub_hat_fm_inc(hat_base);
    int32_t h_met;
    int32_t h_nz, h_nz2;
    uint8_t rst = soul_hat_style[kit];
    int32_t fm = (int32_t)soul_hat_fm[kit];

    (void)hat_env;
    h_met = dub_hat_fm_metal(s, r_inc, (uint8_t)(fm + (rst == HAT_STYLE_TONE ? 12 : 0)), 0, 0);
    h_met = (h_met * met_w) >> 7;
    h_nz = dub_noise(&s->noise);
    dub_svf_mode(h_nz, 6500, &s->hat_f_l1, &s->hat_f_l2, 48, &h_nz);
    h_nz2 = dub_noise(&s->noise);
    dub_svf_mode(h_nz2, 9500, &s->hat_f_l1, &s->hat_f_l2, 36, &h_nz2);
    hm = h_met + (((h_nz + h_nz2) * nz_gain) >> 9);
    if (rst == HAT_STYLE_TAMB || soul_hat_is_tamb_kit(kit)) {
        uint32_t age_ms = (s->hat_snap * 1000u) / (uint32_t)DUB_FS;

        if (age_ms < 18u || (age_ms >= 10u && age_ms < 13u) || (age_ms >= 14u && age_ms < 16u))
            hm += (h_nz2 * 3) >> 2;
    }
    return hm;
}

#endif
