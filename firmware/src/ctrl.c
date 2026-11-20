/* SPDX-License-Identifier: GPL-3.0-only */
#define APP_NAME "KO CTRL"
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_56422"
#endif
#include "platform.c"
#include <stddef.h>
#define C_ACC RGB(210, 140, 255)
#include "ctrl_core.h"

uint8_t g_mod_hold_slot = 255u;
uint32_t g_mod_hold_until = 0u;

static const uint8_t KEY_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t KEY_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

static const char *const NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

static const uint16_t BANK_COL[CTRL_BANK_COUNT] = {
    RGB(120, 200, 255), RGB(100, 180, 255), RGB(90, 220, 200), RGB(140, 160, 255), RGB(200, 140, 255),
    RGB(255, 180, 120), RGB(255, 140, 160), RGB(255, 120, 200), RGB(180, 255, 160)};

static void str_copy(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static uint32_t ctrl_slen(const char *s)
{
    uint32_t n = 0;
    while (s[n])
        n++;
    return n;
}

static void str_cat(char *d, const char *s)
{
    str_copy(d + ctrl_slen(d), s);
}

static void u8_dec(char *s, uint32_t v)
{
    if (v >= 100u) {
        s[0] = (char)('0' + v / 100u);
        s[1] = (char)('0' + (v / 10u) % 10u);
        s[2] = (char)('0' + v % 10u);
        s[3] = 0;
    } else if (v >= 10u) {
        s[0] = (char)('0' + v / 10u);
        s[1] = (char)('0' + v % 10u);
        s[2] = 0;
    } else {
        s[0] = (char)('0' + v);
        s[1] = 0;
    }
}

static void i8_dec(char *s, int32_t v)
{
    if (v < 0) {
        s[0] = '-';
        u8_dec(s + 1, (uint32_t)(-v));
    } else
        u8_dec(s, (uint32_t)v);
}

/* UI readouts: same rounding as fm_app u8_show100_dec / bip50_show. */
static void u8_show100_dec(char *s, uint32_t v)
{
    if (v > 127u)
        v = 127u;
    u8_dec(s, (v * 100u + 63u) / 127u);
}

static void bip50_show(char *s, uint8_t v)
{
    int32_t c = (int32_t)(v > 127u ? 127u : v) - 64;

    if (c < -63)
        c = -63;
    c = c < 0 ? -((-c * 50 + 31) / 63) : (c * 50 + 31) / 63;
    if (c > 0) {
        s[0] = '+';
        u8_dec(s + 1, (uint32_t)c);
    } else
        i8_dec(s, c);
}

static void fmt_hz100(char *s, uint32_t hz100)
{
    uint32_t whole = hz100 / 100u;
    uint32_t frac = (hz100 % 100u) / 10u;

    u8_dec(s, whole);
    str_cat(s, ".");
    s[ctrl_slen(s)] = (char)('0' + frac);
    s[ctrl_slen(s) + 1u] = 0;
    str_cat(s, " Hz");
}

static void fmt_pct(char *s, uint32_t v, uint32_t max)
{
    u8_dec(s, (v * 100u) / max);
    str_cat(s, "%");
}

static void mark_dirty(void)
{
    g_ctrl.set_dirty = 1u;
    g_ctrl.set_t = fm1_ms;
    g_ctrl.ui_dirty = 1u;
}

static int lfo_btn_held(void)
{
    return (int)((fm1_in.buttons >> BTN_LFO) & 1u);
}

static void lfo_depth_wake(uint8_t t)
{
    if (!g_ctrl.cfg.lfo_depth[t])
        g_ctrl.cfg.lfo_depth[t] = 48;
}

static void edit_mark(void)
{
    g_ctrl.edit_used = 1u;
    mark_dirty();
}

static void reset_slot_default(uint32_t slot)
{
    uint8_t p = g_ctrl.cfg.profile % CTRL_NPROFILES;
    uint32_t b = slot >> 3;
    uint32_t col = slot & 7u;

    if (slot >= CTRL_NPARAMS || b > 3u)
        return;
    if (col < 4u) {
        uint32_t p_ix = b * 4u + col;

        g_ctrl.cfg.base_val[slot] = CTRL_PROFILE_INIT[p][p_ix];
        g_ctrl.cfg.cc_map[slot] = CTRL_PROFILE_CC[p][p_ix];
        ctrl_label_set4(g_ctrl.cfg.custom_label[slot], CTRL_PROFILE_LABELS[p][p_ix]);
    } else {
        static const char usr[4][4] = {{'U', 'S', 'R', '1'}, {'U', 'S', 'R', '2'}, {'U', 'S', 'R', '3'},
                                       {'U', 'S', 'R', '4'}};
        uint32_t i;

        g_ctrl.cfg.base_val[slot] = 64u;
        g_ctrl.cfg.cc_map[slot] = (uint8_t)(20u + b * 4u + (col - 4u));
        for (i = 0; i < 4u; i++)
            g_ctrl.cfg.custom_label[slot][i] = usr[col - 4u][i];
    }
    g_ctrl.eff_val[slot] = g_ctrl.cfg.base_val[slot];
}

static char char_cycle_step(char c, int32_t g)
{
    static const char set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ./-+";
    uint32_t i, n = (uint32_t)(sizeof(set) - 1u);

    for (i = 0; i < n; i++) {
        if (set[i] == c)
            break;
    }
    if (i >= n)
        i = 0;
    if (g > 0)
        i = (i + 1u) % n;
    else if (g < 0)
        i = (i + n - 1u) % n;
    return set[i];
}

static void reset_lfo_slot(uint8_t t)
{
    g_ctrl.cfg.lfo_depth[t] = 0;
    g_ctrl.cfg.lfo_rate[t] = FM_LFO_RATE_DEFAULT;
    g_ctrl.cfg.lfo_shape[t] = 0;
    g_ctrl.cfg.lfo_phase[t] = 0;
    g_ctrl.cfg.lfo_smooth[t] = 0;
    g_ctrl.cfg.lfo_skew[t] = FM_LFO_SKEW_DEFAULT;
    g_ctrl.cfg.lfo_algo[t] = FM_LFO_ALG_CLASSIC;
    g_ctrl.cfg.lfo_ratio[t] = 52;
    g_ctrl.cfg.lfo_index[t] = 64;
    g_ctrl.cfg.lfo_fb[t] = 0;
    g_ctrl.cfg.lfo_fold[t] = 0;
}

static void reset_section_lfo(void)
{
    uint32_t i;

    for (i = 0; i < CTRL_NPARAMS; i++)
        reset_lfo_slot((uint8_t)i);
    g_ctrl.cfg.lfo_target = (uint8_t)ctrl_active_nth(&g_ctrl.cfg, 0u);
    g_ctrl.cfg.bank_page[CTRL_BANK_LFO] = 0;
    g_ctrl.lfo_key_page = 0u;
}

static void ctrl_lfo_snap_target(void)
{
    if (!ctrl_slot_active(&g_ctrl.cfg, g_ctrl.cfg.lfo_target))
        g_ctrl.cfg.lfo_target = (uint8_t)ctrl_active_nth(&g_ctrl.cfg, 0u);
}

static uint32_t ctrl_lfo_target_active_ix(void)
{
    uint32_t b, col, ix = 0;
    uint8_t t = g_ctrl.cfg.lfo_target;

    for (b = 0; b < 4u; b++) {
        uint32_t cnt = g_ctrl.cfg.bank_count[b];

        if (cnt < 1u)
            cnt = 1u;
        if (cnt > 8u)
            cnt = 8u;
        for (col = 0; col < cnt; col++) {
            if (b * 8u + col == (uint32_t)t)
                return ix;
            ix++;
        }
    }
    return 0u;
}

static void reset_section_arp(void)
{
    g_ctrl.cfg.scale = 1;
    g_ctrl.cfg.chord = 2;
    g_ctrl.cfg.arp_mode = 0;
    g_ctrl.cfg.arp_rate = 4;
    g_ctrl.cfg.arp_span = 0;
    g_ctrl.cfg.arp_gate = 64;
    g_ctrl.cfg.vel_mode = 3;
    g_ctrl.cfg.root = 0;
    g_ctrl.cfg.bank_page[CTRL_BANK_ARP] = 0;
}

static void reset_section_seq(void)
{
    uint32_t i, s;

    for (i = 0; i < CTRL_NLANES; i++) {
        fm_seq_bits_fill(g_ctrl.cfg.seq_pat[i], 0);
        g_ctrl.cfg.lane_mute[i] = 0;
        g_ctrl.cfg.euclid_hits[i] = 0;
        g_ctrl.cfg.euclid_rot[i] = 0;
        for (s = 0; s < CTRL_NSTEPS; s++) {
            g_ctrl.cfg.seq_note_ofs[i][s] = 0;
            g_ctrl.cfg.seq_vel[i][s] = 100;
            g_ctrl.cfg.seq_prob[i][s] = 100;
        }
    }
    g_ctrl.cfg.seq_lane = 0;
    g_ctrl.cfg.seq_pages = 1;
    g_ctrl.seq_view_page = 0;
    g_ctrl.cfg.seq_pace = 1;
    g_ctrl.cfg.swing = 32;
    g_ctrl.cfg.bank_page[CTRL_BANK_SEQ] = 0;
    for (i = 0; i < CTRL_NLANES; i++) {
        g_ctrl.pattern_origin[i] = FM_SEQ_PATTERN_ORIGIN_NONE;
        g_ctrl.seq_vary[i] = 0;
        g_ctrl.seq_lane_vel[i] = 0;
        g_ctrl.seq_lane_trig[i] = 0;
        g_ctrl.seq_lane_gate[i] = 0;
        g_ctrl.seq_lane_tune[i] = 0;
        g_ctrl.bk_last_ms[i] = 0;
        for (s = 0; s < CTRL_NSTEPS; s++)
            g_ctrl.seq_micro[i][s] = 0;
    }
}

static void reset_section_rec(void)
{
    uint32_t i, b;

    g_ctrl.motion_state = CTRL_MOTION_OFF;
    g_ctrl.cfg.motion_bars = 2;
    for (i = 0; i < CTRL_NPARAMS; i++) {
        for (b = 0; b < CTRL_MOTION_BUCKETS; b++)
            g_ctrl.motion[i][b] = 255u;
    }
    g_ctrl.cfg.bank_page[CTRL_BANK_REC] = 0;
}

static void reset_section_home(void)
{
    g_ctrl.cfg.home_macro[0] = 64;
    g_ctrl.cfg.home_macro[1] = 64;
    g_ctrl.cfg.home_macro[2] = 100;
    g_ctrl.cfg.home_macro[3] = 32;
    g_ctrl.cfg.bank_page[CTRL_BANK_HOME] = 0;
}

static void reset_bank(uint8_t bank)
{
    uint32_t col;

    if (bank > CTRL_BANK_D)
        return;
    g_ctrl.cfg.bank_count[bank] = 4u;
    for (col = 0; col < 8u; col++)
        reset_slot_default((uint32_t)bank * 8u + col);
}

static int32_t lfo_wave_sample(uint32_t slot, uint64_t ph64, uint32_t ph, uint32_t cycle)
{
    (void)ph64;
    {
        uint32_t off = (uint32_t)g_ctrl.cfg.lfo_phase[slot] << 25;
        uint32_t ph2 = ph + off;

        if (ph2 < ph)
            cycle++;
        ph = ph2;
    }
    return fm_lfo_eval_full_at(ph, cycle, slot, g_ctrl.cfg.lfo_shape[slot], g_ctrl.cfg.lfo_skew[slot],
                               g_ctrl.cfg.lfo_algo[slot], g_ctrl.cfg.lfo_ratio[slot],
                               g_ctrl.cfg.lfo_index[slot], g_ctrl.cfg.lfo_fb[slot],
                               g_ctrl.cfg.lfo_fold[slot]);
}

static int32_t ctrl_lfo_trace_sample(void *ctx, uint32_t ph, uint32_t cycle)
{
    uint32_t slot = (uint32_t)(uintptr_t)ctx;

    return fm_lfo_eval_full_q15(ph, cycle, slot, g_ctrl.cfg.lfo_shape[slot], g_ctrl.cfg.lfo_skew[slot],
                                g_ctrl.cfg.lfo_algo[slot], g_ctrl.cfg.lfo_ratio[slot],
                                g_ctrl.cfg.lfo_index[slot], g_ctrl.cfg.lfo_fb[slot],
                                g_ctrl.cfg.lfo_fold[slot]);
}

static void draw_lfo_trace_slot(uint32_t slot, uint16_t col)
{
    uint64_t ph_off = (uint64_t)g_ctrl.cfg.lfo_phase[slot] << 25;
    uint64_t now64 = (((uint64_t)g_ctrl.lfo_cycle[slot] << 32) | (uint64_t)g_ctrl.lfo_ph[slot]) + ph_off;

    fm_lfo_draw_trace((void *)(uintptr_t)slot, ctrl_lfo_trace_sample, now64, g_ctrl.cfg.lfo_smooth[slot],
                      ctrl_lfo_depth_eff(&g_ctrl.cfg, slot), 8, 224, 66, 48, 8, 124, col);
}

static int32_t knob_gain(int32_t steps)
{
    return steps;
}

static uint8_t bank_page(void)
{
    if (g_ctrl.view == CTRL_VIEW_LFO)
        return g_ctrl.cfg.bank_page[CTRL_BANK_LFO] % 3u;
    if (g_ctrl.view == CTRL_VIEW_SEQ)
        return g_ctrl.cfg.bank_page[CTRL_BANK_SEQ] % FM_SEQ_NPAGES;
    return g_ctrl.cfg.bank_page[g_ctrl.bank % CTRL_BANK_COUNT] & 1u;
}

static void seq_flash_knob(const char *msg)
{
    uint32_t i;

    if (!msg)
        return;
    for (i = 0; i < 7u && msg[i]; i++)
        g_ctrl.seq_knob_flash[i] = msg[i];
    g_ctrl.seq_knob_flash[i] = '\0';
    g_ctrl.seq_knob_flash_ms = fm1_ms + 900u;
}

static uint8_t ctrl_lane_state_ix(uint8_t lane)
{
    if (ctrl_lane_is_solo(&g_ctrl, lane))
        return FM_SEQ_STATE_SOLO;
    if (g_ctrl.cfg.lane_mute[lane])
        return FM_SEQ_STATE_MUTE;
    return FM_SEQ_STATE_PLAY;
}

static void ctrl_lane_apply_state(uint8_t lane, uint8_t state, uint8_t edit)
{
    if (edit || state == FM_SEQ_STATE_PLAY) {
        g_ctrl.cfg.lane_mute[lane] = 0u;
        mark_dirty();
        return;
    }
    if (state == FM_SEQ_STATE_MUTE) {
        g_ctrl.cfg.lane_mute[lane] = 1u;
        g_ctrl.lane_solo_mask &= (uint8_t)~(1u << lane);
        mark_dirty();
    } else if (state == FM_SEQ_STATE_SOLO) {
        ctrl_lane_toggle_solo(&g_ctrl, lane);
        mark_dirty();
    } else if (state == FM_SEQ_STATE_INV) {
        uint32_t i;

        for (i = 0; i < FM_SEQ_MASK_BYTES; i++)
            g_ctrl.cfg.seq_pat[lane][i] = (uint8_t)~g_ctrl.cfg.seq_pat[lane][i];
        seq_flash_knob("INVERT");
        mark_dirty();
    }
}

static uint8_t ctrl_lane_meta_active(uint8_t lane)
{
    uint32_t s;

    if (g_ctrl.cfg.euclid_hits[lane] || g_ctrl.cfg.euclid_rot[lane] || g_ctrl.seq_vary[lane])
        return 1u;
    if (g_ctrl.seq_lane_vel[lane] || g_ctrl.seq_lane_trig[lane] || g_ctrl.seq_lane_gate[lane] ||
        g_ctrl.seq_lane_tune[lane])
        return 1u;
    for (s = 0; s < ctrl_seq_len(&g_ctrl); s++) {
        if (g_ctrl.cfg.seq_prob[lane][s] != 100u || g_ctrl.cfg.seq_vel[lane][s] != 100u ||
            g_ctrl.cfg.seq_note_ofs[lane][s] || g_ctrl.seq_micro[lane][s])
            return 1u;
    }
    return 0u;
}

static const char *ctrl_lane_clear_label(uint8_t lane)
{
    if (ctrl_seq_pat_is16(&g_ctrl, lane, 0u) && !ctrl_lane_meta_active(lane))
        return "EMPTY";
    if (ctrl_seq_pat_is16(&g_ctrl, lane, ctrl_seq_lane_song_pat(lane)) && !ctrl_lane_meta_active(lane))
        return "SONG";
    return "EDITED";
}

static void ctrl_invert_all_mutes(void);

static void ctrl_apply_euclid_hits(uint8_t lane, uint8_t eh)
{
    if (lane >= CTRL_NLANES)
        return;
    if (eh == FM_SEQ_EUCLID_EMPTY)
        fm_seq_bits_fill(g_ctrl.cfg.seq_pat[lane], 0);
    else if (eh == FM_SEQ_EUCLID_EDITED)
        ;
    else if (eh == FM_SEQ_EUCLID_SONG || eh == FM_SEQ_EUCLID_FRESH)
        fm_seq_bits_tile16(g_ctrl.cfg.seq_pat[lane], ctrl_seq_lane_song_pat(lane));
    else
        ctrl_euclid_fill(g_ctrl.cfg.seq_pat[lane], eh, g_ctrl.cfg.euclid_rot[lane]);
}

static void ctrl_seq_lane_mark_edited(uint8_t lane)
{
    if (lane >= CTRL_NLANES)
        return;
    fm_seq_lane_pattern_mark_edited(&g_ctrl.cfg.euclid_hits[lane], &g_ctrl.pattern_origin[lane]);
}

static void ctrl_seq_knob_euclid_hits(uint8_t lane, int32_t steps, int edit)
{
    uint8_t oh;
    uint8_t nh;

    if (lane >= CTRL_NLANES)
        return;
    oh = g_ctrl.cfg.euclid_hits[lane];
    nh = fm_seq_euclid_step(oh, steps, edit, g_ctrl.pattern_origin[lane], 1);
    if (nh == FM_SEQ_EUCLID_EDITED) {
        if (oh != FM_SEQ_EUCLID_EDITED)
            ctrl_seq_lane_mark_edited(lane);
    } else {
        if (oh == FM_SEQ_EUCLID_EDITED && steps > 0 &&
            g_ctrl.pattern_origin[lane] != FM_SEQ_PATTERN_ORIGIN_NONE)
            g_ctrl.pattern_origin[lane] = FM_SEQ_PATTERN_ORIGIN_NONE;
        if (nh == FM_SEQ_EUCLID_FRESH)
            g_ctrl.cfg.euclid_hits[lane] = FM_SEQ_EUCLID_SONG;
        else
            g_ctrl.cfg.euclid_hits[lane] = nh;
        if (g_ctrl.cfg.euclid_hits[lane] != FM_SEQ_EUCLID_EDITED)
            ctrl_apply_euclid_hits(lane, g_ctrl.cfg.euclid_hits[lane]);
    }
}

static uint8_t ctrl_lane_p3_state_pos(uint8_t lane)
{
    if (ctrl_lane_is_solo(&g_ctrl, lane))
        return 2u;
    if (g_ctrl.cfg.lane_mute[lane])
        return 0u;
    return 1u;
}

static void ctrl_lane_apply_p3_state(uint8_t lane, uint8_t pos)
{
    uint32_t i;
    uint8_t bit;

    if (lane >= CTRL_NLANES)
        return;
    bit = (uint8_t)(1u << lane);
    pos %= 3u;
    if (pos == 0u) {
        g_ctrl.lane_solo_mask &= (uint8_t)~bit;
        g_ctrl.cfg.lane_mute[lane] = 1u;
    } else if (pos == 1u) {
        if (g_ctrl.lane_solo_mask & bit) {
            g_ctrl.lane_solo_mask &= (uint8_t)~bit;
            if (g_ctrl.lane_solo_mask == 0u) {
                for (i = 0; i < CTRL_NLANES; i++)
                    g_ctrl.cfg.lane_mute[i] = 0u;
            }
        } else {
            g_ctrl.cfg.lane_mute[lane] = 0u;
        }
    } else {
        g_ctrl.lane_solo_mask = bit;
        for (i = 0; i < CTRL_NLANES; i++)
            g_ctrl.cfg.lane_mute[i] = (uint8_t)(i != (uint32_t)lane);
    }
    ctrl_maybe_sync_heard_mutes(&g_ctrl);
}

static const uint8_t CTRL_ENS_MUTE[FM_SEQ_NENSEMBLES] = {
    0xFu, 0xEu, 0x6u, 0x3u, 0xDu, 0x5u, 0xAu, 0x6u, 0xAu,
    0x5u, 0x9u, 0x5u, 0xAu, 0x2u, 0xCu, 0x8u, 0x1u, 0x0u
};

static uint8_t ctrl_detect_ensemble(void)
{
    uint8_t m = 0u;
    uint32_t i;

    for (i = 0; i < CTRL_NLANES; i++) {
        if (g_ctrl.cfg.lane_mute[i])
            m |= (uint8_t)(1u << i);
    }
    for (i = 0; i < FM_SEQ_NENSEMBLES; i++) {
        if (m == CTRL_ENS_MUTE[i])
            return (uint8_t)i;
    }
    return FM_SEQ_ENS_CUSTOM;
}

static void ctrl_apply_ensemble(uint8_t ens)
{
    uint32_t i;
    uint8_t mp;

    if (ens >= FM_SEQ_NENSEMBLES)
        ens = FM_SEQ_ENS_ALL;
    g_ctrl.seq_ens_sel = ens;
    g_ctrl.lane_solo_mask = 0u;
    mp = CTRL_ENS_MUTE[ens];
    for (i = 0; i < CTRL_NLANES; i++)
        g_ctrl.cfg.lane_mute[i] = (mp >> i) & 1u;
    ctrl_maybe_sync_heard_mutes(&g_ctrl);
}

static void ctrl_invert_all_mutes(void)
{
    uint32_t ln;
    uint8_t was[CTRL_NLANES];

    for (ln = 0; ln < CTRL_NLANES; ln++)
        was[ln] = g_ctrl.cfg.lane_mute[ln];
    g_ctrl.lane_solo_mask = 0u;
    for (ln = 0; ln < CTRL_NLANES; ln++)
        g_ctrl.cfg.lane_mute[ln] = was[ln] ? 0u : 1u;
    mark_dirty();
}

static void learn_pick_col(uint8_t k)
{
    uint8_t b = g_ctrl.bank % 4u;
    uint32_t pg = bank_page() ? 4u : 0u;
    uint32_t rel_col = pg + (uint32_t)k;
    uint32_t slot = (uint32_t)b * 8u + rel_col;

    if (k >= 4u || slot >= CTRL_NPARAMS)
        return;
    if (rel_col >= g_ctrl.cfg.bank_count[b]) {
        if (rel_col >= 8u)
            return;
        g_ctrl.cfg.bank_count[b] = (uint8_t)(rel_col + 1u);
    }
    g_ctrl.learn_slot = (uint8_t)slot;
    mark_dirty();
}

static void nav_bank(uint8_t bank)
{
    g_ctrl.bank = bank;
    if (bank <= CTRL_BANK_D)
        g_ctrl.view = CTRL_VIEW_BANK;
    else if (bank == CTRL_BANK_LFO)
        g_ctrl.view = CTRL_VIEW_LFO;
    else if (bank == CTRL_BANK_ARP)
        g_ctrl.view = CTRL_VIEW_ARP;
    else if (bank == CTRL_BANK_SEQ) {
        if (g_ctrl.cfg.arp_mode != 0u) {
            g_ctrl.cfg.arp_mode = 0u;
            mark_dirty();
        }
        g_ctrl.view = CTRL_VIEW_SEQ;
    }
    else if (bank == CTRL_BANK_REC)
        g_ctrl.view = CTRL_VIEW_REC;
    else
        g_ctrl.view = CTRL_VIEW_HOME;
    g_ctrl.ui_dirty = 1u;
}

static uint32_t knob_slot(uint8_t col)
{
    if (g_ctrl.view == CTRL_VIEW_HOME)
        return 255u;
    if (g_ctrl.view == CTRL_VIEW_LFO || g_ctrl.view == CTRL_VIEW_ARP || g_ctrl.view == CTRL_VIEW_SEQ
        || g_ctrl.view == CTRL_VIEW_REC)
        return 255u;
    if (g_ctrl.bank <= CTRL_BANK_D) {
        uint32_t pg = bank_page() ? 4u : 0u;
        return (uint32_t)g_ctrl.bank * 8u + pg + col;
    }
    (void)col;
    return 255u;
}

static void knob_label(uint8_t col, char *s)
{
    if (g_ctrl.view == CTRL_VIEW_HOME) {
        static const char *const L[4] = {"LFO AMT", "LFO SPD", "DYNAM", "SPACE"};
        str_copy(s, L[col]);
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_LFO) {
        uint8_t pg = bank_page();
        if (pg == 0u) {
            static const char *const L[4] = {"TARGET", "RATE", "DEPTH", "SHAPE"};
            str_copy(s, L[col]);
        } else if (pg == 1u) {
            static const char *const L[4] = {"PHASE", "SKEW", "SMOOTH", "FOLD"};
            str_copy(s, L[col]);
        } else {
            static const char *const L[4] = {"ALGO", "RATIO", "FM IDX", "OP FB"};
            str_copy(s, L[col]);
        }
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_ARP) {
        if (!bank_page()) {
            static const char *const L[4] = {"SCALE", "CHORD", "ARP", "RATE"};
            str_copy(s, L[col]);
        } else {
            static const char *const L[4] = {"SPAN", "GATE", "VEL", "ROOT"};
            str_copy(s, L[col]);
        }
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_SEQ) {
        int plock = g_ctrl.seq_plock_step < CTRL_NSTEPS;
        str_copy(s, fm_seq_knob_label(bank_page(), plock, col));
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_REC) {
        static const char *const L[4] = {"MOTION", "LENGTH", "MIDI CH", "LOCAL"};
        str_copy(s, L[col]);
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.learn_active) {
        static const char *const L[4] = {"CC NUM", "PRESET", "CHAR 1", "FADERS"};
        if (col == 2) {
            str_copy(s, "CHAR ");
            s[5] = (char)('1' + g_ctrl.edit_char_ix);
            s[6] = 0;
            return;
        }
        str_copy(s, L[col]);
        return;
    }
    {
        uint32_t slot = knob_slot(col);
        if (slot < CTRL_NPARAMS) {
            ctrl_slot_label(&g_ctrl.cfg, slot, s);
            return;
        }
    }
    s[0] = '-';
    s[1] = 0;
}

static void knob_value(uint8_t col, char *s)
{
    if (g_ctrl.view == CTRL_VIEW_HOME) {
        if (col == 0) {
            u8_dec(s, (g_ctrl.cfg.home_macro[0] * 200u) / 127u);
            str_cat(s, "%");
        }
        else if (col == 1)
            bip50_show(s, g_ctrl.cfg.home_macro[1]);
        else if (col == 2)
            u8_show100_dec(s, g_ctrl.cfg.home_macro[2]);
        else
            u8_show100_dec(s, g_ctrl.cfg.home_macro[3]);
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_LFO) {
        uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
        uint8_t pg = bank_page();
        uint8_t rate = ctrl_lfo_rate_eff(&g_ctrl.cfg, t, g_ctrl.bpm_eff);
        const char *nd;

        if (pg == 0u) {
            if (col == 0) {
                ctrl_slot_label(&g_ctrl.cfg, t, s);
                return;
            }
            if (col == 1) {
                nd = fm_lfo_note_div(rate, g_ctrl.bpm_eff);
                if (nd)
                    str_copy(s, nd);
                else
                    fmt_hz100(s, fm_lfo_hz100(rate));
                return;
            }
            if (col == 2) {
                u8_show100_dec(s, g_ctrl.cfg.lfo_depth[t]);
                return;
            }
            str_copy(s, fm_lfo_shape_name(g_ctrl.cfg.lfo_shape[t]));
            return;
        }
        if (pg == 1u) {
            if (col == 0)
                u8_show100_dec(s, g_ctrl.cfg.lfo_phase[t]);
            else if (col == 1)
                bip50_show(s, g_ctrl.cfg.lfo_skew[t]);
            else if (col == 2)
                u8_show100_dec(s, g_ctrl.cfg.lfo_smooth[t]);
            else
                fm_lfo_fmt_depth(s, (int32_t)g_ctrl.cfg.lfo_fold[t]);
            return;
        }
        if (col == 0)
            str_copy(s, FM_LFO_ALG_NAMES[g_ctrl.cfg.lfo_algo[t] % FM_LFO_NALGOS]);
        else if (col == 1)
            fm_lfo_fmt_ratio(s, g_ctrl.cfg.lfo_ratio[t]);
        else if (col == 2)
            fm_lfo_fmt_depth(s, (int32_t)g_ctrl.cfg.lfo_index[t]);
        else
            fm_lfo_fmt_depth(s, (int32_t)g_ctrl.cfg.lfo_fb[t]);
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_ARP) {
        static const char *const ARP[6] = {"OFF", "UP", "DOWN", "UP+DN", "RAND", "CHORD"};
        static const char *const RATE[6] = {"1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32"};
        static const char *const VEL[4] = {"FIX", "ACC", "LFO", "POT"};
        static const char *const SPAN[3] = {"1 OCT", "2 OCT", "3 OCT"};
        if (!bank_page()) {
            if (col == 0) {
                str_copy(s, CTRL_SCALE_NAMES[g_ctrl.cfg.scale % CTRL_NSCALES]);
                return;
            }
            if (col == 1) {
                str_copy(s, CTRL_CHORD_NAMES[g_ctrl.cfg.chord % CTRL_NCHORDS]);
                return;
            }
            if (col == 2) {
                str_copy(s, ARP[g_ctrl.cfg.arp_mode % 6u]);
                return;
            }
            str_copy(s, RATE[g_ctrl.cfg.arp_rate % 6u]);
            return;
        }
        if (col == 0)
            str_copy(s, SPAN[g_ctrl.cfg.arp_span % 3u]);
        else if (col == 1) {
            u8_dec(s, g_ctrl.cfg.arp_gate);
            str_cat(s, "%");
        }
        else if (col == 2)
            str_copy(s, VEL[g_ctrl.cfg.vel_mode % 4u]);
        else
            str_copy(s, NOTE_NAMES[g_ctrl.cfg.root % 12u]);
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_SEQ) {
        uint8_t lane = g_ctrl.cfg.seq_lane % CTRL_NLANES;
        static const char *const LANE[4] = {"L1", "L2", "L3", "L4"};
        uint8_t cond;

        if (g_ctrl.seq_plock_step < CTRL_NSTEPS) {
            uint8_t k = g_ctrl.seq_plock_step;
            if (col == 0)
                u8_show100_dec(s, g_ctrl.cfg.seq_vel[lane][k]);
            else if (col == 1) {
                cond = g_ctrl.cfg.seq_prob[lane][k];
                if (cond >= FM_SEQ_NCONDS)
                    cond = 0u;
                str_copy(s, FM_SEQ_COND_NAMES[cond]);
            } else if (col == 2)
                i8_dec(s, g_ctrl.seq_micro[lane][k]);
            else
                i8_dec(s, g_ctrl.cfg.seq_note_ofs[lane][k]);
            return;
        }
        if (bank_page() == 2u) {
            if (col == 0)
                str_copy(s, LANE[lane]);
            else if (col == 1)
                str_copy(s, FM_SEQ_P3_STATE_NAMES[ctrl_lane_p3_state_pos(lane)]);
            else if (col == 2) {
                uint8_t det = ctrl_detect_ensemble();
                str_copy(s, det == FM_SEQ_ENS_CUSTOM ? "CUSTOM" : FM_SEQ_ENS_NAMES[det]);
            } else
                str_copy(s, FM_SEQ_FLIP_NAMES[g_ctrl.seq_flip_state & 1u]);
            return;
        }
        if (bank_page() == 0u) {
            if (col == 0)
                str_copy(s, LANE[lane]);
            else if (col == 1) {
                uint8_t eh = g_ctrl.cfg.euclid_hits[lane];

                if (eh == FM_SEQ_EUCLID_SONG &&
                    !ctrl_seq_pat_is16(&g_ctrl, lane, ctrl_seq_lane_song_pat(lane)))
                    str_copy(s, "EDITED");
                else
                    fm_seq_fmt_euclid(eh, s);
            }
            else if (col == 2)
                u8_dec(s, fm_seq_euclid_rot_norm(g_ctrl.cfg.euclid_rot[lane]));
            else
                fm_seq_fmt_vary(g_ctrl.seq_vary[lane], s);
            return;
        }
        {
            uint8_t fn = fm_seq_p1_fn((uint8_t)col);

            if (fn == 1u) {
                fm_seq_fmt_trig(g_ctrl.seq_lane_trig[lane], s);
                return;
            }
            fm_seq_fmt_rand_spread(fn == 0u ? g_ctrl.seq_lane_vel[lane] :
                                   fn == 2u ? g_ctrl.seq_lane_gate[lane] : g_ctrl.seq_lane_tune[lane], s);
        }
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_REC) {
        static const char *const MOT[5] = {"OFF", "ARM", "REC", "PLAY", "CLR"};
        static const char *const LOC[4] = {"S+VOL", "S+CC1", "M+CC1", "M+EXP"};
        if (col == 0) {
            str_copy(s, MOT[g_ctrl.motion_state % 5u]);
            return;
        }
        if (col == 1) {
            u8_dec(s, (uint32_t)(1u << g_ctrl.cfg.motion_bars));
            return;
        }
        if (col == 2) {
            u8_dec(s, g_ctrl.cfg.midi_ch);
            return;
        }
        str_copy(s, LOC[g_ctrl.cfg.local_pot % 4u]);
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.learn_active) {
        ctrl_cfg_t *c = &g_ctrl.cfg;
        uint8_t b = g_ctrl.bank % 4u;
        uint8_t s_ix = g_ctrl.learn_slot % CTRL_NPARAMS;

        if (col == 0) {
            str_copy(s, "CC ");
            u8_dec(s + 3, c->cc_map[s_ix]);
            return;
        }
        if (col == 1) {
            ctrl_slot_label(c, s_ix, s);
            return;
        }
        if (col == 2) {
            char ch = c->custom_label[s_ix][g_ctrl.edit_char_ix % 4u];
            if (ch == ' ')
                str_copy(s, "SPC");
            else {
                s[0] = ch;
                s[1] = 0;
            }
            return;
        }
        u8_dec(s, c->bank_count[b]);
        return;
    }
    {
        uint32_t slot = knob_slot(col);
        if (slot < CTRL_NPARAMS) {
            u8_dec(s, g_ctrl.eff_val[slot]);
            return;
        }
    }
    s[0] = '-';
    s[1] = 0;
}

static uint8_t learn_cc_default(uint8_t slot)
{
    uint8_t p = g_ctrl.cfg.profile % CTRL_NPROFILES;
    uint32_t b = slot >> 3;
    uint32_t c = slot & 7u;

    if (c < 4u)
        return CTRL_PROFILE_CC[p][b * 4u + c];
    return (uint8_t)(20u + b * 4u + (c - 4u));
}

static void knob_edit_turn(uint8_t col, uint32_t slot, int32_t g)
{
    ctrl_cfg_t *c = &g_ctrl.cfg;
    uint8_t t;
    int32_t v;

    edit_mark();
    if (g_ctrl.learn_active && g_ctrl.view == CTRL_VIEW_BANK) {
        uint8_t s_ix = g_ctrl.learn_slot % CTRL_NPARAMS;
        uint8_t b = g_ctrl.bank % 4u;

        if (col == 0)
            c->cc_map[s_ix] = g < 0 ? learn_cc_default(s_ix) : 127u;
        else if (col == 1)
            g_ctrl.tag_ix = g < 0 ? 0u : (uint8_t)(CTRL_NTAGS - 1u);
        else if (col == 2)
            c->custom_label[s_ix][g_ctrl.edit_char_ix % 4u] = g < 0 ? 'A' : 'Z';
        else
            c->bank_count[b] = g < 0 ? 4u : 8u;
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_LFO) {
        t = c->lfo_target % CTRL_NPARAMS;
        {
            uint8_t pg = bank_page();

            if (pg == 0u) {
                if (col == 0)
                    c->lfo_target = g < 0 ? (uint8_t)ctrl_active_nth(c, 0u) : (uint8_t)ctrl_active_nth(
                        c, ctrl_active_count(c) - 1u);
                else if (col == 1)
                    c->lfo_rate[t] = g < 0 ? FM_LFO_RATE_DEFAULT : 127u;
                else if (col == 2)
                    c->lfo_depth[t] = g < 0 ? 0u : 127u;
                else
                    c->lfo_shape[t] = g < 0 ? 0u : 127u;
            } else if (pg == 1u) {
                if (col == 0)
                    c->lfo_phase[t] = g < 0 ? 0u : 127u;
                else if (col == 1)
                    c->lfo_skew[t] = g < 0 ? FM_LFO_SKEW_DEFAULT : 127u;
                else if (col == 2)
                    c->lfo_smooth[t] = g < 0 ? 0u : 127u;
                else
                    c->lfo_fold[t] = g < 0 ? 0u : 127u;
            } else if (col == 0)
                c->lfo_algo[t] = g < 0 ? 0u : (uint8_t)(FM_LFO_NALGOS - 1u);
            else if (col == 1)
                c->lfo_ratio[t] = g < 0 ? 52u : 127u;
            else if (col == 2)
                c->lfo_index[t] = g < 0 ? 64u : 127u;
            else
                c->lfo_fb[t] = g < 0 ? 0u : 127u;
        }
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_HOME) {
        if (g < 0)
            reset_section_home();
        else {
            c->home_macro[0] = 127u;
            c->home_macro[1] = 127u;
            c->home_macro[2] = 127u;
            c->home_macro[3] = 127u;
        }
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_ARP) {
        if (!bank_page()) {
            if (col == 0)
                c->scale = g < 0 ? 1u : (uint8_t)(CTRL_NSCALES - 1u);
            else if (col == 1)
                c->chord = g < 0 ? 2u : (uint8_t)(CTRL_NCHORDS - 1u);
            else if (col == 2)
                c->arp_mode = g < 0 ? 0u : 5u;
            else
                c->arp_rate = g < 0 ? 4u : 5u;
        } else if (col == 0)
            c->arp_span = g < 0 ? 0u : 2u;
        else if (col == 1)
            c->arp_gate = g < 0 ? 64u : 95u;
        else if (col == 2)
            c->vel_mode = g < 0 ? 3u : 3u;
        else
            c->root = g < 0 ? 0u : 11u;
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_SEQ) {
        uint8_t lane = c->seq_lane % CTRL_NLANES;
        uint8_t pg = bank_page();

        if (g_ctrl.seq_plock_step < CTRL_NSTEPS) {
            uint8_t k = g_ctrl.seq_plock_step;
            uint8_t vel_b = c->seq_vel[lane][k];
            uint8_t prob_b = c->seq_prob[lane][k];
            int8_t micro_b = g_ctrl.seq_micro[lane][k];
            int8_t note_b = c->seq_note_ofs[lane][k];

            g_ctrl.seq_plock_knob = 1u;
            if (col == 0)
                c->seq_vel[lane][k] = g < 0 ? 100u : 127u;
            else if (col == 1)
                c->seq_prob[lane][k] = g < 0 ? (uint8_t)FM_SEQ_COND_MAX : 0u;
            else if (col == 2)
                g_ctrl.seq_micro[lane][k] = g < 0 ? 0 : 25;
            else
                c->seq_note_ofs[lane][k] = g < 0 ? 0 : 24;
            if (c->seq_vel[lane][k] != vel_b || c->seq_prob[lane][k] != prob_b ||
                g_ctrl.seq_micro[lane][k] != micro_b || c->seq_note_ofs[lane][k] != note_b)
                ctrl_seq_lane_mark_edited(lane);
        } else if (pg == 1u) {
            int8_t old;
            uint8_t fn = fm_seq_p1_fn((uint8_t)col);

            if (fn == 0u) {
                old = g_ctrl.seq_lane_vel[lane];
                g_ctrl.seq_lane_vel[lane] = g < 0 ? 0 : 50;
                if (g_ctrl.seq_lane_vel[lane] != old)
                    ctrl_seq_lane_mark_edited(lane);
            } else if (fn == 1u) {
                old = g_ctrl.seq_lane_trig[lane];
                g_ctrl.seq_lane_trig[lane] = g < 0 ? 0 : (int8_t)-100;
                if (g_ctrl.seq_lane_trig[lane] != old)
                    ctrl_seq_lane_mark_edited(lane);
            } else if (fn == 2u) {
                old = g_ctrl.seq_lane_gate[lane];
                g_ctrl.seq_lane_gate[lane] = g < 0 ? 0 : 50;
                if (g_ctrl.seq_lane_gate[lane] != old)
                    ctrl_seq_lane_mark_edited(lane);
            } else {
                old = g_ctrl.seq_lane_tune[lane];
                g_ctrl.seq_lane_tune[lane] = g < 0 ? 0 : 24;
                if (g_ctrl.seq_lane_tune[lane] != old)
                    ctrl_seq_lane_mark_edited(lane);
            }
        } else if (pg == 0u) {
            if (col == 2) {
                uint8_t old = g_ctrl.seq_vary[lane];
                v = g < 0 ? 0 : 127;
                g_ctrl.seq_vary[lane] = (uint8_t)v;
                if (g_ctrl.seq_vary[lane] != old)
                    ctrl_seq_lane_mark_edited(lane);
            } else if (col == 3) {
                int8_t old = c->euclid_rot[lane];
                v = g < 0 ? 0 : 15;
                c->euclid_rot[lane] = (int8_t)v;
                if (c->euclid_rot[lane] != old) {
                    ctrl_apply_euclid_hits(lane, c->euclid_hits[lane]);
                    ctrl_seq_lane_mark_edited(lane);
                }
            }
        }
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_REC) {
        if (col == 0)
            g_ctrl.motion_state = g < 0 ? CTRL_MOTION_OFF : CTRL_MOTION_CLEAR;
        else if (col == 1)
            c->motion_bars = g < 0 ? 2u : 3u;
        else if (col == 2)
            c->midi_ch = g < 0 ? 1u : 16u;
        else
            c->local_pot = g < 0 ? 0u : 3u;
        mark_dirty();
        return;
    }
    if (slot < CTRL_NPARAMS) {
        if (g < 0) {
            reset_slot_default(slot);
            mark_dirty();
        } else {
            c->base_val[slot] = 127u;
            g_ctrl.eff_val[slot] = 127u;
            ctrl_set_param(&g_ctrl, slot, 127u);
            mark_dirty();
        }
        return;
    }
}

static void knob_adjust(uint8_t col, int32_t steps)
{
    int32_t g = knob_gain(steps);
    uint32_t slot = knob_slot(col);

    if (g_ctrl.edit_down && steps != 0) {
        if (g_ctrl.view == CTRL_VIEW_SEQ) {
            uint8_t lane = g_ctrl.cfg.seq_lane % CTRL_NLANES;
            uint8_t pg = bank_page();

            if (pg == 2u && col == 1) {
                edit_mark();
                ctrl_lane_apply_p3_state(lane, 1u);
                mark_dirty();
                return;
            }
            if (pg == 2u && col == 2) {
                edit_mark();
                ctrl_apply_ensemble(FM_SEQ_ENS_ALL);
                mark_dirty();
                return;
            }
            if (pg == 2u && col == 3) {
                edit_mark();
                ctrl_invert_all_mutes();
                g_ctrl.seq_flip_state ^= 1u;
                mark_dirty();
                return;
            }
            if (pg == 0u && col == 1) {
                edit_mark();
                ctrl_seq_knob_euclid_hits(lane, steps, 1);
                mark_dirty();
                return;
            }
            if (pg == 1u && fm_seq_p1_fn((uint8_t)col) == 1u) {
                edit_mark();
                g_ctrl.seq_lane_trig[lane] = fm_seq_trig_adj(g_ctrl.seq_lane_trig[lane], g, 1);
                mark_dirty();
                return;
            }
        }
        knob_edit_turn(col, slot, g);
        return;
    }

    if (g_ctrl.learn_active && g_ctrl.view == CTRL_VIEW_BANK) {
        ctrl_cfg_t *c = &g_ctrl.cfg;
        uint8_t b = g_ctrl.bank % 4u;
        uint8_t s_ix = g_ctrl.learn_slot % CTRL_NPARAMS;
        uint32_t i;

        if (col == 0) {
            int32_t v = (int32_t)c->cc_map[s_ix] + g;
            c->cc_map[s_ix] = (uint8_t)ctrl_clamp127(v);
        } else if (col == 1) {
            int32_t v = (int32_t)g_ctrl.tag_ix + g;
            while (v < 0)
                v += (int32_t)CTRL_NTAGS;
            g_ctrl.tag_ix = (uint8_t)((uint32_t)v % CTRL_NTAGS);
            for (i = 0; i < 4u; i++)
                c->custom_label[s_ix][i] = CTRL_TAG_LIB[g_ctrl.tag_ix][i];
        } else if (col == 2) {
            uint8_t ci = g_ctrl.edit_char_ix % 4u;
            char ch = c->custom_label[s_ix][ci];
            c->custom_label[s_ix][ci] = char_cycle_step(ch, g);
        } else {
            int32_t v = (int32_t)c->bank_count[b] + g;
            uint8_t old = c->bank_count[b];

            if (v < 1)
                v = 1;
            if (v > 8)
                v = 8;
            c->bank_count[b] = (uint8_t)v;
            if (old <= 4 && v > 4) {
                c->bank_page[b] = 1u;
                g_ctrl.learn_slot = (uint8_t)((uint32_t)b * 8u + (uint32_t)v - 1u);
            }
            if (v <= 4)
                c->bank_page[b] = 0u;
        }
        mark_dirty();
        return;
    }

    if (g_ctrl.view == CTRL_VIEW_HOME) {
        if (col == 0)
            g_ctrl.cfg.home_macro[0] = (uint8_t)ctrl_clamp127((int32_t)g_ctrl.cfg.home_macro[0] + g);
        else if (col == 1)
            g_ctrl.cfg.home_macro[1] = (uint8_t)ctrl_clamp127((int32_t)g_ctrl.cfg.home_macro[1] + g);
        else if (col == 2)
            g_ctrl.cfg.home_macro[2] = (uint8_t)ctrl_clamp127((int32_t)g_ctrl.cfg.home_macro[2] + g);
        else
            g_ctrl.cfg.home_macro[3] = (uint8_t)ctrl_clamp127((int32_t)g_ctrl.cfg.home_macro[3] + g);
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_LFO) {
        uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
        uint8_t pg = bank_page();
        if (pg == 0u) {
            if (col == 0) {
                g_ctrl.cfg.lfo_target = ctrl_lfo_target_step(&g_ctrl.cfg, t, g);
            } else if (col == 1) {
                int32_t v = (int32_t)g_ctrl.cfg.lfo_rate[t] + g;

                g_ctrl.cfg.lfo_rate[t] =
                    fm_lfo_rate_clamp((uint8_t)ctrl_clamp127(v), g_ctrl.bpm_eff);
            } else if (col == 2) {
                int32_t v = (int32_t)g_ctrl.cfg.lfo_depth[t] + g;
                g_ctrl.cfg.lfo_depth[t] = (uint8_t)ctrl_clamp127(v);
            } else
                g_ctrl.cfg.lfo_shape[t] = fm_lfo_step_shape(g_ctrl.cfg.lfo_shape[t], g, 0u);
        } else if (pg == 1u) {
            if (col == 0) {
                int32_t v = (int32_t)g_ctrl.cfg.lfo_phase[t] + g;
                g_ctrl.cfg.lfo_phase[t] = (uint8_t)ctrl_clamp127(v);
            } else if (col == 1) {
                int32_t v = (int32_t)g_ctrl.cfg.lfo_skew[t] + g;
                g_ctrl.cfg.lfo_skew[t] = (uint8_t)ctrl_clamp127(v);
            } else if (col == 2) {
                int32_t v = (int32_t)g_ctrl.cfg.lfo_smooth[t] + g;
                g_ctrl.cfg.lfo_smooth[t] = (uint8_t)ctrl_clamp127(v);
            } else {
                int32_t v = (int32_t)g_ctrl.cfg.lfo_fold[t] + g;
                g_ctrl.cfg.lfo_fold[t] = (uint8_t)ctrl_clamp127(v);
            }
        } else if (col == 0) {
            int32_t v = (int32_t)g_ctrl.cfg.lfo_algo[t] + g;
            while (v < 0)
                v += (int32_t)FM_LFO_NALGOS;
            g_ctrl.cfg.lfo_algo[t] = (uint8_t)((uint32_t)v % FM_LFO_NALGOS);
        } else if (col == 1) {
            int32_t v = (int32_t)g_ctrl.cfg.lfo_ratio[t] + g;
            g_ctrl.cfg.lfo_ratio[t] = (uint8_t)ctrl_clamp127(v);
        } else if (col == 2) {
            int32_t v = (int32_t)g_ctrl.cfg.lfo_index[t] + g;
            g_ctrl.cfg.lfo_index[t] = (uint8_t)ctrl_clamp127(v);
        } else {
            int32_t v = (int32_t)g_ctrl.cfg.lfo_fb[t] + g;
            g_ctrl.cfg.lfo_fb[t] = (uint8_t)ctrl_clamp127(v);
        }
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_ARP) {
        if (!bank_page()) {
            if (col == 0)
                g_ctrl.cfg.scale = (uint8_t)((g_ctrl.cfg.scale + (uint32_t)g + CTRL_NSCALES) % CTRL_NSCALES);
            else if (col == 1)
                g_ctrl.cfg.chord = (uint8_t)((g_ctrl.cfg.chord + (uint32_t)g + CTRL_NCHORDS) % CTRL_NCHORDS);
            else if (col == 2) {
                int32_t v = (int32_t)g_ctrl.cfg.arp_mode + g;
                while (v < 0)
                    v += 6;
                g_ctrl.cfg.arp_mode = (uint8_t)((uint32_t)v % 6u);
            } else {
                int32_t v = (int32_t)g_ctrl.cfg.arp_rate + g;
                while (v < 0)
                    v += 6;
                g_ctrl.cfg.arp_rate = (uint8_t)((uint32_t)v % 6u);
            }
        } else if (col == 0) {
            int32_t v = (int32_t)g_ctrl.cfg.arp_span + g;
            if (v < 0)
                v = 0;
            if (v > 2)
                v = 2;
            g_ctrl.cfg.arp_span = (uint8_t)v;
        } else if (col == 1) {
            int32_t v = (int32_t)g_ctrl.cfg.arp_gate + g;
            if (v < 15)
                v = 15;
            if (v > 95)
                v = 95;
            g_ctrl.cfg.arp_gate = (uint8_t)v;
        } else if (col == 2) {
            int32_t v = (int32_t)g_ctrl.cfg.vel_mode + g;
            while (v < 0)
                v += 4;
            g_ctrl.cfg.vel_mode = (uint8_t)((uint32_t)v % 4u);
        } else {
            int32_t v = (int32_t)g_ctrl.cfg.root + g;
            while (v < 0)
                v += 12;
            g_ctrl.cfg.root = (uint8_t)((uint32_t)v % 12u);
        }
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_SEQ) {
        uint8_t lane = g_ctrl.cfg.seq_lane % CTRL_NLANES;
        uint8_t pg = bank_page();

        if (g_ctrl.seq_plock_step < CTRL_NSTEPS) {
            uint8_t k = g_ctrl.seq_plock_step;
            uint8_t cond;
            uint8_t vel_b = g_ctrl.cfg.seq_vel[lane][k];
            uint8_t prob_b = g_ctrl.cfg.seq_prob[lane][k];
            int8_t micro_b = g_ctrl.seq_micro[lane][k];
            int8_t note_b = g_ctrl.cfg.seq_note_ofs[lane][k];
            int was_on = fm_seq_bit(g_ctrl.cfg.seq_pat[lane], k);

            g_ctrl.seq_plock_knob = 1u;
            if (col == 0) {
                int32_t v = (int32_t)g_ctrl.cfg.seq_vel[lane][k] + g;
                if (v < 1)
                    v = 1;
                if (v > 127)
                    v = 127;
                g_ctrl.cfg.seq_vel[lane][k] = (uint8_t)v;
            } else if (col == 1) {
                cond = g_ctrl.cfg.seq_prob[lane][k];
                if (cond >= FM_SEQ_NCONDS)
                    cond = 0u;
                {
                    int32_t v = (int32_t)cond + (steps > 0 ? -1 : 1);
                    if (v < 0)
                        v = 0;
                    if (v > (int32_t)FM_SEQ_COND_MAX)
                        v = (int32_t)FM_SEQ_COND_MAX;
                    g_ctrl.cfg.seq_prob[lane][k] = (uint8_t)v;
                }
            } else if (col == 2) {
                int32_t v = (int32_t)g_ctrl.seq_micro[lane][k] + (steps * 2);
                if (v < -25)
                    v = -25;
                if (v > 25)
                    v = 25;
                g_ctrl.seq_micro[lane][k] = (int8_t)v;
            } else {
                int32_t v = (int32_t)g_ctrl.cfg.seq_note_ofs[lane][k] + steps;
                if (v < -24)
                    v = -24;
                if (v > 24)
                    v = 24;
                g_ctrl.cfg.seq_note_ofs[lane][k] = (int8_t)v;
            }
            if (fm_seq_bit(g_ctrl.cfg.seq_pat[lane], k) != was_on ||
                g_ctrl.cfg.seq_vel[lane][k] != vel_b || g_ctrl.cfg.seq_prob[lane][k] != prob_b ||
                g_ctrl.seq_micro[lane][k] != micro_b || g_ctrl.cfg.seq_note_ofs[lane][k] != note_b)
                ctrl_seq_lane_mark_edited(lane);
        } else if (pg == 2u) {
            if (col == 0) {
                g_ctrl.cfg.seq_lane = (uint8_t)((lane + (g > 0 ? 1u : 3u)) % 4u);
            } else if (col == 1) {
                if (g_ctrl.edit_down)
                    ctrl_lane_apply_p3_state(lane, 1u);
                else if (steps != 0) {
                    uint8_t prev = ctrl_lane_p3_state_pos(lane);
                    int32_t pos = (int32_t)prev + (steps > 0 ? 1 : -1);
                    if (pos < 0)
                        pos = 0;
                    if (pos > 2)
                        pos = 2;
                    ctrl_lane_apply_p3_state(lane, (uint8_t)pos);
                    if (ctrl_lane_p3_state_pos(lane) != prev)
                        ctrl_seq_lane_mark_edited(lane);
                }
            } else if (col == 2) {
                if (g_ctrl.edit_down)
                    ctrl_apply_ensemble(FM_SEQ_ENS_ALL);
                else if (steps != 0) {
                    int32_t ix = (int32_t)g_ctrl.seq_ens_sel + (steps > 0 ? 1 : -1);
                    if (ix < 0)
                        ix = 0;
                    if (ix >= (int32_t)FM_SEQ_NENSEMBLES)
                        ix = (int32_t)FM_SEQ_NENSEMBLES - 1;
                    ctrl_apply_ensemble((uint8_t)ix);
                }
            } else if (steps != 0) {
                ctrl_invert_all_mutes();
                g_ctrl.seq_flip_state ^= 1u;
            }
        } else if (pg == 1u) {
            uint8_t fn = fm_seq_p1_fn((uint8_t)col);

            if (fn == 1u) {
                int8_t old = g_ctrl.seq_lane_trig[lane];
                g_ctrl.seq_lane_trig[lane] =
                    fm_seq_trig_adj(g_ctrl.seq_lane_trig[lane], g, g_ctrl.edit_down);
                if (g_ctrl.seq_lane_trig[lane] != old)
                    ctrl_seq_lane_mark_edited(lane);
            } else {
                int8_t *fp = fn == 0u ? &g_ctrl.seq_lane_vel[lane] :
                             fn == 2u ? &g_ctrl.seq_lane_gate[lane] : &g_ctrl.seq_lane_tune[lane];
                int32_t lim = fn == 3u ? 24 : 50;
                int8_t old = *fp;
                int32_t v = (int32_t)*fp + (fn == 3u ? steps : g);

                if (v < -lim)
                    v = -lim;
                if (v > lim)
                    v = lim;
                *fp = (int8_t)v;
                if (*fp != old)
                    ctrl_seq_lane_mark_edited(lane);
            }
        } else if (col == 0) {
            g_ctrl.cfg.seq_lane = (uint8_t)((lane + (g > 0 ? 1u : 3u)) % 4u);
        } else if (col == 1) {
            ctrl_seq_knob_euclid_hits(lane, steps, g_ctrl.edit_down);
        } else if (col == 2) {
            int32_t step1 = steps > 0 ? 1 : -1;
            int32_t nr = (int32_t)fm_seq_euclid_rot_norm(g_ctrl.cfg.euclid_rot[lane]) + step1;

            g_ctrl.cfg.euclid_rot[lane] = (int8_t)fm_seq_euclid_rot_norm(nr);
            ctrl_seq_rotate_lane(&g_ctrl, lane, step1);
            ctrl_seq_lane_mark_edited(lane);
        } else {
            uint8_t old = g_ctrl.seq_vary[lane];
            int32_t v = (int32_t)g_ctrl.seq_vary[lane] + g * 3;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            g_ctrl.seq_vary[lane] = (uint8_t)v;
            if (g_ctrl.seq_vary[lane] != old)
                ctrl_seq_lane_mark_edited(lane);
        }
        mark_dirty();
        return;
    }
    if (g_ctrl.view == CTRL_VIEW_REC) {
        if (col == 0)
            g_ctrl.motion_state = (uint8_t)((g_ctrl.motion_state + (uint32_t)g + 5u) % 5u);
        else if (col == 1)
            g_ctrl.cfg.motion_bars = (uint8_t)(((g_ctrl.cfg.motion_bars + (uint32_t)g) + 4u) % 4u);
        else if (col == 2) {
            int32_t v = (int32_t)g_ctrl.cfg.midi_ch + g;
            if (v < 1)
                v = 1;
            if (v > 16)
                v = 16;
            g_ctrl.cfg.midi_ch = (uint8_t)v;
        } else
            g_ctrl.cfg.local_pot = (uint8_t)((g_ctrl.cfg.local_pot + (uint32_t)g + 4u) % 4u);
        mark_dirty();
        return;
    }
    if (slot < CTRL_NPARAMS && ctrl_slot_active(&g_ctrl.cfg, slot)) {
        if (lfo_btn_held()) {
            int32_t v = (int32_t)g_ctrl.cfg.lfo_depth[slot] + g;
            g_ctrl.cfg.lfo_depth[slot] = (uint8_t)ctrl_clamp127(v);
            mark_dirty();
            return;
        }
        g_ctrl.cfg.lfo_target = (uint8_t)slot;
        {
            int32_t v = (int32_t)g_ctrl.cfg.base_val[slot] + g;
            ctrl_set_param(&g_ctrl, slot, (uint8_t)ctrl_clamp127(v));
        }
    }
}

static void draw_top_bar(void)
{
    char s[40];
    static const char *const POT[4] = {"CUE", "CC1", "CC1", "EXP"};

    cv_begin(240, 26, C_BLACK);
    cv_text(4, 2, &FONT_S, "KO CTRL", C_WHITE);
    cv_text(72, 2, &FONT_S, CTRL_PROFILE_NAMES[g_ctrl.profile], C_HI);
    str_copy(s, "CH ");
    u8_dec(s + 3, g_ctrl.cfg.midi_ch);
    cv_text(4, 14, &FONT_S, s, C_GRAY);
    u8_dec(s, g_ctrl.bpm_eff);
    str_cat(s, g_ctrl.ext_clock ? " EXT" : " BPM");
    cv_text(112, 14, &FONT_S, s, g_ctrl.playing ? C_WHITE : C_DIM);
    cv_text(196, 14, &FONT_S, POT[g_ctrl.cfg.local_pot % 4u], ctrl_local_on(&g_ctrl) ? C_ACC : C_DIM);
    if (g_ctrl.learn_active)
        cv_text(168, 2, &FONT_S, "EDIT", C_ACC);
    if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank <= CTRL_BANK_D) {
        uint8_t b = g_ctrl.bank;
        if (g_ctrl.cfg.bank_count[b] > 4u || g_ctrl.learn_active) {
            uint8_t pg = bank_page();
            cv_rect(222, 4, 5, 5, pg == 0u ? C_WHITE : C_DIM);
            cv_rect(230, 4, 5, 5, pg == 1u ? C_WHITE : C_DIM);
        }
    }
    if (g_ctrl.view == CTRL_VIEW_LFO) {
        uint8_t pg = bank_page();
        cv_rect(206, 4, 5, 5, pg == 0u ? C_WHITE : C_DIM);
        cv_rect(214, 4, 5, 5, pg == 1u ? C_WHITE : C_DIM);
        cv_rect(222, 4, 5, 5, pg == 2u ? C_WHITE : C_DIM);
    }
    if (g_ctrl.view == CTRL_VIEW_SEQ)
        fm_seq_draw_page_dots(214, 4, bank_page(), FM_SEQ_NPAGES);
    cv_line(0, 25, 239, 25, C_LINE);
    cv_blit(0, 0);
}

static void draw_bank_faders(void)
{
    uint32_t col, b = (uint32_t)g_ctrl.bank % 4u;
    uint32_t pg = bank_page() ? 4u : 0u;
    char s[16];

    for (col = 0; col < 4u; col++) {
        int32_t x = 8 + (int32_t)col * 58;
        uint32_t slot = b * 8u + pg + col;
        int sel = g_ctrl.learn_active && g_ctrl.learn_slot == slot;

        if (slot >= CTRL_NPARAMS || !ctrl_slot_active(&g_ctrl.cfg, slot)) {
            cv_rect(x, 24, 48, 108, C_BLACK);
            cv_line(x, 24, x + 47, 24, C_LINE);
            cv_line(x, 131, x + 47, 131, C_LINE);
            cv_line(x, 24, x, 131, C_LINE);
            cv_line(x + 47, 24, x + 47, 131, C_LINE);
            cv_text(x + 4, 70, &FONT_S, "+ ADD", C_DIM);
            continue;
        }
        {
            int32_t h = ((int32_t)g_ctrl.eff_val[slot] * 100) / 127;
            int32_t hb = ((int32_t)g_ctrl.cfg.base_val[slot] * 100) / 127;

            ctrl_slot_label(&g_ctrl.cfg, slot, s);
            cv_text(x, 2, &FONT_S, s, BANK_COL[g_ctrl.bank]);
            if (sel)
                cv_rect(x + (int32_t)(g_ctrl.edit_char_ix % 4u) * 8, 13, 7, 2, C_WHITE);
            str_copy(s, "CC ");
            u8_dec(s + 3, g_ctrl.cfg.cc_map[slot]);
            cv_text(x, 14, &FONT_S, s, C_DIM);
            cv_rect(x, 24, 48, 108, C_LINE);
            cv_rect(x + 22, 124 - hb, 4, hb, C_DIM);
            if (h > 0)
                cv_rect(x + 10, 124 - h, 28, h, C_HI);
            u8_dec(s, g_ctrl.eff_val[slot]);
            cv_text(x + 8, 128, &FONT_S, s, C_WHITE);
            if (sel) {
                cv_line(x, 23, x + 47, 23, C_WHITE);
                cv_line(x, 132, x + 47, 132, C_WHITE);
                cv_line(x, 23, x, 132, C_WHITE);
                cv_line(x + 47, 23, x + 47, 132, C_WHITE);
            }
        }
    }
}

static void draw_lfo_scope_line(uint32_t slot, int32_t x0, int32_t x1, int32_t mid, uint16_t col)
{
    uint32_t step_px = fm_lfo_ph_step_px();
    int32_t depth = ctrl_lfo_depth_eff(&g_ctrl.cfg, slot);
    int32_t py = -1;
    int32_t x;

    if (!depth)
        return;
    for (x = x0; x <= x1; x++) {
        uint64_t p64 = g_ctrl.lfo_ph64[slot];
        uint32_t back = (uint32_t)(x1 - x);
        uint32_t ph;
        uint32_t cycle;
        int32_t w, y;

        fm_lfo_ph64_rewind(&p64, step_px, back);
        ph = (uint32_t)p64;
        cycle = (uint32_t)(p64 >> 32);
        w = lfo_wave_sample(slot, p64, ph, cycle);
        y = mid - (w * depth * 48) / (127 * 64);
        if (y < 8)
            y = 8;
        if (y > 132)
            y = 132;
        if (py >= 0)
            cv_line(x - 1, py, x, y, col);
        py = y;
    }
}

static void draw_view_lfo(void)
{
    uint32_t b, col, i, drawn = 0;
    uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
    uint32_t tb = (t >> 3) & 3u;
    uint32_t tb_cnt = g_ctrl.cfg.bank_count[tb];
    uint32_t n_total = ctrl_active_count(&g_ctrl.cfg);
    uint8_t algo = g_ctrl.cfg.lfo_algo[t];
    uint8_t rate = ctrl_lfo_rate_eff(&g_ctrl.cfg, t, g_ctrl.bpm_eff);
    char s[32];
    char lbl[5];
    const char *nd;
    int32_t mid = 78;
    int32_t x = 8;
    int32_t x_end = 232;
    int32_t tick_w = 10;
    int32_t tick_y = 124;
    int32_t gap = 2;
    int32_t bank_gap = 6;

    if (tb_cnt < 1u)
        tb_cnt = 1u;
    if (tb_cnt > 8u)
        tb_cnt = 8u;

    cv_line(8, mid, 231, mid, C_LINE);
    for (i = 0; i < CTRL_NPARAMS; i++) {
        if (!ctrl_slot_active(&g_ctrl.cfg, i))
            continue;
        if (i == (uint32_t)t || !g_ctrl.cfg.lfo_depth[i])
            continue;
        if (drawn >= 3u)
            break;
        draw_lfo_trace_slot(i, C_DIM);
        drawn++;
    }
    if (ctrl_slot_active(&g_ctrl.cfg, t))
        draw_lfo_trace_slot(t, C_WHITE);

    str_copy(s, "~");
    ctrl_slot_label(&g_ctrl.cfg, t, s + 1);
    str_cat(s, " CC");
    u8_dec(s + ctrl_slen(s), g_ctrl.cfg.cc_map[t]);
    cv_text(8, 4, &FONT_S, s, C_GRAY);
    if (algo != FM_LFO_ALG_CLASSIC)
        fm_lfo_draw_algo_badge(136, 4, algo, C_ACC, C_WHITE, C_GRAY);
    nd = fm_lfo_note_div(rate, g_ctrl.bpm_eff);
    if (nd)
        str_copy(s, nd);
    else
        fmt_hz100(s, fm_lfo_hz100(rate));
    cv_text(176, 4, &FONT_S, s, C_AMB);

    if (n_total < 1u)
        n_total = 1u;
    {
        int32_t avail = (x_end - x) - 3 * 4;

        tick_w = avail / (int32_t)n_total;
        if (tick_w > 10)
            tick_w = 10;
        if (tick_w < 3)
            tick_w = 3;
    }
    gap = 1;
    bank_gap = 4;

    if (tb_cnt > 0u) {
        uint32_t per = tb_cnt > 4u ? 4u : tb_cnt;
        int32_t name_w = 224 / (int32_t)per;

        for (col = 0; col < tb_cnt; col++) {
            uint32_t slot = tb * 8u + col;
            int32_t row = (tb_cnt > 4u && col >= 4u) ? 1 : 0;
            int32_t cix = (int32_t)(tb_cnt > 4u ? (col % 4u) : col);
            int32_t nx = 8 + cix * name_w;

            ctrl_slot_label(&g_ctrl.cfg, slot, lbl);
            cv_text(nx, 96 + row * 12, &FONT_S, lbl, slot == (uint32_t)t ? C_WHITE : C_DIM);
        }
    }

    for (b = 0; b < 4u; b++) {
        uint32_t cnt = g_ctrl.cfg.bank_count[b];

        if (cnt < 1u)
            cnt = 1u;
        if (cnt > 8u)
            cnt = 8u;
        if (b > 0u)
            x += bank_gap;
        for (col = 0; col < cnt; col++) {
            uint32_t slot = b * 8u + col;
            uint16_t fill = C_LINE;
            int32_t bx = x;

            if (bx + tick_w > x_end)
                break;
            x += tick_w + gap;
            if (g_ctrl.cfg.lfo_depth[slot]) {
                if (g_ctrl.cfg.lfo_algo[slot] == FM_LFO_ALG_CLASSIC)
                    fill = C_HI;
                else
                    fill = C_ACC;
            }
            cv_rect(bx, tick_y, tick_w, tick_w, fill);
            if (slot == (uint32_t)t) {
                cv_rect(bx - 1, tick_y - 1, tick_w + 2, tick_w + 2, C_WHITE);
                cv_rect(bx, tick_y, tick_w, tick_w, fill);
            }
        }
    }
}

static void draw_view_arp(void)
{
    uint32_t k;
    char s[24];

    str_copy(s, NOTE_NAMES[g_ctrl.cfg.root % 12u]);
    str_cat(s, " ");
    str_cat(s, CTRL_SCALE_NAMES[g_ctrl.cfg.scale % CTRL_NSCALES]);
    cv_text(8, 4, &FONT_S, s, C_WHITE);
    str_copy(s, CTRL_CHORD_NAMES[g_ctrl.cfg.chord % CTRL_NCHORDS]);
    cv_text(8, 20, &FONT_S, s, C_ACC);
    for (k = 0; k < 12u; k++) {
        int32_t x = 12 + (int32_t)k * 18;
        int in = ctrl_note_in_scale(g_ctrl.cfg.root, g_ctrl.cfg.scale, (uint8_t)k);
        int is_root = (k % 12u) == (g_ctrl.cfg.root % 12u);
        cv_rect(x, 60, 14, 40, is_root ? C_WHITE : (in ? C_HI : C_LINE));
    }
}

static void draw_view_seq(void)
{
    static const char *const LBL[4] = {"1", "2", "3", "4"};
    uint16_t lcols[CTRL_NLANES];
    fm_seq_cell_t cells[CTRL_NLANES * FM_SEQ_PAGE_STEPS];
    uint8_t sel = g_ctrl.cfg.seq_lane % CTRL_NLANES;
    uint32_t lane, st;
    uint32_t base = ctrl_seq_view_base(&g_ctrl);
    uint8_t plock_st = g_ctrl.seq_plock_step >= base && g_ctrl.seq_plock_step < base + FM_SEQ_PAGE_STEPS
                       ? (uint8_t)(g_ctrl.seq_plock_step - base) : FM_SEQ_PLOCK_NONE;
    uint8_t play_st = g_ctrl.step >= base && g_ctrl.step < base + FM_SEQ_PAGE_STEPS
                      ? (uint8_t)(g_ctrl.step - base) : FM_SEQ_PLOCK_NONE;
    uint16_t empty_act = RGB(18, 22, 42);
    const uint16_t mute_col = RGB(22, 26, 44);
    const uint16_t mute_bg = RGB(8, 10, 18);
    static const uint16_t LANE_PLAY[CTRL_NLANES] = {
        RGB(120, 200, 255), RGB(90, 220, 200), RGB(200, 140, 255), RGB(255, 180, 120)};

    for (lane = 0; lane < CTRL_NLANES; lane++) {
        uint16_t lcol = C_GRAY;
        int lane_aud = ctrl_lane_audible(&g_ctrl, (uint8_t)lane) ? 1 : 0;

        if (ctrl_lane_is_solo(&g_ctrl, (uint8_t)lane))
            lcol = C_ACC;
        else if (!lane_aud)
            lcol = C_DIM;
        else if (lane == (uint32_t)sel)
            lcol = C_WHITE;
        else
            lcol = LANE_PLAY[lane];
        if (g_ctrl.playing) {
            uint32_t li;
            uint8_t tm = 0u;

            for (li = 0; li < CTRL_NLANES; li++) {
                if (g_ctrl.cfg.lane_mute[li])
                    tm |= (uint8_t)(1u << li);
            }
            if (((tm ^ g_ctrl.heard_mute_mask) >> lane) & 1u)
                lcol = ((fm1_ms / 80u) & 1u) ? C_WHITE : C_ACC;
        }
        lcols[lane] = lcol;
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            uint32_t ab = base + st;
            int base_on = fm_seq_bit(g_ctrl.cfg.seq_pat[lane], ab);
            int on = fm_seq_vary_hit(base_on, g_ctrl.seq_vary[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                     (uint8_t)lane, g_ctrl.rng);
            uint16_t col = 0;
            uint16_t bg_col = (lane == (uint32_t)sel) ? empty_act : C_LINE;
            fm_seq_cell_t *cell = &cells[lane * FM_SEQ_PAGE_STEPS + st];
            uint8_t prob = g_ctrl.cfg.seq_prob[lane][ab];
            int32_t tune;
            int32_t vel_i;
            uint8_t vel = 0;

            if (on && !fm_seq_trig_pass(g_ctrl.seq_lane_trig[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                        (uint8_t)lane))
                on = 0;
            if (!lane_aud && lane != (uint32_t)sel && (on || base_on)) {
                bg_col = mute_bg;
                col = mute_col;
                if (base_on || on)
                    vel = 40u;
            } else if (!on) {
                if (base_on) {
                    col = lane == (uint32_t)sel ? C_DIM : mute_col;
                    bg_col = lane == (uint32_t)sel ? empty_act : mute_bg;
                    vel = 40u;
                } else
                    col = 0;
            } else if (on && !base_on) {
                col = C_ACC;
            } else if (g_ctrl.seq_vary[lane] > 0u || g_ctrl.cfg.seq_note_ofs[lane][ab] != 0 ||
                     (prob > 0u && prob < FM_SEQ_NCONDS))
                col = C_ACC;
            else
                col = lane == (uint32_t)sel ? C_WHITE : LANE_PLAY[lane];
            if (on && vel == 0u) {
                vel_i = (int32_t)g_ctrl.cfg.seq_vel[lane][ab] +
                        (int32_t)fm_seq_rand_vel(g_ctrl.seq_lane_vel[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                                 (uint8_t)lane, g_ctrl.rng);
                if (vel_i < 1)
                    vel_i = 1;
                if (vel_i > 127)
                    vel_i = 127;
                vel = (uint8_t)vel_i;
            }
            tune = (int32_t)g_ctrl.cfg.seq_note_ofs[lane][ab] +
                   (int32_t)fm_seq_rand_pitch(g_ctrl.seq_lane_tune[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                              (uint8_t)lane, g_ctrl.rng);
            if (tune < -24)
                tune = -24;
            if (tune > 24)
                tune = 24;
            cell->bg_col = bg_col;
            cell->col = col;
            cell->vel = vel;
            cell->tune = (int8_t)tune;
            cell->micro = g_ctrl.seq_micro[lane][ab];
            cell->note_span = 0u;
            cell->note_tail = 0u;
            cell->cond = prob < FM_SEQ_NCONDS ? prob : 0u;
            cell->gate = fm_seq_rand_gate(g_ctrl.seq_lane_gate[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                          (uint8_t)lane, g_ctrl.rng);
        }
    }
    fm_seq_draw_grid(4u, sel, play_st, g_ctrl.playing, plock_st, 0, sel, C_WHITE, LBL, lcols, cells, 0, 0, 0);
    fm_seq_draw_step_dots(133, (uint8_t)(base / FM_SEQ_PAGE_STEPS), (uint8_t)(g_ctrl.step / FM_SEQ_PAGE_STEPS),
                          ctrl_seq_pages(&g_ctrl), g_ctrl.playing, C_ACC);
}

static void draw_view_rec(void)
{
    uint32_t st, slot = g_ctrl.motion_slot % CTRL_NPARAMS;

    for (st = 0; st < 32u; st++) {
        int32_t x = 8 + (int32_t)st * 7;
        uint8_t mv = g_ctrl.motion[slot][st];
        int32_t h = mv == 255u ? 2 : 4 + ((int32_t)mv * 80) / 127;
        cv_rect(x, 120 - h, 5, h, mv == 255u ? C_LINE : C_ACC);
    }
    cv_text(8, 4, &FONT_S, "MOTION", C_WHITE);
}

static void draw_view_home(void)
{
    uint32_t b, slot;
    uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
    char s[40];

    draw_lfo_scope_line(t, 8, 231, 28, C_HI);
    str_copy(s, CTRL_CHORD_NAMES[g_ctrl.cfg.chord % CTRL_NCHORDS]);
    str_cat(s, " ");
    str_cat(s, CTRL_SCALE_NAMES[g_ctrl.cfg.scale % CTRL_NSCALES]);
    str_cat(s, " ");
    str_cat(s, CTRL_PROFILE_NAMES[g_ctrl.profile]);
    cv_text(8, 4, &FONT_S, s, C_ACC);

    for (b = 0; b < 16u; b++) {
        int32_t x = 8 + (int32_t)(b % 4u) * 14 + (int32_t)(b / 4u) * 60;
        int32_t y0 = 62 + (int32_t)(b / 4u) * 18;
        slot = b;
        {
            int32_t hb = ((int32_t)g_ctrl.cfg.base_val[slot] * 16) / 127;
            int32_t he = ((int32_t)g_ctrl.eff_val[slot] * 16) / 127;
            uint16_t col = BANK_COL[slot / 4u];

            cv_rect(x, y0, 10, 18, C_LINE);
            cv_rect(x + 4, y0 + 18 - hb, 2, hb, C_WHITE);
            if (he > 0)
                cv_rect(x + 1, y0 + 18 - he, 8, he, col);
            if (g_ctrl.cfg.lfo_depth[slot])
                cv_rect(x, y0, 10, 2, C_ACC);
        }
    }
}

static void draw_center(void)
{
    cv_begin(240, 140, C_BLACK);
    if (g_ctrl.view == CTRL_VIEW_BANK)
        draw_bank_faders();
    else if (g_ctrl.view == CTRL_VIEW_LFO)
        draw_view_lfo();
    else if (g_ctrl.view == CTRL_VIEW_ARP)
        draw_view_arp();
    else if (g_ctrl.view == CTRL_VIEW_SEQ)
        draw_view_seq();
    else if (g_ctrl.view == CTRL_VIEW_REC)
        draw_view_rec();
    else
        draw_view_home();
    cv_blit(0, 26);
}

static uint8_t knob_seq_bar_px(uint8_t col)
{
    uint8_t lane = g_ctrl.cfg.seq_lane % CTRL_NLANES;
    uint32_t v = 0u;
    uint32_t mx = 127u;

    if (g_ctrl.seq_plock_step < CTRL_NSTEPS) {
        uint8_t k = g_ctrl.seq_plock_step;
        uint8_t cond;

        if (col == 0) {
            v = g_ctrl.cfg.seq_vel[lane][k];
            mx = 127u;
        } else if (col == 1) {
            cond = g_ctrl.cfg.seq_prob[lane][k];
            return fm_seq_cond_bar_px(cond);
        } else if (col == 2) {
            v = (uint32_t)((int32_t)g_ctrl.seq_micro[lane][k] + 25);
            mx = 50u;
        } else {
            v = (uint32_t)((int32_t)g_ctrl.cfg.seq_note_ofs[lane][k] + 24);
            mx = 48u;
        }
    } else if (bank_page() == 2u) {
        if (col == 0) {
            v = lane;
            mx = 3u;
        } else if (col == 1) {
            uint8_t pos = ctrl_lane_p3_state_pos(lane);
            v = pos == 0u ? 0u : pos == 1u ? 64u : 127u;
            mx = 127u;
        } else if (col == 2) {
            uint8_t det = ctrl_detect_ensemble();
            if (det == FM_SEQ_ENS_CUSTOM) {
                uint32_t i;
                uint8_t act = 0u;
                for (i = 0; i < CTRL_NLANES; i++) {
                    if (!g_ctrl.cfg.lane_mute[i])
                        act++;
                }
                v = fm_seq_ens_bar_custom(act, CTRL_NLANES);
            } else
                v = fm_seq_ens_bar_preset(det);
            mx = 127u;
        } else {
            v = g_ctrl.seq_flip_state ? 127u : 0u;
            mx = 127u;
        }
    } else if (bank_page() == 1u) {
        uint8_t fn = fm_seq_p1_fn((uint8_t)col);

        if (fn == 3u) {
            int32_t bt = g_ctrl.seq_lane_tune[lane];
            v = (uint32_t)(bt < 0 ? -bt : bt);
            mx = 24u;
        } else if (fn == 1u) {
            v = fm_seq_trig_pct(g_ctrl.seq_lane_trig[lane]);
            mx = 100u;
        } else {
            int32_t bv = fn == 0u ? g_ctrl.seq_lane_vel[lane] : g_ctrl.seq_lane_gate[lane];
            v = (uint32_t)(bv < 0 ? -bv : bv);
            mx = 50u;
        }
    } else {
        if (col == 0) {
            v = lane;
            mx = 3u;
        } else if (col == 1) {
            v = fm_seq_euclid_bar_hits(g_ctrl.cfg.euclid_hits[lane]);
            mx = 16u;
        } else if (col == 2) {
            v = fm_seq_euclid_rot_norm(g_ctrl.cfg.euclid_rot[lane]);
            mx = 15u;
        } else {
            v = g_ctrl.seq_vary[lane];
            mx = 127u;
        }
    }
    if (v > mx)
        v = mx;
    return mx ? (uint8_t)((v * 127u) / mx) : 0u;
}

static void draw_knob_bar(void)
{
    uint32_t col;
    char s[20];

    cv_begin(240, 72, C_BLACK);
    cv_line(0, 0, 239, 0, C_LINE);
    for (col = 0; col < 4u; col++) {
        int32_t x = (int32_t)col * 60;
        int32_t h = 0;
        uint32_t slot = knob_slot((uint8_t)col);

        if (g_ctrl.view == CTRL_VIEW_SEQ) {
            h = ((int32_t)knob_seq_bar_px((uint8_t)col) * 24) / 127;
        } else if (g_ctrl.view == CTRL_VIEW_HOME) {
            h = ((int32_t)g_ctrl.cfg.home_macro[col] * 24) / 127;
        } else if (g_ctrl.view == CTRL_VIEW_LFO) {
            uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
            uint8_t pg = bank_page();
            {
                uint32_t ac = ctrl_active_count(&g_ctrl.cfg);
                uint32_t aix = ctrl_lfo_target_active_ix();
                uint8_t targ_pos = ac > 1u ? (uint8_t)((aix * 127u) / (ac - 1u)) : 0u;
                uint8_t v = pg == 0u ? (col == 0 ? targ_pos :
                                    col == 1 ? ctrl_lfo_rate_eff(&g_ctrl.cfg, t, g_ctrl.bpm_eff) :
                                    col == 2 ? g_ctrl.cfg.lfo_depth[t] : g_ctrl.cfg.lfo_shape[t])
                                 : pg == 1u ? (col == 0 ? g_ctrl.cfg.lfo_phase[t] :
                                               col == 1 ? g_ctrl.cfg.lfo_skew[t] :
                                               col == 2 ? g_ctrl.cfg.lfo_smooth[t] : g_ctrl.cfg.lfo_fold[t])
                                           : (col == 0 ? g_ctrl.cfg.lfo_algo[t] :
                                              col == 1 ? g_ctrl.cfg.lfo_ratio[t] :
                                              col == 2 ? g_ctrl.cfg.lfo_index[t] : g_ctrl.cfg.lfo_fb[t]);
            h = ((int32_t)v * 24) / 127;
            }
        } else if (slot < CTRL_NPARAMS && ctrl_slot_active(&g_ctrl.cfg, slot)) {
            h = ((int32_t)g_ctrl.eff_val[slot] * 24) / 127;
        }
        knob_label((uint8_t)col, s);
        cv_text(x + 4, 4, &FONT_S, s, BANK_COL[g_ctrl.bank % CTRL_BANK_COUNT]);
        if (g_ctrl.view == CTRL_VIEW_LFO && bank_page() == 0u && col == 3u) {
            uint8_t lt = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
            fm_lfo_shape_draw(x, 20, g_ctrl.cfg.lfo_shape[lt], C_WHITE, C_DIM);
        } else {
            knob_value((uint8_t)col, s);
            cv_text(x + 4, 20, &FONT_S, s, C_WHITE);
        }
        cv_rect(x + 8, 44, 44, 24, C_LINE);
        if (h > 0)
            cv_rect(x + 8, 68 - h, 44, h, C_HI);
    }
    cv_blit(0, 168);
}

static void draw_screen(void)
{
    palette_set(2u);
    draw_top_bar();
    draw_center();
    draw_knob_bar();
}

static void update_leds(uint32_t keys)
{
    uint32_t k;
    int32_t base_f3 = 53 + g_ctrl.oct_off * 12;

    for (k = 0; k < 27u; k++)
        fm1_led_key(KEY_LED(k), 0);

    if (g_ctrl.view == CTRL_VIEW_LFO) {
        uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
        uint32_t ac = ctrl_active_count(&g_ctrl.cfg);

        for (k = 0; k < 16u; k++) {
            uint32_t kid = KEY_WHITE[k];
            uint32_t idx = g_ctrl.lfo_key_page * 16u + k;
            int on = 0;

            if (idx < ac) {
                uint8_t slot = (uint8_t)ctrl_active_nth(&g_ctrl.cfg, idx);

                on = (slot == t) || g_ctrl.cfg.lfo_depth[slot] > 0u;
            }
            fm1_led_key(KEY_LED(kid), on);
        }
        for (k = 0; k < 9u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), g_ctrl.cfg.lfo_algo[t] == (uint8_t)k);
    } else if (g_ctrl.view == CTRL_VIEW_SEQ) {
        uint8_t lane = g_ctrl.cfg.seq_lane % CTRL_NLANES;
        uint32_t base = ctrl_seq_view_base(&g_ctrl);
        for (k = 0; k < 16u; k++) {
            uint32_t kid = KEY_WHITE[k];
            uint32_t ab = base + k;
            int base_on = fm_seq_bit(g_ctrl.cfg.seq_pat[lane], ab);
            int on = fm_seq_vary_hit(base_on, g_ctrl.seq_vary[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                     (uint8_t)lane, g_ctrl.rng);
            if (on && !fm_seq_trig_pass(g_ctrl.seq_lane_trig[lane], (uint8_t)ab, (uint8_t)g_ctrl.bar,
                                        (uint8_t)lane))
                on = 0;
            if (g_ctrl.seq_plock_step == ab)
                on = 1;
            if (g_ctrl.playing && g_ctrl.step == ab)
                on = !on;
            fm1_led_key(KEY_LED(kid), on);
        }
        for (k = 0; k < 4u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), k == lane);
        for (k = 0; k < 4u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[4u + k]), !g_ctrl.cfg.lane_mute[k]);
    } else {
        for (k = 0; k < 27u; k++) {
            uint8_t raw_note = (uint8_t)ctrl_clamp127(base_f3 + (int32_t)k);
            int is_root = (raw_note % 12u) == (g_ctrl.cfg.root % 12u);
            int held = (keys >> k) & 1u;
            fm1_led_key(KEY_LED(k), is_root || held);
        }
    }

    fm1_led_key(BTN_OCT_DOWN, g_ctrl.oct_off < 0);
    fm1_led_key(BTN_OCT_UP, g_ctrl.oct_off > 0);
    fm1_led_key(BTN_ENV, g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_A);
    fm1_led_key(BTN_SEL, g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_B);
    fm1_led_key(BTN_GLO, g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_C);
    fm1_led_key(BTN_FX, g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_D);
    fm1_led_key(BTN_LFO, g_ctrl.view == CTRL_VIEW_LFO);
    fm1_led_key(BTN_ARP, g_ctrl.view == CTRL_VIEW_ARP);
    fm1_led_key(BTN_SEQ, g_ctrl.view == CTRL_VIEW_SEQ);
    fm1_led_key(BTN_REC, g_ctrl.view == CTRL_VIEW_REC);
    fm1_led_key(BTN_HOME, g_ctrl.view == CTRL_VIEW_HOME);
    fm1_led_key(BTN_EDIT, g_ctrl.learn_active || g_ctrl.edit_down);
    fm1_led_key(BTN_PLAY, g_ctrl.playing);
}

static void handle_keys(uint32_t keys, uint32_t rising, uint32_t falling, uint8_t learn)
{
    uint32_t k;

    if (g_ctrl.view == CTRL_VIEW_LFO) {
        uint32_t ac = ctrl_active_count(&g_ctrl.cfg);

        for (k = 0; k < 16u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];
            uint32_t idx = g_ctrl.lfo_key_page * 16u + k;

            if (rising & bit && idx < ac) {
                g_ctrl.cfg.lfo_target = (uint8_t)ctrl_active_nth(&g_ctrl.cfg, idx);
                mark_dirty();
            }
        }
        for (k = 0; k < 9u; k++) {
            uint32_t bit = 1u << KEY_BLACK[k];
            if (rising & bit) {
                uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
                if (k <= 8u)
                    g_ctrl.cfg.lfo_algo[t] = (uint8_t)k;
                lfo_depth_wake(t);
                mark_dirty();
            }
        }
    } else if (g_ctrl.view == CTRL_VIEW_SEQ) {
        uint8_t lane = g_ctrl.cfg.seq_lane % CTRL_NLANES;
        uint32_t base = ctrl_seq_view_base(&g_ctrl);
        for (k = 0; k < 16u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];
            uint32_t ab = base + k;
            if (rising & bit) {
                g_ctrl.seq_plock_step = (uint8_t)ab;
                g_ctrl.seq_plock_knob = 0u;
                g_ctrl.ui_dirty = 1u;
            }
            if (falling & bit) {
                if (g_ctrl.seq_plock_step == ab && !g_ctrl.seq_plock_knob) {
                    int was = fm_seq_bit(g_ctrl.cfg.seq_pat[lane], ab);
                    if (g_ctrl.cfg.euclid_hits[lane] == FM_SEQ_EUCLID_EMPTY && !was)
                        fm_seq_bits_fill(g_ctrl.cfg.seq_pat[lane], 0);
                    fm_seq_bit_put(g_ctrl.cfg.seq_pat[lane], ab, !was);
                    ctrl_seq_lane_mark_edited(lane);
                }
                if (g_ctrl.seq_plock_step < CTRL_NSTEPS && (g_ctrl.seq_plock_step & 15u) == k)
                    g_ctrl.seq_plock_step = 255u;
                mark_dirty();
            }
        }
        for (k = 0; k < 4u; k++) {
            if (rising & (1u << KEY_BLACK[k])) {
                if (g_ctrl.edit_down)
                    ctrl_lane_toggle_mute(&g_ctrl, (uint8_t)k);
                else if (fm1_ms - g_ctrl.bk_last_ms[k] < 300u)
                    ctrl_lane_toggle_solo(&g_ctrl, (uint8_t)k);
                else if (k != lane)
                    g_ctrl.cfg.seq_lane = (uint8_t)k;
                g_ctrl.bk_last_ms[k] = fm1_ms;
                mark_dirty();
            }
        }
        if (rising & (1u << KEY_BLACK[8])) {
            if (g_ctrl.edit_down)
                ctrl_invert_all_mutes();
            else if (ctrl_any_lane_mute_or_solo(&g_ctrl))
                ctrl_unmute_all_lanes(&g_ctrl);
            else
                ctrl_lane_toggle_mute(&g_ctrl, lane);
            mark_dirty();
        }
        if (rising & (1u << KEY_BLACK[9])) {
            if (g_ctrl.edit_down) {
                uint32_t li;
                for (li = 0; li < CTRL_NLANES; li++)
                    ctrl_seq_reset_lane_song(&g_ctrl, (uint8_t)li);
            } else
                ctrl_seq_reset_lane_song(&g_ctrl, lane);
            mark_dirty();
        }
    } else if (g_ctrl.view == CTRL_VIEW_BANK && learn) {
        for (k = 0; k < 4u; k++) {
            if (rising & (1u << KEY_WHITE[k]))
                learn_pick_col((uint8_t)k);
            if (rising & (1u << KEY_BLACK[k]))
                learn_pick_col((uint8_t)k);
        }
    } else {
        for (k = 0; k < 27u; k++) {
            if (rising & (1u << k)) {
                ctrl_note_on(&g_ctrl, (uint8_t)k, 100);
                g_ctrl.ui_dirty = 1u;
            }
            if (falling & (1u << k)) {
                ctrl_note_off_key(&g_ctrl, (uint8_t)k);
                g_ctrl.ui_dirty = 1u;
            }
        }
    }
    (void)keys;
}

/* SEQ view: OCT -/+ turn the page; past the first or last page the pattern doubles (ctrl_seq_view_nav).
 * EDIT + OCT -/+ is the step pace. */
static void seq_oct(int32_t dir)
{
    char msg[8];
    uint8_t nav;

    if ((fm1_in.buttons >> BTN_EDIT) & 1u) {
        g_ctrl.cfg.seq_pace = (uint8_t)((g_ctrl.cfg.seq_pace + (dir > 0 ? 1u : 4u)) % FM_SEQ_PACE_N);
        edit_mark();
        return;
    }
    nav = ctrl_seq_view_nav(&g_ctrl, dir);
    fm_seq_fmt_nav(msg, nav, g_ctrl.seq_view_page, ctrl_seq_pages(&g_ctrl));
    seq_flash_knob(msg);
    if (nav == FM_SEQ_NAV_APPEND || nav == FM_SEQ_NAV_PREPEND)
        mark_dirty();
    g_ctrl.ui_dirty = 1u;
}

static void app_audio(int32_t *out, uint32_t n)
{
    ctrl_cue_render(&g_ctrl, out, n);
}

typedef struct { /* "CTR5": ctrl_cfg_t with 16 steps */
    uint32_t magic;
    uint8_t profile;
    uint8_t midi_ch;
    uint8_t local_pot;
    uint8_t root;
    uint8_t scale;
    uint8_t chord;
    uint8_t arp_mode;
    uint8_t arp_rate;
    uint8_t vel_mode;
    uint8_t bpm;
    uint8_t swing;
    uint8_t motion_bars;
    uint8_t lfo_target;
    uint8_t seq_lane;
    uint8_t pad[2];
    uint8_t cc_map[CTRL_NPARAMS];
    uint8_t base_val[CTRL_NPARAMS];
    uint8_t lfo_depth[CTRL_NPARAMS];
    uint8_t lfo_rate[CTRL_NPARAMS];
    uint8_t lfo_shape[CTRL_NPARAMS];
    uint8_t lfo_phase[CTRL_NPARAMS];
    uint8_t lfo_smooth[CTRL_NPARAMS];
    uint8_t lfo_skew[CTRL_NPARAMS];
    uint8_t lfo_algo[CTRL_NPARAMS];
    uint8_t lfo_ratio[CTRL_NPARAMS];
    uint8_t lfo_index[CTRL_NPARAMS];
    uint8_t lfo_fb[CTRL_NPARAMS];
    uint8_t lfo_fold[CTRL_NPARAMS];
    uint8_t arp_span;
    uint8_t arp_gate;
    int8_t seq_note_ofs[CTRL_NLANES][CTRL_NSTEPS_V5];
    uint8_t seq_vel[CTRL_NLANES][CTRL_NSTEPS_V5];
    uint8_t seq_prob[CTRL_NLANES][CTRL_NSTEPS_V5];
    uint8_t seq_pace;
    uint8_t home_macro[4];
    uint16_t seq_pat[CTRL_NLANES];
    uint8_t lane_mute[CTRL_NLANES];
    uint8_t euclid_hits[CTRL_NLANES];
    int8_t euclid_rot[CTRL_NLANES];
    uint8_t bank_page[CTRL_BANK_COUNT];
    uint8_t bank_count[4];
    char custom_label[CTRL_NPARAMS][4];
} ctrl_cfg_v5_t;

_Static_assert(offsetof(ctrl_cfg_v5_t, seq_note_ofs) == offsetof(ctrl_cfg_t, seq_note_ofs), "CTR5 head");

/* CTR5 -> CTR6: the 16 steps become page 1 of a one-page pattern, pages 2..8 empty */
static int ctrl_cfg_load_v5(ctrl_cfg_t *c)
{
    ctrl_cfg_v5_t o;
    uint32_t i, s;

    if (settings_load(&o, sizeof o) != 0 || o.magic != CTRL_CFG_MAGIC_V5)
        return -1;
    ctrl_cfg_defaults(c);
    memcpy(c, &o, offsetof(ctrl_cfg_v5_t, seq_note_ofs));
    c->magic = CTRL_CFG_MAGIC;
    c->seq_pages = 1u;
    c->pad = 0u;
    for (i = 0; i < CTRL_NLANES; i++) {
        for (s = 0; s < CTRL_NSTEPS_V5; s++) {
            c->seq_note_ofs[i][s] = o.seq_note_ofs[i][s];
            c->seq_vel[i][s] = o.seq_vel[i][s];
            c->seq_prob[i][s] = o.seq_prob[i][s];
        }
        fm_seq_bits_fill(c->seq_pat[i], 0);
        fm_seq_bits_set_page16(c->seq_pat[i], 0u, o.seq_pat[i]);
    }
    c->seq_pace = o.seq_pace;
    memcpy(c->home_macro, o.home_macro, sizeof c->home_macro);
    memcpy(c->lane_mute, o.lane_mute, sizeof c->lane_mute);
    memcpy(c->euclid_hits, o.euclid_hits, sizeof c->euclid_hits);
    memcpy(c->euclid_rot, o.euclid_rot, sizeof c->euclid_rot);
    memcpy(c->bank_page, o.bank_page, sizeof c->bank_page);
    memcpy(c->bank_count, o.bank_count, sizeof c->bank_count);
    memcpy(c->custom_label, o.custom_label, sizeof c->custom_label);
    return 0;
}

static void app_init(void)
{
    palette_set(2u);
    ctrl_init_runtime(&g_ctrl);
    if ((settings_load(&g_ctrl.cfg, sizeof g_ctrl.cfg) == 0 && g_ctrl.cfg.magic == CTRL_CFG_MAGIC) ||
        ctrl_cfg_load_v5(&g_ctrl.cfg) == 0) {
        ctrl_profile_sync(&g_ctrl);
        uint32_t i;
        for (i = 0; i < CTRL_NPARAMS; i++)
            g_ctrl.eff_val[i] = g_ctrl.cfg.base_val[i];
    } else
        ctrl_cfg_defaults(&g_ctrl.cfg);
    ctrl_profile_sync(&g_ctrl);
    g_ctrl.seq_ens_sel = FM_SEQ_ENS_ALL;
    g_ctrl.ui_dirty = 1u;
    draw_screen();
}

static void app_step(void)
{
    uint32_t b, btn_rel = 0, k, rising, falling, keys = fm1_in.notes;

    platform_poll();
    b = fm1_input_edges(&btn_rel);
    rising = (keys & ~g_ctrl.prev_keys) | fm1_input_note_edges();
    falling = g_ctrl.prev_keys & ~keys;
    g_ctrl.prev_keys = keys;

    if (b & (1u << BTN_ENV)) {
        if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_A
            && (g_ctrl.cfg.bank_count[CTRL_BANK_A] > 4u || g_ctrl.learn_active))
            g_ctrl.cfg.bank_page[CTRL_BANK_A] ^= 1u;
        else
            nav_bank(CTRL_BANK_A);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_SEL)) {
        if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_B
            && (g_ctrl.cfg.bank_count[CTRL_BANK_B] > 4u || g_ctrl.learn_active))
            g_ctrl.cfg.bank_page[CTRL_BANK_B] ^= 1u;
        else
            nav_bank(CTRL_BANK_B);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_GLO)) {
        if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_C
            && (g_ctrl.cfg.bank_count[CTRL_BANK_C] > 4u || g_ctrl.learn_active))
            g_ctrl.cfg.bank_page[CTRL_BANK_C] ^= 1u;
        else
            nav_bank(CTRL_BANK_C);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_FX)) {
        if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.bank == CTRL_BANK_D
            && (g_ctrl.cfg.bank_count[CTRL_BANK_D] > 4u || g_ctrl.learn_active))
            g_ctrl.cfg.bank_page[CTRL_BANK_D] ^= 1u;
        else
            nav_bank(CTRL_BANK_D);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_LFO)) {
        if (g_ctrl.view == CTRL_VIEW_LFO)
            g_ctrl.cfg.bank_page[CTRL_BANK_LFO] =
                (uint8_t)((g_ctrl.cfg.bank_page[CTRL_BANK_LFO] + 1u) % 3u);
        else
            nav_bank(CTRL_BANK_LFO);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_ARP)) {
        if (g_ctrl.view == CTRL_VIEW_ARP)
            g_ctrl.cfg.bank_page[CTRL_BANK_ARP] ^= 1u;
        else
            nav_bank(CTRL_BANK_ARP);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_SEQ)) {
        if (g_ctrl.view == CTRL_VIEW_SEQ)
            g_ctrl.cfg.bank_page[CTRL_BANK_SEQ] =
                (uint8_t)((g_ctrl.cfg.bank_page[CTRL_BANK_SEQ] + 1u) % FM_SEQ_NPAGES);
        else
            nav_bank(CTRL_BANK_SEQ);
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_REC))
        nav_bank(CTRL_BANK_REC);
    if (b & (1u << BTN_HOME))
        nav_bank(CTRL_BANK_HOME);
    if (b & (1u << BTN_OCT_DOWN)) {
        if (g_ctrl.view == CTRL_VIEW_LFO && ctrl_active_count(&g_ctrl.cfg) > 16u) {
            g_ctrl.lfo_key_page = 0u;
            g_ctrl.ui_dirty = 1u;
        } else if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.learn_active) {
            if (g_ctrl.edit_char_ix > 0u)
                g_ctrl.edit_char_ix--;
            g_ctrl.ui_dirty = 1u;
        } else if (g_ctrl.view == CTRL_VIEW_ARP) {
            g_ctrl.cfg.root = (uint8_t)((g_ctrl.cfg.root + 11u) % 12u);
            mark_dirty();
        } else if (g_ctrl.view == CTRL_VIEW_SEQ) {
            seq_oct(-1);
        } else if (g_ctrl.oct_off > -2) {
            g_ctrl.oct_off--;
            g_ctrl.ui_dirty = 1u;
        }
    }
    if (b & (1u << BTN_OCT_UP)) {
        if (g_ctrl.view == CTRL_VIEW_LFO && ctrl_active_count(&g_ctrl.cfg) > 16u) {
            g_ctrl.lfo_key_page = 1u;
            g_ctrl.ui_dirty = 1u;
        } else if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.learn_active) {
            if (g_ctrl.edit_char_ix < 3u)
                g_ctrl.edit_char_ix++;
            g_ctrl.ui_dirty = 1u;
        } else if (g_ctrl.view == CTRL_VIEW_ARP) {
            g_ctrl.cfg.root = (uint8_t)((g_ctrl.cfg.root + 1u) % 12u);
            mark_dirty();
        } else if (g_ctrl.view == CTRL_VIEW_SEQ) {
            seq_oct(1);
        } else if (g_ctrl.oct_off < 2) {
            g_ctrl.oct_off++;
            g_ctrl.ui_dirty = 1u;
        }
    }
    if (b & (1u << BTN_EDIT))
        g_ctrl.edit_used = 0u;
    g_ctrl.edit_down = (uint8_t)((fm1_in.buttons >> BTN_EDIT) & 1u);
    if (btn_rel & (1u << BTN_EDIT)) {
        if (!g_ctrl.edit_used && g_ctrl.view == CTRL_VIEW_BANK) {
            g_ctrl.learn_active = g_ctrl.learn_active ? 0u : 1u;
            if (g_ctrl.learn_active) {
                g_ctrl.learn_slot = (uint8_t)knob_slot(0);
                if (g_ctrl.learn_slot >= CTRL_NPARAMS)
                    g_ctrl.learn_slot = (uint8_t)(g_ctrl.bank * 8u);
                g_ctrl.edit_char_ix = 0u;
            }
        }
        g_ctrl.ui_dirty = 1u;
    }
    if (g_ctrl.edit_down) {
        if (b & (1u << BTN_LFO)) {
            reset_section_lfo();
            edit_mark();
        }
        if (b & (1u << BTN_ARP)) {
            reset_section_arp();
            edit_mark();
        }
        if (b & (1u << BTN_SEQ)) {
            reset_section_seq();
            edit_mark();
        }
        if (b & (1u << BTN_REC)) {
            reset_section_rec();
            edit_mark();
        }
        if (b & (1u << BTN_ENV)) {
            reset_bank(CTRL_BANK_A);
            edit_mark();
        }
        if (b & (1u << BTN_SEL)) {
            reset_bank(CTRL_BANK_B);
            edit_mark();
        }
        if (b & (1u << BTN_GLO)) {
            reset_bank(CTRL_BANK_C);
            edit_mark();
        }
        if (b & (1u << BTN_FX)) {
            reset_bank(CTRL_BANK_D);
            edit_mark();
        }
        if (b & (1u << BTN_HOME)) {
            reset_section_home();
            edit_mark();
        }
    }
    if (b & (1u << BTN_PLAY)) {
        g_ctrl.playing = g_ctrl.playing ? 0u : 1u;
        if (g_ctrl.playing) {
            ctrl_transport_reset(&g_ctrl);
            midi_send(0xFA, 0, 0);
        } else {
            ctrl_seq_notes_off(&g_ctrl);
            ctrl_arp_notes_off(&g_ctrl);
            synth_all_notes_off();
            midi_send(0xFC, 0, 0);
        }
        g_ctrl.ui_dirty = 1u;
    }
    if (b & (1u << BTN_SAVE)) {
        synth_all_notes_off();
        settings_save(&g_ctrl.cfg, sizeof g_ctrl.cfg);
        g_ctrl.set_dirty = 0u;
        g_ctrl.saved_until = fm1_ms + 1200u;
        g_ctrl.ui_dirty = 1u;
    }

    {
        int32_t as = fm1_enc_take(ENC_ALGORITHM);
        if (as) {
            if (g_ctrl.view == CTRL_VIEW_BANK && g_ctrl.learn_active) {
                int32_t v = (int32_t)g_ctrl.edit_char_ix + as;
                if (v < 0)
                    v = 0;
                if (v > 3)
                    v = 3;
                g_ctrl.edit_char_ix = (uint8_t)v;
                g_ctrl.ui_dirty = 1u;
            } else {
                uint8_t t = g_ctrl.cfg.lfo_target % CTRL_NPARAMS;
                g_ctrl.cfg.lfo_shape[t] = fm_lfo_step_shape(g_ctrl.cfg.lfo_shape[t], as, 1u);
                lfo_depth_wake(t);
                mark_dirty();
            }
        }
    }
    {
        int32_t ps = fm1_enc_take(ENC_PRESETS);
        if (ps) {
            if (g_ctrl.edit_down) {
                edit_mark();
                if (ps < 0)
                    g_ctrl.cfg.profile = 0u;
                else
                    g_ctrl.cfg.profile = (uint8_t)(CTRL_NPROFILES - 1u);
                ctrl_apply_profile(&g_ctrl.cfg, g_ctrl.cfg.profile);
                ctrl_profile_sync(&g_ctrl);
                mark_dirty();
            } else if (!g_ctrl.learn_active) {
                int32_t p = (int32_t)g_ctrl.cfg.profile + ps;
                while (p < 0)
                    p += (int32_t)CTRL_NPROFILES;
                g_ctrl.cfg.profile = (uint8_t)((uint32_t)p % CTRL_NPROFILES);
                ctrl_apply_profile(&g_ctrl.cfg, g_ctrl.cfg.profile);
                ctrl_profile_sync(&g_ctrl);
                mark_dirty();
            }
        }
    }
    {
        int32_t ss = fm1_enc_take(ENC_SELECT);
        if (ss) {
            if (g_ctrl.learn_active && g_ctrl.view == CTRL_VIEW_BANK) {
                uint8_t b = g_ctrl.bank % 4u;
                uint32_t base = (uint32_t)b * 8u;
                uint8_t cnt = g_ctrl.cfg.bank_count[b];
                int32_t rel;

                if (cnt < 1u)
                    cnt = 1u;
                if (cnt > 8u)
                    cnt = 8u;
                rel = (int32_t)(g_ctrl.learn_slot - base) + ss;
                while (rel < 0)
                    rel += (int32_t)cnt;
                rel %= (int32_t)cnt;
                g_ctrl.learn_slot = (uint8_t)(base + (uint32_t)rel);
                g_ctrl.cfg.bank_page[b] = (uint8_t)((uint32_t)rel >= 4u ? 1u : 0u);
                g_ctrl.ui_dirty = 1u;
            } else if (g_ctrl.learn_active) {
                int32_t v = (int32_t)g_ctrl.learn_slot + ss;
                while (v < 0)
                    v += (int32_t)CTRL_NPARAMS;
                g_ctrl.learn_slot = (uint8_t)((uint32_t)v % CTRL_NPARAMS);
                g_ctrl.ui_dirty = 1u;
            } else {
                int32_t bnk = (int32_t)g_ctrl.bank + ss;
                while (bnk < 0)
                    bnk += (int32_t)CTRL_BANK_COUNT;
                nav_bank((uint8_t)((uint32_t)bnk % CTRL_BANK_COUNT));
            }
        }
    }
    for (k = 0; k < 4u; k++) {
        int32_t steps = fm1_enc_take(ENC_KNOB1 + k);
        if (steps)
            knob_adjust((uint8_t)k, steps);
    }

    if (g_ctrl.view == CTRL_VIEW_LFO) {
        ctrl_lfo_snap_target();
        if (ctrl_active_count(&g_ctrl.cfg) <= 16u)
            g_ctrl.lfo_key_page = 0u;
        else if (g_ctrl.lfo_key_page > 1u)
            g_ctrl.lfo_key_page = 1u;
    }

    handle_keys(keys, rising, falling, g_ctrl.learn_active);

    {
        uint8_t st, d1, d2;
        while (midi_get(&st, &d1, &d2))
            ctrl_handle_midi_rx(&g_ctrl, st, d1, d2, fm1_ms);
    }

    ctrl_engine_tick(&g_ctrl, fm1_ms);
    update_leds(keys);

    if (g_ctrl.ui_dirty || fm1_ms - g_ctrl.ui_ms >= 50u) {
        draw_screen();
        g_ctrl.ui_dirty = 0u;
        g_ctrl.ui_ms = fm1_ms;
    }
    if (g_ctrl.set_dirty && fm1_ms - g_ctrl.set_t > 2000u) {
        settings_save(&g_ctrl.cfg, sizeof g_ctrl.cfg);
        g_ctrl.set_dirty = 0u;
    }
}

static void app_main(void)
{
    app_init();
    for (;;)
        app_step();
}
