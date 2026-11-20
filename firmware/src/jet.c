/* SPDX-License-Identifier: GPL-3.0-only */
#define APP_NAME "KO JET"
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_56415"
#endif
#include "platform.c"
#define C_ACC RGB(120, 200, 255)
#define KO_POT_TICK_CAP 32u
#include "ko_chrome.h"
#define DUB_KNOB_RAIL_COLOR_SCALE(col, amt) ((void)(amt), (col))
#include "dub_knob_progress_rail.h"
#include "ko_era_plate.h"
#include "fm_seq.h"
#include "fm_lfo.h"
static uint16_t color_scale_rgb(uint16_t col, int32_t scale256);
#include "fm_env_kit.h"
#include "deck_store.h"
#include "ko_rec.h"
#include "jet_patch.h"
#include "jet_plates.h"
#include "jet_dsp.h"
#include "jet_seq.h"

static uint16_t color_scale_rgb(uint16_t col, int32_t scale256) /* NOLINT: fm_env_kit.h */
{
    int32_t r = ((col >> 11) & 31) << 3;
    int32_t g_c = ((col >> 5) & 63) << 2;
    int32_t b = (col & 31) << 3;

    if (scale256 <= 0)
        return C_BLACK;
    r = (r * scale256) >> 8;
    g_c = (g_c * scale256) >> 8;
    b = (b * scale256) >> 8;
    if (r > 255)
        r = 255;
    if (g_c > 255)
        g_c = 255;
    if (b > 255)
        b = 255;
    return RGB(r, g_c, b);
}

#define JET_STORE_MAGIC      0x4A545A31u
#define JET_STORE_MAGIC_NOOV 0x4A545A4Eu /* same segments without plate overrides */
#define JET_STORE_VER        3u
#ifndef JET_SETTINGS_N
#define JET_SETTINGS_N SETTINGS_MAX
#endif
#define JET_HOLD_MS     400u
#define JET_LONG_MS     500u
#define JET_BPM_MIN     40u
#define JET_BPM_MAX     200u
#define JET_BPM_DEF     120u
#define JET_EV_Q        64u
#define JET_CLK_PPQN    24u
#define JET_AIR_OPEN    96u
#define KEY0            53u

enum {
    VIEW_HOME = 0,
    VIEW_SEL,
    VIEW_ENV,
    VIEW_GROOVE,
    VIEW_FX,
    VIEW_LFO,
    VIEW_SEQ,
    VIEW_COUNT
};

/* meta.page[] keeps 7 slots (ex-ARP index 6) so the store layout is unchanged. */
#define JET_DESK_N 7u
#define JET_BANK_N 6u

enum {
    JET_P_PSEUDO = JET_P_N,
    JET_P_BPM = JET_P_PSEUDO,
    JET_P_SWING,
    JET_P_ROOT,
    JET_P_SCALE,
    JET_P_CLOCK,
    JET_P_MIDI,
    JET_P_BEND,
    JET_P_OCT,
    JET_P_LFO_TGT,
    JET_P_LFO_RATE,
    JET_P_LFO_DEPTH,
    JET_P_LFO_SHAPE,
    JET_P_LFO_PHASE,
    JET_P_LFO_SKEW,
    JET_P_LFO_SMOOTH,
    JET_P_LFO_FOLD,
    JET_P_REC_MODE,
    JET_P_REC_QUANT,
    JET_P_REC_BARS,
    JET_P_REC_AUTO,
    JET_P_REC_UNDO,
    JET_P_REC_CLEAR,
    JET_P_SEQ_LEN,
    JET_P_SEQ_PAGE,
    JET_P_SEQ_ROT,
    JET_P_SEQ_CLR,
    JET_P_SEQ_VEL,
    JET_P_SEQ_GATE,
    JET_P_SEQ_MICRO,
    JET_P_SEQ_NOTE,
    JET_P_SEQ_PUSH,
    JET_P_SEQ_LPF,
    JET_P_SEQ_SLIDE,
    JET_P_SEQ_ACCENT,
    JET_P_PLAY,
    JET_P_NONE = 255u
};

enum {
    JET_LFO_PITCH = 0,
    JET_LFO_PUSH,
    JET_LFO_BLOW,
    JET_LFO_LPF,
    JET_LFO_HPF,
    JET_LFO_HARM,
    JET_LFO_REVMIX,
    JET_LFO_LEVEL,
    JET_LFO_SAT,
    JET_LFO_DLYLVL,
    JET_LFO_DLYFB,
    JET_LFO_DLYTONE,
    JET_LFO_REVSIZE,
    JET_LFO_WIDTH,
    JET_LFO_ISO,
    JET_LFO_WOW,
    JET_LFO_TGT_N
};

/* LFO target -> patch param (JET_P_NONE = special path, PITCH). Single source for knob mapping and LFO apply. */
static const uint8_t JET_LFO_PARAM[JET_LFO_TGT_N] = {
    JET_P_NONE, JET_P_PUSH, JET_P_BLOW, JET_P_LPF, JET_P_HPF, JET_P_HARM, JET_P_REVMIX,
    JET_P_LEVEL, JET_P_SAT, JET_P_DLYLVL, JET_P_DLYFB, JET_P_DLYTONE, JET_P_REVSIZE, JET_P_WIDTH,
    JET_P_ISO, JET_P_WOW
};

enum {
    JET_CLK_INT = 0,
    JET_CLK_EXT,
    JET_CLK_SEND,
    JET_CLK_N
};

enum {
    JET_EV_NOTE_ON = 0,
    JET_EV_NOTE_OFF,
    JET_EV_ALL_OFF,
    JET_EV_EXPR,
    JET_EV_CLK
};

typedef struct {
    uint8_t type;
    uint8_t note;
    uint8_t vel;
    uint8_t flags;
} jet_ev_t;

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t era;
    uint8_t plate;
    int8_t oct;
    uint8_t page[JET_DESK_N];
} jet_meta_t;

typedef struct {
    uint8_t bpm;
    uint8_t swing;
    uint8_t root;
    uint8_t scale;
    uint8_t clock;
    uint8_t midi_ch;
    uint8_t bend;
} jet_groove_store_t;

typedef struct {
    uint8_t used;
    uint8_t era;
    jet_patch_t patch;
    uint8_t bpm;
    uint8_t swing;
    int8_t root;
} jet_plate_ov_t;

typedef struct {
    jet_meta_t meta;
    jet_groove_store_t groove;
    jet_patch_t patch;
    jet_seq_t seq;
    ko_rec_cfg_t rec;
    uint8_t rec_auto_len;
    uint8_t rec_view_page;
    uint8_t lfo_tgt;
    fm_lfo_slot_t lfo[JET_LFO_TGT_N];
} jet_cfg_t;

typedef struct {
    jet_cfg_t cfg;
    jet_engine_t eng;
    jet_patch_t patch_eff;
    jet_expr_t expr;
    jet_expr_t expr_isr;
    uint8_t view;
    uint8_t playing;
    uint8_t rec_arm;
    uint8_t era;
    uint8_t plate;
    int8_t keyboard_oct;
    uint8_t air;
    uint8_t groove_pending;
    uint8_t seq_page;
    uint8_t seq_knob_page;
    uint8_t seq_follow;
    uint8_t seq_plock_latch;
    uint8_t seq_plock_hold;
    uint8_t seq_plock_bank;
    uint8_t seq_plock_step;
    uint8_t seq_plock_page_saved;
    uint8_t white_press_k;
    uint32_t white_press_ms;
    uint8_t white_long_used;
    uint8_t white_long_cancel;
    uint8_t seq_last_note;
    uint8_t live_note;
    uint8_t seq_last_on;
    uint8_t play_step;
    volatile uint32_t isr_step_seq;
    volatile uint8_t jet_isr_fired_step;
    uint32_t isr_step_seen;
    uint32_t tick_mod6;
    uint32_t clk_spp;
    uint32_t clk_acc;
    uint32_t clk_swing_extra;
    uint8_t clk_odd;
    uint32_t fire_left;
    uint8_t fire_pend;
    uint8_t fire_st;
    uint8_t plk_on; /* bit0 PUSH, bit1 LPF: step plock active (ISR-written) */
    int8_t plk_push;
    uint8_t plk_lpf;
    uint8_t key_note[27]; /* live note per key bit, 0 = none */
    uint32_t seq_clear_ms;
    uint32_t gate_left;
    uint8_t gate_note;
    uint8_t early_done;             /* ISR: the upcoming step already fired early (negative MICRO) */
    volatile uint8_t rec_skip_step; /* REC quantised a live note onto the coming step: do not double it once */
    uint8_t step_cur;               /* step-record cursor (REC armed, transport stopped) */
    uint8_t rec_fresh;              /* REPLACE: the first note of this take wipes the old pattern */
    uint8_t rec_in_on;              /* jet_rec_write called from a note-on (not a gate finalize) */
    uint8_t rec_vel;
    int8_t rec_micro;
    uint8_t aud_note;               /* SEQ edit audition, 0 = none */
    uint32_t aud_off_ms;
    int32_t out_peak;
    int16_t scope[240];
    uint8_t scope_ix;
    uint32_t prev_keys;
    uint32_t ui_dirty;
    uint32_t set_dirty;
    uint32_t set_t;
    uint32_t ui_ms;
    uint32_t coef_ms;
    uint32_t knob_ms[4];
    uint32_t rec_press_ms;
    uint8_t rec_long_used;
    uint32_t save_press_ms;
    uint8_t save_long_used;
    uint32_t seq_press_ms;
    uint8_t seq_long_used;
    uint8_t seq_was_in_seq;
    uint32_t fx_press_ms;
    uint8_t fx_long_used;
    uint32_t lfo_press_ms;
    uint8_t lfo_long_used;
    uint8_t rec_clear_arm;
    uint8_t seq_clear_arm;
    uint8_t ext_clock;
    uint8_t midi_clk_div;
    uint32_t ext_s16_mark;
    ko_rec_t rec;
    ko_rec_voice_t rec_voice;
    ko_rec_writer_t rec_writer;
    jet_seq_t rec_undo;
    uint8_t rec_undo_ready;
    ko_flash_t flash;
    ko_toast_t toast;
    fm_lfo_rt_t lfo_rt[JET_LFO_TGT_N];
    jet_patch_t patch_default;
} jet_app_t;

static jet_app_t g;
static uint8_t seq_len(void);
static void jet_seq_fire_step(uint8_t st);
static void jet_clock_tick(void);
static void jet_plock_latch_end(void);
static void jet_plock_latch_begin(uint8_t st);
static void jet_plock_latch_touch_break(uint32_t btn_edges, uint32_t note_rising);
static void jet_white_plock_latch_poll(uint32_t keys, uint32_t rising);
static void jet_handle_seq_white(uint32_t keys, uint32_t rising, uint32_t released);
static void jet_cc_knob_abs(uint8_t col, uint8_t val);
static int jet_param_get_ptr(uint8_t id, uint8_t **pu, int8_t **pi);
static uint8_t jet_page_param(uint8_t col);
static uint8_t jet_note_store(uint8_t note);
static uint8_t jet_note_sound(uint8_t stored);
static void jet_audition(uint8_t stored, uint8_t vel);
static uint8_t jet_rec_stepping(void);
static uint8_t jet_rec_live(void);
static uint8_t jet_ui_step(void);
static void note_fmt(char *s, uint8_t note);
static uint8_t jet_seq_focus_step(void);
static jet_plate_ov_t jet_plate_ov[JET_PLATE_N];
static uint8_t jet_store_buf[JET_SETTINGS_N] __attribute__((aligned(4)));
static jet_patch_t jet_store_patch_ref;
static jet_seq_t jet_store_seq_ref;
static fm_lfo_slot_t jet_store_lfo_ref[JET_LFO_TGT_N];
static ko_rec_cfg_t jet_store_rec_ref;

static volatile uint8_t jet_ev_w;
static volatile uint8_t jet_ev_r;
static jet_ev_t jet_ev_q[JET_EV_Q];
static volatile uint8_t jet_midi_clk_q;

static const uint8_t KEY_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t KEY_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
static const char *const NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const uint8_t JET_BANK_BTN[JET_BANK_N] = {
    BTN_HOME, BTN_SEL, BTN_ENV, BTN_GLO, BTN_FX, BTN_LFO
};
static const uint8_t JET_BANK_PAGES[JET_DESK_N] = {4u, 3u, 2u, 2u, 3u, 3u, 1u};
static const char *const JET_STAGE[JET_BANK_N][4] = {
    {"KO JET", "PIPE", "SHAPE", "MACRO"},
    {"VOICE", "VOICE", "MIX", "MIX"},
    {"ENV", "ENV", "ENV", "ENV"},
    {"GROOVE", "SETUP", "SETUP", "SETUP"},
    {"ECHO", "SPACE", "REVERB", "REVERB"},
    {"LFO", "LFO", "EXPR", "EXPR"}
};
static const char *const JET_LFO_NAME[JET_LFO_TGT_N] = {
    "PITCH", "PUSH", "BLOW", "LPF", "HPF", "HARM", "REVMIX",
    "LEVEL", "SAT", "DLY", "FB", "TONE", "TAIL", "WIDTH", "ISO", "WOW"
};
static const char *const JET_CLOCK_NAME[JET_CLK_N] = {"INT", "EXT", "SEND"};
static const char *const REC_MODE_LBL[3] = {"OVERDUB", "REPLACE", "MONITOR"};
static const char *const REC_QUANT_LBL[3] = {"FREE", "SLOP", "16TH"};
static const char *const REC_BARS_LBL[4] = {"1 BAR", "2 BARS", "4 BARS", "8 BARS"};

static const uint8_t JET_PAGE[JET_DESK_N][4][4] = {
    {{JET_P_BLOW, JET_P_PUSH, JET_P_LPF, JET_P_HARM},
     {JET_P_HPF, JET_P_FBDEC, JET_P_FBREL, JET_P_SAT},
     {JET_P_SHAPE, JET_P_SYM, JET_P_SPLIT, JET_P_POLAR},
     {JET_P_MACBRTH, JET_P_MACGRIT, JET_P_MACSPC, JET_P_NONE}},
    {{JET_P_MODE, JET_P_GLIDE, JET_P_DETUNE, JET_P_TUNE},
     {JET_P_FINE, JET_P_TCOMP, JET_P_DCN, JET_P_NZLPF},
     {JET_P_LEVEL, JET_P_WIDTH, JET_P_REVSPIN, JET_P_REVLO}},
    {{JET_P_SATK, JET_P_SDEC, JET_P_SSUS, JET_P_SREL},
     {JET_P_SDLY, JET_P_SHOLD, JET_P_SCURVE, JET_P_VEL},
     {JET_P_NONE, JET_P_NONE, JET_P_NONE, JET_P_NONE}},
    {{JET_P_ROOT, JET_P_SCALE, JET_P_SWING, JET_P_BPM},
     {JET_P_MIDI, JET_P_CLOCK, JET_P_BEND, JET_P_OCT},
     {JET_P_NONE, JET_P_NONE, JET_P_NONE, JET_P_NONE}},
    {{JET_P_DLYLVL, JET_P_DLYTIME, JET_P_DLYFB, JET_P_DLYTONE},
     {JET_P_WOW, JET_P_TAP, JET_P_TANK, JET_P_ISO},
     {JET_P_REVMIX, JET_P_REVHI, JET_P_REVSIZE, JET_P_DLYREV}},
    {{JET_P_LFO_TGT, JET_P_LFO_RATE, JET_P_LFO_DEPTH, JET_P_LFO_SHAPE},
     {JET_P_LFO_PHASE, JET_P_LFO_SKEW, JET_P_LFO_SMOOTH, JET_P_LFO_FOLD},
     {JET_P_XBRBLOW, JET_P_XMWLPF, JET_P_XMWVIB, JET_P_XATPUSH}},
    {{JET_P_NONE, JET_P_NONE, JET_P_NONE, JET_P_NONE},
     {JET_P_NONE, JET_P_NONE, JET_P_NONE, JET_P_NONE},
     {JET_P_NONE, JET_P_NONE, JET_P_NONE, JET_P_NONE}}
};

static int32_t jet_wave_sine(uint32_t ph)
{
    return fm_lfo_sine_q15(ph);
}

static int32_t jet_wave_sine_q64(uint32_t ph)
{
    return jet_wave_sine(ph);
}

#define FM_GROOVE_PLOT_SINE jet_wave_sine_q64
#include "fm_groove_plot.h"

static int32_t bass_env_playhead_x(const bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, const int32_t *px)
{
    uint8_t st = rt->stage;
    int32_t seg;
    uint32_t total;
    uint32_t done;

    if (st == BASS_ENV_IDLE)
        return -1;
    seg = (int32_t)st;
    if (seg < 0 || seg > 5)
        return -1;
    if (st == BASS_ENV_SUSTAIN)
        return px[4] + (px[5] - px[4]) / 2;
    total = bass_dahdsr_stage_samples(st, cfg);
    if (total < 1u)
        total = 1u;
    done = rt->stage_left < total ? total - rt->stage_left : 0u;
    while (total > 0x7FFFFFu) {
        total >>= 1;
        done >>= 1;
    }
    return px[seg] + (int32_t)(((uint32_t)(px[seg + 1] - px[seg]) * done) / total);
}


static void str_copy(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static void u8_dec(char *s, uint32_t v)
{
    if (v >= 100u) {
        s[0] = (char)('0' + v / 100u);
        s[1] = (char)('0' + (v / 10u) % 10u);
        s[2] = (char)('0' + v % 10u);
        s[3] = 0;
    } else if (v >= 10u) {
        s[0] = (char)('0' + v / 10u);
        s[1] = (char)('0' + v % 10u);
        s[2] = 0;
    } else {
        s[0] = (char)('0' + v);
        s[1] = 0;
    }
}

static void oct_fmt(char *s, int32_t oct)
{
    if (oct > 0)
        *s++ = '+';
    else if (oct < 0) {
        *s++ = '-';
        oct = -oct;
    }
    u8_dec(s, (uint32_t)oct);
}

static void adj_u8(uint8_t *p, int32_t d, uint8_t lo, uint8_t hi)
{
    int32_t v = (int32_t)*p + d;

    if (v < (int32_t)lo)
        v = (int32_t)lo;
    if (v > (int32_t)hi)
        v = (int32_t)hi;
    *p = (uint8_t)v;
}

static void adj_i8(int8_t *p, int32_t d, int8_t lo, int8_t hi)
{
    int32_t v = (int32_t)*p + d;

    if (v < (int32_t)lo)
        v = (int32_t)lo;
    if (v > (int32_t)hi)
        v = (int32_t)hi;
    *p = (int8_t)v;
}

static void mark_dirty(void)
{
    g.ui_dirty = 1u;
    g.set_dirty = 1u;
    g.set_t = fm1_ms;
}

static void jet_toast(const char *msg, uint8_t kind)
{
    g.toast.msg = msg;
    g.toast.kind = kind;
    g.toast.end_ms = fm1_ms + 900u;
    g.toast.by = ko_toast_by_center(20);
    g.ui_dirty = 1u;
}

static uint8_t jet_desk_view(void)
{
    if (g.view < JET_BANK_N)
        return g.view;
    return VIEW_HOME;
}

static uint8_t jet_bank_page(void)
{
    uint8_t dv = jet_desk_view();

    return (uint8_t)(g.cfg.meta.page[dv] % JET_BANK_PAGES[dv]);
}

static void jet_flash_param(const char *lbl)
{
    ko_flash_set(&g.flash, lbl, fm1_ms, 500u);
}

/* "LBL VAL" when it fits the 7-char flash, else value only. */
static void jet_flash_kv(const char *lbl, const char *val)
{
    char m[8];
    uint32_t a = 0u;
    uint32_t b = 0u;
    uint32_t i;

    while (lbl[a])
        a++;
    while (val[b])
        b++;
    if (a + 1u + b > 7u) {
        jet_flash_param(val);
        return;
    }
    for (i = 0u; i < a; i++)
        m[i] = lbl[i];
    m[a] = ' ';
    for (i = 0u; i <= b; i++)
        m[a + 1u + i] = val[i];
    jet_flash_param(m);
}

static int jet_ev_push(jet_ev_t e)
{
    uint8_t n = (uint8_t)((jet_ev_w + 1u) & (JET_EV_Q - 1u));

    if (n == jet_ev_r)
        return 0;
    jet_ev_q[jet_ev_w] = e;
    jet_ev_w = n;
    return 1;
}

static int jet_ev_pop_isr(jet_ev_t *e)
{
    if (jet_ev_r == jet_ev_w)
        return 0;
    *e = jet_ev_q[jet_ev_r];
    jet_ev_r = (uint8_t)((jet_ev_r + 1u) & (JET_EV_Q - 1u));
    return 1;
}

/* Retarget the voices still on `from` to `to` without a new attack (glide per patch GLIDE), any mode. */
static uint8_t jet_slide_isr(uint8_t from, uint8_t to, uint8_t vel)
{
    uint8_t i;
    uint8_t hit = 0u;

    for (i = 0u; i < JET_NVOICE; i++) {
        jet_voice_t *v = &g.eng.vo[i];

        if (v->active && v->note == from && v->env.stage != BASS_ENV_IDLE) {
            jet_voice_trigger(&g.eng, v, to, vel, 1u);
            hit = 1u;
        }
    }
    return hit;
}

static void jet_ev_dispatch_isr(jet_ev_t ev)
{
    switch (ev.type) {
    case JET_EV_NOTE_ON:
        /* slide (seq only): g.gate_note is still the previous step's note here */
        if (!(ev.flags & 2u) || !jet_slide_isr(g.gate_note, ev.note, ev.vel))
            jet_engine_note_on(&g.eng, ev.note, ev.vel, ev.flags & 1u);
        break;
    case JET_EV_NOTE_OFF:
        jet_engine_note_off(&g.eng, ev.note);
        break;
    case JET_EV_ALL_OFF:
        jet_engine_all_off(&g.eng, ev.flags & 1u);
        break;
    case JET_EV_EXPR:
        jet_engine_expr(&g.eng, &g.expr_isr);
        break;
    case JET_EV_CLK:
        if (g.playing && g.cfg.groove.clock == JET_CLK_EXT)
            jet_clock_tick();
        break;
    default:
        break;
    }
}

static void jet_ev_drain_isr(void)
{
    jet_ev_t ev;

    while (jet_ev_pop_isr(&ev))
        jet_ev_dispatch_isr(ev);
}

static uint8_t jet_midi_ch_ok(uint8_t st)
{
    uint8_t ch = (uint8_t)((st & 0x0Fu) + 1u);
    uint8_t cfg = g.cfg.groove.midi_ch;

    if (cfg == 0u)
        return 1u;
    if (cfg < 1u)
        cfg = 1u;
    if (cfg > 16u)
        cfg = 16u;
    return ch == cfg ? 1u : 0u;
}

static uint8_t seq_len(void)
{
    return jet_seq_len_safe(&g.cfg.seq);
}

static void transport_set_spp(void)
{
    uint32_t bpm = g.cfg.groove.bpm;

    if (bpm < JET_BPM_MIN)
        bpm = JET_BPM_MIN;
    if (bpm > JET_BPM_MAX)
        bpm = JET_BPM_MAX;
    g.clk_spp = (JET_FS * 60u) / (bpm * JET_CLK_PPQN);
    if (g.clk_spp < 32u)
        g.clk_spp = 32u;
    {
        uint32_t sw = g.cfg.groove.swing;

        if (sw < 50u)
            sw = 50u;
        if (sw > 75u)
            sw = 75u;
        /* off-beat 16th delay, BASS swing_samples(): step * (sw - 50) / 50, max half a step */
        g.clk_swing_extra = (g.clk_spp * 6u * (sw - 50u)) / 50u;
        if (g.clk_swing_extra > g.clk_spp * 3u)
            g.clk_swing_extra = g.clk_spp * 3u;
    }
}

static void transport_reset(void)
{
    g.play_step = 0u;
    g.tick_mod6 = 5u;
    g.clk_acc = g.clk_spp;
    g.clk_odd = 0u;
    g.fire_pend = 0u;
    g.fire_left = 0u;
    g.gate_left = 0u;
    g.gate_note = 0u;
    g.seq_last_on = 0u;
    g.plk_on = 0u;
    g.early_done = 0u;
    g.rec_skip_step = 255u;
    g.midi_clk_div = 0u;
    g.jet_isr_fired_step = (uint8_t)(seq_len() - 1u);
}

static int jet_is_quiet(void)
{
    uint8_t i;

    if (g.playing)
        return 0;
    for (i = 0u; i < JET_NVOICE; i++) {
        jet_voice_t *v = &g.eng.vo[i];
        if (v->active || v->env.stage != BASS_ENV_IDLE)
            return 0;
    }
    return 1;
}

/* Current plate voice: factory era+plate, or the saved plate override. */
static void jet_plate_patch(jet_patch_t *p)
{
    const jet_plate_ov_t *ov = &jet_plate_ov[g.plate];

    if (ov->used)
        *p = ov->patch;
    else
        jet_plate_apply(p, g.era, (uint8_t)(g.plate % JET_PLATES_PER_ERA));
}

/* Plate groove (bpm/swing/root), then the plate override on top. */
static void jet_plate_groove_load(void)
{
    const jet_plate_ov_t *ov = &jet_plate_ov[g.plate];
    int8_t root = 0;

    jet_plate_groove(g.era, (uint8_t)(g.plate % JET_PLATES_PER_ERA), &g.cfg.groove.bpm, &g.cfg.groove.swing, &root);
    if (ov->used) {
        if (ov->bpm)
            g.cfg.groove.bpm = ov->bpm;
        if (ov->swing)
            g.cfg.groove.swing = jet_clamp_swing_u8(ov->swing);
        root = ov->root;
    }
    g.cfg.groove.root = (uint8_t)(((int32_t)root % 12 + 12) % 12);
}

static void jet_apply_plate_voice(void)
{
    ko_groove_t held;
    ko_groove_ref_t live;
    jet_patch_t p;

    live.bpm = &g.cfg.groove.bpm;
    live.swing = &g.cfg.groove.swing;
    live.root = &g.cfg.groove.root;
    live.scale = &g.cfg.groove.scale;
    ko_groove_take(&held, &live);
    jet_plate_patch(&p);
    g.cfg.patch = p;
    g.eng.patch = p;
    jet_plate_groove_load();
    ko_turn_groove(&live, &held, KO_TURN_PLATE, g.playing, &g.groove_pending);
    transport_set_spp();
    mark_dirty();
}

static void jet_set_era(int32_t delta)
{
    ko_groove_t held;
    ko_groove_ref_t live;
    int32_t e = (int32_t)g.era + delta;

    while (e < 0)
        e += JET_ERA_N;
    g.era = (uint8_t)((uint32_t)e % JET_ERA_N);
    g.plate = jet_plate_in_era(g.plate, g.era);
    live.bpm = &g.cfg.groove.bpm;
    live.swing = &g.cfg.groove.swing;
    live.root = &g.cfg.groove.root;
    live.scale = &g.cfg.groove.scale;
    ko_groove_take(&held, &live);
    jet_apply_plate_voice();
    ko_turn_groove(&live, &held, KO_TURN_ERA, g.playing, &g.groove_pending);
}

static void jet_set_plate(int32_t delta, uint8_t jump10)
{
    ko_groove_t held;
    ko_groove_ref_t live;
    int32_t p;

    if (jump10) {
        /* EDIT+PRESETS: same slot in the next/previous era */
        p = (int32_t)g.plate + delta * (int32_t)JET_PLATES_PER_ERA;
        while (p < 0)
            p += JET_PLATE_N;
        g.plate = (uint8_t)((uint32_t)p % JET_PLATE_N);
        g.era = jet_era_of_plate(g.plate);
    } else {
        /* plain plate turn keeps the era (ko_era_plate.h): wrap inside the era's slots */
        p = (int32_t)jet_plate_slot(g.plate) + delta;
        while (p < 0)
            p += JET_PLATES_PER_ERA;
        g.plate = jet_plate_in_era((uint8_t)((uint32_t)p % JET_PLATES_PER_ERA), g.era);
    }
    live.bpm = &g.cfg.groove.bpm;
    live.swing = &g.cfg.groove.swing;
    live.root = &g.cfg.groove.root;
    live.scale = &g.cfg.groove.scale;
    ko_groove_take(&held, &live);
    jet_apply_plate_voice();
    ko_turn_groove(&live, &held, KO_TURN_PLATE, g.playing, &g.groove_pending);
}

/* Add a delta (patch units) to one param on the effective patch, clamped to the param range. */
static void jet_mod_add(jet_patch_t *d, uint8_t id, int32_t dl)
{
    const jet_param_t *pr = &JET_PARAM[id];
    int32_t v = jet_param_get(d, id) + dl;

    if (v < pr->lo)
        v = pr->lo;
    if (v > pr->hi)
        v = pr->hi;
    jet_param_set(d, id, v);
}

/* HOME macros: each macro (0..127) adds amt * m / 127 to its routed params. */
typedef struct {
    uint8_t mac;
    uint8_t id;
    int8_t amt;
} jet_mac_route_t;

static const jet_mac_route_t JET_MAC_ROUTES[] = {
    {JET_P_MACBRTH, JET_P_BLOW, 40},    {JET_P_MACBRTH, JET_P_LPF, 30},     {JET_P_MACBRTH, JET_P_PUSH, 12},
    {JET_P_MACGRIT, JET_P_SAT, 60},     {JET_P_MACGRIT, JET_P_HARM, 30},    {JET_P_MACGRIT, JET_P_SHAPE, 40},
    {JET_P_MACSPC, JET_P_DLYLVL, 40},   {JET_P_MACSPC, JET_P_REVMIX, 50},   {JET_P_MACSPC, JET_P_REVSIZE, 30},
    {JET_P_MACSPC, JET_P_WIDTH, 30},
};

static void jet_patch_eff_rebuild(void)
{
    jet_patch_t *d = &g.patch_eff;
    const jet_patch_t *s = &g.cfg.patch;
    uint32_t t;
    uint32_t i;

    *d = *s;
    for (t = 0u; t < JET_LFO_TGT_N; t++) {
        int32_t dl;

        if (JET_LFO_PARAM[t] == JET_P_NONE)
            continue;
        dl = fm_lfo_slot_delta(g.cfg.lfo, g.lfo_rt, (uint8_t)t);
        if (dl)
            jet_mod_add(d, JET_LFO_PARAM[t], dl);
    }
    for (i = 0u; i < sizeof(JET_MAC_ROUTES) / sizeof(JET_MAC_ROUTES[0]); i++) {
        const jet_mac_route_t *r = &JET_MAC_ROUTES[i];
        int32_t m = jet_param_get(d, r->mac);
        int32_t dl = ((int32_t)r->amt * m) / 127;

        if (dl)
            jet_mod_add(d, r->id, dl);
    }
    {
        int32_t blow = (int32_t)d->blow + ((int32_t)g.air * 40 >> 7);

        if (blow > 127)
            blow = 127;
        d->blow = (uint8_t)blow;
    }
    if (g.air > JET_AIR_OPEN)
        d->lpf = fm_lfo_clamp_u8((int32_t)d->lpf + (int32_t)(g.air - JET_AIR_OPEN), 0u, 127u);
    /* EXPR MW>LPF: mod wheel opens (or closes) the loop LPF, full wheel at full depth = the whole range */
    if (g.expr.mw && d->x_mw_lpf != 64u)
        jet_mod_add(d, JET_P_LPF, ((int32_t)g.expr.mw * ((int32_t)d->x_mw_lpf - 64)) / 32);
    {
        uint8_t on = g.plk_on;

        if (on & 1u)
            jet_param_set(d, JET_P_PUSH, g.plk_push);
        if (on & 2u)
            jet_param_set(d, JET_P_LPF, g.plk_lpf);
    }
    if (g.eng.mode != d->mode) {
        g.eng.mode = d->mode;
        (void)jet_ev_push((jet_ev_t){JET_EV_ALL_OFF, 0u, 0u, 0u});
    }
    g.eng.glide_coef = d->glide;
    g.eng.detune_spread = d->detune;
    /* depth 127 = +/-1 octave (BASS transpose LFO range) */
    g.expr.lfo_pitch = (int16_t)((fm_lfo_slot_delta(g.cfg.lfo, g.lfo_rt, JET_LFO_PITCH) * 1200) / 127);
    /* EXPR MW>VIB: mod wheel vibrato, 5.5 Hz, +-1 semitone at full wheel and full depth */
    if (g.expr.mw && d->x_mw_vib != 64u) {
        int32_t dep = (int32_t)g.expr.mw * ((int32_t)d->x_mw_vib - 64); /* +-8001 */
        int32_t vib = ((fm_lfo_sine_q15(fm1_ms * 23622320u) * dep) >> 15) / 80;

        g.expr.lfo_pitch = (int16_t)(g.expr.lfo_pitch + vib);
    }
    g.eng.patch = *d;
}

static void jet_coef_publish(void)
{
    jet_coef_t c;

    jet_coef_build(&c, &g.patch_eff, g.cfg.groove.bpm);
    jet_engine_publish(&g.eng, &c);
}

static void jet_audio_coef_service(void)
{
    if (fm1_ms - g.coef_ms < 4u)
        return;
    g.coef_ms = fm1_ms;
    fm_lfo_slots_sanitize_rates(g.cfg.lfo, JET_LFO_TGT_N, g.cfg.groove.bpm);
    jet_patch_eff_rebuild();
    jet_coef_publish();
}

static void jet_midi_clk_drain(void)
{
    while (jet_midi_clk_q) {
        jet_midi_clk_q--;
        midi_send(0xF8, 0, 0);
    }
}

static void jet_seq_page_clamp(void)
{
    uint8_t pages = jet_seq_pages(&g.cfg.seq);

    if (g.seq_page >= pages)
        g.seq_page = (uint8_t)(pages - 1u);
}

static void jet_rec_snapshot(void *ctx)
{
    (void)ctx;
    jet_seq_copy(&g.rec_undo, &g.cfg.seq);
    g.rec_undo_ready = 1u;
}

static void jet_rec_restore(void *ctx)
{
    (void)ctx;
    jet_seq_copy(&g.cfg.seq, &g.rec_undo);
    jet_seq_page_clamp();
    mark_dirty();
}

static void jet_rec_set_len(void *ctx, uint8_t steps)
{
    (void)ctx;
    jet_seq_set_len(&g.cfg.seq, steps);
    jet_seq_page_clamp();
    mark_dirty();
}

static void jet_rec_erase_step(void *ctx, uint8_t st)
{
    (void)ctx;
    jet_seq_clear_step(&g.cfg.seq, st);
    mark_dirty();
}

static void jet_rec_clear(void *ctx, uint8_t st)
{
    (void)ctx;
    jet_seq_clear_step(&g.cfg.seq, st);
    mark_dirty();
}

static void jet_rec_write(void *ctx, uint8_t st, uint8_t note, uint8_t gate, int8_t micro, uint8_t legato)
{
    uint8_t ix;
    uint8_t ln;

    (void)ctx;
    /* ko_rec gate scale 40/55/75/100 -> JET 1..8 eighths of a step */
    if (gate > 8u) {
        gate = (uint8_t)(((uint32_t)gate * 8u + 50u) / 100u);
        if (gate > 8u)
            gate = 8u;
    }
    if (gate < 1u)
        gate = 1u;
    (void)micro; /* ko_rec micro is in another unit: JET passes its own tick offset in g.rec_micro */
    ln = seq_len();
    ix = (uint8_t)(st % ln);
    if (!legato && !g.rec_in_on) {
        /* note-off: only the gate of the attack step, and only if that step still holds this note */
        if (note && g.cfg.seq.note[ix] == note && !(g.cfg.seq.legato[ix] & 1u)) {
            g.cfg.seq.gate[ix] = gate;
            mark_dirty();
        }
        return;
    }
    jet_seq_write_note(&g.cfg.seq, st, note, gate, g.rec_vel ? g.rec_vel : 100u, legato ? 0 : g.rec_micro, legato);
    mark_dirty();
}

static void jet_plock_latch_end(void)
{
    g.seq_plock_latch = 0u;
    g.seq_plock_hold = 0u;
    g.seq_plock_step = FM_SEQ_PLOCK_NONE;
    g.seq_plock_bank = 0u;
    g.seq_page = g.seq_plock_page_saved;
    g.white_press_k = 255u;
    g.white_press_ms = 0u;
    g.white_long_used = 0u;
    g.white_long_cancel = 0u;
    mark_dirty();
}

static void jet_plock_latch_begin(uint8_t st)
{
    g.seq_plock_page_saved = g.seq_page;
    g.seq_plock_latch = 1u;
    g.seq_plock_hold = 1u;
    g.seq_plock_step = st;
    g.seq_plock_bank = 0u;
    mark_dirty();
}

static void jet_plock_latch_touch_break(uint32_t btn_edges, uint32_t note_rising)
{
    if (!g.seq_plock_latch)
        return;
    if (btn_edges) {
        if (btn_edges == (1u << BTN_SEQ))
            return;
        jet_plock_latch_end();
        return;
    }
    if (note_rising) {
        uint32_t base = (uint32_t)g.seq_page * FM_SEQ_PAGE_STEPS;
        uint32_t k;

        for (k = 0u; k < 16u; k++) {
            if (!(note_rising & (1u << KEY_WHITE[k])))
                continue;
            if ((uint8_t)(base + k) != g.seq_plock_step)
                jet_plock_latch_end();
            break;
        }
    }
}

static uint8_t jet_seq_plock_ix(void)
{
    uint8_t ln = seq_len();

    if (!g.seq_plock_latch || g.seq_plock_step >= ln)
        return 255u;
    return (uint8_t)(g.seq_plock_step % ln);
}

static void jet_seq_rotate(int32_t dir)
{
    jet_seq_t *s = &g.cfg.seq;
    uint8_t ln = seq_len();
    jet_seq_t tmp;
    uint32_t i;
    int32_t sh;

    if (!dir || ln < 2u)
        return;
    sh = dir > 0 ? 1 : -1;
    tmp = *s;
    for (i = 0u; i < ln; i++) {
        uint32_t dst = (uint32_t)(((int32_t)i + sh + (int32_t)ln) % (int32_t)ln);
        s->note[dst] = tmp.note[i];
        s->vel[dst] = tmp.vel[i];
        s->gate[dst] = tmp.gate[i];
        s->micro[dst] = tmp.micro[i];
        s->flags[dst] = tmp.flags[i];
        s->legato[dst] = tmp.legato[i];
        s->pl_push[dst] = tmp.pl_push[i];
        s->pl_lpf[dst] = tmp.pl_lpf[i];
    }
    mark_dirty();
}

static uint8_t jet_map_cc_u8(uint8_t val, int32_t lo, int32_t hi)
{
    if (hi <= lo)
        return (uint8_t)lo;
    return (uint8_t)(lo + ((int32_t)val * (hi - lo) + 63) / 127);
}

static int8_t jet_map_cc_i8(uint8_t val, int32_t lo, int32_t hi)
{
    return (int8_t)jet_map_cc_u8(val, lo, hi);
}

/* PUSH plock -1 is 0xFF == FM_SEQ_PLOCK_NONE: skip it in the turn direction */
static uint8_t jet_pl_push_enc(int32_t v, int32_t dir)
{
    if (v == -1)
        v = dir < 0 ? -2 : 0;
    return (uint8_t)(int8_t)v;
}

static void jet_seq_adj(uint8_t id, int32_t steps, uint8_t edit)
{
    jet_seq_t *s = &g.cfg.seq;
    uint8_t ix = jet_seq_plock_ix();
    uint8_t ln = seq_len();

    if (id == JET_P_SEQ_LEN) {
        int32_t v = (int32_t)ln + steps;

        if (v < 1)
            v = 1;
        if (v > FM_SEQ_STEPS)
            v = FM_SEQ_STEPS;
        jet_seq_set_len(s, (uint8_t)v);
        jet_seq_page_clamp();
        mark_dirty();
        return;
    }
    if (id == JET_P_SEQ_PAGE) {
        uint8_t pages = jet_seq_pages(s);
        int32_t p = (int32_t)g.seq_page + steps;

        while (p < 0)
            p += pages;
        g.seq_page = (uint8_t)((uint32_t)p % pages);
        mark_dirty();
        return;
    }
    if (id == JET_P_SEQ_ROT) {
        jet_seq_rotate(steps);
        return;
    }
    if (ix >= ln)
        return;
    if (id == JET_P_SEQ_VEL) {
        uint8_t v = s->vel[ix] ? s->vel[ix] : 100u;

        if (edit)
            v = steps < 0 ? 1u : 127u;
        else
            adj_u8(&v, steps, 1, 127);
        s->vel[ix] = v;
    } else if (id == JET_P_SEQ_GATE) {
        uint8_t gt = s->gate[ix];

        if (edit)
            gt = steps < 0 ? 0u : 8u;
        else
            adj_u8(&gt, steps, 0, 8);
        s->gate[ix] = gt;
    } else if (id == JET_P_SEQ_MICRO) {
        int32_t v = (int32_t)s->micro[ix] + steps;

        if (edit)
            v = steps < 0 ? -JET_SEQ_MICRO_MAX : JET_SEQ_MICRO_MAX;
        s->micro[ix] = jet_seq_micro_clamp(v);
    } else if (id == JET_P_SEQ_NOTE) {
        uint8_t n = s->note[ix] ? s->note[ix] : g.seq_last_note;

        if (!n)
            n = 60u;
        /* EDIT + turn: an octave per detent */
        adj_u8(&n, edit ? (steps < 0 ? -12 : 12) : steps, 1, 127);
        s->note[ix] = n;
        s->flags[ix] = (uint8_t)(s->flags[ix] | JET_SEQ_F_ON);
        jet_audition(n, s->vel[ix]);
    } else if (id == JET_P_SEQ_PUSH) {
        int32_t v = s->pl_push[ix] == FM_SEQ_PLOCK_NONE ? (int32_t)g.cfg.patch.push : (int32_t)(int8_t)s->pl_push[ix];

        if (edit)
            v = steps < 0 ? -64 : 63;
        else
            v += steps;
        if (v < -64)
            v = -64;
        if (v > 63)
            v = 63;
        s->pl_push[ix] = jet_pl_push_enc(v, steps);
    } else if (id == JET_P_SEQ_LPF) {
        int32_t v = s->pl_lpf[ix] == FM_SEQ_PLOCK_NONE ? (int32_t)g.cfg.patch.lpf : (int32_t)s->pl_lpf[ix];

        if (edit)
            v = steps < 0 ? 0 : 127;
        else
            v += steps;
        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
        s->pl_lpf[ix] = (uint8_t)v;
    } else if (id == JET_P_SEQ_SLIDE) {
        if (edit)
            s->flags[ix] = steps < 0 ? (uint8_t)(s->flags[ix] & (uint8_t)~JET_SEQ_F_SLIDE)
                                     : (uint8_t)(s->flags[ix] | JET_SEQ_F_SLIDE);
        else if (steps != 0)
            s->flags[ix] ^= JET_SEQ_F_SLIDE;
    } else if (id == JET_P_SEQ_ACCENT) {
        if (edit)
            s->flags[ix] = steps < 0 ? (uint8_t)(s->flags[ix] & (uint8_t)~JET_SEQ_F_ACCENT)
                                     : (uint8_t)(s->flags[ix] | JET_SEQ_F_ACCENT);
        else if (steps != 0)
            s->flags[ix] ^= JET_SEQ_F_ACCENT;
    } else
        return;
    mark_dirty();
}

static void jet_seq_set_abs(uint8_t id, uint8_t val)
{
    jet_seq_t *s = &g.cfg.seq;
    uint8_t ix = jet_seq_plock_ix();
    uint8_t ln = seq_len();

    if (id == JET_P_SEQ_LEN) {
        jet_seq_set_len(s, jet_map_cc_u8(val, 1, FM_SEQ_STEPS));
        jet_seq_page_clamp();
        mark_dirty();
        return;
    }
    if (id == JET_P_SEQ_PAGE) {
        uint8_t pages = jet_seq_pages(s);

        if (pages)
            g.seq_page = (uint8_t)(jet_map_cc_u8(val, 0, (int32_t)pages - 1));
        mark_dirty();
        return;
    }
    if (id == JET_P_SEQ_ROT)
        return;
    if (ix >= ln)
        return;
    if (id == JET_P_SEQ_VEL)
        s->vel[ix] = jet_map_cc_u8(val, 1, 127);
    else if (id == JET_P_SEQ_GATE)
        s->gate[ix] = jet_map_cc_u8(val, 0, 8);
    else if (id == JET_P_SEQ_MICRO)
        s->micro[ix] = jet_seq_micro_clamp((int32_t)val * (2 * JET_SEQ_MICRO_MAX) / 127 - JET_SEQ_MICRO_MAX);
    else if (id == JET_P_SEQ_NOTE) {
        s->note[ix] = jet_map_cc_u8(val, 1, 127);
        s->flags[ix] = (uint8_t)(s->flags[ix] | JET_SEQ_F_ON);
    } else if (id == JET_P_SEQ_PUSH)
        s->pl_push[ix] = jet_pl_push_enc(jet_map_cc_i8(val, -64, 63), 1);
    else if (id == JET_P_SEQ_LPF)
        s->pl_lpf[ix] = jet_map_cc_u8(val, 0, 127);
    else if (id == JET_P_SEQ_SLIDE)
        s->flags[ix] = val >= 64u ? (uint8_t)(s->flags[ix] | JET_SEQ_F_SLIDE)
                                  : (uint8_t)(s->flags[ix] & (uint8_t)~JET_SEQ_F_SLIDE);
    else if (id == JET_P_SEQ_ACCENT)
        s->flags[ix] = val >= 64u ? (uint8_t)(s->flags[ix] | JET_SEQ_F_ACCENT)
                                  : (uint8_t)(s->flags[ix] & (uint8_t)~JET_SEQ_F_ACCENT);
    else
        return;
    mark_dirty();
}

static void jet_param_set_abs(uint8_t id, uint8_t val)
{
    uint8_t *pu = 0;
    int8_t *pi = 0;

    if (id == JET_P_NONE)
        return;
    if (id >= JET_P_SEQ_LEN && id <= JET_P_SEQ_ACCENT) {
        jet_seq_set_abs(id, val);
        return;
    }
    if (!jet_param_get_ptr(id, &pu, &pi))
        return;
    if (id < JET_P_N) {
        const jet_param_t *pr = &JET_PARAM[id];

        if (pi)
            *pi = jet_map_cc_i8(val, pr->lo, pr->hi);
        else
            *pu = jet_map_cc_u8(val, pr->lo, pr->hi);
    } else if (pu) {
        if (id == JET_P_BPM)
            *pu = jet_map_cc_u8(val, JET_BPM_MIN, JET_BPM_MAX);
        else if (id == JET_P_SWING)
            *pu = jet_map_cc_u8(val, 50, 75);
        else if (id == JET_P_ROOT)
            *pu = jet_map_cc_u8(val, 0, 11);
        else if (id == JET_P_SCALE)
            *pu = jet_map_cc_u8(val, 0, (int32_t)JET_SCALE_N - 1);
        else if (id == JET_P_CLOCK)
            *pu = jet_map_cc_u8(val, 0, (int32_t)JET_CLK_N - 1);
        else if (id == JET_P_MIDI)
            *pu = jet_map_cc_u8(val, 0, 16);
        else if (id == JET_P_BEND)
            *pu = jet_map_cc_u8(val, 0, 12);
        else if (id == JET_P_LFO_TGT)
            *pu = jet_map_cc_u8(val, 0, (int32_t)JET_LFO_TGT_N - 1);
        else if (id == JET_P_REC_MODE || id == JET_P_REC_QUANT)
            *pu = jet_map_cc_u8(val, 0, 2);
        else if (id == JET_P_REC_BARS)
            *pu = jet_map_cc_u8(val, 0, 3);
        else if (id == JET_P_REC_AUTO)
            *pu = val >= 64u ? 1u : 0u;
        else
            *pu = jet_map_cc_u8(val, 0, 127);
    }
    if (id == JET_P_BPM || id == JET_P_SWING)
        transport_set_spp();
    mark_dirty();
}

static void jet_cc_knob_abs(uint8_t col, uint8_t val)
{
    if (col >= 4u)
        return;
    g.knob_ms[col] = 0u;
    jet_param_set_abs(jet_page_param(col), val);
}


static uint8_t white_to_midi(uint8_t wk)
{
    int32_t oct = (int32_t)g.keyboard_oct * 12;
    int32_t nn = (int32_t)KEY0 + (int32_t)KEY_WHITE[wk & 15u] + oct;

    if (nn < 0)
        nn = 0;
    else if (nn > 127)
        nn = 127;
    return (uint8_t)nn;
}

static void jet_white_plock_latch_poll(uint32_t keys, uint32_t rising)
{
    ko_longpress_t lp = {&g.white_press_ms, &g.white_long_used, &g.white_long_cancel};
    uint32_t bit;
    uint32_t base;
    uint8_t st;

    if (g.view != VIEW_SEQ || g.white_press_k >= 16u)
        return;
    bit = 1u << KEY_WHITE[g.white_press_k];
    if (!(keys & bit))
        return;
    if (!ko_longpress_poll(&lp, fm1_ms, JET_HOLD_MS, (rising & ~bit) != 0u))
        return;
    base = (uint32_t)g.seq_page * FM_SEQ_PAGE_STEPS;
    st = (uint8_t)(base + g.white_press_k);
    if (g.seq_plock_latch && g.seq_plock_step == st) {
        jet_plock_latch_end();
        return;
    }
    if (st >= seq_len())
        return;
    if (!g.cfg.seq.note[st])
        jet_seq_write_note(&g.cfg.seq, st, jet_note_store(g.seq_last_note ? g.seq_last_note : 60u), 8u, 100u, 0, 0u);
    jet_audition(g.cfg.seq.note[st], g.cfg.seq.vel[st]);
    jet_plock_latch_begin(st);
}

static uint8_t black_to_midi(uint8_t bk)
{
    int32_t oct = (int32_t)g.keyboard_oct * 12;
    int32_t nn = (int32_t)KEY0 + (int32_t)KEY_BLACK[bk % 11u] + oct;

    if (nn < 0)
        nn = 0;
    else if (nn > 127)
        nn = 127;
    return (uint8_t)nn;
}

static void jet_live_on(uint8_t note, uint8_t vel)
{
    jet_ev_t ev = {JET_EV_NOTE_ON, note, vel, 0u};

    jet_ev_push(ev);
    g.live_note = note;
    g.seq_last_note = note;
}

static void jet_live_off(uint8_t note)
{
    jet_ev_t ev = {JET_EV_NOTE_OFF, note, 0u, 0u};

    jet_ev_push(ev);
    if (g.live_note == note)
        g.live_note = 0u;
}

static void jet_all_off(uint8_t hard)
{
    jet_ev_t ev = {JET_EV_ALL_OFF, 0u, 0u, hard};

    jet_ev_push(ev);
}

/* Pattern notes are stored relative to KEY C; playback snaps to SCALE and adds KEY (jet_seq_snap). */
static uint8_t jet_note_store(uint8_t note)
{
    int32_t v = (int32_t)note - (int32_t)(g.cfg.groove.root % 12u);

    if (v < 1)
        v += 12;
    return (uint8_t)v;
}

static uint8_t jet_note_sound(uint8_t stored)
{
    return jet_seq_snap(stored, g.cfg.groove.root, g.cfg.groove.scale);
}

static void jet_seq_fire_step(uint8_t st)
{
    uint8_t ln = seq_len();
    uint8_t ix = (uint8_t)(st % ln);
    uint8_t raw = g.cfg.seq.note[ix];
    uint8_t n = raw;
    uint8_t leg = g.cfg.seq.legato[ix] & 1u;
    uint8_t slide = g.cfg.seq.flags[ix] & JET_SEQ_F_SLIDE ? 1u : 0u;
    uint8_t acc = g.cfg.seq.flags[ix] & JET_SEQ_F_ACCENT ? 1u : 0u;
    int8_t push_pl = (int8_t)g.cfg.seq.pl_push[ix];
    int8_t lpf_pl = (int8_t)g.cfg.seq.pl_lpf[ix];
    uint8_t prev_on = g.seq_last_on;
    uint8_t prev_note = g.gate_note;
    uint8_t tie;
    jet_ev_t ev;

    if (g.rec_skip_step == ix) {
        /* this note was just played live and quantised onto this step: it sounds already */
        g.rec_skip_step = 255u;
        n = 0u;
    }
    if (n)
        n = jet_note_sound(n);
    /* tie (REC of a held note): same pitch still sounding -> no new attack, the gate just goes on */
    tie = (uint8_t)(leg && n && prev_on && n == prev_note);
    slide = (uint8_t)(slide && n && prev_on && !tie);
    if (prev_on && !tie && !slide) {
        ev.type = JET_EV_NOTE_OFF;
        ev.note = prev_note;
        ev.vel = 0u;
        ev.flags = 0u;
        jet_ev_dispatch_isr(ev);
        g.seq_last_on = 0u;
    }
    if (n) {
        uint8_t gt = g.cfg.seq.gate[ix];
        uint8_t nx = (uint8_t)((ix + 1u) % ln);
        uint8_t hold = gt == 0u ? 1u : 0u;

        if (!tie) {
            uint8_t v = g.cfg.seq.vel[ix] ? g.cfg.seq.vel[ix] : 100u;
            uint8_t on = 0u;

            if (acc)
                v = (uint8_t)(v + 24u > 127u ? 127u : v + 24u);
            if (push_pl != (int8_t)FM_SEQ_PLOCK_NONE) {
                g.plk_push = push_pl;
                on |= 1u;
            }
            if (lpf_pl != (int8_t)FM_SEQ_PLOCK_NONE) {
                g.plk_lpf = (uint8_t)lpf_pl;
                on |= 2u;
            }
            g.plk_on = on;
            ev.type = JET_EV_NOTE_ON;
            ev.note = n;
            ev.vel = v;
            ev.flags = slide ? 2u : 0u;
            jet_ev_dispatch_isr(ev);
            if (slide && prev_note != n) {
                ev.type = JET_EV_NOTE_OFF;
                ev.note = prev_note;
                ev.vel = 0u;
                ev.flags = 0u;
                jet_ev_dispatch_isr(ev);
            }
        }
        g.seq_last_on = 1u;
        g.gate_note = n;
        /* hold into the next step when that one slides or ties from this note */
        if (nx != ix && g.cfg.seq.note[nx] &&
            ((g.cfg.seq.flags[nx] & JET_SEQ_F_SLIDE) || ((g.cfg.seq.legato[nx] & 1u) && g.cfg.seq.note[nx] == raw)))
            hold = 1u;
        if (hold)
            g.gate_left = 0u;
        else {
            uint32_t gs = (g.clk_spp * 6u * gt) / 8u;

            g.gate_left = gs ? gs : 1u;
        }
    } else {
        g.gate_left = 0u;
        g.plk_on = 0u;
    }
    g.jet_isr_fired_step = ix;
    g.isr_step_seq++;
}

/* Fire now, or after dly samples (one pending slot: an older pending step goes first). */
static void jet_step_launch(uint8_t st, uint32_t dly)
{
    if (g.fire_pend) {
        g.fire_pend = 0u;
        jet_seq_fire_step(g.fire_st);
    }
    if (dly) {
        g.fire_pend = 1u;
        g.fire_st = st;
        g.fire_left = dly;
    } else
        jet_seq_fire_step(st);
}

/* Offset of a step from its 16th in samples: swing on the off-beats plus MICRO (-5..5 ticks). Internal clock only. */
static int32_t jet_step_offset(uint8_t st, uint8_t odd)
{
    int32_t spp = (int32_t)g.clk_spp;
    int32_t off = (int32_t)g.cfg.seq.micro[st % seq_len()] * spp;

    if (g.cfg.groove.clock == JET_CLK_EXT)
        return 0;
    if (odd)
        off += (int32_t)g.clk_swing_extra;
    if (off > spp * 5)
        off = spp * 5;
    if (off < -spp * 5)
        off = -spp * 5;
    return off;
}

/* One 24 PPQN tick (INT/SEND from the sample clock, EXT per incoming F8). Step fires on the wrap, or up to
 * 5 ticks before it when its offset is negative. */
static void jet_clock_tick(void)
{
    uint8_t st = g.play_step;
    int32_t off;

    g.tick_mod6 = (g.tick_mod6 + 1u) % 6u;
    off = jet_step_offset(st, g.clk_odd);
    if (g.tick_mod6 != 0u) {
        if (off < 0 && !g.early_done) {
            uint32_t e = 1u;

            while (e < 5u && (int32_t)(e * g.clk_spp) < -off)
                e++;
            if (g.tick_mod6 == 6u - e) {
                g.early_done = 1u;
                jet_step_launch(st, (uint32_t)((int32_t)(e * g.clk_spp) + off));
            }
        }
        return;
    }
    if (g.early_done)
        g.early_done = 0u;
    else
        jet_step_launch(st, off > 0 ? (uint32_t)off : 0u);
    g.clk_odd ^= 1u;
    g.play_step++;
    if (g.play_step >= seq_len())
        g.play_step = 0u;
}

static void jet_clock_subblock(void)
{
    if (!g.playing)
        return;
    if (g.cfg.groove.clock != JET_CLK_EXT) {
        g.clk_acc += JET_SUB;
        while (g.clk_acc >= g.clk_spp) {
            g.clk_acc -= g.clk_spp;
            if (g.cfg.groove.clock == JET_CLK_SEND && jet_midi_clk_q < 255u)
                jet_midi_clk_q++;
            jet_clock_tick();
        }
    }
    if (g.fire_pend) {
        if (g.fire_left > JET_SUB)
            g.fire_left -= JET_SUB;
        else {
            g.fire_pend = 0u;
            jet_seq_fire_step(g.fire_st);
        }
    }
    if (g.gate_left > 0u) {
        if (g.gate_left > JET_SUB)
            g.gate_left -= JET_SUB;
        else {
            g.gate_left = 0u;
            if (g.seq_last_on) {
                jet_ev_t ev = {JET_EV_NOTE_OFF, g.gate_note, 0u, 0u};

                jet_ev_dispatch_isr(ev);
                g.seq_last_on = 0u;
            }
        }
    }
}

/* Step that sounds now (the transport counter already points at the next one). */
static uint8_t jet_ui_step(void)
{
    return g.playing ? g.jet_isr_fired_step : 0u;
}

static uint8_t jet_rec_on(void)
{
    return (uint8_t)(g.rec_arm && g.cfg.rec.mode != KO_REC_MONITOR);
}

/* REC armed + PLAY running: live record at the playhead. REC armed + stopped: step record at the cursor. */
static uint8_t jet_rec_live(void)
{
    return (uint8_t)(jet_rec_on() && g.playing);
}

static uint8_t jet_rec_stepping(void)
{
    return (uint8_t)(jet_rec_on() && !g.playing);
}

static void jet_rec_auto_len(void)
{
    if (g.cfg.rec_auto_len)
        ko_rec_auto_len(&g.rec, (uint8_t)(ko_rec_loop_steps(&g.cfg.rec, FM_SEQ_PAGE_STEPS) / FM_SEQ_PAGE_STEPS),
                        seq_len());
}

static void jet_main_poll_isr_steps(void)
{
    while (g.isr_step_seen != g.isr_step_seq) {
        uint8_t st = g.jet_isr_fired_step;

        g.isr_step_seen = g.isr_step_seq;
        if (jet_rec_live())
            ko_rec_transport_step_cfg(&g.cfg.rec, &g.rec_voice, &g.rec_writer, &g, st);
        g.ui_dirty = 1u;
    }
}

static void jet_step_cur_home(void)
{
    g.step_cur = g.view == VIEW_SEQ ? (uint8_t)(g.seq_page * FM_SEQ_PAGE_STEPS) : 0u;
    if (g.step_cur >= seq_len())
        g.step_cur = 0u;
}

/* Start of a take (REC armed, or PLAY pressed with REC armed): undo snapshot, AUTO length, REPLACE wipe armed. */
static void jet_rec_take_now(void)
{
    g.rec.playing = g.playing;
    g.rec.armed = g.rec_arm;
    if (!g.rec_arm)
        return;
    jet_rec_snapshot(0);
    g.rec.undo_ready = 1u;
    g.rec_fresh = 1u;
    jet_rec_auto_len();
    if (!g.playing)
        jet_step_cur_home();
}

static void jet_rec_replace_wipe(void)
{
    if (g.rec_fresh && g.cfg.rec.mode == KO_REC_REPLACE) {
        uint8_t ln = seq_len();

        jet_seq_clear(&g.cfg.seq);
        jet_seq_set_len(&g.cfg.seq, ln);
    }
    g.rec_fresh = 0u;
}

static void note_fmt(char *s, uint8_t note)
{
    const char *nm = NOTE_NAMES[note % 12u];
    int32_t oct = (int32_t)(note / 12u) - 1; /* 60 = C4 */

    while (*nm)
        *s++ = *nm++;
    oct_fmt(s, oct);
    if (*s == '+') { /* no sign on the octave */
        s[0] = s[1];
        s[1] = 0;
    }
}

static void jet_flash_step_note(uint8_t st, uint8_t stored)
{
    char m[12];
    uint32_t n = 0u;

    u8_dec(m, (uint32_t)st + 1u);
    while (m[n])
        n++;
    m[n++] = ' ';
    note_fmt(m + n, jet_note_sound(stored));
    jet_flash_param(m);
}

static void jet_step_cur_set(int32_t st)
{
    int32_t ln = (int32_t)seq_len();

    while (st < 0)
        st += ln;
    g.step_cur = (uint8_t)(st % ln);
    if (g.view == VIEW_SEQ)
        g.seq_page = (uint8_t)(g.step_cur / FM_SEQ_PAGE_STEPS);
    mark_dirty();
}

/* A note played on the keys or over MIDI while REC is armed. */
static void jet_rec_note_on(uint8_t note, uint8_t vel)
{
    uint8_t stored = jet_note_store(note);
    uint8_t ln = seq_len();

    if (jet_rec_live()) {
        uint8_t cur = (uint8_t)((g.play_step + ln - 1u) % ln); /* the 16th we are in */
        uint8_t sub = (uint8_t)g.tick_mod6;
        uint8_t st;
        uint8_t k;

        /* mono pattern: the newest note ends the one before */
        for (k = 0u; k < 16u; k++) {
            if (g.rec_voice.held[k])
                ko_rec_note_off(&g.rec_voice, &g.rec_writer, &g, k);
        }
        jet_rec_replace_wipe();
        g.rec_vel = vel;
        /* FREE keeps the tick inside the step, SLOP half of it, 16TH none */
        if (g.cfg.rec.quant == KO_REC_QUANT_FREE)
            g.rec_micro = (int8_t)sub;
        else if (g.cfg.rec.quant == KO_REC_QUANT_SLOP && sub < 4u)
            g.rec_micro = (int8_t)(sub / 2u);
        else
            g.rec_micro = 0;
        st = ko_rec_map_step(ko_rec_quant_step(&g.cfg.rec, sub, cur, ln), ln);
        g.rec_in_on = 1u;
        ko_rec_note_on(&g.cfg.rec, &g.rec_voice, &g.rec_writer, &g, ko_rec_slot_for_note(&g.rec, note), stored, sub,
                       cur, ln);
        g.rec_in_on = 0u;
        if (st != cur)
            g.rec_skip_step = st;
    } else if (jet_rec_stepping()) {
        jet_rec_replace_wipe();
        if (g.step_cur >= ln)
            g.step_cur = 0u;
        jet_seq_write_note(&g.cfg.seq, g.step_cur, stored, 8u, vel, 0, 0u);
        jet_flash_step_note(g.step_cur, stored);
        jet_step_cur_set((int32_t)g.step_cur + 1);
    }
}

static void jet_rec_note_off(uint8_t note)
{
    if (jet_rec_live())
        ko_rec_note_off(&g.rec_voice, &g.rec_writer, &g, ko_rec_slot_for_note(&g.rec, note));
}

static uint8_t jet_seq_is_empty(void)
{
    uint8_t ln = seq_len();
    uint8_t i;

    for (i = 0u; i < ln; i++) {
        if (g.cfg.seq.note[i])
            return 0u;
    }
    return 1u;
}

/* SEQ edit while stopped: let the step be heard. */
static void jet_audition(uint8_t stored, uint8_t vel)
{
    uint8_t n = jet_note_sound(stored);

    if (g.playing || !stored)
        return;
    if (g.aud_note)
        (void)jet_ev_push((jet_ev_t){JET_EV_NOTE_OFF, g.aud_note, 0u, 0u});
    (void)jet_ev_push((jet_ev_t){JET_EV_NOTE_ON, n, vel ? vel : 100u, 0u});
    g.aud_note = n;
    g.aud_off_ms = fm1_ms + 180u;
}

static void jet_audition_poll(void)
{
    if (g.aud_note && (g.playing || (int32_t)(fm1_ms - g.aud_off_ms) >= 0)) {
        (void)jet_ev_push((jet_ev_t){JET_EV_NOTE_OFF, g.aud_note, 0u, 0u});
        g.aud_note = 0u;
    }
}

static void jet_transport_start(uint8_t reset)
{
    g.playing = 1u;
    if (reset) {
        transport_reset();
        fm_lfo_slots_sync_restart(g.cfg.lfo, g.lfo_rt, JET_LFO_TGT_N, g.cfg.groove.bpm);
    }
    if (g.rec_arm)
        jet_rec_take_now();
    else if (jet_seq_is_empty())
        jet_toast("EMPTY", 0u);
}

static void jet_transport_stop(void)
{
    g.playing = 0u;
    g.fire_pend = 0u;
    g.plk_on = 0u;
    jet_all_off(0u);
    ko_rec_voice_reset(&g.rec_voice);
    if (g.rec_arm) /* back to step record, same take (UNDO still reaches before PLAY) */
        jet_step_cur_home();
}

static void jet_groove_commit(void)
{
    if (!g.groove_pending || g.playing)
        return;
    g.groove_pending = 0u;
    jet_plate_groove_load();
    transport_set_spp();
    mark_dirty();
}

static void cfg_defaults(void)
{
    jet_patch_defaults(&g.cfg.patch);
    g.patch_default = g.cfg.patch;
    jet_seq_demo(&g.cfg.seq);
    g.cfg.groove.bpm = JET_BPM_DEF;
    g.cfg.groove.swing = 50u;
    g.cfg.groove.root = 0u;
    g.cfg.groove.scale = 0u;
    g.cfg.groove.clock = JET_CLK_INT;
    g.cfg.groove.midi_ch = 0u;
    g.cfg.groove.bend = 2u;
    g.cfg.rec.mode = KO_REC_OVERDUB;
    g.cfg.rec.quant = KO_REC_QUANT_16;
    g.cfg.rec.bars = 0u;
    g.cfg.lfo_tgt = 0u;
    fm_lfo_slots_init(g.cfg.lfo, g.lfo_rt, JET_LFO_TGT_N);
    g.cfg.meta.magic = JET_STORE_MAGIC;
    g.cfg.meta.version = JET_STORE_VER;
    g.era = 0u;
    g.plate = 0u;
    g.keyboard_oct = 0;
}

static uint32_t jet_store_segs(deck_store_seg_t *s)
{
    jet_patch_defaults(&jet_store_patch_ref);
    jet_seq_default_init(&jet_store_seq_ref);
    fm_lfo_slots_init(jet_store_lfo_ref, 0, JET_LFO_TGT_N);
    jet_store_rec_ref.mode = KO_REC_OVERDUB;
    s[0] = (deck_store_seg_t){(uint8_t *)&g.cfg.meta, sizeof g.cfg.meta, 0, 0u};
    s[1] = (deck_store_seg_t){(uint8_t *)&g.cfg.groove, sizeof g.cfg.groove, 0, 0u};
    s[2] = (deck_store_seg_t){(uint8_t *)&g.cfg.patch, sizeof g.cfg.patch, (const uint8_t *)&jet_store_patch_ref,
                              sizeof jet_store_patch_ref};
    s[3] = (deck_store_seg_t){(uint8_t *)g.cfg.lfo, sizeof g.cfg.lfo, (const uint8_t *)jet_store_lfo_ref,
                              sizeof jet_store_lfo_ref};
    s[4] = (deck_store_seg_t){(uint8_t *)&g.cfg.seq, sizeof g.cfg.seq, (const uint8_t *)&jet_store_seq_ref,
                              sizeof jet_store_seq_ref};
    s[5] = (deck_store_seg_t){(uint8_t *)&g.cfg.rec, sizeof g.cfg.rec, (const uint8_t *)&jet_store_rec_ref,
                              sizeof jet_store_rec_ref};
    s[6] = (deck_store_seg_t){(uint8_t *)jet_plate_ov, sizeof jet_plate_ov, 0, 0u};
    return 7u;
}

static void jet_lfo_slots_sanitize(void)
{
    uint32_t t;

    for (t = 0u; t < JET_LFO_TGT_N; t++) {
        fm_lfo_slot_t *s = &g.cfg.lfo[t];

        if (s->algo >= FM_LFO_NALGOS)
            s->algo = FM_LFO_ALG_CLASSIC;
        s->depth = fm_lfo_clamp_u8(s->depth, 0u, 127u);
        s->wave = fm_lfo_clamp_u8(s->wave, 0u, 127u);
        s->phase = fm_lfo_clamp_u8(s->phase, 0u, 127u);
        s->smooth = fm_lfo_clamp_u8(s->smooth, 0u, 127u);
        s->skew = fm_lfo_clamp_u8(s->skew, 0u, 127u);
        s->ratio = fm_lfo_clamp_u8(s->ratio, 0u, 127u);
        s->index = fm_lfo_clamp_u8(s->index, 0u, 127u);
        s->fb = fm_lfo_clamp_u8(s->fb, 0u, 127u);
        s->fold = fm_lfo_clamp_u8(s->fold, 0u, 127u);
    }
    g.cfg.lfo_tgt %= JET_LFO_TGT_N;
}

/* 0: stored settings loaded; else defaults */
static int settings_load_jet(void)
{
    deck_store_seg_t seg[7];
    uint32_t nseg = jet_store_segs(seg);
    uint32_t k;

    uint32_t magic = JET_STORE_MAGIC;

    if (settings_load(jet_store_buf, sizeof jet_store_buf) == 0) {
        if (deck_store_decode(seg, nseg, magic, jet_store_buf, sizeof jet_store_buf, 0) != 0) {
            magic = JET_STORE_MAGIC_NOOV;
            nseg--;
        }
        if (deck_store_decode(seg, nseg, magic, jet_store_buf, sizeof jet_store_buf, 0) == 0) {
            (void)deck_store_decode(seg, nseg, magic, jet_store_buf, sizeof jet_store_buf, 1);
            if (magic == JET_STORE_MAGIC_NOOV)
                memset(jet_plate_ov, 0, sizeof jet_plate_ov);
            if (g.cfg.meta.version != JET_STORE_VER) {
                cfg_defaults();
                return -1;
            }
            jet_patch_sanitize(&g.cfg.patch);
            jet_seq_sanitize(&g.cfg.seq);
            fm_lfo_slots_fixup_empty_rates(g.cfg.lfo, JET_LFO_TGT_N);
            jet_lfo_slots_sanitize();
            g.era = jet_clamp_era(g.cfg.meta.era);
            g.plate = jet_clamp_plate(g.cfg.meta.plate);
            g.keyboard_oct = g.cfg.meta.oct;
            g.cfg.groove.root %= 12u; /* alte Stände: ROOT war MIDI 0..127 */
            if (g.cfg.groove.scale >= JET_SCALE_N)
                g.cfg.groove.scale = 0u;
            for (k = 0u; k < JET_DESK_N; k++)
                g.cfg.meta.page[k] = (uint8_t)(g.cfg.meta.page[k] % JET_BANK_PAGES[k]);
            return 0;
        }
    }
    cfg_defaults();
    return -1;
}

static int settings_save_jet(void)
{
    deck_store_seg_t seg[7];
    uint32_t nseg = jet_store_segs(seg);
    uint32_t comp;
    uint32_t i;

    g.cfg.meta.era = g.era;
    g.cfg.meta.plate = g.plate;
    g.cfg.meta.oct = (int8_t)g.keyboard_oct;
    comp = deck_store_encode(seg, nseg, JET_STORE_MAGIC, jet_store_buf, sizeof jet_store_buf);
    if (!comp) {
        /* plate overrides stay in RAM but are not persisted this time */
        comp = deck_store_encode(seg, nseg - 1u, JET_STORE_MAGIC_NOOV, jet_store_buf, sizeof jet_store_buf);
        if (!comp)
            return -1;
        jet_toast("OV FULL", 0u);
    }
    /* settings_load checks the stored length: always save the full buffer that settings_load_jet asks for */
    for (i = (uint32_t)sizeof(deck_store_hdr_t) + comp; i < sizeof jet_store_buf; i++)
        jet_store_buf[i] = 0u;
    g.set_dirty = 0u;
    return settings_save(jet_store_buf, sizeof jet_store_buf) != 0 ? -1 : 0;
}

static void jet_save_plate_ov(uint8_t factory)
{
    jet_plate_ov_t *o = &jet_plate_ov[g.plate];

    if (factory) {
        o->used = 0u;
        jet_toast("FACTORY", 0u);
    } else {
        o->used = 1u;
        o->era = g.era;
        o->patch = g.cfg.patch;
        o->bpm = g.cfg.groove.bpm;
        o->swing = g.cfg.groove.swing;
        o->root = (int8_t)g.cfg.groove.root;
        jet_toast("SAVED", 0u);
    }
    mark_dirty();
}

static int jet_param_is_signed(uint8_t id)
{
    if (id >= JET_P_N)
        return 0;
    return JET_PARAM[id].is_signed;
}

static int jet_param_get_ptr(uint8_t id, uint8_t **pu, int8_t **pi)
{
    jet_patch_t *p = &g.cfg.patch;

    if (id < JET_P_N) {
        const jet_param_t *pr = &JET_PARAM[id];
        uint8_t *b = (uint8_t *)p + pr->off;

        if (pr->is_signed) {
            *pi = (int8_t *)b;
            *pu = 0;
        } else {
            *pu = b;
            *pi = 0;
        }
        return 1;
    }
    switch (id) {
    case JET_P_BPM:
        *pu = &g.cfg.groove.bpm;
        return 1;
    case JET_P_SWING:
        *pu = &g.cfg.groove.swing;
        return 1;
    case JET_P_ROOT:
        *pu = &g.cfg.groove.root;
        return 1;
    case JET_P_SCALE:
        *pu = &g.cfg.groove.scale;
        return 1;
    case JET_P_CLOCK:
        *pu = &g.cfg.groove.clock;
        return 1;
    case JET_P_MIDI:
        *pu = &g.cfg.groove.midi_ch;
        return 1;
    case JET_P_BEND:
        *pu = &g.cfg.groove.bend;
        return 1;
    case JET_P_LFO_TGT:
        *pu = &g.cfg.lfo_tgt;
        return 1;
    case JET_P_LFO_RATE:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].rate;
        return 1;
    case JET_P_LFO_DEPTH:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].depth;
        return 1;
    case JET_P_LFO_SHAPE:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].wave;
        return 1;
    case JET_P_LFO_PHASE:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].phase;
        return 1;
    case JET_P_LFO_SKEW:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].skew;
        return 1;
    case JET_P_LFO_SMOOTH:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].smooth;
        return 1;
    case JET_P_LFO_FOLD:
        *pu = &g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N].fold;
        return 1;
    case JET_P_REC_MODE:
        *pu = &g.cfg.rec.mode;
        return 1;
    case JET_P_REC_QUANT:
        *pu = &g.cfg.rec.quant;
        return 1;
    case JET_P_REC_BARS:
        *pu = &g.cfg.rec.bars;
        return 1;
    case JET_P_REC_AUTO:
        *pu = &g.cfg.rec_auto_len;
        return 1;
    default:
        return 0;
    }
}

static uint8_t jet_page_param(uint8_t col)
{
    uint8_t dv;
    uint8_t pg;
    uint8_t id;

    if (g.view == VIEW_SEQ) {
        static const uint8_t SEQ_PL1[4] = {JET_P_SEQ_VEL, JET_P_SEQ_PUSH, JET_P_SEQ_MICRO, JET_P_SEQ_NOTE};
        static const uint8_t SEQ_PL2[4] = {JET_P_SEQ_GATE, JET_P_SEQ_ACCENT, JET_P_SEQ_SLIDE, JET_P_SEQ_LPF};
        static const uint8_t SEQ_PG0[4] = {JET_P_SEQ_LEN, JET_P_SEQ_PAGE, JET_P_SEQ_ROT, JET_P_SEQ_CLR};
        static const uint8_t SEQ_PG1[4] = {JET_P_REC_MODE, JET_P_REC_QUANT, JET_P_REC_BARS, JET_P_REC_AUTO};
        const uint8_t *t;

        if (g.seq_plock_latch)
            t = g.seq_plock_bank ? SEQ_PL2 : SEQ_PL1;
        else
            t = g.seq_knob_page ? SEQ_PG1 : SEQ_PG0;
        return t[col % 4u];
    }
    dv = jet_desk_view();
    pg = jet_bank_page();
    id = JET_PAGE[dv][pg][col % 4u];
    return id;
}


static void jet_knob_label(uint8_t id, char *s);
static void jet_knob_value(uint8_t id, char *s);

static void jet_adj_param(uint8_t id, int32_t steps, uint8_t edit)
{
    uint8_t *pu = 0;
    int8_t *pi = 0;
    char msg[16];

    if (id == JET_P_NONE || !steps)
        return;
    if (id == JET_P_PLAY) {
        g.playing = steps > 0 ? 1u : 0u;
        if (!g.playing)
            jet_all_off(0u);
        else
            transport_reset();
        mark_dirty();
        return;
    }
    if (id == JET_P_OCT) {
        adj_i8(&g.keyboard_oct, steps, -3, 3);
        str_copy(msg, "OCT ");
        oct_fmt(msg + 4, g.keyboard_oct);
        jet_flash_param(msg);
        mark_dirty();
        return;
    }
    if (id == JET_P_REC_UNDO) {
        if (steps > 0)
            ko_rec_take_undo(&g.rec);
        mark_dirty();
        return;
    }
    if (id == JET_P_REC_CLEAR) {
        if (steps > 0) {
            jet_seq_clear_all(&g.cfg.seq);
            jet_seq_page_clamp();
            ko_rec_voice_reset(&g.rec_voice);
        }
        mark_dirty();
        return;
    }
    if (id == JET_P_SEQ_CLR) {
        if (steps > 0) {
            if (++g.seq_clear_arm >= 2u) {
                jet_seq_clear_all(&g.cfg.seq);
                jet_seq_page_clamp();
                g.seq_clear_arm = 0u;
                jet_toast("CLEAR", 0u);
            } else {
                g.seq_clear_ms = fm1_ms;
                jet_toast("CLEAR?", 0u);
            }
        }
        mark_dirty();
        return;
    }
    if (id >= JET_P_SEQ_LEN && id <= JET_P_SEQ_ACCENT) {
        jet_seq_adj(id, steps, edit);
        return;
    }
    if (!jet_param_get_ptr(id, &pu, &pi))
        return;
    if (id < JET_P_N) {
        const jet_param_t *pr = &JET_PARAM[id];

        if (edit)
            ko_edit_snap(steps, pu ? pu : (uint8_t *)pi, (uint8_t)pr->def, (uint8_t)pr->hi);
        else if (pr->fmt == JET_FMT_ENUM || pr->fmt == JET_FMT_TOG) {
            int32_t v = jet_param_get(&g.cfg.patch, id) + (steps > 0 ? 1 : -1);

            while (v < pr->lo)
                v += pr->hi - pr->lo + 1;
            while (v > pr->hi)
                v -= pr->hi - pr->lo + 1;
            jet_param_set(&g.cfg.patch, id, v);
        } else if (pi)
            adj_i8(pi, steps, (int8_t)pr->lo, (int8_t)pr->hi);
        else
            adj_u8(pu, steps, (uint8_t)pr->lo, (uint8_t)pr->hi);
        if (pr->fmt == JET_FMT_SYNC)
            fm_time_fmt(msg, (uint8_t)jet_param_get(&g.cfg.patch, id), g.cfg.groove.bpm, fm1_ms);
        else
            jet_param_fmt(msg, id, jet_param_get(&g.cfg.patch, id));
        jet_flash_param(msg);
    } else if (pu) {
        if (id == JET_P_BPM)
            adj_u8(pu, steps, JET_BPM_MIN, JET_BPM_MAX);
        else if (id == JET_P_SWING)
            adj_u8(pu, steps, 50u, 75u);
        else if (id == JET_P_ROOT)
            adj_u8(pu, steps, 0u, 11u);
        else if (id == JET_P_SCALE)
            adj_u8(pu, steps, 0u, (uint8_t)(JET_SCALE_N - 1u));
        else if (id == JET_P_CLOCK)
            adj_u8(pu, steps, 0u, (uint8_t)(JET_CLK_N - 1u));
        else if (id == JET_P_MIDI)
            adj_u8(pu, steps, 0u, 16u);
        else if (id == JET_P_BEND)
            adj_u8(pu, steps, 0u, 12u);
        else if (id == JET_P_LFO_TGT)
            adj_u8(pu, steps, 0u, (uint8_t)(JET_LFO_TGT_N - 1u));
        else if (id == JET_P_LFO_RATE)
            adj_u8(pu, steps, fm_lfo_rate_min(g.cfg.groove.bpm), 127u);
        else if (id == JET_P_LFO_SHAPE)
            *pu = fm_lfo_step_shape(*pu, steps, 0u);
        else if (id == JET_P_REC_MODE || id == JET_P_REC_QUANT)
            adj_u8(pu, steps, 0u, 2u);
        else if (id == JET_P_REC_BARS)
            adj_u8(pu, steps, 0u, 3u);
        else if (id == JET_P_REC_AUTO)
            *pu = steps > 0 ? 1u : 0u;
        else
            adj_u8(pu, steps, 0u, 127u);
        if (id >= JET_P_LFO_TGT && id <= JET_P_LFO_FOLD) {
            char lbl[16];

            jet_knob_label(id, lbl);
            jet_knob_value(id, msg);
            jet_flash_kv(lbl, msg);
        }
    }
    if (id == JET_P_BPM || id == JET_P_SWING)
        transport_set_spp();
    mark_dirty();
}

static void jet_knob_label(uint8_t id, char *s)
{
    if (id == JET_P_NONE) {
        str_copy(s, "----");
        return;
    }
    if (id < JET_P_N) {
        str_copy(s, JET_PARAM[id].lbl);
        return;
    }
    switch (id) {
    case JET_P_BPM:
        str_copy(s, "TEMPO");
        return;
    case JET_P_SWING:
        str_copy(s, "SWING");
        return;
    case JET_P_ROOT:
        str_copy(s, "KEY");
        return;
    case JET_P_SCALE:
        str_copy(s, "SCALE");
        return;
    case JET_P_CLOCK:
        str_copy(s, "CLOCK");
        return;
    case JET_P_MIDI:
        str_copy(s, "MIDI");
        return;
    case JET_P_BEND:
        str_copy(s, "BEND");
        return;
    case JET_P_OCT:
        str_copy(s, "OCT");
        return;
    case JET_P_LFO_TGT:
        str_copy(s, "TARGET");
        return;
    case JET_P_LFO_RATE:
        str_copy(s, "RATE");
        return;
    case JET_P_LFO_DEPTH:
        str_copy(s, "DEPTH");
        return;
    case JET_P_LFO_SHAPE:
        str_copy(s, "SHAPE");
        return;
    case JET_P_PLAY:
        str_copy(s, "PLAY");
        return;
    case JET_P_LFO_PHASE:
        str_copy(s, "PHASE");
        return;
    case JET_P_LFO_SKEW:
        str_copy(s, "SKEW");
        return;
    case JET_P_LFO_SMOOTH:
        str_copy(s, "SMOOTH");
        return;
    case JET_P_LFO_FOLD:
        str_copy(s, "FOLD");
        return;
    case JET_P_REC_MODE:
        str_copy(s, "MODE");
        return;
    case JET_P_REC_QUANT:
        str_copy(s, "QUANT");
        return;
    case JET_P_REC_BARS:
        str_copy(s, "BARS");
        return;
    case JET_P_REC_AUTO:
        str_copy(s, "AUTO");
        return;
    case JET_P_SEQ_LEN:
        str_copy(s, "LENGTH");
        return;
    case JET_P_SEQ_PAGE:
        str_copy(s, "PAGE");
        return;
    case JET_P_SEQ_ROT:
        str_copy(s, "ROTATE");
        return;
    case JET_P_SEQ_CLR:
        str_copy(s, "CLEAR");
        return;
    case JET_P_SEQ_VEL:
        str_copy(s, "VEL");
        return;
    case JET_P_SEQ_GATE:
        str_copy(s, "GATE");
        return;
    case JET_P_SEQ_MICRO:
        str_copy(s, "MICRO");
        return;
    case JET_P_SEQ_NOTE:
        str_copy(s, "PITCH");
        return;
    case JET_P_SEQ_PUSH:
        str_copy(s, "PUSH");
        return;
    case JET_P_SEQ_LPF:
        str_copy(s, "LPF");
        return;
    case JET_P_SEQ_SLIDE:
        str_copy(s, "SLIDE");
        return;
    case JET_P_SEQ_ACCENT:
        str_copy(s, "ACCNT");
        return;
    default:
        str_copy(s, "----");
        return;
    }
}

static void jet_lfo_rate_fmt(char *s, uint8_t rate)
{
    uint32_t bpm = g.cfg.groove.bpm;
    uint8_t show = fm_lfo_rate_clamp(rate, bpm);
    const char *nd = fm_lfo_note_div(show, bpm);
    uint32_t hz100;
    uint32_t n = 0u;

    if (nd) {
        str_copy(s, nd);
        return;
    }
    hz100 = fm_lfo_hz100(fm_lfo_rate_resolved(show, bpm));
    u8_dec(s, hz100 / 100u);
    while (s[n])
        n++;
    s[n++] = '.';
    s[n++] = (char)('0' + (hz100 % 100u) / 10u);
    s[n++] = 'H';
    s[n++] = 'Z';
    s[n] = 0;
}

static void jet_knob_value(uint8_t id, char *s)
{
    uint8_t *pu = 0;
    int8_t *pi = 0;

    if (id == JET_P_NONE) {
        str_copy(s, "----");
        return;
    }
    if (id == JET_P_PLAY) {
        str_copy(s, g.playing ? "RUN" : "STOP");
        return;
    }
    if (id == JET_P_OCT) {
        oct_fmt(s, g.keyboard_oct);
        return;
    }
    if (id >= JET_P_SEQ_LEN && id <= JET_P_SEQ_ACCENT) {
        jet_seq_t *sq = &g.cfg.seq;
        uint8_t ix = jet_seq_plock_ix();
        uint8_t ln = seq_len();

        if (id == JET_P_SEQ_LEN) {
            u8_dec(s, ln);
            return;
        }
        if (id == JET_P_SEQ_PAGE) {
            u8_dec(s, (uint32_t)(g.seq_page + 1u));
            return;
        }
        if (id == JET_P_SEQ_ROT) {
            str_copy(s, "----");
            return;
        }
        if (id == JET_P_SEQ_CLR) {
            str_copy(s, g.seq_clear_arm ? "?" : "----");
            return;
        }
        if (ix >= ln) {
            str_copy(s, "----");
            return;
        }
        if (id == JET_P_SEQ_VEL)
            u8_dec(s, sq->vel[ix] ? sq->vel[ix] : 100u);
        else if (id == JET_P_SEQ_GATE)
            u8_dec(s, sq->gate[ix]);
        else if (id == JET_P_SEQ_MICRO) {
            if (sq->micro[ix] < 0) {
                s[0] = '-';
                u8_dec(s + 1, (uint32_t)(-sq->micro[ix]));
            } else
                u8_dec(s, (uint32_t)sq->micro[ix]);
        } else if (id == JET_P_SEQ_NOTE) {
            if (sq->note[ix])
                note_fmt(s, jet_note_sound(sq->note[ix]));
            else
                str_copy(s, "----");
        }
        else if (id == JET_P_SEQ_PUSH) {
            if (sq->pl_push[ix] == FM_SEQ_PLOCK_NONE)
                str_copy(s, "----");
            else
                u8_dec(s, (uint32_t)(int8_t)sq->pl_push[ix] + 64u);
        } else if (id == JET_P_SEQ_LPF) {
            if (sq->pl_lpf[ix] == FM_SEQ_PLOCK_NONE)
                str_copy(s, "----");
            else
                u8_dec(s, sq->pl_lpf[ix]);
        } else if (id == JET_P_SEQ_SLIDE)
            str_copy(s, sq->flags[ix] & JET_SEQ_F_SLIDE ? "ON" : "OFF");
        else if (id == JET_P_SEQ_ACCENT)
            str_copy(s, sq->flags[ix] & JET_SEQ_F_ACCENT ? "ON" : "OFF");
        else
            str_copy(s, "----");
        return;
    }
    if (id < JET_P_N) {
        if (JET_PARAM[id].fmt == JET_FMT_SYNC)
            fm_time_fmt(s, (uint8_t)jet_param_get(&g.cfg.patch, id), g.cfg.groove.bpm, fm1_ms);
        else
            jet_param_fmt(s, id, jet_param_get(&g.cfg.patch, id));
        return;
    }
    if (jet_param_get_ptr(id, &pu, &pi) && pu) {
        if (id == JET_P_BPM) {
            u8_dec(s, *pu);
            return;
        }
        if (id == JET_P_ROOT) {
            str_copy(s, NOTE_NAMES[*pu % 12u]);
            return;
        }
        if (id == JET_P_SCALE) {
            str_copy(s, JET_SCALE_NAME[*pu % JET_SCALE_N]);
            return;
        }
        if (id == JET_P_CLOCK) {
            str_copy(s, JET_CLOCK_NAME[*pu % JET_CLK_N]);
            return;
        }
        if (id == JET_P_MIDI && *pu == 0u) {
            str_copy(s, "ALL");
            return;
        }
        if (id == JET_P_LFO_TGT) {
            str_copy(s, JET_LFO_NAME[*pu % JET_LFO_TGT_N]);
            return;
        }
        if (id == JET_P_REC_MODE) {
            str_copy(s, REC_MODE_LBL[*pu % 3u]);
            return;
        }
        if (id == JET_P_REC_QUANT) {
            str_copy(s, REC_QUANT_LBL[*pu % 3u]);
            return;
        }
        if (id == JET_P_REC_BARS) {
            str_copy(s, REC_BARS_LBL[*pu % 4u]);
            return;
        }
        if (id == JET_P_LFO_RATE) {
            jet_lfo_rate_fmt(s, *pu);
            return;
        }
        if (id == JET_P_LFO_SHAPE) {
            str_copy(s, fm_lfo_shape_name(*pu));
            return;
        }
        if (id == JET_P_LFO_PHASE) {
            u8_dec(s, ((uint32_t)*pu * 100u + 63u) / 127u);
            return;
        }
        if (id == JET_P_REC_AUTO) {
            str_copy(s, *pu ? "ON" : "OFF");
            return;
        }
        u8_dec(s, *pu);
        return;
    }
    str_copy(s, "----");
}

static uint8_t jet_param_lfo_tgt(uint8_t id);

/* LFO marker + base±depth readout on a knob whose param has a live LFO target (depth > 0). */
static void jet_knob_lfo_marks(uint8_t id, ko_knob_slot_t *k)
{
    uint8_t lt = jet_param_lfo_tgt(id);
    const jet_param_t *pr;
    int32_t raw;
    int32_t dp;
    int32_t lo;
    int32_t hi;

    if (lt >= JET_LFO_TGT_N || g.cfg.lfo[lt].depth == 0u || k->mode != KNOB_POT_CONT)
        return;
    pr = &JET_PARAM[id];
    if (pr->fmt == JET_FMT_SYNC)
        return;
    k->lfo = 1u;
    k->mark_col = C_ACC;
    if (lt == JET_LFO_PITCH)
        return;
    raw = jet_param_get(&g.cfg.patch, id);
    dp = (int32_t)g.cfg.lfo[lt].depth;
    lo = raw - dp;
    hi = raw + dp;
    if (lo < pr->lo)
        lo = pr->lo;
    if (hi > pr->hi)
        hi = pr->hi;
    jet_param_fmt(k->range_lo, id, lo);
    jet_param_fmt(k->range_hi, id, hi);
    k->range = 1u;
}

/* Pot for groove / LFO / REC / SEQ pseudo params: pv 0..127; enum-like ones as SEL/TICK/TOG. */
static void jet_knob_pseudo(uint8_t id, ko_knob_slot_t *k)
{
    uint8_t *pu = 0;
    int8_t *pi = 0;
    int32_t v = 0;
    int32_t lo = 0;
    int32_t hi = 127;
    uint8_t n = 0u;

    if (id >= JET_P_SEQ_LEN && id <= JET_P_SEQ_ACCENT) {
        const jet_seq_t *sq = &g.cfg.seq;
        uint8_t ix = jet_seq_plock_ix();
        uint8_t ln = seq_len();

        if (id == JET_P_SEQ_LEN) {
            v = ln;
            lo = 1;
            hi = FM_SEQ_STEPS;
        } else if (id == JET_P_SEQ_PAGE) {
            v = g.seq_page;
            n = jet_seq_pages(sq);
        } else if (id == JET_P_SEQ_ROT) {
            v = 64;
            k->bip = 1u;
        } else if (id == JET_P_SEQ_CLR) {
            v = g.seq_clear_arm ? 1 : 0;
            n = 2u;
        } else if (ix >= ln) {
            v = 0;
        } else if (id == JET_P_SEQ_VEL) {
            v = sq->vel[ix] ? sq->vel[ix] : 100;
        } else if (id == JET_P_SEQ_GATE) {
            v = sq->gate[ix];
            hi = 8;
        } else if (id == JET_P_SEQ_MICRO) {
            v = sq->micro[ix];
            lo = -JET_SEQ_MICRO_MAX;
            hi = JET_SEQ_MICRO_MAX;
            k->bip = 1u;
        } else if (id == JET_P_SEQ_NOTE) {
            v = sq->note[ix];
        } else if (id == JET_P_SEQ_PUSH) {
            v = sq->pl_push[ix] == FM_SEQ_PLOCK_NONE ? (int32_t)g.cfg.patch.push : (int32_t)(int8_t)sq->pl_push[ix];
            lo = -64;
            hi = 63;
            k->bip = 1u;
        } else if (id == JET_P_SEQ_LPF) {
            v = sq->pl_lpf[ix] == FM_SEQ_PLOCK_NONE ? (int32_t)g.cfg.patch.lpf : (int32_t)sq->pl_lpf[ix];
        } else {
            v = (sq->flags[ix] & (id == JET_P_SEQ_SLIDE ? JET_SEQ_F_SLIDE : JET_SEQ_F_ACCENT)) ? 1 : 0;
            n = 2u;
        }
    } else if (id == JET_P_PLAY) {
        v = g.playing ? 1 : 0;
        n = 2u;
    } else if (id == JET_P_OCT) {
        v = g.keyboard_oct;
        lo = -3;
        hi = 3;
        k->bip = 1u;
    } else if (jet_param_get_ptr(id, &pu, &pi) && pu) {
        v = *pu;
        switch (id) {
        case JET_P_BPM:
            lo = JET_BPM_MIN;
            hi = JET_BPM_MAX;
            break;
        case JET_P_SWING:
            lo = 50;
            hi = 75;
            break;
        case JET_P_ROOT:
            n = 12u;
            break;
        case JET_P_SCALE:
            n = JET_SCALE_N;
            break;
        case JET_P_CLOCK:
            n = JET_CLK_N;
            break;
        case JET_P_MIDI:
            n = 17u;
            break;
        case JET_P_BEND:
            hi = 12;
            break;
        case JET_P_LFO_TGT:
            n = JET_LFO_TGT_N;
            break;
        case JET_P_LFO_SHAPE:
            k->mode = KNOB_POT_SHAPE;
            k->steps = KNOB_POT_SHAPE_N;
            k->pv = (uint8_t)(v > 127 ? 127 : v);
            return;
        case JET_P_LFO_SKEW:
            k->bip = 1u;
            break;
        case JET_P_REC_MODE:
        case JET_P_REC_QUANT:
            n = 3u;
            break;
        case JET_P_REC_BARS:
            n = 4u;
            break;
        case JET_P_REC_AUTO:
            n = 2u;
            break;
        default:
            break;
        }
    } else {
        return;
    }
    if (n >= 2u) {
        if (v >= n)
            v = (int32_t)n - 1;
        k->mode = n == 2u ? KNOB_POT_TOG : n <= KNOB_POT_SEL_MAX_STEPS ? KNOB_POT_SEL : KNOB_POT_TICK;
        k->steps = n;
        k->pv = (uint8_t)((v * 127) / ((int32_t)n - 1));
        return;
    }
    if (v < lo)
        v = lo;
    if (v > hi)
        v = hi;
    k->pv = (uint8_t)(hi > lo ? ((v - lo) * 127) / (hi - lo) : 0);
}

static void jet_knob_fill(uint32_t col, ko_knob_slot_t *k)
{
    static char valbuf[4][16];
    static char labbuf[4][16];
    uint8_t id = jet_page_param((uint8_t)col);

    jet_knob_label(id, labbuf[col]);
    jet_knob_value(id, valbuf[col]);
    str_copy(k->label, labbuf[col]);
    str_copy(k->value, valbuf[col]);
    k->pv = 0u;
    k->vcol = C_WHITE;
    k->klab = col == 0u ? jet_era_col(g.era) : C_GRAY;
    k->pot_col = jet_era_col(g.era);
    k->mark_col = C_ACC;
    k->pot_track = 52u;
    k->mode = KNOB_POT_CONT;
    k->bip = 0u;
    k->steps = 0u;
    if (id == JET_P_NONE) {
        k->off = 1u;
        return;
    }
    if (id >= JET_P_N) {
        jet_knob_pseudo(id, k);
        return;
    }
    {
        const jet_param_t *pr = &JET_PARAM[id];
        int32_t raw = jet_param_get(&g.cfg.patch, id);
        int32_t span = pr->hi - pr->lo;

        if (pr->fmt == JET_FMT_ENUM || pr->fmt == JET_FMT_TOG) {
            k->mode = span == 1 ? KNOB_POT_TOG : KNOB_POT_SEL;
            k->steps = (uint8_t)(span + 1);
            k->pv = (uint8_t)(span > 0 ? ((raw - pr->lo) * 127) / span : 0);
        } else if (pr->is_signed) {
            int32_t m = -pr->lo > pr->hi ? -pr->lo : pr->hi;
            int32_t pv = 64 + (raw * 64) / (m ? m : 1);

            k->bip = 1u;
            k->pv = (uint8_t)(pv < 0 ? 0 : pv > 127 ? 127 : pv);
        } else {
            k->bip = pr->fmt == JET_FMT_BIP ? 1u : 0u;
            k->pv = (uint8_t)(span > 0 ? ((raw - pr->lo) * 127) / span : 0);
        }
    }
    jet_knob_lfo_marks(id, k);
}

/* Beat/bar progress rail (16 bars x 4 quarters), 16 steps per bar. */
static void jet_draw_rail(int32_t y_bot)
{
    dub_knob_rail_ctx_t c = {0};

    c.playing = g.playing ? 1u : 0u;
    c.ext_clock = g.cfg.groove.clock == JET_CLK_EXT ? 1u : 0u;
    c.era = jet_era_col(g.era);
    c.vis_steps = seq_len();
    c.cur_ix = jet_ui_step();
    c.steps_per_bar = 16u;
    dub_draw_bar_progress_rail(y_bot, &c);
}

static void jet_draw_knobs(void)
{
    ko_knob_bar_t bar = {0};

    bar.h = 86;
    bar.blit_y = 154;
    bar.rail_y = 78;
    bar.center_label = 1u;
    bar.fill = jet_knob_fill;
    bar.rail = jet_draw_rail;
    ko_draw_knob_bar(&bar);
}

static const char *jet_stage_name(void)
{
    if (g.view == VIEW_SEQ) {
        if (g.seq_plock_latch)
            return "STEP";
        if (jet_rec_live())
            return "LIVEREC";
        if (jet_rec_stepping())
            return "STEPREC";
        return g.seq_knob_page ? "REC" : "SEQ";
    }
    return JET_STAGE[jet_desk_view()][jet_bank_page() % 4u];
}

static void jet_draw_top(void)
{
    ko_top_bar_t t = {0};
    ko_wave_ctx_t w = {0};
    char bpm[8];
    char sw[8];

    w.static_glow = 1u;
    w.glow = g.out_peak;
    w.sine_q64 = jet_wave_sine_q64;
    t.wave = &w;
    t.show_title = 1u;
    t.cursor = -1;
    str_copy(t.title, jet_plate_name(g.plate));
    str_copy(t.sub, jet_era_name(g.era));
    ko_top_bar_era_plate_cols(&t, jet_era_col, jet_era_of_plate(g.plate), g.era);
    t.rec = g.rec_arm;
    t.stage = jet_stage_name();
    t.key = NOTE_NAMES[g.cfg.groove.root % 12u];
    t.scale = JET_SCALE_SHORT[g.cfg.groove.scale % JET_SCALE_N];
    u8_dec(bpm, g.cfg.groove.bpm);
    str_copy(t.bpm, bpm);
    if (g.cfg.groove.swing != 50u) {
        u8_dec(sw, g.cfg.groove.swing);
        str_copy(t.swing, sw);
    }
    t.playing = g.playing;
    t.on_beat = g.playing && (g.tick_mod6 == 0u);
    t.on_one = g.playing && jet_ui_step() == 0u;
    if (g.view == VIEW_SEQ) {
        uint8_t fs = jet_seq_focus_step();

        t.pages = 2u;
        t.page = g.seq_plock_latch ? g.seq_plock_bank : g.seq_knob_page;
        /* step number and its note after the stage word */
        t.seq = 1u;
        t.seq_x0 = 150;
        t.seq_w = 0;
        if (fs < seq_len()) {
            u8_dec(t.seq_txt[0], (uint32_t)fs + 1u);
            t.seq_col[0] = C_ACC;
            if (g.cfg.seq.note[fs])
                note_fmt(t.seq_txt[1], jet_note_sound(g.cfg.seq.note[fs]));
            t.seq_col[1] = C_GRAY;
        }
    } else {
        t.pages = JET_BANK_PAGES[jet_desk_view()];
        t.page = jet_bank_page();
    }
    t.flash = &g.flash;
    t.now = fm1_ms;
    ko_draw_top_bar(&t);
}

static void jet_draw_env(void)
{
    fm_env_kit_plot_t ep = {0};
    fm_env_kit_geom_t gm;
    bass_dahdsr_cfg_t cfg;
    const jet_patch_t *p = &g.cfg.patch;

    cfg.t.delay = p->s_dly;
    cfg.t.attack = p->s_atk;
    cfg.t.hold = p->s_hold;
    cfg.t.decay = p->s_dec;
    cfg.t.sustain = p->s_sus;
    cfg.t.release = p->s_rel;
    ep.w[0] = bass_dahdsr_stage_samples(BASS_ENV_DELAY, &cfg);
    ep.w[1] = bass_dahdsr_stage_samples(BASS_ENV_ATTACK, &cfg);
    ep.w[2] = bass_dahdsr_stage_samples(BASS_ENV_HOLD, &cfg);
    ep.w[3] = bass_dahdsr_stage_samples(BASS_ENV_DECAY, &cfg);
    ep.w[4] = bass_dahdsr_stage_samples(BASS_ENV_RELEASE, &cfg);
    ep.sustain = p->s_sus;
    ep.curve = p->s_curve;
    ep.col = jet_era_col(g.era);
    fm_env_kit_draw(&ep, &gm);
    if (g.eng.vo[0].active) {
        int32_t px = bass_env_playhead_x(&g.eng.vo[0].env, &cfg, gm.px);

        if (px >= 0)
            cv_line(px, FM_ENV_KIT_Y_TOP, px, FM_ENV_KIT_Y_BOT, C_WHITE);
    }
}

static void jet_draw_scope(void)
{
    uint32_t i;
    uint32_t at = g.scope_ix; /* oldest sample first, so the trace has no seam */
    int32_t py = 66;

    for (i = 0u; i < 240u; i++) {
        int32_t y = 66 - (((int32_t)g.scope[(at + i) % 240u] * 28) >> 15);

        if (i == 0u)
            py = y;
        cv_line(i ? (int32_t)i - 1 : 0, py, (int32_t)i, y, C_ACC);
        py = y;
    }
}

static void jet_draw_panel(const char *word)
{
    cv_text((240 - text_w(&FONT_L, word)) / 2, 50, &FONT_L, word, C_ACC);
}

#define DUB_DRAW_SINE jet_wave_sine_q64
#define DUB_DRAW_COPY_STR str_copy
#include "dub_draw_space.h"

static void jet_draw_space(void)
{
    dub_params_t dp;
    const jet_patch_t *p = &g.cfg.patch;
    uint8_t *b = (uint8_t *)&dp;
    uint32_t i;

    for (i = 0u; i < (uint32_t)sizeof dp; i++)
        b[i] = 0u;
    dp.dly_time = p->dly_time;
    dp.feedback = p->dly_fb;
    dp.loop_filt = (uint8_t)(64 + p->dly_tone);
    dp.wow = p->wow;
    dp.tap = p->tap;
    dp.tank = p->tank ? 127u : 0u;
    dp.isolator = (uint8_t)(64 + p->iso);
    dp.dly_lvl = p->dly_lvl;
    dp.rev_mix = p->rev_mix;
    dp.rev_damp = p->rev_hi;
    dp.rev_tail = p->rev_size;
    dp.dly_rev = p->dly_rev;
    dp.bpm = g.cfg.groove.bpm;
    dub_draw_view_space(&dp, 0, fm1_ms, 0, JET_STAGE[VIEW_FX][jet_bank_page() % 4u]);
}

static void jet_draw_groove(void)
{
    fm_groove_plot_t gp = {0};
    uint8_t sc = g.cfg.groove.scale % JET_SCALE_N;

    gp.page = jet_bank_page() ? 1u : 0u;
    gp.bpm = g.cfg.groove.bpm;
    gp.swing = g.cfg.groove.swing;
    gp.root = g.cfg.groove.root;
    gp.scale = sc ? (uint8_t)(sc - 1u) : 0u;
    gp.no_ring = sc ? 0u : 1u;
    gp.playing = g.playing;
    gp.step = (uint8_t)(g.play_step % 16u);
    gp.step_frac256 = 0u;
    gp.clock_mode = g.cfg.groove.clock;
    gp.clock_n = JET_CLK_N;
    gp.midi_ch = 16u; /* plot packs "INT" and the channel without a gap: channel drawn below */
    gp.col = jet_era_col(g.era);
    fm_groove_plot_draw(&gp);
    if (gp.page && g.cfg.groove.midi_ch) {
        char mc[4];

        u8_dec(mc, g.cfg.groove.midi_ch);
        cv_text(236 - text_w(&FONT_S, mc), 4, &FONT_S, mc, C_DIM);
    }
}

static void jet_draw_rec(void)
{
    static const char *const AUTO_LBL[2] = {"LENGTH AS IS", "LENGTH = BARS"};
    const char *st = "OFF";
    char cur[12];

    cur[0] = 0;
    if (g.rec_arm) {
        if (g.cfg.rec.mode == KO_REC_MONITOR)
            st = "MONITOR";
        else if (g.playing)
            st = "LIVE";
        else {
            st = "STEP";
            u8_dec(cur, (uint32_t)g.step_cur + 1u);
        }
    }
    cv_text(8, 8, &FONT_L, st, g.rec_arm ? RGB(255, 48, 48) : C_DIM);
    if (cur[0])
        cv_text(16 + text_w(&FONT_L, st), 8, &FONT_L, cur, C_WHITE);
    cv_text(8, 44, &FONT_S, REC_MODE_LBL[g.cfg.rec.mode % 3u], C_WHITE);
    cv_text(8, 64, &FONT_S, REC_QUANT_LBL[g.cfg.rec.quant % 3u], C_WHITE);
    cv_text(8, 84, &FONT_S, REC_BARS_LBL[g.cfg.rec.bars % 4u], g.cfg.rec_auto_len ? C_WHITE : C_DIM);
    cv_text(8, 104, &FONT_S, AUTO_LBL[g.cfg.rec_auto_len ? 1u : 0u], C_WHITE);
}

/* The step the knobs and the top bar talk about: latched step, step-record cursor, else the playhead. */
static uint8_t jet_seq_focus_step(void)
{
    if (g.seq_plock_latch && g.seq_plock_step < seq_len())
        return g.seq_plock_step;
    if (jet_rec_stepping())
        return g.step_cur;
    if (g.playing)
        return jet_ui_step();
    return 255u;
}

/* One-lane piano roll on the shared step grid: height = pitch (fitted to the pattern), width = gate,
 * brightness = velocity, white = accent, nose to the left = slide, bridge = tie. */
static void jet_draw_seq(void)
{
    static const uint8_t outline[1] = {1u};
    fm_seq_cell_t cells[FM_SEQ_PAGE_STEPS];
    const jet_seq_t *sq = &g.cfg.seq;
    uint32_t st;
    uint8_t base = (uint8_t)(g.seq_page * FM_SEQ_PAGE_STEPS);
    uint8_t ln = seq_len();
    uint8_t ui = jet_ui_step();
    uint8_t mark = 255u;
    int32_t lo = 127;
    int32_t hi = 0;
    int32_t span;
    char nm[8];

    for (st = 0u; st < ln; st++) {
        int32_t n = sq->note[st];

        if (!n)
            continue;
        if (n < lo)
            lo = n;
        if (n > hi)
            hi = n;
    }
    if (hi < lo) {
        lo = 60;
        hi = 72;
    }
    span = hi - lo;
    if (span < 12) {
        lo -= (12 - span) / 2;
        if (lo < 0)
            lo = 0;
        span = 12;
        hi = lo + span;
    }
    for (st = 0u; st < FM_SEQ_PAGE_STEPS; st++) {
        uint8_t ix = (uint8_t)(base + st);
        fm_seq_cell_t *c = &cells[st];

        c->col = 0u;
        c->bg_col = ix >= ln ? C_BLACK : sq->note[ix] ? RGB(30, 50, 70) : (st & 3u) ? RGB(20, 28, 44) : RGB(34, 44, 64);
        c->vel = 0u; /* bodies drawn below */
        c->micro = 0;
        c->gate = 0;
        c->note_span = 1u;
        c->note_tail = 0u;
        c->tune = 0;
        c->cond = 0u;
    }
    if (g.seq_plock_latch)
        mark = g.seq_plock_step;
    else if (jet_rec_stepping())
        mark = g.step_cur;
    mark = (uint8_t)(mark >= base && mark < base + FM_SEQ_PAGE_STEPS ? mark - base : 255u);
    fm_seq_draw_grid(1u, 0u, (uint8_t)(ui >= base && ui < base + FM_SEQ_PAGE_STEPS ? ui - base : 255u),
                     g.playing ? 1 : 0, mark, g.seq_plock_latch ? 1 : 0, 0u, C_WHITE, 0, 0, cells, 0, 0, outline);
    for (st = 0u; st < FM_SEQ_PAGE_STEPS; st++) {
        uint8_t ix = (uint8_t)(base + st);
        int32_t x = fm_seq_grid_step_x((int32_t)st);
        int32_t y;
        int32_t w;
        uint8_t gt;
        uint8_t tie;
        uint16_t col;

        if (ix >= ln || !sq->note[ix])
            continue;
        gt = sq->gate[ix];
        tie = (uint8_t)((sq->legato[ix] & 1u) && ix > 0u && sq->note[ix - 1u] == sq->note[ix]);
        y = 116 - (((int32_t)sq->note[ix] - lo) * 100) / span;
        w = gt == 0u || gt >= 8u ? 11 : 2 + ((int32_t)gt * 9) / 8;
        x += (int32_t)sq->micro[ix] * 2;
        col = sq->flags[ix] & JET_SEQ_F_ACCENT ? C_WHITE
                                               : color_scale_rgb(C_ACC, 96 + ((int32_t)sq->vel[ix] * 160) / 127);
        cv_rect(x, y, w, 8, col);
        if (tie)
            cv_rect(x - 5, y + 2, 5, 4, col);
        else if (sq->flags[ix] & JET_SEQ_F_SLIDE)
            cv_rect(x - 4, y + 3, 4, 2, C_WHITE);
        if (gt == 0u)
            cv_rect(x + 11, y + 2, 2, 4, col);
        if (sq->pl_push[ix] != FM_SEQ_PLOCK_NONE || sq->pl_lpf[ix] != FM_SEQ_PLOCK_NONE)
            cv_rect(x + 4, y + 10, 3, 2, C_GRAY);
    }
    /* pitch range of the roll, left margin */
    note_fmt(nm, jet_note_sound((uint8_t)hi));
    cv_text(0, 14, &FONT_S, nm, C_DIM);
    note_fmt(nm, jet_note_sound((uint8_t)lo));
    cv_text(0, 112, &FONT_S, nm, C_DIM);
}

static void jet_draw_canvas(void)
{
    switch (g.view) {
    case VIEW_ENV:
        jet_draw_env();
        break;
    case VIEW_LFO:
        if (jet_bank_page() == 2u)
            jet_draw_panel("EXPR");
        else
            fm_lfo_slots_draw_view(g.cfg.lfo, g.lfo_rt, g.cfg.lfo_tgt, JET_LFO_TGT_N, JET_LFO_NAME, C_ACC);
        break;
    case VIEW_SEQ:
        if (g.seq_knob_page && !g.seq_plock_latch)
            jet_draw_rec();
        else
            jet_draw_seq();
        break;
    case VIEW_GROOVE:
        jet_draw_groove();
        break;
    case VIEW_FX:
        jet_draw_space();
        break;
    default:
        jet_draw_scope();
        break;
    }
}

static void draw_screen(void)
{
    palette_set(2u);
    jet_draw_top();
    cv_begin(240, 132, C_BLACK);
    jet_draw_canvas();
    ko_draw_toast(&g.toast, fm1_ms); /* into the canvas, so before the blit */
    cv_blit(0, 28);
    jet_draw_knobs();
}

static void jet_go_view(uint8_t v)
{
    if (v >= VIEW_COUNT)
        v = VIEW_HOME;
    if (v != VIEW_SEQ && g.seq_plock_latch)
        jet_plock_latch_end();
    g.view = v;
    mark_dirty();
}

static void jet_groove_reset_plate(void)
{
    jet_plate_groove_load();
    g.cfg.groove.scale = 0u;
    transport_set_spp();
}

static uint8_t jet_param_lfo_tgt(uint8_t id)
{
    uint8_t t;

    if (id == JET_P_TUNE || id == JET_P_FINE)
        return JET_LFO_PITCH;
    for (t = 1u; t < JET_LFO_TGT_N; t++) {
        if (JET_LFO_PARAM[t] == id)
            return t;
    }
    return JET_LFO_TGT_N;
}

static void jet_lfo_reset_slot(void)
{
    fm_lfo_slot_reset(&g.cfg.lfo[g.cfg.lfo_tgt % JET_LFO_TGT_N]);
    mark_dirty();
}

static void jet_bank_reset(uint8_t k)
{
    uint8_t pg = (uint8_t)(g.cfg.meta.page[k] % JET_BANK_PAGES[k]);
    uint32_t c;

    if (k == VIEW_LFO && pg < 2u)
        jet_lfo_reset_slot();
    else if (k == VIEW_GROOVE && pg == 0u)
        jet_groove_reset_plate();
    else if (k == VIEW_GROOVE) {
        g.cfg.groove.midi_ch = 0u;
        g.cfg.groove.clock = JET_CLK_INT;
        g.cfg.groove.bend = 2u;
        g.keyboard_oct = 0;
    } else {
        jet_patch_t pl;
        uint32_t q;

        jet_plate_patch(&pl);
        for (q = 0u; q < JET_BANK_PAGES[k]; q++) {
            for (c = 0u; c < 4u; c++) {
                uint8_t id = JET_PAGE[k][q][c];

                if (id < JET_P_N)
                    jet_param_set(&g.cfg.patch, id, jet_param_get(&pl, id));
            }
        }
    }
    jet_flash_param("RESET");
    mark_dirty();
}

static void jet_bank_press(uint8_t k)
{
    if (k >= JET_BANK_N)
        return;
    if (fm1_in.buttons & (1u << BTN_EDIT)) {
        jet_bank_reset(k);
        return;
    }
    if (g.view == k)
        g.cfg.meta.page[k] = (uint8_t)((g.cfg.meta.page[k] + 1u) % JET_BANK_PAGES[k]);
    else
        jet_go_view(k);
    mark_dirty();
}

static void jet_flash_bar(void)
{
    char msg[8];

    str_copy(msg, "BAR ");
    u8_dec(msg + 4, (uint32_t)g.seq_page + 1u);
    jet_flash_param(msg);
}

static void jet_seq_goto_bar(int32_t p)
{
    int32_t pages = (int32_t)jet_seq_pages(&g.cfg.seq);

    if (pages < 1)
        pages = 1;
    if (p < 0)
        p = 0;
    if (p > pages - 1)
        p = pages - 1;
    g.seq_page = (uint8_t)p;
    jet_flash_bar();
    mark_dirty();
}

static void jet_cc_apply(uint8_t cc, uint8_t val)
{
    switch (cc) {
    case 20:
        g.cfg.patch.blow = val;
        break;
    case 21:
        g.cfg.patch.push = (int8_t)((int32_t)val - 64);
        break;
    case 22:
        g.cfg.patch.lpf = val;
        break;
    case 23:
        g.cfg.patch.hpf = val;
        break;
    case 24:
        g.cfg.patch.harm = val;
        break;
    case 25:
        g.cfg.patch.sat = val;
        break;
    case 26:
        g.cfg.patch.sat_sym = val;
        break;
    case 27:
        g.cfg.patch.split = val;
        break;
    case 28:
        g.cfg.patch.fb_dec = val;
        break;
    case 29:
        g.cfg.patch.fb_rel = val;
        break;
    case 30:
        g.cfg.patch.dc_noise = val;
        break;
    case 31:
        g.cfg.patch.noise_lpf = val;
        break;
    case 73:
        g.cfg.patch.s_atk = val;
        break;
    case 75:
        g.cfg.patch.s_dec = val;
        break;
    case 79:
        g.cfg.patch.s_sus = val;
        break;
    case 72:
        g.cfg.patch.s_rel = val;
        break;
    case 5:
        g.cfg.patch.glide = val;
        break;
    case 91:
        g.cfg.patch.rev_mix = val;
        break;
    case 94:
        g.cfg.patch.dly_lvl = val;
        break;
    case 7:
        g.cfg.patch.level = val;
        break;
    case 1:
        g.expr.mw = val;
        break;
    case 2:
        g.expr.breath = val;
        break;
    case 64:
        g.expr.sustain = val >= 64u ? 127u : 0u;
        break;
    case 14:
    case 15:
    case 16:
    case 17:
        jet_cc_knob_abs((uint8_t)(cc - 14u), val);
        break;
    default:
        return;
    }
    mark_dirty();
}


static void jet_handle_midi(void)
{
    uint8_t st, d1, d2;

    while (midi_get(&st, &d1, &d2)) {
        uint8_t ext = g.cfg.groove.clock == JET_CLK_EXT ? 1u : 0u;

        if (st == 0xF8u && ext) {
            g.ext_clock = 1u;
            if (g.playing)
                jet_ev_push((jet_ev_t){JET_EV_CLK, 0u, 0u, 0u});
        } else if (st == 0xFAu && ext) {
            g.ext_clock = 1u;
            g.playing = 0u;
            jet_transport_start(1u);
        } else if (st == 0xFBu && ext) {
            g.ext_clock = 1u;
            if (!g.playing)
                jet_transport_start(0u);
        } else if (st == 0xFCu && ext) {
            if (g.playing)
                jet_transport_stop();
        } else if ((st & 0xF0u) == 0x90u && d2 && jet_midi_ch_ok(st)) {
            jet_live_on(d1, d2);
            jet_rec_note_on(d1, d2);
        } else if (((st & 0xF0u) == 0x80u || ((st & 0xF0u) == 0x90u && !d2)) && jet_midi_ch_ok(st)) {
            jet_live_off(d1);
            jet_rec_note_off(d1);
        } else if ((st & 0xF0u) == 0xE0u && jet_midi_ch_ok(st)) {
            int32_t bend = (int32_t)d1 | ((int32_t)d2 << 7);
            int32_t cents = (bend - 8192) * (int32_t)g.cfg.groove.bend * 100 / 8192;

            g.expr.bend_cents = (int16_t)cents;
        } else if ((st & 0xF0u) == 0xD0u && jet_midi_ch_ok(st)) {
            g.expr.at = d1;
        } else if ((st & 0xF0u) == 0xB0u && jet_midi_ch_ok(st))
            jet_cc_apply(d1, d2);
    }
}

static void jet_enc_knobs(void)
{
    uint32_t col;
    int32_t sel = fm1_enc_take(ENC_SELECT);
    int32_t alg = fm1_enc_take(ENC_ALGORITHM);
    int32_t pre = fm1_enc_take(ENC_PRESETS);
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) ? 1u : 0u;
    uint8_t lfoheld = (fm1_in.buttons & (1u << BTN_LFO)) ? 1u : 0u;

    if (fm1_in.buttons & (1u << BTN_SEQ)) {
        char msg[8];

        if (pre) {
            adj_u8(&g.cfg.groove.root, pre > 0 ? 1 : -1, 0u, 11u);
            str_copy(msg, "KEY ");
            str_copy(msg + 4, NOTE_NAMES[g.cfg.groove.root % 12u]);
            jet_flash_param(msg);
        }
        if (alg) {
            adj_u8(&g.cfg.groove.scale, alg > 0 ? 1 : -1, 0u, (uint8_t)(JET_SCALE_N - 1u));
            jet_flash_param(JET_SCALE_NAME[g.cfg.groove.scale % JET_SCALE_N]);
        }
        if (sel) {
            adj_u8(&g.cfg.groove.bpm, sel > 0 ? 1 : -1, JET_BPM_MIN, JET_BPM_MAX);
            u8_dec(msg, g.cfg.groove.bpm);
            str_copy(msg + (g.cfg.groove.bpm >= 100u ? 3 : 2), "BPM");
            jet_flash_param(msg);
            transport_set_spp();
        }
        if (pre || alg || sel) {
            g.seq_long_used = 1u;
            mark_dirty();
        }
        pre = alg = sel = 0;
    }
    if (sel && g.view == VIEW_SEQ) {
        if (jet_rec_stepping()) {
            /* step record: SELECT walks the cursor (a step turned past stays as it is: rest or old note) */
            char msg[12];

            jet_step_cur_set((int32_t)g.step_cur + (sel > 0 ? 1 : -1));
            str_copy(msg, "STEP ");
            u8_dec(msg + 5, (uint32_t)g.step_cur + 1u);
            jet_flash_param(msg);
        } else
            jet_seq_goto_bar((int32_t)g.seq_page + (sel > 0 ? 1 : -1));
        sel = 0;
    }
    if (sel) {
        int32_t a = (int32_t)g.air + sel;

        if (a < 0)
            a = 0;
        if (a > 127)
            a = 127;
        g.air = (uint8_t)a;
        {
            char msg[8];

            str_copy(msg, "AIR ");
            u8_dec(msg + 4, g.air);
            jet_flash_param(msg);
        }
        mark_dirty();
    }
    if (alg)
        jet_set_era(alg > 0 ? 1 : -1);
    if (pre)
        jet_set_plate(pre, edit);
    for (col = 0u; col < 4u; col++) {
        int32_t steps = fm1_enc_take((uint8_t)(ENC_KNOB1 + col));

        if (!steps)
            continue;
        steps = ko_knob_gain(&g.knob_ms[col], fm1_ms, steps);
        if (lfoheld)
            g.lfo_long_used = 1u;
        if (lfoheld && g.view != VIEW_LFO) {
            uint8_t tgt = jet_param_lfo_tgt(jet_page_param((uint8_t)col));

            if (tgt < JET_LFO_TGT_N) {
                char v[4];

                g.cfg.lfo_tgt = tgt;
                adj_u8(&g.cfg.lfo[tgt].depth, steps, 0u, 127u);
                u8_dec(v, g.cfg.lfo[tgt].depth);
                jet_flash_kv(JET_LFO_NAME[tgt], v);
                mark_dirty();
                continue;
            }
        }
        jet_adj_param(jet_page_param((uint8_t)col), steps, edit);
    }
}

static void jet_handle_seq_white(uint32_t keys, uint32_t rising, uint32_t released)
{
    uint32_t k;
    uint32_t base = (uint32_t)g.seq_page * FM_SEQ_PAGE_STEPS;

    g.seq_plock_hold = g.seq_plock_latch;
    for (k = 0u; k < 16u; k++) {
        uint32_t bit = 1u << KEY_WHITE[k];

        if (rising & bit) {
            if (!g.seq_plock_latch) {
                g.seq_plock_step = (uint8_t)(base + k);
            }
            g.white_press_k = (uint8_t)k;
            g.white_press_ms = fm1_ms;
            g.white_long_used = 0u;
            g.white_long_cancel = 0u;
        }
        if (released & bit) {
            if (g.white_press_k == k && !g.white_long_used && !g.white_long_cancel) {
                if (fm1_ms - g.white_press_ms < JET_HOLD_MS) {
                    uint8_t st = (uint8_t)(base + k);
                    uint8_t note = g.seq_last_note ? g.seq_last_note : 60u;

                    if (g.seq_plock_latch && st == g.seq_plock_step)
                        jet_plock_latch_end();
                    else if (!g.seq_plock_latch && st < seq_len()) {
                        /* tap: step on with the last played note (heard while stopped), or off */
                        if (g.cfg.seq.note[st])
                            jet_seq_clear_step(&g.cfg.seq, st);
                        else {
                            jet_seq_write_note(&g.cfg.seq, st, jet_note_store(note), 8u, 100u, 0, 0u);
                            jet_audition(g.cfg.seq.note[st], 100u);
                        }
                        g.seq_last_note = note;
                        mark_dirty();
                    }
                }
            }
            if (g.white_press_k == k) {
                g.white_press_k = 255u;
                g.white_press_ms = 0u;
                g.white_long_used = 0u;
                g.white_long_cancel = 0u;
            }
        }
    }
    (void)keys;
}

static void jet_key_press(uint8_t kb, uint8_t note)
{
    if (g.key_note[kb])
        jet_live_off(g.key_note[kb]);
    g.key_note[kb] = note;
    jet_live_on(note, 100u);
    jet_rec_note_on(note, 100u);
}

/* Releases whatever note this key started, in any view and after OCT changes. */
static void jet_keys_release(uint32_t released)
{
    uint8_t kb;

    for (kb = 0u; kb < sizeof g.key_note; kb++) {
        uint8_t note = g.key_note[kb];

        if (!(released & (1u << kb)) || !note)
            continue;
        g.key_note[kb] = 0u;
        jet_live_off(note);
        jet_rec_note_off(note);
    }
}

static void jet_white_keys(uint32_t keys, uint32_t rising, uint32_t released)
{
    uint32_t k;

    jet_keys_release(released);
    /* SEQ view: white keys are the 16 steps; with REC armed the whole keyboard plays (and records) notes */
    if (g.view == VIEW_SEQ && !g.rec_arm) {
        jet_handle_seq_white(keys, rising, released);
        return;
    }
    for (k = 0u; k < 16u; k++) {
        if (rising & (1u << KEY_WHITE[k]))
            jet_key_press(KEY_WHITE[k], white_to_midi((uint8_t)k));
    }
    (void)keys;
}

static void jet_black_keys(uint32_t keys, uint32_t rising, uint32_t released)
{
    uint32_t k;

    (void)keys;
    (void)released;
    if (g.view == VIEW_SEQ && !g.rec_arm) {
        for (k = 0u; k < 8u; k++) {
            if ((rising & (1u << KEY_BLACK[k])) && k * FM_SEQ_PAGE_STEPS < seq_len())
                jet_seq_goto_bar((int32_t)k);
        }
        return;
    }
    for (k = 0u; k < 11u; k++) {
        if (rising & (1u << KEY_BLACK[k]))
            jet_key_press(KEY_BLACK[k], black_to_midi((uint8_t)k));
    }
}

static void jet_update_leds(uint32_t keys)
{
    uint32_t k;
    int pulse = ko_led_pulse(fm1_ms);

    uint8_t in_seq = g.view == VIEW_SEQ ? 1u : 0u;

    fm1_led_key(BTN_PLAY, (in_seq && !g.seq_follow) ? (g.playing ? !pulse : pulse) : g.playing);
    /* REC: lit = live record or monitor, pulsing = step record waiting for notes */
    fm1_led_key(BTN_REC, jet_rec_stepping() ? pulse : g.rec_arm);
    fm1_led_key(BTN_SEQ, in_seq);
    fm1_led_key(BTN_ARP, 0);
    fm1_led_key(BTN_EDIT, (fm1_in.buttons & (1u << BTN_EDIT)) ? 1 : 0);
    fm1_led_key(BTN_OCT_DOWN, g.keyboard_oct < 0);
    fm1_led_key(BTN_OCT_UP, g.keyboard_oct > 0);
    for (k = 0u; k < JET_BANK_N; k++) {
        uint8_t btn = JET_BANK_BTN[k];
        int active = !in_seq && g.view == k ? 1 : 0;
        int p2 = (g.cfg.meta.page[k] % JET_BANK_PAGES[k]) != 0u ? 1 : 0;
        uint8_t st = ko_bank_led(active, p2, pulse);

        fm1_led_key(btn, st == KO_LED_ON);
        if (st == KO_LED_DIM)
            fm1_led_key_dim(btn, 1);
    }
    if (in_seq) {
        uint32_t base = (uint32_t)g.seq_page * FM_SEQ_PAGE_STEPS;
        uint8_t ln = seq_len();

        for (k = 0u; k < 16u; k++) {
            uint32_t ix = base + k;
            int on = ix < ln && g.cfg.seq.note[ix] != 0u;
            int ph = g.playing && jet_ui_step() == ix;

            if (g.seq_plock_latch && g.seq_plock_step == ix)
                on = 1;
            else if (jet_rec_stepping() && g.step_cur == ix)
                on = pulse; /* step-record cursor */
            else if (ph)
                on = !on;
            fm1_led_key(KEY_LED(KEY_WHITE[k]), on);
        }
        for (k = 0u; k < 11u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), k == g.seq_page);
        return;
    }
    for (k = 0u; k < 16u; k++)
        fm1_led_key(KEY_LED(KEY_WHITE[k]), (keys >> KEY_WHITE[k]) & 1u);
    for (k = 0u; k < 11u; k++)
        fm1_led_key(KEY_LED(KEY_BLACK[k]), (keys >> KEY_BLACK[k]) & 1u);
}

static void app_audio(int32_t *out, uint32_t n)
{
    int32_t l[JET_SUB];
    int32_t r[JET_SUB];
    uint32_t i;

    g.expr_isr = g.expr;
    jet_engine_expr(&g.eng, &g.expr_isr);
    while (n > 0u) {
        uint32_t m = n > JET_SUB ? JET_SUB : n;

        jet_ev_drain_isr();
        jet_clock_subblock();
        fm_lfo_slots_step_tr(g.cfg.lfo, g.lfo_rt, JET_LFO_TGT_N, g.cfg.lfo_tgt, g.cfg.groove.bpm, m, JET_FS, 1u);
        jet_engine_render(&g.eng, l, r, m);
        for (i = 0u; i < m; i++) {
            int32_t pk = l[i] >= 0 ? l[i] : -l[i];

            if (pk > g.out_peak)
                g.out_peak = pk;
            out[i * 2u] = l[i];
            out[i * 2u + 1u] = r[i];
            if ((i & 7u) == 0u) {
                g.scope[g.scope_ix] = (int16_t)(l[i] > 32767 ? 32767 : l[i] < -32767 ? -32767 : l[i]);
                if (++g.scope_ix >= 240u)
                    g.scope_ix = 0u;
            }
        }
        g.out_peak = (int32_t)(g.out_peak - (g.out_peak >> 6));
        out += 2u * m;
        n -= m;
    }
}

static void app_init(void)
{
    int loaded;

    cfg_defaults();
    loaded = settings_load_jet() == 0;
    jet_engine_init(&g.eng);
    g.rec_writer.clear_step = jet_rec_clear;
    g.rec_writer.write_note = jet_rec_write;
    g.rec_writer.snapshot = jet_rec_snapshot;
    g.rec_writer.restore = jet_rec_restore;
    g.rec_writer.set_len = jet_rec_set_len;
    g.rec_writer.erase_step = jet_rec_erase_step;
    ko_rec_init(&g.rec);
    g.rec.cfg = g.cfg.rec;
    g.rec.w = &g.rec_writer;
    g.rec.wctx = &g;
    if (loaded)
        g.eng.patch = g.cfg.patch;
    else
        jet_apply_plate_voice();
    jet_patch_eff_rebuild();
    jet_coef_publish();
    transport_set_spp();
    transport_reset();
    g.view = VIEW_HOME;
    g.seq_follow = 1u;
    g.rec_skip_step = 255u;
    g.white_press_k = 255u;
    g.ui_dirty = 1u;
    draw_screen();
}

static void app_step(void)
{
    uint32_t b, rising, released, key_up, keys = fm1_in.notes;

    platform_poll();
    g.rec.cfg = g.cfg.rec;
    g.rec.playing = g.playing;
    g.rec.armed = g.rec_arm;
    b = fm1_input_edges(&released);
    rising = keys & ~g.prev_keys;
    key_up = g.prev_keys & ~keys;
    g.prev_keys = keys;

    if (b & (1u << BTN_REC)) {
        g.rec_press_ms = fm1_ms;
        g.rec_long_used = 0u;
        if ((fm1_in.buttons & (1u << BTN_EDIT)) && !g.playing) {
            jet_adj_param(JET_P_REC_UNDO, 1, 0u);
            jet_flash_param("UNDO");
            g.rec_long_used = 1u;
        }
    }
    {
        ko_longpress_t lp = {&g.rec_press_ms, &g.rec_long_used, 0};

        if ((released & (1u << BTN_REC)) && !g.rec_long_used) {
            g.rec_arm = g.rec_arm ? 0u : 1u;
            g.rec.armed = g.rec_arm;
            if (g.rec_arm) {
                jet_rec_take_now();
                if (g.cfg.rec.mode == KO_REC_MONITOR)
                    jet_flash_param("MONITOR");
                else
                    jet_flash_param(g.playing ? "LIVEREC" : "STEPREC");
            } else
                ko_rec_voice_reset(&g.rec_voice);
            mark_dirty();
        }
        if ((fm1_in.buttons & (1u << BTN_REC)) && g.rec_press_ms &&
            ko_longpress_poll(&lp, fm1_ms, JET_LONG_MS, 0)) {
            jet_go_view(VIEW_SEQ);
            g.seq_knob_page = 1u;
        }
    }
    if (b & (1u << BTN_PLAY)) {
        if (!g.playing) {
            if (g.cfg.groove.clock == JET_CLK_SEND)
                midi_send(0xFAu, 0, 0);
            jet_transport_start(1u);
        } else {
            jet_transport_stop();
            if (g.cfg.groove.clock == JET_CLK_SEND) {
                jet_midi_clk_drain();
                midi_send(0xFCu, 0, 0);
            }
        }
        mark_dirty();
    }
    if (b & (1u << BTN_HOME))
        jet_bank_press(VIEW_HOME);
    if (b & (1u << BTN_SEL))
        jet_bank_press(VIEW_SEL);
    if (b & (1u << BTN_ENV))
        jet_bank_press(VIEW_ENV);
    if (b & (1u << BTN_GLO))
        jet_bank_press(VIEW_GROOVE);
    if (b & (1u << BTN_FX)) {
        g.fx_press_ms = fm1_ms;
        g.fx_long_used = 0u;
    }
    {
        ko_longpress_t lp = {&g.fx_press_ms, &g.fx_long_used, 0};

        if ((fm1_in.buttons & (1u << BTN_FX)) && g.fx_press_ms)
            (void)ko_longpress_poll(&lp, fm1_ms, JET_LONG_MS, 0);
        if ((released & (1u << BTN_FX)) && !g.fx_long_used)
            jet_bank_press(VIEW_FX);
    }
    if (b & (1u << BTN_LFO)) {
        g.lfo_press_ms = fm1_ms;
        g.lfo_long_used = 0u;
    }
    /* long hold resets the slot on release, unless a knob was turned while held (hold-and-turn = depth) */
    if ((released & (1u << BTN_LFO)) && g.lfo_press_ms) {
        if (!g.lfo_long_used) {
            if (fm1_ms - g.lfo_press_ms >= JET_LONG_MS) {
                jet_lfo_reset_slot();
                jet_flash_param("LFO RST");
            } else
                jet_bank_press(VIEW_LFO);
        }
        g.lfo_press_ms = 0u;
    }
    if (b & (1u << BTN_SEQ)) {
        g.seq_press_ms = fm1_ms;
        g.seq_long_used = 0u;
        g.seq_was_in_seq = (uint8_t)(g.view == VIEW_SEQ);
    }
    {
        ko_longpress_t lp = {&g.seq_press_ms, &g.seq_long_used, 0};

        if ((fm1_in.buttons & (1u << BTN_SEQ)) && g.seq_press_ms &&
            ko_longpress_poll(&lp, fm1_ms, JET_LONG_MS, 0)) {
            g.seq_follow = g.seq_follow ? 0u : 1u;
            jet_flash_param(g.seq_follow ? "FOLLOW" : "FREE");
            mark_dirty();
        }
        if ((released & (1u << BTN_SEQ)) && !g.seq_long_used) {
            if (!g.seq_was_in_seq) {
                jet_go_view(VIEW_SEQ);
                g.seq_knob_page = 0u;
            } else if (g.seq_plock_latch)
                g.seq_plock_bank ^= 1u;
            else
                g.seq_knob_page ^= 1u;
            mark_dirty();
        }
    }
    if (released & ((1u << BTN_OCT_DOWN) | (1u << BTN_OCT_UP))) {
        char msg[8];

        if (released & (1u << BTN_OCT_DOWN))
            adj_i8(&g.keyboard_oct, -1, -3, 3);
        if (released & (1u << BTN_OCT_UP))
            adj_i8(&g.keyboard_oct, 1, -3, 3);
        str_copy(msg, "OCT ");
        oct_fmt(msg + 4, g.keyboard_oct);
        jet_flash_param(msg);
        mark_dirty();
    }
    if (b & (1u << BTN_SAVE)) {
        g.save_press_ms = fm1_ms;
        g.save_long_used = 0u;
    }
    {
        ko_longpress_t lp = {&g.save_press_ms, &g.save_long_used, 0};
        uint8_t factory = (fm1_in.buttons & (1u << BTN_EDIT)) ? 1u : 0u;

        if ((fm1_in.buttons & (1u << BTN_SAVE)) && g.save_press_ms &&
            ko_longpress_poll(&lp, fm1_ms, JET_HOLD_MS, 0))
            jet_save_plate_ov(factory);
        /* RAM only; the flash write below still waits for jet_is_quiet() */
        if ((released & (1u << BTN_SAVE)) && !g.save_long_used)
            jet_save_plate_ov(factory);
    }
    if ((fm1_in.buttons & (1u << BTN_EDIT)) && (fm1_in.buttons & (1u << BTN_REC)) && g.playing) {
        ko_rec_erase(&g.rec, g.play_step);
        g.rec_long_used = 1u;
    }

    jet_white_keys(keys, rising, key_up);
    jet_black_keys(keys, rising, key_up);
    jet_plock_latch_touch_break(b, rising);
    jet_white_plock_latch_poll(keys, rising);
    jet_handle_midi();
    jet_midi_clk_drain();
    jet_audio_coef_service();
    jet_enc_knobs();
    jet_main_poll_isr_steps();
    jet_audition_poll();
    if (g.playing && g.seq_follow && g.view == VIEW_SEQ && !g.seq_plock_latch) {
        uint8_t fp = (uint8_t)(jet_ui_step() / FM_SEQ_PAGE_STEPS);

        if (fp != g.seq_page) {
            g.seq_page = fp;
            g.ui_dirty = 1u;
        }
    }
    if (g.seq_clear_arm && fm1_ms - g.seq_clear_ms > 1500u) {
        g.seq_clear_arm = 0u;
        g.ui_dirty = 1u;
    }
    jet_groove_commit();
    jet_patch_sanitize(&g.cfg.patch);
    jet_update_leds(keys);

    if (g.ui_dirty || fm1_ms - g.ui_ms > 40u) {
        draw_screen();
        g.ui_dirty = 0u;
        g.ui_ms = fm1_ms;
    }
    if (g.set_dirty && fm1_ms - g.set_t > 2000u && jet_is_quiet()) {
        if (settings_save_jet() != 0)
            jet_toast("FULL", 0u);
    }
}

static void app_main(void)
{
    app_init();
#if defined(DINGHY_HOST)
    for (;;) {
        app_step();
        if (host_sim_should_stop())
            break;
    }
    host_sim_finalize();
#else
    for (;;)
        app_step();
#endif
}

_Static_assert(sizeof(jet_cfg_t) < SETTINGS_MAX, "jet_cfg fits");
_Static_assert(sizeof(jet_plate_ov_t) * JET_PLATE_N < SETTINGS_MAX, "plate ov");
_Static_assert(JET_SETTINGS_N <= SETTINGS_MAX, "jet store save length");
_Static_assert(sizeof jet_store_buf == JET_SETTINGS_N, "jet store buf");
_Static_assert((sizeof(jet_cfg_t) * 129u + 127u) / 128u + sizeof(deck_store_hdr_t) <= JET_SETTINGS_N,
               "jet store worst-case RLE without plate overrides fits");
#if JET_SETTINGS_N >= SETTINGS_MAX
_Static_assert(((sizeof(jet_cfg_t) + sizeof jet_plate_ov) * 129u + 127u) / 128u + sizeof(deck_store_hdr_t) <=
                   JET_SETTINGS_N,
               "jet store worst-case RLE fits");
#endif

