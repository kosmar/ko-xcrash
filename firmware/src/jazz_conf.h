/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef JAZZ_CONF_H
#define JAZZ_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_PRESET_N_LEGACY 36u /* 6 trax/era flash layout */
#define GENRE_CFG_MAGIC  0x4A415A5Cu /* bump: user_song + song_name in cfg */
#define GENRE_APP_NAME   "KO JAZZ"
#define GENRE_FELUCCA_ID "FM-1_56404"

#define GENRE_PRESET_LABEL "TUNE"
#define GENRE_PRESET_HDR   "TUN"

#define GENRE_C_ACC RGB(255, 210, 120)

#define GENRE_BANK_COL \
    { RGB(180, 220, 255), RGB(255, 180, 90), RGB(120, 200, 160), RGB(200, 140, 255), \
      RGB(255, 120, 160), RGB(140, 180, 255) }

#define GENRE_ERA_COL \
    { RGB(255, 50, 45), RGB(255, 130, 25), RGB(255, 220, 35), RGB(45, 215, 65), \
      RGB(35, 200, 255), RGB(210, 55, 255) }

#endif
