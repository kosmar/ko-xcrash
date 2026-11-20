/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef POP_CONF_H
#define POP_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u /* 6 plate/era flash layout */
#define GENRE_CFG_MAGIC  0x4458464Eu /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO POP"
#define GENRE_FELUCCA_ID "FM-1_56407"

#define GENRE_PRESET_LABEL "TRACK"
#define GENRE_PRESET_HDR   "TRK"

#define GENRE_C_ACC RGB(180, 220, 255)

#define GENRE_BANK_COL \
    { RGB(200, 215, 230), RGB(180, 200, 220), RGB(160, 190, 210), RGB(140, 175, 200), \
      RGB(120, 165, 195), RGB(100, 155, 185) }

#define GENRE_ERA_COL \
    { RGB(220, 230, 240), RGB(255, 200, 120), RGB(200, 180, 255), RGB(120, 220, 255), \
      RGB(180, 210, 255), RGB(255, 160, 90) }

#endif
