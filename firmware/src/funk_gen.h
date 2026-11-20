/* SPDX-License-Identifier: GPL-3.0-only */
/* KO FUNK endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of funk_data.h.
 * 16-step masks (dub_step_bit reads step & 15). Ranges follow the ten curated grooves per era
 * (tools/pgen_era_stats.py): FUNK has no slow variants, every slot is full tempo. */
#ifndef FUNK_GEN_H
#define FUNK_GEN_H

#include "fm_pgen_core.h"

#define FUNK_GEN_KEY 0x46554E31u

#define FGB(x) FM_PGEN_B(x)
#define FGEN_ONE 0x0001u         /* the one */
#define FGEN_POCKET_KICK 0x2141u /* 1, a-of-2, 3, e-of-4: the curated pocket kick */
#define FGEN_SLAP_KICK 0x0101u   /* 1 and 3, room for the thumb */
#define FGEN_QUARTERS 0x1111u    /* disco four-on-the-floor */
#define FGEN_BACKBEAT 0x1010u    /* snare/clap on 2 and 4 */
#define FGEN_SNARE_BAN 0x0101u   /* no snare on 1 and 3 */
#define FGEN_GHOST_BAN(kick) (FGEN_SNARE_BAN | (kick)) /* ghosts stay off the anchor kicks */
#define FGEN_KICK_BAN 0x1010u    /* kick never on the backbeat snare */
#define FGEN_DISCO_KICK_BAN 0x0EEEu /* 4/4: extras only as last-beat pickups */
#define FGEN_OFFBEATS 0x4444u    /* hat on the "and" */
#define FGEN_CLAV_HAT 0x4C4Cu    /* and + a: chicken-scratch hats */
#define FGEN_SLAP_HAT 0x0404u
#define FGEN_EIGHTHS 0x5555u     /* disco 8ths, 16ths free on top */
#define FGEN_BASS_ANCHOR 0x0901u /* bass on 1, 3 and the a-of-3: syncopated, anchored on the one */
#define FGEN_SLAP_BASS 0x0501u   /* thumb on 1, 3 and the and-of-3 */
#define FGEN_OCT_BASS 0x5555u    /* disco octave 8ths */

/* must/ban lanes: kick rim(snare) hat sub chord siren(stab) lead (unused). 4/4 eras: the stab owns the backbeat. */
static const fm_pgen_genome_t FUNK_GENOME[FUNK_ERA_N] = {
    /* POCKET: JB pocket kick, backbeat + ghosts, offbeat hat, finger bass */
    { 96, 108, 105, 110, 52, 58, 88, 97, 16, 23, 2, 7, 24, 33, 18, 25, 14, 19, 37, 36, 48,
      FGB(0) | FGB(1) | FGB(2) | FGB(4) | FGB(5) | FGB(7) | FGB(11), FGB(DKIT_DRY), FGB(BASS_FINGER),
      FGB(VOICE_DOM7) | FGB(VOICE_MIN9) | FGB(VOICE_NINTH) | FGB(VOICE_THIRT) | FGB(VOICE_SUS4),
      { 20, 28, 40, 96, 16, 32, 0 },
      { FGEN_POCKET_KICK, FGEN_BACKBEAT, FGEN_OFFBEATS, FGEN_BASS_ANCHOR, FGEN_ONE },
      { FGEN_KICK_BAN, FGEN_GHOST_BAN(FGEN_POCKET_KICK) }, 0, 0, -5, 5 },
    /* CLAV: pocket kick, chicken-scratch hats, clav stabs */
    { 100, 112, 105, 110, 56, 64, 88, 97, 0, 23, 2, 7, 24, 33, 18, 25, 14, 19, 37, 36, 55,
      FGB(0) | FGB(1) | FGB(2) | FGB(4) | FGB(5) | FGB(6) | FGB(10), FGB(DKIT_CLAV), FGB(BASS_FINGER),
      FGB(VOICE_DOM7) | FGB(VOICE_MIN9) | FGB(VOICE_NINTH) | FGB(VOICE_THIRT) | FGB(VOICE_SUS4),
      { 20, 28, 40, 96, 16, 40, 0 },
      { FGEN_POCKET_KICK, FGEN_BACKBEAT, FGEN_CLAV_HAT, FGEN_BASS_ANCHOR, FGEN_ONE },
      { FGEN_KICK_BAN, FGEN_GHOST_BAN(FGEN_POCKET_KICK) }, 0, 0, -6, 5 },
    /* SLAP: kick on 1 and 3, sparse hat, busy thumb-and-pop bass */
    { 92, 104, 105, 110, 54, 60, 88, 97, 0, 7, 2, 7, 24, 33, 18, 25, 14, 19, 37, 36, 46,
      FGB(0) | FGB(1) | FGB(2) | FGB(4) | FGB(5) | FGB(6) | FGB(8), FGB(DKIT_SLAP), FGB(BASS_SLAP),
      FGB(VOICE_DOM7) | FGB(VOICE_MIN9) | FGB(VOICE_NINTH) | FGB(VOICE_THIRT) | FGB(VOICE_SUS4),
      { 24, 28, 48, 120, 16, 28, 0 },
      { FGEN_SLAP_KICK, FGEN_BACKBEAT, FGEN_SLAP_HAT, FGEN_SLAP_BASS, FGEN_ONE },
      { FGEN_KICK_BAN, FGEN_SNARE_BAN }, 0, 0, -5, 5 },
    /* DISCO: four-on-the-floor, 8th hats + 16ths, octave bass, backbeat string stabs */
    { 112, 122, 105, 110, 50, 54, 88, 97, 4, 11, 2, 7, 24, 33, 18, 25, 14, 19, 37, 36, 50,
      FGB(0) | FGB(1) | FGB(2) | FGB(4) | FGB(5) | FGB(7) | FGB(11), FGB(DKIT_DISCO), FGB(BASS_OCT),
      FGB(VOICE_DOM7) | FGB(VOICE_MIN9) | FGB(VOICE_NINTH) | FGB(VOICE_THIRT) | FGB(VOICE_SUS4),
      { 16, 20, 56, 20, 12, 12, 0 },
      { FGEN_QUARTERS, FGEN_BACKBEAT, FGEN_EIGHTHS, FGEN_OCT_BASS, FGEN_ONE, FGEN_BACKBEAT },
      { FGEN_DISCO_KICK_BAN, FGEN_SNARE_BAN, 0u, 0u, FGEN_BACKBEAT }, 0, 0, -4, 5 },
    /* BOOGIE: 4/4, 8th hats, syncopated octave / EP bass */
    { 108, 118, 105, 110, 54, 62, 88, 97, 8, 15, 2, 7, 24, 33, 18, 25, 14, 19, 37, 38, 48,
      FGB(0) | FGB(1) | FGB(2) | FGB(4) | FGB(5) | FGB(8) | FGB(10), FGB(DKIT_BOOG), FGB(BASS_OCT) | FGB(BASS_EP),
      FGB(VOICE_DOM7) | FGB(VOICE_MIN9) | FGB(VOICE_NINTH) | FGB(VOICE_THIRT) | FGB(VOICE_SUS4),
      { 16, 24, 48, 96, 12, 12, 0 },
      { FGEN_QUARTERS, FGEN_BACKBEAT, FGEN_EIGHTHS, FGEN_BASS_ANCHOR, FGEN_ONE, FGEN_BACKBEAT },
      { FGEN_DISCO_KICK_BAN, FGEN_SNARE_BAN, 0u, 0u, FGEN_BACKBEAT }, 0, 0, -7, 3 },
    /* BROKEN: kick on the one only, the rest drifts; heavy swing */
    { 84, 96, 105, 110, 66, 78, 88, 97, 12, 19, 2, 7, 24, 33, 18, 25, 14, 19, 37, 36, 52,
      FGB(0) | FGB(1) | FGB(2) | FGB(4) | FGB(5) | FGB(6) | FGB(7), FGB(DKIT_DUST), FGB(BASS_FINGER) | FGB(BASS_EP),
      FGB(VOICE_DOM7) | FGB(VOICE_MIN9) | FGB(VOICE_NINTH) | FGB(VOICE_THIRT) | FGB(VOICE_SUS4),
      { 48, 28, 56, 96, 16, 32, 0 },
      { FGEN_ONE, FGEN_BACKBEAT, FGEN_OFFBEATS, FGEN_BASS_ANCHOR, FGEN_ONE },
      { FGEN_KICK_BAN, FGEN_SNARE_BAN }, 0, 0, -6, 4 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t FUNK_GEN_LIM = {
    .dens_lo = 96, .dens_hi = 120, .swing_lo = 48, .swing_hi = 80, .pump_lo = 80, .pump_hi = 106,
    .tank_lo = 0, .tank_hi = 40, .wob_lo = 0, .wob_hi = 16, .dly_lo = 16, .dly_hi = 42,
    .rev_lo = 12, .rev_hi = 32, .fb_lo = 10, .fb_hi = 24, .root_lo = 34, .root_hi = 57,
};

/* 178 name words, NUL-separated, empty word ends the list (1171 bytes). */
static const char FUNK_GEN_WORDS[] =
    "GROOVE\0SLICK\0SHAKE\0BOOGIE\0FUNKY\0SOUL\0BROTHER\0SISTER\0SWEAT\0HOT\0HEAT\0BURN\0FIRE\0BLAZE\0FLAME\0"
    "CHICKEN\0GRITTY\0BRASS\0HORNS\0SAX\0TRUMPET\0CLAV\0WAH\0SLAP\0PLUCK\0THUMP\0BUMP\0JUMP\0JIVE\0SHIMMY\0"
    "SWAGGER\0DYNAMITE\0GALAXY\0COSMIC\0SPACE\0FLASH\0GLAMOUR\0SEQUIN\0AFRO\0PLATFORM\0VELVET\0LEATHER\0"
    "SUEDE\0DENIM\0FRINGE\0CHROME\0LOWRIDER\0AVENUE\0STREET\0CORNER\0BLOCK\0UPTOWN\0DOWNTOWN\0CITY\0METRO\0"
    "POCKET\0BACKBEAT\0CHANK\0RIFF\0LICK\0VAMP\0JAM\0BREAK\0BRIDGE\0HOOK\0CHORUS\0HORNLINE\0LOOSE\0SLIPPERY\0"
    "SMOOTH\0SILKY\0SATIN\0SPICY\0PEPPER\0SUGAR\0HONEY\0CANDY\0CHERRY\0JUICY\0BUTTER\0MOJO\0VOODOO\0HOODOO\0"
    "GUMBO\0BAYOU\0PARADE\0MARCH\0KICK\0SNARE\0HAT\0CYMBAL\0CONGA\0BONGO\0TIMBALE\0COWBELL\0ORGAN\0RHYTHM\0"
    "TEMPO\0SYNCOPE\0DANCE\0DANCER\0SPIN\0SPLIT\0SLIDE\0GLIDE\0SKATE\0RINK\0DISCO\0NEON\0LIGHTS\0STAGE\0"
    "SHOWTIME\0ENCORE\0CROWD\0PARTY\0SOIREE\0SUMMER\0SUNSHINE\0HUMID\0STEAMY\0SIZZLER\0SCORCHER\0WILDFIRE\0"
    "BONFIRE\0EMBER\0SPARKS\0FIREWORK\0ROCKET\0ORBIT\0STARSHIP\0PLANET\0SATURN\0JUPITER\0NEBULA\0COMET\0"
    "METEOR\0ALIEN\0ROBOT\0ANDROID\0GOLD\0PLATINUM\0DIAMOND\0JEWEL\0RUBY\0TOPAZ\0GARNET\0BRONZE\0COPPER\0"
    "BRASSY\0SHINY\0GLOSSY\0POLISH\0STRIDER\0PROWL\0SLINK\0CRUISE\0COAST\0RIDE\0DRIVE\0HIGHWAY\0FREEWAY\0"
    "EXPRESS\0SWING\0SWAY\0ROCK\0SHOUT\0HOLLER\0STRUTTER\0GROWL\0SQUAWK\0WHOOP\0YELP\0SCREAM\0PREACH\0"
    "SERMON\0REVIVAL\0TESTIFY\0RAPTURE\0";

static const fm_pgen_def_t FUNK_GEN_DEF = {
    .key = FUNK_GEN_KEY,
    .genome = FUNK_GENOME,
    .lim = &FUNK_GEN_LIM,
    .words = FUNK_GEN_WORDS,
    .word_n = 178u,
    .rom = FUNK_GROOVE,
    .root = FUNK_GROOVE_ROOT,
    .scale = FUNK_GROOVE_SCALE,
    .swing = FUNK_GROOVE_SWING,
    .rate = FUNK_GROOVE_STAB_RATE,
    .kit = FUNK_GROOVE_KIT,
    .era_n = FUNK_ERA_N,
    .steps = 16u,
};

#define FM_PGEN_DEF FUNK_GEN_DEF

static inline void funk_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    fm_pgen_build(&funk_gen, NULL, &FUNK_GEN_DEF, funk_clamp_era(era), n, comply);
}

#endif
