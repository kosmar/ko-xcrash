/* SPDX-License-Identifier: GPL-3.0-only */
/* KO JAZZ endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of jazz_data.h. */
#ifndef JAZZ_GEN_H
#define JAZZ_GEN_H

#include "fm_pgen_core.h"

#define JAZZ_GEN_KEY 0x4A415A31u

#define JGB(x) FM_PGEN_B(x)
#define JGEN_ODD16 0xAAAAAAAAu
/* Step masks as low | high << 16: some 32-bit literals fall in the windows tools/build.py mmio_check guards. */
#define JGEN_M(lo, hi) ((uint32_t)(lo) | ((uint32_t)(hi) << 16))
#define JGEN_BAR1 JGEN_M(0x0001u, 0x0001u)

/* must/ban lanes: kick rim hat sub chord siren lead. The ride (hat) is the era's identity: curated pattern as
 * must, curated never as ban except one ghost slot per beat (SPIRIT/MODAL/FUSION skip note, MUTE offbeat).
 * Feathered kick and hi-hat foot/cross-stick (rim) = curated all, neither plays under the other's figure;
 * swing eras keep the kick off the odd 16ths, rim extras too. Comping (chord) = curated all, never on the bar's
 * one in the swing eras, ACID stabs off the quarters. The walking bass keeps its two most-used curated steps
 * and the lead one, so neither goes silent. */
static const fm_pgen_genome_t JAZZ_GENOME[JAZZ_ERA_N] = {
    /* SPIRIT */
    { 114, 121, 73, 100, 62, 69, 74, 88, 0, 2, 8, 15, 34, 46, 32, 44, 20, 27, 38, 48, 60,
      JGB(0) | JGB(1) | JGB(2) | JGB(4) | JGB(5) | JGB(7) | JGB(11), JGB(0) | JGB(1) | JGB(2) | JGB(6) | JGB(8),
      JGB(BASS_UPRIGHT) | JGB(BASS_WALK) | JGB(BASS_PIZZ),
      JGB(0) | JGB(1) | JGB(2) | JGB(3) | JGB(4) | JGB(5) | JGB(6),
      { 10, 14, 20, 56, 20, 16, 56 },
      { 0x10011001u, 0x04400000u, 0x55555555u, 0x80008000u, 0x12121212u, 0x11111111u, 0x00000004u },
      { JGEN_ODD16 | 0x04400000u, (JGEN_ODD16 & ~0x04400000u) | 0x10011001u, 0x22222222u, 0x05052505u, JGEN_BAR1, 0u, 0u },
      5, 38, -4, 3 },
    /* MODAL */
    { 128, 135, 73, 100, 60, 67, 74, 88, 0, 2, 8, 15, 34, 46, 32, 44, 20, 27, 38, 48, 67,
      JGB(0) | JGB(1) | JGB(2) | JGB(4) | JGB(5) | JGB(6) | JGB(10), JGB(1) | JGB(2) | JGB(3) | JGB(7) | JGB(9),
      JGB(BASS_UPRIGHT) | JGB(BASS_WALK) | JGB(BASS_PIZZ),
      JGB(0) | JGB(1) | JGB(2) | JGB(3) | JGB(4) | JGB(5) | JGB(7),
      { 8, 12, 20, 54, 16, 16, 16 },
      { 0x10001000u, 0x02800080u, 0x33333333u, 0x00080040u, 0x24242424u, 0x55555555u, 0x00000020u },
      { JGEN_ODD16 | 0x02800080u, (JGEN_ODD16 & ~0x02800080u) | 0x10001000u, 0x44444444u, 0x21218181u, JGEN_BAR1, 0u, 0u },
      9, 34, -4, 3 },
    /* MUTE */
    { 116, 123, 73, 100, 64, 71, 74, 88, 0, 2, 8, 15, 34, 46, 32, 44, 20, 27, 38, 48, 58,
      JGB(0) | JGB(1) | JGB(2) | JGB(4) | JGB(5) | JGB(6) | JGB(8), JGB(1) | JGB(2) | JGB(3) | JGB(8) | JGB(9),
      JGB(BASS_UPRIGHT) | JGB(BASS_WALK) | JGB(BASS_PIZZ),
      JGB(0) | JGB(1) | JGB(2) | JGB(3) | JGB(4) | JGB(5) | JGB(8),
      { 10, 12, 24, 23, 12, 16, 24 },
      { JGEN_M(0x1001u, 0x0001u), 0x80808080u, 0x11111111u, 0x00400040u, 0x08080808u, 0x10011001u, 0x00000400u },
      { JGEN_ODD16 | 0x80808080u, (JGEN_ODD16 & ~0x80808080u) | JGEN_M(0x1001u, 0x0001u), 0xAAAAAAAAu, 0xBBBBBBBBu, JGEN_BAR1, 0u, 0u },
      2, 22, -4, 3 },
    /* FUSION */
    { 104, 111, 73, 100, 54, 61, 74, 88, 0, 2, 8, 15, 34, 46, 32, 44, 20, 27, 38, 48, 62,
      JGB(0) | JGB(1) | JGB(2) | JGB(4) | JGB(5) | JGB(7) | JGB(11), JGB(3) | JGB(4) | JGB(5) | JGB(10) | JGB(11),
      JGB(BASS_ORGAN) | JGB(BASS_AMP) | JGB(BASS_FUSION) | JGB(BASS_EP),
      JGB(0) | JGB(1) | JGB(2) | JGB(3) | JGB(4) | JGB(5) | JGB(9),
      { 14, 14, 20, 64, 24, 16, 76 },
      { 0x10011001u, 0x40400000u, 0x66666666u, 0x00001010u, 0x18181818u, 0x55555555u, 0x00000200u },
      { JGEN_ODD16 | 0x40400000u, (JGEN_ODD16 & ~0x40400000u) | 0x10011001u, 0x11111111u, 0x00008387u, JGEN_BAR1, 0u, 0u },
      7, 34, -4, 3 },
    /* ACID */
    { 120, 127, 73, 100, 48, 55, 74, 88, 0, 2, 8, 15, 34, 46, 32, 44, 20, 27, 38, 45, 55,
      JGB(0) | JGB(1) | JGB(2) | JGB(4) | JGB(5) | JGB(8) | JGB(10), JGB(6) | JGB(12),
      JGB(BASS_ORGAN) | JGB(BASS_AMP) | JGB(BASS_FUSION),
      JGB(0) | JGB(1) | JGB(2) | JGB(3) | JGB(4) | JGB(5) | JGB(10),
      { 8, 24, 0, 113, 16, 16, 56 },
      { 0x11111111u, 0x22220000u, 0xFFFFFFFFu, JGEN_BAR1, 0x44444444u, 0xA0A0A0A0u, 0x00000020u },
      { JGEN_ODD16 | 0x22220000u, 0x11111111u, 0u, 0u, 0x11111111u, 0u, 0u },
      1, 14, -4, 3 },
    /* BROKEN */
    { 108, 115, 73, 100, 56, 63, 74, 88, 0, 2, 8, 15, 34, 46, 32, 44, 20, 27, 38, 43, 59,
      JGB(0) | JGB(1) | JGB(2) | JGB(4) | JGB(5) | JGB(6) | JGB(7), JGB(5) | JGB(11),
      JGB(BASS_ORGAN) | JGB(BASS_AMP) | JGB(BASS_FUSION) | JGB(BASS_FM),
      JGB(0) | JGB(1) | JGB(2) | JGB(3) | JGB(4) | JGB(5) | JGB(11),
      { 12, 20, 0, 124, 16, 16, 46 },
      { 0x12481248u, 0x48240800u, 0x5A5AA5A5u, 0x01010101u, 0xC3C33C3Cu, 0x29292929u, 0x00000400u },
      { 0x48240800u, 0x12481248u, 0xA5A55A5Au, 0x00008080u, 0u, 0u, 0u },
      4, 26, -4, 3 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t JAZZ_GEN_LIM = {
    .dens_lo = 50, .dens_hi = 110, .swing_lo = 46, .swing_hi = 78, .pump_lo = 60, .pump_hi = 100,
    .tank_lo = 0, .tank_hi = 8, .wob_lo = 4, .wob_hi = 24, .dly_lo = 24, .dly_hi = 60,
    .rev_lo = 24, .rev_hi = 56, .fb_lo = 12, .fb_hi = 36, .root_lo = 40, .root_hi = 70,
};

/* Lead pitch = semitones above root + 12 (most used curated offsets); snapped to the plate's scale below. */
static const int8_t JGEN_INTERVALS[8] = { 3, 5, 7, 7, 9, 10, 12, 14 };

/* Pitch-class sets of SCALE_SEMIS[9] in fm_app.c (bit = semitone above the root). */
static const uint16_t JGEN_SCALE_PC[9] = { 0x4ADu, 0x5ADu, 0x6ADu, 0x5ABu, 0xAB5u, 0x6B5u, 0x4A9u, 0x4E9u, 0x9ADu };

static inline uint8_t jazz_gen_pitch_legal(int8_t p, uint8_t scale)
{
    return (uint8_t)((JGEN_SCALE_PC[scale % 9u] >> (uint8_t)(((int32_t)p % 12 + 12) % 12)) & 1u);
}

static inline int8_t jazz_gen_pitch_snap(int8_t p, uint8_t scale)
{
    static const int8_t try_d[5] = { 0, -1, 1, -2, 2 };
    uint8_t i;

    for (i = 0u; i < 5u; i++)
        if (jazz_gen_pitch_legal((int8_t)(p + try_d[i]), scale))
            return (int8_t)(p + try_d[i]);
    return 0;
}

/* 176 name words, NUL-separated, empty word ends the list (1204 bytes). */
static const char JAZZ_GEN_WORDS[] =
    "BEBOP\0HARDBOP\0SWING\0COMBO\0TRIO\0QUARTET\0QUINTET\0SESSION\0GIG\0STANDARD\0CHORUS\0BRIDGE\0VAMP\0"
    "RIFF\0LICK\0SCAT\0BRUSHES\0RIDE\0CYMBAL\0UPRIGHT\0WALKING\0STRIDE\0RAGTIME\0HARLEM\0BASIN\0SUPPER\0"
    "LOUNGE\0SALOON\0CELLAR\0SMOKY\0SATIN\0SILK\0MIDNIGHT\0NOCTURNE\0STARDUST\0AUTUMN\0SUMMER\0RAIN\0MOOD\0"
    "MOODY\0INDIGO\0COBALT\0AZURE\0CRIMSON\0SEPIA\0IVORY\0EBONY\0KEYS\0HORN\0BRASS\0REED\0PLUNGER\0MALLET\0"
    "VIBES\0MODES\0DORIAN\0LYDIAN\0AEOLIAN\0PHRYGIAN\0IONIAN\0TRITONE\0ALTERED\0SEVENTH\0NINTH\0CADENCE\0"
    "RESOLVE\0TENSION\0GROOVE\0POCKET\0FUSION\0ELECTRIC\0COSMIC\0ASTRAL\0BLUESY\0SOULFUL\0MELLOW\0TENDER\0"
    "LAMENT\0ELEGY\0SERENADE\0REVERIE\0DREAM\0DREAMY\0SLEEPY\0DAWN\0DUSK\0TWILIGHT\0EVENING\0CANDLE\0"
    "MOONLIT\0STARLIT\0SUNDOWN\0AVENUE\0STREET\0ALLEY\0CORNER\0TAXI\0TRAIN\0STATION\0PLATFORM\0SKYLINE\0"
    "ROOFTOP\0BALCONY\0TERRACE\0CAFE\0BISTRO\0PARLOR\0CLUB\0STAGE\0CURTAIN\0ENCORE\0APPLAUSE\0OVATION\0"
    "ENSEMBLE\0TUTTI\0SOLO\0DUET\0DUO\0TRUMPET\0CORNET\0FLUGEL\0TROMBONE\0TUBA\0SAX\0ALTO\0TENOR\0BARITONE\0"
    "CLARINET\0FLUTE\0PICCOLO\0OBOE\0BASSOON\0PIANO\0ORGAN\0CELESTE\0HARP\0GUITAR\0BANJO\0BASS\0DRUMS\0"
    "SNARE\0HIHAT\0CONGA\0BONGO\0TIMBALE\0SHAKER\0TRIANGLE\0GONG\0CHIME\0BELL\0GOSPEL\0HYMN\0PRAYER\0PSALM\0"
    "CHANT\0RITUAL\0CEREMONY\0JOURNEY\0VOYAGE\0ODYSSEY\0PASSAGE\0CROSSING\0HORIZON\0SKETCH\0SHADOW\0"
    "PORTRAIT\0CANVAS\0BRUSH\0PASTEL\0CHARCOAL\0INK\0QUILL\0VERSE\0SONNET\0HAIKU\0POEM\0";

static const fm_pgen_def_t JAZZ_GEN_DEF = {
    .key = JAZZ_GEN_KEY,
    .genome = JAZZ_GENOME,
    .lim = &JAZZ_GEN_LIM,
    .words = JAZZ_GEN_WORDS,
    .word_n = 176u,
    .rom = JAZZ_TRAX,
    .root = JAZZ_TRAX_ROOT,
    .scale = JAZZ_TRAX_SCALE,
    .swing = JAZZ_TRAX_SWING,
    .rate = JAZZ_TRAX_LEAD_RATE,
    .kit = JAZZ_TRAX_KIT,
    .lead_gate = JAZZ_TRAX_LEAD_GATE,
    .gate_cs = JAZZ_TRAX_LEAD_GATE_CS,
    .lead_pitch = &JAZZ_TRAX_LEAD_PITCH[0][0],
    .lead_len = &JAZZ_TRAX_LEAD_LEN[0][0],
    .lead_bend = &JAZZ_TRAX_LEAD_BEND[0][0],
    .intervals = JGEN_INTERVALS,
    .era_n = JAZZ_ERA_N,
    .steps = 32u,
    .pitch_n = 32u,
    .lead_hi_echo = 1u,
};

#define FM_PGEN_DEF JAZZ_GEN_DEF
#define FM_PGEN_LEAD_STATE jazz_gen_lead

/* Generated phrases (views_live) snap to the generated scale; curated phrases are copied as written. */
static inline void jazz_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    uint8_t s;

    fm_pgen_build(&jazz_gen, &jazz_gen_lead, &JAZZ_GEN_DEF, jazz_clamp_era(era), n, comply);
    if (jazz_gen.views_live)
        for (s = 0u; s < 32u; s++)
            jazz_gen_lead.pitch[s] = jazz_gen_pitch_snap(jazz_gen_lead.pitch[s], (uint8_t)jazz_gen.scale);
}

#endif
