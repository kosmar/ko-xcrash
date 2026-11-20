/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SNOWCRASH_CONF_H
#define SNOWCRASH_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_CFG_USER_PLATE_N 60u /* ROM 60 SLAB; flash user saves ≤64 (uint64_t mask) */
#define GENRE_PRESET_N_LEGACY 48u /* 6 slab/era flash layout */
#define GENRE_CFG_MAGIC  0x534E4335u /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO XCRASH"
#define GENRE_FELUCCA_ID "FM-1_56411"

#define GENRE_PRESET_LABEL "SLAB"
#define GENRE_PRESET_HDR   "SLB"

#define GENRE_C_ACC RGB(180, 220, 255)

#define GENRE_BANK_COL \
    { RGB(200, 210, 240), RGB(160, 180, 220), RGB(140, 160, 200), RGB(120, 140, 190), \
      RGB(100, 130, 180), RGB(90, 120, 170) }

/* CRASH lime → GLOSS magenta → SUMER teal → STATIC amber → TIDE ice → CHAPEL violet */
#define GENRE_ERA_COL \
    { RGB(170, 255, 60), RGB(255, 70, 190), RGB(40, 220, 170), RGB(255, 200, 40), \
      RGB(120, 200, 255), RGB(170, 110, 255) }

#endif
