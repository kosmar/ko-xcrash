/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ additive SEQ pg4 ensemble / invert apply helpers (fm_app.c). */
#ifndef FM_DJ_SEQ_H
#define FM_DJ_SEQ_H

#include "fm_dj.h"
#include "fm_seq.h"
#include <stdint.h>

/* Toast + highlight: same short labels as SEQ page-4 knobs (genre-specific). */
static inline const char *fm_dj_seq_ens_toast_label(void)
{
    return FM_SEQ_KNOB_P4[2];
}

static inline const char *fm_dj_seq_flip_toast_label(void)
{
    return FM_SEQ_KNOB_P4[3];
}

static inline uint8_t fm_dj_seq_ens_blocked(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                            uint8_t steps_per_bar)
{
    if (!dj->on && !dj->slew_off)
        return 1u;
    if (playing && fm_dj_pre_drop_silence(dj, 1u, bar_step, steps_per_bar))
        return 1u;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_DROP))
        return 1u;
    if (fm_dj_is_break_bar(dj) && playing)
        return 1u;
    return 0u;
}

static inline uint8_t fm_dj_seq_flip_blocked(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                             uint8_t steps_per_bar)
{
    return fm_dj_seq_ens_blocked(dj, playing, bar_step, steps_per_bar);
}

static inline uint8_t fm_dj_seq_flip_want(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                          uint8_t steps_per_bar, int32_t fd)
{
    if (fm_dj_seq_flip_blocked(dj, playing, bar_step, steps_per_bar))
        return 0u;
    if (fd > 16)
        return 1u;
    return 0u;
}

static inline uint8_t fm_dj_seq_ens_live(int32_t ed)
{
    return ed > 16 ? 1u : 0u;
}

#define FM_DJ_SEQ_ENS_OV_SEG_A 0u
#define FM_DJ_SEQ_ENS_OV_SEG_B 1u
#define FM_DJ_SEQ_ENS_OV_SEG_RES 2u

typedef struct {
    uint8_t active;
    uint8_t base_ens;
    uint8_t base_mask;
    uint8_t combo_a;
    uint8_t combo_b;
    uint8_t combo_cur;
    uint8_t seg;
    uint8_t seg_bars;
    uint8_t last_combo;
    uint8_t toast_ens;
    uint8_t invert_accent;
    uint8_t invert_left;
    uint8_t plan_b;
    uint8_t bar_seen;
    uint8_t cycle_done;
} fm_dj_seq_ens_ov_t;

static inline void fm_dj_seq_ens_ov_reset(fm_dj_seq_ens_ov_t *ov)
{
    ov->active = 0u;
    ov->invert_accent = 0u;
    ov->invert_left = 0u;
    ov->bar_seen = 0xffu;
    ov->cycle_done = 0u;
}

static inline uint8_t fm_dj_seq_ens_is_combo(uint8_t ens)
{
    return ens > FM_SEQ_ENS_MUTE && ens < FM_SEQ_ENS_ALL;
}

static inline uint8_t fm_dj_seq_mask_low6(uint8_t m)
{
    return (uint8_t)(m & 0x3fu);
}

static inline uint8_t fm_dj_seq_mask_all_muted(uint8_t m)
{
    return fm_dj_seq_mask_low6(m) == 0x3fu;
}

static inline uint8_t fm_dj_seq_mask_all_unmuted(uint8_t m)
{
    return fm_dj_seq_mask_low6(m) == 0u;
}

/* DJ intervention: at least one lane on, not full tutti mask. */
static inline uint8_t fm_dj_seq_mask_dj_intervention_ok(uint8_t m)
{
    m = fm_dj_seq_mask_low6(m);
    return !fm_dj_seq_mask_all_muted(m) && !fm_dj_seq_mask_all_unmuted(m);
}

static inline uint8_t fm_dj_seq_ens_mask_ok(const uint8_t *ens_mute, uint8_t ens)
{
    if (!ens_mute || ens >= FM_SEQ_NENSEMBLES)
        return 0u;
    return fm_dj_seq_mask_dj_intervention_ok(ens_mute[ens]);
}

static inline uint8_t fm_dj_seq_ens_combo_mask_ok(const uint8_t *ens_mute, uint8_t ens)
{
    return fm_dj_seq_ens_is_combo(ens) && fm_dj_seq_ens_mask_ok(ens_mute, ens);
}

static inline uint8_t fm_dj_seq_invert_mask(uint8_t m)
{
    return (uint8_t)((~fm_dj_seq_mask_low6(m)) & 0x3fu);
}

static inline uint8_t fm_dj_seq_invert_mask_dj_safe(uint8_t m)
{
    uint8_t inv = fm_dj_seq_invert_mask(m);

    if (fm_dj_seq_mask_dj_intervention_ok(inv))
        return inv;
    return fm_dj_seq_mask_low6(m);
}

static inline uint8_t fm_dj_seq_ens_safe_combo(const uint8_t *ens_mute)
{
    uint8_t e;

    if (!ens_mute)
        return FM_SEQ_ENS_DRUMS;
    for (e = FM_SEQ_ENS_DRUMS; e < FM_SEQ_ENS_ALL; e++) {
        if (fm_dj_seq_ens_combo_mask_ok(ens_mute, e))
            return e;
    }
    return FM_SEQ_ENS_DRUMS;
}

static inline uint8_t fm_dj_seq_ens_sanitize_combo(const uint8_t *ens_mute, uint8_t ens)
{
    if (fm_dj_seq_ens_combo_mask_ok(ens_mute, ens))
        return ens;
    return fm_dj_seq_ens_safe_combo(ens_mute);
}

static inline uint8_t fm_dj_seq_ens_pick_combo(fm_dj_t *dj, const uint8_t *ens_mute, uint8_t phrase_bar,
                                               uint8_t avoid_a, uint8_t avoid_b)
{
    uint8_t n = (uint8_t)(FM_SEQ_NENSEMBLES - 2u);
    uint8_t t = 0u;
    uint8_t ix;

    if (n == 0u)
        return fm_dj_seq_ens_safe_combo(ens_mute);
    while (t < 16u) {
        ix = (uint8_t)(1u + (fm_dj_rng_u8(dj) % n));
        if (ix != avoid_a && ix != avoid_b && fm_dj_seq_ens_combo_mask_ok(ens_mute, ix))
            return ix;
        t++;
    }
    for (ix = 1u; ix < FM_SEQ_ENS_ALL; ix++) {
        if (ix != avoid_a && ix != avoid_b && fm_dj_seq_ens_combo_mask_ok(ens_mute, ix))
            return ix;
    }
    ix = (uint8_t)(1u + ((phrase_bar * 5u + (uint8_t)(dj->rng & 7u)) % n));
    if (ix == avoid_a || ix == avoid_b)
        ix = (uint8_t)(1u + ((ix + 3u) % n));
    return fm_dj_seq_ens_sanitize_combo(ens_mute, ix);
}

static inline void fm_dj_seq_ens_ov_plan(fm_dj_t *dj, fm_dj_seq_ens_ov_t *ov, uint8_t phrase_bar,
                                         const uint8_t *ens_mute)
{
    uint8_t r = (uint8_t)fm_dj_rng_u8(dj);

    ov->combo_a = fm_dj_seq_ens_pick_combo(dj, ens_mute, phrase_bar, ov->last_combo, 0xffu);
    ov->last_combo = ov->combo_a;
    ov->plan_b = (r & 3u) == 0u ? 0u : 1u;
    if (ov->plan_b) {
        ov->combo_b = fm_dj_seq_ens_pick_combo(dj, ens_mute, phrase_bar, ov->combo_a, ov->last_combo);
        if (ov->combo_b == ov->combo_a)
            ov->plan_b = 0u;
    }
    ov->seg = FM_DJ_SEQ_ENS_OV_SEG_A;
    ov->seg_bars = (r & 1u) ? 2u : 1u;
    ov->combo_cur = ov->combo_a;
    ov->toast_ens = ov->combo_cur;
}

static inline uint8_t fm_dj_seq_ens_ov_resolve_ens(const fm_dj_seq_ens_ov_t *ov)
{
    if (ov->base_ens == FM_SEQ_ENS_CUSTOM)
        return FM_SEQ_ENS_CUSTOM;
    if (ov->base_ens == FM_SEQ_ENS_ALL)
        return FM_SEQ_ENS_ALL;
    return ov->base_ens;
}

static inline uint8_t fm_dj_seq_ens_index_from_bar(uint8_t bar)
{
    uint32_t ix = ((uint32_t)bar * (FM_SEQ_NENSEMBLES - 1u) + 63u) / 127u;

    if (ix >= FM_SEQ_NENSEMBLES)
        ix = FM_SEQ_NENSEMBLES - 1u;
    return (uint8_t)ix;
}

/* Legacy helpers (host tests / non-DJ paths). */
static inline uint8_t fm_dj_seq_blend_mute_toward(uint8_t cur_mask, const uint8_t *ens_mute, uint8_t tgt_ens,
                                                  int32_t ed)
{
    uint8_t tgt_m;
    uint8_t i;
    uint8_t out = cur_mask;

    if (ed == 0 || !ens_mute)
        return cur_mask;
    if (tgt_ens >= FM_SEQ_NENSEMBLES)
        tgt_ens = FM_SEQ_ENS_ALL;
    tgt_m = ens_mute[tgt_ens];
    for (i = 0u; i < 6u; i++) {
        uint8_t bit = (uint8_t)(1u << i);
        int32_t bi = (cur_mask & bit) ? 127 : 0;
        int32_t ti = (tgt_m & bit) ? 127 : 0;
        int32_t ri = bi + (ed * (ti - bi)) / 127;

        if (ri >= 64)
            out |= bit;
        else
            out &= (uint8_t)~bit;
    }
    return out;
}

static inline uint8_t fm_dj_seq_blend_mute_mask(uint8_t cur_mask, const uint8_t *ens_mute, uint8_t base_ens,
                                                int32_t ed)
{
    uint8_t tgt_m;
    uint8_t i;
    uint8_t out = cur_mask;

    if (ed == 0 || !ens_mute)
        return cur_mask;
    if (base_ens >= FM_SEQ_NENSEMBLES)
        base_ens = FM_SEQ_ENS_ALL;
    tgt_m = ed > 0 ? ens_mute[FM_SEQ_ENS_ALL] : ens_mute[FM_SEQ_ENS_TOPS];
    for (i = 0u; i < 6u; i++) {
        uint8_t bit = (uint8_t)(1u << i);
        int32_t bi = (cur_mask & bit) ? 127 : 0;
        int32_t ti = (tgt_m & bit) ? 127 : 0;
        int32_t ri = bi + (ed * (ti - bi)) / 127;

        if (ri >= 64)
            out |= bit;
        else
            out &= (uint8_t)~bit;
    }
    return out;
}

static inline void fm_dj_seq_ens_ov_on_bar(fm_dj_seq_ens_ov_t *ov)
{
    if (!ov->active)
        return;
    if (ov->seg_bars > 0u)
        ov->seg_bars--;
    if (ov->seg_bars > 0u)
        return;
    if (ov->seg == FM_DJ_SEQ_ENS_OV_SEG_A) {
        if (ov->plan_b) {
            ov->seg = FM_DJ_SEQ_ENS_OV_SEG_B;
            ov->combo_cur = ov->combo_b;
            ov->seg_bars = 1u;
        } else {
            ov->seg = FM_DJ_SEQ_ENS_OV_SEG_RES;
            ov->seg_bars = 1u;
            ov->combo_cur = FM_SEQ_ENS_ALL;
        }
    } else if (ov->seg == FM_DJ_SEQ_ENS_OV_SEG_B) {
        ov->seg = FM_DJ_SEQ_ENS_OV_SEG_RES;
        ov->seg_bars = 1u;
        ov->combo_cur = FM_SEQ_ENS_ALL;
    } else {
        ov->active = 0u;
        ov->cycle_done = 1u;
        ov->combo_cur = ov->base_ens;
    }
    ov->toast_ens = ov->combo_cur;
}

static inline void fm_dj_seq_ens_ov_invert_tick(fm_dj_t *dj, fm_dj_seq_ens_ov_t *ov, uint8_t bar_step,
                                                uint8_t steps_per_bar, const uint8_t *ens_mute)
{
    uint8_t half = steps_per_bar / 2u;
    uint8_t combo_m;

    if (!ov->active || ov->seg == FM_DJ_SEQ_ENS_OV_SEG_RES)
        return;
    if (ov->invert_left > 0u) {
        ov->invert_left--;
        ov->invert_accent = 1u;
        if (ov->invert_left == 0u)
            ov->invert_accent = 0u;
        return;
    }
    if (!ens_mute || !fm_dj_seq_ens_is_combo(ov->combo_cur))
        return;
    combo_m = fm_dj_seq_mask_low6(ens_mute[ov->combo_cur]);
    if (!fm_dj_seq_mask_dj_intervention_ok(fm_dj_seq_invert_mask(combo_m)))
        return;
    if (bar_step == half && ((fm_dj_rng_u8(dj) & 7u) == 0u)) {
        ov->invert_left = 2u;
        ov->invert_accent = 1u;
    }
}

/*
 * Rhythmic categorical combo overlay while ENSEMBLE slot is live.
 * Returns lane mute mask to apply (low 6 bits), or 0xff when caller should not override.
 */
static inline uint8_t fm_dj_seq_ens_ov_step(fm_dj_t *dj, fm_dj_seq_ens_ov_t *ov, uint8_t song_bar,
                                            uint8_t bar_step, uint8_t steps_per_bar, uint8_t playing,
                                            uint8_t base_ens, uint8_t cur_mask, int32_t ed, const uint8_t *ens_mute,
                                            uint8_t blocked)
{
    uint8_t live = fm_dj_seq_ens_live(ed);

    if (blocked || !ens_mute) {
        if (ov->active) {
            uint8_t res = fm_dj_seq_ens_ov_resolve_ens(ov);
            uint8_t bm = ov->base_mask;
            uint8_t be = ov->base_ens;

            fm_dj_seq_ens_ov_reset(ov);
            if (be == FM_SEQ_ENS_CUSTOM)
                return bm & 0x3fu;
            return ens_mute[res] & 0x3fu;
        }
        fm_dj_seq_ens_ov_reset(ov);
        return 0xffu;
    }
    if (!live) {
        if (ov->active) {
            uint8_t res = fm_dj_seq_ens_ov_resolve_ens(ov);
            uint8_t bm = ov->base_mask;
            uint8_t be = ov->base_ens;

            fm_dj_seq_ens_ov_reset(ov);
            if (be == FM_SEQ_ENS_CUSTOM)
                return bm & 0x3fu;
            return ens_mute[res] & 0x3fu;
        }
        ov->cycle_done = 0u;
        return 0xffu;
    }
    if (!ov->active) {
        if (ov->cycle_done)
            return 0xffu;
        ov->active = 1u;
        ov->base_ens = base_ens;
        ov->base_mask = (uint8_t)(cur_mask & 0x3fu);
        ov->last_combo = 0xffu;
        fm_dj_seq_ens_ov_plan(dj, ov, dj->phrase_bar, ens_mute);
        ov->bar_seen = song_bar;
    }
    if (bar_step == 0u && song_bar != ov->bar_seen) {
        ov->bar_seen = song_bar;
        fm_dj_seq_ens_ov_on_bar(ov);
    }
    (void)playing;
    fm_dj_seq_ens_ov_invert_tick(dj, ov, bar_step, steps_per_bar, ens_mute);
    if (ov->combo_cur == FM_SEQ_ENS_CUSTOM)
        return ov->base_mask & 0x3fu;
    if (ov->seg != FM_DJ_SEQ_ENS_OV_SEG_RES) {
        if (fm_dj_seq_ens_is_combo(ov->combo_cur))
            ov->combo_cur = fm_dj_seq_ens_sanitize_combo(ens_mute, ov->combo_cur);
        else if (ov->combo_cur != FM_SEQ_ENS_ALL)
            ov->combo_cur = fm_dj_seq_ens_safe_combo(ens_mute);
    }
    if (!fm_dj_seq_ens_is_combo(ov->combo_cur) && ov->combo_cur != FM_SEQ_ENS_ALL)
        return ens_mute[FM_SEQ_ENS_ALL] & 0x3fu;
    if (fm_dj_seq_ens_is_combo(ov->combo_cur)) {
        uint8_t m = fm_dj_seq_mask_low6(ens_mute[ov->combo_cur]);

        ov->toast_ens = ov->combo_cur;
        if (!fm_dj_seq_mask_dj_intervention_ok(m)) {
            ov->combo_cur = fm_dj_seq_ens_safe_combo(ens_mute);
            ov->toast_ens = ov->combo_cur;
            m = fm_dj_seq_mask_low6(ens_mute[ov->combo_cur]);
        }
        return m;
    }
    ov->toast_ens = ov->combo_cur;
    return ens_mute[ov->combo_cur] & 0x3fu;
}

static inline uint8_t fm_dj_seq_ens_ov_base_mask(const fm_dj_seq_ens_ov_t *ov, const uint8_t *ens_mute)
{
    if (!ens_mute)
        return 0u;
    if (ov->base_ens == FM_SEQ_ENS_CUSTOM)
        return (uint8_t)(ov->base_mask & 0x3fu);
    if (ov->base_ens >= FM_SEQ_NENSEMBLES)
        return (uint8_t)(ens_mute[FM_SEQ_ENS_ALL] & 0x3fu);
    return (uint8_t)(ens_mute[ov->base_ens] & 0x3fu);
}

static inline uint8_t fm_dj_seq_ens_ov_cur_mask(const fm_dj_seq_ens_ov_t *ov, const uint8_t *ens_mute)
{
    if (!ens_mute || !ov->active)
        return 0u;
    if (ov->combo_cur == FM_SEQ_ENS_CUSTOM)
        return (uint8_t)(ov->base_mask & 0x3fu);
    if (ov->combo_cur >= FM_SEQ_NENSEMBLES)
        return (uint8_t)(ens_mute[FM_SEQ_ENS_ALL] & 0x3fu);
    return (uint8_t)(ens_mute[ov->combo_cur] & 0x3fu);
}

static inline uint8_t fm_dj_seq_ens_ov_display_ens(const fm_dj_seq_ens_ov_t *ov, int32_t ed,
                                                   const uint8_t *ens_mute)
{
    uint8_t base_m;
    uint8_t cur_m;

    if (!fm_dj_seq_ens_live(ed) || !ov->active || !ens_mute)
        return 0xffu;
    if (ov->invert_accent)
        return 0xfeu; /* sentinel: toast shows INVERT */
    if (ov->seg == FM_DJ_SEQ_ENS_OV_SEG_RES) {
        base_m = fm_dj_seq_ens_ov_base_mask(ov, ens_mute);
        cur_m = (uint8_t)(ens_mute[FM_SEQ_ENS_ALL] & 0x3fu);
        if (cur_m != base_m)
            return FM_SEQ_ENS_ALL;
        return 0xffu;
    }
    base_m = fm_dj_seq_ens_ov_base_mask(ov, ens_mute);
    cur_m = fm_dj_seq_ens_ov_cur_mask(ov, ens_mute);
    if (cur_m == base_m)
        return 0xffu;
    if (ov->toast_ens == FM_SEQ_ENS_MUTE)
        return 0xffu;
    if (fm_dj_seq_ens_is_combo(ov->toast_ens))
        return ov->toast_ens;
    return 0xffu;
}

static inline uint8_t fm_dj_seq_ens_bar_overlay(const fm_dj_t *dj, const fm_dj_seq_ens_ov_t *ov,
                                                const uint8_t *ens_mute, uint8_t base_bar)
{
    uint8_t disp;
    int32_t ed;

    if (!dj->on)
        return base_bar;
    ed = fm_dj_delta_for_tgt(dj, FM_DJ_TGT_ENSEMBLE);
    disp = fm_dj_seq_ens_ov_display_ens(ov, ed, ens_mute);
    if (disp == 0xfeu) {
        if (ov->active && fm_dj_seq_ens_is_combo(ov->toast_ens))
            return fm_seq_ens_bar_preset(ov->toast_ens);
        return base_bar;
    }
    if (disp != 0xffu)
        return fm_seq_ens_bar_preset(disp);
    if (ov->active && ov->seg != FM_DJ_SEQ_ENS_OV_SEG_RES && fm_dj_seq_ens_is_combo(ov->toast_ens))
        return fm_seq_ens_bar_preset(ov->toast_ens);
    return base_bar;
}

#endif /* FM_DJ_SEQ_H */
