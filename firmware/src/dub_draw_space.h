/* SPDX-License-Identifier: GPL-3.0-only */
/* Genre SPACE / FX bus diagram (echo taps, tank, iso). Included from fm_app or DECK. */
#ifndef DUB_DRAW_SPACE_H
#define DUB_DRAW_SPACE_H

#include "dub_types.h"
#include "fm_lfo.h"

#ifndef DUB_DLY_DIV_N
#define DUB_DLY_DIV_N 17u
#endif
#ifndef DUB_DRAW_COLOR_SCALE
#define DUB_DRAW_COLOR_SCALE color_scale_rgb
#endif
#ifndef DUB_DRAW_SINE
#define DUB_DRAW_SINE dub_sine_q64
#endif
#ifndef DUB_DRAW_COPY_STR
#define DUB_DRAW_COPY_STR copy_str
#endif

static inline uint8_t dub_draw_dly_div_ix(uint8_t dly_time)
{
    if (dly_time == 0u)
        return 0u;
    return (uint8_t)(1u + (((uint32_t)(dly_time - 1u) * (DUB_DLY_DIV_N - 2u) + 63u) / 126u));
}

typedef struct {
    uint8_t pg; /* 3 = drive page, 4 = mix page */
    uint8_t drive;
    uint8_t crush;
    uint8_t filter; /* 64 = flat */
    uint8_t width;
    uint8_t comp;
    uint8_t dust;
    uint8_t tape;
} dub_draw_dirt_t;

static inline void dub_draw_view_space(const dub_params_t *p, int32_t macro_iso, uint32_t now_ms, int bypass,
                                       const char *title)
{
    static const char *const TAP_NAMES[TAP_N] = {"SINGLE", "DUAL", "GALLOP", "TRIP", "SPREAD", "PING"};
    char s[24];
    uint16_t acc = bypass ? C_DIM : C_ACC;
    const char *hdr = title ? title : "SPACE";

    if (!p)
        return;
    DUB_DRAW_COPY_STR(s, p->tank < 64u ? "SPR" : "TNK");
    cv_text(68, 4, &FONT_S, s, C_HI);
    DUB_DRAW_COPY_STR(s, TAP_NAMES[p->tap % TAP_N]);
    cv_text(108, 4, &FONT_S, s, acc);
    fm_time_fmt(s, p->dly_time, p->bpm ? p->bpm : 120u, now_ms);
    cv_text(176, 4, &FONT_S, s, C_WHITE);
    cv_text(4, 4, &FONT_S, bypass ? "FX BYP" : hdr, bypass ? C_DIM : C_WHITE);
    {
        uint8_t dix = dub_draw_dly_div_ix(p->dly_time);
        uint8_t hm = p->tap % TAP_N;
        uint32_t ntaps = 12u - ((uint32_t)dix * 7u) / (DUB_DLY_DIV_N - 2u);
        uint32_t t;
        int32_t span_w = 216;
        int32_t fb = (int32_t)p->feedback;
        int32_t mix_op = ((int32_t)p->dly_lvl * 256) / 127; /* 0..256 opacity for MIX */
        int32_t rev = ((int32_t)p->rev_mix * mix_op) >> 8;
        int32_t damp = (int32_t)p->rev_damp;
        int32_t tail = (int32_t)p->rev_tail;
        int32_t bleed = (int32_t)p->dly_rev;
        int32_t lf = (int32_t)p->loop_filt;
        int32_t wow = (int32_t)p->wow;
        int32_t iso = ((int32_t)p->isolator - 64) + ((macro_iso) >> 1);
        int32_t iso_top = iso < 0 ? (28 + ((-iso) * 44) / 64) : 24;
        int32_t iso_bot = iso > 0 ? (118 - (iso * 44) / 63) : 118;
        int32_t h = (84 * (24 + ((int32_t)p->dly_lvl * 103) / 127)) / 127;
        int32_t base_y = 118;
        int32_t prev_tx = -1, prev_ty = -1;

        if (ntaps < 4u)
            ntaps = 4u;

        {
            int32_t tank_bot = iso_bot < base_y ? iso_bot : base_y;
            int32_t field_top = iso_top + 2;

            if (field_top < 24)
                field_top = 24;

            /* TANK architecture + REVERB/TAIL/DAMP hall field (behind echo taps) */
            if (p->tank > 0u || rev > 0) {
                int32_t rh_base = 14 + ((int32_t)p->tank * 28) / 127 + (rev * 22) / 127;
                uint32_t rx;
                int32_t coil_top = tank_bot - rh_base;
                int32_t coil_bot = tank_bot - 2;

                if (coil_top < field_top)
                    coil_top = field_top;
                if (coil_top < 64)
                    coil_top = 64;

                if (p->tank < 64u) {
                    /* SPR: spring coil between iso_top and tank_bot */
                    uint32_t step_s = 5u + (63u - p->tank) / 10u;
                    uint32_t scroll = (now_ms / (10u + p->tank / 8u)) & 15u;
                    int32_t coil_cy = tank_bot - 10 - (rev * 16) / 127;
                    int32_t coil_amp = 4 + ((int32_t)p->tank * 14) / 63;
                    int32_t spy = coil_cy;
                    int32_t spr_bright = 90 + ((int32_t)p->tank * 70) / 63;
                    uint16_t spr_col = DUB_DRAW_COLOR_SCALE(C_HI, spr_bright);
                    uint16_t spr_hi = DUB_DRAW_COLOR_SCALE(C_HI, spr_bright + 18);

                    if (coil_amp > rh_base / 2)
                        coil_amp = rh_base / 2;
                    if (coil_amp < 2)
                        coil_amp = 2;
                    if (coil_cy < coil_top + coil_amp)
                        coil_cy = coil_top + coil_amp;
                    if (coil_cy > coil_bot - coil_amp)
                        coil_cy = coil_bot - coil_amp;
                    spy = coil_cy;

                    for (rx = 8u; rx < 230u; rx += step_s) {
                        int32_t x_norm = ((int32_t)rx - 8) * 256 / 222;
                        int32_t tail_keep = 256 - ((x_norm * (140 - tail)) >> 7);
                        int32_t swing_amp, ph_s;
                        uint32_t coil_ix;
                        uint16_t seg_col;

                        if (tail_keep < 24)
                            tail_keep = 24;
                        swing_amp = (coil_amp * tail_keep * (160 - damp)) >> 13;
                        coil_ix = ((rx + scroll) / step_s) & 7u;
                        ph_s = (coil_ix & 1u) ? (coil_cy - (swing_amp >> 1))
                                              : (coil_cy + (swing_amp >> 1));
                        if (ph_s < coil_top)
                            ph_s = coil_top;
                        if (ph_s > coil_bot)
                            ph_s = coil_bot;
                        seg_col = (coil_ix & 3u) == 0u ? spr_hi : spr_col;
                        cv_line((int32_t)rx, spy, (int32_t)(rx + step_s), ph_s, seg_col);
                        spy = ph_s;
                    }
                } else if (p->tank > 0u) {
                    /* TNK: chamber pillars + ceiling */
                    int32_t tn = (int32_t)p->tank - 64;
                    uint32_t col_step = 6u + (uint32_t)(63 - tn) / 12u;
                    int32_t col_w = 2 + tn / 10;
                    int32_t ceil_y = tank_bot - rh_base - 4;
                    int32_t tnk_edge = 75 + (tn * 85) / 63;
                    uint16_t tnk_hi = DUB_DRAW_COLOR_SCALE(C_HI, tnk_edge);
                    uint16_t tnk_fill = DUB_DRAW_COLOR_SCALE(C_DIM, 150 + (tn * 40) / 63);

                    if (ceil_y < 28)
                        ceil_y = 28;
                    cv_line(8, ceil_y, 231, ceil_y, tnk_hi);

                    for (rx = 8u; rx + (uint32_t)col_w < 231u; rx += col_step) {
                        int32_t x_norm = ((int32_t)rx - 8) * 256 / 222;
                        int32_t tail_keep = 256 - ((x_norm * (140 - tail)) >> 7);
                        int32_t rh_x, ch, cap_h;
                        int32_t damp_sh = (160 - damp) >> 4;

                        if (tail_keep < 24)
                            tail_keep = 24;
                        rh_x = (rh_base * tail_keep * damp_sh) >> 12;
                        ch = rh_x + (tn * rh_base) / 126;
                        if (ch > tank_bot - ceil_y - 2)
                            ch = tank_bot - ceil_y - 2;
                        if (ch < 3)
                            ch = 3;
                        cv_rect((int32_t)rx, tank_bot - ch, col_w, ch, tnk_fill);
                        cv_rect((int32_t)rx, tank_bot - ch, col_w, 1, tnk_hi);
                        cap_h = 2;
                        if (ch > cap_h + 1)
                            cv_rect((int32_t)rx, tank_bot - ch, col_w, cap_h, tnk_hi);
                    }
                }

                /* REVERB decay roof + diffusion rays (TAIL length, DAMP colour/texture) */
                if (rev > 0) {
                    int32_t rev_span = tank_bot - field_top - 8;
                    int32_t rev_h0 = (rev * rev_span) / 145;
                    int32_t prev_roof_x = -1, prev_roof_y = -1;
                    int32_t ray_stride = 4 + (damp >> 4);

                    if (rev_span < 8)
                        rev_span = 8;
                    if (rev_h0 < 4)
                        rev_h0 = 4;

                    for (rx = 8u; rx <= 231u; rx += 2u) {
                        int32_t x_norm = ((int32_t)rx - 8) * 256 / 223;
                        int32_t tail_keep = 256 - ((x_norm * (140 - tail)) >> 7);
                        int32_t rev_h, roof_y, shimmer, ry;
                        uint16_t roof_col, ray_col;

                        if (tail_keep < 8)
                            tail_keep = 8;
                        rev_h = (rev_h0 * tail_keep) >> 8;
                        rev_h = (rev_h * (200 - damp)) >> 8;
                        if (rev_h < 2)
                            rev_h = 2;
                        roof_y = tank_bot - rev_h;
                        if (damp < 80)
                            shimmer = ((DUB_DRAW_SINE(((now_ms << 17) + ((int32_t)rx << 19))) >> 13)
                                       * (80 - damp)) / 96;
                        else
                            shimmer = 0;
                        roof_y -= shimmer;
                        if (roof_y < field_top)
                            roof_y = field_top;

                        roof_col = DUB_DRAW_COLOR_SCALE(damp < 48 ? C_WHITE : damp < 96 ? C_HI : C_GRAY,
                                                   damp < 48 ? (150 + (shimmer >> 1))
                                                             : damp < 96 ? (110 + rev / 3)
                                                                         : (72 + (127 - damp) / 2));
                        if (prev_roof_x >= 0)
                            cv_line(prev_roof_x, prev_roof_y, (int32_t)rx, roof_y, roof_col);
                        prev_roof_x = (int32_t)rx;
                        prev_roof_y = roof_y;

                        ray_col = DUB_DRAW_COLOR_SCALE(damp < 64 ? C_HI : C_GRAY,
                                                  56 + (rev * 90) / 127 - (damp >> 1));
                        for (ry = roof_y + ray_stride; ry < tank_bot - 1; ry += ray_stride) {
                            if ((((int32_t)rx + ry + (int32_t)(now_ms >> 5)) & (7 + (damp >> 3))) == 0)
                                cv_rect((int32_t)rx, ry, 1, 1 + (damp < 72 ? 1 : 0), ray_col);
                        }
                    }
                }
            }
        }

        /* Delay taps spanning full width + MIX opacity + FEEDBK decay + LOOP passband + BLEED halo */
        for (t = 0; dix > 0u && p->dly_lvl > 0u && t < ntaps; t++) {
            int32_t w_sin = DUB_DRAW_SINE(((now_ms * 4u) << 19) + (t * 0x28000000u));
            int32_t wow_dx = ((w_sin * wow) >> 15) * 5 / 127;
            int32_t wow_dh = ((DUB_DRAW_SINE(((now_ms * 3u) << 19) + (t * 0x38000000u)) * wow) >> 15) * 6 / 127;
            int32_t x = 8 + ((int32_t)t * span_w) / (int32_t)(ntaps - 1u) + wow_dx;
            int32_t bar_w = ntaps > 8u ? 6 : 8;
            int32_t cur_h = h + wow_dh < 4 ? 4 : (h + wow_dh > 92 ? 92 : h + wow_dh);
            int32_t top_trim = ((int32_t)t * ((127 - lf) * cur_h)) / (int32_t)(ntaps * 170u);
            int32_t bot_trim = ((int32_t)t * (lf * cur_h)) / (int32_t)(ntaps * 200u);
            int32_t y0 = (base_y - cur_h) + top_trim < iso_top ? iso_top : (base_y - cur_h) + top_trim;
            int32_t y1 = base_y - bot_trim > iso_bot ? iso_bot : base_y - bot_trim;
            uint16_t raw_col = t == 0u ? C_WHITE :
                               ((hm == 5u && (t & 1u)) ? C_ACC :
                                (t * (uint32_t)(127 - lf) > ntaps * 70u ? C_DIM : C_HI));
            uint16_t col = DUB_DRAW_COLOR_SCALE(raw_col, 36 + ((mix_op * 220) >> 8));
            uint16_t bg_col = DUB_DRAW_COLOR_SCALE(C_LINE, 48 + ((mix_op * 208) >> 8));

            if (x < 8)
                x = 8;
            if (x + bar_w > 231)
                x = 231 - bar_w;
            if (y1 - y0 < 3)
                y0 = y1 - 3;

            /* BLEED (`dly_rev`): reverb flag trailing right of each echo tap */
            if (bleed > 6) {
                int32_t bw = 2 + (bleed * (8 + (tail >> 4))) / 127;
                int32_t bh = y1 - y0;
                int32_t by0 = y1 - bh + ((damp * bh) >> 9);
                int32_t bbright = 96 + (bleed * 150) / 127;
                int32_t bi;

                if (bh < 4)
                    bh = 4;
                if (by0 < y0)
                    by0 = y0;
                if (y1 > by0 + 1 && x + bar_w + bw <= 232) {
                    for (bi = 0; bi < bw; bi++) {
                        int32_t fade = 256 - (bi * 220 / (bw > 1 ? bw : 1));
                        uint16_t bcol = DUB_DRAW_COLOR_SCALE(damp < 80 ? C_HI : C_DIM,
                                                        (bbright * fade) >> 8);

                        cv_rect(x + bar_w + bi, by0, 1, y1 - by0, bcol);
                    }
                }
            }

            cv_rect(x, base_y - cur_h, bar_w, cur_h, bg_col);
            cv_rect(x, y0, bar_w, y1 - y0, col);
            if (wow > 4) {
                int32_t tx = x + (bar_w >> 1);
                if (prev_tx >= 0)
                    cv_line(prev_tx, prev_ty, tx, y0,
                            DUB_DRAW_COLOR_SCALE(wow > 64 ? C_ACC : C_DIM, 48 + ((mix_op * 208) >> 8)));
                prev_tx = tx;
                prev_ty = y0;
            }
            /* Draw 2nd tape head (Tap B) between main taps for DUAL, GALLOP, TRIP, SPREAD, PING */
            if (hm != 0u && t + 1u < ntaps) {
                int32_t step_dx = span_w / (int32_t)(ntaps - 1u);
                int32_t bx = (hm == 1u || hm == 5u) ? (x + step_dx / 2) :
                             hm == 2u ? (x + (step_dx * 3) / 4) :
                             hm == 3u ? (x + (step_dx * 2) / 3) : (x + bar_w + 1);
                int32_t bh = (y1 - y0) * 3 / 4;
                uint16_t h2_col = DUB_DRAW_COLOR_SCALE(hm == 4u || hm == 5u ? C_ACC : C_DIM,
                                                  48 + ((mix_op * 208) >> 8));
                if (bh > 3 && bx + 3 <= 231)
                    cv_rect(bx, y1 - bh, 3, bh, h2_col);
            }
            h = (h * (28 + fb)) / 140;
            if (h > 92)
                h = 92;
            if (h < 4)
                h = 4;
        }

        if (iso < -2)
            cv_line(8, iso_top, 231, iso_top, C_ACC);
        else if (iso > 2)
            cv_line(8, iso_bot, 231, iso_bot, C_ACC);
    }
}

/* Grit on the wet FX return (DECK dirt pages): drawn on top of dub_draw_view_space. */
static inline void dub_draw_space_dirt_overlay(const dub_draw_dirt_t *d, const dub_params_t *p, uint32_t now_ms,
                                               int bypass)
{
    uint16_t acc = bypass ? C_DIM : C_ACC;
    int32_t drv, crh, comp, dust, tape, wid;
    int32_t filt_c;
    uint8_t dix;
    uint32_t ntaps, t;
    int32_t span_w = 216;
    int32_t fb;
    int32_t lf;
    int32_t iso;
    int32_t iso_top, iso_bot;
    int32_t h;
    int32_t base_y = 118;
    int32_t path_y = 54;
    int32_t prev_x = -1, prev_py = -1;
    int32_t em_drv, em_crh, em_flt, em_wid, em_cmp, em_dust, em_tape;

    if (!d || !p)
        return;

    drv = (int32_t)d->drive;
    crh = (int32_t)d->crush;
    comp = (int32_t)d->comp;
    dust = (int32_t)d->dust;
    tape = (int32_t)d->tape;
    wid = (int32_t)d->width;
    filt_c = (int32_t)(d->filter ? d->filter : 64u) - 64;

    em_drv = d->pg == 3u ? drv : (drv * 5) / 8;
    em_crh = d->pg == 3u ? crh : (crh * 5) / 8;
    em_flt = d->pg == 3u ? (filt_c < 0 ? -filt_c : filt_c) : ((filt_c < 0 ? -filt_c : filt_c) * 5) / 8;
    em_wid = d->pg == 3u ? wid : (wid * 5) / 8;
    em_cmp = d->pg == 4u ? comp : (comp * 5) / 8;
    em_dust = d->pg == 4u ? dust : (dust * 5) / 8;
    em_tape = d->pg == 4u ? tape : (tape * 5) / 8;

    iso = ((int32_t)p->isolator - 64);
    iso_top = iso < 0 ? (28 + ((-iso) * 44) / 64) : 24;
    iso_bot = iso > 0 ? (118 - (iso * 44) / 63) : 118;

    if (em_flt > 8) {
        if (filt_c < 0) {
            int32_t y1 = iso_top + 2 + ((63 + filt_c) * 28) / 63;

            if (y1 < 22)
                y1 = 22;
            cv_rect(8, 22, 224, (uint32_t)(y1 - 22 + 1),
                    DUB_DRAW_COLOR_SCALE(C_LINE, 32 + em_flt / 2));
        } else if (filt_c > 0) {
            int32_t y0 = iso_bot - 2 - (filt_c * 28) / 63;

            if (y0 > 116)
                y0 = 116;
            cv_rect(8, y0, 224, (uint32_t)(118 - y0),
                    DUB_DRAW_COLOR_SCALE(C_LINE, 32 + em_flt / 2));
        }
    }

    dix = dub_draw_dly_div_ix(p->dly_time);
    ntaps = 12u - ((uint32_t)dix * 7u) / (DUB_DLY_DIV_N - 2u);
    if (ntaps < 4u)
        ntaps = 4u;
    fb = (int32_t)p->feedback;
    lf = (int32_t)p->loop_filt;
    h = (84 * (24 + ((int32_t)p->dly_lvl * 103) / 127)) / 127;

    cv_text(196, 4, &FONT_S, "WET", DUB_DRAW_COLOR_SCALE(acc, 96));

    for (t = 0; dix > 0u && p->dly_lvl > 0u && t < ntaps; t++) {
        int32_t x = 8 + ((int32_t)t * span_w) / (int32_t)(ntaps - 1u);
        int32_t cur_h = h < 4 ? 4 : (h > 92 ? 92 : h);
        int32_t top_trim = ((int32_t)t * ((127 - lf) * cur_h)) / (int32_t)(ntaps * 170u);
        int32_t y1 = base_y - ((int32_t)t * (lf * cur_h)) / (int32_t)(ntaps * 200u);
        int32_t y0 = (base_y - cur_h) + top_trim;
        int32_t tap_mid = (y0 + y1) >> 1;
        int32_t side_dx = (wid * 14) / 127;
        int32_t py = path_y + ((tap_mid - 70) * 12) / 50;
        int32_t q = em_crh > 4 ? (2 + (em_crh >> 4)) : 0;
        int32_t ceil_y = path_y - (em_cmp * 22) / 127;

        if (py < iso_top + 4)
            py = iso_top + 4;
        if (py > iso_bot - 4)
            py = iso_bot - 4;
        if (q > 0)
            py = (py >> q) << q;
        if (py < ceil_y)
            py = ceil_y;

        if (em_wid > 6 && side_dx > 0) {
            uint16_t scol = DUB_DRAW_COLOR_SCALE(C_DIM, 36 + (em_wid * 90) / 127);

            cv_line(x - side_dx, py, x + side_dx, py, scol);
        }

        if (prev_x >= 0) {
            uint16_t lcol = DUB_DRAW_COLOR_SCALE(tape > 48 ? C_ACC : C_HI, 40 + (em_tape * 140) / 127);

            cv_line(prev_x, prev_py, x, py, lcol);
            if (em_tape > 32)
                cv_line(prev_x, prev_py + 1, x, py + 1, DUB_DRAW_COLOR_SCALE(C_DIM, em_tape));
        }

        if (em_drv > 24 && (t & 1u) == 0u)
            cv_line(x - 2, py - 2, x + 2, py - 2, DUB_DRAW_COLOR_SCALE(C_WHITE, 80 + em_drv / 2));

        if (em_crh > 12) {
            int32_t st = 3 + (em_crh >> 3);

            cv_rect(x - 1, py - st, 3, (uint32_t)(st + st + 1),
                    DUB_DRAW_COLOR_SCALE(acc, 48 + em_crh / 2));
        }

        prev_x = x;
        prev_py = py;
        h = (h * (28 + fb)) / 140;
        if (h > 92)
            h = 92;
        if (h < 4)
            h = 4;
    }

    if (em_dust > 4) {
        uint32_t speck = 24u + (uint32_t)em_dust / 3u;
        uint32_t i;
        uint32_t seed = now_ms * 1664525u + 57005u;

        for (i = 0u; i < speck; i++) {
            seed = seed * 1664525u + 1013904223u;
            {
                int32_t sx = 8 + (int32_t)((seed >> 16) % 224u);
                int32_t sy = 26 + (int32_t)((seed >> 8) % 88u);

                if (((seed >> 24) & 3u) == 0u)
                    cv_pset(sx, sy, DUB_DRAW_COLOR_SCALE(C_HI, 40 + (int32_t)(seed & 63u)));
            }
        }
    }

    if (em_drv > 64)
        cv_line(8, path_y + 8, 231, path_y + 8, DUB_DRAW_COLOR_SCALE(C_ACC, em_drv / 2));
}

#endif /* DUB_DRAW_SPACE_H */
