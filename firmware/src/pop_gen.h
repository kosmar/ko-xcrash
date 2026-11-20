/* SPDX-License-Identifier: GPL-3.0-only */
/* KO POP endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of pop_data.h.
 * POP plays one 16-step bar (dub_step_bit reads step & 15) and stores it twice (hi16 = lo16); generated masks
 * are mirrored the same way. Groove, sound and tempo follow the continuum; root and scale stay the nearest
 * landmark's, because POP_TRACK_PROG_IX, POP_MELO_A/B and POP_BASS_STEP are written for that key and mode and
 * are indexed by the shadowed slot. Must = on in every curated plate of the era, ban = on in none. */
#ifndef POP_GEN_H
#define POP_GEN_H

#include "fm_pgen_core.h"

#define POP_GEN_KEY 0x504F5031u

#define PGB(x) FM_PGEN_B(x)
#define PGEN_M(x) ((uint32_t)(x) | ((uint32_t)(x) << 16))
#define PGEN_KITS 0xFFu
#define PGEN_BASSES (PGB(BASS_TINE) | PGB(BASS_2OP) | PGB(BASS_3OP) | PGB(BASS_4OP) | PGB(BASS_BRASS))
#define PGEN_VOICES (PGB(VOICE_MAJ) | PGB(VOICE_MIN) | PGB(VOICE_SUS))

/* must/ban lanes: kick rim(snare) hat sub chord siren(lead trigger) lead (unused). */
static const fm_pgen_genome_t POP_GENOME[POP_ERA_N] = {
    /* COLD: kick on 1, snare backbeat, 8th hats, lead pickups on 9/13 */
    { 108, 126, 48, 48, 50, 50, 64, 72, 10, 10, 0, 8, 6, 12, 14, 22, 6, 12, 32, 55, 65,
      PGB(1) | PGB(2), PGEN_KITS, PGEN_BASSES, PGEN_VOICES,
      { 54, 51, 47, 112, 107, 137, 0 },
      { PGEN_M(0x0001u), PGEN_M(0x1010u), PGEN_M(0x5555u), PGEN_M(0x1111u), PGEN_M(0x1111u), PGEN_M(0x2200u) },
      { PGEN_M(0xA296u), PGEN_M(0xAB23u), PGEN_M(0x8000u), 0u, PGEN_M(0x2002u), PGEN_M(0xDDC1u) }, 0, 0, -5, 4 },
    /* RADIO: kick on 1, snare backbeat, busy 16th hats, lead anchored on step 3 */
    { 100, 124, 48, 48, 50, 50, 64, 72, 11, 11, 0, 8, 6, 12, 14, 22, 6, 12, 32, 54, 65,
      PGB(1) | PGB(2) | PGB(4), PGEN_KITS, PGEN_BASSES, PGEN_VOICES,
      { 51, 43, 207, 86, 119, 90, 0 },
      { PGEN_M(0x0001u), PGEN_M(0x1010u), PGEN_M(0x4444u), PGEN_M(0x1111u), PGEN_M(0x1111u), PGEN_M(0x0008u) },
      { PGEN_M(0xB01Eu), PGEN_M(0x2B23u), 0u, PGEN_M(0x0022u), PGEN_M(0x2002u), PGEN_M(0x8820u) }, 0, 0, -5, 5 },
    /* CHROME: kick on 1, snare backbeat, 8th hats, syncopated bass, light swing */
    { 112, 128, 48, 48, 50, 56, 64, 72, 12, 12, 0, 8, 6, 12, 14, 22, 6, 12, 32, 54, 65,
      PGB(1) | PGB(2) | PGB(4), PGEN_KITS, PGEN_BASSES, PGEN_VOICES,
      { 98, 51, 121, 159, 99, 80, 0 },
      { PGEN_M(0x0001u), PGEN_M(0x1010u), PGEN_M(0x5555u), PGEN_M(0x4501u), PGEN_M(0x1111u), PGEN_M(0x0040u) },
      { PGEN_M(0xB0B6u), PGEN_M(0xA923u), 0u, PGEN_M(0x0002u), PGEN_M(0x0002u), PGEN_M(0x2000u) }, 0, 0, -5, 5 },
    /* ITALO: 4/4 kick, snare backbeat, offbeat hats, octave bass */
    { 118, 134, 48, 48, 50, 50, 64, 72, 13, 13, 0, 8, 6, 12, 14, 22, 6, 12, 32, 54, 65,
      PGB(1) | PGB(2), PGEN_KITS, PGEN_BASSES, PGEN_VOICES,
      { 31, 51, 89, 153, 111, 94, 0 },
      { PGEN_M(0x1111u), PGEN_M(0x1010u), PGEN_M(0x4444u), PGEN_M(0x4545u), PGEN_M(0x1111u), PGEN_M(0x0008u) },
      { PGEN_M(0x8006u), PGEN_M(0xAB23u), PGEN_M(0xA000u), 0u, PGEN_M(0x0002u), PGEN_M(0x4110u) }, 0, 0, -5, 5 },
    /* BLUE: half-time ballad, snare on 3, 8th hats, lead on the beats */
    { 76, 98, 48, 48, 50, 50, 64, 72, 14, 14, 0, 8, 6, 12, 14, 22, 6, 12, 32, 54, 65,
      PGB(1) | PGB(4), PGEN_KITS, PGEN_BASSES, PGEN_VOICES,
      { 44, 25, 51, 67, 107, 25, 0 },
      { PGEN_M(0x0001u), PGEN_M(0x0100u), PGEN_M(0x5555u), PGEN_M(0x1001u), PGEN_M(0x1111u), PGEN_M(0x1010u) },
      { PGEN_M(0x80BEu), PGEN_M(0x3211u), PGEN_M(0x8000u), 0u, PGEN_M(0x0022u), PGEN_M(0xE003u) }, 0, 0, -5, 5 },
    /* AFTER: 4/4 kick, snare backbeat, offbeat hats, 8th bass */
    { 114, 128, 48, 48, 50, 50, 64, 72, 15, 15, 0, 8, 6, 12, 14, 22, 6, 12, 32, 54, 64,
      PGB(1) | PGB(2), PGEN_KITS, PGEN_BASSES, PGEN_VOICES,
      { 31, 51, 89, 95, 111, 78, 0 },
      { PGEN_M(0x1111u), PGEN_M(0x1010u), PGEN_M(0x4444u), PGEN_M(0x5555u), PGEN_M(0x1111u), PGEN_M(0x0008u) },
      { PGEN_M(0x8006u), PGEN_M(0xAB23u), PGEN_M(0xA000u), 0u, PGEN_M(0x0020u), PGEN_M(0x2080u) }, 0, 0, -5, 5 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t POP_GEN_LIM = {
    .dens_lo = 44, .dens_hi = 52, .swing_lo = 50, .swing_hi = 58, .pump_lo = 56, .pump_hi = 80,
    .tank_lo = 8, .tank_hi = 18, .wob_lo = 0, .wob_hi = 14, .dly_lo = 4, .dly_hi = 16,
    .rev_lo = 12, .rev_hi = 28, .fb_lo = 4, .fb_hi = 16, .root_lo = 54, .root_hi = 65,
};

/* 170 name words, NUL-separated, empty word ends the list (1177 bytes). */
static const char POP_GEN_WORDS[] =
    "SUGAR\0CANDY\0CHERRY\0NEON\0CHROME\0GLAM\0IDOL\0ANTHEM\0CHORUS\0HOOK\0SINGLE\0RADIO\0VIDEO\0TEENAGE\0"
    "HEART\0KISS\0CRUSH\0DREAM\0DAYDREAM\0SUNSHINE\0RAINBOW\0SKYLINE\0HOLIDAY\0WEEKEND\0FRIDAY\0SATURDAY\0"
    "DISCO\0ROLLER\0SKATE\0ARCADE\0PINBALL\0CASSETTE\0MIXTAPE\0PHONE\0HOTLINE\0SPUTNIK\0ROCKET\0GALAXY\0"
    "PLANET\0VENUS\0MARS\0LUNA\0COMET\0METEOR\0TOKYO\0MILANO\0MONACO\0CAPRI\0LAGOON\0PALMS\0MIAMI\0SUNSET\0"
    "FLAMINGO\0PASTEL\0LILAC\0PINK\0MAGENTA\0CYAN\0ELECTRO\0SYNTH\0VOCODER\0KEYTAR\0LASER\0HOLOGRAM\0SHINE\0"
    "SPARKLE\0GLITTER\0DIAMOND\0CRYSTAL\0PEARL\0VELOUR\0SATIN\0CHIFFON\0MOONBEAM\0STARRY\0TWINKLE\0GLOW\0"
    "SPARK\0FLASH\0STROBE\0STAGE\0ENCORE\0APPLAUSE\0ENCORES\0FANFARE\0PARADE\0CONFETTI\0BALLOON\0RIBBON\0"
    "BOW\0LACE\0DOLL\0TOYBOX\0CANDYBAR\0LOLLIPOP\0GUMDROP\0BUBBLE\0FIZZ\0SODA\0LEMONADE\0SORBET\0GELATO\0"
    "SUNDAE\0SPRINKLE\0CUPCAKE\0VANILLA\0PEACH\0APRICOT\0PLUM\0MANGO\0PAPAYA\0KIWI\0CITRUS\0LEMON\0LIME\0"
    "ORANGE\0COCONUT\0MELON\0BERRY\0HONEY\0BLOSSOM\0DAISY\0TULIP\0POPPY\0VIOLET\0IRIS\0ORCHID\0LOTUS\0ROSE\0"
    "PETAL\0GARDEN\0MEADOW\0SUMMER\0SPRING\0AUTUMN\0WINTER\0SNOWFALL\0SUNBEAM\0HEATWAVE\0BREEZE\0TIDE\0WAVE\0"
    "SURF\0BEACH\0SHORE\0COAST\0ISLAND\0PARADISE\0TROPICAL\0CABANA\0HAMMOCK\0POSTCARD\0SOUVENIR\0GETAWAY\0"
    "ROADTRIP\0HIGHWAY\0DRIVE\0CRUISE\0MOTEL\0LOBBY\0ELEVATOR\0MALL\0BOUTIQUE\0RUNWAY\0GLAMOUR\0GLOWING\0"
    "DAZZLE\0RADIANT\0VIVID\0BRIGHT\0";

static const fm_pgen_def_t POP_GEN_DEF = {
    .key = POP_GEN_KEY,
    .genome = POP_GENOME,
    .lim = &POP_GEN_LIM,
    .words = POP_GEN_WORDS,
    .word_n = 170u,
    .rom = POP_PLATE,
    .root = POP_PLATE_ROOT,
    .scale = POP_PLATE_SCALE,
    .swing = POP_PLATE_SWING,
    .rate = POP_PLATE_LEAD_RATE,
    .kit = POP_PLATE_KIT,
    .era_n = POP_ERA_N,
    .steps = 32u,
};

/* Max hits per bar (kick rim hat sub chord siren), curated era maximum + headroom. Borrowed densities at low
 * compliance would otherwise stack kicks and lead triggers; caps never touch curated plates. */
static const uint8_t POP_GEN_CAP[POP_ERA_N][6] = {
    { 4, 5, 16, 16, 11, 7 }, { 4, 5, 16, 16, 11, 7 }, { 6, 5, 16, 16, 11, 7 },
    { 7, 5, 16, 16, 11, 7 }, { 4, 3, 16, 16, 11, 4 }, { 7, 5, 16, 16, 11, 7 },
};

#define FM_PGEN_DEF POP_GEN_DEF

/* Drop the latest free hits until the bar has at most cap hits. */
static inline uint32_t pop_gen_cap(uint32_t m, uint32_t must, uint8_t cap)
{
    uint8_t c = 0u, s;

    for (s = 0u; s < 16u; s++)
        c += (uint8_t)((m >> s) & 1u);
    for (s = 16u; s-- > 0u && c > cap;)
        if (((m & ~must) >> s) & 1u) {
            m &= ~(1u << s);
            c--;
        }
    return m;
}

static inline void pop_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    dub_plate_t *pl = &pop_gen.pl;
    uint32_t *const m[6] = { &pl->kick, &pl->rim, &pl->hat, &pl->sub, &pl->chord, &pl->siren };
    uint8_t l;

    era = pop_clamp_era(era);
    fm_pgen_build(&pop_gen, NULL, &POP_GEN_DEF, era, n, comply);
    pop_gen.root = POP_PLATE_ROOT[pop_gen.slot];
    pop_gen.scale = POP_PLATE_SCALE[pop_gen.slot];
    for (l = 0u; l < 6u; l++)
        *m[l] = PGEN_M(pop_gen_cap(*m[l] & 0xFFFFu, POP_GENOME[era].must[l] & 0xFFFFu, POP_GEN_CAP[era][l]));
    if (pop_gen.views_live)
        fm_pgen_views_build(&pop_gen, &POP_GEN_DEF, fm_pgen_hash(fm_pgen_gkey(&POP_GEN_DEF, FM_PGEN_G_BLEND), (uint32_t)n));
}

#endif
