/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef IDM_DATA_H
#define IDM_DATA_H

#include "dub_types.h"
#include "idm_conf.h"
#include "fm_seq.h"

enum { IDM_ERA_N = GENRE_ERA_N, IDM_PIECE_N = GENRE_PRESET_N };
enum { ERA_WARM = 0, ERA_MELO, ERA_GLITCH, ERA_BREAK, ERA_DRILL, ERA_OBLIQ };
enum {
    BASS_ROUND = 0, BASS_THROB, BASS_TEAR, BASS_SUB,
    BASS_CLICK, BASS_SAND, BASS_BEND, BASS_STATIC, BASS_TWANG,
    BASS_N
};
enum {
    DKIT_FOAM = 0, DKIT_SILK, DKIT_HASH, DKIT_VINYL, DKIT_BIT, DKIT_SKEW, DKIT_CHOP, DKIT_TICK, DKIT_N
};
enum {
    VOICE_MAJ7 = 0, VOICE_MIN7, VOICE_DOM9, VOICE_MIN9, VOICE_SUS4, VOICE_69,
    VOICE_QUART, VOICE_ADD9, VOICE_MAJ6, VOICE_DOM13, VOICE_MIN11, VOICE_ADD11,
    VOICE_N
};

typedef dub_plate_t idm_piece_t;

static const char *const IDM_ERA_NAMES[IDM_ERA_N] = {
    "WARM", "MELO", "GLITCH", "BREAK", "DRILL", "OBLIQ"
};

static const char *const DKIT_NAMES[DKIT_N] = {
    "FOAM", "SILK", "HASH", "VINYL", "BIT", "SKEW", "CHOP", "TICK"
};

static const uint8_t DKIT_ERA[DKIT_N] = {
    ERA_WARM, ERA_WARM, ERA_GLITCH, ERA_BREAK, ERA_DRILL, ERA_OBLIQ, ERA_GLITCH, ERA_MELO
};

enum {
    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS
};

static const int8_t idm_hat_note[DKIT_N] = {
    84, 88, 92, 86, 98, 80, 102, 78
};
static const uint8_t idm_hat_fm[DKIT_N] = {
    8, 14, 48, 62, 72, 38, 82, 6
};
static const uint8_t idm_hat_noise[DKIT_N] = {
    18, 28, 88, 96, 102, 64, 118, 14
};
static const uint8_t idm_hat_metal[DKIT_N] = {
    72, 88, 68, 76, 82, 58, 88, 120
};
static const uint8_t idm_hat_decay[DKIT_N] = {
    6, 8, 3, 4, 2, 7, 3, 18
};
static const uint8_t idm_hat_open[DKIT_N] = {
    4, 8, 2, 3, 1, 6, 2, 24
};
static const uint8_t idm_hat_style[DKIT_N] = {
    HAT_STYLE_TICK, HAT_STYLE_TONE, HAT_STYLE_HISS, HAT_STYLE_TR6,
    HAT_STYLE_HISS, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TICK
};
static const uint8_t idm_hat_cut[DKIT_N] = {
    58, 62, 78, 72, 80, 48, 78, 38
};

static const char *const BASS_NAMES[BASS_N] = {
    "ROUND", "THROB", "TEAR", "SUB", "CLICK", "SAND", "BEND", "STATIC", "TWANG"
};

static const uint8_t BASS_ERA[BASS_N] = {
    ERA_WARM, ERA_WARM, ERA_MELO, ERA_MELO,
    ERA_GLITCH, ERA_BREAK, ERA_DRILL, ERA_OBLIQ, ERA_OBLIQ
};

static const char *const VOICE_NAMES[VOICE_N] = {
    "MAJ7", "MIN7", "DOM9", "MIN9", "SUS4", "69",
    "QUART", "ADD9", "MAJ6", "DOM13", "MIN11", "ADD11"
};

static const idm_piece_t IDM_PIECE[IDM_PIECE_N] = {
    /* WARM 0 */ { "VELVET", 0, 0, 0, 0, 8, 90, 74, 34, 32, 20, 38, 10, 4, 0x1001u | ((uint32_t)0x1001u << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xCB21u | ((uint32_t)0x4A3Fu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0xA170u | ((uint32_t)0x3C76u << 16) },
    /* WARM 1 */ { "MURMUR", 0, 0, 1, 1, 9, 93, 76, 34, 32, 20, 38, 11, 5, 0x5A6Eu | ((uint32_t)0x31BCu << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0xFCFDu | ((uint32_t)0xF5FBu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0xA170u | ((uint32_t)0x3C76u << 16) },
    /* WARM 2 */ { "DRIFT", 0, 0, 2, 2, 10, 96, 78, 34, 32, 20, 38, 12, 4, 0x7AD3u | ((uint32_t)0x04FCu << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xFCA7u | ((uint32_t)0xEB17u << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x55D2u | ((uint32_t)0x9C32u << 16) },
    /* WARM 3 */ { "MODES", 0, 0, 3, 3, 11, 99, 80, 34, 32, 20, 38, 13, 5, 0x9F17u | ((uint32_t)0x0976u << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0x9FEBu | ((uint32_t)0xCFBFu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x55D2u | ((uint32_t)0x9C32u << 16) },
    /* WARM 4 */ { "HEARTH", 0, 0, 4, 0, 12, 102, 74, 34, 32, 20, 38, 14, 4, 0x30BBu | ((uint32_t)0xCCE5u << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0x977Bu | ((uint32_t)0x38EDu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0x4943u | ((uint32_t)0x67CCu << 16) },
    /* WARM 5 */ { "AMBER", 0, 0, 5, 1, 13, 105, 76, 34, 32, 20, 38, 15, 5, 0x0311u | ((uint32_t)0x3D2Au << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0x9A91u | ((uint32_t)0x76FFu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0x4943u | ((uint32_t)0x67CCu << 16) },
    /* WARM 6 */ { "FRAMES", 0, 0, 6, 2, 14, 108, 78, 34, 32, 20, 38, 16, 4, 0xC73Du | ((uint32_t)0x1470u << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xDD55u | ((uint32_t)0xEC52u << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* WARM 7 */ { "QUIET", 0, 0, 7, 3, 15, 111, 80, 34, 32, 20, 38, 17, 5, 0x5BE5u | ((uint32_t)0x298Fu << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0x1D57u | ((uint32_t)0xE39Cu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* WARM 6 */ { "GLITCH", 0, 0, 6, 2, 14, 94, 58, 46, 44, 20, 38, 16, 4, 0xC73Du | ((uint32_t)0x1470u << 16), 0xA020u | ((uint32_t)0x3EE5u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xDD55u | ((uint32_t)0xEC52u << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* WARM 7 */ { "PAUSE", 0, 0, 7, 3, 15, 97, 60, 46, 44, 20, 38, 17, 5, 0x5BE5u | ((uint32_t)0x298Fu << 16), 0xA020u | ((uint32_t)0x3EE5u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0x1D57u | ((uint32_t)0xE39Cu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* MELO 0 */ { "LILAC", 1, 0, 1, 0, 14, 92, 78, 34, 32, 20, 38, 12, 4, 0xF1E9u | ((uint32_t)0xD87Eu << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0x9AA9u | ((uint32_t)0x05BDu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0xA170u | ((uint32_t)0x3C76u << 16) },
    /* MELO 1 */ { "PINCH", 1, 0, 2, 1, 15, 95, 80, 34, 32, 20, 38, 13, 5, 0x87C3u | ((uint32_t)0xA6B7u << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0xBBF3u | ((uint32_t)0xD5FFu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0xA170u | ((uint32_t)0x3C76u << 16) },
    /* MELO 2 */ { "FLOAT", 1, 0, 3, 2, 16, 98, 82, 34, 32, 20, 38, 14, 4, 0x080Cu | ((uint32_t)0x720Au << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xBE59u | ((uint32_t)0xF5A7u << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x55D2u | ((uint32_t)0x9C32u << 16) },
    /* MELO 3 */ { "DREAM", 1, 0, 4, 3, 17, 101, 84, 34, 32, 20, 38, 15, 5, 0x70BCu | ((uint32_t)0x137Fu << 16), 0x481Cu | ((uint32_t)0xDCC9u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0xEFDBu | ((uint32_t)0xAA53u << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x55D2u | ((uint32_t)0x9C32u << 16) },
    /* MELO 4 */ { "TAPER", 1, 0, 5, 0, 18, 104, 78, 34, 32, 20, 38, 16, 4, 0x891Au | ((uint32_t)0x041Au << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0x7FFBu | ((uint32_t)0x7FBEu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0x4943u | ((uint32_t)0x67CCu << 16) },
    /* MELO 5 */ { "GLOW", 1, 0, 6, 1, 19, 107, 80, 34, 32, 20, 38, 17, 5, 0x2CDFu | ((uint32_t)0x6F0Du << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0xC3FFu | ((uint32_t)0x953Eu << 16), 0x3A9Du | ((uint32_t)0xE4B5u << 16), 0x4943u | ((uint32_t)0x67CCu << 16) },
    /* MELO 6 */ { "RIBBON", 1, 0, 7, 2, 20, 110, 82, 34, 32, 20, 38, 18, 4, 0x60B1u | ((uint32_t)0x4B1Eu << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xFEEFu | ((uint32_t)0x69AFu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* MELO 7 */ { "NYLON", 1, 0, 8, 3, 21, 113, 84, 34, 32, 20, 38, 19, 5, 0x2A04u | ((uint32_t)0x2568u << 16), 0xB272u | ((uint32_t)0x3EE5u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0x72CFu | ((uint32_t)0x24BBu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* MELO 6 */ { "AMBI", 1, 0, 7, 2, 20, 96, 62, 46, 44, 20, 38, 18, 4, 0x60B1u | ((uint32_t)0x4B1Eu << 16), 0xA020u | ((uint32_t)0x3EE5u << 16), 0xA9C6u | ((uint32_t)0x46D2u << 16), 0xFEEFu | ((uint32_t)0x69AFu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* MELO 7 */ { "WISP", 1, 0, 8, 3, 21, 99, 64, 46, 44, 20, 38, 19, 5, 0x2A04u | ((uint32_t)0x2568u << 16), 0xA020u | ((uint32_t)0x3EE5u << 16), 0x2BBAu | ((uint32_t)0xF928u << 16), 0x72CFu | ((uint32_t)0x24BBu << 16), 0x4B6Du | ((uint32_t)0x00C2u << 16), 0x3DD9u | ((uint32_t)0x4187u << 16) },
    /* GLITCH 0 */ { "RAZOR", 2, 1, 2, 0, 20, 108, 96, 34, 32, 12, 38, 18, 8, 0x23A5u | ((uint32_t)0xDA1Cu << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xFFCBu | ((uint32_t)0xF2BFu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x591Fu | ((uint32_t)0xEA20u << 16) },
    /* GLITCH 1 */ { "FLINCH", 2, 1, 3, 1, 21, 111, 98, 34, 32, 12, 38, 19, 9, 0x9882u | ((uint32_t)0x1A29u << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0x9A67u | ((uint32_t)0xBFADu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x591Fu | ((uint32_t)0xEA20u << 16) },
    /* GLITCH 2 */ { "FREEZE", 2, 1, 4, 2, 22, 115, 100, 34, 32, 12, 38, 20, 8, 0x70F6u | ((uint32_t)0x321Eu << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xFFFFu | ((uint32_t)0xFBCFu << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x4120u | ((uint32_t)0x7439u << 16) },
    /* GLITCH 3 */ { "SPARK", 2, 1, 5, 3, 23, 118, 102, 34, 32, 12, 38, 21, 9, 0xE1FEu | ((uint32_t)0xA0DBu << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0x3DFFu | ((uint32_t)0xBFD7u << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x4120u | ((uint32_t)0x7439u << 16) },
    /* GLITCH 4 */ { "NICK", 2, 1, 6, 0, 24, 122, 96, 34, 32, 12, 38, 22, 8, 0x1D46u | ((uint32_t)0x87D1u << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xBFF3u | ((uint32_t)0xFFF7u << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x7AAEu | ((uint32_t)0x3506u << 16) },
    /* GLITCH 5 */ { "RIFT", 2, 1, 7, 1, 25, 125, 98, 34, 32, 12, 38, 23, 9, 0xED57u | ((uint32_t)0x2EA3u << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xFFFFu | ((uint32_t)0xEF7Bu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x7AAEu | ((uint32_t)0x3506u << 16) },
    /* GLITCH 6 */ { "SHARD", 2, 1, 8, 2, 26, 129, 100, 34, 32, 12, 38, 24, 8, 0x420Au | ((uint32_t)0x2C1Bu << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xFFEFu | ((uint32_t)0xFF7Du << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* GLITCH 7 */ { "SPLICE", 2, 1, 9, 3, 27, 132, 102, 34, 32, 12, 38, 25, 9, 0xFC76u | ((uint32_t)0x45B5u << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xAF3Fu | ((uint32_t)0x1F77u << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* GLITCH 6 */ { "CLICK", 2, 1, 8, 2, 26, 119, 80, 46, 44, 12, 38, 24, 8, 0x420Au | ((uint32_t)0x2C1Bu << 16), 0x0080u | ((uint32_t)0x09DEu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xFFEFu | ((uint32_t)0xFF7Du << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* GLITCH 7 */ { "SPACE", 2, 1, 9, 3, 27, 122, 82, 46, 44, 12, 38, 25, 9, 0xFC76u | ((uint32_t)0x45B5u << 16), 0x0080u | ((uint32_t)0x09DEu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xAF3Fu | ((uint32_t)0x1F77u << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* BREAK 0 */ { "RUNOUT", 3, 1, 3, 0, 26, 114, 100, 34, 32, 12, 38, 22, 10, 0xEE02u | ((uint32_t)0x6D52u << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0x7CE5u | ((uint32_t)0xCDBFu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x591Fu | ((uint32_t)0xEA20u << 16) },
    /* BREAK 1 */ { "SHELL", 3, 1, 4, 1, 27, 119, 102, 34, 32, 12, 38, 23, 11, 0xE83Fu | ((uint32_t)0x2C72u << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0x5FFFu | ((uint32_t)0xBF7Bu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x591Fu | ((uint32_t)0xEA20u << 16) },
    /* BREAK 2 */ { "BRUSH", 3, 1, 5, 2, 28, 124, 104, 34, 32, 12, 38, 24, 10, 0x46B0u | ((uint32_t)0xCBFAu << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xDFFDu | ((uint32_t)0xFBBFu << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x4120u | ((uint32_t)0x7439u << 16) },
    /* BREAK 3 */ { "RIFLE", 3, 1, 6, 3, 29, 129, 106, 34, 32, 12, 38, 25, 11, 0x747Du | ((uint32_t)0xE5E1u << 16), 0x9293u | ((uint32_t)0x4C0Fu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xFFFDu | ((uint32_t)0xF9FFu << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x4120u | ((uint32_t)0x7439u << 16) },
    /* BREAK 4 */ { "ELAST", 3, 1, 7, 0, 30, 134, 100, 34, 32, 12, 38, 26, 10, 0xBA6Bu | ((uint32_t)0xEC52u << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0xFBEFu | ((uint32_t)0x777Fu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x7AAEu | ((uint32_t)0x3506u << 16) },
    /* BREAK 5 */ { "WEDGE", 3, 1, 8, 1, 31, 139, 102, 34, 32, 12, 38, 27, 11, 0x465Du | ((uint32_t)0xF8EBu << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xBFEBu | ((uint32_t)0xFACDu << 16), 0xC96Eu | ((uint32_t)0x1746u << 16), 0x7AAEu | ((uint32_t)0x3506u << 16) },
    /* BREAK 6 */ { "LEAP", 3, 1, 9, 2, 32, 144, 104, 34, 32, 12, 38, 28, 10, 0xFD98u | ((uint32_t)0x6270u << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0x3EF7u | ((uint32_t)0xDBF7u << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* BREAK 7 */ { "CRUMBLE", 3, 1, 10, 3, 33, 149, 106, 34, 32, 12, 38, 29, 11, 0x6603u | ((uint32_t)0x6E26u << 16), 0x4085u | ((uint32_t)0x09DEu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xEFFFu | ((uint32_t)0x7F7Fu << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* BREAK 6 */ { "TEXTUR", 3, 1, 9, 2, 32, 130, 84, 46, 44, 12, 38, 28, 10, 0xFD98u | ((uint32_t)0x6270u << 16), 0x0080u | ((uint32_t)0x09DEu << 16), 0x5936u | ((uint32_t)0xB622u << 16), 0x3EF7u | ((uint32_t)0xDBF7u << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* BREAK 7 */ { "GRAIN", 3, 1, 10, 3, 33, 135, 86, 46, 44, 12, 38, 29, 11, 0x6603u | ((uint32_t)0x6E26u << 16), 0x0080u | ((uint32_t)0x09DEu << 16), 0xDB4Au | ((uint32_t)0x09D8u << 16), 0xEFFFu | ((uint32_t)0x7F7Fu << 16), 0xB89Eu | ((uint32_t)0xF331u << 16), 0x8D15u | ((uint32_t)0x5CA2u << 16) },
    /* DRILL 0 */ { "NEEDLE", 4, 2, 4, 0, 32, 158, 112, 34, 32, 12, 38, 28, 14, 0x72CAu | ((uint32_t)0x81B8u << 16), 0xD1D0u | ((uint32_t)0x96FBu << 16), 0x2E10u | ((uint32_t)0x5BA2u << 16), 0xFFFFu | ((uint32_t)0xFFCEu << 16), 0xEE49u | ((uint32_t)0x3061u << 16), 0x3B7Au | ((uint32_t)0xF1D9u << 16) },
    /* DRILL 1 */ { "FRENZY", 4, 2, 5, 1, 33, 161, 114, 34, 32, 12, 38, 29, 15, 0x6FB0u | ((uint32_t)0x92DFu << 16), 0xD1D0u | ((uint32_t)0x96FBu << 16), 0x20E2u | ((uint32_t)0x4AA7u << 16), 0xF7FFu | ((uint32_t)0xFFFFu << 16), 0xEE49u | ((uint32_t)0x3061u << 16), 0x3B7Au | ((uint32_t)0xF1D9u << 16) },
    /* DRILL 2 */ { "JITTER", 4, 2, 6, 2, 34, 164, 116, 34, 32, 12, 38, 30, 14, 0x2FDFu | ((uint32_t)0x843Eu << 16), 0xD1D0u | ((uint32_t)0x96FBu << 16), 0x2E10u | ((uint32_t)0x5BA2u << 16), 0xFFFFu | ((uint32_t)0xFE7Fu << 16), 0x9FB9u | ((uint32_t)0xD416u << 16), 0xDE59u | ((uint32_t)0xDFABu << 16) },
    /* DRILL 3 */ { "SWIFT", 4, 2, 7, 3, 35, 167, 118, 34, 32, 12, 38, 31, 15, 0xD13Au | ((uint32_t)0x9FC4u << 16), 0xD1D0u | ((uint32_t)0x96FBu << 16), 0x20E2u | ((uint32_t)0x4AA7u << 16), 0xFDFFu | ((uint32_t)0x7BFFu << 16), 0x9FB9u | ((uint32_t)0xD416u << 16), 0xDE59u | ((uint32_t)0xDFABu << 16) },
    /* DRILL 4 */ { "SPECK", 4, 2, 8, 0, 36, 170, 112, 34, 32, 12, 38, 32, 14, 0xF853u | ((uint32_t)0xD408u << 16), 0x7594u | ((uint32_t)0xB30Eu << 16), 0x2E10u | ((uint32_t)0x5BA2u << 16), 0xBDFFu | ((uint32_t)0xF7AFu << 16), 0xEE49u | ((uint32_t)0x3061u << 16), 0x819Au | ((uint32_t)0x799Bu << 16) },
    /* DRILL 5 */ { "LATTIC", 4, 2, 9, 1, 37, 173, 114, 34, 32, 12, 38, 33, 15, 0x0FA4u | ((uint32_t)0xEBE9u << 16), 0x7594u | ((uint32_t)0xB30Eu << 16), 0x20E2u | ((uint32_t)0x4AA7u << 16), 0x7FFFu | ((uint32_t)0xFF9Fu << 16), 0xEE49u | ((uint32_t)0x3061u << 16), 0x819Au | ((uint32_t)0x799Bu << 16) },
    /* DRILL 6 */ { "BURR", 4, 2, 10, 2, 38, 176, 116, 34, 32, 12, 38, 34, 14, 0xF67Du | ((uint32_t)0x7437u << 16), 0x7594u | ((uint32_t)0xB30Eu << 16), 0x2E10u | ((uint32_t)0x5BA2u << 16), 0xDFBFu | ((uint32_t)0xFFDEu << 16), 0x9FB9u | ((uint32_t)0xD416u << 16), 0x3DEFu | ((uint32_t)0x60F4u << 16) },
    /* DRILL 7 */ { "CREST", 4, 2, 11, 3, 39, 176, 118, 34, 32, 12, 38, 35, 15, 0xE889u | ((uint32_t)0x622Eu << 16), 0x7594u | ((uint32_t)0xB30Eu << 16), 0x20E2u | ((uint32_t)0x4AA7u << 16), 0xFFFFu | ((uint32_t)0xFFA3u << 16), 0x9FB9u | ((uint32_t)0xD416u << 16), 0x3DEFu | ((uint32_t)0x60F4u << 16) },
    /* DRILL 6 */ { "BROKEN", 4, 2, 10, 2, 38, 162, 96, 46, 44, 12, 38, 34, 14, 0xF67Du | ((uint32_t)0x7437u << 16), 0x2080u | ((uint32_t)0xB30Eu << 16), 0x2E10u | ((uint32_t)0x5BA2u << 16), 0xDFBFu | ((uint32_t)0xFFDEu << 16), 0x9FB9u | ((uint32_t)0xD416u << 16), 0x3DEFu | ((uint32_t)0x60F4u << 16) },
    /* DRILL 7 */ { "LEVIT", 4, 2, 11, 3, 39, 165, 98, 46, 44, 12, 38, 35, 15, 0xE889u | ((uint32_t)0x622Eu << 16), 0x2080u | ((uint32_t)0xB30Eu << 16), 0x20E2u | ((uint32_t)0x4AA7u << 16), 0xFFFFu | ((uint32_t)0xFFA3u << 16), 0x9FB9u | ((uint32_t)0xD416u << 16), 0x3DEFu | ((uint32_t)0x60F4u << 16) },
    /* OBLIQ 0 */ { "ASKEW", 5, 4, 5, 0, 38, 110, 80, 34, 32, 12, 38, 14, 6, 0x6107u | ((uint32_t)0x2293u << 16), 0xA6A7u | ((uint32_t)0xE18Cu << 16), 0x122Cu | ((uint32_t)0x679Eu << 16), 0xFFFFu | ((uint32_t)0xFFFFu << 16), 0x3C9Bu | ((uint32_t)0xE2B3u << 16), 0xE9A8u | ((uint32_t)0x230Bu << 16) },
    /* OBLIQ 1 */ { "LARGO", 5, 5, 6, 1, 39, 98, 82, 34, 32, 12, 38, 15, 7, 0x1B77u | ((uint32_t)0xC691u << 16), 0xA6A7u | ((uint32_t)0xE18Cu << 16), 0x1CDEu | ((uint32_t)0x769Bu << 16), 0xFE85u | ((uint32_t)0xC31Eu << 16), 0x3C9Bu | ((uint32_t)0xE2B3u << 16), 0xE9A8u | ((uint32_t)0x230Bu << 16) },
    /* OBLIQ 2 */ { "BOLT", 5, 6, 7, 2, 40, 118, 84, 34, 32, 12, 38, 16, 6, 0xEDE8u | ((uint32_t)0x792Bu << 16), 0xA6A7u | ((uint32_t)0xE18Cu << 16), 0x122Cu | ((uint32_t)0x679Eu << 16), 0x777Fu | ((uint32_t)0xFDEFu << 16), 0x4D6Bu | ((uint32_t)0x06C4u << 16), 0x0C8Bu | ((uint32_t)0x0D79u << 16) },
    /* OBLIQ 3 */ { "QUIRK", 5, 7, 8, 3, 41, 122, 86, 34, 32, 12, 38, 17, 7, 0x99F8u | ((uint32_t)0x42F8u << 16), 0xA6A7u | ((uint32_t)0xE18Cu << 16), 0x1CDEu | ((uint32_t)0x769Bu << 16), 0x78F1u | ((uint32_t)0x0118u << 16), 0x4D6Bu | ((uint32_t)0x06C4u << 16), 0x0C8Bu | ((uint32_t)0x0D79u << 16) },
    /* OBLIQ 4 */ { "LEAN", 5, 4, 9, 0, 42, 114, 80, 34, 32, 12, 38, 18, 6, 0xFF74u | ((uint32_t)0x7B06u << 16), 0x02E3u | ((uint32_t)0xC479u << 16), 0x122Cu | ((uint32_t)0x679Eu << 16), 0xFFBFu | ((uint32_t)0xFBFEu << 16), 0x3C9Bu | ((uint32_t)0xE2B3u << 16), 0x5348u | ((uint32_t)0xAB49u << 16) },
    /* OBLIQ 5 */ { "TILT", 5, 5, 10, 1, 43, 130, 82, 34, 32, 12, 38, 19, 7, 0x0DD7u | ((uint32_t)0x29B8u << 16), 0x02E3u | ((uint32_t)0xC479u << 16), 0x1CDEu | ((uint32_t)0x769Bu << 16), 0x62EDu | ((uint32_t)0x9E9Du << 16), 0x3C9Bu | ((uint32_t)0xE2B3u << 16), 0x5348u | ((uint32_t)0xAB49u << 16) },
    /* OBLIQ 6 */ { "CANT", 5, 6, 11, 2, 44, 126, 84, 34, 32, 12, 38, 20, 6, 0x3E0Fu | ((uint32_t)0x29E3u << 16), 0x02E3u | ((uint32_t)0xC479u << 16), 0x122Cu | ((uint32_t)0x679Eu << 16), 0xFFBFu | ((uint32_t)0xA7FFu << 16), 0x4D6Bu | ((uint32_t)0x06C4u << 16), 0xEF3Du | ((uint32_t)0xB226u << 16) },
    /* OBLIQ 7 */ { "BRINK", 5, 7, 0, 3, 45, 134, 86, 34, 32, 12, 38, 21, 7, 0x22C0u | ((uint32_t)0x363Eu << 16), 0x02E3u | ((uint32_t)0xC479u << 16), 0x1CDEu | ((uint32_t)0x769Bu << 16), 0x52CCu | ((uint32_t)0xD4D1u << 16), 0x4D6Bu | ((uint32_t)0x06C4u << 16), 0xEF3Du | ((uint32_t)0xB226u << 16) },
    /* OBLIQ 6 */ { "MINMAL", 5, 6, 11, 2, 44, 106, 64, 46, 44, 12, 38, 20, 6, 0x3E0Fu | ((uint32_t)0x29E3u << 16), 0x00A2u | ((uint32_t)0xC479u << 16), 0x122Cu | ((uint32_t)0x679Eu << 16), 0xFFBFu | ((uint32_t)0xA7FFu << 16), 0x4D6Bu | ((uint32_t)0x06C4u << 16), 0xEF3Du | ((uint32_t)0xB226u << 16) },
    /* OBLIQ 7 */ { "HUSH", 5, 7, 0, 3, 45, 102, 66, 46, 44, 12, 38, 21, 7, 0x22C0u | ((uint32_t)0x363Eu << 16), 0x00A2u | ((uint32_t)0xC479u << 16), 0x1CDEu | ((uint32_t)0x769Bu << 16), 0x52CCu | ((uint32_t)0xD4D1u << 16), 0x4D6Bu | ((uint32_t)0x06C4u << 16), 0xEF3Du | ((uint32_t)0xB226u << 16) },

};

static const int8_t IDM_PIECE_ROOT[IDM_PIECE_N] = {36, 38, 40, 41, 43, 45, 38, 42, 44, 48, 46, 48, 38, 41, 43, 36, 47, 49, 53, 55, 39, 36, 43, 41, 38, 45, 37, 40, 43, 46, 41, 43, 36, 39, 45, 38, 42, 44, 48, 50, 43, 41, 48, 46, 38, 41, 40, 42, 46, 48, 39, 36, 43, 41, 38, 45, 44, 46, 50, 52};

static const int8_t IDM_PIECE_SCALE[IDM_PIECE_N] = {0, 1, 2, 4, 5, 0, 1, 5, 7, 11, 1, 2, 4, 0, 5, 1, 4, 0, 10, 6, 0, 2, 1, 4, 5, 0, 0, 2, 6, 8, 1, 4, 0, 5, 2, 1, 1, 5, 7, 11, 4, 5, 0, 1, 2, 4, 4, 2, 10, 8, 2, 0, 5, 1, 4, 2, 0, 1, 6, 7};

/* 50 = straight; keep near-grid (Aphex/machine), not house shuffle */
static const int8_t IDM_PIECE_SWING[IDM_PIECE_N] = {50, 50, 51, 50, 52, 50, 51, 50, 57, 56, 50, 51, 50, 52, 50, 51, 50, 52, 56, 58, 52, 53, 54, 52, 55, 53, 54, 52, 60, 58, 51, 52, 50, 53, 51, 52, 50, 51, 56, 57, 50, 50, 51, 50, 50, 51, 50, 50, 56, 56, 54, 52, 55, 53, 56, 54, 55, 53, 61, 59};

static const int8_t IDM_PIECE_LEAD_RATE[IDM_PIECE_N] = {-2, -1, 0, 1, 2, 3, -2, -1, 4, 5, -2, -1, 0, 1, 2, 3, -2, -1, 4, 5, 0, 1, 2, 3, -2, -1, 0, 1, 6, 7, 2, 3, -2, -1, 0, 1, 2, 3, 8, 9, -2, -1, 0, 1, 2, 3, -2, -1, 4, 5, 0, 1, 2, 3, -2, -1, 0, 1, 6, 7};

/* [1]=CROP  [2]=REVERS  [5]=HACK  [7]=SCREW  [8]=MOTION  [9]=FLUX
 * CROP/REVERS/MOTION defaults stay low — LFO signature is per-era (not all three). */
static const uint8_t IDM_ERA_MACROS[IDM_ERA_N][10] = {
    { 70,  6,  8, 34, 52, 14, 16,  0, 18, 42 }, /* WARM — FLUX SH macro bed */
    { 68,  6,  8, 40, 50, 16, 18,  0, 22, 48 }, /* MELO — SPACE + FLUX headroom */
    { 54,  8, 18, 42, 38, 20, 30, 14, 30, 52 }, /* GLITCH — REVERS + WOB */
    { 52, 22, 10, 46, 36, 18, 34, 16, 22, 56 }, /* BREAK — CROP window + WEIGHT LFO */
    { 48, 10, 10, 54, 34, 22, 36, 18, 52, 64 }, /* DRILL — MOTION pluck */
    { 50,  8, 12, 38, 48, 16, 26, 12, 76, 80 }, /* OBLIQ — MOTION cross + FLUX smear */
};

/* Seq hit density scale (piece density × num / 100); floor caps era character. */
static const uint8_t IDM_ERA_DENS_NUM[IDM_ERA_N] = { 72u, 86u, 38u, 68u, 112u, 58u };
static const uint8_t IDM_ERA_DENS_FLOOR[IDM_ERA_N] = { 44u, 56u, 22u, 58u, 100u, 40u };
static const int8_t IDM_ERA_BPM_ADJ[IDM_ERA_N] = { 0, 6, 0, -28, 4, -14 };

typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} idm_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} idm_era_fx_t;

static const idm_era_base_t IDM_ERA_BASE[IDM_ERA_N] = {
    /* siren_pitch = LEAD LEVEL; era swing is default when piece swing not loaded */
    { 30, 94, 96, 14, 38, 66, 98, 96, 10, 22, 70, 48, 44, 62, 72, 12,  88, 36, 32, {62,66,48,72},  8, 54, 16, 54 },
    { 34, 92, 94, 18, 42, 64, 96, 94, 12, 26, 72, 52, 50, 64, 66, 18, 108, 40, 36, {64,70,52,76},  6, 52, 18, 52 },
    { 40, 80, 74, 34, 58, 54, 84, 88, 22, 16, 76, 58, 62, 62, 52, 22,  96, 44, 26, {66,78,46,84}, 24, 42, 52, 50 },
    { 42, 76, 70, 38, 62, 50, 80, 90, 24, 14, 82, 62, 58, 64, 50, 24, 100, 48, 30, {64,80,44,88}, 14, 38, 38, 51 },
    { 46, 72, 66, 40, 74, 46, 76, 86, 30, 12, 86, 66, 78, 64, 44, 30, 102, 72, 20, {66,86,42,92}, 26, 34, 44, 50 },
    { 38, 86, 84, 22, 54, 56, 90, 94, 14, 22, 66, 48, 52, 64, 62, 14,  94, 58, 36, {56,70,50,80}, 22, 50, 46, 60 },
};

static const idm_era_fx_t IDM_ERA_FX[IDM_ERA_N] = {
    { DKIT_FOAM,  34, 1, 40, 14, 76, 32, 38, 62, 46,  66, BASS_ROUND,   6 },
    { DKIT_SILK,  38, 2, 38, 12, 80, 34, 36, 58, 50,  70, BASS_THROB,   8 },
    { DKIT_HASH,  16, 0, 50, 16, 58, 38, 22, 42, 42,  52, BASS_TEAR,   32 },
    { DKIT_VINYL, 18, 0, 48, 16, 54, 40, 20, 40, 44,  48, BASS_SAND,   34 },
    { DKIT_BIT,   10, 0, 54, 18, 50, 34, 18, 36, 46,  46, BASS_CLICK,  36 },
    { DKIT_SKEW,  42, 3, 44, 10, 64, 40, 30, 66, 40,  62, BASS_BEND,   14 },
};

/* Bass melodic contour: era × in-era slot × 8-step degree (0..6). */
static const uint8_t IDM_BASS_WALK[IDM_ERA_N][GENRE_PRESETS_PER_ERA][8] = {
    { { 0, 0, 3, 0, 5, 0, 0, 2 }, { 0, 0, 0, 3, 0, 5, 0, 2 }, { 0, 3, 0, 0, 5, 0, 2, 0 }, { 0, 0, 5, 0, 0, 2, 0, 3 }, { 3, 0, 0, 5, 0, 0, 2, 0 }, { 0, 0, 2, 0, 3, 0, 0, 5 }, { 0, 5, 0, 0, 0, 3, 0, 2 }, { 0, 0, 0, 0, 3, 0, 5, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 2, 3, 5, 3, 2, 0, 4 }, { 0, 2, 4, 3, 5, 2, 0, 3 }, { 0, 3, 2, 5, 4, 2, 0, 3 }, { 0, 2, 3, 4, 5, 2, 3, 0 }, { 0, 1, 2, 3, 5, 4, 2, 0 }, { 0, 2, 5, 3, 2, 4, 0, 3 }, { 2, 0, 3, 5, 3, 2, 4, 0 }, { 0, 3, 5, 3, 2, 0, 4, 2 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 5, 1, 4, 0, 6, 2, 5 }, { 0, 6, 2, 5, 1, 4, 0, 5 }, { 5, 0, 1, 6, 4, 0, 2, 5 }, { 0, 4, 6, 1, 5, 0, 2, 5 }, { 1, 5, 0, 4, 6, 2, 0, 5 }, { 0, 5, 4, 1, 0, 2, 6, 5 }, { 6, 0, 5, 2, 4, 1, 0, 5 }, { 0, 1, 5, 6, 0, 4, 2, 5 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 0, 0, 3, 0, 5, 0, 0 }, { 0, 0, 3, 0, 0, 0, 5, 0 }, { 0, 0, 0, 0, 3, 0, 5, 0 }, { 0, 3, 0, 0, 0, 5, 0, 0 }, { 0, 0, 0, 3, 0, 0, 0, 5 }, { 3, 0, 0, 0, 0, 5, 0, 0 }, { 0, 0, 3, 0, 5, 0, 0, 0 }, { 0, 0, 0, 5, 0, 3, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 0, 2, 4, 5, 4, 2, 0, 3 }, { 0, 2, 3, 4, 5, 2, 3, 0 }, { 2, 0, 4, 5, 2, 4, 0, 3 }, { 0, 4, 2, 5, 4, 3, 2, 0 }, { 0, 2, 5, 4, 3, 2, 4, 0 }, { 2, 4, 0, 5, 4, 2, 3, 0 }, { 0, 3, 4, 2, 5, 4, 2, 0 }, { 4, 0, 2, 5, 3, 4, 2, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } },
    { { 5, 0, 3, 0, 2, 0, 6, 0 }, { 0, 5, 0, 3, 0, 2, 0, 6 }, { 5, 0, 0, 3, 0, 6, 0, 2 }, { 0, 3, 0, 5, 2, 0, 0, 6 }, { 3, 0, 5, 0, 0, 2, 6, 0 }, { 5, 0, 2, 0, 3, 0, 0, 6 }, { 0, 0, 5, 3, 0, 6, 2, 0 }, { 5, 3, 0, 0, 2, 6, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 3, 0, 0, 0, 4, 0 } }

};

static inline uint8_t idm_clamp_piece(uint8_t ix)
{
    return ix >= IDM_PIECE_N ? (uint8_t)(IDM_PIECE_N - 1u) : ix;
}

static inline uint8_t idm_clamp_era(uint8_t era)
{
    return era >= IDM_ERA_N ? (uint8_t)(IDM_ERA_N - 1u) : era;
}

#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM piece at idm_gen.slot. */
static fm_pgen_t idm_gen = { .slot = 0xFFu };
static fm_pgen_lead_t idm_gen_lead;

#define FM_PGEN_STATE idm_gen
#define FM_PGEN_BUILD(era, n, c) idm_gen_build(era, n, c)

static inline uint8_t idm_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&idm_gen, ix);
}

static inline uint8_t idm_piece_root(uint8_t ix)
{
    return idm_gen_hit(ix) ? (uint8_t)idm_gen.root : (uint8_t)IDM_PIECE_ROOT[idm_clamp_piece(ix)];
}

static inline uint8_t idm_piece_scale(uint8_t ix)
{
    return idm_gen_hit(ix) ? (uint8_t)idm_gen.scale : (uint8_t)IDM_PIECE_SCALE[idm_clamp_piece(ix)];
}

static inline uint8_t idm_piece_swing(uint8_t ix)
{
    return idm_gen_hit(ix) ? (uint8_t)idm_gen.swing : (uint8_t)IDM_PIECE_SWING[idm_clamp_piece(ix)];
}

static inline int8_t idm_piece_lead_rate(uint8_t ix)
{
    return idm_gen_hit(ix) ? idm_gen.rate : IDM_PIECE_LEAD_RATE[idm_clamp_piece(ix)];
}

static inline uint8_t idm_bass_walk_deg(uint8_t era, uint8_t piece, uint8_t step)
{
    era = idm_clamp_era(era);
    piece = (uint8_t)(idm_clamp_piece(piece) % GENRE_PRESETS_PER_ERA);
    return IDM_BASS_WALK[era][piece][step & 7u];
}

static inline const char *idm_era_name(uint8_t era)
{
    return IDM_ERA_NAMES[idm_clamp_era(era)];
}

static inline const char *idm_piece_name(uint8_t piece)
{
    if (idm_gen_hit(piece))
        return idm_gen.name;
    return IDM_PIECE[idm_clamp_piece(piece)].name;
}

static inline const idm_piece_t *idm_piece(uint8_t ix)
{
    if (idm_gen_hit(ix))
        return &idm_gen.pl;
    return &IDM_PIECE[idm_clamp_piece(ix)];
}

static inline uint8_t idm_era_of_piece(uint8_t piece)
{
    piece = idm_clamp_piece(piece);
    return (uint8_t)(piece / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t idm_era_first_piece(uint8_t era)
{
    return (uint8_t)(idm_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static const int8_t IDM_PIECE_KIT[IDM_PIECE_N] = {0, 0, 1, 0, 2, 1, 0, 2, 6, 8, 1, 1, 2, 3, 1, 2, 3, 1, 9, 7, 2, 2, 3, 2, 1, 2, 3, 2, 9, 8, 3, 4, 3, 4, 3, 4, 5, 4, 11, 10, 6, 6, 6, 6, 6, 6, 6, 6, 12, 12, 5, 5, 5, 5, 5, 5, 5, 5, 11, 11};

static const uint32_t IDM_PIECE_LEAD_GATE[IDM_PIECE_N] = {0x04010011u, 0x00110111u, 0x05401100u, 0x44401000u, 0x09090909u, 0x00511110u, 0x01001100u, 0x40001011u, 0x01001100u, 0x40001011u, 0x00110105u, 0x40110110u, 0x01101100u, 0x41101110u, 0x40011015u, 0x01101011u, 0x41101104u, 0x40111101u, 0x41101104u, 0x40111101u, 0x20010041u, 0x02300050u, 0x01204100u, 0x62025404u, 0x02234441u, 0x20104054u, 0x21004544u, 0x60025044u, 0x21004544u, 0x60025044u, 0x49114911u, 0x44500011u, 0x45400110u, 0x40001010u, 0x04018011u, 0x44141011u, 0x41441110u, 0x40401001u, 0x41441110u, 0x40401001u, 0x228B5015u, 0xAAB80551u, 0x0BAA4155u, 0xE2A85515u, 0xA0A91055u, 0xAAB80514u, 0x8BA25545u, 0xEA2A5555u, 0x8BA25545u, 0xEA2A5555u, 0x4A914A91u, 0x49184918u, 0x4A924A92u, 0x48914891u, 0x49114911u, 0x4A914A91u, 0x49244924u, 0x4C914C91u, 0x49244924u, 0x4C914C91u};

static const int8_t IDM_PIECE_LEAD_PITCH[IDM_PIECE_N][16] = {
    {    5,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
    {    0,    0,    0,    0,   -1,    0,    0,    0,    4,    0,    0,    0,    0,    0,    0,    0 },
    {    0,    0,    0,    0,    0,    0,   -2,    0,   -3,    0,    2,    0,   -3,    0,    0,    0 },
    {    0,    0,    0,    0,    0,    0,   -3,    0,    0,    0,   -1,    0,    0,    0,   -1,    0 },
    {   -2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },
    {   -3,    0,    0,    0,    2,    0,   -2,    0,   -1,    0,    0,    0,   -3,    0,    0,    0 },
    {    0,    0,    0,    0,    0,    0,    0,    0,   -2,    0,    0,    0,   -3,    0,    0,    0 },
    {    3,    0,    0,    0,   -1,    0,    0,    0,    0,    0,    0,    0,   -1,    0,    4,    0 },
    {    0,    0,    0,    0,    0,    0,    0,    0,   -2,    0,    0,    0,   -3,    0,    0,    0 },
    {    3,    0,    0,    0,   -1,    0,    0,    0,    0,    0,    0,    0,   -1,    0,    4,    0 },
    {    5,    0,    3,    0,    5,    0,    0,    0,    3,    0,    0,    0,    0,    0,    0,    0 },
    {    0,    0,    0,    0,   -1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5,    0 },
    {    0,    0,    0,    0,    3,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0 },
    {    0,    0,    0,    0,    2,    0,    0,    0,   -1,    0,    0,    0,   -1,    0,    2,    0 },
    {    2,    0,    4,    0,    5,    0,    0,    0,    0,    0,    0,    0,    4,    0,    4,    0 },
    {    3,    0,    0,    0,   -1,    0,    0,    0,    3,    0,    0,    0,    4,    0,    0,    0 },
    {    0,    0,   -1,    0,   -1,    0,    0,    0,    4,    0,    0,    0,    2,    0,    2,    0 },
    {    0,    0,    0,    0,    2,    0,    0,    0,    3,    0,    0,    0,    3,    0,    0,    0 },
    {    0,    0,   -1,    0,   -1,    0,    0,    0,    4,    0,    0,    0,    2,    0,    2,    0 },
    {    0,    0,    0,    0,    2,    0,    0,    0,    3,    0,    0,    0,    3,    0,    0,    0 },
    {   -2,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    0,    0 },
    {    0,    0,    0,    0,    1,    4,    1,    0,    0,   -1,    0,    0,    0,    0,    0,    0 },
    {    0,    0,    0,    0,    0,    0,    0,    0,   -1,    0,    0,    0,    0,    0,    4,    0 },
    {    0,    0,    0,    0,    0,    0,    0,    0,    0,    3,    2,    0,    1,    2,    2,    0 },
    {   -2,    2,    0,    0,    0,    4,    0,    0,    0,    1,    0,    0,    0,    0,   -2,    0 },
    {    0,    0,    2,    0,   -2,    0,    2,    0,    0,    0,    0,    0,    0,    2,   -1,    0 },
    {    0,    0,    4,    0,    0,    0,    2,    0,    2,    0,   -2,    0,    0,   -1,    3,    0 },
    {    0,    3,    1,    0,    0,    0,    2,    0,    0,    0,    0,    0,   -2,    0,   -1,    0 },
    {    0,    0,    4,    0,    0,    0,    2,    0,    2,    0,   -2,    0,    0,   -1,    3,    0 },
    {    0,    3,    1,    0,    0,    0,    2,    0,    0,    0,    0,    0,   -2,    0,   -1,    0 },
    {    3,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   -2,    0,    0,    0 },
    {   -3,    0,    0,    0,    2,    0,    5,    0,    0,    0,    5,    0,    0,    0,    0,    0 },
    {    0,    0,    0,    0,   -3,    0,   -2,    0,    2,    0,   -2,    0,    0,    0,    3,    0 },
    {    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    5,    0,    0,    0 },
    {   -3,    0,    0,    0,   -3,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,   -2 },
    {   -2,    0,    0,    0,   -2,    0,    0,    0,    0,    0,   -2,    0,    2,    0,    2,    0 },
    {    0,    0,    5,    0,    5,    0,    5,    0,    0,    0,    0,    0,    0,    0,    3,    0 },
    {    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    2,    0,   -2,    0 },
    {    0,    0,    5,    0,    5,    0,    5,    0,    0,    0,    0,    0,    0,    0,    3,    0 },
    {    0,    0,    0,    0,    0,    0,    5,    0,    0,    0,    0,    0,    2,    0,   -2,    0 },
    {    3,    2,    3,   -2,    3,    0,    0,   -1,    0,    3,    0,    0,    0,   -1,   -1,    0 },
    {   -2,    0,    0,    3,    3,   -2,    2,    2,    0,   -2,    3,   -1,    0,    0,    0,    2 },
    {    4,    0,   -2,    3,    3,    0,    0,    1,    0,    2,    0,    1,    0,    0,    3,    0 },
    {    1,    0,   -2,   -1,   -2,    0,    0,    3,   -1,    2,    3,    0,   -2,    0,    1,   -2 },
    {    4,    0,    1,    1,   -1,    3,    2,    3,    0,    0,    0,    0,   -1,   -2,    0,    1 },
    {    0,    0,    3,    2,    2,    0,    0,    0,    2,    2,    0,    1,    0,    2,    0,    0 },
    {    2,    1,    3,    0,    0,   -1,    2,    1,   -1,    3,    4,    3,    2,    0,    3,   -1 },
    {   -1,   -1,   -1,    4,   -1,   -2,    3,    0,    2,    0,   -1,   -1,    4,    3,    0,   -2 },
    {    2,    1,    3,    0,    0,   -1,    2,    1,   -1,    3,    4,    3,    2,    0,    3,   -1 },
    {   -1,   -1,   -1,    4,   -1,   -2,    3,    0,    2,    0,   -1,   -1,    4,    3,    0,   -2 },
    {    5,    0,    0,    5,    0,    0,    0,    2,    0,    0,    0,    4,    0,    0,    0,    5 },
    {    4,    0,    0,    0,    5,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    5 },
    {    0,    0,    0,   -3,    0,    0,    0,    2,    0,    0,    0,   -3,    0,    0,    0,    2 },
    {    5,    0,    0,    0,    0,    0,    0,    4,    0,    0,    0,    5,    4,    0,    5,    4 },
    {   -3,    0,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    0,    5 },
    {   -5,    0,    0,    0,    2,    0,    0,    0,    0,    0,    0,    2,    0,    0,    0,    0 },
    {    5,    0,    0,    0,    0,    0,    0,    2,   -3,    0,    0,   -3,    0,    0,    0,    0 },
    {    5,    0,    0,    5,    0,    0,    0,    5,    0,    0,    0,    5,    0,    0,    5,    0 },
    {    5,    0,    0,    0,    0,    0,    0,    2,   -3,    0,    0,   -3,    0,    0,    0,    0 },
    {    5,    0,    0,    5,    0,    0,    0,    5,    0,    0,    0,    5,    0,    0,    5,    0 },

};

static const uint8_t IDM_PIECE_LEAD_LEN[IDM_PIECE_N][32] = {
    {   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0 },
    {  10,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   9,   0,   1,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   9,   0 },
    {   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   9,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {  10,   0,   1,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   9,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   9,   0 },
    {  10,   0,  10,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   1,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   9,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {   0,   0,   1,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   9,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0 },
    {   0,   0,   0,   0,  10,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,  10,   0,  10,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,  10,  10,   0 },
    {   1,   0,   0,   0,   0,   0,  10,   0,   0,   0,  10,   0,   0,   0,  10,   0,   9,  10,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   9,   0,   9,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0 },
    {   0,   0,  10,   0,   0,   0,  10,   0,   9,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   9,   0,   0 },
    {   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,  10,   0,   9,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  10,   0 },
    {   0,   0,  10,   0,   0,   0,  10,   0,   9,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   9,   0,   0 },
    {   0,   0,  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,  10,   0,   9,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  10,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,  10,   0,   0,   0,   9,   0,   0,   0,   9,   0 },
    {   0,   0,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   9,   0,   9,   0,   0,   0,   9,   0 },
    {   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0 },
    {  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   1,   0,   0,   0,   0,   0,   9,   0,   0,   0,   9,   0 },
    {   0,   0,   0,   0,   9,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   9,   0 },
    {   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {   0,   0,   0,   0,   9,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   9,   0 },
    {   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0 },
    {  10,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   9,   0,  10,   9,   0,  10,   0,   0,   0,   9,   0,   9,   0,   0,   0,   9,   0,   0 },
    {   9,   0,   0,   0,  10,   0,  10,   0,   9,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,  10,   9,   9,   0,   9,   0,  10,   0,  10,   0,  10,   0,   9 },
    {   1,   0,   9,   0,   9,   0,  10,   0,   1,   0,   0,   0,   0,   0,   9,   0,   0,   9,   0,   9,   0,  10,   0,  10,  10,  10,   0,   0,   0,   0,   0,   0 },
    {   9,   0,  10,   0,  10,   0,   0,   0,   9,   0,   9,   0,   9,   0,   1,   0,   0,   0,   0,  10,   0,   9,   0,  10,   0,  10,   0,   0,   0,  10,   9,   9 },
    {   9,   0,  10,   0,   9,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   9,   0,   0,  10,   0,  10,   0,   1,   0,   0,   0,   0,   0,   9,   0,   9 },
    {   0,   0,  10,   0,  10,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  10,  10,   0,  10,   0,  10,   0,  10,   0,  10,   0,   9 },
    {  10,   0,  10,   0,   0,   0,   9,   0,   9,   0,  10,   0,  10,   0,  10,   0,   0,  10,   0,   0,   0,  10,   0,  10,   9,  10,   0,   9,   0,   0,   0,   9 },
    {  10,   0,  10,   0,   9,   0,  10,   0,   9,   0,   9,   0,   9,   0,  10,   0,   0,   9,   0,   1,   0,  10,   0,   0,   0,  10,   0,   9,   0,   9,  10,  10 },
    {  10,   0,  10,   0,   0,   0,   9,   0,   9,   0,  10,   0,  10,   0,  10,   0,   0,  10,   0,   0,   0,  10,   0,  10,   9,  10,   0,   9,   0,   0,   0,   9 },
    {  10,   0,  10,   0,   9,   0,  10,   0,   9,   0,   9,   0,   9,   0,  10,   0,   0,   9,   0,   1,   0,  10,   0,   0,   0,  10,   0,   9,   0,   9,  10,  10 },
    {   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,  10,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0 },
    {  10,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   9,   1,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,  10,   1,   0,   0,   0,   0,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   9,   0,   0,   1,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   9,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,  10,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   1,   0,   0,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0 },
    {   1,   0,   0,   0,   0,   0,   0,   9,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   1,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   9,   9 },
    {   1,   0,   0,   0,   0,   0,   0,   9,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   1,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   9,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   9,   9 },

};

static const uint8_t IDM_PIECE_LEAD_BEND[IDM_PIECE_N][32] = {
    {   3,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   5,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   5,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   4,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,   0,   0,   0,   0,   0,   0,   0,   0,   0,   6,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   4,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0 },
    {   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0 },
    {   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   3,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   4,   0,   4,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   5,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   5,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   5,   0,   0,   0,   3,   0 },
    {   0,   0,   0,   0,   5,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   4,   0,   5,   0,   0,   0,   6,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   4,   0 },
    {   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   3,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   3,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   4,   0,   0,   0,   0,   0,   4,   0,   4,   0,   0,   0,   4,   0,   0 },
    {   5,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   4,   0,   4,   0,   0,   0,   0,   0,   0,   0,   4 },
    {   6,   0,   5,   0,   3,   0,   0,   0,   3,   0,   0,   0,   0,   0,   3,   0,   0,   3,   0,   3,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0 },
    {   3,   0,   0,   0,   0,   0,   0,   0,   4,   0,   5,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   3,   3 },
    {   3,   0,   0,   0,   3,   0,   3,   0,   0,   0,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   3,   0,   3 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   3 },
    {   0,   0,   0,   0,   0,   0,   3,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   4,   0,   0,   0,   3 },
    {   0,   0,   0,   0,   4,   0,   0,   0,   3,   0,   5,   0,   4,   0,   0,   0,   0,   5,   0,   6,   0,   0,   0,   0,   0,   0,   0,   4,   0,   3,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   3,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   4,   0,   0,   0,   3 },
    {   0,   0,   0,   0,   4,   0,   0,   0,   3,   0,   5,   0,   4,   0,   0,   0,   0,   5,   0,   6,   0,   0,   0,   0,   0,   0,   0,   4,   0,   3,   0,   0 },
    {   3,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   5,   5,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   3,   0,   0,   0,   0,   3,   0,   0,   5,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   5,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,   0 },
    {   3,   0,   0,   0,   0,   0,   0,   5,   3,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   5 },
    {   3,   0,   0,   0,   0,   0,   0,   5,   3,   0,   0,   0,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   8,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   5,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,   5 },

};

static const uint8_t IDM_LEAD_LEN_16THS[15] = {
    1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64
};

static const uint8_t IDM_PIECE_LEAD_GATE_CS[IDM_PIECE_N] = {30u, 32u, 28u, 34u, 30u, 26u, 32u, 28u, 38u, 34u, 28u, 30u, 26u, 32u, 28u, 24u, 30u, 26u, 36u, 32u, 22u, 24u, 20u, 26u, 22u, 18u, 24u, 20u, 30u, 26u, 20u, 22u, 18u, 24u, 20u, 16u, 22u, 18u, 28u, 24u, 14u, 16u, 12u, 18u, 14u, 10u, 16u, 12u, 22u, 18u, 18u, 20u, 16u, 22u, 18u, 14u, 20u, 16u, 26u, 22u};

static inline uint8_t idm_piece_kit(uint8_t ix)
{
    return idm_gen_hit(ix) ? (uint8_t)idm_gen.kit : (uint8_t)IDM_PIECE_KIT[idm_clamp_piece(ix)];
}

static inline uint32_t idm_piece_lead_gate32(uint8_t piece)
{
    if (idm_gen_hit(piece))
        return idm_gen.lead_gate;
    return IDM_PIECE_LEAD_GATE[idm_clamp_piece(piece)];
}

static inline uint8_t idm_piece_lead_gate_cs(uint8_t piece)
{
    if (idm_gen_hit(piece))
        return idm_gen.gate_cs;
    return IDM_PIECE_LEAD_GATE_CS[idm_clamp_piece(piece)];
}

static inline uint8_t idm_piece_lead_step_bit(uint8_t piece, uint8_t step)
{
    uint32_t g32 = idm_piece_lead_gate32(piece);
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

static inline int8_t idm_piece_lead_pitch(uint8_t piece, uint8_t step)
{
    if (idm_gen_hit(piece))
        return idm_gen_lead.pitch[step & 15u];
    return IDM_PIECE_LEAD_PITCH[idm_clamp_piece(piece)][step & 15u];
}

static inline uint8_t idm_piece_lead_len_ix(uint8_t piece, uint8_t step)
{
    if (idm_gen_hit(piece))
        return idm_gen_lead.len[step & 31u];
    return IDM_PIECE_LEAD_LEN[idm_clamp_piece(piece)][step & 31u];
}

static inline uint8_t idm_piece_lead_bend_u7(uint8_t piece, uint8_t step)
{
    if (idm_gen_hit(piece))
        return idm_gen_lead.bend[step & 31u];
    return IDM_PIECE_LEAD_BEND[idm_clamp_piece(piece)][step & 31u];
}

static inline void idm_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 34;
    p->cutoff_c = 88;
    p->decay_c = 90;
    p->cutoff_b = 60;
    p->decay_b = 94;
    p->sub_oct = 92;
    p->drive = 14;
    p->pump = 68;
    p->feedback = 18;
    p->density = 64u;
    p->wobble = 10;
    p->kick_boom = 76;
    p->rim_tone = 56;
    p->hat_lvl = 58;
    p->siren_pitch = 100;
    p->siren_rate = 44;
    p->drum_kit = DKIT_FOAM;
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

static inline void idm_era_apply_base(dub_params_t *p, uint8_t era)
{
    const idm_era_base_t *eb;
    const idm_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = idm_clamp_era(era);
    eb = &IDM_ERA_BASE[era];
    fx = &IDM_ERA_FX[era];
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
    p->density = IDM_ERA_DENS_FLOOR[era];
    p->lvl_b = 96;
    p->lvl_c = 92;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

/* Per-preset KICK/RIM/HAT decay trims (drm_dec[0..2]), fitted so every factory voice keeps
 * its envelope length from before the quarter-octave drum decay (±15 %). Siren keeps kit_decay.
 * Column 3: open-hat length trim, 64 + hat_mul steps; read by the DSP via p->plate, never saved. */
static const uint8_t idm_drm_dec_fit[GENRE_PRESET_N][4] = {
    { 39,  48,  67, 64}, { 40,  49,  68, 63}, { 39,  51,  72, 62}, { 38,  51,  67, 64},
    { 40,  49,  65, 64}, { 41,  51,  67, 63}, { 40,  49,  68, 63}, { 40,  51,  59, 65},
    { 38,  48,  40, 63}, { 41,  50,  72, 63}, { 36,  47,  72, 62}, { 39,  47,  67, 65},
    { 39,  44,  67, 64}, { 39,  47,  64, 64}, { 38,  45,  75, 64}, { 37,  47,  63, 65},
    { 37,  44,  60, 62}, { 37,  47,  71, 65}, { 38,  47,  72, 62}, { 37,  47,  76, 63},
    { 35,  38,  92, 64}, { 35,  41,  96, 63}, { 35,  38,  92, 62}, { 34,  34,  92, 64},
    { 34,  41,  67, 63}, { 34,  40,  88, 64}, { 34,  38,  59, 65}, { 35,  41,  96, 63},
    { 33,  39,  68, 63}, { 35,  38,  72, 63}, { 28,  36,  64, 64}, { 28,  37,  68, 63},
    { 28,  37,  59, 65}, { 27,  37,  67, 64}, { 29,  37,  59, 65}, { 28,  36,  67, 64},
    { 29,  35,  68, 63}, { 29,  35,  67, 64}, { 29,  37,  64, 64}, { 28,  35,  88, 64},
    { 24,  30,  67, 64}, { 25,  31,  68, 63}, { 23,  33,  72, 63}, { 24,  32,  68, 63},
    { 23,  31,  67, 64}, { 22,  33,  67, 64}, { 23,  33,  72, 63}, { 25,  32,  68, 63},
    { 23,  32,  67, 64}, { 24,  33,  67, 64}, { 45,  49,  68, 63}, { 44,  51,  68, 63},
    { 43,  49,  67, 64}, { 45,  51,  67, 64}, { 45,  48,  67, 64}, { 44,  48,  67, 64},
    { 44,  51,  72, 63}, { 42,  45,  67, 64}, { 45,  50,  91, 62}, { 43,  51,  92, 62}
};
#define GENRE_DRM_DEC_FIT idm_drm_dec_fit

static inline void idm_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = idm_drm_dec_fit[preset][i];
}

static inline void idm_piece_apply(dub_params_t *p, uint8_t piece, uint8_t what)
{
    const idm_piece_t *pc;
    uint8_t di;

    if (!p)
        return;
    piece = idm_clamp_piece(piece);
    pc = idm_piece(piece);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        idm_apply_defaults(p);
        idm_era_apply_base(p, pc->era);
        idm_drm_dec_fit_apply(p, piece);
    }
    p->era = pc->era;
    p->plate = piece;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        {
            uint8_t er = idm_clamp_era(pc->era);
            uint16_t dens = ((uint16_t)pc->density * (uint16_t)IDM_ERA_DENS_NUM[er]) / 100u;
            int16_t bpm = (int16_t)pc->bpm + (int16_t)IDM_ERA_BPM_ADJ[er];

            if (dens < (uint16_t)IDM_ERA_DENS_FLOOR[er])
                dens = (uint16_t)IDM_ERA_DENS_FLOOR[er];
            if (dens > 127u)
                dens = 127u;
            p->density = (uint8_t)dens;
            if (bpm < 40)
                bpm = 40;
            else if (bpm > 200)
                bpm = 200;
            p->bpm = (uint8_t)bpm;
        }
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = 64u;
        p->drum_kit = idm_piece_kit(piece);
        p->bass_model = pc->bass_model;
        p->voicing = pc->voicing;
        p->pump = pc->pump;
        p->wobble = pc->wobble;
        p->feedback = pc->feedback;
        p->tank = pc->tank;
        p->tap = pc->tap;
        p->dly_c = pc->dly_c;
        p->rev_c = pc->rev_c;
        p->loop_filt = pc->loop_filt;
        p->root = idm_piece_root(piece);
        p->scale = idm_piece_scale(piece);
        p->swing = idm_piece_swing(piece);
        {
            int16_t lead = (int16_t)p->siren_rate + (int16_t)idm_piece_lead_rate(piece);

            if (lead < 0)
                lead = 0;
            else if (lead > 127)
                lead = 127;
            p->siren_rate = (uint8_t)lead;
        }
    }
}

static inline void idm_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t piece)
{
    if (!p)
        return;
    era = idm_clamp_era(era);
    piece = idm_clamp_piece(piece);
    idm_piece_apply(p, piece, DUB_APPLY_ALL);
    p->era = era;
    p->plate = piece;
    p->drum_kit = idm_piece_kit(piece);
    p->bass_model = idm_piece(piece)->bass_model;
    p->voicing = idm_piece(piece)->voicing;
    {
        int16_t bpm = (int16_t)idm_piece(piece)->bpm + (int16_t)IDM_ERA_BPM_ADJ[era];

        if (bpm < 40)
            bpm = 40;
        else if (bpm > 200)
            bpm = 200;
        p->bpm = (uint8_t)bpm;
    }
    p->pump = idm_piece(piece)->pump;
    p->feedback = idm_piece(piece)->feedback;
    p->wobble = idm_piece(piece)->wobble;
    p->drum_pat = piece;
    p->root = idm_piece_root(piece);
    p->scale = idm_piece_scale(piece);
    p->swing = idm_piece_swing(piece);
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
    const idm_piece_t *pc;

    plate = idm_clamp_piece(plate);
    pc = idm_piece(plate);
    if (root)
        *root = idm_piece_root(plate);
    if (scale)
        *scale = idm_piece_scale(plate);
    if (swing)
        *swing = idm_piece_swing(plate);
    if (bpm) {
        int16_t b = (int16_t)pc->bpm + (int16_t)IDM_ERA_BPM_ADJ[idm_clamp_era(pc->era)];

        *bpm = (uint8_t)(b < 40 ? 40 : b > 200 ? 200 : b);
    }
}

static inline uint8_t idm_drm_density_allows(uint8_t density, uint8_t step, uint8_t voice_ix)
{
    return ((step * 19u + (voice_ix * 37u)) & 127u) < density;
}

static inline uint8_t idm_gate_after_drm_density(uint8_t on, uint8_t density, uint8_t step, uint8_t voice_ix)
{
    if (!on)
        return 0u;
    return idm_drm_density_allows(density, step, voice_ix);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const idm_piece_t *drum;
    const idm_piece_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = idm_piece(p->drum_pat);
    main = idm_piece(p->plate);
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
        siren = idm_piece_lead_step_bit(p->plate, step);
        sub = dub_step_bit(main->sub, step);
        chord = dub_step_bit(main->chord, step);
    }
    if (apply_dens) {
        kick = idm_gate_after_drm_density(kick, p->density, step, 0u);
        rim = idm_gate_after_drm_density(rim, p->density, step, 1u);
        hat = idm_gate_after_drm_density(hat, p->density, step, 2u);
        siren = idm_gate_after_drm_density(siren, p->density, step, 3u);
        sub = idm_gate_after_drm_density(sub, p->density, step, 0u);
        chord = idm_gate_after_drm_density(chord, p->density, step, 1u);
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

#include "idm_gen.h"

#endif
