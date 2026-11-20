/* SPDX-License-Identifier: GPL-3.0-only */
/* KO bar stack: 8 rows × 16 steps = one line in time (bars 1…8), drawn from precomputed cells.
 * App-agnostic: the caller fills ko_bar_stack_t from its own sequencer state.
 * Needs from the including TU (gfx.c): cv_rect, cv_line, cv_text, FONT_S, C_WHITE, C_BLACK, C_GRAY, C_DIM, RGB.
 * Needs fm_seq.h: fm_seq_cell_t, FM_SEQ_PAGE_STEPS, FM_SEQ_MICRO_HALF. */
#ifndef KO_BAR_STACK_H
#define KO_BAR_STACK_H

#include <stdint.h>
#include "fm_seq.h"

/* Per-cell step marks: bits 0-1 slide level (0..slide_full), bits 2-4 ratchet count (0 = none). */
#define KO_BAR_MARK_SLIDE_MASK 3u
#define KO_BAR_MARK_RAT_SHIFT 2u
#define KO_BAR_ROWS 8u

typedef struct {
    const fm_seq_cell_t *cells;      /* KO_BAR_ROWS * FM_SEQ_PAGE_STEPS, row-major; NULL = draw nothing */
    const uint8_t *marks;            /* same layout, KO_BAR_MARK_* bits; NULL = none */
    const char *const *bar_lbls;     /* KO_BAR_ROWS labels; NULL = none */
    const uint16_t *bar_lbl_cols;    /* KO_BAR_ROWS label colours, C_DIM = muted bar; NULL = C_GRAY */
    const uint8_t *play_step_by_bar; /* KO_BAR_ROWS play columns, >= FM_SEQ_PAGE_STEPS = none; NULL = none */
    uint16_t play_col;               /* playhead bottom edge */
    uint16_t acc;                    /* cursor cell backing + tune line on white cells */
    uint8_t sel_bar;                 /* highlighted row */
    uint8_t cursor_st;               /* cursor column in sel_bar, >= FM_SEQ_PAGE_STEPS = none */
    uint8_t cursor_latched;          /* white frame around the cursor cell */
    uint8_t playing;
    uint8_t slide_full;              /* slide mark level drawn full width (0 = 3) */
} ko_bar_stack_t;

static void ko_bar_stack_draw(const ko_bar_stack_t *bs)
{
    const int32_t y0 = 2;
    const int32_t row_h = 16;
    const int32_t cell_h = 11;
    const fm_seq_cell_t *cells = bs->cells;
    const uint8_t *marks = bs->marks;
    const char *const *bar_lbls = bs->bar_lbls;
    const uint16_t *bar_lbl_cols = bs->bar_lbl_cols;
    uint8_t plock_st = bs->cursor_st;
    uint32_t slide_full = bs->slide_full ? bs->slide_full : 3u;
    uint32_t row;
    uint32_t st;

    if (!cells)
        return;
    for (row = 0u; row < KO_BAR_ROWS; row++) {
        int32_t y = y0 + (int32_t)row * row_h;
        uint16_t lcol = bar_lbl_cols ? bar_lbl_cols[row] : C_GRAY;
        int muted = lcol == C_DIM;
        int selected = row == (uint32_t)bs->sel_bar;
        uint16_t edge_col = muted ? C_GRAY : C_WHITE;

        if (selected) {
            cv_rect(0, y, 240, cell_h + 4, RGB(18, 22, 42));
            cv_line(0, y, 239, y, edge_col);
            cv_line(0, y + cell_h + 3, 239, y + cell_h + 3, edge_col);
        }
        if (bar_lbls && bar_lbls[row]) {
            if (selected) {
                if (!muted) {
                    cv_rect(1, y + 1, 14, cell_h + 2, C_WHITE);
                    cv_text(4, y + 1, &FONT_S, bar_lbls[row], C_BLACK);
                } else {
                    cv_rect(1, y + 1, 14, cell_h + 2, C_WHITE);
                    cv_rect(3, y + 3, 10, cell_h - 2, C_BLACK);
                    cv_text(4, y + 1, &FONT_S, bar_lbls[row], C_WHITE);
                }
            } else if (!muted) {
                cv_rect(1, y + 1, 14, cell_h + 2, lcol);
                cv_text(4, y + 1, &FONT_S, bar_lbls[row], C_BLACK);
            } else {
                cv_text(4, y + 1, &FONT_S, bar_lbls[row], C_DIM);
                cv_line(2, y + 2 + cell_h / 2, 13, y + 2 + cell_h / 2, C_DIM);
            }
        }
        for (st = 0u; st < FM_SEQ_PAGE_STEPS; st++) {
            int32_t x = 20 + (int32_t)st * 13 + (int32_t)(st / 4u) * 3;
            uint32_t ix = row * FM_SEQ_PAGE_STEPS + st;
            const fm_seq_cell_t *c = &cells[ix];
            int32_t dx;
            int32_t bw;
            int32_t bx;

            if (plock_st < FM_SEQ_PAGE_STEPS && selected && st == (uint32_t)plock_st)
                cv_rect(x - 1, y + 1, 13, cell_h + 2, bs->acc);
            cv_rect(x, y + 2, 11, cell_h, c->bg_col);
            if (c->vel > 0u && c->col != 0u) {
                int32_t bh = 3 + ((int32_t)c->vel * (cell_h - 3)) / 127;
                int32_t dy = 0;
                int32_t by;
                int32_t cl_tune = c->tune;
                int32_t yp;
                uint16_t pline;

                dx = ((int32_t)c->micro * 4) / FM_SEQ_MICRO_HALF;
                bx = x + (dx == 0 ? 0 : (1 + dx));
                if (c->gate < 0) {
                    bw = 11 + ((int32_t)c->gate * 8) / 63;
                    if (bw < 3)
                        bw = 3;
                } else if (c->gate > 0) {
                    bw = 11 + ((int32_t)c->gate * 2) / 63;
                    if (bw > 13)
                        bw = 13;
                } else
                    bw = (dx == 0) ? 11 : 9;
                if (c->tune != 0) {
                    dy = ((int32_t)c->tune * (cell_h - 3)) / 24;
                    if (dy < -(cell_h - 3))
                        dy = -(cell_h - 3);
                    if (dy > cell_h - 3)
                        dy = cell_h - 3;
                }
                if (cl_tune < -24)
                    cl_tune = -24;
                if (cl_tune > 24)
                    cl_tune = 24;
                by = y + 2 + cell_h - bh - dy;
                if (by < y + 2)
                    by = y + 2;
                cv_rect(bx, by, bw, bh, c->col);
                if (c->cond > 0u) {
                    if (bh >= 6)
                        cv_rect(bx + 2, by + 2, bw - 4, bh - 4, c->bg_col);
                    else if (bh >= 4)
                        cv_rect(bx + 3, by + 1, bw - 6, bh - 2, c->bg_col);
                }
                if (c->tune != 0) {
                    yp = y + 2 + ((24 - cl_tune) * (cell_h - 2)) / 48;
                    pline = (c->col == C_WHITE) ? bs->acc : C_WHITE;
                    cv_rect(bx, yp, bw, 2, pline);
                }
                if (marks && marks[ix]) {
                    uint32_t sl = marks[ix] & KO_BAR_MARK_SLIDE_MASK;
                    uint32_t rt = (uint32_t)marks[ix] >> KO_BAR_MARK_RAT_SHIFT;
                    uint32_t k;

                    pline = (c->col == C_WHITE) ? bs->acc : C_WHITE;
                    if (sl)
                        cv_rect(x, y + 2, 3 + (int32_t)((sl * 8u) / slide_full), 1, pline);
                    for (k = 0u; k < rt; k++)
                        cv_rect(x + 1 + (int32_t)k * 3, y + 2, 1, 2, pline);
                }
            }
            if (bs->cursor_latched && selected && plock_st < FM_SEQ_PAGE_STEPS && st == (uint32_t)plock_st) {
                int32_t fx = x - 2;
                int32_t fy = y;
                int32_t fw = 15;
                int32_t fh = cell_h + 4;

                cv_rect(fx, fy, fw, 2, C_WHITE);
                cv_rect(fx, fy + fh - 2, fw, 2, C_WHITE);
                cv_rect(fx, fy, 2, fh, C_WHITE);
                cv_rect(fx + fw - 2, fy, 2, fh, C_WHITE);
            }
        }
        if (bs->playing && bs->play_step_by_bar && bs->play_step_by_bar[row] < FM_SEQ_PAGE_STEPS) {
            int32_t pst = (int32_t)bs->play_step_by_bar[row];
            int32_t px = 20 + pst * 13 + (pst / 4) * 3 - 1;
            int32_t py = y + 1;
            int32_t pw = 13;
            int32_t ph = cell_h + 2;

            cv_rect(px, py, pw, 2, C_WHITE);
            cv_rect(px, py + ph - 2, pw, 2, bs->play_col);
            cv_rect(px, py, 2, ph, C_WHITE);
            cv_rect(px + pw - 2, py, 2, ph, C_WHITE);
        }
    }
}

#endif
