/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HEAVY_PSYCH_DATA_H
#define HEAVY_PSYCH_DATA_H

#include "dub_types.h"
#include "genre_conf.h"
#include "fm_seq.h"

enum { HEAVY_PSYCH_ERA_N = GENRE_ERA_N, HEAVY_PSYCH_TRAX_N = GENRE_PRESET_N };
enum { ERA_STONER = 0, ERA_DESERT, ERA_SPACE, ERA_DRONE, ERA_GAZE, ERA_DOOM };
enum {
    BASS_SUB = 0, BASS_DOOM, BASS_FUZZ, BASS_OCT, BASS_WAH, BASS_MOOG, BASS_MUFF,
    BASS_REESE, BASS_303, BASS_FM, BASS_PWM, BASS_PLUCK, BASS_DRONE,
    BASS_N
};
enum {
    DKIT_FUZZ = 0, DKIT_DOOM, DKIT_DESERT, DKIT_SPACE, DKIT_DRONE, DKIT_GAZE, DKIT_CR78, DKIT_TECH, DKIT_N
};
enum { VOICE_PWR5 = 0, VOICE_MAJ, VOICE_SUS2, VOICE_OCT, VOICE_MIN, VOICE_N };

typedef dub_plate_t heavy_psych_trax_t;

static const char *const HEAVY_PSYCH_ERA_NAMES[HEAVY_PSYCH_ERA_N] = {
    "STONER", "DESERT", "SPACE", "DRONE", "GAZE", "DOOM"
};

static const char *const DKIT_NAMES[DKIT_N] = {
    "FUZZ", "DOOM", "DESERT", "SPACE", "DRONE", "GAZE", "CR79", "TECH"
};

static const uint8_t DKIT_ERA[DKIT_N] = {
    ERA_STONER, ERA_DOOM, ERA_DESERT, ERA_SPACE, ERA_DRONE, ERA_GAZE, ERA_DRONE, ERA_SPACE
};

enum {
    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS
};

/* +12…+18 st: top partial (+13) lands at 3–10 kHz instead of cowbell range */
static const int8_t heavy_psych_hat_note[DKIT_N] = { 93, 90, 99, 108, 84, 103, 93, 114 };
static const uint8_t heavy_psych_hat_fm[DKIT_N] = { 18, 8, 12, 72, 6, 24, 6, 82 };
static const uint8_t heavy_psych_hat_noise[DKIT_N] = { 60, 48, 38, 52, 22, 56, 14, 68 };
static const uint8_t heavy_psych_hat_metal[DKIT_N] = { 108, 96, 88, 72, 120, 42, 120, 88 };
static const uint8_t heavy_psych_hat_decay[DKIT_N] = { 24, 18, 17, 16, 25, 16, 24, 16 };
static const uint8_t heavy_psych_hat_open[DKIT_N] = { 2, 12, 5, 10, 5, 15, 9, 9 };
static const uint8_t heavy_psych_hat_style[DKIT_N] = {
    HAT_STYLE_SIZZLE, HAT_STYLE_TICK, HAT_STYLE_TONE, HAT_STYLE_SIZZLE,
    HAT_STYLE_TONE, HAT_STYLE_HISS, HAT_STYLE_TONE, HAT_STYLE_SIZZLE
};
static const uint8_t heavy_psych_hat_cut[DKIT_N] = { 44, 38, 52, 46, 34, 34, 38, 50 };

static const char *const BASS_NAMES[BASS_N] = {
    "SUB", "DOOM", "FUZZ", "OCT", "WAH", "MOCK", "MUFX", "REESE", "3OE", "FM", "PWM", "PLUCK", "DRONE"
};

static const uint8_t BASS_ERA[BASS_N] = {
    ERA_DOOM, ERA_DOOM, ERA_STONER, ERA_DESERT, ERA_STONER, ERA_GAZE, ERA_DOOM, ERA_DESERT,
    ERA_STONER, ERA_SPACE, ERA_GAZE, ERA_SPACE, ERA_DRONE
};

static const char *const VOICE_NAMES[VOICE_N] = { "PWR5", "MAJ", "SUS2", "OCT", "MIN" };

static const heavy_psych_trax_t HEAVY_PSYCH_TRAX[HEAVY_PSYCH_TRAX_N] = {
    { "FUZZ", 0, 2, 0, 0, 18, 78, 100, 34, 32, 32, 38,  2,  40, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0511u | ((uint32_t)0x0913u << 16))), (0x0101u | ((uint32_t)0x0101u << 16)), (0x0202u | ((uint32_t)0x0202u << 16)) },
    { "RIFTER", 0, 0, 1, 1, 18, 80, 103, 34, 32, 32, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0411u | ((uint32_t)0x0519u << 16))), (0x0001u | ((uint32_t)0x0100u << 16)), (0x0101u | ((uint32_t)0x0101u << 16)) },
    { "SLAB", 0, 4, 2, 2, 18, 83, 106, 34, 32, 32, 38,  6,  96, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0509u | ((uint32_t)0x0411u << 16))), (0x0101u | ((uint32_t)0x0202u << 16)), (0x0004u | ((uint32_t)0x0004u << 16)) },
    { "HASH", 0, 0, 3, 0, 18, 86, 109, 34, 32, 32, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0115u | ((uint32_t)0x0511u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "CRAWL", 0, 6, 4, 1, 18, 89, 112, 34, 32, 32, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0713u | ((uint32_t)0x0311u << 16))), (0x0001u | ((uint32_t)0x0001u << 16)), (0x0080u | ((uint32_t)0x0080u << 16)) },
    { "WEED", 0, 7, 0, 2, 18, 92, 115, 34, 32, 32, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0510u | ((uint32_t)0x0911u << 16))), (0x0101u | ((uint32_t)0x0101u << 16)), (0x0040u | ((uint32_t)0x0040u << 16)) },
    { "TAR", 0, 1, 2, 1, 20, 94, 118, 36, 34, 34, 40,  8,  98, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0409u | ((uint32_t)0x0611u << 16))), (0x0404u | ((uint32_t)0x0101u << 16)), (0x0020u | ((uint32_t)0x0020u << 16)) },
    { "RESIN", 0, 3, 1, 0, 16, 96, 116, 32, 30, 30, 36,  4,  94, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0615u | ((uint32_t)0x0401u << 16))), (0x0202u | ((uint32_t)0x0101u << 16)), (0x0048u | ((uint32_t)0x0048u << 16)) },
    { "CINDR", 0, 1, 2, 1, 20, 80, 98, 48, 46, 34, 40,  8,  98, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0409u | ((uint32_t)0x0611u << 16))), (0x0404u | ((uint32_t)0x0101u << 16)), (0x0020u | ((uint32_t)0x0020u << 16)) },
    { "HAZE", 0, 3, 1, 0, 16, 82, 96, 44, 42, 30, 36,  4,  94, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0615u | ((uint32_t)0x0401u << 16))), (0x0202u | ((uint32_t)0x0101u << 16)), (0x0048u | ((uint32_t)0x0048u << 16)) },
    { "DUNE", 1, 2, 0, 0, 28, 89, 102, 36, 36, 34, 38,  2,  40, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0101u | ((uint32_t)0x0401u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "SAND", 1, 0, 1, 1, 28, 92, 105, 36, 36, 34, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0104u << 16))), (0x0101u | ((uint32_t)0x0101u << 16)), (0x0002u | ((uint32_t)0x0002u << 16)) },
    { "MIRAGE", 1, 4, 2, 2, 28, 94, 108, 36, 36, 34, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0100u | ((uint32_t)0x0010u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0x0010u | ((uint32_t)0x0010u << 16)) },
    { "HEAT", 1, 0, 3, 0, 28, 97, 111, 36, 36, 34, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0401u | ((uint32_t)0x0001u << 16))), (0x0808u | ((uint32_t)0x0808u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "DUST", 1, 6, 4, 1, 28, 100, 114, 36, 36, 34, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0010u | ((uint32_t)0x0101u << 16))), (0x1010u | ((uint32_t)0x1010u << 16)), (0x0004u | ((uint32_t)0x0004u << 16)) },
    { "OASIS", 1, 7, 0, 2, 28, 102, 117, 36, 36, 34, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0104u | ((uint32_t)0x0100u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0008u | ((uint32_t)0x0008u << 16)) },
    { "SCORCH", 1, 3, 3, 1, 26, 105, 104, 40, 40, 32, 36,  8,  94, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0004u | ((uint32_t)0x0404u << 16))), (0x0408u | ((uint32_t)0x0408u << 16)), (0x0010u | ((uint32_t)0x0010u << 16)) },
    { "WADI", 1, 1, 4, 2, 30, 108, 108, 38, 36, 36, 40, 10,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0400u | ((uint32_t)0x0014u << 16))), (0x0810u | ((uint32_t)0x0810u << 16)), (0x0020u | ((uint32_t)0x0020u << 16)) },
    { "MESA", 1, 3, 3, 1, 26, 86, 84, 52, 52, 32, 36,  8,  94, (0x0041u | ((uint32_t)0x0041u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0004u | ((uint32_t)0x0404u << 16))), (0x0408u | ((uint32_t)0x0408u << 16)), (0x0010u | ((uint32_t)0x0010u << 16)) },
    { "SIMMR", 1, 1, 4, 2, 30, 89, 88, 50, 48, 36, 40, 10,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0400u | ((uint32_t)0x0014u << 16))), (0x0810u | ((uint32_t)0x0810u << 16)), (0x0020u | ((uint32_t)0x0020u << 16)) },
    { "ORBIT", 2, 2, 0, 0, 52, 100, 104, 44, 44, 44, 38,  2,  40, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0010u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0x8888u | ((uint32_t)0x8888u << 16)) },
    { "NEBULA", 2, 0, 1, 1, 52, 104, 107, 44, 44, 44, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0010u | ((uint32_t)0x0001u << 16))), (0x0808u | ((uint32_t)0x0808u << 16)), (0xAAAAu | ((uint32_t)0xAAAAu << 16)) },
    { "PULSAR", 2, 4, 2, 2, 52, 108, 110, 44, 44, 44, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0000u | ((uint32_t)0x0100u << 16))), (0x1010u | ((uint32_t)0x1010u << 16)), (0xCCCCu | ((uint32_t)0xCCCCu << 16)) },
    { "VOID", 2, 0, 3, 0, 52, 112, 113, 44, 44, 44, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0100u | ((uint32_t)0x0000u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x6666u | ((uint32_t)0x6666u << 16)) },
    { "STAR", 2, 6, 4, 1, 52, 116, 116, 44, 44, 44, 38,  6,  96, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0800u << 16))), (0x0400u | ((uint32_t)0x0400u << 16)), (0xF0F0u | ((uint32_t)0xF0F0u << 16)) },
    { "COMET", 2, 7, 0, 2, 52, 121, 119, 44, 44, 44, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0800u | ((uint32_t)0x0001u << 16))), (0x0804u | ((uint32_t)0x0804u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)) },
    { "QUASAR", 2, 8, 1, 3, 48, 119, 112, 50, 50, 48, 42,  8, 100, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0004u | ((uint32_t)0x0400u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0xBBBBu | ((uint32_t)0xBBBBu << 16)) },
    { "FLARE", 2, 3, 5, 1, 56, 124, 108, 48, 48, 50, 44, 12,  98, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0400u | ((uint32_t)0x0004u << 16))), (0x0808u | ((uint32_t)0x0808u << 16)), (0xDDDDu | ((uint32_t)0xDDDDu << 16)) },
    { "LUNAR", 2, 8, 1, 3, 48, 100, 92, 62, 62, 48, 42,  8, 100, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0004u | ((uint32_t)0x0400u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0xBBBBu | ((uint32_t)0xBBBBu << 16)) },
    { "ECLIP", 2, 3, 5, 1, 56, 105, 88, 60, 60, 50, 44, 12,  98, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0400u | ((uint32_t)0x0004u << 16))), (0x0808u | ((uint32_t)0x0808u << 16)), (0xDDDDu | ((uint32_t)0xDDDDu << 16)) },
    { "BOHM", 3, 0, 0, 0, 8, 60, 106, 46, 48, 46, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0000u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "HUM", 3, 0, 1, 1, 8, 62, 109, 46, 48, 46, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0000u | ((uint32_t)0x0001u << 16))), (0x0101u | ((uint32_t)0x0101u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "LOWEND", 3, 6, 2, 2, 8, 64, 112, 46, 48, 46, 38,  6,  96, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0100u | ((uint32_t)0x0100u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0x0004u | ((uint32_t)0x0004u << 16)) },
    { "EMPTY", 3, 0, 3, 0, 8, 66, 115, 46, 48, 46, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0010u | ((uint32_t)0x0000u << 16))), (0x0808u | ((uint32_t)0x0808u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "BOW", 3, 4, 4, 1, 8, 68, 118, 46, 48, 46, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0100u << 16))), (0x1010u | ((uint32_t)0x1010u << 16)), (0x0002u | ((uint32_t)0x0002u << 16)) },
    { "SWELL", 3, 0, 0, 2, 8, 70, 121, 46, 48, 46, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0000u | ((uint32_t)0x0010u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "RUMBLE", 3, 0, 2, 0, 6, 58, 104, 42, 44, 50, 34,  4,  92, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0010u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0001u | ((uint32_t)0x0001u << 16)) },
    { "THROB", 3, 6, 3, 1, 10, 60, 108, 44, 46, 48, 36,  6,  94, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0010u | ((uint32_t)0x0001u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0x0008u | ((uint32_t)0x0008u << 16)) },
    { "GRIT", 3, 0, 2, 0, 6, 54, 84, 54, 56, 50, 34,  4,  92, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0010u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0001u | ((uint32_t)0x0001u << 16)) },
    { "SLOW", 3, 6, 3, 1, 10, 54, 88, 56, 58, 48, 36,  6,  94, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0010u | ((uint32_t)0x0001u << 16))), (0x0404u | ((uint32_t)0x0404u << 16)), (0x0008u | ((uint32_t)0x0008u << 16)) },
    { "WASH", 4, 2, 0, 0, 92, 105, 108, 38, 38, 30, 38,  6,  96, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0208u | ((uint32_t)0x0802u << 16))), (0x0001u | ((uint32_t)0x0001u << 16)), (0x0202u | ((uint32_t)0x0202u << 16)) },
    { "SHOE", 4, 0, 1, 1, 92, 108, 111, 38, 38, 30, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0802u | ((uint32_t)0x0208u << 16))), (0x0008u | ((uint32_t)0x0008u << 16)), (0x0101u | ((uint32_t)0x0101u << 16)) },
    { "FEED", 4, 4, 2, 2, 92, 111, 114, 38, 38, 30, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0240u | ((uint32_t)0x4020u << 16))), (0x0101u | ((uint32_t)0x0101u << 16)), (0x0004u | ((uint32_t)0x0004u << 16)) },
    { "MIST", 4, 0, 3, 0, 92, 114, 117, 38, 38, 30, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x4020u | ((uint32_t)0x0240u << 16))), (0x0001u | ((uint32_t)0x0100u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "BLOOM", 4, 6, 4, 1, 92, 116, 120, 38, 38, 30, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0082u | ((uint32_t)0x8200u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0080u | ((uint32_t)0x0080u << 16)) },
    { "ECHO", 4, 7, 0, 2, 92, 120, 123, 38, 38, 30, 38,  6,  96, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x8200u | ((uint32_t)0x0082u << 16))), (0x0010u | ((uint32_t)0x0010u << 16)), (0x0040u | ((uint32_t)0x0040u << 16)) },
    { "SHIMR", 4, 8, 2, 3, 88, 109, 110, 46, 46, 26, 40, 10,  90, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0420u | ((uint32_t)0x2040u << 16))), (0x0101u | ((uint32_t)0x0001u << 16)), (0x0102u | ((uint32_t)0x0102u << 16)) },
    { "GLARE", 4, 2, 4, 2, 96, 113, 114, 44, 44, 28, 42, 14,  92, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x2040u | ((uint32_t)0x0420u << 16))), (0x0004u | ((uint32_t)0x0004u << 16)), (0x0204u | ((uint32_t)0x0204u << 16)) },
    { "SHOEG", 4, 8, 2, 3, 88, 96, 90, 58, 58, 26, 40, 10,  90, (0x0041u | ((uint32_t)0x0041u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0420u | ((uint32_t)0x2040u << 16))), (0x0101u | ((uint32_t)0x0001u << 16)), (0x0102u | ((uint32_t)0x0102u << 16)) },
    { "SMOK", 4, 2, 4, 2, 96, 100, 94, 56, 56, 28, 42, 14,  92, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x2040u | ((uint32_t)0x0420u << 16))), (0x0004u | ((uint32_t)0x0004u << 16)), (0x0204u | ((uint32_t)0x0204u << 16)) },
    { "GRAVE", 5, 0, 0, 0, 6, 50, 110, 30, 34, 44, 32,  4, 100, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0001u | ((uint32_t)0x0000u << 16))), (0x0001u | ((uint32_t)0x0001u << 16)), (0x0000u | ((uint32_t)0x0000u << 16)) },
    { "PIT", 5, 2, 1, 0, 8, 53, 112, 32, 36, 42, 34,  6, 102, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0001u | ((uint32_t)0x0100u << 16))), (0x0008u | ((uint32_t)0x0008u << 16)), (0x0004u | ((uint32_t)0x0004u << 16)) },
    { "SLUDGE", 5, 1, 2, 1, 10, 56, 114, 34, 38, 40, 36,  8, 104, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0100u | ((uint32_t)0x0001u << 16))), (0x0010u | ((uint32_t)0x0010u << 16)), (0x0008u | ((uint32_t)0x0008u << 16)) },
    { "MIRE", 5, 0, 3, 0, 12, 59, 116, 36, 40, 38, 38, 10, 106, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0000u | ((uint32_t)0x0001u << 16))), (0x0001u | ((uint32_t)0x0100u << 16)), (0x0010u | ((uint32_t)0x0010u << 16)) },
    { "MAW", 5, 4, 4, 2, 14, 63, 118, 38, 42, 36, 40, 12, 108, (0x0041u | ((uint32_t)0x0041u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0317u | ((uint32_t)0x0001u << 16))), (0x0202u | ((uint32_t)0x0202u << 16)), (0x0020u | ((uint32_t)0x0020u << 16)) },
    { "BONE", 5, 0, 0, 1, 16, 66, 120, 40, 44, 34, 42, 14, 110, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x1111u | ((uint32_t)0x1111u << 16)), ((0x0001u | ((uint32_t)0x0300u << 16))), (0x0001u | ((uint32_t)0x0010u << 16)), (0x0040u | ((uint32_t)0x0040u << 16)) },
    { "RIFT", 5, 2, 1, 0, 18, 69, 122, 42, 46, 32, 44, 16, 112, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0300u | ((uint32_t)0x0000u << 16))), (0x0100u | ((uint32_t)0x0100u << 16)), (0x0080u | ((uint32_t)0x0080u << 16)) },
    { "ABYSS", 5, 1, 2, 1, 20, 72, 124, 44, 48, 30, 46, 18, 114, (0x0101u | ((uint32_t)0x0101u << 16)), (0x1010u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0001u | ((uint32_t)0x0010u << 16))), (0x0001u | ((uint32_t)0x0008u << 16)), (0x0100u | ((uint32_t)0x0100u << 16)) },
    { "PSYCH", 5, 2, 1, 0, 18, 58, 102, 54, 58, 32, 44, 16, 112, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0300u | ((uint32_t)0x0000u << 16))), (0x0100u | ((uint32_t)0x0100u << 16)), (0x0080u | ((uint32_t)0x0080u << 16)) },
    { "TRIP", 5, 1, 2, 1, 20, 61, 104, 56, 60, 30, 46, 18, 114, (0x0101u | ((uint32_t)0x0101u << 16)), (0x0101u | ((uint32_t)0x1010u << 16)), (0x5555u | ((uint32_t)0x5555u << 16)), ((0x0001u | ((uint32_t)0x0010u << 16))), (0x0001u | ((uint32_t)0x0008u << 16)), (0x0100u | ((uint32_t)0x0100u << 16)) },

};

static const uint8_t HEAVY_PSYCH_TRAX_ROOT[HEAVY_PSYCH_TRAX_N] = {42u, 43u, 44u, 45u, 46u, 42u, 41u, 47u, 47u, 53u, 44u, 45u, 46u, 47u, 48u, 44u, 43u, 49u, 49u, 55u, 46u, 47u, 48u, 49u, 50u, 46u, 45u, 51u, 51u, 57u, 48u, 49u, 50u, 51u, 52u, 48u, 47u, 53u, 53u, 59u, 50u, 51u, 52u, 53u, 54u, 50u, 49u, 55u, 55u, 61u, 36u, 38u, 40u, 42u, 44u, 46u, 48u, 50u, 54u, 56u};

static const uint8_t HEAVY_PSYCH_TRAX_SCALE[HEAVY_PSYCH_TRAX_N] = {0u, 1u, 2u, 3u, 4u, 5u, 1u, 3u, 7u, 9u, 0u, 1u, 2u, 3u, 4u, 5u, 2u, 4u, 8u, 10u, 0u, 1u, 2u, 3u, 4u, 5u, 1u, 5u, 7u, 11u, 0u, 1u, 2u, 3u, 4u, 5u, 2u, 0u, 8u, 6u, 0u, 1u, 2u, 3u, 4u, 5u, 3u, 1u, 9u, 7u, 0u, 1u, 2u, 3u, 4u, 5u, 0u, 1u, 6u, 7u};

static const uint8_t HEAVY_PSYCH_TRAX_SWING[HEAVY_PSYCH_TRAX_N] = {51u, 51u, 52u, 50u, 51u, 52u, 51u, 50u, 57u, 56u, 53u, 54u, 53u, 52u, 54u, 53u, 52u, 54u, 58u, 60u, 50u, 51u, 50u, 50u, 51u, 50u, 51u, 50u, 57u, 56u, 54u, 55u, 54u, 53u, 55u, 54u, 53u, 55u, 59u, 61u, 52u, 53u, 52u, 54u, 53u, 52u, 54u, 53u, 60u, 59u, 55u, 56u, 55u, 54u, 56u, 55u, 54u, 56u, 60u, 62u};

static const int8_t HEAVY_PSYCH_TRAX_LEAD_RATE[HEAVY_PSYCH_TRAX_N] = {-5, -3, -1, 1, 3, 5, 7, -7, 13, 0, -5, -3, -1, 1, 3, 5, 6, -6, 12, 0, -5, -3, -1, 1, 3, 5, 4, -4, 10, 2, -5, -3, -1, 1, 3, 5, 2, -2, 8, 4, -5, -3, -1, 1, 3, 5, 0, 8, 6, 14, -8, -6, -4, -2, 0, 2, 4, 6, 10, 12};

static const uint8_t HEAVY_PSYCH_ERA_MACROS[HEAVY_PSYCH_ERA_N][10] = {
    { 88, 62, 72, 56, 52, 68, 92, 48, 0, 36 },
    { 64, 58, 48, 54, 44, 82, 36, 56, 0, 34 },
    { 58, 54, 28, 62, 78, 64, 44, 32, 0, 42 },
    { 52, 48, 16, 50, 40, 56, 28, 72, 0, 28 },
    { 60, 66, 24, 58, 88, 70, 38, 96, 0, 50 },
    { 48, 42, 12, 46, 32, 52, 22, 88, 0, 24 },
};

typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t atk_c, atk_b, glide_c, lead_dec;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} heavy_psych_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} heavy_psych_era_fx_t;

static const heavy_psych_era_base_t HEAVY_PSYCH_ERA_BASE[HEAVY_PSYCH_ERA_N] = {
    /* STONER: chord swell + fuzz bed — not chop-only skank */
    { 34, 88, 96, 32, 56, 64, 98, 28, 100, 26, 94, 68, 48, 64, 78, 28, 62, 20, 0, 0, 0, 8, 96, {64,66,48,78}, 10, 52, 36, 51 },
    /* DESERT: wide slow pad */
    { 32, 86, 102, 24, 52, 60, 100, 24, 22, 28, 88, 64, 46, 64, 72, 24, 58, 32, 0, 32, 24, 28, 104, {64,66,46,76}, 14, 58, 32, 53 },
    /* SPACE: long pad — bass a bit more open than doom/stoner */
    { 28, 84, 108, 28, 58, 70, 108, 32, 24, 22, 80, 62, 48, 64, 66, 26, 56, 92, 0, 56, 36, 40, 116, {64,68,54,82}, 16, 68, 48, 50 },
    /* DRONE: slowest envelopes, deep sub */
    { 26, 82, 112, 16, 44, 54, 114, 36, 16, 32, 84, 62, 44, 64, 76, 26, 54, 40, 0, 72, 48, 48, 120, {64,66,44,74}, 8, 50, 24, 54 },
    /* GAZE: shoegaze wash */
    { 30, 90, 106, 36, 68, 66, 102, 20, 32, 30, 82, 66, 58, 64, 68, 28, 66, 48, 0, 40, 28, 36, 112, {62,68,50,88}, 18, 92, 52, 52 },
    /* DOOM: swell + long tail, lowest LPF */
    { 24, 78, 104, 8, 36, 52, 106, 32, 120, 36, 92, 66, 46, 64, 82, 28, 52, 22, 0, 6, 6, 12, 108, {64,64,40,70}, 6, 44, 20, 55 },
};

/* KICK PUNCH (drm_dens[0]): below 64 = body without beater click; era defaults tuned for rock/doom. */
static const uint8_t HEAVY_PSYCH_KICK_PUNCH[HEAVY_PSYCH_ERA_N] = {
    58, 56, 62, 54, 58, 52
};

static const heavy_psych_era_fx_t HEAVY_PSYCH_ERA_FX[HEAVY_PSYCH_ERA_N] = {
    { DKIT_FUZZ,  22, 0, 46, 32, 74, 38, 36, 72, 56,  72, BASS_FUZZ,  24 },
    { DKIT_DESERT, 36, 1, 42, 34, 84, 44, 40, 76, 62,  68, BASS_OCT,   10 },
    { DKIT_SPACE,  64, 3, 50, 52, 92, 58, 46, 96, 78,  76, BASS_FM,    14 },
    { DKIT_DRONE,  12, 0, 38, 46, 88, 34, 32, 98, 82,  82, BASS_SUB,    4 },
    { DKIT_GAZE,  100, 5, 54, 82, 96, 56, 52, 88, 74, 100, BASS_MOOG,   6 },
    { DKIT_DOOM,   14, 0, 34, 38, 78, 28, 28, 84, 66,  86, BASS_SUB,    6 },
};

static inline uint8_t heavy_psych_clamp_trax(uint8_t ix)
{
    return ix >= HEAVY_PSYCH_TRAX_N ? (uint8_t)(HEAVY_PSYCH_TRAX_N - 1u) : ix;
}

static inline uint8_t heavy_psych_clamp_era(uint8_t era)
{
    return era >= HEAVY_PSYCH_ERA_N ? (uint8_t)(HEAVY_PSYCH_ERA_N - 1u) : era;
}

#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM trax at heavy_psych_gen.slot. */
static fm_pgen_t heavy_psych_gen = { .slot = 0xFFu };
static fm_pgen_lead_t heavy_psych_gen_lead;

#define FM_PGEN_STATE heavy_psych_gen
#define FM_PGEN_BUILD(era, n, c) heavy_psych_gen_build(era, n, c)

static inline uint8_t heavy_psych_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&heavy_psych_gen, ix);
}

static inline uint8_t heavy_psych_trax_root(uint8_t ix)
{
    return heavy_psych_gen_hit(ix) ? (uint8_t)heavy_psych_gen.root : HEAVY_PSYCH_TRAX_ROOT[heavy_psych_clamp_trax(ix)];
}

static inline uint8_t heavy_psych_trax_scale(uint8_t ix)
{
    return heavy_psych_gen_hit(ix) ? (uint8_t)heavy_psych_gen.scale : HEAVY_PSYCH_TRAX_SCALE[heavy_psych_clamp_trax(ix)];
}

static inline uint8_t heavy_psych_trax_swing(uint8_t ix)
{
    return heavy_psych_gen_hit(ix) ? (uint8_t)heavy_psych_gen.swing : HEAVY_PSYCH_TRAX_SWING[heavy_psych_clamp_trax(ix)];
}

static inline int8_t heavy_psych_trax_lead_rate(uint8_t ix)
{
    return heavy_psych_gen_hit(ix) ? heavy_psych_gen.rate : HEAVY_PSYCH_TRAX_LEAD_RATE[heavy_psych_clamp_trax(ix)];
}

enum {
    HP_LEAD_ALGO_SAW = 0,
    HP_LEAD_ALGO_PLUCK,
    HP_LEAD_ALGO_SINE,
};

typedef struct {
    uint8_t algo;
    uint8_t hold_pct;
    uint8_t env_floor_e8;
    uint8_t grit_swell;
    uint16_t swell_start_smpl;
    uint16_t swell_full_smpl;
    uint8_t wah_swell;
    uint8_t rel_decay_sh;
} heavy_psych_lead_sustain_t;

/* Per-era lead sustain + sound-generator algo (one row per era). */
static const heavy_psych_lead_sustain_t HEAVY_PSYCH_LEAD_SUSTAIN[HEAVY_PSYCH_ERA_N] = {
    /* STONER — shortest era, still longer than pre-fix LEN-only ~166 ms */
    { HP_LEAD_ALGO_PLUCK, 62, 5, 0, 4410u, 11025u, 0, 9 },
    /* DESERT */
    { HP_LEAD_ALGO_PLUCK, 78, 7, 1, 4410u, 11025u, 1, 10 },
    /* SPACE */
    { HP_LEAD_ALGO_SAW, 92, 7, 1, 4410u, 11025u, 1, 10 },
    /* DRONE */
    { HP_LEAD_ALGO_SINE, 86, 8, 1, 4410u, 11025u, 0, 10 },
    /* GAZE */
    { HP_LEAD_ALGO_PLUCK, 84, 7, 1, 4410u, 11025u, 0, 10 },
    /* DOOM */
    { HP_LEAD_ALGO_SAW, 90, 7, 1, 4410u, 11025u, 0, 9 },
};

static inline const heavy_psych_lead_sustain_t *heavy_psych_lead_sustain(uint8_t era)
{
    return &HEAVY_PSYCH_LEAD_SUSTAIN[heavy_psych_clamp_era(era)];
}

static inline uint8_t heavy_psych_lead_algo(uint8_t era)
{
    return heavy_psych_lead_sustain(era)->algo;
}

static inline uint8_t heavy_psych_lead_uses_pluck(uint8_t era)
{
    return heavy_psych_lead_algo(era) == (uint8_t)HP_LEAD_ALGO_PLUCK;
}

static inline int32_t heavy_psych_lead_sustain_mod_q8(uint16_t note_age, uint8_t era)
{
    const heavy_psych_lead_sustain_t *su = heavy_psych_lead_sustain(era);
    uint32_t n = note_age;
    uint16_t t0 = su->swell_start_smpl;
    uint16_t t1 = su->swell_full_smpl;

    if (su->grit_swell == 0u && su->wah_swell == 0u)
        return 0;
    if (n <= (uint32_t)t0)
        return 64;
    if (n >= (uint32_t)t1)
        return 256;
    return (int32_t)(64u + ((n - t0) * 192u) / (uint32_t)(t1 - t0));
}

static inline const char *heavy_psych_era_name(uint8_t era)
{
    return HEAVY_PSYCH_ERA_NAMES[heavy_psych_clamp_era(era)];
}

static inline const char *heavy_psych_trax_name(uint8_t trax)
{
    if (heavy_psych_gen_hit(trax))
        return heavy_psych_gen.name;
    return HEAVY_PSYCH_TRAX[heavy_psych_clamp_trax(trax)].name;
}

static inline const heavy_psych_trax_t *heavy_psych_trax(uint8_t ix)
{
    if (heavy_psych_gen_hit(ix))
        return &heavy_psych_gen.pl;
    return &HEAVY_PSYCH_TRAX[heavy_psych_clamp_trax(ix)];
}

static inline uint8_t heavy_psych_era_of_trax(uint8_t trax)
{
    trax = heavy_psych_clamp_trax(trax);
    return (uint8_t)(trax / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t heavy_psych_era_first_trax(uint8_t era)
{
    return (uint8_t)(heavy_psych_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static const uint8_t HEAVY_PSYCH_TRAX_KIT[HEAVY_PSYCH_TRAX_N] = {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 12u, 13u, 2u, 3u, 4u, 5u, 6u, 7u, 0u, 1u, 6u, 7u, 4u, 5u, 6u, 7u, 0u, 1u, 2u, 3u, 8u, 9u, 6u, 7u, 0u, 1u, 2u, 3u, 4u, 5u, 10u, 11u, 6u, 7u, 0u, 1u, 2u, 3u, 4u, 5u, 10u, 11u, 1u, 0u, 1u, 0u, 1u, 0u, 1u, 0u, 7u, 6u};

static inline uint8_t heavy_psych_trax_kit(uint8_t ix)
{
    return heavy_psych_gen_hit(ix) ? (uint8_t)heavy_psych_gen.kit : HEAVY_PSYCH_TRAX_KIT[heavy_psych_clamp_trax(ix)];
}

static inline void heavy_psych_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 32;
    p->cutoff_c = 88;
    p->decay_c = 86;
    p->cutoff_b = 68;
    p->decay_b = 94;
    p->sub_oct = 24;
    p->drive = 18;
    p->pump = 88;
    p->feedback = 16;
    p->density = 64u;
    p->wobble = 2;
    p->kick_boom = 88;
    p->rim_tone = 66;
    p->hat_lvl = 52;
    p->siren_pitch = 64;
    p->siren_rate = 23;
    p->drum_kit = DKIT_FUZZ;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 20u;
        p->drm_dly[di] = di == 3u ? 28u : 0u;
        p->drm_rev[di] = di == 0u ? 0u : (di == 1u ? 22u : (di == 2u ? 10u : 96u));
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 64u;
    }
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
}

/* Pad hold grid (32 × 16th = 2 bars): one strike per 1–2 bars, env legato carries the rest. */
static inline uint32_t heavy_psych_era_chord_mask(uint8_t era)
{
    era = heavy_psych_clamp_era(era);
    if (era == ERA_GAZE || era == ERA_SPACE || era == ERA_DRONE)
        return 1u;
    return 1u | ((uint32_t)1u << 16);
}

static inline uint8_t heavy_psych_chord_step_on(uint8_t era, uint32_t plate_mask, uint8_t step)
{
    uint32_t hold_m = heavy_psych_era_chord_mask(era);

    (void)plate_mask;
    return dub_step_bit(hold_m, step);
}

static inline void dub_drm_set_bus_dly(dub_params_t *p, uint8_t bus)
{
    uint8_t i;

    if (!p)
        return;
    /* Heavy Psych drums (K, R, H) stay dry of dub delay; only lead (lane 3) uses delay send */
    for (i = 0; i < 3u; i++)
        p->drm_dly[i] = 0u;
    p->drm_dly[3] = bus;
}

static inline void dub_drm_set_bus_rev(dub_params_t *p, uint8_t bus)
{
    (void)bus;
    if (!p)
        return;
    /* Kick dry, snare/rim and hat short room send only; lead full reverb */
    p->drm_rev[0] = 0u;
    p->drm_rev[1] = 22u;
    p->drm_rev[2] = 10u;
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

static inline void heavy_psych_era_apply_base(dub_params_t *p, uint8_t era)
{
    const heavy_psych_era_base_t *eb;
    const heavy_psych_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = heavy_psych_clamp_era(era);
    eb = &HEAVY_PSYCH_ERA_BASE[era];
    fx = &HEAVY_PSYCH_ERA_FX[era];
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
    p->atk_c = eb->atk_c;
    p->atk_b = eb->atk_b;
    p->glide_c = eb->glide_c;
    p->drm_dec[3] = eb->lead_dec ? eb->lead_dec : 64u;
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
    if (era == ERA_DOOM || era == ERA_STONER)
        p->lvl_b = 116;
    else if (era == ERA_DESERT || era == ERA_DRONE)
        p->lvl_b = 112;
    else if (era == ERA_SPACE)
        p->lvl_b = 108;
    else
        p->lvl_b = 110;
    if (era == ERA_DOOM || era == ERA_STONER)
        p->pan[0] = 58;
    else if (era == ERA_SPACE || era == ERA_GAZE)
        p->pan[0] = 70;
    else
        p->pan[0] = 64;
    if (era == ERA_GAZE || era == ERA_SPACE || era == ERA_DRONE)
        p->lvl_c = 108;
    else if (era == ERA_DESERT)
        p->lvl_c = 100;
    else
        p->lvl_c = 96;
    p->rev_b = (uint8_t)(24u + (fx->rev_mix >> 1));
    if (era == ERA_GAZE)
        p->rev_c = (uint8_t)(72u + (fx->rev_mix >> 2));
    else if (era == ERA_SPACE || era == ERA_DRONE)
        p->rev_c = (uint8_t)(64u + (fx->rev_mix >> 2));
    else if (era == ERA_DESERT)
        p->rev_c = (uint8_t)(52u + (fx->rev_mix >> 2));
    else
        p->rev_c = (uint8_t)(40u + (fx->rev_mix >> 2));
    p->dly_c = (uint8_t)(32u + (fx->dly_lvl >> 1));
    p->drm_dens[0] = HEAVY_PSYCH_KICK_PUNCH[era];
    p->density = p->drm_dens[0];
}

/* Per-preset KICK/RIM/HAT decay trims (drm_dec[0..2]), fitted so every factory voice keeps
 * its envelope length from before the quarter-octave drum decay (±15 %). Siren keeps kit_decay.
 * Column 3: open-hat length trim, 64 + hat_mul steps; read by the DSP via p->plate, never saved. */
static const uint8_t heavy_psych_drm_dec_fit[GENRE_PRESET_N][4] = {
    { 27,  41,  67, 64}, { 28,  41,  63, 65}, { 26,  40,  63, 66}, { 27,  41,  67, 64},
    { 29,  43,  67, 64}, { 27,  43,  64, 64}, { 29,  41,  71, 65}, { 26,  42,  67, 64},
    { 29,  42,  67, 64}, { 29,  43,  64, 64}, { 22,  44,  67, 65}, { 20,  47,  67, 64},
    { 23,  47,  71, 64}, { 21,  47,  72, 63}, { 22,  46,  67, 63}, { 23,  46,  67, 64},
    { 23,  46,  67, 64}, { 21,  45,  67, 64}, { 22,  47,  72, 62}, { 22,  46,  67, 64},
    { 29,  48,  71, 64}, { 30,  48,  72, 63}, { 29,  49,  75, 64}, { 30,  47,  39, 64},
    { 31,  47,  43, 66}, { 30,  46,  67, 64}, { 29,  48,  63, 66}, { 31,  46,  35, 64},
    { 29,  46,  43, 66}, { 28,  49,  63, 65}, { 25,  48,  72, 62}, { 26,  48,  68, 64},
    { 26,  49,  72, 63}, { 27,  49,  65, 64}, { 26,  47,  63, 66}, { 27,  49,  72, 63},
    { 24,  47,  67, 64}, { 24,  48,  67, 64}, { 27,  47,  63, 66}, { 24,  48,  67, 64},
    { 28,  45,  96, 63}, { 28,  44,  68, 64}, { 29,  45,  68, 63}, { 26,  43,  96, 61},
    { 28,  45,  67, 65}, { 29,  45,  67, 64}, { 26,  44,  94, 64}, { 29,  42,  68, 63},
    { 29,  45,  92, 63}, { 29,  43,  72, 63}, { 28,  43,  63, 65}, { 28,  43,  68, 63},
    { 30,  43,  63, 65}, { 30,  45,  67, 64}, { 31,  43,  59, 65}, { 28,  45,  67, 64},
    { 29,  44,  64, 64}, { 30,  45,  67, 64}, { 28,  43,  68, 64}, { 31,  44,  68, 63}
};
#define GENRE_DRM_DEC_FIT heavy_psych_drm_dec_fit

static inline void heavy_psych_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = heavy_psych_drm_dec_fit[preset][i];
}

static inline void heavy_psych_trax_apply(dub_params_t *p, uint8_t trax, uint8_t what)
{
    const heavy_psych_trax_t *tx;
    uint8_t di;

    if (!p)
        return;
    trax = heavy_psych_clamp_trax(trax);
    tx = heavy_psych_trax(trax);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        heavy_psych_apply_defaults(p);
        heavy_psych_era_apply_base(p, tx->era);
        heavy_psych_drm_dec_fit_apply(p, trax);
    }
    p->era = tx->era;
    p->plate = trax;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = tx->bpm;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = 64u;
        p->drm_dens[0] = HEAVY_PSYCH_KICK_PUNCH[heavy_psych_clamp_era(tx->era)];
        p->density = p->drm_dens[0];
        p->drum_kit = heavy_psych_trax_kit(trax);
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
        p->root = heavy_psych_trax_root(trax);
        p->scale = heavy_psych_trax_scale(trax);
        p->swing = heavy_psych_trax_swing(trax);
        {
            int16_t stab = (int16_t)p->siren_rate + (int16_t)heavy_psych_trax_lead_rate(trax);

            if (stab < 0)
                stab = 0;
            else if (stab > 127)
                stab = 127;
            p->siren_rate = (uint8_t)stab;
        }
    }
}

static inline void heavy_psych_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t trax)
{
    if (!p)
        return;
    era = heavy_psych_clamp_era(era);
    trax = heavy_psych_clamp_trax(trax);
    heavy_psych_trax_apply(p, trax, DUB_APPLY_ALL);
    p->era = era;
    p->plate = trax;
    p->drum_kit = heavy_psych_trax_kit(trax);
    p->bass_model = heavy_psych_trax(trax)->bass_model;
    p->voicing = heavy_psych_trax(trax)->voicing;
    p->bpm = heavy_psych_trax(trax)->bpm;
    p->pump = heavy_psych_trax(trax)->pump;
    p->feedback = heavy_psych_trax(trax)->feedback;
    p->wobble = heavy_psych_trax(trax)->wobble;
    p->drum_pat = trax;
    p->root = heavy_psych_trax_root(trax);
    p->scale = heavy_psych_trax_scale(trax);
    p->swing = heavy_psych_trax_swing(trax);
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
    const heavy_psych_trax_t *tx;

    plate = heavy_psych_clamp_trax(plate);
    tx = heavy_psych_trax(plate);
    if (root)
        *root = heavy_psych_trax_root(plate);
    if (scale)
        *scale = heavy_psych_trax_scale(plate);
    if (swing)
        *swing = heavy_psych_trax_swing(plate);
    if (bpm)
        *bpm = tx->bpm;
}

static const uint8_t HEAVY_PSYCH_BASS_WALK[HEAVY_PSYCH_ERA_N][GENRE_PRESETS_PER_ERA][8] = {
    { { 0, 0, 3, 0, 5, 3, 0, 5 }, { 0, 3, 0, 5, 0, 3, 5, 0 }, { 0, 5, 3, 0, 3, 0, 5, 3 }, { 0, 0, 5, 3, 0, 5, 3, 0 }, { 0, 3, 5, 0, 5, 0, 3, 5 }, { 5, 0, 3, 0, 0, 5, 0, 3 }, { 0, 0, 3, 5, 0, 3, 0, 5 }, { 0, 5, 0, 3, 5, 3, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 3, 0, 5, 0, 3, 5, 0 }, { 0, 5, 3, 0, 5, 0, 3, 0 }, { 3, 0, 0, 5, 0, 3, 0, 5 }, { 0, 0, 3, 5, 3, 0, 5, 0 }, { 5, 0, 3, 0, 0, 5, 3, 0 }, { 0, 3, 5, 0, 3, 5, 0, 0 }, { 0, 5, 0, 3, 0, 0, 5, 3 }, { 3, 0, 5, 0, 0, 3, 0, 5 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 5, 0, 3, 0, 6, 0, 5 }, { 0, 0, 5, 0, 3, 0, 6, 0 }, { 5, 0, 0, 6, 0, 3, 0, 5 }, { 0, 6, 0, 0, 5, 0, 3, 0 }, { 0, 3, 0, 5, 6, 0, 0, 5 }, { 6, 0, 3, 0, 0, 5, 0, 3 }, { 0, 0, 3, 6, 5, 0, 0, 3 }, { 0, 5, 6, 0, 0, 3, 5, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 2, 3, 4, 2, 5, 4, 0 }, { 0, 3, 2, 4, 3, 2, 5, 0 }, { 2, 0, 3, 2, 4, 3, 0, 2 }, { 0, 2, 4, 3, 2, 0, 5, 3 }, { 3, 2, 0, 4, 2, 5, 3, 0 }, { 0, 4, 2, 3, 0, 2, 4, 5 }, { 2, 3, 0, 2, 5, 4, 0, 3 }, { 0, 2, 3, 0, 4, 2, 3, 5 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 3, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 5, 0, 0 }, { 0, 0, 3, 0, 0, 0, 0, 5 }, { 0, 0, 0, 0, 3, 0, 0, 0 }, { 0, 3, 0, 0, 0, 0, 5, 0 }, { 0, 0, 0, 5, 0, 0, 3, 0 }, { 0, 0, 0, 0, 0, 3, 0, 5 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } }

};

static inline uint8_t heavy_psych_bass_walk_deg(uint8_t era, uint8_t piece, uint8_t step)
{
    era = heavy_psych_clamp_era(era);
    piece = (uint8_t)(heavy_psych_clamp_trax(piece) % GENRE_PRESETS_PER_ERA);
    return HEAVY_PSYCH_BASS_WALK[era][piece][step & 7u];
}

/* Lead melody degrees: 0 root, 1 m3, 2 P4, 3 P5, 4 b7, 5 b5, 6 octave (avoid 6 in patterns). */
static const int8_t HEAVY_PSYCH_LEAD_INTERVAL[7] = { 0, 3, 5, 7, 10, 6, 12 };

static const uint8_t HEAVY_PSYCH_LEAD_MELODY[HEAVY_PSYCH_ERA_N][8] = {
    { 0, 1, 3, 4, 5, 3, 1, 4 },
    { 0, 1, 2, 3, 4, 2, 3, 1 },
    { 0, 3, 4, 5, 1, 4, 3, 0 },
    { 0, 1, 4, 3, 5, 4, 1, 3 },
    { 0, 1, 2, 4, 3, 1, 4, 2 },
    { 0, 3, 1, 4, 5, 3, 4, 1 },
};

static inline uint8_t heavy_psych_lead_deg(uint8_t era, uint8_t step)
{
    uint8_t idx = (uint8_t)(((step >> 2) + (step & 3u) + ((step >> 4) << 1)) & 7u);

    era = heavy_psych_clamp_era(era);
    return HEAVY_PSYCH_LEAD_MELODY[era][idx];
}

static inline uint8_t heavy_psych_lead_note(uint8_t root, uint8_t era, uint8_t step)
{
    uint8_t deg = heavy_psych_lead_deg(era, step);
    int32_t n;

    if (deg > 6u)
        deg = 0u;
    n = (int32_t)root + (int32_t)HEAVY_PSYCH_LEAD_INTERVAL[deg];
    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    return (uint8_t)n;
}

static inline int8_t heavy_psych_lead_tune_from_note(uint8_t bass_root, uint8_t lead_midi)
{
    int32_t ref = (int32_t)bass_root;
    int32_t t = (int32_t)lead_midi;

    while (t < ref)
        t += 12;
    if (t - ref > 72)
        return 72;
    return (int8_t)(t - ref);
}

typedef struct {
    uint8_t grit_start, grit_end, bend_cents, gate_cs, lp_cut, sweep_rate_q6, grit_curve;
} heavy_psych_lead_voice_t;

static const uint32_t HEAVY_PSYCH_TRAX_LEAD_GATE[HEAVY_PSYCH_TRAX_N] = {0x78484C97u, 0x72105497u, 0x7888494Du, 0x7008494Du, 0x98249975u, 0xA048455Fu, 0x70105136u, 0xF8208934u, 0x70105136u, 0xF8208934u, 0x3C104C94u, 0x78884C5Cu, 0x2B1049D3u, 0xBB249365u, 0x8044454Fu, 0xC8084975u, 0x79108910u, 0x714851B2u, 0x79108910u, 0x714851B2u, 0x4888484Du, 0x10105455u, 0xA224E274u, 0x8044445Du, 0x48084972u, 0x5010D550u, 0x48484CE5u, 0x42104DD7u, 0x48484CE5u, 0x42104DD7u, 0x9008C809u, 0x92249225u, 0x8048C84Bu, 0xD010D023u, 0xC8208821u, 0xC848C895u, 0xA410C895u, 0xC888C849u, 0xA410C895u, 0xC888C849u, 0x96349634u, 0x0474445Eu, 0x48784965u, 0x4C708951u, 0x48784997u, 0x227049D7u, 0x4CB84958u, 0x5438491Cu, 0x4CB84958u, 0x5438491Cu, 0x3144444Bu, 0xF9088823u, 0x3144444Bu, 0x794848A3u, 0xA5105095u, 0xE9884849u, 0x71084808u, 0xF3249224u, 0x71084808u, 0xF3249224u};

static const int8_t HEAVY_PSYCH_TRAX_LEAD_PITCH[HEAVY_PSYCH_TRAX_N][16] = {
    {    0,   -3,    4,   -5,   -3,   -2,   -5,   -3,    0,    4,   -3,   -5,   -5,   -5,   -5,    4 },
    {    4,    0,   -1,    4,   -5,    3,    4,    0,    4,   -5,   -1,    4,   -5,   -5,   -5,   -2 },
    {   -3,   -2,    4,   -5,    0,    4,   -3,   -5,   -3,   -2,    4,   -5,    0,   -5,   -5,    3 },
    {   -3,    3,    4,   -5,    4,    4,   -3,    4,   -3,    3,    4,   -3,   -5,   -5,   -3,    4 },
    {   -1,    4,   -5,    3,    4,   -5,    4,    4,    0,    4,    4,   -5,   -5,   -2,    4,   -5 },
    {   -1,    0,   -3,   -5,    4,    3,   -5,   -2,   -5,    4,   -5,    4,    4,    3,   -5,   -5 },
    {    4,    0,    4,    4,   -5,   -5,    4,    3,   -5,   -2,    4,    4,   -5,   -5,   -5,    3 },
    {    4,    3,    4,   -2,   -3,   -5,    0,    4,   -5,    3,    4,   -5,   -5,   -5,   -5,   -5 },
    {   -3,   -3,    4,    4,   -5,   -3,   -5,    3,   -5,   -2,    4,    4,   -5,    4,   -5,   -5 },
    {    0,    3,    4,   -2,    4,   -5,   -5,    4,   -5,    3,    4,   -5,   -5,    4,   -5,   -5 },
    {    2,    4,   -3,    3,   -5,    0,    4,   -3,    2,    4,   -5,   -5,   -5,   -5,   -3,    4 },
    {    4,    4,    2,   -5,   -5,    3,   -5,   -5,    4,    4,   -5,   -5,   -5,   -5,   -5,    0 },
    {    4,   -3,    4,    4,   -5,    4,   -3,   -3,   -5,   -5,    4,   -5,    2,   -5,   -5,    3 },
    {   -3,    3,   -5,    0,    4,   -5,   -3,    4,   -5,   -5,    4,   -5,   -5,    4,    2,   -5 },
    {   -5,   -5,   -5,   -5,    4,    0,   -5,    4,   -5,    4,   -5,    3,    4,    0,   -5,   -5 },
    {    0,    4,    0,   -3,    4,    0,    0,    0,    0,    4,    2,   -5,    4,    3,   -5,   -5 },
    {    4,    0,    4,    4,   -3,    4,    4,    3,   -3,    0,    4,   -5,   -5,   -5,   -5,   -3 },
    {    4,    0,    4,   -3,    0,   -1,   -3,    0,   -3,    3,    4,    0,   -3,   -3,   -3,    4 },
    {    0,    0,    4,    4,   -3,    4,    0,    3,   -3,    0,    4,   -3,    2,    4,   -3,   -1 },
    {    0,    0,    4,   -3,    4,   -1,   -3,    0,   -5,    3,    4,    0,   -5,    4,   -5,   -5 },
    {   -1,    3,   -1,   -5,    4,    4,   -3,   -5,    4,    3,    0,   -5,    4,    4,   -5,   -5 },
    {    0,   -5,   -3,    3,   -5,    4,   -3,    4,    4,   -5,    4,    3,   -5,    4,   -3,    4 },
    {    4,    4,   -5,   -5,   -5,   -5,    0,    4,    4,   -5,    4,   -5,    4,   -5,   -5,   -5 },
    {    0,    4,   -5,    0,   -5,   -5,   -5,    3,    0,    4,   -5,    4,    4,   -5,   -5,   -5 },
    {    4,    0,    0,   -5,   -1,    0,    0,   -5,   -1,    3,    0,   -5,    4,    4,   -5,   -5 },
    {    4,   -5,    4,    3,   -5,    4,   -5,    4,   -5,   -5,    4,    3,   -5,    4,   -5,   -5 },
    {   -3,    4,   -3,   -5,    4,   -3,   -5,   -3,    4,    4,    4,   -5,    4,    3,   -5,    4 },
    {   -5,   -5,   -5,    4,   -5,   -5,   -5,   -5,    0,   -5,   -5,   -5,    4,   -5,   -5,    3 },
    {    4,    4,    0,   -5,    4,   -3,   -5,   -3,   -5,    4,   -5,   -5,    4,    3,   -5,    4 },
    {    0,   -5,    4,    4,   -5,   -5,    4,   -5,   -5,   -5,   -5,   -5,    4,   -5,   -5,    3 },
    {   -3,    4,    4,   -5,   -5,    4,    4,    0,    4,    4,    4,   -3,   -5,    4,   -3,   -5 },
    {   -5,    0,   -5,    4,    4,   -5,   -5,    4,    4,   -5,    4,    4,   -5,    1,   -5,   -5 },
    {   -3,   -3,    4,   -5,    4,    4,   -5,    1,   -5,    4,    4,   -3,    4,    4,   -3,   -5 },
    {   -1,    0,   -5,    4,   -5,    0,    4,    4,    4,    1,   -5,    4,   -5,    0,   -5,   -5 },
    {   -3,    4,    4,    1,   -5,   -5,    4,    0,    4,    4,    4,   -5,   -5,    4,   -5,   -5 },
    {   -3,    0,   -3,   -5,   -5,    1,   -5,   -5,    4,    0,    4,   -5,    4,    1,   -5,   -5 },
    {   -1,    4,    0,    0,   -5,    4,    4,   -5,   -5,    4,   -5,   -5,    4,   -5,   -5,   -5 },
    {   -1,    1,   -5,   -5,    4,    0,   -5,   -5,    4,    1,   -5,   -5,    4,    0,   -5,   -5 },
    {   -1,    4,    0,    0,   -5,    4,    4,   -5,   -5,    4,   -5,   -5,    4,   -5,   -5,   -5 },
    {   -3,    1,   -5,   -5,    4,    0,   -5,   -5,    4,    1,   -5,   -5,    4,    0,   -5,   -5 },
    {    2,    4,   -5,    4,   -5,   -5,    4,    3,    2,   -5,   -5,    4,   -5,    4,    4,   -5 },
    {    4,    0,    0,    0,   -3,   -5,   -5,    4,    4,    3,   -5,    4,    4,    4,    0,    4 },
    {    0,    4,    0,   -3,   -5,   -5,   -3,    4,   -3,    4,    4,   -5,    2,    4,   -5,    4 },
    {   -3,    4,    0,    4,   -5,   -5,   -3,    4,   -5,    4,    0,   -5,    4,    3,   -5,   -5 },
    {   -5,   -5,   -5,   -5,   -5,   -5,   -5,   -5,   -5,    4,    4,   -5,    0,    4,   -5,    3 },
    {   -1,    0,   -3,    4,   -5,   -5,   -5,   -3,   -3,   -5,    2,   -3,    4,   -5,   -3,    4 },
    {    0,    4,    4,   -5,   -5,    4,   -3,   -5,   -5,    4,    4,   -5,    2,    4,   -5,    4 },
    {    4,    4,    0,   -3,   -5,   -5,    2,    4,    0,    4,    0,   -3,   -5,    3,   -5,    4 },
    {   -3,    4,   -3,   -5,    2,    4,   -3,   -5,   -3,    4,    4,   -5,    2,    4,   -5,    4 },
    {   -5,    4,   -5,   -5,    4,   -5,   -5,    4,   -5,    4,    0,   -5,   -5,    3,   -5,    4 },
    {    4,   -3,   -3,   -3,    3,    4,   -5,    4,   -5,    0,   -3,    4,   -5,   -5,   -3,    4 },
    {    0,    0,    4,    0,    4,    0,    3,    4,   -1,    4,    4,   -3,   -3,   -3,    3,   -3 },
    {   -5,   -5,   -5,   -5,    4,    0,   -5,    4,   -5,    4,   -5,    4,    4,   -5,   -5,    4 },
    {   -3,   -3,    3,   -5,   -2,   -3,   -5,   -5,   -5,    4,    3,   -5,   -5,   -5,   -5,    0 },
    {   -5,    0,   -5,    4,   -5,    4,   -2,   -5,    4,    0,   -5,    4,   -5,   -5,   -5,   -5 },
    {   -5,    4,    4,   -5,    4,    4,   -5,   -5,   -5,    4,    4,   -5,    4,   -5,   -5,   -5 },
    {    3,    4,   -2,   -5,    4,    0,    4,    4,   -5,    4,   -2,   -5,   -5,   -5,   -5,    4 },
    {    4,    4,   -5,    4,   -2,   -5,    4,    0,   -5,   -5,    3,    4,   -5,   -5,   -5,   -5 },
    {   -3,    4,   -2,   -5,    4,    0,    4,    4,   -5,    4,   -2,   -5,   -5,    0,   -5,   -5 },
    {   -1,    4,   -5,    4,   -2,   -5,    4,    0,   -5,   -5,    3,    4,   -5,    4,    4,   -5 },
};

/* Per-step lead length (HP_LEAD_LEN ix) and bend depth (0..12 semitones); legato via LEN overlap. */
static const uint8_t HEAVY_PSYCH_TRAX_LEAD_LEN[HEAVY_PSYCH_TRAX_N][32] = {
    {   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,   7,   9,   0,   0,   5,   0,   0,   0,   0,   9,   0,   0,   1,   0,   0,   0,   0,   7,  12,  13,  11,   0 },
    {   0,   7,   7,   0,   9,   0,   0,   9,   0,   0,   7,   0,   8,   0,   7,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   9,   0,   0,  11,  10,   0,   0 },
    {   9,   0,   0,   9,   0,   0,   8,   0,   9,   0,   0,   9,   0,   0,   8,   0,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   8,   0,  13,  12,   0 },
    {   9,   0,   0,   9,   0,   0,   8,   0,   9,   0,   0,   9,   0,   0,   3,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,  11,  11,   0,   0 },
    {   7,   0,   9,   0,   0,   7,   0,   0,   4,   0,   0,   7,   9,   0,   0,   9,   0,   0,   9,   0,   0,   2,   0,   0,   0,   0,   0,   7,  11,   0,   0,  13 },
    {   7,   9,   7,   9,   0,   0,   8,   0,   9,   0,  10,   0,   0,   0,   2,   0,   0,   0,   0,   9,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,  12 },
    {   0,   9,   0,   0,   7,   9,   0,   0,  10,   0,   0,   0,   9,   0,   2,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,  11,  11,  11,   0 },
    {   0,   0,   0,   0,   9,   9,   0,   0,  10,   0,   0,  10,   0,   0,   0,   4,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   7,  12,  12,  10,   0 },
    {   7,  10,   0,   0,   0,   7,   8,   0,  10,   0,   0,   0,   8,   0,   2,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,  13,   0,  13,  13 },
    {  10,   0,   0,   0,   0,   7,   8,   0,   9,   0,   0,  10,   0,   0,   0,   6,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   7,  11,   0,  11,  10 },
    {   0,   0,   9,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,   3,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   7,   7,  12,  13,   0,   0 },
    {   0,   0,   0,   7,   8,   0,  10,   0,   0,   0,   7,   9,   0,   0,   2,   0,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   7,  12,  12,  13,   0 },
    {   0,   9,   0,   0,   8,   0,   7,   7,   9,   0,   0,   9,   0,   0,   1,   0,   0,   0,   0,   0,  10,   0,   0,   0,   7,   8,   0,   8,   0,  11,   0,   0 },
    {   8,   0,  10,   0,   0,   7,   8,   0,   7,   9,   0,   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,   7,   8,   0,   7,  10,   0,   0,  12 },
    {   7,   7,   7,   9,   0,   0,   8,   0,  10,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,  12 },
    {  10,   0,   9,   0,   0,   7,   8,   0,   6,   0,   0,   9,   0,   0,   2,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,  12,  10 },
    {   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,  10,   0,   0,   0,   8,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   7,  13,  13,  11,   0 },
    {   0,   9,   0,   0,   9,   8,   0,   6,  10,   0,   0,   0,   8,   0,   7,   0,   0,   0,   0,   9,   0,   0,   8,   0,  10,   0,   0,   0,  10,  10,  11,   0 },
    {   9,   0,   0,   0,   8,   0,   7,   0,   9,   0,   0,  10,   0,   0,   0,   8,   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   9,   0,   0,  12,   0 },
    {   7,  10,   0,   0,   0,   9,   6,   7,  10,   0,   0,   0,   8,   0,   3,   0,   0,   0,   0,   9,   0,   0,   8,   0,  10,   0,   0,   0,  10,   0,  11,  11 },
    {  11,   0,   6,  11,   0,   0,   5,   0,   0,   0,   0,   9,   0,   0,   9,   0,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,  10,   0 },
    {   7,   0,   8,   0,   9,   0,   9,   0,   0,   0,   0,   0,   8,   0,   4,   0,   0,   0,   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,  11,   0,   0,   0 },
    {   0,   0,   8,   0,   7,  10,   0,   0,   0,  10,   0,   0,   0,   7,   7,   9,   0,   0,  10,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  11,   0,  13 },
    {   0,   0,  11,   4,   8,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,  11 },
    {   0,   9,   0,   0,   7,   7,   8,   0,   9,   0,   0,   9,   0,   0,   1,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,  11,   0 },
    {   0,   0,   0,   0,  11,   0,   8,   0,  11,   0,   0,   0,  10,   0,   7,   4,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,  13,   0,  13,   0 },
    {   8,   0,   9,   0,   0,   7,   9,  10,   0,   0,   0,   9,   0,   0,   1,   0,   0,   0,   0,   9,   0,   0,   7,   0,   0,   0,   0,  10,   0,   0,  11,   0 },
    {   7,   7,   8,   0,   8,   0,   7,  10,   0,   0,   7,  10,   0,   0,   8,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   1,   0,   0,   0,   0,  11,   0 },
    {   0,   0,   9,   0,   0,   8,   0,   7,  10,   0,   7,   9,   0,   0,   2,   0,   0,   0,   0,   9,   0,   0,   8,   0,   0,   0,   0,   9,   0,   0,  11,   0 },
    {   0,   9,   0,   0,   9,   0,   0,   7,   8,   0,   7,   9,   0,   0,   2,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   5,   0,   0,   0,   0,  12,   0 },
    {   9,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,  11,  10,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,  13,   0,   0,  11 },
    {   8,   0,   9,   0,   0,  10,   0,   0,   0,  10,   0,   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,  13,   0,   0,  13 },
    {   7,  11,   0,  11,   0,   0,  11,   0,   0,   0,   0,   9,   0,   0,   7,  10,   0,   0,   0,   9,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,  10 },
    {   7,  10,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,  12,   0,   7,   2,   0,   0,   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,  12,   0,  12,  13 },
    {   6,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,  10,   0,   0,   0,  11,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   9,   0,   0,  13,  11 },
    {  12,   0,  11,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,   7,  10,   0,   0,   0,   9,   0,   0,   5,   0,   0,   0,   0,   9,   0,   0,  13,  11 },
    {   8,   0,   6,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,   7,  11,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   9,   0,   0,  11,   0,  12 },
    {  11,   0,   0,   9,   0,   0,   2,   0,   0,   0,   0,   9,   0,   0,   7,  11,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,  12,  11 },
    {  12,   0,   5,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,   7,   8,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   9,   0,   0,  13,   0,  12 },
    {   9,   0,   0,  10,   0,   0,   8,   0,   0,   0,   0,  10,   0,   0,   7,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,  11,  12 },
    {   0,   0,   8,   0,   7,  10,   0,   0,   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,   8,   0,   7,  10,   0,   0,   0,   7,   9,   0,  10,   0,   0,  10 },
    {   0,   8,   0,   7,   8,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   7,   0,  10,   7,  10,   0,   0,   0,   5,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   0,   0,   7,   8,   0,   9,   0,   0,   9,   0,   0,   4,   0,   0,   0,   0,   7,   7,   4,   0,   0,   0,   0,   0,   9,   0,   0,  12,   0 },
    {  10,   0,   0,   0,   8,   0,   9,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   9,   0,   0,  13,   0 },
    {   7,   7,   8,   0,   9,   0,   0,   7,   9,   0,   0,   9,   0,   0,   4,   0,   0,   0,   0,   8,   0,   7,   6,   0,   0,   0,   0,   9,   0,   0,  11,   0 },
    {   7,   6,   8,   0,   9,   0,   0,   7,   9,   0,   0,   9,   0,   0,   9,   0,   0,   0,   0,   0,   7,   7,   9,   0,   0,  10,   0,   0,   0,  13,   0,   0 },
    {   0,   0,   0,   7,   8,   0,   8,   0,   9,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   7,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,  13,   0 },
    {   0,   0,   0,   7,   1,   0,   0,   0,   9,   0,   0,   9,   0,   0,   1,   0,   0,   0,   0,   7,   7,   3,   0,   0,   0,   0,   0,   0,  11,   0,  10,   0 },
    {   9,   0,   7,   9,   0,   0,   8,   0,   9,   0,   0,   9,   0,   0,   6,   0,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,  11,   0 },
    {   8,   0,   7,   9,   0,   0,   8,   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,   0,   0,   8,   0,   5,   0,   0,   0,   0,   0,   0,  13,   0,   0,   0 },
    {   0,  11,   0,   9,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   8,   0,  10,   0,   0,   0,  13,  12,   0,   0 },
    {   7,  10,   0,   0,   0,   6,   0,   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,   2,   0,   0,   0,   0,   9,   0,   0,   7,  13,  12,   0,  12 },
    {   7,   8,   0,   9,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   8,   0,  12,   0,   0,   0,   0,  12,   0,   0 },
    {   7,  10,   0,   0,   0,   8,   0,  10,   0,   0,   0,   9,   0,   0,   6,   0,   0,   0,   0,   9,   0,   0,  10,   0,   9,   0,   0,   7,  13,  10,  10,   0 },
    {   8,   0,   8,   0,  12,   0,   0,   9,   0,   0,   0,   0,   9,   0,  10,   0,   0,   0,   0,   0,  11,   0,   0,   0,   0,   0,  11,   0,   0,  13,   0,  11 },
    {  13,   0,   0,   9,   0,   0,   9,   0,   0,   0,   0,   9,   0,   0,  11,   0,   0,   0,   0,  10,   0,   0,   0,   7,   9,   0,   0,   9,   0,  13,  11,  13 },
    {   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   9,   0,   0,   0,   0,   9,   0,   0,   0,   0,  10,   0,   0,   0,  11,  12,   0,   0 },
    {   0,   0,   9,   0,   0,  10,   0,   0,   0,   9,   0,   0,  12,   0,   0,   9,   0,   0,   9,   0,   0,   9,   0,   0,   7,   9,   0,   0,  13,  13,  13,  12 },
    {  10,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   6,   0,   0,   0,   0,   9,   0,   0,   0,   0,  11,   0,   0,   0,  11,   0,   0,  12 },
    {   7,   0,  11,   0,   0,  10,   0,   0,   0,   9,   0,   0,   9,   0,   0,  11,   0,   0,   9,   0,   0,   9,   0,   0,   7,   9,   0,   0,  12,   0,   0,  13 },
};

static const uint8_t HEAVY_PSYCH_TRAX_LEAD_BEND[HEAVY_PSYCH_TRAX_N][32] = {
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   4,   0,   5,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   0,   5,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0 },
    {   3,   0,   0,   0,   3,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0 },
    {   0,   0,   0,   0,   0,   0,   3,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5 },
    {   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0 },
    {   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4 },
};

static const heavy_psych_lead_voice_t HEAVY_PSYCH_TRAX_LEAD_VOICE[HEAVY_PSYCH_TRAX_N] = {
    { 12u,  74u, 34u, 20u,  63u,  48u, 0u },
    { 27u,  86u, 40u, 23u,  81u,  64u, 1u },
    { 18u,  98u, 46u, 38u,  99u,  72u, 2u },
    { 33u,  78u, 36u, 21u,  69u, 128u, 0u },
    { 24u,  90u, 42u, 34u,  87u,  42u, 1u },
    { 15u, 102u, 48u, 25u, 105u,  52u, 2u },
    { 30u,  82u, 38u, 39u,  75u, 108u, 0u },
    { 21u,  94u, 44u, 36u,  93u,  92u, 1u },
    { 15u,  64u, 38u, 43u,  62u,  92u, 0u },
    {  6u,  44u, 44u, 24u,  79u,  32u, 1u },
    { 21u,  56u, 34u, 20u,  95u,  88u, 1u },
    { 12u,  68u, 40u, 29u,  68u,  72u, 0u },
    { 27u,  48u, 46u, 44u,  84u, 128u, 1u },
    { 18u,  60u, 36u, 41u,  57u,  42u, 2u },
    {  9u,  72u, 42u, 34u,  73u,  52u, 0u },
    { 24u,  52u, 48u, 26u,  90u, 108u, 1u },
    { 26u,  48u, 42u, 46u,  84u, 108u, 0u },
    { 17u,  60u, 48u, 34u, 104u,  92u, 1u },
    {  8u,  40u, 38u, 52u,  70u,  32u, 2u },
    { 23u,  52u, 44u, 48u,  90u,  64u, 0u },
    { 14u,  64u, 34u, 40u, 111u,  72u, 1u },
    { 29u,  44u, 40u, 30u,  77u, 128u, 2u },
    { 20u,  56u, 46u, 50u,  98u,  42u, 0u },
    { 11u,  68u, 36u, 28u, 118u,  52u, 1u },
    {  4u,  48u, 46u, 54u,  61u,  52u, 0u },
    { 18u,  28u, 36u, 50u,  76u, 108u, 1u },
    {  9u,  40u, 42u, 42u,  52u,  92u, 2u },
    {  4u,  24u, 48u, 32u,  67u,  32u, 0u },
    { 15u,  32u, 38u, 53u,  80u,  64u, 1u },
    {  6u,  44u, 44u, 29u,  57u,  72u, 2u },
    { 21u,  24u, 34u, 48u,  71u, 128u, 0u },
    { 12u,  36u, 40u, 35u,  48u,  42u, 1u },
    { 22u,  68u, 34u, 36u,  92u,  42u, 0u },
    { 13u,  80u, 40u, 27u, 110u,  52u, 1u },
    { 28u,  60u, 46u, 45u,  78u, 108u, 2u },
    { 19u,  72u, 36u, 25u,  98u,  92u, 0u },
    { 10u,  52u, 42u, 41u,  66u,  32u, 1u },
    { 25u,  64u, 48u, 30u,  84u,  64u, 2u },
    { 16u,  76u, 38u, 47u, 104u,  72u, 0u },
    { 31u,  56u, 44u, 43u,  72u, 128u, 1u },
    { 37u,  90u, 38u, 40u,  78u, 128u, 0u },
    { 28u, 102u, 44u, 22u,  52u,  42u, 1u },
    { 36u, 118u, 34u, 28u,  92u, 120u, 2u },
    { 34u,  94u, 40u, 27u,  83u, 108u, 0u },
    { 25u, 106u, 46u, 42u,  57u,  92u, 1u },
    { 16u,  86u, 36u, 38u,  73u,  32u, 2u },
    { 31u,  98u, 42u, 32u,  88u,  64u, 0u },
    { 22u, 110u, 48u, 24u,  62u,  72u, 1u },
};

static inline uint32_t heavy_psych_trax_lead_gate32(uint8_t trax)
{
    if (heavy_psych_gen_hit(trax))
        return heavy_psych_gen.lead_gate;
    return HEAVY_PSYCH_TRAX_LEAD_GATE[heavy_psych_clamp_trax(trax)];
}

static inline uint16_t heavy_psych_trax_lead_gate16(uint8_t trax)
{
    return (uint16_t)(heavy_psych_trax_lead_gate32(trax) & 0xFFFFu);
}

/* 4-bar phrase arrangement across S1..S8 views:
 * Bar 0 (S1/S5): single entry + answer; Bar 1 (S2/S6): peak + release;
 * Bar 2 (S3/S7): peak chorus sustain; Bar 3 (S4/S8): sparse release & rest. */
static inline uint8_t heavy_psych_trax_lead_step_bit(uint8_t trax, uint8_t step)
{
    uint32_t g32 = heavy_psych_trax_lead_gate32(trax);
    uint8_t bar4 = (uint8_t)((step >> 4) & 3u);
    uint8_t s16 = (uint8_t)(step & 15u);

    if (bar4 == 0u || bar4 == 2u)
        return (uint8_t)((g32 >> s16) & 1u);
    if (bar4 == 1u)
        return (uint8_t)((g32 >> (16u + s16)) & 1u);
    /* Bar 3 (outro/release bar of 4-bar phrase): thin out to second-half turnaround only */
    if (s16 < 8u)
        return 0u;
    return (uint8_t)((g32 >> (16u + s16)) & 1u);
}

static inline int8_t heavy_psych_trax_lead_pitch(uint8_t trax, uint8_t step)
{
    if (heavy_psych_gen_hit(trax))
        return heavy_psych_gen_lead.pitch[step & 15u];
    return HEAVY_PSYCH_TRAX_LEAD_PITCH[heavy_psych_clamp_trax(trax)][step & 15u];
}

static inline uint8_t heavy_psych_trax_lead_len_ix(uint8_t trax, uint8_t step)
{
    if (heavy_psych_gen_hit(trax))
        return heavy_psych_gen_lead.len[step & 31u];
    return HEAVY_PSYCH_TRAX_LEAD_LEN[heavy_psych_clamp_trax(trax)][step & 31u];
}

static inline uint8_t heavy_psych_trax_lead_bend_u7(uint8_t trax, uint8_t step)
{
    if (heavy_psych_gen_hit(trax))
        return heavy_psych_gen_lead.bend[step & 31u];
    return HEAVY_PSYCH_TRAX_LEAD_BEND[heavy_psych_clamp_trax(trax)][step & 31u];
}

static inline uint8_t heavy_psych_lead_bend_plock(uint8_t bend_semi)
{
    if (bend_semi < 3u)
        return 0u;
    return (uint8_t)(32u + ((uint32_t)bend_semi * 95u) / 12u);
}

/* Not generated: a generated plate shadows its landmark slot, so it keeps that landmark's lead voice. */
static inline const heavy_psych_lead_voice_t *heavy_psych_trax_lead_voice(uint8_t trax)
{
    return &HEAVY_PSYCH_TRAX_LEAD_VOICE[heavy_psych_clamp_trax(trax)];
}

enum { HP_LEAD_LEN_N = 15u };
/* GATE UI + playback: fraction of one sequencer step (step = one 16th at 4/4). */
static const char *const HP_LEAD_LEN_NAMES[HP_LEAD_LEN_N] = {
    "1/16", "1/8", "1/4", "1/3", "1/2", "2/3", "3/4", "1/1", "5/4", "3/2",
    "7/4", "2/1", "5/2", "3/1", "4/1"
};
static const uint8_t HP_LEAD_LEN_NUM[HP_LEAD_LEN_N] = {
    1u, 1u, 1u, 1u, 1u, 2u, 3u, 1u, 5u, 3u, 7u, 2u, 5u, 3u, 4u
};
static const uint8_t HP_LEAD_LEN_DEN[HP_LEAD_LEN_N] = {
    16u, 8u, 4u, 3u, 2u, 3u, 4u, 1u, 4u, 2u, 4u, 1u, 2u, 1u, 1u
};
/* Legacy grid hint lengths (whole-note sixteenths); playback uses NUM/DEN × samples_per_step. */
static const uint8_t HP_LEAD_LEN_16THS[HP_LEAD_LEN_N] = {
    1u, 2u, 4u, 5u, 8u, 11u, 12u, 16u, 20u, 24u, 28u, 32u, 40u, 48u, 64u
};

static inline uint8_t heavy_psych_lead_len_ix(int8_t stored)
{
    int32_t ix = (int32_t)stored;

    if (ix < 0)
        ix = 0;
    if (ix >= (int32_t)HP_LEAD_LEN_N)
        ix = (int32_t)HP_LEAD_LEN_N - 1;
    return (uint8_t)ix;
}

static inline uint32_t heavy_psych_lead_len_samples(uint32_t samples_per_step, uint8_t ix)
{
    if (samples_per_step == 0u)
        return 0u;
    ix = heavy_psych_lead_len_ix((int8_t)ix);
    return (uint32_t)samples_per_step * (uint32_t)HP_LEAD_LEN_16THS[ix];
}

static inline uint8_t heavy_psych_lead_bend_semi(uint8_t bend)
{
    if (bend == 0u)
        return 0u;
    return (uint8_t)(((uint32_t)bend * 12u + 63u) / 127u);
}

static inline int8_t heavy_psych_lead_pitch_at(uint8_t trax, uint8_t step, int8_t step_tune)
{
    int32_t p;

    if (step_tune != 0)
        p = (int32_t)step_tune;
    else if (heavy_psych_trax_lead_step_bit(trax, step))
        p = (int32_t)heavy_psych_trax_lead_pitch(trax, step);
    else
        return 0;
    if (p < -127)
        p = -127;
    if (p > 127)
        p = 127;
    return (int8_t)p;
}

static inline int8_t heavy_psych_lead_bend_delta(uint8_t trax, uint8_t from_step, int8_t cur_tune,
                                                   const int8_t step_tune_row[DUB_SEQ_STEPS])
{
    uint32_t k;

    for (k = 1u; k <= DUB_SEQ_STEPS; k++) {
        uint8_t ns = (uint8_t)((from_step + k) % DUB_SEQ_STEPS);
        int8_t nxt;

        if (!heavy_psych_trax_lead_step_bit(trax, ns))
            continue;
        nxt = heavy_psych_lead_pitch_at(trax, ns, step_tune_row[ns]);
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

static inline void heavy_psych_lane3_plock_migrate(uint8_t step_vel_row[DUB_SEQ_STEPS],
                                                   int8_t step_micro_row[DUB_SEQ_STEPS])
{
    uint8_t st;

    for (st = 0u; st < DUB_SEQ_STEPS; st++) {
        step_vel_row[st] = 0u;
        step_micro_row[st] = 0;
    }
}

static inline uint8_t heavy_psych_lead_step_vel(uint8_t trax, uint8_t step)
{
    static const int8_t dyn[16] = {
        22, -18, 16, -22, 12, -16, 20, -12, 22, -20, 14, -14, 10, -22, 18, -16
    };
    uint8_t s16 = (uint8_t)(step & 15u);
    uint16_t gm = heavy_psych_trax_lead_gate16(trax);
    uint8_t prev = (uint8_t)((s16 + 15u) & 15u);
    uint8_t prev2 = (uint8_t)((s16 + 14u) & 15u);
    /* Accent (+5 dB -> vel 118..126) on step 0 or first note of a phrase; ±4 dB spread (58..102) */
    uint8_t phrase_start = (s16 == 0u) || (((gm >> s16) & 1u) && !((gm >> prev) & 1u) && !((gm >> prev2) & 1u));
    int32_t d = dyn[(s16 + trax * 3u) & 15u];
    int32_t v = phrase_start ? (120 + (d >> 2)) : (80 + d);

    if (v < 58)
        v = 58;
    else if (v > 126)
        v = 126;
    return (uint8_t)v;
}

enum { HEAVY_PSYCH_DRM_DENS_FLOOR = 2u }; /* pass band widened so gated steps never fully starve at density 127 */

static inline uint8_t heavy_psych_drm_density_allows(uint8_t density, uint8_t step, uint8_t voice_ix)
{
    uint8_t h = (uint8_t)((step * 17u + (voice_ix * 31u)) & 127u);
    uint16_t band = (uint16_t)density + (uint16_t)HEAVY_PSYCH_DRM_DENS_FLOOR;

    if (band > 128u)
        band = 128u;
    return h < (uint8_t)band;
}

static inline uint8_t heavy_psych_gate_after_drm_density(uint8_t on, uint8_t density, uint8_t step, uint8_t voice_ix)
{
    if (!on)
        return 0u;
    return heavy_psych_drm_density_allows(density, step, voice_ix);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const heavy_psych_trax_t *drum;
    const heavy_psych_trax_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = heavy_psych_trax(p->drum_pat);
    main = heavy_psych_trax(p->plate);
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
        siren = heavy_psych_trax_lead_step_bit(p->plate, step);
        sub = dub_step_bit(main->sub, step);
        chord = heavy_psych_chord_step_on(p->era, main->chord, step);
    }
    (void)apply_dens;
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

#include "heavy_psych_gen.h"

#endif
