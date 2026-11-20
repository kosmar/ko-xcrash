/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DECK centre canvas (240×132): SEQ grid or Kit-Scope HOME. Include after platform.c. */
#ifndef DECK_UI_H
#define DECK_UI_H

#include "deck_seq.h"
#include "deck_dsp.h"
#include "deck_plates.h"

/* Genre/BASS reactive accent — not DECK’s purple C_ACC on knobs. */
#define DECK_HOME_CYAN RGB(40, 180, 255)

enum {
    DECK_HOME_FIELD_H = 112,
    DECK_HOME_PUMP_Y = 108,
    DECK_HOME_PERF_Y = 6,
    /* Footer lives in 240×132 blit at y=28; knob bar blits at y=154 → local y≥126 is covered. */
    DECK_HOME_RAIL_Y = 125,
    DECK_HOME_MORPH_Y = 118,
    DECK_HOME_LFO_Y = 119
};

typedef struct {
    uint8_t plate_slot;
    uint8_t era;
    uint8_t plate_era;
    uint8_t lfo_on[4];
    uint8_t lfo_depth[4];
    uint32_t lfo_ph[4];
} deck_home_hud_t;

static inline void deck_ui_u8(char *out, uint8_t v)
{
    out[0] = (char)('0' + (v / 100u) % 10u);
    out[1] = (char)('0' + (v / 10u) % 10u);
    out[2] = (char)('0' + v % 10u);
    out[3] = '\0';
    if (out[0] == '0') {
        out[0] = out[1];
        out[1] = out[2];
        out[2] = '\0';
        if (out[0] == '0' && out[1]) {
            out[0] = out[1];
            out[1] = '\0';
        }
    }
}

#define DECK_UI_LANES 8u
#define DECK_UI_ROW_H FM_SEQ_GRID8_ROW_H
#define DECK_UI_CELL_H FM_SEQ_GRID8_CELL_H
#define DECK_UI_GRID_Y FM_SEQ_GRID8_Y0
#define DECK_UI_NO_STEP 0xFFu
#define DECK_SEQ_LANE_PAGES ((DECK_NVOICES + DECK_UI_LANES - 1u) / DECK_UI_LANES)

static uint8_t deck_ui_win;

static inline uint16_t deck_vcol(uint8_t v)
{
    const uint8_t *c = DECK_VCOL[v % DECK_NVOICES];

    return RGB(c[0], c[1], c[2]);
}

static inline uint16_t deck_ui_mix(uint16_t a, uint16_t b, uint32_t t)
{
    uint32_t ar = (uint32_t)a >> 11, ag = ((uint32_t)a >> 5) & 63u, ab = (uint32_t)a & 31u;
    uint32_t br = (uint32_t)b >> 11, bg = ((uint32_t)b >> 5) & 63u, bb = (uint32_t)b & 31u;

    return (uint16_t)((((ar * (16u - t) + br * t) / 16u) << 11) |
                      (((ag * (16u - t) + bg * t) / 16u) << 5) | ((ab * (16u - t) + bb * t) / 16u));
}

/* 50/50 blend of two RGB565 colors, per channel (5/6/5), via deck_ui_mix at t=8/16 = floor((a+b)/2) per channel. */
static inline uint16_t deck_hue_mix(uint16_t a, uint16_t b)
{
    return deck_ui_mix(a, b, 8u);
}

static inline void deck_ui_frame(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    cv_rect(x, y, w, 1, c);
    cv_rect(x, y + h - 1, w, 1, c);
    cv_rect(x, y, 1, h, c);
    cv_rect(x + w - 1, y, 1, h, c);
}

static inline uint8_t deck_ui_last_step(const deck_runtime_t *rt, const deck_pattern_t *pat, uint8_t v)
{
    const deck_lane_t *ln = &pat->lane[v];
    uint32_t len = fm_seq_lane_len_clamp(ln->length);
    uint32_t s = deck_seq_g.lane_step[v] % len;
    uint32_t back;

    if (!rt->playing || rt->master_step == 0u)
        return DECK_UI_NO_STEP;
    {
        uint8_t ix = fm_seq_clk_ratio_lane_ix(ln->div);
        uint8_t num = FM_SEQ_CLK_RATIO_NUM[ix];
        uint8_t den = FM_SEQ_CLK_RATIO_DEN[ix];

        if (num >= den)
            back = (uint32_t)num / (uint32_t)den;
        else
            back = deck_seq_g.lane_clk_acc[v] ? 1u : (uint32_t)den / (uint32_t)num;
        if (back < 1u)
            back = 1u;
    }
    return (uint8_t)((s + len * 2u - back) % len);
}

static inline void deck_ui_follow(uint8_t cur)
{
    if (cur < deck_ui_win)
        deck_ui_win = cur;
    else if (cur >= deck_ui_win + DECK_UI_LANES)
        deck_ui_win = (uint8_t)(cur - (DECK_UI_LANES - 1u));
    if (deck_ui_win > DECK_NVOICES - DECK_UI_LANES)
        deck_ui_win = (uint8_t)(DECK_NVOICES - DECK_UI_LANES);
}

static inline void deck_ui_draw_seq(const deck_runtime_t *rt, const deck_pattern_t *pat, uint8_t plock_latch,
                                    uint8_t plock_abs_step)
{
    fm_seq_cell_t cells[DECK_UI_LANES * FM_SEQ_PAGE_STEPS];
    const char *lane_lbl[DECK_UI_LANES];
    uint16_t lcols[DECK_UI_LANES];
    uint8_t play_st[DECK_UI_LANES];
    uint32_t base;
    uint8_t cur;
    uint8_t sel_rel = 0u;
    uint8_t plock_st = FM_SEQ_PLOCK_NONE;
    uint32_t row;
    uint32_t st;
    int base_hit;
    int vary_on;

    if (!rt || !pat)
        return;
    cur = (uint8_t)(rt->cur_voice % DECK_NVOICES);
    deck_ui_follow(cur);
    base = deck_seq_view_base(rt);
    if (plock_latch && plock_abs_step >= base && plock_abs_step < base + FM_SEQ_PAGE_STEPS)
        plock_st = (uint8_t)(plock_abs_step - base);
    if (cur >= deck_ui_win && cur < deck_ui_win + DECK_UI_LANES)
        sel_rel = (uint8_t)(cur - deck_ui_win);

    for (row = 0u; row < DECK_UI_LANES; row++) {
        uint8_t v = (uint8_t)(deck_ui_win + row);
        const deck_lane_t *ln = &pat->lane[v];
        int aud = deck_seq_lane_audible(rt, v) ? 1 : 0;
        int solo = deck_seq_lane_is_solo(rt, v) ? 1 : 0;

        lane_lbl[row] = DECK_VNAME_SHORT[v];
        if (!aud)
            lcols[row] = C_DIM;
        else if (solo)
            lcols[row] = C_ACC;
        else
            lcols[row] = deck_vcol(v);
        play_st[row] = FM_SEQ_PLOCK_NONE;
        {
            uint8_t last = deck_ui_last_step(rt, pat, v);

            if (last != DECK_UI_NO_STEP && last >= base && last < base + FM_SEQ_PAGE_STEPS)
                play_st[row] = (uint8_t)(last - base);
        }
        for (st = 0u; st < FM_SEQ_PAGE_STEPS; st++) {
            fm_seq_cell_t *c = &cells[row * FM_SEQ_PAGE_STEPS + st];
            uint32_t ab = base + st;
            int sel = v == cur;

            c->bg_col = sel ? RGB(22, 26, 44) : C_LINE;
            c->col = 0u;
            c->vel = 0u;
            c->tune = 0;
            c->micro = 0;
            c->cond = 0u;
            c->gate = 0;
            c->note_span = 0u;
            c->note_tail = 0u;
            /* Beyond lane length: plain empty cell, still tappable (deck_seq_toggle_step extends the lane). */
            if (ab >= ln->length)
                continue;
            /* VARY as playback (deck_seq_lane_step_resolve): added hit C_ACC, removed hit C_DIM (GENRE grid). */
            base_hit = fm_seq_bit(ln->hit, ab) ? 1 : 0;
            vary_on = fm_seq_vary_hit(base_hit, ln->vary, (uint8_t)ab,
                                      (uint8_t)((rt->master_step / FM_SEQ_PAGE_STEPS) & 7u), v,
                                      deck_seq_vary_seed(rt->master_step));
            if (vary_on) {
                uint8_t acc = deck_seq_get_accent(pat, v, ab);
                uint8_t pat_ix = rt->cur_pat % DECK_NPAT;
                uint8_t bar = (uint8_t)((rt->master_step / FM_SEQ_PAGE_STEPS) & 7u);
                uint8_t vel = deck_plock_vel(pat_ix, v, (uint8_t)ab, acc);
                int32_t vr;
                int32_t tune;
                int32_t gr;

                /* Same rolls as deck_seq_lane_step_resolve / deck_seq_lane_gsr. */
                if (acc && vel < (uint8_t)deck_seq_lv_lerp(acc, 100, 118))
                    vel = (uint8_t)deck_seq_lv_lerp(acc, 100, 118);
                vr = (int32_t)vel + (int32_t)fm_seq_rand_vel(ln->vel_rng, (uint8_t)ab, bar, v, 0u);
                c->vel = (uint8_t)(vr < 1 ? 1 : vr > 127 ? 127 : vr);
                tune = (int32_t)deck_plock_pitch(pat_ix, v, (uint8_t)ab) +
                       (int32_t)fm_seq_rand_pitch(ln->pitch_rng, (uint8_t)ab, bar, v, 0u);
                c->tune = (int8_t)(tune < -24 ? -24 : tune > 24 ? 24 : tune);
                c->micro = deck_plock_micro(pat_ix, v, (uint8_t)ab);
                c->cond = deck_plock_cond(pat_ix, v, (uint8_t)ab);
                if (c->cond >= FM_SEQ_NCONDS)
                    c->cond = 0u;
                gr = (int32_t)deck_seq_step_gate(pat_ix, pat, v, (uint8_t)ab) +
                     (int32_t)fm_seq_rand_gate(ln->gate_rng, (uint8_t)ab, bar, v, 0u);
                /* 0 and TIE never choke: widest body; else DECK_SEQ_GATE_DEF = plain cell. */
                if (gr <= 0 || gr >= (int32_t)DECK_SEQ_GATE_TIE)
                    c->gate = 63;
                else
                    c->gate = (int8_t)(((gr - (int32_t)DECK_SEQ_GATE_DEF) * 63) / (int32_t)DECK_SEQ_GATE_DEF);
                c->col = deck_ui_mix(deck_vcol(v), C_WHITE, (uint32_t)deck_seq_lv_lerp(acc, 0, 9));
                if (!base_hit)
                    c->col = C_ACC;
                if (!fm_seq_trig_pass(ln->trig, (uint8_t)ab, bar, v))
                    c->col = C_DIM;
                else if (!aud)
                    c->col = deck_ui_mix(c->col, C_BLACK, 11u);
            } else if (base_hit) {
                /* VARY removes this hit at playback: draw dim, velocity only sets bar height. */
                c->vel = 96u;
                c->col = C_DIM;
            }
        }
    }

    fm_seq_draw_grid(DECK_UI_LANES, sel_rel, play_st[sel_rel], rt->playing ? 1 : 0, plock_st,
                     plock_latch ? 1 : 0, sel_rel, C_WHITE, lane_lbl, lcols, cells, play_st, NULL, NULL);

    for (row = 0u; row < DECK_UI_LANES; row++) {
        uint8_t v = (uint8_t)(deck_ui_win + row);
        uint8_t last = deck_ui_last_step(rt, pat, v);
        int32_t y = DECK_UI_GRID_Y + (int32_t)row * DECK_UI_ROW_H;
        uint16_t lit;

        if (!deck_seq_lane_audible(rt, v) || last == DECK_UI_NO_STEP || !fm_seq_bit(pat->lane[v].hit, last))
            continue;
        if (row == sel_rel)
            lit = deck_vcol(v);
        else
            lit = deck_ui_mix(lcols[row], C_WHITE,
                              (uint32_t)deck_seq_lv_lerp(deck_seq_get_accent(pat, v, last), 8, 12));
        cv_rect(1, y + 1, FM_SEQ_LANE_LBL_W, DECK_UI_CELL_H + 2, lit);
        cv_text(fm_seq_lane_lbl_tx(lane_lbl[row]), y + 1, &FONT_S, lane_lbl[row], C_BLACK);
    }
}

static inline uint16_t deck_ui_scale(uint16_t col, int32_t scale256)
{
    if (scale256 <= 0)
        return C_BLACK;
    if (scale256 >= 256)
        return col;
    return deck_ui_mix(C_BLACK, col, (uint32_t)((scale256 * 16) / 256));
}

#define FM_ENV_KIT_SCALE deck_ui_scale
#include "fm_env_kit.h"
#define FM_GROOVE_PLOT_SCALE deck_ui_scale
#define FM_GROOVE_PLOT_SINE(a) deck_sine((uint32_t)(a) << 16)
#include "fm_groove_plot.h"
#include "dub_types.h"
#define DUB_DRAW_COLOR_SCALE deck_ui_scale
#define DUB_DRAW_SINE deck_sine
static inline void deck_dub_copy_str(char *dst, const char *src)
{
    while (*src)
        *dst++ = *src++;
    *dst = '\0';
}
#define DUB_DRAW_COPY_STR deck_dub_copy_str
#include "dub_draw_space.h"
#define DUB_KNOB_RAIL_COLOR_SCALE deck_ui_scale
#include "dub_knob_progress_rail.h"

static inline void deck_ui_home_perf_mark(int32_t x, int32_t y, uint8_t val, uint8_t center, uint16_t col)
{
    int32_t dev = (int32_t)val - (int32_t)center;
    int32_t mag = dev < 0 ? -dev : dev;
    int32_t h = 1 + (mag * 6) / 64;
    int32_t mid = y + 4;
    int32_t bright = 72 + (mag >> 1) + (val >> 2);

    if (h > 7)
        h = 7;
    if (dev >= 0)
        cv_rect(x, mid - h, 4u, (uint32_t)(h + 1), deck_ui_scale(col, bright));
    else
        cv_rect(x, mid, 4u, (uint32_t)(h + 1), deck_ui_scale(col, bright));
    cv_rect(x + 1, mid, 2u, 1u, deck_ui_scale(C_DIM, 48 + (center >> 2)));
}

static inline int32_t deck_ui_saw(uint32_t ph)
{
    return (int32_t)(int16_t)(ph >> 16) >> 1;
}

/* Kit-stage HOME (y 0..111): 16-pad kit + thick reactive scope. Footer from y 112. */

enum {
    DECK_HOME_SCOPE_TOP = 18,
    DECK_HOME_SCOPE_BOT = 82,
    DECK_HOME_SCOPE_X1 = 232,
    DECK_HOME_KIT_Y = 86,
    DECK_HOME_KIT_H = 18,
    DECK_HOME_PAD_W = 13
};

static inline int deck_ui_home_quiet(const deck_runtime_t *rt, uint8_t v)
{
    return rt && !deck_seq_lane_audible(rt, v);
}

static inline uint8_t deck_ui_home_kk(const uint8_t *pulse, uint8_t kick_pump)
{
    uint8_t kk = kick_pump;

    if (pulse && pulse[DECK_BD] > kk)
        kk = pulse[DECK_BD];
    return kk;
}

static inline int32_t deck_ui_home_macro_d(uint8_t v)
{
    return (int32_t)v - 64;
}

static inline int32_t deck_ui_home_voice_x(uint8_t v)
{
    return 8 + (int32_t)v * 14 + 6;
}

/* Genre HOME: shallow perspective rays into the scope (draw_view_home layer 1). */
static inline void deck_ui_home_depth_rays(int32_t vp_x, int32_t vp_y, uint8_t playing, int32_t filt_d)
{
    uint32_t i;
    int32_t br_base = 28 + filt_d * 3 + (playing ? 12 : 0);

    for (i = 0u; i < 9u; i++) {
        int32_t fx = 4 + (int32_t)i * 29;
        int32_t bx = vp_x + ((fx - 120) * 52) / 128;
        int32_t fy = DECK_HOME_SCOPE_BOT + 2;

        if (bx < 2)
            bx = 2;
        cv_line(bx, vp_y + 3, fx, fy, deck_ui_scale(C_LINE, br_base + (int32_t)(i & 1u) * 8));
    }
}

static inline void deck_ui_home_vu_tick(uint8_t vu[DECK_NVOICES], const deck_runtime_t *rt, const uint8_t *pulse,
                                        uint8_t kick_pump, int32_t decay_d)
{
    uint32_t v;
    /* Low decay macro → fast release; high → long smear (~10 knob units visible). */
    int32_t drop = 18 + ((decay_d + 64) * 22) / 127;

    if (drop < 6)
        drop = 6;
    if (drop > 48)
        drop = 48;

    for (v = 0u; v < DECK_NVOICES; v++) {
        uint8_t hit = pulse ? pulse[v] : 0u;

        if ((uint8_t)v == DECK_BD && kick_pump > hit)
            hit = kick_pump;
        if (hit >= vu[v])
            vu[v] = hit;
        else if (vu[v] > drop)
            vu[v] = (uint8_t)(vu[v] - (uint8_t)drop);
        else
            vu[v] = 0u;
        if (deck_ui_home_quiet(rt, (uint8_t)v) && vu[v] > 24u)
            vu[v] = 24u;
    }
}

static inline void deck_ui_home_draw_kit(const deck_runtime_t *rt, const uint8_t *pulse, uint8_t kick_pump,
                                         uint16_t sel_col, uint8_t sel, int32_t crush_d, int32_t filt_d)
{
    uint32_t v;
    int32_t mid_y = DECK_HOME_KIT_Y + DECK_HOME_KIT_H / 2;

    cv_line(6, DECK_HOME_KIT_Y - 1, 234, DECK_HOME_KIT_Y - 1, deck_ui_scale(C_LINE, 56 + filt_d * 2));

    for (v = 0u; v < DECK_NVOICES; v++) {
        int32_t cx = deck_ui_home_voice_x((uint8_t)v);
        int32_t x0 = cx - DECK_HOME_PAD_W / 2;
        uint16_t vcol = deck_vcol((uint8_t)v);
        uint8_t pv = pulse ? pulse[v] : 0u;
        int on = (uint8_t)v == sel;
        int quiet = deck_ui_home_quiet(rt, (uint8_t)v);
        int32_t h;
        uint16_t fill;

        if ((uint8_t)v == DECK_BD && kick_pump > pv)
            pv = kick_pump;
        h = 4 + ((int32_t)pv * (DECK_HOME_KIT_H - 2)) / 110;
        if (h > DECK_HOME_KIT_H - 2)
            h = DECK_HOME_KIT_H - 2;
        if (crush_d > 4)
            h = (h >> (crush_d >> 4)) << (crush_d >> 4);
        fill = quiet ? deck_ui_scale(vcol, 32) : deck_ui_scale(vcol, on ? 220 : 120);
        if (pv > 48 && !quiet)
            fill = deck_ui_mix(fill, C_WHITE, (uint32_t)(pv >> 4));
        if (on && !quiet)
            fill = deck_ui_mix(fill, sel_col, 6u);
        cv_rect(x0, mid_y - h / 2, (uint32_t)DECK_HOME_PAD_W, (uint32_t)h, fill);
        if (pv > 72 && !quiet) {
            cv_rect(x0, mid_y - h / 2 - 1, (uint32_t)DECK_HOME_PAD_W, 1u, deck_ui_scale(vcol, 255));
            cv_rect(x0, mid_y + h / 2, (uint32_t)DECK_HOME_PAD_W, 1u, deck_ui_scale(vcol, 200));
        }
        if (on)
            cv_rect(x0 - 1, DECK_HOME_KIT_Y, (uint32_t)(DECK_HOME_PAD_W + 2u), (uint32_t)DECK_HOME_KIT_H,
                    deck_ui_scale(sel_col, 90));
    }
}

/* Right-edge voice tags removed — full scope width (DECK_HOME_SCOPE_X1). */
/* Thick multi-voice scope — genre crest_glow + body fill on transients. */
static inline void deck_ui_home_draw_scope(const deck_runtime_t *rt, const uint8_t vu[DECK_NVOICES],
                                           uint32_t anim_ph, uint32_t beat_ph, uint8_t playing, uint16_t sel_col,
                                           uint8_t sel, int32_t tune_d, int32_t decay_d, int32_t filt_d, int32_t crush_d)
{
    int32_t mid = (DECK_HOME_SCOPE_TOP + DECK_HOME_SCOPE_BOT) / 2;
    int32_t x_step = 1 + (crush_d > 0 ? crush_d >> 4 : 0);
    int32_t vp_x = 120 + (playing ? ((deck_sine(beat_ph << 1) * (12 + tune_d)) >> 17) : (deck_sine(anim_ph) >> 15));
    int32_t vp_y = DECK_HOME_SCOPE_TOP + 6 - (tune_d >> 3);
    uint32_t ph_base = playing ? beat_ph : anim_ph;
    int32_t px = -1;
    int32_t py = mid;
    int32_t x;

    if (x_step > 6)
        x_step = 6;

    deck_ui_home_depth_rays(vp_x, vp_y, playing, filt_d);
    cv_line(8, mid, DECK_HOME_SCOPE_X1, mid, deck_ui_scale(C_DIM, 40 + filt_d * 2));

    for (x = 8; x <= DECK_HOME_SCOPE_X1; x += x_step) {
        int32_t dy = 0;
        int32_t peak_v = -1;
        int32_t peak_mag = 0;
        uint32_t v;
        uint16_t col = deck_ui_mix(C_DIM, sel_col, 4u);
        int32_t crest = 0;

        for (v = 0u; v < DECK_NVOICES; v++) {
            int32_t vx = deck_ui_home_voice_x((uint8_t)v);
            int32_t dx = x - vx;
            int32_t env = 200 - ((dx * dx) >> 3);
            int32_t amp = (int32_t)vu[v];
            int32_t w;
            uint32_t freq;

            if (env < 0)
                env = 0;
            if (deck_ui_home_quiet(rt, (uint8_t)v))
                amp >>= 2;
            freq = (uint32_t)(520 + tune_d * 14 + (int32_t)v * 11 + (decay_d >> 2));
            w = (deck_sine(ph_base + (uint32_t)x * freq * 0x8000u + v * 0x0A000000u) * amp * env) >> 18;
            if (crush_d > 8 && (v & 1u))
                w = (w * 5) / 4 + ((x & (4 + (crush_d >> 3))) ? (amp >> 3) : -(amp >> 3));
            dy += w;
            if (amp > peak_mag) {
                peak_mag = amp;
                peak_v = (int32_t)v;
            }
        }
        crest = dy < 0 ? -dy : dy;
        if (peak_v >= 0 && peak_mag > 20)
            col = deck_vcol((uint8_t)peak_v);
        if ((uint8_t)peak_v == sel && peak_mag > 12)
            col = deck_ui_mix(col, sel_col, 9u);
        {
            int32_t glow = 90 + crest * 6 + (filt_d * 3) + (peak_mag >> 1);

            if (glow > 255)
                glow = 255;
            col = deck_ui_scale(col, glow);
        }
        {
            int32_t y = mid - dy;

            if (crush_d > 0) {
                int32_t q = 2 + (crush_d >> 3);

                y = (y >> q) << q;
            }
            if (y < DECK_HOME_SCOPE_TOP)
                y = DECK_HOME_SCOPE_TOP;
            if (y > DECK_HOME_SCOPE_BOT)
                y = DECK_HOME_SCOPE_BOT;
            if (px >= 0) {
                int32_t thick = 1 + (filt_d >> 4) + (crest > 10 ? 1 : 0);
                int32_t t;

                for (t = -thick; t <= thick; t++) {
                    if (py + t >= DECK_HOME_SCOPE_TOP && y + t <= DECK_HOME_SCOPE_BOT)
                        cv_line(px, py + t, x, y + t, col);
                }
                if (crest > 8 && filt_d > -20) {
                    int32_t fill_x = (px + x) >> 1;

                    if (fill_x >= 8 && fill_x <= DECK_HOME_SCOPE_X1)
                        cv_line(fill_x, mid, fill_x, (mid + y) >> 1, deck_ui_scale(col, 72 + filt_d * 2));
                }
                if (crest > 14 && peak_mag > 40)
                    cv_line(x, y, x, mid, deck_ui_mix(col, C_WHITE, 5u));
            }
            px = x;
            py = y;
        }
    }
    if (playing) {
        int32_t sx = 8 + ((int32_t)(rt->master_step & 15u) * 224) / 15;

        cv_rect(sx, DECK_HOME_SCOPE_TOP, 2, DECK_HOME_SCOPE_BOT - DECK_HOME_SCOPE_TOP,
                deck_ui_scale(DECK_HOME_CYAN, 180));
    }
}

/* KO BASS draw_bar_progress_rail → dub_draw_bar_progress_rail (16-step knob playhead). */
static inline void deck_ui_knob_progress_rail(const deck_runtime_t *rt, uint16_t era, int32_t y_bot)
{
    dub_knob_rail_ctx_t c = {0};

    c.vis_steps = 16u;
    c.steps_per_bar = 1u;
    c.era = era;
    c.era_tick_idle = deck_ui_scale(era, 68);
    c.era_tick_mid = deck_ui_scale(era, 168);
    c.era_tick_dim = deck_ui_scale(era, 44);
    c.playing = rt->playing ? 1u : 0u;
    c.ext_clock = 0u;
    if (rt->playing) {
        uint32_t base = deck_seq_s16_base_samples(rt->bpm);
        uint32_t sounding = rt->master_step ? (rt->master_step - 1u) % 16u : 0u;
        uint8_t odd = (uint8_t)(sounding & 1u);
        uint32_t need = deck_seq_s16_len_samples(base, rt->swing, odd);

        c.cur_ix = sounding;
        c.sample_acc = rt->step_phase;
        c.samples_need = need;
    } else
        c.cur_ix = (uint32_t)(rt->master_step % 16u);
    (void)dub_draw_bar_progress_rail(y_bot, &c);
}

static inline void deck_ui_home_drum_field(const deck_runtime_t *rt, const uint8_t *pulse, uint8_t kick_pump,
                                           uint32_t anim_ph, uint32_t beat_ph, uint8_t playing,
                                           const deck_perf_t *pf)
{
    static uint8_t deck_home_vu[DECK_NVOICES];
    uint8_t kk = deck_ui_home_kk(pulse, kick_pump);
    uint8_t sel = rt ? (uint8_t)(rt->cur_voice % DECK_NVOICES) : 0u;
    uint16_t sel_col = deck_vcol(sel);
    int32_t tune_d = pf ? deck_ui_home_macro_d(pf->tune) : 0;
    int32_t decay_d = pf ? deck_ui_home_macro_d(pf->decay) : 0;
    int32_t filt_d = pf ? deck_ui_home_macro_d(pf->filter) : 0;
    int32_t crush_d = pf ? deck_ui_home_macro_d(pf->crush) : 0;

    deck_ui_home_vu_tick(deck_home_vu, rt, pulse, kick_pump, decay_d);

    cv_rect(0, 0, 240, DECK_HOME_FIELD_H, C_BLACK);
    deck_ui_home_draw_scope(rt, deck_home_vu, anim_ph, beat_ph, playing, sel_col, sel, tune_d, decay_d, filt_d,
                            crush_d);
    deck_ui_home_draw_kit(rt, pulse, kick_pump, sel_col, sel, crush_d, filt_d);
    if (kk > 32)
        cv_rect(0, DECK_HOME_PUMP_Y - 2, 240, 2, deck_ui_scale(deck_vcol(DECK_BD), 100 + kk / 2));
}

/* Plate-in-era ticks (KO BASS `genre_fader` slot); lit slot follows `plate_slot`. */
static inline void deck_ui_home_morph_marks(const deck_home_hud_t *h, uint16_t abyss)
{
    uint8_t slot = h->plate_slot % DECK_PLATES_PER_ERA;
    uint16_t ecol = deck_era_col(h->era);
    uint16_t pcol = deck_era_col(h->plate_era);
    uint32_t i;

    cv_line(8, DECK_HOME_MORPH_Y, 232, DECK_HOME_MORPH_Y, deck_ui_scale(abyss, 88));
    for (i = 0u; i < DECK_PLATES_PER_ERA; i++) {
        int32_t x = 8 + (int32_t)(i * 224u) / (int32_t)(DECK_PLATES_PER_ERA - 1u);
        int32_t y0 = DECK_HOME_MORPH_Y - (i == (uint32_t)slot ? 3 : 1);
        uint16_t col = i == (uint32_t)slot ? deck_ui_scale(pcol, 220) : deck_ui_scale(ecol, 48);

        cv_line(x, y0, x, DECK_HOME_MORPH_Y, col);
    }
    /* No deck DSP morph/xfade/cue yet — chevron only meant transient morph; omit to avoid stuck V. */
}

/* Per-knob-column LFO marks — ko_draw_lfo_mark (phase nudges x when depth>0). */
static inline void deck_ui_home_lfo_marks(const deck_home_hud_t *h)
{
    uint32_t col;

    for (col = 0u; col < 4u; col++) {
        int32_t x = (int32_t)col * 60 + (int32_t)((h->lfo_ph[col] >> 26) & 3u);
        uint16_t col_c;

        if (!h->lfo_on[col])
            continue;
        col_c = deck_ui_scale(C_ACC, 96 + (int32_t)h->lfo_depth[col]);
        ko_draw_lfo_mark(x, col_c);
    }
}

#include "ko_chrome.h"

/* DECK Kit-Scope HOME: drum field + footer HUD (perform / morph / LFO / rail). */
static inline void deck_ui_draw_home(const deck_runtime_t *rt, const uint8_t *pulse, uint8_t kick_pump,
                                     const deck_perf_t *pf, uint8_t accent, uint8_t human,
                                     const deck_home_hud_t *hud)
{
    uint8_t perf_buf[10];
    static const uint8_t perf_ctr[10] = {64u, 64u, 64u, 64u, 64u, 0u, 0u, 0u, 96u, 0u};
    uint32_t pi;
    const uint8_t *pv;
    uint32_t step_frac256 = rt->playing ? (rt->step_phase & 255u) : 0u;
    static uint32_t deck_home_anim_ph;
    static uint32_t deck_home_anim_ms;
    uint32_t ms = fm1_ms;
    uint32_t dt = ms - deck_home_anim_ms;
    uint16_t sel_col;
    uint16_t abyss = RGB(4, 14, 38);

    if (dt > 80u)
        dt = 80u;
    deck_home_anim_ms = ms;
    /* ~1 Hz across the lane stagger (BASS scope_ph). The old +0x28000 step was frozen on screen. */
    deck_home_anim_ph += (dt * (rt->playing ? 94u : 70u)) << 16;
    deck_home_anim_ph += ((uint32_t)kick_pump * dt) << 14;

    if (pf) {
        perf_buf[0] = pf->tune;
        perf_buf[1] = pf->decay;
        perf_buf[2] = pf->filter;
        perf_buf[3] = pf->crush;
        perf_buf[4] = pf->density;
        perf_buf[5] = pf->fill;
        perf_buf[6] = deck_lane_p3_bar_px(deck_lane_p3_state_pos(rt, rt->cur_voice % DECK_NVOICES));
        perf_buf[7] = pf->stutter;
        perf_buf[8] = accent;
        perf_buf[9] = human;
    } else {
        uint32_t zi;

        for (zi = 0u; zi < 10u; zi++)
            perf_buf[zi] = perf_ctr[zi];
    }

    sel_col = deck_vcol(rt->cur_voice);
    deck_ui_home_drum_field(
        rt, pulse, kick_pump, deck_home_anim_ph,
        rt->playing ? (((uint32_t)(rt->master_step & 15u) << 24) + (step_frac256 << 16)) : (fm1_ms << 16),
        rt->playing ? 1u : 0u, pf);

    cv_rect(0, DECK_HOME_FIELD_H, 240, 132 - DECK_HOME_FIELD_H, abyss);

    pv = perf_buf;
    for (pi = 0u; pi < 10u; pi++) {
        int32_t px = 6 + (int32_t)pi * 23;
        uint16_t pcol = pi < 4u ? deck_ui_mix(sel_col, DECK_HOME_CYAN, 7u) : (pi < 8u ? DECK_HOME_CYAN : C_GRAY);

        deck_ui_home_perf_mark(px, DECK_HOME_PERF_Y, pv[pi], perf_ctr[pi], pcol);
    }

    if (hud) {
        deck_ui_home_morph_marks(hud, abyss);
        deck_ui_home_lfo_marks(hud);
    }
}

static inline void deck_master_to_dub_fx(const deck_master_t *fx, uint8_t bpm, dub_params_t *p)
{
    p->dly_lvl = fx->dly_lvl;
    p->dly_time = fx->dly_time;
    p->feedback = fx->feedback;
    p->loop_filt = fx->loop_filt;
    p->wow = fx->wow;
    p->tap = fx->tap;
    p->tank = fx->tank;
    p->isolator = fx->isolator;
    p->rev_mix = fx->rev_mix;
    p->rev_damp = fx->rev_damp;
    p->rev_tail = fx->rev_tail;
    p->dly_rev = fx->dly_rev;
    p->bpm = bpm ? bpm : 120u;
}

/* Genre SPACE diagram (dub_draw_space) + DECK dirt pages 3–4 on same wet bus plot. */
static inline void deck_ui_draw_fx(const deck_master_t *fx, uint8_t bypass, uint8_t pg, uint8_t bpm)
{
    dub_params_t dp;
    const char *hdr = "SPACE";

    if (!fx)
        return;
    deck_master_to_dub_fx(fx, bpm, &dp);
    if (pg >= 3u) {
        dub_draw_dirt_t dirt;

        hdr = pg == 3u ? "DRIVE" : "MIX";
        dirt.pg = pg;
        dirt.drive = fx->drive;
        dirt.crush = fx->crush;
        dirt.filter = fx->filter;
        dirt.width = fx->width;
        dirt.comp = fx->comp;
        dirt.dust = fx->dust;
        dirt.tape = fx->tape;
        dub_draw_view_space(&dp, 0, fm1_ms, bypass ? 1 : 0, hdr);
        dub_draw_space_dirt_overlay(&dirt, &dp, fm1_ms, bypass ? 1 : 0);
        return;
    }
    dub_draw_view_space(&dp, 0, fm1_ms, bypass ? 1 : 0, hdr);
}

static inline void deck_ui_env_weights(const deck_voice_cfg_t *vc, uint8_t delay_seg, uint32_t w[5])
{
    w[0] = (uint32_t)delay_seg;
    w[1] = vc->click > DECK_ATK_KNEE ? (uint32_t)(vc->click - DECK_ATK_KNEE) * 2u : 1u;
    w[2] = (uint32_t)vc->hold;
    w[3] = vc->decay ? (uint32_t)vc->decay : 1u;
    w[4] = vc->release ? (uint32_t)vc->release : 1u;
}

static inline int deck_ui_pitch_env_plot_visible(const deck_voice_cfg_t *vc, uint8_t pitch_lfo, uint8_t pdec_lfo)
{
    (void)vc;
    (void)pitch_lfo;
    (void)pdec_lfo;
    return 1;
}

static inline int32_t deck_env_plot_map_y(int32_t y, int32_t y_top, int32_t y_bot, uint8_t invert)
{
    return fm_env_plot_map_y(y, y_top, y_bot, invert);
}

static inline int32_t deck_env_amp_peak(const deck_voice_t *dv)
{
    int32_t pk;

    if (!dv)
        return 1;
    pk = (int32_t)dv->vel << 9;
    if (pk < 1)
        pk = 1;
    return pk;
}

/* Map runtime amp to playhead on the static AHDSR stroke (BASS-style: time ≈ farther right). */
static inline int32_t deck_env_playhead_x(int32_t live, int32_t peak, int32_t y_rng, const int32_t *px, int32_t y_bot,
                                          int32_t y_peak, int32_t y_sus, uint8_t curve)
{
    int32_t plot_y;
    int32_t x;
    int32_t best_x = -1;
    int32_t best_err = 0x7fffffff;

    if (peak < 1)
        peak = 1;
    if (live <= 0)
        return -1;
    if (live > peak)
        live = peak;
    while (peak > 0x7fffff) { /* keeps live * y_rng in int32: no __divdi3 on pi32v2 */
        peak >>= 1;
        live >>= 1;
    }
    plot_y = y_bot - live * y_rng / peak;
    if (plot_y < y_peak)
        plot_y = y_peak;
    if (plot_y > y_bot)
        plot_y = y_bot;
    for (x = px[1]; x <= px[6]; x++) {
        int32_t cy = fm_env_curve_y(x, px, y_bot, y_peak, y_sus, curve);
        int32_t err = cy - plot_y;

        if (err < 0)
            err = -err;
        if (err < best_err || (err == best_err && x > best_x)) {
            best_err = err;
            best_x = x;
        }
    }
    return best_x;
}

/* KO KIT ENV plot (fm_env_kit.h) + drum pitch tail overlay. */
static inline void deck_ui_draw_env(const deck_runtime_t *rt, const deck_voice_cfg_t *vc, const deck_voice_t *dv,
                                    uint8_t pulse, uint8_t delay_seg, uint8_t pitch_lfo, uint8_t pdec_lfo,
                                    uint8_t env_invert, uint8_t env_retrig)
{
    const int32_t x0 = FM_ENV_KIT_X0;
    const int32_t plot_w = FM_ENV_KIT_X1 - FM_ENV_KIT_X0;
    const int32_t y_bot = FM_ENV_KIT_Y_BOT;
    const int32_t y_top = FM_ENV_KIT_Y_TOP;
    const int32_t y_rng = y_bot - y_top;
    fm_env_kit_plot_t ep;
    fm_env_kit_geom_t gm;
    uint32_t pw[5];
    int32_t raw_live = 0;
    int32_t live = 0;
    uint16_t ecol = rt ? deck_vcol(rt->cur_voice) : DECK_HOME_CYAN;
    int show_pitch = deck_ui_pitch_env_plot_visible(vc, pitch_lfo, pdec_lfo);

    if (!vc)
        return;
    if (dv) {
        raw_live = dv->env > dv->env2 ? dv->env : dv->env2;
        if (dv->hat_env > raw_live)
            raw_live = dv->hat_env;
        live = raw_live;
        if (env_invert) {
            int32_t pk = deck_env_amp_peak(dv);

            live = pk - live;
            if (live < 0)
                live = 0;
        }
    }
    deck_ui_env_weights(vc, delay_seg, ep.w);
    ep.sustain = vc->sustain;
    ep.curve = vc->curve;
    ep.invert = env_invert;
    ep.col = ecol;
    fm_env_kit_draw(&ep, &gm);

    if (show_pitch) {
        deck_voice_cfg_t pc = *vc;
        int32_t px_p[7];
        int32_t y_ppeak;
        int32_t y_psus = y_bot - 4;
        uint16_t pitch_col = deck_ui_scale(C_ACC, 150);
        uint8_t pseg;

        pc.decay = vc->pitch_dec;
        if (pc.decay < 1u)
            pc.decay = 1u;
        pc.sustain = 0u;
        deck_ui_env_weights(&pc, 0u, pw);
        fm_env_plot_bounds(pw, x0, plot_w, px_p);
        y_ppeak = y_bot - (int32_t)((deck_pitch_sweep_mag(vc) * (y_bot - gm.y_peak)) / 63);
        for (pseg = 2u; pseg < 5u; pseg++) {
            int32_t py0 = deck_env_plot_map_y(
                fm_env_curve_y(px_p[pseg - 1u], px_p, y_bot, y_ppeak, y_psus, vc->curve), y_top, y_bot, env_invert);
            int32_t py1 = deck_env_plot_map_y(fm_env_curve_y(px_p[pseg], px_p, y_bot, y_ppeak, y_psus, vc->curve), y_top,
                                              y_bot, env_invert);

            cv_line(px_p[pseg - 1u], py0, px_p[pseg], py1, pitch_col);
        }
        if (px_p[2] >= x0 + 4)
            cv_text(px_p[2] - 3, y_ppeak - 7, &FONT_S, "P", pitch_col);
    }

    if (dv && dv->active && (raw_live > 0 || live > 0)) {
        int32_t peak = deck_env_amp_peak(dv);

        /* Time → X from raw envelope; INVERT is amp-only in DSP (deck_amp_env). */
        fm_env_kit_draw_playhead(&ep, &gm,
                                 raw_live > 0 ? deck_env_playhead_x(raw_live, peak, y_rng, gm.px, y_bot, gm.y_peak,
                                                                    gm.y_sus, vc->curve) :
                                                gm.px[1]);
    }
    fm_env_kit_draw_gate(pulse > 12u ? 1u : 0u, env_retrig);
    if (rt)
        cv_text(4, 4, &FONT_S, DECK_VNAME_SHORT[rt->cur_voice % DECK_NVOICES], ecol);
}

static inline int32_t deck_ui_sel_morph_sample(uint32_t ph, uint8_t filt, uint8_t drive, uint8_t timbre)
{
    int32_t s = deck_sine(ph);
    int32_t w = s;
    int32_t soft = 127 - (int32_t)filt;

    if (timbre > 0u) {
        int32_t tb = (int32_t)timbre;

        w += deck_sine(ph + ((uint32_t)tb << 17)) >> (9 + (127 - tb) / 40);
        w += deck_sine(ph * 2u + ((uint32_t)tb << 15)) >> (10 + (127 - tb) / 32);
    }
    if (soft > 0) {
        int32_t round = (s * soft) >> 8;

        w = s - (round >> 1);
    }
    if (filt > 6u) {
        int32_t saw = deck_ui_saw(ph >> 1) >> 2;
        int32_t blend = (int32_t)filt;

        w = (w * (128 - blend) + saw * blend) >> 7;
    }
    if (filt > 48u) {
        w += deck_sine(ph * 3u) >> (10 - ((int32_t)filt >> 5));
        w += deck_ui_saw(ph << 1) >> (11 - ((int32_t)filt >> 4));
    }
    if (drive > 0u) {
        w = (w * (256 + (int32_t)drive * 6)) >> 8;
        if (w > 30000) {
            w = 60000 - w;
        } else if (w < -30000) {
            w = -60000 - w;
        }
    }
    return w;
}

#define DECK_UI_SEL_VC_N 64u
#define DECK_UI_SEL_VC_TRAILS 3u
/* 2^32 / DECK_UI_SEL_VC_N — closed Lissajous loop */
#define DECK_UI_SEL_VC_STEP 67108864u

static inline int32_t deck_ui_sel_vc_fold(int32_t v)
{
    int32_t k;

    for (k = 0; k < 4 && (v > 32767 || v < -32767); k++) {
        if (v > 32767)
            v = 65534 - v;
        else
            v = -65534 - v;
    }
    if (v > 32767)
        v = 32767;
    if (v < -32767)
        v = -32767;
    return v;
}

static inline void deck_ui_sel_glow(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t r2 = r * r + 1;
    int32_t dx;
    int32_t dy;

    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++) {
            int32_t d2 = dx * dx + dy * dy;

            if (d2 < r2)
                cv_pset(cx + dx, cy + dy, deck_ui_scale(c, 256 - (d2 * 256) / r2));
        }
}

static inline void deck_ui_sel_vc_points(const deck_voice_cfg_t *vc, uint32_t t, int32_t gain, int32_t cx,
                                         int32_t cy, int32_t rx, int32_t ry, int32_t crush_q, int32_t *ox,
                                         int32_t *oy)
{
    uint32_t a = 1u + ((uint32_t)vc->tune * 4u) / 127u;

    if (a > 5u)
        a = 5u;
    {
        uint32_t b = a + 1u + ((uint32_t)vc->tune * 4u) / 85u;
        uint32_t tx = t * 0x00200000u;
        uint32_t ty = t * 0x00140000u + ((uint32_t)vc->timbre << 25);
        uint32_t i;

        for (i = 0u; i < DECK_UI_SEL_VC_N; i++) {
            uint32_t n = i * DECK_UI_SEL_VC_STEP;
            int32_t xv = deck_sine(n * a + tx);
            int32_t yv = deck_sine(n * b + ty);
            int32_t px;
            int32_t py;

            xv = deck_ui_sel_vc_fold((xv * gain) >> 8);
            px = cx + (xv * rx) / 32768;
            py = cy + (yv * ry) / 32768;
            if (crush_q > 1) {
                px = cx + ((px - cx) / crush_q) * crush_q;
                py = cy + ((py - cy) / crush_q) * crush_q;
            }
            ox[i] = px;
            oy[i] = py;
        }
    }
}

/* SEL — 240×132: lane columns + Lissajous figure + thin morph scope (knob chrome is outside this blit). */
static inline void deck_ui_draw_sel(const deck_runtime_t *rt, const deck_voice_cfg_t *all_cfg,
                                    const deck_voice_cfg_t *vc, const deck_voice_t *dv, const uint8_t *pulse,
                                    uint8_t pg, uint16_t ecol)
{
    static uint32_t deck_ui_sel_nph;
    enum {
        SEL_H = 132,
        LVL_Y0 = 0,
        LVL_Y1 = 20,
        WAVE_Y0 = 24,
        WAVE_Y1 = 124,
        WAVE_X0 = 0,
        WAVE_X1 = 240,
        WAVE_W = WAVE_X1 - WAVE_X0,
        LANE_W = 10,
        LANE_STEP = 15,
        PAN_CTR = 120
    };
    uint8_t v;
    uint8_t sel = rt ? (uint8_t)(rt->cur_voice % DECK_NVOICES) : 0u;
    int32_t live = 0;
    int32_t WAVE_MID;
    uint32_t bpm_v = rt->bpm < 40u ? 40u : (rt->bpm > 180u ? 180u : rt->bpm);
    uint32_t scroll = (fm1_ms + 1u) * (40u + bpm_v / 5u);
    uint32_t speed = scroll << 18;
    uint32_t x;
    int32_t nz_mix = (int32_t)vc->noise;
    int32_t kdrv = (int32_t)vc->drive;
    int32_t crush = (int32_t)vc->crush;
    int32_t wave_half;
    int32_t hit = pulse ? (int32_t)pulse[sel] : 0;
    uint16_t lc;
    uint16_t lc_hi;
    int32_t bloom_h;
    int32_t bloom_w;
    int32_t wave_thick;
    int32_t pan_l;
    int32_t pan_r;
    int32_t crush_step;
    int32_t scope_y;
    int32_t scope_half;

    if (!rt || !all_cfg || !vc || !pulse)
        return;
    WAVE_MID = (WAVE_Y0 + WAVE_Y1) / 2;
    wave_half = (WAVE_Y1 - WAVE_Y0 - 6) / 2;
    lc = deck_hue_mix(deck_vcol(sel), ecol);
    lc_hi = deck_ui_scale(lc, 255);
    if (dv) {
        live = dv->env > dv->env2 ? dv->env : dv->env2;
        if (dv->hat_env > live)
            live = dv->hat_env;
        live = live >> 8;
    }
    if (hit > live)
        live = hit;

    bloom_h = 6 + ((int32_t)vc->filter * (wave_half - 4)) / 127;
    bloom_w = 80 + ((int32_t)vc->filter * 150) / 127;
    if (bloom_w > 236)
        bloom_w = 236;
    wave_thick = 1 + kdrv / 18;
    if (wave_thick > 7)
        wave_thick = 7;

    pan_l = 24 + ((127 - (int32_t)vc->pan) * 231) / 127;
    pan_r = 24 + ((int32_t)vc->pan * 231) / 127;
    crush_step = crush < 8 ? 0 : wave_half / (4 + ((127 - crush) * 10) / 127);
    if (crush_step < 2)
        crush_step = 2;

    cv_rect(0, 0, 240, SEL_H, deck_ui_mix(RGB(0, 2, 6), deck_ui_scale(lc, 28), 7u));

    for (v = 0u; v < DECK_NVOICES; v++) {
        int32_t bx = 2 + (int32_t)v * LANE_STEP;
        int32_t h = 4 + ((int32_t)all_cfg[v].level * (LVL_Y1 - 2)) / 127;
        uint16_t c = deck_hue_mix(deck_vcol(v), ecol);
        int on = v == sel;
        uint8_t pl = pulse[v];
        uint16_t bar = deck_ui_scale(c, on ? 220 : (rt->lane_mute[v] ? 20 : 90));

        if (h > LVL_Y1 - 2)
            h = LVL_Y1 - 2;
        if (pl > 6u)
            cv_rect(bx, LVL_Y1 - h - 3, (uint32_t)LANE_W, 3u, deck_ui_scale(c, 120 + (int32_t)pl));
        if (pg == 2u) {
            int32_t hk = 1 + ((int32_t)all_cfg[v].duck * (LVL_Y1 - 3)) / 127;
            int32_t hd = 1 + ((int32_t)all_cfg[v].dly_send * (LVL_Y1 - 3)) / 127;
            int32_t hr = 1 + ((int32_t)all_cfg[v].rev_send * (LVL_Y1 - 3)) / 127;
            uint16_t dim = deck_ui_scale(c, on ? 150 : (rt->lane_mute[v] ? 14 : 60));

            if (v != DECK_BD)
                cv_rect(bx, LVL_Y1 - hk, 3u, (uint32_t)hk, dim);
            cv_rect(bx + 3, LVL_Y1 - hd, 3u, (uint32_t)hd, bar);
            cv_rect(bx + 6, LVL_Y1 - hr, (uint32_t)(LANE_W - 6), (uint32_t)hr, dim);
        } else
            cv_rect(bx, LVL_Y1 - h, (uint32_t)LANE_W, (uint32_t)h, bar);
        if (on)
            deck_ui_frame(bx - 1, LVL_Y0, LANE_W + 2, LVL_Y1 + 1, deck_ui_scale(lc, 160 + (int32_t)pl * 3));
    }

    cv_rect(0, WAVE_Y0, (uint32_t)PAN_CTR, (uint32_t)(WAVE_Y1 - WAVE_Y0),
              deck_ui_mix(RGB(2, 2, 8), deck_ui_scale(lc, pan_l), 14u));
    cv_rect(PAN_CTR, WAVE_Y0, (uint32_t)(240 - PAN_CTR), (uint32_t)(WAVE_Y1 - WAVE_Y0),
              deck_ui_mix(RGB(1, 1, 4), deck_ui_scale(lc, pan_r), 14u));
    cv_line(PAN_CTR, WAVE_Y0, PAN_CTR, WAVE_Y1 - 1, deck_ui_scale(lc, 80));

    cv_rect((240 - bloom_w) / 2, WAVE_MID - bloom_h, (uint32_t)bloom_w, (uint32_t)(bloom_h * 2),
              deck_ui_scale(lc, 20 + ((int32_t)vc->filter * 90) / 127 + (live >> 2)));

    deck_ui_sel_nph += scroll * (3u + (uint32_t)nz_mix / 8u);

    {
        int32_t liss_half = wave_half - 10;
        int32_t size = (int32_t)vc->level + (live >> 1) + (hit >> 1);
        int32_t gain = 256 + kdrv * 6;
        int32_t rx;
        int32_t ry;
        int32_t px[DECK_UI_SEL_VC_N];
        int32_t py[DECK_UI_SEL_VC_N];
        int32_t tr;
        int32_t i;
        int32_t k;
        int32_t crush_q = crush < 8 ? 1 : 2 + ((127 - crush) * 14) / 127;
        uint32_t t_anim = fm1_ms + (deck_ui_sel_nph >> 10);

        scope_y = WAVE_Y1 - 5;
        scope_half = 6 + ((int32_t)vc->level * 8) / 127;

        if (size > 127)
            size = 127;
        if (liss_half < 12)
            liss_half = 12;
        rx = 18 + (size * (WAVE_W / 2 - 14)) / 127;
        ry = 12 + (size * liss_half) / 127;

        for (k = 0; k < 16; k++) {
            uint32_t h = (uint32_t)k * 2654435761u + (t_anim >> 6) * 40503u;

            h ^= h >> 15;
            h *= 0x2c1b3c6du;
            h ^= h >> 12;
            if ((int32_t)(h & 127u) < live + (hit >> 1)) {
                int32_t dx = (int32_t)((h >> 8) % (uint32_t)WAVE_W);
                int32_t dy = WAVE_Y0 + 2 + (int32_t)((h >> 16) % (uint32_t)(WAVE_Y1 - WAVE_Y0 - 12));

                cv_pset(WAVE_X0 + dx, dy, deck_ui_scale(lc, 36 + live / 2));
            }
        }

        for (tr = DECK_UI_SEL_VC_TRAILS - 1; tr >= 0; tr--) {
            uint32_t tt = t_anim - (uint32_t)tr * 60u;
            uint16_t trail_c = tr == 0 ?
                                   deck_ui_scale(lc_hi, 140 + ((int32_t)vc->filter * 70) / 127 + live / 3) :
                                   deck_ui_scale(deck_hue_mix(lc, DECK_HOME_CYAN), 90 - tr * 22);

            deck_ui_sel_vc_points(vc, tt, gain, PAN_CTR, WAVE_MID, rx, ry, crush_q, px, py);
            for (i = 0; i < (int32_t)DECK_UI_SEL_VC_N; i++) {
                int32_t j = (i + 1) % (int32_t)DECK_UI_SEL_VC_N;
                int32_t mx = (px[i] + px[j]) / 2;
                int32_t side = mx < PAN_CTR ? pan_l : pan_r;
                uint16_t seg = deck_ui_mix(trail_c, deck_ui_scale(lc_hi, 70 + side / 4), tr == 0 ? 3u : 8u);

                if (kdrv > 40 && tr == 0)
                    seg = deck_ui_mix(seg, lc_hi, (uint32_t)(kdrv / 20));
                cv_line(px[i], py[i], px[j], py[j], seg);
            }
        }

        deck_ui_sel_glow(px[0], py[0], 2 + live / 22, deck_ui_scale(lc_hi, 220 + live));
        if (hit > 8 || live > 24)
            deck_ui_sel_glow(px[0], py[0], 4 + (int32_t)((t_anim / 14u) % 10u), deck_ui_scale(DECK_HOME_CYAN, 140 + hit));

        if (nz_mix > 0) {
            for (x = 0u; x < (uint32_t)WAVE_W; x += 4u) {
                if (((x + (deck_ui_sel_nph >> 3)) & 7u) == 0u) {
                    int32_t nz_j = deck_noise(&deck_ui_sel_nph) >> 12;

                    cv_pset(WAVE_X0 + (int32_t)x, WAVE_MID + ((nz_j * nz_mix) >> 8),
                            deck_ui_scale(lc, 40 + nz_mix / 2));
                }
            }
        }
    }

    {
        int32_t scope_py = -1;
        const uint32_t scope_step = 2u;

        /* Thin bottom morph — step 2 keeps CPU under the old full-fill wave. */
        for (x = 0u; x < (uint32_t)WAVE_W; x += scope_step) {
            int32_t xp = WAVE_X0 + (int32_t)x;
            uint32_t xrate = 12u + ((uint32_t)vc->tune * 46u);
            uint32_t ph = (x * xrate << 16) + speed + (deck_ui_sel_nph << 12);
            int32_t w = deck_ui_sel_morph_sample(ph, vc->filter, vc->drive, vc->timbre);
            int32_t dy;
            int32_t y;
            int32_t side = xp < PAN_CTR ? pan_l : pan_r;
            uint16_t col;
            int32_t t;

            dy = (w * scope_half) >> 14;
            if (crush_step > 0)
                dy = (dy / crush_step) * crush_step;
            if (dy > scope_half)
                dy = scope_half;
            else if (dy < -scope_half)
                dy = -scope_half;
            y = scope_y - dy;
            if (y < WAVE_Y0 + 1)
                y = WAVE_Y0 + 1;
            if (y > WAVE_Y1 - 1)
                y = WAVE_Y1 - 1;

            col = deck_ui_scale(lc, 55 + side / 3 + ((int32_t)vc->filter * 40) / 127);
            if (kdrv > 48)
                col = deck_ui_mix(col, lc_hi, (uint32_t)(kdrv / 22));
            if (scope_py >= 0) {
                int32_t thick = wave_thick > 3 ? 3 : wave_thick;

                for (t = 0; t < thick; t++) {
                    int32_t off = t - thick / 2;

                    cv_line(xp - (int32_t)scope_step, scope_py + off, xp, y + off, col);
                }
            }
            scope_py = y;
        }
    }
}

static inline void deck_ui_draw_groove(const deck_runtime_t *rt, uint8_t pg, uint8_t accent, uint8_t human,
                                      uint8_t midi_ch, uint8_t clock)
{
    fm_groove_plot_t gp = {0};
    uint8_t step = 0u;
    uint32_t frac256 = 0u;

    if (!rt)
        return;
    if (rt->playing) {
        step = (uint8_t)((rt->master_step ? rt->master_step - 1u : 0u) % 16u);
        if (rt->step_phase > 0u) {
            uint32_t base = deck_seq_s16_base_samples(rt->bpm);
            uint32_t need = deck_seq_s16_len_samples(base, rt->swing, (uint8_t)(step & 1u));

            if (need > 0u)
                frac256 = (rt->step_phase * 256u) / need;
            if (frac256 > 255u)
                frac256 = 255u;
        }
    } else
        step = (uint8_t)(rt->master_step % 16u);
    gp.page = pg == 0u ? 0u : 1u; /* GLO p2 (HUMAN/HUMN T) uses the accent/human plot page */
    gp.bpm = rt->bpm;
    gp.swing = rt->swing;
    gp.accent = accent;
    gp.human = human;
    gp.no_ring = 1u;
    gp.playing = rt->playing;
    gp.step = step;
    gp.step_frac256 = frac256;
    gp.clock_mode = clock;
    gp.clock_n = 2u;
    gp.midi_ch = midi_ch;
    gp.col = DECK_HOME_CYAN;
    fm_groove_plot_draw(&gp);
}

static inline void deck_ui_draw_bank_quiet(uint8_t bank_ix, const char *const *bank_names, uint8_t bank_n)
{
    const char *lbl = "···";

    if (bank_names && bank_ix < bank_n)
        lbl = bank_names[bank_ix];
    cv_rect(0, 0, 240, 112, RGB(4, 10, 28));
    cv_text(96, 50, &FONT_S, lbl, C_DIM);
}

#endif /* DECK_UI_H */
