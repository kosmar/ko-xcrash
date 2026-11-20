/* SPDX-License-Identifier: GPL-3.0-only */
/* TRIAXIS / TRIPTYCH engine: Axis Matrix + Giant Steps + Vamp comping grooves (integer only). */
#ifndef ENGINE_TRIPTYCH_H
#define ENGINE_TRIPTYCH_H

#include <stdint.h>
#include "midi_ctrl.h"
#include "synth_fm2.h"

enum {
    CT_QUAL_MAJ7 = 0,
    CT_QUAL_DOM7 = 1,
    CT_QUAL_MIN7 = 2,
    CT_REST_CODE = 0x80u
};

typedef struct {
    uint8_t root; /* MIDI note, default 48 */
    uint8_t step;
    uint8_t hold_left;
    uint8_t arp_pos;
    uint8_t sounding[4];
    uint8_t n_sound;
    uint8_t voicing;
    uint8_t motion;
    uint8_t density; /* 0..6, 0 = axis mode */
    uint8_t center;  /* 0..2 */
    uint8_t func;    /* 0=I 1=V 2=ii */
    uint8_t approach;
    uint8_t interval; /* 3,4,5,6 */
    uint8_t vamp_idx;
    uint32_t tick_acc;
    uint32_t rnd;
    char chord_name[12];
} triptych_state_t;

typedef struct {
    uint8_t center;
    uint8_t root_offset;
    uint8_t quality;
    uint8_t hold;
} ct_cycle_step_t;

static uint32_t ct_lcg(triptych_state_t *s)
{
    s->rnd = s->rnd * 1664525u + 1013904223u;
    return s->rnd;
}

static uint32_t ct_roll(triptych_state_t *s)
{
    return (ct_lcg(s) >> 16) % 4096u;
}

static uint8_t ct_interval_from_cc(uint8_t v)
{
    /* 0..31: Major 3rd (4 st, classic Giant Steps triangle)
     * 32..63: Minor 3rd (3 st, diminished axis)
     * 64..95: Fourths (5 st)
     * 96..127: Tritone (6 st) */
    static const uint8_t iv[4] = {4u, 3u, 5u, 6u};
    return iv[(v >> 5) & 3u];
}

static void ct_axis_from_cc(uint8_t center_cc, uint8_t motion_cc, uint8_t *center, uint8_t *func,
                            uint8_t *approach)
{
    uint32_t f = (uint32_t)center_cc * 32u;
    uint32_t zone = f / 1366u;

    if (zone > 2u)
        zone = 2u;
    {
        uint32_t within = f % 1366u;
        *approach = (uint8_t)(within < 200u || within > 1166u);
    }
    *center = (uint8_t)zone;
    if (motion_cc < 43u)
        *func = 0;
    else if (motion_cc < 85u)
        *func = 1;
    else
        *func = 2;
}

static uint8_t ct_center_offset(uint8_t center, uint8_t interval)
{
    return (uint8_t)(((uint32_t)center * (uint32_t)interval) % 12u);
}

static uint8_t ct_func_root_offset(uint8_t center, uint8_t func, uint8_t interval)
{
    uint8_t co = ct_center_offset(center, interval);

    if (func == 1u)
        return (uint8_t)((co + 7u) % 12u);
    if (func == 2u)
        return (uint8_t)((co + 2u) % 12u);
    return co;
}

static uint8_t ct_func_quality(uint8_t func, uint8_t approach)
{
    if (approach)
        return CT_QUAL_DOM7;
    if (func == 1u)
        return CT_QUAL_DOM7;
    if (func == 2u)
        return CT_QUAL_MIN7;
    return CT_QUAL_MAJ7;
}

static uint32_t ct_build_cycle(uint8_t interval, uint8_t density, ct_cycle_step_t *out, uint32_t cap)
{
    uint32_t n = 0;
    uint32_t c;
    uint8_t leading = 0;

    interval = interval < 1u ? 1u : interval;
    if (density > 6u)
        density = 6u;

    for (c = 0; c < 3u; c++) {
        uint8_t co = ct_center_offset((uint8_t)c, interval);
        struct {
            uint8_t on;
            uint8_t ro;
            uint8_t q;
        } slots[3];

        slots[0].on = density >= (uint8_t)(4u + c);
        slots[0].ro = (uint8_t)((co + 2u) % 12u);
        slots[0].q = CT_QUAL_MIN7;
        slots[1].on = density > c;
        slots[1].ro = (uint8_t)((co + 7u) % 12u);
        slots[1].q = CT_QUAL_DOM7;
        slots[2].on = 1;
        slots[2].ro = co;
        slots[2].q = CT_QUAL_MAJ7;

        {
            uint32_t k;
            for (k = 0; k < 3u; k++) {
                if (slots[k].on) {
                    if (n < cap) {
                        out[n].center = (uint8_t)c;
                        out[n].root_offset = slots[k].ro;
                        out[n].quality = slots[k].q;
                        out[n].hold = 0;
                        n++;
                    }
                } else if (n > 0u) {
                    out[n - 1u].hold++;
                } else {
                    leading++;
                }
            }
        }
    }
    if (n > 0u && leading > 0u)
        out[n - 1u].hold += leading;
    return n;
}

static uint32_t ct_chord_intervals(uint8_t quality, uint8_t voicing, int16_t *iv, uint32_t cap)
{
    static const int16_t Q4[4] = {0, 5, 10, 15};
    uint32_t n = 0;

    if (voicing > 3u)
        voicing = 3u;
    if (voicing == 3u) {
        for (; n < 4u && n < cap; n++)
            iv[n] = Q4[n];
        return n;
    }
    if (voicing == 0u) {
        if (quality == CT_QUAL_MIN7) {
            iv[0] = 0;
            iv[1] = 3;
            iv[2] = 7;
            return 3;
        }
        iv[0] = 0;
        iv[1] = 4;
        iv[2] = 7;
        return 3;
    }
    if (voicing == 1u) {
        if (quality == CT_QUAL_MIN7) {
            iv[0] = 0;
            iv[1] = 3;
            iv[2] = 7;
            iv[3] = 10;
            return 4;
        }
        if (quality == CT_QUAL_DOM7) {
            iv[0] = 0;
            iv[1] = 4;
            iv[2] = 7;
            iv[3] = 10;
            return 4;
        }
        iv[0] = 0;
        iv[1] = 4;
        iv[2] = 7;
        iv[3] = 11;
        return 4;
    }
    /* drop-2 7th */
    if (quality == CT_QUAL_MIN7) {
        iv[0] = 0;
        iv[1] = 7;
        iv[2] = 10;
        iv[3] = 15;
        return 4;
    }
    if (quality == CT_QUAL_DOM7) {
        iv[0] = 0;
        iv[1] = 7;
        iv[2] = 10;
        iv[3] = 16;
        return 4;
    }
    iv[0] = 0;
    iv[1] = 7;
    iv[2] = 11;
    iv[3] = 16;
    return 4;
}

static uint32_t ct_build_chord(uint8_t root, uint8_t quality, uint8_t voicing, uint8_t *notes,
                               uint32_t cap)
{
    int16_t iv[4];
    uint32_t ni = ct_chord_intervals(quality, voicing, iv, 4u);
    uint32_t i, n = 0;
    int16_t r = (int16_t)root;

    for (i = 0; i < ni && n < cap; i++) {
        int16_t note = r + iv[i];

        if (note >= 0 && note <= 127) {
            notes[n] = (uint8_t)note;
            n++;
        }
    }
    return n;
}

static uint8_t ct_fold_warm_register(uint8_t n)
{
    int32_t v = (int32_t)n;

    while (v < 45)
        v += 12;
    while (v > 76)
        v -= 12;
    if (v < 0)
        v = 0;
    if (v > 127)
        v = 127;
    return (uint8_t)v;
}

static uint32_t ct_voice_cost(const uint8_t *cand, uint32_t cn, const uint8_t *prev, uint32_t pn)
{
    uint32_t cost = 0, i;

    for (i = 0; i < cn; i++) {
        uint32_t best = 0xFFFFFFFFu, j;
        int16_t n = (int16_t)cand[i];

        for (j = 0; j < pn; j++) {
            int16_t d = n - (int16_t)prev[j];

            if (d < 0)
                d = -d;
            if ((uint32_t)d < best)
                best = (uint32_t)d;
        }
        cost += best;
    }
    return cost;
}

static uint32_t ct_build_chord_led(uint8_t root, uint8_t quality, uint8_t voicing,
                                   const uint8_t *prev, uint32_t pn, uint8_t *notes, uint32_t cap)
{
    uint8_t base[4];
    uint32_t bn = ct_build_chord(root, quality, voicing, base, 4u);
    uint32_t best_n = bn, best_cost = 0xFFFFFFFFu;
    uint8_t best[4];
    int16_t oct;
    uint32_t inv;

    if (bn == 0u)
        return 0;
    if (pn == 0u) {
        uint32_t i;
        for (i = 0; i < bn && i < cap; i++)
            notes[i] = ct_fold_warm_register(base[i]);
        return bn < cap ? bn : cap;
    }

    for (oct = -24; oct <= 24; oct += 12) {
        for (inv = 0; inv < bn; inv++) {
            uint8_t cand[4];
            uint32_t cn = 0, i;
            uint32_t cost;

            for (i = 0; i < bn; i++) {
                int16_t v = (int16_t)base[i] + oct + (i < inv ? 12 : 0);

                if (v < 0 || v > 127)
                    break;
                cand[cn++] = (uint8_t)v;
            }
            if (cn != bn)
                continue;
            cost = ct_voice_cost(cand, cn, prev, pn);
            if (cost < best_cost) {
                best_cost = cost;
                best_n = cn;
                {
                    uint32_t k;
                    for (k = 0; k < cn; k++)
                        best[k] = cand[k];
                }
            }
        }
    }
    {
        uint32_t i;
        for (i = 0; i < best_n && i < cap; i++)
            notes[i] = ct_fold_warm_register(best[i]);
    }
    return best_n < cap ? best_n : cap;
}

static uint8_t ct_vel7(uint8_t base, uint16_t feel, uint8_t strong, uint32_t roll)
{
    int32_t b = (int32_t)base;
    int32_t s = ((int32_t)feel * 255) / 4095;
    int32_t shape = strong ? 400 : -500;
    int32_t v = b + (b * shape * s) / (1000 * 255);
    int32_t jitter = ((int32_t)(roll % 4096u) * 2000) / 4095 - 1000;
    int32_t jb = (b * jitter) / 1000;

    v += (jb * 8 * s) / (100 * 255);
    if (v < 1)
        v = 1;
    if (v > 127)
        v = 127;
    return (uint8_t)v;
}

static uint8_t ct_step_div_mult(uint8_t motion, uint8_t quality)
{
    if (motion >= 1u && quality == CT_QUAL_MAJ7)
        return 2u;
    return 1u;
}

static uint8_t ct_tritone_root(uint8_t root, uint8_t quality, uint8_t motion, uint32_t roll)
{
    if (motion < 3u || quality != CT_QUAL_DOM7 || roll >= 1365u)
        return root;
    if (root <= 121u)
        return root + 6u;
    return root - 6u;
}

static void ct_pc_name(uint8_t pc, char *s)
{
    static const char *const NM[12] = {"C", "C#", "D", "D#", "E", "F",
                                       "F#", "G", "G#", "A", "A#", "B"};
    const char *p = NM[pc % 12u];
    s[0] = *p++;
    if (*p) {
        s[1] = *p++;
        s[2] = 0;
    } else
        s[1] = 0;
}

static void ct_format_chord(uint8_t root, uint8_t quality, char *buf, uint32_t n)
{
    char *p = buf;
    uint32_t i = 0;

    ct_pc_name(root % 12u, buf);
    while (buf[i] && i + 1u < n)
        i++;
    p = buf + i;
    /* Font folds a-z to A-Z, so "m7" and "M7" drew as the same badge. */
    if (quality == CT_QUAL_MIN7) {
        if (i + 3u < n) {
            p[0] = '-';
            p[1] = '7';
            p[2] = 0;
        }
    } else if (quality == CT_QUAL_DOM7) {
        if (i + 2u < n) {
            p[0] = '7';
            p[1] = 0;
        }
    } else if (i + 4u < n) {
        p[0] = 'M';
        p[1] = 'A';
        p[2] = '7';
        p[3] = 0;
    }
}

static uint8_t ct_midi_ch(const midi_cfg_t *c)
{
    return c->midi_ch ? (uint8_t)(c->midi_ch - 1u) : 0u;
}

static void ct_silence(triptych_state_t *s, const midi_cfg_t *c)
{
    uint32_t i;
    uint8_t ch = ct_midi_ch(c);

    for (i = 0; i < s->n_sound; i++) {
        synth_note_off(s->sounding[i]);
        trm_off(0x80u | ch, s->sounding[i]);
    }
    s->n_sound = 0;
}

static void ct_play_notes(triptych_state_t *s, const midi_cfg_t *c, const uint8_t *notes,
                          uint32_t n, uint8_t vel, uint32_t voice_mask)
{
    uint32_t i;
    uint8_t ch = ct_midi_ch(c);
    uint32_t last = 255u;

    ct_silence(s, c);
    for (i = 0; i < n && i < 4u; i++) {
        uint32_t hint = 0xFFFFFFFFu;

        if (voice_mask & (1u << i))
            hint = i;
        synth_note_on_ex(hint, notes[i], vel, 0, 256, c, &last);
        trm_on(0x90u | ch, notes[i], vel);
        s->sounding[i] = notes[i];
    }
    s->n_sound = (uint8_t)(n < 4u ? n : 4u);
}

/* Comping groove steps: 1..2 = chord hit (duration in steps), CT_REST_CODE | n = n-step breath (1..2 steps max!) */
static const uint8_t CT_VAMP_SYNC[8] = {1, 2, 1, CT_REST_CODE | 1u, 2, 1, 1, CT_REST_CODE | 1u};
static const uint8_t CT_VAMP_LAID[8] = {2, 1, CT_REST_CODE | 1u, 2, 1, 2, CT_REST_CODE | 1u, 1};

static uint8_t ct_vamp_weight(const triptych_state_t *s, uint8_t motion)
{
    if (motion == 5u)
        return CT_VAMP_LAID[s->vamp_idx & 7u];
    return CT_VAMP_SYNC[s->vamp_idx & 7u];
}

static uint8_t ct_vamp_is_rest(uint8_t w)
{
    return w == 0u || w >= CT_REST_CODE;
}

static uint8_t ct_vamp_rest_holds(uint8_t w)
{
    uint8_t h = w & 0x7Fu;
    return h > 2u ? 2u : (h == 0u ? 1u : h);
}

static uint8_t ct_arp_slot(uint32_t len, uint32_t pos)
{
    /* up then down (skip duplicate ends) */
    if (len <= 1u)
        return 0;
    {
        uint32_t up = len;
        uint32_t down = len > 2u ? len - 2u : 0u;
        uint32_t total = up + down;

        if (total == 0u)
            return 0;
        pos %= total;
        if (pos < up)
            return (uint8_t)pos;
        return (uint8_t)(up + down - pos);
    }
}

static void ct_read_params(triptych_state_t *s, const midi_cfg_t *c)
{
    uint8_t vm = c->val[P_VEL_MOD];

    s->voicing = vm >> 5;
    if (s->voicing > 3u)
        s->voicing = 3u;

    if (c->val[P_DENSITY] == 0u) {
        s->density = 0;
        ct_axis_from_cc(c->val[P_PARAM_A], c->val[P_PARAM_B], &s->center, &s->func, &s->approach);
        s->interval = 4u; /* Major-3rd triangle in manual Axis navigator */
    } else {
        uint32_t d = 1u + (((uint32_t)c->val[P_DENSITY] - 1u) * 6u) / 127u;

        s->density = (uint8_t)(d > 6u ? 6u : d);
        s->interval = ct_interval_from_cc(c->val[P_PARAM_A]);
        {
            uint32_t m = ((uint32_t)c->val[P_PARAM_B] * 6u) / 128u;

            s->motion = (uint8_t)(m > 5u ? 5u : m);
        }
        s->approach = 0;
        s->center = (uint8_t)(((uint32_t)c->val[P_PARAM_A] * 3u) / 128u);
        if (s->center > 2u)
            s->center = 2;
        s->func = 0;
    }
}

static void triptych_init(triptych_state_t *s)
{
    s->root = 45;
    s->step = 0;
    s->hold_left = 0;
    s->arp_pos = 0;
    s->n_sound = 0;
    s->voicing = 1;
    s->motion = 0;
    s->density = 0;
    s->center = 0;
    s->func = 0;
    s->approach = 0;
    s->interval = 4;
    s->vamp_idx = 0;
    s->tick_acc = 0;
    s->rnd = 0xA5A5A5A5u;
    s->chord_name[0] = 0;
}

static void triptych_set_root(triptych_state_t *s, uint8_t note)
{
    if (note <= 127u)
        s->root = note;
}

static void triptych_fire(triptych_state_t *s, const midi_cfg_t *c, uint8_t chord_root,
                          uint8_t quality, uint8_t strong, uint8_t sheets_slice)
{
    uint8_t notes[4];
    uint16_t feel = (uint16_t)((c->val[P_VEL_MOD] & 31u) * 131u);
    uint8_t vel = ct_vel7(100, feel, strong, ct_roll(s));
    uint32_t n;

    chord_root = ct_tritone_root(chord_root, quality, s->motion, ct_roll(s));
    n = ct_build_chord_led(chord_root, quality, s->voicing, s->sounding, s->n_sound, notes, 4u);
    ct_format_chord(chord_root, quality, s->chord_name, 12u);

    if (n == 0u)
        return;
    if (sheets_slice && s->motion == 2u) {
        uint8_t slot = ct_arp_slot(n, s->arp_pos);

        s->arp_pos++;
        ct_play_notes(s, c, &notes[slot], 1u, vel, 1u);
        return;
    }
    if (s->motion == 2u)
        s->arp_pos = 0;
    ct_play_notes(s, c, notes, n, vel, 0xFu);
}

static uint8_t ct_eff_root(const triptych_state_t *s, const midi_cfg_t *c, uint8_t ro)
{
    int32_t r = (int32_t)s->root + midi_oct_offset(c) * 12 + (int32_t)ro;
    while (r < 24)
        r += 12;
    while (r > 96)
        r -= 12;
    return (uint8_t)r;
}

static void triptych_axis_step(triptych_state_t *s, const midi_cfg_t *c)
{
    uint8_t ro = ct_func_root_offset(s->center, s->func, s->interval);
    uint8_t qual = ct_func_quality(s->func, s->approach);
    uint8_t cr = ct_eff_root(s, c, ro);

    triptych_fire(s, c, cr, qual, (uint8_t)(!s->approach && s->func == 0u), 0);
    s->hold_left = 3u; /* In manual Axis mode (DENSITY=0), pulse calmly every 4 steps */
}

static void triptych_cycle_step(triptych_state_t *s, const midi_cfg_t *c, uint8_t fire_chord)
{
    ct_cycle_step_t cycle[9];
    uint32_t len = ct_build_cycle(s->interval, s->density, cycle, 9u);
    uint8_t w = 1u;

    if (len == 0u)
        return;

    /* In SHEETS mode (motion == 2), arpeggiate on every subdivision even while chord is held */
    if (!fire_chord) {
        if (s->motion == 2u) {
            uint8_t idx = (uint8_t)((s->step ? s->step - 1u : 0u) % len);
            uint8_t cr = ct_eff_root(s, c, cycle[idx].root_offset);

            triptych_fire(s, c, cr, cycle[idx].quality,
                          (uint8_t)(cycle[idx].quality == CT_QUAL_MAJ7), 1);
        }
        return;
    }

    if (s->motion >= 4u) {
        w = ct_vamp_weight(s, s->motion);
        s->vamp_idx++;
        if (ct_vamp_is_rest(w)) {
            ct_silence(s, c);
            s->hold_left = (uint8_t)(ct_vamp_rest_holds(w) - 1u);
            return;
        }
    }

    {
        uint8_t idx = s->step % (uint8_t)len;
        uint8_t cr = ct_eff_root(s, c, cycle[idx].root_offset);
        uint8_t mult = s->motion >= 4u ? (w & 3u) : ct_step_div_mult(s->motion, cycle[idx].quality);
        uint8_t strong = (uint8_t)(cycle[idx].quality == CT_QUAL_MAJ7);

        s->center = cycle[idx].center;
        s->func = cycle[idx].quality == CT_QUAL_MIN7 ? 2u
                : cycle[idx].quality == CT_QUAL_DOM7 ? 1u
                                                     : 0u;
        triptych_fire(s, c, cr, cycle[idx].quality, strong, (uint8_t)(s->motion == 2u));
        s->hold_left = (uint8_t)((mult > 0u ? mult - 1u : 0u) + cycle[idx].hold);
        s->step++;
    }
}

static void triptych_tick(triptych_state_t *s, const midi_cfg_t *c, uint32_t step_ticks)
{
    uint32_t div = midi_div_ticks(c);
    uint8_t prev_c = s->center, prev_f = s->func, prev_a = s->approach, prev_v = s->voicing;

    if (div < 1u)
        div = 1u;
    ct_read_params(s, c);
    if (s->density == 0u &&
        (s->center != prev_c || s->func != prev_f || s->approach != prev_a || s->voicing != prev_v)) {
        triptych_axis_step(s, c);
    }
    if (!step_ticks)
        return;
    s->tick_acc += step_ticks;
    while (s->tick_acc >= div) {
        uint8_t boundary = 1;

        s->tick_acc -= div;
        if (s->hold_left > 0u) {
            s->hold_left--;
            boundary = 0;
        }
        if (s->density == 0u) {
            if (boundary)
                triptych_axis_step(s, c);
        } else
            triptych_cycle_step(s, c, boundary);
    }
}

static uint16_t ct_drive_col(uint8_t drive, uint32_t dim_shift)
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

static void ct_thick_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, uint32_t thick)
{
    cv_line(x0, y0, x1, y1, c);
    if (thick >= 2u)
        cv_line(x0 + 1, y0, x1 + 1, y1, c);
    if (thick >= 3u)
        cv_line(x0, y0 + 1, x1, y1 + 1, c);
}

/* 12-point Circle of Fifths coordinates (center = 42, 44, radius = 34 px):
 * Index k = 0..11 is (pitch_class * 7) % 12 (0=C, 1=G, 2=D, 3=A, 4=E, 5=B, 6=F#, 7=Db, 8=Ab, 9=Eb, 10=Bb, 11=F) */
static const int16_t COF_X[12] = {42, 59, 71, 76, 71, 59, 42, 25, 13, 8, 13, 25};
static const int16_t COF_Y[12] = {10, 15, 27, 44, 61, 73, 78, 73, 61, 44, 27, 15};

static uint32_t ct_pc_to_cof(uint32_t pc)
{
    return ((pc % 12u) * 7u) % 12u;
}

/* Interpolate RGB565 color between two corner RGB triples (w = 0..4, where 0 = c0, 4 = c1) */
static uint16_t ct_blend_rgb(const uint8_t *a, const uint8_t *b, uint32_t w, uint32_t dim_shift)
{
    uint32_t r = ((uint32_t)a[0] * (4u - w) + (uint32_t)b[0] * w) >> (2u + dim_shift);
    uint32_t g = ((uint32_t)a[1] * (4u - w) + (uint32_t)b[1] * w) >> (2u + dim_shift);
    uint32_t bl = ((uint32_t)a[2] * (4u - w) + (uint32_t)b[2] * w) >> (2u + dim_shift);
    return RGB(r, g, bl);
}

static void triptych_draw_triangle(const triptych_state_t *s, const midi_cfg_t *c, int32_t ox, int32_t oy)
{
    /* Corner RGB triples: 0 = Blue, 1 = Green, 2 = Orange */
    static const uint8_t C_RGB[3][3] = {{40, 130, 255}, {40, 230, 110}, {255, 140, 30}};
    uint32_t i;
    uint8_t drive = c->val[P_DRIVE];
    uint8_t timbre = c->val[P_TIMBRE];
    uint8_t decay = c->val[P_DECAY];
    uint8_t feel5 = c->val[P_VEL_MOD] & 31u;
    int32_t det_px = ((int32_t)c->val[P_DETUNE] - 64) / 18;
    uint32_t thick = 1u + ((uint32_t)drive * 2u) / 96u;
    uint16_t c_hi = ct_drive_col(drive, 0u);
    uint32_t prev_v = (s->center + 2u) % 3u;
    uint32_t root_pc = s->root % 12u;
    uint32_t v_cof[3];
    int32_t vx[3], vy[3];

    (void)ox;
    (void)oy;
    /* Compute the 3 Tonal Center positions on the 12-point Circle of Fifths */
    for (i = 0; i < 3u; i++) {
        uint32_t t_pc = (root_pc + ct_center_offset((uint8_t)i, s->interval)) % 12u;
        v_cof[i] = ct_pc_to_cof(t_pc);
        vx[i] = COF_X[v_cof[i]];
        vy[i] = COF_Y[v_cof[i]];
    }

    /* 1. Draw the base 12-point Circle of Fifths ring */
    for (i = 0; i < 12u; i++) {
        uint32_t j = (i + 1u) % 12u;
        uint32_t rel = (i + 12u - v_cof[0]) % 12u;
        uint32_t seg = (rel / 4u) % 3u;
        uint32_t w = rel % 4u;
        uint16_t rcol = ct_blend_rgb(C_RGB[seg], C_RGB[(seg + 1u) % 3u], w, 2u);
        cv_line(COF_X[i], COF_Y[i], COF_X[j], COF_Y[j], rcol);
        cv_pset(COF_X[i], COF_Y[i], rcol);
    }

    /* 2. KNOB 3 (DENSITY): Light up all active V7 (density > i) and ii7 (density >= 4 + i) nodes + bridges on the circle! */
    for (i = 0; i < 3u; i++) {
        uint32_t p_num = (i + 2u) % 3u;
        uint32_t c_idx = v_cof[i];
        uint32_t v_idx = (c_idx + 1u) % 12u;
        uint32_t ii_idx = (c_idx + 2u) % 12u;
        uint16_t col_ii = ct_blend_rgb(C_RGB[p_num], C_RGB[i], 2u, 1u);
        uint16_t col_v = ct_blend_rgb(C_RGB[p_num], C_RGB[i], 3u, 1u);

        if (s->density > i) {
            /* V7 slot for center i is active in the cycle */
            cv_line(COF_X[v_idx], COF_Y[v_idx], COF_X[c_idx], COF_Y[c_idx], col_v);
            cv_rect(COF_X[v_idx] - 2, COF_Y[v_idx] - 2, 5, 5, col_v);
        }
        if (s->density >= (uint8_t)(4u + i)) {
            /* ii7 slot for center i is active in the cycle */
            cv_line(COF_X[ii_idx], COF_Y[ii_idx], COF_X[v_idx], COF_Y[v_idx], col_ii);
            cv_rect(COF_X[ii_idx] - 2, COF_Y[ii_idx] - 2, 5, 5, col_ii);
        }
    }

    /* 3. KNOB 2 (MOTION): Visual signature for each Motion mode */
    if (s->density > 0u) {
        if (s->motion == 3u) {
            /* FREE (Tritone Substitution): draw the 3 tritone diameters across the circle center! */
            for (i = 0; i < 3u; i++) {
                uint32_t v_idx = (v_cof[i] + 1u) % 12u;
                uint32_t tt_idx = (v_idx + 6u) % 12u;
                cv_line(COF_X[v_idx], COF_Y[v_idx], COF_X[tt_idx], COF_Y[tt_idx], C_ACC);
                cv_rect(COF_X[tt_idx] - 2, COF_Y[tt_idx] - 2, 5, 5, C_ACC);
            }
        } else if (s->motion >= 4u) {
            /* VAMP 1 / VAMP 2: draw pulsing syncopation ring around center */
            int32_t vr = (s->motion == 5u) ? 14 : 10;
            cv_rect(42 - vr, 44 - vr, vr * 2 + 1, vr * 2 + 1, C_LINE);
        }
    }

    /* DETUNE stereo twin split triangle */
    if (det_px != 0) {
        cv_line(vx[0] + det_px, vy[0], vx[1] + det_px, vy[1], C_LINE);
        cv_line(vx[1] + det_px, vy[1], vx[2] + det_px, vy[2], C_LINE);
        cv_line(vx[2] + det_px, vy[2], vx[0] + det_px, vy[0], C_LINE);
    }

    /* 4. KNOB 4 (VOICING): Morph the triangle geometry by Voicing (0=Triad, 1=7th, 2=Drop-2, 3=Quartal) */
    for (i = 0; i < 3u; i++) {
        uint32_t j = (i + 1u) % 3u;
        uint16_t ecol = ct_blend_rgb(C_RGB[i], C_RGB[j], 2u, 1u);
        int is_trail = (decay > 36u && ((i == prev_v && j == s->center) || (j == prev_v && i == s->center)));
        ct_thick_line(vx[i], vy[i], vx[j], vy[j], is_trail ? c_hi : ecol, thick);

        if (s->voicing == 1u) {
            /* 7TH: inner mid-point diamond connecting the 3 edges (4-voice close chord) */
            int32_t mx0 = (vx[i] + vx[j]) / 2, my0 = (vy[i] + vy[j]) / 2;
            uint32_t k = (j + 1u) % 3u;
            int32_t mx1 = (vx[j] + vx[k]) / 2, my1 = (vy[j] + vy[k]) / 2;
            cv_line(mx0, my0, mx1, my1, ecol);
        } else if (s->voicing == 2u) {
            /* DROP-2: wide double-spread chord lines pulled slightly toward center */
            int32_t ix0 = (vx[i] * 3 + 42) / 4, iy0 = (vy[i] * 3 + 44) / 4;
            int32_t ix1 = (vx[j] * 3 + 42) / 4, iy1 = (vy[j] * 3 + 44) / 4;
            cv_line(ix0, iy0, ix1, iy1, ecol);
        } else if (s->voicing == 3u) {
            /* QUARTAL: 4th-interval radial struts from center to each vertex + edge midpoint */
            int32_t mx = (vx[i] + vx[j]) / 2, my = (vy[i] + vy[j]) / 2;
            cv_line(42, 44, vx[i], vy[i], C_LINE);
            cv_line(42, 44, mx, my, ecol);
        }
    }

    /* 5. Active ii7 -> V7 -> Imaj7 cadence bridge & compass spoke in blended hue */
    {
        uint32_t c_num = s->center % 3u;
        uint32_t p_num = (c_num + 2u) % 3u;
        uint32_t c_idx = v_cof[c_num];
        uint32_t v_idx = (c_idx + 1u) % 12u;
        uint32_t ii_idx = (c_idx + 2u) % 12u;
        uint32_t prev_idx = v_cof[p_num];
        uint16_t col_ii = ct_blend_rgb(C_RGB[p_num], C_RGB[c_num], 2u, 0u);
        uint16_t col_v = ct_blend_rgb(C_RGB[p_num], C_RGB[c_num], 3u, 0u);
        uint16_t col_i = ct_blend_rgb(C_RGB[c_num], C_RGB[c_num], 0u, 0u);
        uint32_t act_idx = (s->func == 2u) ? ii_idx : ((s->func == 1u || s->approach) ? v_idx : c_idx);
        uint16_t act_col = (s->func == 2u) ? col_ii : ((s->func == 1u || s->approach) ? col_v : col_i);

        if (s->func == 2u) {
            ct_thick_line(COF_X[prev_idx], COF_Y[prev_idx], COF_X[ii_idx], COF_Y[ii_idx], col_ii, thick);
            ct_thick_line(COF_X[ii_idx], COF_Y[ii_idx], COF_X[v_idx], COF_Y[v_idx], col_v, 1u);
            cv_rect(COF_X[ii_idx] - 4, COF_Y[ii_idx] - 4, 9, 9, col_ii);
        } else if (s->func == 1u || s->approach) {
            if (decay > 24u) {
                ct_thick_line(COF_X[ii_idx], COF_Y[ii_idx], COF_X[v_idx], COF_Y[v_idx], col_ii, 1u);
                cv_rect(COF_X[ii_idx] - 3, COF_Y[ii_idx] - 3, 7, 7, col_ii);
            }
            ct_thick_line(COF_X[v_idx], COF_Y[v_idx], COF_X[c_idx], COF_Y[c_idx], col_v, thick);
            cv_rect(COF_X[v_idx] - 4, COF_Y[v_idx] - 4, 9, 9, col_v);
        } else if (decay > 24u && s->density > 0u) {
            ct_thick_line(COF_X[v_idx], COF_Y[v_idx], COF_X[c_idx], COF_Y[c_idx], col_v, 1u);
            cv_rect(COF_X[v_idx] - 2, COF_Y[v_idx] - 2, 5, 5, col_v);
        }

        /* Spoke from center (42, 44) to active chord; center hub size scales with FEEL (feel5) */
        ct_thick_line(42, 44, COF_X[act_idx], COF_Y[act_idx], act_col, thick);
        {
            int32_t hr_hub = 2 + (int32_t)(feel5 >> 3);
            cv_rect(42 - hr_hub, 44 - hr_hub, hr_hub * 2 + 1, hr_hub * 2 + 1, act_col);
        }

        /* SHEETS motion (s->motion == 2): draw 4 arpeggio step dots along the spoke */
        if (s->density > 0u && s->motion == 2u) {
            uint32_t k;
            for (k = 1u; k <= 4u; k++) {
                int32_t ax = 42 + ((COF_X[act_idx] - 42) * (int32_t)k) / 4;
                int32_t ay = 44 + ((COF_Y[act_idx] - 44) * (int32_t)k) / 4;
                uint8_t is_arp = ((s->arp_pos & 3u) + 1u) == k;
                cv_rect(ax - (is_arp ? 2 : 1), ay - (is_arp ? 2 : 1), is_arp ? 5 : 3, is_arp ? 5 : 3,
                        is_arp ? C_WHITE : act_col);
            }
        }

        if (timbre > 20u) {
            int32_t hr = 5 + ((int32_t)timbre * 5) / 127;
            cv_line(COF_X[act_idx] - hr, COF_Y[act_idx], COF_X[act_idx] + hr, COF_Y[act_idx], act_col);
            cv_line(COF_X[act_idx], COF_Y[act_idx] - hr, COF_X[act_idx], COF_Y[act_idx] + hr, act_col);
        }
    }

    /* 6. Draw the 3 Tonal Center corner nodes (RUBATO motion == 1 draws double outer rings!) */
    for (i = 0; i < 3u; i++) {
        int32_t cx = vx[i];
        int32_t cy = vy[i];
        uint16_t ccol = ct_blend_rgb(C_RGB[i], C_RGB[i], 0u, 0u);

        if (s->density > 0u && s->motion == 1u) {
            /* RUBATO: outer hold-frame around each Imaj7 corner */
            cv_line(cx - 6, cy - 6, cx + 6, cy - 6, ccol);
            cv_line(cx - 6, cy + 6, cx + 6, cy + 6, ccol);
        }
        if (i == s->center && s->func == 0u && !s->approach) {
            cv_rect(cx - 5, cy - 5, 11, 11, ccol);
            cv_rect(cx - 2, cy - 2, 5, 5, c_hi);
        } else {
            cv_rect(cx - 3, cy - 3, 7, 7, ccol);
        }
    }
}

static const char *ct_motion_label(uint8_t motion)
{
    static const char *const L[6] = {"Straight", "Rubato", "Sheets", "Free", "Vamp 1", "Vamp 2"};

    if (motion > 5u)
        motion = 5u;
    return L[motion];
}

static const char *ct_func_badge(const triptych_state_t *s)
{
    if (s->density == 0u && s->approach)
        return ">V7";
    if (s->func == 1u)
        return "V7";
    if (s->func == 2u)
        return "ii7";
    return "Imaj7";
}

static void triptych_draw_stage(const triptych_state_t *s, const midi_cfg_t *c)
{
    const char *badge = ct_func_badge(s);
    int32_t nx;
    uint8_t echo_mix = c->val[P_ECHO_MIX];
    uint8_t echo_t = c->val[P_ECHO_TIME];
    int32_t echo_d = 2 + ((int32_t)echo_t * 7) / 127;
    uint16_t ecol = RGB((36u * echo_mix) / 127u, (148u * echo_mix) / 127u, (210u * echo_mix) / 127u);
    const char *chord = s->chord_name[0] ? s->chord_name : "---";

    if (cv_home_view) {
        int32_t tw;

        /* Poster: the circle, round, then the chord. No cadence row, no step meter. */
        cv_begin(240, 216, C_BLACK);
        cv_origin(78, 36);
        if (echo_mix > 2u) {
            uint32_t root_pc = s->root % 12u;
            uint32_t i0 = ct_pc_to_cof((root_pc + ct_center_offset(0, s->interval)) % 12u);
            uint32_t i1 = ct_pc_to_cof((root_pc + ct_center_offset(1, s->interval)) % 12u);
            uint32_t i2 = ct_pc_to_cof((root_pc + ct_center_offset(2, s->interval)) % 12u);
            cv_line(COF_X[i0] + echo_d, COF_Y[i0] + echo_d / 2, COF_X[i1] + echo_d, COF_Y[i1] + echo_d / 2, ecol);
            cv_line(COF_X[i1] + echo_d, COF_Y[i1] + echo_d / 2, COF_X[i2] + echo_d, COF_Y[i2] + echo_d / 2, ecol);
            cv_line(COF_X[i2] + echo_d, COF_Y[i2] + echo_d / 2, COF_X[i0] + echo_d, COF_Y[i0] + echo_d / 2, ecol);
        }
        triptych_draw_triangle(s, c, 0, 0);
        cv_origin(0, 0);
        tw = text_w(&FONT_L, chord);
        nx = (240 - tw) / 2;
        if (nx < 4)
            nx = 4;
        cv_text(nx, 140, &FONT_L, chord, c->val[P_DRIVE] > 32u ? ct_drive_col(c->val[P_DRIVE], 0u) : C_WHITE);
        if (s->density > 0u) {
            const char *motion = ct_motion_label(s->motion);
            int32_t mw = text_w(&FONT_S, motion);
            cv_text((240 - mw) / 2, 180, &FONT_S, motion, C_DIM);
        }
        cv_blit(0, 24);
        return;
    }

    cv_begin(240, 88, C_BLACK);
    /* 45-degree echo ghost triangle: distance = E.TIME, opacity/brightness = E.MIX */
    if (echo_mix > 2u) {
        uint32_t root_pc = s->root % 12u;
        uint32_t i0 = ct_pc_to_cof((root_pc + ct_center_offset(0, s->interval)) % 12u);
        uint32_t i1 = ct_pc_to_cof((root_pc + ct_center_offset(1, s->interval)) % 12u);
        uint32_t i2 = ct_pc_to_cof((root_pc + ct_center_offset(2, s->interval)) % 12u);
        cv_line(COF_X[i0] + echo_d, COF_Y[i0] + echo_d / 2, COF_X[i1] + echo_d, COF_Y[i1] + echo_d / 2, ecol);
        cv_line(COF_X[i1] + echo_d, COF_Y[i1] + echo_d / 2, COF_X[i2] + echo_d, COF_Y[i2] + echo_d / 2, ecol);
        cv_line(COF_X[i2] + echo_d, COF_Y[i2] + echo_d / 2, COF_X[i0] + echo_d, COF_Y[i0] + echo_d / 2, ecol);
    }
    triptych_draw_triangle(s, c, 0, 0);
    nx = 92;
    if (s->chord_name[0]) {
        int32_t det_px = ((int32_t)c->val[P_DETUNE] - 64) / 22;
        uint16_t c_drv = ct_drive_col(c->val[P_DRIVE], 0u);
        if (echo_mix > 2u)
            cv_text(nx + echo_d, 6 + echo_d / 2, &FONT_L, s->chord_name, ecol);
        if (det_px != 0)
            cv_text(nx + det_px, 6, &FONT_L, s->chord_name, ct_drive_col(c->val[P_DRIVE], 1u));
        cv_text(nx, 6, &FONT_L, s->chord_name, c->val[P_DRIVE] > 32u ? c_drv : C_WHITE);
    } else {
        cv_text(nx, 6, &FONT_L, "---", C_DIM);
    }
    /* Show active cadence path ii7 -> V7 -> Imaj7 with current function highlighted */
    cv_text(nx, 44, &FONT_S, "ii7", s->func == 2u ? C_WHITE : C_DIM);
    cv_text(nx + 28, 44, &FONT_S, ">", C_LINE);
    cv_text(nx + 40, 44, &FONT_S, "V7", (s->func == 1u || s->approach) ? C_WHITE : C_DIM);
    cv_text(nx + 62, 44, &FONT_S, ">", C_LINE);
    cv_text(nx + 74, 44, &FONT_S, badge, (s->func == 0u && !s->approach) ? C_ACC : C_DIM);

    if (s->density == 0u)
        cv_text(nx, 66, &FONT_S, "Axis Manual", C_AMB);
    else
        cv_text(nx, 66, &FONT_S, ct_motion_label(s->motion), C_AMB);
    {
        /* Show cycle step indicator on right at y = 72 */
        uint32_t k, cyc_n = s->density == 0u ? 3u : (3u + s->density);
        for (k = 0; k < cyc_n && k < 9u; k++) {
            int32_t sx = 172 + (int32_t)k * 7;
            uint8_t active = (s->step % cyc_n) == k;
            cv_rect(sx, 71, 5, active ? 7 : 4, active ? C_WHITE : C_LINE);
        }
    }
    cv_blit_engine_diagram();
}

#endif /* ENGINE_TRIPTYCH_H */
