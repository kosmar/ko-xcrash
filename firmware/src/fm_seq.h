/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared step sequencer: Euclid, conditions, vary, canvas grid (integer only, no float). */
#ifndef FM_SEQ_H
#define FM_SEQ_H

#include <stdint.h>

#define FM_SEQ_PAGE_STEPS 16u                                /* one page: the 16 white keys, the grid on screen */
#define FM_SEQ_MAX_PAGES 8u
#define FM_SEQ_STEPS (FM_SEQ_PAGE_STEPS * FM_SEQ_MAX_PAGES) /* the longest pattern: 128 */
#define FM_SEQ_MASK_BYTES (FM_SEQ_STEPS / 8u)                /* a step bit set: uint8_t[FM_SEQ_MASK_BYTES] */
#define FM_SEQ_MUTE_QUANT 4u
#define FM_SEQ_PLOCK_NONE 255u

static inline int fm_seq_is_mute_quant_step(uint8_t step)
{
    return (step & (FM_SEQ_MUTE_QUANT - 1u)) == 0u;
}
#define FM_SEQ_NPAGES 3u
/* SEQ knob-page tap cycle (g.seq_page indices); slot 3 unused (legacy).
 * Page 5 (KEY/SCALE/SWING/TEMPO) is not in the cycle: those live on GLO; song setup still opens it. */
#define FM_SEQ_KNOB_PAGE_CYCLE_N 4u
static const uint8_t FM_SEQ_KNOB_PAGE_CYCLE[FM_SEQ_KNOB_PAGE_CYCLE_N] = {0u, 1u, 4u, 2u};
#define FM_SEQ_NVARY 5u
#define FM_SEQ_NCONDS 76u
#define FM_SEQ_COND_MAX (FM_SEQ_NCONDS - 1u)
#define FM_SEQ_COND_NEW_BASE 22u
#define FM_SEQ_COND_NEW_NR 21u
#define FM_SEQ_COND_NOT_BASE 43u
#define FM_SEQ_COND_NOT_NR 33u
#define FM_SEQ_COND_NOT_RATIO_SKIP 2u /* skip 1:2 and 2:2 (dup of positive COND) */
#define FM_SEQ_NVIEW_CYC 9u
#define FM_SEQ_VIEW_CYC_OFF 0u
#define FM_SEQ_VIEW_CYC_ALL 1u
#define FM_SEQ_VIEW_CYC_ONLY 8u
/* Integer timing grid: FM_SEQ_TIMING_UNITS = one 16th step; micro is ±half step, nudge is ±one step. */
#define FM_SEQ_TIMING_UNITS 50
#define FM_SEQ_MICRO_HALF (FM_SEQ_TIMING_UNITS / 2)
#define FM_SEQ_MICRO_MIN (-FM_SEQ_MICRO_HALF)
#define FM_SEQ_MICRO_MAX FM_SEQ_MICRO_HALF
#define FM_SEQ_LANE_NUDGE_MIN (-FM_SEQ_TIMING_UNITS)
#define FM_SEQ_LANE_NUDGE_MAX FM_SEQ_TIMING_UNITS
#define FM_SEQ_TIMING_SUM_MAX (FM_SEQ_TIMING_UNITS + FM_SEQ_TIMING_UNITS + FM_SEQ_MICRO_HALF)
#define FM_SEQ_TIMING_SUM_MIN (-FM_SEQ_TIMING_SUM_MAX)

static const char *const FM_SEQ_VIEW_CYC_NAMES[FM_SEQ_NVIEW_CYC] = {
    "OFF", "ALL", "1:2", "2:2", "1:4", "2:4", "3:4", "4:4", "ONLY"
};

enum {
    FM_SEQ_STATE_PLAY = 0,
    FM_SEQ_STATE_MUTE = 1,
    FM_SEQ_STATE_SOLO = 2,
    FM_SEQ_STATE_INV = 3
};

/* Readout bands for fm_seq_vary_tier: 0, then quartiles of 1..127. */
static const char *const FM_SEQ_VARY_NAMES[FM_SEQ_NVARY] = {
    "LOCK", "LOW", "MID", "HIGH", "MAX"};
static const char *const FM_SEQ_COND_NAMES[FM_SEQ_NCONDS] = {
    "100%", "90%", "75%", "50%", "25%", "10%", "1:2", "2:2", "1:4", "2:4", "3:4", "4:4",
    "1:8", "2:8", "3:8", "4:8", "5:8", "6:8", "7:8", "8:8", "FILL", "!FIL",
    "1:3", "2:3", "3:3", "1:5", "2:5", "3:5", "4:5", "5:5", "1:6", "2:6", "3:6", "4:6", "5:6",
    "6:6", "1:7", "2:7", "3:7", "4:7", "5:7", "6:7", "7:7",
    "!8:8", "!7:8", "!6:8", "!5:8", "!4:8", "!3:8", "!2:8", "!1:8", "!7:7", "!6:7", "!5:7",
    "!4:7", "!3:7", "!2:7", "!1:7", "!6:6", "!5:6", "!4:6", "!3:6", "!2:6", "!1:6", "!5:5",
    "!4:5", "!3:5", "!2:5", "!1:5", "!4:4", "!3:4", "!2:4", "!1:4", "!3:3", "!2:3", "!1:3"};
static const uint8_t FM_SEQ_COND_NEW_A[FM_SEQ_COND_NEW_NR] = {
    1u, 2u, 3u, 1u, 2u, 3u, 4u, 5u, 1u, 2u, 3u, 4u, 5u, 6u, 1u, 2u, 3u, 4u, 5u, 6u, 7u};
static const uint8_t FM_SEQ_COND_NEW_B[FM_SEQ_COND_NEW_NR] = {
    3u, 3u, 3u, 5u, 5u, 5u, 5u, 5u, 6u, 6u, 6u, 6u, 6u, 6u, 7u, 7u, 7u, 7u, 7u, 7u, 7u};
#define FM_SEQ_PACE_N 5u
static const char *const FM_SEQ_PACE4_NAMES[FM_SEQ_PACE_N] = {"1/32", "1/16", "1/8", "1/4", "1/2"};
static const uint8_t FM_SEQ_PACE4_TICKS[FM_SEQ_PACE_N] = {3u, 6u, 12u, 24u, 48u};
static const char *const FM_SEQ_PACE7_NAMES[7] = {"1/16", "1/8", "1/4", "1/2B", "1Bar", "2Bar", "4Bar"};
static const uint32_t FM_SEQ_PACE7_TICKS[7] = {6u, 12u, 24u, 48u, 96u, 192u, 384u};
static const uint8_t FM_SEQ_STEPS8_TBL[8] = {0u, 2u, 3u, 4u, 6u, 8u, 16u, 32u};

/* Sequencer clock ratio vs 1/1 grid: lane rate and (DECK) global master 16th. */
#define FM_SEQ_CLK_RATIO_N 7u
#define FM_SEQ_CLK_RATIO_ONE 3u /* "1" */
static const char *const FM_SEQ_CLK_RATIO_NAMES[FM_SEQ_CLK_RATIO_N] = {
    "/8", "/4", "/2", "1", "x2", "x4", "x8"};
static const uint8_t FM_SEQ_CLK_RATIO_NUM[FM_SEQ_CLK_RATIO_N] = {1u, 1u, 1u, 1u, 2u, 4u, 8u};
static const uint8_t FM_SEQ_CLK_RATIO_DEN[FM_SEQ_CLK_RATIO_N] = {8u, 4u, 2u, 1u, 1u, 1u, 1u};

static inline uint8_t fm_seq_clk_ratio_clamp(uint8_t ix)
{
    if (ix >= FM_SEQ_CLK_RATIO_N)
        return FM_SEQ_CLK_RATIO_ONE;
    return ix;
}

/* Legacy DECK lane div 0..2 (1/2, 1, x2) → expanded table index. */
static inline uint8_t fm_seq_clk_ratio_from_legacy(uint8_t v)
{
    if (v <= 2u)
        return (uint8_t)(v + 2u);
    return fm_seq_clk_ratio_clamp(v);
}

/* Per-lane stored div (full index after deck_seq_migrate_lane_div_bank). */
static inline uint8_t fm_seq_clk_ratio_lane_ix(uint8_t stored)
{
    return fm_seq_clk_ratio_clamp(stored);
}

static inline const char *fm_seq_clk_ratio_name(uint8_t ix)
{
    return FM_SEQ_CLK_RATIO_NAMES[fm_seq_clk_ratio_clamp(ix)];
}

static inline uint8_t fm_seq_clk_ratio_nsteps(uint8_t *acc, uint8_t ratio_ix)
{
    uint8_t num;
    uint8_t den;
    uint8_t a;
    uint8_t n = 0u;

    if (!acc)
        return 0u;
    ratio_ix = fm_seq_clk_ratio_clamp(ratio_ix);
    num = FM_SEQ_CLK_RATIO_NUM[ratio_ix];
    den = FM_SEQ_CLK_RATIO_DEN[ratio_ix];
    a = (uint8_t)(*acc + num);
    while (a >= den) {
        a = (uint8_t)(a - den);
        n++;
    }
    *acc = a;
    return n;
}

static inline uint32_t fm_seq_clk_ratio_scale_len(uint32_t len, uint8_t ratio_ix)
{
    uint8_t num;
    uint8_t den;

    ratio_ix = fm_seq_clk_ratio_clamp(ratio_ix);
    num = FM_SEQ_CLK_RATIO_NUM[ratio_ix];
    den = FM_SEQ_CLK_RATIO_DEN[ratio_ix];
    if (!len || !num)
        return len;
    return (len * (uint32_t)den + (uint32_t)num - 1u) / (uint32_t)num;
}

static const char *const FM_SEQ_KNOB_P0[4] = {"LANE", "PATTRN", "ROTATE", "VARY"};
static const char *const FM_SEQ_KNOB_P1[4] = {"TRIG", "\xB1VEL", "\xB1GATE", "\xB1PITCH"};

/* PROB page knob slot -> param index (0 VEL, 1 TRIG, 2 GATE, 3 PITCH; LFO/DJ/save order). Self-inverse. */
static inline uint8_t fm_seq_p1_fn(uint8_t slot)
{
    slot &= 3u;
    return slot < 2u ? (uint8_t)(slot ^ 1u) : slot;
}
static const char *const FM_SEQ_KNOB_P2[4] = {"LENGTH", "STATE", "NUDGE", ""};
#if defined(GENRE_HEAVY_PSYCH)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "BAND", "INVERT"};
#elif defined(GENRE_HOUSE)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "CREW", "INVERT"};
#elif defined(GENRE_FUNK)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "UNIT", "INVERT"};
#elif defined(GENRE_POP)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "TRK", "INVERT"};
#elif defined(GENRE_HIPHOP)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "BLOCK", "INVERT"};
#elif defined(GENRE_JAZZ)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "COMBO", "INVERT"};
#elif defined(GENRE_IDM)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "PACK", "INVERT"};
#elif defined(GENRE_SNOWCRASH)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "DECK", "INVERT"};
#elif defined(GENRE_TECHNO)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "RIG", "INVERT"};
#elif defined(GENRE_TRIPHOP)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "MOOD", "INVERT"};
#elif defined(GENRE_SOUL)
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "REVUE", "INVERT"};
#else
static const char *const FM_SEQ_KNOB_P4[4] = {"CYCLE", "REPEAT", "SYSTEM", "INVERT"};
#endif
static const char *const FM_SEQ_KNOB_PLOCK[4] = {"VEL", "COND", "MICRO", "PITCH"};
static const char *const FM_SEQ_KNOB_MELODY[4] = {"STEPS", "PACE", "PROG", "MUTATE"};
static const char *const FM_SEQ_STATE_NAMES[4] = {"PLAY", "MUTE", "SOLO", "INVERT"};
static const char *const FM_SEQ_P3_STATE_NAMES[3] = {"MUTE", "PLAY", "SOLO"};

#define FM_SEQ_EUCLID_SONG 0u
#define FM_SEQ_EUCLID_EDITED 253u
#define FM_SEQ_EUCLID_FRESH 254u
#define FM_SEQ_EUCLID_EMPTY 255u
#define FM_SEQ_PATTERN_ORIGIN_NONE 252u
#define FM_SEQ_EUCLID_CYCLE_N 20u /* EMPTY, SONG, FRESH, EUC 1..16, EDITED */

static inline void fm_seq_lane_pattern_mark_edited(uint8_t *lane_euclid_hits, uint8_t *pattern_origin)
{
    uint8_t prev = *lane_euclid_hits;

    if (pattern_origin && prev != FM_SEQ_EUCLID_EDITED)
        *pattern_origin = prev;
    *lane_euclid_hits = FM_SEQ_EUCLID_EDITED;
}

#define FM_SEQ_NENSEMBLES 18u
#define FM_SEQ_ENS_CUSTOM 255u

enum {
    /* Knob 0 → 127. Index i mirrors N-1-i. */
    FM_SEQ_ENS_MUTE = 0,   /* TACET:  Full silence */
    FM_SEQ_ENS_KONLY = 1,  /* K ONLY: flip of NO KCK */
    FM_SEQ_ENS_CH_SRN = 2, /* CH+SRN: flip of RIDDIM */
    FM_SEQ_ENS_BASS = 3,   /* BASS+:  flip of DRUMS */
    FM_SEQ_ENS_RSCH = 4,   /* R+SCH:  flip of STEPPA */
    FM_SEQ_ENS_RHSC = 5,   /* RH+SC:  flip of K+BASS */
    FM_SEQ_ENS_KBSR = 6,   /* K+BSR:  flip of SKANK */
    FM_SEQ_ENS_KRMB = 7,   /* K+RMB:  flip of CHORDS */
    FM_SEQ_ENS_KBCH = 8,   /* K+BCH:  flip of TOPS */
    FM_SEQ_ENS_TOPS = 9,   /* TOPS:   Percussion & highs, no Kick/Bass */
    FM_SEQ_ENS_CHORDS = 10,/* CHORDS: Chord + Siren + Hat */
    FM_SEQ_ENS_SKANK = 11, /* SKANK:  Rim + Hat + Chord */
    FM_SEQ_ENS_KBASS = 12, /* K+BASS: Kick + Bass only */
    FM_SEQ_ENS_STEPPA = 13,/* STEPPA: Kick + Hat + Bass */
    FM_SEQ_ENS_DRUMS = 14, /* DRUMS:  Full drum kit only */
    FM_SEQ_ENS_RIDDIM = 15,/* RIDDIM: Drums + Bass, no Chords/Siren */
    FM_SEQ_ENS_NO_KCK = 16,/* NO KCK: Full band minus Kick */
    FM_SEQ_ENS_ALL = 17    /* TUTTI:  Full band */
};

static const char *const FM_SEQ_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "SKANK",
    "DEEP",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
static const char *const FM_SEQ_FLIP_NAMES[2] = {"HEADS", "TAILS"};

static inline uint8_t fm_seq_ens_bar_preset(uint8_t ens)
{
    if (ens >= FM_SEQ_NENSEMBLES)
        return 0u;
    return (uint8_t)(((uint32_t)ens * 127u) / (FM_SEQ_NENSEMBLES - 1u));
}

static inline uint8_t fm_seq_ens_bar_custom(uint8_t active, uint8_t total)
{
    if (total == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)active * 127u) / (uint32_t)total);
}

static inline uint8_t fm_seq_euclid_to_idx(uint8_t eh)
{
    if (eh == FM_SEQ_EUCLID_EMPTY)
        return 0u;
    if (eh == FM_SEQ_EUCLID_SONG)
        return 1u;
    if (eh == FM_SEQ_EUCLID_FRESH)
        return 2u;
    if (eh == FM_SEQ_EUCLID_EDITED)
        return 19u;
    return (uint8_t)((eh > 16u ? 16u : eh) + 2u);
}

static inline uint8_t fm_seq_euclid_from_idx(int32_t idx)
{
    if (idx <= 0)
        return FM_SEQ_EUCLID_EMPTY;
    if (idx == 1)
        return FM_SEQ_EUCLID_SONG;
    if (idx == 2)
        return FM_SEQ_EUCLID_FRESH;
    if (idx >= 19)
        return FM_SEQ_EUCLID_EDITED;
    return (uint8_t)(idx - 2);
}

static inline int fm_seq_euclid_is_pattern(uint8_t eh)
{
    return eh >= 1u && eh <= 16u;
}

static inline uint8_t fm_seq_euclid_step_once(uint8_t eh, int32_t steps, uint8_t pattern_origin)
{
    int32_t idx;

    if (eh == FM_SEQ_EUCLID_EDITED && steps > 0 && fm_seq_euclid_is_pattern(pattern_origin))
        return pattern_origin;
    idx = (int32_t)fm_seq_euclid_to_idx(eh) + (steps > 0 ? 1 : -1);
    if (idx < 0)
        idx = 0;
    if (idx >= (int32_t)FM_SEQ_EUCLID_CYCLE_N)
        idx = (int32_t)FM_SEQ_EUCLID_CYCLE_N - 1;
    return fm_seq_euclid_from_idx(idx);
}

static inline uint8_t fm_seq_euclid_step(uint8_t eh, int32_t steps, int edit, uint8_t pattern_origin,
                                        int song_avail)
{
    uint8_t nh;
    int guard;

    if (edit)
        return song_avail ? FM_SEQ_EUCLID_SONG : eh;
    nh = fm_seq_euclid_step_once(eh, steps, pattern_origin);
    if (song_avail || nh != FM_SEQ_EUCLID_SONG)
        return nh;
    for (guard = 0; guard < 20 && nh == FM_SEQ_EUCLID_SONG; guard++)
        nh = fm_seq_euclid_step_once(nh, steps, FM_SEQ_PATTERN_ORIGIN_NONE);
    return nh;
}

static inline uint8_t fm_seq_euclid_bar_hits(uint8_t eh)
{
    if (eh == FM_SEQ_EUCLID_EMPTY || eh == FM_SEQ_EUCLID_SONG || eh == FM_SEQ_EUCLID_EDITED ||
        eh == FM_SEQ_EUCLID_FRESH)
        return 0u;
    return eh > 16u ? 16u : eh;
}

static inline void fm_seq_fmt_euclid(uint8_t eh, char *out)
{
    uint8_t h;

    if (out == 0)
        return;
    if (eh == FM_SEQ_EUCLID_EMPTY) {
        out[0] = 'E';
        out[1] = 'M';
        out[2] = 'P';
        out[3] = 'T';
        out[4] = 'Y';
        out[5] = '\0';
        return;
    }
    if (eh == FM_SEQ_EUCLID_FRESH) {
        out[0] = 'F';
        out[1] = 'R';
        out[2] = 'E';
        out[3] = 'S';
        out[4] = 'H';
        out[5] = '\0';
        return;
    }
    if (eh == FM_SEQ_EUCLID_EDITED) {
        out[0] = 'E';
        out[1] = 'D';
        out[2] = 'I';
        out[3] = 'T';
        out[4] = 'E';
        out[5] = 'D';
        out[6] = '\0';
        return;
    }
    if (eh == FM_SEQ_EUCLID_SONG) {
        out[0] = 'B';
        out[1] = 'A';
        out[2] = 'S';
        out[3] = 'E';
        out[4] = '\0';
        return;
    }
    h = eh > 16u ? 16u : eh;
    out[0] = 'E';
    out[1] = 'U';
    out[2] = 'C';
    out[3] = ' ';
    if (h >= 10u) {
        out[4] = '1';
        out[5] = (char)('0' + (h - 10u));
        out[6] = '\0';
    } else {
        out[4] = (char)('0' + h);
        out[5] = '\0';
    }
}

static inline const char *fm_seq_knob_label_lane(uint8_t page, int plock_held, uint8_t slot, uint8_t lane)
{
    slot &= 3u;
    if (plock_held) {
        if (lane == 3u) {
            static const char *const HP_PLOCK[4] = {"BEND", "COND", "GATE", "PITCH"};

            return HP_PLOCK[slot];
        }
        return FM_SEQ_KNOB_PLOCK[slot];
    }
    if (page == 0u)
        return FM_SEQ_KNOB_P0[slot];
    if (page == 1u)
        return FM_SEQ_KNOB_P1[slot];
    if (page == 2u || page == 3u)
        return FM_SEQ_KNOB_P2[slot];
    return "";
}

static inline const char *fm_seq_knob_label(uint8_t page, int plock_held, uint8_t slot)
{
    return fm_seq_knob_label_lane(page, plock_held, slot, 255u);
}

static inline uint8_t fm_seq_euclid_rot_norm(int32_t rot)
{
    return (uint8_t)(((rot % 16) + 16) % 16);
}

static inline uint16_t fm_seq_rot16(uint16_t m, uint8_t r)
{
    r &= 15u;
    if (r == 0u)
        return m;
    return (uint16_t)((m << r) | (m >> (16u - r)));
}

static inline uint16_t fm_seq_rot16_step(uint16_t m, int32_t dir)
{
    if (dir > 0)
        return (uint16_t)((m << 1) | (m >> 15));
    return (uint16_t)((m >> 1) | (m << 15));
}

/* Distribute hits evenly, then roll so the first pulse is step 0 (before user ROTATE). */
static inline uint16_t fm_seq_euclid_canon(uint16_t mask, uint8_t steps)
{
    uint8_t i;

    if (mask == 0u || steps == 0u)
        return mask;
    for (i = 0u; i < steps; i++) {
        if ((mask & (uint16_t)(1u << i)) != 0u) {
            if (i == 0u)
                return mask;
            if (steps == 16u)
                return fm_seq_rot16(mask, (uint8_t)(16u - i));
            {
                uint16_t out = 0u;
                uint8_t j;

                for (j = 0u; j < steps; j++) {
                    if ((mask & (uint16_t)(1u << ((uint8_t)(i + j) % steps))) != 0u)
                        out = (uint16_t)(out | (uint16_t)(1u << j));
                }
                return out;
            }
        }
    }
    return mask;
}

/* Steps that differ between two patterns cross one at a time (frac 0 = all a, 255 = all b). */
static inline int fm_seq_morph_bit(int a, int b, uint8_t step, uint8_t frac, uint8_t salt)
{
    uint8_t gate;

    if (a == b)
        return a;
    gate = (uint8_t)(12u + (uint8_t)((salt + (uint8_t)((step & 15u) * 19u)) % 220u));
    return a ? (frac < gate) : (frac >= gate);
}

static inline uint16_t fm_seq_morph_mask(uint16_t lo, uint16_t hi, uint8_t frac, uint8_t salt)
{
    uint16_t out = 0u;
    uint8_t s;

    for (s = 0; s < 16u; s++) {
        if (fm_seq_morph_bit((lo >> s) & 1, (hi >> s) & 1, s, frac, salt))
            out = (uint16_t)(out | (uint16_t)(1u << s));
    }
    return out;
}

static inline void fm_seq_rot_u8_16(uint8_t a[16], int32_t dir)
{
    uint8_t tmp[16];
    uint32_t st;
    uint32_t dst;

    for (st = 0; st < 16u; st++) {
        dst = (st + (dir > 0 ? 1u : 15u)) & 15u;
        tmp[dst] = a[st];
    }
    for (st = 0; st < 16u; st++)
        a[st] = tmp[st];
}

static inline void fm_seq_rot_i8_16(int8_t a[16], int32_t dir)
{
    int8_t tmp[16];
    uint32_t st;
    uint32_t dst;

    for (st = 0; st < 16u; st++) {
        dst = (st + (dir > 0 ? 1u : 15u)) & 15u;
        tmp[dst] = a[st];
    }
    for (st = 0; st < 16u; st++)
        a[st] = tmp[st];
}

/* Patterns are 1..8 pages of 16 steps. Plates, generators, Euclid and ROTATE work per page,
 * so every page keeps its own bar and a 16-step pattern repeats unchanged on every page. */
static inline uint8_t fm_seq_pages_clamp(uint8_t pages)
{
    if (pages < 1u)
        return 1u;
    return pages > FM_SEQ_MAX_PAGES ? (uint8_t)FM_SEQ_MAX_PAGES : pages;
}

static inline uint32_t fm_seq_len(uint8_t pages)
{
    return (uint32_t)fm_seq_pages_clamp(pages) * FM_SEQ_PAGE_STEPS;
}

/* S1 = view 0, S2..S8 = views 1..7 (128-step st). */
static inline uint8_t fm_seq_view_of_step(uint32_t st)
{
    return (uint8_t)((st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
}

/* View chain: playback runs only through the views [0, pages) that are on (view_on), in order and
 * wrapping, FM_SEQ_PAGE_STEPS steps each. st when its view is on, else the start of the next view that
 * is on; FM_SEQ_STEPS when none is. */
static inline uint32_t fm_seq_chain_at(const uint8_t *view_on, uint8_t pages, uint32_t st)
{
    uint32_t v, i;

    pages = fm_seq_pages_clamp(pages);
    st %= fm_seq_len(pages);
    v = st / FM_SEQ_PAGE_STEPS;
    for (i = 0; i < pages; i++) {
        uint32_t w = (v + i) % pages;

        if (view_on[w])
            return i ? w * FM_SEQ_PAGE_STEPS : st;
    }
    return FM_SEQ_STEPS;
}

/* The step after st: st + 1 inside its view, whether that view is on or not (fm_seq_chain_at checks);
 * past the view's end the start of the next view that is on, st's own view last. */
static inline uint32_t fm_seq_chain_next(const uint8_t *view_on, uint8_t pages, uint32_t st)
{
    if ((st + 1u) % FM_SEQ_PAGE_STEPS != 0u)
        return st + 1u;
    return fm_seq_chain_at(view_on, pages, st + 1u);
}

static inline uint8_t fm_seq_lane_len_clamp(uint8_t len)
{
    if (len < 1u)
        return 1u;
    return len > FM_SEQ_STEPS ? (uint8_t)FM_SEQ_STEPS : len;
}

static inline int8_t fm_seq_micro_clamp(int32_t micro)
{
    if (micro < FM_SEQ_MICRO_MIN)
        return (int8_t)FM_SEQ_MICRO_MIN;
    if (micro > FM_SEQ_MICRO_MAX)
        return (int8_t)FM_SEQ_MICRO_MAX;
    return (int8_t)micro;
}

static inline int8_t fm_seq_lane_nudge_clamp(int32_t nudge)
{
    if (nudge < FM_SEQ_LANE_NUDGE_MIN)
        return (int8_t)FM_SEQ_LANE_NUDGE_MIN;
    if (nudge > FM_SEQ_LANE_NUDGE_MAX)
        return (int8_t)FM_SEQ_LANE_NUDGE_MAX;
    return (int8_t)nudge;
}

static inline int32_t fm_seq_timing_sum_clamp(int32_t units)
{
    if (units < FM_SEQ_TIMING_SUM_MIN)
        return FM_SEQ_TIMING_SUM_MIN;
    if (units > FM_SEQ_TIMING_SUM_MAX)
        return FM_SEQ_TIMING_SUM_MAX;
    return units;
}

/* Absolute grid step 0..FM_SEQ_STEPS-1 wrapped to lane length (timing nudge does not shift index). */
static inline uint32_t fm_seq_lane_map_step(uint32_t grid_st, uint8_t len)
{
    int32_t col;

    len = fm_seq_lane_len_clamp(len);
    col = (int32_t)grid_st;
    while (col < 0)
        col += (int32_t)len;
    col %= (int32_t)len;
    return (uint32_t)col;
}

static inline void fm_seq_fmt_micro_readout(int32_t units, char *out)
{
    int32_t pct;
    uint32_t mag;

    if (out == 0)
        return;
    if (units == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    pct = (units * 50) / FM_SEQ_MICRO_HALF;
    if (pct > 0)
        *out++ = '+';
    else {
        *out++ = '-';
        pct = -pct;
    }
    mag = (uint32_t)pct;
    if (mag >= 100u) {
        *out++ = (char)('0' + mag / 100u);
        mag %= 100u;
    }
    if (mag >= 10u) {
        *out++ = (char)('0' + mag / 10u);
        mag %= 10u;
    }
    *out++ = (char)('0' + mag);
    *out++ = '%';
    *out = '\0';
}

/* Nudge readout: ±1.0 step = ±100 % lane shift (fits knob bar; avoids “+100 %” clipped to “+10”). */
static inline void fm_seq_fmt_nudge_readout(int32_t units, char *out)
{
    uint32_t mag;
    uint32_t whole;
    uint32_t frac;

    if (out == 0)
        return;
    if (units == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    if (units > 0)
        *out++ = '+';
    else {
        *out++ = '-';
        units = -units;
    }
    mag = (uint32_t)units;
    whole = (mag * 10u) / (uint32_t)FM_SEQ_TIMING_UNITS;
    frac = whole % 10u;
    whole /= 10u;
    if (whole > 0u)
        *out++ = (char)('0' + whole);
    else
        *out++ = '0';
    *out++ = '.';
    *out++ = (char)('0' + frac);
    *out = '\0';
}

static inline void fm_seq_rot_u8_pages(uint8_t *a, int32_t dir)
{
    uint32_t p;

    for (p = 0; p < FM_SEQ_MAX_PAGES; p++)
        fm_seq_rot_u8_16(a + p * FM_SEQ_PAGE_STEPS, dir);
}

static inline void fm_seq_rot_i8_pages(int8_t *a, int32_t dir)
{
    uint32_t p;

    for (p = 0; p < FM_SEQ_MAX_PAGES; p++)
        fm_seq_rot_i8_16(a + p * FM_SEQ_PAGE_STEPS, dir);
}

static inline int fm_seq_bit(const uint8_t *m, uint32_t st)
{
    st %= FM_SEQ_STEPS;
    return (m[st >> 3] >> (st & 7u)) & 1u;
}

static inline void fm_seq_bit_put(uint8_t *m, uint32_t st, int on)
{
    uint8_t b;

    st %= FM_SEQ_STEPS;
    b = (uint8_t)(1u << (st & 7u));
    if (on)
        m[st >> 3] |= b;
    else
        m[st >> 3] &= (uint8_t)~b;
}

static inline void fm_seq_bits_fill(uint8_t *m, int on)
{
    uint32_t i;

    for (i = 0; i < FM_SEQ_MASK_BYTES; i++)
        m[i] = on ? 0xFFu : 0u;
}

/* any / all of the first len steps (a whole number of pages) */
static inline int fm_seq_bits_any(const uint8_t *m, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len / 8u && i < FM_SEQ_MASK_BYTES; i++)
        if (m[i])
            return 1;
    return 0;
}

static inline int fm_seq_bits_all(const uint8_t *m, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len / 8u && i < FM_SEQ_MASK_BYTES; i++)
        if (m[i] != 0xFFu)
            return 0;
    return 1;
}

static inline uint16_t fm_seq_bits_page16(const uint8_t *m, uint32_t page)
{
    page %= FM_SEQ_MAX_PAGES;
    return (uint16_t)(m[page * 2u] | ((uint16_t)m[page * 2u + 1u] << 8));
}

static inline void fm_seq_bits_set_page16(uint8_t *m, uint32_t page, uint16_t v)
{
    page %= FM_SEQ_MAX_PAGES;
    m[page * 2u] = (uint8_t)v;
    m[page * 2u + 1u] = (uint8_t)(v >> 8);
}

static inline void fm_seq_bits_tile16(uint8_t *m, uint16_t v)
{
    uint32_t p;

    for (p = 0; p < FM_SEQ_MAX_PAGES; p++)
        fm_seq_bits_set_page16(m, p, v);
}

static inline void fm_seq_bits_rot_pages(uint8_t *m, int32_t dir)
{
    uint32_t p;

    for (p = 0; p < FM_SEQ_MAX_PAGES; p++)
        fm_seq_bits_set_page16(m, p, fm_seq_rot16_step(fm_seq_bits_page16(m, p), dir));
}

/* "PG 2/4" and "LEN 64": fit the 8-byte knob flash */
static inline void fm_seq_fmt_page(char *out, uint8_t page, uint8_t pages)
{
    out[0] = 'P';
    out[1] = 'G';
    out[2] = ' ';
    out[3] = (char)('1' + (page % FM_SEQ_MAX_PAGES));
    out[4] = '/';
    out[5] = (char)('0' + fm_seq_pages_clamp(pages));
    out[6] = '\0';
}

static inline void fm_seq_fmt_len(char *out, uint8_t pages)
{
    uint32_t n = fm_seq_len(pages);
    uint32_t i = 4u;

    out[0] = 'L';
    out[1] = 'E';
    out[2] = 'N';
    out[3] = ' ';
    if (n >= 100u)
        out[i++] = (char)('0' + n / 100u);
    out[i++] = (char)('0' + (n / 10u) % 10u);
    out[i++] = (char)('0' + n % 10u);
    out[i] = '\0';
}

/* OCT - / OCT + in SEQ: the view one page back or on. Past the first or last page the whole pattern
 * doubles (16 -> 32 -> 64 -> 128) and the view lands on the copy next to the old page: the copy goes
 * before the pattern on OCT -, after it on OCT +. The data is the same either way, steps [len, 2 len)
 * = [0, len); on FM_SEQ_NAV_PREPEND the original is the second half, so the caller moves the playhead
 * and every absolute step index on by the old length (fm_seq_dup_ix). */
enum { FM_SEQ_NAV_MOVE = 0, FM_SEQ_NAV_APPEND, FM_SEQ_NAV_PREPEND, FM_SEQ_NAV_FULL };

static inline uint8_t fm_seq_view_nav(uint8_t *view, uint8_t pages, int32_t dir)
{
    uint8_t v;

    pages = fm_seq_pages_clamp(pages);
    v = (uint8_t)(*view % pages);
    *view = v;
    if (dir > 0 && v + 1u < pages) {
        *view = (uint8_t)(v + 1u);
        return FM_SEQ_NAV_MOVE;
    }
    if (dir < 0 && v > 0u) {
        *view = (uint8_t)(v - 1u);
        return FM_SEQ_NAV_MOVE;
    }
    if ((uint32_t)pages * 2u > FM_SEQ_MAX_PAGES)
        return FM_SEQ_NAV_FULL;
    *view = dir > 0 ? pages : (uint8_t)(pages - 1u);
    return dir > 0 ? FM_SEQ_NAV_APPEND : FM_SEQ_NAV_PREPEND;
}

/* steps [len, 2 len) of a per-step array (esz bytes a step) = steps [0, len) */
static inline void fm_seq_dup_bytes(void *a, uint32_t len, uint32_t esz)
{
    uint8_t *p = (uint8_t *)a;
    uint32_t i;

    for (i = 0; i < len * esz; i++)
        p[len * esz + i] = p[i];
}

static inline void fm_seq_bits_dup(uint8_t *m, uint32_t len)
{
    fm_seq_dup_bytes(m, len / 8u, 1u);
}

static inline uint32_t fm_seq_dup_ix(uint32_t ix, uint32_t len, uint8_t nav)
{
    return nav == FM_SEQ_NAV_PREPEND && ix < len ? ix + len : ix;
}

/* the knob flash after OCT -/+: "PG 2/4", "LEN 64", "MAX 128" */
static inline void fm_seq_fmt_nav(char *out, uint8_t nav, uint8_t view, uint8_t pages)
{
    if (nav == FM_SEQ_NAV_MOVE) {
        fm_seq_fmt_page(out, view, pages);
    } else if (nav == FM_SEQ_NAV_FULL) {
        fm_seq_fmt_len(out, FM_SEQ_MAX_PAGES);
        out[0] = 'M';
        out[1] = 'A';
        out[2] = 'X';
    } else {
        fm_seq_fmt_len(out, pages);
    }
}

static inline uint16_t fm_seq_euclid16(uint8_t hits, int8_t rot)
{
    uint16_t mask = 0;
    uint8_t h = hits;

    if (h == 0u)
        return 0u;
    if (h >= 16u)
        return 0xFFFFu;
    {
        uint8_t bucket = 0;
        uint8_t i;

        for (i = 0; i < 16u; i++) {
            bucket = (uint8_t)(bucket + h);
            if (bucket >= 16u) {
                bucket = (uint8_t)(bucket - 16u);
                mask = (uint16_t)(mask | (uint16_t)(1u << i));
            }
        }
    }
    mask = fm_seq_euclid_canon(mask, 16u);
    return fm_seq_rot16(mask, (uint8_t)((rot + 16) & 15));
}

static inline void fm_seq_euclid_mask(uint8_t hits, uint8_t steps, uint8_t rot, uint16_t *out_mask)
{
    uint8_t h = hits;
    uint8_t s = steps;
    uint16_t mask = 0;

    if (out_mask == 0)
        return;
    if (s == 0u || h == 0u) {
        *out_mask = 0;
        return;
    }
    if (h >= s) {
        *out_mask = (uint16_t)((1u << s) - 1u);
        return;
    }
    {
        uint8_t bucket = 0;
        uint8_t i;

        for (i = 0; i < s; i++) {
            bucket = (uint8_t)(bucket + h);
            if (bucket >= s) {
                bucket = (uint8_t)(bucket - s);
                mask = (uint16_t)(mask | (uint16_t)(1u << i));
            }
        }
    }
    mask = fm_seq_euclid_canon(mask, s);
    *out_mask = fm_seq_rot16(mask, rot);
}

/* hashed on the step within its page: the copies of a doubled pattern roll like the page did, bar for bar */
static inline uint8_t fm_seq_step_chance(uint8_t step, uint8_t voice, uint32_t salt)
{
    uint32_t h = (uint32_t)(step & 15u) * 374761393u + (uint32_t)voice * 668265263u + salt * 2246822519u;

    h = (h ^ (h >> 13)) * 1274126177u;
    return (uint8_t)(((h ^ (h >> 16)) & 0xFFu) * 100u / 256u);
}

static inline uint8_t fm_seq_vary_norm(uint8_t v)
{
    return v > 127u ? 127u : v;
}

static inline uint8_t fm_seq_vary_tier(uint8_t v)
{
    v = fm_seq_vary_norm(v);
    if (v == 0u)
        return 0u;
    if (v <= 31u)
        return 1u;
    if (v <= 63u)
        return 2u;
    if (v <= 95u)
        return 3u;
    return 4u;
}

static inline void fm_seq_fmt_vary(uint8_t v, char *out)
{
    const char *name;
    uint8_t i;
    uint8_t pct;

    if (out == 0)
        return;
    v = fm_seq_vary_norm(v);
    name = FM_SEQ_VARY_NAMES[fm_seq_vary_tier(v)];
    for (i = 0u; name[i] != '\0' && i < 4u; i++)
        out[i] = name[i];
    out[i++] = ' ';
    if (v == 0u) {
        out[i++] = '0';
        out[i] = '\0';
        return;
    }
    pct = (uint8_t)(((uint32_t)v * 100u) / 127u);
    if (pct == 0u)
        pct = 1u;
    if (pct >= 100u) {
        out[i++] = '1';
        out[i++] = '0';
        out[i++] = '0';
    } else if (pct >= 10u) {
        out[i++] = (char)('0' + (pct / 10u));
        out[i++] = (char)('0' + (pct % 10u));
    } else
        out[i++] = (char)('0' + pct);
    out[i] = '\0';
}

static inline void fm_seq_cond_ratio_order(uint8_t idx, uint8_t *a, uint8_t *b)
{
    uint8_t bv;
    uint8_t av;

    for (bv = 2u; bv <= 8u; bv++) {
        for (av = 1u; av <= bv; av++) {
            if (idx == 0u) {
                *a = av;
                *b = bv;
                return;
            }
            idx--;
        }
    }
    *a = 1u;
    *b = 2u;
}

static inline int fm_seq_cond_ratio_hit(uint8_t a, uint8_t cycle, uint8_t bar_slot)
{
    return (bar_slot % cycle) == (uint8_t)(a - 1u);
}

/* Plock COND knob bar: index 0 = 100% full, max index = !1:3 empty. */
static inline uint8_t fm_seq_cond_bar_px(uint8_t cond)
{
    if (cond >= FM_SEQ_NCONDS)
        cond = 0u;
    if (FM_SEQ_COND_MAX == 0u)
        return 127u;
    return (uint8_t)(((uint32_t)(FM_SEQ_COND_MAX - cond) * 127u) / (uint32_t)FM_SEQ_COND_MAX);
}

static inline int fm_seq_cond_pass(uint8_t cond, uint8_t step, uint8_t bar, uint8_t voice, int is_fill)
{
    static const uint8_t PCT[6] = {100u, 90u, 75u, 50u, 25u, 10u};
    uint8_t b = (uint8_t)(bar & 7u);

    if (cond == 0u)
        return 1;
    if (cond <= 5u)
        return fm_seq_step_chance(step, voice, 90u + bar * 17u) < PCT[cond];
    if (cond <= 7u)
        return (b % 2u) == (uint8_t)(cond - 6u);
    if (cond <= 11u)
        return (b % 4u) == (uint8_t)(cond - 8u);
    if (cond <= 19u)
        return (b % 8u) == (uint8_t)(cond - 12u);
    if (cond == 20u)
        return is_fill;
    if (cond == 21u)
        return !is_fill;
    if (cond >= FM_SEQ_COND_NEW_BASE && cond < FM_SEQ_COND_NEW_BASE + FM_SEQ_COND_NEW_NR) {
        uint8_t i = (uint8_t)(cond - FM_SEQ_COND_NEW_BASE);

        return fm_seq_cond_ratio_hit(FM_SEQ_COND_NEW_A[i], FM_SEQ_COND_NEW_B[i], b);
    }
    if (cond >= FM_SEQ_COND_NOT_BASE && cond < FM_SEQ_COND_NOT_BASE + FM_SEQ_COND_NOT_NR) {
        uint8_t a;
        uint8_t cycle;
        uint8_t not_ix;
        uint8_t ratio_ix;
        int hit;

        not_ix = (uint8_t)(cond - FM_SEQ_COND_NOT_BASE);
        ratio_ix = (uint8_t)(FM_SEQ_COND_NOT_RATIO_SKIP +
                             (FM_SEQ_COND_NOT_NR - 1u - not_ix));
        fm_seq_cond_ratio_order(ratio_ix, &a, &cycle);
        hit = fm_seq_cond_ratio_hit(a, cycle, b);
        return !hit;
    }
    return fm_seq_step_chance(step, voice, 90u + bar) <= cond;
}

/* View CYC: OFF/ALL = no bar filter; 1:2..4:4 bar slots; ONLY = ALL playback + solo other pages OFF. */
static inline int fm_seq_view_cyc_pass(uint8_t cyc, uint8_t bar)
{
    uint8_t cond;

    if (cyc <= FM_SEQ_VIEW_CYC_ALL || cyc == FM_SEQ_VIEW_CYC_ONLY)
        return 1;
    if (cyc >= FM_SEQ_NVIEW_CYC)
        return 1;
    cond = (uint8_t)(cyc + 4u);
    return fm_seq_cond_pass(cond, 0u, bar, 0u, 0);
}

static inline int fm_seq_vary_roll_hit(int base_on, uint8_t vary, uint8_t step, uint8_t roll)
{
    uint8_t mutate_pct;

    mutate_pct = (uint8_t)(((uint32_t)vary * 88u) / 127u);
    if (roll >= mutate_pct)
        return base_on;
    if (!base_on) {
        if (roll < (uint8_t)(((uint32_t)vary * 64u) / 127u))
            return 1;
        return 0;
    }
    if (vary >= 48u && roll > 82u && (step & 3u) != 0u)
        return 0;
    if (vary >= 32u && roll < (uint8_t)(((uint32_t)vary * 28u) / 127u))
        return 0;
    return 1;
}

static inline int fm_seq_vary_hit(int base_on, uint8_t vary, uint8_t step, uint8_t bar, uint8_t lane,
                                  uint32_t seed)
{
    vary = fm_seq_vary_norm(vary);
    if (vary == 0u)
        return base_on;
    return fm_seq_vary_roll_hit(base_on, vary, step,
                                fm_seq_step_chance(step, (uint8_t)(lane + (bar & 7u)), 41u + (seed & 255u)));
}

/* Per-phrase VARY roll: hashes the absolute step (all 8 bars differ) and no bar counter, so a grid that draws
 * every bar with the same seed shows exactly what each bar will play until the seed changes. */
static inline uint8_t fm_seq_vary_chance(uint8_t step, uint8_t lane, uint32_t seed)
{
    uint32_t h = (uint32_t)(step & 127u) * 374761393u + (uint32_t)lane * 668265263u + seed * 2246822519u;

    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (uint8_t)((h & 0xFFu) * 100u / 256u);
}

static inline int fm_seq_vary_hit_phrase(int base_on, uint8_t vary, uint8_t step, uint8_t lane, uint32_t seed)
{
    vary = fm_seq_vary_norm(vary);
    if (vary == 0u)
        return base_on;
    return fm_seq_vary_roll_hit(base_on, vary, step, fm_seq_vary_chance(step, lane, seed));
}

static inline uint8_t fm_seq_trig_pct(int8_t raw)
{
    if (raw >= 0)
        return 100u;
    if (raw < -100)
        return 0u;
    return (uint8_t)(100 + raw);
}

static inline int8_t fm_seq_trig_adj(int8_t raw, int32_t delta, int edit)
{
    int32_t pct;

    if (edit)
        return 0;
    pct = (int32_t)fm_seq_trig_pct(raw) + delta;
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    return (int8_t)(pct - 100);
}

static inline void fm_seq_fmt_trig(int8_t raw, char *out)
{
    uint8_t pct;

    if (out == 0)
        return;
    pct = fm_seq_trig_pct(raw);
    if (pct >= 100u) {
        out[0] = '1';
        out[1] = '0';
        out[2] = '0';
        out[3] = '%';
        out[4] = '\0';
        return;
    }
    if (pct >= 10u) {
        out[0] = (char)('0' + pct / 10u);
        out[1] = (char)('0' + pct % 10u);
        out[2] = '%';
        out[3] = '\0';
        return;
    }
    out[0] = (char)('0' + pct);
    out[1] = '%';
    out[2] = '\0';
}

static inline int fm_seq_trig_pass(int8_t raw, uint8_t step, uint8_t bar, uint8_t lane)
{
    uint8_t pct = fm_seq_trig_pct(raw);

    if (pct >= 100u)
        return 1;
    if (pct == 0u)
        return 0;
    return fm_seq_step_chance(step, lane, 131u + (uint32_t)bar * 23u) < pct;
}

/* OXI One MKII style variance range:
 * - amt == 0: 0 (no variance)
 * - amt > 0:  unipolar upward random variance in [0 .. +amt]
 * - amt < 0:  bipolar random variance in [-|amt| .. +|amt|]
 */
static inline int8_t fm_seq_rand_val(int8_t amt, uint8_t step, uint8_t bar, uint8_t lane, uint32_t seed,
                                     uint32_t salt)
{
    int32_t lo;
    int32_t hi;
    int32_t span;
    uint8_t roll;
    int32_t v;

    if (amt == 0)
        return 0;
    if (amt > 0) {
        lo = 0;
        hi = (int32_t)amt;
    } else {
        int32_t s = -(int32_t)amt;

        lo = -s;
        hi = s;
    }
    span = hi - lo + 1;
    if (span <= 0)
        return 0;
    roll = fm_seq_step_chance(step, lane, salt + (uint32_t)bar * 37u + (seed & 255u));
    v = lo + ((int32_t)roll * span) / 100;
    if (v < lo)
        v = lo;
    if (v > hi)
        v = hi;
    if (v < -127)
        v = -127;
    if (v > 127)
        v = 127;
    return (int8_t)v;
}

static inline int8_t fm_seq_rand_vel(int8_t amt, uint8_t step, uint8_t bar, uint8_t lane, uint32_t seed)
{
    return fm_seq_rand_val(amt, step, bar, lane, seed, 53u);
}

static inline int8_t fm_seq_rand_gate(int8_t amt, uint8_t step, uint8_t bar, uint8_t lane, uint32_t seed)
{
    return fm_seq_rand_val(amt, step, bar, lane, seed, 97u);
}

static inline int8_t fm_seq_rand_pitch(int8_t amt, uint8_t step, uint8_t bar, uint8_t lane, uint32_t seed)
{
    return fm_seq_rand_val(amt, step, bar, lane, seed, 173u);
}

/* Signed amount in [-lim .. +lim] → pot 0..127 with 0 at centre (64). */
static inline uint8_t fm_seq_bip_pot(int32_t amt, int32_t lim)
{
    if (lim <= 0)
        return 64u;
    if (amt > lim)
        amt = lim;
    if (amt < -lim)
        amt = -lim;
    return (uint8_t)(amt >= 0 ? 64 + (int32_t)(amt * 63 / lim) : 64 + (int32_t)(amt * 64 / lim));
}

static inline void fm_seq_fmt_rand_range(int8_t amt, char *out)
{
    uint32_t mag;

    if (out == 0)
        return;
    if (amt == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    if (amt > 0) {
        *out++ = '+';
        mag = (uint32_t)amt;
    } else {
        *out++ = '-';
        mag = (uint32_t)(-(int32_t)amt);
    }
    if (mag >= 100u) {
        *out++ = (char)('0' + mag / 100u);
        mag %= 100u;
        *out++ = (char)('0' + mag / 10u);
        *out++ = (char)('0' + mag % 10u);
    } else if (mag >= 10u) {
        *out++ = (char)('0' + mag / 10u);
        *out++ = (char)('0' + mag % 10u);
    } else {
        *out++ = (char)('0' + mag);
    }
    *out = '\0';
}

/* Random-spread readout matching fm_seq_rand_val: amt > 0 → "+N" (0..+N), amt < 0 → "±N" (−N..+N).
 * 0xB1 is the FONT_S ± glyph (gfx.c glyph()). */
static inline void fm_seq_fmt_rand_spread(int8_t amt, char *out)
{
    if (out == 0)
        return;
    fm_seq_fmt_rand_range(amt, out);
    if (amt < 0)
        out[0] = (char)0xB1;
}

#if defined(C_WHITE) && !defined(FM_SEQ_UI_H)
#define FM_SEQ_UI_H

typedef struct {
    uint16_t col;
    uint16_t bg_col;
    uint8_t vel;
    int8_t tune;
    int8_t micro;
    uint8_t cond;
    int8_t gate;
    uint8_t note_span; /* lead/stab gate length in 16th steps (attack step); 0 or 1 = one cell */
    uint8_t note_tail; /* 1 = held continuation under a prior attack (no retrigger) */
} fm_seq_cell_t;

static inline void fm_seq_draw_dot_diamond(int32_t x, int32_t y, uint16_t col)
{
    int32_t ry;

    for (ry = 0; ry < 5; ry++) {
        int32_t d = ry > 2 ? ry - 2 : 2 - ry;
        int32_t w = 5 - 2 * d;

        cv_rect(x + d, y + ry, w, 1, col);
    }
}

static inline void fm_seq_draw_page_dots(int32_t x0, int32_t y, uint8_t page, uint8_t npages)
{
    uint32_t pi;

    if (npages == 0u)
        return;
    page = (uint8_t)(page % npages);
    for (pi = 0; pi < (uint32_t)npages; pi++)
        fm_seq_draw_dot_diamond(x0 + (int32_t)pi * 8, y, pi == (uint32_t)page ? C_WHITE : C_DIM);
}

static inline uint8_t fm_seq_knob_page_cycle_idx(uint8_t page)
{
    uint32_t i;

    for (i = 0u; i < FM_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (FM_SEQ_KNOB_PAGE_CYCLE[i] == page)
            return (uint8_t)i;
    }
    return 0u;
}

static inline void fm_seq_draw_knob_page_cycle_dots(int32_t x0, int32_t y, uint8_t seq_page)
{
    fm_seq_draw_page_dots(x0, y, fm_seq_knob_page_cycle_idx(seq_page), FM_SEQ_KNOB_PAGE_CYCLE_N);
}

static inline int fm_seq_page_in_knob_cycle(uint8_t p)
{
    uint32_t i;

    for (i = 0u; i < FM_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (FM_SEQ_KNOB_PAGE_CYCLE[i] == p)
            return 1;
    }
    return 0;
}

static inline uint8_t fm_seq_knob_page_cycle_next(uint8_t cur)
{
    uint32_t i;

    for (i = 0u; i < FM_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (FM_SEQ_KNOB_PAGE_CYCLE[i] == cur)
            return FM_SEQ_KNOB_PAGE_CYCLE[(i + 1u) % FM_SEQ_KNOB_PAGE_CYCLE_N];
    }
    return 0u;
}

static inline void fm_seq_view_page_step(uint8_t *view_page, int32_t dir)
{
    uint8_t v;

    if (!view_page || !dir)
        return;
    v = (uint8_t)(*view_page % FM_SEQ_MAX_PAGES);
    if (dir > 0)
        v = (uint8_t)((v + 1u) % FM_SEQ_MAX_PAGES);
    else
        v = (uint8_t)((v + FM_SEQ_MAX_PAGES - 1u) % FM_SEQ_MAX_PAGES);
    *view_page = v;
}

/* One 5x8 view cell: inactive full C_DIM; inactive+viewed 3x5 C_DIM; active full C_WHITE;
 * active+viewed 3x5 play_col; playing 1 px play_col frame (+ 3x5 play_col if viewed). */
static inline void fm_seq_draw_view_dot(int32_t x, int32_t y, int on_play, int on_view, uint16_t core_col,
                                        uint16_t block_col, uint16_t play_col)
{
    int on_active = block_col != C_DIM;

    (void)core_col;

    if (!on_active && !on_play && !on_view) {
        cv_rect(x, y, 5, 8, C_DIM);
        return;
    }
    if (!on_active && on_view && !on_play) {
        cv_rect(x + 1, y + 1, 3, 6, C_DIM);
        return;
    }
    if (on_active && !on_play && !on_view) {
        cv_rect(x, y, 5, 8, play_col);
        return;
    }
    if (on_active && !on_play && on_view) {
        cv_rect(x + 1, y + 1, 3, 6, C_WHITE);
        return;
    }
    if (on_view)
        cv_rect(x + 1, y + 1, 3, 6, C_WHITE);
    cv_rect(x, y, 5, 1, C_WHITE);
    cv_rect(x, y + 7, 5, 1, C_WHITE);
    cv_rect(x, y + 1, 1, 6, C_WHITE);
    cv_rect(x + 4, y + 1, 1, 6, C_WHITE);
}

/* Pattern view dots at x0: all FM_SEQ_MAX_PAGES views whatever the length; core and block C_WHITE when
 * view_on[view], else C_DIM. */
static inline void fm_seq_draw_step_dots_at(int32_t x0, int32_t y, uint8_t view_page, uint8_t play_page,
                                            const uint8_t *view_on, int playing, uint16_t play_col)
{
    uint32_t pi;

    for (pi = 0; pi < FM_SEQ_MAX_PAGES; pi++) {
        uint16_t col = view_on[pi] ? C_WHITE : C_DIM;

        fm_seq_draw_view_dot(x0 + (int32_t)pi * 8, y, playing && pi == (uint32_t)play_page,
                             pi == (uint32_t)view_page, col, col, play_col);
    }
}

/* Centered under the grid, one cell per page up to pages (none past length), C_WHITE core, C_DIM block;
 * rytm/ctrl pass y only — clipping is caller’s problem. */
static inline void fm_seq_draw_step_dots(int32_t y, uint8_t view_page, uint8_t play_page, uint8_t pages,
                                         int playing, uint16_t play_col)
{
    int32_t x0 = (240 - ((int32_t)FM_SEQ_MAX_PAGES * 8 - 3)) / 2;
    uint32_t pi;

    pages = fm_seq_pages_clamp(pages);
    for (pi = 0; pi < (uint32_t)pages; pi++)
        fm_seq_draw_view_dot(x0 + (int32_t)pi * 8, y, playing && pi == (uint32_t)play_page,
                             pi == (uint32_t)view_page, C_WHITE, C_DIM, play_col);
}

#ifndef FM_SEQ_LANE_LBL_W
#define FM_SEQ_LANE_LBL_W 14
#endif
#ifndef FM_SEQ_GRID_X0
#define FM_SEQ_GRID_X0 20
#endif
#ifndef FM_SEQ_GRID8_Y0
#define FM_SEQ_GRID8_Y0 2
#endif
#ifndef FM_SEQ_GRID8_ROW_H
#define FM_SEQ_GRID8_ROW_H 16
#endif
#ifndef FM_SEQ_GRID8_CELL_H
#define FM_SEQ_GRID8_CELL_H 11
#endif

static inline int32_t fm_seq_grid_step_x(int32_t st)
{
    return FM_SEQ_GRID_X0 + st * 13 + (st / 4) * 3;
}

/* Center 2× FONT_S in lane chip; keep glyph ink (±2 px pad) inside chip width. */
static inline int32_t fm_seq_lane_lbl_tx(const char *s)
{
    int32_t chip_x = 1;
    int32_t lbl_w = FM_SEQ_LANE_LBL_W;
    int32_t tw = text_w(&FONT_S, s);
    int32_t tx = chip_x + (lbl_w - tw) / 2;
    const int32_t pad = 2;

    if (tx < chip_x + pad)
        tx = chip_x + pad;
    if (tx + tw + pad > chip_x + lbl_w)
        tx = chip_x + lbl_w - tw - pad;
    return tx;
}

static inline void fm_seq_draw_lane_lbl(int32_t y, int32_t cell_h, int selected, int muted, int solo, uint16_t lcol,
                                        const char *lbl)
{
    int32_t lbl_w = FM_SEQ_LANE_LBL_W;
    int32_t tx = fm_seq_lane_lbl_tx(lbl);

    if (selected) {
        if (!muted) {
            cv_rect(1, y + 1, (uint32_t)lbl_w, cell_h + 2, solo ? C_ACC : C_WHITE);
            cv_text(tx, y + 1, &FONT_S, lbl, C_BLACK);
        } else {
            cv_rect(1, y + 1, (uint32_t)lbl_w, cell_h + 2, C_WHITE);
            cv_rect(3, y + 3, (uint32_t)(lbl_w - 4u), cell_h - 2, C_BLACK);
            cv_text(tx, y + 1, &FONT_S, lbl, C_WHITE);
        }
    } else if (!muted) {
        cv_rect(1, y + 1, (uint32_t)lbl_w, cell_h + 2, lcol);
        cv_text(tx, y + 1, &FONT_S, lbl, C_BLACK);
    } else {
        cv_text(tx, y + 1, &FONT_S, lbl, C_DIM);
        cv_line(2, y + 2 + cell_h / 2, 1 + lbl_w - 1, y + 2 + cell_h / 2, C_DIM);
    }
}

/* cells: nlanes x FM_SEQ_PAGE_STEPS (the page on screen); play_step / plock_step are page-relative,
 * >= FM_SEQ_PAGE_STEPS = not on this page */
static inline void fm_seq_draw_grid(uint8_t nlanes, uint8_t sel_lane, uint8_t play_step, int playing,
                                    uint8_t plock_step, int plock_latched, uint8_t plock_lane,
                                    uint16_t play_col, const char *const *lane_lbls,
                                    const uint16_t *lane_lbl_cols, const fm_seq_cell_t *cells,
                                    const uint8_t *play_step_by_lane,
                                    const int8_t *play_timing_by_lane,
                                    const uint8_t *play_lane_outline)
{
    int32_t y0;
    int32_t row_h;
    int32_t cell_h;
    uint32_t lane;
    uint32_t st;

    if (nlanes == 8u) {
        y0 = FM_SEQ_GRID8_Y0;
        row_h = FM_SEQ_GRID8_ROW_H;
        cell_h = FM_SEQ_GRID8_CELL_H;
    } else if (nlanes == 6u) {
        y0 = 4;
        row_h = 20;
        cell_h = 14;
    } else if (nlanes == 4u) {
        y0 = 10;
        row_h = 28;
        cell_h = 20;
    } else {
        y0 = 2;
        row_h = (int32_t)(128 / (nlanes > 0u ? nlanes : 1u));
        cell_h = row_h - 5;
        if (cell_h < 8)
            cell_h = 8;
    }
    if (cells == 0)
        return;
    for (lane = 0; lane < (uint32_t)nlanes; lane++) {
        int32_t y = y0 + (int32_t)lane * row_h;
        uint16_t lcol = lane_lbl_cols ? lane_lbl_cols[lane] : C_GRAY;
        int muted = lcol == C_DIM;
        int solo = lcol == C_ACC;
        int all_sel = sel_lane >= nlanes;
        int selected = all_sel || lane == (uint32_t)sel_lane;
        uint16_t edge_col = muted ? C_GRAY : C_WHITE;

        if (selected) {
            cv_rect(0, y, 240, cell_h + 4, RGB(18, 22, 42));
            if (all_sel) {
                if (lane == 0u)
                    cv_line(0, y, 239, y, C_WHITE);
                if (lane == (uint32_t)nlanes - 1u)
                    cv_line(0, y + cell_h + 3, 239, y + cell_h + 3, C_WHITE);
            } else {
                cv_line(0, y, 239, y, edge_col);
                cv_line(0, y + cell_h + 3, 239, y + cell_h + 3, edge_col);
            }
        }
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            int32_t x = FM_SEQ_GRID_X0 + (int32_t)st * 13 + (int32_t)(st / 4u) * 3;
            uint32_t ix = lane * FM_SEQ_PAGE_STEPS + st;
            const fm_seq_cell_t *c = &cells[ix];

            if (!plock_latched && plock_step < FM_SEQ_PAGE_STEPS && lane == (uint32_t)sel_lane &&
                st == (uint32_t)plock_step)
                cv_rect(x - 1, y + 1, 13, cell_h + 2, C_ACC);
            cv_rect(x, y + 2, 11, cell_h, c->bg_col);
        }
        /* Note bodies after every cell background, so a span is not wiped by the next step. */
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            int32_t x = FM_SEQ_GRID_X0 + (int32_t)st * 13 + (int32_t)(st / 4u) * 3;
            uint32_t ix = lane * FM_SEQ_PAGE_STEPS + st;
            const fm_seq_cell_t *c = &cells[ix];
            int32_t dx;
            int32_t bw;
            int32_t bx;

            if (c->vel > 0u && c->col != 0u && !c->note_tail) {
                int32_t bh = 3 + ((int32_t)c->vel * (cell_h - 3)) / 127;
                int32_t dy = 0;
                int32_t by;
                int32_t cl_tune = c->tune;
                int32_t yp;
                uint16_t pline;

                dx = ((int32_t)c->micro * 4) / FM_SEQ_MICRO_HALF;
                bx = x + (dx == 0 ? 0 : (1 + dx));
                if (c->note_span > 1u) {
                    int32_t end_x = fm_seq_grid_step_x((int32_t)st + (int32_t)c->note_span);

                    bw = end_x - bx;
                    if (bw < 11)
                        bw = 11;
                } else if (c->gate < 0) {
                    bw = 11 + ((int32_t)c->gate * 8) / 63;
                    if (bw < 3)
                        bw = 3;
                } else if (c->gate > 0) {
                    bw = 11 + ((int32_t)c->gate * 2) / 63;
                    if (bw > 13)
                        bw = 13;
                } else
                    bw = (dx == 0) ? 11 : 9;
                if (bx + bw > 240)
                    bw = 240 - bx;
                if (bw < 1)
                    bw = 1;
                if (c->tune != 0) {
                    dy = ((int32_t)c->tune * (cell_h - 3)) / 24;
                    if (dy < -(cell_h - 3))
                        dy = -(cell_h - 3);
                    if (dy > cell_h - 3)
                        dy = cell_h - 3;
                }
                if (cl_tune < -24)
                    cl_tune = -24;
                if (cl_tune > 24)
                    cl_tune = 24;
                by = y + 2 + cell_h - bh - dy;
                if (by < y + 2)
                    by = y + 2;
                cv_rect(bx, by, bw, bh, c->col);
                if (c->cond > 0u) {
                    if (bh >= 6)
                        cv_rect(bx + 2, by + 2, bw - 4, bh - 4, c->bg_col);
                    else if (bh >= 4)
                        cv_rect(bx + 3, by + 1, bw - 6, bh - 2, c->bg_col);
                }
                if (c->tune != 0) {
                    yp = y + 2 + ((24 - cl_tune) * (cell_h - 2)) / 48;
                    pline = (c->col == C_WHITE) ? C_ACC : C_WHITE;
                    cv_rect(bx, yp, bw, 2, pline);
                }
            }
            if (plock_latched && plock_lane < nlanes && lane == (uint32_t)plock_lane &&
                plock_step < FM_SEQ_PAGE_STEPS && st == (uint32_t)plock_step) {
                int32_t fx = x - 2;
                int32_t fy = y;
                int32_t fw = 15;
                int32_t fh = cell_h + 4;

                cv_rect(fx, fy, fw, 2, C_WHITE);
                cv_rect(fx, fy + fh - 2, fw, 2, C_WHITE);
                cv_rect(fx, fy, 2, fh, C_WHITE);
                cv_rect(fx + fw - 2, fy, 2, fh, C_WHITE);
            }
        }
        {
            uint8_t lane_play = play_step_by_lane ? play_step_by_lane[lane] : play_step;

            if (playing && lane_play < FM_SEQ_PAGE_STEPS) {
                int32_t tun = play_timing_by_lane ? (int32_t)play_timing_by_lane[lane] : 0;
                int32_t pos_q = (int32_t)lane_play * FM_SEQ_TIMING_UNITS + tun;
                int32_t cell = pos_q / FM_SEQ_TIMING_UNITS;
                int32_t rem = pos_q - cell * FM_SEQ_TIMING_UNITS;
                int32_t x0;
                int32_t x1;
                int32_t play_x;

                if (rem < 0) {
                    cell--;
                    rem += FM_SEQ_TIMING_UNITS;
                }
                if (cell < 0 || cell >= (int32_t)FM_SEQ_PAGE_STEPS)
                    continue;
                x0 = fm_seq_grid_step_x(cell);
                x1 = (cell + 1 < (int32_t)FM_SEQ_PAGE_STEPS) ? fm_seq_grid_step_x(cell + 1) : x0 + 13;
                play_x = x0 - 1 + (rem * (x1 - x0)) / FM_SEQ_TIMING_UNITS;
                if (play_lane_outline && play_lane_outline[lane]) {
                    int32_t px = play_x;
                    int32_t py = y + 1;
                    int32_t pw = 13;
                    int32_t ph = cell_h + 2;

                    cv_rect(px, py, pw, 2, play_col);
                    cv_rect(px, py + ph - 2, pw, 2, play_col);
                    cv_rect(px, py, 2, ph, play_col);
                    cv_rect(px + pw - 2, py, 2, ph, play_col);
                } else
                    cv_rect(play_x, y + 1, 13, cell_h + 2, play_col);
            }
        }
        if (lane_lbls && lane_lbls[lane])
            fm_seq_draw_lane_lbl(y, cell_h, selected, muted, solo, lcol, lane_lbls[lane]);
    }
}
#endif /* FM_SEQ_UI_H */

#endif /* FM_SEQ_H */
