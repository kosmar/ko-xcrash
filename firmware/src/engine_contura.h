/* SPDX-License-Identifier: GPL-3.0-only */
/* Mode 1: Contura lead + Umbra bass shadow + Volata staccato bursts. Include after platform.c, midi_ctrl.h, synth_fm2.h. */
#ifndef ENGINE_CONTURA_H
#define ENGINE_CONTURA_H

#include <stdint.h>
#include "midi_ctrl.h"
#include "synth_fm2.h"

enum {
    CONTURA_V_SOLO = 1u << 0,
    CONTURA_V_BASS = 1u << 1,
    CONTURA_V_HIGH = 1u << 2,
    CONTURA_V_MIRR = 1u << 3
};

static const char *const CONTURA_LAYER_LABELS[12] = {
    "Umbra", "Solo", "Duo", "Trio", "Canon", "Warm",
    "Contra", "Ground", "Air", "Ghost", "Volata", "Hollow"
};

static inline uint32_t contura_layer_idx(uint8_t cc_val)
{
    uint32_t i = ((uint32_t)cc_val * 12u) / 128u;

    return i > 11u ? 11u : i;
}

static inline uint32_t contura_layer_mask(uint8_t cc_val)
{
    static const uint8_t MASK[12] = {
        CONTURA_V_BASS,                                                    /* 0: BASS   */
        CONTURA_V_SOLO,                                                    /* 1: SOLO   */
        CONTURA_V_BASS | CONTURA_V_SOLO,                                   /* 2: B+SOLO */
        CONTURA_V_BASS | CONTURA_V_SOLO | CONTURA_V_HIGH,                  /* 3: TRIO   */
        CONTURA_V_BASS | CONTURA_V_SOLO | CONTURA_V_MIRR | CONTURA_V_HIGH, /* 4: CANON  */
        CONTURA_V_BASS | CONTURA_V_SOLO | CONTURA_V_MIRR,                  /* 5: WARM   */
        CONTURA_V_SOLO | CONTURA_V_MIRR,                                   /* 6: S+CAN  */
        CONTURA_V_BASS | CONTURA_V_MIRR,                                   /* 7: B+CAN  */
        CONTURA_V_SOLO | CONTURA_V_HIGH,                                   /* 8: S+HIGH */
        CONTURA_V_MIRR | CONTURA_V_HIGH,                                   /* 9: C+HIGH */
        CONTURA_V_HIGH,                                                    /* 10: HIGH  */
        CONTURA_V_BASS | CONTURA_V_HIGH                                    /* 11: B+HIGH */
    };

    return MASK[contura_layer_idx(cc_val)];
}

typedef struct {
    uint8_t scale; /* 0..19 */
    uint8_t phrase_len; /* 3..28 */
    uint8_t phrase_pos;
    uint8_t lead_note, bass_note, burst_note;
    int8_t lead_idx, bass_idx;
    uint8_t burst_left;
    uint8_t pool[48];
    uint8_t pool_len;
    uint8_t base; /* tonic MIDI */
    uint32_t tick_acc;
    uint32_t rnd;
    uint8_t motif[28]; /* repeating/evolving phrase pool indices (0..27) */
    uint8_t motif_ready;
    uint8_t hist[48]; /* recent lead pitches for UI */
    uint8_t hist_w;
} contura_state_t;

typedef struct {
    int16_t density_bias;
    uint16_t leap_q8;
    int16_t sustain_bias;
    uint16_t tonic_pull;
    uint8_t contour;
    uint16_t step_glue;
    uint16_t ornament;
} contura_feel_t;

static const char *const CONTURA_SCALE_LABELS[20] = {
    "Ionian", "Dorian", "Phrygian", "Mixolydian", "Aeolian", "Pent Maj", "Pent Min", "Blues Min",
    "In Sen", "Yo", "Hirajoshi", "Bhairav", "Kafi", "Bhupali", "Hijaz", "Bayati", "Rast", "Gamelan",
    "Hungarian", "Folk"};
/* 6 glyphs fit the bank box (FONT_S is 8 px). Prefix cuts made PENT MAJ and PENT MIN identical. */
static const char *const CONTURA_SCALE_SHORT[20] = {
    "Ionian", "Dorin", "Phryg", "Mixol", "Aeoln", "P.Maj", "P.Min", "Blues",
    "Insen", "Yo", "Hiraj", "Bhair", "Kafi", "Bhupal", "Hijaz", "Bayati", "Rast", "Gameln",
    "Hungar", "Folk"};

static const uint16_t CONTURA_SCALE_MASKS[20] = {
    0x0AD5u, 0x0B56u, 0x0D2Au, 0x0AD6u, 0x0B5Au, 0x0A94u, 0x0924u, 0x0972u, 0x0C92u, 0x0A94u,
    0x0CA2u, 0x0D69u, 0x0B56u, 0x0A94u, 0x0CDAu, 0x0D6Au, 0x0AD6u, 0x0D18u, 0x0B39u, 0x0EEAu};

static const contura_feel_t CONTURA_SCALE_FEELS[20] = {
    {6, 128, 0, 22, 0, 28, 8},    {5, 110, 6, 16, 2, 38, 11},  {-3, 96, 5, 28, 1, 44, 13},
    {13, 150, -6, 14, 0, 19, 9}, {-2, 112, 7, 27, 1, 34, 9},  {11, 90, -5, 19, 0, 56, 6},
    {8, 92, 3, 20, 1, 53, 7},    {-8, 170, -3, 17, 2, 16, 28}, {-14, 80, 11, 31, 1, 50, 16},
    {9, 100, -2, 16, 0, 47, 8},  {-11, 85, 9, 25, 3, 47, 23},  {-6, 140, 10, 44, 3, 31, 14},
    {3, 115, 4, 19, 2, 34, 13},  {10, 95, -4, 17, 0, 50, 6},   {-3, 185, 5, 34, 1, 22, 20},
    {0, 105, 3, 28, 1, 41, 22},  {4, 120, 8, 38, 0, 31, 11},  {16, 70, -6, 9, 3, 69, 34},
    {-6, 200, 9, 23, 0, 13, 16}, {14, 155, -6, 16, 2, 22, 13}};

static uint32_t contura_lcg(contura_state_t *s)
{
    s->rnd = s->rnd * 1664525u + 1013904223u;
    return s->rnd;
}

static uint32_t contura_mild7(uint8_t v)
{
    uint32_t t = v;

    return (t * t + t * 127u) / 254u;
}

static void contura_degrees(uint16_t mask, uint8_t *deg, uint32_t *n)
{
    uint32_t i, c = 0;

    for (i = 0; i < 12u; i++) {
        if ((mask >> (11u - i)) & 1u)
            deg[c++] = (uint8_t)i;
    }
    if (!c)
        deg[c++] = 0;
    *n = c;
}

static uint8_t contura_clamp_pool_note(int32_t n)
{
    if (n < 36)
        n = 36;
    if (n > 76)
        n = 76;
    return (uint8_t)n;
}

static void contura_build_pool(contura_state_t *s, uint16_t mask)
{
    uint8_t deg[12];
    uint32_t nd, d;
    int32_t o;
    int32_t start = (int32_t)s->base - 12;

    if (start < 24)
        start = 24;
    if (start > 68)
        start = 68;
    contura_degrees(mask, deg, &nd);
    s->pool_len = 0;
    for (o = 0; o <= 3; o++) {
        for (d = 0; d < nd; d++) {
            int32_t n = start + o * 12 + (int32_t)deg[d];

            if (n < 24 || n > 96)
                continue;
            if (s->pool_len < 48u)
                s->pool[s->pool_len++] = (uint8_t)n;
        }
    }
    if (!s->pool_len)
        s->pool[s->pool_len++] = contura_clamp_pool_note((int32_t)s->base);
}

static uint8_t contura_fold_bass(uint8_t n, uint8_t base)
{
    /* Keep Umbra bass shadow within -12 .. -2 semitones below tonic (1 octave / fifth below Lead) */
    int16_t lo = (int16_t)base - 12;
    int16_t hi = (int16_t)base - 2;

    while ((int16_t)n < lo)
        n += 12u;
    while ((int16_t)n > hi)
        n -= 12u;
    return n;
}

static uint8_t contura_fold_volata(uint8_t n, uint8_t base)
{
    /* Bell register C4..G5 — fold down from high pool degrees instead of thin FM squeaks */
    int16_t hi = (int16_t)base + 19;

    if (hi > 79)
        hi = 79;
    while ((int16_t)n > hi)
        n -= 12u;
    while ((int16_t)n < 60)
        n += 12u;
    return n;
}

static uint32_t contura_max_step(uint8_t interval, uint32_t pool_len, const contura_feel_t *feel)
{
    uint32_t max = pool_len > 1u ? pool_len - 1u : 1u;

    if (max > 24u)
        max = 24u;
    {
        uint32_t curved = contura_mild7(interval);
        uint32_t base = 1u + (curved * (max - 1u)) / 127u;
        uint32_t scaled = (base * feel->leap_q8) / 128u;

        if (scaled < 1u)
            scaled = 1u;
        if (scaled > max)
            scaled = max;
        return scaled;
    }
}

static int contura_rising(const contura_feel_t *feel, uint8_t step, uint8_t len)
{
    uint8_t l = len ? len : 1u;

    switch (feel->contour) {
    case 1:
        return step < l / 4u;
    case 2:
        return (step / 2u) & 1u ? 0 : 1;
    case 3:
        return step < l / 3u;
    default:
        return step < (l + 1u) / 2u;
    }
}

static uint32_t contura_nearest_tonic(const contura_state_t *s, uint32_t cur)
{
    uint8_t tonic = s->base % 12u;
    uint32_t i, best = cur, best_d = 0xFFFFFFFFu;

    if (!s->pool_len)
        return 0;
    if (cur >= s->pool_len)
        cur = s->pool_len - 1u;
    for (i = 0; i < s->pool_len; i++) {
        if (s->pool[i] % 12u != tonic)
            continue;
        {
            uint32_t d = i > cur ? i - cur : cur - i;

            if (d < best_d) {
                best_d = d;
                best = i;
            }
        }
    }
    return best;
}

static uint32_t contura_pick_idx(contura_state_t *s, uint32_t cur, uint32_t pool_len, uint32_t max_step,
                                 uint8_t interval, int rising, const contura_feel_t *feel)
{
    uint32_t rnd, r, step;
    int32_t signed_s, next;

    if (pool_len <= 1u)
        return 0;
    rnd = contura_lcg(s);
    r = (rnd >> 16) & 0x7Fu;

    if (max_step < 3u)
        max_step = 3u;
    step = 1u + ((rnd >> 8) % max_step);

    /* Bounce away from floor and ceiling so contour never flattens at 0 or pool_len-1 */
    if (cur <= 1u)
        rising = 1;
    else if (cur + 2u >= pool_len)
        rising = 0;
    else if (((rnd >> 23) & 3u) == 0u)
        rising = !rising; /* occasional neighbor turn for melodic life */

    signed_s = rising ? (int32_t)step : -(int32_t)step;
    (void)interval;
    (void)feel;

    next = (int32_t)cur + signed_s;
    if (next < 0)
        next = (int32_t)(1u + (r % (pool_len > 2u ? pool_len - 1u : 1u)));
    if (next >= (int32_t)pool_len)
        next = (int32_t)(pool_len - 2u);
    if (next == (int32_t)cur && pool_len > 2u)
        next = (cur + 1u < pool_len) ? (int32_t)(cur + 1u) : (int32_t)(cur - 1u);
    return (uint32_t)next;
}

static uint16_t contura_density_gate(uint8_t density_cc, const contura_feel_t *feel)
{
    uint32_t curved = contura_mild7(density_cc);
    int32_t g = 16 + (int32_t)(curved * 111u) / 127 + feel->density_bias / 4;

    if (g < 0)
        g = 0;
    if (g > 127)
        g = 127;
    return (uint16_t)g;
}

static int contura_step_active(uint32_t step_i, uint16_t gate)
{
    uint32_t thr;
    if (step_i == 0u)
        return 1;
    thr = (step_i * 79u + 23u) & 0x7Fu;
    return thr <= gate;
}

static uint8_t contura_midi_status(uint8_t layer, const midi_cfg_t *c)
{
    uint8_t ch = c->midi_ch;

    if (ch == 0u)
        return (uint8_t)(0x90u + layer);
    return (uint8_t)(0x90u + (ch - 1u));
}

static uint8_t midi_last[4]; /* lead=0, umbra=1, vox=2, canon=3 */

static int contura_midi_other_layer_same(uint8_t layer, const midi_cfg_t *c, uint8_t n)
{
    uint32_t j;
    uint32_t ch = contura_midi_status(layer, c) & 0x0Fu;

    for (j = 0; j < 4u; j++) {
        if (j == (uint32_t)layer)
            continue;
        if (midi_last[j] == n && (contura_midi_status((uint8_t)j, c) & 0x0Fu) == ch)
            return 1;
    }
    return 0;
}

static void contura_midi_release_layer(uint8_t layer, const midi_cfg_t *c)
{
    uint8_t n = midi_last[layer];
    uint32_t ch;

    if (!n)
        return;
    if (!contura_midi_other_layer_same(layer, c, n)) {
        ch = contura_midi_status(layer, c) & 0x0Fu;
        if (trm_is_on(ch, n))
            trm_off((uint32_t)contura_midi_status(layer, c) - 0x10u, n);
    }
    midi_last[layer] = 0;
}

static void contura_midi_release_all(const midi_cfg_t *c)
{
    uint32_t layer;

    for (layer = 0; layer < 4u; layer++)
        contura_midi_release_layer((uint8_t)layer, c);
}

static void contura_midi_layer_on(uint8_t layer, const midi_cfg_t *c, uint8_t n, uint8_t vel)
{
    uint8_t prev = midi_last[layer];
    uint32_t ch;

    if (prev) {
        ch = contura_midi_status(layer, c) & 0x0Fu;
        if (!contura_midi_other_layer_same(layer, c, prev) && trm_is_on(ch, prev))
            trm_off((uint32_t)contura_midi_status(layer, c) - 0x10u, prev);
    }
    trm_on(contura_midi_status(layer, c), n, vel);
    midi_last[layer] = n;
}

static uint8_t contura_clamp_note(int32_t n)
{
    while (n < 24)
        n += 12;
    while (n > 108)
        n -= 12;
    return (uint8_t)n;
}

static void contura_play_lead(contura_state_t *s, const midi_cfg_t *c, uint8_t note, uint8_t vel)
{
    uint32_t dummy = 0;
    uint8_t n = contura_clamp_note((int32_t)note + midi_oct_offset(c) * 12);

    synth_note_on_ex(0u, n, vel, 0, 256, c, &dummy);
    contura_midi_layer_on(0u, c, n, vel);
    s->lead_note = n;
}

static void contura_play_canon(contura_state_t *s, const midi_cfg_t *c, uint8_t note, uint8_t vel)
{
    uint32_t dummy = 0;
    uint8_t cn = contura_clamp_note((int32_t)note + midi_oct_offset(c) * 12);

    (void)s;
    /* Voice 1: warm canon/counterpoint voice (2 steps behind Lead, slightly softer & longer decay) */
    synth_note_on_ex(1u, cn, vel, -6, 180, c, &dummy);
    contura_midi_layer_on(3u, c, cn, vel);
}

static void contura_play_umbra(contura_state_t *s, const midi_cfg_t *c, uint8_t note, uint8_t vel)
{
    uint32_t dummy = 0;
    uint8_t bn = contura_fold_bass(note, s->base);

    bn = contura_clamp_note((int32_t)bn + midi_oct_offset(c) * 12);
    synth_note_on_ex(2u, bn, vel, -14, 220, c, &dummy);
    contura_midi_layer_on(1u, c, bn, vel);
    s->bass_note = bn;
}

static void contura_play_volata(contura_state_t *s, const midi_cfg_t *c, uint8_t note, uint8_t vel, int burst)
{
    uint32_t dummy = 0;
    uint8_t vn = contura_fold_volata(note, s->base);

    vn = contura_clamp_note((int32_t)vn + midi_oct_offset(c) * 12);
    if (vel < 85u)
        vel = 85u;
    if (vel > 115u)
        vel = 115u;
    /* Warm bell timbre; step hits sustain, 16th ornaments stay short but audible */
    synth_note_on_ex(3u, vn, vel, burst ? 10 : 12, burst ? 288 : 384, c, &dummy);
    contura_midi_layer_on(2u, c, vn, vel);
    s->burst_note = vn;
}

/* Triangle-wave velocity meander (-32..+32) phased per voice so voices breathe independently */
static int32_t contura_meander_wave(uint32_t ph8)
{
    uint32_t p = ph8 & 0xFFu;
    return p < 128u ? ((int32_t)p >> 1) - 32 : 96 - ((int32_t)p >> 1);
}

static uint8_t contura_vel_from_roll(uint32_t roll, uint8_t step, uint32_t idx, uint32_t pool_len,
                                     uint8_t voice_id, const contura_feel_t *feel)
{
    /* Phased per-voice dynamic wave (90 deg = 64 phase steps apart per voice!) + pitch-contour expression */
    uint32_t ph8 = (uint32_t)step * 29u + (uint32_t)voice_id * 64u + (roll & 0x1Fu);
    int32_t wave = contura_meander_wave(ph8);
    int32_t pitch_lift = pool_len > 1u ? (int32_t)((idx * 24u) / (pool_len - 1u)) - 12 : 0;
    int32_t v = 76 + wave + pitch_lift + feel->sustain_bias / 8;

    if (step == 0u && voice_id == 0u)
        v += 14;
    if (v < 34)
        v = 34;
    if (v > 125)
        v = 125;
    return (uint8_t)v;
}

static void contura_phrase_params(const midi_cfg_t *c, contura_state_t *s, uint8_t *scale_out,
                                  const contura_feel_t **feel_out)
{
    uint8_t sc = (uint8_t)((uint32_t)c->val[P_PARAM_A] * 20u / 128u);

    if (sc > 19u)
        sc = 19u;
    s->scale = sc;
    if (scale_out)
        *scale_out = sc;
    {
        uint8_t pl = (uint8_t)(3u + ((uint32_t)c->val[P_PARAM_B] * 25u) / 127u);

        if (pl < 3u)
            pl = 3u;
        if (pl > 28u)
            pl = 28u;
        s->phrase_len = pl;
    }
    if (feel_out)
        *feel_out = &CONTURA_SCALE_FEELS[sc];
}

static uint8_t contura_pick_umbra_note(contura_state_t *s, uint32_t lead_idx)
{
    uint32_t r = contura_lcg(s) & 0xFFu;
    uint32_t t_idx = contura_nearest_tonic(s, lead_idx);

    if (r < 115u) {
        /* Root anchor */
        return s->pool[t_idx];
    }
    if (r < 195u && s->pool_len > 2u) {
        /* Fifth / dominant degree in scale pool */
        uint32_t f_idx = t_idx + 3u < s->pool_len ? t_idx + 3u : (t_idx >= 3u ? t_idx - 3u : t_idx);
        return s->pool[f_idx];
    }
    /* Scalar approach note near root or lead */
    if (t_idx > 0u && (r & 1u))
        return s->pool[t_idx - 1u];
    if (t_idx + 1u < s->pool_len)
        return s->pool[t_idx + 1u];
    return s->pool[lead_idx];
}

static void contura_seed_motif(contura_state_t *s, const contura_feel_t *feel, uint8_t interval)
{
    uint32_t i;
    uint32_t plen = s->phrase_len >= 3u ? s->phrase_len : 8u;
    uint32_t cur = s->pool_len ? s->pool_len / 3u : 0u;
    uint32_t max_step = contura_max_step(interval, s->pool_len, feel);

    cur = contura_nearest_tonic(s, cur);
    for (i = 0; i < 28u; i++) {
        if (i == 0u) {
            s->motif[i] = (uint8_t)cur;
        } else {
            int rising = contura_rising(feel, (uint8_t)(i % plen), (uint8_t)plen);
            cur = contura_pick_idx(s, cur, s->pool_len, max_step, interval, rising, feel);
            s->motif[i] = (uint8_t)cur;
        }
    }
    s->motif_ready = 1u;
}

static void contura_hist_push(contura_state_t *s, uint8_t note)
{
    s->hist[s->hist_w % 48u] = note;
    if (s->hist_w < 48u)
        s->hist_w++;
}

static uint8_t contura_hist_lag(const contura_state_t *s, uint32_t lag)
{
    if (s->hist_w <= lag)
        return 0u;
    return s->hist[(s->hist_w - 1u - lag) % 48u];
}

static void contura_step(contura_state_t *s, const midi_cfg_t *c)
{
    const contura_feel_t *feel;
    uint8_t sc, prev_scale, prev_len;
    uint16_t gate;
    uint32_t roll, max_step, idx;
    uint32_t mask = contura_layer_mask(c->val[P_VEL_MOD]);
    /* Short phrases = tight interval contour; long phrases = wide leaps */
    uint8_t interval = (uint8_t)(16u + ((uint32_t)c->val[P_PARAM_B] * 100u) / 127u);
    uint8_t vel;

    if (interval > 127u)
        interval = 127u;
    prev_scale = s->scale;
    prev_len = s->phrase_len;
    contura_phrase_params(c, s, &sc, &feel);
    if (sc != prev_scale || s->pool_len == 0u) {
        contura_build_pool(s, CONTURA_SCALE_MASKS[sc]);
        s->motif_ready = 0u;
    }
    if (!s->motif_ready || s->phrase_len != prev_len)
        contura_seed_motif(s, feel, interval);

    gate = contura_density_gate(c->val[P_DENSITY], feel);
    roll = ((s->phrase_pos * 37u + (contura_lcg(s) >> 20)) & 0x7Fu);

    if (!contura_step_active(s->phrase_pos, gate)) {
        /* Rest slot: Umbra hybrid density fires a soft ghost bass note when DENSITY >= 56 */
        if ((mask & CONTURA_V_BASS) && !(mask & (CONTURA_V_SOLO | CONTURA_V_MIRR)) &&
            c->val[P_DENSITY] >= 56u && ((contura_lcg(s) & 0x7Fu) < c->val[P_DENSITY])) {
            uint32_t l_idx = s->lead_idx >= 0 ? (uint32_t)s->lead_idx : 0u;
            contura_play_umbra(s, c, contura_pick_umbra_note(s, l_idx), 44u);
        }
        s->phrase_pos++;
        if (s->phrase_pos >= s->phrase_len)
            s->phrase_pos = 0;
        return;
    }

    max_step = contura_max_step(interval, s->pool_len, feel);
    idx = s->motif[s->phrase_pos % 28u];
    if (idx >= s->pool_len)
        idx = s->pool_len / 2u;

    /* At last step of phrase, tonic_pull resolves toward tonic; keep other steps free */
    if (s->phrase_pos == (uint8_t)(s->phrase_len - 1u)) {
        if (((contura_lcg(s) >> 16) & 0x3Fu) < feel->tonic_pull) {
            idx = contura_nearest_tonic(s, idx);
        }
        /* Slowly mutate 1 interior note of the motif each full phrase loop so it evolves */
        if (s->phrase_len > 2u) {
            uint32_t m_slot = 1u + ((contura_lcg(s) >> 16) % (s->phrase_len - 1u));
            int m_rising = contura_rising(feel, (uint8_t)m_slot, s->phrase_len);
            uint32_t prev_i = s->motif[(m_slot - 1u) % 28u];
            s->motif[m_slot % 28u] =
                (uint8_t)contura_pick_idx(s, prev_i, s->pool_len, max_step, interval, m_rising, feel);
        }
    }

    s->lead_idx = (int8_t)idx;
    contura_hist_push(s, s->pool[idx]);

    vel = contura_vel_from_roll(roll, s->phrase_pos, idx, s->pool_len, 0u, feel);
    if (mask & CONTURA_V_SOLO)
        contura_play_lead(s, c, s->pool[s->lead_idx], vel);
    else {
        s->lead_note = 0;
        contura_midi_release_all(c);
    }

    if ((mask & CONTURA_V_MIRR) && s->pool_len > 1u) {
        uint8_t c_note;
        uint32_t m_idx;

        if (mask & CONTURA_V_SOLO) {
            m_idx = (s->pool_len - 1u) - idx;
            c_note = s->pool[m_idx];
        } else {
            c_note = contura_hist_lag(s, 2u);
            if (!c_note)
                c_note = s->pool[(s->pool_len - 1u) - idx];
        }
        {
            uint8_t mv = contura_vel_from_roll(roll, s->phrase_pos, idx, s->pool_len, 1u, feel);
            contura_play_canon(s, c, c_note, mv);
        }
    }

    if (mask & CONTURA_V_BASS) {
        int bass_every = !(mask & CONTURA_V_SOLO) || (s->phrase_pos == 0u) ||
                         ((s->phrase_pos & 3u) == 0u);

        if (bass_every) {
            uint8_t b_note = (s->phrase_pos == 0u) ? s->pool[contura_nearest_tonic(s, idx)]
                                                     : contura_pick_umbra_note(s, (uint32_t)s->lead_idx);
            uint8_t bv = contura_vel_from_roll(roll, s->phrase_pos, idx / 2u, s->pool_len, 2u, feel);

            contura_play_umbra(s, c, b_note, bv);
            s->bass_idx = s->lead_idx;
        }
    } else {
        s->bass_note = 0;
        s->bass_idx = -1;
    }

    if (mask & CONTURA_V_HIGH) {
        uint32_t v_idx = idx;
        uint8_t vv;

        if (v_idx + 1u < s->pool_len)
            v_idx += 1u;
        else if (v_idx > 0u)
            v_idx -= 1u;
        vv = contura_vel_from_roll(roll, s->phrase_pos, v_idx, s->pool_len, 3u, feel);
        if (vv < 88u)
            vv = 88u;
        if (vv > 112u)
            vv = 112u;
        contura_play_volata(s, c, s->pool[v_idx], vv, 0);
        if (mask == CONTURA_V_HIGH || !(mask & CONTURA_V_SOLO))
            s->burst_left = (uint8_t)(2u + ((contura_lcg(s) >> 12) & 1u));
    }

    s->phrase_pos++;
    if (s->phrase_pos >= s->phrase_len)
        s->phrase_pos = 0;
}

static void contura_volata_16th(contura_state_t *s, const midi_cfg_t *c)
{
    static const uint8_t DEG_W[6] = {8, 3, 5, 2, 4, 1};
    uint32_t r, pick, i, wsum = 0;
    uint32_t mask = contura_layer_mask(c->val[P_VEL_MOD]);
    uint32_t lay = contura_layer_idx(c->val[P_VEL_MOD]);
    uint32_t idx;
    uint32_t thr;

    if (!(mask & CONTURA_V_HIGH)) {
        s->burst_left = 0;
        s->burst_note = 0;
        return;
    }
    r = contura_lcg(s);
    if (!s->burst_left) {
        if (lay >= 10u)
            thr = 240u;
        else if (lay >= 8u)
            thr = 200u;
        else if (lay >= 3u)
            thr = 160u + lay * 6u;
        else
            thr = 255u;
        if ((r & 0xFFu) > thr)
            return;
        s->burst_left = (uint8_t)(1u + ((r >> 8) & 3u));
        if (lay >= 8u && s->burst_left < 2u)
            s->burst_left = 2u;
    }
    for (i = 0; i < 6u; i++)
        wsum += DEG_W[i];
    pick = (r >> 12) % wsum;
    idx = s->lead_idx >= 0 ? (uint32_t)s->lead_idx : 0u;
    for (i = 0; i < 6u; i++) {
        if (pick < DEG_W[i]) {
            /* Ornamental 16ths: +0..+2 pool steps above lead (stay in bell register after fold) */
            int32_t off = (int32_t)(i % 3u);

            idx = (uint32_t)((int32_t)idx + off);
            break;
        }
        pick -= DEG_W[i];
    }
    if (idx >= s->pool_len)
        idx = s->pool_len ? s->pool_len - 1u : 0u;
    {
        /* Independent Volata velocity meander (voice_id = 3) across burst steps */
        int32_t vw = contura_meander_wave((uint32_t)s->phrase_pos * 41u + (uint32_t)s->burst_left * 36u + 192u);
        int32_t vv = 92 + vw + (int32_t)((r >> 20) & 0x0Fu);
        if (vv < 85)
            vv = 85;
        if (vv > 115)
            vv = 115;
        contura_play_volata(s, c, s->pool[idx], (uint8_t)vv, 1);
    }
    s->burst_left--;
}

static void contura_init(contura_state_t *s, const midi_cfg_t *c)
{
    uint32_t i;

    contura_midi_release_all(c);
    s->scale = 4;
    s->phrase_len = 8;
    s->phrase_pos = 0;
    s->lead_note = 0;
    s->bass_note = 0;
    s->burst_note = 0;
    s->lead_idx = -1;
    s->bass_idx = -1;
    s->burst_left = 0;
    s->pool_len = 0;
    s->base = 57;
    s->tick_acc = 0;
    s->rnd = 0xC0FFEE01u;
    s->motif_ready = 0;
    s->hist_w = 0;
    for (i = 0; i < 48u; i++)
        s->hist[i] = 0;
    contura_build_pool(s, CONTURA_SCALE_MASKS[4]);
    s->lead_idx = (int8_t)(s->pool_len / 2u);
    contura_seed_motif(s, &CONTURA_SCALE_FEELS[4], 48u);
}

static void contura_set_tonic(contura_state_t *s, uint8_t note)
{
    int32_t d;
    uint32_t i;

    if (note > 127u)
        note = 127u;
    if (note == s->base)
        return;
    d = (int32_t)note - (int32_t)s->base;
    if (s->lead_note)
        s->lead_note = contura_clamp_pool_note((int32_t)s->lead_note + d);
    if (s->bass_note)
        s->bass_note = contura_clamp_pool_note((int32_t)s->bass_note + d);
    if (s->burst_note)
        s->burst_note = contura_clamp_pool_note((int32_t)s->burst_note + d);
    for (i = 0; i < 48u; i++) {
        if (s->hist[i])
            s->hist[i] = contura_clamp_pool_note((int32_t)s->hist[i] + d);
    }
    s->base = note;
    if (s->motif_ready)
        contura_build_pool(s, CONTURA_SCALE_MASKS[s->scale % 20u]);
}

static void contura_tick(contura_state_t *s, const midi_cfg_t *c, uint32_t step_ticks)
{
    uint32_t div = midi_div_ticks(c);
    uint32_t i;
    uint8_t sc = (uint8_t)((uint32_t)c->val[P_PARAM_A] * 20u / 128u);
    uint8_t pl = (uint8_t)(3u + ((uint32_t)c->val[P_PARAM_B] * 25u) / 127u);

    if (sc > 19u)
        sc = 19u;
    if (pl < 3u)
        pl = 3u;
    if (pl > 28u)
        pl = 28u;
    if (sc != s->scale || pl != s->phrase_len || !s->motif_ready) {
        uint8_t iv = (uint8_t)(16u + ((uint32_t)c->val[P_PARAM_B] * 100u) / 127u);
        s->scale = sc;
        s->phrase_len = pl;
        if (s->phrase_pos >= pl)
            s->phrase_pos = 0;
        contura_build_pool(s, CONTURA_SCALE_MASKS[sc]);
        contura_seed_motif(s, &CONTURA_SCALE_FEELS[sc], iv > 127u ? 127u : iv);
    }
    if (!div)
        div = 1u;
    for (i = 0; i < step_ticks; i++) {
        s->tick_acc++;
        if ((s->tick_acc % 6u) == 0u)
            contura_volata_16th(s, c);
        if (s->tick_acc >= div) {
            s->tick_acc %= div;
            contura_step(s, c);
        }
    }
}

static int32_t contura_step_x(uint32_t i, uint32_t plen, uint8_t swing)
{
    uint32_t denom = plen > 1u ? plen - 1u : 1u;
    int32_t x = 10 + (int32_t)((i * 220u) / denom);

    if ((i & 1u) && i + 1u < plen && swing > 0u) {
        int32_t next_x = 10 + (int32_t)(((i + 1u) * 220u) / denom);
        x += ((next_x - x) * (int32_t)swing) / 254;
    }
    return x;
}

/* Compute continuous shadow opacity color from E.MIX (0..127) and tap number (1 or 2) */
static uint16_t contura_echo_col(uint8_t echo_mix, uint32_t tap)
{
    uint32_t m = tap == 1u ? (uint32_t)echo_mix : ((uint32_t)echo_mix >> 1);
    uint32_t r = (36u * m) / 127u;
    uint32_t g = (148u * m) / 127u;
    uint32_t b = (210u * m) / 127u;
    return RGB(r, g, b);
}

/* Compute heat hue from DRIVE (0..127): 0 = Cyan (56,210,225) -> 64 = Amber (255,165,40) -> 127 = Hot Coral (255,60,90) */
static uint16_t contura_drive_col(uint8_t drive, uint32_t dim_shift)
{
    uint32_t r, g, b;
    if (drive < 64u) {
        uint32_t t = drive;
        r = 56u + (199u * t) / 64u;
        g = 210u - (45u * t) / 64u;
        b = 225u - (185u * t) / 64u;
    } else {
        uint32_t t = drive - 64u;
        r = 255u;
        g = 165u - (105u * t) / 63u;
        b = 40u + (50u * t) / 63u;
    }
    r >>= dim_shift;
    g >>= dim_shift;
    b >>= dim_shift;
    return RGB(r, g, b);
}

/* Draw line with DRIVE stroke thickness (1..3 px) */
static void contura_thick_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, uint32_t thick)
{
    cv_line(x0, y0, x1, y1, c);
    if (thick >= 2u)
        cv_line(x0, y0 + 1, x1, y1 + 1, c);
    if (thick >= 3u)
        cv_line(x0, y0 - 1, x1, y1 - 1, c);
}

static int32_t contura_y_draw(int32_t y)
{
    if (!cv_home_view)
        return y;
    return 72 + (y - 23) * 72 / 21;
}

static uint16_t contura_canon_col(void)
{
    return RGB(150, 95, 235);
}

static uint16_t contura_canon_trail_col(void)
{
    return RGB(205, 155, 255);
}

/* Middle-lane mini viz (y 21..47): Lead upper half, Canon mirrored + 2-step delayed in lower half */
static int32_t contura_lead_y_stage(uint32_t idx, uint32_t pool_max)
{
    int32_t y = 44 - (int32_t)((idx * 20u) / pool_max);

    if (y < 23)
        y = 23;
    if (y > 38)
        y = 38;
    return y;
}

static int32_t contura_canon_y_stage(uint32_t c_idx, uint32_t pool_max)
{
    int32_t ly = contura_lead_y_stage(c_idx, pool_max);
    int32_t cy = 66 - ly;

    if (cy < 20)
        cy = 20;
    if (cy > 46)
        cy = 46;
    return cy;
}

/* Home screen middle lane (y 79..127) */
static int32_t contura_lead_y_home(uint32_t idx, uint32_t pool_max)
{
    int32_t y = 124 - (int32_t)((idx * 42u) / pool_max);

    if (y < 82)
        y = 82;
    if (y > 101)
        y = 101;
    return y;
}

static int32_t contura_canon_y_home(uint32_t c_idx, uint32_t pool_max)
{
    int32_t ly = contura_lead_y_home(c_idx, pool_max);
    int32_t cy = 206 - ly;

    if (cy < 105)
        cy = 105;
    if (cy > 127)
        cy = 127;
    return cy;
}

static void contura_draw_stage(const contura_state_t *s, const midi_cfg_t *c)
{
    uint32_t i;
    uint32_t plen = s->phrase_len >= 3u ? s->phrase_len : 8u;
    uint32_t pool_max = s->pool_len > 1u ? s->pool_len - 1u : 1u;
    const char *label = CONTURA_SCALE_LABELS[s->scale % 20u];
    int32_t prev_x = -1, prev_y = -1, prev_cx = -1, prev_cy = -1;
    uint32_t mask = contura_layer_mask(c->val[P_VEL_MOD]);
    int has_h = (int)(mask & CONTURA_V_HIGH);
    int has_b = (int)(mask & CONTURA_V_BASS);
    int has_s = (int)(mask & CONTURA_V_SOLO);
    int has_c = (int)(mask & CONTURA_V_MIRR);
    uint8_t swing = c->val[P_SWING];
    uint8_t echo_mix = c->val[P_ECHO_MIX];
    uint8_t echo_t = c->val[P_ECHO_TIME];
    uint8_t drive = c->val[P_DRIVE];
    uint8_t timbre = c->val[P_TIMBRE];
    uint8_t decay = c->val[P_DECAY];
    int32_t det_px = ((int32_t)c->val[P_DETUNE] - 64) / 18; /* -3..+3 px stereo split */
    uint32_t thick = 1u + ((uint32_t)drive * 2u) / 96u;     /* 1..3 px stroke width */
    uint32_t tail_steps = 1u + ((uint32_t)decay * 3u) / 127u; /* 1..4 steps comet tail */
    uint16_t c_drv_hi = contura_drive_col(drive, 0u);
    uint16_t c_drv_dim = contura_drive_col(drive, 1u);
    /* 45-degree diagonal offset (dx == dy) scaled by E.TIME; opacity scaled continuously by E.MIX */
    int32_t echo_d = 2 + ((int32_t)echo_t * 7) / 127;
    uint16_t ecol1 = contura_echo_col(echo_mix, 1u);
    uint16_t ecol2 = contura_echo_col(echo_mix, 2u);
    uint16_t gate = contura_density_gate(c->val[P_DENSITY], &CONTURA_SCALE_FEELS[s->scale % 20u]);
    int32_t bar_w = (int32_t)(160u / (plen > 1u ? plen : 1u));

    if (bar_w < 4)
        bar_w = 4;
    if (bar_w > 16)
        bar_w = 16;

    cv_begin(240u, cv_home_view ? 216u : 88u, C_BLACK);
    if (!cv_home_view) {
        /* Lane dividers: Lane 1 Volata (0..20), Lane 2 Contura (21..48), Lane 3 Bass (49..68) */
        cv_line(4, 20, 235, 20, C_LINE);
        cv_line(4, 48, 235, 48, C_LINE);
        cv_line(4, 68, 235, 68, C_LINE);
    }

    for (i = 0; i < plen; i++) {
        uint32_t idx = s->motif[i % 28u];
        int32_t x = contura_step_x(i, plen, swing);
        uint8_t is_now = (i == s->phrase_pos);
        /* Distance backwards from playhead for DECAY comet tail */
        uint32_t age = (s->phrase_pos >= i) ? (s->phrase_pos - i) : (plen + s->phrase_pos - i);
        int in_tail = (age > 0u && age <= tail_steps);
        int is_on = contura_step_active(i, gate);

        if (idx > pool_max)
            idx = pool_max;

        /* Playhead vertical guide across all active lanes */
        if (is_now && !cv_home_view) {
            int32_t y0 = has_h ? 1 : 21;
            int32_t y1 = has_b ? 67 : 47;

            cv_line(x, y0, x, y1, C_LINE);
        }

        /* ---- LANE 1 (y = 1..19): VOLATA Staccato Spikes ---- */
        if (!cv_home_view && has_h) {
            int32_t vh = 4 + (int32_t)(((idx * 3u + i * 5u) % 14u));
            int32_t vy = 18 - vh;
            uint16_t vcol = !is_on ? C_DIM : (is_now ? C_WHITE : (in_tail ? c_drv_hi : c_drv_dim));

            if (is_on || has_h) {
                if (is_on && echo_mix > 2u) {
                    int32_t ex = x + echo_d, ey = vy + echo_d / 2;
                    if (ex < 236 && ey < 19)
                        cv_line(ex, 18, ex, ey, ecol1);
                }
                if (det_px != 0 && is_on)
                    cv_line(x + det_px, 18, x + det_px, vy, C_DIM);
                cv_line(x, 18, x, vy, vcol);
                if (thick >= 2u && is_on)
                    cv_line(x + 1, 18, x + 1, vy, vcol);
                if (is_on)
                    cv_rect(x - 1, vy, 3, 2, is_now ? C_ACC : vcol);
            }
        }

        /* ---- LANE 2 (y = 21..47): Lead melody ---- */
        if (has_s) {
            int32_t y = contura_lead_y_stage(idx, pool_max);

            if (prev_x >= 0) {
                int32_t yd = contura_y_draw(y);
                int32_t pyd = contura_y_draw(prev_y);

                if (echo_mix > 2u && is_on && !cv_home_view) {
                    if (echo_mix > 48u && x + echo_d * 2 < 236)
                        cv_line(prev_x + echo_d * 2, pyd + echo_d, x + echo_d * 2, yd + echo_d, ecol2);
                    if (x + echo_d < 236)
                        cv_line(prev_x + echo_d, pyd + (echo_d * 2) / 3, x + echo_d, yd + (echo_d * 2) / 3, ecol1);
                }
                if (det_px != 0 && is_on && !cv_home_view)
                    cv_line(prev_x, pyd + det_px, x, yd + det_px, c_drv_dim);

                contura_thick_line(prev_x, pyd, x, yd,
                                   !is_on ? C_LINE : (is_now || in_tail ? c_drv_hi : c_drv_dim), thick);
            }

            {
                int32_t yd = contura_y_draw(y);

                if (is_now && is_on) {
                    int32_t hr = 3 + ((int32_t)timbre * 4) / 127;

                    if (!cv_home_view && echo_mix > 2u && x + echo_d < 236)
                        cv_rect(x + echo_d - 2, yd + (echo_d * 2) / 3 - 2, 5, 5, ecol1);
                    if (timbre > 24u) {
                        cv_line(x - hr - 1, yd, x + hr + 1, yd, c_drv_hi);
                        cv_line(x, yd - hr - 1, x, yd + hr + 1, c_drv_hi);
                    }
                    cv_rect(x - 3, yd - 3, 7, 7, C_ACC);
                    cv_rect(x - 1, yd - 1, 3, 3, C_WHITE);
                } else if (is_on) {
                    if (!cv_home_view && echo_mix > 2u && x + echo_d < 236)
                        cv_rect(x + echo_d - 1, yd + (echo_d * 2) / 3 - 1, 2, 2, ecol1);
                    if (in_tail) {
                        int32_t sz = (age == 1u && decay > 48u) ? 5 : 3;

                        cv_rect(x - sz / 2, yd - sz / 2, sz, sz, age == 1u ? C_WHITE : c_drv_hi);
                    } else {
                        cv_rect(x - 1, yd - 1, 3, 3, c_drv_dim);
                    }
                } else {
                    cv_pset(x, yd, C_LINE);
                }
            }
            if (is_on) {
                prev_x = x;
                prev_y = y;
            }
        }

        /* ---- LANE 2: Canon mirror (2-step delay, inverted contour, violet) ---- */
        if (has_c) {
            uint32_t c_step = (i + plen - 2u) % plen;
            uint32_t c_idx = s->motif[c_step % 28u];
            int32_t cy, cyd;
            uint16_t c_vio = contura_canon_col();
            uint16_t c_tr = contura_canon_trail_col();

            if (c_idx > pool_max)
                c_idx = pool_max;
            cy = contura_canon_y_stage(c_idx, pool_max);
            cyd = contura_y_draw(cy);

            if (prev_cx >= 0) {
                int32_t pcyd = contura_y_draw(prev_cy);
                int in_canon_trail = (age > 0u && age <= 5u);

                if (in_canon_trail) {
                    cv_line(prev_cx, pcyd, x, cyd, c_tr);
                    cv_line(prev_cx, pcyd + 1, x, cyd + 1, c_tr);
                } else {
                    cv_line(prev_cx, pcyd, x, cyd, c_vio);
                }
            }
            if (is_now) {
                cv_rect(x - 2, cyd - 2, 5, 5, c_tr);
                cv_rect(x - 1, cyd - 1, 3, 3, C_WHITE);
            } else if (age > 0u && age <= 5u) {
                cv_rect(x - 1, cyd - 1, 3, 3, c_tr);
            }
            prev_cx = x;
            prev_cy = cy;
        }

        /* ---- LANE 3 (y = 49..67): BASS Step-Terrace Blocks ---- */
        if (!cv_home_view && has_b) {
            uint32_t deg_mod = (i == 0u) ? 0u : ((idx + i) % 3u);
            int32_t by = deg_mod == 0u ? 62 : (deg_mod == 1u ? 57 : 52);
            uint16_t bcol = !is_on ? C_DIM : (is_now ? C_WHITE : (in_tail ? c_drv_hi : c_drv_dim));
            int32_t bx = x - bar_w / 2;

            if (is_on && echo_mix > 2u) {
                int32_t ex = bx + echo_d, ey = by + echo_d / 2;

                if (ex + bar_w < 236 && ey < 66)
                    cv_rect(ex, ey, bar_w, 2, ecol1);
            }
            if (det_px != 0 && is_on)
                cv_line(bx, by + det_px, bx + bar_w, by + det_px, c_drv_dim);
            cv_rect(bx, by, bar_w, (int32_t)thick + (is_now ? 2 : 1), bcol);
            if (is_now)
                cv_rect(bx, by, bar_w, 2, C_ACC);
        }
    }

    if (cv_home_view) {
        int32_t tw = text_w(&FONT_S, label);
        cv_text((240 - tw) / 2, 176, &FONT_S, label, C_HI);
        cv_blit(0, 24);
    } else {
        cv_text(6, 70, &FONT_S, label, C_HI);
        {
            const char *ltxt = CONTURA_LAYER_LABELS[contura_layer_idx(c->val[P_VEL_MOD])];

            cv_text(234 - text_w(&FONT_S, ltxt), 70, &FONT_S, ltxt, C_AMB);
        }
        cv_blit_engine_diagram();
    }
}

#endif /* ENGINE_CONTURA_H */
