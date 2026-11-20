/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef DUB_DATA_H
#define DUB_DATA_H

#include "dub_types.h"
#include "genre_conf.h"
#include "fm_seq.h"

static const char *const DUB_ERA_NAMES[DUB_ERA_N] = {
    "KINGSTON", "LOVERS", "CHANNEL", "BRISTOL", "CROYDON", "POSTDUB"
};

enum {
    DUB_HAT_TICK = 0, DUB_HAT_SIZZLE, DUB_HAT_TR6, DUB_HAT_TONE, DUB_HAT_HISS
};

/* Per-kit hat voice (YARD … ANVIL). Kick/rim tables stay in dub_dsp.h. */
static const int8_t dub_hat_note[DKIT_N] = {
    79, 84, 88, 96, 94, 81, 86, 104
};
static const uint8_t dub_hat_fm[DKIT_N] = {
     8, 32, 14, 42, 52,  6, 18, 88
};
static const uint8_t dub_hat_noise[DKIT_N] = {
    60, 38, 28, 34, 22, 14, 56, 48
};
static const uint8_t dub_hat_metal[DKIT_N] = {
    36, 88, 76, 98, 68, 112, 44, 118
};
static const uint8_t dub_hat_decay[DKIT_N] = {
    17,  9,  5,  7,  6, 14,  4,  3
};
static const uint8_t dub_hat_open[DKIT_N] = {
     7, 14,  6, 22,  8, 18,  5, 12
};
static const uint8_t dub_hat_style[DKIT_N] = {
    DUB_HAT_HISS, DUB_HAT_SIZZLE, DUB_HAT_TICK, DUB_HAT_SIZZLE,
    DUB_HAT_TR6, DUB_HAT_TONE, DUB_HAT_HISS, DUB_HAT_SIZZLE
};
static const uint8_t dub_hat_cut[DKIT_N] = {
    40, 48, 56, 48, 46, 44, 34, 52
};

static const dub_plate_t DUB_PLATES[DUB_PLATE_N] = {
{ "MIXROOT",    0, BASS_808,    VOICE_MIN7,  0, 10,  72,  106, 44, 34, 46, 44,  8, 16, 0x0100, 0x0100, 0x4444, 0x0441, 0x4444, 0x4000 },
{ "SOUNDHW",    0, BASS_SUB,    VOICE_MIN7,  0, 14,  78,  114, 36, 28, 40, 50,  0, 12, 0x1111, 0x1010, 0x4444, 0x1111, 0x4001, 0x0000 },
{ "DUBWIZ",    0, BASS_808,    VOICE_MIN9,  2,  8,  68,   98, 52, 48, 54, 36, 10, 14, 0x0100, 0x1100, 0x1111, 0x1081, 0x4410, 0x0040 },
{ "ROCKIT",   0, BASS_SUB,    VOICE_SUS4,  1, 18,  80,  112, 34, 26, 38, 48,  6, 20, 0x0501, 0x0010, 0x5555, 0x0909, 0x8010, 0x2000 },
{ "LABRAT", 0, BASS_808,    VOICE_MIN7,  2,  6,  75,  102, 50, 42, 52, 76, 12, 18, 0x0101, 0x0410, 0x4444, 0x0505, 0x0108, 0x8000 },
{ "RIDDIMX",    0, BASS_FM,     VOICE_FIFTH, 0, 22,  84,  118, 30, 24, 34, 58, 18, 14, 0x0505, 0x1010, 0x5555, 0x3333, 0x0840, 0x0000 },
    { "YARDONE", 0, BASS_SUB,    VOICE_MIN7,  1, 16,  76,  108, 38, 30, 42, 46,  4, 18, 0x0101, 0x0010, 0x4444, 0x8101, 0x4104, 0x0040 },
    { "VERZION",   0, BASS_808,    VOICE_MIN9,  0, 12,  82,  116, 42, 32, 44, 52,  6, 16, 0x1110, 0x0100, 0x5555, 0x9090, 0x1010, 0x8000 },
    { "ISLAND", 0, BASS_SUB,    VOICE_MIN7,  1, 16,  62,  88, 50, 42, 42, 46,  4, 18, 0x0101, 0x0010, 0x0101, 0x8101, 0x4104, 0x0040 },
    { "KINGST",   0, BASS_808,    VOICE_MIN9,  0, 12,  68,  96, 54, 44, 44, 52,  6, 16, 0x1110, 0x0100, 0x0101, 0x9090, 0x1010, 0x8000 },
{ "SWEETIE",  1, BASS_SUB,    VOICE_MIN9,  1, 24,  68,   82, 58, 52, 62, 38,  4,  8, 0x0100, 0x0010, 0x2222, 0x0401, 0x4010, 0x0040 },
{ "RUBADUB",  1, BASS_808,    VOICE_MIN7,  0, 28,  72,   78, 62, 56, 66, 42,  2,  6, 0x0101, 0x0000, 0x1111, 0x1081, 0x0108, 0x0000 },
{ "MOONLIT",  1, BASS_SUB,    VOICE_SUS4,  2, 32,  66,   74, 64, 58, 68, 36,  6, 10, 0x0100, 0x0100, 0x2222, 0x0909, 0x4410, 0x0040 },
{ "SLOWJAM",  1, BASS_808,    VOICE_MIN9,  1, 26,  70,   76, 60, 54, 64, 40,  4,  8, 0x0101, 0x0010, 0x1111, 0x0505, 0x1010, 0x8000 },
{ "ROMANCE",  1, BASS_SUB,    VOICE_MIN7,  0, 30,  74,   80, 56, 50, 60, 44,  8, 12, 0x0100, 0x0000, 0x2222, 0x8101, 0x4104, 0x0000 },
{ "HEARTFX",  1, BASS_808,    VOICE_SUS4,  2, 22,  64,   72, 66, 60, 70, 34,  6, 14, 0x0101, 0x0100, 0x1111, 0x0401, 0x0840, 0x0040 },
{ "SOFTREG",  1, BASS_SUB,    VOICE_MIN7,  1, 34,  76,   84, 54, 48, 58, 46,  4, 10, 0x0100, 0x0010, 0x2222, 0x1111, 0x4001, 0x0000 },
{ "WHISPER",  1, BASS_808,    VOICE_MIN9,  0, 20,  62,   70, 68, 62, 72, 32, 10, 16, 0x0101, 0x0100, 0x1111, 0x9090, 0x8010, 0x8000 },
{ "BALLAD",  1, BASS_SUB,    VOICE_MIN7,  1, 34,  62,   64, 66, 60, 58, 46,  4, 10, 0x0100, 0x0010, 0x0101, 0x1111, 0x4001, 0x0000 },
{ "LULLABY",  1, BASS_808,    VOICE_MIN9,  0, 20,  60,   58, 80, 74, 72, 32, 10, 16, 0x0101, 0x0100, 0x0101, 0x9090, 0x8010, 0x8000 },
{ "MONOCLR", 2, BASS_SUB,    VOICE_MIN7,  2, 92,  124, 100, 54, 48, 52, 74, 14, 34, 0x1111, 0x0100, 0x4444, 0x0181, 0x0401, 0x0000 },
{ "PHASER",    2, BASS_REESE,  VOICE_MIN9,  4, 104, 128, 106, 52, 54, 56, 78, 18, 40, 0x1111, 0x1010, 0x5555, 0x1041, 0x4104, 0x0000 },
{ "DEEPVAC",  2, BASS_PWM,    VOICE_QUART, 2,  88, 132,  96, 48, 44, 48, 68, 26, 36, 0x1111, 0x1000, 0xAAAA, 0x0401, 0x1041, 0x8000 },
{ "CARIBX",  2, BASS_SUB,    VOICE_MIN7,  3,  80, 120, 102, 44, 42, 46, 56, 14, 30, 0x0909, 0x0100, 0x4444, 0x0101, 0x1010, 0x4000 },
{ "LOOPFIL",  2, BASS_303,    VOICE_SUS4,  1,  96, 130, 108, 46, 40, 50, 72, 34, 42, 0x1111, 0x0808, 0xCCCC, 0x2424, 0x0820, 0x0000 },
{ "QUADRANT", 2, BASS_PWM,    VOICE_MIN9,  5, 110, 122,  94, 56, 58, 54, 62, 16, 32, 0x1111, 0x0010, 0x4444, 0x0209, 0x2002, 0x0000 },
    { "RAWLINE",   2, BASS_SUB,    VOICE_MIN7,  3, 90,  126,  98, 56, 50, 50, 70, 12, 38, 0x1111, 0x0100, 0x4444, 0x0001, 0x0104, 0x0000 },
    { "INCNT",      2, BASS_PWM,    VOICE_MIN9,  4,100,  130, 104, 50, 52, 54, 76, 20, 44, 0x1111, 0x1000, 0x5555, 0x8108, 0x4010, 0x4000 },
    { "ECHOES",   2, BASS_SUB,    VOICE_MIN7,  3, 90,  120,  78, 68, 62, 50, 70, 12, 38, 0x1111, 0x0100, 0x0101, 0x0001, 0x0104, 0x0000 },
    { "SPACED",      2, BASS_PWM,    VOICE_MIN9,  4,100,  122, 84, 62, 64, 54, 76, 20, 44, 0x1111, 0x1000, 0x0101, 0x8108, 0x4010, 0x4000 },
{ "WISDOM",    3, BASS_808,    VOICE_MIN9,  4, 40,  98, 110, 42, 34, 44, 52, 14, 52, 0x4141, 0x0808, 0x2222, 0x1449, 0x1010, 0x4000 },
{ "ECHOCVR",  3, BASS_REESE,  VOICE_SUS4,  2, 54,  100, 100, 38, 38, 42, 48, 22, 48, 0x1101, 0x4090, 0x4444, 0x2121, 0x0408, 0x0000 },
{ "REDNEON",  3, BASS_PWM,    VOICE_MIN7,  0, 46,  103, 120, 32, 28, 36, 58, 10, 58, 0x1111, 0x1010, 0xAAAA, 0x1091, 0x4100, 0x0000 },
{ "SPECTRE",  3, BASS_WG,     VOICE_QUART, 3, 36,  97,  92, 46, 42, 46, 44, 14, 44, 0x0401, 0x4848, 0x5555, 0x4141, 0x0808, 0x8000 },
{ "BIGROOM",  3, BASS_808,    VOICE_MIN7,  2, 50,   78,  98, 48, 44, 50, 40, 12, 26, 0x0405, 0x1010, 0x5555, 0x0903, 0x0404, 0x0000 },
{ "GRIP",    3, BASS_SUB,    VOICE_FIFTH, 4, 60,  108,  96, 44, 40, 48, 70, 28, 38, 0x0801, 0x0100, 0x2222, 0x0901, 0x1004, 0x2000 },
    { "SLIPPY",  3, BASS_REESE,  VOICE_SUS4,  3, 48,  96, 104, 40, 32, 40, 50, 16, 50, 0x1101, 0x0208, 0x2222, 0x1212, 0x0202, 0x4000 },
    { "SUBDEEP",     3, BASS_808,    VOICE_MIN9,  2, 52,  99, 112, 36, 30, 38, 54, 12, 46, 0x0401, 0x1010, 0x4444, 0x1411, 0x2010, 0x0000 },
    { "FOGGED",  3, BASS_REESE,  VOICE_SUS4,  3, 48,  87, 84, 52, 44, 40, 50, 16, 50, 0x1101, 0x0208, 0x0101, 0x1212, 0x0202, 0x4000 },
    { "RUSTED",     3, BASS_808,    VOICE_MIN9,  2, 52,  90, 92, 48, 42, 38, 54, 12, 46, 0x0401, 0x1010, 0x0101, 0x1411, 0x2010, 0x0000 },
{ "SUBMAL",     4, BASS_WOBBLE, VOICE_FIFTH, 0, 78,  140,  84, 36, 30, 42, 36, 76, 24, 0x0401, 0x0100, 0x1010, 0x0101, 0x1001, 0x0004 },
{ "SKRECH",   4, BASS_WOBBLE, VOICE_MIN7,  2, 72,  138,  90, 40, 28, 44, 40, 88, 22, 0x0101, 0x0500, 0x5044, 0x0109, 0x0010, 0x4000 },
{ "DEEPMK",     4, BASS_FM,     VOICE_SUS4,  1, 68,  140,  94, 34, 24, 38, 52, 96, 28, 0x0201, 0x0100, 0x4444, 0x0503, 0x0100, 0x0000 },
{ "SPARTN",   4, BASS_808,    VOICE_FIFTH, 0, 84,  140,  76, 42, 36, 46, 30, 32, 32, 0x0001, 0x0100, 0x0404, 0x0104, 0x0020, 0x8000 },
{ "HALFST",    4, BASS_303,    VOICE_MIN7,  3, 66,  139, 102, 32, 26, 36, 56, 82, 26, 0x0501, 0x0110, 0x5555, 0x1311, 0x0410, 0x0000 },
{ "SUBCODE",    4, BASS_SUB,    VOICE_QUART, 4, 88,  136,  82, 46, 42, 48, 74, 40, 20, 0x0801, 0x0108, 0x1111, 0x0901, 0x0040, 0x0000 },
    { "BASSBIN",     4, BASS_WOBBLE, VOICE_FIFTH, 1, 70,  140,  80, 38, 26, 40, 34, 80, 26, 0x0101, 0x0100, 0x0404, 0x0801, 0x0001, 0x0000 },
    { "FLUX",    4, BASS_FM,     VOICE_MIN7,  2, 74,  138,  88, 36, 22, 42, 48, 90, 24, 0x0801, 0x0100, 0x5050, 0x0305, 0x0008, 0x8000 },
    { "DEEPHT",     4, BASS_WOBBLE, VOICE_FIFTH, 1, 70,  138,  60, 50, 38, 40, 34, 80, 26, 0x0101, 0x0100, 0x0101, 0x0801, 0x0001, 0x0000 },
    { "GHOST",    4, BASS_FM,     VOICE_MIN7,  2, 74,  136,  68, 48, 34, 42, 48, 90, 24, 0x0801, 0x0100, 0x0101, 0x0305, 0x0008, 0x8000 },
{ "TECTON",   5, BASS_WG,     VOICE_QUART, 5, 86,  100,  88, 52, 46, 50, 48, 24, 16, 0x0421, 0x8080, 0x1111, 0x1081, 0x1089, 0x2000 },
{ "ROLLERS",  5, BASS_REESE,  VOICE_MIN9,  4, 94,   94,  82, 48, 50, 52, 44, 30, 20, 0x0841, 0x2010, 0x4222, 0x0901, 0x4204, 0x8000 },
{ "MISTY",   5, BASS_SUB,    VOICE_MIN9,  2, 98,  120,  94, 50, 52, 48, 38, 18, 24, 0x0201, 0x1010, 0x2929, 0x0109, 0x0401, 0x0000 },
{ "SHCKLN",  5, BASS_WG,     VOICE_SUS4,  3, 76,  107, 104, 44, 40, 48, 68, 22, 14, 0x0921, 0x4924, 0x1248, 0x0421, 0x2040, 0x4000 },
{ "PERCUK",     5, BASS_FM,     VOICE_QUART, 4, 82,  115,  90, 40, 36, 44, 78, 38, 28, 0x1081, 0x2402, 0x0909, 0x0211, 0x0802, 0x0000 },
{ "VAPOR",    5, BASS_PWM,    VOICE_MIN9,  5, 112,  72,  78, 58, 60, 56, 36, 16, 18, 0x0101, 0x0100, 0x1010, 0x0101, 0x0410, 0x0000 },
    { "CHOPS",  5, BASS_WG,     VOICE_QUART, 4, 88,  111,  86, 46, 44, 50, 52, 28, 22, 0x0841, 0x4040, 0x0909, 0x1240, 0x2080, 0x4000 },
    { "WIDEAK",  5, BASS_SUB,    VOICE_MIN9,  3,102,  115,  92, 48, 46, 46, 42, 22, 26, 0x0201, 0x8080, 0x2929, 0x0190, 0x0204, 0x0000 },
    { "AFTERM",  5, BASS_WG,     VOICE_QUART, 4, 88,  102,  66, 58, 56, 50, 52, 28, 22, 0x0841, 0x4040, 0x0808, 0x1240, 0x2080, 0x4000 },
    { "DRIFTG",  5, BASS_SUB,    VOICE_MIN9,  3,102,  106,  72, 60, 58, 46, 42, 22, 26, 0x0201, 0x8080, 0x0808, 0x0190, 0x0204, 0x0000 },

};

/* Bass melodic contour: era × in-era slot × 8-step scale degree (0..6). */
static const uint8_t DUB_BASS_WALK[DUB_ERA_N][GENRE_PRESETS_PER_ERA][8] = {
    { { 0, 4, 0, 3, 0, 4, 0, 0 }, { 0, 0, 4, 0, 0, 3, 0, 4 }, { 0, 3, 0, 0, 4, 0, 3, 0 }, { 0, 4, 0, 0, 3, 0, 4, 0 }, { 0, 0, 0, 4, 0, 3, 0, 0 }, { 0, 4, 0, 3, 0, 0, 4, 3 }, { 0, 0, 3, 0, 4, 0, 0, 3 }, { 0, 3, 0, 4, 0, 0, 0, 4 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 3, 0, 0, 4, 0, 0 }, { 0, 3, 0, 0, 4, 0, 0, 3 }, { 0, 0, 0, 3, 0, 0, 4, 0 }, { 0, 3, 0, 4, 0, 0, 0, 3 }, { 0, 0, 4, 0, 3, 0, 0, 0 }, { 0, 0, 0, 3, 0, 4, 0, 3 }, { 0, 3, 0, 0, 0, 4, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 3 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 4, 0, 0, 3, 0, 5 }, { 0, 4, 0, 0, 3, 0, 5, 0 }, { 0, 0, 3, 0, 0, 4, 0, 0 }, { 0, 3, 0, 4, 0, 0, 0, 3 }, { 0, 0, 0, 3, 0, 4, 0, 5 }, { 0, 5, 0, 0, 3, 0, 4, 0 }, { 0, 0, 4, 3, 0, 0, 0, 4 }, { 0, 3, 0, 0, 4, 0, 5, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 4, 0, 0, 3, 0, 4, 0 }, { 0, 0, 3, 0, 4, 0, 0, 3 }, { 0, 4, 0, 3, 0, 0, 4, 0 }, { 0, 0, 0, 4, 0, 3, 0, 0 }, { 0, 3, 0, 4, 0, 0, 3, 4 }, { 0, 4, 0, 0, 0, 3, 0, 4 }, { 0, 0, 3, 0, 0, 4, 0, 3 }, { 0, 3, 0, 0, 4, 0, 3, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 0, 0, 3, 0, 0 }, { 0, 0, 0, 0, 0, 0, 3, 0 }, { 0, 0, 0, 4, 0, 0, 0, 0 }, { 0, 0, 4, 0, 0, 0, 0, 3 }, { 0, 3, 0, 0, 0, 0, 4, 0 }, { 0, 0, 0, 0, 3, 0, 0, 4 }, { 0, 4, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 5, 0, 3, 0, 4, 0, 2 }, { 0, 3, 0, 5, 0, 2, 0, 4 }, { 0, 0, 3, 0, 5, 0, 4, 0 }, { 0, 4, 0, 0, 3, 5, 0, 0 }, { 0, 2, 0, 4, 0, 0, 3, 5 }, { 0, 0, 4, 0, 2, 0, 5, 3 }, { 0, 5, 0, 2, 0, 4, 3, 0 }, { 0, 3, 0, 0, 4, 0, 5, 2 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } }

};

#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM plate at dub_gen.slot. */
static fm_pgen_t dub_gen = { .slot = 0xFFu };

#define FM_PGEN_STATE dub_gen
#define FM_PGEN_BUILD(era, n, c) dub_gen_build(era, n, c)

static inline uint8_t dub_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&dub_gen, ix);
}

static inline uint8_t dub_clamp_plate(uint8_t ix)
{
    return ix >= DUB_PLATE_N ? (uint8_t)(DUB_PLATE_N - 1u) : ix;
}

static inline uint8_t dub_clamp_era(uint8_t era)
{
    return era >= DUB_ERA_N ? (uint8_t)(DUB_ERA_N - 1u) : era;
}

static inline const char *dub_era_name(uint8_t era)
{
    return DUB_ERA_NAMES[dub_clamp_era(era)];
}

static inline const char *dub_plate_name(uint8_t plate)
{
    if (dub_gen_hit(plate))
        return dub_gen.name;
    return DUB_PLATES[dub_clamp_plate(plate)].name;
}

static inline const dub_plate_t *dub_plate(uint8_t ix)
{
    if (dub_gen_hit(ix))
        return &dub_gen.pl;
    return &DUB_PLATES[dub_clamp_plate(ix)];
}

static inline uint8_t dub_era_of_plate(uint8_t plate)
{
    plate = dub_clamp_plate(plate);
    return (uint8_t)(plate / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t dub_era_first_plate(uint8_t era)
{
    return (uint8_t)(dub_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint8_t dub_bass_walk_deg(uint8_t era, uint8_t plate, uint8_t step)
{
    era = dub_clamp_era(era);
    plate = (uint8_t)(dub_clamp_plate(plate) % GENRE_PRESETS_PER_ERA);
    return DUB_BASS_WALK[era][plate][step & 7u];
}

static inline uint8_t dub_step_bit(uint16_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & 15u)) & 1u);
}

static inline uint8_t dub_drm_siren_dly_from_bus(uint8_t bus)
{
    uint32_t v = 96u + ((uint32_t)bus >> 1);

    return v > 140u ? 140u : (uint8_t)v;
}

static inline uint8_t dub_drm_kick_rev_from_bus(uint8_t bus)
{
    return (uint8_t)(48u + ((uint32_t)bus >> 1));
}

static inline void dub_drm_set_bus_dly(dub_params_t *p, uint8_t bus)
{
    uint8_t i;

    if (!p)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dly[i] = bus;
    p->drm_dly[3] = dub_drm_siren_dly_from_bus(bus);
}

static inline void dub_drm_voice_kit_fill(dub_params_t *p, uint8_t tune, uint8_t decay, uint8_t drive)
{
    uint8_t i;
    uint8_t t = tune ? tune : 64u;
    uint8_t d = decay ? decay : 64u;

    if (!p)
        return;
    for (i = 0; i < 4u; i++) {
        p->drm_tune[i] = t;
        p->drm_dec[i] = d;
        p->drm_drv[i] = drive;
    }
}

static inline void dub_drm_set_bus_rev(dub_params_t *p, uint8_t bus)
{
    uint8_t rv;
    uint8_t i;

    if (!p)
        return;
    rv = dub_drm_kick_rev_from_bus(bus);
    for (i = 0; i < 3u; i++)
        p->drm_rev[i] = rv;
    p->drm_rev[3] = 96u;
}

/* If per-voice sends are unset (old preset), seed from a shared bus level. */
static inline void dub_drm_send_fixup(dub_params_t *p)
{
    uint8_t i;
    uint8_t dnz = 0u;
    uint8_t rnz = 0u;

    if (!p)
        return;
    for (i = 0; i < 4u; i++) {
        if (p->drm_dly[i])
            dnz = 1u;
        if (p->drm_rev[i])
            rnz = 1u;
    }
    if (!dnz)
        dub_drm_set_bus_dly(p, 44u);
    if (!rnz)
        dub_drm_set_bus_rev(p, 44u);
}

static inline void dub_apply_defaults(dub_params_t *p)
{
    p->timbre = 28;
    p->cutoff_c = 84;
    p->decay_c = 88;
    p->reso_mode = 18;
    p->env_filt = 72;
    p->cutoff_b = 78;
    p->decay_b = 96;
    p->sub_oct = 0;
    p->drive = 16;
    p->dly_b = 16;
    p->lvl_b = 96;
    p->lvl_c = 92;
    p->rev_b = 24;
    p->rev_c = 28;
    p->atk_c = 0;
    p->glide_c = 0;
    p->atk_b = 0;
    p->siren_pitch = 48;
    p->siren_rate = 40;
    p->kick_boom = 76;
    p->hat_lvl = 46;
    p->rim_tone = 42;
    dub_drm_set_bus_dly(p, 44u);
    dub_drm_set_bus_rev(p, 44u);
    p->dly_time = 64;
    p->dly_lvl = 96;
    p->rev_mix = 34;
    p->rev_damp = 48;
    p->rev_tail = 72;
    p->dly_rev = 48;
    p->wow = 14;
    p->isolator = 64;
    p->lfo1_wave = DUB_LFO_SINE;
    p->lfo1_rate = 4;
    p->lfo1_depth = 0;
    p->lfo1_tgt = DUB_LFO_CUT_C;
    p->lfo2_wave = DUB_LFO_SINE;
    p->lfo2_rate = 4;
    p->lfo2_depth = 0;
    p->lfo2_tgt = DUB_LFO_WOB;
    p->arp_mode = 0;
    p->root = 48;
    p->scale = 0;
    p->swing = 50;
    p->drum_kit = 0;
    dub_drm_voice_kit_fill(p, 64u, 64u, 16u);
    {
        uint8_t di;
        for (di = 0; di < 4u; di++) {
            p->drm_cut[di] = 64u;
            p->drm_atk[di] = 0u;
        }
    }
    p->pan[0] = 64; /* SUB center */
    p->pan[1] = 50; /* CHORD slightly L */
    p->pan[2] = 68; /* DRUM slightly R */
    p->pan[3] = 82; /* SIREN wider R */
    p->drm_pan[0] = 64; /* KICK center */
    p->drm_pan[1] = 74; /* RIM slightly R */
    p->drm_pan[2] = 52; /* HAT slightly L */
    p->drm_pan[3] = 82; /* SIREN wider R */
    dub_drm_send_fixup(p);
}

typedef struct {
    uint8_t cutoff_c, decay_c, timbre, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator;
} dub_era_base_t;

static const dub_era_base_t DUB_ERA_BASE[DUB_ERA_N] = {
    /* KINGSTON */
    { 78u, 94u, 34u, 14u, 70u,  72u, 102u, 10u, 12u, 22u,
      76u, 42u, 48u, 58u, 54u, 12u, 40u, 36u, 52u, {64u, 72u, 48u, 80u},
      24u, 58u, 56u },
    /* LOVERS */
    { 82u, 108u, 40u, 10u, 74u,  74u, 112u, 14u, 10u, 28u,
      58u, 36u, 42u, 60u, 62u, 14u, 38u, 34u, 44u, {64u, 70u, 52u, 76u},
      20u, 64u, 58u },
    /* CHANNEL */
    { 70u, 98u, 26u, 28u, 64u,  66u, 108u, 32u, 16u, 18u,
      64u, 32u, 40u, 62u, 58u, 10u, 36u, 32u, 38u, {64u, 68u, 56u, 70u},
      8u, 68u, 62u },
    /* BRISTOL */
    { 74u, 90u, 30u, 22u, 68u,  68u, 96u, 38u, 24u, 20u,
      92u, 52u, 56u, 56u, 68u, 28u, 48u, 44u, 46u, {64u, 76u, 50u, 74u},
      12u, 62u, 60u },
    /* CROYDON */
    { 68u, 82u, 22u, 34u, 58u,  62u, 88u, 72u, 34u, 14u,
      98u, 36u, 42u, 68u, 44u, 22u, 54u, 50u, 34u, {64u, 70u, 58u, 66u},
      6u, 52u, 54u },
    /* POSTDUB */
    { 82u, 104u, 36u, 20u, 76u,  76u, 110u, 18u, 14u, 26u,
      62u, 38u, 44u, 60u, 62u, 8u, 44u, 34u, 40u, {58u, 78u, 42u, 88u},
      18u, 74u, 52u },
};

static const uint8_t plate_roots[DUB_PLATE_N] = {55u, 53u, 57u, 50u, 52u, 48u, 54u, 56u, 60u, 62u, 52u, 55u, 50u, 54u, 57u, 53u, 56u, 51u, 62u, 57u, 48u, 55u, 50u, 52u, 54u, 58u, 49u, 54u, 55u, 60u, 53u, 58u, 55u, 51u, 48u, 54u, 51u, 48u, 57u, 54u, 53u, 54u, 51u, 52u, 55u, 49u, 52u, 55u, 58u, 61u, 49u, 56u, 57u, 50u, 53u, 51u, 47u, 51u, 53u, 57u};

static const uint8_t plate_scales[DUB_PLATE_N] = {6u, 6u, 7u, 6u, 6u, 5u, 6u, 7u, 12u, 13u, 6u, 7u, 6u, 7u, 6u, 6u, 7u, 7u, 13u, 13u, 1u, 1u, 0u, 1u, 0u, 2u, 1u, 0u, 7u, 6u, 2u, 1u, 0u, 3u, 6u, 4u, 2u, 1u, 8u, 7u, 0u, 0u, 2u, 6u, 0u, 3u, 0u, 2u, 6u, 8u, 3u, 1u, 7u, 4u, 3u, 8u, 3u, 7u, 9u, 13u};

static const uint8_t plate_swings[DUB_PLATE_N] = {56u, 58u, 54u, 55u, 53u, 57u, 57u, 56u, 63u, 62u, 58u, 60u, 56u, 59u, 57u, 55u, 58u, 56u, 64u, 62u, 50u, 51u, 52u, 50u, 53u, 51u, 51u, 52u, 57u, 58u, 56u, 58u, 54u, 60u, 55u, 57u, 59u, 56u, 65u, 62u, 50u, 51u, 50u, 52u, 49u, 51u, 51u, 50u, 57u, 56u, 52u, 54u, 55u, 53u, 56u, 58u, 57u, 56u, 63u, 62u};

static inline uint8_t dub_plate_root(uint8_t ix)
{
    return dub_gen_hit(ix) ? (uint8_t)dub_gen.root : plate_roots[dub_clamp_plate(ix)];
}

static inline uint8_t dub_plate_scale(uint8_t ix)
{
    return dub_gen_hit(ix) ? (uint8_t)dub_gen.scale : plate_scales[dub_clamp_plate(ix)];
}

static inline uint8_t dub_plate_swing(uint8_t ix)
{
    return dub_gen_hit(ix) ? (uint8_t)dub_gen.swing : plate_swings[dub_clamp_plate(ix)];
}

static const uint8_t plate_dly_times[DUB_PLATE_N] = {56u, 58u, 54u, 60u, 52u, 62u, 60u, 55u, 66u, 61u, 62u, 66u, 60u, 64u, 68u, 58u, 65u, 61u, 71u, 67u, 68u, 72u, 64u, 66u, 70u, 74u, 70u, 68u, 76u, 74u, 58u, 60u, 56u, 62u, 54u, 64u, 61u, 58u, 67u, 64u, 48u, 50u, 46u, 52u, 44u, 50u, 49u, 47u, 55u, 53u, 76u, 80u, 72u, 84u, 78u, 88u, 82u, 74u, 88u, 80u};

static const uint8_t plate_sub_oct[DUB_PLATE_N] = {8u, 12u, 6u, 14u, 10u, 16u, 11u, 9u, 17u, 15u, 10u, 8u, 12u, 9u, 11u, 7u, 10u, 8u, 16u, 14u, 28u, 32u, 24u, 30u, 26u, 34u, 30u, 27u, 36u, 33u, 42u, 48u, 38u, 44u, 52u, 40u, 46u, 41u, 52u, 47u, 78u, 88u, 92u, 68u, 96u, 72u, 85u, 90u, 91u, 96u, 22u, 18u, 14u, 20u, 16u, 12u, 19u, 16u, 25u, 22u};

static const uint8_t plate_drive[DUB_PLATE_N] = {12u, 10u, 14u, 11u, 13u, 15u, 12u, 14u, 18u, 20u, 10u, 8u, 12u, 9u, 11u, 9u, 10u, 8u, 16u, 14u, 16u, 18u, 14u, 17u, 20u, 15u, 17u, 19u, 23u, 25u, 24u, 26u, 22u, 28u, 30u, 25u, 27u, 23u, 33u, 29u, 32u, 36u, 38u, 28u, 34u, 30u, 33u, 35u, 39u, 41u, 14u, 12u, 10u, 16u, 18u, 11u, 13u, 11u, 19u, 17u};

static const uint8_t plate_cutoff_c[DUB_PLATE_N] = {76u, 80u, 74u, 78u, 72u, 82u, 77u, 79u, 83u, 85u, 80u, 82u, 78u, 81u, 83u, 79u, 80u, 77u, 86u, 83u, 66u, 64u, 68u, 70u, 62u, 66u, 67u, 65u, 73u, 71u, 72u, 70u, 74u, 68u, 76u, 72u, 73u, 71u, 79u, 77u, 64u, 66u, 62u, 68u, 60u, 64u, 65u, 63u, 71u, 69u, 86u, 84u, 88u, 90u, 82u, 92u, 87u, 85u, 93u, 91u};

static const uint8_t plate_cutoff_b[DUB_PLATE_N] = {70u, 74u, 68u, 72u, 66u, 76u, 71u, 73u, 77u, 79u, 74u, 76u, 72u, 75u, 77u, 73u, 74u, 71u, 80u, 77u, 62u, 60u, 64u, 58u, 56u, 62u, 61u, 59u, 67u, 65u, 64u, 66u, 62u, 68u, 70u, 64u, 67u, 65u, 73u, 71u, 58u, 56u, 54u, 60u, 52u, 58u, 57u, 55u, 63u, 61u, 78u, 80u, 74u, 82u, 76u, 84u, 79u, 77u, 85u, 83u};

static const uint8_t plate_decay_c[DUB_PLATE_N] = {96u, 92u, 98u, 94u, 90u, 88u, 95u, 97u, 101u, 103u, 104u, 108u, 102u, 106u, 110u, 100u, 105u, 101u, 111u, 107u, 102u, 104u, 100u, 98u, 106u, 102u, 103u, 105u, 109u, 111u, 88u, 86u, 90u, 84u, 82u, 88u, 89u, 87u, 95u, 93u, 78u, 80u, 76u, 74u, 82u, 78u, 79u, 77u, 85u, 83u, 110u, 112u, 108u, 114u, 106u, 116u, 111u, 109u, 117u, 115u};

static const uint8_t plate_decay_b[DUB_PLATE_N] = {104u, 100u, 106u, 98u, 102u, 96u, 99u, 101u, 105u, 107u, 112u, 116u, 110u, 114u, 118u, 108u, 113u, 109u, 119u, 115u, 112u, 114u, 110u, 108u, 116u, 112u, 113u, 115u, 119u, 121u, 94u, 92u, 96u, 90u, 88u, 94u, 93u, 91u, 99u, 97u, 84u, 86u, 82u, 80u, 88u, 84u, 85u, 83u, 91u, 89u, 114u, 116u, 112u, 118u, 110u, 120u, 115u, 113u, 121u, 119u};

static const uint8_t plate_timbre[DUB_PLATE_N] = {36u, 32u, 38u, 34u, 30u, 40u, 35u, 37u, 41u, 43u, 38u, 40u, 36u, 39u, 41u, 35u, 38u, 34u, 44u, 40u, 24u, 22u, 26u, 28u, 20u, 24u, 25u, 23u, 31u, 29u, 32u, 34u, 28u, 36u, 38u, 30u, 33u, 31u, 39u, 37u, 18u, 20u, 16u, 22u, 14u, 18u, 19u, 17u, 25u, 23u, 40u, 38u, 42u, 44u, 36u, 46u, 41u, 43u, 47u, 49u};

static const uint8_t plate_reso_mode[DUB_PLATE_N] = {12u, 14u, 10u, 16u, 12u, 18u, 13u, 15u, 19u, 21u, 10u, 12u, 8u, 14u, 11u, 9u, 10u, 8u, 16u, 14u, 32u, 34u, 30u, 28u, 36u, 32u, 33u, 35u, 39u, 41u, 24u, 26u, 22u, 28u, 30u, 24u, 25u, 27u, 31u, 33u, 38u, 40u, 42u, 36u, 44u, 38u, 39u, 41u, 45u, 47u, 18u, 16u, 20u, 14u, 22u, 12u, 19u, 17u, 25u, 23u};

static const uint8_t plate_kit_tune[DUB_PLATE_N] = {60u, 62u, 58u, 64u, 56u, 66u, 61u, 59u, 67u, 65u, 58u, 60u, 56u, 59u, 61u, 55u, 58u, 57u, 64u, 63u, 68u, 70u, 66u, 64u, 72u, 68u, 69u, 71u, 75u, 77u, 58u, 60u, 62u, 56u, 54u, 60u, 59u, 57u, 65u, 63u, 76u, 78u, 80u, 74u, 82u, 72u, 77u, 79u, 83u, 85u, 62u, 64u, 60u, 66u, 68u, 58u, 63u, 61u, 69u, 67u};

static const uint8_t plate_kit_decay[DUB_PLATE_N] = {62u, 64u, 58u, 66u, 60u, 56u, 63u, 61u, 69u, 67u, 64u, 66u, 62u, 65u, 68u, 60u, 64u, 63u, 70u, 69u, 66u, 68u, 64u, 62u, 60u, 58u, 67u, 65u, 73u, 71u, 70u, 72u, 68u, 74u, 76u, 70u, 71u, 69u, 77u, 75u, 44u, 42u, 40u, 46u, 38u, 48u, 45u, 43u, 51u, 49u, 68u, 72u, 74u, 70u, 66u, 78u, 69u, 71u, 75u, 77u};

static const uint8_t plate_siren_rate[DUB_PLATE_N] = {36u, 40u, 34u, 42u, 38u, 44u, 39u, 41u, 45u, 47u, 32u, 34u, 30u, 36u, 33u, 28u, 32u, 30u, 38u, 36u, 28u, 30u, 26u, 32u, 24u, 28u, 29u, 27u, 35u, 33u, 40u, 44u, 38u, 42u, 36u, 40u, 41u, 43u, 47u, 49u, 46u, 48u, 50u, 44u, 52u, 42u, 47u, 49u, 53u, 55u, 30u, 28u, 32u, 26u, 34u, 24u, 31u, 29u, 37u, 35u};

static const uint8_t plate_kick_boom[DUB_PLATE_N] = {70u, 74u, 68u, 76u, 72u, 78u, 75u, 77u, 81u, 83u, 56u, 58u, 54u, 57u, 59u, 52u, 56u, 54u, 62u, 60u, 62u, 64u, 60u, 58u, 66u, 62u, 63u, 61u, 69u, 67u, 88u, 92u, 86u, 94u, 98u, 90u, 90u, 88u, 96u, 94u, 96u, 100u, 92u, 98u, 88u, 94u, 97u, 95u, 103u, 101u, 60u, 58u, 64u, 56u, 62u, 54u, 59u, 57u, 65u, 63u};

static const uint8_t plate_rim_tone[DUB_PLATE_N] = {40u, 44u, 38u, 46u, 42u, 48u, 43u, 45u, 49u, 51u, 34u, 36u, 32u, 35u, 37u, 30u, 34u, 32u, 40u, 38u, 30u, 32u, 28u, 34u, 26u, 30u, 31u, 29u, 37u, 35u, 50u, 54u, 48u, 56u, 58u, 52u, 51u, 49u, 57u, 55u, 34u, 36u, 32u, 38u, 30u, 34u, 35u, 33u, 41u, 39u, 36u, 40u, 34u, 42u, 38u, 32u, 37u, 35u, 43u, 41u};

static const uint8_t plate_hat_lvl[DUB_PLATE_N] = {46u, 50u, 44u, 52u, 48u, 54u, 49u, 51u, 55u, 57u, 40u, 42u, 38u, 41u, 43u, 36u, 40u, 38u, 46u, 44u, 38u, 40u, 36u, 42u, 34u, 38u, 39u, 37u, 45u, 43u, 54u, 58u, 52u, 60u, 62u, 56u, 55u, 57u, 61u, 63u, 40u, 44u, 38u, 42u, 36u, 40u, 41u, 39u, 47u, 45u, 42u, 46u, 40u, 48u, 44u, 38u, 43u, 41u, 49u, 47u};

static const uint8_t plate_kit_drive[DUB_PLATE_N] = {10u, 12u, 8u, 14u, 11u, 16u, 11u, 13u, 17u, 19u, 8u, 6u, 10u, 7u, 9u, 6u, 8u, 6u, 14u, 12u, 14u, 16u, 12u, 18u, 20u, 15u, 15u, 17u, 21u, 23u, 32u, 36u, 28u, 40u, 44u, 34u, 33u, 31u, 39u, 37u, 24u, 28u, 22u, 30u, 20u, 26u, 25u, 23u, 31u, 29u, 8u, 10u, 6u, 12u, 14u, 7u, 9u, 7u, 15u, 13u};

static const uint8_t plate_siren_pitch[DUB_PLATE_N] = {42u, 46u, 38u, 48u, 40u, 50u, 43u, 45u, 49u, 51u, 36u, 38u, 34u, 37u, 39u, 32u, 36u, 34u, 42u, 40u, 32u, 36u, 28u, 34u, 26u, 30u, 33u, 31u, 39u, 37u, 44u, 48u, 40u, 52u, 54u, 46u, 45u, 47u, 51u, 53u, 50u, 54u, 46u, 56u, 48u, 52u, 51u, 53u, 57u, 59u, 36u, 40u, 32u, 44u, 38u, 28u, 37u, 35u, 43u, 41u};

static const uint8_t plate_rim_dly[DUB_PLATE_N] = {56u, 60u, 52u, 64u, 58u, 62u, 59u, 57u, 65u, 63u, 52u, 54u, 50u, 53u, 55u, 48u, 52u, 50u, 58u, 56u, 36u, 40u, 32u, 38u, 30u, 34u, 37u, 35u, 43u, 41u, 48u, 52u, 44u, 54u, 58u, 50u, 49u, 51u, 55u, 57u, 28u, 32u, 24u, 30u, 22u, 26u, 29u, 27u, 35u, 33u, 42u, 46u, 38u, 50u, 44u, 36u, 43u, 41u, 49u, 47u};

static const uint8_t plate_dly_b[DUB_PLATE_N] = {18u, 22u, 16u, 24u, 20u, 26u, 19u, 21u, 25u, 27u, 26u, 28u, 24u, 27u, 29u, 22u, 26u, 24u, 32u, 30u, 12u, 14u, 10u, 16u, 18u, 13u, 13u, 11u, 19u, 17u, 20u, 24u, 18u, 26u, 28u, 22u, 21u, 23u, 27u, 29u, 8u, 10u, 6u, 12u, 14u, 9u, 9u, 7u, 15u, 13u, 24u, 28u, 20u, 30u, 26u, 22u, 25u, 23u, 31u, 29u};

static const uint8_t plate_pan_k[DUB_PLATE_N] = {64u, 64u, 62u, 64u, 63u, 65u, 64u, 63u, 70u, 69u, 64u, 63u, 64u, 64u, 63u, 64u, 64u, 63u, 70u, 69u, 64u, 64u, 62u, 64u, 63u, 64u, 64u, 62u, 70u, 68u, 64u, 66u, 62u, 68u, 64u, 66u, 64u, 65u, 70u, 71u, 64u, 64u, 62u, 64u, 63u, 64u, 64u, 63u, 70u, 69u, 58u, 60u, 56u, 62u, 59u, 57u, 59u, 58u, 65u, 64u};

static const uint8_t plate_pan_r[DUB_PLATE_N] = {72u, 74u, 70u, 76u, 73u, 75u, 73u, 71u, 79u, 77u, 70u, 72u, 68u, 71u, 73u, 67u, 70u, 68u, 76u, 74u, 68u, 70u, 66u, 72u, 67u, 69u, 69u, 67u, 75u, 73u, 76u, 78u, 74u, 80u, 77u, 79u, 77u, 75u, 83u, 81u, 70u, 72u, 68u, 74u, 69u, 71u, 71u, 69u, 77u, 75u, 76u, 80u, 74u, 82u, 78u, 75u, 79u, 77u, 85u, 83u};

static const uint8_t plate_pan_h[DUB_PLATE_N] = {48u, 50u, 46u, 52u, 47u, 49u, 49u, 47u, 55u, 53u, 52u, 54u, 50u, 53u, 55u, 48u, 52u, 50u, 58u, 56u, 54u, 56u, 52u, 58u, 53u, 55u, 55u, 53u, 61u, 59u, 46u, 48u, 44u, 50u, 45u, 47u, 47u, 45u, 53u, 51u, 56u, 58u, 54u, 60u, 55u, 57u, 57u, 55u, 63u, 61u, 42u, 44u, 40u, 46u, 43u, 41u, 43u, 41u, 49u, 47u};

static const uint8_t plate_pan_s[DUB_PLATE_N] = {80u, 82u, 78u, 84u, 79u, 83u, 81u, 79u, 87u, 85u, 76u, 78u, 74u, 77u, 79u, 72u, 76u, 74u, 82u, 80u, 70u, 72u, 68u, 74u, 69u, 71u, 71u, 69u, 77u, 75u, 74u, 76u, 72u, 78u, 75u, 77u, 75u, 73u, 81u, 79u, 66u, 68u, 64u, 70u, 65u, 67u, 67u, 65u, 73u, 71u, 86u, 88u, 84u, 90u, 87u, 85u, 87u, 85u, 93u, 91u};

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} dub_era_fx_t;

static const dub_era_fx_t DUB_ERA_FX[DUB_ERA_N] = {
    { DKIT_ROOTS,  12u, 0u, 46u, 42u, 88u, 30u, 32u, 58u, 52u, 0u,  BASS_808,    0u },
    { DKIT_ARK,    36u, 1u, 58u, 48u, 92u, 40u, 72u, 88u, 68u, 0u,  BASS_SUB,    0u },
    { DKIT_909,    92u, 2u, 72u, 52u, 98u, 46u, 84u, 96u, 74u, 0u,  BASS_SUB,    0u },
    { DKIT_808,    44u, 4u, 54u, 44u, 84u, 36u, 68u, 66u, 44u, 52u, BASS_REESE,  0u },
    { DKIT_CROYDON,72u, 1u, 48u, 38u, 78u, 28u, 54u, 74u, 34u, 0u,  BASS_WOBBLE, 78u },
    { DKIT_METAL,  88u, 5u, 48u, 48u, 94u, 44u, 76u,104u, 82u, 0u,  BASS_WG,     0u },
};

static inline uint8_t dub_plate_param_u8(uint8_t plate_val, uint8_t era_val)
{
    return plate_val ? plate_val : era_val;
}

static inline void dub_era_apply_base(dub_params_t *p, uint8_t era)
{
    const dub_era_base_t *eb;
    const dub_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = dub_clamp_era(era);
    eb = &DUB_ERA_BASE[era];
    fx = &DUB_ERA_FX[era];
    p->era = era;
    p->timbre = eb->timbre;
    p->cutoff_c = eb->cutoff_c;
    p->decay_c = eb->decay_c;
    p->reso_mode = eb->reso_mode;
    p->env_filt = eb->env_filt;
    p->cutoff_b = eb->cutoff_b;
    p->decay_b = eb->decay_b;
    p->sub_oct = eb->sub_oct;
    p->drive = eb->drive;
    p->dly_b = eb->dly_b;
    p->kick_boom = eb->kick_boom;
    p->rim_tone = eb->rim_tone;
    p->hat_lvl = eb->hat_lvl;
    dub_drm_voice_kit_fill(p, eb->kit_tune, eb->kit_decay, eb->kit_drive);
    p->siren_pitch = eb->siren_pitch;
    p->siren_rate = eb->siren_rate;
    dub_drm_set_bus_dly(p, eb->rim_dly);
    dub_drm_set_bus_rev(p, eb->rim_dly);
    for (i = 0; i < 4u; i++)
        p->drm_pan[i] = eb->drm_pan[i];
    p->wow = eb->wow;
    p->dly_time = eb->dly_time;
    p->isolator = eb->isolator;
    p->tap = fx->tap;
    p->tank = fx->tank;
    p->loop_filt = fx->loop_filt;
    p->feedback = fx->feedback;
    p->dly_lvl = fx->dly_lvl;
    p->rev_mix = fx->rev_mix;
    p->rev_damp = fx->rev_damp;
    p->rev_tail = fx->rev_tail;
    p->dly_rev = fx->dly_rev;
    p->pump = fx->pump;
    p->wobble = fx->wobble;
    p->lvl_b = 96;
    p->lvl_c = 92;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

static inline uint8_t dub_mix_ep(uint8_t era_v, uint8_t plate_v)
{
    return (uint8_t)(((uint32_t)era_v + (uint32_t)plate_v) >> 1);
}

/* Per-preset KICK/RIM/HAT decay trims (drm_dec[0..2]), fitted so every factory voice keeps
 * its envelope length from before the quarter-octave drum decay (±15 %). Siren keeps kit_decay.
 * Column 3: open-hat length trim, 64 + hat_mul steps; read by the DSP via p->plate, never saved. */
static const uint8_t dub_drm_dec_fit[GENRE_PRESET_N][4] = {
    { 37,  40,  63, 65}, { 36,  53,  67, 64}, { 39,  42,  63, 65}, { 33,  52,  63, 65},
    { 37,  40,  71, 64}, { 32,  38,  71, 64}, { 36,  39,  71, 64}, { 35,  39,  63, 65},
    { 30,  37,  67, 64}, { 30,  34,  71, 64}, { 40,  46,  71, 64}, { 38,  44,  67, 64},
    { 43,  46,  68, 64}, { 40,  48,  68, 64}, { 40,  44,  67, 64}, { 43,  47,  67, 64},
    { 42,  46,  68, 64}, { 42,  46,  68, 63}, { 36,  45,  69, 64}, { 33,  44,  68, 64},
    { 36,  49,  68, 64}, { 34,  49,  68, 64}, { 34,  52,  69, 64}, { 31,  50,  67, 64},
    { 31,  52,  69, 64}, { 34,  49,  71, 64}, { 34,  49,  63, 65}, { 36,  52,  68, 64},
    { 43,  46,  67, 64}, { 45,  49,  67, 64}, { 19,  46,  71, 66}, { 18,  45,  96, 63},
    { 20,  48, 100, 63}, { 15,  43,  92, 64}, { 15,  43, 100, 63}, { 19,  45,  92, 64},
    { 19,  47,  96, 63}, { 20,  48,  67, 63}, { 16,  44, 100, 63}, { 15,  43,  63, 67},
    { 13,  45,  31, 68}, { 12,  44,  35, 64}, { 14,  46,  31, 68}, { 13,  43,  35, 64},
    { 18,  47,  32, 64}, { 12,  47,  36, 64}, { 14,  45,  35, 64}, { 15,  46,  39, 64},
    {  8,  42,  35, 64}, {  9,  44,  38, 64}, { 38,  43,  67, 64}, { 36,  44,  71, 64},
    { 46,  45,  31, 68}, { 38,  42,  67, 64}, { 35,  42,  63, 65}, { 50,  45,  68, 64},
    { 38,  43,  68, 64}, { 38,  60,  68, 64}, { 46,  57,  67, 64}, { 49,  58,  67, 64}
};
#define GENRE_DRM_DEC_FIT dub_drm_dec_fit

static inline void dub_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = dub_drm_dec_fit[preset][i];
}

static inline void dub_plate_apply(dub_params_t *p, uint8_t plate, uint8_t what)
{
    const dub_plate_t *pl;
    const dub_era_base_t *eb;
    const dub_era_fx_t *fx;
    uint8_t all;
    uint8_t keep_era;
    uint8_t plate_dly_lvl;
    uint8_t plate_rev_damp;
    uint8_t plate_rev_tail;
    uint8_t plate_dly_rev;

    if (!p)
        return;
    keep_era = dub_clamp_era(p->era);
    plate = dub_clamp_plate(plate);
    pl = dub_plate(plate);
    eb = &DUB_ERA_BASE[keep_era];
    fx = &DUB_ERA_FX[keep_era];
    all = (what & DUB_APPLY_ALL) == DUB_APPLY_ALL;
    plate_dly_lvl = (uint8_t)(72u + (pl->dly_c >> 2));
    plate_rev_damp = (uint8_t)(120u - (pl->loop_filt >> 1));
    plate_rev_tail = (uint8_t)(44u + (pl->tank >> 1) + (pl->rev_c >> 2));
    plate_dly_rev = (uint8_t)(24u + (pl->rev_c >> 1) + (pl->feedback >> 2));
    if (plate_dly_lvl > 116u)
        plate_dly_lvl = 116u;
    if (plate_rev_tail > 118u)
        plate_rev_tail = 118u;
    if (plate_dly_rev > 112u)
        plate_dly_rev = 112u;

    if (all)
        dub_apply_defaults(p);
    p->era = keep_era;

    if (all) {
        p->plate = plate;
        p->density = 64u;
        {
            uint8_t di;

            for (di = 0; di < 4u; di++)
                p->drm_dens[di] = 64u;
        }
        p->root = dub_plate_root(plate);
        p->scale = dub_plate_scale(plate);
        p->swing = dub_plate_swing(plate);
        p->dly_time = dub_mix_ep(eb->dly_time, plate_dly_times[plate]);
    }

    if ((what & DUB_APPLY_CHORD) || all) {
        p->voicing = pl->voicing;
        p->timbre = dub_mix_ep(eb->timbre, plate_timbre[plate]);
        p->cutoff_c = dub_mix_ep(eb->cutoff_c, plate_cutoff_c[plate]);
        p->decay_c = dub_mix_ep(eb->decay_c, plate_decay_c[plate]);
        p->reso_mode = dub_mix_ep(eb->reso_mode, plate_reso_mode[plate]);
        p->env_filt = eb->env_filt;
        p->dly_c = dub_mix_ep(pl->dly_c, eb->decay_c);
        p->lvl_c = 92;
        p->atk_c = 0;
        p->glide_c = 0;
    }

    if ((what & DUB_APPLY_BASS) || all) {
        p->atk_b = 0;
        p->wobble = dub_mix_ep(fx->wobble, pl->wobble);
        p->pump = dub_mix_ep(fx->pump, pl->pump);
        p->cutoff_b = dub_mix_ep(eb->cutoff_b, plate_cutoff_b[plate]);
        p->decay_b = dub_mix_ep(eb->decay_b, plate_decay_b[plate]);
        p->sub_oct = dub_mix_ep(eb->sub_oct, plate_sub_oct[plate]);
        p->drive = dub_mix_ep(eb->drive, plate_drive[plate]);
        p->dly_b = dub_mix_ep(eb->dly_b, plate_dly_b[plate]);
        p->lvl_b = 96;
        p->rev_b = dub_mix_ep((uint8_t)(pl->rev_c >> 1), (uint8_t)(fx->rev_mix >> 2));
    }

    if ((what & DUB_APPLY_DRUM) || all) {
        p->drum_pat = plate;
        p->kick_boom = dub_mix_ep(eb->kick_boom, plate_kick_boom[plate]);
        p->rim_tone = dub_mix_ep(eb->rim_tone, plate_rim_tone[plate]);
        p->hat_lvl = dub_mix_ep(eb->hat_lvl, plate_hat_lvl[plate]);
        dub_drm_voice_kit_fill(p,
            dub_mix_ep(eb->kit_tune, plate_kit_tune[plate]),
            dub_mix_ep(eb->kit_decay, plate_kit_decay[plate]),
            dub_mix_ep(eb->kit_drive, plate_kit_drive[plate]));
        dub_drm_dec_fit_apply(p, plate);
        p->siren_pitch = dub_mix_ep(eb->siren_pitch, plate_siren_pitch[plate]);
        p->siren_rate = dub_mix_ep(eb->siren_rate, plate_siren_rate[plate]);
        {
            uint8_t bus = dub_mix_ep(eb->rim_dly, plate_rim_dly[plate]);

            dub_drm_set_bus_dly(p, bus);
            dub_drm_set_bus_rev(p, bus);
        }
        p->drm_pan[0] = dub_mix_ep(eb->drm_pan[0], plate_pan_k[plate]);
        p->drm_pan[1] = dub_mix_ep(eb->drm_pan[1], plate_pan_r[plate]);
        p->drm_pan[2] = dub_mix_ep(eb->drm_pan[2], plate_pan_h[plate]);
        p->drm_pan[3] = dub_mix_ep(eb->drm_pan[3], plate_pan_s[plate]);
        {
            uint8_t di;
            for (di = 0; di < 4u; di++) {
                p->drm_cut[di] = 64u;
                p->drm_atk[di] = 0u;
            }
        }
    }

    if ((what & DUB_APPLY_FX) || all) {
        p->tap = keep_era == dub_era_of_plate(plate) ? pl->tap : fx->tap;
        p->tank = dub_mix_ep(fx->tank, pl->tank);
        p->feedback = dub_mix_ep(fx->feedback, pl->feedback);
        p->loop_filt = dub_mix_ep(fx->loop_filt, pl->loop_filt);
        p->dly_lvl = dub_mix_ep(fx->dly_lvl, plate_dly_lvl);
        p->rev_damp = dub_mix_ep(fx->rev_damp, plate_rev_damp);
        p->rev_tail = dub_mix_ep(fx->rev_tail, plate_rev_tail);
        p->dly_rev = dub_mix_ep(fx->dly_rev, plate_dly_rev);
        p->dly_c = dub_mix_ep(pl->dly_c, eb->decay_c);
        p->rev_c = dub_mix_ep(pl->rev_c, fx->rev_mix);
        p->rev_mix = dub_mix_ep(fx->rev_mix, pl->rev_c);
        p->wow = eb->wow;
        p->isolator = eb->isolator;
        if (all)
            p->dly_time = dub_mix_ep(eb->dly_time, plate_dly_times[plate]);
        if (all || p->bpm == 0)
            p->bpm = pl->bpm;
    }
}

static inline void dub_plate_groove_read(uint8_t plate, uint8_t *root, uint8_t *scale, uint8_t *swing, uint8_t *bpm)
{
    const dub_plate_t *pl;

    plate = dub_clamp_plate(plate);
    pl = dub_plate(plate);
    if (root)
        *root = dub_plate_root(plate);
    if (scale)
        *scale = dub_plate_scale(plate);
    if (swing)
        *swing = dub_plate_swing(plate);
    if (bpm)
        *bpm = pl->bpm;
}

static inline void dub_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t plate)
{
    if (!p)
        return;
    era = dub_clamp_era(era);
    plate = dub_clamp_plate(plate);
    p->era = era;
    dub_apply_defaults(p);
    p->era = era;
    dub_era_apply_base(p, era);
    dub_plate_apply(p, plate, DUB_APPLY_ALL);
    p->era = era;
}

static inline uint8_t dub_density_allows(uint8_t density, uint8_t step, uint8_t plate_ix)
{
    if (step == 0u || step == 4u || step == 8u || step == 12u)
        return density >= 16u;
    return ((step * 17u + plate_ix) & 127u) < density;
}

static inline uint8_t dub_gate_after_density(uint8_t on, uint8_t density, uint8_t step, uint8_t plate_ix)
{
    if (!on)
        return 0;
    return dub_density_allows(density, step, plate_ix);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out)
{
    const dub_plate_t *drum;
    const dub_plate_t *main;
    uint8_t era;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);

    drum = dub_plate(p->drum_pat);
    main = dub_plate(p->plate);
    era = p->era;
    forced = (ovr != 0) && ((ovr->force & 1u) != 0);

    if (forced) {
        kick = (ovr->gate >> 0) & 1u;
        rim = (ovr->gate >> 1) & 1u;
        hat = (ovr->gate >> 2) & 1u;
        siren = (ovr->gate >> 3) & 1u;
        sub = (ovr->gate >> 4) & 1u;
        chord = (ovr->gate >> 5) & 1u;
    } else {
        kick = dub_step_bit(drum->kick, step);
        rim = dub_step_bit(drum->rim, step);
        hat = dub_step_bit(drum->hat, step);
        sub = dub_step_bit(main->sub, step);
        chord = dub_step_bit(main->chord, step);
        siren = dub_step_bit(main->siren, step);
    }

    out->kick = kick;
    out->rim = rim;
    out->hat = hat;
    out->sub = sub;
    out->chord = chord;
    out->siren = siren;

    if (ovr && ovr->note != 0)
        out->note = ovr->note;
    else
        out->note = p->root;

    if (ovr && ovr->force) {
        out->dly_throw = ovr->dly_throw;
        out->rev_throw = ovr->rev_throw;
        out->accent = ovr->accent;
    } else {
        out->dly_throw = 0;
        out->rev_throw = 0;
        if (chord && era == ERA_CHANNEL) {
            out->dly_throw = 127;
            out->rev_throw = 127;
        }
        if (chord && era == ERA_LOVERS) {
            out->dly_throw = 110;
            out->rev_throw = 100;
        }
        if (rim && era == ERA_KINGSTON) {
            out->dly_throw = 127;
            out->rev_throw = 127;
        }
        if (kick && (step == 0u || step == 4u || step == 8u || step == 12u))
            out->accent = 100;
        else
            out->accent = 70;
    }
}

/* SRN lane 3 plocks: BEND in step_vel, GATE length index in step_micro (HP/House tables). */
enum { DUB_SIREN_GATE_LEN_N = 15u };
static const char *const DUB_SIREN_GATE_LEN_NAMES[DUB_SIREN_GATE_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t DUB_SIREN_GATE_LEN_NUM[DUB_SIREN_GATE_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t DUB_SIREN_GATE_LEN_DEN[DUB_SIREN_GATE_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
static const uint8_t DUB_SIREN_GATE_LEN_16THS[DUB_SIREN_GATE_LEN_N] = {
    1u, 2u, 4u, 5u, 8u, 11u, 12u, 16u, 20u, 24u, 28u, 32u, 40u, 48u, 64u
};

static inline uint8_t dub_siren_gate_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)DUB_SIREN_GATE_LEN_N)
        ix = (int32_t)DUB_SIREN_GATE_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t dub_siren_gate_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = dub_siren_gate_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)DUB_SIREN_GATE_LEN_16THS[ix];
}

static inline uint8_t dub_siren_bend_semi_u8(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

static inline int8_t dub_siren_bend_delta(uint8_t from_step, int8_t cur_tune,
                                          const int8_t step_tune_row[DUB_SEQ_STEPS],
                                          const dub_step_t seq[DUB_SEQ_STEPS])
{
    uint32_t k;

    for (k = 1u; k <= DUB_SEQ_STEPS; k++) {
        uint8_t ns = (uint8_t)((from_step + k) % DUB_SEQ_STEPS);
        int8_t nxt;

        if ((seq[ns].gate & 8u) == 0u)
            continue;
        nxt = step_tune_row[ns];
        if (nxt == cur_tune)
            continue;
        {
            int32_t d = (int32_t)nxt - (int32_t)cur_tune;

            if (d > 12)
                d = 12;
            else if (d < -12)
                d = -12;
            return (int8_t)d;
        }
    }
    return 0;
}

#include "dub_gen.h"

#endif
