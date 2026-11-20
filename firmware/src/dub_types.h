/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DUB_TYPES_H
#define DUB_TYPES_H

#include <stdint.h>

enum { DUB_STEPS = 32, DUB_SEQ_LANES = 6, DUB_SEQ_STEPS = 128 };

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
enum { DUB_ERA_N = 6, DUB_PLATE_N = 60 };
enum { ERA_KINGSTON = 0, ERA_LOVERS, ERA_CHANNEL, ERA_BRISTOL, ERA_CROYDON, ERA_POSTDUB };
enum {
    BASS_SUB = 0, BASS_808, BASS_REESE, BASS_WOBBLE, BASS_WG, BASS_303, BASS_FM, BASS_PWM,
    BASS_WOB, BASS_SUBS, BASS_N
};
enum { DKIT_ROOTS = 0, DKIT_ARK, DKIT_808, DKIT_909, DKIT_STEPPA, DKIT_CR78, DKIT_CROYDON, DKIT_METAL, DKIT_N };
enum { VOICE_MIN7 = 0, VOICE_MIN9, VOICE_SUS4, VOICE_QUART, VOICE_FIFTH, VOICE_N };
#endif

typedef struct {
    const char *name;
    uint8_t era, bass_model, voicing, tap, tank, bpm, density;
    uint8_t dly_c, rev_c, feedback, loop_filt, wobble, pump;
    uint32_t kick, rim, hat, sub, chord, siren;
} dub_plate_t;

typedef struct {
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t note;
    uint8_t dly_throw, rev_throw, accent;
} dub_events_t;
enum { TAP_SINGLE = 0, TAP_DUAL, TAP_GALLOP, TAP_TRIP, TAP_SPREAD, TAP_PING, TAP_N }; /* complexity order */
enum { DUB_LFO_SINE = 0, DUB_LFO_TRI, DUB_LFO_SQR, DUB_LFO_SH, DUB_LFO_STEP, DUB_LFO_N };
enum {
    /* HOME: Page 1..3, Knobs 1..4 L->R (ids stable; LFO TARGET scroll uses dub_lfo_tgt_order[]) */
    DUB_LFO_M_CHORD = 0,
    DUB_LFO_M_FEED,
    DUB_LFO_M_WOB,
    DUB_LFO_M_ISO,
    DUB_LFO_M_SPACE,
    DUB_LFO_M_WEIGHT,
    DUB_LFO_M_GRIT,
    DUB_LFO_M_DUB,
    DUB_LFO_M_MOTION,
    DUB_LFO_M_FLUX,
    DUB_LFO_ENS,
    DUB_LFO_FLIP,
    /* BASS: Page 1..2 */
    DUB_LFO_MODL_B,
    DUB_LFO_CUT_B,
    DUB_LFO_DEC_B,
    DUB_LFO_WOB,
    DUB_LFO_SUB_B,
    DUB_LFO_DRV_B,
    DUB_LFO_PUMP_B,
    DUB_LFO_DLY_B,
    /* BASS: Page 3 MIX */
    DUB_LFO_LVL_B,
    DUB_LFO_PAN_B,
    DUB_LFO_REV_B,
    /* CHORD: Page 1..2 */
    DUB_LFO_VOIC_C,
    DUB_LFO_CUT_C,
    DUB_LFO_DEC_C,
    DUB_LFO_ATK_C,
    DUB_LFO_DLY_C,
    DUB_LFO_TIMB_C,
    DUB_LFO_RESO_C,
    DUB_LFO_ENV_C,
    DUB_LFO_GLIDE_C,
    DUB_LFO_PAN_C,
    /* CHORD: Page 3 MIX */
    DUB_LFO_LVL_C,
    DUB_LFO_REV_C,
    /* DRUM / SEL: Page 1..4 */
    DUB_LFO_DENS,
    DUB_LFO_KICK,
    DUB_LFO_RIM,
    DUB_LFO_HAT,
    DUB_LFO_KIT,
    DUB_LFO_KTUNE,
    DUB_LFO_KDEC,
    DUB_LFO_KDRV,
    DUB_LFO_SIREN,
    DUB_LFO_SRATE,
    DUB_LFO_DLY_D,
    DUB_LFO_REV_D,
    DUB_LFO_PAN_K,
    DUB_LFO_PAN_R,
    DUB_LFO_PAN_H,
    DUB_LFO_PAN_S,
    DUB_LFO_KEY,
    DUB_LFO_SCALE,
    DUB_LFO_SWING,
    DUB_LFO_TEMPO,
    /* FX / ECHO: Page 1..3 */
    DUB_LFO_EMIX,
    DUB_LFO_TIME,
    DUB_LFO_FB,
    DUB_LFO_LOOP,
    DUB_LFO_WOW,
    DUB_LFO_TAP,
    DUB_LFO_TANK,
    DUB_LFO_ISO,
    DUB_LFO_REV,
    DUB_LFO_RDAMP,
    DUB_LFO_RTAIL,
    DUB_LFO_ESEND,
    /* SEQ lanes (per-voice via seq_lfo_lane[9]) */
    DUB_LFO_SEQ_PAT,
    DUB_LFO_SEQ_ROT,
    DUB_LFO_SEQ_VARY,
    DUB_LFO_SEQ_VEL,
    DUB_LFO_SEQ_TRIG,
    DUB_LFO_SEQ_GATE,
    DUB_LFO_SEQ_PITCH,
    DUB_LFO_SEQ_NUDGE,
    DUB_LFO_SEQ_MICRO,
    DUB_LFO_ATK_B,
    DUB_LFO_DCUT,
    DUB_LFO_DATK,
    /* per-drum LFO (K/R/H/S); ALL = KTUNE/KDEC/KDRV/DLY_D/REV_D/DCUT/DATK above */
    DUB_LFO_KTUNE_K,
    DUB_LFO_KTUNE_R,
    DUB_LFO_KTUNE_H,
    DUB_LFO_KTUNE_S,
    DUB_LFO_DCUT_K,
    DUB_LFO_DCUT_R,
    DUB_LFO_DCUT_H,
    DUB_LFO_DCUT_S,
    DUB_LFO_DATK_K,
    DUB_LFO_DATK_R,
    DUB_LFO_DATK_H,
    DUB_LFO_DATK_S,
    DUB_LFO_KDEC_K,
    DUB_LFO_KDEC_R,
    DUB_LFO_KDEC_H,
    DUB_LFO_KDEC_S,
    DUB_LFO_KDRV_K,
    DUB_LFO_KDRV_R,
    DUB_LFO_KDRV_H,
    DUB_LFO_KDRV_S,
    DUB_LFO_DLY_K,
    DUB_LFO_DLY_R,
    DUB_LFO_DLY_H,
    DUB_LFO_DLY_S,
    DUB_LFO_REV_K,
    DUB_LFO_REV_R,
    DUB_LFO_REV_H,
    DUB_LFO_REV_S,
    DUB_LFO_DENS_K,
    DUB_LFO_DENS_R,
    DUB_LFO_DENS_H,
    DUB_LFO_DENS_S,
    DUB_LFO_OCT,
    /* CPL shares: slot depth (0..127) = share of a macro/FLUX/MOTION coupling into an FX/sound param.
     * Appended after OCT so older ids stay put. Rate/wave of these slots are unused. Table: dub_cpl_map[] (fm_app.c). */
    DUB_LFO_TGT_PRE_CPL = DUB_LFO_OCT + 1,
    DUB_LFO_CPL_FB_MFEED = DUB_LFO_TGT_PRE_CPL,
    DUB_LFO_CPL_FB_MDUB,
    DUB_LFO_CPL_FB_MSPACE,
    DUB_LFO_CPL_FB_FLUX,
    DUB_LFO_CPL_WOW_MFEED,
    DUB_LFO_CPL_WOW_MGRIT,
    DUB_LFO_CPL_WOW_FLUX,
    DUB_LFO_CPL_TANK_MSPACE,
    DUB_LFO_CPL_REV_MFEED,
    DUB_LFO_CPL_REV_MSPACE,
    DUB_LFO_CPL_REV_FLUX,
    DUB_LFO_CPL_LOOP_MDUB,
    DUB_LFO_CPL_LOOP_FLUX,
    DUB_LFO_CPL_MOT_PANB,
    DUB_LFO_CPL_MOT_PANC,
    DUB_LFO_CPL_MOT_GLIDE,
    DUB_LFO_CPL_FLX_CUTC,
    DUB_LFO_CPL_FLX_CUTB,
    DUB_LFO_CPL_FLX_WOB,
    DUB_LFO_CPL_FLX_DRMPAN,
    /* SEQ PACE per lane (bound like SEQ_VARY via seq_lfo_lane[DUB_SEQ_LFO_PACE_IX]). Appended last. */
    DUB_LFO_SEQ_PACE,
    DUB_LFO_TGT_PRE_ENVX,
#if !defined(GENRE_BASS)
    /* Genre KO KIT ENV per lane (genre_vox.h): DUB_LFO_ENVX + param * 6 + lane, params SUS REL DLY HOLD CURV PDEC
     * SWEEP, lanes = seq lanes. Appended last; persisted sparsely in dub_cfg_t.envx_lfo. */
    DUB_LFO_ENVX = DUB_LFO_TGT_PRE_ENVX,
    DUB_LFO_ENVX_END = DUB_LFO_ENVX + 7 * 6,
    DUB_LFO_TGT_N = DUB_LFO_ENVX_END
#else
    DUB_LFO_TGT_N = DUB_LFO_TGT_PRE_ENVX
#endif
};
#define DUB_LFO_ENVX_PARAMS 7u
#define DUB_CPL_N (DUB_LFO_TGT_PRE_ENVX - DUB_LFO_TGT_PRE_CPL) /* CPL shares + SEQ_PACE depth (lfo_cpl tail) */

/* seq_lfo_lane[]: 0..8 = DUB_LFO_SEQ_PAT..MICRO, 9 = DUB_LFO_SEQ_PACE. */
#define DUB_SEQ_LFO_PACE_IX 9u
#define DUB_SEQ_LFO_LANE_N 10u

/* SEQ PACE index 0..4 (FM_SEQ_PACE_N = 5: 1/32 .. 1/2). */
#define DUB_PACE_IDX_MAX 4
/* LFO delta (-127..127) -> whole pace steps (-4..4), rounded half away from zero, dead zone +-15. */
static inline int32_t dub_pace_lfo_offset(int32_t delta)
{
    int32_t d = delta < -127 ? -127 : (delta > 127 ? 127 : delta);
    int32_t n = d * DUB_PACE_IDX_MAX;

    return (n >= 0 ? n + 63 : n - 63) / 127;
}
static inline uint8_t dub_pace_clamp(int32_t v)
{
    if (v < 0)
        return 0u;
    if (v > DUB_PACE_IDX_MAX)
        return DUB_PACE_IDX_MAX;
    return (uint8_t)v;
}
/* Effective pace index: base (cfg.seq_pace) + LFO offset, clamped to 0..4. */
static inline uint8_t dub_pace_eff(uint8_t base, int32_t delta)
{
    return dub_pace_clamp((int32_t)base + dub_pace_lfo_offset(delta));
}
enum { DUB_APPLY_CHORD = 1, DUB_APPLY_BASS = 2, DUB_APPLY_DRUM = 4, DUB_APPLY_FX = 8, DUB_APPLY_ALL = 15 };

/* CPL share: x scaled by depth/128 (depth 127 = 1x minus 1 LSB-ish, 64 = 1/2, 32 = 1/4, 43 = 1/3, 96 = 3/4). */
static inline int32_t dub_cpl_scale(int32_t x, uint8_t depth)
{
    return (x * (int32_t)depth) / 128;
}

/* Multi-term CPL sources: one S per coupling = the old term sum (its fixed ratios kept), share = dub_cpl_scale(S, depth).
 * fd / ph: FLUX drift / phase sums; eff: MOTION eff; md: MOTION GLIDE sum. */
static inline int32_t dub_s_fb_flux(int32_t fd, int32_t ph) { return fd / 2 + ph / 6; }
static inline int32_t dub_s_wow_flux(int32_t fd, int32_t ph) { return ph / 2 + fd / 4; }
static inline int32_t dub_s_rev_flux(int32_t fd, int32_t ph) { return fd / 8 + ph / 10; }
static inline int32_t dub_s_loop_flux(int32_t fd, int32_t ph) { return ph / 5 + fd / 10; }
static inline int32_t dub_s_flx_cutc(int32_t fd, int32_t ph) { return (fd * 3) / 4 + ph / 3; }
static inline int32_t dub_s_flx_cutb(int32_t fd, int32_t ph) { return fd / 2 + ph / 4; }
static inline int32_t dub_s_flx_wob(int32_t fd, int32_t ph) { return (fd * 2) / 3 + ph / 4; }
static inline int32_t dub_s_flx_drmpan(int32_t fd, int32_t ph) { return fd / 3 + ph / 6; }
static inline int32_t dub_s_mot_glide(int32_t eff, int32_t md) { return (eff >> 3) + md / 12; }

typedef struct {
    uint8_t era, plate, drum_pat;
    uint8_t timbre, cutoff_c, decay_c, dly_c;
    uint8_t voicing, reso_mode, env_filt, rev_c;
    uint8_t bass_model, cutoff_b, decay_b, wobble;
    uint8_t sub_oct, drive, pump, dly_b;
    uint8_t density, rim_tone, siren_pitch, siren_rate;
    uint8_t kick_boom, hat_lvl;
    uint8_t drm_dly[4]; /* per-drum delay send: 0=KICK, 1=RIM, 2=HAT, 3=SIREN */
    uint8_t drm_rev[4]; /* per-drum reverb send: same order */
    uint8_t dly_time, feedback, rev_mix, loop_filt;
    uint8_t wow, tap, tank, isolator;
    uint8_t dly_lvl, rev_damp, rev_tail, dly_rev;
    uint8_t lfo1_wave, lfo1_rate, lfo1_depth, lfo1_tgt;
    uint8_t lfo1_phase, lfo1_smooth, lfo1_skew, lfo1_algo;
    uint8_t lfo1_ratio, lfo1_index, lfo1_fb, lfo1_fold;
    uint8_t lfo2_wave, lfo2_rate, lfo2_depth, lfo2_tgt;
    uint8_t lfo2_phase, lfo2_smooth, lfo2_skew, lfo2_algo;
    uint8_t lfo2_ratio, lfo2_index, lfo2_fb, lfo2_fold;
    uint8_t bpm, arp_mode, root;
    uint8_t scale, swing, drum_kit;
    uint8_t drm_tune[4]; /* per-drum pitch: 0=KICK, 1=RIM, 2=HAT, 3=SIREN (64=CENTER) */
    uint8_t drm_dec[4];  /* per-drum decay trim: same order (64=CENTER) */
    uint8_t drm_drv[4];  /* per-drum drive: same order */
    uint8_t lvl_b, lvl_c, rev_b;
    uint8_t pan[4];     /* per-bus stereo pan: 0=SUB, 1=CHORD (64=CENTER); 2 unused; 3 = LEAD PUMP depth (0=off) */
    uint8_t drm_pan[4]; /* per-drum stereo pan: 0=KICK, 1=RIM, 2=HAT, 3=SIREN (64=CENTER) */
    uint8_t atk_c, glide_c;
    uint8_t atk_b;
    uint8_t drm_cut[4]; /* per-drum filter cutoff: 64 = legacy open (0=KICK..3=SIREN) */
    uint8_t drm_atk[4]; /* per-drum amp attack: 0 = instant hit */
    uint8_t drm_dens[4]; /* per-voice character morph (PUNCH/SNAP/SIZZLE/TIMBRE), 64=center */
} dub_params_t;

typedef struct {
    uint8_t force; /* bit0 = this step overrides plate gates */
    uint8_t gate;  /* bits: 0 kick, 1 rim, 2 hat, 3 sub, 4 chord, 5 siren */
    uint8_t note;  /* 0 = none, else MIDI note */
    uint8_t dly_throw, rev_throw, accent;
} dub_step_t;

typedef struct {
    dub_params_t p;
    uint8_t mute;   /* bits 0..3 sub, chord, drum, siren */
    uint8_t throw;  /* momentary full delay send, same bits */
    uint8_t freeze;
    int8_t tape_oct; /* -1, 0, +1 */
    uint8_t macro_chord, macro_fb, macro_wob, macro_iso;
    uint8_t macro_space, macro_weight, macro_grit, macro_dub;
    uint8_t macro_motion, macro_flux;
    uint8_t wob_force; /* 0 off, else sync rate index 1..4 */
    int8_t lane_gate[6];
    int8_t lane_tune[6];
    uint8_t lane_mute_mask;  /* bits 0..5: K,R,H,B,C,S — target (UI) */
    uint8_t heard_mute_mask; /* bits 0..5: audio, quant to beat when playing */
    uint8_t lane_solo_mask;
    uint8_t lane_throw_mask; /* bits 0..5: per-lane momentary delay throw (K,R,H,B,C,S) */
    uint8_t saved_lane_lvl[6];
} dub_voice_t;

#endif
