/* SPDX-License-Identifier: GPL-3.0-only */
/* IDM endless plates: era genome + wiring for fm_pgen_core.h. Included at the end of idm_data.h.
 * 32-step masks. The irregular grids are the identity: must = steps on in every curated piece of the era, ban =
 * steps on in none (tools/pgen_era_stats.py). Ranges follow the eight full-tempo pieces per era; the two variants
 * per era stay landmarks only. Tempo is the raw piece bpm (idm_piece_apply adds IDM_ERA_BPM_ADJ). */
#ifndef IDM_GEN_H
#define IDM_GEN_H

#include "fm_pgen_core.h"

#define IDM_GEN_KEY 0x49444D31u

#define IGB(x) FM_PGEN_B(x)
/* WARM/MELO and GLITCH/BREAK share their curated rim/hat/chord/siren grids. */
#define IGEN_SOFT_RIM 0x1CC10000u
#define IGEN_SOFT_RIM_BAN 0x01120581u
#define IGEN_SOFT_HAT 0x40002982u
#define IGEN_SOFT_HAT_BAN 0x00055401u
#define IGEN_SOFT_CHORD 0x00800A0Du
#define IGEN_SOFT_CHORD_BAN 0x1B088402u
#define IGEN_SOFT_SIREN 0x00000140u
#define IGEN_SOFT_SIREN_BAN 0x00000204u
#define IGEN_CUT_RIM 0x080E0080u
#define IGEN_CUT_RIM_BAN 0xB2202D68u
#define IGEN_CUT_HAT 0x00005902u
#define IGEN_CUT_HAT_BAN 0x40052481u
#define IGEN_CUT_CHORD 0x1300880Eu
#define IGEN_CUT_CHORD_BAN 0x08880601u
#define IGEN_CUT_SIREN_BAN 0x00400040u
/* WARM/MELO lead: only the eight curated on-beat positions, one anchor note (sparse, never silent). */
#define IGEN_WARM_LEAD_BAN 0xBAFEEEEEu
#define IGEN_MELO_LEAD_BAN 0xBEEEEEEEu
/* DRILL hat: curated grid plus the two 16th rolls of bar 1 (busy hats are the era). */
#define IGEN_DRILL_HAT 0x4AA22EE0u

#define IGEN_SCALES_WARM (IGB(0) | IGB(1) | IGB(2) | IGB(4) | IGB(5) | IGB(7) | IGB(11))

/* must/ban lanes: kick rim hat sub chord siren lead. Kick is free in every era. */
static const fm_pgen_genome_t IDM_GENOME[IDM_ERA_N] = {
    /* WARM: slow, soft grids, sparse lead */
    { 90, 111, 74, 80, 50, 52, 4, 5, 8, 15, 10, 17, 34, 46, 32, 44, 20, 20, 38, 36, 48,
      IGEN_SCALES_WARM, IGB(0) | IGB(1) | IGB(2) | IGB(6) | IGB(8), IGB(BASS_ROUND),
      IGB(VOICE_MAJ7) | IGB(VOICE_MIN7) | IGB(VOICE_DOM9) | IGB(VOICE_MIN9) | IGB(VOICE_SUS4) | IGB(VOICE_69) |
          IGB(VOICE_QUART) | IGB(VOICE_ADD9),
      { 124, 127, 128, 157, 119, 123, 130 },
      { 0u, IGEN_SOFT_RIM, IGEN_SOFT_HAT, 0x00000001u, IGEN_SOFT_CHORD, IGEN_SOFT_SIREN, 0x00001000u },
      { ((uint32_t)0x0200u << 16), IGEN_SOFT_RIM_BAN, IGEN_SOFT_HAT_BAN, 0u, IGEN_SOFT_CHORD_BAN, IGEN_SOFT_SIREN_BAN,
        IGEN_WARM_LEAD_BAN },
      10, 30, -2, 3 },
    /* MELO: WARM grids, fuller bass, melodic lead */
    { 92, 113, 78, 84, 50, 52, 4, 5, 14, 21, 12, 19, 34, 46, 32, 44, 20, 20, 38, 36, 55,
      IGB(0) | IGB(1) | IGB(2) | IGB(4) | IGB(5) | IGB(6) | IGB(10), IGB(1) | IGB(2) | IGB(3) | IGB(7) | IGB(9),
      IGB(BASS_ROUND),
      IGB(VOICE_MIN7) | IGB(VOICE_DOM9) | IGB(VOICE_MIN9) | IGB(VOICE_SUS4) | IGB(VOICE_69) | IGB(VOICE_QUART) |
          IGB(VOICE_ADD9) | IGB(VOICE_MAJ6),
      { 114, 127, 128, 166, 119, 123, 165 },
      { 0u, IGEN_SOFT_RIM, IGEN_SOFT_HAT, 0x00000201u, IGEN_SOFT_CHORD, IGEN_SOFT_SIREN, 0x00100000u },
      { 0u, IGEN_SOFT_RIM_BAN, IGEN_SOFT_HAT_BAN, 0u, IGEN_SOFT_CHORD_BAN, IGEN_SOFT_SIREN_BAN, IGEN_MELO_LEAD_BAN },
      10, 28, -2, 3 },
    /* GLITCH: cut-up grids, busy bass */
    { 110, 145, 96, 102, 52, 55, 8, 9, 20, 27, 18, 25, 34, 46, 32, 44, 12, 12, 38, 36, 46,
      IGB(0) | IGB(1) | IGB(2) | IGB(4) | IGB(5) | IGB(6) | IGB(8), IGB(1) | IGB(2) | IGB(3) | IGB(8) | IGB(9),
      IGB(BASS_THROB),
      IGB(VOICE_DOM9) | IGB(VOICE_MIN9) | IGB(VOICE_SUS4) | IGB(VOICE_69) | IGB(VOICE_QUART) | IGB(VOICE_ADD9) |
          IGB(VOICE_MAJ6) | IGB(VOICE_DOM13),
      { 124, 123, 128, 201, 131, 119, 101 },
      { 0u, IGEN_CUT_RIM, IGEN_CUT_HAT, (0x0803u | ((uint32_t)0x0201u << 16)), IGEN_CUT_CHORD, 0u, 0x00004000u },
      { 0u, IGEN_CUT_RIM_BAN, IGEN_CUT_HAT_BAN, 0u, IGEN_CUT_CHORD_BAN, IGEN_CUT_SIREN_BAN, 0x9CCCAAAAu },
      6, 22, -2, 3 },
    /* BREAK: GLITCH grids, chopped break kicks */
    { 114, 149, 100, 106, 50, 53, 10, 11, 26, 33, 22, 29, 34, 46, 32, 44, 12, 12, 38, 36, 50,
      IGEN_SCALES_WARM, IGB(3) | IGB(4) | IGB(5) | IGB(10) | IGB(11), IGB(BASS_THROB),
      IGB(VOICE_MIN9) | IGB(VOICE_SUS4) | IGB(VOICE_69) | IGB(VOICE_QUART) | IGB(VOICE_ADD9) | IGB(VOICE_MAJ6) |
          IGB(VOICE_DOM13) | IGB(VOICE_MIN11),
      { 132, 123, 128, 202, 131, 119, 101 },
      { 0u, IGEN_CUT_RIM, IGEN_CUT_HAT, (0x08E1u | ((uint32_t)0x0001u << 16)), IGEN_CUT_CHORD, 0u, 0x40000000u },
      { 0u, IGEN_CUT_RIM_BAN, IGEN_CUT_HAT_BAN, 0u, IGEN_CUT_CHORD_BAN, IGEN_CUT_SIREN_BAN, 0xB2AA26EEu },
      8, 20, -2, 3 },
    /* DRILL: fast, dense rim/hat rolls, near-constant bass */
    { 150, 171, 112, 118, 50, 51, 14, 15, 32, 39, 28, 35, 34, 46, 32, 44, 12, 12, 38, 38, 48,
      IGB(0) | IGB(1) | IGB(2) | IGB(4) | IGB(5) | IGB(8) | IGB(10), IGB(6) | IGB(12), IGB(BASS_TEAR),
      IGB(VOICE_SUS4) | IGB(VOICE_69) | IGB(VOICE_QUART) | IGB(VOICE_ADD9) | IGB(VOICE_MAJ6) | IGB(VOICE_DOM13) |
          IGB(VOICE_MIN11) | IGB(VOICE_ADD11),
      { 137, 137, 64, 206, 134, 143, 172 },
      { 0u, 0x920A0080u, IGEN_DRILL_HAT, 0x720215BFu, 0x10008E09u, 0x40800008u, 0u },
      { 0u, 0x48000A2Bu, 0xA458D10Du, 0u, 0x0B880006u, 0u, 0x1444AAAAu },
      4, 14, -2, 3 },
    /* OBLIQ: slow tempo family (the 160+ pieces stay landmarks), skewed grids, fixed lead accents */
    { 92, 104, 80, 86, 52, 56, 6, 7, 38, 45, 14, 21, 34, 46, 32, 44, 12, 12, 38, 36, 52,
      IGB(0) | IGB(1) | IGB(2) | IGB(4) | IGB(5) | IGB(6) | IGB(7), IGB(5) | IGB(11),
      IGB(BASS_CLICK) | IGB(BASS_SAND) | IGB(BASS_BEND) | IGB(BASS_STATIC),
      IGB(VOICE_MAJ7) | IGB(VOICE_69) | IGB(VOICE_QUART) | IGB(VOICE_ADD9) | IGB(VOICE_MAJ6) | IGB(VOICE_DOM13) |
          IGB(VOICE_MIN11) | IGB(VOICE_ADD11),
      { 128, 131, 128, 176, 122, 132, 90 },
      { 0u, 0xC00800A2u, 0x669A100Cu, 0x00004000u, 0x02800C0Bu, 0x00000008u, 0x48004800u },
      { 0u, 0x1A025918u, 0x8860E101u, 0u, 0x19088204u, 0x40800000u, 0xB040B040u },
      9, 18, -2, 3 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t IDM_GEN_LIM = {
    .dens_lo = 40, .dens_hi = 127, .swing_lo = 50, .swing_hi = 64, .pump_lo = 0, .pump_hi = 24,
    .tank_lo = 0, .tank_hi = 60, .wob_lo = 0, .wob_hi = 50, .dly_lo = 20, .dly_hi = 64,
    .rev_lo = 20, .rev_hi = 60, .fb_lo = 6, .fb_hi = 30, .root_lo = 33, .root_hi = 58,
};

/* Lead pitch is a scale-degree offset (curated -3..5). */
static const int8_t IGEN_INTERVALS[8] = { 1, -1, 2, -2, 3, -3, 4, 5 };

/* 184 name words, NUL-separated, empty word ends the list (1245 bytes). */
static const char IDM_GEN_WORDS[] =
    "PRISM\0FRACTAL\0HELIX\0SPIRAL\0MOSAIC\0TESSERA\0ORIGAMI\0CIPHER\0CRYSTAL\0LENS\0ALGEBRA\0VECTOR\0"
    "MATRIX\0POLYGON\0TOPOLOGY\0MANIFOLD\0MOBIUS\0ENTROPY\0ECHO\0DELTA\0SINE\0CHROMA\0HUE\0TINT\0PASTEL\0"
    "OPAL\0QUARTZ\0MICA\0PEBBLE\0MOSS\0LICHEN\0FERN\0SPORE\0POLLEN\0SEED\0PETAL\0TIDEPOOL\0DROPLET\0RIPPLE\0"
    "FROST\0RIME\0THAW\0CHIRP\0TICK\0COGWHEEL\0SPROCKET\0ABACUS\0ANALOG\0MODEM\0TELETEXT\0KEYPAD\0CASSETTE\0"
    "DUST\0AURORA\0HALCYON\0NOCTURNE\0SERENE\0LULL\0DROWSY\0SLUMBER\0REVERIE\0PIXEL\0VOXEL\0GRID\0LOOP\0"
    "CYCLE\0ORBIT\0ELLIPSE\0ARC\0CURVE\0ANGLE\0VERTEX\0PARALLAX\0LUMEN\0FLICKER\0SHIMMER\0GLINT\0SPECKLE\0"
    "STIPPLE\0DITHER\0ALIAS\0ARTIFACT\0BITROT\0STATIC\0NOISE\0HISS\0HUM\0DRONE\0PULSE\0BLIP\0BEEP\0PING\0"
    "TONE\0TIMBRE\0HARMONIC\0OVERTONE\0PARTIAL\0GRANULE\0PARTICLE\0ATOM\0ION\0QUANTA\0PHOTON\0PROTON\0QUARK\0"
    "LEPTON\0NEUTRINO\0MOLECULE\0POLYMER\0CELL\0NUCLEUS\0CORTEX\0NEURON\0SYNAPSE\0DENDRITE\0AXON\0PLASMA\0"
    "ENZYME\0CATALYST\0REAGENT\0SOLVENT\0VAPOUR\0CONDENSE\0ICICLE\0GLACIER\0TUNDRA\0FJORD\0ATOLL\0ISTHMUS\0"
    "ESTUARY\0MARSH\0BOG\0HEATH\0MOOR\0TOR\0CAIRN\0ORCHARD\0GROVE\0THICKET\0BRAMBLE\0HEDGEROW\0WILLOW\0"
    "BIRCH\0ALDER\0HAZEL\0ROWAN\0ASPEN\0CEDAR\0LARCH\0SPRUCE\0JUNIPER\0THISTLE\0CLOVER\0NETTLE\0YARROW\0"
    "SORREL\0TEASEL\0RUSH\0SEDGE\0REED\0HERON\0PLOVER\0LAPWING\0CURLEW\0WREN\0FINCH\0LINNET\0SISKIN\0"
    "STARLING\0SWALLOW\0SWIFTLET\0MOTH\0CICADA\0CRICKET\0BEETLE\0MAYFLY\0LARVA\0PUPA\0COCOON\0HATCH\0EMERGE\0"
    "UNFOLD\0BLOOM\0WILT\0";

static const fm_pgen_def_t IDM_GEN_DEF = {
    .key = IDM_GEN_KEY,
    .genome = IDM_GENOME,
    .lim = &IDM_GEN_LIM,
    .words = IDM_GEN_WORDS,
    .word_n = 184u,
    .rom = IDM_PIECE,
    .root = IDM_PIECE_ROOT,
    .scale = IDM_PIECE_SCALE,
    .swing = IDM_PIECE_SWING,
    .rate = IDM_PIECE_LEAD_RATE,
    .kit = IDM_PIECE_KIT,
    .lead_gate = IDM_PIECE_LEAD_GATE,
    .gate_cs = IDM_PIECE_LEAD_GATE_CS,
    .lead_pitch = &IDM_PIECE_LEAD_PITCH[0][0],
    .lead_len = &IDM_PIECE_LEAD_LEN[0][0],
    .lead_bend = &IDM_PIECE_LEAD_BEND[0][0],
    .intervals = IGEN_INTERVALS,
    .era_n = IDM_ERA_N,
    .steps = 32u,
    .pitch_n = 16u,
    .lead_hi_echo = 1u,
};

#define FM_PGEN_DEF IDM_GEN_DEF
#define FM_PGEN_LEAD_STATE idm_gen_lead

static inline void idm_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    fm_pgen_build(&idm_gen, &idm_gen_lead, &IDM_GEN_DEF, idm_clamp_era(era), n, comply);
}

#endif
