/* SPDX-License-Identifier: GPL-3.0-only */
#define APP_NAME "KO KIT"
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_56413"
#endif
#include "platform.c"
#include <stddef.h>
#define C_ACC RGB(180, 140, 255)
#define KO_POT_TICK_CAP 32u
#define DECK_BPM_MIN 40u
#define DECK_BPM_MAX 200u
#define DECK_BPM_DEF 120u
#include "ko_chrome.h"
#include "perf_toast_policy.h"
#include "ko_era_plate.h"
#include "fm_lfo.h"
#include "deck_kit.h"
#define FM_SEQ_LANE_LBL_W 20
#define FM_SEQ_GRID_X0 22
/* 8 lanes from y 2, last band ends y 113 < knob bar (local y 126). */
#define FM_SEQ_GRID8_Y0 2
#define FM_SEQ_GRID8_ROW_H 14
#define FM_SEQ_GRID8_CELL_H 10
#include "fm_seq.h"
#include "deck_dsp.h"
#include "deck_plates.h"
#include "deck_seq.h"
#include "deck_seq_ui.h"
#include "deck_ui.h"
#include "deck_store.h"

#define DECK_CFG_MAGIC 0x444B3131u   /* DK11 — + env delay/retrig/invert */
#define DECK_STORE_MAGIC 0x444B5A38u /* DKZ8 — RLE blob: meta (+key, scale), bank (+accent_cut, lane seq mode, vary), plock, lfo, plate overrides, lane nudge */
#define DECK_STORE_MAGIC_V7 0x444B5A37u /* DKZ7 — same segments, meta without key/scale */
#define DECK_STORE_MAGIC_V6 0x444B5A36u /* DKZ6 — same segments without lane nudge (nudge loads as 0) */
#define DECK_STORE_MAGIC_V5 0x444B5A35u /* DKZ5 — same segments, lane without vary */
#define DECK_STORE_MAGIC_V4 0x444B5A34u /* DKZ4 — same segments, lane without euclid_hits/pattern_origin/euclid_rot */
#define DECK_STORE_MAGIC_V3 0x444B5A33u /* DKZ3 — same segments, cfg without dly_send/rev_send */
#define DECK_HOLD_MS 400u

enum {
    DECK_VIEW_HOME = 0,
    DECK_VIEW_SEQ = 1
};

enum {
    DECK_BANK_SEL = 0,
    DECK_BANK_ENV,
    DECK_BANK_GLO,
    DECK_BANK_FX,
    DECK_BANK_LFO,
    DECK_BANK_ARP,
    DECK_BANK_HOME,
    DECK_BANK_N
};

enum {
    DECK_LFO_LEVEL = 0,
    DECK_LFO_TUNE,
    DECK_LFO_DECAY,
    DECK_LFO_FILTER,
    DECK_LFO_DRIVE,
    DECK_LFO_PAN,
    DECK_LFO_CRUSH,
    DECK_LFO_MFILT,
    DECK_LFO_PITCH,
    DECK_LFO_NOISE,
    DECK_LFO_ATTACK,
    DECK_LFO_HOLD,
    DECK_LFO_PDEC,
    DECK_LFO_CURVE,
    DECK_LFO_VEL,
    DECK_LFO_FXDRIVE,
    DECK_LFO_FXCRUSH,
    DECK_LFO_FXFILTER,
    DECK_LFO_WIDTH,
    DECK_LFO_COMP,
    DECK_LFO_DUST,
    DECK_LFO_TAPE,
    DECK_LFO_ACCENT,
    DECK_LFO_HUMAN,
    DECK_LFO_PTUNE,
    DECK_LFO_PDECAY,
    DECK_LFO_PFILTER,
    DECK_LFO_PCRUSH,
    DECK_LFO_DENS,
    DECK_LFO_FILL,
    DECK_LFO_STUT,
    DECK_LFO_SUST,
    DECK_LFO_RELEASE,
    DECK_LFO_DELAY,
    DECK_LFO_TIMBR,
    DECK_LFO_TGT_N_V4, /* DKZ4 blobs before the send/FX/ramp targets stop here */
    DECK_LFO_DUCK = DECK_LFO_TGT_N_V4,
    DECK_LFO_DSEND,
    DECK_LFO_RSEND,
    DECK_LFO_EMIX,
    DECK_LFO_ETIME,
    DECK_LFO_EFB,
    DECK_LFO_ETONE,
    DECK_LFO_WOW,
    DECK_LFO_TANK,
    DECK_LFO_ISO,
    DECK_LFO_REV,
    DECK_LFO_RDAMP,
    DECK_LFO_RTAIL,
    DECK_LFO_BLEED,
    DECK_LFO_ARAMP,
    DECK_LFO_TGT_N
};

#define DECK_LFO_STORE_N DECK_LFO_TGT_N

#include "deck_dj.h" /* needs DECK_LFO_* and deck_seq.h; before deck_app_t */

static const uint8_t DECK_BANK_BTN[DECK_BANK_N] = {
    BTN_SEL, BTN_ENV, BTN_GLO, BTN_FX, BTN_LFO, BTN_ARP, BTN_HOME
};
/*
 * Bank split (follows KO BASS, not Dub/House genre):
 * - Dub/House BTN_ENV → BANK_CHORD is chord *voice* (VOICE/CUTOFF/ATTACK/DECAY…) with CHORD panel,
 *   overlapping BANK_BASS SEL (MODEL/CUTOFF/ATTACK/DECAY). No amp-env diagram.
 * - KO BASS BTN_ENV → BANK_CHORD is DAHDSR + plot; BTN_SEL → BANK_BASS voice/model only.
 * DECK: ENV = time/shape + modulated fm_env_plot; SEL = level/timbre scope (no decay/sweep/curve dupes).
 */
static const uint8_t DECK_BANK_PAGES[DECK_BANK_N] = {3u, 3u, 3u, 5u, 3u, 1u, 2u};
static const char *const DECK_BANK_NAME[DECK_BANK_N] = {
    "VOICE", "ENV", "GROOVE", "FX", "LFO", "REPEAT", "PERFORM"
};
static const char *const DECK_LFO_DEST[DECK_LFO_TGT_N] = {
    "LEVEL", "TUNE", "DECAY", "FILTER", "DRIVE", "PAN", "CRUSH", "M.FILT", "SWEEP", "NOISE", "ATTACK",
    "HOLD",  "P.DEC", "CURVE", "VEL",   "M.DRV", "M.CRS", "M.FLT", "WIDTH", "COMP", "DUST", "TAPE",
    "ACC",   "HUMAN", "P.TUNE", "P.DEC", "P.FLT", "P.CRS", "DENS", "FILL", "STUT", "SUST",
    "REL",   "DELAY", "TIMBRE", "DUCK", "D.SEND", "R.SEND", "E.MIX", "E.TIME", "E.FB", "E.TONE", "WOW",
    "TANK",  "ISO",   "REVERB", "DAMP", "TAIL",  "BLEED", "RAMP"
};

static uint8_t deck_lfo_dest_order[DECK_LFO_TGT_N];
static uint8_t deck_lfo_dest_pos[DECK_LFO_TGT_N];
static fm_lfo_dests_t deck_lfo_dests;
static const char *const DECK_ARP_RATE[5] = {"1/8", "1/16", "1/32", "1/16T", "1/32T"};
static const char *const DECK_CLOCK_NAME[2] = {"INT", "EXT"};
/* KO KIT GLO page 0 (BTN_GLO): KEY / SCALE / SWING / TEMPO. HUMAN / HUMN T moved to GLO page 2. */
static const char *const DECK_GROOVE_P0_LBL[4] = {"KEY", "SCALE", "SWING", "TEMPO"};
static const char *const DECK_GROOVE_P2_LBL[4] = {"HUMAN", "HUMN T", "", ""};
enum { DECK_TAP_N = 6u };

static const char *const DECK_TAP_NAMES[DECK_TAP_N] = {"SINGLE", "DUAL", "GALLOP", "TRIP", "SPREAD", "PING"};

static const char *deck_fx_page_name(uint8_t pg)
{
    switch (pg % 5u) {
    case 0u:
        return "ECHO";
    case 1u:
        return "SPACE";
    case 2u:
        return "REVERB";
    case 3u:
        return "DRIVE";
    default:
        return "MIX";
    }
}

typedef struct {
    uint32_t magic;
    uint8_t bpm;
    uint8_t swing;
    uint8_t cur_pat;
    uint8_t cur_voice;
    uint8_t bar_page;
    uint8_t seq_page;
    uint8_t bank;
    uint8_t page[DECK_BANK_N];
    uint8_t midi_ch;
    uint8_t clock;
    uint8_t accent_amt;
    uint8_t human;
    uint8_t arp_rate;
    uint8_t arp_gate;
    uint8_t arp_ramp;
    uint8_t arp_latch;
    uint8_t fx_bypass;
    uint8_t era;
    uint8_t plate;
    uint16_t solo_mask;
    uint8_t lane_mute[DECK_NVOICES];
    deck_voice_cfg_t cfg[DECK_NVOICES];
    deck_master_t master;
    deck_perf_t perf;
    uint8_t env_delay[DECK_NVOICES];
    uint16_t env_retrig_mask;
    uint16_t env_invert_mask;
    uint8_t key;   /* KIT GLO KEY 0..11 */
    uint8_t scale; /* KIT GLO SCALE 0..DECK_SCALE_N-1 */
} deck_meta_t;

/* Blobs DKZ3..DKZ7 carry the meta prefix up to (excluding) key/scale; loaders decode only this many bytes. */
#define DECK_META_V7_N ((uint32_t)offsetof(deck_meta_t, key))

/* User version of one factory plate (SAVE); used = 0 → factory. */
typedef struct {
    uint8_t used;
    uint8_t era;
    uint8_t bpm;
    uint8_t swing;
    deck_voice_cfg_t cfg[DECK_NVOICES];
    deck_master_t master;
    deck_perf_t perf;
    uint8_t env_delay[DECK_NVOICES];
    uint16_t env_retrig_mask;
    uint16_t env_invert_mask;
} deck_plate_ov_t;

/* Frozen DKZ3 layouts — load-time migration only, never extend. */
typedef struct {
    uint8_t b[offsetof(deck_voice_cfg_t, dly_send)];
} deck_voice_cfg_v3_t;

typedef struct {
    uint32_t magic;
    uint8_t bpm;
    uint8_t swing;
    uint8_t cur_pat;
    uint8_t cur_voice;
    uint8_t bar_page;
    uint8_t seq_page;
    uint8_t bank;
    uint8_t page[DECK_BANK_N];
    uint8_t midi_ch;
    uint8_t clock;
    uint8_t accent_amt;
    uint8_t human;
    uint8_t arp_rate;
    uint8_t arp_gate;
    uint8_t arp_ramp;
    uint8_t arp_latch;
    uint8_t fx_bypass;
    uint8_t era;
    uint8_t plate;
    uint16_t solo_mask;
    uint8_t lane_mute[DECK_NVOICES];
    deck_voice_cfg_v3_t cfg[DECK_NVOICES];
    deck_master_t master;
    deck_perf_t perf;
    uint8_t env_delay[DECK_NVOICES];
    uint16_t env_retrig_mask;
    uint16_t env_invert_mask;
} deck_meta_v3_t;

typedef struct {
    uint8_t used;
    uint8_t era;
    uint8_t bpm;
    uint8_t swing;
    deck_voice_cfg_v3_t cfg[DECK_NVOICES];
    deck_master_t master;
    deck_perf_t perf;
    uint8_t env_delay[DECK_NVOICES];
    uint16_t env_retrig_mask;
    uint16_t env_invert_mask;
} deck_plate_ov_v3_t;

_Static_assert(sizeof(deck_voice_cfg_v3_t) == 18u, "DKZ3 voice cfg is 18 bytes");
_Static_assert(offsetof(deck_meta_t, cfg) == offsetof(deck_meta_v3_t, cfg), "meta prefix unchanged");
_Static_assert(sizeof(deck_plate_ov_v3_t) <= sizeof(deck_plate_ov_t), "in-place plate migration grows");

/* Frozen DKZ4 bank layout (lane before the seq-mode fields). DKZ3 bank bytes are the same. */
typedef struct {
    uint8_t hit[FM_SEQ_MASK_BYTES];
    uint8_t accent[FM_SEQ_MASK_BYTES];
    uint8_t accent_cut[FM_SEQ_MASK_BYTES];
    uint8_t length;
    uint8_t div;
    int8_t vel_rng;
    int8_t trig;
    int8_t gate_rng;
    int8_t pitch_rng;
} deck_lane_v4_t;

typedef struct {
    deck_lane_v4_t lane[DECK_NVOICES];
    char name[9];
} deck_pattern_v4_t;

typedef struct {
    deck_pattern_v4_t p[DECK_NPAT];
} deck_bank_v4_t;

/* Frozen DKZ5 layout (lane before vary). */
typedef struct {
    uint8_t hit[FM_SEQ_MASK_BYTES];
    uint8_t accent[FM_SEQ_MASK_BYTES];
    uint8_t accent_cut[FM_SEQ_MASK_BYTES];
    uint8_t length;
    uint8_t div;
    int8_t vel_rng;
    int8_t trig;
    int8_t gate_rng;
    int8_t pitch_rng;
    uint8_t euclid_hits;
    uint8_t pattern_origin;
    int8_t euclid_rot;
} deck_lane_v5_t;

typedef struct {
    deck_lane_v5_t lane[DECK_NVOICES];
    char name[9];
} deck_pattern_v5_t;

typedef struct {
    deck_pattern_v5_t p[DECK_NPAT];
} deck_bank_v5_t;

/* DKZ4 and DKZ5 blobs are decoded one at a time, so both layouts share the same buffers.
 * Migration-only (~14.7 KB): on the device it lives in .pool (app.ld), outside the .data/.bss budget. */
#if defined(DINGHY_HOST) || defined(DINGHY_WEB)
#define DECK_MIGRATE_POOL
#else
#define DECK_MIGRATE_POOL __attribute__((section(".pool")))
#endif
static union {
    deck_bank_v4_t v4;
    deck_bank_v5_t v5;
} deck_bank_old_buf DECK_MIGRATE_POOL;
static union {
    deck_pattern_v4_t v4;
    deck_pattern_v5_t v5;
} deck_bank_old_ref;

_Static_assert(sizeof(deck_lane_v5_t) == sizeof(deck_lane_v4_t) + 3u, "DKZ5 lane = DKZ4 lane + seq mode fields");
_Static_assert(sizeof(deck_lane_t) == sizeof(deck_lane_v5_t) + 1u, "DKZ6 lane = DKZ5 lane + vary");
_Static_assert(sizeof(deck_bank_v4_t) == DECK_NPAT * sizeof(deck_pattern_v4_t), "bank v4 XOR ref stride");
_Static_assert(sizeof(deck_bank_v5_t) == DECK_NPAT * sizeof(deck_pattern_v5_t), "bank v5 XOR ref stride");

typedef struct {
    uint8_t view;
    uint8_t bank;
    uint8_t page[DECK_BANK_N];
    uint8_t ui_dirty;
    uint8_t set_dirty;
    uint8_t ext_clock;
    uint8_t midi_clk_div;
    uint32_t audio_pos;    /* samples rendered by app_audio (ext-clock 16th period measurement) */
    uint32_t ext_s16_mark; /* audio_pos at the previous ext-clock 16th, 0 = none yet */
    uint32_t ext_sps;      /* smoothed ext-clock 16th length in samples, 0 = unknown */
    uint8_t midi_ch;
    uint8_t clock;
    uint8_t accent_amt;
    uint8_t human;
    uint8_t arp_rate;
    uint8_t arp_gate;
    uint8_t arp_ramp;
    uint8_t arp_latch;
    uint8_t fx_bypass;
    uint8_t lfo_tgt;
    uint8_t era;
    uint8_t plate;
    uint8_t groove_plate_pending; /* plate/era turn while playing: groove held, plate groove lands on stop */
    uint16_t arp_held;
    uint16_t arp_latch_mask;
    uint8_t arp_ov; /* ARP overlay open (BTN_ARP short): knobs 0..3 = ARP, view underneath unchanged */
    uint8_t arp_phase;
    uint8_t last_voice;
    uint8_t pulse[DECK_NVOICES];
    uint8_t kick_pump;
    uint8_t flash_voice;
    volatile uint8_t midi_out_n;
    uint8_t midi_out_note[DECK_TRIG_Q_MAX];
    uint8_t midi_out_vel[DECK_TRIG_Q_MAX];
    uint32_t prev_keys;
    uint32_t ui_ms;
    uint32_t saved_until;
    uint32_t set_t;
    uint32_t flash_until;
    uint32_t knob_ms[4];
    uint32_t bank_ms[DECK_BANK_N];
    uint8_t bank_used[DECK_BANK_N];
    uint8_t seq_page;
    uint32_t seq_press_ms;
    uint8_t seq_long_used;
    uint8_t seq_was_in_seq;
    uint8_t seq_follow;
    uint32_t rec_press_ms;
    uint32_t save_press_ms;
    uint8_t save_long_used;
    uint8_t seq_plock_latch;
    uint8_t seq_plock_hold;
    uint8_t seq_plock_step;
    uint8_t seq_plock_knob;
    uint8_t seq_plock_bank; /* step overlay: 0=VEL/COND/MICRO/PITCH (P1), 1=GATE/ACCENT/SLIDE/RATCHET (P2) */
    uint8_t seq_plock_page_saved; /* seq_page before p-lock overlay; restored on exit */
    uint8_t seq_ens_sel;
    uint8_t seq_flip_state;
    uint8_t white_press_k;
    uint32_t white_press_ms;
    uint8_t white_long_used;
    uint8_t white_long_cancel;
    uint32_t pad_ms[11];
    uint8_t pad_used[11];
    uint8_t pad_voice[11];
    uint8_t pad_family_voice[11];
    uint8_t last_black_pad;
    ko_flash_t flash;
    fm_lfo_slot_t lfo[DECK_NVOICES][DECK_LFO_TGT_N];
    fm_lfo_rt_t lfo_rt[DECK_NVOICES][DECK_LFO_TGT_N];
    uint8_t env_delay[DECK_NVOICES];  /* BASS delay/retrig/invert, persisted via deck_meta_t */
    uint8_t env_retrig[DECK_NVOICES];
    uint8_t env_invert[DECK_NVOICES];
    deck_dsp_t dsp;
    deck_dj_t dj_app; /* DJ mode (deck_dj.h) */
} deck_app_t;

static deck_app_t g;
static uint32_t deck_rng = 1u;
static deck_plate_ov_t deck_plate_ov[DECK_PLATE_N];
static uint8_t deck_store_buf[SETTINGS_MAX] __attribute__((aligned(4)));

static deck_runtime_t *rt(void);
static void deck_lane_page_follow_voice(deck_runtime_t *r);

static uint8_t deck_bank_pages(uint8_t b)
{
    return DECK_BANK_PAGES[b % DECK_BANK_N];
}

static uint8_t deck_bank_page(void)
{
    uint8_t n = deck_bank_pages(g.bank);

    return (uint8_t)(g.page[g.bank % DECK_BANK_N] % n);
}

/* ARP overlay: live outside SEQ. Knobs 0..3 read/write ARP while it runs; the bank underneath stays put. */
static uint8_t deck_arp_ov_live(void)
{
    return g.arp_ov && g.view != DECK_VIEW_SEQ;
}

static uint8_t deck_lfo_target_hw(uint8_t bank, uint8_t pg, uint8_t col)
{
    if (bank == DECK_BANK_SEL) {
        if (pg == 0u)
            return col == 0u ? DECK_LFO_TIMBR :
                   col == 1u ? DECK_LFO_LEVEL : col == 2u ? DECK_LFO_TUNE : DECK_LFO_FILTER;
        if (pg == 1u)
            return col == 0u ? DECK_LFO_PAN :
                   col == 1u ? DECK_LFO_NOISE : col == 2u ? DECK_LFO_DRIVE : DECK_LFO_CRUSH;
        return col == 0u ? DECK_LFO_VEL :
               col == 1u ? DECK_LFO_DUCK : col == 2u ? DECK_LFO_DSEND : DECK_LFO_RSEND;
    } else if (bank == DECK_BANK_ENV) {
        if (pg == 0u)
            return col == 0u ? DECK_LFO_ATTACK :
                   col == 1u ? DECK_LFO_DECAY :
                   col == 2u ? DECK_LFO_SUST : DECK_LFO_RELEASE;
        if (pg == 1u)
            return col == 0u ? DECK_LFO_DELAY :
                   col == 1u ? DECK_LFO_HOLD : col == 2u ? DECK_LFO_CURVE : 255u;
        return col == 1u ? DECK_LFO_PDEC : col == 2u ? DECK_LFO_PITCH : 255u;
    } else if (bank == DECK_BANK_GLO && pg == 2u) {
        return col == 0u ? DECK_LFO_HUMAN : 255u;
    } else if (bank == DECK_BANK_GLO && pg == 1u) {
        return col == 0u ? DECK_LFO_ACCENT : 255u;
    } else if (bank == DECK_BANK_FX) {
        if (pg == 0u)
            return (uint8_t)(DECK_LFO_EMIX + col);
        if (pg == 1u)
            return col == 0u ? DECK_LFO_WOW : col == 2u ? DECK_LFO_TANK : col == 3u ? DECK_LFO_ISO : 255u;
        if (pg == 2u)
            return (uint8_t)(DECK_LFO_REV + col);
        if (pg == 3u)
            return (uint8_t)(DECK_LFO_FXDRIVE + col);
        if (pg == 4u)
            return (uint8_t)(DECK_LFO_COMP + col);
    } else if (bank == DECK_BANK_ARP) {
        return col == 2u ? DECK_LFO_ARAMP : 255u;
    } else if (bank == DECK_BANK_HOME) {
        if (pg == 0u)
            return (uint8_t)(DECK_LFO_PTUNE + col);
        if (pg == 1u) {
            if (col == 2u)
                return 255u;
            return col == 0u ? DECK_LFO_DENS : col == 1u ? DECK_LFO_FILL : DECK_LFO_STUT;
        }
    }
    return 255u;
}

static void deck_lfo_collect_hw_knob(uint8_t bank, uint8_t pg, uint8_t col)
{
    uint8_t t = deck_lfo_target_hw(bank, pg, col);

    if (t != 255u)
        (void)fm_lfo_dests_add(&deck_lfo_dests, t);
}

static void deck_lfo_order_build(void)
{
    static const uint8_t walk_banks[] = {DECK_BANK_SEL, DECK_BANK_ENV, DECK_BANK_GLO, DECK_BANK_FX, DECK_BANK_ARP,
                                         DECK_BANK_HOME};
    uint8_t bi, pg, col;

    fm_lfo_dests_init(&deck_lfo_dests, deck_lfo_dest_order, deck_lfo_dest_pos, (uint8_t)DECK_LFO_TGT_N);
    for (bi = 0u; bi < sizeof walk_banks / sizeof walk_banks[0]; bi++) {
        uint8_t bank = walk_banks[bi];
        uint8_t pages = deck_bank_pages(bank);

        for (pg = 0u; pg < pages; pg++) {
            for (col = 0u; col < 4u; col++)
                deck_lfo_collect_hw_knob(bank, pg, col);
        }
    }
}

static uint8_t deck_lfo_target_for_knob(uint8_t col)
{
    return deck_lfo_target_hw(g.bank, deck_bank_page(), col);
}

static int deck_lfo_dest_touch(uint8_t t)
{
    return fm_lfo_dests_touch(&deck_lfo_dests, &g.lfo_tgt, t);
}

static uint8_t deck_lfo_tgt_step(uint8_t cur, int32_t delta)
{
    return fm_lfo_dests_step(&deck_lfo_dests, cur, delta);
}

static uint8_t deck_lfo_tgt_knob_val(uint8_t tgt)
{
    return fm_lfo_dests_knob_val(&deck_lfo_dests, (uint8_t)(tgt % DECK_LFO_TGT_N));
}

static uint8_t deck_lfo_vi(void)
{
    return rt()->cur_voice % DECK_NVOICES;
}

static fm_lfo_slot_t *deck_lfo_sl(uint8_t vi)
{
    return g.lfo[vi % DECK_NVOICES];
}

static fm_lfo_rt_t *deck_lfo_rtv(uint8_t vi)
{
    return g.lfo_rt[vi % DECK_NVOICES];
}


static void deck_lfo_init_all(void)
{
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++)
        fm_lfo_slots_init(deck_lfo_sl(v), deck_lfo_rtv(v), DECK_LFO_TGT_N);
}

static uint8_t deck_lfo_mod_u8_v(uint8_t vi, uint8_t base, uint8_t t, uint8_t lo, uint8_t hi)
{
    int32_t d = fm_lfo_slot_delta(deck_lfo_sl(vi), deck_lfo_rtv(vi), t);

    return fm_lfo_clamp_u8((int32_t)base + d, lo, hi);
}

static uint8_t deck_lfo_mod_u8(uint8_t base, uint8_t t, uint8_t lo, uint8_t hi)
{
    return deck_lfo_mod_u8_v(deck_lfo_vi(), base, t, lo, hi);
}

#include "fm_env_plot.h"

static void deck_env_runtime_init(void)
{
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++) {
        g.env_delay[v] = 0u;
        g.env_retrig[v] = 1u;
        g.env_invert[v] = 0u;
        g.dsp.env_invert[v] = 0u;
    }
}

static const deck_plate_ov_t *deck_plate_ov_get(uint8_t pl)
{
    const deck_plate_ov_t *o = &deck_plate_ov[deck_clamp_plate(pl)];

    return o->used ? o : NULL;
}

static void deck_plate_ov_groove(const deck_plate_ov_t *o, uint8_t *bpm, uint8_t *swing)
{
    *bpm = o->bpm < DECK_BPM_MIN || o->bpm > DECK_BPM_MAX ? DECK_BPM_DEF : o->bpm;
    *swing = deck_clamp_swing_u8(o->swing);
}

/* Groove deck_apply_plate would set for the current plate/era. */
static void deck_plate_groove_read(uint8_t *bpm, uint8_t *swing)
{
    const deck_plate_ov_t *o = deck_plate_ov_get(g.plate);

    if (o && deck_clamp_era(o->era) == g.era)
        deck_plate_ov_groove(o, bpm, swing);
    else
        deck_plate_groove(g.plate, g.era, bpm, swing);
}

/* Override applies to its own era only; era knob away from it falls back to factory macros. */
static void deck_apply_plate(deck_runtime_t *r)
{
    const deck_plate_ov_t *o = deck_plate_ov_get(g.plate);
    uint8_t v;

    if (o && deck_clamp_era(o->era) == g.era) {
        g.dsp.era = g.era;
        for (v = 0u; v < DECK_NVOICES; v++) {
            g.dsp.cfg[v] = o->cfg[v];
            g.env_delay[v] = o->env_delay[v] > 127u ? 127u : o->env_delay[v];
            g.env_retrig[v] = (uint8_t)((o->env_retrig_mask >> v) & 1u);
            g.env_invert[v] = (uint8_t)((o->env_invert_mask >> v) & 1u);
            g.dsp.env_invert[v] = g.env_invert[v];
            g.dsp.env_delay[v] = g.env_delay[v];
            g.dsp.env_retrig[v] = g.env_retrig[v];
        }
        g.dsp.master = o->master;
        g.dsp.perf = o->perf;
        if (g.dsp.perf.mute_grp > 4u)
            g.dsp.perf.mute_grp = 0u;
        deck_plate_ov_groove(o, &r->bpm, &r->swing);
        return;
    }
    deck_plate_apply(&g.dsp, &r->bpm, &r->swing, g.plate, g.era);
    for (v = 0u; v < DECK_NVOICES; v++) {
        g.env_invert[v] = g.dsp.env_invert[v];
        g.env_delay[v] = 0u;
        g.env_retrig[v] = 1u;
        g.dsp.env_delay[v] = 0u;
        g.dsp.env_retrig[v] = 1u;
    }
}

static void deck_plate_ov_capture(deck_plate_ov_t *o)
{
    const deck_runtime_t *r = rt();
    uint8_t v;

    memset(o, 0, sizeof *o);
    o->used = 1u;
    o->era = g.era;
    o->bpm = r->bpm;
    o->swing = r->swing;
    for (v = 0u; v < DECK_NVOICES; v++) {
        o->cfg[v] = g.dsp.cfg[v];
        o->env_delay[v] = g.env_delay[v];
        if (g.env_retrig[v])
            o->env_retrig_mask |= (uint16_t)(1u << v);
        if (g.env_invert[v])
            o->env_invert_mask |= (uint16_t)(1u << v);
    }
    o->master = g.dsp.master;
    o->perf = g.dsp.perf;
}

static void deck_dsp_sync_env_flags(void)
{
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++) {
        g.dsp.env_invert[v] = g.env_invert[v];
        g.dsp.env_delay[v] = g.env_delay[v];
        g.dsp.env_retrig[v] = g.env_retrig[v];
    }
}

static void deck_env_plot_cfg(const deck_voice_cfg_t *bc, uint8_t vi, deck_voice_cfg_t *plot)
{
    vi %= DECK_NVOICES;
    *plot = *bc;
    plot->click = deck_lfo_mod_u8_v(vi, bc->click, DECK_LFO_ATTACK, 0u, 127u);
    plot->hold = deck_lfo_mod_u8_v(vi, bc->hold, DECK_LFO_HOLD, 0u, 127u);
    plot->decay = deck_lfo_mod_u8_v(vi, bc->decay, DECK_LFO_DECAY, 0u, 127u);
    plot->sustain = deck_lfo_mod_u8_v(vi, bc->sustain, DECK_LFO_SUST, 0u, 127u);
    plot->release = deck_lfo_mod_u8_v(vi, bc->release, DECK_LFO_RELEASE, 0u, 127u);
    plot->curve = deck_lfo_mod_u8_v(vi, bc->curve, DECK_LFO_CURVE, 0u, 127u);
    plot->pitch_dec = deck_lfo_mod_u8_v(vi, bc->pitch_dec, DECK_LFO_PDEC, 0u, 127u);
    plot->pitch_env = deck_lfo_mod_u8_v(vi, bc->pitch_env, DECK_LFO_PITCH, 0u, 127u);
}

/* DJ mark on a knob whose LFO dest is touched by DJ (same bank/page gating as deck_lfo_knob_modulated). */
static uint8_t deck_dj_knob_live(uint8_t col)
{
    if (!g.dj_app.dj.on || g.view == DECK_VIEW_SEQ || g.bank == DECK_BANK_LFO)
        return 0u;
    if (g.bank == DECK_BANK_GLO && deck_bank_page() == 0u && col != 0u)
        return 0u;
    if (g.bank == DECK_BANK_FX && fm_dj_fx_knob_overlay_live(&g.dj_app.fx_ov, deck_bank_page(), col))
        return 1u;
    return deck_dj_lt_live(&g.dj_app, deck_lfo_target_for_knob(col));
}

/* Kick DUCK (SEL page 2, col 1) is fixed at 0: no edit, no LFO depth, no LFO display mod. */
static int deck_kick_duck_locked(uint8_t b, uint8_t pg, uint32_t col)
{
    return b == DECK_BANK_SEL && pg == 2u && col == 1u && (rt()->cur_voice % DECK_NVOICES) == DECK_BD;
}

static int deck_lfo_knob_modulated(uint8_t col)
{
    uint8_t lt;

    if (g.view == DECK_VIEW_SEQ || g.bank == DECK_BANK_LFO)
        return 0;
    if (g.bank == DECK_BANK_GLO && deck_bank_page() == 0u && col != 0u)
        return 0;
    lt = deck_lfo_target_for_knob(col);
    if (lt >= DECK_LFO_TGT_N)
        return 0;
    return deck_lfo_sl(deck_lfo_vi())[lt].depth > 0u ? 1 : 0;
}

static uint32_t deck_rand(void)
{
    deck_rng = deck_rng * 1664525u + 1013904223u;
    return deck_rng;
}

static uint32_t deck_boot_seed(void)
{
    uint32_t s = fm1_ms ^ 0x9E3779B9u;

#if !defined(DINGHY_HOST) && !defined(DINGHY_WEB)
    s ^= fm1_ticks();
    s ^= ((uint32_t)fm1_boot.p3_rst << 8) ^ ((uint32_t)fm1_boot.rst_src << 16);
    s ^= (uint32_t)bootguard.pending << 24;
    {
        uint32_t i;

        /* fm1_ms is still 0 here: ADC LSB noise + conversion jitter vs TIMER4 carry the entropy. */
        for (i = 0u; i < 16u; i++) {
            int32_t adc = fm1_adc_read((i & 1u) ? FM1_ADC_BATT : FM1_ADC_MASTER);

            s = s * 1664525u + 1013904223u;
            s ^= (uint32_t)adc * 2654435761u;
            s ^= fm1_ticks();
        }
    }
#else
    s ^= (uint32_t)(uintptr_t)&g;
#endif
    return s ? s : 0xC001D00Du;
}

static void deck_fmt_u8(char *s, uint8_t v)
{
    deck_ui_u8(s, v);
}

static uint8_t deck_bpm_pot_px(uint8_t bpm)
{
    uint32_t b = bpm < DECK_BPM_MIN ? DECK_BPM_MIN : (bpm > DECK_BPM_MAX ? DECK_BPM_MAX : (uint32_t)bpm);

    return (uint8_t)((b - DECK_BPM_MIN) * 127u / (DECK_BPM_MAX - DECK_BPM_MIN));
}


static uint8_t deck_seq_gate(const deck_pattern_t *p, uint8_t pat_ix, uint8_t voice, uint8_t st)
{
    (void)p;
    return deck_seq_step_gate(pat_ix, p, voice, st);
}

static uint8_t deck_seq_slide(const deck_pattern_t *p, uint8_t pat_ix, uint8_t voice, uint8_t st)
{
    (void)p;
    return deck_seq_step_slide(pat_ix, p, voice, st);
}

static const char *deck_seq_lv_name(uint8_t lv)
{
    static const char *const N[4] = {"OFF", "33", "66", "100"};

    return N[lv > DECK_SEQ_LV_FULL ? DECK_SEQ_LV_FULL : lv];
}

static uint8_t deck_seq_ratchet(const deck_pattern_t *p, uint8_t pat_ix, uint8_t voice, uint8_t st)
{
    (void)p;
    return deck_seq_step_ratchet(pat_ix, p, voice, st);
}

static uint8_t deck_seq_cursor_step(const deck_runtime_t *r, const deck_pattern_t *pat);
static void adj_u8(uint8_t *p, int32_t d, int32_t lo, int32_t hi);
static void deck_adj(uint8_t *p, int32_t steps, uint8_t lo, uint8_t hi, uint8_t def, int edit);
static void deck_adj_wrap(uint8_t *p, int32_t dir, uint8_t n);
static void deck_cycle_plate(deck_runtime_t *r, int32_t dir);
static void deck_cycle_era(deck_runtime_t *r, int32_t dir);
static void deck_boot_random_plate(void);

static void deck_str_copy(char *d, const char *s)
{
    uint32_t i;

    for (i = 0u; i < 15u && s[i]; i++)
        d[i] = s[i];
    d[i] = '\0';
}

static void deck_u8_dec(char *d, uint32_t v)
{
    if (v >= 100u) {
        d[0] = (char)('0' + (v / 100u) % 10u);
        d[1] = (char)('0' + (v / 10u) % 10u);
        d[2] = (char)('0' + v % 10u);
        d[3] = '\0';
    } else if (v >= 10u) {
        d[0] = (char)('0' + (v / 10u) % 10u);
        d[1] = (char)('0' + v % 10u);
        d[2] = '\0';
    } else {
        d[0] = (char)('0' + v);
        d[1] = '\0';
    }
}

static void deck_i32_signed_dec(char *d, int32_t v)
{
    if (v < 0) {
        *d++ = '-';
        v = -v;
    }
    deck_u8_dec(d, (uint32_t)v);
}

static uint32_t deck_u8_show100_from127(uint32_t v)
{
    if (v > 127u)
        v = 127u;
    return (v * 100u + 63u) / 127u;
}

static void deck_u8_show100_dec(char *d, uint32_t v)
{
    deck_u8_dec(d, deck_u8_show100_from127(v));
}

static void deck_bip50_show(char *s, uint8_t v)
{
    int32_t c = (int32_t)(v > 127u ? 127u : v) - 64;

    if (c < -63)
        c = -63;
    deck_i32_signed_dec(s, c < 0 ? -((-c * 50 + 31) / 63) : (c * 50 + 31) / 63);
}

/* Knob readout: bipolar −50..+50, unipolar 0..100; raw only for values with a real unit (BPM, swing %, channel). */
static void deck_knob_num(char *s, uint8_t v, int bip, int raw)
{
    if (raw)
        deck_fmt_u8(s, v);
    else if (bip)
        deck_bip50_show(s, v);
    else
        deck_u8_show100_dec(s, v);
}

/* PAN side magnitude 1..100 (same mapping as KO BASS pan_parts). */
static void deck_pan_num(char *s, int32_t pn)
{
    uint32_t a = (uint32_t)(pn < 0 ? -pn : pn);
    uint32_t val = a ? 1u + (a - 1u) * 99u / 62u : 0u;

    deck_u8_dec(s, val > 100u ? 100u : val);
}

/* LFO rate as note division or Hz (same as KO BASS). */
static void deck_lfo_rate_show(char *s, uint8_t rate, uint32_t bpm)
{
    uint8_t rate_show = fm_lfo_rate_clamp(rate, bpm);
    const char *nd = fm_lfo_note_div(rate_show, bpm);
    uint32_t hz100 = fm_lfo_hz100(fm_lfo_rate_resolved(rate_show, bpm));
    uint32_t n = 0u;

    if (nd) {
        deck_str_copy(s, nd);
        return;
    }
    deck_u8_dec(s, hz100 / 100u);
    while (s[n])
        n++;
    s[n++] = '.';
    s[n++] = (char)('0' + ((hz100 % 100u) / 10u));
    s[n++] = 'H';
    s[n++] = 'z';
    s[n] = '\0';
}

static void deck_knob_lbl_dry_wet(const ko_knob_slot_t *k, int32_t x)
{
    fm_dual_morph_label_draw(x, 2, "DRY", "WET", k->custom_arg, k->klab, C_DIM);
}

static void deck_fx_knob_value(uint8_t pg, uint8_t col, const deck_master_t *fx, deck_runtime_t *r, char *val,
                               uint8_t v)
{
    if (pg == 0u) {
        if (col == 1u)
            fm_time_fmt(val, v, r->bpm ? r->bpm : 120u, fm1_ms);
        else if (col == 3u)
            deck_bip50_show(val, v);
        else
            deck_u8_show100_dec(val, v);
    } else if (pg == 1u) {
        if (col == 1u)
            deck_str_copy(val, DECK_TAP_NAMES[fx->tap % DECK_TAP_N]);
        else if (col == 3u)
            deck_bip50_show(val, v);
        else
            deck_u8_show100_dec(val, v);
    } else if (pg == 3u && col == 2u)
        deck_bip50_show(val, v);
    else
        deck_u8_show100_dec(val, v);
}

static void deck_plock_seed_step_x(deck_plock_ent_t *e, const deck_pattern_t *p, uint8_t voice, uint8_t st)
{
    uint8_t gate;

    if (!e || e->step_x)
        return;
    gate = deck_seq_get_hit(p, voice, st) ? DECK_SEQ_GATE_DEF : 0u;
    e->step_x = (uint8_t)(gate | ((DECK_SEQ_RATCHET_MIN - 1u) << DECK_SX_RATCH_SHIFT));
}

static uint8_t deck_seq_knob_bar_px(uint8_t slot, const deck_pattern_t *p, uint8_t pat_ix, uint8_t voice,
                                    uint8_t st, uint8_t plock)
{
    uint32_t v = 0u;
    uint32_t mx = 127u;

    if (plock) {
        if (slot == 0u) {
            uint8_t vel = deck_plock_vel(pat_ix, voice, st, deck_seq_get_accent(p, voice, st));

            v = vel ? vel : (uint32_t)deck_seq_lv_lerp(deck_seq_get_accent(p, voice, st), 96, 110);
            mx = 127u;
        } else if (slot == 1u)
            return fm_seq_cond_bar_px(deck_plock_cond(pat_ix, voice, st));
        else if (slot == 2u)
            return fm_seq_bip_pot((int32_t)deck_plock_micro(pat_ix, voice, st), FM_SEQ_MICRO_HALF);
        else
            return fm_seq_bip_pot((int32_t)deck_plock_pitch(pat_ix, voice, st), 24);
    } else {
        uint8_t gate = deck_seq_gate(p, pat_ix, voice, st);
        uint8_t slide = deck_seq_slide(p, pat_ix, voice, st);
        uint8_t acc = deck_seq_get_accent(p, voice, st);
        uint8_t rat = deck_seq_ratchet(p, pat_ix, voice, st);

        if (slot == 0u) {
            v = gate;
            mx = DECK_SEQ_GATE_TIE;
        } else if (slot == 1u) {
            v = acc;
            mx = DECK_SEQ_LV_FULL;
        } else if (slot == 2u) {
            v = slide;
            mx = DECK_SEQ_LV_FULL;
        } else {
            v = (uint32_t)(rat - DECK_SEQ_RATCHET_MIN);
            mx = (uint32_t)(DECK_SEQ_RATCHET_MAX - DECK_SEQ_RATCHET_MIN);
        }
    }
    if (v > mx)
        v = mx;
    return mx ? (uint8_t)((v * 127u) / mx) : 0u;
}

static void deck_seq_knob_discrete(uint8_t pg, uint8_t slot, uint8_t plock, uint8_t *mode, uint8_t *steps)
{
    *mode = KNOB_POT_CONT;
    *steps = 0u;
    if (plock)
        return;
    if (pg == 0u) {
        if (slot == 1u || slot == 2u)
            *mode = KNOB_POT_TICK, *steps = (uint8_t)(DECK_SEQ_LV_FULL + 1u);
        else if (slot == 3u)
            *mode = KNOB_POT_ORD, *steps = (uint8_t)(DECK_SEQ_RATCHET_MAX - DECK_SEQ_RATCHET_MIN + 1u);
        return;
    }
    if (pg == 3u) {
        if (slot == 1u)
            *mode = KNOB_POT_SEL, *steps = 3u;
        else if (slot == 2u)
            *mode = KNOB_POT_ORD, *steps = FM_SEQ_CLK_RATIO_N;
        return;
    }
    if (pg == 2u) {
        if (slot == 0u)
            *mode = KNOB_POT_ORD, *steps = FM_SEQ_MAX_PAGES;
        else if (slot == 1u)
            *mode = KNOB_POT_MASK, *steps = KNOB_POT_MASK_DECK_N;
        else if (slot == 2u)
            *mode = KNOB_POT_HALF, *steps = 2u;
        return;
    }
    if (pg == 4u) {
        if (slot == 0u)
            *mode = KNOB_POT_ORD, *steps = DECK_NVOICES + 1u; /* 16 voices + ALL */
        else if (slot == 1u)
            *mode = KNOB_POT_ORD, *steps = FM_SEQ_EUCLID_CYCLE_N;
        else if (slot == 2u)
            *mode = KNOB_POT_ORD, *steps = 16u;
        return;
    }
}

static uint8_t deck_seq_knob_bar_lane_pages(uint8_t pg, uint8_t col, const deck_runtime_t *r,
                                            const deck_pattern_t *p, uint8_t voice)
{
    const deck_lane_t *ln = &p->lane[voice];
    uint32_t v = 0u;
    uint32_t mx = 127u;

    if (pg == 3u) {
        if (col == 0u) {
            v = fm_seq_lane_len_clamp(ln->length);
            mx = FM_SEQ_STEPS;
        } else if (col == 1u)
            return deck_lane_p3_bar_px(deck_lane_p3_state_pos(r, voice));
        else if (col == 2u) {
            v = (uint32_t)fm_seq_clk_ratio_lane_ix(ln->div);
            mx = (uint32_t)(FM_SEQ_CLK_RATIO_N - 1u);
        } else if (pg == 3u && col == 3u)
            return fm_seq_bip_pot((int32_t)deck_seq_lane_nudge_get(r->cur_pat, voice), FM_SEQ_TIMING_UNITS);
    } else if (pg == 2u) {
        if (col == 0u) {
            v = r->bar_page % FM_SEQ_MAX_PAGES;
            mx = FM_SEQ_MAX_PAGES - 1u;
        } else if (col == 1u)
            return 0u;
        else if (col == 2u)
            return g.seq_flip_state ? 127u : 0u;
        else
            return 0u;
    } else     if (pg == 4u) {
        /* LANE 0..15; PATTRN bar: EUC n out of 16 (0 for EMPTY/SONG/FRESH/EDITED); ROTATE 0..15; VARY 0..127. */
        if (col == 0u) {
            if (deck_seq_g.lane_all) {
                v = DECK_NVOICES;
                mx = DECK_NVOICES;
            } else {
                v = voice;
                mx = DECK_NVOICES - 1u;
            }
        } else if (col == 1u) {
            v = fm_seq_euclid_bar_hits(ln->euclid_hits);
            mx = 16u;
        } else if (col == 2u) {
            v = fm_seq_euclid_rot_norm(ln->euclid_rot);
            mx = 15u;
        } else {
            v = fm_seq_vary_norm(ln->vary);
            mx = 127u;
        }
    }
    if (v > mx)
        v = mx;
    return mx ? (uint8_t)((v * 127u) / mx) : 0u;
}

static void deck_seq_knob_fill_lane_pages(uint8_t pg, uint8_t col, ko_knob_slot_t *k, const deck_runtime_t *r,
                                          const deck_pattern_t *p, uint8_t voice, uint8_t *mode, uint8_t *steps)
{
    const deck_lane_t *ln = &p->lane[voice];

    deck_seq_knob_discrete(pg, col, 0u, mode, steps);
    k->pv = deck_seq_knob_bar_lane_pages(pg, col, r, p, voice);
    if (pg == 2u && col == 1u) {
        uint16_t on = deck_lane_on_mask16(r);

        k->custom_arg = (uint32_t)on;
        k->pv = (uint8_t)(on & 0xFFu);
    }
    if (pg == 3u) {
        if (col == 0u)
            deck_u8_dec(k->value, fm_seq_lane_len_clamp(ln->length));
        else if (col == 1u) {
            uint8_t pos = deck_lane_p3_state_pos(r, voice);

            deck_str_copy(k->value, FM_SEQ_P3_STATE_NAMES[pos]);
        } else if (col == 2u) {
            deck_str_copy(k->value, fm_seq_clk_ratio_name(fm_seq_clk_ratio_lane_ix(ln->div)));
        } else if (pg == 3u && col == 3u) {
            fm_seq_fmt_nudge_readout((int32_t)deck_seq_lane_nudge_get(r->cur_pat, voice), k->value);
            k->bip = 1u;
        }
        return;
    }
    if (pg == 2u) {
        if (col == 0u)
            deck_u8_dec(k->value, (uint32_t)((r->bar_page % FM_SEQ_MAX_PAGES) + 1u));
        else if (col == 1u)
            deck_str_copy(k->value, deck_ens_combo_readout(r));
        else if (col == 2u)
            deck_str_copy(k->value, FM_SEQ_FLIP_NAMES[g.seq_flip_state & 1u]);
        return;
    }
    if (pg == 4u) {
        if (col == 0u)
            deck_str_copy(k->value, deck_seq_g.lane_all ? "ALL" : DECK_VNAME_LONG[voice % DECK_NVOICES]);
        else if (col == 1u)
            fm_seq_fmt_euclid(ln->euclid_hits, k->value);
        else if (col == 2u)
            deck_u8_dec(k->value, fm_seq_euclid_rot_norm(ln->euclid_rot));
        else
            fm_seq_fmt_vary(ln->vary, k->value);
    }
}

static uint8_t deck_seq_knob_page1_bar_px(uint8_t col, const deck_lane_t *ln)
{
    uint32_t v = 0u;
    uint32_t mx = 127u;

    if (!ln)
        return 64u;
    col = fm_seq_p1_fn(col);
    if (col == 1u) {
        v = fm_seq_trig_pct(ln->trig);
        mx = 100u;
        if (v > mx)
            v = mx;
        return mx ? (uint8_t)((v * 127u) / mx) : 0u;
    }
    {
        int8_t amt = col == 0u ? ln->vel_rng : col == 2u ? ln->gate_rng : ln->pitch_rng;
        int32_t lim = col == 3u ? 24 : 50;

        return fm_seq_bip_pot((int32_t)amt, lim);
    }
}

static void deck_seq_knob_fill_page1(uint8_t col, ko_knob_slot_t *k, const deck_lane_t *ln)
{
    uint8_t fn = fm_seq_p1_fn(col);

    if (fn == 1u) {
        fm_seq_fmt_trig(ln->trig, k->value);
        k->bip = 0u;
    } else {
        int8_t amt = fn == 0u ? ln->vel_rng : fn == 2u ? ln->gate_rng : ln->pitch_rng;

        fm_seq_fmt_rand_spread(amt, k->value);
        k->bip = 1u;
    }
    k->pv = deck_seq_knob_page1_bar_px(col, ln);
    k->mode = KNOB_POT_CONT;
    k->steps = 0u;
}

static void deck_seq_knob_adjust_page1(deck_lane_t *ln, uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    int8_t old;

    if (!ln)
        return;
    slot = fm_seq_p1_fn(slot);
    if (slot == 1u) {
        old = ln->trig;
        if (edit)
            ln->trig = steps < 0 ? 0 : (int8_t)-100;
        else
            ln->trig = fm_seq_trig_adj(ln->trig, d, 0);
        (void)old;
        return;
    }
    {
        int8_t *fp = slot == 0u ? &ln->vel_rng : slot == 2u ? &ln->gate_rng : &ln->pitch_rng;
        int32_t lim = slot == 3u ? 24 : 50;
        int32_t v;

        old = *fp;
        if (edit)
            *fp = steps < 0 ? 0 : (int8_t)lim;
        else {
            v = (int32_t)*fp + (slot == 3u ? steps : d);
            if (v < -lim)
                v = -lim;
            if (v > lim)
                v = lim;
            *fp = (int8_t)v;
        }
        (void)old;
    }
}

static void deck_seq_knob_fill(uint32_t col, ko_knob_slot_t *k, const deck_runtime_t *r, const deck_pattern_t *p)
{
    uint8_t pat_ix = r->cur_pat % DECK_NPAT;
    uint8_t voice = r->cur_voice % DECK_NVOICES;
    uint8_t st = g.seq_plock_hold ? g.seq_plock_step : deck_seq_cursor_step(r, p);
    uint8_t mode;
    uint8_t steps;
    uint8_t plock = g.seq_plock_hold ? 1u : 0u;
    uint8_t pg = g.seq_page;

    k->label[0] = k->value[0] = '\0';
    k->bip = 0u;
    k->pan = 0u;
    k->custom_label = NULL;
    k->custom_value = NULL;
    deck_str_copy(k->label, deck_seq_pot_label(pg, plock, g.seq_plock_bank, (uint8_t)col));

    if (st >= FM_SEQ_STEPS && pg == 0u && !plock) {
        deck_str_copy(k->value, "-");
        k->pv = 0u;
        k->mode = KNOB_POT_CONT;
        return;
    }

    if (plock && g.seq_plock_bank == 1u) {
        if (col == 0u)
            deck_u8_dec(k->value, deck_seq_gate(p, pat_ix, voice, st));
        else if (col == 1u)
            deck_str_copy(k->value, deck_seq_lv_name(deck_seq_get_accent(p, voice, st)));
        else if (col == 2u)
            deck_str_copy(k->value, deck_seq_lv_name(deck_seq_slide(p, pat_ix, voice, st)));
        else
            deck_u8_dec(k->value, deck_seq_ratchet(p, pat_ix, voice, st));
        k->pv = deck_seq_knob_bar_px((uint8_t)col, p, pat_ix, voice, st, 0u);
        deck_seq_knob_discrete(0u, (uint8_t)col, 0u, &mode, &steps);
        k->steps = steps;
        k->mode = mode;
        return;
    }

    if (plock) {
        if (col == 0u) {
            uint8_t vel = deck_plock_vel(pat_ix, voice, st, deck_seq_get_accent(p, voice, st));

            deck_u8_show100_dec(k->value, vel ? vel : (uint8_t)deck_seq_lv_lerp(deck_seq_get_accent(p, voice, st), 96, 110));
        } else if (col == 1u) {
            uint8_t cond = deck_plock_cond(pat_ix, voice, st);

            deck_str_copy(k->value, FM_SEQ_COND_NAMES[cond % FM_SEQ_NCONDS]);
        } else if (col == 2u)
            fm_seq_fmt_micro_readout((int32_t)deck_plock_micro(pat_ix, voice, st), k->value);
        else
            fm_seq_fmt_rand_range((int8_t)deck_plock_pitch(pat_ix, voice, st), k->value);
        k->pv = deck_seq_knob_bar_px((uint8_t)col, p, pat_ix, voice, st, plock);
        deck_seq_knob_discrete(pg, (uint8_t)col, plock, &mode, &steps);
        k->steps = steps;
        k->mode = mode;
        if (col == 2u || col == 3u)
            k->bip = 1u;
        return;
    }

    if (pg == 0u) {
        if (col == 0u)
            deck_u8_dec(k->value, deck_seq_gate(p, pat_ix, voice, st));
        else if (col == 1u)
            deck_str_copy(k->value, deck_seq_lv_name(deck_seq_get_accent(p, voice, st)));
        else if (col == 2u)
            deck_str_copy(k->value, deck_seq_lv_name(deck_seq_slide(p, pat_ix, voice, st)));
        else
            deck_u8_dec(k->value, deck_seq_ratchet(p, pat_ix, voice, st));
        k->pv = deck_seq_knob_bar_px((uint8_t)col, p, pat_ix, voice, st, 0u);
        deck_seq_knob_discrete(pg, (uint8_t)col, 0u, &mode, &steps);
        k->steps = steps;
        k->mode = mode;
        return;
    }

    if (pg == 1u) {
        deck_seq_knob_fill_page1((uint8_t)col, k, &p->lane[voice]);
        return;
    }

    if (pg == 3u || pg == 2u || pg == 4u) {
        deck_seq_knob_fill_lane_pages(pg, (uint8_t)col, k, r, p, voice, &mode, &steps);
        k->steps = steps;
        k->mode = mode;
        return;
    }
}

/* DJ toggle feedback on BTN_FX: 6 half-cycles of 90 ms (same as fm_app.c DJ_FX_FLASH_*). */
enum { DECK_DJ_FX_FLASH_MS = 90u, DECK_DJ_FX_FLASH_N = 6u };
static uint32_t deck_dj_fx_flash_ms;
static uint8_t deck_dj_fx_flash_active;

static void deck_dj_fx_flash_start(void)
{
    deck_dj_fx_flash_ms = fm1_ms;
    deck_dj_fx_flash_active = 1u;
}

/* Toast lanes (perf_toast_policy.h, same mechanism as fm_app): PROC one row up, MAIN centre, EPH two rows up.
 * Lanes stack; each lane has its own timer. Bar colour is kept beside the lane (lane struct has none). */
#define DECK_TOAST_MS 900u
static perf_toast_lane_t deck_lanes[PERF_TOAST_LANE_N];
static uint16_t deck_lane_bar[PERF_TOAST_LANE_N];

static void deck_toast_lane_put(uint8_t lane, const char *msg, uint16_t bar, uint32_t end)
{
    perf_toast_lane_t *L = &deck_lanes[lane];

    if (lane == PERF_TOAST_LANE_PROC)
        perf_toast_policy_lane_set_proc(L, msg, 1u, end, fm1_ms);
    else if (lane == PERF_TOAST_LANE_MAIN)
        perf_toast_policy_lane_set_main(L, msg, 1u, -1, end, fm1_ms, "", 0u);
    else
        perf_toast_policy_lane_set_eph(L, msg, 1u, -1, end, fm1_ms);
    deck_lane_bar[lane] = bar;
    g.ui_dirty = 1u;
}

/* MAIN is taken by the sticky DJ label or the ARP label (fm_app perf_toast_lane_sticky_main_occupied). */
static uint8_t deck_toast_main_sticky(void)
{
    const perf_toast_lane_t *main = &deck_lanes[PERF_TOAST_LANE_MAIN];

    return main->on && (perf_toast_policy_is_dj(main->msg) || perf_toast_policy_same_msg(main->msg, "ARP"));
}

/* Gesture / LFO / FX overlay labels: MAIN when free, else EPH (fm_app perf_toast_lanes_sync dest rule). */
static void deck_toast_overlay(const char *msg, uint16_t bar, uint32_t end)
{
    if (perf_toast_policy_dj_absorbs_gesture_msg(msg, g.dj_app.dj.on ? 1u : 0u))
        return;
    deck_toast_lane_put(deck_toast_main_sticky() ? PERF_TOAST_LANE_EPH : PERF_TOAST_LANE_MAIN, msg, bar, end);
}

/* One-shot entry point (~30 callers). Classifier: process labels -> PROC, gesture / overlay labels -> overlay rule,
 * everything else (MUTE, SOLO, FX ON, SAVED, FULL, DJ ON/OFF, ...) -> EPH. */
static void deck_toast(const char *msg, uint16_t bar)
{
    uint32_t end = fm1_ms + DECK_TOAST_MS;

    if (!msg || !msg[0])
        return;
    if (perf_toast_policy_is_process(msg))
        deck_toast_lane_put(PERF_TOAST_LANE_PROC, msg, bar, end);
    else if (perf_toast_policy_is_gesture(msg))
        deck_toast_overlay(msg, bar, end);
    else
        deck_toast_lane_put(PERF_TOAST_LANE_EPH, msg, bar, end);
}

static void deck_toast_lfo(uint8_t lt)
{
    char buf[16];
    const char *s = DECK_LFO_DEST[lt];
    uint32_t n = 0u;

    while (s[n] && n < 10u) {
        buf[n] = s[n];
        n++;
    }
    buf[n++] = ' ';
    buf[n++] = 'L';
    buf[n++] = 'F';
    buf[n++] = 'O';
    buf[n] = '\0';
    deck_toast_overlay(buf, C_ACC, fm1_ms + DECK_TOAST_MS);
}

/* fm_dj_lfo_toast_pick_tgt_stable bounds lt by DUB_LFO_TGT_N; deck lt values must stay below it. */
_Static_assert(DECK_LFO_TGT_N <= DUB_LFO_TGT_N, "deck LFO ids must fit the fm_dj_lfo toast bounds");

/* DJ LFO / FX overlay labels -> toast lanes, set on change only (fm_app: dj_toast_lfo_overlay_side, fm_dj_fx_toast_label). */
/* Side label for the sticky DJ toast, same priority as fm_app dj_toast_param:
 * DROP > transition > build > FX scene > ARP > FLAM / gesture > LFO overlay > audible DJ target. */
static const char *deck_dj_toast_side(const deck_dj_t *d, uint8_t lt, uint8_t sc, uint8_t tgt)
{
    const char *side;
    int32_t delta;

    if (!d->dj.on)
        return 0;
    if (fm_dj_any_slot_role(&d->dj, FM_DJ_ROLE_DROP))
        return "DROP";
    if (d->dj.break_arm >= 2u && d->dj.phrase_bar + 2u >= d->dj.break_arm && d->dj.phrase_bar < d->dj.drop_arm) {
        side = fm_dj_trans_toast_label(&d->dj);
        if (side)
            return side;
    }
    side = fm_dj_build_toast_label(&d->dj);
    if (side)
        return side;
    if (sc)
        return fm_dj_fx_scene_toast((uint8_t)(sc - 1u));
    if (d->gest_ov == FM_DJ_GEST_OV_ARP)
        return "ARP";
    if (d->gest_flam_steps > 0u)
        return "FLAM!";
    if (d->gest_kind) {
        side = deck_dj_gest_label(d->gest_kind);
        if (side[0])
            return side;
    }
    if (lt < DECK_LFO_TGT_N)
        return DECK_LFO_DEST[lt];
    /* SEQ_* targets (VEL/TRIG/GATE/PITCH) have no deck label yet: no side label. */
    if (tgt >= FM_DJ_TGT_N)
        return 0;
    delta = fm_dj_delta_for_tgt(&d->dj, tgt);
    if (!fm_dj_toast_delta_audible(delta))
        return 0;
    switch (tgt) {
    case FM_DJ_TGT_FB:
        return "ECHO FEEDBACK";
    case FM_DJ_TGT_DENS_H:
        return "HAT DENSITY";
    case FM_DJ_TGT_DENS_K:
        return "KICK DENSITY";
    case FM_DJ_TGT_ENSEMBLE:
        return "ENSEMBLE";
    case FM_DJ_TGT_FLIP:
        return "INVERT";
    case FM_DJ_TGT_SEQ_CYC:
        return "CYCLE";
    case FM_DJ_TGT_SEQ_REP:
        return "REPEAT";
    default:
        break;
    }
    side = 0;
    if (deck_dj_lfo_lt(tgt) < DECK_LFO_TGT_N)
        side = DECK_LFO_DEST[deck_dj_lfo_lt(tgt)];
    return side;
}

static void deck_dj_toast_overlay(void)
{
    deck_dj_t *d = &g.dj_app;
    uint8_t lt = fm_dj_lfo_toast_pick_tgt_stable(&d->lfo_ov, &d->dj, d->lfo_eff, d->lfo_rt, &d->lfo_hold);
    uint8_t id = lt < DECK_LFO_TGT_N ? (uint8_t)(lt + 1u) : 0u;
    uint8_t sc = deck_dj_fx_toast_scene(d);
    uint8_t tgt = fm_dj_toast_target_stable(&d->dj, &d->tgt_hold);
    const char *side;

    if (id != d->toast_lfo_last) {
        d->toast_lfo_last = id;
        if (id)
            deck_toast_lfo(lt);
    }
    if (sc != d->toast_fx_last) {
        d->toast_fx_last = sc;
        if (sc)
            deck_toast_overlay(fm_dj_fx_scene_toast((uint8_t)(sc - 1u)), C_ACC, fm1_ms + DECK_TOAST_MS);
    }
    side = deck_dj_toast_side(d, lt, sc, tgt);
    if (side != d->toast_side) {
        d->toast_side = side;
        g.ui_dirty = 1u;
    }
}

static void mark_dirty(void)
{
    g.ui_dirty = 1u;
    g.set_dirty = 1u;
    g.set_t = fm1_ms;
}

static deck_runtime_t *rt(void)
{
    return deck_seq_rt();
}

static deck_pattern_t *pat(void)
{
    return deck_seq_cur_pattern();
}

static uint16_t deck_mute_mask(const deck_runtime_t *r)
{
    uint16_t m = 0u;
    uint8_t v;

    if (!r)
        return 0u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (r->lane_mute[v])
            m |= (uint16_t)(1u << v);
    }
    return m;
}

static int deck_is_quiet(void)
{
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++) {
        if (g.dsp.v[v].active)
            return 0;
    }
    return 1;
}

static void deck_apply_meta(const deck_meta_t *m)
{
    deck_runtime_t *r = rt();
    uint8_t v;

    if (!m || m->magic != DECK_CFG_MAGIC)
        return;
    g.era = deck_clamp_era(m->era);
    g.plate = deck_clamp_plate(m->plate);
    deck_apply_plate(r);
    r->bpm = m->bpm ? m->bpm : 120u;
    r->swing = deck_clamp_swing_u8(m->swing ? m->swing : 50u);
    r->key = m->key % 12u;
    r->scale = m->scale % DECK_SCALE_N;
    r->cur_pat = m->cur_pat % DECK_NPAT;
    r->cur_voice = m->cur_voice % DECK_NVOICES;
    r->bar_page = m->bar_page % FM_SEQ_MAX_PAGES;
    g.seq_page = m->seq_page;
    if (!deck_seq_page_in_cycle(g.seq_page))
        g.seq_page = DECK_SEQ_KNOB_PAGE_CYCLE[0];
    r->solo_mask = m->solo_mask;
    for (v = 0u; v < DECK_NVOICES; v++) {
        r->lane_mute[v] = m->lane_mute[v];
        g.dsp.cfg[v] = m->cfg[v];
        g.env_delay[v] = m->env_delay[v] > 127u ? 127u : m->env_delay[v];
        g.env_retrig[v] = (uint8_t)((m->env_retrig_mask >> v) & 1u);
        g.env_invert[v] = (uint8_t)((m->env_invert_mask >> v) & 1u);
    }
    g.dsp.master = m->master;
    g.dsp.perf = m->perf;
    if (g.dsp.perf.mute_grp > 4u)
        g.dsp.perf.mute_grp = 0u;
    g.bank = m->bank % DECK_BANK_N;
    if (g.bank == DECK_BANK_ARP) /* ARP is an overlay now, not a bank: old saves land on HOME */
        g.bank = DECK_BANK_HOME;
    for (v = 0u; v < DECK_BANK_N; v++)
        g.page[v] = m->page[v] % deck_bank_pages(v);
    g.midi_ch = m->midi_ch > 15u ? 9u : m->midi_ch;
    g.clock = m->clock ? 1u : 0u;
    g.accent_amt = m->accent_amt;
    g.human = m->human;
    g.arp_rate = m->arp_rate % 5u;
    g.arp_gate = m->arp_gate;
    g.arp_ramp = m->arp_ramp;
    g.arp_latch = m->arp_latch ? 1u : 0u;
    g.fx_bypass = m->fx_bypass ? 1u : 0u;
    palette_set(2u);
}

static void deck_fill_meta(deck_meta_t *m)
{
    deck_runtime_t *r = rt();
    uint8_t v;

    memset(m, 0, sizeof *m);
    m->magic = DECK_CFG_MAGIC;
    m->bpm = r->bpm;
    m->swing = r->swing;
    m->key = r->key;
    m->scale = r->scale;
    m->cur_pat = r->cur_pat;
    m->cur_voice = r->cur_voice;
    m->bar_page = r->bar_page;
    m->seq_page = g.seq_page;
    m->bank = g.bank;
    for (v = 0u; v < DECK_BANK_N; v++)
        m->page[v] = g.page[v];
    m->midi_ch = g.midi_ch;
    m->clock = g.clock;
    m->accent_amt = g.accent_amt;
    m->human = g.human;
    m->arp_rate = g.arp_rate;
    m->arp_gate = g.arp_gate;
    m->arp_ramp = g.arp_ramp;
    m->arp_latch = g.arp_latch;
    m->fx_bypass = g.fx_bypass;
    m->era = g.era;
    m->plate = g.plate;
    m->solo_mask = r->solo_mask;
    for (v = 0u; v < DECK_NVOICES; v++) {
        m->lane_mute[v] = r->lane_mute[v];
        m->cfg[v] = g.dsp.cfg[v];
        m->env_delay[v] = g.env_delay[v];
        if (g.env_retrig[v])
            m->env_retrig_mask |= (uint16_t)(1u << v);
        if (g.env_invert[v])
            m->env_invert_mask |= (uint16_t)(1u << v);
    }
    m->master = g.dsp.master;
    m->perf = g.dsp.perf;
}

static void deck_pad_family_init(void)
{
    g.pad_family_voice[DECK_PAD_SHAKER] = DECK_CB;
    g.pad_family_voice[DECK_PAD_TOM] = DECK_TM;
    g.pad_family_voice[DECK_PAD_CYM] = DECK_CR;
    g.pad_family_voice[DECK_PAD_CONGA] = DECK_CNH;
    g.last_black_pad = 255u;
}

static void deck_pad_family_sync_voice(uint8_t voice)
{
    static const uint8_t fam_pads[4] = {DECK_PAD_SHAKER, DECK_PAD_TOM, DECK_PAD_CYM, DECK_PAD_CONGA};
    uint32_t i;

    voice %= DECK_NVOICES;
    for (i = 0u; i < 4u; i++) {
        uint8_t pad = fam_pads[i];

        if (deck_pad_voice_in_family(pad, voice))
            g.pad_family_voice[pad] = voice;
    }
}

static uint8_t deck_pad_voice_for_hit(uint8_t pad, uint32_t btn)
{
    uint8_t mod = (uint8_t)((btn & ((1u << BTN_FX) | (1u << BTN_SEL))) != 0u);
    uint8_t voice;

    if (deck_pad_is_family(pad)) {
        voice = g.pad_family_voice[pad];
        if (!mod && g.last_black_pad == pad)
            voice = deck_pad_family_next(pad, voice);
        g.pad_family_voice[pad] = voice;
    } else {
        voice = deck_pad_resolve(pad, 0u, 0u);
    }
    if (!mod)
        g.last_black_pad = pad;
    return voice;
}

enum { DECK_SAVE_WORK = 0, DECK_SAVE_PLATE = 1, DECK_SAVE_FACTORY = 2 };

static deck_pattern_t deck_store_ref_pat;
static fm_lfo_slot_t deck_store_ref_lfo;

/* Bank XORs against a cleared pattern, LFO against a reset slot: untouched state packs to zero runs. */
static uint32_t deck_store_segs(deck_store_seg_t *s, deck_meta_t *m)
{
    deck_seq_zero(&deck_store_ref_pat, sizeof deck_store_ref_pat);
    deck_seq_clear_pat(&deck_store_ref_pat);
    fm_lfo_slot_reset(&deck_store_ref_lfo);
    s[0] = (deck_store_seg_t){(uint8_t *)m, sizeof *m, NULL, 0u};
    s[1] = (deck_store_seg_t){(uint8_t *)&deck_seq_g.bank, sizeof deck_seq_g.bank,
                              (const uint8_t *)&deck_store_ref_pat, sizeof deck_store_ref_pat};
    s[2] = (deck_store_seg_t){(uint8_t *)&deck_seq_g.plock, sizeof deck_seq_g.plock, NULL, 0u};
    s[3] = (deck_store_seg_t){(uint8_t *)g.lfo, sizeof g.lfo, (const uint8_t *)&deck_store_ref_lfo,
                              sizeof deck_store_ref_lfo};
    s[4] = (deck_store_seg_t){(uint8_t *)deck_plate_ov, sizeof deck_plate_ov, NULL, 0u};
    s[5] = (deck_store_seg_t){(uint8_t *)deck_seq_g.lane_nudge, sizeof deck_seq_g.lane_nudge, NULL, 0u};
    return 6u;
}

_Static_assert(sizeof(deck_bank_t) == DECK_NPAT * sizeof(deck_pattern_t), "bank XOR ref stride");
_Static_assert(sizeof(g.lfo) == DECK_NVOICES * DECK_LFO_TGT_N * sizeof(fm_lfo_slot_t), "lfo XOR ref stride");

#define DECK_LFO_ROW_V4 ((uint32_t)DECK_LFO_TGT_N_V4 * (uint32_t)sizeof(fm_lfo_slot_t))

/* Blobs saved before the appended LFO targets carry DECK_LFO_TGT_N_V4 slots per voice. They decode into
 * the head of g.lfo and widen back-to-front (row v lands at or after its old offset); new slots reset. */
static void deck_lfo_widen_v4(void)
{
    uint8_t *raw = (uint8_t *)g.lfo;
    uint32_t v = DECK_NVOICES, t, i;

    while (v--) {
        uint8_t *dst = (uint8_t *)g.lfo[v];
        const uint8_t *src = raw + v * DECK_LFO_ROW_V4;

        for (i = DECK_LFO_ROW_V4; i--;)
            dst[i] = src[i];
        for (t = DECK_LFO_TGT_N_V4; t < DECK_LFO_TGT_N; t++)
            fm_lfo_slot_reset(&g.lfo[v][t]);
    }
}

static void deck_storage_defaults(deck_meta_t *m)
{
    uint8_t i;

    memset(deck_plate_ov, 0, sizeof deck_plate_ov);
    deck_pad_family_init();
    deck_seq_init();
    deck_dsp_init(&g.dsp);
    deck_env_runtime_init();
    g.midi_ch = 9u;
    g.accent_amt = 96u;
    g.arp_rate = 1u;
    g.arp_gate = 64u;
    g.bank = DECK_BANK_HOME;
    g.era = DECK_ERA_STAX;
    g.plate = 0u;
    g.seq_ens_sel = FM_SEQ_ENS_ALL;
    deck_lfo_order_build();
    (void)deck_lfo_dest_touch(DECK_LFO_LEVEL);
    deck_lfo_init_all();
    deck_apply_plate(rt());
    deck_fill_meta(m);
    for (i = 0u; i < DECK_NPAT; i++)
        deck_seq_clear_pat(&deck_seq_g.bank.p[i]);
    deck_seq_bake(&deck_seq_g.bank.p[0], g.era, (uint8_t)(g.plate % DECK_PLATES_PER_ERA));
}

static void deck_cfg_from_v3(deck_voice_cfg_t *c, const deck_voice_cfg_v3_t *o)
{
    memcpy(c, o, sizeof *o);
    c->dly_send = 127u;
    c->rev_send = 127u;
    c->duck = 0u;
}

/* DKZ3/DKZ4 bank segment: frozen v4 lane layout; XOR ref = cleared v4 pattern (length 16, div ONE). */
static deck_store_seg_t deck_store_seg_bank_v4(void)
{
    uint8_t v;

    deck_seq_zero(&deck_bank_old_ref.v4, sizeof deck_bank_old_ref.v4);
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_bank_old_ref.v4.lane[v].length = (uint8_t)FM_SEQ_PAGE_STEPS;
        deck_bank_old_ref.v4.lane[v].div = FM_SEQ_CLK_RATIO_ONE;
    }
    return (deck_store_seg_t){(uint8_t *)&deck_bank_old_buf, sizeof deck_bank_old_buf,
                              (const uint8_t *)&deck_bank_old_ref.v4, sizeof deck_bank_old_ref.v4};
}

/* DKZ5 bank segment: frozen v5 lane layout; XOR ref = cleared v5 pattern (as deck_seq_clear_pat, without vary). */
static deck_store_seg_t deck_store_seg_bank_v5(void)
{
    uint8_t v;

    deck_seq_zero(&deck_bank_old_ref.v5, sizeof deck_bank_old_ref.v5);
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_bank_old_ref.v5.lane[v].length = (uint8_t)FM_SEQ_PAGE_STEPS;
        deck_bank_old_ref.v5.lane[v].div = FM_SEQ_CLK_RATIO_ONE;
        deck_bank_old_ref.v5.lane[v].euclid_hits = FM_SEQ_EUCLID_EDITED;
        deck_bank_old_ref.v5.lane[v].pattern_origin = FM_SEQ_PATTERN_ORIGIN_NONE;
    }
    return (deck_store_seg_t){(uint8_t *)&deck_bank_old_buf, sizeof deck_bank_old_buf,
                              (const uint8_t *)&deck_bank_old_ref.v5, sizeof deck_bank_old_ref.v5};
}

/* Frozen v4 bank → live bank. New seq-mode fields take the defaults (manual, no origin). */
static void deck_bank_from_v4(void)
{
    uint32_t p, v;

    for (p = 0u; p < DECK_NPAT; p++) {
        deck_pattern_t *d = &deck_seq_g.bank.p[p];
        const deck_pattern_v4_t *o = &deck_bank_old_buf.v4.p[p];

        for (v = 0u; v < DECK_NVOICES; v++) {
            const deck_lane_v4_t *s = &o->lane[v];
            deck_lane_t *ln = &d->lane[v];

            memcpy(ln->hit, s->hit, sizeof ln->hit);
            memcpy(ln->accent, s->accent, sizeof ln->accent);
            memcpy(ln->accent_cut, s->accent_cut, sizeof ln->accent_cut);
            ln->length = s->length;
            ln->div = s->div;
            ln->vel_rng = s->vel_rng;
            ln->trig = s->trig;
            ln->gate_rng = s->gate_rng;
            ln->pitch_rng = s->pitch_rng;
            ln->euclid_hits = FM_SEQ_EUCLID_EDITED;
            ln->pattern_origin = FM_SEQ_PATTERN_ORIGIN_NONE;
            ln->euclid_rot = 0;
        }
        memcpy(d->name, o->name, sizeof d->name);
    }
}

/* Frozen v5 bank → live bank. vary takes the default (0). */
static void deck_bank_from_v5(void)
{
    uint32_t p, v;

    for (p = 0u; p < DECK_NPAT; p++) {
        deck_pattern_t *d = &deck_seq_g.bank.p[p];
        const deck_pattern_v5_t *o = &deck_bank_old_buf.v5.p[p];

        for (v = 0u; v < DECK_NVOICES; v++) {
            const deck_lane_v5_t *s = &o->lane[v];
            deck_lane_t *ln = &d->lane[v];

            memcpy(ln->hit, s->hit, sizeof ln->hit);
            memcpy(ln->accent, s->accent, sizeof ln->accent);
            memcpy(ln->accent_cut, s->accent_cut, sizeof ln->accent_cut);
            ln->length = s->length;
            ln->div = s->div;
            ln->vel_rng = s->vel_rng;
            ln->trig = s->trig;
            ln->gate_rng = s->gate_rng;
            ln->pitch_rng = s->pitch_rng;
            ln->euclid_hits = s->euclid_hits;
            ln->pattern_origin = s->pattern_origin;
            ln->euclid_rot = s->euclid_rot;
            ln->vary = 0u;
        }
        memcpy(d->name, o->name, sizeof d->name);
    }
}

/* DKZ7 blob → live state. Same segments as DKZ8, meta without key/scale; key/scale default (C, MINOR). 0 = migrated. */
static int deck_load_v7(deck_meta_t *meta)
{
    deck_store_seg_t seg[6];

    (void)deck_store_segs(seg, meta);
    seg[0].n = DECK_META_V7_N;
    if (deck_store_decode(seg, 6u, DECK_STORE_MAGIC_V7, deck_store_buf, sizeof deck_store_buf, 0) != 0)
        return -1;
    (void)deck_store_decode(seg, 6u, DECK_STORE_MAGIC_V7, deck_store_buf, sizeof deck_store_buf, 1);
    return 0;
}

/* DKZ6 blob → live state. Same segments as DKZ7 without lane nudge (5 segments); NUDGE stays 0. 0 = migrated. */
static int deck_load_v6(deck_meta_t *meta)
{
    deck_store_seg_t seg[6];

    (void)deck_store_segs(seg, meta);
    seg[0].n = DECK_META_V7_N;
    if (deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V6, deck_store_buf, sizeof deck_store_buf, 0) != 0)
        return -1;
    (void)deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V6, deck_store_buf, sizeof deck_store_buf, 1);
    deck_seq_zero(deck_seq_g.lane_nudge, sizeof deck_seq_g.lane_nudge);
    return 0;
}

/* DKZ5 blob → live state. Same segments as DKZ6; only the bank uses the frozen v5 layout. 0 = migrated. */
static int deck_load_v5(deck_meta_t *meta)
{
    deck_store_seg_t seg[6];

    (void)deck_store_segs(seg, meta);
    seg[0].n = DECK_META_V7_N;
    seg[1] = deck_store_seg_bank_v5();
    if (deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V5, deck_store_buf, sizeof deck_store_buf, 0) != 0)
        return -1;
    (void)deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V5, deck_store_buf, sizeof deck_store_buf, 1);
    deck_bank_from_v5();
    return 0;
}

/* DKZ4 blob → live state. Two LFO widths as before the seq-mode fields; bank via the v4 layout. 0 = migrated. */
static int deck_load_v4(deck_meta_t *meta)
{
    deck_store_seg_t seg[6];

    (void)deck_store_segs(seg, meta);
    seg[0].n = DECK_META_V7_N;
    seg[1] = deck_store_seg_bank_v4();
    if (deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V4, deck_store_buf, sizeof deck_store_buf, 0) == 0) {
        (void)deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V4, deck_store_buf, sizeof deck_store_buf, 1);
        deck_bank_from_v4();
        return 0;
    }
    seg[3].n = DECK_NVOICES * DECK_LFO_ROW_V4;
    if (deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V4, deck_store_buf, sizeof deck_store_buf, 0) != 0)
        return -1;
    (void)deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V4, deck_store_buf, sizeof deck_store_buf, 1);
    deck_lfo_widen_v4();
    deck_bank_from_v4();
    return 0;
}

/* DKZ3 blob → live state. Plate overrides decode into deck_plate_ov at the old stride, then widen
 * back-to-front (new[i] only overlaps old[j >= i]). 0 = migrated; nothing is written on failure. */
static int deck_load_v3(deck_meta_t *meta)
{
    deck_store_seg_t seg[6];
    deck_meta_v3_t m3;
    uint8_t *ov_raw = (uint8_t *)deck_plate_ov;
    uint32_t i, v;

    (void)deck_store_segs(seg, meta);
    seg[0] = (deck_store_seg_t){(uint8_t *)&m3, sizeof m3, NULL, 0u};
    seg[1] = deck_store_seg_bank_v4();
    seg[3].n = DECK_NVOICES * DECK_LFO_ROW_V4;
    seg[4] = (deck_store_seg_t){ov_raw, sizeof(deck_plate_ov_v3_t) * DECK_PLATE_N, NULL, 0u};
    if (deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V3, deck_store_buf, sizeof deck_store_buf, 0) != 0)
        return -1;
    (void)deck_store_decode(seg, 5u, DECK_STORE_MAGIC_V3, deck_store_buf, sizeof deck_store_buf, 1);
    deck_lfo_widen_v4();
    deck_bank_from_v4();

    memset(meta, 0, sizeof *meta);
    memcpy(meta, &m3, offsetof(deck_meta_v3_t, cfg));
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_cfg_from_v3(&meta->cfg[v], &m3.cfg[v]);
        meta->env_delay[v] = m3.env_delay[v];
    }
    meta->master = m3.master;
    meta->perf = m3.perf;
    meta->env_retrig_mask = m3.env_retrig_mask;
    meta->env_invert_mask = m3.env_invert_mask;

    i = DECK_PLATE_N;
    while (i--) {
        deck_plate_ov_v3_t o;
        deck_plate_ov_t *d = &deck_plate_ov[i];

        memcpy(&o, ov_raw + i * sizeof o, sizeof o);
        memset(d, 0, sizeof *d);
        d->used = o.used;
        d->era = o.era;
        d->bpm = o.bpm;
        d->swing = o.swing;
        for (v = 0u; v < DECK_NVOICES; v++) {
            deck_cfg_from_v3(&d->cfg[v], &o.cfg[v]);
            d->env_delay[v] = o.env_delay[v];
        }
        d->master = o.master;
        d->perf = o.perf;
        d->env_retrig_mask = o.env_retrig_mask;
        d->env_invert_mask = o.env_invert_mask;
    }
    return 0;
}

static int deck_load_all(void) /* 1: restored from settings */
{
    deck_store_seg_t seg[6];
    deck_meta_t meta;
    uint32_t nseg;
    uint8_t v;
    int loaded = 0;

    deck_seq_init();
    deck_dsp_init(&g.dsp);
    deck_env_runtime_init();
    deck_dsp_sync_env_flags();
    deck_pad_family_init();
    deck_lfo_init_all();
    g.midi_ch = 9u;
    g.accent_amt = 96u;
    g.arp_rate = 1u;
    g.arp_gate = 64u;
    g.bank = DECK_BANK_HOME;
    g.era = DECK_ERA_STAX;
    g.plate = 0u;
    g.seq_ens_sel = FM_SEQ_ENS_ALL;
    memset(deck_plate_ov, 0, sizeof deck_plate_ov);
    nseg = deck_store_segs(seg, &meta);
    meta.key = 0u;   /* older blobs: KEY C, SCALE MINOR */
    meta.scale = 0u;
    if (settings_load(deck_store_buf, sizeof deck_store_buf) == 0) {
        if (deck_store_decode(seg, nseg, DECK_STORE_MAGIC, deck_store_buf, sizeof deck_store_buf, 0) == 0) {
            (void)deck_store_decode(seg, nseg, DECK_STORE_MAGIC, deck_store_buf, sizeof deck_store_buf, 1);
            loaded = meta.magic == DECK_CFG_MAGIC && deck_seq_g.bank.p[0].lane[0].length != 0u;
        } else if (deck_load_v7(&meta) == 0) {
            loaded = meta.magic == DECK_CFG_MAGIC && deck_seq_g.bank.p[0].lane[0].length != 0u;
        } else if (deck_load_v6(&meta) == 0) {
            loaded = meta.magic == DECK_CFG_MAGIC && deck_seq_g.bank.p[0].lane[0].length != 0u;
        } else if (deck_load_v5(&meta) == 0) {
            loaded = meta.magic == DECK_CFG_MAGIC && deck_seq_g.bank.p[0].lane[0].length != 0u;
        } else if (deck_load_v4(&meta) == 0) {
            loaded = meta.magic == DECK_CFG_MAGIC && deck_seq_g.bank.p[0].lane[0].length != 0u;
        } else if (deck_load_v3(&meta) == 0) {
            loaded = meta.magic == DECK_CFG_MAGIC && deck_seq_g.bank.p[0].lane[0].length != 0u;
        }
    }
    if (!loaded)
        deck_storage_defaults(&meta);
    deck_seq_migrate_lane_div_bank(&deck_seq_g.bank);
    deck_seq_lane_nudge_unify();
    deck_apply_meta(&meta);
    deck_dsp_sync_env_flags();
    {
        uint8_t det = deck_detect_ensemble(rt());

        g.seq_ens_sel = det == FM_SEQ_ENS_CUSTOM ? FM_SEQ_ENS_ALL : det;
    }
    deck_lfo_order_build();
    g.lfo_tgt = fm_lfo_dests_unpack(&deck_lfo_dests, g.lfo_tgt, DECK_LFO_LEVEL);
    for (v = 0u; v < DECK_NVOICES; v++)
        fm_lfo_slots_fixup_empty_rates(deck_lfo_sl(v), DECK_LFO_TGT_N);
    return loaded;
}

static int deck_store_write(void) /* 0: saved; -1: blob does not fit (nothing written) */
{
    deck_store_seg_t seg[6];
    deck_meta_t meta;
    uint32_t nseg = deck_store_segs(seg, &meta), n, used;

    deck_fill_meta(&meta);
    n = deck_store_encode(seg, nseg, DECK_STORE_MAGIC, deck_store_buf, sizeof deck_store_buf);
    if (!n)
        return -1;
    used = (uint32_t)sizeof(deck_store_hdr_t) + n;
    memset(deck_store_buf + used, 0, sizeof deck_store_buf - used);
    settings_save(deck_store_buf, sizeof deck_store_buf);
    return 0;
}

/* WORK: working state only (auto-save); PLATE: also snapshot into the current plate; FACTORY: drop it. */
static void deck_save_all(uint8_t mode)
{
    uint8_t pl = deck_clamp_plate(g.plate);
    deck_plate_ov_t bak = deck_plate_ov[pl];

    if (mode == DECK_SAVE_PLATE)
        deck_plate_ov_capture(&deck_plate_ov[pl]);
    else if (mode == DECK_SAVE_FACTORY) {
        memset(&deck_plate_ov[pl], 0, sizeof deck_plate_ov[pl]);
        deck_apply_plate(rt());
    }
    g.set_dirty = 0u;
    if (deck_store_write() != 0) {
        deck_plate_ov[pl] = bak;
        deck_toast("FULL", RGB(255, 48, 48));
        return;
    }
    /* WORK (auto-save, also after DJ changes) stays silent: no SAVED toast, no SAVE LED flash. */
    if (mode == DECK_SAVE_WORK)
        return;
    g.saved_until = fm1_ms + 1200u;
    deck_toast(mode == DECK_SAVE_FACTORY ? "FACTORY" : "SAVED", C_ACC);
}

static int32_t deck_wave_sine(uint32_t ph)
{
    return deck_sine(ph);
}

static uint8_t deck_seq_cursor_step(const deck_runtime_t *r, const deck_pattern_t *pat)
{
    uint8_t v = r->cur_voice % DECK_NVOICES;
    const deck_lane_t *ln = &pat->lane[v];
    uint32_t len = fm_seq_lane_len_clamp(ln->length);
    uint32_t base = deck_seq_view_base(r);

    if (g.seq_plock_step != FM_SEQ_PLOCK_NONE && (uint32_t)g.seq_plock_step < len)
        return g.seq_plock_step;
    if (r->playing) {
        uint8_t ls = deck_ui_last_step(r, pat, v);

        if (ls != DECK_UI_NO_STEP)
            return ls;
    }
    if (base < len)
        return (uint8_t)base;
    return 0u;
}

static void deck_invert_all_lane_mutes(deck_runtime_t *r)
{
    uint8_t v;

    if (!r)
        return;
    for (v = 0u; v < DECK_NVOICES; v++)
        r->lane_mute[v] = r->lane_mute[v] ? 0u : 1u;
}

static uint8_t deck_seq_step_editable(const deck_runtime_t *r)
{
    return (uint8_t)(!r->playing || g.seq_plock_hold);
}

static void deck_knob_adjust_seq_gate(uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    uint8_t pat_ix = r->cur_pat % DECK_NPAT;
    uint8_t voice = r->cur_voice % DECK_NVOICES;
    deck_lane_t *ln = &p->lane[voice];
    uint8_t st = g.seq_plock_hold ? g.seq_plock_step : deck_seq_cursor_step(r, p);
    deck_plock_ent_t *e;

    if (!deck_seq_step_editable(r))
        return;

    if (st >= ln->length)
        return;
    if (slot == 1u) {
        uint8_t lv = deck_seq_get_accent(p, voice, st);

        if (edit && steps != 0)
            lv = steps < 0 ? 0u : DECK_SEQ_LV_FULL;
        else if (steps != 0)
            adj_u8(&lv, steps < 0 ? -1 : 1, 0, DECK_SEQ_LV_FULL);
        deck_seq_put_accent(ln, st, lv);
        mark_dirty();
        return;
    }
    e = deck_plock_alloc(pat_ix, voice, st);
    if (!e)
        return;
    deck_plock_seed_step_x(e, p, voice, st);
    if (slot == 0u) {
        uint8_t gate = deck_sx_gate(e->step_x);

        if (edit)
            gate = steps < 0 ? 0u : DECK_SEQ_GATE_DEF;
        else if (steps != 0)
            adj_u8(&gate, steps, 0, DECK_SEQ_GATE_TIE);
        else
            adj_u8(&gate, d, 0, DECK_SEQ_GATE_TIE);
        e->step_x = (uint8_t)((e->step_x & (uint8_t)~0xFu) | (gate & 0xFu));
    } else if (slot == 2u) {
        uint8_t lv = deck_sx_slide(e->step_x);

        if (edit && steps != 0)
            lv = steps < 0 ? 0u : DECK_SEQ_LV_FULL;
        else if (steps != 0)
            adj_u8(&lv, steps < 0 ? -1 : 1, 0, DECK_SEQ_LV_FULL);
        e->step_x = deck_sx_with_slide(e->step_x, lv);
    } else {
        uint8_t rat = deck_sx_ratchet(e->step_x);

        if (edit && steps != 0)
            rat = steps < 0 ? DECK_SEQ_RATCHET_MIN : DECK_SEQ_RATCHET_MAX;
        else if (steps != 0)
            adj_u8(&rat, steps, DECK_SEQ_RATCHET_MIN, DECK_SEQ_RATCHET_MAX);
        else
            adj_u8(&rat, d, DECK_SEQ_RATCHET_MIN, DECK_SEQ_RATCHET_MAX);
        e->step_x = (uint8_t)((e->step_x & (uint8_t)~(3u << DECK_SX_RATCH_SHIFT)) |
                              (((rat - DECK_SEQ_RATCHET_MIN) & 3u) << DECK_SX_RATCH_SHIFT));
    }
    deck_plock_free_if_default(e);
    mark_dirty();
}

/* Lane seq mode (UI page 4, seq_page 4). FRESH: random hits, density per voice family; accents cleared. */
static void deck_seq_lane_fresh(deck_lane_t *ln, uint8_t voice)
{
    uint32_t len = ln->length ? ln->length : FM_SEQ_PAGE_STEPS;
    uint32_t lo = 2u, span = 4u, n, placed = 0u, tries = 0u;

    if (voice == DECK_BD) {
        lo = 2u;
        span = 5u; /* kick 2..6 */
    } else if (voice == DECK_CH || voice == DECK_OH || voice == DECK_RM || voice == DECK_RD) {
        lo = 6u;
        span = 7u; /* hats / ride 6..12 */
    }
    fm_seq_bits_fill(ln->hit, 0);
    fm_seq_bits_fill(ln->accent, 0);
    fm_seq_bits_fill(ln->accent_cut, 0);
    n = (lo + deck_rand() % span) * len / FM_SEQ_PAGE_STEPS;
    if (n == 0u)
        n = 1u;
    if (n > len)
        n = len;
    while (placed < n && tries < 256u) {
        uint32_t st = deck_rand() % len;

        tries++;
        if (!fm_seq_bit(ln->hit, st)) {
            fm_seq_bit_put(ln->hit, st, 1);
            placed++;
        }
    }
}

/* EUCLID: fm_seq_euclid16 pattern (16-step, repeated over the lane length); accents cleared. */
static void deck_seq_lane_euclid(deck_lane_t *ln)
{
    uint16_t m = fm_seq_euclid16(ln->euclid_hits, ln->euclid_rot);
    uint32_t s;

    for (s = 0u; s < (uint32_t)FM_SEQ_STEPS; s++) {
        fm_seq_bit_put(ln->hit, s, (s < ln->length && ((m >> (s & 15u)) & 1u)) ? 1 : 0);
        fm_seq_bit_put(ln->accent, s, 0);
        fm_seq_bit_put(ln->accent_cut, s, 0);
    }
}

/* Writes the lane's current mode into its hit bits. EDITED keeps the bits as they are. */
static void deck_seq_lane_apply_mode(deck_pattern_t *p, uint8_t voice)
{
    deck_lane_t *ln;

    if (!p || voice >= DECK_NVOICES)
        return;
    ln = &p->lane[voice];
    if (ln->euclid_hits == FM_SEQ_EUCLID_EMPTY) {
        fm_seq_bits_fill(ln->hit, 0);
        fm_seq_bits_fill(ln->accent, 0);
        fm_seq_bits_fill(ln->accent_cut, 0);
    } else if (ln->euclid_hits == FM_SEQ_EUCLID_SONG) {
        deck_seq_bake_lane(p, voice, g.era, (uint8_t)(g.plate % DECK_PLATES_PER_ERA));
    } else if (ln->euclid_hits == FM_SEQ_EUCLID_FRESH) {
        deck_seq_lane_fresh(ln, voice);
    } else if (fm_seq_euclid_is_pattern(ln->euclid_hits)) {
        deck_seq_lane_euclid(ln);
    }
}

/* KIT-only MODE step: same order as fm_seq_euclid_step (song_avail=1, edit=0), but hard-stops at EMPTY and EDITED
 * instead of wrapping. The shared fm_seq_euclid_step (GENRE/RYTM/CTRL) is left unchanged. */
static uint8_t deck_seq_euclid_step_clamped(uint8_t eh, int32_t dir, uint8_t pattern_origin)
{
    int32_t idx;

    if (eh == FM_SEQ_EUCLID_EDITED && dir > 0 && fm_seq_euclid_is_pattern(pattern_origin))
        return pattern_origin;
    idx = (int32_t)fm_seq_euclid_to_idx(eh) + (dir > 0 ? 1 : -1);
    if (idx < 0)
        idx = 0;
    if (idx >= (int32_t)FM_SEQ_EUCLID_CYCLE_N)
        idx = (int32_t)FM_SEQ_EUCLID_CYCLE_N - 1;
    return fm_seq_euclid_from_idx(idx);
}

/* MODE knob: EMPTY / SONG / FRESH / EUC 1..16 / EDITED (song always available). Mirrors GENRE seq_knob_euclid_hits. */
static void deck_seq_lane_mode_step(deck_pattern_t *p, uint8_t voice, int32_t dir)
{
    deck_lane_t *ln;
    uint8_t oh;
    uint8_t nh;

    if (!p || voice >= DECK_NVOICES || dir == 0)
        return;
    ln = &p->lane[voice];
    oh = ln->euclid_hits;
    nh = deck_seq_euclid_step_clamped(oh, dir, ln->pattern_origin);
    if (nh == FM_SEQ_EUCLID_EDITED) {
        if (oh != FM_SEQ_EUCLID_EDITED)
            fm_seq_lane_pattern_mark_edited(&ln->euclid_hits, &ln->pattern_origin);
        return;
    }
    if (oh == FM_SEQ_EUCLID_EDITED && dir > 0 && ln->pattern_origin != FM_SEQ_PATTERN_ORIGIN_NONE)
        ln->pattern_origin = FM_SEQ_PATTERN_ORIGIN_NONE;
    ln->euclid_hits = nh;
    deck_seq_lane_apply_mode(p, voice);
}

/* ROTATE knob (GENRE ROTATE): EUC re-applies the rotated mask; other modes rotate the existing hit/accent bits per 16-step page (as GENRE seq_rotate_lane), which marks the lane EDITED when something moved. */
static void deck_seq_lane_rot_step(deck_pattern_t *p, uint8_t voice, int32_t dir)
{
    deck_lane_t *ln;
    uint8_t hit0[FM_SEQ_MASK_BYTES];
    int32_t n;

    if (!p || voice >= DECK_NVOICES || dir == 0)
        return;
    ln = &p->lane[voice];
    ln->euclid_rot = (int8_t)fm_seq_euclid_rot_norm((int32_t)fm_seq_euclid_rot_norm(ln->euclid_rot) + dir);
    if (fm_seq_euclid_is_pattern(ln->euclid_hits)) {
        deck_seq_lane_euclid(ln);
        return;
    }
    if (ln->euclid_hits == FM_SEQ_EUCLID_EMPTY)
        return;
    memcpy(hit0, ln->hit, sizeof hit0);
    for (n = 0; n < (dir > 0 ? dir : -dir); n++) {
        fm_seq_bits_rot_pages(ln->hit, dir);
        fm_seq_bits_rot_pages(ln->accent, dir);
        fm_seq_bits_rot_pages(ln->accent_cut, dir);
    }
    if (memcmp(hit0, ln->hit, sizeof hit0) != 0)
        fm_seq_lane_pattern_mark_edited(&ln->euclid_hits, &ln->pattern_origin);
}

/* LANE knob (SEQ p4 slot 0): DECK_SELECT_ORDER voices, then ALL; wraps. ALL leaves cur_voice untouched. */
#define DECK_LANE_SEL_N (DECK_NVOICES + 1u)
static void deck_lane_sel_step(deck_runtime_t *r, int32_t dir)
{
    int32_t n = (int32_t)DECK_LANE_SEL_N;
    int32_t pos = deck_seq_g.lane_all ? (int32_t)DECK_NVOICES : (int32_t)deck_select_order_index(r->cur_voice);

    pos = ((pos + dir) % n + n) % n;
    if (pos == (int32_t)DECK_NVOICES) {
        deck_seq_g.lane_all = 1u;
        return;
    }
    deck_seq_g.lane_all = 0u;
    r->cur_voice = DECK_SELECT_ORDER[pos];
}

/* NUDGE (SEQ lane page slot 3), 1:1 with genre seq_lane_nudge_edit; per lane across all patterns. ALL: relative
 * delta on every lane (EDIT + turn = ±1, else steps or d). Single lane: EDIT + turn = min/max, else +/- (steps or d). */
static void deck_seq_nudge_edit(uint8_t pat_ix, uint8_t voice, int32_t steps, uint8_t edit, int32_t d)
{
    uint8_t v;

    (void)pat_ix;
    if (deck_seq_g.lane_all) {
        int32_t delta = edit && steps != 0 ? (steps < 0 ? -1 : 1) : (steps != 0 ? steps : d);

        for (v = 0u; v < DECK_NVOICES; v++)
            deck_seq_lane_nudge_put(v, fm_seq_lane_nudge_clamp((int32_t)deck_seq_lane_nudge_get(0u, v) + delta));
        return;
    }
    {
        int32_t nv;

        if (edit && steps != 0)
            nv = steps < 0 ? (int32_t)FM_SEQ_LANE_NUDGE_MIN : (int32_t)FM_SEQ_LANE_NUDGE_MAX;
        else
            nv = (int32_t)deck_seq_lane_nudge_get(0u, voice) + (steps != 0 ? steps : d);
        deck_seq_lane_nudge_put(voice, fm_seq_lane_nudge_clamp(nv));
    }
}

static void deck_knob_adjust_seq(uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    uint8_t voice = r->cur_voice % DECK_NVOICES;
    deck_lane_t *ln = &p->lane[voice];
    uint8_t pg = g.seq_page;

    if (pg == 0u) {
        if (!deck_seq_step_editable(r))
            return;
        deck_knob_adjust_seq_gate(slot, steps, d, edit);
        return;
    }
    if (pg == 1u) {
        deck_seq_knob_adjust_page1(ln, slot, steps, d, edit);
        mark_dirty();
        return;
    }
    if (pg == 3u) {
        if (slot == 0u)
            deck_seq_set_length(p, voice, (uint8_t)((int32_t)ln->length + (steps != 0 ? steps : d)));
        else if (slot == 1u) {
            uint8_t pos = deck_lane_p3_state_pos(r, voice);
            int32_t nx = (int32_t)pos + (steps != 0 ? steps : d);

            while (nx < 0)
                nx += 3;
            deck_lane_apply_p3_pos(r, voice, (uint8_t)((uint32_t)nx % 3u));
        } else if (slot == 2u) {
            int32_t dv = (int32_t)fm_seq_clk_ratio_lane_ix(ln->div) + (steps != 0 ? steps : d);

            if (dv < 0)
                dv = 0;
            if (dv > (int32_t)FM_SEQ_CLK_RATIO_N - 1)
                dv = (int32_t)FM_SEQ_CLK_RATIO_N - 1;
            deck_seq_set_div(p, voice, (uint8_t)dv);
        } else if (pg == 3u && slot == 3u) {
            deck_seq_nudge_edit(r->cur_pat, voice, steps, edit, d);
        }
        mark_dirty();
        return;
    }
    if (pg == 2u) {
        if (slot == 0u) {
            g.seq_follow = 0u;
            fm_seq_view_page_step(&r->bar_page, steps != 0 ? steps : d);
        }
        else if (slot == 1u) {
            if (edit)
                deck_apply_ensemble(r, steps < 0 ? FM_SEQ_ENS_MUTE : FM_SEQ_ENS_ALL);
            else if (steps != 0) {
                int32_t ix = (int32_t)g.seq_ens_sel + (steps > 0 ? 1 : -1);

                if (ix < 0)
                    ix = 0;
                if (ix >= (int32_t)FM_SEQ_NENSEMBLES)
                    ix = (int32_t)FM_SEQ_NENSEMBLES - 1;
                g.seq_ens_sel = (uint8_t)ix;
                deck_apply_ensemble(r, g.seq_ens_sel);
                deck_toast(deck_ens_combo_name(g.seq_ens_sel), C_ACC);
            }
        } else if (slot == 2u && steps != 0) {
            deck_invert_all_lane_mutes(r);
            g.seq_flip_state ^= 1u;
        }
        mark_dirty();
        return;
    }
    if (pg == 4u) {
        int32_t dir = steps != 0 ? steps : d;

        if (slot == 0u) {
            /* LANE: voice stepping + ALL stop; family sync and SEQ lane page follow only for a voice. */
            if (dir == 0)
                return;
            deck_lane_sel_step(r, dir > 0 ? 1 : -1);
            if (!deck_seq_g.lane_all) {
                deck_pad_family_sync_voice(r->cur_voice);
                if (g.view == DECK_VIEW_SEQ)
                    deck_lane_page_follow_voice(r);
            }
        } else if (slot == 1u)
            deck_seq_lane_mode_step(p, voice, dir);
        else if (slot == 2u)
            deck_seq_lane_rot_step(p, voice, dir);
        else {
            int32_t v = (int32_t)ln->vary + dir * 3;

            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            ln->vary = (uint8_t)v;
        }
        mark_dirty();
    }
}

static void deck_plock_latch_end(void)
{
    g.seq_plock_latch = 0u;
    g.seq_plock_hold = 0u;
    g.seq_plock_step = FM_SEQ_PLOCK_NONE;
    g.seq_plock_knob = 0u;
    g.seq_plock_bank = 0u;
    g.seq_page = g.seq_plock_page_saved;
    if (!deck_seq_page_in_cycle(g.seq_page))
        g.seq_page = DECK_SEQ_KNOB_PAGE_CYCLE[0];
    g.white_press_k = 255u;
    g.white_press_ms = 0u;
    g.white_long_used = 0u;
    g.white_long_cancel = 0u;
    mark_dirty();
}

static void deck_plock_latch_begin(uint8_t st)
{
    g.seq_plock_page_saved = g.seq_page;
    g.seq_plock_latch = 1u;
    g.seq_plock_hold = 1u;
    g.seq_plock_step = st;
    g.seq_plock_knob = 0u;
    g.seq_plock_bank = 0u;
}

static void deck_plock_latch_touch_break(uint32_t btn_edges, uint32_t note_rising)
{
    if (!g.seq_plock_latch)
        return;
    if (btn_edges) {
        if (btn_edges == (1u << BTN_SEQ))
            return;
        deck_plock_latch_end();
        return;
    }
    if (note_rising) {
        deck_runtime_t *r = rt();
        uint32_t base = deck_seq_view_base(r);
        uint32_t k;

        for (k = 0u; k < 16u; k++) {
            if (!(note_rising & (1u << KEY_WHITE[k])))
                continue;
            if ((uint8_t)(base + k) != g.seq_plock_step)
                deck_plock_latch_end();
            break;
        }
    }
}

static void deck_white_plock_latch_poll(uint32_t keys, uint32_t rising)
{
    ko_longpress_t lp = {&g.white_press_ms, &g.white_long_used, &g.white_long_cancel};
    uint32_t bit;
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    uint32_t base;
    uint8_t st;

    if (g.view != DECK_VIEW_SEQ || g.white_press_k >= 16u)
        return;
    bit = 1u << KEY_WHITE[g.white_press_k];
    if (!(keys & bit))
        return;
    if (!ko_longpress_poll(&lp, fm1_ms, DECK_HOLD_MS, (rising & ~bit) != 0u))
        return;
    base = deck_seq_view_base(r);
    st = (uint8_t)(base + g.white_press_k);
    if (g.seq_plock_latch && g.seq_plock_step == st) {
        deck_plock_latch_end();
        return;
    }
    if (!deck_seq_get_hit(p, r->cur_voice, st))
        deck_seq_toggle_step(p, r, r->cur_voice, g.white_press_k);
    deck_plock_latch_begin(st);
    mark_dirty();
}

static void deck_top_bar_seq_p_dots(const ko_top_bar_t *tb)
{
    int32_t dx = 238 - ((int32_t)DECK_SEQ_KNOB_PAGE_CYCLE_N - 1) * 8 - 5;

    if (g.seq_plock_latch)
        fm_seq_draw_page_dots(238 - 8 - 5, 4, g.seq_plock_bank, 2u);
    else
        deck_seq_draw_page_cycle_dots(dx, 4, g.seq_page);
    (void)tb;
}

/* S1..S8 pattern view dots in the SEQ top bar (GENRE top_bar_seq_step_dots): pages beyond the pattern length are dim,
 * the playhead page is the block, the viewed page the frame. Display only. */
static void deck_top_bar_seq_step_dots(const ko_top_bar_t *tb)
{
    deck_runtime_t *r = rt();
    uint8_t on[FM_SEQ_MAX_PAGES];
    uint8_t pages = deck_seq_pattern_pages(pat());
    uint32_t len = (uint32_t)pages * FM_SEQ_PAGE_STEPS;
    uint8_t cv = r->cur_voice % DECK_NVOICES;
    uint32_t i;

    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
        on[i] = (uint8_t)(i < pages);
    fm_seq_draw_step_dots_at(tb->seq_x0, 14 + 4, (uint8_t)(r->bar_page % FM_SEQ_MAX_PAGES),
                             (uint8_t)(len ? (r->master_step % len) / FM_SEQ_PAGE_STEPS : 0u), on, r->playing,
                             deck_vcol(cv));
}

static void deck_draw_top(void)
{
    deck_runtime_t *r = rt();
    ko_top_bar_t t = {0};
    ko_wave_ctx_t w = {0};
    uint8_t cv = r->cur_voice % DECK_NVOICES;

    w.static_glow = 1;
    w.glow = (int32_t)g.kick_pump >> 1;
    w.sine_q64 = deck_wave_sine;
    t.wave = &w;
    t.show_title = 1u;
    {
        const char *pn = deck_plate_name(g.plate);
        const char *en = deck_era_name(g.era);
        uint32_t i;

        for (i = 0u; i < 7u && pn[i]; i++)
            t.title[i] = pn[i];
        if (deck_plate_ov_get(g.plate))
            t.title[i++] = '*';
        t.title[i] = '\0';
        for (i = 0u; i < 7u && en[i]; i++)
            t.sub[i] = en[i];
        t.sub[i] = '\0';
    }
    ko_top_bar_era_plate_cols(&t, deck_era_col, deck_era_of_plate(g.plate), g.era);
    t.cursor = -1;
    t.rec = r->rec_arm;
    if (g.view == DECK_VIEW_SEQ) {
        t.dots = deck_top_bar_seq_p_dots;
        t.seq_x0 = ((int32_t)KO_TOP_BAR_W - (int32_t)FM_SEQ_MAX_PAGES * 8 + 3) / 2;
        t.seq_w = (int32_t)FM_SEQ_MAX_PAGES * 8 - 3;
        t.seq_dots = deck_top_bar_seq_step_dots;
        deck_u8_dec(t.seq_txt[0], (uint32_t)((r->bar_page % FM_SEQ_MAX_PAGES) + 1u));
        t.seq_col[0] = C_WHITE;
        if (r->playing) {
            deck_u8_dec(t.seq_txt[1], (uint32_t)((r->master_step % FM_SEQ_PAGE_STEPS) + 1u));
            t.seq_col[1] = deck_vcol(cv);
        }
    } else {
        t.pages = deck_bank_pages(g.bank);
        t.page = deck_bank_page();
    }
    t.stage = g.view == DECK_VIEW_SEQ ? "SEQ" :
              (g.bank == DECK_BANK_FX ? deck_fx_page_name(deck_bank_page()) : DECK_BANK_NAME[g.bank]);
    t.key = "";
    t.scale = DECK_VNAME_LONG[cv];
    t.seq = (uint8_t)(g.view == DECK_VIEW_SEQ);
    deck_fmt_u8(t.bpm, r->bpm);
    t.swing[0] = 'S';
    deck_fmt_u8(t.swing + 1, r->swing);
    t.playing = r->playing;
    t.on_one = (uint8_t)(r->playing && (r->master_step % 16u) == 0u);
    t.on_beat = (uint8_t)(r->playing && (r->master_step % 4u) == 0u);
    t.flash = &g.flash;
    t.now = fm1_ms;
    ko_draw_top_bar(&t);
}

static int32_t deck_knob_val_lfo_shape(const ko_knob_slot_t *k, int32_t x)
{
    const fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];

    return fm_lfo_shape_draw(x, 18, ls->wave, k->vcol, C_DIM);
}

static void deck_knob_fill(uint32_t col, ko_knob_slot_t *k)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    deck_voice_cfg_t *vc = &g.dsp.cfg[r->cur_voice];
    deck_master_t *fx = &g.dsp.master;
    deck_perf_t *pf = &g.dsp.perf;
    uint8_t pg = deck_arp_ov_live() ? 0u : deck_bank_page();
    uint8_t b = deck_arp_ov_live() ? DECK_BANK_ARP : g.bank;
    uint8_t v = 0u;
    uint8_t mode = KNOB_POT_CONT;
    uint8_t steps = 0u;
    int bip = 0;
    int raw_unit = b == DECK_BANK_GLO && (pg == 0u ? col >= 2u : col == 2u);
    const char *lab = "";

    k->klab = C_GRAY;
    k->vcol = C_WHITE;
    k->lcol = C_GRAY;
    k->pot_col = deck_vcol(r->cur_voice);
    k->pot_track = 0u;
    k->mark_col = C_ACC;
    k->label[0] = k->value[0] = '\0';
    k->custom_label = NULL;
    k->custom_value = NULL;

    if (g.view == DECK_VIEW_SEQ) {
        deck_seq_knob_fill(col, k, r, p);
        return;
    }

    if (b == DECK_BANK_SEL && pg == 0u) {
        static const char *L[4] = {"TIMBRE", "LEVEL", "TUNE", "FILTER"};
        uint8_t *pp[4] = {&vc->timbre, &vc->level, &vc->tune, &vc->filter};
        lab = L[col];
        v = *pp[col];
        if (col == 2u)
            bip = 1;
    } else if (b == DECK_BANK_SEL && pg == 1u) {
        static const char *L[4] = {"PAN", "NOISE", "DRIVE", "CRUSH"};
        uint8_t *pp[4] = {&vc->pan, &vc->noise, &vc->drive, &vc->crush};
        lab = L[col];
        v = *pp[col];
        if (col == 0u) {
            int32_t pn = (int32_t)v - 64;
            k->pan = 1u;
            k->pan_side = (int8_t)(pn < 0 ? -1 : pn > 0 ? 1 : 0);
            deck_pan_num(k->pan_num, pn);
            bip = 1;
        }
    } else if (b == DECK_BANK_SEL && pg == 2u) {
        static const char *L[4] = {"VEL", "DUCK", "DELAY", "REVERB"};
        lab = L[col];
        if (col == 0u)
            v = vc->vel_sens;
        else if (col == 1u) {
            v = deck_kick_duck_locked(b, pg, col) ? 0u : vc->duck;
            if (r->cur_voice % DECK_NVOICES == DECK_BD)
                k->pot_col = C_DIM;
        } else {
            v = col == 2u ? vc->dly_send : vc->rev_send;
            if (g.fx_bypass)
                k->pot_col = C_DIM;
        }
    } else if (b == DECK_BANK_ENV && pg == 0u) {
        lab = FM_ENV_KIT_LBL[0][col];
        if (col == 0u)
            v = vc->click;
        else if (col == 1u)
            v = vc->decay;
        else if (col == 2u)
            v = vc->sustain;
        else
            v = vc->release;
    } else if (b == DECK_BANK_ENV && pg == 1u) {
        lab = FM_ENV_KIT_LBL[1][col];
        if (col == 0u)
            v = g.env_delay[r->cur_voice % DECK_NVOICES];
        else if (col == 1u)
            v = vc->hold;
        else if (col == 2u) {
            v = vc->curve;
            k->custom_value = fm_env_kit_knob_val_shape;
            bip = 1;
        } else {
            v = g.env_retrig[r->cur_voice % DECK_NVOICES] ? 127u : 0u;
            mode = KNOB_POT_TOG;
            steps = 2u;
        }
    } else if (b == DECK_BANK_ENV && pg == 2u) {
        lab = FM_ENV_KIT_LBL[2][col];
        if (col == 0u) {
            v = g.env_invert[r->cur_voice % DECK_NVOICES] ? 127u : 0u;
            mode = KNOB_POT_TOG;
            steps = 2u;
        }         else if (col == 1u)
            v = vc->pitch_dec;
        else if (col == 2u) {
            v = vc->pitch_env;
            bip = 1;
        }
    } else if (b == DECK_BANK_GLO && pg == 0u) {
        lab = DECK_GROOVE_P0_LBL[col];
        if (col == 0u) {
            v = r->key;
            mode = KNOB_POT_TICK;
            steps = 12u;
        } else if (col == 1u) {
            v = r->scale;
            mode = KNOB_POT_TICK;
            steps = DECK_SCALE_N;
        } else if (col == 2u) {
            v = r->swing;
        } else
            v = r->bpm;
    } else if (b == DECK_BANK_GLO && pg == 2u) {
        lab = DECK_GROOVE_P2_LBL[col];
        if (col == 0u)
            v = g.human;
        else if (col == 1u)
            v = deck_seq_g.timing_hum;
    } else if (b == DECK_BANK_GLO && pg == 1u) {
        if (col == 0u) {
            lab = "ACC";
            v = g.accent_amt;
        } else if (col == 1u) {
            lab = "TUNE";
            v = pf->tune;
            bip = 1;
        } else if (col == 2u) {
            lab = "MIDI";
            v = (uint8_t)(g.midi_ch + 1u);
            mode = KNOB_POT_TICK;
            steps = 16u;
        } else {
            lab = DECK_CLOCK_NAME[g.clock & 1u];
            v = g.clock ? 127u : 0u;
            mode = KNOB_POT_TOG;
            steps = 2u;
        }
    } else if (b == DECK_BANK_FX && pg == 0u) {
        static const char *L[4] = {"MIX", "TIME", "FEEDBK", "TONE"};
        uint8_t *pp[4] = {&fx->dly_lvl, &fx->dly_time, &fx->feedback, &fx->loop_filt};
        lab = L[col];
        v = *pp[col];
        if (col == 0u) {
            k->custom_arg = ((uint32_t)fx->dly_lvl * 255u) / 127u;
            k->custom_label = deck_knob_lbl_dry_wet;
        } else if (col == 3u)
            bip = 1;
        if (g.fx_bypass)
            k->pot_col = C_DIM;
    } else if (b == DECK_BANK_FX && pg == 1u) {
        static const char *L[4] = {"WOW", "TAP", "TANK", "ISO"};
        uint8_t *pp[4] = {&fx->wow, &fx->tap, &fx->tank, &fx->isolator};
        lab = L[col];
        v = *pp[col];
        if (col == 3u)
            bip = 1;
        if (col == 1u) {
            mode = KNOB_POT_ORD;
            steps = DECK_TAP_N;
        }
        if (g.fx_bypass)
            k->pot_col = C_DIM;
    } else if (b == DECK_BANK_FX && pg == 2u) {
        static const char *L[4] = {"REVERB", "DAMP", "TAIL", "BLEED"};
        uint8_t *pp[4] = {&fx->rev_mix, &fx->rev_damp, &fx->rev_tail, &fx->dly_rev};
        lab = L[col];
        v = *pp[col];
        if (g.fx_bypass)
            k->pot_col = C_DIM;
    } else if (b == DECK_BANK_FX && pg == 3u) {
        static const char *L[4] = {"DRIVE", "CRUSH", "FILTER", "WIDTH"};
        uint8_t *pp[4] = {&fx->drive, &fx->crush, &fx->filter, &fx->width};
        lab = L[col];
        v = *pp[col];
        if (col == 2u)
            bip = 1;
        if (g.fx_bypass)
            k->pot_col = C_DIM;
    } else if (b == DECK_BANK_FX && pg == 4u) {
        static const char *L[4] = {"COMP", "DUST", "TAPE", ""};
        uint8_t *pp[3] = {&fx->comp, &fx->dust, &fx->tape};
        lab = L[col];
        if (col < 3u)
            v = *pp[col];
        else
            k->pot_col = C_DIM;
        if (g.fx_bypass)
            k->pot_col = C_DIM;
    } else if (b == DECK_BANK_LFO && pg == 0u) {
        fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];
        if (col == 0u) {
            lab = DECK_LFO_DEST[g.lfo_tgt % DECK_LFO_TGT_N];
            v = deck_lfo_tgt_knob_val(g.lfo_tgt);
            mode = KNOB_POT_TICK;
            steps = deck_lfo_dests.n ? deck_lfo_dests.n : 1u;
            if (steps > KO_POT_TICK_CAP)
                steps = KO_POT_TICK_CAP;
        } else if (col == 1u) {
            lab = "RATE";
            v = ls->rate;
        } else if (col == 2u) {
            lab = "DEPTH";
            v = ls->depth;
        } else {
            lab = "SHAPE";
            v = ls->wave;
            mode = KNOB_POT_SHAPE;
            steps = KNOB_POT_SHAPE_N;
            k->custom_value = deck_knob_val_lfo_shape;
        }
    } else if (b == DECK_BANK_LFO && pg == 1u) {
        fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];
        if (col == 0u) {
            lab = "PHASE";
            v = ls->phase;
        } else if (col == 1u) {
            lab = "SKEW";
            v = ls->skew;
            bip = 1;
        } else if (col == 2u) {
            lab = "SMOOTH";
            v = ls->smooth;
        } else {
            lab = "FOLD";
            v = ls->fold;
        }
    } else if (b == DECK_BANK_LFO && pg == 2u) {
        fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];
        if (col == 0u) {
            lab = FM_LFO_ALG_SHORT[ls->algo % FM_LFO_NALGOS];
            v = (uint8_t)(((uint32_t)ls->algo * 127u) / (FM_LFO_NALGOS - 1u));
            mode = KNOB_POT_SELW;
            steps = FM_LFO_NALGOS;
        } else if (col == 1u) {
            lab = "RATIO";
            v = ls->ratio;
        } else if (col == 2u) {
            lab = "FM IDX";
            v = ls->index;
        } else {
            lab = "OP FB";
            v = ls->fb;
        }
    } else if (b == DECK_BANK_ARP) {
        if (col == 0u) {
            lab = DECK_ARP_RATE[g.arp_rate % 5u];
            v = (uint8_t)(((uint32_t)g.arp_rate * 127u) / 4u);
            mode = KNOB_POT_TICK;
            steps = 5u;
        } else if (col == 1u) {
            lab = "GATE";
            v = g.arp_gate;
        } else if (col == 2u) {
            lab = "RAMP";
            v = g.arp_ramp;
        } else {
            lab = "LATCH";
            v = g.arp_latch ? 127u : 0u;
            mode = KNOB_POT_TOG;
            steps = 2u;
        }
    } else if (b == DECK_BANK_HOME && pg == 0u) {
        uint8_t *pp[4] = {&pf->tune, &pf->decay, &pf->filter, &pf->crush};
        lab = DECK_HOME_P0_LBL[col];
        v = *pp[col];
        bip = 1;
    } else {
        if (col == 0u) {
            lab = "DENS";
            v = pf->density;
            bip = 1;
        } else if (col == 1u) {
            lab = "FILL";
            v = pf->fill;
        } else if (col == 2u) {
            lab = "COMBO";
            mode = KNOB_POT_MASK;
            steps = KNOB_POT_MASK_DECK_N;
        } else {
            lab = "STUT";
            v = pf->stutter;
        }
    }

    if (lab[0])
        deck_str_copy(k->label, lab);
    else
        k->label[0] = '\0';
    if (b == DECK_BANK_LFO && pg == 1u && col == 3u)
        fm_lfo_fmt_depth(k->value, (int32_t)g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N].fold);
    else if (b == DECK_BANK_LFO && pg == 2u && col == 1u)
        fm_lfo_fmt_ratio(k->value, g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N].ratio);
    else if (b == DECK_BANK_LFO && pg == 2u && col == 2u)
        fm_lfo_fmt_depth(k->value, (int32_t)g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N].index);
    else if (b == DECK_BANK_LFO && pg == 2u && col == 3u)
        fm_lfo_fmt_depth(k->value, (int32_t)g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N].fb);
    else if (b == DECK_BANK_FX)
        deck_fx_knob_value(pg, col, fx, r, k->value, v);
    else if (b == DECK_BANK_HOME && pg == 1u && col == 2u)
        deck_str_copy(k->value, deck_ens_combo_readout(r));
    else if (b == DECK_BANK_ENV && ((pg == 1u && col == 3u) || (pg == 2u && col == 0u)))
        deck_str_copy(k->value, (pg == 1u ? g.env_retrig[r->cur_voice % DECK_NVOICES] :
                                         g.env_invert[r->cur_voice % DECK_NVOICES]) ?
                                    "ON" :
                                    "OFF");
    else if (b == DECK_BANK_GLO && pg == 0u && col == 0u)
        deck_str_copy(k->value, DECK_NOTE_NAMES[v % 12u]);
    else if (b == DECK_BANK_GLO && pg == 0u && col == 1u)
        deck_str_copy(k->value, DECK_SCALE_NAMES[v % DECK_SCALE_N]);
    else if (!k->pan && !(b == DECK_BANK_LFO && pg == 0u && col == 3u) &&
             !(b == DECK_BANK_LFO && pg == 0u && col == 0u) && !(b == DECK_BANK_LFO && pg == 2u && col == 0u) &&
             !(b == DECK_BANK_ARP && col == 0u) && !(b == DECK_BANK_GLO && col == 1u) &&
             !(b == DECK_BANK_GLO && pg == 2u && col >= 2u) &&
             !(b == DECK_BANK_GLO && pg == 1u && col == 3u) &&
             !(b == DECK_BANK_HOME && pg == 1u && col == 2u)) {
        if (b == DECK_BANK_LFO && pg == 0u && col == 1u)
            deck_lfo_rate_show(k->value, v, r->bpm ? r->bpm : 120u);
        else
            deck_knob_num(k->value, v, bip, raw_unit);
    }
    if (g.view != DECK_VIEW_SEQ && b != DECK_BANK_LFO && !deck_arp_ov_live() && deck_dj_knob_live((uint8_t)col)) {
        k->dj = 1u;
        k->dj_col = deck_dj_role_col(&g.dj_app);
    }
    k->range = 0u;
    if (g.view != DECK_VIEW_SEQ && b != DECK_BANK_LFO && !deck_arp_ov_live() && !deck_kick_duck_locked(b, pg, col) &&
        deck_lfo_knob_modulated((uint8_t)col)) {
        uint8_t lt = deck_lfo_target_for_knob((uint8_t)col);
        const fm_lfo_slot_t *sl = &deck_lfo_sl(deck_lfo_vi())[lt];
        uint32_t bpm = r->bpm ? r->bpm : 120u;
        uint8_t lo, hi;
        uint8_t base_v = v;

        v = deck_lfo_mod_u8(v, lt, 0u, 127u);
        k->lfo = 1u;
        if (k->pan) {
            int32_t pn = (int32_t)v - 64;

            k->pan_side = (int8_t)(pn < 0 ? -1 : pn > 0 ? 1 : 0);
            deck_pan_num(k->pan_num, pn);
        } else if (!(b == DECK_BANK_ARP && col == 0u) && !(b == DECK_BANK_GLO && pg == 1u && col == 3u) &&
                   !(b == DECK_BANK_HOME && pg == 1u && col == 2u) &&
                   !(b == DECK_BANK_LFO && pg == 0u && col == 3u) && b != DECK_BANK_FX) {
            if (mode == KNOB_POT_CONT) {
                deck_knob_num(k->value, v, bip, raw_unit);
                if (!k->custom_value &&
                    fm_lfo_readout_update((uint8_t)col, v, base_v, sl->depth, sl->rate, bpm, &lo, &hi)) {
                    k->range = 1u;
                    deck_knob_num(k->range_lo, lo, bip, raw_unit);
                    deck_knob_num(k->range_hi, hi, bip, raw_unit);
                }
            }
            /* Discrete: leave the base label from earlier fill; LFO mark only. */
        }
    }
    if (b == DECK_BANK_FX)
        deck_fx_knob_value(pg, col, fx, r, k->value, v);
    {
        uint8_t pv = v;

        if (b == DECK_BANK_FX && pg == 1u && col == 1u)
            pv = (uint8_t)(((uint32_t)(fx->tap % DECK_TAP_N) * 127u) / (DECK_TAP_N - 1u));
        else if (b == DECK_BANK_GLO && pg == 0u) {
            if (col == 0u)
                pv = (uint8_t)((uint32_t)v * 127u / 11u);
            else if (col == 1u)
                pv = (uint8_t)((uint32_t)v * 127u / (DECK_SCALE_N - 1u));
            else if (col == 2u) {
                uint8_t sw = r->swing;

                pv = (uint8_t)(sw < 50u ? 0u : (sw - 50u) * 127u / 25u);
            } else
                pv = deck_bpm_pot_px(v);
        } else if (b == DECK_BANK_GLO && pg == 1u && col == 2u) {
            pv = (uint8_t)(((uint32_t)(g.midi_ch % 16u) * 127u) / 15u);
        } else if (b == DECK_BANK_HOME && pg == 1u && col == 2u) {
            uint16_t on = deck_lane_on_mask16(r);

            k->custom_arg = (uint32_t)on;
            pv = (uint8_t)(on & 0xFFu);
        }
        k->pv = pv;
    }
    if (b == DECK_BANK_ENV && pg == 1u && col == 2u)
        k->custom_arg = ((uint32_t)v * 255u) / 127u;
    k->bip = (uint8_t)bip;
    k->steps = steps;
    k->mode = mode;
    if (b == DECK_BANK_HOME) {
        uint16_t era = deck_era_col(g.era);

        k->pot_col = era;
        k->pot_track = deck_ui_scale(era, 52);
    }
}

enum {
    /* Same strip as KO BASS (DUB_KNOB_MARK_Y + 2 inside the 86px knob bar). */
    DECK_KNOB_PROG_Y = 78
};

static void deck_knob_bar_progress_rail(int32_t y_bot)
{
    deck_runtime_t *r = rt();

    deck_ui_knob_progress_rail(r, deck_era_col(g.era), y_bot);
}

static void deck_draw_knobs(void)
{
    ko_knob_bar_t bar = {0};

    bar.h = 86;
    bar.blit_y = 154;
    bar.rail_y = DECK_KNOB_PROG_Y;
    bar.rail = deck_knob_bar_progress_rail;
    bar.center_label = 1u;
    bar.fill = deck_knob_fill;
    ko_draw_knob_bar(&bar);
}

static void deck_home_hud_for_ui(deck_home_hud_t *h)
{
    uint8_t col;

    h->plate_slot = deck_plate_slot(g.plate);
    h->era = g.era;
    h->plate_era = deck_era_of_plate(g.plate);
    for (col = 0u; col < 4u; col++) {
        h->lfo_on[col] = 0u;
        h->lfo_depth[col] = 0u;
        h->lfo_ph[col] = 0u;
        if (g.view == DECK_VIEW_SEQ || g.bank == DECK_BANK_LFO || g.bank == DECK_BANK_ARP)
            continue;
        if (g.bank == DECK_BANK_GLO && deck_bank_page() == 0u && col != 0u)
            continue;
        if (deck_lfo_knob_modulated(col)) {
            uint8_t lt = deck_lfo_target_for_knob(col);

            h->lfo_on[col] = 1u;
            h->lfo_depth[col] = g.lfo[deck_lfo_vi()][lt].depth;
            h->lfo_ph[col] = g.lfo_rt[deck_lfo_vi()][lt].ph;
        }
    }
}

static void deck_home_perf_for_ui(deck_perf_t *o, uint8_t *accent, uint8_t *human)
{
    const deck_perf_t *b = &g.dsp.perf;

    o->tune = deck_lfo_mod_u8(b->tune, DECK_LFO_PTUNE, 0u, 127u);
    o->decay = deck_lfo_mod_u8(b->decay, DECK_LFO_PDECAY, 0u, 127u);
    o->filter = deck_lfo_mod_u8(b->filter, DECK_LFO_PFILTER, 0u, 127u);
    o->crush = deck_lfo_mod_u8(b->crush, DECK_LFO_PCRUSH, 0u, 127u);
    o->density = deck_lfo_mod_u8(b->density, DECK_LFO_DENS, 0u, 127u);
    o->fill = deck_lfo_mod_u8(b->fill, DECK_LFO_FILL, 0u, 127u);
    o->mute_grp = b->mute_grp;
    o->stutter = deck_lfo_mod_u8(b->stutter, DECK_LFO_STUT, 0u, 127u);
    *accent = deck_lfo_mod_u8(g.accent_amt, DECK_LFO_ACCENT, 0u, 127u);
    *human = deck_lfo_mod_u8(g.human, DECK_LFO_HUMAN, 0u, 127u);
}

/* UI tick: prune lanes, then sticky labels. Order as fm_app perf_toast_pick_sticky: ARP overlay before DJ live.
 * ARP is set after DJ in the same tick, so it wins MAIN. Side labels ride MAIN with no fade (side_end 0). */
static void deck_toast_lanes_sync(const deck_runtime_t *r)
{
    perf_toast_lane_t *main = &deck_lanes[PERF_TOAST_LANE_MAIN];
    const char *side = g.dj_app.toast_side;
    uint8_t has_side = (side && side[0]) ? 1u : 0u;
    uint8_t dj_on = g.dj_app.dj.on ? 1u : 0u;
    uint8_t drop_break = (fm_dj_any_slot_role(&g.dj_app.dj, FM_DJ_ROLE_DROP) ||
                          fm_dj_any_slot_role(&g.dj_app.dj, FM_DJ_ROLE_BREAK))
                             ? 1u
                             : 0u;

    perf_toast_policy_lane_prune(deck_lanes, fm1_ms);
    if (perf_toast_policy_dj_main_live(dj_on, r->playing, has_side)) {
        perf_toast_policy_lane_set_main(main, "DJ", drop_break, -1, 0u, fm1_ms, has_side ? side : "", 0u);
        deck_lane_bar[PERF_TOAST_LANE_MAIN] = C_ACC;
    } else if (main->on && (perf_toast_policy_is_dj(main->msg) || perf_toast_policy_same_msg(main->msg, "ARP")))
        perf_toast_policy_lane_clear(main);
    if (deck_arp_ov_live()) {
        perf_toast_policy_lane_set_main(main, "ARP", 0u, -1, 0u, fm1_ms, DECK_ARP_RATE[g.arp_rate % 5u], 0u);
        deck_lane_bar[PERF_TOAST_LANE_MAIN] = C_ACC;
    }
}

/* Lane draw order as fm_app draw_perf_toast: EPH, PROC, MAIN. Stack rows: MAIN centre, PROC one row up, EPH two rows up. */
static void deck_toast_lanes_draw(void)
{
    perf_toast_lane_t *proc = &deck_lanes[PERF_TOAST_LANE_PROC];
    perf_toast_lane_t *main = &deck_lanes[PERF_TOAST_LANE_MAIN];
    perf_toast_lane_t *eph = &deck_lanes[PERF_TOAST_LANE_EPH];
    int32_t bh = toast_lane_stack_bh((const toast_font_t *)&FONT_S);
    int32_t by_center = ko_toast_by_center(bh);

    if (eph->on) {
        ko_toast_t t = {eph->msg, 0, eph->kind, eph->end_ms,
                        perf_toast_policy_lane_by(PERF_TOAST_LANE_EPH, by_center, bh),
                        deck_lane_bar[PERF_TOAST_LANE_EPH], 0, 0u};

        ko_draw_toast(&t, fm1_ms);
    }
    if (proc->on) {
        ko_toast_t t = {proc->msg, 0, proc->kind, proc->end_ms,
                        perf_toast_policy_lane_by(PERF_TOAST_LANE_PROC, by_center, bh),
                        deck_lane_bar[PERF_TOAST_LANE_PROC], 0, 0u};

        ko_draw_toast(&t, fm1_ms);
    }
    if (main->on) {
        ko_toast_t t = {main->msg, 0, main->kind, main->end_ms,
                        perf_toast_policy_lane_by(PERF_TOAST_LANE_MAIN, by_center, bh),
                        deck_lane_bar[PERF_TOAST_LANE_MAIN], 0, 0u};

        if (main->side[0]) {
            t.side = main->side;
            t.end_ms = main->side_end_ms;
        }
        ko_draw_toast(&t, fm1_ms);
    }
}

static void deck_draw_screen(void)
{
    deck_runtime_t *r = rt();
    uint8_t v;

    deck_toast_lanes_sync(r);
    deck_draw_top();
    cv_begin(240u, 132u, C_BLACK);
    if (g.view == DECK_VIEW_SEQ)
        deck_ui_draw_seq(r, pat(), g.seq_plock_latch, g.seq_plock_step);
    else if (g.bank == DECK_BANK_LFO)
        fm_lfo_slots_draw_view(deck_lfo_sl(deck_lfo_vi()), deck_lfo_rtv(deck_lfo_vi()), g.lfo_tgt, DECK_LFO_TGT_N,
                               DECK_LFO_DEST, C_ACC);
    else if (g.bank == DECK_BANK_HOME) {
        deck_perf_t home_pf;
        uint8_t home_acc;
        uint8_t home_hum;
        deck_home_hud_t home_hud;

        deck_home_perf_for_ui(&home_pf, &home_acc, &home_hum);
        deck_home_hud_for_ui(&home_hud);
        deck_ui_draw_home(r, g.pulse, g.kick_pump, &home_pf, home_acc, home_hum, &home_hud);
    } else if (g.bank == DECK_BANK_GLO) {
        deck_ui_draw_groove(r, deck_bank_page(), g.accent_amt, g.human, g.midi_ch, g.clock);
    } else if (g.bank == DECK_BANK_FX)
        deck_ui_draw_fx(&g.dsp.master, g.fx_bypass, deck_bank_page(), r->bpm);
    else if (g.bank == DECK_BANK_ENV) {
        deck_voice_cfg_t plot_vc;
        uint8_t ev = r->cur_voice % DECK_NVOICES;

        uint8_t plot_delay;

        deck_env_plot_cfg(&g.dsp.cfg[ev], ev, &plot_vc);
        plot_delay = deck_lfo_mod_u8_v(ev, g.env_delay[ev], DECK_LFO_DELAY, 0u, 127u);
        deck_ui_draw_env(r, &plot_vc, &g.dsp.v[ev], g.pulse[ev], plot_delay,
                         g.lfo[ev][DECK_LFO_PITCH].depth > 0u ? 1u : 0u,
                         g.lfo[ev][DECK_LFO_PDEC].depth > 0u ? 1u : 0u,
                         g.env_invert[ev], g.env_retrig[ev]);
    } else if (g.bank == DECK_BANK_SEL) {
        uint8_t sv = r->cur_voice % DECK_NVOICES;

        deck_ui_draw_sel(r, g.dsp.cfg, &g.dsp.cfg[sv], &g.dsp.v[sv], g.pulse, deck_bank_page(),
                         deck_era_col(g.era));
    } else
        deck_ui_draw_bank_quiet(g.bank, DECK_BANK_NAME, DECK_BANK_N);
    /* Toast draws into the 132-px canvas, so it must run before cv_blit copies that canvas to the LCD. */
    deck_toast_lanes_draw();
    cv_blit(0u, 28u);
    deck_draw_knobs();
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (g.pulse[v] > 6u)
            g.pulse[v] = (uint8_t)(g.pulse[v] - 6u);
        else
            g.pulse[v] = 0u;
    }
    if (g.kick_pump > 6u)
        g.kick_pump = (uint8_t)(g.kick_pump - 6u);
    else
        g.kick_pump = 0u;
}

static void deck_midi_out_queue(uint8_t note, uint8_t vel)
{
    if (g.midi_out_n >= DECK_TRIG_Q_MAX)
        return;
    g.midi_out_note[g.midi_out_n] = note;
    g.midi_out_vel[g.midi_out_n] = vel;
    g.midi_out_n++;
}

static void deck_midi_out_flush(void)
{
    uint8_t i;

    for (i = 0u; i < g.midi_out_n; i++) {
        if (g.midi_out_vel[i])
            midi_send(0x99u, g.midi_out_note[i], g.midi_out_vel[i]);
        else
            midi_send(0x89u, g.midi_out_note[i], 0u);
    }
    g.midi_out_n = 0u;
}

static void deck_pulse_hit(uint8_t voice, uint8_t vel)
{
    if (voice >= DECK_NVOICES)
        return;
    g.pulse[voice] = vel;
    g.last_voice = voice;
    if (voice == DECK_BD)
        g.kick_pump = vel;
    g.flash_voice = voice;
    g.flash_until = fm1_ms + 600u;
    ko_flash_set(&g.flash, DECK_VNAME_LONG[voice], fm1_ms, 600u);
    g.ui_dirty = 1u;
}

static uint8_t deck_vel_human(uint8_t vel, uint8_t accent)
{
    int32_t v = (int32_t)vel;

    if (accent)
        v = 64 + (((v - 64) * deck_seq_lv_lerp(accent, 64, (int32_t)g.accent_amt)) >> 6);
    if (g.human) {
        int32_t j = (int32_t)(deck_rand() >> 24) - 128;
        v += (j * (int32_t)g.human) >> 8;
    }
    if (v < 1)
        v = 1;
    if (v > 127)
        v = 127;
    return (uint8_t)v;
}

static int deck_density_thin_voice(uint8_t voice)
{
    return voice == DECK_CH || voice == DECK_OH || voice == DECK_RM || voice == DECK_CB || voice == DECK_TB ||
           voice == DECK_RD || voice == DECK_TH || voice == DECK_TM || voice == DECK_TL;
}

static int deck_density_skip(uint8_t voice, uint8_t accent)
{
    uint8_t d = g.dsp.perf.density;
    uint8_t thin;

    if (accent || !deck_density_thin_voice(voice))
        return 0;
    if (d >= 64u)
        return 0;
    thin = (uint8_t)(((uint32_t)(64u - d) * 128u) / 64u);
    if (thin > 127u)
        thin = 127u;
    return (deck_rand() & 0x7Fu) < thin;
}

static void deck_density_boost(deck_runtime_t *r, uint8_t voice, uint8_t vel, uint8_t accent)
{
    uint8_t d = g.dsp.perf.density;
    uint8_t prob;
    uint8_t layer;
    uint8_t gv;

    if (d <= 64u || accent)
        return;
    prob = (uint8_t)(((uint32_t)(d - 64u) * 160u) / 63u);
    if (prob > 127u)
        prob = 127u;
    if (deck_density_thin_voice(voice)) {
        if ((deck_rand() & 0x7Fu) >= prob)
            return;
        if (voice == DECK_CH && (deck_rand() & 1u))
            layer = DECK_OH;
        else if (voice == DECK_OH && (deck_rand() & 1u))
            layer = DECK_CH;
        else
            layer = voice;
        gv = (uint8_t)(44u + ((uint32_t)vel * 52u) / 127u);
        deck_dsp_trig(&g.dsp, layer, gv, 0u);
        deck_midi_out_queue(DECK_MIDI_NOTE[layer], gv);
        deck_pulse_hit(layer, gv);
    }
    if (d >= 72u && (voice == DECK_SN || voice == DECK_BD) && (deck_rand() & 0x7Fu) < (uint8_t)(prob >> 1)) {
        uint8_t tv = (uint8_t)(DECK_TH + (deck_rand() % 3u));

        gv = (uint8_t)(36u + ((uint32_t)vel * 40u) / 127u);
        deck_dsp_trig(&g.dsp, tv, gv, 0u);
        deck_midi_out_queue(DECK_MIDI_NOTE[tv], gv);
        deck_pulse_hit(tv, gv);
    }
    (void)r;
}

static void deck_perf_fill_on_trig(deck_runtime_t *r, uint8_t voice, uint8_t seq_step)
{
    uint8_t fill = g.dsp.perf.fill;
    uint8_t step;
    uint8_t gate;

    if (!fill || !r || !r->playing)
        return;
    step = (uint8_t)(seq_step & 15u);
    if (step < 8u)
        return;
    gate = (uint8_t)(((uint32_t)fill * 3u) / 2u);
    if (gate > 127u)
        gate = 127u;
    if (voice == DECK_BD && step >= 12u) {
        uint8_t tv = DECK_TH + (uint8_t)((seq_step >> 1) % 3u);

        if ((deck_rand() & 0x7Fu) < gate) {
            deck_dsp_trig(&g.dsp, tv, 80u, 0u);
            deck_midi_out_queue(DECK_MIDI_NOTE[tv], 80u);
            deck_pulse_hit(tv, 80u);
        }
    } else if (voice == DECK_SN && step >= 10u && (deck_rand() & 0x7Fu) < fill) {
        uint8_t tv = DECK_TM;

        deck_dsp_trig(&g.dsp, tv, 72u, 0u);
        deck_midi_out_queue(DECK_MIDI_NOTE[tv], 72u);
        deck_pulse_hit(tv, 72u);
    }
}

static void deck_perf_stutter_on_trig(deck_runtime_t *r, uint8_t voice, uint8_t vel, uint8_t seq_step)
{
    uint8_t st = g.dsp.perf.stutter;
    uint8_t reps;
    uint8_t i;
    uint8_t rv;
    uint8_t thresh;

    if (!r || !r->playing || !st || voice >= DECK_NVOICES)
        return;
    if ((seq_step & 1u) != 0u)
        return;
    if (voice != DECK_SN && voice != DECK_CH && voice != DECK_OH && voice != DECK_RM) {
        if (st < 88u)
            return;
    }
    thresh = (uint8_t)(((uint32_t)st * 5u) / 4u);
    if (thresh > 127u)
        thresh = 127u;
    if ((deck_rand() & 0x7Fu) >= thresh)
        return;
    reps = 1u + (uint8_t)(st >> 5);
    if (st >= 64u)
        reps++;
    rv = (uint8_t)(70u + ((uint32_t)st * 40u) / 127u);
    (void)vel;
    for (i = 0u; i < reps; i++)
        deck_dsp_trig(&g.dsp, voice, rv, 0u);
}

static int deck_voice_pitch_slide(uint8_t voice)
{
    switch (voice) {
    case DECK_RM:
    case DECK_CB:
    case DECK_TB:
    case DECK_CW:
    case DECK_TH:
    case DECK_TM:
    case DECK_TL:
    case DECK_CNH:
    case DECK_CNL:
        return 1;
    default:
        return 0;
    }
}

static void deck_seq_ratchet_arm(uint8_t voice, uint8_t rat, uint16_t step_samples, uint8_t vel, uint8_t accent,
                                 int8_t semi)
{
    uint32_t every;

    if (rat <= 1u || step_samples < 32u)
        return;
    every = (uint32_t)step_samples / (uint32_t)rat;
    if (every < 16u)
        every = 16u;
    if (every > 65535u)
        every = 65535u;
    deck_seq_g.ratchet_left[voice] = (uint8_t)(rat - 1u);
    deck_seq_g.ratchet_every[voice] = (uint16_t)every;
    deck_seq_g.ratchet_acc[voice] = 0u;
    deck_seq_g.ratchet_vel[voice] = vel;
    deck_seq_g.ratchet_accent[voice] = accent;
    deck_seq_g.ratchet_semi[voice] = semi;
}

static void deck_seq_ratchet_service(deck_runtime_t *r)
{
    uint8_t v;

    if (!r || !r->playing)
        return;
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (!deck_seq_g.ratchet_left[v] || !deck_seq_g.ratchet_every[v])
            continue;
        deck_seq_g.ratchet_acc[v]++;
        if (deck_seq_g.ratchet_acc[v] < deck_seq_g.ratchet_every[v])
            continue;
        deck_seq_g.ratchet_acc[v] = 0u;
        deck_seq_g.ratchet_left[v]--;
        deck_dsp_trig_st(&g.dsp, v, deck_seq_g.ratchet_vel[v], deck_seq_g.ratchet_accent[v], 0u,
                         deck_seq_g.ratchet_semi[v]);
        deck_midi_out_queue(DECK_MIDI_NOTE[v], deck_seq_g.ratchet_vel[v]);
        deck_pulse_hit(v, deck_seq_g.ratchet_vel[v]);
    }
}

/* Ext clock: 16th length from audio_pos deltas (block-quantised, hence smoothed); gaps reset to unknown. */
static void deck_ext_s16_measure(void)
{
    uint32_t now = g.audio_pos;

    if (g.ext_s16_mark) {
        uint32_t d = now - g.ext_s16_mark;

        if (d > 65535u)
            g.ext_sps = 0u;
        else if (d >= 32u)
            g.ext_sps = g.ext_sps ? (g.ext_sps * 3u + d) / 4u : d;
    }
    g.ext_s16_mark = now ? now : 1u;
}

static void deck_seq_trig_step(deck_runtime_t *r, const deck_trig_evt_t *ev)
{
    uint8_t voice;
    uint8_t vel;
    uint8_t accent;
    uint8_t gate;
    uint8_t slide;
    uint8_t rat;
    uint16_t sps;
    uint8_t legato;

    if (!ev || !r)
        return;
    voice = ev->voice;
    vel = ev->vel;
    accent = ev->accent;
    gate = ev->gate;
    slide = ev->slide;
    rat = ev->ratchet;
    sps = ev->step_samples;
    if (voice >= DECK_NVOICES)
        return;
    if (deck_density_skip(voice, accent))
        return;
    if (gate == 0u) {
        deck_seq_g.ratchet_left[voice] = 0u;
        deck_voice_gate_close(&g.dsp.v[voice]);
        return;
    }
    vel = deck_vel_human(vel, accent);
    if (accent) {
        uint8_t fl = (uint8_t)deck_seq_lv_lerp(accent, 96,
                                               105 + (int32_t)(((127u - 105u) * (uint32_t)g.accent_amt) >> 7));

        if (vel < fl)
            vel = fl;
    }
    legato = (uint8_t)((deck_voice_pitch_slide(voice) && g.dsp.v[voice].active && g.dsp.v[voice].env > 800) ?
                           slide : 0u);
    deck_dsp_trig_st(&g.dsp, voice, vel, accent, legato, ev->semi);
    deck_midi_out_queue(DECK_MIDI_NOTE[voice], vel);
    deck_pulse_hit(voice, vel);
    deck_density_boost(r, voice, vel, accent);
    deck_perf_fill_on_trig(r, voice, ev->seq_step);
    deck_perf_stutter_on_trig(r, voice, vel, ev->seq_step);
    if (sps > 0u) {
        uint32_t gl = deck_seq_gate_samples(gate, sps);

        g.dsp.v[voice].gate_left = gl > 65535u ? 65535u : (uint16_t)gl;
    } else
        g.dsp.v[voice].gate_left = 0u;
    if (rat > 1u && deck_seq_ratchet_allowed(slide, gate))
        deck_seq_ratchet_arm(voice, rat, sps, vel, accent, ev->semi);
    else
        deck_seq_g.ratchet_left[voice] = 0u;
}

static void deck_trig_live(uint8_t voice, uint8_t vel, uint8_t accent)
{
    accent = accent ? DECK_SEQ_LV_FULL : 0u;
    vel = deck_vel_human(vel, accent);
    deck_dsp_trig(&g.dsp, voice, vel, accent);
    if (voice < DECK_NVOICES) {
        deck_midi_out_queue(DECK_MIDI_NOTE[voice], vel);
        deck_pulse_hit(voice, vel);
    }
}

static void deck_mute_toggle(uint8_t voice)
{
    deck_runtime_t *r = rt();

    if (voice >= DECK_NVOICES)
        return;
    r->lane_mute[voice] = r->lane_mute[voice] ? 0u : 1u;
    deck_toast(r->lane_mute[voice] ? "MUTE" : "UNMUTE", deck_vcol(voice));
    mark_dirty();
}

static void deck_solo_toggle(uint8_t voice)
{
    deck_runtime_t *r = rt();

    if (voice >= DECK_NVOICES)
        return;
    if (deck_seq_lane_is_solo(r, voice))
        deck_seq_lane_apply_state(r, voice, (uint8_t)FM_SEQ_STATE_PLAY);
    else
        deck_seq_lane_apply_state(r, voice, (uint8_t)FM_SEQ_STATE_SOLO);
    deck_toast(deck_seq_lane_is_solo(r, voice) ? "SOLO" : "UNSOLO", deck_vcol(voice));
    mark_dirty();
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

static void deck_knob_adjust_plock(uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    deck_runtime_t *r = rt();
    uint8_t pat_ix = r->cur_pat % DECK_NPAT;
    uint8_t voice = r->cur_voice;
    uint8_t st = g.seq_plock_step;
    deck_plock_ent_t *e;

    if (st >= FM_SEQ_STEPS)
        return;
    g.seq_plock_knob = 1u;
    e = deck_plock_alloc(pat_ix, voice, st);
    if (!e)
        return;
    if (slot == 0u) {
        if (edit)
            e->vel = steps < 0 ? 100u : 127u;
        else {
            if (!e->vel)
                e->vel = (uint8_t)deck_seq_lv_lerp(deck_seq_get_accent(pat(), voice, st), 100, 127);
            adj_u8(&e->vel, d, 1, 127);
        }
    } else if (slot == 1u) {
        if (edit)
            e->cond = steps < 0 ? FM_SEQ_COND_MAX : 0u;
        else if (steps != 0) {
            int32_t v = (int32_t)e->cond + (steps > 0 ? -1 : 1);

            if (v < 0)
                v = 0;
            if (v > (int32_t)FM_SEQ_COND_MAX)
                v = (int32_t)FM_SEQ_COND_MAX;
            e->cond = (uint8_t)v;
        }
    } else if (slot == 2u) {
        if (edit)
            e->micro = steps < 0 ? (int8_t)FM_SEQ_MICRO_MIN : (int8_t)FM_SEQ_MICRO_MAX;
        else {
            int32_t v = (int32_t)e->micro + (steps != 0 ? steps : d);

            e->micro = fm_seq_micro_clamp(v);
        }
    } else if (edit) {
        e->pitch = steps < 0 ? 0 : 24;
    } else if (steps != 0) {
        int32_t v = (int32_t)e->pitch + steps;

        if (v < -24)
            v = -24;
        if (v > 24)
            v = 24;
        e->pitch = (int8_t)v;
    }
    deck_plock_free_if_default(e);
    mark_dirty();
}

static void deck_adj(uint8_t *p, int32_t steps, uint8_t lo, uint8_t hi, uint8_t def, int edit)
{
    if (edit) {
        ko_edit_snap(steps, p, def, hi);
        return;
    }
    adj_u8(p, steps, lo, hi);
}

static void deck_adj_wrap(uint8_t *p, int32_t dir, uint8_t n)
{
    int32_t v = (int32_t)*p + (dir > 0 ? 1 : -1);

    while (v < 0)
        v += n;
    *p = (uint8_t)((uint32_t)v % n);
}

/* OCT -/+ in SEQ: turn the page; past the first or last page the pattern doubles (fm_seq_view_nav).
 * On PREPEND the original becomes the second half -> shift master_step and p-lock step indices by old len. */
static void deck_bar_page_step(int32_t dir)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = deck_seq_cur_pattern();
    uint8_t pat_ix = r->cur_pat % DECK_NPAT;
    uint8_t pages = deck_seq_pattern_pages(p);
    uint32_t len = (uint32_t)pages * FM_SEQ_PAGE_STEPS;
    uint8_t nav;
    char msg[8];

    if ((uint32_t)pages * 2u > FM_SEQ_MAX_PAGES) {
        /* No doubling room left (fm_seq_view_nav would clamp as FULL): loop the view over all S1..S8 pages. */
        uint8_t v = (uint8_t)(r->bar_page % FM_SEQ_MAX_PAGES);

        r->bar_page = dir > 0 ? (uint8_t)((v + 1u) % FM_SEQ_MAX_PAGES)
                              : (uint8_t)((v + FM_SEQ_MAX_PAGES - 1u) % FM_SEQ_MAX_PAGES);
        pages = FM_SEQ_MAX_PAGES;
        nav = FM_SEQ_NAV_MOVE;
    } else {
        nav = fm_seq_view_nav(&r->bar_page, pages, dir);
    }

    g.seq_follow = 0u;

    if (nav == FM_SEQ_NAV_APPEND || nav == FM_SEQ_NAV_PREPEND) {
        deck_seq_pattern_double(p, pat_ix, pages, nav);
        if (nav == FM_SEQ_NAV_PREPEND)
            r->master_step = fm_seq_dup_ix(r->master_step, len, nav);
        pages = fm_seq_pages_clamp((uint8_t)(pages * 2u));
    }
    fm_seq_fmt_nav(msg, nav, r->bar_page, pages);
    deck_toast(msg, C_ACC);
    mark_dirty();
}

static void deck_seq_page_step(void)
{
    uint8_t next;

    if (g.seq_plock_latch) {
        deck_plock_latch_end();
        return;
    }
    next = deck_seq_page_cycle_next(g.seq_page);
    if (next != g.seq_page) {
        g.seq_page = next;
        mark_dirty();
    }
}

static void deck_lane_page_follow_voice(deck_runtime_t *r)
{
    uint8_t v;

    if (!r)
        return;
    v = r->cur_voice % DECK_NVOICES;
    r->lane_page = (uint8_t)(v / DECK_UI_LANES);
    if (r->lane_page >= DECK_SEQ_LANE_PAGES)
        r->lane_page = (uint8_t)(DECK_SEQ_LANE_PAGES - 1u);
}

/* Plate change: every lane takes the new plate's groove and mode SONG (EUCLID/FRESH/EDITED/EMPTY overwritten). */
static void deck_seq_sync_baked(deck_runtime_t *r)
{
    deck_pattern_t *p = deck_seq_cur_pattern();
    uint8_t fader;
    uint8_t v;
    const char *en;

    (void)r;
    fader = (uint8_t)(g.plate % DECK_PLATES_PER_ERA);
    for (v = 0u; v < DECK_NVOICES; v++) {
        p->lane[v].euclid_hits = FM_SEQ_EUCLID_SONG;
        p->lane[v].pattern_origin = FM_SEQ_PATTERN_ORIGIN_NONE;
        p->lane[v].euclid_rot = 0;
        deck_seq_bake_lane(p, v, g.era, fader);
    }
    en = deck_era_name(g.era);
    for (v = 0u; v < 8u && en[v]; v++)
        p->name[v] = en[v];
    p->name[v < 8u ? v : 7u] = '\0';
}

/* One random plate at boot; era follows that plate once — knobs stay independent after. */
static void deck_boot_random_plate(void)
{
    deck_runtime_t *r = rt();

    deck_rng = deck_boot_seed();
    (void)deck_rand();
    g.plate = deck_clamp_plate((uint8_t)(((deck_rng >> 16) & 0xffffu) % DECK_PLATE_N));
    g.era = deck_era_of_plate(g.plate);
    deck_apply_plate(r);
    deck_seq_sync_baked(r);
}

static void deck_turn_groove(deck_runtime_t *r, const ko_groove_t *held, uint8_t turn)
{
    ko_groove_ref_t live = {&r->bpm, &r->swing, NULL, NULL};

    ko_turn_groove(&live, held, turn, r->playing, &g.groove_plate_pending);
}

static void deck_groove_commit_if_pending(deck_runtime_t *r)
{
    if (!g.groove_plate_pending || r->playing)
        return;
    deck_plate_groove_read(&r->bpm, &r->swing);
    g.groove_plate_pending = 0u;
    mark_dirty();
}

/* Global plate index — does not move the era knob (ko_era_plate.h). */
static void deck_cycle_plate(deck_runtime_t *r, int32_t dir)
{
    ko_groove_ref_t live = {&r->bpm, &r->swing, NULL, NULL};
    ko_groove_t held;
    int32_t v;

    if (!dir || !r)
        return;
    ko_groove_take(&held, &live);
    v = (int32_t)g.plate + dir;
    while (v < 0)
        v += (int32_t)DECK_PLATE_N;
    g.plate = deck_clamp_plate((uint8_t)((uint32_t)v % DECK_PLATE_N));
    if (deck_plate_ov_get(g.plate))
        g.era = deck_clamp_era(deck_plate_ov_get(g.plate)->era);
    deck_apply_plate(r);
    deck_turn_groove(r, &held, KO_TURN_PLATE);
    deck_seq_sync_baked(r);
}

/* Era decade macros only — plate index/name unchanged (no seq bake). */
static void deck_cycle_era(deck_runtime_t *r, int32_t dir)
{
    ko_groove_ref_t live = {&r->bpm, &r->swing, NULL, NULL};
    ko_groove_t held;
    int32_t v;

    if (!dir || !r)
        return;
    ko_groove_take(&held, &live);
    v = (int32_t)g.era + dir;
    while (v < 0)
        v += (int32_t)DECK_ERA_N;
    g.era = deck_clamp_era((uint8_t)((uint32_t)v % DECK_ERA_N));
    deck_apply_plate(r);
    deck_turn_groove(r, &held, KO_TURN_ERA);
}

static void deck_enc_nav(void)
{
    deck_runtime_t *r = rt();
    int32_t s;

    s = fm1_enc_take(ENC_SELECT);
    if (s) {
        r->cur_voice = deck_select_voice_step(r->cur_voice, s);
        deck_seq_g.lane_all = 0u;
        deck_pad_family_sync_voice(r->cur_voice);
        if (g.view == DECK_VIEW_SEQ)
            deck_lane_page_follow_voice(r);
        mark_dirty();
    }
    s = fm1_enc_take(ENC_ALGORITHM);
    if (s) {
        deck_cycle_era(r, s);
        mark_dirty();
    }
    s = fm1_enc_take(ENC_PRESETS);
    if (s) {
        if (fm1_in.buttons & (1u << BTN_EDIT)) {
            deck_lane_t *ln = &pat()->lane[r->cur_voice];
            int32_t d = (int32_t)fm_seq_clk_ratio_lane_ix(ln->div) + (s > 0 ? 1 : -1);

            if (d < 0)
                d = 0;
            if (d > (int32_t)FM_SEQ_CLK_RATIO_N - 1)
                d = (int32_t)FM_SEQ_CLK_RATIO_N - 1;
            deck_seq_set_div(pat(), r->cur_voice, (uint8_t)d);
        } else {
            deck_cycle_plate(r, s);
        }
        mark_dirty();
    }
}

static void deck_enc_knobs(void)
{
    deck_runtime_t *r = rt();
    deck_voice_cfg_t *vc = &g.dsp.cfg[r->cur_voice];
    deck_master_t *fx = &g.dsp.master;
    deck_perf_t *pf = &g.dsp.perf;
    uint8_t pg = deck_bank_page();
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    uint8_t lfo_held = (fm1_in.buttons & (1u << BTN_LFO)) != 0u;
    uint32_t col;

    for (col = 0u; col < 4u; col++) {
        int32_t s = fm1_enc_take((uint8_t)(ENC_KNOB1 + col));
        int32_t gsteps;

        if (!s)
            continue;
        gsteps = ko_knob_gain(&g.knob_ms[col], fm1_ms, s);
        if (deck_arp_ov_live()) { /* ARP overlay owns the knob row; LFO hold does not apply */
            if (col == 0u)
                deck_adj(&g.arp_rate, s > 0 ? 1 : -1, 0u, 4u, 1u, edit);
            else if (col == 1u)
                deck_adj(&g.arp_gate, gsteps, 0u, 127u, 64u, edit);
            else if (col == 2u)
                deck_adj(&g.arp_ramp, gsteps, 0u, 127u, 0u, edit);
            else
                g.arp_latch = s > 0 ? 1u : 0u;
            continue;
        }
        if (lfo_held && g.view != DECK_VIEW_SEQ && g.bank != DECK_BANK_LFO) {
            uint8_t lt = deck_lfo_target_for_knob((uint8_t)col);

            if (lt != 255u) {
                (void)deck_lfo_dest_touch(lt);
                if (col != 2u) {
                    if (deck_kick_duck_locked(g.bank, pg, col))
                        continue;
                    adj_u8(&g.lfo[deck_lfo_vi()][lt].depth, gsteps, 0, 127);
                    ko_flash_set(&g.flash, "DEPTH", fm1_ms, 600u);
                    mark_dirty();
                    continue;
                }
            }
        }
        if (lfo_held && g.bank == DECK_BANK_LFO && col != 2u) {
            adj_u8(&g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N].depth, gsteps, 0, 127);
            ko_flash_set(&g.flash, "DEPTH", fm1_ms, 600u);
            mark_dirty();
            continue;
        }
        if (g.view == DECK_VIEW_SEQ && g.seq_plock_hold) {
            if (g.seq_plock_bank == 1u)
                deck_knob_adjust_seq_gate((uint8_t)col, gsteps, gsteps, edit);
            else
                deck_knob_adjust_plock((uint8_t)col, gsteps, gsteps, edit);
            continue;
        }
        if (g.view == DECK_VIEW_SEQ) {
            deck_knob_adjust_seq((uint8_t)col, gsteps, gsteps, edit);
            continue;
        }
        if (g.bank != DECK_BANK_LFO) {
            uint8_t lt = deck_lfo_target_for_knob((uint8_t)col);

            if (lt != 255u)
                (void)deck_lfo_dest_touch(lt);
        }
        if (g.bank == DECK_BANK_SEL && pg == 0u) {
            uint8_t *pp[4] = {&vc->timbre, &vc->level, &vc->tune, &vc->filter};
            uint8_t def[4] = {64u, DECK_CFG_DEF[r->cur_voice][0], DECK_CFG_DEF[r->cur_voice][1],
                              DECK_CFG_DEF[r->cur_voice][6]};
            deck_adj(pp[col], gsteps, 0u, 127u, def[col], edit);
        } else if (g.bank == DECK_BANK_SEL && pg == 1u) {
            uint8_t *pp[4] = {&vc->pan, &vc->noise, &vc->drive, &vc->crush};
            deck_adj(pp[col], gsteps, 0u, 127u,
                     col == 0u ? 64u : col == 3u ? 0u :
                                 col == 1u ? DECK_CFG_DEF[r->cur_voice][4] : DECK_CFG_DEF[r->cur_voice][7],
                     edit);
        } else if (g.bank == DECK_BANK_SEL && pg == 2u) {
            if (col == 0u)
                deck_adj(&vc->vel_sens, gsteps, 0u, 127u, 80u, edit);
            else if (col == 1u) {
                if (r->cur_voice % DECK_NVOICES != DECK_BD)
                    deck_adj(&vc->duck, gsteps, 0u, 127u, 0u, edit);
            } else
                deck_adj(col == 2u ? &vc->dly_send : &vc->rev_send, gsteps, 0u, 127u, 127u, edit);
        } else if (g.bank == DECK_BANK_ENV && pg == 0u) {
            if (col == 0u)
                deck_adj(&vc->click, gsteps, 0u, 127u, 64u, edit);
            else if (col == 1u)
                deck_adj(&vc->decay, gsteps, 0u, 127u, DECK_CFG_DEF[r->cur_voice][2], edit);
            else if (col == 2u)
                deck_adj(&vc->sustain, gsteps, 0u, 127u, 0u, edit);
            else
                deck_adj(&vc->release, gsteps, 0u, 127u, 64u, edit);
        } else if (g.bank == DECK_BANK_ENV && pg == 1u) {
            if (col == 0u)
                deck_adj(&g.env_delay[r->cur_voice % DECK_NVOICES], gsteps, 0u, 127u, 0u, edit);
            else if (col == 1u)
                deck_adj(&vc->hold, gsteps, 0u, 127u, 0u, edit);
            else if (col == 2u)
                deck_adj(&vc->curve, gsteps, 0u, 127u, 64u, edit);
            else if (col == 3u) {
                uint8_t *p = &g.env_retrig[r->cur_voice % DECK_NVOICES];

                if (edit && gsteps != 0)
                    *p = gsteps < 0 ? 0u : 1u;
                else if (gsteps != 0)
                    *p ^= 1u;
            }
        } else if (g.bank == DECK_BANK_ENV && pg == 2u) {
            if (col == 0u) {
                uint8_t *p = &g.env_invert[r->cur_voice % DECK_NVOICES];

                if (edit && gsteps != 0)
                    *p = gsteps < 0 ? 0u : 1u;
                else if (gsteps != 0)
                    *p ^= 1u;
                deck_dsp_sync_env_flags();
            } else if (col == 1u)
                deck_adj(&vc->pitch_dec, gsteps, 0u, 127u, 64u, edit);
            else if (col == 2u)
                deck_adj(&vc->pitch_env, gsteps, 0u, 127u, 64u, edit);
        } else if (g.bank == DECK_BANK_GLO && pg == 0u) {
            if (col == 0u)
                deck_adj(&r->key, gsteps, 0u, 11u, 0u, edit);
            else if (col == 1u)
                deck_adj(&r->scale, gsteps, 0u, DECK_SCALE_N - 1u, 0u, edit);
            else if (col == 2u)
                deck_adj(&r->swing, gsteps, 50u, 75u, 50u, edit);
            else if (col == 3u)
                deck_adj(&r->bpm, gsteps, DECK_BPM_MIN, DECK_BPM_MAX, DECK_BPM_DEF, edit);
        } else if (g.bank == DECK_BANK_GLO && pg == 2u) {
            if (col == 0u)
                deck_adj(&g.human, gsteps, 0u, 127u, 0u, edit);
            else if (col == 1u)
                deck_adj(&deck_seq_g.timing_hum, gsteps, 0u, 127u, 0u, edit);
        } else if (g.bank == DECK_BANK_GLO && pg == 1u) {
            if (col == 0u)
                deck_adj(&g.accent_amt, gsteps, 0u, 127u, 96u, edit);
            else if (col == 1u)
                deck_adj(&pf->tune, gsteps, 0u, 127u, 64u, edit);
            else if (col == 2u)
                deck_adj(&g.midi_ch, s > 0 ? 1 : -1, 0u, 15u, 9u, edit);
            else
                g.clock = s > 0 ? 1u : 0u;
        } else if (g.bank == DECK_BANK_FX && pg == 0u) {
            uint8_t *pp[4] = {&fx->dly_lvl, &fx->dly_time, &fx->feedback, &fx->loop_filt};
            uint8_t def[4] = {0u, 64u, 48u, 64u};
            if (col == 1u && edit && gsteps != 0) {
                uint32_t bpm_t = r->bpm ? r->bpm : 120u;
                fx->dly_time = fm_time_step_snap(fx->dly_time, gsteps, bpm_t);
            } else
                deck_adj(pp[col], gsteps, 0u, 127u, def[col], edit);
        } else if (g.bank == DECK_BANK_FX && pg == 1u) {
            if (col == 1u) {
                if (edit && gsteps != 0)
                    fx->tap = gsteps < 0 ? 0u : (uint8_t)(DECK_TAP_N - 1u);
                else {
                    int32_t tv = (int32_t)fx->tap + gsteps;

                    while (tv < 0)
                        tv += (int32_t)DECK_TAP_N;
                    fx->tap = (uint8_t)((uint32_t)tv % DECK_TAP_N);
                }
            } else {
                uint8_t *pp[4] = {&fx->wow, &fx->tap, &fx->tank, &fx->isolator};
                uint8_t def[4] = {0u, 0u, 64u, 64u};
                deck_adj(pp[col], gsteps, 0u, 127u, def[col], edit);
            }
        } else if (g.bank == DECK_BANK_FX && pg == 2u) {
            uint8_t *pp[4] = {&fx->rev_mix, &fx->rev_damp, &fx->rev_tail, &fx->dly_rev};
            uint8_t def[4] = {0u, 64u, 64u, 0u};
            deck_adj(pp[col], gsteps, 0u, 127u, def[col], edit);
        } else if (g.bank == DECK_BANK_FX && pg == 3u) {
            uint8_t *pp[4] = {&fx->drive, &fx->crush, &fx->filter, &fx->width};
            uint8_t def[4] = {24u, 0u, 64u, 40u};
            deck_adj(pp[col], gsteps, 0u, 127u, def[col], edit);
        } else if (g.bank == DECK_BANK_FX && pg == 4u) {
            uint8_t *pp[3] = {&fx->comp, &fx->dust, &fx->tape};
            uint8_t def[3] = {0u, 0u, 0u};
            if (col < 3u)
                deck_adj(pp[col], gsteps, 0u, 127u, def[col], edit);
        } else if (g.bank == DECK_BANK_LFO && pg == 0u) {
            fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];
            if (col == 0u) {
                if (edit && gsteps != 0)
                    g.lfo_tgt = gsteps < 0 ? fm_lfo_dests_first(&deck_lfo_dests, DECK_LFO_LEVEL) :
                                 fm_lfo_dests_last(&deck_lfo_dests, DECK_LFO_LEVEL);
                else
                    g.lfo_tgt = deck_lfo_tgt_step(g.lfo_tgt, s > 0 ? 1 : -1);
            } else if (col == 1u)
                deck_adj(&ls->rate, gsteps, 1u, 127u, FM_LFO_RATE_DEFAULT, edit);
            else if (col == 2u)
                deck_adj(&ls->depth, gsteps, 0u, 127u, 0u, edit);
            else
                ls->wave = fm_lfo_step_shape(ls->wave, gsteps, edit);
        } else if (g.bank == DECK_BANK_LFO && pg == 1u) {
            fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];
            if (col == 0u)
                deck_adj(&ls->phase, gsteps, 0u, 127u, 0u, edit);
            else if (col == 1u)
                deck_adj(&ls->skew, gsteps, 0u, 127u, FM_LFO_SKEW_DEFAULT, edit);
            else if (col == 2u)
                deck_adj(&ls->smooth, gsteps, 0u, 127u, 0u, edit);
            else
                deck_adj(&ls->fold, gsteps, 0u, 127u, 0u, edit);
        } else if (g.bank == DECK_BANK_LFO && pg == 2u) {
            fm_lfo_slot_t *ls = &g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N];
            if (col == 0u) {
                if (edit && gsteps != 0)
                    ls->algo = gsteps < 0 ? 0u : (uint8_t)(FM_LFO_NALGOS - 1u);
                else {
                    int32_t av = (int32_t)ls->algo + gsteps;
                    while (av < 0)
                        av += (int32_t)FM_LFO_NALGOS;
                    ls->algo = (uint8_t)((uint32_t)av % FM_LFO_NALGOS);
                }
            } else if (col == 1u)
                deck_adj(&ls->ratio, gsteps, 0u, 127u, 52u, edit);
            else if (col == 2u)
                deck_adj(&ls->index, gsteps, 0u, 127u, 64u, edit);
            else
                deck_adj(&ls->fb, gsteps, 0u, 127u, 0u, edit);
        } else if (g.bank == DECK_BANK_ARP) {
            if (col == 0u)
                deck_adj(&g.arp_rate, s > 0 ? 1 : -1, 0u, 4u, 1u, edit);
            else if (col == 1u)
                deck_adj(&g.arp_gate, gsteps, 0u, 127u, 64u, edit);
            else if (col == 2u)
                deck_adj(&g.arp_ramp, gsteps, 0u, 127u, 0u, edit);
            else
                g.arp_latch = s > 0 ? 1u : 0u;
        } else if (g.bank == DECK_BANK_HOME && pg == 0u) {
            uint8_t *pp[4] = {&pf->tune, &pf->decay, &pf->filter, &pf->crush};
            deck_adj(pp[col], gsteps, 0u, 127u, 64u, edit);
        } else {
            if (col == 0u)
                deck_adj(&pf->density, gsteps, 0u, 127u, 64u, edit);
            else if (col == 1u)
                deck_adj(&pf->fill, gsteps, 0u, 127u, 0u, edit);
            else if (col == 2u) {
                if (edit)
                    deck_apply_ensemble(r, gsteps < 0 ? FM_SEQ_ENS_MUTE : FM_SEQ_ENS_ALL);
                else if (gsteps != 0) {
                    int32_t ix = (int32_t)g.seq_ens_sel + (gsteps > 0 ? 1 : -1);

                    if (ix < 0)
                        ix = 0;
                    if (ix >= (int32_t)FM_SEQ_NENSEMBLES)
                        ix = (int32_t)FM_SEQ_NENSEMBLES - 1;
                    g.seq_ens_sel = (uint8_t)ix;
                    deck_apply_ensemble(r, g.seq_ens_sel);
                    deck_toast(deck_ens_combo_name(g.seq_ens_sel), C_ACC);
                }
            } else
                deck_adj(&pf->stutter, gsteps, 0u, 127u, 0u, edit);
        }
        ko_flash_set(&g.flash, g.bank == DECK_BANK_LFO && col == 0u ? DECK_LFO_DEST[g.lfo_tgt % DECK_LFO_TGT_N] : "OK",
                     fm1_ms, 500u);
        mark_dirty();
    }
}

static void deck_bank_short(uint8_t b)
{
    if (g.seq_plock_latch)
        deck_plock_latch_end();
    if (b == DECK_BANK_ARP) { /* BTN_ARP short = overlay toggle, no bank change; SEQ is left like a bank press */
        if (g.view == DECK_VIEW_SEQ) {
            g.view = DECK_VIEW_HOME;
            g.arp_ov = 1u;
        } else {
            g.arp_ov ^= 1u;
        }
        mark_dirty();
        return;
    }
    if (g.bank == b && g.view != DECK_VIEW_SEQ) {
        g.page[b] = (uint8_t)((g.page[b] + 1u) % deck_bank_pages(b));
    } else {
        if (g.view == DECK_VIEW_SEQ)
            g.view = DECK_VIEW_HOME;
        g.bank = b;
    }
    mark_dirty();
}

/* DJ transition callback (fm_dj_app_trans_fn): plate / era steps per deck_dj_trans_act. */
static void deck_dj_on_trans(fm_dj_t *dj, uint8_t trans, uint8_t r, uint8_t *kick_slam_bars)
{
    deck_dj_act_t a = deck_dj_trans_act(&g.dj_app, trans, r, kick_slam_bars);

    if (a.plate)
        deck_cycle_plate(rt(), a.plate);
    if (a.era)
        deck_cycle_era(rt(), a.era);
    deck_toast(fm_dj_trans_toast_label(dj), C_ACC);
}

static void deck_dj_fx_short(void)
{
    deck_bank_short(DECK_BANK_FX);
}

static void deck_bank_long(uint8_t b)
{
    if (b == DECK_BANK_SEL) {
        deck_voice_cfg_default(&g.dsp.cfg[rt()->cur_voice], rt()->cur_voice);
        deck_toast("V RESET", deck_vcol(rt()->cur_voice));
    } else if (b == DECK_BANK_FX) {
        g.fx_bypass ^= 1u;
        deck_toast(g.fx_bypass ? "FX OFF" : "FX ON", C_ACC);
    } else if (b == DECK_BANK_ARP) {
        g.arp_latch ^= 1u;
        if (!g.arp_latch)
            g.arp_latch_mask = 0u;
        deck_toast(g.arp_latch ? "LATCH" : "MOMENT", C_ACC);
    } else if (b == DECK_BANK_HOME) {
        deck_perf_default(&g.dsp.perf);
        deck_toast("PERF 0", C_ACC);
    } else if (b == DECK_BANK_ENV) {
        deck_voice_cfg_t *vc = &g.dsp.cfg[rt()->cur_voice];
        uint8_t vi = rt()->cur_voice % DECK_NVOICES;

        g.env_delay[vi] = 0u;
        vc->click = 64u;
        vc->hold = 0u;
        vc->decay = DECK_CFG_DEF[vi][2];
        vc->pitch_dec = 64u;
        vc->sustain = 0u;
        vc->release = 64u;
        g.env_retrig[vi] = 1u;
        g.env_invert[vi] = 0u;
        g.dsp.env_invert[vi] = 0u;
        vc->curve = 64u;
        vc->pitch_env = 64u;
        deck_toast("ENV RST", deck_vcol(rt()->cur_voice));
    } else if (b == DECK_BANK_LFO) {
        fm_lfo_slot_reset(&g.lfo[deck_lfo_vi()][g.lfo_tgt % DECK_LFO_TGT_N]);
        deck_toast("LFO RST", C_ACC);
    }
    mark_dirty();
}

static void deck_handle_banks(uint32_t rising, uint32_t released, uint32_t held)
{
    uint8_t i;

    for (i = 0u; i < DECK_BANK_N; i++) {
        uint8_t btn = DECK_BANK_BTN[i];

        if (i == DECK_BANK_FX) /* BTN_FX: short = bank, long = DJ toggle (see app_step) */
            continue;
        ko_longpress_t lp = {&g.bank_ms[i], &g.bank_used[i], NULL};

        if (rising & (1u << btn)) {
            g.bank_ms[i] = fm1_ms;
            g.bank_used[i] = 0u;
        }
        if ((held & (1u << btn)) && g.bank_ms[i] &&
            ko_longpress_poll(&lp, fm1_ms, DECK_HOLD_MS, 0))
            deck_bank_long(i);
        if ((released & (1u << btn)) && !g.bank_used[i] && g.bank_ms[i])
            deck_bank_short(i);
    }
}

static void deck_handle_seq_white(uint32_t rising, uint32_t released)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    uint32_t k;
    uint32_t base = deck_seq_view_base(r);

    g.seq_plock_hold = g.seq_plock_latch;
    for (k = 0u; k < 16u; k++) {
        uint32_t bit = 1u << KEY_WHITE[k];

        if (rising & bit) {
            if (!g.seq_plock_latch) {
                g.seq_plock_step = (uint8_t)(base + k);
                g.seq_plock_knob = 0u;
            }
            g.white_press_k = (uint8_t)k;
            g.white_press_ms = fm1_ms;
            g.white_long_used = 0u;
            g.white_long_cancel = 0u;
        }
        if (released & bit) {
            if (g.white_press_k == k && !g.white_long_used && !g.white_long_cancel) {
                if (fm1_ms - g.white_press_ms < DECK_HOLD_MS) {
                    uint8_t st = (uint8_t)(base + k);

                    if (g.seq_plock_latch && st == g.seq_plock_step)
                        deck_plock_latch_end();
                    else if (!g.seq_plock_latch)
                        deck_seq_toggle_step(p, r, r->cur_voice, (uint8_t)k);
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
}

static void deck_handle_keys(uint32_t rising, uint32_t falling, uint32_t btn)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    uint32_t k;
    uint8_t edit = (uint8_t)((btn & (1u << BTN_EDIT)) != 0u);
    uint8_t home = (uint8_t)((btn & (1u << BTN_HOME)) != 0u);
    uint8_t accent = edit;

    r->accent_hold = accent;
    if (g.view == DECK_VIEW_SEQ)
        deck_handle_seq_white(rising, falling);
    else {
        for (k = 0u; k < 16u; k++) {
            if (rising & (1u << KEY_WHITE[k]))
                deck_seq_toggle_step(p, r, r->cur_voice, (uint8_t)k);
        }
    }
    for (k = 0u; k < DECK_NPADS; k++) {
        if (rising & (1u << KEY_BLACK[k])) {
            uint8_t voice = deck_pad_voice_for_hit((uint8_t)k, btn);

            g.pad_ms[k] = fm1_ms;
            g.pad_used[k] = 0u;
            g.pad_voice[k] = voice;
            if (btn & (1u << BTN_FX)) {
                deck_mute_toggle(voice);
                continue;
            }
            if (btn & (1u << BTN_SEL)) {
                deck_solo_toggle(voice);
                continue;
            }
            r->cur_voice = voice;
            deck_seq_g.lane_all = 0u;
            if (g.view == DECK_VIEW_SEQ)
                deck_lane_page_follow_voice(r);
            if (r->playing) {
                uint8_t bk_vel = deck_vel_human(accent ? 127u : 110u, accent ? DECK_SEQ_LV_FULL : 0u);

                if (voice < DECK_NVOICES)
                    deck_midi_out_queue(DECK_MIDI_NOTE[voice], bk_vel);
            } else if (voice < DECK_NVOICES) {
                uint8_t aud_acc = accent ? DECK_SEQ_LV_FULL : 0u;

                deck_dsp_trig(&g.dsp, voice, deck_vel_human(accent ? 127u : 110u, aud_acc), aud_acc);
            }
            if (r->rec_arm && r->playing)
                deck_seq_record_step(p, r, voice, accent);
        }
        if (falling & (1u << KEY_BLACK[k])) {
            uint8_t voice = g.pad_voice[k];

            g.arp_held &= (uint16_t)~(1u << voice);
            if (!g.arp_latch)
                g.arp_latch_mask &= (uint16_t)~(1u << voice);
        }
        if ((fm1_in.notes & (1u << KEY_BLACK[k])) && g.pad_ms[k] && !g.pad_used[k] &&
            fm1_ms - g.pad_ms[k] >= DECK_HOLD_MS) {
            uint8_t voice = deck_pad_is_family((uint8_t)k) ? g.pad_family_voice[k] :
                                                             deck_pad_resolve((uint8_t)k, edit, home);

            g.pad_used[k] = 1u;
            g.pad_voice[k] = voice;
            r->cur_voice = voice;
            deck_seq_g.lane_all = 0u;
            g.bank = DECK_BANK_SEL;
            g.view = DECK_VIEW_HOME;
            deck_toast(DECK_VNAME_LONG[voice], deck_vcol(voice));
            mark_dirty();
        }
    }
    if (rising || falling)
        mark_dirty();
}

static void deck_update_leds(uint32_t keys)
{
    deck_runtime_t *r = rt();
    deck_pattern_t *p = pat();
    uint32_t k;
    uint32_t base = deck_seq_view_base(r);
    uint8_t cur = (uint8_t)(r->cur_voice % DECK_NVOICES);
    uint8_t ph_st = deck_ui_last_step(r, p, cur);
    int pulse = ko_led_pulse(fm1_ms);
    uint8_t i;

    for (k = 0u; k < 16u; k++) {
        uint32_t abs = base + k;
        int on = fm_seq_bit(p->lane[r->cur_voice].hit, abs);

        /* White key k = step k on the visible page; playhead = grid cursor step (deck_ui_last_step), inverted. */
        if (ph_st != DECK_UI_NO_STEP && (uint32_t)ph_st == abs)
            on = !on;
        fm1_led_key(KEY_LED(KEY_WHITE[k]), on);
    }
    for (k = 0u; k < DECK_NPADS; k++) {
        int led = ((keys >> KEY_BLACK[k]) & 1u) != 0;

        if (deck_pad_voice_in_family((uint8_t)k, r->cur_voice))
            led = 1;
        fm1_led_key(KEY_LED(KEY_BLACK[k]), led);
    }
    fm1_led_key(BTN_PLAY, r->playing);
    fm1_led_key(BTN_SEQ, g.view == DECK_VIEW_SEQ);
    fm1_led_key(BTN_REC, r->rec_arm);
    fm1_led_key(BTN_SAVE, (int32_t)(g.saved_until - fm1_ms) > 0);
    for (i = 0u; i < DECK_BANK_N; i++) {
        uint8_t st = ko_bank_led(i == DECK_BANK_ARP ? deck_arp_ov_live() : (g.view != DECK_VIEW_SEQ && g.bank == i),
                                 g.page[i] > 0u, pulse);

        fm1_led_key(DECK_BANK_BTN[i], st == KO_LED_ON);
        if (st == KO_LED_DIM)
            fm1_led_key_dim(DECK_BANK_BTN[i], 1);
    }
    if (deck_dj_fx_flash_active) {
        uint32_t ph = (fm1_ms - deck_dj_fx_flash_ms) / DECK_DJ_FX_FLASH_MS;

        if (ph < DECK_DJ_FX_FLASH_N)
            fm1_led_key(BTN_FX, (ph & 1u) == 0u);
        else
            deck_dj_fx_flash_active = 0u;
    }
}

static uint8_t deck_arp_period(void)
{
    switch (deck_dj_arp_rate(&g.dj_app, g.arp_rate) % 5u) {
    case 0u:
        return 2u;
    case 2u:
    case 4u:
        return 1u;
    default:
        return 1u;
    }
}

static void deck_arp_tick(void)
{
    uint16_t mask = g.arp_held | (g.arp_latch ? g.arp_latch_mask : 0u);
    uint8_t v, n = 0u;
    uint8_t vel;
    uint8_t ramp;
    uint8_t arp_r;

    if (!mask)
        return;
    g.arp_phase++;
    arp_r = deck_dj_arp_rate(&g.dj_app, g.arp_rate);
    if ((g.arp_phase % deck_arp_period()) != 0u && arp_r != 2u && arp_r != 4u)
        return;
    vel = 110u;
    ramp = deck_lfo_mod_u8(g.arp_ramp, DECK_LFO_ARAMP, 0u, 127u);
    if (ramp)
        vel = (uint8_t)(40u + ((uint32_t)g.arp_phase * (uint32_t)ramp >> 3));
    if (vel > 127u)
        vel = 127u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (!(mask & (uint16_t)(1u << v)))
            continue;
        deck_trig_live(v, vel, 0u);
        if (++n >= 4u)
            break;
    }
}

static void deck_lfo_apply_eff(const deck_voice_cfg_t *base_cfg, const uint8_t *base_env_delay, deck_master_t *fx,
                               const deck_master_t *base_fx, deck_perf_t *pf, const deck_perf_t *base_pf,
                               uint8_t *accent, uint8_t base_accent, uint8_t *human, uint8_t base_human)
{
    deck_voice_cfg_t *vc;
    const deck_voice_cfg_t *bc;
    uint8_t v;
    uint8_t gv = deck_lfo_vi();

    for (v = 0u; v < DECK_NVOICES; v++) {
        vc = &g.dsp.cfg[v];
        bc = &base_cfg[v];
        g.dsp.cfg[v] = base_cfg[v];
        vc->level = deck_lfo_mod_u8_v(v, bc->level, DECK_LFO_LEVEL, 0u, 127u);
        vc->tune = deck_lfo_mod_u8_v(v, bc->tune, DECK_LFO_TUNE, 0u, 127u);
        vc->decay = deck_lfo_mod_u8_v(v, bc->decay, DECK_LFO_DECAY, 0u, 127u);
        vc->filter = deck_lfo_mod_u8_v(v, bc->filter, DECK_LFO_FILTER, 0u, 127u);
        vc->drive = deck_lfo_mod_u8_v(v, bc->drive, DECK_LFO_DRIVE, 0u, 127u);
        vc->pan = deck_lfo_mod_u8_v(v, bc->pan, DECK_LFO_PAN, 0u, 127u);
        vc->crush = deck_lfo_mod_u8_v(v, bc->crush, DECK_LFO_CRUSH, 0u, 127u);
        vc->pitch_env = deck_lfo_mod_u8_v(v, bc->pitch_env, DECK_LFO_PITCH, 0u, 127u);
        vc->timbre = deck_lfo_mod_u8_v(v, bc->timbre, DECK_LFO_TIMBR, 0u, 127u);
        vc->noise = deck_lfo_mod_u8_v(v, bc->noise, DECK_LFO_NOISE, 0u, 127u);
        vc->click = deck_lfo_mod_u8_v(v, bc->click, DECK_LFO_ATTACK, 0u, 127u);
        vc->hold = deck_lfo_mod_u8_v(v, bc->hold, DECK_LFO_HOLD, 0u, 127u);
        if (base_env_delay)
            g.env_delay[v] = deck_lfo_mod_u8_v(v, base_env_delay[v], DECK_LFO_DELAY, 0u, 127u);
        vc->sustain = deck_lfo_mod_u8_v(v, bc->sustain, DECK_LFO_SUST, 0u, 127u);
        vc->release = deck_lfo_mod_u8_v(v, bc->release, DECK_LFO_RELEASE, 0u, 127u);
        vc->pitch_dec = deck_lfo_mod_u8_v(v, bc->pitch_dec, DECK_LFO_PDEC, 0u, 127u);
        vc->curve = deck_lfo_mod_u8_v(v, bc->curve, DECK_LFO_CURVE, 0u, 127u);
        vc->vel_sens = deck_lfo_mod_u8_v(v, bc->vel_sens, DECK_LFO_VEL, 0u, 127u);
        vc->duck = v == DECK_BD ? 0u : deck_lfo_mod_u8_v(v, bc->duck, DECK_LFO_DUCK, 0u, 127u);
        vc->dly_send = deck_lfo_mod_u8_v(v, bc->dly_send, DECK_LFO_DSEND, 0u, 127u);
        vc->rev_send = deck_lfo_mod_u8_v(v, bc->rev_send, DECK_LFO_RSEND, 0u, 127u);
    }

    *fx = *base_fx;
    *pf = *base_pf;
    *accent = base_accent;
    *human = base_human;

    fx->filter = deck_lfo_mod_u8_v(gv, base_fx->filter, DECK_LFO_MFILT, 0u, 127u);
    fx->filter = deck_lfo_mod_u8_v(gv, fx->filter, DECK_LFO_FXFILTER, 0u, 127u);
    fx->drive = deck_lfo_mod_u8_v(gv, base_fx->drive, DECK_LFO_FXDRIVE, 0u, 127u);
    fx->crush = deck_lfo_mod_u8_v(gv, base_fx->crush, DECK_LFO_FXCRUSH, 0u, 127u);
    fx->width = deck_lfo_mod_u8_v(gv, base_fx->width, DECK_LFO_WIDTH, 0u, 127u);
    fx->comp = deck_lfo_mod_u8_v(gv, base_fx->comp, DECK_LFO_COMP, 0u, 127u);
    fx->dust = deck_lfo_mod_u8_v(gv, base_fx->dust, DECK_LFO_DUST, 0u, 127u);
    fx->tape = deck_lfo_mod_u8_v(gv, base_fx->tape, DECK_LFO_TAPE, 0u, 127u);
    fx->dly_lvl = deck_lfo_mod_u8_v(gv, base_fx->dly_lvl, DECK_LFO_EMIX, 0u, 127u);
    /* TIME 0 = OFF; modulation must not toggle the echo bus off and on. */
    if (base_fx->dly_time)
        fx->dly_time = deck_lfo_mod_u8_v(gv, base_fx->dly_time, DECK_LFO_ETIME, 1u, 127u);
    fx->feedback = deck_lfo_mod_u8_v(gv, base_fx->feedback, DECK_LFO_EFB, 0u, 127u);
    fx->loop_filt = deck_lfo_mod_u8_v(gv, base_fx->loop_filt, DECK_LFO_ETONE, 0u, 127u);
    fx->wow = deck_lfo_mod_u8_v(gv, base_fx->wow, DECK_LFO_WOW, 0u, 127u);
    fx->tank = deck_lfo_mod_u8_v(gv, base_fx->tank, DECK_LFO_TANK, 0u, 127u);
    fx->isolator = deck_lfo_mod_u8_v(gv, base_fx->isolator, DECK_LFO_ISO, 0u, 127u);
    fx->rev_mix = deck_lfo_mod_u8_v(gv, base_fx->rev_mix, DECK_LFO_REV, 0u, 127u);
    fx->rev_damp = deck_lfo_mod_u8_v(gv, base_fx->rev_damp, DECK_LFO_RDAMP, 0u, 127u);
    fx->rev_tail = deck_lfo_mod_u8_v(gv, base_fx->rev_tail, DECK_LFO_RTAIL, 0u, 127u);
    fx->dly_rev = deck_lfo_mod_u8_v(gv, base_fx->dly_rev, DECK_LFO_BLEED, 0u, 127u);

    *accent = deck_lfo_mod_u8_v(gv, base_accent, DECK_LFO_ACCENT, 0u, 127u);
    *human = deck_lfo_mod_u8_v(gv, base_human, DECK_LFO_HUMAN, 0u, 127u);

    pf->tune = deck_lfo_mod_u8_v(gv, base_pf->tune, DECK_LFO_PTUNE, 0u, 127u);
    pf->decay = deck_lfo_mod_u8_v(gv, base_pf->decay, DECK_LFO_PDECAY, 0u, 127u);
    pf->filter = deck_lfo_mod_u8_v(gv, base_pf->filter, DECK_LFO_PFILTER, 0u, 127u);
    pf->crush = deck_lfo_mod_u8_v(gv, base_pf->crush, DECK_LFO_PCRUSH, 0u, 127u);
    pf->density = deck_lfo_mod_u8_v(gv, base_pf->density, DECK_LFO_DENS, 0u, 127u);
    pf->fill = deck_lfo_mod_u8_v(gv, base_pf->fill, DECK_LFO_FILL, 0u, 127u);
    pf->stutter = deck_lfo_mod_u8_v(gv, base_pf->stutter, DECK_LFO_STUT, 0u, 127u);
}

static void deck_apply_lfo_block(uint32_t n)
{
    uint32_t bpm = rt()->bpm ? rt()->bpm : 120u;
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++) {
        fm_lfo_slots_sanitize_rates(deck_lfo_sl(v), DECK_LFO_TGT_N, bpm);
        fm_lfo_slots_step_tr(deck_lfo_sl(v), deck_lfo_rtv(v), DECK_LFO_TGT_N, g.lfo_tgt, bpm, n, DECK_FS,
                             rt()->playing ? 1u : 0u);
    }
}

/* Play / MIDI Start: tempo-synced LFO slots restart at their slot phase. */
static void deck_lfo_transport_start(void)
{
    uint32_t bpm = rt()->bpm ? rt()->bpm : 120u;
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++)
        fm_lfo_slots_sync_restart(deck_lfo_sl(v), deck_lfo_rtv(v), DECK_LFO_TGT_N, bpm);
}

static void app_audio(int32_t *out, uint32_t n)
{
    deck_trig_evt_t ev[DECK_TRIG_Q_MAX];
    uint8_t nev = 0u;
    uint32_t i;
    deck_runtime_t *r = rt();
    deck_voice_cfg_t cfg_hold[DECK_NVOICES];
    uint8_t env_delay_hold[DECK_NVOICES];
    deck_master_t fx_hold;
    deck_perf_t perf_hold;
    uint8_t accent_hold;
    uint8_t human_hold;
    uint8_t v;

    for (i = 0u; i < n * 2u; i++)
        out[i] = 0;
    for (v = 0u; v < DECK_NVOICES; v++) {
        cfg_hold[v] = g.dsp.cfg[v];
        env_delay_hold[v] = g.env_delay[v];
    }
    fx_hold = g.dsp.master;
    perf_hold = g.dsp.perf;
    accent_hold = g.accent_amt;
    human_hold = g.human;
    deck_apply_lfo_block(n);
    deck_lfo_apply_eff(cfg_hold, env_delay_hold, &g.dsp.master, &fx_hold, &g.dsp.perf, &perf_hold, &g.accent_amt,
                       accent_hold, &g.human, human_hold);
    deck_dj_apply_eff(&g.dj_app, &g.dsp.master, &g.dsp.perf, g.dsp.cfg, r->bpm ? r->bpm : 120u, n,
                      r->playing ? 1u : 0u, g.lfo_tgt);
    deck_dsp_sync_env_flags();
    if (g.fx_bypass) {
        g.dsp.master.dly_lvl = 0u;
        g.dsp.master.rev_mix = 0u;
        g.dsp.master.drive = 16u;
        g.dsp.master.crush = 0u;
        g.dsp.master.filter = 64u;
        g.dsp.master.comp = 0u;
        g.dsp.master.dust = 0u;
        g.dsp.master.tape = 0u;
    }
    g.dsp.render_bpm = r->bpm ? r->bpm : 120u;
    if (r->playing && !g.ext_clock) {
        uint32_t si;

        deck_seq_audio_ticks(ev, &nev, n);
        for (si = 0u; si < n; si++)
            deck_seq_ratchet_service(r);
    } else if (r->playing) {
        uint32_t si;

        deck_seq_micro_service(ev, &nev, n);
        for (si = 0u; si < n; si++)
            deck_seq_ratchet_service(r);
    }
    g.audio_pos += n;
    for (i = 0u; i < nev; i++)
        deck_seq_trig_step(r, &ev[i]);
    deck_dsp_render_block(&g.dsp, out, n,
                          deck_dj_lane_mute(&g.dj_app, deck_mute_mask(r)) | deck_dj_silence_mask(&g.dj_app, r->playing),
                          deck_dj_solo(&g.dj_app, r->solo_mask), g.fx_bypass, r->playing);
    for (v = 0u; v < DECK_NVOICES; v++) {
        g.dsp.cfg[v] = cfg_hold[v];
        g.env_delay[v] = env_delay_hold[v];
    }
    g.dsp.master = fx_hold;
    g.dsp.perf = perf_hold;
    g.accent_amt = accent_hold;
    g.human = human_hold;
}

static void app_init(void)
{
    palette_set(2u);
    if (!deck_load_all())
        deck_boot_random_plate();
    deck_seq_g.vary_base = deck_boot_seed(); /* after deck_load_all: deck_seq_init zeroes deck_seq_g */
    g.seq_page = DECK_SEQ_KNOB_PAGE_CYCLE[0];
    g.seq_plock_step = FM_SEQ_PLOCK_NONE;
    g.seq_follow = 1u;
    g.ui_dirty = 1u;
    g.view = DECK_VIEW_HOME;
    deck_draw_screen();
}

static void app_step(void)
{
    uint32_t b, rising, released, keys = fm1_in.notes;
    uint8_t st, d1, d2;
    deck_runtime_t *r = rt();
    static uint32_t last_step;

    platform_poll();
    b = fm1_input_edges(&released);
    rising = keys & ~g.prev_keys;
    {
        uint32_t falling = g.prev_keys & ~keys;

        g.prev_keys = keys;
        deck_handle_keys(rising, falling, fm1_in.buttons);
    }

    if (b & (1u << BTN_REC))
        g.rec_press_ms = fm1_ms;
    if ((released & (1u << BTN_REC)) && g.rec_press_ms) {
        if (fm1_ms - g.rec_press_ms < DECK_HOLD_MS)
            r->rec_arm = r->rec_arm ? 0u : 1u;
        g.rec_press_ms = 0u;
        mark_dirty();
    }
    if (b & (1u << BTN_PLAY)) {
        if (r->playing)
            deck_seq_transport_stop();
        else {
            deck_seq_transport_start();
            deck_lfo_transport_start();
        }
        mark_dirty();
    }
    if (b & (1u << BTN_OCT_UP))
        deck_bar_page_step(1);
    if (b & (1u << BTN_OCT_DOWN))
        deck_bar_page_step(-1);
    if (b & (1u << BTN_SEQ)) {
        g.seq_press_ms = fm1_ms;
        g.seq_long_used = 0u;
        g.seq_was_in_seq = (uint8_t)(g.view == DECK_VIEW_SEQ);
    }
    {
        ko_longpress_t lp = {&g.seq_press_ms, &g.seq_long_used, NULL};

        if ((fm1_in.buttons & (1u << BTN_SEQ)) && g.seq_press_ms &&
            ko_longpress_poll(&lp, fm1_ms, DECK_HOLD_MS, 0)) {
            g.seq_follow = g.seq_follow ? 0u : 1u;
            deck_toast("FOLLOW", C_ACC);
        }
        if ((released & (1u << BTN_SEQ)) && !g.seq_long_used) {
            /* Pad p-lock: SEQ tap toggles knob bank (P1 VEL/COND/MICRO/PITCH vs P2 GATE…), keeps hold. */
            if (g.seq_plock_latch) {
                g.seq_plock_bank ^= 1u;
                mark_dirty();
            } else if (g.seq_was_in_seq)
                deck_seq_page_step();
            else {
                deck_lane_page_follow_voice(r);
                g.seq_page = DECK_SEQ_KNOB_PAGE_CYCLE[0];
                g.view = DECK_VIEW_SEQ;
                mark_dirty();
            }
        }
    }
    if (b & (1u << BTN_SAVE)) {
        g.save_press_ms = fm1_ms;
        g.save_long_used = 0u;
    }
    {
        ko_longpress_t lp = {&g.save_press_ms, &g.save_long_used, NULL};
        uint8_t save_mode = (fm1_in.buttons & (1u << BTN_EDIT)) ? DECK_SAVE_FACTORY : DECK_SAVE_PLATE;

        if ((fm1_in.buttons & (1u << BTN_SAVE)) && g.save_press_ms &&
            ko_longpress_poll(&lp, fm1_ms, DECK_HOLD_MS, 0) && deck_is_quiet())
            deck_save_all(save_mode);
        if ((released & (1u << BTN_SAVE)) && !g.save_long_used && deck_is_quiet())
            deck_save_all(save_mode);
    }

    /* DJ (deck_dj.h): BTN_FX short = BANK_FX on release, long >= DECK_HOLD_MS toggles DJ. */
    {
        fm_dj_app_fx_t fx_io = deck_dj_fx_io(&g.dj_app);
        uint8_t dj_was_on = g.dj_app.dj.on;

        if (b & (1u << BTN_FX))
            fm_dj_app_fx_on_press(&fx_io, fm1_ms);
        fm_dj_app_fx_long_press_poll(&fx_io, b, rising, fm1_in.buttons, (1u << BTN_FX), fm1_ms, (uint32_t)fm1_ms * 2654435761u ^ 0x9E3779B9u);
        if (g.dj_app.dj.on != dj_was_on) {
            deck_toast(g.dj_app.dj.on ? "DJ ON" : "DJ OFF", C_ACC);
            deck_dj_fx_flash_start();
        }
        if (released & (1u << BTN_FX))
            fm_dj_app_fx_on_release(&fx_io, fm1_ms, deck_dj_fx_short);
    }
    deck_dj_app_step(&g.dj_app, r, g.era, deck_dj_on_trans);
    deck_dj_toast_overlay();
    /* Gesture labels ride the sticky DJ side (deck_dj_toast_side), no separate 900 ms toast. */
    g.dj_app.gest_toast = 0u;
    if (g.dj_app.ui_dirty) {
        g.dj_app.ui_dirty = 0u;
        mark_dirty();
    }
    deck_handle_banks(b, released, fm1_in.buttons);
    deck_plock_latch_touch_break(b, rising);
    deck_white_plock_latch_poll(keys, rising);
    deck_groove_commit_if_pending(r);
    deck_enc_nav();
    deck_enc_knobs();

    if (r->playing && r->master_step != last_step) {
        last_step = r->master_step;
        if (g.seq_follow) {
            /* Page of the step the selected lane just fired (same playhead as the grid); lanes wrap at their own length. */
            uint8_t ps = deck_ui_last_step(r, pat(), (uint8_t)(r->cur_voice % DECK_NVOICES));

            if (ps != DECK_UI_NO_STEP)
                r->bar_page = (uint8_t)((ps / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
        }
        deck_arp_tick();
    }

    while (midi_get(&st, &d1, &d2)) {
        if (st == 0xF8u && r->playing && g.clock) {
            g.ext_clock = 1u;
            g.midi_clk_div++;
            {
                uint8_t num = FM_SEQ_CLK_RATIO_NUM[fm_seq_clk_ratio_clamp(r->seq_clk)];
                uint8_t den = FM_SEQ_CLK_RATIO_DEN[fm_seq_clk_ratio_clamp(r->seq_clk)];
                uint8_t thr = (uint8_t)((6u * (uint32_t)den + (uint32_t)num - 1u) / (uint32_t)num);

                if (g.midi_clk_div >= thr) {
                    deck_trig_evt_t ev[DECK_TRIG_Q_MAX];
                    uint8_t nev = 0u;
                    uint8_t i;

                    g.midi_clk_div = 0u;
                    deck_ext_s16_measure();
                    deck_seq_clock_16th(ev, &nev, g.ext_sps);
                    for (i = 0u; i < nev; i++)
                        deck_seq_trig_step(r, &ev[i]);
                    deck_arp_tick();
                }
            }
        } else if (st == 0xFAu || st == 0xFBu) {
            g.ext_clock = 1u;
            g.midi_clk_div = 0u;
            g.ext_s16_mark = 0u;
            g.ext_sps = 0u;
            deck_seq_transport_start();
            if (st == 0xFAu)
                deck_lfo_transport_start();
        } else if (st == 0xFCu) {
            g.ext_clock = 0u;
            g.midi_clk_div = 0u;
            g.ext_s16_mark = 0u;
            g.ext_sps = 0u;
            deck_seq_transport_stop();
        } else if ((st & 0xF0u) == 0x90u && d2 && ((st & 0x0Fu) == g.midi_ch || g.midi_ch == 0u)) {
            uint8_t v = deck_voice_from_midi(d1);
            uint8_t accent = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;

            if (v < DECK_NVOICES) {
                deck_trig_live(v, d2, accent);
                if (r->rec_arm && r->playing) {
                    deck_seq_record_step(pat(), r, v, accent);
                    mark_dirty();
                }
            }
        } else if (((st & 0xF0u) == 0x80u || ((st & 0xF0u) == 0x90u && d2 == 0u)) &&
                   ((st & 0x0Fu) == g.midi_ch || g.midi_ch == 0u)) {
            uint8_t v = deck_voice_from_midi(d1);

            if (v < DECK_NVOICES)
                deck_midi_out_queue(DECK_MIDI_NOTE[v], 0u);
        }
    }

    deck_midi_out_flush();
    deck_update_leds(keys);

    if (g.ui_dirty || fm1_ms - g.ui_ms >= 40u) {
        deck_draw_screen();
        g.ui_dirty = 0u;
        g.ui_ms = fm1_ms;
    }
    if (g.set_dirty && deck_is_quiet() && fm1_ms - g.set_t > 2000u)
        deck_save_all(DECK_SAVE_WORK);
}

static void app_main(void)
{
    app_init();
    for (;;)
        app_step();
}
