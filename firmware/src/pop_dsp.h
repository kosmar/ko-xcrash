/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef POP_DSP_H
#define POP_DSP_H

#include "pop_data.h"
#include "pop_harmony.h"

#if defined(POP_LEAD_VERIFY) && defined(DINGHY_HOST)
extern uint8_t pop_verify_bass_no_ix12;
extern uint8_t pop_verify_bass_legacy_lp;
#endif

static const int8_t pop_kit_k_note[DKIT_N]  = { 32, 34, 30, 36, 38, 33, 35, 37 };
static const uint8_t pop_kit_k_sweep[DKIT_N] = { 14, 20, 18, 16, 22, 12, 14, 18 };
static const uint8_t pop_kit_k_fm[DKIT_N]   = {  4, 18,  8, 12, 22,  6, 10, 14 };
static const uint8_t pop_kit_k_drv[DKIT_N]  = { 16, 20, 14, 16, 22, 12, 14, 18 };
static const int8_t pop_kit_r_note[DKIT_N]  = { 74, 60, 68, 71, 63, 79, 58, 72 };
static const uint8_t pop_kit_r_wire[DKIT_N] = { 86, 72, 84, 88, 92, 76, 96, 82 };

static inline int32_t pop_clamp_ix(int32_t v)
{
    if (v < 4)
        return 4;
    if (v > 160)
        return 160;
    return v;
}

static inline int32_t pop_clamp_ix_ch(int32_t v)
{
    if (v < 0)
        return 0;
    if (v > 120)
        return 120;
    return v;
}

static inline int32_t pop_bass_ep(dub_synth_t *s, uint32_t inc, int32_t index, int32_t wobble_off)
{
    int32_t ix = pop_clamp_ix(index + wobble_off);
    s->sub_ph += inc;
    return dub_fm_ep_tine(s->sub_ph, s->sub_ph, ix);
}

static inline int32_t pop_bass_2op(dub_synth_t *s, uint32_t inc, int32_t index, int32_t fb_amt)
{
    int32_t ix = pop_clamp_ix(index);
    int32_t fb = pop_clamp_ix(fb_amt);
    s->sub_ph += inc;
    s->sub_ph2 += inc << 1;
    return dub_fm_2op_fb(s->sub_ph, s->sub_ph2, &s->sub_fm_fb, ix, fb);
}

static inline int32_t pop_bass_3op(dub_synth_t *s, uint32_t inc, int32_t index, int32_t fb_amt,
                                    int32_t index3, uint32_t ph3_mul)
{
    int32_t ix = pop_clamp_ix(index);
    int32_t ix3 = pop_clamp_ix(index3);
    int32_t fb = pop_clamp_ix(fb_amt);
    s->sub_ph += inc;
    s->sub_ph2 += inc << 1;
    s->sub_ph3 += inc * ph3_mul;
    return dub_fm_3op(s->sub_ph, s->sub_ph2, s->sub_ph3, &s->sub_fm_fb, ix, fb, ix3);
}

static inline int32_t pop_bass_4op(dub_synth_t *s, uint32_t inc, int32_t index, int32_t fb_amt,
                                      int32_t index3, int32_t index4)
{
    uint32_t ph2, ph3, ph4;
    int32_t ix = pop_clamp_ix(index);
    int32_t ix3 = pop_clamp_ix(index3);
    int32_t ix4 = pop_clamp_ix(index4);
    int32_t fb = pop_clamp_ix(fb_amt);
    s->sub_ph += inc;
    ph2 = s->sub_ph << 1;
    ph3 = s->sub_ph * 3u;
    ph4 = s->sub_ph << 2;
    return dub_fm_4op_stack(s->sub_ph, ph2, ph3, ph4, &s->sub_fm_fb, ix, ix3, ix4, fb);
}

static inline int32_t pop_bass_brass(dub_synth_t *s, uint32_t inc, int32_t index, int32_t wobble_off)
{
    int32_t ix = pop_clamp_ix(index + (wobble_off >> 2));
    s->sub_ph += inc;
    return dub_fm_brass(s->sub_ph, s->sub_ph, ix);
}

static inline int32_t pop_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                     int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw;
    int32_t index = 16 + (int32_t)p->cutoff_b + (s->sub_env >> 9) + (lfo_mod_b >> 4);
    int32_t fb_amt = 24 + (int32_t)p->wobble + (wob_all >> 10);

#if defined(POP_LEAD_VERIFY) && defined(DINGHY_HOST)
    if (!pop_verify_bass_no_ix12)
#endif
        index -= 12;
    if (index < 4)
        index = 4;
    int32_t index3 = 8 + (int32_t)p->timbre;
    int32_t index4 = 8 + (int32_t)p->timbre + ((int32_t)p->cutoff_c >> 3);
    uint8_t era = pop_clamp_era(p->era);

    (void)model;
    switch (era) {
    case ERA_TINE:
        raw = pop_bass_ep(s, inc, index, (int32_t)p->wobble >> 2);
        break;
    case ERA_BASS:
    case ERA_STAB:
        raw = pop_bass_2op(s, inc, index, fb_amt);
        break;
    case ERA_HORN:
        raw = pop_bass_brass(s, inc, index, (int32_t)p->wobble);
        break;
    case ERA_BELL:
        raw = pop_bass_3op(s, inc, index, fb_amt, index3, 7u);
        break;
    case ERA_PAD:
    default:
        raw = pop_bass_4op(s, inc, index, fb_amt, index3, index4);
        break;
    }
    {
        int32_t fc_lp;

#if defined(POP_LEAD_VERIFY) && defined(DINGHY_HOST)
        if (pop_verify_bass_legacy_lp) {
            fc_lp = fc - (fc >> 3);
            if (fc_lp < 380)
                fc_lp = 380;
        } else
#endif
        {
            fc_lp = fc - (fc >> 2);
            if (fc_lp < 280)
                fc_lp = 280;
        }
        dub_svf_mode(raw, fc_lp, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
    }
    raw = (raw * (int32_t)POP_BASS_TRIM_Q8[era]) >> 8;
    return raw;
}

static inline int32_t pop_chord_voice(dub_synth_t *s, uint32_t ph, uint32_t inc, uint8_t era,
                                       int32_t index, int32_t fb_amt, int32_t index3, int32_t index4,
                                       int32_t wobble_off, int32_t *ch_fb)
{
    uint32_t ph2, ph3;
    int32_t ix = pop_clamp_ix_ch(index);
    int32_t fb = pop_clamp_ix_ch(fb_amt);
    int32_t ix3 = pop_clamp_ix_ch(index3);
    int32_t ix4 = pop_clamp_ix_ch(index4);
    int32_t body, sub;

    (void)s;
    (void)inc;
    (void)ix4;
    ph2 = ph + (ph >> 3);
    ph3 = ph + (ph >> 2);
    switch (era) {
    case ERA_TINE:
        body = dub_fm_ep_tine(ph, ph2, pop_clamp_ix_ch(ix + (wobble_off >> 2)));
        sub = dub_sine_q64(ph - (ph >> 4)) >> 2;
        return body + sub;
    case ERA_BASS:
        return dub_fm_2op_fb(ph, ph2, ch_fb, ix, fb);
    case ERA_HORN:
        return dub_fm_2op_fb(ph, ph3, ch_fb, ix, fb >> 1);
    case ERA_BELL:
        return dub_fm_3op(ph, ph2, ph3, ch_fb, ix, fb, ix3);
    case ERA_PAD:
        return dub_fm_3op(ph, ph2, ph3, ch_fb, ix, fb >> 1, ix3 >> 1);
    case ERA_STAB:
    default:
        return dub_fm_2op_fb(ph, ph2, ch_fb, ix, fb);
    }
}

static inline int32_t pop_chord_render(dub_synth_t *s, const dub_params_t *p, int32_t ch_root_live,
                                        int32_t glide_sh, int32_t tmb, int32_t lfo_mod_c, int32_t ch_env)
{
    uint8_t vo = p->voicing % VOICE_N;
    uint8_t era = pop_clamp_era(p->era);
    int32_t mix = 0;
    int32_t env_q = ch_env;
    int32_t index, fb_amt, index3, index4;
    uint32_t vi;

    if (env_q < 0)
        env_q = 0;
    else if (env_q > 32767)
        env_q = 32767;
    index = ((6 + ((int32_t)p->cutoff_c >> 2) + (lfo_mod_c >> 6)) * env_q) >> 15;
    fb_amt = ((8 + ((int32_t)p->wobble >> 2)) * env_q) >> 15;
    index3 = ((3 + ((int32_t)p->timbre >> 2) + (tmb >> 5)) * env_q) >> 15;
    index4 = index3;
    if (era == ERA_BASS)
        index = (index * 3) / 4;
    else if (era == ERA_HORN || era == ERA_STAB)
        index = (index * 4) / 5;

    for (vi = 0; vi < DUB_CHORD_N; vi++) {
        int32_t ch_fb = 0;
        int32_t voice;
        uint8_t n = s->pop_ch_note[vi] ? s->pop_ch_note[vi] :
                                      dub_clamp_midi(ch_root_live + (int32_t)dub_chord_interval(vo, (uint8_t)vi));
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
            }
            s->ch_inc[vi] = cur_inc;
        }
        inc = s->ch_inc[vi];
        s->ch_ph[vi] += inc;
        voice = pop_chord_voice(s, s->ch_ph[vi], inc, era, index, fb_amt, index3, index4,
                                 (int32_t)p->wobble >> 2, &ch_fb);
        if (vi == 1u)
            voice += dub_sine_q64(s->ch_ph[vi] - (s->ch_ph[vi] >> 3)) >> 3;
        mix += voice >> 3;
    }
    return mix;
}

static inline uint16_t pop_siren_atk_samples_tex(uint8_t tex, uint8_t atk)
{
    uint32_t n;

    (void)atk;
    switch (tex) {
    case POP_LEAD_TEX_STRING:
        n = (DUB_FS * 10u) / 1000u;
        break;
    case POP_LEAD_TEX_CHOIR:
        n = (DUB_FS * 18u) / 1000u;
        break;
    case POP_LEAD_TEX_SYNTH:
    default:
        n = (DUB_FS * 3u) / 1000u;
        break;
    }
    if (n < 8u)
        n = 8u;
    if (n > 65535u)
        n = 65535u;
    return (uint16_t)n;
}

static inline uint16_t pop_siren_atk_samples(uint8_t era, uint8_t atk)
{
    uint8_t tex = POP_LEAD_TEX_SYNTH;

    (void)era;
    return pop_siren_atk_samples_tex(tex, atk);
}

static inline uint16_t pop_lead_rel_samples(uint8_t tex)
{
    uint32_t n;

    switch (tex) {
    case POP_LEAD_TEX_STRING:
        n = (DUB_FS * 85u) / 1000u;
        break;
    case POP_LEAD_TEX_CHOIR:
        n = (DUB_FS * 110u) / 1000u;
        break;
    case POP_LEAD_TEX_SYNTH:
    default:
        n = (DUB_FS * 45u) / 1000u;
        break;
    }
    if (n < 16u)
        n = 16u;
    return (uint16_t)n;
}

/* Lead GATE p-lock (step_micro[3]). Index 0 = AUTO: no lock, gate follows the gap to the next lead note.
 * Locks are stored as POP_LEAD_LEN_BASE + ix, outside the ±FM_SEQ_MICRO_HALF micro range, so older
 * patterns whose lane-3 micro values were never audible stay AUTO. */
enum { POP_LEAD_LEN_N = 16u, POP_LEAD_LEN_BASE = 31 };
static const char *const POP_LEAD_LEN_NAMES[POP_LEAD_LEN_N] = {
    "AUTO", "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
/* AUTO spans to the next lead note in the grid (seq_lane3_apply_gate_spans clamps). */
static const uint8_t POP_LEAD_LEN_16THS[POP_LEAD_LEN_N] = {
    64u, 1u, 2u, 4u, 5u, 8u, 11u, 12u, 16u, 20u, 24u, 28u, 32u, 40u, 48u, 64u
};

static inline uint8_t pop_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored - POP_LEAD_LEN_BASE;

    if (ix < 1 || ix >= (int32_t)POP_LEAD_LEN_N)
        return 0u;
    return (uint8_t)ix;
}

static inline int8_t pop_lead_len_store(int32_t ix)
{
    if (ix < 1 || ix >= (int32_t)POP_LEAD_LEN_N)
        return 0;
    return (int8_t)(POP_LEAD_LEN_BASE + ix);
}

static inline void pop_lead_arm_trig_gate(dub_synth_t *s, const dub_params_t *p, uint8_t plate, uint32_t span_steps,
                                          int8_t lane_gate)
{
    uint8_t era = pop_clamp_era(p->era);
    uint8_t var = pop_lead_variant(plate);
    uint8_t tex = pop_lead_texture(era, var);
    uint32_t sp = s->siren_hp_spp;

    if (sp < 220u)
        sp = DUB_FS / 8u;
    if (span_steps < 1u)
        span_steps = 1u;
    {
        uint32_t gap = span_steps * sp;
        uint32_t pct;
        int32_t g = (int32_t)lane_gate;

        if (g < 0)
            g = -g;
        if (tex == POP_LEAD_TEX_CHOIR)
            pct = 80u;
        else if (tex == POP_LEAD_TEX_STRING)
            pct = 76u;
        else if (era == ERA_TINE || era == ERA_BELL)
            pct = 44u;
        else
            pct = 56u;
        pct = (uint32_t)((int32_t)pct - (g >> 4));
        if (pct < 36u)
            pct = 36u;
        if (pct > 82u)
            pct = 82u;
        {
            uint32_t rel0 = pop_lead_rel_samples(tex);
            uint32_t hold_max = (gap * pct) / 100u;

            if (hold_max + rel0 > (gap * 9u) / 10u)
                hold_max = ((gap * 9u) / 10u) > rel0 ? ((gap * 9u) / 10u) - rel0 : sp / 6u;
            s->pop_lead_gate_hold = (uint16_t)hold_max;
            if (s->pop_lead_gate_hold < sp / 6u)
                s->pop_lead_gate_hold = (uint16_t)(sp / 6u);
        }
    }
    s->pop_lead_gate_rel = pop_lead_rel_samples(tex);
    s->pop_lead_tex = tex;
    if (tex == POP_LEAD_TEX_CHOIR)
        s->pop_lead_amp_sus_q8 = 217u;
    else if (tex == POP_LEAD_TEX_STRING)
        s->pop_lead_amp_sus_q8 = 191u;
    else
        s->pop_lead_amp_sus_q8 = 166u;
}

static inline void pop_siren_note_gate(uint8_t trig_cnt, int8_t lane_gate, uint16_t *hold_out,
                                        uint16_t *rel_out, uint8_t len_plock_on, uint8_t len_ix,
                                        uint32_t samples_per_step, uint8_t era)
{
    uint32_t sp, hold, rel;
    int32_t g = (int32_t)lane_gate;

    (void)trig_cnt;
    (void)len_plock_on;
    (void)len_ix;
    era = pop_clamp_era(era);
    sp = samples_per_step;
    if (sp < 220u)
        sp = DUB_FS / 8u;
    if (g < 0)
        g = -g;
    hold = (sp * 5u) + (((uint32_t)g * sp) >> 5);
    rel = sp * 2u;
    switch (era) {
    case ERA_TINE:
        hold = (sp * 4u) + (((uint32_t)g * sp) >> 6);
        rel = sp + (sp >> 1);
        break;
    case ERA_BASS:
        hold = (sp * 7u) + (((uint32_t)g * sp) >> 5);
        rel = sp * 2u;
        break;
    case ERA_HORN:
        hold = (sp * 5u) + (((uint32_t)g * sp) >> 5);
        rel = sp + (sp >> 2);
        break;
    case ERA_BELL:
        hold = (sp * 6u) + (((uint32_t)g * sp) >> 5);
        rel = sp * 2u;
        break;
    case ERA_PAD:
        hold = (sp * 12u) + (((uint32_t)g * sp) >> 4);
        rel = sp * 4u;
        break;
    case ERA_STAB:
    default:
        hold = (sp * 5u) + (((uint32_t)g * sp) >> 5);
        rel = sp + (sp >> 1);
        break;
    }
    if (hold > 65000u)
        hold = 65000u;
    if (rel > 32000u)
        rel = 32000u;
    *hold_out = (uint16_t)hold;
    *rel_out = (uint16_t)rel;
}

static inline void pop_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    uint8_t dec_k = p->drm_dec[3] ? p->drm_dec[3] : 64u;
    int32_t sh;

    if ((s->siren_atk_left > 0u || s->siren_gate > 0u || s->siren_lead_rel > 0u || s->siren_env > 0)
        && s->siren_note_age < 65535u)
        s->siren_note_age++;

    if (s->siren_atk_left > 0u) {
        int32_t pk = s->siren_peak;
        int32_t diff = pk - s->siren_env;
        int32_t step = diff >> 2;

        if (s->pop_lead_tex == POP_LEAD_TEX_CHOIR)
            step = diff >> 4;
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
        int32_t pk = s->siren_peak;
        int32_t sus = (pk * (int32_t)s->pop_lead_amp_sus_q8) >> 8;
        uint32_t dec_start = (DUB_FS * 12u) / 100u;

        if (s->siren_note_age > dec_start && s->siren_env > sus) {
            int32_t d = s->siren_env - sus;

            s->siren_env -= (d >> 6) + 1;
            if (s->siren_env < sus)
                s->siren_env = sus;
        }
        s->siren_gate--;
        return;
    }
    if (s->siren_lead_rel > 0u) {
        uint8_t tex = s->pop_lead_tex;

        sh = 12 + (((int32_t)dec_k - 64) >> 5);
        if (tex == POP_LEAD_TEX_CHOIR)
            sh += 1;
        else if (tex == POP_LEAD_TEX_STRING)
            sh -= 1;
        if (sh < 9)
            sh = 9;
        else if (sh > 14)
            sh = 14;
        s->siren_env -= (s->siren_env >> sh) + 1;
        if (s->siren_env < 0)
            s->siren_env = 0;
        s->siren_lead_rel--;
        return;
    }
    if (s->siren_env > 0) {
        s->siren_env -= (s->siren_env >> 11) + 1;
        if (s->siren_env < 0)
            s->siren_env = 0;
    }
}

static inline int32_t pop_lead_tri(uint32_t ph)
{
    int32_t s = dub_saw(ph);

    if (s < 0)
        return -((s + 8192) * 4);
    return (8192 - s) * 4;
}

static inline int32_t pop_lead_mix_saw_sq(uint32_t ph, uint8_t sq_w)
{
    int32_t saw = dub_saw(ph);
    int32_t sq = saw - dub_saw(ph + 0x80000000u);

    return saw - ((sq * (int32_t)sq_w) >> 2);
}

static inline int32_t pop_lead_fc_q15(uint8_t note, int32_t tmb, int32_t cut, int32_t fenv, int32_t fenv_push)
{
    int32_t fc = 200 + (tmb * 22) + ((int32_t)note - 60) * 10;
    int32_t cut_b = (int32_t)(cut ? cut : 64u) - 64;

    fc += cut_b * 5;
    fc += (fenv * fenv_push) >> 9;
    if (fc < 160)
        fc = 160;
    if (fc > 3800)
        fc = 3800;
    return fc;
}

static inline void pop_lead_4pole(int32_t *raw, int32_t fc, uint8_t reso, dub_synth_t *s)
{
    dub_svf_mode(*raw, fc, &s->pop_lead_lp, &s->pop_lead_bp, reso, raw);
    dub_svf_mode(*raw, fc, &s->pop_lead_lp2, &s->pop_lead_bp2, (uint8_t)(reso >> 1), raw);
}

/* Chamberlin band-pass with fc in Hz: g ≈ f·2π/44100 → Q16 = f·9.337; the g² term cancels the
 * band-peak shift of the damped SVF (peak within ±1 % for 270–2290 Hz). */
static inline int32_t pop_svf_band(int32_t in, int32_t hz, uint8_t q, int32_t *l1, int32_t *l2)
{
    int32_t g = (hz * 9561) >> 10;
    int32_t damp = 220 - (((int32_t)q * 22) >> 7);
    int32_t low = *l2;
    int32_t band = *l1;
    int32_t high = in - low - ((band * damp) >> 8);

    g -= (g * g) >> 18;
    band += (int32_t)(((int64_t)high * g) >> 16);
    low += (int32_t)(((int64_t)band * g) >> 16);
    if (band > 28000)
        band = 28000;
    else if (band < -28000)
        band = -28000;
    if (low > 28000)
        low = 28000;
    else if (low < -28000)
        low = -28000;
    *l1 = band;
    *l2 = low;
    return band;
}

static inline void pop_lead_fenv_step(dub_synth_t *s, uint8_t era, int32_t drv, int32_t fenv_var)
{
    int32_t pk = 20000 + (drv * 48) + (fenv_var * 120);
    int32_t sus = ((int32_t)POP_LEAD_FENV_SUS[era] * pk) >> 8;

    if (s->siren_atk_left > 0u) {
        int32_t rise = pk >> 3;

        if (pop_lead_texture(era, pop_lead_variant(s->voice_plate)) == POP_LEAD_TEX_CHOIR)
            rise = pk >> 6;
        s->pop_lead_fenv += rise;
        if (s->pop_lead_fenv > pk)
            s->pop_lead_fenv = pk;
        return;
    }
    if (s->siren_gate > 0u || s->siren_env > (s->siren_peak >> 3)) {
        if (s->siren_note_age < (DUB_FS * 10u) / 100u
            && pop_lead_texture(era, pop_lead_variant(s->voice_plate)) != POP_LEAD_TEX_CHOIR)
            s->pop_lead_fenv += pk >> 5;
        if (s->pop_lead_fenv > sus)
            s->pop_lead_fenv -= (s->pop_lead_fenv - sus) >> 5;
        else if (s->pop_lead_fenv < sus)
            s->pop_lead_fenv += 96;
        return;
    }
    s->pop_lead_fenv -= (s->pop_lead_fenv >> 8) + 2;
    if (s->pop_lead_fenv < 0)
        s->pop_lead_fenv = 0;
}

static inline void pop_lead_glide_step(dub_synth_t *s, const dub_params_t *p, uint32_t tgt, uint8_t glide_on)
{
    uint32_t cur = s->pop_lead_inc;
    uint32_t g = (uint32_t)p->glide_c;

    if (!glide_on)
        g = 0u;
    if (g > 96u)
        g = 96u;
    if (cur == 0u || cur < (tgt >> 2) || cur > (tgt << 2))
        cur = tgt;
    {
        uint32_t max_s = DUB_FS / 25u;
        uint32_t step;

        if (cur < tgt) {
            step = ((tgt - cur) >> (6u + (g >> 4))) + 1u;
            if (step < ((tgt - cur) + max_s - 1u) / max_s)
                step = ((tgt - cur) + max_s - 1u) / max_s;
            cur += step;
            if (cur > tgt)
                cur = tgt;
        } else if (cur > tgt) {
            step = ((cur - tgt) >> (6u + (g >> 4))) + 1u;
            if (step < ((cur - tgt) + max_s - 1u) / max_s)
                step = ((cur - tgt) + max_s - 1u) / max_s;
            if (cur > step)
                cur -= step;
            else
                cur = tgt;
        }
    }
    s->pop_lead_inc = cur;
    s->pop_lead_tgt_inc = tgt;
}

static inline int32_t pop_lead_render(dub_synth_t *s, const dub_params_t *p, int32_t lfo_mod_c)
{
    uint8_t era = pop_clamp_era(p->era);
    uint8_t var = pop_lead_variant(p->plate);
    uint8_t tex = pop_lead_texture(era, var);
    int32_t sn = pop_lead_resolve_midi_core(p, p->plate, s->siren_st_tune, s->pop_lead_oct_lfo_semi, 1u);
    sn = pop_snap_scale_midi((uint8_t)p->root, p->scale, sn);
    uint8_t note = (uint8_t)sn;
    uint32_t tgt_inc = dub_note_inc(note);
    int32_t tmb = (int32_t)p->drm_dens[3];
    int32_t tmb_fc = tmb;

    if (tex == POP_LEAD_TEX_STRING)
        tmb_fc = (tmb * 5) / 3;
    else if (tex == POP_LEAD_TEX_SYNTH)
        tmb_fc = (tmb * 4) / 3;
    int32_t drv = (int32_t)p->siren_rate;
    int32_t fenv_push = (int32_t)POP_LEAD_VAR_FENV[era][var] + (drv >> 2) + (lfo_mod_c >> 7) + (tmb >> 1);
    uint8_t sq_w = POP_LEAD_VAR_SQR[era][var];
    uint8_t det_w = POP_LEAD_VAR_DET[era][var];
    uint8_t vib_on = POP_LEAD_VAR_VIB[era][var];
    uint8_t glide_on = POP_LEAD_VAR_GLIDE[era][var] || era == ERA_BELL;
    uint32_t ph, ph2, ph3, inc, det;
    int32_t raw, vib, fc;
    uint8_t reso = POP_LEAD_RESONANCE[era];
    static const uint16_t POP_VOWL_F1[3] = { 300u, 730u, 270u };
    static const uint16_t POP_VOWL_F2[3] = { 870u, 1090u, 2290u };

    pop_lead_glide_step(s, p, tgt_inc, glide_on);
    inc = s->pop_lead_inc;
    pop_lead_fenv_step(s, era, drv, (int32_t)POP_LEAD_VAR_FENV[era][var]);
    s->pop_lead_chorus += 48696u; /* 0.5 Hz */
    s->pop_lead_ens += 68174u;    /* 0.7 Hz */
    ph = s->siren_ph;
    det = (inc * (uint32_t)det_w) / 4096u;
    if (vib_on && s->siren_note_age > (DUB_FS * 20u) / 100u) {
        /* 5.5 Hz, ±inc/64 ≈ ±27 cents */
        s->siren_lfo_ph += 535650u;
        vib = dub_sine_q64(s->siren_lfo_ph);
        inc += (uint32_t)(int32_t)(((int64_t)(inc >> 6) * vib) >> 15);
    }
    s->siren_ph += inc;
    if (tex == POP_LEAD_TEX_STRING) {
        /* ensemble: ±inc/256 ≈ ±7 cents */
        int32_t ens = (int32_t)(((int64_t)(inc >> 8) * dub_sine_q64(s->pop_lead_ens)) >> 15);
        uint32_t inc2 = inc + det;
        s->pop_lead_det_ph[0] += inc2 + (uint32_t)ens;
        s->pop_lead_det_ph[1] += inc - (det >> 1) - (uint32_t)ens;
        ph2 = s->pop_lead_det_ph[0];
        ph3 = s->pop_lead_det_ph[1];
        raw = dub_saw(ph) + (dub_saw(ph2) >> 1) + (dub_saw(ph3) >> 2);
        raw += dub_sine_q64(ph >> 1) >> 5;
        fc = pop_lead_fc_q15(note, tmb_fc, p->drm_cut[3], s->pop_lead_fenv, fenv_push);
        pop_lead_4pole(&raw, fc, reso, s);
    } else if (tex == POP_LEAD_TEX_CHOIR) {
        int32_t f1, f2, src;
        int32_t fc1, fc2;
        uint8_t tseg = (uint8_t)(tmb << 1);

        if (tseg < 128u) {
            fc1 = (int32_t)POP_VOWL_F1[0] + (((int32_t)POP_VOWL_F1[1] - (int32_t)POP_VOWL_F1[0]) * (int32_t)tseg) / 128;
            fc2 = (int32_t)POP_VOWL_F2[0] + (((int32_t)POP_VOWL_F2[1] - (int32_t)POP_VOWL_F2[0]) * (int32_t)tseg) / 128;
        } else {
            tseg = (uint8_t)(tseg - 128u);
            fc1 = (int32_t)POP_VOWL_F1[1] + (((int32_t)POP_VOWL_F1[2] - (int32_t)POP_VOWL_F1[1]) * (int32_t)tseg) / 128;
            fc2 = (int32_t)POP_VOWL_F2[1] + (((int32_t)POP_VOWL_F2[2] - (int32_t)POP_VOWL_F2[1]) * (int32_t)tseg) / 128;
        }
        src = dub_saw(ph) >> 1;
        {
            int32_t nz = (dub_noise(&s->noise) * 3) >> 12;

            if (s->siren_note_age < (DUB_FS * 12u) / 1000u)
                nz >>= 1;
            src += nz;
        }
        f1 = pop_svf_band(src, fc1, 10u, &s->pop_lead_f1_lp, &s->pop_lead_f1_bp);
        f2 = pop_svf_band(src, fc2, 8u, &s->pop_lead_f2_lp, &s->pop_lead_f2_bp);
        /* smooth vowel level (old per-segment table jumped 26 dB at TIMBRE 43); dark TIMBRE gets
         * up to ×2.8 back for the closing LP; ~−2 dB vs synth eras at TIMBRE 64 */
        int32_t v_lvl = 36 + (tmb < 64 ? 64 - tmb : 0);

        raw = f1 + (f2 >> 1);
        raw = (raw * 9) >> 1;
        raw = (raw * v_lvl) >> 8;
        raw = (raw * 5) >> 2;
        if (s->siren_note_age > (DUB_FS * 20u) / 100u) {
            /* swell ×1 → ×11/4 over 60 ms */
            uint32_t sw = (uint32_t)s->siren_note_age - (DUB_FS * 20u) / 100u;
            int32_t g_q8 = sw >= (DUB_FS * 6u) / 100u ? 704
                                                         : 256 + (int32_t)((sw * 448u) / ((DUB_FS * 6u) / 100u));

            raw = (raw * g_q8) >> 8;
        }
        fc = 320 + (tmb * 14) + ((int32_t)note - 60) * 6;
        dub_svf_mode(raw, fc, &s->pop_lead_lp, &s->pop_lead_bp, 5u, &raw);
    } else {
        /* chorus: ±inc/512 / ±inc/1024 ≈ ±3.4 / ±1.7 cents, quadrature */
        int32_t ch_a = (int32_t)(((int64_t)(inc >> 9) * dub_sine_q64(s->pop_lead_chorus)) >> 15);
        int32_t ch_b = (int32_t)(((int64_t)(inc >> 10) * dub_sine_q64(s->pop_lead_chorus + 0x40000000u)) >> 15);

        s->pop_lead_det_ph[0] += inc + det + (uint32_t)ch_a;
        s->pop_lead_det_ph[1] += inc - det - (uint32_t)ch_b;
        ph2 = s->pop_lead_det_ph[0];
        switch (era) {
        case ERA_TINE:
            raw = pop_lead_mix_saw_sq(ph, 1u) + (dub_saw(ph) >> 2);
            break;
        case ERA_BASS:
            raw = dub_saw(ph) + (dub_saw(ph2) >> 1);
            raw += dub_sine_q64(ph >> 1) >> 4;
            break;
        case ERA_HORN:
            raw = pop_lead_mix_saw_sq(ph, sq_w) + (dub_saw(ph2) >> 1);
            if (drv > 80) {
                int32_t edge = dub_fm_2op_fb(ph, ph, &s->pop_lead_fb, 4, 6);
                raw += edge >> 6;
            }
            break;
        case ERA_BELL:
            raw = dub_saw(ph) + (dub_saw(ph2) >> 1);
            raw += dub_sine_q64(ph >> 1) >> 4;
            break;
        case ERA_PAD:
            raw = pop_lead_tri(ph) + (dub_saw(ph) >> 2);
            break;
        case ERA_STAB:
        default:
            raw = pop_lead_mix_saw_sq(ph, sq_w + 1u) + (dub_saw(ph2) >> 2);
            break;
        }
        if (era == ERA_TINE)
            fc = pop_lead_fc_q15(note, (tmb_fc * 11) >> 4, p->drm_cut[3], s->pop_lead_fenv, fenv_push);
        else
            fc = pop_lead_fc_q15(note, tmb_fc, p->drm_cut[3], s->pop_lead_fenv, fenv_push);
        pop_lead_4pole(&raw, fc, reso, s);
    }
    if (tex != POP_LEAD_TEX_CHOIR)
        raw = (raw * (int32_t)(122 - (tmb >> 3))) >> 7;
    {
        uint8_t tex_mul = 4u;

        if (tex == POP_LEAD_TEX_CHOIR) {
            if (era == ERA_PAD)
                tex_mul = 5u;
            else if (era == ERA_STAB)
                tex_mul = 11u;
            else
                tex_mul = 5u;
        }
        raw = (raw * (int32_t)POP_LEAD_GAIN_Q6[era] * (int32_t)tex_mul) >> 8;
    }
    if (drv > 72)
        raw = dub_softclip_q15(raw + ((raw * (drv - 72)) >> 8));
    {
        int32_t out = (raw * s->siren_env) >> 15;

        if (tex == POP_LEAD_TEX_CHOIR && s->siren_note_age < (DUB_FS * 12u) / 1000u) {
            uint32_t ramp = (DUB_FS * 12u) / 1000u;

            out = (out * (int32_t)s->siren_note_age) / (int32_t)(ramp ? ramp : 1u);
        }
        if (tex != POP_LEAD_TEX_CHOIR)
            out = (out * (int32_t)POP_LEAD_POST_GN[era]) >> 6;
        return out;
    }
}

#endif
