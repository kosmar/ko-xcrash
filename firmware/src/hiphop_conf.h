/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HIPHOP_CONF_H
#define HIPHOP_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u /* 6 beat/era flash layout */
#define GENRE_CFG_MAGIC  0x4849504Au /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO HIPHOP"
#define GENRE_FELUCCA_ID "FM-1_56406"

#define GENRE_PRESET_LABEL "BEAT"
#define GENRE_PRESET_HDR   "BEA"

#define GENRE_C_ACC RGB(255, 168, 48)

/* PARK brick · GOLD amber · BOOM hot pink · GRIME acid · TRAP ice · CLOUD violet */
#define GENRE_BANK_COL \
    { RGB(220, 64, 48), RGB(255, 188, 36), RGB(255, 56, 148), RGB(96, 240, 40), \
      RGB(24, 220, 255), RGB(168, 112, 255) }

#define GENRE_ERA_COL \
    { RGB(232, 48, 40), RGB(255, 196, 32), RGB(255, 48, 160), RGB(88, 255, 36), \
      RGB(16, 228, 255), RGB(176, 104, 255) }

#endif
