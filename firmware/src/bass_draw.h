/* SPDX-License-Identifier: GPL-3.0-only */
/* Included from fm_app.c when GENRE_BASS — after dub_app_t g and helpers exist. */
#ifndef BASS_DRAW_H
#define BASS_DRAW_H

#include "fm_env_kit.h"
#define FM_GROOVE_PLOT_SINE dub_sine_q64
#include "fm_groove_plot.h"

/* Knob-bar labels for fm_app knob_label() — wire GENRE_BASS branches to these. */
static const char *const BASS_HOME_P0_LBL[4] = {"CUTOFF", "RESO", "DRIVE", "DELAY"};
static const char *const BASS_HOME_P1_LBL[4] = {"GLIDE", "ACID", "GRIT", "SPACE"};
static const char *const BASS_HOME_P2_LBL[4] = {"GATE", "DENS", "RATCH", "SHIFT"};
static const char *const BASS_HOME_ROUTE_LBL[4] = {"MACRO", "SLOT", "DEST", "AMOUNT"};
/* ENV page 3 cols 1..3 (KIT: P.DEC/SWEEP) carry the env mod routing; page 4 = routing slot CURVE. */
static const char *const BASS_ENV_ROUTE_LBL[4] = {"INVERT", "ACCENT", "DEST", "AMOUNT"};

static void bass_draw_home_knob_label(uint8_t pg, uint8_t slot, char *s)
{
    if (pg == 0u)
        copy_str(s, BASS_HOME_P0_LBL[slot % 4u]);
    else if (pg == 1u)
        copy_str(s, BASS_HOME_P1_LBL[slot % 4u]);
    else if ((fm1_in.buttons & (1u << BTN_EDIT)) != 0u)
        copy_str(s, BASS_HOME_ROUTE_LBL[slot % 4u]);
    else
        copy_str(s, BASS_HOME_P2_LBL[slot % 4u]);
}

static void bass_draw_env_knob_label(uint8_t pg, uint8_t slot, char *s)
{
    uint8_t n = (uint8_t)(g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP);

    if (pg < 2u)
        copy_str(s, FM_ENV_KIT_LBL[pg][slot % 4u]);
    else if (pg == 2u)
        copy_str(s, BASS_ENV_ROUTE_LBL[slot % 4u]);
    else
        copy_str(s, slot == 0u ? "CURV " : "");
    /* DEST, AMOUNT and CURVE edit env-map slot n (EDIT + DEST picks it). */
    if ((pg == 2u && slot == 2u) || (pg >= 3u && slot == 0u)) {
        char *e = s;

        while (*e)
            e++;
        if (pg == 2u)
            *e++ = ' ';
        *e++ = (char)('1' + n);
        *e = 0;
    }
}

/* Mod dest → knob LFO target (255: no knob, or matched by position in bass_knob_mod_dest). MODEL is never
 * applied to the voice; AMP and M4 DEP have no knob. */
static const uint8_t BASS_MOD_DEST_LFO_TGT[BASS_MOD_N] = {
    [BASS_MOD_NONE] = 255u,
    [BASS_MOD_AMP] = 255u,
    [BASS_MOD_CUTOFF] = DUB_LFO_CUT_B,
    [BASS_MOD_RESO] = DUB_LFO_RESO_C,
    [BASS_MOD_DRIVE] = DUB_LFO_DRV_B,
    [BASS_MOD_DELAY] = DUB_LFO_DLY_B,
    [BASS_MOD_REV] = DUB_LFO_REV_B,
    [BASS_MOD_GLIDE] = DUB_LFO_ATK_B,
    [BASS_MOD_MODEL] = 255u,
    [BASS_MOD_ENV_DELAY] = DUB_LFO_ENV_DLY,
    [BASS_MOD_ENV_ATTACK] = DUB_LFO_ENV_ATK,
    [BASS_MOD_ENV_HOLD] = DUB_LFO_ENV_HOLD,
    [BASS_MOD_ENV_DECAY] = DUB_LFO_ENV_DEC,
    [BASS_MOD_ENV_SUSTAIN] = DUB_LFO_ENV_SUS,
    [BASS_MOD_ENV_RELEASE] = DUB_LFO_ENV_REL,
    [BASS_MOD_ENV_RETRIG] = 255u,
    [BASS_MOD_ENV_INVERT] = 255u,
    [BASS_MOD_ENV_ACCENT] = DUB_LFO_ENV_ACC,
    [BASS_MOD_SEQ_GATE] = 255u,
    [BASS_MOD_SEQ_RATCHET] = 255u,
    [BASS_MOD_SEQ_ROTATE] = 255u,
    [BASS_MOD_SEQ_DENSITY] = 255u,
    [BASS_MOD_M1_DEPTH] = DUB_LFO_M_CHORD,
    [BASS_MOD_M2_DEPTH] = DUB_LFO_M_FEED,
    [BASS_MOD_M3_DEPTH] = DUB_LFO_M_WOB,
    [BASS_MOD_M4_DEPTH] = 255u,
};

static uint8_t bass_knob_mod_dest(uint8_t slot)
{
    uint8_t pg = bank_page();
    uint8_t tgt;
    uint8_t d;

    if (g.view == VIEW_SEQ || g.seq_plock_hold)
        return BASS_MOD_NONE;
    if (g.bank == BANK_HOME && pg == 2u) {
        static const uint8_t PERF[4] = {BASS_MOD_SEQ_GATE, BASS_MOD_SEQ_DENSITY, BASS_MOD_SEQ_RATCHET,
                                        BASS_MOD_SEQ_ROTATE};

        if ((fm1_in.buttons & (1u << BTN_EDIT)) != 0u)
            return BASS_MOD_NONE;
        return PERF[slot % 4u];
    }
    if (g.bank == BANK_CHORD && pg == 1u && slot == 3u)
        return BASS_MOD_ENV_RETRIG;
    if (g.bank == BANK_CHORD && pg == 2u && slot == 0u)
        return BASS_MOD_ENV_INVERT;
    tgt = lfo_target_for_knob(slot);
    if (tgt == 255u)
        return BASS_MOD_NONE;
    for (d = 1u; d < BASS_MOD_N; d++)
        if (BASS_MOD_DEST_LFO_TGT[d] == tgt)
            return d;
    return BASS_MOD_NONE;
}

/* Knob param is a mod dest fed by a macro that is offset (raw != 64) or has a macro LFO. */
static int bass_knob_mod_routed(uint8_t slot)
{
    uint8_t dest = bass_knob_mod_dest(slot);
    uint8_t on = 0u;
    uint8_t mi;

    if (dest == BASS_MOD_NONE)
        return 0;
    for (mi = 0u; mi < BASS_MAC_AUX; mi++) {
        const uint8_t *mp = dub_macro_ptr(mi);

        if ((mp && *mp != 64u) || dub_lfo_tgt_modulated((uint8_t)(DUB_LFO_M_CHORD + mi)))
            on = (uint8_t)(on | (1u << mi));
    }
    return (int)((bass_mod_dests_live(&g.cfg.bass.mods, on) >> dest) & 1u);
}

static const char *const BASS_GROOVE_LBL[2][4] = {
    {"KEY", "SCALE", "SWING", "TEMPO"},
    {"MIDI", "CLOCK", "TRANSP", "VEL"},
};
static const char *const BASS_CLOCK_NAMES[BASS_CLOCK_N] = {"INT", "EXT", "INT+TX"};

static void bass_groove_knob_label(uint8_t pg, uint8_t slot, char *s)
{
    copy_str(s, BASS_GROOVE_LBL[pg ? 1u : 0u][slot % 4u]);
}

static void bass_groove_knob_value(uint8_t pg, uint8_t slot, char *s)
{
    if (pg == 0u) {
        uint8_t show_root = g.cue_on ? g.cue_root : g.cfg.p.root;

        if (slot == 0u)
            copy_str(s, NOTE_NAMES[show_root % 12u]);
        else if (slot == 1u)
            copy_str(s, SCALE_NAMES[scale_ix(dub_groove_show_scale())]);
        else if (slot == 2u)
            u8_dec(s, dub_groove_show_swing());
        else
            u8_dec(s, dub_groove_show_bpm());
        return;
    }
    if (slot == 0u)
        u8_dec(s, g.cfg.bass.midi_ch);
    else if (slot == 1u)
        copy_str(s, BASS_CLOCK_NAMES[g.cfg.bass.clock_mode % BASS_CLOCK_N]);
    else if (slot == 2u)
        i32_signed_dec(s, g.cfg.bass.transpose);
    else if (g.cfg.bass.vel_sens == 0u)
        copy_str(s, "FIXED");
    else
        u8_show100_dec(s, g.cfg.bass.vel_sens);
}

/* mp = &g.voice.p: heard values (cue + LFO applied by the voice rebuild); &g.cfg.p: stored base (cue shown). */
static uint8_t bass_groove_knob_bar(const dub_params_t *mp, uint8_t pg, uint8_t slot)
{
    if (pg == 0u) {
        int live = mp != &g.cfg.p;
        uint8_t sw = live ? mp->swing : dub_groove_show_swing();
        uint8_t bpm = live ? mp->bpm : dub_groove_show_bpm();
        uint8_t sc = live ? mp->scale : dub_groove_show_scale();

        if (slot == 0u)
            return (uint8_t)((mp->root % 12u) * 127u / 11u);
        if (slot == 1u)
            return (uint8_t)((scale_ix(sc) - SCALE_LO) * 127u / (SCALE_N - 1u));
        if (slot == 2u)
            return (uint8_t)(sw < 50u ? 0u : (sw - 50u) * 127u / 25u);
        return (uint8_t)(bpm < 40u ? 0u : (bpm - 40u) * 127u / 140u);
    }
    if (slot == 0u)
        return (uint8_t)(((g.cfg.bass.midi_ch - 1u) % 16u) * 127u / 15u);
    if (slot == 1u)
        return (uint8_t)((g.cfg.bass.clock_mode % BASS_CLOCK_N) * 127u / (BASS_CLOCK_N - 1u));
    if (slot == 2u)
        return (uint8_t)(((int32_t)bass_transpose_eff() + BASS_TRANSPOSE_MAX) * 127 / (2 * BASS_TRANSPOSE_MAX));
    return bass_vel_sens_eff();
}

static void bass_draw_view_groove(void)
{
    fm_groove_plot_t gp = {0};
    uint8_t pg = bank_page();
    uint8_t show_root = g.cue_on ? g.cue_root : g.cfg.p.root;
    uint8_t show_scale = scale_ix(dub_groove_show_scale());
    uint8_t sw = dub_groove_show_swing();
    uint8_t bpm = dub_groove_show_bpm();

    gp.page = pg;
    gp.bpm = bpm;
    gp.swing = sw;
    gp.root = show_root;
    gp.scale = show_scale;
    gp.playing = g.playing ? 1u : 0u;
    gp.step = (uint8_t)(g.step % 16u);
    gp.step_frac256 = g.playing ? dub_sample_acc_frac256(g.sample_acc, g.samples_per_step) : 0u;
    gp.clock_n = BASS_CLOCK_N;
    gp.col = ERA_COL[g.cfg.p.era % GENRE_ERA_N];
    if (pg == 0u) {
        gp.accent = 0u;
        gp.human = 0u;
        gp.clock_mode = g.cfg.bass.clock_mode;
        gp.midi_ch = g.cfg.bass.midi_ch;
    } else {
        gp.accent = bass_vel_sens_eff();
        gp.human = (uint8_t)(((int32_t)bass_transpose_eff() + BASS_TRANSPOSE_MAX) * 127 / (2 * BASS_TRANSPOSE_MAX));
        gp.clock_mode = g.cfg.bass.clock_mode;
        gp.midi_ch = g.cfg.bass.midi_ch;
    }
    fm_groove_plot_draw(&gp);
}

static int32_t bass_env_playhead_x(const bass_dahdsr_rt_t *rt, const bass_dahdsr_cfg_t *cfg, const int32_t *px)
{
    uint8_t st = rt->stage;
    int32_t seg;
    uint32_t total;
    uint32_t done;

    if (st == BASS_ENV_IDLE)
        return -1;
    seg = (int32_t)st;
    if (seg < 0 || seg > 5)
        return -1;
    if (st == BASS_ENV_SUSTAIN)
        return px[4] + (px[5] - px[4]) / 2;
    total = bass_dahdsr_stage_samples(st, cfg);
    if (total < 1u)
        total = 1u;
    done = rt->stage_left < total ? total - rt->stage_left : 0u;
    while (total > 0x7FFFFFu) { /* span · done stays in uint32: no __divdi3 on pi32v2 */
        total >>= 1;
        done >>= 1;
    }
    return px[seg] + (int32_t)(((uint32_t)(px[seg + 1] - px[seg]) * done) / total);
}

/* ENV page 4 (CURV): transfer plot of the selected mod slot, KIT frame. Dot = live env source level. */
static inline int32_t bass_curve_plot_x(int32_t s)
{
    return FM_ENV_KIT_X0 + (s * (FM_ENV_KIT_X1 - FM_ENV_KIT_X0)) / 128;
}

static inline int32_t bass_curve_plot_y(int32_t o)
{
    return FM_ENV_KIT_Y_BOT - (o * (FM_ENV_KIT_Y_BOT - FM_ENV_KIT_Y_TOP)) / 128;
}

static void bass_draw_curve_transfer(void)
{
    const bass_mod_slot_t *sl = &g.cfg.bass.mods.env[g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP];
    const uint16_t col = ERA_COL[g.voice.p.era % GENRE_ERA_N];
    const int32_t env_q15 = bass_dahdsr_mod_q15(&g.synth.bass_env, &g.synth.bass_env_cfg);
    const int32_t src = env_q15 < 0 ? -env_q15 : env_q15;
    int32_t s_live = src >> 8;
    int32_t px = -1;
    int32_t py = 0;
    int32_t s;

    if (s_live > 128)
        s_live = 128;
    cv_line(FM_ENV_KIT_X0, FM_ENV_KIT_Y_BOT, FM_ENV_KIT_X1, FM_ENV_KIT_Y_BOT, C_LINE);
    cv_line(FM_ENV_KIT_X0, FM_ENV_KIT_Y_TOP - 4, FM_ENV_KIT_X0, FM_ENV_KIT_Y_BOT, C_LINE);
    cv_line(FM_ENV_KIT_X1, FM_ENV_KIT_Y_TOP - 4, FM_ENV_KIT_X1, FM_ENV_KIT_Y_BOT, C_LINE);
    /* LIN reference, dim */
    cv_line(bass_curve_plot_x(0), bass_curve_plot_y(0), bass_curve_plot_x(128), bass_curve_plot_y(128),
            FM_ENV_KIT_SCALE(col, 36));
    for (s = 0; s <= 128; s += 2) {
        int32_t o = bass_mod_shape(s, sl->curve);
        int32_t x = bass_curve_plot_x(s);
        int32_t y = bass_curve_plot_y(o);

        cv_line(x, y, x, FM_ENV_KIT_Y_BOT, FM_ENV_KIT_SCALE(col, 44));
        if (px >= 0)
            cv_line(px, py, x, y, FM_ENV_KIT_SCALE(col, 220));
        px = x;
        py = y;
    }
    {
        const int32_t ph_x = bass_curve_plot_x(s_live);
        const int32_t live_y = bass_curve_plot_y(bass_mod_shape(s_live, sl->curve));

        cv_line(ph_x, FM_ENV_KIT_Y_TOP - 4, ph_x, FM_ENV_KIT_Y_BOT, FM_ENV_KIT_SCALE(C_ACC, 120));
        cv_rect(ph_x - 1, live_y - 1, 3, 3, C_WHITE);
    }
}

static void draw_view_env(void)
{
    if (g.bank == BANK_CHORD && bank_page() >= FM_ENV_KIT_PAGES) {
        bass_draw_curve_transfer();
        return;
    }
    /* Effective cfg (mod matrix + LFO): the shape and playhead move with modulation. */
    const bass_dahdsr_cfg_t *cfg = &g.synth.bass_env_cfg;
    const bass_dahdsr_rt_t *rt = &g.synth.bass_env;
    fm_env_kit_plot_t ep;
    fm_env_kit_geom_t gm;

    ep.w[0] = (uint32_t)cfg->t.delay;
    ep.w[1] = cfg->t.attack ? (uint32_t)cfg->t.attack : 1u;
    ep.w[2] = (uint32_t)cfg->t.hold;
    ep.w[3] = cfg->t.decay ? (uint32_t)cfg->t.decay : 1u;
    ep.w[4] = cfg->t.release ? (uint32_t)cfg->t.release : 1u;
    ep.sustain = cfg->t.sustain;
    ep.curve = g.synth.bass_env_curve;
    ep.invert = cfg->pol.invert;
    ep.col = ERA_COL[g.voice.p.era % GENRE_ERA_N];
    fm_env_kit_draw(&ep, &gm);
    fm_env_kit_draw_playhead(&ep, &gm, bass_env_playhead_x(rt, cfg, gm.px));
    fm_env_kit_draw_gate(rt->gate_open, cfg->pol.retrigger);
}

#include "ko_bar_stack.h"
#define BASS_MARK_SLIDE_MASK KO_BAR_MARK_SLIDE_MASK
#define BASS_MARK_RAT_SHIFT KO_BAR_MARK_RAT_SHIFT

/* 8 rows × 16 steps = one bass line in time (B1…B8). Not instrument lanes. */
static void bass_draw_bar_stack(uint8_t sel_bar, int playing, uint8_t plock_st, int plock_latched,
                                uint16_t play_col, const char *const *bar_lbls,
                                const uint16_t *bar_lbl_cols, const fm_seq_cell_t *cells,
                                const uint8_t *marks, const uint8_t *play_step_by_bar)
{
    ko_bar_stack_t bs;

    bs.cells = cells;
    bs.marks = marks;
    bs.bar_lbls = bar_lbls;
    bs.bar_lbl_cols = bar_lbl_cols;
    bs.play_step_by_bar = play_step_by_bar;
    bs.play_col = play_col;
    bs.acc = C_ACC;
    bs.sel_bar = sel_bar;
    bs.cursor_st = plock_st;
    bs.cursor_latched = plock_latched ? 1u : 0u;
    bs.playing = playing ? 1u : 0u;
    bs.slide_full = BASS_STEP_LV_FULL;
    ko_bar_stack_draw(&bs);
}


#define BASS_RH_SCALE color_scale_rgb
#define BASS_RH_SINE dub_sine_q64
#include "bass_reactive_home.h"

/* One bar of the bass lane as fire_seq_step will play it (SHIFT, DENS, GATE, RATCH applied). */
static void bass_home_lane_fill(bass_reactive_home_t *rh)
{
    uint8_t len = seq_lane_len_v(0u, 4u);
    uint8_t lh = g.cfg.bass.live_hold;
    uint32_t bar = (uint32_t)(g.step % DUB_SEQ_STEPS) / BASS_RH_STEPS;
    uint32_t c;

    for (c = 0u; c < BASS_RH_STEPS; c++) {
        uint8_t st = (uint8_t)(bar * BASS_RH_STEPS + c);
        uint8_t mst;
        uint8_t f = 0u;
        const bass_step_x_t *sx;
        uint8_t gate;
        int32_t nn;

        rh->flags[c] = 0u;
        rh->note[c] = 0u;
        rh->gate[c] = 0u;
        rh->rat[c] = 1u;
        if (st >= len) {
            rh->flags[c] = BASS_RH_OUT;
            continue;
        }
        mst = bass_perf_shift_step(st, len);
        if (bass_seq_lane_step_on(mst))
            f = bass_perf_dens_drop(mst) ? BASS_RH_DROP : BASS_RH_ON;
        else if (mst < len && bass_perf_dens_fill(mst))
            f = BASS_RH_FILL;
        if (!f)
            continue;
        sx = &g.cfg.bass.step_x[mst];
        gate = bass_perf_gate(bass_step_effective_gate(sx, lh, g.cfg.bass.live_gate));
        if (!(f & BASS_RH_FILL)) {
            if (bass_step_effective_accent(sx, lh, g.cfg.bass.live_accent))
                f |= BASS_RH_ACCENT;
            if (bass_step_effective_slide(sx, lh, g.cfg.bass.live_slide))
                f |= BASS_RH_SLIDE;
        }
        if (bass_step_gate_is_tie(gate))
            f |= BASS_RH_TIE;
        nn = g.cfg.seq[mst].note ? bass_note_abs(g.cfg.seq[mst].note, g.voice.p.root)
                                 : (int32_t)scaled_note(g.voice.p.root, g.voice.p.scale, mst);
        nn += (int32_t)g.step_tune[4u][mst] + (int32_t)bass_transpose_eff();
        rh->note[c] = (uint8_t)fm_lfo_clamp_u8(nn, 0, 127);
        rh->gate[c] = gate;
        rh->rat[c] = bass_perf_ratchet(mst, bass_step_effective_ratchet(sx, lh, g.cfg.bass.live_ratchet));
        if (!bass_step_ratchet_allowed((f & BASS_RH_SLIDE) ? 1u : 0u, gate))
            rh->rat[c] = 1u;
        rh->flags[c] = f;
    }
}

/* KO BASS HOME: filter horizon, drive knot + env orb, one bar of the line as heard — overlaid. */
static void bass_draw_reactive_home(uint8_t pg)
{
    const dub_params_t *p = &g.voice.p;
    const bass_dahdsr_rt_t *env = &g.synth.bass_env;
    bass_reactive_home_t rh;

    (void)pg;
    rh.ecol = ERA_COL[p->era % GENRE_ERA_N];
    rh.acc_col = GENRE_C_ACC;
    rh.cut = (int32_t)p->cutoff_b;
    rh.reso = (int32_t)p->reso_mode;
    rh.drive = (int32_t)p->drive;
    rh.env_filt = (int32_t)p->env_filt;
    rh.env = env->level_q15 >> 8;
    if (rh.env > 127)
        rh.env = 127;
    rh.env_sweep = (p->bass_model % BASS_N) == BASS_ACID303 ? 1u : 0u;
    rh.gate_open = env->gate_open ? 1u : 0u;
    rh.playing = g.playing ? 1u : 0u;
    rh.cur = g.playing ? (uint8_t)(g.step % BASS_RH_STEPS) : 255u;
    rh.t = fm1_ms;
    bass_home_lane_fill(&rh);
    bass_reactive_home_draw(&rh);
}

static void draw_view_home(void)
{
    uint8_t pg = bank_page();

    if (fm1_ms < 1600u) {
        cv_rect(0, 0, 240, 112, RGB(4, 12, 32));
        cv_text(60, 44, &FONT_S, "KO BASS", GENRE_C_ACC);
        cv_text(72, 60, &FONT_S, "FM-1", C_DIM);
        return;
    }
    bass_draw_reactive_home(pg);
}

/* KO BASS VOICE (SEL): the voice as a Lissajous figure — ratio from the model, phase from TIMBRE,
 * wavefold from DRIVE, time trails, glow + sparks + dust from the amp env. Text-free. */
#define BASS_VC_N 96u
#define BASS_VC_TRAILS 3

static int32_t bass_vc_fold(int32_t v)
{
    int32_t k;

    for (k = 0; k < 4 && (v > 32767 || v < -32767); k++) {
        if (v > 32767)
            v = 65534 - v;
        else
            v = -65534 - v;
    }
    if (v > 32767)
        v = 32767;
    if (v < -32767)
        v = -32767;
    return v;
}

static void bass_vc_points(const dub_params_t *p, uint32_t t, int32_t gain, int32_t rx, int32_t ry,
                           int32_t *ox, int32_t *oy)
{
    uint32_t a = 1u + (uint32_t)((p->bass_model % BASS_N) % 5u);
    uint32_t b = a + 1u + (uint32_t)p->reso_mode / 48u;
    uint32_t tx = t * 0x00200000u;
    uint32_t ty = t * 0x00140000u + ((uint32_t)p->timbre << 25);
    uint32_t i;

    for (i = 0u; i < BASS_VC_N; i++) {
        uint32_t n = i * 44739242u; /* 2^32 / BASS_VC_N, so the loop closes */
        int32_t xv = BASS_RH_SINE(n * a + tx);
        int32_t yv = BASS_RH_SINE(n * b + ty);

        xv = bass_vc_fold((xv * gain) >> 8);
        ox[i] = 120 + (xv * rx) / 32768;
        oy[i] = 60 + (yv * ry) / 32768;
    }
}

static inline uint16_t bass_vc_mix(uint16_t a, uint16_t b, uint32_t t)
{
    uint32_t ar = (uint32_t)a >> 11, ag = ((uint32_t)a >> 5) & 63u, ab = (uint32_t)a & 31u;
    uint32_t br = (uint32_t)b >> 11, bg = ((uint32_t)b >> 5) & 63u, bb = (uint32_t)b & 31u;

    return (uint16_t)((((ar * (16u - t) + br * t) / 16u) << 11) |
                      (((ag * (16u - t) + bg * t) / 16u) << 5) | ((ab * (16u - t) + bb * t) / 16u));
}

static void bass_draw_view_voice(void)
{
    const dub_params_t *p = &g.voice.p;
    const bass_dahdsr_rt_t *env = &g.synth.bass_env;
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    uint32_t t = fm1_ms;
    int32_t lv = env->level_q15 >> 8;
    int32_t gain = 256 + (int32_t)p->drive * 6;
    int32_t rx = 60 + lv / 3;
    int32_t ry = 36 + lv / 4;
    int32_t px[BASS_VC_N];
    int32_t py[BASS_VC_N];
    int32_t tr;
    int32_t i;
    int32_t k;
    enum { BASS_VC_PAN_CTR = 120 };
    int32_t pan = (int32_t)(p->pan[0] == 0u ? 64u : p->pan[0]);
    int32_t pan_l = 16 + ((127 - pan) * 180) / 127;
    int32_t pan_r = 16 + (pan * 180) / 127;
    int32_t cut = (int32_t)p->cutoff_b;
    int32_t bloom_r;
    int32_t wave_py;
    int32_t wave_x;

    cv_rect(0, 0, 240, 126, RGB(2, 4, 14));
    cv_rect(0, 0, (uint32_t)BASS_VC_PAN_CTR, 126,
            bass_vc_mix(RGB(2, 2, 8), BASS_RH_SCALE(ecol, pan_l), 10u));
    cv_rect(BASS_VC_PAN_CTR, 0, (uint32_t)(240 - BASS_VC_PAN_CTR), 126,
            bass_vc_mix(RGB(1, 1, 4), BASS_RH_SCALE(ecol, pan_r), 10u));

    bloom_r = 18 + (cut * 44) / 127;
    if (bloom_r > 58)
        bloom_r = 58;
    bass_rh_glow(120, 63, bloom_r, BASS_RH_SCALE(ecol, 12 + cut / 5 + (lv >> 3)));
    if (cut > 24)
        bass_rh_glow(120, 63, bloom_r + 14 + cut / 8, BASS_RH_SCALE(ecol, 6 + cut / 8));

    bass_rh_ring(120, 63, 20, BASS_RH_SCALE(ecol, 24));
    bass_rh_ring(120, 63, 40, BASS_RH_SCALE(ecol, 18));
    bass_rh_ring(120, 63, 60, BASS_RH_SCALE(ecol, 12));
    bass_rh_line(0, 63, 239, 63, BASS_RH_SCALE(ecol, 26));

    /* dust: specks appear with the env level */
    for (k = 0; k < 24; k++) {
        uint32_t h = (uint32_t)k * 2654435761u + (t >> 6) * 40503u;

        h ^= h >> 15;
        h *= 0x2c1b3c6du;
        h ^= h >> 12;
        if ((int32_t)(h & 127u) < lv)
            bass_rh_px((int32_t)((h >> 8) % 240u), (int32_t)((h >> 20) % 126u), BASS_RH_SCALE(ecol, 40 + lv));
    }

    /* the figure, back trails first, live curve last */
    for (tr = BASS_VC_TRAILS - 1; tr >= 0; tr--) {
        uint32_t tt = t - (uint32_t)tr * 70u;
        uint16_t c = tr == 0 ? BASS_RH_SCALE(ecol, 120 + (int32_t)p->cutoff_b)
                             : BASS_RH_SCALE(GENRE_C_ACC, 70 - tr * 20);

        bass_vc_points(p, tt, gain, rx, ry, px, py);
        for (i = 0; i < (int32_t)BASS_VC_N; i++) {
            int32_t j = (i + 1) % (int32_t)BASS_VC_N;

            bass_rh_line(px[i], py[i], px[j], py[j], c);
        }
    }
    bass_rh_glow(px[0], py[0], 2 + lv / 24, BASS_RH_SCALE(ecol, 300));
    if (env->gate_open)
        bass_rh_ring(px[0], py[0], 4 + (int32_t)((t / 16u) % 12u), BASS_RH_SCALE(GENRE_C_ACC, 140));

    /* thin morph hint along the bottom — figure stays dominant */
    wave_py = -1;
    for (wave_x = 0; wave_x < 240; wave_x += 4) {
        uint32_t ph = (uint32_t)wave_x * 0x180000u + (t << 12) + ((uint32_t)p->timbre << 20);
        int32_t wy = 120 - (BASS_RH_SINE(ph) * (8 + cut / 16)) / 32768;

        if (wave_py >= 0)
            bass_rh_line(wave_x - 4, wave_py, wave_x, wy, BASS_RH_SCALE(ecol, 22 + cut / 6));
        wave_py = wy;
    }
}

/* VOICE center uses draw_view_panel("VOICE") in fm_app.c — knobs only, no bar widgets. */

static inline int bass_seq_grid_step_on(uint8_t st)
{
    return bass_seq_lane_base_on(st);
}

static inline uint8_t bass_seq_bar_count(void)
{
    uint8_t bars = fm_seq_pages_clamp(g.cfg.seq_pages);

    if (bars > 8u)
        bars = 8u;
    if (bars < 1u)
        bars = 1u;
    return bars;
}

static inline uint8_t bass_seq_edit_bar(void)
{
    return bass_seq_edit_bar_runtime(g.seq_view_page, g.cfg.seq_pages);
}

static inline uint8_t bass_seq_cursor_step(void)
{
    return bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages, g.step);
}

static inline uint8_t bass_seq_edit_base(void)
{
    return (uint8_t)(bass_seq_edit_bar() * FM_SEQ_PAGE_STEPS);
}

static inline uint8_t bass_seq_plock_step(void)
{
    return bass_seq_cursor_step();
}

static void bass_knob_label_seq(uint8_t slot, char *s)
{
    static const char *const P0[4] = {"GATE", "ACCENT", "SLIDE", "RATCHET"};
    static const char *const P2[4] = {"LENGTH", "STATE", "", "VARY"};

    if (g.seq_plock_hold)
        copy_str(s, g.seq_plock_bank ? P0[slot % 4u] : FM_SEQ_KNOB_PLOCK[slot % 4u]);
    else if (g.seq_page == 1u)
        copy_str(s, FM_SEQ_KNOB_P1[slot % 4u]);
    else if (g.seq_page == 2u)
        copy_str(s, P2[slot % 4u]);
    else
        copy_str(s, P0[slot % 4u]);
}

static void bass_knob_value_plock(uint8_t slot, char *s)
{
    uint8_t st = bass_seq_plock_step();
    uint8_t vel = g.step_vel[4u][st];

    if (slot == 0u)
        u8_show100_dec(s, vel ? vel : bass_step_lv_vel(g.cfg.bass.step_x[st].accent));
    else if (slot == 1u)
        copy_str(s, FM_SEQ_COND_NAMES[g.step_prob[4u][st] % FM_SEQ_NCONDS]);
    else if (slot == 2u)
        fm_seq_fmt_micro_readout((int32_t)g.step_micro[4u][st], s);
    else
        i32_signed_dec(s, g.step_tune[4u][st]);
}

static void bass_knob_value_seq(uint8_t slot, char *s)
{
    uint8_t st = bass_seq_plock_step();
    const bass_step_x_t *sx = &g.cfg.bass.step_x[st];
    uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);
    const uint8_t ln = 4u;

    if (g.seq_plock_hold && !g.seq_plock_bank) {
        bass_knob_value_plock(slot, s);
        return;
    }
    if (!g.seq_plock_hold && g.seq_page == 1u) {
        uint8_t fn = fm_seq_p1_fn(slot);

        if (fn == 1u)
            fm_seq_fmt_trig(dub_eff_trig_raw(ln), s);
        else
            fm_seq_fmt_rand_spread(fn == 0u ? dub_eff_vel_rng(ln) :
                                   fn == 2u ? dub_eff_gate_rng(ln) : dub_eff_pitch_rng(ln), s);
        return;
    }
    if (!g.seq_plock_hold && g.seq_page == 2u) {
        if (slot == 0u)
            u8_dec(s, seq_lane_len_v(vp, ln));
        else if (slot == 1u)
            copy_str(s, FM_SEQ_P3_STATE_NAMES[dub_lane_p3_state_pos(ln)]);
        else if (slot == 2u)
            s[0] = '\0';
        else
            fm_seq_fmt_vary(dub_eff_vary(ln), s);
        return;
    }
    {
        uint8_t lh = g.seq_plock_hold ? 0u : g.cfg.bass.live_hold;
        uint8_t gate = bass_step_effective_gate(sx, lh, g.cfg.bass.live_gate);
        uint8_t slide = bass_step_effective_slide(sx, lh, g.cfg.bass.live_slide);
        uint8_t acc = bass_step_effective_accent(sx, lh, g.cfg.bass.live_accent);
        uint8_t rat_stored = bass_step_clamp_ratchet(sx->ratchet);

        if (slot == 0u)
            u8_dec(s, gate);
        else if (slot == 1u)
            copy_str(s, bass_step_lv_name(acc));
        else if (slot == 2u)
            copy_str(s, bass_step_lv_name(slide));
        else
            u8_dec(s, rat_stored);
    }
}

static uint8_t bass_knob_seq_bar_px(uint8_t slot)
{
    uint8_t st = bass_seq_plock_step();
    const bass_step_x_t *sx = &g.cfg.bass.step_x[st];
    uint32_t v = 0u;
    uint32_t mx = 127u;
    uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);
    const uint8_t ln = 4u;

    if (g.seq_plock_hold && !g.seq_plock_bank) {
        if (slot == 0u)
            v = g.step_vel[4u][st] ? g.step_vel[4u][st] : bass_step_lv_vel(sx->accent);
        else if (slot == 1u)
            return fm_seq_cond_bar_px(g.step_prob[4u][st]);
        else if (slot == 2u)
            return fm_seq_bip_pot((int32_t)g.step_micro[4u][st], FM_SEQ_MICRO_HALF);
        else {
            v = (uint32_t)((int32_t)g.step_tune[4u][st] + 24);
            mx = 48u;
        }
    } else if (!g.seq_plock_hold && g.seq_page == 1u) {
        uint8_t fn = fm_seq_p1_fn(slot);

        if (fn == 1u) {
            v = fm_seq_trig_pct(dub_eff_trig_raw(ln));
            mx = 100u;
        } else if (fn == 0u) {
            return fm_seq_bip_pot(dub_eff_vel_rng(ln), 50);
        } else if (fn == 2u) {
            return fm_seq_bip_pot(dub_eff_gate_rng(ln), 50);
        } else {
            return fm_seq_bip_pot(dub_eff_pitch_rng(ln), 24);
        }
    } else if (!g.seq_plock_hold && g.seq_page == 2u) {
        if (slot == 0u) {
            v = (uint32_t)(seq_lane_len_v(vp, ln) - 1u);
            mx = (uint32_t)(FM_SEQ_STEPS - 1u);
        } else if (slot == 1u) {
            uint8_t pos = dub_lane_p3_state_pos(ln);

            v = pos == 0u ? 0u : pos == 1u ? 64u : 127u;
            mx = 127u;
        } else if (slot == 2u) {
            return 0u;
        } else {
            v = dub_eff_vary(ln);
            mx = 127u;
        }
    } else {
        uint8_t lh = g.seq_plock_hold ? 0u : g.cfg.bass.live_hold;
        uint8_t gate = bass_step_effective_gate(sx, lh, g.cfg.bass.live_gate);
        uint8_t slide = bass_step_effective_slide(sx, lh, g.cfg.bass.live_slide);
        uint8_t acc = bass_step_effective_accent(sx, lh, g.cfg.bass.live_accent);
        uint8_t rat_stored = bass_step_clamp_ratchet(sx->ratchet);

        if (slot == 0u) {
            v = gate;
            mx = BASS_GATE_TIE;
        } else if (slot == 1u) {
            v = acc;
            mx = BASS_STEP_LV_FULL;
        } else if (slot == 2u) {
            v = slide;
            mx = BASS_STEP_LV_FULL;
        } else {
            v = (uint32_t)(rat_stored - BASS_RATCHET_MIN);
            mx = (uint32_t)(BASS_RATCHET_MAX - BASS_RATCHET_MIN);
        }
    }
    if (v > mx)
        v = mx;
    return mx ? (uint8_t)((v * 127u) / mx) : 0u;
}

static void bass_draw_view_seq(void)
{
    static char bar_lbl[8][4];
    static const char *bar_ptr[8];
    fm_seq_cell_t cells[8 * FM_SEQ_PAGE_STEPS];
    uint8_t marks[8 * FM_SEQ_PAGE_STEPS];
    uint8_t play_bar_st[8];
    uint16_t lcols[8];
    uint8_t nbars = bass_seq_bar_count();
    uint8_t sel_bar = bass_seq_edit_bar();
    uint32_t slen = fm_seq_len(g.cfg.seq_pages);
    uint32_t row, col, st;
    uint8_t plock_st = FM_SEQ_PLOCK_NONE;
    uint16_t ecol = ERA_COL[g.cfg.p.era % GENRE_ERA_N];
    uint16_t empty_act = RGB(18, 22, 42);
    int play_row = g.playing ? (int)(g.step / FM_SEQ_PAGE_STEPS) : -1;
    int play_col = g.playing ? (int)(g.step % FM_SEQ_PAGE_STEPS) : -1;
    uint8_t cursor_st = bass_seq_cursor_step();
    int cursor_bar = (int)(cursor_st / FM_SEQ_PAGE_STEPS);

    if (cursor_bar == (int)sel_bar)
        plock_st = (uint8_t)(cursor_st % FM_SEQ_PAGE_STEPS);

    for (row = 0u; row < 8u; row++) {
        bar_lbl[row][0] = (char)('1' + row);
        bar_lbl[row][1] = 0;
        bar_ptr[row] = bar_lbl[row];
        play_bar_st[row] = 0xFFu;
        lcols[row] = (int32_t)row < (int32_t)nbars ? C_WHITE : C_DIM;
        if (g.playing && (int32_t)row == play_row)
            play_bar_st[row] = (uint8_t)play_col;
        for (col = 0u; col < FM_SEQ_PAGE_STEPS; col++) {
            fm_seq_cell_t *cell = &cells[row * FM_SEQ_PAGE_STEPS + col];
            uint8_t accent;
            int on;
            int base_on;
            uint8_t vel;
            uint8_t lh;
            const bass_step_x_t *sx;

            cell->bg_col = empty_act;
            cell->col = 0;
            cell->vel = 0;
            cell->tune = 0;
            cell->micro = 0;
            cell->cond = 0;
            cell->gate = 0;
            marks[row * FM_SEQ_PAGE_STEPS + col] = 0u;
            if (row >= (uint32_t)nbars) {
                cell->bg_col = C_LINE;
                continue;
            }
            st = row * FM_SEQ_PAGE_STEPS + col;
            if (st >= slen)
                continue;
            sx = &g.cfg.bass.step_x[st];
            base_on = bass_seq_grid_step_on((uint8_t)st);
            /* VARY as playback (fm_app bass lane 4 hit): same gate, length cut as seq_lane_len_v(0, 4). */
            on = (st < seq_lane_len_v(0u, 4u)) ? bass_seq_vary_on(base_on, (uint8_t)st) : base_on;
            lh = g.seq_plock_hold ? 0u : g.cfg.bass.live_hold;
            accent = bass_step_effective_accent(sx, lh, g.cfg.bass.live_accent);
            if (on) {
                uint8_t g0 = bass_step_effective_gate(sx, lh, g.cfg.bass.live_gate);
                int32_t gr;

                if (!base_on && g0 == 0u && !(lh & BASS_LIVE_HOLD_GATE))
                    g0 = 4u; /* VARY-added: bass_fire_note plays a full step */
                gr = (int32_t)g0 + (int32_t)lane_step_gate_rnd(4u, (uint8_t)st);
                int32_t tune = (int32_t)lane_step_tune(4u, (uint8_t)st);
                uint8_t slide = bass_step_effective_slide(sx, lh, g.cfg.bass.live_slide);
                uint8_t rat = bass_step_effective_ratchet(sx, lh, g.cfg.bass.live_ratchet);

                vel = g.step_vel[4u][st] ? g.step_vel[4u][st] : (uint8_t)(90u + (20u * accent) / BASS_STEP_LV_FULL);
                if (sx->lock)
                    cell->col = C_ACC;
                else if (accent)
                    cell->col = color_sat_wave(ecol, (int)((72u * accent) / BASS_STEP_LV_FULL));
                else
                    cell->col = ecol;
                cell->vel = vel;
                if (gr < 0)
                    gr = 0;
                if (gr > (int32_t)BASS_GATE_TIE)
                    gr = (int32_t)BASS_GATE_TIE;
                /* 4 = full step → 0; 0 → -63 (narrowest), TIE → +63 (widest). */
                cell->gate = (int8_t)(((gr - 4) * 63) / 4);
                if (tune < -24)
                    tune = -24;
                if (tune > 24)
                    tune = 24;
                cell->tune = (int8_t)tune;
                cell->micro = g.step_micro[4u][st];
                cell->cond = g.step_prob[4u][st];
                marks[row * FM_SEQ_PAGE_STEPS + col] =
                    (uint8_t)(slide | ((rat > 1u ? rat : 0u) << BASS_MARK_RAT_SHIFT));
                if (!base_on)
                    cell->col = C_ACC; /* VARY-added hit */
            } else if (base_on) {
                /* VARY-removed hit: dim, as GENRE grid. */
                cell->vel = (uint8_t)(90u + (20u * accent) / BASS_STEP_LV_FULL);
                cell->col = C_DIM;
            }
            if (g.seq_plock_hold && g.seq_plock_step == st && plock_st == col)
                cell->col = C_ACC;
        }
    }

    bass_draw_bar_stack(sel_bar, g.playing, plock_st, g.seq_plock_latch ? 1 : 0, ecol, bar_ptr, lcols, cells,
                        marks, play_bar_st);
    cv_text(168, 100, &FONT_S, "BK BAR", C_DIM);
}

#endif
