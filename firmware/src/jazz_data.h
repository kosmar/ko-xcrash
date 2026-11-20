/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef JAZZ_DATA_H
#define JAZZ_DATA_H

#include "dub_types.h"
#include "jazz_conf.h"
#include "fm_seq.h"

enum { JAZZ_ERA_N = GENRE_ERA_N, JAZZ_TRAX_N = GENRE_PRESET_N };
enum { ERA_SPIRIT = 0, ERA_MODAL, ERA_MUTE, ERA_FUSION, ERA_ACID, ERA_BROKEN };
enum {
    BASS_UPRIGHT = 0, BASS_WALK, BASS_PIZZ, BASS_ORGAN,
    BASS_AMP, BASS_FUSION, BASS_MOOG, BASS_FM, BASS_EP,
    BASS_N
};
enum {
    DKIT_RIDE = 0, DKIT_BRUSH, DKIT_BOP, DKIT_COOL, DKIT_FUSION, DKIT_BROKEN, DKIT_ACID, DKIT_CR78, DKIT_N
};
enum {
    VOICE_MAJ7 = 0, VOICE_MIN7, VOICE_DOM9, VOICE_MIN9, VOICE_SUS4, VOICE_69,
    VOICE_QUART, VOICE_ADD9, VOICE_MAJ6, VOICE_DOM13, VOICE_MIN11, VOICE_ADD11,
    VOICE_N
};

typedef dub_plate_t jazz_trax_t;

static const char *const JAZZ_ERA_NAMES[JAZZ_ERA_N] = {
    "SPIRIT", "MODAL", "MUTE", "FUSION", "ACID", "BROKEN"
};

static const char *const DKIT_NAMES[DKIT_N] = {
    "RIDE", "BRUSH", "BOP", "COOL", "FUSION", "BROKEN", "ACID", "CR79"
};

static const uint8_t DKIT_ERA[DKIT_N] = {
    ERA_SPIRIT, ERA_SPIRIT, ERA_MODAL, ERA_MUTE, ERA_FUSION, ERA_BROKEN, ERA_ACID, ERA_MODAL
};

enum {
    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS
};

/* Per-kit ride/closed-hat voice (ride-first kits). */
static const int8_t jazz_hat_note[DKIT_N] = {
    96, 84, 93, 90, 99, 88, 103, 78
};
static const uint8_t jazz_hat_fm[DKIT_N] = {
    12, 8, 28, 18, 44, 22, 82, 6
};
static const uint8_t jazz_hat_noise[DKIT_N] = {
    24, 14, 48, 32, 58, 118, 72, 14
};
static const uint8_t jazz_hat_metal[DKIT_N] = {
    88, 102, 108, 96, 76, 42, 88, 120
};
static const uint8_t jazz_hat_decay[DKIT_N] = {
    8, 5, 9, 7, 8, 11, 3, 18
};
static const uint8_t jazz_hat_open[DKIT_N] = {
    12, 3, 11, 8, 9, 16, 2, 24
};
static const uint8_t jazz_hat_style[DKIT_N] = {
    HAT_STYLE_SIZZLE, HAT_STYLE_TICK, HAT_STYLE_TR6, HAT_STYLE_TONE,
    HAT_STYLE_TONE, HAT_STYLE_HISS, HAT_STYLE_TR6, HAT_STYLE_TONE
};
static const uint8_t jazz_hat_cut[DKIT_N] = {
    62, 52, 66, 58, 66, 34, 78, 38
};

static const char *const BASS_NAMES[BASS_N] = {
    "UPRIGHT", "WALK", "PIZZ", "ORGAN", "AMP", "FUSION", "MOCK", "FM", "EP"
};

static const uint8_t BASS_ERA[BASS_N] = {
    ERA_SPIRIT, ERA_SPIRIT, ERA_MODAL, ERA_MUTE,
    ERA_FUSION, ERA_FUSION, ERA_ACID, ERA_BROKEN, ERA_BROKEN
};

static const char *const VOICE_NAMES[VOICE_N] = {
    "MAJ7", "MIN7", "DOM9", "MIN9", "SUS4", "69",
    "QUART", "ADD9", "MAJ6", "DOM13", "MIN11", "ADD11"
};

static const jazz_trax_t JAZZ_TRAX[JAZZ_TRAX_N] = {
    /* SPIRIT 0 */ { "LATEHR", 0, 0, 0, 0, 0, 98, 100, 34, 32, 20, 38, 8, 88, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x9090u | ((uint32_t)0x4848u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 1 */ { "VELVET", 0, 1, 1, 1, 1, 99, 99, 34, 32, 21, 38, 9, 86, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 2 */ { "SMOKE", 0, 2, 2, 2, 2, 101, 98, 34, 32, 22, 38, 10, 84, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x9898u | ((uint32_t)0x9090u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 3 */ { "PORCH", 0, 1, 3, 3, 0, 102, 97, 34, 32, 23, 38, 11, 82, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x8822u | ((uint32_t)0x9090u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 4 */ { "AMBER", 0, 0, 4, 0, 1, 104, 96, 34, 32, 24, 38, 12, 80, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x9090u | ((uint32_t)0x8080u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 5 */ { "DRIFT", 0, 2, 5, 1, 2, 105, 95, 34, 32, 25, 38, 13, 78, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x8282u | ((uint32_t)0x9292u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 6 */ { "HUSHED", 0, 1, 6, 2, 0, 107, 94, 34, 32, 26, 38, 14, 76, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x4040u | ((uint32_t)0x9090u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 7 */ { "GLOW", 0, 0, 6, 3, 1, 108, 93, 34, 32, 27, 38, 15, 74, 0x1001u | ((uint32_t)0x1001u << 16), 0x0440u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x9090u | ((uint32_t)0x2020u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 6 */ { "BALLAD", 0, 1, 6, 2, 0, 86, 74, 46, 44, 26, 38, 14, 76, 0x1001u | ((uint32_t)0x1001u << 16), 0x0101u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x4040u | ((uint32_t)0x9090u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* SPIRIT 7 */ { "REPOSE", 0, 0, 6, 3, 1, 87, 73, 46, 44, 27, 38, 15, 74, 0x1001u | ((uint32_t)0x1001u << 16), 0x0101u | ((uint32_t)0x0440u << 16), 0x5555u | ((uint32_t)0x5555u << 16), 0x9090u | ((uint32_t)0x2020u << 16), 0x1212u | ((uint32_t)0x1212u << 16), 0x1111u | ((uint32_t)0x1111u << 16) },
    /* MODAL 0 */ { "OPEN5", 1, 0, 0, 0, 0, 128, 100, 34, 32, 20, 38, 8, 88, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x4848u | ((uint32_t)0x0808u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 1 */ { "PEDAL", 1, 1, 1, 1, 1, 129, 99, 34, 32, 21, 38, 9, 86, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x4040u | ((uint32_t)0x4848u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 2 */ { "AIRY", 1, 2, 2, 2, 2, 130, 98, 34, 32, 22, 38, 10, 84, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x2424u | ((uint32_t)0x4848u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 3 */ { "FLOAT", 1, 1, 3, 3, 0, 131, 97, 34, 32, 23, 38, 11, 82, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x4848u | ((uint32_t)0x1212u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 4 */ { "WIDE", 1, 0, 4, 0, 1, 132, 96, 34, 32, 24, 38, 12, 80, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x4444u | ((uint32_t)0x8888u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 5 */ { "STILL", 1, 2, 5, 1, 2, 133, 95, 34, 32, 25, 38, 13, 78, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x1010u | ((uint32_t)0x4040u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 6 */ { "HAZE", 1, 1, 7, 2, 0, 134, 94, 34, 32, 26, 38, 14, 76, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x4848u | ((uint32_t)0x8484u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 7 */ { "BREATH", 1, 0, 7, 3, 1, 135, 93, 34, 32, 27, 38, 15, 74, 0x1000u | ((uint32_t)0x1000u << 16), 0x0280u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x0202u | ((uint32_t)0x4848u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 6 */ { "MODAL", 1, 1, 7, 2, 0, 120, 74, 46, 44, 26, 38, 14, 76, 0x1000u | ((uint32_t)0x1000u << 16), 0x0080u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x4848u | ((uint32_t)0x8484u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MODAL 7 */ { "SERENE", 1, 0, 7, 3, 1, 121, 73, 46, 44, 27, 38, 15, 74, 0x1000u | ((uint32_t)0x1000u << 16), 0x0080u | ((uint32_t)0x0280u << 16), 0x3333u | ((uint32_t)0x3333u << 16), 0x0202u | ((uint32_t)0x4848u << 16), 0x2424u | ((uint32_t)0x2424u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* MUTE 0 */ { "CUP", 2, 0, 0, 0, 0, 105, 100, 34, 32, 20, 38, 8, 88, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x4040u | ((uint32_t)0x4000u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 1 */ { "WHISPR", 2, 1, 1, 1, 1, 106, 99, 34, 32, 21, 38, 9, 86, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x4000u | ((uint32_t)0x4040u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 2 */ { "CLOSE", 2, 2, 2, 2, 2, 108, 98, 34, 32, 22, 38, 10, 84, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x4040u | ((uint32_t)0x0400u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 3 */ { "SOFTLP", 2, 1, 3, 3, 0, 110, 97, 34, 32, 23, 38, 11, 82, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x0400u | ((uint32_t)0x4040u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 4 */ { "BRASSY", 2, 0, 4, 0, 1, 111, 96, 34, 32, 24, 38, 12, 80, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x4040u | ((uint32_t)0x0040u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 5 */ { "QUIET", 2, 2, 5, 1, 2, 113, 95, 34, 32, 25, 38, 13, 78, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x0040u | ((uint32_t)0x4000u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 6 */ { "SHADE", 2, 1, 8, 2, 0, 114, 94, 34, 32, 26, 38, 14, 76, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x4004u | ((uint32_t)0x0040u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 7 */ { "MUTED", 2, 0, 8, 3, 1, 116, 93, 34, 32, 27, 38, 15, 74, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x0040u | ((uint32_t)0x4004u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 6 */ { "MUFFLE", 2, 1, 8, 2, 0, 92, 74, 46, 44, 26, 38, 14, 76, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x4004u | ((uint32_t)0x0040u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* MUTE 7 */ { "CUPSO", 2, 0, 8, 3, 1, 94, 73, 46, 44, 27, 38, 15, 74, 0x1001u | ((uint32_t)0x0001u << 16), 0x8080u | ((uint32_t)0x8080u << 16), 0x1111u | ((uint32_t)0x1111u << 16), 0x0040u | ((uint32_t)0x4004u << 16), 0x0808u | ((uint32_t)0x0808u << 16), 0x1001u | ((uint32_t)0x1001u << 16) },
    /* FUSION 0 */ { "WIRE", 3, 3, 0, 0, 0, 104, 100, 34, 32, 20, 38, 8, 88, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x3030u | ((uint32_t)0x0C0Cu << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 1 */ { "CHOPBR", 3, 4, 1, 1, 1, 105, 99, 34, 32, 21, 38, 9, 86, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x3030u | ((uint32_t)0x6060u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 2 */ { "NIGHTB", 3, 8, 2, 2, 2, 106, 98, 34, 32, 22, 38, 10, 84, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x0C18u | ((uint32_t)0x3030u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 3 */ { "PHASE", 3, 4, 3, 3, 0, 107, 97, 34, 32, 23, 38, 11, 82, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x3030u | ((uint32_t)0x1818u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 4 */ { "TORQUE", 3, 8, 4, 0, 1, 108, 96, 34, 32, 24, 38, 12, 80, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x6060u | ((uint32_t)0x3030u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 5 */ { "NEON", 3, 5, 5, 1, 2, 109, 95, 34, 32, 25, 38, 13, 78, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x3030u | ((uint32_t)0x0303u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 6 */ { "FLARE", 3, 4, 9, 2, 0, 110, 94, 34, 32, 26, 38, 14, 76, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x1818u | ((uint32_t)0x3030u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 7 */ { "CRUISE", 3, 3, 9, 3, 1, 111, 93, 34, 32, 27, 38, 15, 74, 0x1001u | ((uint32_t)0x1001u << 16), 0x4040u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x3030u | ((uint32_t)0xC0C0u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 6 */ { "LATE", 3, 4, 9, 2, 0, 96, 74, 46, 44, 26, 38, 14, 76, 0x1001u | ((uint32_t)0x1001u << 16), 0x0101u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x1818u | ((uint32_t)0x3030u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* FUSION 7 */ { "BLUE", 3, 3, 9, 3, 1, 97, 73, 46, 44, 27, 38, 15, 74, 0x1001u | ((uint32_t)0x1001u << 16), 0x0101u | ((uint32_t)0x4040u << 16), 0x6666u | ((uint32_t)0x6666u << 16), 0x3030u | ((uint32_t)0xC0C0u << 16), 0x1818u | ((uint32_t)0x1818u << 16), 0x5555u | ((uint32_t)0x5555u << 16) },
    /* ACID 0 */ { "ACIDJ", 4, 3, 0, 0, 0, 115, 100, 34, 32, 20, 38, 8, 88, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0F0Fu | ((uint32_t)0x0A0Au << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 1 */ { "RESON", 4, 4, 1, 1, 1, 116, 99, 34, 32, 21, 38, 9, 86, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0A0Au | ((uint32_t)0x0F0Fu << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 2 */ { "PEAK", 4, 5, 2, 2, 2, 117, 98, 34, 32, 22, 38, 10, 84, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0F05u | ((uint32_t)0x0F0Fu << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 3 */ { "DRIVE", 4, 4, 3, 3, 0, 118, 97, 34, 32, 23, 38, 11, 82, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x3333u | ((uint32_t)0x0F0Fu << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 4 */ { "SQUEL", 4, 3, 4, 0, 1, 119, 96, 34, 32, 24, 38, 12, 80, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0F0Fu | ((uint32_t)0x0505u << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 5 */ { "CUTOFF", 4, 5, 5, 1, 2, 120, 95, 34, 32, 25, 38, 13, 78, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0505u | ((uint32_t)0x0F0Fu << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 6 */ { "RAVE", 4, 4, 10, 2, 0, 121, 94, 34, 32, 26, 38, 14, 76, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0F0Fu | ((uint32_t)0xF0F0u << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 7 */ { "PUMP", 4, 3, 10, 3, 1, 122, 93, 34, 32, 27, 38, 15, 74, 0x1111u | ((uint32_t)0x1111u << 16), 0x2222u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0xF0F0u | ((uint32_t)0x0F0Fu << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 6 */ { "COOL", 4, 4, 10, 2, 0, 108, 74, 46, 44, 26, 38, 14, 76, 0x1111u | ((uint32_t)0x1111u << 16), 0x0101u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x0F0Fu | ((uint32_t)0xF0F0u << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* ACID 7 */ { "MISTY", 4, 3, 10, 3, 1, 109, 73, 46, 44, 27, 38, 15, 74, 0x1111u | ((uint32_t)0x1111u << 16), 0x0101u | ((uint32_t)0x2222u << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0xF0F0u | ((uint32_t)0x0F0Fu << 16), 0x4444u | ((uint32_t)0x4444u << 16), 0xA0A0u | ((uint32_t)0xA0A0u << 16) },
    /* BROKEN 0 */ { "FRACT", 5, 7, 0, 0, 0, 125, 100, 34, 32, 20, 38, 8, 88, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6363u | ((uint32_t)0x9696u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 1 */ { "STUTTR", 5, 4, 1, 1, 1, 126, 99, 34, 32, 21, 38, 9, 86, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6363u | ((uint32_t)0x3333u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 2 */ { "GLITCH", 5, 7, 2, 2, 2, 127, 98, 34, 32, 22, 38, 10, 84, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x3C3Cu | ((uint32_t)0x6363u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 3 */ { "CHOP", 5, 4, 3, 3, 0, 128, 97, 34, 32, 23, 38, 11, 82, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6363u | ((uint32_t)0xC3C3u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 4 */ { "SKIP", 5, 3, 4, 0, 1, 129, 96, 34, 32, 24, 38, 12, 80, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x5151u | ((uint32_t)0x6363u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 5 */ { "OFFK", 5, 5, 5, 1, 2, 130, 95, 34, 32, 25, 38, 13, 78, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6363u | ((uint32_t)0x1515u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 6 */ { "SYNCOP", 5, 4, 11, 2, 0, 131, 94, 34, 32, 26, 38, 14, 76, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6969u | ((uint32_t)0x6363u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 7 */ { "SHUFF", 5, 3, 11, 3, 1, 132, 93, 34, 32, 27, 38, 15, 74, 0x1248u | ((uint32_t)0x1248u << 16), 0x4824u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6363u | ((uint32_t)0x9999u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 6 */ { "BROKEN", 5, 4, 11, 2, 0, 118, 74, 46, 44, 26, 38, 14, 76, 0x1248u | ((uint32_t)0x1248u << 16), 0x0800u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6969u | ((uint32_t)0x6363u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },
    /* BROKEN 7 */ { "HUSH", 5, 3, 11, 3, 1, 119, 73, 46, 44, 27, 38, 15, 74, 0x1248u | ((uint32_t)0x1248u << 16), 0x0800u | ((uint32_t)0x4824u << 16), 0xA5A5u | ((uint32_t)0x5A5Au << 16), 0x6363u | ((uint32_t)0x9999u << 16), 0x3C3Cu | ((uint32_t)0xC3C3u << 16), 0x2929u | ((uint32_t)0x2929u << 16) },

};

static const int8_t JAZZ_TRAX_ROOT[JAZZ_TRAX_N] = {48, 50, 51, 53, 55, 57, 52, 54, 58, 60, 58, 60, 50, 53, 55, 48, 59, 61, 65, 67, 51, 48, 55, 53, 50, 57, 49, 52, 55, 58, 53, 55, 48, 51, 57, 50, 54, 56, 60, 62, 50, 48, 55, 53, 45, 48, 47, 49, 53, 55, 46, 43, 50, 48, 45, 52, 51, 53, 57, 59};

static const int8_t JAZZ_TRAX_SCALE[JAZZ_TRAX_N] = {0, 1, 2, 4, 5, 0, 1, 5, 7, 11, 1, 2, 4, 0, 5, 1, 4, 0, 10, 6, 0, 2, 1, 4, 5, 0, 0, 2, 6, 8, 1, 4, 0, 5, 2, 1, 1, 5, 7, 11, 4, 5, 0, 1, 2, 4, 4, 2, 10, 8, 2, 0, 5, 1, 4, 2, 0, 1, 6, 7};

static const int8_t JAZZ_TRAX_SWING[JAZZ_TRAX_N] = {62, 63, 64, 65, 66, 67, 68, 69, 74, 75, 60, 61, 62, 63, 64, 65, 66, 67, 72, 73, 64, 65, 66, 67, 68, 69, 70, 71, 76, 77, 54, 55, 56, 57, 58, 59, 60, 61, 66, 67, 48, 49, 50, 51, 52, 53, 54, 55, 60, 61, 56, 57, 58, 59, 60, 61, 62, 63, 68, 69};

static const int8_t JAZZ_TRAX_LEAD_RATE[JAZZ_TRAX_N] = {-4, -3, -2, -1, 0, 1, 2, 3, 8, 9, -4, -3, -2, -1, 0, 1, 2, 3, 8, 9, -4, -3, -2, -1, 0, 1, 2, 3, 8, 9, -4, -3, -2, -1, 0, 1, 2, 3, 8, 9, -4, -3, -2, -1, 0, 1, 2, 3, 8, 9, -4, -3, -2, -1, 0, 1, 2, 3, 8, 9};

/* COMP / SWAY (wob) / WASH / MUTE + motion/flux. Combo WALK = Kick+Bass ens. */
static const uint8_t JAZZ_ERA_MACROS[JAZZ_ERA_N][10] = {
    { 70, 72, 58, 52, 54, 82, 28, 38, 0, 40 }, /* SPIRIT */
    { 68, 70, 56, 48, 52, 80, 26, 36, 0, 38 }, /* MODAL */
    { 72, 74, 60, 55, 50, 84, 30, 40, 0, 42 }, /* MUTE */
    { 62, 58, 52, 44, 58, 76, 32, 42, 0, 44 }, /* FUSION */
    { 58, 48, 64, 40, 48, 72, 44, 36, 0, 48 }, /* ACID */
    { 64, 66, 54, 50, 56, 78, 36, 44, 0, 46 }, /* BROKEN */
};

typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} jazz_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} jazz_era_fx_t;

static const jazz_era_base_t JAZZ_ERA_BASE[JAZZ_ERA_N] = {
    /* cutoff_b darker; sub_oct 0=−1oct, >80=−2oct. ACID/BROKEN amp-saw → −2oct */
    { 34, 90, 92, 20, 44, 40, 96, 0, 14, 28, 76, 56, 58, 64, 70, 14, 100, 42, 36, {64,68,52,76}, 10, 54, 26, 74 },
    { 36, 88, 90, 22, 48, 34, 94, 0, 12, 30, 74, 54, 56, 64, 68, 16, 98, 38, 38, {64,70,50,74}, 8, 52, 24, 72 },
    { 32, 86, 94, 18, 40, 46, 98, 0, 10, 32, 72, 52, 54, 64, 72, 12, 96, 36, 40, {62,68,48,72}, 12, 56, 22, 70 },
    /* FUSION: −2oct + warmer cutoff + longer decay for fat amp bass */
    { 40, 84, 86, 24, 44, 44, 104, 96, 28, 24, 78, 58, 52, 64, 64, 18, 102, 48, 32, {64,72,54,78}, 6, 48, 30, 62 },
    /* ACID/BROKEN: sub_oct 96=−2oct; cutoff open enough that amp-saw stays audible */
    { 44, 82, 84, 40, 52, 54, 88, 96, 22, 18, 80, 60, 62, 64, 56, 22, 104, 86, 28, {64,74,50,82}, 4, 44, 36, 48 },
    { 38, 88, 88, 32, 48, 46, 92, 96, 16, 26, 70, 50, 48, 64, 60, 16, 100, 72, 34, {64,72,54,70}, 14, 50, 32, 66 },
};

static const jazz_era_fx_t JAZZ_ERA_FX[JAZZ_ERA_N] = {
    { DKIT_RIDE,  28, 1, 40, 18, 78, 36, 38, 62, 52,  72, BASS_UPRIGHT,  6 },
    { DKIT_BOP,   32, 2, 42, 16, 74, 34, 36, 58, 48,  68, BASS_WALK,     8 },
    { DKIT_COOL,  36, 1, 38, 14, 82, 40, 42, 66, 56,  76, BASS_PIZZ,     4 },
    { DKIT_FUSION,24, 0, 44, 20, 70, 32, 34, 54, 44,  64, BASS_EP,       8 },
    { DKIT_ACID,  12, 0, 52, 28, 62, 26, 28, 48, 38,  58, BASS_FUSION,  22 },
    { DKIT_BROKEN,44, 3, 46, 22, 76, 38, 40, 60, 50,  70, BASS_FM,      16 },
};

/* Bass melodic contour: era × slot × 8 — keep low (deg ≤4; rare 5). High 5/6 made bass thin. */
static const uint8_t JAZZ_BASS_WALK[JAZZ_ERA_N][GENRE_PRESETS_PER_ERA][8] = {
    { { 0, 2, 3, 2, 0, 3, 2, 0 }, { 0, 2, 0, 3, 2, 0, 3, 2 }, { 0, 3, 2, 0, 2, 3, 0, 2 }, { 0, 2, 3, 0, 2, 0, 3, 2 }, { 0, 3, 2, 3, 0, 2, 0, 3 }, { 0, 2, 0, 2, 3, 0, 2, 0 }, { 0, 2, 3, 2, 0, 3, 0, 2 }, { 0, 2, 3, 0, 2, 3, 2, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 2, 0, 0, 3, 0 }, { 0, 0, 2, 0, 0, 3, 0, 0 }, { 0, 0, 0, 0, 2, 0, 0, 3 }, { 0, 2, 0, 0, 0, 0, 3, 0 }, { 0, 0, 3, 0, 0, 2, 0, 0 }, { 0, 0, 0, 2, 0, 0, 0, 3 }, { 0, 2, 0, 0, 3, 0, 0, 0 }, { 0, 0, 0, 0, 0, 2, 0, 3 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 2, 0, 0, 3, 0, 0 }, { 0, 2, 0, 0, 3, 0, 0, 2 }, { 0, 0, 0, 2, 0, 0, 3, 0 }, { 0, 2, 0, 3, 0, 0, 2, 0 }, { 0, 0, 3, 0, 2, 0, 0, 0 }, { 0, 3, 0, 0, 0, 2, 0, 0 }, { 0, 0, 2, 0, 0, 0, 0, 2 }, { 0, 2, 0, 0, 0, 3, 2, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 2, 3, 1, 2, 0, 3, 0 }, { 0, 1, 2, 3, 0, 2, 1, 0 }, { 0, 2, 0, 3, 2, 1, 0, 2 }, { 0, 3, 2, 0, 1, 2, 3, 0 }, { 0, 2, 0, 1, 3, 2, 0, 2 }, { 2, 0, 3, 0, 1, 2, 0, 3 }, { 0, 2, 3, 2, 0, 1, 2, 0 }, { 0, 2, 0, 3, 1, 0, 2, 3 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 2, 0, 0, 2, 0, 0 }, { 0, 2, 0, 2, 0, 2, 0, 2 }, { 0, 0, 0, 2, 0, 0, 0, 2 }, { 0, 2, 2, 0, 0, 2, 2, 0 }, { 0, 0, 3, 0, 0, 3, 0, 0 }, { 0, 3, 0, 3, 0, 2, 0, 3 }, { 0, 2, 0, 2, 0, 2, 3, 2 }, { 0, 0, 2, 3, 0, 0, 2, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 3, 1, 2, 0, 3, 2, 0 }, { 0, 2, 0, 3, 1, 2, 0, 3 }, { 3, 0, 1, 2, 0, 0, 2, 3 }, { 0, 2, 3, 1, 0, 2, 0, 3 }, { 1, 0, 2, 0, 3, 2, 0, 0 }, { 0, 3, 2, 1, 0, 2, 0, 3 }, { 2, 0, 3, 0, 1, 2, 0, 3 }, { 0, 1, 2, 0, 3, 0, 2, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } }

};

#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM trax at jazz_gen.slot. */
static fm_pgen_t jazz_gen = { .slot = 0xFFu };
static fm_pgen_lead_t jazz_gen_lead;

#define FM_PGEN_STATE jazz_gen
#define FM_PGEN_BUILD(era, n, c) jazz_gen_build(era, n, c)

static inline uint8_t jazz_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&jazz_gen, ix);
}

static inline uint8_t jazz_clamp_trax(uint8_t ix)
{
    return ix >= JAZZ_TRAX_N ? (uint8_t)(JAZZ_TRAX_N - 1u) : ix;
}

static inline uint8_t jazz_clamp_era(uint8_t era)
{
    return era >= JAZZ_ERA_N ? (uint8_t)(JAZZ_ERA_N - 1u) : era;
}

static inline const char *jazz_era_name(uint8_t era)
{
    return JAZZ_ERA_NAMES[jazz_clamp_era(era)];
}

static inline const char *jazz_trax_name(uint8_t trax)
{
    if (jazz_gen_hit(trax))
        return jazz_gen.name;
    return JAZZ_TRAX[jazz_clamp_trax(trax)].name;
}

static inline const jazz_trax_t *jazz_trax(uint8_t ix)
{
    if (jazz_gen_hit(ix))
        return &jazz_gen.pl;
    return &JAZZ_TRAX[jazz_clamp_trax(ix)];
}

static inline uint8_t jazz_era_of_trax(uint8_t trax)
{
    trax = jazz_clamp_trax(trax);
    return (uint8_t)(trax / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t jazz_era_first_trax(uint8_t era)
{
    return (uint8_t)(jazz_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint8_t jazz_bass_walk_deg(uint8_t era, uint8_t piece, uint8_t step)
{
    era = jazz_clamp_era(era);
    piece = (uint8_t)(jazz_clamp_trax(piece) % GENRE_PRESETS_PER_ERA);
    return JAZZ_BASS_WALK[era][piece][step & 7u];
}

static const int8_t JAZZ_TRAX_KIT[JAZZ_TRAX_N] = {0, 0, 1, 0, 2, 1, 0, 2, 6, 8, 1, 1, 2, 3, 1, 2, 3, 1, 9, 7, 2, 2, 3, 2, 1, 2, 3, 2, 9, 8, 3, 4, 3, 4, 3, 4, 5, 4, 11, 10, 6, 6, 6, 6, 6, 6, 6, 6, 12, 12, 5, 5, 5, 5, 5, 5, 5, 5, 11, 11};

static inline uint8_t jazz_trax_root(uint8_t ix)
{
    return jazz_gen_hit(ix) ? (uint8_t)jazz_gen.root : (uint8_t)JAZZ_TRAX_ROOT[jazz_clamp_trax(ix)];
}

static inline uint8_t jazz_trax_scale(uint8_t ix)
{
    return jazz_gen_hit(ix) ? (uint8_t)jazz_gen.scale : (uint8_t)JAZZ_TRAX_SCALE[jazz_clamp_trax(ix)];
}

static inline uint8_t jazz_trax_swing(uint8_t ix)
{
    return jazz_gen_hit(ix) ? (uint8_t)jazz_gen.swing : (uint8_t)JAZZ_TRAX_SWING[jazz_clamp_trax(ix)];
}

static inline uint8_t jazz_trax_kit(uint8_t ix)
{
    return jazz_gen_hit(ix) ? (uint8_t)jazz_gen.kit : (uint8_t)JAZZ_TRAX_KIT[jazz_clamp_trax(ix)];
}

static inline int8_t jazz_trax_lead_rate(uint8_t ix)
{
    return jazz_gen_hit(ix) ? jazz_gen.rate : JAZZ_TRAX_LEAD_RATE[jazz_clamp_trax(ix)];
}

static const uint32_t JAZZ_TRAX_LEAD_GATE[JAZZ_TRAX_N] = {
    0x0C280C2Cu, 0x08984848u, 0x242404A4u, 0x40864444u, 0x08242824u, 0x48480A58u, 0x282420ACu, 0x04642624u,
    0x08200424u, 0x04044444u, 0x04000401u, 0x04000020u, 0x00200404u, 0x20000022u, 0x0100u | ((uint32_t)0x0004u << 16), 0x20000404u,
    0x00202020u, 0x04000001u, 0x20000020u, 0x00200404u, 0x04200024u, 0x0090u | ((uint32_t)0x0200u << 16), 0x00480048u, 0x004Au | ((uint32_t)0x0200u << 16),
    0x00140410u, 0x20200220u, 0x00800484u, 0x02200022u, 0x00200410u, 0x04000404u, 0x0A5A0E6Au, 0x0E684A6Au,
    0x8A9A2AAAu, 0x4624466Eu, 0x2A8A8A9Au, 0x24B4A5B4u, 0x46482A6Eu, 0x4A9A4A5Au, 0x41480AA4u, 0x04444444u,
    0x04240464u, 0x22220222u, 0x8888088Cu, 0x06440464u, 0x24242424u, 0x282A082Au, 0x82A022A2u, 0x48444444u,
    0x04820442u, 0x44404440u, 0x48084442u, 0x80A02808u, 0x880C0501u, 0x242280A0u, 0x1030A404u, 0xA0A0200Au,
    0x84048490u, 0x54104404u, 0x40800410u, 0x40404404u,
};

static const int8_t JAZZ_TRAX_LEAD_PITCH[JAZZ_TRAX_N][32] = {
    {    0,    0,    0,    3,    0,    7,    0,    0,    0,    0,   14,   10,    0,    0,    0,    0,    0,    0,    0,   10,    0,    7,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0 }, /* SPIRIT 0 */
    {    0,    0,    0,    3,    0,    0,    7,    0,    0,    0,    0,    8,    0,    0,   12,    0,    0,    0,    0,   12,    8,    0,    0,    7,    0,    0,    0,    3,    0,    0,    0,    0 }, /* SPIRIT 1 */
    {    0,    0,    7,    0,    0,    9,    0,   12,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,   12,    0,    0,    0,    0,    9,    0,    0,    7,    0,    0 }, /* SPIRIT 2 */
    {    0,    0,    9,    0,    0,    0,   12,    0,    0,    0,    7,    0,    0,    0,    4,    0,    0,    4,    7,    0,    0,    0,    0,    9,    0,    0,    0,    0,    0,    0,   12,    0 }, /* SPIRIT 3 */
    {    0,    0,   12,    0,    0,    7,    0,    0,    0,    0,    0,    4,    0,    9,    0,    0,    0,    0,    4,    0,    0,    7,    0,    0,    0,    0,    0,   12,    0,    0,    0,    0 }, /* SPIRIT 4 */
    {    0,    0,    0,    7,    3,    0,   10,    0,    0,   17,    0,   14,    0,    0,    0,    0,    0,    0,    0,   10,    0,    0,   14,    0,    0,    0,    0,    7,    0,    0,    3,    0 }, /* SPIRIT 5 */
    {    0,    0,    0,    3,    0,    7,    0,    8,    0,    0,    0,    0,    0,   12,    0,    0,    0,    0,    8,    0,    0,    7,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0 }, /* SPIRIT 6 */
    {    0,    0,    4,    0,    0,    7,    0,    0,    0,   12,    9,    0,    0,    7,    0,    0,    0,    0,   12,    0,    0,    9,    7,    0,    0,    0,    4,    0,    0,    0,    0,    0 }, /* SPIRIT 7 */
    {    0,    0,    7,    0,    0,   10,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0 }, /* SPIRIT 8 */
    {    0,    0,    9,    0,    0,    0,    7,    0,    0,    0,    7,    0,    0,    0,    3,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0 }, /* SPIRIT 9 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0 }, /* MODAL 0 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0 }, /* MODAL 1 */
    {    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MODAL 2 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0 }, /* MODAL 3 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MODAL 4 */
    {    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MODAL 5 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MODAL 6 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0 }, /* MODAL 7 */
    {    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0 }, /* MODAL 8 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   10,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MODAL 9 */
    {    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 0 */
    {    0,    0,    0,    0,    3,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0 }, /* MUTE 1 */
    {    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    8,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 2 */
    {    0,    7,    0,    4,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 3 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    4,    0,    0,    0,    0,    0,    0,    0,    7,    0,   12,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 4 */
    {    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0 }, /* MUTE 5 */
    {    0,    0,    3,    0,    0,    0,    0,    5,    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 6 */
    {    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 7 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* MUTE 8 */
    {    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0 }, /* MUTE 9 */
    {    0,    0,    0,    3,    0,    7,    8,    0,    0,   12,   15,   12,    0,    0,    0,    0,    0,    7,    0,   10,   12,    0,   15,    0,    0,   17,    0,   15,    0,    0,    0,    0 }, /* FUSION 0 */
    {    0,    9,    0,   12,    0,   16,   12,    0,    0,    9,    0,    7,    0,    0,    4,    0,    0,    0,    0,   16,    0,   17,   16,    0,    0,   12,   11,    7,    0,    0,    0,    0 }, /* FUSION 1 */
    {    0,   14,    0,   10,    0,    7,    0,    3,    0,    5,    0,   10,    0,   14,    0,    0,    0,   14,    0,   12,    7,    0,    0,   10,    0,   14,    0,   17,    0,    0,    0,   12 }, /* FUSION 2 */
    {    0,    4,    7,    9,    0,   12,   16,    0,    0,   12,    9,    0,    0,    0,    7,    0,    0,    0,   10,    0,    0,   12,    0,    0,    0,   16,   17,    0,    0,    0,   16,    0 }, /* FUSION 3 */
    {    0,   12,    0,   15,   12,    0,    0,    9,    0,    7,    0,    3,    0,    0,    0,    5,    0,   17,    0,   15,    0,    0,    0,   12,    0,   10,    0,    7,    0,    9,    0,    0 }, /* FUSION 4 */
    {    0,    0,    8,    0,    7,    3,    0,    5,    8,    0,   12,    0,    0,    7,    0,   15,    0,    0,   10,    0,    7,    8,    0,   12,    0,    0,   15,    0,    0,   10,    0,    0 }, /* FUSION 5 */
    {    0,    7,    8,   12,    0,   15,   12,    0,    0,    8,    0,    7,    0,    3,    0,    0,    0,    0,    0,   12,    0,    0,   15,    0,    0,   17,   15,    0,    0,    0,   12,    0 }, /* FUSION 6 */
    {    0,   16,    0,   12,    9,    0,    7,    0,    0,    4,    0,    5,    0,    0,    9,    0,    0,   16,    0,   12,   10,    0,    0,    7,    0,    9,    0,   12,    0,    0,   16,    0 }, /* FUSION 7 */
    {    0,    0,    0,    0,    0,    5,    0,    7,    0,   10,    0,    7,    0,    0,    0,    0,    0,    0,    0,   15,    0,    0,   10,    0,    7,    0,    0,    0,    0,    0,    5,    0 }, /* FUSION 8 */
    {    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    7,    0,    0,    0,    9,    0,    0,    0,   12,    0,    0,    0,    9,    0,    0,    0,    7,    0,    0,    0,    3,    0 }, /* FUSION 9 */
    {    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* ACID 0 */
    {    0,    0,    0,    0,    0,    5,    0,    0,    0,   12,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,   12,    0,    0,    0,    5,    0,    0,    0,    7,    0,    0 }, /* ACID 1 */
    {    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,   14,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,   14,    0,    0,    0,    0 }, /* ACID 2 */
    {    0,    0,   12,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,   12,    0,    0,    0,    0,    0 }, /* ACID 3 */
    {    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    9,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    9,    0,    0,    0,    0,    0 }, /* ACID 4 */
    {    0,    0,    0,   12,    0,    5,    0,    0,    0,    0,    0,   12,    0,    0,    0,    0,    0,   12,    0,    5,    0,   12,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0 }, /* ACID 5 */
    {    0,    5,    0,    0,    0,    0,    0,    0,    0,   12,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   12,    0,    0,    0,    0,    0,    5 }, /* ACID 6 */
    {    0,    0,    0,    0,    0,    0,    9,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    9,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    7,    0 }, /* ACID 7 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0 }, /* ACID 8 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* ACID 9 */
    {    0,    0,    0,    0,    0,    0,   10,    0,    0,    0,    3,    0,    0,    0,   12,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,   10,    0,    0,    0,    0 }, /* BROKEN 0 */
    {    0,    0,    0,   14,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,   10,    0,    0,    0,    0,    0,    0,    0,    0,    0,   17 }, /* BROKEN 1 */
    {    4,    0,    0,    0,    0,    0,    0,    0,   14,    0,    0,    0,    0,    0,    0,    0,    0,    0,    7,    2,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,   16 }, /* BROKEN 2 */
    {    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,   12,    0,   10,    0,    0,    0,    0,    0,    0,    0,    0,   12,    0,    0,    8,    0,    0 }, /* BROKEN 3 */
    {    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,   12,    0,    0,    5,    0,   14,    0,    0,    0,    0,    4,   16,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, /* BROKEN 4 */
    {    0,   10,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,    0,    5,    0,   14,    0,    0,    0,    0,    0,    0,    0,   12 }, /* BROKEN 5 */
    {    0,    0,    0,    0,    0,    0,    0,   10,    0,    0,   15,    0,    0,    0,    0,    3,    0,    0,   17,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    2 }, /* BROKEN 6 */
    {    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,   15,    0,    0,    0,    0,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,   12,    0,    3,    0,    0,    0 }, /* BROKEN 7 */
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   17,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,   15,    0 }, /* BROKEN 8 */
    {    0,    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   17,    0,    0,    0,    0,    0,    0,    0,   18,    0,    0,    0,    0,    0,    0,    0,   10,    0 }, /* BROKEN 9 */
};

static const uint8_t JAZZ_TRAX_LEAD_LEN[JAZZ_TRAX_N][32] = {
    {   0,   0,   0,   1,   0,   2,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   2,   0,   3,   0,   0,   0,   0,   1,   5,   0,   0,   0,   0 },
    {   0,   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   1,   0,   0,   4,   0,   0,   0,   0,   1,   2,   0,   0,   3,   0,   0,   0,   5,   0,   0,   0,   0 },
    {   0,   0,   1,   0,   0,   1,   0,   1,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   3,   0,   0,   0,   0,   2,   0,   0,   5,   0,   0 },
    {   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   4,   0,   0,   1,   3,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   5,   0 },
    {   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   1,   0,   4,   0,   0,   0,   0,   2,   0,   0,   3,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   1,   0,   1,   0,   0,   1,   0,   4,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   3,   0,   0,   0,   0,   2,   0,   0,   5,   0 },
    {   0,   0,   0,   1,   0,   1,   0,   2,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   2,   0,   0,   3,   0,   0,   0,   0,   0,   2,   0,   5,   0,   0 },
    {   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   1,   0,   0,   4,   0,   0,   0,   0,   2,   0,   0,   1,   3,   0,   0,   0,   5,   0,   0,   0,   0,   0 },
    {   0,   0,   1,   0,   0,   3,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0 },
    {   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   7,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0 },
    {   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   4,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0 },
    {   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  11,   0,   0 },
    {   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  11,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   1,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   1,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0 },
    {   0,   0,   2,   0,   0,   0,   0,   1,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   2,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   1,   0,   1,   0,   1,   1,   0,   0,   1,   1,   7,   0,   0,   0,   0,   0,   7,   0,   7,   7,   0,   7,   0,   0,   7,   0,   7,   0,   0,   0,   0 },
    {   0,   1,   0,   1,   0,   1,   1,   0,   0,   1,   0,   1,   0,   0,   7,   0,   0,   0,   0,   7,   0,   7,   7,   0,   0,   7,   7,   7,   0,   0,   0,   0 },
    {   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   7,   0,   0,   0,   7,   0,   7,   7,   0,   0,   7,   0,   7,   0,   7,   0,   0,   0,   7 },
    {   0,   1,   1,   1,   0,   1,   1,   0,   0,   1,   2,   0,   0,   0,   7,   0,   0,   0,   7,   0,   0,   7,   0,   0,   0,   7,   7,   0,   0,   0,   7,   0 },
    {   0,   1,   0,   1,   1,   0,   0,   1,   0,   1,   0,   2,   0,   0,   0,   7,   0,   7,   0,   7,   0,   0,   0,   7,   0,   7,   0,   7,   0,   7,   0,   0 },
    {   0,   0,   1,   0,   1,   1,   0,   1,   1,   0,   1,   0,   0,   1,   0,   7,   0,   0,   7,   0,   7,   7,   0,   7,   0,   0,   7,   0,   0,   7,   0,   0 },
    {   0,   1,   1,   1,   0,   1,   1,   0,   0,   1,   0,   1,   0,   7,   0,   0,   0,   0,   0,   7,   0,   0,   7,   0,   0,   7,   7,   0,   0,   0,   7,   0 },
    {   0,   1,   0,   1,   1,   0,   1,   0,   0,   1,   0,   1,   0,   0,   7,   0,   0,   7,   0,   7,   7,   0,   0,   7,   0,   7,   0,   7,   0,   0,   7,   0 },
    {   0,   0,   1,   0,   0,   1,   0,   1,   0,   1,   0,   8,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   8,   0,   8,   0,   0,   0,   0,   0,   8,   0 },
    {   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   2,   0,   0,   0,   8,   0,   0,   0,   8,   0,   0,   0,   8,   0,   0,   0,   8,   0,   0,   0,   0,   0 },
    {   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0 },
    {   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0 },
    {   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1 },
    {   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0 },
    {   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   1,   0,   0 },
    {   0,   1,   0,   1,   0,   1,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   1,   0,   1,   0,   1,   0,   0,   0,   0,   0,   1,   0,   1,   0,   0 },
    {   0,   1,   0,   0,   0,   1,   0,   1,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   1,   0,   1,   0,   0,   0,   0,   0,   1 },
    {   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   1,   0 },
    {   0,   1,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0 },
    {   0,   1,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   4,   0 },
    {   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   4 },
    {   0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   4,   0,   0 },
    {   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   1,   0,   1,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0 },
    {   0,   1,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   0,   0,   0,   1,   0,   4 },
    {   0,   0,   0,   0,   1,   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   1,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   4 },
    {   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   1,   0,   1,   0,   4,   0 },
    {   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   4,   0 },
    {   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   4,   0 },
};

static const uint8_t JAZZ_TRAX_LEAD_BEND[JAZZ_TRAX_N][32] = {
    {   0,   0,   3,   4,   0,   7,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   4,   0,   0,   0,   0,   3,   2,   0,   0,   0,   0 },
    {   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,   2,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   3,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0 },
    {   0,   0,   3,   0,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   2,   0 },
    {   0,   0,   5,   0,   0,   3,   0,   0,   0,   0,   0,   5,   0,   5,   0,   0,   0,   0,   3,   0,   0,   5,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0 },
    {   0,   0,   0,   4,   7,   0,   7,   0,   0,   3,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   7,   0,   0,   0,   0,   4,   0,   0,   2,   0 },
    {   0,   0,   3,   4,   0,   0,   0,   4,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   3,   0,   2,   0,   0 },
    {   0,   0,   3,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   5,   0,   0,   0,   0,   3,   0,   0,   0,   3,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   3,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   3,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   4,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   5,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   3,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   2,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   7,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   3,   0,   4,   0,   0,   4,   0,   0,   3,   3,   5,   0,   0,   0,   0,   0,   3,   0,   0,   3,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0 },
    {   0,   3,   0,   4,   0,   4,   3,   0,   0,   0,   0,   3,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   2,   0,   0,   0,   0 },
    {   0,   4,   0,   3,   0,   4,   0,   0,   0,   5,   0,   4,   0,   0,   0,   0,   0,   0,   0,   5,   3,   0,   0,   4,   0,   3,   0,   5,   0,   0,   0,   2 },
    {   0,   3,   0,   3,   0,   4,   4,   0,   0,   3,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   2,   0 },
    {   0,   3,   0,   3,   3,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,  12,   0,   0,   0,   3,   0,   0,   0,   0,   0,   3,   0,   0,   0,   2,   0,   0 },
    {   0,   0,   0,   0,   4,   0,   0,   3,   4,   0,   5,   0,   0,   8,   0,   5,   0,   0,   3,   0,   0,   4,   0,   3,   0,   0,   5,   0,   0,   2,   0,   0 },
    {   0,   0,   4,   3,   0,   3,   4,   0,   0,   0,   0,   4,   0,   9,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   2,   0 },
    {   0,   4,   0,   3,   0,   0,   3,   0,   0,   0,   0,   4,   0,   0,   7,   0,   0,   4,   0,   0,   3,   0,   0,   0,   0,   3,   0,   4,   0,   0,   2,   0 },
    {   0,   0,   5,   0,   0,   0,   0,   3,   0,   3,   0,   8,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   2,   0 },
    {   0,   0,   3,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   2,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   5,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   5,   0,   0,   0,   7,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   7,   0,   0,   0,   0,  12,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,  12,   0,   0,   0,  12,   0,   0,   0,   0 },
    {   0,   0,  12,   0,   0,   5,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   5,   0,   0,  12,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   0,   0,   5,   0,   0,   0,   0,   9,   0,   0,   4,   0,   0,   0,   0,   5,   0,   0,   9,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0 },
    {   0,  12,   0,   7,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   7,   0,  12,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0 },
    {   0,   5,   0,   0,   0,   0,   0,  12,   0,   7,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,  12,   0,   7,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   9,   0,   0,   0,   4,   0,   0,   0,   5,   0,   0,   0,   9,   0,   0,   0,   4,   0,   0,   0,   5,   0,   0,   0,   0,   7,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   7,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,  10,   0,   0,   0,   0,   7,   0,   0,   0,   9,   0,   0,   0,   7,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0 },
    {   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   7,   0,   3,   0,   0,   0,   0,   0,   0,   0,  10,   0,  12,   0,   0,   0,   0,   0,   0,   0,   0 },
    {  10,   0,   0,   0,   0,   0,   0,   0,  12,   0,   7,   0,   0,   0,   0,   0,   0,   0,   5,   3,   0,   0,   0,   0,   0,   0,   0,  11,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   7,   0,  12,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,  12,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0 },
    {   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   9,   0,  10,   0,   0,   0,   0,  12,  12,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,  10,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,  12,   0,   0,   0,   0,   0,  12,   0,   0 },
    {   0,   0,   0,   0,  10,   0,   0,   5,   0,   0,  12,   0,   0,   0,   0,  12,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0 },
    {   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   9,   0,   3,   0,   0,   0 },
    {   0,   0,   0,   0,  12,   0,   0,   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,  12,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
};

/* Per-trax gate length in centiseconds (lean: era-biased table, no full voice struct). */
static const uint8_t JAZZ_TRAX_LEAD_GATE_CS[JAZZ_TRAX_N] = {
    36u, 37u, 38u, 39u, 40u, 36u, 37u, 38u, 47u, 48u, 32u, 33u, 34u, 35u, 36u, 32u, 33u, 34u, 43u, 44u, 20u, 21u, 22u, 23u, 24u, 20u, 21u, 22u, 27u, 28u, 32u, 33u, 34u, 35u, 36u, 32u, 33u, 34u, 43u, 44u, 12u, 13u, 14u, 15u, 16u, 12u, 13u, 14u, 19u, 20u, 24u, 25u, 26u, 27u, 28u, 24u, 25u, 26u, 31u, 32u
};

static inline uint32_t jazz_trax_lead_gate32(uint8_t trax)
{
    if (jazz_gen_hit(trax))
        return jazz_gen.lead_gate;
    return JAZZ_TRAX_LEAD_GATE[jazz_clamp_trax(trax)];
}

static inline uint8_t jazz_trax_lead_gate_cs(uint8_t trax)
{
    if (jazz_gen_hit(trax))
        return jazz_gen.gate_cs;
    return JAZZ_TRAX_LEAD_GATE_CS[jazz_clamp_trax(trax)];
}

/* 4-bar phrase arrangement (same decode as heavy_psych_trax_lead_step_bit).
 * DUB_STEPS=32 → bar0 = low-16 (call), bar1 = high-16 (response). */
static inline uint8_t jazz_trax_lead_step_bit(uint8_t trax, uint8_t step)
{
    uint32_t g32 = jazz_trax_lead_gate32(trax);
    uint8_t bar4 = (uint8_t)((step >> 4) & 3u);
    uint8_t s16 = (uint8_t)(step & 15u);

    if (bar4 == 0u || bar4 == 2u)
        return (uint8_t)((g32 >> s16) & 1u);
    if (bar4 == 1u)
        return (uint8_t)((g32 >> (16u + s16)) & 1u);
    if (s16 < 8u)
        return 0u;
    return (uint8_t)((g32 >> (16u + s16)) & 1u);
}

/* Scale-legal semitone offset from root+12; 32-wide so bar1 ≠ bar0 pitch clone. */
static inline int8_t jazz_trax_lead_pitch(uint8_t trax, uint8_t step)
{
    if (jazz_gen_hit(trax))
        return jazz_gen_lead.pitch[step & 31u];
    return JAZZ_TRAX_LEAD_PITCH[jazz_clamp_trax(trax)][step & 31u];
}

/* POP-like hold for SPIRIT/FUSION/BROKEN: ring through 1-step skips, cut on breath.
 * Stays inside the current 16-step half so call does not smear into response. */
static inline uint8_t jazz_lead_phrase_span(uint8_t trax, uint8_t step)
{
    uint8_t era = jazz_era_of_trax(trax);
    uint8_t s0 = (uint8_t)(step & 31u);
    uint8_t half_end = (s0 < 16u) ? 16u : 32u;
    uint8_t breath, i, empties = 0u, span = 1u;

    if (era == (uint8_t)ERA_SPIRIT)
        breath = 2u;
    else if (era == (uint8_t)ERA_FUSION)
        breath = 3u;
    else if (era == (uint8_t)ERA_BROKEN)
        breath = 1u;
    else
        return 1u;

    for (i = 1u; (uint8_t)(s0 + i) < half_end && i < 16u; i++) {
        uint8_t ns = (uint8_t)(s0 + i);

        if (jazz_trax_lead_step_bit(trax, ns))
            break;
        empties++;
        if (empties >= breath)
            break;
        span = (uint8_t)(i + 1u);
    }
    return span;
}

static inline uint8_t jazz_trax_lead_len_ix(uint8_t trax, uint8_t step)
{
    if (jazz_gen_hit(trax))
        return jazz_gen_lead.len[step & 31u];
    return JAZZ_TRAX_LEAD_LEN[jazz_clamp_trax(trax)][step & 31u];
}

static inline uint8_t jazz_trax_lead_bend_u7(uint8_t trax, uint8_t step)
{
    if (jazz_gen_hit(trax))
        return jazz_gen_lead.bend[step & 31u];
    return JAZZ_TRAX_LEAD_BEND[jazz_clamp_trax(trax)][step & 31u];
}

static inline void jazz_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 34;
    p->cutoff_c = 88;
    p->decay_c = 90;
    p->cutoff_b = 48;
    p->decay_b = 94;
    p->sub_oct = 0;
    p->drive = 14;
    p->pump = 72;
    p->feedback = 16;
    p->density = 64u;
    p->wobble = 6;
    p->kick_boom = 76;
    p->rim_tone = 56;
    p->hat_lvl = 54;
    p->siren_pitch = 100;
    p->siren_rate = 42;
    p->drum_kit = DKIT_RIDE;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 18u;
        p->drm_dly[di] = 28u;
        p->drm_rev[di] = 24u;
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 64u;
    }
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
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

static inline void jazz_era_apply_base(dub_params_t *p, uint8_t era)
{
    const jazz_era_base_t *eb;
    const jazz_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = jazz_clamp_era(era);
    eb = &JAZZ_ERA_BASE[era];
    fx = &JAZZ_ERA_FX[era];
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
    p->lvl_b = 96;
    p->lvl_c = 92;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

/* Per-preset KICK/RIM/HAT decay trims (drm_dec[0..2]), fitted so every factory voice keeps
 * its envelope length from before the quarter-octave drum decay (±15 %). Siren keeps kit_decay.
 * Column 3: open-hat length trim, 64 + hat_mul steps; read by the DSP via p->plate, never saved. */
static const uint8_t jazz_drm_dec_fit[GENRE_PRESET_N][4] = {
    { 35,  41,  63, 65}, { 32,  43,  96, 61}, { 32,  41,  72, 63}, { 34,  43,  92, 62},
    { 34,  42,  67, 64}, { 35,  41,  67, 64}, { 34,  40,  68, 64}, { 32,  42,  67, 64},
    { 35,  43,  71, 64}, { 34,  43,  68, 64}, { 37,  43,  67, 64}, { 35,  43,  67, 64},
    { 35,  42,  72, 63}, { 34,  45,  92, 63}, { 36,  44,  67, 64}, { 36,  43,  67, 64},
    { 36,  45,  88, 64}, { 34,  45,  68, 63}, { 34,  45,  72, 63}, { 35,  44,  76, 64},
    { 39,  46,  67, 64}, { 39,  47,  72, 63}, { 38,  44,  96, 65}, { 36,  47,  68, 63},
    { 36,  45,  67, 64}, { 38,  47,  67, 64}, { 39,  47,  88, 64}, { 39,  43,  72, 63},
    { 36,  47,  68, 63}, { 38,  47, 100, 61}, { 32,  40,  96, 63}, { 32,  41,  68, 63},
    { 30,  41,  59, 67}, { 32,  40,  67, 64}, { 32,  39,  59, 67}, { 33,  41,  73, 65},
    { 31,  41, 104, 63}, { 33,  40,  71, 65}, { 32,  38,  96, 63}, { 30,  40,  67, 64},
    { 31,  36,  63, 65}, { 29,  39,  67, 64}, { 30,  37,  63, 65}, { 30,  39,  96, 61},
    { 30,  39,  67, 64}, { 28,  39,  71, 64}, { 30,  39,  67, 64}, { 29,  38,  71, 64},
    { 30,  37,  67, 64}, { 31,  39,  72, 63}, { 38,  47,  68, 63}, { 38,  46,  68, 63},
    { 40,  49,  67, 64}, { 39,  46,  67, 64}, { 38,  49,  67, 64}, { 39,  49,  67, 64},
    { 40,  49,  72, 63}, { 38,  47,  67, 64}, { 40,  49,  88, 64}, { 38,  46,  65, 66}
};
#define GENRE_DRM_DEC_FIT jazz_drm_dec_fit

static inline void jazz_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = jazz_drm_dec_fit[preset][i];
}

static inline void jazz_trax_apply(dub_params_t *p, uint8_t trax, uint8_t what)
{
    const jazz_trax_t *tx;
    uint8_t di;

    if (!p)
        return;
    trax = jazz_clamp_trax(trax);
    tx = jazz_trax(trax);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        jazz_apply_defaults(p);
        jazz_era_apply_base(p, tx->era);
        jazz_drm_dec_fit_apply(p, trax);
    }
    p->era = tx->era;
    p->plate = trax;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = tx->bpm;
        p->density = 64u;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = 64u;
        p->drum_kit = jazz_trax_kit(trax);
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
        p->root = jazz_trax_root(trax);
        p->scale = jazz_trax_scale(trax);
        p->swing = jazz_trax_swing(trax);
        {
            int16_t lead = (int16_t)p->siren_rate + (int16_t)jazz_trax_lead_rate(trax);

            if (lead < 0)
                lead = 0;
            else if (lead > 127)
                lead = 127;
            p->siren_rate = (uint8_t)lead;
        }
    }
}

static inline void jazz_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t trax)
{
    if (!p)
        return;
    era = jazz_clamp_era(era);
    trax = jazz_clamp_trax(trax);
    jazz_trax_apply(p, trax, DUB_APPLY_ALL);
    p->era = era;
    p->plate = trax;
    p->drum_kit = jazz_trax_kit(trax);
    p->bass_model = jazz_trax(trax)->bass_model;
    p->voicing = jazz_trax(trax)->voicing;
    p->bpm = jazz_trax(trax)->bpm;
    p->pump = jazz_trax(trax)->pump;
    p->feedback = jazz_trax(trax)->feedback;
    p->wobble = jazz_trax(trax)->wobble;
    p->drum_pat = trax;
    p->root = jazz_trax_root(trax);
    p->scale = jazz_trax_scale(trax);
    p->swing = jazz_trax_swing(trax);
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
    const jazz_trax_t *tx;

    plate = jazz_clamp_trax(plate);
    tx = jazz_trax(plate);
    if (root)
        *root = jazz_trax_root(plate);
    if (scale)
        *scale = jazz_trax_scale(plate);
    if (swing)
        *swing = jazz_trax_swing(plate);
    if (bpm)
        *bpm = tx->bpm;
}

/* drm_dens[] is voice character (PUNCH/SNAP/SIZZLE/TIMBRE), not seq density — never gate here. */
static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const jazz_trax_t *drum;
    const jazz_trax_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = jazz_trax(p->drum_pat);
    main = jazz_trax(p->plate);
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
        siren = jazz_trax_lead_step_bit(p->plate, step);
        sub = dub_step_bit(main->sub, step);
        chord = dub_step_bit(main->chord, step);
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

#include "jazz_gen.h"

#endif
