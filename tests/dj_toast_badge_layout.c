/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
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
#include "perf_toast_policy.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "dj_toast_badge_layout: %s\n", m);
        fail = 1;
    }
}

static int32_t toast_label_advance_w(const toast_font_t *f, const char *label)
{
    int32_t w = 0;

    if (!label)
        return 0;
    for (; *label; label++)
        w += (int32_t)f->adv[toast_glyph_ix(f, (uint8_t)*label)];
    return w;
}

static void check_label(const toast_font_t *f, const char *label)
{
    toast_layout_t lay;
    int32_t gap;
    int32_t adv_w;

    toast_layout_msg(f, label, &lay);
    check(lay.bw > 0 && lay.pen_x >= -(int32_t)f->pad, "badge dims");
    check(toast_layout_covers_ink(f, label, lay.pen_x, lay.bw), label);
    adv_w = toast_label_advance_w(f, label);
    check(lay.bw >= adv_w - 2 && lay.bw <= adv_w + TOAST_PAD_MEASURE * 2, "badge width tracks ink+pad");
    check(perf_toast_policy_main_badge_bw(f, label, 0, 0) == lay.bw, "policy matches msg layout");
    gap = (int32_t)f->adv[toast_glyph_ix(f, ' ')];
    if (gap < 1)
        gap = 4;
    toast_layout_mode_note(f, "DJ", "RIDE DENSITY", gap, &lay);
    check(toast_layout_mode_note_covers(f, "DJ", "RIDE DENSITY", &lay, gap), "dj+ride density");
    check(perf_toast_policy_main_badge_bw(f, "DJ", "RIDE DENSITY", 0) == lay.bw, "policy matches mode+note");
}

int main(void)
{
    const toast_font_t *f = (const toast_font_t *)&FONT_S;

    fail = 0;
    check_label(f, "FILL");
    check_label(f, "FLAM!");
    check_label(f, "BREAK");
    check_label(f, "LOOP");
    check_label(f, "ROLL");
    check_label(f, "RIDE");
    check_label(f, "HOOK");
    check_label(f, "DRUM FLARE");
    check_label(f, "BASS FLARE");
    check_label(f, "BAND FLARE");
    return fail ? 1 : 0;
}
