/* SPDX-License-Identifier: GPL-3.0-only */
/* KO DECK pattern RAM: fm_seq bit grid + per-lane length/div + accent mask. */
#ifndef DECK_SEQ_H
#define DECK_SEQ_H

#include <stdint.h>
#include "deck_kit.h"
#include "deck_plates.h"
#include "deck_seq_grooves.h"
#include "fm_seq.h"

enum {
    DECK_NPAT = 16,
    DECK_TRIG_Q_MAX = 24,
    DECK_DUP_ALL = 255u
};

enum {
    DECK_PAGE_KIT = 0,
    DECK_PAGE_SYNTH = 1
};

typedef struct {
    uint8_t hit[FM_SEQ_MASK_BYTES];
    uint8_t accent[FM_SEQ_MASK_BYTES];     /* (accent,cut): 00 OFF, 01 33, 11 66, 10 100 (deck_seq_get_accent) */
    uint8_t accent_cut[FM_SEQ_MASK_BYTES];
    uint8_t length;
    uint8_t div;
    int8_t vel_rng;   /* OXI-style vel variance ±50 (0 = off) */
    int8_t trig;      /* trigger %: raw 0 = 100%, -100 = 0% */
    int8_t gate_rng;  /* gate length variance ±50 */
    int8_t pitch_rng; /* semitone variance ±24 */
    uint8_t euclid_hits;    /* seq mode: 1..16 EUCLID n, or FM_SEQ_EUCLID_SONG/FRESH/EMPTY/EDITED (fm_seq.h) */
    uint8_t pattern_origin; /* mode to return to from EDITED; FM_SEQ_PATTERN_ORIGIN_NONE = none */
    int8_t euclid_rot;      /* EUCLID rotation, normalized by fm_seq_euclid_rot_norm */
    uint8_t vary;           /* VARY 0..127 (fm_seq_vary_hit at playback); 0 = off */
} deck_lane_t;

typedef struct {
    deck_lane_t lane[DECK_NVOICES];
    char name[9];
} deck_pattern_t;

typedef struct {
    deck_pattern_t p[DECK_NPAT];
} deck_bank_t;

typedef struct {
    uint8_t playing;
    uint8_t cur_pat;
    uint8_t cur_voice;
    uint8_t bar_page;
    uint8_t lane_page; /* SEQ grid: voices [lane_page*8 .. +7] */
    uint8_t bpm;
    uint8_t swing;
    uint8_t accent_hold;
    uint8_t key;   /* KIT GLO KEY 0..11; persisted via deck_meta_t */
    uint8_t scale; /* KIT GLO SCALE 0..DECK_SCALE_N-1; persisted via deck_meta_t */
    uint8_t rec_arm; /* latched: REC short tap toggles; survives SEQ/bank/voice/transport */
    uint8_t page_synth;
    uint8_t lane_mute[DECK_NVOICES];
    uint16_t solo_mask;
    uint32_t master_step;
    uint32_t step_phase;
    uint8_t play_col;
    uint8_t seq_clk; /* FM_SEQ_CLK_RATIO index: global master 16th vs BPM grid */
    uint8_t _pad[2];
} deck_runtime_t;

typedef struct {
    uint8_t voice;
    uint8_t vel;
    uint8_t accent;
    uint8_t seq_step; /* rt->master_step when queued (pre master_tick on 16th boundary) */
    uint8_t gate;
    uint8_t slide;
    uint8_t ratchet;
    uint16_t step_samples; /* 16th length in samples at fire time (gate + ratchet) */
    int8_t semi;           /* transpose: pitch_rng roll + P-lock pitch, ±DECK_SEQ_SEMI_MAX */
} deck_trig_evt_t;

enum {
    DECK_SEQ_GATE_TIE = 8u,
    DECK_SEQ_GATE_DEF = 4u,
    DECK_SEQ_RATCHET_MIN = 1u,
    DECK_SEQ_RATCHET_MAX = 4u,
    DECK_SEQ_SEMI_MAX = 24u,
    DECK_SX_SLIDE = 0x10u,
    DECK_SX_RATCH_SHIFT = 5u,
    DECK_SX_SLIDE_CUT = 0x80u, /* same 2-bit level code as accent/accent_cut */
    DECK_SEQ_LV_FULL = 3u      /* ACCENT/SLIDE step level: 0 OFF, 1 = 33, 2 = 66, 3 = 100 */
};

/* Sparse step p-locks (pat+voice+step key); slot count trimmed for per-lane PROB fields in deck_lane_t. */
#define DECK_PLOCK_SLOTS 216u

typedef struct {
    uint16_t tag; /* 0 = free; else (pat<<11)|(voice<<7)|step */
    uint8_t vel;
    uint8_t cond;
    int8_t micro;
    int8_t pitch;
    uint8_t step_x; /* gate nibble 0..8, slide bit4 (+cut bit7), ratchet-1 in bits5-6; 0 = no step overrides */
} deck_plock_ent_t;

typedef struct {
    deck_plock_ent_t ent[DECK_PLOCK_SLOTS];
} deck_plock_bank_t;

typedef struct {
    deck_bank_t bank;
    deck_runtime_t rt;
    deck_pattern_t paste;
    uint8_t paste_valid;
    uint8_t _pad[3];
    uint8_t lane_step[DECK_NVOICES];
    uint8_t lane_clk_acc[DECK_NVOICES];
    uint16_t micro_left[DECK_NVOICES];
    uint8_t micro_vel[DECK_NVOICES];
    uint8_t micro_accent[DECK_NVOICES];
    int8_t micro_semi[DECK_NVOICES];
    uint8_t micro_gate[DECK_NVOICES];
    uint8_t micro_slide[DECK_NVOICES];
    uint8_t micro_rat[DECK_NVOICES];
    uint16_t micro_sps[DECK_NVOICES];
    uint16_t micro_armed;
    uint16_t micro_early; /* lookahead armed the next lane step early (negative micro) */
    uint8_t ratchet_left[DECK_NVOICES];
    uint16_t ratchet_every[DECK_NVOICES];
    uint16_t ratchet_acc[DECK_NVOICES];
    uint8_t ratchet_vel[DECK_NVOICES];
    uint8_t ratchet_accent[DECK_NVOICES];
    int8_t ratchet_semi[DECK_NVOICES];
    deck_plock_bank_t plock;
    uint32_t vary_base; /* boot seed for VARY; per-bar seed derives from it (deck_seq_vary_seed) */
    int8_t lane_nudge[DECK_NPAT][DECK_NVOICES]; /* SEQ p2 NUDGE per lane (row 0 authoritative, mirrored into all rows), FM_SEQ_LANE_NUDGE_MIN..MAX; saved in DKZ7 */
    uint8_t lane_all; /* LANE knob (SEQ p5) on ALL: NUDGE edit hits every lane of the pattern; runtime only */
    uint8_t timing_hum; /* KIT GLO HUMN T: per-hit timing jitter 0..127; runtime only (not in DKZ7) */
} deck_seq_state_t;

static deck_seq_state_t deck_seq_g;

/* DJ lane overlay (deck_dj.h, refreshed in app_step; 0 = off). vel/gate/pitch add to the lane value, trig is raw -100..0. */
typedef struct {
    int8_t vel[DECK_NVOICES];
    int8_t gate[DECK_NVOICES];
    int8_t pitch[DECK_NVOICES];
    int8_t trig[DECK_NVOICES];
    uint16_t flam; /* voice mask: unratcheted hits play as 2x ratchet (DJ FLAM approximation); pattern untouched */
} deck_seq_dj_ov_t;
static deck_seq_dj_ov_t deck_seq_djo;

static inline int8_t deck_seq_djo_trig_raw(int8_t raw, uint8_t voice)
{
    int32_t v;

    if (!deck_seq_djo.trig[voice])
        return raw;
    v = raw > 0 ? 0 : (int32_t)raw;
    v += deck_seq_djo.trig[voice];
    if (v < -100)
        v = -100;
    if (v > 0)
        v = 0;
    return (int8_t)v;
}

static inline deck_pattern_t *deck_seq_cur_pattern(void)
{
    return &deck_seq_g.bank.p[deck_seq_g.rt.cur_pat % DECK_NPAT];
}

static inline deck_runtime_t *deck_seq_rt(void)
{
    return &deck_seq_g.rt;
}

static inline uint8_t deck_seq_lane_is_solo(const deck_runtime_t *rt, uint8_t lane)
{
    if (!rt || lane >= DECK_NVOICES)
        return 0u;
    return (rt->solo_mask & (uint16_t)(1u << lane)) != 0u ? 1u : 0u;
}

static inline uint8_t deck_seq_lane_audible(const deck_runtime_t *rt, uint8_t lane)
{
    if (!rt || lane >= DECK_NVOICES)
        return 0u;
    if (rt->solo_mask)
        return deck_seq_lane_is_solo(rt, lane);
    if (rt->lane_mute[lane])
        return 0u;
    return 1u;
}

static inline uint8_t deck_seq_lane_state_ix(const deck_runtime_t *rt, uint8_t lane)
{
    if (!rt || lane >= DECK_NVOICES)
        return (uint8_t)FM_SEQ_STATE_PLAY;
    if (deck_seq_lane_is_solo(rt, lane))
        return (uint8_t)FM_SEQ_STATE_SOLO;
    if (rt->lane_mute[lane])
        return (uint8_t)FM_SEQ_STATE_MUTE;
    return (uint8_t)FM_SEQ_STATE_PLAY;
}

static inline void deck_lane_unmute_all(deck_runtime_t *rt)
{
    uint8_t v;

    if (!rt)
        return;
    for (v = 0u; v < DECK_NVOICES; v++)
        rt->lane_mute[v] = 0u;
}

static inline void deck_seq_lane_apply_state(deck_runtime_t *rt, uint8_t lane, uint8_t state)
{
    uint16_t bit;

    if (!rt || lane >= DECK_NVOICES)
        return;
    bit = (uint16_t)(1u << lane);
    if (state == (uint8_t)FM_SEQ_STATE_PLAY) {
        if (rt->solo_mask & bit) {
            rt->solo_mask &= (uint16_t)~bit;
            if (rt->solo_mask == 0u)
                deck_lane_unmute_all(rt);
            else
                rt->lane_mute[lane] = 1u;
        } else {
            rt->lane_mute[lane] = 0u;
            if (rt->solo_mask)
                rt->solo_mask |= bit;
        }
        return;
    }
    if (state == (uint8_t)FM_SEQ_STATE_MUTE) {
        rt->lane_mute[lane] = 1u;
        rt->solo_mask &= (uint16_t)~(1u << lane);
        return;
    }
    if (state == (uint8_t)FM_SEQ_STATE_SOLO) {
        uint8_t v;

        rt->solo_mask = bit;
        for (v = 0u; v < DECK_NVOICES; v++)
            rt->lane_mute[v] = (v == lane) ? 0u : 1u;
    }
}

/* 3-slice pie: 0 = MUTE (low), 1 = PLAY (mid), 2 = SOLO (high) — same as SEQ P3 / FM_SEQ_P3_STATE_NAMES. */
static inline uint8_t deck_lane_p3_state_pos(const deck_runtime_t *rt, uint8_t lane)
{
    if (!rt || lane >= DECK_NVOICES)
        return 1u;
    if (deck_seq_lane_is_solo(rt, lane))
        return 2u;
    if (rt->lane_mute[lane])
        return 0u;
    return 1u;
}

static inline uint8_t deck_lane_p3_bar_px(uint8_t pos)
{
    pos %= 3u;
    return pos == 0u ? 0u : pos == 1u ? 64u : 127u;
}

static inline void deck_lane_apply_p3_pos(deck_runtime_t *rt, uint8_t lane, uint8_t pos)
{
    uint8_t v;
    uint16_t bit;

    if (!rt || lane >= DECK_NVOICES)
        return;
    pos %= 3u;
    bit = (uint16_t)(1u << lane);
    if (pos == 0u) {
        rt->solo_mask &= (uint16_t)~bit;
        rt->lane_mute[lane] = 1u;
        return;
    }
    if (pos == 1u) {
        if (rt->solo_mask & bit) {
            rt->solo_mask &= (uint16_t)~bit;
            if (rt->solo_mask == 0u)
                deck_lane_unmute_all(rt);
            else
                rt->lane_mute[lane] = 1u;
        } else {
            rt->lane_mute[lane] = 0u;
            if (rt->solo_mask)
                rt->solo_mask |= bit;
        }
        return;
    }
    rt->solo_mask = bit;
    for (v = 0u; v < DECK_NVOICES; v++)
        rt->lane_mute[v] = (v == lane) ? 0u : 1u;
}

static inline void deck_lane_clear(deck_lane_t *ln)
{
    if (!ln)
        return;
    fm_seq_bits_fill(ln->hit, 0);
    fm_seq_bits_fill(ln->accent, 0);
    fm_seq_bits_fill(ln->accent_cut, 0);
    ln->length = (uint8_t)FM_SEQ_PAGE_STEPS;
    ln->div = FM_SEQ_CLK_RATIO_ONE;
    ln->vel_rng = 0;
    ln->trig = 0;
    ln->gate_rng = 0;
    ln->pitch_rng = 0;
    ln->euclid_hits = FM_SEQ_EUCLID_EDITED; /* existing content stays manual */
    ln->pattern_origin = FM_SEQ_PATTERN_ORIGIN_NONE;
    ln->euclid_rot = 0;
    ln->vary = 0u;
}

static inline void deck_plock_clear_pat(uint8_t pat)
{
    uint32_t i;
    uint16_t mask = (uint16_t)((uint16_t)(pat % DECK_NPAT) << 11);

    for (i = 0u; i < DECK_PLOCK_SLOTS; i++) {
        if ((deck_seq_g.plock.ent[i].tag & (uint16_t)0xF800u) == mask)
            deck_seq_g.plock.ent[i].tag = 0u;
    }
}

static inline void deck_seq_clear_pat(deck_pattern_t *pat)
{
    uint8_t v;

    if (!pat)
        return;
    for (v = 0u; v < DECK_NVOICES; v++)
        deck_lane_clear(&pat->lane[v]);
    pat->name[0] = '\0';
}

static inline void deck_seq_clear_pat_ix(uint8_t pat_ix, deck_pattern_t *pat)
{
    deck_seq_clear_pat(pat);
    deck_plock_clear_pat(pat_ix);
}

static inline void deck_seq_defaults(deck_pattern_t *pat)
{
    deck_seq_clear_pat(pat);
}

static inline void deck_seq_seed_house(deck_pattern_t *p)
{
    deck_lane_t *ln;
    uint8_t s;

    if (!p)
        return;
    deck_seq_clear_pat(p);

    ln = &p->lane[DECK_BD];
    ln->length = fm_seq_lane_len_clamp(16u);
    for (s = 0u; s < 16u; s += 4u)
        fm_seq_bit_put(ln->hit, s, 1);
    fm_seq_bit_put(ln->accent, 0u, 1);
    fm_seq_bit_put(ln->accent, 8u, 1);

    ln = &p->lane[DECK_CP];
    ln->length = fm_seq_lane_len_clamp(16u);
    fm_seq_bit_put(ln->hit, 4u, 1);
    fm_seq_bit_put(ln->accent, 4u, 1);
    fm_seq_bit_put(ln->hit, 12u, 1);
    fm_seq_bit_put(ln->accent, 12u, 1);

    ln = &p->lane[DECK_CH];
    ln->length = fm_seq_lane_len_clamp(16u);
    for (s = 2u; s < 16u; s += 4u)
        fm_seq_bit_put(ln->hit, s, 1);
    for (s = 3u; s < 16u; s += 4u)
        fm_seq_bit_put(ln->hit, s, 1);

    ln = &p->lane[DECK_OH];
    ln->length = fm_seq_lane_len_clamp(16u);
    fm_seq_bit_put(ln->hit, 14u, 1);

    ln = &p->lane[DECK_RM];
    ln->length = fm_seq_lane_len_clamp(16u);
    for (s = 3u; s < 16u; s += 4u)
        fm_seq_bit_put(ln->hit, s, 1);

    p->name[0] = 'H';
    p->name[1] = 'O';
    p->name[2] = 'U';
    p->name[3] = 'S';
    p->name[4] = 'E';
    p->name[5] = '\0';
}

/* Era + plate slot → factory groove (PRESETS / deck_seq_sync_baked). */
static inline void deck_seq_lane_fill(deck_lane_t *ln, uint8_t len, uint16_t hits, uint16_t acc)
{
    uint8_t s;

    if (!ln)
        return;
    ln->length = fm_seq_lane_len_clamp(len);
    ln->div = FM_SEQ_CLK_RATIO_ONE;
    for (s = 0u; s < FM_SEQ_MASK_BYTES; s++)
        ln->hit[s] = ln->accent[s] = ln->accent_cut[s] = 0u;
    for (s = 0u; s < len && s < 16u; s++) {
        if (hits & (1u << s))
            fm_seq_bit_put(ln->hit, s, 1);
        if (acc & (1u << s))
            fm_seq_bit_put(ln->accent, s, 1);
    }
}

/* Pages > 0 survive a plate bake: the groove only writes page 1 (steps 0..15). Snapshot before clear. */
static deck_lane_t deck_bake_keep[DECK_NVOICES];

static inline void deck_seq_bake(deck_pattern_t *pat, uint8_t era, uint8_t fader)
{
    const deck_seq_groove_t *g;
    const char *en;
    uint8_t slot;
    uint8_t i;

    if (!pat)
        return;
    era = deck_clamp_era(era);
    slot = (uint8_t)(fader % DECK_PLATES_PER_ERA);
    g = &DECK_SEQ_GROOVE[era][slot];
    for (i = 0u; i < DECK_NVOICES; i++)
        deck_bake_keep[i] = pat->lane[i];
    deck_seq_clear_pat(pat);

    deck_seq_lane_fill(&pat->lane[DECK_BD], 16u, g->bd, g->bd_acc ? g->bd_acc : (uint16_t)(g->bd & 0x0101u));
    if (g->sn)
        deck_seq_lane_fill(&pat->lane[DECK_SN], 16u, g->sn, g->sn);
    if (g->cp)
        deck_seq_lane_fill(&pat->lane[DECK_CP], 16u, g->cp, g->cp);
    if (g->ch)
        deck_seq_lane_fill(&pat->lane[DECK_CH], 16u, g->ch, 0u);
    if (g->oh)
        deck_seq_lane_fill(&pat->lane[DECK_OH], 16u, g->oh, g->oh);
    if (g->rm)
        deck_seq_lane_fill(&pat->lane[DECK_RM], 16u, g->rm, 0u);
    if (g->rd)
        deck_seq_lane_fill(&pat->lane[DECK_RD], 16u, g->rd, 0u);
    {
        uint16_t cb, tb, cw, tm, th, tl, cnh, cnl, crash;

        deck_seq_groove_perc_resolve(g, era, slot, &cb, &tb, &cw, &tm, &th, &tl, &cnh, &cnl, &crash);
        if (cb)
            deck_seq_lane_fill(&pat->lane[DECK_CB], 16u, cb, 0u);
        if (tb)
            deck_seq_lane_fill(&pat->lane[DECK_TB], 16u, tb, 0u);
        if (cw)
            deck_seq_lane_fill(&pat->lane[DECK_CW], 16u, cw, 0u);
        if (tm)
            deck_seq_lane_fill(&pat->lane[DECK_TM], 16u, tm, 0u);
        if (th)
            deck_seq_lane_fill(&pat->lane[DECK_TH], 16u, th, 0u);
        if (tl)
            deck_seq_lane_fill(&pat->lane[DECK_TL], 16u, tl, 0u);
        if (cnh)
            deck_seq_lane_fill(&pat->lane[DECK_CNH], 16u, cnh, 0u);
        if (cnl)
            deck_seq_lane_fill(&pat->lane[DECK_CNL], 16u, cnl, 0u);
        if (crash)
            deck_seq_lane_fill(&pat->lane[DECK_CR], 16u, crash, 0u);
    }

    /* Restore page 1+ content and length of every lane that had pages beyond 16. */
    for (i = 0u; i < DECK_NVOICES; i++) {
        deck_lane_t *ln = &pat->lane[i];
        const deck_lane_t *k = &deck_bake_keep[i];
        uint32_t s;

        if (k->length <= FM_SEQ_PAGE_STEPS)
            continue;
        ln->length = k->length;
        for (s = FM_SEQ_PAGE_STEPS; s < k->length; s++) {
            fm_seq_bit_put(ln->hit, s, (int)fm_seq_bit(k->hit, s));
            fm_seq_bit_put(ln->accent, s, (int)fm_seq_bit(k->accent, s));
            fm_seq_bit_put(ln->accent_cut, s, (int)fm_seq_bit(k->accent_cut, s));
        }
    }

    en = deck_era_name(era);
    for (i = 0u; i < 8u && en[i]; i++)
        pat->name[i] = en[i];
    pat->name[i < 8u ? i : 7u] = '\0';
}

/* Per-lane SONG: rewrites only `voice` from the era/plate groove (same voice→mask map as deck_seq_bake).
 * Length 16, div ONE. Accent/mode of the other lanes untouched. */
static inline void deck_seq_bake_lane(deck_pattern_t *pat, uint8_t voice, uint8_t era, uint8_t fader)
{
    const deck_seq_groove_t *g;
    uint16_t hits = 0u;
    uint16_t acc = 0u;
    uint8_t slot;

    if (!pat || voice >= DECK_NVOICES)
        return;
    era = deck_clamp_era(era);
    slot = (uint8_t)(fader % DECK_PLATES_PER_ERA);
    g = &DECK_SEQ_GROOVE[era][slot];
    switch (voice) {
    case DECK_BD:
        hits = g->bd;
        acc = g->bd_acc ? g->bd_acc : (uint16_t)(g->bd & 0x0101u);
        break;
    case DECK_SN:
        hits = acc = g->sn;
        break;
    case DECK_CP:
        hits = acc = g->cp;
        break;
    case DECK_CH:
        hits = g->ch;
        break;
    case DECK_OH:
        hits = acc = g->oh;
        break;
    case DECK_RM:
        hits = g->rm;
        break;
    case DECK_RD:
        hits = g->rd;
        break;
    default: {
        uint16_t cb, tb, cw, tm, th, tl, cnh, cnl, crash;

        deck_seq_groove_perc_resolve(g, era, slot, &cb, &tb, &cw, &tm, &th, &tl, &cnh, &cnl, &crash);
        switch (voice) {
        case DECK_CB: hits = cb; break;
        case DECK_TB: hits = tb; break;
        case DECK_CW: hits = cw; break;
        case DECK_TM: hits = tm; break;
        case DECK_TH: hits = th; break;
        case DECK_TL: hits = tl; break;
        case DECK_CNH: hits = cnh; break;
        case DECK_CNL: hits = cnl; break;
        case DECK_CR: hits = crash; break;
        default: break;
        }
        break;
    }
    }
    deck_seq_lane_fill(&pat->lane[voice], 16u, hits, acc);
}

static inline void deck_seq_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;

    while (n--)
        *b++ = 0u;
}

static inline uint16_t deck_plock_tag(uint8_t pat, uint8_t voice, uint8_t step)
{
    step %= (uint8_t)FM_SEQ_STEPS;
    return (uint16_t)(((uint16_t)(pat % DECK_NPAT) << 11) | ((uint16_t)(voice % DECK_NVOICES) << 7) | step);
}

static inline deck_plock_ent_t *deck_plock_find(uint8_t pat, uint8_t voice, uint8_t step)
{
    uint16_t want = deck_plock_tag(pat, voice, step);
    uint32_t i;

    for (i = 0u; i < DECK_PLOCK_SLOTS; i++) {
        if (deck_seq_g.plock.ent[i].tag == want)
            return &deck_seq_g.plock.ent[i];
    }
    return NULL;
}

static inline deck_plock_ent_t *deck_plock_alloc(uint8_t pat, uint8_t voice, uint8_t step)
{
    deck_plock_ent_t *e = deck_plock_find(pat, voice, step);
    uint32_t i;

    if (e)
        return e;
    for (i = 0u; i < DECK_PLOCK_SLOTS; i++) {
        if (deck_seq_g.plock.ent[i].tag == 0u) {
            e = &deck_seq_g.plock.ent[i];
            e->tag = deck_plock_tag(pat, voice, step);
            e->vel = 0u;
            e->cond = 0u;
            e->micro = 0;
            e->pitch = 0;
            e->step_x = 0u;
            return e;
        }
    }
    return NULL;
}

static inline void deck_plock_free_if_default(deck_plock_ent_t *e)
{
    if (!e || !e->tag)
        return;
    if (e->vel == 0u && e->cond == 0u && e->micro == 0 && e->pitch == 0 && e->step_x == 0u)
        e->tag = 0u;
}

/* ACCENT/SLIDE level 0..3 -> lo + (hi - lo) * lv / 3 (hi at DECK_SEQ_LV_FULL). */
static inline int32_t deck_seq_lv_lerp(uint8_t lv, int32_t lo, int32_t hi)
{
    if (lv > DECK_SEQ_LV_FULL)
        lv = DECK_SEQ_LV_FULL;
    return lo + ((hi - lo) * (int32_t)lv) / (int32_t)DECK_SEQ_LV_FULL;
}

static inline uint8_t deck_plock_vel(uint8_t pat, uint8_t voice, uint8_t step, uint8_t accent)
{
    deck_plock_ent_t *e = deck_plock_find(pat, voice, step);

    if (e && e->vel)
        return e->vel;
    return (uint8_t)deck_seq_lv_lerp(accent, 100, 127);
}

static inline uint8_t deck_plock_cond(uint8_t pat, uint8_t voice, uint8_t step)
{
    deck_plock_ent_t *e = deck_plock_find(pat, voice, step);

    return e ? e->cond : 0u;
}

static inline int8_t deck_plock_micro(uint8_t pat, uint8_t voice, uint8_t step)
{
    deck_plock_ent_t *e = deck_plock_find(pat, voice, step);

    return e ? e->micro : 0;
}

static inline int8_t deck_plock_pitch(uint8_t pat, uint8_t voice, uint8_t step)
{
    deck_plock_ent_t *e = deck_plock_find(pat, voice, step);

    return e ? e->pitch : 0;
}

static inline int deck_plock_has_data(uint8_t pat, uint8_t voice, uint8_t step)
{
    return deck_plock_find(pat, voice, step) != NULL;
}

static inline uint8_t deck_sx_gate(uint8_t sx)
{
    return sx & 0xFu;
}

static inline uint8_t deck_seq_lv_code(uint32_t on, uint32_t cut)
{
    return on ? (uint8_t)(DECK_SEQ_LV_FULL - (cut ? 1u : 0u)) : (cut ? 1u : 0u);
}

static inline uint8_t deck_sx_slide(uint8_t sx)
{
    return deck_seq_lv_code(sx & DECK_SX_SLIDE, sx & DECK_SX_SLIDE_CUT);
}

static inline uint8_t deck_sx_with_slide(uint8_t sx, uint8_t lv)
{
    sx = (uint8_t)(sx & (uint8_t)~(DECK_SX_SLIDE | DECK_SX_SLIDE_CUT));
    if (lv >= 2u)
        sx |= DECK_SX_SLIDE;
    if (lv == 1u || lv == 2u)
        sx |= DECK_SX_SLIDE_CUT;
    return sx;
}

static inline uint8_t deck_sx_ratchet(uint8_t sx)
{
    return (uint8_t)(((sx >> DECK_SX_RATCH_SHIFT) & 3u) + DECK_SEQ_RATCHET_MIN);
}

static inline uint8_t deck_seq_gate_is_tie(uint8_t gate)
{
    return gate >= DECK_SEQ_GATE_TIE ? 1u : 0u;
}

static inline uint32_t deck_seq_gate_samples(uint8_t gate, uint32_t samples_per_step)
{
    uint32_t sp;

    if (!gate || deck_seq_gate_is_tie(gate))
        return 0u;
    sp = samples_per_step ? samples_per_step : 1u;
    return (sp * (uint32_t)gate) / 8u + 1u;
}

static inline uint8_t deck_seq_ratchet_allowed(uint8_t slide, uint8_t gate)
{
    if (slide)
        return 0u;
    if (deck_seq_gate_is_tie(gate))
        return 0u;
    return 1u;
}

static inline void deck_seq_init(void)
{
    uint8_t i;

    deck_seq_zero(&deck_seq_g, sizeof deck_seq_g);
    for (i = 0u; i < DECK_NPAT; i++)
        deck_seq_defaults(&deck_seq_g.bank.p[i]);
    deck_seq_g.rt.bpm = 120u;
    deck_seq_g.rt.swing = 50u;
    deck_seq_g.rt.key = 0u;
    deck_seq_g.rt.scale = 0u;
    deck_seq_g.rt.seq_clk = FM_SEQ_CLK_RATIO_ONE;
    deck_seq_g.rt.page_synth = (uint8_t)DECK_PAGE_KIT;
}

static inline uint32_t deck_seq_view_base(const deck_runtime_t *rt)
{
    if (!rt)
        return 0u;
    return (uint32_t)(rt->bar_page % FM_SEQ_MAX_PAGES) * FM_SEQ_PAGE_STEPS;
}

static inline uint8_t deck_seq_get_hit(const deck_pattern_t *pat, uint8_t voice, uint32_t step)
{
    if (!pat || voice >= DECK_NVOICES)
        return 0u;
    if (step >= pat->lane[voice].length)
        return 0u;
    return (uint8_t)fm_seq_bit(pat->lane[voice].hit, step);
}

static inline uint8_t deck_seq_get_accent(const deck_pattern_t *pat, uint8_t voice, uint32_t step)
{
    if (!pat || voice >= DECK_NVOICES)
        return 0u;
    if (step >= pat->lane[voice].length)
        return 0u;
    return deck_seq_lv_code(fm_seq_bit(pat->lane[voice].accent, step), fm_seq_bit(pat->lane[voice].accent_cut, step));
}

static inline void deck_seq_put_accent(deck_lane_t *ln, uint32_t step, uint8_t lv)
{
    fm_seq_bit_put(ln->accent, step, lv >= 2u ? 1 : 0);
    fm_seq_bit_put(ln->accent_cut, step, (lv == 1u || lv == 2u) ? 1 : 0);
}

static inline uint8_t deck_seq_step_gate(uint8_t pat_ix, const deck_pattern_t *p, uint8_t voice, uint8_t st)
{
    deck_plock_ent_t *e = deck_plock_find(pat_ix, voice, st);

    if (e && e->step_x)
        return deck_sx_gate(e->step_x);
    return deck_seq_get_hit(p, voice, st) ? DECK_SEQ_GATE_DEF : 0u;
}

static inline uint8_t deck_seq_step_slide(uint8_t pat_ix, const deck_pattern_t *p, uint8_t voice, uint8_t st)
{
    deck_plock_ent_t *e = deck_plock_find(pat_ix, voice, st);

    (void)p;
    if (e && e->step_x)
        return deck_sx_slide(e->step_x);
    return 0u;
}

static inline uint8_t deck_seq_step_ratchet(uint8_t pat_ix, const deck_pattern_t *p, uint8_t voice, uint8_t st)
{
    deck_plock_ent_t *e = deck_plock_find(pat_ix, voice, st);

    (void)p;
    if (e && e->step_x)
        return deck_sx_ratchet(e->step_x);
    return DECK_SEQ_RATCHET_MIN;
}

static inline void deck_seq_record_step(deck_pattern_t *pat, deck_runtime_t *rt, uint8_t voice,
                                        uint8_t accent)
{
    deck_lane_t *ln;
    uint32_t st;

    if (!pat || !rt || !rt->rec_arm || !rt->playing || voice >= DECK_NVOICES)
        return;
    ln = &pat->lane[voice];
    if (!ln->length)
        return;
    st = rt->master_step ? (rt->master_step - 1u) % (uint32_t)ln->length : 0u;
    fm_seq_lane_pattern_mark_edited(&ln->euclid_hits, &ln->pattern_origin);
    fm_seq_bit_put(ln->hit, st, 1);
    if (accent)
        deck_seq_put_accent(ln, st, DECK_SEQ_LV_FULL);
}

static inline void deck_seq_toggle_step(deck_pattern_t *pat, deck_runtime_t *rt, uint8_t voice,
                                        uint8_t step_in_page)
{
    uint32_t step;
    deck_lane_t *ln;
    int on;

    if (!pat || !rt || voice >= DECK_NVOICES || step_in_page >= FM_SEQ_PAGE_STEPS)
        return;
    step = deck_seq_view_base(rt) + step_in_page;
    ln = &pat->lane[voice];
    if (step >= (uint32_t)FM_SEQ_STEPS)
        return;
    on = step < (uint32_t)ln->length ? fm_seq_bit(ln->hit, step) : 0;
    fm_seq_lane_pattern_mark_edited(&ln->euclid_hits, &ln->pattern_origin);
    if (on) {
        fm_seq_bit_put(ln->hit, step, 0);
        deck_seq_put_accent(ln, step, 0u);
    } else {
        /* Extend lane to cover this page so hits on view pages > 0 stick. */
        if (step >= (uint32_t)ln->length) {
            uint32_t need = ((step / FM_SEQ_PAGE_STEPS) + 1u) * FM_SEQ_PAGE_STEPS;

            ln->length = fm_seq_lane_len_clamp((uint8_t)need);
        }
        fm_seq_bit_put(ln->hit, step, 1);
        if (rt->accent_hold)
            deck_seq_put_accent(ln, step, DECK_SEQ_LV_FULL);
    }
}

static inline void deck_seq_set_length(deck_pattern_t *pat, uint8_t voice, uint8_t len)
{
    if (!pat || voice >= DECK_NVOICES)
        return;
    pat->lane[voice].length = fm_seq_lane_len_clamp(len);
}

static inline void deck_seq_set_div(deck_pattern_t *pat, uint8_t voice, uint8_t div)
{
    if (!pat || voice >= DECK_NVOICES)
        return;
    pat->lane[voice].div = fm_seq_clk_ratio_clamp(div);
}

/* One-time: old pattern RAM stored lane div 0..2 as (1/2, 1, x2) table indices. */
static inline void deck_seq_migrate_lane_div_bank(deck_bank_t *bank)
{
    uint8_t pi;
    uint8_t vi;

    if (!bank)
        return;
    for (pi = 0u; pi < DECK_NPAT; pi++)
        for (vi = 0u; vi < DECK_NVOICES; vi++) {
            deck_lane_t *ln = &bank->p[pi].lane[vi];

            if (ln->div <= 2u)
                ln->div = (uint8_t)(ln->div + 2u);
        }
}

/* Pages = ceil(max lane length / 16); clamp 1..FM_SEQ_MAX_PAGES. Matches genre `seq_pages`. */
static inline uint8_t deck_seq_pattern_pages(const deck_pattern_t *pat)
{
    uint8_t v;
    uint8_t max_len = 0u;

    if (!pat)
        return 1u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (pat->lane[v].length > max_len)
            max_len = pat->lane[v].length;
    }
    if (max_len == 0u)
        return 1u;
    {
        uint8_t pages = (uint8_t)((max_len + FM_SEQ_PAGE_STEPS - 1u) / FM_SEQ_PAGE_STEPS);

        return fm_seq_pages_clamp(pages);
    }
}

/* PREPEND nav: old pattern becomes the second half -> shift every p-lock step of this pat by old_len. */
static inline void deck_plock_shift_pat(uint8_t pat, uint32_t old_len)
{
    uint32_t i;
    uint16_t mask = (uint16_t)((uint16_t)(pat % DECK_NPAT) << 11);

    if (!old_len || old_len >= (uint32_t)FM_SEQ_STEPS)
        return;
    for (i = 0u; i < DECK_PLOCK_SLOTS; i++) {
        deck_plock_ent_t *e = &deck_seq_g.plock.ent[i];
        uint16_t step;

        if (!e->tag || (e->tag & (uint16_t)0xF800u) != mask)
            continue;
        step = (uint16_t)(e->tag & 0x7Fu);
        if ((uint32_t)step + old_len >= (uint32_t)FM_SEQ_STEPS)
            continue;
        e->tag = (uint16_t)((e->tag & (uint16_t)0xFF80u) | (uint16_t)(step + old_len));
    }
}

/* Double the pattern: every lane at full length duplicates, shorter lanes keep their wrap.
 * nav = FM_SEQ_NAV_APPEND or FM_SEQ_NAV_PREPEND; old_pages must be the pre-doubling count. */
static inline void deck_seq_pattern_double(deck_pattern_t *pat, uint8_t pat_ix, uint8_t old_pages, uint8_t nav)
{
    uint32_t old_len;
    uint32_t new_len;
    uint8_t v;

    if (!pat)
        return;
    old_pages = fm_seq_pages_clamp(old_pages);
    if ((uint32_t)old_pages * 2u > FM_SEQ_MAX_PAGES)
        return;
    old_len = (uint32_t)old_pages * FM_SEQ_PAGE_STEPS;
    new_len = old_len * 2u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_lane_t *ln = &pat->lane[v];

        if ((uint32_t)ln->length < old_len)
            continue; /* short lane keeps its wrap */
        fm_seq_bits_dup(ln->hit, old_len);
        fm_seq_bits_dup(ln->accent, old_len);
        fm_seq_bits_dup(ln->accent_cut, old_len);
        ln->length = fm_seq_lane_len_clamp((uint8_t)new_len);
    }
    if (nav == FM_SEQ_NAV_PREPEND)
        deck_plock_shift_pat(pat_ix, old_len);
}

static inline void deck_seq_copy_page_lane(deck_lane_t *ln, uint8_t bar_page)
{
    uint16_t h;
    uint16_t a;
    uint16_t ac;
    uint8_t next;
    uint8_t new_len;

    if (!ln || bar_page >= FM_SEQ_MAX_PAGES - 1u)
        return;
    h = fm_seq_bits_page16(ln->hit, bar_page);
    a = fm_seq_bits_page16(ln->accent, bar_page);
    ac = fm_seq_bits_page16(ln->accent_cut, bar_page);
    next = (uint8_t)(bar_page + 1u);
    fm_seq_bits_set_page16(ln->hit, next, h);
    fm_seq_bits_set_page16(ln->accent, next, a);
    fm_seq_bits_set_page16(ln->accent_cut, next, ac);
    new_len = (uint8_t)((next + 1u) * FM_SEQ_PAGE_STEPS);
    if (new_len > ln->length)
        ln->length = fm_seq_lane_len_clamp(new_len);
}

static inline void deck_seq_duplicate_page(deck_pattern_t *pat, uint8_t voice_or_all, uint8_t bar_page)
{
    uint8_t v;

    if (!pat || bar_page >= FM_SEQ_MAX_PAGES - 1u)
        return;
    if (voice_or_all == DECK_DUP_ALL) {
        for (v = 0u; v < DECK_NVOICES; v++)
            deck_seq_copy_page_lane(&pat->lane[v], bar_page);
    } else if (voice_or_all < DECK_NVOICES)
        deck_seq_copy_page_lane(&pat->lane[voice_or_all], bar_page);
}

static inline void deck_seq_copy_pat(const deck_pattern_t *src, deck_pattern_t *dst)
{
    if (!src || !dst)
        return;
    *dst = *src;
}

static inline void deck_seq_clip_copy(const deck_pattern_t *src)
{
    if (!src)
        return;
    deck_seq_g.paste = *src;
    deck_seq_g.paste_valid = 1u;
}

static inline uint8_t deck_seq_clip_paste(deck_pattern_t *dst)
{
    if (!dst || !deck_seq_g.paste_valid)
        return 0u;
    deck_seq_copy_pat(&deck_seq_g.paste, dst);
    return 1u;
}

static inline uint32_t deck_seq_s16_base_samples(uint8_t bpm)
{
    uint32_t bp = bpm < 40u ? 40u : (bpm > 200u ? 200u : bpm);

    return (uint32_t)((DECK_FS * 60u) / (bp * 4u));
}

static inline uint32_t deck_seq_s16_len_samples(uint32_t base, uint8_t swing, uint8_t odd)
{
    int32_t amt;
    int32_t len;

    if (swing < 50u)
        swing = 50u;
    if (swing > 75u)
        swing = 75u;
    amt = (int32_t)swing - 50;
    if (odd)
        len = (int32_t)base + (int32_t)((base * (uint32_t)amt) / 100u);
    else
        len = (int32_t)base - (int32_t)((base * (uint32_t)amt) / 100u);
    if (len < 32)
        len = 32;
    return (uint32_t)len;
}

/* NUDGE is per lane, global across patterns: row 0 is authoritative, edits mirror it into every row so
 * the DKZ7 segment layout stays unchanged. */
static inline int8_t deck_seq_lane_nudge_get(uint8_t pat_ix, uint8_t voice)
{
    (void)pat_ix;
    return deck_seq_g.lane_nudge[0][voice % DECK_NVOICES];
}

static inline void deck_seq_lane_nudge_put(uint8_t voice, int8_t val)
{
    uint32_t p;

    for (p = 0u; p < DECK_NPAT; p++)
        deck_seq_g.lane_nudge[p][voice % DECK_NVOICES] = val;
}

/* Blobs saved with per-pattern nudge: keep pattern 0's value per lane. */
static inline void deck_seq_lane_nudge_unify(void)
{
    uint8_t v;

    for (v = 0u; v < DECK_NVOICES; v++)
        deck_seq_lane_nudge_put(v, fm_seq_lane_nudge_clamp(deck_seq_g.lane_nudge[0][v]));
}

static inline int32_t deck_seq_timing_to_samples(int32_t units, uint32_t sps)
{
    if (sps == 0u)
        sps = 1u;
    return (units * (int32_t)sps) / FM_SEQ_TIMING_UNITS;
}

static inline void deck_seq_micro_queue(deck_trig_evt_t *ev, uint8_t *nev, uint8_t voice, uint8_t vel,
                                      uint8_t accent, uint8_t gate, uint8_t slide, uint8_t ratchet,
                                      uint16_t step_samples, int8_t semi, uint32_t *mask)
{
    if (mask)
        *mask |= (1u << voice);
    if (ev && nev && *nev < DECK_TRIG_Q_MAX) {
        ev[*nev].voice = voice;
        ev[*nev].vel = vel;
        ev[*nev].accent = accent;
        ev[*nev].seq_step = (uint8_t)(deck_seq_g.rt.master_step & 0xFFu);
        ev[*nev].gate = gate;
        ev[*nev].slide = slide;
        ev[*nev].ratchet = ratchet;
        ev[*nev].step_samples = step_samples;
        ev[*nev].semi = semi;
        (*nev)++;
    }
}

/* Gate/slide/ratchet are latched at arm time: by fire time lane_step already points past the step. */
static inline void deck_seq_micro_fire_voice(uint8_t voice, deck_trig_evt_t *ev, uint8_t *nev, uint32_t *mask)
{
    if (voice >= DECK_NVOICES)
        return;
    deck_seq_g.micro_armed = (uint16_t)(deck_seq_g.micro_armed & (uint16_t)~(1u << voice));
    if (!deck_seq_lane_audible(&deck_seq_g.rt, voice))
        return;
    deck_seq_micro_queue(ev, nev, voice, deck_seq_g.micro_vel[voice], deck_seq_g.micro_accent[voice],
                         deck_seq_g.micro_gate[voice], deck_seq_g.micro_slide[voice], deck_seq_g.micro_rat[voice],
                         deck_seq_g.micro_sps[voice], deck_seq_g.micro_semi[voice], mask);
}

/* Arm a hit `units` from the current 16th boundary; negative = before the next boundary (lookahead). */
static inline void deck_seq_arm_lane_timing(uint8_t voice, int32_t units, uint8_t vel, uint8_t accent,
                                            uint8_t gate, uint8_t slide, uint8_t rat, int8_t semi,
                                            uint32_t sps, deck_trig_evt_t *ev, uint8_t *nev, uint32_t *mask)
{
    int32_t smp = deck_seq_timing_to_samples(units, sps);

    if (voice >= DECK_NVOICES)
        return;
    if (sps == 0u)
        sps = 1u;
    if (deck_seq_g.micro_armed & (uint16_t)(1u << voice))
        deck_seq_micro_fire_voice(voice, ev, nev, mask);
    if (smp < 0) {
        if ((uint32_t)(-smp) >= sps)
            smp = -(int32_t)(sps > 0u ? sps - 1u : 0u);
        deck_seq_g.micro_left[voice] = (uint16_t)(sps - (uint32_t)(-smp));
    } else if (smp > 0) {
        if ((uint32_t)smp >= sps)
            smp = (int32_t)(sps > 0u ? sps - 1u : 0);
        deck_seq_g.micro_left[voice] = (uint16_t)smp;
    } else
        deck_seq_g.micro_left[voice] = 0u;
    deck_seq_g.micro_vel[voice] = vel;
    deck_seq_g.micro_accent[voice] = accent;
    deck_seq_g.micro_semi[voice] = semi;
    deck_seq_g.micro_gate[voice] = gate;
    deck_seq_g.micro_slide[voice] = slide;
    deck_seq_g.micro_rat[voice] = rat;
    deck_seq_g.micro_sps[voice] = (uint16_t)(sps > 65535u ? 65535u : sps);
    deck_seq_g.micro_armed = (uint16_t)(deck_seq_g.micro_armed | (uint16_t)(1u << voice));
}

static inline void deck_seq_micro_tick(deck_trig_evt_t *ev, uint8_t *nev, uint32_t *mask)
{
    uint8_t v;

    if (!deck_seq_g.rt.playing) {
        deck_seq_g.micro_armed = 0u;
        return;
    }
    if (!deck_seq_g.micro_armed)
        return;
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (!(deck_seq_g.micro_armed & (uint16_t)(1u << v)))
            continue;
        if (deck_seq_g.micro_left[v] > 0u) {
            deck_seq_g.micro_left[v]--;
            continue;
        }
        deck_seq_micro_fire_voice(v, ev, nev, mask);
    }
}

/* Ext-clock path: deck_seq_audio_ticks does not run, so armed micro hits are counted down here. */
static inline void deck_seq_micro_service(deck_trig_evt_t *ev, uint8_t *nev, uint32_t nsamples)
{
    uint32_t si;
    uint32_t fired = 0u;

    for (si = 0u; si < nsamples && deck_seq_g.micro_armed; si++)
        deck_seq_micro_tick(ev, nev, &fired);
}

/* VARY seed per bar: stable inside a bar, changes at each bar (GENRE seq_rng advances per bar). */
static inline uint32_t deck_seq_vary_seed(uint32_t master)
{
    return deck_seq_g.vary_base + (master / FM_SEQ_PAGE_STEPS) * 0x9E3779B1u;
}

/* KIT KEY/SCALE snap: v is in the grid when (v - key) mod 12 is a scale degree (tonic always is). */
static inline int deck_seq_in_key(int32_t v, uint8_t key, uint8_t scale)
{
    int32_t pc = ((v - (int32_t)key) % 12 + 12) % 12;
    const uint8_t *tab = DECK_SCALE_SEMIS[scale % DECK_SCALE_N];
    uint32_t i;

    for (i = 0u; i < DECK_SCALE_DEG; i++) {
        if (tab[i] == pc)
            return 1;
    }
    return 0;
}

/* Offset 0 = voice base, never moved. Otherwise nearest in-key offset (ties: up first), within ±DECK_SEQ_SEMI_MAX. */
static inline int32_t deck_seq_snap_semi(int32_t s, uint8_t key, uint8_t scale)
{
    int32_t d, lim = (int32_t)DECK_SEQ_SEMI_MAX;

    if (s == 0 || deck_seq_in_key(s, key, scale))
        return s;
    for (d = 1; d <= 12; d++) {
        if (s + d <= lim && deck_seq_in_key(s + d, key, scale))
            return s + d;
        if (s - d >= -lim && deck_seq_in_key(s - d, key, scale))
            return s - d;
    }
    return s;
}

static inline uint8_t deck_seq_lane_step_resolve(deck_pattern_t *pat, deck_runtime_t *rt, uint8_t voice,
                                                 uint32_t lane_grid, uint32_t master, uint8_t *vel_out,
                                                 uint8_t *acc_out, int8_t *semi_out)
{
    deck_lane_t *ln;
    uint32_t st;
    uint8_t acc;
    uint8_t vel;
    int32_t semi = 0;

    if (!pat || !rt || voice >= DECK_NVOICES)
        return 0u;
    ln = &pat->lane[voice];
    st = fm_seq_lane_map_step(lane_grid, ln->length);
    if (!fm_seq_vary_hit(fm_seq_bit(ln->hit, st), ln->vary, (uint8_t)st,
                         (uint8_t)((master / FM_SEQ_PAGE_STEPS) & 7u), voice, deck_seq_vary_seed(master)))
        return 0u;
    {
        uint8_t bar = (uint8_t)((master / FM_SEQ_PAGE_STEPS) & 7u);

        if (!fm_seq_trig_pass(deck_seq_djo_trig_raw(ln->trig, voice), (uint8_t)st, bar, voice))
            return 0u;
    }
    acc = deck_seq_lv_code(fm_seq_bit(ln->accent, st), fm_seq_bit(ln->accent_cut, st));
    {
        uint8_t bar = (uint8_t)((master / FM_SEQ_PAGE_STEPS) & 7u);
        uint8_t pat_ix = rt->cur_pat % DECK_NPAT;
        uint8_t cond = deck_plock_cond(pat_ix, voice, (uint8_t)st);
        int32_t vr;

        if (!fm_seq_cond_pass(cond, (uint8_t)st, bar, voice, 0))
            return 0u;
        vel = deck_plock_vel(pat_ix, voice, (uint8_t)st, acc);
        if (acc && vel < (uint8_t)deck_seq_lv_lerp(acc, 100, 118))
            vel = (uint8_t)deck_seq_lv_lerp(acc, 100, 118);
        vr = (int32_t)vel + (int32_t)fm_seq_rand_vel(ln->vel_rng, (uint8_t)st, bar, voice, 0u) +
             (int32_t)deck_seq_djo.vel[voice];
        if (vr < 1)
            vr = 1;
        if (vr > 127)
            vr = 127;
        vel = (uint8_t)vr;
        semi = (int32_t)deck_plock_pitch(pat_ix, voice, (uint8_t)st) +
               (int32_t)fm_seq_rand_pitch(ln->pitch_rng, (uint8_t)st, bar, voice, 0u) +
               (int32_t)deck_seq_djo.pitch[voice];
        if (semi < -(int32_t)DECK_SEQ_SEMI_MAX)
            semi = -(int32_t)DECK_SEQ_SEMI_MAX;
        if (semi > (int32_t)DECK_SEQ_SEMI_MAX)
            semi = (int32_t)DECK_SEQ_SEMI_MAX;
        semi = deck_seq_snap_semi(semi, rt->key, rt->scale);
    }
    if (vel_out)
        *vel_out = vel;
    if (acc_out)
        *acc_out = acc;
    if (semi_out)
        *semi_out = (int8_t)semi;
    return 1u;
}

/* lane_step after the next deck_seq_master_tick (div-aware, same wrap as deck_seq_lane_advance). */
static inline uint32_t deck_seq_lane_peek_next(const deck_pattern_t *pat, uint8_t voice)
{
    uint8_t acc = deck_seq_g.lane_clk_acc[voice];
    uint8_t n = fm_seq_clk_ratio_nsteps(&acc, fm_seq_clk_ratio_clamp(pat->lane[voice].div));
    uint8_t s = deck_seq_g.lane_step[voice];
    uint8_t i;

    for (i = 0u; i < n; i++) {
        s++;
        if (s >= pat->lane[voice].length)
            s = 0u;
    }
    return (uint32_t)s;
}

static inline void deck_seq_lane_gsr(deck_pattern_t *pat, uint8_t voice, uint32_t lane_grid, uint32_t master,
                                     uint8_t *gate_out, uint8_t *slide_out, uint8_t *rat_out)
{
    deck_lane_t *ln = &pat->lane[voice];
    uint8_t pat_ix = deck_seq_g.rt.cur_pat % DECK_NPAT;
    uint32_t st = fm_seq_lane_map_step(lane_grid, ln->length);
    uint8_t bar = (uint8_t)((master / FM_SEQ_PAGE_STEPS) & 7u);
    uint8_t gate = deck_seq_step_gate(pat_ix, pat, voice, (uint8_t)st);
    uint8_t slide = deck_seq_step_slide(pat_ix, pat, voice, (uint8_t)st);
    uint8_t rat = deck_seq_step_ratchet(pat_ix, pat, voice, (uint8_t)st);
    deck_plock_ent_t *pe = deck_plock_find(pat_ix, voice, (uint8_t)st);
    int32_t gr;

    /* VARY-added hit (bit off, no p-lock gate): gate 0 would close the voice, so take the default gate. */
    if (!gate && !fm_seq_bit(ln->hit, st) && !(pe && pe->step_x))
        gate = DECK_SEQ_GATE_DEF;
    gr = (int32_t)gate + (int32_t)fm_seq_rand_gate(ln->gate_rng, (uint8_t)st, bar, voice, 0u) +
                 (int32_t)deck_seq_djo.gate[voice];

    if (gr < 0)
        gr = 0;
    if (gr > (int32_t)DECK_SEQ_GATE_TIE)
        gr = (int32_t)DECK_SEQ_GATE_TIE;
    gate = (uint8_t)gr;
    if (rat <= DECK_SEQ_RATCHET_MIN && (deck_seq_djo.flam & (1u << voice)))
        rat = DECK_SEQ_RATCHET_MIN + 1u;
    if (!deck_seq_ratchet_allowed(slide, gate))
        rat = DECK_SEQ_RATCHET_MIN;
    *gate_out = gate;
    *slide_out = slide;
    *rat_out = rat;
}

/* T.HUM: symmetric timing jitter, ±FM_SEQ_MICRO_HALF at 127. Pure hash of (step, voice), so the lookahead
 * (master_step + 1) and the fire (master_step) agree on the same value for the same hit. */
static inline int32_t deck_seq_timing_hum_units(uint32_t step, uint8_t voice)
{
    uint32_t h;
    int32_t j;

    if (deck_seq_g.timing_hum == 0u)
        return 0;
    h = (step * 2654435761u) ^ (((uint32_t)voice + 1u) * 0x9E3779B9u);
    h ^= h >> 16;
    h *= 0x85EBCA6Bu;
    h ^= h >> 13;
    j = (int32_t)(h & 0xFFu) - 128;
    return (j * (int32_t)deck_seq_g.timing_hum * FM_SEQ_MICRO_HALF) / (128 * 127);
}

static inline void deck_seq_lane_micro_lookahead(deck_pattern_t *pat, deck_runtime_t *rt, uint32_t sps,
                                                 deck_trig_evt_t *ev, uint8_t *nev, uint32_t *mask)
{
    uint8_t v;

    if (!pat || !rt || sps == 0u)
        return;
    for (v = 0u; v < DECK_NVOICES; v++) {
        uint32_t nxt_grid;
        uint8_t vel;
        uint8_t acc;
        uint8_t pat_ix;
        int32_t tun;
        int8_t semi;
        uint8_t gate;
        uint8_t slide;
        uint8_t rat;

        if (!deck_seq_lane_audible(rt, v))
            continue;
        nxt_grid = deck_seq_lane_peek_next(pat, v);
        if (!deck_seq_lane_step_resolve(pat, rt, v, nxt_grid, rt->master_step + 1u, &vel, &acc, &semi))
            continue;
        pat_ix = rt->cur_pat % DECK_NPAT;
        tun = (int32_t)deck_plock_micro(pat_ix, v,
                                        (uint8_t)fm_seq_lane_map_step(nxt_grid, pat->lane[v].length)) +
              (int32_t)deck_seq_lane_nudge_get(pat_ix, v) + deck_seq_timing_hum_units(rt->master_step + 1u, v);
        if (tun >= 0)
            continue;
        deck_seq_lane_gsr(pat, v, nxt_grid, rt->master_step + 1u, &gate, &slide, &rat);
        deck_seq_arm_lane_timing(v, tun, vel, acc, gate, slide, rat, semi, sps, ev, nev, mask);
        deck_seq_g.micro_early = (uint16_t)(deck_seq_g.micro_early | (uint16_t)(1u << v));
    }
}

static inline void deck_seq_lane_advance(uint8_t voice, uint8_t n)
{
    deck_lane_t *ln;
    uint8_t i;

    if (voice >= DECK_NVOICES || n == 0u)
        return;
    ln = &deck_seq_cur_pattern()->lane[voice];
    for (i = 0u; i < n; i++) {
        deck_seq_g.lane_step[voice]++;
        if (deck_seq_g.lane_step[voice] >= ln->length)
            deck_seq_g.lane_step[voice] = 0u;
    }
}

static inline void deck_seq_master_tick(deck_pattern_t *pat)
{
    uint8_t v;

    if (!pat)
        return;
    deck_seq_g.rt.master_step++;
    deck_seq_g.rt.play_col = (uint8_t)(deck_seq_g.rt.master_step % FM_SEQ_PAGE_STEPS);
    for (v = 0u; v < DECK_NVOICES; v++) {
        uint8_t n = fm_seq_clk_ratio_nsteps(&deck_seq_g.lane_clk_acc[v],
                                            fm_seq_clk_ratio_clamp(pat->lane[v].div));
        uint8_t i;

        for (i = 0u; i < n; i++)
            deck_seq_lane_advance(v, 1u);
    }
}

static inline void deck_seq_lane_fire(deck_pattern_t *pat, deck_runtime_t *rt, uint8_t voice,
                                      deck_trig_evt_t *ev, uint8_t *nev, uint32_t *mask, uint32_t sps)
{
    uint32_t st;
    uint8_t acc;
    uint8_t vel;
    uint8_t pat_ix;
    int32_t tun;
    int32_t smp;
    uint8_t gate;
    uint8_t slide;
    uint8_t rat;
    int8_t semi;
    uint8_t early;

    if (!pat || !rt || voice >= DECK_NVOICES || sps == 0u)
        return;
    early = (deck_seq_g.micro_early & (uint16_t)(1u << voice)) ? 1u : 0u;
    deck_seq_g.micro_early = (uint16_t)(deck_seq_g.micro_early & (uint16_t)~(1u << voice));
    if (!deck_seq_lane_audible(rt, voice))
        return;
    if (!deck_seq_lane_step_resolve(pat, rt, voice, (uint32_t)deck_seq_g.lane_step[voice], rt->master_step,
                                    &vel, &acc, &semi))
        return;
    st = fm_seq_lane_map_step(deck_seq_g.lane_step[voice], pat->lane[voice].length);
    pat_ix = rt->cur_pat % DECK_NPAT;
    deck_seq_lane_gsr(pat, voice, (uint32_t)deck_seq_g.lane_step[voice], rt->master_step, &gate, &slide, &rat);
    tun = (int32_t)deck_plock_micro(pat_ix, voice, (uint8_t)st) + (int32_t)deck_seq_lane_nudge_get(pat_ix, voice) +
          deck_seq_timing_hum_units(rt->master_step, voice);
    smp = deck_seq_timing_to_samples(tun, sps);
    if (smp > 0) {
        deck_seq_arm_lane_timing(voice, tun, vel, acc, gate, slide, rat, semi, sps, ev, nev, mask);
        return;
    }
    /* Negative micro already fired early via lookahead; without one (first step) play on grid. */
    if (tun < 0 && early)
        return;
    deck_seq_micro_queue(ev, nev, voice, vel, acc, gate, slide, rat, (uint16_t)sps, semi, mask);
}

static inline uint32_t deck_seq_audio_ticks(deck_trig_evt_t *ev, uint8_t *nev, uint32_t nsamples)
{
    deck_runtime_t *rt = &deck_seq_g.rt;
    deck_pattern_t *pat = deck_seq_cur_pattern();
    uint32_t fired = 0u;
    uint32_t base;
    uint32_t remain;
    uint8_t n_ev = 0u;
    uint8_t v;

    if (nev)
        *nev = 0u;
    if (!rt->playing || nsamples == 0u)
        return 0u;

    base = deck_seq_s16_base_samples(rt->bpm);
    remain = nsamples;
    while (remain > 0u) {
        uint32_t s16_len;
        uint32_t chunk;
        uint8_t odd = (uint8_t)(rt->master_step & 1u);

        s16_len = deck_seq_s16_len_samples(base, rt->swing, odd);
        s16_len = fm_seq_clk_ratio_scale_len(s16_len, rt->seq_clk);
        if (rt->step_phase >= s16_len) {
            rt->step_phase -= s16_len;
            for (v = 0u; v < DECK_NVOICES; v++)
                deck_seq_lane_fire(pat, rt, v, ev, &n_ev, &fired, s16_len);
            deck_seq_lane_micro_lookahead(pat, rt, s16_len, ev, &n_ev, &fired);
            deck_seq_master_tick(pat);
        }
        chunk = s16_len - rt->step_phase;
        if (chunk > remain)
            chunk = remain;
        {
            uint32_t si;

            for (si = 0u; si < chunk; si++)
                deck_seq_micro_tick(ev, &n_ev, &fired);
        }
        rt->step_phase += chunk;
        remain -= chunk;
    }
    if (nev)
        *nev = n_ev;
    return fired;
}

static inline void deck_seq_transport_start(void)
{
    uint8_t v;
    uint32_t base = deck_seq_s16_base_samples(deck_seq_g.rt.bpm);

    deck_seq_g.rt.playing = 1u;
    /* Force first audio tick to fire step 0 immediately. */
    deck_seq_g.rt.step_phase = deck_seq_s16_len_samples(base, deck_seq_g.rt.swing, 0u);
    deck_seq_g.rt.master_step = 0u;
    deck_seq_g.rt.play_col = 0u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_seq_g.lane_step[v] = 0u;
        deck_seq_g.lane_clk_acc[v] = 0u;
    }
    deck_seq_g.micro_armed = 0u;
    deck_seq_g.micro_early = 0u;
}

static inline void deck_seq_transport_stop(void)
{
    uint8_t v;

    deck_seq_g.rt.playing = 0u;
    deck_seq_g.rt.step_phase = 0u;
    deck_seq_g.rt.play_col = 0u;
    deck_seq_g.micro_armed = 0u;
    deck_seq_g.micro_early = 0u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        deck_seq_g.ratchet_left[v] = 0u;
        deck_seq_g.ratchet_every[v] = 0u;
        deck_seq_g.ratchet_acc[v] = 0u;
    }
}

/* Dub/House lane-mute presets → 16-voice kit (same 6-bit table as DUB_ENS_MUTE). */
static const uint8_t DECK_ENS_MUTE6[FM_SEQ_NENSEMBLES] = {
    0x3Fu, 0x3Eu, 0x17u, 0x07u, 0x15u, 0x11u, 0x26u, 0x2Cu, 0x0Eu,
    0x31u, 0x13u, 0x19u, 0x2Eu, 0x2Au, 0x38u, 0x28u, 0x01u, 0x00u
};

/* Same indices as FM_SEQ_ENS_* / DECK_ENS_MUTE6; drum-kit readout (not dub/reggae labels). */
static const char *const DECK_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET", "KICK", "CL+PERC", "TOMS", "SN+PERC", "SN+HAT", "K+TOMS", "K+SN",
    "BREAK", "HATS", "HAT+PERC", "SN+PERC", "K+TOM", "GROOVE", "DRUMS", "KIT",
    "MINIMAL", "TUTTI"
};

static inline const char *deck_ens_combo_name(uint8_t ens)
{
    if (ens >= FM_SEQ_NENSEMBLES)
        ens = FM_SEQ_ENS_ALL;
    return DECK_ENS_NAMES[ens];
}

static inline uint16_t deck_ens_mute_expand(uint8_t m6)
{
    uint16_t m = 0u;

    m6 &= 0x3Fu;
    /* FM_SEQ_ENS_MUTE / ix 0: all six dub lanes → every kit voice including cym alts. */
    if (m6 == 0x3Fu)
        return 0xFFFFu;

    if (m6 & 1u)
        m |= (uint16_t)(1u << DECK_BD);
    if (m6 & 2u)
        m |= (uint16_t)(1u << DECK_SN);
    if (m6 & 4u)
        m |= (uint16_t)((1u << DECK_CH) | (1u << DECK_OH) | (1u << DECK_RM) | (1u << DECK_RD) |
                         (1u << DECK_CR));
    if (m6 & 8u)
        m |= (uint16_t)(1u << DECK_CP);
    if (m6 & 16u)
        m |= (uint16_t)((1u << DECK_TH) | (1u << DECK_TM) | (1u << DECK_TL));
    if (m6 & 32u)
        m |= (uint16_t)((1u << DECK_CB) | (1u << DECK_TB) | (1u << DECK_CW) | (1u << DECK_CNH) |
                         (1u << DECK_CNL));
    return m;
}

static inline uint16_t deck_lane_mute_mask16(const deck_runtime_t *rt)
{
    uint16_t m = 0u;
    uint8_t v;

    if (!rt)
        return 0u;
    for (v = 0u; v < DECK_NVOICES; v++) {
        if (rt->lane_mute[v])
            m |= (uint16_t)(1u << v);
    }
    return m;
}

static inline void deck_apply_ens_mask(deck_runtime_t *rt, uint16_t mask)
{
    uint8_t v;

    if (!rt)
        return;
    rt->solo_mask = 0u;
    for (v = 0u; v < DECK_NVOICES; v++)
        rt->lane_mute[v] = (mask & (uint16_t)(1u << v)) ? 1u : 0u;
}

static inline void deck_apply_ensemble(deck_runtime_t *rt, uint8_t ens)
{
    if (!rt)
        return;
    if (ens >= FM_SEQ_NENSEMBLES)
        ens = FM_SEQ_ENS_ALL;
    deck_apply_ens_mask(rt, deck_ens_mute_expand(DECK_ENS_MUTE6[ens]));
}

static inline uint8_t deck_detect_ensemble(const deck_runtime_t *rt)
{
    uint16_t cur;
    uint8_t i;

    if (!rt || rt->solo_mask)
        return FM_SEQ_ENS_CUSTOM;
    cur = deck_lane_mute_mask16(rt);
    for (i = 0u; i < FM_SEQ_NENSEMBLES; i++) {
        if (cur == deck_ens_mute_expand(DECK_ENS_MUTE6[i]))
            return i;
    }
    return FM_SEQ_ENS_CUSTOM;
}

/* SEQ pg4 COMBO knob value (never FM_SEQ_ENS_NAMES / dub labels). */
static inline const char *deck_ens_combo_readout(const deck_runtime_t *rt)
{
    uint8_t det;

    if (!rt)
        return DECK_ENS_NAMES[FM_SEQ_ENS_ALL];
    det = deck_detect_ensemble(rt);
    return det == FM_SEQ_ENS_CUSTOM ? "CUSTOM" : deck_ens_combo_name(det);
}

/* COMBO pot: 16-wedge voice-on bitmap from lane_mute (inverted). Knob still steps named presets. */
static inline uint16_t deck_lane_on_mask16(const deck_runtime_t *rt)
{
    if (!rt)
        return 0u;
    return (uint16_t)(~deck_lane_mute_mask16(rt) & 0xFFFFu);
}

/* sps_ext: measured ext-clock step length in samples (already seq_clk-scaled); 0 = derive from bpm. */
static inline void deck_seq_clock_16th(deck_trig_evt_t *ev, uint8_t *nev, uint32_t sps_ext)
{
    deck_runtime_t *rt = &deck_seq_g.rt;
    deck_pattern_t *pat = deck_seq_cur_pattern();
    uint32_t fired = 0u;
    uint8_t n_ev = 0u;
    uint8_t v;

    if (!rt->playing)
        return;
    {
        uint32_t sps = deck_seq_s16_base_samples(rt->bpm);

        sps = fm_seq_clk_ratio_scale_len(sps, rt->seq_clk);
        if (sps_ext)
            sps = sps_ext > 65535u ? 65535u : sps_ext;
        if (sps == 0u)
            sps = 1u;
        for (v = 0u; v < DECK_NVOICES; v++)
            deck_seq_lane_fire(pat, rt, v, ev, &n_ev, &fired, sps);
        deck_seq_lane_micro_lookahead(pat, rt, sps, ev, &n_ev, &fired);
    }
    deck_seq_master_tick(pat);
    if (nev)
        *nev = n_ev;
}

#endif /* DECK_SEQ_H */
