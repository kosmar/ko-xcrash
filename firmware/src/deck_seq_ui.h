/* SPDX-License-Identifier: GPL-3.0-only */
/* House/Dub-style SEQ paging + knob-page cycle for KO DECK. GATE/ACCENT/SLIDE/RATCHET live only in the
 * step overlay (white long-press, bank 1 = P2); page 0 is not in the tap cycle. */
#ifndef DECK_SEQ_UI_H
#define DECK_SEQ_UI_H

#include "fm_seq.h"

#define DECK_SEQ_KNOB_PAGE_CYCLE_N 4u
/* Tap cycle = display order: dot i is UI page i+1. Values are internal seq_page indices (1, 2, 3, 4). */
static const uint8_t DECK_SEQ_KNOB_PAGE_CYCLE[DECK_SEQ_KNOB_PAGE_CYCLE_N] = {1u, 2u, 3u, 4u};

static inline int deck_seq_page_in_cycle(uint8_t p)
{
    uint32_t i;

    for (i = 0u; i < DECK_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (DECK_SEQ_KNOB_PAGE_CYCLE[i] == p)
            return 1;
    }
    return 0;
}

static inline uint8_t deck_seq_page_cycle_next(uint8_t cur)
{
    uint32_t i;

    for (i = 0u; i < DECK_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (DECK_SEQ_KNOB_PAGE_CYCLE[i] == cur)
            return DECK_SEQ_KNOB_PAGE_CYCLE[(i + 1u) % DECK_SEQ_KNOB_PAGE_CYCLE_N];
    }
    return DECK_SEQ_KNOB_PAGE_CYCLE[0];
}

static inline void deck_seq_draw_page_cycle_dots(int32_t x0, int32_t y, uint8_t seq_page)
{
    uint32_t i;
    uint8_t ix = 0u;

    for (i = 0u; i < DECK_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (DECK_SEQ_KNOB_PAGE_CYCLE[i] == seq_page)
            ix = (uint8_t)i;
    }
    fm_seq_draw_page_dots(x0, y, ix, DECK_SEQ_KNOB_PAGE_CYCLE_N);
}

static const char *const DECK_SEQ_KNOB_GATE[4] = {"GATE", "ACCENT", "SLIDE", "RATCHET"};
/* UI page 3 (seq_page 3): lane length + mute/solo + lane clk ratio (PACE, ln->div) + lane nudge. */
static const char *const DECK_SEQ_KNOB_P3[4] = {"LENGTH", "STATE", "PACE", "NUDGE"};
/* UI page 2 (seq_page 2): PAGE (OCT view page / top bar) + COMBO (ensemble) + INVERT. */
static const char *const DECK_SEQ_KNOB_P2[4] = {"PAGE", "COMBO", "INVERT", ""};
/* UI page 4 (seq_page 4): lane select + selected lane seq pattern (PATTRN = mode + EUC n in one knob, ROTATE, VARY), as GENRE page 0. */
static const char *const DECK_SEQ_KNOB_P4[4] = {"LANE", "PATTRN", "ROTATE", "VARY"};

static inline const char *deck_seq_pot_label(uint8_t seq_page, int plock_held, uint8_t plock_bank, uint8_t slot)
{
    slot &= 3u;
    if (plock_held) {
        if (plock_bank == 1u)
            return DECK_SEQ_KNOB_GATE[slot];
        return FM_SEQ_KNOB_PLOCK[slot];
    }
    if (seq_page == 0u)
        return DECK_SEQ_KNOB_GATE[slot];
    if (seq_page == 1u)
        return FM_SEQ_KNOB_P1[slot];
    if (seq_page == 3u)
        return DECK_SEQ_KNOB_P3[slot];
    if (seq_page == 2u)
        return DECK_SEQ_KNOB_P2[slot];
    if (seq_page == 4u)
        return DECK_SEQ_KNOB_P4[slot];
    return fm_seq_knob_label(seq_page, 0, slot);
}

#endif /* DECK_SEQ_UI_H */
