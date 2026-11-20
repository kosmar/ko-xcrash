/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS knob-aligned 16-bar progress rail (draw_bar_progress_rail). */
#ifndef DUB_KNOB_PROGRESS_RAIL_H
#define DUB_KNOB_PROGRESS_RAIL_H

#ifndef DUB_KNOB_RAIL_COLOR_SCALE
#define DUB_KNOB_RAIL_COLOR_SCALE(col, amt) (col)
#endif

enum {
    DUB_KNOB_PROG_COL_W = 60,
    DUB_KNOB_PROG_INSET = 6,
    DUB_KNOB_PROG_INNER_W = 48,
    DUB_KNOB_PROG_BARS = 16u
};

static inline int32_t dub_prog_bar_x0(uint8_t bar_ix)
{
    uint8_t col = bar_ix >> 2;
    uint8_t sub = bar_ix & 3u;

    return (int32_t)col * DUB_KNOB_PROG_COL_W + DUB_KNOB_PROG_INSET + (int32_t)sub * (DUB_KNOB_PROG_INNER_W / 4);
}

static inline int32_t dub_prog_bar_w(void)
{
    return DUB_KNOB_PROG_INNER_W / 4;
}

static inline uint32_t dub_knob_rail_frac256(uint32_t acc, uint32_t spp)
{
    if (!spp)
        return 0u;
    return (acc * 256u) / spp;
}

typedef struct {
    uint8_t playing;
    uint8_t ext_clock;
    uint16_t era;
    /* Non-zero: inactive tick colours scaled from era (KO DECK). Zero: legacy C_GRAY/C_HI/C_DIM. */
    uint16_t era_tick_dim;
    uint16_t era_tick_mid;
    uint16_t era_tick_idle;
    uint32_t vis_steps;
    uint32_t cur_ix;
    uint32_t steps_per_bar;
    uint32_t sample_acc;
    uint32_t samples_need;
} dub_knob_rail_ctx_t;

static inline uint32_t dub_knob_rail_quarter(const dub_knob_rail_ctx_t *c)
{
    uint32_t ph = 0u;
    uint32_t q;
    uint32_t spb = c->steps_per_bar;

    if (!spb)
        spb = 1u;
    if (c->playing && !c->ext_clock && c->samples_need > 0u) {
        ph = dub_knob_rail_frac256(c->sample_acc, c->samples_need);
        if (ph > 255u)
            ph = 255u;
    }
    q = (((c->cur_ix % spb) * 256u + ph) * 4u) / (spb * 256u);
    return q > 3u ? 3u : q;
}

/* Returns rail_cell (bar<<2|quarter). Same look as fm_app draw_bar_progress_rail(y_bot). */
static inline uint8_t dub_draw_bar_progress_rail(int32_t y_bot, const dub_knob_rail_ctx_t *c)
{
    int32_t y_bar = y_bot - 7;
    int32_t y_beat = y_bot - 4;
    int32_t y_play = y_bot - 7;
    int32_t ry_bot = y_bot;
    uint16_t era = c->era;
    uint16_t tick_dim = c->era_tick_dim ? c->era_tick_dim : C_DIM;
    uint16_t tick_mid = c->era_tick_mid ? c->era_tick_mid : C_HI;
    uint16_t tick_idle = c->era_tick_idle ? c->era_tick_idle : C_GRAY;
    uint32_t vis_steps = c->vis_steps;
    uint32_t cur_ix = c->cur_ix;
    uint32_t steps_per_bar = c->steps_per_bar;
    uint32_t n_active_bars;
    uint32_t bar_ix;
    int32_t rail_x0 = dub_prog_bar_x0(0);
    int32_t bar_w = dub_prog_bar_w();
    int32_t rail_x1 = dub_prog_bar_x0((uint8_t)(DUB_KNOB_PROG_BARS - 1u)) + bar_w;
    uint32_t cur_bar;
    uint8_t rail_cell = 0u;

    if (steps_per_bar < 1u)
        steps_per_bar = 1u;
    n_active_bars = (vis_steps + steps_per_bar - 1u) / steps_per_bar;
    if (n_active_bars > (uint32_t)DUB_KNOB_PROG_BARS)
        n_active_bars = (uint32_t)DUB_KNOB_PROG_BARS;
    cur_bar = cur_ix / steps_per_bar;
    if (n_active_bars > 0u && cur_bar >= n_active_bars)
        cur_bar = n_active_bars - 1u;

    if (y_bar < 0)
        y_bar = 0;
    if (y_beat < 0)
        y_beat = 0;
    if (y_play < 0)
        y_play = 0;

    if (n_active_bars > 0u)
        cv_line(rail_x0, ry_bot, dub_prog_bar_x0((uint8_t)(n_active_bars - 1u)) + bar_w - 1, ry_bot, C_LINE);
    else
        cv_line(rail_x0, ry_bot, rail_x1 - 1, ry_bot, C_LINE);
    for (bar_ix = 0u; bar_ix < (uint32_t)DUB_KNOB_PROG_BARS; bar_ix++) {
        int32_t x = dub_prog_bar_x0((uint8_t)bar_ix);
        int active = bar_ix < n_active_bars;
        int in_cur = bar_ix == cur_bar;
        int past = active && c->playing && bar_ix < cur_bar;
        int measure = (bar_ix & 3u) == 0u;
        uint16_t col;
        int32_t y_top;

        if (!active) {
            col = C_LINE;
            y_top = ry_bot - 1;
        } else if (measure)
            col = past ? era : (in_cur ? (c->era_tick_mid ? tick_mid : C_WHITE) : tick_idle);
        else if ((bar_ix & 1u) == 0u)
            col = past ? era : (in_cur ? tick_mid : tick_dim);
        else if (past)
            col = C_LINE;
        else
            col = in_cur ? tick_dim : C_LINE;

        if (measure)
            y_top = y_bar;
        else if ((bar_ix & 1u) == 0u)
            y_top = y_beat;
        else
            y_top = ry_bot - 1;
        cv_line(x, y_top, x, ry_bot, col);
    }

    if (n_active_bars > 0u) {
        uint32_t q = dub_knob_rail_quarter(c);
        int32_t bx0 = dub_prog_bar_x0((uint8_t)cur_bar);
        int32_t nx0 = bx0 + ((int32_t)q * bar_w) / 4;
        int32_t nx1 = bx0 + ((int32_t)(q + 1u) * bar_w) / 4;
        int32_t bh = ry_bot - y_play + 1;

        rail_cell = (uint8_t)((cur_bar << 2) | q);
        if (cur_bar < n_active_bars && bh > 0) {
            uint32_t ph = 0u;
            uint32_t prog;
            int32_t bar_blend;

            if (c->playing && !c->ext_clock && c->samples_need > 0u) {
                ph = dub_knob_rail_frac256(c->sample_acc, c->samples_need);
                if (ph > 255u)
                    ph = 255u;
            }
            prog = q * 256u + ph;
            bar_blend = (int32_t)(62u * prog / (4u * 256u));

            if (cur_bar > 0u)
                cv_rect(dub_prog_bar_x0((uint8_t)(cur_bar - 1u)), y_play, bar_w, bh,
                        DUB_KNOB_RAIL_COLOR_SCALE(era, (62 - bar_blend) * 256 / 100));
            cv_rect(bx0, y_play, bar_w, bh, DUB_KNOB_RAIL_COLOR_SCALE(era, 159));
            if (cur_bar + 1u < n_active_bars)
                cv_rect(dub_prog_bar_x0((uint8_t)(cur_bar + 1u)), y_play, bar_w, bh,
                        DUB_KNOB_RAIL_COLOR_SCALE(era, bar_blend * 256 / 100));
            cv_rect(nx0, y_play, nx1 - nx0, bh, era);
        }
    }
    return rail_cell;
}

#endif /* DUB_KNOB_PROGRESS_RAIL_H */
