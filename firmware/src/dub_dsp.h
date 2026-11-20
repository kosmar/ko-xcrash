/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DUB_DSP_H
#define DUB_DSP_H

#include "dub_types.h"
#include "fm_lfo.h"
#if defined(GENRE_BASS)
#include "bass_env.h"
#endif
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
#include "dub_data.h"
#endif

#define DUB_FS          44100u
#define DUB_A4_INC      42852281u
#define DUB_SEMI_Q16    69433u
#define DUB_DLY_LEN     32768u
#define DUB_REV_LEN     24576u
#define DUB_WG_LEN      256u
#define DUB_CHORD_N     4u
#include "dub_envx.h"

#if defined(DINGHY_HOST) || defined(DINGHY_WEB)
static int16_t dub_dly_pool[DUB_DLY_LEN];
static int16_t dub_rev_pool[DUB_REV_LEN];
#else
static int16_t dub_dly_pool[DUB_DLY_LEN] __attribute__((section(".pool")));
static int16_t dub_rev_pool[DUB_REV_LEN] __attribute__((section(".pool")));
#endif

/* Reverb region layout (sums to DUB_REV_LEN). */
#define DUB_REV_AP0_OFF   0u
#define DUB_REV_AP0_LEN   142u
#define DUB_REV_AP1_OFF   (DUB_REV_AP0_OFF + DUB_REV_AP0_LEN)
#define DUB_REV_AP1_LEN   107u
#define DUB_REV_AP2_OFF   (DUB_REV_AP1_OFF + DUB_REV_AP1_LEN)
#define DUB_REV_AP2_LEN   379u
#define DUB_REV_AP3_OFF   (DUB_REV_AP2_OFF + DUB_REV_AP2_LEN)
#define DUB_REV_AP3_LEN   277u
#define DUB_REV_CH0_OFF   (DUB_REV_AP3_OFF + DUB_REV_AP3_LEN)
#define DUB_REV_CH0_LEN   113u
#define DUB_REV_CH1_OFF   (DUB_REV_CH0_OFF + DUB_REV_CH0_LEN)
#define DUB_REV_CH1_LEN   181u
#define DUB_REV_CH2_OFF   (DUB_REV_CH1_OFF + DUB_REV_CH1_LEN)
#define DUB_REV_CH2_LEN   307u
#define DUB_REV_T0_OFF    (DUB_REV_CH2_OFF + DUB_REV_CH2_LEN)
#define DUB_REV_T0_LEN    3901u
#define DUB_REV_T1_OFF    (DUB_REV_T0_OFF + DUB_REV_T0_LEN)
#define DUB_REV_T1_LEN    4837u
#define DUB_REV_T2_OFF    (DUB_REV_T1_OFF + DUB_REV_T1_LEN)
#define DUB_REV_T2_LEN    3109u
#define DUB_REV_T3_OFF    (DUB_REV_T2_OFF + DUB_REV_T2_LEN)
#define DUB_REV_T3_LEN    (DUB_REV_LEN - DUB_REV_T3_OFF)

typedef struct {
    uint32_t sub_ph;
    uint32_t sub_ph2;
    uint32_t sub_ph3;
    uint32_t sub_inc;
    uint32_t sub_tgt_inc;
    int32_t sub_env;
    uint16_t sub_snap;
    uint8_t sub_body;
    int32_t sub_peak;
    uint16_t sub_atk_left;
    int32_t sub_svf_l1;
    int32_t sub_svf_l2;
    int32_t sub_svf_b1;
    int32_t sub_svf_b2;
    int32_t sub_fm_fb;
#if defined(GENRE_POP)
    int32_t pop_ch_fb;
    int32_t pop_lead_fb;
    uint8_t pop_ch_note[DUB_CHORD_N];
    int8_t pop_mel_hold;
    uint32_t pop_lead_inc;
    uint32_t pop_lead_tgt_inc;
    int32_t pop_lead_fenv;
    int32_t pop_lead_lp;
    int32_t pop_lead_bp;
    int32_t pop_lead_lp2;
    int32_t pop_lead_bp2;
    uint8_t pop_lead_note;
    uint32_t pop_lead_det_ph[2];
    uint32_t pop_lead_chorus;
    uint32_t pop_lead_ens;
    int32_t pop_lead_f1_lp;
    int32_t pop_lead_f1_bp;
    int32_t pop_lead_f2_lp;
    int32_t pop_lead_f2_bp;
    int8_t pop_lead_oct_lfo_semi;
    uint16_t pop_lead_gate_hold;
    uint16_t pop_lead_gate_rel;
    uint8_t pop_lead_amp_sus_q8;
    uint8_t pop_lead_tex;
#endif
    uint8_t sub_note;
    uint8_t sub_on;
    uint8_t voice_era;
    uint8_t voice_plate;
    uint8_t siren_trig_count;
    uint8_t siren_pluck_arm;
    int8_t siren_prev_tune;
    uint8_t siren_hp_bend;
    int8_t siren_hp_bend_semi;
    uint8_t siren_hp_len_ix;
    uint8_t siren_hp_len_on;
    uint32_t siren_hp_spp;

    uint32_t ch_ph[DUB_CHORD_N];
    uint32_t ch_inc[DUB_CHORD_N];
    int32_t ch_env;
    uint16_t ch_snap;
    uint8_t ch_body;
    int32_t ch_svf_l1;
    int32_t ch_svf_l2;
    uint8_t ch_root;
    uint8_t ch_root_tgt;
    int32_t ch_peak;
    uint16_t ch_atk_left;
    uint32_t ch_tgt_inc[DUB_CHORD_N];

    uint32_t kick_ph;
    uint32_t kick_inc;
    uint32_t kick_tgt_inc;
    int32_t kick_env;
    uint16_t kick_snap;
    uint8_t kick_body;
    int32_t kick_peak;
    uint16_t kick_atk_left;
    int32_t kick_f_l1;
    int32_t kick_f_l2;

    /* Heavy Psych chord fuzz (RAT-style): pre SVF, post SVF, DC blocker */
    /* index 0 = chord fuzz, 1 = lead fuzz */
    int32_t fz_pre_lp[2], fz_pre_bp[2];
    int32_t fz_post_lp[2], fz_post_bp[2];
    int32_t fz_dc[2];

    int32_t rim_env;
    uint16_t rim_snap;
    uint8_t rim_body;
    int32_t rim_peak;
    uint16_t rim_atk_left;
    int32_t rim_f_l1;
    int32_t rim_f_l2;
    uint32_t rim_ph;
    int8_t kick_st_tune;
    uint8_t kick_fresh;
    uint8_t ks_fresh; /* SNOWCRASH: bit0 rim, bit1 hat — excite KS on first render sample */
    int8_t rim_st_tune;
    int8_t hat_st_tune;
    int8_t siren_st_tune;
    int8_t sub_tune_snap;
    int8_t ch_tune_snap;

    uint32_t hat_ph[4];
    int32_t hat_env;
    uint16_t hat_snap;
    uint8_t hat_body;
    int8_t hat_oh_trim; /* open-hat length trim in hat_mul steps, from the preset (0 when closed) */
    int32_t hat_peak;
    uint16_t hat_atk_left;
    int32_t hat_f_l1;
    int32_t hat_f_l2;
    int32_t hat_hp1;
    int32_t hat_hp2;

    uint32_t siren_ph;
    uint32_t siren_lead_inc;
    uint32_t siren_lead_tgt_inc;
    uint8_t siren_lead_glide_on;
    uint32_t siren_lfo_ph;
    uint32_t stab_ph[8];
    int32_t siren_env;
    uint32_t siren_gate;
    uint8_t siren_on;
    uint8_t siren_vel;
    int32_t siren_peak;
    uint16_t siren_atk_left;
    uint32_t siren_lead_rel;
    uint16_t siren_note_age;
    int32_t siren_f_l1;
    int32_t siren_f_l2;

    int16_t wg[DUB_WG_LEN];
    uint32_t wg_w;

    uint32_t dly_w;
    uint8_t dly_half_toggle;
    int32_t dly_hp;
    int32_t dly_lp;
    uint32_t dly_len_sm;
    int32_t dly_lf_sm;
    int32_t dly_loop_dc;
    int32_t sub_send_hp;
    uint32_t wow_ph;

    uint16_t rev_ap_pos[4];
    uint16_t rev_ch_pos[3];
    uint16_t rev_t_pos[4];
    int32_t rev_t_lp[4];

    int32_t pump_env;
    uint16_t crash_left;
    int32_t crash_env;
    uint16_t flam_left[3];
    uint8_t flam_vel[3];

    int32_t lfo_ph[2];
    uint32_t lfo_cycle[2];
    int32_t lfo_sh[2];
    int32_t lfo_slew[2];
    uint32_t noise;

    uint8_t meter[4];
    int32_t iso_lp;
    int32_t iso_hp;
    int32_t iso_lp_r;
    int32_t iso_hp_r;

    /* Heavy Psych lead amp/cab (SING lane only) */
    int32_t lead_amp_hp_lp;
    int32_t lead_amp_hp_bp;
    int32_t lead_amp_mid_lp;
    int32_t lead_amp_mid_bp;
    int32_t lead_cab_lp;
    int32_t lead_cab_bp;
    int32_t lead_cab_body_lp;
    int32_t lead_cab_body_bp;
    int32_t lead_amp_treb_lp;
    int32_t lead_amp_treb_bp;
#if defined(GENRE_BASS)
    bass_dahdsr_rt_t bass_env;
    bass_dahdsr_cfg_t bass_env_cfg;
    uint8_t bass_env_curve; /* KIT CURVE 0..127, 64 = LIN */
    int32_t bass_amp_q15;
    uint8_t bass_slide;
    uint8_t bass_glide_tc;   /* glide time code: τ = 60 ms · 2^((tc − 56) / 32), tc 0..184 */
    uint8_t bass_glide_note; /* last target note the glide offset refers to */
    int32_t bass_glide_q16;  /* current pitch offset from target, semitones Q16 */
#endif
#if !defined(GENRE_BASS)
    dub_envx_v_t envx[DUB_ENVX_LANES]; /* copy of g.cfg.envx (KO KIT ENV per lane) */
    dub_envx_rt_t envx_rt[DUB_ENVX_LANES];
#endif
} dub_synth_t;

static const int16_t DUB_SINE_Q64[64] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403,
    22005, 22594, 23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510,
    28898, 29268, 29621, 29956, 30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285,
    32412, 32521, 32609, 32678, 32728, 32757
};

static uint32_t dub_note_inc(uint8_t note)
{
    static const uint32_t inc_c9[12] = {
        815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
        1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

    if (note > 127u)
        note = 127u;
    return inc_c9[note % 12u] >> (10u - note / 12u);
}

/* 2^(semi_q8 / (12 · 256)) in Q16; semi_q8 in 1/256 semitone, |semi| ≤ 48 st (exp2 table + linear interp). */
static inline uint32_t dub_semi_ratio_q16(int32_t semi_q8)
{
    static const uint32_t st_q16[13] = {
        65536u, 69433u, 73562u, 77936u, 82570u, 87480u, 92682u,
        98193u, 104032u, 110218u, 116772u, 123715u, 131072u
    };
    int32_t oct = 0;
    uint32_t st, fr, r;

    if (semi_q8 > 48 * 256)
        semi_q8 = 48 * 256;
    else if (semi_q8 < -48 * 256)
        semi_q8 = -48 * 256;
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

static int32_t dub_sine_q64(uint32_t ph)
{
    uint32_t q = ph >> 24;
    uint32_t quad = (q >> 6) & 3u;
    uint32_t ix = q & 63u;
    int32_t v = DUB_SINE_Q64[ix];
    int32_t neg = 0;

    if (quad & 1u)
        v = DUB_SINE_Q64[63u - ix];
    if (quad & 2u)
        neg = 1;
    return neg ? -v : v;
}

static int32_t dub_saw(uint32_t ph)
{
    return (int32_t)(int16_t)(ph >> 16) >> 1;
}

static int32_t dub_square(uint32_t ph)
{
    return (ph & 0x80000000u) ? 16000 : -16000;
}

static int32_t dub_noise(uint32_t *st)
{
    *st = *st * 1664525u + 1013904223u;
    return (int32_t)(*st >> 16) - 32768;
}

/* Monotone cubic (3u − u³)/2 on u = x/1.5: unity slope at 0, reaches ±32767 at |x| = 49151, flat beyond. */
static int32_t dub_softclip_q15(int32_t x)
{
    int32_t x2, x3;

    if (x > 49151)
        x = 49151;
    else if (x < -49151)
        x = -49151;
    x2 = (int32_t)(((int64_t)x * x) >> 15);
    x3 = (int32_t)(((int64_t)x2 * x) >> 15);
    return x - ((x3 * 4855) >> 15);
}

/* Grittier loop saturator: x − x³/3, unity slope at 0, saturates at ±21845 for |x| ≥ 32767. */
static int32_t dub_echo_sat_q15(int32_t x)
{
    int32_t x2, x3;

    if (x > 32767)
        x = 32767;
    else if (x < -32767)
        x = -32767;
    x2 = (x * x) >> 15;
    x3 = (x2 * x) >> 15;
    return x - (x3 * 10923 >> 15);
}

#define DUB_FB_ECHO_DIST_START 108 /* top ~15% of 0..127 feedback param */
#define DUB_FB_ECHO_DIST_SPAN  19
#define DUB_FB_ECHO_DIST_AMT   26214 /* ~0.80 Q15 max mix into echo sat at param 127 */

static int32_t dub_master_soft(int32_t x)
{
    x = dub_softclip_q15(x);
    if (x > 28000)
        return 28000;
    if (x < -28000)
        return -28000;
    return x;
}

static uint8_t dub_env_body(uint8_t decay)
{
    /* Shift 6 (tight ~28 ms skank chop at decay=0) up to shift 12 (~1.8 s sustain at decay=127) */
    return (uint8_t)(6u + ((uint32_t)decay * 6u) / 127u);
}

/* Kick/rim decay in quarter-octave τ steps (q4 = body·4): 16 (shift 4, ~5 ms tick)
 * .. 48 (shift 12, ~1,7 s), so kit trims land within ~±9 % of any target length. */
#define DUB_DRM_Q4_MIN 8
#define DUB_DRM_Q4_MAX 64
static uint8_t dub_drm_env_body_q4(uint8_t decay)
{
    return (uint8_t)(16u + ((uint32_t)decay * 32u) / 127u);
}

static uint8_t dub_drm_body_gate_q4(uint8_t decay, int8_t gate)
{
    int32_t b = (int32_t)dub_drm_env_body_q4(decay);

    if (gate < 0)
        b += (int32_t)gate * 8;
    else if (gate > 0)
        b += (int32_t)gate * 5;
    if (b < DUB_DRM_Q4_MIN)
        b = DUB_DRM_Q4_MIN;
    else if (b > DUB_DRM_Q4_MAX)
        b = DUB_DRM_Q4_MAX;
    return (uint8_t)b;
}

/* Hat decay index 0..17 in quarter-octave steps (see hat_mul): decay 0 ≈ 50 ms,
 * +8 decay ≈ +3 steps (one old shift), decay ≥ 46 ≈ 1 s to silence. */
#define DUB_HAT_Q_MAX 17
static uint8_t dub_hat_env_body(uint8_t decay)
{
    uint32_t q = ((uint32_t)decay * 3u) / 8u;

    if (q > DUB_HAT_Q_MAX)
        q = DUB_HAT_Q_MAX;
    return (uint8_t)q;
}

static uint8_t dub_hat_body_gate(uint8_t decay, int8_t gate)
{
    int32_t b = (int32_t)dub_hat_env_body(decay);

    if (gate != 0)
        b += ((int32_t)gate * 3) / 4;
    if (b < 0)
        b = 0;
    else if (b > DUB_HAT_Q_MAX)
        b = DUB_HAT_Q_MAX;
    return (uint8_t)b;
}

static uint8_t dub_env_body_gate(uint8_t decay, int8_t gate)
{
    uint8_t body = dub_env_body(decay);

    if (gate == 0)
        return body;
    if (gate < 0) {
        int32_t b = (int32_t)body + (int32_t)gate * 2;

        if (b < 2)
            b = 2;
        return (uint8_t)b;
    }
    {
        int32_t b = (int32_t)body + ((int32_t)gate * 5) / 4;

        if (b > 16)
            b = 16;
        return (uint8_t)b;
    }
}

static int32_t dub_vel_env_level(uint8_t vel)
{
    int32_t v = vel < 1u ? 1 : (vel > 127u ? 127 : (int32_t)vel);

    return 1200 + (v * v * 31567) / (127 * 127);
}

static uint8_t dub_clamp_midi(int32_t nn)
{
    if (nn < 0)
        nn = 0;
    if (nn > 127)
        nn = 127;
    return (uint8_t)nn;
}

/* Exponential decrement env·mul/2^20 with stochastic rounding of the 20 fractional bits:
 * the mean decay stays exponential down to env 1 (no linear +1 tail, no stall). */
static int32_t dub_env_decr(int32_t env, uint32_t mul)
{
    static uint32_t dither = 0x9E3779B9u;

    if (mul == 0u)
        mul = 1u;
    dither = dither * 1664525u + 1013904223u;
    return (int32_t)(((uint64_t)(uint32_t)env * mul + (dither >> 12)) >> 20);
}

static void dub_env_step(int32_t *env, uint16_t *snap, uint8_t body)
{
    if (*env <= 0)
        return;
    if (*snap > 0u) {
        /* Punchy ~10 ms transient snap that preserves 75% body level instead of killing 95% in 10 samples */
        *env -= (*env >> 9) + 2;
        if (*env < 0)
            *env = 0;
        (*snap)--;
    } else {
        /* τ = 2^body · 16/9 samples: body 6 ≈ 27 ms, body 12 ≈ 1,7 s to silence */
        *env -= dub_env_decr(*env, body < 20u ? 589824u >> body : 1u);
        if (*env < 0)
            *env = 0;
    }
}

/* dub_env_step with body in quarter octaves (q4 = body·4) */
static void dub_env_step_q4(int32_t *env, uint16_t *snap, uint8_t q4)
{
    static const uint32_t frac_q16[4] = { 65536u, 55109u, 46341u, 38968u };

    if (*env <= 0)
        return;
    if (*snap > 0u) {
        *env -= (*env >> 9) + 2;
        if (*env < 0)
            *env = 0;
        (*snap)--;
    } else {
        uint32_t mul = (uint32_t)(((uint64_t)589824u * frac_q16[q4 & 3u]) >> (16u + (q4 >> 2)));

        *env -= dub_env_decr(*env, mul);
        if (*env < 0)
            *env = 0;
    }
}

static void dub_env_trig(int32_t *env, uint16_t *snap, uint8_t vel, uint8_t decay)
{
    *env = dub_vel_env_level(vel);
    *snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
}

static uint16_t dub_ch_attack_samples(uint8_t atk)
{
    if (atk == 0u)
        return 0u;
    return (uint16_t)(24u + ((uint32_t)atk * 2200u) / 127u);
}

#ifdef GENRE_HEAVY_PSYCH
#include "heavy_psych_env.h"
#endif

static void dub_drm_env_trig(int32_t *env, uint16_t *snap, int32_t *peak, uint16_t *atk_left,
                             uint8_t vel, uint8_t decay, uint8_t atk)
{
    int32_t pk = dub_vel_env_level(vel);

    *peak = pk;
    *atk_left = dub_ch_attack_samples(atk);
    if (*atk_left == 0u) {
        *env = pk;
        *snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
    } else {
        *env = pk >> 12;
        if (*env < 1)
            *env = 1;
        *snap = 0u;
    }
}

static void dub_drm_env_step(int32_t *env, uint16_t *snap, int32_t *peak, uint16_t *atk_left,
                             uint8_t body, uint8_t decay)
{
    if (*env <= 0)
        return;
    if (*atk_left > 0u) {
        int32_t pk = *peak;
        int32_t diff = pk - *env;
        int32_t step = diff / (int32_t)*atk_left;

        if (step < 1)
            step = 1;
        *env += step;
        (*atk_left)--;
        if (*atk_left == 0u || *env >= pk) {
            *env = pk;
            *atk_left = 0u;
            *snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
        }
        return;
    }
    dub_env_step_q4(env, snap, body);
}

#if !defined(GENRE_BASS)
/* Kick snap is 180+ samples of near-hold. On a hat that is a rectangular
 * noise burst, then the body cliff. Hats decay on hat_body from sample 0. */
static void dub_hat_env_step(dub_synth_t *s)
{
    if (s->hat_atk_left > 0u)
        dub_drm_env_step(&s->hat_env, &s->hat_snap, &s->hat_peak, &s->hat_atk_left,
                         s->hat_body, 0u);
    s->hat_snap = 0u;
    if (s->hat_atk_left == 0u && s->hat_env > 0) {
        /* 4947·2^(−q/4): q 0 ≈ 50 ms … q 17 ≈ 1 s to silence */
        static const uint16_t hat_mul[DUB_HAT_Q_MAX + 1] = {
            4947u, 4160u, 3498u, 2941u, 2474u, 2080u, 1749u, 1471u, 1237u,
            1040u, 875u, 735u, 618u, 520u, 437u, 368u, 309u, 260u
        };
        int32_t b = (int32_t)s->hat_body + (int32_t)s->hat_oh_trim;

        if (b < 0)
            b = 0;
        else if (b > DUB_HAT_Q_MAX)
            b = DUB_HAT_Q_MAX;

        s->hat_env -= dub_env_decr(s->hat_env, hat_mul[b]);
        if (s->hat_env < 0)
            s->hat_env = 0;
    }
}
#endif

/* hat_sig = hm · env · hat_lvl >> SH (per-genre trim, lvl 127 ≈ peak 9–14 k without clipping) */
#if defined(GENRE_JAZZ) || defined(GENRE_SOUL) || defined(GENRE_IDM) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
#define DUB_HAT_LVL_SH 22
#else
#define DUB_HAT_LVL_SH 21
#endif

/* Ride / glitch-hat metal: carrier phase-modulated by a real √2 modulator plus two inharmonic
 * partials (×1.73, ×2.23). fm 0..127 ≈ index 0..π rad. Uses hat_ph[0..3]. */
static inline int32_t dub_hat_fm_metal(dub_synth_t *s, uint32_t inc, int32_t fm, uint8_t bell, uint8_t acid)
{
    int32_t mod, car, p2, p3;

    s->hat_ph[0] += inc;
    s->hat_ph[1] += inc + (inc >> 1) - (inc >> 4) - (inc >> 5);
    s->hat_ph[2] += inc + (inc >> 1) + (inc >> 2) - (inc >> 6);
    s->hat_ph[3] += (inc << 1) + (inc >> 2) - (inc >> 6);
    mod = dub_sine_q64(s->hat_ph[1]) * fm;
    if (acid)
        car = dub_square(s->hat_ph[0] + ((uint32_t)mod << 9)) * 2;
    else
        car = dub_sine_q64(s->hat_ph[0] + ((uint32_t)mod << 9));
    p2 = dub_sine_q64(s->hat_ph[2] + ((uint32_t)mod << 8));
    p3 = dub_sine_q64(s->hat_ph[3] + ((uint32_t)mod << 7));
    if (bell)
        return (car >> 1) + (p2 >> 1) + (p3 >> 2);
    return (car >> 1) + (p2 >> 2) + (p3 >> 2);
}

/* Hat ride/glitch base note: ≥ 3.1 kHz (MIDI 103), +24 st from the kit tables, ≤ 6.6 kHz. */
static inline uint32_t dub_hat_fm_inc(int32_t base)
{
    base += 24;
    if (base < 103)
        base = 103;
    else if (base > 116)
        base = 116;
    return dub_note_inc((uint8_t)base);
}

/* KO-style 2× one-pole HP (pole 0.5 ≈ 5–7 kHz knee): strips the tonal/bell partials below the sizzle. */
static inline int32_t dub_hat_hp6k(dub_synth_t *s, int32_t x)
{
    s->hat_hp1 += (x - s->hat_hp1) >> 1;
    x -= s->hat_hp1;
    s->hat_hp2 += (x - s->hat_hp2) >> 1;
    return (x - s->hat_hp2) * 2;
}

/* Gentle 1-pole HP ~2 kHz for ride/glitch hats (removes DC/low wash, keeps body). */
static inline int32_t dub_hat_hp2k(dub_synth_t *s, int32_t x)
{
    s->hat_hp1 += (x - s->hat_hp1) >> 2;
    return x - s->hat_hp1;
}

static void dub_ch_env_trig(dub_synth_t *s, uint8_t vel, uint8_t decay, uint8_t atk)
{
#ifdef GENRE_HEAVY_PSYCH
    heavy_psych_ch_env_trig(s, vel, decay, atk);
#else
    int32_t peak;

    if (!s)
        return;
    peak = dub_vel_env_level(vel);
    s->ch_peak = peak;
    s->ch_atk_left = dub_ch_attack_samples(atk);
    if (s->ch_atk_left == 0u) {
        s->ch_env = peak;
        s->ch_snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
    } else {
        s->ch_env = peak >> 12;
        if (s->ch_env < 1)
            s->ch_env = 1;
        s->ch_snap = 0u;
    }
#endif
}

static void dub_ch_env_step(dub_synth_t *s, uint8_t decay)
{
#ifdef GENRE_HEAVY_PSYCH
    heavy_psych_ch_env_step(s, decay, s->voice_era);
#else
    if (!s || s->ch_env <= 0)
        return;
    if (s->ch_atk_left > 0u) {
        int32_t peak = s->ch_peak;
        int32_t diff = peak - s->ch_env;
        int32_t step = diff / (int32_t)s->ch_atk_left;

        if (step < 1)
            step = 1;
        s->ch_env += step;
        s->ch_atk_left--;
        if (s->ch_atk_left == 0u || s->ch_env >= peak) {
            s->ch_env = peak;
            s->ch_atk_left = 0u;
            s->ch_snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
        }
        return;
    }
    dub_env_step(&s->ch_env, &s->ch_snap, s->ch_body);
#endif
}

static void dub_sub_env_trig(dub_synth_t *s, uint8_t vel, uint8_t decay, uint8_t atk)
{
    int32_t peak;

    if (!s)
        return;
    peak = dub_vel_env_level(vel);
    s->sub_peak = peak;
#ifdef GENRE_HEAVY_PSYCH
    s->sub_atk_left = heavy_psych_ch_attack_samples(atk, s->voice_era);
#else
    s->sub_atk_left = dub_ch_attack_samples(atk);
#endif
    if (s->sub_atk_left == 0u) {
        s->sub_env = peak;
        s->sub_snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
    } else {
        s->sub_env = peak >> 12;
        if (s->sub_env < 1)
            s->sub_env = 1;
        s->sub_snap = 0u;
    }
}

static void dub_sub_env_step(dub_synth_t *s, uint8_t decay)
{
    if (!s || s->sub_env <= 0)
        return;
    if (s->sub_atk_left > 0u) {
        int32_t peak = s->sub_peak;
        int32_t diff = peak - s->sub_env;
        int32_t step = diff / (int32_t)s->sub_atk_left;

        if (step < 1)
            step = 1;
        s->sub_env += step;
        s->sub_atk_left--;
        if (s->sub_atk_left == 0u || s->sub_env >= peak) {
            s->sub_env = peak;
            s->sub_atk_left = 0u;
            s->sub_snap = (uint16_t)(180u + ((uint32_t)decay * 3u));
        }
        return;
    }
    dub_env_step(&s->sub_env, &s->sub_snap, s->sub_body);
}

static void dub_meter_peak(uint8_t *m, int32_t sig)
{
    int32_t ax = sig < 0 ? -sig : sig;
    int32_t v = ax >> 9;

    if (v > 255)
        v = 255;
    if ((uint8_t)v > *m)
        *m = (uint8_t)v;
}

static void dub_meter_decay(uint8_t *m)
{
    if (*m > 0u)
        *m = (uint8_t)(*m - (*m >> 6));
}

static void dub_svf_core(int32_t in, int32_t fc_q15, int32_t res_q15, int32_t *l1, int32_t *l2,
                         int32_t *low, int32_t *band, int32_t *high)
{
    int32_t g = fc_q15 >> 7;
    int32_t damp = 220 - (res_q15 >> 7);
    int32_t lowv = *l2;
    int32_t bandv = *l1;
    int32_t highv;

    if (g < 2)
        g = 2;
    if (g > 240)
        g = 240;
    if (damp < 16)
        damp = 16;
    highv = in - lowv - ((bandv * damp) >> 8);
    bandv += (highv * g) >> 8;
    lowv += (bandv * g) >> 8;
    if (bandv > 28000)
        bandv = 28000;
    else if (bandv < -28000)
        bandv = -28000;
    if (lowv > 28000)
        lowv = 28000;
    else if (lowv < -28000)
        lowv = -28000;
    *l1 = bandv;
    *l2 = lowv;
    *low = lowv;
    *band = bandv;
    *high = highv;
}

static void dub_svf_mode(int32_t in, int32_t fc_q15, int32_t *l1, int32_t *l2, uint8_t reso_mode, int32_t *out)
{
    int32_t low, band, high;
    /* Continuous 0..127 resonance sweep + resonant bandpass bite at high RESONA */
    int32_t res = (int32_t)reso_mode * 250;

    dub_svf_core(in, fc_q15, res, l1, l2, &low, &band, &high);
    *out = low + ((band * (int32_t)reso_mode) >> 6);
}

static int32_t dub_drm_fc(uint8_t cut)
{
    int32_t d = (int32_t)(cut ? cut : 64u) - 64;
    int32_t fc = 12000 + d * 160;

    if (fc < 350)
        return 350;
    if (fc > 28000)
        return 28000;
    return fc;
}

/* Siren tone filter: fixed Chamberlin LP (callers clamp cut ≥ 88 for a ~3–5 kHz tone). */
static void dub_siren_svf(int32_t *sig, uint8_t cut, int32_t *l1, int32_t *l2)
{
    uint8_t c = cut ? cut : 64u;

    if (c >= 62u && c <= 66u)
        return;
    dub_svf_mode(*sig, dub_drm_fc(c), l1, l2, 0, sig);
}

/* DJ drum filter, TPT SVF (k = 1.3), coefficients a1/a2/a3 in Q30.
 * cut 1..63: LP 80 Hz → 18 kHz (exp), 64: bypass, 65..127: HP 20 Hz → 8 kHz (exp). */
static const int32_t DUB_DRM_TPT[128][3] = {
    {0, 0, 0}, {1065810801, 6074158, 34617}, {1065089209, 6624167, 41198}, {1064302202, 7223535, 49027},
    {1063443898, 7876605, 58340}, {1062507890, 8588090, 69416}, {1061487211, 9363095, 82589}, {1060374277, 10207149, 98254},
    {1059160843, 11126233, 116878}, {1057837942, 12126818, 139019}, {1056395830, 13215891, 165336}, {1054823921, 14400995, 196610},
    {1053110717, 15690260, 233769}, {1051243740, 17092441, 277910}, {1049209450, 18616953, 330335}, {1046993167, 20273904, 392582},
    {1044578982, 22074130, 466472}, {1041949670, 24029229, 554157}, {1039086589, 26151581, 658179}, {1035969588, 28454382, 781540},
    {1032576897, 30951651, 927781}, {1028885027, 33658249, 1101073}, {1024868659, 36589872, 1306332}, {1020500538, 39763038, 1549337},
    {1015751362, 43195063, 1836880}, {1010589682, 46904007, 2176933}, {1004981795, 50908608, 2578839}, {998891655, 55228179, 3053536},
    {992280790, 59882482, 3613807}, {985108233, 64891555, 4274570}, {977330476, 70275499, 5053199}, {968901442, 76054217, 5969899},
    {959772494, 82247091, 7048112}, {949892477, 88872590, 8314980}, {939207802, 95947817, 9801860}, {927662589, 103487961, 11544885},
    {915198861, 111505672, 13585588}, {901756808, 120010336, 15971580}, {887275125, 129007244, 18757281}, {871691430, 138496670, 22004722},
    {854942767, 148472820, 25784391}, {836966186, 158922682, 30176152}, {817699416, 169824752, 35270230}, {797081603, 181147653, 41168272},
    {775054108, 192848621, 47984509}, {751561350, 204871867, 55847047}, {726551656, 217146786, 64899345}, {699978092, 229585979, 75301959},
    {671799232, 242083015, 87234673}, {641979836, 254509833, 100899205}, {610491404, 266713590, 116522753}, {577312610, 278512669, 134362744},
    {542429661, 289691399, 154713344}, {505836710, 299992794, 177914482}, {467536642, 309108219, 204364497}, {427542782, 316662333, 234538009},
    {385882644, 322190641, 269011346}, {342605714, 325105427, 308499055}, {297799054, 324643052, 353906802}, {251617934, 319780795, 406408857},
    {204345613, 309102595, 467562837}, {156511061, 290576615, 539481163}, {109125369, 261176027, 625087620}, {64170925, 216213141, 728493816},
    {0, 0, 0}, {1071754558, 1526993, 2176}, {1071553098, 1681605, 2639}, {1071331234, 1851838, 3201},
    {1071086903, 2039260, 3883}, {1070817835, 2245600, 4709}, {1070521531, 2472755, 5712}, {1070195242, 2722811, 6927},
    {1069835942, 2998062, 8402}, {1069440302, 3301026, 10189}, {1069004658, 3634469, 12357}, {1068524982, 4001429, 14985},
    {1067996842, 4405239, 18171}, {1067415365, 4849559, 22033}, {1066775191, 5338399, 26715}, {1066070429, 5876159, 32389},
    {1065294603, 6467657, 39267}, {1064440598, 7118173, 47601}, {1063500596, 7833483, 57700}, {1062466009, 8619908, 69934},
    {1061327410, 9484353, 84755}, {1060074449, 10434361, 102706}, {1058695769, 11478163, 124444}, {1057178915, 12624728, 150763},
    {1055510230, 13883824, 182623}, {1053674751, 15266071, 221181}, {1051656089, 16783001, 267834}, {1049436304, 18447118, 324266},
    {1046995777, 20271955, 392506}, {1044313060, 22272127, 474999}, {1041364735, 24463387, 574686}, {1038125247, 26862673, 695102},
    {1034566747, 29488138, 840497}, {1030658914, 32359186, 1015968}, {1026368782, 35496472, 1227628}, {1021660561, 38921898, 1482796},
    {1016495457, 42658572, 1790223}, {1010831495, 46730744, 2160362}, {1004623352, 51163689, 2605676}, {997822202, 55983556, 3140999},
    {990375579, 61217144, 3783957}, {982227269, 66891618, 4555451}, {973317236, 73034135, 5480212}, {963581600, 79671378, 6587432},
    {952952673, 86828974, 7911485}, {941359064, 94530790, 9492733}, {928725881, 102798084, 11378434}, {914975029, 111648499, 13623746},
    {900025636, 121094885, 16292837}, {883794603, 131143938, 19460101}, {866197311, 141794642, 23211479}, {847148473, 153036509, 27645889},
    {826563141, 164847616, 32876782}, {804357864, 177192421, 39033812}, {780451963, 190019370, 46264681}, {754768897, 203258265, 54737182},
    {727237677, 216817377, 64641556}, {697794243, 230580224, 76193291}, {666382750, 244401898, 89636606}, {632956685, 258104730, 105248990},
    {597479753, 271472884, 123347321}, {559926566, 284245284, 144296389}, {520283238, 296105824, 168521015}, {478548301, 306669215, 196523543},
};

static void dub_drm_svf(int32_t *sig, uint8_t cut, int32_t *l1, int32_t *l2)
{
    uint8_t c = cut ? cut : 64u;
    const int32_t *a;
    int64_t x, v1, v2, v3;

    if (c == 64u || c > 127u) {
        *l1 = 0;
        *l2 = 0;
        return;
    }
    a = DUB_DRM_TPT[c];
    x = (int64_t)*sig * 256;
    v3 = x - *l2;
    v1 = ((int64_t)a[0] * *l1 + (int64_t)a[1] * v3) >> 30;
    v2 = *l2 + (((int64_t)a[1] * *l1 + (int64_t)a[2] * v3) >> 30);
    *l1 = (int32_t)(2 * v1 - *l1);
    *l2 = (int32_t)(2 * v2 - *l2);
    if (c < 64u)
        *sig = (int32_t)(v2 >> 8);
    else
        *sig = (int32_t)((x - ((v1 * 21299) >> 14) - v2) >> 8);
}

static int32_t dub_lfo_val(uint8_t wave, int32_t ph, int32_t *sh)
{
    uint32_t u = (uint32_t)ph;
    int32_t v;

    switch (wave) {
    case DUB_LFO_TRI: {
        uint32_t p = u >> 16;
        v = (int32_t)(p & 65535u);
        if (p & 65536u)
            v = 65535 - v;
        return (v >> 1) - 16384;
    }
    case DUB_LFO_SQR:
        return (u & 0x80000000u) ? 16384 : -16384;
    case DUB_LFO_SH:
        if ((u >> 24) != (uint32_t)*sh) {
            *sh = (int32_t)(u >> 24);
            return (int32_t)((u >> 8) & 32767u) - 16384;
        }
        return *sh << 7;
    case DUB_LFO_STEP:
        return (int32_t)(((u >> 28) & 15u) * 2184 - 16384);
    default:
        return dub_sine_q64(u) >> 1;
    }
}

static int32_t dub_lfo_sample(uint8_t wave, uint8_t skew, uint8_t algo, uint8_t ratio, uint8_t index,
                              uint8_t fb, uint8_t fold, uint8_t phase, uint8_t smooth, uint32_t ph,
                              uint32_t cycle, uint32_t slot, int32_t *sh, int32_t *slew)
{
    int32_t q;
    uint32_t ph0 = ph + ((uint32_t)phase << 25);

    (void)sh;
    q = fm_lfo_eval_full_q15(ph0, cycle, slot, wave, skew, algo, ratio, index, fb, fold) >> 1;
    if (smooth > 0u && slew) {
        q = fm_lfo_slew_step(*slew, q, smooth);
        *slew = q;
    }
    return q;
}

static int32_t dub_lfo_depth(int32_t raw, uint8_t depth)
{
    return (raw * (int32_t)depth) >> 7;
}

static uint32_t dub_lfo_inc(uint8_t rate, uint8_t wob_force, uint8_t bpm)
{
    uint32_t r = rate;

    if (wob_force > 0u && wob_force <= 4u) {
        /* cycles per beat ×4 for periods 1/8, 1/4, 1/2T, 1/2 — at 120 BPM = 4 / 2 / 1.5 / 1 Hz */
        static const uint32_t cpb_x4[4] = {8u, 4u, 3u, 2u};
        uint32_t b = bpm < 40u ? 120u : (bpm > 180u ? 180u : (uint32_t)bpm);

        return b * cpb_x4[wob_force - 1u] * 97391u / 240u;
    }
    if (r < 1u)
        r = 1u;
    return 200u + r * r * 12u;
}

static int32_t dub_fc_from_cutoff(uint8_t cutoff, int32_t env, uint8_t env_filt, int32_t lfo_mod)
{
    /* CUTOFF sets base filter opening (300..26000); ENVLPE adds dynamic envelope wah sweep on top */
    int32_t base = 300 + (int32_t)cutoff * (int32_t)cutoff + (int32_t)cutoff * 50;
    int32_t env_sweep = ((env >> 4) * (int32_t)env_filt) >> 3;
    int32_t fc = base + env_sweep + lfo_mod;

    if (fc < 260)
        return 260;
    if (fc > 29000)
        return 29000;
    return fc;
}

static uint8_t dub_chord_interval(uint8_t voicing, uint8_t ix)
{
#if defined(GENRE_HEAVY_PSYCH)
    static const int8_t tab[VOICE_N][4] = {
        {0, 7, 12, 19}, {0, 4, 7, 11}, {0, 2, 7, 10}, {0, 12, 19, 24}, {0, 3, 7, 10}};
#elif defined(GENRE_HOUSE)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 10}, {0, 4, 7, 14}, {0, 4, 7, 9}};
#elif defined(GENRE_FUNK)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 10}, {0, 3, 7, 14}, {0, 4, 7, 14}, {0, 4, 7, 17}, {0, 5, 7, 12}};
#elif defined(GENRE_HIPHOP)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 10}, {0, 4, 7, 14}, {0, 4, 7, 9}};
#elif defined(GENRE_POP)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 10}};
#elif defined(GENRE_JAZZ)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 4, 7, 14}, {0, 3, 7, 14}, {0, 5, 7, 12}, {0, 4, 9, 14},
        {0, 5, 10, 15}, {0, 4, 7, 14}, {0, 4, 7, 9}, {0, 4, 7, 21}, {0, 3, 10, 17}, {0, 4, 7, 17}};
#elif defined(GENRE_SOUL)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 12}, {0, 3, 7, 10}, {0, 4, 7, 10}, {0, 4, 10, 14}, {0, 4, 7, 9}, {0, 4, 9, 10},
        {0, 5, 7, 10}, {0, 3, 7, 14}, {0, 4, 7, 14}, {0, 4, 7, 11}, {0, 4, 7, 21}, {0, 3, 10, 17}};
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    static const int8_t tab[VOICE_N][4] = {
        {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 4, 7, 14}, {0, 3, 7, 14}, {0, 5, 7, 12}, {0, 4, 9, 14},
        {0, 5, 10, 15}, {0, 4, 7, 14}, {0, 4, 7, 9}, {0, 4, 7, 21}, {0, 3, 10, 17}, {0, 4, 7, 17}};
#else
    static const int8_t tab[VOICE_N][4] = {
        {0, 3, 7, 10}, {0, 3, 10, 14}, {0, 5, 7, 10}, {0, 5, 10, 15}, {0, 7, 12, 12}};
#endif

    if (voicing >= VOICE_N)
        voicing = 0u;
    return (uint8_t)tab[voicing][ix];
}

#define DUB_DLY_DIV_N 17u

static uint8_t dub_dly_div_ix(uint8_t dly_time)
{
    if (dly_time == 0u)
        return 0u;
    return (uint8_t)(1u + (((uint32_t)(dly_time - 1u) * (DUB_DLY_DIV_N - 2u) + 63u) / 126u));
}

static uint32_t dub_delay_samples(const dub_params_t *p, int8_t tape_oct, uint8_t *out_rate_div)
{
    uint32_t bpm = p->bpm < 40u ? 72u : (p->bpm > 180u ? 180u : (uint32_t)p->bpm);
    uint32_t d = fm_time_samples(p->dly_time, bpm, DUB_FS);
    uint8_t rdiv = 1u;
    int32_t oct = (int32_t)tape_oct;

    if (d == 0u)
        d = (60u * DUB_FS) / bpm;

    while (oct > 0) {
        d >>= 1;
        oct--;
    }
    while (oct < 0) {
        d <<= 1;
        oct++;
    }
    /* Automatic tape speed divider (1x, 2x, 4x, 8x, 16x) so 1/1, 2/1, 3/1 and 4/1 fit inside 32768-sample pool */
    while (d > (DUB_DLY_LEN - 128u) && rdiv < 16u) {
        d >>= 1;
        rdiv <<= 1;
    }
    if (d < 8u)
        d = 8u;
    if (d > (DUB_DLY_LEN - 64u))
        d = DUB_DLY_LEN - 64u;
    if (out_rate_div)
        *out_rate_div = rdiv;
    return d;
}

static int32_t dub_dly_read_interp(uint32_t wpos, uint32_t delay, int32_t wow_smp)
{
    int64_t rp = (int64_t)wpos + (int64_t)DUB_DLY_LEN - (int64_t)delay - (int64_t)wow_smp;
    int32_t ip;
    int32_t frac;

    while (rp < 0)
        rp += DUB_DLY_LEN;
    while (rp >= (int64_t)DUB_DLY_LEN)
        rp -= DUB_DLY_LEN;
    ip = (int32_t)rp;
    frac = (int32_t)((rp - ip) * 256);
    {
        int32_t i0 = ip;
        int32_t i1 = (i0 + 1) % (int32_t)DUB_DLY_LEN;
        int32_t s0 = dub_dly_pool[i0];
        int32_t s1 = dub_dly_pool[i1];

        return s0 + (((s1 - s0) * frac) >> 8);
    }
}

static int32_t dub_ap_pass(int16_t *buf, uint32_t len, uint16_t *pos, int32_t in)
{
    uint32_t p;
    int32_t ap;
    int32_t v;

    if (len == 0u)
        return in;
    p = (uint32_t)*pos % len;
    ap = buf[p];
    v = in + ((ap * 180) >> 8);
    if (v > 32767)
        v = 32767;
    else if (v < -32767)
        v = -32767;
    buf[p] = (int16_t)(v >> 1);
    *pos = (uint16_t)((p + 1u) % len);
    return ap - v;
}

static int32_t dub_tank_step(int16_t *buf, uint32_t len, uint16_t *pos, int32_t in, int32_t *lp, int32_t damp)
{
    uint32_t p;
    int32_t d;
    int32_t out;

    if (len == 0u)
        return in;
    p = (uint32_t)*pos % len;
    d = buf[p];
    *lp += ((d - *lp) * damp) >> 8;
    out = *lp;
    {
        int32_t nw = in + (out >> 1);

        if (nw > 32767)
            nw = 32767;
        else if (nw < -32767)
            nw = -32767;
        buf[p] = (int16_t)nw;
    }
    *pos = (uint16_t)((p + 1u) % len);
    return out;
}

void dub_synth_init(dub_synth_t *s)
{
    uint32_t i;

    if (!s)
        return;
    for (i = 0; i < DUB_DLY_LEN; i++)
        dub_dly_pool[i] = 0;
    for (i = 0; i < DUB_REV_LEN; i++)
        dub_rev_pool[i] = 0;
    for (i = 0; i < 4u; i++) {
        s->rev_ap_pos[i] = 0u;
        s->rev_t_pos[i] = 0u;
        s->rev_t_lp[i] = 0;
        s->meter[i] = 0u;
    }
    for (i = 0; i < 3u; i++)
        s->rev_ch_pos[i] = 0u;
    s->sub_ph = 0u;
    s->sub_ph2 = 0u;
    s->sub_ph3 = 0u;
    s->sub_inc = 0u;
    s->sub_tgt_inc = 0u;
    s->sub_env = 0;
    s->sub_snap = 0u;
    s->sub_peak = 0;
    s->sub_atk_left = 0u;
    s->sub_svf_b1 = 0;
    s->sub_svf_b2 = 0;
    s->sub_fm_fb = 0;
#if defined(GENRE_POP)
    s->pop_ch_fb = 0;
    s->pop_lead_fb = 0;
#endif
    s->sub_on = 0u;
    s->ch_env = 0;
    s->kick_env = 0;
    s->rim_env = 0;
    s->rim_ph = 0u;
    s->kick_st_tune = 0;
    s->kick_fresh = 0u;
    s->rim_st_tune = 0;
    s->hat_st_tune = 0;
    s->siren_st_tune = 0;
    s->sub_tune_snap = 0;
    s->ch_tune_snap = 0;
    s->hat_env = 0;
    s->hat_hp1 = 0;
    s->hat_hp2 = 0;
    s->ks_fresh = 0u;
    s->siren_env = 0;
    s->siren_gate = 0u;
    s->siren_lead_rel = 0u;
    s->siren_note_age = 0u;
    s->siren_trig_count = 0u;
    s->siren_pluck_arm = 0u;
    s->siren_prev_tune = 0;
    s->siren_lead_inc = 0u;
    s->siren_lead_tgt_inc = 0u;
    s->siren_lead_glide_on = 0u;
    s->siren_on = 0u;
    s->siren_vel = 100u;
    s->dly_w = 0u;
    s->dly_half_toggle = 0u;
    s->dly_hp = 0;
    s->dly_lp = 0;
    s->dly_len_sm = 0u;
    s->dly_lf_sm = 64;
    s->dly_loop_dc = 0;
    s->sub_send_hp = 0;
    s->wow_ph = 0u;
    s->pump_env = 0;
    s->crash_left = 0u;
    s->crash_env = 0;
    for (i = 0; i < 3u; i++) {
        s->flam_left[i] = 0u;
        s->flam_vel[i] = 0u;
    }
    s->lfo_ph[0] = 0;
    s->lfo_ph[1] = 0;
    s->lfo_cycle[0] = 0u;
    s->lfo_cycle[1] = 0u;
    s->lfo_sh[0] = 0;
    s->lfo_sh[1] = 0;
    s->noise = 1u;
    s->iso_lp = 0;
    s->iso_hp = 0;
    s->iso_lp_r = 0;
    s->iso_hp_r = 0;
    s->wg_w = 0u;
    for (i = 0; i < DUB_WG_LEN; i++)
        s->wg[i] = 0;
#if defined(GENRE_BASS)
    bass_dahdsr_init(&s->bass_env);
    s->bass_env_curve = BASS_DAHDSR_CURVE_LIN;
    s->bass_amp_q15 = 0;
    s->bass_slide = 0u;
    s->bass_glide_tc = 56u;
    s->bass_glide_note = 0u;
    s->bass_glide_q16 = 0;
    s->sub_on = 0u;
    s->sub_env = 0;
#endif
}

void dub_trig_sub(dub_synth_t *s, uint8_t note, uint8_t vel, uint8_t atk)
{
    if (!s)
        return;
    s->sub_note = note;
    s->sub_on = 1u;
    s->sub_ph = 0u;
    s->sub_ph2 = 0u;
    s->sub_ph3 = 0u;
    s->sub_fm_fb = 0;
#if defined(GENRE_POP)
    s->pop_ch_fb = 0;
    s->pop_lead_fb = 0;
#endif
    s->sub_tgt_inc = dub_note_inc(note);
    /* Start 808 / slap pitch higher (+7 semitones) so 808 pitch-drop actually glides down */
    s->sub_inc = s->sub_tgt_inc + (s->sub_tgt_inc >> 1);
#if defined(GENRE_BASS)
    s->sub_env = 0;
    s->sub_peak = 0;
    s->sub_atk_left = 0u;
    s->sub_snap = 0u;
    s->sub_body = 0u;
#else
    dub_sub_env_trig(s, vel, 60u, atk);
    s->sub_body = dub_env_body(60u);
#endif
    s->wg_w = 0u;
    {
        uint32_t j;
        for (j = 0; j < DUB_WG_LEN; j++) {
            int32_t n = dub_noise(&s->noise);
            int32_t body = dub_sine_q64(j << 26) + (dub_sine_q64(j << 27) >> 1);
            s->wg[j] = (int16_t)((body >> 1) + (n >> 2));
        }
    }
}

void dub_trig_chord(dub_synth_t *s, uint8_t root, uint8_t vel, uint8_t atk, uint8_t glide, uint8_t voicing)
{
    uint32_t i;
    uint8_t vo = voicing % VOICE_N;

    if (!s)
        return;
    s->ch_root_tgt = root;
#ifdef GENRE_HEAVY_PSYCH
    if (glide == 0u) {
        if (s->ch_env <= 0 || root != s->ch_root) {
            s->ch_root = root;
            for (i = 0; i < DUB_CHORD_N; i++) {
                s->ch_ph[i] = 0u;
                s->ch_inc[i] = 0u;
                s->ch_tgt_inc[i] = 0u;
            }
        }
    } else {
        /* Snap the root so the render target is the new chord; ch_inc slews from the previous pitch. */
        s->ch_root = root;
        for (i = 0; i < DUB_CHORD_N; i++) {
            uint8_t n = dub_clamp_midi((int32_t)root + (int32_t)dub_chord_interval(vo, (uint8_t)i));

            s->ch_tgt_inc[i] = dub_note_inc(n);
            if (s->ch_inc[i] == 0u)
                s->ch_inc[i] = s->ch_tgt_inc[i];
        }
    }
    heavy_psych_ch_env_legato(s, vel, 60u, atk, root);
#elif defined(GENRE_JAZZ)
    if (glide == 0u) {
        if (s->ch_env <= 0 || root != s->ch_root) {
            s->ch_root = root;
            for (i = 0; i < DUB_CHORD_N; i++) {
                s->ch_ph[i] = 0u;
                s->ch_inc[i] = 0u;
                s->ch_tgt_inc[i] = 0u;
            }
        }
    } else {
        s->ch_root = root;
        for (i = 0; i < DUB_CHORD_N; i++) {
            uint8_t n = dub_clamp_midi((int32_t)root + (int32_t)dub_chord_interval(vo, (uint8_t)i));

            s->ch_tgt_inc[i] = dub_note_inc(n);
            if (s->ch_inc[i] == 0u)
                s->ch_inc[i] = s->ch_tgt_inc[i];
        }
    }
    dub_ch_env_trig(s, vel, 60u, atk);
    s->ch_body = dub_env_body(60u);
#elif defined(GENRE_SOUL)
    if (glide == 0u) {
        if (s->ch_env <= 0 || root != s->ch_root) {
            s->ch_root = root;
            for (i = 0; i < DUB_CHORD_N; i++) {
                s->ch_ph[i] = 0u;
                s->ch_inc[i] = 0u;
                s->ch_tgt_inc[i] = 0u;
            }
        }
    } else {
        s->ch_root = root;
        for (i = 0; i < DUB_CHORD_N; i++) {
            uint8_t n = dub_clamp_midi((int32_t)root + (int32_t)dub_chord_interval(vo, (uint8_t)i));

            s->ch_tgt_inc[i] = dub_note_inc(n);
            if (s->ch_inc[i] == 0u)
                s->ch_inc[i] = s->ch_tgt_inc[i];
        }
    }
    dub_ch_env_trig(s, vel, 60u, atk);
    s->ch_body = dub_env_body(60u);
#else
    if (glide == 0u) {
        s->ch_root = root;
        for (i = 0; i < DUB_CHORD_N; i++) {
            s->ch_ph[i] = 0u;
            s->ch_inc[i] = 0u;
            s->ch_tgt_inc[i] = 0u;
        }
    } else {
        for (i = 0; i < DUB_CHORD_N; i++) {
            uint8_t n = dub_clamp_midi((int32_t)root + (int32_t)dub_chord_interval(vo, (uint8_t)i));

            s->ch_tgt_inc[i] = dub_note_inc(n);
            if (s->ch_inc[i] == 0u)
                s->ch_inc[i] = s->ch_tgt_inc[i];
        }
        if (s->ch_env <= 0)
            s->ch_root = root;
    }
    dub_ch_env_trig(s, vel, 60u, atk);
    s->ch_body = dub_env_body(60u);
#endif
}

void dub_trig_kick(dub_synth_t *s, uint8_t vel, int8_t tune, uint8_t atk)
{
    uint8_t note;

    if (!s)
        return;
    s->kick_st_tune = tune;
    s->kick_ph = 0u;
    note = dub_clamp_midi(36 + (int32_t)tune);
    s->kick_tgt_inc = dub_note_inc(note);
    s->kick_inc = s->kick_tgt_inc + (s->kick_tgt_inc >> 1);
    s->kick_fresh = 1u;
    dub_drm_env_trig(&s->kick_env, &s->kick_snap, &s->kick_peak, &s->kick_atk_left, vel, 40u, atk);
    s->kick_body = dub_drm_env_body_q4(40u);
    s->pump_env = 32767;
}

#if defined(GENRE_SNOWCRASH)
static inline void snowcrash_ks_excite(uint8_t voice, int32_t semi);
static inline int32_t snowcrash_ks_step(uint8_t voice);
#endif
void dub_trig_rim(dub_synth_t *s, uint8_t vel, int8_t tune, uint8_t atk)
{
    if (!s)
        return;
    s->rim_st_tune = tune;
    s->rim_ph = 0u;
#if defined(GENRE_SNOWCRASH)
    s->ks_fresh |= 1u;
#endif
    dub_drm_env_trig(&s->rim_env, &s->rim_snap, &s->rim_peak, &s->rim_atk_left, vel, 20u, atk);
    s->rim_body = dub_drm_env_body_q4(20u);
}

void dub_trig_hat(dub_synth_t *s, uint8_t vel, int8_t tune, uint8_t atk)
{
    uint32_t i;

    if (!s)
        return;
    s->hat_st_tune = tune;
    for (i = 0; i < 4u; i++)
        s->hat_ph[i] = (uint32_t)(i * 0x11111111u);
#if defined(GENRE_SNOWCRASH)
    s->ks_fresh |= 2u;
#endif
    dub_drm_env_trig(&s->hat_env, &s->hat_snap, &s->hat_peak, &s->hat_atk_left, vel, 16u, atk);
    s->hat_snap = 0u;
    s->hat_body = dub_hat_env_body(16u);
}

static inline void dub_hat_apply_lane_gate(dub_synth_t *s, int8_t lane_gate)
{
    int32_t body;

    if (!s || lane_gate == 0)
        return;
    body = (int32_t)s->hat_body + lane_gate;
    if (body < 4)
        body = 4;
    if (body > 120)
        body = 120;
    s->hat_body = (uint8_t)body;
    if (lane_gate < 0)
        s->hat_env = (s->hat_env * (127 + lane_gate)) / 127;
}

void dub_schedule_flam(dub_synth_t *s, uint8_t drm_voice, uint8_t vel)
{
    /* ~22 ms flam retrigger (970 samples at 44.1 kHz, matching KO RYTM) */
    if (!s || drm_voice >= 3u)
        return;
    s->flam_left[drm_voice] = (uint16_t)((22u * DUB_FS) / 1000u);
    s->flam_vel[drm_voice] = vel;
}

void dub_spring_crash(dub_synth_t *s)
{
    if (!s)
        return;
    s->crash_left = 200u;
    s->crash_env = 28000;
}

#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_HOUSE) || defined(GENRE_JAZZ) \
    || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_IDM) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_SOUL)
#include "dub_fm.h"
#endif
#ifdef GENRE_HEAVY_PSYCH
#include "heavy_psych_dsp.h"
#elif defined(GENRE_HOUSE)
#include "house_dsp.h"
#elif defined(GENRE_JAZZ)
#include "jazz_dsp.h"
#elif defined(GENRE_SOUL)
#include "soul_dsp.h"
#elif defined(GENRE_SNOWCRASH)
#include "snowcrash_dsp.h"
#elif defined(GENRE_IDM)
#include "idm_dsp.h"
#elif defined(GENRE_HIPHOP)
#include "hiphop_dsp.h"
#elif defined(GENRE_POP)
#include "pop_dsp.h"
#elif defined(GENRE_FUNK)
#include "funk_dsp.h"
#elif defined(GENRE_TECHNO)
#include "techno_dsp.h"
#elif defined(GENRE_TRIPHOP)
#include "triphop_dsp.h"
#elif defined(GENRE_BASS)
#include "bass_dsp.h"
#endif

#if !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_HOUSE) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)

static inline uint32_t dub_siren_bent_inc(const dub_synth_t *s, uint32_t inc)
{
    uint32_t n = s->siren_note_age;

    uint32_t b_smp = (DUB_FS * 75u) / 1000u;

    if (s->siren_hp_bend > 0u && s->siren_hp_bend_semi != 0 && n < b_smp) {
        int32_t semi = (int32_t)s->siren_hp_bend_semi;
        int32_t depth = (int32_t)s->siren_hp_bend;
        int32_t off_q8 = (semi * 256 * depth) / 127;

        off_q8 = (off_q8 * (int32_t)(b_smp - n)) / (int32_t)b_smp; /* |off_q8| ≤ 2^15, b_smp < 2^12 */
        if (off_q8 != 0)
            inc = (uint32_t)(((uint64_t)inc * dub_semi_ratio_q16(-off_q8)) >> 16);
    }
    return inc;
}

static inline uint16_t dub_siren_atk_samples(uint8_t atk)
{
    uint16_t a;

    if (atk)
        a = (uint16_t)(22u + ((uint32_t)atk * 28u) / 127u);
    else
        a = 44u;
    if (a > 110u)
        a = 110u;
    return a;
}

static inline void dub_siren_note_gate(uint8_t trig_cnt, int8_t lane_gate, uint16_t *hold_out,
                                       uint16_t *rel_out, uint8_t len_plock_on, uint8_t len_ix,
                                       uint32_t samples_per_step)
{
    uint32_t total, hold;
    int32_t g = (int32_t)lane_gate;

    (void)trig_cnt;
    if (len_plock_on != 0u && samples_per_step > 0u) {
        total = dub_siren_gate_len_samples(samples_per_step, len_ix);
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

static inline void dub_siren_env_step(dub_synth_t *s, const dub_params_t *p)
{
    if ((s->siren_atk_left > 0u || s->siren_gate > 0u || s->siren_lead_rel > 0u || s->siren_env > 0)
        && s->siren_note_age < 65535u)
        s->siren_note_age++;

    if (s->siren_atk_left > 0u) {
        int32_t pk = s->siren_peak;
        int32_t diff = pk - s->siren_env;
        int32_t step = diff;

        if (step > pk >> 3)
            step = pk >> 3;
        if (step < 1)
            step = 1;
        s->siren_env += step;
        s->siren_atk_left--;
        if (s->siren_atk_left == 0u || s->siren_env >= pk)
            s->siren_env = pk;
        return;
    }
    if (s->siren_on) {
        int32_t tgt = dub_vel_env_level(s->siren_vel ? s->siren_vel : 100u);

        if (tgt > 28000)
            tgt = 28000;
        s->siren_env += (tgt - s->siren_env) >> 7;
    } else if (s->siren_gate > 0u) {
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

void dub_siren_set(dub_synth_t *s, uint8_t on)
{
    if (!s)
        return;
    s->siren_on = on ? 1u : 0u;
    if (on) {
        int32_t tgt = dub_vel_env_level(s->siren_vel ? s->siren_vel : 100u);

        if (tgt > 28000)
            tgt = 28000;
        s->siren_gate = 12000u;
        s->siren_env += (tgt - s->siren_env) >> 3;
    }
}

void dub_siren_trig(dub_synth_t *s, uint8_t vel, int8_t tune, int8_t lane_gate, uint8_t atk, uint8_t era)
{
    int32_t pk;

    if (!s)
        return;
    s->siren_vel = vel ? vel : 1u;
    s->siren_on = 0u;
    pk = dub_vel_env_level(s->siren_vel);
    if (pk > 28000)
        pk = 28000;
    s->siren_peak = pk;
#if defined(GENRE_HEAVY_PSYCH)
    {
        uint32_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        heavy_psych_lead_note_gate_trax(era, s->voice_plate, s->siren_trig_count, lane_gate, &hold, &rel,
                                        s->siren_hp_len_on, s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (heavy_psych_lead_uses_pluck(era))
            s->siren_pluck_arm = 1u;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            s->siren_lead_glide_on = (prev_tune != tune) ? 1u : 0u;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = heavy_psych_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else {
                if (heavy_psych_lead_uses_pluck(era))
                    s->siren_note_age = 0u;
                else if (s->siren_env < pk)
                    s->siren_env = pk;
            }
            return;
        }
        s->siren_lead_glide_on = 0u;
        s->siren_lead_inc = 0u;
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = heavy_psych_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_lfo_ph = 0u;
        s->stab_ph[0] = 0u;
        s->stab_ph[1] = 0u;
        s->lead_amp_hp_lp = 0;
        s->lead_amp_hp_bp = 0;
        s->lead_amp_mid_lp = 0;
        s->lead_amp_mid_bp = 0;
        s->lead_cab_lp = 0;
        s->lead_cab_bp = 0;
        s->lead_cab_body_lp = 0;
        s->lead_cab_body_bp = 0;
        s->lead_amp_treb_lp = 0;
        s->lead_amp_treb_bp = 0;
    }
#elif defined(GENRE_JAZZ)
    {
        uint32_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        jazz_lead_note_gate_trax(era, s->voice_plate, s->siren_trig_count, lane_gate, &hold, &rel,
                                 s->siren_hp_len_on, s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            s->siren_lead_glide_on = (prev_tune != tune) ? 1u : 0u;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = jazz_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk) {
                s->siren_env = pk;
            }
            return;
        }
        s->siren_lead_glide_on = 0u;
        s->siren_lead_inc = 0u;
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = jazz_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_lfo_ph = 0u;
        s->stab_ph[0] = 0u;
        s->stab_ph[1] = 0u;
    }
#elif defined(GENRE_SOUL)
    {
        uint32_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        soul_lead_note_gate_side(era, s->voice_plate, s->siren_trig_count, lane_gate, &hold, &rel,
                                 s->siren_hp_len_on, s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            s->siren_lead_glide_on = (prev_tune != tune) ? 1u : 0u;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = soul_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk) {
                s->siren_env = pk;
            }
            return;
        }
        s->siren_lead_glide_on = 0u;
        s->siren_lead_inc = 0u;
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = soul_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_lfo_ph = 0u;
        s->stab_ph[0] = 0u;
        s->stab_ph[1] = 0u;
    }
#elif defined(GENRE_HIPHOP)
    {
        uint16_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        hiphop_lead_note_gate_beat(era, s->voice_plate, s->siren_trig_count, lane_gate, &hold, &rel,
                                 s->siren_hp_len_on, s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = hiphop_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk) {
                s->siren_env = pk;
            }
            return;
        }
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = hiphop_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_lfo_ph = 0u;
        s->stab_ph[0] = 0u;
        s->stab_ph[1] = 0u;
    }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
    {
        uint16_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        idm_lead_note_gate_piece(era, s->voice_plate, s->siren_trig_count, lane_gate, &hold, &rel,
                                 s->siren_hp_len_on, s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = idm_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk) {
                s->siren_env = pk;
            }
            return;
        }
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = idm_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_lfo_ph = 0u;
        s->stab_ph[0] = 0u;
        s->stab_ph[1] = 0u;
    }
#elif defined(GENRE_HOUSE)
    {
        uint16_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;
        uint32_t j;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        house_stab_note_gate(s->siren_trig_count, lane_gate, &hold, &rel, s->siren_hp_len_on,
                             s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = house_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk)
                s->siren_env = pk;
            return;
        }
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = house_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_ph = 0u;
        s->siren_lfo_ph = 0u;
        for (j = 0; j < 8u; j++)
            s->stab_ph[j] = 0u;
    }
#elif defined(GENRE_FUNK)
    {
        uint16_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;
        uint32_t j;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        funk_stab_note_gate(s->siren_trig_count, lane_gate, &hold, &rel, s->siren_hp_len_on,
                            s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = funk_siren_atk_samples(era, atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk)
                s->siren_env = pk;
            return;
        }
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = funk_siren_atk_samples(era, atk);
        s->siren_env = 0;
        s->siren_ph = 0u;
        s->siren_lfo_ph = 0u;
        for (j = 0; j < 8u; j++)
            s->stab_ph[j] = 0u;
    }
#elif defined(GENRE_POP)
    {
        uint16_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        if (s->pop_lead_gate_hold)
            hold = s->pop_lead_gate_hold;
        else
            pop_siren_note_gate(s->siren_trig_count, lane_gate, &hold, &rel, s->siren_hp_len_on,
                                s->siren_hp_len_ix, s->siren_hp_spp, era);
        if (s->pop_lead_gate_rel)
            rel = s->pop_lead_gate_rel;
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            if (prev_tune != tune) {
                uint8_t tex = pop_lead_texture(era, pop_lead_variant(s->voice_plate));

                s->siren_note_age = 0u;
                s->siren_atk_left = pop_siren_atk_samples_tex(tex, atk);
                s->pop_lead_tex = tex;
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk)
                s->siren_env = pk;
            return;
        }
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = pop_siren_atk_samples_tex(s->pop_lead_tex ? s->pop_lead_tex :
                                                      pop_lead_texture(era, pop_lead_variant(s->voice_plate)), atk);
        s->siren_env = 0;
        s->siren_ph = 0u;
        s->siren_lfo_ph = 0u;
        s->pop_lead_det_ph[0] = 0u;
        s->pop_lead_det_ph[1] = 0u;
        s->pop_lead_inc = 0u;
        s->pop_lead_tgt_inc = 0u;
        s->pop_lead_fenv = 0;
        s->pop_lead_lp = 0;
        s->pop_lead_bp = 0;
        s->pop_lead_lp2 = 0;
        s->pop_lead_bp2 = 0;
        s->pop_lead_note = 0u;
        s->pop_lead_chorus = 0u;
        s->pop_lead_ens = 0u;
        s->pop_lead_note = 0u;
        s->pop_lead_lp2 = 0;
        s->pop_lead_bp2 = 0;
        s->pop_lead_f1_lp = 0;
        s->pop_lead_f1_bp = 0;
        s->pop_lead_f2_lp = 0;
        s->pop_lead_f2_bp = 0;
        s->pop_lead_oct_lfo_semi = 0;
        {
            uint8_t tex_arm = s->pop_lead_tex;
            uint8_t amp_arm = s->pop_lead_amp_sus_q8;

            s->pop_lead_gate_hold = 0u;
            s->pop_lead_gate_rel = 0u;
            if (!tex_arm)
                tex_arm = pop_lead_texture(era, pop_lead_variant(s->voice_plate));
            s->pop_lead_tex = tex_arm;
            if (!amp_arm) {
                if (tex_arm == POP_LEAD_TEX_CHOIR)
                    amp_arm = 217u;
                else if (tex_arm == POP_LEAD_TEX_STRING)
                    amp_arm = 191u;
                else
                    amp_arm = 166u;
            }
            s->pop_lead_amp_sus_q8 = amp_arm;
        }
        s->pop_lead_fb = 0;
    }
#else
    {
        uint16_t hold, rel;
        int8_t prev_tune = s->siren_st_tune;
        uint8_t legato = (s->siren_gate > 0u || s->siren_lead_rel > 0u
                          || s->siren_env > (s->siren_peak >> 4)) ? 1u : 0u;

        s->siren_prev_tune = prev_tune;
        s->siren_st_tune = tune;
        s->siren_trig_count++;
        dub_siren_note_gate(s->siren_trig_count, lane_gate, &hold, &rel, s->siren_hp_len_on,
                            s->siren_hp_len_ix, s->siren_hp_spp);
        s->voice_era = era;
        if (legato) {
            s->siren_gate = hold;
            s->siren_lead_rel = rel;
            if (prev_tune != tune) {
                s->siren_note_age = 0u;
                s->siren_atk_left = dub_siren_atk_samples(atk);
                if (s->siren_env > (pk >> 2))
                    s->siren_env = pk >> 2;
            } else if (s->siren_env < pk)
                s->siren_env = pk;
            return;
        }
        s->siren_gate = hold;
        s->siren_lead_rel = rel;
        s->siren_note_age = 0u;
        s->siren_atk_left = dub_siren_atk_samples(atk);
        s->siren_env = 0;
        s->siren_ph = 0u;
        s->siren_lfo_ph = 0u;
    }
#endif
#if !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_HOUSE) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    (void)era;
#endif
#if defined(GENRE_HOUSE)
    (void)era;
#endif
}

static uint8_t dub_drm_effective_decay(const dub_params_t *p, uint8_t voice, int8_t hat_gate)
{
    uint8_t ix = voice & 3u;
    uint8_t dec = p->drm_dec[ix] ? p->drm_dec[ix] : 64u;

#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_SOUL)
    if (ix == 3u) {
        int32_t kd = (int32_t)dec - 64;
        int32_t sh = 12 + (kd >> 5);
        int32_t eff;

        if (sh < 11)
            sh = 11;
        else if (sh > 13)
            sh = 13;
        /* Display: dub_env_body inverse of release shift + trim (DSP still uses sh). */
        eff = ((sh - 6) * 127) / 6 + kd;
        if (eff < 0)
            eff = 0;
        else if (eff > 127)
            eff = 127;
        return (uint8_t)eff;
    }
#endif
    {
        int32_t kd = (int32_t)dec - 64;
        int32_t eff;

        if (ix == 0u)
            eff = (int32_t)p->kick_boom + kd;
        else if (ix == 1u)
            eff = (int32_t)p->rim_tone + kd;
        else {
            uint8_t hk = p->drum_kit % DKIT_N;

#ifdef GENRE_HEAVY_PSYCH
            eff = (int32_t)heavy_psych_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)heavy_psych_hat_open[hk];
#elif defined(GENRE_HIPHOP)
            eff = (int32_t)hiphop_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)hiphop_hat_open[hk];
#elif defined(GENRE_HOUSE)
            eff = (int32_t)house_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)house_hat_open[hk];
#elif defined(GENRE_TECHNO) || defined(GENRE_BASS)
            /* render uses techno_hat_glitch_* (via idm_hat_glitch_* macros) — decay must match */
            eff = (int32_t)idm_hat_glitch_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)idm_hat_glitch_open[hk];
#elif defined(GENRE_FUNK)
            eff = (int32_t)funk_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)funk_hat_open[hk];
#elif defined(GENRE_JAZZ)
            eff = (int32_t)jazz_ride_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)jazz_ride_open[hk];
#elif defined(GENRE_SOUL)
            eff = (int32_t)soul_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)soul_hat_open[hk];
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            eff = (int32_t)idm_hat_glitch_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)idm_hat_glitch_open[hk];
#elif defined(GENRE_POP)
            eff = (int32_t)pop_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)pop_hat_open[hk];
#else
            eff = (int32_t)dub_hat_decay[hk] + (kd >> 2);
            if (hat_gate > 0)
                eff += (int32_t)dub_hat_open[hk];
#endif
        }
        if (eff < 0)
            eff = 0;
        else if (eff > 127)
            eff = 127;
        return (uint8_t)eff;
    }
}

/* UI only: map drm_dec 0..127 → display 0..127 with neutral (64) at kit base; DSP unchanged. */
static uint8_t dub_drm_display_trim_map(uint8_t dec, uint8_t base_neutral)
{
    int32_t d = (int32_t)dec;
    int32_t eff;

    if (base_neutral > 127u)
        base_neutral = 127u;
    if (d <= 64)
        eff = (d * (int32_t)base_neutral) / 64;
    else
        eff = (int32_t)base_neutral + ((d - 64) * (127 - (int32_t)base_neutral)) / 63;
    if (eff < 0)
        eff = 0;
    else if (eff > 127)
        eff = 127;
    return (uint8_t)eff;
}

/* UI only: like trim_map but dec 1 (min knob) → 0; dec 64/127 unchanged. Snare keeps trim_map. */
static uint8_t dub_drm_display_trim_map_min0(uint8_t dec, uint8_t base_neutral)
{
    int32_t d = (int32_t)dec;
    int32_t eff;

    if (base_neutral > 127u)
        base_neutral = 127u;
    if (d <= 0)
        eff = 0;
    else if (d <= 64)
        eff = ((d - 1) * (int32_t)base_neutral) / 63;
    else
        eff = (int32_t)base_neutral + ((d - 64) * (127 - (int32_t)base_neutral)) / 63;
    if (eff < 0)
        eff = 0;
    else if (eff > 127)
        eff = 127;
    return (uint8_t)eff;
}

static uint8_t dub_drm_display_decay(const dub_params_t *p, uint8_t voice, int8_t hat_gate)
{
    uint8_t ix = voice & 3u;
    uint8_t dec = p->drm_dec[ix];

#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_SOUL)
    if (ix == 3u) {
        if (dec <= 1u)
            return 0u;
        return (uint8_t)(((int32_t)dec - 1) * 127 / 126);
    }
#endif

    if (ix == 0u)
        return dub_drm_display_trim_map_min0(dec, p->kick_boom);

    if (ix == 1u)
        return dub_drm_display_trim_map(dec, p->rim_tone);

    {
        uint8_t hk = p->drum_kit % DKIT_N;
        int32_t neutral;
        uint8_t base;
        uint8_t open = 0u;
        uint8_t decay_max;
        uint8_t open_max;

#ifdef GENRE_HEAVY_PSYCH
        base = heavy_psych_hat_decay[hk];
        decay_max = 18u;
        open_max = 24u;
        if (hat_gate > 0)
            open = heavy_psych_hat_open[hk];
#elif defined(GENRE_HIPHOP)
        base = hiphop_hat_decay[hk];
        decay_max = 18u;
        open_max = 24u;
        if (hat_gate > 0)
            open = hiphop_hat_open[hk];
#elif defined(GENRE_HOUSE)
        base = house_hat_decay[hk];
        decay_max = 18u;
        open_max = 24u;
        if (hat_gate > 0)
            open = house_hat_open[hk];
#elif defined(GENRE_TECHNO) || defined(GENRE_BASS)
        base = idm_hat_glitch_decay[hk];
        decay_max = 18u;
        open_max = 22u;
        if (hat_gate > 0)
            open = idm_hat_glitch_open[hk];
#elif defined(GENRE_FUNK)
        base = funk_hat_decay[hk];
        decay_max = 64u;
        open_max = 32u;
        if (hat_gate > 0)
            open = funk_hat_open[hk];
#elif defined(GENRE_JAZZ)
        base = jazz_ride_decay[hk];
        decay_max = 18u;
        open_max = 24u;
        if (hat_gate > 0)
            open = jazz_ride_open[hk];
#elif defined(GENRE_SOUL)
        base = soul_hat_decay[hk];
        decay_max = 64u;
        open_max = 32u;
        if (hat_gate > 0)
            open = soul_hat_open[hk];
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        base = idm_hat_glitch_decay[hk];
        decay_max = 18u;
        open_max = 24u;
        if (hat_gate > 0)
            open = idm_hat_glitch_open[hk];
#elif defined(GENRE_POP)
        base = pop_hat_decay[hk];
        decay_max = 18u;
        open_max = 24u;
        if (hat_gate > 0)
            open = pop_hat_open[hk];
#else
        base = dub_hat_decay[hk];
        decay_max = 14u;
        open_max = 22u;
        if (hat_gate > 0)
            open = dub_hat_open[hk];
#endif
        neutral = ((int32_t)base * 127) / (int32_t)decay_max;
        if (open)
            neutral += ((int32_t)open * 127) / (int32_t)open_max;
        if (neutral > 127)
            neutral = 127;
        return dub_drm_display_trim_map_min0(dec, (uint8_t)neutral);
    }
}

#if !defined(GENRE_BASS)
#include "dub_envx_dsp.h"
#endif

void dub_synth_render(dub_synth_t *s, const dub_voice_t *v, int32_t *out, uint32_t n)
{
    uint32_t i;
    const dub_params_t *p;
    uint32_t dly_base;
    uint8_t dly_half_rate;
    int32_t tap_b_ofs;

    if (!s || !v || !out)
        return;
    p = &v->p;
    /* drm_dens[0..3] = PUNCH / SNAP / SIZZLE / lead TIMBRE — bipolar around 64.
     * Never gate click with (64+raw)>>7 — that collapses to 0 below 64 (“punch 50 silent”). */
    int32_t punch = (int32_t)(p->drm_dens[0] ? p->drm_dens[0] : 64u) - 64;
    int32_t snap = (int32_t)(p->drm_dens[1] ? p->drm_dens[1] : 64u) - 64;
    int32_t sizzle = (int32_t)(p->drm_dens[2] ? p->drm_dens[2] : 64u) - 64;
#if defined(GENRE_BASS)
    (void)snap;
    (void)sizzle;
#endif
    int32_t p_amt = 64 + punch;

    if (p_amt < 0)
        p_amt = 0;
    else if (p_amt > 127)
        p_amt = 127;

    s->voice_plate = p->plate;
    dly_base = dub_delay_samples(p, v->tape_oct, &dly_half_rate);
    tap_b_ofs = (int32_t)(DUB_FS / (100u * (uint32_t)dly_half_rate));

    for (i = 0; i < 4u; i++)
        dub_meter_decay(&s->meter[i]);

    {
        int8_t hat_gate = v->lane_gate[2];

#ifdef GENRE_HEAVY_PSYCH
        s->sub_body = heavy_psych_env_body_gate(p->decay_b, v->lane_gate[4], p->era);
        s->ch_body = heavy_psych_chord_env_body(p->decay_c, p->era);
#elif defined(GENRE_JAZZ)
        s->sub_body = jazz_env_body_gate(p->decay_b, v->lane_gate[4], p->era);
        s->ch_body = jazz_env_body_gate(p->decay_c, v->lane_gate[5], p->era);
#elif defined(GENRE_SOUL)
        s->sub_body = soul_env_body_gate(p->decay_b, v->lane_gate[4], p->era);
        s->ch_body = soul_env_body_gate(p->decay_c, v->lane_gate[5], p->era);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
        s->sub_body = idm_env_body_gate(p->decay_b, v->lane_gate[4], p->era);
        s->ch_body = idm_env_body_gate(p->decay_c, v->lane_gate[5], p->era);
#elif defined(GENRE_HIPHOP)
        s->sub_body = hiphop_env_body_gate(p->decay_b, v->lane_gate[4], p->era);
        s->ch_body = dub_env_body_gate(p->decay_c, v->lane_gate[5]);
#else
        s->sub_body = dub_env_body_gate(p->decay_b, v->lane_gate[4]);
        s->ch_body = dub_env_body_gate(p->decay_c, v->lane_gate[5]);
#endif
        s->kick_body = dub_drm_body_gate_q4(dub_drm_effective_decay(p, 0u, hat_gate), v->lane_gate[0]);
        s->rim_body = dub_drm_body_gate_q4(dub_drm_effective_decay(p, 1u, hat_gate), v->lane_gate[1]);
        s->hat_body = dub_hat_body_gate(dub_drm_effective_decay(p, 2u, hat_gate), v->lane_gate[2]);
#ifdef GENRE_DRM_DEC_FIT
        s->hat_oh_trim = (hat_gate > 0 && p->plate < GENRE_PRESET_N)
                             ? (int8_t)((int32_t)GENRE_DRM_DEC_FIT[p->plate][3] - 64) : 0;
#else
        s->hat_oh_trim = 0;
#endif
    }

    for (i = 0; i < n; i++) {
        int32_t sub_sig = 0;
        int32_t ch_sig = 0;
        int32_t kick_sig = 0;
        int32_t rim_sig = 0;
        int32_t hat_sig = 0;
        int32_t drum_sig = 0;
#if defined(GENRE_BASS)
        (void)drum_sig;
#endif
        int32_t siren_sig = 0;
        int32_t dly_in = 0;
#if !defined(GENRE_BASS)
        dub_envx_tick(s);
#endif
        int32_t dly_l = 0;
        int32_t dly_r = 0;
        int32_t rev_send = 0;
        uint32_t ph0 = (uint32_t)s->lfo_ph[0];
        uint32_t ph1 = (uint32_t)s->lfo_ph[1];
        int32_t lfo1 = dub_lfo_sample(p->lfo1_wave, p->lfo1_skew, p->lfo1_algo, p->lfo1_ratio, p->lfo1_index,
                                      p->lfo1_fb, p->lfo1_fold, p->lfo1_phase, p->lfo1_smooth, ph0,
                                      s->lfo_cycle[0], 0u, &s->lfo_sh[0], &s->lfo_slew[0]);
        int32_t lfo2 = dub_lfo_sample(p->lfo2_wave, p->lfo2_skew, p->lfo2_algo, p->lfo2_ratio, p->lfo2_index,
                                      p->lfo2_fb, p->lfo2_fold, p->lfo2_phase, p->lfo2_smooth, ph1,
                                      s->lfo_cycle[1], 1u, &s->lfo_sh[1], &s->lfo_slew[1]);
        int32_t lfo_mod_c = 0;
        int32_t lfo_mod_b = 0;
        int32_t lfo_mod_wob = 0;
        int32_t lfo_mod_rev = 0;
        int32_t lfo_mod_iso = 0;
        int32_t duck;
        uint32_t inc0 = dub_lfo_inc(p->lfo1_rate, v->wob_force, p->bpm);
        uint32_t inc1 = dub_lfo_inc(p->lfo2_rate, 0u, p->bpm);

        s->lfo_ph[0] = (int32_t)(ph0 + inc0);
        if ((uint32_t)s->lfo_ph[0] < ph0)
            s->lfo_cycle[0]++;
        s->lfo_ph[1] = (int32_t)(ph1 + inc1);
        if ((uint32_t)s->lfo_ph[1] < ph1)
            s->lfo_cycle[1]++;

        if (p->lfo1_tgt == DUB_LFO_CUT_C)
            lfo_mod_c = dub_lfo_depth(lfo1, p->lfo1_depth);
        else if (p->lfo1_tgt == DUB_LFO_CUT_B)
            lfo_mod_b = dub_lfo_depth(lfo1, p->lfo1_depth);
        else if (p->lfo1_tgt == DUB_LFO_WOB)
            lfo_mod_wob = dub_lfo_depth(lfo1, p->lfo1_depth);
        else if (p->lfo1_tgt == DUB_LFO_REV)
            lfo_mod_rev = dub_lfo_depth(lfo1, p->lfo1_depth);
        else if (p->lfo1_tgt == DUB_LFO_ISO)
            lfo_mod_iso = dub_lfo_depth(lfo1, p->lfo1_depth);

        if (p->lfo2_tgt == DUB_LFO_CUT_C)
            lfo_mod_c += dub_lfo_depth(lfo2, p->lfo2_depth);
        else if (p->lfo2_tgt == DUB_LFO_CUT_B)
            lfo_mod_b += dub_lfo_depth(lfo2, p->lfo2_depth);
        else if (p->lfo2_tgt == DUB_LFO_WOB)
            lfo_mod_wob += dub_lfo_depth(lfo2, p->lfo2_depth);
        else if (p->lfo2_tgt == DUB_LFO_REV)
            lfo_mod_rev += dub_lfo_depth(lfo2, p->lfo2_depth);
        else if (p->lfo2_tgt == DUB_LFO_ISO)
            lfo_mod_iso += dub_lfo_depth(lfo2, p->lfo2_depth);

#if !defined(GENRE_BASS)
        lfo_mod_wob += (lfo1 * (int32_t)p->wobble) >> 14;
#endif

        /* --- Sub / bass (10 models: RYTM port + era WOB/SUB) --- */
#if defined(GENRE_BASS)
        if (!(v->mute & 1u) && s->sub_on && s->bass_env.hard_pass) {
#else
        if (!(v->mute & 1u) && s->sub_on && s->sub_env > 0) {
#endif
            uint32_t inc = s->sub_tgt_inc;
            int32_t raw = 0;
            uint8_t model = p->bass_model % BASS_N;
            int32_t sub_nn = (int32_t)s->sub_note;

            s->sub_tgt_inc = dub_note_inc(dub_clamp_midi(sub_nn));

#if defined(GENRE_BASS)
            {
                int32_t env_mod = bass_dahdsr_step_shaped(&s->bass_env, &s->bass_env_cfg, s->bass_env_curve);

                s->sub_env = env_mod > 0 ? (uint16_t)env_mod : 0u;
            }
#else
            dub_sub_env_step(s, p->decay_b);
#endif
#if !defined(GENRE_BASS)
            if (s->sub_inc > s->sub_tgt_inc)
                s->sub_inc -= ((s->sub_inc - s->sub_tgt_inc) >> 8) + 1u;
#endif
#ifdef GENRE_HEAVY_PSYCH
            if (model == BASS_DOOM || p->era == ERA_DOOM)
                inc = s->sub_inc;
            else if ((p->era == ERA_STONER || p->era == ERA_DESERT) && s->sub_inc > s->sub_tgt_inc)
                inc = s->sub_tgt_inc + ((s->sub_inc - s->sub_tgt_inc) >> 4);
#elif !defined(GENRE_JAZZ) && !defined(GENRE_SOUL) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_TRIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS)
            if (model == BASS_808)
                inc = s->sub_inc;
#endif
#if defined(GENRE_BASS)
            {
                /* 303-style slide: exponential approach in log pitch (semitone offset decays with τ). */
                uint8_t nn = dub_clamp_midi((int32_t)s->sub_note);

                if (nn != s->bass_glide_note) {
                    if (s->bass_slide && s->bass_glide_note)
                        s->bass_glide_q16 += ((int32_t)s->bass_glide_note - (int32_t)nn) * 65536;
                    else
                        s->bass_glide_q16 = 0;
                    s->bass_glide_note = nn;
                }
                if (!s->bass_slide)
                    s->bass_glide_q16 = 0;
                if (s->bass_glide_q16 > 48 * 65536)
                    s->bass_glide_q16 = 48 * 65536;
                else if (s->bass_glide_q16 < -48 * 65536)
                    s->bass_glide_q16 = -48 * 65536;
                if (s->bass_glide_q16 != 0) {
                    int32_t tc = s->bass_glide_tc > 184u ? 184 : (int32_t)s->bass_glide_tc;
                    /* mul = 2^20 / τ_samples; 396 ↔ τ 60 ms (2646 samples) at tc 56 */
                    uint32_t mul = (396u * dub_semi_ratio_q16(-(tc - 56) * 96)) >> 16;
                    int32_t off = s->bass_glide_q16;
                    int32_t d = (int32_t)(((int64_t)off * (int64_t)mul) >> 20);

                    d += off > 0 ? 1 : -1;
                    if ((off > 0 && d > off) || (off < 0 && d < off))
                        d = off;
                    s->bass_glide_q16 = off - d;
                    inc = (uint32_t)(((uint64_t)s->sub_tgt_inc * dub_semi_ratio_q16(s->bass_glide_q16 >> 8)) >> 16);
                }
                s->sub_inc = inc;
            }
#endif

            if (p->sub_oct == 0u)
                inc >>= 1;
            else if (p->sub_oct > 80u)
                inc >>= 2;

            {
                int32_t wob_all = ((int32_t)p->wobble + lfo_mod_wob) * 128;
                int32_t wob_lfo_fc = 0;
                int32_t fc_extra = lfo_mod_b;

#if defined(GENRE_BASS)
                if (p->wobble > 0u || lfo_mod_wob != 0) {
                    wob_lfo_fc = (lfo1 * (int32_t)p->wobble) >> 6;
                    fc_extra += wob_lfo_fc + (wob_all >> 2);
                }
#else
                wob_lfo_fc = (lfo1 * (int32_t)p->wobble) >> 6;
                fc_extra += wob_lfo_fc + (wob_all >> 2);
#endif
                int32_t fc = dub_fc_from_cutoff(p->cutoff_b, s->sub_env, p->env_filt, fc_extra);

#ifdef GENRE_HEAVY_PSYCH
                raw = heavy_psych_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_HOUSE)
                raw = house_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_JAZZ)
                raw = jazz_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_SOUL)
                raw = soul_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_SNOWCRASH)
                raw = snowcrash_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_HIPHOP)
                raw = hiphop_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_IDM)
                raw = idm_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_POP)
                raw = pop_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_TECHNO)
                raw = techno_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_TRIPHOP)
                raw = triphop_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_BASS)
                raw = bass_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#elif defined(GENRE_FUNK)
                raw = funk_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);
#else
                switch (model) {
                case BASS_REESE: {
                    /* RYTM BMODEL_DUB_REESE: 3 detuned saws (+1/32, -1/32) + sub sine / 2 -> SVF */
                    uint32_t det = (inc >> 5) + (uint32_t)(wob_all >> 11);
                    int32_t s1, s2, s3, sub_s;
                    s->sub_ph += inc;
                    s->sub_ph2 += inc + det;
                    s->sub_ph3 += inc - det;
                    s1 = dub_saw(s->sub_ph);
                    s2 = dub_saw(s->sub_ph2);
                    s3 = dub_saw(s->sub_ph3);
                    sub_s = dub_sine_q64(s->sub_ph >> 1);
                    raw = ((s1 + s2 + s3) / 3) + (sub_s >> 1);
                    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
                    break;
                }
                case BASS_WOBBLE: {
                    /* Croydon 2-Op/3-Op talking FM + LFO wobble filter */
                    uint32_t w_rate = 18000u + (uint32_t)p->wobble * 2200u;
                    int32_t w_lfo, fm_mod, wf_fc;
                    s->sub_ph += inc;
                    s->sub_ph2 += w_rate;
                    s->sub_ph3 += inc << 1;
                    w_lfo = dub_sine_q64(s->sub_ph2);
                    fm_mod = (dub_sine_q64(s->sub_ph3) * (16384 + (w_lfo >> 1))) >> 15;
                    raw = dub_sine_q64(s->sub_ph + ((uint32_t)fm_mod << 16)) + (dub_saw(s->sub_ph) >> 1);
                    wf_fc = fc + ((w_lfo * (30 + (int32_t)p->wobble)) >> 6);
                    if (wf_fc < 350)
                        wf_fc = 350;
                    if (wf_fc > 12000)
                        wf_fc = 12000;
                    dub_svf_mode(raw, wf_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                                 (uint8_t)(p->reso_mode > 60u ? p->reso_mode : 60u), &raw);
                    break;
                }
                case BASS_WG: {
                    /* RYTM BMODEL_SLAP_WG: Plucked Karplus-Strong waveguide + sub sine + attack harmonic */
                    uint32_t idx = (s->sub_ph >> 26) & (DUB_WG_LEN - 1u);
                    uint32_t nidx = (idx + 1u) & (DUB_WG_LEN - 1u);
                    uint32_t frac = (s->sub_ph >> 18) & 255u;
                    int32_t s0 = s->wg[idx];
                    int32_t s1 = s->wg[nidx];
                    int32_t interp = s0 + (((s1 - s0) * (int32_t)frac) >> 8);
                    int32_t prev_idx = idx;

                    s->sub_ph += inc;
                    if (((s->sub_ph >> 26) & (DUB_WG_LEN - 1u)) != prev_idx) {
                        int32_t avg = (s0 + s1) >> 1;
                        int32_t damp = 236 + ((int32_t)p->decay_b >> 3);
                        if (damp > 255)
                            damp = 255;
                        s->wg[prev_idx] = (int16_t)((avg * damp) >> 8);
                    }
                    raw = interp + (dub_sine_q64(s->sub_ph >> 1) >> 1) +
                          ((dub_sine_q64(s->sub_ph << 2) * (s->sub_env >> 8)) >> 8);
                    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
                    break;
                }
                case BASS_808: {
                    /* RYTM BMODEL_SUB808: Pitch-drop boom + 2nd harmonic punch + softclip */
                    int32_t fund, harm;
                    s->sub_ph += inc;
                    fund = dub_sine_q64(s->sub_ph);
                    harm = dub_sine_q64(s->sub_ph << 1);
                    raw = fund + (harm >> 2) + ((harm * (int32_t)p->cutoff_b) >> 7);
                    raw = dub_softclip_q15(raw + ((fund * (int32_t)p->wobble) >> 7));
                    dub_svf_mode(raw, 1200 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
                    break;
                }
                case BASS_303: {
                    /* RYTM BMODEL_ACID303: Saw + Square/4 -> dual cascaded resonant SVF -> wavefold */
                    int32_t saw_w, sq_w, fold_amt;
                    s->sub_ph += inc;
                    saw_w = dub_saw(s->sub_ph);
                    sq_w = dub_square(s->sub_ph + ((uint32_t)wob_all << 8));
                    raw = saw_w + (sq_w >> 1);
                    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2,
                                 (uint8_t)(p->reso_mode + 32u > 127u ? 127u : p->reso_mode + 32u), &raw);
                    dub_svf_mode(raw, fc, &s->sub_svf_b1, &s->sub_svf_b2, (uint8_t)(p->reso_mode >> 1), &raw);
                    fold_amt = 256 + (int32_t)p->wobble * 3 + (int32_t)p->drive * 2;
                    raw = (raw * fold_amt) >> 8;
                    {
                        int32_t f;

                        for (f = 0; f < 3; f++) {
                            if (raw > 32767)
                                raw = 65534 - raw;
                            else if (raw < -32767)
                                raw = -65534 - raw;
                        }
                    }
                    break;
                }
                case BASS_FM: {
                    /* RYTM BMODEL_SOLID_FM: 2:1 feedback FM + 3:1 harmonic operator (hollow digital dancehall bass) */
                    int32_t mod2, mod3;
                    s->sub_ph += inc;
                    s->sub_ph2 += inc << 1;
                    s->sub_ph3 += inc * 3u;
                    mod2 = dub_sine_q64(s->sub_ph2 + ((uint32_t)s->sub_fm_fb << 15));
                    s->sub_fm_fb = (mod2 * (48 + (int32_t)p->wobble)) >> 8;
                    mod2 = (mod2 * (28 + (int32_t)p->cutoff_b + (s->sub_env >> 8))) >> 7;
                    mod3 = (dub_sine_q64(s->sub_ph3) * (16 + (int32_t)p->reso_mode + (int32_t)p->wobble)) >> 7;
                    raw = dub_sine_q64(s->sub_ph + ((uint32_t)(mod2 + mod3) << 15));
                    break;
                }
                case BASS_PWM: {
                    /* RYTM BMODEL_PWM: LFO-modulated pulse width + sub-octave square -> SVF */
                    uint32_t duty;
                    int32_t pulse, sub_sq;
                    s->sub_ph += inc;
                    s->sub_ph2 += 24000u + (uint32_t)p->wobble * 1400u;
                    duty = 0x20000000u + ((uint32_t)p->cutoff_b << 23) +
                           (uint32_t)((dub_sine_q64(s->sub_ph2) + 32768) << 14);
                    pulse = (s->sub_ph < duty) ? 20000 : -20000;
                    sub_sq = ((s->sub_ph >> 1) & 0x80000000u) ? 9000 : -9000;
                    raw = pulse + sub_sq;
                    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
                    break;
                }
                case BASS_WOB: {
                    /* Croydon slow square/talk — slower mod than GROWL, static SVF (no filter wobble chase) */
                    uint32_t w_rate = 4800u + (uint32_t)p->wobble * 520u;
                    int32_t w_lfo, sq_mod;
                    s->sub_ph += inc;
                    s->sub_ph2 += w_rate;
                    s->sub_ph3 += inc + (inc >> 2);
                    w_lfo = dub_sine_q64(s->sub_ph2);
                    sq_mod = dub_square(s->sub_ph3 + ((uint32_t)w_lfo << 17));
                    raw = (dub_square(s->sub_ph + (uint32_t)(sq_mod >> 1)) >> 1) +
                          (dub_sine_q64(s->sub_ph) >> 2);
                    dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2,
                                 (uint8_t)(p->reso_mode > 40u ? p->reso_mode : 40u), &raw);
                    break;
                }
                case BASS_SUBS: {
                    /* Postdub digital sub stack — fund + sub + 2nd harmonic sines, no pluck/waveguide */
                    s->sub_ph += inc;
                    raw = dub_sine_q64(s->sub_ph);
                    raw += dub_sine_q64(s->sub_ph >> 1) >> 1;
                    raw += dub_sine_q64(s->sub_ph << 1) >> 3;
                    dub_svf_mode(raw, 800 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
                    break;
                }
                default:
                    /* BASS_SUB: pure deep sub sine + subtle warmth from wobble */
                    s->sub_ph += inc;
                    s->sub_ph2 += (uint32_t)(wob_all >> 11);
                    raw = dub_sine_q64(s->sub_ph + (s->sub_ph2 >> 4)) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
                    dub_svf_mode(raw, 600 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
                    break;
                }
#endif
            }
            if (raw > 32767)
                raw = 32767;
            else if (raw < -32767)
                raw = -32767;
#if defined(GENRE_BASS)
            {
                int32_t env_mod = bass_dahdsr_mod_q15(&s->bass_env, &s->bass_env_cfg);
                int32_t amp = (s->bass_amp_q15 * env_mod) >> 15;
                int32_t vca = bass_hard_vca(raw, &s->bass_env);

                sub_sig = (vca * amp) >> 15;
            }
#else
            sub_sig = (raw * s->sub_env) >> 15;
#endif
            if (p->lvl_b == 0u)
                sub_sig = 0;
#ifdef GENRE_HEAVY_PSYCH
            else
                sub_sig = (sub_sig * (int32_t)p->lvl_b) / 88;
#else
            else
                sub_sig = (sub_sig * (int32_t)p->lvl_b) / 96;
#endif
            dub_meter_peak(&s->meter[0], sub_sig);
#if defined(GENRE_BASS)
            if (s->bass_env.stage == BASS_ENV_IDLE) {
                s->sub_on = 0u;
                s->sub_env = 0;
            }
#endif
#if defined(GENRE_BASS)
        } else if (s->sub_on && s->bass_env.stage != BASS_ENV_IDLE) {
            bass_dahdsr_step_shaped(&s->bass_env, &s->bass_env_cfg, s->bass_env_curve);
            if (s->bass_env.stage == BASS_ENV_IDLE) {
                s->sub_on = 0u;
                s->sub_env = 0;
            }
        }
#else
        } else if (s->sub_env > 0) {
            dub_sub_env_step(s, p->decay_b);
        }
#endif

        /* --- Chord (rich harmonic organ/FM/brass skank so CUTOFF, RESONA, ENVLPE & TIMBRE are all dramatic) --- */
#if !defined(GENRE_BASS)
        if (!(v->mute & 2u) && s->ch_env > 0) {
            int32_t mix = 0;
            int32_t tmb = (int32_t)p->timbre;
            int32_t ch_root_live;
            int32_t glide_sh = 0;

            if (p->glide_c > 0u) {
                int32_t cur = (int32_t)s->ch_root;
                int32_t tgt = (int32_t)s->ch_root_tgt;

#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_SOUL)
                /* Higher GLIDE = slower. 10 ≈ 25 ms, 15 ≈ 0.7 s. */
                glide_sh = 10 + (int32_t)(((uint32_t)p->glide_c * 5u) / 127u);
#else
                glide_sh = 9 - ((int32_t)p->glide_c >> 4);
#endif
                if (glide_sh < 1)
                    glide_sh = 1;
                if (cur != tgt) {
                    if (tgt > cur) {
                        cur += (tgt - cur) >> glide_sh;
                        if (cur >= tgt)
                            cur = tgt;
                    } else {
                        cur -= (cur - tgt) >> glide_sh;
                        if (cur <= tgt)
                            cur = tgt;
                    }
                    s->ch_root = (uint8_t)cur;
                }
            } else {
                s->ch_root = s->ch_root_tgt;
            }
            ch_root_live = (int32_t)s->ch_root;

            dub_ch_env_step(s, p->decay_c);
#ifdef GENRE_HEAVY_PSYCH
            mix = heavy_psych_chord_render(s, p, ch_root_live, glide_sh, tmb, lfo_mod_c, s->ch_env);
#elif defined(GENRE_POP)
            mix = pop_chord_render(s, p, ch_root_live, glide_sh, tmb, lfo_mod_c, s->ch_env);
#elif defined(GENRE_SOUL)
            mix = soul_chord_render(s, p, ch_root_live, glide_sh, tmb, lfo_mod_c, s->ch_env);
#else
            {
                uint32_t vi;
                uint8_t vo = p->voicing;

            for (vi = 0; vi < DUB_CHORD_N; vi++) {
                uint8_t n = dub_clamp_midi(ch_root_live + (int32_t)dub_chord_interval(vo, (uint8_t)vi));
                uint32_t ph;
                int32_t organ, fm_bell, brass_saw, osc;
                uint32_t tgt_inc = dub_note_inc(n);

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
                s->ch_ph[vi] += s->ch_inc[vi];
                ph = s->ch_ph[vi];

                /* Drawbar organ foundation (1st + 2nd + 3rd harmonic + warm sub-pulse) */
                organ = (dub_sine_q64(ph) >> 1) + (dub_sine_q64(ph << 1) >> 2) + (dub_square(ph) >> 2);
                /* Continuous FM bell/metallic modulation scaling smoothly with TIMBRE */
                {
                    int32_t mod = (dub_sine_q64(ph * 2u) + (dub_sine_q64(ph * 3u) >> 1)) * (16 + tmb * 2);
                    fm_bell = dub_sine_q64(ph + ((uint32_t)mod << 9));
                }
                /* Rich detuned saw + wavefold edge for high TIMBRE (64..127) */
                brass_saw = dub_saw(ph) + (dub_saw((ph << 1) + 0x20000000u) >> 1);

#ifdef GENRE_FUNK
                if (p->era == ERA_CLAV || p->era == ERA_BOOGIE) {
                    int32_t ix = (p->era == ERA_CLAV) ? 10 : 16;
                    osc = dub_fm_ep_tine(ph, ph, ix);
                } else
#endif
                if (tmb <= 64)
                    osc = ((organ * (64 - tmb)) + (fm_bell * tmb)) >> 6;
                else
                    osc = ((fm_bell * (127 - tmb)) + (brass_saw * (tmb - 64))) >> 6;

                mix += osc >> 2;
            }
            }
#endif
            {
                int32_t fc = dub_fc_from_cutoff(p->cutoff_c, s->ch_env, p->env_filt, lfo_mod_c);

#ifdef GENRE_HEAVY_PSYCH
                fc += heavy_psych_chord_fc_bias(p->era);
                if (fc < 260)
                    fc = 260;
                fc = heavy_psych_chord_drift_fc(fc, p->era, s->ch_ph[0], lfo_mod_c);
#endif
                dub_svf_mode(mix, fc, &s->ch_svf_l1, &s->ch_svf_l2, p->reso_mode, &mix);
            }
            ch_sig = (mix * s->ch_env) >> 15;
            if (p->lvl_c == 0u)
                ch_sig = 0;
            else
#ifdef GENRE_HEAVY_PSYCH
                ch_sig = (ch_sig * (int32_t)p->lvl_c) / (int32_t)heavy_psych_chord_level_div(p->era);
#else
                ch_sig = (ch_sig * (int32_t)p->lvl_c) / 92;
#endif
#ifdef GENRE_HEAVY_PSYCH
            {
                int32_t clean = ch_sig;
                int32_t fuzzed;
                uint8_t wet = heavy_psych_chord_fuzz_wet_q8(p->era);

                fuzzed = heavy_psych_fuzz(s, clean, heavy_psych_chord_fuzz_grit(p->era, p->drive), 0u);
                ch_sig = (clean * (256 - (int32_t)wet) + fuzzed * (int32_t)wet) >> 8;
            }
            heavy_psych_host_chord_sample(ch_sig, s->ch_env);
#endif
            dub_meter_peak(&s->meter[1], ch_sig);
        } else if (s->ch_env > 0) {
            dub_ch_env_step(s, p->decay_c);
        }
#endif /* !GENRE_BASS */

#if !defined(GENRE_BASS)
        /* --- Drums (separate KICK, RIM, HAT signals + 22ms flam retrigger scheduler) --- */
        {
            uint32_t fi;
            for (fi = 0; fi < 3u; fi++) {
                if (s->flam_left[fi] > 0u) {
                    s->flam_left[fi]--;
                    if (s->flam_left[fi] == 0u) {
                        if (fi == 0u)
                            dub_trig_kick(s, s->flam_vel[0], s->kick_st_tune, p->drm_atk[0]);
                        else if (fi == 1u)
                            dub_trig_rim(s, s->flam_vel[1], s->rim_st_tune, p->drm_atk[1]);
                        else
                            dub_trig_hat(s, s->flam_vel[2], s->hat_st_tune, p->drm_atk[2]);
                    }
                }
            }
        }
        if (!(v->mute & 4u)) {
#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_SOUL)
#ifdef GENRE_HEAVY_PSYCH
            static const int8_t *kit_k_note = heavy_psych_kit_k_note;
            static const uint8_t *kit_k_sweep = heavy_psych_kit_k_sweep;
            static const uint8_t *kit_k_fm = heavy_psych_kit_k_fm;
            static const uint8_t *kit_k_drv = heavy_psych_kit_k_drv;
            static const int8_t *kit_r_note = heavy_psych_kit_r_note;
            static const uint8_t *kit_r_wire = heavy_psych_kit_r_wire;
#elif defined(GENRE_HOUSE)
            static const int8_t *kit_k_note = house_kit_k_note;
            static const uint8_t *kit_k_sweep = house_kit_k_sweep;
            static const uint8_t *kit_k_fm = house_kit_k_fm;
            static const uint8_t *kit_k_drv = house_kit_k_drv;
            static const int8_t *kit_r_note = house_kit_r_note;
            static const uint8_t *kit_r_wire = house_kit_r_wire;
#elif defined(GENRE_TECHNO) || defined(GENRE_BASS)
            static const int8_t *kit_k_note = techno_kit_k_note;
            static const uint8_t *kit_k_sweep = techno_kit_k_sweep;
            static const uint8_t *kit_k_fm = techno_kit_k_fm;
            static const uint8_t *kit_k_drv = techno_kit_k_drv;
            static const int8_t *kit_r_note = techno_kit_r_note;
            static const uint8_t *kit_r_wire = techno_kit_r_wire;
#elif defined(GENRE_FUNK)
            static const int8_t *kit_k_note = funk_kit_k_note;
            static const uint8_t *kit_k_sweep = funk_kit_k_sweep;
            static const uint8_t *kit_k_fm = funk_kit_k_fm;
            static const uint8_t *kit_k_drv = funk_kit_k_drv;
            static const int8_t *kit_r_note = funk_kit_r_note;
            static const uint8_t *kit_r_wire = funk_kit_r_wire;
#elif defined(GENRE_JAZZ)
            static const int8_t *kit_k_note = jazz_kit_k_note;
            static const uint8_t *kit_k_sweep = jazz_kit_k_sweep;
            static const uint8_t *kit_k_fm = jazz_kit_k_fm;
            static const uint8_t *kit_k_drv = jazz_kit_k_drv;
            static const int8_t *kit_r_note = jazz_kit_r_note;
            static const uint8_t *kit_r_wire = jazz_kit_r_wire;
#elif defined(GENRE_SOUL)
            static const int8_t *kit_k_note = soul_kit_k_note;
            static const uint8_t *kit_k_sweep = soul_kit_k_sweep;
            static const uint8_t *kit_k_fm = soul_kit_k_fm;
            static const uint8_t *kit_k_drv = soul_kit_k_drv;
            static const int8_t *kit_r_note = soul_kit_r_note;
            static const uint8_t *kit_r_wire = soul_kit_r_wire;
#elif defined(GENRE_HIPHOP)
            static const int8_t *kit_k_note = hiphop_kit_k_note;
            static const uint8_t *kit_k_sweep = hiphop_kit_k_sweep;
            static const uint8_t *kit_k_fm = hiphop_kit_k_fm;
            static const uint8_t *kit_k_drv = hiphop_kit_k_drv;
            static const int8_t *kit_r_note = hiphop_kit_r_note;
            static const uint8_t *kit_r_wire = hiphop_kit_r_wire;
#elif defined(GENRE_POP)
            static const int8_t *kit_k_note = pop_kit_k_note;
            static const uint8_t *kit_k_sweep = pop_kit_k_sweep;
            static const uint8_t *kit_k_fm = pop_kit_k_fm;
            static const uint8_t *kit_k_drv = pop_kit_k_drv;
            static const int8_t *kit_r_note = pop_kit_r_note;
            static const uint8_t *kit_r_wire = pop_kit_r_wire;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            static const int8_t *kit_k_note = idm_kit_k_note;
            static const uint8_t *kit_k_sweep = idm_kit_k_sweep;
            static const uint8_t *kit_k_fm = idm_kit_k_fm;
            static const uint8_t *kit_k_drv = idm_kit_k_drv;
            static const int8_t *kit_r_note = idm_kit_r_note;
            static const uint8_t *kit_r_wire = idm_kit_r_wire;
#endif
#else
            /* 8 RYTM-inspired drum kits: ROOTS, ARK, 808, 909, STEPPA, CR78, CROYDON, METAL */
            /* ix2=TR808: ~46Hz sine boom; ix6=DMZ / ix7=ANVIL tamed */
            static const int8_t kit_k_note[DKIT_N]  = { 38, 35, 30, 36, 40, 42, 33, 37 };
            static const uint8_t kit_k_sweep[DKIT_N]= { 16, 12, 22, 28, 32, 14, 14, 22 };
            static const uint8_t kit_k_fm[DKIT_N]   = {  4, 12,  0, 18, 24,  6,  6, 14 };
            static const uint8_t kit_k_drv[DKIT_N]  = {  8, 22, 10, 36, 26, 10, 18, 24 };
            static const int8_t kit_r_note[DKIT_N]  = { 71, 63, 76, 60, 74, 79, 58, 68 };
            static const uint8_t kit_r_wire[DKIT_N] = { 28, 64, 16, 78, 36, 22, 86, 92 };
#endif
            uint8_t kit = p->drum_kit % DKIT_N;
            int32_t kt_k = ((int32_t)(p->drm_tune[0] ? p->drm_tune[0] : 64u) - 64) * 12 / 64;
            int32_t kt_r = ((int32_t)(p->drm_tune[1] ? p->drm_tune[1] : 64u) - 64) * 24 / 64;
            int32_t kt_h = ((int32_t)(p->drm_tune[2] ? p->drm_tune[2] : 64u) - 64) * 24 / 64;
            int32_t k_drv_tot = (int32_t)kit_k_drv[kit] + (int32_t)p->drm_drv[0];
            uint8_t rim_drv = p->drm_drv[1];
            uint8_t hat_drv = p->drm_drv[2];

            if (s->kick_env > 0 && p->kick_boom > 0u) {
                int32_t click;
                int32_t kick_nn = (int32_t)kit_k_note[kit] + kt_k + (int32_t)s->kick_st_tune;
                int32_t fm_m, body_k;

                if (kick_nn < 28)
                    kick_nn = 28;
                s->kick_tgt_inc = dub_note_inc(dub_clamp_midi(kick_nn));
                if (s->kick_fresh) {
                    /* trig has no kit/drm_tune — start the sweep from the real kick note */
                    s->kick_inc = s->kick_tgt_inc + (s->kick_tgt_inc >> 1);
                    s->kick_fresh = 0u;
                }
                dub_drm_env_step(&s->kick_env, &s->kick_snap, &s->kick_peak, &s->kick_atk_left,
                                 s->kick_body, 40u);
#if !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS)
                if (kit == (uint8_t)DKIT_808) {
                    /* Classic TR-808 BD: pure sine + long pitch drop. No click/FM.
                     * PUNCH only speeds the drop (like a tighter decay feel). */
                    if (s->kick_snap > 200u)
                        s->kick_inc = s->kick_tgt_inc + (s->kick_tgt_inc >> 1) +
                                      (s->kick_tgt_inc >> 3);
                    {
                        int32_t sw = 5 + ((p_amt * 12) >> 7);

                        if (s->kick_inc > s->kick_tgt_inc)
                            s->kick_inc -= ((s->kick_inc - s->kick_tgt_inc) * sw) >> 14;
                    }
                    s->kick_ph += s->kick_inc;
                    body_k = dub_sine_q64(s->kick_ph);
                    kick_sig = (body_k * s->kick_env) >> 15;
                    kick_sig = dub_softclip_q15((kick_sig * (248 + (k_drv_tot >> 1))) >> 8);
                } else
#endif
                {
                    /* Sweep uses softer punch curve — high kits (DMZ) used to scream above ~50 */
                    {
                        int32_t sw = (kit_k_sweep[kit] * (40 + (p_amt >> 1))) >> 7;

                        if (s->kick_inc > s->kick_tgt_inc)
                            s->kick_inc -= ((s->kick_inc - s->kick_tgt_inc) * sw) >> 12;
                    }
                    s->kick_ph += s->kick_inc;
                    /* PUNCH: mild FM + soft sine beater; noise only near max */
                    fm_m = (dub_sine_q64(s->kick_ph * 3u) *
                            ((int32_t)kit_k_fm[kit] + (punch > 0 ? (punch >> 4) : 0)) *
                            (s->kick_env >> 8)) >> 7;
                    if (fm_m < 0)
                        fm_m = 0;
                    body_k = dub_sine_q64(s->kick_ph + ((uint32_t)fm_m << 13));
                    click = 0;
                    if (s->kick_snap > 150u) {
                        int32_t tick = dub_sine_q64(s->kick_ph << 2);
                        int32_t amt = 8 + (p_amt >> 4) + (punch > 0 ? (punch >> 3) : 0);

                        click = (tick * amt) >> 8;
                        if (punch > 40)
                            click += (dub_noise(&s->noise) * (punch >> 3)) >> 9;
                    }
                    kick_sig = (((body_k + click) * s->kick_env) >> 15);
#ifdef GENRE_HEAVY_PSYCH
                    kick_sig = heavy_psych_drm_kick_shape(kick_sig, s->kick_snap, k_drv_tot, &s->noise);
#elif defined(GENRE_JAZZ)
                    kick_sig = jazz_drm_kick_shape(kick_sig, s->kick_snap, k_drv_tot, &s->noise);
#elif defined(GENRE_SOUL)
                    kick_sig = soul_drm_kick_shape(kick_sig, s->kick_snap, k_drv_tot, &s->noise);
#elif defined(GENRE_HIPHOP)
                    kick_sig = hiphop_drm_kick_shape(kick_sig, s->kick_snap, k_drv_tot, &s->noise);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
                    kick_sig = idm_drm_kick_shape(kick_sig, s->kick_snap, k_drv_tot, &s->noise);
#else
                    kick_sig = dub_softclip_q15((kick_sig * (256 + k_drv_tot * 3)) >> 8);
#endif
                }
                kick_sig = (kick_sig * (int32_t)p->kick_boom) >> 7;
#ifdef GENRE_HEAVY_PSYCH
                kick_sig += kick_sig >> 3;
#endif
                dub_drm_svf(&kick_sig, p->drm_cut[0], &s->kick_f_l1, &s->kick_f_l2);
            } else if (s->kick_env > 0) {
                dub_drm_env_step(&s->kick_env, &s->kick_snap, &s->kick_peak, &s->kick_atk_left,
                                 s->kick_body, 40u);
            }
            if (s->rim_env > 0 && p->rim_tone > 0u) {
                int32_t rim_nn = (int32_t)kit_r_note[kit] + kt_r + (int32_t)s->rim_st_tune;
                uint32_t r_inc = dub_note_inc(dub_clamp_midi(rim_nn));
                int32_t shell, wire, mix_r;

                dub_drm_env_step(&s->rim_env, &s->rim_snap, &s->rim_peak, &s->rim_atk_left,
                                 s->rim_body, 20u);
                /* Clap/snap: shell + wire (House = more wire, less shell) */
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
                if (kit == DKIT_STEPPA || kit == DKIT_METAL)
                    r_inc += (uint32_t)(s->rim_env >> 3) * 420u;
#endif
                s->rim_ph += r_inc;
                shell = (dub_sine_q64(s->rim_ph) + (dub_square(s->rim_ph * 3u >> 1) >> 1));
                {
                    /* SNAP: −64..+63 → wire noise; wide range so Jazz rim (high kit_wire) still moves */
                    int32_t w_amt = (int32_t)kit_r_wire[kit] + snap + ((snap * 3) >> 2);

                    if (w_amt < 0)
                        w_amt = 0;
                    else if (w_amt > 127)
                        w_amt = 127;
                    wire = (dub_noise(&s->noise) * w_amt) >> 6;
                }
#if defined(GENRE_HEAVY_PSYCH)
                mix_r = heavy_psych_drm_rim_mix(shell, wire, kit_r_wire[kit], rim_drv);
#elif defined(GENRE_HOUSE)
                mix_r = ((shell * (48 - (kit_r_wire[kit] >> 3))) >> 7) + (wire >> 1);
#elif defined(GENRE_FUNK)
                mix_r = ((shell * (48 - (kit_r_wire[kit] >> 3))) >> 7) + (wire >> 1);
#elif defined(GENRE_JAZZ)
                mix_r = jazz_drm_rim_mix(shell, wire, kit_r_wire[kit], rim_drv);
#elif defined(GENRE_SOUL)
                mix_r = soul_drm_snare_mix(shell, wire, kit_r_wire[kit], rim_drv, s->rim_snap);
#elif defined(GENRE_HIPHOP)
                mix_r = hiphop_drm_rim_mix(shell, wire, kit_r_wire[kit], rim_drv);
#elif defined(GENRE_SNOWCRASH)
                if (s->ks_fresh & 1u) {
                    snowcrash_ks_excite(0u, kt_r + (int32_t)s->rim_st_tune);
                    s->ks_fresh &= (uint8_t)~1u;
                }
                mix_r = snowcrash_ks_step(0u) >> 1;
                (void)shell;
                (void)wire;
                (void)rim_drv;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
                mix_r = idm_drm_rim_mix(shell, wire, kit_r_wire[kit], rim_drv);
#elif defined(GENRE_POP)
                mix_r = ((shell * (48 - (kit_r_wire[kit] >> 3))) >> 7) + (wire >> 1);
#else
                mix_r = ((shell * (128 - (kit_r_wire[kit] >> 1))) >> 8) + (wire >> 1);
#endif
                {
                    int64_t r = ((((int64_t)mix_r * s->rim_env) >> 15) * (int32_t)p->rim_tone) >> 7;

                    if (r > 32767)
                        r = 32767;
                    else if (r < -32767)
                        r = -32767;
                    rim_sig = (int32_t)r;
                }
#ifdef GENRE_HEAVY_PSYCH
                rim_sig += rim_sig >> 4;
#elif !defined(GENRE_JAZZ) && !defined(GENRE_SOUL) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) \
    && !defined(GENRE_TRIPHOP) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS)
                if (rim_drv > 0u)
                    rim_sig = dub_softclip_q15((rim_sig * (256 + (int32_t)rim_drv * 2)) >> 8);
#endif
                dub_drm_svf(&rim_sig, p->drm_cut[1], &s->rim_f_l1, &s->rim_f_l2);
            } else if (s->rim_env > 0) {
                dub_drm_env_step(&s->rim_env, &s->rim_snap, &s->rim_peak, &s->rim_atk_left,
                                 s->rim_body, 20u);
            }
            if (s->hat_env > 0 && p->hat_lvl > 0u) {
                uint32_t h;
                int32_t hm = 0;
#ifdef GENRE_HEAVY_PSYCH
                int32_t hat_base = (int32_t)heavy_psych_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_HIPHOP)
                int32_t hat_base = (int32_t)hiphop_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_HOUSE)
                int32_t hat_base = (int32_t)house_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_FUNK)
                int32_t hat_base = (int32_t)funk_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_JAZZ)
                int32_t hat_base = (int32_t)jazz_ride_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_SOUL)
                int32_t hat_base = (int32_t)soul_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
                int32_t hat_base = (int32_t)idm_hat_glitch_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#elif defined(GENRE_POP)
                int32_t hat_base = (int32_t)pop_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#else
                int32_t hat_base = (int32_t)dub_hat_note[kit] + kt_h + (int32_t)s->hat_st_tune;
#endif
                uint8_t hc;

                dub_hat_env_step(s);
#ifdef GENRE_HEAVY_PSYCH
                {
                    static const int8_t h_offs_tr[4] = { 0, 4, 8, 13 };
                    static const int8_t h_offs_cr[4] = { 0, -7, 5, 12 };
                    const int8_t *h_offs = h_offs_tr;
                    uint8_t h_style = heavy_psych_hat_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (kit == DKIT_CR78)
                        h_offs = h_offs_cr;
                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)heavy_psych_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)heavy_psych_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)heavy_psych_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)heavy_psych_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)heavy_psych_hat_noise[kit];
                        met_w = (int32_t)heavy_psych_hat_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    for (h = 0; h < 4u; h++) {
                        s->hat_ph[h] += dub_note_inc(dub_clamp_midi(hat_base + h_offs[h]));
                        if (h_style == HAT_STYLE_SIZZLE || (h_style != HAT_STYLE_TONE && heavy_psych_hat_fm[kit] > 24u)) {
                            int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * (int32_t)heavy_psych_hat_fm[kit];

                            hm += dub_sine_q64(s->hat_ph[h] + ((uint32_t)hfm << 11));
                        } else if (h_style == HAT_STYLE_TONE) {
                            hm += dub_sine_q64(s->hat_ph[h]) + (dub_square(s->hat_ph[h] >> 1) >> 3);
                        } else {
                            hm += dub_square(s->hat_ph[h]);
                        }
                    }
                    hm >>= 2;
                    if (h_style == HAT_STYLE_TR6)
                        hm &= ~63;
                    if (h_style == HAT_STYLE_HISS)
                        hm >>= 1;
                    hm = ((hm * met_w) >> 7) + ((dub_noise(&s->noise) * nz_gain) >> 8);
                }
#elif defined(GENRE_JAZZ)
                {
                    uint32_t r_inc = dub_hat_fm_inc(hat_base);
                    int32_t h_met, h_nz;
                    uint8_t rst = jazz_ride_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)jazz_ride_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)jazz_ride_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)jazz_ride_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)jazz_ride_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)jazz_ride_noise[kit];
                        met_w = (int32_t)jazz_ride_metal[kit];
                    }
                    /* RIDE SIZZLE: wash/noise vs bell metal — not hit density */
                    nz_gain += (sizzle * 5) >> 2;
                    met_w += sizzle >> 1;
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    {
                        uint8_t acid = (uint8_t)jazz_ride_is_acid_kit(kit);
                        int32_t fm = (int32_t)jazz_ride_fm[kit] + (rst == JAZZ_RIDE_WASH ? 24 : 0);

                        h_met = dub_hat_fm_metal(s, r_inc, fm, (uint8_t)(rst == JAZZ_RIDE_BELL), acid);
                        h_met = (h_met * met_w) >> 7;
                        h_nz = (dub_noise(&s->noise) * nz_gain) >> (acid ? 8 : 9);
                        if (rst == JAZZ_RIDE_WASH && !acid)
                            h_nz += h_nz >> 1;
                        hm = dub_hat_hp2k(s, h_met + h_nz);
                    }
                    (void)h;
                }
#elif defined(GENRE_SOUL)
                {
                    int32_t nz_gain;
                    int32_t met_w;

                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)soul_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)soul_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)soul_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)soul_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)soul_hat_noise[kit];
                        met_w = (int32_t)soul_hat_metal[kit];
                    }
                    nz_gain += (sizzle * 5) >> 2;
                    met_w += sizzle >> 1;
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    if (s->hat_body > 8) {
                        int32_t oh = (((int32_t)s->hat_body - 6) * 8 * (int32_t)soul_hat_open[kit]) / 3;

                        nz_gain += oh >> 6;
                        if (soul_hat_is_tamb_kit(kit))
                            nz_gain += oh >> 5;
                        if (nz_gain > 127)
                            nz_gain = 127;
                    }
                    hm = soul_hat_render(s, hat_base, kit, nz_gain, met_w, s->hat_env);
                    (void)h;
                }
#elif defined(GENRE_SNOWCRASH)
                {
                    if (s->ks_fresh & 2u) {
                        snowcrash_ks_excite(1u, kt_h + (int32_t)s->hat_st_tune);
                        s->ks_fresh &= (uint8_t)~2u;
                    }
                    hm = snowcrash_ks_step(1u) >> 1;
                    (void)h;
                    (void)hat_base;
                }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
                {
                    uint32_t r_inc = dub_hat_fm_inc(hat_base);
                    int32_t h_met, h_nz;
                    uint8_t rst = idm_hat_glitch_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)idm_hat_glitch_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)idm_hat_glitch_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)idm_hat_glitch_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)idm_hat_glitch_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)idm_hat_glitch_noise[kit];
                        met_w = (int32_t)idm_hat_glitch_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    {
                        uint8_t acid = (uint8_t)idm_hat_glitch_is_acid_kit(kit);
                        int32_t fm = (int32_t)idm_hat_glitch_fm[kit] + (rst == IDM_HAT_GLITCH_WASH ? 24 : 0);

                        h_met = dub_hat_fm_metal(s, r_inc, fm, (uint8_t)(rst == IDM_HAT_GLITCH_BELL), acid);
                        h_met = (h_met * met_w) >> 7;
                        h_nz = (dub_noise(&s->noise) * nz_gain) >> (acid ? 8 : 9);
                        if (rst == IDM_HAT_GLITCH_WASH && !acid)
                            h_nz += h_nz >> 1;
                        hm = dub_hat_hp2k(s, h_met + h_nz);
                    }
                    (void)h;
                }
#elif defined(GENRE_HIPHOP)
                {
                    static const int8_t h_offs_tr[4] = { 0, 4, 8, 13 };
                    static const int8_t h_offs_cr[4] = { 0, -7, 5, 12 };
                    const int8_t *h_offs = h_offs_tr;
                    uint8_t h_style = hiphop_hat_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (kit == DKIT_CR78)
                        h_offs = h_offs_cr;
                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)hiphop_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)hiphop_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)hiphop_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)hiphop_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)hiphop_hat_noise[kit];
                        met_w = (int32_t)hiphop_hat_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    for (h = 0; h < 4u; h++) {
                        s->hat_ph[h] += dub_note_inc(dub_clamp_midi(hat_base + h_offs[h]));
                        if (h_style == HAT_STYLE_SIZZLE || (h_style != HAT_STYLE_TONE && hiphop_hat_fm[kit] > 24u)) {
                            int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * (int32_t)hiphop_hat_fm[kit];

                            hm += dub_sine_q64(s->hat_ph[h] + ((uint32_t)hfm << 11));
                        } else if (h_style == HAT_STYLE_TONE) {
                            hm += dub_sine_q64(s->hat_ph[h]) + (dub_square(s->hat_ph[h] >> 1) >> 3);
                        } else {
                            hm += dub_square(s->hat_ph[h]);
                        }
                    }
                    hm >>= 2;
                    if (h_style == HAT_STYLE_TR6)
                        hm &= ~63;
                    if (h_style == HAT_STYLE_HISS)
                        hm >>= 1;
                    hm = ((hm * met_w) >> 7) + ((dub_noise(&s->noise) * nz_gain) >> 8);
                }
#elif defined(GENRE_POP)
                {
                    static const int8_t h_offs_tr[4] = { 0, 4, 8, 13 };
                    const int8_t *h_offs = h_offs_tr;
                    uint8_t h_style = pop_hat_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)pop_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)pop_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)pop_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)pop_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)pop_hat_noise[kit];
                        met_w = (int32_t)pop_hat_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    for (h = 0; h < 4u; h++) {
                        s->hat_ph[h] += dub_note_inc(dub_clamp_midi(hat_base + h_offs[h]));
                        if (h_style == HAT_STYLE_SIZZLE || (h_style != HAT_STYLE_TONE && pop_hat_fm[kit] > 24u)) {
                            int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * (int32_t)pop_hat_fm[kit];
                            hm += dub_sine_q64(s->hat_ph[h] + ((uint32_t)hfm << 11));
                        } else if (h_style == HAT_STYLE_TONE) {
                            hm += dub_sine_q64(s->hat_ph[h]) + (dub_square(s->hat_ph[h] >> 1) >> 3);
                        } else {
                            hm += dub_square(s->hat_ph[h]);
                        }
                    }
                    hm >>= 2;
                    if (h_style == HAT_STYLE_TR6)
                        hm &= ~63;
                    if (h_style == HAT_STYLE_HISS)
                        hm >>= 1;
                    {
                        int32_t h_nz = (dub_noise(&s->noise) * nz_gain) >> 9;
                        int32_t sq = dub_square(s->hat_ph[0]) >> 8;

                        h_nz = (h_nz * (sq + 112)) >> 7;
                        hm = ((hm * met_w) >> 7) + h_nz;
                    }
                }
#elif defined(GENRE_HOUSE)
                {
                    static const int8_t h_offs_tr[4] = { 0, 4, 8, 13 };
                    static const int8_t h_offs_cr[4] = { 0, -7, 5, 12 };
                    const int8_t *h_offs = h_offs_tr;
                    uint8_t h_style = house_hat_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (kit == DKIT_CR78)
                        h_offs = h_offs_cr;
                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)house_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)house_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)house_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)house_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)house_hat_noise[kit];
                        met_w = (int32_t)house_hat_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    for (h = 0; h < 4u; h++) {
                        s->hat_ph[h] += dub_note_inc(dub_clamp_midi(hat_base + h_offs[h]));
                        if (h_style == HAT_STYLE_SIZZLE || (h_style != HAT_STYLE_TONE && house_hat_fm[kit] > 24u)) {
                            int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * (int32_t)house_hat_fm[kit];

                            hm += dub_sine_q64(s->hat_ph[h] + ((uint32_t)hfm << 11));
                        } else if (h_style == HAT_STYLE_TONE) {
                            hm += dub_sine_q64(s->hat_ph[h]) + (dub_square(s->hat_ph[h] >> 1) >> 3);
                        } else {
                            hm += dub_square(s->hat_ph[h]);
                        }
                    }
                    hm >>= 2;
                    if (h_style == HAT_STYLE_TR6)
                        hm &= ~63;
                    if (h_style == HAT_STYLE_HISS)
                        hm >>= 1;
                    hm = ((hm * met_w) >> 7) + ((dub_noise(&s->noise) * nz_gain) >> 8);
                }
#elif defined(GENRE_FUNK)
                {
                    int32_t nz_gain;
                    int32_t met_w;

                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)funk_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)funk_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)funk_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)funk_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)funk_hat_noise[kit];
                        met_w = (int32_t)funk_hat_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    if (s->hat_body > 8) {
                        /* hat_body is the 0..17 quarter-octave decay index: 9+ = open */
                        int32_t oh = (((int32_t)s->hat_body - 6) * 8 * (int32_t)funk_hat_open[kit]) / 3;

                        nz_gain += oh >> 6;
                        if (kit == DKIT_DISCO)
                            nz_gain += oh >> 5;
                        else if (kit == DKIT_BOOG)
                            nz_gain += oh >> 6;
                        if (nz_gain > 127)
                            nz_gain = 127;
                    }
                    hm = funk_hat_render(s, hat_base, kit, nz_gain, met_w);
                    (void)h;
                }
#else
                {
                    static const int8_t h_offs_tr[4] = { 0, 4, 8, 13 };
                    static const int8_t h_offs_cr[4] = { 0, -7, 5, 12 };
                    const int8_t *h_offs = h_offs_tr;
                    uint8_t h_style = dub_hat_style[kit];
                    int32_t nz_gain;
                    int32_t met_w;

                    if (kit == DKIT_CR78)
                        h_offs = h_offs_cr;
                    if (v->lane_gate[2] < 0) {
                        nz_gain = (int32_t)dub_hat_noise[kit] - (int32_t)v->lane_gate[2];
                        met_w = (int32_t)dub_hat_metal[kit] + 16;
                    } else if (v->lane_gate[2] > 0) {
                        nz_gain = (int32_t)dub_hat_noise[kit] + ((int32_t)v->lane_gate[2] * 3 >> 1);
                        met_w = (int32_t)dub_hat_metal[kit] - 14;
                    } else {
                        nz_gain = (int32_t)dub_hat_noise[kit];
                        met_w = (int32_t)dub_hat_metal[kit];
                    }
                    if (nz_gain < 8)
                        nz_gain = 8;
                    else if (nz_gain > 127)
                        nz_gain = 127;
                    if (met_w < 20)
                        met_w = 20;
                    else if (met_w > 127)
                        met_w = 127;
                    for (h = 0; h < 4u; h++) {
                        s->hat_ph[h] += dub_note_inc(dub_clamp_midi(hat_base + h_offs[h]));
                        if (h_style == DUB_HAT_SIZZLE || (h_style != DUB_HAT_TONE && dub_hat_fm[kit] > 24u)) {
                            int32_t hfm = dub_sine_q64(s->hat_ph[(h + 1u) & 3u]) * (int32_t)dub_hat_fm[kit];

                            hm += dub_sine_q64(s->hat_ph[h] + ((uint32_t)hfm << 11));
                        } else if (h_style == DUB_HAT_TONE) {
                            hm += dub_sine_q64(s->hat_ph[h]) + (dub_square(s->hat_ph[h] >> 1) >> 3);
                        } else {
                            hm += dub_square(s->hat_ph[h]);
                        }
                    }
                    hm >>= 2;
                    if (h_style == DUB_HAT_TR6)
                        hm &= ~63;
                    if (h_style == DUB_HAT_HISS)
                        hm >>= 1;
                    hm = ((hm * met_w) >> 7) + ((dub_noise(&s->noise) * nz_gain) >> 8);
                }
#endif
#if !defined(GENRE_JAZZ) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_IDM) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL) \
    && !defined(GENRE_TECHNO) && !defined(GENRE_BASS)
                hm = dub_hat_hp6k(s, hm);
#endif
                {
                    int64_t hs = ((int64_t)hm * s->hat_env * (int32_t)p->hat_lvl) >> DUB_HAT_LVL_SH;

                    if (hs > 32767)
                        hs = 32767;
                    else if (hs < -32767)
                        hs = -32767;
                    hat_sig = (int32_t)hs;
                }
                if (hat_drv > 0u)
                    hat_sig = dub_softclip_q15((hat_sig * (256 + (int32_t)hat_drv)) >> 8);
                /* Kit hat_cut tables were tuned for the old LP-only filter: only ¼ of their offset
                 * from 64 is applied, so kits sit on a ≥ ~11 kHz LP instead of 1.5–3.5 kHz. */
                {
                    int32_t hc_t = (int32_t)(p->drm_cut[2] ? p->drm_cut[2] : 64u) + (sizzle >> 1);

                    if (hc_t < 0)
                        hc_t = 0;
                    else if (hc_t > 127)
                        hc_t = 127;
                    hc = (uint8_t)hc_t;
                }
#ifdef GENRE_HEAVY_PSYCH
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)heavy_psych_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_JAZZ)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)jazz_ride_cut[kit] - 64) >> 2) + ((sizzle * 40) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_SOUL)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)soul_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)idm_hat_glitch_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_HIPHOP)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)hiphop_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_POP)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)pop_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_HOUSE)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)house_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#elif defined(GENRE_FUNK)
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)funk_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#else
                {
                int32_t hat_fc = (int32_t)hc + (((int32_t)dub_hat_cut[kit] - 64) >> 2) + ((sizzle * 28) >> 6);
                if (hat_fc < 8)
                    hat_fc = 8;
                else if (hat_fc > 127)
                    hat_fc = 127;
                dub_drm_svf(&hat_sig, (uint8_t)hat_fc, &s->hat_f_l1, &s->hat_f_l2);
                }
#endif
            } else if (s->hat_env > 0) {
                dub_hat_env_step(s);
            }
            drum_sig = kick_sig + rim_sig + hat_sig;
#ifdef GENRE_HEAVY_PSYCH
            {
                uint8_t kdrv = (uint8_t)(((int32_t)p->drm_drv[0] + (int32_t)p->drm_drv[1] + (int32_t)p->drm_drv[2]) / 3);

                drum_sig = heavy_psych_drm_bus(drum_sig, kdrv);
            }
#elif defined(GENRE_JAZZ)
            {
                uint8_t kdrv = (uint8_t)(((int32_t)p->drm_drv[0] + (int32_t)p->drm_drv[1] + (int32_t)p->drm_drv[2]) / 3);

                drum_sig = jazz_drm_bus(drum_sig, kdrv);
            }
#elif defined(GENRE_SOUL)
            {
                uint8_t kdrv = (uint8_t)(((int32_t)p->drm_drv[0] + (int32_t)p->drm_drv[1] + (int32_t)p->drm_drv[2]) / 3);

                drum_sig = soul_drm_bus(drum_sig, kdrv);
            }
#elif defined(GENRE_HIPHOP)
            {
                uint8_t kdrv = (uint8_t)(((int32_t)p->drm_drv[0] + (int32_t)p->drm_drv[1] + (int32_t)p->drm_drv[2]) / 3);

                drum_sig = hiphop_drm_bus(drum_sig, kdrv);
            }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            {
                uint8_t kdrv = (uint8_t)(((int32_t)p->drm_drv[0] + (int32_t)p->drm_drv[1] + (int32_t)p->drm_drv[2]) / 3);

                drum_sig = idm_drm_bus(drum_sig, kdrv);
            }
#endif
            dub_meter_peak(&s->meter[2], drum_sig);
        } else {
            if (s->kick_env > 0)
                dub_drm_env_step(&s->kick_env, &s->kick_snap, &s->kick_peak, &s->kick_atk_left,
                                 s->kick_body, 40u);
            if (s->rim_env > 0)
                dub_drm_env_step(&s->rim_env, &s->rim_snap, &s->rim_peak, &s->rim_atk_left,
                                 s->rim_body, 20u);
            if (s->hat_env > 0)
                dub_hat_env_step(s);
        }
#endif /* !GENRE_BASS */

#if !defined(GENRE_BASS)
        /* --- Siren (NJD-style dub siren: starts 1 octave lower; RATE sweeps 0.8 Hz wail .. 42 Hz laser trill) --- */
        {
#if defined(GENRE_HEAVY_PSYCH)
            heavy_psych_siren_env_step(s, p);
#elif defined(GENRE_JAZZ)
            jazz_siren_env_step(s, p);
#elif defined(GENRE_SOUL)
            soul_siren_env_step(s, p);
#elif defined(GENRE_HIPHOP)
            hiphop_siren_env_step(s, p);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            idm_siren_env_step(s, p);
#elif defined(GENRE_HOUSE)
            house_siren_env_step(s, p);
#elif defined(GENRE_FUNK)
            funk_siren_env_step(s, p);
#elif defined(GENRE_POP)
            pop_siren_env_step(s, p);
#else
            dub_siren_env_step(s, p);
#endif
        }
        if (!(v->mute & 8u) && s->siren_env > 0 && p->siren_pitch > 0u) {
#ifdef GENRE_HEAVY_PSYCH
            siren_sig = heavy_psych_lead_render(s, p, lfo_mod_c);
#elif defined(GENRE_JAZZ)
            siren_sig = jazz_lead_render(s, p, lfo_mod_c);
#elif defined(GENRE_SOUL)
            siren_sig = soul_lead_render(s, p, lfo_mod_c);
#elif defined(GENRE_HIPHOP)
            siren_sig = hiphop_lead_render(s, p, lfo_mod_c);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            siren_sig = idm_lead_render(s, p, lfo_mod_c);
#elif defined(GENRE_POP)
            siren_sig = pop_lead_render(s, p, lfo_mod_c);
            siren_sig = (siren_sig * (int32_t)p->siren_pitch) >> 7;
#elif defined(GENRE_HOUSE)
            int32_t raw_s;
            int32_t l_timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;

            raw_s = house_stab_sample(s, p, s->siren_env);
            siren_sig = (raw_s * (int32_t)p->siren_pitch) >> 7;
            {
                {
                    int32_t sc = (int32_t)p->drm_cut[3] + (l_timb >> 1);

                    if (sc < 88)
                        sc = 88;
                    else if (sc > 127)
                        sc = 127;
                    dub_siren_svf(&siren_sig, (uint8_t)sc, &s->siren_f_l1, &s->siren_f_l2);
                }
            }
#elif defined(GENRE_FUNK)
            int32_t raw_s;
            int32_t l_timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;

            raw_s = funk_stab_sample(s, p, s->siren_env);
            siren_sig = (raw_s * (int32_t)p->siren_pitch) >> 7;
            {
                int32_t sc = (int32_t)p->drm_cut[3] + (l_timb >> 1);

                if (sc < 88)
                    sc = 88;
                else if (sc > 127)
                    sc = 127;
                dub_siren_svf(&siren_sig, (uint8_t)sc, &s->siren_f_l1, &s->siren_f_l2);
            }
#else
            int32_t raw_s;
            int32_t l_timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;
            /* SIREN (p->siren_pitch) = volume/amplitude only; RATE (p->siren_rate) = pitch & wail speed */
            uint32_t r = (uint32_t)p->siren_rate;
            int32_t kt_s = ((int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64) * 24 / 64;
            int32_t sn = (int32_t)(p->root < 58u ? p->root : 58u) + ((int32_t)r * 30) / 127 +
                         kt_s + (int32_t)s->siren_st_tune;
            uint8_t s_note = dub_clamp_midi(sn);
            uint32_t base = dub_siren_bent_inc(s, dub_note_inc(s_note));
            uint32_t lfo_inc = 120000u + r * 22000u + r * r * 550u;
            int32_t lfo_s = dub_sine_q64(s->siren_lfo_ph);
            int32_t lfo_sq = (s->siren_lfo_ph & 0x80000000u) ? 28000 : -28000;
            int32_t lfo_w = r < 76u ? lfo_s : (((lfo_s * (127 - (int32_t)r)) + (lfo_sq * ((int32_t)r - 76))) / 51);
            int32_t mod = (int32_t)(base >> 15) * lfo_w;

            s->siren_lfo_ph += lfo_inc;
            if ((int32_t)base + mod < (int32_t)(base >> 2))
                mod = -(int32_t)(base >> 1);
            s->siren_ph += (uint32_t)((int32_t)base + mod);
            {
                int32_t sq_w = 128 + (l_timb * 3) / 4;
                int32_t si_w = 384 - (l_timb * 3) / 4;

                if (sq_w < 0)
                    sq_w = 0;
                if (si_w < 64)
                    si_w = 64;
                raw_s = ((dub_square(s->siren_ph) * sq_w) + (dub_sine_q64(s->siren_ph) * si_w)) >> 8;
            }
            siren_sig = (((raw_s * s->siren_env) >> 15) * (int32_t)p->siren_pitch) >> 7;
            {
                int32_t sc = (int32_t)p->drm_cut[3] + (l_timb >> 1);

                if (sc < 8)
                    sc = 8;
                else if (sc > 127)
                    sc = 127;
                dub_siren_svf(&siren_sig, (uint8_t)sc, &s->siren_f_l1, &s->siren_f_l2);
            }
#endif
            /* LEAD DRIVE (drm_drv[3]) + LEAD PUMP (pan[3], kick sidechain like BASS PUMP) */
            if (p->drm_drv[3] > 0u)
                siren_sig = dub_softclip_q15((siren_sig * (256 + (int32_t)p->drm_drv[3] * 4)) >> 8);
            if (p->pan[3] > 0u) {
                int32_t l_duck = ((int32_t)p->pan[3] * s->pump_env) >> 17;

                if (l_duck > 127)
                    l_duck = 127;
                siren_sig -= (siren_sig * l_duck) >> 7;
            }
            dub_meter_peak(&s->meter[3], siren_sig);
        }
#endif /* !GENRE_BASS */

        /* Pump ducking */
#if defined(GENRE_BASS)
        duck = 0;
#else
        s->pump_env -= s->pump_env >> 9;
        if (s->pump_env < 0)
            s->pump_env = 0;
        duck = ((int32_t)p->pump * s->pump_env) >> 17;
        if (duck > 127)
            duck = 127;
#ifdef GENRE_HEAVY_PSYCH
        duck = heavy_psych_chord_pump_duck(duck, p->era);
#endif
        ch_sig -= (ch_sig * duck) >> 7;
#endif

        {
            int32_t sub_dly = (int32_t)p->dly_b;
            int32_t ch_dly = (int32_t)p->dly_c;
            int32_t kck_dly = (int32_t)p->drm_dly[0];
            int32_t rim_dly = (int32_t)p->drm_dly[1];
            int32_t hat_dly = (int32_t)p->drm_dly[2];
            int32_t sir_dly = (int32_t)p->drm_dly[3];
            int32_t hp_send;

#ifdef GENRE_HEAVY_PSYCH
            /* Heavy Psych drums never feed dub delay / tape echo; only lead (siren) uses delay */
            kck_dly = 0;
            rim_dly = 0;
            hat_dly = 0;
            sir_dly += (int32_t)heavy_psych_lead_dly_boost(p);
#elif defined(GENRE_JAZZ)
            kck_dly = 0;
            rim_dly = 0;
            hat_dly = 0;
            sir_dly += jazz_lead_dly_boost(p);
#elif defined(GENRE_SOUL)
            kck_dly = 0;
            rim_dly = 0;
            hat_dly = 0;
            sir_dly += soul_lead_dly_boost(p);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            kck_dly = 0;
            rim_dly = 0;
            hat_dly = 0;
            sir_dly += idm_lead_dly_boost(p);
#endif
            if (sir_dly > 140)
                sir_dly = 140;
            /* Per-lane momentary delay throws (`lane_throw_mask` bits 0..5 = K, R, H, S, B, C) */
#if !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_SOUL) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS)
            if ((v->lane_throw_mask & 1u) || (v->throw & 4u))
                kck_dly = 140;
            if ((v->lane_throw_mask & 2u) || (v->throw & 4u))
                rim_dly = 140;
            if ((v->lane_throw_mask & 4u) || (v->throw & 4u))
                hat_dly = 140;
#endif
            if ((v->lane_throw_mask & 8u) || (v->throw & 8u))
                sir_dly = 140;
            if ((v->lane_throw_mask & 16u) || (v->throw & 1u))
                sub_dly = 140;
            if ((v->lane_throw_mask & 32u) || (v->throw & 2u))
                ch_dly = 140;

            hp_send = sub_sig;
            s->sub_send_hp += (hp_send - s->sub_send_hp) >> 8;
            hp_send -= s->sub_send_hp >> 8;
            dly_in += (hp_send * sub_dly) >> 7;
            dly_in += (ch_sig * ch_dly) >> 7;
            dly_in += (kick_sig * kck_dly) >> 7;
            dly_in += (rim_sig * rim_dly) >> 7;
            dly_in += (hat_sig * hat_dly) >> 7;
            dly_in += (siren_sig * sir_dly) >> 7;

            rev_send += (sub_sig * (int32_t)p->rev_b) >> 7;
#ifdef GENRE_HEAVY_PSYCH
            rev_send += (ch_sig * (int32_t)heavy_psych_chord_rev_send(p->era, p->rev_c)) >> 7;
            rev_send += heavy_psych_drm_room_send(kick_sig, rim_sig, hat_sig);
#elif defined(GENRE_JAZZ)
            rev_send += (ch_sig * (int32_t)p->rev_c) >> 7;
            rev_send += jazz_drm_room_send(kick_sig, rim_sig, hat_sig);
#elif defined(GENRE_SOUL)
            rev_send += (ch_sig * (int32_t)p->rev_c) >> 7;
            rev_send += soul_drm_room_send(kick_sig, rim_sig, hat_sig);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            rev_send += (ch_sig * (int32_t)p->rev_c) >> 7;
            rev_send += idm_drm_room_send(kick_sig, rim_sig, hat_sig);
#else
            rev_send += (ch_sig * (int32_t)p->rev_c) >> 7;
            rev_send += (kick_sig * (int32_t)p->drm_rev[0]) >> 7;
            rev_send += (rim_sig * (int32_t)p->drm_rev[1]) >> 7;
            rev_send += (hat_sig * (int32_t)p->drm_rev[2]) >> 7;
#endif
            rev_send += (siren_sig * (int32_t)p->drm_rev[3]) >> 7;
        }

        /* --- Delay (RE-201 multi-head modes by complexity: SINGLE, DUAL, GALLOP, TRIP, SPREAD, PING) --- */
        {
            int32_t wow_dummy = 0;
            int32_t wow_tri = dub_lfo_val(DUB_LFO_TRI, (int32_t)(s->wow_ph >> 2), &wow_dummy);
            int32_t wow_smp = (wow_tri * (int32_t)p->wow) >> 11;
            uint32_t dly_s;
            uint32_t dly_b;
            {
                int32_t dly_tgt = (int32_t)dly_base;
                int32_t dly_cur = (int32_t)s->dly_len_sm;

                if (dly_cur == 0)
                    dly_cur = dly_tgt;
                else {
                    int32_t dd = dly_tgt - dly_cur;

                    if (dd > 12)
                        dd = 12;
                    else if (dd < -12)
                        dd = -12;
                    dly_cur += dd;
                }
                if (dly_cur < 8)
                    dly_cur = 8;
                s->dly_len_sm = (uint32_t)dly_cur;
                dly_s = (uint32_t)dly_cur;
            }
            dly_b = dly_s;
            int32_t tap_a;
            int32_t tap_b;
            int32_t fb_gain;
            int32_t wet_l = 0;
            int32_t wet_r = 0;
            uint8_t head_mode = p->tap % TAP_N;

            s->wow_ph += 900u + (uint32_t)p->wow * 20u;
            s->dly_half_toggle = (uint8_t)((s->dly_half_toggle + 1u) % dly_half_rate);

            /* Position of 2nd tape head (Tap B) relative to main head (Tap A = dly_s) */
            switch (head_mode) {
            case 1u: /* DUAL: Head 1 (1/2) + Head 2 (1/1) */
                dly_b = dly_s >> 1;
                break;
            case 2u: /* GALLOP: Head 1 (3/4 dotted) + Head 2 (1/1) */
                dly_b = (dly_s * 3u) >> 2;
                break;
            case 3u: /* TRIP: Head 1 (2/3 triplet) + Head 2 (1/1) */
                dly_b = (dly_s * 2u) / 3u;
                break;
            case 4u: /* SPREAD: Haas stereo head offset */
                dly_b = dly_s + (uint32_t)tap_b_ofs;
                break;
            case 5u: /* PING: true L/R alternating stereo ping-pong (L = 1/2, R = 1/1 loop) */
                dly_b = dly_s >> 1;
                break;
            default: /* SINGLE: single head only */
                dly_b = dly_s;
                break;
            }
            if (dly_b < 8u)
                dly_b = 8u;
            if (dly_b > (DUB_DLY_LEN - 64u))
                dly_b = DUB_DLY_LEN - 64u;

            tap_a = dub_dly_read_interp(s->dly_w, dly_s, wow_smp);
            tap_b = dub_dly_read_interp(s->dly_w, dly_b, -wow_smp);
            if (head_mode == 0u) {
                /* SINGLE */
                wet_l = tap_a;
                wet_r = tap_a;
            } else if (head_mode == 4u) {
                /* SPREAD: true stereo heads */
                wet_l = tap_a;
                wet_r = tap_b;
            } else if (head_mode == 5u) {
                /* PING: 1st bounce T/2 -> L, T -> R (feedback), alternating */
                wet_l = tap_b;
                wet_r = tap_a;
            } else {
                /* DUAL / GALLOP / TRIP: both heads active with slight stereo offset */
                wet_l = tap_a + (tap_b >> 1);
                wet_r = (tap_a >> 1) + tap_b;
            }

            /* Feedback: 106 (100%) = unity sustain; above that taper toward ~0.90 Q15 (no runaway / hiss) */
            {
                /* Feedback LFO + macro deltas already applied in sync_voice_params() → g.voice.p.feedback */
                int32_t fb_eff = (int32_t)p->feedback;
                if (fb_eff < 0)
                    fb_eff = 0;
                if (fb_eff > 127)
                    fb_eff = 127;
                if (fb_eff <= 106) {
                    fb_gain = (fb_eff * 32767) / 106;
                } else {
                    fb_gain = 32767 + ((fb_eff - 106) * (29490 - 32767)) / 21;
                }
                if (v->freeze)
                    fb_gain = 29490;
            }

            if (!v->freeze) {
                int32_t path = tap_a;
                int32_t lf_tgt = (int32_t)p->loop_filt;
                int32_t lf;
                int32_t fb_eff = (int32_t)p->feedback;
                int32_t tilt;
                int32_t ld;

                if (lf_tgt < 0)
                    lf_tgt = 0;
                else if (lf_tgt > 127)
                    lf_tgt = 127;
                lf = s->dly_lf_sm;
                ld = lf_tgt - lf;
                if (ld > 2)
                    ld = 2;
                else if (ld < -2)
                    ld = -2;
                lf += ld;
                s->dly_lf_sm = lf;
                tilt = lf - 64;

                /* DC block in feedback loop (stops offset / digit noise buildup) */
                s->dly_loop_dc += (path - s->dly_loop_dc) >> 9;
                path -= s->dly_loop_dc;

                /* LOOP tilt filter inside feedback loop:
                 * tilt < 0 (LP): progressive warm lowpass darkening each echo repeat
                 * tilt > 0 (HP): progressive King-Tubby highpass thinning out low end */
                if (tilt < 0) {
                    int32_t lp_a = 24 + ((64 + tilt) * 200) / 64;
                    s->dly_lp += ((path - s->dly_lp) * lp_a) >> 8;
                    path = s->dly_lp;
                    if (fb_eff > 96 && fb_eff <= 106)
                        path = (path * (256 + ((-tilt) * 72) / 64)) >> 8;
                } else if (tilt > 0) {
                    int32_t hp_a = 8 + (tilt * 180) / 63;
                    s->dly_hp += ((path - s->dly_hp) * hp_a) >> 8;
                    path = path - s->dly_hp;
                    if (fb_eff > 96 && fb_eff <= 106)
                        path = (path * (256 + (tilt * 72) / 63)) >> 8;
                } else {
                    s->dly_lp += (path - s->dly_lp) >> 1;
                    path = s->dly_lp;
                }
                {
                    if (path > 32767)
                        path = 32767;
                    else if (path < -32767)
                        path = -32767;
                    path = dly_in + ((path * fb_gain) >> 15);
                    if (path > 65000)
                        path = 65000;
                    else if (path < -65000)
                        path = -65000;
                }
                {
                    int32_t clean = dub_softclip_q15(path);

                    if (fb_eff >= DUB_FB_ECHO_DIST_START) {
                        int32_t fb_c = fb_eff;
                        int32_t sat, a, delta;

                        if (fb_c > 127)
                            fb_c = 127;
                        sat = dub_echo_sat_q15(path);
                        {
                            int32_t t = fb_c - DUB_FB_ECHO_DIST_START;
                            int32_t u = DUB_FB_ECHO_DIST_SPAN - t;
                            int32_t span_sq = DUB_FB_ECHO_DIST_SPAN * DUB_FB_ECHO_DIST_SPAN;

                            a = (DUB_FB_ECHO_DIST_AMT * (span_sq - u * u)) / span_sq;
                        }
                        delta = sat - clean;
                        path = clean + ((delta * a) >> 15);
                        if (path > 32767)
                            path = 32767;
                        else if (path < -32767)
                            path = -32767;
                    } else {
                        path = clean;
                    }
                }
                if (s->dly_half_toggle == 0u) {
                    dub_dly_pool[s->dly_w] = (int16_t)path;
                    s->dly_w = (s->dly_w + 1u) % DUB_DLY_LEN;
                }
            } else {
                int32_t path = dub_softclip_q15(tap_a);
                path = ((path * fb_gain) >> 15);
                if (s->dly_half_toggle == 0u) {
                    dub_dly_pool[s->dly_w] = (int16_t)dub_softclip_q15(path);
                    s->dly_w = (s->dly_w + 1u) % DUB_DLY_LEN;
                }
            }

            if (p->dly_time == 0u && !v->freeze && !v->throw && !v->lane_throw_mask) {
                dly_l = 0;
                dly_r = 0;
            } else {
                int32_t dlvl = (int32_t)p->dly_lvl;
                int32_t er_s = (int32_t)p->dly_rev;
                dly_l = (wet_l * dlvl) >> 7;
                dly_r = (wet_r * dlvl) >> 7;
                /* Feed attenuated delay output into reverb via E->REV (`dly_rev`) */
                rev_send += (((dly_l + dly_r) >> 2) * er_s) >> 7;
            }
        }

        /* --- Reverb --- */
        {
            int32_t rev_wet = 0;
            int32_t rev_in = rev_send;
            int32_t rev_mix = (int32_t)p->rev_mix + (lfo_mod_rev >> 8);
            int32_t fx_master = (int32_t)p->dly_lvl;
            /* DAMP (`rev_damp`): 0 = bright splashy spring, 127 = dark muffled dub chamber */
            int32_t damp = 248 - ((int32_t)p->rev_damp * 180) / 127;
            /* TAIL (`rev_tail`): scales tank decay feedback inside the 4 delay lines */
            int32_t tail_g = 110 + ((int32_t)p->rev_tail * 130) / 127;

            if (rev_mix < 0)
                rev_mix = 0;
            else if (rev_mix > 127)
                rev_mix = 127;

            if (s->crash_left > 0u) {
                rev_in += (dub_noise(&s->noise) * s->crash_env) >> 15;
                s->crash_env -= s->crash_env >> 4;
                s->crash_left--;
            }

            rev_in -= (rev_in * duck) >> 7;

            if (p->tank < 64u) {
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_CH0_OFF], DUB_REV_CH0_LEN, &s->rev_ch_pos[0], rev_in);
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_CH1_OFF], DUB_REV_CH1_LEN, &s->rev_ch_pos[1], rev_in);
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_CH2_OFF], DUB_REV_CH2_LEN, &s->rev_ch_pos[2], rev_in);
                rev_wet = dub_tank_step(&dub_rev_pool[DUB_REV_T0_OFF], DUB_REV_T0_LEN, &s->rev_t_pos[0], rev_in,
                                          &s->rev_t_lp[0], damp);
            } else {
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_AP0_OFF], DUB_REV_AP0_LEN, &s->rev_ap_pos[0], rev_in);
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_AP1_OFF], DUB_REV_AP1_LEN, &s->rev_ap_pos[1], rev_in);
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_AP2_OFF], DUB_REV_AP2_LEN, &s->rev_ap_pos[2], rev_in);
                rev_in = dub_ap_pass(&dub_rev_pool[DUB_REV_AP3_OFF], DUB_REV_AP3_LEN, &s->rev_ap_pos[3], rev_in);
                {
                    int32_t t0 = dub_tank_step(&dub_rev_pool[DUB_REV_T0_OFF], DUB_REV_T0_LEN, &s->rev_t_pos[0],
                                               rev_in, &s->rev_t_lp[0], damp);
                    int32_t t1 = dub_tank_step(&dub_rev_pool[DUB_REV_T1_OFF], DUB_REV_T1_LEN, &s->rev_t_pos[1],
                                               rev_in + ((t0 * tail_g) >> 8), &s->rev_t_lp[1], damp);
                    int32_t t2 = dub_tank_step(&dub_rev_pool[DUB_REV_T2_OFF], DUB_REV_T2_LEN, &s->rev_t_pos[2],
                                               rev_in + ((t1 * tail_g) >> 8), &s->rev_t_lp[2], damp);
                    int32_t t3 = dub_tank_step(&dub_rev_pool[DUB_REV_T3_OFF], DUB_REV_T3_LEN, &s->rev_t_pos[3],
                                               rev_in + ((t2 * tail_g) >> 8), &s->rev_t_lp[3], damp);

                    rev_wet = (t0 + t1 + t2 + t3) >> 2;
                }
            }
            /* Scale reverb return by both REVERB (`rev_mix`) and master FX MIX (`fx_master`) so MIX=0 is 100% dry! */
            rev_wet = (((rev_wet * rev_mix) >> 7) * fx_master) >> 7;
            dly_l += rev_wet;
            dly_r += rev_wet;
        }

        /* --- Per-voice stereo pan + master iso + sum (RYTM pan law: pan_l = 127 - pan, pan_r = pan) --- */
        {
            int32_t sum_l = dly_l;
            int32_t sum_r = dly_r;
            int32_t p_sub = (int32_t)(p->pan[0] == 0u ? 64u : p->pan[0]);
            int32_t p_chd = (int32_t)(p->pan[1] == 0u ? 64u : p->pan[1]);
            int32_t p_kck = (int32_t)(p->drm_pan[0] == 0u ? 64u : p->drm_pan[0]);
            int32_t p_rim = (int32_t)(p->drm_pan[1] == 0u ? 74u : p->drm_pan[1]);
            int32_t p_hat = (int32_t)(p->drm_pan[2] == 0u ? 52u : p->drm_pan[2]);
            int32_t p_srn = (int32_t)(p->drm_pan[3] == 0u ? 82u : p->drm_pan[3]);
            int32_t iso = ((int32_t)p->isolator - 64) + (lfo_mod_iso >> 8);

            sum_l += ((sub_sig * (127 - p_sub)) + (ch_sig * (127 - p_chd)) +
                      (kick_sig * (127 - p_kck)) + (rim_sig * (127 - p_rim)) +
                      (hat_sig * (127 - p_hat)) + (siren_sig * (127 - p_srn))) >> 6;
            sum_r += ((sub_sig * p_sub) + (ch_sig * p_chd) +
                      (kick_sig * p_kck) + (rim_sig * p_rim) +
                      (hat_sig * p_hat) + (siren_sig * p_srn)) >> 6;
            if (p->drive > 0u) {
                sum_l = (sum_l * (256 + (int32_t)p->drive)) >> 8;
                sum_r = (sum_r * (256 + (int32_t)p->drive)) >> 8;
            }
            if (iso < -64)
                iso = -64;
            else if (iso > 63)
                iso = 63;
            /* Musical DJ isolator: LP keeps warm sub floor (never mutes to 0); HP shelves bass gently (c <= 18) */
            if (iso < 0) {
                int32_t c = 10 + ((64 + iso) * 54) / 64;
                s->iso_lp += ((sum_l - s->iso_lp) * c) >> 6;
                s->iso_lp_r += ((sum_r - s->iso_lp_r) * c) >> 6;
                sum_l = ((s->iso_lp * 3) + sum_l) >> 2;
                sum_r = ((s->iso_lp_r * 3) + sum_r) >> 2;
            } else if (iso > 0) {
                int32_t c = 1 + (iso * 17) / 63;
                s->iso_hp += ((sum_l - s->iso_hp) * c) >> 6;
                s->iso_hp_r += ((sum_r - s->iso_hp_r) * c) >> 6;
                sum_l = sum_l - ((s->iso_hp * 3) >> 2);
                sum_r = sum_r - ((s->iso_hp_r * 3) >> 2);
            }
            sum_l = dub_master_soft(sum_l);
            sum_r = dub_master_soft(sum_r);
            out[2u * i] += sum_l;
            out[2u * i + 1u] += sum_r;
        }
    }
}

#endif /* DUB_DSP_H */
