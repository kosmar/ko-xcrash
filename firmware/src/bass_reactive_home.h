/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS HOME field (240 × 126; knob bar starts at canvas y 126). Included from bass_draw.h.
 * No text: turning filter card (point-symmetric), drive transfer knot, bass line contour, all blended additively. */
#ifndef BASS_REACTIVE_HOME_H
#define BASS_REACTIVE_HOME_H

#ifndef BASS_RH_SCALE
#define BASS_RH_SCALE(col, amt) (col)
#endif
#ifndef BASS_RH_SINE /* q15 sine over a full uint32 phase */
#define BASS_RH_SINE(ph) ((int32_t)((((ph) >> 16) < 0x8000u ? ((ph) >> 16) : 0xFFFFu - ((ph) >> 16)) * 2u) - 32767)
#endif

#define BASS_RH_STEPS 16u
#define BASS_RH_W 240
#define BASS_RH_H 126
#define BASS_RH_YC 63

enum {
    BASS_RH_ON = 1u,     /* programmed step that sounds */
    BASS_RH_FILL = 2u,   /* rest filled by DENS > 0 */
    BASS_RH_DROP = 4u,   /* programmed step thinned by DENS < 0 */
    BASS_RH_ACCENT = 8u,
    BASS_RH_SLIDE = 16u,
    BASS_RH_TIE = 32u,
    BASS_RH_OUT = 64u /* past the lane length */
};

typedef struct {
    uint16_t ecol;
    uint16_t acc_col;
    int32_t cut;      /* effective cutoff_b 0..127 (LFO + macros included) */
    int32_t reso;     /* effective reso_mode 0..127 */
    int32_t drive;    /* effective drive 0..127 */
    int32_t env_filt; /* filter env amount; only swept when env_sweep */
    int32_t env;      /* amp env level 0..127 */
    uint32_t t;       /* ms, animation clock */
    uint8_t env_sweep;
    uint8_t gate_open;
    uint8_t playing;
    uint8_t cur;      /* 0..15 playing column, 255 = none */
    uint8_t note[BASS_RH_STEPS];
    uint8_t flags[BASS_RH_STEPS];
    uint8_t gate[BASS_RH_STEPS]; /* 1..8, 8 = tie */
    uint8_t rat[BASS_RH_STEPS];
} bass_reactive_home_t;

/* Saturating RGB565 add, clipped to the field so nothing bleeds into the knob bar. */
static inline void bass_rh_px(int32_t x, int32_t y, uint16_t c)
{
    uint32_t i, d, r, gg, b;

    if ((uint32_t)x >= (uint32_t)BASS_RH_W || (uint32_t)y >= (uint32_t)BASS_RH_H)
        return;
    x += cv_ox;
    y += cv_oy;
    if ((uint32_t)x >= cv_w || (uint32_t)y >= cv_h)
        return;
    i = (uint32_t)y * cv_w + (uint32_t)x;
    d = swap16(cv_px[i]);
    r = (d >> 11) + ((uint32_t)c >> 11);
    gg = ((d >> 5) & 63u) + (((uint32_t)c >> 5) & 63u);
    b = (d & 31u) + ((uint32_t)c & 31u);
    if (r > 31u)
        r = 31u;
    if (gg > 63u)
        gg = 63u;
    if (b > 31u)
        b = 31u;
    cv_px[i] = swap16((r << 11) | (gg << 5) | b);
}

static void bass_rh_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int32_t sx = x0 < x1 ? 1 : -1;
    int32_t sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy;
    int32_t guard = 600;

    while (guard--) {
        int32_t e2 = 2 * err;

        bass_rh_px(x0, y0, c);
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

static void bass_rh_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t i, j;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            bass_rh_px(x + i, y + j, c);
}

/* Soft radial blob: brightness falls off with r². */
static void bass_rh_glow(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t r2 = r * r + 1;
    int32_t dx, dy;

    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++) {
            int32_t d2 = dx * dx + dy * dy;

            if (d2 < r2)
                bass_rh_px(cx + dx, cy + dy, BASS_RH_SCALE(c, 256 - (d2 * 256) / r2));
        }
}

static void bass_rh_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t x = r;
    int32_t y = 0;
    int32_t e = 1 - r;

    while (x >= y) {
        bass_rh_px(cx + x, cy + y, c);
        bass_rh_px(cx - x, cy + y, c);
        bass_rh_px(cx + x, cy - y, c);
        bass_rh_px(cx - x, cy - y, c);
        bass_rh_px(cx + y, cy + x, c);
        bass_rh_px(cx - y, cy + x, c);
        bass_rh_px(cx + y, cy - x, c);
        bass_rh_px(cx - y, cy - x, c);
        y++;
        if (e < 0)
            e += 2 * y + 1;
        else {
            x--;
            e += 2 * (y - x) + 1;
        }
    }
}

/* Half-height of the low-pass magnitude: flat pass band, reso bump at xc, roll-off above. */
static inline int32_t bass_rh_filt_h(int32_t x, int32_t xc, int32_t peak)
{
    int32_t d = x - xc;
    int32_t h = 28 + (peak * 512 + (512 + d * d) / 2) / (512 + d * d);

    if (d > 0)
        h -= (d * d) / 96 + d / 4;
    if (h < 0)
        h = 0;
    if (h > 58)
        h = 58;
    return h;
}

static inline int32_t bass_rh_cut_x(int32_t cut)
{
    return 12 + (cut * (BASS_RH_W - 24)) / 127;
}

/* Background: rolling scanline band, breathes with the amp env. */
static void bass_rh_draw_bg(const bass_reactive_home_t *in)
{
    int32_t y;

    cv_rect(0, 0, BASS_RH_W, BASS_RH_H, RGB(2, 4, 14));
    for (y = 0; y < BASS_RH_H; y += 2) {
        int32_t s = BASS_RH_SINE((uint32_t)y * 0x03000000u - in->t * 0x00180000u);
        int32_t a = 10 + ((s + 32768) >> 12) + in->env / 6;

        bass_rh_line(0, y, BASS_RH_W - 1, y, BASS_RH_SCALE(in->ecol, a));
    }
}

/* Horizon axis tilts with the cutoff: closed leans down-right, open leans up-right. */
static inline int32_t bass_rh_axis(int32_t x, int32_t tilt)
{
    return BASS_RH_YC + ((x - BASS_RH_W / 2) * tilt) / 256;
}

/* Lower half is the response flipped left/right and squashed: point-symmetric, never a mirror. */
static inline int32_t bass_rh_filt_hb(int32_t x, int32_t xc, int32_t peak)
{
    return (bass_rh_filt_h(BASS_RH_W - 1 - x, xc, peak) * 5) / 8;
}

typedef struct {
    int32_t cs; /* q15 cos / sin of the card's turn around the vertical axis */
    int32_t sn;
} bass_rh_rot_t;

/* Endless card: x always spans the full field; the turn only swings height (near side tall, far side flat). */
static inline void bass_rh_proj(const bass_rh_rot_t *r, int32_t x, int32_t y, int32_t *ox, int32_t *oy)
{
    int32_t dx = x - BASS_RH_W / 2;
    int32_t sc = 65536 / (256 + ((dx * r->sn) >> 15));

    *ox = x;
    *oy = BASS_RH_YC + (((y - BASS_RH_YC) * sc) >> 8);
}

/* Filter as a turning card: upper response + flipped lower echo; the upper edge shivers with the env. */
static void bass_rh_draw_filter(const bass_reactive_home_t *in)
{
    int32_t xc = bass_rh_cut_x(in->cut);
    int32_t peak = (in->reso * 28) / 127;
    int32_t wob_amt = in->env / 14;
    int32_t split = 2 + in->drive / 28;
    int32_t tilt = ((64 - in->cut) * 3) / 8;
    uint32_t th = (uint32_t)(BASS_RH_SINE(in->t * 0x00080000u) * 0xE38E);
    bass_rh_rot_t rot;
    int32_t ptx = 0, pty = 0, pbx = 0, pby = 0;
    int32_t x;

    rot.sn = BASS_RH_SINE(th);
    rot.cs = BASS_RH_SINE(th + 0x40000000u);
    for (x = 0; x < BASS_RH_W; x++) {
        int32_t ax = bass_rh_axis(x, tilt);
        int32_t h = bass_rh_filt_h(x, xc, peak);
        int32_t wob = (BASS_RH_SINE((uint32_t)x * 0x02400000u + in->t * 0x00500000u) * wob_amt) >> 15;
        int32_t tx, ty, bx, by;
        uint16_t sc;

        h += wob;
        if (h < 0)
            h = 0;
        bass_rh_proj(&rot, x, ax - h, &tx, &ty);
        bass_rh_proj(&rot, x, ax + bass_rh_filt_hb(x, xc, peak), &bx, &by);
        if ((x & 1) == 0 && by > ty + 1) {
            sc = x <= xc ? BASS_RH_SCALE(in->ecol, (x & 7) == 0 ? 70 : 34)
                         : BASS_RH_SCALE(in->acc_col, 30);
            bass_rh_line(tx, ty + 1, bx, by - 1, sc);
        }
        if (x > 0) {
            uint16_t ghost = BASS_RH_SCALE(in->acc_col, 90);

            bass_rh_line(ptx, pty, tx, ty, BASS_RH_SCALE(in->ecol, 230));
            bass_rh_line(pbx, pby, bx, by, BASS_RH_SCALE(in->ecol, 130));
            bass_rh_px(tx, ty - split, ghost);
            bass_rh_px(bx, by + split, ghost);
        }
        ptx = tx;
        pty = ty;
        pbx = bx;
        pby = by;
    }
    for (x = 0; x < BASS_RH_H; x += 3) {
        int32_t sx, sy;

        bass_rh_proj(&rot, xc, x, &sx, &sy);
        bass_rh_px(sx, sy, BASS_RH_SCALE(in->ecol, 110));
    }
    /* ACID: env opens the filter — a second horizon slides right on every note (and left, below). */
    if (in->env_sweep && in->env_filt > 0 && in->env > 0) {
        int32_t xe = xc + (in->env * in->env_filt) / 110;
        uint16_t ec = BASS_RH_SCALE(in->acc_col, 60 + in->env);

        if (xe > BASS_RH_W - 1)
            xe = BASS_RH_W - 1;
        if (xe > xc + 1) {
            int first = 1;

            for (x = 0; x < BASS_RH_W; x += 2) {
                int32_t ax = bass_rh_axis(x, tilt);
                int32_t tx, ty, bx, by;

                bass_rh_proj(&rot, x, ax - bass_rh_filt_h(x, xe, peak), &tx, &ty);
                bass_rh_proj(&rot, x, ax + bass_rh_filt_hb(x, xe, peak), &bx, &by);
                if (!first) {
                    if (x >= xc)
                        bass_rh_line(ptx, pty, tx, ty, ec);
                    if (x <= BASS_RH_W - 1 - xc)
                        bass_rh_line(pbx, pby, bx, by, ec);
                }
                first = 0;
                ptx = tx;
                pty = ty;
                pbx = bx;
                pby = by;
            }
        }
    }
    /* Resonance: ripples rolling out of both peaks. */
    if (in->reso > 8) {
        int32_t xb = BASS_RH_W - 1 - xc;
        int32_t xt, yt, xb2, yb;
        int32_t k;

        bass_rh_proj(&rot, xc, bass_rh_axis(xc, tilt) - bass_rh_filt_h(xc, xc, peak), &xt, &yt);
        bass_rh_proj(&rot, xb, bass_rh_axis(xb, tilt) + bass_rh_filt_hb(xb, xc, peak), &xb2, &yb);
        for (k = 0; k < 3; k++) {
            int32_t r = (int32_t)((in->t / 24u + (uint32_t)k * 14u) % 42u) + 2;
            int32_t a = ((in->reso * (44 - r)) / 44) + in->env / 2;

            bass_rh_ring(xt, yt, r, BASS_RH_SCALE(in->ecol, a));
            bass_rh_ring(xb2, yb, r / 2 + 1, BASS_RH_SCALE(in->acc_col, a / 2));
        }
        bass_rh_glow(xt, yt, 2 + in->reso / 32, BASS_RH_SCALE(in->ecol, 300));
        bass_rh_glow(xb2, yb, 1 + in->reso / 48, BASS_RH_SCALE(in->acc_col, 200));
    }
}

/* Soft clip of a q15 sine through the drive stage, returns q8 (−256..256). */
static inline int32_t bass_rh_shape(int32_t s, int32_t g)
{
    int32_t v = ((s >> 7) * g) >> 8;
    int32_t av = v < 0 ? -v : v;
    int32_t o = (v * 512) / (256 + av);

    if (o > 256)
        o = 256;
    if (o < -256)
        o = -256;
    return o;
}

/* Drive × env: the shaped signal as an endless wave across the field. The amp env sets the height,
 * drive squares the crests off; trailing echoes (same direction) come in with drive. */
static void bass_rh_draw_drive(const bass_reactive_home_t *in)
{
    const int32_t cy = BASS_RH_YC;
    int32_t g = 256 + in->drive * 9;
    int32_t hh = 3 + (in->env * 40) / 127;
    int32_t echoes = in->drive / 40;
    uint32_t ph0 = in->t * 0x00600000u;
    int32_t k;

    for (k = echoes; k >= 0; k--) {
        int32_t lag = k * (6 + in->drive / 10);
        int32_t a = k == 0 ? 110 + in->env + in->drive / 2 : (60 + in->drive / 2) / k;
        uint16_t col = k == 0 ? BASS_RH_SCALE(in->acc_col, a) : BASS_RH_SCALE(in->ecol, a);
        int32_t h = k == 0 ? hh : (hh * 3) / 4;
        int32_t py = 0;
        int32_t x;

        for (x = 0; x < BASS_RH_W; x++) {
            uint32_t ph = (uint32_t)(x - lag) * 0x04400000u - ph0;
            int32_t y = cy - (bass_rh_shape(BASS_RH_SINE(ph), g) * h) / 256;

            if (x > 0)
                bass_rh_line(x - 1, py, x, y, col);
            py = y;
        }
    }
    if (in->gate_open && in->env > 0) {
        uint32_t ph = (uint32_t)(BASS_RH_W / 2) * 0x04400000u - ph0;
        int32_t y = cy - (bass_rh_shape(BASS_RH_SINE(ph), g) * hh) / 256;

        bass_rh_glow(BASS_RH_W / 2, y, 2 + in->env / 22, BASS_RH_SCALE(C_WHITE, 120 + in->env));
    }
}

static inline int32_t bass_rh_note_y(uint8_t n, int32_t lo, int32_t span)
{
    const int32_t y_top = 10;
    const int32_t y_bot = 114;

    return y_bot - (((int32_t)n - lo) * (y_bot - y_top)) / span;
}

static inline int bass_rh_sounds(uint8_t f)
{
    return (f & (BASS_RH_ON | BASS_RH_FILL)) != 0u && !(f & BASS_RH_OUT);
}

/* One bar of the bass line as heard: glowing pitch bars + melodic contour across the whole field. */
static void bass_rh_draw_lane(const bass_reactive_home_t *in)
{
    const int32_t cw = BASS_RH_W / (int32_t)BASS_RH_STEPS;
    int32_t lo = 127;
    int32_t hi = 0;
    int32_t span;
    int32_t px = -1;
    int32_t py = 0;
    uint32_t c;

    for (c = 0u; c < BASS_RH_STEPS; c++) {
        if (!(in->flags[c] & (BASS_RH_ON | BASS_RH_FILL | BASS_RH_DROP)))
            continue;
        if ((int32_t)in->note[c] < lo)
            lo = in->note[c];
        if ((int32_t)in->note[c] > hi)
            hi = in->note[c];
    }
    if (hi < lo)
        lo = hi = 48;
    span = hi - lo;
    if (span < 12) {
        lo -= (12 - span) / 2;
        span = 12;
    }

    if (in->playing && in->cur < BASS_RH_STEPS) {
        int32_t x = (int32_t)in->cur * cw;

        bass_rh_rect(x, 0, cw, BASS_RH_H, BASS_RH_SCALE(in->ecol, 22 + in->env / 4));
        bass_rh_line(x, 0, x, BASS_RH_H - 1, BASS_RH_SCALE(in->ecol, 90));
    }
    /* Contour through every sounding step. */
    for (c = 0u; c < BASS_RH_STEPS; c++) {
        int32_t x, y;

        if (!bass_rh_sounds(in->flags[c]))
            continue;
        x = (int32_t)c * cw + cw / 2;
        y = bass_rh_note_y(in->note[c], lo, span);
        if (px >= 0)
            bass_rh_line(px, py, x, y, BASS_RH_SCALE(in->acc_col, 70));
        px = x;
        py = y;
    }
    for (c = 0u; c < BASS_RH_STEPS; c++) {
        uint8_t f = in->flags[c];
        int32_t x = (int32_t)c * cw;
        int32_t y, w, h;
        uint16_t col;
        int now = in->playing && c == in->cur;

        bass_rh_px(x + cw / 2, BASS_RH_H - 3, BASS_RH_SCALE(in->ecol, (c & 3u) == 0u ? 140 : 50));
        if ((c & 3u) == 0u)
            bass_rh_px(x + cw / 2, BASS_RH_H - 2, BASS_RH_SCALE(in->ecol, 140));
        if (f & BASS_RH_OUT)
            continue;
        if (!(f & (BASS_RH_ON | BASS_RH_FILL | BASS_RH_DROP)))
            continue;
        y = bass_rh_note_y(in->note[c], lo, span);
        w = 2 + ((int32_t)(in->gate[c] ? in->gate[c] : 4u) * 12) / 8;
        if (f & (BASS_RH_TIE | BASS_RH_SLIDE))
            w = cw;
        h = (f & BASS_RH_ACCENT) ? 5 : 3;
        if (f & BASS_RH_DROP) {
            bass_rh_line(x, y - 2, x + w - 1, y - 2, BASS_RH_SCALE(in->ecol, 50));
            bass_rh_line(x, y + 2, x + w - 1, y + 2, BASS_RH_SCALE(in->ecol, 50));
            continue;
        }
        col = (f & BASS_RH_ACCENT) ? BASS_RH_SCALE(in->ecol, 320) : in->ecol;
        if (now)
            col = C_WHITE;
        if (f & BASS_RH_ACCENT) {
            int32_t len = 6 + in->drive / 12;

            bass_rh_line(x + w / 2, y - h - len, x + w / 2, y + h + len, BASS_RH_SCALE(col, 80));
        }
        bass_rh_rect(x - 1, y - h / 2 - 1, w + 2, h + 2, BASS_RH_SCALE(col, 60));
        if (f & BASS_RH_FILL) {
            bass_rh_line(x, y - h / 2, x + w - 1, y - h / 2, col);
            bass_rh_line(x, y + h / 2, x + w - 1, y + h / 2, col);
        } else if (in->rat[c] > 1u) {
            int32_t n = in->rat[c];
            int32_t sw = (cw - 1) / n;
            int32_t k;

            for (k = 0; k < n; k++)
                bass_rh_rect(x + k * sw, y - h / 2, sw > 2 ? sw - 1 : 1, h, col);
        } else
            bass_rh_rect(x, y - h / 2, w, h, col);
        if (now)
            bass_rh_glow(x + w / 2, y, 4 + in->env / 24, BASS_RH_SCALE(in->ecol, 200));
        if ((f & BASS_RH_SLIDE) && c + 1u < BASS_RH_STEPS && bass_rh_sounds(in->flags[c + 1u])) {
            int32_t y2 = bass_rh_note_y(in->note[c + 1u], lo, span);

            bass_rh_line(x + cw - 3, y, x + cw + 1, y2, col);
        }
    }
}

/* Drive × env tears horizontal bands sideways. */
static void bass_rh_glitch(const bass_reactive_home_t *in)
{
    static uint16_t row[BASS_RH_W];
    int32_t amt = in->gate_open ? (in->drive * in->env) >> 7 : 0;
    uint32_t frame = in->t >> 6;
    int32_t y;

    if (amt < 8 || cv_w < (uint32_t)BASS_RH_W || cv_ox != 0)
        return;
    for (y = 0; y < BASS_RH_H; y++) {
        uint32_t h = ((uint32_t)(y >> 2) * 2654435761u) ^ (frame * 40503u);
        int32_t sh, x;
        uint16_t *p;

        h ^= h >> 13;
        h *= 0x5bd1e995u;
        h ^= h >> 15;
        if ((int32_t)(h & 127u) >= amt / 2)
            continue;
        sh = ((int32_t)((h >> 8) & 15u) - 8) * (1 + amt / 32);
        if (sh == 0 || y + cv_oy < 0 || (uint32_t)(y + cv_oy) >= cv_h)
            continue;
        p = &cv_px[(uint32_t)(y + cv_oy) * cv_w];
        for (x = 0; x < BASS_RH_W; x++)
            row[x] = p[x];
        for (x = 0; x < BASS_RH_W; x++)
            p[x] = row[(uint32_t)(x - sh + BASS_RH_W * 4) % (uint32_t)BASS_RH_W];
    }
}

static inline void bass_reactive_home_draw(const bass_reactive_home_t *in)
{
    bass_rh_draw_bg(in);
    bass_rh_draw_filter(in);
    bass_rh_draw_drive(in);
    bass_rh_draw_lane(in);
    bass_rh_glitch(in);
}

#endif
