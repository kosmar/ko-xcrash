/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_SEQ_H
#define BASS_SEQ_H

#include <stdint.h>
#include "dub_types.h"
#include "fm_seq.h"

enum { BASS_GATE_TIE = 8u, BASS_RATCHET_MIN = 1u, BASS_RATCHET_MAX = 4u };
/* Step accent / slide level: 0 OFF, 1 = 33, 2 = 66, 3 = 100 (the former binary ON). */
enum { BASS_STEP_LV_FULL = 3u };

typedef struct {
    uint8_t note;
    uint8_t gate;
    uint8_t accent;
    uint8_t slide;
    uint8_t ratchet;
    uint8_t lock;
} bass_step_t;

typedef struct {
    uint8_t gate;
    uint8_t accent;
    uint8_t slide;
    uint8_t ratchet;
    uint8_t lock;
} bass_step_x_t;

typedef struct {
    uint8_t glide_time;
} bass_seq_voice_t;

typedef struct {
    uint8_t hold_active;
    uint8_t hold_note;
    uint8_t hold_vel;
    uint8_t legato_back_note;
    uint8_t seq_cursor_note;
} bass_midi_hold_t;

enum {
    BASS_LIVE_HOLD_ACCENT = 1u,
    BASS_LIVE_HOLD_SLIDE = 2u,
    BASS_LIVE_HOLD_RATCHET = 4u,
    BASS_LIVE_HOLD_GATE = 8u,
    BASS_LIVE_HOLD_MUTE = 16u
};

static inline void bass_step_defaults(bass_step_t *s, uint8_t n)
{
    uint8_t i;

    if (!s)
        return;
    for (i = 0u; i < n; i++) {
        s[i].note = 0u;
        s[i].gate = 4u;
        s[i].accent = 0u;
        s[i].slide = 0u;
        s[i].ratchet = 1u;
        s[i].lock = 0u;
    }
}

static inline void bass_step_x_defaults(bass_step_x_t *x, uint8_t n)
{
    uint8_t i;

    if (!x)
        return;
    for (i = 0u; i < n; i++) {
        x[i].gate = 4u;
        x[i].accent = 0u;
        x[i].slide = 0u;
        x[i].ratchet = 1u;
        x[i].lock = 0u;
    }
}

static inline void bass_seq_voice_defaults(bass_seq_voice_t *v)
{
    if (!v)
        return;
    v->glide_time = 24u;
}

static inline void bass_step_edit_locks_step(bass_step_t *s)
{
    if (s)
        s->lock = 1u;
}

static inline void bass_step_x_edit_locks_step(bass_step_x_t *x)
{
    if (x)
        x->lock = 1u;
}

static inline void bass_step_long_clear_unlock(bass_step_t *s, uint8_t n)
{
    uint8_t i;

    if (!s)
        return;
    for (i = 0u; i < n; i++)
        s[i].lock = 0u;
}

static inline void bass_step_x_long_clear_unlock(bass_step_x_t *x, uint8_t n)
{
    uint8_t i;

    if (!x)
        return;
    for (i = 0u; i < n; i++)
        x[i].lock = 0u;
}

static inline uint8_t bass_step_gate_is_tie(uint8_t gate)
{
    return gate >= BASS_GATE_TIE ? 1u : 0u;
}

/* Seq gate length in samples (0 = tie / hold until rest or note-off). */
static inline uint32_t bass_step_gate_samples(uint8_t gate, uint32_t samples_per_step)
{
    uint32_t sp;

    if (!gate || bass_step_gate_is_tie(gate))
        return 0u;
    sp = samples_per_step ? samples_per_step : 1u;
    return (sp * (uint32_t)gate) / 8u + 1u;
}

static inline uint8_t bass_step_ratchet_allowed(uint8_t slide, uint8_t gate)
{
    if (slide)
        return 0u;
    if (bass_step_gate_is_tie(gate))
        return 0u;
    return 1u;
}

static inline uint8_t bass_step_clamp_ratchet(uint8_t r)
{
    if (r < BASS_RATCHET_MIN)
        return BASS_RATCHET_MIN;
    if (r > BASS_RATCHET_MAX)
        return BASS_RATCHET_MAX;
    return r;
}

static inline uint8_t bass_step_lv_clamp(uint8_t lv)
{
    return lv > BASS_STEP_LV_FULL ? BASS_STEP_LV_FULL : lv;
}

static inline const char *bass_step_lv_name(uint8_t lv)
{
    static const char *const N[BASS_STEP_LV_FULL + 1u] = {"OFF", "33", "66", "100"};

    return N[bass_step_lv_clamp(lv)];
}

/* Default step VEL readout / P-lock seed: 96 (OFF) .. 110 (accent 100). */
static inline uint8_t bass_step_lv_vel(uint8_t accent_lv)
{
    return (uint8_t)(96u + (14u * (uint32_t)bass_step_lv_clamp(accent_lv)) / BASS_STEP_LV_FULL);
}

static inline uint8_t bass_step_effective_gate(const bass_step_x_t *x, uint8_t live_hold, uint8_t live_gate)
{
    if (live_hold & BASS_LIVE_HOLD_GATE)
        return live_gate;
    return x ? x->gate : 4u;
}

static inline uint8_t bass_step_effective_ratchet(const bass_step_x_t *x, uint8_t live_hold, uint8_t live_r)
{
    uint8_t g;
    uint8_t sl;

    if (live_hold & BASS_LIVE_HOLD_RATCHET)
        return bass_step_clamp_ratchet(live_r);
    g = x ? x->gate : 4u;
    sl = x ? x->slide : 0u;
    if (!bass_step_ratchet_allowed(sl, g))
        return 1u;
    return bass_step_clamp_ratchet(x && x->ratchet ? x->ratchet : 1u);
}

static inline uint8_t bass_step_effective_slide(const bass_step_x_t *x, uint8_t live_hold, uint8_t live_s)
{
    if (live_hold & BASS_LIVE_HOLD_SLIDE)
        return live_s ? BASS_STEP_LV_FULL : 0u;
    return x ? bass_step_lv_clamp(x->slide) : 0u;
}

static inline uint8_t bass_step_effective_accent(const bass_step_x_t *x, uint8_t live_hold, uint8_t live_a)
{
    if (live_hold & BASS_LIVE_HOLD_ACCENT)
        return live_a ? BASS_STEP_LV_FULL : 0u;
    return x ? bass_step_lv_clamp(x->accent) : 0u;
}

static inline uint8_t bass_step_effective_mute(uint8_t live_hold, uint8_t live_mute)
{
    if (live_hold & BASS_LIVE_HOLD_MUTE)
        return live_mute ? 1u : 0u;
    return 0u;
}

static inline uint8_t bass_glide_active(uint8_t slide_on, const bass_seq_voice_t *v)
{
    (void)v;
    return slide_on;
}

static inline uint8_t bass_seq_edit_bar_runtime(uint8_t seq_view_page, uint8_t seq_pages)
{
    uint8_t bars = fm_seq_pages_clamp(seq_pages);

    if (bars > 8u)
        bars = 8u;
    if (bars < 1u)
        bars = 1u;
    {
        uint8_t bar = (uint8_t)(seq_view_page % 8u);

        if (bar >= bars)
            bar = bars > 0u ? (uint8_t)(bars - 1u) : 0u;
        return bar;
    }
}

/* Edit cursor (bar+step), independent of transport playhead unless never touched. */
static inline uint8_t bass_seq_cursor_step_runtime(uint8_t seq_plock_step, uint8_t seq_view_page,
                                                   uint8_t seq_pages, uint8_t play_step)
{
    if (seq_plock_step != FM_SEQ_PLOCK_NONE && seq_plock_step < DUB_SEQ_STEPS)
        return seq_plock_step;
    return (uint8_t)((uint32_t)bass_seq_edit_bar_runtime(seq_view_page, seq_pages) * FM_SEQ_PAGE_STEPS +
                     (uint32_t)(play_step % FM_SEQ_PAGE_STEPS));
}

static inline void bass_midi_hold_init(bass_midi_hold_t *m)
{
    if (!m)
        return;
    m->hold_active = 0u;
    m->hold_note = 0u;
    m->hold_vel = 0u;
    m->legato_back_note = 0u;
    m->seq_cursor_note = 0u;
}

static inline void bass_midi_hold_set_seq_note(bass_midi_hold_t *m, uint8_t note)
{
    if (!m)
        return;
    m->seq_cursor_note = note;
    if (!m->hold_active)
        m->legato_back_note = note;
}

static inline void bass_midi_hold_note_on(bass_midi_hold_t *m, uint8_t note, uint8_t vel)
{
    if (!m)
        return;
    if (!m->hold_active)
        m->legato_back_note = m->seq_cursor_note;
    m->hold_active = 1u;
    m->hold_note = note;
    m->hold_vel = vel;
}

static inline void bass_midi_hold_note_off(bass_midi_hold_t *m, uint8_t note)
{
    if (!m)
        return;
    if (!m->hold_active)
        return;
    if (m->hold_note != note)
        return;
    m->hold_active = 0u;
    m->hold_note = 0u;
    m->hold_vel = 0u;
}

static inline uint8_t bass_midi_priority_note(const bass_midi_hold_t *m)
{
    if (!m)
        return 0u;
    if (m->hold_active)
        return m->hold_note;
    return m->legato_back_note;
}

static inline uint8_t bass_midi_has_priority(const bass_midi_hold_t *m)
{
    return m && m->hold_active ? 1u : 0u;
}

#endif
