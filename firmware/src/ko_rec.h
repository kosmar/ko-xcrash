/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * Portable mono step REC core (RYTM bass semantics).
 *
 * Threading: all ko_rec_* run on the main thread; the app supplies play_step
 * from an ISR step counter (pass the step that just fired, not the next).
 *
 * Port notes (adapters only — no wiring here):
 * - RYTM: map ko_rec_writer_t to seq_ovr.bass_* + fm_seq_bit_put on bass_force_on/off;
 *   rec_map_step = st % seq_len(); call ko_rec_transport_step each seq step from tick;
 *   gate note_on only when rec_arm && playing && mode != KO_REC_MONITOR.
 * - BASS: same writer shape onto bass plate / seq override RAM; page_steps = FM_SEQ_PAGE_STEPS.
 * - DECK: writer targets deck_seq lane hits or synth REC lane; clamp wk to white-key index 0..15.
 * - JET: ko_rec_slot_for_note maps arbitrary MIDI notes onto 16 held slots (LRU).
 * - clear_step (REPLACE): zero note/gate/micro for that step before first note_on write.
 * - note_off write_note: gate finalize only — adapter should keep note/micro/legato if unchanged.
 */
#ifndef KO_REC_H
#define KO_REC_H

#include <stdint.h>

enum {
    KO_REC_OVERDUB = 0,
    KO_REC_REPLACE = 1,
    KO_REC_MONITOR = 2
};

enum {
    KO_REC_QUANT_FREE = 0,
    KO_REC_QUANT_SLOP = 1,
    KO_REC_QUANT_16 = 2
};

typedef struct {
    uint8_t mode;
    uint8_t quant;
    uint8_t bars; /* 0..3 → 1/2/4/8 bars loop hint */
} ko_rec_cfg_t;

typedef struct {
    uint8_t held[16];
    uint8_t note_st[16];
    uint8_t note_sub[16];
    uint8_t note_len[16];
    uint8_t note[16]; /* note at attack step (legato extend) */
    int8_t note_micro[16];
} ko_rec_voice_t;

typedef struct {
    void (*clear_step)(void *ctx, uint8_t st);
    void (*write_note)(void *ctx, uint8_t st, uint8_t note, uint8_t gate, int8_t micro, uint8_t legato);
    void (*snapshot)(void *ctx);
    void (*restore)(void *ctx);
    void (*set_len)(void *ctx, uint8_t steps);
    void (*erase_step)(void *ctx, uint8_t step);
} ko_rec_writer_t;

typedef struct {
    ko_rec_cfg_t cfg;
    ko_rec_voice_t voice;
    const ko_rec_writer_t *w;
    void *wctx;
    uint8_t armed;
    uint8_t playing;
    uint8_t undo_ready;
    uint8_t slot_midi[16];  /* 0 = empty slot */
    uint8_t slot_touch[16]; /* LRU stamp */
    uint8_t slot_gen;
} ko_rec_t;

static inline uint8_t ko_rec_wk_clamp(uint8_t wk)
{
    if (wk > 15u)
        return 15u;
    return wk;
}

static inline uint8_t ko_rec_seq_len_safe(uint8_t seq_len)
{
    return seq_len ? seq_len : 1u;
}

static inline uint8_t ko_rec_map_step(uint8_t st, uint8_t seq_len)
{
    uint8_t ln = ko_rec_seq_len_safe(seq_len);

    return (uint8_t)(st % ln);
}

static inline uint8_t ko_rec_gate_from_len(uint8_t len)
{
    if (len <= 1u)
        return 40u;
    if (len == 2u)
        return 55u;
    if (len <= 4u)
        return 75u;
    return 100u;
}

static inline uint8_t ko_rec_loop_steps(const ko_rec_cfg_t *cfg, uint8_t page_steps)
{
    static const uint8_t bar_mul[4] = {1u, 2u, 4u, 8u};
    uint8_t ix;

    if (!cfg)
        return page_steps;
    ix = cfg->bars > 3u ? 3u : cfg->bars;
    return (uint8_t)(bar_mul[ix] * page_steps);
}

static inline uint8_t ko_rec_quant_step(const ko_rec_cfg_t *cfg, uint8_t tick_mod6, uint8_t play_step,
                                        uint8_t seq_len)
{
    uint8_t ln = ko_rec_seq_len_safe(seq_len);

    if (!cfg || cfg->quant == KO_REC_QUANT_FREE)
        return play_step;
    if (tick_mod6 >= 4u)
        return (uint8_t)((play_step + 1u) % ln);
    return play_step;
}

static inline int8_t ko_rec_sub_micro(const ko_rec_cfg_t *cfg, uint8_t sub)
{
    int32_t m = ((int32_t)sub - 3) * 8;

    if (!cfg || cfg->quant == KO_REC_QUANT_16)
        return 0;
    if (cfg->quant == KO_REC_QUANT_SLOP)
        return (int8_t)(m / 2);
    return (int8_t)m;
}

static inline void ko_rec_voice_reset(ko_rec_voice_t *v)
{
    uint8_t i;

    if (!v)
        return;
    for (i = 0u; i < 16u; i++) {
        v->held[i] = 0u;
        v->note_st[i] = 0u;
        v->note_sub[i] = 0u;
        v->note_len[i] = 0u;
        v->note[i] = 0u;
        v->note_micro[i] = 0;
    }
}

static inline void ko_rec_init(ko_rec_t *r)
{
    uint8_t i;

    if (!r)
        return;
    r->cfg.mode = KO_REC_OVERDUB;
    r->cfg.quant = KO_REC_QUANT_16;
    r->cfg.bars = 0u;
    ko_rec_voice_reset(&r->voice);
    r->w = 0;
    r->wctx = 0;
    r->armed = 0u;
    r->playing = 0u;
    r->undo_ready = 0u;
    r->slot_gen = 0u;
    for (i = 0u; i < 16u; i++) {
        r->slot_midi[i] = 0u;
        r->slot_touch[i] = 0u;
    }
}

static inline uint8_t ko_rec_slot_for_note(ko_rec_t *r, uint8_t note)
{
    uint8_t i;
    uint8_t slot = 0u;
    uint8_t oldest = 255u;
    uint8_t oldest_ix = 0u;

    if (!r || !note)
        return 0u;
    for (i = 0u; i < 16u; i++) {
        if (r->slot_midi[i] == note) {
            r->slot_gen++;
            r->slot_touch[i] = r->slot_gen;
            return i;
        }
    }
    for (i = 0u; i < 16u; i++) {
        if (!r->slot_midi[i]) {
            r->slot_midi[i] = note;
            r->slot_gen++;
            r->slot_touch[i] = r->slot_gen;
            return i;
        }
    }
    for (i = 0u; i < 16u; i++) {
        if (r->slot_touch[i] < oldest) {
            oldest = r->slot_touch[i];
            oldest_ix = i;
        }
    }
    slot = oldest_ix;
    r->slot_midi[slot] = note;
    r->slot_gen++;
    r->slot_touch[slot] = r->slot_gen;
    return slot;
}

static inline void ko_rec_take_begin(ko_rec_t *r)
{
    if (!r || !r->w || !r->w->snapshot)
        return;
    if (!r->armed || !r->playing)
        return;
    r->w->snapshot(r->wctx);
    r->undo_ready = 1u;
}

static inline void ko_rec_take_undo(ko_rec_t *r)
{
    if (!r || !r->w || !r->w->restore || !r->undo_ready)
        return;
    r->w->restore(r->wctx);
    r->undo_ready = 0u;
}

static inline void ko_rec_erase(ko_rec_t *r, uint8_t step)
{
    if (!r || !r->w || !r->w->erase_step)
        return;
    r->w->erase_step(r->wctx, step);
}

static inline void ko_rec_auto_len(ko_rec_t *r, uint8_t bars, uint8_t cur_len)
{
    uint8_t want;
    uint16_t steps;

    if (!r || !r->w || !r->w->set_len || !bars)
        return;
    steps = (uint16_t)bars * 16u;
    if (steps > 128u)
        steps = 128u;
    want = (uint8_t)steps;
    if (cur_len >= want)
        return;
    r->w->set_len(r->wctx, want);
}

static inline void ko_rec_note_on(const ko_rec_cfg_t *cfg, ko_rec_voice_t *v, const ko_rec_writer_t *w, void *ctx,
                                  uint8_t wk, uint8_t note, uint8_t tick_mod6, uint8_t play_step, uint8_t seq_len)
{
    uint8_t k;
    uint8_t st;
    int8_t micro;

    if (!v)
        return;
    k = ko_rec_wk_clamp(wk);
    st = ko_rec_map_step(ko_rec_quant_step(cfg, tick_mod6, play_step, seq_len), seq_len);
    micro = ko_rec_sub_micro(cfg, tick_mod6);

    if (cfg && cfg->mode != KO_REC_MONITOR && w && w->write_note) {
        if (cfg->mode == KO_REC_REPLACE && w->clear_step)
            w->clear_step(ctx, st);
        w->write_note(ctx, st, note, 40u, micro, 0u);
    }

    v->held[k] = 1u;
    v->note_st[k] = st;
    v->note_sub[k] = tick_mod6;
    v->note_len[k] = 1u;
    v->note[k] = note;
    v->note_micro[k] = micro;
}

static inline void ko_rec_transport_step_cfg(const ko_rec_cfg_t *cfg, ko_rec_voice_t *v, const ko_rec_writer_t *w,
                                               void *ctx, uint8_t st)
{
    uint8_t k;

    if (!v)
        return;
    if (cfg && cfg->mode == KO_REC_MONITOR)
        return;
    for (k = 0u; k < 16u; k++) {
        if (!v->held[k])
            continue;
        v->note_len[k]++;
        if (st == v->note_st[k])
            continue;
        if (w && w->write_note)
            w->write_note(ctx, st, v->note[k], 100u, 0, 1u);
    }
}

static inline void ko_rec_transport_step(ko_rec_voice_t *v, const ko_rec_writer_t *w, void *ctx, uint8_t st)
{
    ko_rec_transport_step_cfg(0, v, w, ctx, st);
}

static inline void ko_rec_note_off(ko_rec_voice_t *v, const ko_rec_writer_t *w, void *ctx, uint8_t wk)
{
    uint8_t k;
    uint8_t st;
    uint8_t gate;

    if (!v)
        return;
    k = ko_rec_wk_clamp(wk);
    if (!v->held[k])
        return;
    st = v->note_st[k];
    gate = ko_rec_gate_from_len(v->note_len[k]);
    if (w && w->write_note)
        w->write_note(ctx, st, v->note[k], gate, v->note_micro[k], 0u);
    v->held[k] = 0u;
}

#endif /* KO_REC_H */
