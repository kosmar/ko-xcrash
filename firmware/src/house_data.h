/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HOUSE_DATA_H
#define HOUSE_DATA_H

#include "dub_types.h"
#include "genre_conf.h"
#include "fm_seq.h"

enum { HOUSE_ERA_N = GENRE_ERA_N, HOUSE_TRAX_N = GENRE_PRESET_N };
enum { ERA_JACK = 0, ERA_GARAGE, ERA_ACID, ERA_DEEP, ERA_FILTER, ERA_TECH };
enum {
    BASS_SUB = 0, BASS_808, BASS_ORGAN, BASS_OCTAVE, BASS_303, BASS_MOOG,
    BASS_REESE, BASS_FM, BASS_PWM, BASS_PLUCK, BASS_N
};
enum {
    DKIT_808 = 0, DKIT_909, DKIT_707, DKIT_606, DKIT_LINN, DKIT_CR78, DKIT_UKG, DKIT_TECH, DKIT_N
};
enum { VOICE_MAJ7 = 0, VOICE_MIN7, VOICE_SUS4, VOICE_9TH, VOICE_6TH, VOICE_N };

typedef dub_plate_t house_trax_t;

static const char *const HOUSE_ERA_NAMES[HOUSE_ERA_N] = {
    "JACK", "GARAGE", "ACID", "DEEP", "FILTER", "TECH"
};

static const char *const DKIT_NAMES[DKIT_N] = {
    "EOE", "9OE", "7OE", "6OE", "LYNX", "CR79", "UKG", "TECH"
};

static const uint8_t DKIT_ERA[DKIT_N] = {
    ERA_JACK, ERA_DEEP, ERA_JACK, ERA_ACID, ERA_GARAGE, ERA_FILTER, ERA_GARAGE, ERA_TECH
};

enum {
    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS
};

/* Per-kit closed/open hat voice (808 … TECH). DSP + desk viz use these tables. */
static const int8_t house_hat_note[DKIT_N] = {
    84, 96, 93, 99, 94, 78, 88, 103
};
static const uint8_t house_hat_fm[DKIT_N] = {
     4, 62, 28,  8, 44,  6, 12, 82
};
static const uint8_t house_hat_noise[DKIT_N] = {
    18, 52, 48, 26, 58, 14, 56, 60
};
static const uint8_t house_hat_metal[DKIT_N] = {
   102,  68, 108, 112,  76, 120,  42,  88
};
static const uint8_t house_hat_decay[DKIT_N] = {
     8, 24, 24,  8, 16, 30, 11,  8
};
static const uint8_t house_hat_open[DKIT_N] = {
    10, 13,  2,  2, 10,  0, 13, 10
};
static const uint8_t house_hat_style[DKIT_N] = {
    HAT_STYLE_TICK, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TR6,
    HAT_STYLE_TONE, HAT_STYLE_TONE, HAT_STYLE_HISS, HAT_STYLE_SIZZLE
};
static const uint8_t house_hat_cut[DKIT_N] = {
    52, 58, 46, 48, 47, 38, 34, 50
};

static const char *const BASS_NAMES[BASS_N] = {
    "SUB", "EOE", "ORGAN", "OCTAVE", "3OE", "MOCK", "REESE", "FM", "PWM", "PLUCK"
};

static const uint8_t BASS_ERA[BASS_N] = {
    ERA_DEEP, ERA_JACK, ERA_JACK, ERA_GARAGE, ERA_ACID, ERA_DEEP,
    ERA_GARAGE, ERA_TECH, ERA_FILTER, ERA_ACID
};

static const char *const VOICE_NAMES[VOICE_N] = {
    "MAJ7", "MIN7", "SUS4", "9TH", "6TH"
};

static const house_trax_t HOUSE_TRAX[HOUSE_TRAX_N] = {
{ "VINYLT", 0, 2, 1, 0, 22, 122, 118, 28, 20, 18, 36,  2,  96, 0x1111, 0x1010, 0x4646, 0x4924, 0x0404, 0x8000 },
{ "KNIGHT", 0, 0, 1, 0, 18, 124, 120, 26, 22, 16, 34,  4,  94, 0x9111, 0x9010, 0x6464, 0x2492, 0x4040, 0x0004 },
{ "GROOVE", 0, 2, 4, 0, 20, 120, 116, 30, 18, 20, 38,  6,  98, 0x1111, 0x1210, 0x4C44, 0x5252, 0x2202, 0x4000 },
{ "JACKMST", 0, 2, 1, 1, 24, 126, 114, 32, 24, 14, 40,  8, 100, 0x5111, 0x1010, 0x5555, 0x8A42, 0x0820, 0x0080 },
{ "SPIRIT", 0, 0, 0, 0, 16, 118, 112, 24, 16, 22, 32,  3,  92, 0x1111, 0x5018, 0x4444, 0x294A, 0x4204, 0x2000 },
{ "WRHSE", 0, 2, 5, 0, 22, 125, 119, 28, 20, 18, 36,  5,  96, 0x1111, 0x1090, 0x4646, 0x9424, 0x0442, 0x0800 },
    { "ACIDFX",   0, 2, 1, 0, 20, 121, 117, 27, 19, 17, 35,  3,  95, 0x1111, 0x1010, 0x4848, 0x4A24, 0x0408, 0x0008 },
    { "FEELIT",  0, 0, 0, 0, 19, 119, 115, 25, 17, 19, 33,  2,  93, 0x9111, 0x5010, 0x6464, 0x9242, 0x4044, 0x0010 },
    { "SLOWJK",   0, 2, 1, 0, 20, 118, 97, 39, 31, 17, 35,  3,  95, 0x1111, 0x1010, 0x0808, 0x4A24, 0x0408, 0x0008 },
    { "SMOOTH",  0, 0, 0, 0, 19, 118, 95, 37, 29, 19, 33,  2,  93, 0x9111, 0x5010, 0x2020, 0x9242, 0x4044, 0x0010 },
{ "FREAKY", 1, 6, 1, 2, 40, 130, 110, 34, 28, 22, 42, 12,  88, 0x1111, 0x1010, 0x4444, 0x4488, 0x4082, 0x8080 },
{ "TODRHY", 1, 6, 0, 2, 38, 132, 108, 36, 30, 20, 44, 10,  86, 0x1111, 0x9010, 0x5555, 0x2484, 0x0841, 0x0040 },
{ "MAESTRO", 1, 4, 1, 3, 42, 130, 112, 32, 26, 24, 40, 14,  90, 0x1111, 0x1010, 0xEEEE, 0x8842, 0x2408, 0x8000 },
{ "2STEP", 1, 6, 2, 2, 44, 134, 106, 38, 32, 18, 46, 11,  84, 0x0421, 0x1010, 0x4444, 0x14A1, 0x8204, 0x0004 },
{ "SWINGUK", 1, 7, 1, 2, 46, 136, 104, 40, 34, 16, 48, 13,  82, 0x2809, 0x1210, 0xF7F7, 0x4826, 0x4120, 0x0080 },
{ "JAMHARD", 1, 6, 0, 3, 48, 133, 109, 36, 28, 20, 44, 15,  88, 0x4209, 0x9010, 0x5555, 0x6248, 0x0248, 0x4000 },
    { "ELBOW",   1, 6, 2, 2, 41, 131, 107, 35, 29, 21, 43, 11,  87, 0x1111, 0x1010, 0x5555, 0x8448, 0x4084, 0x0040 },
    { "SOULFUL",   1, 4, 0, 3, 45, 135, 105, 37, 31, 19, 45, 12,  85, 0x4209, 0x9010, 0xEEEE, 0x4288, 0x2410, 0x0100 },
    { "SWAY",   1, 6, 2, 2, 41, 128, 87, 47, 41, 21, 43, 11,  87, 0x1111, 0x1010, 0x0101, 0x8448, 0x4084, 0x0040 },
    { "GARAGE",   1, 4, 0, 3, 45, 130, 85, 49, 43, 19, 45, 12,  85, 0x4209, 0x9010, 0xAAAA, 0x4288, 0x2410, 0x0100 },
{ "ACIDPK", 2, 4, 0, 1, 12, 126, 115, 22, 14, 28, 52, 72,  90, 0x1111, 0x1010, 0xAAAA, 0xD7B5, 0x0040, 0x8000 },
{ "303JAM", 2, 4, 0, 0, 10, 127, 118, 24, 16, 32, 54, 85,  92, 0x1111, 0x9010, 0xCCCC, 0xEDB7, 0x0400, 0x0004 },
{ "LINDEN", 2, 9, 0, 1, 14, 126, 112, 20, 12, 26, 50, 65,  88, 0x1111, 0x1010, 0xFFFF, 0xB6D3, 0x4004, 0x0040 },
{ "SLEAZE", 2, 4, 3, 0, 8, 128, 120, 26, 18, 30, 56, 78,  94, 0x9111, 0x1210, 0xEEEE, 0x7DF5, 0x0080, 0x2000 },
{ "JACKACD", 2, 4, 1, 1, 11, 125, 116, 22, 14, 28, 48, 68,  86, 0x1111, 0x5018, 0xAAAA, 0xE9B6, 0x0200, 0x0800 },
{ "TB303", 2, 4, 0, 0, 6, 127, 114, 24, 16, 34, 58, 92,  96, 0x1111, 0x1010, 0xCCCC, 0xF36D, 0x2000, 0x8080 },
    { "HANDZ",  2, 4, 0, 1, 13, 127, 113, 21, 13, 27, 51, 70,  89, 0x1111, 0x1010, 0xBBBB, 0xB5D7, 0x0020, 0x4000 },
    { "EERIE",  2, 9, 3, 0, 9, 128, 117, 23, 15, 31, 55, 88,  91, 0x9111, 0x1210, 0xFFFF, 0xF57D, 0x0120, 0x0800 },
    { "ACIDBM",  2, 4, 0, 1, 13, 120, 93, 33, 25, 27, 51, 70,  89, 0x1111, 0x1010, 0xAAAA, 0xB5D7, 0x0020, 0x4000 },
    { "LULL",  2, 9, 3, 0, 9, 121, 97, 35, 27, 31, 55, 88,  91, 0x9111, 0x1210, 0xAAAA, 0xF57D, 0x0120, 0x0800 },
{ "HANDED", 3, 0, 1, 0, 28, 122, 100, 36, 32, 14, 28,  8,  82, 0x1111, 0x1010, 0x4444, 0x2409, 0x0101, 0x8000 },
{ "BLUEV", 3, 5, 0, 0, 26, 120, 98, 34, 30, 12, 26,  6,  80, 0x1111, 0x9010, 0x5555, 0x4290, 0x0401, 0x0004 },
{ "DEEPKC", 3, 0, 1, 1, 30, 124, 102, 38, 34, 16, 30, 10,  84, 0x1111, 0x1010, 0x2222, 0x0849, 0x1004, 0x0080 },
{ "CHORDS", 3, 5, 4, 0, 32, 121, 96, 40, 36, 18, 32,  7,  78, 0x5111, 0x1210, 0x4444, 0x2904, 0x0110, 0x4000 },
{ "DEEPCHD", 3, 0, 0, 0, 24, 118, 94, 32, 28, 10, 24,  5,  76, 0x1111, 0x1010, 0x5555, 0x4085, 0x0201, 0x0040 },
{ "NUYORQ", 3, 5, 1, 1, 28, 123, 99, 36, 32, 14, 28,  9,  82, 0x1111, 0x1090, 0x4444, 0x8421, 0x0801, 0x2000 },
    { "DEEPH",  3, 7, 1, 0, 27, 121, 97, 35, 31, 13, 27,  8,  79, 0x1111, 0x9010, 0x5555, 0x9042, 0x0204, 0x0020 },
    { "CHOSEN",  3, 0, 0, 1, 29, 123, 101, 37, 33, 15, 29, 11,  83, 0x5111, 0x1010, 0x2222, 0x4908, 0x1008, 0x0080 },
    { "DEEPER",  3, 5, 1, 0, 27, 118, 77, 47, 43, 13, 27,  8,  79, 0x1111, 0x9010, 0x0101, 0x9042, 0x0204, 0x0020 },
    { "FLOAT",  3, 0, 0, 1, 29, 116, 81, 49, 45, 15, 29, 11,  83, 0x5111, 0x1010, 0x0101, 0x4908, 0x1008, 0x0080 },
{ "DISCOF", 4, 8, 0, 5, 88, 118, 108, 42, 38, 24, 48,  4, 110, 0x1111, 0x1010, 0x4444, 0x4444, 0x4444, 0x8000 },
{ "PHOENX", 4, 8, 4, 5, 92, 120, 106, 44, 40, 22, 50,  6, 108, 0x1111, 0x9010, 0x5555, 0x5454, 0x4404, 0x0004 },
{ "STARTRX", 4, 8, 3, 4, 96, 122, 110, 46, 42, 26, 52,  8, 112, 0x1111, 0x1010, 0x4646, 0x4646, 0x2424, 0x0080 },
{ "LATINH", 4, 8, 0, 5, 90, 119, 104, 40, 36, 20, 46,  5, 106, 0x9111, 0x1210, 0x6464, 0x6464, 0x4242, 0x4000 },
{ "MOTORX", 4, 8, 4, 4, 94, 121, 107, 43, 39, 23, 49,  7, 109, 0x1111, 0x5018, 0x4444, 0x5544, 0x0444, 0x0800 },
{ "FRENCHF", 4, 8, 3, 5, 98, 123, 111, 45, 41, 25, 51,  9, 111, 0x1111, 0x1010, 0x5454, 0x4545, 0x4044, 0x8080 },
    { "ROTATE",  4, 8, 4, 5, 91, 119, 105, 41, 37, 21, 47,  6, 107, 0x1111, 0x9010, 0x5454, 0x4554, 0x4244, 0x0040 },
    { "MODERN",  4, 8, 3, 4, 97, 124, 109, 47, 43, 24, 53, 10, 113, 0x1111, 0x1010, 0x4646, 0x6446, 0x2420, 0x0100 },
    { "FILTER",  4, 8, 4, 5, 91, 116, 85, 53, 49, 21, 47,  6, 107, 0x1111, 0x9010, 0x0101, 0x4554, 0x4244, 0x0040 },
    { "WARM",  4, 8, 3, 4, 97, 118, 89, 59, 55, 24, 53, 10, 113, 0x1111, 0x1010, 0x0202, 0x6446, 0x2420, 0x0100 },
{ "MINIMAL", 5, 7, 2, 2, 52, 126, 92, 30, 26, 16, 34, 18,  74, 0x1111, 0x1010, 0x4444, 0x8420, 0x0800, 0x8000 },
{ "TEUTO", 5, 7, 1, 3, 54, 128, 90, 32, 28, 14, 36, 16,  72, 0x1111, 0x9010, 0x2222, 0x4208, 0x0020, 0x0004 },
{ "STALLN", 5, 7, 2, 2, 50, 125, 94, 28, 24, 18, 32, 20,  76, 0x1111, 0x1010, 0x4444, 0x2840, 0x2008, 0x0080 },
{ "SYNCOP", 5, 7, 0, 3, 56, 130, 88, 34, 30, 12, 38, 14,  70, 0x5111, 0x1210, 0x5555, 0x0A28, 0x0208, 0x4000 },
{ "GROOVYW", 5, 7, 2, 2, 48, 127, 91, 30, 26, 16, 34, 22,  73, 0x1111, 0x1010, 0x4444, 0x6048, 0x0082, 0x0040 },
{ "NORDIC", 5, 7, 1, 3, 58, 129, 89, 32, 28, 14, 36, 17,  71, 0x1111, 0x1090, 0x2222, 0x4806, 0x0802, 0x2000 },
    { "PERU",  5, 7, 2, 2, 51, 127, 93, 29, 25, 17, 33, 19,  75, 0x1111, 0x1010, 0x4444, 0x4028, 0x2004, 0x0040 },
    { "TECHNO",  5, 7, 1, 3, 55, 130, 87, 33, 29, 13, 37, 15,  69, 0x5111, 0x1090, 0x2222, 0x0684, 0x0804, 0x0080 },
    { "CIRCUIT",  5, 7, 2, 2, 51, 124, 73, 41, 37, 17, 33, 19,  75, 0x1111, 0x1010, 0x0101, 0x4028, 0x2004, 0x0040 },
    { "MODULAR",  5, 7, 1, 3, 55, 126, 67, 45, 41, 13, 37, 15,  69, 0x5111, 0x1090, 0x0101, 0x0684, 0x0804, 0x0080 },

};

static const int8_t HOUSE_TRAX_ROOT[HOUSE_TRAX_N] = {48, 50, 51, 53, 55, 57, 52, 54, 58, 60, 58, 60, 50, 53, 55, 48, 59, 61, 65, 67, 51, 48, 55, 53, 50, 57, 49, 52, 55, 58, 53, 55, 48, 51, 57, 50, 54, 56, 60, 62, 55, 53, 60, 58, 50, 53, 52, 54, 58, 60, 51, 48, 55, 53, 50, 57, 56, 58, 62, 64};

static const int8_t HOUSE_TRAX_SCALE[HOUSE_TRAX_N] = {0, 1, 2, 4, 5, 0, 1, 5, 7, 11, 1, 2, 4, 0, 5, 1, 4, 0, 10, 6, 0, 2, 1, 4, 5, 0, 0, 2, 6, 8, 1, 4, 0, 5, 2, 1, 1, 5, 7, 11, 4, 5, 0, 1, 2, 4, 4, 2, 10, 8, 2, 0, 5, 1, 4, 2, 0, 1, 6, 7};

static const int8_t HOUSE_TRAX_SWING[HOUSE_TRAX_N] = {57, 58, 58, 59, 60, 61, 62, 63, 68, 69, 62, 63, 64, 65, 66, 67, 68, 69, 74, 75, 54, 55, 56, 57, 57, 58, 57, 58, 63, 64, 58, 59, 60, 61, 62, 63, 60, 61, 66, 67, 56, 57, 57, 58, 58, 59, 56, 57, 62, 63, 54, 55, 56, 57, 57, 58, 54, 55, 60, 61};

/* Per-trax STAB SNAP (siren_rate) delta from era base; six trax span each zone. */
static const int8_t HOUSE_TRAX_STAB_RATE[HOUSE_TRAX_N] = {-5, -3, -1, 1, 3, 5, -4, -2, 2, 4, -6, -4, -2, 0, 2, 4, -3, -1, 3, 5, -5, -3, -1, 1, 3, 5, -4, -2, 2, 4, -4, -2, 0, 2, 3, 4, -3, -1, 3, 5, -7, -5, -3, -1, 1, 3, -6, -4, 0, 2, -6, -4, -2, 0, 2, 4, -5, -3, 1, 3};

static const uint8_t HOUSE_ERA_MACROS[HOUSE_ERA_N][10] = {
    { 72, 58, 24, 56, 48, 88, 32, 40, 0, 36 }, /* JACK */
    { 68, 62, 20, 52, 44, 92, 28, 38, 0, 42 }, /* GARAGE */
    { 64, 54, 88, 48, 40, 86, 44, 36, 0, 48 }, /* ACID */
    { 60, 56, 16, 50, 52, 84, 24, 42, 0, 34 }, /* DEEP */
    { 70, 60, 12, 54, 56, 88, 30, 44, 0, 50 }, /* FILTER */
    { 66, 58, 28, 58, 46, 90, 36, 40, 0, 46 }, /* TECH */
};

typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} house_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} house_era_fx_t;

/* sub_oct: 0=−1oct (was 32–36 unison — bass sat midrange). ACID 88=−2oct. */
static const house_era_base_t HOUSE_ERA_BASE[HOUSE_ERA_N] = {
    { 36, 92, 88, 20, 44, 68, 92, 0, 28, 24, 82, 58, 54, 64, 64, 18, 64, 23, 32, {64,72,48,80}, 8, 52, 32, 60 },
    { 40, 88, 86, 24, 48, 66, 90, 0, 26, 28, 78, 56, 58, 64, 68, 16, 62, 52, 36, {64,70,52,78}, 10, 48, 28, 62 },
    { 44, 84, 82, 40, 60, 72, 88, 88, 32, 18, 80, 54, 62, 64, 56, 24, 58, 57, 28, {64,74,50,82}, 6, 44, 40, 55 },
    { 32, 90, 90, 18, 40, 64, 96, 0, 24, 32, 76, 52, 50, 64, 72, 14, 60, 42, 40, {64,68,46,76}, 12, 56, 24, 58 },
    { 38, 94, 84, 28, 52, 70, 90, 0, 30, 26, 74, 60, 56, 64, 64, 20, 66, 48, 44, {62,76,50,84}, 14, 50, 36, 57 },
    { 42, 86, 86, 24, 38, 74, 92, 0, 26, 22, 72, 50, 48, 64, 60, 16, 56, 86, 30, {64,72,54,70}, 8, 46, 32, 56 },
};

static const house_era_fx_t HOUSE_ERA_FX[HOUSE_ERA_N] = {
    { DKIT_808,  22, 0, 42, 20, 72, 32, 34, 58, 48,  96, BASS_ORGAN,  8 },
    { DKIT_LINN, 44, 2, 48, 22, 78, 38, 36, 62, 52,  88, BASS_OCTAVE, 12 },
    { DKIT_606,  12, 0, 54, 32, 68, 28, 30, 52, 42,  86, BASS_303,   72 },
    { DKIT_909,  28, 1, 38, 16, 82, 42, 40, 66, 54,  82, BASS_MOOG,   6 },
    { DKIT_CR78, 92, 5, 52, 24, 88, 48, 46, 72, 62, 110, BASS_PWM,    4 },
    { DKIT_TECH, 52, 3, 44, 18, 74, 34, 32, 56, 46,  74, BASS_FM,    18 },
};

/* Bass melodic contour: era × in-era slot × 8-step scale degree (0..6). */
static const uint8_t HOUSE_BASS_WALK[HOUSE_ERA_N][GENRE_PRESETS_PER_ERA][8] = {
    { { 5, 5, 3, 3, 0, 0, 4, 4 }, { 0, 0, 5, 5, 3, 3, 0, 4 }, { 5, 3, 0, 0, 4, 4, 3, 0 }, { 0, 4, 0, 3, 5, 0, 4, 3 }, { 3, 0, 5, 5, 0, 4, 3, 0 }, { 5, 5, 0, 3, 3, 0, 4, 4 }, { 0, 3, 5, 0, 4, 3, 0, 5 }, { 4, 0, 3, 5, 0, 0, 5, 3 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 3, 0, 0, 5, 0, 3 }, { 0, 3, 0, 0, 5, 0, 3, 0 }, { 0, 0, 0, 3, 0, 5, 0, 0 }, { 3, 0, 0, 5, 0, 0, 3, 0 }, { 0, 5, 0, 3, 0, 0, 0, 5 }, { 0, 0, 5, 0, 3, 0, 0, 3 }, { 5, 0, 0, 3, 0, 5, 0, 0 }, { 0, 3, 0, 5, 0, 0, 3, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 2, 0, 0, 2, 0, 0 }, { 0, 2, 0, 2, 0, 0, 2, 0 }, { 0, 0, 0, 2, 0, 0, 0, 2 }, { 0, 2, 2, 0, 0, 2, 2, 0 }, { 0, 0, 4, 0, 0, 4, 0, 0 }, { 0, 4, 0, 4, 0, 2, 0, 2 }, { 0, 2, 4, 2, 0, 2, 4, 2 }, { 0, 0, 2, 4, 0, 0, 2, 4 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 3, 0, 0, 5, 0 }, { 0, 0, 3, 0, 0, 5, 0, 0 }, { 0, 3, 0, 0, 5, 0, 0, 3 }, { 0, 0, 5, 0, 3, 0, 0, 0 }, { 0, 5, 0, 0, 0, 3, 0, 5 }, { 3, 0, 0, 0, 0, 5, 0, 3 }, { 0, 0, 3, 5, 0, 0, 0, 3 }, { 0, 3, 0, 0, 0, 3, 5, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 3, 5, 3, 0, 4, 5, 0 }, { 0, 4, 3, 5, 0, 3, 4, 0 }, { 3, 0, 5, 4, 3, 0, 5, 0 }, { 0, 3, 0, 5, 4, 0, 3, 5 }, { 5, 0, 3, 0, 5, 4, 0, 3 }, { 0, 5, 3, 0, 4, 5, 0, 3 }, { 3, 5, 0, 4, 0, 3, 5, 0 }, { 0, 3, 4, 5, 3, 0, 4, 5 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 2, 0, 4, 2, 0, 5, 2 }, { 0, 2, 4, 2, 0, 5, 2, 0 }, { 2, 0, 4, 5, 2, 0, 4, 0 }, { 0, 4, 2, 0, 5, 4, 2, 0 }, { 0, 2, 5, 4, 0, 2, 4, 5 }, { 4, 0, 2, 5, 4, 0, 2, 5 }, { 0, 5, 4, 2, 0, 4, 2, 5 }, { 2, 4, 0, 5, 2, 4, 0, 2 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } }
};

#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM trax at house_gen.slot. */
static fm_pgen_t house_gen = { .slot = 0xFFu };

#define FM_PGEN_STATE house_gen
#define FM_PGEN_BUILD(era, n, c) house_gen_build(era, n, c)

static inline uint8_t house_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&house_gen, ix);
}

static inline uint8_t house_clamp_trax(uint8_t ix)
{
    return ix >= HOUSE_TRAX_N ? (uint8_t)(HOUSE_TRAX_N - 1u) : ix;
}

static inline uint8_t house_clamp_era(uint8_t era)
{
    return era >= HOUSE_ERA_N ? (uint8_t)(HOUSE_ERA_N - 1u) : era;
}

static inline const char *house_era_name(uint8_t era)
{
    return HOUSE_ERA_NAMES[house_clamp_era(era)];
}

static inline const char *house_trax_name(uint8_t trax)
{
    if (house_gen_hit(trax))
        return house_gen.name;
    return HOUSE_TRAX[house_clamp_trax(trax)].name;
}

static inline const house_trax_t *house_trax(uint8_t ix)
{
    if (house_gen_hit(ix))
        return &house_gen.pl;
    return &HOUSE_TRAX[house_clamp_trax(ix)];
}

static inline uint8_t house_era_of_trax(uint8_t trax)
{
    trax = house_clamp_trax(trax);
    return (uint8_t)(trax / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t house_era_first_trax(uint8_t era)
{
    return (uint8_t)(house_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint8_t house_bass_walk_deg(uint8_t era, uint8_t trax, uint8_t step)
{
    era = house_clamp_era(era);
    trax = (uint8_t)(house_clamp_trax(trax) % GENRE_PRESETS_PER_ERA);
    return HOUSE_BASS_WALK[era][trax][step & 7u];
}

static const int8_t HOUSE_TRAX_KIT[HOUSE_TRAX_N] = {0, 0, 2, 2, 0, 2, 0, 2, 6, 8, 4, 4, 4, 4, 6, 6, 4, 6, 10, 12, 3, 3, 3, 3, 3, 3, 3, 3, 9, 9, 1, 1, 1, 1, 1, 1, 1, 1, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 11, 11, 7, 7, 7, 7, 7, 7, 7, 7, 13, 13};

static inline uint8_t house_trax_root(uint8_t ix)
{
    return house_gen_hit(ix) ? (uint8_t)house_gen.root : (uint8_t)HOUSE_TRAX_ROOT[house_clamp_trax(ix)];
}

static inline uint8_t house_trax_scale(uint8_t ix)
{
    return house_gen_hit(ix) ? (uint8_t)house_gen.scale : (uint8_t)HOUSE_TRAX_SCALE[house_clamp_trax(ix)];
}

static inline uint8_t house_trax_swing(uint8_t ix)
{
    return house_gen_hit(ix) ? (uint8_t)house_gen.swing : (uint8_t)HOUSE_TRAX_SWING[house_clamp_trax(ix)];
}

static inline uint8_t house_trax_kit(uint8_t ix)
{
    return house_gen_hit(ix) ? (uint8_t)house_gen.kit : (uint8_t)HOUSE_TRAX_KIT[house_clamp_trax(ix)];
}

static inline int8_t house_trax_stab_rate(uint8_t ix)
{
    return house_gen_hit(ix) ? house_gen.rate : HOUSE_TRAX_STAB_RATE[house_clamp_trax(ix)];
}

static inline void house_apply_defaults(dub_params_t *p)
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
    p->drum_kit = DKIT_808;
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

static inline void house_era_apply_base(dub_params_t *p, uint8_t era)
{
    const house_era_base_t *eb;
    const house_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = house_clamp_era(era);
    eb = &HOUSE_ERA_BASE[era];
    fx = &HOUSE_ERA_FX[era];
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
static const uint8_t house_drm_dec_fit[GENRE_PRESET_N][4] = {
    { 29,  40,  64, 64}, { 28,  39,  64, 64}, { 27,  40,  72, 63}, { 29,  41,  67, 64},
    { 27,  41,  64, 64}, { 29,  39,  67, 64}, { 28,  40,  64, 64}, { 26,  39,  67, 64},
    { 29,  39,  60, 65}, { 27,  40,  72, 63}, { 33,  40,  68, 63}, { 33,  42,  67, 64},
    { 33,  43,  72, 63}, { 32,  43,  72, 63}, { 32,  41,  88, 62}, { 32,  43,  88, 64},
    { 31,  41,  72, 63}, { 33,  43,  92, 62}, { 32,  41,  72, 63}, { 32,  42,  67, 64},
    { 31,  40,  67, 64}, { 31,  45, 104, 63}, { 28,  44,  72, 63}, { 28,  45, 100, 63},
    { 28,  42,  96, 64}, { 30,  44,  96, 64}, { 30,  42,  72, 63}, { 29,  45, 104, 63},
    { 28,  43,  68, 63}, { 30,  43,  72, 63}, { 34,  44,  72, 63}, { 35,  46,  68, 63},
    { 35,  47,  72, 63}, { 35,  45,  67, 64}, { 34,  46,  72, 63}, { 35,  44,  67, 64},
    { 35,  45,  68, 63}, { 33,  46,  67, 64}, { 35,  44,  97, 64}, { 34,  44,  99, 64},
    { 37,  37,  72, 63}, { 34,  38,  72, 63}, { 36,  39,  80, 63}, { 37,  36,  72, 63},
    { 36,  37,  79, 63}, { 36,  38,  72, 63}, { 37,  38,  75, 63}, { 37,  39,  75, 63},
    { 37,  38,  68, 63}, { 37,  39,  67, 64}, { 37,  49,  68, 63}, { 39,  48,  65, 64},
    { 39,  46,  67, 64}, { 39,  47,  67, 64}, { 36,  46,  72, 63}, { 39,  48,  68, 63},
    { 37,  49,  64, 64}, { 39,  48,  68, 63}, { 36,  48,  43, 66}, { 37,  48,  51, 66}
};
#define GENRE_DRM_DEC_FIT house_drm_dec_fit

static inline void house_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = house_drm_dec_fit[preset][i];
}

static inline void house_trax_apply(dub_params_t *p, uint8_t trax, uint8_t what)
{
    const house_trax_t *tx;
    uint8_t di;

    if (!p)
        return;
    trax = house_clamp_trax(trax);
    tx = house_trax(trax);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        house_apply_defaults(p);
        house_era_apply_base(p, tx->era);
        house_drm_dec_fit_apply(p, trax);
    }
    p->era = tx->era;
    p->plate = trax;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = tx->bpm;
        p->density = 64u;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = 64u;
        p->drum_kit = house_trax_kit(trax);
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
        p->root = house_trax_root(trax);
        p->scale = house_trax_scale(trax);
        p->swing = house_trax_swing(trax);
        {
            int16_t stab = (int16_t)p->siren_rate + (int16_t)house_trax_stab_rate(trax);

            if (stab < 0)
                stab = 0;
            else if (stab > 127)
                stab = 127;
            p->siren_rate = (uint8_t)stab;
        }
    }
}

static inline void house_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t trax)
{
    if (!p)
        return;
    era = house_clamp_era(era);
    trax = house_clamp_trax(trax);
    house_trax_apply(p, trax, DUB_APPLY_ALL);
    p->era = era;
    p->plate = trax;
    p->drum_kit = house_trax_kit(trax);
    p->bass_model = house_trax(trax)->bass_model;
    p->voicing = house_trax(trax)->voicing;
    p->bpm = house_trax(trax)->bpm;
    p->pump = house_trax(trax)->pump;
    p->feedback = house_trax(trax)->feedback;
    p->wobble = house_trax(trax)->wobble;
    p->drum_pat = trax;
    p->root = house_trax_root(trax);
    p->scale = house_trax_scale(trax);
    p->swing = house_trax_swing(trax);
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
    const house_trax_t *tx;

    plate = house_clamp_trax(plate);
    tx = house_trax(plate);
    if (root)
        *root = house_trax_root(plate);
    if (scale)
        *scale = house_trax_scale(plate);
    if (swing)
        *swing = house_trax_swing(plate);
    if (bpm)
        *bpm = tx->bpm;
}

static inline uint8_t house_density_allows(uint8_t density, uint8_t step, uint8_t trax_ix)
{
    if (step == 0u || step == 4u || step == 8u || step == 12u)
        return density >= 16u;
    return ((step * 17u + trax_ix) & 127u) < density;
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out)
{
    const house_trax_t *drum;
    const house_trax_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = house_trax(p->drum_pat);
    main = house_trax(p->plate);
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
enum { HOUSE_STAB_GATE_LEN_N = 15u };
static const char *const HOUSE_STAB_GATE_LEN_NAMES[HOUSE_STAB_GATE_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t HOUSE_STAB_GATE_LEN_NUM[HOUSE_STAB_GATE_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t HOUSE_STAB_GATE_LEN_DEN[HOUSE_STAB_GATE_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
static const uint8_t HOUSE_STAB_GATE_LEN_16THS[HOUSE_STAB_GATE_LEN_N] = {
    1u, 2u, 4u, 5u, 8u, 11u, 12u, 16u, 20u, 24u, 28u, 32u, 40u, 48u, 64u
};

static inline uint8_t house_stab_gate_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)HOUSE_STAB_GATE_LEN_N)
        ix = (int32_t)HOUSE_STAB_GATE_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t house_stab_gate_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = house_stab_gate_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)HOUSE_STAB_GATE_LEN_16THS[ix];
}

static inline uint8_t house_stab_bend_semi_u8(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

/* Per-step hat/rim velocity + micro (Chicago shuffle); pattern gates unchanged. */
static inline void house_groove_step_plock(uint8_t ln, uint8_t st, const house_trax_t *drum,
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

static inline uint32_t house_micro_delay_samples(int8_t micro, uint32_t samples_per_step)
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

static inline int8_t house_stab_bend_delta(uint8_t from_step, int8_t cur_tune,
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

#include "house_gen.h"

#endif
