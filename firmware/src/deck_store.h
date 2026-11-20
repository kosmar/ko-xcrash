/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DECK settings codec: segment list (optional XOR reference per segment) -> PackBits-style RLE.
 * Freestanding, no allocation; encode streams from live state, decode validates before it writes. */
#ifndef DECK_STORE_H
#define DECK_STORE_H

#include <stdint.h>

typedef struct {
    uint8_t *p;
    uint32_t n;
    const uint8_t *ref; /* NULL = raw; else byte ^= ref[i % ref_n] (ref_n > 0) */
    uint32_t ref_n;
} deck_store_seg_t;

typedef struct {
    uint32_t magic;
    uint32_t raw_len;
    uint32_t comp_len;
} deck_store_hdr_t;

/* 0x00..0x7F: c+1 literal bytes follow; 0x80..0xFF: (c & 0x7F) + 2 copies of the next byte. */
#define DECK_STORE_LIT_MAX 128u
#define DECK_STORE_RUN_MAX 129u

typedef struct {
    const deck_store_seg_t *seg;
    uint32_t nseg;
    uint32_t si;
    uint32_t off;
} deck_store_cur_t;

static inline uint32_t deck_store_raw_len(const deck_store_seg_t *seg, uint32_t nseg)
{
    uint32_t i, n = 0u;

    for (i = 0u; i < nseg; i++)
        n += seg[i].n;
    return n;
}

static inline void deck_store_cur_skip_empty(deck_store_cur_t *c)
{
    while (c->si < c->nseg && c->off >= c->seg[c->si].n) {
        c->si++;
        c->off = 0u;
    }
}

static inline uint8_t deck_store_cur_peek(const deck_store_cur_t *c)
{
    const deck_store_seg_t *s = &c->seg[c->si];
    uint8_t b = s->p[c->off];

    if (s->ref)
        b ^= s->ref[c->off % s->ref_n];
    return b;
}

static inline void deck_store_cur_next(deck_store_cur_t *c)
{
    c->off++;
    deck_store_cur_skip_empty(c);
}

static inline void deck_store_cur_put(deck_store_cur_t *c, uint8_t b)
{
    const deck_store_seg_t *s = &c->seg[c->si];

    if (s->ref)
        b ^= s->ref[c->off % s->ref_n];
    s->p[c->off] = b;
    deck_store_cur_next(c);
}

/* Returns compressed length written to out (after hdr), or 0 if it does not fit in cap. */
static inline uint32_t deck_store_encode(const deck_store_seg_t *seg, uint32_t nseg, uint32_t magic, uint8_t *out,
                                         uint32_t cap)
{
    deck_store_hdr_t h;
    deck_store_cur_t c = {seg, nseg, 0u, 0u};
    uint32_t raw = deck_store_raw_len(seg, nseg), done = 0u, o = sizeof h, i;
    uint8_t lit[DECK_STORE_LIT_MAX];
    uint32_t nlit = 0u;

    if (cap < sizeof h)
        return 0u;
    deck_store_cur_skip_empty(&c);
    while (done < raw || nlit) {
        uint8_t b = 0u;
        uint32_t run = 0u;

        if (done < raw) {
            deck_store_cur_t look = c;

            b = deck_store_cur_peek(&c);
            while (run < DECK_STORE_RUN_MAX && done + run < raw && deck_store_cur_peek(&look) == b) {
                run++;
                deck_store_cur_next(&look);
            }
        }
        if ((run >= 3u || done >= raw || nlit == DECK_STORE_LIT_MAX) && nlit) {
            if (o + 1u + nlit > cap)
                return 0u;
            out[o++] = (uint8_t)(nlit - 1u);
            for (i = 0u; i < nlit; i++)
                out[o++] = lit[i];
            nlit = 0u;
            continue;
        }
        if (run >= 3u) {
            if (o + 2u > cap)
                return 0u;
            out[o++] = (uint8_t)(0x80u | (run - 2u));
            out[o++] = b;
            for (i = 0u; i < run; i++)
                deck_store_cur_next(&c);
            done += run;
        } else {
            lit[nlit++] = b;
            deck_store_cur_next(&c);
            done++;
        }
    }
    h.magic = magic;
    h.raw_len = raw;
    h.comp_len = o - (uint32_t)sizeof h;
    {
        const uint8_t *hp = (const uint8_t *)&h;

        for (i = 0u; i < sizeof h; i++)
            out[i] = hp[i];
    }
    return h.comp_len;
}

/* write = 0: validate only (segments untouched). 0 = ok; <0 = bad magic/length/stream. */
static inline int deck_store_decode(const deck_store_seg_t *seg, uint32_t nseg, uint32_t magic, const uint8_t *in,
                                    uint32_t in_n, int write)
{
    deck_store_hdr_t h;
    deck_store_cur_t c = {seg, nseg, 0u, 0u};
    uint32_t raw = deck_store_raw_len(seg, nseg), done = 0u, i, k;
    uint8_t *hp = (uint8_t *)&h;

    if (in_n < sizeof h)
        return -1;
    for (i = 0u; i < sizeof h; i++)
        hp[i] = in[i];
    if (h.magic != magic || h.raw_len != raw || h.comp_len > in_n - (uint32_t)sizeof h)
        return -2;
    in += sizeof h;
    deck_store_cur_skip_empty(&c);
    for (i = 0u; i < h.comp_len;) {
        uint8_t ctl = in[i++];

        if (ctl < 0x80u) {
            k = (uint32_t)ctl + 1u;
            if (k > h.comp_len - i || k > raw - done)
                return -3;
            if (write) {
                uint32_t j;

                for (j = 0u; j < k; j++)
                    deck_store_cur_put(&c, in[i + j]);
            }
            i += k;
        } else {
            k = (uint32_t)(ctl & 0x7Fu) + 2u;
            if (i >= h.comp_len || k > raw - done)
                return -3;
            if (write) {
                uint32_t j;

                for (j = 0u; j < k; j++)
                    deck_store_cur_put(&c, in[i]);
            }
            i++;
        }
        done += k;
    }
    return done == raw ? 0 : -4;
}

#endif /* DECK_STORE_H */
