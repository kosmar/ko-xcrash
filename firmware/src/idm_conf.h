/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef IDM_CONF_H
#define IDM_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u /* 6 piece/era flash layout */
#define GENRE_CFG_MAGIC  0x49444D43u /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO IDM"
#define GENRE_FELUCCA_ID "FM-1_56405"

#define GENRE_PRESET_LABEL "PIECE"
#define GENRE_PRESET_HDR   "PCE"

#define GENRE_C_ACC RGB(140, 220, 255)

#define GENRE_BANK_COL \
    { RGB(200, 180, 160), RGB(160, 200, 240), RGB(180, 255, 200), RGB(255, 160, 220), \
      RGB(255, 200, 120), RGB(160, 140, 255) }

#define GENRE_ERA_COL \
    { RGB(255, 50, 45), RGB(255, 130, 25), RGB(255, 220, 35), RGB(45, 215, 65), \
      RGB(35, 200, 255), RGB(210, 55, 255) }

#endif
