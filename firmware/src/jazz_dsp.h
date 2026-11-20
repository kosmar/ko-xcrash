/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef JAZZ_DSP_H
#define JAZZ_DSP_H

#include "jazz_data.h"
#include "jazz_pluck.h"

/* --- Drum kit tables (kick/rim; lane 2 = ride — see jazz_ride_*). --- */
/* Kick notes kept low (jazz beater, not tom); was up to 42 → thin/high. */
static const int8_t jazz_kit_k_note[DKIT_N]  = { 28, 32, 30, 34, 33, 36, 31, 33 };
static const uint8_t jazz_kit_k_sweep[DKIT_N] = {  6, 18, 22, 12, 14,  8, 10, 12 };
static const uint8_t jazz_kit_k_fm[DKIT_N]   = {  1, 10,  0,  3,  8,  2,  3,  5 };
static const uint8_t jazz_kit_k_drv[DKIT_N]  = { 12, 16,  8,  8, 14,  6, 10, 12 };
static const int8_t jazz_kit_r_note[DKIT_N]  = { 74, 58, 66, 69, 61, 77, 56, 72 };
static const uint8_t jazz_kit_r_wire[DKIT_N] = { 72, 62, 68, 70, 78, 58, 82, 66 };

enum {
    JAZZ_RIDE_SWING = 0, JAZZ_RIDE_BELL, JAZZ_RIDE_WASH, JAZZ_RIDE_ACID_HAT
};

static const int8_t jazz_ride_note[DKIT_N] = {
    88, 90, 86, 92, 84, 80, 94, 96
};
static const uint8_t jazz_ride_fm[DKIT_N] = {
    18, 24, 14, 20, 12,  8, 48, 62
};
static const uint8_t jazz_ride_noise[DKIT_N] = {
    22, 28, 18, 24, 20, 16, 60, 52
};
static const uint8_t jazz_ride_metal[DKIT_N] = {
    92, 88, 96, 90, 86, 78, 68, 72
};
static const uint8_t jazz_ride_decay[DKIT_N] = {
    17, 24, 16, 18, 16, 24,  9,  5
};
static const uint8_t jazz_ride_open[DKIT_N] = {
    13, 13, 10,  7, 10, 13,  7,  9
};
static const uint8_t jazz_ride_style[DKIT_N] = {
    JAZZ_RIDE_SWING, JAZZ_RIDE_BELL, JAZZ_RIDE_WASH, JAZZ_RIDE_SWING,
    JAZZ_RIDE_BELL, JAZZ_RIDE_WASH, JAZZ_RIDE_ACID_HAT, JAZZ_RIDE_ACID_HAT
};
static const uint8_t jazz_ride_cut[DKIT_N] = {
    48, 52, 44, 50, 46, 40, 46, 47
};

static inline int jazz_ride_is_acid_kit(uint8_t kit)
{
    return jazz_ride_style[kit % DKIT_N] == (uint8_t)JAZZ_RIDE_ACID_HAT;
}

static inline int32_t jazz_drm_kick_shape(int32_t kick_sig, uint16_t kick_snap, int32_t k_drv_tot,
                                          uint32_t *noise)
{
    (void)kick_snap;
    (void)noise;
    return dub_softclip_q15((kick_sig * (288 + k_drv_tot * 2)) >> 8);
}

static inline int32_t jazz_drm_rim_mix(int32_t shell, int32_t wire, uint8_t kit_wire, uint8_t rim_drv)
{
    /* More wire in the blend so SNAP (wire amount) is audible vs shell tone */
    int32_t sh = 28 - (kit_wire >> 4);
    int32_t mix_r;

    if (sh < 8)
        sh = 8;
    mix_r = ((shell * sh) >> 7) + ((wire * 3) >> 2);
    return dub_softclip_q15((mix_r * (272 + (int32_t)rim_drv * 2)) >> 8);
}

static inline int32_t jazz_drm_bus(int32_t drum_sig, uint8_t kit_drive)
{
    int32_t gain = 256 + ((int32_t)kit_drive * 2);

    drum_sig = (drum_sig * gain) >> 8;
    return dub_softclip_q15(drum_sig);
}

static inline int32_t jazz_drm_room_send(int32_t kick_sig, int32_t rim_sig, int32_t ride_sig)
{
    (void)kick_sig;
    return ((rim_sig * 10) >> 7) + ((ride_sig * 8) >> 7);
}

/* --- Bass: upright (SPIRIT–MUTE) vs amp (FUSION–BROKEN). --- */
static inline int jazz_bass_use_upright(uint8_t era)
{
    era = jazz_clamp_era(era);
    return era <= (uint8_t)ERA_MUTE;
}

static inline int32_t jazz_bass_upright_voice(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso,
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

static inline int32_t jazz_bass_amp_voice(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso,
                                          int32_t wob_all, int32_t walk_q, int fat)
{
    int32_t saw_w, fund, sub, raw;
    int32_t fc_dyn = fc + (walk_q >> 1) + (wob_all >> 12);

    /* Floor fc so double-SVF can't kill −2oct amp eras. */
    if (fc_dyn < 48)
        fc_dyn = 48;

    s->sub_ph += inc;
    fund = dub_sine_q64(s->sub_ph);
    saw_w = dub_saw(s->sub_ph);
    dub_svf_mode(saw_w, fc_dyn, &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    if (fat) {
        /* FUSION: full sine body + −1oct thump, single SVF (less thin) */
        sub = dub_sine_q64(s->sub_ph >> 1);
        raw = fund + (sub >> 1) + (raw >> 1);
        if (raw > 32767)
            raw = 32767;
        else if (raw < -32768)
            raw = -32768;
        return raw;
    }
    dub_svf_mode(raw, fc_dyn, &s->sub_svf_b1, &s->sub_svf_b2, (uint8_t)(reso >> 1), &raw);
    return (fund >> 1) + raw;
}

static inline int32_t jazz_bass_walk_mod(const dub_params_t *p, int32_t lfo_mod_b)
{
    (void)p;
    return lfo_mod_b;
}

static inline int32_t jazz_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t walk = jazz_bass_walk_mod(p, lfo_mod_b);
    uint8_t era = jazz_clamp_era(p->era);
    int upright = jazz_bass_use_upright(era);
    int32_t fc_e = fc;
    uint8_t reso_amp = p->reso_mode;

    if (era <= (uint8_t)ERA_MUTE)
        fc_e = (fc * (int32_t)(160u + (uint32_t)era * 6u)) >> 8;
    else if (era == (uint8_t)ERA_FUSION)
        fc_e = (fc * 156) >> 8; /* warmer round amp */
    else if (era == (uint8_t)ERA_ACID)
        fc_e = (fc * 192) >> 8; /* open enough for 303-ish bite at −2oct */
    else if (era == (uint8_t)ERA_BROKEN)
        fc_e = (fc * 176) >> 8;
    else
        fc_e = (fc * 188) >> 8;

    {
        int acoustic = (model == (uint8_t)BASS_UPRIGHT || model == (uint8_t)BASS_WALK
                        || model == (uint8_t)BASS_PIZZ);

        if (!acoustic && upright && model != (uint8_t)BASS_FM && model != (uint8_t)BASS_EP
            && model != (uint8_t)BASS_MOOG && model != (uint8_t)BASS_FUSION
            && model != (uint8_t)BASS_AMP)
            acoustic = 1;
        if (acoustic)
            return jazz_bass_upright_voice(s, inc, fc_e, p->reso_mode, walk);
    }
    switch (model) {
    case BASS_FM: {
        int32_t raw, mod_ix, mod3_ix, fb_amt;

        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        s->sub_ph3 += inc * 3u;
        mod_ix = 28 + (int32_t)p->cutoff_b + (s->sub_env >> 8);
        fb_amt = 48 + (int32_t)p->wobble;
        mod3_ix = 16 + (int32_t)p->reso_mode + (int32_t)p->wobble;
        raw = dub_fm_3op(s->sub_ph, s->sub_ph2, s->sub_ph3, &s->sub_fm_fb, mod_ix, fb_amt, mod3_ix);
        dub_svf_mode(raw, fc_e, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        return raw;
    }
    case BASS_EP: {
        int32_t raw, ep_ix;

        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        ep_ix = 18 + (int32_t)p->cutoff_b + (s->sub_env >> 9);
        raw = dub_fm_ep_tine(s->sub_ph, s->sub_ph2, ep_ix);
        dub_svf_mode(raw, fc_e + (fc_e >> 2), &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode >> 1), &raw);
        return raw;
    }
    default: {
        /* ACID reso was +14 → SVF choke at low fc; keep bite milder */
        uint8_t add = (era == (uint8_t)ERA_ACID) ? 6u : (era == (uint8_t)ERA_FUSION) ? 4u : 8u;

        reso_amp = (uint8_t)(reso_amp + add > 127u ? 127u : reso_amp + add);
        return jazz_bass_amp_voice(s, inc, fc_e, reso_amp, wob_all, walk,
                                   era == (uint8_t)ERA_FUSION);
    }
    }
}

/* --- Lane-3 horn env / gate / len / bend. --- */
typedef struct {
    uint8_t hold_pct;
    uint8_t env_floor_e8;
    uint8_t rel_decay_sh;
    uint8_t gate_cs;
} jazz_lead_sustain_t;

static const jazz_lead_sustain_t JAZZ_LEAD_SUSTAIN[JAZZ_ERA_N] = {
    { 82, 6, 11, 44 }, /* SPIRIT  — sax legato / phrase hold */
    { 70, 5, 10, 36 }, /* MODAL   — pedal + long answering tone */
    { 52, 4,  9, 22 }, /* MUTE    — short cup mute */
    { 80, 6, 12, 42 }, /* FUSION  — trombone long / CR tail */
    { 54, 4,  9, 20 }, /* ACID    — synth horn stab */
    { 74, 6, 11, 38 }, /* BROKEN  — clarinet fragments */
};

static inline const jazz_lead_sustain_t *jazz_lead_sustain(uint8_t era)
{
    return &JAZZ_LEAD_SUSTAIN[jazz_clamp_era(era)];
}

enum {
    JAZZ_HORN_SAX = 0, JAZZ_HORN_TRUMPET, JAZZ_HORN_MUTE,
    JAZZ_HORN_TROMBONE, JAZZ_HORN_ACID, JAZZ_HORN_CLARINET, JAZZ_HORN_TIMBRE_N
};

static inline uint8_t jazz_lead_timbre(uint8_t era)
{
    static const uint8_t timbre[JAZZ_ERA_N] = {
        JAZZ_HORN_SAX, JAZZ_HORN_TRUMPET, JAZZ_HORN_MUTE,
        JAZZ_HORN_TROMBONE, JAZZ_HORN_ACID, JAZZ_HORN_CLARINET
    };

    return timbre[jazz_clamp_era(era)];
}

static inline int jazz_lead_uses_karplus(uint8_t timbre)
{
    return timbre == (uint8_t)JAZZ_HORN_SAX || timbre == (uint8_t)JAZZ_HORN_MUTE
           || timbre == (uint8_t)JAZZ_HORN_CLARINET;
}

static inline uint8_t jazz_lead_stoner_pick(uint8_t timbre)
{
    return timbre == (uint8_t)JAZZ_HORN_SAX ? 1u : 0u;
}

static inline void jazz_lead_pluck_trig(dub_synth_t *s, const dub_params_t *p, uint32_t inc, uint8_t timbre)
{
    uint8_t dec = p->drm_dec[3] ? p->drm_dec[3] : 64u;
    uint8_t bright = 40u + ((uint32_t)p->siren_rate * 48u) / 127u;

    if (timbre == (uint8_t)JAZZ_HORN_MUTE) {
        bright = (uint8_t)(bright >> 1);
        dec = (uint8_t)(dec + 16u);
    } else if (timbre == (uint8_t)JAZZ_HORN_CLARINET) {
        bright = (uint8_t)(bright + 8u);
    } else if (timbre == (uint8_t)JAZZ_HORN_SAX) {
        bright = (uint8_t)(bright + 12u);
    }
    jazz_pluck_trig(inc, dec, bright, &s->noise, jazz_lead_stoner_pick(timbre));
}

static inline uint8_t jazz_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = (uint8_t)(8u + ((uint32_t)decay * 5u) / 127u);

    era = jazz_clamp_era(era);
    if (era == (uint8_t)ERA_BROKEN)
        body = (uint8_t)(body + 2u);
    else if (era == (uint8_t)ERA_SPIRIT)
        body = (uint8_t)(body + 1u);
    return body;
}

static inline uint8_t jazz_env_body_gate(uint8_t decay, int8_t lane_gate, uint8_t era)
{
    int32_t g = (int32_t)lane_gate;
    uint8_t body;

    body = jazz_env_body(decay, era);
    if (g < 0)
        g = -g;
    body = (uint8_t)(body + (g >> 5));
    if (body > 127u)
        body = 127u;
    return body;
}

enum { JAZZ_LEAD_LEN_N = 15u };

/* Same GATE note-value labels as HP lead. */
static const char *const JAZZ_LEAD_LEN_NAMES[JAZZ_LEAD_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t JAZZ_LEAD_LEN_NUM[JAZZ_LEAD_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t JAZZ_LEAD_LEN_DEN[JAZZ_LEAD_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
static const uint8_t JAZZ_LEAD_LEN_16THS[JAZZ_LEAD_LEN_N] = {
    1u, 2u, 4u, 5u, 8u, 11u, 12u, 16u, 20u, 24u, 28u, 32u, 40u, 48u, 64u
};

static inline uint8_t jazz_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)JAZZ_LEAD_LEN_N)
        ix = (int32_t)JAZZ_LEAD_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t jazz_lead_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = jazz_lead_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)JAZZ_LEAD_LEN_16THS[ix];
}

static inline uint8_t jazz_lead_bend_semi(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

static inline uint8_t jazz_lead_bend_plock(uint8_t bend_semi)
{
    if (bend_semi < 3u)
        return 0u;
    return (uint8_t)(32u + ((uint32_t)bend_semi * 95u) / 12u);
}

#define JAZZ_LEAD_BEND_SAMPLES ((DUB_FS * 55u) / 1000u)

static inline uint32_t jazz_lead_bent_inc(const dub_synth_t *s, uint32_t inc)
{
    uint32_t n = s->siren_note_age;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        uint32_t depth = (uint32_t)s->siren_hp_bend;
        uint32_t abs_semi = (uint32_t)(semi >= 0 ? semi : -semi);
        uint32_t bend_q16 = abs_semi * 3400u * depth / 127u;

        if (bend_q16 > 0u && n < JAZZ_LEAD_BEND_SAMPLES) {
            if (semi > 0)
                inc = (uint32_t)(((uint64_t)inc * (65536u - bend_q16 + (bend_q16 * n) / JAZZ_LEAD_BEND_SAMPLES)) >> 16);
            else
                inc = (uint32_t)(((uint64_t)inc * (65536u + bend_q16 - (bend_q16 * n) / JAZZ_LEAD_BEND_SAMPLES)) >> 16);
        }
    }
    return inc;
}

static inline uint16_t jazz_siren_atk_samples(uint8_t era, uint8_t atk)
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

static inline void jazz_lead_note_gate_trax(uint8_t era, uint8_t trax, uint8_t trig_cnt, int8_t lane_gate,
                                              uint32_t *hold_out, uint32_t *rel_out,
                                              uint8_t len_plock_on, uint8_t len_ix,
                                              uint32_t samples_per_step)
{
    static const uint16_t role_scale_q8[4] = { 280u, 220u, 320u, 200u };
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;
    uint8_t role = (uint8_t)((trig_cnt - 1u) & 3u);
    const jazz_lead_sustain_t *su = jazz_lead_sustain(era);
    uint8_t gate_cs = jazz_trax_lead_gate_cs(trax);

    total = (DUB_FS * (uint32_t)(gate_cs ? gate_cs : su->gate_cs)) / 100u;
    total = (total * (uint32_t)role_scale_q8[role]) >> 8;
    if (g < 0)
        g = -g;
    total += (uint32_t)g * (DUB_FS / 700u);
    if (len_plock_on != 0u && samples_per_step > 0u) {
        uint32_t len_total = jazz_lead_len_samples(samples_per_step, len_ix);

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

static inline int32_t jazz_lead_release_decay_shift(uint8_t era)
{
    return (int32_t)jazz_lead_sustain(era)->rel_decay_sh;
}

static inline void jazz_siren_env_step(dub_synth_t *s, const dub_params_t *p)
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
        const jazz_lead_sustain_t *su = jazz_lead_sustain(s->voice_era);
        int32_t sustain_floor = (s->siren_peak * (int32_t)su->env_floor_e8) >> 3;

        if (s->siren_env > sustain_floor)
            s->siren_env -= ((s->siren_env - sustain_floor) >> 12) + 1;
        else
            s->siren_env = sustain_floor;
        s->siren_gate--;
    } else if (s->siren_lead_rel > 0u) {
        int32_t sh = jazz_lead_release_decay_shift(s->voice_era);

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

static inline int32_t jazz_siren_decay_shift(uint8_t era, uint8_t drm_dec)
{
    int32_t sh = 12 + (((int32_t)drm_dec - 64) >> 5);

    era = jazz_clamp_era(era);
    if (era == (uint8_t)ERA_BROKEN)
        sh += 1;
    else if (era == (uint8_t)ERA_MUTE)
        sh += 2;
    if (sh < 11)
        sh = 11;
    else if (sh > 15)
        sh = 15;
    return sh;
}

static inline int32_t jazz_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}

/* --- Horn voice: Karplus or saw+formant by era. --- */
static inline int32_t jazz_lead_voice_osc(dub_synth_t *s, uint32_t inc, int32_t env, uint8_t timbre,
                                          int32_t bright)
{
    uint32_t ph = s->stab_ph[0];
    int32_t body;

    if (jazz_lead_uses_karplus(timbre)) {
        int32_t pk = jazz_pluck_step(inc);
        int32_t sine = dub_sine_q64(ph);

        s->stab_ph[0] += inc;
        body = pk + (sine >> (timbre == (uint8_t)JAZZ_HORN_SAX ? 2 : 3));
        if (timbre == (uint8_t)JAZZ_HORN_CLARINET) {
            int32_t mod = dub_sine_q64(ph << 1);

            body += (mod * (8 + (bright >> 4))) >> 11;
        }
        return body;
    }

    s->stab_ph[0] += inc;
    switch (timbre) {
    case JAZZ_HORN_TRUMPET: {
        int32_t ix = 28 + (bright >> 1);

        return dub_fm_brass(ph, ph, ix) + (dub_saw(ph) >> 3);
    }
    case JAZZ_HORN_MUTE: {
        int32_t ix = 14 + (bright >> 2);

        return dub_fm_brass(ph, ph, ix);
    }
    case JAZZ_HORN_TROMBONE: {
        int32_t sawv = dub_saw(ph) >> 1;

        return sawv + (dub_sine_q64(ph) >> 2);
    }
    case JAZZ_HORN_ACID:
    default: {
        int32_t ix = 32 + bright;

        return dub_fm_brass(ph, ph, ix);
    }
    }
    (void)env;
}

static inline int32_t jazz_lead_tone(dub_synth_t *s, int32_t raw, uint8_t timbre, int32_t env, int32_t fc,
                                     uint8_t reso)
{
    int32_t out = raw;
    int32_t treb_fc = 1800 + (fc >> 2);
    int32_t mid_fc = fc + (fc >> 1);

    if (timbre == (uint8_t)JAZZ_HORN_TROMBONE) {
        dub_svf_mode(out, fc - (fc >> 3), &s->lead_cab_lp, &s->lead_cab_bp, reso, &out);
    } else {
        dub_svf_mode(out, fc, &s->lead_cab_lp, &s->lead_cab_bp, reso, &out);
    }
    if (timbre == (uint8_t)JAZZ_HORN_SAX || timbre == (uint8_t)JAZZ_HORN_CLARINET) {
        dub_svf_mode(out, treb_fc, &s->lead_amp_treb_lp, &s->lead_amp_treb_bp, reso, &out);
        out += s->lead_amp_treb_bp >> 2;
    } else if (timbre == (uint8_t)JAZZ_HORN_TRUMPET || timbre == (uint8_t)JAZZ_HORN_ACID) {
        dub_svf_mode(out, mid_fc, &s->lead_amp_mid_lp, &s->lead_amp_mid_bp, reso, &out);
        out += s->lead_amp_mid_bp >> 1;
    } else if (timbre == (uint8_t)JAZZ_HORN_MUTE) {
        dub_svf_mode(out, fc - (fc >> 2), &s->lead_amp_treb_lp, &s->lead_amp_treb_bp,
                     (uint8_t)(reso >> 1), &out);
        /* muted stays dark via the LP; level trimmed to ~−3 dB vs the open horns */
        out = (out * 9) >> 2;
    }
    return (int32_t)(((int64_t)out * env) >> 15);
}

static inline int32_t jazz_lead_sample(dub_synth_t *s, const dub_params_t *p, int32_t env)
{
    int32_t kt_s, root_m, bright, fc;
    uint32_t inc;
    uint8_t era = jazz_clamp_era(p->era);
    uint8_t timbre = jazz_lead_timbre(era);

    kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 20 / 64;
    root_m = (int32_t)(p->root ? p->root : 60u) + kt_s + (int32_t)s->siren_st_tune + 12;
    if (timbre == (uint8_t)JAZZ_HORN_TROMBONE)
        root_m -= 7;
    if (root_m < 55)
        root_m = 55;
    else if (root_m > 84)
        root_m = 84;

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
        inc = jazz_lead_bent_inc(s, cur_inc);
    }
    /* jazz_siren_env_step runs before render: first sample of a note sees age 1 */
    if (jazz_lead_uses_karplus(timbre) && s->siren_note_age == 1u)
        jazz_lead_pluck_trig(s, p, inc, timbre);
    bright = 48 + ((int32_t)p->siren_rate * 56) / 127;
    /* HORN TIMBRE (drm_dens[3]): brightness / breath — bipolar around 64 */
    {
        int32_t l_timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;

        bright += (l_timb * 40) / 64;
        if (bright < 8)
            bright = 8;
        else if (bright > 120)
            bright = 120;
    }
    fc = 900 + ((int32_t)p->cutoff_c * 120) + (bright * 8);
    if (timbre == (uint8_t)JAZZ_HORN_TROMBONE)
        fc -= fc >> 2;
    else if (timbre == (uint8_t)JAZZ_HORN_MUTE)
        fc -= fc >> 3;
    if (fc > 14000)
        fc = 14000;

    return jazz_lead_tone(s, jazz_lead_voice_osc(s, inc, env, timbre, bright), timbre, env, fc,
                          p->reso_mode);
}

/* Post-drive trim (~HP lead presence without fuzz stack). Was 38 → near inaudible. */
#define JAZZ_LEAD_OUT_GAIN_Q12 1600u

static inline int32_t jazz_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t lfo_mod_c)
{
    int32_t sig, lvl;
    uint32_t n = s->siren_note_age;

    (void)lfo_mod_c;
    sig = jazz_lead_sample(s, p, s->siren_env);
    if (n < 120u) {
        int32_t p_env = (int32_t)(120u - n);
        int32_t atk = (dub_noise(&s->noise) >> 3);

        atk = (atk * p_env) >> 9;
        sig += (atk * s->siren_env) >> 16;
    }
    /* LEAD LEVEL = siren_pitch (same as Dub SIREN / HP SING level). */
    sig = (sig * (int32_t)(p->siren_pitch ? p->siren_pitch : 96u)) >> 7;
    lvl = p->lvl_c ? p->lvl_c : 96u;
    sig = (sig * (int32_t)lvl) / 92;
    sig = dub_softclip_q15((sig * 420) >> 8);
    sig = (sig * (int32_t)JAZZ_LEAD_OUT_GAIN_Q12) >> 12;
    return dub_softclip_q15(sig);
}

#endif
