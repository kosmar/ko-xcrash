/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HOUSE_CONF_H
#define HOUSE_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u /* 6 trax/era flash layout */
#define GENRE_CFG_MAGIC  0x484F5541u /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO HOUSE"
#define GENRE_FELUCCA_ID "FM-1_56402"

#define GENRE_PRESET_LABEL "TRAX"
#define GENRE_PRESET_HDR   "TRX"

#define GENRE_C_ACC RGB(255, 180, 90)

#define GENRE_BANK_COL \
    { RGB(220, 80, 220), RGB(255, 200, 60), RGB(140, 240, 80), RGB(160, 100, 255), \
      RGB(80, 220, 255), RGB(255, 180, 90) }

#define GENRE_ERA_COL \
    { RGB(255, 200, 60), RGB(160, 90, 255), RGB(180, 255, 60), RGB(70, 140, 255), \
      RGB(255, 60, 200), RGB(80, 230, 240) }

#endif
