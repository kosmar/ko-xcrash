/* SPDX-License-Identifier: GPL-3.0-only */
/* Per-genre DJ density + gesture caps (include after genre_conf.h, before fm_dj.h). */
#ifndef FM_DJ_CONF_H
#define FM_DJ_CONF_H

/*
 * Jazz is the intentional sparse ceiling for planner / FX rates.
 * Club genres keep fill (and loop where enabled) but use the same or stricter pacing.
 * Macro-slot gaps: +8 bars on every scheduled cooldown, clamp 20 (legacy bias 0 / cap 8).
 */
#define FM_DJ_SPAWN_GAP_BIAS 8u
#define FM_DJ_SPAWN_GAP_CAP 20u
#if defined(GENRE_JAZZ)

#undef FM_DJ_GEST_CAP_FILL
#define FM_DJ_GEST_CAP_FILL 0u
#undef FM_DJ_GEST_CAP_LOOP
#define FM_DJ_GEST_CAP_LOOP 0u

#define FM_DJ_SPAWN_INTRO_CD_MAX 8u
#define FM_DJ_SPAWN_SKIP_NUM 6u
#define FM_DJ_GEST_PICK_DENOM 29u
#define FM_DJ_GEST_ARP_RARE 23u
#define FM_DJ_GEST_ARP_CD_BARS 64u
#define FM_DJ_GEST_FLAM_CD_BARS 32u
#define FM_DJ_GEST_LOOP_RARE 127u
#define FM_DJ_GEST_LOOP_CD_BARS 80u
#define FM_DJ_FX_RIDE_RARE 43u
#define FM_DJ_FX_BREAK_RARE 9u
#define FM_DJ_SEQ_FX_ENS_INTRO_NUM 13u
#define FM_DJ_SEQ_FX_ENS_MID_NUM 11u
#define FM_DJ_SEQ_FX_BUILD_TAIL_NUM 7u
#define FM_DJ_SEQ_FX_BUILD_NUM 11u
#define FM_DJ_SEQ_FX_FLIP_NUM 11u

#elif defined(GENRE_POP) || defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) \
    || defined(GENRE_TRIPHOP) || defined(GENRE_HIPHOP) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) \
    || defined(GENRE_SOUL)

#define FM_DJ_GEST_CAP_FILL 1u

#define FM_DJ_SPAWN_INTRO_CD_MAX 8u
#define FM_DJ_SPAWN_SKIP_NUM 6u
#define FM_DJ_GEST_PICK_DENOM 31u
#define FM_DJ_GEST_ARP_RARE 25u
#define FM_DJ_GEST_ARP_CD_BARS 72u
#define FM_DJ_GEST_FLAM_CD_BARS 32u
#if defined(GENRE_HOUSE)
#undef FM_DJ_SPAWN_GAP_BIAS
#define FM_DJ_SPAWN_GAP_BIAS 14u
#undef FM_DJ_SPAWN_GAP_CAP
#define FM_DJ_SPAWN_GAP_CAP 28u
#undef FM_DJ_GEST_PICK_DENOM
#define FM_DJ_GEST_PICK_DENOM 67u
#undef FM_DJ_GEST_FLAM_CD_BARS
#define FM_DJ_GEST_FLAM_CD_BARS 48u
#undef FM_DJ_GEST_FLAM_RARE_DENOM
#define FM_DJ_GEST_FLAM_RARE_DENOM 85u
#define FM_DJ_GEST_LOOP_RARE 88u
#define FM_DJ_GEST_LOOP_CD_BARS 80u
#else
#define FM_DJ_GEST_LOOP_RARE 127u
#define FM_DJ_GEST_LOOP_CD_BARS 80u
#endif
#define FM_DJ_FX_RIDE_RARE 47u
#define FM_DJ_FX_BREAK_RARE 10u
#define FM_DJ_SEQ_FX_ENS_INTRO_NUM 13u
#define FM_DJ_SEQ_FX_ENS_MID_NUM 11u
#define FM_DJ_SEQ_FX_BUILD_TAIL_NUM 7u
#define FM_DJ_SEQ_FX_BUILD_NUM 11u
#define FM_DJ_SEQ_FX_FLIP_NUM 11u

#elif defined(GENRE_HEAVY_PSYCH)

/* Heavy psych: same scheduler, no fill — match jazz pacing on rates. */
#define FM_DJ_SPAWN_INTRO_CD_MAX 8u
#define FM_DJ_SPAWN_SKIP_NUM 6u
#define FM_DJ_GEST_PICK_DENOM 29u
#define FM_DJ_GEST_ARP_RARE 23u
#define FM_DJ_GEST_ARP_CD_BARS 64u
#define FM_DJ_GEST_FLAM_CD_BARS 32u
#define FM_DJ_GEST_LOOP_RARE 127u
#define FM_DJ_GEST_LOOP_CD_BARS 80u
#define FM_DJ_FX_RIDE_RARE 43u
#define FM_DJ_FX_BREAK_RARE 9u
#define FM_DJ_SEQ_FX_ENS_INTRO_NUM 13u
#define FM_DJ_SEQ_FX_ENS_MID_NUM 11u
#define FM_DJ_SEQ_FX_BUILD_TAIL_NUM 7u
#define FM_DJ_SEQ_FX_BUILD_NUM 11u
#define FM_DJ_SEQ_FX_FLIP_NUM 11u

#else

/* Dub (default ROM): thinner phrase + macro pacing than club defaults. */
#undef FM_DJ_SPAWN_GAP_BIAS
#define FM_DJ_SPAWN_GAP_BIAS 18u
#undef FM_DJ_SPAWN_GAP_CAP
#define FM_DJ_SPAWN_GAP_CAP 36u
#define FM_DJ_SPAWN_INTRO_CD_MAX 8u
#define FM_DJ_SPAWN_SKIP_NUM 6u
#define FM_DJ_GEST_PICK_DENOM 100u
#define FM_DJ_GEST_ARP_RARE 36u
#define FM_DJ_GEST_ARP_CD_BARS 64u
#define FM_DJ_GEST_FLAM_RARE_DENOM 128u
#define FM_DJ_GEST_FLAM_CD_BARS 64u
#define FM_DJ_GEST_LOOP_RARE 127u
#define FM_DJ_GEST_LOOP_CD_BARS 80u
#define FM_DJ_FX_RIDE_RARE 64u
#define FM_DJ_FX_BREAK_RARE 13u
#define FM_DJ_SEQ_FX_ENS_INTRO_NUM 26u
#define FM_DJ_SEQ_FX_ENS_MID_NUM 22u
#define FM_DJ_SEQ_FX_BUILD_TAIL_NUM 15u
#define FM_DJ_SEQ_FX_BUILD_NUM 22u
#define FM_DJ_SEQ_FX_FLIP_NUM 22u

#endif

#endif /* FM_DJ_CONF_H */
