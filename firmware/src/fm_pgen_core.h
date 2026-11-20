/* SPDX-License-Identifier: GPL-3.0-only */
/* Endless plate continuum, genre-agnostic part: genome-driven dub_plate_t builder, landmark blend and live
 * S2..S8 views. A genre supplies one fm_pgen_genome_t per era plus its ROM tables through fm_pgen_def_t
 * (recipe: docs/plate-gen-recipe.md). Include from the genre data header after genre_conf.h. */
#ifndef FM_PGEN_CORE_H
#define FM_PGEN_CORE_H

#include <stddef.h>
#include "dub_types.h"
#include "fm_plate_gen.h"
#include "fm_plate_views.h"

/* Gene lanes in dub_plate_t mask order, then the lead gate. */
enum { FM_PGEN_L_KICK = 0, FM_PGEN_L_RIM, FM_PGEN_L_HAT, FM_PGEN_L_SUB, FM_PGEN_L_CHORD, FM_PGEN_L_SIREN,
       FM_PGEN_L_LEAD, FM_PGEN_L_N };

/* Gene ids are part of the continuum: renumbering changes every generated plate of every genre. */
enum {
    FM_PGEN_G_BPM = 1, FM_PGEN_G_DENS, FM_PGEN_G_SWING, FM_PGEN_G_PUMP, FM_PGEN_G_TANK, FM_PGEN_G_WOB,
    FM_PGEN_G_DLY, FM_PGEN_G_REV, FM_PGEN_G_FB, FM_PGEN_G_ROOT, FM_PGEN_G_SCALE, FM_PGEN_G_KIT, FM_PGEN_G_BASS,
    FM_PGEN_G_VOICE, FM_PGEN_G_TAP, FM_PGEN_G_RATE, FM_PGEN_G_KICK, FM_PGEN_G_RIM, FM_PGEN_G_HAT, FM_PGEN_G_SUB,
    FM_PGEN_G_CHORD, FM_PGEN_G_SIREN, FM_PGEN_G_LGATE, FM_PGEN_G_LPITCH, FM_PGEN_G_LLEN, FM_PGEN_G_LBEND,
    FM_PGEN_G_NAME, FM_PGEN_G_BLEND
};

#define FM_PGEN_B(x) (1u << (x))
#define FM_PGEN_QUARTERS 0x11111111u
#define FM_PGEN_OFFBEATS 0x44444444u
#define FM_PGEN_BACKBEAT 0x10101010u

/* One era. x_lo/x_hi drift ranges (compliance widens them), pools are bit sets of table indices.
 * bpm, scale_pool, must/ban and rate always come from the plate's own era (hard rules); the rest may borrow
 * a neighbour era when compliance < 127. Also used as the limits row: x_lo/x_hi = floor/ceil after widening. */
typedef struct {
    uint8_t bpm_lo, bpm_hi, dens_lo, dens_hi, swing_lo, swing_hi;
    uint8_t pump_lo, pump_hi, tank_lo, tank_hi, wob_lo, wob_hi;
    uint8_t dly_lo, dly_hi, rev_lo, rev_hi, fb_lo, fb_hi, loop_filt;
    uint8_t root_lo, root_hi;
    uint16_t scale_pool, kit_pool, bass_pool, voice_pool;
    uint8_t d[FM_PGEN_L_N];                           /* step density 0..255 before must/ban */
    uint32_t must[FM_PGEN_L_N], ban[FM_PGEN_L_N];
    uint8_t lead_len_max, gate_cs;
    int8_t rate_lo, rate_hi;
} fm_pgen_genome_t;

/* Per-genre wiring. ROM tables are indexed by slot = era * GENRE_PRESETS_PER_ERA + k; NULL = no such table. */
typedef struct {
    uint32_t key;
    const fm_pgen_genome_t *genome, *lim;
    const char *words;                                /* name words, NUL-separated (fm_pgen_name) */
    uint16_t word_n;
    const dub_plate_t *rom;
    const int8_t *root, *scale, *swing, *rate, *kit;  /* uint8_t ROM tables may be cast, values < 128 */
    const uint32_t *lead_gate;                        /* set: lane S of the views is the lead gate, else siren */
    const uint8_t *gate_cs;
    const int8_t *lead_pitch;                         /* [slot][pitch_n] */
    const uint8_t *lead_len, *lead_bend;              /* [slot][32] */
    const int8_t *intervals;                          /* 8 lead pitch moves */
    uint8_t era_n, steps, pitch_n;
    uint8_t lead_hi_echo;                             /* gen_views.py: empty lead gate hi16 replays lo16 in S2 */
} fm_pgen_def_t;

/* Live generated plate; shadows the ROM plate at .slot (0xFF = off). */
typedef struct {
    dub_plate_t pl;
    char name[9];                                     /* curated names have up to 8 letters */
    uint8_t slot, views_live;
    int8_t root, scale, swing, rate, kit;
    uint8_t gate_cs;
    uint32_t lead_gate;
    uint16_t views[PV_LANES][PV_VIEWS - 1];
} fm_pgen_t;

typedef struct {
    int8_t pitch[32];
    uint8_t len[32];
    uint8_t bend[32];
} fm_pgen_lead_t;

static inline uint8_t fm_pgen_hit(const fm_pgen_t *g, uint8_t ix)
{
    return ix == g->slot;
}

static inline uint8_t fm_pgen_views_hit(const fm_pgen_t *g, uint8_t ix)
{
    return ix == g->slot && g->views_live;
}

static inline void fm_pgen_off(fm_pgen_t *g)
{
    g->slot = 0xFFu;
}

static inline uint32_t fm_pgen_gkey(const fm_pgen_def_t *d, uint32_t gene)
{
    return fm_pgen_key(d->key, 0u, gene);
}

static inline const fm_pgen_genome_t *fm_pgen_genome(const fm_pgen_def_t *d, uint32_t key, int32_t n, uint8_t era,
                                                     uint8_t comply)
{
    return &d->genome[fm_pgen_gene_era(key, n, era, d->era_n, comply)];
}

static inline uint8_t fm_pgen_num(const fm_pgen_def_t *d, uint8_t era, int32_t n, uint8_t comply, uint32_t gene,
                                  uint8_t period, size_t off_lo, size_t off_hi)
{
    uint32_t key = fm_pgen_gkey(d, gene);
    const uint8_t *g = (const uint8_t *)fm_pgen_genome(d, key, n, era, comply);
    const uint8_t *lim = (const uint8_t *)d->lim;
    uint8_t lo = g[off_lo], hi = g[off_hi];

    fm_pgen_widen(&lo, &hi, comply, lim[off_lo], lim[off_hi]);
    return fm_pgen_range(key, n, period, lo, hi);
}

static inline uint8_t fm_pgen_cat(const fm_pgen_def_t *d, uint8_t era, int32_t n, uint8_t comply, uint32_t gene,
                                  uint8_t period)
{
    uint32_t key = fm_pgen_gkey(d, gene);
    const fm_pgen_genome_t *g = fm_pgen_genome(d, key, n, era, comply);
    uint16_t pool = gene == FM_PGEN_G_KIT ? g->kit_pool : gene == FM_PGEN_G_BASS ? g->bass_pool : g->voice_pool;

    return fm_pgen_pick(key, n, period, pool);
}

static inline uint32_t fm_pgen_lane(const fm_pgen_def_t *d, const fm_pgen_genome_t *ge, uint8_t era, int32_t n,
                                    uint8_t comply, uint8_t lane, uint8_t period)
{
    uint32_t key = fm_pgen_gkey(d, (uint32_t)FM_PGEN_G_KICK + lane);
    const fm_pgen_genome_t *g = fm_pgen_genome(d, key, n, era, comply);

    return (fm_pgen_mask(key, n, period, g->d[lane], d->steps) | ge->must[lane]) & ~ge->ban[lane];
}

static inline void fm_pgen_views_build(fm_pgen_t *g, const fm_pgen_def_t *d, uint32_t seed)
{
    const dub_plate_t *pl = &g->pl;
    const uint32_t lanes[PV_LANES] = { pl->kick, pl->rim, pl->hat, d->lead_gate ? g->lead_gate : pl->siren,
                                       pl->sub, pl->chord };
    uint16_t lo[PV_LANES], hi[PV_LANES];
    uint8_t l;

    for (l = 0u; l < PV_LANES; l++) {
        lo[l] = (uint16_t)(lanes[l] & 0xFFFFu);
        hi[l] = (uint16_t)(lanes[l] >> 16);
    }
    if (d->lead_gate && d->lead_hi_echo && !hi[PV_S])
        hi[PV_S] = lo[PV_S];
    fm_pgen_views(g->views, lo, hi, seed);
}

#define FM_PGEN_NUM(gene, per, f) \
    fm_pgen_num(d, era, n, comply, (gene), (per), offsetof(fm_pgen_genome_t, f##_lo), offsetof(fm_pgen_genome_t, f##_hi))
#define FM_PGEN_CAT(gene, per) fm_pgen_cat(d, era, n, comply, (gene), (per))

/* Fill g (and ld when the genre has lead tables) for (era, n). Curated plates sit at n = k * FM_PGEN_GAP and
 * fade out over FM_PGEN_RADIUS; at w >= 1/2 the categorical genes and lead phrase are the curated ones, the curated
 * name only at w = 1 so every click shows a new name. */
static inline void fm_pgen_build(fm_pgen_t *g, fm_pgen_lead_t *ld, const fm_pgen_def_t *d, uint8_t era, int32_t n,
                                 uint8_t comply)
{
    const fm_pgen_genome_t *ge;
    const dub_plate_t *cur;
    dub_plate_t *pl = &g->pl;
    uint32_t bkey = fm_pgen_gkey(d, FM_PGEN_G_BLEND);
    uint16_t w;
    uint8_t k, slot, s, l;
    uint32_t *const masks[6] = { &pl->kick, &pl->rim, &pl->hat, &pl->sub, &pl->chord, &pl->siren };
    const uint32_t *cmask[6];
    static const uint8_t lane_per[6] = { 6u, 6u, 6u, 5u, 5u, 5u };

    if (era >= d->era_n)
        era = (uint8_t)(d->era_n - 1u);
    ge = &d->genome[era];
    w = fm_pgen_landmark(n, &k);
    slot = (uint8_t)(era * GENRE_PRESETS_PER_ERA + k);
    cur = &d->rom[slot];
    cmask[0] = &cur->kick;
    cmask[1] = &cur->rim;
    cmask[2] = &cur->hat;
    cmask[3] = &cur->sub;
    cmask[4] = &cur->chord;
    cmask[5] = &cur->siren;

    pl->era = era;
    pl->bpm = fm_pgen_range(fm_pgen_gkey(d, FM_PGEN_G_BPM), n, 40u, ge->bpm_lo, ge->bpm_hi);
    pl->density = FM_PGEN_NUM(FM_PGEN_G_DENS, 8u, dens);
    pl->pump = FM_PGEN_NUM(FM_PGEN_G_PUMP, 4u, pump);
    pl->tank = FM_PGEN_NUM(FM_PGEN_G_TANK, 4u, tank);
    pl->wobble = FM_PGEN_NUM(FM_PGEN_G_WOB, 4u, wob);
    pl->dly_c = FM_PGEN_NUM(FM_PGEN_G_DLY, 4u, dly);
    pl->rev_c = FM_PGEN_NUM(FM_PGEN_G_REV, 4u, rev);
    pl->feedback = FM_PGEN_NUM(FM_PGEN_G_FB, 4u, fb);
    pl->loop_filt = ge->loop_filt;
    pl->tap = fm_pgen_range(fm_pgen_gkey(d, FM_PGEN_G_TAP), n, 10u, 0u, 3u);
    pl->bass_model = FM_PGEN_CAT(FM_PGEN_G_BASS, 16u);
    pl->voicing = FM_PGEN_CAT(FM_PGEN_G_VOICE, 12u);
    for (l = 0u; l < 6u; l++)
        *masks[l] = fm_pgen_lane(d, ge, era, n, comply, l, lane_per[l]);

    g->root = (int8_t)FM_PGEN_NUM(FM_PGEN_G_ROOT, 48u, root);
    g->scale = (int8_t)fm_pgen_pick(fm_pgen_gkey(d, FM_PGEN_G_SCALE), n, 64u, ge->scale_pool);
    g->swing = (int8_t)FM_PGEN_NUM(FM_PGEN_G_SWING, 24u, swing);
    g->kit = (int8_t)FM_PGEN_CAT(FM_PGEN_G_KIT, 16u);
    g->rate = (int8_t)((int32_t)fm_pgen_range(fm_pgen_gkey(d, FM_PGEN_G_RATE), n, 6u, 0u,
                                              (uint8_t)(ge->rate_hi - ge->rate_lo)) + ge->rate_lo);
    g->gate_cs = ge->gate_cs;
    g->lead_gate = d->lead_gate ? fm_pgen_lane(d, ge, era, n, comply, FM_PGEN_L_LEAD, 3u) : 0u;
    if (ld) {
        for (s = 0u; s < 32u; s++) {
            uint32_t kp = fm_pgen_hash(fm_pgen_gkey(d, FM_PGEN_G_LPITCH), s);
            uint32_t kl = fm_pgen_hash(fm_pgen_gkey(d, FM_PGEN_G_LLEN), s);
            uint32_t kb = fm_pgen_hash(fm_pgen_gkey(d, FM_PGEN_G_LBEND), s);
            uint8_t v;

            if (s < d->pitch_n) {
                v = fm_pgen_noise(kp, n, 3u);
                ld->pitch[s] = v < 160u ? 0 : d->intervals[((v - 160u) * 8u) / 96u];
            }
            v = fm_pgen_noise(kl, n, 3u);
            ld->len[s] = v < 200u ? 0u : (uint8_t)(1u + ((v - 200u) * ge->lead_len_max) / 56u);
            v = fm_pgen_noise(kb, n, 3u);
            ld->bend[s] = v < 215u ? 0u : (uint8_t)(3u + ((v - 215u) * 5u) / 41u);
        }
    }

    if (w > 0u) {
        pl->bpm = fm_pgen_blend_u8(pl->bpm, cur->bpm, w);
        pl->density = fm_pgen_blend_u8(pl->density, cur->density, w);
        pl->pump = fm_pgen_blend_u8(pl->pump, cur->pump, w);
        pl->tank = fm_pgen_blend_u8(pl->tank, cur->tank, w);
        pl->wobble = fm_pgen_blend_u8(pl->wobble, cur->wobble, w);
        pl->dly_c = fm_pgen_blend_u8(pl->dly_c, cur->dly_c, w);
        pl->rev_c = fm_pgen_blend_u8(pl->rev_c, cur->rev_c, w);
        pl->feedback = fm_pgen_blend_u8(pl->feedback, cur->feedback, w);
        pl->loop_filt = fm_pgen_blend_u8(pl->loop_filt, cur->loop_filt, w);
        for (l = 0u; l < 6u; l++)
            *masks[l] = fm_pgen_blend_mask(*masks[l], *cmask[l], w, bkey + 1u + l);
        if (d->swing)
            g->swing = fm_pgen_blend_s8(g->swing, d->swing[slot], w);
        if (d->rate)
            g->rate = fm_pgen_blend_s8(g->rate, d->rate[slot], w);
        if (d->lead_gate)
            g->lead_gate = fm_pgen_blend_mask(g->lead_gate, d->lead_gate[slot], w, fm_pgen_hash(bkey, 0x1EADu));
        if (w >= FM_PGEN_W_ONE / 2u) {
            pl->tap = cur->tap;
            pl->bass_model = cur->bass_model;
            pl->voicing = cur->voicing;
            if (d->root)
                g->root = d->root[slot];
            if (d->scale)
                g->scale = d->scale[slot];
            if (d->kit)
                g->kit = d->kit[slot];
            if (d->gate_cs)
                g->gate_cs = d->gate_cs[slot];
            if (ld)
                for (s = 0u; s < 32u; s++) {
                    if (s < d->pitch_n)
                        ld->pitch[s] = d->lead_pitch[(uint32_t)slot * d->pitch_n + s];
                    ld->len[s] = d->lead_len[(uint32_t)slot * 32u + s];
                    ld->bend[s] = d->lead_bend[(uint32_t)slot * 32u + s];
                }
        }
    }

    if (w >= FM_PGEN_W_ONE) {
        uint8_t i;

        for (i = 0u; i < sizeof g->name - 1u && cur->name[i]; i++)
            g->name[i] = cur->name[i];
        g->name[i] = '\0';
    } else {
        fm_pgen_name(g->name, fm_pgen_gkey(d, FM_PGEN_G_NAME), n, d->words, d->word_n);
    }
    pl->name = g->name;
    g->slot = slot;
    g->views_live = w < FM_PGEN_W_ONE / 2u;
    if (g->views_live)
        fm_pgen_views_build(g, d, fm_pgen_hash(bkey, (uint32_t)n));
}

#undef FM_PGEN_NUM
#undef FM_PGEN_CAT

#endif
