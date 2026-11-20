/* SPDX-License-Identifier: GPL-3.0-only */
/* DJ phrase gesture planner (host-testable); app applies via dub_dj_gest_* in fm_app.c */
#ifndef FM_DJ_GEST_H
#define FM_DJ_GEST_H

#include "fm_dj.h"
#include <stdint.h>

enum {
    FM_DJ_GEST_NONE = 0,
    FM_DJ_GEST_SOLO_BASS,
    FM_DJ_GEST_SOLO_DRUM,
    FM_DJ_GEST_FLAM,
    FM_DJ_GEST_BREAK_MICRO,
    FM_DJ_GEST_FILL,
    FM_DJ_GEST_LOOP,
    FM_DJ_GEST_ARP,
    FM_DJ_GEST_N
};

#ifndef FM_DJ_GEST_CAP_SOLO
#define FM_DJ_GEST_CAP_SOLO 1u
#endif
#ifndef FM_DJ_GEST_CAP_FLAM
#define FM_DJ_GEST_CAP_FLAM 1u
#endif
#ifndef FM_DJ_GEST_CAP_BREAK
#define FM_DJ_GEST_CAP_BREAK 1u
#endif
#ifndef FM_DJ_GEST_CAP_FILL
#define FM_DJ_GEST_CAP_FILL 0u
#endif
#if defined(GENRE_JAZZ)
#undef FM_DJ_GEST_CAP_LOOP
#define FM_DJ_GEST_CAP_LOOP 0u
#endif
#ifndef FM_DJ_GEST_CAP_LOOP
#define FM_DJ_GEST_CAP_LOOP 1u
#endif
#ifndef FM_DJ_GEST_CAP_ARP
#define FM_DJ_GEST_CAP_ARP 1u
#endif

#ifndef FM_DJ_GEST_FLAM_CD_BARS
#define FM_DJ_GEST_FLAM_CD_BARS 16u
#endif
#ifndef FM_DJ_GEST_FLAM_RARE_DENOM
#define FM_DJ_GEST_FLAM_RARE_DENOM 5u
#endif
#ifndef FM_DJ_GEST_PICK_DENOM
#define FM_DJ_GEST_PICK_DENOM 13u
#endif
#ifndef FM_DJ_GEST_ARP_RARE
#define FM_DJ_GEST_ARP_RARE 5u
#endif
#ifndef FM_DJ_GEST_ARP_CD_BARS
#define FM_DJ_GEST_ARP_CD_BARS 40u
#endif
#ifndef FM_DJ_GEST_LOOP_CD_BARS
#define FM_DJ_GEST_LOOP_CD_BARS 48u
#endif
#ifndef FM_DJ_GEST_LOOP_RARE
#define FM_DJ_GEST_LOOP_RARE 59u
#endif

typedef struct {
    uint8_t caps;
    uint8_t manual_gesture_latch;
    uint8_t manual_arp_mode;
    uint8_t loop_active;
    uint8_t loop_cd_bars;
    uint8_t arp_cd_bars;
    uint8_t gest_active;
    uint8_t fill_build_live;
    uint8_t playing;
    uint8_t flam_cd_bars;
} fm_dj_gest_env_t;

static inline void fm_dj_gest_env_tick(fm_dj_gest_env_t *env)
{
    if (env->flam_cd_bars > 0u)
        env->flam_cd_bars--;
    if (env->loop_cd_bars > 0u)
        env->loop_cd_bars--;
    if (env->arp_cd_bars > 0u)
        env->arp_cd_bars--;
}

static inline void fm_dj_gest_flam_armed(fm_dj_gest_env_t *env)
{
    env->flam_cd_bars = FM_DJ_GEST_FLAM_CD_BARS;
}

static inline uint8_t fm_dj_gest_caps_default(void)
{
    uint8_t c = 0u;

    if (FM_DJ_GEST_CAP_SOLO)
        c |= 1u;
    if (FM_DJ_GEST_CAP_FLAM)
        c |= 2u;
    if (FM_DJ_GEST_CAP_BREAK)
        c |= 4u;
    if (FM_DJ_GEST_CAP_FILL)
        c |= 8u;
    if (FM_DJ_GEST_CAP_LOOP)
        c |= 16u;
    if (FM_DJ_GEST_CAP_ARP)
        c |= 32u;
    return c;
}

static inline int fm_dj_gest_env_blocked(const fm_dj_t *dj, const fm_dj_gest_env_t *env)
{
    if (!dj->on)
        return 1;
    if (dj->fill_blocked)
        return 1;
    if (!env->playing)
        return 1;
    if (env->manual_gesture_latch != 0u)
        return 1;
    if (env->gest_active)
        return 1;
    if (env->fill_build_live)
        return 1;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_BREAK))
        return 1;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_DROP))
        return 1;
    if (fm_dj_trans_is_drop_path(dj) && fm_dj_pre_drop_silence(dj, 1u, 0u, 16u) &&
        dj->drop_sil_keep == 0xffu)
        return 1;
    if (dj->trans_pending != 0u)
        return 1;
    if (dj->phrase_bar + 1u >= dj->drop_arm)
        return 1;
    return 0;
}

/* Sparse pick: 0 = none. phrase_bar is pre-increment arm index. */
static inline uint8_t fm_dj_gest_plan_pick(fm_dj_t *dj, uint8_t phrase_bar, const fm_dj_gest_env_t *env)
{
    uint8_t r;
    uint8_t role;
    uint8_t pick;
    uint8_t caps;

    if (fm_dj_gest_env_blocked(dj, env))
        return FM_DJ_GEST_NONE;
    caps = env->caps;
    if (!caps)
        return FM_DJ_GEST_NONE;
    r = (uint8_t)fm_dj_rng_u8(dj);
    if ((r % FM_DJ_GEST_PICK_DENOM) != 0u)
        return FM_DJ_GEST_NONE;
    role = dj->role;
    if (phrase_bar <= 3u && (r % 3u) == 0u)
        return FM_DJ_GEST_NONE;

    pick = (uint8_t)(r % 7u);
    if (pick == 0u && (caps & 1u) && phrase_bar >= 4u && phrase_bar < dj->build_end) {
        if ((r & 1u) == 0u)
            return FM_DJ_GEST_SOLO_BASS;
        return FM_DJ_GEST_SOLO_DRUM;
    }
    if (pick == 1u && (caps & 2u) && env->flam_cd_bars == 0u && phrase_bar >= 4u &&
        phrase_bar < dj->break_arm - 3u && ((uint8_t)fm_dj_rng_u8(dj) % FM_DJ_GEST_FLAM_RARE_DENOM) == 0u)
        return FM_DJ_GEST_FLAM;
    if (pick == 2u && (caps & 4u) && phrase_bar >= 6u && phrase_bar < dj->break_arm - 2u)
        return FM_DJ_GEST_BREAK_MICRO;
    if (pick == 3u && (caps & 8u) && phrase_bar >= dj->build_end - 2u && phrase_bar < dj->break_arm - 1u)
        return FM_DJ_GEST_FILL;
    {
        /* Late in the ride. Short phrases (build_end - 2 <= 12) keep a 2-bar tail so 16-bar cycles still can loop. */
        uint8_t loop_hi = dj->build_end > 2u ? (uint8_t)(dj->build_end - 2u) : 0u;
        uint8_t loop_lo = 12u;

        if (loop_lo >= loop_hi && loop_hi > 8u)
            loop_lo = (uint8_t)(loop_hi - 2u);
        if (pick == 4u && (caps & 16u) && !env->loop_active && env->loop_cd_bars == 0u && phrase_bar >= loop_lo &&
            phrase_bar < loop_hi && ((uint8_t)fm_dj_rng_u8(dj) % FM_DJ_GEST_LOOP_RARE) == 0u)
            return FM_DJ_GEST_LOOP;
    }
    if (pick == 5u && (caps & 32u) && env->manual_arp_mode == 0u && env->arp_cd_bars == 0u && phrase_bar >= 10u &&
        role == FM_DJ_ROLE_RIDE && ((uint8_t)fm_dj_rng_u8(dj) % FM_DJ_GEST_ARP_RARE) == 0u)
        return FM_DJ_GEST_ARP;
    if ((caps & 1u) && phrase_bar >= 10u && (r % 11u) == 0u)
        return (r & 2u) ? FM_DJ_GEST_SOLO_DRUM : FM_DJ_GEST_SOLO_BASS;
    return FM_DJ_GEST_NONE;
}

static inline uint8_t fm_dj_gest_plan_bars(uint8_t kind, uint8_t rng4)
{
    switch (kind) {
    case FM_DJ_GEST_SOLO_BASS:
    case FM_DJ_GEST_SOLO_DRUM:
        return (uint8_t)(1u + (rng4 & 3u));
    case FM_DJ_GEST_BREAK_MICRO:
        return 1u;
    case FM_DJ_GEST_FILL:
        return 1u;
    case FM_DJ_GEST_LOOP:
        return (uint8_t)(2u + (rng4 & 1u));
    case FM_DJ_GEST_ARP:
        return (uint8_t)(2u + (rng4 % 3u));
    default:
        return 0u;
    }
}

static inline uint8_t fm_dj_gest_plan_arp_mode(uint8_t rng4)
{
    static const uint8_t modes[4] = {1u, 2u, 3u, 4u};

    return modes[rng4 % 4u];
}

enum {
    FM_DJ_GEST_OV_NONE = 0,
    FM_DJ_GEST_OV_ARP = 1u,
    FM_DJ_GEST_OV_LOOP = 2u,
};

typedef struct {
    uint8_t kind;
    uint8_t bars;
    uint8_t flam_steps;
    uint8_t ov;
    uint8_t saved_arp;
    uint8_t saved_loop;
} fm_dj_gest_st_t;

static inline uint8_t fm_dj_gest_st_active(const fm_dj_gest_st_t *s)
{
    return s->kind != 0u || s->flam_steps != 0u || s->bars != 0u || s->ov != 0u;
}

static inline void fm_dj_gest_st_reset(fm_dj_gest_st_t *s)
{
    s->kind = 0u;
    s->bars = 0u;
    s->flam_steps = 0u;
    s->ov = FM_DJ_GEST_OV_NONE;
}

static inline uint8_t fm_dj_gest_st_abort(const fm_dj_t *dj)
{
    if (!dj->on)
        return 1u;
    if (dj->fill_blocked)
        return 1u;
    if (dj->trans_pending != 0u)
        return 1u;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_DROP))
        return 1u;
    if (fm_dj_slot_role_peaking(dj, FM_DJ_ROLE_BREAK))
        return 1u;
    return 0u;
}

/* After DJ loop overlay (gest bars done): cancel arming; queue stop if looper is on. */
static inline void fm_dj_gest_loop_overlay_teardown(uint8_t loop_active, uint8_t *loop_end_pend,
                                                    uint8_t *loop_pending)
{
    *loop_pending = 0u;
    if (loop_active)
        *loop_end_pend = 1u;
}

/* Idempotent restore: arp_mode in/out; loop_end_pend set when loop overlay ends. */
static inline uint8_t fm_dj_gest_st_restore(fm_dj_gest_st_t *s, uint8_t *arp_mode, uint8_t loop_active,
                                          uint8_t *loop_end_pend)
{
    if (!fm_dj_gest_st_active(s))
        return 0u;
    if (s->saved_loop == 0u && loop_active)
        *loop_end_pend = 1u;
    *arp_mode = s->saved_arp;
    fm_dj_gest_st_reset(s);
    return 1u;
}

#define FM_DJ_GEST_WIRED 1

#endif /* FM_DJ_GEST_H */
