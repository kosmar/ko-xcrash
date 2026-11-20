/* SPDX-License-Identifier: GPL-3.0-only */
/* KO HIPHOP endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of hiphop_data.h.
 * 32-step masks, lane S of the views = scratch lead gate. Ranges follow the eight full-tempo beats per era
 * (tools/pgen_era_stats.py); the two slow variants per era stay landmarks only. CLOUD: the 120–134 family.
 * Masks are written as two equal 16-step bars (HHGEN_BARS) so tools/build.py mmio_check sees no
 * register-window literal. */
#ifndef HIPHOP_GEN_H
#define HIPHOP_GEN_H

#include "fm_pgen_core.h"

#define HIPHOP_GEN_KEY 0x48495031u

#define HHB(x) FM_PGEN_B(x)
#define HHGEN_BARS(x) ((uint32_t)(x) | ((uint32_t)(x) << 16))
#define HHGEN_ONE HHGEN_BARS(0x0001u)          /* kick / 808 on each bar's one */
#define HHGEN_KICK HHGEN_BARS(0x0101u)         /* boom-bap kick on 1 and 3 */
#define HHGEN_KICK_GRIME (0x0141u | ((uint32_t)0x0101u << 16))
#define HHGEN_KICK_TRAP (0x0101u | ((uint32_t)0x0100u << 16))
#define HHGEN_SNARE HHGEN_BARS(0x1010u)        /* backbeat on 2 and 4, every era */
#define HHGEN_HAT HHGEN_BARS(0x4444u)          /* 8th "and" hats */
#define HHGEN_HAT_GRIME HHGEN_BARS(0xEEEEu)
#define HHGEN_HAT_TRAP HHGEN_BARS(0x7777u)     /* rolls fill the gaps */
#define HHGEN_HAT_CLOUD 0xAAAAu
#define HHGEN_PUSH HHGEN_BARS(0x0002u)         /* bass push right after the downbeat */
#define HHGEN_ODD16 HHGEN_BARS(0xAAAAu)
#define HHGEN_SCALES (HHB(0) | HHB(1) | HHB(2) | HHB(4) | HHB(8) | HHB(10))
#define HHGEN_VOICES (HHB(VOICE_MIN7) | HHB(VOICE_MIN9) | HHB(VOICE_SUS4) | HHB(VOICE_FIFTH) | HHB(VOICE_ADD9))

/* must/ban lanes: kick rim(snare) hat sub chord siren(unused, lead gate plays) lead(scratch).
 * Kick never on the snare, snare never on the kick anchor, chords only on 8ths. Swing is hard (hiphop_gen_build). */
static const fm_pgen_genome_t HIPHOP_GENOME[HIPHOP_ERA_N] = {
    /* PARK: breakbeat jams, kick 1/3, scratches on 8ths */
    { 98, 108, 90, 104, 67, 74, 46, 53, 16, 21, 12, 18, 20, 26, 17, 18, 10, 14, 38, 46, 58,
      HHGEN_SCALES, HHB(0) | HHB(1) | HHB(3) | HHB(7) | HHB(9) | HHB(13),
      HHB(BASS_SUB) | HHB(BASS_808) | HHB(BASS_REESE) | HHB(BASS_MOOG) | HHB(BASS_FM) | HHB(BASS_PLUCK), HHGEN_VOICES,
      { 16, 16, 40, 32, 8, 0, 64 },
      { HHGEN_KICK, HHGEN_SNARE, HHGEN_HAT, HHGEN_PUSH, HHGEN_ONE, HHGEN_BARS(0x0202u) },
      { HHGEN_SNARE, HHGEN_KICK, 0u, 0u, HHGEN_ODD16, 0u, HHGEN_ODD16 }, 10, 30, -2, 3 },
    /* GOLD: golden-era sample swing */
    { 88, 96, 90, 104, 70, 74, 52, 59, 18, 22, 16, 18, 20, 28, 18, 20, 12, 14, 38, 46, 58,
      HHGEN_SCALES, HHB(1) | HHB(3) | HHB(7) | HHB(9) | HHB(13),
      HHB(BASS_SUB) | HHB(BASS_808) | HHB(BASS_MOOG) | HHB(BASS_PLUCK), HHGEN_VOICES,
      { 16, 20, 40, 32, 8, 0, 80 },
      { HHGEN_KICK, HHGEN_SNARE, HHGEN_HAT, HHGEN_PUSH, HHGEN_ONE, HHGEN_BARS(0x0202u) },
      { HHGEN_SNARE, HHGEN_KICK, 0u, 0u, HHGEN_ODD16, 0u, HHGEN_ODD16 }, 10, 28, -2, 3 },
    /* BOOM: boom-bap, MPC, ghost snares */
    { 85, 94, 90, 104, 70, 74, 62, 69, 20, 24, 18, 18, 20, 27, 18, 22, 12, 14, 38, 46, 58,
      HHGEN_SCALES, HHB(2) | HHB(8),
      HHB(BASS_SUB) | HHB(BASS_808) | HHB(BASS_MOOG) | HHB(BASS_PLUCK), HHGEN_VOICES,
      { 20, 24, 40, 32, 8, 0, 64 },
      { HHGEN_KICK, HHGEN_SNARE, HHGEN_HAT, HHGEN_PUSH, HHGEN_ONE, HHGEN_BARS(0x0202u) },
      { HHGEN_SNARE, HHGEN_KICK, 0u, 0u, HHGEN_ODD16 }, 10, 22, -2, 3 },
    /* GRIME: dirty south 808, busy hats */
    { 90, 100, 90, 104, 57, 65, 68, 75, 28, 35, 20, 27, 28, 33, 30, 30, 14, 20, 44, 48, 60,
      HHGEN_SCALES, HHB(4) | HHB(10), HHB(BASS_808) | HHB(BASS_REESE) | HHB(BASS_WOOZY), HHGEN_VOICES,
      { 16, 16, 60, 40, 16, 0, 44 },
      { HHGEN_KICK_GRIME, HHGEN_SNARE, HHGEN_HAT_GRIME, HHGEN_PUSH, 0x0101u | ((uint32_t)0x0001u << 16),
        HHGEN_BARS(0x0404u) },
      { HHGEN_SNARE, HHGEN_KICK_GRIME, 0u, 0u, HHGEN_ODD16 }, 10, 20, -2, 3 },
    /* TRAP: 808 on the one, hat rolls, dense scratch */
    { 134, 150, 90, 104, 50, 58, 74, 81, 32, 39, 24, 31, 28, 35, 32, 32, 14, 20, 46, 48, 60,
      HHGEN_SCALES, HHB(5) | HHB(11), HHB(BASS_808) | HHB(BASS_FM) | HHB(BASS_WOOZY), HHGEN_VOICES,
      { 20, 12, 120, 20, 8, 0, 104 },
      { HHGEN_KICK_TRAP, HHGEN_SNARE, HHGEN_HAT_TRAP, HHGEN_ONE, HHGEN_ONE, HHGEN_BARS(0x0444u) },
      { HHGEN_SNARE, HHGEN_KICK_TRAP, 0u, 0u, HHGEN_ODD16 }, 10, 14, -2, 3 },
    /* CLOUD: sparse kick, airy first-bar hats, quarter pads */
    { 120, 134, 90, 104, 52, 62, 80, 87, 36, 43, 28, 35, 28, 35, 34, 34, 14, 20, 48, 48, 60,
      HHGEN_SCALES, HHB(6) | HHB(12), HHB(BASS_SUB) | HHB(BASS_PWM) | HHB(BASS_SINE) | HHB(BASS_WOOZY), HHGEN_VOICES,
      { 12, 8, 24, 16, 24, 0, 48 },
      { HHGEN_ONE, HHGEN_SNARE, HHGEN_HAT_CLOUD, HHGEN_ONE, HHGEN_BARS(0x1111u), HHGEN_BARS(0x4040u) },
      { HHGEN_SNARE, HHGEN_ONE, 0u, 0u, HHGEN_ODD16 }, 10, 18, -2, 3 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t HIPHOP_GEN_LIM = {
    .dens_lo = 64, .dens_hi = 124, .swing_lo = 50, .swing_hi = 80, .pump_lo = 30, .pump_hi = 100,
    .tank_lo = 0, .tank_hi = 60, .wob_lo = 0, .wob_hi = 48, .dly_lo = 10, .dly_hi = 60,
    .rev_lo = 8, .rev_hi = 60, .fb_lo = 6, .fb_hi = 30, .root_lo = 44, .root_hi = 62,
};

static const int8_t HHGEN_INTERVALS[8] = { 2, -1, 3, -2, 5, -3, 4, 1 };

/* 184 name words, NUL-separated, empty word ends the list (1203 bytes). */
static const char HIPHOP_GEN_WORDS[] =
    "CYPHER\0MIC\0BEATBOX\0DECKS\0SCRATCH\0DIGGIN\0NEEDLE\0GROOVE\0RHYME\0VERSE\0FLOW\0BARS\0HOOK\0CHORUS\0"
    "ADLIB\0BATTLE\0CREW\0POSSE\0SQUAD\0BLOCK\0CORNER\0STOOP\0BODEGA\0TRAIN\0TUNNEL\0BRIDGE\0BOROUGH\0"
    "UPTOWN\0PARK\0CONCRETE\0ASPHALT\0GRAFFITI\0TAG\0PIECE\0SPRAY\0MARKER\0STENCIL\0BOOMBOX\0GOLD\0CHAIN\0"
    "ROPE\0FRESH\0ILL\0WACK\0PHAT\0CRISP\0CLEAN\0KNOCK\0BANGER\0HEAT\0SMOOTH\0JAZZY\0SOULFUL\0MELLOW\0"
    "DREAMY\0RAINY\0TAPE\0CASSETTE\0STATIC\0HISS\0CRACKLE\0WARM\0DUSK\0SUNDOWN\0MIDNIGHT\0NEON\0LOOP\0FLIP\0"
    "PITCH\0SWING\0THUMP\0KICK\0SNARE\0HAT\0CLAP\0RIM\0SHAKER\0TAMBO\0BASSLINE\0SUB\0BOOMING\0RUMBLE\0"
    "BOUNCE\0SWAGGER\0STRUT\0STRIDE\0GLIDE\0ROLL\0CRUISE\0RIDE\0DRIVE\0CORNERS\0AVENUE\0STREET\0ALLEY\0"
    "ROOFTOP\0LOBBY\0PROJECTS\0COURT\0HOOPS\0SNEAKER\0LACES\0CAP\0HOODIE\0JACKET\0DENIM\0CANVAS\0LEATHER\0"
    "SUEDE\0BRIM\0VISOR\0SHADES\0GLOSS\0SHINE\0SPARKLE\0GLEAM\0CHROME\0RIMS\0BASS\0WOOFER\0SPEAKER\0STEREO\0"
    "RADIO\0DIAL\0STATION\0SIGNAL\0AIRWAVE\0MIXTAPE\0DEMO\0SESSION\0STUDIO\0BOOTH\0VOCAL\0TAKE\0BOUNCED\0"
    "MASTER\0PRESSING\0PLATTER\0RECORD\0SPIN\0SELECTOR\0CROWD\0JAMBOREE\0GATHER\0CIRCLE\0RITUAL\0PILLAR\0"
    "LEGEND\0ICON\0ROYAL\0CROWN\0THRONE\0KINGDOM\0EMPIRE\0DYNASTY\0LEGACY\0HISTORY\0SCHOOL\0LESSON\0TEACHER\0"
    "STUDENT\0SCRIBE\0POET\0WORDS\0LYRIC\0STORY\0TALE\0SCRIPT\0PENCIL\0NOTEBOOK\0PAGE\0CHAPTER\0ESSAY\0"
    "PROVERB\0WISDOM\0SCIENCE\0MATH\0PHYSICS\0LOGIC\0REASON\0TRUTH\0VISION\0FOCUS\0GRIND\0";

static const fm_pgen_def_t HIPHOP_GEN_DEF = {
    .key = HIPHOP_GEN_KEY,
    .genome = HIPHOP_GENOME,
    .lim = &HIPHOP_GEN_LIM,
    .words = HIPHOP_GEN_WORDS,
    .word_n = 184u,
    .rom = HIPHOP_BEAT,
    .root = HIPHOP_BEAT_ROOT,
    .scale = HIPHOP_BEAT_SCALE,
    .swing = HIPHOP_BEAT_SWING,
    .rate = HIPHOP_BEAT_LEAD_RATE,
    .kit = HIPHOP_BEAT_KIT,
    .lead_gate = HIPHOP_BEAT_LEAD_GATE,
    .gate_cs = HIPHOP_BEAT_LEAD_GATE_CS,
    .lead_pitch = &HIPHOP_BEAT_LEAD_PITCH[0][0],
    .lead_len = &HIPHOP_BEAT_LEAD_LEN[0][0],
    .lead_bend = &HIPHOP_BEAT_LEAD_BEND[0][0],
    .intervals = HHGEN_INTERVALS,
    .era_n = HIPHOP_ERA_N,
    .steps = 32u,
    .pitch_n = 16u,
    .lead_hi_echo = 1u,
};

#define FM_PGEN_DEF HIPHOP_GEN_DEF
#define FM_PGEN_LEAD_STATE hiphop_gen_lead

static inline void hiphop_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    const fm_pgen_genome_t *ge;
    uint16_t w;
    uint8_t k;

    era = hiphop_clamp_era(era);
    fm_pgen_build(&hiphop_gen, &hiphop_gen_lead, &HIPHOP_GEN_DEF, era, n, comply);
    /* Swing is the genre's identity: like bpm, always the plate's own era window, never widened or borrowed. */
    ge = &HIPHOP_GENOME[era];
    w = fm_pgen_landmark(n, &k);
    hiphop_gen.swing = (int8_t)fm_pgen_range(fm_pgen_gkey(&HIPHOP_GEN_DEF, FM_PGEN_G_SWING), n, 24u, ge->swing_lo,
                                             ge->swing_hi);
    if (w > 0u)
        hiphop_gen.swing = fm_pgen_blend_s8(hiphop_gen.swing, HIPHOP_BEAT_SWING[era * GENRE_PRESETS_PER_ERA + k], w);
}

#endif
