/* SPDX-License-Identifier: GPL-3.0-only */
/* Genre apps (not KO BASS): SEL = one voice bank (KICK SNARE HAT BASS CHORD LEAD),
 * ENV = KO KIT DAHDSR per voice (fm_env_kit.h), GLO = groove globals (fm_groove_plot.h).
 * SEL/ENV reuse the legacy CHORD/BASS/DRUM knob code: (page, slot) is remapped onto the legacy layout and
 * g.bank / g.page / g.sel_drm are swapped for the duration of the call (gv_raw guards re-entry).
 * Include from fm_app.c after knob_slot_fill and fm_env_kit.h. */
#ifndef GENRE_VOX_H
#define GENRE_VOX_H

#if !defined(GENRE_BASS)

#include "fm_groove_plot.h"

#define GV_N    6u
#define GV_SCR  4u
#define GV_VOX  0xFEu /* SEL screen: DRUMS / BASS / CHORD / LEAD */
#define GV_DRV  0xFDu /* DRUMS screen: KICK / SNARE / HAT */
#define GV_LDRV 0xFCu /* LEAD screen: DRIVE (drm_drv[3]) */
#define GV_LPMP 0xFBu /* LEAD screen: PUMP (pan[3]); lowest GV_* code */
#define GV_NONE 0xFFu

/* ENV voice order → seq lane */
static const uint8_t GV_LANE[GV_N] = {0u, 1u, 2u, 4u, 5u, 3u};
static uint8_t gv_scr;  /* 0 DRUMS, 1 BASS, 2 CHORD, 3 LEAD */
static uint8_t gv_drum; /* 0..2 last drum in the DRUMS screen */
static uint8_t gv_env;
static uint8_t gv_raw;

#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_SOUL)
#define GV_WOB_LBL "CHAR"
#else
#define GV_WOB_LBL "WOBBLE"
#endif

/* [screen][page][slot] → 0xPS legacy page/slot of the screen's legacy bank, or a GV_* code. */
static const uint8_t GV_MAP[GV_SCR][3][4] = {
    {{GV_VOX, 0x00u, GV_DRV, 0x03u}, {0x11u, 0x20u, 0x10u, GV_NONE}, {0x02u, 0x21u, 0x22u, 0x23u}},
    {{GV_VOX, 0x00u, 0x01u, 0x13u}, {0x10u, 0x11u, 0x12u, GV_NONE}, {0x20u, 0x21u, 0x22u, 0x23u}},
    {{GV_VOX, 0x00u, 0x01u, 0x10u}, {0x11u, 0x12u, 0x13u, GV_NONE}, {0x20u, 0x21u, 0x22u, 0x23u}},
    {{GV_VOX, 0x03u, 0x11u, 0x10u}, {0x20u, GV_LDRV, GV_LPMP, GV_NONE}, {0x02u, 0x21u, 0x22u, 0x23u}},
};

static uint8_t gv_scr_bank(uint8_t scr)
{
    return scr == 1u ? (uint8_t)BANK_BASS : scr == 2u ? (uint8_t)BANK_CHORD : (uint8_t)BANK_DRUM;
}

static const char *gv_vox_name(uint8_t v)
{
    return seq_lane_name(GV_LANE[v % GV_N]);
}

static const char *gv_scr_name(uint8_t scr)
{
    static const uint8_t L[GV_SCR] = {0u, 4u, 5u, 3u};

    return scr == 0u ? "DRUMS" : seq_lane_name(L[scr % GV_SCR]);
}

static int gv_is_sel_bank(uint8_t b)
{
    return b == BANK_CHORD || b == BANK_BASS || b == BANK_DRUM;
}

static int gv_active(void)
{
    return !gv_raw && g.view != VIEW_SEQ && !g.seq_plock_hold;
}

/* Screen of the open voice bank (drum bank: LEAD when the siren lane is selected). */
static uint8_t gv_cur_scr(void)
{
    if (g.bank == BANK_BASS)
        return 1u;
    if (g.bank == BANK_CHORD)
        return 2u;
    return g.sel_drm == 3u ? 3u : 0u;
}

/* Drum ALL is gone: sel_drm is always one lane (0..3). */
static void gv_apply_scr(uint8_t scr)
{
    if (gv_drum > 2u)
        gv_drum = 0u;
    gv_scr = (uint8_t)(scr % GV_SCR);
    if (gv_scr == 3u)
        g.sel_drm = 3u;
    else if (gv_scr == 0u || g.sel_drm > 3u)
        g.sel_drm = gv_drum;
    gv_env = gv_scr == 0u ? gv_drum : gv_scr == 1u ? 3u : gv_scr == 2u ? 4u : 5u;
}

static uint8_t gv_sel_bank(void)
{
    gv_apply_scr(gv_scr);
    return gv_scr_bank(gv_scr);
}

static void gv_select_scr(uint8_t scr, int follow_bank)
{
    uint8_t pg = g.page[g.bank % BANK_COUNT];

    gv_apply_scr(scr);
    if (follow_bank && gv_is_sel_bank(g.bank) && g.view != VIEW_SEQ) {
        uint8_t nb = gv_scr_bank(gv_scr);

        if (nb != g.bank) {
            g.page[nb] = (uint8_t)(pg % 3u);
            nav_bank(nb);
        }
    }
    g.ui_dirty = 1u;
}

/* Black key / ENV VOICE / LFO lane pick: SEL screen, drum voice and ENV voice follow. */
static void gv_from_lane(uint8_t ln)
{
    if (ln >= GV_N)
        return;
    if (ln < 3u)
        gv_drum = ln;
    gv_select_scr(ln < 3u ? 0u : ln == 3u ? 3u : ln == 4u ? 1u : 2u, 1);
}

/* ENV own knob code (0xPS) -> ENV LFO param index (DUB_LFO_ENVX + ix * 6 + lane), 255 = none. */
static uint8_t gv_env_lfo_ix(uint8_t m)
{
    switch (m) {
    case 0x02u: return 0u;
    case 0x03u: return 1u;
    case 0x10u: return 2u;
    case 0x11u: return 3u;
    case 0x12u: return 4u;
    case 0x21u: return 5u;
    case 0x22u: return 6u;
    default: return 255u;
    }
}

static uint8_t gv_env_lfo_tgt(uint8_t m)
{
    uint8_t ix = gv_env_lfo_ix(m);

    if (ix == 255u)
        return 255u;
    return (uint8_t)(DUB_LFO_ENVX + ix * DUB_ENVX_LANES + GV_LANE[gv_env % GV_N]);
}

static int8_t gv_envx_mod_off(int8_t v, uint8_t tgt)
{
    return dub_envx_off(lfo_mod_u8(dub_envx_u8(v), tgt, 0u, 127u));
}

/* cfg.envx + ENV LFOs -> synth.envx; every sync_voice_params() and after edits. */
static void gv_envx_sync(void)
{
    uint8_t i;

    for (i = 0u; i < DUB_ENVX_LANES; i++) {
        dub_envx_v_t e = g.cfg.envx[i];
        uint8_t t = (uint8_t)(DUB_LFO_ENVX + i);

        e.sus = lfo_mod_u8(e.sus, t, 0u, 127u);
        e.rel = lfo_mod_u8(e.rel, (uint8_t)(t + DUB_ENVX_LANES), 0u, 127u);
        e.dly = lfo_mod_u8(e.dly, (uint8_t)(t + 2u * DUB_ENVX_LANES), 0u, 127u);
        e.hold = lfo_mod_u8(e.hold, (uint8_t)(t + 3u * DUB_ENVX_LANES), 0u, 127u);
        e.curve = gv_envx_mod_off(e.curve, (uint8_t)(t + 4u * DUB_ENVX_LANES));
        e.pdec = gv_envx_mod_off(e.pdec, (uint8_t)(t + 5u * DUB_ENVX_LANES));
        e.sweep = gv_envx_mod_off(e.sweep, (uint8_t)(t + 6u * DUB_ENVX_LANES));
        g.synth.envx[i] = e;
    }
}

/* LFO target names for DUB_LFO_ENVX..: short = SEQ lane letter + param ("K SUS"), long = lane name + param. */
static void gv_lfo_names_init(void)
{
    static const char *const P[DUB_LFO_ENVX_PARAMS] = {"SUS", "REL", "DLY", "HLD", "CRV", "PDC", "SWP"};
    static const char *const PL[DUB_LFO_ENVX_PARAMS] = {"SUSTAIN", "RELEASE", "DELAY", "HOLD",
                                                        "CURVE", "P.DECAY", "SWEEP"};
    uint8_t pi, ln;

    for (pi = 0u; pi < DUB_LFO_ENVX_PARAMS; pi++) {
        for (ln = 0u; ln < DUB_ENVX_LANES; ln++) {
            uint32_t ix = pi * DUB_ENVX_LANES + ln;
            const char *nm = seq_lane_name(ln);
            char *s = gv_lfo_nm[ix];
            char *l = gv_lfo_nm_long[ix];
            uint32_t n = 0u, k = 0u;

            s[n++] = DUB_SEQ_LFO_LBL[ln];
            s[n++] = ' ';
            while (P[pi][k] && n < sizeof gv_lfo_nm[0] - 1u)
                s[n++] = P[pi][k++];
            s[n] = '\0';
            n = 0u;
            while (nm[n] && n < 10u) {
                l[n] = nm[n];
                n++;
            }
            l[n++] = ' ';
            k = 0u;
            while (PL[pi][k] && n < sizeof gv_lfo_nm_long[0] - 1u)
                l[n++] = PL[pi][k++];
            l[n] = '\0';
        }
    }
}

/* ENV LFO slots: up to DUB_ENVX_LFO_SAVE_N active ones persist in cfg.envx_lfo. */
static void gv_envx_lfo_pack(void)
{
    uint32_t t, n = 0u;

    memset(g.cfg.envx_lfo, 0, sizeof g.cfg.envx_lfo);
    for (t = 0u; t < DUB_ENVX_LFO_SAVE_N; t++)
        g.cfg.envx_lfo[t].id = 0xFFu;
    for (t = DUB_LFO_ENVX; t < DUB_LFO_ENVX_END && n < DUB_ENVX_LFO_SAVE_N; t++) {
        if (!g.lfo_slots[t].depth)
            continue;
        g.cfg.envx_lfo[n].id = (uint8_t)(t - DUB_LFO_ENVX);
        g.cfg.envx_lfo[n].s = g.lfo_slots[t];
        n++;
    }
}

static void gv_envx_lfo_unpack(void)
{
    uint32_t i;

    for (i = DUB_LFO_ENVX; i < DUB_LFO_ENVX_END; i++)
        fm_lfo_slot_reset(&g.lfo_slots[i]);
    for (i = 0u; i < DUB_ENVX_LFO_SAVE_N; i++) {
        const dub_envx_lfo_t *e = &g.cfg.envx_lfo[i];

        if (e->id < DUB_LFO_ENVX_END - DUB_LFO_ENVX && e->s.depth)
            g.lfo_slots[DUB_LFO_ENVX + e->id] = e->s;
    }
}

/* ---- raw (legacy) calls ---- */

typedef struct {
    uint8_t bank, page, sel_drm, view, seq_page;
} gv_ctx_t;

static void gv_enter(gv_ctx_t *c, uint8_t bank, uint8_t pg, uint8_t drm)
{
    c->bank = g.bank;
    c->page = g.page[bank];
    c->sel_drm = g.sel_drm;
    c->view = g.view;
    c->seq_page = g.seq_page;
    g.bank = bank;
    g.page[bank] = pg;
    if (drm != 255u)
        g.sel_drm = drm;
    gv_raw = 1u;
}

static void gv_leave(const gv_ctx_t *c)
{
    g.page[g.bank] = c->page;
    g.bank = c->bank;
    g.sel_drm = c->sel_drm;
    g.view = c->view;
    g.seq_page = c->seq_page;
    gv_raw = 0u;
}

static void gv_enter_groove(gv_ctx_t *c)
{
    gv_enter(c, g.bank, g.page[g.bank], 255u);
    g.view = VIEW_SEQ;
    g.seq_page = 5u;
}

/* ENV ATTACK/DECAY: legacy bank + 0xPS for the ENV voice. */
static uint8_t gv_env_legacy(uint8_t slot, uint8_t *bank, uint8_t *drm)
{
    uint8_t v = gv_env % GV_N;
    uint8_t ln = GV_LANE[v];

    *bank = v == 3u ? (uint8_t)BANK_BASS : v == 4u ? (uint8_t)BANK_CHORD : (uint8_t)BANK_DRUM;
    *drm = ln < 4u ? ln : 255u;
    if (*bank == BANK_DRUM)
        return (uint8_t)(0x12u + slot);
    return (uint8_t)(0x02u + slot);
}

/* Resolve the current knob: 1 = legacy call via (bank, m, drm); 0 = own handling (m = code). */
static int gv_resolve(uint8_t slot, uint8_t *bank, uint8_t *m, uint8_t *drm)
{
    uint8_t pg = bank_page();

    *drm = 255u;
    if (gv_is_sel_bank(g.bank)) {
        *bank = g.bank;
        if (g.sel_drm > 3u)
            g.sel_drm = 0u;
        if (g.sel_drm < 3u)
            gv_drum = g.sel_drm;
        *m = GV_MAP[gv_cur_scr()][pg % 3u][slot & 3u];
        return *m < GV_LPMP;
    }
    if (g.bank == BANK_ENV && pg == 0u && slot < 2u) {
        *m = gv_env_legacy(slot, bank, drm);
        return 1;
    }
    *bank = g.bank;
    *m = (uint8_t)((pg << 4) | (slot & 3u));
    return 0;
}

static int gv_bank_owned(void)
{
    return gv_active() && (gv_is_sel_bank(g.bank) || g.bank == BANK_ENV || g.bank == BANK_GROOVE);
}

static void gv_label_fix(uint8_t bank, uint8_t m, char *s)
{
    if (bank == BANK_BASS && m == 0x13u)
        copy_str(s, GV_WOB_LBL);
    else if ((bank == BANK_BASS || bank == BANK_CHORD) && m == 0x20u)
        copy_str(s, "LEVEL");
    else if (bank == BANK_CHORD && m == 0x00u)
        copy_str(s, "CHORD");
}

/* ---- ENV own params ---- */

static dub_envx_v_t *gv_envx(void)
{
    return &g.cfg.envx[GV_LANE[gv_env % GV_N]];
}

static uint8_t gv_env_u8_of(const dub_envx_v_t *e, uint8_t m)
{
    switch (m) {
    case 0x02u: return e->sus;
    case 0x03u: return e->rel;
    case 0x10u: return e->dly;
    case 0x11u: return e->hold;
    case 0x12u: return dub_envx_u8(e->curve);
    case 0x13u: return e->noretrig ? 0u : 127u;
    case 0x20u: return e->inv ? 127u : 0u;
    case 0x21u: return dub_envx_u8(e->pdec);
    case 0x22u: return dub_envx_u8(e->sweep);
    default: return (uint8_t)(((gv_env % GV_N) * 127u) / (GV_N - 1u));
    }
}

static uint8_t gv_env_u8(uint8_t m)
{
    return gv_env_u8_of(gv_envx(), m);
}

static void gv_env_label(uint8_t m, char *s)
{
    if (m == 0x23u)
        copy_str(s, "VOICE");
    else
        copy_str(s, FM_ENV_KIT_LBL[(m >> 4) % FM_ENV_KIT_PAGES][m & 3u]);
}

static void gv_env_value(uint8_t m, char *s)
{
    uint8_t v = gv_env_u8(m);

    if (m == 0x13u || m == 0x20u)
        copy_str(s, v ? "ON" : "OFF");
    else if (m == 0x22u)
        bip50_show(s, v);
    else if (m == 0x12u)
        copy_str(s, fm_env_shape_name(v));
    else if (m == 0x23u)
        copy_str(s, gv_vox_name(gv_env % GV_N));
    else
        u8_show100_dec(s, v);
}

static void gv_env_adjust(uint8_t m, uint8_t slot, int32_t steps)
{
    dub_envx_v_t *e = gv_envx();
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    int32_t d = knob_gain(slot, steps);
    uint8_t u;

    if (steps == 0)
        return;
    if (m == 0x23u) {
        int32_t v = (int32_t)(gv_env % GV_N) + (steps > 0 ? 1 : -1);

        if (edit)
            v = steps < 0 ? 0 : (int32_t)GV_N - 1;
        if (v < 0 || v >= (int32_t)GV_N)
            return;
        gv_from_lane(GV_LANE[v]);
        return;
    }
    if (m == 0x13u || m == 0x20u) {
        uint8_t on = steps > 0;

        if (m == 0x13u)
            e->noretrig = (uint8_t)!on;
        else
            e->inv = on;
    } else if (edit) {
        u = steps < 0 ? 0u : 127u;
        if (m == 0x12u || m == 0x21u || m == 0x22u)
            u = 64u;
        goto store;
    } else {
        u = gv_env_u8(m);
        adj_u8(&u, d, 0u, 127u);
store:
        switch (m) {
        case 0x02u: e->sus = u; break;
        case 0x03u: e->rel = u; break;
        case 0x10u: e->dly = u; break;
        case 0x11u: e->hold = u; break;
        case 0x12u: e->curve = dub_envx_off(u); break;
        case 0x21u: e->pdec = dub_envx_off(u); break;
        case 0x22u: e->sweep = dub_envx_off(u); break;
        default: break;
        }
    }
    gv_envx_sync();
    mark_dirty();
}

/* ---- LEAD own params ---- */

static uint8_t *gv_lead_u8(uint8_t m)
{
    return m == GV_LDRV ? &g.cfg.p.drm_drv[3] : m == GV_LPMP ? &g.cfg.p.pan[3] : 0;
}

static void gv_lead_adjust(uint8_t m, uint8_t slot, int32_t steps)
{
    uint8_t *v = gv_lead_u8(m);
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;

    if (!v || steps == 0)
        return;
    dub_edit_u8(steps, edit, v, 0u, 127u);
    if (!edit)
        adj_u8_morph(v, knob_gain(slot, steps), 0, 127);
    dub_morph_sync_field(v);
    mark_dirty();
}

/* ---- hooks called from fm_app.c ---- */

static int gv_knob_adjust(uint8_t slot, int32_t steps)
{
    uint8_t bank, m, drm;
    gv_ctx_t c;

    if (!gv_bank_owned())
        return 0;
    if (g.bank == BANK_GROOVE) {
        gv_enter_groove(&c);
        knob_adjust(slot, steps);
        gv_leave(&c);
        return 1;
    }
    if (gv_resolve(slot, &bank, &m, &drm)) {
        gv_enter(&c, bank, (uint8_t)(m >> 4), drm);
        knob_adjust((uint8_t)(m & 3u), steps);
        gv_leave(&c);
        return 1;
    }
    if (g.bank == BANK_ENV) {
        uint8_t lt = gv_env_lfo_tgt(m);

        if (lt != 255u && dub_lfo_dest_touch(lt) && (fm1_in.buttons & (1u << BTN_LFO))) {
            if (fm1_in.buttons & (1u << BTN_EDIT))
                g.lfo_slots[lt].depth = steps < 0 ? 0u : 127u;
            else
                adj_u8(&g.lfo_slots[lt].depth, knob_gain(slot, steps), 0, 127);
            mark_dirty();
            return 1;
        }
        gv_env_adjust(m, slot, steps);
        return 1;
    }
    if (gv_lead_u8(m)) {
        gv_lead_adjust(m, slot, steps);
        return 1;
    }
    if ((m == GV_VOX || m == GV_DRV) && steps != 0) {
        uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
        int32_t hi = m == GV_VOX ? (int32_t)GV_SCR - 1 : 2;
        int32_t v = (m == GV_VOX ? (int32_t)gv_cur_scr() : (int32_t)gv_drum) + (steps > 0 ? 1 : -1);

        if (edit)
            v = steps < 0 ? 0 : hi;
        if (v >= 0 && v <= hi) {
            if (m == GV_DRV) {
                gv_drum = (uint8_t)v;
                gv_select_scr(0u, 1);
            } else
                gv_select_scr((uint8_t)v, 1);
        }
    }
    return 1;
}

static int gv_knob_label(uint8_t slot, char *s)
{
    uint8_t bank, m, drm;
    gv_ctx_t c;

    if (!gv_bank_owned())
        return 0;
    if (g.bank == BANK_GROOVE) {
        gv_enter_groove(&c);
        knob_label(slot, s);
        gv_leave(&c);
        return 1;
    }
    if (gv_resolve(slot, &bank, &m, &drm)) {
        gv_enter(&c, bank, (uint8_t)(m >> 4), drm);
        knob_label((uint8_t)(m & 3u), s);
        gv_leave(&c);
        gv_label_fix(bank, m, s);
        return 1;
    }
    if (g.bank == BANK_ENV)
        gv_env_label(m, s);
    else
        copy_str(s, m == GV_VOX ? "SEL" : m == GV_DRV ? "VOICE" : m == GV_LDRV ? "DRIVE" :
                    m == GV_LPMP ? "PUMP" : "");
    return 1;
}

static int gv_knob_value(uint8_t slot, char *s)
{
    uint8_t bank, m, drm;
    gv_ctx_t c;

    if (!gv_bank_owned())
        return 0;
    if (g.bank == BANK_GROOVE) {
        gv_enter_groove(&c);
        knob_value(slot, s);
        gv_leave(&c);
        return 1;
    }
    if (gv_resolve(slot, &bank, &m, &drm)) {
        gv_enter(&c, bank, (uint8_t)(m >> 4), drm);
        knob_value((uint8_t)(m & 3u), s);
        gv_leave(&c);
        return 1;
    }
    if (g.bank == BANK_ENV)
        gv_env_value(m, s);
    else if (gv_lead_u8(m))
        u8_show100_dec(s, *gv_lead_u8(m));
    else
        copy_str(s, m == GV_VOX ? gv_scr_name(gv_cur_scr()) : m == GV_DRV ? seq_lane_name(gv_drum) : "");
    return 1;
}

static int gv_lfo_target(uint8_t slot, uint8_t *out)
{
    uint8_t bank, m, drm;
    gv_ctx_t c;

    if (!gv_bank_owned())
        return 0;
    *out = 255u;
    if (g.bank == BANK_GROOVE) {
        gv_enter_groove(&c);
        *out = lfo_target_for_knob(slot);
        gv_leave(&c);
    } else if (gv_resolve(slot, &bank, &m, &drm)) {
        gv_enter(&c, bank, (uint8_t)(m >> 4), drm);
        *out = lfo_target_for_knob((uint8_t)(m & 3u));
        gv_leave(&c);
    } else if (g.bank == BANK_ENV)
        *out = gv_env_lfo_tgt(m);
    return 1;
}

static uint16_t gv_btn_col(uint8_t bank)
{
    if (bank == BANK_ENV)
        return BANK_COL[BANK_CHORD];
    if (bank == BANK_GROOVE)
        return BANK_COL[BANK_BASS];
    return BANK_COL[BANK_DRUM];
}

static void gv_knob_slot_fill(uint32_t col, ko_knob_slot_t *k)
{
    uint8_t bank, m, drm;
    uint8_t b0 = g.bank;
    gv_ctx_t c;

    if (!gv_bank_owned()) {
        knob_slot_fill(col, k);
        return;
    }
    if (b0 == BANK_GROOVE) {
        gv_enter_groove(&c);
        knob_slot_fill(col, k);
        gv_leave(&c);
        k->lcol = gv_btn_col(b0);
        return;
    }
    if (gv_resolve((uint8_t)col, &bank, &m, &drm)) {
        gv_enter(&c, bank, (uint8_t)(m >> 4), drm);
        knob_slot_fill(m & 3u, k);
        gv_leave(&c);
        gv_label_fix(bank, m, k->label);
        if (!(m == 0x00u && (bank == BANK_DRUM || bank == BANK_BASS)))
            k->lcol = gv_btn_col(b0);
        return;
    }
    k->klab = C_GRAY;
    k->lcol = gv_btn_col(b0);
    k->vcol = C_WHITE;
    k->pot_col = C_HI;
    k->mark_col = C_ACC;
    k->bip = 0u;
    k->steps = 0u;
    k->mode = KNOB_POT_CONT;
    k->lfo = 0u;
    k->morph = 0u;
    k->dj = 0u;
        if (b0 == BANK_ENV) {
        uint8_t lt = gv_env_lfo_tgt(m);

        gv_env_label(m, k->label);
        gv_env_value(m, k->value);
        k->pv = gv_env_u8(m);
        if (lt != 255u && g.lfo_slots_eff[lt].depth) {
            k->lfo = 1u;
            k->pv = gv_env_u8_of(&g.synth.envx[GV_LANE[gv_env % GV_N]], m);
        }
        if (m == 0x13u || m == 0x20u) {
            k->mode = KNOB_POT_TOG;
            k->steps = 2u;
        } else if (m == 0x12u) {
            k->custom_arg = ((uint32_t)k->pv * 255u) / 127u;
            k->custom_value = fm_env_kit_knob_val_shape;
        } else if (m == 0x22u)
            k->bip = 1u;
        else if (m == 0x23u) {
            k->mode = KNOB_POT_SEL;
            k->steps = (uint8_t)GV_N;
        }
        k->range = 0u;
        if (k->lfo && k->mode == KNOB_POT_CONT && !k->custom_value) {
            const fm_lfo_slot_t *sl = &g.lfo_slots_eff[lt];
            uint8_t lo, hi;
            uint32_t bpm = g.voice.p.bpm ? g.voice.p.bpm : (g.cfg.p.bpm ? g.cfg.p.bpm : 120u);

            if (fm_lfo_readout_update((uint8_t)col, k->pv, gv_env_u8(m), sl->depth, sl->rate, bpm, &lo, &hi)) {
                k->range = 1u;
                if (k->bip) {
                    bip50_show_plain(k->range_lo, lo);
                    bip50_show_plain(k->range_hi, hi);
                } else {
                    u8_show100_dec(k->range_lo, lo);
                    u8_show100_dec(k->range_hi, hi);
                }
            }
        }
        return;
    }
    if (m == GV_VOX || m == GV_DRV) {
        uint8_t n = m == GV_VOX ? (uint8_t)GV_SCR : 3u;
        uint8_t ix = m == GV_VOX ? gv_cur_scr() : gv_drum;

        copy_str(k->label, m == GV_VOX ? "SEL" : "VOICE");
        copy_str(k->value, m == GV_VOX ? gv_scr_name(ix) : seq_lane_name(ix));
        k->pv = (uint8_t)((ix * 127u) / (n - 1u));
        k->mode = KNOB_POT_SEL;
        k->steps = n;
        return;
    }
    if (gv_lead_u8(m)) {
        copy_str(k->label, m == GV_LDRV ? "DRIVE" : "PUMP");
        k->pv = *gv_lead_u8(m);
        u8_show100_dec(k->value, k->pv);
        return;
    }
    k->label[0] = '\0';
    k->value[0] = '\0';
    k->pv = 0u;
}

static int gv_reset_bank(uint8_t bank)
{
    if (bank == BANK_ENV) {
        dub_envx_v_t z = {0};

        *gv_envx() = z;
        gv_envx_sync();
        mark_dirty();
        return 1;
    }
    if (bank == BANK_GROOVE) {
        dub_plate_groove_read(g.cfg.p.plate, &g.cfg.p.root, &g.cfg.p.scale, &g.cfg.p.swing, &g.cfg.p.bpm);
        dub_morph_sync_field(&g.cfg.p.root);
        dub_morph_sync_field(&g.cfg.p.scale);
        dub_morph_sync_field(&g.cfg.p.swing);
        dub_morph_sync_field(&g.cfg.p.bpm);
        transport_set_bpm();
        mark_dirty();
        return 1;
    }
    return 0;
}

static const char *gv_stage_name(void)
{
    uint8_t pg = bank_page();

    if (g.bank == BANK_ENV)
        return pg == 0u ? "ENV" : pg == 1u ? "SHAPE" : "PITCH";
    if (g.bank == BANK_GROOVE)
        return "GROOVE";
    if (gv_is_sel_bank(g.bank))
        return pg == 0u ? gv_scr_name(gv_cur_scr()) : pg == 1u ? "TONE" : "MIX";
    return 0;
}

/* ---- views ---- */

static void gv_lane_ad(const dub_params_t *p, uint8_t ln, uint8_t *atk, uint8_t *dec)
{
    if (ln == 4u) {
        *atk = p->atk_b;
        *dec = p->decay_b;
    } else if (ln == 5u) {
        *atk = p->atk_c;
        *dec = p->decay_c;
    } else {
        *atk = p->drm_atk[ln & 3u];
        *dec = p->drm_dec[ln & 3u];
    }
}

static int32_t gv_env_live(uint8_t ln, int32_t *peak)
{
    dub_envx_lane_t L;
    int32_t pk;

    dub_envx_lane(&g.synth, ln, &L);
    pk = g.synth.envx_rt[ln].pk > 0 ? g.synth.envx_rt[ln].pk : *L.peak;
    *peak = pk > 0 ? pk : 1;
    return *L.env;
}

static int32_t gv_env_playhead_x(int32_t lvl, int32_t peak, const fm_env_kit_geom_t *gm, uint8_t curve)
{
    int32_t target = FM_ENV_KIT_Y_BOT - (lvl * (FM_ENV_KIT_Y_BOT - gm->y_peak)) / peak;
    int32_t best_x = gm->px[1];
    int32_t best_err = 0x7FFFFFFF;
    int32_t x;

    for (x = gm->px[2]; x <= gm->px[6]; x++) {
        int32_t err = fm_env_curve_y(x, gm->px, FM_ENV_KIT_Y_BOT, gm->y_peak, gm->y_sus, curve) - target;

        if (err < 0)
            err = -err;
        if (err < best_err) {
            best_err = err;
            best_x = x;
        }
    }
    return best_x;
}

static void gv_draw_env(void)
{
    uint8_t v = gv_env % GV_N;
    uint8_t ln = GV_LANE[v];
    const dub_envx_v_t *e = &g.synth.envx[ln];
    const dub_envx_rt_t *rt = &g.synth.envx_rt[ln];
    uint16_t col = ERA_COL[g.cfg.p.era % GENRE_ERA_N];
    fm_env_kit_plot_t ep;
    fm_env_kit_geom_t gm;
    uint8_t atk, dec;
    int32_t peak;
    int32_t live = gv_env_live(ln, &peak);

    gv_lane_ad(&g.voice.p, ln, &atk, &dec);
    ep.w[0] = e->dly;
    ep.w[1] = atk ? atk : 1u;
    ep.w[2] = e->hold;
    ep.w[3] = dec ? dec : 1u;
    ep.w[4] = e->sus ? (e->rel ? e->rel : 1u) : 0u;
    ep.sustain = e->sus;
    ep.curve = dub_envx_u8(e->curve);
    ep.invert = e->inv;
    ep.col = col;
    fm_env_kit_draw(&ep, &gm);

    if (e->sweep) {
        const int32_t y_bot = FM_ENV_KIT_Y_BOT;
        int32_t mag = ((int32_t)(e->sweep < 0 ? -e->sweep : e->sweep) * (y_bot - gm.y_peak)) / 63;
        int32_t len = 16 + ((127 - (int32_t)dub_envx_u8(e->pdec)) * (gm.px[6] - gm.px[1])) / 160;
        uint16_t pcol = color_scale_rgb(C_ACC, 150);
        int32_t i, x0 = gm.px[1], y0 = y_bot - mag;

        for (i = 1; i <= 16; i++) {
            int32_t r = 16 - i;
            int32_t x1 = gm.px[1] + (len * i) / 16;
            int32_t y1 = y_bot - (mag * r * r) / 256;

            cv_line(x0, y0, x1, y1, pcol);
            x0 = x1;
            y0 = y1;
        }
        cv_text(gm.px[1] + 2, y_bot - mag - 8, &FONT_S, e->sweep < 0 ? "P-" : "P", pcol);
    }
    if (live > 0) {
        int32_t lvl = e->inv ? (rt->tru > 0 ? rt->tru : 0) : live;

        fm_env_kit_draw_playhead(&ep, &gm,
                                 rt->dly_left ? gm.px[0] :
                                 g.synth.envx_rt[ln].hold_left ? gm.px[2] :
                                 gv_env_playhead_x(lvl, peak, &gm, ep.curve));
    }
    fm_env_kit_draw_gate(live > 0 ? 1u : 0u, e->noretrig ? 0u : 1u);
    cv_text(4, 4, &FONT_S, gv_vox_name(v), col);
}

static void gv_draw_glo(void)
{
    fm_groove_plot_t gp = {0};

    gp.page = 0u;
    gp.bpm = dub_groove_show_bpm();
    gp.swing = dub_groove_show_swing();
    gp.root = g.cue_on ? g.cue_root : g.cfg.p.root;
    gp.scale = dub_groove_show_scale();
    gp.playing = g.playing ? 1u : 0u;
    gp.step = (uint8_t)(g.step % 16u);
    gp.step_frac256 = g.playing ? dub_sample_acc_frac256(g.sample_acc, g.samples_per_step) : 0u;
    gp.clock_mode = g.ext_clock ? 1u : 0u;
    gp.clock_n = 2u;
    gp.col = ERA_COL[g.cfg.p.era % GENRE_ERA_N];
    fm_groove_plot_draw(&gp);
}

/* Desk scope rows: KICK/SNARE/HAT spread over the plot; LEAD has its own panel (gv_draw_lead). */
static uint32_t gv_desk_row0(void)
{
    return 0u;
}

static uint32_t gv_desk_row_end(void)
{
    return 3u;
}

static int32_t gv_desk_mid(uint32_t row)
{
    return 21 + (int32_t)row * 41;
}

/* LEAD screen: one full-width trace laid out like the BASS panel (draw_view_panel). */
static int32_t gv_lead_wave(uint32_t ph, int32_t timb)
{
    int32_t sq_w = 128 + (timb * 3) / 4;
    int32_t si_w = 384 - (timb * 3) / 4;

    if (sq_w < 0)
        sq_w = 0;
    if (si_w < 64)
        si_w = 64;
    return ((dub_square(ph) * sq_w) + (dub_sine_q64(ph) * si_w)) / (sq_w + si_w);
}

static void gv_draw_lead(void)
{
    const dub_params_t *p = &g.voice.p;
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    int32_t kt = (int32_t)(p->drm_tune[3] ? p->drm_tune[3] : 64u) - 64;
    int32_t timb = (int32_t)(p->drm_dens[3] ? p->drm_dens[3] : 64u) - 64;
    int32_t cut = (int32_t)(p->drm_cut[3] ? p->drm_cut[3] : 64u);
    int32_t dec = (int32_t)(p->drm_dec[3] ? p->drm_dec[3] : 64u);
    int32_t rate = (int32_t)p->siren_rate;
    int32_t lvl = (int32_t)p->siren_pitch;
    int32_t drv = (int32_t)p->drm_drv[3];
    int32_t pmp = (int32_t)p->pan[3];
    int32_t dly = (int32_t)p->drm_dly[3];
    int32_t rev = (int32_t)p->drm_rev[3];
    int32_t pan = (int32_t)(p->drm_pan[3] ? p->drm_pan[3] : 82u);
    int muted = !dub_lane_audible(3u);
    uint32_t bpm = p->bpm ? p->bpm : 120u;
    uint32_t now = (fm1_ms * (bpm / 8u + (uint32_t)rate / 8u)) << 18;
    uint32_t step_ph = 26000000u + (uint32_t)(kt + 64) * 300000u;
    uint32_t vib_step = 9000000u + (uint32_t)rate * 700000u;
    uint32_t d_off = (uint32_t)(1u + dub_dly_div_ix(p->dly_time) / 3u) << 29;
    uint8_t smooth = cut < 105 ? (uint8_t)(122u - (uint32_t)(cut * 114) / 105u) : 0u;
    int32_t dec_drop = 700 - (dec * 640) / 127;
    int32_t g_amp = (16 + (dec * 30) / 127) * dly / 127;
    int32_t mid = 70, env = 256, prev = 0, gprev = 0, py = -1, gpy = -1;
    int32_t pan_x = 8 + ((pan - 1) * 224) / 126;
    uint32_t ph = now, x;

    cv_text(4, 4, &FONT_S, seq_lane_name(3u), C_WHITE);
    cv_line(8, mid, 231, mid, C_LINE);
    if (pan_x < 8)
        pan_x = 8;
    if (pan_x > 231)
        pan_x = 231;
    cv_line(pan_x, 148, pan_x, 156, C_DIM);

    for (x = 0; x < 224u; x++) {
        int32_t xp = 8 + (int32_t)x;
        int32_t vib = (dub_sine_q64(now + x * vib_step) * rate) >> 7;
        int32_t raw = gv_lead_wave(ph, timb);
        int32_t w = fm_lfo_slew_step(prev, raw, smooth);
        int32_t pump_att = 256;
        int32_t amp, y;
        uint16_t col = muted ? C_DIM : (cut > 90 ? C_WHITE : ecol);

        prev = w;
        ph += step_ph + (uint32_t)((((int32_t)(step_ph >> 16) * vib) >> 2));
        env -= (env * dec_drop) >> 13;
        if (env < 44)
            env = 44;
        if (pmp > 8 && x < 72u) {
            int32_t dip = ((72 - (int32_t)x) * pmp * 3) / 72;

            if (dip > 220)
                dip = 220;
            pump_att = 256 - dip;
        }
        if (drv > 0) {
            int32_t hot = (w * (256 + drv * 4)) >> 8;

            w = dub_softclip_q15(hot);
            if (!muted && drv > 40 && (hot > 26000 || hot < -26000))
                col = C_ACC;
        }
        amp = ((14 + (cut * 34) / 127) * env >> 8) * pump_att >> 8;
        amp = (amp * lvl) / 96;
        y = mid - (w * amp) / 32767;
        if (y < 20)
            y = 20;
        if (y > 122)
            y = 122;

        /* DELAY ghost: same trace TIME-shifted, height from DELAY */
        if (dly > 8) {
            int32_t gw = fm_lfo_slew_step(gprev, gv_lead_wave(ph - d_off, timb), smooth);
            int32_t gy = mid - (gw * g_amp) / 32767;

            gprev = gw;
            if (gy < 20)
                gy = 20;
            if (gy > 122)
                gy = 122;
            if (gpy >= 0)
                cv_line(xp - 1, gpy, xp, gy, dly > 64 ? C_DIM : C_LINE);
            gpy = gy;
        }
        if (!muted && (x & 1u) == 0u)
            cv_line(xp, mid, xp, y, C_LINE);
        if (rev > 6 && lvl > 0 && (x & 3u) == 0u) {
            int32_t ry = mid - (dub_sine_q64(ph - 0x28000000u) * ((amp * rev) / 160)) / 32767;

            if (ry < 20)
                ry = 20;
            if (ry > 122)
                ry = 122;
            cv_line(xp, mid, xp, ry, rev > 72 ? C_DIM : C_LINE);
        }
        if (py >= 0) {
            cv_line(xp - 1, py, xp, y, col);
            if (drv > 24 && y + 1 <= 122)
                cv_line(xp - 1, py + 1, xp, y + 1, col);
            if (drv > 84 && y - 1 >= 20)
                cv_line(xp - 1, py - 1, xp, y - 1, C_HI);
        }
        py = y;
    }
}

/* SEL LED: lit for whichever voice bank is open. */
static uint8_t gv_led_bank(uint8_t bank)
{
    if (bank == BANK_DRUM && gv_is_sel_bank(g.bank))
        return g.bank;
    return bank;
}

#endif /* !GENRE_BASS */
#endif /* GENRE_VOX_H */
