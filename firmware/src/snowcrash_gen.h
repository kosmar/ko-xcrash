/* SPDX-License-Identifier: GPL-3.0-only */
/* KO XCRASH endless plates: interference-groove genome + wiring for fm_pgen_core.h. Included at the end of
 * snowcrash_data.h. Every lane is its own p-step pattern (p from SNOWCRASH_LANE_PERIOD, 5/7/9/11) tiled over the
 * 32-step loop; the periods drift against each other and that drift is the groove. snowcrash_gen_build() keeps
 * the tiling: trim/fill works on the pattern, never on single steps. */
#ifndef SNOWCRASH_GEN_H
#define SNOWCRASH_GEN_H

#include "fm_pgen_core.h"

#define SNOWCRASH_GEN_KEY 0x534E4F32u

#define SGB(x) FM_PGEN_B(x)
#define SGEN_SCALES (SGB(0) | SGB(1) | SGB(2) | SGB(3) | SGB(4) | SGB(5))
#define SGEN_DOWNBEAT 0x00000001u /* kick on residue 0: every lane period starts with the downbeat */

/* Lanes: kick rim hat sub chord siren lead. Tempo 88…124 BPM, swing 58…70 (per era). */
static const fm_pgen_genome_t SNOWCRASH_GENOME[SNOWCRASH_ERA_N] = {
    /* CRASH */
    { 88, 96, 40, 48, 60, 66, 52, 64, 0, 60, 0, 40, 50, 66, 56, 72, 12, 22, 40, 48, 60,
      SGEN_SCALES, SGB(DKIT_CLICK) | SGB(DKIT_GLASS) | SGB(DKIT_RIME),
      SGB(0) | SGB(1) | SGB(2) | SGB(3) | SGB(5),
      SGB(0) | SGB(1) | SGB(2) | SGB(3) | SGB(4) | SGB(5) | SGB(6) | SGB(7) | SGB(8) | SGB(9),
      { 120, 100, 130, 120, 110, 120, 110 },
      { SGEN_DOWNBEAT }, { 0u }, 8, 28, -3, 6 },
    /* GLOSS */
    { 92, 100, 44, 52, 62, 70, 50, 62, 10, 70, 10, 50, 60, 80, 60, 80, 14, 26, 46, 46, 58,
      SGEN_SCALES, SGB(DKIT_CLICK) | SGB(DKIT_FOAM) | SGB(DKIT_RIME),
      SGB(0) | SGB(2) | SGB(3) | SGB(4) | SGB(5),
      SGB(1) | SGB(2) | SGB(3) | SGB(4) | SGB(5) | SGB(6) | SGB(7) | SGB(8) | SGB(9) | SGB(10),
      { 130, 120, 120, 128, 110, 120, 118 },
      { SGEN_DOWNBEAT }, { 0u }, 10, 30, -3, 6 },
    /* SUMER */
    { 100, 108, 46, 56, 58, 66, 56, 70, 0, 60, 20, 60, 48, 70, 64, 90, 16, 28, 50, 50, 62,
      SGEN_SCALES, SGB(DKIT_CLICK) | SGB(DKIT_WIRE) | SGB(DKIT_SLAG),
      SGB(1) | SGB(2) | SGB(3) | SGB(4) | SGB(5),
      SGB(2) | SGB(3) | SGB(4) | SGB(5) | SGB(6) | SGB(7) | SGB(8) | SGB(9) | SGB(10) | SGB(11),
      { 120, 110, 126, 118, 124, 112, 100 },
      { SGEN_DOWNBEAT }, { 0u }, 12, 32, -3, 6 },
    /* STATIC */
    { 104, 112, 40, 50, 60, 68, 52, 60, 0, 50, 0, 30, 44, 60, 50, 70, 10, 20, 40, 44, 56,
      SGEN_SCALES, SGB(DKIT_CLICK) | SGB(DKIT_SLAG) | SGB(DKIT_GLASS),
      SGB(0) | SGB(3) | SGB(4) | SGB(5) | SGB(6),
      SGB(0) | SGB(1) | SGB(2) | SGB(3) | SGB(4) | SGB(5) | SGB(6) | SGB(7) | SGB(8),
      { 125, 115, 120, 110, 118, 125, 120 },
      { SGEN_DOWNBEAT }, { 0u }, 6, 34, -3, 6 },
    /* TIDE */
    { 110, 118, 36, 46, 62, 72, 60, 72, 30, 80, 40, 80, 56, 80, 66, 96, 18, 30, 44, 52, 64,
      SGEN_SCALES, SGB(DKIT_HUM) | SGB(DKIT_FOAM) | SGB(DKIT_GLASS),
      SGB(0) | SGB(1) | SGB(4) | SGB(5),
      SGB(3) | SGB(4) | SGB(5) | SGB(6) | SGB(7) | SGB(8) | SGB(9) | SGB(10) | SGB(11),
      { 110, 100, 116, 124, 120, 112, 118 },
      { SGEN_DOWNBEAT }, { 0u }, 10, 36, -3, 6 },
    /* CHAPEL */
    { 116, 124, 50, 60, 56, 66, 56, 66, 24, 80, 0, 40, 70, 100, 80, 110, 20, 32, 36, 45, 58,
      SGEN_SCALES, SGB(DKIT_CLICK) | SGB(DKIT_RIME) | SGB(DKIT_GLASS) | SGB(DKIT_WIRE),
      SGB(2) | SGB(5) | SGB(6) | SGB(7) | SGB(8),
      SGB(4) | SGB(5) | SGB(6) | SGB(7) | SGB(8) | SGB(9) | SGB(10) | SGB(11),
      { 128, 118, 124, 110, 108, 120, 112 },
      { SGEN_DOWNBEAT }, { 0u }, 12, 38, -3, 6 },
};

/* Pattern popcount band per era and lane: [era][lane] = { kmin, kmax } (hits of the tiled lane = pcount()).
 * Derived from the generator (tools/gen_snowcrash_pack.py lane_band). Replaces the old 32-step hit window: a
 * pattern inside the band tiles to a hit count inside the era window, so the texture step cannot break the tiling. */
static const uint8_t SNOWCRASH_GEN_HITS[SNOWCRASH_ERA_N][FM_PGEN_L_N][2] = {
    { { 1, 2 }, { 3, 4 }, { 4, 7 }, { 3, 6 }, { 2, 2 }, { 1, 3 }, { 3, 4 } },
    { { 2, 2 }, { 4, 6 }, { 4, 6 }, { 2, 3 }, { 4, 7 }, { 3, 4 }, { 3, 4 } },
    { { 1, 2 }, { 3, 6 }, { 2, 4 }, { 2, 4 }, { 1, 3 }, { 1, 2 }, { 2, 4 } },
    { { 1, 2 }, { 2, 3 }, { 6, 9 }, { 2, 4 }, { 3, 5 }, { 3, 5 }, { 2, 3 } },
    { { 1, 2 }, { 3, 4 }, { 2, 4 }, { 3, 4 }, { 3, 6 }, { 1, 3 }, { 2, 2 } },
    { { 1, 2 }, { 4, 6 }, { 4, 7 }, { 2, 3 }, { 1, 2 }, { 2, 4 }, { 2, 3 } },
};

/* Floors/ceilings of the widened ranges (x_lo/x_hi only). */
static const fm_pgen_genome_t SNOWCRASH_GEN_LIM = {
    .dens_lo = 12, .dens_hi = 60, .swing_lo = 54, .swing_hi = 78, .pump_lo = 36, .pump_hi = 96,
    .tank_lo = 0, .tank_hi = 110, .wob_lo = 0, .wob_hi = 96, .dly_lo = 36, .dly_hi = 110,
    .rev_lo = 48, .rev_hi = 112, .fb_lo = 8, .fb_hi = 34, .root_lo = 45, .root_hi = 64,
};

/* Lead pitch is in semitones (step_tune), like KO TECHNO. */
static const int8_t SGEN_INTERVALS[8] = { 2, -2, 3, -3, 5, -5, 7, 12 };

/* 186 name words, NUL-separated, empty word ends the list (1207 bytes). */
static const char SNOWCRASH_GEN_WORDS[] =
    "DAEMON\0KATANA\0COURIER\0HACKER\0CODER\0RAFT\0LOGIC\0VIRUS\0GLYPH\0RUNE\0TABLET\0CLAY\0STYLUS\0"
    "ZIGGURAT\0TEMPLE\0TOWER\0ORACLE\0MANTRA\0ASHERAH\0MARDUK\0INANNA\0ENLIL\0ANU\0NEON\0CHROME\0GRID\0"
    "PIXEL\0VOXEL\0BITMAP\0KERNEL\0SHELL\0ROOT\0PROXY\0ROUTER\0PACKET\0SOCKET\0PORT\0HUB\0NODE\0LATTICE\0"
    "MESH\0FIBER\0OPTIC\0LASER\0GOGGLES\0VISOR\0SPRAWL\0STREET\0SKATER\0SKATE\0HARPOON\0GLASS\0SHARD\0BLADE\0"
    "SWORD\0ENCLAVE\0COMPOUND\0FREEWAY\0STATIC\0FLICKER\0GLITCH\0DATA\0CACHE\0CIPHER\0CRYPT\0TOKEN\0LEDGER\0"
    "ARRAY\0BUFFER\0HEAP\0QUEUE\0THREAD\0FORK\0SPAWN\0VIRAL\0TONGUE\0BABBLE\0GLOSSA\0LEXICON\0SCRIBE\0"
    "SCROLL\0ZERO\0BINARY\0AVENUE\0MOTORWAY\0BIKER\0MOPED\0SCOOTER\0PIZZA\0DELIVERY\0RUSH\0TIMER\0CLOCK\0"
    "DEADLINE\0PIXELATE\0RENDER\0SHADER\0POLYGON\0SPRITE\0SHADOW\0HOLOGRAM\0MIRROR\0LENS\0LIGHT\0BEAM\0"
    "PHOTON\0SILICON\0CIRCUIT\0CHIP\0WAFER\0DIODE\0RELAY\0SWITCH\0SIGNAL\0STREAM\0UPLINK\0DOWNLINK\0ORBIT\0"
    "ANTENNA\0DISH\0RADAR\0SONAR\0BEACON\0TRACE\0PROBE\0SCAN\0PING\0ECHO\0RELIC\0ARTIFACT\0RUIN\0DUST\0SAND\0"
    "DELTA\0TIGRIS\0RIVER\0MUD\0REED\0CLAYPIT\0KILN\0FURNACE\0FORGE\0ANVIL\0HAMMER\0BRONZE\0COPPER\0SILVER\0"
    "GOLD\0LAPIS\0OBSIDIAN\0BASALT\0GRANITE\0MARBLE\0PILLAR\0ARCH\0GATE\0VAULT\0BUNKER\0HATCH\0AIRLOCK\0"
    "HULL\0CARRIER\0TANKER\0CONVOY\0FLEET\0ARMADA\0REFUGEE\0EXODUS\0DRIFT\0CURRENT\0WAKE\0SPRAY\0SALT\0TIDE\0"
    "SHORE\0HARBOR\0DOCK\0PIER\0LANTERN\0SIGIL\0TOTEM\0IDOL\0EFFIGY\0SPELL\0CHARM\0VERSE\0";

static const fm_pgen_def_t SNOWCRASH_GEN_DEF = {
    .key = SNOWCRASH_GEN_KEY,
    .genome = SNOWCRASH_GENOME,
    .lim = &SNOWCRASH_GEN_LIM,
    .words = SNOWCRASH_GEN_WORDS,
    .word_n = 186u,
    .rom = SNOWCRASH_SLAB,
    .root = SNOWCRASH_SLAB_ROOT,
    .scale = SNOWCRASH_SLAB_SCALE,
    .swing = SNOWCRASH_SLAB_SWING,
    .rate = SNOWCRASH_SLAB_LEAD_RATE,
    .kit = SNOWCRASH_SLAB_KIT,
    .lead_gate = SNOWCRASH_SLAB_LEAD_GATE,
    .gate_cs = SNOWCRASH_SLAB_LEAD_GATE_CS,
    .lead_pitch = &SNOWCRASH_SLAB_LEAD_PITCH[0][0],
    .lead_len = &SNOWCRASH_SLAB_LEAD_LEN[0][0],
    .lead_bend = &SNOWCRASH_SLAB_LEAD_BEND[0][0],
    .intervals = SGEN_INTERVALS,
    .era_n = SNOWCRASH_ERA_N,
    .steps = 32u,
    .pitch_n = 16u,
    .lead_hi_echo = 1u,
};

#define FM_PGEN_DEF SNOWCRASH_GEN_DEF
#define FM_PGEN_LEAD_STATE snowcrash_gen_lead

static inline uint8_t snowcrash_gen_popc(uint32_t m)
{
    uint8_t c = 0u;

    while (m) {
        m &= m - 1u;
        c++;
    }
    return c;
}

/* Tile a p-step pattern over the 32-step loop: mask bit s = pattern bit (s mod p). */
static inline uint32_t snowcrash_gen_tile(uint32_t pat, uint8_t p)
{
    uint32_t m = 0u;
    uint8_t s;

    for (s = 0u; s < 32u; s++) {
        if ((pat >> (s % p)) & 1u)
            m |= 1u << s;
    }
    return m;
}

/* Lane texture. A lane that is already a tiled pattern inside the era band (curated landmark) is kept as it is.
 * Any other lane (generated plate, landmark blend) is rebuilt from a residue noise shared by all eras (same n gives
 * the same skeleton in every era, the periods make them differ): the hit target is drawn inside [kmin, kmax], the
 * residues with the lowest noise are on. The noise drifts with n like fm_pgen_lane().
 * Must/ban are folded to residues; kick keeps residue 0 as the downbeat anchor. Only patterns are touched, so the
 * tiling survives every step of the drift. */
static inline uint32_t snowcrash_gen_texture(uint32_t m, uint8_t lane, uint8_t p, uint8_t era, int32_t n,
                                             uint8_t kmin, uint8_t kmax, uint32_t must, uint32_t ban)
{
    static const uint8_t per[FM_PGEN_L_N] = { 6u, 6u, 6u, 5u, 5u, 5u, 3u };
    uint32_t key = fm_pgen_gkey(&SNOWCRASH_GEN_DEF, (uint32_t)FM_PGEN_G_KICK + lane);
    uint32_t pm = (1u << p) - 1u, pat = 0u, mp = 0u, bp = 0u, used = 0u;
    uint8_t c, r, s, want, i;

    for (s = 0u; s < 32u; s++) {
        if ((must >> s) & 1u)
            mp |= 1u << (s % p);
        if ((ban >> s) & 1u)
            bp |= 1u << (s % p);
    }
    if (snowcrash_gen_tile(m & pm, p) == m && snowcrash_gen_popc(m & pm) >= kmin
        && snowcrash_gen_popc(m & pm) <= kmax)
        return m;

    want = (uint8_t)(kmin + fm_pgen_noise(fm_pgen_hash(key, 31u), n, per[lane]) % (kmax - kmin + 1u));
    for (i = 0u; i < want; i++) {
        uint8_t best = 0xFFu, bv = 0u;

        for (r = 0u; r < p; r++) {
            uint8_t v;

            if ((used >> r) & 1u)
                continue;
            v = fm_pgen_noise(fm_pgen_hash(key, r), n, per[lane]);
            if (best == 0xFFu || v < bv) {
                best = r;
                bv = v;
            }
        }
        used |= 1u << best;
        pat |= 1u << best;
    }
    pat = ((pat & ~bp) | mp) & pm;
    if (lane == 0u)
        pat |= 1u;
    c = snowcrash_gen_popc(pat);
    while (c < kmin || c > kmax) {
        uint8_t add = c < kmin, best = 0xFFu, bv = 0u;

        for (r = 0u; r < p; r++) {
            uint8_t on = (uint8_t)((pat >> r) & 1u), v;

            if (add ? (on || ((bp >> r) & 1u)) : (!on || ((mp >> r) & 1u) || (lane == 0u && r == 0u)))
                continue;
            v = fm_pgen_noise(fm_pgen_hash(key, r), n, per[lane]);
            if (best == 0xFFu || (add ? v < bv : v > bv)) {
                best = r;
                bv = v;
            }
        }
        if (best == 0xFFu)
            break;
        pat ^= 1u << best;
        c = add ? (uint8_t)(c + 1u) : (uint8_t)(c - 1u);
    }
    return snowcrash_gen_tile(pat, p);
}

static inline void snowcrash_gen_build(uint8_t era, int32_t n, uint8_t comply)
{
    const fm_pgen_genome_t *ge;
    uint32_t *const lanes[FM_PGEN_L_N] = { &snowcrash_gen.pl.kick, &snowcrash_gen.pl.rim, &snowcrash_gen.pl.hat,
                                           &snowcrash_gen.pl.sub, &snowcrash_gen.pl.chord, &snowcrash_gen.pl.siren,
                                           &snowcrash_gen.lead_gate };
    uint8_t l, changed = 0u;

    era = snowcrash_clamp_era(era);
    fm_pgen_build(&snowcrash_gen, &snowcrash_gen_lead, &SNOWCRASH_GEN_DEF, era, n, comply);
    ge = &SNOWCRASH_GENOME[era];
    for (l = 0u; l < FM_PGEN_L_N; l++) {
        uint32_t m = snowcrash_gen_texture(*lanes[l], l, SNOWCRASH_LANE_PERIOD[era][l], era, n,
                                           SNOWCRASH_GEN_HITS[era][l][0], SNOWCRASH_GEN_HITS[era][l][1],
                                           ge->must[l], ge->ban[l]);

        changed |= m != *lanes[l];
        *lanes[l] = m;
    }
    if (changed && snowcrash_gen.views_live)
        fm_pgen_views_build(&snowcrash_gen, &SNOWCRASH_GEN_DEF,
                            fm_pgen_hash(fm_pgen_gkey(&SNOWCRASH_GEN_DEF, FM_PGEN_G_BLEND), (uint32_t)n));
}

#endif
