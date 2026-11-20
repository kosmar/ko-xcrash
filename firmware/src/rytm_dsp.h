/* SPDX-License-Identifier: GPL-3.0-only */
/* KO RYTM drum + bass + dub FX engine (Q15 integer, freestanding). */
#ifndef RYTM_DSP_H
#define RYTM_DSP_H

#include "rytm_types.h"

#define RYTM_FS         44100u
#define RYTM_DLY_LEN    4096u
#define RYTM_EVT_MAX    24u
#define RYTM_WG_LEN     384u
#define RYTM_REV_TAPS   4u
#define RYTM_REV_BUF    702u /* sum of TAP_LEN in rytm_synth_render */
#define RYTM_HAT_CHOKE  132u /* ~3 ms @ 44.1 kHz */

#if defined(DINGHY_HOST) || defined(DINGHY_WEB)
static int16_t rytm_dly_pool[RYTM_DLY_LEN];
#else
static int16_t rytm_dly_pool[RYTM_DLY_LEN] __attribute__((section(".pool")));
#endif

typedef struct {
    uint32_t left;
    uint8_t voice;
    uint8_t vel;
    uint8_t accent;
    uint8_t live;
    int8_t tune_off;
    int8_t dec_off;
} rytm_evt_t;

typedef struct {
    uint8_t active;
    uint8_t vel;
    uint8_t accent;
    uint32_t ph;
    uint32_t mph;
    int32_t env;
    int32_t env2;
    int32_t pitch;
    int32_t mod_env;
    int32_t bp1;
    int32_t bp2;
    int32_t hp;
    int32_t shell1;
    int32_t shell2;
    uint32_t nph;
    uint32_t hat_ph[6];
    int32_t hat_env;
    int32_t oh_ring;
    int32_t clap_env[4];
    uint8_t clap_idx;
    int32_t click;
    int8_t tune_off; /* per-step offsets snapshotted from the event */
    int8_t dec_off;
} rytm_drum_v_t;

typedef struct {
    uint8_t on;
    uint8_t note;
    uint8_t target_note;
    uint8_t model;
    uint8_t accent;
    uint8_t dead_ghost;
    uint8_t legato;
    uint32_t ph;
    uint32_t ph2;
    uint32_t sub_ph;
    uint32_t cur_inc;
    uint32_t tgt_inc;
    int32_t amp;
    int32_t env;
    int32_t filt_env;
    int32_t svf_l1;
    int32_t svf_l2;
    int32_t svf_b1;
    int32_t svf_b2;
    uint32_t m_ph;
    int32_t fm_fb;
    int32_t gate_left;
    int16_t wg[RYTM_WG_LEN];
    uint32_t wg_w;
} rytm_bass_v_t;

typedef struct {
    rytm_drum_v_t dv[RYTM_NDVOICES];
    rytm_bass_v_t bass;
    rytm_bass_v_t bass_prev;
    uint8_t prev_model;
    uint8_t model_xf; /* 0 = previous model, 64 = current */
    rytm_evt_t evt[RYTM_EVT_MAX];
    rytm_bass_hit_t bass_pend[2];
    rytm_bass_cfg_t bass_pend_cfg[2];
    uint32_t bass_pend_left[2];
    uint8_t bass_pend_live[2];
    int16_t rev_buf[RYTM_REV_BUF];
    uint8_t rev_pos[RYTM_REV_TAPS];
    uint32_t dly_w;
    int32_t dly_lp;
    int32_t dly_hp;
    int32_t dly_tap;
    int32_t dly_tap_prev;
    int32_t sidechain;
    int32_t sidechain_tgt;
    uint32_t wow_ph;
    uint32_t flut_ph;
    uint8_t voice_peak[RYTM_NDVOICES];
    uint8_t bass_peak;
    uint8_t harmonic_toms;
    uint8_t root_note;
    uint8_t macro_punch;
    uint8_t macro_sweep;
    uint8_t macro_throw;
    uint8_t macro_hype;
    int32_t master_lp;
    int32_t master_lp_r;
} rytm_synth_t;

static int32_t rytm_sine(uint32_t ph)
{
    int32_t x = (int32_t)(int16_t)(ph >> 16);
    int32_t ax = x < 0 ? -x : x;
    int32_t y = (x * (32768 - ax)) >> 13;
    int32_t ay = y < 0 ? -y : y;

    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    if (y > 32767)
        return 32767;
    if (y < -32767)
        return -32767;
    return y;
}

static int32_t rytm_saw(uint32_t ph)
{
    int32_t x = (int32_t)(int16_t)(ph >> 16);

    return x >> 1;
}

#define RYTM_CLIP_KNEE 20480
#define RYTM_CLIP_R    (32767 - RYTM_CLIP_KNEE)

/* Linear up to the knee, then 32767 - R^2/(d + R): slope 1 at the knee, monotone,
 * approaches full scale asymptotically (101k in -> ~31.1k out). */
static int32_t rytm_softclip(int32_t x, uint32_t drive)
{
    int64_t y = ((int64_t)x * (256 + ((int32_t)drive << 1))) >> 8;
    int32_t ax;

    if (y > (1 << 30))
        y = 1 << 30;
    else if (y < -(1 << 30))
        y = -(1 << 30);
    ax = (int32_t)(y < 0 ? -y : y);
    if (ax <= RYTM_CLIP_KNEE)
        return (int32_t)y;
    ax = 32767 - (RYTM_CLIP_R * RYTM_CLIP_R) / (ax - RYTM_CLIP_KNEE + RYTM_CLIP_R);
    return y < 0 ? -ax : ax;
}

static int32_t rytm_apply_level(int32_t out, uint8_t level)
{
    return (out * (int32_t)level * (int32_t)level) >> 13;
}

static int32_t rytm_fold(int32_t x, uint32_t drive)
{
    int32_t lim;
    int32_t i;

    if (drive <= 8u)
        return rytm_softclip(x, drive);
    lim = 26000 - (int32_t)drive * 155;
    if (lim < 5500)
        lim = 5500;
    for (i = 0; i < 3; i++) {
        if (x > lim)
            x = lim - (x - lim);
        else if (x < -lim)
            x = -lim - (x + lim);
    }
    return rytm_softclip(x, drive >> 1);
}

static uint32_t rytm_note_inc(uint8_t note)
{
    static const uint32_t inc_c9[12] = {
        815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
        1153098554u, 1221665363u, 1294309365u, 1371273005u, 1452813141u, 1539201906u};

    if (note > 127u)
        note = 127u;
    return inc_c9[note % 12u] >> (10u - note / 12u);
}

static int32_t rytm_noise(uint32_t *st)
{
    uint32_t x = *st;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *st = x;
    return (int32_t)(x >> 16);
}

/* A bare >> shift freezes once the value is smaller than the shift. The voice
 * then stays active and keeps hashing. Always step at least 1 toward zero. */
static int32_t rytm_decay(int32_t v, uint32_t sh)
{
    int32_t d;

    if (v <= 0)
        return 0;
    if (sh > 20u)
        sh = 20u;
    d = v >> sh;
    if (d < 1)
        d = 1;
    v -= d;
    return v > 0 ? v : 0;
}

static void rytm_svf_lp(int32_t in, int32_t fc_q15, int32_t res_q15, int32_t *l1, int32_t *l2, int32_t *out)
{
    int32_t g = fc_q15 >> 7;
    int32_t damp = 220 - (res_q15 >> 7);
    int32_t low = *l2;
    int32_t band = *l1;
    int32_t high;

    if (g < 2)
        g = 2;
    if (g > 240)
        g = 240;
    if (damp < 16)
        damp = 16;
    high = in - low - ((band * damp) >> 8);
    band += (high * g) >> 8;
    low += (band * g) >> 8;
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
    *out = low;
}

static void rytm_peak_meter(uint8_t *pk, int32_t sig)
{
    int32_t ax = sig < 0 ? -sig : sig;
    int32_t v = ax >> 6;

    if (v > 255)
        v = 255;
    if ((uint8_t)v > *pk)
        *pk = (uint8_t)v;
}

static void rytm_synth_init(rytm_synth_t *s)
{
    uint32_t i, v;

    for (v = 0; v < RYTM_NDVOICES; v++) {
        s->dv[v].active = 0u;
        s->dv[v].env = 0;
        s->dv[v].env2 = 0;
        s->dv[v].hat_env = 0;
        s->dv[v].oh_ring = 0;
        s->dv[v].tune_off = 0;
        s->dv[v].dec_off = 0;
    }
    s->bass.on = 0u;
    s->bass.amp = 0;
    s->bass.env = 0;
    s->bass.gate_left = 0;
    s->bass_prev.on = 0u;
    s->bass_prev.amp = 0;
    s->model_xf = 64u;
    s->prev_model = 0u;
    for (i = 0; i < RYTM_EVT_MAX; i++)
        s->evt[i].live = 0u;
    s->bass_pend_live[0] = 0u;
    s->bass_pend_live[1] = 0u;
    for (i = 0; i < RYTM_DLY_LEN; i++)
        rytm_dly_pool[i] = 0;
    for (i = 0; i < RYTM_REV_TAPS; i++)
        s->rev_pos[i] = 0u;
    for (i = 0; i < RYTM_REV_BUF; i++)
        s->rev_buf[i] = 0;
    s->dly_w = 0u;
    s->dly_lp = 0;
    s->dly_hp = 0;
    s->dly_tap = 0;
    s->dly_tap_prev = 0;
    s->sidechain = 0;
    s->sidechain_tgt = 0;
    s->wow_ph = 0u;
    s->flut_ph = 0u;
    for (v = 0; v < RYTM_NDVOICES; v++)
        s->voice_peak[v] = 0u;
    s->bass_peak = 0u;
    s->harmonic_toms = 0u;
    s->root_note = 0u;
}

/* Drummer kit bases: tune, decay, tone, drive, level, delay-send. Knobs are offsets around 64. */
static const uint8_t RYTM_KIT_BASE[RYTM_NDRUMMERS][RYTM_NDVOICES][6] = {
    { {58, 70, 62, 40, 120, 18}, {62, 78, 80, 36, 108, 40}, {70, 36, 58, 30, 88, 16}, {66, 90, 54, 28, 82, 22}, {52, 84, 60, 34, 100, 28}, {74, 76, 64, 32, 96, 26}, {60, 70, 58, 30, 96, 36} }, /* Elvin acoustic */
    { {60, 68, 66, 44, 124, 12}, {64, 64, 74, 42, 114, 28}, {74, 32, 62, 34, 92, 10}, {70, 80, 58, 32, 86, 16}, {56, 72, 64, 38, 96, 18}, {78, 66, 68, 36, 92, 16}, {64, 58, 62, 36, 100, 24} }, /* Purdie */
    { {62, 74, 70, 50, 127, 8}, {66, 60, 78, 48, 116, 20}, {78, 28, 70, 40, 94, 6}, {72, 70, 66, 36, 84, 10}, {58, 68, 68, 42, 90, 12}, {80, 62, 72, 40, 88, 10}, {66, 64, 70, 44, 104, 18} }, /* Clyde */
    { {68, 48, 90, 70, 120, 4}, {72, 42, 96, 64, 110, 8}, {84, 22, 100, 58, 96, 2}, {80, 40, 94, 52, 80, 4}, {64, 44, 88, 60, 92, 6}, {88, 40, 92, 56, 88, 4}, {74, 36, 98, 62, 100, 6} }, /* Jaki FM */
    { {48, 96, 58, 46, 127, 20}, {58, 52, 84, 55, 118, 90}, {72, 30, 74, 40, 86, 24}, {68, 64, 70, 36, 78, 30}, {50, 80, 62, 40, 100, 40}, {70, 70, 66, 38, 96, 36}, {56, 48, 80, 50, 110, 100} }, /* Sly dub */
    { {56, 64, 60, 38, 118, 14}, {60, 58, 88, 40, 108, 18}, {66, 48, 55, 28, 100, 8}, {62, 86, 50, 26, 90, 12}, {48, 90, 58, 32, 112, 16}, {68, 82, 62, 30, 108, 14}, {58, 54, 64, 34, 96, 20} }, /* Allen */
    { {42, 110, 72, 62, 127, 6}, {70, 46, 92, 68, 112, 16}, {86, 18, 110, 60, 98, 4}, {82, 100, 104, 54, 90, 8}, {54, 60, 80, 50, 88, 8}, {76, 54, 84, 48, 84, 6}, {68, 96, 90, 58, 108, 22} }, /* Pretty Tony 808 */
    { {52, 40, 48, 55, 122, 10}, {58, 36, 60, 58, 114, 22}, {70, 20, 50, 48, 90, 8}, {66, 44, 46, 44, 80, 12}, {46, 42, 44, 50, 94, 14}, {64, 38, 48, 48, 90, 12}, {54, 34, 52, 56, 102, 28} }  /* Dilla lofi */
};

static const int8_t RYTM_GENRE_TWIST[RYTM_NGENRES][3] = {
    {0, 8, 28}, {4, 0, 6}, {2, -4, 4}, {0, -8, 0}, {-6, 6, 10}, {-4, 4, 8}, {12, -10, 2}, {6, -2, 10}, {-10, 16, 18}
};

static uint8_t rytm_clamp127(int32_t v)
{
    if (v < 0)
        return 0u;
    if (v > 127)
        return 127u;
    return (uint8_t)v;
}

static uint8_t rytm_scale_kit(int32_t base, uint8_t user)
{
    if (base < 0)
        base = 0;
    if (base > 127)
        base = 127;
    if (user <= 64u)
        return (uint8_t)((base * (int32_t)user) / 64);
    return (uint8_t)(base + ((127 - base) * ((int32_t)user - 64)) / 63);
}

static const uint8_t RYTM_DEF_PAN[RYTM_NDVOICES] = {64u, 64u, 52u, 76u, 42u, 86u, 72u};

static void rytm_kit_effective(uint8_t drummer, uint8_t genre, const rytm_dvoice_cfg_t *user, rytm_dvoice_cfg_t *out)
{
    uint8_t gi = (uint8_t)(((uint16_t)genre * (RYTM_NGENRES - 1u)) / 127u);
    int8_t tw_tune = RYTM_GENRE_TWIST[gi][0];
    int8_t tw_dec = RYTM_GENRE_TWIST[gi][1];
    int8_t tw_send = RYTM_GENRE_TWIST[gi][2];
    uint32_t v;

    if (drummer >= RYTM_NDRUMMERS)
        drummer = 0u;
    for (v = 0; v < RYTM_NDVOICES; v++) {
        const uint8_t *b = RYTM_KIT_BASE[drummer][v];
        int32_t tune_tw = (v == DV_SNARE || v == DV_CH || v == DV_OH) ? tw_tune : tw_tune / 2;
        int32_t dec_tw = (v == DV_KICK || v == DV_OH) ? tw_dec : tw_dec / 2;
        int32_t send_tw = (v == DV_SNARE || v == DV_CLAP) ? tw_send : tw_send / 3;

        out[v].tune = rytm_scale_kit((int32_t)b[0] + tune_tw, user[v].tune);
        out[v].decay = rytm_scale_kit((int32_t)b[1] + dec_tw, user[v].decay);
        out[v].tone = rytm_scale_kit((int32_t)b[2], user[v].tone);
        out[v].drive = rytm_scale_kit((int32_t)b[3] + (gi == 2u || gi == 3u ? 12 : 0), user[v].drive);
        out[v].level = rytm_scale_kit((int32_t)b[4], user[v].level);
        out[v].send_dly = rytm_scale_kit((int32_t)b[5] + send_tw, user[v].send_dly);
        out[v].send_rev = user[v].send_rev;
        out[v].pan = rytm_clamp127(user[v].pan);
        out[v].mute = user[v].mute;
    }
}

static void rytm_synth_preset_defaults(rytm_dvoice_cfg_t dv[RYTM_NDVOICES], rytm_bass_cfg_t *bc, rytm_fx_cfg_t *fx)
{
    uint32_t v;

    for (v = 0; v < RYTM_NDVOICES; v++) {
        dv[v].tune = 64u;
        dv[v].decay = 64u;
        dv[v].tone = 64u;
        dv[v].drive = 64u;
        dv[v].level = 64u;
        dv[v].send_dly = 64u;
        dv[v].send_rev = (v == DV_SNARE || v == DV_CLAP || v == DV_OH) ? 56u : 24u;
        dv[v].pan = RYTM_DEF_PAN[v];
        dv[v].mute = 0u;
    }
    bc->model = BMODEL_SOLID_FM;
    bc->cutoff = 72u;
    bc->reso = 48u;
    bc->env_mod = 64u;
    bc->decay = 88u;
    bc->drive = 40u;
    bc->glide = 24u;
    bc->sub_lvl = 72u;
    fx->dly_time = 64u;
    fx->dly_fb = 72u;
    fx->dly_mix = 48u;
    fx->rev_mix = 36u;
    fx->pump = 64u;
    fx->db_bal = 64u;
    fx->master_drive = 52u;
    fx->harmonic_toms = 0u;
}

static void rytm_synth_push_evt(rytm_synth_t *s, uint32_t delay, uint8_t voice, uint8_t vel, uint8_t accent,
                                int8_t tune_off, int8_t dec_off)
{
    uint32_t i, slot = RYTM_EVT_MAX;

    if (!vel)
        return;
    for (i = 0; i < RYTM_EVT_MAX; i++) {
        if (!s->evt[i].live) {
            slot = i;
            break;
        }
    }
    if (slot >= RYTM_EVT_MAX)
        return;
    s->evt[slot].left = delay;
    s->evt[slot].voice = voice;
    s->evt[slot].vel = vel;
    s->evt[slot].accent = accent;
    s->evt[slot].tune_off = tune_off;
    s->evt[slot].dec_off = dec_off;
    s->evt[slot].live = 1u;
}

static void rytm_synth_schedule_drum_mod(rytm_synth_t *s, uint32_t delay_samples, uint8_t voice, uint8_t vel,
                                         uint8_t accent, int8_t tune_off, int8_t dec_off)
{
    if (voice >= RYTM_NDVOICES)
        return;
    rytm_synth_push_evt(s, delay_samples, voice, vel, accent, tune_off, dec_off);
}

static void rytm_synth_schedule_drum(rytm_synth_t *s, uint32_t delay_samples, uint8_t voice, uint8_t vel, uint8_t accent)
{
    rytm_synth_schedule_drum_mod(s, delay_samples, voice, vel, accent, 0, 0);
}

/* Main-hit delay shared by drums and bass so both land on the same sample. */
static uint32_t rytm_hit_delay(uint32_t base_delay_samples, int8_t micro_ms)
{
    int32_t micro = ((int32_t)micro_ms * (int32_t)RYTM_FS) / 1000;

    if (micro >= 0)
        return base_delay_samples + (uint32_t)micro;
    return base_delay_samples > (uint32_t)(-micro) ? base_delay_samples - (uint32_t)(-micro) : 0u;
}

/* tune_off / dec_off: per-step offsets on top of the kit, applied when the event fires. */
static void rytm_synth_schedule_ornament(rytm_synth_t *s, uint8_t voice, const rytm_drum_hit_t *hit,
                                         uint32_t sixteenth_samples, uint32_t base_delay_samples,
                                         int8_t tune_off, int8_t dec_off)
{
    uint32_t flam = (22u * RYTM_FS) / 1000u;
    uint32_t main_d;

    if (!hit || !hit->vel || voice >= RYTM_NDVOICES)
        return;
    main_d = rytm_hit_delay(base_delay_samples, hit->micro_ms);
#define RYTM_ORN_PUSH(d_, vel_, acc_) rytm_synth_schedule_drum_mod(s, (d_), voice, (vel_), (acc_), tune_off, dec_off)
    switch (hit->orn_kind) {
    case ORN_FLAM:
        if (main_d > flam)
            RYTM_ORN_PUSH(main_d - flam, hit->orn_vel ? hit->orn_vel : (uint8_t)(hit->vel >> 1), 0u);
        RYTM_ORN_PUSH(main_d, hit->vel, hit->accent);
        break;
    case ORN_DRAG: {
        uint32_t d1 = flam >> 1;
        uint32_t d2 = flam;

        if (main_d > d2) {
            RYTM_ORN_PUSH(main_d - d2, hit->orn_vel, 0u);
            RYTM_ORN_PUSH(main_d - d1, hit->orn_vel, 0u);
        }
        RYTM_ORN_PUSH(main_d, hit->vel, hit->accent);
        break;
    }
    case ORN_TRIPLET: {
        uint32_t span = sixteenth_samples + (sixteenth_samples >> 1);
        uint32_t t1 = span / 3u;
        uint32_t t2 = (span * 2u) / 3u;

        RYTM_ORN_PUSH(base_delay_samples + t1, hit->orn_vel ? hit->orn_vel : hit->vel, 0u);
        RYTM_ORN_PUSH(base_delay_samples + t2, hit->orn_vel ? hit->orn_vel : hit->vel, 0u);
        RYTM_ORN_PUSH(main_d, hit->vel, hit->accent);
        break;
    }
    case ORN_ROLL: {
        uint32_t n = 4u;
        uint32_t k;

        for (k = 0; k < n; k++) {
            uint32_t off = (sixteenth_samples * k) / n;
            int32_t rv = (int32_t)(hit->orn_vel ? hit->orn_vel : hit->vel) - (int32_t)(k << 2);

            if (rv < 1)
                rv = 1;
            RYTM_ORN_PUSH(base_delay_samples + off, (uint8_t)rv, k == n - 1u ? hit->accent : 0u);
        }
        break;
    }
    default:
        RYTM_ORN_PUSH(main_d, hit->vel, hit->accent);
        break;
    }
#undef RYTM_ORN_PUSH
}

static void rytm_synth_trig_drum(rytm_synth_t *s, uint8_t voice, uint8_t vel, uint8_t accent,
                                 const rytm_dvoice_cfg_t *cfg, uint8_t root_note, uint8_t harmonic_toms)
{
    rytm_drum_v_t *d;
    int32_t vlev;
    uint32_t inc;

    if (!s || voice >= RYTM_NDVOICES || !vel || !cfg || cfg->mute)
        return;
    d = &s->dv[voice];
    /* Level is applied once, post-fold, in rytm_apply_level. */
    vlev = vel > 127u ? 127 : (int32_t)vel;
    d->vel = (uint8_t)vlev;
    d->accent = accent;
    d->active = 1u;
    d->tune_off = 0;
    d->dec_off = 0;
    s->harmonic_toms = harmonic_toms;
    s->root_note = root_note;
    switch (voice) {
    case DV_KICK:
        d->env = (int32_t)d->vel << 9;
        d->env2 = (int32_t)d->vel << 9;
        d->pitch = (140 + (int32_t)cfg->tone) << 15;
        d->mod_env = (int32_t)cfg->tone << 8;
        d->ph = 0u;
        d->mph = 0u;
        d->click = (int32_t)d->vel << 8;
        s->sidechain_tgt = (int32_t)cfg->level << 8;
        if (accent)
            s->sidechain_tgt += s->sidechain_tgt >> 2;
        break;
    case DV_SNARE:
        d->env = (int32_t)d->vel << 9;
        d->env2 = (int32_t)d->vel << 9;
        d->shell1 = 0;
        d->shell2 = 0;
        d->pitch = (80 + (int32_t)cfg->tone) << 14;
        d->ph = 0u;
        d->mph = 0u;
        d->nph = 2463534242u ^ ((uint32_t)vel << 16);
        d->bp1 = 0;
        d->bp2 = 0;
        if (accent)
            d->click = (int32_t)d->vel << 8;
        else
            d->click = (int32_t)d->vel << 6;
        break;
    case DV_CH:
        d->hat_env = (int32_t)d->vel << 8;
        {
            uint32_t h;

            for (h = 0; h < 6u; h++)
                d->hat_ph[h] = (h + 1u) * 0x2AAAAAAu;
        }
        if (s->dv[DV_OH].active && s->dv[DV_OH].oh_ring > 0)
            s->dv[DV_OH].accent = 2u; /* trigger smooth choke */
        break;
    case DV_OH:
        d->oh_ring = (int32_t)d->vel << 8;
        {
            uint32_t h;

            for (h = 0; h < 6u; h++)
                d->hat_ph[h] = (h + 2u) * 0x2492492u;
        }
        break;
    case DV_LTOM:
    case DV_HTOM: {
        uint8_t note = (voice == DV_LTOM) ? 43u : 50u;

        if (harmonic_toms) {
            note = (uint8_t)(36u + (root_note % 12u) + (voice == DV_HTOM ? 7u : 0u));
            note = (uint8_t)(note + (cfg->tune >> 4) - 4);
        } else
            note = (uint8_t)(note + (cfg->tune >> 3) - 8);
        inc = rytm_note_inc(note);
        d->ph = 0u;
        d->mph = 0u;
        d->shell1 = (int32_t)inc;
        d->pitch = (int32_t)(inc >> 1);
        d->env = (int32_t)d->vel << 9;
        d->click = (int32_t)d->vel << 7;
        break;
    }
    case DV_CLAP:
        d->clap_idx = 1u;
        d->clap_env[0] = (int32_t)d->vel << 8;
        d->clap_env[1] = 0;
        d->clap_env[2] = 0;
        d->clap_env[3] = 0;
        d->ph = 0u;
        d->mph = 0u;
        d->bp1 = 0;
        d->bp2 = 0;
        d->nph = 54321u ^ ((uint32_t)vel << 11);
        d->env = (int32_t)d->vel << 8;
        break;
    default:
        break;
    }
    (void)cfg->decay;
    (void)cfg->drive;
}

static void rytm_bass_voice_trig(rytm_bass_v_t *b, const rytm_bass_hit_t *hit, const rytm_bass_cfg_t *cfg, uint8_t model)
{
    uint32_t inc;

    if (!b || !hit || !cfg || !hit->active || !hit->vel)
        return;
    b->model = model >= BMODEL_COUNT ? BMODEL_SOLID_FM : model;
    b->accent = hit->accent;
    b->dead_ghost = hit->dead_ghost;
    b->legato = hit->legato;
    b->target_note = hit->midi_note;
    inc = rytm_note_inc(hit->midi_note);
    if (!b->on || !hit->legato) {
        b->note = hit->midi_note;
        b->cur_inc = inc;
        b->tgt_inc = inc;
        b->ph = 0u;
        b->ph2 = 0u;
        b->sub_ph = 0u;
        b->m_ph = 0u;
        b->env = hit->dead_ghost ? ((int32_t)hit->vel << 8) : ((int32_t)hit->vel << 9);
        b->amp = b->env;
        if (b->model == BMODEL_SLAP_WG) {
            uint32_t i;
            uint32_t nst = 987654321u ^ ((uint32_t)hit->midi_note << 12);
            int32_t gain = hit->accent ? ((int32_t)hit->vel * 190) : ((int32_t)hit->vel * 130);

            for (i = 0; i < 64u; i++) {
                int32_t s1 = rytm_sine(i << 26);
                int32_t s2 = rytm_sine(i << 27) >> 1;
                int32_t nz = rytm_noise(&nst) - 32768;
                nz = hit->accent ? (nz >> 1) : (nz >> 2);
                int32_t v = ((s1 + s2 + nz) * gain) >> 15;
                if (v > 32000)
                    v = 32000;
                else if (v < -32000)
                    v = -32000;
                b->wg[i] = (int16_t)v;
            }
            b->wg_w = 0u;
        }
        if (b->model == BMODEL_FRETLESS) {
            b->amp = 0;
            b->cur_inc = inc - (inc >> 3);
            b->tgt_inc = inc;
        } else if (b->model == BMODEL_SUB808) {
            b->cur_inc = inc + (inc >> 1);
            b->tgt_inc = inc;
        }
    } else
        b->tgt_inc = inc;
    b->filt_env = ((int32_t)hit->filter_mod + ((int32_t)cfg->env_mod >> 1) + (hit->accent ? 36 : 0)) << 8;
    b->gate_left = ((int32_t)hit->gate_pct * 45) + 400;
    b->on = 1u;
}

static void rytm_synth_trig_bass(rytm_synth_t *s, const rytm_bass_hit_t *hit, const rytm_bass_cfg_t *cfg)
{
    if (!s || !hit || !cfg)
        return;
    rytm_bass_voice_trig(&s->bass, hit, cfg, cfg->model);
    if (s->model_xf < 64u)
        rytm_bass_voice_trig(&s->bass_prev, hit, cfg, s->prev_model);
}

/* Counted down per sample like drum events (left == 0 fires at that sample). Two slots:
 * swing + micro can push a step past the next step's schedule time. */
static void rytm_synth_schedule_bass(rytm_synth_t *s, uint32_t delay_samples, const rytm_bass_hit_t *hit,
                                     const rytm_bass_cfg_t *cfg)
{
    uint32_t k;

    if (!s || !hit || !cfg)
        return;
    k = !s->bass_pend_live[0] ? 0u : !s->bass_pend_live[1] ? 1u : 2u;
    if (k == 2u) {
        k = s->bass_pend_left[0] <= s->bass_pend_left[1] ? 0u : 1u;
        rytm_synth_trig_bass(s, &s->bass_pend[k], &s->bass_pend_cfg[k]);
    }
    s->bass_pend[k] = *hit;
    s->bass_pend_cfg[k] = *cfg;
    s->bass_pend_left[k] = delay_samples;
    s->bass_pend_live[k] = 1u;
}

static void rytm_synth_note_off_bass(rytm_synth_t *s)
{
    if (s) {
        s->bass.gate_left = 0;
        s->bass_prev.gate_left = 0;
    }
}

static int rytm_synth_is_quiet(const rytm_synth_t *s)
{
    uint32_t v;

    if (!s)
        return 1;
    if (s->bass.on && (s->bass.amp > 64 || s->bass.env > 64))
        return 0;
    if (s->model_xf < 64u && s->bass_prev.on && (s->bass_prev.amp > 64 || s->bass_prev.env > 64))
        return 0;
    if (s->sidechain > 64)
        return 0;
    for (v = 0; v < RYTM_NDVOICES; v++) {
        const rytm_drum_v_t *d = &s->dv[v];

        if (d->active || d->env > 32 || d->hat_env > 32 || d->oh_ring > 32)
            return 0;
    }
    return 1;
}

static int32_t rytm_render_kick(rytm_drum_v_t *d, const rytm_dvoice_cfg_t *cfg)
{
    int32_t p_shift = 6 + (cfg->decay >> 5);
    int32_t body, fm, click, out;
    uint32_t base_inc = rytm_note_inc((uint8_t)(28u + ((uint32_t)cfg->tune * 24u) / 127u));
    uint32_t sweep_inc = (uint32_t)(d->pitch >> 4) * 380u;
    uint32_t inc = base_inc + sweep_inc;

    d->pitch -= d->pitch >> p_shift;
    if (d->pitch < 0)
        d->pitch = 0;
    d->env = rytm_decay(d->env, 9u + (uint32_t)(cfg->decay >> 5));
    d->env2 = rytm_decay(d->env2, 10u + (uint32_t)(cfg->decay >> 4));
    d->mod_env = rytm_decay(d->mod_env, 8u);
    d->ph += inc;
    d->mph += inc + (inc >> 1);
    fm = rytm_sine(d->mph);
    body = rytm_sine(d->ph + (uint32_t)(int32_t)(((int64_t)fm * d->mod_env * (32 + (int32_t)cfg->tone)) >> 10));
    click = (d->click * rytm_sine(d->mph << 2)) >> 14;
    d->click = rytm_decay(d->click, 4u);
    out = ((body * d->env2) >> 15) + ((fm * d->env) >> 17) + click;
    out = rytm_fold(out, cfg->drive);
    if (d->env < 8 && d->env2 < 8 && d->click < 4) {
        d->env = 0;
        d->env2 = 0;
        d->active = 0u;
    }
    return rytm_apply_level(out, cfg->level);
}

static int32_t rytm_render_snare(rytm_drum_v_t *d, const rytm_dvoice_cfg_t *cfg)
{
    int32_t tone = (int32_t)cfg->tone;
    int32_t shell, wire, n, out;
    uint32_t inc1 = (145u + ((uint32_t)cfg->tune * 3u >> 1)) * 97391u + (uint32_t)(d->pitch >> 3) * 320u;
    uint32_t inc2 = inc1 + (inc1 >> 1) + (inc1 >> 4);

    d->pitch -= d->pitch >> 7;
    d->env = rytm_decay(d->env, 7u + (uint32_t)(cfg->decay >> 5));
    d->env2 = rytm_decay(d->env2, 6u + (uint32_t)((cfg->decay * 5u) >> 7));
    d->ph += inc1;
    d->mph += inc2;
    shell = (rytm_sine(d->ph) + (rytm_sine(d->mph) >> 1)) >> 1;
    n = rytm_noise(&d->nph);
    d->bp1 += (n - d->bp1) >> 2;
    d->bp2 += (d->bp1 - d->bp2) >> 1;
    wire = (d->bp1 - d->bp2) + ((n * shell) >> 16);
    out = (((shell * (148 - (tone >> 1))) >> 7) * d->env) >> 15;
    out += ((((wire * (12 + (tone * 3 >> 2))) >> 7) * d->env2) >> 15) + (d->click >> 2);
    d->click -= d->click >> 3;
    out = rytm_fold(out, cfg->drive);
    if (d->env < 8 && d->env2 < 8 && d->click < 8) {
        d->env = 0;
        d->env2 = 0;
        d->active = 0u;
    }
    return rytm_apply_level(out, cfg->level);
}

static int32_t rytm_render_hat(rytm_drum_v_t *d, const rytm_dvoice_cfg_t *cfg, int32_t open)
{
    int32_t tone = (int32_t)cfg->tone;
    int32_t sum = 0, hpf;
    uint32_t h;
    static const uint32_t ratio[6] = {205u, 298u, 369u, 518u, 545u, 800u};
    uint32_t base = 1100000u + ((uint32_t)cfg->tune * 6000u);

    for (h = 0; h < 6u; h++) {
        uint32_t inc = ratio[h] * base;
        int32_t sq, fm;

        d->hat_ph[h] += inc;
        sq = (d->hat_ph[h] & 0x80000000u) ? 5400 : -5400;
        fm = rytm_sine(d->hat_ph[h] + ((uint32_t)rytm_sine(d->hat_ph[(h + 1u) % 6u]) << (16 + (tone >> 5))));
        sum += (sq + (fm >> 1)) >> 1;
    }
    d->hp += (sum - d->hp) >> 1;
    hpf = sum - d->hp;
    d->bp1 += (hpf - d->bp1) >> 1;
    hpf = hpf - d->bp1;
    hpf = (hpf * (open ? d->oh_ring : d->hat_env)) >> 15;
    if (open) {
        if (d->accent == 2u) {
            d->oh_ring -= (d->oh_ring >> 5) + 16;
        } else {
            d->oh_ring = rytm_decay(d->oh_ring, 9u + (uint32_t)(cfg->decay >> 4));
        }
        if (d->oh_ring <= 8) {
            d->oh_ring = 0;
            d->active = 0u;
        }
    } else {
        d->hat_env = rytm_decay(d->hat_env, 7u + (uint32_t)(cfg->decay >> 5));
        if (d->hat_env <= 8) {
            d->hat_env = 0;
            d->active = 0u;
        }
    }
    /* 27/64 trims the hats back to their kit balance (FM index + unscaled vel added ~7.5 dB). */
    return rytm_apply_level((rytm_fold(hpf, cfg->drive >> 1) * 27) >> 6, cfg->level);
}

static int32_t rytm_render_tom(rytm_drum_v_t *d, const rytm_dvoice_cfg_t *cfg)
{
    int32_t out, mod;
    uint32_t inc = (uint32_t)d->shell1 + (uint32_t)d->pitch;

    d->env = rytm_decay(d->env, 9u + (uint32_t)(cfg->decay >> 4));
    d->pitch -= d->pitch >> (8 + (cfg->decay >> 5));
    if (d->pitch < 0)
        d->pitch = 0;
    d->ph += inc;
    d->mph += inc + (inc >> 1);
    mod = (rytm_sine(d->mph) * (int32_t)cfg->tone) >> 7;
    out = rytm_sine(d->ph + ((uint32_t)mod << 13));
    out = ((out * d->env) >> 15) + (d->click >> 3);
    d->click = rytm_decay(d->click, 3u);
    out = rytm_fold(out, cfg->drive);
    if (d->env < 8 && d->click < 4) {
        d->env = 0;
        d->active = 0u;
    }
    return rytm_apply_level(out, cfg->level);
}

static int32_t rytm_render_clap(rytm_drum_v_t *d, const rytm_dvoice_cfg_t *cfg)
{
    int32_t tone = (int32_t)cfg->tone;
    int32_t burst = 0, n, bp_n, body, out;
    uint32_t i;

    for (i = 0; i < 4u; i++) {
        if (d->clap_env[i] > 0)
            burst += d->clap_env[i] >> 2;
        d->clap_env[i] = rytm_decay(d->clap_env[i], 6u + (uint32_t)(i >> 1));
    }
    if (d->clap_idx < 4u) {
        d->mph++;
        if (d->mph > (11u * 44u)) {
            d->clap_env[d->clap_idx] = (int32_t)d->vel << 8;
            d->clap_idx++;
            d->mph = 0u;
        }
    }
    d->env = rytm_decay(d->env, 9u + (uint32_t)(cfg->decay >> 4));
    n = rytm_noise(&d->nph) - 32768;
    {
        /* SVF band-pass (bp1 = band, bp2 = low), fc 900..1660 Hz by tune, Q ~1.3.
         * f_q15 = 2*pi*fc/fs*32768 ~ fc*4781>>10. */
        int32_t f = (int32_t)(((900u + (uint32_t)cfg->tune * 6u) * 4781u) >> 10);
        int32_t hp;

        d->bp2 += (f * d->bp1) >> 15;
        hp = n - d->bp2 - (d->bp1 >> 1) - (d->bp1 >> 2);
        d->bp1 += (f * hp) >> 15;
        if (d->bp1 > 98304)
            d->bp1 = 98304;
        else if (d->bp1 < -98304)
            d->bp1 = -98304;
        if (d->bp2 > 98304)
            d->bp2 = 98304;
        else if (d->bp2 < -98304)
            d->bp2 = -98304;
        bp_n = d->bp1 * 2;
    }
    d->ph += (850u + (uint32_t)tone * 6u) * 97391u;
    body = rytm_sine(d->ph + ((uint32_t)bp_n << 12));
    out = (int32_t)(((int64_t)bp_n * (burst + (d->env >> 1))) >> 15) + (((body * (128 - (tone >> 1))) >> 7) * burst >> 16);
    out = rytm_fold(out, cfg->drive);
    if (d->env < 8 && burst < 8 && d->clap_idx >= 4u) {
        d->env = 0;
        d->active = 0u;
    }
    return rytm_apply_level(out, cfg->level);
}

static int32_t rytm_render_bass(rytm_bass_v_t *b, const rytm_bass_cfg_t *cfg, int32_t duck_q15)
{
    int32_t out = 0;
    int32_t fc = ((int32_t)cfg->cutoff << 8) + ((b->filt_env * (32 + (int32_t)cfg->env_mod)) >> 8) + (b->accent ? 4096 : 0);
    int32_t res = (int32_t)cfg->reso << 7;

    if (fc > 32000)
        fc = 32000;
    if (fc < 180)
        fc = 180;
    if (b->gate_left > 0)
        b->gate_left--;
    if (b->tgt_inc && b->cur_inc != b->tgt_inc) {
        int32_t g_sh = 9 - (cfg->glide >> 4);
        if (g_sh < 2)
            g_sh = 2;
        if (b->cur_inc < b->tgt_inc)
            b->cur_inc += (b->tgt_inc - b->cur_inc) >> g_sh;
        else
            b->cur_inc -= (b->cur_inc - b->tgt_inc) >> g_sh;
    }
    b->filt_env = rytm_decay(b->filt_env, 8u + (uint32_t)(cfg->decay >> 5));
    if (b->dead_ghost || b->gate_left <= 0)
        b->env = rytm_decay(b->env, 6u);
    else
        b->env = rytm_decay(b->env, 4u + (uint32_t)(cfg->decay >> 3));
    fc = fc - ((fc * duck_q15) >> 16);
    switch (b->model) {
    case BMODEL_ACID303: {
        int32_t saw = rytm_saw(b->ph);
        int32_t sq = rytm_sine(b->ph) > 0 ? 24000 : -24000;

        b->ph += b->cur_inc;
        out = saw + (sq >> 2);
        rytm_svf_lp(out, fc, res, &b->svf_l1, &b->svf_l2, &out);
        rytm_svf_lp(out, fc, res >> 1, &b->svf_b1, &b->svf_b2, &out);
        out = rytm_fold(out, cfg->drive + (b->accent ? 40u : 8u));
        break;
    }
    case BMODEL_SOLID_FM: {
        int32_t mod;

        b->ph += b->cur_inc;
        b->m_ph += b->cur_inc << 1;
        mod = rytm_sine(b->m_ph + ((uint32_t)b->fm_fb << 6));
        b->fm_fb = mod >> 2;
        mod = (mod * (24 + (int32_t)cfg->cutoff + (b->filt_env >> 8))) >> 7;
        if (mod > 22000)
            mod = 22000;
        else if (mod < -22000)
            mod = -22000;
        b->ph2 += b->cur_inc * 3u;
        {
            int32_t mod3 = (rytm_sine(b->ph2) * (8 + (int32_t)cfg->reso)) >> 8;
            out = rytm_sine(b->ph + ((uint32_t)(mod + mod3) << 14));
        }
        break;
    }
    case BMODEL_SLAP_WG: {
        uint32_t prev_idx = (b->ph >> 26) & 63u;
        uint32_t idx, next_idx, frac;
        int32_t s0, s1;

        b->ph += b->cur_inc;
        idx = (b->ph >> 26) & 63u;
        next_idx = (idx + 1u) & 63u;
        frac = (b->ph >> 18) & 255u;
        s0 = b->wg[idx];
        s1 = b->wg[next_idx];
        out = s0 + (((s1 - s0) * (int32_t)frac) >> 8);
        if (idx != prev_idx) {
            int32_t avg = (s0 + s1) >> 1;
            int32_t damp = b->dead_ghost ? 228 : (248 + (cfg->decay >> 4));
            if (damp > 255)
                damp = 255;
            b->wg[prev_idx] = (int16_t)((avg * damp) >> 8);
        }
        b->sub_ph += b->cur_inc;
        out += (rytm_sine(b->sub_ph) * (int32_t)cfg->sub_lvl) >> 10;
        if (b->accent)
            out += (rytm_sine(b->ph << 3) * (b->filt_env >> 7)) >> 10;
        rytm_svf_lp(out, fc >> 2, res >> 3, &b->svf_l1, &b->svf_l2, &out);
        break;
    }
    case BMODEL_DUB_REESE: {
        int32_t s1, s2, s3;

        b->ph += b->cur_inc;
        b->ph2 += b->cur_inc + (b->cur_inc >> 5);
        b->m_ph += b->cur_inc - (b->cur_inc >> 5);
        b->sub_ph += b->cur_inc >> 1;
        s1 = rytm_saw(b->ph);
        s2 = rytm_saw(b->ph2);
        s3 = rytm_saw(b->m_ph);
        out = (s1 + s2 + s3) / 3;
        out += rytm_sine(b->sub_ph) >> 1;
        rytm_svf_lp(out, fc >> 1, res >> 2, &b->svf_l1, &b->svf_l2, &out);
        break;
    }
    case BMODEL_SUB808: {
        int32_t fund, harm;

        if (b->cur_inc > b->tgt_inc)
            b->cur_inc -= (b->cur_inc - b->tgt_inc) >> 6;
        b->ph += b->cur_inc;
        fund = rytm_sine(b->ph);
        harm = rytm_sine(b->ph << 1) >> 2;
        out = rytm_softclip(fund + harm + ((fund * (int32_t)cfg->cutoff) >> 8), 24u + (cfg->reso >> 1));
        rytm_svf_lp(out, 2200 + (fc >> 2), res >> 3, &b->svf_l1, &b->svf_l2, &out);
        break;
    }
    case BMODEL_PWM: {
        uint32_t lfo_w;
        uint32_t duty;
        int32_t pulse, sub;

        b->ph += b->cur_inc;
        b->m_ph += 380000u + ((uint32_t)cfg->reso << 14);
        lfo_w = (uint32_t)(rytm_sine(b->m_ph) + 32768) >> 2;
        duty = 6000u + ((uint32_t)cfg->cutoff * 180u) + ((uint32_t)(b->filt_env >> 3)) + lfo_w;
        if (duty > 58000u)
            duty = 58000u;
        pulse = (b->ph >> 16) < duty ? 22000 : -22000;
        b->sub_ph += b->cur_inc >> 1;
        sub = (b->sub_ph & 0x80000000u) ? 9000 : -9000;
        out = pulse + sub;
        rytm_svf_lp(out, fc, (res * 3) >> 2, &b->svf_l1, &b->svf_l2, &out);
        break;
    }
    case BMODEL_FRETLESS: {
        int32_t vib, raw, lp1, bp1;

        b->m_ph += 460000u;
        vib = (int32_t)(((int64_t)rytm_sine(b->m_ph) * (int32_t)(b->cur_inc >> 9)) >> 15);
        b->ph += (uint32_t)((int32_t)b->cur_inc + vib);
        b->ph2 += (uint32_t)((int32_t)b->cur_inc + (int32_t)(b->cur_inc >> 8));
        raw = (rytm_saw(b->ph) + rytm_sine(b->ph2)) >> 1;
        /* rytm_svf_lp(in, fc, res, band_state, low_state, out): svf_l1 holds the band-pass. */
        rytm_svf_lp(raw, 1800 + (fc >> 1), 10000 + (res >> 1), &b->svf_l1, &b->svf_b1, &lp1);
        bp1 = b->svf_l1;
        out = (((lp1 >> 1) + bp1) * 9) >> 2; /* makeup: band-pass formant vs. old 1.5x LP */
        out = rytm_softclip(out, 28u);
        break;
    }
    default: {
        /* BMODEL_GRIT: hard-sync oscillator + wavefolder + pick bite */
        uint32_t prev_m = b->ph;
        uint32_t sync_mult = 256u + ((uint32_t)cfg->cutoff * 6u) + ((uint32_t)(b->filt_env >> 6));
        int32_t slave, master_w;

        b->ph += b->cur_inc;
        if (b->ph < prev_m)
            b->ph2 = 0u;
        else
            b->ph2 += (b->cur_inc >> 8) * sync_mult;
        slave = rytm_saw(b->ph2);
        master_w = (b->ph & 0x80000000u) ? 14000 : -14000;
        out = rytm_fold(slave + (master_w >> 1), 48u + (cfg->reso >> 1));
        rytm_svf_lp(out, 2400 + fc, res >> 1, &b->svf_l1, &b->svf_l2, &out);
        break;
    }
    }
    if (b->model == BMODEL_FRETLESS)
        b->amp += (b->env - b->amp) >> 7;
    else
        b->amp += (b->env - b->amp) >> 4;
    out = (out * b->amp) >> 15;
    if (b->gate_left <= 0 && b->amp < 32) {
        b->amp = 0;
        b->on = 0u;
    }
    return out;
}

static void rytm_synth_render(rytm_synth_t *s, int32_t *out, uint32_t n, const rytm_dvoice_cfg_t dv[RYTM_NDVOICES],
                              const rytm_bass_cfg_t *bc, const rytm_fx_cfg_t *fx, uint8_t root_note)
{
    uint32_t i, v;

    if (!s || !out || !dv || !bc || !fx)
        return;
    s->root_note = root_note;
    for (v = 0; v < RYTM_NDVOICES; v++)
        s->voice_peak[v] = s->voice_peak[v] > 6u ? (uint8_t)(s->voice_peak[v] - 6u) : 0u;
    s->bass_peak = s->bass_peak > 6u ? (uint8_t)(s->bass_peak - 6u) : 0u;
    for (i = 0; i < n; i++) {
        int32_t drum_l = 0, drum_r = 0, dsend = 0, rsend = 0, bass_sig = 0, bsnd = 0;
        int32_t duck = 0;

        for (v = 0; v < RYTM_EVT_MAX; v++) {
            if (!s->evt[v].live)
                continue;
            if (s->evt[v].left == 0u) {
                const rytm_evt_t *e = &s->evt[v];
                rytm_dvoice_cfg_t c = dv[e->voice];

                c.tune = rytm_clamp127((int32_t)c.tune + e->tune_off);
                c.decay = rytm_clamp127((int32_t)c.decay + e->dec_off);
                rytm_synth_trig_drum(s, e->voice, e->vel, e->accent, &c, root_note, fx->harmonic_toms);
                if (!c.mute) {
                    s->dv[e->voice].tune_off = e->tune_off;
                    s->dv[e->voice].dec_off = e->dec_off;
                }
                s->evt[v].live = 0u;
            } else
                s->evt[v].left--;
        }
        {
            uint32_t a = s->bass_pend_live[0] && s->bass_pend_live[1] &&
                                 s->bass_pend_left[1] < s->bass_pend_left[0] ? 1u : 0u;
            uint32_t r;

            for (r = 0; r < 2u; r++, a ^= 1u) {
                if (!s->bass_pend_live[a])
                    continue;
                if (s->bass_pend_left[a] == 0u) {
                    s->bass_pend_live[a] = 0u;
                    rytm_synth_trig_bass(s, &s->bass_pend[a], &s->bass_pend_cfg[a]);
                } else
                    s->bass_pend_left[a]--;
            }
        }

        /* Attack ~0.4 ms, release tau 4096 smp ~93 ms. */
        s->sidechain += (s->sidechain_tgt - s->sidechain) >> 4;
        s->sidechain_tgt = rytm_decay(s->sidechain_tgt, 12u);
        duck = (((int32_t)fx->pump + (int32_t)(s->macro_hype >> 1)) * s->sidechain) >> 14;
        if (duck > 128)
            duck = 128;
        for (v = 0; v < RYTM_NDVOICES; v++) {
            int32_t sig = 0;
            int32_t pan_r, pan_l;
            rytm_dvoice_cfg_t c;

            if (dv[v].mute || !s->dv[v].active)
                continue;
            c = dv[v];
            if (s->dv[v].tune_off || s->dv[v].dec_off) {
                c.tune = rytm_clamp127((int32_t)c.tune + s->dv[v].tune_off);
                c.decay = rytm_clamp127((int32_t)c.decay + s->dv[v].dec_off);
            }
            switch (v) {
            case DV_KICK:
                sig = rytm_render_kick(&s->dv[v], &c);
                break;
            case DV_SNARE:
                sig = rytm_render_snare(&s->dv[v], &c);
                break;
            case DV_CH:
                sig = rytm_render_hat(&s->dv[v], &c, 0);
                break;
            case DV_OH:
                sig = rytm_render_hat(&s->dv[v], &c, 1);
                break;
            case DV_LTOM:
                sig = rytm_render_tom(&s->dv[v], &c);
                break;
            case DV_HTOM:
                sig = rytm_render_tom(&s->dv[v], &c);
                break;
            case DV_CLAP:
                sig = rytm_render_clap(&s->dv[v], &c);
                break;
            default:
                break;
            }
            pan_r = (int32_t)dv[v].pan;
            pan_l = 127 - pan_r;
            drum_l += (sig * pan_l) >> 6;
            drum_r += (sig * pan_r) >> 6;
            dsend += (sig * (int32_t)dv[v].send_dly) >> 8;
            rsend += (sig * (int32_t)dv[v].send_rev) >> 7;
            rytm_peak_meter(&s->voice_peak[v], sig);
        }
        bass_sig = rytm_render_bass(&s->bass, bc, duck);
        if (s->model_xf < 64u) {
            rytm_bass_cfg_t old = *bc;
            int32_t prev;

            old.model = s->prev_model;
            prev = rytm_render_bass(&s->bass_prev, &old, duck);
            bass_sig = (bass_sig * (int32_t)s->model_xf + prev * (64 - (int32_t)s->model_xf)) >> 6;
        }
        rytm_peak_meter(&s->bass_peak, bass_sig);
        bsnd = (bass_sig * 96) >> 7;
        if ((i & 1u) == 0u) {
            /* Delay runs at fs/2 = 22050 Hz. */
            uint32_t dly_t = 64u + ((uint32_t)fx->dly_time * 28u);
            int32_t wow_q8 = (rytm_sine(s->wow_ph) >> 4) + ((rytm_sine(s->flut_ph) * 3) >> 9);
            int32_t rd_q8 = (int32_t)((s->dly_w + RYTM_DLY_LEN - (dly_t & (RYTM_DLY_LEN - 1u))) << 8) - wow_q8;
            uint32_t rd = ((uint32_t)rd_q8 >> 8) & (RYTM_DLY_LEN - 1u);
            int32_t frac = rd_q8 & 255;
            int32_t t0 = rytm_dly_pool[rd];
            int32_t tap = t0 + (((rytm_dly_pool[(rd + 1u) & (RYTM_DLY_LEN - 1u)] - t0) * frac) >> 8);
            int32_t wet_in = dsend + bsnd;

            s->wow_ph += 116870u;   /* 0.6 Hz, +-8 smp */
            s->flut_ph += 1266090u; /* 6.5 Hz, +-0.75 smp */
            wet_in = rytm_softclip(wet_in, fx->dly_fb >> 2);
            s->dly_lp += (tap - s->dly_lp) >> 1;       /* LP ~2.4 kHz */
            s->dly_hp += (s->dly_lp - s->dly_hp) >> 5; /* HP ~110 Hz */
            {
                int32_t fb = s->dly_lp - s->dly_hp;
                int32_t nw = wet_in + ((fb * (int32_t)fx->dly_fb) >> 7);

                if (nw > 32767)
                    nw = 32767;
                else if (nw < -32767)
                    nw = -32767;
                rytm_dly_pool[s->dly_w] = (int16_t)nw;
            }
            s->dly_tap_prev = s->dly_tap;
            s->dly_tap = tap;
            s->dly_w = (s->dly_w + 1u) & (RYTM_DLY_LEN - 1u);
        }
        {
            /* Linear 2x upsampling of the fs/2 tap (one sample late) instead of zero-stuffing;
             * >>7 keeps the old in-band echo level (zero-stuffed >>6 averaged to half). */
            int32_t dmix_amt = (int32_t)fx->dly_mix + (int32_t)s->macro_throw;
            int32_t tap_up = (i & 1u) ? s->dly_tap : ((s->dly_tap_prev + s->dly_tap) >> 1);
            int32_t dmix;

            if (dmix_amt > 127)
                dmix_amt = 127;
            dmix = (tap_up * dmix_amt) >> 7;
            drum_l += dmix;
            drum_r += dmix - (dmix >> 3);
        }
        {
            int32_t rev_in = rsend + ((drum_l + drum_r) >> 3);
            int32_t rev_amt = (int32_t)fx->rev_mix + (int32_t)(s->macro_throw >> 1);
            uint32_t t;

            for (t = 0; t < RYTM_REV_TAPS; t++) {
                /* Prime lengths 2.6..5.5 ms: no common period between the allpasses. */
                static const uint8_t TAP_LEN[RYTM_REV_TAPS] = {113u, 149u, 199u, 241u};
                static const uint16_t TAP_OFS[RYTM_REV_TAPS] = {0u, 113u, 262u, 461u};
                int16_t *cell = &s->rev_buf[TAP_OFS[t] + s->rev_pos[t]];
                int32_t ap = *cell;
                int32_t v_in = rev_in + ((ap * 180) >> 8);

                if (v_in > 32767)
                    v_in = 32767;
                else if (v_in < -32767)
                    v_in = -32767;
                *cell = (int16_t)(v_in >> 1);
                s->rev_pos[t] = (uint8_t)(s->rev_pos[t] + 1u >= TAP_LEN[t] ? 0u : s->rev_pos[t] + 1u);
                rev_in = ap - v_in;
            }
            {
                int32_t rm = (rev_in * rev_amt) >> 7;

                drum_l += rm;
                drum_r -= rm >> 1;
            }
        }
        {
            int32_t bal = (int32_t)fx->db_bal - 64;
            int32_t dl = drum_l - ((bal * drum_l) >> 7);
            int32_t dr = drum_r - ((bal * drum_r) >> 7);
            int32_t bass_mix = bass_sig - ((bass_sig * duck) >> 7);
            int32_t bm = bass_mix + ((bal * bass_mix) >> 7);
            int32_t punch = 128 + (int32_t)s->macro_punch;
            int32_t sw = (int32_t)s->macro_sweep - 64;
            int32_t sum_l = (((dl * punch) >> 7) + bm) >> 3;
            int32_t sum_r = (((dr * punch) >> 7) + bm) >> 3;
            int32_t ml = rytm_softclip(sum_l, fx->master_drive + (s->macro_punch >> 2));
            int32_t mr = rytm_softclip(sum_r, fx->master_drive + (s->macro_punch >> 2));

            s->master_lp += (ml - s->master_lp) >> 3;
            s->master_lp_r += (mr - s->master_lp_r) >> 3;
            if (sw < -2) {
                ml = s->master_lp;
                mr = s->master_lp_r;
            } else if (sw > 2) {
                ml -= s->master_lp;
                mr -= s->master_lp_r;
            }
            ml >>= 0;
            mr >>= 0;

            out[2u * i] = ml;
            out[2u * i + 1u] = mr;
        }
    }
}

#endif /* RYTM_DSP_H */
