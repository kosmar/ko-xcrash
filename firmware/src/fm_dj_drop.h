/* SPDX-License-Identifier: GPL-3.0-only */
/* DROP pre-silence lane bind + gate (include after fm_dj.h). */
#ifndef FM_DJ_DROP_H
#define FM_DJ_DROP_H

#include "fm_dj.h"

#ifndef FM_DJ_DROP_SIL_LN_N
#define FM_DJ_DROP_SIL_LN_N 6u
#endif

#ifndef FM_DJ_DROP_SIL_GENRE_MASK
#define FM_DJ_DROP_SIL_GENRE_MASK 0x3eu
#endif

static inline uint8_t fm_dj_drop_sil_mask_pick_rng(fm_dj_t *dj, uint8_t mask)
{
    uint8_t n = 0u;
    uint8_t i;
    uint8_t r;

    for (i = 0u; i < FM_DJ_DROP_SIL_LN_N; i++) {
        if (mask & (uint8_t)(1u << i))
            n++;
    }
    if (!n)
        return FM_DJ_DROP_SIL_KEEP_ALL;
    r = (uint8_t)(fm_dj_rng_u8(dj) % n);
    for (i = 0u; i < FM_DJ_DROP_SIL_LN_N; i++) {
        if (!(mask & (uint8_t)(1u << i)))
            continue;
        if (r == 0u)
            return i;
        r--;
    }
    return FM_DJ_DROP_SIL_KEEP_ALL;
}

static inline uint8_t fm_dj_drop_sil_pick_lane(fm_dj_t *dj, uint8_t audible_mask)
{
    uint8_t pool = (uint8_t)(audible_mask & FM_DJ_DROP_SIL_GENRE_MASK);
    uint8_t r;

    if (!pool)
        pool = (uint8_t)(FM_DJ_DROP_SIL_GENRE_MASK & (uint8_t)~1u);
    r = fm_dj_rng_u8(dj);
    if ((r & 0x1fu) != 0u)
        pool &= (uint8_t)~1u;
    if (!pool)
        pool = (uint8_t)(FM_DJ_DROP_SIL_GENRE_MASK & (uint8_t)~1u);
    return fm_dj_drop_sil_mask_pick_rng(dj, pool);
}

static inline void fm_dj_drop_sil_bind_audible(fm_dj_t *dj, uint8_t audible_mask)
{
    if (!dj->drop_sil_lane_pend)
        return;
    dj->drop_sil_lane_pend = 0u;
    if (dj->drop_sil_keep != FM_DJ_DROP_SIL_KEEP_PEND)
        return;
    dj->drop_sil_keep = fm_dj_drop_sil_pick_lane(dj, audible_mask);
}

static inline uint8_t fm_dj_drop_sil_lane_mask(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                               uint8_t steps_per_bar)
{
    if (!fm_dj_pre_drop_silence(dj, playing, bar_step, steps_per_bar))
        return 0u;
    if (dj->drop_sil_keep >= FM_DJ_DROP_SIL_LN_N)
        return 0u;
    return (uint8_t)(1u << dj->drop_sil_keep);
}

static inline void fm_dj_drop_sil_gate_lanes(const fm_dj_t *dj, uint8_t playing, uint8_t bar_step,
                                             uint8_t steps_per_bar, uint8_t *on)
{
    uint8_t keep = dj->drop_sil_keep;
    uint8_t ln;

    if (!fm_dj_pre_drop_silence(dj, playing, bar_step, steps_per_bar))
        return;
    if (keep >= FM_DJ_DROP_SIL_LN_N) {
        for (ln = 0u; ln < FM_DJ_DROP_SIL_LN_N; ln++)
            on[ln] = 0u;
        return;
    }
    for (ln = 0u; ln < FM_DJ_DROP_SIL_LN_N; ln++) {
        if (ln != keep)
            on[ln] = 0u;
    }
}

#endif /* FM_DJ_DROP_H */
