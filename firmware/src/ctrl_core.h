/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CTRL_CORE_H
#define CTRL_CORE_H

#include <stdint.h>
#include "fm_lfo.h"
#include "fm_seq.h"
#include "midi_ctrl.h"
#include "synth_fm2.h"

#define CTRL_CFG_MAGIC 0x43545236u    /* "CTR6": 128 steps */
#define CTRL_CFG_MAGIC_V5 0x43545235u /* "CTR5": 16 steps, migrated on load */
#define CTRL_NPARAMS 32u
#define CTRL_NPROFILE_SLOTS 16u
#define CTRL_NTAGS 48u
#define CTRL_NPROFILES 8u
#define CTRL_NSCALES 12u
#define CTRL_NCHORDS 8u
#define CTRL_NSTEPS FM_SEQ_STEPS
#define CTRL_NSTEPS_V5 16u
#define CTRL_NLANES 4u
#define CTRL_MOTION_BUCKETS 64u
#define CTRL_CC_MIN_INTERVAL_MS 12u
#define CTRL_CC_MAX_PER_MS 2u

enum {
    CTRL_VIEW_BANK = 0,
    CTRL_VIEW_LFO = 1,
    CTRL_VIEW_ARP = 2,
    CTRL_VIEW_SEQ = 3,
    CTRL_VIEW_REC = 4,
    CTRL_VIEW_HOME = 5,
    CTRL_VIEW_COUNT = 6
};

enum {
    CTRL_BANK_A = 0,
    CTRL_BANK_B = 1,
    CTRL_BANK_C = 2,
    CTRL_BANK_D = 3,
    CTRL_BANK_LFO = 4,
    CTRL_BANK_ARP = 5,
    CTRL_BANK_SEQ = 6,
    CTRL_BANK_REC = 7,
    CTRL_BANK_HOME = 8,
    CTRL_BANK_COUNT = 9
};

enum {
    CTRL_LOCAL_SYNTH_VOL = 0,
    CTRL_LOCAL_SYNTH_CC1 = 1,
    CTRL_LOCAL_MIDI_CC1 = 2,
    CTRL_LOCAL_MIDI_EXP = 3
};

enum {
    CTRL_MOTION_OFF = 0,
    CTRL_MOTION_ARM = 1,
    CTRL_MOTION_REC = 2,
    CTRL_MOTION_PLAY = 3,
    CTRL_MOTION_CLEAR = 4
};

static const char *const CTRL_PROFILE_NAMES[CTRL_NPROFILES] = {
    "GENERIC CC", "JUNO / ANALOG", "DX7 / FM", "303 ACID", "WAVETABLE", "DRUM / 909", "EURORACK CV",
    "DAW MIX"};

static const uint8_t CTRL_PROFILE_CC[CTRL_NPROFILES][CTRL_NPROFILE_SLOTS] = {
    {1, 2, 74, 71, 73, 75, 76, 72, 91, 93, 94, 95, 16, 17, 18, 19},
    {3, 15, 74, 71, 16, 73, 75, 70, 72, 18, 19, 93, 91, 20, 10, 7},
    {1, 2, 16, 17, 18, 19, 20, 21, 74, 71, 73, 75, 72, 94, 93, 91},
    {74, 71, 12, 75, 16, 5, 17, 18, 19, 20, 21, 22, 91, 23, 24, 7},
    {16, 17, 18, 19, 20, 21, 22, 23, 74, 71, 73, 75, 24, 94, 93, 91},
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 74, 71, 94, 91, 26, 7},
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 74, 75},
    {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 7}};

static const char *const CTRL_PROFILE_LABELS[CTRL_NPROFILES][CTRL_NPROFILE_SLOTS] = {
    {"CC1", "CC2", "CUT", "RES", "ATK", "DEC", "SUS", "REL", "DLY", "CHR", "BRG", "PHS", "P13", "P14",
     "P15", "P16"},
    {"LFO", "PWM", "CUT", "RES", "ENV", "ATK", "DEC", "SUS", "REL", "SUB", "NSE", "CHR", "DLY", "DRV",
     "PAN", "VOL"},
    {"MW", "BRTH", "ALG", "FB", "OP1", "OP2", "OP3", "OP4", "TIM", "RAT", "ATK", "DEC", "REL", "DET",
     "CHR", "REV"},
    {"CUT", "RES", "ENV", "DEC", "ACC", "SLD", "TUN", "WAV", "SUB", "DRV", "D.T", "D.F", "D.M", "FFM",
     "GAT", "VOL"},
    {"M1", "M2", "M3", "M4", "WT A", "WT B", "WRP", "SUB", "CUT", "RES", "ATK", "DEC", "UNI", "DET",
     "DIM", "REV"},
    {"K.T", "K.D", "S.T", "S.S", "H.D", "CLV", "T.T", "P.T", "COMP", "DRV", "FC", "FR", "SDY", "SRV",
     "SWG", "MST"},
    {"CV1", "CV2", "CV3", "CV4", "MA", "MB", "MC", "MD", "SL1", "SL2", "CLK", "G.P", "E.H", "E.R", "FOLD",
     "LPG"},
    {"TR1", "TR2", "TR3", "TR4", "SA1", "SA2", "SA3", "SA4", "PN1", "PN2", "PN3", "PN4", "FX1", "FX2",
     "CUE", "MST"}};

static const uint8_t CTRL_PROFILE_INIT[CTRL_NPROFILES][CTRL_NPROFILE_SLOTS] = {
    {64, 64, 96, 48, 32, 64, 80, 40, 0, 0, 0, 0, 64, 64, 64, 64},
    {48, 64, 96, 64, 72, 8, 48, 96, 32, 32, 0, 48, 0, 0, 64, 100},
    {0, 0, 32, 16, 99, 99, 80, 60, 64, 64, 4, 48, 32, 0, 0, 0},
    {32, 127, 64, 48, 127, 32, 64, 0, 32, 64, 32, 48, 0, 0, 64, 100},
    {64, 64, 64, 64, 0, 0, 32, 48, 96, 48, 8, 48, 32, 16, 0, 24},
    {64, 48, 64, 80, 32, 48, 64, 64, 48, 32, 127, 32, 0, 24, 54, 100},
    {64, 64, 64, 64, 0, 0, 0, 0, 32, 32, 64, 64, 8, 0, 96, 48},
    {96, 96, 96, 96, 0, 0, 0, 0, 64, 64, 64, 64, 32, 32, 64, 100}};

static const char CTRL_TAG_LIB[CTRL_NTAGS][4] = {
    {'C', 'U', 'T', ' '}, {'R', 'E', 'S', ' '}, {'A', 'T', 'K', ' '}, {'D', 'E', 'C', ' '},
    {'S', 'U', 'S', ' '}, {'R', 'E', 'L', ' '}, {'E', 'N', 'V', ' '}, {'L', 'F', 'O', ' '},
    {'P', 'W', 'M', ' '}, {'S', 'U', 'B', ' '}, {'D', 'R', 'V', ' '}, {'F', 'O', 'L', 'D'},
    {'T', 'U', 'N', 'E'}, {'F', 'I', 'N', 'E'}, {'G', 'L', 'D', ' '}, {'U', 'N', 'I', 'S'},
    {'D', 'E', 'T', ' '}, {'S', 'P', 'R', 'D'}, {'W', 'T', ' ', 'A'}, {'W', 'T', ' ', 'B'},
    {'M', 'O', 'R', 'P'}, {'F', 'M', ' ', ' '}, {'R', 'A', 'T', ' '}, {'I', 'D', 'X', ' '},
    {'F', 'B', ' ', ' '}, {'R', 'I', 'N', 'G'}, {'N', 'O', 'I', 'S'}, {'H', 'P', 'F', ' '},
    {'B', 'P', 'F', ' '}, {'L', 'P', 'F', ' '}, {'D', 'L', 'Y', ' '}, {'T', 'I', 'M', 'E'},
    {'D', '.', 'F', 'B'}, {'R', 'E', 'V', ' '}, {'S', 'I', 'Z', 'E'}, {'D', 'A', 'M', 'P'},
    {'C', 'H', 'O', 'R'}, {'P', 'H', 'A', 'S'}, {'F', 'L', 'N', 'G'}, {'C', 'O', 'M', 'P'},
    {'P', 'U', 'M', 'P'}, {'P', 'A', 'N', ' '}, {'S', 'E', 'N', 'D'}, {'A', 'U', 'X', ' '},
    {'M', '1', ' ', ' '}, {'M', '2', ' ', ' '}, {'M', '3', ' ', ' '}, {'M', '4', ' ', ' '}};

static const char *const CTRL_SCALE_NAMES[CTRL_NSCALES] = {
    "CHROM", "MAJOR", "MINOR", "DORIAN", "MIXO", "LYDIAN", "PHRYG", "PENT+", "PENT-", "BLUES", "JAPAN",
    "HIJAZ"};

static const int8_t CTRL_SCALE_DEG[CTRL_NSCALES][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7}, {0, 2, 4, 5, 7, 9, 11, 12}, {0, 2, 3, 5, 7, 8, 10, 12},
    {0, 2, 3, 5, 7, 9, 10, 12}, {0, 2, 4, 5, 7, 9, 10, 12}, {0, 2, 4, 6, 7, 9, 11, 12},
    {0, 1, 3, 5, 7, 8, 10, 12}, {0, 2, 4, 7, 9, 12, 14, 16}, {0, 3, 5, 7, 10, 12, 15, 17},
    {0, 3, 5, 6, 7, 10, 12, 15}, {0, 2, 5, 7, 8, 12, 14, 17}, {0, 1, 4, 5, 7, 8, 11, 12}};

static const char *const CTRL_CHORD_NAMES[CTRL_NCHORDS] = {
    "OFF", "PWR5", "TRIAD", "7TH", "9TH", "DROP2", "QUART", "OCT"};

typedef struct {
    uint32_t magic;
    uint8_t profile;
    uint8_t midi_ch;
    uint8_t local_pot;
    uint8_t root;
    uint8_t scale;
    uint8_t chord;
    uint8_t arp_mode;
    uint8_t arp_rate;
    uint8_t vel_mode;
    uint8_t bpm;
    uint8_t swing;
    uint8_t motion_bars;
    uint8_t lfo_target;
    uint8_t seq_lane;
    uint8_t seq_pages; /* pattern length in pages of 16 steps, 1..8 */
    uint8_t pad;
    uint8_t cc_map[CTRL_NPARAMS];
    uint8_t base_val[CTRL_NPARAMS];
    uint8_t lfo_depth[CTRL_NPARAMS];
    uint8_t lfo_rate[CTRL_NPARAMS];
    uint8_t lfo_shape[CTRL_NPARAMS];
    uint8_t lfo_phase[CTRL_NPARAMS];
    uint8_t lfo_smooth[CTRL_NPARAMS];
    uint8_t lfo_skew[CTRL_NPARAMS];
    uint8_t lfo_algo[CTRL_NPARAMS];
    uint8_t lfo_ratio[CTRL_NPARAMS];
    uint8_t lfo_index[CTRL_NPARAMS];
    uint8_t lfo_fb[CTRL_NPARAMS];
    uint8_t lfo_fold[CTRL_NPARAMS];
    uint8_t arp_span;
    uint8_t arp_gate;
    int8_t seq_note_ofs[CTRL_NLANES][CTRL_NSTEPS];
    uint8_t seq_vel[CTRL_NLANES][CTRL_NSTEPS];
    uint8_t seq_prob[CTRL_NLANES][CTRL_NSTEPS];
    uint8_t seq_pace;
    uint8_t home_macro[4];
    uint8_t seq_pat[CTRL_NLANES][FM_SEQ_MASK_BYTES];
    uint8_t lane_mute[CTRL_NLANES];
    uint8_t euclid_hits[CTRL_NLANES];
    int8_t euclid_rot[CTRL_NLANES];
    uint8_t bank_page[CTRL_BANK_COUNT];
    uint8_t bank_count[4];
    char custom_label[CTRL_NPARAMS][4];
} ctrl_cfg_t;

typedef struct {
    ctrl_cfg_t cfg;
    uint8_t profile;
    uint8_t view;
    uint8_t bank;
    uint8_t learn_active;
    uint8_t learn_slot;
    uint8_t edit_char_ix;
    uint8_t tag_ix;
    uint8_t playing;
    uint8_t ext_clock;
    uint8_t motion_state;
    uint8_t home_radar[16];
    uint16_t bpm_eff;
    uint8_t eff_val[CTRL_NPARAMS];
    uint8_t last_sent[CTRL_NPARAMS];
    uint32_t cc_last_ms[CTRL_NPARAMS];
    uint32_t cc_rr;
    uint32_t lfo_ph[CTRL_NPARAMS];
    uint32_t lfo_cycle[CTRL_NPARAMS];
    int8_t lfo_sh[CTRL_NPARAMS];
    uint32_t lfo_rng[CTRL_NPARAMS];
    uint32_t lfo_last_ms;
    int16_t lfo_scope[240];
    int32_t lfo_slew_out[CTRL_NPARAMS];
    uint64_t lfo_ph64[CTRL_NPARAMS];
    uint8_t seq_plock_step;
    uint8_t seq_plock_knob;
    uint8_t edit_used;
    uint8_t edit_down;
    uint8_t motion[CTRL_NPARAMS][CTRL_MOTION_BUCKETS];
    uint8_t motion_slot;
    uint32_t motion_bucket;
    uint32_t motion_loop;
    uint32_t tick;
    uint32_t step;
    uint32_t bar;
    uint8_t roll_mask;
    uint32_t rng;
    uint32_t ui_dirty;
    uint32_t set_dirty;
    uint32_t set_t;
    uint32_t saved_until;
    uint32_t ui_ms;
    uint32_t prev_keys;
    uint8_t held_notes[27][5];
    uint8_t held_count[27];
    uint8_t seq_active_note[CTRL_NLANES];
    uint32_t seq_off_tick[CTRL_NLANES];
    uint8_t arp_active_notes[5];
    uint8_t arp_active_count;
    int32_t oct_off;
    uint8_t lfo_key_page;
    int32_t master_last;
    int32_t master_sent;
    uint8_t arp_ix;
    uint8_t arp_dir;
    uint8_t held_root;
    midi_state_t midi;
    midi_cfg_t cue_cfg;
    uint32_t int_last_ms;
    uint32_t int_rem_ms;
    uint32_t step_ticks;
    uint32_t raw_tick;
    uint8_t swing_held;
    uint32_t last_f8_ms;
    uint32_t f8_count;
    uint32_t clk_acc_ms;
    uint8_t seq_vary[CTRL_NLANES];
    uint8_t pattern_origin[CTRL_NLANES];
    int8_t seq_lane_vel[CTRL_NLANES];
    int8_t seq_lane_trig[CTRL_NLANES];
    int8_t seq_lane_gate[CTRL_NLANES];
    int8_t seq_lane_tune[CTRL_NLANES];
    int8_t seq_micro[CTRL_NLANES][CTRL_NSTEPS];
    uint32_t bk_last_ms[CTRL_NLANES];
    uint8_t lane_solo_mask;
    uint8_t heard_mute_mask;
    uint8_t seq_ens_sel;
    uint8_t seq_flip_state;
    uint32_t seq_knob_flash_ms;
    char seq_knob_flash[8];
    uint8_t seq_view_page; /* the page of 16 steps on screen and under the keys */
} ctrl_t;

ctrl_t g_ctrl;

_Static_assert(sizeof(ctrl_cfg_t) <= 4080u, "ctrl_cfg_t fits in one 4 KiB settings sector");

static inline uint8_t ctrl_seq_pages(const ctrl_t *g)
{
    return fm_seq_pages_clamp(g->cfg.seq_pages);
}

static inline uint32_t ctrl_seq_len(const ctrl_t *g)
{
    return fm_seq_len(g->cfg.seq_pages);
}

static inline uint32_t ctrl_seq_view_base(const ctrl_t *g)
{
    return (uint32_t)(g->seq_view_page % ctrl_seq_pages(g)) * FM_SEQ_PAGE_STEPS;
}

/* OCT -/+ in SEQ (fm_seq_view_nav). A doubling copies every per-step array; after a prepend the playhead
 * and the held step stay on the original, now the second half. */
static uint8_t ctrl_seq_view_nav(ctrl_t *g, int32_t dir)
{
    uint8_t pages = ctrl_seq_pages(g);
    uint32_t len = ctrl_seq_len(g);
    uint8_t nav = fm_seq_view_nav(&g->seq_view_page, pages, dir);
    uint32_t lane;

    if (nav != FM_SEQ_NAV_APPEND && nav != FM_SEQ_NAV_PREPEND)
        return nav;
    for (lane = 0; lane < CTRL_NLANES; lane++) {
        fm_seq_bits_dup(g->cfg.seq_pat[lane], len);
        fm_seq_dup_bytes(g->cfg.seq_note_ofs[lane], len, 1u);
        fm_seq_dup_bytes(g->cfg.seq_vel[lane], len, 1u);
        fm_seq_dup_bytes(g->cfg.seq_prob[lane], len, 1u);
        fm_seq_dup_bytes(g->seq_micro[lane], len, 1u);
    }
    g->cfg.seq_pages = (uint8_t)(pages * 2u);
    g->step = fm_seq_dup_ix(g->step, len, nav);
    if (g->seq_plock_step != 255u)
        g->seq_plock_step = (uint8_t)fm_seq_dup_ix(g->seq_plock_step, len, nav);
    return nav;
}

static inline void ctrl_profile_sync(ctrl_t *g)
{
    g->profile = g->cfg.profile;
}

static inline int ctrl_local_on(const ctrl_t *g)
{
    return g->cfg.local_pot <= CTRL_LOCAL_SYNTH_CC1;
}

static inline uint8_t ctrl_midi_ch(const ctrl_t *g)
{
    uint8_t ch = g->cfg.midi_ch;

    if (ch < 1u)
        ch = 1u;
    if (ch > 16u)
        ch = 16u;
    return (uint8_t)(ch - 1u);
}

static inline uint32_t ctrl_rng_next(ctrl_t *g)
{
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

static inline void ctrl_label_set4(char dst[4], const char *src)
{
    uint32_t i;

    for (i = 0; i < 4u; i++) {
        char c = src ? src[i] : 0;
        dst[i] = c ? c : ' ';
    }
}

static inline void ctrl_slot_label(const ctrl_cfg_t *c, uint32_t slot, char out[5])
{
    uint32_t i;

    if (slot >= CTRL_NPARAMS) {
        out[0] = 0;
        return;
    }
    for (i = 0; i < 4u; i++)
        out[i] = c->custom_label[slot][i];
    out[4] = 0;
    for (i = 4u; i > 0u; i--) {
        if (out[i - 1u] != ' ')
            break;
        out[i - 1u] = 0;
    }
}

static inline int ctrl_slot_active(const ctrl_cfg_t *c, uint32_t slot)
{
    uint32_t b, col, cnt;

    if (slot >= CTRL_NPARAMS)
        return 0;
    b = (slot >> 3) & 3u;
    col = slot & 7u;
    cnt = c->bank_count[b];
    if (cnt < 1u)
        cnt = 1u;
    if (cnt > 8u)
        cnt = 8u;
    return col < cnt;
}

static inline uint32_t ctrl_active_count(const ctrl_cfg_t *c)
{
    uint32_t b, n = 0;

    for (b = 0; b < 4u; b++) {
        uint32_t cnt = c->bank_count[b];

        if (cnt < 1u)
            cnt = 1u;
        if (cnt > 8u)
            cnt = 8u;
        n += cnt;
    }
    return n;
}

static inline uint32_t ctrl_active_nth(const ctrl_cfg_t *c, uint32_t n)
{
    uint32_t b, col, ix = 0;

    for (b = 0; b < 4u; b++) {
        uint32_t cnt = c->bank_count[b];

        if (cnt < 1u)
            cnt = 1u;
        if (cnt > 8u)
            cnt = 8u;
        for (col = 0; col < cnt; col++) {
            if (ix == n)
                return b * 8u + col;
            ix++;
        }
    }
    return 0u;
}

static inline uint8_t ctrl_lfo_target_step(const ctrl_cfg_t *c, uint8_t cur, int32_t delta)
{
    uint32_t n = ctrl_active_count(c);
    uint32_t b, col, ix = 0, cur_ix = n;
    int32_t new_ix;

    if (n == 0u)
        return 0u;
    if (!ctrl_slot_active(c, cur)) {
        if (delta < 0)
            return (uint8_t)ctrl_active_nth(c, n - 1u);
        return (uint8_t)ctrl_active_nth(c, 0u);
    }
    for (b = 0; b < 4u; b++) {
        uint32_t cnt = c->bank_count[b];

        if (cnt < 1u)
            cnt = 1u;
        if (cnt > 8u)
            cnt = 8u;
        for (col = 0; col < cnt; col++) {
            if (b * 8u + col == (uint32_t)cur)
                cur_ix = ix;
            ix++;
        }
    }
    if (cur_ix >= n)
        return (uint8_t)ctrl_active_nth(c, 0u);
    if (delta == 0)
        return cur;
    new_ix = (int32_t)cur_ix + delta;
    while (new_ix < 0)
        new_ix += (int32_t)n;
    new_ix %= (int32_t)n;
    return (uint8_t)ctrl_active_nth(c, (uint32_t)new_ix);
}

static void ctrl_apply_profile(ctrl_cfg_t *c, uint8_t profile)
{
    uint8_t p = (uint8_t)(profile % CTRL_NPROFILES);
    uint32_t b, col;

    c->profile = p;
    for (b = 0; b < 4u; b++) {
        static const char usr[4][4] = {{'U', 'S', 'R', '1'}, {'U', 'S', 'R', '2'}, {'U', 'S', 'R', '3'},
                                       {'U', 'S', 'R', '4'}};

        c->bank_count[b] = 4u;
        for (col = 0; col < 4u; col++) {
            uint32_t slot = b * 8u + col;
            uint32_t p_ix = b * 4u + col;

            c->cc_map[slot] = CTRL_PROFILE_CC[p][p_ix];
            c->base_val[slot] = CTRL_PROFILE_INIT[p][p_ix];
            ctrl_label_set4(c->custom_label[slot], CTRL_PROFILE_LABELS[p][p_ix]);
        }
        for (col = 4; col < 8u; col++) {
            uint32_t slot = b * 8u + col;
            uint32_t i;

            c->cc_map[slot] = (uint8_t)(20u + b * 4u + (col - 4u));
            c->base_val[slot] = 64u;
            for (i = 0; i < 4u; i++)
                c->custom_label[slot][i] = usr[col - 4u][i];
        }
    }
}

static void ctrl_cfg_defaults(ctrl_cfg_t *c)
{
    uint32_t i;

    c->magic = CTRL_CFG_MAGIC;
    c->profile = 0;
    c->midi_ch = 1;
    c->local_pot = CTRL_LOCAL_SYNTH_VOL;
    c->root = 0;
    c->scale = 1;
    c->chord = 2;
    c->arp_mode = 0;
    c->arp_rate = 4;
    c->vel_mode = 3;
    c->bpm = 60;
    c->swing = 32;
    c->motion_bars = 2;
    c->lfo_target = 0;
    c->seq_lane = 0;
    c->seq_pages = 1;
    c->arp_span = 0;
    c->arp_gate = 64;
    c->seq_pace = 1;
    c->home_macro[0] = 64;
    c->home_macro[1] = 64;
    c->home_macro[2] = 100;
    c->home_macro[3] = 32;
    ctrl_apply_profile(c, 0);
    for (i = 0; i < CTRL_NPARAMS; i++) {
        c->lfo_depth[i] = 0;
        c->lfo_shape[i] = 0;
        c->lfo_phase[i] = 0;
        c->lfo_smooth[i] = 0;
        c->lfo_skew[i] = FM_LFO_SKEW_DEFAULT;
        c->lfo_algo[i] = FM_LFO_ALG_CLASSIC;
        c->lfo_ratio[i] = 52;
        c->lfo_index[i] = 64;
        c->lfo_fb[i] = 0;
        c->lfo_fold[i] = 0;
    }
    for (i = 0; i < CTRL_NLANES; i++) {
        uint32_t s;
        fm_seq_bits_fill(c->seq_pat[i], 0);
        c->lane_mute[i] = 0;
        c->euclid_hits[i] = 0;
        c->euclid_rot[i] = 0;
        for (s = 0; s < CTRL_NSTEPS; s++) {
            c->seq_note_ofs[i][s] = 0;
            c->seq_vel[i][s] = 100;
            c->seq_prob[i][s] = 100;
        }
    }
    for (i = 0; i < CTRL_BANK_COUNT; i++)
        c->bank_page[i] = 0;
}

static inline int32_t ctrl_clamp127(int32_t v)
{
    if (v < 0)
        return 0;
    if (v > 127)
        return 127;
    return v;
}

static inline uint8_t ctrl_scale_note(uint8_t root, uint8_t scale, int32_t deg, int32_t oct)
{
    uint8_t sc = (uint8_t)(scale % CTRL_NSCALES);
    int32_t n = (int32_t)(deg % 8);
    int32_t o = deg / 8;

    if (n < 0) {
        n += 8;
        o--;
    }
    return (uint8_t)ctrl_clamp127((int32_t)root + (int32_t)CTRL_SCALE_DEG[sc][(uint32_t)n] + (o + oct) * 12);
}

static void ctrl_chord_notes(const ctrl_t *g, uint8_t root_note, uint8_t *out, uint32_t *nout)
{
    uint8_t ch = g->cfg.chord % CTRL_NCHORDS;
    uint32_t n = 0;

    if (ch == 0u) {
        out[n++] = root_note;
    } else if (ch == 1u) {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 7u);
    } else if (ch == 2u) {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 4u);
        out[n++] = (uint8_t)(root_note + 7u);
    } else if (ch == 3u) {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 4u);
        out[n++] = (uint8_t)(root_note + 7u);
        out[n++] = (uint8_t)(root_note + 10u);
    } else if (ch == 4u) {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 4u);
        out[n++] = (uint8_t)(root_note + 7u);
        out[n++] = (uint8_t)(root_note + 14u);
        out[n++] = (uint8_t)(root_note + 17u);
    } else if (ch == 5u) {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 7u);
        out[n++] = (uint8_t)(root_note + 12u);
        out[n++] = (uint8_t)(root_note + 16u);
    } else if (ch == 6u) {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 5u);
        out[n++] = (uint8_t)(root_note + 10u);
        out[n++] = (uint8_t)(root_note + 15u);
    } else {
        out[n++] = root_note;
        out[n++] = (uint8_t)(root_note + 12u);
        out[n++] = (uint8_t)(root_note + 24u);
    }
    *nout = n;
}

static inline void ctrl_euclid_fill(uint8_t *pat, uint8_t hits, int8_t rot)
{
    fm_seq_bits_tile16(pat, fm_seq_euclid16(hits, rot));
}

static inline uint8_t ctrl_lane_is_solo(const ctrl_t *g, uint8_t lane)
{
    uint8_t bit;

    if (lane >= CTRL_NLANES)
        return 0u;
    bit = (uint8_t)(1u << lane);
    return (g->lane_solo_mask & bit) && !g->cfg.lane_mute[lane] ? 1u : 0u;
}

static inline uint8_t ctrl_any_lane_mute_or_solo(const ctrl_t *g)
{
    uint32_t i;

    if (g->lane_solo_mask)
        return 1u;
    for (i = 0; i < CTRL_NLANES; i++) {
        if (g->cfg.lane_mute[i])
            return 1u;
    }
    return 0u;
}

static void ctrl_seq_lane_note_off(ctrl_t *g, uint32_t lane);

static inline void ctrl_sync_heard_mutes(ctrl_t *g);
static inline void ctrl_maybe_sync_heard_mutes(ctrl_t *g);

static inline void ctrl_unmute_all_lanes(ctrl_t *g)
{
    uint32_t i;

    g->lane_solo_mask = 0u;
    for (i = 0; i < CTRL_NLANES; i++)
        g->cfg.lane_mute[i] = 0u;
    ctrl_maybe_sync_heard_mutes(g);
}

static inline void ctrl_lane_toggle_mute(ctrl_t *g, uint8_t lane)
{
    uint8_t bit;

    if (lane >= CTRL_NLANES)
        return;
    bit = (uint8_t)(1u << lane);
    if (g->cfg.lane_mute[lane]) {
        g->cfg.lane_mute[lane] = 0u;
        if (g->lane_solo_mask)
            g->lane_solo_mask |= bit;
    } else {
        g->cfg.lane_mute[lane] = 1u;
        if (g->lane_solo_mask)
            g->lane_solo_mask &= (uint8_t)~bit;
    }
    ctrl_maybe_sync_heard_mutes(g);
}

static inline void ctrl_lane_toggle_solo(ctrl_t *g, uint8_t lane)
{
    uint32_t i;
    uint8_t bit;

    if (lane >= CTRL_NLANES)
        return;
    bit = (uint8_t)(1u << lane);
    if (g->lane_solo_mask & bit) {
        g->lane_solo_mask &= (uint8_t)~bit;
        if (g->lane_solo_mask == 0u)
            ctrl_unmute_all_lanes(g);
        else
            g->cfg.lane_mute[lane] = 1u;
    } else if (g->lane_solo_mask == 0u) {
        g->lane_solo_mask = bit;
        for (i = 0; i < CTRL_NLANES; i++)
            g->cfg.lane_mute[i] = (uint8_t)(i != (uint32_t)lane);
    } else {
        g->lane_solo_mask |= bit;
        g->cfg.lane_mute[lane] = 0u;
    }
    ctrl_maybe_sync_heard_mutes(g);
}

static inline uint8_t ctrl_lane_audible(const ctrl_t *g, uint8_t lane)
{
    if (lane >= CTRL_NLANES)
        return 0u;
    return g->cfg.lane_mute[lane] ? 0u : 1u;
}

static inline void ctrl_seq_lane_clear_meta(ctrl_t *g, uint8_t lane)
{
    uint32_t s;

    if (lane >= CTRL_NLANES)
        return;
    g->cfg.euclid_hits[lane] = 0;
    g->cfg.euclid_rot[lane] = 0;
    g->seq_vary[lane] = 0;
    g->seq_lane_vel[lane] = 0;
    g->seq_lane_trig[lane] = 0;
    g->seq_lane_gate[lane] = 0;
    g->seq_lane_tune[lane] = 0;
    for (s = 0; s < CTRL_NSTEPS; s++) {
        g->cfg.seq_note_ofs[lane][s] = 0;
        g->cfg.seq_vel[lane][s] = 100;
        g->cfg.seq_prob[lane][s] = 100;
        g->seq_micro[lane][s] = 0;
    }
}

static inline uint16_t ctrl_seq_lane_song_pat(uint8_t lane)
{
    if (lane == 0u)
        return 0x9249u;
    if (lane == 1u)
        return 0x1010u;
    if (lane == 2u)
        return 0x5555u;
    return 0x0000u;
}

/* 1: every page of the pattern's length is the 16-step pattern v */
static inline int ctrl_seq_pat_is16(const ctrl_t *g, uint8_t lane, uint16_t v)
{
    uint32_t p;

    for (p = 0; p < ctrl_seq_pages(g); p++)
        if (fm_seq_bits_page16(g->cfg.seq_pat[lane], p) != v)
            return 0;
    return 1;
}

static inline void ctrl_seq_reset_lane_song(ctrl_t *g, uint8_t lane)
{
    if (lane >= CTRL_NLANES)
        return;
    fm_seq_bits_tile16(g->cfg.seq_pat[lane], ctrl_seq_lane_song_pat(lane));
    g->cfg.lane_mute[lane] = 0;
    g->lane_solo_mask &= (uint8_t)~(1u << lane);
    ctrl_seq_lane_clear_meta(g, lane);
}

static inline void ctrl_seq_empty_lane(ctrl_t *g, uint8_t lane)
{
    if (lane >= CTRL_NLANES)
        return;
    fm_seq_bits_fill(g->cfg.seq_pat[lane], 0);
    ctrl_seq_lane_clear_meta(g, lane);
}

static inline void ctrl_seq_clear_lane(ctrl_t *g, uint8_t lane)
{
    ctrl_seq_reset_lane_song(g, lane);
}

static inline void ctrl_seq_rotate_lane(ctrl_t *g, uint8_t lane, int32_t dir)
{
    if (lane >= CTRL_NLANES || dir == 0)
        return;
    fm_seq_bits_rot_pages(g->cfg.seq_pat[lane], dir);
    fm_seq_rot_i8_pages(g->cfg.seq_note_ofs[lane], dir);
    fm_seq_rot_u8_pages(g->cfg.seq_vel[lane], dir);
    fm_seq_rot_u8_pages(g->cfg.seq_prob[lane], dir);
    fm_seq_rot_i8_pages(g->seq_micro[lane], dir);
}

static inline uint8_t ctrl_lfo_rate_eff(const ctrl_cfg_t *c, uint32_t slot, uint32_t bpm)
{
    int32_t v = (int32_t)c->lfo_rate[slot] + (int32_t)c->home_macro[1] - 64;

    return fm_lfo_rate_clamp((uint8_t)ctrl_clamp127(v), bpm);
}

static inline int32_t ctrl_lfo_depth_eff(const ctrl_cfg_t *c, uint32_t slot)
{
    int32_t d = (int32_t)c->lfo_depth[slot];

    if (!d)
        return 0;
    return (d * (int32_t)c->home_macro[0]) / 64;
}

static void ctrl_lfo_tick(ctrl_t *g, uint32_t now_ms)
{
    uint32_t dt, i, inc, prev_ph;
    int32_t bpm;

    if (!g->lfo_last_ms)
        g->lfo_last_ms = now_ms;
    dt = now_ms - g->lfo_last_ms;
    g->lfo_last_ms = now_ms;
    if (dt > 50u)
        dt = 1u;
    bpm = (int32_t)g->bpm_eff;
    if (bpm < 40)
        bpm = 40;

    fm_lfo_rates_sanitize(g->cfg.lfo_rate, CTRL_NPARAMS, (uint32_t)bpm);

    for (i = 0; i < CTRL_NPARAMS; i++) {
        uint8_t rate = ctrl_lfo_rate_eff(&g->cfg, i, (uint32_t)bpm);

        if (!ctrl_slot_active(&g->cfg, i) || !g->cfg.lfo_depth[i])
            continue;
        inc = fm_lfo_slot_inc(rate, (uint32_t)bpm, i) * dt;
        prev_ph = g->lfo_ph[i];
        g->lfo_ph[i] += inc;
        g->lfo_ph64[i] += (uint64_t)inc;
        if (g->lfo_ph[i] < prev_ph) {
            g->lfo_cycle[i]++;
            g->lfo_rng[i] = g->lfo_rng[i] * 1664525u + 1013904223u;
            g->lfo_sh[i] = (int8_t)(((g->lfo_rng[i] >> 16) & 127u) - 64);
        }
    }

    for (i = 0; i < CTRL_NPARAMS; i++) {
        int32_t base = (int32_t)g->cfg.base_val[i];
        int32_t mot = 0;
        int32_t wave, delta, depth, v;

        if (!ctrl_slot_active(&g->cfg, i))
            continue;
        if (g->motion_state == CTRL_MOTION_PLAY && g->motion[i][g->motion_bucket] != 255u)
            mot = (int32_t)g->motion[i][g->motion_bucket] - base;
        depth = ctrl_lfo_depth_eff(&g->cfg, i);
        if (!depth) {
            g->eff_val[i] = (uint8_t)ctrl_clamp127(base + mot);
            continue;
        }
        {
            uint32_t ph0 = g->lfo_ph[i];
            uint32_t off = (uint32_t)g->cfg.lfo_phase[i] << 25;
            uint32_t ph = ph0 + off;
            uint32_t cyc = g->lfo_cycle[i];

            if (ph < ph0)
                cyc++;
            wave = fm_lfo_eval_full_at(ph, cyc, i, g->cfg.lfo_shape[i], g->cfg.lfo_skew[i],
                                       g->cfg.lfo_algo[i], g->cfg.lfo_ratio[i], g->cfg.lfo_index[i],
                                       g->cfg.lfo_fb[i], g->cfg.lfo_fold[i]);
            wave = fm_lfo_slew_step(g->lfo_slew_out[i], wave, g->cfg.lfo_smooth[i]);
            g->lfo_slew_out[i] = wave;
        }
        delta = (wave * depth) / 127;
        v = base + delta + mot;
        g->eff_val[i] = (uint8_t)ctrl_clamp127(v);
    }
}

static void ctrl_motion_advance(ctrl_t *g)
{
    if (g->motion_state != CTRL_MOTION_REC && g->motion_state != CTRL_MOTION_PLAY)
        return;
    g->motion_bucket++;
    if (g->motion_bucket >= g->motion_loop) {
        g->motion_bucket = 0;
        if (g->motion_state == CTRL_MOTION_REC)
            g->motion_state = CTRL_MOTION_PLAY;
    }
}

static void ctrl_motion_record(ctrl_t *g, uint8_t slot, uint8_t val)
{
    if (g->motion_state != CTRL_MOTION_REC)
        return;
    g->motion[slot][g->motion_bucket] = val;
}

static void ctrl_midi_cc_send_immediate(ctrl_t *g, uint32_t slot, uint8_t val)
{
    uint8_t ch = ctrl_midi_ch(g);

    midi_send(0xB0u | ch, g->cfg.cc_map[slot], val);
    g->last_sent[slot] = val;
    g->cc_last_ms[slot] = fm1_ms;
}

static void ctrl_midi_cc_pump(ctrl_t *g, uint32_t now_ms)
{
    uint32_t sent = 0, tries, slot;

    for (tries = 0; tries < CTRL_NPARAMS && sent < CTRL_CC_MAX_PER_MS; tries++) {
        slot = (g->cc_rr + tries) % CTRL_NPARAMS;
        if (!ctrl_slot_active(&g->cfg, slot))
            continue;
        if (g->eff_val[slot] == g->last_sent[slot])
            continue;
        if (now_ms - g->cc_last_ms[slot] < CTRL_CC_MIN_INTERVAL_MS)
            continue;
        ctrl_midi_cc_send_immediate(g, slot, g->eff_val[slot]);
        sent++;
        g->cc_rr = (slot + 1u) % CTRL_NPARAMS;
    }
}

static void ctrl_master_pot(ctrl_t *g)
{
    int32_t pos = master_q12;
    int32_t v, hyst = 24;
    uint8_t ch = ctrl_midi_ch(g);

    if (pos < 0)
        pos = 0;
    if (pos > 4096)
        pos = 4096;
    v = (pos * 127) >> 12;
    if (v < 0)
        v = 0;
    if (v > 127)
        v = 127;
    if (g->master_last >= 0 && v > g->master_last - hyst && v < g->master_last + hyst)
        return;
    g->master_last = v;
    if (g->cfg.local_pot == CTRL_LOCAL_MIDI_CC1 || g->cfg.local_pot == CTRL_LOCAL_SYNTH_CC1) {
        if (g->master_sent != v) {
            midi_send(0xB0u | ch, 1u, (uint8_t)v);
            g->master_sent = v;
        }
    } else if (g->cfg.local_pot == CTRL_LOCAL_MIDI_EXP) {
        if (g->master_sent != v) {
            midi_send(0xB0u | ch, 11u, (uint8_t)v);
            g->master_sent = v;
        }
    }
}

static uint32_t ctrl_clock_poll(ctrl_t *g, uint32_t now_ms)
{
    uint32_t raw_n, i, out_ticks = 0;
    uint32_t swing_delay = ((uint32_t)g->cfg.swing * 3u + 63u) / 127u;

    if (g->ext_clock && now_ms - g->last_f8_ms > 1500u)
        g->ext_clock = 0;

    if (!g->ext_clock)
        g->bpm_eff = (uint16_t)(60u + g->cfg.bpm);

    if (g->playing) {
        uint32_t dt = now_ms - g->int_last_ms;

        g->int_last_ms = now_ms;
        if (!g->ext_clock) {
            uint32_t tick_ms, bpm24 = (uint32_t)g->bpm_eff * 24u;

            g->int_rem_ms += dt;
            if (bpm24 == 0u)
                bpm24 = 1u;
            tick_ms = 60000u / bpm24;
            if (tick_ms == 0u)
                tick_ms = 1u;
            while (g->int_rem_ms >= tick_ms) {
                g->step_ticks++;
                g->int_rem_ms -= tick_ms;
                midi_send(0xF8, 0, 0);
            }
        }
    } else {
        g->int_last_ms = now_ms;
        g->step_ticks = 0;
        return 0;
    }

    raw_n = g->step_ticks;
    g->step_ticks = 0;
    for (i = 0; i < raw_n; i++) {
        uint32_t pos = g->raw_tick % 12u;

        g->raw_tick++;
        if (swing_delay > 0u && pos >= 6u && pos < 6u + swing_delay)
            g->swing_held++;
        else if (pos == 6u + swing_delay && g->swing_held > 0u) {
            out_ticks += 1u + g->swing_held;
            g->swing_held = 0;
        } else
            out_ticks++;
    }
    return out_ticks;
}

static void ctrl_transport_reset(ctrl_t *g)
{
    uint32_t lane;

    for (lane = 0; lane < CTRL_NLANES; lane++)
        ctrl_seq_lane_note_off(g, lane);
    g->tick = 0;
    g->step = 0;
    g->bar = 0;
    g->raw_tick = 0;
    g->motion_bucket = 0;
    g->motion_loop = (uint32_t)(1u + g->cfg.motion_bars) * 96u;
    if (g->motion_loop > CTRL_MOTION_BUCKETS)
        g->motion_loop = CTRL_MOTION_BUCKETS;
}

static void ctrl_seq_lane_note_off(ctrl_t *g, uint32_t lane)
{
    uint8_t ch = ctrl_midi_ch(g);
    uint8_t n;

    if (lane >= CTRL_NLANES || !g->seq_active_note[lane])
        return;
    n = g->seq_active_note[lane];
    midi_send(0x80u | ch, n, 0);
    if (ctrl_local_on(g))
        synth_note_off(n);
    g->seq_active_note[lane] = 0;
}

static inline void ctrl_sync_heard_mutes(ctrl_t *g)
{
    uint8_t prev = g->heard_mute_mask;
    uint8_t now = 0u;
    uint32_t i;

    for (i = 0; i < CTRL_NLANES; i++) {
        if (g->cfg.lane_mute[i])
            now |= (uint8_t)(1u << i);
    }
    g->heard_mute_mask = now;
    for (i = 0; i < CTRL_NLANES; i++) {
        if ((now >> i) & 1u && !((prev >> i) & 1u))
            ctrl_seq_lane_note_off(g, i);
    }
}

static inline void ctrl_maybe_sync_heard_mutes(ctrl_t *g)
{
    if (!g->playing)
        ctrl_sync_heard_mutes(g);
}

static void ctrl_seq_notes_off(ctrl_t *g)
{
    uint32_t lane;

    for (lane = 0; lane < CTRL_NLANES; lane++)
        ctrl_seq_lane_note_off(g, lane);
}

static int ctrl_seq_step_on(ctrl_t *g, uint32_t lane, uint8_t st)
{
    int on;
    uint8_t cond;

    if ((g->heard_mute_mask >> lane) & 1u)
        return 0;
    on = fm_seq_bit(g->cfg.seq_pat[lane], st);
    on = fm_seq_vary_hit(on, g->seq_vary[lane], st, g->bar, (uint8_t)lane, g->rng);
    if (on && !fm_seq_trig_pass(g->seq_lane_trig[lane], st, g->bar, (uint8_t)lane))
        on = 0;
    if (!on)
        return 0;
    cond = g->cfg.seq_prob[lane][st];
    if (cond >= FM_SEQ_NCONDS)
        cond = 0u;
    return fm_seq_cond_pass(cond, st, g->bar, (uint8_t)lane, g->roll_mask != 0u);
}

static void ctrl_seq_gate_off(ctrl_t *g)
{
    uint32_t lane;

    for (lane = 0; lane < CTRL_NLANES; lane++) {
        if (g->seq_active_note[lane] && (int32_t)(g->tick - g->seq_off_tick[lane]) >= 0)
            ctrl_seq_lane_note_off(g, lane);
    }
}

static void ctrl_arp_notes_off(ctrl_t *g)
{
    uint32_t i;
    uint8_t ch = ctrl_midi_ch(g);

    for (i = 0; i < g->arp_active_count; i++) {
        uint8_t n = g->arp_active_notes[i];
        midi_send(0x80u | ch, n, 0);
        if (ctrl_local_on(g))
            synth_note_off(n);
    }
    g->arp_active_count = 0;
}

static void ctrl_seq_tick(ctrl_t *g)
{
    uint32_t lane;
    uint8_t ch = ctrl_midi_ch(g);
    uint8_t st = g->step;
    uint32_t div = FM_SEQ_PACE4_TICKS[g->cfg.seq_pace % FM_SEQ_PACE_N];
    uint8_t tie_mask = 0u;

    if (!g->playing || fm_seq_is_mute_quant_step(st))
        ctrl_sync_heard_mutes(g);
    for (lane = 0; lane < CTRL_NLANES; lane++) {
        int on = ctrl_seq_step_on(g, lane, st);
        uint8_t note;
        int32_t note_i;

        if (!g->seq_active_note[lane])
            continue;
        /* Same note while the previous gate still covers this step → tie (no off, no re-trigger). */
        if (on && (int32_t)(g->seq_off_tick[lane] - g->tick) >= 0) {
            note_i = (int32_t)ctrl_scale_note((uint8_t)(48u + g->cfg.root + lane * 7u), g->cfg.scale,
                                              (int32_t)(st & 7u), 0)
                   + (int32_t)g->cfg.seq_note_ofs[lane][st]
                   + (int32_t)fm_seq_rand_pitch(g->seq_lane_tune[lane], st, g->bar, (uint8_t)lane, g->rng);
            note = (uint8_t)ctrl_clamp127(note_i);
            if (g->seq_active_note[lane] == note) {
                tie_mask |= (uint8_t)(1u << lane);
                continue;
            }
        }
        if (on)
            ctrl_seq_lane_note_off(g, lane);
    }
    for (lane = 0; lane < CTRL_NLANES; lane++) {
        uint8_t vel;
        int32_t note_i;
        int on;
        int32_t gate;

        on = ctrl_seq_step_on(g, lane, st);
        if (!on)
            continue;
        vel = g->cfg.seq_vel[lane][st];
        {
            int32_t vv = (int32_t)vel +
                         (int32_t)fm_seq_rand_vel(g->seq_lane_vel[lane], st, g->bar, (uint8_t)lane, g->rng);
            if (vv < 1)
                vv = 1;
            if (vv > 127)
                vv = 127;
            vel = (uint8_t)vv;
        }
        if (g->cfg.home_macro[2] > 0u)
            vel = (uint8_t)ctrl_clamp127((int32_t)vel * (int32_t)g->cfg.home_macro[2] / 100);
        note_i = (int32_t)ctrl_scale_note((uint8_t)(48u + g->cfg.root + lane * 7u), g->cfg.scale,
                                          (int32_t)(st & 7u), 0)
               + (int32_t)g->cfg.seq_note_ofs[lane][st]
               + (int32_t)fm_seq_rand_pitch(g->seq_lane_tune[lane], st, g->bar, (uint8_t)lane, g->rng);
        gate = 256 + (int32_t)fm_seq_rand_gate(g->seq_lane_gate[lane], st, g->bar, (uint8_t)lane, g->rng) * 4;
        if (gate < 32)
            gate = 32;
        if (gate > 512)
            gate = 512;
        {
            uint32_t len = ((uint32_t)gate * div) >> 8;

            g->seq_off_tick[lane] = g->tick + (len ? len : 1u);
        }
        if ((tie_mask >> lane) & 1u)
            continue;
        {
            uint8_t note = (uint8_t)ctrl_clamp127(note_i);
            midi_send(0x90u | ch, note, vel);
            g->seq_active_note[lane] = note;
            if (ctrl_local_on(g)) {
                static uint32_t cue_last_note;
                synth_note_on_ex(lane, note, vel, 0, (uint32_t)gate, &g->cue_cfg, &cue_last_note);
            }
        }
    }
    g->step = (uint8_t)((g->step + 1u) % ctrl_seq_len(g));
    if ((g->step & 15u) == 0u)
        g->bar = (uint8_t)((g->bar + 1u) & 15u);
}

static void ctrl_arp_tick(ctrl_t *g)
{
    uint8_t notes[8];
    uint32_t n, i;
    uint8_t base_root = g->held_root ? g->held_root : (uint8_t)(60u + g->cfg.root);
    uint8_t root = ctrl_scale_note(base_root, g->cfg.scale, (int32_t)g->arp_ix, 0);
    uint8_t ch = ctrl_midi_ch(g);
    uint8_t mode = g->cfg.arp_mode;

    ctrl_arp_notes_off(g);
    if (mode == 0u)
        return;
    ctrl_chord_notes(g, root, notes, &n);
    if (!n)
        return;
    if (n > 5u)
        n = 5u;
    for (i = 0; i < n; i++) {
        uint8_t vel = 96;

        if (g->cfg.vel_mode == 3u)
            vel = (uint8_t)((master_q12 * 127) >> 12);
        if (vel < 24u)
            vel = 24u;
        midi_send(0x90u | ch, notes[i], vel);
        g->arp_active_notes[i] = notes[i];
        if (ctrl_local_on(g)) {
            static uint32_t cue_last_note;
            synth_note_on_ex(i & 3u, notes[i], vel, 0, 256, &g->cue_cfg, &cue_last_note);
        }
    }
    g->arp_active_count = (uint8_t)n;
    if (mode == 1u)
        g->arp_ix = (uint8_t)((g->arp_ix + 1u) & 7u);
    else if (mode == 2u)
        g->arp_ix = g->arp_ix > 0u ? (uint8_t)(g->arp_ix - 1u) : 7u;
    else if (mode == 3u) {
        if (g->arp_dir)
            g->arp_ix++;
        else if (g->arp_ix > 0u)
            g->arp_ix--;
        if (g->arp_ix >= 7u) {
            g->arp_ix = 7u;
            g->arp_dir = 0;
        }
        if (g->arp_ix == 0u)
            g->arp_dir = 1;
    } else
        g->arp_ix = (uint8_t)(ctrl_rng_next(g) & 7u);
}

static void ctrl_engine_tick(ctrl_t *g, uint32_t now_ms)
{
    uint32_t t;

    ctrl_lfo_tick(g, now_ms);
    t = ctrl_clock_poll(g, now_ms);
    while (t--) {
        ctrl_motion_advance(g);
        {
            uint32_t div = FM_SEQ_PACE4_TICKS[g->cfg.seq_pace % FM_SEQ_PACE_N];

            if (div && (g->tick % div) == 0u)
                ctrl_seq_tick(g);
            ctrl_seq_gate_off(g);
        }
        if ((g->tick % (24u >> (g->cfg.arp_rate & 3u))) == 0u)
            ctrl_arp_tick(g);
        g->tick++;
        g->home_radar[g->bar] = g->eff_val[g->step % CTRL_NPARAMS];
    }
    ctrl_midi_cc_pump(g, now_ms);
    ctrl_master_pot(g);
}

static void ctrl_sync_cue_cfg(ctrl_t *g)
{
    midi_cfg_t *c = &g->cue_cfg;

    c->base_val[P_TIMBRE] = g->eff_val[0];
    c->base_val[P_RATIO] = g->eff_val[1];
    c->base_val[P_ATTACK] = g->eff_val[2];
    c->base_val[P_DECAY] = g->eff_val[3];
    c->base_val[P_ECHO_MIX] = g->eff_val[12];
    c->base_val[P_ECHO_TIME] = g->eff_val[13];
    c->base_val[P_DRIVE] = (uint8_t)ctrl_clamp127((int32_t)g->eff_val[14] + (int32_t)g->cfg.home_macro[3] - 32);
    c->val[P_TIMBRE] = c->base_val[P_TIMBRE];
    c->val[P_RATIO] = c->base_val[P_RATIO];
    c->val[P_ATTACK] = c->base_val[P_ATTACK];
    c->val[P_DECAY] = c->base_val[P_DECAY];
    c->val[P_ECHO_MIX] = c->base_val[P_ECHO_MIX];
    c->val[P_ECHO_TIME] = c->base_val[P_ECHO_TIME];
    c->val[P_DRIVE] = c->base_val[P_DRIVE];
}

static void ctrl_cue_render(ctrl_t *g, int32_t *out, uint32_t n)
{
    if (!ctrl_local_on(g)) {
        uint32_t i;
        for (i = 0; i < n * 2u; i++)
            out[i] = 0;
        return;
    }
    ctrl_sync_cue_cfg(g);
    synth_render(out, n, &g->cue_cfg);
}

static void ctrl_init_runtime(ctrl_t *g)
{
    uint32_t i;

    ctrl_cfg_defaults(&g->cfg);
    ctrl_profile_sync(g);
    g->view = CTRL_VIEW_BANK;
    g->bank = CTRL_BANK_A;
    g->rng = 0xC071u;
    g->bpm_eff = 120;
    g->master_last = -1;
    g->master_sent = -1;
    g->motion_loop = 96u;
    g->arp_dir = 1;
    for (i = 0; i < CTRL_NPARAMS; i++) {
        g->eff_val[i] = g->cfg.base_val[i];
        g->last_sent[i] = g->cfg.base_val[i];
        g->lfo_rng[i] = 0x12345678u + i * 0x9e3779b9u;
        g->lfo_ph64[i] = (uint64_t)g->lfo_ph[i];
        g->lfo_slew_out[i] = 0;
    }
    for (i = 0; i < CTRL_NPARAMS; i++) {
        uint32_t b;
        for (b = 0; b < CTRL_MOTION_BUCKETS; b++)
            g->motion[i][b] = 255u;
    }
    midi_cfg_defaults(&g->cue_cfg);
    midi_state_init(&g->midi, &g->cue_cfg);
    ctrl_transport_reset(g);
    g->seq_plock_step = 255u;
    g->seq_plock_knob = 0u;
    g->seq_view_page = 0u;
    g->edit_used = 0u;
    g->edit_down = 0u;
    g->edit_char_ix = 0u;
    g->tag_ix = 0u;
    g->lfo_key_page = 0u;
    for (i = 0; i < CTRL_NLANES; i++) {
        uint32_t s;
        g->seq_vary[i] = 0;
        g->seq_lane_vel[i] = 0;
        g->seq_lane_trig[i] = 0;
        g->seq_lane_gate[i] = 0;
        g->seq_lane_tune[i] = 0;
        g->bk_last_ms[i] = 0;
        for (s = 0; s < CTRL_NSTEPS; s++)
            g->seq_micro[i][s] = 0;
    }
    g->lane_solo_mask = 0u;
    g->ui_dirty = 1u;
}

static inline int ctrl_note_in_scale(uint8_t root_pc, uint8_t scale, uint8_t note)
{
    uint8_t sc = (uint8_t)(scale % CTRL_NSCALES);
    uint8_t rel = (uint8_t)((note + 120u - (root_pc % 12u)) % 12u);
    uint32_t d;

    for (d = 0; d < 8u; d++) {
        if (((uint8_t)CTRL_SCALE_DEG[sc][d] % 12u) == rel)
            return 1;
    }
    return 0;
}

static uint8_t ctrl_quantize_to_scale(uint8_t root_pc, uint8_t scale, uint8_t raw_note)
{
    int32_t d;

    if (ctrl_note_in_scale(root_pc, scale, raw_note))
        return raw_note;
    for (d = 1; d <= 2; d++) {
        if (raw_note >= (uint8_t)d && ctrl_note_in_scale(root_pc, scale, (uint8_t)(raw_note - d)))
            return (uint8_t)(raw_note - d);
        if (raw_note + d <= 127 && ctrl_note_in_scale(root_pc, scale, (uint8_t)(raw_note + d)))
            return (uint8_t)(raw_note + d);
    }
    return raw_note;
}

static void ctrl_note_on(ctrl_t *g, uint8_t key, uint8_t vel)
{
    int32_t base_f3 = 53 + g->oct_off * 12;
    uint8_t raw_note = (uint8_t)ctrl_clamp127(base_f3 + (int32_t)key);
    uint8_t note = ctrl_quantize_to_scale(g->cfg.root, g->cfg.scale, raw_note);
    uint8_t ch = ctrl_midi_ch(g);
    uint8_t notes[8];
    uint32_t n, i;

    if (key >= 27u)
        return;
    g->held_root = note;
    ctrl_chord_notes(g, note, notes, &n);
    if (n > 5u)
        n = 5u;
    g->held_count[key] = (uint8_t)n;
    for (i = 0; i < n; i++) {
        uint8_t qn = ctrl_quantize_to_scale(g->cfg.root, g->cfg.scale, notes[i]);
        g->held_notes[key][i] = qn;
        midi_send(0x90u | ch, qn, vel);
        if (ctrl_local_on(g)) {
            static uint32_t cue_last_note;
            synth_note_on_ex(i & 3u, qn, vel, 0, 256, &g->cue_cfg, &cue_last_note);
        }
    }
}

static void ctrl_note_off_key(ctrl_t *g, uint8_t key)
{
    uint8_t ch = ctrl_midi_ch(g);
    uint32_t i, n;

    if (key >= 27u)
        return;
    n = g->held_count[key];
    g->held_count[key] = 0;
    for (i = 0; i < n; i++) {
        uint8_t qn = g->held_notes[key][i];
        midi_send(0x80u | ch, qn, 0);
        if (ctrl_local_on(g))
            synth_note_off(qn);
    }
}

static void ctrl_handle_midi_rx(ctrl_t *g, uint8_t st, uint8_t d1, uint8_t d2, uint32_t now_ms)
{
    uint8_t ch = ctrl_midi_ch(g);

    if (st == 0xF8u) {
        g->ext_clock = 1;
        g->last_f8_ms = now_ms;
        g->step_ticks++;
        return;
    }
    if (st == 0xFAu) {
        g->playing = 1;
        g->ext_clock = 1;
        ctrl_transport_reset(g);
        return;
    }
    if (st == 0xFCu) {
        g->playing = 0;
        ctrl_seq_notes_off(g);
        ctrl_arp_notes_off(g);
        return;
    }
    if ((st & 0xF0u) == 0xB0u && (st & 0x0Fu) == ch) {
        uint32_t i;
        if (g->learn_active) {
            g->cfg.cc_map[g->learn_slot % CTRL_NPARAMS] = d1;
            g->set_dirty = 1u;
            g->set_t = now_ms;
            g->ui_dirty = 1u;
            return;
        }
        for (i = 0; i < CTRL_NPARAMS; i++) {
            if (!ctrl_slot_active(&g->cfg, i))
                continue;
            if (g->cfg.cc_map[i] == d1) {
                g->cfg.base_val[i] = d2;
                g->eff_val[i] = d2;
                g->last_sent[i] = d2;
                g->set_dirty = 1u;
                g->set_t = now_ms;
                g->ui_dirty = 1u;
                return;
            }
        }
    }
}

static void ctrl_set_param(ctrl_t *g, uint32_t slot, uint8_t val)
{
    if (slot >= CTRL_NPARAMS)
        return;
    g->cfg.base_val[slot] = val;
    g->eff_val[slot] = val;
    ctrl_motion_record(g, (uint8_t)slot, val);
    ctrl_midi_cc_send_immediate(g, slot, val);
    g->set_dirty = 1u;
    g->set_t = fm1_ms;
    g->ui_dirty = 1u;
}

#endif /* CTRL_CORE_H */
