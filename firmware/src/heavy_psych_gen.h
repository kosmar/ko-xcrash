/* SPDX-License-Identifier: GPL-3.0-only */
/* KO HEAVY_PSYCH endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of heavy_psych_data.h. */
#ifndef HEAVY_PSYCH_GEN_H
#define HEAVY_PSYCH_GEN_H

#include "fm_pgen_core.h"

#define HEAVY_PSYCH_GEN_KEY 0x48565031u

#define HPB(x) FM_PGEN_B(x)
#define HPGEN_KICK_BAN 0xFEBEFEBEu /* kick only on 1, 7, 9 of each bar: heavy = space */
#define HPGEN_SNARE 0x10101010u
#define HPGEN_HAT_BAN 0xAAAAAAAAu  /* never odd 16ths */

/* must/ban lanes: kick rim hat sub chord siren lead. Bans are the curated never-steps of the era; the sub lane has
 * one anchor step per era so a bassline always exists, the lead gate four (the era's most common curated riff
 * steps). Kit indices > DKIT_N are curated (consumers use % DKIT_N). */
static const fm_pgen_genome_t HEAVY_PSYCH_GENOME[HEAVY_PSYCH_ERA_N] = {
    /* STONER */
    { 78, 96, 96, 118, 50, 57, 40, 98, 16, 20, 2, 8, 32, 48, 30, 46, 30, 34, 38, 41, 53,
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(7) | HPB(9),
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(7) | HPB(12) | HPB(13),
      HPB(BASS_SUB) | HPB(BASS_DOOM) | HPB(BASS_FUZZ) | HPB(BASS_OCT) | HPB(BASS_WAH) | HPB(BASS_MUFF) | HPB(BASS_REESE),
      HPB(VOICE_PWR5) | HPB(VOICE_MAJ) | HPB(VOICE_SUS2) | HPB(VOICE_OCT) | HPB(VOICE_MIN),
      { 128, 0, 230, 114, 92, 37, 98 },
      { (0x0001u | ((uint32_t)0x0001u << 16)), HPGEN_SNARE, FM_PGEN_QUARTERS, ((uint32_t)0x0001u << 16), 0u, 0u, 0x30000014u },
      { HPGEN_KICK_BAN, ~HPGEN_SNARE, HPGEN_HAT_BAN, 0xF0E4F8E0u, 0xFCFCF8F8u, 0xFC10FC10u, 0x05032200u },
      9, 0, -7, 13 },
    /* DESERT */
    { 68, 82, 84, 117, 52, 60, 40, 96, 26, 30, 2, 10, 36, 52, 36, 52, 32, 36, 38, 43, 55,
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(8) | HPB(10),
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(7),
      HPB(BASS_SUB) | HPB(BASS_DOOM) | HPB(BASS_FUZZ) | HPB(BASS_OCT) | HPB(BASS_WAH) | HPB(BASS_MUFF) | HPB(BASS_REESE),
      HPB(VOICE_PWR5) | HPB(VOICE_MAJ) | HPB(VOICE_SUS2) | HPB(VOICE_OCT) | HPB(VOICE_MIN),
      { 128, 0, 154, 57, 51, 41, 84 },
      { (0x0001u | ((uint32_t)0x0001u << 16)), HPGEN_SNARE, FM_PGEN_QUARTERS, ((uint32_t)0x0004u << 16), 0u, 0u, 0x30000110u },
      { HPGEN_KICK_BAN, ~HPGEN_SNARE, HPGEN_HAT_BAN, 0xFAEAFAEAu, 0xE0E0E0E0u, 0xFFC1FFC1u, 0x00032000u },
      10, 0, -6, 12 },
    /* SPACE */
    { 88, 106, 88, 119, 50, 57, 40, 100, 48, 56, 2, 12, 44, 62, 44, 62, 44, 50, 40, 45, 57,
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(7) | HPB(11),
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(7) | HPB(8) | HPB(9),
      HPB(BASS_SUB) | HPB(BASS_FUZZ) | HPB(BASS_OCT) | HPB(BASS_WAH) | HPB(BASS_MUFF) | HPB(BASS_REESE) | HPB(BASS_303),
      HPB(VOICE_PWR5) | HPB(VOICE_MAJ) | HPB(VOICE_SUS2) | HPB(VOICE_OCT) | HPB(VOICE_MIN) | HPB(5),
      { 128, 0, 51, 24, 61, 147, 77 },
      { (0x0001u | ((uint32_t)0x0001u << 16)), HPGEN_SNARE, FM_PGEN_QUARTERS, 0x00000001u, 0u, 0u, 0x00004054u },
      { HPGEN_KICK_BAN, ~HPGEN_SNARE, HPGEN_HAT_BAN, 0xF2EAF2EAu, 0xE1E1E1E1u, 0u, 0x05030000u },
      11, 0, -5, 10 },
    /* DRONE */
    { 58, 70, 84, 121, 53, 61, 92, 96, 6, 10, 4, 6, 42, 56, 44, 58, 46, 50, 37, 47, 59,
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(8),
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(7) | HPB(10) | HPB(11),
      HPB(BASS_SUB) | HPB(BASS_WAH) | HPB(BASS_MUFF),
      HPB(VOICE_PWR5) | HPB(VOICE_MAJ) | HPB(VOICE_SUS2) | HPB(VOICE_OCT) | HPB(VOICE_MIN),
      { 128, 0, 102, 31, 51, 38, 73 },
      { (0x0001u | ((uint32_t)0x0001u << 16)), HPGEN_SNARE, FM_PGEN_QUARTERS, 0x00000001u, 0u, 0u, 0x80008801u },
      { HPGEN_KICK_BAN, ~HPGEN_SNARE, HPGEN_HAT_BAN, 0xFEEEFEEEu, 0xE0E0E0E0u, 0xFFF0FFF0u, 0x01032500u },
      13, 0, -5, 8 },
    /* GAZE */
    { 80, 96, 90, 123, 52, 60, 90, 96, 88, 96, 6, 14, 38, 58, 38, 58, 26, 30, 39, 49, 61,
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(7) | HPB(9),
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(7) | HPB(10) | HPB(11),
      HPB(BASS_SUB) | HPB(BASS_FUZZ) | HPB(BASS_WAH) | HPB(BASS_MUFF) | HPB(BASS_REESE) | HPB(BASS_303),
      HPB(VOICE_PWR5) | HPB(VOICE_MAJ) | HPB(VOICE_SUS2) | HPB(VOICE_OCT) | HPB(VOICE_MIN),
      { 128, 0, 179, 37, 48, 55, 95 },
      { (0x0001u | ((uint32_t)0x0001u << 16)), HPGEN_SNARE, FM_PGEN_QUARTERS, 0x00000020u, 0u, 0u, 0x00300110u },
      { HPGEN_KICK_BAN, ~HPGEN_SNARE, HPGEN_HAT_BAN, 0x11151115u, 0xFCE0FCE0u, 0xFC38FC38u, 0x01032000u },
      11, 0, -5, 14 },
    /* DOOM */
    { 58, 86, 102, 124, 54, 62, 100, 114, 6, 20, 4, 18, 30, 56, 34, 60, 30, 44, 40, 36, 56,
      HPB(0) | HPB(1) | HPB(2) | HPB(3) | HPB(4) | HPB(5) | HPB(6) | HPB(7),
      HPB(0) | HPB(1) | HPB(6) | HPB(7),
      HPB(BASS_SUB) | HPB(BASS_DOOM) | HPB(BASS_FUZZ) | HPB(BASS_WAH),
      HPB(VOICE_PWR5) | HPB(VOICE_MAJ) | HPB(VOICE_SUS2) | HPB(VOICE_OCT) | HPB(VOICE_MIN),
      { 128, 0, 230, 40, 47, 33, 77 },
      { (0x0001u | ((uint32_t)0x0001u << 16)), HPGEN_SNARE, FM_PGEN_QUARTERS, 0x00000001u, 0u, 0u, 0x31004000u },
      { HPGEN_KICK_BAN, ~HPGEN_SNARE, HPGEN_HAT_BAN, 0xFCEEFCE8u, 0xFCE4FCE4u, 0xFE03FE03u, 0x00032100u },
      12, 0, -8, 12 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t HEAVY_PSYCH_GEN_LIM = {
    .dens_lo = 80, .dens_hi = 127, .swing_lo = 50, .swing_hi = 64, .pump_lo = 30, .pump_hi = 120,
    .tank_lo = 0, .tank_hi = 110, .wob_lo = 0, .wob_hi = 24, .dly_lo = 20, .dly_hi = 72,
    .rev_lo = 20, .rev_hi = 72, .fb_lo = 20, .fb_hi = 60, .root_lo = 34, .root_hi = 64,
};

/* Lead pitch is a semitone offset; weighted like the curated riffs (fourth below, major third above). */
static const int8_t HPGEN_INTERVALS[8] = { -5, 4, -3, -5, 4, 3, -2, 2 };

/* 178 name words, NUL-separated, empty word ends the list (1208 bytes). */
static const char HEAVY_PSYCH_GEN_WORDS[] =
    "MAGMA\0LAVA\0BASALT\0OBSIDIAN\0CRATER\0CANYON\0BUTTE\0ARROYO\0CACTUS\0SAGUARO\0COYOTE\0VULTURE\0"
    "SCORPION\0RATTLER\0MOJAVE\0SAHARA\0DESERT\0ARID\0DROUGHT\0SUNBURN\0HIGHWAY\0ENGINE\0VAN\0AMP\0TUBES\0"
    "WAH\0FEEDBACK\0DRONE\0MONOLITH\0TITAN\0COLOSSUS\0MAMMOTH\0BEHEMOTH\0GOLEM\0WIZARD\0SORCERER\0WITCH\0"
    "RITUAL\0CANDLE\0OMEN\0PROPHECY\0SERPENT\0DRAGON\0RAVEN\0CROW\0VOLCANO\0ERUPTION\0ASH\0EMBER\0SMOLDER\0"
    "GALAXY\0COSMOS\0ASTRAL\0NOVA\0METEOR\0ASTEROID\0SATURN\0JUPITER\0PLANET\0STARDUST\0STARGAZE\0HORIZON\0"
    "SOLSTICE\0EQUINOX\0GRAVITY\0MASS\0DENSE\0HEAVY\0LEADEN\0IRON\0RUST\0CHAIN\0ANVIL\0DOOM\0GLOOM\0TOMB\0"
    "CRYPT\0BARROW\0CAIRN\0MENHIR\0DOLMEN\0MANTRA\0PRISM\0SPIRAL\0VORTEX\0TEMPEST\0STORM\0THUNDER\0HAIL\0"
    "MONSOON\0TYPHOON\0CYCLONE\0SIROCCO\0HEATWAVE\0SUNDIAL\0OBELISK\0PYRAMID\0SPHINX\0PHARAOH\0TEMPLE\0"
    "ALTAR\0SHRINE\0RELIC\0IDOL\0TOTEM\0SKULL\0BONES\0FOSSIL\0AMBER\0TALON\0FANG\0HORN\0TUSK\0HIDE\0PELT\0"
    "FUR\0WOLF\0BISON\0BUFFALO\0STALLION\0MUSTANG\0JACKAL\0HYENA\0CONDOR\0OSPREY\0FALCON\0HAWK\0OWL\0MOTH\0"
    "LOCUST\0BEETLE\0WASP\0HORNET\0SPIDER\0VENOM\0POISON\0THORN\0BRAMBLE\0THISTLE\0SAGE\0JUNIPER\0YUCCA\0"
    "AGAVE\0MESQUITE\0SALTPAN\0PLAYA\0FLATS\0BADLANDS\0GULCH\0RAVINE\0GORGE\0CHASM\0CLIFF\0LEDGE\0SUMMIT\0"
    "PEAK\0RIDGE\0CAVE\0CAVERN\0GROTTO\0MAZE\0ABYSSAL\0DEPTHS\0MOLTEN\0FORGE\0FURNACE\0KILN\0SMELTER\0"
    "FOUNDRY\0CAULDRON\0BREW\0ELIXIR\0POTION\0TINCTURE\0AETHER\0PLASMA\0PHOTON\0PHANTOM\0";

static const fm_pgen_def_t HEAVY_PSYCH_GEN_DEF = {
    .key = HEAVY_PSYCH_GEN_KEY,
    .genome = HEAVY_PSYCH_GENOME,
    .lim = &HEAVY_PSYCH_GEN_LIM,
    .words = HEAVY_PSYCH_GEN_WORDS,
    .word_n = 178u,
    .rom = HEAVY_PSYCH_TRAX,
    .root = (const int8_t *)HEAVY_PSYCH_TRAX_ROOT,
    .scale = (const int8_t *)HEAVY_PSYCH_TRAX_SCALE,
    .swing = (const int8_t *)HEAVY_PSYCH_TRAX_SWING,
    .rate = HEAVY_PSYCH_TRAX_LEAD_RATE,
    .kit = (const int8_t *)HEAVY_PSYCH_TRAX_KIT,
    .lead_gate = HEAVY_PSYCH_TRAX_LEAD_GATE,
    .gate_cs = NULL,
    .lead_pitch = &HEAVY_PSYCH_TRAX_LEAD_PITCH[0][0],
    .lead_len = &HEAVY_PSYCH_TRAX_LEAD_LEN[0][0],
    .lead_bend = &HEAVY_PSYCH_TRAX_LEAD_BEND[0][0],
    .intervals = HPGEN_INTERVALS,
    .era_n = HEAVY_PSYCH_ERA_N,
    .steps = 32u,
    .pitch_n = 16u,
    .lead_hi_echo = 1u,
};

#define FM_PGEN_DEF HEAVY_PSYCH_GEN_DEF
#define FM_PGEN_LEAD_STATE heavy_psych_gen_lead

static inline void heavy_psych_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    fm_pgen_build(&heavy_psych_gen, &heavy_psych_gen_lead, &HEAVY_PSYCH_GEN_DEF, heavy_psych_clamp_era(era), n,
                  comply);
}

#endif
