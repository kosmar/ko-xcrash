/* SPDX-License-Identifier: GPL-3.0-only */
/* KO HOUSE endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of house_data.h.
 * 16-step masks (dub_step_bit reads step & 15). Ranges follow the eight full-tempo trax per era (tools/pgen_era_stats.py);
 * the two slow variants per era stay landmarks only. */
#ifndef HOUSE_GEN_H
#define HOUSE_GEN_H

#include "fm_pgen_core.h"

#define HOUSE_GEN_KEY 0x484F5531u

#define HGB(x) FM_PGEN_B(x)
#define HGEN_QUARTERS 0x1111u   /* four-on-the-floor */
#define HGEN_OFFBEATS 0x4444u   /* open hat on the "and" */
#define HGEN_BACKBEAT 0x1010u   /* clap on 2 and 4 */
#define HGEN_SIXTEENTHS_A 0x8888u
#define HGEN_KICK_BAN 0x0EEEu   /* extra kicks only as last-beat pickups */
#define HGEN_CLAP_BAN 0x0101u   /* no clap on beats 1 and 3 */
#define HGEN_BASS_ANCHOR 0x0404u /* offbeat bass on the "and" of 1 and 3 */
#define HGEN_SCALES (HGB(0) | HGB(1) | HGB(2) | HGB(4) | HGB(5))

/* must/ban lanes: kick rim(clap) hat sub chord siren(stab) lead (unused). */
static const fm_pgen_genome_t HOUSE_GENOME[HOUSE_ERA_N] = {
    /* JACK: Chicago jack, 4/4 + offbeat hat, organ / sub bass */
    { 118, 126, 100, 120, 57, 63, 92, 100, 16, 24, 2, 8, 24, 32, 16, 24, 14, 22, 36, 48, 57,
      HGEN_SCALES, HGB(0) | HGB(2), HGB(BASS_SUB) | HGB(BASS_ORGAN),
      HGB(VOICE_MAJ7) | HGB(VOICE_MIN7) | HGB(VOICE_6TH),
      { 40, 24, 40, 80, 40, 16, 0 },
      { HGEN_QUARTERS, HGEN_BACKBEAT, HGEN_OFFBEATS, HGEN_BASS_ANCHOR }, { HGEN_KICK_BAN, HGEN_CLAP_BAN }, 0, 0, -5, 5 },
    /* GARAGE: downbeat kick, 2-step allowed, clap backbeat, shuffled hats */
    { 127, 134, 104, 112, 62, 69, 82, 90, 38, 48, 10, 15, 32, 40, 26, 34, 16, 24, 44, 48, 61,
      HGEN_SCALES, HGB(4) | HGB(6), HGB(BASS_303) | HGB(BASS_REESE) | HGB(BASS_FM),
      HGB(VOICE_MAJ7) | HGB(VOICE_MIN7) | HGB(VOICE_SUS4),
      { 30, 24, 100, 69, 48, 18, 0 },
      { 0x0001u, HGEN_BACKBEAT, HGEN_OFFBEATS, HGEN_BASS_ANCHOR }, { HGEN_BACKBEAT, HGEN_CLAP_BAN }, 0, 0, -6, 5 },
    /* ACID: 4/4, 16th hats, busy 303 line */
    { 126, 132, 112, 120, 54, 58, 86, 96, 6, 14, 65, 92, 20, 26, 12, 18, 26, 34, 53, 48, 57,
      HGEN_SCALES, HGB(3), HGB(BASS_303) | HGB(BASS_PLUCK),
      HGB(VOICE_MAJ7) | HGB(VOICE_MIN7) | HGB(VOICE_9TH),
      { 40, 24, 150, 178, 21, 18, 0 },
      { HGEN_QUARTERS, HGEN_BACKBEAT, HGEN_SIXTEENTHS_A, HGEN_BASS_ANCHOR }, { HGEN_KICK_BAN, HGEN_CLAP_BAN }, 0, 0, -5, 5 },
    /* DEEP: 4/4, sparse offbeat hat, warm chords */
    { 118, 124, 94, 102, 58, 63, 76, 84, 24, 32, 5, 11, 32, 40, 28, 36, 10, 18, 28, 48, 57,
      HGEN_SCALES, HGB(1), HGB(BASS_SUB) | HGB(BASS_MOOG) | HGB(BASS_FM),
      HGB(VOICE_MAJ7) | HGB(VOICE_MIN7) | HGB(VOICE_6TH),
      { 40, 24, 30, 64, 32, 16, 0 },
      { HGEN_QUARTERS, HGEN_BACKBEAT, HGEN_OFFBEATS, HGEN_BASS_ANCHOR }, { HGEN_KICK_BAN, HGEN_CLAP_BAN }, 0, 0, -4, 5 },
    /* FILTER: French touch, 4/4, disco offbeat bass */
    { 118, 124, 104, 111, 56, 59, 106, 113, 88, 98, 4, 10, 40, 47, 36, 43, 20, 26, 50, 50, 60,
      HGEN_SCALES, HGB(5), HGB(BASS_PWM),
      HGB(VOICE_MAJ7) | HGB(VOICE_9TH) | HGB(VOICE_6TH),
      { 40, 24, 60, 70, 56, 18, 0 },
      { HGEN_QUARTERS, HGEN_BACKBEAT, HGEN_OFFBEATS, HGEN_OFFBEATS }, { HGEN_KICK_BAN, HGEN_CLAP_BAN }, 0, 0, -7, 3 },
    /* TECH: 4/4, bass off the kick */
    { 125, 131, 87, 94, 54, 58, 69, 76, 48, 58, 14, 22, 28, 34, 24, 30, 12, 18, 35, 48, 58,
      HGEN_SCALES, HGB(7), HGB(BASS_FM),
      HGB(VOICE_MAJ7) | HGB(VOICE_MIN7) | HGB(VOICE_SUS4),
      { 40, 24, 30, 64, 29, 16, 0 },
      { HGEN_QUARTERS, HGEN_BACKBEAT, HGEN_OFFBEATS, HGEN_BASS_ANCHOR }, { HGEN_KICK_BAN, HGEN_CLAP_BAN, 0u, HGEN_QUARTERS }, 0, 0, -6, 4 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t HOUSE_GEN_LIM = {
    .dens_lo = 64, .dens_hi = 124, .swing_lo = 50, .swing_hi = 76, .pump_lo = 60, .pump_hi = 120,
    .tank_lo = 0, .tank_hi = 110, .wob_lo = 0, .wob_hi = 100, .dly_lo = 16, .dly_hi = 64,
    .rev_lo = 8, .rev_hi = 60, .fb_lo = 6, .fb_hi = 40, .root_lo = 45, .root_hi = 64,
};

/* 196 name words, NUL-separated, empty word ends the list (1249 bytes). */
static const char HOUSE_GEN_WORDS[] =
    "JACK\0BOUNCE\0SHAKE\0SHIMMY\0STRUT\0GLIDE\0SWING\0DISCO\0DIVA\0GOSPEL\0CHURCH\0CHOIR\0PRAISE\0HEAVEN\0"
    "ANGEL\0HALO\0GLORY\0SUNRISE\0SUNSET\0SUMMER\0BALMY\0TROPIC\0PALM\0BREEZE\0OCEAN\0WAVE\0TIDE\0SHORE\0"
    "BEACH\0ISLAND\0MARINA\0LAGOON\0CORAL\0VELVET\0SILK\0SATIN\0LACE\0PEARL\0GOLD\0SILVER\0DIAMOND\0CRYSTAL\0"
    "MIRROR\0GLITTER\0SPARKLE\0SEQUIN\0STROBE\0LIGHTS\0NEON\0LOFT\0ATTIC\0BASEMENT\0DANCE\0DANCER\0MOVE\0"
    "STEP\0SPIN\0TWIRL\0CLAP\0SNAP\0STOMP\0FUNK\0FUNKY\0JAZZY\0RHYTHM\0PIANO\0ORGAN\0VOCAL\0HOOK\0ANTHEM\0"
    "HYMN\0UNITY\0FREEDOM\0PEACE\0RESPECT\0FAITH\0HOPE\0DESIRE\0PASSION\0FEVER\0BURN\0HEAT\0TOGETHER\0"
    "FAMILY\0TRIBE\0CROWD\0PARTY\0SOIREE\0FIESTA\0CARNIVAL\0SAMBA\0SALSA\0MAMBO\0CONGA\0BONGO\0CABASA\0"
    "COWBELL\0CHIME\0BELL\0TRUMPET\0HORNS\0BRASS\0STRINGS\0VIOLIN\0CELLO\0HARP\0FLUTE\0SAX\0SUNDAY\0"
    "MIDNIGHT\0LATE\0DAWN\0DUSK\0TWILIGHT\0MOONLIT\0STARRY\0COSMIC\0ORBIT\0LUNAR\0SOLAR\0GOLDEN\0AMBER\0"
    "HONEY\0SUGAR\0CANDY\0CHERRY\0PEACH\0MANGO\0LIME\0LEMON\0MINT\0GINGER\0SPICE\0PEPPER\0COCOA\0COFFEE\0"
    "ESPRESSO\0TONIC\0FIZZ\0BUBBLE\0CHICAGO\0HARLEM\0SOHO\0BROOKLYN\0PARIS\0WARMTH\0TENDER\0SWEET\0GENTLE\0"
    "SOFTLY\0EMBRACE\0HUG\0TOUCH\0HEART\0SOUL\0ELEVATE\0RISE\0LIFT\0HIGHER\0UPLIFT\0FLY\0SOAR\0WINGS\0"
    "FEATHER\0CLOUD\0SKY\0BLUE\0AZURE\0INDIGO\0VIOLET\0LAVENDER\0ROSE\0ORCHID\0LOTUS\0JASMINE\0LILY\0IVY\0"
    "FERN\0MOSS\0GARDEN\0ORCHARD\0MEADOW\0SHELTER\0HOME\0BELIEVE\0TESTIFY\0RAPTURE\0BLISS\0GLOW\0RADIANT\0"
    "SHINE\0HOTEL\0LOBBY\0ROOFTOP\0TERRACE\0LOUNGE\0";

static const fm_pgen_def_t HOUSE_GEN_DEF = {
    .key = HOUSE_GEN_KEY,
    .genome = HOUSE_GENOME,
    .lim = &HOUSE_GEN_LIM,
    .words = HOUSE_GEN_WORDS,
    .word_n = 196u,
    .rom = HOUSE_TRAX,
    .root = HOUSE_TRAX_ROOT,
    .scale = HOUSE_TRAX_SCALE,
    .swing = HOUSE_TRAX_SWING,
    .rate = HOUSE_TRAX_STAB_RATE,
    .kit = HOUSE_TRAX_KIT,
    .era_n = HOUSE_ERA_N,
    .steps = 16u,
};

#define FM_PGEN_DEF HOUSE_GEN_DEF

static inline void house_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    fm_pgen_build(&house_gen, NULL, &HOUSE_GEN_DEF, house_clamp_era(era), n, comply);
}

#endif
