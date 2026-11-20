/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DECK eras + factory plates. Era = decade family; plate = named kit within that decade. */
#ifndef DECK_PLATES_H
#define DECK_PLATES_H

#include "deck_dsp.h"

enum {
    DECK_ERA_STAX = 0, /* 60s — dry, woody, motown/stax */
    DECK_ERA_DEAD,     /* 70s — tight studio/disco, normal kit */
    DECK_ERA_GATE,     /* 80s — gated, bright, click */
    DECK_ERA_CRATE,    /* 90s — dusty hip-hop / sample */
    DECK_ERA_GRID,     /* 00s — clean digital / quantized */
    DECK_ERA_TRAP,     /* 10s — short punch, 808-ish */
    DECK_ERA_N
};

#define DECK_PLATES_PER_ERA 10u
#define DECK_PLATE_N (DECK_ERA_N * DECK_PLATES_PER_ERA)

static const char *const DECK_ERA_NAME[DECK_ERA_N] = {
    "STAX", "DEAD", "GATE", "CRATE", "GRID", "TRAP"
};

/* HOME pg0 knob labels (chrome only — values come from deck.c). */
static const char *const DECK_HOME_P0_LBL[4] = {"TUNE", "DECAY", "FILTER", "CRUSH"};

/* Era colour — same role as GENRE_ERA_COL on KO BASS. */
static const uint8_t DECK_ERA_RGB[DECK_ERA_N][3] = {
    {210u, 160u, 90u},  {255u, 200u, 70u}, {255u, 70u, 180u},
    {185u, 130u, 95u},  {100u, 170u, 255u}, {70u, 230u, 140u}
};

typedef struct {
    uint8_t tune, decay, filter, crush, drive, noise, click, hold;
    uint8_t m_drive, m_crush, m_filter, m_width, m_dust, m_tape, m_comp;
    uint8_t bpm, swing;
} deck_era_base_t;

/* Era base: voice offsets (64 = unity; crush is a unipolar amount). m_filter is centre-64 (64 = open); the
 * DJ filter darkens towards 0 (62 ≈ 18 kHz, 56 ≈ 11 kHz, 52 ≈ 8.3 kHz, 0 ≈ 150 Hz) and goes highpass above 65. */
static const deck_era_base_t DECK_ERA_BASE[DECK_ERA_N] = {
    /* STAX / 60s: warm, dry, low dirt */ {58, 52, 52, 0, 48, 50, 36, 6, 12, 0, 52, 26, 0, 2, 2, 100, 100},
    /* DEAD / 70s: close-mic studio — short, dry, the normal kit */ {64, 50, 60, 0, 54, 54, 46, 2, 16, 0, 54, 14, 0, 0, 4, 100, 102},
    /* GATE / 80s: bright, modest drive, snare gate via ENV */ {64, 46, 70, 0, 52, 60, 58, 0, 22, 6, 56, 12, 0, 0, 6, 100, 104},
    /* CRATE / 90s: dark boom, light dust/crush */ {56, 60, 48, 14, 50, 62, 40, 8, 24, 10, 52, 24, 8, 8, 6, 96, 96},
    /* GRID / 00s: clean, even, crush off */ {64, 48, 64, 0, 46, 36, 56, 0, 12, 0, 64, 16, 0, 0, 10, 102, 104},
    /* TRAP / 10s: tune down, tight hats, light master */ {48, 58, 54, 2, 56, 38, 44, 4, 18, 0, 52, 10, 0, 0, 6, 100, 106}
};

typedef struct {
    uint8_t dly_lvl, dly_time, feedback, rev_mix, rev_damp, rev_tail, dly_rev, tank;
} deck_era_fx_t;

/* Send bus — session-dry defaults; plates/perf can push wetter. */
static const deck_era_fx_t DECK_ERA_FX[DECK_ERA_N] = {
    /* STAX */ {0, 58, 10, 12, 76, 48, 0, 22},
    /* DEAD */ {0, 62, 6, 0, 66, 36, 0, 10},
    /* GATE */ {4, 60, 12, 6, 58, 32, 0, 14},
    /* CRATE */ {0, 62, 8, 6, 72, 40, 0, 10},
    /* GRID */ {4, 58, 10, 8, 62, 38, 0, 18},
    /* TRAP */ {6, 54, 10, 4, 74, 34, 0, 12}
};

typedef struct {
    uint8_t sustain;
    uint8_t release;
    int8_t pitch_env;
} deck_era_sn_env_t;

static const deck_era_sn_env_t DECK_ERA_SN_ENV[DECK_ERA_N] = {
    {76, 58, -4}, /* STAX */
    {70, 52, 0},  /* DEAD */
    {22, 26, 6},  /* GATE — short sustain = gate */
    {66, 46, -6}, /* CRATE */
    {72, 54, 0},  /* GRID */
    {44, 30, 10}  /* TRAP — snappy */
};

/* Per-voice decay/tune/timbre deltas after shared era+plate HOME macros. */
static const int8_t DECK_ERA_VDECAY[DECK_ERA_N][DECK_NVOICES] = {
    {-10, -4, -2, -14, -12, 0, -10, -10, -8, -8, -8, -10, -8, -10, 8, -4},
    {-12, -6, 0, -12, -14, -6, -6, -6, -6, -4, -4, -6, -6, -8, 6, -6},
    {0, -4, 0, -8, -6, -2, -4, -4, -4, -4, -4, -4, -4, -6, 4, -8},
    {8, 0, 2, -10, -12, 0, -6, -6, -6, -4, -4, -4, -4, -6, 6, -10},
    {0, 0, 0, -6, -6, 0, -2, -2, -2, 0, 0, 0, 0, -2, 4, -4},
    {10, -2, 0, -22, -20, -4, -16, -16, -14, -10, -10, -12, -12, -14, -4, -10}
};

static const int8_t DECK_ERA_VTUNE[DECK_ERA_N][DECK_NVOICES] = {
    {-6, 0, 0, -4, -6, 0, -2, -2, 0, 0, 0, 0, 0, -4, -6, -6},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 2, 0, 4, 6, 0, 2, 2, 4, 0, 0, 0, 0, 2, 4, 4},
    {-4, 0, 0, -2, -2, 0, 0, 0, 0, -2, -2, -2, -2, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {-10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

typedef struct {
    char name[8];
    uint8_t era;
    int8_t tune, decay, filter, crush, drive;
    int8_t noise, click, hold;
    int8_t v_bd, v_sn, v_rm, v_ch, v_oh, v_cp;
    int8_t v_sh, v_tom, v_cn, v_cym; /* CA/TB/CW · toms · congas · RD/CR */
    uint8_t bpm, swing;
} deck_plate_t;

/* v_sh/v_tom/v_cn/v_cym default 0 — era seed in deck_plate_apply fills quiet perc unless plate offsets. */
#define DPL(n, er, tu, de, fi, cr, dr, no, cl, ho, bd, sn, rm, ch, oh, cp, bp, sw)                 \
    { n, er, tu, de, fi, cr, dr, no, cl, ho, bd, sn, rm, ch, oh, cp, 0, 0, 0, 0, bp, sw }

static const deck_plate_t DECK_PLATE[DECK_PLATE_N] = {
    /* STAX / 60s */
    DPL("SNAP", 0, -14, -20, 10, 0, 14, -22, 20, 0, 22, -12, 0, -28, -18, -10, 128, 50),
    DPL("TICK", 0, 8, -8, -6, 4, 6, 12, -18, 4, -8, 12, 20, 20, 18, -8, 130, 54),
    DPL("DRY", 0, -6, -4, 14, 0, -12, -24, 8, 0, 16, -8, -12, -20, -24, 14, 126, 50),
    DPL("RIM", 0, 10, -16, -8, 4, 14, -8, 12, 0, -6, 8, 20, -14, -10, 6, 132, 52),
    DPL("BOX", 0, -10, -12, -14, 4, 14, 6, 20, 6, 20, -12, -6, -28, -22, -14, 124, 50),
    DPL("CHIP", 0, 6, -10, -10, 4, 14, 12, 20, 0, -8, -12, 14, 20, 18, -18, 136, 50),
    DPL("PUNT", 0, 4, -6, 6, 0, 10, -14, 20, 0, 18, 16, -10, -26, -16, 22, 128, 58),
    DPL("TOCK", 0, -12, -18, 4, 8, -6, -24, -20, 6, 8, 20, 20, -18, -8, 18, 122, 50),
    DPL("TAC", 0, 16, -22, 16, 4, 8, -18, 20, 0, -4, 24, 20, -28, -28, 22, 134, 52),
    DPL("PIN", 0, 12, -24, 16, 8, -10, -24, -20, 6, 6, 24, 20, -28, -28, 20, 124, 50),
    /* DEAD / 70s */
    DPL("GATE", 1, -8, -18, 6, 10, 12, 8, 14, 0, 14, 18, -8, 20, 16, -12, 118, 54),
    DPL("SLAM", 1, 6, -14, -4, 12, 18, 16, 22, 4, 20, 22, -12, 18, 12, 8, 120, 56),
    DPL("CRACK", 1, 10, -22, 8, 6, 16, -6, 24, 0, 8, 24, 10, 20, 18, -6, 116, 52),
    DPL("ROOM", 1, -4, -6, -8, 0, -8, -12, -10, 0, 12, -6, 6, -16, -18, 10, 114, 58),
    DPL("WHIP", 1, 14, -24, 12, 8, 18, 4, 24, 0, 16, 20, 14, 20, 18, -14, 122, 54),
    DPL("CLIP", 1, -6, -16, 4, 12, 18, 16, 18, 4, 18, 14, -10, 20, 18, 4, 118, 50),
    DPL("SNAP2", 1, 4, -20, -6, 12, 10, -8, 8, 0, 10, 16, 18, 20, 14, -8, 120, 54),
    DPL("THWAK", 1, -10, -8, -12, 12, 18, 12, 24, 4, 22, 24, 8, 16, 10, 12, 116, 56),
    DPL("HARD", 1, 8, -26, 10, 4, 14, -16, 24, 0, 6, 24, 12, 20, 18, -10, 124, 52),
    DPL("AIR", 1, -12, -4, -14, 0, -10, -20, -16, 0, -8, -12, 4, -22, -26, 16, 112, 58),
    /* GATE / 80s — short gate, bright; plates vary snare bite vs hat splash */
    DPL("DEEP", 2, 4, -10, 8, 4, 10, 8, 8, 8, 18, 14, -6, -12, -8, -4, 92, 58),
    DPL("SUB", 2, 2, -12, 6, 0, 8, 6, 6, 8, 16, 10, -8, -14, -10, -6, 88, 60),
    DPL("FAT", 2, 6, -6, 10, 6, 14, 12, 10, 8, 20, 18, -4, -8, -4, 6, 94, 56),
    DPL("THUD", 2, -2, -8, 4, 0, 12, 10, 12, 8, 22, 16, 2, -10, -6, 4, 90, 62),
    DPL("BODY", 2, 8, -4, 8, 4, 16, 14, 8, 6, 18, 20, 0, -6, -2, 10, 96, 58),
    DPL("WOOL", 2, 4, -10, 6, 8, 6, 4, 4, 8, 14, 8, -6, -16, -12, -4, 86, 56),
    DPL("KICK", 2, 0, -6, 0, 0, 18, 10, 14, 8, 20, 12, -2, -8, -4, 0, 98, 56),
    DPL("PRESS", 2, -4, -8, 6, 4, 18, 14, 10, 8, 16, 22, 4, -4, 0, 12, 92, 60),
    DPL("LOW", 2, 4, -12, 4, 0, 6, 2, 6, 8, 12, 6, -8, -12, -10, -8, 84, 62),
    DPL("WARM", 2, 6, -6, 8, 4, 10, 6, 6, 6, 18, 16, 2, -6, -2, 4, 94, 58),
    /* CRATE / 90s — boom, dark, dusty; plates tilt crush vs kick length */
    DPL("ICE", 3, 6, 6, -10, 8, -6, 8, -8, 4, 12, 12, 10, 14, 12, -8, 92, 62),
    DPL("GLASS", 3, 4, 6, -12, 6, -4, 10, -10, 4, 8, 14, 12, 16, 14, -6, 90, 64),
    DPL("BELL", 3, 8, 6, -8, 4, -8, 6, -6, 0, 6, 16, 14, 12, 10, -10, 86, 60),
    DPL("METAL", 3, 2, 6, -14, 10, 10, 4, -4, 0, 10, 14, 12, 18, 16, -4, 94, 58),
    DPL("SHINE", 3, 6, 6, -6, 6, -8, 12, -12, 0, 14, 10, 8, 14, 12, -12, 88, 62),
    DPL("EDGE", 3, 4, 6, -16, 12, 12, 2, -2, 0, 8, 18, 14, 20, 18, -2, 95, 66),
    DPL("RING", 3, 10, 4, -6, 4, -8, 4, -8, 0, 4, 20, 16, 10, 8, -14, 92, 60),
    DPL("NEEDLE", 3, 6, 6, -12, 14, 14, 0, 0, 0, 6, 22, 18, 20, 18, 0, 85, 58),
    DPL("PEAK", 3, 8, 6, -8, 8, 6, 6, -6, 0, 12, 12, 10, 16, 14, -4, 90, 64),
    DPL("FLASH", 3, 4, 6, -10, 6, 8, 8, -8, 0, 10, 16, 12, 18, 16, -6, 94, 62),
    /* GRID / 00s — clean variants; light crush only on GRIT/ASH */
    DPL("TAPE", 4, -6, 2, 4, 0, 4, -6, 4, 4, 6, 4, 2, -4, -4, 4, 122, 52),
    DPL("GRIT", 4, -4, 0, 2, 8, 8, -4, 6, 2, 8, 8, 4, -2, -2, 6, 124, 54),
    DPL("FOG", 4, -8, 4, 6, 6, 2, -8, 2, 4, 4, 2, 0, -6, -8, 2, 120, 60),
    DPL("ASH", 4, -2, -2, 0, 8, 6, -2, 8, 0, 10, 10, 6, 0, 0, 8, 126, 52),
    DPL("MUD", 4, -10, 2, 8, 6, 4, -6, 0, 4, 2, 0, 0, -8, -10, 0, 120, 62),
    DPL("VHS", 4, -4, 0, 2, 6, 6, -4, 4, 2, 6, 6, 4, -4, -6, 4, 128, 58),
    DPL("RUST", 4, 0, -4, -2, 8, 8, 0, 6, 0, 12, 12, 8, 2, 0, 10, 120, 64),
    DPL("SMOKE", 4, -6, 4, 8, 4, 0, -8, 0, 4, 0, -2, -2, -10, -12, -2, 130, 60),
    DPL("WORN", 4, -8, 0, 4, 6, 6, -4, 2, 2, 4, 4, 2, -4, -6, 2, 122, 62),
    DPL("LOFT", 4, -4, -2, 6, 0, 0, -6, -4, 0, -2, -4, -4, -12, -14, -4, 124, 58),
    /* TRAP / 10s — tight hats vs 808 kick bloom on named plates */
    DPL("OPEN", 5, -6, 8, 4, 0, -6, -10, -8, 8, 8, -4, -6, -12, 16, 4, 108, 52),
    DPL("SWELL", 5, -4, 8, 2, 4, -4, -8, -6, 8, 6, -6, -8, -10, 18, 6, 104, 54),
    DPL("BLOOM", 5, -8, 8, 6, 4, -8, -12, -10, 8, 10, -2, -4, -14, 14, 2, 110, 50),
    DPL("HAZE", 5, -10, 8, 0, 6, -2, -6, -8, 8, 4, -8, -10, -8, 18, 8, 100, 56),
    DPL("WIDE", 5, -2, 6, 8, 0, -10, -14, -12, 8, 12, -4, -6, -16, 18, 0, 112, 52),
    DPL("TAIL", 5, -6, 8, 2, 6, -6, -8, -6, 8, 2, -10, -12, -6, 18, 10, 102, 54),
    DPL("AIRY", 5, -4, 4, 10, 0, -10, -14, -14, 8, 14, -6, -8, -18, 16, -2, 114, 50),
    DPL("SHEEN", 5, -2, 8, 6, 4, -6, -10, -8, 8, 8, -4, -6, -12, 18, 4, 106, 52),
    DPL("DRIFT", 5, -8, 8, -2, 8, -4, -6, -6, 8, 4, -8, -10, -10, 18, 6, 98, 56),
    DPL("HALO", 5, -4, 8, 4, 6, -8, -12, -10, 8, 10, -2, -4, -14, 18, 2, 108, 54)
};

#undef DPL

static inline uint8_t deck_clamp_era(uint8_t era)
{
    return era >= DECK_ERA_N ? (uint8_t)(DECK_ERA_N - 1u) : era;
}

static inline uint8_t deck_clamp_plate(uint8_t pl)
{
    return pl >= DECK_PLATE_N ? (uint8_t)(DECK_PLATE_N - 1u) : pl;
}

static inline uint8_t deck_clamp_swing_u8(uint8_t sw)
{
    if (sw < 50u)
        return 50u;
    if (sw > 75u)
        return 75u;
    return sw;
}

static inline uint8_t deck_era_of_plate(uint8_t pl)
{
    return DECK_PLATE[deck_clamp_plate(pl)].era;
}

static inline uint8_t deck_era_first_plate(uint8_t era)
{
    return (uint8_t)(deck_clamp_era(era) * DECK_PLATES_PER_ERA);
}

static inline uint8_t deck_plate_slot(uint8_t pl)
{
    return (uint8_t)(deck_clamp_plate(pl) % DECK_PLATES_PER_ERA);
}

static inline uint8_t deck_plate_in_era(uint8_t pl, uint8_t era)
{
    return deck_clamp_plate((uint8_t)(deck_era_first_plate(era) + deck_plate_slot(pl)));
}

static inline uint16_t deck_era_col(uint8_t era)
{
    const uint8_t *c = DECK_ERA_RGB[deck_clamp_era(era)];

    return RGB(c[0], c[1], c[2]);
}

static inline const char *deck_era_name(uint8_t era)
{
    return DECK_ERA_NAME[deck_clamp_era(era)];
}

static inline const char *deck_plate_name(uint8_t pl)
{
    return DECK_PLATE[deck_clamp_plate(pl)].name;
}

static inline uint8_t deck_sat_i8(int32_t v)
{
    if (v < 0)
        return 0u;
    if (v > 127)
        return 127u;
    return (uint8_t)v;
}

static inline int8_t deck_plate_voice_lvl(int8_t base, int8_t delta)
{
    int32_t v = (int32_t)base + (int32_t)delta;

    if (v < 0)
        return 0;
    if (v > 127)
        return 127;
    return (int8_t)v;
}

/* Factory groove of plate `pl` voiced in `era` (plate value, era base where the plate has none). */
static inline void deck_plate_groove(uint8_t pl, uint8_t era, uint8_t *bpm, uint8_t *swing)
{
    const deck_plate_t *p = &DECK_PLATE[deck_clamp_plate(pl)];
    const deck_era_base_t *e = &DECK_ERA_BASE[deck_clamp_era(era)];

    if (bpm)
        *bpm = p->bpm ? p->bpm : e->bpm;
    if (swing)
        *swing = deck_clamp_swing_u8(p->swing ? p->swing : e->swing);
}

static inline void deck_plate_apply(deck_dsp_t *s, uint8_t *bpm, uint8_t *swing, uint8_t pl, uint8_t era_for_base)
{
    const deck_plate_t *p;
    const deck_era_base_t *e;
    const deck_era_fx_t *fx;
    const deck_era_sn_env_t *sn_env;
    uint8_t v, era;

    if (!s)
        return;
    p = &DECK_PLATE[deck_clamp_plate(pl)];
    era = deck_clamp_era(era_for_base);
    s->era = era;
    e = &DECK_ERA_BASE[era];
    fx = &DECK_ERA_FX[era];
    sn_env = &DECK_ERA_SN_ENV[era];
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_voice_cfg_default(&s->cfg[v], v);
        s->cfg[v].tune = deck_sat_i8((int32_t)s->cfg[v].tune + (int32_t)e->tune - 64 + p->tune +
                                     (int32_t)DECK_ERA_VTUNE[era][v]);
        s->cfg[v].decay = deck_sat_i8((int32_t)s->cfg[v].decay + (int32_t)e->decay - 64 + p->decay +
                                      (int32_t)DECK_ERA_VDECAY[era][v]);
        s->cfg[v].filter = deck_sat_i8((int32_t)s->cfg[v].filter + (int32_t)e->filter - 64 + p->filter);
        s->cfg[v].crush = deck_sat_i8((int32_t)s->cfg[v].crush + (int32_t)e->crush + p->crush);
        s->cfg[v].drive = deck_sat_i8((int32_t)s->cfg[v].drive + (int32_t)e->drive - 64 + p->drive);
        s->cfg[v].noise = deck_sat_i8((int32_t)s->cfg[v].noise + (int32_t)e->noise - 64 + p->noise);
        s->cfg[v].click = deck_sat_i8((int32_t)e->click + (int32_t)p->click);
        s->cfg[v].hold = deck_sat_i8((int32_t)e->hold + (int32_t)p->hold);
        deck_era_voice_apply(&s->cfg[v], v, era);
        if (v == DECK_SN) {
            s->cfg[v].sustain = sn_env->sustain;
            s->cfg[v].release = sn_env->release;
            s->cfg[v].pitch_env =
                deck_sat_i8((int32_t)s->cfg[v].pitch_env + (int32_t)sn_env->pitch_env);
        } else {
            s->cfg[v].sustain = 0u;
            s->cfg[v].release = 64u;
        }
        s->env_invert[v] = (era == DECK_ERA_GATE && v == DECK_SN) ? 1u : 0u;
    }
    {
        uint8_t pslot = deck_plate_slot(pl);
        int32_t ktb = (int32_t)s->cfg[DECK_BD].timbre;

        if (era == DECK_ERA_GATE) {
            /* FAT, BODY, KICK, PRESS — allow FM/808 click; rest stay sine-ish */
            if (pslot != 2u && pslot != 4u && pslot != 6u && pslot != 7u) {
                if (ktb > 64)
                    ktb = 64;
            } else if (ktb < 70)
                ktb += 10;
        } else if (ktb > 64)
            ktb = 64;
        s->cfg[DECK_BD].timbre = deck_sat_i8(ktb);
    }
    s->cfg[DECK_BD].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_BD].level, p->v_bd);
    s->cfg[DECK_SN].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_SN].level, p->v_sn);
    s->cfg[DECK_RM].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_RM].level, p->v_rm);
    s->cfg[DECK_CH].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CH].level, p->v_ch);
    s->cfg[DECK_OH].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_OH].level, p->v_oh);
    s->cfg[DECK_CP].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CP].level, p->v_cp);
    {
        static const int8_t DECK_ERA_PERC_LVL[DECK_ERA_N][4] = {
            {12, 2, 10, -8},  /* STAX — conga/tamb */
            {8, 4, 6, -6},    /* DEAD — tom/cowbell */
            {6, 10, 2, 2},    /* GATE — electric toms */
            {10, 6, 8, -8},   /* CRATE — sparse perc */
            {2, 2, 0, -4},    /* GRID — minimal */
            {4, 8, 0, -8},    /* TRAP — 808 tom lane */
        };
        const int8_t *ep = DECK_ERA_PERC_LVL[era];
        int8_t sh = (int8_t)(ep[0] + p->v_sh);
        int8_t tom = (int8_t)(ep[1] + p->v_tom);
        int8_t cn = (int8_t)(ep[2] + p->v_cn);
        int8_t cym = (int8_t)(ep[3] + p->v_cym);

        s->cfg[DECK_CB].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CB].level, sh);
        s->cfg[DECK_TB].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_TB].level, (int8_t)(sh - 4));
        s->cfg[DECK_CW].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CW].level, (int8_t)(sh - 2));
        s->cfg[DECK_TM].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_TM].level, tom);
        s->cfg[DECK_TH].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_TH].level, (int8_t)(tom - 2));
        s->cfg[DECK_TL].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_TL].level, (int8_t)(tom - 4));
        s->cfg[DECK_CNH].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CNH].level, cn);
        s->cfg[DECK_CNL].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CNL].level, (int8_t)(cn - 6));
        s->cfg[DECK_RD].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_RD].level, cym);
        s->cfg[DECK_CR].level = (uint8_t)deck_plate_voice_lvl((int8_t)s->cfg[DECK_CR].level, (int8_t)(cym - 4));
    }
    /* Era/plate deviation lives in the voice cfg only; HOME macros start neutral. */
    s->perf.tune = 64u;
    s->perf.decay = 64u;
    s->perf.filter = 64u;
    s->perf.crush = 64u;
    deck_master_default(&s->master);
    s->master.drive = e->m_drive;
    s->master.crush = e->m_crush;
    s->master.filter = e->m_filter;
    s->master.width = e->m_width;
    s->master.dust = e->m_dust;
    s->master.tape = e->m_tape;
    s->master.comp = e->m_comp;
    s->master.dly_lvl = fx->dly_lvl;
    s->master.dly_time = fx->dly_time;
    s->master.feedback = fx->feedback;
    s->master.rev_mix = fx->rev_mix;
    s->master.rev_damp = fx->rev_damp;
    s->master.rev_tail = fx->rev_tail;
    s->master.dly_rev = fx->dly_rev;
    s->master.tank = fx->tank;
    deck_plate_groove(pl, era, bpm, swing);
}

#endif /* DECK_PLATES_H */
