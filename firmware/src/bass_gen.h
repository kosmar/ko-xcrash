/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS endless plates (recipe: docs/plate-gen-recipe.md, BASS section). Included from bass_data.h right after
 * the ROM tables, before the routed accessors and the genre-fader bake that read it.
 * A BASS plate = sound genes (dub_plate_t, root/scale/swing/kit via fm_pgen_build) + an 8-bar phrase: bar gates
 * (bar 1 = S1, bars 2..8 = the bar-stack S2..S8), pitch[16], len[128], bend[128]. The phrase rules come from the
 * curated lines: downbeat on every bar, never steps 1/5/9/15, 1..6 hits a bar, long notes on steps 7/14, bends on
 * steps 7/15 (none on the last bar's 15). Landmarks are the curated lines exactly. */
#ifndef BASS_GEN_H
#define BASS_GEN_H

#include "fm_pgen_core.h"

#define BASS_GEN_KEY 0x42415331u
#define BGB(x) FM_PGEN_B(x)
#define BGEN_M(lo, hi) ((uint32_t)(lo) | ((uint32_t)(hi) << 16))
#define BGEN_BAR_MUST 0x0001u      /* the one */
#define BGEN_BAR_BAN 0x8222u       /* steps 1, 5, 9, 15: never in a curated line */
#define BGEN_BAR_HITS_MAX 6u
#define BGEN_SCALES (BGB(0) | BGB(1) | BGB(2) | BGB(3) | BGB(4) | BGB(5))
#define BGEN_KITS (BGB(0) | BGB(1) | BGB(2) | BGB(3) | BGB(4) | BGB(5) | BGB(6) | BGB(7))
#define BGEN_VOICES BGEN_KITS

/* must/ban lanes: kick rim hat sub chord siren lead. KO BASS has one lane: sub = bars 1|2 of the phrase. */
#define BGEN_MUST { 0u, 0u, 0u, BGEN_M(BGEN_BAR_MUST, BGEN_BAR_MUST) }
#define BGEN_BAN { 0u, 0u, 0u, BGEN_M(BGEN_BAR_BAN, BGEN_BAR_BAN) }

static const fm_pgen_genome_t BASS_GENOME[BASS_ERA_N] = {
    /* ACID */
    { 124, 132, 28, 31, 58, 65, 20, 20, 0, 77, 0, 63, 44, 62, 38, 47, 10, 19, 36, 36, 45,
      BGEN_SCALES, BGEN_KITS, BGB(0) | BGB(1) | BGB(2) | BGB(3) | BGB(4) | BGB(5) | BGB(6) | BGB(7) | BGB(8) | BGB(9),
      BGEN_VOICES, { 0 }, BGEN_MUST, BGEN_BAN, 0, 0, 0, 0 },
    /* SUB */
    { 118, 124, 31, 34, 58, 65, 25, 25, 6, 75, 2, 67, 48, 66, 41, 50, 11, 20, 43, 36, 47,
      BGEN_SCALES, BGEN_KITS, BGB(2) | BGB(3) | BGB(4) | BGB(5) | BGB(6) | BGB(7) | BGB(8) | BGB(9) | BGB(10) | BGB(11),
      BGEN_VOICES, { 0 }, BGEN_MUST, BGEN_BAN, 0, 0, 0, 0 },
    /* REESE */
    { 128, 136, 34, 37, 58, 65, 30, 30, 4, 73, 6, 71, 52, 70, 44, 53, 12, 21, 50, 36, 47,
      BGEN_SCALES, BGEN_KITS, BGB(4) | BGB(5) | BGB(6) | BGB(7) | BGB(8) | BGB(9) | BGB(10) | BGB(11) | BGB(12) | BGB(13),
      BGEN_VOICES, { 0 }, BGEN_MUST, BGEN_BAN, 0, 0, 0, 0 },
    /* FM */
    { 126, 134, 37, 40, 58, 65, 35, 35, 2, 71, 3, 68, 44, 70, 47, 56, 13, 22, 57, 36, 47,
      BGEN_SCALES, BGEN_KITS, BGB(0) | BGB(1) | BGB(6) | BGB(7) | BGB(8) | BGB(9) | BGB(10) | BGB(11) | BGB(12) | BGB(13),
      BGEN_VOICES, { 0 }, BGEN_MUST, BGEN_BAN, 0, 0, 0, 0 },
    /* FOLD */
    { 132, 142, 40, 43, 58, 65, 40, 40, 0, 69, 0, 65, 44, 70, 50, 59, 14, 23, 36, 36, 47,
      BGEN_SCALES, BGEN_KITS, BGB(0) | BGB(1) | BGB(2) | BGB(3) | BGB(8) | BGB(9) | BGB(10) | BGB(11) | BGB(12) | BGB(13),
      BGEN_VOICES, { 0 }, BGEN_MUST, BGEN_BAN, 0, 0, 0, 0 },
    /* SYNC */
    { 130, 138, 43, 46, 58, 65, 45, 45, 9, 78, 4, 69, 44, 70, 38, 61, 15, 24, 43, 38, 47,
      BGEN_SCALES, BGEN_KITS, BGB(0) | BGB(1) | BGB(2) | BGB(3) | BGB(4) | BGB(5) | BGB(10) | BGB(11) | BGB(12) | BGB(13),
      BGEN_VOICES, { 0 }, BGEN_MUST, BGEN_BAN, 0, 0, 0, 0 },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t BASS_GEN_LIM = {
    .dens_lo = 20, .dens_hi = 60, .swing_lo = 50, .swing_hi = 72, .pump_lo = 10, .pump_hi = 60,
    .tank_lo = 0, .tank_hi = 100, .wob_lo = 0, .wob_hi = 90, .dly_lo = 30, .dly_hi = 84,
    .rev_lo = 28, .rev_hi = 72, .fb_lo = 6, .fb_hi = 32, .root_lo = 36, .root_hi = 47,
};

/* 192 name words, NUL-separated, empty word ends the list (1220 bytes). */
static const char BASS_GEN_WORDS[] =
    "ACID\0RESONANT\0CUTOFF\0GLIDE\0ACCENT\0OCTAVE\0FILTER\0SWEEP\0ENVELOPE\0DECAY\0SUSTAIN\0RELEASE\0"
    "ATTACK\0RUBBER\0ELASTIC\0BOUNCE\0WOBBLE\0GROWL\0RUMBLE\0GURGLE\0BUBBLE\0FIZZ\0ACIDIC\0SMILEY\0RAVE\0"
    "RAVER\0STROBE\0LASER\0TOXIC\0VENOM\0REAGENT\0CATALYST\0ENZYME\0PLASMA\0MOLTEN\0LIQUID\0MERCURY\0CHROME\0"
    "SILICON\0WIRE\0COIL\0SPRING\0GEYSER\0LAVA\0NEON\0VOLT\0CHARGE\0SPARK\0JOLT\0ZAP\0BUZZ\0HUM\0STEPPER\0"
    "BASSLINE\0GARAGE\0NIBBLE\0CHEW\0GNAW\0SNARL\0HOWL\0FANG\0CLAW\0VIPER\0SERPENT\0COBRA\0EEL\0SQUID\0"
    "KRAKEN\0ANEMONE\0JELLY\0SLIME\0GOO\0OOZE\0SLUDGE\0MUD\0SWAMP\0BOG\0MARSH\0BAYOU\0LAGOON\0SALT\0BRINE\0"
    "ACRID\0CAUSTIC\0ETCH\0BURN\0SCORCH\0SIZZLE\0SPIT\0HISS\0SPUTTER\0CRACKLE\0STATIC\0SURGE\0FLUX\0PULSE\0"
    "THROB\0THUMP\0KNOCK\0STOMP\0PUNCH\0JAB\0HOOK\0UPPERCUT\0KNOCKOUT\0BRAWL\0SCRAP\0GRIND\0CHURN\0TWIST\0"
    "TURN\0SPIN\0SWIRL\0SPIRAL\0HELIX\0VORTEX\0CYCLONE\0TWISTER\0LOOP\0KNOT\0TANGLE\0WEAVE\0STRETCH\0SNAP\0"
    "RECOIL\0REBOUND\0SPRINGY\0SQUISH\0SQUEEZE\0PRESS\0PUMP\0PISTON\0MOTOR\0ENGINE\0TURBO\0NITRO\0DIESEL\0"
    "FUEL\0GAS\0OCTANE\0RACER\0RALLY\0DRIFTER\0SKID\0SWERVE\0ZIGZAG\0ZOOM\0DASH\0SPRINT\0RUSH\0STAMPEDE\0"
    "THUNDER\0RUMBLER\0GROWLER\0HOWLER\0PROWLER\0HUNTER\0CHASER\0RUNNER\0JUMPER\0GLIDER\0DIVER\0SWIMMER\0"
    "DIGGER\0BURROW\0TUNNEL\0CAVE\0PIT\0HOLE\0CRATER\0DEPTH\0ABYSS\0BASEMENT\0CELLAR\0VAULT\0BUNKER\0NIGHT\0"
    "MIDNIGHT\0DUSK\0DAWN\0SHADOW\0SMOG\0FOG\0HAZE\0MIST\0VAPOR\0STEAM\0SMOKE\0EMBER\0ASH\0CINDER\0COAL\0";

static const fm_pgen_def_t BASS_GEN_DEF = {
    .key = BASS_GEN_KEY,
    .genome = BASS_GENOME,
    .lim = &BASS_GEN_LIM,
    .words = BASS_GEN_WORDS,
    .word_n = 192u,
    .rom = BASS_LINE,
    .root = BASS_LINE_ROOT,
    .scale = BASS_LINE_SCALE,
    .swing = BASS_LINE_SWING,
    .kit = BASS_LINE_KIT,
    .era_n = BASS_ERA_N,
    .steps = 32u,
};

#define FM_PGEN_DEF BASS_GEN_DEF
/* KO BASS S2..S8 are the phrase bars, not gen_views.py views: the core's runtime views stay off, so the
 * hand-authored FM_VIEWS block of the nearest landmark answers fm_view_data_get(). */
#define FM_PGEN_VIEWS_HAND 1

/* Phrase genes: BASS-only keys beyond the core gene ids; part of the continuum, never renumber. */
enum {
    BGEN_G_BAR = 0x100, BGEN_G_MUT, BGEN_G_MUTV, BGEN_G_PITCH, BGEN_G_PVAL, BGEN_G_LONG, BGEN_G_LVAL, BGEN_G_BEND7,
    BGEN_G_BENDV, BGEN_G_BLEND,
    /* melodic pitch and archetype rhythm (appended: keep ids stable) */
    BGEN_G_PERA, BGEN_G_POFF, BGEN_G_PANCH, BGEN_G_PREACH, BGEN_G_PWALK, BGEN_G_PLEAP,
    BGEN_G_ARCH, BGEN_G_CORE, BGEN_G_OPTW, BGEN_G_OPTS, BGEN_G_SPARSE, BGEN_G_MIN, BGEN_G_REP
};

/* Per era, from the curated lines: step density before must/ban, chance that bars 2..8 redraw a step instead of
 * repeating bar 1, long-note pick (< long9 -> 9, < long10 -> 10, else 11) and the bend value on step 7. */
typedef struct {
    uint8_t d[16];
    uint8_t mut, long9, long10, bend7;
} bass_gen_phrase_genome_t;

static const bass_gen_phrase_genome_t BASS_GEN_PHRASE[BASS_ERA_N] = {
    { { 0, 0, 22, 42, 26, 0, 157, 38, 230, 0, 74, 42, 48, 22, 51, 0 }, 119, 118, 217, 4 },  /* ACID */
    { { 0, 0, 35, 29, 61, 0, 96, 13, 205, 0, 99, 29, 61, 35, 74, 0 }, 137, 45, 78, 5 },     /* SUB */
    { { 0, 0, 16, 32, 29, 0, 157, 29, 230, 0, 74, 32, 64, 16, 64, 0 }, 142, 85, 228, 4 },   /* REESE */
    { { 0, 0, 35, 32, 42, 0, 134, 10, 205, 0, 102, 32, 48, 35, 61, 0 }, 145, 37, 49, 5 },   /* FM */
    { { 0, 0, 13, 35, 22, 0, 166, 32, 230, 0, 48, 35, 48, 13, 48, 0 }, 161, 111, 234, 4 },  /* FOLD */
    { { 0, 0, 38, 38, 54, 0, 163, 22, 205, 0, 106, 38, 48, 38, 29, 0 }, 120, 112, 128, 5 }, /* SYNC */
};

/* Melodic pitch, port of tools/gen_bass_pack.py (line_pitches128, chord_anchors, PITCH_POOL / ERA_WALK / ERA_PROG).
 * Pool = sorted allowed pitches per era, walk = index deltas between hits, prog = chord root per bar (8 bars,
 * shifted per plate). Integer only; every choice is a fm_pgen_noise draw over n, so neighbouring seeds keep
 * their line. Range -5..12 (the ROM range). */
#define BGEN_PITCH_LO (-5)
#define BGEN_PITCH_HI 12
#define BGEN_POOL_MAX 11u
#define BGEN_WALK_MAX 7u
#define BGEN_CALL_RESP_ERA 5u      /* SYNC: bar 3 ends on the fifth (call), bar 7 resolves to the root */
#define BGEN_CHORD_TONES(e) (((e) == 1u || (e) == 4u) ? 0x81u : 0x99u)   /* mod-12 classes: {0,7} or {0,3,4,7} */
#define BGEN_LEAP_ERA(e) ((e) == 3u || (e) == 5u)                       /* FM and SYNC: occasional leaps */

static const int8_t BGEN_POOL[BASS_ERA_N][BGEN_POOL_MAX] = {
    { -2, -1, 0, 1, 2, 3, 5, 7, 8, 10, 12 },   /* ACID: chromatic/pentatonic 16th motion */
    { -5, 0, 7, 12, 0, 0, 0, 0, 0, 0, 0 },     /* SUB: root-fifth-octave */
    { -5, -2, 0, 3, 5, 7, 10, 12, 0, 0, 0 },   /* REESE: slow detuned intervals */
    { -5, -3, 0, 3, 4, 7, 9, 12, 0, 0, 0 },    /* FM: leaps and thirds */
    { -2, -1, 0, 1, 2, 3, 0, 0, 0, 0, 0 },     /* FOLD: tight chromatic cluster */
    { -5, 0, 3, 5, 7, 10, 12, 0, 0, 0, 0 },    /* SYNC: call and response */
};
static const uint8_t BGEN_POOL_N[BASS_ERA_N] = { 11, 4, 8, 8, 6, 7 };
static const int8_t BGEN_WALK[BASS_ERA_N][BGEN_WALK_MAX] = {
    { -1, 1, 1, -1, 0, 2, -2 },
    { 1, -1, 2, -2, 0, 0, 1 },
    { 1, -1, 2, -2, 0, 0, 0 },
    { 1, -2, 2, -1, 3, -3, 0 },
    { 1, -1, 1, -1, 0, 0, 0 },
    { 1, -1, 2, -2, 0, 0, 0 },
};
static const uint8_t BGEN_WALK_N[BASS_ERA_N] = { 7, 7, 5, 6, 5, 5 };
static const int8_t BGEN_PROG[BASS_ERA_N][8] = {
    { 0, 0, 3, 0, -2, 0, 3, -2 },
    { 0, 0, 7, 0, 0, -5, 0, 7 },
    { 0, 0, -2, -2, 3, 3, 0, 0 },
    { 0, 3, 7, 3, 0, -3, 0, 7 },
    { 0, 0, 1, 1, 0, 0, -1, 0 },
    { 0, 5, 7, 5, 0, 3, 7, 0 },
};

/* Rhythm, port of tools/gen_bass_pack.py (ARCHETYPE_CORE / _ALT, ARCHETYPE_SYNC, ARCHETYPE_OPT, bar_gate16), plus
 * archetypes 6..11 (C only). Archetype per plate from the era's pool; core variant per plate; optional onsets per
 * bar. Masks are step bits (bit s = step s); OPT has no BAN steps (1, 5, 9, 15), so the hard rules hold without a
 * later fix-up. */
#define BGEN_ARCH_N 12u
static const uint16_t BGEN_CORE[BGEN_ARCH_N][3] = {
    { 0x0101 },                   /* one_drop:       0 8 */
    { 0x0181, 0x0141, 0x0481 },   /* root_fifth:     0 7 8 | 0 6 8 | 0 7 10 */
    { 0x1101, 0x0501, 0x1041 },   /* octave_jump:    0 8 12 | 0 8 10 | 0 6 12 */
    { 0x0949, 0x0929 },           /* syncopated_332: 0 3 6 8 11 | 0 3 5 8 11 */
    { 0x1011, 0x0811 },           /* halftime:       0 4 12 | 0 4 11 */
    { 0x2145, 0x2245, 0x2149 },   /* walking_sparse: 0 2 6 8 13 | 0 2 6 9 13 | 0 3 6 8 13 */
    { 0x1111, 0x4111 },           /* four_floor:     0 4 8 12 | 0 4 8 14 */
    { 0x1049, 0x2449, 0x4849 },   /* dotted:         0 3 6 12 | 0 3 6 10 13 | 0 3 6 11 14 */
    { 0x0D0D, 0x1919 },           /* gallop:         0 2 3 8 10 11 | 0 3 4 8 11 12 */
    { 0x4081, 0x4041, 0x4901 },   /* push:           0 7 14 | 0 6 14 | 0 8 11 14 */
    { 0x7101, 0x7041 },           /* roll_end:       0 8 12 13 14 | 0 6 12 13 14 */
    { 0x4445, 0x1445 },           /* offbeat:        0 2 6 10 14 | 0 2 6 10 12 */
};
static const uint8_t BGEN_CORE_N[BGEN_ARCH_N] = { 1, 3, 3, 2, 2, 3, 2, 3, 2, 3, 2, 2 };
static const uint16_t BGEN_SYNC[BGEN_ARCH_N] = { 0x4440, 0x0440, 0x4040, 0x4400, 0x4400, 0x4410,
                                                 0x4000, 0x2800, 0x0808, 0x0C00, 0x2000, 0x0044 };
static const uint16_t BGEN_OPT[BGEN_ARCH_N] = { 0x5410, 0x4C18, 0x4450, 0x6000, 0x4440, 0x4C10,
                                                0x4444, 0x4410, 0x4040, 0x0C18, 0x0418, 0x0808 };
static const uint8_t BGEN_OPT_MAX[BGEN_ARCH_N] = { 1, 2, 2, 2, 2, 3, 1, 1, 1, 2, 1, 2 };
static const int8_t BGEN_OPT_BIAS[BASS_ERA_N] = { 1, 0, 1, -1, 0, 1 };   /* per era: optional-hit count shift */
/* Archetypes an era never plays (bit = archetype); a plate drawing one takes the next one the era allows, so the
 * same n stays related across eras while each era keeps its feel. */
#define BGA(x) (1u << (x))
static const uint16_t BGEN_ARCH_BAN[BASS_ERA_N] = {
    BGA(0) | BGA(4) | BGA(6),    /* ACID: no sparse / straight feels */
    BGA(3) | BGA(8) | BGA(11),   /* SUB: no busy feels */
    BGA(6) | BGA(8) | BGA(11),   /* REESE: halftime, no 4/4 or gallop */
    BGA(0) | BGA(4) | BGA(10),   /* FM: bouncy, no long gaps */
    BGA(0) | BGA(4) | BGA(7),    /* FOLD: tight, mechanical */
    BGA(0) | BGA(6) | BGA(10),   /* SYNC: syncopated, off the grid */
};
#undef BGA

typedef struct {
    int32_t n;
    uint8_t era, comply, slot;
    uint8_t rom;          /* pitch/len/bend are the curated line's (curated weight >= 1/2) */
    uint16_t bar[8];
    int8_t pitch[128];    /* endless only: melodic pitch per bar * 16 + step (0 = root / no move) */
} bass_phrase_t;

static inline uint32_t bass_gen_gkey(uint32_t gene)
{
    return fm_pgen_gkey(&BASS_GEN_DEF, gene);
}

static inline const bass_gen_phrase_genome_t *bass_gen_phrase_ge(const bass_phrase_t *ph, uint32_t gene)
{
    return &BASS_GEN_PHRASE[fm_pgen_gene_era(bass_gen_gkey(gene), ph->n, ph->era, BASS_ERA_N, ph->comply)];
}

static inline uint8_t bass_gen_popc16(uint16_t m)
{
    uint8_t c = 0u;

    while (m) {
        m &= (uint16_t)(m - 1u);
        c++;
    }
    return c;
}

/* Too busy: drop the least likely extra step first (ties: the later step). */
static inline uint16_t bass_gen_bar_cap(uint16_t m, const uint8_t *d)
{
    while (bass_gen_popc16(m) > BGEN_BAR_HITS_MAX) {
        uint8_t s, best = 0xFFu;

        for (s = 0u; s < 16u; s++)
            if (((m >> s) & 1u) && !((BGEN_BAR_MUST >> s) & 1u) && (best == 0xFFu || d[s] <= d[best]))
                best = s;
        if (best == 0xFFu)
            break;
        m &= (uint16_t)~(1u << best);
    }
    return m;
}

/* Noise draw for one pitch/rhythm decision: sub selects the cell (bar * 16 + step, or bar). */
static inline uint8_t bass_gen_nz(uint32_t gene, uint32_t sub, int32_t n, uint8_t period)
{
    return fm_pgen_noise(fm_pgen_hash(bass_gen_gkey(gene), sub), n, period);
}

/* 0..m-1 from a 0..255 draw; monotonic, so small n changes move the index by at most one step. */
static inline uint8_t bass_gen_scale(uint8_t v, uint8_t m)
{
    return (uint8_t)(((uint32_t)v * m) >> 8);
}

/* Index of the pool entry closest to tgt (first one on ties, like Python min()). */
static inline uint8_t bass_gen_nearest(const int8_t *pool, uint8_t pn, int32_t tgt)
{
    uint8_t i, best = 0u;
    int32_t bd = 0x7FFFFFFF;

    for (i = 0u; i < pn; i++) {
        int32_t d = (int32_t)pool[i] - tgt;

        if (d < 0)
            d = -d;
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

/* Chord anchors: pool entries whose mod-12 class is a chord tone of chord (fallback: nearest pool entry). */
static inline uint8_t bass_gen_anchors(uint8_t pe, int32_t chord, int8_t *out)
{
    uint8_t i, c = 0u, tones = BGEN_CHORD_TONES(pe);

    for (i = 0u; i < BGEN_POOL_N[pe]; i++) {
        int32_t cls = ((int32_t)BGEN_POOL[pe][i] - chord) % 12;

        if (cls < 0)
            cls += 12;
        if ((tones >> cls) & 1u)
            out[c++] = BGEN_POOL[pe][i];
    }
    if (!c)
        out[c++] = BGEN_POOL[pe][bass_gen_nearest(BGEN_POOL[pe], BGEN_POOL_N[pe], chord)];
    return c;
}

static inline uint8_t bass_gen_pidx(const int8_t *pool, uint8_t pn, int32_t v)
{
    uint8_t i;

    for (i = 0u; i < pn; i++)
        if (pool[i] == v)
            return i;
    return bass_gen_nearest(pool, pn, v);
}

/* Fill ph->pitch[] for a non-landmark phrase (bars already final). Same walk as line_pitches128: first hit of a
 * bar = chord anchor (nearest to the walk, 1 in 5 the farthest reachable one); middle hits walk the pool; last hit
 * of a bar approaches the next chord root (SYNC: fixed call/response targets). Only gated cells are set. */
static inline void bass_gen_pitch_build(bass_phrase_t *ph)
{
    uint8_t pe = fm_pgen_gene_era(bass_gen_gkey(BGEN_G_PERA), ph->n, ph->era, BASS_ERA_N, ph->comply);
    const int8_t *pool = BGEN_POOL[pe];
    uint8_t pn = BGEN_POOL_N[pe], wn = BGEN_WALK_N[pe], off, b, i;
    int32_t idx = -1;

    off = fm_pgen_pick(bass_gen_gkey(BGEN_G_POFF), ph->n, 64u, 0xFFu);
    for (i = 0u; i < 128u; i++)
        ph->pitch[i] = 0;
    for (b = 0u; b < 8u; b++) {
        int32_t chord = BGEN_PROG[pe][(b + off) & 7u];
        int32_t nxt = BGEN_PROG[pe][(b + 1u + off) & 7u];
        uint8_t hits[16], nh = 0u, k;

        for (i = 0u; i < 16u; i++)
            if ((ph->bar[b] >> i) & 1u)
                hits[nh++] = i;
        for (k = 0u; k < nh; k++) {
            uint8_t cell = (uint8_t)(b * 16u + hits[k]);
            int32_t pick;

            if (k == 0u || idx < 0) {
                int8_t cand[BGEN_POOL_MAX];
                uint8_t cn = bass_gen_anchors(pe, chord, cand), j, rn;

                if (idx < 0) {
                    pick = cand[bass_gen_scale(bass_gen_nz(BGEN_G_PANCH, cell, ph->n, 3u), cn)];
                } else {
                    int32_t prev = pool[idx];

                    for (j = 1u; j < cn; j++) {  /* sort by (|v - prev|, v) */
                        int8_t t = cand[j];
                        uint8_t q = j;

                        while (q > 0u) {
                            int32_t da = (int32_t)t - prev, db = (int32_t)cand[q - 1u] - prev;
                            if (da < 0) da = -da;
                            if (db < 0) db = -db;
                            if (da > db || (da == db && t >= cand[q - 1u]))
                                break;
                            cand[q] = cand[q - 1u];
                            q--;
                        }
                        cand[q] = t;
                    }
                    rn = 0u;  /* reachable (|v - prev| <= 12) entries are a prefix of the sorted list */
                    while (rn < cn && (int32_t)cand[rn] - prev <= 12 && prev - (int32_t)cand[rn] <= 12)
                        rn++;
                    if (!rn)
                        rn = cn;
                    pick = bass_gen_nz(BGEN_G_PREACH, cell, ph->n, 3u) < 52u ? cand[rn - 1u] : cand[0];
                }
                idx = bass_gen_pidx(pool, pn, pick);
            } else if (k == nh - 1u && (b < 7u || pe == BGEN_CALL_RESP_ERA)) {
                int32_t tgt;
                uint8_t tidx;

                if (pe == BGEN_CALL_RESP_ERA && b == 3u)
                    tgt = 7;
                else if (pe == BGEN_CALL_RESP_ERA && b == 7u)
                    tgt = 0;
                else
                    tgt = pool[bass_gen_nearest(pool, pn, nxt)];
                tidx = bass_gen_nearest(pool, pn, tgt);
                idx += (tidx > idx) - (tidx < idx);
            } else {
                int32_t d = BGEN_WALK[pe][bass_gen_scale(bass_gen_nz(BGEN_G_PWALK, cell, ph->n, 3u), wn)], ni;

                if (BGEN_LEAP_ERA(pe) && bass_gen_nz(BGEN_G_PLEAP, cell, ph->n, 3u) < 28u)
                    d *= 2;
                ni = idx + d;
                if (ni < 0 || ni >= pn)  /* bounce off the register edge */
                    ni = idx - d;
                idx = ni < 0 ? 0 : ni >= pn ? pn - 1 : ni;
            }
            {
                int32_t v = pool[idx];

                ph->pitch[cell] = (int8_t)(v < BGEN_PITCH_LO ? BGEN_PITCH_LO : v > BGEN_PITCH_HI ? BGEN_PITCH_HI : v);
            }
        }
    }
}

/* Archetype per plate (Python: pick_archetype, here a noise draw over n), moved off the era's banned feels; low
 * comply borrows a neighbour era's bans. */
static inline uint8_t bass_gen_arch(uint8_t era, int32_t n, uint8_t comply)
{
    uint32_t key = bass_gen_gkey(BGEN_G_ARCH);
    uint16_t ban = BGEN_ARCH_BAN[fm_pgen_gene_era(key, n, era, BASS_ERA_N, comply)];
    uint8_t a = fm_pgen_pick(key, n, 16u, (1u << BGEN_ARCH_N) - 1u), i;

    for (i = 0u; i < BGEN_ARCH_N && ((ban >> a) & 1u); i++)
        a = (uint8_t)((a + 1u) % BGEN_ARCH_N);
    return a;
}

/* Optional onsets for one bar: want = 0..OPT_MAX (era-shifted), the lowest-drawn steps of OPT win. */
static inline uint16_t bass_gen_opt_hits(uint8_t arch, uint8_t era, int32_t n, uint8_t bar)
{
    uint16_t om = BGEN_OPT[arch], m = 0u;
    int32_t want = ((int32_t)bass_gen_nz(BGEN_G_OPTW, bar, n, 8u) * 4) >> 8;
    uint8_t r, s;

    want += BGEN_OPT_BIAS[era];
    if (want < 0)
        want = 0;
    if (want > BGEN_OPT_MAX[arch])
        want = BGEN_OPT_MAX[arch];
    for (r = 0u; r < want; r++) {
        uint8_t best = 16u;
        uint16_t bestv = 256u;

        for (s = 0u; s < 16u; s++) {
            uint16_t v;

            if (!((om >> s) & 1u) || ((m >> s) & 1u))
                continue;
            v = bass_gen_nz(BGEN_G_OPTS, (uint32_t)bar * 16u + s, n, 8u);
            if (best == 16u || v < bestv) {
                best = s;
                bestv = v;
            }
        }
        if (best == 16u)
            break;
        m |= (uint16_t)(1u << best);
    }
    return m;
}

/* Too busy for the archetype: drop the highest sync step first, then the highest step (never step 0). */
static inline void bass_gen_drop_one(uint16_t *m, uint8_t arch)
{
    uint16_t sm = (uint16_t)(*m & BGEN_SYNC[arch]);
    int8_t s;

    if (sm) {
        for (s = 15; !((sm >> s) & 1u); s--)
            ;
        *m = (uint16_t)(*m & ~(1u << s));
        return;
    }
    for (s = 15; s >= 1; s--)
        if ((*m >> s) & 1u) {
            *m = (uint16_t)(*m & ~(1u << s));
            return;
        }
}

/* One bar of the archetype rhythm (Python: bar_gate16). Core variant and archetype are per plate, the rest per bar. */
static inline uint16_t bass_gen_bar_gate(uint8_t arch, uint8_t era, int32_t n, uint8_t bar)
{
    uint8_t cv = fm_pgen_pick(bass_gen_gkey(BGEN_G_CORE), n, 16u, (1u << BGEN_CORE_N[arch]) - 1u);
    uint16_t m = (uint16_t)(BGEN_CORE[arch][cv] | BGEN_BAR_MUST);

    m |= bass_gen_opt_hits(arch, era, n, bar);
    if ((bar == 3u || bar == 7u) && bass_gen_nz(BGEN_G_SPARSE, bar, n, 8u) < 64u)
        m &= 0x0101u;
    else if ((bar == 2u || bar == 4u || bar == 6u) && bass_gen_nz(BGEN_G_SPARSE, 0x100u + bar, n, 8u) < 128u)
        m &= (uint16_t)(0x0101u | (1u << ((era & 1u) ? 10u : 6u)));
    while (bass_gen_popc16(m) > 6u)
        bass_gen_drop_one(&m, arch);
    if (bass_gen_nz(BGEN_G_MIN, bar, n, 8u) < 128u) {
        static const uint8_t fill[5] = { 6, 10, 14, 4, 12 };

        while (bass_gen_popc16(m) < 3u) {
            uint8_t i, added = 0u;

            for (i = 0u; i < 5u && !added; i++) {
                uint8_t s = fill[(i + arch) % 5u];   /* archetype picks where the fill starts */

                if (!((m >> s) & 1u) && bass_gen_popc16(m) < 6u) {
                    m |= (uint16_t)(1u << s);
                    added = 1u;
                }
            }
            if (!added)
                break;
        }
    }
    return m;
}

/* Promoted seeds: 2 endless seeds per era, fixed as named plates. A plate with this (era, n) shows the name and uses
 * the frozen bar gates and pitch line below (frozen from the generator at promotion). len/bend and the sound genes
 * still follow n. Found by (era, n) in every path (phrase build, live name, fader line name). */
#define BASS_PROMO_N 12u

typedef struct {
    uint8_t era;
    int32_t n;
    char name[9];
    uint16_t bar[8];
    int8_t pitch[128];    /* bar * 16 + step, 0 = no move */
} bass_promo_t;

static const bass_promo_t BASS_PROMO[BASS_PROMO_N] = {
    { 0, 2459, "PLUCK",
      { 0x0549u, 0x4541u, 0x0141u, 0x0951u, 0x0141u, 0x0549u, 0x0951u, 0x0D41u },
      {
        3, 0, 0, 3, 0, 0, 5, 0, 7, 0, 5, 0, 0, 0, 0, 0,  /* bar 1 */
        -2, 0, 0, 0, 0, 0, -1, 0, 1, 0, -1, 0, 0, 0, 0, 0,  /* bar 2 */
        12, 0, 0, 0, 0, 0, 12, 0, 10, 0, 0, 0, 0, 0, 0, 0,  /* bar 3 */
        10, 0, 0, 0, 7, 0, 8, 0, 8, 0, 0, 7, 0, 0, 0, 0,  /* bar 4 */
        5, 0, 0, 0, 0, 0, 7, 0, 5, 0, 0, 0, 0, 0, 0, 0,  /* bar 5 */
        12, 0, 0, 10, 0, 0, 12, 0, 10, 0, 8, 0, 0, 0, 0, 0,  /* bar 6 */
        7, 0, 0, 0, 8, 0, 12, 0, 10, 0, 0, 8, 0, 0, 0, 0,  /* bar 7 */
        7, 0, 0, 0, 0, 0, 5, 0, 3, 0, 2, 3, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 0, 2074, "KNIFE",
      { 0x2909u, 0x2909u, 0x4909u, 0x4909u, 0x2909u, 0x2909u, 0x0101u, 0x0101u },
      {
        7, 0, 0, 3, 0, 0, 0, 0, 2, 0, 0, 2, 0, 3, 0, 0,  /* bar 1 */
        3, 0, 0, 5, 0, 0, 0, 0, 8, 0, 0, 12, 0, 10, 0, 0,  /* bar 2 */
        10, 0, 0, 12, 0, 0, 0, 0, 12, 0, 0, 10, 0, 0, 8, 0,  /* bar 3 */
        7, 0, 0, 7, 0, 0, 0, 0, 7, 0, 0, 8, 0, 0, 7, 0,  /* bar 4 */
        7, 0, 0, 7, 0, 0, 0, 0, 3, 0, 0, 5, 0, 3, 0, 0,  /* bar 5 */
        3, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 0, 0,  /* bar 6 */
        0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0,  /* bar 7 */
        -2, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 1, 2058, "HUSH",
      { 0x4C81u, 0x0C81u, 0x0491u, 0x0441u, 0x0491u, 0x0481u, 0x4489u, 0x0489u },
      {
        3, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 3, 0, 0, 3, 0,  /* bar 1 */
        3, 0, 0, 0, 0, 0, 0, 5, 0, 0, 3, 2, 0, 0, 0, 0,  /* bar 2 */
        10, 0, 0, 0, 7, 0, 0, 7, 0, 0, 5, 0, 0, 0, 0, 0,  /* bar 3 */
        12, 0, 0, 0, 0, 0, 10, 0, 0, 0, 8, 0, 0, 0, 0, 0,  /* bar 4 */
        7, 0, 0, 0, 8, 0, 0, 8, 0, 0, 7, 0, 0, 0, 0, 0,  /* bar 5 */
        -2, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 6 */
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -2, 0,  /* bar 7 */
        -2, 0, 0, -1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 1, 2055, "LULL",
      { 0x0C89u, 0x0C81u, 0x0491u, 0x0441u, 0x0441u, 0x0481u, 0x4489u, 0x0489u },
      {
        -2, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 1 */
        12, 0, 0, 0, 0, 0, 0, 8, 0, 0, 5, 3, 0, 0, 0, 0,  /* bar 2 */
        3, 0, 0, 0, 5, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0,  /* bar 3 */
        1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0,  /* bar 4 */
        3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0,  /* bar 5 */
        0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 6 */
        10, 0, 0, 8, 0, 0, 0, 7, 0, 0, 3, 0, 0, 0, 2, 0,  /* bar 7 */
        12, 0, 0, 10, 0, 0, 0, 10, 0, 0, 7, 0, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 2, 2163, "SHIMMER",
      { 0x5041u, 0x1451u, 0x5051u, 0x5051u, 0x0441u, 0x5051u, 0x0441u, 0x5051u },
      {
        12, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 10, 0, 7, 0,  /* bar 1 */
        7, 0, 0, 0, 12, 0, 12, 0, 0, 0, 7, 0, 5, 0, 0, 0,  /* bar 2 */
        3, 0, 0, 0, 5, 0, 3, 0, 0, 0, 0, 0, 7, 0, 5, 0,  /* bar 3 */
        3, 0, 0, 0, -2, 0, 3, 0, 0, 0, 0, 0, 5, 0, 3, 0,  /* bar 4 */
        10, 0, 0, 0, 0, 0, 5, 0, 0, 0, 3, 0, 0, 0, 0, 0,  /* bar 5 */
        5, 0, 0, 0, 10, 0, 7, 0, 0, 0, 0, 0, 7, 0, 5, 0,  /* bar 6 */
        3, 0, 0, 0, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 7 */
        -2, 0, 0, 0, -2, 0, -2, 0, 0, 0, 0, 0, -2, 0, -5, 0,  /* bar 8 */
      } },
    { 2, 2203, "BRAID",
      { 0x0C51u, 0x0C51u, 0x4C11u, 0x0441u, 0x0441u, 0x4811u, 0x0041u, 0x4851u },
      {
        3, 0, 0, 0, -2, 0, 3, 0, 0, 0, 7, 5, 0, 0, 0, 0,  /* bar 1 */
        3, 0, 0, 0, 7, 0, 12, 0, 0, 0, 12, 10, 0, 0, 0, 0,  /* bar 2 */
        12, 0, 0, 0, 7, 0, 0, 0, 0, 0, 12, 10, 0, 0, 7, 0,  /* bar 3 */
        7, 0, 0, 0, 0, 0, 5, 0, 0, 0, 3, 0, 0, 0, 0, 0,  /* bar 4 */
        3, 0, 0, 0, 0, 0, 7, 0, 0, 0, 5, 0, 0, 0, 0, 0,  /* bar 5 */
        3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 7, 0, 0, 5, 0,  /* bar 6 */
        5, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 7 */
        10, 0, 0, 0, 10, 0, 5, 0, 0, 0, 0, 0, 0, 0, -5, 0,  /* bar 8 */
      } },
    { 3, 2022, "PRISM",
      { 0x0909u, 0x2909u, 0x0101u, 0x0141u, 0x0909u, 0x0909u, 0x0101u, 0x0909u },
      {
        0, 0, 0, 3, 0, 0, 0, 0, -5, 0, 0, -3, 0, 0, 0, 0,  /* bar 1 */
        -3, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 12, 0, 9, 0, 0,  /* bar 2 */
        7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0,  /* bar 3 */
        7, 0, 0, 0, 0, 0, 12, 0, 9, 0, 0, 0, 0, 0, 0, 0,  /* bar 4 */
        7, 0, 0, 12, 0, 0, 0, 0, 4, 0, 0, 3, 0, 0, 0, 0,  /* bar 5 */
        3, 0, 0, 9, 0, 0, 0, 0, 3, 0, 0, 4, 0, 0, 0, 0,  /* bar 6 */
        -5, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0, 0, 0, 0, 0,  /* bar 7 */
        7, 0, 0, 4, 0, 0, 0, 0, 3, 0, 0, 7, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 3, 2093, "CHIME",
      { 0x0541u, 0x0141u, 0x0141u, 0x0141u, 0x0541u, 0x0941u, 0x0141u, 0x0149u },
      {
        4, 0, 0, 0, 0, 0, 12, 0, 9, 0, 7, 0, 0, 0, 0, 0,  /* bar 1 */
        7, 0, 0, 0, 0, 0, 4, 0, 3, 0, 0, 0, 0, 0, 0, 0,  /* bar 2 */
        3, 0, 0, 0, 0, 0, 4, 0, 3, 0, 0, 0, 0, 0, 0, 0,  /* bar 3 */
        3, 0, 0, 0, 0, 0, 9, 0, 7, 0, 0, 0, 0, 0, 0, 0,  /* bar 4 */
        7, 0, 0, 0, 0, 0, 3, 0, 9, 0, 7, 0, 0, 0, 0, 0,  /* bar 5 */
        7, 0, 0, 0, 0, 0, 9, 0, 4, 0, 0, 3, 0, 0, 0, 0,  /* bar 6 */
        3, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0, 0, 0, 0, 0,  /* bar 7 */
        -3, 0, 0, 3, 0, 0, -3, 0, -5, 0, 0, 0, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 4, 2664, "CREASE",
      { 0x0141u, 0x0549u, 0x0941u, 0x0149u, 0x0541u, 0x0149u, 0x4149u, 0x0151u },
      {
        10, 0, 0, 0, 0, 0, 12, 0, 10, 0, 0, 0, 0, 0, 0, 0,  /* bar 1 */
        12, 0, 0, 12, 0, 0, 12, 0, 10, 0, 7, 0, 0, 0, 0, 0,  /* bar 2 */
        7, 0, 0, 0, 0, 0, 7, 0, 7, 0, 0, 5, 0, 0, 0, 0,  /* bar 3 */
        5, 0, 0, 3, 0, 0, 5, 0, 7, 0, 0, 0, 0, 0, 0, 0,  /* bar 4 */
        7, 0, 0, 0, 0, 0, 7, 0, -5, 0, 0, 0, 0, 0, 0, 0,  /* bar 5 */
        0, 0, 0, 3, 0, 0, 7, 0, 5, 0, 0, 0, 0, 0, 0, 0,  /* bar 6 */
        3, 0, 0, -5, 0, 0, 7, 0, 5, 0, 0, 0, 0, 0, 3, 0,  /* bar 7 */
        3, 0, 0, 0, 5, 0, 10, 0, 7, 0, 0, 0, 0, 0, 0, 0,  /* bar 8 */
      } },
    { 4, 2054, "KINK",
      { 0x1051u, 0x1441u, 0x1051u, 0x0441u, 0x0441u, 0x1051u, 0x5441u, 0x1441u },
      {
        0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, -3, 0, 0, 0,  /* bar 1 */
        -3, 0, 0, 0, 0, 0, 4, 0, 0, 0, 12, 0, 9, 0, 0, 0,  /* bar 2 */
        7, 0, 0, 0, 3, 0, 7, 0, 0, 0, 0, 0, 7, 0, 0, 0,  /* bar 3 */
        7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* bar 4 */
        0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0,  /* bar 5 */
        3, 0, 0, 0, 9, 0, 4, 0, 0, 0, 0, 0, 7, 0, 0, 0,  /* bar 6 */
        7, 0, 0, 0, 0, 0, 12, 0, 0, 0, 4, 0, 3, 0, 3, 0,  /* bar 7 */
        -5, 0, 0, 0, 0, 0, -3, 0, 0, 0, 4, 0, 12, 0, 0, 0,  /* bar 8 */
      } },
    { 5, 2049, "LOCKSTEP",
      { 0x5051u, 0x1441u, 0x5441u, 0x5441u, 0x0441u, 0x5441u, 0x1451u, 0x1451u },
      {
        -2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1, 0,  /* bar 1 */
        0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0,  /* bar 2 */
        3, 0, 0, 0, 0, 0, 7, 0, 0, 0, 8, 0, 12, 0, 10, 0,  /* bar 3 */
        10, 0, 0, 0, 0, 0, 7, 0, 0, 0, 8, 0, 10, 0, 8, 0,  /* bar 4 */
        7, 0, 0, 0, 0, 0, 5, 0, 0, 0, 3, 0, 0, 0, 0, 0,  /* bar 5 */
        3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 1, 0, 2, 0,  /* bar 6 */
        3, 0, 0, 0, 5, 0, 7, 0, 0, 0, 3, 0, 2, 0, 0, 0,  /* bar 7 */
        3, 0, 0, 0, 3, 0, 3, 0, 0, 0, 1, 0, -1, 0, 0, 0,  /* bar 8 */
      } },
    { 5, 2039, "ORBIT",
      { 0x2909u, 0x2909u, 0x2909u, 0x2909u, 0x0141u, 0x0141u, 0x2909u, 0x2909u },
      {
        2, 0, 0, 3, 0, 0, 0, 0, 5, 0, 0, 3, 0, 2, 0, 0,  /* bar 1 */
        3, 0, 0, 5, 0, 0, 0, 0, 3, 0, 0, 2, 0, 3, 0, 0,  /* bar 2 */
        3, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 0, 0,  /* bar 3 */
        1, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 0, 0,  /* bar 4 */
        0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0,  /* bar 5 */
        12, 0, 0, 0, 0, 0, 10, 0, 8, 0, 0, 0, 0, 0, 0, 0,  /* bar 6 */
        7, 0, 0, 8, 0, 0, 0, 0, 7, 0, 0, 5, 0, 3, 0, 0,  /* bar 7 */
        3, 0, 0, 5, 0, 0, 0, 0, 5, 0, 0, 3, 0, 3, 0, 0,  /* bar 8 */
      } },
};

static inline const bass_promo_t *bass_promo_find(uint8_t era, int32_t n)
{
    uint8_t i;

    for (i = 0u; i < BASS_PROMO_N; i++)
        if (BASS_PROMO[i].era == era && BASS_PROMO[i].n == n)
            return &BASS_PROMO[i];
    return NULL;
}

static inline void bass_phrase_build(bass_phrase_t *ph, uint8_t era, int32_t n, uint8_t comply)
{
    const uint16_t *rom;
    uint32_t kx = bass_gen_gkey(BGEN_G_BLEND);
    const bass_gen_phrase_genome_t *ge;
    uint16_t w, raw[8];
    uint8_t k, b, arch;

    if (era >= BASS_ERA_N)
        era = (uint8_t)(BASS_ERA_N - 1u);
    w = fm_pgen_landmark(n, &k);
    ph->n = n;
    ph->era = era;
    ph->comply = comply;
    ph->slot = (uint8_t)(era * GENRE_PRESETS_PER_ERA + k);
    ph->rom = w >= FM_PGEN_W_ONE / 2u;
    rom = BASS_LINE_BAR_GATE[ph->slot];
    if (w >= FM_PGEN_W_ONE) {
        for (b = 0u; b < 8u; b++)
            ph->bar[b] = rom[b];
        return;
    }
    arch = bass_gen_arch(era, n, comply);
    for (b = 0u; b < 8u; b++) {
        if (b && (b & 1u) && bass_gen_nz(BGEN_G_REP, b, n, 8u) < 64u)
            raw[b] = raw[b - 1u];   /* odd bars sometimes repeat the bar before */
        else
            raw[b] = bass_gen_bar_gate(arch, era, n, b);
    }
    ge = bass_gen_phrase_ge(ph, BGEN_G_BAR);
    for (b = 0u; b < 8u; b++) {
        uint16_t m = (uint16_t)((raw[b] | BGEN_BAR_MUST) & ~BGEN_BAR_BAN);

        m = bass_gen_bar_cap(m, ge->d);
        ph->bar[b] = (uint16_t)fm_pgen_blend_mask(m, rom[b], w, fm_pgen_hash(kx, b));
    }
    if (!ph->rom) {
        const bass_promo_t *pr = bass_promo_find(era, n);

        bass_gen_pitch_build(ph);
        if (pr) {
            for (b = 0u; b < 8u; b++)
                ph->bar[b] = pr->bar[b];
            for (k = 0u; k < 128u; k++)
                ph->pitch[k] = pr->pitch[k];
        }
    }
}

static inline uint8_t bass_phrase_gate(const bass_phrase_t *ph, uint8_t st)
{
    return (uint8_t)((ph->bar[(st >> 4) & 7u] >> (st & 15u)) & 1u);
}

static inline int8_t bass_phrase_pitch(const bass_phrase_t *ph, uint8_t st)
{
    st &= 127u;
    if (ph->rom)
        return BASS_LINE_LEAD_PITCH[ph->slot][st];
    return ph->pitch[st];
}

static inline uint8_t bass_phrase_len(const bass_phrase_t *ph, uint8_t st)
{
    const bass_gen_phrase_genome_t *ge;
    uint8_t pos = st & 15u, v;

    st &= 127u;
    if (ph->rom)
        return BASS_LINE_LEAD_LEN[ph->slot][st];
    if (!bass_phrase_gate(ph, st))
        return 0u;
    if ((pos != 7u && pos != 14u) || fm_pgen_noise(fm_pgen_hash(bass_gen_gkey(BGEN_G_LONG), st), ph->n, 3u) >= 230u)
        return 4u;
    ge = bass_gen_phrase_ge(ph, BGEN_G_LVAL);
    v = fm_pgen_noise(fm_pgen_hash(bass_gen_gkey(BGEN_G_LVAL), st), ph->n, 3u);
    return v < ge->long9 ? 9u : v < ge->long10 ? 10u : 11u;
}

static inline uint8_t bass_phrase_bend(const bass_phrase_t *ph, uint8_t st)
{
    uint8_t pos = st & 15u;

    st &= 127u;
    if (ph->rom)
        return BASS_LINE_LEAD_BEND[ph->slot][st];
    if (pos == 7u)
        return fm_pgen_noise(fm_pgen_hash(bass_gen_gkey(BGEN_G_BEND7), st), ph->n, 3u) < 230u ?
                   bass_gen_phrase_ge(ph, BGEN_G_BEND7)->bend7 : 0u;
    if (pos == 15u && st < 112u)
        return fm_pgen_noise(fm_pgen_hash(bass_gen_gkey(BGEN_G_BENDV), st), ph->n, 3u) < 128u ? 5u : 6u;
    return 0u;
}

/* Live generated plate (endless continuum); shadows the ROM line at bass_gen.slot. */
static fm_pgen_t bass_gen = { .slot = 0xFFu };
static bass_phrase_t bass_gen_ph;

#define FM_PGEN_STATE bass_gen
#define FM_PGEN_BUILD(era, n, c) bass_gen_build(era, n, c)

static inline uint8_t bass_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&bass_gen, ix);
}

static inline void bass_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    if (era >= BASS_ERA_N)
        era = (uint8_t)(BASS_ERA_N - 1u);
    fm_pgen_build(&bass_gen, NULL, &BASS_GEN_DEF, era, n, comply);
    {
        const bass_promo_t *pr = bass_promo_find(era, n);
        uint8_t i;

        if (pr)
            for (i = 0u; i < 9u; i++)
                bass_gen.name[i] = pr->name[i];
    }
    bass_phrase_build(&bass_gen_ph, era, n, comply);
    bass_gen.pl.sub = BGEN_M(bass_gen_ph.bar[0], bass_gen_ph.bar[1]);
    bass_gen.views_live = 0u;
}

/* n of era line `line` for the genre fader: it steps along n by FM_PGEN_GAP around the live plate, so on a
 * landmark (and with the continuum off) every fader line is the curated line. */
static inline int32_t bass_gen_line_n(uint8_t era, uint8_t line, uint8_t *comply)
{
    int32_t k = (int32_t)(line % GENRE_PRESETS_PER_ERA);

    if (bass_gen.slot != 0xFFu && bass_gen_ph.era == era) {
        *comply = bass_gen_ph.comply;
        return bass_gen_ph.n + (k - (int32_t)(bass_gen_ph.slot % GENRE_PRESETS_PER_ERA)) * FM_PGEN_GAP;
    }
    *comply = FM_PGEN_COMPLY_DEF;
    return fm_pgen_landmark_n((uint8_t)k);
}

static inline void bass_gen_line_phrase(bass_phrase_t *ph, uint8_t era, uint8_t line)
{
    uint8_t c;
    int32_t n = bass_gen_line_n(era, line, &c);

    if (bass_gen.slot != 0xFFu && bass_gen_ph.era == era && n == bass_gen_ph.n)
        *ph = bass_gen_ph;
    else
        bass_phrase_build(ph, era, n, c);
}

static inline const char *bass_gen_line_name(uint8_t era, uint8_t line)
{
    static char nm[9];
    uint8_t c;
    int32_t n = bass_gen_line_n(era, line, &c);

    const bass_promo_t *pr;

    if (fm_pgen_landmark(n, NULL) >= FM_PGEN_W_ONE)
        return BASS_LINE[line].name;
    pr = bass_promo_find(era, n);
    if (pr)
        return pr->name;
    fm_pgen_name(nm, bass_gen_gkey(FM_PGEN_G_NAME), n, BASS_GEN_DEF.words, BASS_GEN_DEF.word_n);
    return nm;
}

#endif
