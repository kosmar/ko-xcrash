/* SPDX-License-Identifier: GPL-3.0-only */
/* Toast badge ink bounds (advance vs bitmap overhang). Host-testable with FONT_S. */
#ifndef TOAST_LAYOUT_H
#define TOAST_LAYOUT_H

#include <stdint.h>

typedef struct {
    uint8_t h;
    uint8_t pad;
    uint8_t first, last;
    const uint8_t *adv;
    const uint8_t *bw;
    const uint16_t *off;
    const uint8_t *data;
} toast_font_t;

typedef struct {
    int32_t bw;
    int32_t pen_x;
} toast_layout_t;

static inline uint32_t toast_glyph_ix(const toast_font_t *f, uint32_t ch)
{
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

static inline void toast_ink_extents_at(const toast_font_t *f, const char *s, int32_t pen_x, int32_t *lo,
                                        int32_t *hi)
{
    int32_t x = pen_x;
    int32_t started = 0;

    if (!s || !s[0]) {
        *lo = pen_x;
        *hi = pen_x;
        return;
    }
    for (; *s; s++) {
        uint32_t gi = toast_glyph_ix(f, (uint8_t)*s);
        int32_t w = (int32_t)f->bw[gi];
        int32_t gx0 = x - (int32_t)f->pad;
        int32_t gx, gy;

        for (gy = 0; gy < (int32_t)f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t bpr = ((uint32_t)w + 1u) / 2u;
                const uint8_t *gd = f->data + f->off[gi];
                uint32_t v = gd[gy * bpr + (uint32_t)gx / 2u];
                int32_t px;

                v = ((uint32_t)gx & 1u) ? (v & 15u) : (v >> 4);
                if (!v)
                    continue;
                px = gx0 + gx;
                if (!started) {
                    *lo = px;
                    *hi = px + 1;
                    started = 1;
                } else {
                    if (px < *lo)
                        *lo = px;
                    if (px + 1 > *hi)
                        *hi = px + 1;
                }
            }
        x += (int32_t)f->adv[gi];
    }
}

/* Measure at pen 2; pen bias +2 → ink pad L=2 R=1; pen_y 0, pad_b 2. */
enum { TOAST_PAD_MEASURE = 2, TOAST_PEN_X_BIAS = 2 };
enum { TOAST_INK_PAD_L = 2, TOAST_INK_PAD_R = 1, TOAST_BAR_PEN_Y = 0, TOAST_BAR_PAD_B = 2 };
enum { TOAST_PAD_L = TOAST_INK_PAD_L, TOAST_PAD_R = TOAST_INK_PAD_R }; /* policy/min width */
enum { TOAST_BAR_PAD = 2 }; /* legacy lane ref */

/* Toast bars: ink at pen_y; pad_b is pixels below lowest ink row (not full font cell). */
static inline int32_t toast_layout_bh_at(const toast_font_t *f, const char *s, int32_t pen_x, int32_t pen_y,
                                         int32_t pad_b)
{
    int32_t max_y = -1;

    (void)pen_x;
    if (!s || !s[0])
        return (int32_t)f->h + pen_y + pad_b;
    for (; *s; s++) {
        uint32_t gi = toast_glyph_ix(f, (uint8_t)*s);
        int32_t w = (int32_t)f->bw[gi];
        int32_t gx, gy;

        for (gy = 0; gy < (int32_t)f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t bpr = ((uint32_t)w + 1u) / 2u;
                const uint8_t *gd = f->data + f->off[gi];
                uint32_t v = gd[gy * bpr + (uint32_t)gx / 2u];
                int32_t py;

                v = ((uint32_t)gx & 1u) ? (v & 15u) : (v >> 4);
                if (!v)
                    continue;
                py = pen_y + gy;
                if (py > max_y)
                    max_y = py;
            }
    }
    if (max_y < 0)
        return (int32_t)f->h + pen_y + pad_b;
    return max_y + 1 + pad_b;
}

static inline int32_t toast_layout_bh_msg(const toast_font_t *f, const char *msg, int32_t pen_x)
{
    return toast_layout_bh_at(f, msg, pen_x, TOAST_BAR_PEN_Y, TOAST_BAR_PAD_B);
}

static inline int32_t toast_layout_bh_mode_note(const toast_font_t *f, const char *mode, const char *note,
                                                int32_t mode_pen, int32_t gap_w)
{
    int32_t bh = toast_layout_bh_at(f, mode, mode_pen, TOAST_BAR_PEN_Y, TOAST_BAR_PAD_B);
    int32_t cx, bh2;

    if (!note || !note[0])
        return bh;
    cx = mode_pen;
    {
        const char *s;

        for (s = mode; *s; s++) {
            uint32_t gi = toast_glyph_ix(f, (uint8_t)*s);
            cx += (int32_t)f->adv[gi];
        }
    }
    cx += gap_w;
    bh2 = toast_layout_bh_at(f, note, cx, TOAST_BAR_PEN_Y, TOAST_BAR_PAD_B);
    return bh2 > bh ? bh2 : bh;
}

static inline void toast_layout_msg(const toast_font_t *f, const char *msg, toast_layout_t *out)
{
    int32_t lo, hi;
    const int32_t pad = TOAST_PAD_MEASURE;

    toast_ink_extents_at(f, msg, pad, &lo, &hi);
    if (!msg || !msg[0]) {
        out->bw = TOAST_INK_PAD_L + TOAST_INK_PAD_R;
        out->pen_x = TOAST_INK_PAD_L;
        return;
    }
    out->pen_x = pad - lo + TOAST_PEN_X_BIAS;
    toast_ink_extents_at(f, msg, out->pen_x, &lo, &hi);
    out->bw = hi - lo + TOAST_INK_PAD_L + TOAST_INK_PAD_R;
}

static inline int32_t toast_lane_stack_bh(const toast_font_t *f)
{
    toast_layout_t lay;

    toast_layout_msg(f, "FLAM!", &lay);
    return toast_layout_bh_msg(f, "FLAM!", lay.pen_x);
}

/* Match cv_toast_mode_note_gap_w (one space cell unless mode_note_gap > 0). */
static inline int32_t toast_mode_note_gap_w(const toast_font_t *f, int32_t mode_note_gap)
{
    int32_t gi = toast_glyph_ix(f, ' ');
    int32_t cell = (int32_t)f->adv[gi];

    if (cell < 1)
        cell = 4;
    if (mode_note_gap > 0) {
        if (mode_note_gap < cell)
            return cell;
        return mode_note_gap;
    }
    return cell;
}

static inline void toast_layout_mode_note(const toast_font_t *f, const char *mode, const char *note, int32_t gap_w,
                                          toast_layout_t *out)
{
    int32_t lo, hi, lo2, hi2, cx;

    toast_ink_extents_at(f, mode, TOAST_PAD_MEASURE, &lo, &hi);
    if (!note || !note[0]) {
        out->pen_x = TOAST_PAD_MEASURE - lo + TOAST_PEN_X_BIAS;
        toast_ink_extents_at(f, mode, out->pen_x, &lo, &hi);
        out->bw = hi - lo + TOAST_INK_PAD_L + TOAST_INK_PAD_R;
        return;
    }
    cx = TOAST_PAD_MEASURE;
    {
        const char *s;

        for (s = mode; *s; s++) {
            uint32_t gi = toast_glyph_ix(f, (uint8_t)*s);
            cx += (int32_t)f->adv[gi];
        }
    }
    cx += gap_w;
    toast_ink_extents_at(f, note, cx, &lo2, &hi2);
    if (lo2 < lo)
        lo = lo2;
    if (hi2 > hi)
        hi = hi2;
    out->pen_x = TOAST_PAD_MEASURE - lo + TOAST_PEN_X_BIAS;
    toast_ink_extents_at(f, mode, out->pen_x, &lo, &hi);
    cx = out->pen_x;
    {
        const char *s;

        for (s = mode; *s; s++) {
            uint32_t gi = toast_glyph_ix(f, (uint8_t)*s);
            cx += (int32_t)f->adv[gi];
        }
    }
    cx += gap_w;
    toast_ink_extents_at(f, note, cx, &lo2, &hi2);
    if (lo2 < lo)
        lo = lo2;
    if (hi2 > hi)
        hi = hi2;
    out->bw = hi - lo + TOAST_INK_PAD_L + TOAST_INK_PAD_R;
}

/* Host: every non-zero glyph column must lie in [0, layout.bw). */
static inline int toast_layout_covers_ink(const toast_font_t *f, const char *msg, int32_t pen_x, int32_t bw)
{
    int32_t x = pen_x;

    if (!msg)
        return 1;
    for (; *msg; msg++) {
        uint32_t gi = toast_glyph_ix(f, (uint8_t)*msg);
        int32_t w = (int32_t)f->bw[gi];
        int32_t gx0 = x - (int32_t)f->pad;
        int32_t gx;
        uint32_t gy, bpr;
        const uint8_t *gd;

        bpr = ((uint32_t)w + 1u) / 2u;
        gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + (uint32_t)gx / 2u];

                v = ((uint32_t)gx & 1u) ? (v & 15u) : (v >> 4);
                if (v && (gx0 + gx < 0 || gx0 + gx >= bw))
                    return 0;
            }
        x += (int32_t)f->adv[gi];
    }
    return 1;
}

static inline int toast_layout_mode_note_covers(const toast_font_t *f, const char *mode, const char *note,
                                                const toast_layout_t *lay, int32_t gap_w)
{
    int32_t cx = lay->pen_x;
    const char *s;

    if (!toast_layout_covers_ink(f, mode, cx, lay->bw))
        return 0;
    for (s = mode; *s; s++) {
        uint32_t gi = toast_glyph_ix(f, (uint8_t)*s);
        cx += (int32_t)f->adv[gi];
    }
    cx += gap_w;
    return toast_layout_covers_ink(f, note, cx, lay->bw);
}

#endif /* TOAST_LAYOUT_H */
