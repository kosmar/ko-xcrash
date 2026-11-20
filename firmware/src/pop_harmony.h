/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef POP_HARMONY_H
#define POP_HARMONY_H

#include "dub_types.h"
#include "pop_data.h"

#ifndef DUB_CHORD_N
#define DUB_CHORD_N 4u
#endif

/* Scale ids: 1 = natural minor, 2 = dorian, 4 = major (SCALE_SEMIS in fm_app.c). */
static const uint8_t POP_SCALE_SEMIS[3][7] = {
    { 0, 2, 3, 5, 7, 8, 10 },
    { 0, 2, 3, 5, 7, 9, 10 },
    { 0, 2, 4, 5, 7, 9, 11 },
};

static inline uint8_t pop_scale_id(uint8_t scale)
{
    if (scale == 2u)
        return 1u;
    if (scale == 4u)
        return 2u;
    return 0u;
}

static inline uint8_t pop_deg_note(uint8_t root, uint8_t scale, uint8_t deg)
{
    uint8_t sid = pop_scale_id(scale);
    uint8_t d = deg % 7u;
    uint8_t oct = deg / 7u;
    int32_t n = (int32_t)(root / 12u + oct) * 12 + (int32_t)(root % 12u) + (int32_t)POP_SCALE_SEMIS[sid][d];

    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    return (uint8_t)n;
}

static inline uint16_t pop_music_pos(uint8_t bar, uint8_t bar_step)
{
    return (uint16_t)(((uint16_t)bar * 32u + bar_step) & 255u);
}

static inline uint8_t pop_mbar(uint16_t pos)
{
    return (uint8_t)(pos >> 4);
}

static inline uint8_t pop_s16(uint16_t pos)
{
    return (uint8_t)(pos & 15u);
}

static inline uint8_t pop_chord_slot(uint8_t plate, uint8_t mbar)
{
    uint8_t bars = POP_TRACK_CHORD_BARS[plate];
    uint8_t prog = POP_TRACK_PROG_IX[plate];
    uint8_t n = POP_PROG_LEN[prog];
    uint8_t slot;

    if (bars >= 2u)
        slot = (uint8_t)((mbar >> 1) % n);
    else
        slot = (uint8_t)(mbar % n);
    return slot;
}

static inline uint8_t pop_chord_deg(uint8_t plate, uint8_t mbar)
{
    uint8_t prog = POP_TRACK_PROG_IX[plate];
    uint8_t slot = pop_chord_slot(plate, mbar);

    return POP_PROG_DEG[prog][slot];
}

static inline uint8_t pop_chord_root_midi(uint8_t plate, uint8_t mbar)
{
    uint8_t root = pop_plate_root(plate);
    uint8_t scale = pop_plate_scale(plate);
    uint8_t deg = pop_chord_deg(plate, mbar);

    return pop_deg_note(root, scale, deg);
}

static inline uint8_t pop_nearest_chord_tone_deg(uint8_t scale, uint8_t chord_deg, uint8_t mel_deg)
{
    uint8_t sid = pop_scale_id(scale);
    uint8_t best = mel_deg;
    uint8_t best_dist = 255u;
    uint8_t vi;
    static const int8_t stacks[3][4] = {
        { 0, 2, 4, 0 },
        { 0, 2, 4, 6 },
        { 0, 4, 4, 0 },
    };

    for (vi = 0; vi < 4u; vi++) {
        uint8_t cd = (uint8_t)((int32_t)chord_deg + stacks[sid][vi]);
        uint8_t dist = (uint8_t)((mel_deg + 14u - cd) % 7u);

        if (dist > 3u)
            dist = (uint8_t)(7u - dist);
        if (dist < best_dist) {
            best_dist = dist;
            best = cd;
        }
    }
    (void)sid;
    return best;
}

static inline const int8_t *pop_melody_row_at(uint8_t plate, uint8_t mbar, uint8_t s16)
{
    uint8_t phrase = (uint8_t)((mbar >> 2) & 1u);

    if (mbar >= 4u && mbar < 8u && s16 >= 12u && phrase == 0u)
        phrase = 2u;
    if (mbar >= 8u)
        phrase = (uint8_t)((mbar >> 1) & 1u);
    return phrase ? POP_MELO_B[plate] : POP_MELO_A[plate];
}

static inline uint8_t pop_melody_step_is_rest(uint8_t plate, uint8_t mbar, uint8_t s16)
{
    return pop_melody_row_at(plate, mbar, s16)[s16] < 0 ? 1u : 0u;
}

static inline int8_t pop_melody_row_deg(uint8_t plate, uint8_t mbar, uint8_t s16, int8_t *hold)
{
    const int8_t *row = pop_melody_row_at(plate, mbar, s16);
    int8_t v;

    v = row[s16];
    if (v < 0) {
        if (*hold >= 0)
            return *hold;
        return row[0] >= 0 ? row[0] : 0;
    }
    *hold = v;
    return v;
}

static inline int8_t pop_lead_melody_semi(uint8_t plate, uint8_t root, uint8_t scale, uint8_t mbar, uint8_t s16,
                                           int8_t *hold)
{
    uint8_t chord_deg = pop_chord_deg(plate, mbar);
    int8_t md = pop_melody_row_deg(plate, mbar, s16, hold);
    uint8_t strong = (uint8_t)((s16 & 3u) == 0u);

    if (strong)
        md = (int8_t)pop_nearest_chord_tone_deg(scale, chord_deg, (uint8_t)md);
    {
        uint8_t key_note = pop_deg_note(root, scale, (uint8_t)md);

        return (int8_t)((int32_t)key_note - (int32_t)root);
    }
}

enum {
    POP_BASS_REST = 0,
    POP_BASS_R = 1,
    POP_BASS_O = 2,
    POP_BASS_5 = 3,
    POP_BASS_3 = 4,
    POP_BASS_A = 5,
};

static inline uint8_t pop_bass_note_midi(uint8_t plate, uint8_t root, uint8_t scale, uint8_t mbar, uint8_t s16)
{
    uint8_t sym = POP_BASS_STEP[plate][s16];
    uint8_t chord_deg = pop_chord_deg(plate, mbar);
    uint8_t next_mbar = (uint8_t)(mbar + 1u);
    uint8_t next_deg = pop_chord_deg(plate, next_mbar);
    uint8_t base = pop_deg_note(root, scale, chord_deg);
    int32_t n;

    switch (sym) {
    case POP_BASS_O:
        n = (int32_t)base + 12;
        break;
    case POP_BASS_5:
        n = (int32_t)pop_deg_note(root, scale, (uint8_t)(chord_deg + 4u));
        break;
    case POP_BASS_3:
        n = (int32_t)pop_deg_note(root, scale, (uint8_t)(chord_deg + 2u));
        break;
    case POP_BASS_A:
        n = (int32_t)pop_deg_note(root, scale, next_deg);
        if (n > (int32_t)base)
            n -= 12;
        break;
    case POP_BASS_R:
    default:
        n = (int32_t)base;
        break;
    }
    if (sym == POP_BASS_REST)
        return 0u;
    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    return (uint8_t)n;
}

static inline void pop_chord_voices(uint8_t plate, uint8_t root, uint8_t scale, uint8_t mbar, uint8_t voicing,
                                    uint8_t out[DUB_CHORD_N])
{
    uint8_t chord_deg = pop_chord_deg(plate, mbar);
    uint8_t floor = pop_deg_note(root, scale, chord_deg);
    uint8_t d0 = chord_deg;
    uint8_t d1 = (uint8_t)(chord_deg + 2u);
    uint8_t d2 = (uint8_t)(chord_deg + 4u);
    uint8_t d3 = (uint8_t)(chord_deg + 6u);
    uint8_t i;
    uint8_t deg[4];

    voicing %= VOICE_N;
    if (voicing == 2u) {
        d1 = (uint8_t)(chord_deg + 4u);
        d2 = (uint8_t)(chord_deg + 7u);
        d3 = (uint8_t)(chord_deg + 11u);
    }
    deg[0] = d0;
    deg[1] = d1;
    deg[2] = d2;
    deg[3] = d3;
    if (voicing == 0u)
        deg[3] = d2;
    for (i = 0; i < DUB_CHORD_N; i++) {
        int32_t n = (int32_t)pop_deg_note(root, scale, deg[i]);

        while (n < (int32_t)floor)
            n += 12;
        while (n >= (int32_t)floor + 12)
            n -= 12;
        if (n < 0)
            n = 0;
        if (n > 127)
            n = 127;
        out[i] = (uint8_t)n;
    }
}

static inline int pop_arrange_mute(uint8_t plate, uint8_t era, uint8_t mbar, uint8_t lane, uint8_t views_on)
{
    if (views_on)
        return 0;
    era = pop_clamp_era(era);
    if (mbar < 2u) {
        if (lane == 3u)
            return 1;
    } else if (mbar < 4u) {
        /* lead on */
    } else if (mbar < 8u) {
        /* verse */
    } else if (mbar < 12u) {
        /* chorus full */
    } else if (mbar < 14u) {
        if (lane == 0u || lane == 4u)
            return 1;
        if (era == ERA_PAD && lane == 1u)
            return 1;
    } else {
        /* build */
    }
    if (era == ERA_TINE && mbar < 8u && lane == 3u)
        return 1;
    if (era == ERA_PAD && mbar >= 8u && lane == 1u)
        return 1;
    if ((era == ERA_BELL || era == ERA_STAB) && mbar >= 12u && mbar < 14u && lane == 0u && (mbar & 1u))
        return 0;
    return 0;
}

static inline uint8_t pop_lead_arrange_bar0(uint8_t plate, uint8_t era)
{
    uint8_t m;

    era = pop_clamp_era(era);
    for (m = 0u; m < 16u; m++) {
        if (!pop_arrange_mute(plate, era, m, 3u, 0u))
            return m;
    }
    return 8u;
}

static inline void pop_arrange_events(uint8_t plate, uint8_t era, uint8_t mbar, uint8_t views_on, dub_events_t *ev)
{
    uint8_t ln;

    if (!ev || views_on)
        return;
    for (ln = 0; ln < 6u; ln++) {
        if (!pop_arrange_mute(plate, era, mbar, ln, 0u))
            continue;
        if (ln == 0u)
            ev->kick = 0u;
        else if (ln == 1u)
            ev->rim = 0u;
        else if (ln == 2u)
            ev->hat = 0u;
        else if (ln == 3u)
            ev->siren = 0u;
        else if (ln == 4u)
            ev->sub = 0u;
        else if (ln == 5u)
            ev->chord = 0u;
    }
    if (mbar == 3u || mbar == 7u || mbar == 11u || mbar == 15u) {
        /* small fill: extra snare last 4 steps handled in fm_app fill mask */
    }
}

static inline uint8_t pop_fill_snare_extra(uint8_t mbar, uint8_t s16)
{
    if (mbar == 15u && s16 >= 8u)
        return 1u;
    if ((mbar == 3u || mbar == 7u || mbar == 11u) && s16 >= 12u)
        return 1u;
    return 0u;
}

static inline uint8_t pop_midi_in_scale(uint8_t root, uint8_t scale, int32_t midi)
{
    int32_t rel = midi - (int32_t)root;
    uint8_t sid = pop_scale_id(scale);
    uint8_t oct;
    uint8_t i;

    while (rel < 0)
        rel += 12;
    rel %= 12;
    for (i = 0u; i < 7u; i++) {
        if ((int32_t)POP_SCALE_SEMIS[sid][i] == rel)
            return 1u;
    }
    (void)oct;
    return 0u;
}

static inline int32_t pop_snap_scale_midi(uint8_t root, uint8_t scale, int32_t midi)
{
    int32_t d;

    if (pop_midi_in_scale(root, scale, midi))
        return midi;
    for (d = 1; d <= 6; d++) {
        if (pop_midi_in_scale(root, scale, midi + d))
            return midi + d;
        if (pop_midi_in_scale(root, scale, midi - d))
            return midi - d;
    }
    if (midi < 0)
        return 0;
    if (midi > 127)
        return 127;
    return midi;
}

static inline int32_t pop_snap_chord_midi(uint8_t root, uint8_t scale, uint8_t plate, uint8_t mbar,
                                           uint8_t voicing, int32_t midi)
{
    uint8_t tones[DUB_CHORD_N];
    uint8_t i;
    int32_t best = midi;
    int32_t best_d = 128;

    pop_chord_voices(plate, root, scale, mbar, voicing, tones);
    for (i = 0u; i < DUB_CHORD_N; i++) {
        int32_t d = midi - (int32_t)tones[i];

        if (d < 0)
            d = -d;
        if (d < best_d) {
            best_d = d;
            best = (int32_t)tones[i];
        }
    }
    return best;
}

static inline int32_t pop_lead_resolve_midi(const dub_params_t *p, uint8_t plate, int8_t st_tune, int8_t oct_lfo_semi,
                                            uint8_t apply_register_fix)
{
    int32_t sn = pop_lead_resolve_midi_core(p, plate, st_tune, oct_lfo_semi, apply_register_fix);

    if (apply_register_fix)
        sn = pop_snap_scale_midi((uint8_t)p->root, p->scale, sn);
    return sn;
}

static inline int32_t pop_lead_resolve_midi_step(const dub_params_t *p, uint8_t plate, uint8_t mbar, uint8_t s16,
                                                   int8_t st_tune, int8_t oct_lfo_semi, uint8_t apply_register_fix)
{
    int32_t sn = pop_lead_resolve_midi(p, plate, st_tune, oct_lfo_semi, apply_register_fix);

    if (apply_register_fix && (s16 & 3u) == 0u)
        sn = pop_snap_chord_midi((uint8_t)p->root, p->scale, plate, mbar, p->voicing % VOICE_N, sn);
    return sn;
}

static inline uint8_t pop_lead_play_midi(const dub_params_t *p, uint8_t plate, uint8_t mbar, uint8_t s16,
                                         int8_t st_tune, int8_t oct_lfo_semi)
{
    return (uint8_t)pop_lead_resolve_midi_step(p, plate, mbar, s16, st_tune, oct_lfo_semi, 1u);
}

#endif
