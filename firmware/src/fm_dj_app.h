/* SPDX-License-Identifier: GPL-3.0-only */
/* App glue for DJ mode (FX longpress, phrase step). Include from fm_app.c only. */
#ifndef FM_DJ_APP_H
#define FM_DJ_APP_H

#include "fm_dj.h"
#include <stdint.h>

#ifndef FM_DJ_FX_LONG_MS
#define FM_DJ_FX_LONG_MS 500u
#endif

typedef struct {
    uint32_t *press_ms;
    uint8_t *long_used;
    uint8_t *long_cancel;
    fm_dj_t *dj;
    uint32_t *ui_dirty;
} fm_dj_app_fx_t;

typedef void (*fm_dj_app_trans_fn)(fm_dj_t *dj, uint8_t trans, uint8_t rng4, uint8_t *kick_slam_bars);

/* FX hold: poll each app_step while BTN_FX is down (after platform_poll). */
static inline void fm_dj_app_fx_long_press_poll(fm_dj_app_fx_t *fx, uint32_t b, uint32_t rising,
                                                uint32_t buttons, uint32_t btn_fx_mask,
                                                uint32_t now_ms, uint32_t dj_seed)
{
    if (!(buttons & btn_fx_mask))
        return;
    if (!fx->press_ms || !*fx->press_ms || fx->long_used[0] || fx->long_cancel[0])
        return;
    if (buttons & ~btn_fx_mask)
        fx->long_cancel[0] = 1u;
    if (b & ~btn_fx_mask)
        fx->long_cancel[0] = 1u;
    if (rising)
        fx->long_cancel[0] = 1u;
    if (fx->long_cancel[0])
        return;
    if (now_ms - *fx->press_ms >= FM_DJ_FX_LONG_MS) {
        fm_dj_toggle(fx->dj, dj_seed);
        fx->long_used[0] = 1u;
        *fx->ui_dirty = 1u;
    }
}

static inline void fm_dj_app_fx_on_press(fm_dj_app_fx_t *fx, uint32_t now_ms)
{
    *fx->press_ms = now_ms;
    fx->long_cancel[0] = 0u;
    fx->long_used[0] = 0u;
}

static inline void fm_dj_app_fx_on_release(fm_dj_app_fx_t *fx, uint32_t now_ms, void (*bank_fx)(void))
{
    if (*fx->press_ms && !fx->long_used[0] && !fx->long_cancel[0] &&
        (now_ms - *fx->press_ms) < FM_DJ_FX_LONG_MS)
        bank_fx();
    *fx->press_ms = 0u;
    fx->long_used[0] = 0u;
    fx->long_cancel[0] = 0u;
}

static inline void fm_dj_app_fx_cancel_hold(fm_dj_app_fx_t *fx)
{
    if (*fx->press_ms && !fx->long_used[0] && !fx->long_cancel[0])
        fx->long_cancel[0] = 1u;
}

/* Phrase tick + bar arm; transition invokes app callback (DROP / CROSSFADE / CUEING). */
static inline void fm_dj_app_phrase_step(fm_dj_t *dj, uint8_t bar_step, uint8_t steps_per_bar,
                                         uint8_t fill_blocked, uint32_t *ui_dirty,
                                         fm_dj_app_trans_fn on_trans, uint8_t *kick_slam_bars)
{
    if (dj->on || dj->slew_off)
        fm_dj_tick(dj, bar_step, steps_per_bar);
    if (dj->bar_pending) {
        uint8_t prev_role = dj->role;
        uint8_t prev_tgt = dj->target;

        dj->bar_pending = 0u;
        fm_dj_arm_bar(dj, fill_blocked);
        if (dj->role != prev_role || dj->target != prev_tgt)
            *ui_dirty = 1u;
    }
    if (dj->trans_pending != 0u && on_trans) {
        uint8_t trans = (uint8_t)(dj->trans_pending - 1u);
        uint8_t r = (uint8_t)(fm_dj_rng_u8(dj) & 3u);

        dj->trans_pending = 0u;
        on_trans(dj, trans, r, kick_slam_bars);
        if ((trans == FM_DJ_TRANS_DROP || trans == FM_DJ_TRANS_FILTER_SLAM) && kick_slam_bars &&
            *kick_slam_bars > 0u)
            dj->last_drop_span = *kick_slam_bars > FM_DJ_DROP_EMPH_LAYOUT_MAX ? FM_DJ_DROP_EMPH_LAYOUT_MAX
                                                                              : *kick_slam_bars;
    }
}

#define FM_DJ_APP_WIRED 1

#endif /* FM_DJ_APP_H */
