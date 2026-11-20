#ifndef MIDI_CTRL_H
#define MIDI_CTRL_H

#include <stdint.h>

#include "fm_lfo.h"

enum { BANK_SYNTH = 0, BANK_ENGINE = 1, BANK_TIME = 2, BANK_SPACE = 3, BANK_LFO = 4, NBANKS = 5 };

enum {
    P_TIMBRE = 0,
    P_RATIO,
    P_ATTACK,
    P_DECAY,
    P_PARAM_A,
    P_PARAM_B,
    P_DENSITY,
    P_VEL_MOD,
    P_TEMPO,
    P_SWING,
    P_GATE,
    P_OCTAVE,
    P_DETUNE,
    P_DRIVE,
    P_ECHO_MIX,
    P_ECHO_TIME,
    P_LFO_TARGET,
    P_LFO_RATE,
    P_LFO_DEPTH,
    P_LFO_SHAPE,
    NPARAMS = 20
};

enum { MODE_TRIAXIS = 0, MODE_CONTURA = 1, MODE_DITDAH = 2, NMODES = 3 };

enum {
    VSLOT_ARP_BASE = 20, /* 20..23: ARP MODE, DIV, SPAN, GATE */
    VSLOT_SEQ_BASE = 24, /* 24..27: SEQ STEPS, PACE, PROG, MUTATE */
    NVSLOTS = 28
};

static const char *const BANK_NAMES[NBANKS] = {"SYNTH", "ENGINE", "TIME", "SPACE", "LFO"};
static const char *const MODE_NAMES[NMODES] = {"CYCLIC", "CONTURA", "DIT DAH"};
static const char *const PARAM_NAMES[NPARAMS] = {
    "TIMBRE", "RATIO", "ATTACK", "DECAY", "AXIS", "MOTION", "DENSITY", "VOICING",
    "TEMPO", "SWING", "RATE", "OCTAVE", "DETUNE", "DRIVE", "E.MIX", "E.TIME",
    "TARGET", "FREQ", "DEPTH", "SHAPE"};

#define DITDAH_PHRASES 23u
#define DITDAH_MAX_LEN 36u
#define PERF_SEQ_MAX_STEPS 32u

#define TRYP_CFG_MAGIC_V1 0x54525031u /* "TRP1" */
#define TRYP_CFG_MAGIC    0x54525032u /* "TRP2" */

/* Saved layout before 2026-11; never change (unversioned device flash). */
typedef struct {
    uint8_t val[20];       /* effective 0..127 (with LFO applied to all active targets) */
    uint8_t base_val[20];  /* unmodulated 0..127 set by knobs/CCs */
    uint8_t lfo_depth[16]; /* per-parameter LFO depth (0..127) for simultaneous modulation */
    uint8_t cc_map[20];    /* default: 1,2,73,72, 16..19, 20..23, 24..27, 28,29,30,33 */
    uint8_t midi_ch;       /* 0 = OMNI, 1..16 = Ch 1..16 */
    uint8_t bank;          /* 0..4 */
    uint8_t mode;          /* 0..2 */
    uint8_t reserved;
    int16_t ratio_lfo_q8;  /* continuous decimal LFO offset on modulator ratio (-256..+256 in Q8) */
    char phrases[DITDAH_PHRASES][DITDAH_MAX_LEN]; /* all 23 editable & persistent Morse phrases */
} midi_cfg_v0_t;

_Static_assert(sizeof(midi_cfg_v0_t) == 910u, "v0 layout frozen");

/* TRP1 saved layout (2026-11); never change. */
typedef struct {
    uint32_t magic;
    uint8_t val[20];       /* effective 0..127 (with LFO applied to all active targets) */
    uint8_t base_val[20];  /* unmodulated 0..127 set by knobs/CCs */
    uint8_t lfo_depth[16]; /* per-parameter LFO depth (0..127) for simultaneous modulation */
    uint8_t cc_map[20];    /* default: 1,2,73,72, 16..19, 20..23, 24..27, 28,29,30,33 */
    uint8_t midi_ch;       /* 0 = OMNI, 1..16 = Ch 1..16 */
    uint8_t bank;          /* 0..4 */
    uint8_t mode;          /* 0..2 */
    uint8_t reserved;
    int16_t ratio_lfo_q8;  /* continuous decimal LFO offset on modulator ratio (-256..+256 in Q8) */
    char phrases[DITDAH_PHRASES][DITDAH_MAX_LEN]; /* all 23 editable & persistent Morse phrases */
} midi_cfg_v1_t;

_Static_assert(sizeof(midi_cfg_v1_t) == 916u, "v1 layout frozen");

typedef struct {
    uint32_t magic;
    uint8_t val[20];       /* effective 0..127 (with LFO applied to all active targets) */
    uint8_t base_val[20];  /* unmodulated 0..127 set by knobs/CCs */
    uint8_t lfo_depth[16]; /* per-parameter LFO depth (0..127) for simultaneous modulation */
    uint8_t cc_map[20];    /* default: 1,2,73,72, 16..19, 20..23, 24..27, 28,29,30,33 */
    uint8_t midi_ch;       /* 0 = OMNI, 1..16 = Ch 1..16 */
    uint8_t bank;          /* 0..4 */
    uint8_t mode;          /* 0..2 */
    uint8_t reserved;
    int16_t ratio_lfo_q8;  /* continuous decimal LFO offset on modulator ratio (-256..+256 in Q8) */
    char phrases[DITDAH_PHRASES][DITDAH_MAX_LEN]; /* all 23 editable & persistent Morse phrases */
    uint8_t lfo_rate[NVSLOTS];
    uint8_t lfo_shape[NVSLOTS];
    uint8_t lfo_algo[NVSLOTS];
    uint8_t lfo_ratio[NVSLOTS];
    uint8_t lfo_index[NVSLOTS];
    uint8_t lfo_fb[NVSLOTS];
    uint8_t lfo_fold[NVSLOTS];
    uint8_t lfo_phase[NVSLOTS];
    uint8_t lfo_skew[NVSLOTS];
    uint8_t lfo_smooth[NVSLOTS];
    uint8_t lfo_depth_ext[8];
    uint8_t arp_base[4];
    uint8_t seq_base[4];
    uint8_t rec[4];
    int8_t seq_steps[PERF_SEQ_MAX_STEPS];
    uint8_t page;
    uint8_t mode_mem[NMODES][11];
    uint8_t mode_mem_ok[NMODES];
} midi_cfg_t;

typedef struct {
    uint8_t learn_active;
    uint8_t learn_slot; /* 0..19 */
    uint8_t hud_param;  /* 0..19 */
    uint8_t hud_cc;     /* 0..127, 255 = encoder */
    uint32_t hud_until_ms; /* fm1_ms + 1200 */
    uint8_t ext_sync;   /* 1 if 0xF8 seen within 1500 ms */
    uint8_t playing;    /* clock running */
    uint16_t bpm;       /* 60..240 */
    uint32_t last_f8_ms;
    uint32_t f8_dt_acc; /* smoothed 6-tick interval in ms */
    uint32_t f8_count;
    uint32_t int_last_ms;
    uint32_t int_last_frames;
    uint32_t int_rem_ms;
    uint32_t step_ticks; /* accumulated 24-PPQN ticks ready for engine */
    uint32_t raw_tick;   /* total 24-PPQN ticks for swing calculation */
    uint8_t swing_held;  /* ticks delayed for odd 16th swing */
    uint32_t lfo_ph;     /* legacy; current target phase mirror for debug */
    uint32_t lfo_last_ms;
    uint32_t lfo_rng;
    int8_t lfo_sh;       /* S&H when shape is not per-slot */
    uint8_t lfo_rate[NVSLOTS];      /* per-target FREQ (RAM; not in flash cfg) */
    uint8_t lfo_shape[NVSLOTS];     /* per-target SHAPE (RAM; not in flash cfg) */
    uint8_t lfo_algo[NVSLOTS];
    uint8_t lfo_ratio[NVSLOTS];
    uint8_t lfo_index[NVSLOTS];
    uint8_t lfo_fb[NVSLOTS];
    uint8_t lfo_fold[NVSLOTS];
    uint8_t lfo_phase[NVSLOTS];
    uint8_t lfo_skew[NVSLOTS];
    uint8_t lfo_smooth[NVSLOTS];
    uint8_t lfo_page; /* 0..2 knob pages on BANK_LFO */
    uint32_t lfo_ph_slot[NVSLOTS];  /* per-target LFO phase */
    uint32_t lfo_cycle[NVSLOTS];    /* unwrapped cycle count (ph wrap) */
    int8_t lfo_sh_slot[NVSLOTS];    /* legacy S&H slot state */
    uint32_t lfo_rng_slot[NVSLOTS];
    int32_t lfo_slew_q15[NVSLOTS];
    uint8_t lfo_depth_ext[8];       /* LFO depth for virtual slots 20..27 */
} midi_state_t;

#define MIDI_LFO_SCOPE_MS_PER_PX 5u /* ~1 Hz ≈ 1–2 cycles across 240 px */

#define NLFO_TARGETS 24u

static const uint8_t LFO_TGT[NLFO_TARGETS] = {
    P_DETUNE, P_DRIVE, P_ECHO_MIX, P_ECHO_TIME,
    P_PARAM_A, P_PARAM_B, P_DENSITY, P_VEL_MOD,
    P_TIMBRE, P_RATIO, P_ATTACK, P_DECAY,
    P_TEMPO, P_SWING, P_GATE, P_OCTAVE,
    20, 21, 22, 23,
    24, 25, 26, 27
};

static inline uint32_t midi_lfo_target_idx(const midi_cfg_t *c)
{
    uint32_t idx = ((uint32_t)c->base_val[P_LFO_TARGET] * NLFO_TARGETS) / 128u;

    if (idx >= NLFO_TARGETS)
        idx = NLFO_TARGETS - 1u;
    return idx;
}

static inline uint8_t midi_lfo_target_cc_from_idx(uint32_t idx)
{
    if (idx == 0u)
        return 0u;
    if (idx >= NLFO_TARGETS - 1u)
        return 127u;
    return (uint8_t)((idx * 128u + 64u) / NLFO_TARGETS);
}

static inline uint32_t midi_lfo_target_slot(const midi_cfg_t *c)
{
    return LFO_TGT[midi_lfo_target_idx(c)];
}

static inline uint8_t midi_lfo_get_depth(const midi_cfg_t *c, const midi_state_t *s, uint32_t slot)
{
    if (slot < 16u)
        return c->lfo_depth[slot];
    if (slot >= VSLOT_ARP_BASE && slot < NVSLOTS)
        return s->lfo_depth_ext[slot - VSLOT_ARP_BASE];
    return 0u;
}

static inline void midi_lfo_set_depth(midi_cfg_t *c, midi_state_t *s, uint32_t slot, uint8_t d)
{
    if (slot < 16u)
        c->lfo_depth[slot] = d;
    else if (slot >= VSLOT_ARP_BASE && slot < NVSLOTS)
        s->lfo_depth_ext[slot - VSLOT_ARP_BASE] = d;
}

static inline int midi_lfo_any_active(const midi_cfg_t *c, const midi_state_t *s)
{
    uint32_t i;

    for (i = 0; i < NLFO_TARGETS; i++) {
        if (midi_lfo_get_depth(c, s, LFO_TGT[i]) > 0u)
            return 1;
    }
    return 0;
}

static inline void midi_lfo_clear_all(midi_cfg_t *c, midi_state_t *s)
{
    uint32_t i;

    for (i = 0; i < 16u; i++) {
        c->lfo_depth[i] = 0;
        c->val[i] = c->base_val[i];
    }
    for (i = 0; i < 8u; i++)
        s->lfo_depth_ext[i] = 0;
    c->base_val[P_LFO_DEPTH] = 0;
    c->val[P_LFO_DEPTH] = 0;
    c->ratio_lfo_q8 = 0;
}

static void midi_cfg_mode_preset(midi_cfg_t *c, uint8_t mode)
{
    if (mode == MODE_TRIAXIS) {
        /* Warm Rhodes jazz changes: Maj3 axis, Rubato (holds Imaj7), 6/9 V->I cycle, Drop-2 voicing, 1/4 rate */
        c->base_val[P_TIMBRE] = 26;
        c->base_val[P_RATIO] = 52;   /* 1:2 warm octave overtone */
        c->base_val[P_ATTACK] = 6;
        c->base_val[P_DECAY] = 72;
        c->base_val[P_PARAM_A] = 0;  /* MAJ 3rd axis */
        c->base_val[P_PARAM_B] = 28; /* RUBATO */
        c->base_val[P_DENSITY] = 96; /* 8/9 chords (ii7 -> V7 -> Imaj7 visible on circle!) */
        c->base_val[P_VEL_MOD] = 74; /* DROP-2 voicing */
        c->base_val[P_TEMPO] = 48;   /* 108 BPM */
        c->base_val[P_SWING] = 32;   /* 56% jazz swing */
        c->base_val[P_GATE] = 28;    /* 1/4 note chord pacing */
    } else if (mode == MODE_CONTURA) {
        /* Lush Pentatonic / Dorian chamber trio at 1/8 note pacing */
        c->base_val[P_TIMBRE] = 32;
        c->base_val[P_RATIO] = 8;    /* 2:1 woody bell */
        c->base_val[P_ATTACK] = 4;
        c->base_val[P_DECAY] = 60;
        c->base_val[P_PARAM_A] = 28; /* Aeolian (A minor default) */
        c->base_val[P_PARAM_B] = 32; /* 9-step melodic arch */
        c->base_val[P_DENSITY] = 88;
        c->base_val[P_VEL_MOD] = 37; /* TRIO (bucket 3) */
        c->base_val[P_TEMPO] = 54;   /* 114 BPM */
        c->base_val[P_SWING] = 24;   /* 55% groove */
        c->base_val[P_GATE] = 52;    /* 1/8 note melodic flow */
    } else {
        /* Radiophonic Morse: Perfect 5th (+7 st) interval between dit and dah */
        c->base_val[P_TIMBRE] = 38;
        c->base_val[P_RATIO] = 52;   /* 1:2 */
        c->base_val[P_ATTACK] = 2;
        c->base_val[P_DECAY] = 44;
        c->base_val[P_PARAM_A] = 38; /* +7 st (Perfect 5th) */
        c->base_val[P_PARAM_B] = 0;  /* SOS */
        c->base_val[P_DENSITY] = 68; /* 1/16 speed */
        c->base_val[P_VEL_MOD] = 80; /* +16 warm Dah accent */
        c->base_val[P_TEMPO] = 60;   /* 120 BPM */
        c->base_val[P_SWING] = 0;    /* Straight ITU telegraph */
        c->base_val[P_GATE] = 72;    /* 1/16 */
    }
    {
        uint32_t i;
        for (i = 0; i < NPARAMS; i++)
            c->val[i] = c->base_val[i];
    }
    c->ratio_lfo_q8 = 0;
}

static const char *const DITDAH_DEF_PHRASES[DITDAH_PHRASES] = {
    "E E E E E E T",             /* 01 */
    "EEEE T EEEE T",             /* 02 */
    "SOS",                       /* 03 */
    "0101100101",                /* 04 */
    "A N A N A N A",             /* 05 */
    "I T I T S M",               /* 06 */
    "H T H T H M",               /* 07 */
    "U D U D U D K",             /* 08 */
    "V V V O V V V",             /* 09 */
    "R K R K C Q",               /* 10 */
    "ACID HOUSE 1988",           /* 11 */
    "MUSIC NON STOP",            /* 12 */
    "WE ARE THE ROBOTS",         /* 13 */
    "ALL YOUR BASE ARE BELONG TO US", /* 14 */
    "CQ DX DE TRIPTYCH",         /* 15 */
    "GHOST IN THE SHELL",        /* 16 */
    "ELECTRIC SHEEP DREAM",      /* 17 */
    "I THINK THEREFORE I SYNTH", /* 18 */
    "SORRY DAVE I CANT DO THAT", /* 19 */
    "NO FUTURE NO PRESET",       /* 20 */
    "PLASTIC KEYS MIDI SOUL",    /* 21 */
    "SUBTERRANEAN HOMESICK",     /* 22 */
    "KOSMAR TRIPTYCH"            /* 23 */
};

static inline int midi_phrase_is_unchanged(const midi_cfg_t *c, uint32_t u)
{
    uint32_t k = 0;
    if (u >= DITDAH_PHRASES)
        return 0;
    while (DITDAH_DEF_PHRASES[u][k] || c->phrases[u][k]) {
        if (DITDAH_DEF_PHRASES[u][k] != c->phrases[u][k])
            return 0;
        k++;
        if (k >= DITDAH_MAX_LEN)
            break;
    }
    return 1;
}

static void midi_cfg_tail_defaults(midi_cfg_t *c);

static void midi_cfg_defaults(midi_cfg_t *c)
{
    static const uint8_t def_val[NPARAMS] = {
        26, 52, 6, 72, 0, 28, 96, 74, 48, 32, 28, 64, 68, 0, 28, 56, 0, 36, 0, 0};
    static const uint8_t def_cc[NPARAMS] = {
        1, 2, 73, 72, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 33};
    uint32_t i;

    for (i = 0; i < NPARAMS; i++) {
        c->val[i] = def_val[i];
        c->base_val[i] = def_val[i];
        c->cc_map[i] = def_cc[i];
    }
    for (i = 0; i < 16u; i++)
        c->lfo_depth[i] = 0;
    c->midi_ch = 0;
    c->bank = 0;
    c->mode = 0;
    c->reserved = 0;

    for (i = 0; i < NPARAMS; i++) {
        c->val[i] &= 0x7fu;
        c->base_val[i] &= 0x7fu;
    }
    c->bank %= NBANKS;
    c->mode %= NMODES;
    c->ratio_lfo_q8 = 0;
    {
        uint32_t u, k;
        for (u = 0; u < DITDAH_PHRASES; u++) {
            k = 0;
            while (DITDAH_DEF_PHRASES[u][k] && k + 1u < DITDAH_MAX_LEN) {
                c->phrases[u][k] = DITDAH_DEF_PHRASES[u][k];
                k++;
            }
            while (k < DITDAH_MAX_LEN)
                c->phrases[u][k++] = 0;
        }
    }
    c->magic = TRYP_CFG_MAGIC;
    midi_cfg_tail_defaults(c);
}

static const char *midi_param_name(uint32_t mode, uint32_t slot)
{
    static const char *const engine_triaxis[4] = {"AXIS", "MOTION", "DENSITY", "VOICING"};
    static const char *const engine_contura[4] = {"SCALE", "PHRASE", "DENSITY", "LAYERS"};
    static const char *const engine_ditdah[4] = {"INTERVAL", "PHRASE", "DENSITY", "ACCENT"};
    static const char *const arp_tgt[4] = {"A.MODE", "A.DIV", "A.SPAN", "A.GATE"};
    static const char *const seq_tgt[4] = {"S.STEP", "S.PACE", "S.PROG", "MUTATE"};

    if (slot >= VSLOT_ARP_BASE && slot < VSLOT_SEQ_BASE)
        return arp_tgt[slot - VSLOT_ARP_BASE];
    if (slot >= VSLOT_SEQ_BASE && slot < NVSLOTS)
        return seq_tgt[slot - VSLOT_SEQ_BASE];
    if (slot >= (uint32_t)NPARAMS)
        return "";
    if (slot < 4u || slot > 7u)
        return PARAM_NAMES[slot];
    if (mode == (uint32_t)MODE_TRIAXIS)
        return engine_triaxis[slot - 4u];
    if (mode == (uint32_t)MODE_CONTURA)
        return engine_contura[slot - 4u];
    if (mode == (uint32_t)MODE_DITDAH)
        return engine_ditdah[slot - 4u];
    return PARAM_NAMES[slot];
}

static inline uint32_t midi_div_ticks(const midi_cfg_t *c)
{
    static const uint32_t div_tbl[11] = {96u, 48u, 24u, 16u, 12u, 8u, 6u, 4u, 3u, 2u, 1u};
    uint32_t idx = ((uint32_t)c->val[P_GATE] * 11u) / 128u;

    if (idx > 10u)
        idx = 10u;
    return div_tbl[idx];
}

static inline void midi_ratio_cm_q8(uint32_t v, uint32_t *cm_out, uint32_t *mm_q8_out)
{
    fm_lfo_ratio_cm_q8(v, cm_out, mm_q8_out);
}

static inline int32_t midi_lfo_slot_wave_mod(midi_state_t *s, uint32_t slot, uint32_t ph, uint32_t cycle)
{
    uint32_t ph0 = ph;
    uint32_t off = (uint32_t)s->lfo_phase[slot] << 25;
    int32_t raw_q15;

    ph += off;
    if (ph < ph0)
        cycle++;
    raw_q15 = fm_lfo_eval_full_q15(ph, cycle, slot, s->lfo_shape[slot], s->lfo_skew[slot],
                                   s->lfo_algo[slot], s->lfo_ratio[slot], s->lfo_index[slot],
                                   s->lfo_fb[slot], s->lfo_fold[slot]);
    s->lfo_slew_q15[slot] = fm_lfo_slew_step(s->lfo_slew_q15[slot], raw_q15, s->lfo_smooth[slot]);
    return s->lfo_slew_q15[slot] >> 9;
}

static inline void midi_lfo_recall_target_rate(midi_cfg_t *c, midi_state_t *s)
{
    uint32_t ts = midi_lfo_target_slot(c);

    c->base_val[P_LFO_RATE] = s->lfo_rate[ts];
    c->val[P_LFO_RATE] = s->lfo_rate[ts];
    c->base_val[P_LFO_SHAPE] = s->lfo_shape[ts];
    c->val[P_LFO_SHAPE] = s->lfo_shape[ts];
}

static inline int32_t midi_oct_offset(const midi_cfg_t *c)
{
    int32_t o = (int32_t)c->val[P_OCTAVE] / 26 - 2;

    if (o < -2)
        o = -2;
    if (o > 2)
        o = 2;
    return o;
}

static void midi_state_init(midi_state_t *s, const midi_cfg_t *c)
{
    uint32_t i;

    s->learn_active = 0;
    s->learn_slot = 0;
    s->hud_param = 0;
    s->hud_cc = 0;
    s->hud_until_ms = 0;
    s->ext_sync = 0;
    s->playing = 0;
    s->bpm = 60u + c->val[P_TEMPO];
    s->last_f8_ms = 0;
    s->f8_dt_acc = 0;
    s->f8_count = 0;
    s->int_last_ms = 0;
    s->int_last_frames = 0;
    s->int_rem_ms = 0;
    s->step_ticks = 0;
    s->raw_tick = 0;
    s->swing_held = 0;
    s->lfo_ph = 0;
    s->lfo_last_ms = 0;
    s->lfo_rng = 0x12345678u;
    s->lfo_sh = 0;
    s->lfo_page = 0;
    for (i = 0; i < NVSLOTS; i++) {
        s->lfo_rate[i] = c->base_val[P_LFO_RATE];
        s->lfo_shape[i] = c->base_val[P_LFO_SHAPE];
        s->lfo_algo[i] = 0;
        s->lfo_ratio[i] = 52;
        s->lfo_index[i] = 64;
        s->lfo_fb[i] = 24;
        s->lfo_fold[i] = 0;
        s->lfo_phase[i] = 0;
        s->lfo_skew[i] = FM_LFO_SKEW_DEFAULT;
        s->lfo_smooth[i] = 0;
        s->lfo_ph_slot[i] = 0;
        s->lfo_cycle[i] = 0;
        s->lfo_sh_slot[i] = 0;
        s->lfo_rng_slot[i] = 0x12345678u + i * 0x9e3779b9u;
        s->lfo_slew_q15[i] = 0;
    }
    for (i = 0; i < 8u; i++)
        s->lfo_depth_ext[i] = 0;
}

static void midi_lfo_tick(midi_cfg_t *c, midi_state_t *s, uint32_t now_ms)
{
    uint32_t dt = now_ms - s->lfo_last_ms;
    uint32_t i, ti, prev_ph, ts, dt_eff, inc, slot;
    int any = 0;

    s->lfo_last_ms = now_ms;
    for (i = 0; i < NPARAMS; i++)
        c->val[i] = c->base_val[i];
    c->ratio_lfo_q8 = 0;

    dt_eff = dt > 50u ? 1u : dt;
    ts = midi_lfo_target_slot(c);

    fm_lfo_rates_sanitize(s->lfo_rate, NVSLOTS, s->bpm);

    for (ti = 0; ti < NLFO_TARGETS; ti++) {
        if (midi_lfo_get_depth(c, s, LFO_TGT[ti]) > 0u) {
            any = 1;
            break;
        }
    }

    for (ti = 0; ti < NLFO_TARGETS; ti++) {
        slot = LFO_TGT[ti];
        if (midi_lfo_get_depth(c, s, slot) == 0u && slot != ts)
            continue;
        inc = fm_lfo_slot_inc(s->lfo_rate[slot], s->bpm, slot) * dt_eff;
        prev_ph = s->lfo_ph_slot[slot];
        s->lfo_ph_slot[slot] += inc;
        if (s->lfo_ph_slot[slot] < prev_ph) {
            s->lfo_cycle[slot]++;
            s->lfo_rng_slot[slot] = s->lfo_rng_slot[slot] * 1664525u + 1013904223u;
            s->lfo_sh_slot[slot] =
                (int8_t)(((int32_t)((s->lfo_rng_slot[slot] >> 16) & 0x7Fu)) - 64);
        }
    }
    s->lfo_ph = s->lfo_ph_slot[ts];

    if (!any)
        return;

    for (i = 0; i < 16u; i++) {
        uint32_t d = midi_lfo_get_depth(c, s, i);
        int32_t wave, delta, v;

        if (!d)
            continue;
        wave = midi_lfo_slot_wave_mod(s, i, s->lfo_ph_slot[i], s->lfo_cycle[i]);
        delta = (wave * (int32_t)d) / 127;
        v = (int32_t)c->base_val[i] + delta;
        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
        c->val[i] = (uint8_t)v;
        if (i == (uint32_t)P_RATIO) {
            c->ratio_lfo_q8 = (int16_t)((wave * (int32_t)d * 12) / 127);
        }
    }
}

extern uint8_t g_mod_hold_slot;
extern uint32_t g_mod_hold_until;

#define MIDI_MOD_HOLD_MS 400u

static inline void midi_mod_hold_arm(const midi_cfg_t *c, const midi_state_t *s, uint32_t slot,
                                   uint32_t now_ms)
{
    if (midi_lfo_get_depth(c, s, slot) > 0u) {
        g_mod_hold_slot = (uint8_t)slot;
        g_mod_hold_until = now_ms + MIDI_MOD_HOLD_MS;
    }
}

static inline int midi_mod_hold_active(uint32_t slot, uint32_t now_ms)
{
    return (uint32_t)g_mod_hold_slot == slot && now_ms < g_mod_hold_until;
}

static inline int midi_in_ch_ok(const midi_cfg_t *c, uint8_t ch)
{
    if (c->midi_ch != 0 && c->midi_ch != (uint8_t)(ch + 1u))
        return 0;
    return 1;
}

static int midi_handle_cc(midi_cfg_t *c, midi_state_t *s, uint8_t ch, uint8_t cc, uint8_t val,
                          uint32_t now_ms)
{
    uint32_t i;

    if (!midi_in_ch_ok(c, ch))
        return 0;

    val &= 0x7fu;

    if (cc == 31u) {
        c->mode = (uint8_t)(val % NMODES);
        return 1;
    }

    if (s->learn_active && cc < 120u) {
        c->cc_map[s->learn_slot] = cc;
        s->learn_active = 0;
        s->hud_param = s->learn_slot;
        s->hud_cc = cc;
        s->hud_until_ms = now_ms + 1200u;
        return 2;
    }

    if (cc == 34u) {
        /* CC 34: Reset all LFO depths */
        midi_lfo_clear_all(c, s);
        s->hud_param = P_LFO_DEPTH;
        s->hud_cc = 34u;
        s->hud_until_ms = now_ms + 1200u;
        return 1;
    }

    for (i = 0; i < NPARAMS; i++) {
        if (c->cc_map[i] != cc)
            continue;
        c->base_val[i] = val;
        c->val[i] = val;
        midi_mod_hold_arm(c, s, i, now_ms);
        if (i == (uint32_t)P_LFO_TARGET) {
            uint32_t ts = midi_lfo_target_slot(c);
            c->base_val[P_LFO_DEPTH] = midi_lfo_get_depth(c, s, ts);
            c->val[P_LFO_DEPTH] = c->base_val[P_LFO_DEPTH];
            midi_lfo_recall_target_rate(c, s);
        } else if (i == (uint32_t)P_LFO_DEPTH) {
            uint32_t ts = midi_lfo_target_slot(c);
            midi_lfo_set_depth(c, s, ts, val);
        } else if (i == (uint32_t)P_LFO_RATE) {
            uint32_t ts = midi_lfo_target_slot(c);

            s->lfo_rate[ts] = fm_lfo_rate_clamp(val, s->bpm);
        } else if (i == (uint32_t)P_LFO_SHAPE) {
            uint32_t ts = midi_lfo_target_slot(c);
            s->lfo_shape[ts] = val;
        }
        if (i == (uint32_t)P_TEMPO && !s->ext_sync)
            s->bpm = 60u + val;
        s->hud_param = (uint8_t)i;
        s->hud_cc = cc;
        s->hud_until_ms = now_ms + 1200u;
        return 1;
    }

    return 0;
}

static void midi_clock_rx(midi_state_t *s, uint8_t st, uint32_t now_ms)
{
    if (st == 0xfa) {
        s->playing = 1;
        s->f8_count = 0;
        s->step_ticks++;
        return;
    }
    if (st == 0xfb) {
        s->playing = 1;
        return;
    }
    if (st == 0xfc) {
        s->playing = 0;
        return;
    }
    if (st != 0xf8)
        return;

    s->ext_sync = 1;
    s->step_ticks++;
    s->f8_count++;
    if (s->f8_count >= 6u) {
        uint32_t dt = now_ms - s->last_f8_ms;

        if (dt >= 50u && dt <= 600u) {
            uint32_t acc = s->f8_dt_acc;

            acc += (dt * 16u - acc) >> 3;
            s->f8_dt_acc = acc;
            if (acc != 0u)
                s->bpm = (uint16_t)(15000u * 16u / acc);
        }
        s->last_f8_ms = now_ms;
        s->f8_count = 0;
    }
}

#define MIDI_CLOCK_HZ 44100u
#define MIDI_CLOCK_MIN_FRAMES (60u * MIDI_CLOCK_HZ)

static void midi_clock_poll_frames(const midi_cfg_t *c, midi_state_t *s, uint32_t frames_now)
{
    uint32_t dframes;

    if (!s->playing) {
        s->int_last_frames = frames_now;
        return;
    }

    dframes = frames_now - s->int_last_frames;
    s->int_last_frames = frames_now;
    if (s->ext_sync)
        return;

    if (dframes > MIDI_CLOCK_HZ)
        dframes = MIDI_CLOCK_HZ;

    {
        uint32_t bpm24 = (uint32_t)s->bpm * 24u;

        if (bpm24 == 0u)
            bpm24 = 1u;
        s->int_rem_ms += dframes * bpm24;
        while (s->int_rem_ms >= MIDI_CLOCK_MIN_FRAMES) {
            s->step_ticks++;
            s->int_rem_ms -= MIDI_CLOCK_MIN_FRAMES;
        }
    }
}

static uint32_t midi_clock_poll(const midi_cfg_t *c, midi_state_t *s, uint32_t now_ms)
{
    uint32_t raw_n, i, out_ticks = 0;
    /* P_SWING (0..127) delays the odd 16th note (tick % 12 == 6) by 0..3 ticks (50%..75% swing) */
    uint32_t swing_delay = ((uint32_t)c->val[P_SWING] * 3u + 63u) / 127u;

    if (s->ext_sync && now_ms - s->last_f8_ms > 1500u)
        s->ext_sync = 0;

    if (!s->ext_sync)
        s->bpm = 60u + c->val[P_TEMPO];

    if (!s->playing) {
        s->int_last_ms = now_ms;
        s->step_ticks = 0;
        return 0;
    }

    raw_n = s->step_ticks;
    s->step_ticks = 0;

    for (i = 0; i < raw_n; i++) {
        uint32_t pos = s->raw_tick % 12u;
        s->raw_tick++;
        if (swing_delay > 0u && pos >= 6u && pos < 6u + swing_delay) {
            s->swing_held++;
        } else if (pos == 6u + swing_delay && s->swing_held > 0u) {
            out_ticks += 1u + s->swing_held;
            s->swing_held = 0;
        } else {
            out_ticks++;
        }
    }
    return out_ticks;
}

enum { PERF_PAGE_BANK = 0, PERF_PAGE_ARP = 1, PERF_PAGE_SEQ = 2, PERF_PAGE_REC = 3 };

enum {
    PERF_CC_ARP_MODE = 40u,
    PERF_CC_ARP_DIV = 41u,
    PERF_CC_ARP_SPAN = 42u,
    PERF_CC_ARP_GATE = 43u,
    PERF_CC_SEQ_STEPS = 44u,
    PERF_CC_SEQ_PACE = 45u,
    PERF_CC_SEQ_PROG = 46u,
    PERF_CC_SEQ_MUTATE = 47u,
    PERF_CC_REC_STATE = 48u,
    PERF_CC_REC_TARGET = 49u,
    PERF_CC_REC_LEN = 50u,
    PERF_CC_REC_DECAY = 51u
};

typedef struct {
    uint8_t page;
    uint8_t page_last;
    uint8_t arp[4];
    uint8_t arp_base[4];
    uint8_t seq[4];
    uint8_t seq_base[4];
    uint8_t rec[4];
    int8_t seq_steps[PERF_SEQ_MAX_STEPS];
    int8_t motion_buf[64];
    uint8_t motion_slot;
    uint8_t motion_base[NPARAMS];
    uint8_t seq_base_saved;
    uint8_t seq_root_captured;
    int8_t seq_mut_off;
    uint8_t arp_pat;
    int8_t arp_dir;
    uint32_t arp_acc;
    uint32_t arp_gate_left;
    uint32_t seq_acc;
    uint32_t seq_step_i;
    uint32_t rec_acc;
    uint32_t rec_bucket;
    uint32_t rec_loop_ticks;
    uint32_t perf_tick;
    uint8_t rec_user_step;
} perf_state_t;

static inline uint32_t perf_arp_div_ticks(uint8_t div_cc)
{
    static const uint32_t tbl[6] = {24u, 16u, 12u, 8u, 6u, 3u};
    uint32_t i = ((uint32_t)div_cc * 6u) / 128u;

    if (i > 5u)
        i = 5u;
    return tbl[i];
}

static inline uint32_t perf_seq_pace_idx(uint8_t pace_cc)
{
    uint32_t i = ((uint32_t)pace_cc * 7u) / 128u;

    return i > 6u ? 6u : i;
}

static inline uint32_t perf_seq_pace_ticks(uint8_t pace_cc)
{
    /* 1/16=6, 1/8=12, 1/4=24, 1/2B=48, 1Bar=96, 2Bar=192, 4Bar=384 */
    static const uint32_t tbl[7] = {6u, 12u, 24u, 48u, 96u, 192u, 384u};

    return tbl[perf_seq_pace_idx(pace_cc)];
}

static inline uint32_t perf_seq_nsteps(uint8_t steps_cc)
{
    static const uint8_t tbl[8] = {0u, 2u, 3u, 4u, 6u, 8u, 16u, 32u};
    uint32_t i = ((uint32_t)steps_cc) >> 4;

    if (i > 7u)
        i = 7u;
    return tbl[i];
}

static inline uint32_t perf_rec_loop_bars(uint8_t len_cc)
{
    static const uint32_t tbl[4] = {1u, 2u, 4u, 8u};
    uint32_t i = ((uint32_t)len_cc * 4u) / 128u;

    if (i > 3u)
        i = 3u;
    return tbl[i];
}

static inline void perf_init(perf_state_t *p)
{
    uint32_t i;

    p->page = PERF_PAGE_BANK;
    p->page_last = PERF_PAGE_BANK;
    p->arp[0] = 0;
    p->arp[1] = 64;
    p->arp[2] = 0;
    p->arp[3] = 64;
    p->arp_base[0] = 0;
    p->arp_base[1] = 64;
    p->arp_base[2] = 0;
    p->arp_base[3] = 64;
    p->seq[0] = 0;
    p->seq[1] = 82;
    p->seq[2] = 0;
    p->seq[3] = 0;
    p->seq_base[0] = 0;
    p->seq_base[1] = 82;
    p->seq_base[2] = 0;
    p->seq_base[3] = 0;
    p->rec[0] = 0;
    p->rec[1] = 0;
    p->rec[2] = 0;
    p->rec[3] = 127;
    for (i = 0; i < PERF_SEQ_MAX_STEPS; i++)
        p->seq_steps[i] = 0;
    for (i = 0; i < 64u; i++)
        p->motion_buf[i] = 0;
    p->motion_slot = 0;
    p->seq_base_saved = 60;
    p->seq_root_captured = 0;
    p->seq_mut_off = 0;
    p->arp_pat = 0;
    p->arp_dir = 1;
    p->arp_acc = 0;
    p->arp_gate_left = 0;
    p->seq_acc = 0;
    p->seq_step_i = 0;
    p->rec_acc = 0;
    p->rec_bucket = 0;
    p->rec_loop_ticks = 96u;
    p->perf_tick = 0;
    p->rec_user_step = 0;
}

/* Default tail for TRP2 stores (matches midi_state_init + perf_init + empty mode_mem). */
static void midi_cfg_tail_defaults(midi_cfg_t *c)
{
    midi_state_t s;
    perf_state_t p;
    uint32_t i, m;

    midi_state_init(&s, c);
    memcpy(c->lfo_rate, s.lfo_rate, sizeof c->lfo_rate);
    memcpy(c->lfo_shape, s.lfo_shape, sizeof c->lfo_shape);
    memcpy(c->lfo_algo, s.lfo_algo, sizeof c->lfo_algo);
    memcpy(c->lfo_ratio, s.lfo_ratio, sizeof c->lfo_ratio);
    memcpy(c->lfo_index, s.lfo_index, sizeof c->lfo_index);
    memcpy(c->lfo_fb, s.lfo_fb, sizeof c->lfo_fb);
    memcpy(c->lfo_fold, s.lfo_fold, sizeof c->lfo_fold);
    memcpy(c->lfo_phase, s.lfo_phase, sizeof c->lfo_phase);
    memcpy(c->lfo_skew, s.lfo_skew, sizeof c->lfo_skew);
    memcpy(c->lfo_smooth, s.lfo_smooth, sizeof c->lfo_smooth);
    memcpy(c->lfo_depth_ext, s.lfo_depth_ext, sizeof c->lfo_depth_ext);
    perf_init(&p);
    memcpy(c->arp_base, p.arp_base, 4u);
    memcpy(c->seq_base, p.seq_base, 4u);
    memcpy(c->rec, p.rec, 4u);
    memcpy(c->seq_steps, p.seq_steps, sizeof c->seq_steps);
    c->page = p.page;
    for (m = 0; m < NMODES; m++) {
        for (i = 0; i < 11u; i++)
            c->mode_mem[m][i] = 0u;
        c->mode_mem_ok[m] = 0u;
    }
}

static inline int perf_arp_active(const perf_state_t *p)
{
    return p->arp[0] > 0u;
}

static inline int perf_seq_active(const perf_state_t *p)
{
    return perf_seq_nsteps(p->seq[0]) > 0u;
}

static inline uint32_t perf_rec_state(const perf_state_t *p)
{
    uint32_t st = ((uint32_t)p->rec[0] * 5u) / 128u;

    return st > 4u ? 4u : st;
}

static inline int perf_rec_active(const perf_state_t *p)
{
    return perf_rec_state(p) >= 2u;
}

static inline int perf_is_page(const perf_state_t *p, uint8_t pg)
{
    return p->page == pg;
}

static inline int32_t perf_seq_harm_cycle_off(int32_t b, uint32_t pos, uint32_t cycle)
{
    (void)pos;
    (void)cycle;
    return b;
}

static inline void perf_seq_prog_offset(const perf_state_t *p, uint32_t step, int32_t *off_out)
{
    static const int8_t PROG0[8] = {0, -3, 2, -5, 0, -3, 2, 7};
    static const int8_t PROG1[8] = {0, -4, 3, -2, 0, 8, 3, -2};
    static const int8_t PROG2[8] = {0, 7, 2, -3, 4, -1, -6, 1};
    static const int8_t PROG3[8] = {0, 5, -2, 3, -4, 1, 6, -1};
    static const int8_t PROG4[8] = {0, 12, 0, -12, 0, 12, 0, -12};
    static const int8_t PROG5[8] = {0, 1, 2, 3, 4, 3, 2, 1};
    static const int8_t PROG6[8] = {0, 1, 7, 5, 0, 1, 7, 5};
    const int8_t *tbl = PROG0;
    uint32_t prog = ((uint32_t)p->seq[2] * 8u) / 128u;
    uint32_t pos;
    uint32_t cycle;
    int32_t b;

    if (prog > 7u)
        prog = 7u;
    if (prog == 1u)
        tbl = PROG1;
    else if (prog == 2u)
        tbl = PROG2;
    else if (prog == 3u)
        tbl = PROG3;
    else if (prog == 4u)
        tbl = PROG4;
    else if (prog == 5u)
        tbl = PROG5;
    else if (prog == 6u)
        tbl = PROG6;
    if (prog == 7u) {
        *off_out = (int32_t)p->seq_steps[step % PERF_SEQ_MAX_STEPS];
        return;
    }
    if (prog == 5u) {
        if (step < 16u)
            *off_out = (int32_t)PROG5[step & 7u];
        else if (step < 32u)
            *off_out = (int32_t)(-PROG5[step & 7u]);
        else
            *off_out = (int32_t)PROG5[step & 7u];
        return;
    }
    pos = step & 7u;
    cycle = (step >> 3) & 3u;
    b = (int32_t)tbl[pos];
    *off_out = perf_seq_harm_cycle_off(b, pos, cycle);
}

static inline int midi_perf_handle_cc(perf_state_t *p, midi_cfg_t *c, midi_state_t *s, uint8_t ch,
                                    uint8_t cc, uint8_t val, uint32_t now_ms)
{
    if (!midi_in_ch_ok(c, ch))
        return 0;

    val &= 0x7fu;
    if (cc == PERF_CC_ARP_MODE) {
        p->arp_base[0] = val;
        p->arp[0] = val;
        midi_mod_hold_arm(c, s, VSLOT_ARP_BASE, now_ms);
        s->hud_param = VSLOT_ARP_BASE;
        return 1;
    }
    if (cc == PERF_CC_ARP_DIV) {
        p->arp_base[1] = val;
        p->arp[1] = val;
        midi_mod_hold_arm(c, s, VSLOT_ARP_BASE + 1u, now_ms);
        s->hud_param = VSLOT_ARP_BASE + 1u;
        return 1;
    }
    if (cc == PERF_CC_ARP_SPAN) {
        p->arp_base[2] = val;
        p->arp[2] = val;
        midi_mod_hold_arm(c, s, VSLOT_ARP_BASE + 2u, now_ms);
        s->hud_param = VSLOT_ARP_BASE + 2u;
        return 1;
    }
    if (cc == PERF_CC_ARP_GATE) {
        p->arp_base[3] = val;
        p->arp[3] = val;
        midi_mod_hold_arm(c, s, VSLOT_ARP_BASE + 3u, now_ms);
        s->hud_param = VSLOT_ARP_BASE + 3u;
        return 1;
    }
    if (cc == PERF_CC_SEQ_STEPS) {
        p->seq_base[0] = val;
        p->seq[0] = val;
        midi_mod_hold_arm(c, s, VSLOT_SEQ_BASE, now_ms);
        s->hud_param = VSLOT_SEQ_BASE;
        return 1;
    }
    if (cc == PERF_CC_SEQ_PACE) {
        p->seq_base[1] = val;
        p->seq[1] = val;
        midi_mod_hold_arm(c, s, VSLOT_SEQ_BASE + 1u, now_ms);
        s->hud_param = VSLOT_SEQ_BASE + 1u;
        return 1;
    }
    if (cc == PERF_CC_SEQ_PROG) {
        p->seq_base[2] = val;
        p->seq[2] = val;
        midi_mod_hold_arm(c, s, VSLOT_SEQ_BASE + 2u, now_ms);
        s->hud_param = VSLOT_SEQ_BASE + 2u;
        return 1;
    }
    if (cc == PERF_CC_SEQ_MUTATE) {
        p->seq_base[3] = val;
        p->seq[3] = val;
        midi_mod_hold_arm(c, s, VSLOT_SEQ_BASE + 3u, now_ms);
        s->hud_param = VSLOT_SEQ_BASE + 3u;
        return 1;
    }
    if (cc == PERF_CC_REC_STATE) {
        p->rec[0] = val;
        return 1;
    }
    if (cc == PERF_CC_REC_TARGET) {
        p->rec[1] = val;
        return 1;
    }
    if (cc == PERF_CC_REC_LEN) {
        p->rec[2] = val;
        return 1;
    }
    if (cc == PERF_CC_REC_DECAY) {
        p->rec[3] = val;
        return 1;
    }
    return 0;
}

static inline uint8_t midi_rng_u7(midi_state_t *s)
{
    s->lfo_rng = s->lfo_rng * 1664525u + 1013904223u;
    return (uint8_t)((s->lfo_rng >> 16) & 127u);
}

static inline uint32_t midi_ditdah_phrase_idx(uint8_t param_b)
{
    uint32_t ph = ((uint32_t)param_b * DITDAH_PHRASES) / 128u;

    if (ph >= DITDAH_PHRASES)
        ph = DITDAH_PHRASES - 1u;
    return ph;
}

static inline uint8_t midi_ditdah_random_phrase_param(midi_state_t *s, uint8_t cur_param_b)
{
    uint32_t cur = midi_ditdah_phrase_idx(cur_param_b);
    uint32_t idx;

    if (DITDAH_PHRASES <= 1u)
        return cur_param_b;
    idx = (uint32_t)(midi_rng_u7(s) % (DITDAH_PHRASES - 1u));
    if (idx >= cur)
        idx++;
    return (uint8_t)((idx * 128u + 64u) / DITDAH_PHRASES);
}

static inline void midi_lfo_clear_depth_for_param(midi_cfg_t *c, midi_state_t *s, uint32_t param)
{
    uint32_t i;

    for (i = 0; i < NLFO_TARGETS; i++) {
        if (LFO_TGT[i] == param)
            midi_lfo_set_depth(c, s, LFO_TGT[i], 0);
    }
    {
        uint32_t ts = midi_lfo_target_slot(c);

        c->base_val[P_LFO_DEPTH] = midi_lfo_get_depth(c, s, ts);
        c->val[P_LFO_DEPTH] = c->base_val[P_LFO_DEPTH];
    }
}

static inline void midi_bank_copy_preset_slots(midi_cfg_t *c, uint8_t bank, uint8_t mode)
{
    midi_cfg_t tmp = *c;
    uint32_t k;

    midi_cfg_mode_preset(&tmp, mode);
    for (k = 0; k < 4u; k++) {
        uint32_t slot = (uint32_t)bank * 4u + k;
        c->base_val[slot] = tmp.base_val[slot];
        c->val[slot] = tmp.base_val[slot];
    }
}

/* Encoder detent size (matches dinghy knob_step_for, fast=1). */
static inline int32_t midi_knob_step_mag(const midi_cfg_t *c, uint32_t slot)
{
    if (slot == (uint32_t)P_OCTAVE)
        return 26;
    if (slot == (uint32_t)P_RATIO)
        return 1;
    if (slot == (uint32_t)P_GATE)
        return 12;
    if (slot == (uint32_t)P_LFO_TARGET)
        return 1;
    if (slot == (uint32_t)P_LFO_SHAPE)
        return 1;
    if (slot == (uint32_t)P_TEMPO)
        return 2;
    if (slot == (uint32_t)P_LFO_RATE || slot == (uint32_t)P_LFO_DEPTH)
        return 1;

    if (c->mode == (uint8_t)MODE_TRIAXIS) {
        if (slot == (uint32_t)P_PARAM_A)
            return c->base_val[P_DENSITY] == 0u ? 10 : 32;
        if (slot == (uint32_t)P_PARAM_B)
            return c->base_val[P_DENSITY] == 0u ? 42 : 22;
        if (slot == (uint32_t)P_DENSITY)
            return 19;
        if (slot == (uint32_t)P_VEL_MOD)
            return 16;
    } else if (c->mode == (uint8_t)MODE_CONTURA) {
        if (slot == (uint32_t)P_PARAM_A)
            return 7;
        if (slot == (uint32_t)P_PARAM_B)
            return 5;
        if (slot == (uint32_t)P_VEL_MOD)
            return 11;
    } else if (c->mode == (uint8_t)MODE_DITDAH) {
        if (slot == (uint32_t)P_PARAM_A)
            return 6;
        if (slot == (uint32_t)P_PARAM_B)
            return 5;
    }
    return 5;
}

static inline uint8_t midi_slot_random_u7(midi_cfg_t *c, midi_state_t *s, uint32_t slot)
{
    int32_t step;
    uint32_t idx, v;

    if (slot >= (uint32_t)NPARAMS)
        return midi_rng_u7(s);

    if (slot == (uint32_t)P_RATIO) {
        static const uint8_t anchor[8] = {0u, 8u, 15u, 25u, 40u, 52u, 96u, 127u};
        return anchor[midi_rng_u7(s) % 8u];
    }
    if (slot == (uint32_t)P_GATE) {
        idx = midi_rng_u7(s) % 11u;
        return (uint8_t)((idx * 128u) / 11u);
    }
    if (slot == (uint32_t)P_OCTAVE) {
        int32_t o = (int32_t)(midi_rng_u7(s) % 5u) - 2;
        return (uint8_t)((o + 2) * 26);
    }
    if (slot == (uint32_t)P_LFO_TARGET) {
        idx = midi_rng_u7(s) % NLFO_TARGETS;
        return midi_lfo_target_cc_from_idx(idx);
    }
    if (slot == (uint32_t)P_LFO_SHAPE)
        return FM_LFO_SHAPE_ANCHORS[midi_rng_u7(s) % FM_LFO_NSHAPE_ANCHORS];

    if (c->mode == (uint8_t)MODE_TRIAXIS) {
        if (slot == (uint32_t)P_PARAM_A) {
            if (c->base_val[P_DENSITY] == 0u) {
                idx = midi_rng_u7(s) % 12u;
                return (uint8_t)(idx * 10u);
            }
            idx = midi_rng_u7(s) % 4u;
            return (uint8_t)(idx * 32u + 16u);
        }
        if (slot == (uint32_t)P_PARAM_B) {
            if (c->base_val[P_DENSITY] == 0u) {
                idx = midi_rng_u7(s) % 3u;
                return (uint8_t)(idx * 42u + 21u);
            }
            idx = midi_rng_u7(s) % 6u;
            return (uint8_t)(idx * 22u + 11u);
        }
        if (slot == (uint32_t)P_DENSITY) {
            if ((midi_rng_u7(s) % 6u) == 0u)
                return 0;
            idx = 1u + (midi_rng_u7(s) % 6u);
            return (uint8_t)(((idx - 1u) * 127u) / 5u + 1u);
        }
        if (slot == (uint32_t)P_VEL_MOD) {
            idx = midi_rng_u7(s) % 4u;
            return (uint8_t)(idx * 32u + 16u);
        }
    } else if (c->mode == (uint8_t)MODE_CONTURA) {
        if (slot == (uint32_t)P_PARAM_A) {
            idx = midi_rng_u7(s) % 20u;
            return (uint8_t)((idx * 128u + 64u) / 20u);
        }
        if (slot == (uint32_t)P_PARAM_B) {
            idx = 3u + (midi_rng_u7(s) % 26u);
            return (uint8_t)(((idx - 3u) * 127u + 12u) / 25u);
        }
        if (slot == (uint32_t)P_VEL_MOD) {
            idx = midi_rng_u7(s) % 12u;
            return (uint8_t)((idx * 128u + 64u) / 12u);
        }
    } else if (c->mode == (uint8_t)MODE_DITDAH) {
        if (slot == (uint32_t)P_PARAM_A) {
            idx = midi_rng_u7(s) % 25u;
            return (uint8_t)((idx * 127u) / 24u);
        }
        if (slot == (uint32_t)P_PARAM_B)
            return midi_ditdah_random_phrase_param(s, c->base_val[P_PARAM_B]);
    }

    step = midi_knob_step_mag(c, slot);
    if (step < 1)
        step = 1;
    idx = midi_rng_u7(s) % ((uint32_t)127 / (uint32_t)step + 1u);
    v = idx * (uint32_t)step;
    return v > 127u ? 127u : (uint8_t)v;
}

static inline void midi_bank_random_slots(midi_cfg_t *c, midi_state_t *s, uint8_t bank)
{
    uint32_t k;

    bank %= (uint8_t)NBANKS;
    if (bank == (uint8_t)BANK_LFO)
        return;
    if (bank == (uint8_t)BANK_ENGINE && c->mode == (uint8_t)MODE_TRIAXIS) {
        static const uint8_t ord[4] = {2u, 0u, 1u, 3u}; /* DENSITY before A/B */

        for (k = 0; k < 4u; k++) {
            uint32_t slot = (uint32_t)bank * 4u + (uint32_t)ord[k];

            if (slot >= (uint32_t)NPARAMS)
                continue;
            {
                uint8_t v = midi_slot_random_u7(c, s, slot);

                c->base_val[slot] = v;
                c->val[slot] = v;
            }
        }
        return;
    }
    for (k = 0; k < 4u; k++) {
        uint32_t slot = (uint32_t)bank * 4u + k;

        if (slot >= (uint32_t)NPARAMS)
            continue;
        {
            uint8_t v = midi_slot_random_u7(c, s, slot);

            c->base_val[slot] = v;
            c->val[slot] = v;
        }
    }
}

static inline void midi_lfo_bank_reset(midi_cfg_t *c, midi_state_t *s)
{
    uint32_t i;

    midi_lfo_clear_all(c, s);
    c->base_val[P_LFO_TARGET] = 0;
    c->val[P_LFO_TARGET] = 0;
    c->base_val[P_LFO_RATE] = FM_LFO_RATE_DEFAULT;
    c->val[P_LFO_RATE] = FM_LFO_RATE_DEFAULT;
    c->base_val[P_LFO_SHAPE] = 0;
    c->val[P_LFO_SHAPE] = 0;
    for (i = 0; i < NVSLOTS; i++) {
        s->lfo_rate[i] = FM_LFO_RATE_DEFAULT;
        s->lfo_shape[i] = 0;
        s->lfo_phase[i] = 0;
        s->lfo_skew[i] = FM_LFO_SKEW_DEFAULT;
        s->lfo_smooth[i] = 0;
        s->lfo_slew_q15[i] = 0;
    }
    midi_lfo_recall_target_rate(c, s);
}

static inline void midi_lfo_bank_random(midi_cfg_t *c, midi_state_t *s)
{
    uint32_t ti;
    uint32_t slot;

    for (ti = 0; ti < NLFO_TARGETS; ti++) {
        slot = LFO_TGT[ti];
        midi_lfo_set_depth(c, s, slot, midi_rng_u7(s));
        s->lfo_rate[slot] = midi_rng_u7(s);
        s->lfo_shape[slot] = midi_rng_u7(s);
    }
    {
        uint32_t ts = midi_lfo_target_slot(c);

        c->base_val[P_LFO_DEPTH] = midi_lfo_get_depth(c, s, ts);
        c->val[P_LFO_DEPTH] = c->base_val[P_LFO_DEPTH];
        midi_lfo_recall_target_rate(c, s);
    }
}

static inline void perf_lfo_apply(perf_state_t *p, const midi_cfg_t *c, midi_state_t *s)
{
    uint32_t col;

    for (col = 0; col < 4u; col++) {
        uint32_t d;
        uint32_t arp_slot = VSLOT_ARP_BASE + col;
        uint32_t seq_slot = VSLOT_SEQ_BASE + col;
        int32_t wave, delta, v;

        p->arp[col] = p->arp_base[col];
        p->seq[col] = p->seq_base[col];

        d = midi_lfo_get_depth(c, s, arp_slot);
        if (d) {
            wave = midi_lfo_slot_wave_mod(s, arp_slot, s->lfo_ph_slot[arp_slot], s->lfo_cycle[arp_slot]);
            delta = (wave * (int32_t)d) / 127;
            v = (int32_t)p->arp_base[col] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            p->arp[col] = (uint8_t)v;
        }

        d = midi_lfo_get_depth(c, s, seq_slot);
        if (d) {
            wave = midi_lfo_slot_wave_mod(s, seq_slot, s->lfo_ph_slot[seq_slot], s->lfo_cycle[seq_slot]);
            delta = (wave * (int32_t)d) / 127;
            v = (int32_t)p->seq_base[col] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            p->seq[col] = (uint8_t)v;
        }
    }
}

static inline void perf_arp_reset_values(perf_state_t *p)
{
    p->arp[0] = 0;
    p->arp[1] = 64;
    p->arp[2] = 0;
    p->arp[3] = 64;
    p->arp_base[0] = 0;
    p->arp_base[1] = 64;
    p->arp_base[2] = 0;
    p->arp_base[3] = 64;
    p->arp_pat = 0;
    p->arp_dir = 1;
    p->arp_acc = 0;
    p->arp_gate_left = 0;
}

static inline void perf_seq_reset_meta(perf_state_t *p)
{
    p->seq[0] = 0;
    p->seq[1] = 82;
    p->seq[2] = 0;
    p->seq[3] = 0;
    p->seq_base[0] = 0;
    p->seq_base[1] = 82;
    p->seq_base[2] = 0;
    p->seq_base[3] = 0;
    p->seq_acc = 0;
    p->seq_step_i = 0;
}

static inline void perf_rec_reset_values(perf_state_t *p)
{
    uint32_t i;

    p->rec[0] = 0;
    p->rec[1] = 0;
    p->rec[2] = 0;
    p->rec[3] = 127;
    p->rec_acc = 0;
    p->rec_bucket = 0;
    p->rec_user_step = 0;
    for (i = 0; i < 64u; i++)
        p->motion_buf[i] = 0;
}

#endif /* MIDI_CTRL_H */
