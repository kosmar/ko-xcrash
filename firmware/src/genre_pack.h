/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef GENRE_PACK_H
#define GENRE_PACK_H

#ifdef GENRE_HEAVY_PSYCH
#include "heavy_psych_data.h"
#elif defined(GENRE_HOUSE)
#include "house_data.h"
#elif defined(GENRE_JAZZ)
#include "jazz_data.h"
#elif defined(GENRE_IDM)
#include "idm_data.h"
#elif defined(GENRE_HIPHOP)
#include "hiphop_data.h"
#elif defined(GENRE_SNOWCRASH)
#include "snowcrash_data.h"
#elif defined(GENRE_FUNK)
#include "funk_data.h"
#elif defined(GENRE_POP)
#include "pop_data.h"
#elif defined(GENRE_TECHNO)
#include "techno_data.h"
#elif defined(GENRE_TRIPHOP)
#include "triphop_data.h"
#elif defined(GENRE_SOUL)
#include "soul_data.h"
#elif defined(GENRE_BASS)
#include "bass_data.h"

#else
#include "dub_data.h"
#endif

#ifdef GENRE_HEAVY_PSYCH
#define genre_clamp_era(era)       heavy_psych_clamp_era(era)
#define genre_clamp_preset(ix)     heavy_psych_clamp_trax(ix)
#define genre_era_name(era)        heavy_psych_era_name(era)
#define genre_preset_name(ix)      heavy_psych_trax_name(ix)
#define genre_preset(ix)           heavy_psych_trax(ix)
#define genre_era_of_preset(pl)    heavy_psych_era_of_trax(pl)
#define genre_era_first_preset(er) heavy_psych_era_first_trax(er)
#define genre_preset_rebuild_raw(p, era, pl) heavy_psych_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   heavy_psych_trax_apply(p, pl, what)
#define genre_era_apply_base(p, era)     heavy_psych_era_apply_base(p, era)
#define genre_apply_defaults(p)          heavy_psych_apply_defaults(p)
#elif defined(GENRE_HOUSE)
#define genre_clamp_era(era)       house_clamp_era(era)
#define genre_clamp_preset(ix)     house_clamp_trax(ix)
#define genre_era_name(era)        house_era_name(era)
#define genre_preset_name(ix)      house_trax_name(ix)
#define genre_preset(ix)           house_trax(ix)
#define genre_era_of_preset(pl)    house_era_of_trax(pl)
#define genre_era_first_preset(er) house_era_first_trax(er)
#define genre_preset_rebuild_raw(p, era, pl) house_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   house_trax_apply(p, pl, what)
#define genre_era_apply_base(p, era)     house_era_apply_base(p, era)
#define genre_apply_defaults(p)          house_apply_defaults(p)
#elif defined(GENRE_JAZZ)
#define genre_clamp_era(era)       jazz_clamp_era(era)
#define genre_clamp_preset(ix)     jazz_clamp_trax(ix)
#define genre_era_name(era)        jazz_era_name(era)
#define genre_preset_name(ix)      jazz_trax_name(ix)
#define genre_preset(ix)           jazz_trax(ix)
#define genre_era_of_preset(pl)    jazz_era_of_trax(pl)
#define genre_era_first_preset(er) jazz_era_first_trax(er)
#define genre_preset_rebuild_raw(p, era, pl) jazz_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   jazz_trax_apply(p, pl, what)
#define genre_era_apply_base(p, era)     jazz_era_apply_base(p, era)
#define genre_apply_defaults(p)          jazz_apply_defaults(p)
#elif defined(GENRE_IDM)
#define genre_clamp_era(era)       idm_clamp_era(era)
#define genre_clamp_preset(ix)     idm_clamp_piece(ix)
#define genre_era_name(era)        idm_era_name(era)
#define genre_preset_name(ix)      idm_piece_name(ix)
#define genre_preset(ix)           idm_piece(ix)
#define genre_era_of_preset(pl)    idm_era_of_piece(pl)
#define genre_era_first_preset(er) idm_era_first_piece(er)
#define genre_preset_rebuild_raw(p, era, pl) idm_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   idm_piece_apply(p, pl, what)
#define genre_era_apply_base(p, era)     idm_era_apply_base(p, era)
#define genre_apply_defaults(p)          idm_apply_defaults(p)
#elif defined(GENRE_HIPHOP)
#define genre_clamp_era(era)       hiphop_clamp_era(era)
#define genre_clamp_preset(ix)     hiphop_clamp_beat(ix)
#define genre_era_name(era)        hiphop_era_name(era)
#define genre_preset_name(ix)      hiphop_beat_name(ix)
#define genre_preset(ix)           hiphop_beat(ix)
#define genre_era_of_preset(pl)    hiphop_era_of_beat(pl)
#define genre_era_first_preset(er) hiphop_era_first_beat(er)
#define genre_preset_rebuild_raw(p, era, pl) hiphop_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   hiphop_beat_apply(p, pl, what)
#define genre_era_apply_base(p, era)     hiphop_era_apply_base(p, era)
#define genre_apply_defaults(p)          hiphop_apply_defaults(p)
#elif defined(GENRE_SNOWCRASH)
#define genre_clamp_era(era)       snowcrash_clamp_era(era)
#define genre_clamp_preset(ix)     snowcrash_clamp_slab(ix)
#define genre_era_name(era)        snowcrash_era_name(era)
#define genre_preset_name(ix)      snowcrash_slab_name(ix)
#define genre_preset(ix)           snowcrash_slab(ix)
#define genre_era_of_preset(pl)    snowcrash_era_of_slab(pl)
#define genre_era_first_preset(er) snowcrash_era_first_slab(er)
#define genre_preset_rebuild_raw(p, era, pl) snowcrash_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   snowcrash_slab_apply(p, pl, what)
#define genre_era_apply_base(p, era)     snowcrash_era_apply_base(p, era)
#define genre_apply_defaults(p)          snowcrash_apply_defaults(p)
#elif defined(GENRE_FUNK)
#define genre_clamp_era(era)       funk_clamp_era(era)
#define genre_clamp_preset(ix)     funk_clamp_groove(ix)
#define genre_era_name(era)        funk_era_name(era)
#define genre_preset_name(ix)      funk_groove_name(ix)
#define genre_preset(ix)           funk_groove(ix)
#define genre_era_of_preset(pl)    funk_era_of_groove(pl)
#define genre_era_first_preset(er) funk_era_first_groove(er)
#define genre_preset_rebuild_raw(p, era, pl) funk_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   funk_groove_apply(p, pl, what)
#define genre_era_apply_base(p, era)     funk_era_apply_base(p, era)
#define genre_apply_defaults(p)          funk_apply_defaults(p)
#elif defined(GENRE_POP)
#define genre_clamp_era(era)       pop_clamp_era(era)
#define genre_clamp_preset(ix)     pop_clamp_plate(ix)
#define genre_era_name(era)        pop_era_name(era)
#define genre_preset_name(ix)      pop_plate_name(ix)
#define genre_preset(ix)           pop_plate(ix)
#define genre_era_of_preset(pl)    pop_era_of_plate(pl)
#define genre_era_first_preset(er) pop_era_first_plate(er)
#define genre_preset_rebuild_raw(p, era, pl) pop_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   pop_plate_apply(p, pl, what)
#define genre_era_apply_base(p, era)     pop_era_apply_base(p, era)
#define genre_apply_defaults(p)          pop_apply_defaults(p)
#elif defined(GENRE_TECHNO)
#define genre_clamp_era(era)       techno_clamp_era(era)
#define genre_clamp_preset(ix)     techno_clamp_loop(ix)
#define genre_era_name(era)        techno_era_name(era)
#define genre_preset_name(ix)      techno_loop_name(ix)
#define genre_preset(ix)           techno_loop(ix)
#define genre_era_of_preset(pl)    techno_era_of_loop(pl)
#define genre_era_first_preset(er) techno_era_first_loop(er)
#define genre_preset_rebuild_raw(p, era, pl) techno_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   techno_loop_apply(p, pl, what)
#define genre_era_apply_base(p, era)     techno_era_apply_base(p, era)
#define genre_apply_defaults(p)          techno_apply_defaults(p)
#elif defined(GENRE_TRIPHOP)
#define genre_clamp_era(era)       triphop_clamp_era(era)
#define genre_clamp_preset(ix)     triphop_clamp_loop(ix)
#define genre_era_name(era)        triphop_era_name(era)
#define genre_preset_name(ix)      triphop_loop_name(ix)
#define genre_preset(ix)           triphop_loop(ix)
#define genre_era_of_preset(pl)    triphop_era_of_loop(pl)
#define genre_era_first_preset(er) triphop_era_first_loop(er)
#define genre_preset_rebuild_raw(p, era, pl) triphop_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   triphop_loop_apply(p, pl, what)
#define genre_era_apply_base(p, era)     triphop_era_apply_base(p, era)
#define genre_apply_defaults(p)          triphop_apply_defaults(p)
#elif defined(GENRE_SOUL)
#define genre_clamp_era(era)       soul_clamp_era(era)
#define genre_clamp_preset(ix)     soul_clamp_side(ix)
#define genre_era_name(era)        soul_era_name(era)
#define genre_preset_name(ix)      soul_side_name(ix)
#define genre_preset(ix)           soul_side(ix)
#define genre_era_of_preset(pl)    soul_era_of_side(pl)
#define genre_era_first_preset(er) soul_era_first_side(er)
#define genre_preset_rebuild_raw(p, era, pl) soul_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   soul_trax_apply(p, pl, what)
#define genre_era_apply_base(p, era)     soul_era_apply_base(p, era)
#define genre_apply_defaults(p)          soul_apply_defaults(p)
#elif defined(GENRE_BASS)
#define genre_clamp_era(era)       bass_clamp_era(era)
#define genre_clamp_preset(ix)     bass_clamp_line(ix)
#define genre_era_name(era)        bass_era_name(era)
#define genre_preset_name(ix)      bass_line_name(ix)
#define genre_preset(ix)           bass_line(ix)
#define genre_era_of_preset(pl)    bass_era_of_line(pl)
#define genre_era_first_preset(er) bass_era_first_line(er)
#define genre_preset_rebuild_raw(p, era, pl) bass_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   bass_line_apply(p, pl, what)
#define genre_era_apply_base(p, era)     bass_era_apply_base(p, era)
#define genre_apply_defaults(p)          bass_apply_defaults(p)
#else
#define genre_clamp_era(era)       dub_clamp_era(era)
#define genre_clamp_preset(ix)     dub_clamp_plate(ix)
#define genre_era_name(era)        dub_era_name(era)
#define genre_preset_name(ix)      dub_plate_name(ix)
#define genre_preset(ix)           dub_plate(ix)
#define genre_era_of_preset(pl)    dub_era_of_plate(pl)
#define genre_era_first_preset(er) dub_era_first_plate(er)
#define genre_preset_rebuild_raw(p, era, pl) dub_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   dub_plate_apply(p, pl, what)
#define genre_era_apply_base(p, era)     dub_era_apply_base(p, era)
#define genre_apply_defaults(p)          dub_apply_defaults(p)
#endif

/* Factory plates predate LEAD DRIVE (drm_drv[3]) and LEAD PUMP (pan[3]); both start off. */
static inline void genre_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t pl)
{
    genre_preset_rebuild_raw(p, era, pl);
    p->drm_drv[3] = 0u;
    p->pan[3] = 0u;
}

#ifdef GENRE_HIPHOP
#define genre_lead_step_bit(b, s)     hiphop_beat_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        hiphop_beat_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       hiphop_beat_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      hiphop_beat_lead_bend_u7(b, s)
#define genre_lead_bend_plock(u)      hiphop_lead_bend_plock(u)
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)
#define genre_lead_step_bit(b, s)     idm_piece_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        idm_piece_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       idm_piece_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      idm_piece_lead_bend_u7(b, s)
#define genre_lead_bend_plock(u)      idm_lead_bend_plock(u)
#elif defined(GENRE_JAZZ)
#define genre_lead_step_bit(b, s)     jazz_trax_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        jazz_trax_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       jazz_trax_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      jazz_trax_lead_bend_u7(b, s)
#define genre_lead_bend_plock(u)      jazz_lead_bend_plock(u)
#elif defined(GENRE_SOUL)
#define genre_lead_step_bit(b, s)     soul_side_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        soul_side_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       soul_side_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      soul_side_lead_bend_u7(b, s)
#define genre_lead_bend_plock(u)      soul_lead_bend_plock(u)
#elif defined(GENRE_HEAVY_PSYCH)
#define genre_lead_step_bit(b, s)     heavy_psych_trax_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        heavy_psych_trax_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       heavy_psych_trax_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      heavy_psych_trax_lead_bend_u7(b, s)
#define genre_lead_bend_plock(u)      heavy_psych_lead_bend_plock(u)
#endif

#ifdef GENRE_HIPHOP
#define genre_lead_gate32(b)          hiphop_beat_lead_gate32(b)
#elif defined(GENRE_IDM)
#define genre_lead_gate32(b)          idm_piece_lead_gate32(b)
#elif defined(GENRE_SNOWCRASH)
#define genre_lead_gate32(b)          snowcrash_slab_lead_gate32(b)
#elif defined(GENRE_JAZZ)
#define genre_lead_gate32(b)          jazz_trax_lead_gate32(b)
#elif defined(GENRE_SOUL)
#define genre_lead_gate32(b)          soul_side_lead_gate32(b)
#elif defined(GENRE_HEAVY_PSYCH)
#define genre_lead_gate32(b)          heavy_psych_trax_lead_gate32(b)
#elif defined(GENRE_TECHNO)
#define genre_lead_step_bit(b, s)     techno_loop_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        techno_loop_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       techno_loop_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      techno_loop_lead_bend_u7(b, s)
#define genre_lead_gate32(b)          techno_loop_lead_gate32(b)
#elif defined(GENRE_TRIPHOP)
#define genre_lead_step_bit(b, s)     triphop_loop_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        triphop_loop_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       triphop_loop_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      triphop_loop_lead_bend_u7(b, s)
#define genre_lead_bend_plock(u)      triphop_lead_bend_plock(u)
#define genre_lead_gate32(b)          triphop_loop_lead_gate32(b)
#elif defined(GENRE_BASS)
#define genre_lead_step_bit(b, s)     bass_line_lead_step_bit(b, s)
#define genre_lead_pitch(b, s)        bass_line_lead_pitch(b, s)
#define genre_lead_len_ix(b, s)       bass_line_lead_len_ix(b, s)
#define genre_lead_bend_u7(b, s)      bass_line_lead_bend_u7(b, s)
#define genre_lead_gate32(b)          bass_line_lead_gate32(b)
#endif

/* Lane-3 GATE p-lock: note-length index in step_micro[3]. One table set per genre for knob, readout, pot and
 * grid span. PLATE_DEF: unlocked plate lead steps use the plate length (hp_lead_arm_siren_plock).
 * TIME_DEF: unlocked steps use the voice's fixed note gate (len_plock_on = 0). LEN_STORE: index -> step_micro. */
#if defined(GENRE_HEAVY_PSYCH)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  heavy_psych_lead_len_ix(r)
#define FM_L3_LEN_N      HP_LEAD_LEN_N
#define FM_L3_LEN_NAMES  HP_LEAD_LEN_NAMES
#define FM_L3_LEN_16THS  HP_LEAD_LEN_16THS
#define FM_L3_PLATE_DEF  1
#elif defined(GENRE_FUNK)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  funk_stab_gate_len_ix(r)
#define FM_L3_LEN_N      FUNK_STAB_GATE_LEN_N
#define FM_L3_LEN_NAMES  FUNK_STAB_GATE_LEN_NAMES
#define FM_L3_LEN_16THS  FUNK_STAB_GATE_LEN_16THS
#define FM_L3_TIME_DEF   funk_stab_note_gate
#elif defined(GENRE_HOUSE)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  house_stab_gate_len_ix(r)
#define FM_L3_LEN_N      HOUSE_STAB_GATE_LEN_N
#define FM_L3_LEN_NAMES  HOUSE_STAB_GATE_LEN_NAMES
#define FM_L3_LEN_16THS  HOUSE_STAB_GATE_LEN_16THS
#define FM_L3_TIME_DEF   house_stab_note_gate
#elif defined(GENRE_JAZZ)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  jazz_lead_len_ix(r)
#define FM_L3_LEN_N      JAZZ_LEAD_LEN_N
#define FM_L3_LEN_NAMES  JAZZ_LEAD_LEN_NAMES
#define FM_L3_LEN_16THS  JAZZ_LEAD_LEN_16THS
#define FM_L3_PLATE_DEF  1
#elif defined(GENRE_SOUL)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  soul_lead_len_ix(r)
#define FM_L3_LEN_N      SOUL_LEAD_LEN_N
#define FM_L3_LEN_NAMES  SOUL_LEAD_LEN_NAMES
#define FM_L3_LEN_16THS  SOUL_LEAD_LEN_16THS
#define FM_L3_PLATE_DEF  1
#elif defined(GENRE_HIPHOP)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  hiphop_lead_len_ix(r)
#define FM_L3_LEN_N      HIPHOP_LEAD_LEN_N
#define FM_L3_LEN_NAMES  HIPHOP_LEAD_LEN_NAMES
#define FM_L3_LEN_16THS  HIPHOP_LEAD_LEN_16THS
#define FM_L3_PLATE_DEF  1
#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  idm_lead_len_ix(r)
#define FM_L3_LEN_N      IDM_LEAD_LEN_N
#define FM_L3_LEN_NAMES  IDM_LEAD_LEN_NAMES
#define FM_L3_LEN_16THS  IDM_LEAD_LEN_16THS
#if !defined(GENRE_TECHNO)
#define FM_L3_PLATE_DEF  1
#endif
#elif defined(GENRE_POP)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)    pop_lead_len_ix(r)
#define FM_L3_LEN_N        POP_LEAD_LEN_N
#define FM_L3_LEN_NAMES    POP_LEAD_LEN_NAMES
#define FM_L3_LEN_16THS    POP_LEAD_LEN_16THS
#define FM_L3_LEN_STORE(i) pop_lead_len_store(i)
#elif !defined(GENRE_BASS)
#define FM_L3_GATE 1
#define FM_L3_LEN_IX(r)  dub_siren_gate_len_ix(r)
#define FM_L3_LEN_N      DUB_SIREN_GATE_LEN_N
#define FM_L3_LEN_NAMES  DUB_SIREN_GATE_LEN_NAMES
#define FM_L3_LEN_16THS  DUB_SIREN_GATE_LEN_16THS
#define FM_L3_TIME_DEF   dub_siren_note_gate
#endif
#if defined(FM_L3_GATE) && !defined(FM_L3_LEN_STORE)
#define FM_L3_LEN_STORE(i) ((int8_t)(i))
#endif

/* Endless plate continuum (live generated plate shadows one ROM slot). The genre data header opts in by
 * defining FM_PGEN_STATE (its fm_pgen_t) and FM_PGEN_BUILD(era, n, comply); see docs/plate-gen-recipe.md. */
#if defined(FM_PGEN_STATE)
#define FM_PGEN_ON 1
#define genre_pgen_build(era, n, c)   FM_PGEN_BUILD(era, n, c)
#define genre_pgen_slot()             (FM_PGEN_STATE.slot)
#define genre_pgen_off()              fm_pgen_off(&FM_PGEN_STATE)
#define genre_pgen_views_hit(pl)      fm_pgen_views_hit(&FM_PGEN_STATE, pl)
#define genre_pgen_view(ln, v)        (FM_PGEN_STATE.views[ln][v])
#endif

#include "fm_views_data.h"

#endif
