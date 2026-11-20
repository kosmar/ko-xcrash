/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DECK voice map, panel keys, MIDI notes (freestanding). */
#ifndef DECK_KIT_H
#define DECK_KIT_H

#include <stdint.h>

#define DECK_NVOICES 16u
#define DECK_ERA_COUNT 6u /* STAX..TRAP — matches deck_plates DECK_ERA_N */

/* Kick, Snare, Rim, CH, OH, Clap, Cabasa, Tamb, Cowbell, Tom H/M/L, Conga H/L, Ride, Crash */
enum {
    DECK_BD  = 0,
    DECK_SN  = 1,
    DECK_RM  = 2,
    DECK_CH  = 3,
    DECK_OH  = 4,
    DECK_CP  = 5,
    DECK_CB  = 6,
    DECK_TB  = 7,
    DECK_CW  = 8,
    DECK_TH  = 9,
    DECK_TM  = 10,
    DECK_TL  = 11,
    DECK_CNH = 12,
    DECK_CNL = 13,
    DECK_RD  = 14,
    DECK_CR  = 15
};

static const uint8_t KEY_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t KEY_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

#define DECK_NPADS 11u

/* Black pads L→R: KK SN RM CH OH CP SHAKER CW TOM CYM CONGA — family pads cycle on retap. */
#define DECK_PAD_SHAKER 6u
#define DECK_PAD_TOM    8u
#define DECK_PAD_CYM    9u
#define DECK_PAD_CONGA  10u

static inline int deck_pad_is_family(uint8_t pad)
{
    pad %= DECK_NPADS;
    return pad == DECK_PAD_SHAKER || pad == DECK_PAD_TOM || pad == DECK_PAD_CYM || pad == DECK_PAD_CONGA;
}

static inline uint8_t deck_pad_family_default(uint8_t pad)
{
    switch (pad % DECK_NPADS) {
    case DECK_PAD_SHAKER:
        return DECK_CB;
    case DECK_PAD_TOM:
        return DECK_TM;
    case DECK_PAD_CYM:
        return DECK_CR;
    case DECK_PAD_CONGA:
        return DECK_CNH;
    default:
        return 255u;
    }
}

static inline uint8_t deck_pad_family_next(uint8_t pad, uint8_t cur)
{
    cur %= DECK_NVOICES;
    switch (pad % DECK_NPADS) {
    case DECK_PAD_SHAKER:
        return cur == DECK_CB ? DECK_TB : DECK_CB;
    case DECK_PAD_TOM:
        if (cur == DECK_TM)
            return DECK_TH;
        if (cur == DECK_TH)
            return DECK_TL;
        return DECK_TM;
    case DECK_PAD_CYM:
        return cur == DECK_CR ? DECK_RD : DECK_CR;
    case DECK_PAD_CONGA:
        return cur == DECK_CNH ? DECK_CNL : DECK_CNH;
    default:
        return cur;
    }
}

static inline uint8_t deck_pad_resolve(uint8_t pad, uint8_t edit, uint8_t home)
{
    (void)edit;
    (void)home;
    switch (pad % DECK_NPADS) {
    case 0u:
        return DECK_BD;
    case 1u:
        return DECK_SN;
    case 2u:
        return DECK_RM;
    case 3u:
        return DECK_CH;
    case 4u:
        return DECK_OH;
    case 5u:
        return DECK_CP;
    case DECK_PAD_SHAKER:
        return DECK_CB;
    case 7u:
        return DECK_CW;
    case DECK_PAD_TOM:
        return DECK_TM;
    case DECK_PAD_CYM:
        return DECK_CR;
    case DECK_PAD_CONGA:
        return DECK_CNH;
    default:
        return DECK_BD;
    }
}

static inline int deck_pad_voice_in_family(uint8_t pad, uint8_t voice)
{
    voice %= DECK_NVOICES;
    switch (pad % DECK_NPADS) {
    case DECK_PAD_SHAKER:
        return voice == DECK_CB || voice == DECK_TB;
    case DECK_PAD_TOM:
        return voice == DECK_TH || voice == DECK_TM || voice == DECK_TL;
    case DECK_PAD_CYM:
        return voice == DECK_CR || voice == DECK_RD;
    case DECK_PAD_CONGA:
        return voice == DECK_CNH || voice == DECK_CNL;
    default:
        return voice == deck_pad_resolve(pad, 0u, 0u);
    }
}

/* ENC_SELECT order: black pads L→R, family alts match deck_pad_family_next cycles. */
static const uint8_t DECK_SELECT_ORDER[DECK_NVOICES] = {
    DECK_BD, DECK_SN, DECK_RM, DECK_CH, DECK_OH, DECK_CP, DECK_CB, DECK_TB, DECK_CW,
    DECK_TM, DECK_TH, DECK_TL, DECK_CR, DECK_RD, DECK_CNH, DECK_CNL
};

static inline uint8_t deck_select_order_index(uint8_t voice)
{
    uint32_t i;

    voice %= DECK_NVOICES;
    for (i = 0u; i < DECK_NVOICES; i++) {
        if (DECK_SELECT_ORDER[i] == voice)
            return (uint8_t)i;
    }
    return 0u;
}

static inline uint8_t deck_select_voice_step(uint8_t cur, int32_t dir)
{
    int32_t idx = (int32_t)deck_select_order_index(cur) + (dir > 0 ? 1 : -1);

    while (idx < 0)
        idx += (int32_t)DECK_NVOICES;
    return DECK_SELECT_ORDER[(uint8_t)((uint32_t)idx % DECK_NVOICES)];
}

/* GM-ish note per voice (same order as enum). */
static const uint8_t DECK_MIDI_NOTE[DECK_NVOICES] = {
    36u, 38u, 37u, 42u, 46u, 39u, 70u, 54u, 56u, 50u, 47u, 45u, 62u, 64u, 51u, 49u
};

/* Tune editable: Snare, 3 toms, 2 congas. */
#define DECK_TUNE_OK                                                                               \
    ((1u << DECK_SN) | (1u << DECK_TH) | (1u << DECK_TM) | (1u << DECK_TL) | (1u << DECK_CNH) |    \
     (1u << DECK_CNL))

static const char DECK_VNAME_LONG[DECK_NVOICES][9] = {
    "KICK", "SNARE", "RIM", "CL HAT", "OP HAT", "CLAP", "CABASA", "TAMB",
    "COWBELL", "TOM HI", "TOM MID", "TOM LO", "CONGA HI", "CONGA LO", "RIDE", "CRASH"
};

/* Pad map order; SEQ lane chips = FM_SEQ_LANE_LBL_W (20 px) on DECK. */
static const char DECK_VNAME_SHORT[DECK_NVOICES][3] = {
    "BD", "SN", "RM", "CH", "OH", "CP", "CA", "TA", "CW", "HT", "MT", "LT",
    "HI", "LO", "RD", "CR"
};

enum { DECK_MUTE_GRP_N = 5u };
/* perf.mute_grp → deck_perf_mute_mask; 0 = no group mute. */
static const char *const DECK_MUTE_GRP_NAMES[DECK_MUTE_GRP_N] = {
    "OFF", "HATS", "TOMS", "PERC", "CYM"
};

/* Lane colour by family as r, g, b (RGB() lives in gfx.c; deck_vcol() in deck_ui.h packs it). */
static const uint8_t DECK_VCOL[DECK_NVOICES][3] = {
    {255u, 90u, 60u},   {255u, 190u, 60u}, {255u, 230u, 120u}, {90u, 220u, 255u},
    {60u, 170u, 255u},  {255u, 140u, 90u}, {190u, 255u, 110u}, {150u, 230u, 80u},
    {255u, 170u, 40u},  {120u, 255u, 150u}, {80u, 220u, 120u}, {50u, 180u, 95u},
    {255u, 120u, 200u}, {220u, 90u, 170u}, {130u, 150u, 255u}, {180u, 140u, 255u}
};

static uint8_t deck_voice_from_midi(uint8_t note)
{
    uint32_t v;

    for (v = 0; v < DECK_NVOICES; v++) {
        if (DECK_MIDI_NOTE[v] == note)
            return (uint8_t)v;
    }
    return 255u;
}

/* KO KIT GLO KEY / SCALE: KEY 0..11 (C..B), SCALE 0..7 = KO BASS names MINOR..HARM.M (semis = KO BASS table).
 * Table rows are zero-padded; 0 = tonic, always in scale. Persisted in deck_meta_t, snapped in deck_seq.h. */
#define DECK_SCALE_N   8u
#define DECK_SCALE_DEG 7u
static const char *const DECK_NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static const char *const DECK_SCALE_NAMES[DECK_SCALE_N] = {
    "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const uint8_t DECK_SCALE_SEMIS[DECK_SCALE_N][DECK_SCALE_DEG] = {
    {0, 2, 3, 5, 7, 8, 10},
    {0, 2, 3, 5, 7, 9, 10},
    {0, 1, 3, 5, 7, 8, 10},
    {0, 2, 4, 5, 7, 9, 11},
    {0, 2, 4, 5, 7, 9, 10},
    {0, 3, 5, 7, 10, 0, 0},
    {0, 3, 5, 6, 7, 10, 0},
    {0, 2, 3, 5, 7, 8, 11},
};

#endif /* DECK_KIT_H */
