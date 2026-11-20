/* SPDX-License-Identifier: GPL-3.0-only */
#include "platform.c"
#include <stddef.h>
#include "fm_lfo.h"
#include "genre_conf.h"
#include "fm_dj_conf.h"
#ifndef APP_NAME
#define APP_NAME GENRE_APP_NAME
#endif
#ifndef FELUCCA_ID
#define FELUCCA_ID GENRE_FELUCCA_ID
#endif
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
#define C_ACC RGB(255, 140, 180)
#elif !defined(C_ACC)
#define C_ACC GENRE_C_ACC
#endif
#include "fm_dj.h"
#include "fm_dj_app.h"
#ifndef FM_DJ_APP_WIRED
#error "DJ FX longpress not wired — include fm_dj_app.h and call fm_dj_app_fx_long_press_poll + fm_dj_app_phrase_step"
#endif
#include "dub_types.h"
#define KO_POT_TICK_CAP DUB_LFO_TGT_N
#include "ko_chrome.h"
#include "ko_era_plate.h"
#include "fm_seq.h"
#include "genre_pack.h"
#include "dub_dsp.h"
#if defined(GENRE_BASS)
#include "bass_app.h"
#endif
#include "fm_dj_climax.h"
#include "fm_dj_seq.h"
#include "fm_dj_lfo.h"
#include "fm_dj_drop.h"
#include "fm_dj_fx.h"
#ifndef FM_DJ_FX_WIRED
#error "DJ FX scenes not wired"
#endif
#ifndef FM_DJ_LFO_WIRED
#error "DJ LFO overlay not wired — include fm_dj_lfo.h"
#endif
#include "fm_dj_gest.h"
#include "perf_toast_policy.h"
#ifndef FM_DJ_GEST_WIRED
#error "DJ gesture planner not wired"
#endif
#if defined(GENRE_BASS)
#define DUB_CFG_MAGIC      0x42415338u /* BAS8: BAS7 + step ACCENT/SLIDE level 0..3 */
#else
#define DUB_CFG_MAGIC      (GENRE_CFG_MAGIC ^ 0x80000002u) /* 128-step seq + CPL shares + SEQ_PACE depth (lfo_cpl tail) */
#endif
#if !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_HOUSE) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) \
    && !defined(GENRE_SNOWCRASH) && !defined(GENRE_TECHNO) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL) && !defined(GENRE_BASS) \
    && !defined(GENRE_HIPHOP) && !defined(GENRE_FUNK) && !defined(GENRE_POP)
#define FM_DUB_SRN_PLOCK 1
#endif
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_JAZZ) || defined(GENRE_SOUL) \
    || defined(GENRE_FUNK) || defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) \
    || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_SOUL) || defined(FM_DUB_SRN_PLOCK)
#define FM_GENRE_STEP_MICRO 1
#endif
#define DUB_SEQ_NPAGES 6u
/* Knob-page cycle: FM_SEQ_KNOB_PAGE_CYCLE in fm_seq.h; slot 3 unused (legacy). */

static int dub_seq_page_in_cycle(uint8_t p)
{
    uint32_t i;

    for (i = 0u; i < FM_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (FM_SEQ_KNOB_PAGE_CYCLE[i] == p)
            return 1;
    }
    return 0;
}

/* KO BASS: GATE/ACCENT/SLIDE/RATCHET only in the white long-press step overlay; SEQ taps 1 <-> 2. */
#if defined(GENRE_BASS)
#define DUB_SEQ_PAGE_ENTRY 1u
#else
#define DUB_SEQ_PAGE_ENTRY 0u
#endif

static uint8_t dub_seq_page_norm(uint8_t p)
{
#if defined(GENRE_BASS)
    if (p != 1u && p != 2u)
        return DUB_SEQ_PAGE_ENTRY;
#endif
    if (!dub_seq_page_in_cycle(p))
        return 0u;
    return p;
}

static uint8_t dub_seq_page_cycle_next(uint8_t cur)
{
#if defined(GENRE_BASS)
    return cur == 1u ? 2u : 1u;
#else
    uint32_t i;

    cur = dub_seq_page_norm(cur);
    for (i = 0u; i < FM_SEQ_KNOB_PAGE_CYCLE_N; i++) {
        if (FM_SEQ_KNOB_PAGE_CYCLE[i] == cur)
            return FM_SEQ_KNOB_PAGE_CYCLE[(i + 1u) % FM_SEQ_KNOB_PAGE_CYCLE_N];
    }
    return 0u;
#endif
}
#define DUB_SEQ_MASTER_THRESH 200
/* TEMP emu/host: SAVE long-press opens user-song setup. Remove when done. */
#define DUB_TEMP_SONG_SETUP_LONGPRESS 1
#if defined(DINGHY_WEB)
static uint8_t dub_web_store_primmed;
#endif
#define DUB_SONG_NAME_LEN 7u
#define FM_ERA_ARTIST      GENRE_ERA_N
#define FM_ERA_KNOB_N      (GENRE_ERA_N + 1u)
#define DUB_SEQ_LANE_ALL 6u
#define DUB_SEQ_CLIP_NONE 0u
#define DUB_SEQ_CLIP_VIEW 1u
#define DUB_SEQ_CLIP_LANE 2u

#define KEY0 53u

enum {
    VIEW_DESK = 0,
    VIEW_CHORD,
    VIEW_BASS,
    VIEW_SPACE,
    VIEW_SEQ,
    VIEW_HOME,
    VIEW_LFO,
    VIEW_COUNT
};

enum {
    BANK_CHORD = 0,
    BANK_BASS,
    BANK_DRUM,
    BANK_FX,
    BANK_LFO,
    BANK_HOME,
    BANK_SEQ,
    BANK_GROOVE, /* GLO */
#if !defined(GENRE_BASS)
    BANK_ENV,    /* KO KIT ENV per voice (genre_vox.h) */
#endif
    BANK_COUNT
};

enum {
    GEST_NONE = 0,
    GEST_FILL,
    GEST_BREAK,
    GEST_SOLO_DRUM,
    GEST_SOLO_BASS,
    GEST_SOLO_BAND,
    GEST_FLAM
};

#if !defined(GENRE_BASS)
#define DUB_ENVX_LFO_SAVE_N 8u
typedef struct {
    uint8_t id; /* DUB_LFO_ENVX offset, 0xFF = empty */
    fm_lfo_slot_t s;
} dub_envx_lfo_t;
#endif

typedef struct {
    uint32_t magic;
    dub_params_t p;
    dub_step_t seq[DUB_SEQ_STEPS];
    fm_lfo_slot_t lfo_slots[DUB_LFO_TGT_PRE_CPL]; /* CPL shares are stored in lfo_cpl (tail), not here */
    uint8_t lfo_target;
    uint8_t euclid_hits[6];
    int8_t euclid_rot[6];
    uint8_t vary_lane[6];
    int8_t lane_vel[6];
    int8_t lane_trig[6];
    int8_t lane_gate[6];
    int8_t lane_tune[6];
#if defined(GENRE_BASS)
    /* Only the bass lane (4) is persisted; drum/siren/chord pLocks are never edited in KO BASS. */
    uint8_t step_vel[DUB_SEQ_STEPS];
    uint8_t step_prob[DUB_SEQ_STEPS];
    int8_t step_micro[DUB_SEQ_STEPS];
    int8_t step_tune[DUB_SEQ_STEPS];
#else
    uint8_t step_vel[6][DUB_SEQ_STEPS];
    uint8_t step_prob[6][DUB_SEQ_STEPS];
    int8_t step_micro[6][DUB_SEQ_STEPS];
    int8_t step_tune[6][DUB_SEQ_STEPS];
#endif
    uint64_t user_plate_mask;
#if defined(GENRE_BASS)
    bass_plate_t user_plates[GENRE_CFG_USER_PLATE_N];
#else
    dub_params_t user_plates[GENRE_CFG_USER_PLATE_N];
#endif
    uint8_t seq_lfo_lane[9];
    uint8_t seq_pages;
    uint8_t user_song;
    char song_name[8];
#if defined(GENRE_BASS)
    bass_cfg_ext_t bass;
#endif
#if defined(FM_PGEN_ON)
    fm_pgen_cfg_t pgen __attribute__((aligned(8)));
#endif
    /* SEQ page-4 arrangement, part of the song: stored only while user_song == 1 (kept as-is in preset mode).
     * Runtime copy: g.seq_view_cyc / g.seq_view_reps; preset mode derives those from the plate's triggers. */
    uint8_t seq_cyc[FM_SEQ_MAX_PAGES];
    uint8_t seq_reps[FM_SEQ_MAX_PAGES];
    /* CPL share depths (0..127), one per DUB_LFO_CPL_* id. */
    uint8_t lfo_cpl[DUB_CPL_N];
    /* SEQ PACE per lane: index into FM_SEQ_PACE4_TICKS (tail-appended; 1 = 1/16 = the global grid). */
    uint8_t seq_pace[DUB_SEQ_LANES];
#if defined(GENRE_BASS)
    /* ENV CURVE (tail-appended), bass_env_shape_pack(); 0 in older blobs = LIN. */
    uint8_t bass_env_shape;
#else
    /* KO KIT ENV per lane (tail-appended); all-zero = legacy envelopes. */
    dub_envx_v_t envx[DUB_ENVX_LANES];
    /* Active ENV LFO slots (DUB_LFO_ENVX + id), sparse: the full 42-slot set does not fit the settings sector. */
    dub_envx_lfo_t envx_lfo[DUB_ENVX_LFO_SAVE_N];
#endif
} dub_cfg_t;

typedef struct {
    dub_cfg_t cfg;
    dub_synth_t synth;
    dub_voice_t voice;
    uint8_t view;
    uint8_t bank;
    uint8_t seq_ret_bank;
    uint8_t page[BANK_COUNT];
    uint8_t playing;
    uint8_t rec_arm;
    uint8_t console;
    uint8_t ext_clock;
    uint8_t step;
    uint8_t bar_step; /* steps played in this DUB_STEPS bar; g.step jumps on the view chain */
    uint8_t rail_step; /* sounding step for progress rail (set at fire, before bar_step++) */
    uint8_t rail_cell; /* rail bar << 2 | quarter as last drawn */
    uint32_t sample_acc;
    uint32_t samples_per_step;
    /* Per-lane PACE clock (lanes with pace != 1/16): own step counter, accumulator and samples per step. */
    uint8_t lane_st[DUB_SEQ_LANES];
    uint32_t lane_acc[DUB_SEQ_LANES];
    uint32_t lane_spp[DUB_SEQ_LANES];
    uint8_t lane_pace_cur[DUB_SEQ_LANES]; /* effective pace index last applied per lane (changed at grid tick only) */
    uint32_t lane_den; /* bpm * 24 of the last sync */
    uint8_t lane_any;  /* 1 = at least one lane runs its own clock */
    uint32_t prev_keys;
    uint32_t ui_dirty;
    uint32_t set_dirty;
    uint32_t set_t;
    uint32_t saved_until;
    uint32_t plate_home_until;
    uint32_t ui_ms;
    uint32_t bank_ms[BANK_COUNT];
    uint8_t rim_throw_pulse;
    uint8_t lfo_sel;
    uint8_t seq_lane;
    uint8_t seq_lane_sel; /* 0..5 lane, DUB_SEQ_LANE_ALL = all pages’ LANE knob */
    uint8_t seq_page;
    uint8_t seq_plock_hold;
    uint8_t seq_plock_latch;
    uint8_t seq_plock_bank; /* KO BASS step overlay: 0=VEL/COND/MICRO/PITCH (P1), 1=GATE/ACCENT/SLIDE/RATCHET (P2) */
    uint8_t seq_plock_step;
    uint8_t seq_plock_knob;
    uint8_t white_press_k;
    uint32_t white_press_ms;
    uint8_t white_long_used;
    uint8_t white_long_cancel;
    uint8_t euclid_hits[6];
    uint8_t pattern_origin[6];
    int8_t euclid_rot[6];
    uint8_t vary_lane[6];
    int8_t lane_vel[6];
    int8_t lane_trig[6];
    int8_t lane_gate[6];
    int8_t lane_tune[6];
    uint8_t step_vel[6][DUB_SEQ_STEPS];
    uint8_t step_prob[6][DUB_SEQ_STEPS];
    int8_t step_micro[6][DUB_SEQ_STEPS];
    int8_t step_tune[6][DUB_SEQ_STEPS];
    uint32_t bk_last_ms[6];
    uint8_t bar;
    uint32_t seq_rng;
#if defined(GENRE_BASS)
    uint32_t vary_seed; /* VARY roll per phrase; re-rolled only at step 0 */
#endif
    uint8_t edit_used;
    uint8_t gesture;
    uint8_t gesture_latch;
    uint8_t gesture_solo_kind; /* GEST_SOLO_* while solo counter runs */
    uint8_t gesture_solo_bar;  /* bars since gesture_solo_kind became active */
    uint8_t kick_slam_bars;    /* post-drop kick/rim emphasis bars remaining */
    uint8_t drop_emph_span;    /* span at arm time (for envelope shape) */
    uint8_t gest_hold[4];
    uint8_t lfo_target;
    uint64_t user_plate_mask;
    dub_params_t user_plates[GENRE_CFG_USER_PLATE_N];
    fm_lfo_slot_t lfo_slots[DUB_LFO_TGT_N];
    fm_lfo_slot_t lfo_slots_eff[DUB_LFO_TGT_N];
    fm_lfo_rt_t lfo_rt[DUB_LFO_TGT_N];
    dub_params_t morph_from;
    uint8_t morph_pos;
    uint8_t morph_tick; /* 16ths elapsed since morph begin (enum stagger) */
    uint32_t morph_acc;
    uint8_t cue_on;
    uint8_t groove_plate_pending; /* track/era change while playing: cfg groove locked, plate target pending */
    uint8_t cue_bpm, cue_swing, cue_root, cue_scale;
    uint32_t cue_acc;
    uint32_t cue_ticks;
    uint8_t morph_from_seq[FM_SEQ_MASK_BYTES];
    dub_step_t morph_from_step[DUB_SEQ_STEPS];
    uint32_t knob_ms[4];
    uint8_t seq_ens_sel;
    uint8_t seq_flip_state;
    uint8_t lfo_last_ens;
    uint8_t macro_from[10];
    uint8_t macro_morph_pos;
    uint32_t macro_morph_acc;
#if defined(GENRE_BASS)
    bass_app_rt_t bass_rt;
#endif
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    uint8_t fill_build;
    uint8_t fill_build_snap;
#elif defined(GENRE_HIPHOP)
    uint8_t fill_build;
    uint8_t fill_build_snap;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    uint8_t fill_build;
    uint8_t fill_build_snap;
    uint8_t fill_build_kind; /* GEST_FILL or GEST_BREAK */
    uint8_t fill_drop;       /* 1 = quantized drop punch (one bar) */
    uint8_t idm_edit_latch;
    idm_stutter_t idm_stutter;
#endif
#ifdef FM_GENRE_STEP_MICRO
    uint16_t micro_left[6];
    uint8_t micro_vel[6];
    int8_t micro_tune[6];
    uint8_t micro_atk[6];
    uint8_t micro_armed;
    uint8_t micro_note[6];
    uint8_t micro_mst[6];
#endif
    uint32_t home_morph_flash_ms;
    uint8_t home_morph_flash_kind; /* bits: DUB_FLASH_XF|EV|CUE (OR’d; all can show) */
    ko_flash_t seq_knob_flash;
    uint32_t edit_press_ms;
    uint32_t edit_groove_arm_ms;
    uint8_t edit_long_arm;
    uint8_t edit_long_cancel;
    uint32_t arp_press_ms;
    uint32_t gest_ms[4];
    uint8_t arp_long_used;
    uint32_t seq_press_ms;
    int32_t seq_press_master_q12;
    uint8_t seq_long_cancel;
    uint8_t seq_long_used;
    uint32_t fx_press_ms;
    uint8_t fx_long_cancel;
    uint8_t fx_long_used;
    fm_dj_t dj;
#if defined(FM_PGEN_ON)
    int8_t dj_comply_ofs; /* runtime only; added to cfg.pgen.comply while DJ is on */
#endif
    fm_dj_seq_ens_ov_t dj_ens_ov;
    fm_dj_lfo_ov_t dj_lfo_ov;
    fm_dj_fx_ov_t dj_fx_ov;
    uint8_t dj_gest_kind;
    uint8_t dj_gest_bars;
    uint8_t dj_gest_flam_steps;
    uint8_t dj_gest_saved_latch;
    uint8_t dj_gest_saved_gesture;
    uint8_t dj_gest_saved_arp;
    uint8_t dj_gest_saved_loop;
    uint8_t dj_gest_ov;
    uint8_t dj_gest_flam_cd;
    uint8_t dj_gest_loop_cd;
    uint8_t dj_gest_arp_cd;
    uint8_t dj_flip_overlay;
    uint8_t dj_ens_invert_overlay;
    uint8_t seq_mod_used;
    uint8_t seq_master_mod;
    uint8_t seq_was_in_seq;
#if DUB_TEMP_SONG_SETUP_LONGPRESS
    uint32_t save_press_ms;
    uint8_t save_long_used;
#endif
    uint32_t lfo_press_ms;
    uint8_t lfo_long_used;
    uint8_t song_setup_latch;
    char song_setup_name[8];
    uint8_t song_setup_name_cur;
    uint8_t song_setup_snap_bpm;
    uint8_t song_setup_snap_root;
    uint8_t song_setup_snap_scale;
    uint8_t song_setup_snap_swing;
    uint8_t song_setup_snap_gate[DUB_SEQ_STEPS];
    uint8_t song_setup_snap_force[DUB_SEQ_STEPS];
    uint8_t song_setup_snap_euclid[DUB_SEQ_LANES];
    uint8_t sel_drm; /* 0..3: KICK, RIM, HAT, SIREN */
    uint8_t loop_active;
    uint8_t loop_start_step;
    uint8_t loop_len;
    uint8_t loop_pending;
    uint8_t loop_pending_start;
    uint8_t loop_pending_len;
    uint8_t loop_end_pending;
    uint8_t loop_wander_acc; /* wraps since last drift (short-loop slow scrub) */
    uint8_t slip_step;
    uint8_t loop_snap_valid;
    dub_step_t loop_snap_step[DUB_SEQ_STEPS];
    uint8_t loop_snap_vel[6][DUB_SEQ_STEPS];
    uint8_t loop_snap_prob[6][DUB_SEQ_STEPS];
    int8_t loop_snap_micro[6][DUB_SEQ_STEPS];
    int8_t loop_snap_tune[6][DUB_SEQ_STEPS];
    uint8_t seq_lfo_lane[DUB_SEQ_LFO_LANE_N];
    char key_toast[16];
    uint8_t key_toast_lane;
    uint32_t key_toast_until;
    uint32_t key_toast_arm_ms;
    uint8_t seq_view_page;
    uint8_t seq_view_on[FM_SEQ_MAX_PAGES]; /* S1..S8 enabled; playback runs only through these (fm_seq_chain_at) */
    uint8_t seq_view_cyc[FM_SEQ_MAX_PAGES];  /* runtime: FM_SEQ_VIEW_CYC_* */
    uint8_t seq_view_reps[FM_SEQ_MAX_PAGES]; /* runtime: 1..16 */
    uint8_t seq_view_only_page;              /* FM_SEQ_MAX_PAGES = no ONLY latch */
    uint8_t seq_view_cyc_saved[FM_SEQ_MAX_PAGES]; /* pre-OFF cycle while ONLY active */
    uint8_t seq_lane_len[FM_SEQ_MAX_PAGES][6];
    int8_t seq_lane_nudge[6]; /* per lane, shared by all SEQ views; unused in BASS */
    uint8_t seq_view_rep_left; /* playback: reps left in current view */
    uint8_t seq_follow;
    uint8_t seq_clip_kind; /* DUB_SEQ_CLIP_* */
    uint8_t seq_clip_lane;
    dub_step_t seq_clip_step[FM_SEQ_PAGE_STEPS];
    uint8_t seq_clip_vel[DUB_SEQ_LANES][FM_SEQ_PAGE_STEPS];
    uint8_t seq_clip_prob[DUB_SEQ_LANES][FM_SEQ_PAGE_STEPS];
    int8_t seq_clip_micro[DUB_SEQ_LANES][FM_SEQ_PAGE_STEPS];
    int8_t seq_clip_tune[DUB_SEQ_LANES][FM_SEQ_PAGE_STEPS];
    char seq_clip_toast[28];
    uint32_t seq_clip_toast_until;
    char plate_era_toast[16];
    uint32_t plate_era_toast_until;
    uint16_t plate_era_toast_bar;
} dub_app_t;

static const uint8_t DUB_SAVED_LVL_DEF[6] = {80u, 75u, 70u, 76u, 90u, 88u};

#define DUB_MORPH_STEPS 64u /* morph_pos: 0 = slewing, STEPS = idle (not a lerp range) */
#define DUB_MORPH_ENUM_N 7u /* era, plate, drum_pat, voicing, bass_model, drum_kit, tap */
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
#define HOUSE_FILL_MORPH_STEPS 32u /* one bar (DUB_STEPS) */
#endif
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
#define IDM_STUTTER_MORPH_STEPS 32u  /* edit-latch / BREAK duck — 1 bar */
#define IDM_FILL_MORPH_STEPS    128u /* FILL build — 4 bars, ease-in quad */
#endif

static dub_app_t g;

#if !defined(GENRE_BASS)
/* genre_vox.h: SEL voice bank / ENV / GLO */
static int gv_knob_adjust(uint8_t slot, int32_t steps);
static int gv_knob_label(uint8_t slot, char *s);
static int gv_knob_value(uint8_t slot, char *s);
static int gv_lfo_target(uint8_t slot, uint8_t *out);
static int gv_reset_bank(uint8_t bank);
static const char *gv_stage_name(void);
static uint8_t gv_sel_bank(void);
static void gv_from_lane(uint8_t ln);
static void gv_envx_sync(void);
static void gv_envx_lfo_pack(void);
static void gv_envx_lfo_unpack(void);
static void gv_lfo_names_init(void);
#endif

#if defined(GENRE_BASS)
static void mark_dirty(void);
static uint8_t *dub_macro_ptr(uint8_t idx);
static void bass_draw_home_knob_label(uint8_t pg, uint8_t slot, char *s);
static void bass_draw_env_knob_label(uint8_t pg, uint8_t slot, char *s);
static void bass_groove_knob_label(uint8_t pg, uint8_t slot, char *s);
static void bass_groove_knob_value(uint8_t pg, uint8_t slot, char *s);
static uint8_t bass_groove_knob_bar(const dub_params_t *mp, uint8_t pg, uint8_t slot);
static void bass_draw_view_groove(void);
static void draw_view_env(void);
static void bass_draw_view_seq(void);
static void bass_knob_label_seq(uint8_t slot, char *s);
static void bass_knob_value_seq(uint8_t slot, char *s);
static uint8_t bass_knob_seq_bar_px(uint8_t slot);
static uint8_t bass_seq_edit_base(void);
static void bass_knob_adjust_seq(uint8_t slot, int32_t steps, int32_t d, uint8_t edit);
static int bass_knob_mod_routed(uint8_t slot);
static uint8_t bass_knob_mod_dest(uint8_t slot);
#if defined(GENRE_BASS)
static int8_t dub_eff_gate_rng(uint8_t ln);
static void seq_knob_lane_page1(uint8_t ln, uint8_t slot, int32_t steps, int edit, int d);
static void seq_knob_p3_state(uint8_t ln, int32_t steps, int edit);
static void seq_knob_vary(uint8_t ln, int32_t steps, int edit, int d);
#endif
static void seq_flash_knob(const char *msg);
static void bass_seq_toggle_step(uint8_t st);
static int bass_seq_lane_step_on(uint8_t st);
static int bass_seq_lane_base_on(uint8_t st);
static int bass_seq_vary_on(int base_on, uint8_t st);
#endif

#if defined(GENRE_BASS)
/* KO BASS has no drum lanes: per-drum LFO ids carry ENV / GROOVE p1 (save layout unchanged). */
#define DUB_LFO_ENV_DLY  DUB_LFO_KTUNE_K
#define DUB_LFO_ENV_ATK  DUB_LFO_KTUNE_R
#define DUB_LFO_ENV_HOLD DUB_LFO_KTUNE_H
#define DUB_LFO_ENV_DEC  DUB_LFO_KTUNE_S
#define DUB_LFO_ENV_SUS  DUB_LFO_DCUT_K
#define DUB_LFO_ENV_REL  DUB_LFO_DCUT_R
#define DUB_LFO_ENV_CURV DUB_LFO_DCUT_H
#define DUB_LFO_ENV_ACC  DUB_LFO_DCUT_S
#define DUB_LFO_TRANSP   DUB_LFO_DATK_K
#define DUB_LFO_VELSENS  DUB_LFO_DATK_R

static uint8_t lfo_mod_u8(uint8_t base, uint8_t tgt, uint8_t lo, uint8_t hi);

static int8_t bass_transpose_eff(void)
{
    int32_t t = (int32_t)g.cfg.bass.transpose +
                (fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_TRANSP) * BASS_TRANSPOSE_MAX) / 127;

    if (t < -BASS_TRANSPOSE_MAX)
        t = -BASS_TRANSPOSE_MAX;
    if (t > BASS_TRANSPOSE_MAX)
        t = BASS_TRANSPOSE_MAX;
    return (int8_t)t;
}

static uint8_t bass_vel_sens_eff(void)
{
    return lfo_mod_u8(g.cfg.bass.vel_sens, DUB_LFO_VELSENS, 0u, 127u);
}

/* GLIDE knob's LFO dest is ATK_B; the synth reads glide_time, not vp->atk_b. Mod-map GLIDE adds on top. */
static uint8_t bass_glide_eff(void)
{
    return fm_lfo_clamp_u8((int32_t)lfo_mod_u8(g.cfg.bass.glide_time, DUB_LFO_ATK_B, 0u, 127u) +
                               (int32_t)g.bass_rt.mod.glide,
                           0u, 127u);
}

static void dub_note_to_str(uint8_t nn, char *s, uint32_t n);
static void dub_key_toast_show_part(uint8_t ln, const char *part, uint8_t with_voice);
static void bass_edit_lock_step(uint8_t st);
static void bass_note_off_legato(uint8_t note);

static void bass_trig_sub_note(uint8_t note, uint8_t vel, uint8_t accent)
{
    dub_trig_sub(&g.synth, bass_transpose_note(note, bass_transpose_eff()), vel, bass_glide_eff());
    bass_dahdsr_trig(&g.synth.bass_env, &g.synth.bass_env_cfg, vel, accent);
}

static void bass_seq_gate_clear(void)
{
    g.bass_rt.gate_left = 0u;
}

static void bass_seq_gate_arm(uint8_t gate)
{
    if (bass_midi_has_priority(&g.bass_rt.midi))
        return;
    g.bass_rt.gate_left = bass_step_gate_samples(gate, g.samples_per_step);
}

static void bass_seq_gate_close(void)
{
    bass_dahdsr_gate(&g.synth.bass_env, &g.synth.bass_env_cfg, 0u);
    bass_seq_gate_clear();
}

static void bass_seq_gate_sample(void)
{
    if (bass_midi_has_priority(&g.bass_rt.midi))
        return;
    if (!g.bass_rt.gate_left)
        return;
    g.bass_rt.gate_left--;
    if (!g.bass_rt.gate_left)
        bass_dahdsr_gate(&g.synth.bass_env, &g.synth.bass_env_cfg, 0u);
}

static uint8_t bass_play_toast_ok(void)
{
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;

    if (edit || g.console)
        return 0u;
    return 1u;
}

static void dub_key_toast_show_bass_note(uint8_t nn)
{
    char note[8];

    dub_note_to_str(nn, note, sizeof note);
    dub_key_toast_show_part(255u, note, 0u);
}

static uint8_t bass_kbd_to_midi(uint8_t kid)
{
    /* Concert layout: key 0 = F (KEY0), chromatic to G5; OCT± shifts root by 12 → keyboard octave. */
    int32_t oct_shift = ((int32_t)(g.voice.p.root / 12u) - (int32_t)(KEY0 / 12u)) * 12;
    int32_t nn = (int32_t)KEY0 + (int32_t)kid + oct_shift;

    return (uint8_t)fm_lfo_clamp_u8(nn, 0, 127);
}

static void bass_note_off_legato(uint8_t note)
{
    bass_midi_hold_note_off(&g.bass_rt.midi, note);
    if (bass_midi_has_priority(&g.bass_rt.midi))
        return;
    {
        uint8_t nn = bass_midi_priority_note(&g.bass_rt.midi);

        if (!nn) {
            bass_seq_gate_close();
            g.synth.sub_on = 0u;
            g.synth.sub_env = 0;
            return;
        }
        nn = bass_transpose_note(nn, bass_transpose_eff());
        g.synth.sub_note = nn;
        g.synth.sub_on = 1u;
        g.synth.sub_tgt_inc = dub_note_inc(nn);
    }
}

/* Transport / STOP: drop seq+MIDI hold and hard-mute bass VCA (FX tails OK). */
static void bass_transport_stop(void)
{
    bass_midi_hold_init(&g.bass_rt.midi);
    bass_seq_gate_clear();
    bass_dahdsr_gate(&g.synth.bass_env, &g.synth.bass_env_cfg, 0u);
    g.synth.bass_env.level_q15 = 0;
    g.synth.bass_env.hard_pass = 0u;
    bass_dahdsr_init(&g.synth.bass_env);
    g.synth.sub_on = 0u;
    g.synth.sub_env = 0;
    g.synth.bass_slide = 0u;
    g.bass_rt.ratchet_left = 0u;
    g.bass_rt.micro_armed = 0u;
    g.voice.lane_gate[4] = 0;
}

static void bass_handle_chromatic_keys(uint32_t rising, uint32_t released)
{
    uint8_t kid;

    for (kid = 0u; kid < 27u; kid++) {
        uint32_t bit = 1u << kid;

        if (rising & bit) {
            uint8_t nn = bass_kbd_to_midi(kid);
            uint8_t vel = bass_live_vel(BASS_KEY_VEL, bass_vel_sens_eff());

            bass_midi_hold_note_on(&g.bass_rt.midi, nn, vel);
            g.bass_rt.ratchet_left = 0u;
            bass_trig_sub_note(nn, vel, 0u);
            if (g.rec_arm) {
                uint8_t stp = bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages,
                                                           g.step);

                g.cfg.seq[stp].note = nn;
                g.cfg.seq[stp].gate |= (uint8_t)(1u << 4);
                g.cfg.bass.step_x[stp].gate = 4u;
                bass_edit_lock_step(stp);
            }
            if (bass_play_toast_ok())
                dub_key_toast_show_bass_note(nn);
            mark_dirty();
        }
        if (released & bit)
            bass_note_off_legato(bass_kbd_to_midi(kid));
    }
}

static void bass_edit_lock_step(uint8_t st)
{
    if (st < DUB_SEQ_STEPS)
        bass_step_x_edit_locks_step(&g.cfg.bass.step_x[st]);
}

static void bass_seq_sync_genre_pattern(void)
{
    uint32_t n = fm_seq_len(g.cfg.seq_pages);
    uint32_t i;

    if (g.cfg.user_song)
        return;
    if (n > DUB_SEQ_STEPS)
        n = DUB_SEQ_STEPS;
    bass_genre_fader_bake(g.cfg.seq, g.cfg.bass.step_x, g.cfg.bass.genre_fader, g.cfg.p.era, (uint8_t)n);
    for (i = 0; i < n; i++)
        g.cfg.seq[i].force = 1u;
}

/* Plate load: the genre fader lines are the plates of the live plate's block, line k = the plate itself; park the
 * fader there so the bake plays the new plate (a fixed fader only tracks the plate's offset to its landmark). */
static void bass_fader_to_plate(void)
{
    uint8_t k = g.cfg.p.plate;

#if defined(FM_PGEN_ON)
    if (genre_pgen_slot() != 0xFFu)
        k = genre_pgen_slot();
#endif
    g.cfg.bass.genre_fader = bass_genre_fader_of_line((uint8_t)(k % GENRE_PRESETS_PER_ERA));
}

static void bass_select_fader_step(int32_t steps)
{
    int32_t v = (int32_t)g.cfg.bass.genre_fader + steps;

    if (v < 0)
        v = 0;
    if (v > 127)
        v = 127;
    g.cfg.bass.genre_fader = (uint8_t)v;
    bass_seq_sync_genre_pattern();
    seq_flash_knob(bass_genre_fader_name(g.cfg.bass.genre_fader, g.cfg.p.era));
    mark_dirty();
}

static void bass_sync_voice_mod(dub_params_t *vp)
{
    uint8_t macro_raw[BASS_MOD_MACRO_N];
    int8_t lfo_macro[BASS_MOD_MACRO_N];
    uint8_t mi;
    bass_dahdsr_cfg_t env_eff = g.cfg.bass.env;
    /* Last block's effective cfg: the env map must not feed back into its own INVERT this block. */
    int32_t env_mod_q15 = bass_dahdsr_mod_q15(&g.synth.bass_env, &g.synth.bass_env_cfg);

    g.synth.bass_env_cfg = g.cfg.bass.env;
    /* MAC 1..3 = macros 0..2 with LFO dests M_CHORD/M_FEED/M_WOB; AUX has no knob (macro 3 = p2 RATCH). */
    for (mi = 0u; mi < BASS_MAC_AUX; mi++) {
        uint8_t *mp = dub_macro_ptr(mi);

        macro_raw[mi] = mp ? *mp : 64u;
        lfo_macro[mi] = bass_mod_clamp_i8(
            fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, (uint8_t)(DUB_LFO_M_CHORD + mi)));
    }
    macro_raw[BASS_MAC_AUX] = 64u;
    lfo_macro[BASS_MAC_AUX] = 0;
    bass_mod_apply_two_pass(&g.cfg.bass.mods, macro_raw, lfo_macro, env_mod_q15, &g.bass_rt.mod);
    bass_apply_mod_to_voice(vp, &env_eff, &g.bass_rt.mod);
    env_eff.t.delay = lfo_mod_u8(env_eff.t.delay, DUB_LFO_ENV_DLY, 0u, 127u);
    env_eff.t.attack = lfo_mod_u8(env_eff.t.attack, DUB_LFO_ENV_ATK, 0u, 127u);
    env_eff.t.hold = lfo_mod_u8(env_eff.t.hold, DUB_LFO_ENV_HOLD, 0u, 127u);
    env_eff.t.decay = lfo_mod_u8(env_eff.t.decay, DUB_LFO_ENV_DEC, 0u, 127u);
    env_eff.t.sustain = lfo_mod_u8(env_eff.t.sustain, DUB_LFO_ENV_SUS, 0u, 127u);
    env_eff.t.release = lfo_mod_u8(env_eff.t.release, DUB_LFO_ENV_REL, 0u, 127u);
    env_eff.pol.accent_scale = lfo_mod_u8(env_eff.pol.accent_scale, DUB_LFO_ENV_ACC, 0u, 127u);
    g.synth.bass_env_cfg = env_eff;
    g.synth.bass_amp_q15 = g.bass_rt.mod.amp_q15;
}

static void bass_midi_dispatch(uint8_t st, uint8_t d1, uint8_t d2)
{
    if (!bass_midi_ch_match(g.cfg.bass.midi_ch, st))
        return;
    if ((st & 0xF0u) == 0x90u && d2) {
        uint8_t stp = g.step % DUB_SEQ_STEPS;
        uint8_t vel = bass_live_vel(d2, bass_vel_sens_eff());

        bass_midi_hold_note_on(&g.bass_rt.midi, d1, vel);
        bass_trig_sub_note(d1, vel, 0u);
        if (d2 && bass_play_toast_ok())
            dub_key_toast_show_bass_note(d1);
        if (g.rec_arm) {
            stp = bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages, g.step);
            g.cfg.seq[stp].note = d1;
            g.cfg.seq[stp].gate |= (uint8_t)(1u << 4);
            g.cfg.bass.step_x[stp].gate = 4u;
            bass_edit_lock_step(stp);
            mark_dirty();
        }
    } else if (((st & 0xF0u) == 0x80u) || ((st & 0xF0u) == 0x90u && !d2)) {
        bass_note_off_legato(d1);
    }
}

/* Glide time code for the synth: τ = 60 ms · 2^((tc − 56) / 32); GLIDE 24 + SLIDE 66 = tc 56 (303 default).
 * slide_lv 0 (tie legato) / 2 = GLIDE time; 1 (33) half, 3 (100) double. */
static void bass_arm_glide(uint8_t note, uint8_t slide_on, uint8_t slide_lv)
{
    int32_t tc = (int32_t)bass_glide_eff() + 32;

    if (slide_lv)
        tc += ((int32_t)bass_step_lv_clamp(slide_lv) - 2) * 32;
    if (tc < 0)
        tc = 0;
    if (tc > 184)
        tc = 184;
    g.synth.sub_note = bass_transpose_note(note, bass_transpose_eff());
    g.synth.sub_on = 1u;
    g.synth.sub_tgt_inc = dub_note_inc(g.synth.sub_note);
    g.synth.bass_glide_tc = (uint8_t)tc;
    g.synth.bass_slide = slide_on ? 1u : 0u;
    if (!g.synth.sub_inc)
        g.synth.sub_inc = g.synth.sub_tgt_inc;
    bass_midi_hold_set_seq_note(&g.bass_rt.midi, note);
}

/* HOME p2 PERFORM knobs live on otherwise unused BASS macro slots (era-reset to 64 = off).
 * Macro 3 doubles as the FILL build target (→ 127 = ratchet roll). */
enum { BASS_PERF_GATE = 0, BASS_PERF_DENS, BASS_PERF_RATCH, BASS_PERF_SHIFT, BASS_PERF_N };
static const uint8_t BASS_PERF_MACRO_IX[BASS_PERF_N] = {4u, 6u, 3u, 7u};

static uint8_t *bass_perf_ptr(uint8_t k)
{
    return dub_macro_ptr(BASS_PERF_MACRO_IX[k % BASS_PERF_N]);
}

#if defined(GENRE_BASS)
/* HOME MAC 1..3 as heard: stored knob + the macro LFO, same delta as bass_sync_voice_mod(). Display only. */
static uint8_t bass_mac_eff_raw(uint8_t mi)
{
    uint8_t *mp = dub_macro_ptr(mi);
    int32_t e = mp ? (int32_t)*mp : 64;

    e += bass_mod_clamp_i8(fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, (uint8_t)(DUB_LFO_M_CHORD + mi)));
    return (uint8_t)fm_lfo_clamp_u8(e, 0, 127);
}
#endif

/* HOME p2 PERFORM knob as heard: stored value + its macro LFO (LFO target = BASS_PERF_MACRO_IX). */
static uint8_t bass_perf_eff_raw(uint8_t k)
{
    uint8_t *mp = bass_perf_ptr(k);
    int32_t e = mp ? (int32_t)*mp : 64;

    e += bass_mod_clamp_i8(
        fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, (uint8_t)(DUB_LFO_M_CHORD + BASS_PERF_MACRO_IX[k % BASS_PERF_N])));
    return (uint8_t)fm_lfo_clamp_u8(e, 0, 127);
}

/* Knob offset (raw − 64) + mod-map SEQ dest, −64..63. */
static int32_t bass_perf_amt(uint8_t k)
{
    const bass_mod_runtime_t *rt = &g.bass_rt.mod;
    int32_t c = (int32_t)bass_perf_eff_raw(k) - 64;

    c += k == BASS_PERF_GATE ? rt->seq_gate : k == BASS_PERF_DENS ? rt->seq_density :
         k == BASS_PERF_RATCH ? rt->seq_ratchet : rt->seq_rotate;
    if (c < -64)
        c = -64;
    if (c > 63)
        c = 63;
    return c;
}

/* Stable per-step rank 0..63: turning DENS/RATCH further always adds/drops the same steps first. */
static uint8_t bass_perf_rank(uint8_t st, uint32_t salt)
{
    uint32_t h = ((uint32_t)st + 1u) * 2654435761u ^ (salt * 0x85EBCA6Bu);

    h ^= h >> 13;
    h *= 0xC2B2AE35u;
    h ^= h >> 16;
    return (uint8_t)(h & 63u);
}

static uint8_t bass_perf_gate(uint8_t gate)
{
    int32_t gr;

    if (!gate)
        return 0u;
    gr = (int32_t)gate + bass_perf_amt(BASS_PERF_GATE) / 12;
    if (gr < 1)
        gr = 1;
    if (gr > (int32_t)BASS_GATE_TIE)
        gr = (int32_t)BASS_GATE_TIE;
    return (uint8_t)gr;
}

static uint8_t bass_perf_ratchet(uint8_t st, uint8_t rat)
{
    int32_t c = bass_perf_amt(BASS_PERF_RATCH);
    uint8_t rk = bass_perf_rank(st, 0x52u);

    if (c > 0 && (int32_t)rk < c) {
        uint8_t want = (uint8_t)(c >= 56 ? 4u : c >= 40 ? 3u : 2u);

        if (rat < want)
            rat = want;
    } else if (c < 0 && (int32_t)rk < -c)
        rat = 1u;
    return rat;
}

static uint8_t bass_perf_shift_step(uint8_t st, uint8_t len)
{
    int32_t r = bass_perf_amt(BASS_PERF_SHIFT) / 8;

    if (!len || st >= len || r == 0)
        return st;
    r %= (int32_t)len;
    return (uint8_t)(((int32_t)st - r + (int32_t)len) % (int32_t)len);
}

/* DENS < 64 thins programmed steps (bar downbeat kept, accents until −48); > 64 fills rests. */
static uint8_t bass_perf_dens_drop(uint8_t st)
{
    int32_t c = bass_perf_amt(BASS_PERF_DENS);

    if (c >= 0 || (st % 16u) == 0u)
        return 0u;
    if (g.cfg.bass.step_x[st % DUB_SEQ_STEPS].accent && c > -48)
        return 0u;
    return (int32_t)bass_perf_rank(st, 0x44u) < -c ? 1u : 0u;
}

static uint8_t bass_perf_dens_fill(uint8_t st)
{
    int32_t c = bass_perf_amt(BASS_PERF_DENS);

    return c > 0 && (int32_t)bass_perf_rank(st, 0x46u) < c ? 1u : 0u;
}

static uint8_t bass_fire_note(uint8_t st, int32_t base_note, uint8_t vel, uint8_t accent)
{
    const bass_step_x_t *sx = &g.cfg.bass.step_x[st % DUB_SEQ_STEPS];
    uint8_t lh = g.cfg.bass.live_hold;
    uint8_t gate = bass_perf_gate(bass_step_effective_gate(sx, lh, g.cfg.bass.live_gate));
    uint8_t slide = bass_step_effective_slide(sx, lh, g.cfg.bass.live_slide);
    uint8_t rat = bass_perf_ratchet((uint8_t)(st % DUB_SEQ_STEPS),
                                    bass_step_effective_ratchet(sx, lh, g.cfg.bass.live_ratchet));
    uint8_t acc = bass_step_effective_accent(sx, lh, g.cfg.bass.live_accent);
    uint8_t mute = bass_step_effective_mute(lh, g.cfg.bass.live_mute);
    int32_t nn;
    uint8_t sounding;
    uint8_t legato;
    uint8_t hit_acc;

    if (bass_midi_has_priority(&g.bass_rt.midi))
        return 0u;
    /* VARY-added hit on an empty step: baked gate is 0, play it as a full step. */
    if (gate == 0u && !(lh & BASS_LIVE_HOLD_GATE) && !bass_seq_lane_base_on((uint8_t)(st % DUB_SEQ_STEPS)) &&
        bass_seq_vary_on(0, (uint8_t)(st % DUB_SEQ_STEPS)))
        gate = bass_perf_gate(4u);
    {
        int32_t gr = (int32_t)gate + fm_seq_rand_gate(dub_eff_gate_rng(4u), st, g.bar, 4u, g.seq_rng);

        if (gr < 0)
            gr = 0;
        if (gr > (int32_t)BASS_GATE_TIE)
            gr = (int32_t)BASS_GATE_TIE;
        gate = (uint8_t)gr;
    }
    if (mute || gate == 0u) {
        g.bass_rt.ratchet_left = 0u;
        g.synth.bass_slide = 0u;
        bass_seq_gate_close();
        return 0u;
    }
    nn = base_note;
    if (nn < 0)
        nn = 0;
    if (nn > 127)
        nn = 127;
    hit_acc = accent ? 127u : (uint8_t)((127u * (uint32_t)acc) / BASS_STEP_LV_FULL);
    sounding = g.synth.sub_on && g.synth.bass_env.stage != BASS_ENV_IDLE &&
               g.synth.bass_env.stage != BASS_ENV_RELEASE;
    legato = (slide || bass_step_gate_is_tie(gate)) && sounding;
    if (legato) {
        bass_arm_glide((uint8_t)nn, 1u, slide);
        if (hit_acc)
            bass_dahdsr_trig(&g.synth.bass_env, &g.synth.bass_env_cfg, vel, hit_acc);
        else
            bass_dahdsr_gate(&g.synth.bass_env, &g.synth.bass_env_cfg, 1u);
    } else {
        g.synth.bass_slide = 0u;
        bass_trig_sub_note((uint8_t)nn, vel, hit_acc);
        g.synth.sub_inc = g.synth.sub_tgt_inc;
        bass_midi_hold_set_seq_note(&g.bass_rt.midi, (uint8_t)nn);
    }
    if (rat > 1u && bass_step_ratchet_allowed(slide, gate)) {
        uint32_t every = g.samples_per_step / (uint32_t)rat;

        if (every < 16u)
            every = 16u;
        if (every > 65535u)
            every = 65535u;
        g.bass_rt.ratchet_left = (uint8_t)(rat - 1u);
        g.bass_rt.ratchet_every = (uint16_t)every;
        g.bass_rt.ratchet_acc = 0u;
        g.bass_rt.ratchet_note = (uint8_t)nn;
        g.bass_rt.ratchet_vel = vel;
        g.bass_rt.ratchet_accent = hit_acc;
    } else
        g.bass_rt.ratchet_left = 0u;
    bass_seq_gate_arm(gate);
    return 1u;
}

static void bass_ratchet_sample(void)
{
    if (!g.playing || !g.bass_rt.ratchet_left || !g.bass_rt.ratchet_every)
        return;
    if (bass_midi_has_priority(&g.bass_rt.midi)) {
        g.bass_rt.ratchet_left = 0u;
        return;
    }
    g.bass_rt.ratchet_acc++;
    if (g.bass_rt.ratchet_acc < g.bass_rt.ratchet_every)
        return;
    g.bass_rt.ratchet_acc = 0u;
    g.bass_rt.ratchet_left--;
    g.synth.bass_slide = 0u;
    bass_trig_sub_note(g.bass_rt.ratchet_note, g.bass_rt.ratchet_vel, g.bass_rt.ratchet_accent);
    g.synth.sub_inc = g.synth.sub_tgt_inc;
    {
        uint8_t st = (uint8_t)(g.step % DUB_SEQ_STEPS);
        const bass_step_x_t *sx = &g.cfg.bass.step_x[st];
        uint8_t lh = g.cfg.bass.live_hold;
        uint8_t gat = bass_perf_gate(bass_step_effective_gate(sx, lh, g.cfg.bass.live_gate));

        bass_seq_gate_arm(gat);
    }
}

/* Late-only: early micro would need a lookahead on the next step's resolved note. */
static void bass_micro_sample(void)
{
    if (!g.bass_rt.micro_armed)
        return;
    if (!g.playing) {
        g.bass_rt.micro_armed = 0u;
        return;
    }
    if (g.bass_rt.micro_left > 0u) {
        g.bass_rt.micro_left--;
        return;
    }
    g.bass_rt.micro_armed = 0u;
    bass_fire_note(g.bass_rt.micro_st, g.bass_rt.micro_note, g.bass_rt.micro_vel, g.bass_rt.micro_accent);
}

static int bass_seq_lane_step_on(uint8_t st);
#endif

static void user_song_clear(void);
static int fm_era_is_artist(uint8_t era);
static uint8_t fm_artist_plate_count(void);
static uint8_t fm_artist_first_plate(void);
static uint8_t fm_artist_cycle_plate(uint8_t cur, int32_t dir);
static void cfg_seq_generators_clear(void);
static void seq_lane_wipe_plocks(uint8_t ln);
static void seq_new_song_clear_remnants(void);
static void nav_view(uint8_t view);

_Static_assert(DUB_SEQ_STEPS == FM_SEQ_STEPS, "dub/fm_seq step count");
_Static_assert(sizeof(dub_cfg_t) <= SETTINGS_MAX, "dub cfg fits the settings store");

static uint8_t seq_pages(void)
{
    return fm_seq_pages_clamp(g.cfg.seq_pages);
}

static uint32_t seq_len(void)
{
    return fm_seq_len(g.cfg.seq_pages);
}

static uint32_t seq_view_base(void)
{
    return (uint32_t)(g.seq_view_page % FM_SEQ_MAX_PAGES) * FM_SEQ_PAGE_STEPS;
}

static uint8_t seq_lane_sel(void)
{
    return (uint8_t)(g.seq_lane_sel % (DUB_SEQ_LANE_ALL + 1u));
}

static void seq_lane_sel_set(uint8_t v)
{
    g.seq_lane_sel = (uint8_t)(v % (DUB_SEQ_LANE_ALL + 1u));
    if (g.seq_lane_sel < DUB_SEQ_LANES)
        g.seq_lane = g.seq_lane_sel;
}

static void seq_lane_sel_knob(int32_t steps, int edit)
{
    if (edit)
        seq_lane_sel_set(steps < 0 ? 0u : DUB_SEQ_LANE_ALL);
    else
        seq_lane_sel_set((uint8_t)((seq_lane_sel() + (steps > 0 ? 1u : DUB_SEQ_LANE_ALL)) %
                                   (DUB_SEQ_LANE_ALL + 1u)));
}

static int seq_lane_is_all(void)
{
    return seq_lane_sel() == DUB_SEQ_LANE_ALL;
}

static uint8_t seq_lane_len_v(uint8_t v, uint8_t ln)
{
    (void)v;
    return fm_seq_lane_len_clamp(g.seq_lane_len[0][ln % DUB_SEQ_LANES]);
}

static void seq_lane_pattern_mark_edited(uint8_t ln);

static void seq_lane_len_put(uint8_t ln, uint8_t val)
{
    uint32_t i;
    uint8_t old;

    ln %= DUB_SEQ_LANES;
    old = seq_lane_len_v(0u, ln);
    val = fm_seq_lane_len_clamp(val);
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
        g.seq_lane_len[i][ln] = val;
    if (val != old)
        seq_lane_pattern_mark_edited(ln);
}

static int8_t seq_lane_nudge_base(uint8_t v, uint8_t ln)
{
    (void)v;
    return fm_seq_lane_nudge_clamp(g.seq_lane_nudge[ln % DUB_SEQ_LANES]);
}

static int8_t dub_eff_lane_nudge(uint8_t v, uint8_t ln)
{
#if defined(GENRE_BASS)
    (void)v;
    (void)ln;
    return 0;
#else
    int32_t n = (int32_t)seq_lane_nudge_base(v, ln);

    if (g.seq_lfo_lane[7] == ln && g.lfo_slots[DUB_LFO_SEQ_NUDGE].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_SEQ_NUDGE);

        n += (d * FM_SEQ_TIMING_UNITS) / 127;
    }
    return fm_seq_lane_nudge_clamp(n);
#endif
}

static int8_t seq_lane_nudge_v(uint8_t v, uint8_t ln)
{
    return dub_eff_lane_nudge(v, ln);
}

static int8_t dub_eff_step_micro(uint8_t ln, uint8_t st)
{
    int32_t m = (int32_t)g.step_micro[ln][st];

    if (g.seq_lfo_lane[8] == ln && g.lfo_slots[DUB_LFO_SEQ_MICRO].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_SEQ_MICRO);

        m += (d * FM_SEQ_MICRO_HALF) / 127;
    }
    return fm_seq_micro_clamp(m);
}

static int32_t seq_lane_timing_units(uint8_t ln, uint8_t grid_st)
{
    uint8_t vp = (uint8_t)((grid_st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
    int32_t sum;

    sum = (int32_t)dub_eff_step_micro(ln, grid_st) + (int32_t)dub_eff_lane_nudge(vp, ln);
    return fm_seq_timing_sum_clamp(sum);
}

static uint8_t seq_lane_step_map(uint8_t st, uint8_t ln)
{
    uint8_t v = (uint8_t)((st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);

    return (uint8_t)fm_seq_lane_map_step((uint32_t)st, seq_lane_len_v(v, ln));
}

/* DJ overlay on SEQ pg4 CYCLE/REPEAT: delta applied to base; base is never written. */
static uint8_t dj_seq_eff_cyc(uint8_t v)
{
    int32_t d = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_SEQ_CYC);

    return fm_dj_clamp_u8((int32_t)g.seq_view_cyc[v] + (d * (int32_t)(FM_SEQ_NVIEW_CYC - 1u)) / 127,
                          0, (int32_t)(FM_SEQ_NVIEW_CYC - 1u));
}

static uint8_t dj_seq_eff_reps(uint8_t v)
{
    int32_t d = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_SEQ_REP);

    return fm_dj_clamp_u8((int32_t)g.seq_view_reps[v] + (d * 15) / 127, 1, 16);
}

static int seq_view_chain_on(uint8_t v)
{
    if (g.seq_view_on[v])
        return 1;
    {
        uint32_t i;

        for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
            if (g.seq_view_on[i])
                return 0;
    }
    return v == 0u;
}

static int seq_view_playable(uint8_t v)
{
    if (!seq_view_chain_on(v))
        return 0;
    return fm_seq_view_cyc_pass(dj_seq_eff_cyc(v), g.bar);
}

static int seq_grid_step_audible(uint8_t grid_st)
{
    uint8_t mv = (uint8_t)((grid_st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);

    return seq_view_playable(mv);
}

static void seq_view_rep_arm(uint8_t v)
{
    uint8_t r = dj_seq_eff_reps(v);

    if (r < 1u)
        r = 1u;
    if (r > 16u)
        r = 16u;
    g.seq_view_rep_left = r;
}

static uint8_t seq_view_next_playable(uint8_t from, uint8_t after)
{
    uint32_t i;

    for (i = 1u; i <= FM_SEQ_MAX_PAGES; i++) {
        uint8_t w = (uint8_t)((from + i) % FM_SEQ_MAX_PAGES);

        if (after && w == from)
            continue;
        if (seq_view_playable(w))
            return w;
    }
    if (seq_view_playable(from))
        return from;
    return 255u;
}

static uint8_t seq_view_on_count(void)
{
    uint32_t i, n = 0u;

    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
        if (g.seq_view_on[i])
            n++;
    return (uint8_t)n;
}

static void dub_key_toast_show_part(uint8_t ln, const char *part, uint8_t with_voice);

static void seq_view_cyc_sync_on(uint8_t v)
{
    g.seq_view_on[v] = g.seq_view_cyc[v] != FM_SEQ_VIEW_CYC_OFF ? 1u : 0u;
}

/* SONG mode: the arrangement is stored in g.cfg.seq_cyc / seq_reps. Mirror the runtime copy there (the ONLY
 * latch stays runtime-only, so the pre-ONLY values are stored, same as the save path). No-op in preset mode. */
static void seq_arr_song_mirror(void)
{
    if (!g.cfg.user_song)
        return;
    if (g.seq_view_only_page < FM_SEQ_MAX_PAGES)
        memcpy(g.cfg.seq_cyc, g.seq_view_cyc_saved, sizeof g.cfg.seq_cyc);
    else
        memcpy(g.cfg.seq_cyc, g.seq_view_cyc, sizeof g.cfg.seq_cyc);
    memcpy(g.cfg.seq_reps, g.seq_view_reps, sizeof g.cfg.seq_reps);
}

static int seq_view_cyc_would_block_off(uint8_t v)
{
    if (g.seq_view_cyc[v] == FM_SEQ_VIEW_CYC_OFF)
        return 0;
    return seq_view_on_count() <= 1u;
}

static void seq_view_cyc_edit(uint8_t v, int32_t steps, int edit)
{
    uint8_t prev = g.seq_view_cyc[v];
    uint32_t i;

    if (edit && steps != 0) {
        if (steps < 0 && seq_view_cyc_would_block_off(v)) {
            dub_key_toast_show_part(0u, "LAST VIEW", 0u);
            return;
        }
        g.seq_view_cyc[v] = steps < 0 ? 0u : (uint8_t)(FM_SEQ_NVIEW_CYC - 1u);
    } else if (!edit && steps != 0) {
        int32_t ix = (int32_t)g.seq_view_cyc[v] + (steps > 0 ? 1 : -1);

        if (ix < 0)
            ix = 0;
        if (ix >= (int32_t)FM_SEQ_NVIEW_CYC)
            ix = (int32_t)FM_SEQ_NVIEW_CYC - 1;
        if (ix == (int32_t)FM_SEQ_VIEW_CYC_OFF && seq_view_cyc_would_block_off(v)) {
            dub_key_toast_show_part(0u, "LAST VIEW", 0u);
            return;
        }
        g.seq_view_cyc[v] = (uint8_t)ix;
    }
    seq_view_cyc_sync_on(v);

    if (prev == FM_SEQ_VIEW_CYC_ONLY && g.seq_view_cyc[v] != FM_SEQ_VIEW_CYC_ONLY) {
        if (g.seq_view_only_page == v) {
            for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
                if (i == (uint32_t)v)
                    continue;
                g.seq_view_cyc[i] = g.seq_view_cyc_saved[i];
                seq_view_cyc_sync_on((uint8_t)i);
            }
            g.seq_view_only_page = FM_SEQ_MAX_PAGES;
        }
    } else if (g.seq_view_cyc[v] == FM_SEQ_VIEW_CYC_ONLY) {
        if (g.seq_view_only_page < FM_SEQ_MAX_PAGES && g.seq_view_only_page != v) {
            uint8_t old = g.seq_view_only_page;

            g.seq_view_cyc[old] = g.seq_view_cyc_saved[old];
            seq_view_cyc_sync_on(old);
            g.seq_view_only_page = FM_SEQ_MAX_PAGES;
        }
        if (g.seq_view_only_page >= FM_SEQ_MAX_PAGES) {
            g.seq_view_cyc_saved[v] = prev;
            g.seq_view_only_page = v;
            for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
                if (i == (uint32_t)v)
                    continue;
                g.seq_view_cyc_saved[i] = g.seq_view_cyc[i];
                g.seq_view_cyc[i] = FM_SEQ_VIEW_CYC_OFF;
                seq_view_cyc_sync_on((uint8_t)i);
            }
        }
    }
    seq_arr_song_mirror();
}

static void seq_view_reps_edit(uint8_t v, int32_t steps, int edit, int d)
{
    int32_t nv;

    if (edit && steps != 0)
        g.seq_view_reps[v] = steps < 0 ? 1u : 16u;
    else {
        nv = (int32_t)g.seq_view_reps[v] + d;
        if (nv < 1)
            nv = 1;
        if (nv > 16)
            nv = 16;
        g.seq_view_reps[v] = (uint8_t)nv;
    }
    seq_arr_song_mirror();
}

static void seq_lane_len_edit(uint8_t v, uint8_t ln, int32_t steps, int edit, int d)
{
    (void)v;
    if (seq_lane_is_all()) {
        uint32_t i;
        int32_t delta = edit && steps != 0 ? (steps < 0 ? -1 : 1) : d;

        for (i = 0u; i < DUB_SEQ_LANES; i++) {
            int32_t nv = (int32_t)seq_lane_len_v(0u, (uint8_t)i) + delta;

            seq_lane_len_put((uint8_t)i, (uint8_t)nv);
        }
    } else {
        ln %= DUB_SEQ_LANES;
        if (edit && steps != 0)
            seq_lane_len_put(ln, steps < 0 ? 1u : (uint8_t)FM_SEQ_STEPS);
        else {
            int32_t nv = (int32_t)seq_lane_len_v(0u, ln) + d;

            seq_lane_len_put(ln, (uint8_t)nv);
        }
    }
}

static void seq_lane_nudge_edit(uint8_t v, uint8_t ln, int32_t steps, int edit, int d)
{
    (void)v;
    if (seq_lane_is_all()) {
        uint32_t i;
        int32_t delta = edit && steps != 0 ? (steps < 0 ? -1 : 1) : (steps != 0 ? steps : d);

        for (i = 0u; i < DUB_SEQ_LANES; i++) {
            int8_t old = g.seq_lane_nudge[i];
            int32_t nv = (int32_t)old + delta;

            g.seq_lane_nudge[i] = fm_seq_lane_nudge_clamp(nv);
            if (g.seq_lane_nudge[i] != old)
                seq_lane_pattern_mark_edited((uint8_t)i);
        }
    } else {
        int8_t old;

        ln %= DUB_SEQ_LANES;
        old = g.seq_lane_nudge[ln];
        if (edit && steps != 0)
            g.seq_lane_nudge[ln] = steps < 0 ? (int8_t)FM_SEQ_LANE_NUDGE_MIN :
                                      (int8_t)FM_SEQ_LANE_NUDGE_MAX;
        else {
            int32_t nv = (int32_t)g.seq_lane_nudge[ln] + (steps != 0 ? steps : d);

            g.seq_lane_nudge[ln] = fm_seq_lane_nudge_clamp(nv);
        }
        if (g.seq_lane_nudge[ln] != old)
            seq_lane_pattern_mark_edited(ln);
    }
}

static const uint8_t KEY_BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static const uint8_t KEY_WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const char *const BASS_NAMES[BASS_N] = {
    "SINE", "808", "REESE", "GROWL", "PLUCK", "303", "SOLID", "PULSE", "WOB", "SUB"
};
#endif

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const uint8_t BASS_ERA[BASS_N] = {
    ERA_KINGSTON, ERA_KINGSTON, ERA_BRISTOL, ERA_CROYDON,
    ERA_POSTDUB,  ERA_CHANNEL,  ERA_CHANNEL, ERA_BRISTOL,
    ERA_CROYDON,  ERA_POSTDUB
};
#endif


static void dub_rng_step(void)
{
    g.seq_rng = g.seq_rng * 1664525u + 1013904223u;
}

static uint32_t dub_boot_seed(void)
{
    uint32_t s = fm1_ms ^ 0x9E3779B9u;

#if !defined(DINGHY_HOST) && !defined(DINGHY_WEB)
    s ^= fm1_ticks();
    s ^= ((uint32_t)fm1_boot.p3_rst << 8) ^ ((uint32_t)fm1_boot.rst_src << 16);
    s ^= (uint32_t)bootguard.pending << 24;
    {
        uint32_t i;

        /* fm1_ms is still 0 here: ADC LSB noise + conversion jitter vs TIMER4 carry the entropy. */
        for (i = 0u; i < 16u; i++) {
            int32_t adc = fm1_adc_read((i & 1u) ? FM1_ADC_BATT : FM1_ADC_MASTER);

            s = s * 1664525u + 1013904223u;
            s ^= (uint32_t)adc * 2654435761u;
            s ^= fm1_ticks();
        }
    }
#else
    s ^= (uint32_t)(uintptr_t)&g;
#endif
    return s ? s : 0xC001D00Du;
}

static uint8_t dub_bass_model_for_era(uint8_t era)
{
    uint8_t i;
    uint8_t n = 0u;
    uint8_t pick;
    uint8_t k = 0u;

    era = genre_clamp_era(era);
    for (i = 0u; i < BASS_N; i++) {
        if (BASS_ERA[i] == era)
            n++;
    }
    if (!n)
        return 0u;
    dub_rng_step();
    pick = (uint8_t)(((g.seq_rng >> 16) & 0xffffu) % (uint32_t)n);
    for (i = 0u; i < BASS_N; i++) {
        if (BASS_ERA[i] == era) {
            if (k == pick)
                return i;
            k++;
        }
    }
    return 0u;
}

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const char *const DKIT_NAMES[DKIT_N] = {"YARD", "ARK", "TR808", "TR909", "SYNDRM", "CR78", "DMZ", "ANVIL"};
static const uint8_t DKIT_ERA[DKIT_N] = {
    ERA_KINGSTON, ERA_KINGSTON, ERA_BRISTOL, ERA_CHANNEL,
    ERA_CHANNEL,  ERA_BRISTOL,  ERA_CROYDON, ERA_POSTDUB
};
#endif

static uint8_t dub_drum_kit_for_era(uint8_t era)
{
    uint8_t i;
    uint8_t n = 0u;
    uint8_t pick;
    uint8_t k = 0u;

    era = genre_clamp_era(era);
    for (i = 0u; i < DKIT_N; i++) {
        if (DKIT_ERA[i] == era)
            n++;
    }
    if (!n)
        return 0u;
    dub_rng_step();
    pick = (uint8_t)(((g.seq_rng >> 16) & 0xffffu) % (uint32_t)n);
    for (i = 0u; i < DKIT_N; i++) {
        if (DKIT_ERA[i] == era) {
            if (k == pick)
                return i;
            k++;
        }
    }
    return 0u;
}

/* sync_era: 1 = era follows plate (right turn / absolute); 0 = keep caller's era (left turn). */
static void genre_plate_preset_apply(dub_params_t *p, uint8_t era, uint8_t plate, uint8_t keep_bass,
                                    uint8_t keep_drum_kit, uint8_t sync_era)
{
    plate = genre_clamp_preset(plate);
    if (fm_era_is_artist(p->era) || fm_era_is_artist(era)) {
        era = (uint8_t)FM_ERA_ARTIST;
        p->era = era;
        if (plate < GENRE_CFG_USER_PLATE_N && (g.user_plate_mask & (1ull << plate))) {
            *p = g.user_plates[plate];
            p->era = era;
            p->plate = plate;
        }
#ifndef GENRE_HIPHOP
        p->bass_model = keep_bass;
        p->drum_kit = keep_drum_kit;
#else
        (void)keep_bass;
        (void)keep_drum_kit;
#endif
        return;
    }
    if (sync_era)
        era = genre_era_of_preset(plate);
    era = genre_clamp_era(era);
    if (plate < GENRE_CFG_USER_PLATE_N && (g.user_plate_mask & (1ull << plate))) {
        *p = g.user_plates[plate];
        p->era = era;
        p->plate = plate;
    } else {
        genre_preset_rebuild(p, era, plate);
    }
#ifndef GENRE_HIPHOP
    p->bass_model = keep_bass;
    p->drum_kit = keep_drum_kit;
#else
    (void)keep_bass;
    (void)keep_drum_kit;
#endif
}

#if defined(FM_PGEN_ON)
static uint8_t fm_pgen_comply_eff(void)
{
    int32_t c = (int32_t)g.cfg.pgen.comply;

    if (!g.dj.on)
        g.dj_comply_ofs = 0;
    c += g.dj_comply_ofs;
    return (uint8_t)(c < 0 ? 0 : (c > 127 ? 127 : c));
}

/* Move the live generated plate to (era, n); returns the ROM slot it shadows. */
static uint8_t fm_pgen_goto(uint8_t era, int32_t n)
{
    g.cfg.pgen.n = n;
    genre_pgen_build(genre_clamp_era(era), n, fm_pgen_comply_eff());
    return genre_pgen_slot();
}
#endif
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const char *const VOICE_NAMES[VOICE_N] = {"MIN7", "MIN9", "SUS4", "QUART", "FIFTH"};
#endif
static const char *const TAP_NAMES[TAP_N] = {"SINGLE", "DUAL", "GALLOP", "TRIP", "SPREAD", "PING"};
#ifdef GENRE_HEAVY_PSYCH
static const char *const SEQ_LANE[6] = {"KICK", "SNARE", "HAT", "LEAD", "BASS", "CHORD"};
#elif defined(GENRE_HIPHOP)
static const char *const SEQ_LANE[6] = {"KICK", "SNARE", "HAT", "SCRATCH", "BASS", "CHORD"};
#elif defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO)
static const char *const SEQ_LANE[6] = {"KICK", "SNARE", "HAT", "STAB", "BASS", "CHORD"};
#elif defined(GENRE_BASS)
static const char *const SEQ_LANE[6] = {"BASS", "BASS", "BASS", "BASS", "BASS", "BASS"};
#elif defined(GENRE_JAZZ)
static const char *const SEQ_LANE[6] = {"KICK", "RIM", "RIDE", "HORN", "BASS", "CHORD"};
#elif defined(GENRE_SOUL)
static const char *const SEQ_LANE[6] = {"KICK", "SNARE", "TAMB", "HORN", "BASS", "CHORD"};
#elif defined(GENRE_IDM)
static const char *const SEQ_LANE[6] = {"KICK", "SNAR", "HATS", "TONE", "BASS", "CHORD"};
#elif defined(GENRE_SNOWCRASH)
static const char *const SEQ_LANE[6] = {"KICK", "SNAR", "HATS", "BLIP", "BASS", "CHORD"};
#elif defined(GENRE_TECHNO)
static const char *const SEQ_LANE[6] = {"KICK", "SNAR", "HAT", "RAVE", "BASS", "CHORD"};
#elif defined(GENRE_TRIPHOP)
static const char *const SEQ_LANE[6] = {"KICK", "SNAR", "HAT", "PAD", "BASS", "CHORD"};
#elif defined(GENRE_POP)
static const char *const SEQ_LANE[6] = {"KICK", "SNARE", "HAT", "LEAD", "BASS", "CHORD"};
#else
static const char *const SEQ_LANE[6] = {"KICK", "RIM", "HAT", "SIREN", "BASS", "CHORD"};
#endif

static const char *seq_lane_name(uint8_t ln)
{
    if (ln >= 6u)
        return "";
    return SEQ_LANE[ln];
}
#if !defined(GENRE_BASS)
/* Names for DUB_LFO_ENVX..: filled at boot from seq_lane_name() (gv_lfo_names_init in genre_vox.h). */
static char gv_lfo_nm[DUB_LFO_ENVX_END - DUB_LFO_ENVX][10];
static char gv_lfo_nm_long[DUB_LFO_ENVX_END - DUB_LFO_ENVX][20];
#define GV_LFO_NAME_INITS \
    [DUB_LFO_ENVX + 0] = gv_lfo_nm[0], \
    [DUB_LFO_ENVX + 1] = gv_lfo_nm[1], \
    [DUB_LFO_ENVX + 2] = gv_lfo_nm[2], \
    [DUB_LFO_ENVX + 3] = gv_lfo_nm[3], \
    [DUB_LFO_ENVX + 4] = gv_lfo_nm[4], \
    [DUB_LFO_ENVX + 5] = gv_lfo_nm[5], \
    [DUB_LFO_ENVX + 6] = gv_lfo_nm[6], \
    [DUB_LFO_ENVX + 7] = gv_lfo_nm[7], \
    [DUB_LFO_ENVX + 8] = gv_lfo_nm[8], \
    [DUB_LFO_ENVX + 9] = gv_lfo_nm[9], \
    [DUB_LFO_ENVX + 10] = gv_lfo_nm[10], \
    [DUB_LFO_ENVX + 11] = gv_lfo_nm[11], \
    [DUB_LFO_ENVX + 12] = gv_lfo_nm[12], \
    [DUB_LFO_ENVX + 13] = gv_lfo_nm[13], \
    [DUB_LFO_ENVX + 14] = gv_lfo_nm[14], \
    [DUB_LFO_ENVX + 15] = gv_lfo_nm[15], \
    [DUB_LFO_ENVX + 16] = gv_lfo_nm[16], \
    [DUB_LFO_ENVX + 17] = gv_lfo_nm[17], \
    [DUB_LFO_ENVX + 18] = gv_lfo_nm[18], \
    [DUB_LFO_ENVX + 19] = gv_lfo_nm[19], \
    [DUB_LFO_ENVX + 20] = gv_lfo_nm[20], \
    [DUB_LFO_ENVX + 21] = gv_lfo_nm[21], \
    [DUB_LFO_ENVX + 22] = gv_lfo_nm[22], \
    [DUB_LFO_ENVX + 23] = gv_lfo_nm[23], \
    [DUB_LFO_ENVX + 24] = gv_lfo_nm[24], \
    [DUB_LFO_ENVX + 25] = gv_lfo_nm[25], \
    [DUB_LFO_ENVX + 26] = gv_lfo_nm[26], \
    [DUB_LFO_ENVX + 27] = gv_lfo_nm[27], \
    [DUB_LFO_ENVX + 28] = gv_lfo_nm[28], \
    [DUB_LFO_ENVX + 29] = gv_lfo_nm[29], \
    [DUB_LFO_ENVX + 30] = gv_lfo_nm[30], \
    [DUB_LFO_ENVX + 31] = gv_lfo_nm[31], \
    [DUB_LFO_ENVX + 32] = gv_lfo_nm[32], \
    [DUB_LFO_ENVX + 33] = gv_lfo_nm[33], \
    [DUB_LFO_ENVX + 34] = gv_lfo_nm[34], \
    [DUB_LFO_ENVX + 35] = gv_lfo_nm[35], \
    [DUB_LFO_ENVX + 36] = gv_lfo_nm[36], \
    [DUB_LFO_ENVX + 37] = gv_lfo_nm[37], \
    [DUB_LFO_ENVX + 38] = gv_lfo_nm[38], \
    [DUB_LFO_ENVX + 39] = gv_lfo_nm[39], \
    [DUB_LFO_ENVX + 40] = gv_lfo_nm[40], \
    [DUB_LFO_ENVX + 41] = gv_lfo_nm[41]
#define GV_LFO_LONG_INITS \
    [DUB_LFO_ENVX + 0] = gv_lfo_nm_long[0], \
    [DUB_LFO_ENVX + 1] = gv_lfo_nm_long[1], \
    [DUB_LFO_ENVX + 2] = gv_lfo_nm_long[2], \
    [DUB_LFO_ENVX + 3] = gv_lfo_nm_long[3], \
    [DUB_LFO_ENVX + 4] = gv_lfo_nm_long[4], \
    [DUB_LFO_ENVX + 5] = gv_lfo_nm_long[5], \
    [DUB_LFO_ENVX + 6] = gv_lfo_nm_long[6], \
    [DUB_LFO_ENVX + 7] = gv_lfo_nm_long[7], \
    [DUB_LFO_ENVX + 8] = gv_lfo_nm_long[8], \
    [DUB_LFO_ENVX + 9] = gv_lfo_nm_long[9], \
    [DUB_LFO_ENVX + 10] = gv_lfo_nm_long[10], \
    [DUB_LFO_ENVX + 11] = gv_lfo_nm_long[11], \
    [DUB_LFO_ENVX + 12] = gv_lfo_nm_long[12], \
    [DUB_LFO_ENVX + 13] = gv_lfo_nm_long[13], \
    [DUB_LFO_ENVX + 14] = gv_lfo_nm_long[14], \
    [DUB_LFO_ENVX + 15] = gv_lfo_nm_long[15], \
    [DUB_LFO_ENVX + 16] = gv_lfo_nm_long[16], \
    [DUB_LFO_ENVX + 17] = gv_lfo_nm_long[17], \
    [DUB_LFO_ENVX + 18] = gv_lfo_nm_long[18], \
    [DUB_LFO_ENVX + 19] = gv_lfo_nm_long[19], \
    [DUB_LFO_ENVX + 20] = gv_lfo_nm_long[20], \
    [DUB_LFO_ENVX + 21] = gv_lfo_nm_long[21], \
    [DUB_LFO_ENVX + 22] = gv_lfo_nm_long[22], \
    [DUB_LFO_ENVX + 23] = gv_lfo_nm_long[23], \
    [DUB_LFO_ENVX + 24] = gv_lfo_nm_long[24], \
    [DUB_LFO_ENVX + 25] = gv_lfo_nm_long[25], \
    [DUB_LFO_ENVX + 26] = gv_lfo_nm_long[26], \
    [DUB_LFO_ENVX + 27] = gv_lfo_nm_long[27], \
    [DUB_LFO_ENVX + 28] = gv_lfo_nm_long[28], \
    [DUB_LFO_ENVX + 29] = gv_lfo_nm_long[29], \
    [DUB_LFO_ENVX + 30] = gv_lfo_nm_long[30], \
    [DUB_LFO_ENVX + 31] = gv_lfo_nm_long[31], \
    [DUB_LFO_ENVX + 32] = gv_lfo_nm_long[32], \
    [DUB_LFO_ENVX + 33] = gv_lfo_nm_long[33], \
    [DUB_LFO_ENVX + 34] = gv_lfo_nm_long[34], \
    [DUB_LFO_ENVX + 35] = gv_lfo_nm_long[35], \
    [DUB_LFO_ENVX + 36] = gv_lfo_nm_long[36], \
    [DUB_LFO_ENVX + 37] = gv_lfo_nm_long[37], \
    [DUB_LFO_ENVX + 38] = gv_lfo_nm_long[38], \
    [DUB_LFO_ENVX + 39] = gv_lfo_nm_long[39], \
    [DUB_LFO_ENVX + 40] = gv_lfo_nm_long[40], \
    [DUB_LFO_ENVX + 41] = gv_lfo_nm_long[41]
#endif
static const char *const LFO_TGT_NAMES[DUB_LFO_TGT_N] = {
    /* HOME Page 1..3 */
#ifdef GENRE_HEAVY_PSYCH
    "FUZZ",   "LAYER",  "DROP",   "SWEEP",  "SPACE",  "WEIGHT", "GRIT",   "DRONE",
#elif defined(GENRE_FUNK)
    "CHOP",   "CROP",   "WAH",    "SWEEP",  "SPACE",  "DUCK",   "GRIT",   "THUMP",
#elif defined(GENRE_BASS)
    /* Same names as the HOME knobs: M_CHORD/FEED/WOB = P1 ACID/GRIT/SPACE, M_ISO/M_SPACE/M_GRIT/M_DUB = P2 RATCH/GATE/DENS/SHIFT */
    "ACID",   "GRIT",   "SPACE",  "RATCH",  "GATE",   "DRIVE",  "DENS",   "SHIFT",
#elif defined(GENRE_HOUSE)
    "JACK",   "LAYER",  "CUT",    "SWEEP",  "SPACE",  "PUMP",   "GRIT",   "FLOOR",
#elif defined(GENRE_SOUL)
    "STAB",   "ECHO",   "ROTOR",  "SWEEP",  "ROOM",   "BOTTOM", "GRIT",   "SHOUT",
#elif defined(GENRE_HIPHOP)
    "SNAP",   "DUST",   "ROLL",   "SWEEP",  "ROOM",   "DUCK",   "GRIT",   "CHOP",
#elif defined(GENRE_JAZZ)
    "COMP",   "LAYER",  "SWAY",   "SWEEP",  "SPACE",  "WASH",   "GRIT",   "MUTE",
#elif defined(GENRE_IDM)
    "CELL",   "CROP",   "REVERS", "SWEEP",  "SPACE",  "HACK",   "GRIT",   "SCREW",
#elif defined(GENRE_SNOWCRASH)
    "GLOSS",  "SLICE",  "HOP",    "TIDE",   "GHOST",  "CLICK",  "SLAG",   "HUM",
#elif defined(GENRE_TECHNO)
    "PUMP",   "RUMBLE", "HOVER",  "TANK",   "STEEL",  "DIST",   "HARD",   "GATE",
#elif defined(GENRE_TRIPHOP)
    "DUST",   "VINYL",  "SMOKE",  "PAD",    "SUB",    "TAPE",   "DARK",   "FILM",
#elif defined(GENRE_POP)
    "GATE",   "LINE",   "CHOR",   "WIDE",   "DELAY",  "PUMP",   "BRITE",  "LEVEL",
#else
    "SKANK",  "LAYER",  "WARP",   "SWEEP",  "SPACE",  "WEIGHT", "GRIT",   "DUB",
#endif
#ifdef GENRE_SNOWCRASH
    "DRIFT",  "FLUX",
#elif defined(GENRE_TECHNO) || defined(GENRE_BASS)
    "PHASE",  "DRIVE",
#elif defined(GENRE_POP)
    "GLIDE",  "DENS",
#else
    "MOTION", "FLUX",
#endif
#ifdef GENRE_HEAVY_PSYCH
    "BAND",
#elif defined(GENRE_FUNK)
    "UNIT",
#elif defined(GENRE_HOUSE)
    "CREW",
#elif defined(GENRE_HIPHOP)
    "BLOCK",
#elif defined(GENRE_JAZZ)
    "COMBO",
#elif defined(GENRE_SOUL)
    "REVUE",

#elif defined(GENRE_IDM)
    "PACK",
#elif defined(GENRE_SNOWCRASH)
    "DECK",
#elif defined(GENRE_TECHNO)
    "RIG",
#elif defined(GENRE_TRIPHOP)
    "MOOD",
#elif defined(GENRE_BASS)
    "LINE",
#elif defined(GENRE_POP)
    "TRACK",
#else
    "SYSTEM",
#endif
    "INVERT",
    /* BASS Page 1..3 */
#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP)
    "B MODL", "B CUT",  "B DEC",  "B ENV",  "B SUB",  "B DRV",  "B PUMP", "B DLY",
#else
    "B MODL", "B CUT",  "B DEC",  "B WOB",  "B SUB",  "B DRV",  "B PUMP", "B DLY",
#endif
    "BASS",   "B PAN",  "B REV",
    /* CHORD Page 1..3 */
    "C VOIC", "C CUT",  "C DEC",  "C ATK",  "C DLY",  "C TIMB", "C RESO", "C ENV",
    "C GLID", "C PAN",  "CHORD",  "C REV",
    /* DRUM Page 1..4 */
#ifdef GENRE_HEAVY_PSYCH
    "D TONE", "KICK",   "SNARE",  "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "LEAD",   "SING",   "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN LD",
#elif defined(GENRE_HOUSE)
    "D TONE", "KICK",   "SNARE",  "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "STAB",   "SNAP",   "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN ST",
#elif defined(GENRE_JAZZ)
    "D TONE", "KICK",   "RIM",    "RIDE",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "HORN",   "BEND",   "D DLY",  "D REV",  "PAN K",  "PAN R",  "PAN RD", "PAN HN",
#elif defined(GENRE_IDM)
    "D TONE", "KICK",   "SNAR",   "HATS",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "TONE",   "EDGE",   "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN TN",
#elif defined(GENRE_SNOWCRASH)
    "D TONE", "KICK",   "SNAR",   "HATS",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "BLIP",   "PIERCE", "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN BL",
#elif defined(GENRE_HIPHOP)
    "D TONE", "KICK",   "SNARE",  "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "SCRX",   "DRAG",   "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN SX",
#elif defined(GENRE_POP)
    "D TONE", "KICK",   "SNARE",  "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "LEAD",   "DRIVE",  "D DLY",  "D REV",  "PAN K",  "PAN S",  "PAN H",  "PAN LD",
#elif defined(GENRE_FUNK)
    "D TONE", "KICK",   "SNARE",  "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "STAB",   "SNAP",   "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN ST",
#elif defined(GENRE_BASS)
    "VOICE",  "CUT",    "DRIVE",  "SUB",
    "ENV",    "DECAY",  "ATTACK", "PUMP",
    "LVL",    "GLIDE",  "ECHO",   "ROOM",   "PAN",    "MIX",    "SPACE",  "FLOOR",
#elif defined(GENRE_TECHNO)
    "D TONE", "KICK",   "SNARE",  "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "TONE",   "S RATE", "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN TN",
#elif defined(GENRE_TRIPHOP)
    "D TONE", "KICK",   "SNAR",   "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "PAD",    "P RATE", "D DLY",  "D REV",  "PAN K",  "PAN SN", "PAN H",  "PAN PD",
#else
    "D TONE", "KICK",   "RIM",    "HAT",
    "D KIT",  "D TUNE", "D DEC",  "D DRV",
    "SIREN",  "S RATE", "D DLY",  "D REV",  "PAN K",  "PAN R",  "PAN H",  "PAN S",
#endif
    "KEY",    "SCALE",  "SWING",  "TEMPO",
    /* FX Page 1..3 */
    "E MIX",  "E TIME", "E FDBK", "E TONE",
#if defined(GENRE_POP)
    "E WOW",  "E TAP",  "E ROOM", "E ISO",
#else
    "E WOW",  "E TAP",  "E TANK", "E ISO",
#endif
    "REVERB", "R DAMP", "R TAIL", "BLEED",
    /* SEQ */
    "S PAT",  "S ROT",  "S VARY", "S VEL",  "S TRIG", "S GATE", "S PTCH",
    "S NUDG", "S MICR",
    "B ATK",  "D CUT",  "D ATK",
#ifdef GENRE_HEAVY_PSYCH
    "K TUNE", "SN TUN", "H TUNE", "LD TUN",
    "K CUT",  "SN CUT", "H CUT",  "LD CUT",
    "K ATK",  "SN ATK", "H ATK",  "LD ATK",
    "K DEC",  "SN DEC", "H DEC",  "LD DEC",
    "K DRV",  "SN DRV", "H DRV",  "LD DRV",
    "K DLY",  "SN DLY", "H DLY",  "LD DLY",
    "K REV",  "SN REV", "H REV",  "LD REV",
    "K PNCH", "SN SNP", "H SIZL", "LD TMB"
#elif defined(GENRE_HOUSE)
    "K TUNE", "SN TUN", "H TUNE", "ST TUN",
    "K CUT",  "SN CUT", "H CUT",  "ST CUT",
    "K ATK",  "SN ATK", "H ATK",  "ST ATK",
    "K DEC",  "SN DEC", "H DEC",  "ST DEC",
    "K DRV",  "SN DRV", "H DRV",  "ST SNP",
    "K DLY",  "SN DLY", "H DLY",  "ST DLY",
    "K REV",  "SN REV", "H REV",  "ST REV",
    "K PNCH", "SN SNP", "H SIZL", "ST TMB"
#elif defined(GENRE_JAZZ)
    "K TUNE", "R TUNE", "RD TUN", "HN TUN",
    "K CUT",  "R CUT",  "RD CUT", "HN CUT",
    "K ATK",  "R ATK",  "RD ATK", "HN ATK",
    "K DEC",  "R DEC",  "RD DEC", "HN DEC",
    "K DRV",  "R DRV",  "RD DRV", "HN BND",
    "K DLY",  "R DLY",  "RD DLY", "HN DLY",
    "K REV",  "R REV",  "RD REV", "HN REV",
    "K PNCH", "R SNAP", "RD SZL", "HN TMB"
#elif defined(GENRE_SOUL)
    "K TUNE", "SN TUN", "T TUNE", "HN TUN",
    "K CUT",  "SN CUT", "T CUT",  "HN CUT",
    "K ATK",  "SN ATK", "T ATK",  "HN ATK",
    "K DEC",  "SN DEC", "T DEC",  "HN DEC",
    "K DRV",  "SN DRV", "T DRV",  "HN BND",
    "K DLY",  "SN DLY", "T DLY",  "HN DLY",
    "K REV",  "SN REV", "T REV",  "HN REV",
    "K PNCH", "SN SNP", "T SIZL", "HN TMB"
#elif defined(GENRE_HIPHOP)
    "K TUNE", "SN TUN", "H TUNE", "SX TUN",
    "K CUT",  "SN CUT", "H CUT",  "SX CUT",
    "K ATK",  "SN ATK", "H ATK",  "SX ATK",
    "K DEC",  "SN DEC", "H DEC",  "SX DEC",
    "K DRV",  "SN DRV", "H DRV",  "SX BND",
    "K DLY",  "SN DLY", "H DLY",  "SX DLY",
    "K REV",  "SN REV", "H REV",  "SX REV",
    "K PNCH", "SN SNP", "H SIZL", "SX TMB"
#elif defined(GENRE_IDM)
    "K TUNE", "R TUNE", "RD TUN", "TN TUN",
    "K CUT",  "R CUT",  "RD CUT", "TN CUT",
    "K ATK",  "R ATK",  "RD ATK", "TN ATK",
    "K DEC",  "R DEC",  "RD DEC", "TN DEC",
    "K DRV",  "R DRV",  "RD DRV", "TN EDG",
    "K DLY",  "R DLY",  "RD DLY", "TN DLY",
    "K REV",  "R REV",  "RD REV", "TN REV",
    "K PNCH", "R SNAP", "RD SZL", "TN TMB"
#elif defined(GENRE_SNOWCRASH)
    "K TUNE", "R TUNE", "RD TUN", "BL TUN",
    "K CUT",  "R CUT",  "RD CUT", "BL CUT",
    "K ATK",  "R ATK",  "RD ATK", "BL ATK",
    "K DEC",  "R DEC",  "RD DEC", "BL DEC",
    "K DRV",  "R DRV",  "RD DRV", "BL PRC",
    "K DLY",  "R DLY",  "RD DLY", "BL DLY",
    "K REV",  "R REV",  "RD REV", "BL REV",
    "K PNCH", "R SNAP", "RD SZL", "BL TMB"
#elif defined(GENRE_BASS)
    /* KTUNE_K..DATK_S: KO BASS ENV page + GROOVE p1 (no drum lanes) */
    "E DLY",  "E ATK",  "E HOLD", "E DEC",
    "E SUS",  "E REL",  "E CURV", "E ACC",
    "TRANSP", "VEL S",  "ECHO",   "ROOM",
    "GATE",   "ACCNT",  "SLIDE",  "RATCH",
    "MAC1",   "MAC2",   "MAC3",   "SWEEP",
    "SPACE",  "FLOOR",  "GRIT",   "LINE",
    "PHASE",  "DRIVE",  "LAYER",  "CUT B",
    "VOICE",  "CUT B",  "ENV",    "MIX"
#elif defined(GENRE_TECHNO)
    "K TUNE", "SN TUN", "H TUNE", "TN TUN",
    "K CUT",  "SN CUT", "H CUT",  "TN CUT",
    "K ATK",  "SN ATK", "H ATK",  "TN ATK",
    "K DEC",  "SN DEC", "H DEC",  "TN DEC",
    "K DRV",  "SN DRV", "H DRV",  "TN TMB",
    "K DLY",  "SN DLY", "H DLY",  "TN DLY",
    "K REV",  "SN REV", "H REV",  "TN REV",
    "K PNCH", "SN SNP", "H SIZL", "TN TMB"
#elif defined(GENRE_TRIPHOP)
    "K TUNE", "SN TUN", "H TUNE", "PD TUN",
    "K CUT",  "SN CUT", "H CUT",  "PD CUT",
    "K ATK",  "SN ATK", "H ATK",  "PD ATK",
    "K DEC",  "SN DEC", "H DEC",  "PD DEC",
    "K DRV",  "SN DRV", "H DRV",  "PD TMB",
    "K DLY",  "SN DLY", "H DLY",  "PD DLY",
    "K REV",  "SN REV", "H REV",  "PD REV",
    "K PNCH", "SN SNP", "H SIZL", "PD TMB"
#else
    "K TUNE", "R TUNE", "H TUNE", "S TUNE",
    "K CUT",  "R CUT",  "H CUT",  "S CUT",
    "K ATK",  "R ATK",  "H ATK",  "S ATK",
    "K DEC",  "R DEC",  "H DEC",  "S DEC",
    "K DRV",  "R DRV",  "H DRV",  "S DRV",
    "K DLY",  "R DLY",  "H DLY",  "S DLY",
    "K REV",  "R REV",  "H REV",  "S REV",
    "K PNCH", "R SNAP", "H SIZL", "S TIMB"
#endif
    ,
    "OCT",
    "FB FEED", "FB DUB", "FB SPACE", "FB FLUX",
    "WOW FEED", "WOW GRIT", "WOW FLUX",
    "TANK SPC",
    "REV FEED", "REV SPC", "REV FLUX",
    "LOOP DUB", "LOOP FLX",
    "PAN B MO", "PAN C MO", "GLIDE MO",
    "CUT C FX", "CUT B FX", "WOB FX", "DRUM PAN",
    [DUB_LFO_SEQ_PACE] = "PACE"
#if !defined(GENRE_BASS)
    , GV_LFO_NAME_INITS
#endif
};
static const char *const LFO_TGT_LONG[DUB_LFO_TGT_N] = {
    /* HOME Page 1..3 */
#ifdef GENRE_HEAVY_PSYCH
    "MACRO FUZZ",   "MACRO LAYER",  "MACRO DROP",   "MACRO SWEEP",
    "MACRO SPACE",  "MACRO WEIGHT", "MACRO GRIT",   "MACRO DRONE",
#elif defined(GENRE_HOUSE)
    "MACRO JACK",   "MACRO LAYER",  "MACRO CUT",    "MACRO SWEEP",
    "MACRO SPACE",  "MACRO PUMP",   "MACRO GRIT",   "MACRO FLOOR",
#elif defined(GENRE_HIPHOP)
    "MACRO SNAP",   "MACRO DUST",   "MACRO ROLL",   "MACRO SWEEP",
#elif defined(GENRE_SOUL)
    "MACRO STAB",   "MACRO ECHO",   "MACRO ROTOR",  "MACRO SWEEP",
    "MACRO ROOM",   "MACRO BOTTOM", "MACRO GRIT",   "MACRO SHOUT",

    "MACRO ROOM",   "MACRO DUCK",   "MACRO GRIT",   "MACRO CHOP",
#elif defined(GENRE_JAZZ)
    "MACRO COMP",   "MACRO LAYER",  "MACRO SWAY",   "MACRO SWEEP",
    "MACRO SPACE",  "MACRO WASH",   "MACRO GRIT",   "MACRO MUTE",
#elif defined(GENRE_IDM)
    "MACRO CELL",   "MACRO CROP",   "MACRO REVERSE", "MACRO SWEEP",
    "MACRO SPACE",  "MACRO HACK",   "MACRO GRIT",   "MACRO SCREW",
#elif defined(GENRE_SNOWCRASH)
    "MACRO GLOSS",  "MACRO SLICE",  "MACRO HOP",    "MACRO TIDE",
    "MACRO GHOST",  "MACRO CLICK",  "MACRO SLAG",   "MACRO HUM",
#elif defined(GENRE_POP)
    "MACRO CHORD",  "MACRO TAIL",   "MACRO WOBBLE", "MACRO WIDE",
    "MACRO DELAY",  "MACRO PUMP",   "MACRO BRITE",  "MACRO MIX",
#elif defined(GENRE_FUNK)
    "MACRO CHOP",   "MACRO CROP",   "MACRO WAH",    "MACRO SWEEP",
    "MACRO SPACE",  "MACRO DUCK",   "MACRO GRIT",   "MACRO THUMP",
#elif defined(GENRE_TECHNO)
    "MACRO PUMP",   "MACRO RUMBLE", "MACRO HOVER",  "MACRO TANK",
    "MACRO STEEL",  "MACRO DIST",   "MACRO HARD",   "MACRO GATE",
#elif defined(GENRE_BASS)
    "MACRO PUMP",   "MACRO LAYER",  "MACRO CUT",    "MACRO SWEEP",
    "MACRO SPACE",  "MACRO DRIVE",  "MACRO GRIT",   "MACRO FLOOR",
#elif defined(GENRE_TRIPHOP)
    "MACRO DUST",   "MACRO VINYL",  "MACRO SMOKE",  "MACRO PAD",
    "MACRO SUB",    "MACRO TAPE",   "MACRO DARK",   "MACRO FILM",
#else
    "MACRO SKANK",  "MACRO LAYER",  "MACRO WARP",   "MACRO SWEEP",
    "MACRO SPACE",  "MACRO WEIGHT", "MACRO GRIT",   "MACRO DUB",
#endif
#ifdef GENRE_SNOWCRASH
    "MACRO DRIFT",  "MACRO FLUX",
#elif defined(GENRE_TECHNO) || defined(GENRE_BASS)
    "MACRO PHASE",  "MACRO DRIVE",
#else
    "MACRO MOTION", "MACRO FLUX",
#endif
#ifdef GENRE_HEAVY_PSYCH
    "SEQ BAND",
#elif defined(GENRE_HOUSE)
    "SEQ CREW",
#elif defined(GENRE_HIPHOP)
    "SEQ BLOCK",
#elif defined(GENRE_JAZZ)
    "SEQ COMBO",
#elif defined(GENRE_SOUL)
    "SEQ REVUE",
#elif defined(GENRE_IDM)
    "SEQ PACK",
#elif defined(GENRE_SNOWCRASH)
    "SEQ DECK",
#elif defined(GENRE_POP)
    "SEQ TRACK",
#elif defined(GENRE_FUNK)
    "SEQ UNIT",
#elif defined(GENRE_TECHNO)
    "SEQ RIG",
#elif defined(GENRE_BASS)
    "SEQ LINE",
#elif defined(GENRE_TRIPHOP)
    "SEQ MOOD",
#else
    "SEQ SYSTEM",
#endif
    "SEQ INVERT",
    /* BASS Page 1..3 */
    "BASS MODEL",   "BASS CUTOFF",  "BASS DECAY",
#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP)
    "BASS ENV",
#else
    "BASS WOBBLE",
#endif
    "BASS SUB",     "BASS DRIVE",   "BASS PUMP",    "BASS DELAY",
    "BASS LEVEL",   "BASS PAN",     "BASS REVERB",
    /* CHORD Page 1..3 */
    "CHORD VOICE",  "CHORD CUTOFF", "CHORD DECAY",  "CHORD ATTACK",
    "CHORD DELAY",  "CHORD TIMBRE", "CHORD RESONA", "CHORD ENVLPE",
    "CHORD GLIDE",  "CHORD PAN",    "CHORD LEVEL",  "CHORD REVERB",
    /* DRUM Page 1..4 */
#ifdef GENRE_HEAVY_PSYCH
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "LEAD LEVEL",   "LEAD RATE",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "LEAD PAN",
#elif defined(GENRE_HOUSE)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "STAB LEVEL",   "STAB SNAP",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "STAB PAN",
#elif defined(GENRE_JAZZ)
    "DRUM TONE", "DRUM KICK",    "DRUM RIM",     "DRUM RIDE",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "HORN LEVEL",   "HORN BEND",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "RIM PAN",      "RIDE PAN",     "HORN PAN",
#elif defined(GENRE_IDM)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HATS",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "TONE LEVEL",   "TONE EDGE",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HATS PAN",     "TONE PAN",
#elif defined(GENRE_SNOWCRASH)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HATS",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "BLIP LEVEL",   "BLIP PIERCE",  "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HATS PAN",     "BLIP PAN",
#elif defined(GENRE_HIPHOP)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "SCRATCH LEVEL", "SCRATCH DRAG", "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "SCRATCH PAN",
#elif defined(GENRE_POP)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "LEAD LEVEL",   "LEAD DRIVE",   "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "LEAD PAN",
#elif defined(GENRE_FUNK)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "STAB LEVEL",   "STAB SNAP",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "STAB PAN",
#elif defined(GENRE_BASS)
    "VOICE MODEL",  "VOICE CUT",    "VOICE DRIVE",  "VOICE SUB",
    "ENV SHAPE",    "ENV DECAY",    "ENV ATTACK",   "VOICE PUMP",
    "VOICE LEVEL",  "VOICE GLIDE",  "ECHO MIX",     "REVERB MIX",
    "VOICE PAN",    "VOICE MIX",    "MACRO SPACE",  "MACRO FLOOR",
#elif defined(GENRE_TECHNO)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "TONE LEVEL",   "TONE RATE",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "TONE PAN",
#elif defined(GENRE_TRIPHOP)
    "DRUM TONE", "DRUM KICK",    "DRUM SNARE",   "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "PAD LEVEL",   "PAD RATE",    "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "SNARE PAN",    "HAT PAN",      "PAD PAN",
#else
    "DRUM TONE", "DRUM KICK",    "DRUM RIM",     "DRUM HAT",
    "DRUM KIT",     "KIT TUNE",     "KIT DECAY",    "KIT DRIVE",
    "SIREN LEVEL",  "SIREN RATE",   "DRUM DELAY",   "DRUM REVERB",  "KICK PAN",     "RIM PAN",      "HAT PAN",      "SIREN PAN",
#endif
    "GROOVE KEY",   "GROOVE SCALE", "GROOVE SWING", "GROOVE TEMPO",
    /* FX Page 1..3 */
    "ECHO MIX",     "ECHO TIME",    "ECHO FEEDBACK", "ECHO TONE",
#if defined(GENRE_POP)
    "ECHO WOW",     "ECHO TAP",     "REVERB ROOM",  "ISOLATOR",
#else
    "ECHO WOW",     "ECHO TAP",     "REVERB TANK",  "ISOLATOR",
#endif
    "REVERB MIX",   "REVERB DAMP",  "REVERB TAIL",  "ECHO BLEED",
    /* SEQ */
    "SEQ PATTRN",   "SEQ ROTATE",   "SEQ VARY",     "SEQ VEL+-",
    "SEQ TRIG %",   "SEQ GATE+-",   "SEQ PITCH+-",  "SEQ NUDGE",
    "SEQ MICRO",
    "BASS ATTACK",  "DRUM CUTOFF",  "DRUM ATTACK",
#ifdef GENRE_HEAVY_PSYCH
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "LEAD TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "LEAD CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "LEAD ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "LEAD DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "LEAD DRIVE",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "LEAD DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "LEAD REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "LEAD TIMBRE"
#elif defined(GENRE_HOUSE)
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "STAB TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "STAB CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "STAB ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "STAB DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "STAB SNAP",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "STAB DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "STAB REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "STAB TIMBRE"
#elif defined(GENRE_JAZZ)
    "KICK TUNE",    "RIM TUNE",     "RIDE TUNE",    "HORN TUNE",
    "KICK CUTOFF",  "RIM CUTOFF",   "RIDE CUTOFF",  "HORN CUTOFF",
    "KICK ATTACK",  "RIM ATTACK",   "RIDE ATTACK",  "HORN ATTACK",
    "KICK DECAY",   "RIM DECAY",    "RIDE DECAY",   "HORN DECAY",
    "KICK DRIVE",   "RIM DRIVE",    "RIDE DRIVE",   "HORN BEND",
    "KICK DELAY",   "RIM DELAY",    "RIDE DELAY",   "HORN DELAY",
    "KICK REVERB",  "RIM REVERB",   "RIDE REVERB",  "HORN REVERB",
    "KICK PUNCH", "RIM SNAP", "RIDE SIZZLE", "HORN TIMBRE"
#elif defined(GENRE_IDM)
    "KICK TUNE",    "SNARE TUNE",   "HATS TUNE",    "TONE TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HATS CUTOFF",  "TONE CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HATS ATTACK",  "TONE ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HATS DECAY",   "TONE DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HATS DRIVE",   "TONE EDGE",
    "KICK DELAY",   "SNARE DELAY",  "HATS DELAY",   "TONE DELAY",
    "KICK REVERB",  "SNARE REVERB", "HATS REVERB",  "TONE REVERB",
    "KICK PUNCH", "SNARE SNAP", "HATS SIZZLE", "TONE TIMBRE"
#elif defined(GENRE_SNOWCRASH)
    "KICK TUNE",    "SNARE TUNE",   "HATS TUNE",    "BLIP TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HATS CUTOFF",  "BLIP CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HATS ATTACK",  "BLIP ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HATS DECAY",   "BLIP DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HATS DRIVE",   "BLIP PIERCE",
    "KICK DELAY",   "SNARE DELAY",  "HATS DELAY",   "BLIP DELAY",
    "KICK REVERB",  "SNARE REVERB", "HATS REVERB",  "BLIP REVERB",
    "KICK PUNCH", "SNARE SNAP", "HATS SIZZLE", "BLIP TIMBRE"
#elif defined(GENRE_HIPHOP)
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "SCRATCH TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "SCRATCH CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "SCRATCH ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "SCRATCH DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "SCRATCH BEND",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "SCRATCH DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "SCRATCH REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "SCRATCH TIMB"
#elif defined(GENRE_POP)
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "LEAD TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "LEAD CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "LEAD ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "LEAD DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "LEAD DRIVE",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "LEAD DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "LEAD REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "LEAD TIMBRE"
#elif defined(GENRE_FUNK)
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "STAB TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "STAB CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "STAB ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "STAB DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "STAB SNAP",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "STAB DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "STAB REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "STAB TIMBRE"
#elif defined(GENRE_BASS)
    "ENV DELAY",    "ENV ATTACK",   "ENV HOLD",     "ENV DECAY",
    "ENV SUSTAIN",  "ENV RELEASE",  "ENV CURVE",    "ENV ACCENT",
    "TRANSPOSE",    "VEL SENS",     "ECHO MIX",     "REVERB MIX",
    "MACRO PUMP",   "MACRO LAYER",  "MACRO CUT",    "MACRO SWEEP",
    "MACRO SPACE",  "MACRO DRIVE",  "MACRO GRIT",   "MACRO FLOOR",
    "MACRO PHASE",  "MACRO DRIVE",  "VOICE GLIDE",  "VOICE LEVEL",
    "ENV DEST",     "ENV AMOUNT",   "ENV CURVE",    "VOICE PAN"
#elif defined(GENRE_TECHNO)
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "TONE TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "TONE CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "TONE ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "TONE DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "TONE TIMBRE",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "TONE DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "TONE REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "TONE TIMBRE"
#elif defined(GENRE_TRIPHOP)
    "KICK TUNE",    "SNARE TUNE",   "HAT TUNE",     "PAD TUNE",
    "KICK CUTOFF",  "SNARE CUTOFF", "HAT CUTOFF",   "PAD CUTOFF",
    "KICK ATTACK",  "SNARE ATTACK", "HAT ATTACK",   "PAD ATTACK",
    "KICK DECAY",   "SNARE DECAY",  "HAT DECAY",    "PAD DECAY",
    "KICK DRIVE",   "SNARE DRIVE",  "HAT DRIVE",    "PAD TIMBRE",
    "KICK DELAY",   "SNARE DELAY",  "HAT DELAY",    "PAD DELAY",
    "KICK REVERB",  "SNARE REVERB", "HAT REVERB",   "PAD REVERB",
    "KICK PUNCH", "SNARE SNAP", "HAT SIZZLE", "PAD TIMBRE"
#else
    "KICK TUNE",    "RIM TUNE",     "HAT TUNE",     "SIREN TUNE",
    "KICK CUTOFF",  "RIM CUTOFF",   "HAT CUTOFF",   "SIREN CUTOFF",
    "KICK ATTACK",  "RIM ATTACK",   "HAT ATTACK",   "SIREN ATTACK",
    "KICK DECAY",   "RIM DECAY",    "HAT DECAY",    "SIREN DECAY",
    "KICK DRIVE",   "RIM DRIVE",    "HAT DRIVE",    "SIREN DRIVE",
    "KICK DELAY",   "RIM DELAY",    "HAT DELAY",    "SIREN DELAY",
    "KICK REVERB",  "RIM REVERB",   "HAT REVERB",   "SIREN REVERB",
    "KICK PUNCH", "RIM SNAP", "HAT SIZZLE", "SIREN TIMBRE"
#endif
    ,
    "GROOVE OCTAVE",
    "SHARE FB / FEED", "SHARE FB / DUB", "SHARE FB / SPACE", "SHARE FB / FLUX",
    "SHARE WOW / FEED", "SHARE WOW / GRIT", "SHARE WOW / FLUX",
    "SHARE TANK / SPACE",
    "SHARE REV / FEED", "SHARE REV / SPACE", "SHARE REV / FLUX",
    "SHARE LOOP / DUB", "SHARE LOOP / FLUX",
    "SHARE PAN B / MOTION", "SHARE PAN C / MOTION", "SHARE GLIDE / MOTION",
    "SHARE CUT C / FLUX", "SHARE CUT B / FLUX", "SHARE WOBBLE / FLUX", "SHARE DRUM PAN / FLUX",
    [DUB_LFO_SEQ_PACE] = "SEQ PACE"
#if !defined(GENRE_BASS)
    , GV_LFO_LONG_INITS
#endif
};

static const uint8_t DUB_ENS_MUTE[FM_SEQ_NENSEMBLES] = {
    0x3Fu, 0x3Eu, 0x17u, 0x07u, 0x15u, 0x11u, 0x26u, 0x2Cu, 0x0Eu,
    0x31u, 0x13u, 0x19u, 0x2Eu, 0x2Au, 0x38u, 0x28u, 0x01u, 0x00u
};
#ifdef GENRE_HEAVY_PSYCH
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "FUZZ",
    "STOMP",  "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#elif defined(GENRE_HOUSE)
/* Same lane masks as DUB. Display only: clap/stab/jack instead of rim/siren/skank. */
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "JACK",
    "CLUB",   "PULSE",  "DRUMS",  "GROOVE", "STRIP",  "TUTTI"
};
#elif defined(GENRE_HIPHOP)
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "BLOCK",
    "CLUB",   "PULSE",  "DRUMS",  "GROOVE", "STRIP",  "TUTTI"
};
#elif defined(GENRE_JAZZ)
/* Same masks as DUB; SKANK → COMP (no dub branding). */
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "COMP",
    "WALK",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#elif defined(GENRE_IDM)
/* Same masks as DUB; jazz labels → IDM (CELL = old SKANK slot). */
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "PAIR",   "PAD",    "SNAP",   "SLANT",
    "GLINT",  "BREAK",  "STACK",  "TOPS",   "CHORD",  "CELL",
    "THUD",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#elif defined(GENRE_SNOWCRASH)
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "PAIR",   "PAD",    "SNAP",   "SLANT",
    "GLINT",  "BREAK",  "STACK",  "TOPS",   "CHORD",  "GLOSS",
    "THUD",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#elif defined(GENRE_POP)
/* Same lane masks as DUB; SKANK → HOOK (matches ARP mode 1 toast). */
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "HOOK",
    "CLUB",   "PULSE",  "DRUMS",  "GROOVE", "STRIP",  "TUTTI"
};
#elif defined(GENRE_FUNK)
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "THEONE", "STABS",  "KEYS",   "BACK",   "RIDE",
    "NAIL",   "BREAK",  "TRIO",   "TOPS",   "HITS",   "CUTS",
    "ROOTS",  "PULSE",  "KIT",    "GRID",   "OPEN",   "TUTTI"
};
#elif defined(GENRE_BASS)
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "HIGHS",
    "DEEP",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#elif defined(GENRE_TRIPHOP)
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "PAIR",   "PAD",    "SNAP",   "SLANT",
    "GLINT",  "BREAK",  "STACK",  "TOPS",   "CHORD",  "DUST",
    "THUD",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#elif defined(GENRE_TECHNO)
/* Same lane masks as DUB. Display only: PUMP instead of SKANK. */
static const char *const GENRE_ENS_NAMES[FM_SEQ_NENSEMBLES] = {
    "TACET",  "KICK",   "DUET",   "HARM",   "SNAP",   "SWING",
    "LICK",   "BREAK",  "TRIO",   "TOPS",   "CHORD",  "PUMP",
    "DEEP",   "PULSE",  "DRUMS",  "GROOVE", "SPARSE", "TUTTI"
};
#else
#define GENRE_ENS_NAMES FM_SEQ_ENS_NAMES
#endif

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const uint16_t BANK_COL[BANK_COUNT] = {
    RGB(140, 180, 255), RGB(100, 220, 180), RGB(255, 180, 120), RGB(180, 140, 255),
    RGB(200, 160, 255), RGB(255, 200, 120)
};

static const uint16_t ERA_COL[GENRE_ERA_N] = {
    RGB(255, 190, 60),  /* KINGSTON */
    RGB(255, 150, 200), /* LOVERS */
    RGB(100, 210, 255), /* CHANNEL */
    RGB(80, 235, 160),  /* BRISTOL */
    RGB(255, 95, 120),  /* CROYDON */
    RGB(230, 185, 255)  /* POSTDUB */
};
#elif defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_SOUL) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP)
static const uint16_t BANK_COL[BANK_COUNT] = GENRE_BANK_COL;
static const uint16_t ERA_COL[GENRE_ERA_N] = GENRE_ERA_COL;
#endif


#ifndef fm1_led_key_dim
#define fm1_led_key_dim(id, on) ((void)0)
#endif

static void copy_str(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static int fm_era_is_artist(uint8_t era)
{
    return era == (uint8_t)FM_ERA_ARTIST;
}

static uint8_t fm_era_knob_clamp(uint8_t era)
{
    if (era >= FM_ERA_KNOB_N)
        return (uint8_t)(FM_ERA_KNOB_N - 1u);
    return era;
}

static uint16_t fm_era_knob_col(uint8_t era)
{
    era = fm_era_knob_clamp(era);
    if (fm_era_is_artist(era))
        return RGB(255, 220, 120);
    return ERA_COL[era % GENRE_ERA_N];
}

static const char *fm_era_knob_name(uint8_t era)
{
    if (fm_era_is_artist(fm_era_knob_clamp(era)))
        return "ARTIST";
    return genre_era_name(era);
}

static uint8_t fm_artist_plate_count(void)
{
    uint32_t i, n = 0u;

    for (i = 0u; i < GENRE_CFG_USER_PLATE_N; i++) {
        if (g.user_plate_mask & (1ull << i))
            n++;
    }
    return (uint8_t)n;
}

static uint8_t fm_artist_first_plate(void)
{
    uint32_t i;

    for (i = 0u; i < GENRE_CFG_USER_PLATE_N; i++) {
        if (g.user_plate_mask & (1ull << i))
            return (uint8_t)i;
    }
    return 0u;
}

static uint8_t fm_artist_cycle_plate(uint8_t cur, int32_t dir)
{
    uint32_t i;

    cur = genre_clamp_preset(cur);
    if (!fm_artist_plate_count())
        return cur;
    if (dir > 0) {
        for (i = (uint32_t)cur + 1u; i < GENRE_CFG_USER_PLATE_N; i++) {
            if (g.user_plate_mask & (1ull << i))
                return (uint8_t)i;
        }
        return fm_artist_first_plate();
    }
    if (cur > 0u) {
        for (i = (uint32_t)cur; i > 0u; i--) {
            if (g.user_plate_mask & (1ull << (i - 1u)))
                return (uint8_t)(i - 1u);
        }
    }
    for (i = GENRE_CFG_USER_PLATE_N; i > 0u; i--) {
        if (g.user_plate_mask & (1ull << (i - 1u)))
            return (uint8_t)(i - 1u);
    }
    return fm_artist_first_plate();
}

static void u8_dec(char *d, uint32_t v)
{
    if (v >= 100u) {
        *d++ = (char)('0' + v / 100u);
        v %= 100u;
        *d++ = (char)('0' + v / 10u);
        *d++ = (char)('0' + v % 10u);
    } else if (v >= 10u) {
        *d++ = (char)('0' + v / 10u);
        *d++ = (char)('0' + v % 10u);
    } else
        *d++ = (char)('0' + v);
    *d = 0;
}

/* UI only: map stored/MIDI 0..127 → knob readout 0..100 (rounded). */
static uint32_t u8_show100_from127(uint32_t v)
{
    if (v > 127u)
        v = 127u;
    return (v * 100u + 63u) / 127u;
}

static void u8_show100_dec(char *d, uint32_t v)
{
    u8_dec(d, u8_show100_from127(v));
}

static void i32_signed_dec(char *d, int32_t v)
{
    if (v < 0) {
        *d++ = '-';
        u8_dec(d, (uint32_t)(-v));
    } else if (v > 0) {
        *d++ = '+';
        u8_dec(d, (uint32_t)v);
    } else
        copy_str(d, "0");
}

/* Bipolar 0..127 (centre 64) → −50..50. Single-value readouts keep a leading + via i32_signed_dec;
 * LFO range text uses bip50_show_plain (no +). */
static int32_t bip50_from127(uint8_t v)
{
    int32_t c = (int32_t)(v > 127u ? 127u : v) - 64;

    if (c < -63)
        c = -63;
    return c < 0 ? -((-c * 50 + 31) / 63) : (c * 50 + 31) / 63;
}

static void bip50_show(char *s, uint8_t v)
{
    i32_signed_dec(s, bip50_from127(v));
}

static void bip50_show_plain(char *s, uint8_t v)
{
    int32_t n = bip50_from127(v);

    if (n < 0) {
        *s++ = '-';
        u8_dec(s, (uint32_t)(-n));
    } else
        u8_dec(s, (uint32_t)n);
}

#if defined(GENRE_BASS)
/* Signed mod amount -127..+127 shown as -100 .. 0 .. +100. */
static void amt100_show(char *s, int8_t a)
{
    int32_t m = a < 0 ? -(int32_t)a : (int32_t)a;

    if (m > 127)
        m = 127;
    m = (m * 100 + 63) / 127;
    i32_signed_dec(s, a < 0 ? -m : m);
}
#endif

static void adj_u8(uint8_t *p, int32_t d, uint8_t lo, uint8_t hi)
{
    int32_t v = (int32_t)*p + d;
    if (v < (int32_t)lo)
        v = (int32_t)lo;
    if (v > (int32_t)hi)
        v = (int32_t)hi;
    *p = (uint8_t)v;
}

static int32_t knob_gain(uint8_t slot, int32_t steps)
{
    return ko_knob_gain(&g.knob_ms[slot & 3u], fm1_ms, steps);
}

static const char *const NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const char *const SCALE_NAMES[9] = {
    "DUB", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "DUB", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_HEAVY_PSYCH)
static const char *const SCALE_NAMES[9] = {
    "HEAVY", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "HVY", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_JAZZ)
static const char *const SCALE_NAMES[9] = {
    "JAZZ", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "JAZ", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_IDM)
static const char *const SCALE_NAMES[9] = {
    "IDM", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "IDM", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_SNOWCRASH)
static const char *const SCALE_NAMES[9] = {
    "XCRASH", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "XCR", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_HIPHOP)
static const char *const SCALE_NAMES[9] = {
    "HIPHOP", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "HIP", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_SOUL)
static const char *const SCALE_NAMES[9] = {
    "SOUL", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "SOL", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#elif defined(GENRE_POP)
static const char *const SCALE_NAMES[9] = {
    "POP", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "POP", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#else
static const char *const SCALE_NAMES[9] = {
    "HOUSE", "MINOR", "DORIAN", "PHRYG", "MAJOR", "MIXO", "PENT.M", "BLUES", "HARM.M"
};
static const char *const SCALE_HDR[9] = {
    "HOU", "MIN", "DOR", "PHR", "MAJ", "MIX", "PNT", "BLU", "H.m"
};
#endif
static const uint8_t SCALE_LEN[9] = { 6, 7, 7, 7, 7, 7, 5, 6, 7 };
static const uint8_t SCALE_SEMIS[9][7] = {
    { 0, 2, 3, 5, 7, 10, 0 },
    { 0, 2, 3, 5, 7, 8, 10 },
    { 0, 2, 3, 5, 7, 9, 10 },
    { 0, 1, 3, 5, 7, 8, 10 },
    { 0, 2, 4, 5, 7, 9, 11 },
    { 0, 2, 4, 5, 7, 9, 10 },
    { 0, 3, 5, 7, 10, 0, 0 },
    { 0, 3, 5, 6, 7, 10, 0 },
    { 0, 2, 3, 5, 7, 8, 11 }
};
/* KO BASS hides index 0 (HOUSE): selectable range MINOR..HARM.M. */
#if defined(GENRE_BASS)
#define SCALE_LO 1u
#else
#define SCALE_LO 0u
#endif
#define SCALE_N (9u - SCALE_LO)

static uint8_t scale_ix(uint8_t scale)
{
    scale %= 9u;
#if defined(GENRE_BASS)
    if (scale < SCALE_LO)
        scale = SCALE_LO;
#endif
    return scale;
}

/* Wrap `scale + steps` within SCALE_LO..8. */
static uint8_t scale_step(uint8_t scale, int32_t steps)
{
    int32_t v = (int32_t)scale_ix(scale) - (int32_t)SCALE_LO + steps;

    while (v < 0)
        v += (int32_t)SCALE_N;
    return (uint8_t)(SCALE_LO + (uint32_t)v % SCALE_N);
}
#if defined(GENRE_HEAVY_PSYCH)
/* Chord/sub root walk: heavy_psych_data.h (heavy_psych_bass_walk_deg). */
#elif defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
/* Bass walk: house/funk_data.h */
#elif defined(GENRE_JAZZ)
/* Bass walk: jazz_data.h (jazz_bass_walk_deg). */
#elif defined(GENRE_SOUL)
/* Bass walk: soul_data.h (soul_bass_walk_deg). */
#else
/* Dub bass walk: dub_data.h (dub_bass_walk_deg). */
#endif

static uint8_t bank_pages(uint8_t bank)
{
    if (bank == BANK_LFO || bank == BANK_FX || bank == BANK_HOME)
        return 3u;
    if (bank == BANK_DRUM)
        return 3u;
#if defined(GENRE_BASS)
    if (bank == BANK_CHORD) /* KIT ENV pages + mod-slot CURVE */
        return FM_ENV_KIT_PAGES + 1u;
#else
    if (bank == BANK_ENV)
        return FM_ENV_KIT_PAGES;
    if (bank == BANK_GROOVE)
        return 1u;
#endif
    if (bank == BANK_CHORD || bank == BANK_BASS)
        return 3u;
    /* SEQ pages live in g.seq_page; 3 keeps draw_top_bar's REC dot clear of the SEQ page dots. */
    if (bank == BANK_SEQ)
        return 3u;
    return 2u;
}

/* Display only: 0 = unset → treat as center (64). DSP uses the same sentinel in dub_dsp.h.
 * Returns signed pan (-64..+63 style pn). Fills num (1..100) when pn != 0. */
static int32_t pan_parts(uint8_t pan, char *num)
{
    int32_t pn = (int32_t)(pan == 0u ? 64u : pan) - 64;
    uint32_t val;

    if (pn == 0) {
        num[0] = 0;
        return 0;
    }
    val = 1u + ((uint32_t)(pn < 0 ? -pn : pn) - 1u) * 99u / 62u;
    if (val > 100u)
        val = 100u;
    if (val >= 100u) {
        num[0] = '1';
        num[1] = '0';
        num[2] = '0';
        num[3] = 0;
    } else if (val >= 10u) {
        num[0] = (char)('0' + val / 10u);
        num[1] = (char)('0' + val % 10u);
        num[2] = 0;
    } else {
        num[0] = (char)('0' + val);
        num[1] = 0;
    }
    return pn;
}

static void fmt_pan(char *s, uint8_t pan)
{
    char num[4];
    int32_t pn = pan_parts(pan, num);
    char *p = s;

    if (pn == 0) {
        copy_str(s, "CTR");
        return;
    }
    if (pn < 0) {
        *p++ = 'L';
        *p++ = ' ';
        for (const char *c = num; *c;)
            *p++ = *c++;
        *p = 0;
        return;
    }
    for (const char *c = num; *c;)
        *p++ = *c++;
    *p++ = ' ';
    *p++ = 'R';
    *p = 0;
}

static uint8_t bank_page(void)
{
    uint8_t bank = g.bank % BANK_COUNT;
    return g.page[bank] % bank_pages(bank);
}

static int knob_slot_shows_pan(uint8_t slot)
{
    if (g.view == VIEW_SEQ || g.seq_plock_hold)
        return 0;
#if defined(GENRE_BASS)
    if (g.bank == BANK_BASS && bank_page() == 0u && slot == 3u)
        return 1;
#else
    if (g.bank == BANK_CHORD && bank_page() == 2u && slot == 1u)
        return 1;
    if (g.bank == BANK_BASS && bank_page() == 2u && slot == 1u)
        return 1;
#endif
    if (g.bank == BANK_DRUM && bank_page() == 2u && slot == 1u)
        return 1;
    return 0;
}

static uint8_t sel_drm_ix(void);

static int knob_slot_pan_raw(const dub_params_t *p, uint8_t slot, uint8_t *out)
{
    if (!knob_slot_shows_pan(slot))
        return 0;
    if (g.bank == BANK_CHORD)
        *out = p->pan[1];
    else if (g.bank == BANK_BASS)
        *out = p->pan[0];
    else if (g.bank == BANK_DRUM)
        *out = p->drm_pan[sel_drm_ix()];
    else
        return 0;
    return 1;
}

#if defined(GENRE_BASS)
/* Signed mod amount −127..127 → pot value with 0 exactly at the centre (64). */
static uint8_t knob_amount_pot(int32_t amount)
{
    return (uint8_t)(amount >= 0 ? 64 + amount * 63 / 127 : 64 + amount * 64 / 127);
}
#endif

/* Knob slots drawn as a centre-64 pot; *out = pot value. */
static int knob_slot_bipolar(const dub_params_t *p, uint8_t slot, uint8_t *out)
{
    uint8_t pg;

    if (knob_slot_pan_raw(p, slot, out)) {
        if (*out == 0u)
            *out = 64u;
        return 1;
    }
    if (g.view == VIEW_SEQ || g.seq_plock_hold)
        return 0;
    pg = bank_page();
    if (g.bank == BANK_FX && slot == 3u && pg < 2u) {
        *out = pg == 0u ? p->loop_filt : p->isolator;
        return 1;
    }
    if (g.bank == BANK_DRUM && pg == 0u && slot == 3u) {
        uint8_t di = sel_drm_ix();

        *out = p->drm_tune[di] ? p->drm_tune[di] : 64u;
        return 1;
    }
#if defined(GENRE_BASS)
    if (g.bank == BANK_HOME && pg >= 2u) {
        if ((fm1_in.buttons & (1u << BTN_EDIT)) != 0u) {
            if (slot != 3u)
                return 0;
            *out = knob_amount_pot(bass_mod_route_slot(&g.cfg.bass.mods, g.bass_rt.home_macro_sel)->amount);
            return 1;
        }
        {
            uint8_t *mp = bass_perf_ptr(slot);

            *out = mp ? *mp : 64u;
        }
        return 1;
    }
    if (g.bank == BANK_GROOVE && pg == 1u && slot == 2u) {
        /* TRANSP: raw 64 = 0 semitones, +/-BASS_TRANSPOSE_MAX = raw 127 / 1 (same clamp as the engine). */
        *out = (uint8_t)(64 + ((int32_t)bass_transpose_eff() * 63) / BASS_TRANSPOSE_MAX);
        return 1;
    }
    if (g.bank == BANK_CHORD && pg == 2u && slot == 3u) {
        *out = knob_amount_pot(g.cfg.bass.mods.env[g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP].amount);
        return 1;
    }
    if (g.bank == BANK_CHORD && pg == 1u && slot == 2u) {
        *out = bass_env_shape_curve(g.cfg.bass_env_shape);
        return 1;
    }
#endif
    return 0;
}

static uint8_t scale_deg_note(uint8_t root, uint8_t scale, uint8_t deg)
{
    uint8_t sid = scale_ix(scale);
    uint8_t len = SCALE_LEN[sid];
    uint8_t oct_add = deg / len;
    uint8_t d = deg % len;
    uint8_t oct = root / 12u;
    int32_t n = (int32_t)(oct + oct_add) * 12 + (int32_t)(root % 12u) + SCALE_SEMIS[sid][d];

    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    return (uint8_t)n;
}

/* Pitch class `pc` is a scale degree relative to tonal center `center_pc`. */
static int dub_root_pc_in_scale(uint8_t pc, uint8_t center_pc, uint8_t scale)
{
    uint8_t sid = scale_ix(scale);
    uint8_t len = SCALE_LEN[sid];
    uint8_t rel = (uint8_t)((pc + 12u - (center_pc % 12u)) % 12u);
    uint8_t d;

    for (d = 0; d < len; d++) {
        if (SCALE_SEMIS[sid][d] == rel)
            return 1;
    }
    return 0;
}

static uint8_t scaled_note(uint8_t root, uint8_t scale, uint8_t step)
{
#if defined(GENRE_HEAVY_PSYCH)
    return scale_deg_note(root, scale, heavy_psych_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_FUNK)
    return scale_deg_note(root, scale, funk_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_HOUSE)
    return scale_deg_note(root, scale, house_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_JAZZ)
    return scale_deg_note(root, scale, jazz_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_SOUL)
    return scale_deg_note(root, scale, soul_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_HIPHOP)
    return scale_deg_note(root, scale, hiphop_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_IDM)
    return scale_deg_note(root, scale, idm_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#elif defined(GENRE_TRIPHOP)
    {
        static const uint8_t th_walk[8] = { 0, 0, 2, 0, 4, 0, 2, 0 };
        return scale_deg_note(root, scale, th_walk[step & 7u]);
    }
#elif defined(GENRE_SNOWCRASH)
    {
        static const uint8_t sc_walk[8] = { 0, 0, 3, 0, 5, 0, 3, 0 };
        return scale_deg_note(root, scale, sc_walk[step & 7u]);
    }
#elif defined(GENRE_POP)
    {
        uint16_t pos = pop_music_pos(g.bar, g.bar_step);
        return pop_chord_root_midi(g.cfg.p.plate, pop_mbar(pos));
    }
#elif defined(GENRE_TECHNO) || defined(GENRE_BASS)
    {
        static const uint8_t techno_walk[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        return scale_deg_note(root, scale, techno_walk[step & 7u]);
    }
#else
    return scale_deg_note(root, scale, dub_bass_walk_deg(g.cfg.p.era, g.cfg.p.plate, step));
#endif
}

typedef struct {
    uint8_t tgt, depth, div, wave; /* div = FM_LFO_DIV_*, wave = DUB_LFO_* or shape CC > 4 */
} dub_lfo_era_step_t;

static const dub_lfo_era_step_t DUB_ERA_LFO[GENRE_ERA_N][3] = {
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    { {DUB_LFO_CUT_C, 22u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_WOW, 16u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_PAN_S, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_PAN_S, 12u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_PUMP_B, 8u, FM_LFO_DIV_1_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 26u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_RESO_C, 18u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_FB, 14u, FM_LFO_DIV_1_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 20u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 16u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_DRV_B, 12u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 24u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_CUT_B, 18u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_1_1, DUB_LFO_SQR} },
    { {DUB_LFO_TIMB_C, 20u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_RTAIL, 16u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_PAN_S, 18u, FM_LFO_DIV_1_1, DUB_LFO_SINE} },
#elif defined(GENRE_HEAVY_PSYCH)
    { {DUB_LFO_CUT_C, 36u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_DRV_B, 38u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_CUT_B, 32u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 40u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_ENV_C, 40u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_REV_C, 38u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 40u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_LVL_C, 40u, FM_LFO_DIV_4_1, DUB_LFO_SINE}, {DUB_LFO_ISO, 38u, FM_LFO_DIV_2_1, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_C, 42u, FM_LFO_DIV_4_1, DUB_LFO_SINE}, {DUB_LFO_SUB_B, 40u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_RTAIL, 38u, FM_LFO_DIV_2_1, DUB_LFO_TRI} },
    { {DUB_LFO_ENV_C, 42u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_REV_C, 40u, FM_LFO_DIV_4_1, DUB_LFO_TRI}, {DUB_LFO_TIMB_C, 36u, FM_LFO_DIV_1_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 38u, FM_LFO_DIV_1_1, DUB_LFO_TRI}, {DUB_LFO_DRV_B, 40u, FM_LFO_DIV_1_2, DUB_LFO_SQR}, {DUB_LFO_SRATE, 34u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
#elif defined(GENRE_HOUSE)
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_PUMP_B, 12u, FM_LFO_DIV_1_1, DUB_LFO_SQR}, {DUB_LFO_PAN_H, 10u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 20u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_2_1, DUB_LFO_SINE}, {DUB_LFO_PAN_S, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_B, 28u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_WOB, 22u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 18u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_RESO_C, 12u, FM_LFO_DIV_4_1, DUB_LFO_TRI}, {DUB_LFO_RTAIL, 14u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 30u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_ISO, 20u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_PUMP_B, 16u, FM_LFO_DIV_1_1, DUB_LFO_SQR} },
    { {DUB_LFO_CUT_B, 18u, FM_LFO_DIV_1_8, DUB_LFO_TRI}, {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 16u, FM_LFO_DIV_1_4, DUB_LFO_SQR} },
#elif defined(GENRE_JAZZ)
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_ISO, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_PAN_S, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 18u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_DRV_B, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 14u, FM_LFO_DIV_1_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_CUT_B, 18u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_RESO_C, 12u, FM_LFO_DIV_4_1, DUB_LFO_TRI} },
    { {DUB_LFO_WOB, 20u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_CUT_B, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_PAN_H, 14u, FM_LFO_DIV_1_4, DUB_LFO_SH}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_FB, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
#elif defined(GENRE_HIPHOP)
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE}, {DUB_LFO_PUMP_B, 18u, FM_LFO_DIV_1_1, DUB_LFO_SINE} },
    { {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_WOW, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 22u, FM_LFO_DIV_1_4, DUB_LFO_SQR}, {DUB_LFO_DRV_B, 16u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_2_1, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_B, 24u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_WOB, 20u, FM_LFO_DIV_1_4, DUB_LFO_SQR}, {DUB_LFO_DRV_B, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI} },
    { {DUB_LFO_WOB, 26u, FM_LFO_DIV_1_4, DUB_LFO_SQR}, {DUB_LFO_PUMP_B, 22u, FM_LFO_DIV_1_8, DUB_LFO_SINE}, {DUB_LFO_CUT_B, 16u, FM_LFO_DIV_2_1, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_C, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE}, {DUB_LFO_PAN_S, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_RTAIL, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
#elif defined(GENRE_IDM)
    { {DUB_LFO_CUT_C, 22u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 20u, FM_LFO_DIV_2_1, DUB_LFO_SH}, {DUB_LFO_WOW, 18u, FM_LFO_DIV_4_1, DUB_LFO_TRI} },
    { {DUB_LFO_TIMB_C, 24u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_CUT_C, 22u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_SRATE, 20u, FM_LFO_DIV_1_4, DUB_LFO_SH} },
    { {DUB_LFO_CUT_B, 28u, FM_LFO_DIV_1_8, DUB_LFO_SH}, {DUB_LFO_WOB, 30u, FM_LFO_DIV_1_4, DUB_LFO_STEP}, {DUB_LFO_PAN_H, 26u, FM_LFO_DIV_1_8T, DUB_LFO_SQR} },
    { {DUB_LFO_ISO, 24u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_CUT_C, 26u, FM_LFO_DIV_1_4, DUB_LFO_STEP}, {DUB_LFO_RTAIL, 22u, FM_LFO_DIV_2_1, DUB_LFO_SH} },
    { {DUB_LFO_WOB, 32u, FM_LFO_DIV_1_4, DUB_LFO_SQR}, {DUB_LFO_DRV_B, 34u, FM_LFO_DIV_1_8, DUB_LFO_STEP}, {DUB_LFO_CUT_B, 36u, FM_LFO_DIV_1_16, DUB_LFO_SH} },
    { {DUB_LFO_FB, 28u, FM_LFO_DIV_1_2, DUB_LFO_SH}, {DUB_LFO_WOW, 30u, FM_LFO_DIV_1_4T, DUB_LFO_STEP}, {DUB_LFO_PAN_S, 32u, FM_LFO_DIV_1_8, DUB_LFO_SQR} },
#elif defined(GENRE_SNOWCRASH)
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_FB, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_SRATE, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_DLY_C, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 20u, FM_LFO_DIV_1_8, DUB_LFO_SQR}, {DUB_LFO_SUB_B, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_1_4, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_TIMB_C, 18u, FM_LFO_DIV_1_4, DUB_LFO_SH}, {DUB_LFO_WOW, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 18u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_PUMP_B, 16u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_RTAIL, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_SRATE, 20u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_FB, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
#elif defined(GENRE_POP)
    { {DUB_LFO_DEC_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_DLY_C, 10u, FM_LFO_DIV_1_1, DUB_LFO_TRI}, {DUB_LFO_PAN_S, 8u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_SRATE, 16u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_WOB, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_CUT_C, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_SRATE, 18u, FM_LFO_DIV_1_8, DUB_LFO_SINE}, {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
    { {DUB_LFO_DLY_C, 20u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_FB, 12u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_CUT_B, 14u, FM_LFO_DIV_1_8, DUB_LFO_SQR} },
    { {DUB_LFO_ISO, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE}, {DUB_LFO_PAN_C, 14u, FM_LFO_DIV_1_1, DUB_LFO_TRI}, {DUB_LFO_RTAIL, 10u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_PUMP_B, 18u, FM_LFO_DIV_1_1, DUB_LFO_SQR}, {DUB_LFO_CUT_B, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_DEC_B, 14u, FM_LFO_DIV_1_4, DUB_LFO_SINE} },
#elif defined(GENRE_FUNK)
    { {DUB_LFO_CUT_C, 18u, FM_LFO_DIV_1_8, DUB_LFO_TRI}, {DUB_LFO_PAN_S, 12u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_TIMB_C, 20u, FM_LFO_DIV_1_16, 45u}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_8, DUB_LFO_TRI}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 22u, FM_LFO_DIV_1_8, DUB_LFO_SQR}, {DUB_LFO_WOB, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_PAN_S, 12u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_PUMP_B, 22u, FM_LFO_DIV_1_1, DUB_LFO_SQR}, {DUB_LFO_LVL_C, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 20u, FM_LFO_DIV_1_8, DUB_LFO_TRI}, {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_C, 18u, FM_LFO_DIV_1_8T, 45u}, {DUB_LFO_PAN_H, 16u, FM_LFO_DIV_1_4, DUB_LFO_SH}, {DUB_LFO_FB, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
#elif defined(GENRE_TECHNO)
    { {DUB_LFO_CUT_C, 18u, FM_LFO_DIV_1_8, DUB_LFO_TRI}, {DUB_LFO_WOB, 16u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 24u, FM_LFO_DIV_1_4, DUB_LFO_SQR}, {DUB_LFO_CUT_B, 18u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 28u, FM_LFO_DIV_1_16, DUB_LFO_SQR}, {DUB_LFO_DRV_B, 20u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_WOB, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI} },
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_4, DUB_LFO_SINE}, {DUB_LFO_ISO, 18u, FM_LFO_DIV_1_1, DUB_LFO_TRI}, {DUB_LFO_RTAIL, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_C, 18u, FM_LFO_DIV_1_8, DUB_LFO_SINE}, {DUB_LFO_PAN_H, 14u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_FB, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 24u, FM_LFO_DIV_1_8, DUB_LFO_SQR}, {DUB_LFO_CUT_B, 22u, FM_LFO_DIV_1_16, 45u}, {DUB_LFO_PUMP_B, 18u, FM_LFO_DIV_1_4, DUB_LFO_SQR} },
#elif defined(GENRE_BASS)
    /* Only targets in the BASS LFO dest list (dub_lfo_order_build): no CUT_C/TIMB_C/SRATE (no knob in KO BASS). */
    { {DUB_LFO_WOB, 26u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_CUT_B, 20u, FM_LFO_DIV_1_16, DUB_LFO_SQR}, {DUB_LFO_DRV_B, 14u, FM_LFO_DIV_1_4, DUB_LFO_TRI} },
    { {DUB_LFO_SUB_B, 18u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_CUT_B, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_WOW, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 22u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_CUT_B, 16u, FM_LFO_DIV_1_2, DUB_LFO_SINE}, {DUB_LFO_RESO_C, 18u, FM_LFO_DIV_1_8, DUB_LFO_TRI} },
    { {DUB_LFO_ENV_C, 20u, FM_LFO_DIV_1_8, DUB_LFO_SINE}, {DUB_LFO_WOB, 18u, FM_LFO_DIV_1_4, DUB_LFO_TRI}, {DUB_LFO_CUT_B, 14u, FM_LFO_DIV_1_2, DUB_LFO_SINE} },
    { {DUB_LFO_WOB, 24u, FM_LFO_DIV_1_16, DUB_LFO_SQR}, {DUB_LFO_DRV_B, 18u, FM_LFO_DIV_1_8, 45u}, {DUB_LFO_CUT_B, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI} },
    { {DUB_LFO_DRV_B, 22u, FM_LFO_DIV_1_8, DUB_LFO_SQR}, {DUB_LFO_CUT_B, 16u, FM_LFO_DIV_1_4, 45u}, {DUB_LFO_WOB, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI} },
#elif defined(GENRE_TRIPHOP)
    { {DUB_LFO_CUT_C, 16u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_WOW, 14u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_PAN_S, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_WOW, 16u, FM_LFO_DIV_2_1, DUB_LFO_SINE}, {DUB_LFO_CUT_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_RTAIL, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_ISO, 16u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_CUT_C, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI}, {DUB_LFO_PAN_H, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_TIMB_C, 14u, FM_LFO_DIV_1_1, DUB_LFO_SINE}, {DUB_LFO_LVL_C, 12u, FM_LFO_DIV_2_1, DUB_LFO_TRI}, {DUB_LFO_REV_C, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_CUT_B, 20u, FM_LFO_DIV_1_2, 45u}, {DUB_LFO_DRV_B, 16u, FM_LFO_DIV_1_1, DUB_LFO_TRI}, {DUB_LFO_RTAIL, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
    { {DUB_LFO_PAN_S, 14u, FM_LFO_DIV_1_1, DUB_LFO_TRI}, {DUB_LFO_DLY_C, 16u, FM_LFO_DIV_2_1, DUB_LFO_SINE}, {DUB_LFO_CUT_C, 12u, FM_LFO_DIV_4_1, DUB_LFO_SINE} },
#endif
};

/* HOME macros: SKANK, LAYER, WARP, SWEEP, SPACE, WEIGHT, GRIT, DUB, MOTION, FLUX */
#ifdef GENRE_HEAVY_PSYCH
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS HEAVY_PSYCH_ERA_MACROS
#elif defined(GENRE_HOUSE)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS HOUSE_ERA_MACROS
#elif defined(GENRE_JAZZ)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS JAZZ_ERA_MACROS
#elif defined(GENRE_SOUL)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS SOUL_ERA_MACROS
#elif defined(GENRE_HIPHOP)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS HIPHOP_ERA_MACROS
#elif defined(GENRE_IDM)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS IDM_ERA_MACROS
#elif defined(GENRE_SNOWCRASH)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS SNOWCRASH_ERA_MACROS
#elif defined(GENRE_FUNK)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS FUNK_ERA_MACROS
#elif defined(GENRE_TECHNO)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS TECHNO_ERA_MACROS
#elif defined(GENRE_BASS)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS BASS_ERA_MACROS
#elif defined(GENRE_TRIPHOP)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS TRIPHOP_ERA_MACROS
#elif defined(GENRE_POP)
#undef DUB_ERA_MACROS
#define DUB_ERA_MACROS POP_ERA_MACROS
#else
static const uint8_t DUB_ERA_MACROS[GENRE_ERA_N][10] = {
    { 68u, 48u, 12u, 64u, 52u, 44u, 28u, 74u,  0u, 24u }, /* KINGSTON */
    { 54u, 72u, 18u, 56u, 86u, 48u, 20u, 64u, 18u, 46u }, /* CHANNEL */
    { 42u, 44u, 48u, 60u, 46u, 78u, 66u, 48u,  0u, 32u }, /* BRISTOL */
    { 28u, 36u, 88u, 48u, 38u, 86u, 62u, 34u,  0u, 22u }, /* CROYDON */
    { 46u, 64u, 34u, 72u, 88u, 36u, 24u, 68u, 28u, 82u }, /* POSTDUB */
};
#endif


typedef struct {
    uint8_t vary[6];
    int8_t vel[6];
    int8_t trig[6];
    int8_t gate[6];
    uint8_t siren_cond;
    uint8_t rim_cond;
    uint8_t chord_cond;
    uint8_t ghost_hat;
    uint8_t ghost_rim;
} dub_plate_seq_t;

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static const dub_plate_seq_t DUB_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,12,18,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,14,20,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,10,16,0,4,6}, {0,-4,0,0,0,-6}, {0,-12,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,12,18,0,4,10}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,18,24,0,8,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,0,0,3,0 },
    { {0,14,20,0,4,10}, {0,0,-6,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,12,16,0,6,8}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,14,0,4,6}, {0,0,0,0,0,-4}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,0,3,0 },
    { {0,12,16,0,4,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 11,0,3,3,0 },
    { {0,10,14,0,4,6}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,0,3,3 },
    { {0,8,12,0,4,8}, {0,0,0,0,0,-8}, {0,0,-4,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,0,3,0,0 },
    { {0,10,16,0,4,8}, {0,0,-6,0,0,-8}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,0,3,3 },
    { {0,12,14,0,4,6}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 11,0,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-8,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-10}, 10,0,3,0,0 },
    { {0,10,16,0,6,6}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,4,-6,0,-8}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,16,20,0,6,10}, {0,0,4,0,0,-6}, {0,-10,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,3 },
    { {0,14,18,0,8,12}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 8,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,-8,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-8}, 11,0,0,3,0 },
    { {0,10,14,0,6,8}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,0,0 },
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,3,0 },
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,0,3 },
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,3,0 },
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,0,0 },
    { {0,10,16,0,6,12}, {0,0,0,0,0,10}, {0,0,0,0,0,0}, {0,0,6,-6,0,16}, 10,0,3,3,0 },
    { {0,8,14,0,4,14}, {0,0,-4,0,0,8}, {0,-8,0,0,0,0}, {0,0,4,-8,0,14}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_HEAVY_PSYCH)
static const dub_plate_seq_t HEAVY_PSYCH_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,0,0,0,0,8},  {0,0,-4,0,0,-6},  {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,0,0 },
    { {0,0,0,0,0,10}, {0,-4,0,0,0,-8},  {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,0,0 },
    { {0,0,0,0,0,6},  {0,0,4,0,0,-6},   {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,0,0 },
    { {0,0,0,0,0,8},  {0,0,0,0,4,-4},   {0,0,0,0,0,0}, {0,0,-6,-8,0,-6},  9,0,0,0,0 },
    { {0,0,0,0,0,12}, {0,0,0,0,0,-10},  {0,0,0,0,0,0}, {0,0,-4,-6,0,-8},  8,0,3,0,0 },
    { {0,0,0,0,0,10}, {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,0,3,0,0 },
    { {0,0,0,0,0,10}, {0,-4,0,0,0,-8},  {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,0,0 },
    { {0,0,0,0,0,12}, {0,0,0,0,4,-6},   {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,0,-12},  {0,0,0,0,0,0}, {0,0,4,-6,0,-8},  11,0,3,0,0 },
    { {0,0,0,0,0,8},  {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-6},   9,0,0,0,0 },
    { {0,0,0,0,0,10}, {0,0,0,0,0,-8},   {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,0,3,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,4,-6,0,-8},  11,0,0,0,0 },
    { {0,0,0,0,0,8},  {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10},  9,0,3,0,0 },
    { {0,0,0,0,0,12}, {0,0,0,0,0,-12},  {0,0,0,0,0,0}, {0,0,6,-6,0,-8},   8,0,0,0,0 },
    { {0,0,0,0,0,8},  {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8},  11,0,3,0,0 },
    { {0,0,0,0,0,10}, {0,0,0,0,0,-8},   {0,0,0,0,0,0}, {0,0,4,-6,0,-8},  10,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,0,0,0,0,8},  {0,0,4,0,0,-6},   {0,0,0,0,0,0}, {0,0,-4,-6,0,-8},  9,0,0,0,0 },
    { {0,0,0,0,0,10}, {0,-4,0,0,4,-8},  {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,0 },
    { {0,0,0,0,0,12}, {0,0,0,0,0,-10},  {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,0,0 },
    { {0,0,0,0,0,8},  {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,-4,-6,0,-8},  9,0,3,0,0 },
    { {0,0,0,0,0,6},  {0,0,6,0,0,-4},   {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,0,0 },
    { {0,0,0,0,0,10}, {0,0,0,0,4,-8},   {0,0,0,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,0,0 },
    { {0,0,0,0,0,10}, {0,-4,0,0,4,-8},  {0,0,0,0,0,0}, {0,0,-6,-8,0,-6},  9,0,0,0,0 },
    { {0,0,0,0,0,12}, {0,0,6,0,0,-10},  {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,0,0,0,0,4},  {0,0,-6,0,0,-4},  {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,0,0,0,0 },
    { {0,0,0,0,0,4},  {0,0,-4,0,0,-4},  {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,0,-8},   {0,0,0,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,0,0 },
    { {0,0,0,0,0,8},  {0,-4,0,0,0,-6},  {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,0,0,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,4,-4},   {0,0,0,0,0,0}, {0,0,-8,-8,0,-8},  8,0,3,0,0 },
    { {0,0,0,0,0,8},  {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,-4,-6,0,-8},  9,0,3,0,0 },
    { {0,0,0,0,0,10}, {0,0,4,0,0,-8},   {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,0,0,0,0,14}, {0,0,0,0,0,8},    {0,0,0,0,0,0}, {0,0,4,-4,0,16},   9,0,3,0,0 },
    { {0,0,0,0,0,18}, {0,0,4,0,0,12},   {0,0,0,0,0,0}, {0,0,6,-6,0,20},  10,0,0,0,0 },
    { {0,0,0,0,0,12}, {0,0,0,0,0,6},    {0,0,0,0,0,0}, {0,0,2,-8,0,14},  11,0,3,0,0 },
    { {0,0,0,0,0,16}, {0,0,0,0,4,10},   {0,0,0,0,0,0}, {0,0,8,-4,0,18},   9,0,3,0,0 },
    { {0,0,0,0,0,10}, {0,0,-4,0,0,8},   {0,0,0,0,0,0}, {0,0,4,-6,0,12},  10,0,3,0,0 },
    { {0,0,0,0,0,20}, {0,0,0,0,0,14},   {0,0,0,0,0,0}, {0,0,6,-8,0,20},   8,0,0,0,0 },
    { {0,0,0,0,0,8},  {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,-4,-6,0,-8},  9,0,3,0,0 },
    { {0,0,0,0,0,16}, {0,0,4,0,0,10},   {0,0,0,0,0,0}, {0,0,8,-6,0,18},  10,0,0,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,0,0,0,0,4},  {0,0,-8,0,0,-4},  {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,0,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,0,-6},   {0,0,0,0,0,0}, {0,0,-8,-10,0,-8}, 10,0,0,0,0 },
    { {0,0,0,0,0,4},  {0,0,-6,0,0,-4},  {0,0,0,0,0,0}, {0,0,-12,-8,0,-6}, 11,0,3,0,0 },
    { {0,0,0,0,0,4},  {0,0,0,0,0,-8},   {0,0,0,0,0,0}, {0,0,-10,-8,0,-8},  9,0,0,0,0 },
    { {0,0,0,0,0,6},  {0,-4,0,0,0,-6},  {0,0,0,0,0,0}, {0,0,-12,-6,0,-6}, 10,0,0,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,4,-4},   {0,0,0,0,0,0}, {0,0,-10,-8,0,-8},  8,0,3,0,0 },
    { {0,0,0,0,0,4},  {0,0,-6,0,0,-4},  {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,0 },
    { {0,0,0,0,0,6},  {0,0,0,0,0,-8},   {0,0,0,0,0,0}, {0,0,-8,-8,0,-8},  10,0,0,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_HOUSE) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
static const dub_plate_seq_t HOUSE_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,10,16,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,14,0,4,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,14,20,0,6,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,10,16,0,4,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,16,22,0,8,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,3,3,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,14,20,0,4,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8}, 11,0,3,0,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,0,-8}, 10,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,3 },
    { {0,16,22,0,6,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,16,0,6,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,3,3,0 },
    { {0,12,18,0,8,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,0,0 },
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,3,0 },
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,0,3 },
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,3,0 },
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,0,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,0,18}, 10,3,0,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,0,-8}, 10,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_FUNK)
static const dub_plate_seq_t FUNK_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,10,16,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,14,0,4,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,14,20,0,6,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,10,16,0,4,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,16,22,0,8,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,3,3,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,14,20,0,4,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8}, 11,0,3,0,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,0,-8}, 10,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,3 },
    { {0,16,22,0,6,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,16,0,6,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,3,3,0 },
    { {0,12,18,0,8,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,0,0 },
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,3,0 },
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,0,3 },
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,3,0 },
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,0,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,0,18}, 10,3,0,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,0,-8}, 10,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_SOUL)
static const dub_plate_seq_t SOUL_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,11,17,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,14,0,4,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,14,20,0,6,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,11,17,0,4,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,16,22,0,8,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,3,3,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,14,20,0,4,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,11,17,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8}, 11,0,3,0,0 },
    { {0,11,17,0,6,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,0,-8}, 10,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,3 },
    { {0,16,22,0,6,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,16,0,6,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,3,3,0 },
    { {0,12,18,0,8,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,0,0 },
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,3,0 },
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,0,3 },
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,3,0 },
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,0,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,0,18}, 10,3,0,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,0,-8}, 10,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_JAZZ)
static const dub_plate_seq_t JAZZ_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,10,16,0,7,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,-10,-10}, 0,0,0,3,0 },
    { {0,12,18,0,8,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,-12,-8}, 3,0,3,3,0 },
    { {0,8,14,0,9,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,-10,-12}, 0,0,0,3,3 },
    { {0,14,20,0,7,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,-12,-6}, 3,3,0,3,0 },
    { {0,10,16,0,8,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,-10,-8}, 0,0,3,0,3 },
    { {0,16,22,0,9,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,-12,-10}, 0,3,3,3,0 },
    { {0,12,18,0,7,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,-10,-8}, 3,0,3,3,0 },
    { {0,14,20,0,8,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,-12,-10}, 3,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,6,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,-11,-8}, 0,0,3,0,0 },
    { {0,6,12,0,7,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,-13,-6}, 3,0,0,3,0 },
    { {0,10,16,0,8,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,-11,-12}, 3,3,3,0,0 },
    { {0,8,12,0,6,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,-13,-8}, 0,0,0,3,0 },
    { {0,12,18,0,7,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,-11,-10}, 3,0,3,3,0 },
    { {0,6,10,0,8,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,-13,-8}, 0,3,0,0,3 },
    { {0,8,14,0,6,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,-11,-8}, 0,0,3,0,0 },
    { {0,10,16,0,7,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,-13,-8}, 3,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,8,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,-12,-8}, 3,3,0,3,3 },
    { {0,14,20,0,9,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,-14,-6}, 0,0,3,3,0 },
    { {0,18,24,0,10,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,-12,-12}, 0,0,0,3,0 },
    { {0,12,16,0,8,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,-14,-8}, 3,3,3,0,3 },
    { {0,20,26,0,9,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,-12,-6}, 0,0,0,3,0 },
    { {0,14,18,0,10,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,-14,-10}, 3,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,-12,-6}, 0,3,0,3,3 },
    { {0,16,22,0,9,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,-14,-12}, 0,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,5,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,-6,-6}, 7,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,-8,-8}, 5,3,0,3,0 },
    { {0,6,10,0,7,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,-6,-6}, 3,0,3,0,3 },
    { {0,8,12,0,5,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,-8,-8}, 0,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,-6,-6}, 5,3,0,3,0 },
    { {0,8,14,0,7,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,-8,-8}, 7,0,3,0,0 },
    { {0,10,16,0,5,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,-6,-8}, 3,0,3,3,0 },
    { {0,12,18,0,6,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,-8,-6}, 5,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,3,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,-4,16}, 5,0,3,3,0 },
    { {0,12,18,0,4,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,-6,20}, 7,3,0,0,0 },
    { {0,8,14,0,5,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,-4,14}, 0,0,3,3,0 },
    { {0,14,20,0,3,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,-6,18}, 3,0,3,0,3 },
    { {0,10,16,0,4,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,-4,12}, 7,3,3,3,0 },
    { {0,12,18,0,5,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,-6,20}, 5,0,0,0,0 },
    { {0,10,16,0,3,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,-4,16}, 3,0,3,3,0 },
    { {0,12,18,0,4,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,-6,18}, 0,3,0,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,9,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,-14,-6}, 3,0,0,3,0 },
    { {0,10,14,0,10,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,-16,-8}, 5,3,0,3,0 },
    { {0,6,10,0,11,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,-14,-6}, 7,0,3,0,3 },
    { {0,8,12,0,9,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,-16,-8}, 0,0,0,3,0 },
    { {0,12,16,0,10,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,-14,-6}, 5,3,0,3,0 },
    { {0,8,14,0,11,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,-16,-8}, 3,0,3,0,0 },
    { {0,10,14,0,9,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,-14,-8}, 7,3,0,3,0 },
    { {0,8,12,0,10,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,-16,-6}, 5,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static const dub_plate_seq_t IDM_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,10,16,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,14,0,4,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,14,20,0,6,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,10,16,0,4,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,16,22,0,8,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,3,3,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,14,20,0,4,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8}, 11,0,3,0,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,0,-8}, 10,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,3 },
    { {0,16,22,0,6,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,16,0,6,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,3,3,0 },
    { {0,12,18,0,8,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,0,0 },
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,3,0 },
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,0,3 },
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,3,0 },
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,0,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,0,16}, 9,0,3,3,0 },
    { {0,12,18,0,8,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,0,18}, 10,3,0,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,0,-8}, 10,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_POP)
static const dub_plate_seq_t POP_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,11,17,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,14,0,4,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,14,20,0,6,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,10,16,0,4,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,16,22,0,8,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,3,3,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,14,20,0,4,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8}, 11,0,3,0,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,0,-8}, 10,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,3 },
    { {0,16,22,0,6,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,16,0,6,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,3,3,0 },
    { {0,12,18,0,8,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,7,0},
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,7,0},
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,7,0},
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,7,3},
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,7,0},
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,7,0},
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,0,16}, 9,0,3,7,0},
    { {0,12,18,0,8,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,0,18}, 10,3,0,7,3},
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,0,-8}, 10,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#elif defined(GENRE_HIPHOP)
static const dub_plate_seq_t HIPHOP_PLATE_SEQ[GENRE_PRESET_N] = {
    { {0,10,16,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,0,0,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,8,14,0,4,6}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,3 },
    { {0,14,20,0,6,8}, {0,0,0,0,4,-4}, {0,0,-6,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,0 },
    { {0,10,16,0,4,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-4,-6,0,-8}, 8,0,3,0,3 },
    { {0,16,22,0,8,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,3,3,3,0 },
    { {0,12,18,0,6,10}, {0,-4,0,0,0,-8}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },
    { {0,14,20,0,4,12}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 9,3,3,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,0,-12}, {0,0,0,0,0,0}, {0,0,4,-6,0,-8}, 11,0,3,0,0 },
    { {0,6,12,0,4,8}, {0,0,-6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,2,-8,0,-6}, 9,0,0,3,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,0,0,0,0,0}, {0,0,6,-10,0,-12}, 10,3,3,0,0 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,4,-6,0,-8}, 11,0,0,3,0 },
    { {0,12,18,0,6,8}, {0,-4,0,0,0,-10}, {0,0,0,0,0,0}, {0,0,2,-8,0,-10}, 9,0,3,3,0 },
    { {0,6,10,0,4,12}, {0,0,0,0,0,-12}, {0,-8,0,0,0,0}, {0,0,6,-6,0,-8}, 8,3,0,0,3 },
    { {0,8,14,0,4,8}, {0,0,-6,0,0,-10}, {0,0,0,0,0,0}, {0,0,6,-8,0,-8}, 11,0,3,0,0 },
    { {0,10,16,0,6,10}, {0,0,0,0,0,-8}, {0,-8,0,0,0,0}, {0,0,4,-6,0,-8}, 10,3,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,16,22,0,6,8}, {0,0,4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,-10,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,3,0 },
    { {0,18,24,0,6,12}, {0,0,0,0,0,-10}, {0,0,-8,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,0,3,0 },
    { {0,12,16,0,4,8}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,3,0,3 },
    { {0,20,26,0,8,6}, {0,0,6,0,0,-4}, {0,-12,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,14,18,0,6,10}, {0,0,0,0,4,-8}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-10}, 8,0,3,3,3 },
    { {0,14,20,0,8,10}, {0,-4,0,0,4,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 9,3,0,3,3 },
    { {0,16,22,0,6,12}, {0,0,6,0,0,-10}, {0,-10,0,0,0,0}, {0,0,-8,-10,0,-12}, 11,0,3,0,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 11,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-10,-8,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 10,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-10,0,0,0}, {0,0,-6,-8,0,-8}, 11,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-10,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-10,0,0,0,0}, {0,0,-8,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,16,0,6,8}, {0,0,0,0,0,-6}, {0,0,-6,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,3,3,0 },
    { {0,12,18,0,8,10}, {0,0,4,0,0,-8}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,3,0,3,0 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,4,-4,0,16}, 9,0,3,7,0},
    { {0,12,18,0,8,18}, {0,0,4,0,0,12}, {0,0,0,0,0,0}, {0,0,6,-6,0,20}, 10,3,0,7,0},
    { {0,8,14,0,4,12}, {0,0,0,0,0,6}, {0,-8,0,0,0,0}, {0,0,2,-8,0,14}, 11,0,3,7,0},
    { {0,14,20,0,6,16}, {0,0,0,0,4,10}, {0,0,0,0,0,0}, {0,0,8,-4,0,18}, 9,0,3,7,3},
    { {0,10,16,0,6,10}, {0,0,-4,0,0,8}, {0,0,-6,0,0,0}, {0,0,4,-6,0,12}, 10,3,3,7,0},
    { {0,12,18,0,8,20}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,6,-8,0,20}, 8,0,0,7,0},
    { {0,10,16,0,6,14}, {0,0,0,0,0,8}, {0,0,0,0,0,0}, {0,0,6,-4,0,16}, 9,0,3,7,0},
    { {0,12,18,0,8,16}, {0,0,4,0,0,10}, {0,-6,0,0,0,0}, {0,0,8,-6,0,18}, 10,3,0,7,3},
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },
    { {0,8,12,0,4,4}, {0,0,-4,0,0,-4}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,0,3,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,6,10,0,4,4}, {0,0,-6,0,0,-4}, {0,0,0,0,0,0}, {0,0,-10,-8,0,-6}, 11,0,3,0,3 },
    { {0,8,12,0,4,6}, {0,0,0,0,0,-8}, {0,0,-8,0,0,0}, {0,0,-6,-8,0,-8}, 10,0,0,3,0 },
    { {0,12,16,0,6,8}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-8,-6,0,-6}, 9,3,0,3,0 },
    { {0,8,14,0,4,6}, {0,0,0,0,4,-4}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-8}, 8,0,3,0,0 },
    { {0,10,14,0,6,6}, {0,0,0,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-8,-6,0,-8}, 9,3,0,3,0 },
    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-6}, 10,0,3,0,3 },
    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },
    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },

};
#endif

static void dub_macro_morph_begin(void);
static uint8_t sel_drm_ix(void);

static void dub_macro_lfo_apply_pick(const uint8_t pick[7], uint8_t bump, uint8_t plate)
{
    fm_lfo_slot_t *sl;
    uint8_t d;
    uint8_t cap;
    uint32_t bpm;

    if (pick[0] == 0xFFu)
        return;
    sl = &g.lfo_slots[pick[0]];
    d = (uint8_t)(pick[1] + (bump >> 2));
#if defined(GENRE_HEAVY_PSYCH)
    cap = (pick[4] == (uint8_t)FM_LFO_ALG_CLASSIC) ? 44u : 28u;
#elif defined(GENRE_IDM)
    cap = (pick[4] == (uint8_t)FM_LFO_ALG_CLASSIC) ? 40u : 32u;
#else
    cap = (pick[4] == (uint8_t)FM_LFO_ALG_CLASSIC) ? 28u : 20u;
#endif
    if (d > cap)
        d = cap;
    bpm = g.cfg.p.bpm ? g.cfg.p.bpm : 120u;
    sl->depth = d;
    sl->rate = fm_lfo_rate_from_div(pick[2], bpm);
    sl->wave = fm_lfo_legacy_shape(pick[3]);
    sl->algo = pick[4];
    sl->index = pick[5];
    sl->ratio = pick[6];
    sl->phase = (uint8_t)(((plate % GENRE_PRESETS_PER_ERA) * 13u + (pick[0] & 7u) * 16u) & 127u);
    sl->fb = (pick[4] == (uint8_t)FM_LFO_ALG_CROSS) ? 18u : 6u;
    sl->fold = (pick[4] == (uint8_t)FM_LFO_ALG_FOLD) ? 40u : 0u;
}

/* CPL shares: g.lfo_slots[cpl].depth (0..127) = share of one macro/MOTION/FLUX term into one sound/FX param.
 * Rate/wave of CPL slots are unused. Multi-term couplings (FLUX, MOTION GLIDE) use one combined source S per target
 * (dub_s_*() in dub_types.h, old ratios kept) and one share: new = dub_cpl_scale(S, share). */
typedef struct {
    uint8_t cpl;  /* DUB_LFO_CPL_* */
    uint8_t dst;  /* DUB_LFO_* target whose knob shows the icon */
    uint8_t src;  /* DUB_LFO_M_* source; M_MOTION = MOTION eff (active if > 0), M_FLUX = engine FLUX eff (active if > 0) */
    uint8_t dflt; /* default depth = old factor * 128 (>>1 = 64, >>2 = 32, *3/4 = 96, /3 = 43, *1 = 127) */
} dub_cpl_map_t;

static const dub_cpl_map_t dub_cpl_map[] = {
    { DUB_LFO_CPL_FB_MFEED,    DUB_LFO_FB,      DUB_LFO_M_FEED,   127u },
    { DUB_LFO_CPL_FB_MDUB,     DUB_LFO_FB,      DUB_LFO_M_DUB,     64u },
    { DUB_LFO_CPL_FB_MSPACE,   DUB_LFO_FB,      DUB_LFO_M_SPACE,   32u },
    { DUB_LFO_CPL_WOW_MFEED,   DUB_LFO_WOW,     DUB_LFO_M_FEED,    32u },
#if !defined(GENRE_POP)
    { DUB_LFO_CPL_WOW_MGRIT,   DUB_LFO_WOW,     DUB_LFO_M_GRIT,    64u },
    { DUB_LFO_CPL_TANK_MSPACE, DUB_LFO_TANK,    DUB_LFO_M_SPACE,   64u },
#endif
    { DUB_LFO_CPL_REV_MFEED,   DUB_LFO_REV,     DUB_LFO_M_FEED,    32u },
#if !defined(GENRE_POP)
    { DUB_LFO_CPL_REV_MSPACE,  DUB_LFO_REV,     DUB_LFO_M_SPACE,   96u },
#endif
    { DUB_LFO_CPL_LOOP_MDUB,   DUB_LFO_LOOP,    DUB_LFO_M_DUB,     64u },
#if !defined(GENRE_BASS)
    { DUB_LFO_CPL_MOT_PANB,    DUB_LFO_PAN_B,   DUB_LFO_M_MOTION,  43u },
    { DUB_LFO_CPL_MOT_PANC,    DUB_LFO_PAN_C,   DUB_LFO_M_MOTION,  32u },
#if !defined(GENRE_POP)
    { DUB_LFO_CPL_MOT_GLIDE,   DUB_LFO_GLIDE_C, DUB_LFO_M_MOTION, 127u },
#endif
    { DUB_LFO_CPL_FB_FLUX,     DUB_LFO_FB,      DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_WOW_FLUX,    DUB_LFO_WOW,     DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_REV_FLUX,    DUB_LFO_REV,     DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_LOOP_FLUX,   DUB_LFO_LOOP,    DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_FLX_CUTC,    DUB_LFO_CUT_C,   DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_FLX_CUTB,    DUB_LFO_CUT_B,   DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_FLX_WOB,     DUB_LFO_WOB,     DUB_LFO_M_FLUX,   127u },
    /* DRUM PAN FLUX: one share for all four drum pans, icon on each drum pan knob */
    { DUB_LFO_CPL_FLX_DRMPAN,  DUB_LFO_PAN_K,   DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_FLX_DRMPAN,  DUB_LFO_PAN_R,   DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_FLX_DRMPAN,  DUB_LFO_PAN_H,   DUB_LFO_M_FLUX,   127u },
    { DUB_LFO_CPL_FLX_DRMPAN,  DUB_LFO_PAN_S,   DUB_LFO_M_FLUX,   127u },
#endif
};

/* Scaled share of x by the CPL slot depth (replaces the old hardcoded factor). */
static int32_t dub_cpl_share(int32_t x, uint8_t cpl)
{
    return dub_cpl_scale(x, g.lfo_slots[cpl].depth);
}

/* Genre defaults for all CPL ids; ids not in dub_cpl_map stay 0. */
static void dub_cpl_defaults_fill(uint8_t *out)
{
    uint8_t ci;

    for (ci = 0u; ci < DUB_CPL_N; ci++)
        out[ci] = 0u;
    for (ci = 0u; ci < (uint8_t)(sizeof dub_cpl_map / sizeof dub_cpl_map[0]); ci++)
        out[dub_cpl_map[ci].cpl - DUB_LFO_TGT_PRE_CPL] = dub_cpl_map[ci].dflt;
}

static void dub_cpl_apply_defaults(void)
{
    uint8_t d[DUB_CPL_N];
    uint8_t ci;

    dub_cpl_defaults_fill(d);
    for (ci = 0u; ci < DUB_CPL_N; ci++)
        g.lfo_slots[DUB_LFO_TGT_PRE_CPL + ci].depth = d[ci];
}

/* Runtime CPL depths -> saved tail (called before settings_save and after cfg defaults). */
static void dub_cpl_store(void)
{
    uint8_t ci;

    for (ci = 0u; ci < DUB_CPL_N; ci++)
        g.cfg.lfo_cpl[ci] = g.lfo_slots[DUB_LFO_TGT_PRE_CPL + ci].depth;
}

/* Saved tail -> runtime CPL depths (called in dub_persist_unpack). */
static void dub_cpl_load(void)
{
    uint8_t ci;

    for (ci = 0u; ci < DUB_CPL_N; ci++)
        g.lfo_slots[DUB_LFO_TGT_PRE_CPL + ci].depth = g.cfg.lfo_cpl[ci];
}

static void dub_era_lfo_apply(uint8_t era, uint8_t plate)
{
    const dub_lfo_era_step_t *steps;
    uint8_t i;
    uint8_t bump;
    uint32_t bpm;

    if (fm_era_is_artist(era)) {
        era = genre_era_of_preset(plate);
        plate = genre_clamp_preset(plate);
    } else {
        era = genre_clamp_era(era);
        plate = genre_clamp_preset(plate);
    }
    bump = (uint8_t)((plate % GENRE_PRESETS_PER_ERA) + 2u);
    bpm = g.cfg.p.bpm ? g.cfg.p.bpm : 120u;
    steps = DUB_ERA_LFO[era];
    for (i = 0; i < 3u; i++) {
        fm_lfo_slot_t *sl;
        uint8_t d;
#if defined(GENRE_HEAVY_PSYCH)
        const uint8_t era_lfo_depth_cap = 52u;
#elif defined(GENRE_IDM)
        const uint8_t era_lfo_depth_cap = 48u;
#else
        const uint8_t era_lfo_depth_cap = 32u;
#endif

        if (steps[i].depth == 0u)
            continue;
        sl = &g.lfo_slots[steps[i].tgt];
        d = (uint8_t)(steps[i].depth + (i == 0u ? bump : (bump >> 1)));
        if (d > era_lfo_depth_cap)
            d = era_lfo_depth_cap;
        sl->depth = d;
        sl->rate = fm_lfo_rate_from_div(steps[i].div, bpm);
        sl->wave = fm_lfo_legacy_shape(steps[i].wave);
        sl->algo = FM_LFO_ALG_CLASSIC;
        sl->phase = (uint8_t)(((plate % GENRE_PRESETS_PER_ERA) * 13u + i * 32u) & 127u);
#if defined(GENRE_JAZZ)
        if (i == 0u)
            sl->skew = 76u;
#endif
    }
    /* HOME macro LFO: one per era (HP/IDM: two with odd rate pairs). */
    {
#if defined(GENRE_HOUSE)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_SPACE, 20u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_ISO, 22u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_WOB, 20u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
        };
#elif defined(GENRE_HEAVY_PSYCH)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][14] = {
            { DUB_LFO_M_WOB, 40u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_GRIT, 42u, FM_LFO_DIV_1_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 40u, FM_LFO_DIV_1_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_DUB, 42u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_MOTION, 40u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_WEIGHT, 42u, FM_LFO_DIV_4_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_DUB, 42u, FM_LFO_DIV_4_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_SPACE, 40u, FM_LFO_DIV_1_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_MOTION, 42u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_CHORD, 40u, FM_LFO_DIV_2_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 40u, FM_LFO_DIV_1_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_GRIT, 42u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_JAZZ)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_SPACE, 18u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_MOTION, 20u, FM_LFO_DIV_1_2, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_ISO, 20u, FM_LFO_DIV_1_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
        };
#elif defined(GENRE_SOUL)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { DUB_LFO_M_SPACE, 16u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_GRIT, 18u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_MOTION, 16u, FM_LFO_DIV_1_4, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 18u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 16u, FM_LFO_DIV_1_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_FUNK)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { DUB_LFO_M_MOTION, 14u, FM_LFO_DIV_1_8, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 16u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 14u, FM_LFO_DIV_1_1, DUB_LFO_SQR, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_MOTION, 18u, FM_LFO_DIV_1_4, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 16u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 20u, FM_LFO_DIV_1_4, DUB_LFO_SH, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_POP)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][14] = {
            { DUB_LFO_M_CHORD, 12u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_FEED, 10u, FM_LFO_DIV_1_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WOB, 14u, FM_LFO_DIV_1_4, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_FLUX, 12u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_GRIT, 16u, FM_LFO_DIV_1_8, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_MOTION, 10u, FM_LFO_DIV_1_4, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 20u, FM_LFO_DIV_1_4, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_WOB, 14u, FM_LFO_DIV_1_2, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_ISO, 12u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_FEED, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 18u, FM_LFO_DIV_1_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_ISO, 12u, FM_LFO_DIV_1_2, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_HIPHOP)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_SPACE, 18u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 22u, FM_LFO_DIV_1_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_MOTION, 20u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_FLUX, 20u, FM_LFO_DIV_1_4, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_IDM)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][14] = {
            { DUB_LFO_M_FLUX, 26u, FM_LFO_DIV_1_4, 90u, FM_LFO_ALG_SH_FM, 58u, 62u,
              DUB_LFO_M_MOTION, 22u, FM_LFO_DIV_2_1, 102u, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 28u, FM_LFO_DIV_1_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_FLUX, 24u, FM_LFO_DIV_1_8T, 114u, FM_LFO_ALG_SH_FM, 48u, 56u },
            { DUB_LFO_M_WOB, 30u, FM_LFO_DIV_1_16, 127u, FM_LFO_ALG_SH_FM, 68u, 64u,
              DUB_LFO_M_MOTION, 26u, FM_LFO_DIV_1_4, 114u, FM_LFO_ALG_PLUCK, 60u, 52u },
            { DUB_LFO_M_WEIGHT, 28u, FM_LFO_DIV_1_8, 75u, FM_LFO_ALG_CLASSIC, 0u, 0u,
              DUB_LFO_M_FLUX, 24u, FM_LFO_DIV_1_2, 90u, FM_LFO_ALG_SH_FM, 52u, 58u },
            { DUB_LFO_M_MOTION, 34u, FM_LFO_DIV_1_8, 114u, FM_LFO_ALG_PLUCK, 68u, 60u,
              DUB_LFO_M_WOB, 28u, FM_LFO_DIV_1_4, 90u, FM_LFO_ALG_CROSS, 54u, 50u },
            { DUB_LFO_M_FLUX, 36u, FM_LFO_DIV_1_4, 114u, FM_LFO_ALG_CROSS, 58u, 54u,
              DUB_LFO_M_MOTION, 30u, FM_LFO_DIV_1_16, 127u, FM_LFO_ALG_FOLD, 62u, 48u },
        };
#elif defined(GENRE_SNOWCRASH)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_SPACE, 20u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_FLUX, 22u, FM_LFO_DIV_1_4, DUB_LFO_SH, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_MOTION, 18u, FM_LFO_DIV_1_2, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WOB, 18u, FM_LFO_DIV_1_8, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_TECHNO)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { DUB_LFO_M_SPACE, 16u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WOB, 20u, FM_LFO_DIV_1_4, DUB_LFO_SQR, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_FLUX, 22u, FM_LFO_DIV_1_16, DUB_LFO_SH, FM_LFO_ALG_SH_FM, 56u, 60u },
            { DUB_LFO_M_MOTION, 16u, FM_LFO_DIV_1_2, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_ISO, 14u, FM_LFO_DIV_4_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 20u, FM_LFO_DIV_1_4, DUB_LFO_SQR, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_BASS)
        /* Only MAC 1..3 (M_CHORD = ACID, M_FEED = GRIT, M_WOB = SPACE) are LFO dests in KO BASS. */
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { DUB_LFO_M_CHORD, 22u, FM_LFO_DIV_1_8, 45u, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WOB, 16u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_CHORD, 18u, FM_LFO_DIV_1_4, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_FEED, 18u, FM_LFO_DIV_1_8, DUB_LFO_SINE, FM_LFO_ALG_CROSS, 52u, 56u },
            { DUB_LFO_M_FEED, 22u, FM_LFO_DIV_1_16, DUB_LFO_SQR, FM_LFO_ALG_FOLD, 60u, 48u },
            { DUB_LFO_M_FEED, 18u, FM_LFO_DIV_1_8, DUB_LFO_SQR, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#elif defined(GENRE_TRIPHOP)
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { DUB_LFO_M_SPACE, 14u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_DUB, 16u, FM_LFO_DIV_4_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_ISO, 14u, FM_LFO_DIV_1_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_CHORD, 12u, FM_LFO_DIV_2_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_FLUX, 16u, FM_LFO_DIV_1_1, DUB_LFO_SH, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_SPACE, 14u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#else
        static const uint8_t GENRE_MACRO_LFO_PICK[GENRE_ERA_N][7] = {
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_SPACE, 20u, FM_LFO_DIV_2_1, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { 0xFFu, 0u, 0u, 0u, 0u, 0u, 0u },
            { DUB_LFO_M_WEIGHT, 22u, FM_LFO_DIV_1_1, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_WOB, 20u, FM_LFO_DIV_1_2, DUB_LFO_SINE, FM_LFO_ALG_CLASSIC, 0u, 0u },
            { DUB_LFO_M_FLUX, 22u, FM_LFO_DIV_1_4, DUB_LFO_TRI, FM_LFO_ALG_CLASSIC, 0u, 0u },
        };
#endif
        const uint8_t *pick = GENRE_MACRO_LFO_PICK[era];
        uint8_t mi;

        for (mi = DUB_LFO_M_CHORD; mi <= DUB_LFO_M_FLUX; mi++)
            fm_lfo_slot_reset(&g.lfo_slots[mi]);
#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_IDM) || defined(GENRE_POP)
        dub_macro_lfo_apply_pick(pick, bump, plate);
        dub_macro_lfo_apply_pick(pick + 7u, bump, plate);
#else
        dub_macro_lfo_apply_pick(pick, bump, plate);
#endif
    }
    g.cfg.p.lfo1_wave = g.lfo_slots[steps[0].tgt].wave;
    g.cfg.p.lfo1_rate = g.lfo_slots[steps[0].tgt].rate;
    g.cfg.p.lfo1_depth = g.lfo_slots[steps[0].tgt].depth;
    g.cfg.p.lfo1_tgt = steps[0].tgt;
    if (steps[1].depth) {
        g.cfg.p.lfo2_wave = g.lfo_slots[steps[1].tgt].wave;
        g.cfg.p.lfo2_rate = g.lfo_slots[steps[1].tgt].rate;
        g.cfg.p.lfo2_depth = g.lfo_slots[steps[1].tgt].depth;
        g.cfg.p.lfo2_tgt = steps[1].tgt;
    }
    dub_cpl_apply_defaults();
}

static void dub_era_lfo_preset(uint8_t plate)
{
    dub_era_lfo_apply(g.cfg.p.era, plate);
}

static void seq_runtime_data_defaults(void);
static void seq_runtime_defaults(void);
static uint8_t dub_lfo_tgt_default(void);
static uint8_t dub_lfo_tgt_unpack(uint8_t raw);

static const dub_plate_seq_t *dub_plate_seq_of(uint8_t plate)
{
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    return &DUB_PLATE_SEQ[plate];
#elif defined(GENRE_HEAVY_PSYCH)
    return &HEAVY_PSYCH_PLATE_SEQ[plate];
#elif defined(GENRE_JAZZ)
    return &JAZZ_PLATE_SEQ[plate];
#elif defined(GENRE_SOUL)
    return &SOUL_PLATE_SEQ[plate];
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    return &IDM_PLATE_SEQ[plate];
#elif defined(GENRE_POP)
    return &POP_PLATE_SEQ[plate];
#elif defined(GENRE_HIPHOP)
    return &HIPHOP_PLATE_SEQ[plate];
#elif defined(GENRE_FUNK)
    return &FUNK_PLATE_SEQ[plate];
#elif defined(GENRE_HOUSE) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
    return &HOUSE_PLATE_SEQ[plate];
#else
    return &DUB_PLATE_SEQ[plate];
#endif
}

#ifdef FM_GENRE_STEP_MICRO
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static void snowcrash_seed_plate_groove_micro(void);
#elif defined(GENRE_JAZZ)
static void jazz_seed_plate_groove_micro(void);
#elif defined(GENRE_SOUL)
static void soul_seed_plate_groove_micro(void);
#endif
#endif

static void dub_lane_apply_plate_seq(uint8_t ln, uint8_t plate, const dub_plate_t *pl)
{
    const dub_plate_seq_t *ps = dub_plate_seq_of(plate);
    uint8_t st;

    ln %= DUB_SEQ_LANES;
    if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG || g.cfg.user_song)
        return;
#if defined(GENRE_JAZZ) || defined(GENRE_SOUL)
    g.vary_lane[ln] = ps->vary[ln];
    g.lane_trig[ln] = ps->trig[ln];
#else
    g.vary_lane[ln] = 0u;
    g.lane_trig[ln] = 0;
#endif
    g.lane_vel[ln] = ps->vel[ln];
    g.lane_gate[ln] = ps->gate[ln];
    for (st = 0; st < DUB_SEQ_STEPS; st++) {
#ifdef GENRE_HEAVY_PSYCH
        if (ln == 0u && dub_step_bit(pl->kick, st))
            g.step_vel[0][st] = ((st & 7u) == 0u) ? 112u : 98u;
        if (ln == 1u && dub_step_bit(pl->rim, st))
            g.step_vel[1][st] = 108u;
        if (ln == 2u && dub_step_bit(pl->hat, st))
            g.step_vel[2][st] = ((st & 3u) == 0u) ? 102u : 80u;
        if (ln == 3u && genre_lead_step_bit(plate, st)) {
            g.step_tune[3][st] = genre_lead_pitch(plate, st);
            g.step_micro[3][st] = (int8_t)genre_lead_len_ix(plate, st);
            g.step_vel[3][st] = genre_lead_bend_plock(genre_lead_bend_u7(plate, st));
        }
#elif defined(GENRE_JAZZ) || defined(GENRE_SOUL)
        if (ln == 3u && genre_lead_step_bit(plate, st)) {
            g.step_tune[3][st] = genre_lead_pitch(plate, st);
            g.step_micro[3][st] = (int8_t)genre_lead_len_ix(plate, st);
            g.step_vel[3][st] = genre_lead_bend_plock(genre_lead_bend_u7(plate, st));
        }
#elif defined(GENRE_HIPHOP) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        if (ln == 3u && genre_lead_step_bit(plate, st)) {
            g.step_tune[3][st] = genre_lead_pitch(plate, st);
            g.step_micro[3][st] = (int8_t)genre_lead_len_ix(plate, st);
            g.step_vel[3][st] = genre_lead_bend_plock(genre_lead_bend_u7(plate, st));
        }
#else
        (void)ps;
#endif
    }
}

static uint8_t raw_step_gate_show(uint8_t st);

/* One grid step has a trigger on any lane. Same gate data and lane map as playback (raw_step_gate_show,
 * seq_lane_step_map), but no runtime view state: dub_lane_step_eff_on is gated by seq_view_playable (runtime
 * seq_view_on / cyc and g.bar), which still holds the previous preset's values at preset load. */
static int seq_arr_step_has_trig(uint8_t gst)
{
    uint8_t ln;

    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        if (raw_step_gate_show(seq_lane_step_map(gst, ln)) & (1u << ln))
            return 1;
    }
#if defined(GENRE_BASS)
    if (gst < seq_lane_len_v(0u, 4u) && bass_seq_lane_base_on(gst))
        return 1;
#endif
    return 0;
}

/* PRESET mode arrangement: page ON (ALL) if any lane has a trigger in its 16 steps, else OFF; REPEAT 1.
 * At least one page ON: page 0 if none has a trigger. Runtime only, never stored here. */
static void seq_arr_derive_preset(void)
{
    uint8_t v, st;
    int any_on = 0;

    g.seq_view_only_page = FM_SEQ_MAX_PAGES;
    for (v = 0u; v < FM_SEQ_MAX_PAGES; v++) {
        int on = 0;

        for (st = 0u; st < FM_SEQ_PAGE_STEPS && !on; st++)
            on = seq_arr_step_has_trig((uint8_t)(v * FM_SEQ_PAGE_STEPS + st));
        g.seq_view_cyc[v] = on ? FM_SEQ_VIEW_CYC_ALL : FM_SEQ_VIEW_CYC_OFF;
        g.seq_view_reps[v] = 1u;
        any_on |= on;
    }
    if (!any_on)
        g.seq_view_cyc[0] = FM_SEQ_VIEW_CYC_ALL;
    for (v = 0u; v < FM_SEQ_MAX_PAGES; v++)
        seq_view_cyc_sync_on(v);
}

static void dub_plate_seq_preset(uint8_t plate)
{
    const dub_plate_t *pl;
    uint8_t ln;

    user_song_clear();
    plate = genre_clamp_preset(plate);
    pl = genre_preset(plate);
    seq_runtime_data_defaults();
    for (ln = 0; ln < DUB_SEQ_LANES; ln++)
        dub_lane_apply_plate_seq(ln, plate, pl);
    seq_arr_derive_preset();
#ifdef FM_GENRE_STEP_MICRO
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    snowcrash_seed_plate_groove_micro();
#elif defined(GENRE_JAZZ)
    jazz_seed_plate_groove_micro();
#elif defined(GENRE_SOUL)
    soul_seed_plate_groove_micro();
#endif
#endif
}

static void dub_era_home_macros_apply_era(uint8_t era, uint8_t morph);

static void dub_lfo_fm_defaults(dub_params_t *p)
{
    if (!p)
        return;
    p->lfo1_phase = 0u;
    p->lfo1_smooth = 0u;
    p->lfo1_skew = FM_LFO_SKEW_DEFAULT;
    p->lfo1_algo = 0u;
    p->lfo1_ratio = 52u;
    p->lfo1_index = 64u;
    p->lfo1_fb = 0u;
    p->lfo1_fold = 0u;
    p->lfo2_phase = 0u;
    p->lfo2_smooth = 0u;
    p->lfo2_skew = FM_LFO_SKEW_DEFAULT;
    p->lfo2_algo = 0u;
    p->lfo2_ratio = 52u;
    p->lfo2_index = 64u;
    p->lfo2_fb = 0u;
    p->lfo2_fold = 0u;
    fm_lfo_slots_init(g.lfo_slots, g.lfo_rt, DUB_LFO_TGT_N);
    fm_dj_lfo_eff_build(&g.dj_lfo_ov, &g.dj, g.lfo_slots, g.lfo_slots_eff, DUB_LFO_TGT_N);
    g.lfo_target = dub_lfo_tgt_default();
}

static void dub_dj_lfo_bind_seq_lanes(void)
{
    uint8_t si;
    uint8_t pi;
    uint8_t bound[4];

    for (pi = 0u; pi < 4u; pi++)
        bound[pi] = FM_DJ_SLOT_LANE_GLOBAL;
    if (!g.dj.on && !g.dj.slew_off)
        return;
    for (si = 0u; si < FM_DJ_SLOT_N; si++) {
        const fm_dj_slot_t *s = &g.dj.slot[si];
        const fm_dj_lfo_slot_ov_t *o = &g.dj_lfo_ov.per[si];

        if (!o->active || !fm_dj_tgt_is_seq_param(s->target))
            continue;
        if (s->phase == FM_DJ_SLOT_OFF && s->delta == 0)
            continue;
        pi = (uint8_t)(s->target - FM_DJ_TGT_SEQ_VEL);
        if (pi < 4u)
            bound[pi] = s->lane;
    }
    for (pi = 0u; pi < 4u; pi++) {
        if (bound[pi] != FM_DJ_SLOT_LANE_GLOBAL)
            g.seq_lfo_lane[3u + pi] = bound[pi];
    }
}

static void dub_dj_lfo_sync(void)
{
    uint8_t blocked = fm_dj_lfo_blocked(&g.dj, g.playing ? 1u : 0u, g.bar_step, DUB_STEPS);

    fm_dj_lfo_ov_track(&g.dj_lfo_ov, &g.dj, g.lfo_slots, blocked);
    fm_dj_lfo_eff_build(&g.dj_lfo_ov, &g.dj, g.lfo_slots, g.lfo_slots_eff, DUB_LFO_TGT_N);
    dub_dj_lfo_bind_seq_lanes();
}

static void dub_dj_fx_sync(void)
{
    uint8_t blocked = fm_dj_fx_blocked(&g.dj, g.playing ? 1u : 0u, g.bar_step, DUB_STEPS);

    fm_dj_fx_ov_track(&g.dj_fx_ov, &g.dj, blocked);
}

static const fm_lfo_slot_t *dub_lfo_slot_view(uint8_t tgt)
{
    if (tgt < DUB_LFO_TGT_N && fm_dj_lfo_tgt_overlay_live(&g.dj_lfo_ov, &g.dj, tgt))
        return &g.lfo_slots_eff[tgt];
    return &g.lfo_slots[tgt];
}

static uint8_t lfo_mod_u8(uint8_t base, uint8_t tgt, uint8_t lo, uint8_t hi)
{
    /* fm_lfo_slot_delta is already ±depth at full wave (q15 * depth >> 15). */
    int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, tgt);
    return fm_lfo_clamp_u8((int32_t)base + d, lo, hi);
}

/* Play / MIDI Start: tempo-synced LFO slots restart at their slot phase (stopped transport froze them). */
static void dub_lfo_transport_start(void)
{
    fm_lfo_slots_sync_restart(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_TGT_N, g.cfg.p.bpm ? g.cfg.p.bpm : 120u);
}

static uint8_t dub_lfo_mod_root_oct(uint8_t base)
{
    int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_OCT);
    int32_t semi = (d * 60) / 128;

    return fm_lfo_clamp_u8((int32_t)base + semi, 24, 84);
}

static uint8_t lfo_mod_enum(uint8_t base, uint8_t tgt, uint8_t count)
{
    int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, tgt);
    int32_t step = (d * (int32_t)count) / 128;
    int32_t ix = (int32_t)(base % count) + step;

    while (ix < 0)
        ix += (int32_t)count;
    return (uint8_t)((uint32_t)ix % count);
}

#if defined(GENRE_HEAVY_PSYCH)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'N', 'H', 'L', 'B', 'C'};
#elif defined(GENRE_HOUSE)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'N', 'H', 'S', 'B', 'C'};
#elif defined(GENRE_HIPHOP)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'N', 'H', 'X', 'B', 'C'};
#elif defined(GENRE_JAZZ)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'R', 'D', 'L', 'B', 'C'};
#elif defined(GENRE_SOUL)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'S', 'T', 'H', 'B', 'C'};
#elif defined(GENRE_IDM)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'R', 'D', 'T', 'B', 'C'};
#elif defined(GENRE_SNOWCRASH)
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'R', 'D', 'P', 'B', 'C'};
#else
static const char DUB_SEQ_LFO_LBL[6] = {'K', 'R', 'H', 'S', 'B', 'C'};
#endif
static const char *const DUB_SEQ_LFO_SUF[9] = {
    "PAT", "ROT", "VARY", "VEL", "TRIG", "GATE", "PTCH", "NUDG", "MICR"
};
static const char *const DUB_SEQ_LFO_LONG[9] = {
    "PATTRN", "ROTATE", "VARY", "VEL+-", "TRIG %", "GATE+-", "PITCH+-", "NUDGE", "MICRO"
};

static int dub_is_seq_lfo_tgt(uint8_t t)
{
    return t >= DUB_LFO_SEQ_PAT && t <= DUB_LFO_SEQ_MICRO;
}

static uint8_t dub_seq_lfo_ix(uint8_t t)
{
    return (uint8_t)(t - DUB_LFO_SEQ_PAT);
}

/* seq_lfo_lane[] index for a lane-bound LFO target, 255 = not lane-bound. */
static uint8_t dub_seq_lfo_bind_ix(uint8_t t)
{
    if (dub_is_seq_lfo_tgt(t))
        return dub_seq_lfo_ix(t);
    if (t == DUB_LFO_SEQ_PACE)
        return (uint8_t)DUB_SEQ_LFO_PACE_IX;
    return 255u;
}

static void dub_fmt_lfo_seq_short(char *s, uint8_t t)
{
    uint8_t ix = dub_seq_lfo_ix(t);
    uint8_t ln = g.seq_lfo_lane[ix] % DUB_SEQ_LANES;

    s[0] = DUB_SEQ_LFO_LBL[ln];
    s[1] = ' ';
    copy_str(s + 2, DUB_SEQ_LFO_SUF[ix]);
}

static void dub_fmt_lfo_seq_long(char *s, uint8_t t)
{
    uint8_t ix = dub_seq_lfo_ix(t);
    uint8_t ln = g.seq_lfo_lane[ix] % DUB_SEQ_LANES;
    const char *lane = seq_lane_name(ln);

    while (*lane)
        *s++ = *lane++;
    *s++ = ' ';
    copy_str(s, "SEQ ");
    s += 4;
    copy_str(s, DUB_SEQ_LFO_LONG[ix]);
}

static int dub_seq_knob_lfo_map_ctx(uint8_t seq_page, uint8_t slot, uint8_t *out)
{
#if defined(GENRE_BASS)
    if (g.seq_plock_hold) {
        if (!g.seq_plock_bank && slot == 2u) {
            *out = DUB_LFO_SEQ_MICRO;
            return 1;
        }
        return 0;
    }
    if (seq_page == 0u)
        return 0;
    if (seq_page == 1u) {
        *out = (uint8_t)(DUB_LFO_SEQ_VEL + fm_seq_p1_fn(slot));
        return 1;
    }
    if (seq_page == 2u) {
        if (slot == 3u) {
            *out = DUB_LFO_SEQ_VARY;
            return 1;
        }
        return 0;
    }
    return 0;
#endif
    if (g.seq_plock_hold) {
        if (slot == 2u) {
            *out = DUB_LFO_SEQ_MICRO;
            return 1;
        }
        return 0;
    }
    if (seq_page == 0u) {
        if (slot == 1u)
            *out = DUB_LFO_SEQ_PAT;
        else if (slot == 2u)
            *out = DUB_LFO_SEQ_ROT;
        else if (slot == 3u)
            *out = DUB_LFO_SEQ_VARY;
        else
            return 0;
        return 1;
    }
    if (seq_page == 1u) {
        *out = (uint8_t)(DUB_LFO_SEQ_VEL + fm_seq_p1_fn(slot));
        return 1;
    }
    if (seq_page == 4u) {
        if (slot == 2u)
            *out = DUB_LFO_ENS;
        else if (slot == 3u)
            *out = DUB_LFO_FLIP;
        else
            return 0;
        return 1;
    }
    if (seq_page == 5u) {
        *out = slot == 0 ? DUB_LFO_KEY : slot == 1 ? DUB_LFO_SCALE :
               slot == 2 ? DUB_LFO_SWING : DUB_LFO_TEMPO;
        return 1;
    }
    if (seq_page == 2u || seq_page == 3u) {
        if (slot == 2u) {
            *out = DUB_LFO_SEQ_NUDGE;
            return 1;
        }
        return 0;
    }
    return 0;
}

static int dub_seq_knob_lfo_map(uint8_t slot, uint8_t *out)
{
    if (g.view != VIEW_SEQ)
        return 0;
    return dub_seq_knob_lfo_map_ctx(g.seq_page, slot, out);
}

static uint8_t dub_eff_vary(uint8_t ln)
{
    uint8_t v = g.vary_lane[ln];

    if (g.seq_lfo_lane[2] == ln && g.lfo_slots[DUB_LFO_SEQ_VARY].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_SEQ_VARY);
        int32_t nv = (int32_t)v + d;

        if (nv < 0)
            nv = 0;
        if (nv > 127)
            nv = 127;
        v = (uint8_t)nv;
    }
    return v;
}

static int8_t dub_eff_vel_rng(uint8_t ln)
{
    int32_t v = g.lane_vel[ln];

    if (g.seq_lfo_lane[3] == ln && g.lfo_slots_eff[DUB_LFO_SEQ_VEL].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_SEQ_VEL);
        v += (d * 50) / 127;
    }
    v += (fm_dj_delta_for_tgt_lane(&g.dj, FM_DJ_TGT_SEQ_VEL, ln) * 50) / 127;
    if (v < -50)
        v = -50;
    if (v > 50)
        v = 50;
    return (int8_t)v;
}

static int8_t dub_eff_trig_raw(uint8_t ln)
{
    int32_t v = g.lane_trig[ln];

    if (g.seq_lfo_lane[4] == ln && g.lfo_slots_eff[DUB_LFO_SEQ_TRIG].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_SEQ_TRIG);
        v += (d * 100) / 127;
    }
    v += (fm_dj_delta_for_tgt_lane(&g.dj, FM_DJ_TGT_SEQ_TRIG, ln) * 100) / 127;
    if (v < -100)
        v = -100;
    if (v > 0)
        v = 0;
    return (int8_t)v;
}

static int8_t dub_eff_gate_rng(uint8_t ln)
{
    int32_t v = g.lane_gate[ln];

    if (g.seq_lfo_lane[5] == ln && g.lfo_slots_eff[DUB_LFO_SEQ_GATE].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_SEQ_GATE);
        v += (d * 50) / 127;
    }
    v += (fm_dj_delta_for_tgt_lane(&g.dj, FM_DJ_TGT_SEQ_GATE, ln) * 50) / 127;
    if (v < -50)
        v = -50;
    if (v > 50)
        v = 50;
    return (int8_t)v;
}

static int8_t dub_eff_pitch_rng(uint8_t ln)
{
    int32_t v = g.lane_tune[ln];

    if (g.seq_lfo_lane[6] == ln && g.lfo_slots_eff[DUB_LFO_SEQ_PITCH].depth > 0u) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_SEQ_PITCH);
        v += (d * 24) / 127;
    }
    v += (fm_dj_delta_for_tgt_lane(&g.dj, FM_DJ_TGT_SEQ_PITCH, ln) * 24) / 127;
    if (v < -24)
        v = -24;
    if (v > 24)
        v = 24;
    return (int8_t)v;
}

static uint8_t dub_eff_euclid_hits(uint8_t ln)
{
    uint8_t h = g.euclid_hits[ln];

    if (g.lfo_slots[DUB_LFO_SEQ_PAT].depth > 0u && g.seq_lfo_lane[0] == ln &&
        fm_seq_euclid_is_pattern(h)) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_SEQ_PAT);
        int32_t nh = (int32_t)h + (d * 15) / 127;

        if (nh < 1)
            nh = 1;
        if (nh > 16)
            nh = 16;
        h = (uint8_t)nh;
    }
    return h;
}

static int8_t dub_eff_euclid_rot(uint8_t ln)
{
    int32_t r = g.euclid_rot[ln];

    r = fm_seq_euclid_rot_norm(r);
    if (g.lfo_slots[DUB_LFO_SEQ_ROT].depth > 0u && g.seq_lfo_lane[1] == ln) {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_SEQ_ROT);
        r += (d * 15) / 127;
    }
    r += (fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_SEQ_ROT) * 15) / 127;
    return (int8_t)fm_seq_euclid_rot_norm(r);
}

static uint8_t raw_step_gate(uint8_t st);
static uint8_t raw_step_gate_show(uint8_t st);
static uint8_t step_gate(uint8_t st);

static uint8_t dub_lane_step_map_lfo_rot(uint8_t ln, uint8_t st)
{
    int32_t d;
    int32_t off;
    uint32_t pg;
    int32_t col;

    if (g.lfo_slots[DUB_LFO_SEQ_ROT].depth == 0u || g.seq_lfo_lane[1] != ln)
        return seq_lane_step_map(st, ln);
    pg = (uint32_t)(st / FM_SEQ_PAGE_STEPS) * FM_SEQ_PAGE_STEPS;
    if (pg != seq_view_base())
        return seq_lane_step_map(st, ln);
    d = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_SEQ_ROT);
    off = (d * 15) / 127;
    if (off == 0)
        return seq_lane_step_map(st, ln);
    col = (int32_t)(st % FM_SEQ_PAGE_STEPS) - off;
    while (col < 0)
        col += FM_SEQ_PAGE_STEPS;
    while (col >= (int32_t)FM_SEQ_PAGE_STEPS)
        col -= FM_SEQ_PAGE_STEPS;
    return seq_lane_step_map((uint8_t)(pg + (uint32_t)col), ln);
}

static int dub_lane_step_eff_on(uint8_t ln, uint8_t st, int playback_only)
{
    uint8_t h = g.euclid_hits[ln];
    uint8_t ms;

    if (playback_only && !seq_grid_step_audible(st))
        return 0;
    if ((g.lfo_slots[DUB_LFO_SEQ_PAT].depth > 0u && g.seq_lfo_lane[0] == ln) ||
        (g.lfo_slots[DUB_LFO_SEQ_ROT].depth > 0u && g.seq_lfo_lane[1] == ln)) {
        if (fm_seq_euclid_is_pattern(h)) {
            ms = seq_lane_step_map(st, ln);
            {
                uint16_t mask = fm_seq_euclid16(dub_eff_euclid_hits(ln), dub_eff_euclid_rot(ln));
                return (int)((mask >> (ms & (FM_SEQ_PAGE_STEPS - 1u))) & 1u);
            }
        }
        ms = dub_lane_step_map_lfo_rot(ln, st);
        return playback_only ? (int)((step_gate(ms) & (1u << ln)) != 0) :
               (int)((raw_step_gate(ms) & (1u << ln)) != 0);
    }
    ms = seq_lane_step_map(st, ln);
    return playback_only ? (int)((step_gate(ms) & (1u << ln)) != 0) :
           (int)((raw_step_gate(ms) & (1u << ln)) != 0);
}

static int dub_lane_step_base_on(uint8_t ln, uint8_t st)
{
    return dub_lane_step_eff_on(ln, st, 1);
}

static uint8_t dub_drop_sil_audible_mask(void)
{
    uint8_t mask = 0u;
    uint8_t ln;
    uint8_t st;

    for (ln = 0u; ln < DUB_SEQ_LANES; ln++) {
        for (st = 0u; st < DUB_STEPS; st++) {
            if (dub_lane_step_base_on(ln, st)) {
                mask |= (uint8_t)(1u << ln);
                break;
            }
        }
    }
    return mask;
}

static void sync_voice_params(void);
static uint8_t gesture_active(void);
static void dub_dj_gest_reset(void);
static void dub_dj_gest_restore(void);
static void dub_dj_gest_on_bar(void);
static void dub_dj_gest_seq_step(void);
static void gesture_solo_sync(uint8_t act_g);
static void dub_arp_set_mode(uint8_t mode);

static uint8_t dub_morph_lerp(uint8_t a, uint8_t b, uint8_t pos)
{
    return (uint8_t)((int32_t)a + (((int32_t)b - (int32_t)a) * (int32_t)pos) / (int32_t)DUB_MORPH_STEPS);
}

static uint8_t dub_morph_lerp_steps(uint8_t a, uint8_t b, uint8_t pos, uint8_t steps)
{
    if (steps < 1u)
        steps = 1u;
    if (pos >= steps)
        return b;
    return (uint8_t)((int32_t)a + (((int32_t)b - (int32_t)a) * (int32_t)pos) / (int32_t)steps);
}

/* Slow ease-in: stays low early, rises late (t²). */
static uint8_t dub_morph_ease_in_quad_steps(uint8_t a, uint8_t b, uint8_t pos, uint8_t steps)
{
    uint32_t t;

    if (steps < 1u)
        steps = 1u;
    if (pos >= steps)
        return b;
    t = ((uint32_t)pos << 15) / (uint32_t)steps; /* 0..32768 */
    t = (t * t) >> 15;                             /* 0..32768 curved */
    return (uint8_t)((int32_t)a + (((int32_t)b - (int32_t)a) * (int32_t)t) / 32768);
}

static void dub_morph_nudge_u8(uint8_t *cur, uint8_t to)
{
    if (*cur < to)
        (*cur)++;
    else if (*cur > to)
        (*cur)--;
}

static void dub_morph_nudge_def64(uint8_t *cur, uint8_t to)
{
    uint8_t c = *cur ? *cur : 64u;
    uint8_t t = to ? to : 64u;

    if (c < t)
        c++;
    else if (c > t)
        c--;
    if (to == 0u && c == 64u)
        *cur = 0u;
    else
        *cur = c;
}

/* Pan storage 0 = default center (per-channel default in sync/DSP). */
static void dub_morph_nudge_pan(uint8_t *cur, uint8_t to, uint8_t def)
{
    uint8_t c = *cur ? *cur : def;
    uint8_t t = to ? to : def;

    if (c < t)
        c++;
    else if (c > t)
        c--;
    if (to == 0u && c == def)
        *cur = 0u;
    else
        *cur = c;
}

static uint8_t dub_morph_u8_match(uint8_t a, uint8_t b)
{
    return a == b;
}

static uint8_t dub_morph_def64_match(uint8_t a, uint8_t b)
{
    uint8_t ea = a ? a : 64u;
    uint8_t eb = b ? b : 64u;

    return ea == eb;
}

static uint8_t dub_morph_pan_match(uint8_t a, uint8_t b, uint8_t def)
{
    uint8_t ea = a ? a : def;
    uint8_t eb = b ? b : def;

    return ea == eb;
}

static void dub_morph_nudge_params(dub_params_t *from, const dub_params_t *to)
{
    uint32_t i;

    dub_morph_nudge_u8(&from->timbre, to->timbre);
    dub_morph_nudge_u8(&from->cutoff_c, to->cutoff_c);
    dub_morph_nudge_u8(&from->decay_c, to->decay_c);
    dub_morph_nudge_u8(&from->atk_c, to->atk_c);
    dub_morph_nudge_u8(&from->dly_c, to->dly_c);
    dub_morph_nudge_u8(&from->reso_mode, to->reso_mode);
    dub_morph_nudge_u8(&from->env_filt, to->env_filt);
    dub_morph_nudge_u8(&from->glide_c, to->glide_c);
    dub_morph_nudge_u8(&from->rev_c, to->rev_c);
    dub_morph_nudge_u8(&from->cutoff_b, to->cutoff_b);
    dub_morph_nudge_u8(&from->decay_b, to->decay_b);
    dub_morph_nudge_u8(&from->atk_b, to->atk_b);
    dub_morph_nudge_u8(&from->wobble, to->wobble);
    dub_morph_nudge_u8(&from->sub_oct, to->sub_oct);
    dub_morph_nudge_u8(&from->drive, to->drive);
    dub_morph_nudge_u8(&from->pump, to->pump);
    dub_morph_nudge_u8(&from->dly_b, to->dly_b);
    dub_morph_nudge_u8(&from->lvl_b, to->lvl_b);
    dub_morph_nudge_u8(&from->lvl_c, to->lvl_c);
    dub_morph_nudge_u8(&from->rev_b, to->rev_b);
    dub_morph_nudge_u8(&from->density, to->density);
    dub_morph_nudge_u8(&from->rim_tone, to->rim_tone);
    dub_morph_nudge_u8(&from->siren_pitch, to->siren_pitch);
    dub_morph_nudge_u8(&from->siren_rate, to->siren_rate);
    dub_morph_nudge_u8(&from->kick_boom, to->kick_boom);
    dub_morph_nudge_u8(&from->hat_lvl, to->hat_lvl);
    for (i = 0; i < 4u; i++) {
        dub_morph_nudge_u8(&from->drm_dly[i], to->drm_dly[i]);
        dub_morph_nudge_u8(&from->drm_rev[i], to->drm_rev[i]);
    }
    dub_morph_nudge_u8(&from->dly_time, to->dly_time);
    dub_morph_nudge_u8(&from->feedback, to->feedback);
    dub_morph_nudge_u8(&from->dly_lvl, to->dly_lvl);
    dub_morph_nudge_u8(&from->rev_mix, to->rev_mix);
    dub_morph_nudge_u8(&from->rev_damp, to->rev_damp);
    dub_morph_nudge_u8(&from->rev_tail, to->rev_tail);
    dub_morph_nudge_u8(&from->dly_rev, to->dly_rev);
    dub_morph_nudge_u8(&from->loop_filt, to->loop_filt);
    dub_morph_nudge_u8(&from->wow, to->wow);
    dub_morph_nudge_u8(&from->tank, to->tank);
    dub_morph_nudge_u8(&from->isolator, to->isolator);
    for (i = 0; i < 4u; i++) {
        static const uint8_t def_dp[4] = {64u, 74u, 52u, 82u};

        dub_morph_nudge_pan(&from->pan[i], to->pan[i], 64u);
        dub_morph_nudge_pan(&from->drm_pan[i], to->drm_pan[i], def_dp[i]);
    }
    for (i = 0; i < 4u; i++) {
        dub_morph_nudge_def64(&from->drm_tune[i], to->drm_tune[i]);
        dub_morph_nudge_def64(&from->drm_dec[i], to->drm_dec[i]);
        dub_morph_nudge_u8(&from->drm_drv[i], to->drm_drv[i]);
        dub_morph_nudge_def64(&from->drm_cut[i], to->drm_cut[i]);
        dub_morph_nudge_u8(&from->drm_atk[i], to->drm_atk[i]);
        dub_morph_nudge_u8(&from->drm_dens[i], to->drm_dens[i]);
    }
}

static int dub_morph_arrived(void)
{
    uint32_t i;
    const dub_params_t *f = &g.morph_from;
    const dub_params_t *t = &g.cfg.p;

    if (!dub_morph_u8_match(f->timbre, t->timbre) ||
        !dub_morph_u8_match(f->cutoff_c, t->cutoff_c) ||
        !dub_morph_u8_match(f->decay_c, t->decay_c) ||
        !dub_morph_u8_match(f->atk_c, t->atk_c) ||
        !dub_morph_u8_match(f->dly_c, t->dly_c) ||
        !dub_morph_u8_match(f->reso_mode, t->reso_mode) ||
        !dub_morph_u8_match(f->env_filt, t->env_filt) ||
        !dub_morph_u8_match(f->glide_c, t->glide_c) ||
        !dub_morph_u8_match(f->rev_c, t->rev_c) ||
        !dub_morph_u8_match(f->cutoff_b, t->cutoff_b) ||
        !dub_morph_u8_match(f->decay_b, t->decay_b) ||
        !dub_morph_u8_match(f->atk_b, t->atk_b) ||
        !dub_morph_u8_match(f->wobble, t->wobble) ||
        !dub_morph_u8_match(f->sub_oct, t->sub_oct) ||
        !dub_morph_u8_match(f->drive, t->drive) ||
        !dub_morph_u8_match(f->pump, t->pump) ||
        !dub_morph_u8_match(f->dly_b, t->dly_b) ||
        !dub_morph_u8_match(f->lvl_b, t->lvl_b) ||
        !dub_morph_u8_match(f->lvl_c, t->lvl_c) ||
        !dub_morph_u8_match(f->rev_b, t->rev_b) ||
        !dub_morph_u8_match(f->density, t->density) ||
        !dub_morph_u8_match(f->rim_tone, t->rim_tone) ||
        !dub_morph_u8_match(f->siren_pitch, t->siren_pitch) ||
        !dub_morph_u8_match(f->siren_rate, t->siren_rate) ||
        !dub_morph_u8_match(f->kick_boom, t->kick_boom) ||
        !dub_morph_u8_match(f->hat_lvl, t->hat_lvl))
        return 0;
    for (i = 0; i < 4u; i++) {
        if (!dub_morph_u8_match(f->drm_dly[i], t->drm_dly[i]) ||
            !dub_morph_u8_match(f->drm_rev[i], t->drm_rev[i]) ||
            !dub_morph_pan_match(f->pan[i], t->pan[i], 64u) ||
            !dub_morph_pan_match(f->drm_pan[i], t->drm_pan[i], (uint8_t)(i == 0u ? 64u : i == 1u ? 74u : i == 2u ? 52u : 82u)) ||
            !dub_morph_def64_match(f->drm_tune[i], t->drm_tune[i]) ||
            !dub_morph_def64_match(f->drm_dec[i], t->drm_dec[i]) ||
            !dub_morph_u8_match(f->drm_drv[i], t->drm_drv[i]) ||
            !dub_morph_def64_match(f->drm_cut[i], t->drm_cut[i]) ||
            !dub_morph_u8_match(f->drm_atk[i], t->drm_atk[i]) ||
            !dub_morph_u8_match(f->drm_dens[i], t->drm_dens[i]))
            return 0;
    }
    if (!dub_morph_u8_match(f->dly_time, t->dly_time) ||
        !dub_morph_u8_match(f->feedback, t->feedback) ||
        !dub_morph_u8_match(f->dly_lvl, t->dly_lvl) ||
        !dub_morph_u8_match(f->rev_mix, t->rev_mix) ||
        !dub_morph_u8_match(f->rev_damp, t->rev_damp) ||
        !dub_morph_u8_match(f->rev_tail, t->rev_tail) ||
        !dub_morph_u8_match(f->dly_rev, t->dly_rev) ||
        !dub_morph_u8_match(f->loop_filt, t->loop_filt) ||
        !dub_morph_u8_match(f->wow, t->wow) ||
        !dub_morph_u8_match(f->tank, t->tank) ||
        !dub_morph_u8_match(f->isolator, t->isolator))
        return 0;
    if (f->era != t->era || f->plate != t->plate || f->drum_pat != t->drum_pat ||
        f->voicing != t->voicing || f->bass_model != t->bass_model || f->drum_kit != t->drum_kit ||
        f->tap != t->tap)
        return 0;
    return 1;
}

static uint8_t dub_morph_enum_tick(uint8_t k)
{
    return (uint8_t)((k * DUB_MORPH_STEPS) / DUB_MORPH_ENUM_N);
}

static void dub_morph_sync_enums(dub_params_t *from, const dub_params_t *to, uint8_t tick)
{
    if (tick >= dub_morph_enum_tick(0u))
        from->era = to->era;
    if (tick >= dub_morph_enum_tick(1u))
        from->plate = to->plate;
    if (tick >= dub_morph_enum_tick(2u))
        from->drum_pat = to->drum_pat;
    if (tick >= dub_morph_enum_tick(3u))
        from->voicing = to->voicing;
    if (tick >= dub_morph_enum_tick(4u))
        from->bass_model = to->bass_model;
    if (tick >= dub_morph_enum_tick(5u))
        from->drum_kit = to->drum_kit;
    if (tick >= dub_morph_enum_tick(6u))
        from->tap = to->tap;
}

static uint8_t dub_groove_show_swing(void)
{
    if (g.cue_on)
        return g.cue_swing ? g.cue_swing : g.cfg.p.swing;
    return g.cfg.p.swing;
}

static uint8_t dub_groove_show_bpm(void)
{
    if (g.cue_on)
        return g.cue_bpm ? g.cue_bpm : g.cfg.p.bpm;
    return g.cfg.p.bpm;
}

static uint8_t dub_groove_show_scale(void)
{
    if (g.cue_on)
        return g.cue_scale;
    return g.cfg.p.scale;
}

static uint32_t dub_sample_acc_frac256(uint32_t acc, uint32_t spp)
{
    if (!spp)
        return 0u;
    return (acc * 256u) / spp;
}

static uint32_t dub_sample_acc_frac_q28(uint32_t acc, uint32_t spp)
{
    if (!spp)
        return 0u;
    return ((acc * 65536u) / spp) << 12;
}

static void dub_eval_morphed_params(dub_params_t *dst, const dub_params_t *from, const dub_params_t *to, uint8_t pos)
{
    uint32_t i;

    (void)pos;
    *dst = *to;
    dst->era = from->era;
    dst->plate = from->plate;
    dst->drum_pat = from->drum_pat;
    dst->voicing = from->voicing;
    dst->bass_model = from->bass_model;
    dst->drum_kit = from->drum_kit;
    dst->tap = from->tap;
    dst->timbre = from->timbre;
    dst->cutoff_c = from->cutoff_c;
    dst->decay_c = from->decay_c;
    dst->atk_c = from->atk_c;
    dst->dly_c = from->dly_c;
    dst->reso_mode = from->reso_mode;
    dst->env_filt = from->env_filt;
    dst->glide_c = from->glide_c;
    dst->rev_c = from->rev_c;
    dst->cutoff_b = from->cutoff_b;
    dst->decay_b = from->decay_b;
    dst->atk_b = from->atk_b;
    dst->wobble = from->wobble;
    dst->sub_oct = from->sub_oct;
    dst->drive = from->drive;
    dst->pump = from->pump;
    dst->dly_b = from->dly_b;
    dst->lvl_b = from->lvl_b;
    dst->lvl_c = from->lvl_c;
    dst->rev_b = from->rev_b;
    dst->density = from->density;
    dst->rim_tone = from->rim_tone;
    dst->siren_pitch = from->siren_pitch;
    dst->siren_rate = from->siren_rate;
    dst->kick_boom = from->kick_boom;
    dst->hat_lvl = from->hat_lvl;
    for (i = 0; i < 4u; i++) {
        dst->drm_dly[i] = from->drm_dly[i];
        dst->drm_rev[i] = from->drm_rev[i];
    }
    dst->dly_time = from->dly_time;
    dst->feedback = from->feedback;
    dst->dly_lvl = from->dly_lvl;
    dst->rev_mix = from->rev_mix;
    dst->rev_damp = from->rev_damp;
    dst->rev_tail = from->rev_tail;
    dst->dly_rev = from->dly_rev;
    dst->loop_filt = from->loop_filt;
    dst->wow = from->wow;
    dst->tank = from->tank;
    dst->isolator = from->isolator;
    /* Groove group (bpm/root/scale/swing): never plate/era crossfade — live lock or cue only. */
    dst->bpm = from->bpm;
    dst->root = from->root;
    dst->scale = from->scale;
    dst->swing = from->swing;
    for (i = 0; i < 4u; i++) {
        dst->pan[i] = from->pan[i];
        dst->drm_pan[i] = from->drm_pan[i];
    }
    for (i = 0; i < 4u; i++) {
        dst->drm_tune[i] = from->drm_tune[i];
        dst->drm_dec[i] = from->drm_dec[i];
        dst->drm_drv[i] = from->drm_drv[i];
        dst->drm_cut[i] = from->drm_cut[i];
        dst->drm_atk[i] = from->drm_atk[i];
        dst->drm_dens[i] = from->drm_dens[i];
    }
}

static void dub_morph_blend(dub_params_t *dst, const dub_params_t *from, const dub_params_t *to, uint8_t pos)
{
    dub_eval_morphed_params(dst, from, to, pos);
}

static void dub_morph_snapshot(dub_params_t *dst)
{
    if (g.morph_pos < DUB_MORPH_STEPS)
        dub_morph_blend(dst, &g.morph_from, &g.cfg.p, g.morph_pos);
    else
        *dst = g.cfg.p;
}

static void dub_morph_snapshot_voice(dub_params_t *dst)
{
    sync_voice_params();
    *dst = g.voice.p;
}

static void dub_morph_begin(const dub_params_t *from)
{
    g.morph_from = *from;
    if (!g.playing) {
        g.morph_pos = DUB_MORPH_STEPS;
        g.macro_morph_pos = DUB_MORPH_STEPS;
        g.morph_acc = 0u;
        g.macro_morph_acc = 0u;
    } else {
        g.morph_pos = 0u;
        g.morph_tick = 0u;
        g.morph_acc = 0u;
    }
}

static void transport_set_bpm(void);
static void dub_home_morph_flash_arm(uint8_t kind);
static void dub_home_morph_flash_end_check(void);
static uint8_t dub_macro_morph_cap(void);

static void dub_morph_clock(uint32_t samples)
{
    uint32_t step_smp;

    if (g.morph_pos >= DUB_MORPH_STEPS)
        return;
    step_smp = g.samples_per_step;
    if (step_smp < 32u)
        step_smp = 5512u;
    g.morph_acc += samples;
    while (g.morph_acc >= step_smp) {
        g.morph_acc -= step_smp;
        if (g.morph_tick < 255u)
            g.morph_tick++;
        dub_morph_nudge_params(&g.morph_from, &g.cfg.p);
        dub_morph_sync_enums(&g.morph_from, &g.cfg.p, g.morph_tick);
        g.ui_dirty = 1u;
        if (dub_morph_arrived()) {
            g.morph_pos = DUB_MORPH_STEPS;
            g.morph_from = g.cfg.p;
            transport_set_bpm();
            dub_home_morph_flash_end_check();
        }
    }
}

#define DUB_NHOME_MACROS 10u

static uint8_t dub_raw_macro(uint8_t idx)
{
    switch (idx) {
    case 0:
        return g.voice.macro_chord;
    case 1:
        return g.voice.macro_fb;
    case 2:
        return g.voice.macro_wob;
    case 3:
        return g.voice.macro_iso;
    case 4:
        return g.voice.macro_space;
    case 5:
        return g.voice.macro_weight;
    case 6:
        return g.voice.macro_grit;
    case 7:
        return g.voice.macro_dub;
    case 8:
        return g.voice.macro_motion;
    case 9:
        return g.voice.macro_flux;
    default:
        return 0u;
    }
}

static uint8_t *dub_macro_ptr(uint8_t idx)
{
    switch (idx) {
    case 0:
        return &g.voice.macro_chord;
    case 1:
        return &g.voice.macro_fb;
    case 2:
        return &g.voice.macro_wob;
    case 3:
        return &g.voice.macro_iso;
    case 4:
        return &g.voice.macro_space;
    case 5:
        return &g.voice.macro_weight;
    case 6:
        return &g.voice.macro_grit;
    case 7:
        return &g.voice.macro_dub;
    case 8:
        return &g.voice.macro_motion;
    case 9:
        return &g.voice.macro_flux;
    default:
        return 0;
    }
}

#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static uint8_t idm_fill_morph_steps(void)
{
    if (!(g.fill_build || g.fill_build_snap || g.fill_drop))
        return DUB_MORPH_STEPS;
    if (g.fill_build_kind == GEST_FILL)
        return IDM_FILL_MORPH_STEPS;
    return IDM_STUTTER_MORPH_STEPS;
}
#endif

static uint8_t dub_macro_morph_base(uint8_t idx)
{
    uint8_t raw;

    if (idx >= DUB_NHOME_MACROS)
        return 0u;
    raw = dub_raw_macro(idx);
#if defined(GENRE_HOUSE)
    if (g.fill_build || g.fill_build_snap) {
        uint8_t msteps = HOUSE_FILL_MORPH_STEPS;

        if (g.macro_morph_pos < msteps)
            return dub_morph_lerp_steps(g.macro_from[idx], raw, g.macro_morph_pos, msteps);
        return raw;
    }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    if (g.fill_build || g.fill_build_snap) {
        uint8_t msteps = idm_fill_morph_steps();

        if (g.macro_morph_pos < msteps) {
            if (g.fill_build_kind == GEST_FILL)
                return dub_morph_ease_in_quad_steps(g.macro_from[idx], raw, g.macro_morph_pos, msteps);
            return dub_morph_lerp_steps(g.macro_from[idx], raw, g.macro_morph_pos, msteps);
        }
        return raw;
    }
#endif
    if (g.macro_morph_pos < DUB_MORPH_STEPS)
        return dub_morph_lerp(g.macro_from[idx], raw, g.macro_morph_pos);
    return raw;
}

static uint8_t dub_eff_macro(uint8_t idx)
{
    uint8_t v = lfo_mod_u8(dub_macro_morph_base(idx), (uint8_t)(DUB_LFO_M_CHORD + idx), 0, 127);

    if (fm_dj_lfo_macro_static_bypass(&g.dj, &g.dj_lfo_ov, idx))
        return v;
    return fm_dj_mod_macro(v, &g.dj, idx);
}

/* Engine FLUX: morph base only (dub_eff_macro() LFO on M_FLUX must not run FLUX at raw 0). Drives FLUX couplings + icon. */
static uint8_t dub_eff_flux_engine(void)
{
    return fm_dj_mod_macro(dub_macro_morph_base(9u), &g.dj, 9u);
}

static int dub_home_macros_all_zero(void)
{
    uint8_t i;

    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        if (dub_raw_macro(i) != 0u)
            return 0;
    }
    return 1;
}

static void dub_macro_morph_snapshot(void)
{
    uint8_t i;

    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        uint8_t raw = dub_raw_macro(i);
        if (g.macro_morph_pos < DUB_MORPH_STEPS)
            g.macro_from[i] = dub_morph_lerp(g.macro_from[i], raw, g.macro_morph_pos);
        else
            g.macro_from[i] = raw;
    }
}

enum { DUB_FLASH_XF = 1u, DUB_FLASH_EV = 2u, DUB_FLASH_CUE = 4u };

static void dub_home_morph_flash_arm(uint8_t bits)
{
    if (bits == 0u) {
        g.home_morph_flash_kind = 0u;
        g.home_morph_flash_ms = 0u;
        return;
    }
    g.home_morph_flash_kind = (uint8_t)(g.home_morph_flash_kind | bits);
    g.home_morph_flash_ms = fm1_ms + 1000u;
}

static void dub_home_morph_flash_clear_bits(uint8_t bits)
{
    g.home_morph_flash_kind = (uint8_t)(g.home_morph_flash_kind & (uint8_t)~bits);
    if (g.home_morph_flash_kind == 0u)
        g.home_morph_flash_ms = 0u;
}

static uint8_t dub_home_morph_flash_live_mask(void)
{
    uint8_t k = g.home_morph_flash_kind;
    uint8_t live = 0u;

    if (!k)
        return 0u;
    if (g.playing) {
        if ((k & DUB_FLASH_XF) && g.morph_pos < DUB_MORPH_STEPS)
            live |= DUB_FLASH_XF;
        if ((k & DUB_FLASH_EV) && g.macro_morph_pos < dub_macro_morph_cap())
            live |= DUB_FLASH_EV;
        if ((k & DUB_FLASH_CUE) && g.cue_on)
            live |= DUB_FLASH_CUE;
    } else {
        if ((k & (DUB_FLASH_XF | DUB_FLASH_EV)) && fm1_ms < g.home_morph_flash_ms)
            live |= (uint8_t)(k & (DUB_FLASH_XF | DUB_FLASH_EV));
        if ((k & DUB_FLASH_CUE) && fm1_ms < g.home_morph_flash_ms)
            live |= DUB_FLASH_CUE;
    }
    return live;
}

static const char *dub_home_morph_flash_msg(uint8_t bit)
{
    if (bit & DUB_FLASH_XF)
        return "CROSSFADE";
    if (bit & DUB_FLASH_EV)
        return "EVOLVING";
    if (bit & DUB_FLASH_CUE)
        return "CUEING";
    return 0;
}

/* Rotate among live bits so XF/EV/CUE all get toast time when coupled. */
static uint8_t dub_home_morph_flash_pick_bit(uint8_t live)
{
    uint8_t bits[3];
    uint8_t n = 0u;
    uint8_t i;

    if (live & DUB_FLASH_XF)
        bits[n++] = DUB_FLASH_XF;
    if (live & DUB_FLASH_EV)
        bits[n++] = DUB_FLASH_EV;
    if (live & DUB_FLASH_CUE)
        bits[n++] = DUB_FLASH_CUE;
    if (n == 0u)
        return 0u;
    if (n == 1u)
        return bits[0];
    i = (uint8_t)((fm1_ms / 450u) % (uint32_t)n);
    return bits[i];
}

static void dub_home_morph_flash_end_check(void)
{
    uint8_t k = g.home_morph_flash_kind;

    if (k == 0u)
        return;
    if (g.playing) {
        if ((k & DUB_FLASH_XF) && g.morph_pos >= DUB_MORPH_STEPS)
            dub_home_morph_flash_clear_bits(DUB_FLASH_XF);
        if ((k & DUB_FLASH_EV) && g.macro_morph_pos >= dub_macro_morph_cap())
            dub_home_morph_flash_clear_bits(DUB_FLASH_EV);
        if ((k & DUB_FLASH_CUE) && !g.cue_on)
            dub_home_morph_flash_clear_bits(DUB_FLASH_CUE);
    } else if (fm1_ms >= g.home_morph_flash_ms)
        dub_home_morph_flash_arm(0u);
}

static int dub_home_morph_flash_visible(void)
{
    return dub_home_morph_flash_live_mask() != 0u;
}

static void dub_macro_morph_begin(void)
{
    dub_macro_morph_snapshot();
    if (!g.playing) {
        g.macro_morph_pos = DUB_MORPH_STEPS;
        g.macro_morph_acc = 0u;
    } else {
        g.macro_morph_pos = 0u;
        g.macro_morph_acc = 0u;
    }
}

static uint8_t dub_target_macro(uint8_t era, uint8_t plate, uint8_t idx)
{
    (void)plate;
    era = genre_clamp_era(era);
    if (idx >= DUB_NHOME_MACROS)
        return 0u;
    return DUB_ERA_MACROS[era][idx];
}

static void dub_era_home_macros_apply_era(uint8_t era, uint8_t morph)
{
    uint8_t i;

    if (fm_era_is_artist(era))
        era = genre_era_of_preset(g.cfg.p.plate);
    else
        era = genre_clamp_era(era);
    if (morph)
        dub_macro_morph_begin();
    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        uint8_t v = dub_target_macro(era, g.cfg.p.plate, i);
        uint8_t *mp = dub_macro_ptr(i);

        if (mp)
            *mp = v;
    }
    if (!morph) {
        g.macro_morph_pos = DUB_MORPH_STEPS;
        g.macro_morph_acc = 0u;
        for (i = 0; i < DUB_NHOME_MACROS; i++)
            g.macro_from[i] = dub_target_macro(era, g.cfg.p.plate, i);
    }
}

static int dub_home_macros_at_era_preset(void)
{
    uint8_t era = genre_clamp_era(g.cfg.p.era);
    uint8_t i;

    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        if (dub_raw_macro(i) != dub_target_macro(era, g.cfg.p.plate, i))
            return 0;
    }
    return 1;
}

static void mark_dirty(void);

static void dub_sel_sync_saved_lvls(void)
{
    const dub_params_t *p = &g.cfg.p;

    g.voice.saved_lane_lvl[0] = (uint8_t)(50u + ((uint32_t)p->kick_boom * 42u) / 127u);
    g.voice.saved_lane_lvl[1] = (uint8_t)(48u + ((uint32_t)p->rim_tone * 40u) / 127u);
    g.voice.saved_lane_lvl[2] = (uint8_t)(46u + ((uint32_t)p->hat_lvl * 38u) / 127u);
    g.voice.saved_lane_lvl[3] = (uint8_t)(52u + ((uint32_t)p->siren_pitch * 36u) / 127u);
    g.voice.saved_lane_lvl[4] = p->lvl_b ? p->lvl_b : 96u;
    g.voice.saved_lane_lvl[5] = p->lvl_c ? p->lvl_c : 92u;
}

static void dub_home_randomize_morph(void)
{
    uint8_t i;

    dub_macro_morph_begin();
    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        uint8_t *mp = dub_macro_ptr(i);

        if (mp) {
            g.seq_rng = g.seq_rng * 1664525u + 1013904223u;
            *mp = (uint8_t)((g.seq_rng >> 16) & 127u);
        }
    }
    mark_dirty();
}

static uint8_t dub_macro_morph_cap(void)
{
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    if (g.fill_build || g.fill_build_snap)
        return HOUSE_FILL_MORPH_STEPS;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    if (g.fill_build || g.fill_build_snap)
        return idm_fill_morph_steps();
#endif
    return DUB_MORPH_STEPS;
}

#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
static int house_fill_build_live(void)
{
    return g.fill_build != 0u || g.fill_build_snap != 0u;
}

static void house_fill_build_targets(void)
{
    uint8_t *mp;

    mp = dub_macro_ptr(2u);
    if (mp)
        *mp = 127u;
    mp = dub_macro_ptr(3u);
    if (mp)
        *mp = 127u;
    mp = dub_macro_ptr(5u);
    if (mp)
        *mp = 0u;
}

static void house_fill_build_restore(void)
{
    uint8_t i;

    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        uint8_t *mp = dub_macro_ptr(i);

        if (mp)
            *mp = g.macro_from[i];
    }
    g.macro_morph_pos = DUB_MORPH_STEPS;
    g.macro_morph_acc = 0u;
    g.fill_build = 0u;
    g.fill_build_snap = 0u;
    mark_dirty();
    g.ui_dirty = 1u;
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_HOUSE_FILL_TEST");

        if (e && e[0])
            printf("HOUSE_FILL restored\n");
    }
#endif
}

static void house_fill_build_start(uint8_t held_g)
{
    if (held_g != GEST_FILL || g.cfg.p.density >= 100u)
        return;
    if (g.view != VIEW_HOME || !g.playing || house_fill_build_live())
        return;
    dub_macro_morph_begin();
    house_fill_build_targets();
    g.fill_build = 1u;
    g.fill_build_snap = 0u;
    mark_dirty();
    g.ui_dirty = 1u;
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_HOUSE_FILL_TEST");

        if (e && e[0])
            printf("HOUSE_FILL morph_on\n");
    }
#endif
}

static void house_fill_build_release(void)
{
    if (!g.fill_build)
        return;
    g.fill_build_snap = 1u;
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_HOUSE_FILL_TEST");

        if (e && e[0] && g.gesture_latch == GEST_NONE)
            printf("HOUSE_FILL no_latch\n");
    }
#endif
}

static void house_fill_build_bar(void)
{
    if (g.fill_build_snap)
        house_fill_build_restore();
}

static int house_fill_latch_block(uint8_t gi)
{
    return gi == 0u && g.view == VIEW_HOME;
}
#endif

#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static int idm_stutter_edit_live(void)
{
    return g.fill_build != 0u || g.fill_build_snap != 0u || g.fill_drop != 0u;
}
#endif

static int dj_fill_live(void)
{
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    return house_fill_build_live();
#elif defined(GENRE_HIPHOP)
    return g.fill_build != 0u || g.fill_build_snap != 0u;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)
    return idm_stutter_edit_live();
#else
    return 0;
#endif
}

#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static uint8_t idm_u8_blend(uint8_t a, uint8_t b, uint8_t amt)
{
    if (amt >= 127u)
        return b;
    return (uint8_t)(((uint16_t)a * (127u - amt) + (uint16_t)b * amt) / 127u);
}

/* 0..127 build intensity. FILL: 4-bar ease-in²; BREAK: 1-bar linear. Snap freezes height. */
static uint8_t idm_fill_build_amt(void)
{
    uint8_t pos;
    uint8_t cap;
    uint32_t t;

    if (g.fill_drop)
        return 0u;
    if (!g.fill_build && !g.fill_build_snap)
        return 0u;
    cap = idm_fill_morph_steps();
    pos = g.macro_morph_pos;
    if (pos >= cap)
        return 127u;
    if (g.fill_build_kind == GEST_FILL) {
        t = ((uint32_t)pos << 15) / (uint32_t)(cap ? cap : 1u);
        t = (t * t) >> 15;
        return (uint8_t)((t * 127u) / 32768u);
    }
    return (uint8_t)((pos * 127u) / (cap ? cap : 1u));
}

static void idm_stutter_edit_targets(void)
{
    uint8_t *mp;
    uint8_t kind = g.fill_build_kind;

    /* FILL: mild SPACE/SWEEP swell only. BREAK: duck space. No HACK/REVERS. */
    mp = dub_macro_ptr(2u); /* REVERS */
    if (mp)
        *mp = 8u;
    mp = dub_macro_ptr(3u); /* SWEEP */
    if (mp)
        *mp = (kind == GEST_BREAK) ? 48u : 96u;
    mp = dub_macro_ptr(4u); /* SPACE */
    if (mp)
        *mp = (kind == GEST_BREAK) ? 0u : 80u;
    mp = dub_macro_ptr(6u); /* GRIT */
    if (mp)
        *mp = (kind == GEST_BREAK) ? 12u : 40u;
}

static void idm_fill_drop_targets(void)
{
    uint8_t *mp;
    uint8_t kind = g.fill_build_kind;

    /* FILL drop = clear air. BREAK drop = full loud. No HACK spike. */
    mp = dub_macro_ptr(2u); /* REVERS */
    if (mp)
        *mp = (kind == GEST_BREAK) ? 28u : 16u;
    mp = dub_macro_ptr(3u); /* SWEEP */
    if (mp)
        *mp = (kind == GEST_BREAK) ? 64u : 48u;
    mp = dub_macro_ptr(4u); /* SPACE */
    if (mp)
        *mp = 8u;
    /* HACK (5) left alone */
    mp = dub_macro_ptr(6u); /* GRIT */
    if (mp)
        *mp = (kind == GEST_BREAK) ? 56u : 40u;
}

/* Morph voice into build/drop. Called from sync_voice_params after base+LFO. */
static void idm_apply_fill_break_voice(dub_params_t *vp)
{
    uint8_t amt;
    uint8_t kind = g.fill_build_kind;
    uint8_t di;

    if (!vp)
        return;
    if (g.fill_drop) {
        if (kind == GEST_BREAK) {
            /* Sudden loud on melodic stack — kick/snare levels untouched */
            vp->lvl_b = 127u;
            vp->lvl_c = 127u;
            vp->hat_lvl = idm_u8_blend(vp->hat_lvl, 110u, 96u);
            vp->siren_pitch = idm_u8_blend(vp->siren_pitch, 120u, 110u);
            vp->density = 120u;
            vp->pump = idm_u8_blend(vp->pump, 96u, 80u);
            vp->isolator = 64u;
            vp->rev_mix = idm_u8_blend(vp->rev_mix, 18u, 100u);
            vp->feedback = idm_u8_blend(vp->feedback, 22u, 100u);
            vp->dly_c = idm_u8_blend(vp->dly_c, 12u, 100u);
            vp->dly_b = idm_u8_blend(vp->dly_b, 8u, 100u);
            for (di = 2u; di < 4u; di++) {
                vp->drm_dly[di] = idm_u8_blend(vp->drm_dly[di], 10u, 100u);
                vp->drm_dens[di] = 118u;
            }
        } else {
            /* Clear air + max weight on kick/bass/chord/lead */
            vp->rev_mix = idm_u8_blend(vp->rev_mix, 6u, 120u);
            vp->tank = idm_u8_blend(vp->tank, 20u, 100u);
            vp->feedback = idm_u8_blend(vp->feedback, 10u, 120u);
            vp->dly_c = idm_u8_blend(vp->dly_c, 4u, 120u);
            vp->dly_b = idm_u8_blend(vp->dly_b, 4u, 120u);
            vp->dly_lvl = idm_u8_blend(vp->dly_lvl, 16u, 110u);
            for (di = 0u; di < 4u; di++)
                vp->drm_dly[di] = idm_u8_blend(vp->drm_dly[di], 4u, 120u);
            vp->lvl_b = 127u;
            vp->lvl_c = 127u;
            vp->kick_boom = 127u;
            vp->siren_pitch = idm_u8_blend(vp->siren_pitch, 124u, 120u);
            vp->hat_lvl = idm_u8_blend(vp->hat_lvl, 108u, 90u);
            vp->rim_tone = idm_u8_blend(vp->rim_tone, 108u, 90u);
            vp->density = 124u;
            vp->pump = 110u;
            vp->sub_oct = idm_u8_blend(vp->sub_oct, 110u, 90u);
            vp->reso_mode = idm_u8_blend(vp->reso_mode, 48u, 70u);
            vp->isolator = 64u;
            for (di = 0u; di < 4u; di++)
                vp->drm_dens[di] = 120u;
        }
        return;
    }

    amt = idm_fill_build_amt();
    if (amt < 4u)
        return;

    if (kind == GEST_BREAK) {
        /* Fade everything except kick + snare/rim */
        uint8_t duck = amt;

        vp->lvl_b = idm_u8_blend(vp->lvl_b, 0u, duck);
        vp->lvl_c = idm_u8_blend(vp->lvl_c, 0u, duck);
        vp->hat_lvl = idm_u8_blend(vp->hat_lvl, 0u, duck);
        vp->siren_pitch = idm_u8_blend(vp->siren_pitch, 0u, duck);
        vp->density = idm_u8_blend(vp->density, 8u, duck);
        vp->rev_mix = idm_u8_blend(vp->rev_mix, 0u, (uint8_t)((duck * 3u) / 4u));
        vp->feedback = idm_u8_blend(vp->feedback, 0u, (uint8_t)((duck * 3u) / 4u));
        vp->dly_c = idm_u8_blend(vp->dly_c, 0u, duck);
        vp->dly_b = idm_u8_blend(vp->dly_b, 0u, duck);
        vp->pump = idm_u8_blend(vp->pump, 0u, duck);
        for (di = 2u; di < 4u; di++) {
            vp->drm_dly[di] = idm_u8_blend(vp->drm_dly[di], 0u, duck);
            vp->drm_dens[di] = idm_u8_blend(vp->drm_dens[di], 0u, duck);
        }
    } else {
        /* FILL swell: dens/space first; delay/feedback only late & tame (~+50% denser) */
        uint8_t soft = (uint8_t)(((uint16_t)amt * (uint16_t)amt) / 127u);

        vp->density = idm_u8_blend(vp->density, 127u, soft);
        vp->hat_lvl = idm_u8_blend(vp->hat_lvl, 118u, (uint8_t)((soft * 3u) / 4u));
        vp->rim_tone = idm_u8_blend(vp->rim_tone, 110u, (uint8_t)((soft * 2u) / 3u));
        vp->reso_mode = idm_u8_blend(vp->reso_mode, 72u, (uint8_t)((soft * 2u) / 3u));
        vp->rev_mix = idm_u8_blend(vp->rev_mix, 72u, soft);
        vp->tank = idm_u8_blend(vp->tank, 72u, (uint8_t)((soft * 2u) / 3u));
        vp->cutoff_c = idm_u8_blend(vp->cutoff_c, 110u, (uint8_t)((soft * 2u) / 3u));
        for (di = 0u; di < 4u; di++)
            vp->drm_dens[di] = idm_u8_blend(vp->drm_dens[di], 124u, soft);
        /* Late send climb — more feedback/delay, still below old crash caps */
        if (amt >= 36u) {
            uint8_t late = (uint8_t)(((uint16_t)(amt - 36u) * 127u) / 91u);

            vp->feedback = idm_u8_blend(vp->feedback, 72u, late);
            vp->dly_c = idm_u8_blend(vp->dly_c, 96u, late);
            vp->dly_b = idm_u8_blend(vp->dly_b, 72u, late);
            vp->dly_lvl = idm_u8_blend(vp->dly_lvl, 100u, late);
            for (di = 0u; di < 4u; di++) {
                vp->drm_dly[di] = idm_u8_blend(vp->drm_dly[di], 88u, late);
                vp->drm_rev[di] = idm_u8_blend(vp->drm_rev[di], 80u, (uint8_t)((late * 3u) / 4u));
            }
        }
    }
}

static void idm_stutter_edit_restore(void)
{
    uint8_t i;

    for (i = 0; i < DUB_NHOME_MACROS; i++) {
        uint8_t *mp = dub_macro_ptr(i);

        if (mp)
            *mp = g.macro_from[i];
    }
    g.macro_morph_pos = DUB_MORPH_STEPS;
    g.macro_morph_acc = 0u;
    g.fill_build = 0u;
    g.fill_build_snap = 0u;
    g.fill_drop = 0u;
    g.fill_build_kind = GEST_NONE;
    g.idm_edit_latch = 0u;
    g.idm_stutter.active = 0u;
    g.idm_stutter.slice_left = 0u;
    mark_dirty();
    g.ui_dirty = 1u;
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_IDM_STUTTER_TEST");

        if (e && e[0])
            printf("IDM_STUTTER restored\n");
    }
#endif
}

static void idm_stutter_start(uint8_t held_g)
{
    if (held_g != GEST_FILL && held_g != GEST_BREAK)
        return;
    if (g.view != VIEW_HOME || !g.playing || idm_stutter_edit_live())
        return;
    g.fill_build_kind = held_g;
    dub_macro_morph_begin();
    idm_stutter_edit_targets();
    g.fill_build = 1u;
    g.fill_build_snap = 0u;
    g.fill_drop = 0u;
    /* Stutter wash only on FILL build — BREAK is silence fade */
    g.idm_stutter.active = (held_g == GEST_FILL) ? 1u : 0u;
    g.idm_stutter.slice_left = 0u;
    mark_dirty();
    g.ui_dirty = 1u;
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_IDM_STUTTER_TEST");

        if (e && e[0])
            printf("IDM_%s morph_on\n", held_g == GEST_BREAK ? "BREAK" : "FILL");
    }
#endif
}

static void idm_stutter_release(void)
{
    if (!g.fill_build)
        return;
    g.fill_build_snap = 1u;
    /* Freeze curve height — no leap to peak before drop */
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_IDM_STUTTER_TEST");

        if (e && e[0] && g.gesture_latch == GEST_NONE && !g.idm_edit_latch)
            printf("IDM_STUTTER snap_wait\n");
    }
#endif
}

static void idm_stutter_bar(void)
{
    if (g.fill_drop) {
        idm_stutter_edit_restore();
        return;
    }
    if (g.fill_build_snap) {
        /* Quantized drop: one bar of impact, then restore on next bar */
        g.fill_build = 0u;
        g.fill_build_snap = 0u;
        g.fill_drop = 1u;
        g.idm_stutter.active = 0u;
        g.idm_stutter.slice_left = 0u;
        idm_fill_drop_targets();
        g.macro_morph_pos = DUB_MORPH_STEPS;
        mark_dirty();
        g.ui_dirty = 1u;
#if defined(DINGHY_HOST)
        {
            const char *e = getenv("FM1_IDM_STUTTER_TEST");

            if (e && e[0])
                printf("IDM_%s drop\n", g.fill_build_kind == GEST_BREAK ? "BREAK" : "FILL");
        }
#endif
    }
}

static void idm_edit_latch_begin(void)
{
    if (g.view != VIEW_HOME || !g.playing)
        return;
    if (!idm_stutter_edit_live()) {
        dub_macro_morph_begin();
        idm_stutter_edit_targets();
    }
    g.idm_edit_latch = 1u;
    mark_dirty();
    g.ui_dirty = 1u;
#if defined(DINGHY_HOST)
    {
        const char *e = getenv("FM1_IDM_STUTTER_TEST");

        if (e && e[0])
            printf("IDM_EDIT latch_on\n");
    }
#endif
}

static int idm_stutter_latch_block(uint8_t gi, uint8_t edit)
{
    if (gi != 0u || g.view != VIEW_HOME)
        return 0;
    (void)edit;
    return 1;
}

static uint8_t idm_rev_stutter_flip(uint8_t era, uint8_t rev)
{
    if (rev < 8u)
        return 0u;
    if (rev >= 96u)
        return 1u;
    if (era >= (uint8_t)ERA_DRILL)
        return ((g.step & 3u) == 3u && rev >= 32u) ? 1u : 0u;
    if (era == (uint8_t)ERA_OBLIQ)
        return (((g.bar ^ g.step) & 1u) && rev >= 40u) ? 1u : 0u;
    if (rev >= 72u && ((g.step & 7u) == 5u))
        return 1u;
    if (rev >= 48u && ((g.step & 1u) == 1u))
        return 1u;
    return 0u;
}

/* Sparse rhythmic windows: short burst, long silence. amt raises hit rate only. */
static uint8_t idm_glitch_burst_hit(uint8_t amt, uint8_t screw)
{
    uint8_t st = (uint8_t)(g.step & 15u);
    uint8_t period;
    uint8_t dur;
    uint8_t phase;

    if (screw) {
        /* SCREW: freeze blasts every 16→8→4 steps; 1 step wide (2 at max) */
        if (amt >= 100u)
            period = 4u;
        else if (amt >= 72u)
            period = 8u;
        else
            period = 16u;
        dur = (amt >= 110u) ? 2u : 1u;
        phase = (uint8_t)((g.bar & 1u) ? 0u : (period >> 1)); /* alternate bar offset */
    } else {
        /* HACK: syncopated chops every 8→4→2; always 1-step blip */
        if (amt >= 100u)
            period = 2u;
        else if (amt >= 72u)
            period = 4u;
        else
            period = 8u;
        dur = 1u;
        phase = 3u % period; /* off-beat: …x……x… */
    }
    return ((uint8_t)((st + 16u - phase) % period) < dur) ? 1u : 0u;
}

static void idm_stutter_sync_runtime(void)
{
    uint8_t era = genre_clamp_era(g.voice.p.era);
    uint8_t hack = dub_eff_macro(5u);  /* HACK: stutter wet / slices */
    uint8_t rev = dub_eff_macro(2u);   /* REVERS: audio reverse amount */
    uint8_t screw = dub_eff_macro(7u); /* SCREW: freeze hold */
    uint8_t wet;
    uint8_t vel;
    uint8_t mst;
    uint8_t gest = idm_stutter_edit_live() || g.idm_edit_latch;
    /* Higher gates: era presets stay below; user must dial into stutter */
    uint8_t hack_gate = (era >= (uint8_t)ERA_GLITCH) ? 40u : 56u;
    uint8_t screw_gate = (era >= (uint8_t)ERA_GLITCH) ? 44u : 60u;
    uint8_t hack_on = (hack >= hack_gate) ? 1u : 0u;
    uint8_t screw_on = (screw >= screw_gate) ? 1u : 0u;

    if (!gest && !hack_on && !screw_on) {
        g.idm_stutter.active = 0u;
        g.idm_stutter.reverse = 0u;
        return;
    }

    mst = seq_lane_step_map(g.step, 1u);
    if (gest) {
        uint8_t amt = idm_fill_build_amt();
        uint8_t burst_amt;

        /* BREAK duck / drop: no stutter — FILL only after early swell */
        if (g.fill_build_kind != GEST_FILL || g.fill_drop || amt < 32u) {
            g.idm_stutter.active = 0u;
            g.idm_stutter.reverse = 0u;
            return;
        }
        /* Rhythmic hits — ~50% denser than prior sparse map; silence between */
        burst_amt = (amt < 48u) ? 72u : (amt < 80u ? 100u : 110u);
        if (!idm_glitch_burst_hit(burst_amt, 0u)) {
            g.idm_stutter.active = 0u;
            g.idm_stutter.reverse = 0u;
            return;
        }
        wet = (uint8_t)(24u + ((uint16_t)amt * 30u) / 127u); /* 24..54 */
        if (wet > 54u)
            wet = 54u;
        vel = (uint8_t)(80u + ((127u - amt) >> 3)); /* short chops */
        if (g.step_vel[1][mst] > 0u)
            vel = (uint8_t)(((uint16_t)vel + (uint16_t)g.step_vel[1][mst]) >> 1);
        g.idm_stutter.active = 1u;
        g.idm_stutter.mode = (uint8_t)IDM_STUT_HACK;
        g.idm_stutter.reverse = 0u;
        if (amt >= 96u && rev >= 56u && ((g.step & 7u) == 7u))
            g.idm_stutter.reverse = 1u;
    } else if (screw_on) {
        if (!idm_glitch_burst_hit(screw, 1u)) {
            g.idm_stutter.active = 0u;
            g.idm_stutter.reverse = 0u;
            return;
        }
        /* SCREW: long freeze — soft env + LP iso in mixer */
        wet = (uint8_t)(28u + ((uint16_t)(screw - screw_gate) * 48u) / (127u - screw_gate));
        if (wet > 72u)
            wet = 72u;
        vel = (uint8_t)(20u + ((127u - screw) >> 3)); /* long slices */
        g.idm_stutter.active = 1u;
        g.idm_stutter.mode = (uint8_t)IDM_STUT_SCREW;
        g.idm_stutter.reverse = (rev >= 72u) ? idm_rev_stutter_flip(era, (uint8_t)(rev >> 1)) : 0u;
    } else {
        if (!idm_glitch_burst_hit(hack, 0u)) {
            g.idm_stutter.active = 0u;
            g.idm_stutter.reverse = 0u;
            return;
        }
        /* HACK: short HP click — hard env in mixer */
        wet = (uint8_t)(22u + ((uint16_t)(hack - hack_gate) * 40u) / (127u - hack_gate));
        if (wet > 60u)
            wet = 60u;
        vel = (uint8_t)(110u + ((127u - hack) >> 4)); /* very short slices */
        g.idm_stutter.active = 1u;
        g.idm_stutter.mode = (uint8_t)IDM_STUT_HACK;
        g.idm_stutter.reverse = idm_rev_stutter_flip(era, rev);
    }
    g.idm_stutter.wet = wet;
    g.idm_stutter.slice_len = idm_stutter_slice_from_vel(vel, era);
}

static void idm_edit_apply_drums(uint8_t on[DUB_SEQ_LANES], uint8_t vel[DUB_SEQ_LANES], uint8_t st)
{
    uint8_t dens = dub_eff_macro(3u);
    uint8_t ln;

    if (!g.idm_edit_latch || dens < 16u)
        return;
    for (ln = 0u; ln < 3u; ln++) {
        uint8_t mst = seq_lane_step_map(st, ln);
        uint32_t r = g.seq_rng ^ ((uint32_t)st << 8) ^ ((uint32_t)ln << 16);

        g.seq_rng = r * 1664525u + 1013904223u;
        if ((g.seq_rng & 127u) < (dens >> 1)) {
            on[ln] = 1u;
            vel[ln] = (uint8_t)(48u + ((g.seq_rng >> 7) & 79u));
        }
        if (on[ln] && ((g.seq_rng >> 11) & 127u) < dens) {
            int8_t nudge = (int8_t)((g.seq_rng >> 18) & 7u);

            if (nudge > 3)
                nudge = (int8_t)(nudge - 8);
            g.step_micro[ln][mst] = (int8_t)(g.step_micro[ln][mst] + nudge);
        }
    }
}

static void idm_stutter_process_buffer(int32_t *out, uint32_t n)
{
    uint32_t i;
    uint8_t era = genre_clamp_era(g.voice.p.era);
    uint8_t degr;
    uint8_t gest = idm_stutter_edit_live() || g.idm_edit_latch;
    uint8_t hack = dub_eff_macro(5u);

    if (!g.idm_stutter.active && !g.idm_edit_latch)
        return;
    degr = idm_degrade_amount(era, dub_eff_macro(6u), dub_eff_macro(3u));
    for (i = 0u; i < n; i++) {
        int32_t mono = (out[2u * i] + out[2u * i + 1u]) >> 1;

        idm_stutter_push(&g.idm_stutter, mono);
        if (g.idm_stutter.active) {
            mono = idm_stutter_mix(&g.idm_stutter, mono, g.idm_stutter.wet);
            /* No degrade on FILL swell; HACK+GRIT only at high dial */
            if (!gest && hack >= 96u && dub_eff_macro(6u) >= 88u)
                mono = idm_degrade_sample(mono, &g.synth.noise, (uint8_t)(degr >> 3));
        } else if (g.idm_edit_latch)
            mono = idm_degrade_sample(mono, &g.synth.noise, (uint8_t)(degr >> 3));
        out[2u * i] = mono;
        out[2u * i + 1u] = mono;
    }
}
#endif

static void dub_macro_morph_clock(uint32_t samples)
{
    uint32_t step_smp;
#if defined(GENRE_HOUSE)
    uint8_t morph_cap = g.fill_build ? HOUSE_FILL_MORPH_STEPS : DUB_MORPH_STEPS;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    uint8_t morph_cap;

    /* Freeze FILL/BREAK height while snap-waiting for quantized drop */
    if (g.fill_build_snap)
        return;
    morph_cap = g.fill_build ? idm_fill_morph_steps() : DUB_MORPH_STEPS;
#else
    uint8_t morph_cap = DUB_MORPH_STEPS;
#endif

    if (g.macro_morph_pos >= morph_cap)
        return;
    step_smp = g.samples_per_step;
    if (step_smp < 32u)
        step_smp = 5512u;
    g.macro_morph_acc += samples;
    while (g.macro_morph_acc >= step_smp) {
        g.macro_morph_acc -= step_smp;
        if (g.macro_morph_pos < morph_cap) {
            g.macro_morph_pos++;
            g.ui_dirty = 1u;
            if (g.macro_morph_pos >= morph_cap)
                dub_home_morph_flash_end_check();
        }
    }
}

static void dub_macro_knob_sync(uint8_t idx)
{
    if (idx >= DUB_NHOME_MACROS || g.macro_morph_pos >= DUB_MORPH_STEPS)
        return;
    g.macro_from[idx] = dub_raw_macro(idx);
}

static const char *dub_arp_mode_toast_label(uint8_t mode);

static void fmt_motion_macro(uint8_t v, char *out)
{
    const char *pfx;
    char tier1[5];

    if (out == 0)
        return;
    if (v >= 112u)
        pfx = "LOP ";
    else if (v >= 88u)
        pfx = "ROL ";
    else if (v >= 62u)
        pfx = "DOT ";
    else if (v >= 36u)
        pfx = "CHP ";
    else if (v >= 14u) {
        const char *nm = dub_arp_mode_toast_label(1u);
        uint8_t i;

        /* 3-letter + space from genre ARP mode-1 name (DUST/CELL/JACK/…, was hard-coded SKK). */
        for (i = 0u; i < 3u; i++)
            tier1[i] = (nm != 0 && nm[i] != 0) ? nm[i] : ' ';
        tier1[3] = ' ';
        tier1[4] = 0;
        pfx = tier1;
    } else
        pfx = "OFF ";
    out[0] = pfx[0];
    out[1] = pfx[1];
    out[2] = pfx[2];
    out[3] = pfx[3];
    u8_show100_dec(out + 4, v);
}

static uint8_t dub_motion_arp_tier(uint8_t motion)
{
    if (motion <= 13u)
        return 0u;
    if (motion <= 35u)
        return 1u;
    if (motion <= 61u)
        return 2u;
    if (motion <= 87u)
        return 3u;
    if (motion <= 111u)
        return 4u;
    return 5u;
}

static void dub_morph_sync_field(uint8_t *field)
{
    uintptr_t off;

    if (g.morph_pos >= DUB_MORPH_STEPS)
        return;
    off = (uintptr_t)field - (uintptr_t)&g.cfg.p;
    if (off < sizeof(dub_params_t))
        ((uint8_t *)&g.morph_from)[off] = *field;
}

static void adj_u8_morph(uint8_t *field, int32_t d, uint8_t lo, uint8_t hi)
{
    adj_u8(field, d, lo, hi);
    dub_morph_sync_field(field);
}

static void apply_home_macro_delta(uint8_t pg, uint8_t slot, int32_t delta)
{
    dub_params_t *p = &g.cfg.p;
    int32_t half = delta >= 0 ? (delta + 1) / 2 : (delta - 1) / 2;

    if (delta == 0)
        return;
    if (pg == 0u) {
        if (slot == 0) {
#if defined(GENRE_POP)
            /* GATE: shorter chord + lead bodies, dry */
            adj_u8_morph(&p->decay_c, -delta, 8, 127);
            adj_u8_morph(&p->drm_dec[3], -half, 8, 127);
#else
            /* SKANK / JACK: chord snap; house also nudges pump */
            adj_u8_morph(&p->cutoff_c,  delta, 0, 127);
            adj_u8_morph(&p->reso_mode, half,  0, 127);
            adj_u8_morph(&p->timbre,    half,  0, 127);
            adj_u8_morph(&p->env_filt,  half,  0, 127);
            adj_u8_morph(&p->dly_c,     half,  0, 127);
#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP)
            adj_u8_morph(&p->pump,      half,  0, 127);
#endif
#endif
        } else if (slot == 1) {
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            /* CROP: wandering seq window — runtime sync, not param morph */
            (void)half;
#elif defined(GENRE_HIPHOP)
            /* DUST: vinyl wash — rev/sends/wow, no feedback scream */
            adj_u8_morph(&p->rev_mix,  half, 0, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dly[di], half, 0, 127);
            }
            adj_u8_morph(&p->wow,      half, 0, 127);
#elif defined(GENRE_POP)
            /* LINE: dry tone + short bodies, no wash */
            adj_u8_morph(&p->timbre, half, 0, 127);
            adj_u8_morph(&p->decay_c, -half, 8, 127);
            adj_u8_morph(&p->decay_b, -(half >> 1), 8, 127);
            adj_u8_morph(&p->rev_tail, -(half >> 1), 0, 127);
#elif !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ)
            /* LAYER: wash via rev/sends/wow. Dub also raises feedback; house does not. */
            adj_u8_morph(&p->feedback, half, 0, 110);
            adj_u8_morph(&p->rev_mix,  half, 0, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dly[di], half, 0, 127);
            }
            adj_u8_morph(&p->wow,      half, 0, 127);
#else
            adj_u8_morph(&p->rev_mix,  half, 0, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dly[di], half, 0, 127);
            }
            adj_u8_morph(&p->wow,      half, 0, 127);
#endif
        } else if (slot == 2) {
#if defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            /* HOP: hard stereo hops + short gate feel (Snowcrash signature) */
            adj_u8_morph(&p->pan[0], half, 1, 127);
            adj_u8_morph(&p->pan[1], -half, 1, 127);
            adj_u8_morph(&p->drm_pan[1], half, 1, 127);
            adj_u8_morph(&p->drm_pan[3], -half, 1, 127);
            adj_u8_morph(&p->decay_c, -half, 8, 127);
            adj_u8_morph(&p->decay_b, -(half >> 1), 8, 127);
            adj_u8_morph(&p->glide_c, half >> 1, 0, 127);
            adj_u8_morph(&p->wow, half >> 2, 0, 127);
#elif defined(GENRE_IDM)
            /* REVERS: audio reverse — slight wow + glide morph */
            adj_u8_morph(&p->wow, half, 0, 127);
            adj_u8_morph(&p->glide_c, half, 0, 127);
#elif defined(GENRE_HIPHOP)
            /* ROLL: hat density + open/brightness — trap rolls / swung 16ths */
            adj_u8_morph(&p->drm_dens[2], delta, 0, 127);
            adj_u8_morph(&p->density, half, 0, 127);
            adj_u8_morph(&p->drm_cut[2], half, 0, 127);
            adj_u8_morph(&p->hat_lvl, half, 0, 127);
#elif defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ)
            adj_u8_morph(&p->cutoff_b, delta, 0, 127);
            adj_u8_morph(&p->cutoff_c, half,  0, 127);
            {
                uint8_t di;
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_cut[di], half, 0, 127);
            }
#elif defined(GENRE_POP)
            /* CHOR: hook brightness + light wobble, no chord cutoff */
            adj_u8_morph(&p->wobble, delta, 0, 127);
            adj_u8_morph(&p->siren_rate, delta, 0, 127);
#else
            adj_u8_morph(&p->wobble,   delta, 0, 127);
            adj_u8_morph(&p->cutoff_b, half,  0, 127);
            adj_u8_morph(&p->drive,    half,  0, 127);
#endif
        } else {
            /* SWEEP: sweeps master isolator + filter cutoffs */
            adj_u8_morph(&p->isolator, delta, 0, 127);
            adj_u8_morph(&p->cutoff_c, half,  0, 127);
            adj_u8_morph(&p->cutoff_b, half,  0, 127);
        }
    } else if (pg == 1u) {
        if (slot == 0) {
#if defined(GENRE_POP)
            /* DELAY: echo throw — sends + feedback, no wash */
            adj_u8_morph(&p->dly_c, delta, 0, 127);
            adj_u8_morph(&p->feedback, half, 0, 110);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dly[di], half, 0, 127);
            }
#else
            /* SPACE: expands reverb mix, tank size & chord/drum delay sends */
            adj_u8_morph(&p->rev_mix, delta, 0, 127);
            adj_u8_morph(&p->tank,    half,  0, 127);
            adj_u8_morph(&p->dly_c,   half,  0, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dly[di], half, 0, 127);
            }
#endif
        } else if (slot == 1) {
#if defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
            /* CLICK: hat/rim click density + wire; soft thin-iso only */
            adj_u8_morph(&p->drm_dens[1], delta, 0, 127);
            adj_u8_morph(&p->drm_dens[2], delta, 0, 127);
            adj_u8_morph(&p->rim_tone, half, 0, 127);
            adj_u8_morph(&p->hat_lvl, half, 0, 127);
            adj_u8_morph(&p->density, half >> 1, 0, 127);
            adj_u8_morph(&p->isolator, half > 0 ? (half >> 2) : (int32_t)(-((-half) >> 2)), 56, 96);
            adj_u8_morph(&p->drive, half >> 1, 0, 127);
#elif defined(GENRE_IDM)
            /* HACK: thin iso (HP-ish) + click density */
            {
                uint8_t di;

                adj_u8_morph(&p->density, half, 0, 127);
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dens[di], half, 0, 127);
            }
            adj_u8_morph(&p->isolator, half > 0 ? half : (int32_t)(-((-half) >> 1)), 64, 110);
            adj_u8_morph(&p->drive, half, 0, 127);
#elif defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP)
            /* PUMP / DUCK: sidechain on bass+chord; kick stays */
            adj_u8_morph(&p->pump, delta, 0, 127);
#else
            /* WEIGHT: boosts sub octave, kick boom, pump & bass decay */
            adj_u8_morph(&p->sub_oct,   delta, 0, 127);
            adj_u8_morph(&p->kick_boom, half,  0, 127);
            adj_u8_morph(&p->pump,      half,  0, 127);
            adj_u8_morph(&p->decay_b,   half,  0, 127);
#endif
        } else if (slot == 2) {
#if defined(GENRE_POP)
            /* BRITE: lead hook brightness; pad stays dark */
            adj_u8_morph(&p->siren_rate, delta, 0, 127);
            adj_u8_morph(&p->drive, half >> 1, 0, 127);
#else
            /* GRIT: drives bass saturation, kit drive, chord timbre/reso & tape wow */
            adj_u8_morph(&p->drive,     delta, 0, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++) {
#if defined(GENRE_HOUSE) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
                    if (di == 3u)
                        adj_u8_morph(&p->siren_rate, half, 0, 127);
                    else
#endif
                        adj_u8_morph(&p->drm_drv[di], half, 0, 127);
                }
            }
            adj_u8_morph(&p->timbre,    half,  0, 127);
            adj_u8_morph(&p->reso_mode, half,  0, 127);
            adj_u8_morph(&p->wow,       half,  0, 127);
#endif
        } else {
#ifdef GENRE_HEAVY_PSYCH
            adj_u8_morph(&p->density,   delta, 0, 127);
            adj_u8_morph(&p->feedback,  half,  0, 127);
            {
                uint8_t di;
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dens[di], half, 0, 127);
            }
#elif defined(GENRE_HOUSE)
            /* FLOOR: density and kick up; feedback stays */
            adj_u8_morph(&p->density,   delta, 0, 127);
            adj_u8_morph(&p->kick_boom, half,  0, 127);
            {
                uint8_t di;
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dens[di], half, 0, 127);
            }
#elif defined(GENRE_HIPHOP)
            /* CHOP: denser hits / snare crack — no feedback scream */
            adj_u8_morph(&p->density,   delta, 0, 127);
            adj_u8_morph(&p->kick_boom, half,  0, 127);
            {
                uint8_t di;
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dens[di], half, 0, 127);
            }
#elif defined(GENRE_SNOWCRASH)
            /* HUM: LP iso freeze + 50/60 Hz hum-bass bias; light decay, no feedback scream */
            adj_u8_morph(&p->isolator, half > 0 ? (int32_t)(-(half)) : (-half >> 1), 18, 64);
            adj_u8_morph(&p->sub_oct, half, 0, 127);
            adj_u8_morph(&p->cutoff_b, half > 0 ? -half : half, 0, 72);
            adj_u8_morph(&p->decay_b, half >> 1, 0, 127);
            adj_u8_morph(&p->wow, (half > 0 ? half : -half) >> 1, 0, 127);
            if (half > 0 && p->bass_model != (uint8_t)BASS_HUM) {
                if (p->bass_model < (uint8_t)BASS_HUM)
                    p->bass_model = (uint8_t)(p->bass_model + 1u);
                else
                    p->bass_model = (uint8_t)(p->bass_model - 1u);
                dub_morph_sync_field(&p->bass_model);
            }
#elif defined(GENRE_IDM)
            /* SCREW: LP iso freeze + long envelope feel; never feedback scream */
            adj_u8_morph(&p->isolator, half > 0 ? (int32_t)(-(half)) : (-half >> 1), 18, 64);
            adj_u8_morph(&p->decay_c, half, 0, 127);
            adj_u8_morph(&p->decay_b, half, 0, 127);
            adj_u8_morph(&p->wow, (half > 0 ? half : -half) >> 1, 0, 127);
#elif defined(GENRE_POP)
            /* LEVEL: hook vs chord balance, dry (mix only — not siren_pitch / lead octave) */
            adj_u8_morph(&p->lvl_c, -half, 0, 127);
            adj_u8_morph(&p->lvl_b, half >> 1, 0, 127);
#elif defined(GENRE_TECHNO)
            /* GATE: shorter chord + lead bodies, dry */
            adj_u8_morph(&p->decay_c, -delta, 8, 127);
            adj_u8_morph(&p->drm_dec[3], -half, 8, 127);
#elif defined(GENRE_BASS)
            /* FLOOR: density and kick up; feedback stays */
            adj_u8_morph(&p->density,   delta, 0, 127);
            adj_u8_morph(&p->kick_boom, half,  0, 127);
            {
                uint8_t di;
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dens[di], half, 0, 127);
            }
#elif defined(GENRE_SOUL)
            /* SHOUT: lead level/drive up; invert soft/dark */
            if (half < 0) {
                adj_u8_morph(&p->cutoff_c, half, 0, 127);
                adj_u8_morph(&p->timbre, half >> 1, 0, 127);
                adj_u8_morph(&p->lvl_c, half >> 1, 0, 127);
                adj_u8_morph(&p->drive, half >> 1, 0, 127);
            } else {
                adj_u8_morph(&p->lvl_c, delta, 0, 127);
                adj_u8_morph(&p->drive, half, 0, 127);
                adj_u8_morph(&p->timbre, half >> 1, 0, 127);
                adj_u8_morph(&p->siren_rate, half >> 1, 0, 127);
                {
                    uint8_t di;
                    for (di = 0; di < 4u; di++)
                        adj_u8_morph(&p->drm_drv[di], half >> 1, 0, 127);
                }
            }
#elif defined(GENRE_JAZZ)
            /* MUTE: shorter bodies, drier */
            adj_u8_morph(&p->decay_c, -delta, 8, 127);
            adj_u8_morph(&p->decay_b, -half, 8, 127);
            adj_u8_morph(&p->rev_mix, -half, 0, 127);
            adj_u8_morph(&p->dly_c, -half, 0, 127);
#elif defined(GENRE_FUNK)
            /* THUMP: kick body + pump */
            adj_u8_morph(&p->kick_boom, delta, 0, 127);
            adj_u8_morph(&p->pump, half, 0, 127);
            adj_u8_morph(&p->sub_oct, half, 0, 127);
            adj_u8_morph(&p->decay_b, half, 0, 127);
#elif defined(GENRE_TRIPHOP)
            /* FILM: longer tails */
            adj_u8_morph(&p->rev_tail, delta, 0, 127);
            adj_u8_morph(&p->rev_mix, half, 0, 127);
            adj_u8_morph(&p->decay_c, half, 0, 127);
#else
            {
                uint8_t di;

                adj_u8_morph(&p->density, -half, 0, 127);
                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dens[di], -half, 0, 127);
            }
            adj_u8_morph(&p->feedback,   half, 0, 127);
            adj_u8_morph(&p->dly_c,      half, 0, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_dly[di], half, 0, 127);
            }
            adj_u8_morph(&p->loop_filt, -half, 0, 127);
#endif
        }
    } else if (pg == 2u) {
        if (slot == 0) {
#if defined(GENRE_POP)
            /* GLIDE: hook slides + light stereo */
            adj_u8_morph(&p->glide_c, delta, 0, 127);
            adj_u8_morph(&p->pan[1], half, 1, 127);
#else
            /* MOTION / DRIFT: glide, swing feel, stereo drift (always audible while playing) */
            adj_u8_morph(&p->glide_c, delta, 0, 127);
            adj_u8_morph(&p->swing, half, 50, 75);
            adj_u8_morph(&p->pan[0], half, 1, 127);
            adj_u8_morph(&p->pan[1], -half, 1, 127);
            {
                uint8_t di;

                for (di = 0; di < 4u; di++)
                    adj_u8_morph(&p->drm_pan[di], half, 1, 127);
            }
#endif
        } else if (slot == 1) {
#if defined(GENRE_POP)
            /* DENS: hat/rim presence + light swing */
            adj_u8_morph(&p->drm_dens[2], delta, 0, 127);
            adj_u8_morph(&p->drm_dens[1], half, 0, 127);
            adj_u8_morph(&p->hat_lvl, half, 0, 127);
            adj_u8_morph(&p->swing, half >> 1, 50, 75);
#else
            /* FLUX: wandering filters, tape, echo motion */
            adj_u8_morph(&p->wobble, delta, 0, 127);
            adj_u8_morph(&p->wow, half, 0, 127);
            adj_u8_morph(&p->loop_filt, half, 0, 127);
            adj_u8_morph(&p->cutoff_c, half, 0, 127);
            adj_u8_morph(&p->cutoff_b, half, 0, 127);
            adj_u8_morph(&p->feedback, half, 0, 127);
#endif
        }
    }
}

/* Triangle -128..+127 from wall clock; 32-bit only, no 64-bit division. */
static int32_t dub_ms_tri(uint32_t ms, uint32_t period_ms)
{
    uint32_t t;
    uint32_t half;

    if (period_ms < 4u)
        period_ms = 4u;
    t = ms % period_ms;
    half = period_ms >> 1;
    if (half < 2u)
        half = 2u;
    if (t < half)
        return (int32_t)((t * 256u) / half) - 128;
    return 128 - (int32_t)(((t - half) * 256u) / (period_ms - half));
}

/* Global grid (sample clock, swing, bar/beat markers) is always 1/16 = 6 ticks (24 ppqn). */
#define DUB_SEQ_GRID_TICKS 6u
#define DUB_SEQ_PACE_GRID 1u /* PACE index of 1/16: lane fires from the global seq_tick */

static void dub_lane_spp_refresh(void);
static void dub_lane_resync(uint8_t ln);

/* Effective PACE index of lane ln: cfg.seq_pace[ln] + LFO offset (only if SEQ_PACE is bound to ln and depth > 0).
 * Read-only on cfg. Changes take effect at the grid tick (see dub_lane_pace_lfo_sync). */
_Static_assert(FM_SEQ_PACE_N == DUB_PACE_IDX_MAX + 1u, "PACE clamp range must match FM_SEQ_PACE_N");

static uint8_t dub_lane_pace_eff(uint8_t ln)
{
    uint8_t base = g.cfg.seq_pace[ln % DUB_SEQ_LANES] % FM_SEQ_PACE_N;
    int32_t d = 0;

    if (g.seq_lfo_lane[DUB_SEQ_LFO_PACE_IX] == ln % DUB_SEQ_LANES && g.lfo_slots_eff[DUB_LFO_SEQ_PACE].depth > 0u)
        d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_SEQ_PACE);
    return dub_pace_eff(base, d);
}

/* Ticks (24 ppqn) of one step of lane ln: 3 = 1/32, 6 = 1/16, 48 = 1/2. */
static uint32_t dub_lane_ticks(uint8_t ln)
{
    return FM_SEQ_PACE4_TICKS[dub_lane_pace_eff(ln) % FM_SEQ_PACE_N];
}

/* 1 = lane runs its own clock (pace other than 1/16). GENRE_BASS has no per-lane clock. */
static int dub_lane_is_sub(uint8_t ln)
{
#if defined(GENRE_BASS)
    (void)ln;
    return 0;
#else
    return dub_lane_pace_eff(ln) != DUB_SEQ_PACE_GRID;
#endif
}

/* Lanes that fire from the global seq_tick (mask bit per lane). */
static uint8_t dub_grid_lane_mask(void)
{
    uint8_t ln;
    uint8_t m = 0u;

    for (ln = 0u; ln < DUB_SEQ_LANES; ln++)
        if (!dub_lane_is_sub(ln))
            m |= (uint8_t)(1u << ln);
    return m;
}

static void sync_voice_params(void)
{
    static const uint8_t def_dp[4] = {64u, 74u, 52u, 82u};
    dub_params_t *vp = &g.voice.p;

    dub_dj_lfo_sync();
    dub_dj_fx_sync();
    /* Macro knob turns already adjust `g.cfg.p` via `apply_home_macro_delta()`,
     * so `sync_voice_params()` only adds LFO modulation (`fm_lfo_slot_delta`) on top — no double-counting! */
    int32_t m_ch = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_CHORD);
    int32_t m_fb = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_FEED);
    int32_t m_wb = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_WOB);
    int32_t m_is = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_ISO);
    int32_t m_sp = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_SPACE);
    int32_t m_wt = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_WEIGHT);
    int32_t m_gr = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_GRIT);
    int32_t m_db = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_DUB);
    int32_t dj_sp = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_SPACE);
    int32_t dj_db = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_DUB);

    m_sp += dj_sp;
    m_db += dj_db;
#if defined(GENRE_BASS)
    /* MAC LFOs go through the bass mod maps (bass_sync_voice_mod); no legacy dub couplings. */
    m_ch = m_fb = m_wb = m_gr = 0;
#endif
    uint8_t eff_motion = dub_eff_macro(8);
    uint8_t eff_flux = dub_eff_flux_engine();
#if defined(GENRE_BASS)
    (void)eff_motion;
    (void)eff_flux;
#endif
    uint32_t i;

    if (g.morph_pos < DUB_MORPH_STEPS)
        dub_morph_blend(vp, &g.morph_from, &g.cfg.p, g.morph_pos);
    else
        *vp = g.cfg.p;

    /* ENV / CHORD (Page 1 -> Page 2) */
    vp->voicing     = lfo_mod_enum(vp->voicing,   DUB_LFO_VOIC_C, VOICE_N);
#if defined(GENRE_POP)
    vp->cutoff_c    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->cutoff_c + (m_is >> 1), 0, 127), DUB_LFO_CUT_C, 0, 127),
                                  &g.dj, FM_DJ_TGT_CUTOFF, 0, 127);
    vp->decay_c     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->decay_c - (m_ch >> 2), 0, 127), DUB_LFO_DEC_C, 0, 127);
    vp->atk_c       = lfo_mod_u8(vp->atk_c, DUB_LFO_ATK_C, 0, 127);
    vp->dly_c       = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->dly_c + (m_sp >> 1) + (m_db >> 1), 0, 127), DUB_LFO_DLY_C, 0, 127);
    vp->timbre      = lfo_mod_u8(vp->timbre, DUB_LFO_TIMB_C, 0, 127);
    vp->reso_mode   = lfo_mod_u8(vp->reso_mode, DUB_LFO_RESO_C, 0, 127);
    vp->env_filt    = lfo_mod_u8(vp->env_filt, DUB_LFO_ENV_C, 0, 127);
#else
    vp->cutoff_c    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->cutoff_c + (m_ch * 3) / 4 + (m_is >> 1), 0, 127), DUB_LFO_CUT_C, 0, 127),
                                  &g.dj, FM_DJ_TGT_CUTOFF, 0, 127);
    vp->decay_c     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->decay_c + (m_ch >> 2), 0, 127), DUB_LFO_DEC_C, 0, 127);
    vp->atk_c       = lfo_mod_u8(vp->atk_c, DUB_LFO_ATK_C, 0, 127);
    vp->dly_c       = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->dly_c + (m_ch >> 1) + (m_sp >> 1) + (m_db >> 1), 0, 127), DUB_LFO_DLY_C, 0, 127);
    vp->timbre      = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->timbre + (m_ch >> 1) + (m_gr >> 1), 0, 127), DUB_LFO_TIMB_C, 0, 127);
    vp->reso_mode   = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->reso_mode + (m_ch >> 1) + (m_gr >> 1), 0, 127), DUB_LFO_RESO_C, 0, 127);
    vp->env_filt    = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->env_filt + (m_ch >> 1), 0, 127), DUB_LFO_ENV_C, 0, 127);
#endif
    vp->glide_c     = lfo_mod_u8(vp->glide_c, DUB_LFO_GLIDE_C, 0, 127);
    vp->pan[1]      = lfo_mod_u8(vp->pan[1] ? vp->pan[1] : 64u, DUB_LFO_PAN_C, 1, 127);
    vp->lvl_c       = lfo_mod_u8(vp->lvl_c, DUB_LFO_LVL_C, 0, 127);
    vp->rev_c       = lfo_mod_u8(vp->rev_c, DUB_LFO_REV_C, 0, 127);

    /* GLO / BASS (Page 1 -> Page 2) */
    vp->bass_model  = lfo_mod_enum(vp->bass_model, DUB_LFO_MODL_B, BASS_N);
#if defined(GENRE_POP)
    vp->cutoff_b    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->cutoff_b + (m_is >> 1), 0, 127), DUB_LFO_CUT_B, 0, 127),
                                  &g.dj, FM_DJ_TGT_CUTOFF, 0, 127);
    vp->decay_b     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->decay_b + (m_wt >> 1), 0, 127), DUB_LFO_DEC_B, 0, 127);
    vp->atk_b       = lfo_mod_u8(vp->atk_b, DUB_LFO_ATK_B, 0, 127);
    vp->wobble      = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->wobble + m_wb, 0, 127), DUB_LFO_WOB, 0, 127),
                                  &g.dj, FM_DJ_TGT_WOB, 0, 127);
    vp->sub_oct     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->sub_oct + (m_wt >> 1), 0, 127), DUB_LFO_SUB_B, 0, 127);
    vp->drive       = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->drive + (m_gr >> 2), 0, 127), DUB_LFO_DRV_B, 0, 127);
#else
    vp->cutoff_b    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->cutoff_b + (m_wb >> 1) + (m_is >> 1), 0, 127), DUB_LFO_CUT_B, 0, 127),
                                  &g.dj, FM_DJ_TGT_CUTOFF, 0, 127);
    vp->decay_b     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->decay_b + (m_wt >> 1), 0, 127), DUB_LFO_DEC_B, 0, 127);
    vp->atk_b       = lfo_mod_u8(vp->atk_b, DUB_LFO_ATK_B, 0, 127);
    vp->wobble      = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->wobble + m_wb, 0, 127), DUB_LFO_WOB, 0, 127),
                                  &g.dj, FM_DJ_TGT_WOB, 0, 127);
    vp->sub_oct     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->sub_oct + (m_wt >> 1), 0, 127), DUB_LFO_SUB_B, 0, 127);
    vp->drive       = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->drive + (m_wb >> 2) + (m_gr * 3) / 4, 0, 127), DUB_LFO_DRV_B, 0, 127);
#endif
    vp->pump        = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->pump + (m_wt * 3) / 4, 0, 127), DUB_LFO_PUMP_B, 0, 127),
                                  &g.dj, FM_DJ_TGT_WEIGHT, 0, 127);
    vp->dly_b       = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->dly_b + (m_db >> 2), 0, 127), DUB_LFO_DLY_B, 0, 127);
    vp->pan[0]      = lfo_mod_u8(vp->pan[0] ? vp->pan[0] : 64u, DUB_LFO_PAN_B, 1, 127);
    vp->lvl_b       = lfo_mod_u8(vp->lvl_b, DUB_LFO_LVL_B, 0, 127);
    vp->rev_b       = lfo_mod_u8(vp->rev_b, DUB_LFO_REV_B, 0, 127);

    /* SEL / DRUM (Page 1..4) */
    vp->density     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->density - (m_db * 3) / 4, 0, 127), DUB_LFO_DENS, 0, 127);
    vp->kick_boom   = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->kick_boom + (m_wt >> 1), 0, 127), DUB_LFO_KICK, 0, 127);
    vp->rim_tone    = lfo_mod_u8(vp->rim_tone,    DUB_LFO_RIM,    0, 127);
    vp->hat_lvl     = lfo_mod_u8(vp->hat_lvl,     DUB_LFO_HAT,    0, 127);
    vp->drum_kit    = lfo_mod_enum(vp->drum_kit,  DUB_LFO_KIT,    DKIT_N);
    {
        uint8_t j;
        int32_t dly_macro = (m_fb >> 2) + (m_sp >> 1) + (m_db >> 1);

        for (j = 0; j < 4u; j++) {
            uint8_t tun = vp->drm_tune[j] ? vp->drm_tune[j] : 64u;
            uint8_t dec = vp->drm_dec[j] ? vp->drm_dec[j] : 64u;
#if defined(GENRE_POP)
            int32_t drv = (int32_t)vp->drm_drv[j];
#else
            int32_t drv = fm_lfo_clamp_u8((int32_t)vp->drm_drv[j] + (m_gr >> 1), 0, 127);
#endif
            int32_t dly = fm_lfo_clamp_u8((int32_t)vp->drm_dly[j] + dly_macro, 0, 127);
            uint8_t cut = vp->drm_cut[j] ? vp->drm_cut[j] : 64u;
            uint8_t atk = vp->drm_atk[j];

            tun = lfo_mod_u8(tun, DUB_LFO_KTUNE, 1, 127);
            tun = lfo_mod_u8(tun, (uint8_t)(DUB_LFO_KTUNE_K + j), 1, 127);
#if defined(GENRE_POP)
            if (j == 3u)
                dec = fm_lfo_clamp_u8((int32_t)dec - (m_ch >> 2), 8, 127);
#endif
            dec = lfo_mod_u8(dec, DUB_LFO_KDEC, 1, 127);
            dec = lfo_mod_u8(dec, (uint8_t)(DUB_LFO_KDEC_K + j), 1, 127);
#if defined(GENRE_POP)
            if (j == 3u) {
                int32_t snap = fm_lfo_clamp_u8((int32_t)vp->siren_rate + m_wb + (m_gr >> 1), 0, 127);

                snap = lfo_mod_u8((uint8_t)snap, DUB_LFO_KDRV, 0, 127);
                snap = lfo_mod_u8((uint8_t)snap, DUB_LFO_KDRV_S, 0, 127);
                vp->siren_rate = (uint8_t)snap;
            } else
#elif defined(GENRE_HOUSE) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            if (j == 3u) {
                int32_t snap = fm_lfo_clamp_u8((int32_t)vp->siren_rate + (m_gr >> 1), 0, 127);

                snap = lfo_mod_u8((uint8_t)snap, DUB_LFO_KDRV, 0, 127);
                snap = lfo_mod_u8((uint8_t)snap, DUB_LFO_KDRV_S, 0, 127);
                vp->siren_rate = (uint8_t)snap;
            } else
#endif
            {
                drv = lfo_mod_u8((uint8_t)drv, DUB_LFO_KDRV, 0, 127);
                drv = lfo_mod_u8((uint8_t)drv, (uint8_t)(DUB_LFO_KDRV_K + j), 0, 127);
                vp->drm_drv[j] = (uint8_t)drv;
            }
            cut = lfo_mod_u8(cut, DUB_LFO_DCUT, 0, 127);
            cut = lfo_mod_u8(cut, (uint8_t)(DUB_LFO_DCUT_K + j), 0, 127);
            atk = lfo_mod_u8(atk, DUB_LFO_DATK, 0, 127);
            atk = lfo_mod_u8(atk, (uint8_t)(DUB_LFO_DATK_K + j), 0, 127);
            dly = lfo_mod_u8((uint8_t)dly, DUB_LFO_DLY_D, 0, 127);
            dly = lfo_mod_u8((uint8_t)dly, (uint8_t)(DUB_LFO_DLY_K + j), 0, 127);
            vp->drm_tune[j] = tun;
            vp->drm_dec[j] = dec;
            vp->drm_cut[j] = cut;
            vp->drm_atk[j] = atk;
            vp->drm_dly[j] = (uint8_t)dly;
            vp->drm_rev[j] = lfo_mod_u8(
                lfo_mod_u8(vp->drm_rev[j], DUB_LFO_REV_D, 0, 127),
                (uint8_t)(DUB_LFO_REV_K + j), 0, 127);
            {
                uint8_t dd = vp->drm_dens[j];

#if defined(GENRE_POP)
                if (j == 3u) {
                    /* Lane 3 = LEAD TIMBRE only — not DUB/density macro or master DENS LFO */
                    dd = lfo_mod_u8(dd, (uint8_t)(DUB_LFO_DENS_S), 0, 127);
                } else {
                    dd = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)dd - (m_db * 3) / 4, 0, 127), DUB_LFO_DENS, 0, 127);
                    dd = lfo_mod_u8(dd, (uint8_t)(DUB_LFO_DENS_K + j), 0, 127);
                    if (j == 0u)
                        dd = fm_dj_mod_u8(dd, &g.dj, FM_DJ_TGT_DENS_K, 0, 127);
                    if (j == 2u)
                        dd = fm_dj_mod_u8(dd, &g.dj, FM_DJ_TGT_DENS_H, 0, 127);
                }
#else
                dd = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)dd - (m_db * 3) / 4, 0, 127), DUB_LFO_DENS, 0, 127);
                dd = lfo_mod_u8(dd, (uint8_t)(DUB_LFO_DENS_K + j), 0, 127);
                if (j == 0u)
                    dd = fm_dj_mod_u8(dd, &g.dj, FM_DJ_TGT_DENS_K, 0, 127);
                if (j == 2u)
                    dd = fm_dj_mod_u8(dd, &g.dj, FM_DJ_TGT_DENS_H, 0, 127);
#endif
                vp->drm_dens[j] = dd;
            }
        }
        if (gesture_active() == GEST_BREAK) {
            /* Match DJ BREAK dens duck while black-7 BREAK is live */
            vp->drm_dens[0] = fm_lfo_clamp_u8((int32_t)vp->drm_dens[0] - 120, 0, 127);
        }
    }
    vp->siren_pitch = lfo_mod_u8(vp->siren_pitch, DUB_LFO_SIREN,  0, 127);
    vp->siren_rate  = lfo_mod_u8(vp->siren_rate,  DUB_LFO_SRATE,  0, 127);
    for (i = 0; i < 4u; i++) {
        uint8_t base_dp = vp->drm_pan[i] ? vp->drm_pan[i] : def_dp[i];
        vp->drm_pan[i] = lfo_mod_u8(base_dp, (uint8_t)(DUB_LFO_PAN_K + i), 1, 127);
    }
    if (g.cue_on) {
        vp->bpm = g.cue_bpm;
        vp->swing = g.cue_swing;
        vp->root = g.cue_root;
        vp->scale = g.cue_scale;
    }
#if defined(GENRE_POP)
    {
        int32_t d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_OCT);

        g.synth.pop_lead_oct_lfo_semi = (int8_t)((d * 60) / 128);
    }
    vp->scale = lfo_mod_enum(vp->scale, DUB_LFO_SCALE, 9u);
#else
    vp->root        = dub_lfo_mod_root_oct(vp->root);
    vp->root        = (uint8_t)((vp->root / 12u) * 12u + lfo_mod_enum(vp->root % 12u, DUB_LFO_KEY, 12u));
    vp->scale       = (uint8_t)(SCALE_LO + lfo_mod_enum((uint8_t)(scale_ix(vp->scale) - SCALE_LO), DUB_LFO_SCALE, SCALE_N));
#endif
    {
        int32_t sw_d = (fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_SWING) * 25) / 128;
        int32_t bpm_d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_TEMPO) / 2;
        vp->swing = fm_lfo_clamp_u8((int32_t)(vp->swing ? vp->swing : 54u) + sw_d, 50, 75);
        vp->bpm   = fm_lfo_clamp_u8((int32_t)(vp->bpm ? vp->bpm : 120u) + bpm_d, 40, 180);
    }

    /* FX (Page 1..3) */
    if (!fm_dj_fx_echo_time_live(&g.dj, &g.dj_fx_ov))
        vp->dly_time = lfo_mod_u8(vp->dly_time, DUB_LFO_TIME, 0, 127);
    else if (vp->dly_time != 0u)
        vp->dly_time = fm_dj_time_quantize_cc(vp->dly_time, vp->bpm ? vp->bpm : 120u);
    vp->feedback    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->feedback + dub_cpl_share(m_fb, DUB_LFO_CPL_FB_MFEED) + dub_cpl_share(m_db, DUB_LFO_CPL_FB_MDUB) + dub_cpl_share(m_sp, DUB_LFO_CPL_FB_MSPACE), 0, 127), DUB_LFO_FB, 0, 127),
                                  &g.dj, FM_DJ_TGT_FB, 0, 127);
    vp->dly_lvl     = lfo_mod_u8(vp->dly_lvl,     DUB_LFO_EMIX,   0, 127);
    vp->loop_filt   = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->loop_filt - dub_cpl_share(m_db, DUB_LFO_CPL_LOOP_MDUB), 0, 127), DUB_LFO_LOOP, 0, 127);
#if defined(GENRE_POP)
    vp->wow         = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->wow + dub_cpl_share(m_fb, DUB_LFO_CPL_WOW_MFEED), 0, 127), DUB_LFO_WOW, 0, 127);
#else
    vp->wow         = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->wow + dub_cpl_share(m_fb, DUB_LFO_CPL_WOW_MFEED) + dub_cpl_share(m_gr, DUB_LFO_CPL_WOW_MGRIT), 0, 127), DUB_LFO_WOW, 0, 127);
#endif
    vp->tap         = lfo_mod_enum(vp->tap,       DUB_LFO_TAP,    TAP_N);
#if defined(GENRE_POP)
    vp->tank        = lfo_mod_u8(vp->tank, DUB_LFO_TANK, 0, 127);
    vp->isolator    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->isolator + (m_is >> 1), 0, 127), DUB_LFO_ISO, 0, 127),
                                  &g.dj, FM_DJ_TGT_ISO, 0, 127);
    vp->rev_mix     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->rev_mix + dub_cpl_share(m_fb, DUB_LFO_CPL_REV_MFEED), 0, 127), DUB_LFO_REV, 0, 127);
#else
    vp->tank        = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->tank + dub_cpl_share(m_sp, DUB_LFO_CPL_TANK_MSPACE), 0, 127), DUB_LFO_TANK, 0, 127);
    vp->isolator    = fm_dj_mod_u8(lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->isolator + (m_is >> 1), 0, 127), DUB_LFO_ISO, 0, 127),
                                  &g.dj, FM_DJ_TGT_ISO, 0, 127);
    vp->rev_mix     = lfo_mod_u8(fm_lfo_clamp_u8((int32_t)vp->rev_mix + dub_cpl_share(m_fb, DUB_LFO_CPL_REV_MFEED) + dub_cpl_share(m_sp, DUB_LFO_CPL_REV_MSPACE), 0, 127), DUB_LFO_REV, 0, 127);
#endif
    vp->rev_damp    = lfo_mod_u8(vp->rev_damp,    DUB_LFO_RDAMP,  0, 127);
    vp->rev_tail    = lfo_mod_u8(vp->rev_tail,    DUB_LFO_RTAIL,  0, 127);
    vp->dly_rev     = lfo_mod_u8(vp->dly_rev,     DUB_LFO_ESEND,  0, 127);
    /* Phrase climb + DJ FX scenes layer on morph+LFO+slot base (never write cfg). */
    fm_dj_apply_phrase_climax(vp, &g.dj, g.kick_slam_bars, g.drop_emph_span);
    fm_dj_fx_apply(vp, &g.dj, &g.dj_fx_ov, g.bar_step);
#if !defined(GENRE_BASS)
    if (eff_motion > 0u) {
        uint32_t bpm_ms = vp->bpm ? (uint32_t)vp->bpm : 120u;
        uint32_t bar_ms = (60000u * 4u) / bpm_ms;
        int32_t m_slot = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_MOTION);
        int32_t m_slow = dub_ms_tri(fm1_ms, bar_ms > 800u ? bar_ms : 800u);
        int32_t m_fast = dub_ms_tri(fm1_ms + 900u, (bar_ms >> 1) > 400u ? (bar_ms >> 1) : 400u);
        int32_t md = (m_slow * (int32_t)eff_motion) / 64 + (m_fast * (int32_t)eff_motion) / 96 +
                     (m_slot * (int32_t)eff_motion) / 128;

#if !defined(GENRE_POP)
        vp->glide_c = fm_lfo_clamp_u8((int32_t)vp->glide_c + dub_cpl_share(dub_s_mot_glide(eff_motion, md), DUB_LFO_CPL_MOT_GLIDE), 0, 127);
#endif
        vp->pan[0] = fm_lfo_clamp_u8((int32_t)(vp->pan[0] ? vp->pan[0] : 64u) + dub_cpl_share(md, DUB_LFO_CPL_MOT_PANB), 1, 127);
        vp->pan[1] = fm_lfo_clamp_u8((int32_t)(vp->pan[1] ? vp->pan[1] : 64u) - dub_cpl_share(md, DUB_LFO_CPL_MOT_PANC), 1, 127);
        for (i = 0; i < 4u; i++) {
            int32_t pn = (int32_t)(vp->drm_pan[i] ? vp->drm_pan[i] : def_dp[i]) + md / 5;
            vp->drm_pan[i] = fm_lfo_clamp_u8(pn, 1, 127);
        }
    }
    if (eff_flux > 0u) {
        int32_t w_slow = dub_ms_tri(fm1_ms, 5400u);
        int32_t w_fast = dub_ms_tri(fm1_ms + 1300u, 3200u);
        int32_t slot_d = fm_lfo_slot_delta(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_M_FLUX);
        int32_t fd = (w_slow * (int32_t)eff_flux) / 64 + (slot_d * (int32_t)eff_flux) / 128;
        int32_t ph = (w_fast * (int32_t)eff_flux) / 64 - (slot_d * (int32_t)eff_flux) / 192;

        vp->cutoff_c = fm_lfo_clamp_u8((int32_t)vp->cutoff_c + dub_cpl_share(dub_s_flx_cutc(fd, ph), DUB_LFO_CPL_FLX_CUTC), 0, 127);
        vp->cutoff_b = fm_lfo_clamp_u8((int32_t)vp->cutoff_b + dub_cpl_share(dub_s_flx_cutb(fd, ph), DUB_LFO_CPL_FLX_CUTB), 0, 127);
        vp->wobble = fm_lfo_clamp_u8((int32_t)vp->wobble + dub_cpl_share(dub_s_flx_wob(fd, ph), DUB_LFO_CPL_FLX_WOB), 0, 127);
        vp->feedback = fm_lfo_clamp_u8((int32_t)vp->feedback + dub_cpl_share(dub_s_fb_flux(fd, ph), DUB_LFO_CPL_FB_FLUX), 0, 127);
        vp->wow = fm_lfo_clamp_u8((int32_t)vp->wow + dub_cpl_share(dub_s_wow_flux(fd, ph), DUB_LFO_CPL_WOW_FLUX), 0, 127);
        vp->loop_filt = fm_lfo_clamp_u8((int32_t)vp->loop_filt + dub_cpl_share(dub_s_loop_flux(fd, ph), DUB_LFO_CPL_LOOP_FLUX), 0, 127);
        vp->rev_mix = fm_lfo_clamp_u8((int32_t)vp->rev_mix + dub_cpl_share(dub_s_rev_flux(fd, ph), DUB_LFO_CPL_REV_FLUX), 0, 127);
        for (i = 0; i < 4u; i++) {
            int32_t pn = (int32_t)(vp->drm_pan[i] ? vp->drm_pan[i] : def_dp[i]) + dub_cpl_share(dub_s_flx_drmpan(fd, ph), DUB_LFO_CPL_FLX_DRMPAN);
            vp->drm_pan[i] = fm_lfo_clamp_u8(pn, 1, 127);
        }
        vp->lfo1_depth = fm_lfo_clamp_u8((int32_t)vp->lfo1_depth + ((int32_t)eff_flux * 3) / 2, 0, 127);
        vp->lfo2_depth = fm_lfo_clamp_u8((int32_t)vp->lfo2_depth + ((int32_t)eff_flux * 5) / 6, 0, 127);
    }
#endif /* !GENRE_BASS */
    if (g.playing && fm_dj_pre_drop_silence(&g.dj, 1u, g.bar_step, DUB_STEPS)) {
        g.voice.throw = 0u;
        g.voice.lane_throw_mask = 0u;
        g.voice.freeze = 0u;
        vp->feedback = 0u;
        vp->dly_rev = 0u;
        if (!fm_dj_build_tail_hold(&g.dj)) {
            vp->rev_mix = 0u;
            vp->rev_tail = fm_lfo_clamp_u8((int32_t)vp->rev_tail - 90, 0, 127);
            vp->tank = fm_lfo_clamp_u8((int32_t)vp->tank - 90, 0, 127);
        }
        /* WASH / ISO+TANK: keep rev/tank tail; dry hits gated in fire_seq_step */
    }
    {
        uint32_t ln;

        for (ln = 0; ln < DUB_SEQ_LANES; ln++)
            g.voice.lane_tune[ln] = 0;
    }
    {
        uint32_t eff_bpm = vp->bpm ? vp->bpm : 120u;

        if (eff_bpm < 40u)
            eff_bpm = 40u;
        if (eff_bpm > 180u)
            eff_bpm = 180u;
        g.samples_per_step = DUB_FS * 60u * DUB_SEQ_GRID_TICKS / (eff_bpm * 24u);
        if (g.samples_per_step < 32u)
            g.samples_per_step = 32u;
        g.lane_den = eff_bpm * 24u;
        dub_lane_spp_refresh();
#if defined(GENRE_BASS)
        bass_spp_set_frac(&g.bass_rt, DUB_FS * 60u * DUB_SEQ_GRID_TICKS, eff_bpm * 24u);
        if (g.ext_clock && g.bass_rt.ext_spp)
            g.samples_per_step = g.bass_rt.ext_spp;
#endif
    }
#ifdef GENRE_HEAVY_PSYCH
    g.synth.voice_era = vp->era;
#endif
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    idm_apply_fill_break_voice(vp);
#endif
#if defined(GENRE_BASS)
    bass_sync_voice_mod(vp);
    g.synth.bass_env_curve = lfo_mod_u8(bass_env_shape_curve(g.cfg.bass_env_shape), DUB_LFO_ENV_CURV, 0u, 127u);
#else
    gv_envx_sync();
#endif
}

static void mark_dirty(void)
{
    sync_voice_params();
    g.set_dirty = 1u;
    g.set_t = fm1_ms;
    g.ui_dirty = 1u;
}

static void transport_set_bpm(void)
{
    uint32_t bpm = g.cfg.p.bpm;
    if (bpm < 40u)
        bpm = 40u;
    if (bpm > 180u)
        bpm = 180u;
    g.samples_per_step = DUB_FS * 60u * DUB_SEQ_GRID_TICKS / (bpm * 24u);
    if (g.samples_per_step < 32u)
        g.samples_per_step = 32u;
    g.lane_den = bpm * 24u;
    dub_lane_spp_refresh();
#if defined(GENRE_BASS)
    bass_spp_set_frac(&g.bass_rt, DUB_FS * 60u * DUB_SEQ_GRID_TICKS, bpm * 24u);
    if (g.ext_clock && g.bass_rt.ext_spp)
        g.samples_per_step = g.bass_rt.ext_spp;
#endif
}

#if defined(GENRE_BASS)
static uint32_t swing_samples(void);
#endif

static void transport_reset(void)
{
    g.step = 0;
    {
        uint8_t ln;

        for (ln = 0u; ln < DUB_SEQ_LANES; ln++)
            dub_lane_resync(ln);
    }
    g.bar = 0u;
    g.bar_step = 0u;
    g.rail_step = 0u;
    g.slip_step = 0u;
    g.sample_acc = g.samples_per_step > 1u ? g.samples_per_step - 1u : 0u;
    seq_view_rep_arm((uint8_t)(g.step / FM_SEQ_PAGE_STEPS));
#if defined(GENRE_BASS)
    {
        /* step 0 fires on the next sample with no swing residue, else the whole grid shifts by sw */
        uint32_t spp = g.samples_per_step;
        uint32_t sw = swing_samples();
        uint32_t need0 = spp;

        if (sw > spp / 2u)
            sw = spp / 2u;
        if (spp > sw + 32u)
            need0 = spp - sw;
        g.sample_acc = need0 > 1u ? need0 - 1u : 0u;
    }
    g.bass_rt.spp_frac_acc = 0u;
    g.bass_rt.spp_extra = 0u;
    g.bass_rt.clk_pc = BASS_CLK_PAIR;
    g.bass_rt.clk_acc = 0u;
    g.bass_rt.clk_fa_pending = 0u;
    g.bass_rt.ext_pulse = 0u;
    g.bass_rt.ext_have_last = 0u;
    if (!g.playing)
        bass_transport_stop();
#endif
}

static void user_song_clear(void)
{
    g.cfg.user_song = 0u;
    g.cfg.song_name[0] = 0;
    seq_arr_derive_preset();
}

static void cfg_user_song_defaults(void)
{
    user_song_clear();
}

static const char DUB_SONG_NAME_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";

static void song_setup_name_default(void)
{
    uint32_t i;

    for (i = 0; i < DUB_SONG_NAME_LEN; i++)
        g.song_setup_name[i] = ' ';
    g.song_setup_name[0] = 'S';
    g.song_setup_name[1] = 'O';
    g.song_setup_name[2] = 'N';
    g.song_setup_name[3] = 'G';
    g.song_setup_name[DUB_SONG_NAME_LEN] = 0;
}

static void song_setup_snap_groove(void)
{
    g.song_setup_snap_bpm = g.cfg.p.bpm;
    g.song_setup_snap_root = g.cfg.p.root;
    g.song_setup_snap_scale = g.cfg.p.scale;
    g.song_setup_snap_swing = g.cfg.p.swing;
}

static void song_setup_restore_groove(void)
{
    g.cfg.p.bpm = g.song_setup_snap_bpm;
    g.cfg.p.root = g.song_setup_snap_root;
    g.cfg.p.scale = g.song_setup_snap_scale;
    g.cfg.p.swing = g.song_setup_snap_swing;
    dub_morph_sync_field(&g.cfg.p.root);
    dub_morph_sync_field(&g.cfg.p.scale);
    dub_morph_sync_field(&g.cfg.p.bpm);
    transport_set_bpm();
}

static void song_setup_restore_grid(void)
{
    uint32_t i, ln;

    for (i = 0; i < DUB_SEQ_STEPS; i++) {
        g.cfg.seq[i].gate = g.song_setup_snap_gate[i];
        g.cfg.seq[i].force = g.song_setup_snap_force[i];
    }
    for (ln = 0; ln < DUB_SEQ_LANES; ln++)
        g.euclid_hits[ln] = g.song_setup_snap_euclid[ln];
    if (!g.cfg.user_song)
        seq_arr_derive_preset(); /* grid is back: re-derive the preset arrangement from it */
}

static void seq_new_song_clear_remnants(void)
{
    uint32_t i, ln;

    user_song_clear();
    cfg_seq_generators_clear();
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        g.pattern_origin[ln] = FM_SEQ_PATTERN_ORIGIN_NONE;
        g.euclid_hits[ln] = FM_SEQ_EUCLID_EMPTY;
        seq_lane_wipe_plocks((uint8_t)ln);
    }
    for (i = 0; i < DUB_SEQ_STEPS; i++) {
        g.cfg.seq[i].gate = 0u;
        g.cfg.seq[i].force = 1u;
        g.cfg.seq[i].note = 0u;
        g.cfg.seq[i].dly_throw = 0u;
        g.cfg.seq[i].rev_throw = 0u;
        g.cfg.seq[i].accent = 0u;
    }
    /* new empty song: every page starts ON (ALL), REPEAT 1; the user turns pages off from here */
    for (i = 0; i < FM_SEQ_MAX_PAGES; i++) {
        g.seq_view_cyc[i] = FM_SEQ_VIEW_CYC_ALL;
        g.seq_view_reps[i] = 1u;
        seq_view_cyc_sync_on((uint8_t)i);
    }
    g.seq_view_only_page = FM_SEQ_MAX_PAGES;
}

static void song_setup_empty_grid(void)
{
    uint32_t i, ln;

    for (i = 0; i < DUB_SEQ_STEPS; i++) {
        g.song_setup_snap_gate[i] = g.cfg.seq[i].gate;
        g.song_setup_snap_force[i] = g.cfg.seq[i].force;
    }
    for (ln = 0; ln < DUB_SEQ_LANES; ln++)
        g.song_setup_snap_euclid[ln] = g.euclid_hits[ln];
    seq_new_song_clear_remnants();
}

static void song_setup_discard(void)
{
    if (!g.song_setup_latch)
        return;
    song_setup_restore_groove();
    song_setup_restore_grid();
    g.song_setup_latch = 0u;
    g.seq_page = DUB_SEQ_PAGE_ENTRY;
    g.song_setup_name_cur = 0u;
    g.set_dirty = 0u;
    g.ui_dirty = 1u;
}

static void song_setup_open(void)
{
    uint32_t i;

    song_setup_snap_groove();
    song_setup_empty_grid();
    if (g.cfg.user_song && g.cfg.song_name[0]) {
        for (i = 0; i < DUB_SONG_NAME_LEN; i++)
            g.song_setup_name[i] = g.cfg.song_name[i] ? g.cfg.song_name[i] : ' ';
    } else
        song_setup_name_default();
    g.song_setup_name[DUB_SONG_NAME_LEN] = 0;
    g.song_setup_name_cur = 0u;
    g.song_setup_latch = 1u;
    g.seq_page = 5u;
    if (g.view != VIEW_SEQ)
        nav_view(VIEW_SEQ);
    g.ui_dirty = 1u;
}

static void song_setup_name_cycle(int32_t dir)
{
    uint32_t i, n = (uint32_t)(sizeof DUB_SONG_NAME_CHARS - 1u);
    char c = g.song_setup_name[g.song_setup_name_cur % DUB_SONG_NAME_LEN];
    uint32_t ix = 0;

    for (i = 0; i < n; i++) {
        if (DUB_SONG_NAME_CHARS[i] == c) {
            ix = i;
            break;
        }
    }
    if (dir > 0) {
        ix++;
        if (ix >= n)
            ix = 0;
    } else if (dir < 0) {
        if (ix == 0)
            ix = n - 1u;
        else
            ix--;
    }
    g.song_setup_name[g.song_setup_name_cur % DUB_SONG_NAME_LEN] = DUB_SONG_NAME_CHARS[ix];
}

static void cfg_seq_generators_clear(void)
{
    uint32_t ln, st;

    memset(g.cfg.euclid_hits, 0, sizeof g.cfg.euclid_hits);
    memset(g.cfg.euclid_rot, 0, sizeof g.cfg.euclid_rot);
    memset(g.cfg.vary_lane, 0, sizeof g.cfg.vary_lane);
    memset(g.cfg.lane_vel, 0, sizeof g.cfg.lane_vel);
    memset(g.cfg.lane_trig, 0, sizeof g.cfg.lane_trig);
    memset(g.cfg.lane_gate, 0, sizeof g.cfg.lane_gate);
    memset(g.cfg.lane_tune, 0, sizeof g.cfg.lane_tune);
#if defined(GENRE_BASS)
    (void)ln;
    (void)st;
    memset(g.cfg.step_vel, 0, sizeof g.cfg.step_vel);
    memset(g.cfg.step_prob, 0, sizeof g.cfg.step_prob);
    memset(g.cfg.step_micro, 0, sizeof g.cfg.step_micro);
    memset(g.cfg.step_tune, 0, sizeof g.cfg.step_tune);
#else
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        for (st = 0; st < DUB_SEQ_STEPS; st++) {
            g.cfg.step_vel[ln][st] = 0u;
            g.cfg.step_prob[ln][st] = 0u;
            g.cfg.step_micro[ln][st] = 0;
            g.cfg.step_tune[ln][st] = 0;
        }
    }
#endif
}

static void user_song_commit(void)
{
    uint32_t i;

    for (i = 0; i < DUB_SONG_NAME_LEN; i++)
        g.cfg.song_name[i] = g.song_setup_name[i];
    g.cfg.song_name[DUB_SONG_NAME_LEN] = 0;
    cfg_seq_generators_clear();
    seq_runtime_data_defaults();
#if defined(GENRE_BASS)
    g.cfg.seq_pages = 8u;
#else
    g.cfg.seq_pages = 2u;
#endif
    g.cfg.p.arp_mode = 0u;
    dub_morph_sync_field(&g.cfg.p.arp_mode);
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
        seq_view_cyc_sync_on((uint8_t)i); /* the song's arrangement decides which pages play */
    g.voice.lane_mute_mask = 0u;
    g.voice.heard_mute_mask = 0u;
    g.voice.lane_solo_mask = 0u;
    g.voice.mute = 0u;
    transport_reset();
    g.cfg.user_song = 1u;
    seq_arr_song_mirror(); /* runtime arrangement -> song storage */
    g.song_setup_latch = 0u;
    g.seq_page = DUB_SEQ_PAGE_ENTRY;
    mark_dirty();
    g.ui_dirty = 1u;
}

static void seq_runtime_data_defaults(void)
{
    uint32_t ln, st;

    g.bar = 0u;
    g.seq_rng = 0x12345678u;
#if defined(GENRE_BASS)
    g.vary_seed = g.seq_rng;
#endif
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        g.euclid_hits[ln] = 0u;
        g.pattern_origin[ln] = FM_SEQ_PATTERN_ORIGIN_NONE;
        g.euclid_rot[ln] = 0;
        g.vary_lane[ln] = 0u;
        g.lane_vel[ln] = 0;
        g.lane_trig[ln] = 0;
        g.lane_gate[ln] = 0;
        g.lane_tune[ln] = 0;
        g.bk_last_ms[ln] = 0u;
        for (st = 0; st < DUB_SEQ_STEPS; st++) {
            g.step_vel[ln][st] = 0u;
            g.step_prob[ln][st] = 0u;
            g.step_micro[ln][st] = 0;
            g.step_tune[ln][st] = 0;
        }
    }
}

static void seq_runtime_defaults(void)
{
    g.seq_page = dub_seq_page_norm(g.seq_page);
    g.seq_view_page = 0u;
    g.seq_plock_hold = 0u;
    g.seq_plock_latch = 0u;
    g.seq_plock_bank = 0u;
    g.seq_plock_step = FM_SEQ_PLOCK_NONE;
    g.seq_plock_knob = 0u;
    g.white_press_k = 255u;
    g.white_press_ms = 0u;
    g.white_long_used = 0u;
    g.white_long_cancel = 0u;
    seq_runtime_data_defaults();
}

/* Same values as the runtime init in app_init: S1 OFF, S2..S8 ALL, reps 1. */
static void seq_cfg_defaults(dub_cfg_t *c)
{
    uint32_t i;

    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
        c->seq_cyc[i] = i == 0u ? FM_SEQ_VIEW_CYC_OFF : FM_SEQ_VIEW_CYC_ALL;
        c->seq_reps[i] = 1u;
    }
    for (i = 0u; i < DUB_SEQ_LANES; i++)
        c->seq_pace[i] = DUB_SEQ_PACE_GRID;
}

static void cfg_defaults(void)
{
    uint32_t i;
    const uint8_t boot_plate = 0u;
    g.cfg.magic = DUB_CFG_MAGIC;
    seq_cfg_defaults(&g.cfg);
    cfg_user_song_defaults();
    genre_preset_rebuild(&g.cfg.p, genre_era_of_preset(boot_plate), boot_plate);
    g.cfg.p.bass_model = dub_bass_model_for_era(g.cfg.p.era);
    g.cfg.p.drum_kit = dub_drum_kit_for_era(g.cfg.p.era);
    dub_lfo_fm_defaults(&g.cfg.p);
    for (i = 0; i < DUB_SEQ_STEPS; i++)
        g.cfg.seq[i].force = 0;
#if defined(GENRE_BASS)
    g.cfg.seq_pages = 8u;
#else
    g.cfg.seq_pages = 2u;
#endif
    g.voice.mute = 0;
    g.voice.throw = 0;
    g.voice.freeze = 0;
    g.voice.tape_oct = 0;
    g.voice.wob_force = 0;
    {
        uint32_t li;
        g.voice.lane_mute_mask = 0u;
        g.voice.heard_mute_mask = 0u;
        g.voice.lane_solo_mask = 0u;
        for (li = 0; li < DUB_SEQ_LANES; li++)
            g.voice.saved_lane_lvl[li] = DUB_SAVED_LVL_DEF[li];
    }
    g.morph_pos = DUB_MORPH_STEPS;
    g.morph_acc = 0u;
    g.cue_on = 0u;
    g.groove_plate_pending = 0u;
    g.cue_acc = 0u;
    g.cue_ticks = 0u;
    g.morph_from = g.cfg.p;
    fm_seq_bits_fill(g.morph_from_seq, 0);
    g.macro_morph_pos = DUB_MORPH_STEPS;
    g.macro_morph_acc = 0u;
    g.seq_ens_sel = FM_SEQ_ENS_ALL;
    g.lfo_last_ens = FM_SEQ_ENS_ALL;
    fm_dj_seq_ens_ov_reset(&g.dj_ens_ov);
    g.dj_ens_invert_overlay = 0u;
    g.home_morph_flash_ms = 0u;
    g.home_morph_flash_kind = 0u;
    dub_plate_seq_preset(g.cfg.p.plate);
    dub_era_lfo_apply(g.cfg.p.era, g.cfg.p.plate);
    dub_era_home_macros_apply_era(g.cfg.p.era, g.cfg.p.plate);
    dub_sel_sync_saved_lvls();
    g.lfo_target = dub_lfo_tgt_default();
    memcpy(g.cfg.lfo_slots, g.lfo_slots, sizeof g.cfg.lfo_slots);
    dub_cpl_store();
#if !defined(GENRE_BASS)
    gv_envx_lfo_pack();
#endif
    g.cfg.lfo_target = g.lfo_target;
#if defined(GENRE_BASS)
    bass_cfg_ext_defaults(&g.cfg.bass);
    g.cfg.bass_env_shape = bass_env_shape_pack(BASS_DAHDSR_CURVE_LIN);
    bass_genre_fader_bake(g.cfg.seq, g.cfg.bass.step_x, g.cfg.bass.genre_fader, g.cfg.p.era,
                          (uint8_t)fm_seq_len(g.cfg.seq_pages));
    g.voice.mute = 0xEu;
#endif
}

static void cfg_boot_random_song(void)
{
    dub_params_t keep = g.cfg.p;
    uint8_t plate;
    uint8_t era;

    g.seq_rng = dub_boot_seed();
#if defined(GENRE_BASS)
    g.vary_seed = g.seq_rng;
#endif
    plate = 0u;
    era = genre_era_of_preset(0u);
    for (uint8_t attempt = 0u; attempt < 32u; attempt++) {
        dub_rng_step();
        plate = genre_clamp_preset((uint8_t)(((g.seq_rng >> 16) & 0xffffu) % GENRE_PRESET_N));
        era = genre_era_of_preset(plate);
        if (plate < GENRE_PRESET_N)
            break;
    }
    if (plate >= GENRE_PRESET_N) {
        plate = 0u;
        era = genre_era_of_preset(0u);
    }
#if defined(FM_PGEN_ON)
    dub_rng_step();
    plate = fm_pgen_goto(era, (int32_t)((g.seq_rng >> 8) & 0xFFFFu) - 0x8000);
#endif

    if (g.user_plate_mask & (1ull << plate))
        genre_plate_preset_apply(&g.cfg.p, era, plate, keep.bass_model, keep.drum_kit, 1u);
    else {
        genre_preset_rebuild(&g.cfg.p, era, plate);
        g.cfg.p.bass_model = dub_bass_model_for_era(era);
        g.cfg.p.drum_kit = dub_drum_kit_for_era(era);
    }

    {
        uint8_t gr_root, gr_scale, gr_swing, gr_bpm;

        dub_plate_groove_read(plate, &gr_root, &gr_scale, &gr_swing, &gr_bpm);
        g.cfg.p.root = gr_root;
        g.cfg.p.scale = gr_scale;
        g.cfg.p.swing = gr_swing;
        g.cfg.p.bpm = gr_bpm;
    }
    g.cfg.p.dly_time = keep.dly_time;
    g.cfg.p.feedback = keep.feedback;
    g.cfg.p.rev_mix = keep.rev_mix;
    g.cfg.p.loop_filt = keep.loop_filt;
    g.cfg.p.wow = keep.wow;
    g.cfg.p.tap = keep.tap;
    g.cfg.p.tank = keep.tank;
    g.cfg.p.isolator = keep.isolator;
    g.cfg.p.dly_lvl = keep.dly_lvl;
    g.cfg.p.rev_damp = keep.rev_damp;
    g.cfg.p.rev_tail = keep.rev_tail;
    g.cfg.p.dly_rev = keep.dly_rev;

    dub_plate_seq_preset(plate);
    dub_era_lfo_apply(era, plate);
    dub_era_home_macros_apply_era(era, plate);
    dub_sel_sync_saved_lvls();
#if defined(GENRE_BASS)
    bass_fader_to_plate();
#endif
}

static int synth_is_quiet(void)
{
    return g.synth.meter[0] < 4u && g.synth.meter[1] < 4u && g.synth.meter[2] < 4u && g.synth.meter[3] < 4u;
}

#if defined(GENRE_BASS)
/* INT+TX: 24 ppqn, straight (unswung). Pulses are spaced evenly over a swing pair (two steps =
 * 2 * samples_per_step); the pair restarts on every even fired step so the count never drifts. */
static void bass_clock_tx_tick(uint8_t fired)
{
    bass_app_rt_t *rt = &g.bass_rt;

    if (!bass_clock_sends(g.cfg.bass.clock_mode) || (fired & 1u))
        return;
    while (rt->clk_pc < BASS_CLK_PAIR) {
        midi_send(0xF8u, 0, 0);
        rt->clk_pc++;
    }
    if (rt->clk_fa_pending) {
        rt->clk_fa_pending = 0u;
        midi_send(0xFAu, 0, 0);
    }
    rt->clk_pc = 0u;
    rt->clk_acc = 0u;
}

static void bass_clock_tx_sample(void)
{
    bass_app_rt_t *rt = &g.bass_rt;

    if (!bass_clock_sends(g.cfg.bass.clock_mode))
        return;
    while (rt->clk_pc < BASS_CLK_PAIR && rt->clk_acc * BASS_CLK_PPS >= (uint32_t)rt->clk_pc * g.samples_per_step) {
        midi_send(0xF8u, 0, 0);
        rt->clk_pc++;
    }
    rt->clk_acc++;
}
#else
static void midi_clock_tx(void)
{
    midi_send(0xF8u, 0, 0);
}
#endif

static void midi_start_tx(void)
{
#if defined(GENRE_BASS)
    if (!bass_clock_sends(g.cfg.bass.clock_mode))
        return;
#endif
    midi_send(0xFAu, 0, 0);
}

static void midi_stop_tx(void)
{
#if defined(GENRE_BASS)
    if (!bass_clock_sends(g.cfg.bass.clock_mode))
        return;
#endif
    midi_send(0xFCu, 0, 0);
}

#if defined(GENRE_BASS)
static void dub_transport_morph_snap(void);

/* Leaving INT+TX while playing sends FC; entering it sends FA with the next pair start (even
 * step), so the slave's beat 1 lines up with pulse 0. Entering EXT stops the internal transport. */
static void bass_clock_mode_set(uint8_t m)
{
    uint8_t old_m = g.cfg.bass.clock_mode;

    if (m >= BASS_CLOCK_N)
        m = BASS_CLOCK_SEND;
    g.cfg.bass.clock_mode = m;
    g.ext_clock = bass_clock_follows_ext(m);
    if (m == old_m)
        return;
    g.bass_rt.ext_pulse = 0u;
    g.bass_rt.ext_have_last = 0u;
    g.bass_rt.clk_fa_pending = 0u;
    if (!g.playing)
        return;
    if (bass_clock_sends(old_m))
        midi_send(0xFCu, 0, 0);
    if (m == BASS_CLOCK_EXT) {
        g.playing = 0u;
        dub_dj_gest_restore();
        dub_transport_morph_snap();
        bass_transport_stop();
        g.ui_dirty = 1u;
        return;
    }
    if (bass_clock_sends(m)) {
        g.bass_rt.clk_pc = BASS_CLK_PAIR;
        g.bass_rt.clk_fa_pending = 1u;
    }
}
#endif

static uint8_t white_Play_active(void)
{
    int edit_held = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    if (g.view == VIEW_SEQ || edit_held || g.console)
        return 0u;
    return 1u;
}

static uint8_t bass_keys_held(void)
{
    uint32_t k;
    if (!white_Play_active())
        return 0u;
    for (k = 0u; k < 8u; k++) {
        if (fm1_in.notes & (1u << KEY_WHITE[k]))
            return 1u;
    }
    return 0u;
}

static uint8_t bass_note_from_keys(uint8_t step)
{
    uint8_t held[8];
    uint8_t n = 0u;
    uint32_t k;
    for (k = 0u; k < 8u; k++) {
        if (fm1_in.notes & (1u << KEY_WHITE[k]))
            held[n++] = scale_deg_note(g.voice.p.root, g.voice.p.scale, (uint8_t)k);
    }
    if (n == 0u)
        return g.voice.p.root;
    return held[(step >> 1) % n];
}

static uint8_t chord_keys_held(void)
{
    uint32_t k;
    if (!white_Play_active())
        return 0u;
    for (k = 8u; k < 16u; k++) {
        if (fm1_in.notes & (1u << KEY_WHITE[k]))
            return 1u;
    }
    return 0u;
}

static uint8_t chord_root_from_keys(uint8_t step)
{
    uint8_t held[8];
    uint8_t n = 0u;
    uint32_t k;
    for (k = 8u; k < 16u; k++) {
        if (fm1_in.notes & (1u << KEY_WHITE[k]))
            held[n++] = scale_deg_note(g.voice.p.root, g.voice.p.scale, (uint8_t)(k - 8u));
    }
    if (n == 0u)
        return g.voice.p.root;
    return held[(step >> 1) % n];
}

static int arp_chord_step(uint8_t mode, uint8_t step)
{
    if (mode == 1u)
        return step == 2u || step == 6u || step == 10u || step == 14u;
    if (mode == 2u)
        return arp_chord_step(1u, step) || step == 1u || step == 5u || step == 9u || step == 13u;
    if (mode == 3u)
        return step == 0u || step == 3u || step == 6u || step == 9u || step == 12u || step == 15u;
    return 0;
}

/* Pure ×2 ladder 1/32 → 4/1. Keys 0..7 ascending; 8..15 mirror descending. */
static const uint8_t DUB_LOOP_LEN_WHITE_KEY[8] = {
    1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u
};

static uint8_t loop_len_from_white_key(uint8_t k)
{
    if (k >= 16u)
        k = 15u;
    if (k < 8u)
        return DUB_LOOP_LEN_WHITE_KEY[k];
    return DUB_LOOP_LEN_WHITE_KEY[15u - k];
}

static uint8_t dub_active_arp_mode(void)
{
    return g.cfg.p.arp_mode;
}

static void seq_materialize_all(void);

static void dub_loop_snap_capture(void)
{
    uint8_t st;
    uint8_t ln;

    seq_materialize_all();
    for (st = 0; st < DUB_SEQ_STEPS; st++)
        g.loop_snap_step[st] = g.cfg.seq[st];
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        for (st = 0; st < DUB_SEQ_STEPS; st++) {
            g.loop_snap_vel[ln][st] = g.step_vel[ln][st];
            g.loop_snap_prob[ln][st] = g.step_prob[ln][st];
            g.loop_snap_micro[ln][st] = g.step_micro[ln][st];
            g.loop_snap_tune[ln][st] = g.step_tune[ln][st];
        }
    }
    g.loop_snap_valid = 1u;
}

static void dub_loop_snap_sync_step(uint8_t st)
{
    uint8_t ln;

    if (!g.loop_snap_valid || !g.loop_active)
        return;
    st %= DUB_SEQ_STEPS;
    g.loop_snap_step[st] = g.cfg.seq[st];
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        g.loop_snap_vel[ln][st] = g.step_vel[ln][st];
        g.loop_snap_prob[ln][st] = g.step_prob[ln][st];
        g.loop_snap_micro[ln][st] = g.step_micro[ln][st];
        g.loop_snap_tune[ln][st] = g.step_tune[ln][st];
    }
}

static void dub_loop_snap_manage(void)
{
    static uint8_t prev_active;
    static uint8_t prev_start;
    static uint8_t prev_len;

    if (g.loop_active) {
        if (!prev_active || g.loop_start_step != prev_start || g.loop_len != prev_len)
            dub_loop_snap_capture();
    } else
        g.loop_snap_valid = 0u;
    prev_active = g.loop_active;
    prev_start = g.loop_start_step;
    prev_len = g.loop_len;
}

static void dub_loop_rec_burn(uint8_t src, uint8_t dst)
{
    uint8_t ln;
    const dub_step_t *from;

    src %= DUB_SEQ_STEPS;
    dst %= DUB_SEQ_STEPS;
    if (src == dst || !g.loop_snap_valid)
        return;
    from = &g.loop_snap_step[src];
    seq_materialize_all();
    g.cfg.seq[dst] = *from;
    g.cfg.seq[dst].force = 1u;
    g.morph_from_step[dst] = *from;
    fm_seq_bit_put(g.morph_from_seq, dst, 1);
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        g.step_vel[ln][dst] = g.loop_snap_vel[ln][src];
        g.step_prob[ln][dst] = g.loop_snap_prob[ln][src];
        g.step_micro[ln][dst] = g.loop_snap_micro[ln][src];
        g.step_tune[ln][dst] = g.loop_snap_tune[ln][src];
        if ((from->gate & (uint8_t)(1u << ln)) || g.loop_snap_vel[ln][src] ||
            g.loop_snap_prob[ln][src] || g.loop_snap_micro[ln][src] || g.loop_snap_tune[ln][src])
            g.euclid_hits[ln] = FM_SEQ_EUCLID_SONG;
    }
    mark_dirty();
}

static uint8_t seq_clip_lane_key_held(uint32_t keys, uint8_t *ln_out)
{
    uint32_t k;

    for (k = 0; k < 6u; k++) {
        if (keys & (1u << KEY_BLACK[k])) {
            *ln_out = (uint8_t)k;
            return 1u;
        }
    }
    return 0u;
}

#define SEQ_CLIP_TOAST_MS 1000u

static const char *seq_clip_lane_lbl(uint8_t ln)
{
#if defined(GENRE_HEAVY_PSYCH)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "N", "H", "L", "B", "C"};
#elif defined(GENRE_HOUSE)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "N", "H", "S", "B", "C"};
#elif defined(GENRE_HIPHOP)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "N", "H", "X", "B", "C"};
#elif defined(GENRE_JAZZ)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "D", "H", "B", "C"};
#elif defined(GENRE_SOUL)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "S", "T", "H", "B", "C"};
#elif defined(GENRE_IDM)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "D", "T", "B", "C"};
#elif defined(GENRE_SNOWCRASH)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "D", "P", "B", "C"};
#elif defined(GENRE_POP)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "H", "L", "B", "C"};
#else
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "H", "S", "B", "C"};
#endif

    if (ln >= DUB_SEQ_LANES)
        return "ALL";
    return LBL[ln];
}

static void seq_clip_toast_set(const char *msg)
{
    uint32_t i = 0u;

    if (!msg)
        return;
    while (msg[i] && i + 1u < sizeof g.seq_clip_toast) {
        g.seq_clip_toast[i] = msg[i];
        i++;
    }
    g.seq_clip_toast[i] = 0;
    g.seq_clip_toast_until = fm1_ms + SEQ_CLIP_TOAST_MS;
    g.ui_dirty = 1u;
}

static void seq_clip_toast_after_copy(void)
{
    char s[28];
    char vn[4];
    char *p;
    uint8_t vp = (uint8_t)((g.seq_view_page % FM_SEQ_MAX_PAGES) + 1u);

    u8_dec(vn, (uint32_t)vp);
    p = s;
    if (!seq_lane_is_all()) {
        copy_str(p, "COPY LANE ");
        while (*p)
            p++;
        copy_str(p, seq_clip_lane_lbl(g.seq_clip_lane));
        while (*p)
            p++;
        copy_str(p, " VIEW ");
        while (*p)
            p++;
        copy_str(p, vn);
    } else {
        copy_str(p, "COPY VIEW ");
        while (*p)
            p++;
        copy_str(p, vn);
    }
    seq_clip_toast_set(s);
}

static void seq_clip_toast_after_paste(void)
{
    char s[28];
    char vn[4];
    char *p;
    uint8_t vp = (uint8_t)((g.seq_view_page % FM_SEQ_MAX_PAGES) + 1u);

    u8_dec(vn, (uint32_t)vp);
    p = s;
    if (g.seq_clip_kind == DUB_SEQ_CLIP_VIEW) {
        copy_str(p, "PASTE VIEW ");
        while (*p)
            p++;
        copy_str(p, vn);
    } else {
        copy_str(p, "PASTE LANE ");
        while (*p)
            p++;
        copy_str(p, seq_clip_lane_lbl(seq_lane_sel()));
    }
    seq_clip_toast_set(s);
}

static void seq_clip_copy(uint32_t keys)
{
    uint8_t st;
    uint8_t ln;
    uint8_t src_ln;
    uint32_t base = seq_view_base();

    seq_materialize_all();
    if (!seq_lane_is_all()) {
        src_ln = seq_lane_sel();
        uint8_t src_bit = (uint8_t)(1u << (src_ln % DUB_SEQ_LANES));

        g.seq_clip_kind = DUB_SEQ_CLIP_LANE;
        g.seq_clip_lane = src_ln % DUB_SEQ_LANES;
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            uint8_t ab = (uint8_t)(base + st);
            dub_step_t *cs = &g.seq_clip_step[st];

            cs->gate = (g.cfg.seq[ab].gate & src_bit) ? src_bit : 0u;
            cs->force = 0u;
            cs->note = 0u;
            cs->dly_throw = 0u;
            cs->rev_throw = 0u;
            cs->accent = 0u;
            g.seq_clip_vel[g.seq_clip_lane][st] = g.step_vel[g.seq_clip_lane][ab];
            g.seq_clip_prob[g.seq_clip_lane][st] = g.step_prob[g.seq_clip_lane][ab];
            g.seq_clip_micro[g.seq_clip_lane][st] = g.step_micro[g.seq_clip_lane][ab];
            g.seq_clip_tune[g.seq_clip_lane][st] = g.step_tune[g.seq_clip_lane][ab];
        }
    } else {
        g.seq_clip_kind = DUB_SEQ_CLIP_VIEW;
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            uint8_t ab = (uint8_t)(base + st);

            g.seq_clip_step[st] = g.cfg.seq[ab];
            for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
                g.seq_clip_vel[ln][st] = g.step_vel[ln][ab];
                g.seq_clip_prob[ln][st] = g.step_prob[ln][ab];
                g.seq_clip_micro[ln][st] = g.step_micro[ln][ab];
                g.seq_clip_tune[ln][st] = g.step_tune[ln][ab];
            }
        }
    }
    seq_clip_toast_after_copy();
    g.ui_dirty = 1u;
}

static void seq_clip_paste(void)
{
    uint8_t st;
    uint8_t ln;
    uint32_t base;

    if (g.seq_clip_kind == DUB_SEQ_CLIP_NONE)
        return;
    base = seq_view_base();
    seq_materialize_all();
    if (g.seq_clip_kind == DUB_SEQ_CLIP_VIEW) {
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            uint8_t ab = (uint8_t)(base + st);
            const dub_step_t *cs = &g.seq_clip_step[st];

            g.cfg.seq[ab] = *cs;
            g.cfg.seq[ab].force = 1u;
            for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
                g.step_vel[ln][ab] = g.seq_clip_vel[ln][st];
                g.step_prob[ln][ab] = g.seq_clip_prob[ln][st];
                g.step_micro[ln][ab] = g.seq_clip_micro[ln][st];
                g.step_tune[ln][ab] = g.seq_clip_tune[ln][st];
                if ((cs->gate & (uint8_t)(1u << ln)) || g.seq_clip_vel[ln][st] || g.seq_clip_prob[ln][st] ||
                    g.seq_clip_micro[ln][st] || g.seq_clip_tune[ln][st])
                    g.euclid_hits[ln] = FM_SEQ_EUCLID_SONG;
            }
        }
    } else {
        uint8_t dst_ln;
        uint8_t src_ln;

        if (seq_lane_is_all())
            return;
        dst_ln = seq_lane_sel();
        if (dst_ln >= DUB_SEQ_LANES)
            return;
        src_ln = g.seq_clip_lane % DUB_SEQ_LANES;
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            uint8_t ab = (uint8_t)(base + st);
            uint8_t dst_bit = (uint8_t)(1u << dst_ln);
            uint8_t src_bit = (uint8_t)(1u << src_ln);
            uint8_t src_on = (g.seq_clip_step[st].gate & src_bit) != 0u;
            dub_step_t *s = &g.cfg.seq[ab];

            s->gate = (uint8_t)((s->gate & (uint8_t)~dst_bit) | (src_on ? dst_bit : 0u));
            s->force = 1u;
            g.step_vel[dst_ln][ab] = g.seq_clip_vel[src_ln][st];
            g.step_prob[dst_ln][ab] = g.seq_clip_prob[src_ln][st];
            g.step_micro[dst_ln][ab] = g.seq_clip_micro[src_ln][st];
            g.step_tune[dst_ln][ab] = g.seq_clip_tune[src_ln][st];
            if (src_on || g.seq_clip_vel[src_ln][st] || g.seq_clip_prob[src_ln][st] ||
                g.seq_clip_micro[src_ln][st] || g.seq_clip_tune[src_ln][st])
                g.euclid_hits[dst_ln] = FM_SEQ_EUCLID_SONG;
        }
    }
    mark_dirty();
    g.ui_dirty = 1u;
}

static void arp_loop_mode_update(void)
{
    uint32_t k;
    uint8_t white_cnt = 0u;
    uint8_t s_min = 15u;
    uint8_t s_max = 0u;

    if (dub_active_arp_mode() != 5u)
        return;
    if (!white_Play_active())
        return;
    for (k = 0u; k < 16u; k++) {
        if (fm1_in.notes & (1u << KEY_WHITE[k])) {
            white_cnt++;
            if (k < s_min)
                s_min = (uint8_t)k;
            if (k > s_max)
                s_max = (uint8_t)k;
        }
    }
    if (white_cnt == 0u) {
        /* Release path also clears; keep pending from going stale if release missed */
        g.loop_pending = 0u;
        return;
    }
    /* New hold cancels a queued stop — latest intent wins, no backlog */
    g.loop_end_pending = 0u;
    g.loop_pending = 1u;
    if (white_cnt >= 2u) {
        g.loop_pending_start = s_min;
        g.loop_pending_len = (uint8_t)(s_max - s_min + 1u);
    } else {
        g.loop_pending_start = s_min;
        g.loop_pending_len = loop_len_from_white_key(s_min);
    }
}

#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static void idm_loop_macro_sync(void)
{
    uint8_t macro;
    uint8_t k;

    if (dub_active_arp_mode() == 5u && white_Play_active())
        return;
    macro = dub_eff_macro(1u);
    if (macro < 16u) {
        g.loop_pending = 0u;
        if (g.loop_active)
            g.loop_end_pending = 1u;
        return;
    }
    /* Higher CROP → shorter window: 16→128 (4/1) … 127→1 (1/32) */
    k = (uint8_t)(7u - (((uint16_t)(macro - 16u) * 7u) / 111u));
    g.loop_end_pending = 0u;
    g.loop_pending = 1u;
    g.loop_pending_len = loop_len_from_white_key(k);
    /* First engage only — even 16th start (0,2,4…14); wander owns start after */
    if (!g.loop_active) {
        g.loop_pending_start = (uint8_t)(g.slip_step & 14u);
        g.loop_wander_acc = 0u;
    }
}
#endif

static void dub_loop_pending_apply(void)
{
    /* Stop first when no start is still armed — release must win over stale press */
    if (g.loop_end_pending && !g.loop_pending) {
        g.loop_end_pending = 0u;
        g.loop_active = 0u;
        g.loop_wander_acc = 0u;
        g.step = g.slip_step;
        return;
    }
    if (g.loop_pending) {
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        /* Already wandering: refresh length only, keep absolute start */
        if (g.loop_active) {
            g.loop_len = g.loop_pending_len;
            g.loop_pending = 0u;
            g.loop_end_pending = 0u;
            return;
        }
#endif
        g.loop_start_step = (uint8_t)((g.slip_step & ~15u) + g.loop_pending_start);
        g.loop_len = g.loop_pending_len;
        g.loop_active = 1u;
        g.loop_wander_acc = 0u;
        g.loop_pending = 0u;
        g.loop_end_pending = 0u;
        return;
    }
}

static uint8_t seq_chain_at_step(uint8_t st)
{
    uint8_t v = (uint8_t)((st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
    uint32_t i;

    if (seq_view_playable(v))
        return st;
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
        uint8_t w = (uint8_t)((v + i) % FM_SEQ_MAX_PAGES);

        if (seq_view_playable(w))
            return (uint8_t)(i ? w * FM_SEQ_PAGE_STEPS : st);
    }
    return st;
}

/* View chain: enable + CYC + REPS; disabled / CYC-fail views skipped. */
static uint8_t seq_chain_next(uint8_t st)
{
    uint8_t v = (uint8_t)((st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
    uint8_t pos = (uint8_t)(st % FM_SEQ_PAGE_STEPS);

    if (!seq_view_playable(v)) {
        uint8_t ns = seq_chain_at_step(st);

        if (ns != st) {
            seq_view_rep_arm((uint8_t)(ns / FM_SEQ_PAGE_STEPS));
            return ns;
        }
        /* lone view CYC rest: keep transport moving, no audible chain */
        if (pos + 1u < FM_SEQ_PAGE_STEPS)
            return (uint8_t)(st + 1u);
        return (uint8_t)(v * FM_SEQ_PAGE_STEPS);
    }
    if (pos + 1u < FM_SEQ_PAGE_STEPS)
        return (uint8_t)(st + 1u);
    if (g.seq_view_rep_left > 1u) {
        g.seq_view_rep_left--;
        return (uint8_t)(v * FM_SEQ_PAGE_STEPS);
    }
    {
        uint8_t nv = seq_view_next_playable(v, 1u);

        if (nv == 255u)
            return st;
        seq_view_rep_arm(nv);
        return (uint8_t)(nv * FM_SEQ_PAGE_STEPS);
    }
}

/* g.step and g.slip_step onto playable views; a running loop keeps g.step; 0 = none playable */
static int seq_chain_enter(void)
{
    uint8_t sl = seq_chain_at_step(g.slip_step);

    if (!seq_view_playable((uint8_t)(sl / FM_SEQ_PAGE_STEPS)))
        return 0;
    g.slip_step = sl;
    if (!g.loop_active)
        g.step = seq_chain_at_step(g.step);
    return 1;
}

/* steps from the loop start to st along the chain; g.loop_len when st is off the loop */
static uint32_t seq_loop_rel(uint8_t st)
{
    uint8_t s = g.loop_start_step;
    uint32_t n;

    for (n = 0; n < g.loop_len && s != st; n++)
        s = seq_chain_next(s);
    return n;
}

static uint32_t seq_view_rep_steps(uint8_t v)
{
    uint8_t r = dj_seq_eff_reps(v);

    if (r < 1u)
        r = 1u;
    if (r > 16u)
        r = 16u;
    return (uint32_t)FM_SEQ_PAGE_STEPS * (uint32_t)r;
}

static uint32_t seq_chain_linear_total(void)
{
    uint32_t n = 0u;
    uint32_t i;

    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
        if (seq_view_playable((uint8_t)i))
            n += seq_view_rep_steps((uint8_t)i);
    return n;
}

static uint32_t seq_chain_linear_pos1(uint8_t st)
{
    uint8_t v = (uint8_t)((st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
    uint8_t pos = (uint8_t)(st % FM_SEQ_PAGE_STEPS);
    uint32_t chain = 0u;
    uint32_t i;
    int found = 0;

    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
        if (!seq_view_playable((uint8_t)i))
            continue;
        if (i == (uint32_t)v) {
            found = 1;
            break;
        }
        chain += seq_view_rep_steps((uint8_t)i);
    }
    if (!found)
        return chain > 0u ? chain : 0u;
    {
        uint8_t r = dj_seq_eff_reps(v);
        uint32_t rep1;

        if (r < 1u)
            r = 1u;
        if (r > 16u)
            r = 16u;
        if (g.playing) {
            uint8_t left = g.seq_view_rep_left;

            if (left < 1u)
                left = 1u;
            if (left > r)
                left = r;
            rep1 = (uint32_t)r - (uint32_t)left + 1u;
        } else
            rep1 = 1u;
        return chain + (rep1 - 1u) * (uint32_t)FM_SEQ_PAGE_STEPS + (uint32_t)pos + 1u;
    }
}

static void seq_advance_step(uint8_t st)
{
    if (g.rec_arm && g.playing && g.loop_active)
        dub_loop_rec_burn(st, g.slip_step);
    g.slip_step = seq_chain_next(g.slip_step);
    /* Engage/stop on the next beat (4 steps); start offset stays relative to the page */
    if ((g.bar_step & 3u) == 0u)
        dub_loop_pending_apply();
    if (g.loop_active && g.loop_len > 0u) {
        if (seq_loop_rel(st) + 1u >= g.loop_len) {
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            /* Metrical wander: only even step jumps (keep grid with K+S on slip). */
            {
                uint8_t len = g.loop_len;
                uint8_t period;
                uint8_t drift = 0u;
                uint8_t di;

                if (len <= 2u)
                    period = 8u;
                else if (len <= 4u)
                    period = 4u;
                else if (len <= 8u)
                    period = 2u;
                else
                    period = 1u;

                g.loop_wander_acc++;
                if (g.loop_wander_acc >= period) {
                    g.loop_wander_acc = 0u;
                    if (len <= 16u)
                        drift = 2u;
                    else if (len <= 32u)
                        drift = 2u;
                    else if (len <= 64u)
                        drift = 4u;
                    else
                        drift = 8u;
                }
                for (di = 0u; di < drift; di++)
                    g.loop_start_step = seq_chain_next(g.loop_start_step);
                /* Snap page offset to even 16th */
                if ((g.loop_start_step & 1u) != 0u)
                    g.loop_start_step = seq_chain_next(g.loop_start_step);
            }
#endif
            g.step = seq_chain_at_step(g.loop_start_step);
            seq_view_rep_arm((uint8_t)(g.step / FM_SEQ_PAGE_STEPS));
        } else
            g.step = seq_chain_next(st);
    } else
        g.step = seq_chain_next(st);
}

static uint8_t events_gate(const dub_events_t *ev)
{
    uint8_t gate = 0;

    if (!ev)
        return 0;
    if (ev->kick)
        gate |= 1u;
    if (ev->rim)
        gate |= 2u;
    if (ev->hat)
        gate |= 4u;
    if (ev->siren)
        gate |= 8u;
    if (ev->sub)
        gate |= 16u;
    if (ev->chord)
        gate |= 32u;
    return gate;
}

#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_SOUL) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
#define FM_DUB_RESOLVE_STEP(P, OVR, STEP, OUT, DENS) dub_resolve_step((P), (OVR), (STEP), (OUT), (DENS))
#else
#define FM_DUB_RESOLVE_STEP(P, OVR, STEP, OUT, DENS) \
    do { \
        (void)(DENS); \
        dub_resolve_step((P), (OVR), (STEP), (OUT)); \
    } while (0)
#endif

static uint8_t seq_plate_gate(uint8_t st);
static void dub_apply_seq_plate_to_events(uint8_t step, dub_events_t *out);

static void dub_resolve_step_morphed(const dub_step_t *ovr, uint8_t step, dub_events_t *out)
{
    if (!out)
        return;
    if (g.morph_pos >= DUB_MORPH_STEPS || (ovr && (ovr->force & 1u))) {
        FM_DUB_RESOLVE_STEP(&g.voice.p, ovr, step & (DUB_STEPS - 1u), out, 1u);
        if (!ovr || !(ovr->force & 1u))
            dub_apply_seq_plate_to_events(step, out);
#if defined(GENRE_POP)
        {
            uint16_t pos = pop_music_pos(g.bar, step & (DUB_STEPS - 1u));
            pop_arrange_events(g.cfg.p.plate, g.cfg.p.era, pop_mbar(pos),
                               seq_view_on_count() > 0u ? 1u : 0u, out);
        }
#endif
        return;
    }
    {
        const dub_step_t *from_ovr = 0;

        if (fm_seq_bit(g.morph_from_seq, step))
            from_ovr = &g.morph_from_step[step % DUB_SEQ_STEPS];
        /* CROSSFADE/EVOLVE: resolve from morph_from without DENS (matches seq materialize gates) */
        FM_DUB_RESOLVE_STEP(&g.morph_from, from_ovr, step & (DUB_STEPS - 1u), out, 0u);
        out->note = g.morph_from.root;
        if (!from_ovr || !(from_ovr->force & 1u))
            dub_apply_seq_plate_to_events(step, out);
    }
#if defined(GENRE_POP)
    {
        uint16_t pos = pop_music_pos(g.bar, step & (DUB_STEPS - 1u));
        pop_arrange_events(g.cfg.p.plate, g.cfg.p.era, pop_mbar(pos),
                           seq_view_on_count() > 0u ? 1u : 0u, out);
    }
#endif
}

static void dub_plate_seq_morph_prep(void)
{
    uint8_t st;
    uint8_t ln;

    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG)
            return;
    }

    fm_seq_bits_fill(g.morph_from_seq, 0);
    for (st = 0; st < DUB_SEQ_STEPS; st++) {
        dub_step_t *s = &g.cfg.seq[st];

        if (s->force & 1u) {
            g.morph_from_step[st] = *s;
            fm_seq_bit_put(g.morph_from_seq, st, 1);
        }
        s->force = 0u;
    }
}

static uint8_t raw_plate_gate(uint8_t st)
{
    const dub_plate_t *drum = genre_preset(g.cfg.p.drum_pat);
    const dub_plate_t *main = genre_preset(g.cfg.p.plate);
    uint8_t gate = 0u;

    st &= (uint8_t)(DUB_STEPS - 1u);
    if (dub_step_bit(drum->kick, st))
        gate |= 1u;
    if (dub_step_bit(drum->rim, st))
        gate |= 2u;
    if (dub_step_bit(drum->hat, st))
        gate |= 4u;
#ifdef GENRE_HEAVY_PSYCH
    if (genre_lead_step_bit(g.cfg.p.plate, st))
        gate |= 8u;
#elif defined(GENRE_JAZZ) || defined(GENRE_SOUL)
    if (genre_lead_step_bit(g.cfg.p.plate, st))
        gate |= 8u;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    if (genre_lead_step_bit(g.cfg.p.plate, st))
        gate |= 8u;
#elif defined(GENRE_HIPHOP)
    if (genre_lead_step_bit(g.cfg.p.plate, st))
        gate |= 8u;
#else
    if (dub_step_bit(main->siren, st))
        gate |= 8u;
#endif
    if (dub_step_bit(main->sub, st))
        gate |= 16u;
    if (dub_step_bit(main->chord, st))
        gate |= 32u;
    return gate;
}

/* S2..S8 playback gates: fm_views_data via fm_view_step_bit (same plate index as S1 siren/bass/chord). */
static uint8_t seq_view_table_gate(uint8_t st)
{
    uint8_t plate = genre_clamp_preset(g.cfg.p.plate);
    uint8_t gate = 0u;
    uint8_t ln;

    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        if (fm_view_step_bit(plate, ln, (uint32_t)st))
            gate |= (uint8_t)(1u << ln);
    }
    return gate;
}

static uint8_t raw_step_gate_show(uint8_t st)
{
    uint8_t v = fm_seq_view_of_step(st);
    dub_step_t *s = &g.cfg.seq[st];

    if (s->force & 1u)
        return s->gate;
    if (v >= 1u)
        return seq_view_table_gate(st);
    return raw_plate_gate(st);
}

static uint8_t raw_step_gate(uint8_t st)
{
    uint8_t v = fm_seq_view_of_step(st);

    if (v >= 1u && !seq_view_playable(v))
        return 0u;
    return raw_step_gate_show(st);
}

static uint8_t step_gate(uint8_t st)
{
    dub_step_t *s = &g.cfg.seq[st];
    const dub_step_t *use = (s->force & 1u) ? s : 0;
    dub_events_t ev;

    dub_resolve_step_morphed(use, st, &ev);
    return events_gate(&ev);
}

static uint8_t seq_lane_on(uint8_t gate, uint8_t ln)
{
    return (uint8_t)((gate >> (ln % DUB_SEQ_LANES)) & 1u);
}

#if defined(GENRE_BASS)
static uint8_t bass_seq_plate_gate(uint8_t st)
{
    uint8_t slab;

    if (st >= DUB_SEQ_STEPS)
        st = (uint8_t)(DUB_SEQ_STEPS - 1u);
    slab = genre_clamp_preset(g.cfg.p.plate);
    if (!bass_line_lead_step_bit(slab, st))
        return 0u;
    return (uint8_t)(1u << 4u);
}
#endif

static uint8_t seq_plate_gate(uint8_t st)
{
#if defined(GENRE_BASS)
    return bass_seq_plate_gate(st);
#else
    uint8_t v = fm_seq_view_of_step(st);

    if (v >= 1u)
        return seq_view_table_gate(st);
    return raw_plate_gate(st);
#endif
}

static void dub_apply_seq_plate_to_events(uint8_t step, dub_events_t *out)
{
    uint8_t pg;

    if (!out)
        return;
    pg = seq_plate_gate(step);
    out->kick = (pg >> 0) & 1u;
    out->rim = (pg >> 1) & 1u;
    out->hat = (pg >> 2) & 1u;
    out->siren = (pg >> 3) & 1u;
    out->sub = (pg >> 4) & 1u;
    out->chord = (pg >> 5) & 1u;
}

static uint8_t seq_step_has_plock(uint8_t ln, uint8_t st)
{
#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_SOUL) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP)
    if (ln == 3u) {
        uint8_t pl = g.cfg.p.plate;
        uint8_t act = genre_lead_step_bit(pl, st);
        int8_t def_t = act ? genre_lead_pitch(pl, st) : 0;
        int8_t def_m = act ? (int8_t)genre_lead_len_ix(pl, st) : 0;
        uint8_t def_v = act ? genre_lead_bend_plock(genre_lead_bend_u7(pl, st)) : 0u;

        return (g.step_vel[3][st] != def_v) || g.step_prob[3][st] || (g.step_micro[3][st] != def_m)
               || (g.step_tune[3][st] != def_t);
    }
#elif defined(GENRE_POP)
    if (ln == 3u)
        return (g.step_vel[3][st] != 0u) || g.step_prob[3][st] || (pop_lead_len_ix(g.step_micro[3][st]) != 0u)
               || g.step_tune[3][st];
#elif defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP) || defined(FM_DUB_SRN_PLOCK)
    if (ln == 3u)
        return (g.step_vel[3][st] != 0u) || g.step_prob[3][st] || (g.step_micro[3][st] != 0)
               || g.step_tune[3][st];
#endif
    return g.step_vel[ln][st] || g.step_prob[ln][st] || g.step_micro[ln][st] || g.step_tune[ln][st];
}

static uint8_t seq_lane_meta_active(uint8_t ln)
{
    uint8_t st;

    if (g.euclid_hits[ln] || g.euclid_rot[ln] || g.vary_lane[ln])
        return 1u;
    if (g.lane_vel[ln] || g.lane_trig[ln] || g.lane_gate[ln] || g.lane_tune[ln])
        return 1u;
    for (st = 0; st < seq_len(); st++) {
        if (seq_step_has_plock(ln, st))
            return 1u;
    }
    return 0u;
}

static uint8_t seq_pattern_edited(void)
{
    uint8_t st;

    for (st = 0; st < DUB_SEQ_STEPS; st++) {
        if ((g.cfg.seq[st].force & 1u) && g.cfg.seq[st].gate != seq_plate_gate(st))
            return 1u;
    }
    return 0u;
}

static void seq_reconcile_force(void)
{
    uint8_t ln;

    if (seq_pattern_edited())
        return;
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        if (seq_lane_meta_active(ln))
            return;
    }
    /* Keep seq[].force: unforced dub_resolve_step uses step&31 on uint16 plate masks (House/Dub
     * steps 16..31 read as zero). seq_plate_gate / rematerialize hold S0 plate + S1+ FM_VIEWS. */
}

static void seq_materialize_all(void)
{
#if defined(GENRE_BASS)
    bass_seq_sync_genre_pattern();
    return;
#endif
    uint32_t i;

    for (i = 0; i < DUB_SEQ_STEPS; i++) {
        dub_step_t *si = &g.cfg.seq[i];
        if (!(si->force & 1u)) {
            dub_events_t ev;
            uint8_t plate_gate = seq_plate_gate((uint8_t)i);
            uint8_t gate = si->gate;
            uint8_t ln;

            FM_DUB_RESOLVE_STEP(&g.cfg.p, 0, (uint8_t)(i & (DUB_STEPS - 1u)), &ev, 1u);
            /* Only SONG lanes track plate ROM; EDITED / EUCLID lanes keep live gate bits. */
            for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
                uint8_t bit = (uint8_t)(1u << ln);

                if (g.euclid_hits[ln] == FM_SEQ_EUCLID_SONG && !g.cfg.user_song) {
                    if (plate_gate & bit)
                        gate |= bit;
                    else
                        gate &= (uint8_t)~bit;
                }
            }
            si->gate = gate;
            si->note = 0;
            si->dly_throw = ev.dly_throw;
            si->rev_throw = ev.rev_throw;
            si->accent = ev.accent;
            si->force = 1u;
        }
    }
}

/* Once at boot: NVM may keep stale seq[].gate while DUB_CFG_MAGIC is unchanged (e.g. after flash).
 * Must set force — seq_tick only uses g.cfg.seq[st] when force&1; else Resolve falls back to
 * plate ROM with step&31 (sounds like a 32-step sub loop while UI shows FM_VIEWS). */
static void seq_rematerialize_gates_boot(void)
{
    uint32_t i;

#if defined(GENRE_BASS)
    bass_seq_sync_genre_pattern();
    return;
#endif
    for (i = 0; i < DUB_SEQ_STEPS; i++) {
        g.cfg.seq[i].gate = seq_plate_gate((uint8_t)i);
        g.cfg.seq[i].force = 1u;
    }
}

static void seq_set_lane_plate(uint8_t ln, uint8_t st)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));
    uint8_t pg = (uint8_t)((seq_plate_gate(st) >> ln) & 1u);
    dub_step_t *s = &g.cfg.seq[st];

    ln %= DUB_SEQ_LANES;
    if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG)
        return;
    if (pg)
        s->gate |= bit;
    else
        s->gate &= (uint8_t)~bit;
}

static void seq_lane_wipe_step_plocks(uint8_t ln)
{
    uint8_t st;

    ln %= DUB_SEQ_LANES;
    for (st = 0; st < DUB_SEQ_STEPS; st++) {
        g.step_vel[ln][st] = 0u;
        g.step_prob[ln][st] = 0u;
        g.step_micro[ln][st] = 0;
        g.step_tune[ln][st] = 0;
    }
}

static void seq_lane_wipe_plocks(uint8_t ln)
{
    ln %= DUB_SEQ_LANES;
    seq_lane_wipe_step_plocks(ln);
    g.euclid_rot[ln] = 0;
    g.vary_lane[ln] = 0u;
    g.lane_vel[ln] = 0;
    g.lane_trig[ln] = 0;
    g.lane_gate[ln] = 0;
    g.lane_tune[ln] = 0;
}

static void seq_lane_clear_meta(uint8_t ln)
{
    ln %= DUB_SEQ_LANES;
    if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG)
        return;
    g.euclid_hits[ln] = FM_SEQ_EUCLID_SONG;
    seq_lane_wipe_plocks(ln);
}

static void seq_flash_knob(const char *msg);
static void dub_unmute_all_lanes(void);
static void dub_invert_all_lane_mutes(void);
static void seq_lane_enter_song(uint8_t ln);
static void seq_reset_lane_song(uint8_t ln);
static void seq_empty_lane(uint8_t ln);
static void seq_invert_lane(uint8_t ln);
static uint8_t dub_lane_is_solo(uint8_t ln);
static void dub_lane_mute(uint8_t ln);
static void dub_lane_unmute(uint8_t ln);

static uint8_t dub_lane_p3_state_pos(uint8_t ln)
{
    if (dub_lane_is_solo(ln))
        return 2u;
    if (g.voice.lane_mute_mask & (uint8_t)(1u << (ln % DUB_SEQ_LANES)))
        return 0u;
    return 1u;
}

static void dub_apply_mute_mask(uint8_t mp)
{
    uint32_t i;

    for (i = 0; i < DUB_SEQ_LANES; i++) {
        if (mp & (uint8_t)(1u << i))
            dub_lane_mute((uint8_t)i);
        else
            dub_lane_unmute((uint8_t)i);
    }
}

static uint8_t dub_ens_knob_bar(void);

static void dub_dj_ens_ov_restore_from_base(void)
{
    if (!g.dj_ens_ov.active)
        return;
    if (g.dj_ens_ov.base_ens == FM_SEQ_ENS_CUSTOM)
        g.voice.lane_mute_mask =
            (uint8_t)((g.voice.lane_mute_mask & 0xc0u) | (g.dj_ens_ov.base_mask & 0x3fu));
    else {
        uint8_t e = g.dj_ens_ov.base_ens;

        if (e >= FM_SEQ_NENSEMBLES)
            e = FM_SEQ_ENS_ALL;
        g.voice.lane_solo_mask = 0u;
        dub_apply_mute_mask(DUB_ENS_MUTE[e]);
    }
}

static void dub_dj_ens_invert_accent_sync(uint8_t on)
{
    if (on) {
        uint8_t m = (uint8_t)(g.voice.lane_mute_mask & 0x3fu);
        uint8_t inv = fm_dj_seq_invert_mask(m);

        if (!fm_dj_seq_mask_dj_intervention_ok(inv))
            on = 0u;
    }
    if (on != g.dj_ens_invert_overlay) {
        dub_invert_all_lane_mutes();
        g.dj_ens_invert_overlay = on;
    }
}

static void dub_apply_dj_seq_overlay(void)
{
    int32_t ed;
    int32_t fd;
    uint8_t want_flip;
    uint8_t blocked;
    uint8_t m_apply;

    if (!g.dj.on && !g.dj.slew_off) {
        if (g.dj_flip_overlay) {
            dub_invert_all_lane_mutes();
            g.dj_flip_overlay = 0u;
        }
        dub_dj_ens_invert_accent_sync(0u);
        dub_dj_ens_ov_restore_from_base();
        fm_dj_seq_ens_ov_reset(&g.dj_ens_ov);
        fm_dj_lfo_ov_reset(&g.dj_lfo_ov);
        fm_dj_fx_ov_reset(&g.dj_fx_ov);
        fm_dj_lfo_eff_build(&g.dj_lfo_ov, &g.dj, g.lfo_slots, g.lfo_slots_eff, DUB_LFO_TGT_N);
        dub_dj_gest_restore();
        return;
    }
    blocked = fm_dj_seq_ens_blocked(&g.dj, g.playing ? 1u : 0u, g.bar_step, DUB_STEPS);
    ed = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_ENSEMBLE);
    m_apply = fm_dj_seq_ens_ov_step(&g.dj, &g.dj_ens_ov, g.bar, g.bar_step, DUB_STEPS, g.playing ? 1u : 0u,
                                      g.seq_ens_sel, (uint8_t)(g.voice.lane_mute_mask & 0x3fu), ed, DUB_ENS_MUTE,
                                      blocked);
    if (m_apply != 0xffu) {
        uint8_t eff = m_apply;

        g.voice.lane_mute_mask = (uint8_t)((g.voice.lane_mute_mask & 0xc0u) | eff);
    }
    dub_dj_ens_invert_accent_sync(g.dj_ens_ov.invert_accent);
    if (g.dj_ens_ov.active && g.dj_ens_ov.seg != FM_DJ_SEQ_ENS_OV_SEG_RES) {
        uint8_t eff = (uint8_t)(g.voice.lane_mute_mask & 0x3fu);

        if (!fm_dj_seq_mask_dj_intervention_ok(eff)) {
            uint8_t safe = fm_dj_seq_ens_safe_combo(DUB_ENS_MUTE);

            g.dj_ens_ov.combo_cur = safe;
            g.dj_ens_ov.toast_ens = safe;
            g.voice.lane_mute_mask =
                (uint8_t)((g.voice.lane_mute_mask & 0xc0u) | fm_dj_seq_mask_low6(DUB_ENS_MUTE[safe]));
            g.dj_ens_invert_overlay = 0u;
            g.dj_ens_ov.invert_accent = 0u;
            g.dj_ens_ov.invert_left = 0u;
        }
    }
    fd = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_FLIP);
    want_flip = fm_dj_seq_flip_want(&g.dj, g.playing ? 1u : 0u, g.bar_step, DUB_STEPS, fd);
    if (want_flip != g.dj_flip_overlay) {
        dub_invert_all_lane_mutes();
        g.dj_flip_overlay = want_flip;
    }
}

static void dub_apply_lfo_seq_mutes(void)
{
    if (g.lfo_slots[DUB_LFO_ENS].depth > 0u) {
        uint8_t ens = lfo_mod_enum(g.seq_ens_sel, DUB_LFO_ENS, FM_SEQ_NENSEMBLES);

        if (ens >= FM_SEQ_NENSEMBLES)
            ens = FM_SEQ_ENS_ALL;
        if (ens != g.lfo_last_ens) {
            g.voice.lane_solo_mask = 0u;
            dub_apply_mute_mask(DUB_ENS_MUTE[ens]);
            g.lfo_last_ens = ens;
        }
    } else
        g.lfo_last_ens = g.seq_ens_sel;

    if (g.lfo_slots[DUB_LFO_FLIP].depth > 0u) {
        int32_t fd = fm_lfo_slot_delta(g.lfo_slots, g.lfo_rt, DUB_LFO_FLIP);
        uint8_t flip_on = fd > 16 ? 1u : (fd < -16 ? 0u : g.seq_flip_state);

        if (flip_on != g.seq_flip_state) {
            dub_invert_all_lane_mutes();
            g.seq_flip_state = flip_on;
            g.dj_flip_overlay = 0u;
        }
    }
    dub_apply_dj_seq_overlay();
}

static void dub_lane_apply_p3_state(uint8_t ln, uint8_t pos)
{
    uint32_t i;
    uint8_t bit;
    ln %= DUB_SEQ_LANES;
    bit = (uint8_t)(1u << ln);
    pos %= 3u;
    if (pos == 0u) {
        g.voice.lane_solo_mask &= (uint8_t)~bit;
        dub_lane_mute(ln);
    } else if (pos == 1u) {
        if (g.voice.lane_solo_mask & bit) {
            g.voice.lane_solo_mask &= (uint8_t)~bit;
            if (g.voice.lane_solo_mask == 0u)
                dub_unmute_all_lanes();
        } else {
            dub_lane_unmute(ln);
        }
    } else {
        g.voice.lane_solo_mask = bit;
        for (i = 0; i < DUB_SEQ_LANES; i++) {
            if (i == (uint32_t)ln)
                dub_lane_unmute((uint8_t)i);
            else
                dub_lane_mute((uint8_t)i);
        }
    }
}

static void dub_lane_apply_p3_state_all(uint8_t pos)
{
    uint32_t i;

    pos %= 3u;
    if (pos == 0u) {
        g.voice.lane_solo_mask = 0u;
        for (i = 0; i < DUB_SEQ_LANES; i++)
            dub_lane_mute((uint8_t)i);
    } else if (pos == 1u) {
        dub_unmute_all_lanes();
    } else {
        g.voice.lane_solo_mask = (uint8_t)((1u << DUB_SEQ_LANES) - 1u);
        for (i = 0; i < DUB_SEQ_LANES; i++)
            dub_lane_unmute((uint8_t)i);
    }
}

static uint8_t dub_detect_ensemble(void)
{
    uint8_t m = (uint8_t)(g.voice.lane_mute_mask & 0x3Fu);
    uint32_t i;

    for (i = 0; i < FM_SEQ_NENSEMBLES; i++) {
        if (m == DUB_ENS_MUTE[i])
            return (uint8_t)i;
    }
    return FM_SEQ_ENS_CUSTOM;
}

static uint8_t dub_ens_knob_bar(void)
{
    uint8_t det = dub_detect_ensemble();
    if (det == FM_SEQ_ENS_CUSTOM) {
        uint32_t i;
        uint8_t act = 0u;

        for (i = 0; i < DUB_SEQ_LANES; i++) {
            if (!(g.voice.lane_mute_mask & (uint8_t)(1u << i)))
                act++;
        }
        return fm_seq_ens_bar_custom(act, DUB_SEQ_LANES);
    }
    return fm_seq_ens_bar_preset(det);
}

static void dub_apply_ensemble(uint8_t ens)
{
    if (ens >= FM_SEQ_NENSEMBLES)
        ens = FM_SEQ_ENS_ALL;
    g.seq_ens_sel = ens;
    g.voice.lane_solo_mask = 0u;
    dub_apply_mute_mask(DUB_ENS_MUTE[ens]);
    mark_dirty();
}

static void seq_flash_knob(const char *msg)
{
    ko_flash_set(&g.seq_knob_flash, msg, fm1_ms, 900u);
}

static void seq_flash_bpm_val(uint8_t bpm)
{
    char s[8];
    char *p;

    u8_dec(s, bpm);
    p = s;
    while (*p)
        p++;
    copy_str(p, "BPM");
    seq_flash_knob(s);
}

static void seq_flash_key_val(uint8_t root)
{
    char s[8];
    char *p;

    copy_str(s, "KEY ");
    p = s;
    while (*p)
        p++;
    copy_str(p, NOTE_NAMES[root % 12u]);
    seq_flash_knob(s);
}

static uint8_t *dub_lane_level_param(uint8_t ln)
{
    dub_params_t *p = &g.cfg.p;

    ln %= DUB_SEQ_LANES;
    if (ln == 0u)
        return &p->kick_boom;
    if (ln == 1u)
        return &p->rim_tone;
    if (ln == 2u)
        return &p->hat_lvl;
    if (ln == 3u)
        return &p->siren_pitch;
    if (ln == 4u)
        return &p->lvl_b;
    return &p->lvl_c;
}

static void dub_sync_bus_mute_from_mask(uint8_t lane_mask)
{
    uint8_t dm = g.voice.mute & (uint8_t)~15u;

    if ((lane_mask >> 4) & 1u)
        dm |= 1u;
    if ((lane_mask >> 5) & 1u)
        dm |= 2u;
    if (((lane_mask >> 0) & 1u) && ((lane_mask >> 1) & 1u) && ((lane_mask >> 2) & 1u))
        dm |= 4u;
    if ((lane_mask >> 3) & 1u)
        dm |= 8u;
    g.voice.mute = dm;
}

static void dub_apply_heard_lane_levels(uint8_t prev, uint8_t now)
{
    uint32_t ln;

    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        uint8_t bit = (uint8_t)(1u << ln);
        uint8_t *lv = dub_lane_level_param((uint8_t)ln);

        if ((now & bit) && !(prev & bit)) {
            if (*lv > 0u)
                g.voice.saved_lane_lvl[ln] = *lv;
            *lv = 0u;
        } else if (!(now & bit) && (prev & bit) && *lv == 0u)
            *lv = g.voice.saved_lane_lvl[ln];
    }
}

static void dub_sync_heard_mutes(void)
{
    uint8_t prev = g.voice.heard_mute_mask;
    uint8_t now = g.voice.lane_mute_mask;

    g.voice.heard_mute_mask = now;
    dub_apply_heard_lane_levels(prev, now);
    dub_sync_bus_mute_from_mask(now);
}

static void dub_maybe_sync_heard_mutes(void)
{
    if (!g.playing)
        dub_sync_heard_mutes();
}

static uint8_t dub_bus_mute_blocks_lane(uint8_t ln)
{
    uint8_t dm = g.voice.mute;

    if (ln <= 2u)
        return (dm >> 2) & 1u;
    if (ln == 3u)
        return (dm >> 3) & 1u;
    if (ln == 4u)
        return dm & 1u;
    return (dm >> 1) & 1u;
}

static uint8_t dub_lane_is_solo(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    return (g.voice.lane_solo_mask & bit) && !(g.voice.lane_mute_mask & bit) ? 1u : 0u;
}

static uint8_t dub_any_lane_mute_or_solo(void)
{
    return (g.voice.lane_mute_mask || g.voice.lane_solo_mask) ? 1u : 0u;
}

static uint8_t dub_lane_bus_blocks(uint8_t ln, uint8_t lane_mask)
{
    uint8_t dm = g.voice.mute & (uint8_t)~15u;

    if ((lane_mask >> 4) & 1u)
        dm |= 1u;
    if ((lane_mask >> 5) & 1u)
        dm |= 2u;
    if (((lane_mask >> 0) & 1u) && ((lane_mask >> 1) & 1u) && ((lane_mask >> 2) & 1u))
        dm |= 4u;
    if ((lane_mask >> 3) & 1u)
        dm |= 8u;
    if (ln <= 2u)
        return (dm >> 2) & 1u;
    if (ln == 3u)
        return (dm >> 3) & 1u;
    if (ln == 4u)
        return dm & 1u;
    return (dm >> 1) & 1u;
}

static uint8_t dub_lane_audible(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    if (g.voice.lane_mute_mask & bit)
        return 0u;
    return dub_lane_bus_blocks(ln, g.voice.lane_mute_mask) ? 0u : 1u;
}

static uint8_t dub_lane_heard_audible(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    if (g.voice.heard_mute_mask & bit)
        return 0u;
    return dub_bus_mute_blocks_lane(ln) ? 0u : 1u;
}

static void dub_lane_unmute(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));
    uint8_t *lv;

    g.voice.lane_mute_mask &= (uint8_t)~bit;
    lv = dub_lane_level_param(ln);
    if (*lv == 0u)
        *lv = g.voice.saved_lane_lvl[ln];
    dub_maybe_sync_heard_mutes();
}

static void dub_lane_mute(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));
    uint8_t *lv = dub_lane_level_param(ln);

    if (*lv > 0u)
        g.voice.saved_lane_lvl[ln] = *lv;
    g.voice.lane_mute_mask |= bit;
    (void)lv;
    dub_maybe_sync_heard_mutes();
}

static void dub_unmute_all_lanes(void)
{
    uint32_t ln;

    g.voice.lane_mute_mask = 0u;
    g.voice.lane_solo_mask = 0u;
    for (ln = 0; ln < DUB_SEQ_LANES; ln++)
        dub_lane_unmute((uint8_t)ln);
}

static void dub_invert_all_lane_mutes(void)
{
    uint32_t ln;
    uint8_t was = g.voice.lane_mute_mask;

    g.voice.lane_solo_mask = 0u;
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        uint8_t bit = (uint8_t)(1u << ln);
        if (was & bit)
            dub_lane_unmute((uint8_t)ln);
        else
            dub_lane_mute((uint8_t)ln);
    }
}

static void dub_lane_toggle_mute(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    if (g.voice.lane_mute_mask & bit) {
        dub_lane_unmute(ln);
        if (g.voice.lane_solo_mask)
            g.voice.lane_solo_mask |= bit;
    } else {
        dub_lane_mute(ln);
        if (g.voice.lane_solo_mask)
            g.voice.lane_solo_mask &= (uint8_t)~bit;
    }
}

static void dub_lane_toggle_solo(uint8_t ln)
{
    uint32_t i;
    uint8_t bit;

    ln %= DUB_SEQ_LANES;
    bit = (uint8_t)(1u << ln);
    if (g.voice.lane_solo_mask & bit) {
        g.voice.lane_solo_mask &= (uint8_t)~bit;
        if (g.voice.lane_solo_mask == 0u)
            dub_unmute_all_lanes();
        else
            dub_lane_mute(ln);
    } else if (g.voice.lane_solo_mask == 0u) {
        g.voice.lane_solo_mask = bit;
        for (i = 0; i < DUB_SEQ_LANES; i++) {
            if (i == (uint32_t)ln)
                dub_lane_unmute((uint8_t)i);
            else
                dub_lane_mute((uint8_t)i);
        }
    } else {
        g.voice.lane_solo_mask |= bit;
        dub_lane_unmute(ln);
    }
    mark_dirty();
}

static void dub_lane_sync_level_mute(uint8_t ln, uint8_t lvl)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));
    uint8_t *lv = dub_lane_level_param(ln);

    if (lvl == 0u) {
        if (*lv > 0u)
            g.voice.saved_lane_lvl[ln] = *lv;
        g.voice.lane_mute_mask |= bit;
        g.voice.heard_mute_mask |= bit;
        *lv = 0u;
        dub_sync_bus_mute_from_mask(g.voice.heard_mute_mask);
    } else {
        g.voice.lane_mute_mask &= (uint8_t)~bit;
        g.voice.heard_mute_mask &= (uint8_t)~bit;
        if (g.voice.lane_solo_mask)
            g.voice.lane_solo_mask |= bit;
        if (ln <= 2u)
            g.voice.mute &= (uint8_t)~4u;
        else if (ln == 3u)
            g.voice.mute &= (uint8_t)~8u;
        else if (ln == 4u)
            g.voice.mute &= (uint8_t)~1u;
        else
            g.voice.mute &= (uint8_t)~2u;
        g.voice.saved_lane_lvl[ln] = lvl;
        dub_sync_bus_mute_from_mask(g.voice.heard_mute_mask);
    }
}

static void dub_lane_apply_plate_gates(uint8_t ln)
{
    const dub_plate_t *pl;
    uint8_t st;
    uint8_t plate;

    ln %= DUB_SEQ_LANES;
    plate = genre_clamp_preset(g.cfg.p.plate);
    pl = genre_preset(plate);
    for (st = 0; st < DUB_SEQ_STEPS; st++)
        seq_set_lane_plate(ln, st);
    dub_lane_apply_plate_seq(ln, plate, pl);
    seq_reconcile_force();
    mark_dirty();
    g.ui_dirty = 1u;
}

static void dub_lane_restore_plate(uint8_t ln)
{
    ln %= DUB_SEQ_LANES;
    if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG)
        return;
    seq_lane_clear_meta(ln);
    if (!g.cfg.user_song)
        dub_lane_apply_plate_gates(ln);
}

static void seq_lane_enter_song(uint8_t ln)
{
    ln %= DUB_SEQ_LANES;
    seq_lane_wipe_plocks(ln);
    g.euclid_hits[ln] = FM_SEQ_EUCLID_SONG;
    if (!g.cfg.user_song)
        dub_lane_apply_plate_gates(ln);
}

static void dub_plate_seq_reload_song_lanes(uint8_t plate)
{
    uint8_t ln;

#if defined(GENRE_BASS)
    (void)plate;
    bass_fader_to_plate();
    bass_seq_sync_genre_pattern();
    return;
#endif
    (void)plate;
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        if (g.euclid_hits[ln] == FM_SEQ_EUCLID_SONG)
            dub_lane_restore_plate(ln);
    }
}

static void seq_lane_pattern_mark_edited(uint8_t ln)
{
    ln %= DUB_SEQ_LANES;
    fm_seq_lane_pattern_mark_edited(&g.euclid_hits[ln], &g.pattern_origin[ln]);
}

static void seq_reset_lane_song(uint8_t ln)
{
    dub_lane_restore_plate(ln);
    dub_lane_unmute(ln);
    mark_dirty();
    g.ui_dirty = 1u;
}

static void seq_empty_lane(uint8_t ln)
{
    uint8_t st;
    uint8_t bit;

    ln %= DUB_SEQ_LANES;
    bit = (uint8_t)(1u << ln);
    seq_lane_clear_meta(ln);
    seq_materialize_all();
    for (st = 0; st < DUB_SEQ_STEPS; st++)
        g.cfg.seq[st].gate &= (uint8_t)~bit;
    seq_reconcile_force();
    mark_dirty();
    g.ui_dirty = 1u;
}

static void seq_invert_lane(uint8_t ln)
{
    uint8_t st;
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    ln %= DUB_SEQ_LANES;
    seq_materialize_all();
    for (st = 0; st < DUB_SEQ_STEPS; st++) {
        dub_step_t *s = &g.cfg.seq[st];
        s->gate ^= bit;
    }
    seq_lane_pattern_mark_edited(ln);
    mark_dirty();
    g.ui_dirty = 1u;
}

static void seq_clear_lane(uint8_t ln)
{
    seq_reset_lane_song(ln);
}

static uint8_t seq_lane_no_gate_bits(uint8_t ln)
{
    uint8_t st;
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    for (st = 0; st < FM_SEQ_STEPS; st++) {
        if (g.cfg.seq[st].gate & bit)
            return 0u;
    }
    return 1u;
}

static uint8_t seq_lane_empty(uint8_t ln)
{
    uint8_t st;
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    seq_materialize_all();
    for (st = 0; st < FM_SEQ_STEPS; st++) {
        if (g.cfg.seq[st].gate & bit)
            return 0u;
    }
    return 1u;
}

static void seq_lane_arm_step(uint8_t ln, uint8_t st)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));
    dub_step_t *s = &g.cfg.seq[st];

    if (g.euclid_hits[ln] == FM_SEQ_EUCLID_EMPTY) {
        uint8_t st2;

        for (st2 = 0; st2 < DUB_SEQ_STEPS; st2++)
            g.cfg.seq[st2].gate &= (uint8_t)~bit;
    } else if (!seq_lane_no_gate_bits(ln))
        seq_materialize_all();
    s->gate |= bit;
    s->force = 1u;
}

static uint8_t seq_lane_on_song_plate(uint8_t ln)
{
    uint8_t st;

    if (seq_lane_meta_active(ln))
        return 0u;
    seq_materialize_all();
    for (st = 0; st < seq_len(); st++) {
        uint8_t pg = (uint8_t)((seq_plate_gate(st) >> ln) & 1u);
        uint8_t gbit = (uint8_t)((g.cfg.seq[st].gate >> ln) & 1u);
        if (gbit != pg)
            return 0u;
    }
    return 1u;
}

static uint8_t seq_lane_content_is_song(uint8_t ln)
{
    uint8_t st;

    ln %= DUB_SEQ_LANES;
    for (st = 0; st < seq_len(); st++) {
        dub_step_t *s = &g.cfg.seq[st];

#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_SOUL) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP)
        if (ln == 3u) {
            uint8_t act = genre_lead_step_bit(g.cfg.p.plate, st);
            int8_t def_t = act ? genre_lead_pitch(g.cfg.p.plate, st) : 0;

            if (g.step_tune[3][st] != def_t)
                return 0u;
        } else if (g.step_tune[ln][st])
            return 0u;
#else
        if (g.step_tune[ln][st])
            return 0u;
#endif
        if (s->force & 1u) {
            uint8_t gbit = (uint8_t)((s->gate >> ln) & 1u);
            uint8_t pg = (uint8_t)((seq_plate_gate(st) >> ln) & 1u);

            if (gbit != pg)
                return 0u;
        }
    }
    return 1u;
}

static const char *seq_lane_clear_label(uint8_t ln)
{
    if (seq_lane_empty(ln))
        return "EMPTY";
    if (seq_lane_on_song_plate(ln))
        return "BASE";
    return "EDITED";
}

static uint8_t dub_lane_state_ix(uint8_t ln)
{
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    if (dub_lane_is_solo(ln))
        return FM_SEQ_STATE_SOLO;
    if ((g.voice.lane_mute_mask & bit) || !dub_lane_audible(ln))
        return FM_SEQ_STATE_MUTE;
    return FM_SEQ_STATE_PLAY;
}

static void dub_lane_apply_state(uint8_t ln, uint8_t state, uint8_t prev, uint8_t edit)
{
    if (edit) {
        dub_lane_unmute(ln);
        return;
    }
    if (state == FM_SEQ_STATE_PLAY)
        dub_lane_unmute(ln);
    else if (state == FM_SEQ_STATE_MUTE)
        dub_lane_mute(ln);
    else if (state == FM_SEQ_STATE_SOLO)
        dub_lane_toggle_solo(ln);
    else if (state == FM_SEQ_STATE_INV) {
        (void)prev;
        seq_invert_lane(ln);
        seq_flash_knob("INVERT");
    }
}

static uint8_t dub_seq_rng_u8(uint8_t n)
{
    if (n == 0u)
        return 0u;
    dub_rng_step();
    return (uint8_t)(((g.seq_rng >> 16) & 0xffffu) % (uint32_t)n);
}

static uint8_t dub_mask_popcount16(uint16_t m)
{
    uint8_t c = 0u;
    uint8_t i;

    for (i = 0u; i < 16u; i++) {
        if (m & (uint16_t)(1u << i))
            c++;
    }
    return c;
}

static uint16_t dub_mask_drop_one(uint16_t m)
{
    uint8_t n = dub_mask_popcount16(m);
    uint8_t pick;
    uint8_t k = 0u;
    uint8_t i;

    if (n <= 1u)
        return m;
    pick = dub_seq_rng_u8(n);
    for (i = 0u; i < 16u; i++) {
        if (m & (uint16_t)(1u << i)) {
            if (k == pick)
                return (uint16_t)(m & (uint16_t)~(1u << i));
            k++;
        }
    }
    return m;
}

static uint16_t dub_mask_add_one(uint16_t m, uint16_t legal)
{
    uint8_t cand[16];
    uint8_t nc = 0u;
    uint8_t i;

    for (i = 0u; i < 16u; i++) {
        if ((legal & (uint16_t)(1u << i)) && !(m & (uint16_t)(1u << i)))
            cand[nc++] = i;
    }
    if (nc == 0u)
        return m;
    i = cand[dub_seq_rng_u8(nc)];
    return (uint16_t)(m | (uint16_t)(1u << i));
}

static uint16_t dub_fresh_pick_mask(const uint16_t *tpl, uint8_t n_tpl, uint8_t min_h, uint8_t max_h,
                                    uint16_t legal)
{
    uint16_t m;
    uint8_t rot;

    m = tpl[dub_seq_rng_u8(n_tpl)];
    rot = dub_seq_rng_u8(4u);
    m = fm_seq_rot16(m, rot);
    if (dub_seq_rng_u8(2u) == 0u && dub_mask_popcount16(m) > min_h)
        m = dub_mask_drop_one(m);
    if (dub_seq_rng_u8(2u) == 0u && dub_mask_popcount16(m) < max_h)
        m = dub_mask_add_one(m, legal);
    if (dub_mask_popcount16(m) > max_h)
        m = dub_mask_drop_one(m);
    return m;
}

static uint16_t dub_fresh_lane_mask(uint8_t ln)
{
    static const uint16_t KICK[] = {
        (uint16_t)((1u << 0) | (1u << 8)),
        (uint16_t)((1u << 0) | (1u << 8) | (1u << 4)),
        (uint16_t)((1u << 0) | (1u << 8) | (1u << 12)),
        (uint16_t)((1u << 0) | (1u << 4) | (1u << 8) | (1u << 12)),
        (uint16_t)((1u << 0) | (1u << 8) | (1u << 14)),
        (uint16_t)((1u << 0) | (1u << 3) | (1u << 8) | (1u << 12)),
    };
    static const uint16_t RIM[] = {
        (uint16_t)((1u << 4) | (1u << 12)),
        (uint16_t)((1u << 2) | (1u << 6) | (1u << 10) | (1u << 14)),
        (uint16_t)((1u << 4) | (1u << 12) | (1u << 2)),
        (uint16_t)((1u << 6) | (1u << 14)),
        (uint16_t)((1u << 4) | (1u << 10) | (1u << 12)),
    };
    static const uint16_t HAT[] = {
        0xAAAAu,
        0x5555u,
        0xAAAAu,
        0x6B6Bu,
        0xDB6Eu,
        0xDDDDu,
    };
    static const uint16_t SIREN[] = {
        (uint16_t)(1u << 4),
        (uint16_t)(1u << 8),
        (uint16_t)((1u << 6) | (1u << 12)),
        (uint16_t)((1u << 2) | (1u << 10)),
        (uint16_t)((1u << 4) | (1u << 12)),
    };
    static const uint16_t BASS[] = {
        (uint16_t)((1u << 0) | (1u << 8)),
        (uint16_t)((1u << 0) | (1u << 8) | (1u << 4)),
        (uint16_t)((1u << 0) | (1u << 2) | (1u << 4) | (1u << 8) | (1u << 10) | (1u << 12)),
        (uint16_t)((1u << 0) | (1u << 4) | (1u << 8) | (1u << 12)),
        (uint16_t)((1u << 0) | (1u << 6) | (1u << 8) | (1u << 14)),
    };
    static const uint16_t CHORD[] = {
        (uint16_t)((1u << 4) | (1u << 12)),
        (uint16_t)((1u << 3) | (1u << 11)),
        (uint16_t)((1u << 4) | (1u << 12) | (1u << 0)),
        (uint16_t)((1u << 3) | (1u << 11) | (1u << 8)),
    };
    uint16_t legal = 0xFFFFu;

    ln %= DUB_SEQ_LANES;
    if (ln == 3u)
        legal = (uint16_t)~1u;
    switch (ln) {
    case 0u:
        return dub_fresh_pick_mask(KICK, (uint8_t)(sizeof KICK / sizeof KICK[0]), 2u, 6u, legal);
    case 1u:
        return dub_fresh_pick_mask(RIM, (uint8_t)(sizeof RIM / sizeof RIM[0]), 1u, 5u, legal);
    case 2u:
        return dub_fresh_pick_mask(HAT, (uint8_t)(sizeof HAT / sizeof HAT[0]), 4u, 12u, legal);
    case 3u:
        return dub_fresh_pick_mask(SIREN, (uint8_t)(sizeof SIREN / sizeof SIREN[0]), 1u, 3u, legal);
    case 4u:
        return dub_fresh_pick_mask(BASS, (uint8_t)(sizeof BASS / sizeof BASS[0]), 2u, 8u, legal);
    default:
        return dub_fresh_pick_mask(CHORD, (uint8_t)(sizeof CHORD / sizeof CHORD[0]), 2u, 4u, legal);
    }
}

static uint8_t dub_fresh_cond(uint8_t ln, uint8_t st)
{
    static const uint8_t KICK[] = {0u, 0u, 0u, 0u, 8u};
    static const uint8_t RIM[] = {0u, 0u, 6u, 7u, 8u};
    static const uint8_t HAT[] = {0u, 0u, 2u, 3u, 6u, 7u};
    static const uint8_t SIREN[] = {0u, 6u, 7u, 8u, 9u};
    static const uint8_t BASS[] = {0u, 0u, 0u, 6u, 8u};
    static const uint8_t CHORD[] = {0u, 6u, 7u, 8u, 10u};
    const uint8_t *pool;
    uint8_t n;

    if ((st & 3u) == 0u && dub_seq_rng_u8(3u) != 0u)
        return 0u;
    if (ln == 0u) {
        pool = KICK;
        n = (uint8_t)(sizeof KICK);
    } else if (ln == 1u) {
        pool = RIM;
        n = (uint8_t)(sizeof RIM);
    } else if (ln == 2u) {
        pool = HAT;
        n = (uint8_t)(sizeof HAT);
    } else if (ln == 3u) {
        pool = SIREN;
        n = (uint8_t)(sizeof SIREN);
    } else if (ln == 4u) {
        pool = BASS;
        n = (uint8_t)(sizeof BASS);
    } else {
        pool = CHORD;
        n = (uint8_t)(sizeof CHORD);
    }
    return pool[dub_seq_rng_u8(n)];
}

static uint8_t dub_fresh_vel(uint8_t ln, uint8_t st)
{
    uint8_t down = (uint8_t)((st & 3u) == 0u);

    if (dub_seq_rng_u8(down ? 2u : 3u) != 0u)
        return 0u;
    if (ln == 2u)
        return (uint8_t)(48u + dub_seq_rng_u8(40u));
    if (down)
        return (uint8_t)(100u + dub_seq_rng_u8(28u));
    return (uint8_t)(64u + dub_seq_rng_u8(40u));
}

#if defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_BASS)
static int8_t dub_fresh_micro(uint8_t ln)
{
    int32_t m;

    (void)ln;
    if (dub_seq_rng_u8(2u) != 0u)
        return 0;
    m = (int32_t)dub_seq_rng_u8(37u) - 18;
    return (int8_t)m;
}
#elif defined(GENRE_IDM)
static int8_t dub_fresh_micro(uint8_t ln)
{
    int32_t m;

    if (ln == 0u || dub_seq_rng_u8(3u) != 0u)
        return 0;
    m = (int32_t)dub_seq_rng_u8(25u) - 12;
    return (int8_t)m;
}
#else
static int8_t dub_fresh_micro(uint8_t ln)
{
    int32_t m;

    if (ln == 0u || dub_seq_rng_u8(3u) != 0u)
        return 0;
    m = (int32_t)dub_seq_rng_u8(25u) - 12;
    return (int8_t)m;
}
#endif

#ifdef FM_L3_GATE
/* Fresh lane-3 GATE lock: ~1/3 of steps stay unlocked, rest short/mid lengths (1/8 … 1/1). */
static int8_t dub_fresh_l3_gate(void)
{
    static const uint8_t PICK16[9] = {2u, 2u, 4u, 4u, 4u, 8u, 8u, 12u, 16u};
    uint8_t want;
    uint32_t ix;

    if (dub_seq_rng_u8(3u) == 0u)
        return 0;
    want = PICK16[dub_seq_rng_u8((uint8_t)sizeof PICK16)];
    for (ix = 0u; ix < (uint32_t)FM_L3_LEN_N; ix++) {
        if (FM_L3_LEN_IX(FM_L3_LEN_STORE(ix)) == ix && FM_L3_LEN_16THS[ix] == want)
            return FM_L3_LEN_STORE(ix);
    }
    return 0;
}
#endif

static int8_t dub_fresh_pitch(uint8_t ln)
{
    static const int8_t BASS[] = {0, 0, 0, 3, 5, 7, -2, 10, 12};
    static const int8_t CHORD[] = {0, 0, 3, 7, 10, 12};
    static const int8_t DRUM[] = {-2, -1, 0, 1, 2};

    if (ln == 4u)
        return BASS[dub_seq_rng_u8((uint8_t)(sizeof BASS))];
    if (ln == 5u)
        return CHORD[dub_seq_rng_u8((uint8_t)(sizeof CHORD))];
    if (dub_seq_rng_u8(5u) != 0u)
        return 0;
    return DRUM[dub_seq_rng_u8((uint8_t)(sizeof DRUM))];
}

static void dub_seq_fresh_lane(uint8_t ln)
{
    uint8_t st;
    uint8_t bit;
    uint16_t mask16;
    uint32_t mask;

    ln %= DUB_SEQ_LANES;
    bit = (uint8_t)(1u << ln);
    mask16 = dub_fresh_lane_mask(ln);
    mask = (uint32_t)mask16 | ((uint32_t)mask16 << 16);
    seq_materialize_all();
    for (st = 0u; st < DUB_STEPS; st++) {
        dub_step_t *s = &g.cfg.seq[st];
        int on = (mask & (1u << st)) != 0u;

        if (on)
            s->gate |= bit;
        else
            s->gate &= (uint8_t)~bit;
        g.step_vel[ln][st] = 0u;
        g.step_prob[ln][st] = 0u;
        g.step_micro[ln][st] = 0;
        g.step_tune[ln][st] = 0;
        if (!on)
            continue;
        g.step_prob[ln][st] = dub_fresh_cond(ln, st);
        g.step_vel[ln][st] = dub_fresh_vel(ln, st);
#ifdef FM_L3_GATE
        g.step_micro[ln][st] = ln == 3u ? dub_fresh_l3_gate() : dub_fresh_micro(ln);
#else
        g.step_micro[ln][st] = dub_fresh_micro(ln);
#endif
        g.step_tune[ln][st] = dub_fresh_pitch(ln);
    }
    for (st = DUB_STEPS; st < DUB_SEQ_STEPS; st++) {
        dub_step_t *s = &g.cfg.seq[st];

        s->gate = (uint8_t)((s->gate & (uint8_t)~bit) | (g.cfg.seq[st & (DUB_STEPS - 1u)].gate & bit));
        g.step_vel[ln][st] = g.step_vel[ln][st & (DUB_STEPS - 1u)];
        g.step_prob[ln][st] = g.step_prob[ln][st & (DUB_STEPS - 1u)];
        g.step_micro[ln][st] = g.step_micro[ln][st & (DUB_STEPS - 1u)];
        g.step_tune[ln][st] = g.step_tune[ln][st & (DUB_STEPS - 1u)];
    }
    mark_dirty();
    g.ui_dirty = 1u;
}

static void seq_apply_euclid(uint8_t ln)
{
    uint8_t st;
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));

    ln %= DUB_SEQ_LANES;
    if (g.euclid_hits[ln] == FM_SEQ_EUCLID_EMPTY) {
        seq_materialize_all();
        for (st = 0; st < DUB_SEQ_STEPS; st++)
            g.cfg.seq[st].gate &= (uint8_t)~bit;
        mark_dirty();
        g.ui_dirty = 1u;
        return;
    }
    if (g.euclid_hits[ln] == FM_SEQ_EUCLID_EDITED)
        return;
    if (g.euclid_hits[ln] == FM_SEQ_EUCLID_SONG) {
        if (!g.cfg.user_song)
            dub_lane_restore_plate(ln);
        return;
    }
    seq_materialize_all();
    {
        uint16_t mask = fm_seq_euclid16(g.euclid_hits[ln], g.euclid_rot[ln]);
        for (st = 0; st < DUB_SEQ_STEPS; st++) {
            dub_step_t *s = &g.cfg.seq[st];
            if ((mask >> (st & 15u)) & 1u)
                s->gate |= bit;
            else
                s->gate &= (uint8_t)~bit;
        }
    }
    mark_dirty();
    g.ui_dirty = 1u;
}

static void seq_rotate_lane(uint8_t ln, int32_t dir)
{
    uint8_t st;
    uint32_t base = seq_view_base();
    uint8_t bit = (uint8_t)(1u << (ln % DUB_SEQ_LANES));
    uint16_t mask_before;
    uint16_t mask_after;
    uint8_t vel_before[FM_SEQ_PAGE_STEPS];
    uint8_t prob_before[FM_SEQ_PAGE_STEPS];
    int8_t micro_before[FM_SEQ_PAGE_STEPS];
    int8_t tune_before[FM_SEQ_PAGE_STEPS];

    if (dir == 0)
        return;
    ln %= DUB_SEQ_LANES;
    seq_materialize_all();
    {
        dub_step_t *ps = &g.cfg.seq[base];

        mask_before = 0;
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            if (ps[st].gate & bit)
                mask_before |= (uint16_t)(1u << st);
        }
        memcpy(vel_before, g.step_vel[ln] + base, sizeof vel_before);
        memcpy(prob_before, g.step_prob[ln] + base, sizeof prob_before);
        memcpy(micro_before, g.step_micro[ln] + base, sizeof micro_before);
        memcpy(tune_before, g.step_tune[ln] + base, sizeof tune_before);
        mask_after = fm_seq_rot16_step(mask_before, dir);
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            if ((mask_after >> st) & 1u)
                ps[st].gate |= bit;
            else
                ps[st].gate &= (uint8_t)~bit;
        }
    }
    fm_seq_rot_u8_16(g.step_vel[ln] + base, dir);
    fm_seq_rot_u8_16(g.step_prob[ln] + base, dir);
    fm_seq_rot_i8_16(g.step_micro[ln] + base, dir);
    fm_seq_rot_i8_16(g.step_tune[ln] + base, dir);
    if (mask_before != mask_after || memcmp(vel_before, g.step_vel[ln] + base, sizeof vel_before) ||
        memcmp(prob_before, g.step_prob[ln] + base, sizeof prob_before) ||
        memcmp(micro_before, g.step_micro[ln] + base, sizeof micro_before) ||
        memcmp(tune_before, g.step_tune[ln] + base, sizeof tune_before))
        seq_lane_pattern_mark_edited(ln);
    mark_dirty();
    g.ui_dirty = 1u;
}

static void hp_lead_arm_siren_plock(uint8_t mst, int8_t tune3)
{
#ifdef GENRE_HEAVY_PSYCH
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    g.synth.siren_hp_bend_semi = bend > 0u ?
        heavy_psych_lead_bend_delta(g.cfg.p.plate, mst, tune3, g.step_tune[3]) : 0;
    {
        uint8_t plate = g.cfg.p.plate;
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = genre_lead_len_ix(plate, mst);

        if (raw != 0)
            len_ix = heavy_psych_lead_len_ix(raw);
        g.synth.siren_hp_len_on = (genre_lead_step_bit(plate, mst) || raw != 0) ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#elif defined(GENRE_JAZZ) || defined(GENRE_SOUL)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
#if defined(GENRE_JAZZ)
    g.synth.siren_hp_bend_semi = bend > 0u ? jazz_lead_bend_semi(bend) : 0u;
#elif defined(GENRE_SOUL)
    g.synth.siren_hp_bend_semi = bend > 0u ? soul_lead_bend_semi(bend) : 0u;
#endif
    {
        uint8_t plate = g.cfg.p.plate;
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = genre_lead_len_ix(plate, mst);

        if (raw != 0) {
#if defined(GENRE_JAZZ)
            len_ix = jazz_lead_len_ix(raw);
#elif defined(GENRE_SOUL)
            len_ix = soul_lead_len_ix(raw);
#endif
        }
        g.synth.siren_hp_len_on = (genre_lead_step_bit(plate, mst) || raw != 0) ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    g.synth.siren_hp_bend_semi = bend > 0u ? idm_lead_bend_semi(bend) : 0u;
    {
        uint8_t plate = g.cfg.p.plate;
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = genre_lead_len_ix(plate, mst);

        if (raw != 0)
            len_ix = idm_lead_len_ix(raw);
        g.synth.siren_hp_len_on = (genre_lead_step_bit(plate, mst) || raw != 0) ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#elif defined(GENRE_TECHNO)
    {
        int8_t raw = g.step_micro[3][mst];

        g.synth.siren_hp_len_on = raw != 0 ? 1u : 0u;
        g.synth.siren_hp_len_ix = idm_lead_len_ix(raw);
    }
    g.synth.siren_hp_spp = g.samples_per_step;
    (void)tune3;
#elif defined(GENRE_HIPHOP)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    g.synth.siren_hp_bend_semi = bend > 0u ? hiphop_lead_bend_semi(bend) : 0u;
    {
        uint8_t plate = g.cfg.p.plate;
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = genre_lead_len_ix(plate, mst);

        if (raw != 0)
            len_ix = hiphop_lead_len_ix(raw);
        g.synth.siren_hp_len_on = (genre_lead_step_bit(plate, mst) || raw != 0) ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#elif defined(GENRE_FUNK)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    if (bend > 0u) {
        int8_t d = funk_stab_bend_delta(mst, tune3, g.step_tune[3], g.cfg.seq);

        g.synth.siren_hp_bend_semi = d != 0 ? d : (int8_t)funk_stab_bend_semi_u8(bend);
    } else
        g.synth.siren_hp_bend_semi = 0;
    {
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = funk_stab_gate_len_ix(raw);

        g.synth.siren_hp_len_on = raw != 0 ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#elif defined(GENRE_HOUSE)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    if (bend > 0u) {
        int8_t d = house_stab_bend_delta(mst, tune3, g.step_tune[3], g.cfg.seq);

        g.synth.siren_hp_bend_semi = d != 0 ? d : (int8_t)house_stab_bend_semi_u8(bend);
    } else
        g.synth.siren_hp_bend_semi = 0;
    {
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = house_stab_gate_len_ix(raw);

        g.synth.siren_hp_len_on = raw != 0 ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#elif defined(GENRE_POP)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    g.synth.siren_hp_bend_semi = 0;
    g.synth.siren_hp_spp = g.samples_per_step;
    {
        uint8_t lix = pop_lead_len_ix(g.step_micro[3][mst]);

        g.synth.siren_hp_len_on = lix != 0u ? 1u : 0u;
        g.synth.siren_hp_len_ix = lix;
        if (lix != 0u) {
            /* Locked length replaces the gap to the next note; cap keeps the u16 hold from wrapping. */
            uint32_t sp = g.synth.siren_hp_spp < 220u ? DUB_FS / 8u : g.synth.siren_hp_spp;
            uint32_t span = POP_LEAD_LEN_16THS[lix];
            uint32_t cap = 79000u / sp;

            if (span > cap)
                span = cap > 0u ? cap : 1u;
            pop_lead_arm_trig_gate(&g.synth, &g.voice.p, g.cfg.p.plate, span, g.voice.lane_gate[3]);
        }
    }
    (void)tune3;
#elif defined(FM_DUB_SRN_PLOCK)
    uint8_t bend = g.step_vel[3][mst];

    g.synth.siren_hp_bend = bend;
    if (bend > 0u) {
        int8_t d = dub_siren_bend_delta(mst, tune3, g.step_tune[3], g.cfg.seq);

        g.synth.siren_hp_bend_semi = d != 0 ? d : (int8_t)dub_siren_bend_semi_u8(bend);
    } else
        g.synth.siren_hp_bend_semi = 0;
    {
        int8_t raw = g.step_micro[3][mst];
        uint8_t len_ix = dub_siren_gate_len_ix(raw);

        g.synth.siren_hp_len_on = raw != 0 ? 1u : 0u;
        g.synth.siren_hp_len_ix = len_ix;
    }
    g.synth.siren_hp_spp = g.samples_per_step;
#else
    (void)mst;
    (void)tune3;
#endif
}

static uint8_t seq_lane_velocity(uint8_t ln, uint8_t st, uint8_t base_accent, uint8_t ghost_lt)
{
    int32_t vel;

    if (ghost_lt > 0)
        vel = 48 + (int32_t)ghost_lt;
    else {
#if defined(GENRE_HEAVY_PSYCH)
        if (ln == 3u) {
            vel = (int32_t)heavy_psych_lead_step_vel(g.cfg.p.plate, st);
            vel += fm_seq_rand_vel(dub_eff_vel_rng(ln), st, g.bar, ln, g.seq_rng);
        } else
#elif defined(GENRE_HOUSE) || defined(FM_DUB_SRN_PLOCK)
        if (ln == 3u) {
            vel = base_accent ? (int32_t)base_accent : 90;
            vel += fm_seq_rand_vel(dub_eff_vel_rng(ln), st, g.bar, ln, g.seq_rng);
        } else
#elif defined(GENRE_JAZZ) || defined(GENRE_SOUL)
        if (ln == 3u && g.step_vel[3][st])
            vel = (int32_t)g.step_vel[3][st];
        else
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        if (ln == 3u && g.step_vel[3][st])
            vel = (int32_t)g.step_vel[3][st];
        else
#endif
        {
            vel = g.step_vel[ln][st] ? (int32_t)g.step_vel[ln][st] :
                  (base_accent ? (int32_t)base_accent : 90);
            vel += fm_seq_rand_vel(dub_eff_vel_rng(ln), st, g.bar, ln, g.seq_rng);
        }
    }
    if (vel < 1)
        vel = 1;
    if (vel > 127)
        vel = 127;
    return (uint8_t)vel;
}

static int8_t lane_step_tune(uint8_t ln, uint8_t st)
{
    int32_t base = (int32_t)g.step_tune[ln][st];

#if defined(GENRE_HEAVY_PSYCH)
    if (ln == 3u) {
        uint8_t plate = g.cfg.p.plate;
        uint32_t grid_st = (uint32_t)(g.playing ? g.step : st);

        if (fm_seq_view_of_step(grid_st) >= 1u && fm_view_step_bit(plate, 3u, grid_st))
            base = (int32_t)genre_lead_pitch(plate, st);
        else if (base == 0 && genre_lead_step_bit(plate, st))
            base = (int32_t)genre_lead_pitch(plate, st);
    }
#elif defined(GENRE_JAZZ) || defined(GENRE_SOUL)
    if (ln == 3u) {
        uint8_t plate = g.cfg.p.plate;
        uint32_t grid_st = (uint32_t)(g.playing ? g.step : st);

        if (fm_seq_view_of_step(grid_st) >= 1u && fm_view_step_bit(plate, 3u, grid_st))
            base = (int32_t)genre_lead_pitch(plate, st);
        else if (base == 0 && genre_lead_step_bit(plate, st))
            base = (int32_t)genre_lead_pitch(plate, st);
    }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP)
    if (ln == 3u) {
        uint8_t plate = g.cfg.p.plate;
        uint32_t grid_st = (uint32_t)(g.playing ? g.step : st);

        if (fm_seq_view_of_step(grid_st) >= 1u && fm_view_step_bit(plate, 3u, grid_st))
            base = (int32_t)genre_lead_pitch(plate, st);
        else if (base == 0 && genre_lead_step_bit(plate, st))
            base = (int32_t)genre_lead_pitch(plate, st);
    }
#endif
    int32_t t = base + (int32_t)fm_seq_rand_pitch(dub_eff_pitch_rng(ln), st, g.bar, ln, g.seq_rng);

    if (t < -127)
        t = -127;
    if (t > 127)
        t = 127;
    return (int8_t)t;
}

static int8_t lane_step_gate_rnd(uint8_t ln, uint8_t st)
{
    return fm_seq_rand_gate(dub_eff_gate_rng(ln), st, g.bar, ln, g.seq_rng);
}

static void dub_preview_lane(uint8_t ln, uint8_t st)
{
    uint8_t vel = seq_lane_velocity(ln, st, 90u, 0u);
    int8_t tune = lane_step_tune(ln, st);
    int8_t gate_rnd = lane_step_gate_rnd(ln, st);

    if (ln == 0u) {
        g.voice.lane_gate[0] = gate_rnd;
        dub_trig_kick(&g.synth, vel, tune, g.voice.p.drm_atk[0]);
    } else if (ln == 1u) {
        g.voice.lane_gate[1] = gate_rnd;
        dub_trig_rim(&g.synth, vel, tune, g.voice.p.drm_atk[1]);
    } else if (ln == 2u) {
        g.voice.lane_gate[2] = gate_rnd;
        dub_trig_hat(&g.synth, vel, tune, g.voice.p.drm_atk[2]);
    } else if (ln == 3u) {
        g.voice.lane_gate[3] = gate_rnd;
        hp_lead_arm_siren_plock(st, tune);
        dub_siren_trig(&g.synth, vel, tune, gate_rnd, g.voice.p.drm_atk[3], g.voice.p.era);
    } else if (ln == 4u) {
        int32_t nn = (int32_t)g.cfg.p.root + (int32_t)tune;

        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        g.voice.lane_gate[4] = gate_rnd;
        dub_trig_sub(&g.synth, (uint8_t)nn, vel, g.voice.p.atk_b);
    } else if (ln == 5u) {
        int32_t nn = (int32_t)g.cfg.p.root + (int32_t)tune;

        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        g.voice.lane_gate[5] = gate_rnd;
        dub_trig_chord(&g.synth, (uint8_t)nn, vel, g.voice.p.atk_c, g.voice.p.glide_c, g.voice.p.voicing);
    }
}

static void dub_rec_lane_hit(uint8_t ln, int32_t tune_note)
{
    uint8_t st;

    if (!g.rec_arm || !g.playing)
        return;
    st = g.step;
    seq_materialize_all();
    g.cfg.seq[st].gate |= (uint8_t)(1u << ln);
    g.cfg.seq[st].force = 1u;
    if (tune_note >= 0 && (ln == 4u || ln == 5u))
        g.step_tune[ln][st] = (int8_t)(tune_note - (int32_t)g.cfg.p.root);
    if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG)
        seq_lane_wipe_plocks(ln);
    g.euclid_hits[ln] = FM_SEQ_EUCLID_SONG;
    dub_loop_snap_sync_step(st);
    mark_dirty();
}

#ifdef FM_GENRE_STEP_MICRO
static int32_t seq_timing_to_samples(int32_t units)
{
    uint32_t sps = g.samples_per_step ? g.samples_per_step : 1u;

    return (units * (int32_t)sps) / FM_SEQ_TIMING_UNITS;
}

static void seq_arm_lane_timing(uint8_t ln, uint8_t mst, int32_t units, uint8_t vel, int32_t note)
{
    int32_t smp = seq_timing_to_samples(units);
    uint32_t sps = g.samples_per_step ? g.samples_per_step : 1u;

    if (smp < 0) {
        if ((uint32_t)(-smp) >= sps)
            smp = -(int32_t)(sps > 0u ? sps - 1u : 0u);
        g.micro_left[ln] = (uint16_t)((uint32_t)sps + (uint32_t)(-smp));
    } else if (smp > 0) {
        if ((uint32_t)smp >= sps)
            smp = (int32_t)(sps > 0u ? sps - 1u : 0);
        g.micro_left[ln] = (uint16_t)smp;
    } else
        g.micro_left[ln] = 0u;
    g.micro_vel[ln] = vel;
    g.micro_tune[ln] = lane_step_tune(ln, mst);
    g.micro_mst[ln] = mst;
    if (ln < 3u)
        g.micro_atk[ln] = g.voice.p.drm_atk[ln];
    else if (ln == 4u) {
        int32_t nn = note + (int32_t)g.micro_tune[ln];

        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        g.micro_note[4] = (uint8_t)nn;
        g.micro_atk[4] = g.voice.p.atk_b;
    } else if (ln == 5u) {
        int32_t nn = note + (int32_t)g.micro_tune[ln];

        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        g.micro_note[5] = (uint8_t)nn;
        g.micro_atk[5] = g.voice.p.atk_c;
    }
    g.micro_armed = (uint8_t)(g.micro_armed | (uint8_t)(1u << ln));
}

static void snowcrash_micro_fire_lane(uint8_t ln)
{
    uint8_t dm = g.voice.mute;
    uint8_t bit = (uint8_t)(1u << (ln % 6u));

    g.micro_armed = (uint8_t)(g.micro_armed & (uint8_t)~bit);
    if (ln >= 6u)
        return;
    sync_voice_params();
    switch (ln) {
    case 0u:
        if (dub_lane_heard_audible(0u) && !(dm & 4u)) {
            g.voice.lane_gate[0] = lane_step_gate_rnd(0u, g.micro_mst[0]);
            dub_trig_kick(&g.synth, g.micro_vel[0], g.micro_tune[0], g.micro_atk[0]);
        }
        break;
    case 1u:
        if (dub_lane_heard_audible(1u) && !(dm & 4u)) {
            g.voice.lane_gate[1] = lane_step_gate_rnd(1u, g.micro_mst[1]);
            dub_trig_rim(&g.synth, g.micro_vel[1], g.micro_tune[1], g.micro_atk[1]);
        }
        break;
    case 2u:
        if (dub_lane_heard_audible(2u) && !(dm & 4u)) {
            g.voice.lane_gate[2] = lane_step_gate_rnd(2u, g.micro_mst[2]);
            dub_trig_hat(&g.synth, g.micro_vel[2], g.micro_tune[2], g.micro_atk[2]);
        }
        break;
    case 4u:
        if (dub_lane_heard_audible(4u) && !(dm & 1u)) {
            g.voice.lane_gate[4] = lane_step_gate_rnd(4u, g.micro_mst[4]);
            dub_trig_sub(&g.synth, g.micro_note[4], g.micro_vel[4], g.micro_atk[4]);
        }
        break;
    case 5u:
        if (dub_lane_heard_audible(5u) && !(dm & 2u)) {
            g.voice.lane_gate[5] = lane_step_gate_rnd(5u, g.micro_mst[5]);
            dub_trig_chord(&g.synth, g.micro_note[5], g.micro_vel[5], g.micro_atk[5],
                           g.voice.p.glide_c, g.voice.p.voicing);
        }
        break;
    default:
        break;
    }
}

static void snowcrash_micro_tick(void)
{
    uint8_t ln;

    if (!g.playing) {
        if (g.micro_armed)
            g.micro_armed = 0u;
        return;
    }
    if (!g.micro_armed)
        return;
    for (ln = 0; ln < 6u; ln++) {
        uint8_t bit = (uint8_t)(1u << ln);

        if (!(g.micro_armed & bit))
            continue;
        if (g.micro_left[ln] > 0u) {
            g.micro_left[ln]--;
            continue;
        }
        snowcrash_micro_fire_lane(ln);
    }
}

#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
static void snowcrash_seed_plate_groove_micro(void)
{
}
#elif defined(GENRE_JAZZ)
static void jazz_seed_plate_groove_micro(void)
{
    const dub_plate_t *pl = genre_preset(genre_clamp_preset(g.cfg.p.plate));
    uint8_t era = genre_era_of_preset(g.cfg.p.plate);
    uint8_t st;

    era = genre_clamp_era(era);
    for (st = 0; st < DUB_SEQ_STEPS; st++) {
        int8_t micro = 0;

        if (!dub_step_bit(pl->sub, st))
            continue;
        if (era <= (uint8_t)ERA_MUTE) {
            if (st & 1u)
                micro = (int8_t)(12 + (st & 3u) * 2);
            else if ((st & 3u) == 2u)
                micro = 8;
        } else if (era == (uint8_t)ERA_FUSION) {
            if (st & 1u)
                micro = (int8_t)(5 + (st & 3u));
        } else if (era == (uint8_t)ERA_ACID) {
            if ((st & 7u) == 3u || (st & 7u) == 7u)
                micro = 6;
        } else if (era == (uint8_t)ERA_BROKEN) {
            if ((st & 3u) == 1u)
                micro = 11;
            else if ((st & 3u) == 2u)
                micro = 14;
            else if ((st & 7u) == 5u)
                micro = 9;
        }
        if (micro > 0 && !fm_seq_trig_pass(dub_eff_trig_raw(4u), st, 0u, 4u))
            micro = 0;
        g.step_micro[4][st] = micro;
    }
}
#elif defined(GENRE_SOUL)
static void soul_seed_plate_groove_micro(void)
{
    const dub_plate_t *pl = genre_preset(genre_clamp_preset(g.cfg.p.plate));
    uint8_t era = genre_clamp_era(genre_era_of_preset(g.cfg.p.plate));
    uint8_t st;

    for (st = 0; st < DUB_SEQ_STEPS; st++) {
        int8_t micro = 0;

        if (!dub_step_bit(pl->sub, st))
            continue;
        if (era == (uint8_t)ERA_CHURCH || era == (uint8_t)ERA_STREET) {
            if (st & 1u)
                micro = (int8_t)(8 + (st & 3u) * 2);
        } else if (era == (uint8_t)ERA_MEMPHIS) {
            if ((st & 3u) == 2u || (st & 3u) == 3u)
                micro = 6;
        } else if (era == (uint8_t)ERA_PHILLY) {
            if ((st & 7u) == 3u || (st & 7u) == 7u)
                micro = 4;
        }
        if (micro > 0 && !fm_seq_trig_pass(dub_eff_trig_raw(4u), st, 0u, 4u))
            micro = 0;
        g.step_micro[4][st] = micro;
    }
}
#endif
#endif

static fm_dj_gest_st_t dub_dj_gest_snap(void)
{
    fm_dj_gest_st_t s;

    s.kind = g.dj_gest_kind;
    s.bars = g.dj_gest_bars;
    s.flam_steps = g.dj_gest_flam_steps;
    s.ov = g.dj_gest_ov;
    s.saved_arp = g.dj_gest_saved_arp;
    s.saved_loop = g.dj_gest_saved_loop;
    return s;
}

static void dub_dj_gest_apply_snap(const fm_dj_gest_st_t *s)
{
    g.dj_gest_kind = s->kind;
    g.dj_gest_bars = s->bars;
    g.dj_gest_flam_steps = s->flam_steps;
    g.dj_gest_ov = s->ov;
    g.dj_gest_saved_arp = s->saved_arp;
    g.dj_gest_saved_loop = s->saved_loop;
}

static void dub_dj_gest_reset(void)
{
    fm_dj_gest_st_t s = dub_dj_gest_snap();

    fm_dj_gest_st_reset(&s);
    dub_dj_gest_apply_snap(&s);
    g.dj_gest_saved_latch = 0u;
    g.dj_gest_saved_gesture = 0u;
}

static uint8_t dub_dj_gest_active(void)
{
    fm_dj_gest_st_t s = dub_dj_gest_snap();

    return fm_dj_gest_st_active(&s);
}

static void dub_dj_gest_restore(void)
{
    fm_dj_gest_st_t s = dub_dj_gest_snap();
    uint8_t arp = g.cfg.p.arp_mode;
    uint8_t loop_end = 0u;
    uint8_t was_loop_ov = (s.ov == FM_DJ_GEST_OV_LOOP);
    uint8_t latch = g.dj_gest_saved_latch;
    uint8_t gesture = g.dj_gest_saved_gesture;
    uint8_t kind = g.dj_gest_kind;

    if (!fm_dj_gest_st_active(&s))
        return;
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    if (kind == GEST_FILL && house_fill_build_live())
        house_fill_build_restore();
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    if ((kind == GEST_FILL || kind == GEST_BREAK) && idm_stutter_edit_live())
        idm_stutter_edit_restore();
#else
    (void)kind;
#endif
    if (!fm_dj_gest_st_restore(&s, &arp, g.loop_active, &loop_end))
        return;
    dub_dj_gest_apply_snap(&s);
    g.dj_gest_saved_latch = 0u;
    g.dj_gest_saved_gesture = 0u;
    if (was_loop_ov) {
        uint8_t loop_end_q = 0u;

        fm_dj_gest_loop_overlay_teardown(g.loop_active, &loop_end_q, &g.loop_pending);
        if (loop_end_q)
            g.loop_end_pending = 1u;
    } else if (loop_end)
        g.loop_end_pending = 1u;
    dub_arp_set_mode(arp);
    g.gesture_latch = latch;
    g.gesture = gesture;
    g.ui_dirty = 1u;
}

static uint8_t dub_dj_gest_from_plan(uint8_t plan)
{
    switch (plan) {
    case FM_DJ_GEST_SOLO_BASS:
        return GEST_SOLO_BASS;
    case FM_DJ_GEST_SOLO_DRUM:
        return GEST_SOLO_DRUM;
    case FM_DJ_GEST_FLAM:
        return GEST_FLAM;
    case FM_DJ_GEST_BREAK_MICRO:
        return GEST_BREAK;
    case FM_DJ_GEST_FILL:
        return GEST_FILL;
    default:
        return GEST_NONE;
    }
}

static void dub_dj_gest_arm(uint8_t plan)
{
    uint8_t kind;
    uint8_t rng;

    if (plan == FM_DJ_GEST_NONE)
        return;
    if (dub_dj_gest_active())
        dub_dj_gest_restore();
    if (plan == FM_DJ_GEST_LOOP && (g.loop_active || fm_dj_pre_drop_silence(&g.dj, 1u, g.bar_step, DUB_STEPS)))
        return;
    if (plan == FM_DJ_GEST_ARP && g.cfg.p.arp_mode != 0u)
        return;
    g.dj_gest_saved_latch = g.gesture_latch;
    g.dj_gest_saved_gesture = g.gesture;
    g.dj_gest_saved_arp = g.cfg.p.arp_mode;
    g.dj_gest_saved_loop = g.loop_active;
    g.dj_gest_ov = FM_DJ_GEST_OV_NONE;
    rng = (uint8_t)fm_dj_rng_u8(&g.dj);
    if (plan == FM_DJ_GEST_FLAM) {
        g.dj_gest_flam_steps = (uint8_t)(2u + (rng & 3u));
        g.dj_gest_kind = GEST_FLAM;
        g.gesture = GEST_FLAM;
        g.dj_gest_flam_cd = FM_DJ_GEST_FLAM_CD_BARS;
        g.ui_dirty = 1u;
        return;
    }
    if (plan == FM_DJ_GEST_BREAK_MICRO) {
        g.dj_gest_kind = GEST_BREAK;
        g.gesture = GEST_BREAK;
        g.dj_gest_bars = fm_dj_gest_plan_bars(plan, rng);
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        if (g.view == VIEW_HOME && g.playing)
            idm_stutter_start(GEST_BREAK);
#endif
        g.ui_dirty = 1u;
        return;
    }
    if (plan == FM_DJ_GEST_LOOP) {
        g.dj_gest_kind = GEST_NONE;
        g.dj_gest_ov = FM_DJ_GEST_OV_LOOP;
        g.dj_gest_bars = fm_dj_gest_plan_bars(plan, rng);
        g.loop_end_pending = 0u;
        g.loop_pending = 1u;
        g.loop_pending_start = (uint8_t)(g.slip_step & 14u);
        g.loop_pending_len = 8u;
        g.dj_gest_loop_cd = FM_DJ_GEST_LOOP_CD_BARS;
        if (g.dj_gest_arp_cd < FM_DJ_GEST_ARP_CD_BARS)
            g.dj_gest_arp_cd = FM_DJ_GEST_ARP_CD_BARS;
        g.ui_dirty = 1u;
        return;
    }
    if (plan == FM_DJ_GEST_ARP) {
        uint8_t mode = fm_dj_gest_plan_arp_mode(rng);

        g.dj_gest_kind = GEST_NONE;
        g.dj_gest_ov = FM_DJ_GEST_OV_ARP;
        g.dj_gest_bars = fm_dj_gest_plan_bars(plan, rng);
        g.dj_gest_arp_cd = FM_DJ_GEST_ARP_CD_BARS;
        if (g.dj_gest_loop_cd < 16u)
            g.dj_gest_loop_cd = 16u;
        dub_arp_set_mode(mode);
        g.ui_dirty = 1u;
        return;
    }
    kind = dub_dj_gest_from_plan(plan);
    if (kind == GEST_NONE)
        return;
    g.dj_gest_kind = kind;
    g.dj_gest_bars = fm_dj_gest_plan_bars(plan, rng);
    g.gesture_latch = kind;
    gesture_solo_sync(kind);
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    if (kind == GEST_FILL && g.view == VIEW_HOME && g.playing)
        house_fill_build_start(GEST_FILL);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    if ((kind == GEST_FILL || kind == GEST_BREAK) && g.view == VIEW_HOME && g.playing)
        idm_stutter_start(kind);
#endif
    g.ui_dirty = 1u;
}

static void dub_dj_gest_on_bar(void)
{
    fm_dj_gest_env_t env;
    uint8_t plan;

    if (!g.dj.on) {
        if (dub_dj_gest_active())
            dub_dj_gest_restore();
        return;
    }
    if (dub_dj_gest_active() && fm_dj_gest_st_abort(&g.dj))
        dub_dj_gest_restore();
    if (g.dj_gest_bars > 0u) {
        g.dj_gest_bars--;
        if (g.dj_gest_bars == 0u)
            dub_dj_gest_restore();
    }
    if (g.dj_gest_flam_cd > 0u)
        g.dj_gest_flam_cd--;
    if (g.dj_gest_loop_cd > 0u)
        g.dj_gest_loop_cd--;
    if (g.dj_gest_arp_cd > 0u)
        g.dj_gest_arp_cd--;
    if (dub_dj_gest_active())
        return;
    env.caps = fm_dj_gest_caps_default();
    env.manual_gesture_latch = (g.dj_gest_kind == GEST_NONE && g.dj_gest_flam_steps == 0u && g.dj_gest_bars == 0u)
                                   ? g.gesture_latch
                                   : 0u;
    env.manual_arp_mode = g.cfg.p.arp_mode;
    env.loop_active = g.loop_active;
    env.loop_cd_bars = g.dj_gest_loop_cd;
    env.arp_cd_bars = g.dj_gest_arp_cd;
    env.gest_active = 0u;
    env.fill_build_live = dj_fill_live() ? 1u : 0u;
    env.playing = g.playing ? 1u : 0u;
    env.flam_cd_bars = g.dj_gest_flam_cd;
    plan = fm_dj_gest_plan_pick(&g.dj, g.dj.phrase_bar, &env);
    dub_dj_gest_arm(plan);
}

static void dub_dj_gest_seq_step(void)
{
    if (g.dj_gest_flam_steps > 0u) {
        g.dj_gest_flam_steps--;
        if (g.dj_gest_flam_steps == 0u)
            dub_dj_gest_restore();
    }
}

static uint8_t gesture_active(void)
{
    if (g.dj_gest_flam_steps > 0u || g.dj_gest_kind != GEST_NONE)
        return g.dj_gest_kind != GEST_NONE ? g.dj_gest_kind : GEST_FLAM;
    return g.gesture_latch ? g.gesture_latch : g.gesture;
}

static uint32_t gesture_solo_hash(uint8_t bar, uint8_t step, uint8_t salt, uint8_t mbar)
{
    uint32_t h = (uint32_t)bar * 131u + (uint32_t)step * 17u + (uint32_t)salt * 911u + (uint32_t)mbar * 73u +
                 0x9e3779b9u;

    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

static void gesture_solo_sync(uint8_t act_g)
{
    if (act_g != GEST_SOLO_BASS && act_g != GEST_SOLO_DRUM) {
        if (g.gesture_solo_kind == GEST_SOLO_DRUM) {
            g.kick_slam_bars = 2u;
            g.drop_emph_span = 2u;
        }
        g.gesture_solo_kind = GEST_NONE;
        g.gesture_solo_bar = 0u;
        return;
    }
    if (g.gesture_solo_kind != act_g) {
        g.gesture_solo_kind = act_g;
        g.gesture_solo_bar = 0u;
    }
}

static int8_t gesture_bass_solo_gate(uint8_t sb, uint8_t solo_bars, uint8_t bar, uint8_t st)
{
    int8_t gval;
    uint32_t h;
    uint8_t gentle = solo_bars == 0u;

    if ((sb & 3u) == 0u) {
        gval = gentle ? 6 : 10;
    } else if ((sb & 1u) == 0u) {
        gval = gentle ? 2 : 4;
    } else {
        gval = gentle ? -3 : -6;
        h = gesture_solo_hash(bar, sb, 41u, (uint8_t)(st >> 4));
        if ((h & 7u) == 0u)
            gval = gentle ? -6 : -10;
    }
    return gval;
}

static uint8_t gesture_bass_solo_note(int32_t base, uint8_t sb, uint8_t st, uint8_t solo_bars, uint8_t bar)
{
    uint32_t h;
    uint8_t pick;
    int32_t nn;
    int8_t off;
    uint8_t mbar = (uint8_t)(st >> 4);

    if (sb == 0u || sb == 16u)
        return dub_clamp_midi(base);
    h = gesture_solo_hash(bar, sb, 19u, mbar);
    pick = (uint8_t)(h % 100u);
    if (solo_bars == 0u) {
        if (pick < 50u)
            off = 0;
        else if (pick < 75u)
            off = 12;
        else
            off = -12;
    } else if (solo_bars == 1u) {
        if (pick < 30u)
            off = 0;
        else if (pick < 50u)
            off = 12;
        else if (pick < 65u)
            off = -12;
        else if (pick < 85u)
            off = 7;
        else
            off = -5;
    } else {
        if (pick < 25u)
            off = 0;
        else if (pick < 40u)
            off = 12;
        else if (pick < 55u)
            off = -12;
        else if (pick < 75u)
            off = 7;
        else if (pick < 90u)
            off = -5;
        else
            off = 19;
    }
    nn = base + (int32_t)off;
    if (nn < 24 && off == -12) {
        if (base + 12 <= 127)
            nn = base + 12;
        else
            nn = base;
    }
    return dub_clamp_midi(nn);
}

static uint8_t gesture_drum_lane_density(uint8_t ln, uint8_t sb)
{
    uint32_t rise = (uint32_t)sb * 45u;
    uint32_t wave = (uint32_t)((sb + ln * 11u) & 31u);
    int32_t dens;

    dens = 35 + (int32_t)(rise / 31u) + (int32_t)wave - 16;
    dens += (int32_t)ln * 6;
    if (sb >= 28u && dens < 88)
        dens = 88;
    if (dens < 12)
        dens = 12;
    if (dens > 96)
        dens = 96;
    return (uint8_t)dens;
}

static void gesture_drum_solo_mod(uint8_t *on, uint8_t *vel, uint8_t st, uint8_t sb, uint8_t sb_ks, uint8_t bar)
{
    uint8_t ln;
    uint8_t mhalf = (uint8_t)(st >> 4);

    for (ln = 0; ln < 3u; ln++) {
        uint8_t dens = gesture_drum_lane_density(ln, sb);
        uint8_t roll = fm_seq_step_chance(st, ln, 211u + (uint32_t)bar * 41u + (uint32_t)mhalf * 17u);

        if (on[ln]) {
            if (ln == 0u && (sb_ks & 3u) == 0u)
                continue;
            if ((sb_ks & 3u) != 0u) {
                uint8_t thr = dens;

                if (ln == 0u && thr > 14u)
                    thr = (uint8_t)(thr - 14u);
                if (roll > thr)
                    on[ln] = 0u;
            }
        } else if (dens > 72u && (sb & 1u) != 0u) {
            uint8_t ghost = fm_seq_step_chance(st, ln, 307u + (uint32_t)bar * 53u + (uint32_t)mhalf * 23u);

            if (ghost < (uint8_t)(dens - 58u)) {
                on[ln] = 1u;
                vel[ln] = (uint8_t)((vel[ln] > 40u ? 40u : vel[ln]) + 28u);
            }
        }
    }
}

/* Lane gate for fire_seq_step: only lanes in the mask fire. Grid call = dub_grid_lane_mask();
 * sub-lane call (dub_lane_fire_at) = one lane, and g_lane_fire_sub drops the global slip for K+S. */
#define DUB_LANE_MASK_ALL ((1u << DUB_SEQ_LANES) - 1u)
static uint8_t g_lane_fire_mask = DUB_LANE_MASK_ALL;
static uint8_t g_lane_fire_sub;

static void fire_seq_step(uint8_t st, const dub_events_t *ev, uint8_t gate)
{
#if defined(GENRE_BASS)
    int32_t note;
    uint8_t vel;
    uint8_t mst;
    uint8_t len;
    uint8_t acc_ev;
    uint8_t fill = 0u;

    (void)gate;
    if (!ev)
        return;
    len = seq_lane_len_v(0u, 4u);
    mst = bass_perf_shift_step((uint8_t)(st % DUB_SEQ_STEPS), len);
    acc_ev = ev->accent;
    if (!bass_seq_lane_step_on(mst)) {
        if (mst >= len || !bass_perf_dens_fill(mst))
            return;
        acc_ev = 0u;
        fill = 1u;
    } else if (bass_perf_dens_drop(mst))
        return;
    if (!dub_lane_heard_audible(4u) || (g.voice.mute & 1u))
        return;
    note = g.cfg.seq[mst].note ? bass_note_abs(g.cfg.seq[mst].note, g.voice.p.root) : (int32_t)ev->note;
    note += (int32_t)g.step_tune[4u][mst];
    note += (int32_t)fm_seq_rand_pitch(dub_eff_pitch_rng(4u), mst, g.bar, 4u, g.seq_rng);
    if (note < 0)
        note = 0;
    if (note > 127)
        note = 127;
    vel = fill ? 84u : g.step_vel[4u][mst] ? g.step_vel[4u][mst] : (acc_ev ? 110u : 96u);
    {
        int32_t vv = (int32_t)vel + fm_seq_rand_vel(dub_eff_vel_rng(4u), mst, g.bar, 4u, g.seq_rng);

        if (vv < 1)
            vv = 1;
        if (vv > 127)
            vv = 127;
        vel = (uint8_t)vv;
    }
    {
        int32_t tun = seq_lane_timing_units(4u, mst);

        if (tun > 0) {
            int32_t smp = seq_timing_to_samples(tun);
            uint32_t sps = g.samples_per_step ? g.samples_per_step : 1u;

            if (smp > 0) {
                if (g.bass_rt.micro_armed) {
                    g.bass_rt.micro_left = 0u;
                    bass_micro_sample();
                }
                if ((uint32_t)smp >= sps)
                    smp = (int32_t)(sps - 1u);
                g.bass_rt.micro_left = (uint32_t)smp;
                g.bass_rt.micro_st = mst == st % DUB_SEQ_STEPS ? st : mst;
                g.bass_rt.micro_note = (uint8_t)note;
                g.bass_rt.micro_vel = vel;
                g.bass_rt.micro_accent = acc_ev;
                g.bass_rt.micro_armed = 1u;
                return;
            }
        }
    }
    bass_fire_note(mst == st % DUB_SEQ_STEPS ? st : mst, note, vel, acc_ev);
    return;
#else
    uint8_t dm = g.voice.mute;
    uint8_t ln;
    uint8_t on[DUB_SEQ_LANES];
    uint8_t vel[DUB_SEQ_LANES];
    int32_t note = ev ? (int32_t)ev->note : (int32_t)g.cfg.p.root;
    uint8_t act_g0 = gesture_active();
    uint8_t solo_bars = g.gesture_solo_bar;
    int8_t bass_solo_gate = 0;
    uint8_t bass_solo_note = 0u;
    uint8_t bass_solo_use = 0u;
    uint8_t sb = (uint8_t)(st & (DUB_STEPS - 1u));
    /* Kick+snare keep song pulse (slip); hats/lead/bass/chord follow looped st */
    uint8_t ks_st = (g.loop_active && g.loop_len > 0u && !g_lane_fire_sub) ? g.slip_step : st;
    uint8_t sb_ks = (uint8_t)(ks_st & (DUB_STEPS - 1u));
    uint8_t gate_ks = gate;
#ifdef FM_GENRE_STEP_MICRO
    uint8_t sc_defer = 0u;
#endif

    if (!ev)
        return;
    if (ks_st != st) {
        dub_events_t ev_ks;
        dub_step_t *ovr_ks = &g.cfg.seq[ks_st % DUB_SEQ_STEPS];
        const dub_step_t *use_ks = (ovr_ks->force & 1u) ? ovr_ks : 0;
        uint8_t vv_ks;

        dub_resolve_step_morphed(use_ks, ks_st, &ev_ks);
        gate_ks = events_gate(&ev_ks);
        vv_ks = fm_seq_view_of_step(ks_st);
        if (vv_ks >= 1u && !seq_view_playable(vv_ks))
            gate_ks = 0u;
    }
    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        uint8_t use_st = (ln < 2u) ? ks_st : st;
        uint8_t use_gate = (ln < 2u) ? gate_ks : gate;
        uint8_t mst = seq_lane_step_map(use_st, ln);

        if ((g.lfo_slots[DUB_LFO_SEQ_PAT].depth > 0u && g.seq_lfo_lane[0] == ln &&
             fm_seq_euclid_is_pattern(g.euclid_hits[ln])) ||
            (g.lfo_slots[DUB_LFO_SEQ_ROT].depth > 0u && g.seq_lfo_lane[1] == ln))
            on[ln] = (uint8_t)dub_lane_step_base_on(ln, use_st);
        else
            on[ln] = seq_lane_on(use_gate, ln);
        on[ln] = (uint8_t)fm_seq_vary_hit(on[ln], dub_eff_vary(ln), mst, g.bar, ln, g.seq_rng);
        if (on[ln] && g.step_prob[ln][mst] > 0u &&
            !fm_seq_cond_pass(g.step_prob[ln][mst], mst, g.bar, ln, 0))
            on[ln] = 0u;
        if (on[ln] && !fm_seq_trig_pass(dub_eff_trig_raw(ln), mst, g.bar, ln))
            on[ln] = 0u;
        if (!((g_lane_fire_mask >> ln) & 1u)) {
            on[ln] = 0u;
            vel[ln] = 0u;
            continue;
        }
        vel[ln] = seq_lane_velocity(ln, mst, ev->accent, 0u);
#ifdef FM_GENRE_STEP_MICRO
        if (ln != 3u && on[ln]) {
            int32_t tun = seq_lane_timing_units(ln, use_st);
            int32_t smp = seq_timing_to_samples(tun);

            if (smp > 0) {
                seq_arm_lane_timing(ln, mst, tun, vel[ln], note);
                sc_defer = (uint8_t)(sc_defer | (uint8_t)(1u << ln));
            } else if (tun < 0)
                sc_defer = (uint8_t)(sc_defer | (uint8_t)(1u << ln));
        }
#endif
    }
#ifdef FM_GENRE_STEP_MICRO
    if (!g_lane_fire_sub)
    {
        uint8_t ln2;

        for (ln2 = 0; ln2 < DUB_SEQ_LANES; ln2++) {
            uint8_t nxt_st = (ln2 < 2u) ? (uint8_t)(ks_st + 1u) : (uint8_t)(st + 1u);
            uint8_t nmst = seq_lane_step_map(nxt_st, ln2);
            uint8_t n_on;
            uint8_t n_vel;

            if ((g.lfo_slots[DUB_LFO_SEQ_PAT].depth > 0u && g.seq_lfo_lane[0] == ln2 &&
                 fm_seq_euclid_is_pattern(g.euclid_hits[ln2])) ||
                (g.lfo_slots[DUB_LFO_SEQ_ROT].depth > 0u && g.seq_lfo_lane[1] == ln2))
                n_on = (uint8_t)dub_lane_step_base_on(ln2, nxt_st);
            else
                n_on = seq_lane_on((ln2 < 2u) ? gate_ks : gate, ln2);
            n_on = (uint8_t)fm_seq_vary_hit(n_on, dub_eff_vary(ln2), nmst, g.bar, ln2, g.seq_rng);
            if (n_on && g.step_prob[ln2][nmst] > 0u &&
                !fm_seq_cond_pass(g.step_prob[ln2][nmst], nmst, g.bar, ln2, 0))
                n_on = 0u;
            if (n_on && !fm_seq_trig_pass(dub_eff_trig_raw(ln2), nmst, g.bar, ln2))
                n_on = 0u;
            if (ln2 == 3u || !n_on)
                continue;
            {
                uint8_t nxt_use = (ln2 < 2u) ? (uint8_t)(ks_st + 1u) : (uint8_t)(st + 1u);
                int32_t ntun = seq_lane_timing_units(ln2, nxt_use);

                if (ntun < 0) {
                    n_vel = seq_lane_velocity(ln2, nmst, ev->accent, 0u);
                    seq_arm_lane_timing(ln2, nmst, ntun, n_vel, note);
                }
            }
        }
    }
#endif

    /* Apply active RYTM-style performance gesture (KEY_BLACK[7..10]: FILL/BREAK, SOLO D, SOLO B, FLAM!)
     * Global lane order: 0=KICK, 1=RIM, 2=HAT, 3=SIREN, 4=BASS, 5=CHORD */
    {
        uint8_t act_g = act_g0;
        gesture_solo_sync(act_g);
        solo_bars = g.gesture_solo_bar;
        if (act_g == GEST_BREAK) {
            /* Same as DJ break→drop: kick out; snare keeps pulse; rest thinned */
            on[0] = 0u;
            on[2] = (sb & 3u) == 2u ? 1u : 0u;
            on[4] = (sb == 0u) ? 1u : 0u;
            on[5] = (sb == 2u) ? 1u : 0u;
        } else if (act_g == GEST_FILL) {
            if (sb_ks >= 8u) {
                on[0] = ((sb_ks & 1u) == 0u || sb_ks == 15u) ? 1u : 0u;
                on[1] = (sb_ks == 10u || sb_ks == 11u || sb_ks >= 13u) ? 1u : 0u;
                on[2] = 1u;
                vel[1] = 115u;
            }
        } else if (act_g == GEST_SOLO_DRUM) {
            on[3] = 0u;
            on[4] = 0u;
            on[5] = 0u;
            if ((sb_ks & 3u) == 3u)
                on[1] = 1u;
            if ((sb & 1u) == 1u)
                on[2] = 1u;
            gesture_drum_solo_mod(on, vel, st, sb, sb_ks, g.bar);
        } else if (act_g == GEST_SOLO_BASS) {
            on[0] = 0u;
            on[1] = 0u;
            on[3] = 0u;
            on[5] = 0u;
            if ((sb & 1u) == 0u || (sb & 7u) == 7u) {
                uint8_t mst4s = seq_lane_step_map(st, 4u);
                int8_t tune4s = lane_step_tune(4u, mst4s);
                int32_t bass_base = note + (int32_t)tune4s;

                on[4] = 1u;
                bass_solo_use = 1u;
                bass_solo_note = gesture_bass_solo_note(bass_base, sb, st, solo_bars, g.bar);
                bass_solo_gate = gesture_bass_solo_gate(sb, solo_bars, g.bar, st);
            }
        } else if (act_g == GEST_SOLO_BAND) {
            /* Jamaican Drum & Bass version solo: mute chords/siren, boost sub + drums */
            on[3] = 0u;
            on[5] = 0u;
            if ((sb & 3u) == 0u)
                on[4] = 1u;
        } else if (g.dj.on && fm_dj_break_seq_thin(&g.dj)) {
            on[0] = 0u;
            on[2] = (sb & 3u) == 2u ? 1u : 0u;
            on[4] = (sb == 0u) ? 1u : 0u;
            on[5] = (sb == 2u) ? 1u : 0u;
        }
    }
    if (g.dj.on && fm_dj_trans_is_drop_path(&g.dj) && fm_dj_is_break_bar(&g.dj) && g.bar_step == 0u)
        fm_dj_drop_sil_bind_audible(&g.dj, dub_drop_sil_audible_mask());
    if (g.playing)
        fm_dj_drop_sil_gate_lanes(&g.dj, 1u, g.bar_step, DUB_STEPS, on);
    if (g.dj.on) {
        uint8_t emph = fm_dj_drop_emph_level(g.kick_slam_bars, g.drop_emph_span);

        if (fm_dj_any_slot_role(&g.dj, FM_DJ_ROLE_DROP))
            emph = FM_DJ_DROP_EMPH_PEAK;
        if (emph > 0u) {
#if defined(GENRE_JAZZ)
            if (on[0])
                vel[0] = fm_lfo_clamp_u8((int32_t)vel[0] + ((127 - vel[0]) * emph) / 127, 0, 127);
            if (on[1])
                vel[1] = fm_lfo_clamp_u8((int32_t)vel[1] + ((127 - vel[1]) * emph * 4) / (127 * 5), 0,
                                       127);
            if (on[2] && emph >= 72u)
                vel[2] = fm_lfo_clamp_u8((int32_t)vel[2] + ((127 - vel[2]) * emph) / (127 * 2), 0, 127);
            if (on[3] && emph >= 88u)
                vel[3] = fm_lfo_clamp_u8((int32_t)vel[3] + (32 * emph) / 127, 0, 127);
#else
            if (on[0])
                vel[0] = fm_lfo_clamp_u8((int32_t)vel[0] + ((127 - vel[0]) * emph) / 127, 0, 127);
            if (on[1])
                vel[1] = fm_lfo_clamp_u8((int32_t)vel[1] + ((127 - vel[1]) * emph * 9) / (127 * 10), 0,
                                       127);
            if (on[2] && emph >= 96u)
                vel[2] = fm_lfo_clamp_u8((int32_t)vel[2] + (40 * emph) / 127, 0, 127);
#endif
        }
    }
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    idm_edit_apply_drums(on, vel, st);
#endif

    sync_voice_params();
    for (ln = 0u; ln < DUB_SEQ_LANES; ln++) {
        if (!((g_lane_fire_mask >> ln) & 1u))
            on[ln] = 0u;
    }
#ifdef DUB_LANE_TRACE
    fprintf(stderr, "ON %u %u %u %u %u %u %u %u\n", (unsigned)st, on[0], on[1], on[2], on[3], on[4], on[5],
            (unsigned)g.samples_per_step);
#endif
    if (on[0] && dub_lane_heard_audible(0u) && !(dm & 4u)
#ifdef FM_GENRE_STEP_MICRO
        && !(sc_defer & 1u)
#endif
    ) {
        uint8_t mst0 = seq_lane_step_map(ks_st, 0u);
        int8_t tune0 = lane_step_tune(0u, mst0);
        g.voice.lane_gate[0] = lane_step_gate_rnd(0u, mst0);
        dub_trig_kick(&g.synth, vel[0], tune0, g.voice.p.drm_atk[0]);
        if ((g.gesture_latch ? g.gesture_latch : g.gesture) == GEST_FLAM)
            dub_schedule_flam(&g.synth, 0u, (uint8_t)((vel[0] * 3u) >> 2));
    }
    if (on[1] && dub_lane_heard_audible(1u) && !(dm & 4u)
#ifdef FM_GENRE_STEP_MICRO
        && !(sc_defer & 2u)
#endif
    ) {
        uint8_t mst1 = seq_lane_step_map(ks_st, 1u);
        int8_t tune1 = lane_step_tune(1u, mst1);
        g.voice.lane_gate[1] = lane_step_gate_rnd(1u, mst1);
        dub_trig_rim(&g.synth, vel[1], tune1, g.voice.p.drm_atk[1]);
        if ((g.gesture_latch ? g.gesture_latch : g.gesture) == GEST_FLAM ||
            ((g.gesture_latch ? g.gesture_latch : g.gesture) == GEST_FILL && sb_ks >= 12u))
            dub_schedule_flam(&g.synth, 1u, (uint8_t)((vel[1] * 3u) >> 2));
    }
    if (on[2] && dub_lane_heard_audible(2u) && !(dm & 4u)
#ifdef FM_GENRE_STEP_MICRO
        && !(sc_defer & 4u)
#endif
    ) {
        uint8_t mst2 = seq_lane_step_map(st, 2u);
        int8_t tune2 = lane_step_tune(2u, mst2);
        g.voice.lane_gate[2] = lane_step_gate_rnd(2u, mst2);
        dub_trig_hat(&g.synth, vel[2], tune2, g.voice.p.drm_atk[2]);
        if ((g.gesture_latch ? g.gesture_latch : g.gesture) == GEST_FLAM)
            dub_schedule_flam(&g.synth, 2u, (uint8_t)((vel[2] * 3u) >> 2));
    }
    if (on[3] && dub_lane_heard_audible(3u) && !(dm & 8u)) {
        uint8_t mst3 = seq_lane_step_map(st, 3u);
        int8_t tune3 = lane_step_tune(3u, mst3);
        int8_t gate3 = lane_step_gate_rnd(3u, mst3);
#if defined(GENRE_POP)
        {
            uint16_t pos = pop_music_pos(g.bar, g.rail_step);
            uint8_t mbar = pop_mbar(pos);
            uint8_t s16 = pop_s16(pos);
            uint8_t lead_rest = pop_melody_step_is_rest(g.cfg.p.plate, mbar, s16);

            if (lead_rest) {
                g.synth.pop_mel_hold = -1;
            } else {
                int8_t mel = pop_lead_melody_semi(g.cfg.p.plate, g.voice.p.root, g.voice.p.scale, mbar, s16,
                                                  &g.synth.pop_mel_hold);
                uint32_t span = 1u;
                uint8_t b = mbar;
                uint8_t st = s16;

                tune3 = (int8_t)(tune3 + mel);
                while (span < 128u) {
                    st++;
                    if (st >= 16u) {
                        st = 0u;
                        b++;
                        if (b >= 16u)
                            break;
                    }
                    if (pop_melody_step_is_rest(g.cfg.p.plate, b, st))
                        break;
                    {
                        dub_events_t ev2;

                        FM_DUB_RESOLVE_STEP(&g.cfg.p, 0, st, &ev2, 1u);
                        pop_arrange_events(g.cfg.p.plate, g.voice.p.era, b, 0u, &ev2);
                        if (ev2.siren)
                            break;
                    }
                    span++;
                }
                pop_lead_arm_trig_gate(&g.synth, &g.voice.p, g.cfg.p.plate, span, gate3);
                g.voice.lane_gate[3] = gate3;
                hp_lead_arm_siren_plock(mst3, tune3);
                dub_siren_trig(&g.synth, vel[3], tune3, gate3, g.voice.p.drm_atk[3], g.voice.p.era);
            }
            g.synth.siren_on = (fm1_in.notes & (1u << KEY_BLACK[3])) ? 1u : 0u;
        }
#elif defined(GENRE_JAZZ) || defined(GENRE_SOUL)
        g.voice.lane_gate[3] = gate3;
        hp_lead_arm_siren_plock(mst3, tune3);
        {
#if defined(GENRE_JAZZ)
            uint8_t jz_era = jazz_era_of_trax(g.cfg.p.plate);

            if (jz_era == (uint8_t)ERA_SPIRIT || jz_era == (uint8_t)ERA_FUSION
                || jz_era == (uint8_t)ERA_BROKEN) {
                uint8_t span = jazz_lead_phrase_span(g.cfg.p.plate, mst3);
                uint8_t lix = g.synth.siren_hp_len_ix;

                while ((uint8_t)(lix + 1u) < (uint8_t)JAZZ_LEAD_LEN_N
                       && JAZZ_LEAD_LEN_16THS[lix] < span)
                    lix++;
                g.synth.siren_hp_len_on = 1u;
                g.synth.siren_hp_len_ix = lix;
            }
#elif defined(GENRE_SOUL)
            {
                uint8_t span = soul_lead_phrase_span(g.cfg.p.plate, mst3);
                uint8_t lix = g.synth.siren_hp_len_ix;

                while ((uint8_t)(lix + 1u) < (uint8_t)SOUL_LEAD_LEN_N
                       && SOUL_LEAD_LEN_16THS[lix] < span)
                    lix++;
                g.synth.siren_hp_len_on = 1u;
                g.synth.siren_hp_len_ix = lix;
            }
#endif
        }
        dub_siren_trig(&g.synth, vel[3], tune3, gate3, g.voice.p.drm_atk[3], g.voice.p.era);
        g.synth.siren_on = (fm1_in.notes & (1u << KEY_BLACK[3])) ? 1u : 0u;
#else
        g.voice.lane_gate[3] = gate3;
        hp_lead_arm_siren_plock(mst3, tune3);
        dub_siren_trig(&g.synth, vel[3], tune3, gate3, g.voice.p.drm_atk[3], g.voice.p.era);
        g.synth.siren_on = (fm1_in.notes & (1u << KEY_BLACK[3])) ? 1u : 0u;
#endif
    }
    if (on[4] && dub_lane_heard_audible(4u) && !(dm & 1u)
#ifdef FM_GENRE_STEP_MICRO
        && !(sc_defer & 16u)
#endif
    ) {
        uint8_t mst4 = seq_lane_step_map(st, 4u);
        int8_t tune4 = lane_step_tune(4u, mst4);
        int32_t nn = bass_solo_use ? (int32_t)bass_solo_note : note + (int32_t)tune4;
#if defined(GENRE_POP)
        if (!bass_solo_use) {
            uint16_t pos = pop_music_pos(g.bar, g.rail_step);
            uint8_t bn = pop_bass_note_midi(g.cfg.p.plate, g.voice.p.root, g.voice.p.scale, pop_mbar(pos),
                                              pop_s16(pos));

            if (bn)
                nn = (int32_t)bn + (int32_t)tune4;
        }
#endif
        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        g.voice.lane_gate[4] = bass_solo_use ? bass_solo_gate : lane_step_gate_rnd(4u, mst4);
#if defined(GENRE_BASS)
        bass_fire_note(st, nn, vel[4], ev->accent);
#else
        dub_trig_sub(&g.synth, (uint8_t)nn, vel[4], g.voice.p.atk_b);
#endif
    }
    if (on[5] && dub_lane_heard_audible(5u) && !(dm & 2u)
#ifdef FM_GENRE_STEP_MICRO
        && !(sc_defer & 32u)
#endif
    ) {
        uint8_t mst5 = seq_lane_step_map(st, 5u);
        int8_t tune5 = lane_step_tune(5u, mst5);
        int32_t nn = note + (int32_t)tune5;
#if defined(GENRE_POP)
        {
            uint16_t pos = pop_music_pos(g.bar, g.rail_step);
            uint8_t mbar = pop_mbar(pos);

            nn = (int32_t)pop_chord_root_midi(g.cfg.p.plate, mbar) + (int32_t)tune5;
            pop_chord_voices(g.cfg.p.plate, g.voice.p.root, g.voice.p.scale, mbar, g.voice.p.voicing,
                             g.synth.pop_ch_note);
        }
#endif
        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        g.voice.lane_gate[5] = lane_step_gate_rnd(5u, mst5);
        dub_trig_chord(&g.synth, (uint8_t)nn, vel[5], g.voice.p.atk_c, g.voice.p.glide_c, g.voice.p.voicing);
    }
#if defined(GENRE_POP)
    {
        uint16_t pos = pop_music_pos(g.bar, g.rail_step);
        if (pop_fill_snare_extra(pop_mbar(pos), pop_s16(pos)) && dub_lane_heard_audible(1u) && !(dm & 4u) &&
            ((g_lane_fire_mask >> 1) & 1u)) {
            uint8_t mst1 = seq_lane_step_map(ks_st, 1u);
            dub_trig_rim(&g.synth, (uint8_t)(72u + (pop_s16(pos) & 3u) * 4u), lane_step_tune(1u, mst1),
                           g.voice.p.drm_atk[1]);
        }
    }
#endif
#endif /* GENRE_BASS fire_seq_step */
}

static void arp_roll_hits(uint8_t step)
{
#if defined(GENRE_BASS)
    (void)step;
    return;
#endif
    if (g.cfg.p.arp_mode != 4u)
        return;
    if (bass_keys_held()) {
        uint8_t bn = bass_note_from_keys(step);
        dub_trig_sub(&g.synth, bn, (step & 1u) ? 78u : 104u, g.voice.p.atk_b);
        dub_rec_lane_hit(4u, (int32_t)bn);
    }
    if (chord_keys_held()) {
        dub_trig_hat(&g.synth, 72, 0, g.voice.p.drm_atk[2]);
        dub_trig_rim(&g.synth, 68, 0, g.voice.p.drm_atk[1]);
        dub_rec_lane_hit(2u, -1);
        dub_rec_lane_hit(1u, -1);
    }
}

static void arp_chord_hits(uint8_t step)
{
    uint8_t mode = g.cfg.p.arp_mode;
    uint8_t motion = dub_eff_macro(8);
    uint8_t note;
    uint8_t arp_vel;

    if (!mode || mode == 4u || mode == 5u)
        return;
    if (!arp_chord_step(mode, step))
        return;
    note = scaled_note(g.voice.p.root, g.voice.p.scale, step);
    arp_vel = (uint8_t)(56u + ((uint32_t)motion * 40u) / 127u);
    if (bass_keys_held()) {
        uint8_t bn = bass_note_from_keys(step);

        dub_trig_sub(&g.synth, bn, arp_vel, g.voice.p.atk_b);
        dub_rec_lane_hit(4u, (int32_t)bn);
    } else if (dub_lane_heard_audible(4u)) {
        int8_t tune4 = lane_step_tune(4u, step);
        int32_t nn = (int32_t)note + (int32_t)tune4;

        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        dub_trig_sub(&g.synth, (uint8_t)nn, arp_vel, g.voice.p.atk_b);
        dub_rec_lane_hit(4u, nn);
    }
    if (chord_keys_held()) {
        uint8_t cn = chord_root_from_keys(step);

        dub_trig_chord(&g.synth, cn, arp_vel, g.voice.p.atk_c, g.voice.p.glide_c, g.voice.p.voicing);
        dub_rec_lane_hit(5u, (int32_t)cn);
    } else if (dub_lane_heard_audible(5u)) {
        int8_t tune5 = lane_step_tune(5u, step);
        int32_t nn = (int32_t)note + (int32_t)tune5;

        if (nn < 0)
            nn = 0;
        if (nn > 127)
            nn = 127;
        dub_trig_chord(&g.synth, (uint8_t)nn, arp_vel, g.voice.p.atk_c, g.voice.p.glide_c, g.voice.p.voicing);
        dub_rec_lane_hit(5u, nn);
    }
}

static void rec_step(uint8_t step, const dub_events_t *ev)
{
    dub_step_t *s = &g.cfg.seq[step];
    uint8_t gate = 0;

    if (!ev)
        return;
    if (ev->kick)
        gate |= 1u;
    if (ev->rim)
        gate |= 2u;
    if (ev->hat)
        gate |= 4u;
    if (ev->siren)
        gate |= 8u;
    if (ev->sub)
        gate |= 16u;
    if (ev->chord)
        gate |= 32u;
    s->force = 1;
    s->gate = gate;
    s->note = g.cfg.p.root;
    s->dly_throw = g.voice.throw ? 127u : ev->dly_throw;
    s->rev_throw = g.voice.throw ? 127u : ev->rev_throw;
    s->accent = ev->accent;
    mark_dirty();
}

/* ---- Per-lane PACE clock ----------------------------------------------------
 * Grid lanes (1/16) fire from seq_tick as before. Other lanes fire from
 * dub_lane_clock_sample() on their own counter g.lane_st[ln] and accumulator
 * (g.lane_spp[ln] = FS*60*ticks/(bpm*24)). Loop wraps the lane counter inside the
 * loop window; Chain moves to the next playable view. REPS stay on the grid clock. */
static uint8_t dub_lane_next_step(uint8_t st)
{
    uint8_t v = (uint8_t)((st / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
    uint8_t pos = (uint8_t)(st % FM_SEQ_PAGE_STEPS);
    uint8_t nv;

    if (g.loop_active && g.loop_len > 0u) {
        uint32_t rel = ((uint32_t)st + DUB_SEQ_STEPS - g.loop_start_step) % DUB_SEQ_STEPS;

        if (rel < g.loop_len)
            return (uint8_t)((g.loop_start_step + (rel + 1u) % g.loop_len) % DUB_SEQ_STEPS);
    }
    if (!seq_view_playable(v))
        return seq_chain_at_step(st);
    if (pos + 1u < FM_SEQ_PAGE_STEPS)
        return (uint8_t)(st + 1u);
    nv = seq_view_next_playable(v, 1u);
    if (nv == 255u)
        return st;
    return (uint8_t)(nv * FM_SEQ_PAGE_STEPS);
}

/* Lane counter follows the global step after a transport reset or a pace change. */
static void dub_lane_resync(uint8_t ln)
{
    g.lane_st[ln % DUB_SEQ_LANES] = g.step;
    g.lane_acc[ln % DUB_SEQ_LANES] = 0u;
}

static void dub_lane_spp_refresh(void)
{
    uint8_t ln;

    g.lane_any = 0u;
    for (ln = 0u; ln < DUB_SEQ_LANES; ln++) {
        uint32_t spp = g.lane_den ? (uint32_t)DUB_FS * 60u * dub_lane_ticks(ln) / g.lane_den : 0xFFFFFFFFu;

        if (spp < 32u)
            spp = 32u;
        g.lane_spp[ln] = spp;
        if (dub_lane_is_sub(ln))
            g.lane_any = 1u;
    }
}

/* Called at every grid tick (seq_tick) before the grid mask is built: LFO pace moves are quantized to the
 * 1/16 grid. A changed lane counter resyncs to the global step (acc = 0), same as a manual pace change. */
static void dub_lane_pace_lfo_sync(void)
{
#if !defined(GENRE_BASS)
    uint8_t ln;
    uint8_t changed = 0u;

    for (ln = 0u; ln < DUB_SEQ_LANES; ln++) {
        uint8_t e = dub_lane_pace_eff(ln);

        if (e != g.lane_pace_cur[ln]) {
            g.lane_pace_cur[ln] = e;
            dub_lane_resync(ln);
            changed = 1u;
        }
    }
    if (changed)
        dub_lane_spp_refresh();
#endif
}

/* Fire one lane at its own step st: same event resolution as seq_tick, lane mask = ln. */
static void dub_lane_fire_at(uint8_t ln, uint8_t st)
{
    dub_events_t ev;
    dub_step_t *ovr = &g.cfg.seq[st];
    const dub_step_t *use = (ovr->force & 1u) ? ovr : 0;
    uint8_t pg;
    uint8_t vv;

    if (!g.playing)
        return;
    dub_resolve_step_morphed(use, st, &ev);
    if (!(ovr->force & 1u) || ovr->note == 0u)
        ev.note = scaled_note(g.voice.p.root, g.voice.p.scale, st);
    pg = events_gate(&ev);
    vv = fm_seq_view_of_step(st);
    if (vv >= 1u && !seq_view_playable(vv))
        pg = 0u;
#ifdef DUB_LANE_TRACE
    fprintf(stderr, "LANE %u %u %u %u\n", (unsigned)ln, (unsigned)fm1_ms, (unsigned)st, (unsigned)g.lane_spp[ln]);
#endif
    g_lane_fire_mask = (uint8_t)(1u << ln);
    g_lane_fire_sub = 1u;
    fire_seq_step(st, &ev, pg);
    g_lane_fire_sub = 0u;
    g_lane_fire_mask = DUB_LANE_MASK_ALL;
}

/* Called once per audio sample while playing (internal clock). */
static void dub_lane_clock_sample(void)
{
    uint8_t ln;

    if (!g.lane_any)
        return;
    for (ln = 0u; ln < DUB_SEQ_LANES; ln++) {
        if (!dub_lane_is_sub(ln))
            continue;
        g.lane_acc[ln]++;
        while (g.lane_acc[ln] >= g.lane_spp[ln]) {
            g.lane_acc[ln] -= g.lane_spp[ln];
            g.lane_st[ln] = dub_lane_next_step(g.lane_st[ln]);
            dub_lane_fire_at(ln, g.lane_st[ln]);
        }
    }
}

static void seq_tick(void)
{
    uint8_t st;
    dub_events_t ev;
    dub_step_t *ovr;
    const dub_step_t *use;

    dub_lane_pace_lfo_sync();
    (void)seq_chain_enter();
    st = g.step;
    ovr = &g.cfg.seq[st];
    use = (ovr->force & 1u) ? ovr : 0;
    arp_loop_mode_update();
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    idm_loop_macro_sync();
#endif
    dub_loop_snap_manage();
    sync_voice_params();
    dub_resolve_step_morphed(use, st, &ev);
    if (!(ovr->force & 1u) || ovr->note == 0u)
        ev.note = scaled_note(g.voice.p.root, g.voice.p.scale, st);
    if (!g.playing || fm_seq_is_mute_quant_step(st)) {
        dub_apply_lfo_seq_mutes();
        dub_sync_heard_mutes();
    }
    if (g.playing) {
        uint8_t pg = events_gate(&ev);
        uint8_t vv = fm_seq_view_of_step(st);

#if defined(GENRE_BASS)
        if (st == 0u) {
            g.vary_seed = g.vary_seed * 1664525u + 1013904223u;
            g.ui_dirty = 1u;
        }
#endif
        if (vv >= 1u && !seq_view_playable(vv))
            pg = 0u;
        /* Metrical index of the step being fired (before bar_step++). */
        g.rail_step = (g.loop_active && g.loop_len > 0u)
                          ? (uint8_t)seq_loop_rel(st)
                          : g.bar_step;
#ifdef DUB_LANE_TRACE
        {
            uint8_t tl;

            for (tl = 0u; tl < DUB_SEQ_LANES; tl++)
                if ((dub_grid_lane_mask() >> tl) & 1u)
                    fprintf(stderr, "LANE %u %u %u %u\n", (unsigned)tl, (unsigned)fm1_ms, (unsigned)st,
                            (unsigned)g.samples_per_step);
        }
#endif
        g_lane_fire_mask = dub_grid_lane_mask();
        fire_seq_step(st, &ev, pg);
        g_lane_fire_mask = DUB_LANE_MASK_ALL;
#if !defined(GENRE_BASS)
        arp_chord_hits((uint8_t)(st & (DUB_STEPS - 1u)));
        arp_roll_hits((uint8_t)(st & (DUB_STEPS - 1u)));
        dub_dj_gest_seq_step();
#endif
    }
    g.bar_step = (uint8_t)((g.bar_step + 1u) & (DUB_STEPS - 1u));
    if (g.bar_step == 0u) {
        g.bar = (uint8_t)((g.bar + 1u) & 7u);
        g.seq_rng = g.seq_rng * 1664525u + 1013904223u;
        if (g.kick_slam_bars > 0u) {
            g.kick_slam_bars--;
            if (g.kick_slam_bars == 0u)
                g.drop_emph_span = 0u;
        }
        {
            uint8_t ag = gesture_active();
            if (ag == GEST_SOLO_BASS || ag == GEST_SOLO_DRUM) {
                if (g.gesture_solo_kind == ag && g.gesture_solo_bar < 255u)
                    g.gesture_solo_bar++;
            } else {
                if (g.gesture_solo_kind == GEST_SOLO_DRUM) {
                    g.kick_slam_bars = 2u;
                    g.drop_emph_span = 2u;
                }
                g.gesture_solo_kind = GEST_NONE;
                g.gesture_solo_bar = 0u;
            }
        }
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
        house_fill_build_bar();
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        idm_stutter_bar();
#endif
        if (g.dj.on)
            g.dj.bar_pending = 1u;
    }
    if (g.view != VIEW_SEQ) {
        static const uint8_t gk[4] = {7u, 8u, 9u, 10u};
        uint32_t gi;
        for (gi = 0; gi < 4u; gi++) {
            if (fm1_in.notes & (1u << KEY_BLACK[gk[gi]])) {
                if (g.gest_hold[gi] < 250u)
                    g.gest_hold[gi]++;
                /* Latch performance gesture after holding for 1 bar (16 steps) */
                if (g.gest_hold[gi] == 16u && g.gesture != GEST_NONE) {
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
                    if (house_fill_latch_block(gi))
                        ;
                    else
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
                    if (idm_stutter_latch_block(gi, (uint8_t)((fm1_in.buttons >> BTN_EDIT) & 1u))) {
                        if (gi == 0u && (fm1_in.buttons & (1u << BTN_EDIT)))
                            idm_edit_latch_begin();
                    } else
#endif
                        g.gesture_latch = g.gesture;
                }
            } else {
                g.gest_hold[gi] = 0u;
            }
        }
    }
    seq_advance_step(st);
    g.ui_dirty = 1u;
}

static void reset_bank(uint8_t bank)
{
    uint8_t what = 0;

#if !defined(GENRE_BASS)
    if (gv_reset_bank(bank))
        return;
#endif
#if defined(GENRE_BASS)
    if (bank == BANK_GROOVE) {
        if (g.page[BANK_GROOVE] % bank_pages(BANK_GROOVE) == 0u) {
            dub_plate_groove_read(g.cfg.p.plate, &g.cfg.p.root, &g.cfg.p.scale, &g.cfg.p.swing, &g.cfg.p.bpm);
            dub_morph_sync_field(&g.cfg.p.root);
            dub_morph_sync_field(&g.cfg.p.scale);
            dub_morph_sync_field(&g.cfg.p.swing);
            dub_morph_sync_field(&g.cfg.p.bpm);
            transport_set_bpm();
        } else {
            g.cfg.bass.midi_ch = 1u;
            bass_clock_mode_set(BASS_CLOCK_SEND);
            g.cfg.bass.transpose = 0;
            g.cfg.bass.vel_sens = 127u;
        }
        mark_dirty();
        return;
    }
    if (bank == BANK_CHORD) {
        bass_dahdsr_defaults(&g.cfg.bass.env);
        g.cfg.bass_env_shape = bass_env_shape_pack(BASS_DAHDSR_CURVE_LIN);
        mark_dirty();
        return;
    }
#endif
    if (bank == BANK_CHORD)
        what = DUB_APPLY_CHORD;
    else if (bank == BANK_BASS)
        what = DUB_APPLY_BASS;
    else if (bank == BANK_DRUM)
        what = DUB_APPLY_DRUM;
    else if (bank == BANK_FX)
        what = DUB_APPLY_FX;
    else if (bank == BANK_LFO) {
        g.lfo_target = dub_lfo_tgt_default();
        g.cfg.p.lfo1_wave = 0u;
        g.cfg.p.lfo1_rate = 4;
        g.cfg.p.lfo1_depth = 0;
        g.cfg.p.lfo1_tgt = DUB_LFO_CUT_C;
        g.cfg.p.lfo2_wave = 0u;
        g.cfg.p.lfo2_rate = 4;
        g.cfg.p.lfo2_depth = 0;
        g.cfg.p.lfo2_tgt = DUB_LFO_WOB;
        dub_lfo_fm_defaults(&g.cfg.p);
        dub_era_lfo_preset(g.cfg.p.plate);
        mark_dirty();
        return;
    } else if (bank == BANK_HOME) {
        if (!dub_home_macros_at_era_preset()) {
            dub_era_home_macros_apply_era(g.cfg.p.era, 1u);
            mark_dirty();
            return;
        }
        dub_home_randomize_morph();
        return;
    }
    if (what) {
        genre_plate_apply(&g.cfg.p, g.cfg.p.plate, what);
        if (bank == BANK_BASS) {
#ifdef GENRE_HEAVY_PSYCH
            g.cfg.p.bass_model = HEAVY_PSYCH_TRAX[heavy_psych_era_first_trax(genre_clamp_era(g.cfg.p.era))].bass_model;
#elif defined(GENRE_FUNK)
            g.cfg.p.bass_model = FUNK_GROOVE[funk_era_first_groove(genre_clamp_era(g.cfg.p.era))].bass_model;
#elif defined(GENRE_HOUSE)
            g.cfg.p.bass_model = HOUSE_TRAX[house_era_first_trax(genre_clamp_era(g.cfg.p.era))].bass_model;
#elif defined(GENRE_JAZZ)
            g.cfg.p.bass_model = JAZZ_TRAX[jazz_era_first_trax(genre_clamp_era(g.cfg.p.era))].bass_model;
#elif defined(GENRE_SOUL)
            g.cfg.p.bass_model = soul_side(soul_era_first_side(genre_clamp_era(g.cfg.p.era)))->bass_model;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            g.cfg.p.bass_model = IDM_PIECE[idm_era_first_piece(genre_clamp_era(g.cfg.p.era))].bass_model;
#elif defined(GENRE_HIPHOP)
            g.cfg.p.bass_model = HIPHOP_ERA_FX[hiphop_clamp_era(g.cfg.p.era)].bass_model;
#elif defined(GENRE_POP)
            g.cfg.p.bass_model = POP_ERA_FX[pop_clamp_era(g.cfg.p.era)].bass_model;
#elif defined(GENRE_TECHNO)
            g.cfg.p.bass_model = TECHNO_ERA_FX[techno_clamp_era(g.cfg.p.era)].bass_model;
#elif defined(GENRE_BASS)
            g.cfg.p.bass_model = BASS_ERA_FX[bass_clamp_era(g.cfg.p.era)].bass_model;
#else
            g.cfg.p.bass_model = DUB_ERA_FX[genre_clamp_era(g.cfg.p.era)].bass_model;
#endif
        }
        if (bank == BANK_DRUM) {
#ifdef GENRE_HEAVY_PSYCH
            g.cfg.p.drum_kit = HEAVY_PSYCH_TRAX_KIT[heavy_psych_era_first_trax(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_FUNK)
            g.cfg.p.drum_kit = FUNK_GROOVE_KIT[funk_era_first_groove(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_HOUSE)
            g.cfg.p.drum_kit = HOUSE_TRAX_KIT[house_era_first_trax(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_JAZZ)
            g.cfg.p.drum_kit = JAZZ_TRAX_KIT[jazz_era_first_trax(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_SOUL)
            g.cfg.p.drum_kit = SOUL_SIDE_KIT[soul_era_first_side(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            g.cfg.p.drum_kit = IDM_PIECE_KIT[idm_era_first_piece(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_HIPHOP)
            g.cfg.p.drum_kit = (uint8_t)HIPHOP_BEAT_KIT[hiphop_era_first_beat(genre_clamp_era(g.cfg.p.era))];
#elif defined(GENRE_POP)
            g.cfg.p.drum_kit = POP_ERA_FX[pop_clamp_era(g.cfg.p.era)].drum_kit;
#elif defined(GENRE_TECHNO)
            g.cfg.p.drum_kit = TECHNO_ERA_FX[techno_clamp_era(g.cfg.p.era)].drum_kit;
#elif defined(GENRE_BASS)
            g.cfg.p.drum_kit = BASS_ERA_FX[bass_clamp_era(g.cfg.p.era)].drum_kit;
#else
            g.cfg.p.drum_kit = DUB_ERA_FX[genre_clamp_era(g.cfg.p.era)].drum_kit;
#endif
            dub_sel_sync_saved_lvls();
        }
    }
    mark_dirty();
}

static uint8_t bank_view(uint8_t bank)
{
    if (bank == BANK_CHORD)
        return VIEW_CHORD;
    if (bank == BANK_BASS)
        return VIEW_BASS;
    if (bank == BANK_FX)
        return VIEW_SPACE;
    if (bank == BANK_LFO)
        return VIEW_LFO;
    if (bank == BANK_HOME)
        return VIEW_HOME;
    if (bank == BANK_SEQ)
        return VIEW_SEQ;
#if defined(GENRE_BASS)
    if (bank == BANK_GROOVE)
        return VIEW_DESK;
    return VIEW_HOME;
#else
    /* VIEW_CHORD / VIEW_BASS host the ENV / GLO plots (draw dispatch checks g.bank). */
    if (bank == BANK_ENV)
        return VIEW_CHORD;
    if (bank == BANK_GROOVE)
        return VIEW_BASS;
    return VIEW_DESK;
#endif
}

static void seq_plock_latch_end(void)
{
    g.seq_plock_latch = 0u;
    g.seq_plock_hold = 0u;
    g.white_press_k = 255u;
    g.white_press_ms = 0u;
    g.white_long_used = 0u;
    g.white_long_cancel = 0u;
}

/* Any panel button or note-key press drops latched pLock; 4 param knobs are exempt. */
static void seq_plock_latch_touch_break(uint32_t btn_edges, uint32_t note_rising)
{
    if (!g.seq_plock_latch)
        return;
#if defined(GENRE_BASS)
    if (g.view == VIEW_SEQ && g.white_press_k < 16u && note_rising)
        return;
#endif
    if (g.view == VIEW_SEQ) {
        uint32_t k;

        /* White step keys resolve the latch on release / long-press in handle_keys, not on press. */
        for (k = 0u; k < 16u; k++)
            note_rising &= ~(1u << KEY_WHITE[k]);
    }
#if defined(GENRE_BASS)
    if (btn_edges == (1u << BTN_SEQ) && !note_rising)
        return;
#endif
    if (btn_edges || note_rising) {
        seq_plock_latch_end();
        g.ui_dirty = 1u;
    }
}

#if defined(GENRE_BASS)
static void bass_sanitize_nav(void)
{
    if (g.bank == BANK_DRUM) {
        g.bank = BANK_HOME;
        g.view = VIEW_HOME;
        g.ui_dirty = 1u;
        return;
    }
    if (g.view == VIEW_DESK && g.bank != BANK_GROOVE) {
        g.bank = BANK_HOME;
        g.view = VIEW_HOME;
        g.ui_dirty = 1u;
        return;
    }
    if (g.view == VIEW_SEQ || g.bank == BANK_SEQ) {
        g.seq_page = dub_seq_page_norm(g.seq_page);
        return;
    }
    {
        uint8_t want = bank_view(g.bank);

        if (g.view != want) {
            g.view = want;
            g.ui_dirty = 1u;
        }
    }
}

static int bass_bank_is_drumish(void)
{
    return g.bank == BANK_DRUM || (g.view == VIEW_DESK && g.bank != BANK_GROOVE);
}
#endif

static void nav_bank(uint8_t bank)
{
#if defined(GENRE_BASS)
    if (bank == BANK_DRUM)
        bank = BANK_HOME;
#endif
    uint8_t nv = bank_view(bank);

    if (g.view == VIEW_SEQ && nv != VIEW_SEQ)
        seq_plock_latch_end();
    g.bank = bank;
    g.view = nv;
#if defined(GENRE_BASS)
    if (bank == BANK_FX)
        g.view = VIEW_SPACE;
#endif
    g.ui_dirty = 1u;
}

static void nav_view(uint8_t view)
{
    if (g.view == VIEW_SEQ && view != VIEW_SEQ)
        seq_plock_latch_end();
    g.view = (uint8_t)(view % VIEW_COUNT);
#if defined(GENRE_BASS)
    if (g.view == VIEW_DESK) {
        g.view = VIEW_HOME;
        g.bank = BANK_HOME;
    } else
#endif
    if (g.view == VIEW_DESK)
        g.bank = BANK_DRUM;
    else if (g.view == VIEW_CHORD)
        g.bank = BANK_CHORD;
    else if (g.view == VIEW_BASS)
        g.bank = BANK_BASS;
    else if (g.view == VIEW_SPACE)
        g.bank = BANK_FX;
    else if (g.view == VIEW_LFO)
        g.bank = BANK_LFO;
    else if (g.view == VIEW_HOME)
        g.bank = BANK_HOME;
    else if (g.view == VIEW_SEQ && g.bank != BANK_SEQ) {
        g.seq_ret_bank = g.bank;
        g.bank = BANK_SEQ;
    }
#if defined(GENRE_BASS)
    if (g.view == VIEW_SEQ && g.seq_plock_step == FM_SEQ_PLOCK_NONE)
        g.seq_plock_step = bass_seq_cursor_step_runtime(FM_SEQ_PLOCK_NONE, g.seq_view_page, g.cfg.seq_pages,
                                                        g.step);
#endif
    g.ui_dirty = 1u;
}

/* LFO long-press (>= 500 ms): reset only the current LFO target (g.lfo_target), stays on the current view. */
#define DUB_LFO_LONG_MS 500u
static void lfo_target_reset(void)
{
    uint8_t t = (uint8_t)(g.lfo_target % DUB_LFO_TGT_N);

    fm_lfo_slot_reset(&g.lfo_slots[t]);
    g.lfo_rt[t].ph = 0u;
    g.lfo_rt[t].cycle = 0u;
    g.lfo_rt[t].sm = 0;
    fm_dj_lfo_eff_build(&g.dj_lfo_ov, &g.dj, g.lfo_slots, g.lfo_slots_eff, DUB_LFO_TGT_N);
    mark_dirty();
}

static void bank_button(uint8_t bank, uint8_t edit_held)
{
    if (edit_held) {
        reset_bank(bank);
        return;
    }
    /* Only advance page if we are ALREADY on both this bank AND its canonical stage view.
     * If we are in VIEW_SEQ (or any other overlay view), pressing ANY bank button (even the same bank)
     * immediately exits SEQ and returns to that bank's view! */
    if (g.bank == bank && g.view == bank_view(bank)) {
        g.page[bank] = (uint8_t)((g.page[bank] + 1u) % bank_pages(bank));
    } else
        nav_bank(bank);
    g.ui_dirty = 1u;
}

/* Era knob DNA only — plate index / pgen n unchanged (ko_era_plate.h). */
static void genre_era_knob_apply(dub_params_t *p)
{
    if (!p)
        return;
    genre_era_apply_base(p, p->era);
#if defined(GENRE_BASS)
    bass_seq_sync_genre_pattern();
#endif
}

/* Groove heard before a turn: voice snapshot, cfg where the voice has none. Take before p changes. */
static ko_groove_t dub_groove_held(const dub_params_t *prev, const dub_params_t *p)
{
    ko_groove_t held = {prev->bpm ? prev->bpm : p->bpm, prev->swing ? prev->swing : p->swing, prev->root,
                        prev->scale};

    return held;
}

static void dub_turn_groove(dub_params_t *p, const ko_groove_t *held, uint8_t turn)
{
    ko_groove_ref_t live = {&p->bpm, &p->swing, &p->root, &p->scale};

    ko_turn_groove(&live, held, turn, g.playing, &g.groove_plate_pending);
}

static void dub_groove_commit_plate_if_pending(void)
{
    uint8_t root, scale, swing, bpm;

    if (!g.groove_plate_pending)
        return;
    dub_plate_groove_read(g.cfg.p.plate, &root, &scale, &swing, &bpm);
    g.cfg.p.root = root;
    g.cfg.p.scale = scale;
    g.cfg.p.swing = swing;
    g.cfg.p.bpm = bpm;
    g.groove_plate_pending = 0u;
    transport_set_bpm();
}

static void dub_groove_freeze_both(const dub_params_t *live)
{
    if (!g.playing || !live)
        return;
    g.cfg.p.bpm = live->bpm ? live->bpm : g.cfg.p.bpm;
    g.cfg.p.root = live->root;
    g.cfg.p.scale = live->scale;
    g.cfg.p.swing = live->swing ? live->swing : g.cfg.p.swing;
    g.morph_from.bpm = g.cfg.p.bpm;
    g.morph_from.root = g.cfg.p.root;
    g.morph_from.scale = g.cfg.p.scale;
    g.morph_from.swing = g.cfg.p.swing;
}

static void dub_morph_retarget_begin(const dub_params_t *live_from)
{
    if (g.morph_pos >= DUB_MORPH_STEPS)
        dub_morph_begin(live_from);
    dub_groove_freeze_both(live_from);
    if (g.morph_pos < DUB_MORPH_STEPS)
        dub_morph_sync_enums(&g.morph_from, &g.cfg.p, g.morph_tick);
}

static void dub_cue_nudge(uint8_t *cur, uint8_t to)
{
    if (*cur < to)
        (*cur)++;
    else if (*cur > to)
        (*cur)--;
}

/* One KEY cue step: shortest path toward dest, skipping pitch classes outside live scale. */
static void dub_cue_nudge_root(uint8_t *cur, uint8_t to, uint8_t scale)
{
    uint8_t c = *cur;
    int32_t sign;
    uint8_t n;
    uint8_t k;

    if (c == to)
        return;
    sign = c < to ? 1 : -1;
    n = c;
    for (k = 0; k < 12u; k++) {
        n = (uint8_t)((int32_t)n + sign);
        if (n == to) {
            *cur = to;
            return;
        }
        if (dub_root_pc_in_scale(n % 12u, c % 12u, scale)) {
            *cur = n;
            return;
        }
    }
}

static int dub_cue_arrived(void)
{
    return g.cue_bpm == g.cfg.p.bpm && g.cue_swing == g.cfg.p.swing &&
           g.cue_root == g.cfg.p.root && g.cue_scale == g.cfg.p.scale;
}

static void dub_cue_snap_to_goal(void)
{
    g.cue_bpm = g.cfg.p.bpm;
    g.cue_swing = g.cfg.p.swing;
    g.cue_root = g.cfg.p.root;
    g.cue_scale = g.cfg.p.scale;
    g.cue_acc = 0u;
    g.cue_ticks = 0u;
    if (g.cue_on)
        g.cue_on = 0u;
    dub_home_morph_flash_end_check();
}

static void dub_morph_snap_to_goal(void)
{
    if (g.morph_pos < DUB_MORPH_STEPS) {
        g.morph_from = g.cfg.p;
        g.morph_pos = DUB_MORPH_STEPS;
        g.morph_acc = 0u;
        g.morph_tick = 0u;
        fm_seq_bits_fill(g.morph_from_seq, 0);
        transport_set_bpm();
    }
}

static void dub_macro_snap_to_goal(void)
{
    uint8_t macro_cap = dub_macro_morph_cap();

    if (g.macro_morph_pos < macro_cap) {
        g.macro_morph_pos = macro_cap;
        g.macro_morph_acc = 0u;
        dub_home_morph_flash_end_check();
    }
}

static void dub_transport_morph_snap(void)
{
    if (!g.playing)
        dub_groove_commit_plate_if_pending();
    dub_morph_snap_to_goal();
    dub_macro_snap_to_goal();
    dub_cue_snap_to_goal();
    g.ui_dirty = 1u;
}

/* Constant slew, not a 4-bar ramp: 1 BPM and 1 swing per beat, 1 in-scale KEY step per bar,
 * 1 scale step per bar (same cadence as KEY). Short hops land first. */
static void dub_cue_clock(uint32_t samples)
{
    uint32_t step_smp;
    uint8_t mbpm;

    if (!g.playing) {
        if (g.cue_on)
            dub_cue_snap_to_goal();
        return;
    }
    if (!g.cue_on)
        return;
    if (dub_cue_arrived()) {
        g.cue_on = 0u;
        dub_home_morph_flash_end_check();
        return;
    }
    mbpm = g.cue_bpm ? g.cue_bpm : 120u;
    if (mbpm < 40u)
        mbpm = 40u;
    if (mbpm > 180u)
        mbpm = 180u;
    step_smp = DUB_FS * 60u / ((uint32_t)mbpm * 4u);
    if (step_smp < 32u)
        step_smp = 5512u;
    g.cue_acc += samples;
    while (g.cue_acc >= step_smp && g.cue_on) {
        g.cue_acc -= step_smp;
        g.cue_ticks++;
        if ((g.cue_ticks % 4u) == 0u) {
            dub_cue_nudge(&g.cue_bpm, g.cfg.p.bpm);
            dub_cue_nudge(&g.cue_swing, g.cfg.p.swing);
        }
        if ((g.cue_ticks % 16u) == 0u) {
            dub_cue_nudge_root(&g.cue_root, g.cfg.p.root, g.cue_scale);
            dub_cue_nudge(&g.cue_scale, g.cfg.p.scale);
        }
        g.ui_dirty = 1u;
        if (dub_cue_arrived()) {
            g.cue_on = 0u;
            dub_home_morph_flash_end_check();
        }
    }
}

static void dub_load_plate_song_groove(uint8_t plate)
{
    dub_params_t prev;
    uint8_t root, scale, swing, bpm;

    dub_plate_groove_read(plate, &root, &scale, &swing, &bpm);
    dub_morph_snapshot_voice(&prev);
    g.cue_bpm = prev.bpm ? prev.bpm : bpm;
    g.cue_swing = prev.swing ? prev.swing : swing;
    g.cue_root = prev.root;
    g.cue_scale = prev.scale;
    g.cue_ticks = 0u;
    g.cue_acc = 0u;
    g.cfg.p.root = root;
    g.cfg.p.scale = scale;
    g.cfg.p.swing = swing;
    g.cfg.p.bpm = bpm;
    if (!g.playing || dub_cue_arrived()) {
        g.cue_on = 0u;
        g.cue_bpm = bpm;
        g.cue_swing = swing;
        g.cue_root = root;
        g.cue_scale = scale;
        dub_home_morph_flash_clear_bits(DUB_FLASH_CUE);
    } else {
        g.cue_on = 1u;
        dub_home_morph_flash_arm(DUB_FLASH_CUE);
    }
    mark_dirty();
}

/* Toast-Helfer (900 ms), aktuell nur fuer DJ ON/OFF. Plate-/Era-Namen werden nicht mehr als Toast angezeigt. */
static void plate_era_toast_set(const char *msg, uint16_t bar)
{
    uint32_t i;

    for (i = 0u; i < 15u && msg[i]; i++)
        g.plate_era_toast[i] = msg[i];
    g.plate_era_toast[i] = '\0';
    g.plate_era_toast_bar = bar;
    g.plate_era_toast_until = fm1_ms + 900u;
    g.ui_dirty = 1u;
}

static void cycle_plate(int32_t dir)
{
    dub_params_t prev;
    dub_params_t *p = &g.cfg.p;
    uint8_t knob_era = p->era;
    uint8_t keep_bass = p->bass_model;
    uint8_t keep_drum_kit = p->drum_kit;
    ko_groove_t held;
#if !defined(FM_PGEN_ON)
    int32_t v;
#endif

    dub_morph_snapshot_voice(&prev);
    held = dub_groove_held(&prev, p);
    if (fm_era_is_artist(p->era)) {
        if (!fm_artist_plate_count())
            return;
#if defined(FM_PGEN_ON)
        genre_pgen_off();
#endif
        if (!(g.user_plate_mask & (1ull << (p->plate % GENRE_CFG_USER_PLATE_N))))
            p->plate = fm_artist_first_plate();
        else if (dir != 0)
            p->plate = fm_artist_cycle_plate(p->plate, dir);
        genre_plate_preset_apply(p, (uint8_t)FM_ERA_ARTIST, p->plate, keep_bass, keep_drum_kit, 0u);
        dub_plate_seq_reload_song_lanes(p->plate);
        dub_sel_sync_saved_lvls();
        dub_plate_seq_morph_prep();
        dub_home_morph_flash_arm(DUB_FLASH_XF);
        dub_morph_retarget_begin(&prev);
        dub_era_lfo_apply(p->era, p->plate);
        mark_dirty();
        return;
    }
#if defined(FM_PGEN_ON)
    p->plate = fm_pgen_goto(knob_era, g.cfg.pgen.n + dir);
#else
    v = (int32_t)p->plate + dir;
    while (v < 0)
        v += GENRE_PRESET_N;
    p->plate = (uint8_t)(v % GENRE_PRESET_N);
#endif
    genre_plate_preset_apply(p, knob_era, p->plate, keep_bass, keep_drum_kit, 0u);
    genre_era_knob_apply(p);
    p->era = knob_era;
    dub_plate_seq_reload_song_lanes(p->plate);
    dub_sel_sync_saved_lvls();
    dub_turn_groove(p, &held, KO_TURN_PLATE);
    dub_era_lfo_apply(p->era, p->plate);
    mark_dirty();
}

static void cycle_era(int32_t dir)
{
    dub_params_t prev;
    ko_groove_t held;
    uint8_t era_n = fm_artist_plate_count() ? FM_ERA_KNOB_N : GENRE_ERA_N;

    dub_morph_snapshot_voice(&prev);
    held = dub_groove_held(&prev, &g.cfg.p);
    int32_t v = (int32_t)g.cfg.p.era + dir;
    while (v < 0)
        v += era_n;
    g.cfg.p.era = fm_era_knob_clamp((uint8_t)(v % era_n));
    if (fm_era_is_artist(g.cfg.p.era)) {
        if (fm_artist_plate_count()) {
#if defined(FM_PGEN_ON)
            genre_pgen_off();
#endif
            g.cfg.p.plate = fm_artist_first_plate();
            genre_plate_preset_apply(&g.cfg.p, (uint8_t)FM_ERA_ARTIST, g.cfg.p.plate, g.cfg.p.bass_model,
                                    g.cfg.p.drum_kit, 0u);
        } else {
            g.cfg.p.era = genre_era_of_preset(g.cfg.p.plate);
            genre_preset_rebuild(&g.cfg.p, g.cfg.p.era, g.cfg.p.plate);
        }
        dub_plate_seq_reload_song_lanes(g.cfg.p.plate);
    } else {
        g.cfg.p.era = genre_clamp_era(g.cfg.p.era);
        genre_era_knob_apply(&g.cfg.p);
        dub_turn_groove(&g.cfg.p, &held, KO_TURN_ERA);
        dub_lfo_fm_defaults(&g.cfg.p);
        dub_era_lfo_apply(g.cfg.p.era, g.cfg.p.plate);
        dub_era_home_macros_apply_era(g.cfg.p.era, 1u);
        dub_sel_sync_saved_lvls();
        mark_dirty();
        return;
    }
    dub_turn_groove(&g.cfg.p, &held, KO_TURN_ERA);
    dub_lfo_fm_defaults(&g.cfg.p);
    dub_era_lfo_apply(g.cfg.p.era, g.cfg.p.plate);
    if (!fm_era_is_artist(g.cfg.p.era))
        dub_plate_seq_reload_song_lanes(g.cfg.p.plate);
    dub_home_morph_flash_arm(DUB_FLASH_EV);
    dub_era_home_macros_apply_era(g.cfg.p.era, 1u);
    dub_sel_sync_saved_lvls();
    dub_morph_retarget_begin(&prev);
    mark_dirty();
}

static uint8_t sel_drm_ix(void)
{
    return g.sel_drm % 4u;
}

static uint8_t sel_drm_is_siren(void)
{
    return (g.sel_drm % 4u) == 3u;
}

static void dub_drm_dens_fixup(dub_params_t *p)
{
    uint8_t i;
    uint8_t nz = 0u;

    if (!p)
        return;
    for (i = 0; i < 4u; i++) {
        if (p->drm_dens[i] != 0u)
            nz = 1u;
    }
    if (!nz) {
        for (i = 0; i < 4u; i++)
            p->drm_dens[i] = 64u;
        p->density = 64u;
    }
}

/* Drum knob LFO target of voice sel_drm (0..3); the drum bus targets only survive as load migration. */
static uint8_t dub_lfo_drm_v(uint8_t voice0_tgt, uint8_t sel_drm)
{
    return (uint8_t)(voice0_tgt + (sel_drm % 4u));
}

static uint8_t dub_lfo_target_hw(uint8_t bank, uint8_t pg, uint8_t slot, uint8_t seq_page, uint8_t seq_view,
                                 uint8_t sel_drm)
{
    uint8_t seq_lt;

#if defined(GENRE_BASS)
    if (seq_view && dub_seq_knob_lfo_map_ctx(seq_page, slot, &seq_lt))
        return seq_lt;
    if (seq_view)
        return 255u;
    if (bank == BANK_HOME) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_CUT_B : slot == 1 ? DUB_LFO_RESO_C : slot == 2 ? DUB_LFO_DRV_B :
                   DUB_LFO_DLY_B;
        /* GLIDE rides atk_b (bass_app.h); MAC 1..3 = macro 0..2 */
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_ATK_B : (uint8_t)(DUB_LFO_M_CHORD + slot - 1u);
        /* PERFORM p2 GATE/DENS/RATCH/SHIFT = BASS_PERF_MACRO_IX macros (not in EDIT routing). */
        if (pg == 2u && slot < BASS_PERF_N && (fm1_in.buttons & (1u << BTN_EDIT)) == 0u)
            return (uint8_t)(DUB_LFO_M_CHORD + BASS_PERF_MACRO_IX[slot]);
        return 255u;
    }
    if (bank == BANK_CHORD) {
        /* ENV: RETRIG / INVERT / mod-slot select+amount / M.CURV are switches or routing. */
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_ENV_ATK : slot == 1 ? DUB_LFO_ENV_DEC :
                   slot == 2 ? DUB_LFO_ENV_SUS : DUB_LFO_ENV_REL;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_ENV_DLY : slot == 1 ? DUB_LFO_ENV_HOLD :
                   slot == 2 ? DUB_LFO_ENV_CURV : 255u;
        if (pg == 2u && slot == 1u)
            return DUB_LFO_ENV_ACC;
        return 255u;
    }
    if (bank == BANK_BASS) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_MODL_B : slot == 1 ? DUB_LFO_ATK_B :
                   slot == 2 ? DUB_LFO_SUB_B : DUB_LFO_PAN_B;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_LVL_B : slot == 1 ? DUB_LFO_DRV_B :
                   slot == 2 ? DUB_LFO_DLY_B : DUB_LFO_REV_B;
        return slot == 0 ? DUB_LFO_RESO_C : slot == 1 ? DUB_LFO_CUT_B :
               slot == 2 ? DUB_LFO_ENV_C : DUB_LFO_WOB;
    }
    if (bank == BANK_GROOVE) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_KEY : slot == 1 ? DUB_LFO_SCALE :
                   slot == 2 ? DUB_LFO_SWING : DUB_LFO_TEMPO;
        /* p1: MIDI CH / CLOCK excluded */
        return slot == 2 ? DUB_LFO_TRANSP : slot == 3 ? DUB_LFO_VELSENS : 255u;
    }
    if (bank == BANK_FX) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_EMIX : slot == 1 ? DUB_LFO_TIME : slot == 2 ? DUB_LFO_FB : DUB_LFO_LOOP;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_WOW : slot == 1 ? DUB_LFO_TAP : slot == 2 ? DUB_LFO_TANK : DUB_LFO_ISO;
        return slot == 0 ? DUB_LFO_REV : slot == 1 ? DUB_LFO_RDAMP : slot == 2 ? DUB_LFO_RTAIL : DUB_LFO_ESEND;
    }
    return 255u;
#endif
    if (seq_view && dub_seq_knob_lfo_map_ctx(seq_page, slot, &seq_lt))
        return seq_lt;
    if (seq_view)
        return 255u;
    if (bank == BANK_HOME) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_M_CHORD : slot == 1 ? DUB_LFO_M_FEED : slot == 2 ? DUB_LFO_M_WOB : DUB_LFO_M_ISO;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_M_SPACE : slot == 1 ? DUB_LFO_M_WEIGHT : slot == 2 ? DUB_LFO_M_GRIT : DUB_LFO_M_DUB;
        return slot == 0 ? DUB_LFO_M_MOTION : slot == 1 ? DUB_LFO_M_FLUX :
               slot == 2 ? DUB_LFO_ENS : DUB_LFO_FLIP;
    }
    if (bank == BANK_CHORD) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_VOIC_C : slot == 1 ? DUB_LFO_CUT_C : slot == 2 ? DUB_LFO_ATK_C : DUB_LFO_DEC_C;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_TIMB_C : slot == 1 ? DUB_LFO_RESO_C : slot == 2 ? DUB_LFO_ENV_C : DUB_LFO_GLIDE_C;
        return slot == 0 ? DUB_LFO_LVL_C : slot == 1 ? DUB_LFO_PAN_C :
               slot == 2 ? DUB_LFO_DLY_C : DUB_LFO_REV_C;
    }
    if (bank == BANK_BASS) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_MODL_B : slot == 1 ? DUB_LFO_CUT_B : slot == 2 ? DUB_LFO_ATK_B : DUB_LFO_DEC_B;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_SUB_B : slot == 1 ? DUB_LFO_PUMP_B : slot == 2 ? DUB_LFO_DRV_B : DUB_LFO_WOB;
        return slot == 0 ? DUB_LFO_LVL_B : slot == 1 ? DUB_LFO_PAN_B :
               slot == 2 ? DUB_LFO_DLY_B : DUB_LFO_REV_B;
    }
    if (bank == BANK_DRUM) {
        uint8_t drm_ix = sel_drm % 4u;

        if (pg == 0u) {
            if (slot == 0)
                return DUB_LFO_KIT;
            if (slot == 1)
                return 255u;
            if (slot == 2)
                return (uint8_t)(DUB_LFO_KICK + drm_ix);
            return dub_lfo_drm_v(DUB_LFO_KTUNE_K, sel_drm);
        }
        if (pg == 1u) {
            if (slot == 0)
                return dub_lfo_drm_v(DUB_LFO_DENS_K, sel_drm);
            if (slot == 1)
                return dub_lfo_drm_v(DUB_LFO_DCUT_K, sel_drm);
            if (slot == 2)
                return dub_lfo_drm_v(DUB_LFO_DATK_K, sel_drm);
            return dub_lfo_drm_v(DUB_LFO_KDEC_K, sel_drm);
        }
        if (slot == 0)
            return drm_ix == 3u ? DUB_LFO_SRATE :
                   dub_lfo_drm_v(DUB_LFO_KDRV_K, sel_drm);
        if (slot == 1)
            return (uint8_t)(DUB_LFO_PAN_K + drm_ix);
        if (slot == 2)
            return dub_lfo_drm_v(DUB_LFO_DLY_K, sel_drm);
        return dub_lfo_drm_v(DUB_LFO_REV_K, sel_drm);
    }
    if (bank == BANK_FX) {
        if (pg == 0u)
            return slot == 0 ? DUB_LFO_EMIX : slot == 1 ? DUB_LFO_TIME : slot == 2 ? DUB_LFO_FB : DUB_LFO_LOOP;
        if (pg == 1u)
            return slot == 0 ? DUB_LFO_WOW : slot == 1 ? DUB_LFO_TAP : slot == 2 ? DUB_LFO_TANK : DUB_LFO_ISO;
        return slot == 0 ? DUB_LFO_REV : slot == 1 ? DUB_LFO_RDAMP : slot == 2 ? DUB_LFO_RTAIL : DUB_LFO_ESEND;
    }
    return 255u;
}

static uint8_t dub_lfo_dest_order[DUB_LFO_TGT_N];
static uint8_t dub_lfo_dest_pos[DUB_LFO_TGT_N];
static fm_lfo_dests_t dub_lfo_dests;

static void dub_lfo_collect_hw_knob(uint8_t bank, uint8_t pg, uint8_t slot, uint8_t seq_page, uint8_t seq_view)
{
    uint8_t si;

    /* Drum lanes 0..3 only: the drum ALL selection (sel_drm 4) and its bus targets are gone. */
    for (si = 0u; si < (bank == BANK_DRUM ? 4u : 1u); si++)
        (void)fm_lfo_dests_add(&dub_lfo_dests, dub_lfo_target_hw(bank, pg, slot, seq_page, seq_view, si));
}

/* Dest list = exactly the targets reachable from bank/SEQ knobs (+ OCT); no separate allow-list. */
static void dub_lfo_order_build(void)
{
#if defined(GENRE_BASS)
    static const uint8_t walk_banks[] = {BANK_HOME, BANK_CHORD, BANK_BASS, BANK_FX, BANK_GROOVE};
#else
    static const uint8_t walk_banks[] = {BANK_HOME, BANK_CHORD, BANK_BASS, BANK_DRUM, BANK_FX};
#endif
    uint8_t bi, pg, slot, seq_pg;

    fm_lfo_dests_init(&dub_lfo_dests, dub_lfo_dest_order, dub_lfo_dest_pos, (uint8_t)DUB_LFO_TGT_N);
    (void)fm_lfo_dests_add(&dub_lfo_dests, DUB_LFO_OCT);

    for (bi = 0u; bi < sizeof walk_banks / sizeof walk_banks[0]; bi++) {
        uint8_t bank = walk_banks[bi];
        uint8_t pages = bank_pages(bank);

        for (pg = 0u; pg < pages; pg++) {
            for (slot = 0u; slot < 4u; slot++)
                dub_lfo_collect_hw_knob(bank, pg, slot, 0u, 0u);
        }
    }
#if !defined(GENRE_BASS)
    for (seq_pg = 0u; seq_pg < DUB_SEQ_NPAGES; seq_pg++) {
        for (slot = 0u; slot < 4u; slot++)
            dub_lfo_collect_hw_knob(0u, 0u, slot, seq_pg, 1u);
    }
#else
    for (seq_pg = 0u; seq_pg <= 2u; seq_pg++) {
        for (slot = 0u; slot < 4u; slot++)
            dub_lfo_collect_hw_knob(0u, 0u, slot, seq_pg, 1u);
    }
#endif
    /* CPL shares: only depth is used (rate/wave unused); Rate/Wave knobs do nothing on these. */
    {
        uint8_t ci;

        for (ci = 0u; ci < (uint8_t)(sizeof dub_cpl_map / sizeof dub_cpl_map[0]); ci++)
            (void)fm_lfo_dests_add(&dub_lfo_dests, dub_cpl_map[ci].cpl);
    }
#if !defined(GENRE_BASS)
    /* SEQ PACE: BASS has no per-lane clock, so the target is not offered there. */
    (void)fm_lfo_dests_add(&dub_lfo_dests, DUB_LFO_SEQ_PACE);
    {
        /* ENV own knobs (genre_vox.h), voice order KICK SNARE HAT BASS CHORD LEAD. */
        static const uint8_t env_ln[DUB_ENVX_LANES] = {0u, 1u, 2u, 4u, 5u, 3u};
        uint8_t li, pi;

        for (li = 0u; li < DUB_ENVX_LANES; li++) {
            for (pi = 0u; pi < DUB_LFO_ENVX_PARAMS; pi++)
                (void)fm_lfo_dests_add(&dub_lfo_dests, (uint8_t)(DUB_LFO_ENVX + pi * DUB_ENVX_LANES + env_ln[li]));
        }
    }
#endif
}

static uint8_t dub_lfo_tgt_step(uint8_t cur, int32_t delta)
{
    return fm_lfo_dests_step(&dub_lfo_dests, cur, delta);
}

static uint8_t dub_lfo_tgt_order_knob_val(uint8_t tgt)
{
    return fm_lfo_dests_knob_val(&dub_lfo_dests, (uint8_t)(tgt % DUB_LFO_TGT_N));
}

static uint8_t dub_lfo_tgt_default(void)
{
    return fm_lfo_dests_first(&dub_lfo_dests, (uint8_t)DUB_LFO_OCT);
}

static uint8_t dub_lfo_tgt_unpack(uint8_t raw)
{
    return fm_lfo_dests_unpack(&dub_lfo_dests, raw, (uint8_t)DUB_LFO_OCT);
}

/* Bank knob touched: its param becomes the LFO dest (preselected when switching to LFO). */
static int dub_lfo_dest_touch(uint8_t t)
{
    return fm_lfo_dests_touch(&dub_lfo_dests, &g.lfo_target, t);
}

static uint8_t *drm_lvl_ptr(dub_params_t *p, uint8_t ix)
{
    return ix == 0u ? &p->kick_boom : ix == 1u ? &p->rim_tone :
           ix == 2u ? &p->hat_lvl : &p->siren_pitch;
}

static const uint8_t *drm_lvl_cptr(const dub_params_t *p, uint8_t ix)
{
    return ix == 0u ? &p->kick_boom : ix == 1u ? &p->rim_tone :
           ix == 2u ? &p->hat_lvl : &p->siren_pitch;
}

static uint8_t lfo_target_for_knob(uint8_t slot)
{
    uint8_t pg = bank_page();
    uint8_t bank = g.bank % BANK_COUNT;
    uint8_t seq_lt;

#if !defined(GENRE_BASS)
    if (gv_lfo_target(slot, &seq_lt))
        return seq_lt;
#endif
    if (dub_seq_knob_lfo_map(slot, &seq_lt))
        return seq_lt;
    return dub_lfo_target_hw(bank, pg, slot, g.seq_page, g.view == VIEW_SEQ ? 1u : 0u, g.sel_drm);
}

static uint8_t dub_lfo_all_bus_tgt(uint8_t tgt)
{
    if (tgt >= DUB_LFO_KTUNE_K && tgt <= DUB_LFO_KTUNE_S)
        return DUB_LFO_KTUNE;
    if (tgt >= DUB_LFO_KDEC_K && tgt <= DUB_LFO_KDEC_S)
        return DUB_LFO_KDEC;
    if (tgt >= DUB_LFO_KDRV_K && tgt <= DUB_LFO_KDRV_S)
        return DUB_LFO_KDRV;
    if (tgt >= DUB_LFO_DCUT_K && tgt <= DUB_LFO_DCUT_S)
        return DUB_LFO_DCUT;
    if (tgt >= DUB_LFO_DATK_K && tgt <= DUB_LFO_DATK_S)
        return DUB_LFO_DATK;
    if (tgt >= DUB_LFO_DLY_K && tgt <= DUB_LFO_DLY_S)
        return DUB_LFO_DLY_D;
    if (tgt >= DUB_LFO_REV_K && tgt <= DUB_LFO_REV_S)
        return DUB_LFO_REV_D;
    if (tgt >= DUB_LFO_DENS_K && tgt <= DUB_LFO_DENS_S)
        return DUB_LFO_DENS;
    return 255u;
}

static int dub_lfo_tgt_modulated(uint8_t tgt)
{
    const fm_lfo_slot_t *sl;

    if (tgt >= DUB_LFO_TGT_N)
        return 0;
    sl = dub_lfo_slot_view(tgt);
    return sl->depth > 0u;
}

enum {
    DUB_KNOB_BAR_H = 86,
    DUB_KNOB_BAR_BLIT_Y = 154,
    DUB_KNOB_MARK_Y = 76,
    /* Beat/bar rail baseline — same strip as ko_draw_lfo_mark / ko_draw_morph_chevron (y±2). */
    DUB_KNOB_PROG_Y = DUB_KNOB_MARK_Y + 2,
    DUB_KNOB_COL_W = 60,
    DUB_KNOB_INSET = 6,
    DUB_KNOB_INNER_W = 48,
    DUB_PROG_BARS = 16u,
    DUB_PROG_STEPS_PER_BAR = 2u
};

static uint32_t dub_step_need_samples(uint8_t step, uint32_t spp, uint32_t sw_amt)
{
    uint32_t need = spp;
    uint32_t sw = sw_amt;

    if (sw > need / 2u)
        sw = need / 2u;
    if (step & 1u)
        need += sw;
    else if (need > sw + 32u)
        need -= sw;
    return need;
}

static uint32_t dub_rail_swing_push(uint32_t spp)
{
    uint32_t amt = g.voice.p.swing ? g.voice.p.swing : g.cfg.p.swing;
    uint32_t sw;

    if (amt < 50u)
        amt = 50u;
    if (amt > 75u)
        amt = 75u;
    sw = (spp * (amt - 50u)) / 50u;
    if (sw > spp / 2u)
        sw = spp / 2u;
    return sw;
}

static int dub_cpl_knob_active(uint8_t tgt)
{
    uint8_t ci;

    for (ci = 0u; ci < (uint8_t)(sizeof dub_cpl_map / sizeof dub_cpl_map[0]); ci++) {
        const dub_cpl_map_t *m = &dub_cpl_map[ci];

        if (m->dst != tgt || g.lfo_slots[m->cpl].depth == 0u)
            continue;
        if (m->src == DUB_LFO_M_MOTION) {
            if (dub_eff_macro(8u) > 0u)
                return 1;
        } else if (m->src == DUB_LFO_M_FLUX) {
            if (dub_eff_flux_engine() > 0u)
                return 1;
        } else if (g.lfo_slots[m->src].depth > 0u) {
            return 1;
        }
    }
    return 0;
}

static int dub_lfo_knob_modulated(uint8_t slot, const dub_params_t *p)
{
    uint8_t tgt;
    uint8_t all;

    (void)p;
    if (!g.seq_plock_hold && g.view != VIEW_SEQ && g.bank == BANK_DRUM &&
        bank_page() == 0u && slot == 1u)
        return 0;
#if !defined(GENRE_BASS)
    if (!g.seq_plock_hold && g.view != VIEW_SEQ && g.bank == BANK_CHORD &&
        bank_page() == 0u && slot == 0u)
        return 0;
#endif
    tgt = lfo_target_for_knob(slot);
    if (dub_lfo_tgt_modulated(tgt))
        return 1;
    if (dub_cpl_knob_active(tgt))
        return 1;
    all = dub_lfo_all_bus_tgt(tgt);
    if (all < DUB_LFO_TGT_N && all != tgt && dub_lfo_tgt_modulated(all))
        return 1;
    return 0;
}

static int dub_home_macro_knob_ix(uint8_t slot, uint8_t *ix_out)
{
    uint8_t pg;

    if (g.view == VIEW_SEQ || g.bank != BANK_HOME)
        return 0;
    pg = bank_page();
    if (pg <= 1u) {
        *ix_out = pg == 0u ? slot : (uint8_t)(4u + slot);
        return 1;
    }
    if (pg == 2u && slot <= 1u) {
#if defined(GENRE_BASS)
        /* BASS: GATE / DENS live on macros 4 / 6 (BASS_PERF_MACRO_IX), not MOTION / FLUX. */
        *ix_out = BASS_PERF_MACRO_IX[slot];
#else
        *ix_out = (uint8_t)(8u + slot);
#endif
        return 1;
    }
    return 0;
}

static int dub_knob_morph_u8(uint8_t slot, uint8_t *from_u8, uint8_t *to_u8)
{
    const dub_params_t *mf = &g.morph_from;
    const dub_params_t *to = &g.cfg.p;
    uint8_t pg = bank_page();
    uint8_t di = sel_drm_ix();
    uint8_t mix;

    if (dub_home_macro_knob_ix(slot, &mix)) {
        if (g.macro_morph_pos >= DUB_MORPH_STEPS)
            return 0;
        *from_u8 = g.macro_from[mix];
        *to_u8 = dub_raw_macro(mix);
        return *from_u8 != *to_u8;
    }
    if (g.view == VIEW_SEQ && g.seq_page == 5u && !g.seq_plock_hold) {
        if (!g.cue_on)
            return 0;
        if (slot == 0u) {
            *from_u8 = g.cue_root ? g.cue_root : to->root;
            *to_u8 = to->root;
        } else if (slot == 1u) {
            /* Scale: discrete steps; dual label in draw_knob_bar, no chevron. */
            return 0;
        } else if (slot == 2u) {
            *from_u8 = g.cue_swing ? g.cue_swing : to->swing;
            *to_u8 = to->swing;
        } else {
            *from_u8 = g.cue_bpm ? g.cue_bpm : to->bpm;
            *to_u8 = to->bpm;
        }
        return *from_u8 != *to_u8;
    }
    if (g.morph_pos >= DUB_MORPH_STEPS || g.view == VIEW_SEQ || g.bank == BANK_HOME ||
        g.bank == BANK_LFO)
        return 0;
    if (g.bank == BANK_CHORD && pg == 0u && slot == 0u)
        return 0;
    if (g.bank == BANK_BASS && pg == 0u && slot == 0u)
        return 0;
    if (g.bank == BANK_DRUM && pg == 0u && slot <= 1u)
        return 0;

    if (g.bank == BANK_CHORD) {
        if (pg == 0u)
            return slot == 1 ? (*from_u8 = mf->cutoff_c, *to_u8 = to->cutoff_c, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->atk_c, *to_u8 = to->atk_c, *from_u8 != *to_u8) :
                   (*from_u8 = mf->decay_c, *to_u8 = to->decay_c, *from_u8 != *to_u8);
        if (pg == 1u)
            return slot == 0 ? (*from_u8 = mf->timbre, *to_u8 = to->timbre, *from_u8 != *to_u8) :
                   slot == 1 ? (*from_u8 = mf->reso_mode, *to_u8 = to->reso_mode, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->env_filt, *to_u8 = to->env_filt, *from_u8 != *to_u8) :
                   (*from_u8 = mf->glide_c, *to_u8 = to->glide_c, *from_u8 != *to_u8);
        return slot == 0 ? (*from_u8 = mf->lvl_c, *to_u8 = to->lvl_c, *from_u8 != *to_u8) :
               slot == 1 ? (*from_u8 = mf->pan[1], *to_u8 = to->pan[1], *from_u8 != *to_u8) :
               slot == 2 ? (*from_u8 = mf->dly_c, *to_u8 = to->dly_c, *from_u8 != *to_u8) :
               (*from_u8 = mf->rev_c, *to_u8 = to->rev_c, *from_u8 != *to_u8);
    }
    if (g.bank == BANK_BASS) {
#if defined(GENRE_BASS)
        if (pg == 0u)
            return slot == 2 ? (*from_u8 = mf->sub_oct, *to_u8 = to->sub_oct, *from_u8 != *to_u8) :
                   slot == 3 ? (*from_u8 = mf->pan[0], *to_u8 = to->pan[0], *from_u8 != *to_u8) :
                   0;
        if (pg == 1u)
            return slot == 0 ? (*from_u8 = mf->lvl_b, *to_u8 = to->lvl_b, *from_u8 != *to_u8) :
                   slot == 1 ? (*from_u8 = mf->drive, *to_u8 = to->drive, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->dly_b, *to_u8 = to->dly_b, *from_u8 != *to_u8) :
                   (*from_u8 = mf->rev_b, *to_u8 = to->rev_b, *from_u8 != *to_u8);
        if (pg == 2u && slot == 0u)
            return (*from_u8 = mf->reso_mode, *to_u8 = to->reso_mode, *from_u8 != *to_u8);
        if (pg == 2u && slot == 1u)
            return (*from_u8 = mf->cutoff_b, *to_u8 = to->cutoff_b, *from_u8 != *to_u8);
        return 0;
#else
        if (pg == 0u)
            return slot == 1 ? (*from_u8 = mf->cutoff_b, *to_u8 = to->cutoff_b, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->atk_b, *to_u8 = to->atk_b, *from_u8 != *to_u8) :
                   (*from_u8 = mf->decay_b, *to_u8 = to->decay_b, *from_u8 != *to_u8);
        if (pg == 1u)
            return slot == 0 ? (*from_u8 = mf->sub_oct, *to_u8 = to->sub_oct, *from_u8 != *to_u8) :
                   slot == 1 ? (*from_u8 = mf->pump, *to_u8 = to->pump, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->drive, *to_u8 = to->drive, *from_u8 != *to_u8) :
                   (*from_u8 = mf->wobble, *to_u8 = to->wobble, *from_u8 != *to_u8);
        return slot == 0 ? (*from_u8 = mf->lvl_b, *to_u8 = to->lvl_b, *from_u8 != *to_u8) :
               slot == 1 ? (*from_u8 = mf->pan[0], *to_u8 = to->pan[0], *from_u8 != *to_u8) :
               slot == 2 ? (*from_u8 = mf->dly_b, *to_u8 = to->dly_b, *from_u8 != *to_u8) :
               (*from_u8 = mf->rev_b, *to_u8 = to->rev_b, *from_u8 != *to_u8);
#endif
    }
    if (g.bank == BANK_DRUM) {
        if (pg == 0u) {
            if (slot == 2u) {
                *from_u8 = *drm_lvl_cptr(mf, di);
                *to_u8 = *drm_lvl_cptr(to, di);
                return *from_u8 != *to_u8;
            }
            *from_u8 = mf->drm_tune[di] ? mf->drm_tune[di] : 64u;
            *to_u8 = to->drm_tune[di] ? to->drm_tune[di] : 64u;
            return *from_u8 != *to_u8;
        }
        if (pg == 1u)
            return slot == 0 ? (*from_u8 = mf->drm_dens[di], *to_u8 = to->drm_dens[di], *from_u8 != *to_u8) :
                   slot == 1 ? (*from_u8 = mf->drm_cut[di] ? mf->drm_cut[di] : 64u,
                                 *to_u8 = to->drm_cut[di] ? to->drm_cut[di] : 64u, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->drm_atk[di], *to_u8 = to->drm_atk[di], *from_u8 != *to_u8) :
                   (*from_u8 = mf->drm_dec[di] ? mf->drm_dec[di] : 64u,
                    *to_u8 = to->drm_dec[di] ? to->drm_dec[di] : 64u, *from_u8 != *to_u8);
        if (pg == 2u) {
            if (slot == 0u) {
                if (sel_drm_is_siren()) {
                    *from_u8 = mf->siren_rate;
                    *to_u8 = to->siren_rate;
                } else {
                    *from_u8 = mf->drm_drv[di];
                    *to_u8 = to->drm_drv[di];
                }
                return *from_u8 != *to_u8;
            }
            if (slot == 1u) {
                *from_u8 = mf->drm_pan[di];
                *to_u8 = to->drm_pan[di];
                return *from_u8 != *to_u8;
            }
            if (slot == 2u) {
                *from_u8 = mf->drm_dly[di];
                *to_u8 = to->drm_dly[di];
                return *from_u8 != *to_u8;
            }
            *from_u8 = mf->drm_rev[di];
            *to_u8 = to->drm_rev[di];
            return *from_u8 != *to_u8;
        }
    }
    if (g.bank == BANK_FX) {
        if (pg == 0u)
            return slot == 0 ? (*from_u8 = mf->dly_lvl, *to_u8 = to->dly_lvl, *from_u8 != *to_u8) :
                   slot == 1 ? (*from_u8 = mf->dly_time, *to_u8 = to->dly_time, *from_u8 != *to_u8) :
                   slot == 2 ? (*from_u8 = mf->feedback, *to_u8 = to->feedback, *from_u8 != *to_u8) :
                   (*from_u8 = mf->loop_filt, *to_u8 = to->loop_filt, *from_u8 != *to_u8);
        if (pg == 1u)
            return slot == 0 ? (*from_u8 = mf->wow, *to_u8 = to->wow, *from_u8 != *to_u8) :
                   slot == 1 ? 0 :
                   slot == 2 ? (*from_u8 = mf->tank, *to_u8 = to->tank, *from_u8 != *to_u8) :
                   (*from_u8 = mf->isolator, *to_u8 = to->isolator, *from_u8 != *to_u8);
        return slot == 0 ? (*from_u8 = mf->rev_mix, *to_u8 = to->rev_mix, *from_u8 != *to_u8) :
               slot == 1 ? (*from_u8 = mf->rev_damp, *to_u8 = to->rev_damp, *from_u8 != *to_u8) :
               slot == 2 ? (*from_u8 = mf->rev_tail, *to_u8 = to->rev_tail, *from_u8 != *to_u8) :
               (*from_u8 = mf->dly_rev, *to_u8 = to->dly_rev, *from_u8 != *to_u8);
    }
    return 0;
}

static inline void dub_edit_knob(int32_t steps)
{
    if (steps != 0) {
        g.edit_press_ms = 0u;
        g.edit_used = 1u;
#if defined(GENRE_BASS)
        if (g.view == VIEW_SEQ)
            bass_edit_lock_step(bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages,
                                                             g.step));
#endif
    }
}

static inline void dub_edit_u8(int32_t steps, uint8_t edit, uint8_t *v, uint8_t def, uint8_t max)
{
    if (!edit || steps == 0)
        return;
    dub_edit_knob(steps);
    ko_edit_snap(steps, v, def, max);
}

#if defined(GENRE_BASS)
static void bass_seq_select_bar(int32_t bar)
{
    uint8_t bars = fm_seq_pages_clamp(g.cfg.seq_pages);
    uint8_t col;
    char msg[8];

    if (bars > 8u)
        bars = 8u;
    if (bars < 1u)
        bars = 1u;
    while (bar < 0)
        bar += (int32_t)bars;
    bar %= (int32_t)bars;
    col = (uint8_t)(bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages,
                                                 g.step) %
                    FM_SEQ_PAGE_STEPS);
    g.seq_view_page = (uint8_t)bar;
    g.seq_follow = 0u;
    g.seq_plock_step = (uint8_t)((uint32_t)bar * FM_SEQ_PAGE_STEPS + col);
    copy_str(msg, "BAR ");
    u8_dec(msg + 4, (uint8_t)((uint32_t)bar + 1u));
    seq_flash_knob(msg);
    g.ui_dirty = 1u;
}

static void bass_seq_toggle_step(uint8_t st)
{
    uint8_t bit = (uint8_t)(1u << 4u);

    if (st >= DUB_SEQ_STEPS)
        return;
    seq_materialize_all();
    if (g.cfg.seq[st].gate & bit)
        g.cfg.seq[st].gate &= (uint8_t)~bit;
    else {
        g.cfg.seq[st].gate |= bit;
        g.cfg.seq[st].force = 1u;
    }
    bass_edit_lock_step(st);
    mark_dirty();
    g.ui_dirty = 1u;
}

static int bass_seq_lane_base_on(uint8_t st)
{
    if (st >= DUB_SEQ_STEPS)
        return 0;
    if ((g.cfg.seq[st].gate & (1u << 4u)) != 0u)
        return 1;
    if (g.cfg.user_song || g.cfg.bass.step_x[st].lock)
        return 0;
    return bass_line_lead_step_bit(genre_clamp_preset(g.cfg.p.plate), st) ? 1 : 0;
}

/* Grid, LEDs and playback share this roll so every bar shows what it will play. */
static int bass_seq_vary_on(int base_on, uint8_t st)
{
    return fm_seq_vary_hit_phrase(base_on, dub_eff_vary(4u), st, 4u, g.vary_seed);
}

static int bass_seq_lane_step_on(uint8_t st)
{
    uint8_t mst;

    if (st >= DUB_SEQ_STEPS)
        return 0;
    /* Sole voice: length is a cutoff on the linear phrase, not a 6-lane wrap. */
    if (st >= seq_lane_len_v(0u, 4u))
        return 0;
    mst = st;
    if (!bass_seq_vary_on(bass_seq_lane_base_on(mst), mst))
        return 0;
    if (g.step_prob[4u][mst] > 0u && !fm_seq_cond_pass(g.step_prob[4u][mst], mst, g.bar, 4u, 0))
        return 0;
    if (!fm_seq_trig_pass(dub_eff_trig_raw(4u), mst, g.bar, 4u))
        return 0;
    return 1;
}

static void bass_live_from_knob(uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    bass_cfg_ext_t *b = &g.cfg.bass;
    char msg[8];

    if (!steps && !d)
        return;
    if (edit && steps) {
        if (slot == 0u)
            b->live_hold = (uint8_t)(b->live_hold & (uint8_t)~BASS_LIVE_HOLD_GATE);
        else if (slot == 1u)
            b->live_hold = (uint8_t)(b->live_hold & (uint8_t)~BASS_LIVE_HOLD_ACCENT);
        else if (slot == 2u)
            b->live_hold = (uint8_t)(b->live_hold & (uint8_t)~BASS_LIVE_HOLD_SLIDE);
        else
            b->live_hold = (uint8_t)(b->live_hold & (uint8_t)~BASS_LIVE_HOLD_RATCHET);
        seq_flash_knob("STEP");
        return;
    }
    if (slot == 0u) {
        adj_u8(&b->live_gate, d ? d : steps, 0, BASS_GATE_TIE);
        b->live_hold = (uint8_t)(b->live_hold | BASS_LIVE_HOLD_GATE);
        copy_str(msg, "GATE ");
        u8_dec(msg + 5, b->live_gate);
        seq_flash_knob(msg);
    } else if (slot == 1u) {
        if (!steps)
            return;
        b->live_accent ^= 1u;
        b->live_hold = (uint8_t)(b->live_hold | BASS_LIVE_HOLD_ACCENT);
        seq_flash_knob(b->live_accent ? "ACCENT" : "ACC OFF");
    } else if (slot == 2u) {
        if (!steps)
            return;
        b->live_slide ^= 1u;
        b->live_hold = (uint8_t)(b->live_hold | BASS_LIVE_HOLD_SLIDE);
        seq_flash_knob(b->live_slide ? "SLIDE" : "SL OFF");
    } else {
        adj_u8(&b->live_ratchet, d ? d : steps, BASS_RATCHET_MIN, BASS_RATCHET_MAX);
        b->live_hold = (uint8_t)(b->live_hold | BASS_LIVE_HOLD_RATCHET);
        copy_str(msg, "RATCH ");
        u8_dec(msg + 6, b->live_ratchet);
        seq_flash_knob(msg);
    }
}

static void bass_knob_adjust_plock(uint8_t st, uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    if (steps != 0)
        g.seq_follow = 0;
    g.seq_plock_knob = 1u;
    if (slot == 0u) {
        if (edit)
            g.step_vel[4u][st] = steps < 0 ? 96u : 127u;
        else {
            if (!g.step_vel[4u][st])
                g.step_vel[4u][st] = bass_step_lv_vel(g.cfg.bass.step_x[st].accent);
            adj_u8(&g.step_vel[4u][st], d, 1, 127);
        }
    } else if (slot == 1u) {
        if (edit)
            g.step_prob[4u][st] = steps < 0 ? FM_SEQ_COND_MAX : 0u;
        else if (steps != 0) {
            int32_t v = (int32_t)g.step_prob[4u][st] + (steps > 0 ? -1 : 1);

            if (v < 0)
                v = 0;
            if (v > (int32_t)FM_SEQ_COND_MAX)
                v = (int32_t)FM_SEQ_COND_MAX;
            g.step_prob[4u][st] = (uint8_t)v;
        }
    } else if (slot == 2u) {
        int32_t v = edit ? (steps < 0 ? 0 : (int32_t)FM_SEQ_MICRO_MAX)
                         : (int32_t)g.step_micro[4u][st] + (steps != 0 ? steps : d);

        if (v < 0)
            v = 0;
        g.step_micro[4u][st] = fm_seq_micro_clamp(v);
    } else if (edit) {
        g.step_tune[4u][st] = steps < 0 ? 0 : 24;
    } else if (steps != 0) {
        int32_t v = (int32_t)g.step_tune[4u][st] + steps;

        if (v < -24)
            v = -24;
        if (v > 24)
            v = 24;
        g.step_tune[4u][st] = (int8_t)v;
    }
    bass_edit_lock_step(st);
    mark_dirty();
}

static void bass_knob_adjust_seq(uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    uint8_t st = bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages, g.step);
    bass_step_x_t *sx = &g.cfg.bass.step_x[st];
    uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);
    const uint8_t ln = 4u;

    if (g.seq_plock_hold) {
        if (!g.seq_plock_bank) {
            bass_knob_adjust_plock(st, slot, steps, d, edit);
            return;
        }
    } else if (g.seq_page == 1u) {
        seq_knob_lane_page1(ln, slot, steps, edit, d);
        mark_dirty();
        return;
    } else if (g.seq_page == 2u) {
        if (slot == 0u)
            seq_lane_len_edit(vp, ln, steps, edit, d);
        else if (slot == 1u)
            seq_knob_p3_state(ln, steps, edit);
        else if (slot == 3u)
            seq_knob_vary(ln, steps, edit, d);
        else
            return;
        mark_dirty();
        return;
    }
    {
        if (slot == 0u) {
            dub_edit_u8(steps, edit, &sx->gate, 4u, BASS_GATE_TIE);
            if (!edit || steps == 0)
                adj_u8(&sx->gate, d, 0, BASS_GATE_TIE);
        } else if (slot == 1u || slot == 2u) {
            uint8_t *lv = slot == 1u ? &sx->accent : &sx->slide;

            *lv = bass_step_lv_clamp(*lv);
            if (edit && steps != 0)
                *lv = steps < 0 ? 0u : BASS_STEP_LV_FULL;
            else if (steps != 0)
                adj_u8(lv, steps < 0 ? -1 : 1, 0, BASS_STEP_LV_FULL);
        } else {
            if (edit && steps != 0)
                sx->ratchet = steps < 0 ? BASS_RATCHET_MIN : BASS_RATCHET_MAX;
            else if (steps != 0)
                adj_u8(&sx->ratchet, steps, BASS_RATCHET_MIN, BASS_RATCHET_MAX);
            else
                adj_u8(&sx->ratchet, d, BASS_RATCHET_MIN, BASS_RATCHET_MAX);
            sx->ratchet = bass_step_clamp_ratchet(sx->ratchet);
        }
    }
    bass_edit_lock_step(st);
    mark_dirty();
}
#endif

static void dub_lfo_touch_oct(int32_t steps)
{
    uint8_t lfo_held = (fm1_in.buttons & (1u << BTN_LFO)) != 0u;
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;

    if (g.bank == BANK_LFO || g.view == VIEW_SEQ)
        return;
    (void)dub_lfo_dest_touch(DUB_LFO_OCT);
    if (lfo_held) {
        if (edit)
            g.lfo_slots[DUB_LFO_OCT].depth = steps < 0 ? 0u : 127u;
        else
            adj_u8(&g.lfo_slots[DUB_LFO_OCT].depth, steps, 0, 127);
        mark_dirty();
    }
}

static int seq_pattern_song_avail(void)
{
    return !g.song_setup_latch;
}

static void seq_knob_euclid_hits(uint8_t ln, int32_t steps, int edit)
{
    int song_avail = seq_pattern_song_avail();

    ln %= DUB_SEQ_LANES;
    if (edit) {
        if (steps < 0) {
            if (song_avail) {
                if (g.euclid_hits[ln] != FM_SEQ_EUCLID_SONG)
                    seq_lane_enter_song(ln);
                else
                    dub_lane_restore_plate(ln);
            }
        } else {
            if (g.euclid_hits[ln] == FM_SEQ_EUCLID_FRESH)
                seq_lane_wipe_step_plocks(ln);
            g.euclid_hits[ln] = 16u;
            seq_apply_euclid(ln);
        }
    } else if (steps != 0) {
        uint8_t oh = g.euclid_hits[ln];
        uint8_t nh = fm_seq_euclid_step(oh, steps, 0, g.pattern_origin[ln], song_avail);

        if (nh == FM_SEQ_EUCLID_EDITED) {
            if (oh != FM_SEQ_EUCLID_EDITED)
                fm_seq_lane_pattern_mark_edited(&g.euclid_hits[ln], &g.pattern_origin[ln]);
        } else {
            if (oh == FM_SEQ_EUCLID_EDITED && steps > 0 &&
                g.pattern_origin[ln] != FM_SEQ_PATTERN_ORIGIN_NONE)
                g.pattern_origin[ln] = FM_SEQ_PATTERN_ORIGIN_NONE;
            if (nh == FM_SEQ_EUCLID_FRESH) {
                dub_seq_fresh_lane(ln);
                g.euclid_hits[ln] = FM_SEQ_EUCLID_FRESH;
            } else if (nh == FM_SEQ_EUCLID_SONG) {
                if (oh != FM_SEQ_EUCLID_SONG)
                    seq_lane_enter_song(ln);
                else
                    dub_lane_restore_plate(ln);
            } else {
                if (oh == FM_SEQ_EUCLID_FRESH && fm_seq_euclid_is_pattern(nh))
                    seq_lane_wipe_step_plocks(ln);
                g.euclid_hits[ln] = nh;
                seq_apply_euclid(ln);
            }
        }
    }
}

static void seq_knob_euclid_rot(uint8_t ln, int32_t steps, int edit)
{
    int8_t old_rot;

    ln %= DUB_SEQ_LANES;
    if (edit) {
        old_rot = g.euclid_rot[ln];
        g.euclid_rot[ln] = steps < 0 ? 0 : 15;
        if (g.euclid_rot[ln] != old_rot) {
            seq_apply_euclid(ln);
            seq_lane_pattern_mark_edited(ln);
        }
    } else {
        int32_t step1 = steps > 0 ? 1 : -1;
        int32_t nr = (int32_t)fm_seq_euclid_rot_norm(g.euclid_rot[ln]) + step1;

        g.euclid_rot[ln] = (int8_t)fm_seq_euclid_rot_norm(nr);
        seq_rotate_lane(ln, step1);
    }
}

static void seq_knob_vary(uint8_t ln, int32_t steps, int edit, int d)
{
    int32_t v;
    uint8_t prev;

    ln %= DUB_SEQ_LANES;
    prev = g.vary_lane[ln];
    if (edit)
        v = steps < 0 ? 0 : 127;
    else {
        v = (int32_t)g.vary_lane[ln] + d * 3;
        if (v < 0)
            v = 0;
        if (v > 127)
            v = 127;
    }
    g.vary_lane[ln] = (uint8_t)v;
    if (g.vary_lane[ln] != prev)
        seq_lane_pattern_mark_edited(ln);
}

static void seq_knob_lane_page1(uint8_t ln, uint8_t slot, int32_t steps, int edit, int d)
{
    int8_t old_i8;

    ln %= DUB_SEQ_LANES;
    slot = fm_seq_p1_fn(slot);
    if (slot == 1) {
        old_i8 = g.lane_trig[ln];
        if (edit)
            g.lane_trig[ln] = steps < 0 ? 0 : (int8_t)-100;
        else
            g.lane_trig[ln] = fm_seq_trig_adj(g.lane_trig[ln], d, 0);
        if (g.lane_trig[ln] != old_i8)
            seq_lane_pattern_mark_edited(ln);
        return;
    }
    {
        int8_t *fp = slot == 0 ? &g.lane_vel[ln] :
                     slot == 2 ? &g.lane_gate[ln] : &g.lane_tune[ln];
        int32_t lim = slot == 3 ? 24 : 50;

        old_i8 = *fp;
        if (edit)
            *fp = steps < 0 ? 0 : (int8_t)lim;
        else {
            int32_t v = (int32_t)*fp + (slot == 3 ? steps : d);

            if (v < -lim)
                v = -lim;
            if (v > lim)
                v = lim;
            *fp = (int8_t)v;
        }
        if (*fp != old_i8)
            seq_lane_pattern_mark_edited(ln);
    }
}

static void seq_knob_prob_all(uint8_t slot, int32_t steps, int edit, int d)
{
    uint32_t i;
    int32_t delta;

    slot = fm_seq_p1_fn(slot);
    delta = edit && steps != 0 ? (steps < 0 ? -1 : 1) : (slot == 3u ? steps : d);

    for (i = 0u; i < DUB_SEQ_LANES; i++) {
        uint8_t ln = (uint8_t)i;
        int8_t old_i8;

        if (slot == 1u) {
            old_i8 = g.lane_trig[ln];
            g.lane_trig[ln] = fm_seq_trig_adj(g.lane_trig[ln], (int)delta, 0);
            if (g.lane_trig[ln] != old_i8)
                seq_lane_pattern_mark_edited(ln);
        } else {
            int8_t *fp = slot == 0u ? &g.lane_vel[ln] :
                         slot == 2u ? &g.lane_gate[ln] : &g.lane_tune[ln];
            int32_t lim = slot == 3u ? 24 : 50;
            int32_t v;

            old_i8 = *fp;
            v = (int32_t)*fp + delta;
            if (v < -lim)
                v = -lim;
            if (v > lim)
                v = lim;
            *fp = (int8_t)v;
            if (*fp != old_i8)
                seq_lane_pattern_mark_edited(ln);
        }
    }
}

static void seq_knob_p3_state(uint8_t ln, int32_t steps, int edit)
{
    uint8_t pos;
    uint8_t prev = 0u;

    if (edit)
        pos = steps < 0 ? 1u : 2u;
    else if (steps != 0) {
        int32_t p = (int32_t)dub_lane_p3_state_pos(ln) + (steps > 0 ? 1 : -1);

        if (p < 0)
            p = 0;
        if (p > 2)
            p = 2;
        pos = (uint8_t)p;
    } else
        return;

    if (seq_lane_is_all()) {
        uint32_t i;

        for (i = 0u; i < DUB_SEQ_LANES; i++) {
            prev = dub_lane_p3_state_pos((uint8_t)i);
            dub_lane_apply_p3_state((uint8_t)i, pos);
            if (dub_lane_p3_state_pos((uint8_t)i) != prev)
                seq_lane_pattern_mark_edited((uint8_t)i);
        }
    } else {
        prev = dub_lane_p3_state_pos(ln);
        dub_lane_apply_p3_state(ln, pos);
        if (dub_lane_p3_state_pos(ln) != prev)
            seq_lane_pattern_mark_edited(ln);
    }
}

/* SEQ page 2 slot 3: PACE, 5 discrete steps, clamped (no wrap). Same edit rule as NUDGE: EDIT + turn = min/max. */
/* Per-lane like NUDGE/LENGTH: ALL-lanes edits every lane, otherwise the selected lane only. */
static void seq_knob_pace(uint8_t sl, int32_t steps, int edit, int d)
{
    uint8_t all = (uint8_t)seq_lane_is_all();
    uint8_t base = all ? (uint8_t)(g.seq_lane % DUB_SEQ_LANES) : (uint8_t)(sl % DUB_SEQ_LANES);
    int32_t v = (int32_t)g.cfg.seq_pace[base];
    uint8_t i;

    if (edit && steps != 0)
        v = steps < 0 ? 0 : (int32_t)FM_SEQ_PACE_N - 1;
    else
        v += steps != 0 ? steps : d;
    if (v < 0)
        v = 0;
    if (v > (int32_t)FM_SEQ_PACE_N - 1)
        v = (int32_t)FM_SEQ_PACE_N - 1;
    for (i = 0u; i < DUB_SEQ_LANES; i++) {
        if (!all && i != base)
            continue;
        if (g.cfg.seq_pace[i] != (uint8_t)v) {
            g.cfg.seq_pace[i] = (uint8_t)v;
            dub_lane_resync(i);
        }
    }
    dub_lane_spp_refresh();
}

#if defined(GENRE_BASS)
static void bass_cycle_u8(uint8_t *v, int32_t steps, uint8_t lo, uint8_t hi)
{
    int32_t n = (int32_t)*v + (steps > 0 ? 1 : -1);

    if (n < (int32_t)lo)
        n = (int32_t)hi;
    if (n > (int32_t)hi)
        n = (int32_t)lo;
    *v = (uint8_t)n;
}

static void bass_adj_mod_amount(int8_t *amt, int32_t d, uint8_t edit, int32_t steps)
{
    if (edit && steps != 0)
        *amt = steps < 0 ? (int8_t)-127 : (int8_t)127;
    else {
        int32_t v = (int32_t)*amt + d;

        if (v < -127)
            v = -127;
        if (v > 127)
            v = 127;
        *amt = (int8_t)v;
    }
}

static void bass_cycle_mod_dest(uint8_t *dest, int32_t steps)
{
    int32_t dir = steps > 0 ? 1 : -1;
    int32_t v = (int32_t)*dest + dir;

    while (v < 0)
        v += BASS_MOD_N;
    v %= (int32_t)BASS_MOD_N;
    /* MODEL has no runtime effect (model jumps are not a modulation); not offered. */
    if (v == (int32_t)BASS_MOD_MODEL)
        v = (v + dir + (int32_t)BASS_MOD_N) % (int32_t)BASS_MOD_N;
    *dest = (uint8_t)v;
}

static void bass_knob_adjust_home(uint8_t pg, uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    dub_params_t *p = &g.cfg.p;

    if (pg == 0u) {
        uint8_t *q = slot == 0 ? &p->cutoff_b : slot == 1 ? &p->reso_mode : slot == 2 ? &p->drive : &p->dly_b;

        dub_edit_u8(steps, edit, q, 0u, 127u);
        if (!edit || steps == 0)
            adj_u8_morph(q, d, 0, 127);
        return;
    }
    if (pg == 1u) {
        if (slot == 0u) {
            dub_edit_u8(steps, edit, &g.cfg.bass.glide_time, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&g.cfg.bass.glide_time, d, 0, 127);
        } else {
            uint8_t mi = (uint8_t)(slot - 1u);
            uint8_t *mp = dub_macro_ptr(mi);

            if (mp) {
                dub_edit_u8(steps, edit, mp, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(mp, d, 0, 127);
                dub_macro_knob_sync(mi);
            }
        }
        return;
    }
    /* p2: PERFORM knobs; EDIT held = MAC routing editor (MACRO / SLOT / DEST / AMOUNT). */
    if (edit) {
        uint8_t *sel = &g.bass_rt.home_macro_sel;
        uint8_t mac = (uint8_t)((*sel / BASS_MOD_SLOTS_PER_MAP) % BASS_MOD_MACRO_N);
        uint8_t si = (uint8_t)(*sel % BASS_MOD_SLOTS_PER_MAP);
        bass_mod_slot_t *sl = bass_mod_route_slot(&g.cfg.bass.mods, *sel);

        if (slot == 0u)
            bass_cycle_u8(&mac, steps != 0 ? steps : d, 0u, (uint8_t)(BASS_MOD_MACRO_N - 1u));
        else if (slot == 1u)
            bass_cycle_u8(&si, steps != 0 ? steps : d, 0u, (uint8_t)(BASS_MOD_SLOTS_PER_MAP - 1u));
        else if (slot == 2u)
            bass_cycle_mod_dest(&sl->dest, steps != 0 ? steps : d);
        else
            bass_adj_mod_amount(&sl->amount, d, 0u, steps);
        *sel = (uint8_t)(mac * BASS_MOD_SLOTS_PER_MAP + si);
        return;
    }
    {
        uint8_t *mp = bass_perf_ptr(slot);

        if (!mp)
            return;
        adj_u8(mp, d, 0, 127);
        dub_macro_knob_sync(BASS_PERF_MACRO_IX[slot % BASS_PERF_N]);
    }
}

static void bass_knob_adjust_env(uint8_t pg, uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    bass_dahdsr_cfg_t *env = &g.cfg.bass.env;
    bass_mod_slot_t *esl;
    uint8_t esi;

    if (pg == 0u || (pg == 1u && slot < 2u)) {
        uint8_t *q = pg == 1u ? (slot == 0 ? &env->t.delay : &env->t.hold) :
                     slot == 0 ? &env->t.attack : slot == 1 ? &env->t.decay :
                     slot == 2 ? &env->t.sustain : &env->t.release;

        dub_edit_u8(steps, edit, q, 0u, 127u);
        if (!edit || steps == 0)
            adj_u8(q, d, 0, 127);
        return;
    }
    if (pg == 1u && slot == 2u) {
        uint8_t c = bass_env_shape_curve(g.cfg.bass_env_shape);

        dub_edit_u8(steps, edit, &c, BASS_DAHDSR_CURVE_LIN, 127u);
        if (!edit || steps == 0)
            adj_u8(&c, d, 0, 127);
        g.cfg.bass_env_shape = bass_env_shape_pack(c);
        return;
    }
    if ((pg == 1u && slot == 3u) || (pg == 2u && slot == 0u)) {
        uint8_t *q = pg == 1u ? &env->pol.retrigger : &env->pol.invert;

        if (edit && steps != 0)
            *q = steps < 0 ? 0u : 1u;
        else if (steps != 0)
            *q ^= 1u;
        return;
    }
    esi = g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP;
    esl = &g.cfg.bass.mods.env[esi];
    if (pg >= FM_ENV_KIT_PAGES) {
        if (slot == 0u) {
            dub_edit_u8(steps, edit, &esl->curve, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&esl->curve, d, 0, 127);
        }
        return;
    }
    if (slot == 1u) {
        dub_edit_u8(steps, edit, &env->pol.accent_scale, 0u, 127u);
        if (!edit || steps == 0)
            adj_u8(&env->pol.accent_scale, d, 0, 127);
    } else if (slot == 2u) {
        if (steps == 0 && d == 0)
            return;
        if (edit)
            bass_cycle_u8(&g.bass_rt.env_edit_slot, steps != 0 ? steps : d, 0u,
                          (uint8_t)(BASS_MOD_SLOTS_PER_MAP - 1u));
        else
            bass_cycle_mod_dest(&esl->dest, steps != 0 ? steps : d);
    } else
        bass_adj_mod_amount(&esl->amount, d, edit, steps);
}
#endif

#if defined(GENRE_BASS)
static void bass_groove_adj_wrap(uint8_t *v, int32_t steps, uint8_t n)
{
    int32_t k = (int32_t)*v + steps;

    while (k < 0)
        k += n;
    *v = (uint8_t)(k % n);
}

static void bass_groove_knob_adjust(uint8_t pg, uint8_t slot, int32_t steps, int32_t d, uint8_t edit)
{
    dub_params_t *p = &g.cfg.p;
    bass_cfg_ext_t *b = &g.cfg.bass;

    if (pg == 0u) {
        if (slot == 0u) {
            uint8_t k = (uint8_t)(p->root % 12u);

            if (edit && steps != 0)
                k = steps < 0 ? 0u : 11u;
            else if (!edit)
                bass_groove_adj_wrap(&k, steps, 12u);
            p->root = (uint8_t)((p->root / 12u) * 12u + k);
            dub_morph_sync_field(&p->root);
        } else if (slot == 1u) {
            if (edit && steps != 0)
                p->scale = steps < 0 ? SCALE_LO : 8u;
            else if (!edit)
                p->scale = scale_step(p->scale, steps);
            dub_morph_sync_field(&p->scale);
        } else if (slot == 2u) {
            dub_edit_u8(steps, edit, &p->swing, 50u, 75u);
            if (!edit || steps == 0)
                adj_u8_morph(&p->swing, d, 50, 75);
            else
                dub_morph_sync_field(&p->swing);
        } else {
            dub_edit_u8(steps, edit, &p->bpm, 40u, 180u);
            if (!edit || steps == 0)
                adj_u8_morph(&p->bpm, d, 40, 180);
            else
                dub_morph_sync_field(&p->bpm);
            transport_set_bpm();
        }
        mark_dirty();
        return;
    }
    if (slot == 0u) {
        dub_edit_u8(steps, edit, &b->midi_ch, 1u, 16u);
        if (!edit || steps == 0)
            adj_u8(&b->midi_ch, steps, 1, 16);
    } else if (slot == 1u) {
        if (edit && steps != 0)
            bass_clock_mode_set(steps < 0 ? 0u : (uint8_t)(BASS_CLOCK_N - 1u));
        else if (!edit) {
            int32_t m = (int32_t)b->clock_mode + (steps > 0 ? 1 : steps < 0 ? -1 : 0);

            if (m < 0)
                m = 0;
            if (m >= (int32_t)BASS_CLOCK_N)
                m = (int32_t)BASS_CLOCK_N - 1;
            bass_clock_mode_set((uint8_t)m);
        }
    } else if (slot == 2u) {
        if (edit && steps != 0) {
            b->transpose = steps < 0 ? (int8_t)-BASS_TRANSPOSE_MAX : (int8_t)BASS_TRANSPOSE_MAX;
        } else if (!edit) {
            int32_t t = (int32_t)b->transpose + steps;

            if (t < -BASS_TRANSPOSE_MAX)
                t = -BASS_TRANSPOSE_MAX;
            if (t > BASS_TRANSPOSE_MAX)
                t = BASS_TRANSPOSE_MAX;
            b->transpose = (int8_t)t;
        }
    } else {
        dub_edit_u8(steps, edit, &b->vel_sens, 0u, 127u);
        if (!edit || steps == 0)
            adj_u8(&b->vel_sens, d, 0, 127);
    }
    mark_dirty();
}
#endif

static void knob_adjust_raw(uint8_t slot, int32_t steps);

static void knob_adjust(uint8_t slot, int32_t steps)
{
#if !defined(GENRE_BASS)
    if (gv_knob_adjust(slot, steps))
        return;
#endif
    knob_adjust_raw(slot, steps);
}

static void knob_adjust_raw(uint8_t slot, int32_t steps)
{
    dub_params_t *p = &g.cfg.p;
    int32_t d = knob_gain(slot, steps);
    uint8_t pg = bank_page();
    uint8_t edit = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    uint8_t lfo_held = (fm1_in.buttons & (1u << BTN_LFO)) != 0u;

#if defined(GENRE_BASS)
    if (bass_bank_is_drumish())
        bass_sanitize_nav();
#endif
    if (edit && steps != 0)
        dub_edit_knob(steps);

    /* Last moved exposed param becomes LFO target. Hold LFO to edit its depth. */
    if (g.view == VIEW_SEQ || g.bank != BANK_LFO) {
        uint8_t lt = lfo_target_for_knob(slot);
        uint8_t seq_lt = 255u;
        int seq_lfo_knob = g.view == VIEW_SEQ && !g.seq_plock_hold &&
                           dub_seq_knob_lfo_map(slot, &seq_lt);

        if ((g.view != VIEW_SEQ || seq_lfo_knob) && dub_lfo_dest_touch(lt)) {
            if (dub_seq_lfo_bind_ix(lt) != 255u)
                g.seq_lfo_lane[dub_seq_lfo_bind_ix(lt)] = g.seq_lane % DUB_SEQ_LANES;
            if (lfo_held) {
                if (edit)
                    g.lfo_slots[lt].depth = steps < 0 ? 0u : 127u;
                else
                    adj_u8(&g.lfo_slots[lt].depth, d, 0, 127);
                mark_dirty();
                return;
            }
        }
    }

#if defined(GENRE_BASS)
    if (!g.seq_plock_hold && g.view != VIEW_SEQ && g.bank == BANK_GROOVE) {
        bass_groove_knob_adjust(pg, slot, steps, d, edit);
        return;
    }
#endif
    if (g.seq_plock_hold) {
#if defined(GENRE_BASS)
        bass_knob_adjust_seq(slot, steps, d, edit);
        return;
#endif
        uint8_t ln = g.seq_lane % DUB_SEQ_LANES;
        uint8_t st = (uint8_t)(g.seq_plock_step % DUB_SEQ_STEPS);
        uint8_t bit = (uint8_t)(1u << ln);
        uint8_t gate_before;
        uint8_t vel_before;
        uint8_t prob_before;
        int8_t micro_before;
        int8_t tune_before;

        if (steps != 0)
            g.seq_follow = 0;
        gate_before = (uint8_t)(g.cfg.seq[st].gate & bit);
        vel_before = g.step_vel[ln][st];
        prob_before = g.step_prob[ln][st];
        micro_before = g.step_micro[ln][st];
        tune_before = g.step_tune[ln][st];
        g.seq_plock_knob = 1u;
        seq_lane_arm_step(ln, st);
        if (slot == 0) {
#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_HOUSE) || defined(FM_DUB_SRN_PLOCK)
            if (ln == 3u) {
                if (edit)
                    g.step_vel[ln][st] = steps < 0 ? 0u : 127u;
                else
                    adj_u8(&g.step_vel[ln][st], d, 0, 127);
            } else
#endif
            {
                if (edit)
                    g.step_vel[ln][st] = steps < 0 ? 100u : 127u;
                else
                    adj_u8(&g.step_vel[ln][st], d, 1, 127);
            }
        } else if (slot == 1) {
            if (edit)
                g.step_prob[ln][st] = steps < 0 ? FM_SEQ_COND_MAX : 0u;
            else {
                int32_t v = (int32_t)g.step_prob[ln][st] + (steps > 0 ? -1 : 1);
                if (v < 0)
                    v = 0;
                if (v > (int32_t)FM_SEQ_COND_MAX)
                    v = (int32_t)FM_SEQ_COND_MAX;
                g.step_prob[ln][st] = (uint8_t)v;
            }
        } else if (slot == 2) {
#ifdef FM_L3_GATE
            if (ln == 3u) {
                /* Lead GATE = note-length index (1/16 … 4/1), same as HP. */
                int32_t ix = (int32_t)FM_L3_LEN_IX(g.step_micro[ln][st]);
                int32_t nmax = (int32_t)FM_L3_LEN_N - 1;

                if (edit)
                    ix = steps < 0 ? 0 : nmax;
                else
                    ix += steps > 0 ? 1 : -1;
                if (ix < 0)
                    ix = 0;
                if (ix > nmax)
                    ix = nmax;
                g.step_micro[ln][st] = FM_L3_LEN_STORE(ix);
            } else
#endif
            {
                if (edit)
                    g.step_micro[ln][st] = steps < 0 ? (int8_t)FM_SEQ_MICRO_MIN : (int8_t)FM_SEQ_MICRO_MAX;
                else {
                    int32_t v = (int32_t)g.step_micro[ln][st] + (steps != 0 ? steps : d);

                    g.step_micro[ln][st] = fm_seq_micro_clamp(v);
                }
            }
        } else if (edit) {
            g.step_tune[ln][st] = steps < 0 ? 0 : 24;
            if ((ln == 4u || ln == 5u) && !g.playing)
                dub_preview_lane(ln, st);
        } else {
            int32_t v = (int32_t)g.step_tune[ln][st] + steps;
            if (v < -24)
                v = -24;
            if (v > 24)
                v = 24;
            g.step_tune[ln][st] = (int8_t)v;
            if ((ln == 4u || ln == 5u) && !g.playing)
                dub_preview_lane(ln, st);
        }
        if ((g.cfg.seq[st].gate & bit) != gate_before || g.step_vel[ln][st] != vel_before ||
            g.step_prob[ln][st] != prob_before || g.step_micro[ln][st] != micro_before ||
            g.step_tune[ln][st] != tune_before)
            seq_lane_pattern_mark_edited(ln);
        mark_dirty();
        return;
    }

    if (g.view == VIEW_SEQ) {
#if defined(GENRE_BASS)
        if (g.seq_page == 5u)
            return; /* song setup page: no knobs */
#else
        if (g.seq_page == 5u && g.bank != BANK_GROOVE)
            return; /* song setup page: no knobs (GLO keeps its knobs) */
#endif
#if defined(GENRE_BASS)
        bass_knob_adjust_seq(slot, steps, d, edit);
        return;
#endif
        uint8_t sl = seq_lane_sel();
        uint8_t ln = seq_lane_is_all() ? (g.seq_lane % DUB_SEQ_LANES) : sl;
        uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);

        if (steps != 0)
            g.seq_follow = 0;
        if (g.seq_page == 4u) {
            if (slot == 0)
                seq_view_cyc_edit(vp, steps, edit);
            else if (slot == 1)
                seq_view_reps_edit(vp, steps, edit, d);
            else if (slot == 2) {
                if (edit)
                    dub_apply_ensemble(steps < 0 ? FM_SEQ_ENS_MUTE : FM_SEQ_ENS_ALL);
                else if (steps != 0) {
                    int32_t ix = (int32_t)g.seq_ens_sel + (steps > 0 ? 1 : -1);
                    if (ix < 0)
                        ix = 0;
                    if (ix >= (int32_t)FM_SEQ_NENSEMBLES)
                        ix = (int32_t)FM_SEQ_NENSEMBLES - 1;
                    dub_apply_ensemble((uint8_t)ix);
                }
            } else if (steps != 0) {
                dub_invert_all_lane_mutes();
                g.seq_flip_state ^= 1u;
            }
            mark_dirty();
            return;
        }
        if (g.seq_page == 5u) {
            if (slot == 0) {
                if (edit && steps != 0)
                    p->root = (uint8_t)((p->root / 12u) * 12u);
                else if (!edit) {
                    int32_t k = (int32_t)(p->root % 12u) + steps;
                    while (k < 0)
                        k += 12;
                    p->root = (uint8_t)((p->root / 12u) * 12u + (uint32_t)(k % 12));
                }
                dub_morph_sync_field(&p->root);
            } else if (slot == 1) {
                if (edit && steps != 0)
                    p->scale = steps < 0 ? 0u : 8u;
                else if (!edit) {
                    int32_t v = (int32_t)p->scale + steps;
                    while (v < 0)
                        v += 9;
                    p->scale = (uint8_t)(v % 9);
                }
                dub_morph_sync_field(&p->scale);
            } else if (slot == 2) {
                dub_edit_u8(steps, edit, &p->swing, 50u, 75u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->swing, d, 50, 75);
            } else if (edit && steps != 0) {
                p->bpm = steps < 0 ? 120u : 180u;
                dub_morph_sync_field(&p->bpm);
                transport_set_bpm();
            } else if (!edit) {
                int32_t v = (int32_t)p->bpm + d;
                if (v < 40)
                    v = 40;
                if (v > 180)
                    v = 180;
                p->bpm = (uint8_t)v;
                dub_morph_sync_field(&p->bpm);
                transport_set_bpm();
            }
            /* Setup latch: audition only until SAVE — do not persist draft groove. */
            if (!g.song_setup_latch)
                mark_dirty();
            else
                g.ui_dirty = 1u;
            return;
        }
        if (g.seq_page == 2u || g.seq_page == 3u) {
            if (slot == 0)
                seq_lane_len_edit(vp, sl, steps, edit, d);
            else if (slot == 1)
                seq_knob_p3_state(ln, steps, edit);
            else if (slot == 2)
                seq_lane_nudge_edit(vp, sl, steps, edit, d);
            else if (slot == 3 && g.seq_page == 2u)
                seq_knob_pace(sl, steps, edit, d);
            else if (steps != 0)
                return;
            mark_dirty();
            return;
        }
        if (g.seq_page == 1u) {
            if (seq_lane_is_all())
                seq_knob_prob_all(slot, steps, edit, d);
            else
                seq_knob_lane_page1(sl, slot, steps, edit, d);
            mark_dirty();
            return;
        }
        if (slot == 0)
            seq_lane_sel_knob(steps, edit);
        else if (slot == 1) {
            if (seq_lane_is_all()) {
                uint8_t i;

                for (i = 0u; i < DUB_SEQ_LANES; i++)
                    seq_knob_euclid_hits(i, steps, edit);
            } else
                seq_knob_euclid_hits(sl, steps, edit);
        } else if (slot == 2) {
            if (seq_lane_is_all()) {
                uint8_t i;

                for (i = 0u; i < DUB_SEQ_LANES; i++)
                    seq_knob_euclid_rot(i, steps, edit);
            } else
                seq_knob_euclid_rot(sl, steps, edit);
        } else {
            if (seq_lane_is_all()) {
                uint8_t i;

                for (i = 0u; i < DUB_SEQ_LANES; i++)
                    seq_knob_vary(i, steps, edit, d);
            } else
                seq_knob_vary(sl, steps, edit, d);
        }
        mark_dirty();
        return;
    }

    if (g.bank == BANK_HOME) {
#if defined(GENRE_BASS)
        bass_knob_adjust_home(pg, slot, steps, d, edit);
        mark_dirty();
        return;
#else
        if (pg == 2u) {
            if (slot == 0) {
                uint8_t *m = &g.voice.macro_motion;
                uint8_t old_v = *m;

                dub_edit_u8(steps, edit, m, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(m, d, 0, 127);
                dub_macro_knob_sync(8u);
                apply_home_macro_delta(2u, 0u, (int32_t)*m - (int32_t)old_v);
            } else if (slot == 1) {
                uint8_t *m = &g.voice.macro_flux;
                uint8_t old_v = *m;

                dub_edit_u8(steps, edit, m, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(m, d, 0, 127);
                dub_macro_knob_sync(9u);
                apply_home_macro_delta(2u, 1u, (int32_t)*m - (int32_t)old_v);
            } else if (slot == 2) {
                if (edit)
                    dub_apply_ensemble(steps < 0 ? FM_SEQ_ENS_MUTE : FM_SEQ_ENS_ALL);
                else if (steps != 0) {
                    int32_t ix = (int32_t)g.seq_ens_sel + (steps > 0 ? 1 : -1);
                    if (ix < 0)
                        ix = 0;
                    if (ix >= (int32_t)FM_SEQ_NENSEMBLES)
                        ix = (int32_t)FM_SEQ_NENSEMBLES - 1;
                    dub_apply_ensemble((uint8_t)ix);
                }
            } else if (steps != 0) {
                dub_invert_all_lane_mutes();
                g.seq_flip_state ^= 1u;
            }
            mark_dirty();
            return;
        }
        {
            uint8_t midx = pg == 0u ? slot : (uint8_t)(4u + slot);
            uint8_t *m = dub_macro_ptr(midx);
            uint8_t old_v;

            if (!m)
                return;
            old_v = *m;
            dub_edit_u8(steps, edit, m, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(m, d, 0, 127);
            dub_macro_knob_sync(midx);
            apply_home_macro_delta(pg, slot, (int32_t)*m - (int32_t)old_v);
            mark_dirty();
            return;
        }
#endif
    }
    if (g.bank == BANK_CHORD) {
#if defined(GENRE_BASS)
        bass_knob_adjust_env(pg, slot, steps, d, edit);
        mark_dirty();
        return;
#else
        if (pg == 0u) {
            if (slot == 0) {
                if (edit && steps != 0) {
                    p->voicing = steps < 0 ? 0u : (uint8_t)(VOICE_N - 1u);
                    dub_morph_sync_field(&p->voicing);
                } else if (!edit) {
                    int32_t v = (int32_t)p->voicing + steps;
                    while (v < 0)
                        v += VOICE_N;
                    p->voicing = (uint8_t)(v % VOICE_N);
                    dub_morph_sync_field(&p->voicing);
                }
            } else {
                uint8_t *q = slot == 1 ? &p->cutoff_c : slot == 2 ? &p->atk_c : &p->decay_c;

                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
            }
        } else if (pg == 1u) {
            if (slot == 3) {
                dub_edit_u8(steps, edit, &p->glide_c, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->glide_c, d, 0, 127);
            } else {
                uint8_t *q = slot == 0 ? &p->timbre : slot == 1 ? &p->reso_mode : &p->env_filt;
                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
            }
        } else {
            if (slot == 0) {
                g.seq_lane = 5u;
                dub_edit_u8(steps, edit, &p->lvl_c, 92u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->lvl_c, d, 0, 127);
                dub_lane_sync_level_mute(5u, p->lvl_c);
            } else if (slot == 1) {
                dub_edit_u8(steps, edit, &p->pan[1], 64u, 127u);
                if (!edit || steps == 0) {
                    if (p->pan[1] == 0u)
                        p->pan[1] = 64u;
                    adj_u8_morph(&p->pan[1], d, 1, 127);
                }
                dub_morph_sync_field(&p->pan[1]);
            } else {
                uint8_t *q = slot == 2 ? &p->dly_c : &p->rev_c;
                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
            }
        }
#endif
    } else if (g.bank == BANK_BASS) {
#if defined(GENRE_BASS)
        if (pg == 0u) {
            if (slot == 0) {
                if (edit && steps != 0)
                    p->bass_model = steps < 0 ? 0u : (uint8_t)(BASS_N - 1u);
                else if (!edit) {
                    int32_t v = (int32_t)p->bass_model + steps;
                    while (v < 0)
                        v += BASS_N;
                    p->bass_model = (uint8_t)(v % BASS_N);
                    dub_morph_sync_field(&p->bass_model);
                }
            } else if (slot == 1) {
                dub_edit_u8(steps, edit, &g.cfg.bass.glide_time, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&g.cfg.bass.glide_time, d, 0, 127);
            } else if (slot == 2) {
                dub_edit_u8(steps, edit, &p->sub_oct, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->sub_oct, d, 0, 127);
            } else {
                dub_edit_u8(steps, edit, &p->pan[0], 64u, 127u);
                if (!edit || steps == 0) {
                    if (p->pan[0] == 0u)
                        p->pan[0] = 64u;
                    adj_u8_morph(&p->pan[0], d, 1, 127);
                }
                dub_morph_sync_field(&p->pan[0]);
            }
        } else if (pg == 1u) {
            if (slot == 0) {
                g.seq_lane = 4u;
                dub_edit_u8(steps, edit, &p->lvl_b, 92u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->lvl_b, d, 0, 127);
                dub_lane_sync_level_mute(4u, p->lvl_b);
            } else if (slot == 1) {
                dub_edit_u8(steps, edit, &p->drive, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->drive, d, 0, 127);
            } else {
                uint8_t *q = slot == 2 ? &p->dly_b : &p->rev_b;
                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
            }
        } else if (pg == 2u) {
            if (slot == 0) {
                dub_edit_u8(steps, edit, &p->reso_mode, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->reso_mode, d, 0, 127);
            } else if (slot == 1) {
                dub_edit_u8(steps, edit, &p->cutoff_b, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->cutoff_b, d, 0, 127);
            } else if (slot == 2) {
                dub_edit_u8(steps, edit, &p->env_filt, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->env_filt, d, 0, 127);
            } else {
                dub_edit_u8(steps, edit, &p->wobble, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->wobble, d, 0, 127);
            }
        }
        mark_dirty();
        return;
#endif
        if (pg == 0u) {
            if (slot == 0) {
                if (edit && steps != 0) {
                    p->bass_model = steps < 0 ? 0u : (uint8_t)(BASS_N - 1u);
                    dub_morph_sync_field(&p->bass_model);
                } else if (!edit) {
                    int32_t v = (int32_t)p->bass_model + steps;
                    while (v < 0)
                        v += BASS_N;
                    p->bass_model = (uint8_t)(v % BASS_N);
                    dub_morph_sync_field(&p->bass_model);
                }
            } else {
                uint8_t *q = slot == 1 ? &p->cutoff_b : slot == 2 ? &p->atk_b : &p->decay_b;

                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
            }
        } else if (pg == 1u) {
            uint8_t *q = slot == 0 ? &p->sub_oct : slot == 1 ? &p->pump : slot == 2 ? &p->drive : &p->wobble;
            dub_edit_u8(steps, edit, q, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8_morph(q, d, 0, 127);
        } else {
            if (slot == 0) {
                g.seq_lane = 4u;
                dub_edit_u8(steps, edit, &p->lvl_b, 96u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->lvl_b, d, 0, 127);
                dub_lane_sync_level_mute(4u, p->lvl_b);
            } else if (slot == 1) {
                dub_edit_u8(steps, edit, &p->pan[0], 64u, 127u);
                if (!edit || steps == 0) {
                    if (p->pan[0] == 0u)
                        p->pan[0] = 64u;
                    adj_u8_morph(&p->pan[0], d, 1, 127);
                }
                dub_morph_sync_field(&p->pan[0]);
            } else {
                uint8_t *q = slot == 2 ? &p->dly_b : &p->rev_b;
                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
            }
        }
    } else if (g.bank == BANK_DRUM) {
        if (pg == 0u) {
            if (slot == 0) {
                if (edit && steps != 0) {
                    p->drum_kit = steps < 0 ? 0u : (uint8_t)(DKIT_N - 1u);
                    dub_morph_sync_field(&p->drum_kit);
                } else if (!edit) {
                    int32_t v = (int32_t)p->drum_kit + steps;

                    while (v < 0)
                        v += DKIT_N;
                    p->drum_kit = (uint8_t)(v % DKIT_N);
                    dub_morph_sync_field(&p->drum_kit);
                }
            } else if (slot == 2) {
                uint8_t di = g.sel_drm % 4u;
                uint8_t *q = drm_lvl_ptr(p, di);

                g.seq_lane = di;
                dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
                dub_lane_sync_level_mute(di, *q);
            } else {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_tune[di], 64u, 127u);
                if (!edit || steps == 0) {
                    if (p->drm_tune[di] == 0u)
                        p->drm_tune[di] = 64u;
                    adj_u8_morph(&p->drm_tune[di], d, 1, 127);
                }
                dub_morph_sync_field(&p->drm_tune[di]);
            }
        } else if (pg == 1u) {
            if (slot == 0) {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_dens[di], 64u, 108u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->drm_dens[di], d, 0, 127);
                dub_morph_sync_field(&p->drm_dens[di]);
                if (di == 0u)
                    p->density = p->drm_dens[0];
            } else if (slot == 1) {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_cut[di], 64u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->drm_cut[di], d, 0, 127);
                dub_morph_sync_field(&p->drm_cut[di]);
            } else if (slot == 2) {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_atk[di], 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->drm_atk[di], d, 0, 127);
                dub_morph_sync_field(&p->drm_atk[di]);
            } else {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_dec[di], 64u, 127u);
                if (!edit || steps == 0) {
                    if (p->drm_dec[di] == 0u)
                        p->drm_dec[di] = 64u;
                    adj_u8_morph(&p->drm_dec[di], d, 1, 127);
                }
                dub_morph_sync_field(&p->drm_dec[di]);
            }
        } else {
            static const uint8_t def_dp[4] = {64u, 74u, 52u, 82u};
            static const uint8_t def_dr[4] = {70u, 70u, 70u, 96u};

            if (slot == 0) {
                if (sel_drm_is_siren()) {
                    dub_edit_u8(steps, edit, &p->siren_rate, 0u, 127u);
                    if (!edit || steps == 0)
                        adj_u8_morph(&p->siren_rate, d, 0, 127);
                    dub_morph_sync_field(&p->siren_rate);
                } else {
                    uint8_t di = sel_drm_ix();

                    dub_edit_u8(steps, edit, &p->drm_drv[di], 0u, 127u);
                    if (!edit || steps == 0)
                        adj_u8_morph(&p->drm_drv[di], d, 0, 127);
                    dub_morph_sync_field(&p->drm_drv[di]);
                }
            } else if (slot == 1) {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_pan[di], def_dp[di], 127u);
                if (!edit || steps == 0) {
                    if (p->drm_pan[di] == 0u)
                        p->drm_pan[di] = def_dp[di];
                    adj_u8_morph(&p->drm_pan[di], d, 1, 127);
                }
                dub_morph_sync_field(&p->drm_pan[di]);
            } else if (slot == 2) {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_dly[di], 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->drm_dly[di], d, 0, 127);
                dub_morph_sync_field(&p->drm_dly[di]);
            } else {
                uint8_t di = sel_drm_ix();

                dub_edit_u8(steps, edit, &p->drm_rev[di], def_dr[di], 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(&p->drm_rev[di], d, 0, 127);
                dub_morph_sync_field(&p->drm_rev[di]);
            }
        }
    } else if (g.bank == BANK_FX) {
        if (pg == 0u) {
            if (slot == 1) {
                uint32_t bpm_t = p->bpm ? p->bpm : 120u;
                if (edit && steps != 0) {
                    /* EDIT + TIME steps cleanly between musical fraction locks (OFF, 1/32, 1/24 .. 3/1, 4/1) */
                    p->dly_time = fm_time_step_snap(p->dly_time, steps, bpm_t);
                    dub_morph_sync_field(&p->dly_time);
                } else if (!edit) {
                    /* Continuous 0..127 delay time continuum (locks to musical fractions when passing them) */
                    adj_u8_morph(&p->dly_time, d, 0, 127);
                }
            } else {
                uint8_t *q = slot == 0 ? &p->dly_lvl : slot == 2 ? &p->feedback : &p->loop_filt;
                if (slot == 3)
                    dub_edit_u8(steps, edit, &p->loop_filt, 64u, 127u);
                else
                    dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
                if (slot == 3)
                    dub_morph_sync_field(&p->loop_filt);
            }
        } else if (pg == 1u) {
            if (slot == 1) {
                if (edit && steps != 0)
                    p->tap = steps < 0 ? 0u : (uint8_t)(TAP_N - 1u);
                else if (!edit) {
                    int32_t v = (int32_t)p->tap + steps;
                    while (v < 0)
                        v += TAP_N;
                    p->tap = (uint8_t)(v % TAP_N);
                }
                dub_morph_sync_field(&p->tap);
            } else {
                uint8_t *q = slot == 0 ? &p->wow : slot == 2 ? &p->tank : &p->isolator;
                if (slot == 3)
                    dub_edit_u8(steps, edit, &p->isolator, 64u, 127u);
                else
                    dub_edit_u8(steps, edit, q, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8_morph(q, d, 0, 127);
                if (slot == 3)
                    dub_morph_sync_field(&p->isolator);
            }
        } else {
            uint8_t *q = slot == 0 ? &p->rev_mix : slot == 1 ? &p->rev_damp :
                         slot == 2 ? &p->rev_tail : &p->dly_rev;
            dub_edit_u8(steps, edit, q, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8_morph(q, d, 0, 127);
        }
    } else if (g.bank == BANK_LFO) {
        uint8_t t = g.lfo_target % DUB_LFO_TGT_N;
        fm_lfo_slot_t *sl;

        g.lfo_target = t;
        sl = &g.lfo_slots[t];

        if (pg == 0u) {
            if (slot == 0) {
                if (edit && steps != 0)
                    g.lfo_target = steps < 0 ? fm_lfo_dests_first(&dub_lfo_dests, (uint8_t)DUB_LFO_OCT) :
                                   fm_lfo_dests_last(&dub_lfo_dests, (uint8_t)DUB_LFO_OCT);
                else if (!edit)
                    g.lfo_target = dub_lfo_tgt_step(t, steps);
            } else if (slot == 1) {
                uint8_t rate_min = fm_lfo_rate_min(g.cfg.p.bpm ? g.cfg.p.bpm : 120u);

                dub_edit_u8(steps, edit, &sl->rate, FM_LFO_RATE_DEFAULT, 127u);
                if (!edit || steps == 0)
                    adj_u8(&sl->rate, d, rate_min, 127);
            } else if (slot == 2) {
                dub_edit_u8(steps, edit, &sl->depth, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&sl->depth, d, 0, 127);
            } else if (edit && steps != 0)
                sl->wave = steps < 0 ? 0u : 127u;
            else
                sl->wave = fm_lfo_step_shape(sl->wave, steps, 0u);
        } else if (pg == 1u) {
            if (slot == 0) {
                dub_edit_u8(steps, edit, &sl->phase, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&sl->phase, d, 0, 127);
            } else if (slot == 1) {
                dub_edit_u8(steps, edit, &sl->skew, FM_LFO_SKEW_DEFAULT, 127u);
                if (!edit || steps == 0)
                    adj_u8(&sl->skew, d, 0, 127);
            } else if (slot == 2) {
                dub_edit_u8(steps, edit, &sl->smooth, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&sl->smooth, d, 0, 127);
            } else {
                dub_edit_u8(steps, edit, &sl->fold, 0u, 127u);
                if (!edit || steps == 0)
                    adj_u8(&sl->fold, d, 0, 127);
            }
        } else if (slot == 0) {
            int32_t v = (int32_t)sl->algo + steps;
            if (edit && steps != 0)
                sl->algo = steps < 0 ? 0u : (uint8_t)(FM_LFO_NALGOS - 1u);
            else if (!edit) {
                while (v < 0)
                    v += (int32_t)FM_LFO_NALGOS;
                sl->algo = (uint8_t)((uint32_t)v % FM_LFO_NALGOS);
            }
        } else if (slot == 1) {
            dub_edit_u8(steps, edit, &sl->ratio, 52u, 127u);
            if (!edit || steps == 0)
                adj_u8(&sl->ratio, d, 0, 127);
        } else if (slot == 2) {
            dub_edit_u8(steps, edit, &sl->index, 64u, 127u);
            if (!edit || steps == 0)
                adj_u8(&sl->index, d, 0, 127);
        } else {
            dub_edit_u8(steps, edit, &sl->fb, 0u, 127u);
            if (!edit || steps == 0)
                adj_u8(&sl->fb, d, 0, 127);
        }
    }
    mark_dirty();
}

static void knob_label(uint8_t slot, char *s)
{
    uint8_t pg = bank_page();

#if !defined(GENRE_BASS)
    if (gv_knob_label(slot, s))
        return;
#endif
#if defined(GENRE_BASS)
    if (bass_bank_is_drumish())
        bass_sanitize_nav();
#endif
    if (g.seq_plock_hold) {
#if defined(GENRE_BASS)
        bass_knob_label_seq(slot, s);
        return;
#else
        uint8_t ln = g.seq_lane % DUB_SEQ_LANES;

        copy_str(s, fm_seq_knob_label_lane(0u, 1, slot, ln));
        return;
#endif
    }
    if (g.view == VIEW_SEQ) {
#if defined(GENRE_BASS)
        bass_knob_label_seq(slot, s);
        return;
#endif
        if (g.seq_page == 4u) {
            copy_str(s, FM_SEQ_KNOB_P4[slot]);
            return;
        }
        if (g.seq_page == 5u) {
            static const char *const L[4] = {"KEY", "SCALE", "SWING", "TEMPO"};
            copy_str(s, L[slot]);
            return;
        }
        if (g.seq_page == 2u && slot == 3u) {
            copy_str(s, "PACE");
            return;
        }
        copy_str(s, fm_seq_knob_label(g.seq_page, 0, slot));
        return;
    }
#if defined(GENRE_BASS)
    if (g.bank == BANK_GROOVE) {
        bass_groove_knob_label(pg, slot, s);
        return;
    }
#endif
    if (g.bank == BANK_HOME) {
#if defined(GENRE_BASS)
        bass_draw_home_knob_label(pg, slot, s);
        return;
#endif
#ifdef GENRE_HEAVY_PSYCH
        static const char *const L0[4] = {"FUZZ", "LAYER", "DROP", "SWEEP"};
        static const char *const L1[4] = {"SPACE", "WEIGHT", "GRIT", "DRONE"};
#elif defined(GENRE_HOUSE)
        static const char *const L0[4] = {"JACK", "LAYER", "CUT", "SWEEP"};
        static const char *const L1[4] = {"SPACE", "PUMP", "GRIT", "FLOOR"};
#elif defined(GENRE_JAZZ)
        static const char *const L0[4] = {"COMP", "LAYER", "SWAY", "SWEEP"};
        static const char *const L1[4] = {"SPACE", "WASH", "GRIT", "MUTE"};
#elif defined(GENRE_SOUL)
        static const char *const L0[4] = {"STAB", "ECHO", "ROTOR", "SWEEP"};
        static const char *const L1[4] = {"ROOM", "BOTTOM", "GRIT", "SHOUT"};
#elif defined(GENRE_IDM)
        static const char *const L0[4] = {"CELL", "CROP", "REVERS", "SWEEP"};
        static const char *const L1[4] = {"SPACE", "HACK", "GRIT", "SCREW"};
#elif defined(GENRE_SNOWCRASH)
        static const char *const L0[4] = {"GLOSS", "SLICE", "HOP", "TIDE"};
        static const char *const L1[4] = {"GHOST", "CLICK", "SLAG", "HUM"};
#elif defined(GENRE_HIPHOP)
        static const char *const L0[4] = {"SNAP", "DUST", "ROLL", "SWEEP"};
        static const char *const L1[4] = {"ROOM", "DUCK", "GRIT", "CHOP"};
#elif defined(GENRE_FUNK)
        static const char *const L0[4] = {"CHOP", "CROP", "WAH", "SWEEP"};
        static const char *const L1[4] = {"SPACE", "DUCK", "GRIT", "THUMP"};
#elif defined(GENRE_POP)
        static const char *const L0[4] = {"GATE", "LINE", "CHOR", "WIDE"};
        static const char *const L1[4] = {"DELAY", "PUMP", "BRITE", "LEVEL"};
#elif defined(GENRE_TRIPHOP)
        static const char *const L0[4] = {"DUST", "VINYL", "SMOKE", "PAD"};
        static const char *const L1[4] = {"SUB", "TAPE", "DARK", "FILM"};
#elif defined(GENRE_TECHNO)
        static const char *const L0[4] = {"PUMP", "RUMBLE", "HOVER", "TANK"};
        static const char *const L1[4] = {"STEEL", "DIST", "HARD", "GATE"};
#else
        static const char *const L0[4] = {"SKANK", "LAYER", "WARP", "SWEEP"};
        static const char *const L1[4] = {"SPACE", "WEIGHT", "GRIT", "DUB"};
#endif
#ifdef GENRE_HEAVY_PSYCH
        static const char *const L2[4] = {"MOTION", "FLUX", "BAND", "INVERT"};
#elif defined(GENRE_HOUSE)
        static const char *const L2[4] = {"MOTION", "FLUX", "CREW", "INVERT"};
#elif defined(GENRE_JAZZ)
        static const char *const L2[4] = {"MOTION", "FLUX", "COMBO", "INVERT"};
#elif defined(GENRE_SOUL)
        static const char *const L2[4] = {"MOTION", "FLUX", "REVUE", "INVERT"};
#elif defined(GENRE_IDM)
        static const char *const L2[4] = {"MOTION", "FLUX", "PACK", "INVERT"};
#elif defined(GENRE_SNOWCRASH)
        static const char *const L2[4] = {"DRIFT", "FLUX", "DECK", "INVERT"};
#elif defined(GENRE_HIPHOP)
        static const char *const L2[4] = {"MOTION", "FLUX", "BLOCK", "INVERT"};
#elif defined(GENRE_FUNK)
        static const char *const L2[4] = {"MOTION", "FLUX", "UNIT", "INVERT"};
#elif defined(GENRE_POP)
        static const char *const L2[4] = {"GLIDE", "DENS", "TRACK", "INVERT"};
#elif defined(GENRE_TRIPHOP)
        static const char *const L2[4] = {"MOTION", "FLUX", "MOOD", "INVERT"};
#elif defined(GENRE_TECHNO)
        static const char *const L2[4] = {"PHASE", "DRIVE", "RIG", "INVERT"};
#else
        static const char *const L2[4] = {"MOTION", "FLUX", "SYSTEM", "INVERT"};
#endif
        copy_str(s, pg == 0u ? L0[slot] : pg == 1u ? L1[slot] : L2[slot]);
        return;
    }
    if (g.bank == BANK_CHORD) {
#if defined(GENRE_BASS)
        bass_draw_env_knob_label(pg, slot, s);
        return;
#else
        if (pg == 0u) {
            static const char *const L[4] = {"VOICE", "CUTOFF", "ATTACK", "DECAY"};
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
            static const char *const L[4] = {"TIMBRE", "RESONA", "ENVLPE", "GLIDE"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"CHORD", "PAN", "DELAY", "REVERB"};
            copy_str(s, L[slot]);
        }
#endif
        return;
    }
    if (g.bank == BANK_BASS) {
#if defined(GENRE_BASS)
        if (pg == 0u) {
            static const char *const L[4] = {"MODEL", "GLIDE", "SUB", "PAN"};
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
            static const char *const L[4] = {"LEVEL", "DRIVE", "DELAY", "REVERB"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"RESO", "CUTOFF", "FENV", "WOB"};
            copy_str(s, L[slot]);
        }
#else
        if (pg == 0u) {
            static const char *const L[4] = {"MODEL", "CUTOFF", "ATTACK", "DECAY"};
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
#if defined(GENRE_HOUSE) || defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_TRIPHOP)
            static const char *const L[4] = {"SUB", "PUMP", "DRIVE", "ENV"};
#else
            static const char *const L[4] = {"SUB", "PUMP", "DRIVE", "WOBBLE"};
#endif
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"BASS", "PAN", "DELAY", "REVERB"};
            copy_str(s, L[slot]);
        }
#endif
        return;
    }
    if (g.bank == BANK_DRUM) {
        if (pg == 0u) {
            static const char *const L[4] = {"KIT", "", "LEVEL", "TUNE"};
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
            if (slot == 0u) {
                static const char *const T[4] = {"PUNCH", "SNAP", "SIZZLE", "TIMBRE"};
                copy_str(s, T[sel_drm_ix()]);
            } else {
                static const char *const L[3] = {"CUTOFF", "ATTACK", "DECAY"};
                copy_str(s, L[slot - 1u]);
            }
        } else if (slot == 0) {
#if defined(GENRE_HEAVY_PSYCH)
            copy_str(s, sel_drm_is_siren() ? "SING" : "DRIVE");
#elif defined(GENRE_HOUSE)
            copy_str(s, sel_drm_is_siren() ? "SNAP" : "DRIVE");
#elif defined(GENRE_JAZZ)
            copy_str(s, sel_drm_is_siren() ? "BEND" : "DRIVE");
#elif defined(GENRE_IDM)
            copy_str(s, sel_drm_is_siren() ? "EDGE" : "DRIVE");
#elif defined(GENRE_SNOWCRASH)
            copy_str(s, sel_drm_is_siren() ? "PIERCE" : "DRIVE");
#elif defined(GENRE_HIPHOP)
            copy_str(s, sel_drm_is_siren() ? "DRAG" : "DRIVE");
#elif defined(GENRE_POP)
            copy_str(s, sel_drm_is_siren() ? pop_lead_drive(g.cfg.p.era) : "DRIVE");
#elif defined(GENRE_FUNK)
            copy_str(s, sel_drm_is_siren() ? "SNAP" : "DRIVE");
#else
            copy_str(s, sel_drm_is_siren() ? "RATE" : "DRIVE");
#endif
        } else if (slot == 1) {
            copy_str(s, "PAN");
        } else if (slot == 2) {
            copy_str(s, "DELAY");
        } else {
            copy_str(s, "REVERB");
        }
        return;
    }
    if (g.bank == BANK_FX) {
        if (pg == 0u) {
#if defined(GENRE_BASS)
            static const char *const L[4] = {"DLY", "TIME", "FB", "TONE"};
#else
            static const char *const L[4] = {"MIX", "TIME", "FEEDBK", "TONE"};
#endif
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
#if defined(GENRE_POP)
            static const char *const L[4] = {"WOW", "TAP", "ROOM", "ISO"};
#else
            static const char *const L[4] = {"WOW", "TAP", "TANK", "ISO"};
#endif
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"REVERB", "DAMP", "TAIL", "BLEED"};
            copy_str(s, L[slot]);
        }
        return;
    }
    if (g.bank == BANK_LFO) {
        if (pg == 0u) {
            static const char *const L[4] = {"TARGET", "RATE", "DEPTH", "SHAPE"};
            copy_str(s, L[slot]);
        } else if (pg == 1u) {
            static const char *const L[4] = {"PHASE", "SKEW", "SMOOTH", "FOLD"};
            copy_str(s, L[slot]);
        } else {
            static const char *const L[4] = {"ALGO", "RATIO", "FM IDX", "OP FB"};
            copy_str(s, L[slot]);
        }
    }
}

static void knob_value(uint8_t slot, char *s)
{
    const dub_params_t *p = g.view == VIEW_SEQ ? &g.cfg.p : &g.voice.p;
    uint8_t pg = bank_page();

#if !defined(GENRE_BASS)
    if (gv_knob_value(slot, s))
        return;
#endif
#if defined(GENRE_BASS)
    if (bass_bank_is_drumish())
        bass_sanitize_nav();
#endif
    if (g.seq_plock_hold) {
#if defined(GENRE_BASS)
        bass_knob_value_seq(slot, s);
        return;
#endif
        uint8_t ln = g.seq_lane % DUB_SEQ_LANES;
        uint8_t st = (uint8_t)(g.seq_plock_step % DUB_SEQ_STEPS);
        dub_step_t *ovr = &g.cfg.seq[st];
        uint8_t base_accent = ovr->accent ? ovr->accent : 90u;

        if (slot == 0) {
#if defined(GENRE_HEAVY_PSYCH)
            if (ln == 3u) {
                uint8_t b = g.step_vel[ln][st];

                if (b == 0u)
                    copy_str(s, "OFF");
                else
                    u8_show100_dec(s, heavy_psych_lead_bend_semi(b));
            } else
#elif defined(GENRE_FUNK)
            if (ln == 3u) {
                uint8_t b = g.step_vel[ln][st];

                if (b == 0u)
                    copy_str(s, "OFF");
                else
                    u8_show100_dec(s, funk_stab_bend_semi_u8(b));
            } else
#elif defined(GENRE_HOUSE)
            if (ln == 3u) {
                uint8_t b = g.step_vel[ln][st];

                if (b == 0u)
                    copy_str(s, "OFF");
                else
                    u8_show100_dec(s, house_stab_bend_semi_u8(b));
            } else
#elif defined(FM_DUB_SRN_PLOCK)
            if (ln == 3u) {
                uint8_t b = g.step_vel[ln][st];

                if (b == 0u)
                    copy_str(s, "OFF");
                else
                    u8_show100_dec(s, dub_siren_bend_semi_u8(b));
            } else
#endif
                u8_show100_dec(s, g.step_vel[ln][st] ? g.step_vel[ln][st] : base_accent);
        } else if (slot == 1)
            copy_str(s, FM_SEQ_COND_NAMES[g.step_prob[ln][st] % FM_SEQ_NCONDS]);
        else if (slot == 2) {
#ifdef FM_L3_GATE
            if (ln == 3u)
                copy_str(s, FM_L3_LEN_NAMES[FM_L3_LEN_IX(g.step_micro[ln][st])]);
            else
#endif
                fm_seq_fmt_micro_readout((int32_t)dub_eff_step_micro(ln, st), s);
        } else if (ln == 4u || ln == 5u) {
            int32_t nn = (int32_t)p->root + g.lane_tune[ln] + g.step_tune[ln][st];

            copy_str(s, NOTE_NAMES[((uint32_t)nn) % 12u]);
        } else
            i32_signed_dec(s, g.step_tune[ln][st]);
        return;
    }
    if (g.view == VIEW_SEQ) {
#if defined(GENRE_BASS)
        bass_knob_value_seq(slot, s);
        return;
#endif
        uint8_t sl = seq_lane_sel();
        uint8_t ln = seq_lane_is_all() ? (g.seq_lane % DUB_SEQ_LANES) : sl;
        uint8_t seq_lt = 255u;

        if ((fm1_in.buttons & (1u << BTN_LFO)) && dub_seq_knob_lfo_map(slot, &seq_lt)) {
            s[0] = '~';
            u8_show100_dec(s + 1, g.lfo_slots[seq_lt].depth);
            return;
        }

        if (g.seq_page == 4u) {
            uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);

            if (slot == 0)
                copy_str(s, FM_SEQ_VIEW_CYC_NAMES[dj_seq_eff_cyc(vp) % FM_SEQ_NVIEW_CYC]);
            else if (slot == 1)
                u8_dec(s, dj_seq_eff_reps(vp));
            else if (slot == 2) {
                uint8_t det = dub_detect_ensemble();
                copy_str(s, det == FM_SEQ_ENS_CUSTOM ? "CUSTOM" : GENRE_ENS_NAMES[det]);
            } else
                copy_str(s, FM_SEQ_FLIP_NAMES[g.seq_flip_state & 1u]);
            return;
        }
        if (g.seq_page == 5u) {
            uint8_t show_root = g.cue_on ? g.cue_root : p->root;
            uint8_t show_scale = dub_groove_show_scale();

            if (slot == 0)
                copy_str(s, NOTE_NAMES[show_root % 12u]);
            else if (slot == 1)
                copy_str(s, SCALE_NAMES[show_scale % 9u]);
            else if (slot == 2)
                u8_dec(s, dub_groove_show_swing());
            else
                u8_dec(s, dub_groove_show_bpm());
            return;
        }
        if (g.seq_page == 2u || g.seq_page == 3u) {
            uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);

            if (slot == 0) {
                if (seq_lane_is_all())
                    copy_str(s, "ALL");
                else
                    u8_dec(s, seq_lane_len_v(vp, sl));
            } else if (slot == 1)
                copy_str(s, FM_SEQ_P3_STATE_NAMES[dub_lane_p3_state_pos(ln)]);
            else if (slot == 2)
                fm_seq_fmt_nudge_readout((int32_t)seq_lane_nudge_v(vp, ln), s);
            else if (slot == 3 && g.seq_page == 2u)
                copy_str(s, FM_SEQ_PACE4_NAMES[dub_lane_pace_eff(ln)]);
            else
                s[0] = 0;
            return;
        }
        if (g.seq_page == 1u) {
            uint8_t fn = fm_seq_p1_fn((uint8_t)slot);

            if (fn == 1u) {
                fm_seq_fmt_trig(dub_eff_trig_raw(ln), s);
                return;
            }
            fm_seq_fmt_rand_spread(fn == 0u ? dub_eff_vel_rng(ln) :
                                   fn == 2u ? dub_eff_gate_rng(ln) : dub_eff_pitch_rng(ln), s);
            return;
        }
        if (slot == 0) {
            if (seq_lane_is_all())
                copy_str(s, "ALL");
            else
                copy_str(s, seq_lane_name(sl));
        } else if (slot == 1) {
            uint8_t eh = dub_eff_euclid_hits(ln);

            if (eh == FM_SEQ_EUCLID_EMPTY)
                copy_str(s, seq_lane_clear_label(ln));
            else if (eh == FM_SEQ_EUCLID_SONG && !seq_lane_content_is_song(ln))
                copy_str(s, "EDITED");
            else
                fm_seq_fmt_euclid(eh, s);
        } else if (slot == 2)
            u8_dec(s, (uint32_t)dub_eff_euclid_rot(ln));
        else
            fm_seq_fmt_vary(dub_eff_vary(ln), s);
        return;
    }

#if defined(GENRE_BASS)
    if (g.bank == BANK_GROOVE) {
        bass_groove_knob_value(pg, slot, s);
        return;
    }
#endif
    if (g.bank == BANK_HOME) {
#if defined(GENRE_BASS)
        if (pg == 0u)
            u8_show100_dec(s, slot == 0 ? p->cutoff_b : slot == 1 ? p->reso_mode :
                           slot == 2 ? p->drive : p->dly_b);
        else if (pg == 1u) {
            if (slot == 0u)
                u8_show100_dec(s, g.cfg.bass.glide_time);
            else {
                u8_show100_dec(s, bass_mac_eff_raw((uint8_t)(slot - 1u)));
            }
        } else if ((fm1_in.buttons & (1u << BTN_EDIT)) != 0u) {
            uint8_t sel = g.bass_rt.home_macro_sel;
            bass_mod_slot_t *sl = bass_mod_route_slot(&g.cfg.bass.mods, sel);

            if (slot == 0u)
                copy_str(s, bass_mod_macro_name((uint8_t)(sel / BASS_MOD_SLOTS_PER_MAP)));
            else if (slot == 1u)
                u8_dec(s, (uint32_t)(sel % BASS_MOD_SLOTS_PER_MAP) + 1u);
            else if (slot == 2u)
                copy_str(s, bass_mod_dest_name(sl->dest));
            else
                amt100_show(s, sl->amount);
        } else {
            int32_t c = (int32_t)bass_perf_eff_raw(slot) - 64;

            if (slot == BASS_PERF_SHIFT)
                i32_signed_dec(s, c / 8);
            else
                amt100_show(s, (int8_t)(c * 2));
        }
        return;
#else
        if (pg == 2u) {
            if (slot == 0)
                fmt_motion_macro(dub_eff_macro(8), s);
            else if (slot == 1)
                u8_show100_dec(s, dub_eff_macro(9));
            else if (slot == 2) {
                uint8_t det = dub_detect_ensemble();
                copy_str(s, det == FM_SEQ_ENS_CUSTOM ? "CUSTOM" : GENRE_ENS_NAMES[det]);
            } else
                copy_str(s, FM_SEQ_FLIP_NAMES[g.seq_flip_state & 1u]);
            return;
        }
        u8_show100_dec(s, dub_eff_macro(pg == 0u ? slot : (uint8_t)(4u + slot)));
        return;
#endif
    }
    if (g.bank == BANK_CHORD) {
#if defined(GENRE_BASS)
        const bass_dahdsr_cfg_t *env = &g.cfg.bass.env;
        bass_mod_slot_t *esl;
        uint8_t esi;

        esi = g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP;
        esl = &g.cfg.bass.mods.env[esi];
        if (pg == 0u)
            u8_show100_dec(s, slot == 0 ? env->t.attack : slot == 1 ? env->t.decay :
                           slot == 2 ? env->t.sustain : env->t.release);
        else if (pg == 1u) {
            if (slot < 2u)
                u8_show100_dec(s, slot == 0 ? env->t.delay : env->t.hold);
            else if (slot == 2u)
                bip50_show(s, bass_env_shape_curve(g.cfg.bass_env_shape));
            else
                copy_str(s, env->pol.retrigger ? "ON" : "OFF");
        } else if (pg >= FM_ENV_KIT_PAGES) {
            if (slot == 0u)
                u8_dec(s, esl->curve);
        } else if (slot == 0u)
            copy_str(s, env->pol.invert ? "ON" : "OFF");
        else if (slot == 1u)
            u8_show100_dec(s, env->pol.accent_scale);
        else if (slot == 2u)
            copy_str(s, bass_mod_dest_name(esl->dest));
        else
            amt100_show(s, esl->amount);
        return;
#else
        if (pg == 0u) {
            if (slot == 0)
                copy_str(s, VOICE_NAMES[p->voicing % VOICE_N]);
            else
                u8_show100_dec(s, slot == 1 ? p->cutoff_c : slot == 2 ? p->atk_c : p->decay_c);
        } else if (pg == 1u) {
            u8_show100_dec(s, slot == 3 ? p->glide_c :
                   slot == 0 ? p->timbre : slot == 1 ? p->reso_mode : p->env_filt);
        } else {
            if (slot == 0 && !dub_lane_audible(5u))
                copy_str(s, "MUTE");
            else if (slot == 1)
                fmt_pan(s, p->pan[1]);
            else
                u8_show100_dec(s, slot == 0 ? p->lvl_c : slot == 2 ? p->dly_c : p->rev_c);
        }
        return;
#endif
    }
    if (g.bank == BANK_BASS) {
#if defined(GENRE_BASS)
        if (pg == 0u) {
            if (slot == 0)
                copy_str(s, BASS_NAMES[p->bass_model % BASS_N]);
            else if (slot == 1)
                u8_show100_dec(s, g.cfg.bass.glide_time);
            else if (slot == 2)
                u8_show100_dec(s, p->sub_oct);
            else
                fmt_pan(s, p->pan[0]);
        } else if (pg == 1u) {
            if (slot == 0 && !dub_lane_audible(4u))
                copy_str(s, "MUTE");
            else
                u8_show100_dec(s, slot == 0 ? p->lvl_b : slot == 1 ? p->drive :
                               slot == 2 ? p->dly_b : p->rev_b);
        } else if (slot == 0)
            u8_show100_dec(s, p->reso_mode);
        else if (slot == 1)
            u8_show100_dec(s, p->cutoff_b);
        else if (slot == 2)
            u8_show100_dec(s, p->env_filt);
        else
            u8_show100_dec(s, p->wobble);
#else
        if (pg == 0u) {
            if (slot == 0)
                copy_str(s, BASS_NAMES[p->bass_model % BASS_N]);
            else
                u8_show100_dec(s, slot == 1 ? p->cutoff_b : slot == 2 ? p->atk_b : p->decay_b);
        } else if (pg == 1u)
            u8_show100_dec(s, slot == 0 ? p->sub_oct : slot == 1 ? p->pump : slot == 2 ? p->drive : p->wobble);
        else {
            if (slot == 0 && !dub_lane_audible(4u))
                copy_str(s, "MUTE");
            else if (slot == 1)
                fmt_pan(s, p->pan[0]);
            else
                u8_show100_dec(s, slot == 0 ? p->lvl_b : slot == 2 ? p->dly_b : p->rev_b);
        }
#endif
        return;
    }
    if (g.bank == BANK_DRUM) {
        uint8_t di = sel_drm_ix();

        if (pg == 0u) {
            if (slot == 0)
                copy_str(s, DKIT_NAMES[p->drum_kit % DKIT_N]);
            else if (slot == 1)
                s[0] = '\0';
            else if (slot == 2) {
                if (!dub_lane_audible(g.sel_drm % 4u))
                    copy_str(s, "MUTE");
                else
                    u8_show100_dec(s, *drm_lvl_cptr(p, di));
            } else
                bip50_show(s, p->drm_tune[di] ? p->drm_tune[di] : 64u);
        } else if (pg == 1u) {
            if (slot == 0)
                u8_show100_dec(s, p->drm_dens[di]);
            else if (slot == 1)
                u8_show100_dec(s, p->drm_cut[di]);
            else if (slot == 2)
                u8_show100_dec(s, p->drm_atk[di]);
            else {
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
                if (di == 3u)
                    bip50_show(s, p->drm_dec[di] ? p->drm_dec[di] : 64u);
                else
#endif
                    u8_show100_dec(s, dub_drm_display_decay(p, di, g.voice.lane_gate[2]));
            }
        } else {
            if (slot == 0) {
                if (sel_drm_is_siren())
                    u8_show100_dec(s, p->siren_rate);
                else
                    u8_show100_dec(s, p->drm_drv[di]);
            } else if (slot == 1)
                fmt_pan(s, p->drm_pan[di]);
            else
                u8_show100_dec(s, slot == 2 ? p->drm_dly[di] : p->drm_rev[di]);
        }
        return;
    }
    if (g.bank == BANK_FX) {
        if (pg == 0u) {
            if (slot == 1)
                fm_time_fmt(s, p->dly_time, p->bpm ? p->bpm : 120u, fm1_ms);
            else if (slot == 2)
                u8_show100_dec(s, p->feedback);
            else if (slot == 3)
                bip50_show(s, p->loop_filt);
            else
                u8_show100_dec(s, p->dly_lvl);
        } else if (pg == 1u) {
            if (slot == 1)
                copy_str(s, TAP_NAMES[p->tap % TAP_N]);
            else if (slot == 3)
                bip50_show(s, p->isolator);
            else
                u8_show100_dec(s, slot == 0 ? p->wow : p->tank);
        } else {
            u8_show100_dec(s, slot == 0 ? p->rev_mix : slot == 1 ? p->rev_damp :
                      slot == 2 ? p->rev_tail : p->dly_rev);
        }
        return;
    }
    if (g.bank == BANK_LFO) {
        uint8_t t = g.lfo_target % DUB_LFO_TGT_N;
        const fm_lfo_slot_t *sl = dub_lfo_slot_view(t);

        if (pg == 0u) {
            if (slot == 0) {
                if (dub_is_seq_lfo_tgt(t))
                    dub_fmt_lfo_seq_short(s, t);
                else
                    copy_str(s, LFO_TGT_NAMES[t]);
            } else if (slot == 1) {
                uint32_t bpm = g.cfg.p.bpm ? g.cfg.p.bpm : 120u;
                uint8_t rate_show = fm_lfo_rate_clamp(sl->rate, bpm);
                uint8_t rate_eff = fm_lfo_rate_resolved(rate_show, bpm);
                const char *nd = fm_lfo_note_div(rate_show, bpm);
                uint32_t hz100 = fm_lfo_hz100(rate_eff);
                uint32_t n = 0;

                if (nd)
                    copy_str(s, nd);
                else {
                    u8_dec(s, hz100 / 100u);
                    while (s[n])
                        n++;
                    s[n++] = '.';
                    s[n++] = (char)('0' + ((hz100 % 100u) / 10u));
                    s[n++] = 'H';
                    s[n++] = 'z';
                    s[n] = 0;
                }
            } else if (slot == 3)
                copy_str(s, fm_lfo_shape_name(sl->wave));
            else
                u8_show100_dec(s, sl->depth);
        } else if (pg == 1u) {
            if (slot == 0)
                u8_show100_dec(s, sl->phase);
            else if (slot == 1)
                bip50_show(s, sl->skew);
            else if (slot == 2)
                u8_show100_dec(s, sl->smooth);
            else
                fm_lfo_fmt_depth(s, (int32_t)sl->fold);
        } else if (slot == 0)
            copy_str(s, FM_LFO_ALG_NAMES[sl->algo % FM_LFO_NALGOS]);
        else if (slot == 1)
            fm_lfo_fmt_ratio(s, sl->ratio);
        else if (slot == 2)
            fm_lfo_fmt_depth(s, (int32_t)sl->index);
        else
            fm_lfo_fmt_depth(s, (int32_t)sl->fb);
    }
}

static void apply_cc(uint8_t cc, uint8_t val)
{
    if (cc >= 14u && cc <= 17u) {
        uint8_t sl = (uint8_t)(cc - 14u);
        g.knob_ms[sl] = 0u;
        knob_adjust(sl, (int32_t)val - 64);
    } else if (cc == 18u) {
        uint8_t e = (uint8_t)((val * FM_ERA_KNOB_N) / 128u);
        if (e >= FM_ERA_KNOB_N)
            e = (uint8_t)(FM_ERA_KNOB_N - 1u);
        cycle_era((int32_t)e - (int32_t)g.cfg.p.era);
    } else if (cc == 19u) {
        dub_params_t prev;
        uint8_t cur_era = g.cfg.p.era;
        uint8_t keep_bass = g.cfg.p.bass_model;
        uint8_t keep_drum_kit = g.cfg.p.drum_kit;

        dub_morph_snapshot_voice(&prev);
#if defined(FM_PGEN_ON)
        if (!fm_era_is_artist(cur_era))
            g.cfg.p.plate = fm_pgen_goto(cur_era, (int32_t)val);
        else
#endif
        {
            g.cfg.p.plate = (uint8_t)((val * GENRE_PRESET_N) / 128u);
            if (g.cfg.p.plate >= GENRE_PRESET_N)
                g.cfg.p.plate = (uint8_t)(GENRE_PRESET_N - 1u);
        }
        genre_plate_preset_apply(&g.cfg.p, cur_era, g.cfg.p.plate, keep_bass, keep_drum_kit, 1u);
        dub_plate_seq_reload_song_lanes(g.cfg.p.plate);
        dub_sel_sync_saved_lvls();
        dub_plate_seq_morph_prep();
        dub_home_morph_flash_arm(DUB_FLASH_XF);
        if (g.cfg.p.era != cur_era) {
            dub_home_morph_flash_arm(DUB_FLASH_EV);
            dub_lfo_fm_defaults(&g.cfg.p);
            dub_era_home_macros_apply_era(g.cfg.p.era, 1u);
        }
        dub_era_lfo_apply(g.cfg.p.era, g.cfg.p.plate);
        dub_morph_retarget_begin(&prev);
        dub_load_plate_song_groove(g.cfg.p.plate);
        mark_dirty();
    }
}

static const char *stage_name(void)
{
    if (g.view == VIEW_SEQ) {
        if (g.song_setup_latch)
            return "SETUP";
        if (g.seq_page == 5u)
            return "GROOVE";
        if (g.seq_page == 1u)
            return "PROB";
        return "SEQ";
    }
#if !defined(GENRE_BASS)
    if (gv_stage_name())
        return gv_stage_name();
    if (g.bank == BANK_DRUM) {
        uint8_t pg = bank_page();
        if (pg == 0u)
            return "VOICE";
        if (pg == 1u)
            return "TONE";
        return "MIX";
    }
#else
    if (g.bank == BANK_GROOVE)
        return bank_page() == 0u ? "GROOVE" : "SETUP";
#endif
    if (g.view == VIEW_HOME || g.bank == BANK_HOME)
        return GENRE_APP_NAME;
    if (g.bank == BANK_CHORD) {
#if defined(GENRE_BASS)
        return "ENV";
#else
        uint8_t pg = bank_page();
        if (pg == 2u)
            return "MIX";
        if (pg == 0u)
            return "SOUND";
        return "CHORD";
#endif
    }
    if (g.bank == BANK_BASS) {
#if defined(GENRE_BASS)
        uint8_t pg = bank_page();

        if (pg == 2u)
            return "MIX";
        return "VOICE";
#else
        uint8_t pg = bank_page();
        if (pg == 2u)
            return "MIX";
        if (pg == 0u)
            return "SOUND";
        return "BASS";
#endif
    }
    if (g.bank == BANK_FX) {
        uint8_t fx_pg = bank_page();

        if (fx_pg == 2u)
            return "REVERB";
        if (fx_pg == 1u)
            return "SPACE";
        return "ECHO";
    }
    if (g.bank == BANK_LFO)
        return "LFO";
#if defined(GENRE_BASS)
    return "BASS";
#else
    return "DRUM";
#endif
}

static uint16_t color_scale_rgb(uint16_t col, int32_t scale256)
{
    int32_t r = ((col >> 11) & 31) << 3;
    int32_t g_c = ((col >> 5) & 63) << 2;
    int32_t b = (col & 31) << 3;

    if (scale256 <= 0)
        return C_BLACK;
    r = (r * scale256) >> 8;
    g_c = (g_c * scale256) >> 8;
    b = (b * scale256) >> 8;
    if (r > 255) r = 255;
    if (g_c > 255) g_c = 255;
    if (b > 255) b = 255;
    return RGB(r, g_c, b);
}

static uint16_t color_sat_wave(uint16_t base_col, int32_t glow)
{
    return ko_color_sat_wave(base_col, glow);
}

static void wave_ctx_fill(ko_wave_ctx_t *w_out)
{
    uint32_t sub_m = g.synth.meter[0] > g.synth.meter[2] ? g.synth.meter[0] : g.synth.meter[2];
    uint32_t ch_m = g.synth.meter[1];
    ko_wave_ctx_t w = {0};
#ifdef GENRE_HEAVY_PSYCH
    /* Plain drive/saturation glow from drum + bass/chord energy (no skank or tape-echo phase) */
    int32_t energy = 64 + (int32_t)g.cfg.p.drive + (int32_t)(sub_m >> 1) + (int32_t)(ch_m >> 1);

    w.static_glow = 1;
    w.glow = (energy > 256 ? 256 : energy) >> 1;
#else
    /* Groove-driven saturation wave: combines sub/kick pulse, offbeat skank feel & tape-echo phase */
    uint32_t step_ph = g.playing
        ? (((uint32_t)g.step << 28) + dub_sample_acc_frac_q28(g.sample_acc, g.samples_per_step))
        : (fm1_ms * 1200000u);
    /* Offbeat skank swell (peaks on "&" beats: step 2, 6, 10, 14) + swing lilt */
    uint8_t sk_sw = g.voice.p.swing ? g.voice.p.swing : g.cfg.p.swing;
    int32_t energy = 96 + (int32_t)(sub_m >> 1) + (int32_t)(ch_m >> 1);

    w.step_ph = step_ph;
    w.skank_ph = (step_ph << 2) - 0x40000000u + ((uint32_t)(sk_sw - 50u) << 24);
    /* Travelling wave across characters: slower/wider with longer echo time, deeper with audio energy */
    w.idx_ph = 0x1B000000u - (uint32_t)g.cfg.p.dly_time * 0x00120000u;
    w.energy = energy > 256 ? 256 : energy;
    w.sine_q64 = dub_sine_q64;
#endif
    *w_out = w;
}

typedef struct {
    uint32_t start_ms;
    uint8_t active;
} arp_note_anim_t;

static arp_note_anim_t arp_note_anim;

typedef struct {
    char out[12];
    uint32_t start_ms;
    uint8_t active;
} dj_param_anim_t;

static dj_param_anim_t dj_param_anim;

/* Side-label animation key: note divisions / drop-sil lanes must not retrigger each step. */
static const char *dj_toast_side_anim_key(const char *side)
{
    uint32_t i;
    static const char *const LANES[] = { "KICK", "SNARE", "HAT", "LEAD", "BASS", "CHORD" };

    if (!side || !side[0])
        return side;
    if (side[0] >= '1' && side[0] <= '9' && side[1] == '/')
        return "ECHO TIME";
    for (i = 0u; i < 6u; i++) {
        if (perf_toast_policy_same_msg(side, LANES[i]))
            return "DROP SIL";
    }
    return side;
}

static void dub_note_to_str(uint8_t nn, char *s, uint32_t n)
{
    uint32_t i = 0u;

    if (nn >= 128u) {
        s[0] = '-';
        s[1] = 0;
        return;
    }
    copy_str(s, NOTE_NAMES[nn % 12u]);
    while (s[i])
        i++;
    if (nn >= 12u && i + 2u < n) {
        uint32_t o = (uint32_t)(nn / 12u) - 1u;

        if (o < 10u)
            s[i++] = (char)('0' + o);
    }
    s[i] = 0;
}

static uint8_t dub_u8_gcd(uint8_t a, uint8_t b)
{
    while (b != 0u) {
        uint8_t t = (uint8_t)(a % b);

        a = b;
        b = t;
    }
    return a;
}

static void dub_loop_len_to_str(uint8_t len, char *s, uint32_t n)
{
    uint8_t num;
    uint8_t den;
    uint8_t g;
    uint32_t i = 0u;

    if (n < 5u) {
        s[0] = 0;
        return;
    }
    if (len == 0u)
        len = 1u;
    if (len > (uint8_t)DUB_SEQ_STEPS)
        len = (uint8_t)DUB_SEQ_STEPS;
    /* Denominator = one bar (32 steps) → 1→1/32 … 128→4/1 */
    num = len;
    den = (uint8_t)DUB_STEPS;
    g = dub_u8_gcd(num, den);
    num = (uint8_t)(num / g);
    den = (uint8_t)(den / g);
    if (num >= 10u && i + 2u < n) {
        s[i++] = (char)('0' + num / 10u);
        s[i++] = (char)('0' + num % 10u);
    } else
        s[i++] = (char)('0' + num);
    if (i + 1u < n)
        s[i++] = '/';
    if (den >= 10u && i + 2u < n) {
        s[i++] = (char)('0' + den / 10u);
        s[i++] = (char)('0' + den % 10u);
    } else if (i < n)
        s[i++] = (char)('0' + den);
    s[i] = 0;
}

static void dub_key_toast_show_part(uint8_t ln, const char *part, uint8_t with_voice)
{
    const char *voice;
    uint32_t i = 0u;
    uint32_t pi;

    /* ln 0..5 = voice color; 0xFF = neutral (LOOP length toast) */
    if (ln >= 6u && ln != 255u)
        return;
    if (fm1_ms - g.key_toast_arm_ms < 150u)
        return;
    if (!part || !part[0])
        return;
    if (with_voice) {
        voice = seq_lane_name(ln);
        while (voice[i] && i + 1u < sizeof g.key_toast) {
            g.key_toast[i] = voice[i];
            i++;
        }
        if (i + 1u + 3u < sizeof g.key_toast) {
            g.key_toast[i++] = ' ';
            pi = 0u;
            while (part[pi] && i + 1u < sizeof g.key_toast)
                g.key_toast[i++] = part[pi++];
        } else {
            pi = 0u;
            i = 0u;
            while (part[pi] && i + 1u < sizeof g.key_toast)
                g.key_toast[i++] = part[pi++];
        }
    } else {
        pi = 0u;
        while (part[pi] && i + 1u < sizeof g.key_toast)
            g.key_toast[i++] = part[pi++];
    }
    g.key_toast[i] = 0;
    g.key_toast_lane = ln;
    g.key_toast_arm_ms = fm1_ms;
    g.key_toast_until = fm1_ms + 2500u;
    if (g.cfg.p.arp_mode != 0u || g.dj.on) {
        arp_note_anim.start_ms = fm1_ms;
        arp_note_anim.active = 1u;
    }
    g.ui_dirty = 1u;
}

static void dub_key_toast_show(uint8_t ln, uint8_t nn)
{
    char note[8];

    dub_note_to_str(nn, note, sizeof note);
    dub_key_toast_show_part(ln, note, 1u);
}

static void dub_loop_toast_show(uint8_t loop_len)
{
    char len_s[8];

    dub_loop_len_to_str(loop_len, len_s, sizeof len_s);
    dub_key_toast_show_part(255u, len_s, 0u);
}

static uint16_t dub_seq_lane_play_col(uint8_t ln)
{
    ln %= DUB_SEQ_LANES;
    if (ln == 1u)
        return RGB(140, 210, 255);
    if (ln == 2u)
        return RGB(110, 235, 190);
    if (ln == 3u)
        return C_ACC;
    if (ln == 4u)
        return ERA_COL[g.cfg.p.era % GENRE_ERA_N];
    if (ln == 5u)
        return RGB(255, 190, 95);
    return C_HI;
}

static int8_t perf_toast_gesture_lane(uint8_t ag)
{
    if (ag == GEST_SOLO_BASS)
        return 4;
    if (ag == GEST_SOLO_DRUM || ag == GEST_FLAM)
        return 1;
    return -1;
}

/* SEQ pg4 ensemble value readout (same as manual knob value column). */
static const char *dj_seq_ens_value_label(void)
{
    int32_t ed = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_ENSEMBLE);
    uint8_t disp = fm_dj_seq_ens_ov_display_ens(&g.dj_ens_ov, ed, DUB_ENS_MUTE);

    if (disp == 0xfeu)
        return fm_dj_seq_flip_toast_label();
    if (disp != 0xffu) {
        if (disp == FM_SEQ_ENS_CUSTOM)
            return "CUSTOM";
        return GENRE_ENS_NAMES[disp];
    }
    return 0;
}

static const char *dj_seq_flip_value_label(void)
{
    uint8_t on = g.dj_flip_overlay;

    if (!on && fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_FLIP) == 0)
        on = g.seq_flip_state;
    return FM_SEQ_FLIP_NAMES[on & 1u];
}

static const char *dj_toast_seq_side(uint8_t tgt)
{
    int32_t d;

    if (tgt == FM_DJ_TGT_ENSEMBLE) {
        const char *v;

        d = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_ENSEMBLE);
        if (!fm_dj_seq_ens_live(d))
            return 0;
        v = dj_seq_ens_value_label();
        return v;
    }
    if (tgt == FM_DJ_TGT_FLIP) {
        d = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_FLIP);
        if (!fm_dj_toast_delta_audible(d) && !g.dj_flip_overlay)
            return 0;
        return dj_seq_flip_value_label();
    }
    return 0;
}

/* Visible knob/LFO label for the active DJ target (genre macros via LFO_TGT_NAMES). */
static const char *dj_visible_target_name(void)
{
    uint8_t lfo;

    if (g.dj.target >= FM_DJ_TGT_N)
        return 0;
    switch (g.dj.target) {
    case FM_DJ_TGT_MOTION:
        lfo = DUB_LFO_M_MOTION;
        break;
    case FM_DJ_TGT_FLUX:
        lfo = DUB_LFO_M_FLUX;
        break;
    case FM_DJ_TGT_WOB:
        lfo = DUB_LFO_M_WOB;
        break;
    case FM_DJ_TGT_ISO:
        lfo = DUB_LFO_M_ISO;
        break;
    case FM_DJ_TGT_SPACE:
        lfo = DUB_LFO_M_SPACE;
        break;
    case FM_DJ_TGT_WEIGHT:
        lfo = DUB_LFO_M_WEIGHT;
        break;
    case FM_DJ_TGT_DUB:
        lfo = DUB_LFO_M_DUB;
        break;
    case FM_DJ_TGT_CUTOFF:
        return "CUTOFF"; /* CHORD/BASS pg0 knob */
    case FM_DJ_TGT_FB:
        return "FEEDBK"; /* FX pg0 — not LFO "E FDBK" */
    case FM_DJ_TGT_DENS_H:
#if defined(GENRE_BASS)
        return "DRIVE";
#else
        return LFO_TGT_NAMES[DUB_LFO_HAT]; /* "HAT" / "HATS" / "RIDE" */
#endif
    case FM_DJ_TGT_DENS_K:
#if defined(GENRE_BASS)
        return "CUT";
#else
        return LFO_TGT_NAMES[DUB_LFO_DENS_K];
#endif
    case FM_DJ_TGT_SEQ_ROT:
        return "ROTATE"; /* SEQ pg0 — not LFO "S ROT" */
    case FM_DJ_TGT_SEQ_VEL:
        return DUB_SEQ_LFO_LONG[3];
    case FM_DJ_TGT_SEQ_TRIG:
        return DUB_SEQ_LFO_LONG[4];
    case FM_DJ_TGT_SEQ_GATE:
        return DUB_SEQ_LFO_LONG[5];
    case FM_DJ_TGT_SEQ_PITCH:
        return DUB_SEQ_LFO_LONG[6];
    case FM_DJ_TGT_ENSEMBLE:
        return fm_dj_seq_ens_toast_label();
    case FM_DJ_TGT_FLIP:
        return fm_dj_seq_flip_toast_label();
    case FM_DJ_TGT_SEQ_CYC:
        return "CYCLE";
    case FM_DJ_TGT_SEQ_REP:
        return "REPEAT";
    default:
        return 0;
    }
    if (lfo >= DUB_LFO_TGT_N)
        return 0;
    return LFO_TGT_NAMES[lfo];
}

/* DJ toast: LFO long label without redundant "MACRO " (LFO UI keeps full string). */
static const char *dj_toast_lfo_long(uint8_t lfo)
{
    const char *s;

    if (lfo >= DUB_LFO_TGT_N)
        return 0;
    s = LFO_TGT_LONG[lfo];
    if (s[0] == 'M' && s[1] == 'A' && s[2] == 'C' && s[3] == 'R' && s[4] == 'O' && s[5] == ' ' && s[6])
        return s + 6;
    return s;
}

/* Toast side label (long names; macros via dj_toast_lfo_long). */
static const char *dj_toast_target_long(uint8_t tgt)
{
    if (tgt >= FM_DJ_TGT_N)
        return 0;
    switch (tgt) {
    case FM_DJ_TGT_MOTION:
        return dj_toast_lfo_long(DUB_LFO_M_MOTION);
    case FM_DJ_TGT_FLUX:
        return dj_toast_lfo_long(DUB_LFO_M_FLUX);
    case FM_DJ_TGT_WOB:
        return dj_toast_lfo_long(DUB_LFO_M_WOB);
    case FM_DJ_TGT_ISO:
        return dj_toast_lfo_long(DUB_LFO_M_ISO);
    case FM_DJ_TGT_SPACE:
        return dj_toast_lfo_long(DUB_LFO_M_SPACE);
    case FM_DJ_TGT_WEIGHT:
        return dj_toast_lfo_long(DUB_LFO_M_WEIGHT);
    case FM_DJ_TGT_DUB:
        return dj_toast_lfo_long(DUB_LFO_M_DUB);
    case FM_DJ_TGT_CUTOFF:
        return dj_toast_lfo_long(DUB_LFO_CUT_C);
    case FM_DJ_TGT_FB:
        return "ECHO FEEDBACK";
    case FM_DJ_TGT_DENS_H:
#if defined(GENRE_JAZZ)
        return "RIDE DENSITY";
#elif defined(GENRE_SOUL)
        return "TAMB DENSITY";
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        return "HATS DENSITY";
#else
        return "HAT DENSITY";
#endif
    case FM_DJ_TGT_SEQ_ROT:
        return dj_toast_lfo_long(DUB_LFO_SEQ_ROT);
    case FM_DJ_TGT_SEQ_VEL:
        return DUB_SEQ_LFO_LONG[3];
    case FM_DJ_TGT_SEQ_TRIG:
        return DUB_SEQ_LFO_LONG[4];
    case FM_DJ_TGT_SEQ_GATE:
        return DUB_SEQ_LFO_LONG[5];
    case FM_DJ_TGT_SEQ_PITCH:
        return DUB_SEQ_LFO_LONG[6];
    case FM_DJ_TGT_DENS_K:
        return "KICK DENSITY";
    case FM_DJ_TGT_ENSEMBLE:
        return fm_dj_seq_ens_toast_label();
    case FM_DJ_TGT_FLIP:
        return fm_dj_seq_flip_toast_label();
    case FM_DJ_TGT_SEQ_CYC:
        return "CYCLE";
    case FM_DJ_TGT_SEQ_REP:
        return "REPEAT";
    default:
        return 0;
    }
}

/* Alternates track/plate vs era each DROP; toast reads last action (flag toggles after on_drop). */
static uint8_t fm_dj_drop_do_plate = 1u;
static uint8_t fm_dj_xf_seq;
static uint8_t fm_dj_drop_last_was_plate = 1u;
/* Sticky mode label only — param rides beside via cv_draw_toast_arp_note (like ARP|note). */
static const char *dj_toast_msg(void)
{
    return g.dj.on ? "DJ" : 0;
}

enum { DJ_FX_FLASH_MS = 90u, DJ_FX_FLASH_N = 6u };
static uint32_t dj_fx_flash_ms;
static uint8_t dj_fx_flash_active;

static void dj_mode_toggled(void)
{
    plate_era_toast_set(g.dj.on ? "DJ ON" : "DJ OFF", fm_era_knob_col(g.cfg.p.era));
    dj_fx_flash_ms = fm1_ms;
    dj_fx_flash_active = 1u;
}

static int dj_toast_phrase_role_active(void)
{
    if (!g.dj.on)
        return 0;
    return fm_dj_any_slot_role(&g.dj, FM_DJ_ROLE_DROP) ||
           fm_dj_any_slot_role(&g.dj, FM_DJ_ROLE_BREAK);
}

static perf_toast_dj_ctx_t perf_toast_dj_ctx = { 0xffu, 0xffu, 0xffu, 0xffu };
static fm_dj_toast_tgt_hold_t g_dj_toast_tgt_hold;
static fm_dj_lfo_toast_hold_t g_dj_lfo_toast_hold;

static uint8_t dj_toast_sticky_kind(void)
{
    static uint8_t held = CV_TOAST_OK;
    static uint16_t flip_ticks;
    uint8_t want = dj_toast_phrase_role_active() ? CV_TOAST_WARN : CV_TOAST_OK;

    if (want == held) {
        flip_ticks = 0u;
        return held;
    }
    if (flip_ticks < 48u) {
        flip_ticks++;
        return held;
    }
    held = want;
    flip_ticks = 0u;
    return held;
}

static void perf_toast_dj_holds_reset(void)
{
    fm_dj_toast_tgt_hold_reset(&g_dj_toast_tgt_hold);
    fm_dj_lfo_toast_hold_reset(&g_dj_lfo_toast_hold);
}

static const char *dj_toast_gesture_side_label(uint8_t ag)
{
    switch (ag) {
    case GEST_FILL:
        return "FILL";
    case GEST_BREAK:
        return "BREAK";
    case GEST_SOLO_DRUM:
        return "DRUM FLARE";
    case GEST_SOLO_BASS:
        return "BASS FLARE";
    case GEST_SOLO_BAND:
        return "BAND FLARE";
    case GEST_FLAM:
        return "FLAM!";
    default:
        return 0;
    }
}

static const char *dj_toast_ephemeral_gesture_side(void)
{
    uint8_t ag;

    if (g.dj_gest_flam_steps > 0u)
        return "FLAM!";
    ag = gesture_active();
    return dj_toast_gesture_side_label(ag);
}

static const char *dj_toast_lfo_overlay_side(void)
{
    static char buf[20];
    uint8_t lt;
    const char *n;
    uint32_t o;

    lt = fm_dj_lfo_toast_pick_tgt_stable(&g.dj_lfo_ov, &g.dj, g.lfo_slots_eff, g.lfo_rt, &g_dj_lfo_toast_hold);
    if (lt >= DUB_LFO_TGT_N)
        return 0;
    n = LFO_TGT_NAMES[lt];
    if (!n || !n[0])
        return 0;
    o = 0u;
    while (n[o] && o + 5u < sizeof buf) {
        buf[o] = n[o];
        o++;
    }
    if (o + 4u >= sizeof buf)
        return n;
    buf[o++] = ' ';
    buf[o++] = 'L';
    buf[o++] = 'F';
    buf[o++] = 'O';
    buf[o] = 0;
    return buf;
}

static uint8_t dub_dj_gest_loop_toast_live(void);
static uint8_t dub_dj_gest_arp_toast_live(void);
static const char *dub_arp_mode_toast_label(uint8_t mode);

/* Right-hand toast: phrase roles win over macro motion; sticky for whole slot bar. */
static const char *dj_toast_param(void)
{
    const char *side;
    uint8_t tgt;

    if (!g.dj.on)
        return 0;
    if (fm_dj_any_slot_role(&g.dj, FM_DJ_ROLE_DROP))
        return "DROP";
    if (g.dj.break_arm >= 2u && g.dj.phrase_bar + 2u >= g.dj.break_arm &&
        g.dj.phrase_bar < g.dj.drop_arm) {
        const char *tl = fm_dj_trans_toast_label(&g.dj);

        if (tl)
            return tl;
    }
    {
        const char *bl = fm_dj_build_toast_label(&g.dj);

        if (bl)
            return bl;
    }
    {
        const char *fxl = fm_dj_fx_toast_label_vp(&g.dj, &g.dj_fx_ov, &g.voice.p, &g.cfg.p);

        if (fxl)
            return fxl;
    }
    if (dub_dj_gest_loop_toast_live())
        return "LOOP";
    if (dub_dj_gest_arp_toast_live())
        return dub_arp_mode_toast_label(g.cfg.p.arp_mode);
    if (g.playing && fm_dj_trans_is_drop_path(&g.dj) &&
        fm_dj_drop_sil_lane_mask(&g.dj, 1u, g.bar_step, DUB_STEPS) != 0u &&
        g.dj.drop_sil_keep < DUB_SEQ_LANES)
        return SEQ_LANE[g.dj.drop_sil_keep];
    side = dj_toast_ephemeral_gesture_side();
    if (side)
        return side;
    side = dj_toast_lfo_overlay_side();
    if (side)
        return side;
    tgt = fm_dj_toast_target_stable(&g.dj, &g_dj_toast_tgt_hold);
    if (tgt >= FM_DJ_TGT_N)
        return 0;
    side = dj_toast_seq_side(tgt);
    if (side)
        return side;
    if (tgt == FM_DJ_TGT_ENSEMBLE || tgt == FM_DJ_TGT_FLIP)
        return 0;
    if (!fm_dj_toast_delta_audible(fm_dj_delta_for_tgt(&g.dj, tgt)))
        return 0;
    side = dj_toast_target_long(tgt);
    if (!side) {
#if defined(GENRE_POP)
        if (tgt == FM_DJ_TGT_FLUX)
            side = "DENSITY";
        else
#endif
            side = dj_visible_target_name();
    }
    return side;
}

static int dj_toast_has_side_content(void)
{
    const char *side;

    if (!g.dj.on)
        return 0;
    if (fm1_ms < g.key_toast_until && g.key_toast[0])
        return 1;
    side = dj_toast_param();
    return side && side[0];
}

/* Playing hides the bare "DJ" main unless a side param rides. */
static int dj_toast_main_live(void)
{
    return perf_toast_policy_dj_main_live(g.dj.on, g.playing, dj_toast_has_side_content());
}

static int perf_toast_is_dj_msg(const char *msg)
{
    return msg && msg[0] == 'D' && msg[1] == 'J' && msg[2] == 0;
}

static uint16_t dj_role_col(void)
{
    switch (g.dj.role) {
    case FM_DJ_ROLE_RIDE:
        return RGB(80, 220, 255);
    case FM_DJ_ROLE_THROW:
        return RGB(255, 180, 60);
    case FM_DJ_ROLE_FILTER:
        return RGB(255, 100, 220);
    case FM_DJ_ROLE_BREAK:
        return RGB(255, 70, 70);
    case FM_DJ_ROLE_DROP:
        return C_WHITE;
    default:
        return C_ACC;
    }
}

/* Panel button that owns the active DJ target; 0xff = none. */
static uint8_t dj_target_btn(void)
{
    if (!g.dj.on || g.dj.target >= FM_DJ_TGT_N)
        return 0xffu;
    switch (g.dj.target) {
    case FM_DJ_TGT_MOTION:
    case FM_DJ_TGT_FLUX:
    case FM_DJ_TGT_WOB:
    case FM_DJ_TGT_ISO:
    case FM_DJ_TGT_SPACE:
    case FM_DJ_TGT_WEIGHT:
    case FM_DJ_TGT_DUB:
        return BTN_HOME;
    case FM_DJ_TGT_CUTOFF:
        return BTN_ENV;
    case FM_DJ_TGT_FB:
        return BTN_FX;
    case FM_DJ_TGT_DENS_H:
    case FM_DJ_TGT_DENS_K:
        return BTN_SEL;
    case FM_DJ_TGT_SEQ_ROT:
    case FM_DJ_TGT_SEQ_VEL:
    case FM_DJ_TGT_SEQ_TRIG:
    case FM_DJ_TGT_SEQ_GATE:
    case FM_DJ_TGT_SEQ_PITCH:
    case FM_DJ_TGT_ENSEMBLE:
    case FM_DJ_TGT_FLIP:
    case FM_DJ_TGT_SEQ_CYC:
    case FM_DJ_TGT_SEQ_REP:
        return BTN_SEQ;
    default:
        return 0xffu;
    }
}

static int dj_seq_param_knob_live(uint8_t slot, uint8_t tgt)
{
    uint8_t expect;
    uint8_t lane;
    const fm_dj_slot_t *s;

    if (!g.dj.on || g.dj.target != tgt || !fm_dj_tgt_is_seq_param(tgt))
        return 0;
    expect = (uint8_t)(tgt - FM_DJ_TGT_SEQ_VEL);
    if (fm_seq_p1_fn(slot) != expect)
        return 0;
    if (g.dj.lead_slot >= FM_DJ_SLOT_N)
        return 0;
    s = &g.dj.slot[g.dj.lead_slot];
    if (s->target != tgt)
        return 0;
    lane = s->lane;
    if (lane < 6u && !seq_lane_is_all() && seq_lane_sel() != lane)
        return 0;
    return 1;
}

static int dub_dj_knob_modulated(uint8_t slot)
{
    uint8_t ix;
    uint8_t pg;

    if (!g.dj.on)
        return 0;
    if (g.bank == BANK_FX && fm_dj_fx_knob_overlay_live(&g.dj_fx_ov, bank_page(), slot))
        return 1;
    if (g.dj.target >= FM_DJ_TGT_N)
        return 0;
    if (g.view == VIEW_SEQ) {
        if (g.dj.target == FM_DJ_TGT_SEQ_ROT && g.seq_page == 0u && slot == 2u)
            return 1;
        if (g.dj.target == FM_DJ_TGT_ENSEMBLE && g.seq_page == 4u && slot == 2u)
            return 1;
        if (g.dj.target == FM_DJ_TGT_FLIP && g.seq_page == 4u && slot == 3u)
            return 1;
        if (g.dj.target == FM_DJ_TGT_SEQ_CYC && g.seq_page == 4u && slot == 0u)
            return 1;
        if (g.dj.target == FM_DJ_TGT_SEQ_REP && g.seq_page == 4u && slot == 1u)
            return 1;
        if (g.seq_page == 1u) {
            if (dj_seq_param_knob_live(slot, FM_DJ_TGT_SEQ_VEL))
                return 1;
            if (dj_seq_param_knob_live(slot, FM_DJ_TGT_SEQ_TRIG))
                return 1;
            if (dj_seq_param_knob_live(slot, FM_DJ_TGT_SEQ_GATE))
                return 1;
            if (dj_seq_param_knob_live(slot, FM_DJ_TGT_SEQ_PITCH))
                return 1;
        }
        return 0;
    }
    switch (g.dj.target) {
    case FM_DJ_TGT_MOTION:
    case FM_DJ_TGT_FLUX:
    case FM_DJ_TGT_WOB:
    case FM_DJ_TGT_ISO:
    case FM_DJ_TGT_SPACE:
    case FM_DJ_TGT_WEIGHT:
    case FM_DJ_TGT_DUB:
        if (!dub_home_macro_knob_ix(slot, &ix))
            return 0;
        return (g.dj.target == FM_DJ_TGT_MOTION && ix == 8u) ||
               (g.dj.target == FM_DJ_TGT_FLUX && ix == 9u) ||
               (g.dj.target == FM_DJ_TGT_WOB && ix == 2u) ||
               (g.dj.target == FM_DJ_TGT_ISO && ix == 3u) ||
               (g.dj.target == FM_DJ_TGT_SPACE && ix == 4u) ||
               (g.dj.target == FM_DJ_TGT_WEIGHT && ix == 5u) ||
               (g.dj.target == FM_DJ_TGT_DUB && ix == 7u);
    case FM_DJ_TGT_FB:
        return g.bank == BANK_FX && bank_page() == 0u && slot == 2u;
    case FM_DJ_TGT_CUTOFF:
        pg = bank_page();
        if (g.bank == BANK_CHORD && pg == 0u && slot == 1u)
            return 1;
        if (g.bank == BANK_BASS && pg == 0u && slot == 1u)
            return 1;
        return 0;
    case FM_DJ_TGT_DENS_H:
        /* Hats dens lives on voice dens; mark DRUM VOICE dens when hats selected. */
        return g.bank == BANK_DRUM && bank_page() == 0u && slot == 2u &&
               sel_drm_ix() == 2u;
    case FM_DJ_TGT_DENS_K:
        return g.bank == BANK_DRUM && bank_page() == 0u && sel_drm_ix() == 0u &&
               (slot == 0u || slot == 2u);
    default:
        return 0;
    }
}

static const char *dub_arp_mode_toast_label(uint8_t mode)
{
#ifdef GENRE_HEAVY_PSYCH
    static const char *const AN[6] = {"", "FUZZ", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_HOUSE)
    static const char *const AN[6] = {"", "JACK", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_JAZZ)
    static const char *const AN[6] = {"", "COMP", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)
    static const char *const AN[6] = {"", "CELL", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_TRIPHOP)
    static const char *const AN[6] = {"", "DUST", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_HIPHOP)
    static const char *const AN[6] = {"", "SNAP", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_POP)
    static const char *const AN[6] = {"", "HOOK", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_FUNK)
    static const char *const AN[6] = {"", "CHOP", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_SOUL)
    static const char *const AN[6] = {"", "STAB", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_BASS)
    static const char *const AN[6] = {"", "OFFBEAT", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_TECHNO)
    static const char *const AN[6] = {"", "PUMP", "CHOP", "DOT", "ROLL", "LOOP"};
#else
    static const char *const AN[6] = {"", "SKANK", "CHOP", "DOT", "ROLL", "LOOP"};
#endif

    return AN[mode % 6u];
}

static uint8_t dub_dj_gest_arp_toast_live(void)
{
    return perf_toast_policy_dj_arp_overlay_live(g.dj.on, g.dj_gest_ov, g.dj_gest_bars, g.cfg.p.arp_mode) ? 1u : 0u;
}

static uint8_t dub_dj_gest_loop_toast_live(void)
{
    return perf_toast_policy_dj_loop_audible(g.dj.on, g.dj_gest_ov == FM_DJ_GEST_OV_LOOP, g.dj_gest_bars,
                                             g.loop_active, g.loop_pending) ?
               1u :
               0u;
}


static int perf_toast_pick_overlay(const char **out_msg, uint8_t *out_kind, uint32_t *out_end, int8_t *out_voice_ln)
{
    const char *msg = 0;
    uint8_t kind = CV_TOAST_OK;
    uint32_t end = 0;
    uint8_t ag;
    int8_t voice_ln = -1;

    if (g.voice.freeze) {
        msg = "FRZ";
        kind = CV_TOAST_WARN;
    } else if (g.console) {
        msg = "CONSOLE";
        kind = CV_TOAST_WARN;
    } else if (fm1_ms < g.saved_until) {
        msg = "SAVED";
        kind = CV_TOAST_OK;
        end = g.saved_until;
    } else if (fm1_ms < g.plate_home_until) {
        msg = "HOME";
        kind = CV_TOAST_OK;
        end = g.plate_home_until;
    } else if (fm1_ms < g.key_toast_until && g.key_toast[0] && g.cfg.p.arp_mode == 0u && !g.dj.on) {
        msg = g.key_toast;
        kind = CV_TOAST_OK;
        end = g.key_toast_until;
        voice_ln = (int8_t)g.key_toast_lane;
    } else {
        ag = gesture_active();
        msg = ag == GEST_FILL ? "FILL" :
              ag == GEST_BREAK ? "BREAK" :
              ag == GEST_SOLO_DRUM ? "DRUM FLARE" :
              ag == GEST_SOLO_BASS ? "BASS FLARE" :
              ag == GEST_SOLO_BAND ? "BAND FLARE" :
              ag == GEST_FLAM ? "FLAM!" : 0;
        if (msg && g.dj.on && perf_toast_policy_dj_absorbs_gesture_msg(msg, 1u))
            msg = 0;
        if (msg) {
            kind = g.gesture_latch ? CV_TOAST_OK : CV_TOAST_WARN;
            voice_ln = perf_toast_gesture_lane(ag);
        }
    }
    if (!msg || !msg[0])
        return 0;
    *out_msg = msg;
    *out_kind = kind;
    *out_end = end;
    *out_voice_ln = voice_ln;
    return 1;
}

static int perf_toast_pick_sticky(const char **out_msg, uint8_t *out_kind, uint32_t *out_end, int8_t *out_voice_ln)
{
    const char *msg = 0;
    uint8_t kind = CV_TOAST_OK;
    uint32_t end = 0;
    int8_t voice_ln = -1;

    if (dub_dj_gest_loop_toast_live()) {
        msg = dj_toast_msg();
        kind = CV_TOAST_WARN;
    } else if (dub_dj_gest_arp_toast_live()) {
        msg = dj_toast_msg();
        kind = CV_TOAST_WARN;
    } else if (perf_toast_policy_manual_arp_ok(g.dj.on, g.cfg.p.arp_mode)) {
        msg = dub_arp_mode_toast_label(g.cfg.p.arp_mode);
    } else if (dj_toast_main_live()) {
        msg = dj_toast_msg();
        kind = dj_toast_sticky_kind();
    }
    if (!msg || !msg[0])
        return 0;
    *out_msg = msg;
    *out_kind = kind;
    *out_end = end;
    *out_voice_ln = voice_ln;
    return 1;
}

enum { ARP_TOAST_SLIDE_MS = 150u };

static perf_toast_lane_t perf_toast_lanes[PERF_TOAST_LANE_N];

typedef struct {
    char out[12];
    uint32_t start_ms;
    uint8_t active;
} arp_toast_anim_t;

static arp_toast_anim_t arp_toast_anim;

static int perf_toast_same_msg(const char *a, const char *b)
{
    return perf_toast_policy_same_msg(a, b);
}

static int perf_toast_is_gesture_msg(const char *msg)
{
    return perf_toast_policy_is_gesture(msg);
}

static void perf_toast_lane_gesture_fade_out(perf_toast_lane_t *L)
{
    if (L->on && perf_toast_is_gesture_msg(L->msg) && L->end_ms == 0u)
        L->end_ms = fm1_ms + CV_TOAST_FADE_MS;
}

static void perf_toast_main_gesture_fade_out(void)
{
    perf_toast_lane_gesture_fade_out(&perf_toast_lanes[PERF_TOAST_LANE_MAIN]);
}

static void perf_toast_eph_gesture_fade_out(void)
{
    perf_toast_lane_gesture_fade_out(&perf_toast_lanes[PERF_TOAST_LANE_EPH]);
}

static int perf_toast_is_process_msg(const char *msg)
{
    return perf_toast_policy_is_process(msg);
}

static int perf_toast_pick_process(const char **out_msg, uint8_t *out_kind, uint32_t *out_end)
{
    uint8_t live;
    uint8_t bit;
    const char *msg;
    uint8_t kind = CV_TOAST_OK;
    uint32_t end;

    live = dub_home_morph_flash_live_mask();
    if (!live)
        return 0;
    bit = dub_home_morph_flash_pick_bit(live);
    msg = dub_home_morph_flash_msg(bit);
    if (!msg)
        return 0;
    end = g.playing ? 0u : g.home_morph_flash_ms;
    *out_msg = msg;
    *out_kind = kind;
    *out_end = end;
    return 1;
}

static int perf_toast_is_arp_label(const char *msg)
{
#ifdef GENRE_HEAVY_PSYCH
    static const char *const AN[6] = {"", "FUZZ", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_HOUSE)
    static const char *const AN[6] = {"", "JACK", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_JAZZ)
    static const char *const AN[6] = {"", "COMP", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)
    static const char *const AN[6] = {"", "CELL", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_TRIPHOP)
    static const char *const AN[6] = {"", "DUST", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_HIPHOP)
    static const char *const AN[6] = {"", "SNAP", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_POP)
    static const char *const AN[6] = {"", "HOOK", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_FUNK)
    static const char *const AN[6] = {"", "CHOP", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_BASS)
    static const char *const AN[6] = {"", "OFFBEAT", "CHOP", "DOT", "ROLL", "LOOP"};
#elif defined(GENRE_TECHNO)
    static const char *const AN[6] = {"", "PUMP", "CHOP", "DOT", "ROLL", "LOOP"};
#else
    static const char *const AN[6] = {"", "SKANK", "CHOP", "DOT", "ROLL", "LOOP"};
#endif
    uint32_t i;

    if (!msg || !msg[0])
        return 0;
    for (i = 1u; i < 6u; i++) {
        if (perf_toast_same_msg(msg, AN[i]))
            return 1;
    }
    return 0;
}

static int perf_toast_lane_sticky_main_occupied(void)
{
    perf_toast_lane_t *main = &perf_toast_lanes[PERF_TOAST_LANE_MAIN];

    if (!main->on)
        return 0;
    return perf_toast_is_dj_msg(main->msg) || perf_toast_is_arp_label(main->msg);
}

static void perf_toast_strip_dj_arp_stale(void)
{
    perf_toast_lane_t *main = &perf_toast_lanes[PERF_TOAST_LANE_MAIN];

    /* Mode still latched (manual page or DJ overlay): keep the label so the
     * mode-slide can read the previous badge. Only drop it once ARP is off. */
    if (!g.dj.on || g.cfg.p.arp_mode != 0u)
        return;
    if (main->on && perf_toast_is_arp_label(main->msg))
        perf_toast_policy_lane_clear(main);
    arp_toast_anim.active = 0u;
}

static void perf_toast_strip_dj_ephemeral_gestures(void)
{
    perf_toast_lane_t *main = &perf_toast_lanes[PERF_TOAST_LANE_MAIN];

    if (!g.dj.on)
        return;
    if (main->on && perf_toast_policy_gesture_ephemeral_msg(main->msg))
        perf_toast_policy_lane_clear(main);
}

static void perf_toast_sync_dj_strips(void)
{
    uint8_t gest_arp = dub_dj_gest_arp_toast_live();
    uint8_t gest_loop = dub_dj_gest_loop_toast_live();

    if (!perf_toast_policy_dj_ctx_changed(&perf_toast_dj_ctx, g.dj.on, g.cfg.p.arp_mode, gest_arp, gest_loop))
        return;
    perf_toast_dj_ctx.dj_on = g.dj.on;
    perf_toast_dj_ctx.arp_mode = g.cfg.p.arp_mode;
    perf_toast_dj_ctx.gest_arp = gest_arp;
    perf_toast_dj_ctx.gest_loop = gest_loop;
    if (!g.dj.on) {
        perf_toast_dj_holds_reset();
        perf_toast_lanes[PERF_TOAST_LANE_MAIN].side[0] = 0;
        perf_toast_lanes[PERF_TOAST_LANE_MAIN].side_end_ms = 0u;
    }
    perf_toast_strip_dj_arp_stale();
    perf_toast_strip_dj_ephemeral_gestures();
}

static void dub_arp_toast_clear(void)
{
    perf_toast_lane_t *main = &perf_toast_lanes[PERF_TOAST_LANE_MAIN];

    g.key_toast[0] = 0;
    g.key_toast_until = 0u;
    arp_note_anim.active = 0u;
    arp_toast_anim.active = 0u;
    if (main->on && perf_toast_is_arp_label(main->msg))
        perf_toast_policy_lane_clear(main);
    else {
        main->side[0] = 0;
        main->side_end_ms = 0u;
    }
}

static void dub_arp_set_mode(uint8_t mode)
{
    uint8_t prev = g.cfg.p.arp_mode;

    g.cfg.p.arp_mode = mode;
    if (mode == 0u && prev != 0u)
        dub_arp_toast_clear();
}

/* Button owns the mode: a live DJ ARP overlay must not restore the old mode,
 * and any other open gesture restores this mode instead of the pre-press one. */
static void dub_arp_user_set(uint8_t mode)
{
    if (g.dj_gest_ov == FM_DJ_GEST_OV_ARP && g.dj_gest_kind == GEST_NONE && g.dj_gest_flam_steps == 0u) {
        g.dj_gest_ov = FM_DJ_GEST_OV_NONE;
        g.dj_gest_bars = 0u;
    }
    if (dub_dj_gest_active())
        g.dj_gest_saved_arp = mode;
    dub_arp_set_mode(mode);
}

static void perf_toast_lanes_sync(void)
{
    const char *msg;
    uint8_t kind;
    int8_t voice_ln;
    uint32_t end;
    perf_toast_lane_t *proc = &perf_toast_lanes[PERF_TOAST_LANE_PROC];
    perf_toast_lane_t *main = &perf_toast_lanes[PERF_TOAST_LANE_MAIN];
    perf_toast_lane_t *eph = &perf_toast_lanes[PERF_TOAST_LANE_EPH];

    perf_toast_policy_lane_prune(perf_toast_lanes, fm1_ms);
    perf_toast_sync_dj_strips();
    if (g.cfg.p.arp_mode == 0u) {
        arp_toast_anim.active = 0u;
        arp_note_anim.active = 0u;
        if (main->on && perf_toast_is_arp_label(main->msg))
            perf_toast_policy_lane_clear(main);
    }
    if (!g.dj.on) {
        dj_param_anim.active = 0u;
        if (main->on && perf_toast_is_dj_msg(main->msg))
            perf_toast_policy_lane_clear(main);
    }
    if (!dub_home_morph_flash_visible() && proc->on && perf_toast_is_process_msg(proc->msg) && proc->end_ms == 0u)
        proc->end_ms = fm1_ms + CV_TOAST_FADE_MS;
    if (perf_toast_pick_process(&msg, &kind, &end))
        perf_toast_policy_lane_set_proc(proc, msg, kind, end, fm1_ms);

    if (perf_toast_pick_sticky(&msg, &kind, &end, &voice_ln)) {
        if (!perf_toast_is_arp_label(msg))
            arp_toast_anim.active = 0u;
        if (g.dj.on && perf_toast_is_dj_msg(msg)) {
            if (!dj_toast_main_live())
                perf_toast_policy_lane_clear(main);
            else
                perf_toast_policy_lane_set_main(main, "DJ", kind, voice_ln, end, fm1_ms, 0, 0u);
        } else {
            const char *side = "";
            uint32_t side_end = 0u;

            if (g.cfg.p.arp_mode != 0u && perf_toast_is_arp_label(msg) && fm1_ms < g.key_toast_until &&
                g.key_toast[0]) {
                side = g.key_toast;
                side_end = g.key_toast_until;
            }
            if (main->on && perf_toast_is_arp_label(main->msg) && perf_toast_is_arp_label(msg) &&
                !perf_toast_same_msg(main->msg, msg)) {
                uint32_t oi = 0u;

                while (main->msg[oi] && oi + 1u < sizeof arp_toast_anim.out) {
                    arp_toast_anim.out[oi] = main->msg[oi];
                    oi++;
                }
                arp_toast_anim.out[oi] = 0;
                arp_toast_anim.start_ms = fm1_ms;
                arp_toast_anim.active = 1u;
            }
            perf_toast_policy_lane_set_main(main, msg, kind, voice_ln, end, fm1_ms, side, side_end);
        }
    } else {
        if (main->on && perf_toast_is_dj_msg(main->msg) &&
            !dj_toast_main_live())
            perf_toast_policy_lane_clear(main);
    }

    if (perf_toast_pick_overlay(&msg, &kind, &end, &voice_ln)) {
        perf_toast_lane_t *dest = main;

        if (perf_toast_lane_sticky_main_occupied())
            dest = eph;
        if (dest == eph) {
            if (eph->on && !perf_toast_same_msg(eph->msg, msg))
                perf_toast_eph_gesture_fade_out();
            perf_toast_policy_lane_set_eph(dest, msg, kind, voice_ln, end, fm1_ms);
        } else {
            if (main->on && !perf_toast_same_msg(main->msg, msg) && perf_toast_is_gesture_msg(main->msg))
                perf_toast_main_gesture_fade_out();
            perf_toast_policy_lane_set_main(main, msg, kind, voice_ln, end, fm1_ms, "", 0u);
        }
    } else {
        perf_toast_eph_gesture_fade_out();
        if (main->on && perf_toast_is_gesture_msg(main->msg) && !perf_toast_lane_sticky_main_occupied())
            perf_toast_main_gesture_fade_out();
    }
}

static int perf_toast_main_bw_slide_latch(uint32_t now)
{
    if (arp_toast_anim.active) {
        uint32_t el = now - arp_toast_anim.start_ms;

        if (el < ARP_TOAST_SLIDE_MS)
            return 1;
    }
    if (arp_note_anim.active) {
        uint32_t el = now - arp_note_anim.start_ms;

        if (el < ARP_TOAST_SLIDE_MS)
            return 1;
    }
    if (dj_param_anim.active) {
        uint32_t el = now - dj_param_anim.start_ms;

        if (el < ARP_TOAST_SLIDE_MS)
            return 1;
    }
    return 0;
}

/* DJ main is always "DJ"; side from dj_toast_param or ephemeral key toast. */
static const char *dj_toast_draw_side(void)
{
    static char loop_side[20];

    if (fm1_ms < g.key_toast_until && g.key_toast[0]) {
        if (g.dj.on && perf_toast_policy_key_toast_is_loop_len(g.key_toast_lane)) {
            perf_toast_policy_dj_loop_side(loop_side, sizeof loop_side, g.key_toast);
            return loop_side;
        }
        return g.key_toast;
    }
    return dj_toast_param();
}

static void draw_plate_era_toast(void)
{
    ko_toast_t t = {0};

    if (!((int32_t)(g.plate_era_toast_until - fm1_ms) > 0 && g.plate_era_toast[0]))
        return;
    t.msg = g.plate_era_toast;
    t.kind = 1u;
    t.end_ms = g.plate_era_toast_until;
    {
        int32_t bh = toast_lane_stack_bh((const toast_font_t *)&FONT_S);

        t.by = perf_toast_policy_lane_by(PERF_TOAST_LANE_EPH, ko_toast_by_center(bh), bh);
    }
    t.bar = g.plate_era_toast_bar;
    ko_draw_toast(&t, fm1_ms);
}

static void draw_perf_toast(void)
{
    int32_t bh, by_center, by_proc, by_main, by_eph;
    perf_toast_lane_t *proc = &perf_toast_lanes[PERF_TOAST_LANE_PROC];
    perf_toast_lane_t *main = &perf_toast_lanes[PERF_TOAST_LANE_MAIN];
    perf_toast_lane_t *eph = &perf_toast_lanes[PERF_TOAST_LANE_EPH];
    static uint32_t main_bw_key;
    static int32_t main_bw_floor;

    perf_toast_lanes_sync();
    bh = toast_lane_stack_bh((const toast_font_t *)&FONT_S);
    by_center = ko_toast_by_center(bh);
    by_eph = perf_toast_policy_lane_by(PERF_TOAST_LANE_EPH, by_center, bh);
    by_proc = perf_toast_policy_lane_by(PERF_TOAST_LANE_PROC, by_center, bh);
    by_main = perf_toast_policy_lane_by(PERF_TOAST_LANE_MAIN, by_center, bh);
    cv_toast_bw_floor_set(0);
    if (eph->on) {
        ko_toast_t t = {eph->msg, 0, eph->kind, eph->end_ms, by_eph, 0u, 0, 0u};

        t.bar = eph->voice_ln >= 0 ? dub_seq_lane_play_col((uint8_t)eph->voice_ln) : 0u;
        ko_draw_toast(&t, fm1_ms);
    }
    if (proc->on) {
        ko_toast_t t = {proc->msg, 0, proc->kind, proc->end_ms, by_proc, 0u, 0, 0u};

        ko_draw_toast(&t, fm1_ms);
    }
    if (!main->on) {
        main_bw_key = 0u;
        main_bw_floor = 0;
        cv_toast_bw_floor_set(0);
        return;
    }
    if (main->on && perf_toast_is_dj_msg(main->msg) &&
        !dj_toast_main_live()) {
        main_bw_key = 0u;
        main_bw_floor = 0;
        cv_toast_bw_floor_set(0);
        return;
    }
    if (main->start_ms != main_bw_key) {
        main_bw_key = main->start_ms;
        main_bw_floor = 0;
    }
    {
        /* Latch floor only during horizontal side/main slide — else exact ink layout (no sticky padding). */
        const toast_font_t *tf = (const toast_font_t *)&FONT_S;
        const char *side_for_bw = 0;
        int32_t need;
        static char main_bw_side_tag[24];
        static char main_bw_side_prev[24];

        if (g.dj.on && perf_toast_is_dj_msg(main->msg)) {
            const char *side = dj_toast_draw_side();

            if (side && side[0])
                side_for_bw = side;
        } else if (main->side[0])
            side_for_bw = main->side;
        main_bw_side_tag[0] = 0;
        if (side_for_bw && side_for_bw[0])
            perf_toast_policy_lane_copy_msg(main_bw_side_tag, sizeof main_bw_side_tag, side_for_bw);
        if (!perf_toast_policy_same_msg(main_bw_side_tag, main_bw_side_prev)) {
            perf_toast_policy_lane_copy_msg(main_bw_side_prev, sizeof main_bw_side_prev, main_bw_side_tag);
            main_bw_floor = 0;
        }
        need = perf_toast_policy_main_badge_bw(tf, main->msg, side_for_bw, 0);
        if (!side_for_bw || !side_for_bw[0])
            main_bw_floor = need;
        else if (perf_toast_main_bw_slide_latch(fm1_ms)) {
            if (need > main_bw_floor)
                main_bw_floor = need;
        } else
            main_bw_floor = need;
        cv_toast_bw_floor_set(main_bw_floor);
    }
    if (arp_toast_anim.active && perf_toast_is_arp_label(main->msg)) {
        uint32_t el = fm1_ms - arp_toast_anim.start_ms;
        ko_toast_t out = {arp_toast_anim.out, 0, CV_TOAST_OK, 0u, by_main, 0u, 0, 0u};
        ko_toast_t in = {main->msg, 0, main->kind, main->end_ms, by_main, 0u, 0, 0u};

        if (!ko_toast_slide(el, ARP_TOAST_SLIDE_MS, &in.slide, &out.dith))
            arp_toast_anim.active = 0u;
        else {
            out.slide = -(int32_t)(el * 32u / ARP_TOAST_SLIDE_MS);
            in.bar = main->voice_ln >= 0 ? dub_seq_lane_play_col((uint8_t)main->voice_ln) : 0u;
            ko_draw_toast(&out, fm1_ms);
            ko_draw_toast(&in, fm1_ms);
            return;
        }
    }
    {
        uint16_t bar = main->voice_ln >= 0 ? dub_seq_lane_play_col((uint8_t)main->voice_ln) : 0u;

        if (g.cfg.p.arp_mode != 0u && perf_toast_is_arp_label(main->msg) &&
            fm1_ms < g.key_toast_until && g.key_toast[0]) {
            ko_toast_t t = {main->msg, g.key_toast, main->kind, g.key_toast_until, by_main, 0u, 0, 0u};

            if (arp_note_anim.active &&
                !ko_toast_slide(fm1_ms - arp_note_anim.start_ms, ARP_TOAST_SLIDE_MS, &t.slide, &t.dith))
                arp_note_anim.active = 0u;
            t.bar = (g.key_toast_lane < 6u) ? dub_seq_lane_play_col(g.key_toast_lane) : 0u;
            ko_draw_toast(&t, fm1_ms);
            return;
        }
        if (perf_toast_is_dj_msg(main->msg)) {
            ko_toast_t t = {"DJ", 0, main->kind, 0u, by_main, 0u, 0, 0u};
            static char dj_param_last[24];

            if (fm1_ms < g.key_toast_until && g.key_toast[0]) {
                t.side = dj_toast_draw_side();
                t.end_ms = g.key_toast_until;
                bar = (g.key_toast_lane < 6u) ? dub_seq_lane_play_col(g.key_toast_lane) : 0u;
                if (arp_note_anim.active &&
                    !ko_toast_slide(fm1_ms - arp_note_anim.start_ms, ARP_TOAST_SLIDE_MS, &t.slide, &t.dith))
                    arp_note_anim.active = 0u;
            } else {
                const char *side = dj_toast_draw_side();

                t.side = side;
                if (side && side[0]) {
                    const char *key = dj_toast_side_anim_key(side);
                    int changed = !perf_toast_policy_same_msg(key, dj_param_last);

                    if (changed) {
                        uint32_t pi = 0u;

                        while (key[pi] && pi + 1u < sizeof dj_param_last) {
                            dj_param_last[pi] = key[pi];
                            pi++;
                        }
                        dj_param_last[pi] = 0;
                        dj_param_anim.start_ms = fm1_ms;
                        dj_param_anim.active = 1u;
                    }
                    if (dj_param_anim.active &&
                        !ko_toast_slide(fm1_ms - dj_param_anim.start_ms, ARP_TOAST_SLIDE_MS, &t.slide, &t.dith))
                        dj_param_anim.active = 0u;
                } else {
                    dj_param_last[0] = 0;
                    dj_param_anim.active = 0u;
                }
            }
            if (t.side && t.side[0]) {
                t.bar = bar;
                ko_draw_toast(&t, fm1_ms);
                return;
            }
        }
        if (main->side[0] && (!perf_toast_is_dj_msg(main->msg))) {
            ko_toast_t t = {main->msg, main->side, main->kind, main->side_end_ms, by_main, bar, 0, 0u};

            if (g.key_toast_lane < 6u)
                t.bar = dub_seq_lane_play_col(g.key_toast_lane);
            ko_draw_toast(&t, fm1_ms);
            return;
        }
        if (perf_toast_is_dj_msg(main->msg))
            cv_toast_bw_floor_set(0);
        {
            ko_toast_t t = {main->msg, 0, main->kind, main->end_ms, by_main, bar, 0, 0u};

            ko_draw_toast(&t, fm1_ms);
        }
    }
    cv_toast_bw_floor_set(0);
}

static int32_t top_bar_seq_dots_w(void)
{
    return (int32_t)FM_SEQ_MAX_PAGES * 8 - 3;
}

static int32_t top_bar_seq_dots_x0(void)
{
    return ((int32_t)KO_TOP_BAR_W - top_bar_seq_dots_w()) / 2;
}

static int32_t top_bar_note_max_w(void)
{
    int32_t m = 0;
    uint32_t i;

    for (i = 0u; i < 12u; i++) {
        int32_t w = text_w(&FONT_S, NOTE_NAMES[i]);

        if (w > m)
            m = w;
    }
    return m;
}

static int32_t top_bar_scale_hdr_max_w(void)
{
    int32_t m = 0;
    uint32_t i;

    for (i = 0u; i < 9u; i++) {
        int32_t w = text_w(&FONT_S, SCALE_HDR[i]);

        if (w > m)
            m = w;
    }
    return m;
}

static int32_t top_bar_bpm_max_w(void)
{
    char bpm_s[8];

    u8_dec(bpm_s, 999u);
    return text_w(&FONT_S, bpm_s);
}

static void dub_rail_pos(uint32_t *vis_out, uint32_t *ix_out, uint32_t *spb_out)
{
    uint32_t vis = (uint32_t)DUB_STEPS;
    /* rail_step = metrical index of the step that fired last */
    uint32_t ix = g.playing ? (uint32_t)g.rail_step : (uint32_t)g.bar_step;
    uint32_t spb;

    if (g.loop_active && g.loop_len > 0u) {
        uint32_t len = (uint32_t)g.loop_len;

        vis = len;
        if (!g.playing)
            ix = seq_loop_rel(g.step);
        if (ix >= vis)
            ix = vis - 1u;
        /* 1/1+ stretch: 8 slots, spb = len/8 (slower than idle 16×2) */
        if (len >= 32u)
            spb = len / 8u;
        else
            spb = 1u;
    } else {
        spb = vis / (uint32_t)DUB_PROG_BARS;
        if (spb < 1u)
            spb = 1u;
    }
    *vis_out = vis;
    *ix_out = ix;
    *spb_out = spb;
}

/* Quarter cell 0..3 of the playhead inside its rail bar. Phase is measured against the swung
 * length the audio loop is counting down (g.step = next step), so swing moves when the cell
 * jumps, never where it sits. */
#define DUB_KNOB_RAIL_COLOR_SCALE color_scale_rgb
#include "dub_knob_progress_rail.h"

static void dub_knob_rail_ctx_from_g(dub_knob_rail_ctx_t *c)
{
    dub_rail_pos(&c->vis_steps, &c->cur_ix, &c->steps_per_bar);
    c->playing = g.playing ? 1u : 0u;
    c->ext_clock = g.ext_clock ? 1u : 0u;
    c->era = ERA_COL[g.cfg.p.era % GENRE_ERA_N];
    c->sample_acc = g.sample_acc;
    if (g.playing && !g.ext_clock && g.samples_per_step > 0u)
        c->samples_need = dub_step_need_samples(g.step, g.samples_per_step, dub_rail_swing_push(g.samples_per_step));
    else
        c->samples_need = 0u;
}

static uint8_t dub_rail_cell(void)
{
    dub_knob_rail_ctx_t c;
    uint32_t bar, n_bars;

    dub_knob_rail_ctx_from_g(&c);
    n_bars = (c.vis_steps + c.steps_per_bar - 1u) / c.steps_per_bar;
    if (n_bars > (uint32_t)DUB_PROG_BARS)
        n_bars = (uint32_t)DUB_PROG_BARS;
    bar = c.cur_ix / c.steps_per_bar;
    if (n_bars > 0u && bar >= n_bars)
        bar = n_bars - 1u;
    return (uint8_t)((bar << 2) | dub_knob_rail_quarter(&c));
}

/* Global beat/bar rail aligned to knob slots; 16 bars (KO BASS knob bar + DECK HOME footer). */
static void draw_bar_progress_rail(int32_t y_bot)
{
    dub_knob_rail_ctx_t c;

    dub_knob_rail_ctx_from_g(&c);
    g.rail_cell = dub_draw_bar_progress_rail(y_bot, &c);
}

static void top_bar_page_cycle_dots(const ko_top_bar_t *t)
{
#if defined(GENRE_BASS)
    int32_t dx = 238 - 13;
    uint8_t ix = g.seq_plock_latch ? g.seq_plock_bank : (uint8_t)(g.seq_page == 2u);

    cv_rect(dx, 4, 5, 5, ix == 0u ? C_WHITE : C_DIM);
    cv_rect(dx + 8, 4, 5, 5, ix == 1u ? C_WHITE : C_DIM);
#else
    fm_seq_draw_knob_page_cycle_dots(238 - ((int32_t)FM_SEQ_KNOB_PAGE_CYCLE_N - 1) * 8 - 5, 4, g.seq_page);
#endif
    (void)t;
}

#if !defined(GENRE_BASS)
static void top_bar_seq_step_dots(const ko_top_bar_t *t)
{
    fm_seq_draw_step_dots_at(t->seq_x0, 14 + 4, (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES),
                             (uint8_t)(g.step / FM_SEQ_PAGE_STEPS), g.seq_view_on, g.playing,
                             ERA_COL[g.cfg.p.era % GENRE_ERA_N]);
}

static void top_bar_kit_name(const ko_top_bar_t *t)
{
    uint8_t kix = g.voice.p.drum_kit % DKIT_N;

    cv_text(44, 14, &FONT_S, DKIT_NAMES[kix], ERA_COL[DKIT_ERA[kix] % GENRE_ERA_N]);
    (void)t;
}
#endif

static void draw_top_bar(void)
{
    ko_top_bar_t t = {0};
    ko_wave_ctx_t w;
    const dub_params_t *hp = &g.voice.p;
    uint8_t pages = bank_pages(g.bank);
    uint16_t play_col = ERA_COL[g.cfg.p.era % GENRE_ERA_N];

    u8_dec(t.bpm, hp->bpm ? hp->bpm : g.cfg.p.bpm);
    t.key = NOTE_NAMES[hp->root % 12u];
    t.scale = SCALE_HDR[scale_ix(hp->scale)];
    if (g.cue_on)
        u8_dec(t.swing, dub_groove_show_swing());
    dub_home_morph_flash_end_check();
    if (dub_home_morph_flash_visible())
        t.overlay = dub_home_morph_flash_msg(dub_home_morph_flash_pick_bit(dub_home_morph_flash_live_mask()));
    t.playing = g.playing ? 1u : 0u;
    /* Downbeat blink on beat 1 (step 0) and subtle pulse on quarters (steps 4, 8, 12) like KO RYTM */
    t.on_one = g.playing && (g.bar_step == 0u) && (g.sample_acc * 2u < g.samples_per_step);
    t.on_beat = g.playing && ((g.step & 3u) == 0u) && (g.sample_acc * 3u < g.samples_per_step);

    /* Left: PLATE or song name, then ERA or ARTIST (wave color = sound era / plate) */
    t.show_title = 1u;
    t.cursor = -1;
    if (g.song_setup_latch) {
        uint32_t ni;

        for (ni = 0; ni < DUB_SONG_NAME_LEN; ni++)
            t.title[ni] = g.song_setup_name[ni];
        t.title[DUB_SONG_NAME_LEN] = 0;
        t.cursor = (int8_t)g.song_setup_name_cur;
    } else {
        uint8_t pix = g.cfg.p.plate % GENRE_PRESET_N;

        copy_str(t.title, genre_preset_name(g.cfg.p.plate));
        if (!fm_era_is_artist(g.cfg.p.era) && pix < GENRE_CFG_USER_PLATE_N &&
            (g.user_plate_mask & (1ull << pix))) {
            char *ep = t.title;
            while (*ep)
                ep++;
            *ep++ = '*';
            *ep = 0;
        }
    }
    wave_ctx_fill(&w);
    t.wave = &w;
    copy_str(t.sub, fm_era_knob_name(g.cfg.p.era));
    ko_top_bar_era_plate_cols(&t, fm_era_knob_col, genre_era_of_preset(g.cfg.p.plate), g.cfg.p.era);

    t.rec = g.rec_arm ? 1u : 0u;
    t.pages = pages;
    t.page = bank_page() % pages;
    if (g.view == VIEW_SEQ)
        t.dots = top_bar_page_cycle_dots;

    t.stage = g.view == VIEW_SEQ || (g.view != VIEW_HOME && g.bank != BANK_HOME) ? stage_name() : GENRE_APP_NAME;
    t.seq = g.view == VIEW_SEQ;
    t.seq_x0 = top_bar_seq_dots_x0();
    t.seq_w = top_bar_seq_dots_w();
    if (t.seq) {
#if !defined(GENRE_BASS)
        t.seq_dots = top_bar_seq_step_dots;
#endif
        if (g.seq_page != 5u) {
#if defined(GENRE_BASS)
            u8_dec(t.seq_txt[0], (uint32_t)(g.step / FM_SEQ_PAGE_STEPS + 1u));
            t.seq_col[0] = C_WHITE;
            if (g.playing) {
                u8_dec(t.seq_txt[1], (uint32_t)(g.step % FM_SEQ_PAGE_STEPS + 1u));
                t.seq_col[1] = play_col;
            }
#else
            u8_dec(t.seq_txt[0], (uint32_t)((g.seq_view_page % FM_SEQ_MAX_PAGES) + 1u));
            t.seq_col[0] = g.seq_view_on[g.seq_view_page % FM_SEQ_MAX_PAGES] ? C_WHITE : C_DIM;
            if (g.playing) {
                u8_dec(t.seq_txt[1], (uint32_t)g.step + 1u);
                t.seq_col[1] = play_col;
            } else {
                copy_str(t.seq_txt[1], "-");
                t.seq_col[1] = C_DIM;
            }
#endif
        }
    }

    t.mid_busy = g.voice.freeze || g.console || g.cfg.p.arp_mode != 0u;
#if !defined(GENRE_BASS)
    {
        uint8_t ag_chk = g.gesture_latch ? g.gesture_latch : g.gesture;

        if (ag_chk == GEST_FILL || ag_chk == GEST_BREAK || ag_chk == GEST_SOLO_DRUM ||
            ag_chk == GEST_SOLO_BASS || ag_chk == GEST_SOLO_BAND || ag_chk == GEST_FLAM)
            t.mid_busy = 1u;
    }
    if (g.view == VIEW_DESK && g.bank == BANK_DRUM && bank_page() != 0u && g.sel_drm != 3u)
        t.mid = top_bar_kit_name;
#endif
    t.flash = &g.seq_knob_flash;
    t.now = fm1_ms;
    ko_draw_top_bar(&t);
}

static uint8_t knob_seq_bar_px(uint8_t slot)
{
#if defined(GENRE_BASS)
    if (g.view == VIEW_SEQ || g.seq_plock_hold)
        return bass_knob_seq_bar_px(slot);
#endif
    uint8_t ln = g.seq_lane % DUB_SEQ_LANES;
    uint32_t v = 0u;
    uint32_t mx = 127u;

    if (g.seq_plock_hold) {
        uint8_t st = (uint8_t)(g.seq_plock_step % DUB_SEQ_STEPS);
        dub_step_t *ovr = &g.cfg.seq[st];
        uint8_t base_accent = ovr->accent ? ovr->accent : 90u;

        if (slot == 0) {
#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_HOUSE) || defined(FM_DUB_SRN_PLOCK)
            if (ln == 3u) {
                v = g.step_vel[ln][st];
                mx = 127u;
            } else
#endif
            {
                v = g.step_vel[ln][st] ? g.step_vel[ln][st] : base_accent;
                mx = 127u;
            }
        } else if (slot == 1) {
            return fm_seq_cond_bar_px(g.step_prob[ln][st]);
        } else if (slot == 2) {
#ifdef FM_L3_GATE
            if (ln == 3u) {
                v = FM_L3_LEN_IX(g.step_micro[ln][st]);
                mx = FM_L3_LEN_N - 1u;
            } else
#endif
            {
                v = (uint32_t)((int32_t)dub_eff_step_micro(ln, st) + FM_SEQ_MICRO_HALF);
                mx = (uint32_t)FM_SEQ_TIMING_UNITS;
            }
        } else {
            v = (uint32_t)((int32_t)g.step_tune[ln][st] + 24);
            mx = 48u;
        }
    } else if (g.seq_page == 4u) {
        uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);

        if (slot == 0) {
            v = dj_seq_eff_cyc(vp);
            mx = (uint32_t)(FM_SEQ_NVIEW_CYC - 1u);
        } else if (slot == 1) {
            v = (uint32_t)(dj_seq_eff_reps(vp) - 1u);
            mx = 15u;
        } else if (slot == 2) {
            v = fm_dj_seq_ens_bar_overlay(&g.dj, &g.dj_ens_ov, DUB_ENS_MUTE, dub_ens_knob_bar());
            mx = 127u;
        } else {
            int32_t fd = fm_dj_delta_for_tgt(&g.dj, FM_DJ_TGT_FLIP);

            if (fd > 16)
                v = 127u;
            else if (fd < -16)
                v = 0u;
            else
                v = g.seq_flip_state ? 127u : 0u;
            mx = 127u;
        }
    } else if (g.seq_page == 5u) {
        const dub_params_t *mp = &g.cfg.p;
        uint8_t show_sw = dub_groove_show_swing();
        uint8_t show_bpm = dub_groove_show_bpm();
        if (slot == 0)
            v = (uint32_t)((mp->root % 12u) * 127u / 11u);
        else if (slot == 1)
            v = (uint32_t)(dub_groove_show_scale() * 127u / 8u);
        else if (slot == 2)
            v = (uint32_t)(show_sw < 50u ? 0u : (show_sw - 50u) * 127u / 25u);
        else
            v = (uint32_t)((show_bpm < 40u ? 0u : (show_bpm - 40u) * 127u / 140u));
        mx = 127u;
    } else if (g.seq_page == 2u || g.seq_page == 3u) {
        uint8_t vp = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);
        uint8_t sl = seq_lane_sel();

        if (slot == 0) {
            v = (uint32_t)(seq_lane_len_v(vp, sl) - 1u);
            mx = (uint32_t)(FM_SEQ_STEPS - 1u);
        } else if (slot == 1) {
            uint8_t pos = dub_lane_p3_state_pos(ln);
            v = pos == 0u ? 0u : pos == 1u ? 64u : 127u;
            mx = 127u;
        } else if (slot == 2) {
            v = (uint32_t)((int32_t)seq_lane_nudge_base(vp, seq_lane_sel()) + FM_SEQ_TIMING_UNITS);
            mx = (uint32_t)(FM_SEQ_TIMING_UNITS * 2);
        } else if (slot == 3 && g.seq_page == 2u) {
            v = dub_lane_pace_eff((seq_lane_is_all() ? g.seq_lane : seq_lane_sel()) % DUB_SEQ_LANES);
            mx = FM_SEQ_PACE_N - 1u;
        } else {
            v = 0u;
            mx = 0u;
        }
    } else if (g.seq_page == 1u) {
        uint8_t fn = fm_seq_p1_fn((uint8_t)slot);

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
    } else {
        if (slot == 0) {
            uint8_t sl = seq_lane_sel();

            v = seq_lane_is_all() ? DUB_SEQ_LANE_ALL : sl;
            mx = DUB_SEQ_LANE_ALL;
        } else if (slot == 1) {
            v = fm_seq_euclid_bar_hits(dub_eff_euclid_hits(ln));
            mx = 16u;
        } else if (slot == 2) {
            v = (uint32_t)dub_eff_euclid_rot(ln);
            mx = 15u;
        } else {
            v = dub_eff_vary(ln);
            mx = 127u;
        }
    }
    if (v > mx)
        v = mx;
    return mx ? (uint8_t)((v * 127u) / mx) : 0u;
}

static uint8_t knob_bar_val(const dub_params_t *mp, uint8_t col)
{
    uint8_t pg = bank_page();

    if (g.seq_plock_hold)
        return knob_seq_bar_px(col);
    if (g.view == VIEW_SEQ && (fm1_in.buttons & (1u << BTN_LFO))) {
        uint8_t lfo_lt = 255u;

        /* LFO held on VEL/GATE/PTCH lanes: pot shows the stored unsigned depth (0..127). */
        if (dub_seq_knob_lfo_map(col, &lfo_lt) &&
            (lfo_lt == DUB_LFO_SEQ_VEL || lfo_lt == DUB_LFO_SEQ_GATE || lfo_lt == DUB_LFO_SEQ_PITCH))
            return g.lfo_slots[lfo_lt].depth;
    }
    if (g.view == VIEW_SEQ)
        return knob_seq_bar_px(col);
#if defined(GENRE_BASS)
    if (g.bank == BANK_GROOVE)
        return bass_groove_knob_bar(mp, pg, col);
#endif
    if (g.bank == BANK_HOME) {
#if defined(GENRE_BASS)
        if (pg == 0u)
            return col == 0 ? mp->cutoff_b : col == 1 ? mp->reso_mode : col == 2 ? mp->drive : mp->dly_b;
        if (pg == 1u) {
            if (col == 0u)
                return bass_glide_eff();
            {
                return bass_mac_eff_raw((uint8_t)(col - 1u));
            }
        }
        if ((fm1_in.buttons & (1u << BTN_EDIT)) != 0u) {
            uint8_t sel = g.bass_rt.home_macro_sel;
            const bass_mod_slot_t *sl = bass_mod_route_slot(&g.cfg.bass.mods, sel);

            if (col == 0u)
                return (uint8_t)(((sel / BASS_MOD_SLOTS_PER_MAP) % BASS_MOD_MACRO_N) * 127u / (BASS_MOD_MACRO_N - 1u));
            if (col == 1u)
                return (uint8_t)((sel % BASS_MOD_SLOTS_PER_MAP) * 127u / (BASS_MOD_SLOTS_PER_MAP - 1u));
            if (col == 2u)
                return (uint8_t)((sl->dest * 127u) / (BASS_MOD_N - 1u));
            return (uint8_t)(((int32_t)sl->amount + 127) * 127 / 254);
        }
        {
            return bass_perf_eff_raw(col);
        }
#else
        if (pg == 2u) {
            if (col == 0)
                return dub_eff_macro(8);
            if (col == 1)
                return dub_eff_macro(9);
            if (col == 2)
                return dub_ens_knob_bar();
            return g.seq_flip_state ? 127u : 0u;
        }
        return dub_eff_macro(pg == 0u ? col : (uint8_t)(4u + col));
#endif
    }
    if (g.bank == BANK_CHORD) {
#if defined(GENRE_BASS)
        const bass_dahdsr_cfg_t *env = &g.synth.bass_env_cfg; /* mod + LFO effective, like g.voice.p */
        bass_mod_slot_t *esl;
        uint8_t esi;

        if (pg == 0u)
            return col == 0 ? env->t.attack : col == 1 ? env->t.decay :
                   col == 2 ? env->t.sustain : env->t.release;
        if (pg == 1u)
            return col == 0 ? env->t.delay : col == 1 ? env->t.hold :
                   col == 2 ? g.synth.bass_env_curve : env->pol.retrigger * 127u;
        esi = g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP;
        esl = &g.cfg.bass.mods.env[esi];
        if (pg >= FM_ENV_KIT_PAGES)
            return col == 0u ? esl->curve : 0u;
        if (col == 0u)
            return env->pol.invert * 127u;
        if (col == 1u)
            return env->pol.accent_scale;
        if (col == 2u)
            return (uint8_t)((esl->dest * 127u) / (BASS_MOD_N - 1u));
        return (uint8_t)(((int32_t)esl->amount + 127) * 127 / 254);
#else
        if (pg == 2u && col == 0 && !dub_lane_audible(5u))
            return 0u;
        return pg == 0u ? (col == 0 ? ((mp->voicing % VOICE_N) * 127u) / (VOICE_N - 1u) : col == 1 ? mp->cutoff_c :
                           col == 2 ? mp->atk_c : mp->decay_c)
             : pg == 1u ? (col == 0 ? mp->timbre :
                           col == 1 ? mp->reso_mode : col == 2 ? mp->env_filt : mp->glide_c)
             : (col == 0 ? mp->lvl_c : col == 1 ? (mp->pan[1] == 0u ? 64u : mp->pan[1]) :
                col == 2 ? mp->dly_c : mp->rev_c);
#endif
    }
    if (g.bank == BANK_BASS) {
#if defined(GENRE_BASS)
        if (pg == 2u && col == 0)
            return mp->reso_mode;
        if (pg == 2u && col == 1)
            return mp->cutoff_b;
        if (pg == 2u && col == 2)
            return mp->env_filt;
        if (pg == 2u && col == 3)
            return mp->wobble;
        if (pg == 1u && col == 0 && !dub_lane_audible(4u))
            return 0u;
        return pg == 0u ? (col == 0 ? (mp->bass_model * 127u) / (BASS_N - 1u) :
                           col == 1 ? g.cfg.bass.glide_time : col == 2 ? mp->sub_oct :
                           (mp->pan[0] == 0u ? 64u : mp->pan[0]))
             : pg == 1u ? (col == 0 ? mp->lvl_b : col == 1 ? mp->drive :
                           col == 2 ? mp->dly_b : mp->rev_b)
             : 0u;
#else
        if (pg == 2u && col == 0 && !dub_lane_audible(4u))
            return 0u;
        return pg == 0u ? (col == 0 ? (mp->bass_model * 127u) / (BASS_N - 1u) :
                           col == 1 ? mp->cutoff_b : col == 2 ? mp->atk_b : mp->decay_b)
             : pg == 1u ? (col == 0 ? mp->sub_oct : col == 1 ? mp->pump :
                           col == 2 ? mp->drive : mp->wobble)
             : (col == 0 ? mp->lvl_b : col == 1 ? (mp->pan[0] == 0u ? 64u : mp->pan[0]) :
                col == 2 ? mp->dly_b : mp->rev_b);
#endif
    }
    if (g.bank == BANK_DRUM) {
        uint8_t dp = mp->drm_pan[sel_drm_ix()];
        uint8_t di = sel_drm_ix();

        if (pg == 0u && col == 2 && !dub_lane_audible(g.sel_drm % 4u))
            return 0u;
        return pg == 0u ? (col == 0 ? (uint8_t)((mp->drum_kit % DKIT_N) * 127u / (DKIT_N - 1u)) :
                           col == 1 ? 0u :
                           col == 2 ? *drm_lvl_cptr(mp, di) :
                           (mp->drm_tune[di] ? mp->drm_tune[di] : 64u))
             : pg == 1u ? (col == 0 ? mp->drm_dens[di] :
                           col == 1 ? (mp->drm_cut[di] ? mp->drm_cut[di] : 64u) :
                           col == 2 ? mp->drm_atk[di] :
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
                           (di == 3u ? (mp->drm_dec[di] ? mp->drm_dec[di] : 64u) :
                            dub_drm_display_decay(mp, di, g.voice.lane_gate[2])))
#else
                           dub_drm_display_decay(mp, di, g.voice.lane_gate[2]))
#endif
             : (col == 0 ? (sel_drm_is_siren() ? mp->siren_rate : mp->drm_drv[di]) :
                col == 1 ? (dp == 0u ? 64u : dp) :
                col == 2 ? mp->drm_dly[di] : mp->drm_rev[di]);
    }
    if (g.bank == BANK_FX) {
        return pg == 0u ? (col == 0 ? mp->dly_lvl : col == 1 ? mp->dly_time :
                           col == 2 ? mp->feedback : mp->loop_filt)
             : pg == 1u ? (col == 0 ? mp->wow : col == 1 ? (mp->tap * 127u) / (TAP_N - 1u) :
                           col == 2 ? mp->tank : mp->isolator)
             : (col == 0 ? mp->rev_mix : col == 1 ? mp->rev_damp :
                col == 2 ? mp->rev_tail : mp->dly_rev);
    }
    if (g.bank == BANK_LFO) {
        uint8_t t = g.lfo_target % DUB_LFO_TGT_N;
        const fm_lfo_slot_t *sl = dub_lfo_slot_view(t);

        if (pg == 0u)
            return col == 0 ? dub_lfo_tgt_order_knob_val(t) :
                   col == 1 ? sl->rate :
                   col == 2 ? sl->depth : sl->wave;
        if (pg == 1u)
            return col == 0 ? sl->phase :
                   col == 1 ? sl->skew :
                   col == 2 ? sl->smooth : sl->fold;
        return col == 0 ? (sl->algo * 127u) / (FM_LFO_NALGOS - 1u) :
               col == 1 ? sl->ratio :
               col == 2 ? sl->index : sl->fb;
    }
    return 64u;
}

/* Knob slots with few discrete values: returns a KNOB_POT_* mode, *steps = value count. The current step
 * comes from knob_bar_val() (step * 127 / (steps - 1)). More than the mode's cap (KNOB_POT_ORD_MAX_STEPS for
 * ORD, KNOB_POT_MAX_STEPS otherwise) stays continuous. */
static uint8_t knob_slot_discrete(uint8_t slot, uint8_t *steps)
{
    uint8_t pg = bank_page();
    uint8_t mode = KNOB_POT_CONT;

    *steps = 0u;
#if defined(GENRE_BASS)
    if (g.seq_plock_hold && g.seq_plock_bank) {
        if (slot == 1u || slot == 2u)
            mode = KNOB_POT_TICK, *steps = (uint8_t)(BASS_STEP_LV_FULL + 1u);
        else if (slot == 3u)
            mode = KNOB_POT_ORD, *steps = (uint8_t)(BASS_RATCHET_MAX - BASS_RATCHET_MIN + 1u);
        return mode;
    }
#endif
    if (g.seq_plock_hold)
        return KNOB_POT_CONT;
    if (g.view == VIEW_SEQ) {
#if defined(GENRE_BASS)
        if (g.seq_page == 0u) {
            if (slot == 1u || slot == 2u)
                mode = KNOB_POT_TICK, *steps = (uint8_t)(BASS_STEP_LV_FULL + 1u);
            else if (slot == 3u)
                mode = KNOB_POT_ORD, *steps = (uint8_t)(BASS_RATCHET_MAX - BASS_RATCHET_MIN + 1u);
        } else if (g.seq_page == 2u && slot == 1u)
            mode = KNOB_POT_SEL, *steps = 3u;
#else
        if (g.seq_page == 0u && slot == 0u)
            mode = KNOB_POT_SEL, *steps = (uint8_t)(DUB_SEQ_LANE_ALL + 1u);
        else if ((g.seq_page == 2u || g.seq_page == 3u) && slot == 1u)
            mode = KNOB_POT_SEL, *steps = 3u;
        else if (g.seq_page == 2u && slot == 3u)
            mode = KNOB_POT_SEL, *steps = FM_SEQ_PACE_N;
        else if (g.seq_page == 4u && slot == 2u)
            mode = KNOB_POT_MASK, *steps = KNOB_POT_MASK_N;
        else if (g.seq_page == 4u && slot == 3u)
            mode = KNOB_POT_HALF, *steps = 2u;
#endif
    } else if (g.bank == BANK_FX) {
        if (pg == 1u && slot == 1u)
            mode = KNOB_POT_ORD, *steps = TAP_N;
    } else if (g.bank == BANK_HOME) {
#if !defined(GENRE_BASS)
        if (pg == 2u && slot == 3u)
            mode = KNOB_POT_HALF, *steps = 2u;
        else if (pg == 2u && slot == 2u)
            mode = KNOB_POT_MASK, *steps = KNOB_POT_MASK_N;
#endif
    } else if (g.bank == BANK_CHORD) {
#if defined(GENRE_BASS)
        if ((pg == 1u && slot == 3u) || (pg == 2u && slot == 0u))
            mode = KNOB_POT_TOG, *steps = 2u;
#else
        if (pg == 0u && slot == 0u)
            mode = KNOB_POT_SELW, *steps = VOICE_N;
#endif
    } else if (g.bank == BANK_BASS) {
        if (pg == 0u && slot == 0u)
            mode = KNOB_POT_SELW, *steps = BASS_N;
    } else if (g.bank == BANK_DRUM) {
        if (pg == 0u && slot == 0u)
            mode = KNOB_POT_SEL, *steps = DKIT_N;
    } else if (g.bank == BANK_LFO) {
        if (pg == 0u && slot == 0u)
            mode = KNOB_POT_TICK, *steps = dub_lfo_dests.n; /* runtime target count (genre-dependent) */
        else if (pg == 0u && slot == 3u)
            mode = KNOB_POT_SHAPE, *steps = KNOB_POT_SHAPE_N;
        else if (pg == 2u && slot == 0u)
            mode = KNOB_POT_SELW, *steps = FM_LFO_NALGOS;
    }
#if defined(GENRE_BASS)
    else if (g.bank == BANK_GROOVE) {
        /* KEY: 12 ticks (TICK, same as KIT GLO MIDI CH). */
        if (pg == 0u && slot == 0u)
            mode = KNOB_POT_TICK, *steps = (uint8_t)(sizeof(NOTE_NAMES) / sizeof(NOTE_NAMES[0]));
        else if (pg == 0u && slot == 1u)
            mode = KNOB_POT_SEL, *steps = (uint8_t)SCALE_N;
        /* MIDI CH: 16 ticks (TICK, same as KIT GLO MIDI). */
        else if (pg == 1u && slot == 0u)
            mode = KNOB_POT_TICK, *steps = 16u;
        else if (pg == 1u && slot == 1u)
            mode = KNOB_POT_SEL, *steps = BASS_CLOCK_N;
    }
#endif
    if (mode != KNOB_POT_TOG && *steps > knob_pot_max_steps(mode, (uint8_t)DUB_LFO_TGT_N))
        return KNOB_POT_CONT;
    return *steps >= 2u ? mode : KNOB_POT_CONT;
}

static void knob_lbl_dry_wet(const ko_knob_slot_t *k, int32_t x)
{
    fm_dual_morph_label_draw(x, 2, "DRY", "WET", k->custom_arg, k->klab, C_DIM);
}

static int32_t knob_val_scale_cue(const ko_knob_slot_t *k, int32_t x)
{
    /* Live cue name bright (t=0); dest cfg scale dim — no duplicate dest/dest. */
    fm_dual_morph_label_draw(x, 18, SCALE_HDR[g.cue_scale % 9u], SCALE_HDR[g.cfg.p.scale % 9u], 0u, k->vcol,
                             C_DIM);
    return x;
}

static int32_t knob_val_lfo_shape(const ko_knob_slot_t *k, int32_t x)
{
    return fm_lfo_shape_draw(x, 18, dub_lfo_slot_view((uint8_t)k->custom_arg)->wave, k->vcol, C_DIM);
}

#if defined(GENRE_BASS)
/* BASS ENV page 4 CURV: 0..127 axis, anchors -EXP · LOG · LIN · -LOG · EXP; morph between anchors. */
static int32_t knob_val_mod_curve(const ko_knob_slot_t *k, int32_t x)
{
    static const char *const ANCH[5] = {"-EXP", "LOG", "LIN", "-LOG", "EXP"};
    uint8_t c = (uint8_t)(k->custom_arg > 127u ? 127u : k->custom_arg);
    uint8_t seg;
    uint32_t t;

    if (bass_mod_curve_anchor(c)) {
        const char *name = ANCH[c == 127u ? 4u : c / 32u];
        int32_t tw = text_w(&FONT_S, name);
        int32_t tx = x + 6 + (48 - tw) / 2;

        return cv_text(tx, 18, &FONT_S, name, k->vcol);
    }
    seg = (uint8_t)(c / 32u);
    t = ((uint32_t)(c & 31u) * 128u) / 31u;
    return fm_dual_morph_label_draw(x, 18, ANCH[seg], ANCH[seg + 1u], (t * 255u) / 128u, k->vcol, C_DIM);
}
#endif

#include "fm_env_kit.h"

/* Frame-constant for knob_slot_fill: taken once per draw_knob_bar before the columns. */
static uint8_t knob_bar_b;
static const dub_params_t *knob_bar_mp;

#if defined(GENRE_BASS)
/* Largest |delta| one BASS route can add to a knob: src = full swing, sign both ways. */
static int32_t bass_route_amp(const bass_mod_slot_t *sl, int32_t src)
{
    int32_t a = bass_mod_slot_delta(sl, src);
    int32_t b = bass_mod_slot_delta(sl, -src);

    if (a < 0)
        a = -a;
    if (b < 0)
        b = -b;
    return a > b ? a : b;
}

/* Span a BASS mod route adds to knob col. Macro rows: only if a macro is live (bass_knob_mod_routed),
 * src = ±64 macro swing. Env rows: src = live env level (0 = no span). */
static uint8_t bass_route_span(uint8_t col)
{
    const bass_mod_maps_t *maps = &g.cfg.bass.mods;
    uint8_t dest = bass_knob_mod_dest(col);
    uint8_t mi, si;
    int32_t best = 0, v;
    /* Env routes: src = live envelope level (same unit as the route); level 0 = no span. */
    int32_t env_lvl = bass_dahdsr_mod_q15(&g.synth.bass_env, &g.synth.bass_env_cfg);

    if (dest == BASS_MOD_NONE)
        return 0u;
    if (bass_knob_mod_routed(col)) {
        for (mi = 0u; mi < BASS_MOD_MACRO_N; mi++) {
            for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
                if (maps->macro[mi][si].dest != dest)
                    continue;
                v = bass_route_amp(&maps->macro[mi][si], 64 * 258);
                if (v > best)
                    best = v;
            }
        }
    }
    for (si = 0u; si < BASS_MOD_SLOTS_PER_MAP; si++) {
        if (env_lvl <= 0 || maps->env[si].dest != dest)
            continue;
        v = bass_route_amp(&maps->env[si], env_lvl);
        if (v > best)
            best = v;
    }
    return best > 127 ? 127u : (uint8_t)best;
}
#endif

#if !defined(GENRE_BASS)
/* FM macro feeds: target LFO <- macro LFO whose m_* value couples into it (dub_sync m_ch..m_db, dub_cpl_map).
 * Upper bound per pair = macro LFO depth. Span = max over pairs (never a sum). */
static const struct { uint8_t tgt; uint8_t mac; } FM_MACRO_FEED[] = {
    { DUB_LFO_CUT_C, DUB_LFO_M_CHORD },  { DUB_LFO_CUT_C, DUB_LFO_M_ISO },    { DUB_LFO_CUT_C, DUB_LFO_M_FLUX },
    { DUB_LFO_DEC_C, DUB_LFO_M_CHORD },
    { DUB_LFO_DLY_C, DUB_LFO_M_CHORD },  { DUB_LFO_DLY_C, DUB_LFO_M_SPACE },  { DUB_LFO_DLY_C, DUB_LFO_M_DUB },
    { DUB_LFO_TIMB_C, DUB_LFO_M_CHORD }, { DUB_LFO_TIMB_C, DUB_LFO_M_GRIT },
    { DUB_LFO_RESO_C, DUB_LFO_M_CHORD }, { DUB_LFO_RESO_C, DUB_LFO_M_GRIT },
    { DUB_LFO_ENV_C, DUB_LFO_M_CHORD },
    { DUB_LFO_CUT_B, DUB_LFO_M_WOB },    { DUB_LFO_CUT_B, DUB_LFO_M_ISO },    { DUB_LFO_CUT_B, DUB_LFO_M_FLUX },
    { DUB_LFO_DEC_B, DUB_LFO_M_WEIGHT },
    { DUB_LFO_WOB, DUB_LFO_M_WOB },      { DUB_LFO_WOB, DUB_LFO_M_FLUX },
    { DUB_LFO_SUB_B, DUB_LFO_M_WEIGHT },
    { DUB_LFO_DRV_B, DUB_LFO_M_WOB },    { DUB_LFO_DRV_B, DUB_LFO_M_GRIT },
    { DUB_LFO_PUMP_B, DUB_LFO_M_WEIGHT },
    { DUB_LFO_DLY_B, DUB_LFO_M_DUB },
    { DUB_LFO_DENS, DUB_LFO_M_DUB },
    { DUB_LFO_KICK, DUB_LFO_M_WEIGHT },
    { DUB_LFO_FB, DUB_LFO_M_FEED },      { DUB_LFO_FB, DUB_LFO_M_DUB },       { DUB_LFO_FB, DUB_LFO_M_SPACE },
    { DUB_LFO_FB, DUB_LFO_M_FLUX },
    { DUB_LFO_LOOP, DUB_LFO_M_DUB },     { DUB_LFO_LOOP, DUB_LFO_M_FLUX },
    { DUB_LFO_WOW, DUB_LFO_M_FEED },     { DUB_LFO_WOW, DUB_LFO_M_GRIT },     { DUB_LFO_WOW, DUB_LFO_M_FLUX },
    { DUB_LFO_TANK, DUB_LFO_M_SPACE },
    { DUB_LFO_ISO, DUB_LFO_M_ISO },
    { DUB_LFO_REV, DUB_LFO_M_FEED },     { DUB_LFO_REV, DUB_LFO_M_SPACE },    { DUB_LFO_REV, DUB_LFO_M_FLUX },
    { DUB_LFO_PAN_B, DUB_LFO_M_MOTION }, { DUB_LFO_PAN_C, DUB_LFO_M_MOTION }, { DUB_LFO_GLIDE_C, DUB_LFO_M_MOTION },
    { DUB_LFO_PAN_K, DUB_LFO_M_FLUX },   { DUB_LFO_PAN_R, DUB_LFO_M_FLUX },
    { DUB_LFO_PAN_H, DUB_LFO_M_FLUX },   { DUB_LFO_PAN_S, DUB_LFO_M_FLUX },
};

static uint8_t fm_macro_feed_span(uint8_t lt)
{
    uint32_t i;
    uint8_t best = 0u;

    for (i = 0u; i < sizeof(FM_MACRO_FEED) / sizeof(FM_MACRO_FEED[0]); i++) {
        uint8_t d;

        if (FM_MACRO_FEED[i].tgt != lt)
            continue;
        d = dub_lfo_slot_view(FM_MACRO_FEED[i].mac)->depth;
        if (d > best)
            best = d;
    }
    return best;
}
#endif

/* Readout span = largest single source: direct LFO depth, macro feeds (FM), BASS mod routes (BASS).
 * Discrete: base + mark. */
static void knob_slot_apply_lfo_range(uint8_t col, ko_knob_slot_t *k, uint8_t base_u8)
{
    uint8_t lt, lo, hi, span = 0u;
    const fm_lfo_slot_t *sl = NULL;
    uint32_t bpm;

    k->range = 0u;
    if (k->pan || k->custom_value || k->morph)
        return;
    if (k->mode != KNOB_POT_CONT)
        return;
    lt = lfo_target_for_knob(col);
    if (lt < DUB_LFO_TGT_N) {
        sl = dub_lfo_slot_view(lt);
        if (k->lfo)
            span = sl->depth;
#if !defined(GENRE_BASS)
        {
            uint8_t ms = fm_macro_feed_span(lt);

            if (ms > span)
                span = ms;
        }
#endif
    }
#if defined(GENRE_BASS)
    {
        uint8_t rs = bass_route_span(col);

        if (rs > span)
            span = rs;
    }
#endif
    if (!span)
        return;
    bpm = knob_bar_mp && knob_bar_mp->bpm ? knob_bar_mp->bpm : (g.cfg.p.bpm ? g.cfg.p.bpm : 120u);
    if (!fm_lfo_readout_update(col, base_u8, base_u8, span, sl ? sl->rate : 0u, bpm, &lo, &hi))
        return;
    k->range = 1u;
#if defined(GENRE_BASS)
    if (g.view != VIEW_SEQ && g.bank == BANK_GROOVE && bank_page() == 0u && col == 2u) {
        /* SWING: show 50..75 like the value text (raw map as bass_groove_knob_bar). */
        u8_dec(k->range_lo, 50u + ((uint32_t)lo * 25u + 63u) / 127u);
        u8_dec(k->range_hi, 50u + ((uint32_t)hi * 25u + 63u) / 127u);
        return;
    }
#endif
    if (k->bip) {
        bip50_show_plain(k->range_lo, lo);
        bip50_show_plain(k->range_hi, hi);
    } else {
        u8_show100_dec(k->range_lo, lo);
        u8_show100_dec(k->range_hi, hi);
    }
}

static void knob_slot_fill(uint32_t col, ko_knob_slot_t *k)
{
    /* SEQ page 5 (song setup): no knobs; KEY/SCALE/SWING/TEMPO are edited on GLO only.
     * GLO (BANK_GROOVE) fills via gv_enter_groove(), which fakes SEQ page 5: keep its knobs. */
    if (g.view == VIEW_SEQ && g.seq_page == 5u && g.bank != BANK_GROOVE) {
        k->off = 1u;
        return;
    }
    uint8_t b = knob_bar_b;
    const dub_params_t *mp = knob_bar_mp;
    uint8_t v = knob_bar_val(mp, (uint8_t)col);

    int is_kit_col = (g.view != VIEW_SEQ && g.bank == BANK_DRUM && bank_page() == 0u && col == 0u);
    int is_mod_col = (g.view != VIEW_SEQ && g.bank == BANK_BASS && bank_page() == 0u && col == 0u);
    uint16_t ecol = is_kit_col ? ERA_COL[DKIT_ERA[mp->drum_kit % DKIT_N] % GENRE_ERA_N] :
                    is_mod_col ? ERA_COL[BASS_ERA[mp->bass_model % BASS_N] % GENRE_ERA_N] : 0u;
    int has_ecol = is_kit_col || is_mod_col;

    k->klab = has_ecol ? ecol : C_GRAY;
    knob_label((uint8_t)col, k->label);
    if (g.view != VIEW_SEQ && g.bank == BANK_FX && bank_page() == 0u && col == 0u) {
        k->custom_arg = ((uint32_t)mp->dly_lvl * 255u) / 127u;
        k->custom_label = knob_lbl_dry_wet;
    }
    {
        uint8_t pan_raw;

        k->lcol = has_ecol ? ecol : BANK_COL[b];
        k->vcol = has_ecol ? ecol : C_WHITE;
        if (knob_slot_pan_raw(mp, (uint8_t)col, &pan_raw)) {
            int32_t pn = pan_parts(pan_raw, k->pan_num);

            k->pan = 1u;
            k->pan_side = (int8_t)(pn < 0 ? -1 : pn > 0 ? 1 : 0);
        } else {
            /* GLO (BANK_GROOVE) SCALE cue: dual label (live cue vs. stored scale), no value text. */
            int scale_cue_dual = g.view == VIEW_SEQ && g.seq_page == 5u && g.bank == BANK_GROOVE && col == 1u &&
                                 g.cue_on && g.cue_scale != g.cfg.p.scale;

            if (scale_cue_dual) {
                k->custom_value = knob_val_scale_cue;
            } else if (g.view != VIEW_SEQ && g.bank == BANK_LFO && bank_page() == 0u && col == 3u) {
                k->custom_arg = g.lfo_target % DUB_LFO_TGT_N;
                k->custom_value = knob_val_lfo_shape;
#if defined(GENRE_BASS)
            } else if (g.view != VIEW_SEQ && !g.seq_plock_hold && g.bank == BANK_CHORD && bank_page() == 1u &&
                       col == 2u) {
                k->custom_arg = ((uint32_t)bass_env_shape_curve(g.cfg.bass_env_shape) * 255u) / 127u;
                k->custom_value = fm_env_kit_knob_val_shape;
            } else if (g.view != VIEW_SEQ && !g.seq_plock_hold && g.bank == BANK_CHORD &&
                       bank_page() >= FM_ENV_KIT_PAGES && col == 0u) {
                k->custom_arg = g.cfg.bass.mods.env[g.bass_rt.env_edit_slot % BASS_MOD_SLOTS_PER_MAP].curve;
                k->custom_value = knob_val_mod_curve;
#endif
            } else {
                /* knob_value() may leave s untouched; the label then shows as value. */
                copy_str(k->value, k->label);
                knob_value((uint8_t)col, k->value);
            }
        }
    }
    {
        uint8_t pv, steps = 0u;
        int bip = knob_slot_bipolar(mp, (uint8_t)col, &pv);
        uint8_t mode = bip ? KNOB_POT_CONT : knob_slot_discrete((uint8_t)col, &steps);

        if (!bip)
            pv = v;
        if (g.view == VIEW_SEQ && !g.seq_plock_hold &&
            (fm1_in.buttons & (1u << BTN_LFO)) == 0u) {
#if defined(GENRE_BASS)
            if (g.seq_page == 1u && fm_seq_p1_fn((uint8_t)col) != 1u) {
                bip = 1;
                pv = knob_seq_bar_px((uint8_t)col);
            }
#else
            if (g.seq_page == 1u && fm_seq_p1_fn((uint8_t)col) != 1u) {
                bip = 1;
                pv = knob_seq_bar_px((uint8_t)col);
            }
#endif
        }
        if (mode == KNOB_POT_MASK)
            pv = (uint8_t)(g.voice.lane_mute_mask & 0x3Fu);
        k->pv = pv;
        k->bip = (uint8_t)(bip != 0);
        k->steps = steps;
        k->mode = mode;
        k->pot_col = has_ecol ? ecol : C_HI;
#if defined(GENRE_BASS)
        /* GLO MIDI CH tick ring: voice colour (BASS model), like KIT deck_vcol(cur_voice). */
        if (g.view != VIEW_SEQ && g.bank == BANK_GROOVE && bank_page() == 1u && col == 0u)
            k->pot_col = ERA_COL[BASS_ERA[mp->bass_model % BASS_N] % GENRE_ERA_N];
#endif
    }
    k->morph = (uint8_t)(dub_knob_morph_u8((uint8_t)col, &k->mf, &k->mt) != 0);
    k->lfo = (uint8_t)(dub_lfo_knob_modulated((uint8_t)col, mp) != 0);
    k->dj = (uint8_t)(dub_dj_knob_modulated((uint8_t)col) != 0);
    k->mark_col = C_ACC;
#if defined(GENRE_BASS)
    if (!k->lfo && !k->morph && bass_knob_mod_routed((uint8_t)col)) {
        k->lfo = 1u;
        k->mark_col = C_AMB;
    }
#endif
    if (k->dj)
        k->dj_col = dj_role_col();
    /* Readout base = stored base (cfg.p), not the LFO-modulated voice copy (rebuilt every draw).
     * Span sources (direct LFO if k->lfo, macro feeds, BASS routes) are resolved in knob_slot_apply_lfo_range(); 0 = live. */
    {
        uint8_t rb = knob_bar_val(&g.cfg.p, (uint8_t)col);
#if defined(GENRE_BASS)
        /* HOME p1 MAC 1..3: base = stored macro (dub_macro_ptr), not bass_mac_eff_raw() (already LFO-modulated). */
        if (g.view != VIEW_SEQ && g.bank == BANK_HOME && bank_page() == 1u && col >= 1u && col <= 3u)
            rb = *dub_macro_ptr((uint8_t)(col - 1u));
        /* ENV p0 SUST: knob_bar_val() returns g.synth.bass_env_cfg (mod + LFO); base = stored cfg. */
        if (g.view != VIEW_SEQ && g.bank == BANK_CHORD && bank_page() == 0u && col == 2u)
            rb = g.cfg.bass.env.t.sustain;
        /* ENV p0 DECAY: same as SUST; base = stored cfg, not g.synth.bass_env_cfg (mod + LFO). */
        if (g.view != VIEW_SEQ && g.bank == BANK_CHORD && bank_page() == 0u && col == 1u)
            rb = g.cfg.bass.env.t.decay;
        /* GROOVE p1 TRANSP / VEL: knob_bar_val() returns bass_transpose_eff() / bass_vel_sens_eff() (LFO). */
        if (g.view != VIEW_SEQ && g.bank == BANK_GROOVE && bank_page() == 1u && col == 2u)
            rb = (uint8_t)(((int32_t)g.cfg.bass.transpose + BASS_TRANSPOSE_MAX) * 127 / (2 * BASS_TRANSPOSE_MAX));
        if (g.view != VIEW_SEQ && g.bank == BANK_GROOVE && bank_page() == 1u && col == 3u)
            rb = g.cfg.bass.vel_sens;
#endif
        knob_slot_apply_lfo_range((uint8_t)col, k, rb);
    }
}

#include "genre_vox.h"

static void draw_knob_bar(void)
{
    ko_knob_bar_t bar = {0};

    knob_bar_b = (g.bank == BANK_SEQ ? g.seq_ret_bank : g.bank) % BANK_COUNT;
    knob_bar_mp = g.view == VIEW_SEQ ? &g.cfg.p : &g.voice.p;
    bar.h = DUB_KNOB_BAR_H;
    bar.blit_y = DUB_KNOB_BAR_BLIT_Y;
    bar.rail_y = DUB_KNOB_PROG_Y;
#if defined(GENRE_BASS)
    bar.center_label = 1u;
#endif
    bar.rail = draw_bar_progress_rail;
#if defined(GENRE_BASS)
    bar.fill = knob_slot_fill;
#else
    bar.fill = gv_knob_slot_fill;
#endif
    ko_draw_knob_bar(&bar);
}

static uint8_t bus_muted(uint32_t bus)
{
    return (g.voice.mute & (uint8_t)(1u << bus)) != 0u;
}

static void draw_view_seq(void);

static int32_t desk_siren_wave(uint32_t sph, uint8_t rate)
{
    int32_t sin_w = dub_sine_q64(sph);

    if (rate <= 76u)
        return sin_w;
    {
        int32_t sq_w = dub_square(sph) << 1;
        uint8_t m = rate - 76u;

        return (sin_w * (int32_t)(127 - m) + sq_w * (int32_t)m) / 127;
    }
}

#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
static void draw_view_desk(void)
{
    /* 4-Voice Drum & Siren Synth Scope (inspired by CHORD / BASS / ECHO diagrams):
     * - 4 scrolling waves (KICK, RIM, HAT, SRN) that all flow smoothly like CHORD & BASS
     * - DENS (0..127) directly controls the number of active rhythm transient pulses across the 188px scope
     *   (at DENS=0 the waves flatten; as DENS rises, more rhythmic transient bursts & offbeat pulses erupt!)
     * - KIT, TUNE, DECAY, DRIVE, KICK/RIM/HAT/SIREN levels, RATE, DELAY echo ghosts & PAN markers shape each trace */
    const dub_params_t *p = &g.voice.p;
    static const char *const VNAME[4] = {"KICK", "RIM", "HAT", "SRN"};
    static const int8_t kit_k_sweep[DKIT_N]= { 16, 12, 10, 32, 38, 14, 24, 44 };
    static const uint8_t kit_k_fm[DKIT_N]  = {  4, 12,  2, 24, 36,  6, 18, 60 };
    static const uint8_t kit_r_wire[DKIT_N]= { 28, 64, 16, 78, 36, 22, 86, 92 };
    uint8_t kit = p->drum_kit % DKIT_N;
    uint16_t kcol = ERA_COL[DKIT_ERA[kit] % GENRE_ERA_N];
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    uint8_t sel_ix = g.sel_drm % 4u;
    /* Echo re-trigger distance from TIME */
    int32_t dly_step = 24 + ((int32_t)dub_dly_div_ix(p->dly_time) * 28) / (int32_t)DUB_DLY_DIV_N;
    static const uint8_t PULSE_SP_TAB[9] = {188, 176, 88, 58, 44, 35, 29, 25, 22};
    uint32_t row, x;

    for (row = gv_desk_row0(); row < gv_desk_row_end(); row++) {
        int32_t dens = (int32_t)p->drm_dens[row];
        uint32_t npulses = dens < 8 ? 0u : (uint32_t)(1 + (dens * 7) / 127);
        uint32_t pulse_sp = PULSE_SP_TAB[npulses > 8u ? 8u : npulses];
        int32_t mid = gv_desk_mid(row);
        int muted = !dub_lane_audible((uint8_t)row);
        int solo = dub_lane_is_solo((uint8_t)row);
        uint8_t lvl = row == 0u ? p->kick_boom :
                      row == 1u ? p->rim_tone :
                      row == 2u ? p->hat_lvl : p->siren_pitch;
        int32_t live_env = row == 0u ? (g.synth.kick_env >> 8) :
                           row == 1u ? (g.synth.rim_env >> 8) :
                           row == 2u ? (g.synth.hat_env >> 8) : (g.synth.siren_env >> 8);
        uint8_t pan = p->drm_pan[row] ? p->drm_pan[row] : (row == 0u ? 64u : row == 1u ? 74u : row == 2u ? 52u : 82u);
        int32_t pan_x = 46 + ((int32_t)pan * 184) / 127;
        uint16_t lcol = solo ? C_ACC : (muted || lvl == 0u ? C_DIM : (row == sel_ix ? C_WHITE : (row == 3u ? C_ACC : kcol)));
        uint16_t base_col = muted || lvl == 0u || (row < 3u && dens == 0) ? C_DIM :
                            (live_env > 32 ? C_WHITE : (row == 0u ? kcol : row == 1u ? ecol : row == 2u ? C_HI : C_ACC));
        uint32_t bpm_v = p->bpm < 40u ? 40u : (p->bpm > 180u ? 180u : p->bpm);
        /* Continuous scrolling speed for all 4 rows (scales with TEMPO `bpm`, plus DENS / RATE) */
        uint32_t speed = (fm1_ms * (bpm_v / 14u + (uint32_t)(row == 3u ? p->siren_rate : (uint8_t)dens) / 14u)) << 17;
        uint32_t p_cnt = (row == 1u) ? (pulse_sp >> 1) : (row == 2u ? (pulse_sp >> 2) : 0u);
        uint32_t d_cnt = 0u;
        int32_t py = -1;
        int32_t main_env = (row == 3u) ? 220 : (dens == 0 ? 0 : (140 + (dens * 116) / 127));
        int32_t echo_env = 0;
        int32_t dly_d = (int32_t)p->drm_dly[row];
        int32_t kt = (int32_t)(p->drm_tune[row] ? p->drm_tune[row] : 64u) - 64;
        int32_t kd = (int32_t)(p->drm_dec[row] ? p->drm_dec[row] : 64u) - 64;
        int32_t kdrv = (int32_t)p->drm_drv[row];
        /* Decay rate across each burst responds to voice level + KIT DECAY */
        int32_t dec_drop = 960 - (((int32_t)lvl * 4 + (kd + 64) * 4) * 760) / (127 * 8);

        if (dec_drop < 110)
            dec_drop = 110;
        if (dec_drop > 1100)
            dec_drop = 1100;

        /* Left voice label — selected row matches SEQ lane chrome (inverted tag + band) */
        if (row == sel_ix) {
            int32_t band_y = mid - 13;
            int32_t band_h = 26;

            cv_rect(0, band_y, 240, band_h, RGB(18, 22, 42));
            if (!muted) {
                cv_rect(1, mid - 7, 38, 14, solo ? C_ACC : C_WHITE);
                cv_text(4, mid - 7, &FONT_S, VNAME[row], C_BLACK);
            } else {
                cv_rect(1, mid - 7, 38, 14, C_WHITE);
                cv_rect(3, mid - 5, 34, 10, C_BLACK);
                cv_text(4, mid - 7, &FONT_S, VNAME[row], C_WHITE);
            }
        } else
            cv_text(4, mid - 7, &FONT_S, VNAME[row], lcol);
        cv_line(46, mid, 234, mid, row == sel_ix ? C_DIM : C_LINE);
        cv_line(pan_x, mid - 3, pan_x, mid + 3, row == sel_ix ? C_WHITE : C_DIM);

        for (x = 0u; x < 188u; x++) {
            int32_t xp = 46 + (int32_t)x;
            int32_t w = 0;
            int32_t w_min = 0;
            int32_t w_max = 0;
            int32_t max_amp = ((3 + ((int32_t)lvl * 10) / 127) + (live_env >> 4));
            int32_t env, amp, dy, dy_lo, dy_hi, y;
            uint16_t col = base_col;

            if (max_amp > 14)
                max_amp = 14;

            p_cnt++;
            if (p_cnt >= pulse_sp)
                p_cnt = 0u;
            d_cnt++;
            if (d_cnt >= (uint32_t)dly_step)
                d_cnt = 0u;

            if (row < 3u) {
                /* DENS spawns repeated transient pulse crests across X (more pulses as DENS increases!) */
                if (npulses > 0u && x > 6u && p_cnt == 0u) {
                    int32_t re_lvl = 130 + (dens * 126) / 127;
                    if (re_lvl > main_env)
                        main_env = re_lvl;
                }
                /* DELAY spawns secondary echo bursts in C_ACC / C_HI */
                if (dly_d > 8 && x > 14u && d_cnt == 0u) {
                    int32_t burst = (210 * dly_d) / 127;
                    if (burst > echo_env)
                        echo_env = burst;
                }
                main_env -= (main_env * dec_drop) >> 12;
                if (main_env < (dens >> 2))
                    main_env = (dens >> 2);
                if (echo_env > 0)
                    echo_env -= (echo_env * (dec_drop + 160)) >> 13;
                env = main_env > echo_env ? main_env : echo_env;
                if (echo_env > main_env && !muted)
                    col = echo_env > 110 ? C_ACC : C_HI;
            } else {
                /* SIREN: continuous wail modulated by DLY send & live siren envelope */
                if (dly_d > 12 && x > 16u && d_cnt == 0u)
                    echo_env = (180 * dly_d) / 127;
                if (echo_env > 0)
                    echo_env -= (echo_env * 280) >> 13;
                env = 190 + (echo_env >> 2);
                if (echo_env > 110 && !muted)
                    col = C_HI;
            }

            if (row == 0u) {
                /* KICK: rolling sub-sweep sine + kit FM punch */
                uint32_t sweep = (uint32_t)(main_env * kit_k_sweep[kit]) << 17;
                uint32_t kph = (x * (uint32_t)(600 + (kt + 64) * 8 + dens * 3) << 15) + sweep + speed;
                int32_t fm = (dub_sine_q64(kph << 1) * (int32_t)kit_k_fm[kit] * env) >> 14;
                w = dub_sine_q64(kph + (uint32_t)(fm << 14));
            } else if (row == 1u) {
                /* RIM: dual-shell wood/metal transient + snare wire harmonics */
                uint32_t rph = (x * (uint32_t)(1200 + (kt + 64) * 14 + dens * 6) << 15) + (speed + (speed >> 1));
                int32_t sh = (dub_sine_q64(rph) + (dub_square(rph + (rph >> 1)) >> 1)) >> 1;
                int32_t wr = (dub_sine_q64((rph << 2) + speed) * (int32_t)kit_r_wire[kit]) >> 7;
                w = sh + (wr >> 1);
            } else if (row == 2u) {
                /* HAT: kit noise/metal balance + style (matches dub_hat_* tables) */
                uint32_t hph = (x * (uint32_t)(1800 + (kt + 64) * 18 + dens * 8) << 15) + (speed << 1);
                int32_t h_met = (dub_sine_q64(hph) * (int32_t)dub_hat_metal[kit]) >> 7;
                int32_t h_nz = (dub_noise(&g.synth.noise) * (int32_t)dub_hat_noise[kit]) >> 9;
                uint8_t hst = dub_hat_style[kit];

                if (hst == DUB_HAT_HISS)
                    h_met >>= 1;
                else if (hst == DUB_HAT_TR6)
                    h_met = (h_met >> 6) << 6;
                w = h_met + h_nz + (dub_square(hph + ((uint32_t)dub_hat_fm[kit] << 20)) >> 3);
            } else {
                /* SIREN: NJD LFO wail / square laser trill (min/max per column avoids beat-null viz) */
                uint32_t rate = (uint32_t)p->siren_rate;
                uint32_t mult = 380u + rate * 48u;
                uint64_t sph0 = ((uint64_t)x * (uint64_t)mult << 15) + (uint64_t)speed;
                uint32_t xinc = mult << 15;
                uint32_t si;

                for (si = 0u; si < 4u; si++) {
                    int32_t wi = desk_siren_wave((uint32_t)(sph0 + (((uint64_t)xinc * si) >> 2)), (uint8_t)rate);

                    if (kdrv > 0) {
                        wi = (wi * (256 + kdrv * 2)) >> 8;
                        if (wi > 32767) {
                            wi = 65534 - wi;
                            if (!muted && kdrv > 40)
                                col = C_ACC;
                        } else if (wi < -32767) {
                            wi = -65534 - wi;
                            if (!muted && kdrv > 40)
                                col = C_ACC;
                        }
                    }
                    if (si == 0u)
                        w_min = w_max = wi;
                    else {
                        if (wi < w_min)
                            w_min = wi;
                        if (wi > w_max)
                            w_max = wi;
                    }
                }
                w = (w_min + w_max) >> 1;
            }

            /* KIT DRIVE saturates & folds peaks, highlighting hot crests in C_ACC */
            if (kdrv > 0 && row < 3u) {
                w = (w * (256 + kdrv * 2)) >> 8;
                if (w > 32767) {
                    w = 65534 - w;
                    if (!muted && kdrv > 40)
                        col = C_ACC;
                } else if (w < -32767) {
                    w = -65534 - w;
                    if (!muted && kdrv > 40)
                        col = C_ACC;
                }
            }

            amp = (max_amp * env) >> 8;
            if (row == 3u) {
                dy_lo = (w_min * amp) / 32767;
                dy_hi = (w_max * amp) / 32767;
                if (dy_lo > 13)
                    dy_lo = 13;
                else if (dy_lo < -13)
                    dy_lo = -13;
                if (dy_hi > 13)
                    dy_hi = 13;
                else if (dy_hi < -13)
                    dy_hi = -13;
                dy = (dy_lo + dy_hi) >> 1;
                y = mid - dy;

                if (!muted && env > 135 && (dy_hi > 2 || dy_lo < -2) && (x & 1u) == 0u)
                    cv_line(xp, mid - dy_hi, xp, mid - dy_lo, echo_env > main_env ? C_LINE : C_DIM);
                cv_line(xp, mid - dy_hi, xp, mid - dy_lo, col);
                if ((live_env > 36 || kdrv > 48) && (mid - dy_hi) + 1 <= mid + 13)
                    cv_pset(xp, (mid - dy_hi) + 1, col);
                py = -1;
            } else {
                dy = (w * amp) / 32767;
                if (dy > 13)
                    dy = 13;
                else if (dy < -13)
                    dy = -13;
                y = mid - dy;

                /* Body fill inside strong transient / high-density regions (just like CHORD & BASS views!) */
                if (!muted && env > 135 && (dy > 2 || dy < -2) && (x & 1u) == 0u)
                    cv_line(xp, mid, xp, y, echo_env > main_env ? C_LINE : C_DIM);

                if (py >= 0) {
                    cv_line(xp - 1, py, xp, y, col);
                    if ((live_env > 36 || kdrv > 48) && y + 1 <= mid + 13)
                        cv_pset(xp, y + 1, col);
                }
                py = y;
            }
        }
    }
}

static void draw_view_home(void)
{
    /* 3D skewed audio-reactive dub terrain:
     * - Still when paused & silent (only moves with playback clock or live played voices)
     * - Each plane in depth = 1 real sequencer step (row = nplanes-1 is current step NOW at front,
     *   receding planes behind it are the echo/history of the past 11..15 steps!)
     * - Across X (left to right), the 4 voice buses have dedicated spatial & visual zones:
     *   Left/Center-Left = SUB/BASS (deep wide swell), Center = KICK/RIM/HAT (sharp transient spikes),
     *   Center-Right = CHORD SKANK (multi-peak harmonic organ/brass), Right = SIREN (high frequency ripple) */
    const dub_params_t *p = &g.voice.p;
    uint8_t m_chord = dub_eff_macro(0);
    uint8_t m_fb = dub_eff_macro(1);
    uint8_t m_wob = dub_eff_macro(2);
    uint8_t m_iso = dub_eff_macro(3);
    uint8_t m_space = dub_eff_macro(4);
    uint8_t m_weight = dub_eff_macro(5);
    uint8_t m_grit = dub_eff_macro(6);
    uint8_t m_dub = dub_eff_macro(7);
    uint8_t m_motion = dub_eff_macro(8);
    uint8_t m_flux = dub_eff_macro(9);
    int32_t flip_sign = (g.seq_flip_state & 1u) ? -1 : 1;
    uint32_t row, i;
    int32_t iso = ((int32_t)p->isolator - 64) + ((int32_t)m_iso >> 1);
    int32_t fb_tot = (int32_t)p->feedback + (int32_t)m_fb + ((int32_t)m_space >> 2);
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    int32_t wob_tot = (int32_t)p->wobble + (int32_t)m_wob + ((int32_t)m_weight >> 1);
    int32_t ch_tot = (int32_t)p->cutoff_c + (int32_t)m_chord + ((int32_t)m_grit >> 1);
    int32_t rev_tot = (int32_t)p->rev_mix + (int32_t)m_space;
    int32_t amp_scale = 256 + ((int32_t)m_chord * 384) / 127;
    int32_t roll_tilt = ((int32_t)m_wob * 26) / 127;
    int32_t fov_zoom = 100 + ((int32_t)m_space * 55) / 127;
    /* Live voice envelopes & meters (0 when silent or when that individual lane is muted: 0=K,1=R,2=H,3=S,4=B,5=C) */
    int32_t sub_e  = !dub_lane_heard_audible(4u) ? 0 : ((int32_t)g.synth.meter[0] + (g.synth.sub_env >> 7));
    int32_t chd_e  = !dub_lane_heard_audible(5u) ? 0 : ((int32_t)g.synth.meter[1] + (g.synth.ch_env >> 7));
    int32_t kick_e = !dub_lane_heard_audible(0u) ? 0 : (g.synth.kick_env >> 7);
    int32_t rim_e  = !dub_lane_heard_audible(1u) ? 0 : (g.synth.rim_env >> 7);
    int32_t hat_e  = !dub_lane_heard_audible(2u) ? 0 : (g.synth.hat_env >> 7);
    int32_t drm_e  = kick_e + rim_e + (hat_e >> 1);
    int32_t srn_e  = !dub_lane_heard_audible(3u) ? 0 : ((int32_t)g.synth.meter[3] + (g.synth.siren_env >> 7));
    /* Strictly transport-locked: completely still when paused (`!g.playing`), advances smoothly when playing */
    uint32_t step_frac256 = g.playing
                            ? dub_sample_acc_frac256(g.sample_acc, g.samples_per_step) : 0u;
    uint32_t motion_mul = 1u + (uint32_t)(m_motion > 48u ? (m_motion - 48u) / 24u : 0u);
    uint32_t beat_ph = g.playing
                           ? (((uint32_t)g.step << 24) + (step_frac256 * motion_mul << 16)) : 0u;
    uint32_t z_frac = (step_frac256 * motion_mul) & 255u;
    if (m_motion > 96u)
        z_frac = (uint8_t)(255u - z_frac);
    /* 3D vanishing point starts in the exact classic centered hero pose (120, 12 - iso/3) when macros are 0 */
    int32_t pan_bias = ((int32_t)(p->pan[1] ? p->pan[1] : 64u) - 64) / 2;
    int32_t flux_orb = m_flux > 0u
                       ? ((dub_sine_q64(beat_ph + ((uint32_t)m_flux << 18)) * ((int32_t)m_flux >> 2)) >> 15) : 0;
    int32_t flux_orb_y = m_flux > 0u
                         ? ((dub_sine_q64(beat_ph + ((uint32_t)m_flux << 20) + 0x40000000u) *
                             ((int32_t)m_flux >> 3)) >> 15) : 0;
    int32_t sway_x = (g.playing ? ((dub_sine_q64(beat_ph << 2) * (6 + (wob_tot >> 4))) >> 15) : 0) +
                     ((sub_e * ((dub_sine_q64((uint32_t)g.synth.sub_ph2) * 8) >> 15)) >> 7) + pan_bias +
                     flux_orb;
    int32_t vp_x = 120 + sway_x;
    int32_t vp_y = 12 - (iso / 3) - ((kick_e > 120 ? 120 : kick_e) >> 5) +
                   ((int32_t)m_iso * 36) / 127 + flux_orb_y;
    /* LAYER (m_fb) stacks extra 3D depth planes & echo strata on top of the classic 12-plane base */
    uint32_t nplanes = 12u + (uint32_t)(p->feedback / 40u) + (uint32_t)(((int32_t)m_fb * 8) / 127);
    if (nplanes > 22u)
        nplanes = 22u;

    /* Layer 1: Wide-angle 3D perspective depth rays spanning the full 240px width */
    for (i = 0u; i < 11u; i++) {
        int32_t fx = -16 + (int32_t)i * 27;
        int32_t bx = vp_x + ((fx - 120) * 74) / 128;
        int32_t fx_cl = fx < 1 ? 1 : (fx > 238 ? 238 : fx);
        uint16_t rcol = ((i & 1u) && (chd_e > 60 || rev_tot > 90)) ? C_DIM : C_LINE;
        cv_line(bx, vp_y + 4, fx_cl, 126, rcol);
    }

    /* Layer 2: Back-to-front 3D skewed wave planes.
     * Front plane (row = nplanes - 1) = current playing step `g.step` + live voice envelopes;
     * earlier planes (row < nplanes - 1) = actual sequencer steps behind/ahead in the 16-step groove! */
    for (row = 0u; row < nplanes; row++) {
        uint32_t dist_from_front = (nplanes - 1u) - row;
        uint8_t st_hist = (uint8_t)((g.step & ~(DUB_STEPS - 1u)) +
                                    ((g.step + DUB_STEPS - (dist_from_front & (DUB_STEPS - 1u))) &
                                     (DUB_STEPS - 1u)));
        uint8_t gate_h = step_gate(st_hist);
        /* Echo feedback decay into the distance (front plane = 256, back planes decay by FEEDBK/REGEN) */
        int32_t echo_att = 256 - (int32_t)(dist_from_front * (uint32_t)(180 - (fb_tot > 140 ? 140 : fb_tot))) / (int32_t)nplanes;
        /* Per-step voice activity from the actual pattern + live envelopes on the front planes */
        int32_t live_blend = dist_from_front == 0u ? 256 : (dist_from_front == 1u ? 140 : (dist_from_front == 2u ? 64 : 0));
        int32_t h_sub  = ((dub_lane_heard_audible(4u) && (gate_h & 16u)) ? (18 + (wob_tot >> 2)) : 3) * echo_att >> 8;
        int32_t h_kick = ((dub_lane_heard_audible(0u) && (gate_h & 1u))  ? (22 + (p->kick_boom >> 3)) : 0) * echo_att >> 8;
        int32_t h_rim  = ((dub_lane_heard_audible(1u) && (gate_h & 2u))  ? (18 + (p->rim_tone >> 3)) : 0) * echo_att >> 8;
        int32_t h_hat  = ((dub_lane_heard_audible(2u) && (gate_h & 4u))  ? (10 + (p->hat_lvl >> 4)) : 0) * echo_att >> 8;
        int32_t h_chd  = ((dub_lane_heard_audible(5u) && (gate_h & 32u)) ? (16 + (ch_tot >> 2)) : 2) * echo_att >> 8;
        int32_t h_srn  = ((dub_lane_heard_audible(3u) && (gate_h & 8u))  ? (16 + (p->siren_pitch >> 2)) : 0) * echo_att >> 8;

        int32_t z_lin = (int32_t)(((row << 8) + z_frac) / nplanes);
        int32_t z_persp = ((84 + ((z_lin * z_lin * 172) >> 16)) * fov_zoom) / 100;
        int32_t base_y = vp_y + ((z_lin * (118 - vp_y)) >> 8);
        int32_t skew_shift = ((256 - z_persp) * (18 + sway_x)) >> 8;
        int32_t half_w = (122 * z_persp) >> 8;
        int32_t cx = vp_x + skew_shift;
        int32_t x0 = cx - half_w;
        int32_t x1 = cx + half_w;
        uint32_t segs = 40u;
        uint32_t s_ix;
        int32_t py = -1, px = -1;
        /* Live audio envelopes boost the front planes directly (works even when paused and playing keys!) */
        h_sub  += (sub_e  * live_blend) >> 9;
        h_kick += (kick_e * live_blend) >> 9;
        h_rim  += (rim_e  * live_blend) >> 9;
        h_hat  += (hat_e  * live_blend) >> 10;
        h_chd  += (chd_e  * live_blend) >> 9;
        h_srn  += (srn_e  * live_blend) >> 9;

        if (x0 < 2)
            x0 = 2;
        if (x1 > 237)
            x1 = 237;
        if (x1 <= x0 + 8)
            continue;

        for (s_ix = 0u; s_ix <= segs; s_ix++) {
            int32_t xp = x0 + ((int32_t)s_ix * (x1 - x0)) / (int32_t)segs;
            /* Spatial voice envelopes across X (0..40):
             * - BASS/SUB: broad low-frequency swell on left-center (s_ix 0..22)
             * - KICK/RIM/HAT: punchy drum transient peak in center (s_ix 12..28)
             * - CHORD SKANK: 3-finger harmonic organ/brass chord ridges on center-right (s_ix 18..36)
             * - SIREN: fast NJD laser oscillation on far right (s_ix 26..40) */
            int32_t d_sub = (int32_t)s_ix - 10;
            int32_t env_sub_x = 256 - (d_sub * d_sub * 3) / 2;
            int32_t d_drm = (int32_t)s_ix - 20;
            int32_t env_drm_x = 256 - d_drm * d_drm * 4;
            int32_t d_chd = (int32_t)s_ix - 29;
            int32_t env_chd_x = 256 - d_chd * d_chd * 3;
            int32_t d_srn = (int32_t)s_ix - 35;
            int32_t env_srn_x = 256 - d_srn * d_srn * 6;
            uint32_t ph_anim = g.playing ? (beat_ph << 2) : (uint32_t)(g.synth.sub_ph >> 4);
            int32_t w_sub, w_drm, w_chd, w_srn, dy, y;
#ifdef GENRE_HEAVY_PSYCH
            /* DRONE: terrain moves at half speed. DOOM: 1.5x speed. */
            if (g.playing && p->era == ERA_DRONE)
                ph_anim >>= 1;
            else if (g.playing && p->era == ERA_DOOM)
                ph_anim += ph_anim >> 1;
#endif
            int32_t depth_bright, crest_glow, glow;
            uint16_t col;

            if (env_sub_x < 0) env_sub_x = 0;
            if (env_drm_x < 0) env_drm_x = 0;
            if (env_chd_x < 0) env_chd_x = 0;
            if (env_srn_x < 0) env_srn_x = 0;

            /* 1. SUB / BASS: deep fundamental wave + WOBBLE/WARP harmonic twist */
            w_sub = ((dub_sine_q64(s_ix * (0x07000000u + (uint32_t)wob_tot * 0x00090000u) +
                                   st_hist * 0x20000000u - ph_anim) * h_sub) >> 15) * env_sub_x >> 8;
            /* 2. DRUMS: KICK central dome + sharp RIM crack + HAT high-frequency ticks */
            w_drm = (((dub_sine_q64((uint32_t)(d_drm + 8) * 0x10000000u) * h_kick) >> 15) +
                     ((dub_sine_q64(s_ix * 0x28000000u + st_hist * 0x50000000u) * h_rim) >> 15)) * env_drm_x >> 8;
            if (h_hat > 0 && (s_ix & 1u))
                w_drm += (h_hat * env_drm_x) >> 9;
            /* 3. CHORD SKANK: multi-voice triad/7th interference pattern */
            w_chd = (((dub_sine_q64(s_ix * 0x16000000u + st_hist * 0x30000000u) +
                       (dub_sine_q64(s_ix * 0x22000000u - ph_anim) >> 1)) * h_chd) >> 15) * env_chd_x >> 8;
            /* 4. SIREN: high-pitched laser wave on the right edge */
            w_srn = ((dub_sine_q64(s_ix * (0x38000000u + (uint32_t)p->siren_rate * 0x00400000u) +
                                   (uint32_t)g.synth.siren_lfo_ph) * h_srn) >> 15) * env_srn_x >> 8;
            if (m_grit > 24u) {
                w_sub = (w_sub * 3) / 2 + ((s_ix & 3u) ? (h_sub >> 2) : -(h_sub >> 2));
                w_chd = (w_chd * 5) / 4 + ((s_ix & 1u) ? (h_chd >> 1) : 0);
            }
#ifdef GENRE_HEAVY_PSYCH
            /* Chord fuzz: jagged ridge edges, amount follows drive (p->drive) */
            w_chd += ((s_ix & 1u) ? 1 : -1) * ((h_chd * (int32_t)p->drive) >> 8);
#endif

            {
                int32_t cx_rel = (int32_t)s_ix - 20;
                int32_t roll_y = (cx_rel * roll_tilt * z_persp) / (20 * 256);
                int32_t weight_crater = ((cx_rel * cx_rel - 200) * (int32_t)m_weight * z_persp) / (1800 * 256);

                dy = ((((w_sub + w_drm + w_chd + w_srn) * z_persp) >> 8) * amp_scale * flip_sign) / 256;
                y = base_y - dy + roll_y - weight_crater;
            }
            if (m_chord > 96u && (s_ix & 7u) == 0u && dy > 6)
                cv_line(xp, y, xp, y - (dy >> 1), C_ACC);

            depth_bright = ((z_persp - 104) * 3) >> 1;
            crest_glow = ((dy < 0 ? -dy : dy) * 14) + ((sub_e + chd_e + drm_e) >> 2);
            glow = depth_bright + crest_glow - (row == 0u ? (int32_t)(255u - z_frac) >> 1 : 56);

            if (y < 3)
                y = 3;
            if (y > 127)
                y = 127;

            /* Color-code the active voice peaks so you can visually read which voice is firing! */
            if (env_srn_x > 64 && h_srn > 8)
                col = C_ACC;
            else if (env_drm_x > 80 && (h_kick > 12 || h_rim > 10) && (dy > 5 || dy < -5))
                col = C_WHITE;
            else if (env_chd_x > 80 && h_chd > 10 && (dy > 4 || dy < -4))
                col = C_HI;
            else if (st_hist & 1u)
                col = color_sat_wave(ERA_COL[((p->era + m_flux / 26u) % GENRE_ERA_N)], glow);
            else
                col = color_sat_wave(ERA_COL[((genre_era_of_preset(p->plate) + m_flux / 32u) % GENRE_ERA_N)],
                                     glow - 20);
            if (m_grit > 64u && (dy > 8 || dy < -8) && (s_ix & 3u) == 0u)
                cv_pset(xp, y, (s_ix & 1u) ? C_ACC : C_WHITE);

            if (px >= 0) {
                /* 3D hidden-surface occlusion curtain beneath the ridge so front waves mask back waves */
                int32_t drop = 4 + (z_persp >> 5);
                int32_t mx = (px + xp) >> 1;
                int32_t my = (py + y) >> 1;
                if (my + drop <= 128)
                    cv_line(mx, my + 1, mx, my + drop, C_BLACK);
                /* 3D side-wall strut on active voice peaks */
                if ((s_ix & 3u) == 0u && (dy > 4 || fb_tot > 70)) {
                    int32_t sx = xp - ((skew_shift >> 3) + 2);
                    int32_t sy = base_y + 2;
                    if (sx >= 2 && sx <= 237 && sy <= 127 && y < sy)
                        cv_line(xp, y, sx, sy, z_persp > 165 ? C_DIM : C_LINE);
                }
                cv_line(px, py, xp, y, col);
#ifdef GENRE_HEAVY_PSYCH
                /* Lead glow: second accent line above the siren ridges on the right */
                if (s_ix >= 26u && h_srn > 8 && py >= 3 && y >= 3)
                    cv_line(px, py - 2, xp, y - 2,
                            color_scale_rgb(C_ACC, 36 + ((h_srn * 2 > 255 ? 255 : h_srn * 2) / 3)));
#endif
                /* DUB macro: neon tape-echo halo + lush drop shadow on front waves (no geometry warp) */
                if (m_dub > 0u) {
                    uint32_t dub_front_span = 4u + ((uint32_t)m_dub >> 4);
                    if (row + dub_front_span >= nplanes) {
                        int32_t echo_dx = (int32_t)(m_dub >> 5);
                        uint32_t nglow = 1u + ((uint32_t)m_dub / 43u);
                        int32_t sh_end = 3 + (m_dub >> 4);
                        uint16_t sh_col = color_scale_rgb(ecol, 40 + (m_dub >> 1));
                        int32_t gx0 = px + echo_dx;
                        int32_t gx1 = xp + echo_dx;

                        if (nglow > 3u)
                            nglow = 3u;
                        if (py - 1 >= 3 && y - 1 >= 3)
                            cv_line(gx0, py - 1, gx1, y - 1,
                                    color_scale_rgb(col, 68 + (m_dub >> 2)));
                        if (py + 1 <= 127 && y + 1 <= 127)
                            cv_line(gx0, py + 1, gx1, y + 1,
                                    color_scale_rgb(col, 52 + (m_dub >> 2)));
                        if (nglow >= 2u) {
                            if (py - 2 >= 3 && y - 2 >= 3)
                                cv_line(gx0, py - 2, gx1, y - 2,
                                        color_scale_rgb(C_ACC, 44 + (m_dub >> 1)));
                            if (py + 2 <= 127 && y + 2 <= 127)
                                cv_line(gx0, py + 2, gx1, y + 2,
                                        color_scale_rgb(ecol, 40 + (m_dub >> 1)));
                        }
                        if (nglow >= 3u) {
                            int32_t echo_dx2 = echo_dx + (echo_dx > 0 ? 1 : (echo_dx < 0 ? -1 : 0));
                            if (py - 2 >= 3 && y - 2 >= 3)
                                cv_line(gx0 + echo_dx2, py - 2, gx1 + echo_dx2, y - 2,
                                        color_scale_rgb(C_ACC, 28 + (m_dub >> 2)));
                            if (py + 2 <= 127 && y + 2 <= 127)
                                cv_line(gx0 + echo_dx2, py + 2, gx1 + echo_dx2, y + 2,
                                        color_scale_rgb(ecol, 24 + (m_dub >> 2)));
                        }
                        if (py + 2 <= 127 && y + 2 <= 127)
                            cv_line(gx0, py + 2, gx1, y + 2, sh_col);
                        if (sh_end > 2 && py + sh_end <= 127 && y + sh_end <= 127)
                            cv_line(gx0, py + sh_end, gx1, y + sh_end,
                                    color_scale_rgb(ecol, 28 + (m_dub >> 2)));
                    }
                }
                /* LAYER (m_fb) adds stacked echo-strata contour lines underneath each 3D wave ridge */
                if (m_fb > 20u) {
                    int32_t lgap = 2 + (m_fb >> 5);
                    if (py + lgap <= 127 && y + lgap <= 127)
                        cv_line(px, py + lgap, xp, y + lgap, z_persp > 155 ? C_DIM : C_LINE);
                }
                /* Thick foreground rib on the closest 3D planes when bass/kick hits or WEIGHT is high */
                if ((z_persp > 195 || m_weight > 48u) && (h_sub > 14 || h_kick > 14 || h_chd > 14 || m_weight > 64u) &&
                    y + 1 <= 127)
                    cv_line(px, py + 1, xp, y + 1, col);
            }
            px = xp;
            py = y;
        }
    }
}
#elif defined(GENRE_POP)
#include "pop_draw.h"
#elif defined(GENRE_JAZZ)
#include "jazz_draw.h"
#elif defined(GENRE_SOUL)
#include "soul_draw.h"
#elif defined(GENRE_HIPHOP)
#include "hiphop_draw.h"
#elif defined(GENRE_SNOWCRASH)
#include "snowcrash_draw.h"
#elif defined(GENRE_TECHNO)
#include "techno_draw.h"
#elif defined(GENRE_TRIPHOP)
#include "triphop_draw.h"
#elif defined(GENRE_BASS)
#include "bass_draw.h"
#elif defined(GENRE_SNOWCRASH)
#include "snowcrash_draw.h"
#elif defined(GENRE_IDM)
#include "idm_draw.h"
#elif defined(GENRE_FUNK)
#include "funk_draw.h"
#elif defined(GENRE_HOUSE)
#include "house_draw.h"
#elif defined(GENRE_HEAVY_PSYCH)
#include "heavy_psych_draw.h"
#endif /* genre fork draw views */

/* SEQ view: OCT -/+ step through all FM_SEQ_MAX_PAGES views (S1..S8) whatever the length or seq_view_on,
 * stopping at S1 / S8. Views past the length are shown and edited, not played.
 * EDIT + OCT -/+ stays the tape octave. */
static void seq_oct(int32_t dir)
{
#if defined(GENRE_BASS)
    (void)dir;
    return;
#endif
    uint8_t v = (uint8_t)(g.seq_view_page % FM_SEQ_MAX_PAGES);

    /* OCT loops S1..S8: S8 +1 goes to S1, S1 -1 goes to S8. */
    if (dir > 0)
        v = (uint8_t)((v + 1u) % FM_SEQ_MAX_PAGES);
    else if (dir < 0)
        v = (uint8_t)((v + FM_SEQ_MAX_PAGES - 1u) % FM_SEQ_MAX_PAGES);
    g.seq_view_page = v;
    seq_view_rep_arm(v);
    /* OCT in SEQ leaves follow: no cursor toggle; playback must not snap the page back. */
    g.seq_follow = 0;
    g.ui_dirty = 1u;
}

static void seq_toggle_step(uint8_t st, uint8_t lane)
{
#if defined(GENRE_BASS)
    bass_seq_toggle_step(st);
    return;
#endif
    dub_step_t *s = &g.cfg.seq[st];
    uint8_t bit = (uint8_t)(1u << (lane % DUB_SEQ_LANES));
    uint8_t ln = lane % DUB_SEQ_LANES;
    int on = (int)((raw_step_gate_show(st) & bit) != 0);

    if (!on && (g.euclid_hits[ln] == FM_SEQ_EUCLID_EMPTY || seq_lane_no_gate_bits(ln)))
        seq_lane_arm_step(ln, st);
    else {
        seq_materialize_all();
        s->gate ^= bit;
        if (s->gate & bit)
            s->force = 1u;
    }
    seq_lane_pattern_mark_edited(ln);
#if defined(GENRE_BASS)
    bass_edit_lock_step(st);
#endif
    mark_dirty();
    g.ui_dirty = 1u;
}

#ifdef FM_L3_GATE

static uint8_t seq_lane3_gate16_at(uint8_t gs)
{
    int8_t raw = g.step_micro[3][gs];

#if defined(FM_L3_PLATE_DEF)
    if (raw == 0)
        raw = (int8_t)genre_lead_len_ix(g.cfg.p.plate, gs);
#elif defined(FM_L3_TIME_DEF)
    if (raw == 0 && g.samples_per_step > 0u) {
        uint16_t hold = 0u;
        uint16_t rel = 0u;
        uint32_t n;

        FM_L3_TIME_DEF(0u, lane_step_gate_rnd(3u, gs), &hold, &rel, 0u, 0u, g.samples_per_step);
        n = ((uint32_t)hold + (uint32_t)rel + g.samples_per_step / 2u) / g.samples_per_step;
        return (uint8_t)(n < 1u ? 1u : n > 64u ? 64u : n);
    }
#endif
    return FM_L3_LEN_16THS[FM_L3_LEN_IX(raw)];
}

static void seq_lane3_apply_gate_spans(fm_seq_cell_t *row, const uint8_t *base_trig, uint32_t base)
{
    uint32_t st;

    for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
        uint8_t span;
        uint32_t ns;

        row[st].note_span = 0u;
        row[st].note_tail = 0u;
        if (!base_trig[st])
            continue;
        span = seq_lane3_gate16_at((uint8_t)(base + st));
        if (span < 1u)
            span = 1u;
        for (ns = st + 1u; ns < FM_SEQ_PAGE_STEPS; ns++) {
            if (base_trig[ns]) {
                if (ns - st < span)
                    span = (uint8_t)(ns - st);
                break;
            }
        }
        if (st + span > FM_SEQ_PAGE_STEPS)
            span = (uint8_t)(FM_SEQ_PAGE_STEPS - st);
        if (span < 1u)
            span = 1u;
        row[st].note_span = span;
        for (ns = 1u; ns < span; ns++) {
            fm_seq_cell_t *tail = &row[st + ns];

            tail->note_tail = 1u;
            tail->col = 0;
            tail->vel = 0;
        }
    }
}
#endif

static void draw_view_seq(void)
{
#if defined(GENRE_BASS)
    bass_draw_view_seq();
    return;
#endif
#if defined(GENRE_HEAVY_PSYCH)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "N", "H", "L", "B", "C"};
#elif defined(GENRE_HOUSE)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "N", "H", "S", "B", "C"};
#elif defined(GENRE_HIPHOP)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "N", "H", "X", "B", "C"};
#elif defined(GENRE_JAZZ)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "D", "H", "B", "C"};
#elif defined(GENRE_IDM)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "D", "T", "B", "C"};
#elif defined(GENRE_SNOWCRASH)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "D", "P", "B", "C"};
#elif defined(GENRE_POP)
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "H", "L", "B", "C"};
#else
    static const char *const LBL[DUB_SEQ_LANES] = {"K", "R", "H", "S", "B", "C"};
#endif
    const dub_params_t *mp = &g.voice.p;
    uint16_t lcols[DUB_SEQ_LANES];
    fm_seq_cell_t cells[DUB_SEQ_LANES * FM_SEQ_PAGE_STEPS];
    uint8_t lane = g.seq_lane % DUB_SEQ_LANES;
    int all_lanes = seq_lane_is_all();
    uint32_t st;
    uint32_t ln;
    uint32_t base = seq_view_base();
    uint8_t view_pg = (uint8_t)((base / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES);
    int view_off = !g.seq_view_on[view_pg];
    uint8_t plock_st = FM_SEQ_PLOCK_NONE;
    uint8_t play_lane_st[DUB_SEQ_LANES];
    int8_t play_lane_tun[DUB_SEQ_LANES];
#ifdef FM_L3_GATE
    uint8_t l3_base_trig[FM_SEQ_PAGE_STEPS];
#endif

    if (g.seq_plock_hold && g.seq_plock_step >= base && g.seq_plock_step < base + FM_SEQ_PAGE_STEPS)
        plock_st = (uint8_t)(g.seq_plock_step - base);
    uint16_t empty_act = RGB(18, 22, 42);
    const uint16_t mute_col = RGB(22, 26, 44);
    const uint16_t mute_bg = RGB(8, 10, 18);

    for (ln = 0; ln < DUB_SEQ_LANES; ln++) {
        /* SEL level for this lane (0=K, 1=R, 2=H, 3=S, 4=B, 5=C) */
        uint8_t sel_lvl = ln == 0u ? mp->kick_boom :
                          ln == 1u ? mp->rim_tone :
                          ln == 2u ? mp->hat_lvl :
                          ln == 3u ? mp->siren_pitch :
                          ln == 4u ? mp->lvl_b : mp->lvl_c;
        uint16_t play_col = dub_seq_lane_play_col((uint8_t)ln);
        uint16_t lcol = C_GRAY;
        int lane_muted = !dub_lane_audible((uint8_t)ln) || sel_lvl == 0u;

        if (dub_lane_is_solo((uint8_t)ln))
            lcol = C_ACC;
        else if (lane_muted)
            lcol = C_DIM;
        else if (all_lanes || ln == lane)
            lcol = C_WHITE;
        else
            lcol = play_col;
        if (g.playing && ((g.voice.lane_mute_mask ^ g.voice.heard_mute_mask) >> ln) & 1u)
            lcol = ((fm1_ms / 80u) & 1u) ? C_WHITE : C_ACC;
        lcols[ln] = lcol;
        {
            /* K+S follow slip when CROP/loop active; rest follow looped g.step */
            uint8_t src = (g.loop_active && g.loop_len > 0u && ln < 2u) ? g.slip_step : g.step;
            uint8_t mplay;

            if (dub_lane_is_sub((uint8_t)ln))
                src = g.lane_st[ln];
            mplay = seq_lane_step_map(src, (uint8_t)ln);

            play_lane_st[ln] = mplay >= base && mplay < base + FM_SEQ_PAGE_STEPS ? (uint8_t)(mplay - base) :
                               0xFFu;
            play_lane_tun[ln] = g.playing ?
                dub_eff_lane_nudge((uint8_t)((src / FM_SEQ_PAGE_STEPS) % FM_SEQ_MAX_PAGES), (uint8_t)ln) : 0;
        }
        for (st = 0; st < FM_SEQ_PAGE_STEPS; st++) {
            uint32_t ab = base + st;
            uint8_t gs = (uint8_t)ab;
            /* Show pattern even when view CYC=OFF; LFO euclid/rot uses same eff_on as playback. */
            int base_on;

            if ((g.lfo_slots[DUB_LFO_SEQ_PAT].depth > 0u && g.seq_lfo_lane[0] == ln &&
                 fm_seq_euclid_is_pattern(g.euclid_hits[ln])) ||
                (g.lfo_slots[DUB_LFO_SEQ_ROT].depth > 0u && g.seq_lfo_lane[1] == ln))
                base_on = dub_lane_step_eff_on((uint8_t)ln, gs, 0);
            else {
                uint8_t rgate = raw_step_gate_show(gs);

                base_on = (int)((rgate & (1u << ln)) != 0);
            }
            int on = fm_seq_vary_hit(base_on, dub_eff_vary((uint8_t)ln), gs, g.bar, (uint8_t)ln, g.seq_rng);
            uint16_t col = 0;
            uint16_t bg_col = (all_lanes || ln == lane) ? empty_act : C_LINE;
            fm_seq_cell_t *cell = &cells[ln * FM_SEQ_PAGE_STEPS + st];
            int32_t tune;
            uint8_t vel = 0;

            if (on && !fm_seq_trig_pass(dub_eff_trig_raw((uint8_t)ln), gs, g.bar, (uint8_t)ln))
                on = 0;
            if (all_lanes || ln == lane) {
                bg_col = empty_act;
                if (base_on || on)
                    vel = seq_lane_velocity((uint8_t)ln, gs, g.cfg.seq[gs].accent, 0u);
                if (!on && base_on)
                    col = C_DIM;
                else if (!on)
                    col = 0;
                else if (!base_on)
                    col = C_ACC;
                else if (seq_step_has_plock(ln, gs))
                    col = C_ACC;
                else if (fm_seq_euclid_is_pattern(dub_eff_euclid_hits((uint8_t)ln)) || ((g.cfg.seq[gs].force & 1u) &&
                         ((g.cfg.seq[gs].gate ^ seq_plate_gate(gs)) & (1u << ln))))
                    col = C_WHITE;
                else {
                    uint8_t vis_lvl = sel_lvl > 0u ? sel_lvl : g.voice.saved_lane_lvl[ln];
                    col = color_sat_wave(play_col, ((int32_t)vis_lvl - 80) * 2);
                }
            } else if (lane_muted && (base_on || on)) {
                bg_col = mute_bg;
                col = mute_col;
                vel = seq_lane_velocity((uint8_t)ln, gs, g.cfg.seq[gs].accent, 0u);
            } else if (!on) {
                if (base_on) {
                    col = C_DIM;
                    vel = seq_lane_velocity((uint8_t)ln, gs, g.cfg.seq[gs].accent, 0u);
                } else
                    col = 0;
            } else {
                vel = seq_lane_velocity((uint8_t)ln, gs, g.cfg.seq[gs].accent, 0u);
                if (!base_on)
                    col = C_ACC;
                else if (fm_seq_euclid_is_pattern(dub_eff_euclid_hits((uint8_t)ln)) ||
                         dub_eff_vary((uint8_t)ln) > 0u || seq_step_has_plock(ln, gs))
                    col = C_ACC;
                else
                    col = play_col;
            }
            tune = (int32_t)lane_step_tune((uint8_t)ln, gs);
            if (tune < -24)
                tune = -24;
            if (tune > 24)
                tune = 24;
            cell->bg_col = bg_col;
            cell->col = col;
            cell->vel = vel;
            cell->tune = (int8_t)tune;
#ifdef FM_GENRE_STEP_MICRO
            cell->micro = ln == 3u ? 0 : dub_eff_step_micro((uint8_t)ln, gs);
#else
            cell->micro = dub_eff_step_micro((uint8_t)ln, gs);
#endif
            cell->cond = g.step_prob[ln][gs];
            cell->gate = lane_step_gate_rnd((uint8_t)ln, gs);
            cell->note_span = 0u;
            cell->note_tail = 0u;
#ifdef FM_L3_GATE
            if (ln == 3u)
                l3_base_trig[st] = base_on ? 1u : 0u;
#endif
            if (view_off && cell->col != 0u)
                cell->col = C_DIM;
        }
    }
#ifdef FM_L3_GATE
    seq_lane3_apply_gate_spans(&cells[3u * FM_SEQ_PAGE_STEPS], l3_base_trig, base);
#endif
    fm_seq_draw_grid(DUB_SEQ_LANES, all_lanes ? DUB_SEQ_LANE_ALL : lane, 0xFFu, g.playing, plock_st,
                     g.seq_plock_latch ? 1 : 0, lane, ERA_COL[g.cfg.p.era % GENRE_ERA_N], LBL, lcols,
                     cells, play_lane_st, play_lane_tun, 0);
    if (fm1_ms < g.seq_clip_toast_until && g.seq_clip_toast[0]) {
        int32_t tw = text_w(&FONT_S, g.seq_clip_toast);
        int32_t fx = (240 - tw) / 2;
        int32_t fy = 58;

        cv_rect(fx - 3, fy - 1, tw + 6, 11, C_BLACK);
        cv_text(fx, fy, &FONT_S, g.seq_clip_toast, C_WHITE);
    }
}

static int32_t dub_bass_trace_sample(void *ctx, uint32_t ph, uint32_t cycle)
{
    const dub_params_t *p = &g.voice.p;
    uint8_t model = p->bass_model % BASS_N;
    int32_t wob = (int32_t)p->wobble + (int32_t)g.voice.macro_wob;
    int32_t cut = (int32_t)p->cutoff_b;
    int32_t wob_mod;
    uint32_t wph;
    int32_t v, fund;

    (void)ctx;
    if (wob > 127)
        wob = 127;
    /* WOBBLE warps phase & harmonic fold across ALL bass models so knob 4 is always visible */
    wob_mod = (dub_sine_q64((ph << 1) + (cycle << 29)) * wob) / 127;
    wph = ph + (uint32_t)(wob_mod << 15);
    fund = dub_sine_q64(wph);

    switch (model) {
#ifdef GENRE_HEAVY_PSYCH
    case BASS_MUFF:
        v = (dub_square(wph) + (dub_square(wph + (wph >> 2)) >> 1)) >> 1;
        break;
    case BASS_WAH:
        v = dub_saw(wph) - (dub_square(wph) >> 2);
        break;
    case BASS_DRONE:
        v = dub_sine_q64(wph) + (dub_sine_q64(wph >> 1) >> 1);
        break;
#elif !defined(GENRE_HOUSE) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    case BASS_REESE: {
        uint32_t det = (ph >> 3) + (uint32_t)(wob << 22);
        v = (dub_saw(wph) + dub_saw(wph + det) + dub_saw(wph - det)) / 3;
        break;
    }
#endif
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    case BASS_WOBBLE: {
        int32_t fm = (dub_sine_q64(ph << 2) * (32 + wob)) / 127;
        v = dub_sine_q64(wph + (uint32_t)(fm << 16));
        break;
    }
    case BASS_WG:
        v = fm_lfo_eval_full_q15(wph, cycle, 0u, 64u, 64u, FM_LFO_ALG_PLUCK, 52u,
                                 (uint8_t)(24u + (cut >> 1) + (wob >> 2)), p->drive, 0u);
        break;
#endif
#ifdef GENRE_HEAVY_PSYCH
    case BASS_DOOM: {
        uint32_t p8 = wph + ((0u - ph) >> 2);
        v = dub_sine_q64(p8) + (dub_sine_q64(p8 << 1) >> 2);
        break;
    }
#elif defined(GENRE_JAZZ)
    case BASS_UPRIGHT:
    case BASS_WALK:
    case BASS_PIZZ:
        v = fund + (fund >> 2);
        break;
    case BASS_ORGAN:
        v = (dub_square(wph) >> 1) + (dub_sine_q64(wph >> 1) >> 1);
        break;
    case BASS_AMP:
    case BASS_FUSION:
    case BASS_MOOG:
        v = dub_saw(wph);
        break;
    case BASS_FM: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (40 + cut)) >> 7;
        int32_t m3 = (dub_sine_q64(wph * 3u) * (24 + wob)) >> 7;

        v = dub_sine_q64(wph + (uint32_t)((m2 + m3) << 15));
        break;
    }
    case BASS_EP: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (32 + cut)) >> 7;

        v = dub_sine_q64(wph + (uint32_t)(m2 << 15));
        break;
    }
#elif defined(GENRE_IDM)
    case BASS_ROUND:
    case BASS_SUB:
    case BASS_TWANG:
        v = fund + (fund >> 2);
        break;
    case BASS_THROB:
        v = (dub_square(wph) >> 1) + (dub_sine_q64(wph >> 1) >> 1);
        break;
    case BASS_TEAR:
    case BASS_SAND:
    case BASS_BEND:
    case BASS_STATIC:
        v = dub_saw(wph);
        break;
    case BASS_CLICK: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (40 + cut)) >> 7;
        int32_t m3 = (dub_sine_q64(wph * 3u) * (24 + wob)) >> 7;
        v = dub_sine_q64(wph + (uint32_t)((m2 + m3) << 15));
        break;
    }
#elif defined(GENRE_SNOWCRASH)
    case BASS_SUB:
    case BASS_HUM:
    case BASS_GLOSS:
        v = fund + (fund >> 2);
        break;
    case BASS_CLICK:
        v = fund + (dub_sine_q64(wph << 2) >> 3);
        break;
    case BASS_808:
    case BASS_TIDE:
        v = fund + (dub_sine_q64(wph << 1) >> 2);
        break;
    case BASS_REESE:
    case BASS_303:
    case BASS_FM:
    case BASS_MOOG:
    case BASS_PWM:
    case BASS_PLUCK:
    case BASS_RUST:
    case BASS_UNDRT:
        v = dub_saw(wph);
        break;
#elif defined(GENRE_POP)
    case BASS_TINE:
        v = fund + (dub_sine_q64(wph << 1) >> 2);
        break;
    case BASS_2OP: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (32 + cut)) >> 7;
        v = dub_sine_q64(wph + (uint32_t)(m2 << 15));
        break;
    }
    case BASS_3OP: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (28 + cut)) >> 7;
        int32_t m3 = (dub_sine_q64(wph * 3u) * (20 + wob)) >> 7;
        v = dub_sine_q64(wph + (uint32_t)((m2 + m3) << 15));
        break;
    }
    case BASS_4OP: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (24 + cut)) >> 7;
        int32_t m3 = (dub_sine_q64(wph * 3u) * (18 + wob)) >> 7;
        int32_t m4 = (dub_sine_q64(wph << 2) * (14 + cut)) >> 7;
        v = dub_sine_q64(wph + (uint32_t)((m2 + m3 + m4) << 15));
        break;
    }
    case BASS_BRASS:
        v = (dub_saw(wph) >> 1) + (dub_sine_q64(wph) >> 1);
        break;
#elif !defined(GENRE_HOUSE) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    case BASS_808: {
        uint32_t p8 = wph + ((0u - ph) >> 2);
        v = dub_sine_q64(p8) + (dub_sine_q64(p8 << 1) >> 2);
        break;
    }
#endif
#if !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_HOUSE)
    case BASS_303: {
        int32_t s3 = dub_saw(wph) + (dub_square(wph) >> 1);
        int32_t fold = 256 + wob * 3 + (int32_t)p->drive * 2;
        v = (s3 * fold) >> 8;
        if (v > 32767)
            v = 65534 - v;
        else if (v < -32767)
            v = -65534 - v;
        break;
    }
#endif
#if !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL) && !defined(GENRE_JAZZ) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS)
    case BASS_FM: {
        int32_t m2 = (dub_sine_q64(wph << 1) * (40 + cut)) >> 7;
        int32_t m3 = (dub_sine_q64(wph * 3u) * (24 + wob)) >> 7;
        v = dub_sine_q64(wph + (uint32_t)((m2 + m3) << 15));
        break;
    }
#endif
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    case BASS_PWM: {
        uint32_t duty = 0x20000000u + ((uint32_t)cut << 23) + ((uint32_t)wob << 22);
        v = ((wph < duty) ? 20000 : -20000) + (((wph >> 1) & 0x80000000u) ? 9000 : -9000);
        break;
    }
#endif
#ifdef GENRE_HEAVY_PSYCH
    case BASS_FUZZ: {
        v = (dub_square(wph) >> 1) + (dub_sine_q64(wph >> 1) >> 1) + (dub_sine_q64(wph + (wph >> 1)) >> 2);
        break;
    }
    case BASS_OCT:
        v = dub_sine_q64(wph) + (dub_sine_q64(wph >> 1) >> 1);
        break;
    case BASS_MOOG: {
        int32_t mg = dub_saw(wph) - (dub_saw(wph + (ph >> 4)) >> 2);
        v = (mg * (180 + cut + (wob >> 1))) >> 8;
        break;
    }
    case BASS_PLUCK: {
        uint32_t atk = (cycle < (cycle >> 2) + 64u) ? cycle : (cycle >> 2);
        int32_t env_mod = ((int32_t)(32767 - (int32_t)(atk & 0x7FFFu)) * (48 + wob)) >> 6;
        v = dub_sine_q64(wph + (uint32_t)(env_mod << 10));
        break;
    }
#elif defined(GENRE_HOUSE)
    case BASS_ORGAN: {
        v = (dub_square(wph) >> 1) + (dub_sine_q64(wph >> 1) >> 1) + (dub_sine_q64(wph + (wph >> 1)) >> 2);
        break;
    }
    case BASS_OCTAVE:
        v = dub_sine_q64(wph) + (dub_sine_q64(wph >> 1) >> 1);
        break;
    case BASS_MOOG: {
        int32_t mg = dub_saw(wph) - (dub_saw(wph + (ph >> 4)) >> 2);
        v = (mg * (180 + cut + (wob >> 1))) >> 8;
        break;
    }
    case BASS_PLUCK: {
        uint32_t atk = (cycle < (cycle >> 2) + 64u) ? cycle : (cycle >> 2);
        int32_t env_mod = ((int32_t)(32767 - (int32_t)(atk & 0x7FFFu)) * (48 + wob)) >> 6;
        v = dub_sine_q64(wph + (uint32_t)(env_mod << 10));
        break;
    }
#endif
#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH) && !defined(GENRE_HIPHOP) && !defined(GENRE_POP) && !defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_BASS) && !defined(GENRE_TRIPHOP) && !defined(GENRE_SOUL)
    case BASS_WOB: {
        int32_t w_lfo = dub_sine_q64(ph + (cycle << 27));
        int32_t sq_mod = dub_square(wph + (uint32_t)(w_lfo << 17));
        v = (dub_square(wph + (uint32_t)(sq_mod >> 1)) >> 1) + (fund >> 1);
        break;
    }
    case BASS_SUBS:
        v = fund + (dub_sine_q64(wph >> 1) >> 1) + (dub_sine_q64(wph << 1) >> 3);
        break;
#endif
    default:
        v = dub_sine_q64(wph + (uint32_t)((dub_sine_q64(ph << 1) * wob) << 12));
        break;
    }
    /* CUTOFF morphs between pure fundamental sine (cut=0) and full harmonic/edged model (cut=127) */
    v = ((fund * (127 - cut)) + (v * cut)) / 127;
    if (p->drive > 0u)
        v = dub_softclip_q15((v * (256 + (int32_t)p->drive * 2)) >> 8);
    return v;
}

static void draw_view_panel(const char *title)
{
    const dub_params_t *p = &g.voice.p;
    uint16_t ecol = ERA_COL[p->era % GENRE_ERA_N];
    char s[24];

    cv_text(4, 4, &FONT_S, title, C_WHITE);
#if defined(GENRE_BASS)
    if (g.view == VIEW_BASS) {
        copy_str(s, BASS_NAMES[p->bass_model % BASS_N]);
        cv_text(96, 4, &FONT_S, s, ERA_COL[BASS_ERA[p->bass_model % BASS_N] % GENRE_ERA_N]);
        return;
    }
#endif
    if (g.view == VIEW_CHORD) {
        copy_str(s, VOICE_NAMES[p->voicing % VOICE_N]);
        cv_text(96, 4, &FONT_S, s, C_ACC);
        {
#ifdef GENRE_HEAVY_PSYCH
            static const int8_t iv[VOICE_N][4] = {
                {0, 7, 12, 19}, {0, 4, 7, 11}, {0, 2, 7, 10}, {0, 12, 19, 24}, {0, 3, 7, 10}
            };
#elif defined(GENRE_HOUSE)
            static const int8_t iv[VOICE_N][4] = {
                {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 10}, {0, 4, 7, 14}, {0, 4, 7, 9}
            };
#elif defined(GENRE_POP)
            static const int8_t iv[VOICE_N][4] = {
                {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, 10}
            };
#else
            static const int8_t iv[VOICE_N][4] = {
                {0, 3, 7, 10}, {0, 3, 10, 14}, {0, 5, 7, 10}, {0, 5, 10, 15}, {0, 7, 12, 12}
            };
#endif
            static const int32_t lane_y[4] = {112, 88, 64, 40};
            uint32_t v, x;
            int32_t cut = (int32_t)p->cutoff_c;
            int32_t res = (int32_t)p->reso_mode;
            int32_t env_f = (int32_t)p->env_filt;
            int32_t rev_c = (int32_t)p->rev_c;
            int32_t lvl_c = (int32_t)p->lvl_c;
            int32_t pan_c = (int32_t)(p->pan[1] ? p->pan[1] : 64u);
            uint8_t smooth = cut < 112 ? (uint8_t)(124 - (cut * 118) / 112) : 0u;
            /* DECAY controls body length from tight 20 px chop to full 224 px sustain */
            int32_t dec_drop = 920 - ((int32_t)p->decay_c * 860) / 127;
            /* TIME sets distance between echo bursts; DELAY sets echo burst height */
            int32_t dly_step = 32 + ((int32_t)dub_dly_div_ix(p->dly_time) * 38) / 11;
            uint32_t bpm_c = p->bpm < 40u ? 40u : (p->bpm > 180u ? 180u : p->bpm);
            uint32_t speed = (fm1_ms * (bpm_c / 14u + (uint32_t)cut / 24u)) << 17;
            int32_t pan_x = 8 + ((pan_c - 1) * 224) / 126;

            if (pan_x < 8)
                pan_x = 8;
            if (pan_x > 231)
                pan_x = 231;
            cv_line(pan_x, 148, pan_x, 156, C_DIM);

            for (v = 0; v < 4u; v++) {
                int32_t semi = iv[p->voicing % VOICE_N][v];
                int32_t cy = lane_y[v] - ((semi & 7) - 3);
                uint16_t base_col = bus_muted(1u) ? C_DIM : (cut < 28 ? C_GRAY : (v == 0u ? C_WHITE : ecol));
                int32_t main_env = 256;
                int32_t echo_env = 0;
                int32_t lp = 0;
                int32_t py = -1;

                cv_line(8, cy, 231, cy, C_LINE);
                for (x = 0; x < 224u; x++) {
                    int32_t xp = 8 + (int32_t)x;
                    /* ENVLPE compresses cycles at attack (left) and relaxes rightwards; VOICE shifts pitch spacing */
                    uint32_t env_sweep = (uint32_t)((main_env * env_f) >> 5) * 420000u;
                    uint32_t freq = 65000000u + (uint32_t)semi * 11500000u + (uint32_t)cut * 900000u + env_sweep;
                    uint32_t ph = x * freq + (uint32_t)g.synth.ch_ph[v] + speed;
                    int32_t fund = dub_sine_q64(ph);
                    int32_t wv;
                    int32_t env;
                    int32_t eff_cut;
                    int32_t dy, y;
                    uint16_t col = base_col;

                    main_env -= (main_env * dec_drop) >> 12;
                    if (main_env < 18)
                        main_env = 18;

                    if (p->dly_c > 4u && x >= (uint32_t)dly_step && ((int32_t)x % dly_step) == 0) {
                        int32_t tap_no = (int32_t)x / dly_step;
                        int32_t tap_peak = ((int32_t)p->dly_c * 250) / (100 + (tap_no - 1) * 42);
                        if (tap_peak > echo_env)
                            echo_env = tap_peak;
                    }
                    echo_env -= (echo_env * (dec_drop + 70)) >> 12;
                    if (echo_env < 0)
                        echo_env = 0;

                    env = main_env > echo_env ? main_env : echo_env;
                    if (echo_env > main_env && !bus_muted(1u))
                        col = C_HI;

                    /* TIMBRE: 0..40 warm sine, 41..80 FM bells, 81..127 jagged wavefold/grit */
                    if (p->timbre <= 40u) {
                        int32_t h2 = (dub_sine_q64(ph << 1) * (int32_t)p->timbre) / 50;
                        wv = fund + (h2 >> 1);
                    } else if (p->timbre <= 80u) {
                        int32_t m = (dub_sine_q64(ph << 1) * (int32_t)(p->timbre - 40u)) / 36;
                        wv = dub_sine_q64(ph + (uint32_t)(m << 16));
                    } else {
                        int32_t m = dub_sine_q64(ph << 1);
                        int32_t fold = dub_square((ph * 3u) + (uint32_t)((p->timbre - 80u) << 24));
                        wv = (dub_sine_q64(ph + (uint32_t)(m << 16)) + fold) >> 1;
                    }

                    /* ENVLPE + CUTOFF open harmonic bite and amplitude on attack + echoes */
                    eff_cut = cut + ((env * env_f) >> 9);
                    if (eff_cut > 127)
                        eff_cut = 127;
                    wv = ((fund * (127 - eff_cut)) + (wv * eff_cut)) / 127;
                    lp = fm_lfo_slew_step(lp, wv, smooth);
                    /* RESONA adds self-oscillating filter ringing (high harmonic ripple riding on the wave) */
                    if (res > 6) {
                        int32_t ring = (dub_sine_q64(ph * 5u + (uint32_t)(eff_cut << 24)) * res) / 110;
                        lp += ((wv - lp) * res) >> 5;
                        lp += ring;
                    }
                    if (lp > 32767)
                        lp = 32767;
                    else if (lp < -32767)
                        lp = -32767;

                    dy = (((lp * (2 + (eff_cut * 8) / 127)) / 32767) * env) >> 8;
                    if (lvl_c == 0)
                        dy = 0;
                    else if (lvl_c != 92)
                        dy = (dy * lvl_c) / 92;
                    if (dy > 10)
                        dy = 10;
                    else if (dy < -10)
                        dy = -10;
                    y = cy - dy;

                    /* REVERB casts a glowing diffusion cloud & trailing shimmer behind each chord voice */
                    if (rev_c > 6) {
                        int32_t r_env = 40 + (env * rev_c) / 160;
                        int32_t r_wv = dub_sine_q64(ph - 0x38000000u) + (dub_sine_q64((ph << 1) + 0x20000000u) >> 1);
                        int32_t r_dy = (((r_wv * (3 + (rev_c * 8) / 127)) / 32767) * r_env) >> 8;
                        int32_t ry;
                        if (r_dy > 11)
                            r_dy = 11;
                        else if (r_dy < -11)
                            r_dy = -11;
                        ry = cy - r_dy;
                        if (((x + v) & 1u) == 0u)
                            cv_line(xp, cy, xp, ry, rev_c > 72 ? C_DIM : C_LINE);
                        cv_pset(xp, ry, rev_c > 84 ? C_HI : C_DIM);
                    }
                    /* Body fill inside strong envelope / high decay regions */
                    if (env > 110 && (dy > 2 || dy < -2))
                        cv_line(xp, cy, xp, y, echo_env > main_env ? C_LINE : C_DIM);

                    /* RESONA highlights resonant ringing peaks & crests in C_ACC / C_WHITE */
                    if (!bus_muted(1u) && res > 24) {
                        int32_t pk_thresh = 9 - (res * 5) / 127;
                        if (dy >= pk_thresh || dy <= -pk_thresh) {
                            col = C_ACC;
                            if (res > 72 && y > 23 && y < 123)
                                cv_pset(xp, dy > 0 ? y - 1 : y + 1, C_WHITE);
                        }
                    }

                    if (py >= 0) {
                        cv_line(xp - 1, py, xp, y, col);
                        /* Thickness on loud attack & high timbre */
                        if (env > 140 && p->timbre > 50u && y + 1 <= cy + 10)
                            cv_pset(xp, y + 1, col);
                    }
                    py = y;
                }
            }
        }
        return;
    }
    if (g.view == VIEW_BASS) {
        int32_t cut = (int32_t)p->cutoff_b;
        int32_t dec = (int32_t)p->decay_b;
        int32_t wob = (int32_t)p->wobble + (int32_t)g.voice.macro_wob;
        int32_t drv = (int32_t)p->drive;
        int32_t sub_o = (int32_t)p->sub_oct;
        int32_t pmp = (int32_t)p->pump;
        int32_t dly = (int32_t)p->dly_b;
        int32_t rev_b = (int32_t)p->rev_b;
        int32_t lvl_b = (int32_t)p->lvl_b;
        int32_t pan_b = (int32_t)(p->pan[0] ? p->pan[0] : 64u);
        uint32_t bpm_b = p->bpm ? p->bpm : 120u;
        uint8_t smooth = cut < 105 ? (uint8_t)(122u - (uint32_t)(cut * 114) / 105u) : 0u;
        uint64_t speed = (uint64_t)(fm1_ms * ((bpm_b / 8u) + (uint32_t)wob / 8u)) << 18;
        uint64_t now64 = speed + (uint64_t)(uint32_t)g.synth.sub_ph;
        uint32_t x;
        int32_t prev = 0, py = -1, spy = -1;
        int32_t mid = 70;
        int32_t dec_drop = 700 - (dec * 640) / 127;
        int32_t env = 256;

        if (wob > 127)
            wob = 127;
        copy_str(s, BASS_NAMES[p->bass_model % BASS_N]);
        cv_text(88, 4, &FONT_S, s, ERA_COL[BASS_ERA[p->bass_model % BASS_N] % GENRE_ERA_N]);
        cv_line(8, mid, 231, mid, C_LINE);
        {
            int32_t pan_x = 8 + ((pan_b - 1) * 224) / 126;

            if (pan_x < 8)
                pan_x = 8;
            if (pan_x > 231)
                pan_x = 231;
            cv_line(pan_x, 148, pan_x, 156, C_DIM);
        }

        /* DELAY ghosts: 1..3 offset echo traces whose distance & height follow DELAY + TIME */
        if (dly > 8) {
            uint64_t d_off = (uint64_t)(1u + dub_dly_div_ix(p->dly_time) / 3u) << 29;
            int32_t g_amp = (16 + (dec * 30) / 127) * dly / 127;
            if (dly > 64)
                fm_lfo_draw_trace(0, dub_bass_trace_sample, now64 - (d_off << 1), smooth,
                                  g_amp >> 1, 8, 224, mid, 46, 20, 122, C_LINE);
            fm_lfo_draw_trace(0, dub_bass_trace_sample, now64 - d_off, smooth,
                              g_amp, 8, 224, mid, 46, 20, 122, C_DIM);
        }

        for (x = 0; x < 224u; x++) {
            int32_t xp = 8 + (int32_t)x;
            /* WOBBLE accelerates wave density across X */
            uint32_t step_ph = (uint32_t)(32000000u + (uint32_t)wob * 380000u);
            uint32_t ph = (uint32_t)now64 + x * step_ph;
            uint32_t cyc = (uint32_t)(now64 >> 32) + ((x * (uint32_t)(1 + wob / 32)) >> 6);
            int32_t raw = dub_bass_trace_sample(0, ph, cyc);
            int32_t w = fm_lfo_slew_step(prev, raw, smooth);
            int32_t pump_att = 256;
            int32_t amp, y, sy;
            uint16_t col = bus_muted(0u) ? C_DIM : (cut > 90 ? C_WHITE : ecol);

            prev = w;
            /* DECAY shapes envelope falloff from left to right */
            env -= (env * dec_drop) >> 13;
            if (env < 44)
                env = 44;
            /* PUMP ducks the front of the wave (sidechain dip on kick beat) */
            if (pmp > 8 && x < 72u) {
                int32_t dip = ((72 - (int32_t)x) * pmp * 3) / 72;
                if (dip > 220)
                    dip = 220;
                pump_att = 256 - dip;
            }
            amp = ((14 + (cut * 34) / 127) * env >> 8) * pump_att >> 8;
            if (lvl_b == 0)
                amp = 0;
            else if (lvl_b != 96)
                amp = (amp * lvl_b) / 96;
            y = mid - (w * amp) / 32767;
            if (y < 20)
                y = 20;
            if (y > 122)
                y = 122;

            /* SUB octave shadow wave underneath */
            if (sub_o > 8) {
                int32_t sw = dub_sine_q64(ph >> (sub_o > 80 ? 2 : 1));
                int32_t samp = (amp * (24 + sub_o)) >> 8;
                sy = mid - (sw * samp) / 32767;
                if (sy < 20)
                    sy = 20;
                if (sy > 122)
                    sy = 122;
                if ( (x & 1u) == 0u )
                    cv_line(xp, mid, xp, sy, C_LINE);
                if (spy >= 0)
                    cv_line(xp - 1, spy, xp, sy, C_DIM);
                spy = sy;
            }

            /* Vertical body shadow under main bass wave */
            if (!bus_muted(0u) && (x & 1u) == 0u)
                cv_line(xp, mid, xp, y, C_LINE);

            if (rev_b > 6 && lvl_b > 0) {
                int32_t r_wv = dub_sine_q64(ph - 0x28000000u);
                int32_t r_amp = (amp * rev_b) / 160;
                int32_t r_dy = (r_wv * r_amp) / 32767;
                int32_t ry = mid - r_dy;

                if (ry < 20)
                    ry = 20;
                if (ry > 122)
                    ry = 122;
                if ((x & 3u) == 0u)
                    cv_line(xp, mid, xp, ry, rev_b > 72 ? C_DIM : C_LINE);
            }

            /* DRIVE thickens stroke (1..3 px) and highlights saturated peaks in C_ACC */
            if (!bus_muted(0u) && drv > 40 && (w > 23000 || w < -23000))
                col = C_ACC;

            if (py >= 0) {
                cv_line(xp - 1, py, xp, y, col);
                if (drv > 24 && y + 1 <= 122)
                    cv_line(xp - 1, py + 1, xp, y + 1, col);
                if (drv > 84 && y - 1 >= 20)
                    cv_line(xp - 1, py - 1, xp, y - 1, C_HI);
            }
            py = y;
        }
        return;
    }
    copy_str(s, p->tank < 64u ? "SPR" : "TNK");
    cv_text(68, 4, &FONT_S, s, C_HI);
    copy_str(s, TAP_NAMES[p->tap % TAP_N]);
    cv_text(108, 4, &FONT_S, s, C_ACC);
    fm_time_fmt(s, p->dly_time, p->bpm ? p->bpm : 120u, fm1_ms);
    cv_text(176, 4, &FONT_S, s, C_WHITE);
    {
        /* Clean full-width ECHO tap bars + visible MIX opacity, TANK, DAMP, TAIL, BLEED, WOW, ISO and 2nd HEAD (TAP) */
        uint8_t dix = dub_dly_div_ix(p->dly_time);
        uint8_t hm = p->tap % TAP_N;
        uint32_t ntaps = 12u - ((uint32_t)dix * 7u) / (DUB_DLY_DIV_N - 2u);
        uint32_t t;
        int32_t span_w = 216;
        int32_t fb = (int32_t)p->feedback;
        int32_t mix_op = ((int32_t)p->dly_lvl * 256) / 127; /* 0..256 opacity for MIX */
        int32_t rev = ((int32_t)p->rev_mix * mix_op) >> 8;
        int32_t damp = (int32_t)p->rev_damp;
        int32_t tail = (int32_t)p->rev_tail;
        int32_t bleed = (int32_t)p->dly_rev;
        int32_t lf = (int32_t)p->loop_filt;
        int32_t wow = (int32_t)p->wow;
        int32_t iso = ((int32_t)p->isolator - 64) + ((int32_t)g.voice.macro_iso >> 1);
        int32_t iso_top = iso < 0 ? (28 + ((-iso) * 44) / 64) : 24;
        int32_t iso_bot = iso > 0 ? (118 - (iso * 44) / 63) : 118;
        int32_t h = (84 * (24 + ((int32_t)p->dly_lvl * 103) / 127)) / 127;
        int32_t base_y = 118;
        int32_t prev_tx = -1, prev_ty = -1;

        if (ntaps < 4u)
            ntaps = 4u;

        {
            int32_t tank_bot = iso_bot < base_y ? iso_bot : base_y;
            int32_t field_top = iso_top + 2;

            if (field_top < 24)
                field_top = 24;

            /* TANK architecture + REVERB/TAIL/DAMP hall field (behind echo taps) */
            if (p->tank > 0u || rev > 0) {
                int32_t rh_base = 14 + ((int32_t)p->tank * 28) / 127 + (rev * 22) / 127;
                uint32_t rx;
                int32_t coil_top = tank_bot - rh_base;
                int32_t coil_bot = tank_bot - 2;

                if (coil_top < field_top)
                    coil_top = field_top;
                if (coil_top < 64)
                    coil_top = 64;

                if (p->tank < 64u) {
                    /* SPR: spring coil between iso_top and tank_bot */
                    uint32_t step_s = 5u + (63u - p->tank) / 10u;
                    uint32_t scroll = (fm1_ms / (10u + p->tank / 8u)) & 15u;
                    int32_t coil_cy = tank_bot - 10 - (rev * 16) / 127;
                    int32_t coil_amp = 4 + ((int32_t)p->tank * 14) / 63;
                    int32_t spy = coil_cy;
                    int32_t spr_bright = 90 + ((int32_t)p->tank * 70) / 63;
                    uint16_t spr_col = color_scale_rgb(C_HI, spr_bright);
                    uint16_t spr_hi = color_scale_rgb(C_HI, spr_bright + 18);

                    if (coil_amp > rh_base / 2)
                        coil_amp = rh_base / 2;
                    if (coil_amp < 2)
                        coil_amp = 2;
                    if (coil_cy < coil_top + coil_amp)
                        coil_cy = coil_top + coil_amp;
                    if (coil_cy > coil_bot - coil_amp)
                        coil_cy = coil_bot - coil_amp;
                    spy = coil_cy;

                    for (rx = 8u; rx < 230u; rx += step_s) {
                        int32_t x_norm = ((int32_t)rx - 8) * 256 / 222;
                        int32_t tail_keep = 256 - ((x_norm * (140 - tail)) >> 7);
                        int32_t swing_amp, ph_s;
                        uint32_t coil_ix;
                        uint16_t seg_col;

                        if (tail_keep < 24)
                            tail_keep = 24;
                        swing_amp = (coil_amp * tail_keep * (160 - damp)) >> 13;
                        coil_ix = ((rx + scroll) / step_s) & 7u;
                        ph_s = (coil_ix & 1u) ? (coil_cy - (swing_amp >> 1))
                                              : (coil_cy + (swing_amp >> 1));
                        if (ph_s < coil_top)
                            ph_s = coil_top;
                        if (ph_s > coil_bot)
                            ph_s = coil_bot;
                        seg_col = (coil_ix & 3u) == 0u ? spr_hi : spr_col;
                        cv_line((int32_t)rx, spy, (int32_t)(rx + step_s), ph_s, seg_col);
                        spy = ph_s;
                    }
                } else if (p->tank > 0u) {
                    /* TNK: chamber pillars + ceiling */
                    int32_t tn = (int32_t)p->tank - 64;
                    uint32_t col_step = 6u + (uint32_t)(63 - tn) / 12u;
                    int32_t col_w = 2 + tn / 10;
                    int32_t ceil_y = tank_bot - rh_base - 4;
                    int32_t tnk_edge = 75 + (tn * 85) / 63;
                    uint16_t tnk_hi = color_scale_rgb(C_HI, tnk_edge);
                    uint16_t tnk_fill = color_scale_rgb(C_DIM, 150 + (tn * 40) / 63);

                    if (ceil_y < 28)
                        ceil_y = 28;
                    cv_line(8, ceil_y, 231, ceil_y, tnk_hi);

                    for (rx = 8u; rx + (uint32_t)col_w < 231u; rx += col_step) {
                        int32_t x_norm = ((int32_t)rx - 8) * 256 / 222;
                        int32_t tail_keep = 256 - ((x_norm * (140 - tail)) >> 7);
                        int32_t rh_x, ch, cap_h;
                        int32_t damp_sh = (160 - damp) >> 4;

                        if (tail_keep < 24)
                            tail_keep = 24;
                        rh_x = (rh_base * tail_keep * damp_sh) >> 12;
                        ch = rh_x + (tn * rh_base) / 126;
                        if (ch > tank_bot - ceil_y - 2)
                            ch = tank_bot - ceil_y - 2;
                        if (ch < 3)
                            ch = 3;
                        cv_rect((int32_t)rx, tank_bot - ch, col_w, ch, tnk_fill);
                        cv_rect((int32_t)rx, tank_bot - ch, col_w, 1, tnk_hi);
                        cap_h = 2;
                        if (ch > cap_h + 1)
                            cv_rect((int32_t)rx, tank_bot - ch, col_w, cap_h, tnk_hi);
                    }
                }

                /* REVERB decay roof + diffusion rays (TAIL length, DAMP colour/texture) */
                if (rev > 0) {
                    int32_t rev_span = tank_bot - field_top - 8;
                    int32_t rev_h0 = (rev * rev_span) / 145;
                    int32_t prev_roof_x = -1, prev_roof_y = -1;
                    int32_t ray_stride = 4 + (damp >> 4);

                    if (rev_span < 8)
                        rev_span = 8;
                    if (rev_h0 < 4)
                        rev_h0 = 4;

                    for (rx = 8u; rx <= 231u; rx += 2u) {
                        int32_t x_norm = ((int32_t)rx - 8) * 256 / 223;
                        int32_t tail_keep = 256 - ((x_norm * (140 - tail)) >> 7);
                        int32_t rev_h, roof_y, shimmer, ry;
                        uint16_t roof_col, ray_col;

                        if (tail_keep < 8)
                            tail_keep = 8;
                        rev_h = (rev_h0 * tail_keep) >> 8;
                        rev_h = (rev_h * (200 - damp)) >> 8;
                        if (rev_h < 2)
                            rev_h = 2;
                        roof_y = tank_bot - rev_h;
                        if (damp < 80)
                            shimmer = ((dub_sine_q64(((fm1_ms << 17) + ((int32_t)rx << 19))) >> 13)
                                       * (80 - damp)) / 96;
                        else
                            shimmer = 0;
                        roof_y -= shimmer;
                        if (roof_y < field_top)
                            roof_y = field_top;

                        roof_col = color_scale_rgb(damp < 48 ? C_WHITE : damp < 96 ? C_HI : C_GRAY,
                                                   damp < 48 ? (150 + (shimmer >> 1))
                                                             : damp < 96 ? (110 + rev / 3)
                                                                         : (72 + (127 - damp) / 2));
                        if (prev_roof_x >= 0)
                            cv_line(prev_roof_x, prev_roof_y, (int32_t)rx, roof_y, roof_col);
                        prev_roof_x = (int32_t)rx;
                        prev_roof_y = roof_y;

                        ray_col = color_scale_rgb(damp < 64 ? C_HI : C_GRAY,
                                                  56 + (rev * 90) / 127 - (damp >> 1));
                        for (ry = roof_y + ray_stride; ry < tank_bot - 1; ry += ray_stride) {
                            if ((((int32_t)rx + ry + (int32_t)(fm1_ms >> 5)) & (7 + (damp >> 3))) == 0)
                                cv_rect((int32_t)rx, ry, 1, 1 + (damp < 72 ? 1 : 0), ray_col);
                        }
                    }
                }
            }
        }

        /* Delay taps spanning full width + MIX opacity + FEEDBK decay + LOOP passband + BLEED halo */
        for (t = 0; dix > 0u && p->dly_lvl > 0u && t < ntaps; t++) {
            int32_t w_sin = dub_sine_q64(((fm1_ms * 4u) << 19) + (t * 0x28000000u));
            int32_t wow_dx = ((w_sin * wow) >> 15) * 5 / 127;
            int32_t wow_dh = ((dub_sine_q64(((fm1_ms * 3u) << 19) + (t * 0x38000000u)) * wow) >> 15) * 6 / 127;
            int32_t x = 8 + ((int32_t)t * span_w) / (int32_t)(ntaps - 1u) + wow_dx;
            int32_t bar_w = ntaps > 8u ? 6 : 8;
            int32_t cur_h = h + wow_dh < 4 ? 4 : (h + wow_dh > 92 ? 92 : h + wow_dh);
            int32_t top_trim = ((int32_t)t * ((127 - lf) * cur_h)) / (int32_t)(ntaps * 170u);
            int32_t bot_trim = ((int32_t)t * (lf * cur_h)) / (int32_t)(ntaps * 200u);
            int32_t y0 = (base_y - cur_h) + top_trim < iso_top ? iso_top : (base_y - cur_h) + top_trim;
            int32_t y1 = base_y - bot_trim > iso_bot ? iso_bot : base_y - bot_trim;
            uint16_t raw_col = t == 0u ? C_WHITE :
                               ((hm == 5u && (t & 1u)) ? C_ACC :
                                (t * (uint32_t)(127 - lf) > ntaps * 70u ? C_DIM : C_HI));
            uint16_t col = color_scale_rgb(raw_col, 36 + ((mix_op * 220) >> 8));
            uint16_t bg_col = color_scale_rgb(C_LINE, 48 + ((mix_op * 208) >> 8));

            if (x < 8)
                x = 8;
            if (x + bar_w > 231)
                x = 231 - bar_w;
            if (y1 - y0 < 3)
                y0 = y1 - 3;

            /* BLEED (`dly_rev`): reverb flag trailing right of each echo tap */
            if (bleed > 6) {
                int32_t bw = 2 + (bleed * (8 + (tail >> 4))) / 127;
                int32_t bh = y1 - y0;
                int32_t by0 = y1 - bh + ((damp * bh) >> 9);
                int32_t bbright = 96 + (bleed * 150) / 127;
                int32_t bi;

                if (bh < 4)
                    bh = 4;
                if (by0 < y0)
                    by0 = y0;
                if (y1 > by0 + 1 && x + bar_w + bw <= 232) {
                    for (bi = 0; bi < bw; bi++) {
                        int32_t fade = 256 - (bi * 220 / (bw > 1 ? bw : 1));
                        uint16_t bcol = color_scale_rgb(damp < 80 ? C_HI : C_DIM,
                                                        (bbright * fade) >> 8);

                        cv_rect(x + bar_w + bi, by0, 1, y1 - by0, bcol);
                    }
                }
            }

            cv_rect(x, base_y - cur_h, bar_w, cur_h, bg_col);
            cv_rect(x, y0, bar_w, y1 - y0, col);
            if (wow > 4) {
                int32_t tx = x + (bar_w >> 1);
                if (prev_tx >= 0)
                    cv_line(prev_tx, prev_ty, tx, y0,
                            color_scale_rgb(wow > 64 ? C_ACC : C_DIM, 48 + ((mix_op * 208) >> 8)));
                prev_tx = tx;
                prev_ty = y0;
            }
            /* Draw 2nd tape head (Tap B) between main taps for DUAL, GALLOP, TRIP, SPREAD, PING */
            if (hm != 0u && t + 1u < ntaps) {
                int32_t step_dx = span_w / (int32_t)(ntaps - 1u);
                int32_t bx = (hm == 1u || hm == 5u) ? (x + step_dx / 2) :
                             hm == 2u ? (x + (step_dx * 3) / 4) :
                             hm == 3u ? (x + (step_dx * 2) / 3) : (x + bar_w + 1);
                int32_t bh = (y1 - y0) * 3 / 4;
                uint16_t h2_col = color_scale_rgb(hm == 4u || hm == 5u ? C_ACC : C_DIM,
                                                  48 + ((mix_op * 208) >> 8));
                if (bh > 3 && bx + 3 <= 231)
                    cv_rect(bx, y1 - bh, 3, bh, h2_col);
            }
            h = (h * (28 + fb)) / 140;
            if (h > 92)
                h = 92;
            if (h < 4)
                h = 4;
        }

        if (iso < -2)
            cv_line(8, iso_top, 231, iso_top, C_ACC);
        else if (iso > 2)
            cv_line(8, iso_bot, 231, iso_bot, C_ACC);
    }
}

static void draw_view_lfo(void)
{
    uint8_t t = g.lfo_target % DUB_LFO_TGT_N;
    enum { DUB_SEQ_LFO_NAME_N = (uint32_t)(sizeof DUB_SEQ_LFO_SUF / sizeof DUB_SEQ_LFO_SUF[0]) };
    char seq_long[DUB_SEQ_LFO_NAME_N][24];
    const char *names[DUB_LFO_TGT_N];
    uint8_t i;

    for (i = 0; i < DUB_LFO_TGT_N; i++) {
        if (dub_is_seq_lfo_tgt(i)) {
            uint8_t six = dub_seq_lfo_ix(i);

            if (six >= DUB_SEQ_LFO_NAME_N)
                names[i] = "?";
            else {
                dub_fmt_lfo_seq_long(seq_long[six], i);
                names[i] = seq_long[six];
            }
        } else
            names[i] = LFO_TGT_LONG[i];
    }
    fm_lfo_slots_draw_view(g.lfo_slots, g.lfo_rt, t, DUB_LFO_TGT_N, names, C_ACC);
}

static void draw_screen(void)
{
    palette_set(2u);
    sync_voice_params();
#if defined(GENRE_BASS)
    bass_sanitize_nav();
    g.lfo_target = dub_lfo_tgt_unpack(g.lfo_target);
#endif
    draw_top_bar();
    cv_begin(240, 132, C_BLACK);
    if (g.view == VIEW_DESK)
#if defined(GENRE_BASS)
    {
        if (g.bank == BANK_GROOVE)
            bass_draw_view_groove();
        else
            draw_view_home();
    }
#else
    {
        if (g.sel_drm == 3u)
            gv_draw_lead();
        else
            draw_view_desk();
    }
#endif
    else if (g.view == VIEW_CHORD)
#if defined(GENRE_BASS)
        draw_view_env();
#else
    {
        if (g.bank == BANK_ENV)
            gv_draw_env();
        else
            draw_view_panel(seq_lane_name(5u));
    }
#endif
    else if (g.view == VIEW_BASS)
#if defined(GENRE_BASS)
        bass_draw_view_voice();
#else
    {
        if (g.bank == BANK_GROOVE)
            gv_draw_glo();
        else
            draw_view_panel(seq_lane_name(4u));
    }
#endif
    else if (g.view == VIEW_SPACE)
        draw_view_panel("SPACE");
    else if (g.view == VIEW_HOME)
        draw_view_home();
    else if (g.view == VIEW_LFO)
        draw_view_lfo();
    else
        draw_view_seq();
    draw_perf_toast();
    draw_plate_era_toast();
    cv_blit(0, 28);
    draw_knob_bar();
}

static int dub_seq_lane_step_on(uint8_t lane, uint8_t st)
{
    int base_on = g.view == VIEW_SEQ ? (int)((raw_step_gate_show(st) & (1u << lane)) != 0) :
                  (int)((step_gate(st) & (1u << lane)) != 0);
    int on = fm_seq_vary_hit(base_on, dub_eff_vary(lane), st, g.bar, lane, g.seq_rng);
    if (on && g.step_prob[lane][st] > 0u &&
        !fm_seq_cond_pass(g.step_prob[lane][st], st, g.bar, lane, 0))
        on = 0;
    if (on && !fm_seq_trig_pass(dub_eff_trig_raw(lane), st, g.bar, lane))
        on = 0;
    return on;
}

static void bank_led_set(uint8_t btn, uint8_t st)
{
    fm1_led_key(btn, st == KO_LED_ON);
    if (st == KO_LED_DIM)
        fm1_led_key_dim(btn, 1);
}

static void update_leds(uint32_t keys)
{
    uint32_t k;
#if defined(GENRE_BASS)
    static const uint8_t BBTN[6] = {BTN_ENV, BTN_SEL, BTN_GLO, BTN_FX, BTN_LFO, BTN_HOME};
    static const uint8_t BBNK[6] = {BANK_CHORD, BANK_BASS, BANK_GROOVE, BANK_FX, BANK_LFO, BANK_HOME};
    const uint32_t bank_btn_n = 6u;
#else
    static const uint8_t BBTN[5] = {BTN_ENV, BTN_GLO, BTN_SEL, BTN_FX, BTN_LFO};
    static const uint8_t BBNK[5] = {BANK_ENV, BANK_GROOVE, BANK_DRUM, BANK_FX, BANK_LFO};
    const uint32_t bank_btn_n = 5u;
#endif
    uint32_t bi;
    int pulse = ko_led_pulse(fm1_ms);

    int edit_held = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    int con_active = (g.view != VIEW_SEQ) && (edit_held || g.console);

    if (con_active) {
#if defined(GENRE_BASS)
        fm1_led_key(KEY_LED(KEY_WHITE[0]), dub_lane_audible(4u));
        for (k = 1; k < 16u; k++)
            fm1_led_key(KEY_LED(KEY_WHITE[k]), 0);
#else
        /* 6-voice Dub Console on white keys:
         * White 1..6  (0..5)  = Mute KICK, RIM, HAT, SIREN, BASS, CHORD (lit = audible)
         * White 7..12 (6..11) = Delay Throw KICK, RIM, HAT, SIREN, BASS, CHORD (lit while thrown)
         * White 13    (12)    = Tape Freeze
         * White 14..16(13..15)= Wobble sync 1..3 */
        for (k = 0; k < 6u; k++)
            fm1_led_key(KEY_LED(KEY_WHITE[k]), dub_lane_audible((uint8_t)k));
        for (k = 6; k < 12u; k++)
            fm1_led_key(KEY_LED(KEY_WHITE[k]), (g.voice.lane_throw_mask >> (k - 6u)) & 1u);
        fm1_led_key(KEY_LED(KEY_WHITE[12]), g.voice.freeze != 0u);
        for (k = 13; k < 16u; k++)
            fm1_led_key(KEY_LED(KEY_WHITE[k]), g.voice.wob_force == (uint8_t)(k - 12u));
        for (k = 0; k < 11u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), (keys >> KEY_BLACK[k]) & 1u);
#endif
    } else if (g.view == VIEW_SEQ) {
#if defined(GENRE_BASS)
        uint8_t led_base = bass_seq_edit_base();

        for (k = 0; k < 16u; k++) {
            int on = bass_seq_lane_step_on((uint8_t)(led_base + k));
            uint8_t cst = bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages,
                                                       g.step);

            fm1_led_key(KEY_LED(KEY_WHITE[k]), (led_base + k) == cst ? 1 :
                                                 ((g.playing && g.step == led_base + k) ? !on : on));
        }
        for (k = 0; k < 8u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), (uint8_t)(k == (g.seq_view_page % 8u)));
#else
        uint8_t lane = g.seq_lane % DUB_SEQ_LANES;
        uint32_t base = seq_view_base();
        for (k = 0; k < 16u; k++) {
            int on = dub_seq_lane_step_on(lane, (uint8_t)(base + k));
            fm1_led_key(KEY_LED(KEY_WHITE[k]), (g.playing && g.step == base + k) ? !on : on);
        }
        for (k = 0; k < 11u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), k < 6u && k == lane);
#endif
    } else {
        for (k = 0; k < 16u; k++)
            fm1_led_key(KEY_LED(KEY_WHITE[k]), (keys >> KEY_WHITE[k]) & 1u);
        for (k = 0; k < 11u; k++)
            fm1_led_key(KEY_LED(KEY_BLACK[k]), (keys >> KEY_BLACK[k]) & 1u);
    }

    if (g.view != VIEW_SEQ) {
#if !defined(GENRE_BASS)
        uint8_t ag = g.gesture_latch ? g.gesture_latch : g.gesture;
        fm1_led_key(KEY_LED(KEY_BLACK[7]), ag == GEST_FILL || ag == GEST_BREAK);
        fm1_led_key(KEY_LED(KEY_BLACK[8]), ag == GEST_SOLO_DRUM || ag == GEST_SOLO_BAND);
        fm1_led_key(KEY_LED(KEY_BLACK[9]), ag == GEST_SOLO_BASS || ag == GEST_SOLO_BAND);
        fm1_led_key(KEY_LED(KEY_BLACK[10]), ag == GEST_FLAM);
#endif
    }

    fm1_led_key(BTN_PLAY, (!g.seq_follow && (g.view == VIEW_SEQ || (fm1_in.buttons & (1u << BTN_SEQ)))) ?
                              (g.playing ? !pulse : pulse) : g.playing);
    fm1_led_key(BTN_ARP, g.cfg.p.arp_mode != 0u);
    fm1_led_key(BTN_REC, g.rec_arm);
    fm1_led_key(BTN_EDIT, edit_held || g.console || g.gesture_latch != GEST_NONE);
    {
        uint8_t dj_btn = dj_target_btn();

        if (dj_btn == BTN_HOME) {
            bank_led_set(BTN_HOME, ko_bank_led(1, 1, pulse));
        } else {
            fm1_led_key(BTN_HOME, g.view != VIEW_SEQ && g.bank == BANK_HOME);
        }
        if (dj_btn == BTN_SEQ) {
            bank_led_set(BTN_SEQ, ko_bank_led(1, 1, pulse));
        } else {
            fm1_led_key(BTN_SEQ, g.view == VIEW_SEQ);
        }
    }
    fm1_led_key(BTN_OCT_DOWN, g.voice.tape_oct < 0 || g.cfg.p.root < 48u);
    fm1_led_key(BTN_OCT_UP,   g.voice.tape_oct > 0 || g.cfg.p.root >= 60u);

    for (bi = 0; bi < bank_btn_n; bi++) {
#if defined(GENRE_BASS)
        uint8_t bnk = BBNK[bi];
#else
        uint8_t bnk = gv_led_bank(BBNK[bi]);
#endif
        int active = g.view != VIEW_SEQ && g.bank == bnk && g.view == bank_view(bnk);
        int p2 = g.page[bnk] != 0u;
        uint8_t dj_btn = dj_target_btn();

        if (g.dj.on && dj_btn == BBTN[bi]) {
            bank_led_set(BBTN[bi], ko_bank_led(1, 1, pulse));
            continue;
        }
        bank_led_set(BBTN[bi], ko_bank_led(active, p2, pulse));
    }
    if (dj_fx_flash_active) {
        uint32_t ph = (fm1_ms - dj_fx_flash_ms) / DJ_FX_FLASH_MS;

        if (ph < DJ_FX_FLASH_N)
            fm1_led_key(BTN_FX, (ph & 1u) == 0u);
        else
            dj_fx_flash_active = 0u;
    }
}

static uint8_t gesture_from_keys(uint32_t keys, uint8_t edit)
{
    uint32_t fill = 1u << KEY_BLACK[7];
    uint32_t dsolo = 1u << KEY_BLACK[8];
    uint32_t bsolo = 1u << KEY_BLACK[9];
    uint32_t flam = 1u << KEY_BLACK[10];
    uint8_t g0 = GEST_NONE;

    if ((keys & dsolo) && (keys & bsolo))
        g0 = GEST_SOLO_BAND;
    else if (keys & dsolo)
        g0 = GEST_SOLO_DRUM;
    else if (keys & bsolo)
        g0 = GEST_SOLO_BASS;
    else if (keys & fill) {
        g0 = g.cfg.p.density >= 100u ? GEST_BREAK : GEST_FILL;
        if (edit)
            g0 = g0 == GEST_FILL ? GEST_BREAK : GEST_FILL;
    } else if (keys & flam)
        g0 = GEST_FLAM;
    return g0;
}

static void dub_console_latch_end(void);

/* Sibling gesture key cancels latched solo/flam/break/fill (and HOME fill-build). */
static void gesture_cancel_siblings(uint8_t held_g)
{
    if (held_g == GEST_NONE)
        return;
    if (g.gesture_latch != GEST_NONE && g.gesture_latch != held_g) {
        g.gesture_latch = GEST_NONE;
        g.ui_dirty = 1u;
    }
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    if (house_fill_build_live() && held_g != GEST_FILL) {
        house_fill_build_restore();
    }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    if ((idm_stutter_edit_live() || g.idm_edit_latch) && held_g != GEST_FILL &&
        held_g != GEST_BREAK) {
        idm_stutter_edit_restore();
        g.idm_edit_latch = 0u;
    } else if ((g.fill_build || g.fill_build_snap || g.fill_drop) &&
               (held_g == GEST_FILL || held_g == GEST_BREAK) && held_g != g.fill_build_kind) {
        idm_stutter_edit_restore();
        g.idm_edit_latch = 0u;
    }
#endif
}

static void handle_gesture_keys(uint32_t keys, uint32_t rising, uint32_t released, uint8_t edit)
{
#if defined(GENRE_BASS)
    (void)keys;
    (void)rising;
    (void)released;
    (void)edit;
    return;
#endif
    uint32_t gbits[4] = {1u << KEY_BLACK[7], 1u << KEY_BLACK[8], 1u << KEY_BLACK[9], 1u << KEY_BLACK[10]};
    uint32_t gi;
    uint8_t held_g = gesture_from_keys(keys, edit);
    uint32_t bpm = g.voice.p.bpm ? g.voice.p.bpm : (g.cfg.p.bpm ? g.cfg.p.bpm : 120u);
    /* 1 bar (4 beats) in ms at current BPM (e.g. 2000 ms at 120 BPM); works even when paused! */
    uint32_t bar_ms = (240000u / (bpm < 40u ? 40u : bpm));

    if (g.console && (rising & (gbits[0] | gbits[1] | gbits[2] | gbits[3])))
        dub_console_latch_end();
    for (gi = 0; gi < 4u; gi++) {
        if (rising & gbits[gi]) {
            gesture_cancel_siblings(held_g);
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
            if (gi == 0u && g.view == VIEW_HOME && !edit)
                house_fill_build_start(held_g);
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            if (gi == 0u && g.view == VIEW_HOME && !edit)
                idm_stutter_start(held_g);
#endif
            if (g.gesture_latch != GEST_NONE && g.gesture_latch == held_g)
                g.gesture_latch = GEST_NONE;
            g.gest_hold[gi] = 0u;
            g.gest_ms[gi] = fm1_ms;
            g.ui_dirty = 1u;
        }
        if ((keys & gbits[gi]) && g.gest_ms[gi] != 0u && (fm1_ms - g.gest_ms[gi] >= bar_ms) && held_g != GEST_NONE) {
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
            if (house_fill_latch_block(gi))
                ;
            else
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            if (idm_stutter_latch_block(gi, edit))
                ;
            else
#endif
            if (g.gesture_latch != held_g) {
                g.gesture_latch = held_g;
                g.ui_dirty = 1u;
            }
        }
        if (released & gbits[gi]) {
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
            if (gi == 0u) {
                house_fill_build_release();
                if (g.view == VIEW_HOME)
                    g.gesture_latch = GEST_NONE;
            }
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
            if (gi == 0u) {
                idm_stutter_release();
                if (g.view == VIEW_HOME) {
                    g.gesture_latch = GEST_NONE;
                    g.idm_edit_latch = 0u;
                }
            }
#endif
            g.gest_hold[gi] = 0u;
            g.gest_ms[gi] = 0u;
            g.ui_dirty = 1u;
        }
    }
    if (held_g != GEST_NONE)
        g.gesture = held_g;
    else {
        g.gesture = g.gesture_latch;
        if (g.gesture == GEST_NONE)
            perf_toast_main_gesture_fade_out();
    }
    {
        /* Black 7 FILL/BREAK release → same kick-slam as DJ DROP */
        static uint8_t was_fb;
        uint8_t ag = gesture_active();
        uint8_t fb = (ag == GEST_FILL || ag == GEST_BREAK);

        if (was_fb && !fb) {
            g.kick_slam_bars = 2u;
            g.drop_emph_span = 2u;
        }
        was_fb = fb;
    }
}

#define DUB_EDIT_LATCH_MS 450u
#define DUB_STEP_LATCH_MS 400u

static uint32_t dub_console_key_mask(void)
{
    uint32_t m = 0u;
    uint8_t i;

    for (i = 0u; i < 16u; i++)
        m |= 1u << KEY_WHITE[i];
    return m;
}

static void dub_console_latch_end(void)
{
    if (!g.console)
        return;
    g.console = 0u;
    g.edit_long_arm = 0u;
    g.edit_long_cancel = 0u;
    g.edit_press_ms = 0u;
    g.edit_groove_arm_ms = 0u;
    g.ui_dirty = 1u;
}

static void edit_long_press_poll(uint32_t b, uint32_t rising, uint8_t edit_held)
{
#if defined(GENRE_BASS)
    static uint8_t bass_edit_long_lock_done;
#endif

    if (!edit_held || !g.edit_long_arm || g.edit_long_cancel) {
#if defined(GENRE_BASS)
        if (!edit_held)
            bass_edit_long_lock_done = 0u;
#endif
        return;
    }
    if (fm1_in.buttons & ~(1u << BTN_EDIT))
        g.edit_long_cancel = 1u;
    if (b & ~(1u << BTN_EDIT))
        g.edit_long_cancel = 1u;
#if defined(GENRE_BASS)
    if (g.view == VIEW_SEQ) {
        ko_longpress_t lp = {&g.edit_press_ms, &bass_edit_long_lock_done, NULL};

        if (ko_longpress_poll(&lp, fm1_ms, DUB_EDIT_LATCH_MS, 0)) {
            uint8_t st = bass_seq_cursor_step_runtime(g.seq_plock_step, g.seq_view_page, g.cfg.seq_pages,
                                                      g.step);

            bass_edit_lock_step(st);
            seq_flash_knob("LOCK");
            mark_dirty();
            g.ui_dirty = 1u;
        }
        return;
    }
#endif
    if (rising & ~dub_console_key_mask())
        g.edit_long_cancel = 1u;
}

static fm_dj_app_fx_t fm_dj_app_fx_io(void)
{
    fm_dj_app_fx_t io;

    io.press_ms = &g.fx_press_ms;
    io.long_used = &g.fx_long_used;
    io.long_cancel = &g.fx_long_cancel;
    io.dj = &g.dj;
    io.ui_dirty = &g.ui_dirty;
    return io;
}

static void fm_dj_app_bank_fx_short(void)
{
    bank_button(BANK_FX, 0);
}

/* Plate step for a DJ turn: n distance on the endless continuum, else one preset. */
#if defined(FM_PGEN_ON)
#define FM_DJ_PLATE_STEP(pgen_step) ((int32_t)(pgen_step))
#else
#define FM_DJ_PLATE_STEP(pgen_step) ((int32_t)1)
#endif

static void fm_dj_comply_wander(void)
{
#if defined(FM_PGEN_ON)
    dub_rng_step();
    g.dj_comply_ofs = (int8_t)((int32_t)((g.seq_rng >> 16) % (2u * FM_PGEN_DJ_WANDER + 1u)) - FM_PGEN_DJ_WANDER);
#endif
}

static void fm_dj_on_drop(fm_dj_t *dj, uint8_t r, uint8_t *kick_slam_bars)
{
    int32_t edir;
    uint8_t span;

    (void)dj;
    span = fm_dj_drop_emph_span_phrase(r);
    *kick_slam_bars = span;
    g.drop_emph_span = span;
    if (fm_dj_drop_do_plate) {
        fm_dj_drop_last_was_plate = 1u;
        cycle_plate(-FM_DJ_PLATE_STEP(FM_PGEN_DJ_DROP));
    } else {
        fm_dj_drop_last_was_plate = 0u;
        edir = (r == 2u) ? 1 : -1;
        cycle_era(edir);
        if (fm_era_is_artist(g.cfg.p.era))
            cycle_era(edir);
    }
    fm_dj_drop_do_plate ^= 1u;
}

/* CUEING phrase end: next plate + song groove cue (no XF/EV morph arm). */
static void fm_dj_trans_cue_plate(void)
{
    dub_params_t *p = &g.cfg.p;
    uint8_t prev_era = p->era;
    int32_t v;

    if (fm_era_is_artist(p->era)) {
        if (!fm_artist_plate_count())
            return;
#if defined(FM_PGEN_ON)
        genre_pgen_off();
#endif
        if (!(g.user_plate_mask & (1ull << (p->plate % GENRE_CFG_USER_PLATE_N))))
            p->plate = fm_artist_first_plate();
        else
            p->plate = fm_artist_cycle_plate(p->plate, 1);
        genre_plate_preset_apply(p, (uint8_t)FM_ERA_ARTIST, p->plate, p->bass_model, p->drum_kit, 0u);
    } else {
#if defined(FM_PGEN_ON)
        (void)v;
        p->plate = fm_pgen_goto(prev_era, g.cfg.pgen.n + FM_PGEN_DJ_CUE);
#else
        v = (int32_t)p->plate + 1;
        while (v < 0)
            v += GENRE_PRESET_N;
        p->plate = (uint8_t)(v % GENRE_PRESET_N);
#endif
        genre_plate_preset_apply(p, prev_era, p->plate, p->bass_model, p->drum_kit, 0u);
    }
    dub_plate_seq_reload_song_lanes(p->plate);
    dub_sel_sync_saved_lvls();
    dub_load_plate_song_groove(p->plate);
    dub_era_lfo_apply(p->era, p->plate);
    mark_dirty();
}

static void fm_dj_on_trans(fm_dj_t *dj, uint8_t trans, uint8_t r, uint8_t *kick_slam_bars)
{
    dub_dj_gest_restore();
    fm_dj_comply_wander();
    if (trans == FM_DJ_TRANS_DROP || trans == FM_DJ_TRANS_FILTER_SLAM) {
        fm_dj_on_drop(dj, r, kick_slam_bars);
        return;
    }
    if (kick_slam_bars)
        *kick_slam_bars = 0u;
    g.kick_slam_bars = 0u;
    g.drop_emph_span = 0u;
    if (trans == FM_DJ_TRANS_CROSSFADE || trans == FM_DJ_TRANS_ECHO_OUT) {
        if (fm_dj_trans_era_step_picked(trans, fm_dj_xf_seq)) {
            int32_t edir = fm_dj_trans_era_dir(r);

            cycle_era(edir);
            if (fm_era_is_artist(g.cfg.p.era))
                cycle_era(edir);
        } else
            cycle_plate(fm_dj_trans_plate_dir(trans) *
                        FM_DJ_PLATE_STEP(trans == FM_DJ_TRANS_ECHO_OUT ? FM_PGEN_DJ_ECHO : FM_PGEN_DJ_XF));
        fm_dj_xf_seq++;
    } else if (trans == FM_DJ_TRANS_CUEING)
        fm_dj_trans_cue_plate();
}

static void seq_long_press_poll(uint32_t b, uint32_t rising)
{
    ko_longpress_t lp = {&g.seq_press_ms, &g.seq_long_used, &g.seq_long_cancel};

    if (!(fm1_in.buttons & (1u << BTN_SEQ)))
        return;
    if (!g.seq_press_ms)
        return;
    if (ko_longpress_poll(&lp, fm1_ms, 500u,
                          (fm1_in.buttons & ~(1u << BTN_SEQ)) || (b & ~(1u << BTN_SEQ)) || rising)) {
        g.seq_follow = g.seq_follow ? 0u : 1u;
        g.seq_mod_used = 1u;
        g.ui_dirty = 1u;
    }
}

static void seq_white_plock_latch_poll(uint32_t keys, uint32_t rising)
{
    ko_longpress_t lp = {&g.white_press_ms, &g.white_long_used, &g.white_long_cancel};
    uint32_t bit;

    if (g.view != VIEW_SEQ || g.white_press_k >= 16u)
        return;
    bit = 1u << KEY_WHITE[g.white_press_k];
    if (!(keys & bit))
        return;
    if (ko_longpress_poll(&lp, fm1_ms, DUB_STEP_LATCH_MS, (rising & ~bit) != 0u)) {
#if defined(GENRE_BASS)
        uint8_t base = bass_seq_edit_base();
        uint8_t st = (uint8_t)(base + g.white_press_k);
        uint8_t bit = (uint8_t)(1u << 4u);

        if (g.seq_plock_latch && g.seq_plock_step == st) {
            g.seq_plock_latch = 0u;
            g.seq_plock_hold = 0u;
        } else {
            if (!(g.cfg.seq[st].gate & bit))
                bass_seq_toggle_step(st);
#else
        uint8_t base = (uint8_t)seq_view_base();
        uint8_t st = (uint8_t)(base + g.white_press_k);
        uint8_t ln = g.seq_lane % DUB_SEQ_LANES;
        uint8_t ln_bit = (uint8_t)(1u << ln);

        if (g.seq_plock_latch && g.seq_plock_step == st) {
            g.seq_plock_latch = 0u;
            g.seq_plock_hold = 0u;
        } else {
            if (!(raw_step_gate(st) & ln_bit))
                seq_toggle_step(st, ln);
#endif
            g.seq_plock_latch = 1u;
            g.seq_plock_hold = 1u;
            g.seq_plock_bank = 0u;
            g.seq_plock_step = st;
            g.seq_plock_knob = 0u;
        }
        g.ui_dirty = 1u;
    }
}

static void handle_keys(uint32_t keys, uint32_t rising, uint32_t released, uint8_t edit)
{
    uint32_t k;
    uint8_t seq_held = (fm1_in.buttons & (1u << BTN_SEQ)) != 0u;
    uint8_t act_arp = dub_active_arp_mode();

    if (act_arp == 5u) {
        uint8_t any_white = 0u;

        for (k = 0u; k < 16u; k++) {
            if (fm1_in.notes & (1u << KEY_WHITE[k])) {
                any_white = 1u;
                break;
            }
        }
        if (!any_white) {
            /* Drop any queued start so release cannot re-arm a loop after let-go */
            g.loop_pending = 0u;
            if (g.loop_active == 1u)
                g.loop_end_pending = 1u;
        }
    }

    /* Step-grid editing is strictly in VIEW_SEQ only! */
    if (g.view == VIEW_SEQ) {
#if defined(GENRE_BASS)
        /* SEQ grid (bars/steps) before chromatic play — no live voice from white/black in SEQ. */
        uint8_t ln_seq = 4u;
        uint8_t white_cnt = 0u;
        uint8_t base = bass_seq_edit_base();
#else
        handle_gesture_keys(keys, rising, released, edit);
        uint8_t ln_seq = g.seq_lane % DUB_SEQ_LANES;
        uint8_t white_cnt = 0u;
        uint8_t base = (uint8_t)seq_view_base();
#endif

        for (k = 0; k < 16u; k++) {
            if (keys & (1u << KEY_WHITE[k]))
                white_cnt++;
        }

        g.seq_plock_hold = g.seq_plock_latch;
        for (k = 0; k < 16u; k++) {
            uint32_t bit = 1u << KEY_WHITE[k];

            if (rising & bit) {
                if (!g.seq_plock_latch) {
                    g.seq_plock_step = (uint8_t)(base + k);
                    g.seq_plock_knob = 0u;
                }
#if defined(GENRE_BASS)
                if (g.rec_arm) {
                    uint8_t stp = (uint8_t)(base + k);
                    uint8_t nn = bass_kbd_to_midi(KEY_WHITE[k]);

                    g.cfg.seq[stp].note = nn;
                    g.cfg.seq[stp].gate |= (uint8_t)(1u << 4);
                    g.cfg.bass.step_x[stp].gate = 4u;
                    bass_edit_lock_step(stp);
                    mark_dirty();
                }
#endif
                if (white_cnt >= 2u &&
#if defined(GENRE_BASS)
                    1
#else
                    g.seq_view_on[base / FM_SEQ_PAGE_STEPS]
#endif
                   ) {
                    uint8_t s_min = 15u;
                    uint8_t s_max = 0u;
                    uint32_t kk;

                    for (kk = 0; kk < 16u; kk++) {
                        if (keys & (1u << KEY_WHITE[kk])) {
                            if (kk < s_min)
                                s_min = (uint8_t)kk;
                            if (kk > s_max)
                                s_max = (uint8_t)kk;
                        }
                    }
                    g.loop_active = 2u;
                    g.loop_start_step = (uint8_t)(base + s_min);
                    g.loop_len = (uint8_t)(s_max - s_min + 1u);
                    g.white_press_k = 255u;
                    g.white_long_used = 1u;
                } else {
                    g.white_press_k = (uint8_t)k;
                    g.white_press_ms = fm1_ms;
                    g.white_long_used = 0u;
                    g.white_long_cancel = 0u;
                }
            }
            if (released & bit) {
                if (g.white_press_k == k && !g.white_long_used && !g.white_long_cancel) {
                    if (fm1_ms - g.white_press_ms >= DUB_STEP_LATCH_MS) {
                        uint8_t st = (uint8_t)(base + k);
                        uint8_t bit4 = (uint8_t)(1u << 4u);

#if defined(GENRE_BASS)
                        if (!(g.cfg.seq[st].gate & bit4))
                            bass_seq_toggle_step(st);
                        if (!g.seq_plock_latch) {
                            g.seq_plock_latch = 1u;
                            g.seq_plock_hold = 1u;
                            g.seq_plock_bank = 0u;
                            g.seq_plock_step = st;
                            g.seq_plock_knob = 0u;
                            g.white_long_used = 1u;
                            g.ui_dirty = 1u;
                        }
#else
                        (void)st;
                        (void)bit4;
#endif
                    } else if (g.seq_plock_latch) {
                        g.seq_plock_latch = 0u;
                        g.seq_plock_hold = 0u;
                        g.ui_dirty = 1u;
                    } else {
                        seq_toggle_step((uint8_t)(base + k), ln_seq);
#if !defined(GENRE_BASS)
                        if (!g.playing && (step_gate((uint8_t)(base + k)) & (1u << ln_seq)))
                            dub_preview_lane(ln_seq, (uint8_t)(base + k));
#endif
                    }
                }
                if (g.white_press_k == k) {
                    g.white_press_k = 255u;
                    g.white_press_ms = 0u;
                    g.white_long_used = 0u;
                    g.white_long_cancel = 0u;
                }
            }
        }
        if (white_cnt < 2u && g.loop_active == 2u) {
            g.loop_active = 0u;
            g.step = g.slip_step;
        }
#if defined(GENRE_BASS)
        for (k = 0; k < 8u; k++) {
            uint32_t bit = 1u << KEY_BLACK[k];

            if (rising & bit) {
                if (edit && k == 0u) {
                    dub_lane_toggle_mute(4u);
                    g.edit_used = 1u;
                    mark_dirty();
                } else if (!edit && !seq_held)
                    bass_seq_select_bar((int32_t)k);
            }
        }
#else
        for (k = 0; k < 6u; k++) {
            uint32_t bit = 1u << KEY_BLACK[k];
            if (rising & bit) {
                if (edit) {
                    dub_lane_toggle_mute((uint8_t)k);
                    g.edit_used = 1u;
                } else if (seq_held) {
                    dub_lane_toggle_solo((uint8_t)k);
                    g.seq_mod_used = 1u;
                } else {
                    seq_lane_sel_set((uint8_t)k);
                    if (k < 4u)
                        g.sel_drm = (uint8_t)k;
                }
                mark_dirty();
            }
        }
#endif
#if !defined(GENRE_BASS)
        if (rising & (1u << KEY_BLACK[8])) {
            if (edit)
                dub_invert_all_lane_mutes();
            else if (seq_held) {
                dub_lane_toggle_solo(ln_seq);
                g.seq_mod_used = 1u;
            } else if (dub_any_lane_mute_or_solo() || g.voice.mute != 0u)
                dub_unmute_all_lanes();
            else
                dub_lane_toggle_mute(ln_seq);
            mark_dirty();
        }
        if (rising & (1u << KEY_BLACK[9])) {
            if (edit) {
                uint32_t li;
                for (li = 0; li < DUB_SEQ_LANES; li++)
                    seq_reset_lane_song((uint8_t)li);
            } else
                seq_reset_lane_song(ln_seq);
            mark_dirty();
        }
        if (rising & (1u << KEY_BLACK[10])) {
            if (edit) {
                uint32_t li;
                for (li = 0; li < DUB_SEQ_LANES; li++)
                    seq_empty_lane((uint8_t)li);
            } else
                seq_empty_lane(ln_seq);
            mark_dirty();
        }
#endif
        return;
    }

    if (g.view == VIEW_LFO && dub_seq_lfo_bind_ix(g.lfo_target) != 255u) {
        uint8_t ix = dub_seq_lfo_bind_ix(g.lfo_target);

        for (k = 0; k < 6u; k++) {
            if (rising & (1u << KEY_BLACK[k])) {
                g.seq_lfo_lane[ix] = (uint8_t)k;
                seq_lane_sel_set((uint8_t)k);
                if (k < 4u)
                    g.sel_drm = (uint8_t)k;
                mark_dirty();
            }
        }
    }

#if !defined(GENRE_BASS)
    /* Outside VIEW_SEQ: Black 1..6 always map 1:1 to 0=KICK, 1=RIM, 2=HAT, 3=SIREN, 4=BASS, 5=CHORD */
    handle_gesture_keys(keys, rising, released, edit);
    for (k = 0; k < 6u; k++) {
        uint32_t bit = 1u << KEY_BLACK[k];
        uint8_t ln_k = (uint8_t)k;

        if (rising & bit) {
            if (edit) {
                /* EDIT + Black 1..6 = Mute toggle */
                dub_lane_toggle_mute(ln_k);
                g.edit_used = 1u;
                mark_dirty();
            } else if (seq_held) {
                /* SEQ + Black 1..6 = Solo toggle */
                dub_lane_toggle_solo(ln_k);
                g.seq_mod_used = 1u;
                mark_dirty();
            } else {
                seq_lane_sel_set(ln_k);
                if (ln_k < 4u)
                    g.sel_drm = ln_k;
                gv_from_lane(ln_k);
                if (!(g.playing && g.view == VIEW_DESK))
                    dub_preview_lane(ln_k, g.step);
                dub_rec_lane_hit(ln_k, -1);
                g.ui_dirty = 1u;
            }
        }
        if (ln_k == 3u && !edit && !seq_held && !(g.playing && g.view == VIEW_DESK)) {
            if (keys & bit)
                dub_siren_set(&g.synth, 1u);
            else if (released & bit)
                dub_siren_set(&g.synth, 0u);
        }
    }
    if ((g.view == VIEW_HOME || g.bank == BANK_HOME) && !edit && !seq_held &&
        (rising & (1u << KEY_BLACK[6]))) {
        dub_home_randomize_morph();
        g.ui_dirty = 1u;
    } else if (rising & (1u << KEY_BLACK[6]))
        dub_spring_crash(&g.synth);
#endif /* !GENRE_BASS */

    /* 6-voice Dub Console when holding EDIT or when Console mode is latched via double-click EDIT:
     * White 1..6  (0..5)  = Mute KICK, RIM, HAT, SIREN, BASS, CHORD
     * White 7..12 (6..11) = Momentary Delay-Throw KICK, RIM, HAT, SIREN, BASS, CHORD
     * White 13    (12)    = Tape Freeze toggle
     * White 14..16(13..15)= Wobble sync 1..3 */
    if (edit || g.console) {
        uint32_t wf_mask = (1u << KEY_WHITE[13]) | (1u << KEY_WHITE[14]) | (1u << KEY_WHITE[15]);
        for (k = 0; k < 6u; k++) {
            if (rising & (1u << KEY_WHITE[k])) {
                dub_lane_toggle_mute((uint8_t)k);
                g.edit_used = 1u;
                mark_dirty();
            }
        }
        /* White 7..12 (6..11) = Momentary Delay-Throw directly from held state `keys` so releasing ALWAYS clears */
        {
            uint8_t tm = 0u;
            for (k = 6; k < 12u; k++) {
                if (keys & (1u << KEY_WHITE[k]))
                    tm |= (uint8_t)(1u << (k - 6u));
            }
            if (tm != g.voice.lane_throw_mask) {
                if (tm)
                    g.edit_used = 1u;
                g.voice.lane_throw_mask = tm;
                g.ui_dirty = 1u;
            }
        }
        if (rising & (1u << KEY_WHITE[12])) {
            g.voice.freeze ^= 1u;
            g.edit_used = 1u;
            g.ui_dirty = 1u;
        }
        for (k = 13; k < 16u; k++) {
            if (keys & (1u << KEY_WHITE[k])) {
                g.voice.wob_force = (uint8_t)(k - 12u);
                g.edit_used = 1u;
                g.ui_dirty = 1u;
            }
        }
        if (!(keys & wf_mask) && g.voice.wob_force) {
            g.voice.wob_force = 0;
            g.ui_dirty = 1u;
        }
        return;
    }
    if (g.voice.lane_throw_mask) {
        g.voice.lane_throw_mask = 0u;
        g.ui_dirty = 1u;
    }

#if defined(GENRE_BASS)
    if (act_arp == 5u && rising) {
        arp_loop_mode_update();
        g.ui_dirty = 1u;
        if (white_Play_active())
            dub_loop_toast_show(g.loop_pending_len);
    }
    bass_handle_chromatic_keys(rising, released);
#else
    for (k = 0; k < 16u; k++) {
        uint32_t bit = 1u << KEY_WHITE[k];
        uint8_t ln;
        int32_t nn;

        if (!(rising & bit))
            continue;
        if (act_arp == 5u) {
            arp_loop_mode_update();
            g.ui_dirty = 1u;
            if (white_Play_active())
                dub_loop_toast_show(g.loop_pending_len);
            continue;
        }
        if (k < 8u) {
            /* Left 8 white keys (0..7): play 8 ascending scale degrees on BASS (lane 4) */
            ln = 4u;
            nn = (int32_t)scale_deg_note(g.voice.p.root, g.voice.p.scale, (uint8_t)k);
            dub_trig_sub(&g.synth, (uint8_t)nn, 100, g.voice.p.atk_b);
            dub_rec_lane_hit(ln, nn);
            if (white_Play_active())
                dub_key_toast_show(ln, (uint8_t)nn);
        } else {
            /* Right 8 white keys (8..15): play 8 ascending scale degrees on CHORD (lane 5) */
            ln = 5u;
            nn = (int32_t)scale_deg_note(g.voice.p.root, g.voice.p.scale, (uint8_t)(k - 8u));
            dub_trig_chord(&g.synth, (uint8_t)nn, 100, g.voice.p.atk_c, g.voice.p.glide_c, g.voice.p.voicing);
            dub_rec_lane_hit(ln, nn);
            if (white_Play_active())
                dub_key_toast_show(ln, (uint8_t)nn);
        }
        mark_dirty();
    }
#endif
}

static uint32_t swing_samples(void)
{
    uint32_t amt = g.voice.p.swing ? g.voice.p.swing : g.cfg.p.swing;
    uint32_t sw;

    if (amt < 50u)
        amt = 50u;
    if (amt > 75u)
        amt = 75u;
    sw = (g.samples_per_step * (amt - 50u)) / 50u;
    if (sw > g.samples_per_step / 2u)
        sw = g.samples_per_step / 2u;
    return sw;
}

static void app_audio(int32_t *out, uint32_t n)
{
    uint32_t i;

#if defined(GENRE_BASS)
    g.bass_rt.smp_clock += n;
#endif
    dub_morph_clock(n);
    dub_cue_clock(n);
    dub_macro_morph_clock(n);
    {
        uint32_t lfo_bpm = g.cfg.p.bpm ? g.cfg.p.bpm : 120u;

        dub_dj_lfo_sync();
        fm_lfo_slots_sanitize_rates(g.lfo_slots_eff, DUB_LFO_TGT_N, lfo_bpm);
        fm_lfo_slots_step_tr(g.lfo_slots_eff, g.lfo_rt, DUB_LFO_TGT_N, g.lfo_target % DUB_LFO_TGT_N, lfo_bpm,
                             n, DUB_FS, g.playing ? 1u : 0u);
    }
    sync_voice_params();
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    idm_stutter_sync_runtime();
#endif
    for (i = 0; i < n; i++) {
#if defined(GENRE_BASS)
        bass_seq_gate_sample();
        bass_ratchet_sample();
        bass_micro_sample();
#endif
#ifdef FM_GENRE_STEP_MICRO
        snowcrash_micro_tick();
#endif
        if (g.playing && !g.ext_clock) {
            uint32_t sw = swing_samples();
            uint32_t need = g.samples_per_step;

            if (sw > need / 2u)
                sw = need / 2u;
            if (g.step & 1u)
                need += sw;
            else if (need > sw + 32u)
                need -= sw;
#if defined(GENRE_BASS)
            need += g.bass_rt.spp_extra;
#endif
            dub_lane_clock_sample();
            g.sample_acc++;
            while (g.sample_acc >= need) {
#if defined(GENRE_BASS)
                uint8_t fired = g.step;
#endif
                g.sample_acc -= need;
                seq_tick();
#if defined(GENRE_BASS)
                bass_clock_tx_tick(fired);
                bass_spp_frac_step(&g.bass_rt);
#else
                midi_clock_tx();
#endif
                sw = swing_samples();
                need = g.samples_per_step;
                if (sw > need / 2u)
                    sw = need / 2u;
                if (g.step & 1u)
                    need += sw;
                else if (need > sw + 32u)
                    need -= sw;
#if defined(GENRE_BASS)
                need += g.bass_rt.spp_extra;
#endif
            }
#if defined(GENRE_BASS)
            bass_clock_tx_sample();
#endif
        }
        out[i * 2u] = 0;
        out[i * 2u + 1u] = 0;
    }
    {
        uint8_t mc = g.voice.macro_chord;
        uint8_t mf = g.voice.macro_fb;
        uint8_t mw = g.voice.macro_wob;
        uint8_t mi = g.voice.macro_iso;

#if defined(DINGHY_HOST) && defined(GENRE_HEAVY_PSYCH)
        {
            const char *lead = getenv("FM1_HVYPSY_LEAD_ONLY");

            if (lead && lead[0])
                g.voice.mute = 7u;
        }
#endif
        dub_synth_render(&g.synth, &g.voice, out, n);
#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
        idm_stutter_process_buffer(out, n);
#endif
        (void)mc;
        (void)mf;
        (void)mw;
        (void)mi;
    }
}

#if defined(DINGHY_HOST)
#include <stdlib.h>
#include <stdio.h>

static void host_seq_chain_selftest_run(void)
{
    const char *e = getenv("FM1_SEQ_CHAIN_TEST");
    uint8_t save_on[FM_SEQ_MAX_PAGES];
    uint8_t save_cyc[FM_SEQ_MAX_PAGES];
    uint8_t save_reps[FM_SEQ_MAX_PAGES];
    uint8_t save_rep_left;
    uint8_t save_bar;
    uint32_t i;
    uint8_t st;
    uint8_t only_v = 3u;

    if (!e || !e[0])
        return;
    memcpy(save_on, g.seq_view_on, sizeof save_on);
    memcpy(save_cyc, g.seq_view_cyc, sizeof save_cyc);
    memcpy(save_reps, g.seq_view_reps, sizeof save_reps);
    save_rep_left = g.seq_view_rep_left;
    save_bar = g.bar;
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
        g.seq_view_on[i] = 0u;
        g.seq_view_cyc[i] = FM_SEQ_VIEW_CYC_ALL;
        g.seq_view_reps[i] = 1u;
    }
    g.seq_view_on[only_v] = 1u;
    g.seq_view_rep_left = 1u;
    g.bar = 0u;
    st = (uint8_t)(only_v * FM_SEQ_PAGE_STEPS + (FM_SEQ_PAGE_STEPS - 1u));
    if (seq_chain_next(st) != only_v * FM_SEQ_PAGE_STEPS) {
        fprintf(stderr, "SEQ_CHAIN: single view wrap failed at page end\n");
        exit(1);
    }
    st = (uint8_t)(only_v * FM_SEQ_PAGE_STEPS);
    for (i = 0u; i < FM_SEQ_PAGE_STEPS; i++)
        st = seq_chain_next(st);
    if (st != only_v * FM_SEQ_PAGE_STEPS) {
        fprintf(stderr, "SEQ_CHAIN: single view did not wrap (st=%u)\n", (unsigned)st);
        exit(1);
    }
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
        g.seq_view_on[i] = (uint8_t)(i == 0u || i == 2u);
        g.seq_view_cyc[i] = FM_SEQ_VIEW_CYC_ALL;
        g.seq_view_reps[i] = 1u;
    }
    g.seq_view_rep_left = 1u;
    st = (uint8_t)(FM_SEQ_PAGE_STEPS - 1u);
    if (seq_chain_next(st) != 2u * FM_SEQ_PAGE_STEPS) {
        fprintf(stderr, "SEQ_CHAIN: skip off middle view failed (end v0)\n");
        exit(1);
    }
    st = (uint8_t)FM_SEQ_PAGE_STEPS;
    if (seq_chain_next(st) != 2u * FM_SEQ_PAGE_STEPS) {
        fprintf(stderr, "SEQ_CHAIN: skip off middle view failed (inside v1)\n");
        exit(1);
    }
    {
        uint8_t save_loop = g.loop_active;
        uint8_t save_loop_start = g.loop_start_step;
        uint8_t save_loop_len = g.loop_len;
        uint8_t save_step = g.step;

        g.loop_active = 1u;
        g.loop_start_step = 0u;
        g.loop_len = (uint8_t)FM_SEQ_PAGE_STEPS;
        g.step = (uint8_t)(FM_SEQ_PAGE_STEPS - 1u);
        seq_advance_step(g.step);
        if (g.step != 0u) {
            fprintf(stderr, "SEQ_CHAIN: loop wrap skip failed (st=%u)\n", (unsigned)g.step);
            exit(1);
        }
        g.loop_active = save_loop;
        g.loop_start_step = save_loop_start;
        g.loop_len = save_loop_len;
        g.step = save_step;
    }
    memcpy(g.seq_view_on, save_on, sizeof save_on);
    memcpy(g.seq_view_cyc, save_cyc, sizeof save_cyc);
    memcpy(g.seq_view_reps, save_reps, sizeof save_reps);
    g.seq_view_rep_left = save_rep_left;
    g.bar = save_bar;
    printf("SEQ_CHAIN single-view wrap ok\n");
    printf("SEQ_CHAIN skip-off-view ok\n");
    exit(0);
}

static void host_seq_oxi_shot_prep(void)
{
    const char *e = getenv("FM1_SEQ_OXI_SHOT");
    uint32_t st;
    static const uint8_t HAT_ST[7] = {0u, 2u, 4u, 8u, 10u, 12u, 14u};

    if (!e || !e[0])
        return;
    nav_view(VIEW_SEQ);
    g.seq_page = 1u;
    g.seq_lane = 2u;
    g.lane_vel[2] = -13;
    g.lane_trig[2] = -23;
    g.lane_gate[2] = 30;
    g.lane_tune[2] = 20;
    seq_materialize_all();
    for (st = 0; st < 7u; st++)
        g.cfg.seq[HAT_ST[st]].gate |= (uint8_t)(1u << 2);
}
#else
static void host_seq_chain_selftest_run(void) {}
static void host_seq_oxi_shot_prep(void) {}
#endif

#if defined(DINGHY_HOST) && defined(GENRE_HEAVY_PSYCH)
static void host_hvypsy_lead_only_prep(void)
{
    const char *lead = getenv("FM1_HVYPSY_LEAD_ONLY");
    const char *chord = getenv("FM1_HVYPSY_CHORD_ONLY");
    const char *trax_env = getenv("FM1_HVYPSY_TRAX");
    uint8_t pl = 0u;

    if (trax_env && trax_env[0])
        pl = genre_clamp_preset((uint8_t)atoi(trax_env));
    if ((lead && lead[0]) || (chord && chord[0]) || (trax_env && trax_env[0])) {
        g.cfg.p.plate = pl;
        genre_preset_rebuild(&g.cfg.p, genre_era_of_preset(pl), pl);
        g.cfg.p.bass_model = dub_bass_model_for_era(g.cfg.p.era);
        g.cfg.p.drum_kit = dub_drum_kit_for_era(g.cfg.p.era);
        dub_plate_seq_preset(pl);
        dub_era_lfo_apply(g.cfg.p.era, pl);
        dub_era_home_macros_apply_era(g.cfg.p.era, 0u);
        dub_sel_sync_saved_lvls();
        transport_set_bpm();
        sync_voice_params();
    }
    if (lead && lead[0]) {
        dub_lane_apply_p3_state(3u, 2u);
        dub_sync_heard_mutes();
    }
    {
        const char *met = getenv("FM1_HVYPSY_LEAD_METRICS");

        if (met && met[0])
            atexit(heavy_psych_host_lead_metrics_print);
    }
    if (chord && chord[0]) {
        dub_lane_apply_p3_state(5u, 2u);
        dub_sync_heard_mutes();
    }
}
#else
static void host_hvypsy_lead_only_prep(void) {}
#endif

static void dub_persist_save(void)
{
#if defined(DINGHY_WEB)
    dub_web_store_primmed = 1u;
#endif
    memcpy(g.cfg.lfo_slots, g.lfo_slots, sizeof g.cfg.lfo_slots);
    dub_cpl_store();
#if !defined(GENRE_BASS)
    gv_envx_lfo_pack();
#endif
    g.cfg.lfo_target = g.lfo_target;
    memcpy(g.cfg.euclid_hits, g.euclid_hits, sizeof g.euclid_hits);
    memcpy(g.cfg.euclid_rot, g.euclid_rot, sizeof g.euclid_rot);
    memcpy(g.cfg.vary_lane, g.vary_lane, sizeof g.vary_lane);
    memcpy(g.cfg.lane_vel, g.lane_vel, sizeof g.lane_vel);
    memcpy(g.cfg.lane_trig, g.lane_trig, sizeof g.lane_trig);
    memcpy(g.cfg.lane_gate, g.lane_gate, sizeof g.lane_gate);
    memcpy(g.cfg.lane_tune, g.lane_tune, sizeof g.lane_tune);
#if defined(GENRE_BASS)
    memcpy(g.cfg.step_vel, g.step_vel[4], sizeof g.cfg.step_vel);
    memcpy(g.cfg.step_prob, g.step_prob[4], sizeof g.cfg.step_prob);
    memcpy(g.cfg.step_micro, g.step_micro[4], sizeof g.cfg.step_micro);
    memcpy(g.cfg.step_tune, g.step_tune[4], sizeof g.cfg.step_tune);
#else
    memcpy(g.cfg.step_vel, g.step_vel, sizeof g.step_vel);
    memcpy(g.cfg.step_prob, g.step_prob, sizeof g.step_prob);
    memcpy(g.cfg.step_micro, g.step_micro, sizeof g.step_micro);
    memcpy(g.cfg.step_tune, g.step_tune, sizeof g.step_tune);
#endif
#if GENRE_CFG_USER_PLATE_N < 64u
    g.user_plate_mask &= (1ull << GENRE_CFG_USER_PLATE_N) - 1ull;
#endif
    g.cfg.user_plate_mask = g.user_plate_mask;
#if defined(GENRE_BASS)
    {
        uint32_t pi;

        for (pi = 0u; pi < GENRE_CFG_USER_PLATE_N; pi++)
            bass_plate_pack(&g.cfg.user_plates[pi], &g.user_plates[pi]);
    }
#else
    memcpy(g.cfg.user_plates, g.user_plates, sizeof g.cfg.user_plates);
#endif
    memcpy(g.cfg.seq_lfo_lane, g.seq_lfo_lane, sizeof g.seq_lfo_lane);
    /* song arrangement: only in SONG mode; ONLY latch is runtime-only (the pre-ONLY values are stored) */
    seq_arr_song_mirror();
    settings_save(&g.cfg, sizeof g.cfg);
}

static void dub_persist_unpack(void)
{
    uint32_t i;
    uint8_t lt;

    memcpy(g.lfo_slots, g.cfg.lfo_slots, sizeof g.cfg.lfo_slots);
    dub_cpl_load();
#if !defined(GENRE_BASS)
    gv_envx_lfo_unpack();
#endif
    fm_lfo_slots_fixup_empty_rates(g.lfo_slots, DUB_LFO_TGT_N);
#if defined(GENRE_BASS)
    {
        /* Older BASS era presets put depth on these; KO BASS has no knob for them (invisible LFO). */
        static const uint8_t stale[] = {DUB_LFO_CUT_C, DUB_LFO_TIMB_C, DUB_LFO_SRATE, DUB_LFO_M_DUB,
                                        DUB_LFO_M_MOTION, DUB_LFO_M_FLUX, DUB_LFO_M_GRIT};

        for (i = 0u; i < sizeof stale; i++)
            if (!fm_lfo_dests_has(&dub_lfo_dests, stale[i]))
                g.lfo_slots[stale[i]].depth = 0u;
    }
#else
    {
        /* Drum ALL bus targets have no knob any more: hand a stored depth to KICK/SNARE/HAT
         * (only where that slot is empty), then clear the bus slot so nothing modulates invisibly. */
        static const uint8_t bus[][2] = {
            {DUB_LFO_KTUNE, DUB_LFO_KTUNE_K}, {DUB_LFO_KDEC, DUB_LFO_KDEC_K}, {DUB_LFO_KDRV, DUB_LFO_KDRV_K},
            {DUB_LFO_DENS, DUB_LFO_DENS_K},   {DUB_LFO_DCUT, DUB_LFO_DCUT_K}, {DUB_LFO_DATK, DUB_LFO_DATK_K},
            {DUB_LFO_DLY_D, DUB_LFO_DLY_K},   {DUB_LFO_REV_D, DUB_LFO_REV_K},
        };
        uint32_t j;

        for (i = 0u; i < sizeof bus / sizeof bus[0]; i++) {
            fm_lfo_slot_t *src = &g.lfo_slots[bus[i][0]];

            if (src->depth == 0u)
                continue;
            for (j = 0u; j < 3u; j++) {
                if (g.lfo_slots[bus[i][1] + j].depth == 0u)
                    g.lfo_slots[bus[i][1] + j] = *src;
            }
            fm_lfo_slot_reset(src);
            src->depth = 0u;
        }
    }
#endif
    lt = g.cfg.lfo_target % DUB_LFO_TGT_N;
    /* Legacy: unset lfo_target (0) tracked chord cutoff via lfo1_tgt. */
    if (lt == 0u && g.cfg.p.lfo1_tgt == (uint8_t)DUB_LFO_CUT_C)
        lt = dub_lfo_tgt_default();
    else
        lt = dub_lfo_tgt_unpack(lt);
    g.lfo_target = lt;
    g.cfg.lfo_target = lt;
    for (i = 0; i < DUB_LFO_TGT_N; i++) {
        g.lfo_rt[i].ph = 0u;
        g.lfo_rt[i].cycle = 0u;
        g.lfo_rt[i].sm = 0;
    }
    memcpy(g.euclid_hits, g.cfg.euclid_hits, sizeof g.euclid_hits);
    for (i = 0; i < DUB_SEQ_LANES; i++)
        g.pattern_origin[i] = FM_SEQ_PATTERN_ORIGIN_NONE;
    memcpy(g.euclid_rot, g.cfg.euclid_rot, sizeof g.euclid_rot);
    memcpy(g.vary_lane, g.cfg.vary_lane, sizeof g.vary_lane);
    memcpy(g.lane_vel, g.cfg.lane_vel, sizeof g.lane_vel);
    memcpy(g.lane_trig, g.cfg.lane_trig, sizeof g.lane_trig);
    memcpy(g.lane_gate, g.cfg.lane_gate, sizeof g.lane_gate);
    memcpy(g.lane_tune, g.cfg.lane_tune, sizeof g.lane_tune);
#if defined(GENRE_BASS)
    memset(g.step_vel, 0, sizeof g.step_vel);
    memset(g.step_prob, 0, sizeof g.step_prob);
    memset(g.step_micro, 0, sizeof g.step_micro);
    memset(g.step_tune, 0, sizeof g.step_tune);
    memcpy(g.step_vel[4], g.cfg.step_vel, sizeof g.cfg.step_vel);
    memcpy(g.step_prob[4], g.cfg.step_prob, sizeof g.cfg.step_prob);
    memcpy(g.step_micro[4], g.cfg.step_micro, sizeof g.cfg.step_micro);
    memcpy(g.step_tune[4], g.cfg.step_tune, sizeof g.cfg.step_tune);
#else
    memcpy(g.step_vel, g.cfg.step_vel, sizeof g.step_vel);
    memcpy(g.step_prob, g.cfg.step_prob, sizeof g.step_prob);
    memcpy(g.step_micro, g.cfg.step_micro, sizeof g.step_micro);
    memcpy(g.step_tune, g.cfg.step_tune, sizeof g.step_tune);
#endif
#if GENRE_CFG_USER_PLATE_N < 64u
    g.cfg.user_plate_mask &= (1ull << GENRE_CFG_USER_PLATE_N) - 1ull;
#endif
    g.user_plate_mask = g.cfg.user_plate_mask;
#if defined(GENRE_BASS)
    for (i = 0u; i < GENRE_CFG_USER_PLATE_N; i++) {
        memset(&g.user_plates[i], 0, sizeof g.user_plates[i]);
        if (!(g.user_plate_mask & (1ull << i)))
            continue;
        genre_preset_rebuild(&g.user_plates[i], genre_era_of_preset((uint8_t)i), (uint8_t)i);
        bass_plate_unpack(&g.user_plates[i], &g.cfg.user_plates[i]);
    }
#else
    memcpy(g.user_plates, g.cfg.user_plates, sizeof g.user_plates);
#endif
    memcpy(g.seq_lfo_lane, g.cfg.seq_lfo_lane, sizeof g.seq_lfo_lane);
    g.cfg.p.era = fm_era_knob_clamp(g.cfg.p.era);
    if (fm_era_is_artist(g.cfg.p.era) && !fm_artist_plate_count())
        g.cfg.p.era = genre_era_of_preset(g.cfg.p.plate);
#if defined(GENRE_BASS)
    bass_app_rt_init(&g.bass_rt);
    g.voice.mute = 0xEu;
    g.synth.bass_env_cfg = g.cfg.bass.env;
#endif
}

static int dub_cfg_load(void)
{
    if (settings_load(&g.cfg, sizeof g.cfg) == 0)
        return 0;
#if defined(GENRE_BASS)
    {
        /* Blob saved before the ENV CURVE tail byte: same layout, sizeof without it. */
        const uint32_t al = (uint32_t)_Alignof(dub_cfg_t);
        const uint32_t old_n = ((uint32_t)offsetof(dub_cfg_t, bass_env_shape) + al - 1u) & ~(al - 1u);

        if (old_n != (uint32_t)sizeof g.cfg) {
            g.cfg.bass_env_shape = 0u;
            return settings_load(&g.cfg, old_n);
        }
    }
#else
    {
        /* Blobs saved before the ENV LFO / KO KIT ENV tails: same layout, sizeof without them. */
        const uint32_t al = (uint32_t)_Alignof(dub_cfg_t);
        const uint32_t n_lfo = ((uint32_t)offsetof(dub_cfg_t, envx_lfo) + al - 1u) & ~(al - 1u);
        const uint32_t n_env = ((uint32_t)offsetof(dub_cfg_t, envx) + al - 1u) & ~(al - 1u);

        memset(g.cfg.envx_lfo, 0, sizeof g.cfg.envx_lfo);
        if (n_lfo != (uint32_t)sizeof g.cfg && settings_load(&g.cfg, n_lfo) == 0)
            return 0;
        if (n_env != n_lfo) {
            memset(g.cfg.envx, 0, sizeof g.cfg.envx);
            memset(g.cfg.envx_lfo, 0, sizeof g.cfg.envx_lfo);
            return settings_load(&g.cfg, n_env);
        }
    }
#endif
    return -1;
}

static void app_init(void)
{
    uint32_t i;
    palette_set(2u);
#if !defined(GENRE_BASS)
    gv_lfo_names_init();
#endif
    dub_lfo_order_build();
    seq_runtime_defaults();
    memset(g.seq_view_on, 1, sizeof g.seq_view_on);
    memset(g.seq_view_cyc, 0, sizeof g.seq_view_cyc);
    for (i = 1u; i < FM_SEQ_MAX_PAGES; i++)
        g.seq_view_cyc[i] = FM_SEQ_VIEW_CYC_ALL;
    g.seq_view_only_page = FM_SEQ_MAX_PAGES;
    memset(g.seq_view_reps, 1, sizeof g.seq_view_reps);
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
        uint32_t ln;

        for (ln = 0u; ln < DUB_SEQ_LANES; ln++)
            g.seq_lane_len[i][ln] = (uint8_t)FM_SEQ_STEPS;
    }
    memset(g.seq_lane_nudge, 0, sizeof g.seq_lane_nudge);
    g.seq_view_rep_left = 1u;
    fm_lfo_slots_init(g.lfo_slots, g.lfo_rt, DUB_LFO_TGT_N);
    fm_dj_lfo_eff_build(&g.dj_lfo_ov, &g.dj, g.lfo_slots, g.lfo_slots_eff, DUB_LFO_TGT_N);
    g.lfo_target = dub_lfo_tgt_default();
#if defined(GENRE_BASS)
    bass_app_rt_init(&g.bass_rt);
#endif
#if defined(DINGHY_WEB)
    dub_web_store_primmed = 0u;
#endif
    {
        if (dub_cfg_load() == 0 && g.cfg.magic == DUB_CFG_MAGIC) {
#if defined(DINGHY_WEB)
            dub_web_store_primmed = 1u;
#endif
            dub_persist_unpack();
#if defined(GENRE_BASS)
            bass_sanitize_nav();
            /* g.voice.macro_* (HOME macros + PERFORM) are not in the blob: start neutral. */
            dub_era_home_macros_apply_era(g.cfg.p.era, 0u);
#endif
            if (g.cfg.p.lvl_b == 0u && g.cfg.p.lvl_c == 0u) {
                g.cfg.p.lvl_b = 96u;
                g.cfg.p.lvl_c = 92u;
            }
        } else {
            /* magic mismatch or unreadable blob: factory reset incl. CPL defaults */
            cfg_defaults();
            for (i = 0; i < DUB_SEQ_STEPS; i++)
                g.cfg.seq[i].force = 0;
            dub_persist_unpack();
#if defined(GENRE_BASS)
            bass_sanitize_nav();
#else
            memset(g.cfg.envx, 0, sizeof g.cfg.envx);
#endif
        }
    }
#if !defined(GENRE_BASS)
    gv_envx_sync();
#endif
    /* Arrangement at boot: SONG (user_song == 1) reads the song storage; preset mode derives it from the plate.
     * Blobs saved before seq_cyc existed load its tail as 0 (= OFF on every page): clamp invalid values, and a
     * song with no page ON counts as invalid and falls back to the preset derivation. */
    {
        uint8_t any_on = 0u;

        for (i = 0u; i < FM_SEQ_MAX_PAGES; i++) {
            if (g.cfg.seq_cyc[i] >= FM_SEQ_NVIEW_CYC)
                g.cfg.seq_cyc[i] = FM_SEQ_VIEW_CYC_OFF;
            if (g.cfg.seq_cyc[i] != FM_SEQ_VIEW_CYC_OFF)
                any_on = 1u;
            if (g.cfg.seq_reps[i] < 1u || g.cfg.seq_reps[i] > 16u)
                g.cfg.seq_reps[i] = 1u;
        }
        if (g.cfg.user_song && any_on) {
            memcpy(g.seq_view_cyc, g.cfg.seq_cyc, sizeof g.seq_view_cyc);
            memcpy(g.seq_view_reps, g.cfg.seq_reps, sizeof g.seq_view_reps);
        } else {
            seq_arr_derive_preset();
            seq_arr_song_mirror(); /* no-op unless user_song: an invalid song gets the derived arrangement */
        }
    }
    for (i = 0u; i < FM_SEQ_MAX_PAGES; i++)
        seq_view_cyc_sync_on((uint8_t)i);
#if defined(FM_PGEN_ON)
    if (g.cfg.pgen.ok != FM_PGEN_CFG_OK || g.cfg.pgen.comply > 127u) {
        g.cfg.pgen.n = 0;
        g.cfg.pgen.comply = FM_PGEN_COMPLY_DEF;
        g.cfg.pgen.ok = FM_PGEN_CFG_OK;
    }
#endif
    cfg_boot_random_song();
    seq_rematerialize_gates_boot();
    dub_drm_send_fixup(&g.cfg.p);
    dub_drm_dens_fixup(&g.cfg.p);
    transport_set_bpm();
    g.morph_pos = DUB_MORPH_STEPS;
    g.morph_acc = 0u;
    g.cue_on = 0u;
    g.groove_plate_pending = 0u;
    g.cue_acc = 0u;
    g.cue_ticks = 0u;
    g.morph_from = g.cfg.p;
    fm_seq_bits_fill(g.morph_from_seq, 0);
    g.macro_morph_pos = DUB_MORPH_STEPS;
    g.macro_morph_acc = 0u;
#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_BASS) || defined(GENRE_POP) || defined(GENRE_SOUL)
    g.fill_build = 0u;
    g.fill_build_snap = 0u;
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)
    g.fill_build = 0u;
    g.fill_build_snap = 0u;
    g.idm_edit_latch = 0u;
    g.idm_stutter.active = 0u;
    g.idm_stutter.slice_len = 0u;
    g.idm_stutter.slice_left = 0u;
#endif
    g.lfo_last_ens = g.seq_ens_sel;
    g.home_morph_flash_ms = 0u;
    g.home_morph_flash_kind = 0u;
    dub_synth_init(&g.synth);
    sync_voice_params();
    transport_reset();
    g.view = VIEW_HOME;
    g.bank = BANK_HOME;
    for (i = 0; i < BANK_COUNT; i++)
        g.page[i] = 0;
    g.lfo_sel = 0u;
#if defined(GENRE_BASS)
    g.seq_lane = 4u;
    g.seq_lane_sel = 4u;
    g.seq_view_page = 0u;
#else
    g.seq_lane = 0u;
    g.seq_lane_sel = 0u;
#endif
    g.seq_follow = 1u;
#if defined(GENRE_BASS)
    bass_sanitize_nav();
    bass_cfg_ext_sanitize(&g.cfg.bass);
    g.ext_clock = bass_clock_follows_ext(g.cfg.bass.clock_mode);
#endif
    host_seq_chain_selftest_run();
    host_seq_oxi_shot_prep();
    host_hvypsy_lead_only_prep();
#if defined(DINGHY_HOST) && defined(GENRE_HOUSE)
    {
        const char *e = getenv("FM1_HOUSE_FILL_TEST");

        if (e && e[0])
            g.cfg.p.density = 80u;
    }
#endif
    g.ui_dirty = 1u;
    draw_screen();
}

static void app_step(void)
{
    uint32_t b, k, rising, released, btn_released, keys = fm1_in.notes;
    uint8_t st, d1, d2;
    uint8_t edit_held;

    platform_poll();
    b = fm1_input_edges(&btn_released);
    rising = (keys & ~g.prev_keys) | fm1_input_note_edges();
    released = g.prev_keys & ~keys;
    g.prev_keys = keys;
    edit_held = (fm1_in.buttons & (1u << BTN_EDIT)) != 0u;
    {
        uint8_t dj_gest_bar = (g.dj.on && g.dj.bar_pending) ? 1u : 0u;

        fm_dj_app_phrase_step(&g.dj, g.bar_step, DUB_STEPS, dj_fill_live() ? 1u : 0u, &g.ui_dirty,
                              fm_dj_on_trans, &g.kick_slam_bars);
        if (dj_gest_bar)
            dub_dj_gest_on_bar();
    }
    seq_plock_latch_touch_break(b, rising);
    {
        uint8_t seq_held_btn = (fm1_in.buttons & (1u << BTN_SEQ)) != 0u;

        if (b & (1u << BTN_REC)) {
            if (g.view == VIEW_SEQ && seq_held_btn && !edit_held)
                seq_clip_paste();
            else if (edit_held) {
                uint32_t i;

                for (i = 0; i < DUB_SEQ_STEPS; i++)
                    g.cfg.seq[i].force = 0;
                mark_dirty();
            } else {
                g.rec_arm = g.rec_arm ? 0u : 1u;
            }
            g.ui_dirty = 1u;
        }
        if (b & (1u << BTN_SAVE)) {
            if (g.song_setup_latch) {
                user_song_commit();
                dub_persist_save();
                g.set_dirty = 0u;
                g.saved_until = fm1_ms + 1200u;
                g.ui_dirty = 1u;
#if DUB_TEMP_SONG_SETUP_LONGPRESS
                g.save_press_ms = 0u;
                g.save_long_used = 0u;
#endif
            } else if (g.view == VIEW_SEQ && seq_held_btn && !edit_held)
                seq_clip_copy(keys);
#if DUB_TEMP_SONG_SETUP_LONGPRESS
            else {
                /* Defer plate-save to release; long-press opens user-song setup. */
                g.save_press_ms = fm1_ms;
                g.save_long_used = 0u;
            }
#else
            else {
                uint8_t pl_ix = g.cfg.p.plate % GENRE_PRESET_N;

                if (edit_held) {
                    if (pl_ix < GENRE_CFG_USER_PLATE_N)
                        g.user_plate_mask &= ~(1ull << pl_ix);
                    genre_plate_apply(&g.cfg.p, pl_ix, DUB_APPLY_ALL);
                    transport_set_bpm();
                } else if (pl_ix < GENRE_CFG_USER_PLATE_N) {
                    g.user_plates[pl_ix] = g.cfg.p;
                    g.user_plate_mask |= (1ull << pl_ix);
                }
                dub_persist_save();
                g.set_dirty = 0u;
                g.saved_until = fm1_ms + 1200u;
                g.ui_dirty = 1u;
            }
#endif
        }
#if DUB_TEMP_SONG_SETUP_LONGPRESS
        if ((fm1_in.buttons & (1u << BTN_SAVE)) && g.save_press_ms && !g.song_setup_latch) {
            ko_longpress_t lp = {&g.save_press_ms, &g.save_long_used, NULL};

            if (ko_longpress_poll(&lp, fm1_ms, 500u, 0)) {
                song_setup_open();
                g.ui_dirty = 1u;
            }
        }
        if ((btn_released & (1u << BTN_SAVE)) && g.save_press_ms && !g.save_long_used &&
            !g.song_setup_latch) {
            uint8_t pl_ix = g.cfg.p.plate % GENRE_PRESET_N;

            if (edit_held) {
                if (pl_ix < GENRE_CFG_USER_PLATE_N)
                    g.user_plate_mask &= ~(1ull << pl_ix);
                genre_plate_apply(&g.cfg.p, pl_ix, DUB_APPLY_ALL);
                transport_set_bpm();
            } else if (pl_ix < GENRE_CFG_USER_PLATE_N) {
                g.user_plates[pl_ix] = g.cfg.p;
                g.user_plate_mask |= (1ull << pl_ix);
            }
            dub_persist_save();
            g.set_dirty = 0u;
            g.saved_until = fm1_ms + 1200u;
            g.ui_dirty = 1u;
        }
        if (btn_released & (1u << BTN_SAVE)) {
            g.save_press_ms = 0u;
            g.save_long_used = 0u;
        }
#endif
    }
    if (g.view == VIEW_SEQ && g.cfg.p.arp_mode != 0u) {
        dub_arp_set_mode(0u);
        mark_dirty();
    }
    if (g.edit_groove_arm_ms != 0u && fm1_ms - g.edit_groove_arm_ms >= DUB_EDIT_LATCH_MS &&
        !(fm1_in.buttons & (1u << BTN_EDIT))) {
        g.edit_groove_arm_ms = 0u;
        g.edit_press_ms = 0u;
        if (g.view == VIEW_SEQ && g.seq_page == 5u && !g.console && !g.song_setup_latch)
            dub_load_plate_song_groove(g.cfg.p.plate);
    }
    if (b & (1u << BTN_EDIT)) {
        if (g.song_setup_latch) {
            song_setup_discard();
        } else {
            /* Double-click EDIT (< DUB_EDIT_LATCH_MS) latches Console; long release latches too if hold stayed clean.
             * Single tap when latched exits Console. */
            g.edit_long_arm = 0u;
            g.edit_long_cancel = 0u;
            if (g.console) {
                g.console = 0u;
                g.edit_press_ms = 0u;
                g.edit_groove_arm_ms = 0u;
            } else if (g.edit_press_ms != 0u && fm1_ms - g.edit_press_ms < DUB_EDIT_LATCH_MS) {
                g.console = 1u;
                g.edit_press_ms = 0u;
                g.edit_groove_arm_ms = 0u;
            } else {
                g.edit_press_ms = fm1_ms;
                g.edit_long_arm = 1u;
            }
        }
        g.ui_dirty = 1u;
    }
    edit_long_press_poll(b, rising, edit_held);
    if ((btn_released & (1u << BTN_EDIT)) && !g.console && g.edit_press_ms != 0u) {
        uint32_t held = fm1_ms - g.edit_press_ms;

        if (g.edit_long_arm && !g.edit_long_cancel && held >= DUB_EDIT_LATCH_MS) {
#if defined(GENRE_BASS)
            if (g.view != VIEW_SEQ)
#endif
            {
                g.console = 1u;
                g.ui_dirty = 1u;
            }
            g.edit_press_ms = 0u;
            g.edit_groove_arm_ms = 0u;
        } else if (held < DUB_EDIT_LATCH_MS && g.view == VIEW_SEQ && g.seq_page == 5u && !g.song_setup_latch)
            g.edit_groove_arm_ms = fm1_ms;
    }
    if (btn_released & (1u << BTN_EDIT)) {
        g.edit_long_arm = 0u;
        g.edit_long_cancel = 0u;
    }

#if defined(GENRE_BASS)
    if (b & (1u << BTN_ENV))
        bank_button(BANK_CHORD, edit_held);
#else
    if (b & (1u << BTN_ENV))
        bank_button(BANK_ENV, edit_held);
#endif
#if defined(GENRE_BASS)
    if (b & (1u << BTN_GLO))
        bank_button(BANK_GROOVE, edit_held);
    if (b & (1u << BTN_SEL))
        bank_button(BANK_BASS, edit_held);
#else
    if (b & (1u << BTN_GLO))
        bank_button(BANK_GROOVE, edit_held);
    if (b & (1u << BTN_SEL))
        bank_button(gv_sel_bank(), edit_held);
#endif
    /* BTN_FX: short = BANK_FX on release; long >=500ms toggles DJ (fm_dj_app.h).
     * Do not git checkout fm_app.c without re-applying DJ — run tests/check_dj_wired.sh */
    if (b & (1u << BTN_FX)) {
        fm_dj_app_fx_t fx_io = fm_dj_app_fx_io();

        if (edit_held)
            bank_button(BANK_FX, edit_held);
        else
            fm_dj_app_fx_on_press(&fx_io, fm1_ms);
    }
    {
        fm_dj_app_fx_t fx_io = fm_dj_app_fx_io();
        uint8_t dj_was_on = g.dj.on;

        fm_dj_app_fx_long_press_poll(&fx_io, b, rising, fm1_in.buttons, (1u << BTN_FX), fm1_ms,
                                     dub_boot_seed());
        if (g.dj.on != dj_was_on)
            dj_mode_toggled();
        if ((btn_released & (1u << BTN_FX)) && !edit_held)
            fm_dj_app_fx_on_release(&fx_io, fm1_ms, fm_dj_app_bank_fx_short);
    }
    /* BTN_LFO: EDIT+LFO = bank reset (unchanged). Plain: short = BANK_LFO on release, long >= 500 ms = reset
     * current target only (no navigation). */
    if (b & (1u << BTN_LFO)) {
        if (edit_held)
            bank_button(BANK_LFO, edit_held);
        else {
            g.lfo_press_ms = fm1_ms;
            g.lfo_long_used = 0u;
        }
    }
    if (g.lfo_press_ms && !edit_held && (fm1_in.buttons & (1u << BTN_LFO))) {
        ko_longpress_t lp = {&g.lfo_press_ms, &g.lfo_long_used, NULL};

        if (ko_longpress_poll(&lp, fm1_ms, DUB_LFO_LONG_MS, 0)) {
            lfo_target_reset();
            g.ui_dirty = 1u;
        }
    }
    if (btn_released & (1u << BTN_LFO)) {
        if (g.lfo_press_ms && !g.lfo_long_used)
            bank_button(BANK_LFO, 0u);
        g.lfo_press_ms = 0u;
        g.lfo_long_used = 0u;
    }
    if (b & (1u << BTN_HOME))
        bank_button(BANK_HOME, edit_held);

    if (b & (1u << BTN_SEQ)) {
        uint8_t setup_cancel = 0u;

        if (g.song_setup_latch) {
            song_setup_discard();
            setup_cancel = 1u;
        }
        g.seq_press_ms = fm1_ms;
        g.seq_press_master_q12 = master_q12;
        g.seq_long_cancel = 0u;
        g.seq_long_used = 0u;
        /* Cancel setup via SEQ must not page-cycle on release. */
        g.seq_mod_used = setup_cancel;
        g.seq_master_mod = 0u;
        g.seq_was_in_seq = (g.view == VIEW_SEQ) ? 1u : 0u;
        if (edit_held) {
            uint8_t ln;
            uint32_t si;
            for (ln = 0; ln < DUB_SEQ_LANES; ln++)
                seq_clear_lane(ln);
            for (si = 0; si < DUB_SEQ_STEPS; si++)
                g.cfg.seq[si].force = 0u;
#if defined(GENRE_BASS)
            bass_step_x_long_clear_unlock(g.cfg.bass.step_x, DUB_SEQ_STEPS);
#endif
            g.seq_mod_used = 1u;
            mark_dirty();
        }
        g.ui_dirty = 1u;
    }
    seq_long_press_poll(b, rising);
    if ((fm1_in.buttons & (1u << BTN_SEQ)) && g.seq_press_ms && !g.seq_long_used && !g.seq_long_cancel) {
        int32_t md = master_q12 - g.seq_press_master_q12;

        if (md < 0)
            md = -md;
        if (md > DUB_SEQ_MASTER_THRESH) {
            g.seq_mod_used = 1u;
            g.seq_master_mod = 1u;
            g.seq_long_cancel = 1u;
        }
    }
    if ((btn_released & (1u << BTN_SEQ)) && g.seq_master_mod) {
        song_setup_open();
        g.ui_dirty = 1u;
    } else if ((btn_released & (1u << BTN_SEQ)) && !g.seq_mod_used) {
#if defined(GENRE_BASS)
        if (g.seq_plock_latch && g.view == VIEW_SEQ) {
            g.seq_plock_bank ^= 1u;
        } else
#endif
        if (g.seq_was_in_seq) {
            g.seq_page = dub_seq_page_cycle_next(g.seq_page);
        } else {
            /* Auto-select matching lane when entering SEQ from BASS (GLO) or CHORD (ENV) */
#if defined(GENRE_BASS)
            g.seq_lane = 4u;
            seq_lane_sel_set(4u);
#else
            if (g.bank == BANK_BASS)
                seq_lane_sel_set(4u);
            else if (g.bank == BANK_CHORD)
                seq_lane_sel_set(5u);
#endif
            g.seq_page = DUB_SEQ_PAGE_ENTRY;
            nav_view(VIEW_SEQ);
        }
        g.ui_dirty = 1u;
    }
    if (btn_released & (1u << BTN_SEQ))
        g.seq_press_ms = 0u;
    if (b & (1u << BTN_ARP)) {
        g.arp_press_ms = fm1_ms;
        g.arp_long_used = 0u;
        if (edit_held) {
            dub_arp_user_set(0u);
            g.arp_long_used = 1u;
            if (g.view == VIEW_SEQ)
                nav_bank(g.seq_ret_bank);
            mark_dirty();
        }
    }
    if ((fm1_in.buttons & (1u << BTN_ARP)) && !g.arp_long_used && g.arp_press_ms != 0u &&
        (fm1_ms - g.arp_press_ms >= 420u)) {
        /* Long-press ARP (>= 420 ms) turns ARP off */
        dub_arp_user_set(0u);
        g.arp_long_used = 1u;
        if (g.view == VIEW_SEQ)
            nav_bank(g.seq_ret_bank);
        mark_dirty();
    }
    if ((btn_released & (1u << BTN_ARP)) && !g.arp_long_used) {
        dub_arp_user_set((uint8_t)((g.cfg.p.arp_mode + 1u) % 6u));
        if (g.view == VIEW_SEQ)
            nav_bank(g.seq_ret_bank);
        mark_dirty();
    }
    if (b & (1u << BTN_PLAY)) {
        g.playing = g.playing ? 0u : 1u;
        if (g.playing) {
            dub_transport_morph_snap();
            transport_reset();
            dub_lfo_transport_start();
            midi_start_tx();
        } else {
            dub_dj_gest_restore();
            dub_transport_morph_snap();
            midi_stop_tx();
#if defined(GENRE_BASS)
            bass_transport_stop();
#endif
        }
        g.ui_dirty = 1u;
    }
#if defined(GENRE_BASS)
    if (b & (1u << BTN_OCT_DOWN)) {
        dub_lfo_touch_oct(-1);
        if (edit_held) {
            g.voice.tape_oct = (g.voice.tape_oct == -1) ? 0 : -1;
            g.ui_dirty = 1u;
        } else if (g.cfg.p.root >= 24u) {
            g.cfg.p.root = (uint8_t)(g.cfg.p.root - 12u);
            dub_morph_sync_field(&g.cfg.p.root);
            mark_dirty();
        }
    }
    if (b & (1u << BTN_OCT_UP)) {
        dub_lfo_touch_oct(1);
        if (edit_held) {
            g.voice.tape_oct = (g.voice.tape_oct == 1) ? 0 : 1;
            g.ui_dirty = 1u;
        } else if (g.cfg.p.root <= 84u) {
            g.cfg.p.root = (uint8_t)(g.cfg.p.root + 12u);
            dub_morph_sync_field(&g.cfg.p.root);
            mark_dirty();
        }
    }
#else
    if ((b & (1u << BTN_OCT_DOWN)) && g.view == VIEW_SEQ && !edit_held) {
        seq_oct(-1);
    } else if (b & (1u << BTN_OCT_DOWN)) {
        dub_lfo_touch_oct(-1);
        if (edit_held) {
            g.voice.tape_oct = (g.voice.tape_oct == -1) ? 0 : -1;
            g.ui_dirty = 1u;
        } else if (g.cfg.p.root >= 24u) {
            g.cfg.p.root = (uint8_t)(g.cfg.p.root - 12u);
            dub_morph_sync_field(&g.cfg.p.root);
            mark_dirty();
        }
    }
    if ((b & (1u << BTN_OCT_UP)) && g.view == VIEW_SEQ && !edit_held) {
        seq_oct(1);
    } else if (b & (1u << BTN_OCT_UP)) {
        dub_lfo_touch_oct(1);
        if (edit_held) {
            g.voice.tape_oct = (g.voice.tape_oct == 1) ? 0 : 1;
            g.ui_dirty = 1u;
        } else if (g.cfg.p.root <= 84u) {
            g.cfg.p.root = (uint8_t)(g.cfg.p.root + 12u);
            dub_morph_sync_field(&g.cfg.p.root);
            mark_dirty();
        }
    }
#endif

    {
        uint8_t seq_held_enc = (fm1_in.buttons & (1u << BTN_SEQ)) != 0u;
        int32_t pst = fm1_enc_take(ENC_PRESETS);
        int32_t est = fm1_enc_take(ENC_ALGORITHM);
        int32_t vst = fm1_enc_take(ENC_SELECT);

        if (g.seq_plock_latch && (pst || est || vst)) {
            seq_plock_latch_end();
            g.ui_dirty = 1u;
        }

        if (pst && edit_held && g.edit_long_arm && !g.edit_long_cancel)
            g.edit_long_cancel = 1u;
        if (est && edit_held && g.edit_long_arm && !g.edit_long_cancel)
            g.edit_long_cancel = 1u;
        if (vst && edit_held && g.edit_long_arm && !g.edit_long_cancel)
            g.edit_long_cancel = 1u;

        if (seq_held_enc && g.seq_press_ms && !g.seq_long_used && !g.seq_long_cancel &&
            (pst || est || vst))
            g.seq_long_cancel = 1u;
        if (pst || est || vst) {
            fm_dj_app_fx_t fx_io = fm_dj_app_fx_io();

            fm_dj_app_fx_cancel_hold(&fx_io);
        }

        if (g.song_setup_latch) {
            if (pst) {
                int32_t c = (int32_t)g.song_setup_name_cur + pst;

                while (c < 0)
                    c += (int32_t)DUB_SONG_NAME_LEN;
                g.song_setup_name_cur = (uint8_t)((uint32_t)c % DUB_SONG_NAME_LEN);
                g.ui_dirty = 1u;
            }
            if (vst) {
                song_setup_name_cycle(vst);
                g.ui_dirty = 1u;
            }
        } else if (pst && seq_held_enc) {
            int32_t k = (int32_t)(g.cfg.p.root % 12u) + pst;

            while (k < 0)
                k += 12;
            g.cfg.p.root = (uint8_t)((g.cfg.p.root / 12u) * 12u + (uint32_t)(k % 12));
            dub_morph_sync_field(&g.cfg.p.root);
            g.seq_mod_used = 1u;
            seq_flash_key_val(g.cfg.p.root);
            mark_dirty();
#if !defined(GENRE_BASS)
        } else if (pst && (fm1_in.buttons & (1u << BTN_HOME)) &&
            g.view == VIEW_HOME) {
            dub_home_randomize_morph();
            if (pst > 0) {
                int32_t ix = (int32_t)g.seq_ens_sel + pst;
                while (ix < 0)
                    ix += (int32_t)FM_SEQ_NENSEMBLES;
                dub_apply_ensemble((uint8_t)((uint32_t)ix % FM_SEQ_NENSEMBLES));
            }
        } else if (pst) {
#else
        } else if (pst) {
#endif
            if (g.dj.on)
                g.dj.drop_armed = 0u;
#if defined(FM_PGEN_ON)
            /* EDIT + PRESET: right jumps far along n, left returns to the era's first landmark (n = 0). */
            if (edit_held && pst < 0 && !fm_era_is_artist(g.cfg.p.era)) {
                cycle_plate(-g.cfg.pgen.n);
                g.plate_home_until = fm1_ms + 1200u;
                g.ui_dirty = 1u;
            } else
                cycle_plate(edit_held ? pst * (int32_t)FM_PGEN_JUMP : pst);
#else
            cycle_plate(pst);
#endif
        }

        if (!g.song_setup_latch && est && seq_held_enc) {
            g.cfg.p.scale = scale_step(g.cfg.p.scale, est);
            dub_morph_sync_field(&g.cfg.p.scale);
            g.seq_mod_used = 1u;
            seq_flash_knob(SCALE_NAMES[scale_ix(g.cfg.p.scale)]);
            mark_dirty();
#if defined(FM_PGEN_ON)
        } else if (!g.song_setup_latch && est && edit_held && !fm_era_is_artist(g.cfg.p.era)) {
            int32_t c = (int32_t)g.cfg.pgen.comply + est * 4;
            char s[8];

            if (c < 0)
                c = 0;
            if (c > 127)
                c = 127;
            g.cfg.pgen.comply = (uint8_t)c;
            cycle_plate(0);
            copy_str(s, "ERA ");
            u8_show100_dec(s + 4, g.cfg.pgen.comply);
            seq_flash_knob(s);
#endif
        } else if (est) {
            cycle_era(est);
        }

        if (!g.song_setup_latch && vst && seq_held_enc) {
            adj_u8_morph(&g.cfg.p.bpm, vst, 40, 180);
            transport_set_bpm();
            g.seq_mod_used = 1u;
            seq_flash_bpm_val(g.cfg.p.bpm);
            mark_dirty();
#if !defined(GENRE_BASS)
        } else if (!g.song_setup_latch && vst && (fm1_in.buttons & (1u << BTN_HOME)) &&
            g.view == VIEW_HOME)
            dub_home_randomize_morph();
        else if (!g.song_setup_latch && vst) {
#else
        } else if (!g.song_setup_latch && vst) {
#endif
#if defined(GENRE_BASS)
            if (g.view == VIEW_SEQ)
                bass_seq_select_bar((int32_t)bass_seq_edit_bar_runtime(g.seq_view_page, g.cfg.seq_pages) +
                                    (vst > 0 ? 1 : -1));
            else
                bass_select_fader_step(vst);
#else
            static const uint8_t order[7] = {VIEW_DESK, VIEW_CHORD, VIEW_BASS, VIEW_SPACE, VIEW_LFO, VIEW_SEQ, VIEW_HOME};
            uint32_t i, ix = 0;

            for (i = 0; i < 7u; i++)
                if (order[i] == g.view)
                    ix = i;
            ix = (uint32_t)((int32_t)ix + vst);
            while ((int32_t)ix < 0)
                ix += 7u;
            nav_view(order[ix % 7u]);
#endif
        }
    }
    for (k = 0; k < 4u; k++) {
        int32_t steps = fm1_enc_take(ENC_KNOB1 + k);
        if (steps) {
            if (edit_held && g.edit_long_arm && !g.edit_long_cancel)
                g.edit_long_cancel = 1u;
            if ((fm1_in.buttons & (1u << BTN_SEQ)) && g.seq_press_ms && !g.seq_long_used &&
                !g.seq_long_cancel)
                g.seq_long_cancel = 1u;
            {
                fm_dj_app_fx_t fx_io = fm_dj_app_fx_io();

                fm_dj_app_fx_cancel_hold(&fx_io);
            }
            if (edit_held)
                g.edit_used = 1u;
            knob_adjust((uint8_t)k, steps);
        }
    }

    edit_long_press_poll(b, rising, edit_held);
    seq_white_plock_latch_poll(keys, rising);
    handle_keys(keys, rising, released, edit_held);

    while (midi_get(&st, &d1, &d2)) {
#if defined(GENRE_BASS)
        if ((st == 0xF8u || st == 0xFAu || st == 0xFBu || st == 0xFCu) &&
            !bass_clock_follows_ext(g.cfg.bass.clock_mode))
            continue;
#endif
        if (st == 0xF8u) {
            g.ext_clock = 1u;
#if defined(GENRE_BASS)
            /* swing is the master's job in EXT */
            if (bass_ext_clock_pulse(&g.bass_rt, g.bass_rt.smp_clock, DUB_FS) && g.playing)
                seq_tick();
#else
            if (g.playing)
                seq_tick();
#endif
        } else if (st == 0xFAu) {
            g.playing = 1u;
            g.ext_clock = 1u;
            dub_transport_morph_snap();
            transport_reset();
            dub_lfo_transport_start();
            g.ui_dirty = 1u;
        } else if (st == 0xFBu) {
            g.playing = 1u;
            g.ext_clock = 1u;
            dub_transport_morph_snap();
            g.ui_dirty = 1u;
        } else if (st == 0xFCu) {
            g.playing = 0u;
#if !defined(GENRE_BASS)
            g.ext_clock = 0u;
#endif
            if (g.dj_flip_overlay) {
                dub_invert_all_lane_mutes();
                g.dj_flip_overlay = 0u;
            }
            dub_dj_ens_invert_accent_sync(0u);
            dub_dj_ens_ov_restore_from_base();
            fm_dj_seq_ens_ov_reset(&g.dj_ens_ov);
            if (g.dj.on || g.dj.slew_off)
                fm_dj_slots_phrase_flush(&g.dj);
            dub_transport_morph_snap();
#if defined(GENRE_BASS)
            bass_transport_stop();
#endif
            g.ui_dirty = 1u;
        } else if ((st & 0xF0u) == 0xB0u && (st & 0x0Fu) == 0u && d1 >= 14u && d1 <= 19u)
            apply_cc(d1, d2);
#if defined(GENRE_BASS)
        else if (((st & 0xF0u) == 0x90u || (st & 0xF0u) == 0x80u) && bass_midi_ch_match(g.cfg.bass.midi_ch, st))
            bass_midi_dispatch(st, d1, d2);
#else
        else if ((st & 0xF0u) == 0x90u && (st & 0x0Fu) == 0u && d2) {
            if (d1 < 60u) {
                g.cfg.p.root = d1;
                dub_trig_sub(&g.synth, d1, d2, g.voice.p.atk_b);
                dub_key_toast_show(4u, d1);
                mark_dirty();
            } else if (d1 < 84u) {
                dub_trig_chord(&g.synth, d1, d2, g.voice.p.atk_c, g.voice.p.glide_c, g.voice.p.voicing);
                dub_key_toast_show(5u, d1);
            }
            else if (d1 == 84u)
                dub_siren_set(&g.synth, d2 ? 1u : 0u);
            else if (d1 == 85u)
                dub_trig_rim(&g.synth, d2, 0, g.voice.p.drm_atk[1]);
            else if (d1 == 86u)
                dub_spring_crash(&g.synth);
        }
#endif
    }

    if (g.seq_follow && g.playing)
        g.seq_view_page = (uint8_t)(g.step / FM_SEQ_PAGE_STEPS);
    update_leds(keys);

    if (g.ui_dirty || (g.playing && fm1_ms - g.ui_ms >= 250u) || fm1_ms - g.ui_ms >= 50u) {
        draw_screen();
        g.ui_dirty = 0u;
        g.ui_ms = fm1_ms;
    } else if (g.playing && dub_rail_cell() != g.rail_cell) {
        /* Mid-step quarter jumps fall between step redraws; footer only keeps cells evenly long. */
        palette_set(2u);
        draw_knob_bar();
    }
#if defined(DINGHY_WEB)
    if (dub_web_store_primmed && g.set_dirty && synth_is_quiet() && fm1_ms - g.set_t > 2000u)
#else
    if (g.set_dirty && synth_is_quiet() && fm1_ms - g.set_t > 2000u)
#endif
    {
        dub_persist_save();
        g.set_dirty = 0u;
    }
}

static void app_main(void)
{
    app_init();
    for (;;)
        app_step();
}

#if defined(POP_LEAD_VERIFY) && defined(DINGHY_HOST) && defined(GENRE_POP)
#include <math.h>
#include <time.h>

uint8_t pop_verify_bass_no_ix12;
uint8_t pop_verify_bass_legacy_lp;

static uint32_t pop_v_isqrt64(uint64_t x)
{
    uint64_t res = 0u;
    uint64_t one = 1ull << 62;

    while (one > x)
        one >>= 2;
    while (one) {
        if (x >= res + one) {
            x -= res + one;
            res += one << 1;
        }
        res >>= 1;
        one >>= 2;
    }
    return (uint32_t)res;
}

static void pop_verify_setup_plate(uint8_t plate, uint8_t timbre)
{
    uint8_t era = pop_era_of_plate(plate);

    pop_preset_rebuild(&g.cfg.p, era, plate);
    g.cfg.p.drm_dens[3] = timbre;
    g.cfg.p.siren_rate = 48u;
    g.cfg.p.lvl_c = 92u;
    g.cfg.p.lvl_b = 96u;
    g.cfg.p.feedback = 0u;
    g.cfg.p.pump = 0u;
    g.voice.mute = 7u;
    dub_lane_apply_p3_state(3u, 2u);
    dub_sync_heard_mutes();
    sync_voice_params();
    dub_siren_trig(&g.synth, 120, 0, 8, 0, era);
    g.synth.siren_peak = 30000;
}

static void pop_verify_setup(uint8_t era, uint8_t timbre)
{
    pop_verify_setup_plate(pop_era_first_plate(era), timbre);
}

static uint64_t pop_v_ns(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint32_t pop_v_rms_buf(const int32_t *out, uint32_t nfr)
{
    uint64_t sq = 0u;
    uint32_t i;

    for (i = 0u; i < nfr; i++) {
        int32_t m = (out[(i * 2u)] + out[(i * 2u) + 1u]) >> 1;
        int64_t s = (int64_t)m;

        sq += (uint64_t)(s * s);
    }
    return pop_v_isqrt64(sq / (uint64_t)nfr);
}

static void pop_verify_metrics(uint8_t era, uint8_t timbre, uint32_t *diff_rms, uint32_t *rms)
{
    uint64_t sq = 0u, diff_sq = 0u;
    uint32_t n = 0u, zcr = 0u;
    int32_t prev = 0;
    uint8_t have = 0u;
    uint32_t i;

    (void)era;
    g.cfg.p.drm_dens[3] = timbre;
    sync_voice_params();
    g.synth.siren_env = 28000;
    g.synth.siren_st_tune = 0;
    for (i = 0u; i < DUB_FS / 2u; i++) {
        int32_t sig = pop_lead_render(&g.synth, &g.voice.p, 0);
        int64_t s = (int64_t)sig;

        sq += (uint64_t)(s * s);
        if (have) {
            int32_t d = sig - prev;

            diff_sq += (uint64_t)((int64_t)d * (int64_t)d);
            if ((sig ^ prev) < 0)
                zcr++;
        } else
            have = 1u;
        prev = sig;
        pop_siren_env_step(&g.synth, &g.voice.p);
        n++;
    }
    *rms = pop_v_isqrt64(sq / (uint64_t)n);
    *diff_rms = pop_v_isqrt64(diff_sq / (uint64_t)n);
    (void)zcr;
}

static uint32_t pop_verify_amp_sus_pct(uint8_t plate)
{
    uint8_t era = pop_era_of_plate(plate);
    uint8_t tex = pop_lead_texture(era, pop_lead_variant(plate));
    uint32_t i, n_atk = pop_siren_atk_samples_tex(tex, 64u);
    uint32_t n = n_atk + (DUB_FS * 180u) / 1000u;
    int32_t pk = 1;

    pop_verify_setup_plate(plate, POP_LEAD_TIMB_DEF[era]);
    g.synth.siren_hp_spp = DUB_FS / 8u;
    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 6u, 0);
    dub_siren_trig(&g.synth, 110u, 0, 0, 64u, era);
    for (i = 0u; i < n; i++) {
        (void)pop_lead_render(&g.synth, &g.voice.p, 0);
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    pk = g.synth.siren_peak;
    if (pk < 1)
        pk = 1;
    return (uint32_t)((g.synth.siren_env * 100u) / pk);
}

static uint32_t pop_verify_silence_pct(uint8_t era, uint8_t plate, uint8_t long_gate)
{
    uint32_t i, n = DUB_FS * 4u;
    uint32_t quiet = 0u;
    int32_t pk = 1;
    int32_t thr;
    uint16_t hold, rel;

    pop_preset_rebuild(&g.cfg.p, era, plate);
    sync_voice_params();
    g.synth.siren_hp_spp = DUB_FS / 8u;
    g.synth.siren_env = 0;
    g.synth.siren_gate = 0u;
    g.synth.siren_lead_rel = 0u;
    if (long_gate) {
        pop_siren_note_gate(1u, 0, &hold, &rel, 0u, 0u, g.synth.siren_hp_spp, era);
        g.synth.pop_lead_gate_hold = (uint16_t)(hold * 4u);
        g.synth.pop_lead_gate_rel = rel;
        g.synth.pop_lead_amp_sus_q8 = 250u;
    } else
        pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 4u, 0);
    dub_siren_trig(&g.synth, 110u, 0, 0, 64u, era);
    for (i = 0u; i < n; i++) {
        int32_t sig = pop_lead_render(&g.synth, &g.voice.p, 0);
        int32_t a = sig < 0 ? -sig : sig;

        if (a > pk)
            pk = a;
        pop_siren_env_step(&g.synth, &g.voice.p);
        if (i > DUB_FS / 8u && (i % (DUB_FS / 4u)) == 0u) {
            pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, long_gate ? 8u : 3u, 0);
            dub_siren_trig(&g.synth, 110u, 0, 0, 64u, era);
        }
    }
    thr = pk / 100;
    if (thr < 8)
        thr = 8;
    for (i = 0u; i < n; i++) {
        int32_t sig = pop_lead_render(&g.synth, &g.voice.p, 0);
        int32_t a = sig < 0 ? -sig : sig;

        pop_siren_env_step(&g.synth, &g.voice.p);
        if (a < thr)
            quiet++;
        if (i > DUB_FS / 8u && (i % (DUB_FS / 4u)) == 0u) {
            pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, long_gate ? 8u : 3u, 0);
            dub_siren_trig(&g.synth, 110u, 0, 0, 64u, era);
        }
    }
    return (quiet * 100u) / n;
}

static uint32_t pop_verify_onset_ratio(uint8_t plate)
{
    uint8_t era = pop_era_of_plate(plate);
    uint32_t i;
    uint32_t n0 = (DUB_FS * 10u) / 1000u;
    uint32_t n1 = (DUB_FS * 50u) / 1000u;
    uint32_t n2 = (DUB_FS * 100u) / 1000u;
    uint64_t sq0 = 0u, sq1 = 0u;
    uint32_t r0, r1;
    int32_t max_d = 0;
    int32_t prev = 0;

    pop_verify_setup_plate(plate, POP_LEAD_TIMB_DEF[era]);
    g.synth.siren_hp_spp = DUB_FS / 8u;
    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 4u, 0);
    dub_siren_trig(&g.synth, 110u, 0, 0, 64u, era);
    for (i = 0u; i < n2; i++) {
        int32_t sig = pop_lead_render(&g.synth, &g.voice.p, 0);
        int32_t a = sig < 0 ? -sig : sig;
        int32_t d;

        if (i > 0u) {
            d = sig - prev;
            if (d < 0)
                d = -d;
            if (d > max_d)
                max_d = d;
        }
        prev = sig;
        if (i < n0)
            sq0 += (uint64_t)((int64_t)a * (int64_t)a);
        else if (i >= n1)
            sq1 += (uint64_t)((int64_t)a * (int64_t)a);
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    r0 = pop_v_isqrt64(sq0 / (uint64_t)(n0 ? n0 : 1u));
    r1 = pop_v_isqrt64(sq1 / (uint64_t)((n2 - n1) ? (n2 - n1) : 1u));
    (void)max_d;
    return r1 ? (r0 * 100u) / r1 : 0u;
}

static void pop_verify_onset_det(uint8_t plate, uint32_t *ratio, uint32_t *pk_early, uint32_t *pk_late,
                                 int32_t *max_d)
{
    uint8_t era = pop_era_of_plate(plate);
    uint32_t i;
    uint32_t n0 = (DUB_FS * 10u) / 1000u;
    uint32_t n1 = (DUB_FS * 50u) / 1000u;
    uint32_t n2 = (DUB_FS * 100u) / 1000u;
    uint64_t sq0 = 0u, sq1 = 0u;
    int32_t prev = 0;
    int32_t md = 0;

    pop_verify_setup_plate(plate, POP_LEAD_TIMB_DEF[era]);
    g.synth.siren_hp_spp = DUB_FS / 8u;
    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 4u, 0);
    dub_siren_trig(&g.synth, 110u, 0, 0, 64u, era);
    for (i = 0u; i < n2; i++) {
        int32_t sig = pop_lead_render(&g.synth, &g.voice.p, 0);
        int32_t a = sig < 0 ? -sig : sig;
        int32_t d;

        if (i > 0u) {
            d = sig - prev;
            if (d < 0)
                d = -d;
            if (d > md)
                md = d;
        }
        prev = sig;
        if (i < n0)
            sq0 += (uint64_t)((int64_t)a * (int64_t)a);
        else if (i >= n1)
            sq1 += (uint64_t)((int64_t)a * (int64_t)a);
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    *pk_early = pop_v_isqrt64(sq0 / (uint64_t)(n0 ? n0 : 1u));
    *pk_late = pop_v_isqrt64(sq1 / (uint64_t)((n2 - n1) ? (n2 - n1) : 1u));
    *ratio = *pk_late ? (*pk_early * 100u) / *pk_late : 0u;
    *max_d = md;
}

static uint32_t pop_lead_span_steps(uint8_t plate, uint8_t era, uint8_t mbar, uint8_t s16)
{
    uint32_t span = 1u;
    uint8_t b = mbar;
    uint8_t st = s16;

    while (span < 128u) {
        st++;
        if (st >= 16u) {
            st = 0u;
            b++;
            if (b >= 16u)
                break;
        }
        if (pop_melody_step_is_rest(plate, b, st))
            break;
        {
            dub_events_t ev2;

            FM_DUB_RESOLVE_STEP(&g.cfg.p, 0, st, &ev2, 1u);
            pop_arrange_events(plate, era, b, 0u, &ev2);
            if (ev2.siren)
                break;
        }
        span++;
    }
    return span;
}

static uint32_t pop_verify_silence_seq(uint8_t plate, uint32_t *lead_notes, uint8_t *win_mbar0)
{
    uint8_t era = pop_era_of_plate(plate);
    uint32_t quiet = 0u;
    uint32_t total = 0u;
    uint32_t notes = 0u;
    int32_t pk = 1;
    int32_t thr;
    uint32_t spp;
    uint8_t mbar;
    uint8_t s16;
    uint8_t m0;
    uint8_t m1;
    uint8_t pass;

    m0 = pop_lead_arrange_bar0(plate, era);
    m1 = (uint8_t)(m0 + 8u);
    if (m1 > 16u)
        m1 = 16u;
    if (win_mbar0)
        *win_mbar0 = m0;

    for (pass = 0u; pass < 2u; pass++) {
        pop_preset_rebuild(&g.cfg.p, era, plate);
        g.cfg.p.drm_dens[3] = POP_LEAD_TIMB_DEF[era];
        sync_voice_params();
        g.synth.siren_env = 0;
        g.synth.siren_gate = 0u;
        g.synth.siren_lead_rel = 0u;
        g.synth.siren_atk_left = 0u;
        g.synth.pop_mel_hold = -1;
        g.synth.voice_plate = plate;
        spp = g.synth.siren_hp_spp;
        if (spp < 220u)
            spp = (DUB_FS * 60u) / ((uint32_t)g.voice.p.bpm * 4u * (uint32_t)DUB_STEPS);
        if (pass == 1u) {
            thr = pk / 100;
            if (thr < 8)
                thr = 8;
            quiet = 0u;
            total = 0u;
            notes = 0u;
        }

        for (mbar = m0; mbar < m1; mbar++) {
            if (pop_arrange_mute(plate, era, mbar, 3u, 0u))
                continue;
            for (s16 = 0u; s16 < 16u; s16++) {
                dub_events_t ev;
                uint32_t si;

                FM_DUB_RESOLVE_STEP(&g.cfg.p, 0, s16, &ev, 1u);
                pop_arrange_events(plate, era, mbar, 0u, &ev);
                if (pass == 1u && ev.siren && !pop_melody_step_is_rest(plate, mbar, s16)) {
                    int8_t hold = -1;
                    int8_t mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16,
                                                      &hold);
                    uint32_t span = pop_lead_span_steps(plate, era, mbar, s16);

                    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, span, 0);
                    dub_siren_trig(&g.synth, 110u, mel, 0, 64u, era);
                    notes++;
                } else if (pass == 0u && ev.siren && !pop_melody_step_is_rest(plate, mbar, s16)) {
                    int8_t hold = -1;
                    int8_t mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16,
                                                      &hold);
                    uint32_t span = pop_lead_span_steps(plate, era, mbar, s16);

                    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, span, 0);
                    dub_siren_trig(&g.synth, 110u, mel, 0, 64u, era);
                }
                for (si = 0u; si < spp; si++) {
                    int32_t sig = pop_lead_render(&g.synth, &g.voice.p, 0);
                    int32_t a = sig < 0 ? -sig : sig;

                    if (pass == 0u) {
                        if (a > pk)
                            pk = a;
                    } else {
                        total++;
                        if (a < thr)
                            quiet++;
                    }
                    pop_siren_env_step(&g.synth, &g.voice.p);
                }
            }
        }
    }
    if (lead_notes)
        *lead_notes = notes;
    if (!notes || pk < 64)
        return 0u;
    return total ? (quiet * 100u) / total : 0u;
}

static void pop_verify_articulation(void)
{
    const char *eras[] = { "COLD", "RADIO", "CHROME", "ITALO", "BLUE", "AFTER" };
    uint8_t era;
    uint32_t sil_after = 0u;

    for (era = 0u; era < 6u; era++) {
        uint8_t p0 = pop_era_first_plate(era);
        uint8_t plate = p0;
        uint32_t lead_notes = 0u;
        uint8_t win_m0 = 0u;
        uint32_t sil_seq;
        uint8_t pi;

        for (pi = 0u; pi < GENRE_PRESETS_PER_ERA; pi++) {
            uint8_t pl = (uint8_t)(p0 + pi);
            uint32_t n = 0u;
            uint8_t w = 0u;

            pop_verify_silence_seq(pl, &n, &w);
            if (n > lead_notes) {
                lead_notes = n;
                plate = pl;
                win_m0 = w;
            }
        }
        sil_seq = pop_verify_silence_seq(plate, &lead_notes, &win_m0);
        uint32_t sus = pop_verify_amp_sus_pct(plate);
        uint8_t tex = pop_lead_texture(era, pop_lead_variant(plate));
        uint32_t gate_nom = tex == POP_LEAD_TEX_CHOIR ? 80u :
                            (tex == POP_LEAD_TEX_STRING ? 76u : (era == ERA_TINE || era == ERA_BELL ? 44u : 56u));

        sil_after = sil_seq;
        printf("ARTIC %s sil_seq8=%u lead_notes=%u win_mbar=%u amp_sus_pct=%u gate_nom=%u\n", eras[era], sil_seq,
               lead_notes, win_m0, sus, gate_nom);
    }
    {
        uint32_t r, e, l;
        int32_t md;

        pop_verify_onset_det(0u, &r, &e, &l, &md);
        printf("ONSET SYN ratio=%u pk0=%u pk1=%u max_d=%d\n", r, e, l, md);
        pop_verify_onset_det(2u, &r, &e, &l, &md);
        printf("ONSET STR ratio=%u pk0=%u pk1=%u max_d=%d\n", r, e, l, md);
        pop_verify_onset_det(40u, &r, &e, &l, &md);
        printf("ONSET CHR ratio=%u pk0=%u pk1=%u max_d=%d\n", r, e, l, md);
    }
    printf("ARTIC_SEQ_SUM last_sil_seq8=%u\n", sil_after);
}

static void pop_verify_bench(void)
{
    const char *tn[] = { "SYNTH", "STRING", "CHOIR" };
    uint32_t N = 80000u;
    uint8_t tex;

    for (tex = 0u; tex < 3u; tex++) {
        uint8_t era = (tex == POP_LEAD_TEX_CHOIR) ? ERA_PAD : (tex == POP_LEAD_TEX_STRING ? ERA_BASS : ERA_HORN);
        uint8_t p0 = pop_era_first_plate(era);
        uint8_t plate = p0;
        uint8_t pi;
        uint64_t t0, t1;
        uint32_t i;

        for (pi = 0u; pi < 10u; pi++) {
            if (pop_lead_texture(era, pop_lead_variant((uint8_t)(p0 + pi))) == tex) {
                plate = (uint8_t)(p0 + pi);
                break;
            }
        }
        pop_preset_rebuild(&g.cfg.p, era, plate);
        g.cfg.p.drm_dens[3] = POP_LEAD_TIMB_DEF[era];
        sync_voice_params();
        g.synth.pop_lead_note = 72u;
        g.synth.siren_env = 28000;
        g.synth.pop_lead_fenv = 12000;
        t0 = pop_v_ns();
        for (i = 0u; i < N; i++)
            (void)pop_lead_render(&g.synth, &g.voice.p, 0);
        t1 = pop_v_ns();
        printf("BENCH_LEAD_%s ns=%llu\n", tn[tex], (unsigned long long)((t1 - t0) / N));
    }
    {
        uint64_t t0, t1;
        uint32_t i;

        pop_verify_setup(ERA_HORN, 64u);
        g.synth.ch_env = 28000;
        g.synth.ch_ph[0] = 0x10000000u;
        t0 = pop_v_ns();
        for (i = 0u; i < N; i++)
            (void)pop_chord_render(&g.synth, &g.voice.p, (int32_t)g.voice.p.root, 4, 64, 0, g.synth.ch_env);
        t1 = pop_v_ns();
        printf("BENCH_CHORD ns=%llu\n", (unsigned long long)((t1 - t0) / N));
        t0 = pop_v_ns();
        g.synth.sub_env = 28000;
        g.synth.sub_on = 1u;
        for (i = 0u; i < N; i++)
            (void)pop_bass_render(&g.synth, &g.voice.p, dub_note_inc(36), 2000, 0, 0, 0);
        t1 = pop_v_ns();
        printf("BENCH_BASS ns=%llu\n", (unsigned long long)((t1 - t0) / N));
    }
}

static void pop_verify_scale_plate(uint8_t plate, uint32_t *hits, uint32_t *fix_ok, uint32_t *chord_ok,
                                   uint32_t *pc_fix)
{
    uint8_t era = pop_era_of_plate(plate);
    uint8_t mbar, s16;
    int8_t hold = -1;

    pop_preset_rebuild(&g.cfg.p, era, plate);
    sync_voice_params();
    for (mbar = 0u; mbar < 16u; mbar++) {
        for (s16 = 0u; s16 < 16u; s16++) {
            dub_events_t ev;
            int32_t pre, post;
            uint8_t tones[4];
            uint8_t i;
            uint8_t on_ch = 0u;

            dub_resolve_step(&g.cfg.p, 0, s16, &ev, 0u);
            pop_arrange_events(plate, era, mbar, 0u, &ev);
            if (!ev.siren)
                continue;
            {
                int8_t mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16, &hold);

                pre = pop_lead_resolve_midi_core(&g.voice.p, plate, mel, 0, 1u);
                post = pop_lead_resolve_midi_step(&g.voice.p, plate, mbar, s16, mel, 0, 1u);
                (*hits)++;
                if (pop_midi_in_scale(g.voice.p.root, g.voice.p.scale, post))
                    (*fix_ok)++;
                pop_chord_voices(plate, g.voice.p.root, g.voice.p.scale, mbar, g.voice.p.voicing % VOICE_N,
                                 tones);
                for (i = 0u; i < 4u; i++) {
                    if (tones[i] == (uint8_t)post)
                        on_ch = 1u;
                }
                if ((s16 & 3u) != 0u || on_ch)
                    (*chord_ok)++;
                if (((uint8_t)pre % 12u) != ((uint8_t)post % 12u))
                    (*pc_fix)++;
            }
        }
    }
}

static void pop_verify_scale(void)
{
    const char *eras[] = { "COLD", "RADIO", "CHROME", "ITALO", "BLUE", "AFTER" };
    uint8_t era;
    uint32_t sweep_h = 0u, sweep_ok = 0u;

    {
        uint8_t plate = 0u;
        uint8_t root = pop_plate_root(plate);
        uint8_t scale = pop_plate_scale(plate);
        int32_t rel57 = 57 - (int32_t)root;
        int32_t rel65 = 65 - (int32_t)root;

        while (rel57 < 0)
            rel57 += 12;
        while (rel65 < 0)
            rel65 += 12;
        rel57 %= 12;
        rel65 %= 12;
        printf("SCALE_CHK bug=no root=%u scale=%u note57_rel=%d in57=%u note65_rel=%d in65=%u "
               "(0%% was siren-60 semitone not octave)\n",
               root, scale, (int)rel57, pop_midi_in_scale(root, scale, 57),
               (int)rel65, pop_midi_in_scale(root, scale, 65));
    }
    for (era = 0u; era < 6u; era++) {
        uint8_t plate = pop_era_first_plate(era);
        uint32_t hits = 0u, fix_ok = 0u, chord_ok = 0u, pc_fix = 0u;

        pop_verify_scale_plate(plate, &hits, &fix_ok, &chord_ok, &pc_fix);
        printf("SCALE %s fix_pct=%u chord_pct=%u pitch_fix=%u hits=%u\n", eras[era],
               hits ? (fix_ok * 100u) / hits : 100u, hits ? (chord_ok * 100u) / hits : 100u, pc_fix, hits);
    }
    for (era = 0u; era < POP_PLATE_N; era++) {
        uint32_t h = 0u, ok = 0u, ch = 0u, pf = 0u;

        pop_verify_scale_plate(era, &h, &ok, &ch, &pf);
        sweep_h += h;
        sweep_ok += ok;
        (void)pf;
        (void)ch;
    }
    printf("SCALE_SWEEP60 pct=%u hits=%u\n", sweep_h ? (sweep_ok * 100u) / sweep_h : 100u, sweep_h);
}

static void pop_verify_kazoo(void)
{
    const char *eras[] = { "COLD", "RADIO", "CHROME", "ITALO", "BLUE", "AFTER" };
    uint8_t era;

    for (era = 0u; era < 6u; era++) {
        uint32_t d0, d127, r0, r127;
        uint32_t hf_x100;

        pop_verify_setup(era, 0u);
        pop_verify_metrics(era, 0u, &d0, &r0);
        pop_verify_setup(era, 127u);
        pop_verify_metrics(era, 127u, &d127, &r127);
        hf_x100 = r127 ? (d127 * 100u) / r127 : 0u;
        printf("KAZOO %s reso=%u diff0=%u diff127=%u hf_x100=%u timb_def=%u\n", eras[era],
               (unsigned)POP_LEAD_RESONANCE[era], d0, d127, hf_x100, (unsigned)POP_LEAD_TIMB_DEF[era]);
    }
}

static void pop_verify_vowel(void)
{
    uint8_t era = ERA_PAD;
    uint8_t p0 = pop_era_first_plate(era);
    uint8_t plate = p0;
    uint8_t pi;
    uint32_t r0, r64, r127;

    for (pi = 0u; pi < 10u; pi++) {
        if (pop_lead_texture(era, pop_lead_variant((uint8_t)(p0 + pi))) == POP_LEAD_TEX_CHOIR) {
            plate = (uint8_t)(p0 + pi);
            break;
        }
    }
    {
        uint32_t d0, d64, d127;

        pop_verify_setup_plate(plate, 0u);
        pop_verify_metrics(era, 0u, &d0, &r0);
        pop_verify_setup_plate(plate, 64u);
        pop_verify_metrics(era, 64u, &d64, &r64);
        pop_verify_setup_plate(plate, 127u);
        pop_verify_metrics(era, 127u, &d127, &r127);
        (void)d0;
        (void)d64;
        (void)d127;
    }
    {
        uint32_t mx = r0, mn = r0;

        if (r64 > mx)
            mx = r64;
        if (r127 > mx)
            mx = r127;
        if (r64 < mn)
            mn = r64;
        if (r127 < mn)
            mn = r127;
        printf("VOWEL CHOIR plate=%u rms0=%u rms64=%u rms127=%u swing_db_x10=%u\n", plate, r0, r64, r127,
               mn ? (uint32_t)((2000ull * (uint64_t)mx) / (uint64_t)mn) : 0u);
    }
}

static void pop_verify_timbre_level(uint8_t plate, const char *tag)
{
    uint8_t era = pop_era_of_plate(plate);
    uint32_t r0, r64, r127, d0;
    uint32_t mx, mn;

    pop_verify_setup_plate(plate, 0u);
    pop_verify_metrics(era, 0u, &d0, &r0);
    pop_verify_setup_plate(plate, 64u);
    pop_verify_metrics(era, 64u, &d0, &r64);
    pop_verify_setup_plate(plate, 127u);
    pop_verify_metrics(era, 127u, &d0, &r127);
    mx = r0;
    mn = r0;
    if (r64 > mx)
        mx = r64;
    if (r127 > mx)
        mx = r127;
    if (r64 < mn)
        mn = r64;
    if (r127 < mn)
        mn = r127;
    printf("TIMBRE_LVL %s plate=%u swing_db_x10=%u\n", tag, plate, mn ? (uint32_t)((2000ull * (uint64_t)mx) / (uint64_t)mn) : 0u);
}

static void pop_verify_map(void)
{
    const char *eras[] = { "COLD", "RADIO", "CHROME", "ITALO", "BLUE", "AFTER" };
    const char *tn[] = { "SYN", "STR", "CHR" };
    uint8_t era;

    for (era = 0u; era < 6u; era++) {
        uint8_t v;
        uint8_t tex[4];

        for (v = 0u; v < 4u; v++) {
            uint8_t t = pop_lead_texture(era, v);

            tex[v] = t;
        }
        printf("MAP %s tex=%s/%s/%s/%s\n", eras[era], tn[tex[0]], tn[tex[1]], tn[tex[2]], tn[tex[3]]);
    }
}

static void pop_verify_cold_melody(void)
{
    uint8_t plate = 0u;
    uint8_t era = pop_era_of_plate(plate);
    uint8_t mbar, s16;
    int8_t hold = -1;
    uint8_t mel_seen[256];
    uint8_t deg_seen[8];
    uint8_t pc_seen[12];
    uint32_t hits = 0u;
    uint8_t uniq_mel = 0u, uniq_deg = 0u, uniq_pc = 0u;

    for (s16 = 0u; s16 < 16u; s16++)
        mel_seen[s16] = 0u;
    for (mbar = 0u; mbar < 8u; mbar++)
        deg_seen[mbar] = 0u;
    for (s16 = 0u; s16 < 12u; s16++)
        pc_seen[s16] = 0u;

    pop_preset_rebuild(&g.cfg.p, era, plate);
    sync_voice_params();
    for (mbar = 0u; mbar < 16u; mbar++) {
        for (s16 = 0u; s16 < 16u; s16++) {
            dub_events_t ev;
            int8_t mel;
            int8_t row_deg;
            int8_t hold_row = -1;
            uint8_t note;
            uint8_t mi;

            dub_resolve_step(&g.cfg.p, 0, s16, &ev, 0u);
            pop_arrange_events(plate, era, mbar, 0u, &ev);
            if (!ev.siren)
                continue;
            row_deg = pop_melody_row_deg(plate, mbar, s16, &hold_row);
            mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16, &hold);
            note = pop_lead_play_midi(&g.voice.p, plate, mbar, s16, mel, 0);
            mi = (uint8_t)(mel + 128);
            hits++;
            if (!mel_seen[mi]) {
                mel_seen[mi] = 1u;
                uniq_mel++;
            }
            if (row_deg >= 0 && row_deg < 8 && !deg_seen[row_deg]) {
                deg_seen[row_deg] = 1u;
                uniq_deg++;
            }
            if (!pc_seen[note % 12u]) {
                pc_seen[note % 12u] = 1u;
                uniq_pc++;
            }
        }
    }
    printf("COLD_MELO plate=0 hits=%u uniq_mel_semi=%u uniq_row_deg=%u uniq_pc=%u "
           "(row=A:deg0@0/4/8+deg2@12; one_pc=chord_snap+sparse_gates)\n",
           hits, uniq_mel, uniq_deg, uniq_pc);
}

static void pop_verify_melody_pc(void)
{
    static const uint8_t POP_SIREN_LEGACY[POP_ERA_N] = { 52u, 58u, 68u, 64u, 40u, 56u };
    uint8_t era;
    uint32_t diffs = 0u;
    uint32_t steps = 0u;

    for (era = 0u; era < 6u; era++) {
        uint8_t plate = pop_era_first_plate(era);
        uint8_t mbar, s16;
        int8_t hold = -1;
        dub_params_t leg;

        pop_preset_rebuild(&g.cfg.p, era, plate);
        sync_voice_params();
        leg = g.voice.p;
        for (mbar = 0u; mbar < 16u; mbar++) {
            for (s16 = 0u; s16 < 16u; s16++) {
                dub_events_t ev;

                dub_resolve_step(&g.cfg.p, 0, s16, &ev, 0u);
                pop_arrange_events(plate, era, mbar, 0u, &ev);
                if (!ev.siren)
                    continue;
                {
                    int8_t mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16, &hold);
                    uint8_t a, b;

                    leg.siren_pitch = POP_SIREN_LEGACY[era];
                    a = (uint8_t)pop_lead_resolve_midi(&leg, plate, mel, 0, 0u);
                    b = (uint8_t)pop_lead_resolve_midi(&g.voice.p, plate, mel, 0, 1u);
                    steps++;
                    if ((a % 12u) != (b % 12u))
                        diffs++;
                }
            }
        }
    }
    printf("MELODY_PC_DIFF count=%u steps=%u\n", diffs, steps);
}

static void pop_verify_lead_reset_voice(uint8_t note)
{
    g.synth.siren_env = 28000;
    g.synth.siren_atk_left = 0u;
    g.synth.siren_gate = 65535u;
    g.synth.pop_lead_fenv = 12000;
    g.synth.siren_ph = 0x24000000u;
    g.synth.pop_lead_inc = dub_note_inc(note);
    g.synth.pop_lead_tgt_inc = g.synth.pop_lead_inc;
    g.synth.pop_lead_lp = 0;
    g.synth.pop_lead_bp = 0;
    g.synth.pop_lead_lp2 = 0;
    g.synth.pop_lead_bp2 = 0;
    g.synth.pop_lead_f1_lp = 0;
    g.synth.pop_lead_f1_bp = 0;
    g.synth.pop_lead_f2_lp = 0;
    g.synth.pop_lead_f2_bp = 0;
    g.synth.pop_lead_det_ph[0] = 0x10000000u;
    g.synth.pop_lead_det_ph[1] = 0x30000000u;
}

static uint32_t pop_verify_timbre_chg_pct(uint8_t era, uint8_t plate, uint8_t t0, uint8_t t1)
{
    static int32_t wave_a[2048];
    static int32_t wave_b[2048];
    uint32_t i, n = DUB_FS / 16u;
    uint64_t sq0 = 0u, sqd = 0u;
    uint32_t r0, rd;

    (void)era;
    if (n > 2048u)
        n = 2048u;
    pop_verify_setup_plate(plate, t0);
    pop_verify_lead_reset_voice(72u);
    for (i = 0u; i < n; i++) {
        wave_a[i] = pop_lead_render(&g.synth, &g.voice.p, 0);
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    pop_verify_setup_plate(plate, t1);
    pop_verify_lead_reset_voice(72u);
    for (i = 0u; i < n; i++)
        wave_b[i] = pop_lead_render(&g.synth, &g.voice.p, 0);
    for (i = 0u; i < n; i++) {
        int64_t d = (int64_t)wave_b[i] - (int64_t)wave_a[i];
        int64_t a = (int64_t)wave_a[i];

        sqd += (uint64_t)(d * d);
        sq0 += (uint64_t)(a * a);
    }
    if (sq0 == 0u)
        return 0u;
    r0 = pop_v_isqrt64(sq0 / (uint64_t)n);
    rd = pop_v_isqrt64(sqd / (uint64_t)n);
    return r0 ? (rd * 100u) / r0 : 0u;
}

static void pop_verify_variant_bench(uint8_t era, uint8_t var, uint8_t plate)
{
    const char *tn[] = { "SYNTH", "STRING", "CHOIR" };
    uint32_t d0, r64, sus, diff_chg;
    uint64_t t0, t1;
    uint32_t i;
    uint32_t N = 40000u;
    uint8_t tex = pop_lead_texture(era, var);

    diff_chg = pop_verify_timbre_chg_pct(era, plate, 0u, 127u);
    pop_verify_setup_plate(plate, POP_LEAD_TIMB_DEF[era]);
    pop_verify_metrics(era, POP_LEAD_TIMB_DEF[era], &d0, &r64);
    sus = pop_verify_amp_sus_pct(plate);
    g.synth.siren_env = 28000;
    g.synth.pop_lead_fenv = 12000;
    t0 = pop_v_ns();
    for (i = 0u; i < N; i++)
        (void)pop_lead_render(&g.synth, &g.voice.p, 0);
    t1 = pop_v_ns();
    printf("PLATE %s var=%u tex=%s plate=%u ns=%llu diff_chg=%u rms64=%u amp_sus_pct=%u\n",
           POP_ERA_NAMES[era], var, tn[tex], plate, (unsigned long long)((t1 - t0) / N), diff_chg, r64, sus);
}

static uint8_t pop_verify_median_u8(uint8_t *v, uint32_t n)
{
    uint32_t i, j;

    for (i = 1u; i < n; i++) {
        uint8_t k = v[i];

        for (j = i; j > 0u && v[j - 1u] > k; j--)
            v[j] = v[j - 1u];
        v[j] = k;
    }
    return v[n / 2u];
}

static void pop_verify_register(void)
{
    static const uint8_t POP_SIREN_LEGACY[POP_ERA_N] = { 52u, 58u, 68u, 64u, 40u, 56u };
    static const int8_t POP_OCT_LFO_FULL = 59;
    const char *eras[] = { "COLD", "RADIO", "CHROME", "ITALO", "BLUE", "AFTER" };
    uint8_t era;

    for (era = 0u; era < 6u; era++) {
        uint8_t plate = pop_era_first_plate(era);
        uint8_t mbar, s16;
        int8_t hold = -1;
        uint8_t before[512];
        uint8_t after[512];
        uint8_t oct_samp[1536];
        uint32_t nh = 0u, noct = 0u;
        uint8_t bmin = 127u, bmax = 0u, amin = 127u, amax = 0u;
        uint8_t omin = 127u, omax = 0u;
        uint8_t bmed, amed, omed;
        dub_params_t leg;
        int32_t siren_delta = (int32_t)POP_ERA_BASE[era].siren_pitch - (int32_t)POP_SIREN_LEGACY[era];
        int32_t var_shift = (int32_t)POP_LEAD_VAR_REG_SHIFT[era][pop_lead_variant(plate)];

        pop_preset_rebuild(&g.cfg.p, era, plate);
        sync_voice_params();
        for (mbar = 0u; mbar < 16u; mbar++) {
            for (s16 = 0u; s16 < 16u; s16++) {
                dub_events_t ev;
                uint8_t n0, n1, nu, nd;

                dub_resolve_step(&g.cfg.p, 0, s16, &ev, 0u);
                pop_arrange_events(plate, era, mbar, 0u, &ev);
                if (!ev.siren)
                    continue;
                {
                    int8_t mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16, &hold);

                    leg = g.voice.p;
                    leg.siren_pitch = POP_SIREN_LEGACY[era];
                    n0 = (uint8_t)pop_lead_resolve_midi(&leg, plate, mel, 0, 0u);
                    n1 = (uint8_t)pop_lead_resolve_midi_step(&g.voice.p, plate, mbar, s16, mel, 0, 1u);
                    nu = (uint8_t)pop_lead_resolve_midi_step(&g.voice.p, plate, mbar, s16, mel, POP_OCT_LFO_FULL,
                                                              1u);
                    nd = (uint8_t)pop_lead_resolve_midi_step(&g.voice.p, plate, mbar, s16, mel,
                                                              (int8_t)(-POP_OCT_LFO_FULL), 1u);
                    if (nh < 512u) {
                        uint8_t lo, hi;

                        before[nh] = n0;
                        after[nh] = n1;
                        lo = n1;
                        hi = n1;
                        if (nu < lo)
                            lo = nu;
                        if (nd < lo)
                            lo = nd;
                        if (nu > hi)
                            hi = nu;
                        if (nd > hi)
                            hi = nd;
                        if (lo < omin)
                            omin = lo;
                        if (hi > omax)
                            omax = hi;
                        if (noct + 3u <= 1536u) {
                            oct_samp[noct++] = n1;
                            oct_samp[noct++] = nu;
                            oct_samp[noct++] = nd;
                        }
                        nh++;
                    }
                    if (n0 < bmin)
                        bmin = n0;
                    if (n0 > bmax)
                        bmax = n0;
                    if (n1 < amin)
                        amin = n1;
                    if (n1 > amax)
                        amax = n1;
                }
            }
        }
        if (nh == 0u) {
            printf("REG %s hits=0\n", eras[era]);
            continue;
        }
        bmed = pop_verify_median_u8(before, nh);
        amed = pop_verify_median_u8(after, nh);
        omed = noct ? pop_verify_median_u8(oct_samp, noct) : amed;
        printf("REG %s shift_siren=%d shift_var=%d before=%u/%u/%u after=%u/%u/%u oct_lfo=%u/%u/%u hits=%u\n",
               eras[era], (int)siren_delta, (int)var_shift, bmin, bmed, bmax, amin, amed, amax, omin, omed, omax,
               nh);
    }
}

static void pop_verify_melody(void)
{
    const char *eras[] = { "COLD", "RADIO", "CHROME", "ITALO", "BLUE", "AFTER" };
    uint8_t era;
    uint32_t diffs = 0u;
    uint32_t steps = 0u;

    for (era = 0u; era < 6u; era++) {
        uint8_t plate = pop_era_first_plate(era);
        uint8_t mbar, s16;
        int8_t hold = -1;
        int8_t hold_b = -1;

        pop_preset_rebuild(&g.cfg.p, era, plate);
        sync_voice_params();
        for (mbar = 0u; mbar < 16u; mbar++) {
            for (s16 = 0u; s16 < 16u; s16++) {
                dub_events_t ev;

                dub_resolve_step(&g.cfg.p, 0, s16, &ev, 0u);
                pop_arrange_events(plate, era, mbar, 0u, &ev);
                if (!ev.siren)
                    continue;
                {
                    int8_t mel = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16, &hold);
                    int8_t mel_b = pop_lead_melody_semi(plate, g.voice.p.root, g.voice.p.scale, mbar, s16,
                                                          &hold_b);
                    uint8_t a = pop_lead_play_midi(&g.voice.p, plate, mbar, s16, mel, 0);
                    uint8_t b = pop_lead_play_midi(&g.voice.p, plate, mbar, s16, mel_b, 0);

                    steps++;
                    if (a != b)
                        diffs++;
                    (void)eras;
                }
            }
        }
    }
    printf("MELODY_DIFF count=%u steps=%u\n", diffs, steps);
}

static uint32_t pop_rms_voice(uint8_t plate, uint8_t which)
{
    uint64_t sq = 0u;
    uint32_t i, n = DUB_FS / 4u;
    uint8_t era = pop_era_of_plate(plate);

    pop_preset_rebuild(&g.cfg.p, era, plate);
    sync_voice_params();
    g.synth.siren_env = 0;
    g.synth.siren_peak = 30000;
    g.synth.siren_gate = 0u;
    g.synth.siren_atk_left = 0u;
    g.synth.ch_env = 28000;
    g.synth.sub_env = 28000;
    g.synth.sub_on = 1u;
    g.synth.siren_st_tune = 0;
    g.synth.pop_lead_fenv = 0;
    g.synth.voice_plate = plate;
    if (which == 0u) {
        pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 4u, 0);
        dub_siren_trig(&g.synth, 120u, 0, 0, 64u, era);
    }
    for (i = 0u; i < n; i++) {
        int32_t sig = 0;

        if (which == 0u)
            sig = pop_lead_render(&g.synth, &g.voice.p, 0);
        else if (which == 1u)
            sig = pop_chord_render(&g.synth, &g.voice.p, (int32_t)g.voice.p.root, 4, 64, 0, g.synth.ch_env);
        else {
            sig = pop_bass_render(&g.synth, &g.voice.p, dub_note_inc(40), 2000, 0, 0, 0);
            sig = (sig * (int32_t)g.voice.p.lvl_b) / 96;
        }
        sq += (uint64_t)((int64_t)sig * (int64_t)sig);
        if (which == 0u)
            pop_siren_env_step(&g.synth, &g.voice.p);
    }
    return pop_v_isqrt64(sq / (uint64_t)n);
}

static uint32_t pop_db10_ratio(uint32_t num, uint32_t den)
{
    if (!den || !num)
        return 0u;
    return (uint32_t)((2000ull * (uint64_t)num) / (uint64_t)den);
}

static int32_t pop_db_centi(uint32_t num, uint32_t den)
{
    if (!den || !num)
        return -9999;
    return (int32_t)(2000.0 * log10((double)num / (double)den));
}

static const uint8_t POP_VERIFY_REP_PLATE[POP_ERA_N] = { 0u, 10u, 22u, 31u, 40u, 52u };

static void pop_verify_arm_mix(uint8_t plate)
{
    uint8_t era = pop_era_of_plate(plate);

    pop_preset_rebuild(&g.cfg.p, era, plate);
    sync_voice_params();
    g.voice.mute = 0u;
    g.synth.siren_env = 0;
    g.synth.siren_peak = 30000;
    g.synth.siren_gate = 0u;
    g.synth.siren_atk_left = 0u;
    g.synth.ch_env = 28000;
    g.synth.sub_env = 28000;
    g.synth.sub_on = 1u;
    g.synth.voice_plate = plate;
    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 4u, 0);
    dub_siren_trig(&g.synth, 120u, 0, 0, 64u, era);
}

static uint32_t pop_verify_bass_hf800_pct(uint8_t plate, uint8_t legacy)
{
    uint32_t i, n = DUB_FS / 4u;
    uint64_t sq_b = 0u, sq_hf = 0u;
    int32_t lp = 0;
    uint32_t rms_b, hf_pct;

    pop_verify_bass_no_ix12 = legacy;
    pop_verify_bass_legacy_lp = legacy;
    pop_verify_arm_mix(plate);
    for (i = 0u; i < n; i++) {
        int32_t bass = pop_bass_render(&g.synth, &g.voice.p, dub_note_inc(52), 2000, 0, 0, 0);
        int32_t hf;

        bass = (bass * (int32_t)g.voice.p.lvl_b) / 96;
        lp += ((bass - lp) * 13) >> 7;
        hf = bass - lp;
        sq_b += (uint64_t)((int64_t)bass * (int64_t)bass);
        sq_hf += (uint64_t)((int64_t)hf * (int64_t)hf);
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    pop_verify_bass_no_ix12 = 0u;
    pop_verify_bass_legacy_lp = 0u;
    rms_b = pop_v_isqrt64(sq_b / (uint64_t)n);
    hf_pct = rms_b ? (pop_v_isqrt64(sq_hf / (uint64_t)n) * 100u) / rms_b : 0u;
    return hf_pct;
}

static void pop_verify_balance_plate(uint8_t plate)
{
    uint32_t i, n = DUB_FS / 4u;
    uint64_t sq_l = 0u, sq_c = 0u, sq_b = 0u;
    uint32_t rms_l, rms_c, rms_b, hf_pre, hf_post;

    pop_verify_arm_mix(plate);
    for (i = 0u; i < n; i++) {
        int32_t lead = pop_lead_render(&g.synth, &g.voice.p, 0);
        int32_t ch = pop_chord_render(&g.synth, &g.voice.p, (int32_t)g.voice.p.root, 4, 64, 0, g.synth.ch_env);
        int32_t bass = pop_bass_render(&g.synth, &g.voice.p, dub_note_inc(40), 2000, 0, 0, 0);

        bass = (bass * (int32_t)g.voice.p.lvl_b) / 96;
        ch = (ch * (int32_t)g.voice.p.lvl_c) / 96;
        sq_l += (uint64_t)((int64_t)lead * (int64_t)lead);
        sq_c += (uint64_t)((int64_t)ch * (int64_t)ch);
        sq_b += (uint64_t)((int64_t)bass * (int64_t)bass);
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    rms_l = pop_v_isqrt64(sq_l / (uint64_t)n);
    rms_c = pop_v_isqrt64(sq_c / (uint64_t)n);
    rms_b = pop_v_isqrt64(sq_b / (uint64_t)n);
    hf_pre = pop_verify_bass_hf800_pct(plate, 1u);
    hf_post = pop_verify_bass_hf800_pct(plate, 0u);
    printf("TBL plate=%u ldb_centi=%d ldc_centi=%d bass_hf8_pre=%u bass_hf8_post=%u\n", plate, pop_db_centi(rms_l, rms_b),
           pop_db_centi(rms_l, rms_c), hf_pre, hf_post);
}

static void pop_verify_balance_table(void)
{
    uint8_t era;

    for (era = 0u; era < POP_ERA_N; era++) {
        uint8_t p0 = pop_era_first_plate(era);
        uint8_t var;
        uint8_t nout = 0u;

        pop_verify_balance_plate(POP_VERIFY_REP_PLATE[era]);
        for (var = 0u; var < 4u && nout < 2u; var++) {
            uint8_t pi;
            uint8_t plate = p0;

            for (pi = 0u; pi < GENRE_PRESETS_PER_ERA; pi++) {
                uint8_t pl = (uint8_t)(p0 + pi);

                if (pop_lead_variant(pl) == var && pl != POP_VERIFY_REP_PLATE[era]) {
                    plate = pl;
                    pop_verify_balance_plate(plate);
                    nout++;
                    break;
                }
            }
        }
    }
}

static void pop_verify_out_path(void)
{
    static int32_t buf[2u * (DUB_FS / 4u)];
    uint8_t era;
    const int32_t fs_ref = 32767;
    const int32_t mst = 2048;

    for (era = 0u; era < POP_ERA_N; era++) {
        uint8_t plate = POP_VERIFY_REP_PLATE[era];
        uint32_t i, n = DUB_FS / 4u;
        int32_t pk_dsp = 0;
        int32_t pk_emu = 0;
        uint32_t clip_emu = 0u;

        pop_verify_arm_mix(plate);
        for (i = 0u; i < 2u * n; i++)
            buf[i] = 0;
        dub_synth_render(&g.synth, &g.voice, buf, n);
        for (i = 0u; i < n; i++) {
            int32_t l = buf[2u * i];
            int32_t r = buf[2u * i + 1u];
            int32_t al = l < 0 ? -l : l;
            int32_t ar = r < 0 ? -r : r;
            int32_t el = (l * mst) >> 12;
            int32_t er = (r * mst) >> 12;

            if (al > pk_dsp)
                pk_dsp = al;
            if (ar > pk_dsp)
                pk_dsp = ar;
            if (el > fs_ref) {
                clip_emu++;
                el = fs_ref;
            } else if (el < -fs_ref) {
                clip_emu++;
                el = -fs_ref;
            }
            if (er > fs_ref) {
                clip_emu++;
                er = fs_ref;
            } else if (er < -fs_ref) {
                clip_emu++;
                er = -fs_ref;
            }
            al = el < 0 ? -el : el;
            ar = er < 0 ? -er : er;
            if (al > pk_emu)
                pk_emu = al;
            if (ar > pk_emu)
                pk_emu = ar;
        }
        printf("OUT %s plate=%u pk_dsp=%d pk_emu=%d clip_emu=%u (BAL_sum_peak is pre-master int32 sum, not this)\n",
               POP_ERA_NAMES[era], plate, pk_dsp, pk_emu, clip_emu);
    }
}

static void pop_verify_mix(uint8_t plate)
{
    uint32_t rms_lead = pop_rms_voice(plate, 0u);
    uint32_t rms_ch = pop_rms_voice(plate, 1u);
    uint32_t rms_bass = pop_rms_voice(plate, 2u);

    printf("MIX plate=%u lead_rms=%u chord_rms=%u bass_rms=%u lead_over_chord_db_x10=%u\n", plate, rms_lead,
           rms_ch, rms_bass, pop_db10_ratio(rms_lead, rms_ch));
}

static void pop_verify_balance_era(uint8_t era)
{
    uint8_t plate = pop_era_first_plate(era);
    uint32_t i, n = DUB_FS / 4u;
    uint64_t sq_l = 0u, sq_c = 0u, sq_b = 0u, sq_hf = 0u;
    int32_t pk = 0;
    int32_t lp = 0;
    uint32_t rms_l, rms_c, rms_b, hf_pct;

    pop_preset_rebuild(&g.cfg.p, era, plate);
    sync_voice_params();
    g.synth.siren_env = 0;
    g.synth.siren_peak = 30000;
    g.synth.siren_gate = 0u;
    g.synth.siren_atk_left = 0u;
    g.synth.ch_env = 28000;
    g.synth.sub_env = 28000;
    g.synth.sub_on = 1u;
    g.synth.voice_plate = plate;
    pop_lead_arm_trig_gate(&g.synth, &g.voice.p, plate, 4u, 0);
    dub_siren_trig(&g.synth, 120u, 0, 0, 64u, era);
    for (i = 0u; i < n; i++) {
        int32_t lead = pop_lead_render(&g.synth, &g.voice.p, 0);
        int32_t ch = pop_chord_render(&g.synth, &g.voice.p, (int32_t)g.voice.p.root, 4, 64, 0, g.synth.ch_env);
        int32_t bass = pop_bass_render(&g.synth, &g.voice.p, dub_note_inc(40), 2000, 0, 0, 0);
        int32_t hf;
        int32_t mix;
        int32_t a;

        bass = (bass * (int32_t)g.voice.p.lvl_b) / 96;
        ch = (ch * (int32_t)g.voice.p.lvl_c) / 96;
        lp += ((bass - lp) * 19) >> 5;
        hf = bass - lp;
        sq_l += (uint64_t)((int64_t)lead * (int64_t)lead);
        sq_c += (uint64_t)((int64_t)ch * (int64_t)ch);
        sq_b += (uint64_t)((int64_t)bass * (int64_t)bass);
        sq_hf += (uint64_t)((int64_t)hf * (int64_t)hf);
        mix = lead + ch + bass;
        a = mix < 0 ? -mix : mix;
        if (a > pk)
            pk = a;
        pop_siren_env_step(&g.synth, &g.voice.p);
    }
    rms_l = pop_v_isqrt64(sq_l / (uint64_t)n);
    rms_c = pop_v_isqrt64(sq_c / (uint64_t)n);
    rms_b = pop_v_isqrt64(sq_b / (uint64_t)n);
    hf_pct = rms_b ? (pop_v_isqrt64(sq_hf / (uint64_t)n) * 100u) / rms_b : 0u;
    printf("BAL %s lead=%u bass=%u chord=%u ldb_x10=%u ldc_x10=%u bass_hf_pct=%u peak=%d lvl_b=%u\n",
           POP_ERA_NAMES[era], rms_l, rms_b, rms_c, pop_db10_ratio(rms_l, rms_b), pop_db10_ratio(rms_l, rms_c),
           hf_pct, pk, g.voice.p.lvl_b);
}

static void pop_verify_balance(void)
{
    uint8_t era;

    for (era = 0u; era < POP_ERA_N; era++)
        pop_verify_balance_era(era);
}

int pop_lead_verify_main(int argc, char **argv)
{
    uint8_t era;

    app_init();
    genre_pgen_off(); /* the boot song may shadow a ROM plate; the suite measures the curated plates */
    if (argc > 1 && !strcmp(argv[1], "bench")) {
        pop_verify_bench();
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "trace")) {
        pop_verify_scale();
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "melody")) {
        pop_verify_melody();
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "register")) {
        pop_verify_register();
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "mix")) {
        pop_verify_balance();
        pop_verify_mix(10u);
        pop_verify_mix(40u);
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "suite")) {
        pop_verify_map();
        pop_verify_register();
        pop_verify_melody();
        pop_verify_melody_pc();
        pop_verify_cold_melody();
        pop_verify_scale();
        pop_verify_kazoo();
        pop_verify_vowel();
        pop_verify_articulation();
        pop_verify_balance();
        pop_verify_balance_table();
        pop_verify_out_path();
        pop_verify_timbre_level(10u, "RADIO_SYN");
        pop_verify_timbre_level(2u, "COLD_STR");
        pop_verify_timbre_level(40u, "BLUE_CHR");
        pop_verify_bench();
        pop_verify_mix(10u);
        pop_verify_mix(40u);
        for (era = 0u; era < 6u; era++) {
            uint8_t p0 = pop_era_first_plate(era);
            uint8_t var;

            for (var = 0u; var < 4u; var++) {
                uint8_t pi;
                uint8_t plate = p0;
                uint8_t found = 0u;

                for (pi = 0u; pi < GENRE_PRESETS_PER_ERA; pi++) {
                    uint8_t pl = (uint8_t)(p0 + pi);

                    if (pop_lead_variant(pl) == var) {
                        plate = pl;
                        found = 1u;
                        break;
                    }
                }
                if (found)
                    pop_verify_variant_bench(era, var, plate);
            }
        }
        return 0;
    }
    return 1;
}
#endif
