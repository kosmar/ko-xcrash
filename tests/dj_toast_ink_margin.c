/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
typedef struct {
    uint8_t h;
    uint8_t pad;
    uint8_t first, last;
    const uint8_t *adv;
    const uint8_t *bw;
    const uint16_t *off;
    const uint8_t *data;
} felucca_font_t;
#include "felucca_font.h"
#include "toast_layout.h"

static int ink_outside(const felucca_font_t *f, const char *msg, int32_t pen, int32_t bw)
{
    int32_t x = pen;
    int32_t bad = 0;

    for (; *msg; msg++) {
        uint32_t gi = toast_glyph_ix((const toast_font_t *)f, (uint8_t)*msg);
        int32_t w = (int32_t)f->bw[gi];
        int32_t gx0 = x - (int32_t)f->pad;
        int32_t gx, gy;

        for (gy = 0; gy < (int32_t)f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t bpr = ((uint32_t)w + 1u) / 2u;
                const uint8_t *gd = f->data + f->off[gi];
                uint32_t v = gd[gy * bpr + (uint32_t)gx / 2u];

                v = ((uint32_t)gx & 1u) ? (v & 15u) : (v >> 4);
                if (v && (gx0 + gx < 0 || gx0 + gx >= bw))
                    bad++;
            }
        x += (int32_t)f->adv[gi];
    }
    return bad;
}

static int32_t max_ink_x(const felucca_font_t *f, const char *msg, int32_t pen)
{
    int32_t x = pen, mx = -1;

    for (; *msg; msg++) {
        uint32_t gi = toast_glyph_ix((const toast_font_t *)f, (uint8_t)*msg);
        int32_t w = (int32_t)f->bw[gi];
        int32_t gx0 = x - (int32_t)f->pad;
        int32_t gx, gy;

        for (gy = 0; gy < (int32_t)f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t bpr = ((uint32_t)w + 1u) / 2u;
                const uint8_t *gd = f->data + f->off[gi];
                uint32_t v = gd[gy * bpr + (uint32_t)gx / 2u];

                v = ((uint32_t)gx & 1u) ? (v & 15u) : (v >> 4);
                if (v) {
                    int32_t px = gx0 + gx;

                    if (px > mx)
                        mx = px;
                }
            }
        x += (int32_t)f->adv[gi];
    }
    return mx;
}

int main(void)
{
    const felucca_font_t *f = &FONT_S;
    const char *labels[] = {"DRUM FLARE", "BASS FLARE", "BAND FLARE", "FLAM!"};
    toast_layout_t lay;
    int i, fail = 0;

    for (i = 0; i < 4; i++) {
        const char *l = labels[i];
        int32_t bh;

        toast_layout_msg((const toast_font_t *)f, l, &lay);
        bh = toast_layout_bh_msg((const toast_font_t *)f, l, lay.pen_x);
        {
            int32_t lo, hi;

            toast_ink_extents_at((const toast_font_t *)f, l, lay.pen_x, &lo, &hi);
            if (lo != TOAST_INK_PAD_L || lay.bw - hi != TOAST_INK_PAD_R || ink_outside(f, l, lay.pen_x, lay.bw) ||
                bh != 14) {
                fprintf(stderr, "dj_toast_ink_margin: %s bw=%d lo=%d right=%d bh=%d\n", l, lay.bw, lo,
                        (int)(lay.bw - hi), bh);
                fail = 1;
            }
        }
    }
    return fail ? 1 : 0;
}
