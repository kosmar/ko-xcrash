/* SPDX-License-Identifier: GPL-3.0-only */
/* KO JET eras + factory plates. Era = pipe family; plate = named preset within that family. */
#ifndef JET_PLATES_H
#define JET_PLATES_H

#include "jet_patch.h"

enum {
    JET_ERA_FLUE = 0, /* flute / whistle / breath */
    JET_ERA_REED,     /* clarinet / sax — odd harmonics */
    JET_ERA_ORGAN,    /* pipe organ — long loop, unison */
    JET_ERA_BRASS,    /* horns — bright, MW→LPF */
    JET_ERA_BELL,     /* mallet / pluck resonators */
    JET_ERA_STEAM,    /* industrial / drone / noise */
    JET_ERA_N
};

#define JET_PLATES_PER_ERA 10u
#define JET_PLATE_N (JET_ERA_N * JET_PLATES_PER_ERA)

static const char *const JET_ERA_NAME[JET_ERA_N] = {
    "FLUE", "REED", "ORGAN", "BRASS", "BELL", "STEAM"
};

#ifndef RGB
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#endif

static const uint8_t JET_ERA_RGB[JET_ERA_N][3] = {
    { 140u, 210u, 230u }, /* FLUE — airy cyan */
    { 180u, 120u, 90u },  /* REED — warm wood */
    { 220u, 200u, 140u }, /* ORGAN — stone gold */
    { 240u, 180u, 60u },  /* BRASS — amber */
    { 200u, 220u, 255u }, /* BELL — ice */
    { 120u, 130u, 145u }  /* STEAM — steel */
};

typedef struct {
    uint8_t bpm;
    uint8_t swing;
    int8_t root; /* semitones added to default seq root (0 = era default) */
} jet_era_groove_t;

static const jet_era_groove_t JET_ERA_GROOVE[JET_ERA_N] = {
    { 96u, 52u, 0 },  /* FLUE */
    { 88u, 54u, 0 },  /* REED */
    { 72u, 50u, 0 },  /* ORGAN */
    { 100u, 56u, 0 }, /* BRASS */
    { 110u, 50u, 0 }, /* BELL */
    { 80u, 58u, -12 } /* STEAM — slower, darker root */
};

/* Full patch seed per era (musically playable before plate deltas). */
static const jet_patch_t JET_ERA_BASE[JET_ERA_N] = {
    /* FLUE — plan defaults, breathy DC, moderate push */
    {
        90u, 48u, 96u, 0u, 28u, 0u, 55u, 100u, 48u, 64u, 38u,
        28, 100u, 42u, 48u, 38u, 0u, 0u, 0u, 0u, 102u, 22u, 127u,
        0, 0,
        JET_MODE_POLY, 0u, 8u, 100u,
        72u, 64u, 64u, 64u,
        0u, 58u, 36u, 0u,
        28u, 70u, 64u, 64u, 22u, 64u,
        -10, 10u, 1u, 1u, 0, 0u, 0u, 0u
    },
    /* REED — polarity odd, harder edge */
    {
        88u, 28u, 94u, 0u, 24u, 0u, 50u, 95u, 45u, 64u, 42u,
        34, 92u, 38u, 68u, 52u, 28u, 22u, 0u, 1u, 98u, 28u, 127u,
        0, 0,
        JET_MODE_POLY, 0u, 6u, 98u,
        76u, 64u, 58u, 70u,
        0u, 60u, 32u, 0u,
        22u, 68u, 62u, 64u, 18u, 62u,
        -20, 18u, 0u, 0u, 0, 0u, 0u, 0u
    },
    /* ORGAN — long fb, dry exciter, unison detune, wet room */
    {
        86u, 14u, 92u, 0u, 18u, 0u, 40u, 100u, 55u, 64u, 30u,
        26, 118u, 98u, 42u, 35u, 8u, 12u, 0u, 0u, 96u, 18u, 127u,
        0, 0,
        JET_MODE_UNISON, 8u, 18u, 96u,
        64u, 64u, 64u, 64u,
        8u, 62u, 28u, 12u,
        48u, 82u, 58u, 62u, 28u, 72u,
        -8, 0u, 4u, 1u, 0, 0u, 0u, 0u
    },
    /* BRASS — bright sym, hard sat, MW opens LPF */
    {
        92u, 32u, 98u, 0u, 22u, 0u, 32u, 88u, 42u, 64u, 48u,
        38, 86u, 36u, 74u, 78u, 62u, 18u, 0u, 0u, 108u, 32u, 127u,
        0, 0,
        JET_MODE_POLY, 4u, 10u, 102u,
        64u, 88u, 52u, 72u,
        6u, 58u, 40u, 0u,
        32u, 72u, 66u, 68u, 24u, 68u,
        0, 6u, 2u, 0u, 0, 0u, 0u, 0u
    },
    /* BELL — pluck, split, stiff, no push whistle */
    {
        78u, 22u, 100u, 0u, 8u, 0u, 38u, 55u, 18u, 64u, 55u,
        0, 96u, 48u, 58u, 70u, 12u, 72u, 58u, 0u, 112u, 36u, 127u,
        0, 0,
        JET_MODE_POLY, 0u, 4u, 94u,
        64u, 64u, 64u, 64u,
        0u, 55u, 30u, 0u,
        18u, 64u, 70u, 72u, 16u, 70u,
        10, 4u, 5u, 1u, 0, 0u, 0u, 0u
    },
    /* STEAM — noisy, pull, huge space */
    {
        94u, 92u, 88u, 0u, 26u, 0u, 48u, 90u, 62u, 64u, 28u,
        -16, 108u, 68u, 56u, 48u, 24u, 28u, 8u, 0u, 88u, 14u, 120u,
        0, 0,
        JET_MODE_MONO, 12u, 14u, 92u,
        80u, 64u, 48u, 64u,
        12u, 64u, 34u, 18u,
        52u, 88u, 52u, 58u, 32u, 78u,
        -30, 40u, 3u, 0u, -8, 0u, 0u, 0u
    }
};

typedef struct {
    char name[8];
    uint8_t era;
    int8_t blow, dc_noise, push, fb_dec, fb_rel, sat, sym, harm, split, lpf, hpf, polarity;
    uint8_t bpm, swing;
    int8_t root;
} jet_plate_t;

#define JPL(n, er, bl, dc, ps, fbd, fbr, sa, sy, ha, sp, lp, hp, po, bp, sw, rt)                      \
    { n, er, bl, dc, ps, fbd, fbr, sa, sy, ha, sp, lp, hp, po, bp, sw, rt }

static const jet_plate_t JET_PLATE[JET_PLATE_N] = {
    /* FLUE */
    JPL("PAN", 0, 4, 6, 4, -4, 0, -6, 0, 0, 0, 4, -2, 0, 98, 52, 0),
    JPL("SHAKU", 0, -2, 10, 2, 2, -4, -8, 0, 0, 0, -6, 0, 0, 88, 54, 0),
    JPL("PICCO", 0, 6, -4, 8, -8, -6, 4, 4, 0, 0, 8, 4, 0, 112, 50, 12),
    JPL("OCARI", 0, 0, 12, 0, 4, 2, -4, -4, 0, 0, -4, -4, 0, 92, 52, 0),
    JPL("WHIST", 0, 8, -6, 12, -12, -8, 2, 0, 0, 0, 6, 2, 0, 120, 50, 0),
    JPL("NEY", 0, -4, 8, -2, 6, 4, -10, 0, 0, 0, -8, -6, 0, 86, 56, 0),
    JPL("RECORD", 0, 2, 4, 6, 0, 0, 0, 0, 0, 0, 2, 0, 0, 100, 52, 0),
    JPL("AIRY", 0, -6, -8, -4, 8, 6, -12, 0, 0, 0, -10, -8, 0, 94, 58, 0),
    JPL("BREATH", 0, -8, 14, -6, 10, 8, -14, 0, 0, 0, -12, -10, 0, 90, 54, -5),
    JPL("HOLLOW", 0, 0, 6, 0, 12, 10, -8, 0, 0, 0, -6, -12, 0, 84, 50, 0),
    /* REED */
    JPL("CLAR", 1, 0, -4, 2, 0, 0, 4, 6, 4, 0, -4, 2, 0, 92, 54, 0),
    JPL("SAX", 1, 4, 6, 6, -4, -2, 8, 10, 8, 0, 2, 0, 0, 96, 56, 0),
    JPL("BASSO", 1, -6, -8, -4, 6, 4, 6, 8, 6, 0, -8, -6, 0, 78, 52, -12),
    JPL("OBOE", 1, 2, -6, 0, 4, 2, 2, 4, 10, 0, 0, 4, 0, 86, 54, 0),
    JPL("CHAL", 1, -4, 4, -2, 8, 6, -4, 0, 0, 0, -6, -4, 0, 82, 58, 0),
    JPL("TENOR", 1, 6, 8, 8, -6, -4, 10, 12, 6, 0, 4, -2, 0, 102, 55, 0),
    JPL("ALTO", 1, 2, 2, 4, -2, 0, 6, 8, 4, 0, 0, 0, 0, 98, 54, 5),
    JPL("CIRCUS", 1, 8, 10, 10, -10, -6, 12, 14, 0, 0, 6, 0, 0, 108, 52, 0),
    JPL("BUZZ", 1, 0, 12, 4, -4, 0, 14, 16, 12, 0, 0, -4, 0, 94, 58, 0),
    JPL("REZON", 1, -2, -4, 0, 10, 8, 0, 4, 16, 0, -10, -8, 0, 80, 56, 0),
    /* ORGAN */
    JPL("PIPE", 2, 0, -4, 0, 4, 2, 0, 0, 4, 0, -2, -4, 0, 70, 50, 0),
    JPL("CHURCH", 2, -4, -6, -2, 8, 6, -4, 0, 0, 0, -4, -6, 0, 68, 50, 0),
    JPL("CATH", 2, -6, -8, -4, 12, 10, -6, 0, 0, 0, -6, -8, 0, 64, 48, -5),
    JPL("BAROQ", 2, 2, -2, 2, 0, 0, 4, 0, 8, 0, 0, 0, 0, 76, 52, 0),
    JPL("POSIT", 2, 4, 0, 4, -4, -2, 6, 4, 6, 0, 2, 2, 0, 84, 54, 0),
    JPL("PRINC", 2, -2, -4, 0, 6, 4, 2, 0, 4, 0, -2, 0, 0, 72, 50, 0),
    JPL("FLUTE8", 2, 6, 4, 6, -6, -4, -8, 0, 0, 0, 4, 0, 0, 88, 52, 12),
    JPL("CELEST", 2, -8, -10, -6, 14, 12, -8, 0, 0, 0, -8, -10, 0, 66, 48, 0),
    JPL("CHOIR", 2, 0, 0, 0, 10, 8, 0, 0, 0, 0, -4, -4, 0, 74, 52, 0),
    JPL("DRONE", 2, -4, -2, -8, 16, 14, -4, 0, 0, 0, -12, -6, 0, 60, 50, -12),
    /* BRASS */
    JPL("HORN", 3, 0, 0, 0, 0, 0, 4, 8, 0, 0, 4, 0, 0, 96, 56, 0),
    JPL("TRUMP", 3, 6, -4, 8, -6, -4, 10, 12, 0, 0, 8, 2, 0, 108, 58, 0),
    JPL("TROMB", 3, -4, 2, -2, 4, 2, 6, 10, 4, 0, -2, -4, 0, 92, 54, -7),
    JPL("TUBA", 3, -8, 4, -6, 8, 6, 8, 6, 0, 0, -8, -8, 0, 84, 52, -12),
    JPL("FANF", 3, 8, -6, 10, -8, -6, 12, 14, 0, 0, 6, 0, 0, 112, 60, 0),
    JPL("MARCH", 3, 4, 0, 4, 0, 0, 8, 8, 0, 0, 2, 0, 0, 104, 56, 0),
    JPL("SOLO", 3, 2, -2, 6, -4, -2, 6, 10, 0, 0, 6, 2, 0, 100, 54, 0),
    JPL("COPPER", 3, 0, 2, 2, 2, 0, 10, 12, 6, 0, 0, 0, 0, 98, 55, 0),
    JPL("BLAST", 3, 10, -8, 12, -10, -8, 14, 16, 0, 0, 10, 4, 0, 116, 58, 0),
    JPL("MUTE", 3, -2, 6, 0, 6, 4, 4, 18, 8, 0, -6, -2, 0, 90, 56, 0),
    /* BELL */
    JPL("XYLO", 4, 4, -6, 0, -4, -2, 4, 0, 8, 6, 6, 4, 0, 120, 50, 0),
    JPL("MARIM", 4, -2, -4, 0, 4, 2, 2, 0, 12, 8, -4, -2, 0, 100, 52, -5),
    JPL("VIBE", 4, 0, -2, 0, 0, 0, 0, 0, 10, 10, 2, 0, 0, 108, 54, 0),
    JPL("GLOCK", 4, 6, -8, 0, -8, -6, 6, 0, 16, 4, 10, 6, 0, 126, 50, 12),
    JPL("CHIME", 4, -4, -4, 0, 6, 4, 0, 0, 14, 12, -2, 0, 0, 96, 52, 0),
    JPL("TUBUL", 4, 2, 0, 0, 8, 6, -2, 0, 18, 14, -6, -4, 0, 88, 50, 0),
    JPL("METAL", 4, 4, 4, 0, -2, 0, 8, 4, 20, 8, 8, 8, 0, 114, 50, 0),
    JPL("GLASS", 4, 0, -6, 0, 2, 0, 4, 0, 22, 6, 12, 10, 0, 118, 52, 7),
    JPL("PLUCK", 4, -6, -8, 0, -6, -8, -4, 0, 6, 4, 4, 2, 0, 112, 48, 0),
    JPL("HAND", 4, -4, -2, 0, 0, 0, 2, 0, 8, 8, 0, 0, 0, 104, 54, 0),
    /* STEAM */
    JPL("FACT", 5, 4, 8, -4, 4, 2, 4, 0, 4, 0, -4, -6, 0, 78, 58, 0),
    JPL("SIREN", 5, 6, 12, 8, -6, -4, 6, 4, 0, 0, 0, 0, 0, 82, 60, 0),
    JPL("FOGHN", 5, 0, 16, -8, 8, 6, 0, 0, 8, 0, -8, -10, 0, 76, 56, -5),
    JPL("RUST", 5, -4, 10, -6, 10, 8, 8, 6, 12, 0, -6, -8, 0, 74, 58, -7),
    JPL("BOIL", 5, 2, 6, -2, 6, 4, 10, 8, 6, 0, -2, -4, 0, 80, 60, 0),
    JPL("TANK", 5, -2, 4, -10, 12, 10, 6, 4, 10, 0, -10, -6, 0, 72, 54, -12),
    JPL("VALVE", 5, 4, -4, 6, 0, 0, 12, 10, 0, 0, 2, 0, 0, 86, 56, 0),
    JPL("ECHO", 5, -6, 8, -4, 14, 12, -4, 0, 0, 0, -12, -8, 0, 68, 62, 0),
    JPL("WIND", 5, 0, 18, -12, 6, 4, 2, 0, 16, 0, -6, -12, 0, 84, 58, 0),
    JPL("HULL", 5, -8, 6, -6, 16, 14, 4, 2, 8, 0, -14, -10, 0, 70, 60, -12)
};

#undef JPL

static inline uint8_t jet_clamp_era(uint8_t era)
{
    return era >= JET_ERA_N ? (uint8_t)(JET_ERA_N - 1u) : era;
}

static inline uint8_t jet_clamp_plate(uint8_t pl)
{
    return pl >= JET_PLATE_N ? (uint8_t)(JET_PLATE_N - 1u) : pl;
}

static inline uint8_t jet_clamp_swing_u8(uint8_t sw)
{
    if (sw < 50u)
        return 50u;
    if (sw > 75u)
        return 75u;
    return sw;
}

static inline uint8_t jet_era_of_plate(uint8_t pl)
{
    return JET_PLATE[jet_clamp_plate(pl)].era;
}

static inline uint8_t jet_era_first_plate(uint8_t era)
{
    return (uint8_t)(jet_clamp_era(era) * JET_PLATES_PER_ERA);
}

static inline uint8_t jet_plate_slot(uint8_t pl)
{
    return (uint8_t)(jet_clamp_plate(pl) % JET_PLATES_PER_ERA);
}

static inline uint8_t jet_plate_in_era(uint8_t pl, uint8_t era)
{
    return jet_clamp_plate((uint8_t)(jet_era_first_plate(era) + jet_plate_slot(pl)));
}

static inline uint16_t jet_era_col(uint8_t era)
{
    const uint8_t *c = JET_ERA_RGB[jet_clamp_era(era)];

    return RGB(c[0], c[1], c[2]);
}

static inline const char *jet_era_name(uint8_t era)
{
    return JET_ERA_NAME[jet_clamp_era(era)];
}

static inline const char *jet_plate_name(uint8_t pl)
{
    return JET_PLATE[jet_clamp_plate(pl)].name;
}

static inline uint8_t jet_sat_u8(int32_t v)
{
    if (v < 0)
        return 0u;
    if (v > 127)
        return 127u;
    return (uint8_t)v;
}

static inline int8_t jet_sat_push(int32_t v)
{
    if (v < -64)
        return -64;
    if (v > 63)
        return 63;
    return (int8_t)v;
}

static inline uint8_t jet_sat_polar(int32_t v)
{
    if (v <= 0)
        return 0u;
    return 1u;
}

static inline void jet_plate_groove(uint8_t era, uint8_t plate_idx, uint8_t *bpm, uint8_t *swing, int8_t *root)
{
    const jet_plate_t *p;
    const jet_era_groove_t *g;
    uint8_t e;

    e = jet_clamp_era(era);
    p = &JET_PLATE[jet_clamp_plate((uint8_t)(jet_era_first_plate(e) + (plate_idx % JET_PLATES_PER_ERA)))];
    g = &JET_ERA_GROOVE[e];
    if (bpm)
        *bpm = p->bpm ? p->bpm : g->bpm;
    if (swing)
        *swing = jet_clamp_swing_u8(p->swing ? p->swing : g->swing);
    if (root)
        *root = (int8_t)(g->root + p->root);
}

/* Era base + plate deltas on 12 core pipe params; full patch then sanitize. */
static inline void jet_plate_apply(jet_patch_t *patch_out, uint8_t era, uint8_t plate_idx)
{
    const jet_plate_t *pl;
    const jet_patch_t *base;
    uint8_t e;
    uint8_t idx;

    if (!patch_out)
        return;
    e = jet_clamp_era(era);
    idx = (uint8_t)(jet_era_first_plate(e) + (plate_idx % JET_PLATES_PER_ERA));
    pl = &JET_PLATE[idx];
    base = &JET_ERA_BASE[e];
    *patch_out = *base;
    patch_out->blow = jet_sat_u8((int32_t)base->blow + (int32_t)pl->blow);
    patch_out->dc_noise = jet_sat_u8((int32_t)base->dc_noise + (int32_t)pl->dc_noise);
    patch_out->push = jet_sat_push((int32_t)base->push + (int32_t)pl->push);
    patch_out->fb_dec = jet_sat_u8((int32_t)base->fb_dec + (int32_t)pl->fb_dec);
    patch_out->fb_rel = jet_sat_u8((int32_t)base->fb_rel + (int32_t)pl->fb_rel);
    patch_out->sat = jet_sat_u8((int32_t)base->sat + (int32_t)pl->sat);
    patch_out->sat_sym = jet_sat_u8((int32_t)base->sat_sym + (int32_t)pl->sym);
    patch_out->harm = jet_sat_u8((int32_t)base->harm + (int32_t)pl->harm);
    patch_out->split = jet_sat_u8((int32_t)base->split + (int32_t)pl->split);
    patch_out->lpf = jet_sat_u8((int32_t)base->lpf + (int32_t)pl->lpf);
    patch_out->hpf = jet_sat_u8((int32_t)base->hpf + (int32_t)pl->hpf);
    patch_out->polarity = jet_sat_polar((int32_t)base->polarity + (int32_t)pl->polarity);
    jet_patch_sanitize(patch_out);
}

#endif /* JET_PLATES_H */
