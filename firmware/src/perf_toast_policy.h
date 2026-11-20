/* SPDX-License-Identifier: GPL-3.0-only */
/* Performance toast lane policy (host-testable). */
#ifndef PERF_TOAST_POLICY_H
#define PERF_TOAST_POLICY_H

#include <stdint.h>

#include "toast_layout.h"

enum {
    PERF_TOAST_LANE_PROC = 0,
    PERF_TOAST_LANE_MAIN = 1,
    PERF_TOAST_LANE_EPH = 2,
    PERF_TOAST_LANE_N = 3
};

typedef struct {
    char msg[20];
    char side[24];
    uint8_t kind;
    int8_t voice_ln;
    uint32_t end_ms;
    uint32_t side_end_ms;
    uint32_t start_ms;
    uint8_t on;
} perf_toast_lane_t;

typedef struct {
    char msg[20];
    uint8_t kind;
    int8_t voice_ln;
    uint32_t end_ms;
    uint32_t start_ms;
} perf_toast_slot_t;

typedef struct {
    uint8_t dj_on;
    uint8_t arp_mode;
    uint8_t gest_arp;
    uint8_t gest_loop;
} perf_toast_dj_ctx_t;

static inline int perf_toast_policy_same_msg(const char *a, const char *b)
{
    uint32_t i = 0u;

    if (!a || !b)
        return a == b;
    while (a[i] && b[i] && a[i] == b[i])
        i++;
    return a[i] == b[i];
}

static inline int32_t perf_toast_policy_find_ix(const perf_toast_slot_t *stack, uint8_t n, const char *msg)
{
    uint32_t i;

    if (!msg)
        return -1;
    for (i = 0u; i < n; i++) {
        if (perf_toast_policy_same_msg(stack[i].msg, msg))
            return (int32_t)i;
    }
    return -1;
}

static inline void perf_toast_policy_slot_touch(perf_toast_slot_t *s, uint8_t kind, int8_t voice_ln, uint32_t end)
{
    s->kind = kind;
    if (end == 0u || end > s->end_ms)
        s->end_ms = end;
    if (voice_ln >= 0)
        s->voice_ln = voice_ln;
}

static inline int perf_toast_policy_dj_ctx_changed(const perf_toast_dj_ctx_t *prev, uint8_t dj_on, uint8_t arp_mode,
                                                   uint8_t gest_arp, uint8_t gest_loop)
{
    if (!prev)
        return 1;
    return prev->dj_on != dj_on || prev->arp_mode != arp_mode || prev->gest_arp != gest_arp ||
           prev->gest_loop != gest_loop;
}

static inline int perf_toast_policy_is_gesture(const char *msg)
{
    return perf_toast_policy_same_msg(msg, "FILL") || perf_toast_policy_same_msg(msg, "BREAK") ||
           perf_toast_policy_same_msg(msg, "DRUM FLARE") || perf_toast_policy_same_msg(msg, "BASS FLARE") ||
           perf_toast_policy_same_msg(msg, "BAND FLARE") || perf_toast_policy_same_msg(msg, "FLAM!");
}

static inline int perf_toast_policy_is_dj(const char *msg)
{
    return msg && msg[0] == 'D' && msg[1] == 'J' && msg[2] == 0;
}

/* MAIN lane: bare “DJ” only when stopped; while playing require a side label. */
static inline int perf_toast_policy_dj_main_live(uint8_t dj_on, uint8_t playing, int has_side)
{
    if (!dj_on)
        return 0;
    if (!playing)
        return 1;
    return has_side != 0;
}

static inline int perf_toast_policy_is_arp_label(const char *msg, const char *const *arp_names, uint32_t arp_n)
{
    uint32_t i;

    if (!msg || !msg[0])
        return 0;
    for (i = 0u; i < arp_n; i++) {
        if (arp_names[i] && arp_names[i][0] && perf_toast_policy_same_msg(msg, arp_names[i]))
            return 1;
    }
    return 0;
}

static inline int perf_toast_policy_is_process(const char *msg)
{
    return perf_toast_policy_same_msg(msg, "CROSSFADE") || perf_toast_policy_same_msg(msg, "EVOLVING") ||
           perf_toast_policy_same_msg(msg, "CUEING") || perf_toast_policy_same_msg(msg, "ECHO OUT") ||
           perf_toast_policy_same_msg(msg, "FILTER SLAM");
}

/* Band gestures ride on DJ|side while DJ on — not a second MAIN/EPH row. */
static inline int perf_toast_policy_gesture_ephemeral_msg(const char *msg)
{
    return perf_toast_policy_is_gesture(msg);
}

static inline int perf_toast_policy_dj_absorbs_gesture_msg(const char *msg, uint8_t dj_on)
{
    return dj_on != 0u && perf_toast_policy_gesture_ephemeral_msg(msg);
}

static inline int perf_toast_policy_is_sticky_perf(const char *msg, const char *const *arp_names, uint32_t arp_n)
{
    return perf_toast_policy_is_gesture(msg) || perf_toast_policy_is_dj(msg) ||
           perf_toast_policy_is_arp_label(msg, arp_names, arp_n);
}

/* Keep one sticky perf toast (live_msg); process/ephemeral rows stay. */
static inline uint8_t perf_toast_policy_evict_other_sticky(perf_toast_slot_t *stack, uint8_t n,
                                                             const char *live_msg, const char *const *arp_names,
                                                             uint32_t arp_n)
{
    uint32_t w = 0u;
    uint32_t r;

    for (r = 0u; r < n; r++) {
        const char *m = stack[r].msg;

        if (perf_toast_policy_is_sticky_perf(m, arp_names, arp_n) &&
            !perf_toast_policy_same_msg(m, live_msg)) {
            continue;
        }
        if (w != r)
            stack[w] = stack[r];
        w++;
    }
    return (uint8_t)w;
}

static inline uint8_t perf_toast_policy_count_sticky(const perf_toast_slot_t *stack, uint8_t n,
                                                     const char *const *arp_names, uint32_t arp_n)
{
    uint32_t r;
    uint8_t c = 0u;

    for (r = 0u; r < n; r++) {
        if (perf_toast_policy_is_sticky_perf(stack[r].msg, arp_names, arp_n))
            c++;
    }
    return c;
}

/* Latched ARP mode owns the badge (page, keys, loop). DJ overlay is a separate
 * predicate and is checked first; dj_on alone does not hide a manual mode. */
static inline int perf_toast_policy_manual_arp_ok(uint8_t dj_on, uint8_t arp_mode)
{
    (void)dj_on;
    return arp_mode != 0u;
}

static inline int perf_toast_policy_dj_arp_overlay_live(uint8_t dj_on, uint8_t ov_arp, uint8_t gest_bars,
                                                        uint8_t arp_mode)
{
    return dj_on && ov_arp != 0u && gest_bars > 0u && arp_mode != 0u;
}

static inline int perf_toast_policy_dj_loop_overlay_live(uint8_t dj_on, uint8_t ov_loop, uint8_t gest_bars)
{
    return dj_on && ov_loop != 0u && gest_bars > 0u;
}

/* DJ loop overlay toast: gest window + looper actually engaged (or arming). */
static inline int perf_toast_policy_dj_loop_audible(uint8_t dj_on, uint8_t ov_loop, uint8_t gest_bars,
                                                    uint8_t loop_active, uint8_t loop_pending)
{
    if (!perf_toast_policy_dj_loop_overlay_live(dj_on, ov_loop, gest_bars))
        return 0;
    return loop_active || loop_pending;
}

#ifndef PERF_TOAST_KEY_TOAST_LOOP_LEN_LN
#define PERF_TOAST_KEY_TOAST_LOOP_LEN_LN 255u
#endif

static inline int perf_toast_policy_key_toast_is_loop_len(uint8_t key_toast_lane)
{
    return key_toast_lane == PERF_TOAST_KEY_TOAST_LOOP_LEN_LN;
}

static inline uint8_t perf_toast_policy_strip_dj_arp_labels(perf_toast_slot_t *stack, uint8_t n,
                                                            const char *allow_msg, const char *const *arp_names,
                                                            uint32_t arp_n)
{
    uint32_t w = 0u;
    uint32_t r;

    for (r = 0u; r < n; r++) {
        const char *m = stack[r].msg;

        if (perf_toast_policy_is_arp_label(m, arp_names, arp_n)) {
            if (!allow_msg || !perf_toast_policy_same_msg(m, allow_msg))
                continue;
        }
        if (w != r)
            stack[w] = stack[r];
        w++;
    }
    return (uint8_t)w;
}

/* Fixed stack: MAIN at center, PROC one row up, EPH two rows up — no push_y / reindex. */
static inline int32_t perf_toast_policy_lane_by(uint8_t lane, int32_t by_center, int32_t bh)
{
    if (lane == PERF_TOAST_LANE_MAIN)
        return by_center;
    if (lane == PERF_TOAST_LANE_PROC)
        return by_center - bh;
    if (lane == PERF_TOAST_LANE_EPH)
        return by_center - bh - bh;
    return by_center;
}

static inline int perf_toast_policy_lane_msg_is_sticky_main(const char *msg, const char *const *arp_names,
                                                            uint32_t arp_n)
{
    return perf_toast_policy_is_dj(msg) || perf_toast_policy_is_arp_label(msg, arp_names, arp_n);
}

static inline int perf_toast_policy_lane_overlay_targets_eph(int sticky_main_on, const char *sticky_msg,
                                                             const char *const *arp_names, uint32_t arp_n)
{
    return sticky_main_on &&
           perf_toast_policy_lane_msg_is_sticky_main(sticky_msg, arp_names, arp_n);
}

/* Ink-tight badge width for MAIN draw (matches cv_draw_toast / cv_draw_toast_arp_note). */
static inline int32_t perf_toast_policy_main_badge_bw(const toast_font_t *f, const char *msg, const char *side,
                                                      int32_t mode_note_gap)
{
    toast_layout_t lay;

    if (!msg || !msg[0])
        return TOAST_INK_PAD_L + TOAST_INK_PAD_R;
    if (side && side[0]) {
        int32_t gap = toast_mode_note_gap_w(f, mode_note_gap);

        toast_layout_mode_note(f, msg, side, gap, &lay);
    } else
        toast_layout_msg(f, msg, &lay);
    return lay.bw;
}

static inline void perf_toast_policy_lane_clear(perf_toast_lane_t *L)
{
    if (!L)
        return;
    L->on = 0u;
    L->msg[0] = 0;
    L->side[0] = 0;
    L->kind = 0u;
    L->voice_ln = -1;
    L->end_ms = 0u;
    L->side_end_ms = 0u;
    L->start_ms = 0u;
}

static inline void perf_toast_policy_lane_prune(perf_toast_lane_t *lanes, uint32_t now)
{
    uint32_t i;

    if (!lanes)
        return;
    for (i = 0u; i < PERF_TOAST_LANE_N; i++) {
        perf_toast_lane_t *L = &lanes[i];

        if (!L->on)
            continue;
        if (L->end_ms != 0u && L->end_ms <= now)
            perf_toast_policy_lane_clear(L);
        else if (L->side[0] && L->side_end_ms != 0u && L->side_end_ms <= now) {
            L->side[0] = 0;
            L->side_end_ms = 0u;
        }
    }
}

static inline void perf_toast_policy_lane_copy_msg(char *dst, uint32_t dst_sz, const char *msg)
{
    uint32_t i = 0u;

    if (!dst || dst_sz == 0u)
        return;
    if (!msg) {
        dst[0] = 0;
        return;
    }
    while (msg[i] && i + 1u < dst_sz) {
        dst[i] = msg[i];
        i++;
    }
    dst[i] = 0;
}

/* DJ|side: loop-length key toast is bare division (e.g. "3/1") — prefix LOOP for display. */
static inline void perf_toast_policy_dj_loop_side(char *dst, uint32_t dst_sz, const char *loop_div)
{
    uint32_t i = 0u;
    const char *pfx = "LOOP ";

    if (!dst || dst_sz == 0u)
        return;
    if (!loop_div || !loop_div[0]) {
        dst[0] = 0;
        return;
    }
    while (pfx[i] && i + 1u < dst_sz) {
        dst[i] = pfx[i];
        i++;
    }
    perf_toast_policy_lane_copy_msg(dst + i, dst_sz - i, loop_div);
}

static inline void perf_toast_policy_lane_set_eph(perf_toast_lane_t *L, const char *msg, uint8_t kind,
                                                 int8_t voice_ln, uint32_t end, uint32_t now)
{
    if (!L || !msg || !msg[0]) {
        if (L)
            perf_toast_policy_lane_clear(L);
        return;
    }
    if (L->on && perf_toast_policy_same_msg(L->msg, msg)) {
        L->kind = kind;
        if (end == 0u || end > L->end_ms)
            L->end_ms = end;
        if (voice_ln >= 0)
            L->voice_ln = voice_ln;
    } else {
        perf_toast_policy_lane_copy_msg(L->msg, sizeof L->msg, msg);
        L->kind = kind;
        L->voice_ln = voice_ln;
        L->end_ms = end;
        L->side[0] = 0;
        L->side_end_ms = 0u;
        L->start_ms = now;
        L->on = 1u;
    }
}

static inline void perf_toast_policy_lane_set_proc(perf_toast_lane_t *L, const char *msg, uint8_t kind,
                                                   uint32_t end, uint32_t now)
{
    if (!L || !msg || !msg[0]) {
        if (L)
            perf_toast_policy_lane_clear(L);
        return;
    }
    if (L->on && perf_toast_policy_same_msg(L->msg, msg)) {
        L->kind = kind;
        if (end == 0u || end > L->end_ms)
            L->end_ms = end;
    } else {
        perf_toast_policy_lane_copy_msg(L->msg, sizeof L->msg, msg);
        L->kind = kind;
        L->voice_ln = -1;
        L->end_ms = end;
        L->side[0] = 0;
        L->side_end_ms = 0u;
        L->start_ms = now;
        L->on = 1u;
    }
}

static inline void perf_toast_policy_lane_set_main(perf_toast_lane_t *L, const char *msg, uint8_t kind,
                                                   int8_t voice_ln, uint32_t end, uint32_t now, const char *side,
                                                   uint32_t side_end)
{
    if (!L || !msg || !msg[0]) {
        if (L)
            perf_toast_policy_lane_clear(L);
        return;
    }
    if (L->on && perf_toast_policy_same_msg(L->msg, msg)) {
        L->kind = kind;
        if (end == 0u || end > L->end_ms)
            L->end_ms = end;
        if (voice_ln >= 0)
            L->voice_ln = voice_ln;
    } else {
        perf_toast_policy_lane_copy_msg(L->msg, sizeof L->msg, msg);
        L->kind = kind;
        L->voice_ln = voice_ln;
        L->end_ms = end;
        L->start_ms = now;
        L->on = 1u;
        /* New identity drops prior side unless this call supplies one. */
        if (!side) {
            L->side[0] = 0;
            L->side_end_ms = 0u;
        }
    }
    /* side == NULL → leave side alone (caller draws live). side "" → clear. */
    if (side) {
        if (!side[0]) {
            L->side[0] = 0;
            L->side_end_ms = 0u;
        } else {
            perf_toast_policy_lane_copy_msg(L->side, sizeof L->side, side);
            L->side_end_ms = side_end;
        }
    }
}

#endif /* PERF_TOAST_POLICY_H */
