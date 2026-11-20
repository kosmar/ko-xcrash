/* SPDX-License-Identifier: GPL-3.0-only */
/* KO VOICES UI chrome shared by the genre apps (fm_app.c) and KO DECK: knob pots, knob acceleration,
 * knob flash text, long-press timing, edit snap, bank LED pulse, groove wave text, knob bar, top bar, toast
 * renderer. Header-only; expects cv_* / cv_draw_toast* / text_w / FONT_S / RGB / C_* / fm1_ms from platform.c +
 * gfx.c to be in scope. */
#ifndef KO_CHROME_H
#define KO_CHROME_H

#include <stdint.h>

/* Largest TICK pot list (static per-tick tables). Define before including to match the app's list size. */
#ifndef KO_POT_TICK_CAP
#define KO_POT_TICK_CAP 32u
#endif

/* ---- knob acceleration ---- */

static inline int32_t ko_knob_gain(uint32_t *last_ms, uint32_t now, int32_t steps)
{
    uint32_t dt = now - *last_ms;
    int32_t a = steps < 0 ? -steps : steps;
    int32_t mul = 1;

    *last_ms = now;
    /* Dynamic velocity acceleration:
     * - slow turn (dt > 90 ms): 1x per click (exact 1-unit precision)
     * - medium turn (45..90 ms): 2x
     * - fast turn (22..44 ms): 4x
     * - rapid flick (< 22 ms or multiple detents in 1 poll): 7x..10x */
    if (a >= 3 || dt < 16u)
        mul = 9;
    else if (a >= 2 || dt < 28u)
        mul = 5;
    else if (dt < 55u)
        mul = 3;
    else if (dt < 95u)
        mul = 2;
    return steps * mul;
}

/* ---- knob flash text (short value/state word in the top bar) ---- */

typedef struct {
    uint32_t until_ms;
    char msg[8];
} ko_flash_t;

static inline void ko_flash_set(ko_flash_t *f, const char *msg, uint32_t now, uint32_t hold_ms)
{
    uint32_t i;

    if (!msg)
        return;
    for (i = 0; i < 7u && msg[i]; i++)
        f->msg[i] = msg[i];
    f->msg[i] = '\0';
    f->until_ms = now + hold_ms;
}

static inline int ko_flash_active(const ko_flash_t *f, uint32_t now)
{
    return now < f->until_ms && f->msg[0];
}

/* ---- long press (timing only; the caller owns the state fields and the action) ---- */

/* used / cancel may be NULL when the caller tracks them itself. */
typedef struct {
    const uint32_t *press_ms;
    uint8_t *used;
    uint8_t *cancel;
} ko_longpress_t;

/* 1 exactly once when the hold reaches hold_ms without a cancel (then *used = 1). cancel_now latches *cancel. */
static inline int ko_longpress_poll(const ko_longpress_t *lp, uint32_t now, uint32_t hold_ms, int cancel_now)
{
    if (lp->used && *lp->used)
        return 0;
    if (lp->cancel) {
        if (*lp->cancel)
            return 0;
        if (cancel_now) {
            *lp->cancel = 1u;
            return 0;
        }
    }
    if (now - *lp->press_ms < hold_ms)
        return 0;
    if (lp->used)
        *lp->used = 1u;
    return 1;
}

/* ---- edit snap (EDIT + knob: CCW = default, CW = max) ---- */

static inline void ko_edit_snap(int32_t steps, uint8_t *v, uint8_t def, uint8_t max)
{
    if (steps != 0)
        *v = steps < 0 ? def : max;
}

/* ---- bank button LEDs ---- */

static inline int ko_led_pulse(uint32_t now)
{
    uint32_t phase = (now / 110u) % 8u;

    return phase == 0u || phase == 2u;
}

enum { KO_LED_OFF = 0, KO_LED_ON, KO_LED_DIM };

/* Bank button: off when not shown, lit on page 1, pulsing (lit / dim) on page 2. */
static inline uint8_t ko_bank_led(int active, int p2, int pulse)
{
    if (!active)
        return KO_LED_OFF;
    return !p2 || pulse ? KO_LED_ON : KO_LED_DIM;
}

/* ---- knob pots ---- */

static const uint16_t KNOB_POT_SIN_Q8[65] = {
    0, 6, 13, 19, 25, 31, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92, 98, 104, 109, 115, 121, 126,
    132, 137, 142, 147, 152, 157, 162, 167, 172, 177, 181, 185, 190, 194, 198, 202, 206, 209, 213, 216,
    220, 223, 226, 229, 231, 234, 237, 239, 241, 243, 245, 247, 248, 250, 251, 252, 253, 254, 255, 255,
    256, 256, 256
};

/* t in pi/128 units, 0..128 (0 = left, 64 = bottom, 128 = right). */
static inline void knob_pot_dir(uint32_t t, int32_t *c, int32_t *s)
{
    if (t > 128u)
        t = 128u;
    *s = (int32_t)KNOB_POT_SIN_Q8[t <= 64u ? t : 128u - t];
    *c = t <= 64u ? (int32_t)KNOB_POT_SIN_Q8[64u - t] : -(int32_t)KNOB_POT_SIN_Q8[t - 64u];
}

static inline int32_t knob_pot_cov(int32_t q8)
{
    return q8 < 0 ? 0 : (q8 > 256 ? 256 : q8);
}

/* Inner-edge coverage for a radius in 1/16 px (d2 in half-pixel units squared). */
static inline int32_t knob_pot_in_cov(int32_t d2, int32_t r_q4)
{
    return knob_pot_cov(((d2 * 64 - r_q4 * r_q4) * 8) / r_q4 + 128);
}

enum { KNOB_POT_CONT = 0, KNOB_POT_ORD, KNOB_POT_SEL, KNOB_POT_TOG, KNOB_POT_SHAPE, KNOB_POT_SELW, KNOB_POT_MASK,
       KNOB_POT_TICK, KNOB_POT_HALF };
#define KNOB_POT_MAX_STEPS 8u
/* SEL: up to 9 wedges (KO BASS SCALE uses 8: MINOR..HARM.M). */
#define KNOB_POT_SEL_MAX_STEPS 9u
/* MASK (ensemble donut): 6 wedges, one per DUB lane. Lane k = mute mask bit k = wedge k.
 * Wedge 0 starts at the right end (step 0), wedge 5 ends at the left end. Lit = lane plays (bit clear),
 * unlit = outline in track colour. Layout is mirror-symmetric about the vertical axis (see knob_pot_wedges). */
#define KNOB_POT_MASK_N 6u
/* DECK kit COMBO: one wedge per voice; v/custom_arg = voice-on bitmap (bit set = plays). */
#define KNOB_POT_MASK_DECK_N 16u
#define KNOB_POT_ORD_MAX_STEPS 18u
/* LFO shape pure anchors (fm_lfo_shape_pure_name), raw 0..127. SHAPE draws one wedge per anchor. */
#define KNOB_POT_SHAPE_N 11u
static const uint8_t KNOB_POT_SHAPE_ANCHOR[KNOB_POT_SHAPE_N] = {0, 15, 22, 30, 45, 60, 75, 90, 102, 114, 127};

/* Largest step count that still gets a discrete pot drawing for this mode. */
static inline uint8_t knob_pot_max_steps(uint8_t mode, uint8_t tick_max)
{
    /* TICK: one tick per list entry; the count is runtime, capped by the caller (tick_max <= KO_POT_TICK_CAP). */
    if (mode == KNOB_POT_TICK)
        return tick_max;
    if (mode == KNOB_POT_SEL)
        return KNOB_POT_SEL_MAX_STEPS;
    return mode == KNOB_POT_ORD || mode == KNOB_POT_SHAPE || mode == KNOB_POT_SELW ? (uint8_t)KNOB_POT_ORD_MAX_STEPS :
           (uint8_t)KNOB_POT_MAX_STEPS;
}

/* SHAPE: raw value v as position in 1/256 anchor units (anchor index * 256 + linear fraction to the next anchor). */
static inline int32_t knob_pot_shape_pos(uint8_t v)
{
    uint32_t i = 0u;
    int32_t frac;

    while (i + 1u < KNOB_POT_SHAPE_N && v >= KNOB_POT_SHAPE_ANCHOR[i + 1u])
        i++;
    if (i + 1u >= KNOB_POT_SHAPE_N)
        return (int32_t)(i * 256u);
    frac = ((int32_t)v - (int32_t)KNOB_POT_SHAPE_ANCHOR[i]) * 256 /
           ((int32_t)KNOB_POT_SHAPE_ANCHOR[i + 1u] - (int32_t)KNOB_POT_SHAPE_ANCHOR[i]);
    return (int32_t)(i * 256u) + frac;
}

/* t16 in pi/2048 units, 0..2048 (knob_pot_dir at 1/16 resolution). */
static inline void knob_pot_dir_fine(uint32_t t16, int32_t *c, int32_t *s)
{
    int32_t c0, s0, c1, s1;
    int32_t f = (int32_t)(t16 & 15u);

    knob_pot_dir(t16 >> 4, &c0, &s0);
    knob_pot_dir((t16 >> 4) + 1u, &c1, &s1);
    *c = c0 + ((c1 - c0) * f) / 16;
    *s = s0 + ((s1 - s0) * f) / 16;
}

/* Discrete pot: n donut wedges across the half ring (hole radius RH = 10 keeps the LFO mark free), 1 px apart,
 * step 0 at the right like knob_pot_draw.
 * Every discrete mode draws the full wedge set: the active wedge(s) filled, all others outlined in the track
 * colour (same piece outlines as SHAPE). ORD wedges may overlap when n is large.
 * TOG is a single wedge filled when on; its unfilled wedge is outlined in the track colour. */
static inline void knob_pot_wedges(int32_t cx, int32_t cy, uint8_t n, uint8_t mode, uint16_t ix, uint16_t fill,
                            uint16_t track, uint16_t bg)
{
    const int32_t R = 27;
    const int32_t RH = 10;
    int32_t bc[KNOB_POT_ORD_MAX_STEPS + 1u], bs[KNOB_POT_ORD_MAX_STEPS + 1u];
    int32_t px, py;
    int32_t sp = mode == KNOB_POT_SHAPE ? knob_pot_shape_pos(ix) : 0;
    uint32_t k;

    for (k = 0; k <= n; k++) {
        /* MASK: mirror the edges around the centre axis (k and n-k sum to 2048) so integer rounding stays symmetric. */
        uint32_t t16 = k * 2048u / n;

        if (mode == KNOB_POT_MASK && k > n / 2u)
            t16 = 2048u - (n - k) * 2048u / n;
        knob_pot_dir_fine(t16, &bc[k], &bs[k]);
    }
    for (py = cy; py <= cy + R; py++) {
        int32_t W = 2 * (py - cy) + 1;

        for (px = cx - R - 1; px <= cx + R; px++) {
            int32_t U = 2 * (cx - px) - 1;
            int32_t d2 = U * U + W * W;
            int32_t out = knob_pot_cov(((4 * R * R - d2) * 32) / R + 128);
            int32_t out_in = knob_pot_cov(((4 * (R - 1) * (R - 1) - d2) * 32) / (R - 1) + 128);
            int32_t hub = knob_pot_cov(((d2 - 4 * RH * RH) * 32) / RH + 128);
            int32_t hub_in = knob_pot_cov(((d2 - 4 * (RH + 1) * (RH + 1)) * 32) / (RH + 1) + 128);
            int32_t base = out < hub ? out : hub;
            int32_t base_in = out_in < hub_in ? out_in : hub_in;
            int32_t fcov = 0, ocov = 0;
            uint16_t col;

            if (base == 0)
                continue;
            for (k = 0; k < n; k++) {
                int32_t da = (W * bc[k] - U * bs[k]) / 2 + 128 - (k ? 128 : 0);
                int32_t db = (U * bs[k + 1u] - W * bc[k + 1u]) / 2 + 128 - (k + 1u < n ? 128 : 0);
                int32_t ca = knob_pot_cov(da), cb = knob_pot_cov(db);
                int32_t w = base < ca ? base : ca;
                uint32_t st = n - 1u - k;
                int on = mode == KNOB_POT_MASK ?
                             (n == KNOB_POT_MASK_DECK_N ? ((ix >> k) & 1u) != 0u : ((ix >> k) & 1u) == 0u) :
                         mode == KNOB_POT_ORD || mode == KNOB_POT_SEL || mode == KNOB_POT_SELW ? st == ix :
                         mode == KNOB_POT_HALF ? (ix != 0u) == (U < 0) : ix != 0u;

                if (cb < w)
                    w = cb;
                if (w == 0)
                    continue;
                if (mode == KNOB_POT_SHAPE) {
                    /* Fill weight 256 at this wedge's anchor, linear to 0 at the neighbouring anchors. */
                    int32_t sd = sp - (int32_t)st * 256;
                    int32_t sf = (sd < 0 ? -sd : sd) >= 256 ? 0 : 256 - (sd < 0 ? -sd : sd);

                    on = 0;
                    if (sf > 0 && (w < sf ? w : sf) > fcov)
                        fcov = w < sf ? w : sf;
                }
                if (on) {
                    if (w > fcov)
                        fcov = w;
                } else {
                    /* Unlit wedge: outline only in the track colour, same look for every discrete mode. */
                    int32_t wi = base_in;
                    int32_t ia = knob_pot_cov(da - 256), ib = knob_pot_cov(db - 256);

                    if (ia < wi)
                        wi = ia;
                    if (ib < wi)
                        wi = ib;
                    if (w - wi > ocov)
                        ocov = w - wi;
                }
            }
            col = cv_blend_on_bg(track, bg, (uint32_t)ocov >> 4);
            col = cv_blend_on_bg(fill, col, (uint32_t)fcov >> 4);
            cv_pset(px, py, col);
        }
    }
}

/* TICK pot (LFO target list): n radial ticks, one per list position, 1 px wide, each centred in its equal sector
 * of the half ring (same sector layout as knob_pot_wedges: step 0 at the right, step n-1 at the left, mirror-
 * symmetric). Ticks span radius KNOB_POT_TICK_RI..R, so the hole (RH) stays free. Track ticks use max coverage,
 * the active tick (v = knob value of the position) is drawn in fill on top, so no pixel is doubled. */
#define KNOB_POT_TICK_RI 18
static inline void knob_pot_ticks(int32_t cx, int32_t cy, uint8_t n, uint8_t v, uint16_t fill, uint16_t track, uint16_t bg)
{
    const int32_t R = 27;
    const int32_t RI = KNOB_POT_TICK_RI;
    static int32_t tc[KO_POT_TICK_CAP], ts[KO_POT_TICK_CAP];
    /* knob_val = pos * 127 / (n - 1), so ceil(v * (n - 1) / 127) recovers pos exactly for n - 1 < 127. */
    uint32_t ix = ((uint32_t)v * (n - 1u) + 126u) / 127u;
    uint32_t k;
    int32_t px, py;

    if (ix > n - 1u)
        ix = n - 1u;
    for (k = 0; k < n; k++)
        knob_pot_dir_fine((2u * k + 1u) * 1024u / n, &tc[k], &ts[k]);
    for (py = cy; py <= cy + R; py++) {
        int32_t W = 2 * (py - cy) + 1;

        for (px = cx - R - 1; px <= cx + R; px++) {
            int32_t U = 2 * (cx - px) - 1;
            int32_t d2 = U * U + W * W;
            int32_t ocov = 0, fcov = 0;
            uint16_t col;

            if (d2 > 4 * (R + 1) * (R + 1) || d2 < 4 * (RI - 1) * (RI - 1))
                continue;
            for (k = 0; k < n; k++) {
                int32_t D = (W * tc[k] - U * ts[k]) / 2;
                int32_t q = (U * tc[k] + W * ts[k]) / 2; /* 256 * radius along the tick */
                int32_t band = knob_pot_cov(256 - (D < 0 ? -D : D));
                int32_t cov_o = knob_pot_cov(256 * R + 128 - q);
                int32_t cov_i = knob_pot_cov(q - 256 * RI + 128);
                int32_t w = band < cov_o ? band : cov_o;

                if (cov_i < w)
                    w = cov_i;
                if (w == 0)
                    continue;
                if (k == n - 1u - ix) {
                    if (w > fcov)
                        fcov = w;
                } else if (w > ocov) {
                    ocov = w;
                }
            }
            if (ocov == 0 && fcov == 0)
                continue;
            col = cv_blend_on_bg(track, bg, (uint32_t)ocov >> 4);
            col = cv_blend_on_bg(fill, col, (uint32_t)fcov >> 4);
            cv_pset(px, py, col);
        }
    }
}

/* Half-disk pot, bowl opening downward from the flat edge at y = cy. Thin track, fill arc whose
 * thickness grows with |value| (bipolar: distance from centre), AA.
 * Scale is mirrored so the sweep matches a clockwise knob: min at the right (t=128), max at the
 * left (t=0), centre of a bipolar pot stays at the bottom. steps 2..8 (TOG: any) with a KNOB_POT_*
 * mode draws wedges instead (steps 2..9 for SEL, 2..16 for ORD; v is then the bar value of the step, 0..127). */
/* Active value-arc fill: modest lift toward white on black (track stays separate/dimmer). */
#ifndef KNOB_POT_FILL_LIFT16
#define KNOB_POT_FILL_LIFT16 3u /* /16 toward white (~19%) */
#endif

static inline uint16_t knob_pot_fill_col(uint16_t fill)
{
    uint32_t t = (uint32_t)KNOB_POT_FILL_LIFT16;
    uint32_t r = (uint32_t)fill >> 11;
    uint32_t g = ((uint32_t)fill >> 5) & 63u;
    uint32_t b = (uint32_t)fill & 31u;

    r = r + ((31u - r) * t) / 16u;
    g = g + ((63u - g) * t) / 16u;
    b = b + ((31u - b) * t) / 16u;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static inline void knob_pot_draw(int32_t cx, int32_t cy, uint8_t v, int bipolar, uint8_t steps, uint8_t mode,
                          uint16_t fill, uint16_t track, uint16_t bg, uint32_t mask_ext)
{
    const int32_t R = 27;
    const int32_t TRACK_Q4 = 2 * 16;
    const int32_t FILL_MIN_Q4 = 2 * 16;
    const int32_t FILL_MAX_Q4 = 9 * 16;
    /* Bipolar: raw 64 must land exactly on the centre (64) so 0 draws no fill. */
    uint32_t tv = !bipolar ? ((uint32_t)v * 128u + 63u) / 127u :
                  v <= 64u ? (uint32_t)v : 64u + (((uint32_t)(v > 127u ? 127u : v) - 64u) * 64u + 31u) / 63u;
    /* Unipolar: mirror for clockwise knob (min right, max left). Bipolar centre-64 (bipolar == 2): +N grows
     * left like unipolar. bipolar == 1 (PAN): v<64 → left arc, v>64 → right arc, so L/R reads true. */
    uint32_t pos = bipolar == 1 ? tv : (128u - tv);
    uint32_t tlo = bipolar ? (pos < 64u ? pos : 64u) : pos;
    uint32_t thi = bipolar ? (pos > 64u ? pos : 64u) : 128u;
    int32_t mag = bipolar ? (((int32_t)v > 64 ? (int32_t)v - 64 : 64 - (int32_t)v) * 127) / 64 : (int32_t)v;
    int32_t rt_q4 = R * 16 - TRACK_Q4;
    int32_t rf_q4;
    int32_t lc, ls, hc, hs;
    int32_t px, py;

    if (mode == KNOB_POT_TOG || mode == KNOB_POT_HALF) {
        /* HALF (INVERT): v >= 64 = tails, right half filled; heads fills the left half. */
        knob_pot_wedges(cx, cy, 1u, mode, v >= 64u, fill, track, bg);
        return;
    }
    if (mode == KNOB_POT_TICK) {
        /* TICK: v is the knob value of the list position (0..127), steps = list length. */
        knob_pot_ticks(cx, cy, steps, v, fill, track, bg);
        return;
    }
    if (mode == KNOB_POT_MASK) {
        uint8_t n = steps == KNOB_POT_MASK_DECK_N ? KNOB_POT_MASK_DECK_N : KNOB_POT_MASK_N;
        uint16_t ix = n == KNOB_POT_MASK_DECK_N ? (uint16_t)(mask_ext & 0xFFFFu) :
                                                    (uint16_t)(v & 0x3Fu);

        /* 6-lane dub: ix = mute mask (bit set = muted). DECK 16: mask_ext = on mask (bit set = plays). */
        knob_pot_wedges(cx, cy, n, mode, ix, fill, track, bg);
        return;
    }
    if (mode == KNOB_POT_SHAPE) {
        /* SHAPE: ix is the raw shape value (fractional blend between anchors), not a step index. */
        knob_pot_wedges(cx, cy, KNOB_POT_SHAPE_N, mode, v, fill, track, bg);
        return;
    }
    if (mode != KNOB_POT_CONT && steps >= 2u && steps <= knob_pot_max_steps(mode, (uint8_t)KO_POT_TICK_CAP)) {
        knob_pot_wedges(cx, cy, steps, mode, (uint8_t)(((uint32_t)v * (steps - 1u) + 63u) / 127u), fill, track,
                        bg);
        return;
    }
    if (mag > 127)
        mag = 127;
    rf_q4 = R * 16 - (FILL_MIN_Q4 + ((FILL_MAX_Q4 - FILL_MIN_Q4) * mag) / 127);

    knob_pot_dir(tlo, &lc, &ls);
    knob_pot_dir(thi, &hc, &hs);
    for (py = cy; py <= cy + R; py++) {
        int32_t W = 2 * (py - cy) + 1;

        for (px = cx - R - 1; px <= cx + R; px++) {
            int32_t U = 2 * (cx - px) - 1;
            int32_t d2 = U * U + W * W;
            int32_t out = knob_pot_cov(((4 * R * R - d2) * 32) / R + 128);
            int32_t t_in = knob_pot_in_cov(d2, rt_q4);
            int32_t f_in = knob_pot_in_cov(d2, rf_q4);
            int32_t ring = out < t_in ? out : t_in;
            int32_t band = out < f_in ? out : f_in;
            int32_t fcov = 0;
            uint16_t col;

            if (ring == 0 && band == 0)
                continue;
            if (band > 0 && thi > tlo) {
                int32_t a_hi = knob_pot_cov((U * hs - W * hc) / 2 + 128);
                int32_t a_lo = tlo == 0u ? 256 : knob_pot_cov((W * lc - U * ls) / 2 + 128);

                fcov = a_hi < a_lo ? a_hi : a_lo;
                if (fcov > band)
                    fcov = band;
            }
            col = cv_blend_on_bg(track, bg, (uint32_t)ring >> 4);
            col = cv_blend_on_bg(fill, col, (uint32_t)fcov >> 4);
            cv_pset(px, py, col);
        }
    }
}

/* ---- groove wave text ---- */

/* Unpack RGB565, modulate saturation/luminance with a rhythmic wave (glow -128..+256). */
static inline uint16_t ko_color_sat_wave(uint16_t base_col, int32_t glow)
{
    int32_t r = ((base_col >> 11) & 31) << 3;
    int32_t g_c = ((base_col >> 5) & 63) << 2;
    int32_t b = (base_col & 31) << 3;

    if (glow >= 0) {
        /* Swell towards hot tape-saturated cream/gold peak */
        r = r + (((255 - r) * glow) >> 8);
        g_c = g_c + (((240 - g_c) * glow) >> 8);
        b = b + (((200 - b) * (glow * 3 >> 2)) >> 8);
    } else {
        /* Dip into deep shadow between skanks */
        int32_t att = 256 + glow;
        if (att < 96)
            att = 96;
        r = (r * att) >> 8;
        g_c = (g_c * att) >> 8;
        b = (b * att) >> 8;
    }
    if (r > 255) r = 255;
    if (g_c > 255) g_c = 255;
    if (b > 255) b = 255;
    return RGB(r, g_c, b);
}

/* static_glow: every character gets `glow`. Otherwise a travelling wave across the characters:
 * w1 = sine(skank_ph - idx * idx_ph), w2 = sine(step_ph * 2 - idx * 0x26000000), scaled by energy (<= 256). */
typedef struct {
    int static_glow;
    int32_t glow;
    uint32_t step_ph;
    uint32_t skank_ph;
    uint32_t idx_ph;
    int32_t energy;
    int32_t (*sine_q64)(uint32_t ph);
} ko_wave_ctx_t;

static inline int32_t ko_draw_wave_str(const ko_wave_ctx_t *w, int32_t x, int32_t y, const char *str,
                                       uint16_t base_col, uint32_t char_ofs)
{
    char ch_buf[2];
    uint32_t idx = char_ofs;

    ch_buf[1] = 0;
    while (*str) {
        int32_t glow = w->glow;

        if (!w->static_glow) {
            int32_t w1 = w->sine_q64(w->skank_ph - idx * w->idx_ph);
            int32_t w2 = w->sine_q64((w->step_ph << 1) - idx * 0x26000000u);

            glow = (((w1 * 3 + w2) >> 2) * w->energy) >> 15;
        }
        ch_buf[0] = *str++;
        cv_text(x, y, &FONT_S, ch_buf, ko_color_sat_wave(base_col, glow));
        x += 8;
        idx++;
    }
    return x;
}

/* ---- knob bar ---- */

/* DJ mark: filled 4x4 square at bottom centre (x+28..x+31, y 74..77). Distinct from the LFO wave glyph. */
static inline void ko_draw_lfo_mark(int32_t x, uint16_t col)
{
    int32_t px, py;

    for (py = 74; py <= 77; py++)
        for (px = x + 28; px <= x + 31; px++)
            cv_pset(px, py, col);
}

/* LFO mark in the hollow centre of the pot (hole centre at x+30, flat edge at y = 38). */
static inline void ko_draw_lfo_center_mark(int32_t x, uint16_t col)
{
    static const int8_t dy[11] = {0, 1, 2, 2, 1, 0, -1, -2, -2, -1, 0};
    const int32_t x0 = x + 25;
    const int32_t y = 42;
    int i;

    for (i = 0; i < 10; i++)
        cv_line(x0 + i, y + (int32_t)dy[i], x0 + i + 1, y + (int32_t)dy[i + 1], col);
}

/* Morph direction glyph in the hollow centre of the pot, same spot as the LFO wave (x+25..35, y 40..44).
 * to > from: up-chevron (∧); to < from: down-chevron (∨). Drawn instead of the LFO wave while morphing. */
static inline void ko_draw_morph_chevron(int32_t xl, uint8_t from, uint8_t to, uint16_t col)
{
    const int32_t x0 = xl + 25;

    if (to > from) {
        cv_line(x0, 44, x0 + 5, 40, col);
        cv_line(x0 + 5, 40, x0 + 10, 44, col);
    } else if (to < from) {
        cv_line(x0, 40, x0 + 5, 44, col);
        cv_line(x0 + 5, 44, x0 + 10, 40, col);
    }
}

/* One 60 px knob column, filled by the app per frame. custom_label / custom_value (non-NULL) replace the
 * label text / value text; custom_value returns the x after the value (song text goes beside it). */
typedef struct ko_knob_slot ko_knob_slot_t;
struct ko_knob_slot {
    char label[16];
    char value[16];
    char song[8];
    /* Fast LFO: held min/max (no +). Drawn hi left / lo right with a mid hairline. */
    char range_lo[6];
    char range_hi[6];
    uint16_t klab, vcol, lcol, pot_col, pot_track;
    uint8_t show_song;
    uint8_t off; /* 1: empty column, nothing drawn (SEQ page 5 has no knobs) */
    uint8_t pan;
    int8_t pan_side;
    char pan_num[4];
    uint8_t pv, bip, steps, mode;
    uint8_t morph, mf, mt, lfo, dj;
    uint8_t range;
    uint16_t mark_col, dj_col;
    uint32_t custom_arg;
    void (*custom_label)(const ko_knob_slot_t *k, int32_t x);
    int32_t (*custom_value)(const ko_knob_slot_t *k, int32_t x);
};

typedef struct {
    int32_t h, blit_y, rail_y;
    uint8_t center_label;
    void (*rail)(int32_t y_bot);
    void (*fill)(uint32_t col, ko_knob_slot_t *k);
} ko_knob_bar_t;

static inline void ko_draw_knob_slot(const ko_knob_bar_t *bar, const ko_knob_slot_t *k, int32_t x)
{
    if (k->custom_label) {
        k->custom_label(k, x);
    } else if (bar->center_label) {
        int32_t tw = text_w(&FONT_S, k->label);
        int32_t lx = x + (60 - tw) / 2;

        if (lx < x + 2)
            lx = x + 2;
        cv_text(lx, 2, &FONT_S, k->label, k->klab);
    } else {
        cv_text(x + 4, 2, &FONT_S, k->label, k->klab);
    }
    if (k->pan) {
        const int32_t bar_l = x + 6;
        const int32_t bar_r = x + 54;

        if (k->pan_side == 0) {
            const char *ctr = "CTR";
            int32_t tx = bar_l + (48 - text_w(&FONT_S, ctr)) / 2;

            cv_text(tx, 18, &FONT_S, ctr, k->vcol);
        } else if (k->pan_side < 0) {
            cv_text(x + 4, 18, &FONT_S, "L", k->vcol);
            cv_text(bar_r - text_w(&FONT_S, k->pan_num), 18, &FONT_S, k->pan_num, k->vcol);
        } else {
            cv_text(x + 4, 18, &FONT_S, k->pan_num, k->vcol);
            cv_text(bar_r - text_w(&FONT_S, "R"), 18, &FONT_S, "R", k->vcol);
        }
    } else if (k->range && !k->custom_value) {
        /* hi left, lo right (like pan L/R). Mid hairline separates without eating digit space. */
        const int32_t y = 18;
        const int32_t bar_l = x + 4;
        const int32_t bar_r = x + 56;
        int32_t lo_w = text_w(&FONT_S, k->range_lo);

        cv_text(bar_l, y, &FONT_S, k->range_hi, k->vcol);
        cv_text(bar_r - lo_w, y, &FONT_S, k->range_lo, k->vcol);
        cv_line(x + 29, y + 1, x + 29, y + 11, C_DIM);
    } else {
        int32_t vx = k->custom_value ? k->custom_value(k, x) : cv_text(x + 4, 18, &FONT_S, k->value, k->vcol);

        if (k->show_song)
            cv_text(vx + 4, 18, &FONT_S, k->song, k->lcol);
    }
    knob_pot_draw(x + 30, 38, k->pv, k->bip ? (k->pan ? 1 : 2) : 0, k->steps, k->mode, knob_pot_fill_col(k->pot_col),
                  k->pot_track ? k->pot_track : C_GRAY, C_BLACK, k->custom_arg);
    if (k->morph)
        ko_draw_morph_chevron(x, k->mf, k->mt, k->mark_col);
    else if (k->lfo)
        ko_draw_lfo_center_mark(x, k->mark_col);
    if (k->dj)
        ko_draw_lfo_mark(x, k->dj_col);
}

/* 4-column knob bar: rule, progress rail (optional callback), per column label, value or pan, pot, markers. */
static inline void ko_draw_knob_bar(const ko_knob_bar_t *bar)
{
    uint32_t col;

    cv_begin(240, bar->h, C_BLACK);
    cv_line(0, 0, 239, 0, C_LINE);
    if (bar->rail)
        bar->rail(bar->rail_y);
    for (col = 0; col < 4u; col++) {
        ko_knob_slot_t k = {0};

        bar->fill(col, &k);
        if (k.off)
            continue;
        ko_draw_knob_slot(bar, &k, (int32_t)col * 60);
    }
    cv_blit(0, bar->blit_y);
}

/* ---- top bar (240 x 28) ---- */

enum {
    KO_TOP_BAR_W = 240,
    KO_TOP_BAR_H = 28,
    KO_TOP_BAR_RIGHT = 236,
    KO_TOP_BAR_HALF_CH = 4 /* 0.5 character @ FONT_S (8 px cell) */
};

/* Filled by the app per frame. Row 1: wave title + subtitle (left), REC dot + page dots (right).
 * Row 2: stage word, SEQ step text, centre extra (left/middle), key, scale, swing, BPM (right), centre flash.
 * Strings that are empty are not drawn (swing, seq_txt). */
typedef struct ko_top_bar ko_top_bar_t;
struct ko_top_bar {
    const ko_wave_ctx_t *wave;
    uint8_t show_title;
    char title[32];
    char sub[32];
    uint16_t title_col, sub_col;
    int8_t cursor; /* >= 0: edit cursor under that title character */
    uint8_t rec;
    uint8_t pages, page;
    void (*dots)(const ko_top_bar_t *t); /* non-NULL replaces the page dots */
    const char *stage;
    uint8_t seq;                          /* SEQ view: step text after the stage, key/scale clipped */
    int32_t seq_x0, seq_w;                /* centre zone kept free for the SEQ step dots */
    void (*seq_dots)(const ko_top_bar_t *t);
    char seq_txt[2][8];
    uint16_t seq_col[2];
    uint8_t mid_busy;
    void (*mid)(const ko_top_bar_t *t);   /* centre extra, skipped while mid_busy or the flash is up */
    const char *key, *scale;
    char swing[8];
    char bpm[8];
    uint8_t playing, on_one, on_beat;
    const char *overlay; /* message drawn over the bar elsewhere: hides key/scale, may block the flash */
    const ko_flash_t *flash;
    uint32_t now;
};

static inline void ko_draw_top_bar(const ko_top_bar_t *t)
{
    int32_t x0 = KO_TOP_BAR_RIGHT - (int32_t)t->pages * 8;
    int32_t bpm_w = text_w(&FONT_S, t->bpm);
    int32_t bx = KO_TOP_BAR_RIGHT - bpm_w;
    int32_t key_w = text_w(&FONT_S, t->key);
    int32_t sc_w = text_w(&FONT_S, t->scale);
    int32_t right_x = bx;
    int32_t sw_x = bx, sc_x, key_x;
    int show_swing = t->swing[0] != 0;
    int flash_on = t->flash && ko_flash_active(t->flash, t->now);
    int mid_busy = t->mid_busy || flash_on;
    int mid_flash_busy = t->overlay && 4 + text_w(&FONT_S, t->overlay) > 52;
    uint16_t bpm_col = !t->playing ? C_DIM : (t->on_one ? C_ACC : (t->on_beat ? C_WHITE : C_HI));
    uint32_t pi;

    if (show_swing) {
        sw_x = right_x - KO_TOP_BAR_HALF_CH - text_w(&FONT_S, t->swing);
        right_x = sw_x;
    }
    sc_x = right_x - KO_TOP_BAR_HALF_CH - sc_w;
    key_x = sc_x - KO_TOP_BAR_HALF_CH - key_w;

    cv_begin(KO_TOP_BAR_W, KO_TOP_BAR_H, C_BLACK);
    if (t->show_title) {
        int32_t ex = ko_draw_wave_str(t->wave, 4, 2, t->title, t->title_col, 0u);

        ko_draw_wave_str(t->wave, ex + 8, 2, t->sub, t->sub_col, (uint32_t)((ex - 4) / 8) + 1u);
        if (t->cursor >= 0)
            cv_rect(4 + (int32_t)t->cursor * 8, 18, 7, 2, C_WHITE);
    }
    if (t->rec)
        cv_rect(x0 - 12, 4, 5, 5, RGB(255, 48, 48));
    if (t->dots)
        t->dots(t);
    else
        for (pi = 0; pi < t->pages; pi++)
            cv_rect(x0 + (int32_t)pi * 8, 4, 5, 5, pi == t->page ? C_WHITE : C_DIM);
    cv_text(4, 14, &FONT_S, t->stage, C_WHITE);
    if (t->seq) {
        int32_t lx = 4 + text_w(&FONT_S, t->stage);
        int32_t left_lim = t->seq_x0 - KO_TOP_BAR_HALF_CH;

        if (t->seq_dots)
            t->seq_dots(t);
        if (t->seq_txt[0][0]) {
            int32_t w = text_w(&FONT_S, t->seq_txt[0]);

            lx += KO_TOP_BAR_HALF_CH;
            if (lx + w <= left_lim) {
                cv_text(lx, 14, &FONT_S, t->seq_txt[0], t->seq_col[0]);
                lx += w;
            }
            lx += KO_TOP_BAR_HALF_CH;
            if (t->seq_txt[1][0] && lx + text_w(&FONT_S, t->seq_txt[1]) <= left_lim)
                cv_text(lx, 14, &FONT_S, t->seq_txt[1], t->seq_col[1]);
        }
    }
    if (!mid_busy && t->mid)
        t->mid(t);
    if (!t->overlay) {
        int32_t dots_right = t->seq_x0 + t->seq_w + KO_TOP_BAR_HALF_CH;
        int draw_key = key_x >= dots_right && key_x + key_w <= sc_x - KO_TOP_BAR_HALF_CH;
        int draw_sc = sc_x >= dots_right && sc_x + sc_w <= (show_swing ? sw_x : bx) - KO_TOP_BAR_HALF_CH;

        if (!t->seq) {
            draw_key = 1;
            draw_sc = 1;
        } else if (!draw_sc)
            draw_key = 0;
        if (draw_sc)
            cv_text(sc_x, 14, &FONT_S, t->scale, C_WHITE);
        if (draw_key)
            cv_text(key_x, 14, &FONT_S, t->key, C_WHITE);
    }
    if (show_swing)
        cv_text(sw_x, 14, &FONT_S, t->swing, bpm_col);
    if (t->playing && t->on_one) {
        cv_rect(bx, 14, bpm_w, 16, C_ACC);
        cv_text(bx, 14, &FONT_S, t->bpm, C_BLACK);
    } else if (t->playing && t->on_beat) {
        cv_rect(bx, 14, bpm_w, 16, C_WHITE);
        cv_text(bx, 14, &FONT_S, t->bpm, C_BLACK);
    } else
        cv_text(bx, 14, &FONT_S, t->bpm, !t->playing ? C_DIM : C_HI);
    if (!mid_flash_busy && flash_on) {
        int32_t fw = text_w(&FONT_S, t->flash->msg);
        int32_t fx = KO_TOP_BAR_W / 2 - fw / 2;

        if (fx + fw < key_x - KO_TOP_BAR_HALF_CH)
            cv_text(fx, 14, &FONT_S, t->flash->msg, C_ACC);
    }
    cv_blit(0, 0);
}

/* ---- toast renderer (lane policy and lane state live in perf_toast_policy.h / the app) ---- */

enum { KO_TOAST_Y0 = 28, KO_TOAST_CANVAS_H = 132 };

/* Top y of a toast stack of height bh, centred in the canvas below the top bar. */
static inline int32_t ko_toast_by_center(int32_t bh)
{
    return KO_TOAST_Y0 + (KO_TOAST_CANVAS_H - bh) / 2;
}

/* Slide-in el ms after the start: 1 while el < ms (slide 32..0 px, dither 16..2), 0 when done. */
static inline int ko_toast_slide(uint32_t el, uint32_t ms, int32_t *slide, uint8_t *dith)
{
    uint8_t d;

    if (el >= ms)
        return 0;
    *slide = (int32_t)((ms - el) * 32u / ms);
    d = (uint8_t)(16u - (el * 14u / ms));
    *dith = d < 2u ? 2u : d;
    return 1;
}

/* One toast badge. side non-NULL: msg + side badge, the side part ends at end_ms. */
typedef struct {
    const char *msg;
    const char *side;
    uint8_t kind;
    uint32_t end_ms;
    int32_t by;
    uint16_t bar;
    int32_t slide;
    uint8_t dith;
} ko_toast_t;

static inline void ko_draw_toast(const ko_toast_t *t, uint32_t now)
{
    if (t->side)
        cv_draw_toast_arp_note(&FONT_S, t->msg, t->side, t->kind, now, t->end_ms, t->by, t->bar, t->slide, t->dith,
                               0, KO_TOAST_Y0);
    else
        cv_draw_toast(&FONT_S, t->msg, t->kind, now, t->end_ms, t->by, t->bar, t->slide, t->dith, KO_TOAST_Y0);
}

#endif /* KO_CHROME_H */
