/* SPDX-License-Identifier: GPL-3.0-only */
/* KO KIT ENV standard (DECK ENV bank, KO BASS ENV bank): page layout, CURVE readout, DAHDSR plot.
 * Include after the canvas (cv_*, text_w, FONT_S, C_*), ko_chrome.h and fm_lfo.h. */
#ifndef FM_ENV_KIT_H
#define FM_ENV_KIT_H

#include "fm_env_plot.h"

#ifndef FM_ENV_KIT_SCALE
#define FM_ENV_KIT_SCALE color_scale_rgb
#endif

/* Page 2 cols 1..3 are app-specific (KIT: pitch tail; BASS: env mod routing). */
static const char *const FM_ENV_KIT_LBL[FM_ENV_KIT_PAGES][4] = {
    {"ATTACK", "DECAY", "SUST", "REL"},
    {"DELAY", "HOLD", "CURVE", "RETRIG"},
    {"INVERT", "P.DEC", "SWEEP", ""},
};

#define FM_ENV_KIT_X0    8
#define FM_ENV_KIT_X1    232
#define FM_ENV_KIT_Y_TOP 22
#define FM_ENV_KIT_Y_BOT 108

typedef struct {
    uint32_t w[5];   /* plot weights: delay, attack, hold, decay, release */
    uint8_t sustain; /* 0..127 */
    uint8_t curve;   /* 0 = −EXP, 64 = LIN, 127 = EXP */
    uint8_t invert;
    uint16_t col;
} fm_env_kit_plot_t;

typedef struct {
    int32_t px[7];
    int32_t y_peak;
    int32_t y_sus;
} fm_env_kit_geom_t;

static inline int32_t fm_env_kit_y(const fm_env_kit_plot_t *e, const fm_env_kit_geom_t *gm, int32_t x)
{
    return fm_env_plot_map_y(fm_env_curve_y(x, gm->px, FM_ENV_KIT_Y_BOT, gm->y_peak, gm->y_sus, e->curve),
                             FM_ENV_KIT_Y_TOP, FM_ENV_KIT_Y_BOT, e->invert);
}

/* Frame, filled DAHDSR stroke and D/A/H/D/S/R stage letters. */
static inline void fm_env_kit_draw(const fm_env_kit_plot_t *e, fm_env_kit_geom_t *gm)
{
    static const char *const STAGE[6] = {"D", "A", "H", "D", "S", "R"};
    const int32_t x0 = FM_ENV_KIT_X0;
    const int32_t x1 = FM_ENV_KIT_X1;
    const int32_t y_top = FM_ENV_KIT_Y_TOP;
    const int32_t y_bot = FM_ENV_KIT_Y_BOT;
    uint16_t fill_col = FM_ENV_KIT_SCALE(e->col, 44);
    uint16_t curve_col = FM_ENV_KIT_SCALE(e->col, 220);
    int32_t x;
    uint8_t seg;

    gm->y_peak = y_top + 4;
    gm->y_sus = y_bot - ((int32_t)e->sustain * (y_bot - y_top)) / 127;
    fm_env_plot_bounds(e->w, x0, x1 - x0, gm->px);
    cv_line(x0, y_bot, x1, y_bot, C_LINE);
    cv_line(x0, y_top - 4, x0, y_bot, C_LINE);
    cv_line(x1, y_top - 4, x1, y_bot, C_LINE);

    for (x = x0; x <= x1; x++) {
        int32_t y = fm_env_kit_y(e, gm, x);

        if (e->invert) {
            if (y > y_top && ((x - x0) & 1) == 0)
                cv_line(x, y_top, x, y - 1, fill_col);
        } else if (y < y_bot && ((x - x0) & 1) == 0)
            cv_line(x, y + 1, x, y_bot, fill_col);
    }
    for (seg = 1u; seg <= 6u; seg++) {
        int32_t xa = gm->px[seg - 1u];
        const int32_t xb = gm->px[seg];

        if (xb <= xa)
            continue;
        for (;;) {
            int32_t xn = xa + 3;

            if (xn > xb)
                xn = xb;
            cv_line(xa, fm_env_kit_y(e, gm, xa), xn, fm_env_kit_y(e, gm, xn), curve_col);
            if (xn >= xb)
                break;
            xa = xn;
        }
    }
    for (seg = 0u; seg < 6u; seg++) {
        int32_t mx = (gm->px[seg] + gm->px[seg + 1u]) / 2;

        if (gm->px[seg + 1u] - gm->px[seg] >= 10 && mx >= x0 + 2 && mx <= x1 - 2)
            cv_text(mx - 2, y_bot + 2, &FONT_S, STAGE[seg], C_DIM);
    }
}

/* Playhead at ph_x (< X0 = none), dot on the stroke. */
static inline void fm_env_kit_draw_playhead(const fm_env_kit_plot_t *e, const fm_env_kit_geom_t *gm, int32_t ph_x)
{
    int32_t live_y;

    if (ph_x < FM_ENV_KIT_X0)
        return;
    live_y = fm_env_kit_y(e, gm, ph_x);
    cv_line(ph_x, FM_ENV_KIT_Y_TOP - 4, ph_x, FM_ENV_KIT_Y_BOT, FM_ENV_KIT_SCALE(C_ACC, 120));
    cv_rect(ph_x - 1, live_y - 1, 3, 3, C_WHITE);
}

/* Top-right gate box while sounding (ACC = RETRIG on); “1SH” when RETRIG is off and idle. */
static inline void fm_env_kit_draw_gate(uint8_t active, uint8_t retrig)
{
    if (active)
        cv_rect(200, 6, 34, 10, retrig ? C_ACC : C_DIM);
    else if (!retrig)
        cv_text(200, 6, &FONT_S, "1SH", C_DIM);
}

/* CURVE knob value: anchor name (-EXP / LOG / LIN / -LOG / EXP at 0 / 32 / 64 / 96 / 127),
 * otherwise a dual morph label of the neighbouring anchors. Needs k->pv = curve only. */
static int32_t fm_env_kit_knob_val_shape(const ko_knob_slot_t *k, int32_t x)
{
    static const char *const ANCH[5] = {"-EXP", "LOG", "LIN", "-LOG", "EXP"};
    uint8_t c = k->pv;
    uint8_t seg;
    uint32_t t;

    if (c > 127u)
        c = 127u;
    if (c == 0u || c == 32u || c == 64u || c == 96u || c == 127u) {
        const char *s = ANCH[c == 127u ? 4u : c / 32u];
        const int32_t bar_l = x + 6;
        const int32_t bar_w = 48;
        int32_t tw = text_w(&FONT_S, s);
        int32_t tx = bar_l + (bar_w - tw) / 2;

        return cv_text(tx, 18, &FONT_S, s, k->vcol);
    }
    seg = (uint8_t)(c / 32u);
    t = ((uint32_t)(c & 31u) * 128u) / 31u;
    return fm_dual_morph_label_draw(x, 18, ANCH[seg], ANCH[seg + 1u], (t * 255u) / 128u, k->vcol, C_DIM);
}

#endif /* FM_ENV_KIT_H */
