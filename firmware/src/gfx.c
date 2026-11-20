/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Small-canvas renderer (no full framebuffer). Draw text/lines into
 * an off-screen strip, then blit it in one DMA transfer. Pixels are stored
 * byte-swapped (the panel takes RGB565 big-endian). */
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;
} felucca_font_t;
#include "felucca_font.h"
#include "toast_layout.h"

#ifndef CV_MAX
#define CV_MAX (240u * 132u)      /* tallest strip: Dit Dah stage */
#endif
#if defined(DINGHY_HOST) || defined(DINGHY_WEB)
static uint16_t cv_px[CV_MAX];
#else
static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));
#endif
static uint32_t cv_w, cv_h;
static int32_t cv_ox, cv_oy;     /* origin for a shifted diagram */
static uint8_t cv_zoom;          /* 1 or 2: true 2x2 isotropic scale for circle geometry */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define C_BLACK 0x0000u
#define C_WHITE 0xFFFFu              /* accent only: what is being touched / where we are */
/* The screen is five steps of one colour, darkest to brightest, plus white.
 * Palettes are picked in the HOME-hold menu (COLOR). */
typedef struct {
    const char *name;
    uint16_t c[5];
} palette_t;
static const palette_t PALETTES[] = {
    {"GREEN", {RGB(0, 40, 12), RGB(0, 84, 30), RGB(16, 140, 54), RGB(56, 200, 92), RGB(120, 255, 146)}},
    {"AMBER", {RGB(60, 26, 0), RGB(110, 50, 0), RGB(170, 82, 0), RGB(225, 120, 8), RGB(255, 166, 40)}},
    {"CYAN", {RGB(0, 30, 50), RGB(0, 62, 96), RGB(16, 112, 160), RGB(56, 172, 222), RGB(140, 222, 255)}},
    {"RED", {RGB(52, 8, 8), RGB(100, 18, 14), RGB(170, 36, 26), RGB(226, 64, 48), RGB(255, 112, 92)}},
    {"MONO", {RGB(40, 40, 40), RGB(80, 80, 80), RGB(130, 130, 130), RGB(186, 186, 186), RGB(226, 226, 226)}},
    {"DX", {RGB(16, 46, 46), RGB(26, 90, 92), RGB(40, 150, 152), RGB(56, 210, 212), RGB(66, 245, 245)}},   /* sloopDX: cyan */
};
#define NPALETTES (sizeof(PALETTES) / sizeof(PALETTES[0]))
static uint16_t pal[5];
#define C_LINE pal[0]                /* 1 rules, separators */
#define C_DIM pal[1]                 /* 2 inactive, empty steps, units */
#define C_GRAY pal[2]                /* 3 labels */
#define C_AMB pal[3]                 /* 4 secondary text */
#define C_HI pal[4]                  /* 5 values, curves */

static void palette_set(uint32_t i)
{
    uint32_t k;
    for (k = 0; k < 5u; k++)
        pal[k] = PALETTES[i % NPALETTES].c[k];
}

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i, n;
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    lcd_sync();                     /* the last blit may still read cv_px */
    cv_ox = 0;
    cv_oy = 0;
    cv_zoom = 1u;
    cv_w = w;
    cv_h = h;
    n = w * h;
    for (i = 0; i < n; i++)
        cv_px[i] = swap16(bg);
}

static void cv_blit(uint32_t x, uint32_t y) { lcd_blit(x, y, cv_w, cv_h, cv_px); }

/* Set by dinghy before each redraw. Home draws its own canvas; this only places the working diagram. */
uint8_t cv_home_view;

static void cv_origin(int32_t x, int32_t y)
{
    cv_ox = x;
    cv_oy = y;
}

static void cv_set_zoom(uint8_t z)
{
    cv_zoom = z > 1u ? 2u : 1u;
}

static void cv_blit_engine_diagram(void)
{
    lcd_fill(0u, 24u, 240u, 14u, C_BLACK);
    lcd_fill(0u, 38u + 88u, 240u, 14u, C_BLACK);
    cv_blit(0u, 38u);
}

/* canvas rows r0 .. cv_h-1 only, to screen row y + r0 */
static void cv_blit_from(uint32_t x, uint32_t y, uint32_t r0)
{
    if (r0 < cv_h)
        lcd_blit(x, y + r0, cv_w, cv_h - r0, cv_px + r0 * cv_w);
}

static inline void cv_pset_raw(int32_t x, int32_t y, uint16_t c)
{
    x += cv_ox;
    y += cv_oy;
    if ((uint32_t)x < cv_w && (uint32_t)y < cv_h)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
}

static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    if (cv_zoom == 2u) {
        int32_t zx = (x * 3) / 2, zy = (y * 3) / 2;
        cv_pset_raw(zx, zy, c);
        cv_pset_raw(zx + 1, zy, c);
        cv_pset_raw(zx, zy + 1, c);
        return;
    }
    cv_pset_raw(x, y, c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1, y1, i;
    uint16_t sc = swap16(c);
    if (cv_zoom == 2u) {
        int32_t nx1 = ((x + w) * 3) / 2;
        int32_t ny1 = ((y + h) * 3) / 2;
        x = (x * 3) / 2;
        y = (y * 3) / 2;
        w = nx1 > x ? nx1 - x : 1;
        h = ny1 > y ? ny1 - y : 1;
    }
    x1 = x + w + cv_ox;
    y1 = y + h + cv_oy;
    x += cv_ox;
    y += cv_oy;                             /* clipped once, then filled row by row */
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > (int32_t)cv_h)
        y1 = (int32_t)cv_h;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx, sx, dy, sy, err, guard = 2000;
    if (cv_zoom == 2u) {
        x0 = (x0 * 3) / 2;
        y0 = (y0 * 3) / 2;
        x1 = (x1 * 3) / 2;
        y1 = (y1 * 3) / 2;
    }
    dx = x1 > x0 ? x1 - x0 : x0 - x1;
    sx = x0 < x1 ? 1 : -1;
    dy = y1 > y0 ? y0 - y1 : y1 - y0;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;
    while (guard--) {                       /* bounded: a line is never longer than 480 px */
        int32_t e2 = 2 * err;               /* both tests use the same error value */
        cv_pset_raw(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* glyph index of a character: lower case folds to upper case when the font
 * has none, anything missing (and C1 controls) draws as '?' */
static uint32_t glyph(const felucca_font_t *f, uint32_t ch)
{
    if (ch == 0xB1u && f == &FONT_S)
        return FONT_S_PM_GI;
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

static uint16_t cv_blend_on_bg(uint16_t fg, uint16_t bg, uint32_t cov)
{
    uint32_t fr, fg_g, fb_c, br, bg_g, bb_c;

    if (cov >= 15u)
        return fg;
    if (cov == 0u)
        return bg;
    fr = (uint32_t)fg >> 11;
    fg_g = ((uint32_t)fg >> 5) & 63u;
    fb_c = (uint32_t)fg & 31u;
    br = (uint32_t)bg >> 11;
    bg_g = ((uint32_t)bg >> 5) & 63u;
    bb_c = (uint32_t)bg & 31u;
    return (uint16_t)((((fr * cov + br * (15u - cov)) / 15u) << 11) |
                      (((fg_g * cov + bg_g * (15u - cov)) / 15u) << 5) |
                      ((fb_c * cov + bb_c * (15u - cov)) / 15u));
}

/* text, alpha-blended onto black with colour c; returns the end x */
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    uint16_t ramp[16];
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u, a;
    for (a = 0; a < 16u; a++)
        ramp[a] = (uint16_t)(((r * a / 15u) << 11) | ((g * a / 15u) << 5) | (b * a / 15u));
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        bpr = (w + 1u) / 2u;
        gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                v = (gx & 1u) ? (v & 15u) : (v >> 4);
                if (v)
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, ramp[v]);
            }
        x += f->adv[gi];
    }
    return x;
}

/* Toast ink: blend fg onto bar bg (not black). */
static int32_t cv_text_on_bg(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t fg,
                             uint16_t bg)
{
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        bpr = (w + 1u) / 2u;
        gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                v = (gx & 1u) ? (v & 15u) : (v >> 4);
                if (v)
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, cv_blend_on_bg(fg, bg, v));
            }
        x += f->adv[gi];
    }
    return x;
}

static int32_t text_w(const felucca_font_t *f, const char *s)
{
    int32_t w = 0;
    for (; *s; s++)
        w += f->adv[glyph(f, (uint8_t)*s)];
    return w;
}

enum { CV_TOAST_OK = 0, CV_TOAST_WARN = 1, CV_TOAST_ERR = 2 };
#define CV_TOAST_FADE_MS 800u
#define CV_LCD_W 240u
#define CV_LCD_H 240u

/* 4×4 Bayer (0…15); toast fade thins coverage, not brightness. */
static const uint8_t cv_bayer4[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

static inline uint8_t cv_toast_dither_on(int32_t x, int32_t y, uint32_t thresh)
{
    if (thresh >= 16u)
        return 1u;
    if (thresh == 0u)
        return 0u;
    return cv_bayer4[((uint32_t)y & 3u) * 4u + ((uint32_t)x & 3u)] < thresh;
}

static uint8_t cv_glyph_cov(const felucca_font_t *f, int32_t px, int32_t py, int32_t pen_x, int32_t pen_y,
                            const char *s)
{
    int32_t x = pen_x;

    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s);
        int32_t w = (int32_t)f->bw[gi];
        int32_t gx0 = x - (int32_t)f->pad;
        int32_t gy = py - pen_y;

        if (px >= gx0 && px < gx0 + w && gy >= 0 && gy < (int32_t)f->h) {
            uint32_t gx = (uint32_t)(px - gx0);
            uint32_t bpr = ((uint32_t)w + 1u) / 2u;
            const uint8_t *gd = f->data + f->off[gi];
            uint32_t v = gd[(uint32_t)gy * bpr + gx / 2u];

            v = (gx & 1u) ? (v & 15u) : (v >> 4);
            return (uint8_t)v;
        }
        x += (int32_t)f->adv[gi];
    }
    return 0u;
}

static void cv_toast_fade_draw_at(int32_t bw, int32_t bh, int32_t bx, int32_t by, int32_t pen_x, int32_t pen_y,
                                  int32_t clip_x0, int32_t clip_x1, const felucca_font_t *f, const char *msg,
                                  uint16_t bg, uint16_t fg, uint32_t dith)
{
    int32_t xi, yi;

    if (clip_x0 < 0)
        clip_x0 = 0;
    if (clip_x1 > bw)
        clip_x1 = bw;
    if (clip_x0 >= clip_x1)
        return;
    for (yi = 0; yi < bh; yi++)
        for (xi = clip_x0; xi < clip_x1; xi++) {
            uint8_t cov;

            if (!cv_toast_dither_on(bx + xi, by + yi, dith)) {
                cv_pset(xi, yi, bg);
                continue;
            }
            cov = cv_glyph_cov(f, xi, yi, pen_x, pen_y, msg);
            cv_pset(xi, yi, cov ? cv_blend_on_bg(fg, bg, cov) : bg);
        }
}

static void cv_toast_fade_draw(int32_t bw, int32_t bh, int32_t bx, int32_t by, int32_t pad, const felucca_font_t *f,
                               const char *msg, uint16_t bg, uint16_t fg, uint32_t dith)
{
    cv_toast_fade_draw_at(bw, bh, bx, by, pad, TOAST_BAR_PEN_Y, 0, bw, f, msg, bg, fg, dith);
}

static uint16_t cv_toast_bar_rgb(uint8_t kind)
{
    if (kind == CV_TOAST_WARN)
        return RGB(150, 118, 0);
    if (kind == CV_TOAST_ERR)
        return RGB(130, 28, 28);
    return RGB(28, 96, 40);
}

static uint32_t cv_rgb565_luma(uint16_t col)
{
    uint32_t r, g, b;

    r = ((uint32_t)col >> 11) * 255u / 31u;
    g = (((uint32_t)col >> 5) & 63u) * 255u / 63u;
    b = ((uint32_t)col & 31u) * 255u / 31u;
    return (r * 299u + g * 587u + b * 114u) / 1000u;
}

static uint16_t cv_toast_lane_bar_rgb(uint16_t lane_col)
{
    uint32_t r, g, b, y;

    r = ((uint32_t)lane_col >> 11) * 255u / 31u;
    g = (((uint32_t)lane_col >> 5) & 63u) * 255u / 63u;
    b = ((uint32_t)lane_col & 31u) * 255u / 31u;
    y = cv_rgb565_luma(lane_col);
    if (y <= 200u)
        return lane_col;
    return RGB((uint8_t)(r * 55u / 100u), (uint8_t)(g * 55u / 100u), (uint8_t)(b * 55u / 100u));
}

#define CV_TOAST_TEXT_LUMA_THRESH 170u
#define C_TOAST_TEXT_DARK RGB(16, 16, 16)

static uint16_t cv_toast_text_fg(uint16_t bg)
{
    if (cv_rgb565_luma(bg) > CV_TOAST_TEXT_LUMA_THRESH)
        return C_TOAST_TEXT_DARK;
    return C_WHITE;
}

/* Optional minimum toast bar width (latched by caller to stop center-X shimmer). */
static int32_t cv_toast_bw_floor;

static void cv_toast_bw_floor_set(int32_t floor)
{
    cv_toast_bw_floor = floor > 0 ? floor : 0;
}

/* Performance toast bar; last CV_TOAST_FADE_MS fades via 4×4 Bayer dither. by = top Y on LCD. */
static void cv_draw_toast(const felucca_font_t *f, const char *msg, uint8_t kind, uint32_t now_ms,
                          uint32_t end_ms, int32_t by, uint16_t lane_bar_col, int32_t x_shift,
                          uint8_t dith_fixed, int32_t layer_y0)
{
    uint32_t rem, dith;
    int32_t tw, bw, bh, bx, ly;
    int32_t ox0, oy0;
    uint16_t bg, fg;
    uint8_t fade;

    if (!msg || !msg[0])
        return;
    dith = 16u;
    fade = 0u;
    if (dith_fixed) {
        dith = dith_fixed;
        fade = 1u;
    } else if (end_ms > now_ms) {
        rem = end_ms - now_ms;
        if (rem < CV_TOAST_FADE_MS) {
            fade = 1u;
            if (rem == 0u)
                return;
            if (rem <= 200u)
                dith = 4u;
            else if (rem <= 400u)
                dith = 8u;
            else if (rem <= 600u)
                dith = 12u;
        }
    } else if (end_ms != 0u)
        return;
    {
        toast_layout_t lay;
        const toast_font_t *tf = (const toast_font_t *)f;

        toast_layout_msg(tf, msg, &lay);
        bw = lay.bw;
        tw = lay.pen_x;
        bh = toast_layout_bh_msg(tf, msg, tw);
    }
    if (cv_toast_bw_floor > bw) {
        tw += (cv_toast_bw_floor - bw) / 2;
        bw = cv_toast_bw_floor;
    }
    bx = ((int32_t)CV_LCD_W - bw) / 2 + x_shift;
    bg = lane_bar_col ? cv_toast_lane_bar_rgb(lane_bar_col) : cv_toast_bar_rgb(kind);
    fg = cv_toast_text_fg(bg);
    ox0 = cv_ox;
    oy0 = cv_oy;
    if (layer_y0 >= 0) {
        ly = by - layer_y0;
        if (ly + bh <= 0 || ly >= (int32_t)cv_h)
            return;
        cv_origin(bx, ly);
        cv_rect(0, 0, bw, bh, bg);
        if (fade)
            cv_toast_fade_draw(bw, bh, bx, by, tw, f, msg, bg, fg, dith);
        else
            cv_text_on_bg(tw, TOAST_BAR_PEN_Y, f, msg, fg, bg);
        cv_origin(ox0, oy0);
        return;
    }
    cv_begin((uint32_t)bw, (uint32_t)bh, C_BLACK);
    cv_rect(0, 0, bw, bh, bg);
    if (fade)
        cv_toast_fade_draw(bw, bh, bx, by, tw, f, msg, bg, fg, dith);
    else
        cv_text_on_bg(tw, TOAST_BAR_PEN_Y, f, msg, fg, bg);
    cv_blit((uint32_t)bx, (uint32_t)by);
}

/* ARP mode label stays solid; note suffix slides/fades on its own timer. */
/* mode_note_gap: 0 or -1 = one space in f (min; glyphs overhang advance); else pixel gap (clamped up). */
static int32_t cv_toast_mode_note_gap_w(const felucca_font_t *f, int32_t mode_note_gap)
{
    return toast_mode_note_gap_w((const toast_font_t *)f, mode_note_gap);
}

static void cv_draw_toast_arp_note(const felucca_font_t *f, const char *mode, const char *note, uint8_t kind,
                                   uint32_t now_ms, uint32_t note_end_ms, int32_t by, uint16_t lane_bar_col,
                                   int32_t note_x_shift, uint8_t note_dith_fixed, int32_t mode_note_gap,
                                   int32_t layer_y0)
{
    uint32_t rem, dith;
    int32_t gap_w, mode_w, bw, bh, bx, note_x, ly, mode_pen;
    int32_t ox0, oy0;
    uint16_t bg, fg;
    uint8_t fade;
    const toast_font_t *tf = (const toast_font_t *)f;

    if (!mode || !mode[0])
        return;
    if (!note || !note[0] || (note_end_ms != 0u && note_end_ms <= now_ms))
        note = 0;
    gap_w = cv_toast_mode_note_gap_w(f, mode_note_gap);
    mode_w = text_w(f, mode);
    bg = lane_bar_col ? cv_toast_lane_bar_rgb(lane_bar_col) : cv_toast_bar_rgb(kind);
    fg = cv_toast_text_fg(bg);
    dith = 16u;
    fade = 0u;
    if (note && note_dith_fixed) {
        dith = note_dith_fixed;
        fade = 1u;
    } else if (note && note_end_ms > now_ms) {
        rem = note_end_ms - now_ms;
        if (rem < CV_TOAST_FADE_MS) {
            fade = 1u;
            if (rem == 0u)
                note = 0;
            else if (rem <= 200u)
                dith = 4u;
            else if (rem <= 400u)
                dith = 8u;
            else if (rem <= 600u)
                dith = 12u;
        }
    } else if (note && note_end_ms != 0u)
        note = 0;
    {
        toast_layout_t lay;

        if (note)
            toast_layout_mode_note(tf, mode, note, gap_w, &lay);
        else
            toast_layout_msg(tf, mode, &lay);
        bw = lay.bw;
        mode_pen = lay.pen_x;
        bh = toast_layout_bh_mode_note(tf, mode, note, mode_pen, gap_w);
    }
    if (cv_toast_bw_floor > bw) {
        mode_pen += (cv_toast_bw_floor - bw) / 2;
        bw = cv_toast_bw_floor;
    }
    bx = ((int32_t)CV_LCD_W - bw) / 2;
    ox0 = cv_ox;
    oy0 = cv_oy;
    if (layer_y0 >= 0) {
        ly = by - layer_y0;
        if (ly + bh <= 0 || ly >= (int32_t)cv_h)
            return;
        cv_origin(bx, ly);
        cv_rect(0, 0, bw, bh, bg);
        cv_text_on_bg(mode_pen, TOAST_BAR_PEN_Y, f, mode, fg, bg);
        if (note) {
            note_x = mode_pen + mode_w + gap_w + note_x_shift;
            if (fade)
                cv_toast_fade_draw_at(bw, bh, bx, by, note_x, TOAST_BAR_PEN_Y, 0, bw, f, note, bg, fg, dith);
            else
                cv_text_on_bg(note_x, TOAST_BAR_PEN_Y, f, note, fg, bg);
        }
        cv_origin(ox0, oy0);
        return;
    }
    cv_begin((uint32_t)bw, (uint32_t)bh, C_BLACK);
    cv_rect(0, 0, bw, bh, bg);
    cv_text_on_bg(mode_pen, TOAST_BAR_PEN_Y, f, mode, fg, bg);
    if (note) {
        note_x = mode_pen + mode_w + gap_w + note_x_shift;
        if (fade)
            cv_toast_fade_draw_at(bw, bh, bx, by, note_x, TOAST_BAR_PEN_Y, 0, bw, f, note, bg, fg, dith);
        else
            cv_text_on_bg(note_x, TOAST_BAR_PEN_Y, f, note, fg, bg);
    }
    cv_blit((uint32_t)bx, (uint32_t)by);
}

/* one-shot: text in a box, cleared to black, blitted */
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s,
                          uint16_t c, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, C_BLACK);
    if (align == 1)
        tx = ((int32_t)w - tw) / 2;
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();                     /* one-shots (boot, crash, UBOOT, update) finish here */
}

#if defined(TOAST_FADE_BG_HOST_TEST)
#include <stdio.h>

/* Host: badge must fully occlude desk waveform under fade + layer_y0 compositing. */
int toast_fade_bg_host_test(void)
{
    static const char *const labels[] = {"FILL", "BREAK", "DRUM FLARE", "BASS FLARE"};
    static const uint16_t WAVE = 0xF81Fu; /* loud magenta — must not show through badge */
    enum { LAYER_Y0 = 28, CANVAS_H = 132 };
    int fail = 0;
    uint32_t li;

    for (li = 0; li < sizeof labels / sizeof labels[0]; li++) {
        const char *msg = labels[li];
        toast_layout_t lay;
        int32_t bh, by, ly, bx, xi, yi;
        uint16_t bg;
        int32_t ox0, oy0;

        toast_layout_msg((const toast_font_t *)&FONT_S, msg, &lay);
        bh = toast_layout_bh_msg((const toast_font_t *)&FONT_S, msg, lay.pen_x);
        by = LAYER_Y0 + (CANVAS_H - bh) / 2;
        ly = by - LAYER_Y0;
        bx = ((int32_t)CV_LCD_W - lay.bw) / 2;
        bg = cv_toast_bar_rgb(CV_TOAST_WARN);

        cv_begin(240u, (uint32_t)CANVAS_H, C_BLACK);
        for (yi = ly; yi < ly + bh; yi++)
            for (xi = 0; xi < 240; xi++)
                cv_pset_raw(xi, yi, WAVE);

        ox0 = cv_ox;
        oy0 = cv_oy;
        cv_origin(bx, ly);
        cv_rect(0, 0, lay.bw, bh, bg);
        cv_toast_fade_draw(lay.bw, bh, bx, by, lay.pen_x, &FONT_S, msg, bg, cv_toast_text_fg(bg), 8u);
        cv_origin(ox0, oy0);

        for (yi = 0; yi < bh; yi++)
            for (xi = 0; xi < lay.bw; xi++) {
                int32_t cx = bx + xi;
                int32_t cy = ly + yi;
                uint16_t got;

                if ((uint32_t)cx >= cv_w || (uint32_t)cy >= cv_h)
                    continue;
                got = cv_px[(uint32_t)cy * cv_w + (uint32_t)cx];
                got = (uint16_t)(got >> 8 | got << 8);
                if (got == WAVE) {
                    fprintf(stderr, "toast_fade_bg: %s pixel (%d,%d) still waveform\n", msg, xi, yi);
                    fail = 1;
                }
            }
    }
    return fail ? 1 : 0;
}
#endif
