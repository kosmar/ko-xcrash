/* SPDX-License-Identifier: GPL-3.0-only */
#define APP_NAME "KO RYTM"
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_56421"
#endif
#include "platform.c"
#include <stddef.h>
#define C_ACC RGB(180, 140, 255)
#include "fm_lfo.h"
#include "fm_seq.h"
#include "rytm_types.h"
#include "rytm_data.h"
#include "rytm_dsp.h"

#define KEY0 53u
#define RYTM_CFG_MAGIC 0x52595458u /* 128-step seq_ovr */
#define RYTM_CFG_MAGIC_S16 0x52595457u
#define RYTM_MOTION_STEPS RYTM_PAGE_STEPS
#define LFO_NTARGETS 53u

enum {
    VIEW_BAND = 0,
    VIEW_DRUM = 1,
    VIEW_BASS = 2,
    VIEW_SEQ = 3,
    VIEW_HOME = 4,
    VIEW_LFO = 5,
    VIEW_REC = 6,
    VIEW_COUNT = 7
};

enum {
    BANK_GROOVE = 0,
    BANK_DRUM = 1,
    BANK_BASS = 2,
    BANK_FX = 3,
    BANK_LFO = 4,
    BANK_HOME = 5,
    BANK_COUNT = 6
};

enum {
    LFO_WAVE_SINE = 0,
    LFO_WAVE_TRI = 1,
    LFO_WAVE_SAW = 2,
    LFO_WAVE_SQUARE = 3,
    LFO_WAVE_SH = 4,
    LFO_WAVE_RAMP = 5
};

enum {
    LFO_TGT_BASS_CUT = 0,
    LFO_TGT_BASS_RES = 1,
    LFO_TGT_BASS_ENV = 2,
    LFO_TGT_BASS_DEC = 3,
    LFO_TGT_BASS_GLD = 4,
    LFO_TGT_BASS_PRG = 5,
    LFO_TGT_DRUM_TUNE = 6,
    LFO_TGT_DRUM_DEC = 7,
    LFO_TGT_DRUM_TONE = 8,
    LFO_TGT_DRUM_DRV = 9,
    LFO_TGT_DRUM_LVL = 10,
    LFO_TGT_DRUM_DLY = 11,
    LFO_TGT_DRUM_REV = 12,
    LFO_TGT_DRUM_PAN = 13,
    LFO_TGT_GENRE = 14,
    LFO_TGT_DENSE = 15,
    LFO_TGT_FEEL = 16,
    LFO_TGT_FLAM = 17,
    LFO_TGT_SWING = 18,
    LFO_TGT_HUMAN = 19,
    LFO_TGT_DLY = 20,
    LFO_TGT_ROOM = 21,
    LFO_TGT_PUMP = 22,
    LFO_TGT_DB_BAL = 23,
    LFO_TGT_DLY_TIME = 24,
    LFO_TGT_DLY_FB = 25,
    LFO_TGT_REV_SIZE = 26,
    LFO_TGT_M_DRIVE = 27,
    LFO_TGT_MACRO0 = 28, /* 28..43 = all 16 HOME macros */
    LFO_TGT_ENSEMBL = 44,
    LFO_TGT_SEQ_FLIP = 45,
    LFO_TGT_SEQ_PAT = 46,
    LFO_TGT_SEQ_ROT = 47,
    LFO_TGT_SEQ_VARY = 48,
    LFO_TGT_SEQ_VEL = 49,
    LFO_TGT_SEQ_TRIG = 50,
    LFO_TGT_SEQ_GATE = 51,
    LFO_TGT_SEQ_PITCH = 52
};

enum {
    REC_MODE_OVERDUB = 0,
    REC_MODE_REPLACE = 1,
    REC_MODE_MONITOR = 2
};

enum {
    REC_QUANT_FREE = 0,
    REC_QUANT_SLOP = 1,
    REC_QUANT_16 = 2
};

enum {
    PERF_LIVE = 0,
    PERF_ARP = 1,
    PERF_SEQ = 2,
    PERF_REC = 3
};

enum {
    MOT_GENRE = 0,
    MOT_DENS = 1,
    MOT_FEEL = 2,
    MOT_FLAM = 3,
    MOT_BCUT = 4,
    MOT_BRES = 5,
    MOT_DTUN = 6,
    MOT_DUB = 7
};

static const uint8_t GM_DRUM[RYTM_NDVOICES] = {36, 38, 42, 46, 45, 50, 39};
/* Genre-matched pairs: 0 DUB=Sly/Robbie(4), 1 DISCO=Purdie/Jamerson(1), 2 HOUSE=Allen/Flabba(5),
 * 3 TECHNO=Jaki/Jaco(3), 4 TRIPHP=Clyde/Bootsy(2), 5 HIPHOP=Dilla/Claypool(7), 6 JUNGLE=P.Tony/Flea(6), 7 UKG=Elvin/Mingus(0) */
static const uint8_t BAND_PAIR_DRUM[8] = {4, 1, 5, 3, 2, 7, 6, 0};
static const uint8_t BAND_PAIR_BASS[8] = {4, 1, 5, 3, 2, 7, 6, 0};
/* F3..G5: 11 black keys, then 16 white keys. */
static const uint8_t KEY_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t KEY_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

typedef struct {
    uint8_t target;
    uint8_t wave[LFO_NTARGETS];
    uint8_t rate[LFO_NTARGETS];
    uint8_t depth[LFO_NTARGETS];
    uint8_t phase[LFO_NTARGETS];
    uint8_t smooth[LFO_NTARGETS];
    uint8_t skew[LFO_NTARGETS];
    uint8_t algo[LFO_NTARGETS];
    uint8_t ratio[LFO_NTARGETS];
    uint8_t index[LFO_NTARGETS];
    uint8_t fb[LFO_NTARGETS];
    uint8_t fold[LFO_NTARGETS];
    uint8_t drum_v[8];
    uint8_t seq_v[7];
} rytm_lfo_cfg_t;

#define RYTM_CFG_FIELDS(OVR_T) \
    uint32_t magic; \
    uint8_t genre; \
    uint8_t density; \
    uint8_t feel; \
    uint8_t flam; \
    uint8_t bass_density; \
    uint8_t bass_feel; \
    uint8_t bass_flam; \
    uint8_t bpm; \
    uint8_t swing_max; \
    uint8_t drummer; \
    uint8_t bassist; \
    uint8_t drummer_auto; \
    uint8_t bassist_auto; \
    uint8_t root; \
    uint8_t scale; \
    uint8_t prog_depth; \
    uint8_t human; \
    uint8_t sel_voice; \
    uint8_t pair_ix; \
    rytm_dvoice_cfg_t dv[RYTM_NDVOICES]; \
    rytm_bass_cfg_t bass; \
    rytm_fx_cfg_t fx; \
    OVR_T seq_ovr; \
    rytm_lfo_cfg_t lfo; \
    uint8_t motion_val[8][RYTM_MOTION_STEPS]; \
    uint8_t macro_motion[16][RYTM_MOTION_STEPS]; \
    uint8_t drum_motion[RYTM_NDVOICES][8][RYTM_MOTION_STEPS]; \
    uint8_t euclid_hits[8]; \
    int8_t euclid_rot[8]; \
    uint8_t arp_rate; \
    uint8_t arp_spread; \
    uint8_t arp_solo; \
    uint8_t arp_ramp; \
    uint8_t rec_bars; \
    uint8_t rec_mode; \
    uint8_t rec_quant; \
    uint8_t fx_rev_size; \
    uint8_t bass_mute; \
    uint8_t solo_mask; \
    uint8_t bank_page[BANK_COUNT];

typedef struct {
    RYTM_CFG_FIELDS(rytm_seq_ovr_t)
} rytm_cfg_t;

typedef struct {
    RYTM_CFG_FIELDS(rytm_seq_ovr_s16_t)
} rytm_cfg_s16_t;

typedef struct {
    rytm_cfg_t cfg;
    rytm_synth_t synth;
    uint8_t view;
    uint8_t bank;
    uint8_t perf;
    uint8_t playing;
    uint8_t ext_clock;
    uint8_t gesture;
    uint8_t gesture_latch;
    uint8_t bar;
    uint8_t step;
    uint8_t seq_lane;
    uint8_t seq_edit_step;
    uint8_t bass_oct;
    uint8_t last_bass_note;
    uint8_t rec_arm;
    uint32_t tick;
    uint32_t sample_acc;
    uint32_t samples_per_tick;
    uint32_t rng;
    uint32_t ui_dirty;
    uint32_t set_dirty;
    uint32_t set_t;
    uint32_t saved_until;
    uint32_t ui_ms;
    uint32_t midi_clk_acc;
    uint32_t ext_f8_in;
    uint32_t ext_f8_done;
    uint8_t xport_reset_req;
    uint32_t ext_last_ms;
    uint8_t save_req;
    uint8_t live_drum_mask;
    uint8_t live_bass_note;
    int8_t live_bass_vel;
    uint32_t prev_keys;
    uint32_t arp_mark[RYTM_NDVOICES];
    uint8_t arp_hits[RYTM_NDVOICES];
    uint32_t arp_bass_mark;
    uint8_t arp_bass_step;
    uint32_t morph_acc;
    uint32_t lfo_ph[LFO_NTARGETS];
    uint32_t lfo_cycle[LFO_NTARGETS];
    int32_t lfo_sm[LFO_NTARGETS];
    uint8_t eff_genre;
    uint8_t eff_bcut;
    uint8_t eff_bres;
    uint8_t eff_dtun;
    uint8_t eff_dly;
    uint8_t eff_pump;
    uint8_t eff_dens;
    uint8_t eff_bdens;
    uint8_t eff_feel;
    uint8_t eff_bfeel;
    uint8_t eff_flam;
    uint8_t eff_bflam;
    uint8_t eff_swing;
    uint8_t eff_human;
    uint8_t eff_prog;
    uint8_t eff_rev_size;
    uint8_t eff_macro[16];
    rytm_dvoice_cfg_t eff_dv[RYTM_NDVOICES];
    rytm_bass_cfg_t eff_bass;
    rytm_fx_cfg_t eff_fx;
    uint8_t seq_plock_step;
    uint8_t seq_plock_hold;
    uint8_t gest_hold[4];
    uint8_t gest_latch_badge;
    uint32_t bk_last_ms[11];
    uint32_t home_last_ms;
    uint32_t sel_last_ms;
    uint32_t rec_last_ms;
    uint8_t rec_wkey[16];
    uint8_t rec_note_st[16];
    uint8_t rec_note_sub[16];
    uint8_t rec_note_len[16];
    uint8_t rec_clear_ix;
    uint8_t rec_lane;
    uint8_t mot_last_step[8];
    uint8_t mot_last_val[8];
    uint32_t mot_last_ms[8];
    uint8_t mod_btn_used;
    uint8_t edit_used;
    uint8_t home_macro[16];
    uint8_t oct_down;
    uint8_t oct_up;
    uint8_t seq_page;
    uint8_t seq_ens_sel;
    uint8_t seq_flip_state;
    uint32_t seq_knob_flash_ms;
    char seq_knob_flash[8];
    uint8_t heard_mute_mask;
    uint8_t lfo_mute_on;
    uint8_t lfo_mute_mask;
    rytm_seq_ovr_t eff_seq_ovr;
    uint8_t seq_view_page;
    uint8_t pattern_origin[8];
} rytm_app_t;

static rytm_app_t g;

_Static_assert(RYTM_NSTEPS == FM_SEQ_STEPS && RYTM_MASK_BYTES == FM_SEQ_MASK_BYTES, "rytm/fm_seq step count");
_Static_assert(sizeof(rytm_cfg_t) <= SETTINGS_MAX, "rytm_cfg_t must fit the settings area");

static uint8_t seq_pages(void)
{
    return fm_seq_pages_clamp(g.cfg.seq_ovr.pages);
}

static uint32_t seq_len(void)
{
    return fm_seq_len(g.cfg.seq_ovr.pages);
}

static uint32_t seq_view_base(void)
{
    return (uint32_t)(g.seq_view_page % seq_pages()) * RYTM_PAGE_STEPS;
}

static void query_at(rytm_query_t *q, uint32_t st)
{
    q->step = (uint8_t)(st & 15u);
    q->ovr_step = (uint8_t)(st % RYTM_NSTEPS);
}
static rytm_dvoice_cfg_t g_kit[RYTM_NDVOICES];
static rytm_dvoice_cfg_t g_kit_from[RYTM_NDVOICES];
static rytm_dvoice_cfg_t g_kit_tgt[RYTM_NDVOICES];
static uint8_t g_kit_pos = 64u;
static uint8_t g_kit_drummer;
static uint8_t g_heard_model;
static uint8_t g_morph_ready;

#define RYTM_MORPH_STEPS 64u /* 4 bars of 16ths: a once-per-bar hit gets four stages */

static uint8_t morph_lerp(uint8_t a, uint8_t b, uint8_t pos)
{
    return (uint8_t)((int32_t)a + (((int32_t)b - (int32_t)a) * (int32_t)pos) / (int32_t)RYTM_MORPH_STEPS);
}

static void morph_clock(uint32_t samples)
{
    uint32_t sixteenth;

    if (g_kit_pos >= RYTM_MORPH_STEPS && g.synth.model_xf >= RYTM_MORPH_STEPS)
        return;
    sixteenth = g.samples_per_tick * 6u;
    if (sixteenth < 32u)
        sixteenth = 32u;
    g.morph_acc += samples;
    while (g.morph_acc >= sixteenth) {
        g.morph_acc -= sixteenth;
        if (g_kit_pos < RYTM_MORPH_STEPS)
            g_kit_pos++;
        if (g.synth.model_xf < RYTM_MORPH_STEPS)
            g.synth.model_xf++;
    }
}

static const uint8_t HOME_MACRO_DEF[16] = {
    0u, 64u, 0u, 0u,     /* P1: PUNCH, SWEEP, THROW, HYPE */
    64u, 64u, 64u, 64u,  /* P2: K.TUNE, K.DECAY, K.TONE, K.DRIVE */
    64u, 64u, 0u, 64u,   /* P3: ACID, B.BODY, B.GLIDE, HARM */
    64u, 64u, 64u, 0u    /* P4: ENERGY, POCKET, ORNAM, SPACE */
};

static void reset_home_macros(void)
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        g.home_macro[i] = HOME_MACRO_DEF[i];
        g.eff_macro[i] = HOME_MACRO_DEF[i];
    }
    g.synth.macro_punch = 0;
    g.synth.macro_sweep = 64;
    g.synth.macro_throw = 0;
    g.synth.macro_hype = 0;
}

static void sync_effective_params(void);
static int rytm_is_seq_lfo_tgt(uint8_t t);
static int rytm_seq_knob_lfo_map(uint8_t slot, uint8_t *out);
static void rytm_fmt_lfo_seq_long(char *s, uint8_t t);
static void rytm_fmt_lfo_seq_short(char *s, uint8_t t);
static uint8_t rytm_eff_seq_vary(uint8_t ln);
static int8_t rytm_eff_seq_vel(uint8_t ln);
static int8_t rytm_eff_seq_trig(uint8_t ln);
static int8_t rytm_eff_seq_gate(uint8_t ln);
static int8_t rytm_eff_seq_pitch(uint8_t ln);
static uint8_t rytm_eff_euclid_hits(uint8_t ln);
static int8_t rytm_eff_euclid_rot(uint8_t ln);

static void refresh_kit(void)
{
    uint32_t v;

    rytm_kit_effective(g.cfg.drummer, g.eff_genre, g.eff_dv, g_kit_tgt);
    if (!g_morph_ready) {
        for (v = 0; v < RYTM_NDVOICES; v++)
            g_kit[v] = g_kit_tgt[v];
        g_kit_pos = RYTM_MORPH_STEPS;
        g_kit_drummer = g.cfg.drummer;
        g_heard_model = g.cfg.bass.model;
        g.synth.model_xf = RYTM_MORPH_STEPS;
        g.synth.prev_model = g_heard_model;
        g_morph_ready = 1u;
        return;
    }
    if (g.cfg.drummer != g_kit_drummer) {
        for (v = 0; v < RYTM_NDVOICES; v++)
            g_kit_from[v] = g_kit[v];
        g_kit_pos = 0u;
        g_kit_drummer = g.cfg.drummer;
        g.morph_acc = 0u;
    }
    if (g.cfg.bass.model != g_heard_model) {
        g.synth.bass_prev = g.synth.bass;
        g.synth.prev_model = g_heard_model;
        g.synth.bass_prev.model = g_heard_model;
        g.synth.model_xf = 0u;
        g_heard_model = g.cfg.bass.model;
        g.morph_acc = 0u;
    }
    if (g_kit_pos >= RYTM_MORPH_STEPS) {
        for (v = 0; v < RYTM_NDVOICES; v++)
            g_kit[v] = g_kit_tgt[v];
        return;
    }
    for (v = 0; v < RYTM_NDVOICES; v++) {
        g_kit[v].tune = morph_lerp(g_kit_from[v].tune, g_kit_tgt[v].tune, g_kit_pos);
        g_kit[v].decay = morph_lerp(g_kit_from[v].decay, g_kit_tgt[v].decay, g_kit_pos);
        g_kit[v].tone = morph_lerp(g_kit_from[v].tone, g_kit_tgt[v].tone, g_kit_pos);
        g_kit[v].drive = morph_lerp(g_kit_from[v].drive, g_kit_tgt[v].drive, g_kit_pos);
        g_kit[v].level = morph_lerp(g_kit_from[v].level, g_kit_tgt[v].level, g_kit_pos);
        g_kit[v].send_dly = morph_lerp(g_kit_from[v].send_dly, g_kit_tgt[v].send_dly, g_kit_pos);
        g_kit[v].send_rev = morph_lerp(g_kit_from[v].send_rev, g_kit_tgt[v].send_rev, g_kit_pos);
        g_kit[v].pan = morph_lerp(g_kit_from[v].pan, g_kit_tgt[v].pan, g_kit_pos);
        g_kit[v].mute = g_kit_tgt[v].mute;
    }
}

#define C_DRUM RGB(95, 160, 255)
#define C_BASS RGB(110, 130, 255)
#define C_FX RGB(75, 220, 255)
#define C_GRO RGB(85, 190, 255)
#define C_HARM RGB(155, 120, 255)

/* Open spectrum Red -> Yellow -> Green -> Cyan -> Blue (0..240 deg, no magenta wrap) from led_fx */
static uint16_t spectrum_rgb565(uint16_t pos4095)
{
    uint32_t pos = pos4095 > 4095u ? 4095u : pos4095;
    uint32_t hue = (pos * 240u) / 4095u;
    uint32_t sector = hue / 60u;
    uint32_t ramp = ((hue % 60u) * 255u) / 59u;
    uint8_t r = 0, gr = 0, b = 0;

    switch (sector) {
    case 0:
        r = 255u;
        gr = (uint8_t)ramp;
        b = 0u;
        break;
    case 1:
        r = (uint8_t)(255u - ramp);
        gr = 255u;
        b = 0u;
        break;
    case 2:
        r = 0u;
        gr = 255u;
        b = (uint8_t)ramp;
        break;
    case 3:
        r = 0u;
        gr = (uint8_t)(255u - ramp);
        b = 255u;
        break;
    default:
        r = 0u;
        gr = 0u;
        b = 255u;
        break;
    }
    return RGB(r, gr, b);
}

/* Persona colors matched to their home genre on the R->Y->G->C->B rainbow:
 * 0 Elvin/Mingus (UKG/Jazz): Indigo-Blue, 1 Purdie/Jamerson (Disco): Orange,
 * 2 Clyde/Bootsy (Funk/TripHop): Lime-Green, 3 Jaki/Jaco (Techno): Emerald-Green,
 * 4 Sly/Robbie (Dub): Dub-Red, 5 Allen/Flabba (House): Yellow-Gold,
 * 6 P.Tony/Flea (Jungle): Sky-Blue, 7 Dilla/Claypool (HipHop/Dubstep): Cyan-Teal */
static const uint16_t PERSONA_COL[8] = {
    RGB(110, 120, 255),
    RGB(255, 150, 40),
    RGB(145, 255, 55),
    RGB(45, 245, 115),
    RGB(255, 65, 55),
    RGB(255, 230, 45),
    RGB(65, 170, 255),
    RGB(45, 235, 225)
};

/* Model hue is the player it belongs to, so 303 reads as Bootsy, 808 as Flabba. */
static const uint8_t MODEL_OWNER[8] = {2, 3, 1, 4, 5, 0, 0, 7};

static uint16_t model_color(uint8_t model)
{
    model = (uint8_t)(model % BMODEL_COUNT);
    if (model == BMODEL_FRETLESS)
        return RGB(140, 170, 255);
    if (model == BMODEL_SLAP_WG && (g.cfg.bassist % RYTM_NBASSISTS) == 6u)
        return PERSONA_COL[6];
    return PERSONA_COL[MODEL_OWNER[model]];
}

static const uint16_t BANK_COL[BANK_COUNT] = {C_GRO, C_DRUM, C_BASS, C_FX, C_HARM, C_ACC};
static const uint16_t BANK_BG[BANK_COUNT] = {
    RGB(9, 20, 36), RGB(10, 17, 38), RGB(12, 14, 40), RGB(8, 24, 34), RGB(18, 12, 38), RGB(14, 10, 34)
};

#ifndef fm1_led_key_dim
#define fm1_led_key_dim(id, on) ((void)0)
#endif

static const char *const ARP_RATE_NAMES_V2[6] = {"1/8", "1/16", "1/16T", "1/32", "FLAM!", "BUZZ"};
static const char *const LFO_RATE_NAMES[7] = {"4 BAR", "2 BAR", "1 BAR", "1/2", "1/4", "1/8", "1/16"};
static const char *const LFO_TGT_NAMES[LFO_NTARGETS] = {
    "B CUT", "B RES", "B ENV", "B DEC", "B GLD", "B PRG",
    "D TUN", "D DEC", "D TON", "D DRV", "D LVL", "D DLY", "D REV", "D PAN",
    "GENRE", "DENSE", "FEEL", "FLAM", "SWING", "HUMAN",
    "DLY", "ROOM", "PUMP", "D/B", "DL TM", "DL FB", "RM SZ", "M DRV",
    "PUNCH", "SWEEP", "THROW", "HYPE",
    "D.TUN", "D.DEC", "D.TON", "D.DRV",
    "ACID", "B.BDY", "B.GLD", "HARM",
    "ENRGY", "POCKT", "ORNAM", "SPACE",
    "BAND",   "FLIP",
    "S PAT", "S ROT", "S VARY", "S VEL", "S TRIG", "S GATE", "S PTCH"
};
static const char *const LFO_TGT_LONG[LFO_NTARGETS] = {
    "BASS CUTOFF", "BASS RESO", "BASS ENV MOD", "BASS DECAY", "BASS GLIDE", "BASS PROG",
    "DRUM TUNE", "DRUM DECAY", "DRUM TONE", "DRUM DRIVE", "DRUM LEVEL", "DRUM DELAY", "DRUM REVERB", "DRUM PAN",
    "GENRE MORPH", "DENSITY", "GROOVE FEEL", "GROOVE FLAM", "GROOVE SWING", "GROOVE HUMAN",
    "DELAY MIX", "REVERB ROOM", "SIDECHAIN PUMP", "DRUM/BASS BAL", "DELAY TIME", "DELAY FEEDBACK", "ROOM SIZE", "MASTER DRIVE",
    "MACRO PUNCH", "MACRO SWEEP", "MACRO THROW", "MACRO HYPE",
    "MACRO D.TUNE", "MACRO D.DECAY", "MACRO D.TONE", "MACRO D.DRIVE",
    "BASS ACID", "BASS BODY", "MACRO GLIDE", "HARMONY",
    "MACRO ENERGY", "MACRO POCKET", "ORNAMENT", "MACRO SPACE",
    "SEQ BAND",     "SEQ FLIP",
    "SEQ PATTRN", "SEQ ROTATE", "SEQ VARY", "SEQ VEL+-", "SEQ TRIG %", "SEQ GATE+-", "SEQ PITCH+-"
};

static const uint8_t RYTM_ENS_MUTE[FM_SEQ_NENSEMBLES] = {
    0xFFu, 0xFEu, 0x83u, 0x7Fu, 0x8Du, 0x81u, 0xAEu, 0x4Cu, 0x7Eu,
    0x81u, 0xB3u, 0x51u, 0x7Eu, 0x72u, 0x80u, 0x70u, 0x01u, 0x00u
};
static const char *const ARP_SOLO_NAMES[5] = {"UP", "DOWN", "OCT", "5TH", "ACID"};

static void copy_str(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static uint32_t rytm_strlen(const char *s)
{
    uint32_t n = 0;
    while (s[n])
        n++;
    return n;
}

static void u8_dec(char *d, uint32_t v)
{
    if (v >= 100u) {
        *d++ = (char)('0' + v / 100u);
        v %= 100u;
        *d++ = (char)('0' + v / 10u);
        *d++ = (char)('0' + v % 10u);
    } else if (v >= 10u) {
        *d++ = (char)('0' + v / 10u);
        *d++ = (char)('0' + v % 10u);
    } else
        *d++ = (char)('0' + v);
    *d = 0;
}

static const char *const NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static const char *const GENRE_SHORT[RYTM_NGENRES] = {
    "DUB", "DISCO", "HOUSE", "TECHNO", "TRIPHP", "HIPHOP", "JUNGLE", "UKG", "DUBSTP"
};
static const char *const BMODEL_SHORT[8] = {"303", "FM", "SLAP", "REESE", "808", "PWM", "FRET", "GRIT"};

static void i32_signed_dec(char *d, int32_t v)
{
    if (v > 0) {
        *d++ = '+';
        u8_dec(d, (uint32_t)v);
    } else if (v < 0) {
        *d++ = '-';
        u8_dec(d, (uint32_t)(-v));
    } else {
        *d++ = '0';
        *d = 0;
    }
}

/* UI readouts: same rounding as fm_app u8_show100_dec / bip50_show. */
static void u8_show100_dec(char *d, uint32_t v)
{
    if (v > 127u)
        v = 127u;
    u8_dec(d, (v * 100u + 63u) / 127u);
}

static void bip50_show(char *s, uint8_t v)
{
    int32_t c = (int32_t)(v > 127u ? 127u : v) - 64;

    if (c < -63)
        c = -63;
    i32_signed_dec(s, c < 0 ? -((-c * 50 + 31) / 63) : (c * 50 + 31) / 63);
}

static uint16_t genre_pos4095(void)
{
    return (uint16_t)((uint32_t)g.eff_genre * 4095u / 127u);
}

static uint16_t genre_hue(void)
{
    return spectrum_rgb565(genre_pos4095());
}

static uint8_t active_chord_root(void)
{
    uint8_t lo, hi, gf, near;
    int32_t chord, deg;
    rytm_genre_pair(genre_pos4095(), &lo, &hi, &gf);
    near = gf < 128u ? lo : hi;
    chord = (int32_t)RYTM_GENRE_PROG_8[near][g.bar % RYTM_PHRASE_BARS];
    deg = (chord * (int32_t)g.eff_prog) / 127;
    return (uint8_t)(rytm_scale_note(g.cfg.root, g.cfg.scale, deg, 0) % 12u);
}

static uint8_t rytm_target_mute_mask(void)
{
    uint32_t v;
    uint8_t m = 0u;

    for (v = 0; v < RYTM_NDVOICES; v++) {
        if (g.cfg.dv[v].mute)
            m |= (uint8_t)(1u << v);
    }
    if (g.cfg.bass_mute)
        m |= 1u << 7;
    return m;
}

static uint8_t rytm_eff_mute_mask(void)
{
    return g.lfo_mute_on ? g.lfo_mute_mask : rytm_target_mute_mask();
}

static void rytm_sync_heard_mutes(void)
{
    g.heard_mute_mask = rytm_eff_mute_mask();
}

static void rytm_apply_lfo_seq_mutes(void);

static void mark_dirty(void)
{
    g.set_dirty = 1u;
    g.set_t = fm1_ms;
    g.ui_dirty = 1u;
}

static uint8_t rytm_solo_bit(uint8_t v)
{
    return v < RYTM_NDVOICES ? (uint8_t)(1u << v) : (uint8_t)(1u << 7);
}

static uint8_t rytm_voice_muted(uint8_t v)
{
    return v < RYTM_NDVOICES ? g.cfg.dv[v].mute : g.cfg.bass_mute;
}

static void rytm_voice_set_unmute(uint8_t v)
{
    if (v < RYTM_NDVOICES) {
        g.cfg.dv[v].mute = 0u;
        if (g.cfg.dv[v].level == 0u)
            g.cfg.dv[v].level = 90u;
    } else
        g.cfg.bass_mute = 0u;
}

static void rytm_voice_set_mute(uint8_t v)
{
    if (v < RYTM_NDVOICES)
        g.cfg.dv[v].mute = 1u;
    else
        g.cfg.bass_mute = 1u;
}

static uint8_t rytm_voice_is_solo(uint8_t v)
{
    uint8_t bit = rytm_solo_bit(v);

    return (g.cfg.solo_mask & bit) && !rytm_voice_muted(v) ? 1u : 0u;
}

static uint8_t rytm_any_mute_or_solo(void)
{
    uint32_t i;

    if (g.cfg.solo_mask)
        return 1u;
    if (g.cfg.bass_mute)
        return 1u;
    for (i = 0; i < RYTM_NDVOICES; i++) {
        if (g.cfg.dv[i].mute)
            return 1u;
    }
    return 0u;
}

static uint8_t voice_audible(uint8_t v)
{
    if (v < RYTM_NDVOICES)
        return g.cfg.dv[v].mute ? 0u : 1u;
    return g.cfg.bass_mute ? 0u : 1u;
}

static uint8_t voice_heard_audible(uint8_t v)
{
    uint8_t bit = v < RYTM_NDVOICES ? v : 7u;

    return ((g.heard_mute_mask >> bit) & 1u) == 0u ? 1u : 0u;
}

static void clear_band_mute_solo(void)
{
    uint32_t v;

    g.cfg.solo_mask = 0;
    g.cfg.bass_mute = 0;
    g.gesture_latch = GEST_NONE;
    g.gest_latch_badge = 0;
    for (v = 0; v < RYTM_NDVOICES; v++)
        g.cfg.dv[v].mute = 0u;
    mark_dirty();
}

static void solo_toggle_voice(uint8_t v)
{
    uint32_t i;
    uint8_t bit = rytm_solo_bit(v);

    if (g.cfg.solo_mask & bit) {
        g.cfg.solo_mask &= (uint8_t)~bit;
        if (g.cfg.solo_mask == 0u)
            clear_band_mute_solo();
        else
            rytm_voice_set_mute(v);
    } else if (g.cfg.solo_mask == 0u) {
        g.cfg.solo_mask = bit;
        for (i = 0; i < 8u; i++) {
            if (i == (uint32_t)v)
                rytm_voice_set_unmute((uint8_t)i);
            else
                rytm_voice_set_mute((uint8_t)i);
        }
    } else {
        g.cfg.solo_mask |= bit;
        rytm_voice_set_unmute(v);
    }
    mark_dirty();
}

static void mute_toggle_voice(uint8_t v)
{
    uint8_t bit = rytm_solo_bit(v);

    if (rytm_voice_muted(v)) {
        rytm_voice_set_unmute(v);
        if (g.cfg.solo_mask)
            g.cfg.solo_mask |= bit;
    } else {
        rytm_voice_set_mute(v);
        if (g.cfg.solo_mask)
            g.cfg.solo_mask &= (uint8_t)~bit;
    }
    mark_dirty();
}

static void seq_lane_clear_meta(uint8_t lane)
{
    if (lane >= 8u)
        return;
    {
        uint32_t s;
        for (s = 0; s < RYTM_NSTEPS; s++) {
            g.cfg.seq_ovr.step_vel[lane][s] = 0;
            g.cfg.seq_ovr.step_prob[lane][s] = 0;
            g.cfg.seq_ovr.step_micro[lane][s] = 0;
            g.cfg.seq_ovr.step_tune[lane][s] = 0;
            if (lane == 7u) {
                g.cfg.seq_ovr.bass_note[s] = 0;
                g.cfg.seq_ovr.bass_acc[s] = 0;
                g.cfg.seq_ovr.bass_gate[s] = 0;
            }
        }
    }
    g.cfg.seq_ovr.vary_lane[lane] = 0;
    g.cfg.seq_ovr.lane_vel[lane] = 0;
    g.cfg.seq_ovr.lane_trig[lane] = 0;
    g.cfg.seq_ovr.lane_gate[lane] = 0;
    g.cfg.seq_ovr.lane_tune[lane] = 0;
    g.cfg.euclid_hits[lane] = 0;
    g.cfg.euclid_rot[lane] = 0;
}

static void seq_flash_knob(const char *msg)
{
    uint32_t i;

    if (!msg)
        return;
    for (i = 0; i < 7u && msg[i]; i++)
        g.seq_knob_flash[i] = msg[i];
    g.seq_knob_flash[i] = '\0';
    g.seq_knob_flash_ms = fm1_ms + 900u;
}

static void seq_reset_lane_song(uint8_t lane)
{
    if (lane < RYTM_NDVOICES) {
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_force_on[lane], 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_force_off[lane], 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_accent[lane], 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_flam[lane], 0);
        g.cfg.dv[lane].mute = 0u;
        if (g.cfg.solo_mask & (1u << lane))
            g.cfg.solo_mask &= (uint8_t)~(1u << lane);
        {
            uint32_t p, s;
            for (p = 0; p < 8u; p++)
                for (s = 0; s < RYTM_MOTION_STEPS; s++)
                    g.cfg.drum_motion[lane][p][s] = 255u;
        }
    } else {
        fm_seq_bits_fill(g.cfg.seq_ovr.bass_force_on, 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.bass_force_off, 0);
        g.cfg.bass_mute = 0u;
        if (g.cfg.solo_mask & (1u << 7))
            g.cfg.solo_mask &= (uint8_t)~(1u << 7);
    }
    seq_lane_clear_meta(lane);
    mark_dirty();
}

static void seq_empty_lane(uint8_t lane)
{
    if (lane < RYTM_NDVOICES) {
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_force_on[lane], 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_force_off[lane], 1);
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_accent[lane], 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.drum_flam[lane], 0);
    } else {
        fm_seq_bits_fill(g.cfg.seq_ovr.bass_force_on, 0);
        fm_seq_bits_fill(g.cfg.seq_ovr.bass_force_off, 1);
    }
    seq_lane_clear_meta(lane);
    mark_dirty();
}

static void seq_clear_lane(uint8_t lane);
static void seq_invert_lane(uint8_t lane);

static void seq_clear_lane(uint8_t lane)
{
    seq_reset_lane_song(lane);
}

static uint8_t rytm_lane_state_ix(uint8_t lane)
{
    if (rytm_voice_is_solo(lane))
        return FM_SEQ_STATE_SOLO;
    if (lane < RYTM_NDVOICES) {
        if (g.cfg.dv[lane].mute)
            return FM_SEQ_STATE_MUTE;
    } else if (g.cfg.bass_mute)
        return FM_SEQ_STATE_MUTE;
    return FM_SEQ_STATE_PLAY;
}

static void rytm_lane_apply_state(uint8_t lane, uint8_t state, uint8_t edit)
{
    if (edit || state == FM_SEQ_STATE_PLAY) {
        rytm_voice_set_unmute(lane);
        mark_dirty();
        return;
    }
    if (state == FM_SEQ_STATE_MUTE) {
        rytm_voice_set_mute(lane);
        g.cfg.solo_mask &= (uint8_t)~rytm_solo_bit(lane);
        mark_dirty();
    } else if (state == FM_SEQ_STATE_SOLO)
        solo_toggle_voice(lane);
    else if (state == FM_SEQ_STATE_INV) {
        seq_invert_lane(lane);
        seq_flash_knob("INVERT");
    }
}

static uint8_t seq_lane_meta_active(uint8_t lane)
{
    uint32_t s;

    if (g.cfg.euclid_hits[lane] || g.cfg.euclid_rot[lane] || g.cfg.seq_ovr.vary_lane[lane])
        return 1u;
    if (g.cfg.seq_ovr.lane_vel[lane] || g.cfg.seq_ovr.lane_trig[lane] || g.cfg.seq_ovr.lane_gate[lane] ||
        g.cfg.seq_ovr.lane_tune[lane])
        return 1u;
    for (s = 0; s < seq_len(); s++) {
        if (g.cfg.seq_ovr.step_vel[lane][s] || g.cfg.seq_ovr.step_prob[lane][s] ||
            g.cfg.seq_ovr.step_micro[lane][s] || g.cfg.seq_ovr.step_tune[lane][s])
            return 1u;
    }
    return 0u;
}

static uint8_t seq_lane_empty(uint8_t lane)
{
    uint32_t n = seq_len();

    if (lane < RYTM_NDVOICES)
        return fm_seq_bits_all(g.cfg.seq_ovr.drum_force_off[lane], n) &&
               !fm_seq_bits_any(g.cfg.seq_ovr.drum_force_on[lane], n) && !seq_lane_meta_active(lane);
    return fm_seq_bits_all(g.cfg.seq_ovr.bass_force_off, n) && !fm_seq_bits_any(g.cfg.seq_ovr.bass_force_on, n) &&
           !seq_lane_meta_active(lane);
}

static uint8_t seq_lane_on_song(uint8_t lane)
{
    uint32_t n = seq_len();

    if (lane < RYTM_NDVOICES)
        return !fm_seq_bits_any(g.cfg.seq_ovr.drum_force_on[lane], n) &&
               !fm_seq_bits_any(g.cfg.seq_ovr.drum_force_off[lane], n) && !seq_lane_meta_active(lane);
    return !fm_seq_bits_any(g.cfg.seq_ovr.bass_force_on, n) && !fm_seq_bits_any(g.cfg.seq_ovr.bass_force_off, n) &&
           !seq_lane_meta_active(lane);
}

static uint8_t seq_lane_content_is_song(uint8_t lane)
{
    uint32_t n = seq_len();
    uint8_t s;

    if (lane < RYTM_NDVOICES) {
        if (fm_seq_bits_any(g.cfg.seq_ovr.drum_force_on[lane], n) ||
            fm_seq_bits_any(g.cfg.seq_ovr.drum_force_off[lane], n))
            return 0u;
    } else if (fm_seq_bits_any(g.cfg.seq_ovr.bass_force_on, n) || fm_seq_bits_any(g.cfg.seq_ovr.bass_force_off, n)) {
        return 0u;
    }
    for (s = 0; s < n; s++) {
        if (g.cfg.seq_ovr.step_tune[lane][s])
            return 0u;
        if (lane >= RYTM_NDVOICES && g.cfg.seq_ovr.bass_note[s])
            return 0u;
    }
    return 1u;
}

static const char *seq_lane_clear_label(uint8_t lane)
{
    if (seq_lane_empty(lane))
        return "EMPTY";
    if (seq_lane_on_song(lane))
        return "SONG";
    return "EDITED";
}

static void rytm_invert_all_mutes(void)
{
    uint32_t v;
    uint8_t bm = g.cfg.bass_mute;
    uint8_t m[RYTM_NDVOICES];

    for (v = 0; v < RYTM_NDVOICES; v++)
        m[v] = g.cfg.dv[v].mute;
    g.cfg.solo_mask = 0;
    for (v = 0; v < RYTM_NDVOICES; v++) {
        if (m[v])
            g.cfg.dv[v].mute = 0u;
        else
            g.cfg.dv[v].mute = 1u;
    }
    g.cfg.bass_mute = bm ? 0u : 1u;
    mark_dirty();
}

static void build_query(rytm_query_t *q);

static void seq_rotate_lane(uint8_t lane, int32_t dir)
{
    rytm_seq_ovr_t *o = &g.cfg.seq_ovr;
    uint32_t st;
    uint32_t i;

    if (lane < RYTM_NDVOICES) {
        for (i = 0; i < RYTM_MAX_PAGES; i++) {
            uint16_t on_m = 0;
            rytm_query_t q;
            rytm_step_events_t ev;
            if ((uint16_t)(fm_seq_bits_page16(o->drum_force_on[lane], i) |
                           fm_seq_bits_page16(o->drum_force_off[lane], i)) == 0xFFFFu)
                continue;
            build_query(&q);
            for (st = 0; st < RYTM_PAGE_STEPS; st++) {
                query_at(&q, i * RYTM_PAGE_STEPS + st);
                rytm_resolve_step(&q, o, &ev);
                if (ev.drums[lane].vel > 0u)
                    on_m |= (uint16_t)(1u << st);
            }
            fm_seq_bits_set_page16(o->drum_force_on[lane], i, on_m);
            fm_seq_bits_set_page16(o->drum_force_off[lane], i, (uint16_t)~on_m);
        }
        fm_seq_bits_rot_pages(o->drum_force_on[lane], dir);
        fm_seq_bits_rot_pages(o->drum_force_off[lane], dir);
        fm_seq_bits_rot_pages(o->drum_accent[lane], dir);
        fm_seq_bits_rot_pages(o->drum_flam[lane], dir);
    } else {
        for (i = 0; i < RYTM_MAX_PAGES; i++) {
            uint16_t on_m = 0;
            rytm_query_t q;
            rytm_step_events_t ev;
            if ((uint16_t)(fm_seq_bits_page16(o->bass_force_on, i) | fm_seq_bits_page16(o->bass_force_off, i)) ==
                0xFFFFu)
                continue;
            build_query(&q);
            for (st = 0; st < RYTM_PAGE_STEPS; st++) {
                uint32_t ab = i * RYTM_PAGE_STEPS + st;
                query_at(&q, ab);
                rytm_resolve_step(&q, o, &ev);
                if (ev.bass.active) {
                    on_m |= (uint16_t)(1u << st);
                    if (!o->bass_note[ab])
                        o->bass_note[ab] = ev.bass.midi_note;
                }
            }
            fm_seq_bits_set_page16(o->bass_force_on, i, on_m);
            fm_seq_bits_set_page16(o->bass_force_off, i, (uint16_t)~on_m);
        }
        fm_seq_bits_rot_pages(o->bass_force_on, dir);
        fm_seq_bits_rot_pages(o->bass_force_off, dir);
    }
    fm_seq_rot_u8_pages(o->step_vel[lane], dir);
    fm_seq_rot_u8_pages(o->step_prob[lane], dir);
    fm_seq_rot_i8_pages(o->step_micro[lane], dir);
    fm_seq_rot_i8_pages(o->step_tune[lane], dir);
    if (lane == 7u) {
        fm_seq_rot_u8_pages(o->bass_note, dir);
        fm_seq_rot_u8_pages(o->bass_acc, dir);
        fm_seq_rot_u8_pages(o->bass_gate, dir);
    }
}

static void clear_all_overlays(void)
{
    uint32_t i, p, s;
    memset(&g.cfg.seq_ovr, 0, sizeof g.cfg.seq_ovr);
    for (i = 0; i < 8u; i++)
        for (s = 0; s < RYTM_MOTION_STEPS; s++)
            g.cfg.motion_val[i][s] = 255u;
    for (i = 0; i < 16u; i++)
        for (s = 0; s < RYTM_MOTION_STEPS; s++)
            g.cfg.macro_motion[i][s] = 255u;
    for (i = 0; i < RYTM_NDVOICES; i++)
        for (p = 0; p < 8u; p++)
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                g.cfg.drum_motion[i][p][s] = 255u;
    mark_dirty();
}

static uint8_t bank_page(void)
{
    if (g.bank == BANK_HOME)
        return g.cfg.bank_page[BANK_HOME] % 4u;
    if (g.bank == BANK_LFO)
        return g.cfg.bank_page[BANK_LFO] % 3u;
    if (g.bank == BANK_BASS)
        return g.cfg.bank_page[BANK_BASS] % 3u;
    return g.cfg.bank_page[g.bank % BANK_COUNT] & 1u;
}

static void toggle_bank_page(uint8_t bank)
{
    if (bank == BANK_HOME)
        g.cfg.bank_page[BANK_HOME] = (uint8_t)((g.cfg.bank_page[BANK_HOME] + 1u) % 4u);
    else if (bank == BANK_LFO)
        g.cfg.bank_page[BANK_LFO] = (uint8_t)((g.cfg.bank_page[BANK_LFO] + 1u) % 3u);
    else if (bank == BANK_BASS)
        g.cfg.bank_page[BANK_BASS] = (uint8_t)((g.cfg.bank_page[BANK_BASS] + 1u) % 3u);
    else
        g.cfg.bank_page[bank % BANK_COUNT] ^= 1u;
    mark_dirty();
}

static uint8_t motion_lane_for_knob(uint8_t slot)
{
    uint8_t pg = bank_page();
    if (g.perf == PERF_ARP || g.perf == PERF_SEQ || g.view == VIEW_REC)
        return 255u;
    if (g.bank == BANK_HOME)
        return 255u;
    if (g.bank == BANK_GROOVE) {
        if (pg == 0u)
            return slot == 0 ? MOT_DENS : slot == 1 ? MOT_FEEL : slot == 2 ? MOT_FLAM : 255u;
        return 255u;
    }
    if (g.bank == BANK_DRUM)
        return 255u;
    if (g.bank == BANK_BASS) {
        if (pg == 0u)
            return slot == 1 ? MOT_BCUT : slot == 2 ? MOT_BRES : 255u;
        return 255u;
    }
    if (g.bank == BANK_FX && pg == 0u && slot == 0)
        return MOT_DUB;
    return 255u;
}

static uint8_t lfo_target_for_knob(uint8_t slot)
{
    uint8_t pg = bank_page();
    uint8_t seq_lt;

    if (rytm_seq_knob_lfo_map(slot, &seq_lt))
        return seq_lt;
    if (g.perf == PERF_ARP || g.perf == PERF_SEQ || g.view == VIEW_REC || g.view == VIEW_LFO || g.bank == BANK_LFO)
        return 255u;
    if (g.bank == BANK_HOME)
        return (uint8_t)(LFO_TGT_MACRO0 + (pg % 4u) * 4u + slot);
    if (g.bank == BANK_GROOVE) {
        if (pg == 0u)
            return slot == 0 ? LFO_TGT_DENSE : slot == 1 ? LFO_TGT_FEEL : slot == 2 ? LFO_TGT_FLAM : 255u;
        return slot == 0 ? LFO_TGT_SWING : slot == 1 ? LFO_TGT_HUMAN : 255u;
    }
    if (g.bank == BANK_DRUM) {
        if (pg == 0u)
            return (uint8_t)(LFO_TGT_DRUM_TUNE + slot);
        return (uint8_t)(LFO_TGT_DRUM_LVL + slot);
    }
    if (g.bank == BANK_BASS) {
        if (pg == 0u)
            return slot == 1 ? LFO_TGT_BASS_CUT : slot == 2 ? LFO_TGT_BASS_RES : slot == 3 ? LFO_TGT_BASS_DEC : 255u;
        if (pg == 1u)
            return slot == 0 ? LFO_TGT_BASS_ENV : slot == 1 ? LFO_TGT_BASS_GLD : slot == 3 ? LFO_TGT_BASS_PRG : 255u;
        return 255u;
    }
    if (g.bank == BANK_FX) {
        if (pg == 0u)
            return (uint8_t)(LFO_TGT_DLY + slot);
        return (uint8_t)(LFO_TGT_DLY_TIME + slot);
    }
    return 255u;
}

static uint32_t lfo_period_samples_for(uint8_t rate)
{
    uint32_t bar_smp = g.samples_per_tick * 6u * RYTM_PAGE_STEPS;
    static const uint32_t mul[7] = {4u, 2u, 1u, 1u, 1u, 1u, 1u};
    static const uint32_t div[7] = {1u, 1u, 1u, 2u, 4u, 8u, 16u};
    uint8_t r = rate > 6u ? 6u : rate;
    uint32_t p = bar_smp * mul[r] / div[r];
    return p < 64u ? 64u : p;
}

static int32_t rytm_lfo_eval_slot(uint8_t slot, uint32_t ph, uint32_t cycle)
{
    return fm_lfo_eval_full_q15(ph, cycle, slot, g.cfg.lfo.wave[slot], g.cfg.lfo.skew[slot],
                                g.cfg.lfo.algo[slot], g.cfg.lfo.ratio[slot], g.cfg.lfo.index[slot],
                                g.cfg.lfo.fb[slot], g.cfg.lfo.fold[slot]);
}

static int32_t rytm_lfo_trace_sample(void *ctx, uint32_t ph, uint32_t cycle)
{
    return rytm_lfo_eval_slot((uint8_t)(uintptr_t)ctx, ph, cycle);
}

static int32_t draw_lfo_trace_slot(uint8_t slot, uint16_t col)
{
    uint64_t ph_off = (uint64_t)g.cfg.lfo.phase[slot] << 25;
    uint64_t now64 = (((uint64_t)g.lfo_cycle[slot] << 32) | (uint64_t)g.lfo_ph[slot]) + ph_off;

    return fm_lfo_draw_trace((void *)(uintptr_t)slot, rytm_lfo_trace_sample, now64, g.cfg.lfo.smooth[slot],
                             (int32_t)g.cfg.lfo.depth[slot], 8, 224, 66, 48, 8, 124, col);
}

static void draw_view_lfo(void)
{
    uint8_t sel = g.cfg.lfo.target % LFO_NTARGETS;
    uint32_t t;

    cv_line(8, 66, 231, 66, C_LINE);
    for (t = 0; t < LFO_NTARGETS; t++) {
        if (t == sel || g.cfg.lfo.depth[t] == 0u)
            continue;
        draw_lfo_trace_slot((uint8_t)t, C_DIM);
    }
    {
        int32_t y_now = draw_lfo_trace_slot(sel, C_WHITE);

        fm_lfo_draw_now_tick(8, y_now, 8, 124, C_HARM);
    }
    {
        char seq_hdr[24];
        const char *hdr = LFO_TGT_LONG[sel];

        if (rytm_is_seq_lfo_tgt(sel)) {
            rytm_fmt_lfo_seq_long(seq_hdr, sel);
            hdr = seq_hdr;
        }
        cv_text(8, 8, &FONT_S, hdr, C_WHITE);
    }
    cv_text(176, 8, &FONT_S, LFO_RATE_NAMES[g.cfg.lfo.rate[sel] % 7u], C_HARM);
    if (g.cfg.lfo.algo[sel] != FM_LFO_ALG_CLASSIC)
        fm_lfo_draw_algo_badge(136, 6, g.cfg.lfo.algo[sel], C_ACC, C_WHITE, C_GRAY);
}

static int32_t lfo_slot_delta(uint8_t t)
{
    int32_t d = (int32_t)g.cfg.lfo.depth[t];
    if (d == 0)
        return 0;
    return ((g.lfo_sm[t] * d) >> 16);
}

static uint8_t rytm_lfo_mod_enum(uint8_t base, uint8_t tgt, uint8_t count)
{
    int32_t d = lfo_slot_delta(tgt);
    int32_t step = (d * (int32_t)count) / 128;
    int32_t ix = (int32_t)(base % count) + step;

    while (ix < 0)
        ix += (int32_t)count;
    return (uint8_t)((uint32_t)ix % count);
}

static void rytm_apply_mute_mask(uint8_t dm)
{
    uint32_t i;

    for (i = 0; i < RYTM_NDVOICES; i++)
        g.cfg.dv[i].mute = (uint8_t)((dm >> i) & 1u);
    g.cfg.bass_mute = (uint8_t)((dm >> 7) & 1u);
}

static void rytm_apply_lfo_seq_mutes(void)
{
    uint8_t ens;
    uint8_t dm;
    uint8_t flip;

    if (g.cfg.lfo.depth[LFO_TGT_ENSEMBL] == 0u && g.cfg.lfo.depth[LFO_TGT_SEQ_FLIP] == 0u) {
        g.lfo_mute_on = 0u;
        return;
    }
    ens = g.seq_ens_sel;
    if (g.cfg.lfo.depth[LFO_TGT_ENSEMBL] > 0u)
        ens = rytm_lfo_mod_enum(g.seq_ens_sel, LFO_TGT_ENSEMBL, FM_SEQ_NENSEMBLES);
    if (ens >= FM_SEQ_NENSEMBLES)
        ens = FM_SEQ_ENS_ALL;
    dm = RYTM_ENS_MUTE[ens];
    flip = g.seq_flip_state & 1u;
    if (g.cfg.lfo.depth[LFO_TGT_SEQ_FLIP] > 0u) {
        int32_t fd = lfo_slot_delta(LFO_TGT_SEQ_FLIP);
        flip ^= (fd > 16) ? 1u : 0u;
    }
    if (flip)
        dm ^= 0xFFu;
    g.lfo_mute_mask = dm;
    g.lfo_mute_on = 1u;
}

static void lfo_step_all_slots(void)
{
    uint32_t t;
    for (t = 0; t < LFO_NTARGETS; t++) {
        uint64_t ph_off = (uint64_t)g.cfg.lfo.phase[t] << 25;
        uint64_t now64 = (((uint64_t)g.lfo_cycle[t] << 32) | (uint64_t)g.lfo_ph[t]) + ph_off;
        int32_t raw = rytm_lfo_eval_slot((uint8_t)t, (uint32_t)now64, (uint32_t)(now64 >> 32));

        g.lfo_sm[t] = fm_lfo_slew_step(g.lfo_sm[t], raw, g.cfg.lfo.smooth[t]);
    }
}

static uint8_t rec_loop_steps(void)
{
    static const uint8_t bars[4] = {1u, 2u, 4u, 8u};
    uint8_t ix = g.cfg.rec_bars > 3u ? 3u : g.cfg.rec_bars;
    return (uint8_t)(bars[ix] * RYTM_PAGE_STEPS);
}

static uint8_t rec_quant_step(uint8_t tick_mod6)
{
    if (g.cfg.rec_quant == REC_QUANT_FREE)
        return g.step;
    if (tick_mod6 >= 4u)
        return (uint8_t)((g.step + 1u) % seq_len());
    return g.step;
}

static uint8_t rec_map_step(uint8_t st)
{
    return (uint8_t)(st % seq_len());
}

static int8_t rec_sub_micro(uint8_t sub)
{
    int32_t m = ((int32_t)sub - 3) * 8;

    if (g.cfg.rec_quant == REC_QUANT_16)
        return 0;
    if (g.cfg.rec_quant == REC_QUANT_SLOP)
        return (int8_t)(m / 2);
    return (int8_t)m;
}

static void rec_bass_write_step(uint8_t st, uint8_t note, uint8_t legato, uint8_t gate, int8_t micro)
{
    if (g.cfg.rec_mode == REC_MODE_REPLACE) {
        g.cfg.seq_ovr.bass_note[st] = 0;
        g.cfg.seq_ovr.bass_acc[st] = 0;
        g.cfg.seq_ovr.bass_gate[st] = 0;
        g.cfg.seq_ovr.step_micro[7][st] = 0;
    }
    fm_seq_bit_put(g.cfg.seq_ovr.bass_force_on, st, 1);
    fm_seq_bit_put(g.cfg.seq_ovr.bass_force_off, st, 0);
    g.cfg.seq_ovr.bass_note[st] = note;
    g.cfg.seq_ovr.step_micro[7][st] = micro;
    g.cfg.seq_ovr.bass_gate[st] = gate;
    if (legato)
        g.cfg.seq_ovr.bass_acc[st] |= 2u;
    else
        g.cfg.seq_ovr.bass_acc[st] &= (uint8_t)~2u;
}

static void rec_bass_note_on(uint8_t wk, uint8_t note)
{
    uint8_t sub = (uint8_t)(g.tick % 6u);
    uint8_t st = rec_map_step(rec_quant_step(sub));

    rec_bass_write_step(st, note, 0u, 40u, rec_sub_micro(sub));
    g.rec_wkey[wk] = 1u;
    g.rec_note_st[wk] = st;
    g.rec_note_sub[wk] = sub;
    g.rec_note_len[wk] = 1u;
}

static void rec_bass_legato_step(uint8_t st, uint8_t note)
{
    fm_seq_bit_put(g.cfg.seq_ovr.bass_force_on, st, 1);
    fm_seq_bit_put(g.cfg.seq_ovr.bass_force_off, st, 0);
    g.cfg.seq_ovr.bass_note[st] = note;
    g.cfg.seq_ovr.bass_acc[st] |= 2u;
    g.cfg.seq_ovr.bass_gate[st] = 100u;
}

static void rec_bass_note_extend(uint8_t st)
{
    uint32_t k;

    for (k = 0; k < 16u; k++) {
        uint8_t note;

        if (!g.rec_wkey[k])
            continue;
        g.rec_note_len[k]++;
        if (st == g.rec_note_st[k])
            continue;
        note = g.cfg.seq_ovr.bass_note[g.rec_note_st[k]];
        rec_bass_legato_step(st, note);
    }
}

static void rec_bass_note_off(uint8_t wk)
{
    uint8_t st = g.rec_note_st[wk];
    uint8_t len = g.rec_note_len[wk];
    uint8_t gate = 40u;

    if (len <= 1u)
        gate = 40u;
    else if (len == 2u)
        gate = 55u;
    else if (len <= 4u)
        gate = 75u;
    else
        gate = 100u;
    g.cfg.seq_ovr.bass_gate[st] = gate;
    g.rec_wkey[wk] = 0u;
}

static int32_t bass_wave_point(uint8_t model, uint32_t ph, uint32_t pos)
{
    switch (model % BMODEL_COUNT) {
    case BMODEL_ACID303:
        return rytm_saw(ph);
    case BMODEL_SOLID_FM:
        return (rytm_sine(ph) + (rytm_sine(ph * 2u) >> 1) + (rytm_sine(ph * 3u) >> 1)) * 2 / 3;
    case BMODEL_SLAP_WG: {
        int32_t env = 32767 - (int32_t)((pos * 32767u) / 80u);
        if (env < 4096)
            env = 4096;
        return ((rytm_sine(ph) + (rytm_sine(ph * 5u) >> 1)) * env) >> 15;
    }
    case BMODEL_DUB_REESE:
        return (rytm_saw(ph) + rytm_saw(ph + (pos << 23)) + rytm_saw(ph - (pos << 23))) / 3;
    case BMODEL_SUB808: {
        uint32_t drop = pos < 36u ? (36u - pos) * (36u - pos) : 0u;
        return rytm_sine(ph + (drop << 19));
    }
    case BMODEL_PWM: {
        uint32_t duty = 8192u + ((uint32_t)g.eff_bcut * 320u);
        return (ph >> 16) < duty ? 26000 : -26000;
    }
    case BMODEL_FRETLESS: {
        int32_t atk = (int32_t)((pos * 32767u) / 48u);
        if (atk > 32767)
            atk = 32767;
        return (((rytm_saw(ph) + rytm_sine(ph)) >> 1) * atk) >> 15;
    }
    default: {
        int32_t m = rytm_saw(ph * 3u);
        return rytm_fold(m, 64u + (g.eff_bres >> 1));
    }
    }
}

static void rytm_apply_euclid_hits(uint8_t ln, uint8_t eh)
{
    uint8_t *on_m = ln < RYTM_NDVOICES ? g.cfg.seq_ovr.drum_force_on[ln] : g.cfg.seq_ovr.bass_force_on;
    uint8_t *off_m = ln < RYTM_NDVOICES ? g.cfg.seq_ovr.drum_force_off[ln] : g.cfg.seq_ovr.bass_force_off;

    if (eh == FM_SEQ_EUCLID_EMPTY) {
        fm_seq_bits_fill(on_m, 0);
        fm_seq_bits_fill(off_m, 1);
    } else if (eh == FM_SEQ_EUCLID_EDITED) {
        ;
    } else if (eh == FM_SEQ_EUCLID_SONG || eh == FM_SEQ_EUCLID_FRESH) {
        fm_seq_bits_fill(on_m, 0);
        fm_seq_bits_fill(off_m, 0);
    } else {
        uint16_t mask = 0;
        uint8_t rot = (uint8_t)((g.cfg.euclid_rot[ln] + 16) & 15);
        fm_seq_euclid_mask(eh, 16u, rot, &mask);
        fm_seq_bits_tile16(on_m, mask);
        fm_seq_bits_tile16(off_m, (uint16_t)~mask);
    }
}

static void seq_toggle_lane_step(uint8_t lane, uint8_t st)
{
    rytm_seq_ovr_t *o = &g.cfg.seq_ovr;
    uint8_t *on_m = lane < RYTM_NDVOICES ? o->drum_force_on[lane] : o->bass_force_on;
    uint8_t *off_m = lane < RYTM_NDVOICES ? o->drum_force_off[lane] : o->bass_force_off;
    rytm_query_t q;
    rytm_step_events_t ev;
    int hit;

    st = (uint8_t)(st % RYTM_NSTEPS);
    build_query(&q);
    query_at(&q, st);
    rytm_resolve_step(&q, o, &ev);
    hit = lane < RYTM_NDVOICES ? ev.drums[lane].vel > 0u : ev.bass.active;
    if (hit) {
        fm_seq_bit_put(on_m, st, 0);
        fm_seq_bit_put(off_m, st, 1);
    } else {
        if (g.cfg.euclid_hits[lane] == FM_SEQ_EUCLID_EMPTY) {
            fm_seq_bits_fill(on_m, 0);
            fm_seq_bits_fill(off_m, 1);
        }
        fm_seq_bit_put(on_m, st, 1);
        fm_seq_bit_put(off_m, st, 0);
        if (lane >= RYTM_NDVOICES && !o->bass_note[st])
            o->bass_note[st] = (uint8_t)(36u + (g.cfg.root % 12u));
    }
    fm_seq_lane_pattern_mark_edited(&g.cfg.euclid_hits[lane], &g.pattern_origin[lane]);
    mark_dirty();
}

static void seq_invert_lane(uint8_t lane)
{
    uint8_t st;
    for (st = 0; st < RYTM_NSTEPS; st++)
        seq_toggle_lane_step(lane, st);
}

static uint8_t rytm_lane_p3_state_pos(uint8_t v)
{
    if (rytm_voice_is_solo(v))
        return 2u;
    if (v < RYTM_NDVOICES) {
        if (g.cfg.dv[v].mute)
            return 0u;
    } else if (g.cfg.bass_mute)
        return 0u;
    return 1u;
}

static void rytm_lane_apply_p3_state(uint8_t v, uint8_t pos)
{
    uint32_t i;
    uint8_t bit = rytm_solo_bit(v);

    pos %= 3u;
    if (pos == 0u) {
        if (v < RYTM_NDVOICES)
            g.cfg.dv[v].mute = 1u;
        else
            g.cfg.bass_mute = 1u;
        g.cfg.solo_mask &= (uint8_t)~bit;
    } else if (pos == 1u) {
        if (g.cfg.solo_mask & bit) {
            g.cfg.solo_mask &= (uint8_t)~bit;
            if (g.cfg.solo_mask == 0u)
                clear_band_mute_solo();
        } else {
            rytm_voice_set_unmute(v);
        }
    } else {
        g.cfg.solo_mask = bit;
        if (v < RYTM_NDVOICES) {
            for (i = 0; i < RYTM_NDVOICES; i++)
                g.cfg.dv[i].mute = (uint8_t)(i != (uint32_t)v);
            g.cfg.bass_mute = 1u;
        } else {
            for (i = 0; i < RYTM_NDVOICES; i++)
                g.cfg.dv[i].mute = 1u;
            g.cfg.bass_mute = 0u;
        }
    }
    mark_dirty();
}

static uint8_t rytm_detect_ensemble(void)
{
    uint32_t i;
    uint8_t dm = 0u;

    for (i = 0; i < RYTM_NDVOICES; i++) {
        if (g.cfg.dv[i].mute)
            dm |= (uint8_t)(1u << i);
    }
    if (g.cfg.bass_mute)
        dm |= (uint8_t)(1u << 7);
    for (i = 0; i < FM_SEQ_NENSEMBLES; i++) {
        if (dm == RYTM_ENS_MUTE[i])
            return (uint8_t)i;
    }
    return FM_SEQ_ENS_CUSTOM;
}

static void rytm_apply_ensemble(uint8_t ens)
{
    if (ens >= FM_SEQ_NENSEMBLES)
        ens = FM_SEQ_ENS_ALL;
    g.seq_ens_sel = ens;
    g.cfg.solo_mask = 0u;
    rytm_apply_mute_mask(RYTM_ENS_MUTE[ens]);
    mark_dirty();
}

static void drum_toggle_step(uint8_t st)
{
    seq_toggle_lane_step(g.cfg.sel_voice % RYTM_NDVOICES, st);
}

static void cfg_defaults(void)
{
    uint32_t i, s;
    g.cfg.magic = RYTM_CFG_MAGIC;
    g.cfg.genre = 0;
    g.cfg.density = 96;
    g.cfg.feel = 64;
    g.cfg.flam = 48;
    g.cfg.bass_density = 96;
    g.cfg.bass_feel = 64;
    g.cfg.bass_flam = 48;
    g.cfg.bpm = 120;
    g.cfg.swing_max = 65;
    g.cfg.drummer = 1;
    g.cfg.bassist = 1;
    g.cfg.drummer_auto = 0;
    g.cfg.bassist_auto = 0;
    g.cfg.root = 0;
    g.cfg.scale = 0;
    g.cfg.prog_depth = 96;
    g.cfg.human = 100;
    g.cfg.sel_voice = 0;
    g.cfg.pair_ix = 0;
    memset(g.cfg.euclid_hits, 0, sizeof g.cfg.euclid_hits);
    memset(g.cfg.euclid_rot, 0, sizeof g.cfg.euclid_rot);
    memset(g.pattern_origin, FM_SEQ_PATTERN_ORIGIN_NONE, sizeof g.pattern_origin);
    g.cfg.arp_rate = 1;
    g.cfg.arp_spread = 64;
    g.cfg.arp_solo = 0;
    g.cfg.arp_ramp = 64;
    g.cfg.rec_bars = 2;
    g.cfg.rec_mode = REC_MODE_OVERDUB;
    g.cfg.rec_quant = REC_QUANT_16;
    g.cfg.fx_rev_size = 64;
    g.cfg.bass_mute = 0;
    g.cfg.solo_mask = 0;
    memset(g.cfg.bank_page, 0, sizeof g.cfg.bank_page);
    g.cfg.lfo.target = 0;
    for (i = 0; i < LFO_NTARGETS; i++) {
        g.cfg.lfo.wave[i] = 0;
        g.cfg.lfo.rate[i] = 2;
        g.cfg.lfo.depth[i] = 0;
        g.cfg.lfo.phase[i] = 0;
        g.cfg.lfo.smooth[i] = 0;
        g.cfg.lfo.skew[i] = FM_LFO_SKEW_DEFAULT;
        g.cfg.lfo.algo[i] = 0;
        g.cfg.lfo.ratio[i] = 52u;
        g.cfg.lfo.index[i] = 64u;
        g.cfg.lfo.fb[i] = 0;
        g.cfg.lfo.fold[i] = 0;
    }
    memset(g.cfg.lfo.drum_v, 0, sizeof g.cfg.lfo.drum_v);
    memset(g.cfg.lfo.seq_v, 0, sizeof g.cfg.lfo.seq_v);
    rytm_synth_preset_defaults(g.cfg.dv, &g.cfg.bass, &g.cfg.fx);
    g.cfg.bass.model = RYTM_BASSIST_DEFAULT_MODEL[g.cfg.bassist];
    memset(&g.cfg.seq_ovr, 0, sizeof g.cfg.seq_ovr);
    g.cfg.seq_ovr.pages = 1u;
    for (i = 0; i < 8u; i++)
        for (s = 0; s < RYTM_MOTION_STEPS; s++)
            g.cfg.motion_val[i][s] = 255u;
    for (i = 0; i < 16u; i++)
        for (s = 0; s < RYTM_MOTION_STEPS; s++)
            g.cfg.macro_motion[i][s] = 255u;
    {
        uint32_t p;
        for (i = 0; i < RYTM_NDVOICES; i++)
            for (p = 0; p < 8u; p++)
                for (s = 0; s < RYTM_MOTION_STEPS; s++)
                    g.cfg.drum_motion[i][p][s] = 255u;
    }
}

static void transport_reset(void)
{
    g.bar = 0;
    g.step = 0;
    g.tick = 5u; /* next tick (6) fires Bar 0 Step 0 immediately */
    g.sample_acc = g.samples_per_tick > 1u ? g.samples_per_tick - 1u : 0u;
}

static void transport_set_bpm(void)
{
    uint32_t bpm = g.cfg.bpm < 60u ? 60u : g.cfg.bpm > 200u ? 200u : g.cfg.bpm;
    g.samples_per_tick = RYTM_FS * 60u / (bpm * 24u);
    if (g.samples_per_tick < 32u)
        g.samples_per_tick = 32u;
}

static uint8_t motion_step_hold(const uint8_t *arr, uint8_t step, uint8_t def)
{
    uint32_t i;
    if (arr[step & 15u] != 255u)
        return arr[step & 15u];
    for (i = 1u; i < RYTM_MOTION_STEPS; i++) {
        uint8_t prev = (uint8_t)((step + RYTM_MOTION_STEPS - i) & 15u);
        if (arr[prev] != 255u)
            return arr[prev];
    }
    return def;
}

static uint8_t motion_read(uint8_t lane, uint8_t def)
{
    if (g.perf != PERF_REC)
        return motion_step_hold(g.cfg.motion_val[lane & 7u], g.step, def);
    return def;
}

static void rytm_motion_lane_sanitize(uint8_t *arr)
{
    uint32_t s;
    uint8_t all0 = 1u;

    for (s = 0; s < RYTM_MOTION_STEPS; s++) {
        if (arr[s] != 0u) {
            all0 = 0u;
            break;
        }
    }
    if (!all0)
        return;
    for (s = 0; s < RYTM_MOTION_STEPS; s++)
        arr[s] = 255u;
}

static void rytm_sanitize_saved_motion(void)
{
    uint32_t i, p;

    for (i = 0; i < 8u; i++)
        rytm_motion_lane_sanitize(g.cfg.motion_val[i]);
    for (i = 0; i < 16u; i++)
        rytm_motion_lane_sanitize(g.cfg.macro_motion[i]);
    for (i = 0; i < RYTM_NDVOICES; i++)
        for (p = 0; p < 8u; p++)
            rytm_motion_lane_sanitize(g.cfg.drum_motion[i][p]);
}

/* Drum/bass share MOT_DENS/FEEL/FLAM lanes when linked; split edits must not be masked by drum motion. */
static uint8_t rytm_bass_groove_val(uint8_t mot, uint8_t bass_v, uint8_t drum_v)
{
    if (bass_v != drum_v)
        return bass_v;
    return motion_read(mot, bass_v);
}

static void motion_offset_arr(uint8_t *arr, int32_t delta)
{
    uint32_t s;
    if (delta == 0)
        return;
    for (s = 0; s < RYTM_MOTION_STEPS; s++) {
        if (arr[s] != 255u)
            arr[s] = rytm_clamp127((int32_t)arr[s] + delta);
    }
}

#define REC_TOTAL_SLOTS 80u

static uint8_t *rec_slot_arr(uint8_t id)
{
    if (id < 8u)
        return g.cfg.motion_val[id];
    if (id < 24u)
        return g.cfg.macro_motion[id - 8u];
    {
        uint8_t d = (uint8_t)(id - 24u);
        return g.cfg.drum_motion[(d >> 3) % RYTM_NDVOICES][d & 7u];
    }
}

static void rec_slot_label(uint8_t id, char *s)
{
    static const char *const CORE[8] = {"GENRE", "DENS", "FEEL", "FLAM", "CUTOFF", "RESO", "TUNE", "DELAY"};
    static const char *const MAC[16] = {
        "PUNCH", "SWEEP", "THROW", "HYPE",
        "D.TUN", "D.DEC", "D.TON", "D.DRV",
        "ACID", "B.BDY", "B.GLD", "HARM",
        "ENRGY", "POCKT", "ORNAM", "SPACE"
    };
    static const char VCHR[7] = {'K', 'S', 'C', 'O', 'L', 'H', 'P'};
    static const char *const DPRM[8] = {".TUN", ".DEC", ".TON", ".DRV", ".LVL", ".DLY", ".REV", ".PAN"};

    if (id < 8u) {
        copy_str(s, CORE[id]);
    } else if (id < 24u) {
        copy_str(s, MAC[id - 8u]);
    } else {
        uint8_t d = (uint8_t)(id - 24u);
        s[0] = VCHR[(d >> 3) % RYTM_NDVOICES];
        copy_str(s + 1, DPRM[d & 7u]);
    }
}

static uint8_t rec_slot_active(uint8_t id)
{
    const uint8_t *arr = rec_slot_arr(id);
    uint32_t s;
    for (s = 0; s < RYTM_MOTION_STEPS; s++) {
        if (arr[s] != 255u)
            return 1u;
    }
    return 0u;
}

static uint32_t rec_build_active_list(uint8_t *out)
{
    uint32_t id, n = 0;
    for (id = 0; id < REC_TOTAL_SLOTS; id++) {
        if (id == MOT_DTUN)
            continue;
        if (rec_slot_active((uint8_t)id))
            out[n++] = (uint8_t)id;
    }
    return n;
}

static void rec_focus_slot(uint8_t slot_id)
{
    uint8_t act[REC_TOTAL_SLOTS];
    uint32_t i, n = rec_build_active_list(act);
    for (i = 0; i < n; i++) {
        if (act[i] == slot_id) {
            g.rec_lane = (uint8_t)i;
            return;
        }
    }
}

static void motion_write(uint8_t lane, uint8_t val)
{
    uint8_t l = lane & 7u;
    if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR) {
        uint8_t cur = g.step & 15u;
        if (fm1_ms - g.mot_last_ms[l] <= 600u) {
            uint8_t s = g.mot_last_step[l] & 15u;
            uint8_t v0 = g.mot_last_val[l];
            uint32_t dist = (uint32_t)((cur + 16u - s) & 15u);
            if (dist > 0u && dist <= 8u) {
                uint32_t k;
                for (k = 1u; k <= dist; k++) {
                    uint8_t st = (uint8_t)((s + k) & 15u);
                    g.cfg.motion_val[l][st] = (uint8_t)((int32_t)v0 + (((int32_t)val - (int32_t)v0) * (int32_t)k) / (int32_t)dist);
                }
            }
        }
        g.cfg.motion_val[l][cur] = val;
        g.mot_last_step[l] = cur;
        g.mot_last_val[l] = val;
        g.mot_last_ms[l] = fm1_ms;
        rec_focus_slot(l);
    } else if (!g.rec_arm) {
        int32_t cur_eff = (int32_t)motion_read(l, val);
        motion_offset_arr(g.cfg.motion_val[l], (int32_t)val - cur_eff);
    }
}

static uint8_t macro_motion_read(uint8_t ix, uint8_t def)
{
    if (g.perf != PERF_REC)
        return motion_step_hold(g.cfg.macro_motion[ix & 15u], g.step, def);
    return def;
}

static void macro_motion_write(uint8_t ix, uint8_t val)
{
    uint8_t m = ix & 15u;
    if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR) {
        g.cfg.macro_motion[m][g.step & 15u] = val;
        rec_focus_slot((uint8_t)(8u + m));
    } else if (!g.rec_arm) {
        int32_t cur_eff = (int32_t)macro_motion_read(m, val);
        motion_offset_arr(g.cfg.macro_motion[m], (int32_t)val - cur_eff);
    }
}

static uint8_t drum_motion_read(uint8_t v, uint8_t param, uint8_t def)
{
    if (g.perf != PERF_REC)
        return motion_step_hold(g.cfg.drum_motion[v % RYTM_NDVOICES][param & 7u], g.step, def);
    return def;
}

static void drum_motion_write(uint8_t v, uint8_t param, uint8_t val)
{
    uint8_t dv_ix = v % RYTM_NDVOICES;
    uint8_t p = param & 7u;
    if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR) {
        g.cfg.drum_motion[dv_ix][p][g.step & 15u] = val;
        rec_focus_slot((uint8_t)(24u + dv_ix * 8u + p));
    } else if (!g.rec_arm) {
        int32_t cur_eff = (int32_t)drum_motion_read(dv_ix, p, val);
        motion_offset_arr(g.cfg.drum_motion[dv_ix][p], (int32_t)val - cur_eff);
    }
}

static const char RYTM_SEQ_LFO_LBL[8] = {'K', 'S', 'C', 'O', 'L', 'H', 'P', 'B'};
static const char *const RYTM_SEQ_LFO_SUF[7] = {
    "PAT", "ROT", "VARY", "VEL", "TRIG", "GATE", "PTCH"
};
static const char *const RYTM_SEQ_LFO_LONG[7] = {
    "PATTRN", "ROTATE", "VARY", "VEL+-", "TRIG %", "GATE+-", "PITCH+-"
};

static int rytm_is_seq_lfo_tgt(uint8_t t)
{
    return t >= LFO_TGT_SEQ_PAT && t <= LFO_TGT_SEQ_PITCH;
}

static uint8_t rytm_seq_lfo_ix(uint8_t t)
{
    return (uint8_t)(t - LFO_TGT_SEQ_PAT);
}

static void rytm_fmt_lfo_seq_short(char *s, uint8_t t)
{
    uint8_t ix = rytm_seq_lfo_ix(t);
    uint8_t ln = g.cfg.lfo.seq_v[ix] % 8u;

    s[0] = RYTM_SEQ_LFO_LBL[ln];
    s[1] = ' ';
    copy_str(s + 2, RYTM_SEQ_LFO_SUF[ix]);
}

static void rytm_fmt_lfo_seq_long(char *s, uint8_t t)
{
    uint8_t ix = rytm_seq_lfo_ix(t);
    uint8_t ln = g.cfg.lfo.seq_v[ix] % 8u;
    const char *lane = ln < RYTM_NDVOICES ? RYTM_DVOICE_NAMES[ln] : "BASS";

    while (*lane)
        *s++ = *lane++;
    *s++ = ' ';
    copy_str(s, "SEQ ");
    s += 4;
    copy_str(s, RYTM_SEQ_LFO_LONG[ix]);
}

static int rytm_seq_knob_lfo_map(uint8_t slot, uint8_t *out)
{
    if (g.perf != PERF_SEQ || g.view != VIEW_SEQ || g.seq_plock_hold)
        return 0;
    if (g.seq_page == 0u) {
        if (slot == 1u)
            *out = LFO_TGT_SEQ_PAT;
        else if (slot == 2u)
            *out = LFO_TGT_SEQ_ROT;
        else if (slot == 3u)
            *out = LFO_TGT_SEQ_VARY;
        else
            return 0;
        return 1;
    }
    if (g.seq_page == 1u) {
        *out = (uint8_t)(LFO_TGT_SEQ_VEL + fm_seq_p1_fn(slot));
        return 1;
    }
    if (g.seq_page == 2u) {
        if (slot == 2u)
            *out = LFO_TGT_ENSEMBL;
        else if (slot == 3u)
            *out = LFO_TGT_SEQ_FLIP;
        else
            return 0;
        return 1;
    }
    return 0;
}

static uint8_t rytm_eff_seq_vary(uint8_t ln)
{
    uint8_t v = g.cfg.seq_ovr.vary_lane[ln];

    if (g.cfg.lfo.seq_v[2] == ln && g.cfg.lfo.depth[LFO_TGT_SEQ_VARY] > 0u) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_VARY);
        int32_t nv = (int32_t)v + d;

        if (nv < 0)
            nv = 0;
        if (nv > 127)
            nv = 127;
        v = (uint8_t)nv;
    }
    return v;
}

static int8_t rytm_eff_seq_vel(uint8_t ln)
{
    int32_t v = g.cfg.seq_ovr.lane_vel[ln];

    if (g.cfg.lfo.seq_v[3] == ln && g.cfg.lfo.depth[LFO_TGT_SEQ_VEL] > 0u) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_VEL);
        v += (d * 50) / 127;
    }
    if (v < -50)
        v = -50;
    if (v > 50)
        v = 50;
    return (int8_t)v;
}

static int8_t rytm_eff_seq_trig(uint8_t ln)
{
    int32_t v = g.cfg.seq_ovr.lane_trig[ln];

    if (g.cfg.lfo.seq_v[4] == ln && g.cfg.lfo.depth[LFO_TGT_SEQ_TRIG] > 0u) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_TRIG);
        v += (d * 100) / 127;
    }
    if (v < -100)
        v = -100;
    if (v > 0)
        v = 0;
    return (int8_t)v;
}

static int8_t rytm_eff_seq_gate(uint8_t ln)
{
    int32_t v = g.cfg.seq_ovr.lane_gate[ln];

    if (g.cfg.lfo.seq_v[5] == ln && g.cfg.lfo.depth[LFO_TGT_SEQ_GATE] > 0u) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_GATE);
        v += (d * 50) / 127;
    }
    if (v < -50)
        v = -50;
    if (v > 50)
        v = 50;
    return (int8_t)v;
}

static int8_t rytm_eff_seq_pitch(uint8_t ln)
{
    int32_t v = g.cfg.seq_ovr.lane_tune[ln];

    if (g.cfg.lfo.seq_v[6] == ln && g.cfg.lfo.depth[LFO_TGT_SEQ_PITCH] > 0u) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_PITCH);
        v += (d * 24) / 127;
    }
    if (v < -24)
        v = -24;
    if (v > 24)
        v = 24;
    return (int8_t)v;
}

static uint8_t rytm_eff_euclid_hits(uint8_t ln)
{
    uint8_t h = g.cfg.euclid_hits[ln];

    if (g.cfg.lfo.depth[LFO_TGT_SEQ_PAT] > 0u && g.cfg.lfo.seq_v[0] == ln &&
        fm_seq_euclid_is_pattern(h)) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_PAT);
        int32_t nh = (int32_t)h + (d * 15) / 127;

        if (nh < 1)
            nh = 1;
        if (nh > 16)
            nh = 16;
        h = (uint8_t)nh;
    }
    return h;
}

static int8_t rytm_eff_euclid_rot(uint8_t ln)
{
    int32_t r = g.cfg.euclid_rot[ln];

    r = fm_seq_euclid_rot_norm(r);
    if (g.cfg.lfo.depth[LFO_TGT_SEQ_ROT] > 0u && g.cfg.lfo.seq_v[1] == ln) {
        int32_t d = lfo_slot_delta(LFO_TGT_SEQ_ROT);
        r += (d * 15) / 127;
    }
    return (int8_t)fm_seq_euclid_rot_norm(r);
}

static void rytm_euclid_patch_ovr(rytm_seq_ovr_t *o, uint8_t ln, uint8_t eh, int8_t rot)
{
    uint8_t *on_m = ln < RYTM_NDVOICES ? o->drum_force_on[ln] : o->bass_force_on;
    uint8_t *off_m = ln < RYTM_NDVOICES ? o->drum_force_off[ln] : o->bass_force_off;

    if (eh == FM_SEQ_EUCLID_EMPTY) {
        fm_seq_bits_fill(on_m, 0);
        fm_seq_bits_fill(off_m, 1);
    } else if (eh == FM_SEQ_EUCLID_SONG) {
        fm_seq_bits_fill(on_m, 0);
        fm_seq_bits_fill(off_m, 0);
    } else {
        uint16_t mask = 0;
        uint8_t rr = (uint8_t)(((int32_t)rot + 16) & 15);
        fm_seq_euclid_mask(eh, 16u, rr, &mask);
        fm_seq_bits_tile16(on_m, mask);
        fm_seq_bits_tile16(off_m, (uint16_t)~mask);
    }
}

static void rytm_build_eff_seq_ovr(void)
{
    uint8_t ln;

    g.eff_seq_ovr = g.cfg.seq_ovr;
    for (ln = 0; ln < 8u; ln++) {
        g.eff_seq_ovr.vary_lane[ln] = rytm_eff_seq_vary(ln);
        g.eff_seq_ovr.lane_vel[ln] = rytm_eff_seq_vel(ln);
        g.eff_seq_ovr.lane_trig[ln] = rytm_eff_seq_trig(ln);
        g.eff_seq_ovr.lane_gate[ln] = rytm_eff_seq_gate(ln);
        g.eff_seq_ovr.lane_tune[ln] = rytm_eff_seq_pitch(ln);
        if ((g.cfg.lfo.depth[LFO_TGT_SEQ_PAT] > 0u && g.cfg.lfo.seq_v[0] == ln) ||
            (g.cfg.lfo.depth[LFO_TGT_SEQ_ROT] > 0u && g.cfg.lfo.seq_v[1] == ln)) {
            if (fm_seq_euclid_is_pattern(g.cfg.euclid_hits[ln]))
                rytm_euclid_patch_ovr(&g.eff_seq_ovr, ln, rytm_eff_euclid_hits(ln), rytm_eff_euclid_rot(ln));
        }
    }
}

static void sync_effective_params(void)
{
    uint32_t m, v;
    int32_t pu, sw, th, hy, dt, dd, do_, dv_drv, m_ac, m_bd, m_gl, m_hm, m_en, m_pk, m_or, m_sp;
    int32_t l_dens = lfo_slot_delta(LFO_TGT_DENSE);

    for (m = 0; m < 16u; m++) {
        int32_t base_m = (int32_t)macro_motion_read((uint8_t)m, g.home_macro[m]);
        g.eff_macro[m] = rytm_clamp127(base_m + lfo_slot_delta((uint8_t)(LFO_TGT_MACRO0 + m)));
    }
    g.synth.macro_punch = g.eff_macro[0];
    g.synth.macro_sweep = g.eff_macro[1];
    g.synth.macro_throw = g.eff_macro[2];
    g.synth.macro_hype = g.eff_macro[3];

    pu = (int32_t)g.eff_macro[0];
    sw = (int32_t)g.eff_macro[1] - 64;
    th = (int32_t)g.eff_macro[2];
    hy = (int32_t)g.eff_macro[3];
    dt = (int32_t)g.eff_macro[4] - 64;
    dd = (int32_t)g.eff_macro[5] - 64;
    do_ = (int32_t)g.eff_macro[6] - 64;
    dv_drv = (int32_t)g.eff_macro[7] - 64;
    m_ac = (int32_t)g.eff_macro[8] - 64;
    m_bd = (int32_t)g.eff_macro[9] - 64;
    m_gl = (int32_t)g.eff_macro[10];
    m_hm = (int32_t)g.eff_macro[11] - 64;
    m_en = (int32_t)g.eff_macro[12] - 64;
    m_pk = (int32_t)g.eff_macro[13] - 64;
    m_or = (int32_t)g.eff_macro[14] - 64;
    m_sp = (int32_t)g.eff_macro[15];

    g.eff_genre = rytm_clamp127((int32_t)motion_read(MOT_GENRE, g.cfg.genre) + lfo_slot_delta(LFO_TGT_GENRE));
    g.eff_dens = rytm_clamp127((int32_t)motion_read(MOT_DENS, g.cfg.density) + m_en + (hy >> 2) + l_dens);
    g.eff_bdens = rytm_clamp127((int32_t)rytm_bass_groove_val(MOT_DENS, g.cfg.bass_density, g.cfg.density) + m_en +
                                 (hy >> 2) + l_dens);
    g.eff_feel = rytm_clamp127((int32_t)motion_read(MOT_FEEL, g.cfg.feel) + m_pk + lfo_slot_delta(LFO_TGT_FEEL));
    g.eff_bfeel = rytm_clamp127((int32_t)rytm_bass_groove_val(MOT_FEEL, g.cfg.bass_feel, g.cfg.feel) + m_pk +
                                 lfo_slot_delta(LFO_TGT_FEEL));
    g.eff_flam = rytm_clamp127((int32_t)motion_read(MOT_FLAM, g.cfg.flam) + m_or + (hy >> 1) + lfo_slot_delta(LFO_TGT_FLAM));
    g.eff_bflam = rytm_clamp127((int32_t)rytm_bass_groove_val(MOT_FLAM, g.cfg.bass_flam, g.cfg.flam) + m_or +
                                 (m_gl >> 1) + lfo_slot_delta(LFO_TGT_FLAM));
    {
        int32_t swg = (int32_t)g.cfg.swing_max + (m_pk >> 1) + lfo_slot_delta(LFO_TGT_SWING);
        if (swg < 0) swg = 0;
        if (swg > 100) swg = 100;
        g.eff_swing = (uint8_t)swg;
    }
    g.eff_human = rytm_clamp127((int32_t)g.cfg.human + (m_pk >> 1) + lfo_slot_delta(LFO_TGT_HUMAN));
    g.eff_prog = rytm_clamp127((int32_t)g.cfg.prog_depth + m_hm + lfo_slot_delta(LFO_TGT_BASS_PRG));

    g.eff_bass = g.cfg.bass;
    g.eff_bass.cutoff = rytm_clamp127((int32_t)motion_read(MOT_BCUT, g.cfg.bass.cutoff) + m_ac + sw + (hy >> 2) + lfo_slot_delta(LFO_TGT_BASS_CUT));
    g.eff_bass.reso = rytm_clamp127((int32_t)motion_read(MOT_BRES, g.cfg.bass.reso) + (m_ac >> 1) + ((sw < 0 ? -sw : sw) >> 1) + lfo_slot_delta(LFO_TGT_BASS_RES));
    g.eff_bass.env_mod = rytm_clamp127((int32_t)g.cfg.bass.env_mod + (m_ac >> 1) + (pu >> 2) + (hy >> 2) + lfo_slot_delta(LFO_TGT_BASS_ENV));
    g.eff_bass.decay = rytm_clamp127((int32_t)g.cfg.bass.decay + m_bd + lfo_slot_delta(LFO_TGT_BASS_DEC));
    g.eff_bass.sub_lvl = rytm_clamp127((int32_t)g.cfg.bass.sub_lvl + m_bd + (pu >> 1));
    g.eff_bass.glide = rytm_clamp127((int32_t)g.cfg.bass.glide + m_gl + lfo_slot_delta(LFO_TGT_BASS_GLD));
    g.eff_bcut = g.eff_bass.cutoff;
    g.eff_bres = g.eff_bass.reso;

    g.eff_fx = g.cfg.fx;
    g.eff_fx.dly_mix = rytm_clamp127((int32_t)motion_read(MOT_DUB, g.cfg.fx.dly_mix) + (m_sp >> 1) + th + lfo_slot_delta(LFO_TGT_DLY));
    g.eff_fx.rev_mix = rytm_clamp127((int32_t)g.cfg.fx.rev_mix + (m_sp >> 1) + (th >> 1) + lfo_slot_delta(LFO_TGT_ROOM));
    g.eff_fx.pump = rytm_clamp127((int32_t)g.cfg.fx.pump + (hy >> 1) + lfo_slot_delta(LFO_TGT_PUMP));
    g.eff_fx.db_bal = rytm_clamp127((int32_t)g.cfg.fx.db_bal + (m_bd >> 2) + lfo_slot_delta(LFO_TGT_DB_BAL));
    g.eff_fx.dly_time = rytm_clamp127((int32_t)g.cfg.fx.dly_time + lfo_slot_delta(LFO_TGT_DLY_TIME));
    g.eff_fx.dly_fb = rytm_clamp127((int32_t)g.cfg.fx.dly_fb + (m_sp >> 2) + (th >> 1) + lfo_slot_delta(LFO_TGT_DLY_FB));
    g.eff_rev_size = rytm_clamp127((int32_t)g.cfg.fx_rev_size + (m_sp >> 2) + lfo_slot_delta(LFO_TGT_REV_SIZE));
    g.eff_fx.master_drive = rytm_clamp127((int32_t)g.cfg.fx.master_drive + (pu >> 1) + (hy >> 2) + lfo_slot_delta(LFO_TGT_M_DRIVE));
    g.eff_fx.harmonic_toms = (uint8_t)(g.cfg.fx.harmonic_toms || g.eff_prog >= 48u);
    g.eff_dly = g.eff_fx.dly_mix;
    g.eff_pump = g.eff_fx.pump;

    for (v = 0; v < RYTM_NDVOICES; v++) {
        int32_t lt = (v == (g.cfg.lfo.drum_v[0] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_TUNE) : 0;
        int32_t ld = (v == (g.cfg.lfo.drum_v[1] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_DEC) : 0;
        int32_t lo = (v == (g.cfg.lfo.drum_v[2] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_TONE) : 0;
        int32_t lv = (v == (g.cfg.lfo.drum_v[3] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_DRV) : 0;
        int32_t ll = (v == (g.cfg.lfo.drum_v[4] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_LVL) : 0;
        int32_t ls = (v == (g.cfg.lfo.drum_v[5] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_DLY) : 0;
        int32_t lr = (v == (g.cfg.lfo.drum_v[6] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_REV) : 0;
        int32_t lp = (v == (g.cfg.lfo.drum_v[7] % RYTM_NDVOICES)) ? lfo_slot_delta(LFO_TGT_DRUM_PAN) : 0;
        g.eff_dv[v].tune = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 0u, g.cfg.dv[v].tune) + dt + lt);
        g.eff_dv[v].decay = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 1u, g.cfg.dv[v].decay) + dd + ld);
        g.eff_dv[v].tone = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 2u, g.cfg.dv[v].tone) + do_ + (sw >> 1) + lo);
        g.eff_dv[v].drive = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 3u, g.cfg.dv[v].drive) + dv_drv + (pu >> 1) + lv);
        g.eff_dv[v].level = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 4u, g.cfg.dv[v].level) + (pu >> 3) + ll);
        g.eff_dv[v].send_dly = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 5u, g.cfg.dv[v].send_dly) + (m_sp >> 1) + (th >> 1) + ls);
        g.eff_dv[v].send_rev = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 6u, g.cfg.dv[v].send_rev) + (m_sp >> 1) + lr);
        g.eff_dv[v].pan = rytm_clamp127((int32_t)drum_motion_read((uint8_t)v, 7u, g.cfg.dv[v].pan) + lp);
        g.eff_dv[v].mute = (uint8_t)((rytm_eff_mute_mask() >> v) & 1u);
    }
    rytm_build_eff_seq_ovr();
}

static void build_query(rytm_query_t *q)
{
    query_at(q, g.step);
    q->bar = g.bar;
    q->genre = g.eff_genre;
    q->density = g.eff_dens;
    q->feel = g.eff_feel;
    q->flam_amt = g.perf == PERF_ARP ? rytm_clamp127((int32_t)g.eff_flam + ((int32_t)g.cfg.arp_spread >> 1)) : g.eff_flam;
    q->bass_density = g.eff_bdens;
    q->bass_feel = g.eff_bfeel;
    q->bass_flam = g.perf == PERF_ARP ? rytm_clamp127((int32_t)g.eff_bflam + ((int32_t)g.cfg.arp_spread >> 1)) : g.eff_bflam;
    q->drummer = g.cfg.drummer;
    q->bassist = g.cfg.bassist;
    q->root_note = g.cfg.root;
    q->scale_id = g.cfg.scale;
    q->prog_depth = g.eff_prog;
    q->swing_max = g.eff_swing;
    q->bpm = g.cfg.bpm;
    q->gesture = g.gesture_latch ? g.gesture_latch : g.gesture;
    if (g.perf == PERF_ARP && q->gesture == GEST_NONE && g.cfg.arp_spread >= 80u)
        q->gesture = GEST_FILL;
    {
        uint8_t tier = (uint8_t)((g.cfg.arp_solo * 3u) / 127u);
        if (q->gesture == GEST_SOLO_DRUM || q->gesture == GEST_SOLO_BAND || g.perf == PERF_ARP)
            q->solo_bar = (uint8_t)(tier + g.bar);
        else
            q->solo_bar = g.bar;
    }
    q->human = g.eff_human;
    q->last_bass_note = g.last_bass_note;
    q->rng_seed = g.cfg.seq_ovr.freeze ? (991u + (uint32_t)g.bar * 131u) : g.rng;
}

/* Frame offset of the current seq_advance() inside the audio block; the synth's event
 * delays count from the block start. 0 outside app_audio (ext clock, UI). */
static uint32_t g_evt_ofs;

static void fire_step_events(const rytm_step_events_t *ev)
{
    uint32_t v;
    uint32_t sixteenth = g.samples_per_tick * 6u;
    uint32_t swing_smp = ((uint32_t)ev->swing_delay_ms * RYTM_FS) / 1000u;
    uint32_t base_delay = g_evt_ofs + swing_smp + (sixteenth / 10u);
    uint8_t c_root = active_chord_root();
    int32_t p_off = 0;

    refresh_kit();
    if (g.perf == PERF_ARP) {
        int32_t ramp = (int32_t)g.cfg.arp_ramp - 64;
        if (ramp != 0) {
            p_off = (ramp * (int32_t)(g.step & 15u)) / 8;
            for (v = 0; v < RYTM_NDVOICES; v++)
                g_kit[v].tune = rytm_clamp127((int32_t)g_kit[v].tune + p_off);
        }
    }
    g.eff_dtun = g_kit[g.cfg.sel_voice % RYTM_NDVOICES].tune;
    g.synth.harmonic_toms = g.eff_fx.harmonic_toms;
    g.synth.root_note = c_root;

    for (v = 0; v < RYTM_NDVOICES; v++) {
        rytm_drum_hit_t dh = ev->drums[v];
        if (dh.vel && voice_heard_audible((uint8_t)v)) {
            int8_t gate_rnd = fm_seq_rand_gate(g.cfg.seq_ovr.lane_gate[v], g.step, g.bar, (uint8_t)v, g.rng);
            int8_t tune_rnd = (int8_t)((int32_t)g.cfg.seq_ovr.step_tune[v][g.step] +
                                       (int32_t)fm_seq_rand_pitch(g.cfg.seq_ovr.lane_tune[v], g.step, g.bar,
                                                                  (uint8_t)v, g.rng));
            /* Offsets ride in the event: the trigger fires after base_delay, when g_kit is refreshed. */
            int32_t t_off = p_off + (int32_t)tune_rnd;

            if (t_off > 127)
                t_off = 127;
            else if (t_off < -127)
                t_off = -127;
            if (g.perf == PERF_ARP && v == (g.cfg.sel_voice % RYTM_NDVOICES)) {
                uint8_t rate = g.cfg.arp_rate % 6u;
                if (rate == 2u || rate == 4u) {
                    dh.orn_kind = ORN_FLAM;
                    dh.orn_vel = rytm_clamp127((int32_t)dh.vel - 12 + ((int32_t)g.cfg.arp_spread >> 2));
                } else if (rate == 3u || rate == 5u) {
                    dh.orn_kind = ORN_ROLL;
                    dh.orn_vel = rytm_clamp127((int32_t)dh.vel - 8 + ((int32_t)g.cfg.arp_spread >> 2));
                }
            }
            rytm_synth_schedule_ornament(&g.synth, (uint8_t)v, &dh, sixteenth, base_delay, (int8_t)t_off, gate_rnd);
            midi_send(0x99u, GM_DRUM[v], dh.vel);
        }
    }
    if (ev->bass.active && voice_heard_audible(7u)) {
        rytm_bass_hit_t bh = ev->bass;
        rytm_bass_cfg_t bc = g.eff_bass;
        if (g.perf == PERF_ARP) {
            uint8_t pat = (uint8_t)((g.cfg.arp_solo * 4u) / 127u);
            int32_t ramp = (int32_t)g.cfg.arp_ramp - 64;
            if (pat == 1u)
                bh.midi_note = rytm_scale_note(g.cfg.root, g.cfg.scale, 7 - (int32_t)(g.step & 7u), 0);
            else if (pat == 2u && (g.step & 1u))
                bh.midi_note = (uint8_t)(bh.midi_note + 12u <= 84u ? bh.midi_note + 12u : bh.midi_note);
            else if (pat == 3u && (g.step & 1u))
                bh.midi_note = (uint8_t)(bh.midi_note + 7u <= 84u ? bh.midi_note + 7u : bh.midi_note);
            else if (pat == 4u) {
                bh.accent = 1u;
                bh.legato = (g.step & 1u);
                if ((g.step & 3u) == 2u && bh.midi_note + 12u <= 84u)
                    bh.midi_note = (uint8_t)(bh.midi_note + 12u);
            }
            if (ramp != 0)
                bc.cutoff = rytm_clamp127((int32_t)bc.cutoff + (ramp * (int32_t)(g.step & 15u)) / 8);
        }
        rytm_synth_schedule_bass(&g.synth, rytm_hit_delay(base_delay, bh.micro_ms), &bh, &bc);
        g.last_bass_note = bh.midi_note;
        g.live_bass_note = bh.midi_note;
        midi_send(0x90u, bh.midi_note, bh.vel);
        midi_send(0xB0u, 74u, bc.cutoff);
    }
}

static void seq_advance(void)
{
    g.tick++;
    if (g.tick % 6u)
        return;
    sync_effective_params();
    if (!g.playing || fm_seq_is_mute_quant_step(g.step)) {
        rytm_apply_lfo_seq_mutes();
        rytm_sync_heard_mutes();
    }
    {
        rytm_query_t q;
        rytm_step_events_t ev;
        build_query(&q);
        rytm_resolve_step(&q, &g.eff_seq_ovr, &ev);
        fire_step_events(&ev);
    }
    {
        uint32_t gbits[4] = {1u << KEY_BLACK[7], 1u << KEY_BLACK[8], 1u << KEY_BLACK[9], 1u << KEY_BLACK[10]};
        uint32_t gi;
        for (gi = 0; gi < 4u; gi++) {
            if (fm1_in.notes & gbits[gi]) {
                if (g.gest_hold[gi] < 250u)
                    g.gest_hold[gi]++;
                if (g.gest_hold[gi] == 32u && g.gesture != GEST_NONE) {
                    g.gesture_latch = g.gesture;
                    g.gest_latch_badge = 1u;
                }
            }
        }
    }
    g.step++;
    if (g.step >= seq_len())
        g.step = 0;
    if ((g.step & 15u) == 0u) {
        g.bar = (uint8_t)((g.bar + 1u) % RYTM_PHRASE_BARS);
        if (!g.cfg.seq_ovr.freeze)
            g.rng = g.rng * 1664525u + 1013904223u;
    }
    if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR) {
        uint32_t ml;
        rec_bass_note_extend(rec_map_step(g.step));
        for (ml = 0; ml < 8u; ml++) {
            if (fm1_ms - g.mot_last_ms[ml] <= 320u) {
                g.cfg.motion_val[ml][g.step & 15u] = g.mot_last_val[ml];
                g.mot_last_step[ml] = g.step & 15u;
            }
        }
    }
    g.ui_dirty = 1u;
}

static void midi_clock_tx(void)
{
    midi_send(0xF8u, 0, 0);
}

static void midi_start_tx(void)
{
    midi_send(0xFAu, 0, 0);
}

static void midi_stop_tx(void)
{
    midi_send(0xFCu, 0, 0);
}

static void live_trig_drum(uint8_t v, uint8_t vel)
{
    refresh_kit();
    rytm_synth_trig_drum(&g.synth, v, vel, vel > 100u ? 1u : 0u, &g_kit[v], g.cfg.root,
                         (uint8_t)(g.eff_fx.harmonic_toms || g.eff_prog >= 64u));
    midi_send(0x99u, GM_DRUM[v], vel);
}

static void live_trig_bass_note(uint8_t note, uint8_t vel)
{
    rytm_bass_hit_t h;
    h.active = 1;
    h.midi_note = note;
    h.vel = vel;
    h.accent = vel > 100u ? 1u : 0u;
    h.legato = 0;
    h.dead_ghost = 0;
    h.gate_pct = 80;
    h.filter_mod = g.eff_bass.env_mod;
    h.micro_ms = 0;
    rytm_synth_trig_bass(&g.synth, &h, &g.eff_bass);
    midi_send(0x90u, note, vel);
    g.live_bass_note = note;
    g.live_bass_vel = (int8_t)vel;
}

static void apply_cc(uint8_t cc, uint8_t val)
{
    switch (cc) {
    case 14:
        g.cfg.genre = val;
        motion_write(MOT_GENRE, val);
        break;
    case 15:
        g.cfg.density = val;
        motion_write(MOT_DENS, val);
        break;
    case 16:
        g.cfg.feel = val;
        motion_write(MOT_FEEL, val);
        break;
    case 17:
        g.cfg.flam = val;
        motion_write(MOT_FLAM, val);
        break;
    case 18:
        g.cfg.bpm = (uint8_t)(60u + ((uint32_t)val * 140u) / 127u);
        transport_set_bpm();
        break;
    case 19:
        g.cfg.swing_max = val;
        break;
    case 20:
        g.cfg.drummer = val % RYTM_NDRUMMERS;
        break;
    case 21:
        g.cfg.bassist = val % RYTM_NBASSISTS;
        g.cfg.bass.model = RYTM_BASSIST_DEFAULT_MODEL[g.cfg.bassist];
        break;
    case 22:
        g.cfg.root = val % 12u;
        break;
    case 23:
        g.cfg.scale = val % RYTM_NSCALES;
        break;
    case 24:
        g.view = val % 4u;
        break;
    default:
        return;
    }
    mark_dirty();
}

/* One detent is a useful jump on 0..127. Several detents in one poll (a spin) accelerate. */
static int32_t knob_gain(int32_t steps)
{
    int32_t a = steps < 0 ? -steps : steps;
    int32_t g = 4;

    if (a >= 2)
        g = 8;
    if (a >= 4)
        g = 16;
    return steps * g;
}

static void adj_u8(uint8_t *p, int32_t d, int32_t lo, int32_t hi)
{
    int32_t v = (int32_t)*p + d;
    if (v < lo)
        v = lo;
    if (v > hi)
        v = hi;
    *p = (uint8_t)v;
}

static void exec_rec_clear(void)
{
    rytm_cfg_t *c = &g.cfg;
    if (g.rec_clear_ix == 0u) {
        uint32_t i, p, s;
        for (i = 0; i < 8u; i++)
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                c->motion_val[i][s] = 255u;
        for (i = 0; i < 16u; i++)
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                c->macro_motion[i][s] = 255u;
        for (i = 0; i < RYTM_NDVOICES; i++)
            for (p = 0; p < 8u; p++)
                for (s = 0; s < RYTM_MOTION_STEPS; s++)
                    c->drum_motion[i][p][s] = 255u;
    } else if (g.rec_clear_ix == 1u) {
        fm_seq_bits_fill(c->seq_ovr.bass_force_on, 0);
        fm_seq_bits_fill(c->seq_ovr.bass_force_off, 0);
        memset(c->seq_ovr.bass_note, 0, sizeof c->seq_ovr.bass_note);
        memset(c->seq_ovr.bass_acc, 0, sizeof c->seq_ovr.bass_acc);
        memset(c->seq_ovr.bass_gate, 0, sizeof c->seq_ovr.bass_gate);
    } else if (g.rec_clear_ix == 2u) {
        uint8_t act[REC_TOTAL_SLOTS];
        uint32_t s, n = rec_build_active_list(act);
        if (n > 0u) {
            uint8_t id = act[g.rec_lane % n];
            uint8_t *arr = rec_slot_arr(id);
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                arr[s] = 255u;
            if (g.rec_lane > 0u && g.rec_lane >= n - 1u)
                g.rec_lane = (uint8_t)(n - 2u);
        }
    } else {
        clear_all_overlays();
    }
    mark_dirty();
}

static uint8_t sel_split_mode(void)
{
    uint32_t btns = fm1_in.buttons;
    if (g.perf == PERF_ARP || g.perf == PERF_SEQ || g.view == VIEW_REC)
        return 0u;
    if (g.bank == BANK_GROOVE && bank_page() == 0u) {
        if (btns & (1u << BTN_ENV))
            return 1u;
        if (btns & (1u << BTN_GLO))
            return 2u;
    }
    if ((btns & (1u << BTN_SEL)) && g.bank == BANK_DRUM)
        return 1u;
    if ((btns & (1u << BTN_SEL)) && g.bank == BANK_BASS)
        return 2u;
    return 0u;
}

static inline void rytm_edit_knob(int32_t steps)
{
    if (steps != 0)
        g.edit_used = 1u;
}

static inline void rytm_edit_u8(int32_t steps, uint8_t edit, uint8_t *v, uint8_t def, uint8_t max)
{
    if (!edit || steps == 0)
        return;
    rytm_edit_knob(steps);
    *v = steps < 0 ? def : max;
}

static void knob_adjust(uint8_t slot, int32_t steps)
{
    int32_t d = knob_gain(steps);
    rytm_cfg_t *c = &g.cfg;
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    uint8_t sp = sel_split_mode();

    if (fm1_in.buttons & ((1u << BTN_ENV) | (1u << BTN_GLO) | (1u << BTN_SEL) | (1u << BTN_LFO)))
        g.mod_btn_used = 1u;
    if (edit && steps != 0)
        rytm_edit_knob(steps);

    {
        uint8_t lt = lfo_target_for_knob(slot);
        if (lt != 255u) {
            c->lfo.target = lt;
            if (lt >= LFO_TGT_DRUM_TUNE && lt <= LFO_TGT_DRUM_PAN)
                c->lfo.drum_v[lt - LFO_TGT_DRUM_TUNE] = c->sel_voice % RYTM_NDVOICES;
            if (lt >= LFO_TGT_SEQ_PAT && lt <= LFO_TGT_SEQ_PITCH)
                c->lfo.seq_v[lt - LFO_TGT_SEQ_PAT] = g.seq_lane % 8u;
            if (fm1_in.buttons & (1u << BTN_LFO)) {
                if (edit)
                    c->lfo.depth[lt] = steps < 0 ? 0u : 127u;
                else
                    adj_u8(&c->lfo.depth[lt], d, 0, 127);
                mark_dirty();
                return;
            }
        }
    }

    if (sp != 0u) {
        if (slot == 3) {
            if (edit && steps != 0) {
                c->bpm = steps < 0 ? 120u : 200u;
                transport_set_bpm();
            } else if (!edit) {
                int32_t v = (int32_t)c->bpm + d;
                if (v < 60)
                    v = 60;
                if (v > 200)
                    v = 200;
                c->bpm = (uint8_t)v;
                transport_set_bpm();
            }
        } else if (sp == 1u) {
            uint8_t *p = slot == 0 ? &c->density : slot == 1 ? &c->feel : &c->flam;
            rytm_edit_u8(steps, edit, p, 0u, 127u);
            if (!edit || steps == 0) {
                adj_u8(p, d, 0, 127);
                if (slot == 0)
                    motion_write(MOT_DENS, *p);
                else if (slot == 1)
                    motion_write(MOT_FEEL, *p);
                else
                    motion_write(MOT_FLAM, *p);
            }
        } else {
            uint8_t *p = slot == 0 ? &c->bass_density : slot == 1 ? &c->bass_feel : &c->bass_flam;
            rytm_edit_u8(steps, edit, p, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(p, d, 0, 127);
        }
        mark_dirty();
        return;
    }

    if (g.view == VIEW_REC) {
        if (slot == 0) {
            if (edit && steps != 0)
                c->rec_bars = steps < 0 ? 0u : 3u;
            else if (!edit) {
                int32_t v = (int32_t)c->rec_bars + steps;
                if (v < 0)
                    v = 0;
                if (v > 3)
                    v = 3;
                c->rec_bars = (uint8_t)v;
            }
        } else if (slot == 1) {
            if (edit && steps != 0)
                c->rec_mode = steps < 0 ? 0u : 2u;
            else if (!edit) {
                int32_t v = (int32_t)c->rec_mode + steps;
                if (v < 0)
                    v = 0;
                if (v > 2)
                    v = 2;
                c->rec_mode = (uint8_t)v;
            }
        } else if (slot == 2) {
            if (edit && steps != 0)
                c->rec_quant = steps < 0 ? 0u : 2u;
            else if (!edit) {
                int32_t v = (int32_t)c->rec_quant + steps;
                if (v < 0)
                    v = 0;
                if (v > 2)
                    v = 2;
                c->rec_quant = (uint8_t)v;
            }
        } else if (steps) {
            if (edit && steps > 0) {
                exec_rec_clear();
            } else if (!edit) {
                int32_t v = (int32_t)g.rec_clear_ix + (steps > 0 ? 1 : -1);
                if (v < 0)
                    v = 0;
                if (v > 3)
                    v = 3;
                g.rec_clear_ix = (uint8_t)v;
            } else if (edit && steps < 0)
                g.rec_clear_ix = 0u;
        }
        mark_dirty();
        return;
    }
    if (g.perf == PERF_LIVE && g.bank == BANK_HOME) {
        uint8_t pg = bank_page() % 4u;
        uint8_t ix = (uint8_t)(pg * 4u + slot);
        if (fm1_in.buttons & (1u << BTN_LFO)) {
            uint8_t lt = (uint8_t)(LFO_TGT_MACRO0 + ix);
            c->lfo.target = lt;
            if (edit)
                c->lfo.depth[lt] = steps < 0 ? 0u : 127u;
            else
                adj_u8(&c->lfo.depth[lt], d, 0, 127);
            mark_dirty();
            return;
        }
        if (edit && steps != 0) {
            uint32_t s;
            g.home_macro[ix] = steps < 0 ? HOME_MACRO_DEF[ix] : 127u;
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                c->macro_motion[ix][s] = 255u;
        } else if (!edit) {
            adj_u8(&g.home_macro[ix], d, 0, 127);
            macro_motion_write(ix, g.home_macro[ix]);
        }
        mark_dirty();
        return;
    }
    if (g.perf == PERF_LIVE && (g.view == VIEW_LFO || g.bank == BANK_LFO)) {
        rytm_lfo_cfg_t *l = &c->lfo;
        uint8_t t = l->target % LFO_NTARGETS;
        uint8_t pg = bank_page();
        if (pg == 0u) {
            if (slot == 0) {
                int32_t v = (int32_t)l->target + steps;
                if (edit && steps != 0)
                    l->target = steps < 0 ? 0u : (uint8_t)(LFO_NTARGETS - 1u);
                else if (!edit) {
                    while (v < 0)
                        v += (int32_t)LFO_NTARGETS;
                    l->target = (uint8_t)((uint32_t)v % LFO_NTARGETS);
                }
            } else if (slot == 1) {
                if (edit && steps != 0)
                    l->rate[t] = steps < 0 ? 0u : 6u;
                else if (!edit) {
                    int32_t v = (int32_t)l->rate[t] + steps;
                    if (v < 0)
                        v = 0;
                    if (v > 6)
                        v = 6;
                    l->rate[t] = (uint8_t)v;
                }
            } else if (slot == 2) {
                rytm_edit_u8(steps, edit, &l->depth[t], 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&l->depth[t], d, 0, 127);
            } else if (edit && steps != 0)
                l->wave[t] = steps < 0 ? 0u : 127u;
            else
                l->wave[t] = fm_lfo_step_shape(l->wave[t], steps, 0u);
        } else if (pg == 1u) {
            if (slot == 0) {
                rytm_edit_u8(steps, edit, &l->phase[t], 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&l->phase[t], d, 0, 127);
            } else if (slot == 1) {
                rytm_edit_u8(steps, edit, &l->skew[t], FM_LFO_SKEW_DEFAULT, 127u);
                if (!edit || steps == 0)
                    adj_u8(&l->skew[t], d, 0, 127);
            } else if (slot == 2) {
                rytm_edit_u8(steps, edit, &l->smooth[t], 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&l->smooth[t], d, 0, 127);
            } else {
                rytm_edit_u8(steps, edit, &l->fold[t], 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&l->fold[t], d, 0, 127);
            }
        } else if (slot == 0) {
            int32_t v = (int32_t)l->algo[t] + steps;
            if (edit && steps != 0)
                l->algo[t] = steps < 0 ? 0u : (uint8_t)(FM_LFO_NALGOS - 1u);
            else if (!edit) {
                while (v < 0)
                    v += (int32_t)FM_LFO_NALGOS;
                l->algo[t] = (uint8_t)((uint32_t)v % FM_LFO_NALGOS);
            }
        } else if (slot == 1) {
            rytm_edit_u8(steps, edit, &l->ratio[t], 52u, 127u);
            if (!edit || steps == 0)
                adj_u8(&l->ratio[t], d, 0, 127);
        } else if (slot == 2) {
            rytm_edit_u8(steps, edit, &l->index[t], 64u, 127u);
            if (!edit || steps == 0)
                adj_u8(&l->index[t], d, 0, 127);
        } else {
            rytm_edit_u8(steps, edit, &l->fb[t], 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&l->fb[t], d, 0, 127);
        }
        mark_dirty();
        return;
    }
    if (g.perf == PERF_SEQ && g.seq_plock_hold) {
        uint8_t lane = g.seq_lane % 8u;
        uint8_t st = (uint8_t)(g.seq_plock_step % RYTM_NSTEPS);
        if (lane < RYTM_NDVOICES) {
            fm_seq_bit_put(c->seq_ovr.drum_force_on[lane], st, 1);
            fm_seq_bit_put(c->seq_ovr.drum_force_off[lane], st, 0);
        } else {
            fm_seq_bit_put(c->seq_ovr.bass_force_on, st, 1);
            fm_seq_bit_put(c->seq_ovr.bass_force_off, st, 0);
        }
        if (slot == 0) {
            if (edit)
                c->seq_ovr.step_vel[lane][st] = steps < 0 ? 100u : 127u;
            else
                adj_u8(&c->seq_ovr.step_vel[lane][st], d, 1, 127);
        } else if (slot == 1) {
            if (edit)
                c->seq_ovr.step_prob[lane][st] = steps < 0 ? FM_SEQ_COND_MAX : 0u;
            else {
                int32_t v = (int32_t)c->seq_ovr.step_prob[lane][st] + (steps > 0 ? -1 : 1);
                if (v < 0)
                    v = 0;
                if (v > (int32_t)FM_SEQ_COND_MAX)
                    v = (int32_t)FM_SEQ_COND_MAX;
                c->seq_ovr.step_prob[lane][st] = (uint8_t)v;
            }
        } else if (slot == 2) {
            if (edit)
                c->seq_ovr.step_micro[lane][st] = steps < 0 ? (int8_t)FM_SEQ_MICRO_MIN : (int8_t)FM_SEQ_MICRO_MAX;
            else {
                int32_t v = (int32_t)c->seq_ovr.step_micro[lane][st] + steps;

                c->seq_ovr.step_micro[lane][st] = fm_seq_micro_clamp(v);
            }
        } else if (lane == 7u) {
            int32_t def_n = 36 + (int32_t)(c->root % 12u);
            if (edit && steps != 0)
                c->seq_ovr.bass_note[st] = steps < 0 ? (uint8_t)def_n : 84u;
            else if (!edit) {
                int32_t note = (int32_t)(c->seq_ovr.bass_note[st] ? c->seq_ovr.bass_note[st] : (uint8_t)def_n) + steps;
                if (note < 24)
                    note = 24;
                if (note > 84)
                    note = 84;
                c->seq_ovr.bass_note[st] = (uint8_t)note;
            }
        } else if (edit) {
            c->seq_ovr.step_tune[lane][st] = steps < 0 ? 0 : 24;
        } else {
            int32_t v = (int32_t)c->seq_ovr.step_tune[lane][st] + steps;
            if (v < -24)
                v = -24;
            if (v > 24)
                v = 24;
            c->seq_ovr.step_tune[lane][st] = (int8_t)v;
        }
        mark_dirty();
        return;
    }

    if (g.perf == PERF_ARP) {
        if (slot == 0) {
            if (edit && steps != 0)
                c->arp_rate = steps < 0 ? 0u : 5u;
            else if (!edit) {
                int32_t v = (int32_t)c->arp_rate + steps;
                if (v < 0)
                    v = 0;
                if (v > 5)
                    v = 5;
                c->arp_rate = (uint8_t)v;
            }
        } else if (slot == 2) {
            if (edit && steps != 0)
                c->arp_solo = steps < 0 ? 0u : 127u;
            else if (!edit) {
                int32_t cur = ((int32_t)c->arp_solo * 4 + 63) / 127;
                int32_t nv = cur + (steps > 0 ? 1 : -1);
                if (nv < 0)
                    nv = 0;
                if (nv > 4)
                    nv = 4;
                c->arp_solo = (uint8_t)((nv * 127) / 4);
            }
        } else {
            uint8_t *p = slot == 1 ? &c->arp_spread : &c->arp_ramp;
            rytm_edit_u8(steps, edit, p, 64u, 127u);
            if (!edit || steps == 0)
                adj_u8(p, d, 0, 127);
        }
        mark_dirty();
        return;
    }
    if (g.perf == PERF_SEQ) {
        uint8_t ln = g.seq_lane % 8u;
        if (g.seq_page == 2u) {
            if (slot == 0) {
                if (edit)
                    g.seq_lane = steps < 0 ? 0u : 7u;
                else {
                    g.seq_lane = (uint8_t)(((int32_t)g.seq_lane + (steps > 0 ? 1 : 7)) % 8);
                    if (g.seq_lane < RYTM_NDVOICES)
                        c->sel_voice = g.seq_lane;
                }
            } else if (slot == 1) {
                if (edit)
                    rytm_lane_apply_p3_state(ln, steps < 0 ? 1u : 2u);
                else if (steps != 0) {
                    int32_t pos = (int32_t)rytm_lane_p3_state_pos(ln) + (steps > 0 ? 1 : -1);
                    if (pos < 0)
                        pos = 0;
                    if (pos > 2)
                        pos = 2;
                    rytm_lane_apply_p3_state(ln, (uint8_t)pos);
                }
            } else if (slot == 2) {
                if (edit)
                    rytm_apply_ensemble(steps < 0 ? FM_SEQ_ENS_MUTE : FM_SEQ_ENS_ALL);
                else if (steps != 0) {
                    int32_t ix = (int32_t)g.seq_ens_sel + (steps > 0 ? 1 : -1);
                    if (ix < 0)
                        ix = 0;
                    if (ix >= (int32_t)FM_SEQ_NENSEMBLES)
                        ix = (int32_t)FM_SEQ_NENSEMBLES - 1;
                    rytm_apply_ensemble((uint8_t)ix);
                }
            } else if (steps != 0) {
                rytm_invert_all_mutes();
                g.seq_flip_state ^= 1u;
            }
            mark_dirty();
            return;
        }
        if (g.seq_page == 1u) {
            uint8_t fn = fm_seq_p1_fn((uint8_t)slot);

            if (fn == 1u) {
                if (edit)
                    c->seq_ovr.lane_trig[ln] = steps < 0 ? 0 : (int8_t)-100;
                else
                    c->seq_ovr.lane_trig[ln] = fm_seq_trig_adj(c->seq_ovr.lane_trig[ln], d, 0);
            } else {
                int8_t *p = fn == 0u ? &c->seq_ovr.lane_vel[ln] :
                            fn == 2u ? &c->seq_ovr.lane_gate[ln] : &c->seq_ovr.lane_tune[ln];
                int32_t lim = fn == 3u ? 24 : 50;
                if (edit)
                    *p = steps < 0 ? 0 : (int8_t)lim;
                else {
                    int32_t v = (int32_t)*p + (fn == 3u ? steps : d);
                    if (v < -lim)
                        v = -lim;
                    if (v > lim)
                        v = lim;
                    *p = (int8_t)v;
                }
            }
            mark_dirty();
            return;
        }
        if (slot == 0) {
            if (edit)
                g.seq_lane = steps < 0 ? 0u : 7u;
            else {
                g.seq_lane = (uint8_t)(((int32_t)g.seq_lane + (steps > 0 ? 1 : 7)) % 8);
                if (g.seq_lane < RYTM_NDVOICES)
                    c->sel_voice = g.seq_lane;
            }
        } else if (slot == 1) {
            if (edit)
                c->euclid_hits[ln] = steps < 0 ? FM_SEQ_EUCLID_SONG : 16u;
            else {
                uint8_t oh = c->euclid_hits[ln];
                uint8_t nh = fm_seq_euclid_step(oh, steps, 0, g.pattern_origin[ln], 1);

                if (nh == FM_SEQ_EUCLID_EDITED) {
                    if (oh != FM_SEQ_EUCLID_EDITED)
                        fm_seq_lane_pattern_mark_edited(&c->euclid_hits[ln], &g.pattern_origin[ln]);
                } else {
                    if (oh == FM_SEQ_EUCLID_EDITED && steps > 0 &&
                        g.pattern_origin[ln] != FM_SEQ_PATTERN_ORIGIN_NONE)
                        g.pattern_origin[ln] = FM_SEQ_PATTERN_ORIGIN_NONE;
                    if (nh == FM_SEQ_EUCLID_FRESH)
                        c->euclid_hits[ln] = FM_SEQ_EUCLID_SONG;
                    else
                        c->euclid_hits[ln] = nh;
                }
            }
            if (c->euclid_hits[ln] != FM_SEQ_EUCLID_EDITED)
                rytm_apply_euclid_hits(ln, c->euclid_hits[ln]);
        } else if (slot == 2) {
            if (edit)
                c->euclid_rot[ln] = steps < 0 ? 0 : 15;
            else {
                int32_t step1 = steps > 0 ? 1 : -1;
                int32_t nr = (int32_t)fm_seq_euclid_rot_norm(c->euclid_rot[ln]) + step1;

                c->euclid_rot[ln] = (int8_t)fm_seq_euclid_rot_norm(nr);
                seq_rotate_lane(ln, step1);
            }
        } else if (slot == 3) {
            if (edit)
                c->seq_ovr.vary_lane[ln] = steps < 0 ? 0u : 127u;
            else {
                int32_t v = (int32_t)c->seq_ovr.vary_lane[ln] + d * 3;
                if (v < 0)
                    v = 0;
                if (v > 127)
                    v = 127;
                c->seq_ovr.vary_lane[ln] = (uint8_t)v;
            }
        }
        mark_dirty();
        return;
    }

    if (g.bank == BANK_GROOVE) {
        if (bank_page() == 0u) {
            uint8_t *pd = slot == 0 ? &c->density : slot == 1 ? &c->feel : slot == 2 ? &c->flam : 0;
            uint8_t *pb = slot == 0 ? &c->bass_density : slot == 1 ? &c->bass_feel : slot == 2 ? &c->bass_flam : 0;
            if (slot == 3) {
                if (edit && steps != 0) {
                    c->bpm = steps < 0 ? 120u : 200u;
                    transport_set_bpm();
                } else if (!edit) {
                    int32_t v = (int32_t)c->bpm + d;
                    if (v < 60)
                        v = 60;
                    if (v > 200)
                        v = 200;
                    c->bpm = (uint8_t)v;
                    transport_set_bpm();
                }
            } else if (pd && pb) {
                if (edit && steps != 0) {
                    *pd = steps < 0 ? 0u : 127u;
                    *pb = *pd;
                } else if (!edit) {
                    adj_u8(pd, d, 0, 127);
                    adj_u8(pb, d, 0, 127);
                }
                if (slot == 0)
                    motion_write(MOT_DENS, *pd);
                if (slot == 1)
                    motion_write(MOT_FEEL, *pd);
                if (slot == 2)
                    motion_write(MOT_FLAM, *pd);
            }
        } else if (slot == 0) {
            rytm_edit_u8(steps, edit, &c->swing_max, 0u, 100u);
            if (!edit || steps == 0)
                adj_u8(&c->swing_max, d, 0, 100);
        } else if (slot == 1) {
            rytm_edit_u8(steps, edit, &c->human, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->human, d, 0, 127);
        } else if (slot == 2) {
            if (edit && steps != 0) {
                if (steps < 0)
                    c->drummer_auto = 1u;
                else {
                    c->drummer_auto = 0u;
                    c->drummer = (uint8_t)(RYTM_NDRUMMERS - 1u);
                }
            } else if (!edit) {
                int32_t cur = c->drummer_auto ? 0 : ((int32_t)(c->drummer % RYTM_NDRUMMERS) + 1);
                int32_t v = cur + steps;
                while (v < 0)
                    v += (int32_t)(RYTM_NDRUMMERS + 1u);
                v %= (int32_t)(RYTM_NDRUMMERS + 1u);
                if (v == 0) {
                    c->drummer_auto = 1u;
                } else {
                    c->drummer_auto = 0u;
                    c->drummer = (uint8_t)(v - 1);
                }
            }
        } else if (edit && steps != 0) {
            if (steps < 0) {
                c->bassist_auto = 1u;
            } else {
                c->bassist_auto = 0u;
                c->bassist = (uint8_t)(RYTM_NBASSISTS - 1u);
                c->bass.model = RYTM_BASSIST_DEFAULT_MODEL[c->bassist];
            }
        } else if (!edit) {
            int32_t cur = c->bassist_auto ? 0 : ((int32_t)(c->bassist % RYTM_NBASSISTS) + 1);
            int32_t v = cur + steps;
            while (v < 0)
                v += (int32_t)(RYTM_NBASSISTS + 1u);
            v %= (int32_t)(RYTM_NBASSISTS + 1u);
            if (v == 0) {
                c->bassist_auto = 1u;
            } else {
                c->bassist_auto = 0u;
                c->bassist = (uint8_t)(v - 1);
                c->bass.model = RYTM_BASSIST_DEFAULT_MODEL[c->bassist];
            }
        }
    } else if (g.bank == BANK_DRUM) {
        uint8_t sv = c->sel_voice % RYTM_NDVOICES;
        rytm_dvoice_cfg_t *dv = &c->dv[sv];
        uint8_t dp = (uint8_t)((bank_page() & 1u) * 4u + slot);
        if (edit && steps != 0) {
            uint32_t s;
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                c->drum_motion[sv][dp][s] = 255u;
        }
        if (bank_page() == 0u) {
            uint8_t *p = slot == 0 ? &dv->tune : slot == 1 ? &dv->decay : slot == 2 ? &dv->tone : &dv->drive;
            rytm_edit_u8(steps, edit, p, 64u, 127u);
            if (!edit || steps == 0) {
                adj_u8(p, d, 0, 127);
                drum_motion_write(sv, dp, *p);
            }
        } else if (slot == 0) {
            rytm_edit_u8(steps, edit, &dv->level, 64u, 127u);
            if (!edit || steps == 0) {
                uint8_t old = dv->level;
                adj_u8(&dv->level, d, 0, 127);
                if (dv->level == 0u)
                    dv->mute = 1u;
                else if (dv->mute && steps > 0)
                    dv->mute = 0u;
                if (old == 0u && dv->level > 0u && dv->level < 90u)
                    dv->level = 90u;
                drum_motion_write(sv, dp, dv->level);
            }
        } else if (slot == 1) {
            rytm_edit_u8(steps, edit, &dv->send_dly, 64u, 127u);
            if (!edit || steps == 0) {
                adj_u8(&dv->send_dly, d, 0, 127);
                drum_motion_write(sv, dp, dv->send_dly);
            }
        } else if (slot == 2) {
            uint8_t def_rev = (sv == DV_SNARE || sv == DV_CLAP || sv == DV_OH) ? 56u : 24u;
            rytm_edit_u8(steps, edit, &dv->send_rev, def_rev, 127u);
            if (!edit || steps == 0) {
                adj_u8(&dv->send_rev, d, 0, 127);
                drum_motion_write(sv, dp, dv->send_rev);
            }
        } else {
            rytm_edit_u8(steps, edit, &dv->pan, RYTM_DEF_PAN[sv], 127u);
            if (!edit || steps == 0) {
                if (dv->pan == 0u)
                    dv->pan = RYTM_DEF_PAN[sv];
                adj_u8(&dv->pan, d, 1, 127);
                drum_motion_write(sv, dp, dv->pan);
            }
        }
    } else if (g.bank == BANK_BASS) {
        if (bank_page() == 0u) {
            if (slot == 0) {
                if (edit && steps != 0)
                    c->bass.model = steps < 0 ? 0u : (uint8_t)(BMODEL_COUNT - 1u);
                else if (!edit) {
                    int32_t v = (int32_t)c->bass.model + steps;
                    while (v < 0)
                        v += BMODEL_COUNT;
                    c->bass.model = (uint8_t)(v % BMODEL_COUNT);
                }
            } else {
                uint8_t *p = slot == 1 ? &c->bass.cutoff : slot == 2 ? &c->bass.reso : &c->bass.decay;
                rytm_edit_u8(steps, edit, p, 0u, 127u);
                if (!edit || steps == 0) {
                    adj_u8(p, d, 0, 127);
                    if (slot == 1)
                        motion_write(MOT_BCUT, *p);
                    if (slot == 2)
                        motion_write(MOT_BRES, *p);
                }
            }
        } else if (bank_page() == 2u) {
            if (slot == 0) {
                if (edit && steps != 0)
                    c->root = steps < 0 ? 0u : 11u;
                else if (!edit) {
                    int32_t v = (int32_t)c->root + steps;
                    while (v < 0)
                        v += 12;
                    c->root = (uint8_t)(v % 12);
                }
            }
        } else if (slot == 0) {
            rytm_edit_u8(steps, edit, &c->bass.env_mod, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->bass.env_mod, d, 0, 127);
        } else if (slot == 1) {
            rytm_edit_u8(steps, edit, &c->bass.glide, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->bass.glide, d, 0, 127);
        } else if (slot == 2) {
            if (edit && steps != 0)
                c->scale = steps < 0 ? 0u : (uint8_t)(RYTM_NSCALES - 1u);
            else if (!edit) {
                int32_t v = (int32_t)c->scale + steps;
                while (v < 0)
                    v += RYTM_NSCALES;
                c->scale = (uint8_t)(v % RYTM_NSCALES);
            }
        } else {
            rytm_edit_u8(steps, edit, &c->prog_depth, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->prog_depth, d, 0, 127);
        }
    } else if (g.bank == BANK_FX) {
        if (bank_page() == 0u) {
            uint8_t *p = slot == 0 ? &c->fx.dly_mix : slot == 1 ? &c->fx.rev_mix : slot == 2 ? &c->fx.pump : &c->fx.db_bal;
            rytm_edit_u8(steps, edit, p, 0u, 127u);
            if (!edit || steps == 0) {
                adj_u8(p, d, 0, 127);
                if (slot == 0)
                    motion_write(MOT_DUB, *p);
            }
        } else if (slot == 0) {
            rytm_edit_u8(steps, edit, &c->fx.dly_time, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->fx.dly_time, d, 0, 127);
        } else if (slot == 1) {
            rytm_edit_u8(steps, edit, &c->fx.dly_fb, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->fx.dly_fb, d, 0, 127);
        } else if (slot == 2) {
            rytm_edit_u8(steps, edit, &c->fx_rev_size, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->fx_rev_size, d, 0, 127);
        } else {
            rytm_edit_u8(steps, edit, &c->fx.master_drive, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&c->fx.master_drive, d, 0, 127);
        }
    }
    mark_dirty();
}

static void knob_label(uint8_t slot, char *s)
{
    uint8_t sp = sel_split_mode();
    if (sp == 1u) {
        static const char *const L[4] = {"D.DENS", "D.FEEL", "D.FLAM", "TEMPO"};
        copy_str(s, L[slot]);
        return;
    }
    if (sp == 2u) {
        static const char *const L[4] = {"B.DENS", "B.FEEL", "B.SLID", "TEMPO"};
        copy_str(s, L[slot]);
        return;
    }
    if (g.perf == PERF_ARP) {
        static const char *const L[4] = {"ROLL RT", "FLAM SP", "SOLO T", "P RAMP"};
        copy_str(s, L[slot]);
        return;
    }
    if (g.view == VIEW_REC) {
        static const char *const L[4] = {"LENGTH", "MODE", "QUANT", "CLEAR"};
        copy_str(s, L[slot]);
        return;
    }
    if (g.perf == PERF_SEQ) {
        copy_str(s, fm_seq_knob_label(g.seq_page, g.seq_plock_hold, slot));
        return;
    }
    if (g.bank == BANK_HOME) {
        static const char *const L[4][4] = {
            {"PUNCH", "SWEEP", "THROW", "HYPE"},
            {"D.TUNE", "D.DECAY", "D.TONE", "D.DRIVE"},
            {"ACID", "B.BODY", "B.GLIDE", "HARM"},
            {"ENERGY", "POCKET", "ORNAM", "SPACE"}
        };
        copy_str(s, L[bank_page() % 4u][slot]);
        return;
    }
    if (g.view == VIEW_LFO || g.bank == BANK_LFO) {
        uint8_t pg = bank_page();
        if (pg == 0u) {
            static const char *const L[4] = {"TARGET", "RATE", "DEPTH", "SHAPE"};
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
            static const char *const L[4] = {"PHASE", "SKEW", "SMOOTH", "FOLD"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"ALGO", "RATIO", "FM IDX", "OP FB"};
            copy_str(s, L[slot]);
        }
        return;
    }
    if (g.bank == BANK_GROOVE) {
        if (bank_page() == 0u) {
            static const char *const L[4] = {"DENSITY", "FEEL", "FLAM", "TEMPO"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"SWING", "HUMAN", "DRUMMER", "BASSIST"};
            copy_str(s, L[slot]);
        }
    } else if (g.bank == BANK_DRUM) {
        if (bank_page() == 0u) {
            static const char *const L[4] = {"TUNE", "DECAY", "TONE", "DRIVE"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"LEVEL", "DELAY", "REVERB", "PAN"};
            copy_str(s, L[slot]);
        }
    } else if (g.bank == BANK_BASS) {
        if (bank_page() == 0u) {
            static const char *const L[4] = {"MODEL", "CUTOFF", "RESO", "DECAY"};
            copy_str(s, L[slot]);
        } else if (bank_page() == 2u) {
            copy_str(s, slot == 0 ? "KEY" : "");
        } else {
            static const char *const L[4] = {"ENV MOD", "GLIDE", "SCALE", "PROG"};
            copy_str(s, L[slot]);
        }
    } else if (g.bank == BANK_FX) {
        if (bank_page() == 0u) {
            static const char *const L[4] = {"DLY MIX", "ROOM", "PUMP", "DRM/BAS"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"DL TIME", "DLY FB", "ROOM SZ", "DRIVE"};
            copy_str(s, L[slot]);
        }
    }
}

/* Display: 0 = unset → voice default (RYTM_DEF_PAN). Same mapping as KO DUB fmt_pan. */
static uint8_t rytm_pan_eff(uint8_t pan, uint8_t voice)
{
    return pan ? pan : RYTM_DEF_PAN[voice % RYTM_NDVOICES];
}

static int32_t pan_parts(uint8_t pan, char *num)
{
    int32_t pn = (int32_t)pan - 64;
    uint32_t val;

    if (pn == 0) {
        num[0] = 0;
        return 0;
    }
    val = 1u + ((uint32_t)(pn < 0 ? -pn : pn) - 1u) * 99u / 62u;
    if (val > 100u)
        val = 100u;
    if (val >= 100u) {
        num[0] = '1';
        num[1] = '0';
        num[2] = '0';
        num[3] = 0;
    } else if (val >= 10u) {
        num[0] = (char)('0' + val / 10u);
        num[1] = (char)('0' + val % 10u);
        num[2] = 0;
    } else {
        num[0] = (char)('0' + val);
        num[1] = 0;
    }
    return pn;
}

static void fmt_pan_voice(char *s, uint8_t pan, uint8_t voice)
{
    char num[4];
    uint8_t eff = rytm_pan_eff(pan, voice);
    int32_t pn = pan_parts(eff, num);
    char *p = s;

    if (pn == 0) {
        copy_str(s, "CTR");
        return;
    }
    if (pn < 0) {
        *p++ = 'L';
        *p++ = ' ';
        for (const char *c = num; *c;)
            *p++ = *c++;
        *p = 0;
        return;
    }
    for (const char *c = num; *c;)
        *p++ = *c++;
    *p++ = ' ';
    *p++ = 'R';
    *p = 0;
}

static void knob_value(uint8_t slot, char *s)
{
    rytm_cfg_t *c = &g.cfg;
    uint32_t v = 0;
    uint8_t sp = sel_split_mode();
    if (sp == 1u) {
        v = slot == 0 ? c->density : slot == 1 ? c->feel : slot == 2 ? c->flam : c->bpm;
        if (slot == 3)
            u8_dec(s, v);
        else
            u8_show100_dec(s, v);
        return;
    }
    if (sp == 2u) {
        v = slot == 0 ? c->bass_density : slot == 1 ? c->bass_feel : slot == 2 ? c->bass_flam : c->bpm;
        if (slot == 3)
            u8_dec(s, v);
        else
            u8_show100_dec(s, v);
        return;
    }
    if (g.view == VIEW_REC) {
        static const char *const LEN[4] = {"1 BAR", "2 BARS", "4 BARS", "8 BARS"};
        static const char *const MOD[3] = {"OVERDUB", "REPLACE", "MONITOR"};
        static const char *const QNT[3] = {"FREE", "SLOP", "1/16"};
        static const char *const CLR[4] = {"MOTION", "NOTES", "LANE", "ALL"};
        if (slot == 0)
            copy_str(s, LEN[c->rec_bars % 4u]);
        else if (slot == 1)
            copy_str(s, MOD[c->rec_mode % 3u]);
        else if (slot == 2)
            copy_str(s, QNT[c->rec_quant % 3u]);
        else
            copy_str(s, CLR[g.rec_clear_ix % 4u]);
        return;
    }
    if (g.perf == PERF_LIVE && g.bank == BANK_HOME) {
        uint8_t pg = bank_page() % 4u;
        uint8_t ix = (uint8_t)(pg * 4u + slot);
        if (fm1_in.buttons & (1u << BTN_LFO)) {
            s[0] = '~';
            u8_show100_dec(s + 1, c->lfo.depth[LFO_TGT_MACRO0 + ix]);
            return;
        }
        {
            uint8_t mv = g.home_macro[ix];
            if (HOME_MACRO_DEF[ix] == 64u)
                bip50_show(s, mv);
            else
                u8_show100_dec(s, mv);
        }
        return;
    }
    if ((g.view == VIEW_LFO || g.bank == BANK_LFO) && g.perf != PERF_ARP) {
        uint8_t t = c->lfo.target % LFO_NTARGETS;
        uint8_t pg = bank_page();
        if (pg == 0u) {
            if (slot == 0) {
                if (rytm_is_seq_lfo_tgt(t))
                    rytm_fmt_lfo_seq_short(s, t);
                else
                    copy_str(s, LFO_TGT_NAMES[t]);
            } else if (slot == 1)
                copy_str(s, LFO_RATE_NAMES[c->lfo.rate[t] % 7u]);
            else if (slot == 2)
                u8_show100_dec(s, c->lfo.depth[t]);
            else
                copy_str(s, fm_lfo_shape_name(c->lfo.wave[t]));
        } else if (pg == 1u) {
            if (slot == 0)
                u8_show100_dec(s, c->lfo.phase[t]);
            else if (slot == 1)
                bip50_show(s, c->lfo.skew[t]);
            else if (slot == 2)
                u8_show100_dec(s, c->lfo.smooth[t]);
            else
                fm_lfo_fmt_depth(s, (int32_t)c->lfo.fold[t]);
        } else if (slot == 0)
            copy_str(s, FM_LFO_ALG_NAMES[c->lfo.algo[t] % FM_LFO_NALGOS]);
        else if (slot == 1)
            fm_lfo_fmt_ratio(s, c->lfo.ratio[t]);
        else if (slot == 2)
            fm_lfo_fmt_depth(s, (int32_t)c->lfo.index[t]);
        else
            fm_lfo_fmt_depth(s, (int32_t)c->lfo.fb[t]);
        return;
    }
    if (g.perf == PERF_ARP) {
        if (slot == 0) {
            copy_str(s, ARP_RATE_NAMES_V2[c->arp_rate % 6u]);
            return;
        }
        if (slot == 2) {
            copy_str(s, ARP_SOLO_NAMES[((c->arp_solo * 4u + 63u) / 127u) % 5u]);
            return;
        }
        if (slot == 3) {
            bip50_show(s, c->arp_ramp);
            return;
        }
        v = c->arp_spread;
    } else if (g.perf == PERF_SEQ && g.seq_plock_hold) {
        uint8_t lane = g.seq_lane, st = g.seq_plock_step;
        if (slot == 0)
            v = c->seq_ovr.step_vel[lane][st] ? c->seq_ovr.step_vel[lane][st] : 100u;
        else if (slot == 1) {
            copy_str(s, FM_SEQ_COND_NAMES[c->seq_ovr.step_prob[lane][st] % FM_SEQ_NCONDS]);
            return;
        } else if (slot == 2) {
            i32_signed_dec(s, c->seq_ovr.step_micro[lane][st]);
            return;
        } else if (lane == 7u) {
            uint8_t n = c->seq_ovr.bass_note[st] ? c->seq_ovr.bass_note[st] : (uint8_t)(36u + c->root % 12u);
            copy_str(s, NOTE_NAMES[n % 12u]);
            u8_dec(s + rytm_strlen(s), n / 12u);
            return;
        } else {
            i32_signed_dec(s, c->seq_ovr.step_tune[lane][st]);
            return;
        }
    } else if (g.perf == PERF_SEQ) {
        uint8_t ln = g.seq_lane % 8u;
        uint8_t seq_lt = 255u;

        if ((fm1_in.buttons & (1u << BTN_LFO)) && rytm_seq_knob_lfo_map(slot, &seq_lt)) {
            s[0] = '~';
            u8_show100_dec(s + 1, c->lfo.depth[seq_lt]);
            return;
        }

        if (g.seq_page == 2u) {
            if (slot == 0)
                copy_str(s, ln < RYTM_NDVOICES ? RYTM_DVOICE_NAMES[ln] : "BASS");
            else if (slot == 1)
                copy_str(s, FM_SEQ_P3_STATE_NAMES[rytm_lane_p3_state_pos(ln)]);
            else if (slot == 2) {
                uint8_t det = rytm_detect_ensemble();
                copy_str(s, det == FM_SEQ_ENS_CUSTOM ? "CUSTOM" : FM_SEQ_ENS_NAMES[det]);
            } else
                copy_str(s, FM_SEQ_FLIP_NAMES[g.seq_flip_state & 1u]);
            return;
        }
        if (g.seq_page == 1u) {
            uint8_t fn = fm_seq_p1_fn((uint8_t)slot);

            if (fn == 1u) {
                fm_seq_fmt_trig(c->seq_ovr.lane_trig[ln], s);
                return;
            }
            {
                int8_t bv = fn == 0u ? c->seq_ovr.lane_vel[ln] : fn == 2u ? c->seq_ovr.lane_gate[ln] :
                            c->seq_ovr.lane_tune[ln];
                fm_seq_fmt_rand_spread(bv, s);
            }
            return;
        }
        if (slot == 0) {
            copy_str(s, ln < RYTM_NDVOICES ? RYTM_DVOICE_NAMES[ln] : "BASS");
            return;
        }
        if (slot == 3) {
            fm_seq_fmt_vary(c->seq_ovr.vary_lane[ln], s);
            return;
        }
        if (slot == 2) {
            u8_dec(s, (uint32_t)c->euclid_rot[ln]);
            return;
        }
        {
            uint8_t eh = c->euclid_hits[ln];

            if (eh == FM_SEQ_EUCLID_EMPTY)
                copy_str(s, seq_lane_clear_label(ln));
            else if (eh == FM_SEQ_EUCLID_SONG && !seq_lane_content_is_song(ln))
                copy_str(s, "EDITED");
            else
                fm_seq_fmt_euclid(eh, s);
        }
        return;
    } else if (g.bank == BANK_GROOVE) {
        if (bank_page() == 0u) {
            if (slot < 3u) {
                uint8_t d_v = slot == 0 ? c->density : slot == 1 ? c->feel : c->flam;
                uint8_t b_v = slot == 0 ? c->bass_density : slot == 1 ? c->bass_feel : c->bass_flam;
                if (d_v != b_v) {
                    u8_show100_dec(s, d_v);
                    copy_str(s + rytm_strlen(s), "/");
                    u8_show100_dec(s + rytm_strlen(s), b_v);
                    return;
                }
            }
            v = slot == 0 ? c->density : slot == 1 ? c->feel : slot == 2 ? c->flam : c->bpm;
            if (slot == 3) {
                u8_dec(s, v);
                return;
            }
        } else if (slot == 2) {
            copy_str(s, c->drummer_auto ? "AUTO" : RYTM_DRUMMER_NAMES[c->drummer % RYTM_NDRUMMERS]);
            return;
        } else if (slot == 3) {
            static const char *const BSHORT[8] = {"MINGUS", "JAMERSN", "BOOTSY", "JACO", "ROBBIE", "FLABBA", "FLEA", "CLAYPL"};
            copy_str(s, c->bassist_auto ? "AUTO" : BSHORT[c->bassist % RYTM_NBASSISTS]);
            return;
        } else if (slot == 0) {
            u8_dec(s, c->swing_max);
            return;
        } else
            v = c->human;
    } else if (g.bank == BANK_DRUM) {
        rytm_dvoice_cfg_t *dv = &c->dv[c->sel_voice % RYTM_NDVOICES];
        if (bank_page() == 0u) {
            uint8_t raw = slot == 0 ? dv->tune : slot == 1 ? dv->decay : slot == 2 ? dv->tone : dv->drive;
            bip50_show(s, raw);
            return;
        }
        if (slot == 3) {
            fmt_pan_voice(s, dv->pan, c->sel_voice);
            return;
        }
        if (bank_page() == 1u && slot == 0 && !voice_audible(c->sel_voice % RYTM_NDVOICES)) {
            copy_str(s, "MUTE");
            return;
        }
        v = slot == 0 ? dv->level : slot == 1 ? dv->send_dly : dv->send_rev;
    } else if (g.bank == BANK_BASS) {
        if (bank_page() == 0u) {
            if (slot == 0) {
                copy_str(s, BMODEL_SHORT[c->bass.model % BMODEL_COUNT]);
                return;
            }
            v = slot == 1 ? c->bass.cutoff : slot == 2 ? c->bass.reso : c->bass.decay;
        } else if (bank_page() == 2u) {
            copy_str(s, slot == 0 ? NOTE_NAMES[c->root % 12u] : "");
            return;
        } else if (slot == 2) {
            copy_str(s, RYTM_SCALE_NAMES[c->scale % RYTM_NSCALES]);
            return;
        } else
            v = slot == 0 ? c->bass.env_mod : slot == 1 ? c->bass.glide : c->prog_depth;
    } else if (g.bank == BANK_FX) {
        if (bank_page() == 0u) {
            if (slot == 3) {
                int32_t bal = (int32_t)(c->fx.db_bal > 127u ? 127u : c->fx.db_bal) - 64;
                uint32_t mag = (uint32_t)(((bal < 0 ? (bal < -63 ? 63 : -bal) : bal) * 50 + 31) / 63);
                if (bal == 0)
                    copy_str(s, "50/50");
                else if (bal < 0) {
                    copy_str(s, "DR +");
                    u8_dec(s + 4, mag);
                } else {
                    copy_str(s, "BS +");
                    u8_dec(s + 4, mag);
                }
                return;
            }
            v = slot == 0 ? c->fx.dly_mix : slot == 1 ? c->fx.rev_mix : c->fx.pump;
        } else
            v = slot == 0 ? c->fx.dly_time : slot == 1 ? c->fx.dly_fb : slot == 2 ? c->fx_rev_size : c->fx.master_drive;
    }
    u8_show100_dec(s, v);
}

static uint32_t knob_bar_px(uint8_t slot, uint32_t max_h)
{
    rytm_cfg_t *c = &g.cfg;
    uint32_t v = 0, mx = 127u;
    uint8_t sp = sel_split_mode();
    if (sp == 1u) {
        if (slot == 3) {
            v = c->bpm > 60u ? c->bpm - 60u : 0u;
            mx = 140u;
        } else
            v = slot == 0 ? c->density : slot == 1 ? c->feel : c->flam;
    } else if (sp == 2u) {
        if (slot == 3) {
            v = c->bpm > 60u ? c->bpm - 60u : 0u;
            mx = 140u;
        } else
            v = slot == 0 ? c->bass_density : slot == 1 ? c->bass_feel : c->bass_flam;
    } else if (g.view == VIEW_REC) {
        v = slot == 0 ? c->rec_bars : slot == 1 ? c->rec_mode : slot == 2 ? c->rec_quant : g.rec_clear_ix;
        mx = slot == 0 ? 3u : slot == 1 ? 2u : slot == 2 ? 2u : 3u;
    } else if (g.perf == PERF_ARP) {
        v = slot == 0 ? c->arp_rate : slot == 1 ? c->arp_spread : slot == 2 ? c->arp_solo : c->arp_ramp;
        mx = slot == 0 ? 5u : 127u;
    } else if (g.perf == PERF_SEQ) {
        uint8_t ln = g.seq_lane % 8u;
        if (g.seq_plock_hold) {
            uint8_t st = g.seq_plock_step;
            if (slot == 0) { v = c->seq_ovr.step_vel[ln][st] ? c->seq_ovr.step_vel[ln][st] : 100u; mx = 127u; }
            else if (slot == 1)
                return ((uint32_t)fm_seq_cond_bar_px(c->seq_ovr.step_prob[ln][st]) * max_h) / 127u;
            else if (slot == 2) {
                v = (uint32_t)((int32_t)c->seq_ovr.step_micro[ln][st] + FM_SEQ_MICRO_HALF);
                mx = (uint32_t)FM_SEQ_TIMING_UNITS;
            }
            else if (ln == 7u) {
                uint8_t n = c->seq_ovr.bass_note[st] ? c->seq_ovr.bass_note[st] : (uint8_t)(36u + c->root % 12u);
                v = n > 24u ? (n - 24u) : 0u;
                mx = 60u;
            } else { v = (uint32_t)((int32_t)c->seq_ovr.step_tune[ln][st] + 24); mx = 48u; }
        } else if (g.seq_page == 2u) {
            if (slot == 0) {
                v = ln;
                mx = 7u;
            } else if (slot == 1) {
                uint8_t pos = rytm_lane_p3_state_pos(ln);
                v = pos == 0u ? 0u : pos == 1u ? 64u : 127u;
                mx = 127u;
            } else if (slot == 2) {
                uint8_t det = rytm_detect_ensemble();
                if (det == FM_SEQ_ENS_CUSTOM) {
                    uint32_t i;
                    uint8_t act = 0u;
                    for (i = 0; i < RYTM_NDVOICES; i++) {
                        if (!g.cfg.dv[i].mute)
                            act++;
                    }
                    if (!g.cfg.bass_mute)
                        act++;
                    v = fm_seq_ens_bar_custom(act, 8u);
                } else
                    v = fm_seq_ens_bar_preset(det);
                mx = 127u;
            } else {
                v = g.seq_flip_state ? 127u : 0u;
                mx = 127u;
            }
        } else if (g.seq_page == 1u) {
            uint8_t fn = fm_seq_p1_fn((uint8_t)slot);

            if (fn == 3u) {
                int32_t bt = c->seq_ovr.lane_tune[ln];
                v = (uint32_t)(bt < 0 ? -bt : bt);
                mx = 24u;
            } else if (fn == 1u) {
                v = fm_seq_trig_pct(c->seq_ovr.lane_trig[ln]);
                mx = 100u;
            } else {
                int32_t bv = fn == 0u ? c->seq_ovr.lane_vel[ln] : c->seq_ovr.lane_gate[ln];
                v = (uint32_t)(bv < 0 ? -bv : bv);
                mx = 50u;
            }
        } else {
            if (slot == 0) {
                v = ln;
                mx = 7u;
            } else if (slot == 1) {
                v = fm_seq_euclid_bar_hits(c->euclid_hits[ln]);
                mx = 16u;
            } else if (slot == 2) {
                v = (uint32_t)c->euclid_rot[ln];
                mx = 15u;
            } else {
                v = c->seq_ovr.vary_lane[ln];
                mx = 127u;
            }
        }
    } else if (g.bank == BANK_HOME) {
        uint8_t pg = bank_page() % 4u;
        uint8_t ix = (uint8_t)(pg * 4u + slot);
        if (fm1_in.buttons & (1u << BTN_LFO))
            v = c->lfo.depth[LFO_TGT_MACRO0 + ix];
        else
            v = g.home_macro[ix];
    } else if (g.bank == BANK_GROOVE) {
        if (bank_page() == 0u) {
            if (slot == 3) {
                v = c->bpm > 60u ? c->bpm - 60u : 0u;
                mx = 140u;
            } else
                v = slot == 0 ? c->density : slot == 1 ? c->feel : c->flam;
        } else if (slot == 0) {
            v = c->swing_max;
            mx = 100u;
        } else if (slot == 1)
            v = c->human;
        else if (slot == 2) {
            v = c->drummer_auto ? 0u : (c->drummer + 1u);
            mx = 8u;
        } else {
            v = c->bassist_auto ? 0u : (c->bassist + 1u);
            mx = 8u;
        }
    } else if (g.bank == BANK_DRUM) {
        rytm_dvoice_cfg_t *dv = &c->dv[c->sel_voice % RYTM_NDVOICES];
        if (bank_page() == 0u)
            v = slot == 0 ? dv->tune : slot == 1 ? dv->decay : slot == 2 ? dv->tone : dv->drive;
        else if (slot == 0) {
            if (!voice_audible(c->sel_voice % RYTM_NDVOICES))
                v = 0u;
            else
                v = dv->level;
        }
        else if (slot == 1)
            v = dv->send_dly;
        else if (slot == 2)
            v = dv->send_rev;
        else
            v = rytm_pan_eff(dv->pan, c->sel_voice);
    } else if (g.bank == BANK_BASS) {
        if (bank_page() == 0u) {
            v = slot == 0 ? c->bass.model : slot == 1 ? c->bass.cutoff : slot == 2 ? c->bass.reso : c->bass.decay;
            mx = slot == 0 ? (BMODEL_COUNT - 1u) : 127u;
        } else if (bank_page() == 2u) {
            v = slot == 0 ? c->root : 0u;
            mx = 11u;
        } else if (slot == 2) {
            v = c->scale;
            mx = RYTM_NSCALES - 1u;
        } else
            v = slot == 0 ? c->bass.env_mod : slot == 1 ? c->bass.glide : c->prog_depth;
    } else if (g.bank == BANK_FX) {
        if (bank_page() == 0u)
            v = slot == 0 ? c->fx.dly_mix : slot == 1 ? c->fx.rev_mix : slot == 2 ? c->fx.pump : c->fx.db_bal;
        else
            v = slot == 0 ? c->fx.dly_time : slot == 1 ? c->fx.dly_fb : slot == 2 ? c->fx_rev_size : c->fx.master_drive;
    } else if (g.bank == BANK_LFO || g.view == VIEW_LFO) {
        uint8_t t = c->lfo.target % LFO_NTARGETS;
        uint8_t pg = bank_page();
        if (pg == 0u) {
            if (slot == 0) {
                v = t;
                mx = LFO_NTARGETS - 1u;
            } else if (slot == 1) {
                v = c->lfo.rate[t];
                mx = 6u;
            } else if (slot == 2) {
                v = c->lfo.depth[t];
                mx = 127u;
            } else {
                v = c->lfo.wave[t];
                mx = 127u;
            }
        } else if (pg == 1u) {
            v = slot == 0 ? c->lfo.phase[t] : slot == 1 ? c->lfo.skew[t] :
                slot == 2 ? c->lfo.smooth[t] : c->lfo.fold[t];
            mx = 127u;
        } else {
            v = slot == 0 ? c->lfo.algo[t] : slot == 1 ? c->lfo.ratio[t] :
                slot == 2 ? c->lfo.index[t] : c->lfo.fb[t];
            mx = slot == 0 ? (FM_LFO_NALGOS - 1u) : 127u;
        }
    } else {
        v = slot == 0 ? c->root : slot == 1 ? c->scale : slot == 2 ? c->prog_depth : c->human;
        mx = slot == 0 ? 11u : slot == 1 ? 8u : 127u;
    }
    if (v > mx)
        v = mx;
    return mx ? (v * max_h) / mx : 0u;
}

static void draw_top_bar(void)
{
    char s[48];
    uint8_t glo, ghi, gf;
    uint8_t act_gest = g.gesture_latch ? g.gesture_latch : g.gesture;
    const char *mode = 0;
    uint16_t gpos = genre_pos4095();
    uint16_t gcol = spectrum_rgb565(gpos);
    uint32_t x;
    rytm_genre_pair(gpos, &glo, &ghi, &gf);

    cv_begin(240, 34, C_BLACK);
    cv_text(4, 1, &FONT_S, "KO RYTM", C_WHITE);
    if (ghi != glo && gf > 20u && gf < 236u) {
        int32_t nx = cv_text(80, 1, &FONT_S, GENRE_SHORT[glo], spectrum_rgb565((uint16_t)((uint32_t)glo * 4095u / (RYTM_NGENRES - 1u))));
        nx = cv_text(nx, 1, &FONT_S, ">", gcol);
        cv_text(nx, 1, &FONT_S, GENRE_SHORT[ghi], spectrum_rgb565((uint16_t)((uint32_t)ghi * 4095u / (RYTM_NGENRES - 1u))));
    } else {
        uint8_t near = gf >= 128u ? ghi : glo;
        cv_text(80, 1, &FONT_S, RYTM_GENRE_NAMES[near], gcol);
    }
    if (g.rec_arm)
        cv_rect(198, 4, 5, 5, RGB(255, 48, 48));
    if (g.perf == PERF_SEQ) {
        fm_seq_draw_page_dots(214, 4, g.seq_page, FM_SEQ_NPAGES);
    } else if (g.perf != PERF_ARP && g.view != VIEW_REC) {
        if (g.bank == BANK_HOME) {
            uint8_t hp = bank_page() % 4u;
            uint32_t pi;
            for (pi = 0; pi < 4u; pi++)
                cv_rect(208 + (int32_t)pi * 7, 4, 4, 4, pi == hp ? C_WHITE : C_DIM);
        } else if (g.bank == BANK_LFO || g.bank == BANK_BASS) {
            uint8_t lp = bank_page() % 3u;
            uint32_t pi;

            for (pi = 0; pi < 3u; pi++)
                cv_rect(214 + (int32_t)pi * 8, 4, 5, 5, pi == lp ? C_WHITE : C_DIM);
        } else if (g.bank == BANK_GROOVE || g.bank == BANK_DRUM || g.bank == BANK_FX) {
            int p2 = bank_page() != 0u;
            cv_rect(222, 4, 5, 5, p2 ? C_DIM : C_WHITE);
            cv_rect(230, 4, 5, 5, p2 ? C_WHITE : C_DIM);
        }
    }

    {
        static const char *const SC_TINY[RYTM_NSCALES] = {
            "m", "dor", "phr", "mix", "mp", "Mp", "blu", "hm", "whl"
        };
        int32_t nx;
        u8_dec(s, g.bar + 1u);
        copy_str(s + rytm_strlen(s), "/8 ");
        nx = cv_text(4, 17, &FONT_S, s, C_GRAY);
        copy_str(s, NOTE_NAMES[g.cfg.root % 12u]);
        copy_str(s + rytm_strlen(s), SC_TINY[g.cfg.scale % RYTM_NSCALES]);
        cv_text(nx, 17, &FONT_S, s, C_HI);
    }

    /* Open R->Y->G->C->B genre spectrum bar + active thumb (aligned with genre at x=80) */
    for (x = 0; x < 104u; x++) {
        uint16_t scol = spectrum_rgb565((uint16_t)((x * 4095u) / 103u));
        cv_line(80 + (int32_t)x, 23, 80 + (int32_t)x, 26, scol);
    }
    {
        int32_t tx = 80 + ((int32_t)g.eff_genre * 98) / 127;
        cv_rect(tx - 1, 19, 8, 12, C_WHITE);
        cv_rect(tx, 20, 6, 10, gcol);
    }

    if (fm1_ms < g.saved_until)
        mode = "SAVED";
    else if (g.save_req && g.playing)
        mode = "SAVE ON STOP";
    else if (act_gest == GEST_FILL)
        mode = "FILL";
    else if (act_gest == GEST_BREAK)
        mode = "BREAK";
    else if (act_gest == GEST_SOLO_DRUM)
        mode = "SOLO D";
    else if (act_gest == GEST_SOLO_BASS)
        mode = "SOLO B";
    else if (act_gest == GEST_SOLO_BAND)
        mode = "SOLO+";
    else if (act_gest == GEST_FLAM_BURST)
        mode = "FLAM!";
    else if (g.perf == PERF_SEQ)
        mode = "SEQ";
    else if (g.perf == PERF_ARP)
        mode = "ARP";
    if (mode) {
        int32_t mx = 236 - (int32_t)rytm_strlen(mode) * 8;
        cv_text(mx, 17, &FONT_S, mode, C_WHITE);
    } else {
        int32_t bx;
        uint16_t bcol = (g.playing && g.step == 0u) ? C_WHITE :
                        (g.playing && (g.step & 3u) == 0u) ? C_HI : C_GRAY;
        u8_dec(s, g.cfg.bpm);
        bx = 236 - (int32_t)rytm_strlen(s) * 8;
        cv_text(bx, 17, &FONT_S, s, bcol);
    }
    cv_line(0, 33, 239, 33, C_LINE);
    cv_blit(0, 0);
}

static void draw_view_band(void)
{
    static const char *const VSHORT[7] = {"K", "S", "C", "O", "L", "H", "P"};
    uint32_t v, st;
    char s[32];
    rytm_query_t q;
    rytm_step_events_t ev;

    copy_str(s, RYTM_DRUMMER_NAMES[g.cfg.drummer % RYTM_NDRUMMERS]);
    cv_text(6, 2, &FONT_S, s, PERSONA_COL[g.cfg.drummer % RYTM_NDRUMMERS]);
    copy_str(s, RYTM_BASSIST_NAMES[g.cfg.bassist % RYTM_NBASSISTS]);
    {
        int32_t nx = cv_text(116, 2, &FONT_S, s, PERSONA_COL[g.cfg.bassist % RYTM_NBASSISTS]);
        cv_text(nx + 6, 2, &FONT_S, BMODEL_SHORT[g.cfg.bass.model % BMODEL_COUNT], model_color(g.cfg.bass.model));
    }

    /* 7 Drum Voice live peak meters (left half) */
    for (v = 0; v < RYTM_NDVOICES; v++) {
        int32_t x = 6 + (int32_t)v * 15;
        int32_t h = ((int32_t)g.synth.voice_peak[v] * 42) / 255;
        uint16_t col = g.cfg.dv[v].mute ? C_LINE : (rytm_voice_is_solo(v) ? C_HI : C_DRUM);
        cv_rect(x, 22, 11, 42, BANK_BG[1]);
        if (h > 0)
            cv_rect(x, 64 - h, 11, h, col);
        cv_text(x + 2, 69, &FONT_S, VSHORT[v], g.cfg.dv[v].mute ? C_DIM : C_GRAY);
    }

    /* Bass live scope + filter (right half, clean without overlapping text) */
    cv_rect(116, 22, 118, 44, BANK_BG[2]);
    {
        int32_t bh = ((int32_t)g.synth.bass_peak * 38) / 255;
        int32_t cx = 142 + ((int32_t)g.eff_bcut * 86) / 127;
        int32_t rh = 4 + ((int32_t)g.eff_bres * 16) / 127;
        uint8_t mdl = g.cfg.bass.model % BMODEL_COUNT;
        uint16_t mcol = model_color(mdl);
        int32_t prev_y = 44;
        uint32_t i;

        if (bh > 0)
            cv_rect(120, 64 - bh, 14, bh, mcol);
        cv_line(140, 44, 230, 44, C_LINE);
        for (i = 0; i < 88u; i++) {
            uint32_t ph = (i * 65536u * 2u / 88u) << 16;
            int32_t wf = bass_wave_point(mdl, ph, i);
            int32_t env = 32767 - (int32_t)((i * 32767u) / (30u + g.cfg.bass.decay / 2u));
            int32_t y;
            if (env < 0) env = 0;
            wf = (wf * env) >> 15;
            y = 44 - ((wf * 16) >> 15);
            if (i > 0u)
                cv_line(140 + (int32_t)i - 1, prev_y, 140 + (int32_t)i, y, mcol);
            prev_y = y;
        }
        cv_line(cx, 25, cx, 63, C_DIM);
        cv_rect(cx - 2, 44 - rh, 5, rh, C_HI);
    }

    /* 16-step Rhythm Section Lock strip (Drums top, Bass bottom) */
    build_query(&q);
    for (st = 0; st < RYTM_PAGE_STEPS; st++) {
        int32_t x = 8 + (int32_t)st * 14;
        uint8_t d_on = 0, d_orn = 0;
        query_at(&q, seq_view_base() + st);
        rytm_resolve_step(&q, &g.cfg.seq_ovr, &ev);
        for (v = 0; v < RYTM_NDVOICES; v++) {
            if (ev.drums[v].vel && !g.cfg.dv[v].mute) {
                d_on = 1;
                if (ev.drums[v].orn_kind)
                    d_orn = 1;
            }
        }
        if (seq_view_base() + st == g.step && g.playing)
            cv_rect(x - 1, 87, 12, 22, genre_hue());
        cv_rect(x, 88, 10, 9, d_orn ? C_ACC : (d_on ? C_DRUM : C_LINE));
        cv_rect(x, 99, 10, 9, ev.bass.active ? (ev.bass.accent ? C_HI : C_BASS) : C_LINE);
    }

    /* 8-bar AA'B phrase indicator */
    for (v = 0; v < 8u; v++) {
        int32_t x = 8 + (int32_t)v * 28;
        uint16_t col = (v == g.bar) ? genre_hue() : ((v == 3u || v == 7u) ? C_ACC : C_DIM);
        cv_rect(x, 114, 24, 10, col);
    }
}

static void draw_view_home(void)
{
    static const char *const LN[8] = {"K", "S", "C", "O", "L", "H", "P", "B"};
    uint32_t v, st;
    rytm_query_t q;
    rytm_step_events_t ev;
    uint8_t act_gest = g.gesture_latch ? g.gesture_latch : g.gesture;
    uint8_t gi = (uint8_t)((genre_pos4095() * (RYTM_NGENRES - 1u)) / 4095u);
    uint16_t gh = genre_hue();
    char s[24];

    for (v = 0; v < 8u; v++) {
        int32_t x = 4 + (int32_t)v * 29;
        int32_t pk = v < RYTM_NDVOICES ? g.synth.voice_peak[v] : g.synth.bass_peak;
        int32_t h = (pk * 32) / 255;
        uint16_t col = C_DRUM;

        if (v == 7u)
            col = C_BASS;
        if (v < RYTM_NDVOICES && g.cfg.dv[v].mute)
            col = C_DIM;
        if (v == 7u && g.cfg.bass_mute)
            col = C_DIM;
        if (rytm_voice_is_solo((uint8_t)v))
            col = C_HI;
        cv_rect(x, 2, 24, 34, BANK_BG[5]);
        if (h > 0)
            cv_rect(x + 2, 34 - h, 20, h, col);
        cv_text(x + 8, 37, &FONT_S, LN[v], col);
    }

    {
        static const char *const GT[4] = {"F/B", "D.S", "B.S", "FLM"};
        uint32_t i;
        for (i = 0; i < 4u; i++) {
            int32_t x = 8 + (int32_t)i * 56;
            int lit = 0;
            if (i == 0u)
                lit = act_gest == GEST_FILL || act_gest == GEST_BREAK;
            else if (i == 1u)
                lit = act_gest == GEST_SOLO_DRUM || act_gest == GEST_SOLO_BAND;
            else if (i == 2u)
                lit = act_gest == GEST_SOLO_BASS || act_gest == GEST_SOLO_BAND;
            else
                lit = act_gest == GEST_FLAM_BURST;
            cv_rect(x, 54, 48, 20, lit ? gh : C_LINE);
            cv_text(x + 12, 56, &FONT_S, GT[i], lit ? C_BLACK : C_GRAY);
        }
    }

    build_query(&q);
    for (st = 0; st < RYTM_PAGE_STEPS; st++) {
        int32_t x = 6 + (int32_t)st * 14;
        uint32_t e = 0;
        query_at(&q, seq_view_base() + st);
        rytm_resolve_step(&q, &g.cfg.seq_ovr, &ev);
        for (v = 0; v < RYTM_NDVOICES; v++)
            if (ev.drums[v].vel)
                e += ev.drums[v].vel;
        if (ev.bass.active)
            e += ev.bass.vel;
        e = e > 255u ? 255u : e;
        {
            int32_t h = (int32_t)((e * 20u) / 255u);
            cv_rect(x, 80, 10, 20, C_LINE);
            if (h > 0)
                cv_rect(x, 100 - h, 10, h, gh);
        }
        if (seq_view_base() + st == g.step && g.playing)
            cv_rect(x - 1, 79, 12, 22, C_WHITE);
    }

    for (v = 0; v < 8u; v++) {
        int32_t x = 6 + (int32_t)v * 28;
        int32_t deg = ((int32_t)RYTM_GENRE_PROG_8[gi][v] * (int32_t)g.eff_prog) / 127;
        uint8_t c_pc = (uint8_t)(rytm_scale_note(g.cfg.root, g.cfg.scale, deg, 0) % 12u);
        uint16_t col = (v == g.bar) ? gh : C_LINE;
        int32_t tx;

        copy_str(s, NOTE_NAMES[c_pc]);
        tx = x + (24 - (int32_t)rytm_strlen(s) * 8) / 2;
        cv_rect(x, 106, 24, 20, col);
        cv_text(tx, 108, &FONT_S, s, (v == g.bar) ? C_BLACK : C_GRAY);
    }
}

static void draw_view_drum(void)
{
    uint32_t v, st;
    uint8_t sv = g.cfg.sel_voice % RYTM_NDVOICES;
    char s[48];
    rytm_query_t q;
    rytm_step_events_t ev;
    int32_t w = 240;

    refresh_kit();
    copy_str(s, RYTM_DVOICE_NAMES[sv]);
    {
        int32_t nx = cv_text(6, 0, &FONT_L, s, C_WHITE);
        copy_str(s, RYTM_DRUMMER_NAMES[g.cfg.drummer % RYTM_NDRUMMERS]);
        copy_str(s + rytm_strlen(s), " KIT");
        cv_text(nx + 8, 2, &FONT_S, s, PERSONA_COL[g.cfg.drummer % RYTM_NDRUMMERS]);
        if (g.cfg.dv[sv].mute)
            cv_text(nx + 8, 15, &FONT_S, "MUTED", C_ACC);
        else {
            copy_str(s, "L");
            u8_show100_dec(s + 1, g_kit[sv].level);
            copy_str(s + rytm_strlen(s), " S");
            u8_show100_dec(s + rytm_strlen(s), g_kit[sv].send_dly);
            cv_text(nx + 8, 15, &FONT_S, s, C_GRAY);
        }
    }
    if (g.eff_fx.harmonic_toms) {
        uint8_t note = (uint8_t)(36u + (g.cfg.root % 12u) + (sv == DV_HTOM ? 7u : 0u));
        cv_text(210, 2, &FONT_S, sv == DV_LTOM || sv == DV_HTOM ? NOTE_NAMES[note % 12u] : "HT", C_HI);
    } else
        cv_text(210, 2, &FONT_S, "HT", C_DIM);

    cv_rect(0, 32, 240, 50, BANK_BG[1]);
    {
        int32_t ym = 57;
        int32_t y_top = 34;
        int32_t y_bot = 80;
        int32_t max_amp = ((int32_t)g_kit[sv].level * 21) / 127;
        int32_t dec_w = 18 + ((int32_t)g_kit[sv].decay * (w - 38)) / 127;
        int32_t tune = (int32_t)g_kit[sv].tune;
        int32_t tone = (int32_t)g_kit[sv].tone;
        int32_t drv = (int32_t)g_kit[sv].drive;
        int32_t send = (int32_t)g_kit[sv].send_dly;
        int32_t rev = (int32_t)g_kit[sv].send_rev;
        int32_t echo_step = 22 + ((int32_t)g.eff_fx.dly_time * 28) / 127;
        int32_t echo_env = 0;
        int32_t prev_top = ym, prev_bot = ym, prev_wy = ym;
        uint16_t wave_col = g.cfg.dv[sv].mute ? C_DIM :
                            PERSONA_COL[g.cfg.drummer % RYTM_NDRUMMERS];
        uint32_t ph = 0;
        uint32_t i;

        cv_line(0, ym, 239, ym, C_LINE);
        for (i = 0; i < 240u; i++) {
            int32_t x = (int32_t)i;
            int32_t env = 0;
            int32_t main_env;
            int32_t top, bot, wave, wy, crest;
            uint16_t col = wave_col;
            uint32_t inc = (uint32_t)(900 + tune * 28);

            if ((int32_t)i < 20)
                inc += (uint32_t)((20 - (int32_t)i) * (40 + tone * 3));
            ph += inc << 16;

            if ((int32_t)i < dec_w) {
                int32_t rem = dec_w - (int32_t)i;
                env = (max_amp * rem * (rem + dec_w)) / (2 * dec_w * dec_w);
            }
            if ((int32_t)i < 6 && max_amp > 0)
                env += (tone * (6 - (int32_t)i)) / 64;
            if (rev > 8 && (int32_t)i > dec_w / 2) {
                int32_t tail = (max_amp * rev * (int32_t)(240 - (int32_t)i)) / (127 * 240 * 2);
                if (tail > env)
                    env = tail;
            }
            if (env > 22)
                env = 22;
            main_env = env;

            echo_env -= (echo_env * 52) / 127;
            if (echo_env < 0)
                echo_env = 0;
            if (send > 4 && (int32_t)i >= echo_step && ((int32_t)i % echo_step) == 0) {
                int32_t tap_no = (int32_t)i / echo_step;
                int32_t tap_peak = (max_amp * send) / (127 * (1 + tap_no));
                if (tap_peak > echo_env)
                    echo_env = tap_peak;
            }
            if (echo_env > main_env && !g.cfg.dv[sv].mute)
                col = C_HI;

            wave = rytm_sine(ph);
            if (tone > 16 && (int32_t)i < 48) {
                int32_t fm = rytm_sine(ph * 3u + (uint32_t)(i << 26));
                wave = wave + ((fm * tone * (48 - (int32_t)i)) >> 12);
            }
            wave = rytm_fold(wave, (uint32_t)drv);
            wy = ym - ((wave * env) / 32767);
            if (wy < y_top) wy = y_top;
            if (wy > y_bot) wy = y_bot;
            top = ym - env;
            bot = ym + env;

            if (rev > 6 && (int32_t)i > dec_w / 2) {
                int32_t r_env = 6 + (env * rev) / 100;
                int32_t r_wv = rytm_sine(ph - 0x28000000u) +
                               (rytm_sine((ph << 1) + 0x18000000u) >> 1);
                int32_t ry = ym - ((r_wv * r_env) / 32767);
                if (ry < y_top) ry = y_top;
                if (ry > y_bot) ry = y_bot;
                if (((i + (uint32_t)sv) & 1u) == 0u)
                    cv_line(x, ym, x, ry, rev > 72 ? C_DIM : C_LINE);
                cv_pset(x, ry, rev > 84 ? C_HI : C_DIM);
            }

            if (env > 8 && (i & 1u) == 0u)
                cv_line(x, ym, x, wy, echo_env > main_env ? C_LINE : C_DIM);

            crest = ym - wy;
            if (crest < 0)
                crest = -crest;
            if (!g.cfg.dv[sv].mute && (drv > 20 || tone > 40) && crest > env / 2)
                col = C_ACC;
            if (!g.cfg.dv[sv].mute && drv > 64 && crest > (env * 3) / 4)
                cv_pset(x, wy > ym ? wy - 1 : wy + 1, C_WHITE);

            if (i > 0u) {
                cv_line(x - 1, prev_wy, x, wy, col);
                cv_line(x - 1, prev_top, x, top, C_HI);
                cv_line(x - 1, prev_bot, x, bot, C_DRUM);
            }
            prev_wy = wy;
            prev_top = top;
            prev_bot = bot;
        }
    }

    build_query(&q);
    for (st = 0; st < RYTM_PAGE_STEPS; st++) {
        int32_t x = 8 + (int32_t)st * 14;
        uint16_t col = C_LINE;
        query_at(&q, seq_view_base() + st);
        rytm_resolve_step(&q, &g.cfg.seq_ovr, &ev);
        if (ev.drums[sv].vel) {
            if (fm_seq_bit(g.cfg.seq_ovr.drum_force_on[sv], seq_view_base() + st))
                col = C_WHITE;
            else if (fm_seq_bit(g.cfg.seq_ovr.drum_force_off[sv], seq_view_base() + st) || g.cfg.seq_ovr.vary_lane[sv] > 0u)
                col = C_ACC;
            else
                col = ev.drums[sv].orn_kind ? C_ACC : C_DRUM;
        } else if (fm_seq_bit(g.cfg.seq_ovr.drum_force_on[sv], seq_view_base() + st))
            col = C_DIM;
        else if (fm_seq_bit(g.cfg.seq_ovr.drum_force_off[sv], seq_view_base() + st))
            col = C_BLACK;
        if (seq_view_base() + st == g.step && g.playing)
            cv_rect(x - 1, 87, 12, 10, genre_hue());
        cv_rect(x, 88, 10, 8, col);
    }

    for (v = 0; v < RYTM_NDVOICES; v++) {
        int32_t x = 8 + (int32_t)v * 32;
        int32_t lh = ((int32_t)g_kit[v].level * 36) / 127;
        int32_t pan = (int32_t)rytm_pan_eff(g_kit[v].pan, (uint8_t)v);
        int32_t lh_l = (lh * (127 - pan)) / 127;
        int32_t lh_r = (lh * pan) / 127;
        int32_t ph = ((int32_t)g.synth.voice_peak[v] * 28) / 255;
        uint16_t col = g.cfg.dv[v].mute ? C_LINE : (v == sv ? C_ACC : C_DRUM);
        if (lh_l > 28)
            lh_l = 28;
        if (lh_r > 28)
            lh_r = 28;
        cv_rect(x, 102, 22, 28, BANK_BG[1]);
        if (lh_l > 0)
            cv_rect(x, 130 - lh_l, 7, lh_l, col);
        if (lh_r > 0)
            cv_rect(x + 7, 130 - lh_r, 7, lh_r, col);
        if (ph > 0)
            cv_rect(x + 16, 130 - ph, 6, ph, C_WHITE);
    }
}

static void draw_view_bass(void)
{
    char s[32];
    uint32_t i, st;
    uint8_t gi = (uint8_t)((genre_pos4095() * (RYTM_NGENRES - 1u)) / 4095u);
    uint8_t mdl = g.cfg.bass.model % BMODEL_COUNT;
    rytm_query_t q;
    rytm_step_events_t ev;
    int32_t w = 240, y0 = 18, h = 38, ym = 37;
    int32_t reso = (int32_t)g.eff_bass.reso;

    copy_str(s, RYTM_BASSIST_NAMES[g.cfg.bassist % RYTM_NBASSISTS]);
    cv_text(6, 2, &FONT_S, s, PERSONA_COL[g.cfg.bassist % RYTM_NBASSISTS]);
    copy_str(s, RYTM_BMODEL_NAMES[mdl]);
    cv_text(120, 2, &FONT_S, s, model_color(mdl));

    cv_rect(0, y0, 240, h, BANK_BG[2]);
    {
        int32_t fc = 16 + ((int32_t)g.eff_bass.cutoff * (w - 40)) / 127;
        int32_t res_h = (reso * 14) / 127;
        int32_t env_w = ((int32_t)g.eff_bass.env_mod * 48) / 127;
        int32_t prev_y = ym;
        int32_t lp = 0;
        uint32_t dec = g.eff_bass.decay;
        uint32_t glide_w = 2u + ((uint32_t)g.eff_bass.glide * 80u) / 127u;
        uint32_t ph = 0;

        cv_line(0, ym, 239, ym, C_LINE);
        cv_line(8, ym - 1, fc - 6, ym - 1, C_DIM);
        cv_line(fc - 6, ym - 1, fc, ym - 1 - res_h, C_HI);
        cv_line(fc, ym - 1 - res_h, fc + 14, y0 + h - 2, C_HI);
        if (env_w > 2) {
            int32_t fce = fc + env_w > w - 8 ? w - 8 : fc + env_w;
            cv_line(fc, ym - 1 - res_h, fce, ym - 1 - res_h, C_ACC);
        }

        for (i = 0; i < 240u; i++) {
            int32_t x = (int32_t)i;
            uint32_t step = (65536u * 3u) / 240u;
            int32_t wf, env, alpha, y, crest;
            uint16_t col = model_color(mdl);

            if (i < glide_w)
                step = (step * (2u + i)) / (2u + glide_w);
            ph += step << 16;
            wf = bass_wave_point(mdl, ph, i);
            env = 32767 - (int32_t)((i * 32767u) / (28u + dec * 2u));
            if (env < 0)
                env = 0;
            alpha = 32 + ((int32_t)g.eff_bass.cutoff * 190) / 127 +
                    (((int32_t)g.eff_bass.env_mod * env) >> 15);
            if (alpha > 256)
                alpha = 256;
            lp = lp + (((wf - lp) * alpha) >> 8);
            wf = (lp * env) >> 15;
            y = ym - ((wf * 16) >> 15);
            if (y < y0 + 2) y = y0 + 2;
            if (y > y0 + h - 2) y = y0 + h - 2;

            crest = ym - y;
            if (crest < 0)
                crest = -crest;
            if (env > 12000 && (i & 1u) == 0u)
                cv_line(x, ym, x, y, C_DIM);
            if (reso > 28 && crest >= 4) {
                col = C_ACC;
                if (reso > 72)
                    cv_pset(x, y < ym ? y - 1 : y + 1, C_WHITE);
            }

            if (i > 0u)
                cv_line(x - 1, prev_y, x, y, col);
            prev_y = y;
        }
    }

    build_query(&q);
    for (st = 0; st < RYTM_PAGE_STEPS; st++) {
        int32_t x = 8 + (int32_t)st * 14;
        uint32_t ab = seq_view_base() + st;
        uint16_t col = C_BASS;
        int32_t semi;
        int32_t ny;

        query_at(&q, ab);
        rytm_resolve_step(&q, &g.cfg.seq_ovr, &ev);
        cv_rect(x, 58, 10, 14, C_LINE);
        if (fm_seq_bit(g.cfg.seq_ovr.bass_force_off, ab))
            col = C_BLACK;
        else if (fm_seq_bit(g.cfg.seq_ovr.bass_force_on, ab))
            col = C_WHITE;
        else if (ev.bass.active)
            col = ev.bass.accent ? C_HI : C_BASS;
        if (ab == g.step && g.playing)
            cv_rect(x - 1, 57, 12, 16, genre_hue());
        if (!ev.bass.active || fm_seq_bit(g.cfg.seq_ovr.bass_force_off, ab))
            continue;
        semi = (int32_t)ev.bass.midi_note - (int32_t)(36u + (g.cfg.root % 12u));
        if (semi < 0)
            semi = 0;
        if (semi > 24)
            semi = 24;
        ny = 68 - (semi * 10) / 24;
        cv_rect(x, ny, 10, 4, col);
        if (ev.bass.legato && st + 1u < RYTM_PAGE_STEPS)
            cv_rect(x + 10, ny + 1, 4, 2, C_ACC);
    }

    for (i = 0; i < 8u; i++) {
        int32_t x = 8 + (int32_t)i * 28;
        int32_t deg = ((int32_t)RYTM_GENRE_PROG_8[gi][i] * (int32_t)g.eff_prog) / 127;
        uint8_t c_pc = (uint8_t)(rytm_scale_note(g.cfg.root, g.cfg.scale, deg, 0) % 12u);
        int32_t tx;

        copy_str(s, NOTE_NAMES[c_pc]);
        tx = x + (24 - (int32_t)rytm_strlen(s) * 8) / 2;
        cv_rect(x, 76, 24, 24, (i == g.bar) ? BANK_BG[4] : C_LINE);
        if (i == g.bar)
            cv_rect(x, 76, 24, 3, genre_hue());
        cv_text(tx, 81, &FONT_S, s, (i == g.bar) ? C_WHITE : C_HI);
    }
}

static void draw_view_seq(void)
{
    static const char *const LBL[8] = {"K", "S", "C", "O", "L", "H", "P", "B"};
    uint16_t lcols[8];
    fm_seq_cell_t cells[8u * FM_SEQ_PAGE_STEPS];
    uint32_t lane, st;
    rytm_query_t q;
    rytm_step_events_t ev[RYTM_PAGE_STEPS];
    rytm_step_events_t base_ev[RYTM_PAGE_STEPS];
    static rytm_seq_ovr_t base_ovr;
    uint32_t base = seq_view_base();
    uint8_t plock_st = g.seq_plock_hold && g.seq_plock_step >= base && g.seq_plock_step < base + RYTM_PAGE_STEPS ?
                       (uint8_t)(g.seq_plock_step - base) : FM_SEQ_PLOCK_NONE;
    uint8_t play_st = g.step >= base && g.step < base + RYTM_PAGE_STEPS ? (uint8_t)(g.step - base) : 0xFFu;
    uint32_t li;

    base_ovr = g.cfg.seq_ovr;
    for (li = 0; li < 8u; li++)
        base_ovr.lane_trig[li] = 0;
    build_query(&q);
    for (st = 0; st < RYTM_PAGE_STEPS; st++) {
        query_at(&q, base + st);
        rytm_resolve_step(&q, &g.eff_seq_ovr, &ev[st]);
        rytm_resolve_step(&q, &base_ovr, &base_ev[st]);
    }
    for (lane = 0; lane < 8u; lane++) {
        uint16_t lcol = C_GRAY;
        int lane_aud = voice_audible(lane < RYTM_NDVOICES ? (uint8_t)lane : 7u) ? 1 : 0;
        uint16_t play_col = lane >= RYTM_NDVOICES ? C_BASS : C_DRUM;

        if (rytm_voice_is_solo(lane))
            lcol = C_ACC;
        else if (!lane_aud)
            lcol = C_DIM;
        else if (lane == g.seq_lane)
            lcol = C_WHITE;
        else
            lcol = play_col;
        {
            uint8_t tm = rytm_target_mute_mask();
            if (g.playing && ((tm ^ g.heard_mute_mask) >> lane) & 1u)
                lcol = ((fm1_ms / 80u) & 1u) ? C_WHITE : C_ACC;
        }
        lcols[lane] = lcol;
        for (st = 0; st < RYTM_PAGE_STEPS; st++) {
            uint32_t ab = base + st;
            uint16_t col = 0;
            uint16_t bg_col = (lane == g.seq_lane) ? BANK_BG[0] : C_LINE;
            const uint16_t mute_col = RGB(22, 26, 44);
            const uint16_t mute_bg = RGB(8, 10, 18);
            int base_on = lane < RYTM_NDVOICES ? (base_ev[st].drums[lane].vel > 0u) :
                          base_ev[st].bass.active;
            int on = base_on &&
                     fm_seq_trig_pass(g.eff_seq_ovr.lane_trig[lane], (uint8_t)ab, g.bar, (uint8_t)lane);
            fm_seq_cell_t *cell = &cells[lane * FM_SEQ_PAGE_STEPS + st];
            int32_t tune = 0;
            int32_t micro = 0;
            uint8_t vel = 0;
            uint8_t cond = g.cfg.seq_ovr.step_prob[lane][ab];

            cell->col = 0;
            cell->vel = 0;
            cell->tune = 0;
            cell->micro = 0;
            cell->note_span = 0u;
            cell->note_tail = 0u;
            cell->cond = cond;
            cell->gate = fm_seq_rand_gate(g.eff_seq_ovr.lane_gate[lane], (uint8_t)ab, g.bar, (uint8_t)lane,
                                          g.rng);
            if (lane < RYTM_NDVOICES) {
                if (on) {
                    if (fm_seq_bit(g.eff_seq_ovr.drum_force_on[lane], ab)) {
                        if (g.eff_seq_ovr.vary_lane[lane] > 0u &&
                            (ev[st].drums[lane].vel < 60u || ev[st].drums[lane].orn_kind))
                            col = C_ACC;
                        else
                            col = C_WHITE;
                    } else if (fm_seq_bit(g.eff_seq_ovr.drum_force_off[lane], ab) || g.eff_seq_ovr.vary_lane[lane] > 0u)
                        col = C_ACC;
                    else
                        col = ev[st].drums[lane].orn_kind ? C_ACC :
                              (ev[st].drums[lane].accent ? C_HI : C_DRUM);
                    vel = ev[st].drums[lane].vel;
                    micro = ev[st].drums[lane].micro_ms;
                } else if (base_on) {
                    col = C_DIM;
                    vel = 40u;
                } else if (fm_seq_bit(g.eff_seq_ovr.drum_force_on[lane], ab))
                    col = C_DIM;
                else if (fm_seq_bit(g.eff_seq_ovr.drum_force_off[lane], ab))
                    col = 0;
                tune = (int32_t)g.cfg.seq_ovr.step_tune[lane][ab] +
                       (int32_t)fm_seq_rand_pitch(g.eff_seq_ovr.lane_tune[lane], (uint8_t)ab, g.bar,
                                                  (uint8_t)lane, g.rng);
                if (g.cfg.seq_ovr.step_micro[lane][ab])
                    micro = g.cfg.seq_ovr.step_micro[lane][ab];
            } else {
                if (on) {
                    if (fm_seq_bit(g.eff_seq_ovr.bass_force_on, ab)) {
                        if (g.eff_seq_ovr.vary_lane[7] > 0u && ev[st].bass.vel < 85u)
                            col = C_ACC;
                        else
                            col = C_WHITE;
                    } else if (fm_seq_bit(g.eff_seq_ovr.bass_force_off, ab) || g.eff_seq_ovr.vary_lane[7] > 0u)
                        col = C_ACC;
                    else
                        col = ev[st].bass.accent ? C_HI : C_BASS;
                    vel = ev[st].bass.vel;
                    micro = ev[st].bass.micro_ms;
                } else if (base_on) {
                    col = C_DIM;
                    vel = 40u;
                } else if (fm_seq_bit(g.eff_seq_ovr.bass_force_on, ab))
                    col = C_DIM;
                else if (fm_seq_bit(g.eff_seq_ovr.bass_force_off, ab))
                    col = 0;
                {
                    int32_t base_n = 36 + (int32_t)(g.cfg.root % 12u);
                    int32_t cur_n = g.cfg.seq_ovr.bass_note[ab] ? g.cfg.seq_ovr.bass_note[ab] :
                                    (base_ev[st].bass.active ? base_ev[st].bass.midi_note : base_n);
                    tune = (cur_n - base_n) +
                           (int32_t)fm_seq_rand_pitch(g.eff_seq_ovr.lane_tune[7], (uint8_t)ab, g.bar, 7u,
                                                      g.rng);
                }
                if (g.cfg.seq_ovr.step_micro[7][ab])
                    micro = g.cfg.seq_ovr.step_micro[7][ab];
            }
            if (tune < -24)
                tune = -24;
            if (tune > 24)
                tune = 24;
            if (micro < -25)
                micro = -25;
            if (micro > 25)
                micro = 25;
            if (!lane_aud && lane != g.seq_lane && (on || base_on)) {
                bg_col = mute_bg;
                col = mute_col;
            }
            cell->col = col;
            cell->vel = vel;
            cell->tune = (int8_t)tune;
            cell->micro = (int8_t)micro;
            cell->bg_col = bg_col;
        }
    }
    fm_seq_draw_grid(8u, g.seq_lane, play_st, g.playing, plock_st, 0, g.seq_lane, genre_hue(), LBL, lcols,
                     cells, 0, 0, 0);
    fm_seq_draw_step_dots(129, (uint8_t)(base / RYTM_PAGE_STEPS), (uint8_t)(g.step / RYTM_PAGE_STEPS), seq_pages(),
                          g.playing, genre_hue());
}

static void draw_knob_bar(void)
{
    uint32_t col;
    char s[16];
    uint8_t b = g.bank % BANK_COUNT;
    cv_begin(240, 74, C_BLACK);
    cv_line(0, 0, 239, 0, C_LINE);
    for (col = 0; col < 4u; col++) {
        int32_t x = (int32_t)col * 60;
        uint32_t bar = knob_bar_px((uint8_t)col, 30u);
        knob_label((uint8_t)col, s);
        cv_text(x + 4, 2, &FONT_S, s, BANK_COL[b]);
        if ((g.view == VIEW_LFO || g.bank == BANK_LFO) && g.perf != PERF_ARP && bank_page() == 0u &&
            col == 3u) {
            uint8_t lt = g.cfg.lfo.target % LFO_NTARGETS;
            fm_lfo_shape_draw(x, 18, g.cfg.lfo.wave[lt], C_WHITE, C_DIM);
        } else {
            knob_value((uint8_t)col, s);
            cv_text(x + 4, 18, &FONT_S, s, C_WHITE);
        }
        cv_rect(x + 6, 38, 48, 30, BANK_BG[b]);
        if (bar > 0u) {
            uint16_t bar_col = genre_hue();
            if (g.perf == PERF_LIVE && g.view != VIEW_REC) {
                if (g.bank == BANK_BASS && bank_page() == 0u && col == 0u)
                    bar_col = model_color(g.cfg.bass.model);
                else if (g.bank == BANK_GROOVE && bank_page() == 1u && col == 2u)
                    bar_col = PERSONA_COL[g.cfg.drummer % RYTM_NDRUMMERS];
                else if (g.bank == BANK_GROOVE && bank_page() == 1u && col == 3u)
                    bar_col = PERSONA_COL[g.cfg.bassist % RYTM_NBASSISTS];
            }
            cv_rect(x + 6, 68 - (int32_t)bar, 48, (int32_t)bar, bar_col);
        }
        if (g.bank == BANK_HOME) {
            uint8_t ix = (uint8_t)((bank_page() % 4u) * 4u + col);
            if (g.cfg.macro_motion[ix][g.step & 15u] != 255u || g.cfg.lfo.depth[LFO_TGT_MACRO0 + ix] > 0u)
                cv_rect(x + 50, 22, 5, 5, genre_hue());
        } else if (g.bank == BANK_DRUM) {
            uint8_t sv = g.cfg.sel_voice % RYTM_NDVOICES;
            uint8_t dp = (uint8_t)((bank_page() & 1u) * 4u + col);
            if (g.cfg.drum_motion[sv][dp][g.step & 15u] != 255u ||
                (g.cfg.lfo.depth[LFO_TGT_DRUM_TUNE + dp] > 0u && (g.cfg.lfo.drum_v[dp] % RYTM_NDVOICES) == sv))
                cv_rect(x + 50, 22, 5, 5, genre_hue());
        } else {
            uint8_t ml = motion_lane_for_knob((uint8_t)col);
            if (ml != 255u && g.cfg.motion_val[ml][g.step & 15u] != 255u)
                cv_rect(x + 50, 22, 5, 5, genre_hue());
        }
    }
    cv_blit(0, 166);
}

static void draw_view_rec(void)
{
    static const char *const RS[3] = {"OVERDUB", "REPLACE", "MONITOR"};
    uint8_t act[REC_TOTAL_SLOTS];
    uint32_t n = rec_build_active_list(act);
    uint32_t row, st, top = 0;
    const char *state = "STOPPED";
    uint16_t scol = C_DIM;
    char lbl[16];

    if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR) {
        state = RS[g.cfg.rec_mode % 3u];
        scol = C_HI;
    } else if (g.rec_arm && g.playing) {
        state = "MONITOR";
        scol = C_GRAY;
    } else if (g.rec_arm) {
        state = "ARMED";
        scol = C_ACC;
    } else if (g.playing) {
        state = "PLAYING";
        scol = C_WHITE;
    }
    cv_text(4, 2, &FONT_S, state, scol);
    cv_text(80, 2, &FONT_S, g.cfg.rec_bars == 0u ? "1 BAR" : g.cfg.rec_bars == 1u ? "2 BARS" :
            g.cfg.rec_bars == 2u ? "4 BARS" : "8 BARS", C_GRAY);
    if (n == 0u) {
        g.rec_lane = 0u;
        cv_text(76, 62, &FONT_S, "NO MOTION", C_DIM);
        return;
    }
    if (g.rec_lane >= n)
        g.rec_lane = (uint8_t)(n - 1u);
    if (g.rec_lane >= 8u)
        top = (uint32_t)g.rec_lane - 7u;
    {
        u8_dec(lbl, (uint32_t)g.rec_lane + 1u);
        copy_str(lbl + rytm_strlen(lbl), "/");
        u8_dec(lbl + rytm_strlen(lbl), n);
        cv_text(184, 2, &FONT_S, lbl, C_HI);
    }
    if (g.playing)
        cv_rect(57 + (int32_t)(g.step & 15u) * 11, 14, 11, 114, genre_hue());
    for (row = 0; row < 8u && (top + row) < n; row++) {
        uint32_t idx = top + row;
        uint8_t id = act[idx];
        const uint8_t *arr = rec_slot_arr(id);
        int32_t yb = 26 + (int32_t)row * 14;
        int sel = (idx == g.rec_lane);

        rec_slot_label(id, lbl);
        cv_text(4, yb - 10, &FONT_S, lbl, sel ? C_WHITE : C_GRAY);
        for (st = 0; st < RYTM_MOTION_STEPS; st++) {
            int32_t x = 58 + (int32_t)st * 11;
            uint8_t mv = motion_step_hold(arr, (uint8_t)st, 255u);
            int32_t h = mv == 255u ? 1 : 2 + ((int32_t)mv * 10) / 127;
            uint16_t col = mv == 255u ? (sel ? C_DIM : C_LINE) : (sel ? C_WHITE : C_ACC);
            cv_rect(x, yb - h, 9, h, col);
        }
    }
}

static void draw_screen(void)
{
    palette_set(2u);
    draw_top_bar();
    cv_begin(240, 132, C_BLACK);
    if (g.view == VIEW_HOME)
        draw_view_home();
    else if (g.view == VIEW_BAND)
        draw_view_band();
    else if (g.view == VIEW_DRUM)
        draw_view_drum();
    else if (g.view == VIEW_BASS)
        draw_view_bass();
    else if (g.view == VIEW_LFO)
        draw_view_lfo();
    else if (g.view == VIEW_REC)
        draw_view_rec();
    else
        draw_view_seq();
    cv_blit(0, 34);
    draw_knob_bar();
}

static uint8_t gesture_from_keys(uint32_t keys, uint32_t edit)
{
    uint32_t fill = 1u << KEY_BLACK[7];
    uint32_t dsolo = 1u << KEY_BLACK[8];
    uint32_t bsolo = 1u << KEY_BLACK[9];
    uint32_t flam = 1u << KEY_BLACK[10];
    uint8_t g0;

    if ((keys & dsolo) && (keys & bsolo))
        g0 = GEST_SOLO_BAND;
    else if (keys & dsolo)
        g0 = GEST_SOLO_DRUM;
    else if (keys & bsolo)
        g0 = GEST_SOLO_BASS;
    else if (keys & fill) {
        g0 = g.cfg.density >= 100u ? GEST_BREAK : GEST_FILL;
        if (edit)
            g0 = g0 == GEST_FILL ? GEST_BREAK : GEST_FILL;
    } else if (keys & flam)
        g0 = GEST_FLAM_BURST;
    else
        g0 = GEST_NONE;
    return g0;
}

static void handle_gesture_keys(uint32_t keys, uint32_t rising, uint32_t released, uint32_t edit)
{
    uint32_t gbits[4] = {1u << KEY_BLACK[7], 1u << KEY_BLACK[8], 1u << KEY_BLACK[9], 1u << KEY_BLACK[10]};
    uint32_t gi;
    uint8_t held_g = gesture_from_keys(keys, edit);

    for (gi = 0; gi < 4u; gi++) {
        if (rising & gbits[gi]) {
            if (g.gesture_latch != GEST_NONE && g.gesture_latch == held_g) {
                g.gesture_latch = GEST_NONE;
                g.gest_latch_badge = 0u;
            }
            g.gest_hold[gi] = 0u;
        }
        if (released & gbits[gi]) {
            g.gest_hold[gi] = 0u;
        }
    }
    if (held_g != GEST_NONE)
        g.gesture = held_g;
    else {
        g.gesture = g.gesture_latch;
        if (g.gesture_latch == GEST_NONE)
            g.gest_latch_badge = 0u;
    }
}

static void handle_keys(uint32_t keys, uint32_t rising, uint32_t released, uint32_t edit)
{
    uint32_t k;
    uint32_t fill_k = 1u << KEY_BLACK[7];
    uint32_t dsolo_k = 1u << KEY_BLACK[8];

    if (g.view == VIEW_LFO && rytm_is_seq_lfo_tgt(g.cfg.lfo.target)) {
        uint8_t ix = rytm_seq_lfo_ix(g.cfg.lfo.target);

        for (k = 0; k < 8u; k++) {
            if (rising & (1u << KEY_BLACK[k])) {
                g.cfg.lfo.seq_v[ix] = (uint8_t)k;
                g.seq_lane = (uint8_t)k;
                if (k < RYTM_NDVOICES)
                    g.cfg.sel_voice = (uint8_t)k;
                mark_dirty();
            }
        }
    }

    g.seq_plock_hold = 0;
    if (g.perf == PERF_SEQ) {
        for (k = 0; k < 16u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];
            if (keys & bit) {
                g.seq_plock_hold = 1u;
                g.seq_plock_step = (uint8_t)(seq_view_base() + k);
            }
            if (rising & bit) {
                g.seq_edit_step = (uint8_t)(seq_view_base() + k);
                seq_toggle_lane_step(g.seq_lane % 8u, (uint8_t)(seq_view_base() + k));
            }
        }
    }
    if (g.view == VIEW_HOME && g.perf != PERF_ARP) {
        for (k = 0; k < 8u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];
            if (rising & bit)
                mute_toggle_voice((uint8_t)k);
        }
        for (k = 8; k < 16u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];
            if (rising & bit)
                solo_toggle_voice((uint8_t)(k - 8u));
        }
    } else if ((g.view == VIEW_DRUM || g.bank == BANK_DRUM) && g.perf != PERF_SEQ && g.perf != PERF_ARP) {
        for (k = 0; k < 16u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];
            if (rising & bit)
                drum_toggle_step((uint8_t)(seq_view_base() + k));
        }
    }
    if (g.perf == PERF_SEQ) {
        uint8_t ln_seq = g.seq_lane % 8u;

        for (k = 0; k < 8u; k++) {
            uint32_t bit = 1u << KEY_BLACK[k];
            if (rising & bit) {
                if (edit) {
                    mute_toggle_voice((uint8_t)k);
                    g.edit_used = 1u;
                } else if (fm1_ms - g.bk_last_ms[k] < 300u)
                    solo_toggle_voice((uint8_t)k);
                else if (k != ln_seq) {
                    g.seq_lane = (uint8_t)k;
                    if (k < RYTM_NDVOICES)
                        g.cfg.sel_voice = (uint8_t)k;
                }
                g.bk_last_ms[k] = fm1_ms;
                mark_dirty();
            }
        }
        if (rising & (1u << KEY_BLACK[8])) {
            if (edit)
                rytm_invert_all_mutes();
            else if (rytm_any_mute_or_solo())
                clear_band_mute_solo();
            else
                mute_toggle_voice(ln_seq);
            mark_dirty();
        }
        if (rising & (1u << KEY_BLACK[9])) {
            if (edit) {
                uint32_t li;
                for (li = 0; li < 8u; li++)
                    seq_reset_lane_song((uint8_t)li);
            } else
                seq_reset_lane_song(ln_seq);
            mark_dirty();
        }
    } else {
        for (k = 0; k < RYTM_NDVOICES; k++) {
            uint32_t bit = 1u << KEY_BLACK[k];
            if (rising & bit) {
                if (g.view == VIEW_DRUM && fm1_ms - g.bk_last_ms[k] < 300u)
                    solo_toggle_voice((uint8_t)k);
                g.bk_last_ms[k] = fm1_ms;
                g.seq_lane = (uint8_t)k;
                g.cfg.sel_voice = (uint8_t)k;
                if (keys & dsolo_k)
                    solo_toggle_voice((uint8_t)k);
                else if ((keys & fill_k) || edit) {
                    mute_toggle_voice((uint8_t)k);
                    if (edit)
                        g.edit_used = 1u;
                } else {
                    live_trig_drum((uint8_t)k, 112);
                    if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR) {
                        uint8_t st = rec_quant_step((uint8_t)(g.tick % 6u));
                        fm_seq_bit_put(g.cfg.seq_ovr.drum_force_on[k], st, 1);
                        fm_seq_bit_put(g.cfg.seq_ovr.drum_force_off[k], st, 0);
                    }
                }
                mark_dirty();
            }
            if (!(keys & bit))
                g.arp_hits[k] = 0u;
            if ((keys & bit) && g.perf == PERF_ARP && g.playing && g.arp_mark[k] != g.tick) {
                uint8_t rate = g.cfg.arp_rate % 6u;
                uint32_t div = rate == 0u ? 12u : rate == 1u ? 6u : rate == 2u ? 4u : rate == 3u ? 3u : 6u;
                if ((g.tick % div) == 0u) {
                    int32_t ramp = (int32_t)g.cfg.arp_ramp - 64;
                    uint8_t hit = g.arp_hits[k] < 16u ? g.arp_hits[k] : 16u;
                    uint8_t vel = (uint8_t)rytm_clamp127(96 + ((int32_t)g.cfg.arp_spread * 31) / 127 + (ramp * (int32_t)hit) / 6);
                    int32_t p_off = (ramp * (int32_t)hit) / 3;
                    rytm_dvoice_cfg_t kv;

                    g.arp_mark[k] = g.tick;
                    if (g.arp_hits[k] < 32u)
                        g.arp_hits[k]++;
                    refresh_kit();
                    kv = g_kit[k];
                    kv.tune = rytm_clamp127((int32_t)kv.tune + p_off);
                    g_kit[k] = kv;
                    if (rate >= 4u || g.cfg.arp_spread >= 48u) {
                        rytm_drum_hit_t dh;
                        dh.vel = vel;
                        dh.accent = 1u;
                        dh.orn_kind = (rate == 5u || g.cfg.arp_spread >= 96u) ? ORN_ROLL : ORN_FLAM;
                        dh.orn_vel = (uint8_t)rytm_clamp127((int32_t)vel - 16 + ((int32_t)g.cfg.arp_spread >> 2));
                        dh.micro_ms = (int8_t)((int32_t)g.cfg.arp_spread / 4);
                        rytm_synth_schedule_ornament(&g.synth, (uint8_t)k, &dh, g.samples_per_tick * 6u,
                                                     12u + ((uint32_t)g.cfg.arp_spread * 33u) / 127u,
                                                     (int8_t)(p_off > 127 ? 127 : p_off < -127 ? -127 : p_off), 0);
                    } else {
                        rytm_synth_trig_drum(&g.synth, (uint8_t)k, vel, vel > 100u ? 1u : 0u, &kv, g.cfg.root,
                                             (uint8_t)(g.eff_fx.harmonic_toms));
                        midi_send(0x99u, GM_DRUM[k], vel);
                    }
                }
            }
        }
        if ((rising & (1u << KEY_BLACK[9])) && ((keys & fill_k) || edit)) {
            mute_toggle_voice(7u);
            if (edit)
                g.edit_used = 1u;
        }
        handle_gesture_keys(keys, rising, released, edit);
    }
    for (k = 0; k < 16u; k++) {
        uint32_t bit = 1u << KEY_WHITE[k];
        uint8_t note = rytm_scale_note(g.cfg.root, g.cfg.scale, (int32_t)k, (int32_t)g.bass_oct);
        uint8_t live_bass = (uint8_t)(!g.playing || g.perf == PERF_ARP || g.view == VIEW_BASS || g.bank == BANK_BASS ||
                                      (g.rec_arm && g.playing));
        if (rising & bit && g.perf != PERF_SEQ && !g.seq_plock_hold && live_bass) {
            g.arp_bass_step = 0u;
            if (g.rec_arm && g.playing && g.cfg.rec_mode != REC_MODE_MONITOR)
                rec_bass_note_on((uint8_t)k, note);
            if (g.perf == PERF_ARP && g.playing)
                live_trig_bass_note(note, 118);
            else if (!g.rec_arm || g.cfg.rec_mode == REC_MODE_MONITOR)
                live_trig_bass_note(note, 108);
            else
                live_trig_bass_note(note, 110);
            mark_dirty();
        }
        if ((keys & bit) && g.perf == PERF_ARP && g.playing && g.arp_bass_mark != g.tick) {
            uint8_t rate = g.cfg.arp_rate % 6u;
            uint32_t div = rate == 0u ? 12u : rate == 1u ? 6u : rate == 2u ? 4u : rate == 3u ? 3u : 6u;
            if ((g.tick % div) == 0u) {
                static const int8_t PAT_OFF[5][4] = {
                    {0, 2, 4, 7},
                    {7, 4, 2, 0},
                    {0, 7, 14, 7},
                    {0, 4, 7, 11},
                    {0, 7, -5, 3}
                };
                uint8_t pat = (uint8_t)(((g.cfg.arp_solo * 4u + 63u) / 127u) % 5u);
                uint8_t a_ix = g.arp_bass_step & 3u;
                int32_t deg = (int32_t)k + (int32_t)PAT_OFF[pat][a_ix];
                int32_t ramp = (int32_t)g.cfg.arp_ramp - 64;
                uint8_t vel = (uint8_t)rytm_clamp127(100 + ((int32_t)g.cfg.arp_spread >> 2) + (ramp * (int32_t)(g.arp_bass_step & 7u)) / 6);

                g.arp_bass_mark = g.tick;
                g.arp_bass_step++;
                live_trig_bass_note(rytm_scale_note(g.cfg.root, g.cfg.scale, deg, (int32_t)g.bass_oct), vel);
            }
        }
        if (!(keys & bit) && g.rec_wkey[k] && g.rec_arm)
            rec_bass_note_off((uint8_t)k);
    }
}

static void cycle_persona(int32_t dir, uint8_t drummer_only, uint8_t bassist_only)
{
    if (drummer_only) {
        int32_t d = (int32_t)g.cfg.drummer + dir;
        while (d < 0)
            d += RYTM_NDRUMMERS;
        g.cfg.drummer_auto = 0u;
        g.cfg.drummer = (uint8_t)(d % RYTM_NDRUMMERS);
    } else if (bassist_only) {
        int32_t b = (int32_t)g.cfg.bassist + dir;
        while (b < 0)
            b += RYTM_NBASSISTS;
        g.cfg.bassist_auto = 0u;
        g.cfg.bassist = (uint8_t)(b % RYTM_NBASSISTS);
        g.cfg.bass.model = RYTM_BASSIST_DEFAULT_MODEL[g.cfg.bassist];
    } else if (g.bank == BANK_DRUM || g.view == VIEW_DRUM) {
        cycle_persona(dir, 1, 0);
    } else if (g.bank == BANK_BASS || g.view == VIEW_BASS) {
        cycle_persona(dir, 0, 1);
    } else if (g.cfg.drummer_auto && !g.cfg.bassist_auto) {
        cycle_persona(dir, 0, 1);
    } else if (!g.cfg.drummer_auto && g.cfg.bassist_auto) {
        cycle_persona(dir, 1, 0);
    } else {
        int32_t p = (int32_t)g.cfg.pair_ix + dir;
        while (p < 0)
            p += 8;
        g.cfg.pair_ix = (uint8_t)(p % 8);
        g.cfg.drummer_auto = 0u;
        g.cfg.bassist_auto = 0u;
        g.cfg.drummer = BAND_PAIR_DRUM[g.cfg.pair_ix];
        g.cfg.bassist = BAND_PAIR_BASS[g.cfg.pair_ix];
        g.cfg.bass.model = RYTM_BASSIST_DEFAULT_MODEL[g.cfg.bassist];
    }
    mark_dirty();
}

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t i, v, t;
    rytm_fx_cfg_t fx;
    rytm_dvoice_cfg_t kit[RYTM_NDVOICES];
    uint32_t inc[LFO_NTARGETS];

    if (g.xport_reset_req) {
        transport_reset();
        g.ext_f8_done = g.ext_f8_in;
        g.xport_reset_req = 0u;
        for (t = 0; t < LFO_NTARGETS; t++) {
            g.lfo_ph[t] = 0u;
            g.lfo_cycle[t] = 0u;
        }
    }
    if (g.ext_clock) {
        uint32_t k = 0u;
        while (g.ext_f8_done != g.ext_f8_in && k < 8u) {
            g.ext_f8_done++;
            k++;
            g_evt_ofs = 0u;
            seq_advance();
        }
    }

    for (t = 0; t < LFO_NTARGETS; t++) {
        uint32_t p = lfo_period_samples_for(g.cfg.lfo.rate[t]);
        uint32_t base_inc = p ? (4294967295u / p) : 0u;
        inc[t] = base_inc;
    }
    for (t = 0; t < LFO_NTARGETS; t++) {
        uint64_t adv = (uint64_t)inc[t] * (uint64_t)n;
        uint64_t sum = (uint64_t)g.lfo_ph[t] + adv;
        g.lfo_ph[t] = (uint32_t)sum;
        g.lfo_cycle[t] += (uint32_t)(sum >> 32);
    }

    for (i = 0; i < n; i++) {
        if (g.playing && !g.ext_clock) {
            g.sample_acc++;
            while (g.sample_acc >= g.samples_per_tick) {
                g.sample_acc -= g.samples_per_tick;
                g_evt_ofs = i;
                seq_advance();
                g_evt_ofs = 0u;
                if ((g.tick & 1u) == 0u)
                    midi_clock_tx();
            }
        }
        out[i * 2u] = 0;
        out[i * 2u + 1u] = 0;
    }

    lfo_step_all_slots();
    if (!g.playing) {
        rytm_apply_lfo_seq_mutes();
        rytm_sync_heard_mutes();
    }
    sync_effective_params();
    refresh_kit();
    morph_clock(n);
    for (v = 0; v < RYTM_NDVOICES; v++)
        kit[v] = g_kit[v];
    g.eff_dtun = kit[g.cfg.sel_voice % RYTM_NDVOICES].tune;
    fx = g.eff_fx;
    {
        int32_t rs = (int32_t)fx.rev_mix + (((int32_t)g.eff_rev_size - 64) * (int32_t)fx.rev_mix) / 64;
        fx.rev_mix = (uint8_t)rytm_clamp127(rs);
    }
    rytm_synth_render(&g.synth, out, n, kit, &g.eff_bass, &fx, active_chord_root());
}

static void rytm_auto_pair_poll(void)
{
    if (g.cfg.drummer_auto || g.cfg.bassist_auto) {
        uint8_t glo, ghi, gf, near;
        rytm_genre_pair(genre_pos4095(), &glo, &ghi, &gf);
        near = gf >= 128u ? ghi : glo;
        if (g.cfg.drummer_auto)
            g.cfg.drummer = BAND_PAIR_DRUM[near % 8u];
        if (g.cfg.bassist_auto) {
            uint8_t nb = BAND_PAIR_BASS[near % 8u];
            if (g.cfg.bassist != nb) {
                g.cfg.bassist = nb;
                g.cfg.bass.model = RYTM_BASSIST_DEFAULT_MODEL[nb];
            }
        }
    }
}

static void update_leds(uint32_t keys)
{
    uint32_t k;
    if (g.perf == PERF_SEQ || g.view == VIEW_DRUM || g.bank == BANK_DRUM) {
        uint8_t ln = (g.perf == PERF_SEQ) ? (g.seq_lane % 8u) : (g.cfg.sel_voice % RYTM_NDVOICES);
        rytm_query_t q;
        rytm_step_events_t ev;

        build_query(&q);
        for (k = 0; k < 16u; k++) {
            int has_beat;

            query_at(&q, seq_view_base() + k);
            rytm_resolve_step(&q, &g.eff_seq_ovr, &ev);
            has_beat = ln < RYTM_NDVOICES ? (ev.drums[ln].vel > 0u) : (ev.bass.active != 0u);
            fm1_led_key(KEY_LED(KEY_WHITE[k]), (g.playing && g.step == seq_view_base() + k) ? !has_beat : has_beat);
        }
        if (g.perf == PERF_SEQ) {
            for (k = 0; k < 11u; k++)
                fm1_led_key(KEY_LED(KEY_BLACK[k]), (k < 8u && g.seq_lane == k) || (keys >> KEY_BLACK[k] & 1u));
        } else {
            uint8_t act_gest = g.gesture_latch ? g.gesture_latch : g.gesture;
            for (k = 0; k < 7u; k++) {
                int lit = (g.synth.voice_peak[k] > 48u) || (k == g.cfg.sel_voice && !g.cfg.dv[k].mute);
                fm1_led_key(KEY_LED(KEY_BLACK[k]), lit);
            }
            fm1_led_key(KEY_LED(KEY_BLACK[7]), act_gest == GEST_FILL || act_gest == GEST_BREAK);
            fm1_led_key(KEY_LED(KEY_BLACK[8]), act_gest == GEST_SOLO_DRUM || act_gest == GEST_SOLO_BAND);
            fm1_led_key(KEY_LED(KEY_BLACK[9]), act_gest == GEST_SOLO_BASS || act_gest == GEST_SOLO_BAND);
            fm1_led_key(KEY_LED(KEY_BLACK[10]), act_gest == GEST_FLAM_BURST);
        }
    } else {
        uint8_t act_gest = g.gesture_latch ? g.gesture_latch : g.gesture;
        for (k = 0; k < 7u; k++) {
            int lit = (g.synth.voice_peak[k] > 48u) || ((keys >> KEY_BLACK[k]) & 1u);
            fm1_led_key(KEY_LED(KEY_BLACK[k]), lit);
        }
        fm1_led_key(KEY_LED(KEY_BLACK[7]), act_gest == GEST_FILL || act_gest == GEST_BREAK);
        fm1_led_key(KEY_LED(KEY_BLACK[8]), act_gest == GEST_SOLO_DRUM || act_gest == GEST_SOLO_BAND);
        fm1_led_key(KEY_LED(KEY_BLACK[9]), act_gest == GEST_SOLO_BASS || act_gest == GEST_SOLO_BAND);
        fm1_led_key(KEY_LED(KEY_BLACK[10]), act_gest == GEST_FLAM_BURST);
        for (k = 0; k < 16u; k++) {
            uint8_t pc = (uint8_t)((KEY0 + KEY_WHITE[k]) % 12u);
            int lit = (keys >> KEY_WHITE[k] & 1u) || (pc == (g.cfg.root % 12u) && k < 12u);
            fm1_led_key(KEY_LED(KEY_WHITE[k]), lit);
        }
    }
    fm1_led_key(BTN_PLAY, g.playing);
    fm1_led_key(BTN_ARP, g.perf == PERF_ARP);
    fm1_led_key(BTN_REC, g.rec_arm || g.view == VIEW_REC);
    {
        static const uint8_t BBTN[5] = {BTN_SEL, BTN_ENV, BTN_GLO, BTN_FX, BTN_LFO};
        static const uint8_t BBNK[5] = {BANK_GROOVE, BANK_DRUM, BANK_BASS, BANK_FX, BANK_LFO};
        int page = g.perf == PERF_ARP || g.perf == PERF_SEQ;
        uint32_t phase = (fm1_ms / 110u) % 8u;
        int p2_pulse = (phase == 0u || phase == 2u);
        uint32_t bi;

        if (g.perf == PERF_SEQ) {
            if (!g.seq_page)
                fm1_led_key(BTN_SEQ, 1);
            else {
                fm1_led_key(BTN_SEQ, p2_pulse);
                if (!p2_pulse)
                    fm1_led_key_dim(BTN_SEQ, 1);
            }
        } else {
            fm1_led_key(BTN_SEQ, 0);
        }

        fm1_led_key(BTN_HOME, !page && g.bank == BANK_HOME);
        for (bi = 0; bi < 5u; bi++) {
            int active = !page && g.bank == BBNK[bi];
            int is_p2 = BBNK[bi] == BANK_BASS ? g.cfg.bank_page[BANK_BASS] != 0u : (g.cfg.bank_page[BBNK[bi]] & 1u) != 0u;
            if (!active) {
                fm1_led_key(BBTN[bi], 0);
            } else if (!is_p2) {
                fm1_led_key(BBTN[bi], 1);
            } else {
                fm1_led_key(BBTN[bi], p2_pulse);
                if (!p2_pulse)
                    fm1_led_key_dim(BBTN[bi], 1);
            }
        }
    }
    fm1_led_key(BTN_EDIT, (fm1_in.buttons & (1u << BTN_EDIT)) != 0u || g.gesture_latch != GEST_NONE);
}

/* SEQ view: OCT -/+ turn the page; past the first or last page the pattern doubles (fm_seq_view_nav).
 * SEL or GLO held keep OCT on the personas. */
static int seq_oct_takes(void)
{
    return g.view == VIEW_SEQ && !(fm1_in.buttons & ((1u << BTN_SEL) | (1u << BTN_GLO)));
}

/* A doubling copies every per-step override; after a prepend the playhead and the held REC notes stay
 * on the original, now the second half. */
static void seq_oct(int32_t dir)
{
    rytm_seq_ovr_t *o = &g.cfg.seq_ovr;
    uint8_t pages = seq_pages();
    uint32_t len = seq_len();
    uint8_t nav = fm_seq_view_nav(&g.seq_view_page, pages, dir);
    char msg[8];
    uint32_t v;

    if (nav == FM_SEQ_NAV_APPEND || nav == FM_SEQ_NAV_PREPEND) {
        for (v = 0; v < RYTM_NDVOICES; v++) {
            fm_seq_bits_dup(o->drum_force_on[v], len);
            fm_seq_bits_dup(o->drum_force_off[v], len);
            fm_seq_bits_dup(o->drum_accent[v], len);
            fm_seq_bits_dup(o->drum_flam[v], len);
        }
        fm_seq_bits_dup(o->bass_force_on, len);
        fm_seq_bits_dup(o->bass_force_off, len);
        fm_seq_dup_bytes(o->bass_note, len, 1u);
        fm_seq_dup_bytes(o->bass_acc, len, 1u);
        fm_seq_dup_bytes(o->bass_gate, len, 1u);
        for (v = 0; v < 8u; v++) {
            fm_seq_dup_bytes(o->step_vel[v], len, 1u);
            fm_seq_dup_bytes(o->step_prob[v], len, 1u);
            fm_seq_dup_bytes(o->step_micro[v], len, 1u);
            fm_seq_dup_bytes(o->step_tune[v], len, 1u);
        }
        o->pages = (uint8_t)(pages * 2u);
        g.step = (uint8_t)fm_seq_dup_ix(g.step, len, nav);
        g.seq_edit_step = (uint8_t)fm_seq_dup_ix(g.seq_edit_step, len, nav);
        for (v = 0; v < 16u; v++)
            if (g.rec_wkey[v])
                g.rec_note_st[v] = (uint8_t)fm_seq_dup_ix(g.rec_note_st[v], len, nav);
        mark_dirty();
    }
    fm_seq_fmt_nav(msg, nav, g.seq_view_page, seq_pages());
    seq_flash_knob(msg);
    g.ui_dirty = 1u;
}

static void seq_ovr_from_s16(rytm_seq_ovr_t *n, const rytm_seq_ovr_s16_t *o)
{
    uint32_t v;

    memset(n, 0, sizeof *n);
    n->pages = 1u;
    for (v = 0; v < RYTM_NDVOICES; v++) {
        fm_seq_bits_set_page16(n->drum_force_on[v], 0u, o->drum_force_on[v]);
        fm_seq_bits_set_page16(n->drum_force_off[v], 0u, o->drum_force_off[v]);
        fm_seq_bits_set_page16(n->drum_accent[v], 0u, o->drum_accent[v]);
        fm_seq_bits_set_page16(n->drum_flam[v], 0u, o->drum_flam[v]);
    }
    fm_seq_bits_set_page16(n->bass_force_on, 0u, o->bass_force_on);
    fm_seq_bits_set_page16(n->bass_force_off, 0u, o->bass_force_off);
    memcpy(n->bass_note, o->bass_note, sizeof o->bass_note);
    memcpy(n->bass_acc, o->bass_acc, sizeof o->bass_acc);
    n->vary = o->vary;
    memcpy(n->vary_lane, o->vary_lane, sizeof o->vary_lane);
    n->freeze = o->freeze;
    for (v = 0; v < 8u; v++) {
        memcpy(n->step_vel[v], o->step_vel[v], sizeof o->step_vel[v]);
        memcpy(n->step_prob[v], o->step_prob[v], sizeof o->step_prob[v]);
        memcpy(n->step_micro[v], o->step_micro[v], sizeof o->step_micro[v]);
        memcpy(n->step_tune[v], o->step_tune[v], sizeof o->step_tune[v]);
    }
    memcpy(n->bass_gate, o->bass_gate, sizeof o->bass_gate);
    memcpy(n->lane_vel, o->lane_vel, sizeof o->lane_vel);
    memcpy(n->lane_trig, o->lane_trig, sizeof o->lane_trig);
    memcpy(n->lane_gate, o->lane_gate, sizeof o->lane_gate);
    memcpy(n->lane_tune, o->lane_tune, sizeof o->lane_tune);
}

static int rytm_cfg_load_s16(rytm_cfg_t *c)
{
    static rytm_cfg_s16_t o;
    size_t tail = offsetof(rytm_cfg_s16_t, bank_page) + sizeof o.bank_page - offsetof(rytm_cfg_s16_t, lfo);

    if (settings_load(&o, sizeof o) != 0 || o.magic != RYTM_CFG_MAGIC_S16)
        return -1;
    memcpy(c, &o, offsetof(rytm_cfg_s16_t, seq_ovr));
    seq_ovr_from_s16(&c->seq_ovr, &o.seq_ovr);
    memcpy(&c->lfo, &o.lfo, tail);
    c->magic = RYTM_CFG_MAGIC;
    return 0;
}

static void app_init(void)
{
    palette_set(2u);
    if ((settings_load(&g.cfg, sizeof g.cfg) != 0 || g.cfg.magic != RYTM_CFG_MAGIC) && rytm_cfg_load_s16(&g.cfg) != 0)
        cfg_defaults();
    else
        rytm_sanitize_saved_motion();
    transport_set_bpm();
    rytm_synth_init(&g.synth);
    reset_home_macros();
    g.cfg.bank_page[BANK_HOME] = 0u;
    sync_effective_params();
    transport_reset();
    transport_set_bpm();
    g.view = VIEW_BAND;
    g.bank = BANK_GROOVE;
    g.perf = PERF_LIVE;
    g.rng = 991u;
    g.seq_ens_sel = FM_SEQ_ENS_ALL;
    g.ui_dirty = 1u;
    draw_screen();
}

static void reset_bank(uint8_t bank)
{
    uint32_t i;
    rytm_dvoice_cfg_t tmp_dv[RYTM_NDVOICES];
    rytm_bass_cfg_t tmp_b;
    rytm_fx_cfg_t tmp_fx;
    rytm_synth_preset_defaults(tmp_dv, &tmp_b, &tmp_fx);

    if (bank == BANK_GROOVE) {
        g.cfg.density = 96;
        g.cfg.feel = 64;
        g.cfg.flam = 48;
        g.cfg.bass_density = 96;
        g.cfg.bass_feel = 64;
        g.cfg.bass_flam = 48;
        g.cfg.swing_max = 65;
        g.cfg.human = 100;
        g.cfg.drummer = 1;
        g.cfg.bassist = 1;
        g.cfg.drummer_auto = 0;
        g.cfg.bassist_auto = 0;
    } else if (bank == BANK_DRUM) {
        uint32_t p, s;
        for (i = 0; i < RYTM_NDVOICES; i++) {
            g.cfg.dv[i] = tmp_dv[i];
            for (p = 0; p < 8u; p++)
                for (s = 0; s < RYTM_MOTION_STEPS; s++)
                    g.cfg.drum_motion[i][p][s] = 255u;
        }
        g.cfg.fx.harmonic_toms = 0;
    } else if (bank == BANK_BASS) {
        g.cfg.bass = tmp_b;
        g.cfg.bass.model = RYTM_BASSIST_DEFAULT_MODEL[g.cfg.bassist % RYTM_NBASSISTS];
        g.cfg.bass_mute = 0;
        g.cfg.scale = 0;
        g.cfg.prog_depth = 96;
    } else if (bank == BANK_FX) {
        g.cfg.fx = tmp_fx;
        g.cfg.fx_rev_size = 64;
    } else if (bank == BANK_LFO) {
        for (i = 0; i < LFO_NTARGETS; i++) {
            g.cfg.lfo.wave[i] = 0;
            g.cfg.lfo.rate[i] = 2;
            g.cfg.lfo.depth[i] = 0;
            g.cfg.lfo.phase[i] = 0;
            g.cfg.lfo.smooth[i] = 0;
            g.cfg.lfo.skew[i] = FM_LFO_SKEW_DEFAULT;
            g.cfg.lfo.algo[i] = 0;
            g.cfg.lfo.ratio[i] = 52u;
            g.cfg.lfo.index[i] = 64u;
            g.cfg.lfo.fb[i] = 0;
            g.cfg.lfo.fold[i] = 0;
        }
    } else if (bank == BANK_HOME) {
        uint32_t s;
        reset_home_macros();
        for (i = 0; i < 16u; i++)
            for (s = 0; s < RYTM_MOTION_STEPS; s++)
                g.cfg.macro_motion[i][s] = 255u;
    }
    mark_dirty();
}

static void reset_all(void)
{
    cfg_defaults();
    transport_set_bpm();
    reset_home_macros();
    clear_band_mute_solo();
    g.rec_arm = 0u;
    mark_dirty();
}

static void nav_bank(uint8_t bank)
{
    g.bank = bank;
    g.perf = PERF_LIVE;
    if (bank == BANK_HOME)
        g.view = VIEW_HOME;
    else if (bank == BANK_DRUM)
        g.view = VIEW_DRUM;
    else if (bank == BANK_BASS)
        g.view = VIEW_BASS;
    else if (bank == BANK_LFO)
        g.view = VIEW_LFO;
    else if (bank == BANK_GROOVE || bank == BANK_FX)
        g.view = VIEW_BAND;
    else if (g.view == VIEW_SEQ || g.view == VIEW_REC)
        g.view = VIEW_BAND;
    g.ui_dirty = 1u;
}

static void nav_stage(uint8_t view)
{
    view = (uint8_t)(view % VIEW_COUNT);
    if (view == VIEW_SEQ && g.perf == PERF_ARP)
        g.perf = PERF_LIVE;
    g.view = view;
    if (g.view == VIEW_SEQ)
        g.perf = PERF_SEQ;
    else if (g.perf == PERF_SEQ)
        g.perf = PERF_LIVE;
    if (g.view == VIEW_DRUM)
        g.bank = BANK_DRUM;
    else if (g.view == VIEW_BASS)
        g.bank = BANK_BASS;
    else if (g.view == VIEW_HOME)
        g.bank = BANK_HOME;
    else if (g.view == VIEW_LFO)
        g.bank = BANK_LFO;
    else if (g.view == VIEW_BAND)
        g.bank = BANK_GROOVE;
    g.ui_dirty = 1u;
}

static void app_step(void)
{
    uint32_t b, k, rising, released, keys = fm1_in.notes;
    uint8_t st, d1, d2;

    platform_poll();
    b = fm1_input_edges(&released);
    rising = (keys & ~g.prev_keys) | fm1_input_note_edges();
    g.prev_keys = keys;

    {
        uint8_t edit_held = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
        if (edit_held && (b & ((1u << BTN_HOME) | (1u << BTN_SEL) | (1u << BTN_ENV) | (1u << BTN_GLO) |
                               (1u << BTN_FX) | (1u << BTN_LFO) | (1u << BTN_ARP) | (1u << BTN_SEQ) | (1u << BTN_REC))))
            g.edit_used = 1u;

        if (b & (1u << BTN_HOME)) {
            if (edit_held) {
                reset_all();
            } else if (fm1_ms - g.home_last_ms < 260u && g.bank == BANK_HOME && g.perf == PERF_LIVE)
                clear_band_mute_solo();
            else if (g.perf == PERF_LIVE && g.bank == BANK_HOME && g.view == VIEW_HOME)
                toggle_bank_page(BANK_HOME);
            else
                nav_bank(BANK_HOME);
            g.home_last_ms = fm1_ms;
        }
        if (b & (1u << BTN_SEL)) {
            if (edit_held)
                reset_bank(BANK_GROOVE);
            else if (g.perf == PERF_LIVE && g.bank == BANK_GROOVE && g.view == VIEW_BAND)
                toggle_bank_page(BANK_GROOVE);
            else if (fm1_ms - g.sel_last_ms < 300u)
                clear_band_mute_solo();
            else
                nav_bank(BANK_GROOVE);
            g.sel_last_ms = fm1_ms;
        }
        if (b & (1u << BTN_ENV)) {
            if (edit_held)
                reset_bank(BANK_DRUM);
            else if (g.perf == PERF_LIVE && g.bank == BANK_DRUM && g.view == VIEW_DRUM)
                toggle_bank_page(BANK_DRUM);
            else
                nav_bank(BANK_DRUM);
        }
        if (b & (1u << BTN_GLO)) {
            if (edit_held)
                reset_bank(BANK_BASS);
            else if (g.perf == PERF_LIVE && g.bank == BANK_BASS && g.view == VIEW_BASS)
                toggle_bank_page(BANK_BASS);
            else
                nav_bank(BANK_BASS);
        }
        if (b & (1u << BTN_FX)) {
            if (edit_held)
                reset_bank(BANK_FX);
            else if (g.perf == PERF_LIVE && g.bank == BANK_FX && g.view != VIEW_SEQ && g.view != VIEW_REC)
                toggle_bank_page(BANK_FX);
            else
                nav_bank(BANK_FX);
        }
        if (b & (1u << BTN_LFO)) {
            g.mod_btn_used = 0u;
            if (edit_held)
                reset_bank(BANK_LFO);
            else if (g.perf == PERF_LIVE && g.bank == BANK_LFO && g.view == VIEW_LFO)
                toggle_bank_page(BANK_LFO);
            else
                nav_bank(BANK_LFO);
        }
        if (released & (1u << BTN_LFO)) {
            g.ui_dirty = 1u;
        }
        if (b & (1u << BTN_ARP)) {
            if (edit_held) {
                g.cfg.arp_rate = 1;
                g.cfg.arp_spread = 64;
                g.cfg.arp_solo = 0;
                g.cfg.arp_ramp = 64;
                mark_dirty();
            } else if (g.perf == PERF_ARP)
                g.perf = PERF_LIVE;
            else {
                g.perf = PERF_ARP;
                if (g.view == VIEW_SEQ)
                    g.view = g.bank == BANK_DRUM ? VIEW_DRUM : g.bank == BANK_BASS ? VIEW_BASS : VIEW_BAND;
            }
            g.ui_dirty = 1u;
        }
        if (b & (1u << BTN_SEQ)) {
            if (edit_held) {
                memset(&g.cfg.seq_ovr, 0, sizeof g.cfg.seq_ovr);
                memset(g.cfg.euclid_hits, 0, sizeof g.cfg.euclid_hits);
                memset(g.cfg.euclid_rot, 0, sizeof g.cfg.euclid_rot);
                mark_dirty();
            } else if (g.perf == PERF_SEQ) {
                g.seq_page = (uint8_t)((g.seq_page + 1u) % FM_SEQ_NPAGES);
                g.ui_dirty = 1u;
            } else {
                g.seq_page = 0u;
                nav_stage(VIEW_SEQ);
            }
        }
    }
    if (b & (1u << BTN_REC)) {
        if (fm1_in.buttons & (1u << BTN_EDIT)) {
            clear_all_overlays();
            g.rec_arm = 0u;
        } else if (g.view == VIEW_REC) {
            g.view = g.bank == BANK_DRUM ? VIEW_DRUM : g.bank == BANK_BASS ? VIEW_BASS :
                     g.bank == BANK_HOME ? VIEW_HOME : g.bank == BANK_LFO ? VIEW_LFO : VIEW_BAND;
        } else if (fm1_ms - g.rec_last_ms < 300u) {
            g.view = VIEW_REC;
            g.rec_arm = 1u;
        } else {
            g.rec_arm = g.rec_arm ? 0u : 1u;
        }
        g.rec_last_ms = fm1_ms;
        g.ui_dirty = 1u;
    }
    if (b & (1u << BTN_EDIT)) {
        g.edit_used = 0u;
        if (g.view == VIEW_REC) {
            exec_rec_clear();
            g.edit_used = 1u;
        }
    }
    if ((released & (1u << BTN_EDIT)) && g.perf == PERF_SEQ && !g.edit_used)
        seq_clear_lane(g.seq_lane % 8u);
    if (b & (1u << BTN_PLAY)) {
        g.playing = g.playing ? 0u : 1u;
        if (g.playing) {
            g.xport_reset_req = 1u;
            midi_start_tx();
        } else {
            rytm_synth_note_off_bass(&g.synth);
            midi_stop_tx();
        }
        g.ui_dirty = 1u;
    }
    if (b & (1u << BTN_SAVE)) {
        if (g.playing) {
            g.set_dirty = 1u;
            g.save_req = 1u;
            g.set_t = fm1_ms;
        } else {
            settings_save(&g.cfg, sizeof g.cfg);
            g.set_dirty = 0u;
            g.save_req = 0u;
            g.saved_until = fm1_ms + 1200u;
        }
        g.ui_dirty = 1u;
    }
    g.oct_down = (fm1_in.buttons & (1u << BTN_OCT_DOWN)) != 0u;
    g.oct_up = (fm1_in.buttons & (1u << BTN_OCT_UP)) != 0u;
    if ((b & (1u << BTN_OCT_DOWN)) && seq_oct_takes()) {
        seq_oct(-1);
    } else if (b & (1u << BTN_OCT_DOWN)) {
        if (g.view == VIEW_REC) {
            uint8_t act[REC_TOTAL_SLOTS];
            uint32_t n = rec_build_active_list(act);
            if (n > 0u)
                g.rec_lane = (uint8_t)((g.rec_lane + n - 1u) % n);
        } else if (g.view == VIEW_DRUM || g.bank == BANK_DRUM || (fm1_in.buttons & (1u << BTN_SEL))) {
            cycle_persona(-1, 1, 0);
        } else if (fm1_in.buttons & (1u << BTN_GLO))
            cycle_persona(-1, 0, 1);
        else {
            int32_t r = (int32_t)g.cfg.root - 1;
            while (r < 0)
                r += 12;
            g.cfg.root = (uint8_t)r;
        }
        mark_dirty();
    }
    if ((b & (1u << BTN_OCT_UP)) && seq_oct_takes()) {
        seq_oct(1);
    } else if (b & (1u << BTN_OCT_UP)) {
        if (g.view == VIEW_REC) {
            uint8_t act[REC_TOTAL_SLOTS];
            uint32_t n = rec_build_active_list(act);
            if (n > 0u)
                g.rec_lane = (uint8_t)((g.rec_lane + 1u) % n);
        } else if (g.view == VIEW_DRUM || g.bank == BANK_DRUM || (fm1_in.buttons & (1u << BTN_SEL))) {
            cycle_persona(1, 1, 0);
        } else if (fm1_in.buttons & (1u << BTN_GLO))
            cycle_persona(1, 0, 1);
        else
            g.cfg.root = (uint8_t)((g.cfg.root + 1u) % 12u);
        mark_dirty();
    }

    {
        int32_t gsteps = fm1_enc_take(ENC_ALGORITHM);
        if (gsteps) {
            if (fm1_in.buttons & (1u << BTN_EDIT)) {
                uint32_t s;
                g.edit_used = 1u;
                for (s = 0; s < RYTM_MOTION_STEPS; s++)
                    g.cfg.motion_val[MOT_GENRE][s] = 255u;
            } else {
                int32_t base = (g.rec_arm && g.playing) ? (int32_t)g.eff_genre : (int32_t)g.cfg.genre;
                int32_t v = base + knob_gain(gsteps);
                if (v < 0)
                    v = 0;
                if (v > 127)
                    v = 127;
                g.cfg.genre = (uint8_t)v;
                g.cfg.lfo.target = LFO_TGT_GENRE;
                motion_write(MOT_GENRE, g.cfg.genre);
            }
            mark_dirty();
        }
    }
    {
        int32_t psteps = fm1_enc_take(ENC_PRESETS);
        if (psteps) {
            if (g.view == VIEW_REC) {
                uint8_t act[REC_TOTAL_SLOTS];
                uint32_t n = rec_build_active_list(act);
                if (n > 0u) {
                    int32_t nl = (int32_t)g.rec_lane + (psteps > 0 ? 1 : -1);
                    while (nl < 0)
                        nl += (int32_t)n;
                    g.rec_lane = (uint8_t)((uint32_t)nl % n);
                }
                g.ui_dirty = 1u;
            } else if (g.view == VIEW_DRUM || g.bank == BANK_DRUM) {
                int32_t nv = (int32_t)g.cfg.sel_voice + (psteps > 0 ? 1 : -1);
                while (nv < 0)
                    nv += (int32_t)RYTM_NDVOICES;
                g.cfg.sel_voice = (uint8_t)((uint32_t)nv % RYTM_NDVOICES);
                g.seq_lane = g.cfg.sel_voice;
                mark_dirty();
            } else {
                cycle_persona(psteps, fm1_in.buttons & (1u << BTN_SEL), fm1_in.buttons & (1u << BTN_GLO));
            }
        }
    }
    {
        int32_t vsteps = fm1_enc_take(ENC_SELECT);
        if (vsteps) {
            static const uint8_t order[5] = {VIEW_BAND, VIEW_DRUM, VIEW_BASS, VIEW_SEQ, VIEW_HOME};
            uint32_t i, ix = 0;
            for (i = 0; i < 5u; i++)
                if (order[i] == g.view)
                    ix = i;
            ix = (uint32_t)((int32_t)ix + vsteps);
            while ((int32_t)ix < 0)
                ix += 5u;
            nav_stage(order[ix % 5u]);
        }
    }
    for (k = 0; k < 4u; k++) {
        int32_t steps = fm1_enc_take(ENC_KNOB1 + k);
        if (steps)
            knob_adjust((uint8_t)k, steps);
    }

    if (g.oct_down && g.oct_up && g.view == VIEW_DRUM)
        solo_toggle_voice(g.cfg.sel_voice);
    handle_keys(keys, rising, released, fm1_in.buttons & (1u << BTN_EDIT));

    while (midi_get(&st, &d1, &d2)) {
        if (st == 0xF8u) {
            g.ext_clock = 1;
            g.ext_last_ms = fm1_ms;
            if (g.playing)
                g.ext_f8_in++;
        } else if (st == 0xFAu) {
            g.playing = 1;
            g.ext_clock = 1;
            g.ext_last_ms = fm1_ms;
            g.xport_reset_req = 1u;
        } else if (st == 0xFCu) {
            g.playing = 0;
            g.ext_clock = 0;
        } else if ((st & 0xF0u) == 0xB0u && d1 >= 14u && d1 <= 31u)
            apply_cc(d1, d2);
        else if (st == 0x99u && d2)
            live_trig_drum((uint8_t)(d1 == 36 ? 0 : d1 == 38 ? 1 : d1 == 42 ? 2 : d1 == 46 ? 3 : d1 == 45 ? 4 : d1 == 50 ? 5 : 6), d2);
        else if ((st & 0xF0u) == 0x90u && d2)
            live_trig_bass_note((uint8_t)(d1 + g.cfg.root), d2);
    }
    if (g.ext_clock && fm1_ms - g.ext_last_ms > 1000u)
        g.ext_clock = 0;

    update_leds(keys);
    rytm_auto_pair_poll();

    if (g.ui_dirty || fm1_ms - g.ui_ms >= 50u) {
        draw_screen();
        g.ui_dirty = 0u;
        g.ui_ms = fm1_ms;
    }
    if ((g.set_dirty || g.save_req) && rytm_synth_is_quiet(&g.synth) &&
        (g.save_req ? (!g.playing || fm1_ms - g.set_t > 2000u) : (fm1_ms - g.set_t > 2000u))) {
        settings_save(&g.cfg, sizeof g.cfg);
        g.set_dirty = 0u;
        g.save_req = 0u;
        g.saved_until = fm1_ms + 1200u;
        g.ui_dirty = 1u;
    }
}

static void app_main(void)
{
    app_init();
    for (;;)
        app_step();
}
