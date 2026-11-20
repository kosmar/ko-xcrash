/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef JET_SEQ_H
#define JET_SEQ_H

#include "fm_seq.h"
#include <stdint.h>

enum {
    JET_SEQ_F_ON = 1u,
    JET_SEQ_F_ACCENT = 2u,
    JET_SEQ_F_SLIDE = 4u
};

typedef struct {
    uint8_t note[FM_SEQ_STEPS]; /* 0 = empty */
    uint8_t vel[FM_SEQ_STEPS];
    uint8_t gate[FM_SEQ_STEPS]; /* 0 = TIE, 1..8 eighth of a 16th step, 8 = full */
    int8_t micro[FM_SEQ_STEPS]; /* -5..5 timing ticks at 24 PPQN grid */
    uint8_t flags[FM_SEQ_STEPS]; /* JET_SEQ_F_ON | ACCENT | SLIDE */
    uint8_t legato[FM_SEQ_STEPS]; /* bit0 legato tie from REC */
    uint8_t pl_push[FM_SEQ_STEPS];
    uint8_t pl_lpf[FM_SEQ_STEPS]; /* FM_SEQ_PLOCK_NONE = off */
    uint8_t length;               /* active length 1..128, default 16 */
} jet_seq_t;

/* MICRO is in 24 PPQN ticks: a 16th is 6 ticks, so +-5 stays inside the neighbouring steps. */
#define JET_SEQ_MICRO_MAX 5

static inline int8_t jet_seq_micro_clamp(int32_t v)
{
    if (v < -JET_SEQ_MICRO_MAX)
        v = -JET_SEQ_MICRO_MAX;
    if (v > JET_SEQ_MICRO_MAX)
        v = JET_SEQ_MICRO_MAX;
    return (int8_t)v;
}

static inline uint8_t jet_seq_len_safe(const jet_seq_t *s)
{
    if (!s || s->length < 1u)
        return FM_SEQ_PAGE_STEPS;
    if (s->length > FM_SEQ_STEPS)
        return FM_SEQ_STEPS;
    return s->length;
}

static inline uint8_t jet_seq_pages(const jet_seq_t *s)
{
    uint8_t ln = jet_seq_len_safe(s);

    return (uint8_t)((ln + FM_SEQ_PAGE_STEPS - 1u) / FM_SEQ_PAGE_STEPS);
}

static inline void jet_seq_default_init(jet_seq_t *s)
{
    uint32_t i;

    if (!s)
        return;
    s->length = FM_SEQ_PAGE_STEPS;
    for (i = 0u; i < FM_SEQ_STEPS; i++) {
        s->note[i] = 0u;
        s->gate[i] = 0u;
        s->vel[i] = 100u;
        s->micro[i] = 0;
        s->flags[i] = 0u;
        s->legato[i] = 0u;
        s->pl_push[i] = FM_SEQ_PLOCK_NONE;
        s->pl_lpf[i] = FM_SEQ_PLOCK_NONE;
    }
}

/* After a load: every field back inside what the player and the knobs expect (older builds let MICRO
 * run to +-25, which the clock never played). */
static inline void jet_seq_sanitize(jet_seq_t *s)
{
    uint32_t i;

    if (!s)
        return;
    if (s->length < 1u || s->length > FM_SEQ_STEPS)
        s->length = FM_SEQ_PAGE_STEPS;
    for (i = 0u; i < FM_SEQ_STEPS; i++) {
        if (s->note[i] > 127u)
            s->note[i] = 0u;
        if (s->vel[i] < 1u || s->vel[i] > 127u)
            s->vel[i] = 100u;
        if (s->gate[i] > 8u)
            s->gate[i] = 8u;
        s->micro[i] = jet_seq_micro_clamp(s->micro[i]);
        s->legato[i] &= 1u;
    }
}

/* Factory pattern for a fresh device, so PLAY is heard at once: a one-bar minor riff around C4,
 * stored relative to KEY like every pattern. CLEAR empties it. */
static inline void jet_seq_demo(jet_seq_t *s)
{
    static const uint8_t ST[7] = {0u, 3u, 6u, 8u, 10u, 12u, 14u};
    static const uint8_t NT[7] = {60u, 60u, 63u, 67u, 72u, 70u, 67u};
    uint32_t i;

    if (!s)
        return;
    jet_seq_default_init(s);
    for (i = 0u; i < 7u; i++) {
        s->note[ST[i]] = NT[i];
        s->gate[ST[i]] = 6u;
        s->flags[ST[i]] = JET_SEQ_F_ON;
    }
    s->flags[10] |= JET_SEQ_F_ACCENT;
}

static inline void jet_seq_clear(jet_seq_t *s)
{
    jet_seq_default_init(s);
}

static inline void jet_seq_clear_all(jet_seq_t *s)
{
    jet_seq_clear(s);
}

static inline void jet_seq_clear_step(jet_seq_t *s, uint8_t st)
{
    uint8_t ix;

    if (!s)
        return;
    ix = (uint8_t)(st % jet_seq_len_safe(s));
    s->note[ix] = 0u;
    s->gate[ix] = 0u;
    s->micro[ix] = 0;
    s->flags[ix] = 0u;
    s->legato[ix] = 0u;
    s->pl_push[ix] = FM_SEQ_PLOCK_NONE;
    s->pl_lpf[ix] = FM_SEQ_PLOCK_NONE;
}

static inline void jet_seq_set_len(jet_seq_t *s, uint8_t len)
{
    if (!s)
        return;
    if (len < 1u)
        len = 1u;
    if (len > FM_SEQ_STEPS)
        len = FM_SEQ_STEPS;
    s->length = len;
}

static inline void jet_seq_copy(jet_seq_t *dst, const jet_seq_t *src)
{
    uint32_t i;

    if (!dst || !src)
        return;
    dst->length = jet_seq_len_safe(src);
    for (i = 0u; i < FM_SEQ_STEPS; i++) {
        dst->note[i] = src->note[i];
        dst->vel[i] = src->vel[i];
        dst->gate[i] = src->gate[i];
        dst->micro[i] = src->micro[i];
        dst->flags[i] = src->flags[i];
        dst->legato[i] = src->legato[i];
        dst->pl_push[i] = src->pl_push[i];
        dst->pl_lpf[i] = src->pl_lpf[i];
    }
}

static inline void jet_seq_write_note(jet_seq_t *s, uint8_t st, uint8_t note, uint8_t gate, uint8_t vel,
                                      int8_t micro, uint8_t legato)
{
    uint8_t ix;

    if (!s)
        return;
    ix = (uint8_t)(st % jet_seq_len_safe(s));
    s->note[ix] = note;
    s->gate[ix] = gate;
    if (vel)
        s->vel[ix] = vel;
    s->micro[ix] = jet_seq_micro_clamp(micro);
    s->legato[ix] = legato;
    if (note)
        s->flags[ix] = (uint8_t)((s->flags[ix] & (uint8_t)(JET_SEQ_F_ACCENT | JET_SEQ_F_SLIDE)) | JET_SEQ_F_ON);
    else
        s->flags[ix] = (uint8_t)(s->flags[ix] & (uint8_t)(JET_SEQ_F_ACCENT | JET_SEQ_F_SLIDE));
}

/* GROOVE SCALE: index 0 = CHROM (kein Snap), 1..9 = Masken aus fm_groove_plot.h (FM_GROOVE_SCALE_SEMIS, Zeile 16ff). */
#define JET_SCALE_N 10u
static const char *const JET_SCALE_NAME[JET_SCALE_N] = {
    "CHROM", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "DOR.2", "BLUES", "HARM.M"
};
static const char *const JET_SCALE_SHORT[JET_SCALE_N] = {
    "CHR", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "DO2", "BLU", "HMN"
};
/* Padding mit 0 ist unkritisch: Grundton 0 liegt in jeder Skala. */
static const uint8_t JET_SCALE_SEMIS[JET_SCALE_N][12] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {0, 2, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0},
    {0, 2, 3, 5, 7, 9, 10, 0, 0, 0, 0, 0},
    {0, 1, 3, 5, 7, 8, 10, 0, 0, 0, 0, 0},
    {0, 2, 4, 5, 7, 9, 11, 0, 0, 0, 0, 0},
    {0, 2, 4, 5, 7, 9, 10, 0, 0, 0, 0, 0},
    {0, 3, 5, 7, 10, 0, 0, 0, 0, 0, 0, 0},
    {0, 2, 3, 5, 7, 9, 10, 0, 0, 0, 0, 0},
    {0, 3, 5, 6, 7, 10, 0, 0, 0, 0, 0, 0},
    {0, 2, 3, 5, 7, 8, 11, 0, 0, 0, 0, 0},
};

static inline uint8_t jet_scale_has(uint8_t scale, uint8_t pc)
{
    const uint8_t *t = JET_SCALE_SEMIS[scale % JET_SCALE_N];
    uint8_t k;

    for (k = 0; k < 12u; k++) {
        if (t[k] == pc)
            return 1u;
    }
    return 0u;
}

/* Snap auf die Skala (um Grundton 0), danach Tonart als Offset (0..11 Halbtöne). */
static inline uint8_t jet_seq_snap(uint8_t n, uint8_t key, uint8_t scale)
{
    int32_t v = (int32_t)n;
    int32_t d;

    for (d = 0; d < 12; d++) {
        int32_t up = v + d;
        int32_t dn = v - d;

        if (up <= 127 && jet_scale_has(scale, (uint8_t)(up % 12))) {
            v = up;
            break;
        }
        if (d && dn >= 0 && jet_scale_has(scale, (uint8_t)(dn % 12))) {
            v = dn;
            break;
        }
    }
    v += (int32_t)(key % 12u);
    if (v > 127)
        v = 127;
    return (uint8_t)v;
}

#endif /* JET_SEQ_H */
