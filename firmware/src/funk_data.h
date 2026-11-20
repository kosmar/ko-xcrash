/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FUNK_DATA_H
#define FUNK_DATA_H

#include "dub_types.h"
#include "genre_conf.h"
#include "fm_seq.h"

enum { FUNK_ERA_N = GENRE_ERA_N, FUNK_GROOVE_N = GENRE_PRESET_N };
enum { ERA_POCKET = 0, ERA_CLAV, ERA_SLAP, ERA_DISCO, ERA_BOOGIE, ERA_BROKEN };
enum {
    BASS_FINGER = 0, BASS_SLAP, BASS_OCT, BASS_EP, BASS_N
};
enum {
    DKIT_DRY = 0, DKIT_CLAV, DKIT_SLAP, DKIT_DISCO, DKIT_BOOG, DKIT_DUST, DKIT_N
};
enum { VOICE_DOM7 = 0, VOICE_MIN9, VOICE_NINTH, VOICE_THIRT, VOICE_SUS4, VOICE_N };

typedef dub_plate_t funk_groove_t;

static const char *const FUNK_ERA_NAMES[FUNK_ERA_N] = {
    "POCKET", "CLAV", "SLAP", "DISCO", "BOOGIE", "BROKEN"
};

static const char *const DKIT_NAMES[DKIT_N] = {
    "DRY", "QUILL", "SNAPPY", "SATIN", "RUBBER", "ASHEN"
};

static const uint8_t DKIT_ERA[DKIT_N] = {
    ERA_POCKET, ERA_CLAV, ERA_SLAP, ERA_DISCO, ERA_BOOGIE, ERA_BROKEN
};

enum {
    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS
};

static const int8_t funk_hat_note[DKIT_N] = {
    82, 95, 88, 108, 100, 74
};
static const uint8_t funk_hat_fm[DKIT_N] = {
     6, 58, 10, 84, 54,  4
};
static const uint8_t funk_hat_noise[DKIT_N] = {
    36, 44, 40, 68, 54, 24
};
static const uint8_t funk_hat_metal[DKIT_N] = {
    68, 122, 82, 108, 94, 40
};
static const uint8_t funk_hat_decay[DKIT_N] = {
    33, 43, 39, 43, 43, 43
};
static const uint8_t funk_hat_open[DKIT_N] = {
     2,  10,   3,  24,  16,   5
};
static const uint8_t funk_hat_style[DKIT_N] = {
    HAT_STYLE_TICK, HAT_STYLE_TONE, HAT_STYLE_TR6, HAT_STYLE_SIZZLE,
    HAT_STYLE_SIZZLE, HAT_STYLE_TICK
};
static const uint8_t funk_hat_cut[DKIT_N] = {
    42, 48, 54, 51, 50, 24
};

static const char *const BASS_NAMES[BASS_N] = {
    "FINGER", "FLICK", "OCT", "EP"
};

static const uint8_t BASS_ERA[BASS_N] = {
    ERA_POCKET, ERA_SLAP, ERA_DISCO, ERA_BOOGIE
};

static const char *const VOICE_NAMES[VOICE_N] = {
    "DOM7", "MIN9", "NINTH", "THIRT", "SUS4"
};

static const funk_groove_t FUNK_GROOVE[FUNK_GROOVE_N] = {
    { "ONONE", 0, 0, 0, 0, 16, 100, 110, 24, 18, 14, 34, 2, 88, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "PECK", 0, 0, 1, 1, 17, 102, 109, 25, 19, 15, 35, 3, 89, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4004 },
    { "GHOSTS", 0, 0, 2, 2, 18, 104, 108, 26, 20, 16, 36, 4, 90, 0x2141, 0x9090, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "TIGHT", 0, 0, 3, 0, 19, 104, 107, 27, 21, 17, 37, 5, 91, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4004 },
    { "LOWEND", 0, 0, 4, 1, 20, 106, 106, 28, 22, 18, 38, 6, 92, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "BEHIND", 0, 0, 0, 2, 21, 106, 105, 29, 23, 19, 39, 7, 93, 0x2141, 0x9090, 0x4444, 0x4949, 0x0101, 0x4004 },
    { "LAGGED", 0, 0, 1, 0, 22, 108, 110, 30, 24, 14, 40, 2, 94, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "LOCKIN", 0, 0, 2, 1, 23, 108, 109, 31, 25, 15, 41, 3, 95, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4004 },
    { "ANDONE", 0, 0, 3, 2, 16, 110, 108, 32, 18, 16, 34, 4, 96, 0x2141, 0x9090, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "SETTLE", 0, 0, 4, 0, 17, 112, 107, 33, 19, 17, 35, 5, 97, 0x2141, 0x1010, 0x4444, 0x4949, 0x0101, 0x4004 },
    { "SCRATCH", 1, 0, 0, 0, 20, 100, 110, 24, 18, 14, 34, 2, 88, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x0404 },
    { "TALKER", 1, 0, 1, 1, 21, 102, 109, 25, 19, 15, 35, 3, 89, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x4040 },
    { "CHOKED", 1, 0, 2, 2, 22, 104, 108, 26, 20, 16, 36, 4, 90, 0x2141, 0x1090, 0x4C4C, 0x4949, 0x0101, 0x4404 },
    { "OFFBEAT", 1, 0, 3, 0, 23, 104, 107, 27, 21, 17, 37, 5, 91, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x0404 },
    { "NIBBLE", 1, 0, 4, 1, 0, 106, 106, 28, 22, 18, 38, 6, 92, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x4040 },
    { "TOPKEY", 1, 0, 0, 2, 1, 106, 105, 29, 23, 19, 39, 7, 93, 0x2141, 0x1090, 0x4C4C, 0x4949, 0x0101, 0x4404 },
    { "MUTED", 1, 0, 1, 0, 2, 108, 110, 30, 24, 14, 40, 2, 94, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x0404 },
    { "STUTTER", 1, 0, 2, 1, 3, 108, 109, 31, 25, 15, 41, 3, 95, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x4040 },
    { "NASTY", 1, 0, 3, 2, 20, 110, 108, 32, 18, 16, 34, 4, 96, 0x2141, 0x1090, 0x4C4C, 0x4949, 0x0101, 0x4404 },
    { "LOWKEY", 1, 0, 4, 0, 21, 112, 107, 33, 19, 17, 35, 5, 97, 0x2141, 0x1010, 0x4C4C, 0x4949, 0x0101, 0x0404 },
    { "SNAPUP", 2, 1, 0, 0, 0, 100, 110, 24, 18, 14, 34, 2, 88, 0x0101, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "KNUCKS", 2, 1, 1, 1, 1, 102, 109, 25, 19, 15, 35, 3, 89, 0x0501, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "POPPED", 2, 1, 2, 2, 2, 104, 108, 26, 20, 16, 36, 4, 90, 0x0101, 0x1090, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "RUSHED", 2, 1, 3, 0, 3, 104, 107, 27, 21, 17, 37, 5, 91, 0x0501, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "SKINNY", 2, 1, 4, 1, 4, 106, 106, 28, 22, 18, 38, 6, 92, 0x0101, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "GREASY", 2, 1, 0, 2, 5, 106, 105, 29, 23, 19, 39, 7, 93, 0x0501, 0x1090, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "THUMB", 2, 1, 1, 0, 6, 108, 110, 30, 24, 14, 40, 2, 94, 0x0101, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "THWACK", 2, 1, 2, 1, 7, 108, 109, 31, 25, 15, 41, 3, 95, 0x0501, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "BOUNCE", 2, 1, 3, 2, 0, 110, 108, 32, 18, 16, 34, 4, 96, 0x0101, 0x1090, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "DEEPER", 2, 1, 4, 0, 1, 112, 107, 33, 19, 17, 35, 5, 97, 0x0501, 0x1010, 0x0404, 0x5555, 0x0101, 0x4010 },
    { "FEVER", 3, 2, 0, 0, 4, 112, 110, 24, 18, 14, 34, 2, 88, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "ONFOUR", 3, 2, 1, 1, 5, 114, 109, 25, 19, 15, 35, 3, 89, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "SIZZLE", 3, 2, 2, 2, 6, 114, 108, 26, 20, 16, 36, 4, 90, 0x1111, 0x1090, 0x5D5D, 0x5555, 0x0101, 0x1010 },
    { "HUSTLE", 3, 2, 3, 0, 7, 116, 107, 27, 21, 17, 37, 5, 91, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "GLITTER", 3, 2, 4, 1, 8, 116, 106, 28, 22, 18, 38, 6, 92, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "MIRROR", 3, 2, 0, 2, 9, 118, 105, 29, 23, 19, 39, 7, 93, 0x1111, 0x1090, 0x5D5D, 0x5555, 0x0101, 0x1010 },
    { "ROLLER", 3, 2, 1, 0, 10, 118, 110, 30, 24, 14, 40, 2, 94, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "STROBE", 3, 2, 2, 1, 11, 120, 109, 31, 25, 15, 41, 3, 95, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "SPARK", 3, 2, 3, 2, 4, 120, 108, 32, 18, 16, 34, 4, 96, 0x1111, 0x1090, 0x5D5D, 0x5555, 0x0101, 0x1010 },
    { "AFTERS", 3, 2, 4, 0, 5, 122, 107, 33, 19, 17, 35, 5, 97, 0x1111, 0x1010, 0x5555, 0x5555, 0x0101, 0x1010 },
    { "STRIDE", 4, 2, 0, 0, 8, 108, 110, 24, 18, 14, 34, 2, 88, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "TUCKED", 4, 2, 1, 1, 9, 110, 109, 25, 19, 15, 35, 3, 89, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "TINES", 4, 3, 2, 2, 10, 110, 108, 26, 20, 16, 36, 4, 90, 0x1111, 0x1090, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "ONTWO", 4, 2, 3, 0, 11, 112, 107, 27, 21, 17, 37, 5, 91, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "STOMP", 4, 2, 4, 1, 12, 112, 106, 28, 22, 18, 38, 6, 92, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "FLOOR", 4, 2, 0, 2, 13, 114, 105, 29, 23, 19, 39, 7, 93, 0x1111, 0x1090, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "MIDNIT", 4, 2, 1, 0, 14, 114, 110, 30, 24, 14, 40, 2, 94, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "EPWALK", 4, 3, 2, 1, 15, 116, 109, 31, 25, 15, 41, 3, 95, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "STRUT", 4, 2, 3, 2, 8, 116, 108, 32, 18, 16, 34, 4, 96, 0x1111, 0x1090, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "OCTAVE", 4, 2, 4, 0, 9, 118, 107, 33, 19, 17, 35, 5, 97, 0x1111, 0x1010, 0x5555, 0x4949, 0x0101, 0x1010 },
    { "FRAYED", 5, 0, 0, 0, 12, 84, 110, 24, 18, 14, 34, 2, 88, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "DUSTY", 5, 0, 1, 1, 13, 86, 109, 25, 19, 15, 35, 3, 89, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "SLANT", 5, 0, 2, 2, 14, 88, 108, 26, 20, 16, 36, 4, 90, 0x2081, 0x1090, 0x5D5D, 0x4949, 0x0101, 0x4010 },
    { "HUSHED", 5, 3, 3, 0, 15, 88, 107, 27, 21, 17, 37, 5, 91, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "LIMP", 5, 0, 4, 1, 16, 90, 106, 28, 22, 18, 38, 6, 92, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "SHUFFL", 5, 0, 0, 2, 17, 90, 105, 29, 23, 19, 39, 7, 93, 0x0841, 0x1090, 0x5D5D, 0x4949, 0x0101, 0x4010 },
    { "VINYL", 5, 0, 1, 0, 18, 92, 110, 30, 24, 14, 40, 2, 94, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "CRACK", 5, 0, 2, 1, 19, 92, 109, 31, 25, 15, 41, 3, 95, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
    { "SMOKE", 5, 3, 3, 2, 12, 94, 108, 32, 18, 16, 34, 4, 96, 0x0841, 0x1090, 0x5D5D, 0x4949, 0x0101, 0x4010 },
    { "NESOUL", 5, 0, 4, 0, 13, 96, 107, 33, 19, 17, 35, 5, 97, 0x2081, 0x1010, 0x4444, 0x4949, 0x0101, 0x4010 },
};

static const int8_t FUNK_GROOVE_ROOT[FUNK_GROOVE_N] = {
    36, 38, 39, 41, 43, 45, 40, 42, 46, 48,
    46, 48, 38, 41, 43, 36, 47, 49, 53, 55,
    39, 36, 43, 41, 38, 45, 37, 40, 43, 46,
    41, 43, 36, 39, 45, 38, 42, 44, 48, 50,
    43, 41, 48, 46, 38, 41, 40, 42, 46, 48,
    39, 36, 43, 41, 38, 45, 44, 46, 50, 52
};

static const int8_t FUNK_GROOVE_SCALE[FUNK_GROOVE_N] = {0, 1, 2, 4, 5, 0, 1, 5, 7, 11, 1, 2, 4, 0, 5, 1, 4, 0, 10, 6, 0, 2, 1, 4, 5, 0, 0, 2, 6, 8, 1, 4, 0, 5, 2, 1, 1, 5, 7, 11, 4, 5, 0, 1, 2, 4, 4, 2, 10, 8, 2, 0, 5, 1, 4, 2, 0, 1, 6, 7};

static const int8_t FUNK_GROOVE_SWING[FUNK_GROOVE_N] = {
    52, 53, 54, 54, 55, 55, 56, 56, 57, 58,
    56, 57, 58, 58, 59, 60, 60, 61, 62, 64,
    54, 54, 55, 55, 56, 56, 57, 58, 58, 60,
    50, 50, 51, 51, 51, 52, 52, 52, 53, 54,
    54, 55, 56, 56, 57, 58, 58, 59, 60, 62,
    66, 68, 69, 70, 71, 72, 73, 74, 76, 78
};

/* Per-trax STAB SNAP (siren_rate) delta from era base; six trax span each zone. */
static const int8_t FUNK_GROOVE_STAB_RATE[FUNK_GROOVE_N] = {-5, -3, -1, 1, 3, 5, -4, -2, 2, 4, -6, -4, -2, 0, 2, 4, -3, -1, 3, 5, -5, -3, -1, 1, 3, 5, -4, -2, 2, 4, -4, -2, 0, 2, 3, 4, -3, -1, 3, 5, -7, -5, -3, -1, 1, 3, -6, -4, 0, 2, -6, -4, -2, 0, 2, 4, -5, -3, 1, 3};

static const uint8_t FUNK_ERA_MACROS[FUNK_ERA_N][10] = {
    /* slot 1 CROP stays 0: above 16 locks a seq vamp */
    { 70, 0, 24, 56, 48, 88, 32, 40, 0, 36 },
    { 68, 0, 30, 52, 44, 92, 28, 38, 0, 42 },
    { 64, 0, 20, 48, 40, 86, 44, 36, 0, 48 },
    { 60, 0, 16, 50, 52, 84, 24, 42, 0, 34 },
    { 70, 0, 22, 54, 56, 88, 30, 44, 0, 50 },
    { 66, 0, 26, 58, 46, 90, 36, 40, 0, 46 },
};

typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} funk_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} funk_era_fx_t;

/* sub_oct: 0=−1oct (was 32–36 unison — bass sat midrange). ACID 88=−2oct. */
static const funk_era_base_t FUNK_ERA_BASE[FUNK_ERA_N] = {
    { 36, 92, 88, 20, 44, 68, 92, 0, 28, 24, 82, 58, 54, 64, 64, 18, 64, 23, 32, {64,72,48,80}, 8, 52, 32, 60 },
    { 40, 88, 86, 24, 48, 66, 90, 0, 26, 28, 78, 56, 58, 64, 68, 16, 62, 52, 36, {64,70,52,78}, 10, 48, 28, 62 },
    { 44, 84, 82, 40, 60, 72, 88, 88, 32, 18, 80, 54, 62, 64, 56, 24, 58, 57, 28, {64,74,50,82}, 6, 44, 40, 55 },
    { 32, 90, 90, 18, 40, 64, 96, 0, 24, 32, 76, 52, 50, 64, 72, 14, 60, 42, 40, {64,68,46,76}, 12, 56, 24, 58 },
    { 38, 94, 84, 28, 52, 70, 90, 0, 30, 26, 74, 60, 56, 64, 64, 20, 66, 48, 44, {62,76,50,84}, 14, 50, 36, 57 },
    { 42, 86, 86, 24, 38, 74, 92, 0, 26, 22, 72, 50, 48, 64, 60, 16, 56, 86, 30, {64,72,54,70}, 8, 46, 32, 56 },
};

static const funk_era_fx_t FUNK_ERA_FX[FUNK_ERA_N] = {
    { DKIT_DRY,  22, 0, 42, 20, 72, 32, 34, 58, 48,  96, BASS_FINGER,  8 },
    { DKIT_CLAV, 28, 1, 44, 18, 78, 36, 36, 60, 50,  88, BASS_FINGER, 10 },
    { DKIT_SLAP, 18, 0, 48, 22, 70, 30, 32, 54, 44,  90, BASS_SLAP,   12 },
    { DKIT_DISCO, 32, 2, 40, 16, 82, 40, 38, 62, 52, 102, BASS_OCT,     6 },
    { DKIT_BOOG, 36, 2, 46, 20, 80, 38, 36, 64, 54,  94, BASS_OCT,    14 },
    { DKIT_DUST, 48, 3, 52, 24, 76, 42, 40, 66, 56,  86, BASS_FINGER, 18 },
};

/* Bass melodic contour: era × in-era slot × 8-step scale degree (0..6). */
static const uint8_t FUNK_BASS_WALK[FUNK_ERA_N][GENRE_PRESETS_PER_ERA][8] = {
    { { 5, 5, 3, 3, 0, 0, 4, 4 }, { 0, 0, 5, 5, 3, 3, 0, 4 }, { 5, 3, 0, 0, 4, 4, 3, 0 }, { 0, 4, 0, 3, 5, 0, 4, 3 }, { 3, 0, 5, 5, 0, 4, 3, 0 }, { 5, 5, 0, 3, 3, 0, 4, 4 }, { 0, 3, 5, 0, 4, 3, 0, 5 }, { 4, 0, 3, 5, 0, 0, 5, 3 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 3, 0, 0, 5, 0, 3 }, { 0, 3, 0, 0, 5, 0, 3, 0 }, { 0, 0, 0, 3, 0, 5, 0, 0 }, { 3, 0, 0, 5, 0, 0, 3, 0 }, { 0, 5, 0, 3, 0, 0, 0, 5 }, { 0, 0, 5, 0, 3, 0, 0, 3 }, { 5, 0, 0, 3, 0, 5, 0, 0 }, { 0, 3, 0, 5, 0, 0, 3, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 2, 0, 0, 2, 0, 0 }, { 0, 2, 0, 2, 0, 0, 2, 0 }, { 0, 0, 0, 2, 0, 0, 0, 2 }, { 0, 2, 2, 0, 0, 2, 2, 0 }, { 0, 0, 4, 0, 0, 4, 0, 0 }, { 0, 4, 0, 4, 0, 2, 0, 2 }, { 0, 2, 4, 2, 0, 2, 4, 2 }, { 0, 0, 2, 4, 0, 0, 2, 4 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 3, 0, 0, 5, 0 }, { 0, 0, 3, 0, 0, 5, 0, 0 }, { 0, 3, 0, 0, 5, 0, 0, 3 }, { 0, 0, 5, 0, 3, 0, 0, 0 }, { 0, 5, 0, 0, 0, 3, 0, 5 }, { 3, 0, 0, 0, 0, 5, 0, 3 }, { 0, 0, 3, 5, 0, 0, 0, 3 }, { 0, 3, 0, 0, 0, 3, 5, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 3, 5, 3, 0, 4, 5, 0 }, { 0, 4, 3, 5, 0, 3, 4, 0 }, { 3, 0, 5, 4, 3, 0, 5, 0 }, { 0, 3, 0, 5, 4, 0, 3, 5 }, { 5, 0, 3, 0, 5, 4, 0, 3 }, { 0, 5, 3, 0, 4, 5, 0, 3 }, { 3, 5, 0, 4, 0, 3, 5, 0 }, { 0, 3, 4, 5, 3, 0, 4, 5 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 2, 0, 4, 2, 0, 5, 2 }, { 0, 2, 4, 2, 0, 5, 2, 0 }, { 2, 0, 4, 5, 2, 0, 4, 0 }, { 0, 4, 2, 0, 5, 4, 2, 0 }, { 0, 2, 5, 4, 0, 2, 4, 5 }, { 4, 0, 2, 5, 4, 0, 2, 5 }, { 0, 5, 4, 2, 0, 4, 2, 5 }, { 2, 4, 0, 5, 2, 4, 0, 2 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } }
};

#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM groove at funk_gen.slot. */
static fm_pgen_t funk_gen = { .slot = 0xFFu };

#define FM_PGEN_STATE funk_gen
#define FM_PGEN_BUILD(era, n, c) funk_gen_build(era, n, c)

static inline uint8_t funk_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&funk_gen, ix);
}

static inline uint8_t funk_clamp_groove(uint8_t ix)
{
    return ix >= FUNK_GROOVE_N ? (uint8_t)(FUNK_GROOVE_N - 1u) : ix;
}

static inline uint8_t funk_clamp_era(uint8_t era)
{
    return era >= FUNK_ERA_N ? (uint8_t)(FUNK_ERA_N - 1u) : era;
}

static inline const char *funk_era_name(uint8_t era)
{
    return FUNK_ERA_NAMES[funk_clamp_era(era)];
}

static inline const char *funk_groove_name(uint8_t trax)
{
    if (funk_gen_hit(trax))
        return funk_gen.name;
    return FUNK_GROOVE[funk_clamp_groove(trax)].name;
}

static inline const funk_groove_t *funk_groove(uint8_t ix)
{
    if (funk_gen_hit(ix))
        return &funk_gen.pl;
    return &FUNK_GROOVE[funk_clamp_groove(ix)];
}

static inline uint8_t funk_era_of_groove(uint8_t trax)
{
    trax = funk_clamp_groove(trax);
    return (uint8_t)(trax / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t funk_era_first_groove(uint8_t era)
{
    return (uint8_t)(funk_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint8_t funk_bass_walk_deg(uint8_t era, uint8_t trax, uint8_t step)
{
    era = funk_clamp_era(era);
    trax = (uint8_t)(funk_clamp_groove(trax) % GENRE_PRESETS_PER_ERA);
    return FUNK_BASS_WALK[era][trax][step & 7u];
}

static const int8_t FUNK_GROOVE_KIT[FUNK_GROOVE_N] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

static inline uint8_t funk_groove_root(uint8_t ix)
{
    return funk_gen_hit(ix) ? (uint8_t)funk_gen.root : (uint8_t)FUNK_GROOVE_ROOT[funk_clamp_groove(ix)];
}

static inline uint8_t funk_groove_scale(uint8_t ix)
{
    return funk_gen_hit(ix) ? (uint8_t)funk_gen.scale : (uint8_t)FUNK_GROOVE_SCALE[funk_clamp_groove(ix)];
}

static inline uint8_t funk_groove_swing(uint8_t ix)
{
    return funk_gen_hit(ix) ? (uint8_t)funk_gen.swing : (uint8_t)FUNK_GROOVE_SWING[funk_clamp_groove(ix)];
}

static inline uint8_t funk_groove_kit(uint8_t ix)
{
    return funk_gen_hit(ix) ? (uint8_t)funk_gen.kit : (uint8_t)FUNK_GROOVE_KIT[funk_clamp_groove(ix)];
}

static inline int8_t funk_groove_stab_rate(uint8_t ix)
{
    return funk_gen_hit(ix) ? funk_gen.rate : FUNK_GROOVE_STAB_RATE[funk_clamp_groove(ix)];
}

static inline void funk_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 32;
    p->cutoff_c = 88;
    p->decay_c = 86;
    p->cutoff_b = 70;
    p->decay_b = 90;
    p->sub_oct = 0; /* −1oct */
    p->drive = 26;
    p->pump = 88;
    p->feedback = 16;
    p->density = 64u;
    p->wobble = 2;
    p->kick_boom = 82;
    p->rim_tone = 58;
    p->hat_lvl = 52;
    p->siren_pitch = 64;
    p->siren_rate = 23;
    p->drum_kit = DKIT_DRY;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 20u;
        p->drm_dly[di] = 28u;
        p->drm_rev[di] = 24u;
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 64u;
    }
}

static inline uint8_t dub_step_bit(uint16_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & 15u)) & 1u);
}

static inline void dub_drm_set_bus_dly(dub_params_t *p, uint8_t bus)
{
    uint8_t i;

    if (!p)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dly[i] = bus;
    p->drm_dly[3] = bus;
}

static inline void dub_drm_set_bus_rev(dub_params_t *p, uint8_t bus)
{
    uint8_t i;
    uint8_t rv = (uint8_t)(48u + (bus >> 1));

    if (!p)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_rev[i] = rv;
    p->drm_rev[3] = 96u;
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

static inline void funk_era_apply_base(dub_params_t *p, uint8_t era)
{
    const funk_era_base_t *eb;
    const funk_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = funk_clamp_era(era);
    eb = &FUNK_ERA_BASE[era];
    fx = &FUNK_ERA_FX[era];
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
    p->swing = eb->swing;
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
    p->drum_kit = fx->drum_kit;
    p->bass_model = fx->bass_model;
    p->lvl_b = 104;
    p->lvl_c = 92;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

/* Per-preset KICK/RIM/HAT decay trims (drm_dec[0..2]), fitted so every factory voice keeps
 * its envelope length from before the quarter-octave drum decay (±15 %). Siren keeps kit_decay.
 * Column 3: open-hat length trim, 64 + hat_mul steps; read by the DSP via p->plate, never saved. */
static const uint8_t funk_drm_dec_fit[GENRE_PRESET_N][4] = {
    { 29,  38,  64, 62}, { 28,  39,  88, 63}, { 27,  40,  68, 64}, { 29,  41,  68, 62},
    { 27,  41,  64, 62}, { 29,  39,  60, 63}, { 28,  39,  63, 65}, { 26,  39,  68, 62},
    { 28,  40,  87, 63}, { 27,  38,  60, 63}, { 33,  40,  66, 63}, { 33,  42,  68, 63},
    { 33,  43,  64, 63}, { 32,  42,  72, 63}, { 32,  41,  64, 63}, { 32,  43,  67, 63},
    { 31,  41,  72, 63}, { 33,  43,  72, 63}, { 32,  42,  64, 63}, { 32,  41,  64, 63},
    { 31,  40,  63, 63}, { 29,  45,  64, 63}, { 28,  44,  60, 63}, { 28,  45,  60, 63},
    { 28,  42,  60, 63}, { 30,  44,  64, 63}, { 30,  42,  67, 63}, { 29,  43,  64, 63},
    { 28,  45,  67, 63}, { 30,  45,  63, 63}, { 34,  45,  64, 63}, { 34,  46,  68, 63},
    { 34,  46,  72, 63}, { 35,  45,  68, 63}, { 34,  46,  72, 63}, { 35,  44,  72, 64},
    { 35,  45,  64, 63}, { 33,  46,  64, 63}, { 35,  44,  64, 63}, { 34,  46,  64, 63},
    { 37,  37,  68, 63}, { 34,  38,  67, 63}, { 36,  39,  72, 63}, { 37,  36,  68, 63},
    { 36,  37,  67, 63}, { 37,  38,  67, 63}, { 37,  38,  72, 63}, { 37,  39,  67, 63},
    { 37,  38,  72, 64}, { 37,  39,  72, 63}, { 37,  49,  72, 63}, { 38,  48,  68, 63},
    { 39,  46,  72, 63}, { 39,  47,  64, 63}, { 36,  46,  68, 63}, { 39,  49,  64, 63},
    { 37,  49,  64, 63}, { 39,  48,  64, 63}, { 36,  48,  68, 63}, { 37,  48,  64, 63}
};
#define GENRE_DRM_DEC_FIT funk_drm_dec_fit

static inline void funk_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = funk_drm_dec_fit[preset][i];
}

static inline void funk_groove_apply(dub_params_t *p, uint8_t trax, uint8_t what)
{
    const funk_groove_t *tx;
    uint8_t di;

    if (!p)
        return;
    trax = funk_clamp_groove(trax);
    tx = funk_groove(trax);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        funk_apply_defaults(p);
        funk_era_apply_base(p, tx->era);
        funk_drm_dec_fit_apply(p, trax);
    }
    p->era = tx->era;
    p->plate = trax;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = tx->bpm;
        p->density = 64u;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = 64u;
        p->drum_kit = funk_groove_kit(trax);
        p->bass_model = tx->bass_model;
        p->voicing = tx->voicing;
        p->pump = tx->pump;
        p->wobble = tx->wobble;
        p->feedback = tx->feedback;
        p->tank = tx->tank;
        p->tap = tx->tap;
        p->dly_c = tx->dly_c;
        p->rev_c = tx->rev_c;
        p->loop_filt = tx->loop_filt;
        p->root = funk_groove_root(trax);
        p->scale = funk_groove_scale(trax);
        p->swing = funk_groove_swing(trax);
        {
            int16_t stab = (int16_t)p->siren_rate + (int16_t)funk_groove_stab_rate(trax);

            if (stab < 0)
                stab = 0;
            else if (stab > 127)
                stab = 127;
            p->siren_rate = (uint8_t)stab;
        }
    }
}

static inline void funk_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t trax)
{
    if (!p)
        return;
    era = funk_clamp_era(era);
    trax = funk_clamp_groove(trax);
    funk_groove_apply(p, trax, DUB_APPLY_ALL);
    p->era = era;
    p->plate = trax;
    p->drum_kit = funk_groove_kit(trax);
    p->bass_model = funk_groove(trax)->bass_model;
    p->voicing = funk_groove(trax)->voicing;
    p->bpm = funk_groove(trax)->bpm;
    p->pump = funk_groove(trax)->pump;
    p->feedback = funk_groove(trax)->feedback;
    p->wobble = funk_groove(trax)->wobble;
    p->drum_pat = trax;
    p->root = funk_groove_root(trax);
    p->scale = funk_groove_scale(trax);
    p->swing = funk_groove_swing(trax);
}

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
        dub_drm_set_bus_dly(p, 28u);
    if (!rnz)
        dub_drm_set_bus_rev(p, 28u);
}

static inline void dub_plate_groove_read(uint8_t plate, uint8_t *root, uint8_t *scale, uint8_t *swing, uint8_t *bpm)
{
    const funk_groove_t *tx;

    plate = funk_clamp_groove(plate);
    tx = funk_groove(plate);
    if (root)
        *root = funk_groove_root(plate);
    if (scale)
        *scale = funk_groove_scale(plate);
    if (swing)
        *swing = funk_groove_swing(plate);
    if (bpm)
        *bpm = tx->bpm;
}

static inline uint8_t funk_density_allows(uint8_t density, uint8_t step, uint8_t trax_ix)
{
    if (step == 0u || step == 4u || step == 8u || step == 12u)
        return density >= 16u;
    return ((step * 17u + trax_ix) & 127u) < density;
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out)
{
    const funk_groove_t *drum;
    const funk_groove_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = funk_groove(p->drum_pat);
    main = funk_groove(p->plate);
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
        siren = dub_step_bit(drum->siren, step);
        sub = dub_step_bit(main->sub, step);
        chord = dub_step_bit(main->chord, step);
    }
    out->kick = kick;
    out->rim = rim;
    out->hat = hat;
    out->siren = siren;
    out->sub = sub;
    out->chord = chord;
    out->note = forced ? ovr->note : 0u;
    out->dly_throw = forced ? ovr->dly_throw : 0u;
    out->rev_throw = forced ? ovr->rev_throw : 0u;
    out->accent = forced ? ovr->accent : 0u;
}

/* STAB lane GATE / BEND plocks share HP lead length tables (1/16 … 4/1). */
enum { FUNK_STAB_GATE_LEN_N = 8u };
static const char *const FUNK_STAB_GATE_LEN_NAMES[FUNK_STAB_GATE_LEN_N] = {
    "1/32", "1/16", "1/8", "1/4", "1/2", "3/4", "1/1", "2/1"
};
static const uint8_t FUNK_STAB_GATE_LEN_NUM[FUNK_STAB_GATE_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 3u, 1u, 2u
};
static const uint8_t FUNK_STAB_GATE_LEN_DEN[FUNK_STAB_GATE_LEN_N] = {
    32u, 16u, 8u, 4u, 2u, 4u, 1u, 1u
};
static const uint8_t FUNK_STAB_GATE_LEN_16THS[FUNK_STAB_GATE_LEN_N] = {
    1u, 1u, 2u, 4u, 8u, 12u, 16u, 32u
};

static inline uint8_t funk_stab_gate_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)FUNK_STAB_GATE_LEN_N)
        ix = (int32_t)FUNK_STAB_GATE_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t funk_stab_gate_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = funk_stab_gate_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)FUNK_STAB_GATE_LEN_16THS[ix];
}

static inline uint8_t funk_stab_bend_semi_u8(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

/* Per-step hat/rim velocity + micro (Chicago shuffle); pattern gates unchanged. */
static inline void funk_groove_step_plock(uint8_t ln, uint8_t st, const funk_groove_t *drum,
                                           uint8_t *out_vel, int8_t *out_micro)
{
    uint8_t on;

    if (!drum || !out_vel || !out_micro)
        return;
    *out_vel = 0u;
    *out_micro = 0;
    if (ln == 2u) {
        on = dub_step_bit(drum->hat, st);
        if (!on)
            return;
        if ((st & 3u) == 2u || (st & 3u) == 3u)
            *out_vel = 106u;
        else if (st & 1u)
            *out_vel = 76u;
        else
            *out_vel = 92u;
        if (st & 1u)
            *out_micro = (int8_t)(5 + (int8_t)(st & 7u));
        else if ((st & 3u) == 2u)
            *out_micro = (int8_t)(3 + (int8_t)((st >> 2) & 3u));
        return;
    }
    if (ln == 1u) {
        on = dub_step_bit(drum->rim, st);
        if (!on)
            return;
        if ((st & 4u) == 4u && (st & 1u))
            *out_vel = 36u;
        else if ((st & 4u) == 0u)
            *out_vel = 114u;
        else
            *out_vel = 88u;
        if ((st & 3u) == 1u || (st & 3u) == 3u)
            *out_micro = (int8_t)(4 + (int8_t)(st & 3u));
        return;
    }
    if (ln == 0u && dub_step_bit(drum->kick, st) && (st & 3u) == 3u)
        *out_micro = 2;
}

static inline uint32_t funk_micro_delay_samples(int8_t micro, uint32_t samples_per_step)
{
    uint32_t cap;
    uint32_t dly;
    int32_t a;

    if (samples_per_step == 0u)
        samples_per_step = 1u;
    cap = samples_per_step / 2u;
    if (cap < 1u)
        cap = 1u;
    a = micro < 0 ? -micro : (int32_t)micro;
    dly = ((uint32_t)a * samples_per_step) / 52u;
    if (dly > cap)
        dly = cap;
    return dly;
}

static inline int8_t funk_stab_bend_delta(uint8_t from_step, int8_t cur_tune,
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

#include "funk_gen.h"

#endif
