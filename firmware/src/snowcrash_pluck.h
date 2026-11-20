/* SPDX-License-Identifier: GPL-3.0-only */
/* KO XCRASH tuned percussion: Karplus-Strong loops for rim and hat. Integer only, one ring per voice.
 * Loop length per era comes from SNOWCRASH_KS_LEN (generator: DUB_FS / note Hz), so every era plucks its own scale. */
#ifndef SNOWCRASH_PLUCK_H
#define SNOWCRASH_PLUCK_H

#include <stdint.h>

#define SNOWCRASH_KS_MAX 128u /* longest loop: 44100 / 392 Hz = 113 */
#define SNOWCRASH_KS_VOICES 2u /* 0 = rim, 1 = hat */

typedef struct {
    int16_t ring[SNOWCRASH_KS_MAX];
    uint16_t fb;       /* loop feedback, Q15: rim rings longer than hat */
    uint8_t len, pos, on, quiet;
} snowcrash_ks_t;

static snowcrash_ks_t snowcrash_ks[SNOWCRASH_KS_VOICES];
static uint32_t snowcrash_ks_rng = 0x2545F491u;

static inline uint16_t snowcrash_ks_rand(void)
{
    /* xorshift32 */
    uint32_t x = snowcrash_ks_rng;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    snowcrash_ks_rng = x;
    return (uint16_t)(x >> 16);
}

/* Pluck trigger: fill the ring with a noise burst of the era's loop length, transposed by semi
 * (drm_tune + step tune). Called once per note-on, from the first render sample. */
static inline void snowcrash_ks_excite(uint8_t voice, int32_t semi)
{
    /* 2^(-n/12) in Q15, n = 0..11 */
    static const uint16_t down_q15[12] = { 32768u, 30929u, 29193u, 27554u, 26008u, 24548u,
                                           23170u, 21870u, 20643u, 19484u, 18390u, 17358u };
    snowcrash_ks_t *k;
    uint8_t i, len;
    int32_t l;

    if (voice >= SNOWCRASH_KS_VOICES)
        return;
    k = &snowcrash_ks[voice];
    l = (int32_t)SNOWCRASH_KS_LEN[snowcrash_ks_era][voice];
    if (semi > 36)
        semi = 36;
    else if (semi < -36)
        semi = -36;
    while (semi < 0) {
        l <<= 1;
        semi += 12;
    }
    l = (l * (int32_t)down_q15[semi % 12]) >> 15;
    l >>= semi / 12;
    if (l < 2)
        l = 2;
    if (l > (int32_t)SNOWCRASH_KS_MAX)
        l = (int32_t)SNOWCRASH_KS_MAX;
    len = (uint8_t)l;
    for (i = 0u; i < len; i++)
        k->ring[i] = (int16_t)(((int32_t)snowcrash_ks_rand() - 32768) >> 2); /* about ±8192 */
    k->len = len;
    k->pos = 0u;
    k->on = 1u;
    k->quiet = 0u;
    k->fb = voice ? 27000u : 31000u; /* hat decays faster than rim */
}

/* One output sample. Averaging filter (x[n] + x[n+1]) / 2 inside the feedback gives the string its damping. */
static inline int32_t snowcrash_ks_step(uint8_t voice)
{
    snowcrash_ks_t *k;
    int32_t y, nx, avg;
    uint8_t nxt;

    if (voice >= SNOWCRASH_KS_VOICES)
        return 0;
    k = &snowcrash_ks[voice];
    if (!k->on)
        return 0;
    y = k->ring[k->pos];
    nxt = (uint8_t)(k->pos + 1u);
    if (nxt >= k->len)
        nxt = 0u;
    nx = k->ring[nxt];
    /* Truncate toward zero: floor shifts park negative samples (−2·31000>>15 = −2) and never decay. */
    avg = (y + nx) / 2;
    k->ring[k->pos] = (int16_t)((avg * (int32_t)k->fb) / 32768);
    k->pos = nxt;
    if (y > 1 || y < -1)
        k->quiet = 0u;
    else if (++k->quiet >= k->len)
        k->on = 0u;
    return y;
}

#endif
