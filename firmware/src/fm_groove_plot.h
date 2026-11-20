/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared GROOVE feel plot — timing pulse + swing grid + scale ring (DECK GLO + KO BASS BANK_GROOVE). */
#ifndef FM_GROOVE_PLOT_H
#define FM_GROOVE_PLOT_H

#include <stdint.h>

#ifndef FM_GROOVE_PLOT_SCALE
#define FM_GROOVE_PLOT_SCALE color_scale_rgb
#endif
#ifndef FM_GROOVE_PLOT_SINE
#define FM_GROOVE_PLOT_SINE dub_sine_q64
#endif

/* Semitone sets aligned with RYTM / DECK scale order (0..8). */
static const uint8_t FM_GROOVE_SCALE_SEMIS[9][8] = {
    {0, 2, 3, 5, 7, 8, 10, 0},
    {0, 2, 3, 5, 7, 9, 10, 0},
    {0, 1, 3, 5, 7, 8, 10, 0},
    {0, 2, 4, 5, 7, 9, 11, 0},
    {0, 2, 4, 5, 7, 9, 10, 0},
    {0, 3, 5, 7, 10, 0, 0, 0},
    {0, 2, 3, 5, 7, 9, 10, 0},
    {0, 3, 5, 6, 7, 10, 0, 0},
    {0, 2, 3, 5, 7, 8, 11, 0},
};

typedef struct {
    uint8_t page; /* 0 = timing/harmony; 1 = accent/human/clock */
    uint8_t bpm;
    uint8_t swing;
    uint8_t accent;
    uint8_t human;
    uint8_t root;
    uint8_t scale;
    uint8_t no_ring; /* 1 = kein Skalenring auf page 0 (KIT) */
    uint8_t playing;
    uint8_t step; /* 0..15 */
    uint32_t step_frac256;
    uint8_t clock_mode;
    uint8_t clock_n; /* 2 (DECK) or 3 (BASS) */
    uint8_t midi_ch; /* 0-based */
    uint16_t col;
} fm_groove_plot_t;

static inline uint8_t fm_groove_plot_scale_has(uint8_t scale, uint8_t semi)
{
    const uint8_t *tab = FM_GROOVE_SCALE_SEMIS[scale % 9u];
    uint8_t k;

    for (k = 0u; k < 8u && tab[k] != 0u; k++)
        if (tab[k] == semi)
            return 1u;
    return 0u;
}

static inline void fm_groove_plot_step_x(int32_t x0, int32_t plot_w, uint8_t swing, int32_t xs[17])
{
    uint32_t lens[16];
    uint32_t tot = 0u;
    uint32_t amt;
    uint8_t i;
    int32_t acc = 0;

    if (swing < 50u)
        swing = 50u;
    if (swing > 75u)
        swing = 75u;
    amt = (uint32_t)swing - 50u;
    for (i = 0u; i < 16u; i++) {
        if (i & 1u)
            lens[i] = 100u + amt * 2u;
        else
            lens[i] = 100u - amt * 2u;
        tot += lens[i];
    }
    if (tot < 1u)
        tot = 1u;
    xs[0] = x0;
    for (i = 0u; i < 16u; i++) {
        acc += (int32_t)((plot_w * (int32_t)lens[i]) / (int32_t)tot);
        xs[i + 1u] = x0 + acc;
    }
    xs[16] = x0 + plot_w;
}

static inline int32_t fm_groove_plot_play_x(const int32_t *xs, uint8_t step, uint32_t frac256)
{
    uint8_t st = (uint8_t)(step % 16u);

    if (frac256 > 255u)
        frac256 = 255u;
    return xs[st] + ((xs[st + 1u] - xs[st]) * (int32_t)frac256) / 256;
}

static inline int32_t fm_groove_plot_human_dx(uint8_t step, uint8_t human)
{
    uint32_t h = human;

    if (h < 8u)
        return 0;
    return (int32_t)(((step * 13u + h * 5u) % 17u) - 8u) * (int32_t)h / 32;
}

static inline void fm_groove_plot_ring(int32_t cx, int32_t cy, int32_t r, uint8_t root, uint8_t scale, uint16_t col)
{
    uint8_t pc;

    for (pc = 0u; pc < 12u; pc++) {
        uint32_t ang = (uint32_t)pc * 65536u / 12u;
        int32_t s = FM_GROOVE_PLOT_SINE(ang);
        int32_t c = FM_GROOVE_PLOT_SINE(ang + 16384u);
        int32_t x = cx + (c * r) / 32767;
        int32_t y = cy + (s * r) / 32767;
        uint8_t in = fm_groove_plot_scale_has(scale, pc);
        uint16_t dot = (pc == (root % 12u)) ? col : (in ? FM_GROOVE_PLOT_SCALE(col, 120) : C_LINE);

        if (in || pc == (root % 12u))
            cv_rect(x - 1, y - 1, 3u, 3u, dot);
        else
            cv_line(x, y, x, y, C_LINE);
    }
}

static inline void fm_groove_plot_draw(const fm_groove_plot_t *in)
{
    const int32_t x0 = 8;
    const int32_t x1 = 232;
    const int32_t plot_w = x1 - x0;
    const int32_t y_bot = 106;
    const int32_t y_top = 28;
    const int32_t y_mid = 68;
    int32_t xs[17];
    uint8_t i;
    uint16_t col = in->col ? in->col : C_ACC;
    uint16_t fill = FM_GROOVE_PLOT_SCALE(col, 40);
    uint16_t pulse = FM_GROOVE_PLOT_SCALE(col, 200);
    uint16_t odd_col = FM_GROOVE_PLOT_SCALE(col, 130);
    int32_t ph_x;
    uint32_t idle_ph = 0u;

    if (!in)
        return;

    fm_groove_plot_step_x(x0, plot_w, in->swing, xs);

    cv_line(x0, y_bot, x1, y_bot, C_LINE);
    cv_line(x0, y_top - 6, x0, y_bot, C_LINE);
    cv_line(x1, y_top - 6, x1, y_bot, C_LINE);

    if (in->page == 0u && !in->no_ring)
        fm_groove_plot_ring(36, y_mid - 6, 14, in->root, in->scale, col);

    for (i = 0u; i < 16u; i++) {
        int32_t x = xs[i] + (xs[i + 1u] - xs[i]) / 2 + fm_groove_plot_human_dx(i, in->human);
        int32_t h = 6 + (int32_t)in->accent / 18;
        int32_t y0p = y_bot - h;
        uint8_t down = (uint8_t)((i & 3u) == 0u);
        uint16_t pcol = (i & 1u) ? odd_col : pulse;

        if (down)
            h += 4 + (int32_t)in->accent / 24;
        if (in->page == 1u && down)
            pcol = FM_GROOVE_PLOT_SCALE(C_WHITE, 160 + in->accent / 2);
        if (x < x0 + 2)
            x = x0 + 2;
        if (x > x1 - 2)
            x = x1 - 2;
        cv_line(x, y_bot, x, y0p, FM_GROOVE_PLOT_SCALE(pcol, down ? 220 : 150));
        cv_rect(x - 1, y0p, 3u, 2u, pcol);
    }

    if (in->playing) {
        ph_x = fm_groove_plot_play_x(xs, in->step, in->step_frac256);
    } else {
        idle_ph = (fm1_ms * (uint32_t)(in->bpm < 40u ? 40u : in->bpm)) >> 9;
        ph_x = fm_groove_plot_play_x(xs, (uint8_t)((idle_ph / 256u) % 16u), idle_ph & 255u);
    }

    if (ph_x >= x0 && ph_x <= x1) {
        int32_t beam = y_top + (int32_t)(127 - (in->bpm < 40u ? 40u : in->bpm > 200u ? 200u : in->bpm)) / 6;

        cv_line(ph_x, beam, ph_x, y_bot, FM_GROOVE_PLOT_SCALE(C_WHITE, in->playing ? 200 : 80));
        cv_rect(ph_x - 2, beam - 2, 5u, 5u, in->playing ? C_WHITE : FM_GROOVE_PLOT_SCALE(col, 100));
    }

    if (in->page == 0u) {
        int32_t lean = (int32_t)in->swing - 62;

        if (lean < -12)
            lean = -12;
        if (lean > 12)
            lean = 12;
        cv_line(x0 + 52, y_top, x1 - 8 + lean, y_top, FM_GROOVE_PLOT_SCALE(col, 90));
        cv_line(x0 + 52, y_top + 1, x1 - 8 - lean, y_top + 1, fill);
    } else {
        uint8_t cm = in->clock_mode;
        const char *clk = "INT";

        if (in->clock_n >= 3u && cm == 2u)
            clk = "TX";
        else if (cm != 0u)
            clk = "EXT";
        if (in->clock_n <= 2u && cm > 1u)
            cm = 1u;
        cv_text(188, 4, &FONT_S, clk, cm ? FM_GROOVE_PLOT_SCALE(C_ACC, 180) : C_DIM);
        if (in->midi_ch < 16u) {
            char mc[4];

            mc[0] = (char)('0' + ((in->midi_ch + 1u) / 10u) % 10u);
            mc[1] = (char)('0' + (in->midi_ch + 1u) % 10u);
            mc[2] = 0;
            if (mc[0] == '0') {
                mc[0] = mc[1];
                mc[1] = 0;
            }
            cv_text(210, 4, &FONT_S, mc, C_DIM);
        }
        if (in->human > 12u) {
            int32_t wob = fm_groove_plot_human_dx((uint8_t)(in->step + 3u), in->human);

            cv_line(x0, y_top - 2, x1 + wob, y_top - 2, FM_GROOVE_PLOT_SCALE(col, 60 + in->human / 2));
        }
    }

}

#endif /* FM_GROOVE_PLOT_H */
