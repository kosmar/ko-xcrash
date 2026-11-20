/* SPDX-License-Identifier: GPL-3.0-only */
/* Endless plate continuum: deterministic per (genre key, era, n). Neighbouring n share most genes. */
#ifndef FM_PLATE_GEN_H
#define FM_PLATE_GEN_H

#include <stdint.h>

enum {
    FM_PGEN_LANDMARKS = 10,     /* curated plates per era, placed at n = k * GAP */
    FM_PGEN_GAP = 8,
    FM_PGEN_RADIUS = 3,         /* curated weight fades to 0 at this distance */
    FM_PGEN_JUMP = 997,         /* EDIT + PRESET: prime, no resonance with gene periods */
    FM_PGEN_COMPLY_DEF = 104,
    FM_PGEN_W_ONE = 256,
    /* DJ transition plate steps along n (manual PRESET stays 1). */
    FM_PGEN_DJ_XF = FM_PGEN_GAP,
    FM_PGEN_DJ_CUE = FM_PGEN_GAP,
    FM_PGEN_DJ_ECHO = 3,
    FM_PGEN_DJ_DROP = FM_PGEN_JUMP,
    FM_PGEN_DJ_WANDER = 16      /* DJ compliance offset range: +-WANDER per transition */
};

#define FM_PGEN_CFG_OK 0xA5u

/* Persisted tail of dub_cfg_t; 8-aligned so the old cfg size equals its offset. */
typedef struct {
    int32_t n;
    uint8_t comply;
    uint8_t ok;
    uint8_t rsv[2];
} fm_pgen_cfg_t;

static inline uint32_t fm_pgen_hash(uint32_t a, uint32_t b)
{
    uint32_t x = a * 0x9E3779B1u + b * 0x85EBCA77u + 0x7F4A7C15u;

    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

static inline uint32_t fm_pgen_key(uint32_t genre, uint32_t era, uint32_t gene)
{
    return fm_pgen_hash(fm_pgen_hash(genre, era), gene);
}

static inline int32_t fm_pgen_floordiv(int32_t n, int32_t d)
{
    int32_t q = n / d;

    if ((n % d) != 0 && ((n < 0) != (d < 0)))
        q--;
    return q;
}

/* Smooth 0..255 value noise over n; one new random target every `period` clicks. */
static inline uint8_t fm_pgen_noise(uint32_t key, int32_t n, uint8_t period)
{
    int32_t cell, frac, a, b, t;

    if (period < 1u)
        period = 1u;
    cell = fm_pgen_floordiv(n, (int32_t)period);
    frac = n - cell * (int32_t)period;
    a = (int32_t)(fm_pgen_hash(key, (uint32_t)cell) & 0xFFu);
    b = (int32_t)(fm_pgen_hash(key, (uint32_t)(cell + 1)) & 0xFFu);
    t = (frac * 256) / (int32_t)period;
    t = (t * t * (768 - 2 * t)) >> 16;
    return (uint8_t)(a + (((b - a) * t) >> 8));
}

/* Value in [lo, hi] driven by noise. */
static inline uint8_t fm_pgen_range(uint32_t key, int32_t n, uint8_t period, uint8_t lo, uint8_t hi)
{
    uint32_t v = fm_pgen_noise(key, n, period);

    if (hi <= lo)
        return lo;
    return (uint8_t)(lo + (v * (uint32_t)(hi - lo + 1u)) / 256u);
}

/* Pick set bit index from a pool mask (fallback: first bit or 0). */
static inline uint8_t fm_pgen_pick(uint32_t key, int32_t n, uint8_t period, uint32_t pool)
{
    uint8_t cnt = 0u, i, k;

    for (i = 0u; i < 32u; i++)
        if (pool & (1u << i))
            cnt++;
    if (!cnt)
        return 0u;
    k = (uint8_t)((fm_pgen_noise(key, n, period) * (uint32_t)cnt) / 256u);
    for (i = 0u; i < 32u; i++) {
        if (!(pool & (1u << i)))
            continue;
        if (k == 0u)
            return i;
        k--;
    }
    return 0u;
}

/* Era-compliance: some genes borrow a neighbour era when comply < 127. */
static inline uint8_t fm_pgen_gene_era(uint32_t key, int32_t n, uint8_t era, uint8_t era_n, uint8_t comply)
{
    uint8_t sel = fm_pgen_noise(key ^ 0xC0FFEEu, n, 24u);
    uint8_t loose = (uint8_t)(127u - (comply > 127u ? 127u : comply));

    if (era_n < 2u || sel >= loose)
        return era;
    if (sel & 1u)
        return (uint8_t)((era + 1u) % era_n);
    return (uint8_t)((era + era_n - 1u) % era_n);
}

/* Widen a range around its centre when comply < 127 (up to 2x at 0). */
static inline void fm_pgen_widen(uint8_t *lo, uint8_t *hi, uint8_t comply, uint8_t floor_v, uint8_t ceil_v)
{
    int32_t c = ((int32_t)*lo + (int32_t)*hi) / 2;
    int32_t half = ((int32_t)*hi - (int32_t)*lo) / 2 + 1;
    int32_t loose = 127 - (int32_t)(comply > 127u ? 127u : comply);
    int32_t h = half + (half * loose) / 127;
    int32_t l = c - h, u = c + h;

    if (l < (int32_t)floor_v)
        l = floor_v;
    if (u > (int32_t)ceil_v)
        u = ceil_v;
    *lo = (uint8_t)l;
    *hi = (uint8_t)u;
}

/* Drifting step mask: each step has its own slow noise; on when below density (0..255). */
static inline uint32_t fm_pgen_mask(uint32_t key, int32_t n, uint8_t period, uint8_t density, uint8_t steps)
{
    uint32_t m = 0u;
    uint8_t s;

    for (s = 0u; s < steps && s < 32u; s++)
        if (fm_pgen_noise(fm_pgen_hash(key, s), n, period) < density)
            m |= 1u << s;
    return m;
}

/* Nearest landmark k (0..9) and curated weight 0..256. Far from landmarks: k wraps, w = 0. */
static inline uint16_t fm_pgen_landmark(int32_t n, uint8_t *k_out)
{
    int32_t k = fm_pgen_floordiv(n + FM_PGEN_GAP / 2, FM_PGEN_GAP);
    int32_t d = n - k * FM_PGEN_GAP;
    uint16_t w = 0u;

    if (d < 0)
        d = -d;
    if (k >= 0 && k < FM_PGEN_LANDMARKS && d < FM_PGEN_RADIUS)
        w = (uint16_t)((FM_PGEN_W_ONE * (FM_PGEN_RADIUS - d)) / FM_PGEN_RADIUS);
    k %= FM_PGEN_LANDMARKS;
    if (k < 0)
        k += FM_PGEN_LANDMARKS;
    if (k_out)
        *k_out = (uint8_t)k;
    return w;
}

static inline int32_t fm_pgen_landmark_n(uint8_t k)
{
    return (int32_t)(k % FM_PGEN_LANDMARKS) * FM_PGEN_GAP;
}

static inline uint8_t fm_pgen_blend_u8(uint8_t gen, uint8_t cur, uint16_t w)
{
    return (uint8_t)((int32_t)gen + ((((int32_t)cur - (int32_t)gen) * (int32_t)w) >> 8));
}

static inline int8_t fm_pgen_blend_s8(int8_t gen, int8_t cur, uint16_t w)
{
    return (int8_t)((int32_t)gen + ((((int32_t)cur - (int32_t)gen) * (int32_t)w) >> 8));
}

/* Per-step handover: step s takes the curated bit once w passes its fixed rank. */
static inline uint32_t fm_pgen_blend_mask(uint32_t gen, uint32_t cur, uint16_t w, uint32_t key)
{
    uint32_t m = 0u;
    uint8_t s;

    if (w >= FM_PGEN_W_ONE)
        return cur;
    if (w == 0u)
        return gen;
    for (s = 0u; s < 32u; s++) {
        uint32_t src = ((fm_pgen_hash(key, s) & 0xFFu) < w) ? cur : gen;
        m |= src & (1u << s);
    }
    return m;
}

static inline uint32_t fm_pgen_gcd(uint32_t a, uint32_t b)
{
    while (b) {
        uint32_t t = a % b;

        a = b;
        b = t;
    }
    return a;
}

/* Name = word of the genre list (NUL-separated, word_n words of up to 8 letters). Even n pick even indices,
 * odd n odd ones, so neighbouring n never show the same word; each parity walks a hashed affine permutation of
 * its half per block of `half` picks, so a word does not come back within one block. */
static inline void fm_pgen_name(char out[9], uint32_t key, int32_t n, const char *words, uint16_t word_n)
{
    uint32_t half = word_n / 2u, par = (uint32_t)n & 1u, ix, h, a, b;
    int32_t m = fm_pgen_floordiv(n, 2), blk;
    uint8_t i;

    out[0] = '\0';
    if (!words || !half)
        return;
    blk = fm_pgen_floordiv(m, (int32_t)half);
    h = fm_pgen_hash(key, (uint32_t)blk * 2u + par);
    a = half > 1u ? 1u + h % (half - 1u) : 1u;
    while (fm_pgen_gcd(a, half) != 1u)
        a = a + 1u < half ? a + 1u : 1u;
    b = fm_pgen_hash(h, 1u) % half;
    ix = ((a * (uint32_t)(m - blk * (int32_t)half) + b) % half) * 2u + par;
    while (ix--) {
        while (*words)
            words++;
        words++;
    }
    for (i = 0u; i < 8u && words[i]; i++)
        out[i] = words[i];
    out[i] = '\0';
}

#endif
