/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DUB endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of dub_data.h.
 * 16-step masks (dub_step_bit reads step & 15). Ranges follow the eight full-tempo plates per era
 * (tools/pgen_era_stats.py; root/scale/swing from plate_roots/plate_scales/plate_swings); the two slow variants per
 * era stay landmarks only. BRISTOL and POSTDUB tempo windows keep the dominant family (outliers stay landmarks). */
#ifndef DUB_GEN_H
#define DUB_GEN_H

#include "fm_pgen_core.h"

#define DUB_GEN_KEY 0x44554231u

#define DGB(x) FM_PGEN_B(x)
#define DGEN_ONE_DROP 0x0100u     /* kick (and rim) on beat 3 */
#define DGEN_DOWNBEAT 0x0001u
#define DGEN_ODD16 0xAAAAu        /* off-grid 16ths */
#define DGEN_SKANK_BAN 0x22A2u    /* roots chords never on these 16ths (curated) */
#define DGEN_BASS_ANCHOR 0x0101u  /* bass on 1 and 3 */

/* must/ban lanes: kick rim hat sub chord siren lead (unused). */
static const fm_pgen_genome_t DUB_GENOME[DUB_ERA_N] = {
    /* KINGSTON: one-drop, 8th hats, roots bass */
    { 68, 84, 98, 118, 53, 58, 12, 20, 6, 22, 0, 18, 30, 54, 24, 48, 34, 54, 51, 48, 62,
      DGB(5) | DGB(6) | DGB(7) | DGB(12) | DGB(13), DGB(0), DGB(BASS_SUB) | DGB(BASS_808) | DGB(BASS_FM),
      DGB(VOICE_MIN7) | DGB(VOICE_MIN9) | DGB(VOICE_SUS4) | DGB(VOICE_FIFTH),
      { 90, 65, 150, 52, 75, 12, 0 },
      { DGEN_ONE_DROP, 0u, 0u, DGEN_BASS_ANCHOR }, { 0xEAEAu, DGEN_ODD16, DGEN_ODD16, 0u, DGEN_SKANK_BAN }, 0, 0, 0, 0 },
    /* LOVERS: one-drop, kick only on 1 and 3, slow and soft */
    { 62, 76, 70, 84, 55, 60, 6, 16, 20, 34, 2, 10, 54, 80, 48, 74, 58, 72, 39, 50, 62,
      DGB(6) | DGB(7) | DGB(13), DGB(0), DGB(BASS_SUB) | DGB(BASS_808),
      DGB(VOICE_MIN7) | DGB(VOICE_MIN9) | DGB(VOICE_SUS4),
      { 90, 40, 128, 36, 70, 12, 0 },
      { DGEN_ONE_DROP, 0u, 0u, DGEN_BASS_ANCHOR }, { 0xFEFEu, DGEN_ODD16, 0xCCCCu, 0u, DGEN_SKANK_BAN }, 0, 0, 0, 0 },
    /* CHANNEL: steppers, four-on-the-floor kick */
    { 120, 132, 94, 108, 50, 53, 30, 44, 80, 110, 12, 34, 44, 68, 40, 64, 46, 56, 70, 48, 60,
      DGB(0) | DGB(1) | DGB(2) | DGB(6) | DGB(7), DGB(0),
      DGB(BASS_SUB) | DGB(BASS_REESE) | DGB(BASS_303) | DGB(BASS_PWM),
      DGB(VOICE_MIN7) | DGB(VOICE_MIN9) | DGB(VOICE_SUS4) | DGB(VOICE_QUART),
      { 30, 27, 90, 16, 50, 10, 0 },
      { 0x1111u, 0u, 0u, DGEN_BASS_ANCHOR }, { 0xEEEEu }, 0, 0, 0, 0 },
    /* BRISTOL: trip-hop / dub breaks, downbeat kick */
    { 112, 128, 92, 120, 54, 60, 26, 58, 36, 60, 10, 28, 32, 52, 28, 44, 36, 50, 52, 48, 58,
      DGB(0) | DGB(1) | DGB(2) | DGB(3) | DGB(4) | DGB(6) | DGB(7) | DGB(8), DGB(0),
      DGB(BASS_SUB) | DGB(BASS_808) | DGB(BASS_REESE) | DGB(BASS_WG) | DGB(BASS_PWM),
      DGB(VOICE_MIN7) | DGB(VOICE_MIN9) | DGB(VOICE_SUS4) | DGB(VOICE_QUART) | DGB(VOICE_FIFTH),
      { 65, 50, 90, 52, 45, 10, 0 },
      { DGEN_DOWNBEAT, 0u, 0u, 0x1001u /* bass on 1 and 4 */ }, { 0xA2AAu }, 0, 0, 0, 0 },
    /* CROYDON: half-time, kick on 1, snare on 3, space in the bassline */
    { 136, 140, 76, 102, 49, 52, 20, 32, 66, 88, 32, 96, 32, 50, 22, 42, 36, 48, 45, 49, 61,
      DGB(0) | DGB(2) | DGB(3) | DGB(6) | DGB(8), DGB(0),
      DGB(BASS_SUB) | DGB(BASS_808) | DGB(BASS_WOBBLE) | DGB(BASS_303) | DGB(BASS_FM),
      DGB(VOICE_MIN7) | DGB(VOICE_SUS4) | DGB(VOICE_QUART) | DGB(VOICE_FIFTH),
      { 70, 35, 150, 50, 35, 10, 0 },
      { DGEN_DOWNBEAT, DGEN_ONE_DROP, 0u, DGEN_BASS_ANCHOR }, { 0xF1FEu, 0xFAE7u, DGEN_ODD16, 0xE0E0u }, 0, 0, 0, 0 },
    /* POSTDUB: broken, downbeat kick, open grid */
    { 96, 124, 78, 104, 52, 58, 14, 28, 76, 112, 16, 38, 40, 60, 36, 60, 44, 56, 50, 47, 57,
      DGB(1) | DGB(3) | DGB(4) | DGB(7) | DGB(8) | DGB(9) | DGB(13), DGB(0),
      DGB(BASS_SUB) | DGB(BASS_REESE) | DGB(BASS_WG) | DGB(BASS_FM) | DGB(BASS_PWM),
      DGB(VOICE_MIN9) | DGB(VOICE_SUS4) | DGB(VOICE_QUART),
      { 75, 50, 85, 26, 50, 12, 0 },
      { DGEN_DOWNBEAT, 0u, 0u, DGEN_BASS_ANCHOR }, { 0xE01Eu, 0u, 0x8484u }, 0, 0, 0, 0 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t DUB_GEN_LIM = {
    .dens_lo = 56, .dens_hi = 124, .swing_lo = 48, .swing_hi = 66, .pump_lo = 4, .pump_hi = 64,
    .tank_lo = 0, .tank_hi = 118, .wob_lo = 0, .wob_hi = 100, .dly_lo = 24, .dly_hi = 84,
    .rev_lo = 16, .rev_hi = 80, .fb_lo = 30, .fb_hi = 76, .root_lo = 45, .root_hi = 64,
};

/* 196 name words, NUL-separated, empty word ends the list (1235 bytes). */
static const char DUB_GEN_WORDS[] =
    "SMOKE\0RIVER\0STONE\0ROOTS\0ZION\0LION\0MOUNTAIN\0VALLEY\0HILLS\0YARD\0GULLY\0TOWN\0HARBOR\0SEA\0OCEAN\0"
    "TIDE\0WAVE\0SURF\0SPRAY\0DRIFT\0FOG\0MIST\0RAIN\0STORM\0THUNDER\0CLOUD\0SKY\0MOON\0SUN\0STAR\0DUSK\0"
    "DAWN\0NIGHT\0SHADOW\0SHADE\0DARK\0DEEP\0BASS\0SOUND\0SYSTEM\0SPEAKER\0STACK\0TOWER\0WOOFER\0DUBPLATE\0"
    "ACETATE\0VINYL\0RECORD\0PRESS\0LATHE\0TAPE\0REEL\0SPOOL\0ECHO\0DELAY\0REVERB\0SPRING\0PLATE\0CHAMBER\0"
    "CAVE\0CAVERN\0TUNNEL\0WELL\0FADER\0KNOB\0DIAL\0SEND\0RETURN\0MIXER\0DESK\0STUDIO\0ROCKERS\0STEPPERS\0"
    "SKANK\0SKA\0RIDDIM\0DUBWISE\0VERSION\0DRUM\0HEART\0PULSE\0MARCH\0EXODUS\0EXILE\0FREEDOM\0UNITY\0"
    "JUSTICE\0MOVEMENT\0PEOPLE\0PALM\0COCONUT\0MANGO\0GINGER\0PEPPER\0SPICE\0CANE\0SUGAR\0SALT\0BRINE\0"
    "COAST\0BEACH\0SHORE\0REEF\0LAGOON\0BAY\0PORT\0DOCK\0ANCHOR\0SAIL\0VESSEL\0CANOE\0RAFT\0FERRY\0BRIDGE\0"
    "STREAM\0CREEK\0DELTA\0FLOOD\0SPILL\0CURRENT\0DEPTH\0ABYSS\0HOLLOW\0VAULT\0CELLAR\0BUNKER\0DUNGEON\0"
    "SMOG\0HAZE\0PHANTOM\0SPIRIT\0DUPPY\0MYSTIC\0ANCIENT\0ELDER\0WARRIOR\0SHIELD\0SPEAR\0DRUMMER\0BUBBLE\0"
    "WOBBLE\0SWAY\0CHANT\0MELODICA\0ORGAN\0HORN\0TROMBONE\0SAX\0FLUTE\0BELL\0CHIME\0SIREN\0RADIO\0STATIC\0"
    "SIGNAL\0WIRE\0AERIAL\0ANTENNA\0SPLASH\0RIPPLE\0UNDERTOW\0EMBER\0ASH\0CINDER\0COAL\0FIRE\0FLAME\0"
    "LANTERN\0CANDLE\0TORCH\0BEACON\0SUNSET\0SUNRISE\0HORIZON\0DISTANCE\0YONDER\0WANDER\0ROAM\0JOURNEY\0"
    "EXPRESS\0BUS\0TRAIN\0STATION\0CROSSING\0JUNCTION\0BORDER\0DUST\0GRAVEL\0CLAY\0CORAL\0SHELL\0CONCH\0"
    "DRIFTER\0ROVER\0NOMAD\0DREAD\0";

static const fm_pgen_def_t DUB_GEN_DEF = {
    .key = DUB_GEN_KEY,
    .genome = DUB_GENOME,
    .lim = &DUB_GEN_LIM,
    .words = DUB_GEN_WORDS,
    .word_n = 196u,
    .rom = DUB_PLATES,
    .root = (const int8_t *)plate_roots,
    .scale = (const int8_t *)plate_scales,
    .swing = (const int8_t *)plate_swings,
    .era_n = DUB_ERA_N,
    .steps = 16u,
};

#define FM_PGEN_DEF DUB_GEN_DEF

static inline void dub_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    fm_pgen_build(&dub_gen, NULL, &DUB_GEN_DEF, dub_clamp_era(era), n, comply);
}

#endif
