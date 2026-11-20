/* SPDX-License-Identifier: GPL-3.0-only */
/* DIT DAH — ITU Morse phrase player (Mode 2). Include after platform.c / gfx. */
#ifndef ENGINE_DITDAH_H
#define ENGINE_DITDAH_H

#include "midi_ctrl.h"
#include "synth_fm2.h"

enum { DITDAH_MAX_EV = 256 };

typedef struct {
    uint8_t phrase_idx; /* 0..22 (displayed as 01..23) */
    uint8_t edit_active; /* 1 when EDIT cursor mode is active in DIT DAH */
    uint8_t cursor_pos;  /* 0..26 character cursor position in active phrase */
    uint8_t ev_dah[256];   /* 0=dit 1=dah 2=silence; packed length in ev_len */
    uint8_t ev_units[256]; /* duration in dit units */
    uint16_t ev_len;
    uint16_t ev_pos;
    uint8_t unit_left; /* remaining dit-units in current event */
    uint8_t base_note;
    uint8_t playing;
    uint8_t cur_char;
    uint32_t tick_acc;
    uint32_t rnd;
    char ticker[32];
} ditdah_state_t;

static uint8_t ditdah_ev_char[DITDAH_MAX_EV];
static uint8_t ditdah_ev_idx[DITDAH_MAX_EV];
static uint16_t ditdah_ticker_ev[32];
static uint8_t ditdah_midi_last[2]; /* last MIDI note sent per dit/dah kind */
static uint32_t ditdah_midi_until_ms;

static const char *ditdah_morse_pattern(uint8_t ch)
{
    switch (ch) {
    case 'A':
        return ".-";
    case 'B':
        return "-...";
    case 'C':
        return "-.-.";
    case 'D':
        return "-..";
    case 'E':
        return ".";
    case 'F':
        return "..-.";
    case 'G':
        return "--.";
    case 'H':
        return "....";
    case 'I':
        return "..";
    case 'J':
        return ".---";
    case 'K':
        return "-.-";
    case 'L':
        return ".-..";
    case 'M':
        return "--";
    case 'N':
        return "-.";
    case 'O':
        return "---";
    case 'P':
        return ".--.";
    case 'Q':
        return "--.-";
    case 'R':
        return ".-.";
    case 'S':
        return "...";
    case 'T':
        return "-";
    case 'U':
        return "..-";
    case 'V':
        return "...-";
    case 'W':
        return ".--";
    case 'X':
        return "-..-";
    case 'Y':
        return "-.--";
    case 'Z':
        return "--..";
    case '0':
        return "-----";
    case '1':
        return ".----";
    case '2':
        return "..---";
    case '3':
        return "...--";
    case '4':
        return "....-";
    case '5':
        return ".....";
    case '6':
        return "-....";
    case '7':
        return "--...";
    case '8':
        return "---..";
    case '9':
        return "----.";
    default:
        return 0;
    }
}

static uint8_t ditdah_up(uint8_t c)
{
    if (c >= 'a' && c <= 'z')
        return (uint8_t)(c - 32u);
    return c;
}

static void ditdah_push_ev(ditdah_state_t *s, uint8_t kind, uint8_t units, uint8_t ch_ix, uint8_t str_idx)
{
    if (s->ev_len >= DITDAH_MAX_EV)
        return;
    s->ev_dah[s->ev_len] = kind;
    s->ev_units[s->ev_len] = units;
    ditdah_ev_char[s->ev_len] = ch_ix;
    ditdah_ev_idx[s->ev_len] = str_idx;
    s->ev_len++;
}

static void ditdah_compile_chars(ditdah_state_t *s, const uint8_t *chars, uint8_t n)
{
    uint32_t idx;
    uint16_t prev_pos = s->ev_pos;
    uint8_t prev_left = s->unit_left;
    uint8_t prev_play = s->playing;

    s->ev_len = 0;

    for (idx = 0; idx < n; idx++) {
        uint8_t ch = ditdah_up(chars[idx]);
        const char *pat;
        uint32_t pi, plen;

        if (ch == ' ') {
            ditdah_push_ev(s, 2u, 7u, ' ', (uint8_t)idx);
            continue;
        }
        pat = ditdah_morse_pattern(ch);
        if (!pat)
            continue;
        plen = 0;
        while (pat[plen])
            plen++;
        for (pi = 0; pi < plen; pi++) {
            uint8_t dah = pat[pi] == '-' ? 1u : 0u;
            ditdah_push_ev(s, dah, dah ? 3u : 1u, ch, (uint8_t)idx);
            if (pi + 1u < plen)
                ditdah_push_ev(s, 2u, 1u, ch, (uint8_t)idx);
        }
        if (idx + 1u < n && chars[idx + 1u] != ' ')
            ditdah_push_ev(s, 2u, 3u, ch, (uint8_t)idx);
    }
    if (s->ev_len)
        ditdah_push_ev(s, 2u, 7u, ' ', n > 0u ? (uint8_t)(n - 1u) : 0u);

    if (!s->ev_len) {
        s->ev_pos = 0;
        s->unit_left = 0;
        s->playing = 0;
        s->cur_char = '_';
        synth_note_off_voice(0);
        synth_note_off_voice(1);
    } else if (prev_pos < s->ev_len) {
        s->ev_pos = prev_pos;
        s->unit_left = prev_left;
        s->playing = prev_play;
    } else {
        s->ev_pos = 0;
        s->unit_left = 0;
        s->playing = 0;
    }
}

static uint32_t ditdah_phrase_len(const midi_cfg_t *c, uint32_t idx)
{
    uint32_t n = 0;
    const char *p = c->phrases[idx < DITDAH_PHRASES ? idx : 0u];
    while (n + 1u < DITDAH_MAX_LEN && p[n])
        n++;
    return n;
}

static void ditdah_phrase_text(const ditdah_state_t *s, const midi_cfg_t *c, char *out, uint32_t cap)
{
    const char *src = c->phrases[s->phrase_idx < DITDAH_PHRASES ? s->phrase_idx : 0u];
    uint32_t i = 0;

    while (src[i] && i + 1u < cap && i + 1u < DITDAH_MAX_LEN) {
        out[i] = src[i];
        i++;
    }
    out[i] = 0;
}

static void ditdah_refresh_ticker(ditdah_state_t *s)
{
    uint32_t w = 0;
    uint32_t idx = 0;
    uint32_t page_start = 0;

    /* Find the start of a ~10-element page containing s->ev_pos so playhead moves left-to-right */
    if (s->ev_pos > 10u) {
        page_start = (s->ev_pos / 10u) * 10u;
    }
    idx = page_start;
    while (w + 2u < sizeof s->ticker && idx < s->ev_len) {
        uint8_t k = s->ev_dah[idx];
        uint8_t u = s->ev_units[idx];
        if (k == 2u) {
            /* Only insert a visual space between letters/words (units >= 3), not inside a letter (units == 1) */
            if (u >= 3u && w && s->ticker[w - 1u] != ' ') {
                ditdah_ticker_ev[w] = (uint16_t)idx;
                s->ticker[w++] = ' ';
            }
        } else {
            ditdah_ticker_ev[w] = (uint16_t)idx;
            s->ticker[w++] = (k == 0u) ? '.' : '-';
        }
        idx++;
    }
    s->ticker[w] = 0;
}

static void ditdah_init(ditdah_state_t *s)
{
    s->phrase_idx = 0;
    s->edit_active = 0;
    s->cursor_pos = 0;
    s->ev_len = 0;
    s->ev_pos = 0;
    s->unit_left = 0;
    s->base_note = 57;
    s->playing = 0;
    s->cur_char = 'E';
    s->tick_acc = 0;
    s->rnd = 1u;
    s->ticker[0] = 0;
    ditdah_midi_last[0] = 0;
    ditdah_midi_last[1] = 0;
    ditdah_midi_until_ms = 0;
}

static void ditdah_set_base(ditdah_state_t *s, uint8_t note)
{
    if (note > 127u)
        note = 127u;
    s->base_note = note;
}

/* Move character cursor left/right along active phrase (including 1 slot past end to append) */
static void ditdah_cursor_move(ditdah_state_t *s, const midi_cfg_t *c, int32_t delta)
{
    uint32_t idx = s->phrase_idx < DITDAH_PHRASES ? s->phrase_idx : 0u;
    int32_t len = (int32_t)ditdah_phrase_len(c, idx);
    int32_t max_pos = len < (int32_t)(DITDAH_MAX_LEN - 2u) ? len : (int32_t)(DITDAH_MAX_LEN - 2u);
    int32_t pos = (int32_t)s->cursor_pos + (delta > 0 ? 1 : -1);

    if (pos < 0)
        pos = 0;
    if (pos > max_pos)
        pos = max_pos;
    s->cursor_pos = (uint8_t)pos;
    {
        char ch = c->phrases[idx][s->cursor_pos];
        s->cur_char = ch ? (uint8_t)ch : (uint8_t)'_';
    }
}

/* Replace character at s->cursor_pos in place; cursor stays on this letter until moved by user! */
static void ditdah_key(ditdah_state_t *s, midi_cfg_t *c, uint32_t key_index)
{
    uint32_t idx = s->phrase_idx < DITDAH_PHRASES ? s->phrase_idx : 0u;
    uint32_t len = ditdah_phrase_len(c, idx);
    char ch;

    if (key_index <= 25u)
        ch = (char)('A' + key_index);
    else if (key_index == 26u)
        ch = ' ';
    else
        return;

    /* Automatically enter edit mode if user types on keys in DIT DAH */
    s->edit_active = 1u;
    if (s->cursor_pos > len)
        s->cursor_pos = (uint8_t)len;
    if (s->cursor_pos >= DITDAH_MAX_LEN - 1u)
        s->cursor_pos = DITDAH_MAX_LEN - 2u;

    c->phrases[idx][s->cursor_pos] = ch;
    if (s->cursor_pos >= len) {
        c->phrases[idx][s->cursor_pos + 1u] = 0;
    }
    s->cur_char = ch == ' ' ? '_' : (uint8_t)ch;
}

/* Delete character at cursor_pos (shifting tail left) */
static void ditdah_backspace(ditdah_state_t *s, midi_cfg_t *c)
{
    uint32_t idx = s->phrase_idx < DITDAH_PHRASES ? s->phrase_idx : 0u;
    uint32_t len = ditdah_phrase_len(c, idx);
    uint32_t p = s->cursor_pos;

    s->edit_active = 1u;
    if (len == 0u)
        return;
    if (p >= len)
        p = len - 1u;
    for (; p < len; p++)
        c->phrases[idx][p] = c->phrases[idx][p + 1u];
    c->phrases[idx][len - 1u] = 0;
    if (s->cursor_pos > 0u && s->cursor_pos >= len - 1u)
        s->cursor_pos = (uint8_t)(len > 1u ? len - 2u : 0u);
    {
        char ch = c->phrases[idx][s->cursor_pos];
        s->cur_char = ch ? (uint8_t)ch : (uint8_t)'_';
    }
}

/* Clear phrase (or truncate from cursor_pos) */
static void ditdah_clear(ditdah_state_t *s, midi_cfg_t *c)
{
    uint32_t idx = s->phrase_idx < DITDAH_PHRASES ? s->phrase_idx : 0u;
    uint32_t k;

    s->edit_active = 1u;
    for (k = 0; k < DITDAH_MAX_LEN; k++)
        c->phrases[idx][k] = 0;
    s->cursor_pos = 0;
    s->ev_len = 0;
    s->ev_pos = 0;
    s->unit_left = 0;
    s->playing = 0;
    s->cur_char = '_';
    s->ticker[0] = 0;
    synth_note_off_voice(0);
    synth_note_off_voice(1);
}

static void ditdah_select_phrase(ditdah_state_t *s, const midi_cfg_t *c)
{
    uint32_t ph = midi_ditdah_phrase_idx(c->base_val[P_PARAM_B]);

    s->phrase_idx = (uint8_t)ph;
    s->cursor_pos = 0;
    s->ev_pos = 0;
    s->unit_left = 0;
    s->tick_acc = 96u;
    s->playing = 0;
    synth_note_off_voice(0);
    synth_note_off_voice(1);
}

static void ditdah_recompile(ditdah_state_t *s, const midi_cfg_t *c)
{
    uint32_t ph = midi_ditdah_phrase_idx(c->base_val[P_PARAM_B]);
    const char *p;
    uint8_t buf[DITDAH_MAX_LEN];
    uint32_t i = 0;

    if (ph >= DITDAH_PHRASES)
        ph = DITDAH_PHRASES - 1u;
    s->phrase_idx = (uint8_t)ph;
    p = c->phrases[s->phrase_idx];
    while (p[i] && i + 1u < DITDAH_MAX_LEN) {
        buf[i] = (uint8_t)p[i];
        i++;
    }
    if (s->cursor_pos > i)
        s->cursor_pos = (uint8_t)i;
    ditdah_compile_chars(s, buf, (uint8_t)i);
    ditdah_refresh_ticker(s);
}

static void ditdah_midi_ch(const midi_cfg_t *c, uint32_t *on, uint32_t *off)
{
    uint32_t ch = c->midi_ch ? ((uint32_t)c->midi_ch - 1u) : 0u;

    *on = 0x90u | ch;
    *off = 0x80u | ch;
}

static void ditdah_voice_for_kind(uint8_t kind, uint32_t invert, uint32_t *voice_out)
{
    uint32_t is_dah = kind == 1u;

    if (invert)
        is_dah = is_dah ? 0u : 1u;
    *voice_out = is_dah ? 1u : 0u;
}

static void ditdah_midi_release(const midi_cfg_t *c)
{
    uint32_t on_st, off_st;
    uint32_t k;

    ditdah_midi_ch(c, &on_st, &off_st);
    (void)on_st;
    for (k = 0; k < 2u; k++) {
        if (ditdah_midi_last[k]) {
            trm_off(off_st, (uint32_t)ditdah_midi_last[k]);
            ditdah_midi_last[k] = 0;
        }
    }
    ditdah_midi_until_ms = 0;
}

static void ditdah_tone_off(const midi_cfg_t *c, uint8_t kind, uint32_t invert)
{
    (void)invert;
    if (kind <= 1u && ditdah_midi_last[kind]) {
        uint32_t on_st, off_st;

        ditdah_midi_ch(c, &on_st, &off_st);
        (void)on_st;
        trm_off(off_st, (uint32_t)ditdah_midi_last[kind]);
        ditdah_midi_last[kind] = 0;
    }
    synth_note_off_voice(0);
    synth_note_off_voice(1);
}

static void ditdah_begin_event(ditdah_state_t *s, const midi_cfg_t *c)
{
    int32_t acc_bip = (int32_t)c->val[P_VEL_MOD] - 64;
    uint32_t invert = acc_bip < 0 ? 1u : 0u;
    uint32_t texture = (uint32_t)(acc_bip < 0 ? -acc_bip : acc_bip);
    uint32_t interval = ((uint32_t)c->val[P_PARAM_A] * 24u) / 127u;
    uint32_t on_st, off_st;
    uint8_t kind;
    uint8_t units;
    uint32_t dit_note;
    uint32_t dah_note;
    uint32_t note;
    uint32_t vel;
    uint32_t voice;
    int32_t timbre;

    if (!s->ev_len)
        return;

    kind = s->ev_dah[s->ev_pos];
    units = s->ev_units[s->ev_pos];
    s->cur_char = ditdah_ev_char[s->ev_pos];
    if (!s->cur_char)
        s->cur_char = ' ';

    if (kind == 2u) {
        /* Scale inter-letter / word silence by DENSITY (low DENSITY = wide gaps, high DENSITY = tight 1-unit techno lock) */
        if (units >= 3u) {
            uint32_t dens = c->val[P_DENSITY];
            if (dens >= 108u)
                units = 1u;
            else if (dens >= 80u)
                units = (uint8_t)(units > 2u ? 2u : 1u);
            else if (dens < 40u)
                units = (uint8_t)(units + 3u);
        }
        s->unit_left = units;
        ditdah_midi_release(c);
        synth_note_off_voice(0);
        synth_note_off_voice(1);
        s->playing = 0;
        ditdah_refresh_ticker(s);
        return;
    }
    s->unit_left = units;

    {
        int32_t bn = (int32_t)s->base_note + midi_oct_offset(c) * 12;
        while (bn < 24)
            bn += 12;
        while (bn > 103)
            bn -= 12;
        dit_note = (uint32_t)bn;
    }
    dah_note = dit_note + interval;
    if (dah_note > 127u)
        dah_note = 127u;

    {
        uint32_t is_dah = kind == 1u;

        if (invert)
            is_dah = is_dah ? 0u : 1u;
        note = is_dah ? dah_note : dit_note;
        voice = is_dah ? 1u : 0u;
        timbre = is_dah ? (texture ? 48 : 24) : (texture ? 8 : 0);
    }

    s->rnd = s->rnd * 1103515245u + 12345u;
    vel = texture ? (40u + (s->rnd >> 16) % 88u) : 100u;
    if (vel > 127u)
        vel = 127u;
    if (vel < 1u)
        vel = 1u;

    ditdah_midi_ch(c, &on_st, &off_st);
    if (kind <= 1u)
        ditdah_midi_last[kind] = (uint8_t)note;
    synth_note_on_ex(voice, note, vel, timbre, 256, c, 0);
    trm_on(on_st, note, vel);
    {
        uint32_t unit_ppqn = midi_div_ticks(c);
        uint32_t bpm24 = 60u + (uint32_t)c->val[P_TEMPO];
        uint32_t ms = bpm24 ? 60000u / (bpm24 * 24u) : 25u;
        uint32_t span;

        if (unit_ppqn < 1u)
            unit_ppqn = 1u;
        if (ms < 1u)
            ms = 1u;
        span = (uint32_t)units * unit_ppqn * ms;
#if defined(DINGHY_HOST)
        if (span > (uint32_t)units * 40u)
            span = (uint32_t)units * 40u;
#endif
        ditdah_midi_until_ms = fm1_ms + span;
    }
    s->playing = 1;
    ditdah_refresh_ticker(s);
}

static void ditdah_midi_poll(const midi_cfg_t *c)
{
    if (ditdah_midi_until_ms && fm1_ms >= ditdah_midi_until_ms)
        ditdah_midi_release(c);
}

static void ditdah_tick(ditdah_state_t *s, const midi_cfg_t *c, uint32_t step_ticks)
{
    uint32_t unit_ppqn;
    uint32_t invert;

    if (!step_ticks || !s->ev_len)
        return;

    ditdah_midi_poll(c);

    invert = c->val[P_VEL_MOD] < 64u ? 1u : 0u;

    /* RATE (P_GATE / CC 22 in TIME bank) sets the Morse dit clock division, identical to TRIAXIS & CONTURA */
    unit_ppqn = midi_div_ticks(c);
    if (unit_ppqn < 1u)
        unit_ppqn = 1u;

    s->tick_acc += step_ticks;
    while (s->tick_acc >= unit_ppqn) {
        s->tick_acc -= unit_ppqn;

        if (s->unit_left == 0) {
            ditdah_begin_event(s, c);
            continue;
        }
        s->unit_left--;
        if (s->unit_left == 0) {
            uint8_t kind = s->ev_dah[s->ev_pos];

            ditdah_tone_off(c, kind, invert);
            s->playing = 0;
            s->ev_pos++;
            if (s->ev_pos >= s->ev_len)
                s->ev_pos = 0;
        }
    }
}

static uint16_t ditdah_drive_col(uint8_t drive, uint32_t dim_shift)
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

/* Main stage canvas is 116px tall (y=24..139), right above the on-screen keyboard at y=140. */
enum {
    DITDAH_CV_H = 116,
    DITDAH_PHRASE_Y = 82,
    DITDAH_PHRASE_BOX_Y = 76,
    DITDAH_PHRASE_UL_Y = 100
};

static void ditdah_draw_stage(const ditdah_state_t *s, const midi_cfg_t *c)
{
    char letter[2];
    char phrase[44];
    int32_t x;
    uint32_t i;
    int32_t tx = 72;
    int32_t ty;
    int32_t hero_y;
    int32_t rule_y;
    int32_t phrase_y;
    int32_t phrase_box_y;
    int32_t phrase_ul_y;
    uint32_t cv_h = cv_home_view ? 216u : DITDAH_CV_H;
    uint8_t echo_mix = c->val[P_ECHO_MIX];
    uint8_t echo_t = c->val[P_ECHO_TIME];
    uint8_t drive = c->val[P_DRIVE];
    uint8_t timbre = c->val[P_TIMBRE];
    uint8_t decay = c->val[P_DECAY];
    int32_t det_px = ((int32_t)c->val[P_DETUNE] - 64) / 18;
    int32_t drv_add = ((int32_t)drive * 3) / 127; /* +0..3 px stroke thickness */
    uint32_t tail_ev = 1u + ((uint32_t)decay * 5u) / 127u;
    uint16_t c_drv_hi = ditdah_drive_col(drive, 0u);
    uint16_t c_drv_dim = ditdah_drive_col(drive, 1u);
    int32_t echo_d = 2 + ((int32_t)echo_t * 6) / 127;
    uint16_t ecol = RGB((36u * echo_mix) / 127u, (148u * echo_mix) / 127u, (210u * echo_mix) / 127u);

    if (cv_home_view) {
        hero_y = 48;
        ty = 108;
        rule_y = 156;
        phrase_box_y = 168;
        phrase_y = 174;
        phrase_ul_y = 196;
    } else {
        hero_y = 6;
        ty = 40;
        rule_y = 68;
        phrase_box_y = DITDAH_PHRASE_BOX_Y;
        phrase_y = DITDAH_PHRASE_Y;
        phrase_ul_y = DITDAH_PHRASE_UL_Y;
    }

    cv_begin(240, cv_h, C_BLACK);

    ditdah_phrase_text(s, c, phrase, sizeof phrase);
    {
        char hero = s->edit_active
            ? (phrase[s->cursor_pos] ? phrase[s->cursor_pos] : '_')
            : (char)s->cur_char;
        int32_t hx;

        letter[0] = hero ? hero : ' ';
        letter[1] = 0;
        hx = cv_home_view ? (240 - text_w(&FONT_L, letter)) / 2 : 8;
        if (hx < 4)
            hx = 4;
        if (echo_mix > 2u && !cv_home_view)
            cv_text(hx + echo_d, hero_y + echo_d, &FONT_L, letter, ecol);
        if (det_px != 0 && !cv_home_view)
            cv_text(hx + det_px, hero_y, &FONT_L, letter, c_drv_dim);
        cv_text(hx, hero_y, &FONT_L, letter, drive > 32u ? c_drv_hi : C_WHITE);
        if (s->edit_active) {
            int32_t ex = cv_home_view ? (240 - text_w(&FONT_S, "[EDIT]")) / 2 : 184;
            cv_text(ex, cv_home_view ? hero_y - 18 : hero_y + 4, &FONT_S, "[EDIT]", C_ACC);
        }
    }
    if (cv_home_view) {
        int32_t mw = 0;
        for (i = 0; s->ticker[i]; i++)
            mw += s->ticker[i] == '.' ? 16 : (s->ticker[i] == '-' ? 32 : 12);
        tx = (240 - mw) / 2;
        if (tx < 4)
            tx = 4;
    }
    for (i = 0; s->ticker[i]; i++) {
        char sym = s->ticker[i];
        uint16_t ev_i = ditdah_ticker_ev[i];
        uint32_t age = (s->ev_pos >= ev_i) ? (uint32_t)(s->ev_pos - ev_i) : 999u;
        int in_tail = (age > 0u && age <= tail_ev);
        uint16_t col = (ev_i == s->ev_pos) ? C_WHITE : (in_tail ? c_drv_hi : (ev_i < s->ev_pos ? C_DIM : c_drv_dim));
        int32_t hr = ((int32_t)timbre * 10) / 127;

        if (sym == '.') {
            if (echo_mix > 2u)
                cv_rect(tx + echo_d, ty + 8 + echo_d, 10, 12 + drv_add, ecol);
            if (det_px != 0)
                cv_rect(tx + det_px, ty + 8, 10, 12, C_LINE);
            cv_rect(tx, ty + 6 - drv_add / 2, 10, 16 + drv_add, col);
            if (ev_i == s->ev_pos) {
                if (hr > 0)
                    cv_line(tx + 5, ty + 4 - hr, tx + 5, ty + 24 + hr, c_drv_hi);
                cv_rect(tx, ty + 26, 10, 4, C_ACC);
            }
            tx += 16;
        } else if (sym == '-') {
            if (echo_mix > 2u)
                cv_rect(tx + echo_d, ty + 10 + echo_d, 26, 8 + drv_add, ecol);
            if (det_px != 0)
                cv_rect(tx + det_px, ty + 10, 26, 8, C_LINE);
            cv_rect(tx, ty + 8 - drv_add / 2, 26, 12 + drv_add, col);
            if (ev_i == s->ev_pos) {
                if (hr > 0)
                    cv_line(tx + 13, ty + 6 - hr, tx + 13, ty + 22 + hr, c_drv_hi);
                cv_rect(tx, ty + 26, 26, 4, C_ACC);
            }
            tx += 32;
        } else {
            tx += 12;
        }
        if (tx > 228)
            break;
    }

    cv_line(cv_home_view ? 48 : 8, rule_y, cv_home_view ? 192 : 232, rule_y, C_LINE);
    x = 8;
    {
        uint32_t play_k = s->ev_len ? ditdah_ev_idx[s->ev_pos % s->ev_len] : 0u;
        uint32_t focus_k = s->edit_active ? (uint32_t)s->cursor_pos : play_k;
        uint32_t k = focus_k > 12u ? (focus_k - 12u) : 0u;
        uint32_t k0 = k;
        char ch_s[2] = {0, 0};

        if (cv_home_view) {
            int32_t total = 0;
            uint32_t kk = k0;
            while (phrase[kk] || (s->edit_active && kk == s->cursor_pos)) {
                int32_t cw;
                ch_s[0] = phrase[kk] ? phrase[kk] : '_';
                cw = text_w(&FONT_S, ch_s);
                if (cw < 8)
                    cw = 8;
                total += cw + (s->edit_active ? 2 : 0);
                if (!phrase[kk])
                    break;
                kk++;
            }
            x = (240 - total) / 2;
            if (x < 4)
                x = 4;
        }

        while (phrase[k] || (s->edit_active && k == s->cursor_pos)) {
            int32_t cw;
            ch_s[0] = phrase[k] ? phrase[k] : '_';
            cw = text_w(&FONT_S, ch_s);
            if (cw < 8)
                cw = 8;

            if (s->edit_active && k == s->cursor_pos) {
                cv_rect(x - 2, phrase_box_y, cw + 4, 30, C_ACC);
                cv_text(x, phrase_y, &FONT_S, ch_s, C_WHITE);
            } else if (k == play_k && s->ev_len) {
                cv_rect(x - 1, phrase_box_y + 2, cw + 2, 26, C_LINE);
                cv_text(x, phrase_y, &FONT_S, ch_s, C_WHITE);
                cv_rect(x, phrase_ul_y, cw, 3, C_ACC);
            } else {
                cv_text(x, phrase_y, &FONT_S, ch_s, k < play_k ? c_drv_dim : c_drv_hi);
            }
            x += cw + (s->edit_active ? 2 : 0);
            if (!phrase[k])
                break;
            k++;
            if (x > 228)
                break;
        }
    }
    cv_blit(0, 24);
}

#endif /* ENGINE_DITDAH_H */
