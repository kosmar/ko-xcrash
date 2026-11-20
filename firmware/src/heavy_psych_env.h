/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HEAVY_PSYCH_ENV_H
#define HEAVY_PSYCH_ENV_H

#include "heavy_psych_data.h"

static inline uint8_t heavy_psych_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = (uint8_t)(6u + ((uint32_t)decay * 6u) / 127u);

    era = heavy_psych_clamp_era(era);
    if (era == ERA_DRONE)
        body = (uint8_t)(body + 4u);
    else if (era == ERA_SPACE)
        body = (uint8_t)(body + 3u);
    else if (era == ERA_STONER || era == ERA_DOOM)
        body = (uint8_t)(body + 2u);
    return body;
}

static inline uint8_t heavy_psych_env_body_gate(uint8_t decay, int8_t lane_gate, uint8_t era)
{
    int32_t g = (int32_t)lane_gate;
    uint8_t body;

    body = heavy_psych_env_body(decay, era);
    if (g < 0)
        g = -g;
    body = (uint8_t)(body + (g >> 5));
    if (body > 127u)
        body = 127u;
    return body;
}

/* Chord pad release: era + DECAY only — no step GATE pLock / lane_gate length. */
static inline uint8_t heavy_psych_chord_env_body(uint8_t decay, uint8_t era)
{
    uint8_t body = heavy_psych_env_body(decay, era);

    era = heavy_psych_clamp_era(era);
    if (era == ERA_DRONE || era == ERA_SPACE || era == ERA_GAZE)
        body = (uint8_t)(body + 2u);
    else if (era == ERA_DESERT)
        body = (uint8_t)(body + 1u);
    if (body > 22u)
        body = 22u;
    return body;
}

/* Sustain floor: numerator / 8 of ch_peak (same scale as lead env_floor_e8). */
static inline uint8_t heavy_psych_chord_sustain_floor_e8(uint8_t era)
{
    era = heavy_psych_clamp_era(era);
    if (era == ERA_GAZE || era == ERA_SPACE || era == ERA_DRONE)
        return 7u;
    if (era == ERA_DESERT)
        return 6u;
    if (era == ERA_STONER || era == ERA_DOOM)
        return 5u;
    return 6u;
}

/* No dub_env_step snap chop on chord — pad swell/release only. */
static inline uint16_t heavy_psych_ch_snap_samples(uint8_t decay, uint8_t era)
{
    (void)decay;
    (void)era;
    return 0u;
}

static inline int32_t heavy_psych_chord_drift_fc(int32_t fc, uint8_t era, uint32_t ph0, int32_t lfo_mod_c)
{
    int32_t drift;
    uint32_t rate;

    era = heavy_psych_clamp_era(era);
    if (era == ERA_STONER || era == ERA_DOOM)
        return fc + lfo_mod_c;
    rate = era == ERA_DRONE ? 900u : (era == ERA_SPACE ? 1400u : 2200u);
    drift = dub_sine_q64(ph0 + (ph0 >> 2) + rate) >> 9;
    if (era == ERA_DRONE)
        drift = (drift * 72) >> 6;
    else if (era == ERA_SPACE)
        drift = (drift * 58) >> 6;
    else
        drift = (drift * 40) >> 6;
    fc += drift + (lfo_mod_c >> 1);
    if (fc < 260)
        return 260;
    if (fc > 29000)
        return 29000;
    return fc;
}

static inline uint16_t heavy_psych_ch_attack_samples(uint8_t atk, uint8_t era)
{
    uint16_t s = dub_ch_attack_samples(atk);

    era = heavy_psych_clamp_era(era);
    if (era == ERA_SPACE || era == ERA_GAZE || era == ERA_DRONE)
        s = (uint16_t)(s + (s >> 1) + 880u);
    else if (era == ERA_DESERT)
        s = (uint16_t)(s + (s >> 2) + 440u);
    else if (era == ERA_STONER || era == ERA_DOOM)
        s = (uint16_t)(s + 220u);
    return s;
}

static inline void heavy_psych_ch_env_trig(dub_synth_t *s, uint8_t vel, uint8_t decay, uint8_t atk)
{
    int32_t peak;
    uint8_t era = heavy_psych_clamp_era(s->voice_era);

    if (!s)
        return;
    peak = dub_vel_env_level(vel);
    s->ch_peak = peak;
    s->ch_atk_left = heavy_psych_ch_attack_samples(atk, era);
    if (s->ch_atk_left == 0u) {
        s->ch_env = peak;
        s->ch_snap = heavy_psych_ch_snap_samples(decay, era);
    } else {
        s->ch_env = peak >> 12;
        if (s->ch_env < 1)
            s->ch_env = 1;
        s->ch_snap = 0u;
    }
}

/* Same harmony: no snap/attack chop — swell only on first strike or after full release. */
static inline void heavy_psych_ch_env_legato(dub_synth_t *s, uint8_t vel, uint8_t decay, uint8_t atk,
                                             uint8_t root)
{
    int32_t peak;
    int32_t floor_hold;

    if (!s)
        return;
    peak = dub_vel_env_level(vel);
    floor_hold = s->ch_peak >> 4;
    if (s->ch_env > floor_hold && root == s->ch_root && root == s->ch_root_tgt) {
        if (peak > s->ch_peak)
            s->ch_peak = peak;
        if (s->ch_env < s->ch_peak) {
            int32_t diff = s->ch_peak - s->ch_env;
            int32_t step = diff >> 4;

            if (step < 1)
                step = 1;
            s->ch_env += step;
            if (s->ch_env > s->ch_peak)
                s->ch_env = s->ch_peak;
        }
        return;
    }
    heavy_psych_ch_env_trig(s, vel, decay, atk);
}

static inline void heavy_psych_ch_env_step(dub_synth_t *s, uint8_t decay, uint8_t era)
{
    int32_t sustain_floor;
    uint8_t floor_e8;
    int32_t rel_sh;

    if (!s || s->ch_env <= 0)
        return;
    era = heavy_psych_clamp_era(era);
    if (s->ch_atk_left > 0u) {
        int32_t peak = s->ch_peak;
        int32_t diff = peak - s->ch_env;
        int32_t step = (peak >> 6) + 1;

        if (era == ERA_STONER || era == ERA_DOOM)
            step = (peak >> 5) + 1;
        if (step > diff)
            step = diff;
        s->ch_env += step;
        s->ch_atk_left--;
        if (s->ch_atk_left == 0u || s->ch_env >= peak) {
            s->ch_env = peak;
            s->ch_atk_left = 0u;
            s->ch_snap = heavy_psych_ch_snap_samples(decay, era);
        }
        return;
    }
    floor_e8 = heavy_psych_chord_sustain_floor_e8(era);
    sustain_floor = (s->ch_peak * (int32_t)floor_e8) >> 3;
    if (sustain_floor > 0 && s->ch_env > sustain_floor) {
        rel_sh = era == ERA_DRONE ? 15 : (era == ERA_GAZE || era == ERA_SPACE ? 14 : 13);
        s->ch_env -= ((s->ch_env - sustain_floor) >> rel_sh) + 1;
        if (s->ch_env < sustain_floor)
            s->ch_env = sustain_floor;
        return;
    }
    dub_env_step(&s->ch_env, &s->ch_snap, s->ch_body);
}

static inline uint16_t heavy_psych_siren_atk_samples(uint8_t era, uint8_t atk)
{
    uint16_t pick;

    (void)era;
    /* Guitar pick: ~1–3 ms at 44.1 kHz */
    if (atk)
        pick = (uint16_t)(44u + ((uint32_t)atk * 40u) / 127u);
    else
        pick = 66u;
    if (pick > 132u)
        pick = 132u;
    return pick;
}

/* Total note 0.18–0.54 s per trax & 4-bar phrase role:
 * role 0 (bar 1 entry): single held note (~115%); role 1 (bar 2 answer): short figure (~80%);
 * role 2 (bar 3 peak): longest note (~135%); role 3 (bar 4 release): short dip + rest (~75%). */
static inline void heavy_psych_lead_note_gate_trax(uint8_t era, uint8_t trax, uint8_t trig_cnt,
                                                   int8_t lane_gate, uint32_t *hold_out, uint32_t *rel_out,
                                                   uint8_t len_plock_on, uint8_t len_ix,
                                                   uint32_t samples_per_step)
{
    static const uint16_t role_scale_q8[4] = { 294u, 205u, 346u, 192u };
    uint32_t total;
    uint32_t hold;
    int32_t g = (int32_t)lane_gate;
    const heavy_psych_lead_voice_t *lv = heavy_psych_trax_lead_voice(trax);
    uint8_t role = (uint8_t)((trig_cnt - 1u) & 3u);

    {
        const heavy_psych_lead_sustain_t *su = heavy_psych_lead_sustain(era);

        total = (DUB_FS * (uint32_t)(lv->gate_cs ? lv->gate_cs : 26u)) / 100u;
        total = (total * (uint32_t)role_scale_q8[role]) >> 8;
        if (g < 0)
            g = -g;
        total += (uint32_t)g * (DUB_FS / 600u);
        if (len_plock_on != 0u && samples_per_step > 0u) {
            uint32_t len_total = heavy_psych_lead_len_samples(samples_per_step, len_ix);

            if (len_total < 1u)
                len_total = 1u;
            /* gate_cs is the floor; a longer note value wins. */
            if (len_total > total)
                total = len_total;
        }
        hold = (total * (uint32_t)su->hold_pct) / 100u;
    }
    if (hold > total)
        hold = total;
    *hold_out = hold;
    *rel_out = total - hold;
}

static inline void heavy_psych_lead_note_gate(uint8_t era, int8_t lane_gate, uint32_t *hold_out, uint32_t *rel_out)
{
    heavy_psych_lead_note_gate_trax(era, (uint8_t)(era * GENRE_PRESETS_PER_ERA), 1u, lane_gate, hold_out, rel_out, 0u, 0u, 0u);
}

static inline uint16_t heavy_psych_lead_gate_samples(uint8_t era, int8_t lane_gate)
{
    uint32_t hold, rel;

    heavy_psych_lead_note_gate(era, lane_gate, &hold, &rel);
    return hold > 65535u ? 65535u : (uint16_t)hold;
}

static inline int32_t heavy_psych_lead_release_decay_shift(uint8_t era)
{
    return (int32_t)heavy_psych_lead_sustain(era)->rel_decay_sh;
}

static inline void heavy_psych_siren_env_step(dub_synth_t *s, const dub_params_t *p)
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
        const heavy_psych_lead_sustain_t *su = heavy_psych_lead_sustain(s->voice_era);
        int32_t sustain_floor = (s->siren_peak * (int32_t)su->env_floor_e8) >> 3;

        if (s->siren_env > sustain_floor) {
            int32_t gate_sh = 12;

            if (heavy_psych_lead_uses_pluck(s->voice_era))
                gate_sh = 14;
            else if (s->voice_era == ERA_DRONE || s->voice_era == ERA_SPACE || s->voice_era == ERA_DOOM)
                gate_sh = 13;
            s->siren_env -= ((s->siren_env - sustain_floor) >> gate_sh) + 1;
        }
        else
            s->siren_env = sustain_floor;
        s->siren_gate--;
    } else if (s->siren_lead_rel > 0u) {
        int32_t sh = heavy_psych_lead_release_decay_shift(s->voice_era);

        s->siren_env -= (s->siren_env >> sh) + 2;
        if (s->siren_env < 0)
            s->siren_env = 0;
        s->siren_lead_rel--;
    } else if (s->siren_env > 0) {
        s->siren_env -= (s->siren_env >> 9) + 4;
        if (s->siren_env < 0)
            s->siren_env = 0;
    }
    (void)p;
}

static inline int32_t heavy_psych_siren_decay_shift(uint8_t era, uint8_t drm_dec)
{
    int32_t sh = 12 + (((int32_t)drm_dec - 64) >> 5);

    era = heavy_psych_clamp_era(era);
    if (era == ERA_DRONE || era == ERA_SPACE)
        sh += 3;
    else if (era == ERA_DESERT || era == ERA_GAZE)
        sh += 2;
    else if (era == ERA_STONER || era == ERA_DOOM)
        sh += 2;
    if (sh < 11)
        sh = 11;
    else if (sh > 15)
        sh = 15;
    return sh;
}

#endif
