/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_APP_H
#define BASS_APP_H

#include "fm_lfo.h"
#include "bass_env.h"
#include "bass_mod.h"
#include "bass_seq.h"

typedef struct {
    bass_dahdsr_cfg_t env;
    bass_mod_maps_t mods;
    bass_step_x_t step_x[DUB_SEQ_STEPS];
    uint8_t genre_fader;
    uint8_t midi_ch;
    uint8_t glide_time;
    uint8_t live_hold;
    uint8_t live_accent;
    uint8_t live_slide;
    uint8_t live_ratchet;
    uint8_t live_gate;
    uint8_t live_mute;
    uint8_t clock_mode;
    int8_t transpose;
    uint8_t vel_sens;
} bass_cfg_ext_t;

/* User-plate snapshot: only dub_params_t fields KO BASS reads or edits (no drum/chord/siren). */
#define BASS_PLATE_FIELDS(X)                                                                         \
    X(era) X(plate) X(drum_pat) X(reso_mode) X(env_filt) X(bass_model) X(cutoff_b) X(decay_b)        \
    X(wobble) X(sub_oct) X(drive) X(pump) X(dly_b) X(density) X(dly_time) X(feedback) X(rev_mix)    \
    X(loop_filt) X(wow) X(tap) X(tank) X(isolator) X(dly_lvl) X(rev_damp) X(rev_tail) X(dly_rev)    \
    X(lfo1_wave) X(lfo1_rate) X(lfo1_depth) X(lfo1_tgt) X(lfo1_phase) X(lfo1_smooth) X(lfo1_skew)   \
    X(lfo1_algo) X(lfo1_ratio) X(lfo1_index) X(lfo1_fb) X(lfo1_fold) X(lfo2_wave) X(lfo2_rate)      \
    X(lfo2_depth) X(lfo2_tgt) X(lfo2_phase) X(lfo2_smooth) X(lfo2_skew) X(lfo2_algo) X(lfo2_ratio)  \
    X(lfo2_index) X(lfo2_fb) X(lfo2_fold) X(bpm) X(arp_mode) X(root) X(scale) X(swing) X(lvl_b)     \
    X(rev_b) X(atk_b)

typedef struct {
#define BASS_PLATE_DECL(f) uint8_t f;
    BASS_PLATE_FIELDS(BASS_PLATE_DECL)
#undef BASS_PLATE_DECL
    uint8_t pan_b;
} bass_plate_t;

static inline void bass_plate_pack(bass_plate_t *dst, const dub_params_t *src)
{
#define BASS_PLATE_PACK(f) dst->f = src->f;
    BASS_PLATE_FIELDS(BASS_PLATE_PACK)
#undef BASS_PLATE_PACK
    dst->pan_b = src->pan[0];
}

/* dst must already hold a full base (factory plate); only the stored fields are overlaid. */
static inline void bass_plate_unpack(dub_params_t *dst, const bass_plate_t *src)
{
#define BASS_PLATE_UNPACK(f) dst->f = src->f;
    BASS_PLATE_FIELDS(BASS_PLATE_UNPACK)
#undef BASS_PLATE_UNPACK
    dst->pan[0] = src->pan_b;
}

enum {
    BASS_CLOCK_INT = 0,
    BASS_CLOCK_EXT,
    BASS_CLOCK_SEND,
    BASS_CLOCK_N
};

#define BASS_TRANSPOSE_MAX 12
#define BASS_KEY_VEL       100u

typedef struct {
    bass_midi_hold_t midi;
    bass_mod_runtime_t mod;
    uint8_t home_macro_sel;
    uint8_t env_edit_slot;
    uint8_t ratchet_left;
    uint8_t ratchet_note;
    uint8_t ratchet_vel;
    uint8_t ratchet_accent;
    uint16_t ratchet_every;
    uint16_t ratchet_acc;
    uint32_t gate_left;
    uint32_t micro_left;
    uint8_t micro_armed;
    uint8_t micro_st;
    uint8_t micro_note;
    uint8_t micro_vel;
    uint8_t micro_accent;
    /* samples_per_step = FS*60/(bpm*4) truncated; the remainder is spread over steps */
    uint32_t spp_rem;
    uint32_t spp_den;
    uint32_t spp_frac_acc;
    uint8_t spp_extra;
    /* MIDI clock, 24 ppqn = BASS_CLK_PPS pulses per 16th step */
    uint32_t smp_clock;
    uint32_t clk_acc;
    uint8_t clk_pc;
    uint8_t clk_fa_pending;
    uint8_t ext_pulse;
    uint8_t ext_have_last;
    uint32_t ext_last_smp;
    uint32_t ext_spp;
} bass_app_rt_t;

#define BASS_CLK_PPS  6u
#define BASS_CLK_PAIR (BASS_CLK_PPS * 2u)

static inline void bass_cfg_ext_defaults(bass_cfg_ext_t *b)
{
    if (!b)
        return;
    bass_dahdsr_defaults(&b->env);
    bass_mod_maps_defaults(&b->mods);
    bass_step_x_defaults(b->step_x, DUB_SEQ_STEPS);
    b->genre_fader = 64u;
    b->midi_ch = 1u;
    b->glide_time = 24u;
    b->live_hold = 0u;
    b->live_accent = 0u;
    b->live_slide = 0u;
    b->live_ratchet = 1u;
    b->live_gate = 4u;
    b->live_mute = 0u;
    b->clock_mode = BASS_CLOCK_SEND;
    b->transpose = 0;
    b->vel_sens = 127u;
}

static inline void bass_cfg_ext_sanitize(bass_cfg_ext_t *b)
{
    if (!b)
        return;
    if (b->midi_ch < 1u || b->midi_ch > 16u)
        b->midi_ch = 1u;
    if (b->clock_mode >= BASS_CLOCK_N)
        b->clock_mode = BASS_CLOCK_SEND;
    if (b->transpose < -BASS_TRANSPOSE_MAX || b->transpose > BASS_TRANSPOSE_MAX)
        b->transpose = 0;
    if (b->vel_sens > 127u)
        b->vel_sens = 127u;
    /* Saves from before the MAC defaults have all macro rows empty (only M1.1–M1.4 were editable). */
    if (bass_mod_macro_rows_empty(&b->mods))
        bass_mod_macro_rows_defaults(&b->mods);
    {
        uint8_t mi;
        uint8_t si;

        for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
            for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++)
                if (b->mods.macro[mi][si].dest == BASS_MOD_MODEL)
                    b->mods.macro[mi][si].dest = BASS_MOD_NONE;
            if (b->mods.env[si].dest == BASS_MOD_MODEL)
                b->mods.env[si].dest = BASS_MOD_NONE;
        }
    }
}

static inline uint8_t bass_clock_follows_ext(uint8_t mode)
{
    return mode == BASS_CLOCK_EXT ? 1u : 0u;
}

static inline uint8_t bass_clock_sends(uint8_t mode)
{
    return mode == BASS_CLOCK_SEND ? 1u : 0u;
}

static inline uint8_t bass_transpose_note(int32_t note, int8_t transpose)
{
    int32_t nn = note + (int32_t)transpose;

    if (nn < 0)
        nn = 0;
    if (nn > 127)
        nn = 127;
    return (uint8_t)nn;
}

/* 0 = every live note plays at BASS_KEY_VEL, 127 = incoming velocity passes through. */
static inline uint8_t bass_live_vel(uint8_t in_vel, uint8_t sens)
{
    int32_t v = (int32_t)BASS_KEY_VEL + (((int32_t)in_vel - (int32_t)BASS_KEY_VEL) * (int32_t)sens) / 127;

    if (v < 1)
        v = 1;
    if (v > 127)
        v = 127;
    return (uint8_t)v;
}

static inline void bass_app_rt_init(bass_app_rt_t *rt)
{
    if (!rt)
        return;
    bass_midi_hold_init(&rt->midi);
    bass_mod_runtime_clear(&rt->mod);
    rt->home_macro_sel = 0u;
    rt->env_edit_slot = 0u;
    rt->ratchet_left = 0u;
    rt->ratchet_note = 0u;
    rt->ratchet_vel = 0u;
    rt->ratchet_accent = 0u;
    rt->ratchet_every = 0u;
    rt->ratchet_acc = 0u;
    rt->gate_left = 0u;
    rt->micro_left = 0u;
    rt->micro_armed = 0u;
    rt->spp_rem = 0u;
    rt->spp_den = 1u;
    rt->spp_frac_acc = 0u;
    rt->spp_extra = 0u;
    rt->smp_clock = 0u;
    rt->clk_acc = 0u;
    rt->clk_pc = BASS_CLK_PAIR;
    rt->clk_fa_pending = 0u;
    rt->ext_pulse = 0u;
    rt->ext_have_last = 0u;
    rt->ext_last_smp = 0u;
    rt->ext_spp = 0u;
}

/* At each step boundary of the incoming clock (audio sample time now): smoothed
 * samples-per-step, EMA 1/8. Per step, not per pulse, so audio-block jitter stays small. */
static inline void bass_ext_clock_measure(bass_app_rt_t *rt, uint32_t now, uint32_t fs)
{
    uint32_t spp;

    if (!rt)
        return;
    spp = now - rt->ext_last_smp;
    rt->ext_last_smp = now;
    if (!rt->ext_have_last) {
        rt->ext_have_last = 1u;
        return;
    }
    /* 20..300 BPM */
    if (spp < (fs * 60u) / (300u * 4u) || spp > (fs * 60u) / (20u * 4u))
        return;
    if (!rt->ext_spp)
        rt->ext_spp = spp;
    else
        rt->ext_spp = (uint32_t)((int32_t)rt->ext_spp + ((int32_t)spp - (int32_t)rt->ext_spp) / 8);
    if (rt->ext_spp < 32u)
        rt->ext_spp = 32u;
}

static inline void bass_spp_set_frac(bass_app_rt_t *rt, uint32_t num, uint32_t den)
{
    if (!rt)
        return;
    if (!den)
        den = 1u;
    rt->spp_rem = num % den;
    if (den != rt->spp_den) {
        rt->spp_den = den;
        rt->spp_frac_acc = 0u;
    }
}

/* Once per fired step: 1 when the next step needs one extra sample. */
static inline void bass_spp_frac_step(bass_app_rt_t *rt)
{
    if (!rt)
        return;
    rt->spp_frac_acc += rt->spp_rem;
    if (rt->spp_frac_acc >= rt->spp_den) {
        rt->spp_frac_acc -= rt->spp_den;
        rt->spp_extra = 1u;
    } else {
        rt->spp_extra = 0u;
    }
}

/* One incoming F8; returns 1 when it starts a new 16th step. */
static inline uint8_t bass_ext_clock_pulse(bass_app_rt_t *rt, uint32_t now, uint32_t fs)
{
    uint8_t on_step;

    if (!rt)
        return 0u;
    on_step = rt->ext_pulse == 0u ? 1u : 0u;
    if (on_step)
        bass_ext_clock_measure(rt, now, fs);
    rt->ext_pulse = (uint8_t)((rt->ext_pulse + 1u) % BASS_CLK_PPS);
    return on_step;
}

static inline bass_mod_slot_t *bass_home_macro_slot(bass_mod_maps_t *maps, uint8_t pg, uint8_t knob)
{
    uint8_t off;

    if (!maps || pg < 2u || knob > 3u)
        return 0;
    off = pg - 2u;
    if (off >= BASS_MOD_MACRO_N * 2u)
        return 0;
    return &maps->macro[off / 2u][(uint8_t)(((off % 2u) * 4u) + knob)];
}

/* HOME p2 EDIT-hold routing cursor: sel = macro * 8 + slot. */
static inline bass_mod_slot_t *bass_mod_route_slot(bass_mod_maps_t *maps, uint8_t sel)
{
    sel = (uint8_t)(sel % (BASS_MOD_MACRO_N * BASS_MOD_SLOTS_PER_MAP));
    return &maps->macro[sel / BASS_MOD_SLOTS_PER_MAP][sel % BASS_MOD_SLOTS_PER_MAP];
}

static inline uint8_t bass_midi_ch_match(uint8_t cfg_ch, uint8_t st)
{
    uint8_t ch = (uint8_t)((st & 0x0Fu) + 1u);

    if (cfg_ch < 1u)
        cfg_ch = 1u;
    if (cfg_ch > 16u)
        cfg_ch = 16u;
    return ch == cfg_ch ? 1u : 0u;
}

static inline void bass_apply_mod_to_voice(dub_params_t *vp, bass_dahdsr_cfg_t *env, const bass_mod_runtime_t *rt)
{
    if (!vp || !env || !rt)
        return;
    if (rt->cutoff)
        vp->cutoff_b = (uint8_t)fm_lfo_clamp_u8((int32_t)vp->cutoff_b + rt->cutoff, 0, 127);
    if (rt->reso)
        vp->reso_mode = (uint8_t)fm_lfo_clamp_u8((int32_t)vp->reso_mode + rt->reso, 0, 127);
    if (rt->drive)
        vp->drive = (uint8_t)fm_lfo_clamp_u8((int32_t)vp->drive + rt->drive, 0, 127);
    if (rt->delay)
        vp->dly_b = (uint8_t)fm_lfo_clamp_u8((int32_t)vp->dly_b + rt->delay, 0, 127);
    if (rt->rev)
        vp->rev_b = (uint8_t)fm_lfo_clamp_u8((int32_t)vp->rev_b + rt->rev, 0, 127);
    if (rt->env_delay)
        env->t.delay = (uint8_t)fm_lfo_clamp_u8((int32_t)env->t.delay + rt->env_delay, 0, 127);
    if (rt->env_attack)
        env->t.attack = (uint8_t)fm_lfo_clamp_u8((int32_t)env->t.attack + rt->env_attack, 0, 127);
    if (rt->env_hold)
        env->t.hold = (uint8_t)fm_lfo_clamp_u8((int32_t)env->t.hold + rt->env_hold, 0, 127);
    if (rt->env_decay)
        env->t.decay = (uint8_t)fm_lfo_clamp_u8((int32_t)env->t.decay + rt->env_decay, 0, 127);
    if (rt->env_sustain)
        env->t.sustain = (uint8_t)fm_lfo_clamp_u8((int32_t)env->t.sustain + rt->env_sustain, 0, 127);
    if (rt->env_release)
        env->t.release = (uint8_t)fm_lfo_clamp_u8((int32_t)env->t.release + rt->env_release, 0, 127);
    if (rt->env_retrig)
        env->pol.retrigger = (uint8_t)fm_lfo_clamp_u8((int32_t)env->pol.retrigger + rt->env_retrig, 0, 1);
    if (rt->env_invert)
        env->pol.invert = (uint8_t)fm_lfo_clamp_u8((int32_t)env->pol.invert + rt->env_invert, 0, 1);
    if (rt->env_accent)
        env->pol.accent_scale =
            (uint8_t)fm_lfo_clamp_u8((int32_t)env->pol.accent_scale + rt->env_accent, 0, 127);
}

#endif
