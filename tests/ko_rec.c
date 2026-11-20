/* SPDX-License-Identifier: GPL-3.0-only */
#include "jet_seq.h"
#include "ko_rec.h"
#include <stdio.h>
#include <string.h>

static int fail;

typedef struct {
    unsigned clears;
    unsigned writes;
    unsigned snapshots;
    unsigned restores;
    unsigned set_lens;
    unsigned erases;
    uint8_t last_st;
    uint8_t last_note;
    uint8_t last_gate;
    int8_t last_micro;
    uint8_t last_legato;
    uint8_t cleared_st;
    uint8_t len_set;
    uint8_t erased_st;
    jet_seq_t snap;
    jet_seq_t live;
} log_t;

static void mock_clear(void *ctx, uint8_t st)
{
    log_t *l = (log_t *)ctx;

    l->clears++;
    l->cleared_st = st;
}

static void mock_write(void *ctx, uint8_t st, uint8_t note, uint8_t gate, int8_t micro, uint8_t legato)
{
    log_t *l = (log_t *)ctx;

    l->writes++;
    l->last_st = st;
    l->last_note = note;
    l->last_gate = gate;
    l->last_micro = micro;
    l->last_legato = legato;
}

static void mock_snapshot(void *ctx)
{
    log_t *l = (log_t *)ctx;

    l->snapshots++;
    jet_seq_copy(&l->snap, &l->live);
}

static void mock_restore(void *ctx)
{
    log_t *l = (log_t *)ctx;

    l->restores++;
    jet_seq_copy(&l->live, &l->snap);
}

static void mock_set_len(void *ctx, uint8_t steps)
{
    log_t *l = (log_t *)ctx;

    l->set_lens++;
    l->len_set = steps;
    jet_seq_set_len(&l->live, steps);
}

static void mock_erase(void *ctx, uint8_t st)
{
    log_t *l = (log_t *)ctx;

    l->erases++;
    l->erased_st = st;
    jet_seq_clear_step(&l->live, st);
}

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "ko_rec: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    ko_rec_cfg_t cfg;
    ko_rec_voice_t v;
    ko_rec_writer_t w;
    ko_rec_t rec;
    log_t log;

    fail = 0;
    ko_rec_voice_reset(&v);
    ko_rec_init(&rec);
    jet_seq_default_init(&log.live);
    jet_seq_default_init(&log.snap);
    memset(&log, 0, sizeof(log));
    jet_seq_default_init(&log.live);
    jet_seq_default_init(&log.snap);
    w.clear_step = mock_clear;
    w.write_note = mock_write;
    w.snapshot = mock_snapshot;
    w.restore = mock_restore;
    w.set_len = mock_set_len;
    w.erase_step = mock_erase;

    rec.w = &w;
    rec.wctx = &log;

    cfg.mode = KO_REC_OVERDUB;
    cfg.quant = KO_REC_QUANT_16;
    cfg.bars = 2u;
    rec.cfg = cfg;

    check(ko_rec_loop_steps(&cfg, 16u) == 64u, "4 bars × 16 steps");
    check(ko_rec_quant_step(&cfg, 5u, 3u, 8u) == 4u, "quant16 late sub → next step");
    check(ko_rec_quant_step(&cfg, 2u, 3u, 8u) == 3u, "quant16 early sub → play step");

    cfg.quant = KO_REC_QUANT_FREE;
    check(ko_rec_quant_step(&cfg, 5u, 7u, 8u) == 7u, "FREE ignores sub phase");

    cfg.quant = KO_REC_QUANT_SLOP;
    check(ko_rec_sub_micro(&cfg, 3u) == 0, "sub 3 → micro 0 slop");
    check(ko_rec_sub_micro(&cfg, 0u) == -12, "sub 0 slop halved");
    cfg.quant = KO_REC_QUANT_FREE;
    check(ko_rec_sub_micro(&cfg, 0u) == -24, "sub 0 free full");
    cfg.quant = KO_REC_QUANT_16;
    check(ko_rec_sub_micro(&cfg, 0u) == 0, "quant16 micro always 0");

    cfg.quant = KO_REC_QUANT_16;
    rec.cfg = cfg;
    ko_rec_note_on(&cfg, &v, &w, &log, 0u, 60u, 2u, 5u, 8u);
    check(log.writes == 1u, "overdub note_on writes");
    check(log.last_gate == 40u && log.last_legato == 0u, "note_on gate 40");
    check(log.last_st == 5u, "note_on writes fired play_step (no +1)");
    check(v.held[0] == 1u && v.note_st[0] == 5u, "voice state on attack step");

    cfg.mode = KO_REC_MONITOR;
    rec.cfg = cfg;
    log.writes = 0u;
    ko_rec_note_on(&cfg, &v, &w, &log, 1u, 62u, 0u, 1u, 8u);
    check(log.writes == 0u, "monitor note_on silent to writer");
    check(v.held[1] == 1u, "monitor still tracks held");

    cfg.mode = KO_REC_REPLACE;
    rec.cfg = cfg;
    log.clears = 0u;
    log.writes = 0u;
    ko_rec_note_on(&cfg, &v, &w, &log, 2u, 64u, 4u, 2u, 8u);
    check(log.clears == 1u && log.cleared_st == 3u, "replace clears quantized step");
    check(log.writes == 1u, "replace then write");

    cfg.mode = KO_REC_OVERDUB;
    rec.cfg = cfg;
    log.writes = 0u;
    ko_rec_transport_step(&v, &w, &log, 4u);
    check(v.note_len[0] == 2u, "transport bumps len on step 0");
    check(log.last_st == 4u && log.last_legato == 1u && log.last_gate == 100u, "legato extend");

    cfg.mode = KO_REC_MONITOR;
    rec.cfg = cfg;
    log.writes = 0u;
    ko_rec_transport_step_cfg(&cfg, &v, &w, &log, 5u);
    check(log.writes == 0u, "monitor transport_step is no-op");

    cfg.mode = KO_REC_OVERDUB;
    rec.cfg = cfg;
    cfg.quant = KO_REC_QUANT_FREE;
    rec.cfg = cfg;
    ko_rec_voice_reset(&v);
    log.writes = 0u;
    ko_rec_note_on(&cfg, &v, &w, &log, 0u, 60u, 0u, 7u, 16u);
    check(log.last_st == 7u, "FREE quant maps play_step 7 → write st 7");
    check(log.last_micro == -24, "FREE micro preserved for note_off");
    v.note_len[0] = 3u;
    ko_rec_note_off(&v, &w, &log, 0u);
    check(log.last_micro == -24, "note_off preserves attack micro");
    check(log.last_legato == 0u, "note_off clears legato flag on write");

    ko_rec_note_on(&cfg, &v, &w, &log, 99u, 70u, 0u, 0u, 8u);
    check(v.held[15] == 1u, "wk clamp 99 → 15");

    rec.armed = 0u;
    rec.playing = 1u;
    ko_rec_take_begin(&rec);
    check(log.snapshots == 0u, "take_begin skips when not armed");
    rec.armed = 1u;
    ko_rec_take_begin(&rec);
    check(log.snapshots == 1u && rec.undo_ready == 1u, "take_begin snapshots when armed+playing");
    log.live.note[0] = 99u;
    ko_rec_take_undo(&rec);
    check(log.restores == 1u && rec.undo_ready == 0u, "take_undo restores snapshot");
    check(log.live.note[0] != 99u, "undo rolled back live seq");

    jet_seq_set_len(&log.live, 16u);
    ko_rec_auto_len(&rec, 4u, 16u);
    check(log.set_lens == 1u && log.len_set == 64u, "auto_len extends to 4×16");
    ko_rec_auto_len(&rec, 2u, 64u);
    check(log.set_lens == 1u, "auto_len no-op when already long enough");

    ko_rec_erase(&rec, 3u);
    check(log.erases == 1u && log.erased_st == 3u, "erase forwards step to writer");

    ko_rec_init(&rec);
    check(ko_rec_slot_for_note(&rec, 60u) == 0u, "slot first note → 0");
    check(ko_rec_slot_for_note(&rec, 72u) == 1u, "slot second note → 1");
    check(ko_rec_slot_for_note(&rec, 60u) == 0u, "same note reuses slot");
    {
        uint8_t k;
        ko_rec_init(&rec);
        for (k = 0u; k < 16u; k++)
            (void)ko_rec_slot_for_note(&rec, (uint8_t)(60u + k));
        check(ko_rec_slot_for_note(&rec, 127u) == 0u, "LRU evicts oldest slot 0");
        check(rec.slot_midi[0] == 127u, "evicted slot holds new note");
    }

    check(jet_seq_pages(&log.live) == 4u, "jet_seq_pages ceil(64/16)");
    jet_seq_clear_step(&log.live, 0u);
    check(log.live.pl_lpf[0] == FM_SEQ_PLOCK_NONE, "clear_step resets plocks");

    return fail ? 1 : 0;
}
