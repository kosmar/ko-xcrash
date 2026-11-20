/* SPDX-License-Identifier: GPL-3.0-only */
/* TRIPTYCH: three MIDI-first generative engines (CYCLIC, CONTURA, DIT DAH) for the M-VAVE FM-1. */
#define APP_NAME "TRIPTYCH"
#include "platform.c"

#define C_ACC RGB(180, 140, 255)
#define C_KEY RGB(210, 210, 210)

#include "midi_ctrl.h"
#include "tr_midi_out.h"
#include "synth_fm2.h"

#include "engine_triptych.h"
#include "engine_contura.h"
#include "engine_ditdah.h"

static midi_cfg_t g_cfg;
static midi_state_t g_st;
static triptych_state_t g_triptych;
static contura_state_t g_contura;
static ditdah_state_t g_ditdah;
static uint8_t g_mode_prev;
static uint32_t g_last_note = 255u;
static uint32_t g_ui_dirty = 1u;
static uint32_t g_set_dirty = 0u;
static uint32_t g_set_t = 0u;
static uint32_t g_ui_ms = 0u;
static uint32_t g_confirm_until = 0u;
static uint8_t g_confirm_kind = 0u; /* 0 = param bank, 1 = perf page, 2 = home all */
static uint8_t g_confirm_id = 0u;
static uint32_t g_saved_until = 0u;
static uint8_t g_home_view = 0u;
static uint32_t g_home_hud_until = 0u;
static uint8_t g_home_hud_kind = 0u; /* 1 = bank (ENC_SELECT), 2 = param knob/CC */
static uint8_t g_home_hud_col = 0u;  /* perf col 0..3, or param slot 0..19 on BANK page */
static uint8_t g_edit_combo_used = 0u;
static perf_state_t g_perf;
uint8_t g_mod_hold_slot = 255u;
uint32_t g_mod_hold_until = 0u;

static const struct {
    uint8_t btn;
    uint8_t bank;
} BANK_BTN[4] = {
    {BTN_ENV, BANK_SYNTH},
    {BTN_SEL, BANK_ENGINE},
    {BTN_GLO, BANK_TIME},
    {BTN_FX, BANK_SPACE},
};

static volatile uint32_t g_audio_frames;

#define KEY0 53u /* F3 */

static int key_is_black(uint32_t note) { return (0x54Au >> (note % 12u)) & 1u; }

static void app_audio(int32_t *out, uint32_t n)
{
    g_audio_frames += n;
    synth_render(out, n, &g_cfg);
}

static void hud_show(uint32_t param, uint32_t cc)
{
    g_st.hud_param = (uint8_t)(param < NVSLOTS ? param : param % NPARAMS);
    g_st.hud_cc = (uint8_t)(cc & 255u);
    g_st.hud_until_ms = fm1_ms + 1200u;
    g_ui_dirty = 1u;
}

static void mark_cfg_dirty(void)
{
    g_set_dirty = 1u;
    g_set_t = fm1_ms;
    g_ui_dirty = 1u;
}

static uint8_t engine_last_pitch(uint8_t fallback)
{
    return g_last_note < 128u ? (uint8_t)g_last_note : fallback;
}

static uint8_t tryp_keys_on_st(const midi_cfg_t *c)
{
    uint8_t ch = c->midi_ch ? (uint8_t)(c->midi_ch - 1u) : 0u;

    return (uint8_t)(0x90u | ch);
}

static uint8_t tryp_keys_off_st(const midi_cfg_t *c)
{
    uint8_t ch = c->midi_ch ? (uint8_t)(c->midi_ch - 1u) : 0u;

    return (uint8_t)(0x80u | ch);
}

/* Slots that midi_cfg_mode_preset rewrites. Kept per mode so a scale (etc.) survives a switch. */
static const uint8_t MODE_MEM_SLOTS[11] = {
    P_TIMBRE, P_RATIO, P_ATTACK, P_DECAY, P_PARAM_A, P_PARAM_B,
    P_DENSITY, P_VEL_MOD, P_TEMPO, P_SWING, P_GATE};
static uint8_t g_mode_mem[NMODES][11];
static uint8_t g_mode_mem_ok[NMODES];

static void mode_mem_save(uint8_t mode)
{
    uint32_t i;
    if (mode >= NMODES)
        return;
    for (i = 0; i < 11u; i++)
        g_mode_mem[mode][i] = g_cfg.base_val[MODE_MEM_SLOTS[i]];
    g_mode_mem_ok[mode] = 1u;
}

static void mode_mem_load(uint8_t mode)
{
    uint32_t i;
    for (i = 0; i < 11u; i++) {
        uint8_t v = g_mode_mem[mode][i];
        g_cfg.base_val[MODE_MEM_SLOTS[i]] = v;
        g_cfg.val[MODE_MEM_SLOTS[i]] = v;
    }
}

static void mode_mem_preset_all_modes(void)
{
    uint32_t m, i;

    for (m = 0; m < NMODES; m++) {
        midi_cfg_t tmp = g_cfg;

        midi_cfg_mode_preset(&tmp, (uint8_t)m);
        for (i = 0; i < 11u; i++)
            g_mode_mem[m][i] = tmp.base_val[MODE_MEM_SLOTS[i]];
        g_mode_mem_ok[m] = 1u;
    }
}

static void mode_mem_random_all_modes(void)
{
    uint8_t save_mode = g_cfg.mode;
    uint32_t m, i;

    for (m = 0; m < NMODES; m++) {
        if (!g_mode_mem_ok[m]) {
            midi_cfg_t seed = g_cfg;

            midi_cfg_mode_preset(&seed, (uint8_t)m);
            for (i = 0; i < 11u; i++)
                g_mode_mem[m][i] = seed.base_val[MODE_MEM_SLOTS[i]];
        }
        g_cfg.mode = (uint8_t)m;
        for (i = 0; i < 11u; i++) {
            uint32_t slot = MODE_MEM_SLOTS[i];
            uint8_t v;

            if (m == (uint32_t)MODE_DITDAH && slot == (uint32_t)P_PARAM_B)
                v = midi_ditdah_random_phrase_param(&g_st, g_mode_mem[m][i]);
            else
                v = midi_slot_random_u7(&g_cfg, &g_st, slot);
            g_mode_mem[m][i] = v;
        }
        g_mode_mem_ok[m] = 1u;
    }
    g_cfg.mode = save_mode;
}

static void engine_restart_for_mode(uint8_t mode)
{
    if (mode == MODE_CONTURA) {
        contura_init(&g_contura, &g_cfg);
        contura_set_tonic(&g_contura, engine_last_pitch(57));
    } else if (mode == MODE_TRIAXIS) {
        triptych_init(&g_triptych);
        triptych_set_root(&g_triptych, engine_last_pitch(45));
    } else {
        ditdah_init(&g_ditdah);
        ditdah_set_base(&g_ditdah, engine_last_pitch(57));
        ditdah_recompile(&g_ditdah, &g_cfg);
    }
}

static void engine_on_mode_change(uint8_t new_mode)
{
    synth_all_notes_off();
    trm_panic();
    if (g_mode_prev < NMODES && g_mode_prev != new_mode)
        mode_mem_save(g_mode_prev);
    if (new_mode < NMODES && g_mode_mem_ok[new_mode])
        mode_mem_load(new_mode);
    else
        midi_cfg_mode_preset(&g_cfg, new_mode);
    mode_mem_save(new_mode);
    engine_restart_for_mode(new_mode);
}

static void engine_mode_sync(void)
{
    if (g_cfg.mode == g_mode_prev)
        return;
    engine_on_mode_change(g_cfg.mode);
    g_mode_prev = g_cfg.mode;
}

static void note_name(uint32_t note, char *s, uint32_t n)
{
    static const char *const NM[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    uint32_t i = 0;
    if (note >= 128u) {
        s[0] = '-';
        s[1] = 0;
        return;
    }
    {
        const char *p = NM[note % 12u];
        while (*p && i + 1u < n)
            s[i++] = *p++;
    }
    if (note >= 12u && i + 2u < n) {
        uint32_t o = note / 12u - 1u;
        if (o < 10u)
            s[i++] = (char)('0' + o);
    }
    s[i] = 0;
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

static void u8_dec_pct(char *s, uint32_t v)
{
    u8_dec(s, v);
    uint32_t i = 0;
    while (s[i])
        i++;
    s[i] = '%';
    s[i + 1u] = 0;
}

static void i8_dec(char *s, int32_t v)
{
    uint32_t i = 0;

    if (v < 0) {
        s[i++] = '-';
        v = -v;
    }
    u8_dec(s + i, (uint32_t)v);
}

static void copy_str(char *dst, const char *src)
{
    uint32_t i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

static uint32_t home_hud_norm_label(const char *src, char *dst, uint32_t cap);

static void param_fmt_val(uint32_t slot, char *s, int compact)
{
    static const char *const DIVS[11] = {"1/1", "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T", "1/64"};
    uint32_t hold = midi_mod_hold_active(slot, fm1_ms);
    uint32_t v = hold ? g_cfg.base_val[slot % NPARAMS] : g_cfg.val[slot % NPARAMS];

    if (g_cfg.bank == BANK_LFO && g_st.lfo_page == 2u && g_perf.page == PERF_PAGE_BANK &&
        slot >= (uint32_t)P_LFO_TARGET && slot <= (uint32_t)P_LFO_SHAPE) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        uint32_t col = slot - (uint32_t)P_LFO_TARGET;
        uint8_t raw;

        if (col == 0u) {
            raw = g_st.lfo_algo[ts];
            if (raw >= FM_LFO_NALGOS)
                raw = 0;
            copy_str(s, FM_LFO_ALG_NAMES[raw]);
            (void)compact;
            return;
        }
        if (col == 1u) {
            fm_lfo_fmt_ratio(s, g_st.lfo_ratio[ts]);
            return;
        }
        if (col == 2u) {
            fm_lfo_fmt_depth(s, (int32_t)g_st.lfo_index[ts]);
            return;
        }
        fm_lfo_fmt_depth(s, (int32_t)g_st.lfo_fb[ts]);
        return;
    }
    if (g_cfg.bank == BANK_LFO && g_st.lfo_page == 1u && g_perf.page == PERF_PAGE_BANK &&
        slot >= (uint32_t)P_LFO_TARGET && slot <= (uint32_t)P_LFO_SHAPE) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        uint32_t col = slot - (uint32_t)P_LFO_TARGET;

        if (col == 0u) {
            u8_dec(s, g_st.lfo_phase[ts]);
            return;
        }
        if (col == 1u) {
            i8_dec(s, (int32_t)g_st.lfo_skew[ts] - (int32_t)FM_LFO_SKEW_DEFAULT);
            return;
        }
        if (col == 2u) {
            u8_dec(s, g_st.lfo_smooth[ts]);
            return;
        }
        fm_lfo_fmt_depth(s, (int32_t)g_st.lfo_fold[ts]);
        return;
    }

    if (slot == P_LFO_TARGET) {
        uint32_t tslot = midi_lfo_target_slot(&g_cfg);
        const char *nm = midi_param_name(g_cfg.mode, tslot);
        char norm[16];

        home_hud_norm_label(nm, norm, sizeof norm);
        if (compact) {
            uint32_t i = 0, lim = 6u;

            while (norm[i] && i < lim) {
                s[i] = norm[i];
                i++;
            }
            s[i] = 0;
        } else
            copy_str(s, norm);
        return;
    }
    if (slot == P_LFO_SHAPE) {
        copy_str(s, fm_lfo_shape_name(v));
        return;
    }

    if (slot == P_OCTAVE) {
        int32_t oct = midi_oct_offset(&g_cfg);
        s[0] = oct < 0 ? '-' : '+';
        s[1] = (char)('0' + (oct < 0 ? -oct : oct));
        s[2] = 0;
        return;
    }
    if (slot == P_RATIO) {
        const char *frac = fm_lfo_ratio_name(v);
        if (frac) {
            copy_str(s, frac);
        } else {
            uint32_t r100 = (fm_lfo_ratio_curve_q8(v) * 100u + 128u) >> 8;
            uint32_t whole = r100 / 100u;
            uint32_t dec = r100 % 100u;
            s[0] = (char)('0' + (whole > 9u ? 9u : whole));
            s[1] = '.';
            s[2] = (char)('0' + (dec / 10u));
            s[3] = (char)('0' + (dec % 10u));
            s[4] = 0;
        }
        return;
    }
    if (slot == P_TEMPO) {
        u8_dec(s, 60u + v);
        return;
    }
    if (slot == P_SWING) {
        uint32_t sw = 50u + (v * 25u) / 127u;
        u8_dec(s, sw);
        s[2] = '%';
        s[3] = 0;
        return;
    }
    if (slot == P_GATE) {
        uint32_t idx = (v * 11u) / 128u;
        copy_str(s, DIVS[idx > 10u ? 10u : idx]);
        return;
    }
    if (slot == P_DETUNE) {
        int32_t dt = (int32_t)v - 64;
        if (dt < 0) {
            s[0] = '-';
            u8_dec(s + 1, (uint32_t)(-dt));
        } else {
            s[0] = '+';
            u8_dec(s + 1, (uint32_t)dt);
        }
        return;
    }
    if (g_cfg.mode == MODE_TRIAXIS) {
        if (slot == P_PARAM_A) {
            if (g_cfg.val[P_DENSITY] == 0u) {
                uint8_t c = 0, fn = 0, ap = 0;
                ct_axis_from_cc((uint8_t)v, g_cfg.val[P_PARAM_B], &c, &fn, &ap);
                if (ap) {
                    s[0] = '>'; s[1] = 'T'; s[2] = (char)('1' + c); s[3] = 0;
                } else {
                    s[0] = 'T'; s[1] = (char)('1' + c); s[2] = 0;
                }
            } else {
                static const char *const AX[4] = {"Maj3", "Min3", "4ths", "Trit"};
                copy_str(s, AX[(v >> 5) & 3u]);
            }
            return;
        }
        if (slot == P_PARAM_B) {
            if (g_cfg.val[P_DENSITY] == 0u) {
                static const char *const FN[3] = {"Imaj7", "V7", "ii7"};
                uint32_t fi = (v * 3u) / 128u;
                copy_str(s, FN[fi > 2u ? 2u : fi]);
            } else {
                static const char *const MOT_S[6] = {"Strt", "Ruba", "Shts", "Free", "Vmp1", "Vmp2"};
                static const char *const MOT_L[6] = {"Straight", "Rubato", "Sheets", "Free", "Vamp 1", "Vamp 2"};
                uint32_t mi = (v * 6u) / 128u;
                if (mi > 5u) mi = 5u;
                copy_str(s, compact ? MOT_S[mi] : MOT_L[mi]);
            }
            return;
        }
        if (slot == P_DENSITY) {
            if (v == 0u) {
                copy_str(s, "AXIS");
            } else {
                uint32_t d = 1u + ((v - 1u) * 6u) / 127u;
                if (d > 6u) d = 6u;
                s[0] = (char)('3' + d);
                s[1] = '/';
                s[2] = '9';
                s[3] = 0;
            }
            return;
        }
        if (slot == P_VEL_MOD) {
            static const char *const VC[4] = {"Triad", "7th", "Drop2", "Quart"};
            copy_str(s, VC[(v >> 5) & 3u]);
            return;
        }
    } else if (g_cfg.mode == MODE_CONTURA) {
        if (slot == P_PARAM_A) {
            uint32_t sc = (v * 20u) / 128u;
            const char *lbl = (compact ? CONTURA_SCALE_SHORT : CONTURA_SCALE_LABELS)[sc > 19u ? 19u : sc];
            copy_str(s, lbl);
            return;
        }
        if (slot == P_PARAM_B) {
            uint32_t stp = 3u + (v * 25u) / 127u;
            const char *q = stp <= 5u ? (compact ? "Cell" : " Cell")
                          : stp <= 9u ? (compact ? "Motif" : " Motif")
                          : stp <= 15u ? (compact ? "Arch" : " Arch")
                          : stp <= 21u ? (compact ? "Weave" : " Weave")
                                       : (compact ? "Odys" : " Odyssey");
            uint32_t len = 0;
            u8_dec(s, stp);
            while (s[len])
                len++;
            copy_str(s + len, q);
            return;
        }
        if (slot == P_VEL_MOD) {
            copy_str(s, CONTURA_LAYER_LABELS[contura_layer_idx((uint8_t)v)]);
            return;
        }
    } else if (g_cfg.mode == MODE_DITDAH) {
        if (slot == P_PARAM_A) {
            s[0] = '+';
            u8_dec(s + 1, (v * 24u) / 127u);
            return;
        }
        if (slot == P_PARAM_B) {
            static const char *const PR_L[23] = {
                "MONO PLS", "4X4 TECH", "SOS", "BINARY",
                "HOUSE", "ACID PMP", "MINIMAL", "TRESILLO", "DROP 5TH", "ELECTRO",
                "ACID 88", "NON STOP", "ROBOTS", "ALL BASE", "CQ DX",
                "GHOST", "EL.SHEEP", "I SYNTH", "HAL 9000", "NO FUTUR", "PLASTIC", "SUBTERR",
                "KOSMAR"};
            uint32_t ph = g_ditdah.phrase_idx < 23u ? g_ditdah.phrase_idx : ((v * 23u) / 128u);
            uint32_t ord;
            if (ph > 22u) ph = 22u;
            ord = ph + 1u;
            s[0] = (char)('0' + (ord / 10u));
            s[1] = (char)('0' + (ord % 10u));
            if (!compact && midi_phrase_is_unchanged(&g_cfg, ph)) {
                s[2] = ' ';
                copy_str(s + 3, PR_L[ph]);
            } else {
                s[2] = 0;
            }
            return;
        }
        if (slot == P_DENSITY) {
            u8_dec(s, v);
            return;
        }
        if (slot == P_VEL_MOD) {
            int32_t ac = (int32_t)v - 64;
            if (ac < 0) {
                s[0] = '-';
                u8_dec(s + 1, (uint32_t)(-ac));
            } else {
                s[0] = '+';
                u8_dec(s + 1, (uint32_t)ac);
            }
            return;
        }
    }
    if (slot == P_LFO_RATE) {
        v = fm_lfo_rate_clamp(v, g_st.bpm);
        const char *div = fm_lfo_note_div(v, g_st.bpm);
        uint32_t n = 0;

        if (div) {
            copy_str(s, div);
            return;
        }
        {
            uint32_t t = (fm_lfo_hz100(v) + 5u) / 10u;
            u8_dec(s, t / 10u);
            while (s[n])
                n++;
            s[n++] = '.';
            s[n++] = (char)('0' + (t % 10u));
            if (!compact)
                s[n++] = ' ';
            s[n++] = 'H';
            s[n++] = 'z';
            s[n] = 0;
        }
        return;
    }
    u8_dec(s, v);
}

static void draw_topbar(void)
{
    char s[32];
    int32_t oct = midi_oct_offset(&g_cfg);
    cv_begin(240, 24, C_BLACK);
    cv_text(4, 4, &FONT_S, MODE_NAMES[g_cfg.mode % NMODES], C_HI);
    if (g_cfg.midi_ch == 0) {
        cv_text(104, 4, &FONT_S, "OMNI", C_DIM);
    } else {
        s[0] = 'C';
        s[1] = 'H';
        u8_dec(s + 2, g_cfg.midi_ch);
        cv_text(96, 4, &FONT_S, s, C_DIM);
    }
    s[0] = g_st.ext_sync ? 'E' : 'I';
    s[1] = g_st.ext_sync ? 'X' : 'N';
    s[2] = 'T';
    s[3] = ' ';
    u8_dec(s + 4, g_st.bpm);
    {
        char octs[4];
        int32_t bpm_w, oct_w;
        octs[0] = ' ';
        octs[1] = oct < 0 ? '-' : '+';
        octs[2] = (char)('0' + (oct < 0 ? -oct : oct));
        octs[3] = 0;
        bpm_w = (int32_t)text_w(&FONT_S, s);
        oct_w = (int32_t)text_w(&FONT_S, octs);
        cv_text(236 - bpm_w - oct_w, 4, &FONT_S, s, C_AMB);
        {
            static const uint16_t OCT_COL[5] = {
                RGB(70, 150, 255),  /* -2 */
                RGB(90, 230, 255),  /* -1 */
                RGB(255, 255, 255), /*  0 */
                RGB(255, 176, 48),  /* +1 */
                RGB(255, 72, 64)    /* +2 */
            };
            int32_t oi = oct + 2;
            if (oi < 0)
                oi = 0;
            if (oi > 4)
                oi = 4;
            cv_text(236 - oct_w, 4, &FONT_S, octs, OCT_COL[oi]);
        }
    }
    cv_blit(0, 0);
}

static uint32_t ui_stage_h(void)
{
    if (g_home_view)
        return 216u;
    return 116u;
}

static uint32_t ui_keys_y(void)
{
    return 24u + 116u;
}

static uint32_t ui_bank_y(void)
{
    return 156u;
}

static uint32_t ui_param_y(void)
{
    return 218u;
}

static void lfo_target_from_last(void)
{
    uint32_t slot = g_st.hud_param;
    uint32_t i;

    for (i = 0; i < NLFO_TARGETS; i++) {
        if (LFO_TGT[i] != (uint8_t)slot)
            continue;
        g_cfg.base_val[P_LFO_TARGET] = midi_lfo_target_cc_from_idx(i);
        g_cfg.val[P_LFO_TARGET] = g_cfg.base_val[P_LFO_TARGET];
        g_cfg.base_val[P_LFO_DEPTH] = midi_lfo_get_depth(&g_cfg, &g_st, slot);
        g_cfg.val[P_LFO_DEPTH] = g_cfg.base_val[P_LFO_DEPTH];
        midi_lfo_recall_target_rate(&g_cfg, &g_st);
        return;
    }
}

static void perf_page_open(uint8_t pg)
{
    g_home_view = 0u;
    g_perf.page = pg;
    if (pg != PERF_PAGE_BANK)
        g_perf.page_last = pg;
    g_ui_dirty = 1u;
}

static void perf_page_bank(void)
{
    g_perf.page = PERF_PAGE_BANK;
    g_perf.page_last = PERF_PAGE_BANK;
}

static int32_t perf_select_walk_ix(void)
{
    if (g_perf.page == PERF_PAGE_ARP)
        return 5;
    if (g_perf.page == PERF_PAGE_SEQ)
        return 6;
    if (g_cfg.bank == BANK_SPACE)
        return 0;
    if (g_cfg.bank == BANK_ENGINE)
        return 1;
    if (g_cfg.bank == BANK_SYNTH)
        return 2;
    if (g_cfg.bank == BANK_LFO)
        return 3;
    if (g_cfg.bank == BANK_TIME)
        return 4;
    return 0;
}

static void home_hud_arm_bank(void)
{
    if (!g_home_view)
        return;
    g_home_hud_kind = 1u;
    g_home_hud_until = fm1_ms + 1400u;
    g_ui_dirty = 1u;
}

static void home_hud_arm_param(uint8_t col_or_slot)
{
    if (!g_home_view)
        return;
    g_home_hud_kind = 2u;
    g_home_hud_col = col_or_slot;
    g_home_hud_until = fm1_ms + 1400u;
    g_ui_dirty = 1u;
}

static void perf_select_walk_apply(int32_t ix)
{
    while (ix < 0)
        ix += 7;
    ix %= 7;
    if (ix == 5) {
        g_perf.page = PERF_PAGE_ARP;
        g_perf.page_last = PERF_PAGE_ARP;
        return;
    }
    if (ix == 6) {
        g_perf.page = PERF_PAGE_SEQ;
        g_perf.page_last = PERF_PAGE_SEQ;
        return;
    }
    g_perf.page = PERF_PAGE_BANK;
    g_perf.page_last = PERF_PAGE_BANK;
    if (ix == 0)
        g_cfg.bank = BANK_SPACE;
    else if (ix == 1)
        g_cfg.bank = BANK_ENGINE;
    else if (ix == 2)
        g_cfg.bank = BANK_SYNTH;
    else if (ix == 3) {
        g_cfg.bank = BANK_LFO;
        lfo_target_from_last();
    } else
        g_cfg.bank = BANK_TIME;
}

static uint8_t perf_capture_root(void)
{
    if (g_cfg.mode == MODE_TRIAXIS)
        return g_triptych.root;
    if (g_cfg.mode == MODE_CONTURA)
        return g_contura.base;
    return g_ditdah.base_note;
}

static void perf_apply_root(uint8_t note)
{
    if (note > 127u)
        note = 127u;
    if (g_cfg.mode == MODE_TRIAXIS)
        triptych_set_root(&g_triptych, note);
    else if (g_cfg.mode == MODE_CONTURA)
        contura_set_tonic(&g_contura, note);
    else
        ditdah_set_base(&g_ditdah, note);
}

static uint32_t perf_arp_mode_idx(uint8_t cc)
{
    uint32_t m = ((uint32_t)cc * 10u) / 128u;

    return m > 9u ? 9u : m;
}

static uint32_t perf_arp_mode(const perf_state_t *p)
{
    return perf_arp_mode_idx(p->arp[0]);
}

static uint32_t perf_arp_sp(const perf_state_t *p)
{
    uint32_t sp = ((uint32_t)p->arp[2] * 6u) / 128u;

    return sp > 5u ? 5u : sp;
}

static int32_t perf_arp_span_max(uint32_t sp)
{
    if (sp == 1u || sp == 4u)
        return 24;
    if (sp == 2u || sp == 5u)
        return 36;
    return 12;
}

static int32_t perf_arp_span_top(uint32_t sp)
{
    if (sp > 5u)
        sp = 5u;
    return perf_arp_span_max(sp);
}

static int32_t perf_arp_ladder_mag(uint32_t sp, int32_t level)
{
    static const int8_t L3[] = {0, 7, 12};
    static const int8_t L5[] = {0, 7, 12, 19, 24};
    static const int8_t L7[] = {0, 7, 14, 19, 24, 31, 36};
    uint32_t idx;

    if (level < 0)
        level = 0;
    if (level > 5)
        level = 5;
    if (sp <= 2u) {
        int32_t top = perf_arp_span_max(sp);

        return (level * top) / 5;
    }
    if (sp == 3u) {
        idx = (uint32_t)level * 2u / 5u;
        return (int32_t)L3[idx];
    }
    if (sp == 4u) {
        idx = (uint32_t)level * 4u / 5u;
        return (int32_t)L5[idx];
    }
    idx = (uint32_t)level * 6u / 5u;
    return (int32_t)L7[idx];
}

static int32_t perf_arp_pat_ix(uint32_t cell, uint32_t n)
{
    if (n <= 1u)
        return 0;
    return (int32_t)(cell * 5u / (n - 1u));
}

static int32_t perf_arp_signed_pat_level(uint32_t mode, uint32_t cell, uint32_t n)
{
    static const int8_t CONV[6] = {5, -5, 3, -3, 1, 0};
    static const int8_t DIVG[6] = {0, 1, -1, 3, -3, 5};
    static const int8_t PEDAL[6] = {0, 2, 0, 4, 0, 5};
    const int8_t *tab = 0;
    int32_t ix;

    if (n < 1u)
        n = 1u;
    cell %= n;
    ix = perf_arp_pat_ix(cell, n);
    if (ix < 0)
        ix = 0;
    if (ix > 5)
        ix = 5;
    if (mode == 4u)
        tab = CONV;
    else if (mode == 5u)
        tab = DIVG;
    else if (mode == 6u)
        tab = PEDAL;
    else
        return 0;
    return (int32_t)tab[ix];
}

static int32_t perf_arp_walk_level(const perf_state_t *p, uint32_t cell, uint32_t n)
{
    int32_t lvl = 3;
    uint32_t c;
    uint32_t end;

    if (n < 1u)
        n = 1u;
    end = cell % n;
    for (c = 0; c <= end; c++) {
        uint32_t seed = (c * 1103515245u + (p->arp_pat / n) * 12345u + 7u) >> 16;
        int32_t step = (seed & 1u) ? 1 : -1;

        lvl += step;
        if (lvl < 0)
            lvl = 0;
        if (lvl > 5)
            lvl = 5;
    }
    return lvl;
}

static int32_t perf_arp_signed_to_semi(uint32_t sp, int32_t signed_lvl)
{
    int32_t a;
    int32_t mag;

    if (signed_lvl == 0)
        return 0;
    a = signed_lvl < 0 ? -signed_lvl : signed_lvl;
    if (a > 5)
        a = 5;
    mag = perf_arp_ladder_mag(sp, a);
    return signed_lvl < 0 ? -mag : mag;
}

static int32_t perf_arp_level_to_mag(uint32_t sp, int32_t level)
{
    if (sp > 5u)
        sp = 5u;
    return perf_arp_ladder_mag(sp, level);
}

static int32_t perf_arp_updn_semi(uint32_t cell, uint32_t n, int32_t top)
{
    uint32_t q;
    uint32_t c;

    if (n <= 1u || top < 1)
        return 0;
    q = n / 4u;
    if (q < 1u)
        q = 1u;
    c = cell % n;
    if (c < q)
        return (int32_t)((int32_t)c * top / (int32_t)q);
    if (c < 2u * q)
        return top - (int32_t)((int32_t)(c - q) * top / (int32_t)q);
    if (c < 3u * q)
        return -((int32_t)((int32_t)(c - 2u * q) * top / (int32_t)q));
    return -top + (int32_t)((int32_t)(c - 3u * q) * top / (int32_t)q);
}

static int32_t perf_arp_rnd_semi(const perf_state_t *p, uint32_t cell, uint32_t n, int32_t top)
{
    uint32_t seed;

    if (top < 1)
        return 0;
    seed = (cell * 1103515245u + (p->arp_pat / (n ? n : 1u)) * 12345u + 17u) >> 16;
    return (int32_t)(seed % (2u * (uint32_t)top + 1u)) - top;
}

static uint32_t perf_arp_n_cells(uint8_t div_cc);
static int32_t perf_arp_stage_level(const perf_state_t *p, uint32_t mode, uint32_t cell, uint32_t n);

static int32_t perf_arp_stage_semi(const perf_state_t *p, uint32_t cell)
{
    uint32_t sp = perf_arp_sp(p);
    uint32_t mode = perf_arp_mode(p);
    uint32_t n = perf_arp_n_cells(p->arp[1]);
    int32_t top;
    int32_t lvl;
    int32_t mag;
    int32_t sl;
    int32_t wl;

    if (mode == 0u || mode == 9u)
        return 0;
    top = perf_arp_span_top(sp);
    if (mode == 3u)
        return perf_arp_updn_semi(cell, n, top);
    if (mode == 8u)
        return perf_arp_rnd_semi(p, cell, n, top);
    if (mode == 4u || mode == 5u || mode == 6u)
        return perf_arp_signed_to_semi(sp, perf_arp_signed_pat_level(mode, cell, n));
    if (mode == 7u) {
        wl = perf_arp_walk_level(p, cell, n);
        sl = wl - 3;
        if (sl == 0)
            return 0;
        mag = perf_arp_ladder_mag(sp, sl < 0 ? -sl : sl);
        return sl < 0 ? -mag : mag;
    }
    if (mode == 2u) {
        lvl = perf_arp_pat_ix(cell, n);
        return -perf_arp_level_to_mag(sp, lvl);
    }
    lvl = perf_arp_stage_level(p, mode, cell, n);
    mag = perf_arp_level_to_mag(sp, lvl);
    return mag;
}

static int32_t perf_arp_span_offset(const perf_state_t *p, uint32_t pat)
{
    return perf_arp_stage_semi(p, pat);
}

static uint32_t perf_arp_n_cells(uint8_t div_cc)
{
    static const uint8_t BEAT_CELLS[6] = {2u, 3u, 4u, 6u, 8u, 16u};
    uint32_t div_i = ((uint32_t)div_cc * 6u) / 128u;

    if (div_i > 5u)
        div_i = 5u;
    return BEAT_CELLS[div_i];
}

static int32_t perf_arp_stage_level(const perf_state_t *p, uint32_t mode, uint32_t cell, uint32_t n)
{
    uint32_t leg;

    (void)p;
    if (n < 1u)
        n = 1u;
    cell %= n;
    if (mode == 0u || mode == 9u)
        return 0;
    if (mode == 1u)
        return perf_arp_pat_ix(cell, n);
    if (mode == 2u)
        return 5 - perf_arp_pat_ix(cell, n);
    if (mode == 3u) {
        leg = n - 1u;
        if (leg == 0u)
            return 0;
        if (cell * 2u <= leg)
            return (int32_t)(cell * 2u * 5u / leg);
        return (int32_t)((leg - cell) * 2u * 5u / leg);
    }
    if (mode == 4u || mode == 5u || mode == 6u)
        return perf_arp_signed_pat_level(mode, cell, n);
    if (mode == 7u)
        return perf_arp_walk_level(p, cell, n);
    if (mode == 8u) {
        uint32_t cycle = p->arp_pat / n;

        return (int32_t)(((cell * 1103515245u + cycle * 12345u + 17u) >> 16) % 6u);
    }
    return 0;
}

static int32_t perf_arp_stage_y_top(uint32_t sp, int32_t y_lo, int32_t y_hi)
{
    int32_t amp = perf_arp_span_top(sp > 5u ? 5u : sp);

    return y_hi - ((y_hi - y_lo) * amp) / 36;
}

static int32_t perf_arp_semi_y(int32_t semi, int32_t y_mid, int32_t y_top, int32_t y_bot, int32_t max_semi)
{
    int32_t amp;

    (void)y_bot;
    if (max_semi < 1)
        max_semi = 1;
    amp = y_mid - y_top;
    if (amp < 8)
        amp = 8;
    if (semi > max_semi)
        semi = max_semi;
    if (semi < -max_semi)
        semi = -max_semi;
    return y_mid - (semi * amp) / max_semi;
}

static void perf_arp_semi_bar(int32_t semi, int32_t y_mid, int32_t y_top, int32_t y_bot, int32_t max_semi,
                              uint32_t arp_mode, int32_t *y0_out, int32_t *bh_out)
{
    int32_t amp;
    int32_t y_edge;

    if (max_semi < 1)
        max_semi = 1;
    amp = y_mid - y_top;
    if (amp < 8)
        amp = 8;
    if (semi > max_semi)
        semi = max_semi;
    if (semi < -max_semi)
        semi = -max_semi;
    if (semi == 0) {
        *bh_out = 6;
        if (arp_mode == 2u)
            *y0_out = y_mid;
        else
            *y0_out = y_mid - 6;
        return;
    }
    if (semi >= 0) {
        y_edge = y_mid - (semi * amp) / max_semi;
        *y0_out = y_edge;
        *bh_out = y_mid - y_edge;
    } else {
        y_edge = y_mid + (-semi * amp) / max_semi;
        if (y_edge > y_bot)
            y_edge = y_bot;
        *y0_out = y_mid;
        *bh_out = y_edge - y_mid;
    }
    if (*bh_out < 2)
        *bh_out = 2;
}

static int32_t perf_seq_slot_w(uint32_t nsteps)
{
    uint32_t n = nsteps ? nsteps : 4u;

    if (n == 32u)
        return 7;
    return (int32_t)(216u / n);
}

static int32_t perf_seq_col_x0(uint32_t nsteps)
{
    uint32_t n = nsteps ? nsteps : 4u;
    int32_t sw = perf_seq_slot_w(nsteps);
    int32_t total = sw * (int32_t)n;

    return 120 - total / 2;
}

static int32_t perf_seq_col_cx(uint32_t step, uint32_t nsteps)
{
    int32_t sw = perf_seq_slot_w(nsteps);

    return perf_seq_col_x0(nsteps) + (int32_t)step * sw + sw / 2;
}

static int32_t perf_seq_col_x(uint32_t step, uint32_t nsteps)
{
    return perf_seq_col_cx(step, nsteps);
}

static int32_t perf_seq_mut_pick(const perf_state_t *p, uint32_t step, uint32_t pct)
{
    static const int8_t M_LOW[4] = {2, -2, 5, -5};
    static const int8_t M_MED[5] = {7, -5, 3, -3, 5};
    static const int8_t M_HI[5] = {7, -5, 12, 3, -7};
    uint32_t h;

    if (pct == 0u)
        return 0;
    if (step == 0u && pct < 90u)
        return 0;
    h = (p->perf_tick * 17u + step * 31u + (uint32_t)p->seq_base_saved) % 100u;
    if (h >= pct)
        return 0;
    h = (p->perf_tick * 41u + step * 59u + (uint32_t)p->seq_base_saved * 3u) % 100u;
    if (pct <= 35u)
        return (int32_t)M_LOW[h % 4u];
    if (pct <= 75u)
        return (int32_t)M_MED[h % 5u];
    return (int32_t)M_HI[h % 5u];
}

static uint8_t arp_extra[8];
static uint8_t arp_extra_n;

static void perf_arp_release_extra(void)
{
    uint32_t i;

    for (i = 0; i < arp_extra_n && i < 8u; i++) {
        synth_note_off(arp_extra[i]);
        trm_off(tryp_keys_off_st(&g_cfg), arp_extra[i]);
    }
    arp_extra_n = 0;
}

static void perf_arp_hit(uint32_t note, int32_t semi_off)
{
    int32_t n = (int32_t)note + semi_off;
    uint32_t vh = 2u + (g_perf.arp_pat & 1u);

    while (n < 24)
        n += 12;
    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    synth_note_on_ex(vh, (uint32_t)n, 110u, 10, 192u, &g_cfg, &g_last_note);
    trm_on(tryp_keys_on_st(&g_cfg), (uint32_t)n, 110u);
    if (arp_extra_n >= 8u) {
        uint32_t i;

        synth_note_off(arp_extra[0]);
        trm_off(tryp_keys_off_st(&g_cfg), arp_extra[0]);
        for (i = 0; i < 7u; i++)
            arp_extra[i] = arp_extra[i + 1u];
        arp_extra[7] = (uint8_t)n;
    } else
        arp_extra[arp_extra_n++] = (uint8_t)n;
}

static int perf_arp_chord_mode(const perf_state_t *p)
{
    return perf_arp_mode(p) == 9u;
}

static void perf_arp_chord_stack(uint32_t n0, int32_t semi_off)
{
    perf_arp_hit(n0, semi_off);
    perf_arp_hit(n0, semi_off + 7);
    perf_arp_hit(n0, semi_off + 12);
}

static void perf_arp_sort_notes(uint8_t *dst, const uint8_t *src, uint32_t nn)
{
    uint32_t i;
    uint32_t j;

    for (i = 0; i < nn && i < 4u; i++)
        dst[i] = src[i];
    for (i = 0; i + 1u < nn; i++) {
        for (j = i + 1u; j < nn; j++) {
            if (dst[j] < dst[i]) {
                uint8_t t = dst[i];

                dst[i] = dst[j];
                dst[j] = t;
            }
        }
    }
}

static uint32_t perf_arp_bounce_ix(uint32_t pat, uint32_t nn)
{
    uint32_t per;

    if (nn <= 1u)
        return 0u;
    per = 2u * nn - 2u;
    pat %= per;
    if (pat < nn)
        return pat;
    return 2u * nn - 2u - pat;
}

static uint32_t perf_arp_tone_ix(uint32_t mode, uint32_t pat, uint32_t nn)
{
    uint32_t seed;

    if (nn == 0u)
        return 0u;
    if (mode == 1u)
        return pat % nn;
    if (mode == 2u)
        return nn - 1u - (pat % nn);
    if (mode == 3u)
        return perf_arp_bounce_ix(pat, nn);
    seed = (pat * 1103515245u + 17u) >> 16;
    return seed % nn;
}

static int32_t perf_arp_chord_accent(const perf_state_t *p)
{
    uint32_t sp = perf_arp_sp(p);

    if ((g_perf.arp_pat & 1u) == 0u)
        return 0;
    if (sp >= 3u)
        return 7;
    return 12;
}

static uint32_t perf_arp_contura_note(uint32_t mode, uint32_t pat)
{
    uint32_t pl = g_contura.pool_len;
    int32_t base_i = g_contura.lead_idx >= 0 ? g_contura.lead_idx : 0;
    uint32_t ix;

    if (pl == 0u)
        return g_contura.base;
    ix = perf_arp_tone_ix(mode, pat, pl);
    ix = ((uint32_t)base_i + ix) % pl;
    return g_contura.pool[ix];
}

static void perf_arp_retrigger(int32_t semi_off)
{
    uint32_t mode = perf_arp_mode(&g_perf);
    uint32_t pat = g_perf.arp_pat;

    perf_arp_release_extra();
    if (mode == 0u)
        return;
    if (g_cfg.mode == MODE_TRIAXIS) {
        uint8_t notes[4];
        uint32_t nn = (uint32_t)g_triptych.n_sound;

        if (nn == 0u)
            return;
        if (nn > 4u)
            nn = 4u;
        perf_arp_sort_notes(notes, g_triptych.sounding, nn);
        if (mode == 9u) {
            uint32_t v;
            int32_t acc = perf_arp_chord_accent(&g_perf);

            for (v = 0; v < nn; v++)
                perf_arp_hit(notes[v], semi_off + acc);
        } else {
            perf_arp_hit(notes[perf_arp_tone_ix(mode, pat, nn)], semi_off);
        }
    } else if (g_cfg.mode == MODE_CONTURA) {
        if (mode == 9u) {
            uint32_t n0;

            if (g_contura.lead_note)
                n0 = g_contura.lead_note;
            else if (g_contura.burst_note)
                n0 = g_contura.burst_note;
            else if (g_contura.bass_note)
                n0 = g_contura.bass_note;
            else
                n0 = g_contura.base;
            perf_arp_chord_stack(n0, semi_off + perf_arp_chord_accent(&g_perf));
        } else
            perf_arp_hit(perf_arp_contura_note(mode, pat), semi_off);
    } else {
        uint32_t n0 = g_ditdah.base_note;
        uint32_t dit;
        uint32_t dah;

        if (!n0)
            return;
        dit = n0;
        dah = n0 + ((uint32_t)g_cfg.val[P_PARAM_A] * 24u) / 127u;
        if (dah > 127u)
            dah = 127u;
        if (mode == 9u)
            perf_arp_chord_stack(n0, semi_off + perf_arp_chord_accent(&g_perf));
        else {
            uint8_t notes[2];

            notes[0] = (uint8_t)dit;
            notes[1] = (uint8_t)dah;
            perf_arp_hit(notes[perf_arp_tone_ix(mode, pat, 2u)], semi_off);
        }
    }
}

static uint8_t seq_extra[8];
static uint8_t seq_extra_n;

static void perf_seq_release_extra(void)
{
    uint32_t i;

    for (i = 0; i < seq_extra_n && i < 8u; i++) {
        synth_note_off(seq_extra[i]);
        trm_off(tryp_keys_off_st(&g_cfg), seq_extra[i]);
    }
    seq_extra_n = 0;
}

static uint8_t perf_seq_fold_voice(int32_t n)
{
    while (n < 45)
        n += 12;
    while (n > 76)
        n -= 12;
    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    return (uint8_t)n;
}

static void perf_seq_voice_retrigger(uint8_t *note_io, int32_t delta)
{
    int32_t n;
    uint8_t old = *note_io;

    if (!old)
        return;
    synth_note_off(old);
    trm_off(tryp_keys_off_st(&g_cfg), old);
    n = (int32_t)old + delta;
    n = (int32_t)perf_seq_fold_voice(n);
    synth_note_on((uint32_t)n, 100u, &g_cfg, &g_last_note);
    trm_on(tryp_keys_on_st(&g_cfg), (uint32_t)n, 100u);
    *note_io = (uint8_t)n;
    if (seq_extra_n >= 8u) {
        uint32_t i;

        synth_note_off(seq_extra[0]);
        trm_off(tryp_keys_off_st(&g_cfg), seq_extra[0]);
        for (i = 0; i < 7u; i++)
            seq_extra[i] = seq_extra[i + 1u];
        seq_extra[7] = (uint8_t)n;
    } else
        seq_extra[seq_extra_n++] = (uint8_t)n;
}

static void perf_seq_voice_fire(uint8_t note)
{
    if (!note)
        return;
    synth_note_off(note);
    trm_off(tryp_keys_off_st(&g_cfg), note);
    synth_note_on((uint32_t)note, 100u, &g_cfg, &g_last_note);
    trm_on(tryp_keys_on_st(&g_cfg), note, 100u);
    if (seq_extra_n >= 8u) {
        uint32_t i;

        synth_note_off(seq_extra[0]);
        trm_off(tryp_keys_off_st(&g_cfg), seq_extra[0]);
        for (i = 0; i < 7u; i++)
            seq_extra[i] = seq_extra[i + 1u];
        seq_extra[7] = note;
    } else
        seq_extra[seq_extra_n++] = note;
}

static void perf_seq_retrigger(int32_t delta)
{
    uint32_t v;

    perf_seq_release_extra();
    if (delta == 0)
        return;
    if (g_cfg.mode == MODE_TRIAXIS) {
        for (v = 0; v < (uint32_t)g_triptych.n_sound && v < 4u; v++)
            perf_seq_voice_retrigger(&g_triptych.sounding[v], delta);
    } else if (g_cfg.mode == MODE_CONTURA) {
        perf_seq_voice_fire(g_contura.lead_note);
        perf_seq_voice_fire(g_contura.bass_note);
        perf_seq_voice_fire(g_contura.burst_note);
    } else if (g_ditdah.base_note) {
        perf_seq_voice_retrigger(&g_ditdah.base_note, delta);
    }
}

static uint8_t perf_seq_clamp_root(int32_t base, int32_t off)
{
    int32_t nn = base + off;
    int32_t lo = base - 12;
    int32_t hi = base + 12;
    int32_t abs_lo = 36;
    int32_t abs_hi = (g_cfg.mode == MODE_TRIAXIS) ? 64 : 72;

    if (nn < lo)
        nn = lo;
    if (nn > hi)
        nn = hi;
    if (nn < abs_lo)
        nn = abs_lo;
    if (nn > abs_hi)
        nn = abs_hi;
    return (uint8_t)nn;
}

static void perf_seq_step_advance(perf_state_t *p)
{
    uint32_t nsteps = perf_seq_nsteps(p->seq[0]);
    uint32_t step;
    int32_t off;
    int32_t mut;
    uint32_t pct;
    uint8_t prev;
    uint8_t nn;

    if (!nsteps)
        return;
    if (!p->seq_root_captured) {
        p->seq_base_saved = perf_capture_root();
        p->seq_root_captured = 1u;
    }
    step = p->seq_step_i % nsteps;
    perf_seq_prog_offset(p, step, &off);
    pct = ((uint32_t)p->seq[3] * 100u) / 127u;
    mut = perf_seq_mut_pick(p, step, pct);
    off += mut;
    p->seq_mut_off = (int8_t)mut;
    prev = perf_capture_root();
    nn = perf_seq_clamp_root((int32_t)p->seq_base_saved, off);
    perf_apply_root(nn);
    if (nn != prev)
        perf_seq_retrigger((int32_t)nn - (int32_t)prev);
    p->seq_step_i = (p->seq_step_i + 1u) % nsteps;
}

static void perf_rec_apply_motion(perf_state_t *p, midi_cfg_t *c)
{
    uint32_t slot = p->motion_slot % NPARAMS;
    int32_t delta = (int32_t)p->motion_buf[p->rec_bucket % 64u];
    int32_t v = (int32_t)p->motion_base[slot] + delta;
    uint32_t pass = p->rec_bucket / 64u;
    uint32_t decay = p->rec[3];

    if (decay < 127u && pass > 0u) {
        int32_t fade = (int32_t)(127u - decay);
        v = (int32_t)p->motion_base[slot] + (delta * (127 - (int32_t)(pass * fade / 8u))) / 127;
    }
    if (v < 0)
        v = 0;
    if (v > 127)
        v = 127;
    c->base_val[slot] = (uint8_t)v;
    c->val[slot] = (uint8_t)v;
}

static void perf_pre_tick(perf_state_t *p, midi_cfg_t *c, uint32_t step_ticks)
{
    uint32_t i;

    if (!step_ticks || !g_st.playing)
        return;

    for (i = 0; i < step_ticks; i++) {
        p->perf_tick++;

        if (perf_seq_active(p)) {
            if (!p->seq_root_captured) {
                p->seq_base_saved = perf_capture_root();
                p->seq_root_captured = 1u;
            }
            p->seq_acc++;
            if (p->seq_acc >= perf_seq_pace_ticks(p->seq[1])) {
                p->seq_acc = 0;
                perf_seq_step_advance(p);
            }
        } else if (p->seq_root_captured) {
            perf_apply_root(p->seq_base_saved);
            perf_seq_release_extra();
            p->seq_root_captured = 0;
            p->seq_mut_off = 0;
            p->seq_step_i = 0;
            p->seq_acc = 0;
        }

        {
            uint32_t rec_st = perf_rec_state(p);

            if (rec_st >= 2u) {
                uint32_t bars = perf_rec_loop_bars(p->rec[2]);

                p->rec_loop_ticks = bars * 96u;
                p->rec_acc++;
                if (p->rec_loop_ticks > 0u) {
                    p->rec_bucket = (p->rec_acc * 64u) / p->rec_loop_ticks;
                    if (p->rec_bucket >= 64u)
                        p->rec_bucket = 63u;
                }
                {
                    uint32_t rt = ((uint32_t)p->rec[1] * 3u) / 128u;

                    if (rt != 0u)
                        perf_rec_apply_motion(p, c);
                }
                if (p->rec_acc >= p->rec_loop_ticks) {
                    p->rec_acc = 0;
                    p->rec_bucket = 0;
                    if (rec_st == 2u) {
                        p->rec[0] = 89u;
                        g_ui_dirty = 1u;
                    }
                }
            }
        }
    }
}

static void perf_post_tick(perf_state_t *p, midi_cfg_t *c, uint32_t step_ticks)
{
    uint32_t i;

    (void)c;
    if (!step_ticks || !g_st.playing)
        return;

    for (i = 0; i < step_ticks; i++) {
        if (perf_arp_active(p)) {
            uint32_t div = perf_arp_div_ticks(p->arp[1]);
            uint32_t gate_pct = 15u + ((uint32_t)p->arp[3] * 80u) / 127u;

            p->arp_acc++;
            if (p->arp_acc >= div) {
                int32_t semi = perf_arp_span_offset(p, p->arp_pat);

                p->arp_acc = 0;
                perf_arp_retrigger(semi);
                p->arp_gate_left = (div * gate_pct) / 100u;
                if (p->arp_gate_left == 0u)
                    p->arp_gate_left = 1u;
                p->arp_pat++;
            } else if (p->arp_gate_left > 0u) {
                p->arp_gate_left--;
                if (p->arp_gate_left == 0u)
                    perf_arp_release_extra();
            }
        } else if (arp_extra_n)
            perf_arp_release_extra();
    }
}

static uint8_t perf_btn_lamp(uint8_t btn, uint8_t pg)
{
    int open = !g_home_view && g_perf.page == pg;
    int bg = 0;

    if (pg == PERF_PAGE_ARP)
        bg = perf_arp_active(&g_perf);
    else if (pg == PERF_PAGE_SEQ)
        bg = perf_seq_active(&g_perf);
    else if (pg == PERF_PAGE_REC)
        bg = perf_rec_active(&g_perf);

    if (open)
        return 255u;
    if (bg && g_st.playing) {
        uint32_t ph = fm1_ms * 2147484u;
        int32_t w = fm_lfo_classic_at(ph, 0u, 0u, btn);
        return (uint8_t)((uint32_t)(w + 64) * 255u / 127u);
    }
    if (g_home_view && g_perf.page_last == pg)
        return 0u;
    return 0u;
}

static void perf_fmt_param(uint8_t page, uint32_t col, char *s)
{
    const perf_state_t *p = &g_perf;

    if (page == PERF_PAGE_ARP) {
        static const char *const MODE[10] = {
            "Off", "Up", "Down", "UpDn", "Conv", "Divg", "Pedal", "Walk", "Rnd", "Chord"
        };
        static const char *const DIV[6] = {"1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64"};
        static const char *const SPAN[6] = {
            "1 Oct", "2 Oct", "3 Oct", "1 5th", "2 5ths", "3 5ths"
        };
        if (col == 0u) {
            uint32_t m = perf_arp_mode_idx(p->arp[0]);

            copy_str(s, MODE[m > 9u ? 9u : m]);
        } else if (col == 1u) {
            uint32_t d = ((uint32_t)p->arp[1] * 6u) / 128u;
            copy_str(s, DIV[d > 5u ? 5u : d]);
        } else if (col == 2u) {
            uint32_t sp = perf_arp_sp(p);

            copy_str(s, SPAN[sp > 5u ? 5u : sp]);
        } else {
            uint32_t g = 15u + ((uint32_t)p->arp[3] * 80u) / 127u;
            u8_dec(s, g);
            s[2] = '%';
            s[3] = 0;
        }
        return;
    }
    if (page == PERF_PAGE_SEQ) {
        static const char *const PACE[7] = {"1/16", "1/8", "1/4", "1/2B", "1Bar", "2Bar", "4Bar"};
        static const char *const PROG[8] = {"I-vi", "i-VI", "5ths", "4ths", "Octv", "Chrm", "Phryg", "User"};
        if (col == 0u) {
            uint32_t st = perf_seq_nsteps(p->seq[0]);
            if (st == 0u)
                copy_str(s, "Off");
            else
                u8_dec(s, st);
        } else if (col == 1u) {
            copy_str(s, PACE[perf_seq_pace_idx(p->seq[1])]);
        } else if (col == 2u) {
            uint32_t pr = ((uint32_t)p->seq[2] * 8u) / 128u;
            copy_str(s, PROG[pr > 7u ? 7u : pr]);
        } else {
            uint32_t m = ((uint32_t)p->seq[3] * 100u) / 127u;
            u8_dec_pct(s, m);
        }
        return;
    }
    if (page == PERF_PAGE_REC) {
        static const char *const ST[5] = {"Off", "Arm", "Rec", "Play", "Dub"};
        static const char *const TG[3] = {"Notes", "Knobs", "Both"};
        static const char *const LN[4] = {"1 Bar", "2 Bar", "4 Bar", "8 Bar"};
        if (col == 0u) {
            uint32_t st = perf_rec_state(p);

            copy_str(s, ST[st]);
        } else if (col == 1u) {
            uint32_t t = ((uint32_t)p->rec[1] * 3u) / 128u;
            copy_str(s, TG[t > 2u ? 2u : t]);
        } else if (col == 2u) {
            uint32_t l = ((uint32_t)p->rec[2] * 4u) / 128u;
            copy_str(s, LN[l > 3u ? 3u : l]);
        } else {
            uint32_t d = 20u + ((uint32_t)p->rec[3] * 80u) / 127u;
            u8_dec_pct(s, d);
        }
    }
}

static const char *perf_param_label(uint8_t page, uint32_t col)
{
    static const char *const ARP[4] = {"MODE", "DIV", "SPAN", "GATE"};
    static const char *const SEQ[4] = {"STEPS", "PACE", "PROG", "MUTATE"};
    static const char *const REC[4] = {"STATE", "TARGET", "LENGTH", "DECAY"};
    if (page == PERF_PAGE_ARP)
        return ARP[col];
    if (page == PERF_PAGE_SEQ)
        return SEQ[col];
    if (page == PERF_PAGE_REC)
        return REC[col];
    return "";
}

static uint32_t home_hud_append(char *dst, uint32_t j, uint32_t cap, const char *src)
{
    while (src[0] && j + 1u < cap) {
        dst[j++] = src[0];
        src++;
    }
    dst[j] = 0;
    return j;
}

static char home_hud_fold(char c, int upper)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(upper ? c : (char)(c + ('a' - 'A')));
    if (c >= 'a' && c <= 'z')
        return (char)(upper ? (char)(c - ('a' - 'A')) : c);
    return c;
}

static int home_hud_word_eq(const char *s, const char *w)
{
    while (w[0]) {
        char a = s[0];
        char b = w[0];

        if (a >= 'a' && a <= 'z')
            a = (char)(a - ('a' - 'A'));
        if (b >= 'a' && b <= 'z')
            b = (char)(b - ('a' - 'A'));
        if (a != b)
            return 0;
        s++;
        w++;
    }
    if ((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z'))
        return 0;
    return 1;
}

static uint32_t home_hud_norm_label(const char *src, char *dst, uint32_t cap)
{
    uint32_t j = 0;
    int word = 1;

    if (!src || cap < 2u)
        return 0;
    while (src[0] && j + 1u < cap) {
        char c = src[0];

        if (c == ' ' || c == '.' || c == '/') {
            dst[j++] = c;
            word = 1;
            src++;
            continue;
        }
        if (word) {
            if (home_hud_word_eq(src, "LFO")) {
                if (j + 4u >= cap)
                    break;
                dst[j++] = 'L';
                dst[j++] = 'F';
                dst[j++] = 'O';
                src += 3;
                word = 0;
                continue;
            }
            if (home_hud_word_eq(src, "FX")) {
                if (j + 3u >= cap)
                    break;
                dst[j++] = 'F';
                dst[j++] = 'X';
                src += 2;
                word = 0;
                continue;
            }
            dst[j++] = home_hud_fold(c, 1);
            word = 0;
        } else
            dst[j++] = home_hud_fold(c, 0);
        src++;
    }
    dst[j] = 0;
    return j;
}

static void home_hud_perf_label(char *dst, uint32_t cap, uint8_t page, uint32_t col)
{
    const char *raw = perf_param_label(page, col);
    char piece[16];

    if (page == PERF_PAGE_ARP && cap >= 10u) {
        if (col == 0u)
            copy_str(piece, "Mode");
        else if (col == 1u)
            copy_str(piece, "Div");
        else if (col == 2u)
            copy_str(piece, "Span");
        else
            copy_str(piece, "Gate");
        dst[0] = 'A';
        dst[1] = '.';
        copy_str(dst + 2, piece);
        return;
    }
    if (page == PERF_PAGE_SEQ && cap >= 10u) {
        if (col == 0u)
            copy_str(piece, "Step");
        else if (col == 1u)
            copy_str(piece, "Pace");
        else if (col == 2u)
            copy_str(piece, "Prog");
        else
            home_hud_norm_label(raw, piece, sizeof piece);
        dst[0] = 'S';
        dst[1] = '.';
        copy_str(dst + 2, piece);
        return;
    }
    home_hud_norm_label(raw, dst, cap);
}

static void draw_home_hud_band(void)
{
    char line[52];
    char val_s[24];
    char lbl[28];
    uint32_t j = 0;
    uint16_t fg = C_WHITE;
    int32_t tw, x;

    line[0] = 0;
    if (g_home_hud_kind == 1u) {
        static const uint16_t GRP_COL[4] = {
            RGB(75, 220, 255),
            RGB(85, 190, 255),
            RGB(95, 160, 255),
            RGB(110, 130, 255)
        };
        static const uint8_t GRP_BANK[4] = {BANK_SPACE, BANK_ENGINE, BANK_SYNTH, BANK_TIME};
        static const uint16_t PERF_COL[2] = {
            RGB(70, 210, 235),
            RGB(130, 145, 255)
        };

        if (g_perf.page == PERF_PAGE_ARP) {
            fg = PERF_COL[0];
            home_hud_append(line, j, sizeof line, "[ARP] Ratchet");
        } else if (g_perf.page == PERF_PAGE_SEQ) {
            fg = PERF_COL[1];
            home_hud_append(line, j, sizeof line, "[SEQ] Transpose");
        } else if (g_perf.page == PERF_PAGE_REC) {
            fg = RGB(200, 80, 180);
            home_hud_append(line, j, sizeof line, "Rec / Looper");
        } else if (g_cfg.bank == BANK_LFO) {
            fg = C_ACC;
            home_hud_append(line, j, sizeof line, "[LFO] Modulation");
        } else {
            uint32_t b = g_cfg.bank;
            uint32_t gi;
            const char *btn_role = 0;

            for (gi = 0; gi < 4u; gi++) {
                if (GRP_BANK[gi] == b)
                    break;
            }
            if (gi >= 4u)
                gi = 0u;
            fg = GRP_COL[gi];
            if (b == BANK_SYNTH)
                btn_role = "[ENV] Voice";
            else if (b == BANK_ENGINE)
                btn_role = "[SEL] Engine";
            else if (b == BANK_TIME)
                btn_role = "[GLO] Time";
            else if (b == BANK_SPACE)
                btn_role = "[FX] Space";
            if (btn_role)
                home_hud_append(line, j, sizeof line, btn_role);
        }
    } else if (g_home_hud_kind == 2u) {
        if (g_perf.page == PERF_PAGE_ARP || g_perf.page == PERF_PAGE_SEQ ||
            g_perf.page == PERF_PAGE_REC) {
            uint32_t col = (uint32_t)g_home_hud_col % 4u;

            if (g_perf.page == PERF_PAGE_ARP)
                fg = RGB(70, 210, 235);
            else if (g_perf.page == PERF_PAGE_SEQ)
                fg = RGB(130, 145, 255);
            else
                fg = RGB(200, 80, 180);
            home_hud_perf_label(lbl, sizeof lbl, g_perf.page, col);
            j = home_hud_append(line, j, sizeof line, lbl);
            j = home_hud_append(line, j, sizeof line, ": ");
            perf_fmt_param(g_perf.page, col, val_s);
            home_hud_append(line, j, sizeof line, val_s);
        } else {
            uint32_t slot = (uint32_t)g_home_hud_col % NPARAMS;
            const char *nm = midi_param_name(g_cfg.mode, slot);

            param_fmt_val(slot, val_s, 0);
            if (g_cfg.bank == BANK_LFO && g_perf.page == PERF_PAGE_BANK &&
                slot >= (uint32_t)P_LFO_TARGET && slot <= (uint32_t)P_LFO_SHAPE) {
                uint32_t ts = midi_lfo_target_slot(&g_cfg);
                const char *tgt = midi_param_name(g_cfg.mode, ts);
                char tgt_lbl[20];
                char sfx[8];

                fg = C_ACC;
                home_hud_norm_label(tgt, tgt_lbl, sizeof tgt_lbl);
                if (slot == (uint32_t)P_LFO_TARGET) {
                    uint32_t dep = g_cfg.val[P_LFO_DEPTH];

                    home_hud_norm_label(val_s, lbl, sizeof lbl);
                    j = home_hud_append(line, j, sizeof line, "[LFO] Target: ");
                    j = home_hud_append(line, j, sizeof line, lbl);
                    if (dep > 0u) {
                        u8_dec(sfx, dep);
                        j = home_hud_append(line, j, sizeof line, " (");
                        j = home_hud_append(line, j, sizeof line, sfx);
                        j = home_hud_append(line, j, sizeof line, ")");
                    }
                } else if (slot == (uint32_t)P_LFO_RATE) {
                    j = home_hud_append(line, j, sizeof line, "[");
                    j = home_hud_append(line, j, sizeof line, tgt_lbl);
                    j = home_hud_append(line, j, sizeof line, "] Freq: ");
                    home_hud_append(line, j, sizeof line, val_s);
                } else if (slot == (uint32_t)P_LFO_DEPTH) {
                    j = home_hud_append(line, j, sizeof line, "[");
                    j = home_hud_append(line, j, sizeof line, tgt_lbl);
                    j = home_hud_append(line, j, sizeof line, "] Depth: ");
                    home_hud_append(line, j, sizeof line, val_s);
                } else {
                    j = home_hud_append(line, j, sizeof line, "[");
                    j = home_hud_append(line, j, sizeof line, tgt_lbl);
                    j = home_hud_append(line, j, sizeof line, "] Shape: ");
                    home_hud_append(line, j, sizeof line, val_s);
                }
            } else {
                fg = C_HI;
                home_hud_norm_label(nm, lbl, sizeof lbl);
                j = home_hud_append(line, j, sizeof line, lbl);
                j = home_hud_append(line, j, sizeof line, ": ");
                home_hud_append(line, j, sizeof line, val_s);
            }
        }
    } else
        return;

    tw = (int32_t)text_w(&FONT_S, line);
    x = (240 - tw) / 2;
    if (x < 4)
        x = 4;
    cv_text(x, 152, &FONT_S, line, fg);
}


static void draw_rec_ring_pt(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t bucket, int32_t rad, int32_t *ox, int32_t *oy);


static void draw_arp_stage(const perf_state_t *p)
{
    static const int32_t CHORD_LV[3] = {1, 3, 5};
    uint32_t h = ui_stage_h();
    uint32_t mode = perf_arp_mode(p);
    uint32_t sp = perf_arp_sp(p);
    uint32_t gate_pct = 15u + ((uint32_t)p->arp[3] * 80u) / 127u;
    uint32_t n_cells = perf_arp_n_cells(p->arp[1]);
    uint32_t div = perf_arp_div_ticks(p->arp[1]);
    uint32_t play = p->arp_pat % n_cells;
    uint32_t i;
    int32_t y_lo = 24;
    int32_t y_hi = 108;
    int32_t y_top = perf_arp_stage_y_top(sp, y_lo, y_hi);
    int32_t y_mid = 66;
    int32_t max_semi = perf_arp_span_top(sp);
    int32_t prev_cx = -1;
    int32_t prev_y = -1;
    uint16_t col = RGB(64, 200, 220);
    uint16_t slot_col = RGB(22, 58, 66);

    if (mode > 9u)
        mode = 9u;
    cv_begin(240, h, C_BLACK);
    cv_line(12, y_hi, 228, y_hi, C_LINE);
    if (mode != 0u)
        cv_line(12, y_mid, 228, y_mid, C_DIM);
    for (i = 0; i < n_cells; i++) {
        int32_t cell_w = (int32_t)(216u / n_cells);
        int32_t sx = 12 + (int32_t)i * cell_w;
        int32_t slot_w = cell_w - 2;
        int32_t gw = (slot_w * (int32_t)gate_pct) / 100;
        int32_t bx;
        int32_t bh;
        int32_t y0;
        int32_t y_traj;

        if (slot_w < 2)
            slot_w = 2;
        if (gw < 2)
            gw = 2;
        if (gw > slot_w)
            gw = slot_w;
        bx = sx;
        cv_line(sx, y_hi, sx + slot_w - 1, y_hi, slot_col);
        cv_pset(sx, y_hi - 1, slot_col);
        cv_pset(sx + slot_w - 1, y_hi - 1, slot_col);
        if (mode == 0u)
            continue;
        if (mode == 9u) {
            int32_t k;
            int32_t y_min = y_mid;

            for (k = 0; k < 3; k++) {
                int32_t semi = perf_arp_ladder_mag(sp, CHORD_LV[k]);
                int32_t yk = perf_arp_semi_y(semi, y_mid, y_top, y_hi, max_semi);

                if (yk < y_min)
                    y_min = yk;
                cv_rect(bx, yk - 2, gw, 4, col);
            }
            bh = y_mid - y_min;
            if (bh < 2)
                bh = 2;
            y0 = y_min;
            y_traj = y0;
        } else {
            int32_t semi = perf_arp_stage_semi(p, i);

            perf_arp_semi_bar(semi, y_mid, y_top, y_hi, max_semi, mode, &y0, &bh);
            y_traj = semi == 0 ? y_mid : (semi >= 0 ? y0 : y0 + bh);
            cv_rect(bx, y0, gw, bh, col);
        }
        if (prev_cx >= 0)
            cv_line(prev_cx + cell_w / 2, prev_y, sx + cell_w / 2, y_traj, RGB(32, 110, 125));
        prev_cx = sx;
        prev_y = y_traj;
        if (i == play) {
            uint32_t max_gate = (div * gate_pct) / 100u;
            int32_t tail;
            int32_t gx = bx + gw;
            int32_t rest_w = slot_w - gw;

            if (max_gate == 0u)
                max_gate = 1u;
            tail = (rest_w * (int32_t)p->arp_gate_left) / (int32_t)max_gate;
            cv_rect(bx, y0, gw, bh, C_WHITE);
            if (gw >= 2) {
                cv_line(bx, y0, bx + gw - 1, y0, C_ACC);
                cv_line(bx, y0, bx, y0 + bh - 1, C_ACC);
                cv_line(bx + gw - 1, y0, bx + gw - 1, y0 + bh - 1, C_ACC);
                cv_line(bx, y0 + bh - 1, bx + gw - 1, y0 + bh - 1, C_ACC);
            }
            if (rest_w > 0)
                cv_line(gx, y_hi - 2, gx + rest_w - 1, y_hi - 2, RGB(18, 48, 56));
            if (tail > 0) {
                if (tail > rest_w)
                    tail = rest_w;
                if (tail > 0) {
                    cv_rect(gx, y_hi - 4, tail, 3, C_ACC);
                    cv_rect(gx, y_hi - 6, tail > 2 ? tail - 2 : 1, 2, RGB(120, 90, 200));
                }
            }
        }
    }
    cv_text(8, 4, &FONT_S, "Ratchet", RGB(64, 200, 220));
    cv_blit(0, 24);
}


static int32_t perf_seq_off_y(int32_t y_base, int32_t off, int32_t y_min, int32_t y_max)
{
    int32_t y = y_base - off * 3;

    if (y < y_min)
        y = y_min;
    if (y > y_max)
        y = y_max;
    return y;
}

static int32_t seq_pace_arch_lift(uint32_t pace_idx)
{
    static const int32_t L[7] = {4, 7, 10, 14, 18, 23, 28};

    return L[pace_idx > 6u ? 6u : pace_idx];
}

static int32_t seq_arch_parab_y(int32_t x0, int32_t x1, int32_t y0, int32_t lift, int32_t x)
{
    int32_t span = x1 - x0;
    int32_t dy;

    if (span == 0)
        return y0;
    dy = (4 * lift * (x - x0) * (x1 - x)) / (span * span);
    return y0 - dy;
}

static void draw_seq_pace_arch_span(int32_t x0, int32_t x1, int32_t y0, int32_t lift, uint16_t col,
    int32_t x_stop)
{
    int32_t span = x1 - x0;
    int32_t step;
    int32_t px = x0;
    int32_t py = y0;
    int32_t x;

    if (span == 0)
        return;
    step = span > 0 ? 1 : -1;
    if (span < 0)
        span = -span;
    x = x0;
    for (;;) {
        x += step;
        if (step > 0) {
            if (x > x1)
                break;
            if (x_stop >= 0 && x > x_stop)
                break;
        } else {
            if (x < x1)
                break;
            if (x_stop >= 0 && x < x_stop)
                break;
        }
        {
            int32_t y = seq_arch_parab_y(x0, x1, y0, lift, x);

            cv_line(px, py, x, y, col);
            py = y;
        }
        px = x;
    }
}

static void draw_seq_pace_arch_beads(int32_t x0, int32_t x1, int32_t y0, int32_t lift, uint32_t pace_idx,
    uint16_t col_dim, uint16_t col_lit, int32_t prog256)
{
    uint32_t n;
    uint32_t den;

    if (pace_idx < 4u)
        return;
    if (pace_idx == 4u) {
        den = 2u;
        n = 1u;
    } else if (pace_idx == 5u) {
        den = 3u;
        n = 2u;
    } else {
        den = 5u;
        n = 4u;
    }
    for (uint32_t k = 1u; k <= n; k++) {
        int32_t span = x1 - x0;
        int32_t bx;
        int32_t by;
        int32_t t256;
        uint16_t bc;

        if (span == 0)
            continue;
        t256 = (int32_t)((k * 256u) / den);
        bx = x0 + (span * t256) / 256;
        by = seq_arch_parab_y(x0, x1, y0, lift, bx);
        bc = col_dim;
        if (prog256 >= 0 && t256 <= prog256)
            bc = col_lit;
        cv_rect(bx - 1, by - 1, 3, 3, bc);
    }
}

static void draw_seq_pace_link(int32_t x0, int32_t x1, int32_t y0, int32_t lift, uint32_t pace_idx,
    uint16_t col, uint16_t bead_dim, uint16_t bead_lit, int32_t prog256, int draw_inner)
{
    draw_seq_pace_arch_span(x0, x1, y0, lift, col, -1);
    if (draw_inner)
        draw_seq_pace_arch_span(x0, x1, y0 + 3, lift - 3, col, -1);
    draw_seq_pace_arch_beads(x0, x1, y0, lift, pace_idx, bead_dim, bead_lit, prog256);
    if (prog256 >= 0 && prog256 < 256) {
        int32_t span = x1 - x0;
        int32_t sx;
        int32_t sy;

        if (span != 0) {
            sx = x0 + (span * prog256) / 256;
            sy = seq_arch_parab_y(x0, x1, y0, lift, sx);
            cv_rect(sx - 2, sy - 2, 5, 5, C_WHITE);
            cv_rect(sx - 1, sy - 1, 3, 3, C_ACC);
        }
    }
}

static void draw_seq_stage(const perf_state_t *p)
{
    uint32_t h = ui_stage_h();
    uint32_t nsteps = perf_seq_nsteps(p->seq[0]);
    uint32_t draw_n = nsteps ? nsteps : 4u;
    uint32_t mut_pct = ((uint32_t)p->seq[3] * 100u) / 127u;
    uint32_t pace_idx = perf_seq_pace_idx(p->seq[1]);
    uint32_t pace = perf_seq_pace_ticks(p->seq[1]);
    static const uint8_t PACE_DIVS[7] = {1u, 1u, 1u, 1u, 2u, 4u, 8u};
    uint32_t n_divs = PACE_DIVS[pace_idx];
    char root_s[8];
    uint8_t root = perf_capture_root();
    int32_t y_base = 76;
    int32_t y_min = 44;
    int32_t y_max = 106;
    int32_t y_sky = 54;
    int32_t arch_lift = seq_pace_arch_lift(pace_idx);
    int32_t slot_w = perf_seq_slot_w(nsteps);
    int32_t bar_h = 8;
    uint16_t bar_col = RGB(50, 42, 120);
    uint16_t bar_dim = RGB(28, 24, 58);
    uint16_t arc_col = RGB(42, 36, 92);
    uint16_t bead_dim = RGB(64, 54, 130);
    uint32_t s;
    uint32_t cur = 0u;
    int32_t active_prog = -1;

    cv_begin(240, h, C_BLACK);
    note_name(root, root_s, 8u);
    if (nsteps != 0u && perf_seq_active(p) && g_st.playing && pace > 0u) {
        cur = p->seq_step_i % nsteps;
        active_prog = (int32_t)((uint32_t)p->seq_acc * 256u / pace);
        if (active_prog > 256)
            active_prog = 256;
    }
    if (nsteps != 0u) {
        for (s = 0; s < nsteps; s++) {
            uint32_t sn = (s + 1u) % nsteps;
            int32_t x0 = perf_seq_col_cx(s, nsteps);
            int32_t x1 = perf_seq_col_cx(sn, nsteps);
            int32_t prog = -1;
            uint16_t col = arc_col;

            if ((int32_t)s == (int32_t)cur && active_prog >= 0) {
                int32_t xs;

                prog = active_prog;
                draw_seq_pace_link(x0, x1, y_sky, arch_lift, pace_idx, arc_col, bead_dim, C_WHITE, prog,
                    pace_idx == 6u);
                xs = x0 + ((x1 - x0) * active_prog) / 256;
                draw_seq_pace_arch_span(x0, x1, y_sky, arch_lift, C_WHITE, xs);
                if (pace_idx == 6u)
                    draw_seq_pace_arch_span(x0, x1, y_sky + 3, arch_lift - 3, C_ACC, xs);
            } else {
                draw_seq_pace_link(x0, x1, y_sky, arch_lift, pace_idx, col, bead_dim, bead_dim, prog,
                    pace_idx == 6u);
            }
        }
        {
            int32_t x0 = perf_seq_col_cx(nsteps - 1u, nsteps);
            int32_t x1 = perf_seq_col_cx(0u, nsteps);
            int32_t wrap_lift = 6 + (int32_t)pace_idx * 2;

            draw_seq_pace_arch_span(x0, x1, y_base + 14, wrap_lift, RGB(24, 22, 48), -1);
        }
    }
    {
        int32_t x0 = perf_seq_col_x0(nsteps);
        int32_t x1 = x0 + slot_w * (int32_t)draw_n;

        cv_line(x0, y_base, x1, y_base, C_LINE);
    }
    for (s = 0; s < draw_n; s++) {
        int32_t gap = (draw_n >= 16u) ? 1 : 3;
        int32_t cx = perf_seq_col_cx(s, nsteps);
        int32_t bx = perf_seq_col_x0(nsteps) + (int32_t)s * slot_w + gap;
        int32_t bw = slot_w - 2 * gap;
        int32_t off = 0;
        int32_t y;
        int32_t cap_h;
        uint32_t d;

        if (nsteps != 0u) {
            perf_seq_prog_offset(p, s, &off);
            if (perf_seq_active(p) && g_st.playing && s == (p->seq_step_i % nsteps))
                off += (int32_t)p->seq_mut_off;
        }
        y = perf_seq_off_y(y_base, off, y_min, y_max);
        cap_h = bar_h;
        if (nsteps == 0u) {
            cv_rect(bx, y_base - cap_h / 2, bw, cap_h, bar_dim);
            continue;
        }
        cv_rect(bx, y - cap_h / 2, bw, cap_h, bar_col);
        if (bw > 5) {
            for (d = 1u; d < n_divs; d++) {
                int32_t tx = bx + (bw * (int32_t)d) / (int32_t)n_divs;

                cv_line(tx, y - cap_h / 2, tx, y + cap_h / 2, RGB(32, 28, 72));
            }
        }
        if (perf_seq_active(p) && g_st.playing) {
            uint32_t cur = p->seq_step_i % nsteps;
            int32_t fill_w;

            if (s == cur && pace > 0u) {
                fill_w = (bw * (int32_t)p->seq_acc) / (int32_t)pace;
                if (fill_w < 1 && p->seq_acc > 0u)
                    fill_w = 1;
                if (fill_w > bw)
                    fill_w = bw;
                cv_rect(bx, y - cap_h / 2, fill_w, cap_h, RGB(120, 100, 255));
                if (fill_w > 0)
                    cv_line(bx + fill_w - 1, y - cap_h / 2, bx + fill_w - 1, y + cap_h / 2, C_WHITE);
            }
            if (s == (p->seq_step_i % nsteps))
                cv_rect(bx - 1, y - cap_h / 2 - 2, bw + 2, cap_h + 4, C_WHITE);
        }
        if (s > 0u && mut_pct > 0u) {
            int32_t reach = 4 + (int32_t)(mut_pct * 8u / 100u);
            int32_t sh = 2 + (int32_t)(mut_pct / 25u);
            int32_t y_up = perf_seq_off_y(y_base, off + reach, y_min, y_max);
            int32_t y_dn = perf_seq_off_y(y_base, off - reach, y_min, y_max);
            uint16_t sh_col = RGB(42, 34, 88);
            int32_t lit = 0;
            int32_t y_mut;

            if (perf_seq_active(p) && g_st.playing && s == (p->seq_step_i % nsteps) && p->seq_mut_off != 0)
                lit = 1;
            cv_line(cx, y - cap_h / 2, cx, y_up + sh, sh_col);
            cv_line(cx, y + cap_h / 2, cx, y_dn - sh, sh_col);
            for (d = 0; d < (uint32_t)bw; d += 4u)
                cv_pset(bx + (int32_t)d, y_up, sh_col);
            cv_rect(bx, y_up - sh / 2, bw, sh, sh_col);
            for (d = 0; d < (uint32_t)bw; d += 4u)
                cv_pset(bx + (int32_t)d, y_dn, sh_col);
            cv_rect(bx, y_dn - sh / 2, bw, sh, sh_col);
            if (lit) {
                y_mut = perf_seq_off_y(y_base, off + (int32_t)p->seq_mut_off, y_min, y_max);
                cv_rect(bx, y_mut - sh / 2 - 1, bw, sh + 2, C_ACC);
                cv_line(bx, y_mut, bx + bw - 1, y_mut, C_WHITE);
            }
        }
    }
    cv_text(8, 4, &FONT_S, "Sequence", RGB(120, 100, 255));
    cv_text(180, 4, &FONT_S, root_s, C_AMB);
    cv_blit(0, 24);
}

static void draw_rec_ring_pt(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint32_t bucket, int32_t rad, int32_t *ox, int32_t *oy)
{
    uint32_t ph = bucket * 67403400u;
    int32_t sx = (fm2_sine(ph) * rx) >> 15;
    int32_t sy = (fm2_sine(ph + 1073741824u) * ry) >> 15;

    *ox = cx + sx + (sx * rad) / (rx > 8 ? rx : 8);
    *oy = cy - sy - (sy * rad) / (ry > 8 ? ry : 8);
}


static void draw_rec_stage(const perf_state_t *p)
{
    uint32_t h = ui_stage_h();
    uint32_t st = perf_rec_state(p);
    uint32_t tgt = ((uint32_t)p->rec[1] * 3u) / 128u;
    uint32_t bars = perf_rec_loop_bars(p->rec[2]);
    uint32_t b, i, seg;
    int32_t cx = 120;
    int32_t cy = 24 + (int32_t)h / 2;
    int32_t rx = 88;
    int32_t ry = 34;
    uint16_t ring_col = RGB(55, 22, 50);
    uint16_t wave_col = RGB(200, 80, 180);
    int32_t px0 = -1;
    int32_t py0 = -1;

    if (st > 4u)
        st = 4u;
    if (tgt > 2u)
        tgt = 2u;
    cv_begin(240, h, C_BLACK);
    for (seg = 0; seg < 64u; seg++) {
        int32_t ox;
        int32_t oy;

        draw_rec_ring_pt(cx, cy, rx, ry, seg, 0, &ox, &oy);
        if (px0 >= 0)
            cv_line(px0, py0, ox, oy, ring_col);
        px0 = ox;
        py0 = oy;
    }
    if (st >= 1u) {
        uint32_t bar_seg = 64u / (bars > 0u ? bars : 1u);

        for (b = 0; b < bars; b++) {
            int32_t ox;
            int32_t oy;

            draw_rec_ring_pt(cx, cy, rx, ry, b * bar_seg, 0, &ox, &oy);
            cv_pset(ox, oy, RGB(140, 60, 120));
        }
    }
    if ((tgt == 1u || tgt == 2u) && p->rec[3] < 100u) {
        uint32_t fade = 127u - (uint32_t)p->rec[3];

        for (b = 0; b < 64u; b++) {
            int32_t ox;
            int32_t oy;
            int32_t depth = (int32_t)(fade * (int32_t)b / 512);

            draw_rec_ring_pt(cx, cy, rx + depth / 3, ry + depth / 4, b, 0, &ox, &oy);
            cv_pset(ox, oy, RGB(90, 30, 70));
            draw_rec_ring_pt(cx, cy, rx - depth / 4, ry - depth / 5, b, 0, &ox, &oy);
            cv_pset(ox, oy, RGB(60, 20, 48));
        }
    }
    if (tgt == 1u || tgt == 2u) {
        int32_t prev_x = -1;
        int32_t prev_y = -1;

        for (b = 0; b < 64u; b++) {
            int32_t ox;
            int32_t oy;
            int32_t mv = (int32_t)p->motion_buf[b];
            int32_t rad = mv / 6;

            draw_rec_ring_pt(cx, cy, rx, ry, b, rad, &ox, &oy);
            if (prev_x >= 0)
                cv_line(prev_x, prev_y, ox, oy, st == 0u ? RGB(40, 16, 36) : wave_col);
            prev_x = ox;
            prev_y = oy;
        }
    }
    if (tgt == 0u || tgt == 2u) {
        uint32_t nshow = perf_seq_nsteps(p->seq[0]);

        if (nshow == 0u)
            nshow = PERF_SEQ_MAX_STEPS;
        if (nshow > PERF_SEQ_MAX_STEPS)
            nshow = PERF_SEQ_MAX_STEPS;
        for (i = 0; i < nshow; i++) {
            int32_t ox;
            int32_t oy;
            int32_t mv = (int32_t)p->seq_steps[i];
            int32_t rad = 8 + mv / 8;
            uint32_t seg = (i * 64u) / nshow;

            draw_rec_ring_pt(cx, cy, rx, ry, seg, rad, &ox, &oy);
            cv_rect(ox - 1, oy - 2, 3, 5, C_ACC);
        }
    }
    if (st >= 1u) {
        int32_t hx;
        int32_t hy;
        uint16_t head = C_WHITE;

        draw_rec_ring_pt(cx, cy, rx, ry, p->rec_bucket, 0, &hx, &hy);
        if (st == 1u)
            head = RGB(230, 60, 55);
        else if (st == 2u)
            head = RGB(235, 65, 60);
        else if (st == 3u)
            head = RGB(80, 230, 140);
        else if (st == 4u)
            head = RGB(230, 140, 255);
        cv_rect(hx - 2, hy - 2, 5, 5, head);
        cv_line(cx, cy, hx, hy, RGB(80, 40, 70));
    }
    cv_text(8, 4, &FONT_S, "Looper", RGB(200, 80, 180));
    {
        static const char *const RS[5] = {"Off", "Arm", "Rec", "Play", "Dub"};

        cv_text(180, 4, &FONT_S, RS[st > 4u ? 4u : st], C_AMB);
    }
    cv_blit(0, 24);
}

static int32_t dinghy_lfo_trace_sample(void *ctx, uint32_t ph, uint32_t cycle)
{
    uint32_t slot = (uint32_t)(uintptr_t)ctx;

    return fm_lfo_eval_full_q15(ph, cycle, slot, g_st.lfo_shape[slot], g_st.lfo_skew[slot],
                                g_st.lfo_algo[slot], g_st.lfo_ratio[slot], g_st.lfo_index[slot],
                                g_st.lfo_fb[slot], g_st.lfo_fold[slot]);
}

static void draw_lfo_stage(const midi_cfg_t *c, const midi_state_t *s)
{
    uint32_t target = midi_lfo_target_slot(c);
    uint32_t slots[4];
    uint32_t n_slots = 0;
    uint32_t i, si, h;
    char label[16];
    char rate_s[16];
    const char *nd;
    uint8_t rate = fm_lfo_rate_clamp(s->lfo_rate[target], s->bpm);

    for (i = 0; i < NLFO_TARGETS && n_slots < 3u; i++) {
        uint32_t slot_i = LFO_TGT[i];

        if (slot_i == target)
            continue;
        if (midi_lfo_get_depth(c, s, slot_i) > 0u)
            slots[n_slots++] = slot_i;
    }
    slots[n_slots++] = target;
    h = ui_stage_h();

    /* Bank stage is 116 px; the RYTM window (mid 66, y 8..124) only fits the home canvas. */
    int32_t mid = h >= 128u ? 66 : (int32_t)(h / 2u);
    int32_t y_hi = h >= 128u ? 124 : (int32_t)h - 2;

    cv_begin(240, h, C_BLACK);
    cv_line(8, mid, 231, mid, C_LINE);

    for (si = 0; si < n_slots; si++) {
        uint32_t slot = slots[si];
        int is_target = slot == target;
        int32_t depth =
            is_target ? (int32_t)c->base_val[P_LFO_DEPTH] : (int32_t)midi_lfo_get_depth(c, s, slot);
        uint64_t ph_off = (uint64_t)s->lfo_phase[slot] << 25;
        uint64_t now64 = (((uint64_t)s->lfo_cycle[slot] << 32) | (uint64_t)s->lfo_ph_slot[slot]) + ph_off;
        uint16_t col = is_target ? C_WHITE : C_DIM;

        if (!is_target && depth == 0)
            continue;
        fm_lfo_draw_trace((void *)(uintptr_t)slot, dinghy_lfo_trace_sample, now64, s->lfo_smooth[slot],
                          depth, 8, 224, mid, 48, 8, y_hi, col);
    }

    label[0] = '~';
    copy_str(label + 1, midi_param_name(g_cfg.mode, target));
    nd = fm_lfo_note_div(rate, s->bpm);
    if (nd)
        copy_str(rate_s, nd);
    else {
        uint32_t t = (fm_lfo_hz100(rate) + 5u) / 10u;
        uint32_t n = 0;

        u8_dec(rate_s, t / 10u);
        while (rate_s[n])
            n++;
        rate_s[n++] = '.';
        rate_s[n++] = (char)('0' + (t % 10u));
        rate_s[n++] = ' ';
        rate_s[n++] = 'H';
        rate_s[n++] = 'z';
        rate_s[n] = 0;
    }
    if (cv_home_view) {
        int32_t tw = text_w(&FONT_S, label);
        int32_t lx = (240 - tw) / 2;
        uint8_t ag = s->lfo_algo[target];

        cv_text(lx, 8, &FONT_S, label, C_AMB);
        cv_text(176, 8, &FONT_S, rate_s, C_DIM);
        if (ag != FM_LFO_ALG_CLASSIC && ag < FM_LFO_NALGOS)
            fm_lfo_draw_algo_badge(lx + tw + 4, 6, ag, C_DIM, C_WHITE, C_LINE);
    } else {
        int32_t tw = text_w(&FONT_S, label);
        uint8_t ag = s->lfo_algo[target];

        cv_text(8, 4, &FONT_S, label, C_AMB);
        cv_text(176, 4, &FONT_S, rate_s, C_DIM);
        if (ag != FM_LFO_ALG_CLASSIC && ag < FM_LFO_NALGOS)
            fm_lfo_draw_algo_badge(8 + tw + 4, 2, ag, C_DIM, C_WHITE, C_LINE);
    }
    cv_blit(0, 24);
}

static void draw_home_screen(void)
{
    /* Total-result screen (not a bank diagram, not the LFO scope):
     * top band = live FM spectrum/waveform + envelope + stereo/echo field;
     * mid band = mode's musical result (Fifths circle / Contura melody / Dit Dah cipher);
     * bottom band = live modulation matrix across all 16 parameters. */
    uint32_t x, v;
    uint32_t cm, mm_q8;
    int32_t timbre = (int32_t)g_cfg.val[P_TIMBRE];
    uint32_t drive = (uint32_t)g_cfg.val[P_DRIVE];
    int32_t det = (int32_t)g_cfg.val[P_DETUNE] - 64;
    uint32_t echo_mix = (uint32_t)g_cfg.val[P_ECHO_MIX];
    uint32_t echo_t = (uint32_t)g_cfg.val[P_ECHO_TIME];
    int32_t echo_dx = 3 + (int32_t)(echo_t * 10u) / 127;
    int32_t env_sum = 0, mod_sum = 0;
    uint16_t col_wave, col_echo;
    int32_t py_l = -1, py_r = -1, py_e = -1;
    uint32_t ph0 = fm1_ms * 11000000u;
    int hud_on = fm1_ms < g_home_hud_until;

    midi_ratio_cm_q8(g_cfg.val[P_RATIO], &cm, &mm_q8);
    for (v = 0; v < NV; v++) {
        env_sum += voice[v].amp;
        mod_sum += voice[v].mod_env;
    }
    if (env_sum > 32767)
        env_sum = 32767;
    if (mod_sum > 32767)
        mod_sum = 32767;
    /* Keep a living idle breath when no note is held so the sound's timbre/ratio/drive is still readable */
    if (env_sum < 6000)
        env_sum = 6000;
    if (mod_sum < 8000)
        mod_sum = 8000;

    col_wave = RGB(120u + (drive * 135u) / 127u, 210u - (drive * 90u) / 127u, 255u - (drive * 180u) / 127u);
    col_echo = RGB((36u * echo_mix) / 127u, (148u * echo_mix) / 127u, (210u * echo_mix) / 127u);

    cv_begin(240, 216, C_BLACK);

    /* ---- 1. LIVE SOUND FIELD (y = 4..52): actual FM carrier+modulator+feedback+drive+detune+echo ---- */
    cv_line(8, 28, 231, 28, C_LINE);
    {
        int32_t fm_depth = (timbre * (timbre + 32)) >> 4;
        int32_t fb_amt = timbre > 40 ? (timbre - 40) * 2 : 0;
        int32_t amp_px = 6 + (env_sum * 16) / 32767;
        int32_t fb_l = 0;

        for (x = 8; x < 232u; x++) {
            uint32_t t = x - 8u;
            uint32_t c_ph = ph0 + t * 38000000u * cm;
            uint32_t c_ph_r = c_ph + (uint32_t)((int32_t)t * det * 180000);
            uint32_t m_ph = (ph0 >> 1) + ((t * 38000000u) >> 8) * mm_q8;
            int32_t mod = fm2_sine(m_ph + (uint32_t)((fb_l * fb_amt) << 9));
            int32_t pmod, sl, sr, yl, yr;

            fb_l = mod;
            mod = (mod * mod_sum) >> 15;
            pmod = (mod * fm_depth) << 11;
            sl = fm2_soft_clip((fm2_sine(c_ph + (uint32_t)pmod) * 200) >> 8, drive);
            sr = fm2_soft_clip((fm2_sine(c_ph_r + (uint32_t)pmod) * 200) >> 8, drive);

            yl = 28 - (sl * amp_px) / 32768;
            yr = 28 - (sr * amp_px) / 32768;
            if (yl < 3) yl = 3;
            if (yl > 53) yl = 53;
            if (yr < 3) yr = 3;
            if (yr > 53) yr = 53;

            if (echo_mix > 4u && x >= (uint32_t)(8 + echo_dx)) {
                int32_t ye = yl + 2;
                if (py_e >= 0)
                    cv_line((int32_t)x - 1, py_e, (int32_t)x, ye, col_echo);
                py_e = ye;
            }
            if (det != 0 && py_r >= 0)
                cv_line((int32_t)x - 1, py_r, (int32_t)x, yr, C_ACC);
            if (py_l >= 0)
                cv_line((int32_t)x - 1, py_l, (int32_t)x, yl, col_wave);
            py_l = yl;
            py_r = yr;
        }
    }

    /* ---- 2. MODE HARMONY / MELODY / CIPHER RESULT ---- */
    if (g_cfg.mode == MODE_TRIAXIS) {
        const char *chord = g_triptych.chord_name[0] ? g_triptych.chord_name : "---";
        const char *badge = ct_func_badge(&g_triptych);
        int32_t echo_d = 2 + ((int32_t)echo_t * 7) / 127;
        int32_t det_px = det / 22;
        uint16_t c_drv = ct_drive_col((uint8_t)drive, 0u);

        /* Large round 1.5x circle (diameter 102px) centered at (120, 108) with breathing room */
        cv_origin(57, 42);
        cv_set_zoom(2u);
        if (echo_mix > 2u) {
            uint32_t root_pc = g_triptych.root % 12u;
            uint32_t i0 = ct_pc_to_cof((root_pc + ct_center_offset(0, g_triptych.interval)) % 12u);
            uint32_t i1 = ct_pc_to_cof((root_pc + ct_center_offset(1, g_triptych.interval)) % 12u);
            uint32_t i2 = ct_pc_to_cof((root_pc + ct_center_offset(2, g_triptych.interval)) % 12u);
            cv_line(COF_X[i0] + echo_d, COF_Y[i0] + echo_d / 2, COF_X[i1] + echo_d, COF_Y[i1] + echo_d / 2, col_echo);
            cv_line(COF_X[i1] + echo_d, COF_Y[i1] + echo_d / 2, COF_X[i2] + echo_d, COF_Y[i2] + echo_d / 2, col_echo);
            cv_line(COF_X[i2] + echo_d, COF_Y[i2] + echo_d / 2, COF_X[i0] + echo_d, COF_Y[i0] + echo_d / 2, col_echo);
        }
        triptych_draw_triangle(&g_triptych, &g_cfg, 0, 0);
        cv_set_zoom(1u);
        cv_origin(0, 0);

        /* Minimal small typography in the lower corners around the large cycle */
        if (!hud_on) {
            if (echo_mix > 2u)
                cv_text(8 + echo_d / 2, 152 + echo_d / 3, &FONT_S, chord, col_echo);
            if (det_px != 0)
                cv_text(8 + det_px, 152, &FONT_S, chord, ct_drive_col((uint8_t)drive, 1u));
            cv_text(8, 152, &FONT_S, chord, drive > 32u ? c_drv : C_WHITE);
            cv_text(232 - text_w(&FONT_S, badge), 152, &FONT_S, badge, C_ACC);
        }
    } else if (g_cfg.mode == MODE_CONTURA) {
        uint32_t i;
        uint32_t plen = g_contura.phrase_len >= 3u ? g_contura.phrase_len : 8u;
        uint32_t pool_max = g_contura.pool_len > 1u ? g_contura.pool_len - 1u : 1u;
        uint32_t lay_mask = contura_layer_mask(g_cfg.val[P_VEL_MOD]);
        int has_h = (int)(lay_mask & CONTURA_V_HIGH);
        int has_b = (int)(lay_mask & CONTURA_V_BASS);
        int has_s = (int)(lay_mask & CONTURA_V_SOLO);
        int has_c = (int)(lay_mask & CONTURA_V_MIRR);
        uint8_t swing = g_cfg.val[P_SWING];
        uint8_t decay = g_cfg.val[P_DECAY];
        int32_t det_px = ((int32_t)g_cfg.val[P_DETUNE] - 64) / 18;
        uint32_t thick = 1u + (drive * 2u) / 96u;
        uint32_t tail_steps = 1u + ((uint32_t)decay * 3u) / 127u;
        uint16_t c_drv_hi = contura_drive_col((uint8_t)drive, 0u);
        uint16_t c_drv_dim = contura_drive_col((uint8_t)drive, 1u);
        int32_t echo_d = 2 + ((int32_t)echo_t * 7) / 127;
        uint16_t ecol1 = contura_echo_col((uint8_t)echo_mix, 1u);
        uint16_t ecol2 = contura_echo_col((uint8_t)echo_mix, 2u);
        uint16_t gate = contura_density_gate(g_cfg.val[P_DENSITY], &CONTURA_SCALE_FEELS[g_contura.scale % 20u]);
        int32_t px = -1, py = -1, pcx = -1, pcy = -1;
        const char *label = CONTURA_SCALE_LABELS[g_contura.scale % 20u];
        const char *ltxt = CONTURA_LAYER_LABELS[contura_layer_idx(g_cfg.val[P_VEL_MOD])];
        int32_t bar_w = (int32_t)(160u / (plen > 1u ? plen : 1u));

        if (bar_w < 4)
            bar_w = 4;
        if (bar_w > 16)
            bar_w = 16;

        cv_line(8, 78, 231, 78, C_LINE);
        cv_line(8, 128, 231, 128, C_LINE);
        cv_line(8, 148, 231, 148, C_LINE);

        for (i = 0; i < plen; i++) {
            uint32_t idx = g_contura.motif[i % 28u];
            int32_t sx = contura_step_x(i, plen, swing);
            int is_on = contura_step_active(i, gate);
            uint8_t is_now = (i == g_contura.phrase_pos);
            uint32_t age = (g_contura.phrase_pos >= i) ? (g_contura.phrase_pos - i) : (plen + g_contura.phrase_pos - i);
            int in_tail = (age > 0u && age <= tail_steps);
            int32_t sy;

            if (idx > pool_max)
                idx = pool_max;

            if (is_now) {
                int32_t y0 = has_h ? 58 : 79;
                int32_t y1 = has_b ? 147 : 127;

                cv_line(sx, y0, sx, y1, C_LINE);
            }

            if (has_h) {
                int32_t vh = 4 + (int32_t)(((idx * 3u + i * 5u) % 13u));
                int32_t vy = 76 - vh;
                uint16_t vcol = !is_on ? C_DIM : (is_now ? C_WHITE : (in_tail ? c_drv_hi : c_drv_dim));

                if (is_on || has_h) {
                    if (is_on && echo_mix > 2u) {
                        int32_t ex = sx + echo_d, ey = vy + echo_d / 2;

                        if (ex < 236 && ey < 77)
                            cv_line(ex, 76, ex, ey, ecol1);
                    }
                    if (det_px != 0 && is_on)
                        cv_line(sx + det_px, 76, sx + det_px, vy, C_DIM);
                    cv_line(sx, 76, sx, vy, vcol);
                    if (thick >= 2u && is_on)
                        cv_line(sx + 1, 76, sx + 1, vy, vcol);
                    if (is_on)
                        cv_rect(sx - 1, vy, 3, 2, is_now ? C_ACC : vcol);
                }
            }

            if (has_s) {
                sy = contura_lead_y_home(idx, pool_max);

                if (px >= 0) {
                    if (echo_mix > 2u && is_on) {
                        if (echo_mix > 48u && sx + echo_d * 2 < 236)
                            cv_line(px + echo_d * 2, py + echo_d, sx + echo_d * 2, sy + echo_d, ecol2);
                        if (sx + echo_d < 236)
                            cv_line(px + echo_d, py + (echo_d * 2) / 3, sx + echo_d, sy + (echo_d * 2) / 3, ecol1);
                    }
                    if (det_px != 0 && is_on)
                        cv_line(px, py + det_px, sx, sy + det_px, c_drv_dim);
                    contura_thick_line(px, py, sx, sy,
                                       !is_on ? C_LINE : (is_now || in_tail ? c_drv_hi : c_drv_dim), thick);
                }
                if (is_now && is_on) {
                    int32_t hr = 3 + (timbre * 4) / 127;

                    if (echo_mix > 2u && sx + echo_d < 236)
                        cv_rect(sx + echo_d - 2, sy + (echo_d * 2) / 3 - 2, 5, 5, ecol1);
                    if (timbre > 24) {
                        cv_line(sx - hr - 1, sy, sx + hr + 1, sy, c_drv_hi);
                        cv_line(sx, sy - hr - 1, sx, sy + hr + 1, c_drv_hi);
                    }
                    cv_rect(sx - 3, sy - 3, 7, 7, C_ACC);
                    cv_rect(sx - 1, sy - 1, 3, 3, C_WHITE);
                } else if (is_on) {
                    if (echo_mix > 2u && sx + echo_d < 236)
                        cv_rect(sx + echo_d - 1, sy + (echo_d * 2) / 3 - 1, 2, 2, ecol1);
                    if (in_tail) {
                        int32_t sz = (age == 1u && decay > 48u) ? 5 : 3;

                        cv_rect(sx - sz / 2, sy - sz / 2, sz, sz, age == 1u ? C_WHITE : c_drv_hi);
                    } else {
                        cv_rect(sx - 1, sy - 1, 3, 3, c_drv_dim);
                    }
                } else {
                    cv_pset(sx, sy, C_LINE);
                }
                if (is_on) {
                    px = sx;
                    py = sy;
                }
            }

            if (has_c) {
                uint32_t c_step = (i + plen - 2u) % plen;
                uint32_t c_idx = g_contura.motif[c_step % 28u];
                int32_t cy;
                uint16_t c_vio = contura_canon_col();
                uint16_t c_tr = contura_canon_trail_col();

                if (c_idx > pool_max)
                    c_idx = pool_max;
                cy = contura_canon_y_home(c_idx, pool_max);

                if (pcx >= 0) {
                    int in_canon_trail = (age > 0u && age <= 5u);

                    if (in_canon_trail) {
                        cv_line(pcx, pcy, sx, cy, c_tr);
                        cv_line(pcx, pcy + 1, sx, cy + 1, c_tr);
                    } else {
                        cv_line(pcx, pcy, sx, cy, c_vio);
                    }
                }
                if (is_now) {
                    cv_rect(sx - 2, cy - 2, 5, 5, c_tr);
                    cv_rect(sx - 1, cy - 1, 3, 3, C_WHITE);
                } else if (age > 0u && age <= 5u) {
                    cv_rect(sx - 1, cy - 1, 3, 3, c_tr);
                }
                pcx = sx;
                pcy = cy;
            }

            if (has_b) {
                uint32_t deg_mod = (i == 0u) ? 0u : ((idx + i) % 3u);
                int32_t by = deg_mod == 0u ? 142 : (deg_mod == 1u ? 137 : 132);
                uint16_t bcol = !is_on ? C_DIM : (is_now ? C_WHITE : (in_tail ? c_drv_hi : c_drv_dim));
                int32_t bx = sx - bar_w / 2;

                if (is_on && echo_mix > 2u) {
                    int32_t ex = bx + echo_d, ey = by + echo_d / 2;

                    if (ex + bar_w < 236 && ey < 147)
                        cv_rect(ex, ey, bar_w, 2, ecol1);
                }
                if (det_px != 0 && is_on)
                    cv_line(bx, by + det_px, bx + bar_w, by + det_px, c_drv_dim);
                cv_rect(bx, by, bar_w, (int32_t)thick + (is_now ? 2 : 1), bcol);
                if (is_now)
                    cv_rect(bx, by, bar_w, 2, C_ACC);
            }
        }
        if (!hud_on) {
            cv_text(8, 152, &FONT_S, label, C_HI);
            cv_text(232 - text_w(&FONT_S, ltxt), 152, &FONT_S, ltxt, C_AMB);
        }
    } else {
        char letter[2];
        char phrase[44];
        uint32_t i;
        int32_t tx = 72, ty = 74;
        uint8_t decay = g_cfg.val[P_DECAY];
        int32_t det_px = det / 18;
        int32_t drv_add = ((int32_t)drive * 3) / 127;
        uint32_t tail_ev = 1u + ((uint32_t)decay * 5u) / 127u;
        uint16_t c_drv_hi = ditdah_drive_col((uint8_t)drive, 0u);
        uint16_t c_drv_dim = ditdah_drive_col((uint8_t)drive, 1u);
        int32_t echo_d = 2 + ((int32_t)echo_t * 6) / 127;

        ditdah_phrase_text(&g_ditdah, &g_cfg, phrase, sizeof phrase);
        letter[0] = g_ditdah.cur_char ? (char)g_ditdah.cur_char : ' ';
        letter[1] = 0;
        if (echo_mix > 2u)
            cv_text(8 + echo_d, 64 + echo_d, &FONT_L, letter, col_echo);
        if (det_px != 0)
            cv_text(8 + det_px, 64, &FONT_L, letter, c_drv_dim);
        cv_text(8, 64, &FONT_L, letter, drive > 32u ? c_drv_hi : C_WHITE);

        for (i = 0; g_ditdah.ticker[i] && tx < 228; i++) {
            char sym = g_ditdah.ticker[i];
            uint16_t ev_i = ditdah_ticker_ev[i];
            uint32_t age = (g_ditdah.ev_pos >= ev_i) ? (uint32_t)(g_ditdah.ev_pos - ev_i) : 999u;
            int in_tail = (age > 0u && age <= tail_ev);
            uint16_t col = (ev_i == g_ditdah.ev_pos) ? C_WHITE : (in_tail ? c_drv_hi : (ev_i < g_ditdah.ev_pos ? C_DIM : c_drv_dim));
            int32_t hr = (timbre * 10) / 127;

            if (sym == '.') {
                if (echo_mix > 2u)
                    cv_rect(tx + echo_d, ty + 8 + echo_d, 10, 12 + drv_add, col_echo);
                if (det_px != 0)
                    cv_rect(tx + det_px, ty + 8, 10, 12, C_LINE);
                cv_rect(tx, ty + 6 - drv_add / 2, 10, 16 + drv_add, col);
                if (ev_i == g_ditdah.ev_pos) {
                    if (hr > 0)
                        cv_line(tx + 5, ty + 4 - hr, tx + 5, ty + 24 + hr, c_drv_hi);
                    cv_rect(tx, ty + 26, 10, 4, C_ACC);
                }
                tx += 16;
            } else if (sym == '-') {
                if (echo_mix > 2u)
                    cv_rect(tx + echo_d, ty + 10 + echo_d, 26, 8 + drv_add, col_echo);
                if (det_px != 0)
                    cv_rect(tx + det_px, ty + 10, 26, 8, C_LINE);
                cv_rect(tx, ty + 8 - drv_add / 2, 26, 12 + drv_add, col);
                if (ev_i == g_ditdah.ev_pos) {
                    if (hr > 0)
                        cv_line(tx + 13, ty + 6 - hr, tx + 13, ty + 22 + hr, c_drv_hi);
                    cv_rect(tx, ty + 26, 26, 4, C_ACC);
                }
                tx += 32;
            } else {
                tx += 12;
            }
        }
        cv_line(8, 114, 232, 114, C_LINE);
        {
            uint32_t play_k = g_ditdah.ev_len ? ditdah_ev_idx[g_ditdah.ev_pos % g_ditdah.ev_len] : 0u;
            uint32_t k = play_k > 12u ? (play_k - 12u) : 0u;
            char ch_s[2] = {0, 0};
            int32_t lx = 8;

            while (phrase[k] && lx < 228) {
                int32_t cw;
                ch_s[0] = phrase[k];
                cw = text_w(&FONT_S, ch_s);
                if (cw < 8)
                    cw = 8;
                if (k == play_k && g_ditdah.ev_len) {
                    cv_rect(lx - 1, 122, cw + 2, 26, C_LINE);
                    cv_text(lx, 128, &FONT_S, ch_s, C_WHITE);
                    cv_rect(lx, 146, cw, 3, C_ACC);
                } else {
                    cv_text(lx, 128, &FONT_S, ch_s, k < play_k ? c_drv_dim : c_drv_hi);
                }
                lx += cw;
                k++;
            }
        }
    }

    if (hud_on)
        draw_home_hud_band();

    /* ---- 3. HOME matrix (y = 172..210): FX · SEL · ENV · GLO · ARP · SEQ (6×4 bars) ---- */
    cv_line(8, 170, 231, 170, C_LINE);
    {
        static const uint16_t GRP_COL[4] = {
            RGB(75, 220, 255),
            RGB(85, 190, 255),
            RGB(95, 160, 255),
            RGB(110, 130, 255)
        };
        static const uint16_t GRP_BG[4] = {
            RGB(8, 24, 34),
            RGB(9, 20, 36),
            RGB(10, 17, 38),
            RGB(12, 14, 40)
        };
        static const uint16_t PERF_COL[2] = {
            RGB(70, 210, 235),
            RGB(130, 145, 255)
        };
        static const uint16_t PERF_BG[2] = {
            RGB(8, 22, 30),
            RGB(14, 15, 38)
        };
        static const uint8_t GRP_BANK[4] = {BANK_SPACE, BANK_ENGINE, BANK_SYNTH, BANK_TIME};
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        uint32_t grp, c;
        int arp_pulse = perf_arp_active(&g_perf) && g_st.playing && g_perf.arp_gate_left > 0u;
        uint32_t seq_hi = 255u;

        if (perf_seq_active(&g_perf) && g_st.playing)
            seq_hi = (g_perf.seq_step_i % 4u);

        for (grp = 0; grp < 6u; grp++) {
            int32_t gx = 8 + (int32_t)grp * 38;
            uint16_t fg, bg;

            if (grp < 4u) {
                fg = GRP_COL[grp];
                bg = GRP_BG[grp];
            } else if (grp == 4u) {
                fg = PERF_COL[0];
                bg = PERF_BG[0];
            } else {
                fg = PERF_COL[1];
                bg = PERF_BG[1];
            }
            for (c = 0; c < 4u; c++) {
                int32_t bx = gx + (int32_t)c * 9;
                int32_t bh, vh;
                int mod = 0;

                if (grp < 4u) {
                    uint32_t slot = LFO_TGT[grp * 4u + c];

                    bh = ((int32_t)g_cfg.base_val[slot] * 32) / 127;
                    vh = ((int32_t)g_cfg.val[slot] * 32) / 127;
                    mod = midi_lfo_get_depth(&g_cfg, &g_st, slot) > 0u;
                    cv_rect(bx, 176, 7, 32, bg);
                    if (vh > 0)
                        cv_rect(bx, 176 + 32 - vh, 7, vh, fg);
                    if (mod)
                        cv_rect(bx, 176 + 32 - vh, 7, 3, C_ACC);
                    if (bh > 0)
                        cv_rect(bx, 176 + 32 - bh, 7, 2, C_WHITE);
                    if (g_perf.page == PERF_PAGE_BANK && g_cfg.bank == BANK_LFO && slot == ts)
                        cv_rect(bx, 211, 7, 2, C_ACC);
                } else if (grp == 4u) {
                    uint32_t vslot = VSLOT_ARP_BASE + c;

                    bh = ((int32_t)g_perf.arp_base[c] * 32) / 127;
                    vh = ((int32_t)g_perf.arp[c] * 32) / 127;
                    mod = midi_lfo_get_depth(&g_cfg, &g_st, vslot) > 0u;
                    cv_rect(bx, 176, 7, 32, bg);
                    if (vh > 0)
                        cv_rect(bx, 176 + 32 - vh, 7, vh, fg);
                    if (mod)
                        cv_rect(bx, 176 + 32 - vh, 7, 3, C_ACC);
                    if (bh > 0)
                        cv_rect(bx, 176 + 32 - bh, 7, 2, C_WHITE);
                    if (arp_pulse && vh > 0)
                        cv_rect(bx, 176 + 32 - vh, 7, 2, RGB(180, 240, 255));
                    if (g_perf.page == PERF_PAGE_BANK && g_cfg.bank == BANK_LFO && ts == vslot)
                        cv_rect(bx, 211, 7, 2, C_ACC);
                } else {
                    uint32_t vslot = VSLOT_SEQ_BASE + c;

                    bh = ((int32_t)g_perf.seq_base[c] * 32) / 127;
                    vh = ((int32_t)g_perf.seq[c] * 32) / 127;
                    mod = midi_lfo_get_depth(&g_cfg, &g_st, vslot) > 0u;
                    cv_rect(bx, 176, 7, 32, bg);
                    if (vh > 0)
                        cv_rect(bx, 176 + 32 - vh, 7, vh, fg);
                    if (mod)
                        cv_rect(bx, 176 + 32 - vh, 7, 3, C_ACC);
                    if (bh > 0)
                        cv_rect(bx, 176 + 32 - bh, 7, 2, C_WHITE);
                    if (perf_seq_active(&g_perf) && g_st.playing && c == seq_hi && vh > 0)
                        cv_rect(bx, 176 + 32 - vh, 7, 2, C_WHITE);
                    if (g_perf.page == PERF_PAGE_BANK && g_cfg.bank == BANK_LFO && ts == vslot)
                        cv_rect(bx, 211, 7, 2, C_ACC);
                }
            }
        }
        if (g_perf.page == PERF_PAGE_BANK) {
            for (x = 0; x < 4u; x++) {
                if (g_cfg.bank == GRP_BANK[x])
                    cv_rect(8 + (int32_t)x * 38, 211, 34, 2, GRP_COL[x]);
            }
        } else if (g_perf.page == PERF_PAGE_ARP)
            cv_rect(8 + 4 * 38, 211, 34, 2, PERF_COL[0]);
        else if (g_perf.page == PERF_PAGE_SEQ)
            cv_rect(8 + 5 * 38, 211, 34, 2, PERF_COL[1]);
    }

    cv_blit(0, 24);
}

static void draw_stage_idle(void)
{
    if (g_home_view) {
        draw_home_screen();
        return;
    }
    if (g_perf.page == PERF_PAGE_ARP) {
        draw_arp_stage(&g_perf);
        return;
    }
    if (g_perf.page == PERF_PAGE_SEQ) {
        draw_seq_stage(&g_perf);
        return;
    }
    if (g_perf.page == PERF_PAGE_REC) {
        draw_rec_stage(&g_perf);
        return;
    }
    if (g_cfg.bank == BANK_LFO) {
        draw_lfo_stage(&g_cfg, &g_st);
        return;
    }
    if (g_cfg.mode == MODE_TRIAXIS)
        triptych_draw_stage(&g_triptych, &g_cfg);
    else if (g_cfg.mode == MODE_CONTURA)
        contura_draw_stage(&g_contura, &g_cfg);
    else
        ditdah_draw_stage(&g_ditdah, &g_cfg);
}

static int bank_confirm_active(void)
{
    return fm1_ms < g_confirm_until;
}

static void bank_confirm_close(void)
{
    g_confirm_until = 0u;
    g_ui_dirty = 1u;
}

static void bank_confirm_open(uint8_t kind, uint8_t id)
{
    g_confirm_kind = kind;
    g_confirm_id = id;
    g_confirm_until = fm1_ms + 5000u;
    g_ui_dirty = 1u;
}

static const char *bank_confirm_title(void)
{
    static const char *const BANK_LBL[NBANKS] = {"ENV", "SEL", "GLO", "FX", "LFO"};
    static const char *const PERF_LBL[4] = {"BANK", "ARP", "SEQ", "REC"};

    if (g_confirm_kind == 2u)
        return "ALL";
    if (g_confirm_kind == 0u)
        return BANK_LBL[g_confirm_id % NBANKS];
    return PERF_LBL[g_confirm_id % 4u];
}

static int bank_confirm_has_random(void)
{
    return !(g_confirm_kind == 1u && g_confirm_id == PERF_PAGE_REC);
}

static void bank_confirm_draw_arrow_left(int32_t by, int32_t bh)
{
    int32_t dy;
    const uint16_t fill = RGB(150, 38, 38);
    const uint16_t edge = RGB(235, 75, 70);

    for (dy = 0; dy < bh; dy++) {
        int32_t dist = dy < 13 ? (12 - dy) : (dy - 13);
        int32_t x_tip = 14 + dist;
        int32_t w = 78 - x_tip + 1;

        if (w < 1)
            continue;
        cv_rect(x_tip, by + dy, w, 1, fill);
        if (w >= 2)
            cv_rect(x_tip, by + dy, 2, 1, edge);
    }
    {
        const char *lab = "Reset";
        int32_t lw = (int32_t)text_w(&FONT_S, lab);

        cv_text(52 - lw / 2, by + 5, &FONT_S, lab, C_WHITE);
    }
}

static void bank_confirm_draw_arrow_right(int32_t by, int32_t bh)
{
    int32_t dy;
    const uint16_t fill = RGB(30, 125, 48);
    const uint16_t edge = RGB(75, 220, 90);

    for (dy = 0; dy < bh; dy++) {
        int32_t dist = dy < 13 ? (12 - dy) : (dy - 13);
        int32_t x_tip = 226 - dist;
        int32_t w = x_tip - 162 + 1;

        if (w < 1)
            continue;
        cv_rect(162, by + dy, w, 1, fill);
        if (w >= 2)
            cv_rect(x_tip - 1, by + dy, 2, 1, edge);
    }
    cv_text(164, by + 5, &FONT_S, "Random", C_WHITE);
}

static void bank_confirm_draw_knob_icon(int32_t x0, int32_t y0)
{
    int32_t cx = x0 + 3;
    int32_t cy = y0 + 3;
    int32_t x;
    int32_t y;

    for (x = -3; x <= 3; x++) {
        for (y = -3; y <= 3; y++) {
            int32_t d2 = x * x + y * y;

            if (d2 >= 5 && d2 <= 10)
                cv_pset(cx + x, cy + y, C_AMB);
        }
    }
    cv_pset(cx, cy - 3, C_AMB);
    cv_pset(cx, cy - 2, C_AMB);
}

static void param_slot_side_effects(uint32_t slot)
{
    if (slot == (uint32_t)P_LFO_TARGET) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);

        g_cfg.base_val[P_LFO_DEPTH] = midi_lfo_get_depth(&g_cfg, &g_st, ts);
        g_cfg.val[P_LFO_DEPTH] = g_cfg.base_val[P_LFO_DEPTH];
        midi_lfo_recall_target_rate(&g_cfg, &g_st);
    } else if (slot == (uint32_t)P_LFO_DEPTH) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);

        midi_lfo_set_depth(&g_cfg, &g_st, ts, g_cfg.base_val[P_LFO_DEPTH]);
    } else if (slot == (uint32_t)P_LFO_RATE) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);

        g_st.lfo_rate[ts] = fm_lfo_rate_clamp(g_cfg.base_val[P_LFO_RATE], g_st.bpm);
    } else if (slot == (uint32_t)P_LFO_SHAPE) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);

        g_st.lfo_shape[ts] = g_cfg.base_val[P_LFO_SHAPE];
    }
    if (slot == (uint32_t)P_TEMPO && !g_st.ext_sync)
        g_st.bpm = (uint16_t)(60u + g_cfg.val[P_TEMPO]);
    if (slot == (uint32_t)P_PARAM_B && g_cfg.mode == MODE_DITDAH) {
        ditdah_select_phrase(&g_ditdah, &g_cfg);
        ditdah_recompile(&g_ditdah, &g_cfg);
    }
    if (g_cfg.mode == MODE_CONTURA &&
        (slot == (uint32_t)P_PARAM_A || slot == (uint32_t)P_PARAM_B || slot == (uint32_t)P_DENSITY))
        contura_tick(&g_contura, &g_cfg, 0);
    if (g_cfg.mode == MODE_TRIAXIS && slot >= (uint32_t)P_PARAM_A && slot <= (uint32_t)P_VEL_MOD)
        triptych_tick(&g_triptych, &g_cfg, 0);
}

static void bank_confirm_slot_side_effects(uint32_t slot)
{
    param_slot_side_effects(slot);
    midi_mod_hold_arm(&g_cfg, &g_st, slot, fm1_ms);
    midi_send(0xB0, g_cfg.cc_map[slot], g_cfg.base_val[slot]);
}

static uint8_t perf_random_active_bucket(midi_state_t *s, uint32_t n, uint8_t cur)
{
    uint32_t cur_b = ((uint32_t)cur * n) / 128u;
    uint32_t b;

    if (cur_b >= n)
        cur_b = n - 1u;
    if (cur_b == 0u)
        b = 1u + (midi_rng_u7(s) % (n - 1u));
    else {
        b = midi_rng_u7(s) % (n - 1u);
        if (b >= cur_b - 1u)
            b++;
        b += 1u;
    }
    return (uint8_t)((b * 128u + 64u) / n);
}

static void perf_randomize_seq_steps(perf_state_t *p, midi_state_t *s)
{
    static const int8_t SEMI[11] = {-7, -5, -4, -3, -2, 0, 2, 3, 5, 7, 12};
    uint32_t i;

    for (i = 0; i < PERF_SEQ_MAX_STEPS; i++)
        p->seq_steps[i] = SEMI[midi_rng_u7(s) % 11u];
}

static void bank_confirm_random_arp(midi_state_t *s)
{
    uint32_t k;

    for (k = 0; k < 4u; k++) {
        uint8_t v;

        if (k == 0u)
            v = perf_random_active_bucket(s, 10u, g_perf.arp_base[0]);
        else if (k == 1u || k == 2u) {
            uint32_t b = midi_rng_u7(s) % 6u;

            v = (uint8_t)((b * 128u + 64u) / 6u);
        } else
            v = (uint8_t)(16u + (midi_rng_u7(s) % 105u));
        g_perf.arp_base[k] = v;
        g_perf.arp[k] = v;
        midi_mod_hold_arm(&g_cfg, &g_st, VSLOT_ARP_BASE + k, fm1_ms);
        midi_send(0xB0, (uint32_t)PERF_CC_ARP_MODE + k, v);
    }
    perf_lfo_apply(&g_perf, &g_cfg, &g_st);
}

static void bank_confirm_random_seq(midi_state_t *s, int always_steps)
{
    uint32_t k;
    uint8_t prog_v = 0u;

    for (k = 0; k < 4u; k++) {
        uint8_t v;

        if (k == 0u)
            v = perf_random_active_bucket(s, 8u, g_perf.seq_base[0]);
        else if (k == 1u) {
            uint32_t b = midi_rng_u7(s) % 7u;

            v = (uint8_t)((b * 128u + 64u) / 7u);
        } else if (k == 2u) {
            uint32_t b = midi_rng_u7(s) % 8u;

            v = (uint8_t)((b * 128u + 64u) / 8u);
            prog_v = v;
        } else
            v = midi_rng_u7(s);
        g_perf.seq_base[k] = v;
        g_perf.seq[k] = v;
        midi_mod_hold_arm(&g_cfg, &g_st, VSLOT_SEQ_BASE + k, fm1_ms);
        midi_send(0xB0, (uint32_t)PERF_CC_SEQ_STEPS + k, v);
    }
    if (always_steps || ((uint32_t)prog_v * 8u) / 128u >= 7u)
        perf_randomize_seq_steps(&g_perf, s);
    perf_lfo_apply(&g_perf, &g_cfg, &g_st);
    if (perf_seq_active(&g_perf))
        perf_seq_step_advance(&g_perf);
}

static void bank_confirm_apply_reset(void)
{
    uint32_t k;
    uint32_t j;

    if (g_confirm_kind == 2u) {
        for (k = 0; k < 4u; k++) {
            midi_bank_copy_preset_slots(&g_cfg, (uint8_t)k, g_cfg.mode);
            for (j = 0; j < 4u; j++)
                param_slot_side_effects((uint32_t)k * 4u + j);
        }
        midi_lfo_bank_reset(&g_cfg, &g_st);
        mode_mem_preset_all_modes();
        mode_mem_save(g_cfg.mode);
        perf_arp_release_extra();
        perf_arp_reset_values(&g_perf);
        if (g_perf.seq_root_captured) {
            perf_apply_root(g_perf.seq_base_saved);
            g_perf.seq_root_captured = 0u;
        }
        perf_seq_reset_meta(&g_perf);
        perf_rec_reset_values(&g_perf);
        mark_cfg_dirty();
        g_ui_dirty = 1u;
        return;
    }
    if (g_confirm_kind == 0u) {
        if (g_confirm_id == (uint8_t)BANK_LFO) {
            midi_lfo_bank_reset(&g_cfg, &g_st);
        } else {
            midi_bank_copy_preset_slots(&g_cfg, g_confirm_id, g_cfg.mode);
            for (k = 0; k < 4u; k++)
                param_slot_side_effects((uint32_t)g_confirm_id * 4u + k);
        }
        mode_mem_save(g_cfg.mode);
        mark_cfg_dirty();
        return;
    }
    if (g_confirm_id == PERF_PAGE_ARP) {
        perf_arp_release_extra();
        perf_arp_reset_values(&g_perf);
        g_ui_dirty = 1u;
    } else if (g_confirm_id == PERF_PAGE_SEQ) {
        if (g_perf.seq_root_captured) {
            perf_apply_root(g_perf.seq_base_saved);
            g_perf.seq_root_captured = 0u;
        }
        perf_seq_reset_meta(&g_perf);
        g_ui_dirty = 1u;
    } else if (g_confirm_id == PERF_PAGE_REC) {
        perf_rec_reset_values(&g_perf);
        g_ui_dirty = 1u;
    }
}

static void bank_confirm_apply_random(void)
{
    uint32_t k;
    uint32_t j;

    if (g_confirm_kind == 2u) {
        for (k = 0; k < 4u; k++)
            midi_bank_random_slots(&g_cfg, &g_st, (uint8_t)k);
        midi_lfo_bank_random(&g_cfg, &g_st);
        midi_lfo_clear_depth_for_param(&g_cfg, &g_st, P_PARAM_B);
        midi_lfo_tick(&g_cfg, &g_st, fm1_ms);
        perf_lfo_apply(&g_perf, &g_cfg, &g_st);
        for (k = 0; k < 4u; k++) {
            for (j = 0; j < 4u; j++)
                bank_confirm_slot_side_effects(k * 4u + j);
        }
        mode_mem_random_all_modes();
        mode_mem_save(g_cfg.mode);
        bank_confirm_random_arp(&g_st);
        bank_confirm_random_seq(&g_st, 0);
        mark_cfg_dirty();
        g_ui_dirty = 1u;
        return;
    }
    if (g_confirm_kind == 0u) {
        if (g_confirm_id == (uint8_t)BANK_LFO) {
            midi_lfo_bank_random(&g_cfg, &g_st);
        } else {
            uint8_t bank = g_confirm_id % (uint8_t)NBANKS;

            midi_bank_random_slots(&g_cfg, &g_st, bank);
            midi_lfo_tick(&g_cfg, &g_st, fm1_ms);
            perf_lfo_apply(&g_perf, &g_cfg, &g_st);
            for (k = 0; k < 4u; k++)
                bank_confirm_slot_side_effects((uint32_t)bank * 4u + k);
        }
        mode_mem_save(g_cfg.mode);
        mark_cfg_dirty();
        return;
    }
    if (g_confirm_id == PERF_PAGE_ARP) {
        bank_confirm_random_arp(&g_st);
        g_ui_dirty = 1u;
    } else if (g_confirm_id == PERF_PAGE_SEQ) {
        bank_confirm_random_seq(&g_st, 1);
        g_ui_dirty = 1u;
    }
}

static void draw_bank_confirm_header(int32_t cy)
{
    static const char turn_pre[] = "Turn ";
    static const char turn_post[] = " [Select]";
    const int32_t knob_w = 7;
    const int32_t knob_gap = 2;
    int32_t knob_y = cy - 28 + ((int32_t)FONT_S.h - knob_w) / 2;
    int32_t w_turn = (int32_t)text_w(&FONT_S, turn_pre);
    int32_t w_post = (int32_t)text_w(&FONT_S, turn_post);
    int32_t total_w = w_turn + knob_gap + knob_w + w_post;
    int32_t x0 = 120 - total_w / 2;

    cv_text(x0, cy - 28, &FONT_S, turn_pre, C_AMB);
    bank_confirm_draw_knob_icon(x0 + w_turn + knob_gap, knob_y);
    cv_text(x0 + w_turn + knob_gap + knob_w, cy - 28, &FONT_S, turn_post, C_WHITE);
}

static void draw_bank_confirm(void)
{
    uint32_t h = ui_stage_h();
    const int32_t dlg_h = 88;
    int32_t blit_y = 24 + ((int32_t)h - dlg_h) / 2;
    int32_t cy = dlg_h / 2;
    int32_t is_all = (g_confirm_kind == 2u);
    const char *mid_lab = bank_confirm_title();
    int32_t btn_h = 26;
    int32_t btn_y = cy + 2;
    int32_t lw = (int32_t)text_w(&FONT_S, mid_lab);

    cv_begin(240, (uint32_t)dlg_h, C_BLACK);
    if (is_all) {
        cv_rect(6, cy - 40, 228, 80, C_ACC);
        cv_rect(7, cy - 39, 226, 78, C_WHITE);
    }
    cv_rect(8, cy - 38, 224, 76, C_LINE);
    draw_bank_confirm_header(cy);
    bank_confirm_draw_arrow_left(btn_y, btn_h);
    if (bank_confirm_has_random())
        bank_confirm_draw_arrow_right(btn_y, btn_h);
    cv_rect(86, btn_y, 68, btn_h, C_ACC);
    cv_text(86 + 34 - lw / 2, btn_y + 5, &FONT_S, mid_lab, C_WHITE);
    cv_blit(0, (uint32_t)blit_y);
}

static void draw_stage(void)
{
    if (fm1_ms < g_saved_until) {
        uint32_t h = ui_stage_h();
        int32_t cy = (int32_t)h / 2;
        const char *mname = MODE_NAMES[g_cfg.mode % NMODES];

        cv_begin(240, h, C_BLACK);
        cv_rect(8, cy - 36, 224, 72, C_LINE);
        cv_rect(8, cy - 36, 224, 3, C_ACC);
        cv_text(120 - text_w(&FONT_L, "SAVED") / 2, cy - 26, &FONT_L, "SAVED", C_WHITE);
        cv_text(120 - text_w(&FONT_S, mname) / 2, cy + 12, &FONT_S, mname, C_AMB);
        cv_blit(0, 24);
        return;
    }
    if (bank_confirm_active()) {
        draw_bank_confirm();
        return;
    }
    /* Stage diagram is NEVER obscured by parameter tweaks! */
    draw_stage_idle();
}

static void draw_bank_strip(void)
{
    if (g_perf.page != PERF_PAGE_BANK) {
        static const uint16_t PERF_COL[3] = {
            RGB(64, 200, 220),
            RGB(120, 100, 255),
            RGB(200, 80, 180)
        };
        static const uint16_t PERF_BG[3] = {
            RGB(6, 28, 32),
            RGB(14, 12, 36),
            RGB(32, 10, 28)
        };
        uint32_t pi = (uint32_t)g_perf.page - 1u;
        uint32_t col;

        cv_begin(240, 62, C_BLACK);
        for (col = 0; col < 4u; col++) {
            uint32_t v;
            uint32_t vbase;
            uint32_t vslot = 0u;
            int32_t x = 4 + (int32_t)col * 59;
            char s[16];
            uint32_t bar;
            uint32_t cc = 40u + (g_perf.page - 1u) * 4u + col;
            int mod = 0;

            if (g_perf.page == PERF_PAGE_ARP) {
                vslot = VSLOT_ARP_BASE + col;
                vbase = g_perf.arp_base[col];
                v = midi_mod_hold_active(vslot, fm1_ms) ? vbase : g_perf.arp[col];
                mod = midi_lfo_get_depth(&g_cfg, &g_st, vslot) > 0u;
            } else if (g_perf.page == PERF_PAGE_SEQ) {
                vslot = VSLOT_SEQ_BASE + col;
                vbase = g_perf.seq_base[col];
                v = midi_mod_hold_active(vslot, fm1_ms) ? vbase : g_perf.seq[col];
                mod = midi_lfo_get_depth(&g_cfg, &g_st, vslot) > 0u;
            } else {
                v = g_perf.rec[col];
                vbase = v;
            }
            bar = (v * 26u) / 127u;
            if (mod) {
                s[0] = '~';
                s[1] = 'C';
                s[2] = 'C';
                u8_dec(s + 3, cc);
            } else {
                s[0] = 'C';
                s[1] = 'C';
                u8_dec(s + 2, cc);
            }
            cv_text(x + 29 - (int32_t)text_w(&FONT_S, s) / 2, 1, &FONT_S, s, mod ? C_ACC : C_DIM);
            cv_rect(x + 4, 17, 50, 26, PERF_BG[pi]);
            if (bar)
                cv_rect(x + 4, 17 + 26 - (int32_t)bar, 50, (int32_t)bar, PERF_COL[pi]);
            if (mod && g_perf.page != PERF_PAGE_REC) {
                uint32_t vh = (v * 26u) / 127u;
                uint32_t bh = (vbase * 26u) / 127u;

                if (vh > 0)
                    cv_rect(x + 4, 17 + 26 - (int32_t)vh, 50, 3, C_ACC);
                if (bh > 0)
                    cv_rect(x + 4, 17 + 26 - (int32_t)bh, 50, 2, C_WHITE);
            }
            perf_fmt_param(g_perf.page, col, s);
            cv_text(x + 29 - (int32_t)text_w(&FONT_S, s) / 2, 20, &FONT_S, s, C_WHITE);
            copy_str(s, perf_param_label(g_perf.page, col));
            cv_text(x + 29 - (int32_t)text_w(&FONT_S, s) / 2, 46, &FONT_S, s, PERF_COL[pi]);
        }
        cv_blit(0, (int32_t)ui_bank_y());
        return;
    }
    /* Same ~12-deg blue bank progression as HOME matrix (indexed by BANK_SYNTH..BANK_LFO):
     * FX (SPACE)=192 deg, SEL (ENGINE)=204 deg, ENV (SYNTH)=216 deg, GLO (TIME)=228 deg, LFO=violet */
    static const uint16_t BANK_COL[NBANKS] = {
        RGB(95, 160, 255),  /* BANK_SYNTH  (ENV) ~216 deg */
        RGB(85, 190, 255),  /* BANK_ENGINE (SEL) ~204 deg */
        RGB(110, 130, 255), /* BANK_TIME   (GLO) ~228 deg */
        RGB(75, 220, 255),  /* BANK_SPACE  (FX)  ~192 deg */
        RGB(155, 120, 255)  /* BANK_LFO    (LFO) violet   */
    };
    static const uint16_t BANK_BG[NBANKS] = {
        RGB(10, 17, 38),
        RGB(9, 20, 36),
        RGB(12, 14, 40),
        RGB(8, 24, 34),
        RGB(18, 12, 38)
    };
    uint32_t b = g_cfg.bank % NBANKS;
    uint32_t col, base = b * 4u;

    cv_begin(240, 62, C_BLACK);
    for (col = 0; col < 4u; col++) {
        uint32_t slot = base + col;
        uint32_t v;
        uint32_t cc = g_cfg.cc_map[slot];
        int32_t x = 4 + (int32_t)col * 59;
        char s[16];
        uint32_t bar;

        if (b == BANK_LFO && g_st.lfo_page == 2u) {
            uint32_t ts = midi_lfo_target_slot(&g_cfg);

            if (col == 0u)
                v = (g_st.lfo_algo[ts] * 127u) / (FM_LFO_NALGOS - 1u);
            else if (col == 1u)
                v = g_st.lfo_ratio[ts];
            else if (col == 2u)
                v = g_st.lfo_index[ts];
            else
                v = g_st.lfo_fb[ts];
        } else if (b == BANK_LFO && g_st.lfo_page == 1u) {
            uint32_t ts = midi_lfo_target_slot(&g_cfg);

            if (col == 0u)
                v = g_st.lfo_phase[ts];
            else if (col == 1u)
                v = g_st.lfo_skew[ts];
            else if (col == 2u)
                v = g_st.lfo_smooth[ts];
            else
                v = g_st.lfo_fold[ts];
        } else
            v = midi_mod_hold_active(slot, fm1_ms) ? g_cfg.base_val[slot] : g_cfg.val[slot];
        bar = (v * 26u) / 127u;

        if (midi_lfo_get_depth(&g_cfg, &g_st, slot) > 0u) {
            s[0] = '~';
            s[1] = 'C';
            s[2] = 'C';
            u8_dec(s + 3, cc);
        } else {
            s[0] = 'C';
            s[1] = 'C';
            u8_dec(s + 2, cc);
        }
        cv_text(x + 29 - (int32_t)text_w(&FONT_S, s) / 2, 1, &FONT_S, s,
                midi_lfo_get_depth(&g_cfg, &g_st, slot) > 0u ? C_ACC : C_DIM);
        cv_rect(x + 4, 17, 50, 26, BANK_BG[b]);
        if (bar)
            cv_rect(x + 4, 17 + 26 - (int32_t)bar, 50, (int32_t)bar,
                    slot == g_st.hud_param ? C_ACC : BANK_COL[b]);
        param_fmt_val(slot, s, 1);
        cv_text(x + 29 - (int32_t)text_w(&FONT_S, s) / 2, 20, &FONT_S, s, C_WHITE);
        if (b == BANK_LFO && g_st.lfo_page == 2u) {
            static const char *const LFO_P2[4] = {"ALGO", "RATIO", "FM IDX", "OP FB"};

            copy_str(s, LFO_P2[col]);
        } else if (b == BANK_LFO && g_st.lfo_page == 1u) {
            static const char *const LFO_P1[4] = {"PHASE", "SKEW", "SMOOTH", "FOLD"};

            copy_str(s, LFO_P1[col]);
        } else {
            const char *nm = midi_param_name(g_cfg.mode, slot);
            uint32_t i = 0;

            while (nm[i] && i < 6u) {
                s[i] = nm[i];
                i++;
            }
            s[i] = 0;
        }
        cv_text(x + 29 - (int32_t)text_w(&FONT_S, s) / 2, 46, &FONT_S, s, BANK_COL[b]);
    }
    cv_blit(0, (int32_t)ui_bank_y());
}

static int32_t param_pitch_x(int32_t note)
{
    if (note < 36)
        note = 36;
    if (note > 84)
        note = 84;
    return 24 + (note - 36) * 192 / 48;
}

static void draw_sel_triaxis_viz(void)
{
    uint8_t notes[4];
    uint32_t i, j, nn = g_triptych.n_sound;
    int32_t arch_h = 4 + (int32_t)(g_triptych.voicing > 3u ? 3u : g_triptych.voicing) * 4;
    uint16_t arc_col = RGB(45, 120, 180);
    uint16_t node_col = RGB(85, 190, 255);

    cv_line(8, 11, 231, 11, C_LINE);
    if (nn > 4u)
        nn = 4u;
    for (i = 0; i < nn; i++)
        notes[i] = g_triptych.sounding[i];
    for (i = 0; i < nn; i++)
        for (j = i + 1u; j < nn; j++)
            if (notes[j] < notes[i]) {
                uint8_t t = notes[i];
                notes[i] = notes[j];
                notes[j] = t;
            }
    for (i = 0; i + 1u < nn; i++) {
        int32_t x0 = param_pitch_x(notes[i]);
        int32_t x1 = param_pitch_x(notes[i + 1u]);
        int32_t span = x1 - x0;
        int32_t py = 11;
        int32_t t;

        if (span < 1)
            span = 1;
        for (t = x0 + 1; t <= x1; t++) {
            int32_t dy = (4 * arch_h * (t - x0) * (x1 - t)) / (span * span);
            int32_t y = 11 - dy;

            cv_line(t - 1, py, t, y, arc_col);
            py = y;
        }
    }
    for (i = 0; i < nn; i++) {
        int32_t x = param_pitch_x(notes[i]);

        cv_rect(x - 2, 6, 5, 10, node_col);
        cv_rect(x - 1, 4, 3, 4, C_WHITE);
    }
}

static void draw_sel_contura_viz(void)
{
    uint32_t sc = g_contura.scale % 20u;
    uint16_t mask = CONTURA_SCALE_MASKS[sc];
    uint8_t lead_pc = g_contura.lead_note % 12u;
    uint32_t semi;

    for (semi = 0; semi < 12u; semi++) {
        int32_t x = 16 + (int32_t)semi * 208 / 11;
        int active = (mask >> (11u - semi)) & 1u;
        int playing = lead_pc == (uint8_t)((g_contura.base + semi) % 12u);

        if (active) {
            uint16_t col = semi == 0u ? C_WHITE : RGB(85, 190, 255);

            if (playing)
                cv_rect(x - 3, 3, 7, 16, C_ACC);
            cv_rect(x - 1, 7, 3, 8, col);
        } else
            cv_pset(x, 11, C_DIM);
    }
}

static void draw_arp_strip(const perf_state_t *p)
{
    static const uint16_t STR[3] = {RGB(64, 200, 220), RGB(38, 130, 150), RGB(95, 230, 245)};
    uint32_t mode = perf_arp_mode(p);
    uint32_t sp = perf_arp_sp(p);
    uint32_t n_cells = perf_arp_n_cells(p->arp[1]);
    uint32_t gate_pct = 15u + ((uint32_t)p->arp[3] * 80u) / 127u;
    int32_t max_semi;
    int32_t amp;
    int32_t s;
    int32_t x;

    if (mode > 9u)
        mode = 9u;
    if (mode == 0u) {
        cv_line(8, 11, 231, 11, RGB(18, 62, 72));
        return;
    }
    max_semi = perf_arp_span_top(sp);
    if (max_semi < 1)
        max_semi = 1;
    amp = 4 + max_semi / 12 + (int32_t)gate_pct / 32;
    if (amp < 4)
        amp = 4;
    if (amp > 9)
        amp = 9;
    for (s = 0; s < 3; s++) {
        int32_t py = -1;

        for (x = 8; x < 232; x++) {
            uint32_t cell = (uint32_t)(x - 8) * n_cells / 224u;

            if (cell >= n_cells)
                cell = n_cells - 1u;
            if (mode == 9u) {
                int32_t k;
                int32_t y = 11;

                for (k = -1; k <= 1; k++) {
                    int32_t yy = y + k * 3;

                    if (py >= 0)
                        cv_line(x - 1, py + k * 3, x, yy, STR[s]);
                }
                py = y;
            } else {
                int32_t semi = perf_arp_stage_semi(p, cell);
                int32_t y = 11 - semi * amp / max_semi;

                if (y < 2)
                    y = 2;
                if (y > 20)
                    y = 20;
                if (py >= 0)
                    cv_line(x - 1, py, x, y, STR[s]);
                py = y;
            }
        }
    }
}


static int32_t perf_seq_strip_y(int32_t off)
{
    int32_t y = 10 - (off * 7) / 6;

    if (y < 2)
        y = 2;
    if (y > 19)
        y = 19;
    return y;
}

static void draw_seq_strip(const perf_state_t *p)
{
    uint32_t nsteps = perf_seq_nsteps(p->seq[0]);
    uint32_t draw_n = nsteps ? nsteps : 4u;
    uint32_t mut_pct = ((uint32_t)p->seq[3] * 100u) / 127u;
    uint32_t pace_idx = perf_seq_pace_idx(p->seq[1]);
    uint32_t pace = perf_seq_pace_ticks(p->seq[1]);
    uint32_t ph0 = fm1_ms * 11000u;
    uint32_t wave_t = 44000u - pace_idx * 5500u;
    int32_t ax_cx[PERF_SEQ_MAX_STEPS];
    int32_t ax_y[PERF_SEQ_MAX_STEPS];
    int32_t pulse_x = -1;
    uint32_t s;
    int32_t x;
    int32_t py0 = -1;
    int32_t py1 = -1;
    int32_t py2 = -1;

    for (s = 0; s < draw_n; s++) {
        int32_t off = 0;

        ax_cx[s] = perf_seq_col_cx(s, nsteps);
        if (nsteps != 0u) {
            perf_seq_prog_offset(p, s, &off);
            if (perf_seq_active(p) && g_st.playing && s == (p->seq_step_i % nsteps))
                off += (int32_t)p->seq_mut_off;
        }
        ax_y[s] = perf_seq_strip_y(off);
    }
    if (nsteps != 0u && perf_seq_active(p) && g_st.playing && pace > 0u) {
        uint32_t cur = p->seq_step_i % nsteps;
        uint32_t sn = (cur + 1u) % nsteps;
        int32_t x0 = ax_cx[cur];
        int32_t x1 = ax_cx[sn];
        int32_t prog = (int32_t)p->seq_acc * 256 / (int32_t)pace;

        pulse_x = x0 + ((x1 - x0) * prog) / 256;
    }
    for (x = 8; x < 232; x++) {
        uint32_t t = (uint32_t)(x - 8);
        int32_t base = 10;
        int32_t wtot = 0;
        int32_t w1;
        int32_t w2;
        int32_t w3;
        int32_t y;
        int32_t y0;
        int32_t y1;
        int32_t y2;
        int32_t pulse = 0;
        int32_t mod;
        uint16_t col;

        for (s = 0; s < draw_n; s++) {
            int32_t dist = x - ax_cx[s];

            if (dist < 0)
                dist = -dist;
            if (dist < 48) {
                int32_t w = 48 - dist;

                base += (ax_y[s] - 10) * w;
                wtot += w;
            }
        }
        if (wtot > 0)
            base /= wtot;
        w1 = fm2_sine(ph0 + t * wave_t);
        w2 = fm2_sine(ph0 + t * (wave_t + wave_t / 3u) + 90000000u);
        w3 = fm2_sine(ph0 + t * (wave_t / 2u) + 180000000u);
        mod = 0;
        if (mut_pct > 0u)
            mod = (fm2_sine(ph0 * 3u + t * 28000u + p->seq_step_i * 4000000u) * (int32_t)mut_pct) / 180;
        y = base - ((w1 * 5 + w2 * 3 + w3 * 2) >> 17) - mod / 4;
        y0 = y - ((w1 * 3) >> 16);
        y1 = y - ((w2 * 4) >> 16);
        y2 = y - ((w3 * 2) >> 16);
        if (pulse_x >= 0) {
            int32_t dx = x - pulse_x;

            if (dx < 0)
                dx = -dx;
            if (dx < 14)
                pulse = 14 - dx;
        }
        if (pulse > 0)
            y -= pulse / 3;
        if (y < 2)
            y = 2;
        if (y > 19)
            y = 19;
        col = RGB(36, 30, 78);
        if (py0 >= 0)
            cv_line(x - 1, py0, x, y0, col);
        if (py1 >= 0)
            cv_line(x - 1, py1, x, y1, RGB(70, 58, 130));
        if (py2 >= 0)
            cv_line(x - 1, py2, x, y2, RGB(120, 100, 255));
        if (pulse > 4)
            cv_pset(x, y, C_ACC);
        py0 = y0;
        py1 = y1;
        py2 = y;
    }
    for (s = 0; s < draw_n; s++) {
        int32_t cx = ax_cx[s];
        int32_t cy = ax_y[s];

        cv_rect(cx - 2, cy - 2, 5, 5, C_WHITE);
        cv_pset(cx, cy, RGB(120, 100, 255));
    }
    if (mut_pct > 0u) {
        uint32_t k;

        for (k = 0; k < 6u; k++) {
            uint32_t ph = fm1_ms * 14000u + k * 715827883u;
            int32_t cx = 120 + ((fm2_sine(ph) * 100) >> 15);
            int32_t cy = 10 - ((fm2_sine(ph + 1073741824u) * 7) >> 15);
            int32_t r = 1 + (int32_t)(mut_pct / 40u);

            cv_rect(cx - r, cy - r, r * 2 + 1, r * 2 + 1, RGB(180, 140, 255));
        }
    }
}

static void draw_rec_reel_spoke(int32_t cx, int32_t cy, int32_t r, uint32_t spin, uint16_t col)
{
    int32_t k;

    for (k = 0; k < 4; k++) {
        uint32_t ph = spin + (uint32_t)k * 1073741824u;
        int32_t x1 = cx + ((fm2_sine(ph) * r) >> 15);
        int32_t y1 = cy - ((fm2_sine(ph + 1073741824u) * r) >> 15);

        cv_line(cx, cy, x1, y1, col);
    }
}

static void draw_rec_reel(int32_t cx, int32_t cy, int32_t r_rim, int32_t r_spoke, uint32_t ang, uint16_t rim, uint16_t spoke)
{
    int32_t k;
    int32_t px0 = -1;
    int32_t py0 = -1;

    for (k = 0; k < 8; k++) {
        uint32_t ph = ang + (uint32_t)k * 536870912u;
        int32_t px = cx + ((fm2_sine(ph) * r_rim) >> 15);
        int32_t py = cy - ((fm2_sine(ph + 1073741824u) * r_rim) >> 15);

        if (px0 >= 0)
            cv_line(px0, py0, px, py, rim);
        px0 = px;
        py0 = py;
    }
    if (px0 >= 0)
        cv_line(px0, py0, cx + ((fm2_sine(ang) * r_rim) >> 15),
                cy - ((fm2_sine(ang + 1073741824u) * r_rim) >> 15), rim);
    draw_rec_reel_spoke(cx, cy, r_spoke, ang, spoke);
}


static void draw_rec_strip(const perf_state_t *p)
{
    uint32_t st = perf_rec_state(p);
    uint32_t tgt = ((uint32_t)p->rec[1] * 3u) / 128u;
    uint32_t bars = perf_rec_loop_bars(p->rec[2]);
    uint32_t bar_div = bars > 0u ? bars : 1u;
    int running = (st >= 2u && g_st.playing);
    uint32_t ang;
    uint16_t rim_col;
    uint16_t spoke_col;
    uint16_t tape_col;
    int32_t lx = 24;
    int32_t rx = 216;
    int32_t cy = 11;
    int32_t tape_x0 = 36;
    int32_t tape_x1 = 204;
    int32_t tape_w = tape_x1 - tape_x0;
    int32_t head_x = 120;
    int32_t x;
    uint32_t scroll_px;
    uint32_t lt;

    if (st > 4u)
        st = 4u;
    if (tgt > 2u)
        tgt = 2u;
    if (running)
        ang = (fm1_ms * 12000000u) / bar_div;
    else
        ang = p->rec_acc * 0x04000000u;

    if (st == 0u) {
        rim_col = RGB(42, 36, 58);
        spoke_col = RGB(55, 48, 72);
        tape_col = RGB(38, 32, 48);
    } else if (st == 1u) {
        rim_col = RGB(90, 28, 32);
        spoke_col = RGB(200, 55, 50);
        tape_col = RGB(48, 28, 36);
    } else if (st == 2u) {
        rim_col = RGB(90, 28, 32);
        spoke_col = RGB(235, 65, 60);
        tape_col = RGB(72, 32, 32);
    } else if (st == 3u) {
        rim_col = RGB(40, 70, 88);
        spoke_col = RGB(70, 180, 210);
        tape_col = RGB(45, 100, 120);
    } else {
        rim_col = RGB(72, 40, 98);
        spoke_col = RGB(230, 140, 255);
        tape_col = RGB(58, 36, 72);
    }

    lt = p->rec_loop_ticks > 0u ? p->rec_loop_ticks : bar_div * 96u;
    scroll_px = (p->rec_acc * (uint32_t)tape_w) / lt;
    if (running)
        scroll_px += ((fm1_ms % 33u) * (uint32_t)tape_w) / (lt * 33u);

    cv_line(lx + 8, 4, rx - 8, 4, RGB(28, 24, 38));
    cv_line(tape_x0, cy, tape_x1, cy, RGB(30, 26, 40));

    for (x = tape_x0; x < tape_x1; x++) {
        uint32_t b = ((uint32_t)(x - tape_x0) + scroll_px) * 64u / (uint32_t)tape_w;
        int32_t mv = (int32_t)p->motion_buf[b & 63u];
        int32_t y = cy - mv / 10;
        uint16_t pc = tape_col;

        if (y < 2)
            y = 2;
        if (y > 20)
            y = 20;
        if ((b & 7u) == 0u)
            cv_line(x, cy - 1, x, cy + 1, RGB(90, 75, 55));
        if (tgt == 0u || tgt == 2u) {
            uint32_t nshow = perf_seq_nsteps(p->seq[0]);

            if (nshow == 0u)
                nshow = PERF_SEQ_MAX_STEPS;
            if (nshow > PERF_SEQ_MAX_STEPS)
                nshow = PERF_SEQ_MAX_STEPS;
            int32_t step_i = (int32_t)((b * nshow) / 64u);
            int32_t semi = (int32_t)p->seq_steps[(uint32_t)step_i % PERF_SEQ_MAX_STEPS];

            if (semi != 0) {
                int32_t ny = cy - semi;

                if (ny < 3)
                    ny = 3;
                if (ny > 19)
                    ny = 19;
                cv_pset(x, ny, C_ACC);
            }
        }
        if (tgt == 1u || tgt == 2u) {
            if (mv != 0)
                pc = st >= 2u ? RGB(200, 80, 180) : RGB(55, 28, 50);
            cv_pset(x, y, pc);
            if (mv != 0 && y != cy)
                cv_pset(x, cy, RGB(35, 28, 42));
        } else if ((b & 7u) == 0u) {
            cv_pset(x, cy, RGB(50, 42, 58));
        }
    }

    if (running) {
        uint32_t dash = scroll_px / 4u;

        for (x = tape_x0; x < tape_x1; x++) {
            if (((uint32_t)(x - tape_x0) + dash) % 8u < 4u)
                cv_pset(x, 18, RGB(45, 38, 55));
        }
    } else {
        cv_line(tape_x0, 18, tape_x1, 18, RGB(32, 28, 42));
    }

    draw_rec_reel(lx, cy, 8, 7, ang, rim_col, spoke_col);
    draw_rec_reel(rx, cy, 8, 7, ang + 536870912u, rim_col, spoke_col);

    if (st >= 1u) {
        uint16_t head = C_WHITE;
        int32_t pulse = 0;

        if (st == 1u) {
            pulse = (int32_t)((fm2_sine(fm1_ms * 9000u) * 3) >> 15);
            head = RGB(230, 60, 55);
        } else if (st == 2u)
            head = RGB(235, 65, 60);
        else if (st == 3u)
            head = RGB(80, 230, 140);
        else if (st == 4u)
            head = RGB(230, 140, 255);
        cv_line(head_x - 4, 3, head_x - 4, 19 + pulse, head);
        cv_line(head_x + 3, 3, head_x + 3, 19 + pulse, head);
        cv_line(head_x - 4, 3, head_x + 3, 3, head);
        cv_line(head_x - 4, 19 + pulse, head_x + 3, 19 + pulse, head);
    }
}

static void draw_param_strip_label(uint32_t p, int learn)
{
    const char *nm;
    char val_s[24], line[44];
    uint32_t i = 0, j = 0;
    int32_t lw, x;

    if (p >= NPARAMS)
        p = 0;
    nm = midi_param_name(g_cfg.mode, p);
    param_fmt_val(p, val_s, 0);

    while (nm[i] && j + 1u < sizeof line)
        line[j++] = nm[i++];
    if (j + 2u < sizeof line) {
        line[j++] = ':';
        line[j++] = ' ';
    }
    i = 0;
    while (val_s[i] && j + 1u < sizeof line)
        line[j++] = val_s[i++];
    if (learn)
        copy_str(line + j, "  [MOVE CC]");
    else
        line[j] = 0;

    lw = (int32_t)text_w(&FONT_S, line);
    x = 8 + (224 - lw) / 2;
    if (x < 4)
        x = 4;
    cv_text(x, 3, &FONT_S, line, learn ? C_WHITE : RGB(85, 190, 255));
}

/* Bottom 22px strip: live bank visualization (ENV / FX / GLO / LFO / SEL), or MIDI learn / Dit Dah status text. */
static void draw_param_line(void)
{
    uint32_t b = g_cfg.bank % NBANKS;

    cv_begin(240, 22, C_BLACK);

    if (g_perf.page == PERF_PAGE_ARP) {
        draw_arp_strip(&g_perf);
    } else if (g_perf.page == PERF_PAGE_SEQ) {
        draw_seq_strip(&g_perf);
    } else if (g_perf.page == PERF_PAGE_REC) {
        draw_rec_strip(&g_perf);
    }
    else if (b == BANK_SYNTH) {
        /* ENV: AD envelope contour (Attack, Decay, Sustain floor) filled with FM wave (Ratio + Timbre) */
        uint32_t atk = g_cfg.val[P_ATTACK];
        uint32_t dec = g_cfg.val[P_DECAY];
        int32_t timbre = (int32_t)g_cfg.val[P_TIMBRE];
        uint32_t cm, mm_q8;
        int32_t ax = 12 + (int32_t)(atk * 68u) / 127;
        int32_t dx = ax + 14 + (int32_t)(dec * 126u) / 127;
        int32_t sus_h = dec > 96u ? ((int32_t)(dec - 96u) * 14) / 31 : 0;
        int32_t fm_depth = (timbre * (timbre + 32)) >> 4;
        uint32_t ph0 = fm1_ms * 9000000u;
        int32_t x, py = -1;

        if (dx > 228)
            dx = 228;
        midi_ratio_cm_q8(g_cfg.val[P_RATIO], &cm, &mm_q8);
        cv_line(8, 19, 231, 19, C_LINE);
        for (x = 8; x < 232; x++) {
            int32_t eh = 0;
            uint32_t t = (uint32_t)(x - 8);
            uint32_t c_ph = ph0 + t * 42000000u * cm;
            uint32_t m_ph = (ph0 >> 1) + ((t * 42000000u) >> 8) * mm_q8;
            int32_t mod = fm2_sine(m_ph);
            int32_t pmod = (mod * fm_depth) << 10;
            int32_t s = fm2_sine(c_ph + (uint32_t)pmod);
            int32_t wy;

            if (x <= ax)
                eh = ax > 8 ? ((x - 8) * 16) / (ax - 8) : 16;
            else if (x <= dx)
                eh = 16 - ((x - ax) * (16 - sus_h)) / (dx > ax ? dx - ax : 1);
            else
                eh = sus_h;

            wy = 19 - (eh / 2) - (s * (eh / 2)) / 32768;
            if (wy < 2) wy = 2;
            if (wy > 19) wy = 19;
            cv_pset(x, 19 - eh, RGB(95, 160, 255));
            if (py >= 0 && eh > 1)
                cv_line(x - 1, py, x, wy, RGB(45, 95, 175));
            py = wy;
        }
    } else if (b == BANK_SPACE) {
        /* FX: Drive-clipped stereo wave split by Detune + Echo repeat taps (E.Time spacing, E.Mix height) */
        uint32_t drive = g_cfg.val[P_DRIVE];
        int32_t det = (int32_t)g_cfg.val[P_DETUNE] - 64;
        uint32_t emix = g_cfg.val[P_ECHO_MIX];
        uint32_t etime = g_cfg.val[P_ECHO_TIME];
        int32_t step_x = 12 + (int32_t)(etime * 28u) / 127;
        uint32_t ph0 = fm1_ms * 10000000u;
        int32_t x, pyl = -1, pyr = -1, tap;

        cv_line(8, 11, 231, 11, C_LINE);
        for (tap = 3; tap >= 1; tap--) {
            int32_t tx = 28 + tap * step_x;
            int32_t th = ((int32_t)emix * (4 - tap) * 9) / (127 * 3);
            if (th > 0 && tx < 228)
                cv_rect(tx, 11 - th, 4, th * 2 + 1, RGB(28, 95, 130));
        }
        for (x = 8; x < 232; x++) {
            uint32_t t = (uint32_t)(x - 8);
            uint32_t ph_l = ph0 + t * 36000000u;
            uint32_t ph_r = ph_l + (uint32_t)((int32_t)t * det * 260000);
            int32_t sl = fm2_soft_clip((fm2_sine(ph_l) * 220) >> 8, drive);
            int32_t sr = fm2_soft_clip((fm2_sine(ph_r) * 220) >> 8, drive);
            int32_t yl = 11 - (sl * 8) / 32768;
            int32_t yr = 11 - (sr * 8) / 32768;

            if (det != 0 && pyr >= 0)
                cv_line(x - 1, pyr, x, yr, C_ACC);
            if (pyl >= 0)
                cv_line(x - 1, pyl, x, yl, RGB(75, 220, 255));
            pyl = yl;
            pyr = yr;
        }
    } else if (b == BANK_TIME) {
        /* GLO: Rhythm pulse timeline — density=RATE, odd-step shift=SWING, height=OCTAVE, playhead=TEMPO */
        uint32_t rate_idx = ((uint32_t)g_cfg.val[P_GATE] * 11u) / 128u;
        uint32_t nsteps = 4u + rate_idx * 2u;
        uint32_t swing = g_cfg.val[P_SWING];
        int32_t oct = midi_oct_offset(&g_cfg);
        int32_t y_base = 11 - oct * 3;
        uint32_t play_step = (g_st.raw_tick / 3u) % nsteps;
        uint32_t i;

        cv_line(8, y_base, 231, y_base, C_LINE);
        for (i = 0; i < nsteps; i++) {
            int32_t span = 216 / (int32_t)nsteps;
            int32_t sx = 12 + (int32_t)i * span;
            if (i & 1u)
                sx += (span * (int32_t)swing) / 254;
            if (i == play_step)
                cv_rect(sx - 2, 3, 5, 16, C_WHITE);
            else
                cv_rect(sx - 1, y_base - 4, 2, 9, (i & 1u) ? RGB(70, 85, 180) : RGB(110, 130, 255));
        }
    } else if (b == BANK_LFO) {
        /* LFO: Bipolar modulation excursion meter around target's base_val */
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        int32_t bx;
        int32_t vx;

        if (ts >= VSLOT_ARP_BASE && ts < VSLOT_SEQ_BASE) {
            uint32_t col = ts - VSLOT_ARP_BASE;
            bx = 16 + ((int32_t)g_perf.arp_base[col] * 208) / 127;
            vx = 16 + ((int32_t)g_perf.arp[col] * 208) / 127;
        } else if (ts >= VSLOT_SEQ_BASE && ts < NVSLOTS) {
            uint32_t col = ts - VSLOT_SEQ_BASE;
            bx = 16 + ((int32_t)g_perf.seq_base[col] * 208) / 127;
            vx = 16 + ((int32_t)g_perf.seq[col] * 208) / 127;
        } else {
            bx = 16 + ((int32_t)g_cfg.base_val[ts] * 208) / 127;
            vx = 16 + ((int32_t)g_cfg.val[ts] * 208) / 127;
        }
        int32_t span = ((int32_t)g_cfg.base_val[P_LFO_DEPTH] * 52) / 127;
        int32_t x0 = bx - span < 16 ? 16 : bx - span;
        int32_t x1 = bx + span > 224 ? 224 : bx + span;

        cv_rect(16, 9, 208, 4, C_LINE);
        if (x1 > x0)
            cv_rect(x0, 7, x1 - x0, 8, RGB(50, 34, 95));
        cv_rect(vx - 2, 4, 5, 14, C_ACC);
        cv_rect(bx - 1, 3, 2, 16, C_WHITE);
    } else if (b == BANK_ENGINE && !g_st.learn_active) {
        if (g_cfg.mode == MODE_TRIAXIS)
            draw_sel_triaxis_viz();
        else if (g_cfg.mode == MODE_CONTURA)
            draw_sel_contura_viz();
    }

    if (g_st.learn_active)
        draw_param_strip_label(g_st.learn_slot, 1);
    else if (g_perf.page == PERF_PAGE_BANK && b == BANK_ENGINE && g_cfg.mode == MODE_DITDAH) {
        uint32_t p = g_st.hud_param;

        if (p < 4u || p >= 8u)
            p = P_PARAM_B;
        draw_param_strip_label(p, 0);
    }

    cv_blit(0, (int32_t)ui_param_y());
}

static void draw_keys_and_tabs(void)
{
    int32_t oct = midi_oct_offset(&g_cfg);
    uint32_t base = (uint32_t)(KEY0 + 12 * oct);
    uint32_t lit = (g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active) ? fm1_in.notes : synth_lit_keys(base);
    uint32_t k, w;

    cv_begin(240, 16, C_BLACK);
    for (k = 0, w = 0; k < 27u; k++)
        if (!key_is_black(KEY0 + k)) {
            cv_rect(8 + (int32_t)w * 14, 2, 13, 12, lit >> k & 1u ? C_ACC : C_DIM);
            w++;
        }
    for (k = 0, w = 0; k < 27u; k++)
        if (key_is_black(KEY0 + k)) {
            int32_t x = 8 + (int32_t)w * 14 - 5;
            int on = (lit >> k) & 1u;
            cv_rect(x, 2, 9, 7, on ? C_ACC : C_BLACK);
            if (on) {
                cv_rect(x, 2, 1, 7, C_BLACK);
                cv_rect(x + 8, 2, 1, 7, C_BLACK);
                cv_rect(x, 8, 9, 1, C_BLACK);
            }
        } else
            w++;
    cv_blit(0, (int32_t)ui_keys_y());
}

static void ui_redraw(void)
{
    cv_home_view = g_home_view;
    draw_topbar();
    draw_stage();
    if (!g_home_view) {
        draw_bank_strip();
        draw_param_line();
        draw_keys_and_tabs();
    }
    g_ui_dirty = 0u;
    g_ui_ms = fm1_ms;
}

static int32_t knob_step_for(uint32_t slot, int32_t delta)
{
    static uint32_t last_enc_ms;
    uint32_t dt = fm1_ms - last_enc_ms;
    int32_t fast = (dt < 85u || delta > 1 || delta < -1) ? 2 : 1;

    last_enc_ms = fm1_ms;
    if (slot == P_OCTAVE)
        return delta * 26;
    if (slot == P_RATIO)
        return delta;
    if (slot == P_GATE)
        return delta * 12;
    if (slot == P_LFO_TARGET)
        return delta;
    if (slot == P_LFO_SHAPE)
        return delta > 0 ? 1 : -1;
    if (slot == P_TEMPO)
        return delta * (fast > 1 ? 5 : 2);
    if (slot == P_LFO_RATE)
        return delta;

    if (g_cfg.mode == MODE_TRIAXIS) {
        if (slot == P_PARAM_A)
            return delta * (g_cfg.val[P_DENSITY] == 0u ? 10 : 32);
        if (slot == P_PARAM_B)
            return delta * (g_cfg.val[P_DENSITY] == 0u ? 42 : 22);
        if (slot == P_DENSITY)
            return delta * 19;
        if (slot == P_VEL_MOD)
            return delta * 16;
    } else if (g_cfg.mode == MODE_CONTURA) {
        if (slot == P_PARAM_A)
            return delta * 7;  /* 1 click = 1 scale */
        if (slot == P_PARAM_B)
            return delta * 5;  /* 1 click = 1 phrase step */
        if (slot == P_VEL_MOD)
            return delta * 11; /* 12 LAYERS buckets */
    } else if (g_cfg.mode == MODE_DITDAH) {
        if (slot == P_PARAM_A)
            return delta * 6;  /* 1 click = +1 semitone */
        if (slot == P_PARAM_B)
            return delta * 5;  /* 1 click = 1 preset phrase */
    }
    /* Continuous parameters: 5 per click, 10 per click when spun fast */
    return delta * (5 * fast);
}

static uint8_t perf_enum_bucket_cc(uint32_t bucket, uint32_t n)
{
    if (bucket == 0u)
        return 0u;
    if (bucket >= n - 1u)
        return 127u;
    return (uint8_t)(((uint32_t)bucket * 128u + 64u) / n);
}

static uint8_t perf_enum_step_cc(uint8_t val, int32_t delta, uint32_t n)
{
    uint32_t cur = ((uint32_t)val * n) / 128u;
    int32_t next;

    if (cur >= n)
        cur = n - 1u;
    next = (int32_t)cur + (delta > 0 ? 1 : -1);
    if (next < 0)
        next = 0;
    if (next >= (int32_t)n)
        next = (int32_t)n - 1;
    return perf_enum_bucket_cc((uint32_t)next, n);
}

static void edit_combo_mark(void)
{
    g_edit_combo_used = 1u;
    g_st.learn_active = 0u;
}

static uint8_t param_slot_edit_default(uint32_t slot)
{
    if (slot == (uint32_t)P_DETUNE)
        return 64u;
    if (slot == (uint32_t)P_DRIVE)
        return 0u;
    if (slot == (uint32_t)P_ECHO_MIX)
        return 28u;
    if (slot == (uint32_t)P_ECHO_TIME)
        return 56u;
    if (slot == (uint32_t)P_OCTAVE)
        return 64u;
    {
        midi_cfg_t tmp;

        midi_cfg_defaults(&tmp);
        midi_cfg_mode_preset(&tmp, g_cfg.mode);
        return tmp.base_val[slot];
    }
}

static void perf_param_adjust(uint32_t col, int32_t delta)
{
    uint8_t *arr;
    uint8_t *base;
    uint32_t vslot = 0u;
    uint8_t cc = 0u;
    int32_t v;
    int edit = (fm1_in.buttons >> BTN_EDIT) & 1u;

    if (delta == 0)
        return;
    if (edit) {
        static const uint8_t arp_def[4] = {0u, 64u, 0u, 64u};
        static const uint8_t seq_def[4] = {0u, 82u, 0u, 0u};
        static const uint8_t rec_def[4] = {0u, 0u, 0u, 127u};

        edit_combo_mark();
        if (g_perf.page == PERF_PAGE_ARP) {
            arr = g_perf.arp;
            base = g_perf.arp_base;
            v = delta < 0 ? (int32_t)arp_def[col] : 127;
            arr[col] = (uint8_t)v;
            base[col] = (uint8_t)v;
            if (col == 0u && delta < 0)
                perf_arp_release_extra();
            perf_lfo_apply(&g_perf, &g_cfg, &g_st);
            midi_mod_hold_arm(&g_cfg, &g_st, VSLOT_ARP_BASE + col, fm1_ms);
            g_st.hud_param = (uint8_t)(VSLOT_ARP_BASE + col);
            g_st.hud_cc = 255u;
            g_st.hud_until_ms = fm1_ms + 1200u;
            midi_send(0xB0, (uint32_t)PERF_CC_ARP_MODE + col, (uint32_t)v);
        } else if (g_perf.page == PERF_PAGE_SEQ) {
            arr = g_perf.seq;
            base = g_perf.seq_base;
            v = delta < 0 ? (int32_t)seq_def[col] : 127;
            arr[col] = (uint8_t)v;
            base[col] = (uint8_t)v;
            if (col == 0u && delta < 0 && g_perf.seq_root_captured) {
                perf_apply_root(g_perf.seq_base_saved);
                g_perf.seq_root_captured = 0u;
            }
            perf_lfo_apply(&g_perf, &g_cfg, &g_st);
            midi_mod_hold_arm(&g_cfg, &g_st, VSLOT_SEQ_BASE + col, fm1_ms);
            g_st.hud_param = (uint8_t)(VSLOT_SEQ_BASE + col);
            g_st.hud_cc = 255u;
            g_st.hud_until_ms = fm1_ms + 1200u;
            midi_send(0xB0, (uint32_t)PERF_CC_SEQ_STEPS + col, (uint32_t)v);
        } else if (g_perf.page == PERF_PAGE_REC) {
            v = delta < 0 ? (int32_t)rec_def[col] : 127;
            g_perf.rec[col] = (uint8_t)v;
        } else
            return;
        g_ui_dirty = 1u;
        return;
    }
    if (g_perf.page == PERF_PAGE_ARP) {
        arr = g_perf.arp;
        base = g_perf.arp_base;
        vslot = VSLOT_ARP_BASE + col;
        cc = (uint8_t)(PERF_CC_ARP_MODE + col);
    } else if (g_perf.page == PERF_PAGE_SEQ) {
        arr = g_perf.seq;
        base = g_perf.seq_base;
        vslot = VSLOT_SEQ_BASE + col;
        cc = (uint8_t)(PERF_CC_SEQ_STEPS + col);
    } else if (g_perf.page == PERF_PAGE_REC) {
        arr = g_perf.rec;
        base = g_perf.rec;
    } else
        return;

    if (g_perf.page == PERF_PAGE_ARP && col < 3u) {
        static const uint32_t n[3] = {10u, 6u, 6u};

        arr[col] = perf_enum_step_cc(arr[col], delta, n[col]);
    } else if (g_perf.page == PERF_PAGE_SEQ && col < 3u) {
        static const uint32_t n[3] = {8u, 7u, 8u};

        arr[col] = perf_enum_step_cc(arr[col], delta, n[col]);
    } else if (g_perf.page == PERF_PAGE_REC && col == 0u) {
        arr[0] = perf_enum_step_cc(arr[0], delta, 5u);
        if (perf_rec_state(&g_perf) == 1u) {
            g_perf.rec_acc = 0;
            g_perf.rec_bucket = 0;
        }
    } else if (g_perf.page == PERF_PAGE_REC && (col == 1u || col == 2u)) {
        arr[col] = perf_enum_step_cc(arr[col], delta, col == 1u ? 3u : 4u);
    } else {
        v = (int32_t)arr[col] + delta * 5;
        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
        arr[col] = (uint8_t)v;
    }
    if (g_perf.page == PERF_PAGE_ARP || g_perf.page == PERF_PAGE_SEQ) {
        base[col] = arr[col];
        perf_lfo_apply(&g_perf, &g_cfg, &g_st);
        midi_mod_hold_arm(&g_cfg, &g_st, vslot, fm1_ms);
        g_st.hud_param = (uint8_t)vslot;
        g_st.hud_cc = 255u;
        g_st.hud_until_ms = fm1_ms + 1200u;
        midi_send(0xB0, cc, arr[col]);
    }
    g_ui_dirty = 1u;
}

static void perf_rec_begin_from_arm(void)
{
    uint32_t i;

    if (perf_rec_state(&g_perf) != 1u)
        return;
    for (i = 0; i < 64u; i++)
        g_perf.motion_buf[i] = 0;
    for (i = 0; i < NPARAMS; i++)
        g_perf.motion_base[i] = g_cfg.base_val[i];
    g_perf.rec_acc = 0;
    g_perf.rec_bucket = 0;
    g_perf.rec_user_step = 0;
    g_perf.rec[0] = 64u;
    g_ui_dirty = 1u;
}

static void perf_rec_capture_knob(uint32_t slot, int32_t delta)
{
    uint32_t tgt = ((uint32_t)g_perf.rec[1] * 3u) / 128u;
    uint32_t st = perf_rec_state(&g_perf);

    if (st != 1u && st != 2u && st != 4u)
        return;
    if (tgt == 0u)
        return;
    g_perf.motion_slot = (uint8_t)(slot % NPARAMS);
    if (st == 1u)
        perf_rec_begin_from_arm();
    {
        int32_t d = (int32_t)g_perf.motion_buf[g_perf.rec_bucket % 64u] + delta * 3;
        if (d < -64)
            d = -64;
        if (d > 64)
            d = 64;
        g_perf.motion_buf[g_perf.rec_bucket % 64u] = (int8_t)d;
    }
}

static void lfo_page1_adjust(uint32_t col, int32_t delta)
{
    uint32_t ts = midi_lfo_target_slot(&g_cfg);
    int32_t v;
    int edit = (fm1_in.buttons >> BTN_EDIT) & 1u;

    if (delta == 0)
        return;
    if (col == 0u) {
        if (edit) {
            edit_combo_mark();
            g_st.lfo_phase[ts] = delta < 0 ? 0u : 127u;
        } else {
            v = (int32_t)g_st.lfo_phase[ts] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            g_st.lfo_phase[ts] = (uint8_t)v;
        }
    } else if (col == 1u) {
        if (edit) {
            edit_combo_mark();
            g_st.lfo_skew[ts] = delta < 0 ? FM_LFO_SKEW_DEFAULT : 127u;
        } else {
            v = (int32_t)g_st.lfo_skew[ts] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            g_st.lfo_skew[ts] = (uint8_t)v;
        }
    } else if (col == 2u) {
        if (edit) {
            edit_combo_mark();
            g_st.lfo_smooth[ts] = delta < 0 ? 0u : 127u;
        } else {
            v = (int32_t)g_st.lfo_smooth[ts] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            g_st.lfo_smooth[ts] = (uint8_t)v;
        }
    } else if (edit) {
        edit_combo_mark();
        g_st.lfo_fold[ts] = delta < 0 ? 0u : 127u;
    } else {
        v = (int32_t)g_st.lfo_fold[ts] + delta;
        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
        g_st.lfo_fold[ts] = (uint8_t)v;
    }
    hud_show((uint32_t)P_LFO_TARGET + col, 255u);
    g_ui_dirty = 1u;
}

static void lfo_page2_adjust(uint32_t col, int32_t delta)
{
    uint32_t ts = midi_lfo_target_slot(&g_cfg);
    int32_t v;
    int edit = (fm1_in.buttons >> BTN_EDIT) & 1u;

    if (delta == 0)
        return;
    if (col == 0u) {
        if (edit) {
            edit_combo_mark();
            g_st.lfo_algo[ts] = delta < 0 ? 0u : (uint8_t)(FM_LFO_NALGOS - 1u);
        } else {
            v = (int32_t)g_st.lfo_algo[ts] + (delta > 0 ? 1 : -1);
            if (v < 0)
                v = 0;
            if (v > (int32_t)(FM_LFO_NALGOS - 1u))
                v = (int32_t)(FM_LFO_NALGOS - 1u);
            g_st.lfo_algo[ts] = (uint8_t)v;
        }
    } else if (col == 1u) {
        if (edit) {
            edit_combo_mark();
            g_st.lfo_ratio[ts] = delta < 0 ? 52u : 127u;
        } else {
            v = (int32_t)g_st.lfo_ratio[ts] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            g_st.lfo_ratio[ts] = (uint8_t)v;
        }
    } else if (col == 2u) {
        if (edit) {
            edit_combo_mark();
            g_st.lfo_index[ts] = delta < 0 ? 64u : 127u;
        } else {
            v = (int32_t)g_st.lfo_index[ts] + delta;
            if (v < 0)
                v = 0;
            if (v > 127)
                v = 127;
            g_st.lfo_index[ts] = (uint8_t)v;
        }
    } else if (edit) {
        edit_combo_mark();
        g_st.lfo_fb[ts] = delta < 0 ? 0u : 127u;
    } else {
        v = (int32_t)g_st.lfo_fb[ts] + delta;
        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
        g_st.lfo_fb[ts] = (uint8_t)v;
    }
    hud_show((uint32_t)P_LFO_TARGET + col, 255u);
    g_ui_dirty = 1u;
}

static void param_adjust(uint32_t slot, int32_t delta)
{
    int32_t v;

    if (g_cfg.bank == BANK_LFO && g_st.lfo_page == 0u && g_perf.page == PERF_PAGE_BANK &&
        (fm1_in.buttons >> BTN_EDIT) & 1u && slot >= (uint32_t)P_LFO_TARGET &&
        slot <= (uint32_t)P_LFO_SHAPE && delta != 0) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        uint32_t col = slot - (uint32_t)P_LFO_TARGET;

        edit_combo_mark();
        if (col == 0u) {
            v = delta < 0 ? 0 : 127;
            g_cfg.base_val[P_LFO_TARGET] = (uint8_t)v;
            g_cfg.val[P_LFO_TARGET] = (uint8_t)v;
        } else if (col == 1u) {
            v = delta < 0 ? (int32_t)fm_lfo_rate_min(g_st.bpm) : 127;
            g_cfg.base_val[P_LFO_RATE] = (uint8_t)v;
            g_cfg.val[P_LFO_RATE] = (uint8_t)v;
            g_st.lfo_rate[ts] = fm_lfo_rate_clamp((uint8_t)v, g_st.bpm);
        } else if (col == 2u) {
            v = delta < 0 ? 0 : 127;
            g_cfg.base_val[P_LFO_DEPTH] = (uint8_t)v;
            g_cfg.val[P_LFO_DEPTH] = (uint8_t)v;
            midi_lfo_set_depth(&g_cfg, &g_st, ts, (uint8_t)v);
        } else {
            v = delta < 0 ? 0 : 127;
            g_cfg.base_val[P_LFO_SHAPE] = (uint8_t)v;
            g_cfg.val[P_LFO_SHAPE] = (uint8_t)v;
            g_st.lfo_shape[ts] = (uint8_t)v;
        }
        if (col == 0u) {
            g_cfg.base_val[P_LFO_DEPTH] = midi_lfo_get_depth(&g_cfg, &g_st, ts);
            g_cfg.val[P_LFO_DEPTH] = g_cfg.base_val[P_LFO_DEPTH];
            midi_lfo_recall_target_rate(&g_cfg, &g_st);
        }
        midi_mod_hold_arm(&g_cfg, &g_st, slot, fm1_ms);
        hud_show(slot, 255u);
        midi_send(0xB0, g_cfg.cc_map[slot], (uint32_t)g_cfg.base_val[slot]);
        mark_cfg_dirty();
        g_ui_dirty = 1u;
        return;
    }

    if (g_cfg.bank == BANK_LFO && g_st.lfo_page == 2u && g_perf.page == PERF_PAGE_BANK &&
        slot >= (uint32_t)P_LFO_TARGET && slot <= (uint32_t)P_LFO_SHAPE) {
        lfo_page2_adjust(slot - (uint32_t)P_LFO_TARGET, delta);
        return;
    }
    if (g_cfg.bank == BANK_LFO && g_st.lfo_page == 1u && g_perf.page == PERF_PAGE_BANK &&
        slot >= (uint32_t)P_LFO_TARGET && slot <= (uint32_t)P_LFO_SHAPE) {
        lfo_page1_adjust(slot - (uint32_t)P_LFO_TARGET, delta);
        return;
    }

    if (g_perf.page == PERF_PAGE_BANK && ((fm1_in.buttons >> BTN_EDIT) & 1u) && delta != 0 &&
        g_cfg.bank != BANK_LFO) {
        edit_combo_mark();
        v = delta < 0 ? (int32_t)param_slot_edit_default(slot) : 127;
        g_cfg.base_val[slot] = (uint8_t)v;
        g_cfg.val[slot] = (uint8_t)v;
        midi_mod_hold_arm(&g_cfg, &g_st, slot, fm1_ms);
        param_slot_side_effects(slot);
        hud_show(slot, 255u);
        midi_send(0xB0, g_cfg.cc_map[slot], (uint32_t)v);
        mark_cfg_dirty();
        return;
    }

    perf_rec_capture_knob(slot, delta);
    if (slot == (uint32_t)P_LFO_TARGET) {
        uint32_t ti = midi_lfo_target_idx(&g_cfg);

        if (delta > 0 && ti + 1u < NLFO_TARGETS)
            ti++;
        else if (delta < 0 && ti > 0u)
            ti--;
        v = (int32_t)midi_lfo_target_cc_from_idx(ti);
    } else if (g_cfg.mode == MODE_CONTURA && slot == (uint32_t)P_VEL_MOD) {
        uint8_t cur = g_cfg.base_val[slot];
        int32_t steps = delta;

        while (steps > 0) {
            cur = perf_enum_step_cc(cur, 1, 12u);
            steps--;
        }
        while (steps < 0) {
            cur = perf_enum_step_cc(cur, -1, 12u);
            steps++;
        }
        v = (int32_t)cur;
    } else if (slot == (uint32_t)P_LFO_SHAPE) {
        int edit = (fm1_in.buttons >> BTN_EDIT) & 1u;

        v = (int32_t)fm_lfo_step_shape(g_cfg.base_val[slot], delta, edit ? 1u : 0u);
    } else {
        int32_t step = knob_step_for(slot, delta);

        v = (int32_t)g_cfg.base_val[slot] + step;
    }

    if (v < 0)
        v = 0;
    if (v > 127)
        v = 127;
    if (slot == (uint32_t)P_LFO_RATE)
        v = (int32_t)fm_lfo_rate_clamp((uint8_t)v, g_st.bpm);
    g_cfg.base_val[slot] = (uint8_t)v;
    g_cfg.val[slot] = (uint8_t)v;
    midi_mod_hold_arm(&g_cfg, &g_st, slot, fm1_ms);
    if (slot == (uint32_t)P_LFO_TARGET) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        g_cfg.base_val[P_LFO_DEPTH] = midi_lfo_get_depth(&g_cfg, &g_st, ts);
        g_cfg.val[P_LFO_DEPTH] = g_cfg.base_val[P_LFO_DEPTH];
        midi_lfo_recall_target_rate(&g_cfg, &g_st);
    } else if (slot == (uint32_t)P_LFO_DEPTH) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        midi_lfo_set_depth(&g_cfg, &g_st, ts, (uint8_t)v);
    } else if (slot == (uint32_t)P_LFO_RATE) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        g_st.lfo_rate[ts] = (uint8_t)v;
    } else if (slot == (uint32_t)P_LFO_SHAPE) {
        uint32_t ts = midi_lfo_target_slot(&g_cfg);
        g_st.lfo_shape[ts] = (uint8_t)v;
    }
    if (slot == P_TEMPO && !g_st.ext_sync)
        g_st.bpm = (uint16_t)(60u + g_cfg.val[P_TEMPO]);
    if (slot == (uint32_t)P_PARAM_B && g_cfg.mode == MODE_DITDAH) {
        ditdah_select_phrase(&g_ditdah, &g_cfg);
        ditdah_recompile(&g_ditdah, &g_cfg);
    }
    hud_show(slot, 255u);
    midi_send(0xB0, g_cfg.cc_map[slot], (uint32_t)v);
    mark_cfg_dirty();
}

static void ditdah_midi_cipher(uint8_t note, int32_t oct)
{
    uint32_t base = (uint32_t)(KEY0 + 12 * oct);
    uint32_t ki;

    if (note >= 60u && note <= 86u)
        ki = note - 60u;
    else if (note >= base && note < base + 27u)
        ki = note - base;
    else
        return;
    ditdah_key(&g_ditdah, &g_cfg, ki);
    ditdah_recompile(&g_ditdah, &g_cfg);
    mark_cfg_dirty();
}

static uint32_t prev_keys = 0, lit_led = ~0u;
static int was_hud = 0;
static uint8_t held[27];

static void tryp_cfg_copy_v1_prefix(midi_cfg_t *c, const midi_cfg_v1_t *o)
{
    c->magic = TRYP_CFG_MAGIC;
    memcpy(c->val, o->val, sizeof o->val);
    memcpy(c->base_val, o->base_val, sizeof o->base_val);
    memcpy(c->lfo_depth, o->lfo_depth, sizeof o->lfo_depth);
    memcpy(c->cc_map, o->cc_map, sizeof o->cc_map);
    c->midi_ch = o->midi_ch;
    c->bank = o->bank;
    c->mode = o->mode;
    c->reserved = o->reserved;
    c->ratio_lfo_q8 = o->ratio_lfo_q8;
    memcpy(c->phrases, o->phrases, sizeof o->phrases);
}

static void tryp_cfg_from_v0(midi_cfg_t *c, const midi_cfg_v0_t *o)
{
    memcpy(c->val, o->val, sizeof o->val);
    memcpy(c->base_val, o->base_val, sizeof o->base_val);
    memcpy(c->lfo_depth, o->lfo_depth, sizeof o->lfo_depth);
    memcpy(c->cc_map, o->cc_map, sizeof o->cc_map);
    c->midi_ch = o->midi_ch;
    c->bank = o->bank;
    c->mode = o->mode;
    c->reserved = o->reserved;
    c->ratio_lfo_q8 = o->ratio_lfo_q8;
    memcpy(c->phrases, o->phrases, sizeof o->phrases);
    c->magic = TRYP_CFG_MAGIC;
    midi_cfg_tail_defaults(c);
}

static void tryp_cfg_from_v1(midi_cfg_t *c, const midi_cfg_v1_t *o)
{
    tryp_cfg_copy_v1_prefix(c, o);
    midi_cfg_tail_defaults(c);
}

static void tryp_cfg_store(midi_cfg_t *c)
{
    uint32_t i;

    memcpy(c->lfo_rate, g_st.lfo_rate, sizeof c->lfo_rate);
    memcpy(c->lfo_shape, g_st.lfo_shape, sizeof c->lfo_shape);
    memcpy(c->lfo_algo, g_st.lfo_algo, sizeof c->lfo_algo);
    memcpy(c->lfo_ratio, g_st.lfo_ratio, sizeof c->lfo_ratio);
    memcpy(c->lfo_index, g_st.lfo_index, sizeof c->lfo_index);
    memcpy(c->lfo_fb, g_st.lfo_fb, sizeof c->lfo_fb);
    memcpy(c->lfo_fold, g_st.lfo_fold, sizeof c->lfo_fold);
    memcpy(c->lfo_phase, g_st.lfo_phase, sizeof c->lfo_phase);
    memcpy(c->lfo_skew, g_st.lfo_skew, sizeof c->lfo_skew);
    memcpy(c->lfo_smooth, g_st.lfo_smooth, sizeof c->lfo_smooth);
    memcpy(c->lfo_depth_ext, g_st.lfo_depth_ext, sizeof c->lfo_depth_ext);
    memcpy(c->arp_base, g_perf.arp_base, 4u);
    memcpy(c->seq_base, g_perf.seq_base, 4u);
    memcpy(c->rec, g_perf.rec, 4u);
    memcpy(c->seq_steps, g_perf.seq_steps, sizeof c->seq_steps);
    c->page = g_perf.page;
    for (i = 0; i < NMODES; i++) {
        memcpy(c->mode_mem[i], g_mode_mem[i], 11u);
        c->mode_mem_ok[i] = g_mode_mem_ok[i];
    }
}

static void tryp_cfg_unpack(midi_cfg_t *c)
{
    uint32_t i;

    memcpy(g_st.lfo_rate, c->lfo_rate, sizeof g_st.lfo_rate);
    memcpy(g_st.lfo_shape, c->lfo_shape, sizeof g_st.lfo_shape);
    memcpy(g_st.lfo_algo, c->lfo_algo, sizeof g_st.lfo_algo);
    memcpy(g_st.lfo_ratio, c->lfo_ratio, sizeof g_st.lfo_ratio);
    memcpy(g_st.lfo_index, c->lfo_index, sizeof g_st.lfo_index);
    memcpy(g_st.lfo_fb, c->lfo_fb, sizeof g_st.lfo_fb);
    memcpy(g_st.lfo_fold, c->lfo_fold, sizeof g_st.lfo_fold);
    memcpy(g_st.lfo_phase, c->lfo_phase, sizeof g_st.lfo_phase);
    memcpy(g_st.lfo_skew, c->lfo_skew, sizeof g_st.lfo_skew);
    memcpy(g_st.lfo_smooth, c->lfo_smooth, sizeof g_st.lfo_smooth);
    memcpy(g_st.lfo_depth_ext, c->lfo_depth_ext, sizeof g_st.lfo_depth_ext);
    memcpy(g_perf.arp_base, c->arp_base, 4u);
    memcpy(g_perf.seq_base, c->seq_base, 4u);
    memcpy(g_perf.rec, c->rec, 4u);
    memcpy(g_perf.seq_steps, c->seq_steps, sizeof g_perf.seq_steps);
    g_perf.page = c->page;
    g_perf.page_last = c->page;
    g_perf.rec[0] = 0u;
    for (i = 0; i < NMODES; i++) {
        memcpy(g_mode_mem[i], c->mode_mem[i], 11u);
        g_mode_mem_ok[i] = c->mode_mem_ok[i];
    }
    perf_lfo_apply(&g_perf, c, &g_st);
}

static int tryp_cfg_load(midi_cfg_t *c)
{
    if (settings_load(c, (uint32_t)sizeof *c) == 0 && c->magic == TRYP_CFG_MAGIC)
        return 0;
    {
        midi_cfg_v1_t v1;

        if (settings_load(&v1, (uint32_t)sizeof v1) == 0 && v1.magic == TRYP_CFG_MAGIC_V1) {
            tryp_cfg_from_v1(c, &v1);
            return 0;
        }
    }
    {
        midi_cfg_v0_t o;

        if (settings_load(&o, (uint32_t)sizeof o) == 0) {
            tryp_cfg_from_v0(c, &o);
            return 0;
        }
    }
    midi_cfg_defaults(c);
    return -1;
}

static void tryp_cfg_persist(void)
{
    tryp_cfg_store(&g_cfg);
    settings_save(&g_cfg, sizeof g_cfg);
}

#if defined(DINGHY_HOST)
static void tryp_cfg_host_dump(const midi_cfg_t *c)
{
    static const uint32_t MODE_MEM_AXIS = 4u; /* P_PARAM_A in MODE_MEM_SLOTS */

    printf("cfg magic=%08x mode=%u ch=%u tempo=%u timbre=%u phrase0=%s "
           "lfo_r0=%u seq_s0=%d contura_scale=%u live_axis=%u\n",
           (unsigned)c->magic, (unsigned)c->mode, (unsigned)c->midi_ch,
           (unsigned)c->base_val[P_TEMPO], (unsigned)c->base_val[P_TIMBRE], c->phrases[0],
           (unsigned)g_st.lfo_rate[0], (int)g_perf.seq_steps[0],
           c->mode_mem_ok[MODE_CONTURA] ? (unsigned)c->mode_mem[MODE_CONTURA][MODE_MEM_AXIS] : 0u,
           (unsigned)c->base_val[P_PARAM_A]);
}
#endif

static void app_init(void)
{
    uint32_t k;

    palette_set(2);
    for (k = 0; k < 27u; k++)
        held[k] = 0;
    tryp_cfg_load(&g_cfg);
    g_cfg.mode %= NMODES;
    midi_state_init(&g_st, &g_cfg);
    perf_init(&g_perf);
    tryp_cfg_unpack(&g_cfg);
    g_st.playing = 1u;
    mode_mem_save(g_cfg.mode);
#if defined(DINGHY_HOST)
    tryp_cfg_host_dump(&g_cfg);
#endif
    engine_restart_for_mode(g_cfg.mode);
    g_mode_prev = g_cfg.mode;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui_redraw();
}

static void app_step(void)
{
    uint32_t keys, down, up, rel, b, base;
    uint32_t k;
        int32_t oct;
        uint8_t st, d1, d2;
        int redraw = 0;
        int modal_ui = bank_confirm_active() || (fm1_ms < g_saved_until);

        platform_poll();
        midi_lfo_tick(&g_cfg, &g_st, fm1_ms);
        perf_lfo_apply(&g_perf, &g_cfg, &g_st);
        if (!modal_ui &&
            (g_cfg.bank == BANK_LFO || midi_lfo_any_active(&g_cfg, &g_st) ||
             (g_st.playing && (perf_arp_active(&g_perf) || perf_seq_active(&g_perf) ||
                               perf_rec_active(&g_perf)))) &&
            fm1_ms - g_ui_ms >= 33u)
            g_ui_dirty = 1u;
        {
            uint32_t frames_now = g_audio_frames;
            uint32_t ticks;

            midi_clock_poll_frames(&g_cfg, &g_st, frames_now);
            ticks = midi_clock_poll(&g_cfg, &g_st, fm1_ms);

            if (ticks) {
                perf_pre_tick(&g_perf, &g_cfg, ticks);
                if (g_cfg.mode == MODE_TRIAXIS)
                    triptych_tick(&g_triptych, &g_cfg, ticks);
                else if (g_cfg.mode == MODE_CONTURA)
                    contura_tick(&g_contura, &g_cfg, ticks);
                else
                    ditdah_tick(&g_ditdah, &g_cfg, ticks);
                perf_post_tick(&g_perf, &g_cfg, ticks);
            }
            if (ticks && !modal_ui && fm1_ms - g_ui_ms >= 33u)
                g_ui_dirty = 1u;
        }
        ditdah_midi_poll(&g_cfg);

        oct = midi_oct_offset(&g_cfg);
        base = (uint32_t)(KEY0 + 12 * oct);
        keys = fm1_in.notes;
        down = keys & ~prev_keys;
        up = prev_keys & ~keys;
        prev_keys = keys;

        for (k = 0; k < 27u; k++) {
            if (down >> k & 1u) {
                uint8_t note = (uint8_t)(base + k);

                held[k] = note;
                g_last_note = note;
                {
                    uint32_t rst = perf_rec_state(&g_perf);
                    uint32_t rtgt = ((uint32_t)g_perf.rec[1] * 3u) / 128u;
                    int capture = 0;

                    if (rst == 1u && (rtgt == 0u || rtgt == 2u))
                        perf_rec_begin_from_arm();
                    rst = perf_rec_state(&g_perf);
                    if ((rst == 2u || rst == 4u) && (rtgt == 0u || rtgt == 2u))
                        capture = 1;
                    if (perf_seq_active(&g_perf) && g_perf.page == PERF_PAGE_SEQ)
                        capture = 1;
                    if (capture) {
                        int32_t base = (int32_t)perf_capture_root();
                        int32_t semi = (int32_t)note - base;

                        while (semi < -12)
                            semi += 12;
                        while (semi > 12)
                            semi -= 12;
                        {
                            uint32_t cap_n = perf_seq_nsteps(g_perf.seq[0]);

                            if (cap_n == 0u)
                                cap_n = PERF_SEQ_MAX_STEPS;
                            g_perf.seq_steps[g_perf.rec_user_step % cap_n] = (int8_t)semi;
                        }
                        g_perf.rec_user_step++;
                        g_perf.seq[2] = (uint8_t)((7u * 128u) / 8u);
                    }
                }
                if (g_cfg.mode == MODE_DITDAH) {
                    if (g_ditdah.edit_active) {
                        ditdah_key(&g_ditdah, &g_cfg, k);
                        ditdah_recompile(&g_ditdah, &g_cfg);
                        mark_cfg_dirty();
                    } else {
                        ditdah_set_base(&g_ditdah, note);
                        synth_note_on(note, 100u, &g_cfg, &g_last_note);
                        trm_on(tryp_keys_on_st(&g_cfg), note, 100);
                    }
                } else if (g_cfg.mode == MODE_TRIAXIS) {
                    triptych_set_root(&g_triptych, note);
                    synth_note_on(note, 100u, &g_cfg, &g_last_note);
                    trm_on(tryp_keys_on_st(&g_cfg), note, 100);
                } else {
                    contura_set_tonic(&g_contura, note);
                    synth_note_on(note, 100u, &g_cfg, &g_last_note);
                    trm_on(tryp_keys_on_st(&g_cfg), note, 100);
                }
                g_ui_dirty = 1u;
            }
            if (up >> k & 1u) {
                if (!(g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active)) {
                    synth_note_off(held[k]);
                    trm_off(tryp_keys_off_st(&g_cfg), held[k]);
                }
            }
        }

        b = fm1_input_edges(&rel);
        if (bank_confirm_active() && b) {
            uint8_t kind = g_confirm_kind;

            if ((b & (1u << BTN_HOME)) && kind == 2u)
                g_home_view = 0u;
            bank_confirm_close();
            b = 0u;
        }
        for (k = 0; k < 4u; k++) {
            uint8_t btn = BANK_BTN[k].btn;
            uint8_t bank = BANK_BTN[k].bank;

            if (b & (1u << btn)) {
                if (!g_home_view && g_perf.page == PERF_PAGE_BANK && g_cfg.bank == bank)
                    bank_confirm_open(0u, bank);
                else {
                    g_home_view = 0u;
                    perf_page_bank();
                    g_cfg.bank = bank;
                    mark_cfg_dirty();
                }
            }
        }
        if (b & (1u << BTN_LFO)) {
            if (!g_home_view && g_perf.page == PERF_PAGE_BANK && g_cfg.bank == BANK_LFO) {
                if ((fm1_in.buttons >> BTN_EDIT) & 1u)
                    bank_confirm_open(0u, BANK_LFO);
                else {
                    g_st.lfo_page = (uint8_t)((g_st.lfo_page + 1u) % 3u);
                    g_ui_dirty = 1u;
                }
            } else {
                g_home_view = 0u;
                perf_page_bank();
                g_cfg.bank = BANK_LFO;
                lfo_target_from_last();
                mark_cfg_dirty();
            }
        }
        if (b & (1u << BTN_ARP)) {
            if (g_perf.page == PERF_PAGE_ARP && !g_home_view)
                bank_confirm_open(1u, PERF_PAGE_ARP);
            else
                perf_page_open(PERF_PAGE_ARP);
        }
        if (b & (1u << BTN_SEQ)) {
            if (!(g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active)) {
                if (g_perf.page == PERF_PAGE_SEQ && !g_home_view)
                    bank_confirm_open(1u, PERF_PAGE_SEQ);
                else
                    perf_page_open(PERF_PAGE_SEQ);
            }
        }
        if (b & (1u << BTN_OCT_DOWN)) {
            int32_t v = (int32_t)g_cfg.base_val[P_OCTAVE] - 26;
            if (v < 0)
                v = 0;
            g_cfg.base_val[P_OCTAVE] = (uint8_t)v;
            g_cfg.val[P_OCTAVE] = (uint8_t)v;
            mark_cfg_dirty();
        }
        if (b & (1u << BTN_OCT_UP)) {
            int32_t v = (int32_t)g_cfg.base_val[P_OCTAVE] + 26;
            if (v > 127)
                v = 127;
            g_cfg.base_val[P_OCTAVE] = (uint8_t)v;
            g_cfg.val[P_OCTAVE] = (uint8_t)v;
            mark_cfg_dirty();
        }
        if (b & (1u << BTN_EDIT))
            g_edit_combo_used = 0u;
        if (rel & (1u << BTN_EDIT)) {
            if (!g_edit_combo_used) {
                if (g_cfg.mode == MODE_DITDAH) {
                    g_ditdah.edit_active = g_ditdah.edit_active ? 0u : 1u;
                    g_st.learn_active = 0u;
                } else {
                    g_st.learn_active = g_st.learn_active ? 0u : 1u;
                    if (g_st.learn_active) {
                        g_st.learn_slot = (uint8_t)((g_cfg.bank * 4u) % NPARAMS);
                        g_st.hud_until_ms = fm1_ms + 6000u; /* auto-exit MIDI learn after 6 s */
                    }
                }
            }
            g_ui_dirty = 1u;
        }
        if (g_st.learn_active && fm1_ms >= g_st.hud_until_ms) {
            g_st.learn_active = 0u;
            g_ui_dirty = 1u;
        }
        if (b & (1u << BTN_PLAY)) {
            g_st.playing = g_st.playing ? 0u : 1u;
            if (!g_st.playing) {
                synth_all_notes_off();
                trm_panic();
            } else if (g_cfg.mode == MODE_DITDAH) {
                g_ditdah.ev_pos = 0;
                g_ditdah.unit_left = 0;
                g_ditdah.tick_acc = 96u;
            }
            g_ui_dirty = 1u;
        }
        if (b & (1u << BTN_REC)) {
            if (g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active) {
                ditdah_backspace(&g_ditdah, &g_cfg);
                ditdah_recompile(&g_ditdah, &g_cfg);
                mark_cfg_dirty();
            } else if (g_perf.page == PERF_PAGE_REC && !g_home_view) {
                bank_confirm_open(1u, PERF_PAGE_REC);
            } else {
                perf_page_open(PERF_PAGE_REC);
            }
        }
        if (b & (1u << BTN_SEQ) && g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active) {
            ditdah_clear(&g_ditdah, &g_cfg);
            mark_cfg_dirty();
        }
        if (b & (1u << BTN_SAVE)) {
            synth_all_notes_off();
            trm_panic();
            tryp_cfg_persist();
            g_set_dirty = 0u;
            g_saved_until = fm1_ms + 1200u;
            g_ui_dirty = 1u;
        }
        if (b & (1u << BTN_HOME)) {
            if (g_st.learn_active) {
                g_st.learn_active = 0u;
            } else if (g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active) {
                g_ditdah.edit_active = 0u;
            } else if (g_home_view && !bank_confirm_active()) {
                bank_confirm_open(2u, 0u);
                g_st.hud_until_ms = 0u;
            } else {
                uint8_t entering = g_home_view ? 0u : 1u;

                g_home_view = entering;
                if (entering && g_perf.page == PERF_PAGE_REC)
                    perf_page_bank();
                g_st.hud_until_ms = 0u;
            }
            g_ui_dirty = 1u;
        }

        {
            int32_t msteps = fm1_enc_take(ENC_ALGORITHM);
            if (msteps && !bank_confirm_active()) {
                int32_t md = (int32_t)g_cfg.mode + (msteps > 0 ? 1 : -1);
                while (md < 0)
                    md += NMODES;
                g_cfg.mode = (uint8_t)(md % NMODES);
                mark_cfg_dirty();
                engine_mode_sync();
                g_ui_dirty = 1u;
            }
        }
        {
            int32_t psteps = fm1_enc_take(ENC_PRESETS);
            if (psteps && !bank_confirm_active()) {
                /* PRESETS knob steps cleanly through main selector (CYCLIC / Scale / Phrase) */
                uint32_t pslot = (g_cfg.mode == MODE_DITDAH) ? (uint32_t)P_PARAM_B : (uint32_t)P_PARAM_A;
                param_adjust(pslot, psteps);
                home_hud_arm_param((uint8_t)pslot);
            }
        }
        {
            int32_t steps = fm1_enc_take(ENC_SELECT);
            if (steps) {
                if (g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active) {
                    /* In DIT DAH Edit mode, SELECT moves the letter cursor across the phrase! */
                    ditdah_cursor_move(&g_ditdah, &g_cfg, steps);
                } else if (bank_confirm_active()) {
                    if (steps < 0) {
                        bank_confirm_apply_reset();
                        bank_confirm_close();
                    } else if (steps > 0 && bank_confirm_has_random()) {
                        bank_confirm_apply_random();
                        bank_confirm_close();
                    }
                } else if (g_st.learn_active) {
                    int32_t s = (int32_t)g_st.learn_slot + steps;
                    while (s < 0)
                        s += NPARAMS;
                    g_st.learn_slot = (uint8_t)(s % NPARAMS);
                } else {
                    int32_t ix = perf_select_walk_ix() + steps;

                    perf_select_walk_apply(ix);
                    mark_cfg_dirty();
                    home_hud_arm_bank();
                }
                g_ui_dirty = 1u;
            }
        }
        if (!bank_confirm_active()) {
            for (k = 0; k < 4u; k++) {
                int32_t steps = fm1_enc_take(ENC_KNOB1 + k);
                if (steps) {
                    if (g_perf.page != PERF_PAGE_BANK) {
                        perf_param_adjust(k, steps);
                        home_hud_arm_param((uint8_t)k);
                    } else {
                        param_adjust((uint32_t)g_cfg.bank * 4u + k, steps);
                        home_hud_arm_param((uint8_t)(g_cfg.bank * 4u + k));
                    }
                }
            }
        } else {
            for (k = 0; k < 4u; k++)
                (void)fm1_enc_take(ENC_KNOB1 + k);
        }

        while (midi_get(&st, &d1, &d2)) {
            if (st >= 0xF8u) {
                uint8_t was_playing = g_st.playing;

                midi_clock_rx(&g_st, st, fm1_ms);
                if (st == 0xFCu && was_playing && !g_st.playing) {
                    synth_all_notes_off();
                    trm_panic();
                }
                g_ui_dirty = 1u;
            } else if ((st & 0xF0u) == 0xB0u) {
                int r = midi_perf_handle_cc(&g_perf, &g_cfg, &g_st, st & 0x0Fu, d1, d2, fm1_ms);
                if (!r)
                    r = midi_handle_cc(&g_cfg, &g_st, st & 0x0Fu, d1, d2, fm1_ms);
                if (r) {
                    perf_lfo_apply(&g_perf, &g_cfg, &g_st);
                    if (d1 == 31u)
                        engine_mode_sync();
                    if (g_cfg.mode == MODE_DITDAH && d1 == g_cfg.cc_map[P_PARAM_B]) {
                        ditdah_select_phrase(&g_ditdah, &g_cfg);
                        ditdah_recompile(&g_ditdah, &g_cfg);
                    }
                    mark_cfg_dirty();
                }
                g_ui_dirty = 1u;
            } else if ((st & 0xF0u) == 0xC0u && midi_in_ch_ok(&g_cfg, st & 0x0Fu)) {
                g_cfg.mode = (uint8_t)(d1 % NMODES);
                mark_cfg_dirty();
                engine_mode_sync();
            } else if ((st & 0xF0u) == 0x90u && d2 && midi_in_ch_ok(&g_cfg, st & 0x0Fu)) {
                g_last_note = d1;
                if (g_cfg.mode == MODE_TRIAXIS)
                    triptych_set_root(&g_triptych, d1);
                else if (g_cfg.mode == MODE_CONTURA)
                    contura_set_tonic(&g_contura, d1);
                else if (g_ditdah.edit_active)
                    ditdah_midi_cipher(d1, oct);
                else
                    ditdah_set_base(&g_ditdah, d1);
                g_ui_dirty = 1u;
            } else if (((st & 0xF0u) == 0x80u || ((st & 0xF0u) == 0x90u && !d2)) &&
                       midi_in_ch_ok(&g_cfg, st & 0x0Fu)) {
                (void)d1;
            }
        }

        {
            uint32_t lit = (g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active) ? keys : synth_lit_keys(base);
            if (lit != lit_led) {
                lit_led = lit;
                g_ui_dirty = 1u;
            }
            for (k = 0; k < 27u; k++)
                fm1_led_key(KEY_LED(k), lit >> k & 1u);
            fm1_led_key(BTN_OCT_DOWN, oct < 0);
            fm1_led_key(BTN_OCT_UP, oct > 0);
            for (k = 0; k < 4u; k++) {
                uint8_t btn = BANK_BTN[k].btn;
                uint8_t bank = BANK_BTN[k].bank;
                int on_bank = !g_home_view && g_perf.page == PERF_PAGE_BANK && g_cfg.bank == bank;

                fm1_led_key(btn, on_bank);
                fm1_led_key_dim(btn, g_home_view && g_perf.page == PERF_PAGE_BANK && g_cfg.bank == bank);
            }
            {
                uint8_t lfo_lamp = 0;
                if (!g_home_view && g_perf.page == PERF_PAGE_BANK && g_cfg.bank == BANK_LFO)
                    lfo_lamp = 255u;
                else if (midi_lfo_any_active(&g_cfg, &g_st)) {
                    uint32_t ph = fm1_ms * 2147484u; /* ~0.5 Hz */
                    int32_t w = fm_lfo_classic_at(ph, 0u, 0u, 0u);
                    lfo_lamp = (uint8_t)((uint32_t)(w + 64) * 255u / 127u);
                }
                fm1_led_key_level(BTN_LFO, lfo_lamp);
                fm1_led_key_dim(BTN_LFO, g_home_view && g_perf.page == PERF_PAGE_BANK && g_cfg.bank == BANK_LFO && lfo_lamp == 0u);
            }
            {
                uint8_t arp_l = perf_btn_lamp(BTN_ARP, PERF_PAGE_ARP);
                uint8_t seq_l = perf_btn_lamp(BTN_SEQ, PERF_PAGE_SEQ);
                uint8_t rec_l = perf_btn_lamp(BTN_REC, PERF_PAGE_REC);

                fm1_led_key_level(BTN_ARP, arp_l);
                fm1_led_key_level(BTN_SEQ, seq_l);
                fm1_led_key_level(BTN_REC, rec_l);
                fm1_led_key_dim(BTN_ARP, g_home_view && g_perf.page == PERF_PAGE_ARP && arp_l == 0u);
                fm1_led_key_dim(BTN_SEQ, g_home_view && g_perf.page == PERF_PAGE_SEQ && seq_l == 0u);
                fm1_led_key_dim(BTN_REC, g_home_view && g_perf.page == PERF_PAGE_REC && rec_l == 0u);
            }
            fm1_led_key(BTN_EDIT, g_st.learn_active || (g_cfg.mode == MODE_DITDAH && g_ditdah.edit_active));
            fm1_led_key(BTN_PLAY, g_st.playing);
        }

        {
            int hud_now = g_st.learn_active || (fm1_ms < g_st.hud_until_ms) ||
                          bank_confirm_active() || (fm1_ms < g_saved_until) ||
                          (g_home_view && fm1_ms < g_home_hud_until);
            if (hud_now != was_hud) {
                was_hud = hud_now;
                g_ui_dirty = 1u;
            }
            if (g_ui_dirty)
                redraw = 1;
        }
        if (redraw)
            ui_redraw();
        else if (!modal_ui && fm1_ms - g_ui_ms >= 50u) {
            if (g_home_view)
                draw_home_screen();
            else
                draw_param_line();
            g_ui_ms = fm1_ms;
        }

        if (g_set_dirty && synth_is_quiet() && fm1_ms - g_set_t > 2000u) {
            tryp_cfg_persist();
            g_set_dirty = 0u;
        }
}

static void app_main(void)
{
    app_init();
#if defined(DINGHY_HOST)
    for (;;) {
        app_step();
        if (!host_sim_should_stop())
            continue;
        while (ditdah_midi_until_ms > fm1_ms)
            app_step();
        ditdah_midi_poll(&g_cfg);
        tryp_cfg_host_dump(&g_cfg);
        host_sim_finalize();
    }
#else
    for (;;)
        app_step();
#endif
}
