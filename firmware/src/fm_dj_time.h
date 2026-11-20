/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ echo TIME: discrete note-division anchors only (host-testable). */
#ifndef FM_DJ_TIME_H
#define FM_DJ_TIME_H

#include "fm_dj.h"
#include "fm_lfo.h"
#include <stdint.h>

#ifndef DUB_STEPS
#define DUB_STEPS 16u
#endif

static inline int fm_dj_time_div_nearest(uint8_t cc, uint32_t bpm)
{
    int ix = fm_time_lock_idx(cc, bpm);
    uint32_t i;
    uint32_t best_d = 99u;
    int best = 0;

    if (cc == 0u)
        return -1;
    if (ix >= 0)
        return ix;
    for (i = 0u; i < FM_TIME_NDIVS; i++) {
        uint8_t a = fm_time_anchor_cc(i, bpm);
        uint32_t d = cc > a ? cc - a : a - cc;

        if (d < best_d) {
            best_d = d;
            best = (int)i;
        }
    }
    return best;
}

static inline uint8_t fm_dj_time_anchor_div(uint32_t div_ix, uint32_t bpm)
{
    if (div_ix >= FM_TIME_NDIVS)
        div_ix = FM_TIME_NDIVS - 1u;
    return fm_time_anchor_cc(div_ix, bpm);
}

static inline uint8_t fm_dj_time_quantize_cc(uint8_t cc, uint32_t bpm)
{
    int ix;

    if (cc == 0u)
        return 0u;
    ix = fm_dj_time_div_nearest(cc, bpm);
    if (ix < 0)
        return cc;
    return fm_dj_time_anchor_div((uint32_t)ix, bpm);
}

static inline int fm_dj_time_is_locked_cc(uint8_t cc, uint32_t bpm)
{
    return fm_time_lock_idx(cc, bpm) >= 0;
}

static inline uint8_t fm_dj_time_step_div(uint8_t cc, uint32_t bpm, int32_t steps)
{
    int ix = fm_dj_time_div_nearest(cc, bpm);
    int ni;

    if (cc == 0u && steps > 0)
        return fm_dj_time_anchor_div(0u, bpm);
    if (ix < 0)
        ix = 0;
    ni = ix + (int)steps;
    if (ni < 0)
        ni = 0;
    if (ni >= (int)FM_TIME_NDIVS)
        ni = (int)FM_TIME_NDIVS - 1;
    return fm_dj_time_anchor_div((uint32_t)ni, bpm);
}

static inline uint8_t fm_dj_time_climb_div(uint8_t base_cc, uint32_t bpm, int32_t climb)
{
    int extra = (int)((climb * 4) / 127);

    if (extra < 1 && climb > 0)
        extra = 1;
    return fm_dj_time_step_div(base_cc, bpm, extra);
}

static inline int fm_dj_build_moves_echo_time(const fm_dj_t *dj)
{
    if (!dj->on)
        return 0;
    if (dj->build_arc != FM_DJ_BUILD_FB_PONG && dj->build_arc != FM_DJ_BUILD_DUB_ECHO)
        return 0;
    return dj->phrase_bar >= dj->build_start && dj->phrase_bar <= dj->build_end;
}

#endif /* FM_DJ_TIME_H */
